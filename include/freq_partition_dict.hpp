#pragma once

#include "hot_zone.hpp"
#include "cold_zone.hpp"
#include <optional>
#include <vector>
#include <cstddef>

namespace fpd {

/**
 * @brief Frequency-Partitioned Dictionary / 频率分区字典
 * 
 * A hybrid dictionary that partitions data into hot and cold zones based on access frequency.
 * 一种基于访问频率将数据分区为热区和冷区的混合字典。
 * 
 * Hot Zone: std::unordered_map for O(1) lookup of frequently accessed items
 * 热区：使用 std::unordered_map 实现高频项的 O(1) 查找
 * 
 * Cold Zone: std::map for O(log n) lookup of infrequently accessed items
 * 冷区：使用 std::map 实现低频项的 O(log n) 查找
 */
template<typename K, typename V>
class FreqPartitionDict {
private:
    HotZone<K, V> hot_zone_;           // Hot zone - high frequency items / 热区 - 高频项
    ColdZone<K, V> cold_zone_;         // Cold zone - low frequency items / 冷区 - 低频项
    size_t hot_capacity_;              // Maximum hot zone capacity / 热区最大容量
    size_t promote_threshold_;         // Access threshold for promotion / 晋升访问阈值

    // Statistics / 统计信息
    size_t hot_hits_ = 0;              // Hot zone hit count / 热区命中次数
    size_t cold_hits_ = 0;             // Cold zone hit count / 冷区命中次数
    size_t misses_ = 0;                // Miss count / 未命中次数
    size_t promotions_ = 0;            // Promotion count / 晋升次数
    size_t demotions_ = 0;             // Demotion count / 降级次数

    /**
     * @brief Promote item from cold zone to hot zone
     *        将项从冷区晋升到热区
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
            auto [min_key, min_freq] = hot_zone_.get_min_freq_key();
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
    FreqPartitionDict(size_t hot_capacity = 128, size_t promote_threshold = 3)
        : hot_zone_(hot_capacity)
        , hot_capacity_(hot_capacity)
        , promote_threshold_(promote_threshold) {}

    /**
     * @brief Lookup item by key / 按键查找项
     * @param key Key to lookup / 要查找的键
     * @return std::optional<V> Value if found, nullopt otherwise / 找到返回值，否则返回 nullopt
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
    }

    struct MemoryStats {
        size_t hot_zone_bytes;
        size_t cold_zone_bytes;
        size_t overhead_bytes;
        size_t total_bytes() const { 
            return hot_zone_bytes + cold_zone_bytes + overhead_bytes; 
        }
    };

    MemoryStats memory_usage() const {
        MemoryStats stats;
        stats.hot_zone_bytes = hot_zone_.size() * (sizeof(K) + sizeof(V) + sizeof(size_t) + 8);
        stats.cold_zone_bytes = cold_zone_.size() * (sizeof(K) + sizeof(V) + sizeof(size_t) + 24);
        stats.overhead_bytes = sizeof(hot_capacity_) + sizeof(promote_threshold_) +
                               sizeof(hot_hits_) + sizeof(cold_hits_) +
                               sizeof(misses_) + sizeof(promotions_) + sizeof(demotions_);
        return stats;
    }

    void reserve(size_t hot_capacity) {
        hot_zone_.reserve(hot_capacity);
    }

    template<typename Iterator>
    void insert_batch(Iterator begin, Iterator end) {
        for (auto it = begin; it != end; ++it) {
            insert(it->first, it->second);
        }
    }

    template<typename Container>
    void get_batch(const Container& keys, std::vector<std::optional<V>>& results) {
        results.clear();
        results.reserve(keys.size());
        for (const auto& key : keys) {
            results.push_back(get(key));
        }
    }

    template<typename Iterator>
    void erase_batch(Iterator begin, Iterator end) {
        for (auto it = begin; it != end; ++it) {
            erase(*it);
        }
    }

    template<typename Iterator>
    std::vector<std::pair<K, V>> get_hot_items(Iterator begin, Iterator end) const {
        std::vector<std::pair<K, V>> result;
        for (auto it = begin; it != end; ++it) {
            if (hot_zone_.contains(*it)) {
                auto val = hot_zone_.get(*it);
                if (val) {
                    result.emplace_back(*it, *val);
                }
            }
        }
        return result;
    }

    std::vector<std::pair<K, V>> get_all_hot_items() const {
        std::vector<std::pair<K, V>> result;
        result.reserve(hot_zone_.size());
        for (auto it = hot_zone_.begin(); it != hot_zone_.end(); ++it) {
            result.emplace_back(it->first, it->second.value);
        }
        return result;
    }

    // Iterator support / 迭代器支持
    /**
     * @brief Iterator for traversing all entries / 遍历所有条目的迭代器
     * 
     * Iterates over hot zone first, then cold zone.
     * 先遍历热区，再遍历冷区。
     */
    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const K, V>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

    private:
        using HotIterator = typename HotZone<K, V>::const_iterator;
        using ColdIterator = typename ColdZone<K, V>::const_iterator;

        const FreqPartitionDict* dict_;
        HotIterator hot_it_;
        HotIterator hot_end_;
        ColdIterator cold_it_;
        ColdIterator cold_end_;
        bool in_hot_zone_;

    public:
        iterator(const FreqPartitionDict* dict, HotIterator hot_it, HotIterator hot_end,
                 ColdIterator cold_it, ColdIterator cold_end, bool in_hot)
            : dict_(dict), hot_it_(hot_it), hot_end_(hot_end),
              cold_it_(cold_it), cold_end_(cold_end), in_hot_zone_(in_hot) {}

        std::pair<K, V> operator*() const {
            if (in_hot_zone_) {
                return {hot_it_->first, hot_it_->second.value};
            } else {
                return {cold_it_->first, cold_it_->second.first};
            }
        }

        iterator& operator++() {
            if (in_hot_zone_) {
                ++hot_it_;
                if (hot_it_ == hot_end_) {
                    in_hot_zone_ = false;
                }
            } else {
                ++cold_it_;
            }
            return *this;
        }

        bool operator!=(const iterator& other) const {
            if (in_hot_zone_ != other.in_hot_zone_) return true;
            if (in_hot_zone_) return hot_it_ != other.hot_it_;
            return cold_it_ != other.cold_it_;
        }

        bool operator==(const iterator& other) const {
            return !(*this != other);
        }
    };

    iterator begin() const {
        return iterator(this, hot_zone_.begin(), hot_zone_.end(),
                       cold_zone_.begin(), cold_zone_.end(),
                       !hot_zone_.empty());
    }

    iterator end() const {
        return iterator(this, hot_zone_.end(), hot_zone_.end(),
                       cold_zone_.end(), cold_zone_.end(), false);
    }
};

} // namespace fpd
