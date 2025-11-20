/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "app_music_player.h"
#include <apps/utils/common.h>
#include <apps/utils/theme.h>
#include <mooncake_log.h>
#include <cstdio>
#include <dirent.h>
#include <algorithm>
#include <cstring>

using namespace mooncake;

AppMusicPlayer::AppMusicPlayer()
{
    setAppInfo().name = "Music";
    // TODO: 添加图标资源
    setAppInfo().userData = nullptr;
}

AppMusicPlayer::~AppMusicPlayer()
{
    if (_decoder) {
        delete _decoder;
        _decoder = nullptr;
    }
    if (_pcm_buffer) {
        delete[] _pcm_buffer;
        _pcm_buffer = nullptr;
    }
}

void AppMusicPlayer::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");
    
    // 确保 SD 卡已初始化
    GetHAL().sdCardProbe();
    
    // 初始化音频
    GetHAL().speaker.begin();
    GetHAL().speaker.setVolume(255);
    
    // 分配 PCM 缓冲区
    _pcm_buffer = new int16_t[PCM_BUFFER_SIZE * 2]; // 立体声最大
    
    // 扫描音频文件
    scanAudioFiles();
    
    // 如果有文件，开始播放第一个
    if (!_audio_files.empty()) {
        playCurrentFile();
    }
    
    _last_update_time = GetHAL().millis();
}

void AppMusicPlayer::onRunning()
{
    handleKeyboard();
    
    // 更新播放状态
    if (_is_playing && !_is_paused && _decoder) {
        // 检查是否需要填充新的音频数据
        if (!GetHAL().speaker.isPlaying()) {
            // 解码更多数据
            if (_decoder->hasMore()) {
                audio_decoder::AudioInfo info;
                _decoder->getInfo(info);
                
                size_t samples = _decoder->decode(_pcm_buffer, PCM_BUFFER_SIZE);
                if (samples > 0) {
                    // 转换为单声道（节省内存）
                    if (info.channels == 2) {
                        std::vector<int16_t> mono_buffer(samples);
                        for (size_t i = 0; i < samples; ++i) {
                            mono_buffer[i] = (_pcm_buffer[i * 2] + _pcm_buffer[i * 2 + 1]) / 2;
                        }
                        GetHAL().speaker.playRaw(mono_buffer.data(), samples, info.sample_rate, false);
                    } else {
                        GetHAL().speaker.playRaw(_pcm_buffer, samples, info.sample_rate, false);
                    }
                } else {
                    // 文件播放完成，播放下一首
                    nextFile();
                }
            } else {
                // 文件播放完成
                nextFile();
            }
        }
    }
    
    // 定期更新 UI
    if (GetHAL().millis() - _last_update_time > 100) {
        renderUI();
        _last_update_time = GetHAL().millis();
    }
    
    // 关闭应用
    if (GetHAL().homeButton.wasClicked()) {
        stopPlayback();
        close();
    }
}

void AppMusicPlayer::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");
    
    stopPlayback();
    
    if (_pcm_buffer) {
        delete[] _pcm_buffer;
        _pcm_buffer = nullptr;
    }
}

void AppMusicPlayer::scanAudioFiles()
{
    _audio_files.clear();
    
    // 扫描 /sdcard 目录
    DIR* dir = opendir("/sdcard");
    if (!dir) {
        mclog::tagError(getAppInfo().name, "Failed to open /sdcard directory");
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr && _audio_files.size() < MAX_FILES) {
        std::string filename = entry->d_name;
        
        // 检查文件扩展名
        size_t dot_pos = filename.find_last_of('.');
        if (dot_pos != std::string::npos) {
            std::string ext = filename.substr(dot_pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == "mp3" || ext == "wav") {
                std::string full_path = "/sdcard/" + filename;
                _audio_files.push_back(full_path);
                mclog::tagInfo(getAppInfo().name, "Found audio file: {}", full_path);
            }
        }
    }
    
    closedir(dir);
    
    mclog::tagInfo(getAppInfo().name, "Found {} audio files", _audio_files.size());
}

void AppMusicPlayer::playCurrentFile()
{
    if (_audio_files.empty() || _current_file_index >= _audio_files.size()) {
        return;
    }
    
    stopPlayback();
    
    std::string filepath = _audio_files[_current_file_index];
    mclog::tagInfo(getAppInfo().name, "Playing: {}", filepath);
    
    // 检测格式
    auto format = audio_decoder::detectFormat(filepath);
    if (format == audio_decoder::Format::UNKNOWN) {
        mclog::tagError(getAppInfo().name, "Unknown audio format: {}", filepath);
        return;
    }
    
    // 创建解码器
    _decoder = audio_decoder::createDecoder(format);
    if (!_decoder) {
        mclog::tagError(getAppInfo().name, "Failed to create decoder");
        return;
    }
    
    // 打开文件
    if (!_decoder->open(filepath)) {
        mclog::tagError(getAppInfo().name, "Failed to open file: {}", filepath);
        delete _decoder;
        _decoder = nullptr;
        return;
    }
    
    // 获取音频信息
    audio_decoder::AudioInfo info;
    if (!_decoder->getInfo(info)) {
        mclog::tagError(getAppInfo().name, "Failed to get audio info");
        delete _decoder;
        _decoder = nullptr;
        return;
    }
    
    mclog::tagInfo(getAppInfo().name, "Audio info: {}Hz, {}ch, {}bit",
                   info.sample_rate, info.channels, info.bits_per_sample);
    
    _is_playing = true;
    _is_paused = false;
}

void AppMusicPlayer::stopPlayback()
{
    if (_decoder) {
        _decoder->close();
        delete _decoder;
        _decoder = nullptr;
    }
    
    GetHAL().speaker.stop();
    _is_playing = false;
    _is_paused = false;
}

void AppMusicPlayer::pausePlayback()
{
    if (_is_playing && !_is_paused) {
        GetHAL().speaker.stop();
        _is_paused = true;
    }
}

void AppMusicPlayer::resumePlayback()
{
    if (_is_playing && _is_paused) {
        _is_paused = false;
    }
}

void AppMusicPlayer::nextFile()
{
    if (_audio_files.empty()) {
        return;
    }
    
    _current_file_index++;
    if (_current_file_index >= _audio_files.size()) {
        _current_file_index = 0;
    }
    
    playCurrentFile();
}

void AppMusicPlayer::previousFile()
{
    if (_audio_files.empty()) {
        return;
    }
    
    if (_current_file_index == 0) {
        _current_file_index = _audio_files.size() - 1;
    } else {
        _current_file_index--;
    }
    
    playCurrentFile();
}

void AppMusicPlayer::renderUI()
{
    GetHAL().canvas.fillScreen(THEME_COLOR_BG);
    GetHAL().canvas.setFont(FONT_REPL);
    GetHAL().canvas.setTextSize(1);
    
    // 标题
    GetHAL().canvas.setTextColor(TFT_ORANGE);
    GetHAL().canvas.setCursor(5, 5);
    GetHAL().canvas.println("Music Player");
    
    // 当前文件信息
    if (!_audio_files.empty() && _current_file_index < _audio_files.size()) {
        std::string filename = _audio_files[_current_file_index];
        size_t slash_pos = filename.find_last_of('/');
        if (slash_pos != std::string::npos) {
            filename = filename.substr(slash_pos + 1);
        }
        
        GetHAL().canvas.setTextColor(TFT_CYAN);
        GetHAL().canvas.setCursor(5, 25);
        GetHAL().canvas.printf("File: %s", filename.c_str());
        
        // 播放状态
        GetHAL().canvas.setCursor(5, 40);
        if (_is_paused) {
            GetHAL().canvas.setTextColor(TFT_YELLOW);
            GetHAL().canvas.println("Status: PAUSED");
        } else if (_is_playing) {
            GetHAL().canvas.setTextColor(TFT_GREEN);
            GetHAL().canvas.println("Status: PLAYING");
        } else {
            GetHAL().canvas.setTextColor(TFT_RED);
            GetHAL().canvas.println("Status: STOPPED");
        }
        
        // 文件索引
        GetHAL().canvas.setTextColor(TFT_WHITE);
        GetHAL().canvas.setCursor(5, 55);
        GetHAL().canvas.printf("%zu / %zu", _current_file_index + 1, _audio_files.size());
    } else {
        GetHAL().canvas.setTextColor(TFT_RED);
        GetHAL().canvas.setCursor(5, 25);
        GetHAL().canvas.println("No audio files found!");
    }
    
    // 控制提示
    GetHAL().canvas.setTextColor(TFT_DARKGREY);
    GetHAL().canvas.setCursor(5, 80);
    GetHAL().canvas.println("Controls:");
    GetHAL().canvas.setCursor(5, 95);
    GetHAL().canvas.println("N/P: Next/Prev");
    GetHAL().canvas.setCursor(5, 110);
    GetHAL().canvas.println("Space: Play/Pause");
    
    GetHAL().pushCanvas();
}

void AppMusicPlayer::handleKeyboard()
{
    auto key_event = GetHAL().keyboard.getLatestKeyEvent();
    if (key_event.state == true) {
        if (key_event.keyCode == 'n' || key_event.keyCode == 'N') {
            nextFile();
        } else if (key_event.keyCode == 'p' || key_event.keyCode == 'P') {
            previousFile();
        } else if (key_event.keyCode == ' ') {
            if (_is_paused) {
                resumePlayback();
            } else if (_is_playing) {
                pausePlayback();
            } else {
                playCurrentFile();
            }
        }
    }
}

