#pragma once

#include <unordered_map>
#include <stdexcept>
#include <utility>

namespace fpd {

/**
 * @brief Hot Zone - High-frequency data storage / 热区 - 高频数据存储
 * 
 * Stores frequently accessed items using std::unordered_map for O(1) lookup.
 * 使用 std::unordered_map 存储高频访问项，实现 O(1) 查找。
 */
template<typename K, typename V>
class HotZone {
private:
    /**
     * @brief Internal entry structure / 内部条目结构
     */
    struct Entry {
        V value;           // Stored value / 存储的值
        size_t freq;       // Access frequency / 访问频率
    };

    std::unordered_map<K, Entry> data_;    // Hash table storage / 哈希表存储
    size_t capacity_;                       // Maximum capacity / 最大容量

public:
    /**
     * @brief Constructor / 构造函数
     * @param capacity Maximum number of items / 最大项目数
     */
    explicit HotZone(size_t capacity) : capacity_(capacity) {}

    /**
     * @brief Check if key exists / 检查键是否存在
     * @param key Key to check / 要检查的键
     * @return bool True if exists / 存在返回 true
     */
    bool contains(const K& key) const {
        return data_.find(key) != data_.end();
    }

    /**
     * @brief Get value reference / 获取值引用
     * @param key Key to lookup / 要查找的键
     * @return V& Reference to value / 值的引用
     */
    V& at(const K& key) {
        return data_.at(key).value;
    }

    /**
     * @brief Get const value reference / 获取常量值引用
     * @param key Key to lookup / 要查找的键
     * @return const V& Const reference to value / 值的常量引用
     */
    const V& at(const K& key) const {
        return data_.at(key).value;
    }

    /**
     * @brief Get access frequency / 获取访问频率
     * @param key Key to query / 要查询的键
     * @return size_t Access frequency / 访问频率
     */
    size_t get_freq(const K& key) const {
        return data_.at(key).freq;
    }

    /**
     * @brief Insert or update entry / 插入或更新条目
     * @param key Key to insert / 要插入的键
     * @param value Value to insert / 要插入的值
     * @param freq Initial frequency (default: 0) / 初始频率（默认：0）
     */
    void insert(const K& key, const V& value, size_t freq = 0) {
        data_[key] = {value, freq};
    }

    /**
     * @brief Increment access frequency / 增加访问频率
     * @param key Key to update / 要更新的键
     */
    void increase_freq(const K& key) {
        auto it = data_.find(key);
        if (it != data_.end()) {
            ++it->second.freq;
        }
    }

    /**
     * @brief Get current size / 获取当前大小
     * @return size_t Number of items / 项目数量
     */
    size_t size() const {
        return data_.size();
    }

    /**
     * @brief Check if at capacity / 检查是否已满
     * @return bool True if full / 已满返回 true
     */
    bool is_full() const {
        return data_.size() >= capacity_;
    }

    /**
     * @brief Get capacity / 获取容量
     * @return size_t Maximum capacity / 最大容量
     */
    size_t capacity() const {
        return capacity_;
    }

    /**
     * @brief Find key with minimum frequency / 查找频率最低的键
     * @return std::pair<K, size_t> Key and its frequency / 键及其频率
     * @throws std::logic_error If empty / 如果为空则抛出异常
     * 
     * Time complexity: O(H) where H is hot zone size
     * 时间复杂度：O(H)，H 为热区大小
     */
    std::pair<K, size_t> get_min_freq_key() const {
        if (data_.empty()) {
            throw std::logic_error("HotZone is empty / 热区为空");
        }

        // Linear scan to find minimum / 线性扫描查找最小值
        auto min_it = data_.begin();
        for (auto it = data_.begin(); it != data_.end(); ++it) {
            if (it->second.freq < min_it->second.freq) {
                min_it = it;
            }
        }
        return {min_it->first, min_it->second.freq};
    }

    /**
     * @brief Remove entry by key / 按键移除条目
     * @param key Key to remove / 要移除的键
     */
    void erase(const K& key) {
        data_.erase(key);
    }

    /**
     * @brief Clear all entries / 清空所有条目
     */
    void clear() {
        data_.clear();
    }

    /**
     * @brief Check if empty / 检查是否为空
     * @return bool True if empty / 为空返回 true
     */
    bool empty() const {
        return data_.empty();
    }

    // Iterator support / 迭代器支持
    typename std::unordered_map<K, Entry>::const_iterator begin() const {
        return data_.begin();
    }

    typename std::unordered_map<K, Entry>::const_iterator end() const {
        return data_.end();
    }
};

} // namespace fpd
