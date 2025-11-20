/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "audio_decoder.h"
#include <cstdio>
#include <vector>

// 包含 minimp3.h 以获取类型定义（但不定义实现）
// 实现将在 .cpp 文件中定义
#ifndef MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#endif

namespace audio_decoder {

class Mp3Decoder : public AudioDecoder {
private:
    FILE* _file = nullptr;
    mp3dec_t _mp3d;  // 直接使用类型，不需要指针
    std::vector<int16_t> _decode_buffer;  // 解码缓冲区
    size_t _current_pos = 0;              // 当前文件位置（字节）
    AudioInfo _info;
    bool _is_open = false;
    bool _info_parsed = false;
    
    bool parseInfo();
    
public:
    Mp3Decoder();
    ~Mp3Decoder() override;
    
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

