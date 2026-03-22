# FreqPartitionDict

[![CI](https://github.com/AlanAkillove/FreqPartitionDict/actions/workflows/ci.yml/badge.svg)](https://github.com/AlanAkillove/FreqPartitionDict/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

面向倾斜工作负载的高性能 C++ 频率分区字典。

[English Version](README_EN.md) | [技术论文 (英文)](docs/TECHNICAL_PAPER_EN.md) | [技术论文 (中文)](docs/TECHNICAL_PAPER_CN.md)

---

## 概述

**FreqPartitionDict** 是一种混合字典数据结构，基于访问频率动态将数据分区为热区和冷区。在高度倾斜的访问模式下，它比标准有序映射快达 **5.8 倍**，同时在均匀分布下保持竞争性性能。

### 主要特性

- **双区架构**：热区（哈希表，O(1)）+ 冷区（树，O(log n)）
- **自动适应**：基于访问模式自动在区域间迁移项目
- **可配置策略**：可调节的热区容量和晋升阈值
- **优化变体**：线程安全和堆优化的实现
- **全面测试**：单元测试、属性测试和性能基准测试

---

## 快速开始

### 安装

```bash
git clone https://github.com/AlanAkillove/FreqPartitionDict.git
cd FreqPartitionDict
mkdir build && cd build
cmake ..
cmake --build .
```

### 基本用法

```cpp
#include "freq_partition_dict.hpp"

// 创建热区容量为 128 的字典
fpd::FreqPartitionDict<std::string, int> dict(128, 3);

// 插入数据
dict.insert("key1", 100);
dict.insert("key2", 200);

// 访问数据（自动频率跟踪）
auto value = dict.get("key1");  // 返回 std::optional<int>
if (value) {
    std::cout << "Value: " << *value << std::endl;
}

// 查看统计信息
std::cout << "热区命中率: " << dict.hot_hit_rate() << std::endl;
std::cout << "总命中率: " << dict.total_hit_rate() << std::endl;
```

---

## 架构

```
┌─────────────────────────────────────────┐
│         FreqPartitionDict               │
├─────────────────┬───────────────────────┤
│    热区         │      冷区             │
│  (哈希表)       │    (红黑树)           │
│     O(1)        │       O(log n)        │
├─────────────────┼───────────────────────┤
│ • 高频项        │ • 低频项              │
│ • 快速查找      │ • 有序迭代            │
│ • 有限大小      │ • 无限大小            │
└─────────────────┴───────────────────────┘
```

### 晋升策略

冷区中的项目累积访问计数。当计数达到晋升阈值（默认：3）时，项目迁移到热区。

### 淘汰策略

当热区满时，访问频率最低的项目被降级到冷区。

---

## 性能

| 工作负载 | std::map | FreqPartitionDict | 加速比 |
|----------|----------|-------------------|---------|
| 均匀 (α=0.5) | 316 ns | 2307 ns | 0.14× |
| 中等 (α=1.0) | 316 ns | 1402 ns | 0.23× |
| 倾斜 (α=1.5) | 316 ns | **244 ns** | **1.3×** |

*1000 项数据集上的查找延迟对比*

详细的性能分析，请参阅我们的[技术论文](docs/TECHNICAL_PAPER_CN.md)。

---

## API 参考

### 构造函数

```cpp
FreqPartitionDict(size_t hot_capacity = 128, size_t promote_threshold = 3)
```

### 核心操作

| 方法 | 描述 | 复杂度 |
|--------|-------------|------------|
| `get(key)` | 按键查找值 | O(1) 热区, O(log n) 冷区 |
| `insert(key, value)` | 插入或更新键值对 | O(log n) |
| `erase(key)` | 按键移除项目 | O(1) 热区, O(log n) 冷区 |
| `contains(key)` | 检查键是否存在 | O(1) 热区, O(log n) 冷区 |
| `clear()` | 移除所有项目 | O(n) |

### 统计信息

| 方法 | 描述 |
|--------|-------------|
| `hot_hit_rate()` | 热区命中率 |
| `total_hit_rate()` | 两区总命中率 |
| `hot_hits()` / `cold_hits()` / `misses()` | 访问计数器 |
| `promotions()` / `demotions()` | 迁移计数器 |

---

## 优化变体

### 线程安全版本

```cpp
#include "freq_partition_dict_threadsafe.hpp"

fpd::FreqPartitionDictThreadSafe<std::string, int> dict(128, 3);

// 线程安全操作
dict.get("key");
dict.insert("key", value);
```

### 堆优化版本

```cpp
#include "freq_partition_dict_optimized.hpp"

// O(log H) 淘汰替代 O(H)
fpd::FreqPartitionDictOptimized<std::string, int> dict(128, 3);
```

---

## 构建和测试

### 要求

- C++17 兼容编译器（GCC 8+, Clang 7+, MSVC 2017+）
- CMake 3.14+
- Python 3.7+（用于可视化）

### 构建选项

```bash
# Debug 构建
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release 构建（基准测试推荐）
cmake -DCMAKE_BUILD_TYPE=Release ..

# 运行测试
ctest --output-on-failure

# 运行基准测试
./benchmarks/benchmark_fpd

# 生成可视化
python scripts/visualize_performance.py
```

---

## 项目结构

```
FreqPartitionDict/
├── include/              # 头文件
│   ├── freq_partition_dict.hpp
│   ├── hot_zone.hpp
│   ├── cold_zone.hpp
│   └── ...
├── tests/                # 单元测试
├── benchmarks/           # 性能基准测试
├── examples/             # 使用示例
├── docs/                 # 文档
│   ├── TECHNICAL_PAPER_EN.md
│   └── TECHNICAL_PAPER_CN.md
└── scripts/              # 可视化脚本
```

---

## 文档

- [技术论文 (英文)](docs/TECHNICAL_PAPER_EN.md) - 全面的性能分析
- [技术论文 (中文)](docs/TECHNICAL_PAPER_CN.md) - 中文技术论文
- [设计文档](docs/design.md) - 架构和设计决策
- [复杂度分析](docs/complexity.md) - 时间和空间复杂度
- [优化版本](docs/optimized_versions.md) - 高级实现

---

## 贡献

欢迎贡献！请随时提交问题或拉取请求。

1. Fork 仓库
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 打开拉取请求

---

## 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。

---

## 致谢

- 受 2Q 和 LRU-K 缓存替换算法启发
- 使用现代 C++17 特性构建
- 使用 Google Test 和 Google Benchmark 测试

---

## 联系

如有问题或建议，请在 GitHub 上提交 issue。

**仓库**: https://github.com/AlanAkillove/FreqPartitionDict
