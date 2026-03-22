#pragma once

#include "hot_zone.hpp"
#include "cold_zone.hpp"
#include <optional>
#include <cstddef>

namespace fpd {

template<typename K, typename V>
class FreqPartitionDict {
private:
    HotZone<K, V> hot_zone_;
    ColdZone<K, V> cold_zone_;
    size_t hot_capacity_;
    size_t promote_threshold_;

    size_t hot_hits_ = 0;
    size_t cold_hits_ = 0;
    size_t misses_ = 0;
    size_t promotions_ = 0;
    size_t demotions_ = 0;

    void promote(const K& key) {
        // 如果热区容量为0，不执行晋升
        if (hot_capacity_ == 0) {
            return;
        }

        V value = cold_zone_.at(key);
        size_t freq = cold_zone_.get_freq(key);
        cold_zone_.erase(key);

        if (hot_zone_.is_full()) {
            auto [min_key, min_freq] = hot_zone_.get_min_freq_key();
            V min_value = hot_zone_.at(min_key);
            hot_zone_.erase(min_key);
            cold_zone_.insert(min_key, min_value, min_freq);
            ++demotions_;
        }

        hot_zone_.insert(key, value, freq);
        ++promotions_;
    }

public:
    FreqPartitionDict(size_t hot_capacity = 128, size_t promote_threshold = 3)
        : hot_zone_(hot_capacity)
        , hot_capacity_(hot_capacity)
        , promote_threshold_(promote_threshold) {}

    std::optional<V> get(const K& key) {
        if (hot_zone_.contains(key)) {
            hot_zone_.increase_freq(key);
            ++hot_hits_;
            return hot_zone_.at(key);
        }

        if (cold_zone_.contains(key)) {
            cold_zone_.increase_freq(key);
            ++cold_hits_;

            if (cold_zone_.should_promote(key, promote_threshold_)) {
                promote(key);
                // 晋升后从热区返回值（如果热区容量为0，则仍在冷区）
                if (hot_capacity_ > 0) {
                    return hot_zone_.at(key);
                }
            }
            return cold_zone_.at(key);
        }

        ++misses_;
        return std::nullopt;
    }

    bool contains(const K& key) const {
        return hot_zone_.contains(key) || cold_zone_.contains(key);
    }

    void insert(const K& key, const V& value) {
        if (hot_zone_.contains(key)) {
            hot_zone_.at(key) = value;
        } else if (cold_zone_.contains(key)) {
            cold_zone_.at(key) = value;
        } else {
            cold_zone_.insert(key, value, 0);
        }
    }

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

    void clear() {
        hot_zone_.clear();
        cold_zone_.clear();
        hot_hits_ = 0;
        cold_hits_ = 0;
        misses_ = 0;
        promotions_ = 0;
        demotions_ = 0;
    }

    size_t size() const {
        return hot_zone_.size() + cold_zone_.size();
    }

    bool empty() const {
        return hot_zone_.empty() && cold_zone_.empty();
    }

    size_t hot_size() const {
        return hot_zone_.size();
    }

    size_t cold_size() const {
        return cold_zone_.size();
    }

    size_t hot_capacity() const {
        return hot_capacity_;
    }

    size_t promote_threshold() const {
        return promote_threshold_;
    }

    void set_promote_threshold(size_t threshold) {
        promote_threshold_ = threshold;
    }

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

    const HotZone<K, V>& hot_zone() const { return hot_zone_; }
    const ColdZone<K, V>& cold_zone() const { return cold_zone_; }
};

} // namespace fpd
