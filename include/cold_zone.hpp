#pragma once

#include <map>
#include <utility>

namespace fpd {

template<typename K, typename V>
class ColdZone {
private:
    std::map<K, std::pair<V, size_t>> data_;

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

    typename std::map<K, std::pair<V, size_t>>::const_iterator begin() const {
        return data_.begin();
    }

    typename std::map<K, std::pair<V, size_t>>::const_iterator end() const {
        return data_.end();
    }

    std::pair<K, V> extract(const K& key) {
        auto it = data_.find(key);
        if (it == data_.end()) {
            throw std::runtime_error("Key not found in ColdZone");
        }
        K k = it->first;
        V v = it->second.first;
        data_.erase(it);
        return {k, v};
    }
};

} // namespace fpd
