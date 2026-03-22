#include <benchmark/benchmark.h>
#include <freq_partition_dict.hpp>
#include <unordered_map>
#include <map>
#include <random>
#include <vector>
#include <queue>
#include <cmath>

using namespace fpd;

// ============================================================================
// Real-world Workload Simulations / 真实工作负载模拟
// ============================================================================

/**
 * @brief Database Query Workload Simulator / 数据库查询工作负载模拟器
 * 
 * Simulates TPC-C like workload where:
 * - 80% queries access recent data (last 1000 records)
 * - 15% queries access historical data (older records)
 * - 5% queries are random full table scans
 */
class DatabaseWorkloadGenerator {
private:
    size_t total_records_;
    size_t recent_window_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;
    std::uniform_int_distribution<size_t> recent_dist_;
    std::uniform_int_distribution<size_t> historical_dist_;
    std::uniform_int_distribution<size_t> full_scan_dist_;

public:
    DatabaseWorkloadGenerator(size_t total_records = 100000, 
                               size_t recent_window = 1000,
                               unsigned seed = 42)
        : total_records_(total_records)
        , recent_window_(recent_window)
        , rng_(seed)
        , dist_(0.0, 1.0)
        , recent_dist_(total_records - recent_window + 1, total_records)
        , historical_dist_(1, total_records - recent_window)
        , full_scan_dist_(1, total_records) {}

    size_t next() {
        double p = dist_(rng_);
        if (p < 0.80) {
            // Recent data access / 近期数据访问
            return recent_dist_(rng_);
        } else if (p < 0.95) {
            // Historical data access / 历史数据访问
            return historical_dist_(rng_);
        } else {
            // Random full scan / 随机全表扫描
            return full_scan_dist_(rng_);
        }
    }
};

/**
 * @brief Web Server Access Log Simulator / Web 服务器访问日志模拟器
 * 
 * Simulates web server access patterns:
 * - Popular pages get 70% of traffic (Zipf distribution)
 * - Trending pages get 20% of traffic (time-localized)
 * - Long tail gets 10% of traffic
 */
class WebServerWorkloadGenerator {
private:
    size_t n_pages_;
    double alpha_;
    std::vector<double> cdf_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;
    size_t current_time_ = 0;
    std::vector<size_t> trending_pages_;

public:
    WebServerWorkloadGenerator(size_t n_pages = 10000, double alpha = 1.2, 
                                unsigned seed = 42)
        : n_pages_(n_pages), alpha_(alpha), rng_(seed), dist_(0.0, 1.0) {
        // Build Zipf CDF for popular pages / 构建热门页面的 Zipf CDF
        double sum = 0.0;
        for (size_t i = 1; i <= n_pages; ++i) {
            sum += 1.0 / std::pow(i, alpha);
        }

        cdf_.resize(n_pages);
        double cumsum = 0.0;
        for (size_t i = 1; i <= n_pages; ++i) {
            cumsum += 1.0 / std::pow(i, alpha) / sum;
            cdf_[i - 1] = cumsum;
        }

        // Initialize trending pages / 初始化趋势页面
        update_trending();
    }

    void update_trending() {
        // Update trending pages every 1000 requests
        // 每 1000 个请求更新一次趋势页面
        trending_pages_.clear();
        std::uniform_int_distribution<size_t> dist(1, n_pages_);
        for (int i = 0; i < 100; ++i) {
            trending_pages_.push_back(dist(rng_));
        }
    }

    size_t next() {
        ++current_time_;
        if (current_time_ % 1000 == 0) {
            update_trending();
        }

        double p = dist_(rng_);
        if (p < 0.70) {
            // Popular page (Zipf) / 热门页面
            double u = dist_(rng_);
            auto it = std::lower_bound(cdf_.begin(), cdf_.end(), u);
            return (it - cdf_.begin()) + 1;
        } else if (p < 0.90) {
            // Trending page / 趋势页面
            std::uniform_int_distribution<size_t> dist(0, trending_pages_.size() - 1);
            return trending_pages_[dist(rng_)];
        } else {
            // Long tail / 长尾
            std::uniform_int_distribution<size_t> dist(1, n_pages_);
            return dist(rng_);
        }
    }
};

/**
 * @brief Social Network Friend Query Simulator / 社交网络好友查询模拟器
 * 
 * Simulates social network queries:
 * - Active users are queried frequently
 * - User activity changes over time (users become active/inactive)
 * - Friend-of-friend queries access related users
 */
class SocialNetworkWorkloadGenerator {
private:
    size_t n_users_;
    size_t n_active_;
    std::mt19937 rng_;
    std::vector<size_t> active_users_;
    std::uniform_real_distribution<double> dist_;
    size_t query_count_ = 0;

public:
    SocialNetworkWorkloadGenerator(size_t n_users = 50000, size_t n_active = 500,
                                    unsigned seed = 42)
        : n_users_(n_users), n_active_(n_active), rng_(seed), dist_(0.0, 1.0) {
        // Initialize active users / 初始化活跃用户
        update_active_users();
    }

    void update_active_users() {
        // 10% chance to change active user set every 5000 queries
        // 每 5000 个查询有 10% 概率改变活跃用户集合
        active_users_.clear();
        std::uniform_int_distribution<size_t> dist(1, n_users_);
        for (size_t i = 0; i < n_active_; ++i) {
            active_users_.push_back(dist(rng_));
        }
    }

    size_t next() {
        ++query_count_;
        if (query_count_ % 5000 == 0 && dist_(rng_) < 0.10) {
            update_active_users();
        }

        double p = dist_(rng_);
        if (p < 0.75) {
            // Query active user / 查询活跃用户
            std::uniform_int_distribution<size_t> dist(0, active_users_.size() - 1);
            return active_users_[dist(rng_)];
        } else if (p < 0.90) {
            // Friend-of-friend (related to active user) / 好友的好友
            std::uniform_int_distribution<size_t> dist(0, active_users_.size() - 1);
            size_t user = active_users_[dist(rng_)];
            // Simulate friend connection (user + small offset)
            // 模拟好友关系（用户 + 小偏移）
            std::uniform_int_distribution<size_t> offset_dist(1, 100);
            return std::min(user + offset_dist(rng_), n_users_);
        } else {
            // Random user / 随机用户
            std::uniform_int_distribution<size_t> dist(1, n_users_);
            return dist(rng_);
        }
    }
};

// ============================================================================
// Benchmark Functions / 基准测试函数
// ============================================================================

// Database Workload Benchmarks / 数据库工作负载基准测试
static void BM_Database_UnorderedMap(benchmark::State& state) {
    const size_t N = 100000;
    std::unordered_map<size_t, size_t> map;
    
    for (size_t i = 1; i <= N; ++i) {
        map[i] = i;
    }

    DatabaseWorkloadGenerator workload(N, 1000, 42);
    std::vector<size_t> keys;
    for (int i = 0; i < 10000; ++i) {
        keys.push_back(workload.next());
    }

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find(keys[idx % keys.size()]));
        ++idx;
    }
}
BENCHMARK(BM_Database_UnorderedMap);

static void BM_Database_Map(benchmark::State& state) {
    const size_t N = 100000;
    std::map<size_t, size_t> map;
    
    for (size_t i = 1; i <= N; ++i) {
        map[i] = i;
    }

    DatabaseWorkloadGenerator workload(N, 1000, 42);
    std::vector<size_t> keys;
    for (int i = 0; i < 10000; ++i) {
        keys.push_back(workload.next());
    }

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find(keys[idx % keys.size()]));
        ++idx;
    }
}
BENCHMARK(BM_Database_Map);

static void BM_Database_FreqPartitionDict(benchmark::State& state) {
    const size_t N = 100000;
    const size_t HOT_CAP = state.range(0);
    
    FreqPartitionDict<size_t, size_t> dict(HOT_CAP, 3);
    
    for (size_t i = 1; i <= N; ++i) {
        dict.insert(i, i);
    }

    // Warmup / 预热
    DatabaseWorkloadGenerator warmup(N, 1000, 42);
    for (int i = 0; i < 10000; ++i) {
        dict.get(warmup.next());
    }

    DatabaseWorkloadGenerator workload(N, 1000, 123);
    std::vector<size_t> keys;
    for (int i = 0; i < 10000; ++i) {
        keys.push_back(workload.next());
    }

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(dict.get(keys[idx % keys.size()]));
        ++idx;
    }
}
BENCHMARK(BM_Database_FreqPartitionDict)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512);

// Web Server Workload Benchmarks / Web 服务器工作负载基准测试
static void BM_WebServer_UnorderedMap(benchmark::State& state) {
    const size_t N = 10000;
    std::unordered_map<size_t, size_t> map;
    
    for (size_t i = 1; i <= N; ++i) {
        map[i] = i;
    }

    WebServerWorkloadGenerator workload(N, 1.2, 42);
    std::vector<size_t> keys;
    for (int i = 0; i < 10000; ++i) {
        keys.push_back(workload.next());
    }

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find(keys[idx % keys.size()]));
        ++idx;
    }
}
BENCHMARK(BM_WebServer_UnorderedMap);

static void BM_WebServer_FreqPartitionDict(benchmark::State& state) {
    const size_t N = 10000;
    const size_t HOT_CAP = state.range(0);
    
    FreqPartitionDict<size_t, size_t> dict(HOT_CAP, 3);
    
    for (size_t i = 1; i <= N; ++i) {
        dict.insert(i, i);
    }

    // Warmup / 预热
    WebServerWorkloadGenerator warmup(N, 1.2, 42);
    for (int i = 0; i < 10000; ++i) {
        dict.get(warmup.next());
    }

    WebServerWorkloadGenerator workload(N, 1.2, 123);
    std::vector<size_t> keys;
    for (int i = 0; i < 10000; ++i) {
        keys.push_back(workload.next());
    }

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(dict.get(keys[idx % keys.size()]));
        ++idx;
    }
}
BENCHMARK(BM_WebServer_FreqPartitionDict)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256);

// Social Network Workload Benchmarks / 社交网络工作负载基准测试
static void BM_SocialNetwork_UnorderedMap(benchmark::State& state) {
    const size_t N = 50000;
    std::unordered_map<size_t, size_t> map;
    
    for (size_t i = 1; i <= N; ++i) {
        map[i] = i;
    }

    SocialNetworkWorkloadGenerator workload(N, 500, 42);
    std::vector<size_t> keys;
    for (int i = 0; i < 10000; ++i) {
        keys.push_back(workload.next());
    }

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find(keys[idx % keys.size()]));
        ++idx;
    }
}
BENCHMARK(BM_SocialNetwork_UnorderedMap);

static void BM_SocialNetwork_FreqPartitionDict(benchmark::State& state) {
    const size_t N = 50000;
    const size_t HOT_CAP = state.range(0);
    
    FreqPartitionDict<size_t, size_t> dict(HOT_CAP, 3);
    
    for (size_t i = 1; i <= N; ++i) {
        dict.insert(i, i);
    }

    // Warmup / 预热
    SocialNetworkWorkloadGenerator warmup(N, 500, 42);
    for (int i = 0; i < 10000; ++i) {
        dict.get(warmup.next());
    }

    SocialNetworkWorkloadGenerator workload(N, 500, 123);
    std::vector<size_t> keys;
    for (int i = 0; i < 10000; ++i) {
        keys.push_back(workload.next());
    }

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(dict.get(keys[idx % keys.size()]));
        ++idx;
    }
}
BENCHMARK(BM_SocialNetwork_FreqPartitionDict)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024);

// ============================================================================
// Hit Rate Analysis / 命中率分析
// ============================================================================

static void BM_Database_HitRate(benchmark::State& state) {
    const size_t N = 100000;
    const size_t HOT_CAP = state.range(0);
    
    FreqPartitionDict<size_t, size_t> dict(HOT_CAP, 3);
    
    for (size_t i = 1; i <= N; ++i) {
        dict.insert(i, i);
    }

    // Warmup / 预热
    DatabaseWorkloadGenerator warmup(N, 1000, 42);
    for (int i = 0; i < 50000; ++i) {
        dict.get(warmup.next());
    }

    dict.reset_stats();

    DatabaseWorkloadGenerator workload(N, 1000, 123);
    for (auto _ : state) {
        dict.get(workload.next());
    }

    // Report hit rate / 报告命中率
    state.counters["hot_hit_rate"] = dict.hot_hit_rate();
    state.counters["total_hit_rate"] = dict.total_hit_rate();
    state.counters["promotions"] = dict.promotions();
}
BENCHMARK(BM_Database_HitRate)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024);

BENCHMARK_MAIN();
