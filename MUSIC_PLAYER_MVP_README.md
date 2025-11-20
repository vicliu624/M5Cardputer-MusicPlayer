# 音乐播放器 MVP 使用说明

## 功能

✅ **已实现**：
- 从 SD 卡扫描 MP3 和 WAV 文件
- 播放 WAV 文件（完整支持）
- 播放 MP3 文件（占位符，需要添加 minimp3）
- 播放控制（播放/暂停/上一首/下一首）
- 简单的 UI 显示

## 使用方法

1. **准备 SD 卡**：
   - 将 MP3 或 WAV 文件放入 SD 卡根目录
   - 插入 SD 卡到设备

2. **启动应用**：
   - 在 Launcher 中找到 "Music" 应用
   - 点击打开

3. **控制**：
   - `N` 键：下一首
   - `P` 键：上一首
   - `空格` 键：播放/暂停
   - `Home` 键：退出应用

## 当前状态

### WAV 播放
- ✅ 完全支持
- ✅ 自动检测格式
- ✅ 支持单声道和立体声
- ✅ 自动转换为单声道播放（节省内存）

### MP3 播放
- ⚠️ 占位符实现
- ⚠️ 需要添加 minimp3 库才能实际播放
- ✅ 文件扫描和格式检测已支持

## 下一步

要完整支持 MP3 播放，需要：

1. **下载 minimp3**：
   ```bash
   # 下载 minimp3.h 到 main/apps/utils/audio_decoder/
   ```

2. **实现 MP3 解码**：
   - 修改 `mp3_decoder.cpp`
   - 使用 minimp3 API 进行解码

## 文件结构

```
main/apps/
├── app_music_player/
│   ├── app_music_player.h      # App 头文件
│   └── app_music_player.cpp    # App 实现
└── utils/
    └── audio_decoder/
        ├── audio_decoder.h      # 解码器接口
        ├── wav_decoder.h        # WAV 解码器
        ├── wav_decoder.cpp      # WAV 解码器实现
        ├── mp3_decoder.h        # MP3 解码器（占位符）
        ├── mp3_decoder.cpp      # MP3 解码器实现（占位符）
        └── format_detector.cpp  # 格式检测
```

## 资源消耗

- **Flash**: ~5KB（音频解码器代码）
- **RAM**: ~4KB（PCM 缓冲区 2KB + 解码器状态）
- **CPU**: 低（I2S DMA 硬件加速）

## 已知问题

1. MP3 解码未实现（需要 minimp3）
2. 没有图标资源（显示为默认图标）
3. UI 较简单（可以后续优化）

## 测试建议

1. 准备一个 16-bit PCM WAV 文件（44.1kHz 或 22.05kHz）
2. 放入 SD 卡根目录
3. 启动应用测试播放
4. 测试播放控制功能

