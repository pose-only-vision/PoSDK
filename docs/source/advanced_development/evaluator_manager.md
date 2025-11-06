# EvaluatorManager Accuracy Evaluator

`EvaluatorManager` is a static utility class in PoSDK used for collecting, managing, and exporting various accuracy evaluation metrics. It does not directly perform evaluation calculations, but provides a centralized system to process evaluation results submitted by various data plugins (`DataIO` derived classes) and export them as standardized CSV table formats.

## Core Functions

### 1. Evaluation Result Collection
- Supports batch adding evaluation results from `EvaluatorStatus`
- Supports unified storage and management of multiple evaluation metrics
- Provides standardized evaluation result data structures

### 2. CSV Table Export
- **Structured Reports**: Generate standardized CSV evaluation reports for data analysis and algorithm comparison
- **Batch Export**: Supports CSV format export of batch evaluation results
- **Multi-format Support**: Supports algorithm comparison tables, statistical summary tables, raw data tables, and other CSV formats

---

## Standard Evaluator Development Flow

### Step 1: DataIO Plugin Development

Create a data plugin class inheriting from `DataIO`, implement `Evaluate()` function:

```cpp
class MyDataPlugin : public DataIO
{
public:
    // Implement Evaluate function for accuracy evaluation
    bool Evaluate(DataPtr gt_data_ptr) override
    {
        EvaluatorStatus eval_status;
        eval_status.SetEvalType("MyDataType");  // Set evaluation type

        // Execute specific accuracy calculations
        for (size_t i = 0; i < my_results.size(); ++i) {
            double accuracy = ComputeAccuracy(my_results[i], gt_results[i]);
            eval_status.AddResult("accuracy_metric", accuracy);

            // Optional: Add note information
            eval_status.SubmitNoteMsg("test_case", "case_" + std::to_string(i));
        }

        // Set evaluation status
        SetEvaluatorStatus(eval_status);
        return true;
    }
};
```

### Step 2: EvaluatorStatus Result Encapsulation

Use `EvaluatorStatus` to encapsulate evaluation results and status information:

#### EvaluatorStatus Data Structure

```cpp
struct EvaluatorStatus
{
    bool is_successful = false;                               // Whether evaluation was successful
    std::string eval_type;                                    // Evaluation data type (e.g., "RelativePoses")
    std::vector<std::pair<std::string, double>> eval_results; // Evaluation results: <metric_name, evaluation_value>

    // Multi-type note data: each note type corresponds to a string vector, index corresponds to eval_results
    std::unordered_map<std::string, std::vector<std::string>> note_data;
};
```

#### Main Usage Methods

```cpp
EvaluatorStatus eval_status;

// Set evaluation type
eval_status.SetEvalType("RelativePoses");

// Add evaluation results
eval_status.AddResult("rotation_error", 0.001523);
eval_status.AddResult("translation_error", 0.002156);

// Add evaluation result with note
eval_status.AddResult("rotation_error", 0.001234, "test_case", "noise_0.01");

// Or use multiple note types
std::unordered_map<std::string, std::string> notes = {
    {"test_case", "noise_0.01"},
    {"view_pair", "view(0,1)"}
};
eval_status.AddResult("rotation_error", 0.001234, notes);

// Submit notes for already added results
eval_status.SubmitNoteMsg("runtime", "0.125ms");
eval_status.SubmitNoteMsg("inlier_count", "156");

// Validate data consistency
bool valid = eval_status.ValidateNoteData();
```

#### EvaluatorStatus Core Methods

##### `SetEvalType()`
Set evaluation data type
```cpp
void SetEvalType(const std::string &eval_type);
```

##### `AddResult()`
Multiple ways to add evaluation results:

```cpp
// Method 1: Add result only
void AddResult(const std::string &metric_name, double value);

// Method 2: Add result with single note
void AddResult(const std::string &metric_name, double value,
               const std::string &note_type, const std::string &note_value);

// Method 3: Add result with multiple notes
void AddResult(const std::string &metric_name, double value,
               const std::unordered_map<std::string, std::string> &notes);
```

##### `SubmitNoteMsg()`
Submit note information for evaluation results:

```cpp
// Submit note for most recently added result
void SubmitNoteMsg(const std::string &note_type, const std::string &note_value);

// Submit note for specified index result
void SubmitNoteMsg(size_t result_index, const std::string &note_type,
                   const std::string &note_value);

// Backward compatibility: submit to "default" type
void SubmitNoteMsg(const std::string &note_value);
```

##### `ValidateNoteData()`
Validate note data consistency
```cpp
bool ValidateNoteData() const;
```

Validation rules:
1. Count of each note type must equal `eval_results.size()` (one-to-one correspondence)
2. Or the note count of that type is 1 (globally shared)
3. Or the vector of that type is empty (that type does not provide notes)

### Step 3: EvaluatorManager CSV Export

Export evaluation results as CSV tables via `EvaluatorManager`:

```cpp
// After method call, automatically triggers evaluation result submission
auto result = method->Build(input_data);

// Export statistical summary table for specific metric
EvaluatorManager::ExportMetricAllStatsToCSV(
    "RelativePoses",                          // Evaluation type
    "rotation_error",                         // Metric name
    "output/rotation_error_stats.csv"        // Output path
);

// Batch export all metrics
EvaluatorManager::ExportAllMetricsToCSV(
    "RelativePoses",                          // Evaluation type
    "output/"                                 // Output directory
);

// Export raw evaluation data (including notes)
EvaluatorManager::ExportAllRawValuesToCSV(
    "RelativePoses",                          // Evaluation type
    "output/",                                // Output directory
    "test_case|view_pair"                     // Note types to export
);
```

---

## CSV Table Style Details

EvaluatorManager supports multiple CSV export formats, each corresponding to different usage scenarios. The following details various table styles and parameter correspondences.

### CSV Header Parameter Correspondence

| CSV Header                     | Data Source                        | Example Value             | Description                                                    |
| ------------------------------ | ---------------------------------- | ------------------------- | -------------------------------------------------------------- |
| **Algorithm**                  | `Method::GetType()`                | `globalsfm_pipeline`      | Algorithm name, returned by method plugin's GetType() function |
| **EvalCommit**                 | `ProfileCommit` configuration item | `GlobalSfM Pipeline Demo` | Evaluation scenario identifier, set in ini file                |
| **Metric**                     | `AddResult(metric_name, ...)`      | `rotation_error`          | Evaluation metric name, defined in DataIO::Evaluate()          |
| **EvalType**                   | `SetEvalType(eval_type)`           | `RelativePoses`           | Evaluation data type, corresponding to data structure type     |
| **Value**                      | `AddResult(..., value)`            | `0.001523`                | Specific evaluation value                                      |
| **Mean/Median/Min/Max/StdDev** | Statistical calculation results    | `0.001450`                | Statistical analysis of multiple Value values                  |
| **Count**                      | Data point count                   | `10`                      | Number of Values participating in statistics                   |
| **Note Columns**               | `SubmitNoteMsg(note_type, ...)`    | `view(0,1)`               | Optional multi-type note information                           |

### Parameter Configuration Example

Taking GlobalSfM Pipeline as an example, parameter configuration and CSV output correspondence:

```ini
# In globalsfm_pipeline.ini
[globalsfm_pipeline]
ProfileCommit=GlobalSfM Pipeline Demo  # → EvalCommit column
enable_evaluator=true

# GetType() returns "globalsfm_pipeline" → Algorithm column
```

```cpp
// In DataIO::Evaluate()
EvaluatorStatus eval_status;
eval_status.SetEvalType("RelativePoses");  // → EvalType filter parameter

// Add evaluation results
eval_status.AddResult("rotation_error", 0.001523);     // → Metric="rotation_error", Value=0.001523
eval_status.AddResult("translation_error", 0.002156);  // → Metric="translation_error", Value=0.002156

// Add note information
eval_status.SubmitNoteMsg("view_pair", "view(0,1)");   // → view_pair column
eval_status.SubmitNoteMsg("runtime", "125ms");         // → runtime column
```

### CSV Table Style Details

#### Style 1: Statistical Summary Table (`ExportMetricAllStatsToCSV`)

**Purpose**: Complete statistical analysis for single metric
**Generation Function**: `ExportMetricAllStatsToCSV("RelativePoses", "rotation_error", "output.csv")`

```text
Algorithm,EvalCommit,Mean,Median,Min,Max,StdDev,Count
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001523,0.001450,0.000234,0.003456,0.000567,10
method_opencv,OpenCV Essential Matrix,0.002134,0.001987,0.000456,0.004567,0.000789,10
method_opengv,OpenGV Five Point,0.001234,0.001189,0.000123,0.002345,0.000445,10
```

**Description**:
- Each row represents statistical results of one algorithm under specific scenario
- Includes complete statistical information such as mean, median, min/max, standard deviation
- Suitable for algorithm performance comparison analysis

#### Style 2: Algorithm Comparison Table (`ExportAlgorithmComparisonToCSV`)

**Purpose**: Single statistical value comparison of multiple algorithms under different scenarios
**Generation Function**: `ExportAlgorithmComparisonToCSV("RelativePoses", "rotation_error", "comparison.csv", "mean")`

```text
Algorithm,noise_0.01,noise_0.05,noise_0.10,points_100,points_200
globalsfm_pipeline,0.001523,0.005234,0.012456,0.001234,0.000987
method_opencv,0.002134,0.006789,0.015678,0.001789,0.001234
method_opengv,0.001234,0.004567,0.010123,0.001023,0.000856
```

**Description**:
- Column names are different EvalCommit scenarios
- Table values are values of specified statistical type (e.g., mean)
- Facilitates intuitive comparison of different algorithms' performance under various conditions

#### Style 3: Raw Data Table (`ExportAllRawValuesToCSV`)

**Purpose**: Export all raw evaluation values and note information
**Generation Function**: `ExportAllRawValuesToCSV("RelativePoses", "output/", "view_pair|runtime")`

**File**: `rotation_error_all_evaluated_values.csv`
```text
Algorithm,EvalCommit,Value,view_pair,runtime
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001523,view(0,1),125ms
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001678,view(1,2),134ms
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001234,view(2,3),118ms
method_opencv,OpenCV Essential Matrix,0.002134,view(0,1),98ms
method_opencv,OpenCV Essential Matrix,0.002345,view(1,2),102ms
```

**Description**:
- Each row represents a specific evaluation result
- Includes complete note information (view_pair, runtime, etc.)
- Suitable for detailed analysis and data traceability

#### Style 4: Smart Format Table (ParsedEvalCommit Parsing)

When EvalCommit has unified format (e.g., "noise_0.01", "noise_0.05"), the system automatically parses to generate smart format:

```text
Algorithm,Noise,Mean,Median,Min,Max,StdDev
globalsfm_pipeline,0.01,0.001523,0.001450,0.000234,0.003456,0.000567
globalsfm_pipeline,0.05,0.005234,0.004987,0.001234,0.012345,0.002134
globalsfm_pipeline,0.10,0.012456,0.011234,0.003456,0.023456,0.005678
```

**Description**:
- "Noise" column contains parsed numeric part
- X-axis coordinates are clearer, facilitating data visualization
- Automatically recognizes "prefix_number" format EvalCommit

### Multi-type Note CSV Output

When using multi-type notes, can selectively export:

```cpp
// Add multi-type notes
eval_status.SubmitNoteMsg("view_pair", "view(0,1)");
eval_status.SubmitNoteMsg("runtime", "125ms");
eval_status.SubmitNoteMsg("inlier_count", "156");
eval_status.SubmitNoteMsg("dataset_name", "castle-P19");
```

**Export Specified Types**: `ExportAllRawValuesToCSV("RelativePoses", "output/", "view_pair|runtime")`
```text
Algorithm,EvalCommit,Value,view_pair,runtime
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001523,view(0,1),125ms
```

**Export All Types**: `ExportAllRawValuesToCSV("RelativePoses", "output/", "ALL")`
```text
Algorithm,EvalCommit,Value,view_pair,runtime,inlier_count,dataset_name
globalsfm_pipeline,GlobalSfM Pipeline Demo,0.001523,view(0,1),125ms,156,castle-P19
```

### Batch Export Results

`ExportAllMetricsToCSV("RelativePoses", "output/")` generates multiple files:

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

### Actual Usage Example

```cpp
// 1. Configure method (in ini file)
[globalsfm_pipeline]
ProfileCommit=noise_0.01              // Will be used as EvalCommit
enable_evaluator=true

// 2. Add evaluation results in DataIO
EvaluatorStatus eval_status;
eval_status.SetEvalType("RelativePoses");

for (size_t i = 0; i < pose_pairs.size(); ++i) {
    double rot_err = ComputeRotationError(estimated[i], gt[i]);
    eval_status.AddResult("rotation_error", rot_err);
    eval_status.SubmitNoteMsg("view_pair", "view(" + std::to_string(i) + "," + std::to_string(i+1) + ")");
}

// 3. System automatically calls (in MethodPreset)
EvaluatorManager::AddEvaluationResultsFromStatus(
    "RelativePoses",           // EvalType
    "globalsfm_pipeline",      // Algorithm (from GetType())
    "noise_0.01",             // EvalCommit (from ProfileCommit)
    eval_status               // Contains all evaluation results
);

// 4. Export CSV tables
EvaluatorManager::ExportMetricAllStatsToCSV(
    "RelativePoses", "rotation_error", "rotation_stats.csv"
);
```

---

## Core Function Reference

### Evaluation Result Addition Functions

#### `AddEvaluationResultsFromStatus()`

Core function for batch adding evaluation results from EvaluatorStatus

```cpp
static bool AddEvaluationResultsFromStatus(
    const std::string &eval_type,        // Evaluation type
    const std::string &algorithm_name,   // Algorithm name
    const std::string &eval_commit,      // Evaluation scenario identifier
    const EvaluatorStatus &eval_status   // Evaluation status object
);
```

**Usage Example**:
```cpp
// Automatically called in MethodPreset
EvaluatorManager::AddEvaluationResultsFromStatus(
    "RelativePoses", "method_example", "noise_0.01", eval_status
);
```

#### `AddEvaluationResult()`

Add single evaluation result

```cpp
// Method 1: With single note
static bool AddEvaluationResult(
    const std::string &eval_type,
    const std::string &algorithm_name,
    const std::string &eval_commit,
    const std::string &metric_name,
    double value,
    const std::string &note = ""
);

// Method 2: With multi-type notes
static bool AddEvaluationResult(
    const std::string &eval_type,
    const std::string &algorithm_name,
    const std::string &eval_commit,
    const std::string &metric_name,
    double value,
    const std::unordered_map<std::string, std::string> &notes
);
```

### CSV Table Export Functions

#### `ExportMetricAllStatsToCSV()`

Export complete statistical summary table for specified metric

```cpp
static bool ExportMetricAllStatsToCSV(
    const std::string &eval_type,              // Evaluation type
    const std::string &metric_name,            // Metric name
    const std::filesystem::path &output_path   // Output file path
);
```

**Output Format**:
```text
Algorithm,EvalCommit,Mean,Median,Min,Max,StdDev,Count
method_example,noise_0.01,0.001523,0.001450,0.000234,0.003456,0.000567,10
method_example,noise_0.05,0.005234,0.004987,0.001234,0.012345,0.002134,10
```

#### `ExportAllMetricsToCSV()`

Batch export CSV tables for all metrics

```cpp
static bool ExportAllMetricsToCSV(
    const std::string &eval_type,                // Evaluation type
    const std::filesystem::path &output_dir      // Output directory
);
```

#### `ExportAllRawValuesToCSV()`

Export raw evaluation data (including note information)

```cpp
static bool ExportAllRawValuesToCSV(
    const std::string &eval_type,                // Evaluation type
    const std::filesystem::path &output_dir,     // Output directory
    const std::string &note_types = ""           // Note types (optional)
);
```

**Note Type Parameter**:
- `""` (default): Do not export notes
- `"type1|type2"`: Export specified type notes
- `"ALL"`: Export all note types

**Output Format**:
```text
Algorithm,EvalCommit,Value,test_case,view_pair
method_example,noise_0.01,0.001523,case_1,view(0,1)
method_example,noise_0.01,0.002156,case_2,view(1,2)
```

#### `ExportAlgorithmComparisonToCSV()`

Export algorithm comparison table

```cpp
static bool ExportAlgorithmComparisonToCSV(
    const std::string &eval_type,              // Evaluation type
    const std::string &metric_name,            // Metric name
    const std::filesystem::path &output_path,  // Output file path
    const std::string &stat_type = "mean"      // Statistical type
);
```

---

## Usage Examples

### Complete Evaluation Flow Example

```cpp
// 1. Implement Evaluate function in DataIO plugin
bool MyRelativePosePlugin::Evaluate(DataPtr gt_data_ptr)
{
    auto gt_poses = GetDataPtr<RelativePoses>(gt_data_ptr, "data_relative_poses");

    EvaluatorStatus eval_status;
    eval_status.SetEvalType("RelativePoses");

    for (size_t i = 0; i < estimated_poses.size(); ++i) {
        // Calculate rotation error
        double rot_error = ComputeRotationError(estimated_poses[i], (*gt_poses)[i]);
        eval_status.AddResult("rotation_error", rot_error);

        // Calculate translation error
        double trans_error = ComputeTranslationError(estimated_poses[i], (*gt_poses)[i]);
        eval_status.AddResult("translation_error", trans_error);

        // Add note information
        eval_status.SubmitNoteMsg("view_pair", "view(" + std::to_string(i) + "," + std::to_string(i+1) + ")");
        eval_status.SubmitNoteMsg("runtime", std::to_string(runtimes[i]) + "ms");
    }

    SetEvaluatorStatus(eval_status);
    return true;
}

// 2. Run method and export results in test code
auto method = FactoryMethod::Create("my_method");
auto method_preset = std::dynamic_pointer_cast<MethodPreset>(method);

// Set ground truth data
method_preset->SetGTData(gt_data);

// Run method (automatically triggers evaluation)
auto result = method_preset->Build(input_data);

// 3. Export CSV tables
// Export statistical summary
EvaluatorManager::ExportMetricAllStatsToCSV(
    "RelativePoses", "rotation_error", "output/rotation_error_stats.csv"
);

// Export raw data and notes
EvaluatorManager::ExportAllRawValuesToCSV(
    "RelativePoses", "output/", "view_pair|runtime"
);

// Batch export all metrics
EvaluatorManager::ExportAllMetricsToCSV("RelativePoses", "output/");
```

### Multi-algorithm Comparison Example

```cpp
// Run multiple algorithms
std::vector<std::string> methods = {"method_A", "method_B", "method_C"};

for (const auto& method_name : methods) {
    auto method = FactoryMethod::Create(method_name);
    auto method_preset = std::dynamic_pointer_cast<MethodPreset>(method);

    method_preset->SetGTData(gt_data);
    auto result = method_preset->Build(input_data);
}

// Export algorithm comparison table
EvaluatorManager::ExportAlgorithmComparisonToCSV(
    "RelativePoses", "rotation_error", "output/algorithm_comparison.csv", "mean"
);
```

---

## Notes

1. **Evaluation Type Consistency**: Type set by `EvaluatorStatus::SetEvalType()` must match the `eval_type` parameter used when exporting

2. **Note Data Validation**: Use `ValidateNoteData()` to ensure note data matches evaluation result count

3. **Algorithm Name Retrieval**: In `MethodPreset`, algorithm name is automatically obtained via `GetType()`

4. **Configuration File Settings**: Need to set `ProfileCommit` parameter in method's ini configuration file as evaluation scenario identifier

5. **CSV File Naming**: Exported CSV files are automatically named based on evaluation type and metric name to avoid file overwriting

---

*This document focuses on EvaluatorManager's accuracy evaluation functionality, providing a complete development guide for CSV table export and evaluation result management.*
