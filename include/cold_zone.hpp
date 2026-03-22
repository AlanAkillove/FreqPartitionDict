#pragma once

#include <map>
#include <utility>
#include <stdexcept>

namespace fpd {

/**
 * @brief Cold Zone - Low-frequency data storage / 冷区 - 低频数据存储
 * 
 * Stores infrequently accessed items using std::map for O(log n) lookup.
 * Supports ordered iteration and efficient range queries.
 * 使用 std::map 存储低频访问项，实现 O(log n) 查找。
 * 支持有序迭代和高效的范围查询。
 */
template<typename K, typename V>
class ColdZone {
private:
    // Map storage: key -> {value, frequency}
    // 映射存储：键 -> {值, 频率}
    std::map<K, std::pair<V, size_t>> data_;

public:
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
        return data_.at(key).first;
    }

    /**
     * @brief Get const value reference / 获取常量值引用
     * @param key Key to lookup / 要查找的键
     * @return const V& Const reference to value / 值的常量引用
     */
    const V& at(const K& key) const {
        return data_.at(key).first;
    }

    /**
     * @brief Get access frequency / 获取访问频率
     * @param key Key to query / 要查询的键
     * @return size_t Access frequency / 访问频率
     */
    size_t get_freq(const K& key) const {
        return data_.at(key).second;
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
            ++it->second.second;
        }
    }

    /**
     * @brief Check if item should be promoted / 检查项目是否应该晋升
     * @param key Key to check / 要检查的键
     * @param threshold Promotion threshold / 晋升阈值
     * @return bool True if frequency >= threshold / 频率 >= 阈值返回 true
     */
    bool should_promote(const K& key, size_t threshold) const {
        auto it = data_.find(key);
        if (it == data_.end()) {
            return false;
        }
        return it->second.second >= threshold;
    }

    /**
     * @brief Remove entry by key / 按键移除条目
     * @param key Key to remove / 要移除的键
     */
    void erase(const K& key) {
        data_.erase(key);
    }

    /**
     * @brief Get current size / 获取当前大小
     * @return size_t Number of items / 项目数量
     */
    size_t size() const {
        return data_.size();
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
    typename std::map<K, std::pair<V, size_t>>::const_iterator begin() const {
        return data_.begin();
    }

    typename std::map<K, std::pair<V, size_t>>::const_iterator end() const {
        return data_.end();
    }

    /**
     * @brief Extract key-value pair / 提取键值对
     * @param key Key to extract / 要提取的键
     * @return std::pair<K, V> Extracted key-value pair / 提取的键值对
     * @throws std::runtime_error If key not found / 如果键不存在则抛出异常
     * 
     * Removes the entry from cold zone and returns its data.
     * 从冷区移除条目并返回其数据。
     */
    std::pair<K, V> extract(const K& key) {
        auto it = data_.find(key);
        if (it == data_.end()) {
            throw std::runtime_error("Key not found in ColdZone / 键不在冷区中");
        }
        K k = it->first;
        V v = it->second.first;
        data_.erase(it);
        return {k, v};
    }
};

} // namespace fpd
