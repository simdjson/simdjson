#!/usr/bin/env python3
"""
Calculate statistics from ablation study results.

This script processes the CSV output from ablation_study.sh
and generates formatted statistical summaries.
"""

import sys
import csv
import os
from pathlib import Path

def read_csv_results(filename):
    """Read CSV results file and return data."""
    results = []

    try:
        with open(filename, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                results.append({
                    'variant': row['Variant'],
                    'mean': float(row['Mean_MB/s']),
                    'stdev': float(row['StdDev']),
                    'cv': float(row['CV%']),
                    'runs': int(row['Runs']),
                    'impact': float(row['Impact%']),
                    'compile_time': float(row['CompileTime_s'])
                })
    except FileNotFoundError:
        return None
    except Exception as e:
        print(f"Error reading {filename}: {e}")
        return None

    return results

def print_results_table(title, results):
    """Print formatted results table."""
    if not results:
        return

    print(f"\n{'='*80}")
    print(f"{title}")
    print(f"{'='*80}")

    # Print header
    print(f"\n{'Variant':<25} {'Mean (MB/s)':<12} {'Std Dev':<10} {'CV (%)':<8} {'Impact':<12} {'Compile (s)':<12}")
    print(f"{'-'*25} {'-'*12} {'-'*10} {'-'*8} {'-'*12} {'-'*12}")

    for result in results:
        variant_display = result['variant'].replace('_', ' ').title()
        if result['variant'] == 'baseline':
            variant_display = "**Baseline**"
            impact_str = "Reference"
        else:
            impact_str = f"{result['impact']:+.1f}%"

        print(f"{variant_display:<25} {result['mean']:<12.2f} ±{result['stdev']:<8.2f} "
              f"{result['cv']:<8.2f} {impact_str:<12} {result['compile_time']:<12.2f}")

def print_comparison_table(twitter_results, citm_results):
    """Print comparison table between Twitter and CITM results."""
    if not twitter_results or not citm_results:
        return

    print(f"\n{'='*80}")
    print("Performance Comparison: Twitter vs CITM")
    print(f"{'='*80}")

    print(f"\n{'Optimization':<25} {'Twitter Impact':<15} {'CITM Impact':<15} {'Difference':<20}")
    print(f"{'-'*25} {'-'*15} {'-'*15} {'-'*20}")

    # Create lookup dictionaries
    twitter_dict = {r['variant']: r for r in twitter_results}
    citm_dict = {r['variant']: r for r in citm_results}

    for variant in ['no_consteval', 'no_simd_escaping', 'no_fast_digits', 'no_branch_hints', 'linear_growth']:
        if variant in twitter_dict and variant in citm_dict:
            twitter_impact = twitter_dict[variant]['impact']
            citm_impact = citm_dict[variant]['impact']

            variant_display = variant.replace('_', ' ').title()
            diff_abs = abs(citm_impact - twitter_impact)

            if abs(twitter_impact) > 0.1:
                diff_factor = citm_impact / twitter_impact
                diff_str = f"{diff_factor:.1f}x"
            else:
                diff_str = "Different direction"

            print(f"{variant_display:<25} {twitter_impact:>+14.1f}% {citm_impact:>+14.1f}% {diff_str:<20}")

def print_summary_insights(twitter_results, citm_results):
    """Print summary insights from the ablation study."""
    print(f"\n{'='*80}")
    print("Key Insights")
    print(f"{'='*80}\n")

    if twitter_results and citm_results:
        # Find baseline performance
        twitter_baseline = next((r['mean'] for r in twitter_results if r['variant'] == 'baseline'), 0)
        citm_baseline = next((r['mean'] for r in citm_results if r['variant'] == 'baseline'), 0)

        print(f"1. Baseline Performance:")
        print(f"   - Twitter: {twitter_baseline:.2f} MB/s")
        print(f"   - CITM: {citm_baseline:.2f} MB/s")
        print(f"   - CITM is {((citm_baseline / twitter_baseline - 1) * 100):.1f}% slower than Twitter\n")

        # Find most impactful optimizations
        print(f"2. Most Impactful Optimizations:")

        all_impacts = []
        for r in twitter_results[1:]:  # Skip baseline
            all_impacts.append(('Twitter', r['variant'], r['impact']))
        for r in citm_results[1:]:  # Skip baseline
            all_impacts.append(('CITM', r['variant'], r['impact']))

        all_impacts.sort(key=lambda x: abs(x[2]), reverse=True)

        for i, (bench, variant, impact) in enumerate(all_impacts[:5]):
            variant_display = variant.replace('_', ' ').title()
            print(f"   {i+1}. {variant_display} on {bench}: {impact:+.1f}%")

        print(f"\n3. Variance Analysis:")
        twitter_cv = next((r['cv'] for r in twitter_results if r['variant'] == 'baseline'), 0)
        citm_cv = next((r['cv'] for r in citm_results if r['variant'] == 'baseline'), 0)
        print(f"   - Twitter baseline CV: {twitter_cv:.2f}%")
        print(f"   - CITM baseline CV: {citm_cv:.2f}%")
        print(f"   - CITM shows {citm_cv / twitter_cv:.1f}x higher variance than Twitter")

def main():
    # Default to ablation_results directory
    results_dir = "ablation_results"

    # Allow custom directory as argument
    if len(sys.argv) > 1:
        results_dir = sys.argv[1]

    # Check if directory exists
    if not os.path.exists(results_dir):
        print(f"Error: Results directory '{results_dir}' not found.")
        print("Please run ablation_study.sh first.")
        sys.exit(1)

    # Read results files
    twitter_file = os.path.join(results_dir, "twitter_ablation_results.csv")
    citm_file = os.path.join(results_dir, "citm_ablation_results.csv")

    twitter_results = read_csv_results(twitter_file)
    citm_results = read_csv_results(citm_file)

    if not twitter_results and not citm_results:
        print("No results found. Please run ablation_study.sh first.")
        sys.exit(1)

    # Print results
    if twitter_results:
        print_results_table("Twitter Benchmark Results", twitter_results)

    if citm_results:
        print_results_table("CITM Benchmark Results", citm_results)

    if twitter_results and citm_results:
        print_comparison_table(twitter_results, citm_results)
        print_summary_insights(twitter_results, citm_results)

    print(f"\n{'='*80}")
    print("Statistical Analysis Complete")
    print(f"{'='*80}")

if __name__ == "__main__":
    main()