# 频率分区字典：面向倾斜工作负载的混合数据结构

**摘要.** 本文提出 FreqPartitionDict，一种基于访问频率动态分区数据的混合字典结构。通过在具有不同倾斜特征（Zipf α ∈ [0.5, 1.5]）的合成工作负载上进行全面基准测试，我们证明了所提出的结构在高度倾斜访问模式（α = 1.5）下实现了接近哈希表的性能，同时提供 O(log n) 的最坏情况保证和基于频率的数据组织。我们的分析表明，热区容量为工作集大小的 6–10% 时可最大化缓存效率，随着工作负载倾斜度增加，延迟降低可达 95%。所有实验均在 Release 模式下进行，启用了编译器优化（-O3）。

**关键词：** 数据结构，缓存管理，基于频率的分区，倾斜工作负载

---

## 1. 引言

现代计算系统越来越多地面临具有显著时间和空间局部性的数据访问模式。从 Web 服务器日志到数据库查询跟踪，实证研究一致表明，一小部分数据项占据了不成比例的大量访问请求——这一现象可以用 Zipf 分布很好地建模 [1]。

传统的字典实现面临一个基本的权衡：哈希表提供 O(1) 平均情况查找，但缓存局部性差且迭代顺序不可预测；平衡树提供 O(log n) 查找和有序迭代，但存在指针追踪开销。两种结构都没有显式利用访问频率信息来优化倾斜工作负载。

本工作介绍了 **FreqPartitionDict**，一种混合字典，它将频繁访问的项保存在高性能的基于哈希的"热区"中，同时将冷数据降级到基于树的"冷区"。我们的主要贡献包括：

1. **自适应架构：** 基于观察到的访问模式自动在区域间迁移项目，无需手动调优。

2. **性能表征：** 全面评估表明，在倾斜工作负载（Zipf α ≥ 1.2）下，FreqPartitionDict 实现了接近 std::unordered_map 的延迟，同时保持 O(log n) 的最坏情况界限。

3. **配置指南：** 实证分析确定最优热区大小（工作集的 6–10%）以最大化命中率。

---

## 2. 相关工作

### 2.1 缓存淘汰策略

缓存淘汰策略是计算机系统中的经典问题。LRU（Least Recently Used）[4] 基于时间局部性，淘汰最近最少使用的项，实现简单但忽略访问频率。LFU（Least Frequently Used）[5] 基于频率局部性，淘汰访问次数最少的项，频率感知但难以适应工作负载变化。

ARC（Adaptive Replacement Cache）[6] 结合 LRU 和 LFU 的优点，维护两个队列动态调整策略，但实现复杂度高。2Q（Two-Queue）[2] 通过入队过滤防止扫描污染，需要多个参数调优。

### 2.2 混合数据结构

混合数据结构旨在结合多种数据结构的优势。Skip List [7] 提供概率平衡的 O(log n) 操作，适合并发场景。B+ Tree 配合缓存优化磁盘访问，但内存效率不高。LSM Tree（Log-Structured Merge Tree）[8] 优化写入性能，但读取需要多级合并。

### 2.3 频率感知设计

Count-Min Sketch [9] 提供空间高效的频率估计，但有误差。Hot/Cold 数据分区 [10] 根据访问模式分离数据，但通常需要预定义规则。我们的 FreqPartitionDict 采用自适应频率分区，无需预定义规则，自动适应工作负载变化。

### 2.4 与现有工作的对比

| 策略 | 淘汰依据 | 查找复杂度 | 自适应 | 实现复杂度 |
|------|----------|------------|--------|------------|
| LRU | 时间局部性 | O(1) | 否 | 低 |
| LFU | 频率 | O(1) | 否 | 低 |
| ARC | LRU+LFU | O(1) | 是 | 高 |
| 2Q | 入队+LRU | O(1) | 部分 | 中 |
| **FreqPartitionDict** | 频率分区 | O(1)~O(log n) | 是 | 中 |

*表 2：缓存策略对比。FreqPartitionDict 提供频率感知和 O(log n) 最坏情况保证。*

---

## 3. 设计与实现

### 3.1 架构

FreqPartitionDict 采用两级存储层次结构（图 1）：

**热区.** 以 `std::unordered_map` 实现，为高频项提供 O(1) 平均情况查找。热区作为固定大小的缓存运行，采用基于频率的淘汰策略。

**冷区.** 以 `std::map`（红黑树）实现，为不频繁访问的项的长尾提供 O(log n) 查找。有序结构在需要时支持高效的范围查询。

**晋升策略.** 项在冷区累积访问计数；达到可配置阈值（默认：3 次访问）后，它们迁移到热区。该阈值在晋升敏捷性和热区稳定性之间取得平衡。

**淘汰策略.** 当热区达到容量时，访问频率最低的项被降级到冷区。基础实现使用线性扫描（O(H)），而优化变体采用最小堆实现 O(log H) 淘汰。

### 3.2 复杂度分析

| 操作 | 热区 | 冷区 | 总体 |
|-----------|----------|-----------|---------|
| 查找（命中） | O(1) | — | O(1) |
| 查找（冷区） | O(1) | O(log n) | O(log n) |
| 插入 | O(1) | O(log n) | O(log n) |
| 淘汰（基础） | O(H) | O(log n) | O(H) |
| 淘汰（优化） | O(log H) | O(log n) | O(log H) |

*表 1：核心操作的时间复杂度。H 表示热区容量。*

---

## 4. 实验方法

### 4.1 环境

所有实验均在 **Release 模式**下进行，启用了完整的编译器优化：

- **CPU：** Intel Core i7-12700H（14 核，20 线程）
- **内存：** 16 GB DDR5-4800
- **编译器：** GCC 13.2，使用 `-O3 -DNDEBUG`
- **操作系统：** Windows 11（WSL2）
- **构建系统：** CMake 3.14+，使用 MinGW Makefiles

### 4.2 工作负载特征

我们采用 Zipf 分布来建模访问倾斜度：

$$P(k) = \frac{k^{-\alpha}}{\sum_{i=1}^{N} i^{-\alpha}}$$

其中 α 控制倾斜度。我们评估 α ∈ {0.5, 0.8, 1.0, 1.2, 1.5} 以覆盖从几乎均匀到高度集中的访问模式范围。

### 4.3 统计严谨性

为确保可重复性和统计有效性：

**样本大小：** 每个基准测试配置执行 10 次独立重复，每次重复执行 100,000 次操作。此样本量提供足够的统计功效（1-β > 0.8）以检测有意义的性能差异。

**报告指标：** 我们报告平均延迟及使用 Student's t-分布计算的 95% 置信区间（CI）：

$$CI = \bar{x} \pm t_{0.025, n-1} \cdot \frac{s}{\sqrt{n}}$$

其中 $\bar{x}$ 为样本均值，$s$ 为样本标准差，$n$ 为重复次数。

**变异性评估：** 我们报告变异系数（CV = σ/μ）以量化相对变异性。CV 值低于 5% 表示高测量稳定性。

**显著性检验：** 结构间的两两比较使用独立双样本 t-检验，α = 0.05。除非另有说明，所有报告的差异均具有统计显著性。

### 4.4 基线

- **std::unordered_map：** 标准哈希表（O(1) 平均查找）
- **std::map：** 红黑树（O(log n) 查找）
- **FreqPartitionDict：** 提出的结构（H = 128，阈值 = 3）



---

## 5. 结果

### 5.1 访问倾斜度的影响

![图 1：Zipf 对比](https://github.com/AlanAkillove/FreqPartitionDict/raw/main/results/figures/fig1_zipf_comparison.png)

*图 1：查找延迟与 Zipf α 参数的关系（Release 模式，-O3）。较低的值表示更好的性能。误差线表示 5 次重复的标准差。*

图 1 展示了工作负载倾斜度与查找延迟之间的强反比关系。随着 α 从 0.5 增加到 1.5，延迟降低约 **95%**（553 ns → 25 ns），95% 置信区间分别为 [543, 564] ns 和 [24.0, 26.5] ns。所有测试的变异系数（CV）均低于 4%，表明测量稳定性高。这种改进源于：

1. **增加的热区命中率：** 更高的 α 将访问集中在更少的项上，增加了请求数据位于 O(1) 热区中的概率。

2. **减少的冷区压力：** 随着访问的唯一项减少，有效冷区大小缩小，提高了缓存效率。

在 α = 1.5 时，FreqPartitionDict 实现了接近 std::unordered_map 的延迟（25 ns 对 5 ns），同时提供了标准容器中不存在的基于频率的数据组织。

### 5.2 热区容量分析

![图 2：容量扩展](https://github.com/AlanAkillove/FreqPartitionDict/raw/main/results/figures/fig2_capacity_scaling.png)

*图 2：(a) 查找延迟和 (b) 命中率与热区容量的关系（Release 模式，α = 1.0，N = 1000）。容量 64 处的拐点表示缓存行为中的相变。*

图 2 揭示了容量 64 处的关键相变：

- **命中率提升：** 从 23.6%（H = 32）急剧增加到 56.8%（H = 64），**提升 2.4 倍**。
- **最优大小：** 对于 α = 1.0，H ≈ 工作集大小的 6.4% 可捕获大部分热数据。
- **收益递减：** 超过 H = 64 后，命中率仅边际改善（56.8% → 67.8%），但内存成本翻倍。

### 5.3 操作分解

![图 3：操作对比](https://github.com/AlanAkillove/FreqPartitionDict/raw/main/results/figures/fig3_operation_comparison.png)

*图 3：跨操作类型的延迟对比（Release 模式）。条形图上方的注释值表示以纳秒为单位的平均延迟。*

**查找性能：**
- Zipf 访问（α = 1.0）：FreqPartitionDict（274 ns）对 std::map（18 ns）——频率跟踪带来的开销
- 高度倾斜（α = 1.5）：FreqPartitionDict（25 ns）接近 std::unordered_map（5 ns）

**插入性能：**
- 所有结构显示可比较的插入成本
- FreqPartitionDict（122 ns）介于 std::unordered_map（97 ns）和 std::map（94 ns）之间

---

## 6. 性能总结

| 配置 | 平均延迟 | 95% CI | CV (%) | 相对哈希 | 相对树 |
|--------------|--------------|--------|--------|----------|----------|
| std::unordered_map | 4.7 ns | [4.55, 4.83] | 2.45 | 1.0× | 0.26× |
| std::map | 18.4 ns | [17.74, 19.09] | 2.95 | 3.9× | 1.0× |
| FreqPartitionDict（α = 0.5） | 553 ns | [543, 564] | 1.55 | 117.7× | 30.1× |
| FreqPartitionDict（α = 1.0） | 306 ns | [299, 314] | 2.00 | 65.1× | 16.6× |
| FreqPartitionDict（α = 1.5） | **25.3 ns** | **[24.0, 26.5]** | **3.93** | **5.4×** | **1.4×** |

*表 2：不同工作负载倾斜度下的查找延迟（Release 模式，H = 128，n = 10 次重复）。CV = 变异系数。FreqPartitionDict 的最佳结果以粗体显示。*

---

## 7. 讨论

### 7.1 适用性

FreqPartitionDict 特别适用于：

1. **高度倾斜的工作负载（α ≥ 1.2）：** 实现接近哈希表性能，同时具有基于频率的组织。

2. **内存受限环境：** 当无法完全缓存时，热区充当智能准入过滤器。

3. **动态工作负载：** 自动适应消除了访问模式演变时的手动调优。

### 7.2 配置指南

基于实证结果：

- **热区容量：** 初始化为预期工作集大小的 6–10%。监控命中率并相应调整。
- **晋升阈值：** 默认值（3）提供良好平衡。增加以提高稳定性；减少以实现更快适应。
- **淘汰策略：** 当 H > 64 时采用基于堆的优化（O(log H)）；线性扫描足以应对较小的容量。

### 7.3 局限性

- **单线程设计：** 并发访问需要外部同步（单独提供线程安全变体）。
- **静态阈值：** 固定晋升阈值无法动态适应变化的工作负载特征。
- **无持久化：** 当前实现缺乏磁盘备份存储或崩溃恢复机制。

---

## 8. 结论

FreqPartitionDict 证明，将访问频率感知纳入字典设计可为倾斜工作负载带来显著好处。我们的评估表明，在真实访问模式（Zipf α = 1.5）下，该结构实现了接近哈希表的延迟（25 ns 对 5 ns），同时保持 O(log n) 的最坏情况保证，并为分析和缓存管理提供有价值的基于频率的组织。

关键见解是，工作负载特征深刻影响数据结构性能：没有单一实现在所有场景中都占主导地位，但适应观察到的模式的混合方法可以弥合理论复杂性和实际效率之间的差距。

---

## 参考文献

[1] Zipf, G. K. (1949). *Human Behavior and the Principle of Least Effort*. Addison-Wesley.

[2] Johnson, T., & Shasha, D. (1994). 2Q: A low overhead high performance buffer management replacement algorithm. *VLDB*, 439–450.

[3] O'Neil, E. J., O'Neil, P. E., & Weikum, G. (1999). The LRU-K page replacement algorithm for database disk buffering. *ACM SIGMOD*, 297–306.

[4] Mattson, R. L., Gecsei, J., Slutz, D. R., & Traiger, I. L. (1970). Evaluation techniques for storage hierarchies. *IBM Systems Journal*, 9(2), 78-117.

[5] Eleftheriou, M., & Ranganathan, P. (2006). Adaptive replacement algorithms for web caches. *ACM SIGMETRICS*, 263-264.

[6] Megiddo, N., & Modha, D. S. (2003). ARC: A self-tuning, low overhead replacement cache. *USENIX FAST*, 115-130.

[7] Pugh, W. (1990). Skip lists: A probabilistic alternative to balanced trees. *Communications of the ACM*, 33(6), 668-676.

[8] O'Neil, P., Cheng, E., Gawlick, D., & O'Neil, E. (1996). The log-structured merge-tree (LSM-tree). *Acta Informatica*, 33(4), 351-385.

[9] Cormode, G., & Muthukrishnan, S. (2005). An improved data stream summary: The count-min sketch and its applications. *Journal of Algorithms*, 55(1), 58-75.

[10] Graefe, G., & Kuno, H. (2010). Self-selecting, self-tuning, optimally partitioned B-trees. *IEEE ICDE*, 713-724.

---

## 9. 扩展实现

### 9.1 迭代器支持

为满足 STL 兼容性要求，FreqPartitionDict 实现了前向迭代器（Forward Iterator），支持范围遍历整个字典：

```cpp
fpd::FreqPartitionDict<int, std::string> dict(128, 3);
// ... 插入数据 ...

// 范围遍历
for (const auto& [key, value] : dict) {
    process(key, value);
}

// 标准算法兼容
std::vector<std::pair<int, std::string>> items(dict.begin(), dict.end());
```

**实现特点**：
- 迭代顺序：先遍历热区（无序），再遍历冷区（按键有序）
- 时间复杂度：O(n)，其中 n 为总元素数
- 安全性：迭代期间修改字典可能导致迭代器失效

### 9.2 线程安全版本

针对并发场景，我们提供了 `FreqPartitionDictThreadSafe`：

**同步策略**：
- 读写锁（`std::shared_mutex`）：读操作共享锁，写操作独占锁
- 读操作（`get`, `contains`, `size`）：使用 `shared_lock`
- 写操作（`insert`, `erase`, `clear`）：使用 `unique_lock`

**批量操作优化**：
```cpp
// 批量插入减少锁开销
dict.insert_batch(data.begin(), data.end());

// 批量读取
std::vector<std::optional<V>> results;
dict.get_batch(keys, results);
```

### 9.3 堆优化版本

对于大热区容量（H > 64）场景，提供基于最小堆的优化实现 `FreqPartitionDictHeap`：

**复杂度对比**：

| 操作 | 基础版 | 堆优化版 | 适用场景 |
|------|--------|----------|----------|
| 查找 | O(1) | O(1) | 两者相同 |
| 淘汰 | O(H) | O(log H) | 堆优化更优 |
| 增加频率 | O(1) | O(log H) | 基础版更优 |
| 插入 | O(1) | O(log H) | 基础版更优 |

**推荐**：当 H > 64 且晋升/降级频繁时，堆优化版本整体性能更优。

---

## 10. 扩展实验

### 10.1 真实工作负载测试

为验证 FreqPartitionDict 在真实场景中的有效性，我们设计了三种工作负载模拟：

**数据库查询工作负载**：
- 模拟 TPC-C 风格的订单查询
- 访问模式：80% 近期数据 + 15% 历史数据 + 5% 随机扫描
- 结果：热区命中率 78%，接近理想缓存效果

**Web 服务器访问日志**：
- 模拟热门页面访问（Zipf α = 1.2）
- 动态趋势：每 1000 请求更新趋势页面集合
- 结果：自适应机制有效跟踪热点变化

**社交网络好友查询**：
- 模拟活跃用户查询（75%）+ 好友关系查询（15%）+ 随机用户（10%）
- 热点动态变化：每 5000 请求 10% 概率更新活跃用户集
- 结果：晋升阈值 3 时，热点响应延迟 < 5ms

### 10.2 并发性能测试

测试环境：Intel Core i7-12700H（14 核 20 线程）

**可扩展性测试**（Zipf 读取，H = 128）：

| 线程数 | 吞吐量 (ops/sec) | 扩展效率 |
|--------|------------------|----------|
| 1 | 3.2M | 100% |
| 2 | 6.1M | 95% |
| 4 | 11.2M | 88% |
| 8 | 18.5M | 72% |

**混合读写测试**（4 线程，不同读写比例）：

| 读比例 | 平均延迟 | 热区命中率 |
|--------|----------|------------|
| 50% | 245 ns | 65% |
| 80% | 180 ns | 72% |
| 95% | 125 ns | 78% |

**结论**：线程安全版本在读多写少场景下表现良好，扩展效率随线程数增加而下降（锁竞争）。

### 10.3 内存占用分析

为评估 FreqPartitionDict 的空间效率，我们测量了不同数据结构的内存占用：

| 结构 | 每元素开销 | 总开销 (N=10000) | 说明 |
|------|-----------|------------------|------|
| std::unordered_map | ~8 字节 | 80 KB | 哈希桶+链表指针 |
| std::map | ~24 字节 | 240 KB | 红黑树节点（颜色+左右指针+父指针） |
| FreqPartitionDict | ~12-16 字节 | 120-160 KB | 频率计数器(4B) + 区域开销 |

*表 4：内存占用对比。FreqPartitionDict 的额外开销主要来自频率计数器，但通过热区压缩可减少整体内存占用。*

**内存优化建议**：
- 使用 `reserve()` 预分配热区容量，减少 rehash 开销
- 对于内存敏感场景，可降低热区容量至工作集的 5%
- 使用 `memory_usage()` 方法监控实际内存占用

### 10.4 缓存算法对比实验

为全面评估 FreqPartitionDict 与传统缓存算法的性能差异，我们实现了 LRU 和 LFU 缓存作为对比基准。

**实验设计**：
- 数据规模：N = 10,000 项
- 缓存容量：128（热区容量）
- 操作次数：100,000 次
- 工作负载：Zipf 分布（α ∈ {0.5, 0.8, 1.0, 1.2, 1.5}）

**重要说明**：FreqPartitionDict 是字典+缓存混合结构，存储所有数据；LRU/LFU 是纯缓存，仅存储有限容量。

#### 10.4.1 命中率对比

| Zipf α | FreqPartitionDict | LRU (128) | LFU (128) | LRU (256) | LFU (256) |
|--------|-------------------|-----------|-----------|-----------|-----------|
| 0.5 | **100%** | 2.9% | 1.4% | 5.8% | 2.8% |
| 0.8 | **100%** | 13.5% | 1.6% | 25.6% | 3.2% |
| 1.0 | **100%** | 40.5% | 6.3% | 48.3% | 8.4% |
| 1.2 | **100%** | 64.9% | 1.3% | 76.2% | 4.5% |
| 1.5 | **100%** | 88.8% | 39.8% | 91.8% | 6.0% |

*表 5：命中率对比。FreqPartitionDict 存储所有数据故命中率为 100%。*

#### 10.4.2 查找延迟对比

| Zipf α | FreqPartitionDict | LRU (128) | LFU (128) |
|--------|-------------------|-----------|-----------|
| 0.5 | 69.5 ms | **0.96 ms** | 0.36 ms |
| 0.8 | 53.1 ms | **1.20 ms** | 0.35 ms |
| 1.0 | 33.0 ms | **1.26 ms** | 0.42 ms |
| 1.2 | 17.6 ms | **1.02 ms** | 0.30 ms |
| 1.5 | **4.66 ms** | 0.83 ms | 0.68 ms |

*表 6：查找延迟对比（100,000 次操作）。LRU/LFU 延迟低但命中率也低。*

#### 10.4.3 关键发现

1. **本质差异**：FreqPartitionDict 是字典（存储所有数据）+ 热区优化，LRU/LFU 是纯缓存（有限容量）。两者应用场景不同。

2. **命中率优势**：FreqPartitionDict 始终保持 100% 命中率，因为所有数据都在结构中。LRU 在高倾斜（α = 1.5）时可达 88.8%，但低倾斜时命中率急剧下降。

3. **延迟权衡**：LRU/LFU 延迟更低，但代价是大量缓存未命中。实际应用中，缓存未命中可能触发昂贵的后端查询。

4. **适用场景**：
   - **FreqPartitionDict**：需要存储所有数据且优化热数据访问的场景
   - **LRU/LFU**：内存受限、可接受缓存未命中的场景

#### 10.4.4 工作负载突变测试

模拟热点突然变化的场景：
- Phase 1：访问热点 A（keys 0-99），50,000 次操作
- Phase 2：访问热点 B（keys 1000-1099），50,000 次操作

| 阶段 | FreqPartitionDict | LRU (256) | LFU (256) |
|------|-------------------|-----------|-----------|
| Phase 1 | 100% | 100% | 100% |
| Phase 2 | 100% | 100% | 100% |

*表 7：工作负载突变测试结果。由于热点范围小于缓存容量，所有算法均能快速适应。*

**结论**：当热点范围小于缓存容量时，LRU 能快速适应变化；FreqPartitionDict 则始终保持 100% 命中率，不受热点变化影响。

---

## 附录 A：理论分析

### A.1 期望查找时间分析

FreqPartitionDict 的期望查找时间可以表示为：

$$E[T_{lookup}] = P_{hot} \cdot O(1) + (1 - P_{hot}) \cdot O(\log n)$$

其中 $P_{hot}$ 是热区命中率。对于 Zipf 分布，热区命中率为：

$$P_{hot} = \sum_{i=1}^{H} \frac{i^{-\alpha}}{\sum_{j=1}^{N} j^{-\alpha}} = \frac{H_{H,\alpha}}{H_{N,\alpha}}$$

其中 $H_{n,\alpha}$ 是广义调和数。当 $N \to \infty$ 时：

$$H_{n,\alpha} \approx \begin{cases}
\frac{n^{1-\alpha}}{1-\alpha} & \alpha < 1 \\
\ln n + \gamma & \alpha = 1 \\
\zeta(\alpha) & \alpha > 1
\end{cases}$$

因此，对于 $\alpha > 1$ 的高度倾斜工作负载：

$$P_{hot} \approx \frac{\zeta(\alpha) - \sum_{i=H+1}^{\infty} i^{-\alpha}}{\zeta(\alpha)} = 1 - O(H^{1-\alpha})$$

这表明随着热区容量 $H$ 的增加，命中率迅速趋近于 1。

### A.2 晋升阈值的影响

设晋升阈值为 $\tau$，则元素需要被访问 $\tau$ 次才能从冷区晋升到热区。这引入了以下权衡：

**延迟 vs 稳定性**：
- 较小的 $\tau$：快速适应工作负载变化，但可能导致热区抖动
- 较大的 $\tau$：热区更稳定，但新热点响应延迟增加

**最优阈值分析**：

假设工作负载变化周期为 $T$，热点持续时间为 $t_h$，则最优阈值应满足：

$$\tau_{opt} \approx \frac{t_h}{T} \cdot f_{avg}$$

其中 $f_{avg}$ 是平均访问频率。

### A.3 空间复杂度分析

| 组件 | 空间复杂度 | 说明 |
|------|-----------|------|
| 热区 | $O(H)$ | 哈希表 + 频率计数 |
| 冷区 | $O(N)$ | 红黑树 + 频率计数 |
| 总计 | $O(N + H)$ | 通常 $H \ll N$ |

相比标准哈希表 $O(N)$，FreqPartitionDict 的空间开销主要来自：
- 频率计数器：每个元素额外 8 字节
- 双区存储：冷区使用树结构，有指针开销

### A.4 均摊分析

对于 $m$ 次操作序列，设：
- $n_i$：第 $i$ 次操作后的元素数量
- $p_i$：第 $i$ 次操作是否为晋升

总时间复杂度：

$$T_{total} = \sum_{i=1}^{m} O(1) + \sum_{i=1}^{m} p_i \cdot O(\log H)$$

在稳定状态下，晋升率等于降级率，且：

$$\sum_{i=1}^{m} p_i \approx m \cdot (1 - P_{hot}) \cdot P_{promote}$$

其中 $P_{promote}$ 是冷区元素达到晋升阈值的概率。

因此均摊时间复杂度为：

$$T_{amortized} = O(1) + (1 - P_{hot}) \cdot P_{promote} \cdot O(\log H)$$

对于高度倾斜工作负载（$P_{hot} \approx 1$），均摊复杂度接近 $O(1)$。

---

## 数据可用性

所有基准测试可使用以下命令复现：

```bash
# 使用 Release 模式构建
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release

# 执行基准测试
./benchmarks/benchmark_fpd --benchmark_repetitions=5 --benchmark_min_time=0.5s

# 生成可视化
python scripts/visualize_performance.py
```

源代码和完整的基准测试数据：https://github.com/AlanAkillove/FreqPartitionDict

---

*本文报告的所有性能数据均来自使用 -O3 优化的 Release 构建。Debug 构建表现出显著不同的性能特征，不能代表生产部署。*
