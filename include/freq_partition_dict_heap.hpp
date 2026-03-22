#pragma once

#include "hot_zone_heap.hpp"
#include "cold_zone.hpp"
#include <optional>
#include <cstddef>

namespace fpd {

/**
 * @brief Optimized FreqPartitionDict with Heap-based Hot Zone
 *        基于堆优化热区的频率分区字典
 * 
 * This version uses a min-heap for the hot zone to achieve O(log H) eviction
 * instead of O(H) linear scanning.
 * 此版本使用最小堆实现热区，达到 O(log H) 淘汰复杂度，而非 O(H) 线性扫描。
 * 
 * Performance comparison with base version:
 * - Lookup: O(1) same / 相同
 * - Insert: O(log H) vs O(1) / 稍慢
 * - Eviction: O(log H) vs O(H) / 显著更快（H > 64 时）
 * - Increase frequency: O(log H) vs O(1) / 稍慢
 * 
 * Recommended when H > 64 for better overall performance.
 * 建议当 H > 64 时使用以获得更好的整体性能。
 */
template<typename K, typename V>
class FreqPartitionDictHeap {
private:
    HotZoneHeap<K, V> hot_zone_;           // Hot zone with heap / 堆优化热区
    ColdZone<K, V> cold_zone_;             // Cold zone / 冷区
    size_t hot_capacity_;                  // Maximum hot zone capacity / 热区最大容量
    size_t promote_threshold_;             // Access threshold for promotion / 晋升访问阈值

    // Statistics / 统计信息
    size_t hot_hits_ = 0;                  // Hot zone hit count / 热区命中次数
    size_t cold_hits_ = 0;                 // Cold zone hit count / 冷区命中次数
    size_t misses_ = 0;                    // Miss count / 未命中次数
    size_t promotions_ = 0;                // Promotion count / 晋升次数
    size_t demotions_ = 0;                 // Demotion count / 降级次数

    /**
     * @brief Promote item from cold zone to hot zone
     *        将项从冷区晋升到热区
     * 
     * Time complexity: O(log H) for eviction
     * 时间复杂度：O(log H) 用于淘汰
     */
    void promote(const K& key) {
        // Skip if hot zone capacity is zero / 如果热区容量为0，跳过晋升
        if (hot_capacity_ == 0) {
            return;
        }

        // Get value and frequency from cold zone / 从冷区获取值和频率
        V value = cold_zone_.at(key);
        size_t freq = cold_zone_.get_freq(key);
        cold_zone_.erase(key);

        // Evict lowest frequency item if hot zone is full / 如果热区已满，淘汰频率最低的项
        if (hot_zone_.is_full()) {
            auto [min_key, min_freq] = hot_zone_.pop_min();
            V min_value = hot_zone_.at(min_key);
            hot_zone_.erase(min_key);
            cold_zone_.insert(min_key, min_value, min_freq);
            ++demotions_;
        }

        // Insert into hot zone / 插入热区
        hot_zone_.insert(key, value, freq);
        ++promotions_;
    }

public:
    /**
     * @brief Constructor / 构造函数
     * @param hot_capacity Hot zone capacity (default: 128) / 热区容量（默认：128）
     * @param promote_threshold Access threshold for promotion (default: 3) / 晋升访问阈值（默认：3）
     */
    FreqPartitionDictHeap(size_t hot_capacity = 128, size_t promote_threshold = 3)
        : hot_zone_(hot_capacity)
        , hot_capacity_(hot_capacity)
        , promote_threshold_(promote_threshold) {}

    /**
     * @brief Lookup item by key / 按键查找项
     * @param key Key to lookup / 要查找的键
     * @return std::optional<V> Value if found, nullopt otherwise / 找到返回值，否则返回 nullopt
     * 
     * Time complexity:
     * - Hot hit: O(1)
     * - Cold hit: O(log n) lookup + O(log H) frequency update
     * - Miss: O(1) + O(log n)
     */
    std::optional<V> get(const K& key) {
        // Check hot zone first / 首先检查热区
        if (hot_zone_.contains(key)) {
            hot_zone_.increase_freq(key);
            ++hot_hits_;
            return hot_zone_.at(key);
        }

        // Check cold zone / 检查冷区
        if (cold_zone_.contains(key)) {
            cold_zone_.increase_freq(key);
            ++cold_hits_;

            // Promote if threshold reached / 如果达到阈值则晋升
            if (cold_zone_.should_promote(key, promote_threshold_)) {
                promote(key);
                // Return from hot zone after promotion (if capacity > 0)
                // 晋升后从热区返回（如果容量 > 0）
                if (hot_capacity_ > 0) {
                    return hot_zone_.at(key);
                }
            }
            return cold_zone_.at(key);
        }

        // Not found / 未找到
        ++misses_;
        return std::nullopt;
    }

    /**
     * @brief Check if key exists / 检查键是否存在
     * @param key Key to check / 要检查的键
     * @return bool True if exists / 存在返回 true
     */
    bool contains(const K& key) const {
        return hot_zone_.contains(key) || cold_zone_.contains(key);
    }

    /**
     * @brief Insert or update key-value pair / 插入或更新键值对
     * @param key Key to insert / 要插入的键
     * @param value Value to insert / 要插入的值
     * 
     * Time complexity: O(log H) for hot zone, O(log n) for cold zone
     */
    void insert(const K& key, const V& value) {
        if (hot_zone_.contains(key)) {
            hot_zone_.at(key) = value;
        } else if (cold_zone_.contains(key)) {
            cold_zone_.at(key) = value;
        } else {
            cold_zone_.insert(key, value, 0);
        }
    }

    /**
     * @brief Remove item by key / 按键移除项
     * @param key Key to remove / 要移除的键
     * @return bool True if removed / 成功移除返回 true
     */
    bool erase(const K& key) {
        if (hot_zone_.contains(key)) {
            hot_zone_.erase(key);
            return true;
        }
        if (cold_zone_.contains(key)) {
            cold_zone_.erase(key);
            return true;
        }
        return false;
    }

    /**
     * @brief Clear all data / 清空所有数据
     */
    void clear() {
        hot_zone_.clear();
        cold_zone_.clear();
        reset_stats();
    }

    // Capacity and size / 容量和大小
    size_t size() const { return hot_zone_.size() + cold_zone_.size(); }
    bool empty() const { return hot_zone_.empty() && cold_zone_.empty(); }
    size_t hot_size() const { return hot_zone_.size(); }
    size_t cold_size() const { return cold_zone_.size(); }
    size_t hot_capacity() const { return hot_capacity_; }
    size_t promote_threshold() const { return promote_threshold_; }

    void set_promote_threshold(size_t threshold) { promote_threshold_ = threshold; }

    /**
     * @brief Reserve capacity / 预分配容量
     * @param hot_capacity Hot zone capacity to reserve / 热区预分配容量
     * @param cold_capacity Cold zone capacity to reserve / 冷区预分配容量
     */
    void reserve(size_t hot_capacity, size_t cold_capacity) {
        hot_zone_.reserve(hot_capacity);
        // Note: std::map doesn't have reserve, but we could use a custom allocator
    }

    // Statistics / 统计信息
    double hot_hit_rate() const {
        size_t total = hot_hits_ + cold_hits_ + misses_;
        return total == 0 ? 0.0 : static_cast<double>(hot_hits_) / total;
    }

    double total_hit_rate() const {
        size_t total = hot_hits_ + cold_hits_ + misses_;
        return total == 0 ? 0.0 : static_cast<double>(hot_hits_ + cold_hits_) / total;
    }

    size_t hot_hits() const { return hot_hits_; }
    size_t cold_hits() const { return cold_hits_; }
    size_t misses() const { return misses_; }
    size_t promotions() const { return promotions_; }
    size_t demotions() const { return demotions_; }

    void reset_stats() {
        hot_hits_ = 0;
        cold_hits_ = 0;
        misses_ = 0;
        // Note: promotions_ and demotions_ are cumulative and not reset
    }
};

} // namespace fpd
