#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Benchmark Comparison Table Generator
生成基准测试对比表格

This script processes CSV benchmark data and generates HTML tables with
conditional formatting (highlighting best and second-best results).
此脚本处理 CSV 基准测试数据并生成带条件格式化的 HTML 表格（高亮最优和次优结果）。

Author: PoSDK Team
Date: 2025
"""

import csv
import os
from pathlib import Path
from typing import List, Dict, Tuple, Any


class BenchmarkTableGenerator:
    """Benchmark table generator with conditional formatting"""

    def __init__(self, data_dir: str, output_dir: str):
        """
        Initialize generator

        Args:
            data_dir: Directory containing CSV data files
            output_dir: Directory for generated HTML files
        """
        self.data_dir = Path(data_dir)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def read_csv(self, filename: str) -> List[Dict[str, str]]:
        """
        Read CSV file and return list of dictionaries | 读取CSV文件并返回字典列表

        Args:
            filename: CSV filename | CSV文件名

        Returns:
            List of row dictionaries | 行字典列表
        """
        csv_path = self.data_dir / filename
        rows = []

        with open(csv_path, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                # Remove EvalCommit column if exists (not needed for display)
                # 如果存在EvalCommit列则移除（显示时不需要）
                if 'EvalCommit' in row:
                    del row['EvalCommit']
                rows.append(row)

        return rows

    def find_best_values(self, rows: List[Dict], group_key: str,
                        value_keys: List[str], lower_is_better: bool = True) -> Dict[str, Dict[str, Tuple[Any, Any]]]:
        """
        Find best and second-best values for each group

        Args:
            rows: List of data rows
            group_key: Key to group by (e.g., 'Dataset')
            value_keys: Keys of values to compare (e.g., ['Mean', 'Median'])
            lower_is_better: True if lower values are better (default for errors/time)

        Returns:
            Dictionary mapping group -> value_key -> (best_value, second_best_value)
        """
        # Group rows by group_key
        groups = {}
        for row in rows:
            group = row[group_key]
            if group not in groups:
                groups[group] = []
            groups[group].append(row)

        # Find best and second-best for each group and value_key
        best_values = {}
        for group, group_rows in groups.items():
            best_values[group] = {}

            for value_key in value_keys:
                # Extract values (skip empty or non-numeric)
                values = []
                for row in group_rows:
                    try:
                        val = float(row[value_key])
                        values.append((val, row))
                    except (ValueError, KeyError):
                        continue

                if len(values) < 2:
                    continue

                # Sort based on lower_is_better
                values.sort(key=lambda x: x[0], reverse=not lower_is_better)

                # Best and second-best
                best = values[0][0]
                second_best = values[1][0] if len(values) > 1 else None

                best_values[group][value_key] = (best, second_best)

        return best_values

    def format_number(self, value: float, decimal_places: int = 4) -> str:
        """
        Format number with specified decimal places | 格式化数字为指定小数位数

        Args:
            value: Number to format | 待格式化的数字
            decimal_places: Number of decimal places (default 4) | 小数位数（默认4）

        Returns:
            Formatted string | 格式化后的字符串
        """
        if value == 0:
            return "0.0000"

        # For large numbers (>= 10000), use comma separators
        # 对于大数字(>= 10000)，使用逗号分隔符
        if abs(value) >= 10000:
            # Format with comma separators and specified decimal places
            # 使用逗号分隔符和指定小数位数格式化
            return f"{value:,.{decimal_places}f}"

        # For small numbers (< 10000), use fixed decimal places
        # 对于小数字(< 10000)，使用固定小数位数
        return f"{value:.{decimal_places}f}"

    def format_value(self, value: str, best_info: Tuple[Any, Any],
                    is_numeric: bool = True, as_integer: bool = False) -> str:
        """
        Format value with conditional CSS class | 格式化数值并添加条件CSS类

        Args:
            value: Value to format | 待格式化的值
            best_info: Tuple of (best_value, second_best_value) | (最优值, 次优值)元组
            is_numeric: Whether the value is numeric | 是否为数值型
            as_integer: Format as integer (for time values) | 格式化为整数（用于时间值）

        Returns:
            HTML string with appropriate class | 带有相应CSS类的HTML字符串
        """
        if not is_numeric or not best_info:
            return f'<td>{value}</td>'

        try:
            val = float(value)
            best, second_best = best_info

            # Format value: as integer or with 4 decimal places
            # 格式化数值：整数或4位小数
            if as_integer:
                # Round to nearest integer and add comma separators
                # 四舍五入为整数并添加千位分隔符
                formatted_val = f"{int(round(val)):,}"
            else:
                formatted_val = self.format_number(val, decimal_places=4)

            # Check if this is best or second-best
            # 检查是否为最优或次优值
            if abs(val - best) < 1e-6:  # Float comparison tolerance | 浮点数比较容差
                return f'<td class="best-value">{formatted_val}</td>'
            elif second_best and abs(val - second_best) < 1e-6:
                return f'<td class="second-best-value">{formatted_val}</td>'
            else:
                return f'<td>{formatted_val}</td>'
        except ValueError:
            return f'<td>{value}</td>'

    def generate_performance_table(self):
        """Generate performance comparison table (Total Time)"""
        print("Generating performance comparison table...")

        rows = self.read_csv('profiler_performance_summary.csv')

        # Find best values (lower time is better)
        best_values = self.find_best_values(
            rows,
            group_key='dataset',
            value_keys=['Total Time(ms)'],
            lower_is_better=True
        )

        # Group by dataset
        datasets = {}
        for row in rows:
            dataset = row['dataset']
            if dataset not in datasets:
                datasets[dataset] = []
            datasets[dataset].append(row)

        # Generate HTML
        html = '''
<div class="benchmark-table-container">
<table class="benchmark-table">
<thead>
<tr>
    <th>数据集<br>Dataset</th>
    <th>PoSDK (ms)</th>
    <th>OpenMVG (ms)</th>
    <th>COLMAP (ms)</th>
    <th>GLOMAP (ms)</th>
</tr>
</thead>
<tbody>
'''

        dataset_list = sorted(datasets.keys())
        for dataset_idx, dataset in enumerate(dataset_list):
            # Find times for each pipeline
            posdk_time = None
            openmvg_time = None
            colmap_time = None
            glomap_time = None

            for row in datasets[dataset]:
                pipeline = row['pipeline']
                time_ms = float(row['Total Time(ms)'])

                if pipeline == 'PoSDK':
                    posdk_time = time_ms
                elif pipeline == 'OpenMVG' or pipeline == 'openmvg_pipeline':
                    openmvg_time = time_ms
                elif pipeline == 'COLMAP' or pipeline == 'colmap_pipeline':
                    colmap_time = time_ms
                elif pipeline == 'GLOMAP' or pipeline == 'glomap_pipeline':
                    glomap_time = time_ms

            # Skip if PoSDK time is missing (need at least PoSDK for comparison)
            if posdk_time is None:
                continue

            # Get best info for this dataset
            best_info = best_values.get(dataset, {}).get('Total Time(ms)', (None, None))

            # Build row classes
            # 构建行class属性
            row_classes = []

            # Add dataset group class for color alternation
            # 添加数据集分组类用于颜色交替
            row_classes.append(f'dataset-group-{dataset_idx % 2}')

            # Add dataset separator class for first row of each dataset (except first dataset)
            # 为每个数据集的第一行添加分隔符类（除第一个数据集外）
            if dataset_idx > 0:
                row_classes.append('dataset-separator')

            row_class = f' class="{" ".join(row_classes)}"' if row_classes else ''

            # Format values as integers with comma separators (only if available)
            # 使用整数格式和千位分隔符格式化数值（仅当可用时）
            posdk_cell = self.format_value(str(posdk_time), best_info, as_integer=True) if posdk_time else '<td>-</td>'
            openmvg_cell = self.format_value(str(openmvg_time), best_info, as_integer=True) if openmvg_time else '<td>-</td>'
            colmap_cell = self.format_value(str(colmap_time), best_info, as_integer=True) if colmap_time else '<td>-</td>'
            glomap_cell = self.format_value(str(glomap_time), best_info, as_integer=True) if glomap_time else '<td>-</td>'

            html += f'''<tr{row_class}>
    <td>{dataset}</td>
    {posdk_cell}
    {openmvg_cell}
    {colmap_cell}
    {glomap_cell}
</tr>
'''

        html += '''</tbody>
</table>
</div>
'''

        # Save to file
        output_file = self.output_dir / 'performance_comparison.html'
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(html)

        print(f"  ✓ Saved to {output_file}")

    def generate_accuracy_table(self, csv_filename: str, output_filename: str,
                                title: str, metric_columns: List[str]):
        """
        Generate accuracy comparison table (generic for rotation/translation errors)

        Args:
            csv_filename: Input CSV filename
            output_filename: Output HTML filename
            title: Table title
            metric_columns: List of metric column names to compare
        """
        print(f"Generating {title} table...")

        rows = self.read_csv(csv_filename)

        # Find best values (lower error is better)
        best_values = self.find_best_values(
            rows,
            group_key='Dataset',
            value_keys=metric_columns,
            lower_is_better=True
        )

        # Generate HTML (without title h3 tag)
        # 生成HTML（不包含标题h3标签）
        html = '''
<div class="benchmark-table-container">
<table class="benchmark-table">
<thead>
<tr>
    <th>数据集<br>Dataset</th>
    <th>算法<br>Algorithm</th>
'''

        # Add column headers
        column_names = {
            'Mean': '均值<br>Mean',
            'Median': '中位数<br>Median',
            'Min': '最小值<br>Min',
            'Max': '最大值<br>Max',
            'StdDev': '标准差<br>StdDev'
        }

        for col in metric_columns:
            col_name = column_names.get(col, col)
            html += f'    <th>{col_name}</th>\n'

        html += '''</tr>
</thead>
<tbody>
'''

        # Group by dataset
        datasets = {}
        for row in rows:
            dataset = row['Dataset']
            if dataset not in datasets:
                datasets[dataset] = []
            datasets[dataset].append(row)

        # Generate rows
        dataset_list = sorted(datasets.keys())
        for dataset_idx, dataset in enumerate(dataset_list):
            dataset_rows = datasets[dataset]

            # Sort by algorithm (PoSDK first)
            dataset_rows.sort(key=lambda r: (r['Algorithm'] != 'PoSDK', r['Algorithm']))

            for row_idx, row in enumerate(dataset_rows):
                algorithm = row['Algorithm']

                # Map algorithm names
                # 算法名称映射
                algorithm_name_map = {
                    'openmvg_pipeline': 'OpenMVG',
                    'colmap_pipeline': 'COLMAP',
                    'glomap_pipeline': 'GLOMAP'
                }
                algorithm = algorithm_name_map.get(algorithm, algorithm)

                # Build row classes
                # 构建行class属性
                row_classes = []

                # Add dataset group class for color alternation
                # 添加数据集分组类用于颜色交替
                row_classes.append(f'dataset-group-{dataset_idx % 2}')

                # Add dataset separator class for the first row of each dataset (except the first dataset)
                # 为每个数据集的第一行添加分隔符类（除第一个数据集外）
                if row_idx == 0 and dataset_idx > 0:
                    row_classes.append('dataset-separator')

                row_class = f' class="{" ".join(row_classes)}"' if row_classes else ''

                html += f'''<tr{row_class}>
    <td>{dataset}</td>
    <td>{algorithm}</td>
'''

                # Add metric values with formatting
                # Only highlight Mean and Median columns, but format all columns with 4 decimal places
                highlight_columns = ['Mean', 'Median']

                for col in metric_columns:
                    value = row.get(col, '')

                    # Only apply highlighting to Mean and Median
                    if col in highlight_columns:
                        best_info = best_values.get(dataset, {}).get(col, (None, None))
                        html += '    ' + self.format_value(value, best_info) + '\n'
                    else:
                        # Other columns: format with 4 decimal places but no highlighting
                        try:
                            val = float(value)
                            formatted_val = self.format_number(val, decimal_places=4)
                            html += f'    <td>{formatted_val}</td>\n'
                        except (ValueError, TypeError):
                            html += f'    <td>{value}</td>\n'

                html += '</tr>\n'

        html += '''</tbody>
</table>
</div>
'''

        # Save to file
        output_file = self.output_dir / output_filename
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(html)

        print(f"  ✓ Saved to {output_file}")

    def generate_all_tables(self):
        """Generate all benchmark comparison tables"""
        print("\n" + "="*60)
        print("Benchmark Comparison Table Generator")
        print("基准测试对比表格生成器")
        print("="*60 + "\n")

        # 1. Performance comparison
        self.generate_performance_table()

        # 2. Global pose rotation error
        self.generate_accuracy_table(
            'summary_GlobalPoses_rotation_error_deg_ALL_STATS.csv',
            'global_rotation_error.html',
            '全局位姿旋转误差 (度) | Global Pose Rotation Error (deg)',
            ['Mean', 'Median', 'Min', 'Max', 'StdDev']
        )

        # 3. Global pose translation error
        self.generate_accuracy_table(
            'summary_GlobalPoses_translation_error_ALL_STATS.csv',
            'global_translation_error.html',
            '全局位姿平移误差 (归一化) | Global Pose Translation Error (normalized)',
            ['Mean', 'Median', 'Min', 'Max', 'StdDev']
        )

        # 4. Relative pose rotation error
        self.generate_accuracy_table(
            'summary_RelativePoses_rotation_error_deg_ALL_STATS.csv',
            'relative_rotation_error.html',
            '相对位姿旋转误差 (度) | Relative Pose Rotation Error (deg)',
            ['Mean', 'Median', 'Min', 'Max', 'StdDev']
        )

        print("\n" + "="*60)
        print("All tables generated successfully! ✓")
        print("所有表格生成成功！✓")
        print("="*60 + "\n")


def main():
    """Main function"""
    # Get script directory
    script_dir = Path(__file__).parent
    source_dir = script_dir.parent.parent

    # Set paths
    data_dir = script_dir.parent / 'data'
    output_dir = script_dir.parent / 'processed'

    # Generate tables
    generator = BenchmarkTableGenerator(str(data_dir), str(output_dir))
    generator.generate_all_tables()


if __name__ == '__main__':
    main()
