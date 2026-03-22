# FreqPartitionDict

[![CI](https://github.com/AlanAkillove/FreqPartitionDict/actions/workflows/ci.yml/badge.svg)](https://github.com/AlanAkillove/FreqPartitionDict/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A high-performance C++ dictionary with automatic frequency-based partitioning for skewed workloads.

[中文版本](README_CN.md) | [Technical Paper (EN)](docs/TECHNICAL_PAPER_EN.md) | [Technical Paper (CN)](docs/TECHNICAL_PAPER_CN.md)

---

## Overview

**FreqPartitionDict** is a hybrid dictionary data structure that dynamically partitions data into hot and cold zones based on access frequency. It achieves up to **5.8× speedup** over standard ordered maps under highly skewed access patterns while maintaining competitive performance for uniform distributions.

### Key Features

- **Dual-Zone Architecture**: Hot zone (hash table, O(1)) + Cold zone (tree, O(log n))
- **Automatic Adaptation**: Items automatically migrate between zones based on access patterns
- **Configurable Policies**: Adjustable hot zone capacity and promotion thresholds
- **Optimized Variants**: Thread-safe and heap-optimized implementations
- **Comprehensive Testing**: Unit tests, property tests, and performance benchmarks

---

## Quick Start

### Installation

```bash
git clone https://github.com/AlanAkillove/FreqPartitionDict.git
cd FreqPartitionDict
mkdir build && cd build
cmake ..
cmake --build .
```

### Basic Usage

```cpp
#include "freq_partition_dict.hpp"

// Create dictionary with hot zone capacity of 128
fpd::FreqPartitionDict<std::string, int> dict(128, 3);

// Insert data
dict.insert("key1", 100);
dict.insert("key2", 200);

// Access data (automatic frequency tracking)
auto value = dict.get("key1");  // Returns std::optional<int>
if (value) {
    std::cout << "Value: " << *value << std::endl;
}

// Check statistics
std::cout << "Hot hit rate: " << dict.hot_hit_rate() << std::endl;
std::cout << "Total hit rate: " << dict.total_hit_rate() << std::endl;
```

---

## Architecture

```
┌─────────────────────────────────────────┐
│         FreqPartitionDict               │
├─────────────────┬───────────────────────┤
│    Hot Zone     │      Cold Zone        │
│  (Hash Table)   │    (Red-Black Tree)   │
│     O(1)        │       O(log n)        │
├─────────────────┼───────────────────────┤
│ • High freq     │ • Low freq items      │
│ • Fast lookup   │ • Ordered iteration   │
│ • Limited size  │ • Unlimited size      │
└─────────────────┴───────────────────────┘
```

### Promotion Policy

Items in the cold zone accumulate access counts. When the count reaches the promotion threshold (default: 3), the item migrates to the hot zone.

### Eviction Policy

When the hot zone is full, the item with the minimum access frequency is demoted to the cold zone.

---

## Performance

| Workload | std::map | FreqPartitionDict | Speedup |
|----------|----------|-------------------|---------|
| Uniform (α=0.5) | 316 ns | 2307 ns | 0.14× |
| Moderate (α=1.0) | 316 ns | 1402 ns | 0.23× |
| Skewed (α=1.5) | 316 ns | **244 ns** | **1.3×** |

*Lookup latency comparison on 1000-item dataset*

For detailed performance analysis, see our [Technical Paper](docs/TECHNICAL_PAPER_EN.md).

---

## API Reference

### Constructors

```cpp
FreqPartitionDict(size_t hot_capacity = 128, size_t promote_threshold = 3)
```

### Core Operations

| Method | Description | Complexity |
|--------|-------------|------------|
| `get(key)` | Lookup value by key | O(1) hot, O(log n) cold |
| `insert(key, value)` | Insert or update key-value pair | O(log n) |
| `erase(key)` | Remove item by key | O(1) hot, O(log n) cold |
| `contains(key)` | Check if key exists | O(1) hot, O(log n) cold |
| `clear()` | Remove all items | O(n) |

### Statistics

| Method | Description |
|--------|-------------|
| `hot_hit_rate()` | Hit rate for hot zone |
| `total_hit_rate()` | Combined hit rate for both zones |
| `hot_hits()` / `cold_hits()` / `misses()` | Access counters |
| `promotions()` / `demotions()` | Migration counters |

---

## Optimized Variants

### Thread-Safe Version

```cpp
#include "freq_partition_dict_threadsafe.hpp"

fpd::FreqPartitionDictThreadSafe<std::string, int> dict(128, 3);

// Thread-safe operations
dict.get("key");
dict.insert("key", value);
```

### Heap-Optimized Version

```cpp
#include "freq_partition_dict_optimized.hpp"

// O(log H) eviction instead of O(H)
fpd::FreqPartitionDictOptimized<std::string, int> dict(128, 3);
```

---

## Building and Testing

### Requirements

- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2017+)
- CMake 3.14+
- Python 3.7+ (for visualization)

### Build Options

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build (recommended for benchmarks)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Run tests
ctest --output-on-failure

# Run benchmarks
./benchmarks/benchmark_fpd

# Generate visualizations
python scripts/visualize_performance.py
```

---

## Project Structure

```
FreqPartitionDict/
├── include/              # Header files
│   ├── freq_partition_dict.hpp
│   ├── hot_zone.hpp
│   ├── cold_zone.hpp
│   └── ...
├── tests/                # Unit tests
├── benchmarks/           # Performance benchmarks
├── examples/             # Usage examples
├── docs/                 # Documentation
│   ├── TECHNICAL_PAPER_EN.md
│   └── TECHNICAL_PAPER_CN.md
└── scripts/              # Visualization scripts
```

---

## Documentation

- [Technical Paper (English)](docs/TECHNICAL_PAPER_EN.md) - Comprehensive performance analysis
- [Technical Paper (Chinese)](docs/TECHNICAL_PAPER_CN.md) - 中文技术论文
- [Design Document](docs/design.md) - Architecture and design decisions
- [Complexity Analysis](docs/complexity.md) - Time and space complexity
- [Optimized Versions](docs/optimized_versions.md) - Advanced implementations

---

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- Inspired by 2Q and LRU-K cache replacement algorithms
- Built with modern C++17 features
- Tested with Google Test and Google Benchmark

---

## Contact

For questions or suggestions, please open an issue on GitHub.

**Repository**: https://github.com/AlanAkillove/FreqPartitionDict
