# Profiler

## View performance in the GUI

1. Select a method node and enable `enable_profiling` in **Configuration**. Enable `enable_core_profiling` as well if the method provides core-algorithm profiling.
2. Save and run the workflow. When it finishes, click **Performance** in the toolbar.
3. Select **Method Time**, **Core Time**, or **Stages**. Click a column heading to sort; use the decimal-place control to change display precision.
4. Click **Export** to save CSV, or open **Export → Performance Report** to preview or export the complete report.

Results are saved in the project's output directory. Opening a project or switching output directories restores the latest saved results for that directory. Starting a new run clears the previous display.

## Read the statistics

| Field | Meaning |
| --- | --- |
| `Total Time(ms)` | Accumulated session time for a label, including child calls. |
| `Session Count` | Number of submitted timing sessions. |
| `Logical Run Count` | Logical runs explicitly registered by the method. |
| `Average Time(ms)` | Total time divided by `Average Time Count`: logical runs when available, otherwise session count. |
| `Average CPU(%)` | Arithmetic mean of valid session CPU percentages; multithreaded work may exceed 100%. |
| `CPU Sample Count` | Number of valid CPU-measured sessions, also used to weight batch CPU averages. |
| `Peak Memory(MB)` | Absolute peak process memory observed by sampling, rather than an increase from the initial value. |
| Stage `Count` | Number of occurrences of the named interval, including repeats within one session. |
| Process / batch wall time | Elapsed time measured by the GUI from process launch to completion, or from batch start until all tasks finish. |

Method and core sessions can nest or run in parallel. Their accumulated times overlap and must not be added to obtain workflow wall time. Unmeasured values appear as `N/A`; `0` means a measured zero.

## Reconcile workflow wall time

Each run's result directory contains:

- `workflow_timing.json`: process wall time and exit status recorded by the GUI.
- `runner-events.jsonl`: node start, completion and run completion events; use `monotonic_elapsed_ms` to calculate node intervals.

Add non-overlapping node intervals to process startup, evaluation/output work between nodes and shutdown to reconcile process wall time. Batch wall time also includes task transitions and result aggregation. Method, core and stage tables locate different costs; do not add their totals across tables.

## Instrument a method

Use the following inside a method derived from `MethodPreset`:

```cpp
DataPtr MyMethod::Run()
{
    // Use the method's configured label; measure time only by default.
    PROFILER_START_AUTO(GetOptionAsBool("enable_profiling", false));

    PrepareData();
    PROFILER_STAGE("prepared");
    Solve();
    PROFILER_STAGE("solved");

    PROFILER_END();
    return result;
}
```

To collect memory, CPU, and thread metrics, replace the start statement with:

```cpp
POSDK_START(GetOptionAsBool("enable_profiling", false), "time|memory|cpu|threads");
```

Use one start macro per scope. `PROFILER_START_AUTO` and `POSDK_START` use the current method's label, rather than the C++ function name. In a free function, supply a label explicitly:

```cpp
void ProcessData()
{
    PROFILER_START("process_data", true);
    // Process data.
    PROFILER_STAGE("processed");
    PROFILER_END();
}
```

For scope timing without an explicit end macro, construct a session directly:

```cpp
#include <po_core/ProfilerManager.hpp>

void ProcessBlock()
{
    PoSDK::Profiler::ProfilerSession session(
        "process_block", PoSDK::Profiler::MetricsType::TIME);
    // Timing is submitted when the session leaves scope.
}
```

## Save CSV

GUI runs collect profiling files automatically. A standalone C++ program can export after its sessions finish:

```cpp
auto &profiler = PoSDK::Profiler::ProfilerManager::GetInstance();
profiler.ExportToCSV("profile.csv");
```

The summary is saved in `profile.csv` and stages in `profile_stages.csv`. GUI exports also write wall-time metadata to `profile.meta.json`; retain all three files when moving or importing results. Batch exports preserve method/core scope and experiment dimensions.

Legacy CSV files remain readable. If a legacy file has no CPU sample count, its session count is used for compatibility. Missing CPU measurements or discarded stages require a new run.

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

- **TIME**: Elapsed session time; stage intervals accumulate in checkpoint order.
- **MEMORY**: Absolute peak process memory observed by sampling. With subprocess monitoring, monitored descendants are included. Peaks shorter than the sampling interval may be missed.
- **CPU**: Process CPU-time change divided by session elapsed time. Valid sessions are aggregated using `CPU Sample Count`, independently of logical run count.
- **THREADS**: Process thread count and the sampled peak.
- **SUBPROCESS**: Process-tree resource monitoring for external programs. Use `POSDK_SYSTEM(command, true)` when invoking them.

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

#### 8. `POSDK_SYSTEM(command [, enable_subprocess_monitoring])`
**Function**: System command execution with profiling awareness
**Parameters**:
- `command`: Command string to execute
- `enable_subprocess_monitoring`: Optional, whether to enable subprocess monitoring (defaults to false)
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

### Summary

The summary contains dynamic label columns, `Label`, `Profile Scope`, `Average Time Count`, `Total Time(ms)`, `Average Time(ms)`, `Session Count`, and `Logical Run Count`. Memory, CPU, thread, and subprocess columns depend on the enabled metrics. CPU exports also include `CPU Sample Count`.

`Profile Scope` preserves `method` or `core` classification. GUI batch exports also preserve `Experiment Dimensions`. Mean time uses its exported denominator, CPU uses valid measured-session counts, and peak metrics use the maximum.

### Stages

Stage exports contain labels, scope, `Interval Name`, `Total Duration(ms)`, `Average Duration(ms)`, and `Count`, retaining experiment dimensions in batch runs.

Repeated occurrences of an `A_to_B` interval accumulate their durations and increment `Count`. The average is total interval duration divided by the number of occurrences.

### Choose the measurement scope

Use the default time metric for timing comparisons. Enable memory, CPU, thread, or subprocess sampling when those measurements are needed. Keep metric settings identical across algorithm comparisons and use the GUI's separately measured wall time for end-to-end timing.
