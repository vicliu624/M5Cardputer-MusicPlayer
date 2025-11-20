/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "mp3_decoder.h"
#include <mooncake_log.h>
#include <cstring>
#include <algorithm>

// 定义实现（必须在包含头文件之后）
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

namespace audio_decoder {

Mp3Decoder::Mp3Decoder() {
    mp3dec_init(&_mp3d);
    _decode_buffer.resize(MINIMP3_MAX_SAMPLES_PER_FRAME * 2); // 立体声最大帧
}

Mp3Decoder::~Mp3Decoder() {
    close();
}

bool Mp3Decoder::parseInfo() {
    if (_info_parsed) {
        return true;
    }
    
    if (!_file) {
        return false;
    }
    
    // 保存当前位置
    long saved_pos = ftell(_file);
    rewind(_file);
    
    // 读取第一帧以获取信息
    uint8_t header[4];
    if (fread(header, 1, 4, _file) != 4) {
        fseek(_file, saved_pos, SEEK_SET);
        return false;
    }
    
    // 检查是否是有效的 MP3 帧头
    if (header[0] != 0xFF || (header[1] & 0xE0) != 0xE0) {
        // 可能是 ID3 标签，跳过
        if (header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
            // 读取 ID3 标签大小
            uint8_t id3_size[4];
            if (fread(id3_size, 1, 4, _file) == 4) {
                // ID3v2 标签大小是 7-bit 编码
                size_t id3_tag_size = (id3_size[0] << 21) | (id3_size[1] << 14) | 
                                      (id3_size[2] << 7) | id3_size[3];
                id3_tag_size += 10; // 包含头部
                fseek(_file, id3_tag_size, SEEK_SET);
                // 重新读取帧头
                if (fread(header, 1, 4, _file) != 4) {
                    fseek(_file, saved_pos, SEEK_SET);
                    return false;
                }
            }
        } else {
            fseek(_file, saved_pos, SEEK_SET);
            return false;
        }
    }
    
    // 使用 minimp3 解码第一帧
    mp3dec_t temp_mp3d;
    mp3dec_init(&temp_mp3d);
    
    // 读取足够的字节来解码第一帧（最多 1441 字节）
    uint8_t frame_buffer[1441];
    size_t bytes_read = fread(frame_buffer, 1, sizeof(frame_buffer), _file);
    
    mp3dec_frame_info_t frame_info;
    int16_t dummy_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    int samples = mp3dec_decode_frame(&temp_mp3d, frame_buffer, bytes_read, dummy_pcm, &frame_info);
    
    if (samples > 0 && frame_info.bitrate_kbps > 0) {
        _info.sample_rate = frame_info.hz;
        _info.channels = frame_info.channels;
        _info.bits_per_sample = 16;
        _info.total_samples = 0; // MP3 总长度未知
        _info_parsed = true;
        
        mclog::tagInfo("Mp3Decoder", "MP3 file: {}Hz, {}ch, {}kbps",
                       _info.sample_rate, _info.channels, frame_info.bitrate_kbps);
        
        // 恢复文件位置
        fseek(_file, saved_pos, SEEK_SET);
        return true;
    }
    
    // 恢复文件位置
    fseek(_file, saved_pos, SEEK_SET);
    return false;
}

bool Mp3Decoder::open(const uint8_t* data, size_t size) {
    (void)data;
    (void)size;
    // 不支持内存数据，只支持文件
    return false;
}

bool Mp3Decoder::open(const std::string& filepath) {
    close();
    
    _file = fopen(filepath.c_str(), "rb");
    if (!_file) {
        mclog::tagError("Mp3Decoder", "Failed to open file: {}", filepath);
        return false;
    }
    
    // 初始化解码器
    mp3dec_init(&_mp3d);
    _current_pos = 0;
    _info_parsed = false;
    
    // 解析文件信息
    if (!parseInfo()) {
        fclose(_file);
        _file = nullptr;
        return false;
    }
    
    // 重置到文件开始（跳过 ID3 标签）
    rewind(_file);
    _current_pos = 0;
    
    // 预读一些数据以跳过 ID3 标签
    uint8_t header[4];
    if (fread(header, 1, 4, _file) == 4) {
        if (header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
            uint8_t id3_size[4];
            if (fread(id3_size, 1, 4, _file) == 4) {
                size_t id3_tag_size = (id3_size[0] << 21) | (id3_size[1] << 14) | 
                                      (id3_size[2] << 7) | id3_size[3];
                id3_tag_size += 10;
                fseek(_file, id3_tag_size, SEEK_SET);
                _current_pos = id3_tag_size;
            }
        } else {
            // 不是 ID3，重置到开始
            rewind(_file);
            _current_pos = 0;
        }
    }
    
    _is_open = true;
    return true;
}

bool Mp3Decoder::getInfo(AudioInfo& info) {
    if (!_is_open) {
        return false;
    }
    info = _info;
    return true;
}

size_t Mp3Decoder::decode(int16_t* pcm_buffer, size_t max_samples) {
    if (!_is_open || !hasMore()) {
        return 0;
    }
    
    size_t total_decoded = 0;
    
    // 读取 MP3 数据到缓冲区
    constexpr size_t MP3_BUFFER_SIZE = 1441; // 足够解码一帧
    uint8_t mp3_buffer[MP3_BUFFER_SIZE];
    
    while (total_decoded < max_samples && hasMore()) {
        // 定位到当前位置
        fseek(_file, _current_pos, SEEK_SET);
        
        // 读取 MP3 数据
        size_t bytes_read = fread(mp3_buffer, 1, MP3_BUFFER_SIZE, _file);
        if (bytes_read < 4) {
            break; // 数据不足
        }
        
        // 解码一帧
        mp3dec_frame_info_t frame_info;
        int samples = mp3dec_decode_frame(&_mp3d, 
                                          mp3_buffer, 
                                          bytes_read,
                                          _decode_buffer.data(), 
                                          &frame_info);
        
        if (samples <= 0) {
            // 解码失败或文件结束
            break;
        }
        
        // 更新位置
        _current_pos += frame_info.frame_bytes;
        
        // 复制解码数据
        size_t samples_to_copy = std::min(static_cast<size_t>(samples * frame_info.channels),
                                          max_samples - total_decoded);
        
        memcpy(pcm_buffer + total_decoded, 
               _decode_buffer.data(), 
               samples_to_copy * sizeof(int16_t));
        
        total_decoded += samples_to_copy;
        
        // 如果缓冲区已满，停止解码
        if (samples_to_copy < static_cast<size_t>(samples * frame_info.channels)) {
            break;
        }
    }
    
    return total_decoded;
}

bool Mp3Decoder::reset() {
    if (!_is_open || !_file) {
        return false;
    }
    
    // 重新初始化解码器
    mp3dec_init(&_mp3d);
    
    // 重置文件位置
    rewind(_file);
    _current_pos = 0;
    
    // 跳过 ID3 标签
    uint8_t header[4];
    if (fread(header, 1, 4, _file) == 4) {
        if (header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
            uint8_t id3_size[4];
            if (fread(id3_size, 1, 4, _file) == 4) {
                size_t id3_tag_size = (id3_size[0] << 21) | (id3_size[1] << 14) | 
                                      (id3_size[2] << 7) | id3_size[3];
                id3_tag_size += 10;
                fseek(_file, id3_tag_size, SEEK_SET);
                _current_pos = id3_tag_size;
            }
        } else {
            rewind(_file);
            _current_pos = 0;
        }
    }
    
    return true;
}

void Mp3Decoder::close() {
    if (_file) {
        fclose(_file);
        _file = nullptr;
    }
    _is_open = false;
    _info_parsed = false;
    _current_pos = 0;
}

bool Mp3Decoder::hasMore() const {
    if (!_is_open || !_file) {
        return false;
    }
    
    // 检查是否到达文件末尾
    long current = ftell(const_cast<FILE*>(_file));
    fseek(const_cast<FILE*>(_file), 0, SEEK_END);
    long end = ftell(const_cast<FILE*>(_file));
    fseek(const_cast<FILE*>(_file), current, SEEK_SET);
    
    return _current_pos < static_cast<size_t>(end);
}

size_t Mp3Decoder::getPosition() const {
    // MP3 无法精确计算位置（因为帧大小可变）
    // 返回近似值：基于已解码的帧数
    return 0; // TODO: 可以实现近似计算
}

} // namespace audio_decoder

