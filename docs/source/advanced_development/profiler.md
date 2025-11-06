# Profiler

PoSDK includes a high-performance code execution monitoring tool that helps developers identify performance bottlenecks in program execution.

## What is a Performance Label?

**Label** is an attribute tag for code segments, used to track the performance of different functions or code blocks. It's like attaching name tags to code blocks, making it easy to see which parts run fast or slow.

For example:
- `"feature_extraction"` - marks feature detection code segments
- `"image_matching"` - marks image matching algorithm code segments
- `"triangulation_reconstruction"` - marks 3D point reconstruction code segments

## Basic Usage

### 1. Automatic Function Performance Measurement

```cpp
void FeatureExtractionFunction()
{
    PROFILER_START_AUTO(true);  // Automatically use function name as label for performance measurement

    // Your feature extraction code...

    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

### 2. Measure Specific Code Segment Performance

```cpp
void ImageProcessingFunction()
{
    PROFILER_START_AUTO(true);

    // Measure image preprocessing section
    InitializeData();
    PROFILER_STAGE("image_preprocessing");

    // Measure feature detection section
    ExtractFeatures();
    PROFILER_STAGE("SIFT_feature_detection");

    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

### 3. Enable Profiler and View Results

```cpp
int main()
{
    // Enable performance profiler (for specific methods)
    PM_SET_PROFILING_ENABLED("my_algorithm", true);

    // Run your program
    FeatureExtractionFunction();
    ImageProcessingFunction();

    // Generate performance report file
    PM_EXPORT_TO_CSV("performance_report.csv");

    // Or print directly to console for viewing
    PM_DISPLAY_ALL_PROFILING_DATA();

    return 0;
}
```

## Performance Report Output Examples

After running the above code, you will see performance statistics similar to the following:

### Console Output
```
=== FeatureExtractionFunction Current Session Performance Statistics ===
Duration: 125.43 ms
Memory Increment: 34.2 MB (relative to start)
CPU Usage: 187.4%
Thread Count (start/end): 1/4

=== Stage Interval Statistics ===
  START_to_image_preprocessing: 34.21 ms
  image_preprocessing_to_SIFT_feature_detection: 55.46 ms
  SIFT_feature_detection_to_END: 35.76 ms
```

### CSV File Output (performance_report.csv)

| Label Name (label_str)    | Session Count | Total Execution Time (ms) | Average Time (ms) | Min Time (ms) | Max Time (ms) | Peak Memory (MB) | Average CPU (%) |
| ------------------------- | ------------- | ------------------------- | ----------------- | ------------- | ------------- | ---------------- | --------------- |
| FeatureExtractionFunction | 1             | 125.43                    | 125.43            | 125.43        | 125.43        | 34.2             | 187.4           |
| ImageProcessingFunction   | 1             | 89.67                     | 89.67             | 89.67         | 89.67         | 28.1             | 156.8           |

**Table Description:**
- **Label Name**: The label string (label_str) you set in your code
- **Session Count**: How many times this code segment was executed
- **Total Execution Time**: How long this code segment has accumulated in total
- **Average Time**: Average execution time per run
- **Peak Memory**: Maximum memory increment relative to start during this code segment's execution
- **Average CPU**: Average CPU usage rate for this code segment

## Advanced Usage Tips

### Using Different Performance Metrics

```cpp
void DataProcessingFunction()
{
    // Time only (fastest, suitable for frequently called functions)
    POSDK_START(true, "time");

    // Time + memory statistics (suitable for memory-sensitive algorithms)
    POSDK_START(true, "time|memory");

    // Complete statistics (time + memory + CPU + thread count)
    POSDK_START(true, "time|memory|cpu");

    // Your algorithm code...

    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

### Monitoring External Program Performance

```cpp
void CallExternalTool()
{
    PROFILER_START_AUTO(true);

    // Automatically monitor subprocess resource usage when executing external programs
    int result = POSDK_SYSTEM("python train_model.py --epochs 100");

    PROFILER_END();
    PROFILER_PRINT_STATS(true);  // Will display total resource consumption including subprocesses
}
```

### Experiment Comparison Analysis

```cpp
void AlgorithmComparisonExperiment()
{
    // Experiment 1: Basic algorithm
    std::unordered_map<std::string, std::string> labels1 = {
        {"algorithm_type", "basic_sfm"},
        {"dataset", "indoor_scene"},
        {"experiment_batch", "2025_01"}
    };
    PROFILER_START_STRUCTURED(labels1, true);
    RunBasicAlgorithm();
    PROFILER_END();

    // Experiment 2: Improved algorithm
    std::unordered_map<std::string, std::string> labels2 = {
        {"algorithm_type", "improved_sfm"},
        {"dataset", "indoor_scene"},
        {"experiment_batch", "2025_01"}
    };
    PROFILER_START_STRUCTURED(labels2, true);
    RunImprovedAlgorithm();
    PROFILER_END();

    // Export comparison results to CSV for Excel analysis
    PM_EXPORT_TO_CSV("algorithm_comparison_results.csv");
}
```

## Practical Recommendations

1. **Use meaningful label names**: Use descriptive names such as "feature_matching", "pose_estimation", etc.
2. **Measure key code segments**: Focus on algorithms and loops that may be time-consuming
3. **Review reports regularly**: Use performance reports to identify program bottlenecks and optimize the most time-consuming parts
4. **Can be disabled in release**: Performance profiling can be disabled via compile options in final release versions to avoid affecting runtime speed
5. **Choose appropriate metrics**: For frequently called functions, only track time; for memory-sensitive algorithms, add memory monitoring

## System Configuration

The profiler is disabled by default and needs to be manually enabled:

```cpp
// Enable profiler for specific methods
PM_SET_PROFILING_ENABLED("my_algorithm", true);

// Check if enabled
bool profiler_enabled = PM_IS_PROFILING_ENABLED("my_algorithm");

// Clear historical statistics
PM_CLEAR();
```

This way, developers can easily monitor code performance, identify program execution bottlenecks, and optimize code to improve overall performance.

---

## Quick Feature Navigation

### Quick Start
(quick-start-profiler)=

- **Basic Macros**: [`PROFILER_START_AUTO()`](#profiler-start-auto) | [`PROFILER_END()`](#profiler-end) | [`PROFILER_PRINT_STATS()`](#profiler-print-stats)
- **Advanced Macros**: [`POSDK_START()`](#posdk-start) | [`POSDK_SYSTEM()`](#posdk-system) | [`PROFILER_STAGE()`](#profiler-stage)
- **Management Functions**: [`GetInstance()`](#getinstance) | [`SetProfilingEnabled()`](#setprofilingenabled) | [`ExportToCSV()`](#exporttocsv)

### Profiler Macro Categories
(profiler-macros-navigation)=

| Macro Category             | Core Macros                                                           | Description                            |
| -------------------------- | --------------------------------------------------------------------- | -------------------------------------- |
| **Auto Label Macros**      | [`PROFILER_START_AUTO()`](#profiler-start-auto)                       | Automatically use method preset labels |
|                            | [`PROFILER_SCOPE()`](#profiler-scope)                                 | RAII-style automatic scope analysis    |
| **Manual Label Macros**    | [`PROFILER_START()`](#profiler-start)                                 | Use string labels                      |
|                            | [`PROFILER_START_STRUCTURED()`](#profiler-start-structured)           | Use structured labels                  |
| **Advanced Config Macros** | [`POSDK_START()`](#posdk-start)                                       | Configurable metric types              |
|                            | [`PROFILER_START_WITH_METRICS()`](#profiler-start-with-metrics)       | Specify specific metrics               |
|                            | [`PROFILER_START_WITH_SUBPROCESS()`](#profiler-start-with-subprocess) | Include subprocess monitoring          |
| **System Command Macros**  | [`POSDK_SYSTEM()`](#posdk-system)                                     | Performance-aware system commands      |
|                            | [`POSDK_SYSTEM_AUTO()`](#posdk-system-auto)                           | Automatic session management           |
| **Stage Analysis Macros**  | [`PROFILER_STAGE()`](#profiler-stage)                                 | Stage checkpoint                       |
| **End and Output Macros**  | [`PROFILER_END()`](#profiler-end)                                     | End session                            |
|                            | [`PROFILER_PRINT_STATS()`](#profiler-print-stats)                     | Print current session statistics       |
|                            | [`PM_PRINT_STATS()`](#pm-print-stats)                                 | Print accumulated statistics           |

### ProfilerManager Management Functions
(profiler-manager-navigation)=

| Function Category       | Core Functions                                          | Description            |
| ----------------------- | ------------------------------------------------------- | ---------------------- |
| **Instance Management** | [`GetInstance()`](#getinstance)                         | Get singleton instance |
| **Status Control**      | [`IsProfilingEnabled()`](#isprofilingenabled)           | Check enabled status   |
|                         | [`SetProfilingEnabled()`](#setprofilingenabled)         | Set enabled status     |
| **Data Submission**     | [`SubmitSessionData()`](#submitsessiondata)             | Submit session data    |
| **Data Query**          | [`GetTotalProfilerInfo()`](#gettotalprofilerinfo)       | Get accumulated data   |
| **Data Export**         | [`ExportToCSV()`](#exporttocsv)                         | Export CSV file        |
| **Data Management**     | [`DisplayAllProfilingData()`](#displayallprofilingdata) | Display all data       |
|                         | [`Clear()`](#clear)                                     | Clear data             |

### ProfilerSession Session Functions
(profiler-session-navigation)=

| Function Category      | Core Functions                                                      | Description                   |
| ---------------------- | ------------------------------------------------------------------- | ----------------------------- |
| **Session Status**     | [`IsActive()`](#isactive)                                           | Check session status          |
|                        | [`IsMetricsEnabled()`](#ismetricsenabled)                           | Check metrics enabled         |
|                        | [`IsSubprocessMonitoringEnabled()`](#issubprocessmonitoringenabled) | Check subprocess monitoring   |
| **Subprocess Control** | [`EnableSubprocessMonitoring()`](#enablesubprocessmonitoring)       | Enable subprocess monitoring  |
|                        | [`DisableSubprocessMonitoring()`](#disablesubprocessmonitoring)     | Disable subprocess monitoring |
|                        | [`NotifySubprocessStartup()`](#notifysubprocessstartup)             | Notify subprocess startup     |
| **Stage Management**   | [`AddStageCheckpoint()`](#addstagecheckpoint)                       | Add stage checkpoint          |
| **System Command**     | [`ExecuteSystemCommand()`](#executesystemcommand)                   | Execute system command        |
| **Data Access**        | [`GetSessionData()`](#getsessiondata)                               | Get session data              |

---

## Complete Function and Macro Reference

### ProfilerManager Core Functions
(profiler-manager-functions)=

#### `GetInstance()`
(getinstance)=

Get ProfilerManager singleton instance

```cpp
static ProfilerManager &GetInstance();
```

**Return Value**: `ProfilerManager &` - Reference to singleton instance

**Usage Example**:
```cpp
auto& manager = ProfilerManager::GetInstance();
manager.SetProfilingEnabled("my_algorithm", true);
```

#### `IsProfilingEnabled()`
(isprofilingenabled)=

Check if performance profiling is enabled for a method type

```cpp
bool IsProfilingEnabled(const std::string &method_type) const;
```

**Parameters**:
- `method_type` `const std::string &`: Method type to check

**Return Value**: `bool` - Returns true if performance profiling is enabled

#### `SetProfilingEnabled()`
(setprofilingenabled)=

Enable or disable performance profiling for a method type

```cpp
void SetProfilingEnabled(const std::string &method_type, bool enabled);
```

**Parameters**:
- `method_type` `const std::string &`: Method type
- `enabled` `bool`: Whether to enable performance profiling

#### `SubmitSessionData()`
(submitsessiondata)=

Submit completed performance profiling session data

```cpp
void SubmitSessionData(const ProfilerInfo &session_data);
```

**Parameters**:
- `session_data` `const ProfilerInfo &`: Complete session data

**Note**: This is the only function that modifies global state, called once per session

#### `GetTotalProfilerInfo()`
(gettotalprofilerinfo)=

Get accumulated performance profiling data for a label

```cpp
TotalProfilerInfo GetTotalProfilerInfo(const std::string &label_str) const;
```

**Parameters**:
- `label_str` `const std::string &`: Profiling label

**Return Value**: `TotalProfilerInfo` - Copy of accumulated performance profiling data

#### `DisplayAllProfilingData()`
(displayallprofilingdata)=

Display all accumulated performance profiling data

```cpp
void DisplayAllProfilingData() const;
```

**Note**: Displays performance statistics for all analyzed sessions

#### `ExportToCSV()`
(exporttocsv)=

Export performance profiling data to CSV file

```cpp
bool ExportToCSV(const std::string &file_path) const;
```

**Parameters**:
- `file_path` `const std::string &`: CSV file path

**Return Value**: `bool` - Returns true if export succeeds

#### `Clear()`
(clear)=

Clear all performance profiling data

```cpp
void Clear();
```

**Note**: Resets all accumulated statistics

### ProfilerSession Core Functions
(profiler-session-functions)=

#### `IsActive()`
(isactive)=

Check if session is active

```cpp
bool IsActive() const;
```

**Return Value**: `bool` - Returns true if session is active

#### `IsMetricsEnabled()`
(ismetricsenabled)=

Check if a specific metric type is enabled

```cpp
bool IsMetricsEnabled(MetricsType type) const;
```

**Parameters**:
- `type` `MetricsType`: Metric type to check

**Return Value**: `bool` - Returns true if enabled

#### `IsSubprocessMonitoringEnabled()`
(issubprocessmonitoringenabled)=

Check if subprocess monitoring is enabled

```cpp
bool IsSubprocessMonitoringEnabled() const;
```

**Return Value**: `bool` - Returns true if enabled

#### `EnableSubprocessMonitoring()`
(enablesubprocessmonitoring)=

Dynamically enable subprocess monitoring

```cpp
void EnableSubprocessMonitoring();
```

**Note**: Switch from current process monitoring to process tree monitoring

#### `DisableSubprocessMonitoring()`
(disablesubprocessmonitoring)=

Disable subprocess monitoring

```cpp
void DisableSubprocessMonitoring();
```

**Note**: Temporarily disable subprocess monitoring without changing other metrics

#### `AddStageCheckpoint()`
(addstagecheckpoint)=

Add intelligent stage checkpoint

```cpp
void AddStageCheckpoint(const std::string &stage_name);
```

**Parameters**:
- `stage_name` `const std::string &`: Stage name

**Note**: Automatically captures performance metrics at this checkpoint (lock-free operation)

#### `GetSessionData()`
(getsessiondata)=

Get copy of current session data

```cpp
ProfilerInfo GetSessionData() const;
```

**Return Value**: `ProfilerInfo` - Copy of current session data

**Note**: Returns a copy of session_data_ for use after session ends

#### `NotifySubprocessStartup()`
(notifysubprocessstartup)=

Notify session that subprocess has started

```cpp
void NotifySubprocessStartup(const std::string &command);
```

**Parameters**:
- `command` `const std::string &`: Command being executed

**Note**: Triggers immediate sampling when subprocess starts

#### `ExecuteSystemCommand()`
(executesystemcommand)=

Execute system command with profiling awareness

```cpp
int ExecuteSystemCommand(const std::string &command);
```

**Parameters**:
- `command` `const std::string &`: Command to execute

**Return Value**: `int` - Command exit code

**Note**: Automatically triggers subprocess monitoring if enabled

### Profiler Macro Detailed Reference
(profiler-macros-reference)=

#### `PROFILER_START_AUTO()`
(profiler-start-auto)=

Start performance profiling session for current method and automatically select label

```cpp
#define PROFILER_START_AUTO(enable_profiling)
```

**Parameters**:
- `enable_profiling` `bool`: Whether to enable performance profiling

**Notes**:
- Must be called in a method that inherits from MethodPresetProfiler
- Automatically uses structured labels or string labels from the method
- By default only tracks time to save overhead

**Usage Example**:
```cpp
PROFILER_START_AUTO(true);  // Automatically use method-configured labels
```

#### `PROFILER_START()`
(profiler-start)=

Start performance profiling session with string label (lock-free)

```cpp
#define PROFILER_START(label_str, enable_profiling)
```

**Parameters**:
- `label_str` `const char*` / `std::string`: Profiling label string
- `enable_profiling` `bool`: Whether to enable performance profiling

**Note**: Creates lock-free session with specified string label

**Usage Example**:
```cpp
PROFILER_START("experiment_1", true);  // Create "experiment_1" session
```

#### `POSDK_START()`
(posdk-start)=

Start performance profiling session with configurable metrics (lock-free)

```cpp
// Single parameter version (default metrics is "time")
#define POSDK_START(enable_profiling)

// Two parameter version
#define POSDK_START(enable_profiling, metrics_config)
```

**Parameters**:
- `enable_profiling` `bool`: Whether to enable performance profiling
- `metrics_config` `const char*` / `std::string`: Metrics configuration string (optional, default "time")

**Supported Metric Configurations**:
- `"time"` - Track time only
- `"time|memory"` - Time + memory
- `"time|memory|cpu"` - Time + memory + CPU
- `"ALL"` - All metrics

**Usage Example**:
```cpp
POSDK_START(true);                      // Default: time only
POSDK_START(true, "time|memory");       // Time + memory
POSDK_START(true, "time|memory|cpu");   // Time + memory + CPU
```

#### `PROFILER_START_STRUCTURED()`
(profiler-start-structured)=

Start performance profiling session with structured labels (lock-free)

```cpp
#define PROFILER_START_STRUCTURED(structured_labels, enable_profiling)
```

**Parameters**:
- `structured_labels` `std::unordered_map<std::string, std::string>`: Structured label mapping
- `enable_profiling` `bool`: Whether to enable performance profiling

**Usage Example**:
```cpp
std::unordered_map<std::string, std::string> labels = {
    {"pipeline", "PoSDK"},
    {"dataname", "test_data1"}
};
PROFILER_START_STRUCTURED(labels, true);
```

#### `PROFILER_START_WITH_METRICS()`
(profiler-start-with-metrics)=

Start performance profiling session with specific metrics (lock-free)

```cpp
#define PROFILER_START_WITH_METRICS(label_str, enable_profiling, metrics_config)
```

**Parameters**:
- `label_str` `const char*` / `std::string`: Profiling label string
- `enable_profiling` `bool`: Whether to enable performance profiling
- `metrics_config` `const char*` / `std::string`: Metrics configuration string

#### `PROFILER_START_WITH_SUBPROCESS()`
(profiler-start-with-subprocess)=

Start performance profiling session with subprocess monitoring (lock-free)

```cpp
#define PROFILER_START_WITH_SUBPROCESS(label_str, enable_profiling)
```

**Parameters**:
- `label_str` `const char*` / `std::string`: Profiling label string
- `enable_profiling` `bool`: Whether to enable performance profiling

**Note**: Monitors current process + all subprocesses (e.g., external executables like COLMAP, GLOMAP)

**Usage Example**:
```cpp
PROFILER_START_WITH_SUBPROCESS("glomap_pipeline", true);  // Include colmap+glomap resource usage
```

#### `PROFILER_END()`
(profiler-end)=

End performance profiling session (lock-free)

```cpp
#define PROFILER_END()
```

**Notes**:
- Automatically called when _profiler_session_ goes out of scope
- Saves session data to _current_profiler_info

#### `PROFILER_STAGE()`
(profiler-stage)=

Intelligent stage checkpoint (lock-free)

```cpp
#define PROFILER_STAGE(stage_name)
```

**Parameters**:
- `stage_name` `const char*` / `std::string`: Stage name for automatic interval analysis

**Notes**:
- Automatically measures performance metrics from the previous checkpoint to the current stage (lock-free operation)
- Calculates intervals: [START, STAGE1], [STAGE1, STAGE2], ..., [STAGEn, END]

**Usage Example**:
```cpp
PROFILER_STAGE("initialization");    // Measure [START, initialization]
PROFILER_STAGE("computation");       // Measure [initialization, computation]
PROFILER_STAGE("finalization");      // Measure [computation, finalization]
PROFILER_END();                      // Measure [finalization, END]
```

#### `PROFILER_SCOPE()`
(profiler-scope)=

Automatic performance profiler for entire method scope (lock-free)

```cpp
#define PROFILER_SCOPE(enable_profiling)
```

**Parameters**:
- `enable_profiling` `bool`: Whether to enable performance profiling

**Notes**:
- Creates RAII performance profiling session that automatically ends when scope exits
- Use this macro at the beginning of Run() method for complete method performance analysis
- Automatically uses labels from GetProfilerLabel()/GetProfilerLabels()

**Usage Example**:
```cpp
PROFILER_SCOPE(true);  // Use method-configured labels
```

#### `POSDK_SYSTEM()`
(posdk-system)=

Execute system command with optional subprocess monitoring

```cpp
// Single parameter version (subprocess monitoring disabled by default)
#define POSDK_SYSTEM(command)

// Two parameter version
#define POSDK_SYSTEM(command, enable_subprocess_monitoring)
```

**Parameters**:
- `command` `const char*` / `std::string`: Command string to execute
- `enable_subprocess_monitoring` `bool`: Whether to enable subprocess monitoring (optional, default false)

**Return Value**: `int` - Command exit code

**Notes**:
- When an active ProfilerSession exists, uses the session's metric configuration
- If enable_subprocess_monitoring=true, temporarily enables subprocess monitoring

**Usage Example**:
```cpp
int result = POSDK_SYSTEM("./command");              // Use session metrics, no subprocess monitoring
int result = POSDK_SYSTEM("./command", true);        // Use session metrics + subprocess monitoring
```

#### `POSDK_SYSTEM_AUTO()`
(posdk-system-auto)=

Execute system command with automatic performance profiling session management

```cpp
#define POSDK_SYSTEM_AUTO(command, enable_profiling)
```

**Parameters**:
- `command` `const char*` / `std::string`: Command string to execute
- `enable_profiling` `bool`: Whether to enable performance profiling

**Return Value**: `int` - Command exit code

**Notes**:
- Automatically creates a ProfilerSession that tracks time only, executes command, and ends session
- Equivalent to: PROFILER_START_AUTO + POSDK_SYSTEM + PROFILER_END
- Uses session's metric configuration, subprocess monitoring disabled by default

**Usage Example**:
```cpp
int result = POSDK_SYSTEM_AUTO("./intensive_workload 3 150 2 4 2", true);  // Complete performance profiling session
```

#### `PROFILER_PRINT_STATS()`
(profiler-print-stats)=

Print current session's performance profiling statistics

```cpp
#define PROFILER_PRINT_STATS(enable_profiling)
```

**Parameters**:
- `enable_profiling` `bool`: Whether performance profiling is enabled

**Notes**:
- Prints current session data from _current_profiler_info
- Must be called after PROFILER_END()

**Usage Example**:
```cpp
PROFILER_PRINT_STATS(true);
```

#### `PM_PRINT_STATS()`
(pm-print-stats)=

Print accumulated performance statistics from ProfilerManager

```cpp
#define PM_PRINT_STATS(label_str, enable_profiling)
```

**Parameters**:
- `label_str` `const char*` / `std::string`: Profiling label to query
- `enable_profiling` `bool`: Whether performance profiling is enabled

**Note**: Prints accumulated performance profiling data for the specified label in ProfilerManager

**Usage Example**:
```cpp
PM_PRINT_STATS("core_build_time", true);
```

#### `PROFILER_GET_CURRENT_TIME()`
(profiler-get-current-time)=

Get current session duration

```cpp
#define PROFILER_GET_CURRENT_TIME(enable_profiling)
```

**Parameters**:
- `enable_profiling` `bool`: Whether performance profiling is enabled

**Return Value**: `double` - Current session duration (milliseconds), returns 0.0 if unavailable

**Notes**:
- Returns duration from _current_profiler_info
- Must be called after PROFILER_END()

#### `PM_GET_AVG_TIME()`
(pm-get-avg-time)=

Get accumulated average duration from ProfilerManager

```cpp
#define PM_GET_AVG_TIME(label_str, enable_profiling)
```

**Parameters**:
- `label_str` `const char*` / `std::string`: Profiling label to query
- `enable_profiling` `bool`: Whether performance profiling is enabled

**Return Value**: `double` - Average duration (milliseconds), returns 0.0 if no data

**Note**: Returns accumulated average duration for all sessions of the specified label

---

## Performance Metrics Description

### Time Metric (TIME)
- **Measurement Content**: Wall-clock time from `PROFILER_START` to `PROFILER_END`
- **Precision**: Millisecond level, using `std::chrono::high_resolution_clock`
- **Use Cases**: Algorithm execution time analysis, function call duration analysis

### Memory Metric (MEMORY) - **Important: Now Incremental Statistics**
- **Measurement Content**: **Peak memory increment relative to start** (not absolute memory value)
- **Calculation Method**: 
  ```
  Peak Increment = max(memory observed during monitoring) - memory at start
  ```
- **Measurement Scope**: 
  - Default: Only current process RSS memory
  - With subprocess monitoring enabled: Current process + sum of all subprocess memory
- **Platform Implementation**:
  - **Windows**: `WorkingSetSize` (physical memory)
  - **macOS**: `resident_size` (RSS memory)
  - **Linux**: `VmRSS` in `/proc/self/status` (RSS memory)

### CPU Metric (CPU)
- **Measurement Content**: Process CPU time usage rate (user mode + kernel mode)
- **Calculation Method**: 
  ```
  CPU Usage = (end CPU time - start CPU time) / wall-clock time × 100%
  ```
- **Note**: Multi-threaded programs may exceed 100%
- **Platform Implementation**:
  - **Windows**: CPU time based on process handle (currently returns 0)
  - **macOS**: `getrusage(RUSAGE_SELF)` to get user mode + system mode time
  - **Linux**: utime+stime fields in `/proc/self/stat`

### Thread Count Metric (THREADS)
- **Measurement Content**: Process thread count (start/end/peak)
- **Platform Implementation**:
  - **Windows**: `CreateToolhelp32Snapshot` + `THREADENTRY32`
  - **macOS**: `task_threads()` to get accurate thread count
  - **Linux**: `Threads` field in `/proc/self/status`

### Subprocess Monitoring (SUBPROCESS)
- **Measurement Content**: Resource usage of all descendant processes in the current process tree
- **Use Cases**: Complete resource statistics when calling external tools (e.g., COLMAP, GLOMAP)
- **Implementation**:
  - **Windows**: `CreateToolhelp32Snapshot` to enumerate process tree
  - **macOS/Linux**: `ps` command + `awk` to recursively find descendant processes

---

## Typical Usage Examples

### Basic Performance Profiling Usage
```cpp
// Use in MethodPresetProfiler derived class
class MyAlgorithm : public MethodPresetProfiler {
public:
    DataPtr Run() override {
        // Automatically start performance profiling (using preset labels, default time only)
        PROFILER_START_AUTO(true);

        // Add stage checkpoints
        InitializeData();
        PROFILER_STAGE("initialization");

        ProcessData();
        PROFILER_STAGE("processing");

        // End profiling and print statistics
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return result;
    }
};
```
See also: [`PROFILER_START_AUTO()`](#profiler-start-auto) | [`PROFILER_STAGE()`](#profiler-stage) | [`PROFILER_END()`](#profiler-end) | [`PROFILER_PRINT_STATS()`](#profiler-print-stats)

### Advanced Performance Profiling Configuration
```cpp
// Performance profiling with configurable metrics
POSDK_START(true, "time|memory|cpu");  // Track time, memory, CPU

// Process data...
ComplexComputation();

// Subprocess monitoring (when calling external tools)
int result = POSDK_SYSTEM("./external_tool", true);  // Enable subprocess monitoring

PROFILER_END();
PROFILER_PRINT_STATS(true);
```
See also: [`POSDK_START()`](#posdk-start) | [`POSDK_SYSTEM()`](#posdk-system)

### Structured Labels and Stage Analysis
```cpp
// Use structured labels for experiment comparison
std::unordered_map<std::string, std::string> labels = {
    {"algorithm", "advanced_sfm"},
    {"dataset", "dataset_A"},
    {"experiment", "comparison_2025"}
};
PROFILER_START_STRUCTURED(labels, true);

// Multi-stage performance analysis
PROFILER_STAGE("feature_extraction");   // [START, feature_extraction]
PROFILER_STAGE("feature_matching");     // [feature_extraction, feature_matching]
PROFILER_STAGE("reconstruction");       // [feature_matching, reconstruction]

PROFILER_END();  // [reconstruction, END]
```
See also: [`PROFILER_START_STRUCTURED()`](#profiler-start-structured)

### System Command Performance Monitoring
```cpp
// System command with automatic performance profiling
int result = POSDK_SYSTEM_AUTO("./intensive_workload", true);

// Or manually manage session
PROFILER_START_AUTO(true);
int result1 = POSDK_SYSTEM("./command1");              // No subprocess monitoring
int result2 = POSDK_SYSTEM("./command2", true);        // Enable subprocess monitoring
PROFILER_END();
```
See also: [`POSDK_SYSTEM_AUTO()`](#posdk-system-auto)

### Accumulated Statistics and Data Export
```cpp
// View accumulated performance statistics
PM_PRINT_STATS("my_algorithm", true);

// Get average execution time
double avg_time = PM_GET_AVG_TIME("my_algorithm", true);

// Export performance data to CSV
ProfilerManager::GetInstance().ExportToCSV("performance_results.csv");

// Clear statistics
ProfilerManager::GetInstance().Clear();
```
See also: [`PM_PRINT_STATS()`](#pm-print-stats) | [`PM_GET_AVG_TIME()`](#pm-get-avg-time) | [`ExportToCSV()`](#exporttocsv) | [`Clear()`](#clear)

### Scope Automatic Management
```cpp
class MyMethod : public MethodPresetProfiler {
public:
    DataPtr Run() override {
        // Automatic performance profiling for entire method (RAII style)
        PROFILER_SCOPE(true);

        // Automatically statistics and submit when method ends
        return ProcessData();
    }
};
```
See also: [`PROFILER_SCOPE()`](#profiler-scope)

---

## Profiler Macro Usage Guide

### Basic Usage Macros

#### 1. `PROFILER_START_AUTO(enable_profiling)`
**Function**: Automatically start performance profiling using method preset labels
**Use Case**: In methods that inherit from `MethodPresetProfiler`
**Features**: Automatically selects string or structured labels, **optimized to disable subprocess monitoring by default** (saves overhead)

```cpp
// Use in MethodPresetProfiler derived class
class MyAlgorithm : public MethodPresetProfiler {
public:
    DataPtr Run() override {
        PROFILER_START_AUTO(true);  // Automatically use configured labels, subprocess monitoring disabled by default
        
        // Algorithm implementation...
        auto result = ProcessData();
        
        PROFILER_END();
        PROFILER_PRINT_STATS(true);
        return result;
    }
};
```

**Performance Optimization Notes**: 
- **Default Behavior**: Subprocess monitoring disabled to avoid unnecessary system call overhead
- **Dynamic Enable**: Subprocess monitoring is automatically enabled only when calling `POSDK_SYSTEM`
- **Backward Compatible**: Existing code requires no modification, automatically gains performance improvement

#### 2. `PROFILER_START(label_str, enable_profiling)`
**Function**: Start performance profiling with specified string label
**Parameters**: 
- `label_str`: Profiling label string
- `enable_profiling`: Whether to enable profiling (bool)

```cpp
void MyFunction() {
    PROFILER_START("my_custom_algorithm", true);
    
    // Execute time-consuming operations
    ComplexComputation();
    
    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

#### 3. `PROFILER_START_STRUCTURED(structured_labels, enable_profiling)`
**Function**: Start performance profiling with structured labels, supports CSV export
**Purpose**: Generate CSV statistics tables with custom column headers

```cpp
void ExperimentFunction() {
    std::unordered_map<std::string, std::string> labels = {
        {"pipeline", "PoSDK"},
        {"dataset", "synthetic_data"},
        {"algorithm", "advanced_sfm"},
        {"version", "v2.1"}
    };
    
    PROFILER_START_STRUCTURED(labels, true);
    
    // Experiment code
    RunExperiment();
    
    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

### Advanced Feature Macros

#### 4. `PROFILER_START_WITH_METRICS(label_str, enable_profiling, metrics_config)`
**Function**: Selectively enable specific performance metrics
**Parameters**:
- `metrics_config`: Metrics configuration string, separated by `|`
  - Options: `"time"`, `"memory"`, `"cpu"`, `"threads"`, `"subprocess"`, `"ALL"`

```cpp
void LightweightProfiling() {
    // Track time and memory only, skip CPU and thread statistics
    PROFILER_START_WITH_METRICS("lightweight_test", true, "time|memory");
    
    FastOperation();
    
    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

#### 5. `PROFILER_START_WITH_SUBPROCESS(label_str, enable_profiling)`
**Function**: Complete resource monitoring including subprocesses
**Use Case**: Complete performance statistics when calling external programs

```cpp
void CallExternalTool() {
    PROFILER_START_WITH_SUBPROCESS("colmap_pipeline", true);
    
    // Call external tools, will track subprocess resources
    POSDK_SYSTEM("COLMAP feature_extractor --database_path database.db");
    POSDK_SYSTEM("COLMAP exhaustive_matcher --database_path database.db");
    
    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

### Stage Analysis Macros

#### 6. `PROFILER_STAGE(stage_name)`
**Function**: Add stage checkpoint, automatically calculate interval performance
**Features**: Lock-free operation, automatically generates interval statistics

```cpp
void MultiStageAlgorithm() {
    PROFILER_START("multi_stage_sfm", true);
    
    InitializeData();
    PROFILER_STAGE("initialization");    // [START, initialization]
    
    ExtractFeatures();
    PROFILER_STAGE("feature_extraction"); // [initialization, feature_extraction]
    
    MatchFeatures();
    PROFILER_STAGE("feature_matching");   // [feature_extraction, feature_matching]
    
    ReconstructScene();
    PROFILER_STAGE("reconstruction");     // [feature_matching, reconstruction]
    
    PROFILER_END();                      // [reconstruction, END]
    PROFILER_PRINT_STATS(true);
}
```

### Scope Analysis Macros

#### 7. `PROFILER_SCOPE(enable_profiling)`
**Function**: RAII-style automatic scope performance profiling
**Features**: Automatically submits statistics when method ends

```cpp
class MyAlgorithm : public MethodPresetProfiler {
public:
    DataPtr Run() override {
        PROFILER_SCOPE(true);  // Automatic analysis for entire method
        
        // Method implementation...
        auto result = ProcessComplexData();
        
        // Automatically statistics and submit when method ends
        return result;
    }
};
```

### System Command Macros

#### 8. `POSDK_SYSTEM(command [, enable_profiling])`
**Function**: System command execution with profiling awareness
**Parameters**: 
- `command`: Command string to execute
- `enable_profiling`: Optional, defaults to true
**Features**: **Automatically dynamically enables subprocess monitoring**, performs pre/post execution sampling

```cpp
void RunExternalCommands() {
    PROFILER_START_AUTO(true);  // Subprocess monitoring disabled by default, saves overhead
    
    // POSDK_SYSTEM automatically enables subprocess monitoring, performs pre/post execution sampling
    int result1 = POSDK_SYSTEM("./heavy_computation --input data.txt");
    int result2 = POSDK_SYSTEM("python process_results.py", true);
    int result3 = POSDK_SYSTEM("echo 'simple command'", false);  // Disable monitoring
    
    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

**Smart Optimization**: 
- **Enable on Demand**: Subprocess monitoring is enabled only when calling `POSDK_SYSTEM`
- **Automatic Switch**: Automatically switches from current process monitoring to process tree monitoring
- **Performance Improvement**: Avoids expensive subprocess system calls when not needed

#### 9. `POSDK_SYSTEM_AUTO(command, enable_profiling)`
**Function**: Automatically create performance profiling session and execute command
**Features**: Equivalent to `PROFILER_START_AUTO` + `POSDK_SYSTEM` + `PROFILER_END`

```cpp
void QuickSystemCommand() {
    // Automatically create session, execute command, print statistics
    int result = POSDK_SYSTEM_AUTO("./intensive_workload 3 150 2 4 2", true);
}
```

### Statistics Display Macros

#### 10. `PROFILER_PRINT_STATS(enable_profiling)`
**Function**: Print current session's performance statistics
**Usage**: Must be called after `PROFILER_END()`

#### 11. `PM_PRINT_STATS(label_str, enable_profiling)`
**Function**: Print accumulated performance statistics for specified label
**Features**: Displays aggregated data from multiple runs

```cpp
void ShowAccumulatedStats() {
    // Display accumulated statistics for "my_algorithm" label
    PM_PRINT_STATS("my_algorithm", true);
}
```

## MethodPresetProfiler Integration Examples

### Basic Integration
```cpp
class MySfMAlgorithm : public MethodPresetProfiler 
{
public:
    MySfMAlgorithm() {
        // Set profiling label
        SetProfilerLabel("advanced_sfm_v2.1");
        
        // Or use structured labels (recommended for experiment comparison)
        SetProfilerLabels({
            {"pipeline", "PoSDK"}, 
            {"algorithm", "advanced_sfm"},
            {"version", "v2.1"},
            {"dataset", "synthetic"}
        });
    }
    
    const std::string& GetType() const override {
        static std::string type = "AdvancedSfMAlgorithm";
        return type;
    }
    
    DataPtr Run() override {
        // Method 1: Auto label + complete monitoring
        PROFILER_START_AUTO(true);
        
        // Initialization
        auto cameras = InitializeCameras();
        PROFILER_STAGE("camera_initialization");
        
        // Feature extraction
        auto features = ExtractFeatures();
        PROFILER_STAGE("feature_extraction");
        
        // Feature matching
        auto matches = MatchFeatures(features);
        PROFILER_STAGE("feature_matching");
        
        // Call external tool for reconstruction
        int result = POSDK_SYSTEM("COLMAP mapper --database_path temp.db");
        PROFILER_STAGE("external_reconstruction");
        
        // Post-processing
        auto final_result = PostProcessing();
        PROFILER_STAGE("post_processing");
        
        PROFILER_END();
        PROFILER_PRINT_STATS(true);
        
        return final_result;
    }
    
private:
    DataPtr InitializeCameras() { /* Implementation... */ }
    DataPtr ExtractFeatures() { /* Implementation... */ }
    DataPtr MatchFeatures(const DataPtr& features) { /* Implementation... */ }
    DataPtr PostProcessing() { /* Implementation... */ }
};
```

### Experiment Comparison Example
```cpp
class ExperimentRunner {
public:
    void RunComparison() {
        // Experiment 1: Basic algorithm
        {
            MySfMAlgorithm algo1;
            algo1.SetProfilerLabels({
                {"algorithm", "basic_sfm"},
                {"dataset", "dataset_A"},
                {"experiment", "comparison_2025"}
            });
            auto result1 = algo1.Build(input_data);
        }
        
        // Experiment 2: Improved algorithm
        {
            MySfMAlgorithm algo2;
            algo2.SetProfilerLabels({
                {"algorithm", "advanced_sfm"},
                {"dataset", "dataset_A"}, 
                {"experiment", "comparison_2025"}
            });
            auto result2 = algo2.Build(input_data);
        }
        
        // Export comparison data
        ProfilerManager::GetInstance().ExportToCSV("experiment_results.csv");
    }
};
```

## Performance Statistics Output Description

### Console Output Example
```
=== advanced_sfm_v2.1 Current Session Performance Statistics ===
Duration: 2847.32 ms
Memory Increment: 156.8 MB (relative to start)
CPU Usage: 287.4%
Thread Count (start/end): 1/8
Subprocess Count (detected/peak): 3/5

=== Stage Interval Statistics ===
  START_to_camera_initialization: 145.23 ms
  camera_initialization_to_feature_extraction: 856.45 ms
  feature_extraction_to_feature_matching: 1243.67 ms
  feature_matching_to_external_reconstruction: 445.89 ms
  external_reconstruction_to_post_processing: 156.08 ms
  post_processing_to_END: 45.67 ms
```

### CSV Export Format

#### Main File (experiment_results.csv)
| algorithm    | dataset   | experiment      | Label                                                               | Session Count | Profiling Threads | Total Time(ms) | Average Time(ms) | Min Time(ms) | Max Time(ms) | Peak Memory(MB) | Average CPU(%) | Process Peak Threads | Total Subprocesses | Peak Subprocesses |
| ------------ | --------- | --------------- | ------------------------------------------------------------------- | ------------- | ----------------- | -------------- | ---------------- | ------------ | ------------ | --------------- | -------------- | -------------------- | ------------------ | ----------------- |
| basic_sfm    | dataset_A | comparison_2025 | algorithm=basic_sfm_dataset=dataset_A_experiment=comparison_2025    | 5             | 2                 | 12847.32       | 2569.46          | 2234.12      | 2847.32      | 156.8           | 245.6          | 8                    | 15                 | 5                 |
| advanced_sfm | dataset_A | comparison_2025 | algorithm=advanced_sfm_dataset=dataset_A_experiment=comparison_2025 | 5             | 2                 | 11234.67       | 2246.93          | 2156.78      | 2334.12      | 142.3           | 267.8          | 12                   | 18                 | 6                 |

#### Stage File (experiment_results_stages.csv) 
| Label                                                            | Interval Name                               | Total Duration(ms) | Average Duration(ms) | Count |
| ---------------------------------------------------------------- | ------------------------------------------- | ------------------ | -------------------- | ----- |
| algorithm=basic_sfm_dataset=dataset_A_experiment=comparison_2025 | START_to_camera_initialization              | 725.65             | 145.13               | 5     |
| algorithm=basic_sfm_dataset=dataset_A_experiment=comparison_2025 | camera_initialization_to_feature_extraction | 4234.56            | 846.91               | 5     |

### CSV Header Description

#### Main Statistics File Headers
- **Dynamic Structured Label Columns** (e.g., algorithm, dataset, experiment): User-defined structured labels
- **Label**: Profiling label (structured label combination or user-set string)
- **Session Count**: Total number of sessions for this label
- **Profiling Threads**: Number of different threads participating in profiling
- **Total Time(ms)**: Total time for all sessions (milliseconds)
- **Average Time(ms)**: Average time per session (milliseconds)
- **Min Time(ms)**: Shortest session time (milliseconds)
- **Max Time(ms)**: Longest session time (milliseconds)
- **Peak Memory(MB)**: Peak memory increment (MB, relative to start)
- **Average CPU(%)**: Average CPU usage rate (%, may exceed 100%)
- **Process Peak Threads**: Process peak thread count
- **Total Subprocesses**: Total number of detected subprocesses
- **Peak Subprocesses**: Peak subprocess count

#### Stage Statistics File Headers
- **Label**: Profiling label
- **Interval Name**: Interval name (e.g., START_to_stage1, stage1_to_stage2)
- **Total Duration(ms)**: Total duration for this interval (accumulated across all sessions)
- **Average Duration(ms)**: Average duration for this interval
- **Count**: Number of statistics for this interval

## Important Notes

### Memory Statistics Description
- **Peak memory is now incremental statistics**, not absolute memory value
- Calculation method: `Peak Memory = max(memory during monitoring) - memory at start`
- More suitable for analyzing actual algorithm memory consumption, excluding program base memory impact

### Performance Overhead
- Slight performance overhead when all metrics are enabled (usually <1-2%)
- Subprocess monitoring adds additional overhead
- Recommend disabling detailed monitoring in release versions

### Platform Compatibility
- All features fully supported on macOS and Linux
- Windows platform has limited CPU monitoring functionality
- Subprocess monitoring implementation differs across platforms but interface is consistent

### Usage Recommendations
1. **Development Phase**: Use `PROFILER_START_AUTO` + complete metric monitoring (optimized, subprocess monitoring disabled by default)
2. **Performance Tuning**: Use `PROFILER_STAGE` for fine-grained analysis
3. **Experiment Comparison**: Use structured labels + CSV export
4. **Production Environment**: Selectively enable key metrics, avoid comprehensive monitoring
5. **External Tool Calls**: Use `POSDK_SYSTEM` to automatically enable subprocess monitoring for complete resource statistics

By properly using these performance profiling tools, developers can effectively identify performance bottlenecks, optimize algorithm implementations, and conduct scientific experiment comparison analysis.