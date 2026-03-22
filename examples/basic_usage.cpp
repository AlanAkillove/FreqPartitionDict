#include <freq_partition_dict.hpp>
#include <iostream>
#include <string>

int main() {
    std::cout << "=== FreqPartitionDict 基础用法示例 ===" << std::endl;
    std::cout << std::endl;

    // 创建一个频率分区字典
    // 热区容量: 3, 晋升阈值: 2
    fpd::FreqPartitionDict<int, std::string> dict(3, 2);

    std::cout << "1. 插入一些数据..." << std::endl;
    dict.insert(1, "one");
    dict.insert(2, "two");
    dict.insert(3, "three");
    dict.insert(4, "four");
    dict.insert(5, "five");

    std::cout << "   字典大小: " << dict.size() << std::endl;
    std::cout << "   热区大小: " << dict.hot_size() << std::endl;
    std::cout << "   冷区大小: " << dict.cold_size() << std::endl;
    std::cout << std::endl;

    std::cout << "2. 访问数据（冷区元素需要访问 " << dict.promote_threshold() << " 次才会晋升）..." << std::endl;
    
    // 访问 key=1 (在冷区，需要访问2次才会晋升)
    for (int i = 0; i < 3; ++i) {
        auto result = dict.get(1);
        if (result) {
            std::cout << "   访问 key=1, 值=" << *result << ", 热区命中=" << dict.hot_hits() << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "3. 查看统计信息..." << std::endl;
    std::cout << "   热区命中次数: " << dict.hot_hits() << std::endl;
    std::cout << "   冷区命中次数: " << dict.cold_hits() << std::endl;
    std::cout << "   晋升次数: " << dict.promotions() << std::endl;
    std::cout << "   热区命中率: " << (dict.hot_hit_rate() * 100) << "%" << std::endl;
    std::cout << "   总命中率: " << (dict.total_hit_rate() * 100) << "%" << std::endl;

    std::cout << std::endl;
    std::cout << "4. 继续访问其他元素，观察淘汰机制..." << std::endl;
    
    // 访问其他元素多次，让它们晋升到热区
    for (int key : {2, 3, 4, 5}) {
        for (int i = 0; i < 3; ++i) {
            dict.get(key);
        }
    }

    std::cout << "   热区大小: " << dict.hot_size() << std::endl;
    std::cout << "   冷区大小: " << dict.cold_size() << std::endl;
    std::cout << "   晋升次数: " << dict.promotions() << std::endl;
    std::cout << "   降级次数: " << dict.demotions() << std::endl;

    std::cout << std::endl;
    std::cout << "5. 删除元素..." << std::endl;
    bool erased = dict.erase(1);
    std::cout << "   删除 key=1: " << (erased ? "成功" : "失败") << std::endl;
    std::cout << "   当前大小: " << dict.size() << std::endl;

    std::cout << std::endl;
    std::cout << "=== 示例结束 ===" << std::endl;

    return 0;
}
