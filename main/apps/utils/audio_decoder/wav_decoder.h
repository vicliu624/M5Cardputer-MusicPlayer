/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "audio_decoder.h"
#include <vector>
#include <cstdio>

namespace audio_decoder {

class WavDecoder : public AudioDecoder {
private:
    struct WavHeader {
        char riff[4];           // "RIFF"
        uint32_t file_size;     // 文件大小 - 8
        char wave[4];           // "WAVE"
        char fmt[4];            // "fmt "
        uint32_t fmt_size;      // fmt chunk 大小
        uint16_t audio_format;  // 1 = PCM
        uint16_t channels;      // 声道数
        uint32_t sample_rate;   // 采样率
        uint32_t byte_rate;     // 字节率
        uint16_t block_align;   // 块对齐
        uint16_t bits_per_sample; // 位深
        char data[4];           // "data"
        uint32_t data_size;     // 数据大小
    };
    
    FILE* _file = nullptr;
    size_t _data_offset = 0;    // PCM 数据起始位置
    size_t _current_pos = 0;    // 当前解码位置（字节）
    AudioInfo _info;
    bool _is_open = false;
    
    bool parseHeader();
    
public:
    ~WavDecoder() override { close(); }
    
    bool open(const uint8_t* data, size_t size) override;
    bool open(const std::string& filepath) override;
    bool getInfo(AudioInfo& info) override;
    size_t decode(int16_t* pcm_buffer, size_t max_samples) override;
    bool reset() override;
    void close() override;
    bool hasMore() const override;
    size_t getPosition() const override;
};

} // namespace audio_decoder

