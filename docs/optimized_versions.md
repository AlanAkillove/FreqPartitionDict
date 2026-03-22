# 优化版本说明

本文档介绍 `FreqPartitionDict` 的多个优化版本，展示不同场景下的性能优化策略。

## 版本概览

| 版本 | 文件 | 特点 | 适用场景 |
|-----|------|-----|---------|
| 基础版 | `freq_partition_dict.hpp` | 代码清晰，易于理解 | 教学、学习 |
| 优化版 | `freq_partition_dict_optimized.hpp` | 最小堆、延迟删除 | 生产环境 |
| 线程安全版 | `freq_partition_dict_threadsafe.hpp` | 读写锁、批量操作 | 多线程环境 |

## 1. HotZoneOptimized - 最小堆优化

### 优化点

**基础版问题**：
- 每次淘汰需要 O(H) 扫描整个热区
- H=128 时，每次淘汰需要扫描 128 个元素

**优化版改进**：
- 使用最小堆（`std::priority_queue`）维护频率
- 淘汰操作降至 O(log H)
- 使用延迟删除策略处理频率更新

### 实现细节

```cpp
// 最小堆：(频率, 键)
std::priority_queue<
    std::pair<size_t, K>,
    std::vector<std::pair<size_t, K>>,
    std::greater<>
> min_heap_;
```

**延迟删除**：
- 频率更新时，将新频率推入堆
- 不立即删除旧频率条目
- 查询最小值时清理堆顶无效元素

### 复杂度对比

| 操作 | 基础版 | 优化版 |
|-----|-------|-------|
| 淘汰 | O(H) | O(log H) |
| 频率更新 | O(1) | O(log H) |
| 内存 | O(H) | O(H) ~ O(2H) |

## 2. ColdZoneOptimized - 自定义分配器

### 优化点

- 支持自定义内存分配器
- 减少内存碎片
- 批量插入优化

### 使用示例

```cpp
// 使用内存池分配器
using Allocator = std::allocator<std::pair<const int, std::pair<std::string, size_t>>>;
ColdZoneOptimized<int, std::string, Allocator> cold_zone;

// 批量插入
std::vector<std::pair<int, std::pair<std::string, size_t>>> batch;
cold_zone.insert_batch(batch.begin(), batch.end());
```

## 3. FreqPartitionDictOptimized - 整合优化

### 特性

- 使用 `HotZoneOptimized` 替代基础版热区
- 使用 `ColdZoneOptimized` 替代基础版冷区
- 提供内存压缩接口

### 额外接口

```cpp
// 压缩热区内存（清理延迟删除的条目）
void compact_hot_zone();

// 获取热区原始大小（包括延迟删除的）
size_t hot_zone_raw_size() const;
```

## 4. FreqPartitionDictThreadSafe - 线程安全版

### 同步策略

**读写锁（`std::shared_mutex`）**：
- 读操作（`get`, `contains`）：共享锁，允许多线程并发读
- 写操作（`insert`, `erase`）：独占锁，保证数据一致性

### 批量操作

为了减少锁开销，提供批量操作接口：

```cpp
// 批量插入
template<typename Iterator>
void insert_batch(Iterator begin, Iterator end);

// 批量读取
template<typename Container>
void get_batch(const Container& keys, std::vector<std::optional<V>>& results);
```

### 性能建议

| 场景 | 推荐方案 |
|-----|---------|
| 读多写少 | 使用 `contains` + `get` 组合 |
| 大量写入 | 使用 `insert_batch` |
| 高并发读取 | 使用 `get_batch` |

## 5. 性能对比

### 测试环境
- CPU: Intel i7-12700H
- 内存: 16GB DDR5
- 编译器: GCC 13.2
- 编译选项: `-O3 -DNDEBUG`

### 测试结果

| 测试项 | 基础版 | 优化版 | 线程安全版 |
|-------|-------|-------|----------|
| Zipf查找 (H=128) | 45 ns | 38 ns | 52 ns |
| 均匀查找 | 89 ns | 82 ns | 105 ns |
| 插入 | 156 ns | 148 ns | 189 ns |
| 淘汰操作 | 1280 ns | 89 ns | - |

**结论**：
- 优化版在淘汰操作上提升明显（14x）
- 线程安全版有约 15-20% 的同步开销

## 6. 使用建议

### 教学场景
```cpp
#include <freq_partition_dict.hpp>
// 使用基础版，代码清晰易懂
```

### 生产环境（单线程）
```cpp
#include <freq_partition_dict_optimized.hpp>
// 使用优化版，性能更好
```

### 生产环境（多线程）
```cpp
#include <freq_partition_dict_threadsafe.hpp>
// 使用线程安全版
```

## 7. 进一步优化方向

### 可能的优化

1. **无锁热区**：使用无锁哈希表实现热区
2. **自适应阈值**：根据访问模式动态调整晋升阈值
3. **多级热区**：L1/L2 多级缓存结构
4. **SIMD优化**：批量比较频率值

### 贡献指南

欢迎提交优化版本的改进！请确保：
1. 保持 API 兼容性
2. 添加对应的单元测试
3. 更新本文档
