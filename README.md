# FreqPartitionDict - 频率分区字典

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.14+-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

一个基于访问频率的分区字典实现。

## 项目简介

`FreqPartitionDict` 是一种混合数据结构，结合了哈希表和平衡树的优点，通过将高频访问的数据保存在"热区"（哈希表，O(1) 查找）和低频数据保存在"冷区"（红黑树，O(log n) 查找），实现对倾斜访问模式的高效处理。

### 核心特性

- **双区存储**: 热区使用 `std::unordered_map` 实现 O(1) 查找，冷区使用 `std::map` 实现 O(log n) 查找
- **自适应晋升**: 基于访问频率的晋升策略，高频数据自动进入热区
- **频率淘汰**: 热区满时淘汰访问频率最低的元素
- **完整统计**: 提供命中率、晋升/降级次数等详细统计信息
- **纯头文件**: 模板实现，只需包含头文件即可使用
- **教学友好**: 代码清晰，注释详尽，适合学习数据结构

## 快速开始

### 环境要求

- C++17 兼容的编译器 (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.14 或更高版本

### 构建项目

```bash
# 克隆仓库
git clone https://github.com/yourusername/freq_partition_dict.git
cd freq_partition_dict

# 创建构建目录
mkdir build && cd build

# 配置和构建
cmake ..
cmake --build .

# 运行测试
ctest

# 运行示例
./examples/basic_usage
./examples/zipf_demo

# 运行基准测试
./benchmarks/benchmark_fpd
```

### 基础用法

```cpp
#include <freq_partition_dict.hpp>
#include <iostream>

int main() {
    // 创建字典：热区容量 128，晋升阈值 3
    fpd::FreqPartitionDict<int, std::string> dict(128, 3);

    // 插入数据
    dict.insert(1, "one");
    dict.insert(2, "two");

    // 访问数据
    auto result = dict.get(1);
    if (result) {
        std::cout << "Value: " << *result << std::endl;
    }

    // 查看统计
    std::cout << "Hot hit rate: " << dict.hot_hit_rate() * 100 << "%" << std::endl;

    return 0;
}
```

## 设计原理

### 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                    FreqPartitionDict                        │
├─────────────────────────────┬───────────────────────────────┤
│         Hot Zone            │          Cold Zone            │
│    (std::unordered_map)     │       (std::map)              │
│                             │                               │
│  ┌─────┐  ┌─────┐  ┌─────┐  │    ┌───┐                     │
│  │ K1  │  │ K2  │  │ K3  │  │    │K10│                     │
│  │ f:5 │  │ f:3 │  │ f:8 │  │    └───┘                     │
│  └──┬──┘  └──┬──┘  └──┬──┘  │      │                       │
│     └─────────┴────────┘     │    ┌─┴─┐                     │
│        O(1) Lookup           │    │K20│                     │
│                              │    └───┘                     │
│  晋升: 冷区元素访问达到阈值    │      │                       │
│  淘汰: 频率最低的元素         │    ┌─┴─┐                     │
│                              │    │K30│                     │
└──────────────────────────────┘    └───┘                     │
                                     O(log n) Lookup          │
                                                              │
                                    降级: 热区满时淘汰的元素    │
└─────────────────────────────────────────────────────────────┘
```

### 晋升策略

1. 冷区元素每次被访问时，其频率计数器增加
2. 当频率达到晋升阈值时，元素被晋升到热区
3. 如果热区已满，频率最低的元素被降级到冷区

### 复杂度分析

| 操作 | 平均时间复杂度 | 最坏时间复杂度 |
|------|--------------|--------------|
| 查找（热区命中） | O(1) | O(n) |
| 查找（冷区命中） | O(log n) | O(log n) |
| 插入 | O(log n) | O(log n) |
| 删除 | O(log n) | O(log n) |

其中 n 是总元素数。在倾斜访问模式下（如 Zipf 分布），大部分访问命中热区，均摊查找复杂度接近 O(1)。

## 性能对比

在 Zipf 分布（α=1.0）访问模式下，与标准容器的对比：

| 数据结构 | 平均查找时间 | 热区命中率 |
|---------|------------|-----------|
| `std::unordered_map` | 基准 | N/A |
| `std::map` | ~3x 基准 | N/A |
| `FreqPartitionDict` (H=32) | ~1.2x 基准 | ~65% |
| `FreqPartitionDict` (H=128) | ~1.1x 基准 | ~85% |

## 项目结构

```
freq_partition_dict/
├── include/                    # 头文件
│   ├── freq_partition_dict.hpp # 主类
│   ├── hot_zone.hpp            # 热区实现
│   └── cold_zone.hpp           # 冷区实现
├── src/                        # 源文件（模板类，暂无）
├── tests/                      # 单元测试
│   ├── test_correctness.cpp    # 正确性测试
│   └── test_properties.cpp     # 属性测试
├── benchmarks/                 # 性能基准测试
│   └── benchmark.cpp           # Google Benchmark
├── examples/                   # 示例程序
│   ├── basic_usage.cpp         # 基础用法
│   └── zipf_demo.cpp           # Zipf 分布演示
├── docs/                       # 文档
│   ├── design.md               # 设计文档
│   └── complexity.md           # 复杂度分析
└── CMakeLists.txt              # 构建配置
```

## API 参考

### FreqPartitionDict<K, V>

#### 构造函数

```cpp
FreqPartitionDict(size_t hot_capacity = 128, size_t promote_threshold = 3);
```

- `hot_capacity`: 热区最大容量
- `promote_threshold`: 冷区元素晋升到热区所需的访问次数

#### 主要方法

| 方法 | 说明 |
|-----|------|
| `insert(key, value)` | 插入键值对 |
| `get(key)` | 查找键，返回 `std::optional<V>` |
| `contains(key)` | 检查键是否存在 |
| `erase(key)` | 删除键 |
| `clear()` | 清空字典 |
| `size()` | 返回总元素数 |
| `hot_size()` | 返回热区元素数 |
| `cold_size()` | 返回冷区元素数 |

#### 统计方法

| 方法 | 说明 |
|-----|------|
| `hot_hit_rate()` | 热区命中率 |
| `total_hit_rate()` | 总命中率 |
| `hot_hits()` | 热区命中次数 |
| `cold_hits()` | 冷区命中次数 |
| `misses()` | 未命中次数 |
| `promotions()` | 晋升次数 |
| `demotions()` | 降级次数 |
| `reset_stats()` | 重置统计 |

## 学习资源

本项目适合用于学习以下数据结构和算法概念：

- **哈希表**: 热区使用 `std::unordered_map` 的实现
- **平衡树**: 冷区使用 `std::map`（红黑树）的实现
- **缓存替换策略**: LRU、LFU 与频率分区的对比
- **工作集模型**: 程序访问局部性原理
- **Zipf 分布**: 真实世界访问模式的建模

## 贡献指南

欢迎提交 Issue 和 Pull Request！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

## 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

## 致谢

- 感谢 Google Test 和 Google Benchmark 提供的测试框架

---

