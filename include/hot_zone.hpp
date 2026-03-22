#pragma once

#include <unordered_map>
#include <stdexcept>
#include <utility>

namespace fpd {

template<typename K, typename V>
class HotZone {
private:
    struct Entry {
        V value;
        size_t freq;
    };

    std::unordered_map<K, Entry> data_;
    size_t capacity_;

public:
    explicit HotZone(size_t capacity) : capacity_(capacity) {}

    bool contains(const K& key) const {
        return data_.find(key) != data_.end();
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
        data_[key] = {value, freq};
    }

    void increase_freq(const K& key) {
        auto it = data_.find(key);
        if (it != data_.end()) {
            ++it->second.freq;
        }
    }

    size_t size() const {
        return data_.size();
    }

    bool is_full() const {
        return data_.size() >= capacity_;
    }

    size_t capacity() const {
        return capacity_;
    }

    std::pair<K, size_t> get_min_freq_key() const {
        if (data_.empty()) {
            throw std::logic_error("HotZone is empty");
        }

        auto min_it = data_.begin();
        for (auto it = data_.begin(); it != data_.end(); ++it) {
            if (it->second.freq < min_it->second.freq) {
                min_it = it;
            }
        }
        return {min_it->first, min_it->second.freq};
    }

    void erase(const K& key) {
        data_.erase(key);
    }

    void clear() {
        data_.clear();
    }

    bool empty() const {
        return data_.empty();
    }

    typename std::unordered_map<K, Entry>::const_iterator begin() const {
        return data_.begin();
    }

    typename std::unordered_map<K, Entry>::const_iterator end() const {
        return data_.end();
    }
};

} // namespace fpd
