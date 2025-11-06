# PoSDK GlobalSfM Pipeline 可执行程序

这是基于PoSDK的GlobalSfM重建流水线的命令行可执行程序。

## 编译方法

### 前置条件
- 已安装CMake (版本 >= 3.16)
- 已安装gflags库
- 已编译po_core库

### 编译步骤

由于PoSDK可执行程序已集成到主项目中，需要从主项目根目录进行编译：

```bash
cd /Users/caiqi/Documents/PoMVG/src
mkdir -p build && cd build
cmake ..
make
```

编译成功后，可执行文件将位于 `build/output/bin/PoSDK`

## 使用方法

### 基本用法

PoSDK支持两种运行模式：

#### 1. Default模式（推荐）- 使用配置文件默认参数
```bash
# 直接运行，使用globalsfm_pipeline.ini中的默认配置
./build/output/bin/PoSDK

# 或者显式指定default模式
./build/output/bin/PoSDK --preset=default
```

#### 2. Custom模式 - 使用命令行自定义参数

**单个数据集处理：**
```bash
./build/output/bin/PoSDK --preset=custom --dataset_dir=/path/to/strecha --image_folder=/path/to/strecha/castle-P19/images
```

**批处理多个数据集：**
```bash
./build/output/bin/PoSDK --preset=custom --dataset_dir=/path/to/strecha
```

### 命令行参数

#### 预设模式参数
- `--preset`: 参数预设模式（default, custom）
  - `default`: 使用配置文件默认参数，无需其他参数
  - `custom`: 使用命令行自定义参数，需要指定数据集路径

#### Custom模式必需参数
- `--dataset_dir`: 数据集根目录路径

#### Custom模式可选参数
- `--image_folder`: 图像文件夹路径（不指定则批处理dataset_dir中的所有数据集）
- `--preprocess_type`: 预处理类型，支持 `openmvg`, `posdk`, `colmap`, `glomap`（默认: `posdk`）
- `--work_dir`: 工作目录（默认: 当前目录/globalsfm_pipeline_work）
- `--enable_evaluation`: 是否启用精度评估（默认: true）
- `--max_iterations`: 迭代优化最大次数（默认: 5）
- `--enable_summary_table`: 是否启用统一制表功能（默认: false）
- `--enable_profiling`: 是否启用性能分析（默认: true）
- `--enable_csv_export`: 是否启用评估结果CSV导出（默认: true）
- `--evaluation_print_mode`: 评估结果打印模式，支持 `none`, `summary`, `detailed`, `comparison`（默认: `summary`）
- `--compared_pipelines`: 对比流水线列表，逗号分隔，如 `openmvg,colmap,glomap`
- `--log_level`: 日志级别（默认: 0）

### 使用示例

#### 1. Default模式（推荐）- 零配置运行
```bash
# 最简单的运行方式，使用配置文件中的所有默认参数
./build/output/bin/PoSDK

# 等价于显式指定default模式
./build/output/bin/PoSDK --preset=default
```

#### 2. Custom模式 - 基础单数据集重建
```bash
./build/output/bin/PoSDK --preset=custom \
        --dataset_dir=/Users/caiqi/Documents/PoMVG/tests/Strecha \
        --image_folder=/Users/caiqi/Documents/PoMVG/tests/Strecha/castle-P19/images
```

#### 3. Custom模式 - 批处理所有数据集并生成汇总表格
```bash
./build/output/bin/PoSDK --preset=custom \
        --dataset_dir=/Users/caiqi/Documents/PoMVG/tests/Strecha \
        --enable_summary_table=true
```

#### 4. Custom模式 - 使用OpenMVG预处理并对比多个流水线
```bash
./build/output/bin/PoSDK --preset=custom \
        --dataset_dir=/Users/caiqi/Documents/PoMVG/tests/Strecha \
        --image_folder=/Users/caiqi/Documents/PoMVG/tests/Strecha/castle-P19/images \
        --preprocess_type=openmvg \
        --compared_pipelines=openmvg,colmap \
        --evaluation_print_mode=comparison
```

#### 5. Custom模式 - 高性能模式（关闭评估和性能分析）
```bash
./build/output/bin/PoSDK --preset=custom \
        --dataset_dir=/Users/caiqi/Documents/PoMVG/tests/Strecha \
        --image_folder=/Users/caiqi/Documents/PoMVG/tests/Strecha/castle-P19/images \
        --enable_evaluation=false \
        --enable_profiling=false
```

### Default模式配置文件说明

在Default模式下，程序使用 `src/plugins/methods/GlobalSfMPipeline/globalsfm_pipeline.ini` 配置文件中的参数。

**主要配置参数**（在[globalsfm_pipeline]部分）：
- `dataset_dir=`: 数据集根目录路径
- `image_folder=`: 图像文件夹路径（可选）
- `work_dir=`: 工作目录
- `preprocess_type=posdk`: 预处理类型
- `enable_evaluation=true`: 是否启用精度评估
- `max_iterations=5`: 迭代优化最大次数
- `enable_summary_table=false`: 是否启用统一制表功能
- `compared_pipelines=`: 对比流水线列表

**使用Default模式的优势**：
1. 无需记忆复杂的命令行参数
2. 通过修改配置文件可以保存常用设置
3. 支持更丰富的配置选项（配置文件支持更多参数）
4. 便于脚本化和自动化执行

### 输出结果

程序执行后会在工作目录中生成以下结果：
- `poses/`: 位姿估计结果
- `logs/`: 执行日志
- `features/` 和 `matches/`: 预处理特征和匹配结果
- `summary/`: 汇总表格（如果启用了enable_summary_table）

### 错误处理

**Default模式**：
- 程序会使用配置文件中的路径参数
- 如果配置文件中的路径无效，程序会在运行时报错

**Custom模式**：
- 程序会自动验证输入参数的有效性
- 检查数据集目录是否存在
- 验证图像文件夹路径（如果指定）
- 确认预处理类型有效性

如果发生错误，程序会输出详细的错误信息并退出。

## 注意事项

1. 确保dataset_dir包含有效的图像数据
2. 对于Strecha数据集，程序会自动进行数据集验证
3. 批处理模式会自动扫描dataset_dir中包含`images`子目录的所有数据集
4. 程序支持多种预处理器，但建议使用`posdk`获得最佳性能
