/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "audio_decoder.h"
#include "wav_decoder.h"
#include "mp3_decoder.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cstdio>

namespace audio_decoder {

Format detectFormat(const uint8_t* data, size_t size) {
    if (size < 12) {
        return Format::UNKNOWN;
    }
    
    // 检测 WAV
    if (size >= 12 && 
        memcmp(data, "RIFF", 4) == 0 &&
        memcmp(data + 8, "WAVE", 4) == 0) {
        return Format::WAV;
    }
    
    // 检测 MP3 (ID3v2 或 MP3 帧同步)
    if (size >= 3) {
        // ID3v2 标签
        if (memcmp(data, "ID3", 3) == 0) {
            return Format::MP3;
        }
        // MP3 帧同步 (0xFF 0xFB/0xFA/0xF2/0xF3)
        if (data[0] == 0xFF && (data[1] & 0xE0) == 0xE0) {
            return Format::MP3;
        }
    }
    
    return Format::UNKNOWN;
}

Format detectFormat(const std::string& filepath) {
    // 根据文件扩展名检测
    size_t dot_pos = filepath.find_last_of('.');
    if (dot_pos == std::string::npos) {
        // 尝试读取文件头
        FILE* f = fopen(filepath.c_str(), "rb");
        if (f) {
            uint8_t header[12];
            size_t read = fread(header, 1, sizeof(header), f);
            fclose(f);
            if (read >= 12) {
                return detectFormat(header, read);
            }
        }
        return Format::UNKNOWN;
    }
    
    std::string ext = filepath.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "wav") {
        return Format::WAV;
    }
    if (ext == "mp3") {
        return Format::MP3;
    }
    
    return Format::UNKNOWN;
}

AudioDecoder* createDecoder(Format format) {
    switch (format) {
        case Format::WAV:
            return new WavDecoder();
        case Format::MP3:
            return new Mp3Decoder();
        default:
            return nullptr;
    }
}

} // namespace audio_decoder

