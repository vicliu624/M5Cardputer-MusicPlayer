# M5GFX 字体优化指南

## 当前问题

M5GFX 编译了**所有字体**，包括：
- 48 个 GFXFF 字体（FreeMono, FreeSans, FreeSerif 系列）
- 7 个 Custom 字体（DejaVu, Orbitron, Roboto, Satisfy, Yellowtail）
- 大量 efont（中文、日文、韩文、繁体中文，每个都有多个尺寸和样式）
- 所有 IPA 日文字体
- 基础字体（Font0, Font2, Font4, Font6, Font7, Font8 等）

**预计占用**：**200-400KB Flash**

## 需要支持的语言

- **简体中文 (CN)** - 使用 `efontCN`
- **繁体中文 (TW)** - 使用 `efontTW`
- **日文 (JA)** - 使用 `efontJA`（不使用 IPA）
- **韩文 (KR)** - 使用 `efontKR`
- **英文 (EN)** - 使用 `Font0`

## 实际使用的字体

根据代码分析，项目中使用了：
1. **`fonts::Font0`** - 英文基础字体，在 `app_lora_chat` 和 `system_bar` 中使用
2. **`fonts::efontCN_16`** - 简体中文，在 `theme.h` 中定义为 `FONT_BASIC` 和 `FONT_REPL`，在 `boot_anim` 中使用

## 优化方案

### 已实施的优化

在 `components/M5GFX/src/lgfx/v1/lgfx_fonts.cpp` 中注释掉未使用的字体。

**保留的字体**：
- `Font0` (glcdfont.h) - 英文基础字体
- `efontCN` - 简体中文（所有尺寸：10, 12, 14, 16, 24）
- `efontTW` - 繁体中文（所有尺寸：10, 12, 14, 16, 24）
- `efontJA` - 日文（所有尺寸：10, 12, 14, 16, 24）
- `efontKR` - 韩文（所有尺寸：10, 12, 14, 16, 24）

**已移除的字体**：
- 所有 GFXFF 字体（48个）- 预计节省 **100-150KB**
- 所有 Custom 字体（7个）- 预计节省 **20-30KB**
- 所有 IPA 日文字体（27个）- 预计节省 **30-50KB**（使用 efontJA 代替）
- 未使用的 RLE 字体（Font2, Font4, Font6, Font7, Font8）- 预计节省 **20-30KB**

**预计总节省**：**170-260KB**

### 方案 2：修改 CMakeLists.txt

排除未使用的字体源文件：
- `src/lgfx/Fonts/IPA/*.c` - 日文字体
- 部分 efont 文件（如果只需要中文）

## 实施步骤

由于 `lgfx_fonts.cpp` 是 M5GFX 库的一部分，直接修改可能会在更新库时丢失。

**建议**：
1. 创建补丁文件或 fork M5GFX
2. 或者使用条件编译宏来控制字体包含

## 快速优化（最小改动）

只保留必要的字体，注释掉其他所有字体的包含。

