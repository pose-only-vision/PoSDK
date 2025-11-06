#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Benchmark Comparison Table Generator

This script processes CSV benchmark data and generates HTML tables with
conditional formatting (highlighting best and second-best results).

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
        Read CSV file and return list of dictionaries

        Args:
            filename: CSV filename

        Returns:
            List of row dictionaries
        """
        csv_path = self.data_dir / filename
        rows = []

        with open(csv_path, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                # Remove EvalCommit column if exists (not needed for display)
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
        Format number with specified decimal places

        Args:
            value: Number to format
            decimal_places: Number of decimal places (default 4)

        Returns:
            Formatted string
        """
        if value == 0:
            return "0.0000"

        # For large numbers (>= 10000), use comma separators
        if abs(value) >= 10000:
            # Format with comma separators and specified decimal places
            return f"{value:,.{decimal_places}f}"

        # For small numbers (< 10000), use fixed decimal places
        return f"{value:.{decimal_places}f}"

    def format_value(self, value: str, best_info: Tuple[Any, Any],
                    is_numeric: bool = True, as_integer: bool = False) -> str:
        """
        Format value with conditional CSS class

        Args:
            value: Value to format
            best_info: Tuple of (best_value, second_best_value)
            is_numeric: Whether the value is numeric
            as_integer: Format as integer (for time values)

        Returns:
            HTML string with appropriate class
        """
        if not is_numeric or not best_info:
            return f'<td>{value}</td>'

        try:
            val = float(value)
            best, second_best = best_info

            # Format value: as integer or with 4 decimal places
            if as_integer:
                # Round to nearest integer and add comma separators
                formatted_val = f"{int(round(val)):,}"
            else:
                formatted_val = self.format_number(val, decimal_places=4)

            # Check if this is best or second-best
            if abs(val - best) < 1e-6:  # Float comparison tolerance
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
    <th>Dataset</th>
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

                # Use case-insensitive comparison for pipeline names
                pipeline_lower = pipeline.lower()
                
                if pipeline == 'PoSDK':
                    posdk_time = time_ms
                elif pipeline == 'OpenMVG' or pipeline == 'openmvg_pipeline' or pipeline_lower == 'openmvg':
                    openmvg_time = time_ms
                elif pipeline == 'COLMAP' or pipeline == 'colmap_pipeline' or pipeline_lower == 'colmap':
                    colmap_time = time_ms
                elif pipeline_lower in ['glomap', 'glomap_pipeline']:
                    # Handles "GLOMAP", "GlOMAP", "glomap", "glomap_pipeline", etc.
                    glomap_time = time_ms

            # Skip if PoSDK time is missing (need at least PoSDK for comparison)
            if posdk_time is None:
                continue

            # Get best info for this dataset
            best_info = best_values.get(dataset, {}).get('Total Time(ms)', (None, None))

            # Build row classes
            row_classes = []

            # Add dataset group class for color alternation
            row_classes.append(f'dataset-group-{dataset_idx % 2}')

            # Add dataset separator class for first row of each dataset (except first dataset)
            if dataset_idx > 0:
                row_classes.append('dataset-separator')

            row_class = f' class="{" ".join(row_classes)}"' if row_classes else ''

            # Format values as integers with comma separators (only if available)
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
        html = '''
<div class="benchmark-table-container">
<table class="benchmark-table">
<thead>
<tr>
    <th>Dataset</th>
    <th>Algorithm</th>
'''

        # Add column headers
        column_names = {
            'Mean': 'Mean',
            'Median': 'Median',
            'Min': 'Min',
            'Max': 'Max',
            'StdDev': 'StdDev'
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

                # Map algorithm names (case-insensitive)
                algorithm_lower = algorithm.lower()
                algorithm_name_map = {
                    'openmvg_pipeline': 'OpenMVG',
                    'openmvg': 'OpenMVG',
                    'colmap_pipeline': 'COLMAP',
                    'colmap': 'COLMAP',
                    'glomap_pipeline': 'GLOMAP',
                    'glomap': 'GLOMAP'  # Handles "GlOMAP", "GLOMAP", "glomap", etc.
                }
                # Try exact match first, then case-insensitive match
                algorithm = algorithm_name_map.get(algorithm, 
                    algorithm_name_map.get(algorithm_lower, algorithm))

                # Build row classes
                row_classes = []

                # Add dataset group class for color alternation
                row_classes.append(f'dataset-group-{dataset_idx % 2}')

                # Add dataset separator class for the first row of each dataset (except the first dataset)
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
        print("="*60 + "\n")

        # 1. Performance comparison
        self.generate_performance_table()

        # 2. Global pose rotation error
        self.generate_accuracy_table(
            'summary_GlobalPoses_rotation_error_deg_ALL_STATS.csv',
            'global_rotation_error.html',
            'Global Pose Rotation Error (deg)',
            ['Mean', 'Median', 'Min', 'Max', 'StdDev']
        )

        # 3. Global pose translation error
        self.generate_accuracy_table(
            'summary_GlobalPoses_translation_error_ALL_STATS.csv',
            'global_translation_error.html',
            'Global Pose Translation Error (normalized)',
            ['Mean', 'Median', 'Min', 'Max', 'StdDev']
        )

        # 4. Relative pose rotation error
        self.generate_accuracy_table(
            'summary_RelativePoses_rotation_error_deg_ALL_STATS.csv',
            'relative_rotation_error.html',
            'Relative Pose Rotation Error (deg)',
            ['Mean', 'Median', 'Min', 'Max', 'StdDev']
        )

        print("\n" + "="*60)
        print("All tables generated successfully! ✓")
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
