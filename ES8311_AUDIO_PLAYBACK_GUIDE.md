# ES8311 音频编解码器使用指南

## 概述

ES8311 是项目中的音频编解码器，通过 **M5Unified** 库封装，提供了 `Speaker_Class` 接口来播放音频。

## 架构说明

```
ES8311 编解码器
    ↓ (I2S 接口)
M5Unified 库 (M5.Speaker)
    ↓ (封装)
Hal::speaker (GetHAL().speaker)
    ↓ (应用层)
你的代码
```

## 基本使用

### 1. 初始化

```cpp
#include <hal/hal.h>

// 在系统初始化时（通常在 Hal::init() 中）
GetHAL().speaker.begin();  // 初始化 ES8311 编解码器
GetHAL().speaker.setVolume(255);  // 设置音量 (0-255)
```

**注意**：ES8311 初始化需要一些时间，建议在系统启动时尽早初始化。

### 2. 播放原始 PCM 音频数据

#### 方法签名
```cpp
bool playRaw(const int16_t* raw_data, size_t array_len, 
             uint32_t sample_rate = 44100, 
             bool stereo = false, 
             uint32_t repeat = 1, 
             int channel = -1, 
             bool stop_current_sound = false);
```

#### 参数说明
- `raw_data`: PCM 音频数据指针（int16_t 数组）
- `array_len`: 数据长度（采样点数，不是字节数）
- `sample_rate`: 采样率（Hz），默认 44100
- `stereo`: 是否立体声，默认 false（单声道）
- `repeat`: 重复播放次数，默认 1
- `channel`: 虚拟声道编号（0-7），-1 表示自动选择
- `stop_current_sound`: 是否停止当前播放，默认 false

#### 使用示例

```cpp
// 示例 1: 播放单声道 PCM 数据
int16_t audio_data[1000];  // 1000 个采样点
// ... 填充 audio_data ...
GetHAL().speaker.playRaw(audio_data, 1000, 44100, false);

// 示例 2: 播放立体声 PCM 数据
int16_t stereo_data[2000];  // 1000 个采样点 × 2 声道
// ... 填充 stereo_data (L, R, L, R, ...) ...
GetHAL().speaker.playRaw(stereo_data, 2000, 44100, true);

// 示例 3: 播放并等待完成
GetHAL().speaker.playRaw(audio_data, 1000, 44100, false);
while (GetHAL().speaker.isPlaying()) {
    GetHAL().delay(1);
}
```

### 3. 播放 WAV 文件

```cpp
bool playWav(const uint8_t* wav_data, size_t data_len = ~0u, 
             uint32_t repeat = 1, 
             int channel = -1, 
             bool stop_current_sound = false);
```

**注意**：需要完整的 WAV 文件数据（包含 WAV 头）。

### 4. 控制播放

```cpp
// 检查是否正在播放
bool isPlaying = GetHAL().speaker.isPlaying();

// 停止播放
GetHAL().speaker.stop();

// 停止指定声道
GetHAL().speaker.stop(0);  // 停止声道 0

// 设置音量 (0-255)
GetHAL().speaker.setVolume(128);  // 50% 音量

// 获取音量
uint8_t volume = GetHAL().speaker.getVolume();

// 获取配置信息
auto cfg = GetHAL().speaker.config();
uint32_t sample_rate = cfg.sample_rate;  // 当前采样率
```

### 5. 关闭音频输出

```cpp
GetHAL().speaker.end();  // 关闭 ES8311，释放资源
```

**注意**：麦克风和扬声器不能同时使用，录音时需要先关闭扬声器。

## 实际项目中的使用示例

### 示例 1: 播放录音数据（来自 app_record）

```cpp
void AppRecord::start_playback()
{
    // 停止录音，启动播放
    GetHAL().mic.end();
    GetHAL().speaker.begin();
    GetHAL().speaker.setVolume(255);

    // 播放录制的数据
    int16_t* audio_data = _rec_data;  // 录音数据
    size_t data_size = RECORD_SIZE;   // 数据大小（采样点数）
    uint32_t sample_rate = PLAYBACK_SAMPLERATE;  // 采样率
    
    GetHAL().speaker.playRaw(audio_data, data_size, sample_rate, false);
    
    // 等待播放完成
    while (GetHAL().speaker.isPlaying()) {
        GetHAL().delay(1);
    }
}
```

### 示例 2: 播放音调（来自 audio.cpp）

```cpp
void play_tone(int frequency, double durationSec)
{
    const int sample_rate = GetHAL().speaker.config().sample_rate;
    const int samples = static_cast<int>(sample_rate * durationSec);
    std::vector<int16_t> buffer(samples * 2);  // 双声道

    // 生成正弦波
    for (int i = 0; i < samples; ++i) {
        int16_t value = static_cast<int16_t>(
            amplitude * sin(2.0 * M_PI * frequency * i / sample_rate)
        );
        buffer[i * 2] = value;      // 左声道
        buffer[i * 2 + 1] = value;  // 右声道
    }

    GetHAL().speaker.playRaw(buffer.data(), buffer.size(), sample_rate, true);
}
```

## WiFi 流式播放实现建议

### 方案 1: 双缓冲流式播放

```cpp
class WiFiMusicPlayer {
private:
    static constexpr size_t BUFFER_SIZE = 4096;  // 4KB 缓冲区
    int16_t buffer1[BUFFER_SIZE];
    int16_t buffer2[BUFFER_SIZE];
    bool using_buffer1 = true;
    
public:
    void playStream() {
        // 初始化
        GetHAL().speaker.begin();
        GetHAL().speaker.setVolume(255);
        
        // 启动 HTTP 流下载
        // ... 下载数据到 buffer1 ...
        
        // 开始播放第一个缓冲区
        GetHAL().speaker.playRaw(buffer1, BUFFER_SIZE, 44100, false);
        
        while (is_streaming) {
            // 如果当前播放的是 buffer1，下载到 buffer2
            if (using_buffer1) {
                // ... 下载数据到 buffer2 ...
                // 等待 buffer1 播放完成
                while (GetHAL().speaker.isPlaying()) {
                    GetHAL().delay(1);
                }
                // 播放 buffer2
                GetHAL().speaker.playRaw(buffer2, BUFFER_SIZE, 44100, false);
                using_buffer1 = false;
            } else {
                // ... 下载数据到 buffer1 ...
                while (GetHAL().speaker.isPlaying()) {
                    GetHAL().delay(1);
                }
                GetHAL().speaker.playRaw(buffer1, BUFFER_SIZE, 44100, false);
                using_buffer1 = true;
            }
        }
    }
};
```

### 方案 2: 使用虚拟声道（推荐）

```cpp
void playStream() {
    GetHAL().speaker.begin();
    GetHAL().speaker.setVolume(255);
    
    int16_t buffer[4096];
    int channel = 0;  // 使用固定声道
    
    while (is_streaming) {
        // 下载音频数据
        size_t downloaded = downloadAudioChunk(buffer, sizeof(buffer));
        
        // 播放到指定声道（不阻塞）
        GetHAL().speaker.playRaw(buffer, downloaded / sizeof(int16_t), 
                                 44100, false, 1, channel, false);
        
        // 检查缓冲区是否还有空间
        // 如果播放速度 > 下载速度，需要等待
        while (GetHAL().speaker.isPlaying(channel)) {
            GetHAL().delay(10);
        }
    }
}
```

## 音频格式要求

### 支持的格式
- **PCM 格式**：
  - 采样位深：16-bit signed
  - 采样率：建议 22050, 44100, 48000 Hz
  - 声道：单声道或立体声
  - 字节序：小端序（Little Endian）

- **WAV 格式**：
  - 需要完整的 WAV 文件（包含 WAV 头）
  - 支持标准的 WAV 格式

### 不支持的格式
- MP3（需要解码库，如 libhelix）
- AAC（需要解码库）
- OGG（需要解码库）

## 内存优化建议

### 1. 使用较小的缓冲区
```cpp
// 对于 WiFi 流式播放，使用 2-4KB 缓冲区
constexpr size_t STREAM_BUFFER_SIZE = 2048;  // 2KB
int16_t stream_buffer[STREAM_BUFFER_SIZE];
```

### 2. 使用单声道
```cpp
// 单声道可以节省一半内存
GetHAL().speaker.playRaw(data, size, 44100, false);  // false = 单声道
```

### 3. 降低采样率
```cpp
// 22050 Hz 对于语音和简单音乐足够
GetHAL().speaker.playRaw(data, size, 22050, false);
```

## 常见问题

### Q1: 播放时出现卡顿或断断续续？
**A**: 
- 检查缓冲区是否足够大
- 确保下载速度 > 播放速度
- 检查 WiFi 连接稳定性
- 考虑降低采样率或使用单声道

### Q2: 如何同时播放多个音频？
**A**: 使用不同的虚拟声道（0-7）：
```cpp
GetHAL().speaker.playRaw(data1, size1, 44100, false, 1, 0);  // 声道 0
GetHAL().speaker.playRaw(data2, size2, 44100, false, 1, 1);  // 声道 1
```

### Q3: 麦克风和扬声器可以同时使用吗？
**A**: 不可以。ES8311 在同一时间只能用于输入或输出。使用前需要：
```cpp
// 录音时
GetHAL().speaker.end();
GetHAL().mic.begin();

// 播放时
GetHAL().mic.end();
GetHAL().speaker.begin();
```

### Q4: 如何播放 MP3 文件？
**A**: 需要先解码 MP3 为 PCM。可以使用：
- libhelix（轻量级 MP3 解码库）
- ESP32-AudioI2S（ESP32 音频库）

## 性能参考

### 内存占用
- 每个缓冲区（4KB，单声道，44.1kHz）：约 0.18 秒音频
- 双缓冲：约 8KB 内存
- ES8311 驱动开销：约 1-2KB

### CPU 占用
- 播放时：约 5-10% CPU（取决于采样率）
- I2S DMA 传输：硬件加速，CPU 占用低

### 延迟
- 缓冲区延迟：取决于缓冲区大小
- 4KB 缓冲区（44.1kHz）：约 90ms 延迟

## 总结

ES8311 通过 M5Unified 库提供了简单易用的音频播放接口。对于 WiFi 流式播放：

1. ✅ **支持流式播放**：使用 `playRaw()` 可以边下载边播放
2. ✅ **内存占用小**：可以使用 2-4KB 小缓冲区
3. ✅ **性能良好**：I2S DMA 硬件加速
4. ⚠️ **需要解码**：MP3/AAC 需要先解码为 PCM
5. ⚠️ **单声道优先**：节省内存和带宽

建议使用 **双缓冲 + 单声道 + 22kHz 采样率** 的方案，可以在有限内存下实现流畅播放。

