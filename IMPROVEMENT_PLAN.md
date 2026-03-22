# FreqPartitionDict 改进规划文档

## 一、改进目标

基于项目评估结果，本规划旨在：
1. 修复现有代码中的关键问题
2. 优化核心算法性能
3. 完善实验验证体系
4. 提升论文学术质量

---

## 二、改进任务清单

### Phase 1: 关键 Bug 修复（高优先级）

#### 任务 1.1: 修复线程安全版本的锁粒度问题
- **文件**: `include/freq_partition_dict_threadsafe.hpp`
- **问题**: `get()` 方法使用了 `unique_lock`（写锁），应该使用 `shared_lock`（读锁）
- **影响**: 并发读性能极差
- **预计时间**: 30 分钟

#### 任务 1.2: 修复统计信息重置不完整
- **文件**: `include/freq_partition_dict.hpp`
- **问题**: `reset_stats()` 未重置 `promotions_` 和 `demotions_`
- **预计时间**: 15 分钟

### Phase 2: 性能优化（高优先级）

#### 任务 2.1: 优化热区淘汰策略
- **文件**: 新建 `include/hot_zone_heap.hpp`
- **目标**: 使用最小堆实现 O(log H) 淘汰
- **当前**: 线性扫描 O(H)
- **预计时间**: 2 小时

#### 任务 2.2: 增加内存预分配接口
- **文件**: `include/freq_partition_dict.hpp`
- **目标**: 提供 `reserve()` 方法减少重新哈希开销
- **预计时间**: 1 小时

### Phase 3: 功能增强（中优先级）

#### 任务 3.1: 增加迭代器支持
- **文件**: `include/freq_partition_dict.hpp`
- **目标**: 支持范围遍历整个字典
- **预计时间**: 2 小时

#### 任务 3.2: 增加批量操作接口
- **文件**: `include/freq_partition_dict.hpp`
- **目标**: 提供 `insert_batch()`, `get_batch()` 方法
- **预计时间**: 1.5 小时

### Phase 4: 实验完善（中优先级）

#### 任务 4.1: 增加真实工作负载测试
- **文件**: 新建 `benchmarks/benchmark_real_workload.cpp`
- **目标**: 模拟数据库查询、Web 访问日志等场景
- **预计时间**: 3 小时

#### 任务 4.2: 增加并发性能测试
- **文件**: 新建 `benchmarks/benchmark_concurrent.cpp`
- **目标**: 测试不同线程数、读写比例下的性能
- **预计时间**: 2 小时

#### 任务 4.3: 增加长期稳定性测试
- **文件**: 新建 `benchmarks/benchmark_stability.cpp`
- **目标**: 测试热点动态变化时的适应性
- **预计时间**: 2 小时

### Phase 5: 论文完善（低优先级）

#### 任务 5.1: 增加理论分析章节
- **文件**: `docs/TECHNICAL_PAPER_CN.md`, `docs/TECHNICAL_PAPER_EN.md`
- **内容**: 期望查找时间数学模型、命中率公式推导
- **预计时间**: 4 小时

#### 任务 5.2: 扩展相关工作对比
- **文件**: `docs/TECHNICAL_PAPER_CN.md`, `docs/TECHNICAL_PAPER_EN.md`
- **内容**: 与 LRU、LFU、2Q、ARC 等算法深度对比
- **预计时间**: 3 小时

---

## 三、详细改进计划

### 3.1 线程安全版本修复

```cpp
// 修复前（错误）
std::optional<V> get(const K& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);  // 写锁！
    return dict_.get(key);
}

// 修复后（正确）
std::optional<V> get(const K& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);  // 读锁
    return dict_.get(key);
}
```

### 3.2 热区最小堆优化

```cpp
// 新设计：使用最小堆 + 哈希表实现 O(log H) 淘汰
template<typename K, typename V>
class HotZoneHeap {
private:
    struct HeapNode {
        K key;
        size_t freq;
        size_t index;  // 在堆中的位置
    };
    
    std::unordered_map<K, V> data_;           // 存储数据
    std::unordered_map<K, size_t> freq_map_;  // 频率映射
    std::vector<HeapNode> min_heap_;          // 最小堆
    
public:
    void insert(const K& key, const V& value, size_t freq);
    std::pair<K, size_t> pop_min();  // O(log H)
    void increase_freq(const K& key);  // O(log H)
};
```

### 3.3 迭代器支持设计

```cpp
// 统一迭代器遍历热区和冷区
template<typename K, typename V>
class FreqPartitionDict {
public:
    class iterator {
        // 同时遍历热区和冷区的迭代器
    };
    
    iterator begin();
    iterator end();
};
```

### 3.4 真实工作负载场景

```cpp
// 场景 1: 数据库查询模拟
void benchmark_database_workload() {
    // 模拟 TPC-C 订单查询模式
    // 80% 查询最近订单，20% 查询历史订单
}

// 场景 2: Web 服务器访问日志
void benchmark_web_server_workload() {
    // 模拟热门页面访问
    // 符合 Zipf 分布的 URL 访问
}

// 场景 3: 社交网络好友查询
void benchmark_social_network_workload() {
    // 模拟活跃用户查询
    // 热点用户动态变化
}
```

---

## 四、实施时间表

| 阶段 | 任务 | 预计时间 | 优先级 |
|------|------|----------|--------|
| Phase 1 | 修复线程安全锁粒度 | 30 分钟 | 高 |
| Phase 1 | 修复统计信息重置 | 15 分钟 | 高 |
| Phase 2 | 热区最小堆优化 | 2 小时 | 高 |
| Phase 2 | 内存预分配接口 | 1 小时 | 高 |
| Phase 3 | 迭代器支持 | 2 小时 | 中 |
| Phase 3 | 批量操作接口 | 1.5 小时 | 中 |
| Phase 4 | 真实工作负载测试 | 3 小时 | 中 |
| Phase 4 | 并发性能测试 | 2 小时 | 中 |
| Phase 4 | 长期稳定性测试 | 2 小时 | 中 |
| Phase 5 | 理论分析章节 | 4 小时 | 低 |
| Phase 5 | 相关工作对比 | 3 小时 | 低 |

**总计**: 约 21.75 小时

---

## 五、验证标准

### 5.1 代码质量
- [ ] 所有单元测试通过
- [ ] 新增代码覆盖率 > 80%
- [ ] 无内存泄漏（通过 valgrind 检测）

### 5.2 性能指标
- [ ] 线程安全版本并发读性能提升 > 10 倍
- [ ] 热区最小堆版本淘汰操作 O(log H) 复杂度验证
- [ ] 真实工作负载下命中率 > 60%

### 5.3 文档完整
- [ ] 所有新增接口有完整注释
- [ ] 论文新增章节通过学术规范检查
- [ ] README 更新使用示例

---

## 六、风险评估

| 风险 | 可能性 | 影响 | 应对措施 |
|------|--------|------|----------|
| 最小堆实现复杂度高 | 中 | 中 | 先实现简化版本，逐步优化 |
| 迭代器实现影响性能 | 低 | 中 | 提供可选的轻量级迭代器 |
| 真实工作负载数据获取困难 | 中 | 低 | 使用合成数据模拟真实特征 |

---

## 七、后续规划

### 7.1 版本发布计划
- **v1.1**: 修复 Bug + 性能优化（Phase 1-2）
- **v1.2**: 功能增强（Phase 3）
- **v2.0**: 实验完善 + 论文发表（Phase 4-5）

### 7.2 潜在研究方向
1. 自适应晋升阈值算法
2. 机器学习预测热点
3. 分布式版本设计
4. 持久化存储支持

---

*文档创建时间: 2026-03-22*
*最后更新: 2026-03-22*
