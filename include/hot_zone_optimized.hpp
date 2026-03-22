#pragma once

#include <unordered_map>
#include <queue>
#include <stdexcept>
#include <utility>
#include <functional>

namespace fpd {

// 优化版 HotZone：使用最小堆实现 O(log H) 淘汰
template<typename K, typename V>
class HotZoneOptimized {
private:
    struct Entry {
        V value;
        size_t freq;
        // 用于延迟删除的标记
        bool valid = true;
    };

    std::unordered_map<K, Entry> data_;
    
    // 最小堆：(频率, 键)
    using HeapElement = std::pair<size_t, K>;
    std::priority_queue<
        HeapElement,
        std::vector<HeapElement>,
        std::greater<HeapElement>
    > min_heap_;
    
    size_t capacity_;

    // 清理堆顶无效元素
    void cleanup_heap() {
        while (!min_heap_.empty()) {
            auto [freq, key] = min_heap_.top();
            auto it = data_.find(key);
            if (it != data_.end() && it->second.valid && it->second.freq == freq) {
                break;
            }
            min_heap_.pop();
        }
    }

public:
    explicit HotZoneOptimized(size_t capacity) : capacity_(capacity) {}

    bool contains(const K& key) const {
        auto it = data_.find(key);
        return it != data_.end() && it->second.valid;
    }

    V& at(const K& key) {
        return data_.at(key).value;
    }

    const V& at(const K& key) const {
        return data_.at(key).value;
    }

    size_t get_freq(const K& key) const {
        return data_.at(key).freq;
    }

    void insert(const K& key, const V& value, size_t freq = 0) {
        data_[key] = {value, freq, true};
        min_heap_.push({freq, key});
    }

    void increase_freq(const K& key) {
        auto it = data_.find(key);
        if (it != data_.end() && it->second.valid) {
            it->second.freq++;
            // 将新频率推入堆（延迟删除旧条目）
            min_heap_.push({it->second.freq, key});
        }
    }

    size_t size() const {
        size_t count = 0;
        for (const auto& [k, v] : data_) {
            if (v.valid) count++;
        }
        return count;
    }

    bool is_full() const {
        return size() >= capacity_;
    }

    size_t capacity() const {
        return capacity_;
    }

    // O(log H) 获取频率最小的键
    std::pair<K, size_t> get_min_freq_key() {
        cleanup_heap();
        if (min_heap_.empty()) {
            throw std::logic_error("HotZoneOptimized is empty");
        }
        auto [freq, key] = min_heap_.top();
        return {key, freq};
    }

    void erase(const K& key) {
        auto it = data_.find(key);
        if (it != data_.end()) {
            it->second.valid = false;  // 标记为无效（延迟删除）
        }
    }

    void clear() {
        data_.clear();
        while (!min_heap_.empty()) min_heap_.pop();
    }

    bool empty() const {
        return size() == 0;
    }

    // 获取实际存储大小（包括无效标记的）
    size_t raw_size() const {
        return data_.size();
    }

    // 压缩：移除无效条目
    void compact() {
        for (auto it = data_.begin(); it != data_.end();) {
            if (!it->second.valid) {
                it = data_.erase(it);
            } else {
                ++it;
            }
        }
        // 重建堆
        std::priority_queue<HeapElement, std::vector<HeapElement>, std::greater<HeapElement>> new_heap;
        for (const auto& [key, entry] : data_) {
            if (entry.valid) {
                new_heap.push({entry.freq, key});
            }
        }
        min_heap_ = std::move(new_heap);
    }
};

} // namespace fpd
