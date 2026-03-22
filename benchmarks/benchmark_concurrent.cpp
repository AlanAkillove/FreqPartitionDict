#include <benchmark/benchmark.h>
#include <freq_partition_dict_threadsafe.hpp>
#include <unordered_map>
#include <map>
#include <random>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

using namespace fpd;

// ============================================================================
// Concurrent Workload Generators / 并发工作负载生成器
// ============================================================================

/**
 * @brief Zipf distribution generator / Zipf 分布生成器
 */
class ZipfGenerator {
private:
    size_t n_;
    double alpha_;
    std::vector<double> cdf_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;

public:
    ZipfGenerator(size_t n, double alpha, unsigned seed = 42)
        : n_(n), alpha_(alpha), rng_(seed), dist_(0.0, 1.0) {
        double sum = 0.0;
        for (size_t i = 1; i <= n; ++i) {
            sum += 1.0 / std::pow(i, alpha);
        }

        cdf_.resize(n);
        double cumsum = 0.0;
        for (size_t i = 1; i <= n; ++i) {
            cumsum += 1.0 / std::pow(i, alpha) / sum;
            cdf_[i - 1] = cumsum;
        }
    }

    size_t next() {
        double u = dist_(rng_);
        auto it = std::lower_bound(cdf_.begin(), cdf_.end(), u);
        return (it - cdf_.begin()) + 1;
    }
};

// ============================================================================
// Thread-safe wrappers for standard containers / 标准容器的线程安全包装
// ============================================================================

template<typename K, typename V>
class ThreadSafeUnorderedMap {
private:
    std::unordered_map<K, V> map_;
    mutable std::shared_mutex mutex_;

public:
    void insert(const K& key, const V& value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        map_[key] = value;
    }

    std::optional<V> get(const K& key) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return map_.size();
    }
};

// ============================================================================
// Concurrent Benchmarks / 并发基准测试
// ============================================================================

/**
 * @brief Concurrent read benchmark / 并发读取基准测试
 * 
 * Tests scalability with multiple reader threads.
 * 测试多读取线程的可扩展性。
 */
static void BM_ConcurrentRead_FreqPartitionDict(benchmark::State& state) {
    const size_t N = 10000;
    const size_t num_threads = state.range(0);
    const size_t ops_per_thread = 10000;

    FreqPartitionDictThreadSafe<size_t, size_t> dict(128, 3);

    // Populate data / 填充数据
    for (size_t i = 1; i <= N; ++i) {
        dict.insert(i, i);
    }

    // Warmup / 预热
    ZipfGenerator warmup_zipf(N, 1.0, 42);
    for (size_t i = 0; i < 5000; ++i) {
        dict.get(warmup_zipf.next());
    }

    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<size_t> total_ops{0};

        auto start = std::chrono::high_resolution_clock::now();

        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                ZipfGenerator zipf(N, 1.0, 42 + t);
                for (size_t i = 0; i < ops_per_thread; ++i) {
                    dict.get(zipf.next());
                    ++total_ops;
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        state.SetIterationTime(elapsed / 1e9);
        state.counters["ops_per_sec"] = benchmark::Counter(
            total_ops.load(), benchmark::Counter::kIsRate);
    }

    state.SetItemsProcessed(state.iterations() * num_threads * ops_per_thread);
}
BENCHMARK(BM_ConcurrentRead_FreqPartitionDict)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->UseManualTime();

/**
 * @brief Concurrent read benchmark for std::unordered_map / std::unordered_map 并发读取基准测试
 */
static void BM_ConcurrentRead_UnorderedMap(benchmark::State& state) {
    const size_t N = 10000;
    const size_t num_threads = state.range(0);
    const size_t ops_per_thread = 10000;

    ThreadSafeUnorderedMap<size_t, size_t> map;

    // Populate data / 填充数据
    for (size_t i = 1; i <= N; ++i) {
        map.insert(i, i);
    }

    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<size_t> total_ops{0};

        auto start = std::chrono::high_resolution_clock::now();

        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                ZipfGenerator zipf(N, 1.0, 42 + t);
                for (size_t i = 0; i < ops_per_thread; ++i) {
                    map.get(zipf.next());
                    ++total_ops;
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        state.SetIterationTime(elapsed / 1e9);
        state.counters["ops_per_sec"] = benchmark::Counter(
            total_ops.load(), benchmark::Counter::kIsRate);
    }

    state.SetItemsProcessed(state.iterations() * num_threads * ops_per_thread);
}
BENCHMARK(BM_ConcurrentRead_UnorderedMap)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->UseManualTime();

/**
 * @brief Mixed read-write benchmark / 混合读写基准测试
 * 
 * Tests performance under different read-write ratios.
 * 测试不同读写比例下的性能。
 */
static void BM_MixedWorkload_FreqPartitionDict(benchmark::State& state) {
    const size_t N = 10000;
    const size_t num_threads = 4;
    const size_t ops_per_thread = 5000;
    const double read_ratio = state.range(0) / 100.0;  // 0.5, 0.8, 0.95

    FreqPartitionDictThreadSafe<size_t, size_t> dict(128, 3);

    // Populate initial data / 填充初始数据
    for (size_t i = 1; i <= N; ++i) {
        dict.insert(i, i);
    }

    for (auto _ : state) {
        std::vector<std::thread> threads;
        std::atomic<size_t> total_ops{0};

        auto start = std::chrono::high_resolution_clock::now();

        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                std::mt19937 rng(42 + t);
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                ZipfGenerator zipf(N, 1.0, 42 + t);
                std::uniform_int_distribution<size_t> key_dist(1, N * 2);

                for (size_t i = 0; i < ops_per_thread; ++i) {
                    if (dist(rng) < read_ratio) {
                        // Read operation / 读操作
                        dict.get(zipf.next());
                    } else {
                        // Write operation / 写操作
                        dict.insert(key_dist(rng), i);
                    }
                    ++total_ops;
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        state.SetIterationTime(elapsed / 1e9);
    }

    state.SetItemsProcessed(state.iterations() * num_threads * ops_per_thread);
    state.counters["read_ratio"] = read_ratio;
}
BENCHMARK(BM_MixedWorkload_FreqPartitionDict)
    ->Arg(50)   // 50% reads
    ->Arg(80)   // 80% reads
    ->Arg(95)   // 95% reads
    ->UseManualTime();

/**
 * @brief Scalability benchmark / 可扩展性基准测试
 * 
 * Measures how performance scales with increasing thread count.
 * 测量性能如何随线程数增加而扩展。
 */
static void BM_Scalability_FreqPartitionDict(benchmark::State& state) {
    const size_t N = 10000;
    const size_t num_threads = state.range(0);
    const size_t total_ops = 100000;
    const size_t ops_per_thread = total_ops / num_threads;

    FreqPartitionDictThreadSafe<size_t, size_t> dict(128, 3);

    // Populate data / 填充数据
    for (size_t i = 1; i <= N; ++i) {
        dict.insert(i, i);
    }

    for (auto _ : state) {
        std::vector<std::thread> threads;

        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                ZipfGenerator zipf(N, 1.0, 42 + t);
                for (size_t i = 0; i < ops_per_thread; ++i) {
                    dict.get(zipf.next());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }

    state.SetItemsProcessed(state.iterations() * total_ops);
    state.counters["threads"] = num_threads;
    state.counters["ops_per_thread"] = ops_per_thread;
}
BENCHMARK(BM_Scalability_FreqPartitionDict)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16);

/**
 * @brief Lock contention analysis / 锁竞争分析
 * 
 * Measures hit rate under concurrent access.
 * 测量并发访问下的命中率。
 */
static void BM_ConcurrentHitRate(benchmark::State& state) {
    const size_t N = 10000;
    const size_t num_threads = state.range(0);
    const size_t ops_per_thread = 10000;

    FreqPartitionDictThreadSafe<size_t, size_t> dict(128, 3);

    // Populate data / 填充数据
    for (size_t i = 1; i <= N; ++i) {
        dict.insert(i, i);
    }

    // Warmup / 预热
    ZipfGenerator warmup_zipf(N, 1.0, 42);
    for (size_t i = 0; i < 5000; ++i) {
        dict.get(warmup_zipf.next());
    }

    dict.reset_stats();

    for (auto _ : state) {
        std::vector<std::thread> threads;

        for (size_t t = 0; t < num_threads; ++t) {
            threads.emplace_back([&, t]() {
                ZipfGenerator zipf(N, 1.0, 42 + t);
                for (size_t i = 0; i < ops_per_thread; ++i) {
                    dict.get(zipf.next());
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }

    // Report hit rate / 报告命中率
    state.counters["hot_hit_rate"] = dict.hot_hit_rate();
    state.counters["total_hit_rate"] = dict.total_hit_rate();
    state.SetItemsProcessed(state.iterations() * num_threads * ops_per_thread);
}
BENCHMARK(BM_ConcurrentHitRate)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8);

BENCHMARK_MAIN();
