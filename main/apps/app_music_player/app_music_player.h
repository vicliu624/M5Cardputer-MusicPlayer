/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <mooncake.h>
#include <cstdint>
#include <string>
#include <vector>
#include <hal/hal.h>
#include <apps/utils/audio_decoder/audio_decoder.h>

class AppMusicPlayer : public mooncake::AppAbility {
public:
    AppMusicPlayer();
    ~AppMusicPlayer();

    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    static constexpr size_t MAX_FILES = 100;
    static constexpr size_t PCM_BUFFER_SIZE = 1024; // 1024 个采样点
    
    std::vector<std::string> _audio_files;
    size_t _current_file_index = 0;
    audio_decoder::AudioDecoder* _decoder = nullptr;
    int16_t* _pcm_buffer = nullptr;
    bool _is_playing = false;
    bool _is_paused = false;
    uint32_t _last_update_time = 0;
    
    void scanAudioFiles();
    void playCurrentFile();
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();
    void nextFile();
    void previousFile();
    void renderUI();
    void handleKeyboard();
};

