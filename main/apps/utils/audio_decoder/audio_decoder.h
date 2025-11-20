/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace audio_decoder {

enum class Format {
    UNKNOWN,
    WAV,
    MP3,
};

struct AudioInfo {
    uint32_t sample_rate = 0;    // 采样率 (Hz)
    uint16_t channels = 0;       // 声道数 (1=单声道, 2=立体声)
    uint16_t bits_per_sample = 0; // 位深 (16, 24, 32)
    size_t total_samples = 0;    // 总采样点数（如果已知）
};

class AudioDecoder {
public:
    virtual ~AudioDecoder() = default;
    
    // 打开音频文件/流
    virtual bool open(const uint8_t* data, size_t size) = 0;
    virtual bool open(const std::string& filepath) = 0;
    
    // 获取音频信息
    virtual bool getInfo(AudioInfo& info) = 0;
    
    // 解码音频数据
    // 返回解码的采样点数（不是字节数）
    virtual size_t decode(int16_t* pcm_buffer, size_t max_samples) = 0;
    
    // 重置解码器（用于重新播放）
    virtual bool reset() = 0;
    
    // 关闭解码器
    virtual void close() = 0;
    
    // 检查是否还有数据
    virtual bool hasMore() const = 0;
    
    // 获取当前解码位置（采样点数）
    virtual size_t getPosition() const = 0;
};

// 工厂函数：根据文件扩展名或数据头创建解码器
AudioDecoder* createDecoder(Format format);
Format detectFormat(const uint8_t* data, size_t size);
Format detectFormat(const std::string& filepath);

} // namespace audio_decoder

