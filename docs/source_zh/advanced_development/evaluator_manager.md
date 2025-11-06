# EvaluatorManager 精度评估器

`EvaluatorManager` 是 PoSDK 中用于收集、管理和导出各种精度评估指标的静态工具类。它不直接执行评估计算，而是提供一个中心化的系统来处理由各个数据插件 (`DataIO` 派生类) 提交的评估结果，并导出为标准化的CSV表格格式。

## 核心功能

### 1. 评估结果收集
- 支持从 `EvaluatorStatus` 批量添加评估结果
- 支持多种评估指标的统一存储和管理
- 提供标准化的评估结果数据结构

### 2. CSV表格导出
- **结构化报告**: 生成标准化的CSV评估报告，便于数据分析和算法对比
- **批量导出**: 支持批量评估结果的CSV格式导出
- **多格式支持**: 支持算法对比表、统计摘要表、原始数据表等多种CSV格式

---

## 标准评估器开发流程

### 第1步：DataIO插件开发

创建继承自 `DataIO` 的数据插件类，实现 `Evaluate()` 函数：

```cpp
class MyDataPlugin : public DataIO
{
public:
    // 实现Evaluate函数进行精度评估
    bool Evaluate(DataPtr gt_data_ptr) override
    {
        EvaluatorStatus eval_status;
        eval_status.SetEvalType("MyDataType");  // 设置评估类型

        // 执行具体的精度计算
        for (size_t i = 0; i < my_results.size(); ++i) {
            double accuracy = ComputeAccuracy(my_results[i], gt_results[i]);
            eval_status.AddResult("accuracy_metric", accuracy);

            // 可选：添加备注信息
            eval_status.SubmitNoteMsg("test_case", "case_" + std::to_string(i));
        }

        // 设置评估状态
        SetEvaluatorStatus(eval_status);
        return true;
    }
};
```

### 第2步：EvaluatorStatus结果封装

使用 `EvaluatorStatus` 封装评估结果和状态信息：

#### EvaluatorStatus数据结构

```cpp
struct EvaluatorStatus
{
    bool is_successful = false;                               // 评估是否成功
    std::string eval_type;                                    // 评估数据类型（如"RelativePoses"）
    std::vector<std::pair<std::string, double>> eval_results; // 评估结果: <指标名, 评估值>

    // 多类型备注数据：每个备注类型对应一个字符串向量，索引与eval_results对应
    std::unordered_map<std::string, std::vector<std::string>> note_data;
};
```

#### 主要使用方法

```cpp
EvaluatorStatus eval_status;

// 设置评估类型
eval_status.SetEvalType("RelativePoses");

// 添加评估结果
eval_status.AddResult("rotation_error", 0.001523);
eval_status.AddResult("translation_error", 0.002156);

// 添加带备注的评估结果
eval_status.AddResult("rotation_error", 0.001234, "test_case", "noise_0.01");

// 或使用多类型备注
std::unordered_map<std::string, std::string> notes = {
    {"test_case", "noise_0.01"},
    {"view_pair", "view(0,1)"}
};
eval_status.AddResult("rotation_error", 0.001234, notes);

// 为已添加的结果提交备注
eval_status.SubmitNoteMsg("runtime", "0.125ms");
eval_status.SubmitNoteMsg("inlier_count", "156");

// 验证数据一致性
bool valid = eval_status.ValidateNoteData();
```

#### EvaluatorStatus核心方法

##### `SetEvalType()`
设置评估数据类型
```cpp
void SetEvalType(const std::string &eval_type);
```

##### `AddResult()`
添加评估结果的多种方式：

```cpp
// 方式1：仅添加结果
void AddResult(const std::string &metric_name, double value);

// 方式2：添加结果和单个备注
void AddResult(const std::string &metric_name, double value,
               const std::string &note_type, const std::string &note_value);

// 方式3：添加结果和多个备注
void AddResult(const std::string &metric_name, double value,
               const std::unordered_map<std::string, std::string> &notes);
```

##### `SubmitNoteMsg()`
为评估结果提交备注信息：

```cpp
// 为最新添加的结果提交备注
void SubmitNoteMsg(const std::string &note_type, const std::string &note_value);

// 为指定索引的结果提交备注
void SubmitNoteMsg(size_t result_index, const std::string &note_type,
                   const std::string &note_value);

// 向后兼容：提交到"default"类型
void SubmitNoteMsg(const std::string &note_value);
```

##### `ValidateNoteData()`
验证备注数据的一致性
```cpp
bool ValidateNoteData() const;
```

验证规则：
1. 每种备注类型的数量必须等于 `eval_results.size()`（一对一对应）
2. 或者该类型的备注数量为1（全局共享）
3. 或者该类型的向量为空（该类型不提供备注）

### 第3步：EvaluatorManager CSV导出

通过 `EvaluatorManager` 将评估结果导出为CSV表格：

```cpp
// 方法调用后，自动触发评估结果提交
auto result = method->Build(input_data);

// 导出特定指标的统计摘要表
EvaluatorManager::ExportMetricAllStatsToCSV(
    "RelativePoses",                          // 评估类型
    "rotation_error",                         // 指标名称
    "output/rotation_error_stats.csv"        // 输出路径
);

// 批量导出所有指标
EvaluatorManager::ExportAllMetricsToCSV(
    "RelativePoses",                          // 评估类型
    "output/"                                 // 输出目录
);

// 导出原始评估数据（包含备注）
EvaluatorManager::ExportAllRawValuesToCSV(
    "RelativePoses",                          // 评估类型
    "output/",                                // 输出目录
    "test_case|view_pair"                     // 导出的备注类型
);
```

---

## CSV表格样式详解

EvaluatorManager支持多种CSV导出格式，每种格式对应不同的使用场景。以下详细说明各种表格样式和参数对应关系。

### CSV表头参数对应关系

| CSV表头 | 数据来源 | 示例值 | 说明 |
|---------|----------|---------|------|
| **Algorithm** | `Method::GetType()` | `globalsfm_pipeline` | 算法名称，由方法插件的GetType()函数返回 |
| **EvalCommit** | `ProfileCommit`配置项 | `GlobalSfM Pipeline Demo` | 评估场景标识，在ini文件中设置 |
| **Metric** | `AddResult(metric_name, ...)` | `rotation_error` | 评估指标名称，在DataIO::Evaluate()中定义 |
| **EvalType** | `SetEvalType(eval_type)` | `RelativePoses` | 评估数据类型，对应数据结构类型 |
| **Value** | `AddResult(..., value)` | `0.001523` | 具体的评估数值 |
| **Mean/Median/Min/Max/StdDev** | 统计计算结果 | `0.001450` | 对多个Value值的统计分析 |
| **Count** | 数据点数量 | `10` | 参与统计的Value数量 |
| **备注列** | `SubmitNoteMsg(note_type, ...)` | `view(0,1)` | 可选的多类型备注信息 |

### 参数配置示例

以GlobalSfM Pipeline为例，参数配置和CSV输出的对应关系：

```ini
# 在globalsfm_pipeline.ini中
[globalsfm_pipeline]
ProfileCommit=GlobalSfM Pipeline Demo  # → EvalCommit列
enable_evaluator=true

# GetType()返回"globalsfm_pipeline" → Algorithm列
```

```cpp
// 在DataIO::Evaluate()中
EvaluatorStatus eval_status;
eval_status.SetEvalType("RelativePoses");  // → EvalType过滤参数

// 添加评估结果
eval_status.AddResult("rotation_error", 0.001523);     // → Metric="rotation_error", Value=0.001523
eval_status.AddResult("translation_error", 0.002156);  // → Metric="translation_error", Value=0.002156

// 添加备注信息
eval_status.SubmitNoteMsg("view_pair", "view(0,1)");   // → view_pair列
eval_status.SubmitNoteMsg("runtime", "125ms");         // → runtime列
```

### CSV表格样式详解

#### 样式1：统计摘要表 (`ExportMetricAllStatsToCSV`)

**用途**: 单一指标的完整统计分析
**生成函数**: `ExportMetricAllStatsToCSV("RelativePoses", "rotation_error", "output.csv")`

```text
Algorithm,EvalCommit,Mean,Median,Min,Max,StdDev,Count
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001523,0.001450,0.000234,0.003456,0.000567,10
method_opencv,OpenCV Essential Matrix,0.002134,0.001987,0.000456,0.004567,0.000789,10
method_opengv,OpenGV Five Point,0.001234,0.001189,0.000123,0.002345,0.000445,10
```

**说明**:
- 每行代表一个算法在特定场景下的统计结果
- 包含均值、中位数、最值、标准差等完整统计信息
- 适用于算法性能对比分析

#### 样式2：算法对比表 (`ExportAlgorithmComparisonToCSV`)

**用途**: 多算法在不同场景下的单一统计值对比
**生成函数**: `ExportAlgorithmComparisonToCSV("RelativePoses", "rotation_error", "comparison.csv", "mean")`

```text
Algorithm,noise_0.01,noise_0.05,noise_0.10,points_100,points_200
globalsfm_pipeline,0.001523,0.005234,0.012456,0.001234,0.000987
method_opencv,0.002134,0.006789,0.015678,0.001789,0.001234
method_opengv,0.001234,0.004567,0.010123,0.001023,0.000856
```

**说明**:
- 列名为不同的EvalCommit场景
- 表格值为指定统计类型的数值（如mean）
- 便于直观对比不同算法在各种条件下的性能

#### 样式3：原始数据表 (`ExportAllRawValuesToCSV`)

**用途**: 导出所有原始评估值和备注信息
**生成函数**: `ExportAllRawValuesToCSV("RelativePoses", "output/", "view_pair|runtime")`

**文件**: `rotation_error_all_evaluated_values.csv`
```text
Algorithm,EvalCommit,Value,view_pair,runtime
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001523,view(0,1),125ms
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001678,view(1,2),134ms
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001234,view(2,3),118ms
method_opencv,OpenCV Essential Matrix,0.002134,view(0,1),98ms
method_opencv,OpenCV Essential Matrix,0.002345,view(1,2),102ms
```

**说明**:
- 每行代表一个具体的评估结果
- 包含完整的备注信息（view_pair, runtime等）
- 适用于详细分析和数据追溯

#### 样式4：智能格式表（ParsedEvalCommit解析）

当EvalCommit具有统一格式时（如"noise_0.01", "noise_0.05"），系统自动解析生成智能格式：

```text
Algorithm,Noise,Mean,Median,Min,Max,StdDev
globalsfm_pipeline,0.01,0.001523,0.001450,0.000234,0.003456,0.000567
globalsfm_pipeline,0.05,0.005234,0.004987,0.001234,0.012345,0.002134
globalsfm_pipeline,0.10,0.012456,0.011234,0.003456,0.023456,0.005678
```

**说明**:
- "Noise"列包含解析出的数值部分
- X轴坐标更加清晰，便于数据可视化
- 自动识别"prefix_number"格式的EvalCommit

### 多类型备注的CSV输出

当使用多类型备注时，可以选择性导出：

```cpp
// 添加多类型备注
eval_status.SubmitNoteMsg("view_pair", "view(0,1)");
eval_status.SubmitNoteMsg("runtime", "125ms");
eval_status.SubmitNoteMsg("inlier_count", "156");
eval_status.SubmitNoteMsg("dataset_name", "castle-P19");
```

**导出指定类型**: `ExportAllRawValuesToCSV("RelativePoses", "output/", "view_pair|runtime")`
```text
Algorithm,EvalCommit,Value,view_pair,runtime
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001523,view(0,1),125ms
```

**导出所有类型**: `ExportAllRawValuesToCSV("RelativePoses", "output/", "ALL")`
```text
Algorithm,EvalCommit,Value,view_pair,runtime,inlier_count,dataset_name
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001523,view(0,1),125ms,156,castle-P19
```

### 批量导出结果

`ExportAllMetricsToCSV("RelativePoses", "output/")`会生成多个文件：

```
output/
├── RelativePoses_rotation_error_mean_comparison.csv
├── RelativePoses_rotation_error_median_comparison.csv
├── RelativePoses_rotation_error_ALL_STATS.csv
├── RelativePoses_translation_error_mean_comparison.csv
├── RelativePoses_translation_error_median_comparison.csv
├── RelativePoses_translation_error_ALL_STATS.csv
├── rotation_error_all_evaluated_values.csv
└── translation_error_all_evaluated_values.csv
```

### 实际使用示例

```cpp
// 1. 配置方法（在ini文件中）
[globalsfm_pipeline]
ProfileCommit=noise_0.01              // 将作为EvalCommit
enable_evaluator=true

// 2. 在DataIO中添加评估结果
EvaluatorStatus eval_status;
eval_status.SetEvalType("RelativePoses");

for (size_t i = 0; i < pose_pairs.size(); ++i) {
    double rot_err = ComputeRotationError(estimated[i], gt[i]);
    eval_status.AddResult("rotation_error", rot_err);
    eval_status.SubmitNoteMsg("view_pair", "view(" + std::to_string(i) + "," + std::to_string(i+1) + ")");
}

// 3. 系统自动调用（在MethodPreset中）
EvaluatorManager::AddEvaluationResultsFromStatus(
    "RelativePoses",           // EvalType
    "globalsfm_pipeline",      // Algorithm (from GetType())
    "noise_0.01",             // EvalCommit (from ProfileCommit)
    eval_status               // 包含所有评估结果
);

// 4. 导出CSV表格
EvaluatorManager::ExportMetricAllStatsToCSV(
    "RelativePoses", "rotation_error", "rotation_stats.csv"
);
```

---

## 核心函数参考

### 评估结果添加函数

#### `AddEvaluationResultsFromStatus()`

从EvaluatorStatus批量添加评估结果的核心函数

```cpp
static bool AddEvaluationResultsFromStatus(
    const std::string &eval_type,        // 评估类型
    const std::string &algorithm_name,   // 算法名称
    const std::string &eval_commit,      // 评估场景标识
    const EvaluatorStatus &eval_status   // 评估状态对象
);
```

**使用示例**:
```cpp
// 在MethodPreset中自动调用
EvaluatorManager::AddEvaluationResultsFromStatus(
    "RelativePoses", "method_example", "noise_0.01", eval_status
);
```

#### `AddEvaluationResult()`

添加单个评估结果

```cpp
// 方式1：带单一备注
static bool AddEvaluationResult(
    const std::string &eval_type,
    const std::string &algorithm_name,
    const std::string &eval_commit,
    const std::string &metric_name,
    double value,
    const std::string &note = ""
);

// 方式2：带多类型备注
static bool AddEvaluationResult(
    const std::string &eval_type,
    const std::string &algorithm_name,
    const std::string &eval_commit,
    const std::string &metric_name,
    double value,
    const std::unordered_map<std::string, std::string> &notes
);
```

### CSV表格导出函数

#### `ExportMetricAllStatsToCSV()`

导出指定指标的完整统计摘要表

```cpp
static bool ExportMetricAllStatsToCSV(
    const std::string &eval_type,              // 评估类型
    const std::string &metric_name,            // 指标名称
    const std::filesystem::path &output_path   // 输出文件路径
);
```

**输出格式**:
```text
Algorithm,EvalCommit,Mean,Median,Min,Max,StdDev,Count
method_example,noise_0.01,0.001523,0.001450,0.000234,0.003456,0.000567,10
method_example,noise_0.05,0.005234,0.004987,0.001234,0.012345,0.002134,10
```

#### `ExportAllMetricsToCSV()`

批量导出所有指标的CSV表格

```cpp
static bool ExportAllMetricsToCSV(
    const std::string &eval_type,                // 评估类型
    const std::filesystem::path &output_dir      // 输出目录
);
```

#### `ExportAllRawValuesToCSV()`

导出原始评估数据（包含备注信息）

```cpp
static bool ExportAllRawValuesToCSV(
    const std::string &eval_type,                // 评估类型
    const std::filesystem::path &output_dir,     // 输出目录
    const std::string &note_types = ""           // 备注类型（可选）
);
```

**备注类型参数**:
- `""` (默认): 不导出备注
- `"type1|type2"`: 导出指定类型备注
- `"ALL"`: 导出所有备注类型

**输出格式**:
```text
Algorithm,EvalCommit,Value,test_case,view_pair
method_example,noise_0.01,0.001523,case_1,view(0,1)
method_example,noise_0.01,0.002156,case_2,view(1,2)
```

#### `ExportAlgorithmComparisonToCSV()`

导出算法对比表格

```cpp
static bool ExportAlgorithmComparisonToCSV(
    const std::string &eval_type,              // 评估类型
    const std::string &metric_name,            // 指标名称
    const std::filesystem::path &output_path,  // 输出文件路径
    const std::string &stat_type = "mean"      // 统计类型
);
```

---

## 使用示例

### 完整的评估流程示例

```cpp
// 1. 在DataIO插件中实现Evaluate函数
bool MyRelativePosePlugin::Evaluate(DataPtr gt_data_ptr)
{
    auto gt_poses = GetDataPtr<RelativePoses>(gt_data_ptr, "data_relative_poses");

    EvaluatorStatus eval_status;
    eval_status.SetEvalType("RelativePoses");

    for (size_t i = 0; i < estimated_poses.size(); ++i) {
        // 计算旋转误差
        double rot_error = ComputeRotationError(estimated_poses[i], (*gt_poses)[i]);
        eval_status.AddResult("rotation_error", rot_error);

        // 计算平移误差
        double trans_error = ComputeTranslationError(estimated_poses[i], (*gt_poses)[i]);
        eval_status.AddResult("translation_error", trans_error);

        // 添加备注信息
        eval_status.SubmitNoteMsg("view_pair", "view(" + std::to_string(i) + "," + std::to_string(i+1) + ")");
        eval_status.SubmitNoteMsg("runtime", std::to_string(runtimes[i]) + "ms");
    }

    SetEvaluatorStatus(eval_status);
    return true;
}

// 2. 在测试代码中运行方法并导出结果
auto method = FactoryMethod::Create("my_method");
auto method_preset = std::dynamic_pointer_cast<MethodPreset>(method);

// 设置真值数据
method_preset->SetGTData(gt_data);

// 运行方法（自动触发评估）
auto result = method_preset->Build(input_data);

// 3. 导出CSV表格
// 导出统计摘要
EvaluatorManager::ExportMetricAllStatsToCSV(
    "RelativePoses", "rotation_error", "output/rotation_error_stats.csv"
);

// 导出原始数据和备注
EvaluatorManager::ExportAllRawValuesToCSV(
    "RelativePoses", "output/", "view_pair|runtime"
);

// 批量导出所有指标
EvaluatorManager::ExportAllMetricsToCSV("RelativePoses", "output/");
```

### 多算法对比示例

```cpp
// 运行多个算法
std::vector<std::string> methods = {"method_A", "method_B", "method_C"};

for (const auto& method_name : methods) {
    auto method = FactoryMethod::Create(method_name);
    auto method_preset = std::dynamic_pointer_cast<MethodPreset>(method);

    method_preset->SetGTData(gt_data);
    auto result = method_preset->Build(input_data);
}

// 导出算法对比表格
EvaluatorManager::ExportAlgorithmComparisonToCSV(
    "RelativePoses", "rotation_error", "output/algorithm_comparison.csv", "mean"
);
```

---

## 注意事项

1. **评估类型一致性**: `EvaluatorStatus::SetEvalType()` 设置的类型必须与导出时使用的 `eval_type` 参数一致

2. **备注数据验证**: 使用 `ValidateNoteData()` 确保备注数据与评估结果数量匹配

3. **算法名称获取**: 在 `MethodPreset` 中，算法名称通过 `GetType()` 自动获取

4. **配置文件设置**: 在方法的ini配置文件中需要设置 `ProfileCommit` 参数作为评估场景标识

5. **CSV文件命名**: 导出的CSV文件会根据评估类型和指标名称自动命名，避免文件覆盖

---

*本文档专注于 EvaluatorManager 的精度评估功能，提供CSV表格导出和评估结果管理的完整开发指南。*