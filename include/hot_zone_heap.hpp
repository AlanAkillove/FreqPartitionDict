#pragma once

#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <utility>
#include <functional>

namespace fpd {

/**
 * @brief Hot Zone with Min-Heap Optimization / 基于最小堆优化的热区
 * 
 * Uses a combination of hash table and min-heap to achieve O(log H) eviction.
 * 使用哈希表和最小堆的组合实现 O(log H) 淘汰。
 * 
 * Time Complexities:
 * - Lookup: O(1)
 * - Insert: O(log H)
 * - Eviction (pop_min): O(log H)
 * - Increase frequency: O(log H)
 */
template<typename K, typename V>
class HotZoneHeap {
private:
    /**
     * @brief Heap node structure / 堆节点结构
     */
    struct HeapNode {
        K key;              // Key / 键
        size_t freq;        // Access frequency / 访问频率
        size_t index;       // Position in heap / 在堆中的位置
    };

    /**
     * @brief Entry stored in hash table / 哈希表中存储的条目
     */
    struct Entry {
        V value;                    // Stored value / 存储的值
        size_t heap_index;          // Index in heap / 在堆中的索引
    };

    std::unordered_map<K, Entry> data_;     // Hash table storage / 哈希表存储
    std::vector<HeapNode> heap_;             // Min-heap / 最小堆
    size_t capacity_;                        // Maximum capacity / 最大容量

    /**
     * @brief Heapify up operation / 上浮操作
     * @param index Starting index / 起始索引
     */
    void heapify_up(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (heap_[index].freq >= heap_[parent].freq) {
                break;
            }
            // Swap with parent
            std::swap(heap_[index], heap_[parent]);
            // Update indices
            data_[heap_[index].key].heap_index = index;
            data_[heap_[parent].key].heap_index = parent;
            index = parent;
        }
    }

    /**
     * @brief Heapify down operation / 下沉操作
     * @param index Starting index / 起始索引
     */
    void heapify_down(size_t index) {
        size_t size = heap_.size();
        while (true) {
            size_t smallest = index;
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;

            if (left < size && heap_[left].freq < heap_[smallest].freq) {
                smallest = left;
            }
            if (right < size && heap_[right].freq < heap_[smallest].freq) {
                smallest = right;
            }
            if (smallest == index) {
                break;
            }
            // Swap with smallest child
            std::swap(heap_[index], heap_[smallest]);
            // Update indices
            data_[heap_[index].key].heap_index = index;
            data_[heap_[smallest].key].heap_index = smallest;
            index = smallest;
        }
    }

public:
    /**
     * @brief Constructor / 构造函数
     * @param capacity Maximum number of items / 最大项目数
     */
    explicit HotZoneHeap(size_t capacity) : capacity_(capacity) {
        heap_.reserve(capacity);
    }

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
        size_t heap_index = data_.at(key).heap_index;
        return heap_[heap_index].freq;
    }

    /**
     * @brief Insert or update entry / 插入或更新条目
     * @param key Key to insert / 要插入的键
     * @param value Value to insert / 要插入的值
     * @param freq Initial frequency (default: 0) / 初始频率（默认：0）
     * 
     * Time complexity: O(log H)
     * 时间复杂度：O(log H)
     */
    void insert(const K& key, const V& value, size_t freq = 0) {
        if (data_.find(key) != data_.end()) {
            // Update existing entry
            data_[key].value = value;
            size_t idx = data_[key].heap_index;
            heap_[idx].freq = freq;
            heapify_up(idx);
            heapify_down(idx);
        } else {
            // Insert new entry
            size_t index = heap_.size();
            heap_.push_back({key, freq, index});
            data_[key] = {value, index};
            heapify_up(index);
        }
    }

    /**
     * @brief Increment access frequency / 增加访问频率
     * @param key Key to update / 要更新的键
     * 
     * Time complexity: O(log H)
     * 时间复杂度：O(log H)
     */
    void increase_freq(const K& key) {
        auto it = data_.find(key);
        if (it != data_.end()) {
            size_t idx = it->second.heap_index;
            ++heap_[idx].freq;
            // Frequency increased, may need to move down
            heapify_down(idx);
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
     * @brief Remove and return the item with minimum frequency
     *        移除并返回频率最低的项
     * @return std::pair<K, size_t> Key and its frequency / 键及其频率
     * @throws std::logic_error If empty / 如果为空则抛出异常
     * 
     * Time complexity: O(log H)
     * 时间复杂度：O(log H)
     */
    std::pair<K, size_t> pop_min() {
        if (heap_.empty()) {
            throw std::logic_error("HotZoneHeap is empty / 热区为空");
        }

        // Get minimum (root of heap)
        K min_key = heap_[0].key;
        size_t min_freq = heap_[0].freq;

        // Remove from hash table
        data_.erase(min_key);

        // Remove root from heap
        if (heap_.size() > 1) {
            heap_[0] = heap_.back();
            heap_[0].index = 0;
            data_[heap_[0].key].heap_index = 0;
            heap_.pop_back();
            heapify_down(0);
        } else {
            heap_.pop_back();
        }

        return {min_key, min_freq};
    }

    /**
     * @brief Find key with minimum frequency (without removing)
     *        查找频率最低的键（不移除）
     * @return std::pair<K, size_t> Key and its frequency / 键及其频率
     * @throws std::logic_error If empty / 如果为空则抛出异常
     * 
     * Time complexity: O(1)
     * 时间复杂度：O(1)
     */
    std::pair<K, size_t> get_min_freq_key() const {
        if (heap_.empty()) {
            throw std::logic_error("HotZoneHeap is empty / 热区为空");
        }
        return {heap_[0].key, heap_[0].freq};
    }

    /**
     * @brief Remove entry by key / 按键移除条目
     * @param key Key to remove / 要移除的键
     * 
     * Time complexity: O(log H)
     * 时间复杂度：O(log H)
     */
    void erase(const K& key) {
        auto it = data_.find(key);
        if (it == data_.end()) {
            return;
        }

        size_t idx = it->second.heap_index;
        data_.erase(it);

        // Remove from heap
        if (idx == heap_.size() - 1) {
            heap_.pop_back();
        } else {
            // Move last element to position idx
            heap_[idx] = heap_.back();
            heap_[idx].index = idx;
            data_[heap_[idx].key].heap_index = idx;
            heap_.pop_back();

            // Restore heap property
            heapify_up(idx);
            heapify_down(idx);
        }
    }

    /**
     * @brief Clear all entries / 清空所有条目
     */
    void clear() {
        data_.clear();
        heap_.clear();
    }

    /**
     * @brief Check if empty / 检查是否为空
     * @return bool True if empty / 为空返回 true
     */
    bool empty() const {
        return data_.empty();
    }

    /**
     * @brief Reserve capacity / 预分配容量
     * @param capacity Capacity to reserve / 要预分配的容量
     */
    void reserve(size_t capacity) {
        heap_.reserve(capacity);
        data_.reserve(capacity * 2);
    }
};

} // namespace fpd
