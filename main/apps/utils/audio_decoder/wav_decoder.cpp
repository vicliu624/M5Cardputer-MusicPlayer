/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "wav_decoder.h"
#include <cstring>
#include <algorithm>
#include <mooncake_log.h>

namespace audio_decoder {

bool WavDecoder::parseHeader() {
    if (!_file) {
        return false;
    }
    
    // 读取 RIFF 头
    WavHeader header;
    if (fread(&header, sizeof(WavHeader), 1, _file) != 1) {
        return false;
    }
    
    // 检查 RIFF 和 WAVE 标识
    if (memcmp(header.riff, "RIFF", 4) != 0 ||
        memcmp(header.wave, "WAVE", 4) != 0) {
        return false;
    }
    
    // 检查格式（只支持 PCM）
    if (header.audio_format != 1) {
        mclog::tagWarn("WavDecoder", "Unsupported audio format: {}", header.audio_format);
        return false; // 不支持非 PCM 格式
    }
    
    // 检查位深（只支持 16-bit）
    if (header.bits_per_sample != 16) {
        mclog::tagWarn("WavDecoder", "Unsupported bits per sample: {}", header.bits_per_sample);
        return false; // 只支持 16-bit
    }
    
    // 填充音频信息
    _info.sample_rate = header.sample_rate;
    _info.channels = header.channels;
    _info.bits_per_sample = header.bits_per_sample;
    _info.total_samples = header.data_size / (header.channels * 2); // 16-bit = 2 bytes
    
    _data_offset = ftell(_file);
    
    mclog::tagInfo("WavDecoder", "WAV file: {}Hz, {}ch, {}bit, {} samples",
                   _info.sample_rate, _info.channels, _info.bits_per_sample, _info.total_samples);
    
    return true;
}

bool WavDecoder::open(const uint8_t* data, size_t size) {
    // 不支持内存数据，只支持文件
    (void)data;
    (void)size;
    return false;
}

bool WavDecoder::open(const std::string& filepath) {
    close();
    
    _file = fopen(filepath.c_str(), "rb");
    if (!_file) {
        mclog::tagError("WavDecoder", "Failed to open file: {}", filepath);
        return false;
    }
    
    _current_pos = 0;
    
    if (!parseHeader()) {
        fclose(_file);
        _file = nullptr;
        return false;
    }
    
    _current_pos = _data_offset;
    _is_open = true;
    return true;
}

bool WavDecoder::getInfo(AudioInfo& info) {
    if (!_is_open) {
        return false;
    }
    info = _info;
    return true;
}

size_t WavDecoder::decode(int16_t* pcm_buffer, size_t max_samples) {
    if (!_is_open || !hasMore()) {
        return 0;
    }
    
    // 定位到当前位置
    fseek(_file, _current_pos, SEEK_SET);
    
    // 计算要读取的字节数
    size_t bytes_per_sample = _info.channels * 2; // 16-bit = 2 bytes
    size_t bytes_to_read = max_samples * bytes_per_sample;
    
    // 读取数据
    size_t bytes_read = fread(pcm_buffer, 1, bytes_to_read, _file);
    size_t samples_read = bytes_read / bytes_per_sample;
    
    _current_pos = ftell(_file);
    
    return samples_read;
}

bool WavDecoder::reset() {
    if (!_is_open) {
        return false;
    }
    _current_pos = _data_offset;
    fseek(_file, _current_pos, SEEK_SET);
    return true;
}

void WavDecoder::close() {
    if (_file) {
        fclose(_file);
        _file = nullptr;
    }
    _data_offset = 0;
    _current_pos = 0;
    _is_open = false;
}

bool WavDecoder::hasMore() const {
    if (!_is_open || !_file) {
        return false;
    }
    
    // 检查是否到达文件末尾
    long current = ftell(const_cast<FILE*>(_file));
    fseek(const_cast<FILE*>(_file), 0, SEEK_END);
    long end = ftell(const_cast<FILE*>(_file));
    fseek(const_cast<FILE*>(_file), current, SEEK_SET);
    
    return current < end;
}

size_t WavDecoder::getPosition() const {
    if (!_is_open) {
        return 0;
    }
    size_t bytes_decoded = _current_pos - _data_offset;
    return bytes_decoded / (_info.channels * 2);
}

} // namespace audio_decoder

