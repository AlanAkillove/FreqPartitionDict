#pragma once

#include "freq_partition_dict.hpp"
#include <shared_mutex>
#include <mutex>

namespace fpd {

// 线程安全版本：使用读写锁
template<typename K, typename V>
class FreqPartitionDictThreadSafe {
private:
    FreqPartitionDict<K, V> dict_;
    mutable std::shared_mutex mutex_;

public:
    FreqPartitionDictThreadSafe(size_t hot_capacity = 128, size_t promote_threshold = 3)
        : dict_(hot_capacity, promote_threshold) {}

    std::optional<V> get(const K& key) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        return dict_.get(key);
    }

    // 只读操作使用共享锁
    bool contains(const K& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return dict_.contains(key);
    }

    void insert(const K& key, const V& value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        dict_.insert(key, value);
    }

    bool erase(const K& key) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        return dict_.erase(key);
    }

    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        dict_.clear();
    }

    // 只读操作使用共享锁
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return dict_.size();
    }

    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return dict_.empty();
    }

    size_t hot_size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return dict_.hot_size();
    }

    size_t cold_size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return dict_.cold_size();
    }

    // 批量操作：减少锁开销
    template<typename Iterator>
    void insert_batch(Iterator begin, Iterator end) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        for (auto it = begin; it != end; ++it) {
            dict_.insert(it->first, it->second);
        }
    }

    // 批量读取
    template<typename Container>
    void get_batch(const Container& keys, std::vector<std::optional<V>>& results) {
        results.clear();
        results.reserve(keys.size());
        std::shared_lock<std::shared_mutex> lock(mutex_);
        for (const auto& key : keys) {
            results.push_back(dict_.get(key));
        }
    }

    // 统计信息
    double hot_hit_rate() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return dict_.hot_hit_rate();
    }

    double total_hit_rate() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return dict_.total_hit_rate();
    }

    void reset_stats() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        dict_.reset_stats();
    }
};

} // namespace fpd
