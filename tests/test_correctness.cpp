#include <freq_partition_dict.hpp>
#include <gtest/gtest.h>
#include <string>

using namespace fpd;

// 基本功能测试
TEST(FreqPartitionDictTest, BasicInsertAndGet) {
    FreqPartitionDict<int, std::string> dict(4, 2);

    dict.insert(1, "one");
    dict.insert(2, "two");

    auto result1 = dict.get(1);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(*result1, "one");

    auto result2 = dict.get(2);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(*result2, "two");

    auto result3 = dict.get(3);
    EXPECT_FALSE(result3.has_value());
}

TEST(FreqPartitionDictTest, Contains) {
    FreqPartitionDict<int, std::string> dict(4, 2);

    dict.insert(1, "one");

    EXPECT_TRUE(dict.contains(1));
    EXPECT_FALSE(dict.contains(2));
}

TEST(FreqPartitionDictTest, Erase) {
    FreqPartitionDict<int, std::string> dict(4, 2);

    dict.insert(1, "one");
    EXPECT_TRUE(dict.erase(1));
    EXPECT_FALSE(dict.contains(1));
    EXPECT_FALSE(dict.erase(1));
}

TEST(FreqPartitionDictTest, SizeAndEmpty) {
    FreqPartitionDict<int, std::string> dict(4, 2);

    EXPECT_TRUE(dict.empty());
    EXPECT_EQ(dict.size(), 0);

    dict.insert(1, "one");
    EXPECT_FALSE(dict.empty());
    EXPECT_EQ(dict.size(), 1);

    dict.insert(2, "two");
    EXPECT_EQ(dict.size(), 2);

    dict.erase(1);
    EXPECT_EQ(dict.size(), 1);

    dict.clear();
    EXPECT_TRUE(dict.empty());
    EXPECT_EQ(dict.size(), 0);
}

// 晋升测试
TEST(FreqPartitionDictTest, Promotion) {
    FreqPartitionDict<int, std::string> dict(2, 2);

    // 插入3个元素，都在冷区
    dict.insert(1, "one");
    dict.insert(2, "two");
    dict.insert(3, "three");

    EXPECT_EQ(dict.hot_size(), 0);
    EXPECT_EQ(dict.cold_size(), 3);

    // 访问 key=1 两次，应该晋升到热区
    dict.get(1);
    dict.get(1);

    EXPECT_EQ(dict.hot_size(), 1);
    EXPECT_EQ(dict.cold_size(), 2);
    EXPECT_EQ(dict.promotions(), 1);
}

// 淘汰测试
TEST(FreqPartitionDictTest, Eviction) {
    FreqPartitionDict<int, std::string> dict(2, 2);

    // 插入并晋升两个元素到热区
    dict.insert(1, "one");
    dict.insert(2, "two");

    // 晋升 key=1
    dict.get(1);
    dict.get(1);

    // 晋升 key=2
    dict.get(2);
    dict.get(2);

    EXPECT_EQ(dict.hot_size(), 2);
    EXPECT_EQ(dict.demotions(), 0);

    // 插入并晋升第三个元素，应该淘汰频率最低的
    dict.insert(3, "three");
    dict.get(3);
    dict.get(3);

    EXPECT_EQ(dict.hot_size(), 2);
    EXPECT_EQ(dict.demotions(), 1);
}

// 更新测试
TEST(FreqPartitionDictTest, UpdateValue) {
    FreqPartitionDict<int, std::string> dict(4, 2);

    dict.insert(1, "one");
    dict.insert(1, "updated");

    auto result = dict.get(1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "updated");
}

// 统计信息测试
TEST(FreqPartitionDictTest, Statistics) {
    FreqPartitionDict<int, std::string> dict(2, 2);

    dict.insert(1, "one");
    dict.insert(2, "two");

    // 访问 key=1 三次（一次冷区，两次热区）
    dict.get(1);
    dict.get(1);
    dict.get(1);

    EXPECT_EQ(dict.hot_hits(), 2);
    EXPECT_EQ(dict.cold_hits(), 1);
    EXPECT_EQ(dict.total_hit_rate(), 1.0);

    dict.reset_stats();
    EXPECT_EQ(dict.hot_hits(), 0);
    EXPECT_EQ(dict.cold_hits(), 0);
}

// 边界条件测试
TEST(FreqPartitionDictTest, ZeroHotCapacity) {
    FreqPartitionDict<int, std::string> dict(0, 2);

    dict.insert(1, "one");
    EXPECT_EQ(dict.hot_size(), 0);

    // 即使访问多次也不会晋升
    for (int i = 0; i < 10; ++i) {
        dict.get(1);
    }

    EXPECT_EQ(dict.hot_size(), 0);
    EXPECT_EQ(dict.promotions(), 0);
}

TEST(FreqPartitionDictTest, ThresholdOne) {
    FreqPartitionDict<int, std::string> dict(4, 1);

    dict.insert(1, "one");

    // 访问一次就应该晋升
    dict.get(1);

    EXPECT_EQ(dict.hot_size(), 1);
    EXPECT_EQ(dict.promotions(), 1);
}

// HotZone 单独测试
TEST(HotZoneTest, BasicOperations) {
    HotZone<int, std::string> hot(4);

    EXPECT_TRUE(hot.empty());

    hot.insert(1, "one", 5);
    hot.insert(2, "two", 3);
    hot.insert(3, "three", 7);

    EXPECT_EQ(hot.size(), 3);
    EXPECT_FALSE(hot.empty());
    EXPECT_TRUE(hot.contains(1));
    EXPECT_FALSE(hot.contains(99));

    EXPECT_EQ(hot.at(1), "one");
    EXPECT_EQ(hot.get_freq(1), 5);

    auto min_key = hot.get_min_freq_key();
    EXPECT_EQ(min_key.first, 2);
    EXPECT_EQ(min_key.second, 3);

    hot.increase_freq(2);
    min_key = hot.get_min_freq_key();
    EXPECT_EQ(min_key.first, 2);
    EXPECT_EQ(min_key.second, 4);
}

TEST(HotZoneTest, EraseAndClear) {
    HotZone<int, std::string> hot(4);

    hot.insert(1, "one");
    hot.insert(2, "two");

    hot.erase(1);
    EXPECT_FALSE(hot.contains(1));
    EXPECT_EQ(hot.size(), 1);

    hot.clear();
    EXPECT_TRUE(hot.empty());
}

// ColdZone 单独测试
TEST(ColdZoneTest, BasicOperations) {
    ColdZone<int, std::string> cold;

    EXPECT_TRUE(cold.empty());

    cold.insert(1, "one", 5);
    cold.insert(2, "two", 3);

    EXPECT_EQ(cold.size(), 2);
    EXPECT_TRUE(cold.contains(1));
    EXPECT_FALSE(cold.contains(99));

    EXPECT_EQ(cold.at(1), "one");
    EXPECT_EQ(cold.get_freq(1), 5);

    EXPECT_TRUE(cold.should_promote(1, 5));
    EXPECT_FALSE(cold.should_promote(1, 6));
    EXPECT_FALSE(cold.should_promote(99, 1));

    cold.increase_freq(2);
    EXPECT_EQ(cold.get_freq(2), 4);
}

TEST(ColdZoneTest, EraseAndClear) {
    ColdZone<int, std::string> cold;

    cold.insert(1, "one");
    cold.insert(2, "two");

    cold.erase(1);
    EXPECT_FALSE(cold.contains(1));
    EXPECT_EQ(cold.size(), 1);

    cold.clear();
    EXPECT_TRUE(cold.empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
