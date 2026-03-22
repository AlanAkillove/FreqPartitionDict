#include <freq_partition_dict.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <iomanip>

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
        // 计算归一化常数
        double sum = 0.0;
        for (size_t i = 1; i <= n; ++i) {
            sum += 1.0 / std::pow(i, alpha);
        }

        // 构建 CDF
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

void run_zipf_demo(size_t n_keys, size_t n_operations, double alpha, size_t hot_capacity) {
    std::cout << "=== Zipf 分布性能测试 ===" << std::endl;
    std::cout << "参数:" << std::endl;
    std::cout << "  键数量: " << n_keys << std::endl;
    std::cout << "  操作次数: " << n_operations << std::endl;
    std::cout << "  Zipf alpha: " << alpha << std::endl;
    std::cout << "  热区容量: " << hot_capacity << std::endl;
    std::cout << std::endl;

    // 创建字典
    fpd::FreqPartitionDict<size_t, std::string> dict(hot_capacity, 3);

    // 插入数据
    for (size_t i = 1; i <= n_keys; ++i) {
        dict.insert(i, "value_" + std::to_string(i));
    }

    // 创建 Zipf 生成器
    ZipfGenerator zipf(n_keys, alpha, 42);

    // 预热阶段
    std::cout << "预热阶段 (前 10000 次访问)..." << std::endl;
    for (size_t i = 0; i < 10000; ++i) {
        size_t key = zipf.next();
        dict.get(key);
    }

    dict.reset_stats();

    // 正式测试
    std::cout << "正式测试阶段..." << std::endl;
    for (size_t i = 0; i < n_operations; ++i) {
        size_t key = zipf.next();
        dict.get(key);
    }

    // 输出结果
    std::cout << std::endl;
    std::cout << "=== 测试结果 ===" << std::endl;
    std::cout << "热区命中次数: " << dict.hot_hits() << std::endl;
    std::cout << "冷区命中次数: " << dict.cold_hits() << std::endl;
    std::cout << "未命中次数: " << dict.misses() << std::endl;
    std::cout << "热区命中率: " << std::fixed << std::setprecision(2) 
              << (dict.hot_hit_rate() * 100) << "%" << std::endl;
    std::cout << "总命中率: " << std::fixed << std::setprecision(2) 
              << (dict.total_hit_rate() * 100) << "%" << std::endl;
    std::cout << "晋升次数: " << dict.promotions() << std::endl;
    std::cout << "降级次数: " << dict.demotions() << std::endl;
    std::cout << std::endl;
}

void compare_hot_capacity(size_t n_keys, size_t n_operations, double alpha) {
    std::cout << "=== 不同热区容量对比 ===" << std::endl;
    std::cout << "键数量: " << n_keys << ", 操作次数: " << n_operations 
              << ", alpha: " << alpha << std::endl;
    std::cout << std::endl;

    std::vector<size_t> capacities = {8, 16, 32, 64, 128, 256};

    std::cout << std::setw(15) << "热区容量" 
              << std::setw(15) << "热区命中率" 
              << std::setw(15) << "总命中率"
              << std::setw(15) << "晋升次数" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    for (size_t cap : capacities) {
        fpd::FreqPartitionDict<size_t, std::string> dict(cap, 3);

        for (size_t i = 1; i <= n_keys; ++i) {
            dict.insert(i, "value_" + std::to_string(i));
        }

        ZipfGenerator zipf(n_keys, alpha, 42);

        // 预热
        for (size_t i = 0; i < 10000; ++i) {
            dict.get(zipf.next());
        }

        dict.reset_stats();

        // 测试
        for (size_t i = 0; i < n_operations; ++i) {
            dict.get(zipf.next());
        }

        std::cout << std::setw(15) << cap
                  << std::setw(14) << std::fixed << std::setprecision(2) 
                  << (dict.hot_hit_rate() * 100) << "%"
                  << std::setw(14) << std::fixed << std::setprecision(2) 
                  << (dict.total_hit_rate() * 100) << "%"
                  << std::setw(15) << dict.promotions() << std::endl;
    }
}

int main() {
    // 场景1: 高倾斜度 (alpha=1.2)
    run_zipf_demo(1000, 100000, 1.2, 32);

    std::cout << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << std::endl;

    // 场景2: 中等倾斜度 (alpha=0.8)
    run_zipf_demo(1000, 100000, 0.8, 32);

    std::cout << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << std::endl;

    // 对比不同热区容量
    compare_hot_capacity(1000, 100000, 1.0);

    return 0;
}
