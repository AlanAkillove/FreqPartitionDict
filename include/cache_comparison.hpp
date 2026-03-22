#pragma once

#include <unordered_map>
#include <list>
#include <optional>
#include <cstddef>

namespace fpd {

template<typename K, typename V>
class LRUCache {
private:
    size_t capacity_;
    std::list<std::pair<K, V>> order_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> cache_;
    
    size_t hits_ = 0;
    size_t misses_ = 0;

public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            ++misses_;
            return std::nullopt;
        }
        ++hits_;
        order_.splice(order_.begin(), order_, it->second);
        return it->second->second;
    }

    void insert(const K& key, const V& value) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            it->second->second = value;
            order_.splice(order_.begin(), order_, it->second);
            return;
        }
        if (cache_.size() >= capacity_) {
            auto last = order_.back();
            cache_.erase(last.first);
            order_.pop_back();
        }
        order_.emplace_front(key, value);
        cache_[key] = order_.begin();
    }

    bool contains(const K& key) const {
        return cache_.find(key) != cache_.end();
    }

    bool erase(const K& key) {
        auto it = cache_.find(key);
        if (it == cache_.end()) return false;
        order_.erase(it->second);
        cache_.erase(it);
        return true;
    }

    void clear() {
        cache_.clear();
        order_.clear();
        hits_ = 0;
        misses_ = 0;
    }

    size_t size() const { return cache_.size(); }
    bool empty() const { return cache_.empty(); }
    size_t capacity() const { return capacity_; }
    
    double hit_rate() const {
        size_t total = hits_ + misses_;
        return total == 0 ? 0.0 : static_cast<double>(hits_) / total;
    }
    
    size_t hits() const { return hits_; }
    size_t misses() const { return misses_; }
    void reset_stats() { hits_ = 0; misses_ = 0; }
};

template<typename K, typename V>
class LFUCache {
private:
    struct Node {
        V value;
        size_t freq;
    };
    
    size_t capacity_;
    std::unordered_map<K, Node> cache_;
    
    size_t hits_ = 0;
    size_t misses_ = 0;

public:
    explicit LFUCache(size_t capacity) : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            ++misses_;
            return std::nullopt;
        }
        ++hits_;
        ++it->second.freq;
        return it->second.value;
    }

    void insert(const K& key, const V& value) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            it->second.value = value;
            ++it->second.freq;
            return;
        }
        if (cache_.size() >= capacity_) {
            K min_key = cache_.begin()->first;
            size_t min_freq = cache_.begin()->second.freq;
            for (const auto& [k, node] : cache_) {
                if (node.freq < min_freq) {
                    min_freq = node.freq;
                    min_key = k;
                }
            }
            cache_.erase(min_key);
        }
        cache_[key] = {value, 1};
    }

    bool contains(const K& key) const {
        return cache_.find(key) != cache_.end();
    }

    bool erase(const K& key) {
        return cache_.erase(key) > 0;
    }

    void clear() {
        cache_.clear();
        hits_ = 0;
        misses_ = 0;
    }

    size_t size() const { return cache_.size(); }
    bool empty() const { return cache_.empty(); }
    size_t capacity() const { return capacity_; }
    
    double hit_rate() const {
        size_t total = hits_ + misses_;
        return total == 0 ? 0.0 : static_cast<double>(hits_) / total;
    }
    
    size_t hits() const { return hits_; }
    size_t misses() const { return misses_; }
    void reset_stats() { hits_ = 0; misses_ = 0; }
};

} // namespace fpd
