# minimp3 在 ESP32 上的使用指南

## 兼容性确认

✅ **minimp3 完全支持 ESP32 系列**，包括：
- ESP32 (单核/双核)
- ESP32-S2
- ESP32-S3 (你的设备)
- ESP32-C3
- ESP32-C6

## 性能数据

### ESP32-C3 测试结果（参考）
- **解码速度**：128kbps MP3，平均每帧 9.3ms
- **代码体积**：< 16KB Flash
- **内存占用**：约 1-2KB RAM（解码缓冲区）

### ESP32-S3 预期性能
ESP32-S3 性能更强（双核 240MHz），预期表现：
- ✅ 解码速度更快（可能 < 5ms/帧）
- ✅ 支持更高采样率（44.1kHz, 48kHz）
- ✅ 支持更高比特率（192kbps, 320kbps）

## 注意事项

### ⚠️ 性能限制
根据一些开发者反馈：
- 在 **32kHz 以上采样率**时，某些 ESP32 型号可能性能不足
- **ESP32-S3 应该没问题**（性能更强）

### ✅ 优势
- 轻量级：代码体积小（< 16KB）
- 内存占用低（1-2KB RAM）
- 单文件头库，易于集成
- 无外部依赖

## 集成步骤

### 1. 下载 minimp3

从 GitHub 下载：
```bash
# 下载 minimp3.h 和 minimp3_ex.h
# https://github.com/lieff/minimp3
```

### 2. 项目结构

```
main/
  apps/
    utils/
      audio_decoder/
        minimp3.h          # minimp3 头文件
        minimp3_ex.h       # minimp3 扩展头文件（可选）
        mp3_decoder.h      # MP3 解码器封装
        mp3_decoder.cpp    # MP3 解码器实现
        CMakeLists.txt     # 组件配置
```

### 3. CMakeLists.txt

```cmake
idf_component_register(
    SRCS "mp3_decoder.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver
)
```

### 4. 基本使用示例

```cpp
#include "minimp3.h"
#include "minimp3_ex.h"

// 初始化解码器
mp3dec_t mp3d;
mp3dec_init(&mp3d);

// 解码一帧
mp3dec_frame_info_t frame_info;
int16_t pcm_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME * 2]; // 立体声

int samples = mp3dec_decode_frame(&mp3d, 
                                  mp3_data, 
                                  mp3_data_size,
                                  pcm_buffer, 
                                  &frame_info);

if (samples > 0) {
    // samples: 解码的采样点数
    // frame_info.hz: 采样率
    // frame_info.channels: 声道数 (1 或 2)
    // frame_info.bitrate_kbps: 比特率
    // pcm_buffer: PCM 数据（16-bit signed）
}
```

## 针对 ESP32-S3 的优化建议

### 1. 编译优化

在 `CMakeLists.txt` 中添加：
```cmake
target_compile_options(${COMPONENT_LIB} PRIVATE 
    -O2              # 优化级别
    -ffast-math      # 快速数学运算
)
```

### 2. 内存优化

```cpp
// 使用较小的缓冲区（单声道）
constexpr size_t BUFFER_SIZE = 512;  // 512 个采样点
int16_t pcm_buffer[BUFFER_SIZE];

// 如果内存紧张，可以限制为单声道
// 立体声转单声道：混合左右声道
```

### 3. 流式解码

```cpp
// 适合 WiFi 流式播放
size_t decode_stream(const uint8_t* mp3_data, 
                     size_t mp3_size,
                     int16_t* pcm_buffer,
                     size_t max_samples) {
    size_t total_decoded = 0;
    size_t offset = 0;
    
    while (offset < mp3_size && total_decoded < max_samples) {
        mp3dec_frame_info_t frame_info;
        int samples = mp3dec_decode_frame(&mp3d,
                                          mp3_data + offset,
                                          mp3_size - offset,
                                          pcm_buffer + total_decoded,
                                          &frame_info);
        
        if (samples <= 0) break;
        
        offset += frame_info.frame_bytes;
        total_decoded += samples * frame_info.channels;
    }
    
    return total_decoded;
}
```

## 与其他库的对比

| 库 | Flash | RAM | 性能 | 支持格式 |
|----|-------|-----|------|----------|
| **minimp3** | < 16KB | 1-2KB | ⭐⭐⭐⭐ | MP3 |
| libhelix | ~20KB | 2-3KB | ⭐⭐⭐ | MP3 |
| libmad | ~50KB | 4-6KB | ⭐⭐⭐⭐⭐ | MP3 |
| ESP8266Audio | ~30KB | 3-5KB | ⭐⭐⭐ | MP3, AAC, WAV |

**推荐**：minimp3（轻量级，性能足够）

## 实际测试建议

### 测试用例

1. **低比特率测试**
   - 64kbps, 128kbps MP3
   - 采样率：22.05kHz, 44.1kHz

2. **高比特率测试**
   - 192kbps, 320kbps MP3
   - 采样率：44.1kHz, 48kHz

3. **流式播放测试**
   - WiFi 下载 + 实时解码
   - 检查是否有卡顿

### 性能监控

```cpp
#include <esp_timer.h>

int64_t start_time = esp_timer_get_time();
int samples = mp3dec_decode_frame(...);
int64_t decode_time = esp_timer_get_time() - start_time;

mclog::tagInfo("MP3", "Decoded {} samples in {} us", 
               samples, decode_time);
```

## 常见问题

### Q1: 解码速度不够快？
**A**: 
- 确保启用 `-O2` 优化
- 降低采样率（22.05kHz 或 32kHz）
- 降低比特率（128kbps 或更低）
- ESP32-S3 应该足够快，如果还是慢，检查是否有其他任务占用 CPU

### Q2: 内存不足？
**A**:
- 使用单声道（节省 50% 内存）
- 减小解码缓冲区大小
- 使用流式解码，不要一次性加载整个文件

### Q3: 音质不好？
**A**:
- 使用更高的比特率（192kbps 或 320kbps）
- 确保采样率匹配（44.1kHz 或 48kHz）
- 检查音量设置和 DAC 配置

## 总结

✅ **minimp3 非常适合 ESP32-S3**：
- 轻量级（< 16KB Flash）
- 性能足够（ESP32-S3 双核 240MHz）
- 易于集成（单文件头库）
- 内存占用低（1-2KB RAM）

**推荐使用 minimp3 作为 MP3 解码库！**

