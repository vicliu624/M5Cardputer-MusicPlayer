# Mooncake 框架内存优化分析报告

## 当前内存使用情况

### 1. 核心数据结构内存占用

#### AbilityManager 内存占用
- `_ability_list`: `std::vector<AbilityInfo_t>` 
  - 每个 AbilityInfo_t 约 24-32 字节（取决于指针大小）
  - 当前项目安装 13 个 App ≈ 312-416 字节
  - vector 本身开销：约 24 字节（3个指针）
  
- `_new_ability_list`: `std::vector<AbilityInfo_t>`
  - 临时列表，通常为空或很小
  - 约 24 字节基础开销

- `_available_ability_id_list`: `std::vector<int>`
  - 存储回收的 ID，通常很小
  - 约 24 字节基础开销

**总计：约 400-500 字节**

#### AppInfo_t 内存占用
每个 App 的 `AppInfo_t` 结构：
```cpp
struct AppInfo_t {
    std::string name;      // 动态分配，通常 16-32 字节（小字符串优化）
    void* icon = nullptr;  // 8 字节指针
    void* userData = nullptr; // 8 字节指针
};
```
- 13 个 App ≈ 416-832 字节（取决于名称长度）

#### Mooncake 实例内存占用
- `_app_ability_manager`: `std::unique_ptr<AbilityManager>` ≈ 8 字节
- `_extension_ability_manager`: `std::unique_ptr<AbilityManager>` ≈ 8 字节
- 单例指针：8 字节

**Mooncake 框架总内存占用：约 1-2 KB**

## 优化建议

### 优化 1: 限制 App 数量（高优先级）
**当前问题**：项目安装了 13 个 App，但可能不需要同时运行所有 App

**优化方案**：
- 实现 App 懒加载：只在需要时安装 App
- 实现 App 卸载机制：不常用的 App 可以卸载释放内存
- 使用 App 池：只保留最常用的 5-8 个 App

**预期节省**：每个未安装的 App 可节省约 50-100 字节

### 优化 2: 优化 std::string 使用（中优先级）
**当前问题**：`AppInfo_t::name` 使用 `std::string`，即使小字符串也有 16-32 字节开销

**优化方案**：
```cpp
// 方案 A: 使用固定大小字符数组（如果名称长度有限）
struct AppInfo_t {
    char name[16];  // 固定 16 字节，节省动态分配开销
    void* icon = nullptr;
    void* userData = nullptr;
};

// 方案 B: 使用 const char* 指向静态字符串
struct AppInfo_t {
    const char* name;  // 指向编译时常量，不占用堆内存
    void* icon = nullptr;
    void* userData = nullptr;
};
```

**预期节省**：每个 App 节省 8-16 字节，13 个 App 约 100-200 字节

### 优化 3: 优化 vector 容量（低优先级）
**当前问题**：`std::vector` 可能预分配了比实际需要更多的内存

**优化方案**：
```cpp
// 在 destroyAbility 后收缩 vector
void AbilityManager::destroyAbility(int abilityID) {
    // ... 现有代码 ...
    
    // 如果列表变得很小，收缩容量
    if (_ability_list.size() < _ability_list.capacity() / 2) {
        _ability_list.shrink_to_fit();
    }
}
```

**预期节省**：通常很少，但在频繁创建/销毁时可能有帮助

### 优化 4: 移除 Extension Manager（如果未使用）
**当前问题**：Extension Manager 即使不使用也会占用内存

**优化方案**：
- 检查项目是否使用了 Extension Manager
- 如果未使用，可以移除或条件编译

**预期节省**：约 400-500 字节

### 优化 5: 使用对象池模式（高级优化）
**当前问题**：频繁创建/销毁 App 会导致内存碎片

**优化方案**：
- 实现 App 对象池
- 预分配固定数量的 App 槽位
- 使用位图管理空闲槽位

**预期节省**：减少内存碎片，提高内存利用率

## 具体实施建议

### 优先级排序

1. **立即实施（高优先级）**：
   - ✅ 检查并移除未使用的 App
   - ✅ 将 AppInfo_t::name 改为 `const char*` 或固定数组
   - ✅ 检查 Extension Manager 使用情况

2. **中期优化（中优先级）**：
   - 实现 App 懒加载机制
   - 添加 vector 容量管理

3. **长期优化（低优先级）**：
   - 实现对象池模式
   - 考虑使用更轻量的容器（如果 C++ 标准库开销太大）

## 预期优化效果

| 优化项 | 节省内存 | 实施难度 | 优先级 |
|--------|---------|---------|--------|
| 移除未使用 App | 50-100 字节/App | 低 | 高 |
| 优化 AppInfo_t::name | 100-200 字节 | 中 | 高 |
| 移除 Extension Manager | 400-500 字节 | 低 | 中 |
| 懒加载 App | 50-100 字节/App | 中 | 中 |
| vector 容量管理 | 10-50 字节 | 低 | 低 |

**总计预期节省：约 1-2 KB**

## 注意事项

1. **不要过度优化**：Mooncake 框架本身已经很轻量（约 1-2 KB），优化空间有限
2. **关注 App 本身的内存使用**：App 实例的内存占用可能远大于框架本身
3. **考虑 WiFi 音乐播放器的实际需求**：可能需要优化的是 App 实现，而不是框架

## 结论

Mooncake 框架本身的内存占用很小（约 1-2 KB），主要的内存占用来自：
1. 安装的 App 数量（13 个）
2. 每个 App 的实例数据
3. AppInfo_t 中的 std::string

**建议优先优化 App 层面的内存使用，而不是框架本身。**

