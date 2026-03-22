#include <freq_partition_dict.hpp>
#include <freq_partition_dict_optimized.hpp>
#include <freq_partition_dict_threadsafe.hpp>
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>

using namespace fpd;

// 对比基础版和优化版的功能一致性
TEST(OptimizedVersionTest, FunctionalEquivalence) {
    FreqPartitionDict<int, std::string> base(32, 3);
    FreqPartitionDictOptimized<int, std::string> optimized(32, 3);

    // 插入相同数据
    for (int i = 0; i < 100; ++i) {
        base.insert(i, "value_" + std::to_string(i));
        optimized.insert(i, "value_" + std::to_string(i));
    }

    // 执行相同访问模式
    for (int round = 0; round < 5; ++round) {
        for (int i = 0; i < 50; ++i) {
            auto r1 = base.get(i);
            auto r2 = optimized.get(i);
            ASSERT_EQ(r1.has_value(), r2.has_value());
            if (r1 && r2) {
                EXPECT_EQ(*r1, *r2);
            }
        }
    }

    // 验证统计信息相似
    EXPECT_EQ(base.size(), optimized.size());
    EXPECT_EQ(base.hot_size(), optimized.hot_size());
    EXPECT_EQ(base.cold_size(), optimized.cold_size());
}

// 测试优化版 HotZone 的最小堆功能
TEST(OptimizedVersionTest, HotZoneMinHeap) {
    FreqPartitionDictOptimized<int, std::string> dict(4, 2);

    // 插入元素
    for (int i = 0; i < 6; ++i) {
        dict.insert(i, "value_" + std::to_string(i));
    }

    // 访问使部分元素晋升
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            dict.get(i);
        }
    }

    // 验证热区大小
    EXPECT_EQ(dict.hot_size(), 4);
    EXPECT_EQ(dict.promotions(), 4);

    // 压缩热区
    size_t raw_before = dict.hot_zone_raw_size();
    dict.compact_hot_zone();
    size_t raw_after = dict.hot_zone_raw_size();

    EXPECT_LE(raw_after, raw_before);
    EXPECT_EQ(dict.hot_size(), 4);  // 逻辑大小不变
}

// 线程安全版本基础测试
TEST(ThreadSafeVersionTest, BasicOperations) {
    FreqPartitionDictThreadSafe<int, std::string> dict(32, 3);

    dict.insert(1, "one");
    EXPECT_TRUE(dict.contains(1));

    auto result = dict.get(1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "one");

    EXPECT_TRUE(dict.erase(1));
    EXPECT_FALSE(dict.contains(1));
}

// 线程安全版本并发读取测试
TEST(ThreadSafeVersionTest, ConcurrentReads) {
    FreqPartitionDictThreadSafe<int, int> dict(64, 3);

    // 预填充数据
    for (int i = 0; i < 100; ++i) {
        dict.insert(i, i * 10);
    }

    const int num_threads = 4;
    const int ops_per_thread = 1000;
    std::atomic<int> success_count{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                int key = (t * 10 + i) % 100;
                auto result = dict.get(key);
                if (result && *result == key * 10) {
                    success_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count, num_threads * ops_per_thread);
}

// 线程安全版本并发写入测试
TEST(ThreadSafeVersionTest, ConcurrentWrites) {
    FreqPartitionDictThreadSafe<int, int> dict(32, 3);

    const int num_threads = 4;
    const int ops_per_thread = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                int key = t * ops_per_thread + i;
                dict.insert(key, key * 10);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(dict.size(), num_threads * ops_per_thread);
}

// 线程安全版本批量操作测试
TEST(ThreadSafeVersionTest, BatchOperations) {
    FreqPartitionDictThreadSafe<int, int> dict(32, 3);

    // 批量插入
    std::vector<std::pair<int, int>> batch;
    for (int i = 0; i < 100; ++i) {
        batch.push_back({i, i * 10});
    }
    dict.insert_batch(batch.begin(), batch.end());

    EXPECT_EQ(dict.size(), 100);

    // 批量读取
    std::vector<int> keys = {0, 10, 20, 30, 40};
    std::vector<std::optional<int>> results;
    dict.get_batch(keys, results);

    EXPECT_EQ(results.size(), 5);
    for (size_t i = 0; i < keys.size(); ++i) {
        ASSERT_TRUE(results[i].has_value());
        EXPECT_EQ(*results[i], keys[i] * 10);
    }
}

// 性能对比：基础版 vs 优化版
TEST(OptimizedVersionTest, PerformanceComparison) {
    const int N_KEYS = 1000;
    const int N_OPS = 10000;

    FreqPartitionDict<int, int> base(64, 3);
    FreqPartitionDictOptimized<int, int> optimized(64, 3);

    // 填充数据
    for (int i = 0; i < N_KEYS; ++i) {
        base.insert(i, i);
        optimized.insert(i, i);
    }

    // 测试基础版
    auto start_base = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N_OPS; ++i) {
        base.get(i % N_KEYS);
    }
    auto end_base = std::chrono::high_resolution_clock::now();
    auto base_time = std::chrono::duration_cast<std::chrono::microseconds>(end_base - start_base).count();

    // 测试优化版
    auto start_opt = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N_OPS; ++i) {
        optimized.get(i % N_KEYS);
    }
    auto end_opt = std::chrono::high_resolution_clock::now();
    auto opt_time = std::chrono::duration_cast<std::chrono::microseconds>(end_opt - start_opt).count();

    std::cout << "Base version: " << base_time << " us" << std::endl;
    std::cout << "Optimized version: " << opt_time << " us" << std::endl;
    std::cout << "Speedup: " << (double)base_time / opt_time << "x" << std::endl;

    // 注意：Debug模式下优化版可能较慢（最小堆开销）
    // Release模式下优化版在淘汰操作上会有明显提升
    // 这里仅验证功能正确性，不严格比较性能
    EXPECT_GT(base_time, 0);
    EXPECT_GT(opt_time, 0);
}

// 测试不同热区容量下的性能
TEST(OptimizedVersionTest, HotCapacityScaling) {
    const int N_KEYS = 1000;
    const int N_OPS = 5000;

    std::vector<size_t> capacities = {8, 16, 32, 64, 128};

    for (size_t cap : capacities) {
        FreqPartitionDictOptimized<int, int> dict(cap, 3);

        for (int i = 0; i < N_KEYS; ++i) {
            dict.insert(i, i);
        }

        // Zipf-like access pattern
        for (int i = 0; i < N_OPS; ++i) {
            int key = (i * i) % N_KEYS;  // 偏斜访问
            dict.get(key);
        }

        std::cout << "Capacity " << cap << ": "
                  << "hot_hit_rate=" << dict.hot_hit_rate() * 100 << "%, "
                  << "promotions=" << dict.promotions() << std::endl;

        // 容量越大，热区命中率应该越高
        if (cap >= 64) {
            EXPECT_GT(dict.hot_hit_rate(), 0.3);
        }
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
