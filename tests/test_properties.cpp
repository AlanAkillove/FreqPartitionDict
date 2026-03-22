#include <freq_partition_dict.hpp>
#include <gtest/gtest.h>
#include <unordered_map>
#include <random>
#include <vector>

using namespace fpd;

// 属性测试：随机操作序列验证
TEST(PropertyTest, RandomOperations) {
    const int NUM_OPERATIONS = 10000;
    const int KEY_RANGE = 100;

    FreqPartitionDict<int, int> dict(16, 3);
    std::unordered_map<int, int> reference;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> key_dist(0, KEY_RANGE - 1);
    std::uniform_int_distribution<int> value_dist(0, 1000);
    std::uniform_int_distribution<int> op_dist(0, 2);

    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        int op = op_dist(rng);
        int key = key_dist(rng);

        switch (op) {
            case 0: { // Insert
                int value = value_dist(rng);
                dict.insert(key, value);
                reference[key] = value;
                break;
            }
            case 1: { // Get
                auto dict_result = dict.get(key);
                auto ref_it = reference.find(key);
                
                if (ref_it != reference.end()) {
                    ASSERT_TRUE(dict_result.has_value()) 
                        << "Key " << key << " should exist";
                    EXPECT_EQ(*dict_result, ref_it->second)
                        << "Value mismatch for key " << key;
                } else {
                    EXPECT_FALSE(dict_result.has_value())
                        << "Key " << key << " should not exist";
                }
                break;
            }
            case 2: { // Erase
                dict.erase(key);
                reference.erase(key);
                break;
            }
        }

        // 验证大小一致
        EXPECT_EQ(dict.size(), reference.size())
            << "Size mismatch after operation " << i;
    }

    // 最终验证所有键值对
    for (const auto& [key, value] : reference) {
        auto result = dict.get(key);
        ASSERT_TRUE(result.has_value())
            << "Final check: key " << key << " should exist";
        EXPECT_EQ(*result, value)
            << "Final check: value mismatch for key " << key;
    }
}

// 属性测试：热区命中率随访问倾斜度变化
TEST(PropertyTest, HitRateWithSkew) {
    const int N_KEYS = 100;
    const int N_OPS = 50000;

    // 测试不同热区容量下的命中率
    for (size_t hot_cap : {8, 16, 32, 64}) {
        FreqPartitionDict<int, int> dict(hot_cap, 3);

        // 插入数据
        for (int i = 0; i < N_KEYS; ++i) {
            dict.insert(i, i * 10);
        }

        // 模拟倾斜访问：前 20% 的键获得 80% 的访问
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        for (int i = 0; i < N_OPS; ++i) {
            double u = dist(rng);
            int key;
            if (u < 0.8) {
                // 80% 访问前 20% 的键
                std::uniform_int_distribution<int> hot_dist(0, N_KEYS / 5 - 1);
                key = hot_dist(rng);
            } else {
                // 20% 访问其余键
                std::uniform_int_distribution<int> cold_dist(N_KEYS / 5, N_KEYS - 1);
                key = cold_dist(rng);
            }
            dict.get(key);
        }

        // 热区命中率应该较高
        EXPECT_GT(dict.hot_hit_rate(), 0.5)
            << "Hot capacity " << hot_cap << " should achieve >50% hot hit rate";
        EXPECT_EQ(dict.total_hit_rate(), 1.0)
            << "All accesses should hit";
    }
}

// 属性测试：晋升阈值影响
TEST(PropertyTest, PromotionThresholdEffect) {
    const int N_KEYS = 50;
    const int N_ACCESSES = 100;

    for (size_t threshold : {1, 3, 5, 10}) {
        FreqPartitionDict<int, int> dict(20, threshold);

        for (int i = 0; i < N_KEYS; ++i) {
            dict.insert(i, i);
        }

        // 每个键访问 N_ACCESSES 次
        for (int i = 0; i < N_KEYS; ++i) {
            for (int j = 0; j < N_ACCESSES; ++j) {
                dict.get(i);
            }
        }

        // 阈值越低，晋升次数越多
        // 阈值=1: 每个键访问第一次就晋升
        // 阈值=10: 需要访问10次才晋升
        if (threshold == 1) {
            EXPECT_GE(dict.promotions(), static_cast<size_t>(N_KEYS));
        }
    }
}

// 属性测试：热点迁移
TEST(PropertyTest, HotspotShift) {
    FreqPartitionDict<int, int> dict(10, 3);

    // 第一阶段：访问 A 组键
    for (int i = 0; i < 10; ++i) {
        dict.insert(i, i);
    }

    for (int round = 0; round < 5; ++round) {
        for (int i = 0; i < 10; ++i) {
            for (int j = 0; j < 5; ++j) {
                dict.get(i);
            }
        }
    }

    size_t promotions_after_phase1 = dict.promotions();

    // 第二阶段：访问 B 组键（新热点）
    for (int i = 10; i < 20; ++i) {
        dict.insert(i, i);
    }

    for (int round = 0; round < 5; ++round) {
        for (int i = 10; i < 20; ++i) {
            for (int j = 0; j < 5; ++j) {
                dict.get(i);
            }
        }
    }

    // 应该有更多晋升（B组键晋升）和降级（A组键被淘汰）
    EXPECT_GT(dict.promotions(), promotions_after_phase1);
    EXPECT_GT(dict.demotions(), 0);
}

// 属性测试：空字典行为
TEST(PropertyTest, EmptyDictBehavior) {
    FreqPartitionDict<int, int> dict(10, 3);

    EXPECT_TRUE(dict.empty());
    EXPECT_EQ(dict.size(), 0);
    EXPECT_EQ(dict.hot_size(), 0);
    EXPECT_EQ(dict.cold_size(), 0);
    EXPECT_EQ(dict.hot_hit_rate(), 0.0);
    EXPECT_EQ(dict.total_hit_rate(), 0.0);

    // 访问不存在的键
    auto result = dict.get(1);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(dict.misses(), 1);

    // 删除不存在的键
    EXPECT_FALSE(dict.erase(1));

    // 清空空字典
    dict.clear();
    EXPECT_TRUE(dict.empty());
}

// 压力测试：大量操作
TEST(StressTest, LargeScaleOperations) {
    const int N_KEYS = 10000;
    const int N_OPS = 100000;

    FreqPartitionDict<int, int> dict(128, 3);

    // 插入大量数据
    for (int i = 0; i < N_KEYS; ++i) {
        dict.insert(i, i);
    }

    EXPECT_EQ(dict.size(), static_cast<size_t>(N_KEYS));

    // 大量随机访问
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> key_dist(0, N_KEYS - 1);

    for (int i = 0; i < N_OPS; ++i) {
        dict.get(key_dist(rng));
    }

    // 验证所有数据仍然存在
    for (int i = 0; i < N_KEYS; ++i) {
        auto result = dict.get(i);
        ASSERT_TRUE(result.has_value())
            << "Key " << i << " should exist after stress test";
        EXPECT_EQ(*result, i);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
