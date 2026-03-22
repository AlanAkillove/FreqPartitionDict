#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Statistical Analysis for FreqPartitionDict Benchmarks
统计分析脚本 - 提供置信区间和显著性检验
"""

import json
import numpy as np
from scipy import stats
import matplotlib.pyplot as plt
from matplotlib import rcParams
import os

# 配置绘图风格
plt.style.use('seaborn-v0_8-paper')
rcParams['font.family'] = 'serif'
rcParams['font.serif'] = ['Times New Roman', 'DejaVu Serif']
rcParams['font.size'] = 10
rcParams['axes.labelsize'] = 11
rcParams['axes.titlesize'] = 12
rcParams['figure.dpi'] = 300
rcParams['savefig.dpi'] = 600

def load_raw_benchmark_data(filepath):
    """加载原始基准测试数据（非聚合数据）"""
    with open(filepath, 'r') as f:
        data = json.load(f)
    
    # 按测试名称分组原始数据
    raw_data = {}
    for bm in data['benchmarks']:
        name = bm['name']
        # 只保留原始迭代数据，排除聚合统计
        if bm['run_type'] == 'iteration' and 'mean' not in name and 'median' not in name and 'stddev' not in name and 'cv' not in name:
            base_name = name
            if base_name not in raw_data:
                raw_data[base_name] = []
            raw_data[base_name].append(bm['cpu_time'])
    
    return raw_data

def calculate_statistics(values, confidence=0.95):
    """计算统计指标和置信区间"""
    n = len(values)
    mean = np.mean(values)
    std = np.std(values, ddof=1)  # 样本标准差
    sem = stats.sem(values)  # 标准误
    
    # 计算置信区间
    ci = stats.t.interval(confidence, n-1, loc=mean, scale=sem)
    
    # 变异系数 (CV)
    cv = (std / mean) * 100 if mean != 0 else 0
    
    return {
        'n': n,
        'mean': mean,
        'std': std,
        'sem': sem,
        'ci_lower': ci[0],
        'ci_upper': ci[1],
        'cv': cv,
        'median': np.median(values),
        'min': np.min(values),
        'max': np.max(values)
    }

def perform_t_test(group1, group2):
    """执行独立样本t检验"""
    t_stat, p_value = stats.ttest_ind(group1, group2)
    return {
        't_statistic': t_stat,
        'p_value': p_value,
        'significant': p_value < 0.05
    }

def generate_statistical_report(raw_data, output_dir):
    """生成统计分析报告"""
    report = []
    report.append("=" * 80)
    report.append("Statistical Analysis Report - FreqPartitionDict Benchmarks")
    report.append("=" * 80)
    report.append("")
    
    # 存储统计结果
    stats_results = {}
    
    for name, values in sorted(raw_data.items()):
        if len(values) < 2:
            continue
            
        stats_dict = calculate_statistics(values)
        stats_results[name] = stats_dict
        
        report.append(f"\n{name}")
        report.append("-" * 60)
        report.append(f"  Sample size (n):        {stats_dict['n']}")
        report.append(f"  Mean:                   {stats_dict['mean']:.2f} ns")
        report.append(f"  Median:                 {stats_dict['median']:.2f} ns")
        report.append(f"  Std Dev:                {stats_dict['std']:.2f} ns")
        report.append(f"  Std Error:              {stats_dict['sem']:.2f} ns")
        report.append(f"  Coefficient of Variation: {stats_dict['cv']:.2f}%")
        report.append(f"  95% CI:                 [{stats_dict['ci_lower']:.2f}, {stats_dict['ci_upper']:.2f}] ns")
        report.append(f"  Range:                  [{stats_dict['min']:.2f}, {stats_dict['max']:.2f}] ns")
    
    # 显著性检验
    report.append("\n" + "=" * 80)
    report.append("Significance Tests (α = 0.05)")
    report.append("=" * 80)
    
    # FreqPartitionDict vs std::unordered_map at α=1.5
    if 'BM_FreqPartitionDict_ZipfAlpha/15' in raw_data and 'BM_UnorderedMap_Zipf' in raw_data:
        fpd_values = raw_data['BM_FreqPartitionDict_ZipfAlpha/15']
        hash_values = raw_data['BM_UnorderedMap_Zipf']
        t_test = perform_t_test(fpd_values, hash_values)
        
        report.append(f"\nFreqPartitionDict (α=1.5) vs std::unordered_map:")
        report.append(f"  t-statistic: {t_test['t_statistic']:.4f}")
        report.append(f"  p-value: {t_test['p_value']:.6f}")
        report.append(f"  Significant difference: {'Yes' if t_test['significant'] else 'No'}")
    
    # 保存报告
    report_path = os.path.join(output_dir, 'statistical_report.txt')
    with open(report_path, 'w') as f:
        f.write('\n'.join(report))
    
    print(f"Statistical report saved to: {report_path}")
    return stats_results

def plot_with_confidence_intervals(stats_results, output_dir):
    """绘制带置信区间的图表"""
    
    # 图1: Zipf对比（带误差条）
    fig, ax = plt.subplots(figsize=(8, 6))
    
    zipf_alphas = []
    means = []
    ci_lowers = []
    ci_uppers = []
    
    for alpha in [0.5, 0.8, 1.0, 1.2, 1.5]:
        name = f'BM_FreqPartitionDict_ZipfAlpha/{int(alpha*10)}'
        if name in stats_results:
            stats_dict = stats_results[name]
            zipf_alphas.append(alpha)
            means.append(stats_dict['mean'])
            ci_lowers.append(stats_dict['mean'] - stats_dict['ci_lower'])
            ci_uppers.append(stats_dict['ci_upper'] - stats_dict['mean'])
    
    # 绘制带置信区间的曲线
    ax.errorbar(zipf_alphas, means, yerr=[ci_lowers, ci_uppers], 
                fmt='o-', linewidth=2, markersize=8, capsize=5, capthick=2,
                color='#2E5C8A', label='FreqPartitionDict (95% CI)')
    
    # 添加参考线
    if 'BM_UnorderedMap_Zipf' in stats_results:
        hash_mean = stats_results['BM_UnorderedMap_Zipf']['mean']
        ax.axhline(y=hash_mean, color='#E74C3C', linestyle='--', linewidth=1.5,
                   label=f'std::unordered_map ({hash_mean:.1f} ns)')
    
    if 'BM_Map_Zipf' in stats_results:
        tree_mean = stats_results['BM_Map_Zipf']['mean']
        ax.axhline(y=tree_mean, color='#27AE60', linestyle='--', linewidth=1.5,
                   label=f'std::map ({tree_mean:.1f} ns)')
    
    ax.set_xlabel('Zipf α (skewness parameter)', fontweight='bold')
    ax.set_ylabel('Lookup latency (ns)', fontweight='bold')
    ax.set_title('Impact of Access Skewness on Lookup Performance\n(with 95% Confidence Intervals)', 
                 fontweight='bold', pad=15)
    ax.legend(loc='upper right', frameon=True, fancybox=False, 
              edgecolor='black', framealpha=0.9)
    ax.grid(True, alpha=0.3, linestyle='-', linewidth=0.5)
    ax.set_xlim(0.4, 1.6)
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'fig1_zipf_with_ci.pdf'), format='pdf', dpi=600)
    plt.savefig(os.path.join(output_dir, 'fig1_zipf_with_ci.png'), format='png', dpi=600)
    plt.close()
    print("Figure with confidence intervals saved")

def main():
    # 创建输出目录
    output_dir = os.path.join(os.path.dirname(__file__), '..', 'results', 'figures')
    os.makedirs(output_dir, exist_ok=True)
    
    # 加载数据
    data_file = os.path.join(os.path.dirname(__file__), '..', 'benchmark_results.json')
    
    if not os.path.exists(data_file):
        print(f"Error: {data_file} not found. Run benchmarks first.")
        return
    
    print("Loading benchmark data...")
    raw_data = load_raw_benchmark_data(data_file)
    
    print(f"Found {len(raw_data)} benchmark tests")
    
    # 生成统计报告
    print("\nGenerating statistical analysis...")
    stats_results = generate_statistical_report(raw_data, output_dir)
    
    # 生成带置信区间的图表
    print("\nGenerating plots with confidence intervals...")
    plot_with_confidence_intervals(stats_results, output_dir)
    
    print("\n" + "=" * 60)
    print("Statistical analysis complete!")
    print(f"Results saved to: {output_dir}")
    print("=" * 60)

if __name__ == '__main__':
    main()
