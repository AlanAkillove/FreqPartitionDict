#include <benchmark/benchmark.h>
#include <freq_partition_dict.hpp>
#include <unordered_map>
#include <map>
#include <random>
#include <cmath>

using namespace fpd;

// Zipf 分布生成器
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

// 基准测试：不同数据结构查找性能对比
static void BM_UnorderedMap_Zipf(benchmark::State& state) {
    const size_t N = 10000;
    std::unordered_map<size_t, size_t> map;
    
    for (size_t i = 1; i <= N; ++i) {
        map[i] = i;
    }

    ZipfGenerator zipf(N, 1.0, 42);
    std::vector<size_t> keys;
    for (int i = 0; i < 1000; ++i) {
        keys.push_back(zipf.next());
    }

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find(keys[idx % keys.size()]));
        ++idx;
    }
}
BENCHMARK(BM_UnorderedMap_Zipf);

static void BM_Map_Zipf(benchmark::State& state) {
    const size_t N = 10000;
    std::map<size_t, size_t> map;
    
    for (size_t i = 1; i <= N; ++i) {
        map[i] = i;
    }

    ZipfGenerator zipf(N, 1.0, 42);
    std::vector<size_t> keys;
    for (int i = 0; i < 1000; ++i) {
        keys.push_back(zipf.next());
    }

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find(keys[idx % keys.size()]));
        ++idx;
    }
}
BENCHMARK(BM_Map_Zipf);

static void BM_FreqPartitionDict_Zipf(benchmark::State& state) {
    const size_t N = 10000;
    const size_t HOT_CAP = state.range(0);
    
    FreqPartitionDict<size_t, size_t> dict(HOT_CAP, 3);
    
    for (size_t i = 1; i <= N; ++i) {
        dict.insert(i, i);
    }

    // 预热
    ZipfGenerator warmup_zipf(N, 1.0, 42);
    for (int i = 0; i < 5000; ++i) {
        dict.get(warmup_zipf.next());
    }

    ZipfGenerator zipf(N, 1.0, 123);
    std::vector<size_t> keys;
    for (int i = 0; i < 1000; ++i) {
        keys.push_back(zipf.next());
    }

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(dict.get(keys[idx % keys.size()]));
        ++idx;
    }
}
BENCHMARK(BM_FreqPartitionDict_Zipf)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Arg(128);

// 基准测试：均匀随机访问
static void BM_UnorderedMap_Uniform(benchmark::State& state) {
    const size_t N = 10000;
    std::unordered_map<size_t, size_t> map;
    
    for (size_t i = 1; i <= N; ++i) {
        map[i] = i;
    }

    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(1, N);

    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find(dist(rng)));
    }
}
BENCHMARK(BM_UnorderedMap_Uniform);

static void BM_Map_Uniform(benchmark::State& state) {
    const size_t N = 10000;
    std::map<size_t, size_t> map;
    
    for (size_t i = 1; i <= N; ++i) {
        map[i] = i;
    }

    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(1, N);

    for (auto _ : state) {
        benchmark::DoNotOptimize(map.find(dist(rng)));
    }
}
BENCHMARK(BM_Map_Uniform);

static void BM_FreqPartitionDict_Uniform(benchmark::State& state) {
    const size_t N = 10000;
    FreqPartitionDict<size_t, size_t> dict(128, 3);
    
    for (size_t i = 1; i <= N; ++i) {
        dict.insert(i, i);
    }

    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(1, N);

    for (auto _ : state) {
        benchmark::DoNotOptimize(dict.get(dist(rng)));
    }
}
BENCHMARK(BM_FreqPartitionDict_Uniform);

// 基准测试：插入性能
static void BM_UnorderedMap_Insert(benchmark::State& state) {
    std::unordered_map<size_t, size_t> map;
    size_t i = 0;
    for (auto _ : state) {
        map[i] = i;
        ++i;
    }
}
BENCHMARK(BM_UnorderedMap_Insert);

static void BM_Map_Insert(benchmark::State& state) {
    std::map<size_t, size_t> map;
    size_t i = 0;
    for (auto _ : state) {
        map[i] = i;
        ++i;
    }
}
BENCHMARK(BM_Map_Insert);

static void BM_FreqPartitionDict_Insert(benchmark::State& state) {
    FreqPartitionDict<size_t, size_t> dict(128, 3);
    size_t i = 0;
    for (auto _ : state) {
        dict.insert(i, i);
        ++i;
    }
}
BENCHMARK(BM_FreqPartitionDict_Insert);

// 基准测试：不同 Zipf alpha 值
static void BM_FreqPartitionDict_ZipfAlpha(benchmark::State& state) {
    const size_t N = 10000;
    const double alpha = static_cast<double>(state.range(0)) / 10.0;
    
    FreqPartitionDict<size_t, size_t> dict(64, 3);
    
    for (size_t i = 1; i <= N; ++i) {
        dict.insert(i, i);
    }

    // 预热
    ZipfGenerator warmup_zipf(N, alpha, 42);
    for (int i = 0; i < 5000; ++i) {
        dict.get(warmup_zipf.next());
    }

    ZipfGenerator zipf(N, alpha, 123);
    std::vector<size_t> keys;
    for (int i = 0; i < 1000; ++i) {
        keys.push_back(zipf.next());
    }

    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(dict.get(keys[idx % keys.size()]));
        ++idx;
    }
}
BENCHMARK(BM_FreqPartitionDict_ZipfAlpha)
    ->Arg(5)   // alpha = 0.5
    ->Arg(8)   // alpha = 0.8
    ->Arg(10)  // alpha = 1.0
    ->Arg(12)  // alpha = 1.2
    ->Arg(15); // alpha = 1.5

BENCHMARK_MAIN();
