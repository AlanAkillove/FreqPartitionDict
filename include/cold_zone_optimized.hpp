#pragma once

#include <map>
#include <utility>
#include <memory>

namespace fpd {

// 优化版 ColdZone：使用自定义分配器减少内存碎片
template<typename K, typename V, typename Allocator = std::allocator<std::pair<const K, std::pair<V, size_t>>>>
class ColdZoneOptimized {
private:
    using ValueType = std::pair<V, size_t>;
    using MapType = std::map<K, ValueType, std::less<K>, Allocator>;
    MapType data_;

public:
    bool contains(const K& key) const {
        return data_.find(key) != data_.end();
    }

    V& at(const K& key) {
        return data_.at(key).first;
    }

    const V& at(const K& key) const {
        return data_.at(key).first;
    }

    size_t get_freq(const K& key) const {
        return data_.at(key).second;
    }

    void insert(const K& key, const V& value, size_t freq = 0) {
        data_[key] = {value, freq};
    }

    void increase_freq(const K& key) {
        auto it = data_.find(key);
        if (it != data_.end()) {
            ++it->second.second;
        }
    }

    bool should_promote(const K& key, size_t threshold) const {
        auto it = data_.find(key);
        if (it == data_.end()) {
            return false;
        }
        return it->second.second >= threshold;
    }

    void erase(const K& key) {
        data_.erase(key);
    }

    size_t size() const {
        return data_.size();
    }

    void clear() {
        data_.clear();
    }

    bool empty() const {
        return data_.empty();
    }

    typename MapType::const_iterator begin() const {
        return data_.begin();
    }

    typename MapType::const_iterator end() const {
        return data_.end();
    }

    // 批量插入优化
    template<typename Iterator>
    void insert_batch(Iterator begin, Iterator end) {
        for (auto it = begin; it != end; ++it) {
            data_.insert(*it);
        }
    }

    // 预分配空间
    void reserve(size_t n) {
        // map 不支持 reserve，但可以通过提示优化
        // 这里仅作为接口保留
    }
};

} // namespace fpd
