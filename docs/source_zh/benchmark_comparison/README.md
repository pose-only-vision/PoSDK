# Benchmark Comparison Tables

这个目录包含 PoSDK 与当前主流三维重建平台的对比测试数据和自动生成的表格。

## 📁 目录结构

```
benchmark_comparison/
├── data/                           # 原始 CSV 数据文件
│   ├── profiler_performance_summary.csv
│   ├── summary_GlobalPoses_rotation_error_deg_ALL_STATS.csv
│   ├── summary_GlobalPoses_translation_error_ALL_STATS.csv
│   └── summary_RelativePoses_rotation_error_deg_ALL_STATS.csv
├── processed/                      # 生成的 HTML 表格
│   ├── performance_comparison.html
│   ├── global_rotation_error.html
│   ├── global_translation_error.html
│   └── relative_rotation_error.html
├── scripts/                        # 表格生成脚本
│   └── generate_tables.py
├── index.md                        # 主文档
└── README.md                       # 本文件
```

## 🚀 使用方法

### 方式 1：使用 make 命令（推荐）

```bash
# 进入文档目录
cd /Users/caiqi/Documents/PoMVG/sync_docs_macos

# 方式 1a：只生成表格
make benchmark-tables

# 方式 1b：生成表格 + 构建文档（一步到位）
make html-with-tables
```

### 方式 2：直接运行 Python 脚本

```bash
# 进入 source 目录
cd /Users/caiqi/Documents/PoMVG/sync_docs_macos/source

# 运行脚本
python3 benchmark_comparison/scripts/generate_tables.py
```

## 📊 生成的表格说明

生成的 HTML 表格包含条件格式化：

- <span style="color: #d32f2f; font-weight: bold;">红色加粗</span>：同一数据集下的最优结果
- **黑色加粗**：同一数据集下的次优结果

### 表格列表

1. **performance_comparison.html**
   - 性能对比（总运行时间）
   - 越小越好（时间）

2. **global_rotation_error.html**
   - 全局位姿旋转误差
   - 越小越好（误差）

3. **global_translation_error.html**
   - 全局位姿平移误差
   - 越小越好（误差）

4. **relative_rotation_error.html**
   - 相对位姿旋转误差
   - 越小越好（误差）

## 🔄 更新测试数据

当有新的测试数据时，按以下步骤更新：

### 步骤 1：更新 CSV 数据

将新的 CSV 文件复制到 `data/` 目录：

```bash
cp /path/to/new/*.csv benchmark_comparison/data/
```

### 步骤 2：重新生成表格

```bash
# 使用 make 命令
make benchmark-tables

# 或直接运行脚本
python3 benchmark_comparison/scripts/generate_tables.py
```

### 步骤 3：构建文档

```bash
make html
```

### 一步完成

```bash
make html-with-tables
```

## 🎨 自定义样式

表格样式由 `_static/css/benchmark_tables.css` 控制。

### 修改颜色

编辑 CSS 文件中的颜色定义：

```css
/* 最优值：红色加粗 */
.benchmark-table .best-value {
    font-weight: 700;
    color: #d32f2f;  /* 修改这里的颜色 */
    background-color: rgba(211, 47, 47, 0.05);
}

/* 次优值：黑色加粗 */
.benchmark-table .second-best-value {
    font-weight: 700;
    color: #333;  /* 修改这里的颜色 */
    background-color: rgba(0, 0, 0, 0.02);
}
```

## 🔧 脚本说明

### generate_tables.py

**核心功能**：

1. 读取 CSV 数据文件
2. 按数据集分组比较各算法的指标
3. 自动找出最优和次优值
4. 生成带条件格式化的 HTML 表格

**主要类和方法**：

- `BenchmarkTableGenerator`: 主类
  - `read_csv()`: 读取 CSV 文件
  - `find_best_values()`: 找出最优/次优值
  - `format_value()`: 格式化数值（添加 CSS class）
  - `generate_performance_table()`: 生成性能对比表
  - `generate_accuracy_table()`: 生成精度对比表（通用）
  - `generate_all_tables()`: 生成所有表格

## 📝 注意事项

1. **CSV 格式要求**：
   - 必须包含表头行
   - 数据集名称列必须为 `Dataset` 或 `dataset`
   - 算法名称列必须为 `Algorithm` 或 `pipeline`

2. **算法名称映射**：
   - `openmvg_pipeline` → `OpenMVG`（自动转换）

3. **比较逻辑**：
   - 所有指标都是"越小越好"（时间、误差）
   - 在同一数据集内比较不同算法

4. **生成的 HTML 文件**：
   - 建议提交到 git（便于版本控制）
   - 不需要每次构建都重新生成

## 🐛 故障排查

### 问题：脚本运行失败

**解决方案**：
1. 检查 Python 版本（需要 3.6+）
2. 确认 CSV 文件存在且格式正确
3. 查看错误信息中的具体提示

### 问题：表格样式不生效

**解决方案**：
1. 确认 `_static/css/benchmark_tables.css` 存在
2. 检查 `conf.py` 中的 `html_css_files` 配置
3. 清除浏览器缓存或使用隐私模式查看

### 问题：表格中没有高亮

**解决方案**：
1. 检查 CSV 数据是否有效（数值型）
2. 确认至少有两个算法的数据
3. 查看生成的 HTML 源码是否有 `best-value` class

## 📚 参考资料

- [Sphinx 文档](https://www.sphinx-doc.org/)
- [MyST Parser](https://myst-parser.readthedocs.io/)
- [Read the Docs 主题](https://sphinx-rtd-theme.readthedocs.io/)

---

**维护者**: PoSDK Team
**最后更新**: 2025-01
