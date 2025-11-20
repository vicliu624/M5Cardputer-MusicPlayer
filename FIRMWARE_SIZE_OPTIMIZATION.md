# 固件大小优化指南

## 当前问题

编译出来的固件有 **2.5M**，而历史工程只有 **1.6M**，大了约 **900KB**。

## 主要原因分析

### 1. ⚠️ **编译优化设置为 DEBUG 模式**（最大问题）

**当前配置**：
```
CONFIG_COMPILER_OPTIMIZATION_DEBUG=y
CONFIG_COMPILER_OPTIMIZATION_LEVEL_DEBUG=y
```

**问题**：
- DEBUG 模式**没有优化**，代码体积会大很多
- 包含调试符号和未优化的代码
- 预计会增加 **300-500KB**

**解决方案**：改为 SIZE 优化模式

### 2. ⚠️ **错误查找表启用**

**当前配置**：
```
CONFIG_ESP_ERR_TO_NAME_LOOKUP=y
```

**问题**：
- 包含所有错误码的字符串查找表
- 预计增加 **50-100KB**

**解决方案**：禁用（如果不需要错误码转字符串）

### 3. ⚠️ **日志级别较高**

**当前配置**：
```
CONFIG_LOG_DEFAULT_LEVEL_INFO=y
CONFIG_LOG_DEFAULT_LEVEL=3
```

**问题**：
- INFO 级别会包含很多日志字符串
- 预计增加 **50-100KB**

**解决方案**：降低到 WARN 或 ERROR 级别

### 4. ⚠️ **包含大量组件**

**新工程包含的组件**：
- Mooncake 框架
- M5GFX（图形库，包含大量字体）
- M5Unified
- smooth_ui_toolkit
- PikaPython（Python 解释器，很大！）
- esp-now
- radiolib
- 多个应用（launcher、chat、repl 等）

**历史工程**：
- 只有基本的 Arduino 库
- ESP32-audioI2S
- M5Cardputer

**预计差异**：**200-400KB**

## 优化方案

### 方案 1：快速优化（推荐）

修改 `sdkconfig` 文件：

```bash
# 1. 改为 SIZE 优化
CONFIG_COMPILER_OPTIMIZATION_SIZE=y
# CONFIG_COMPILER_OPTIMIZATION_DEBUG is not set
CONFIG_COMPILER_OPTIMIZATION_LEVEL_RELEASE=y

# 2. 禁用错误查找表
# CONFIG_ESP_ERR_TO_NAME_LOOKUP is not set

# 3. 降低日志级别
CONFIG_LOG_DEFAULT_LEVEL_WARN=y
CONFIG_LOG_DEFAULT_LEVEL=2
```

**预期效果**：减少 **400-600KB**

### 方案 2：使用 menuconfig 配置

```bash
idf.py menuconfig
```

然后修改：
1. **Compiler options** → **Optimization Level** → 选择 **Optimize for size (-Os)**
2. **Component config** → **Log output** → **Default log verbosity** → 选择 **Warning**
3. **Component config** → **Common ESP-related** → **Enable lookup of error code strings** → 取消勾选

### 方案 3：移除不必要的组件（如果不需要）

如果不需要某些功能，可以移除：

1. **PikaPython**（如果不需要 Python 解释器）
   - 预计节省：**200-300KB**

2. **smooth_ui_toolkit**（如果不需要高级 UI）
   - 预计节省：**50-100KB**

3. **不需要的应用**（如 chat、repl 等）
   - 预计节省：**50-100KB**

## 预期优化结果

| 优化项 | 预计节省 | 累计 |
|--------|---------|------|
| DEBUG → SIZE 优化 | 300-500KB | 300-500KB |
| 禁用错误查找表 | 50-100KB | 350-600KB |
| 降低日志级别 | 50-100KB | 400-700KB |
| 移除 PikaPython | 200-300KB | 600-1000KB |
| **总计** | | **600KB - 1MB** |

**优化后预期大小**：**1.5M - 1.9M**（接近历史工程的 1.6M）

## 快速修复命令

使用 `idf.py menuconfig` 或直接编辑 `sdkconfig`：

```bash
# 使用 sed 快速修改（Linux/Mac）
sed -i 's/CONFIG_COMPILER_OPTIMIZATION_DEBUG=y/# CONFIG_COMPILER_OPTIMIZATION_DEBUG is not set/' sdkconfig
sed -i 's/# CONFIG_COMPILER_OPTIMIZATION_SIZE is not set/CONFIG_COMPILER_OPTIMIZATION_SIZE=y/' sdkconfig
sed -i 's/CONFIG_COMPILER_OPTIMIZATION_LEVEL_DEBUG=y/CONFIG_COMPILER_OPTIMIZATION_LEVEL_RELEASE=y/' sdkconfig
sed -i 's/CONFIG_ESP_ERR_TO_NAME_LOOKUP=y/# CONFIG_ESP_ERR_TO_NAME_LOOKUP is not set/' sdkconfig
sed -i 's/CONFIG_LOG_DEFAULT_LEVEL_INFO=y/CONFIG_LOG_DEFAULT_LEVEL_WARN=y/' sdkconfig
sed -i 's/CONFIG_LOG_DEFAULT_LEVEL=3/CONFIG_LOG_DEFAULT_LEVEL=2/' sdkconfig
```

## 验证

优化后重新编译：

```bash
idf.py build
idf.py size
```

查看固件大小是否减少。

