# 性能分析器 (Profiler)

## 在 GUI 中查看性能

1. 选中方法节点，在 **Configuration** 中开启 `enable_profiling`。需要核心算法统计时，同时开启该方法提供的 `enable_core_profiling`。
2. 保存并运行工作流。运行完成后，点击工具栏的 **性能**。
3. 在左侧选择 **方法时间**、**核心时间** 或 **阶段**。点击列标题排序，用上方的小数位设置调整显示精度。
4. 点击 **导出** 保存 CSV；从工具栏 **导出 → 性能报告** 查看或导出完整报告。

结果保存在当前工程的输出目录中。重新打开工程或切换输出目录时，会读取对应目录最近保存的结果。新一轮运行开始时会清空上一轮显示。

## 怎样读这些数值

| 字段 | 含义 |
| --- | --- |
| `Total Time(ms)` | 同一标签所有会话的累计时间，包含子调用。 |
| `Session Count` | 实际提交的计时会话数。 |
| `Logical Run Count` | 方法主动登记的逻辑运行次数。 |
| `Average Time(ms)` | 累计时间除以 `Average Time Count`；有逻辑运行次数时优先用逻辑次数，否则用会话数。 |
| `Average CPU(%)` | 有效 CPU 测量会话的百分比算术平均值，多线程运行可以超过 100%。 |
| `CPU Sample Count` | 参与 CPU 平均值的有效会话数。批量运行按此数量加权汇总。 |
| `Peak Memory(MB)` | 采样期间进程的绝对峰值内存，不是相对开始时的增量。 |
| 阶段 `Count` | 该命名区间实际出现的次数；同一会话内重复出现也会累加。 |
| 运行／批次墙钟 | GUI 从启动计算进程到进程结束，或从批次开始到所有任务完成的实际经过时间。 |

方法和核心会话可能嵌套或并行，累计时间会重叠，不能把这些行相加当作工作流的总耗时。未采集的指标显示 `N/A`；数值 `0` 表示采集结果为零。

## 核对工作流墙钟

在每次运行的结果目录中查看：

- `workflow_timing.json`：GUI 记录的计算进程墙钟时间和退出状态。
- `runner-events.jsonl`：节点开始、完成及运行结束记录；`monotonic_elapsed_ms` 用于计算节点区间耗时。

将互不重叠的节点区间相加，再加上进程启动、节点间评估／结果保存和收尾时间，才能与进程墙钟对应。批次墙钟还包含任务切换和统计汇总时间。方法、核心和阶段统计各自用于定位开销，不要跨表相加。

## 给方法加入计时

在继承 `MethodPreset` 的方法中使用：

```cpp
DataPtr MyMethod::Run()
{
    // 使用方法配置的标签，默认只测量时间。
    PROFILER_START_AUTO(GetOptionAsBool("enable_profiling", false));

    PrepareData();
    PROFILER_STAGE("prepared");
    Solve();
    PROFILER_STAGE("solved");

    PROFILER_END();
    return result;
}
```

需要内存、CPU 或线程指标时，将开始语句替换为：

```cpp
POSDK_START(GetOptionAsBool("enable_profiling", false), "time|memory|cpu|threads");
```

每个作用域只放一个开始宏。`PROFILER_START_AUTO` 和 `POSDK_START` 使用当前方法的标签，不会自动取 C++ 函数名。普通函数可使用显式标签：

```cpp
void ProcessData()
{
    PROFILER_START("process_data", true);
    // 执行处理。
    PROFILER_STAGE("processed");
    PROFILER_END();
}
```

只需作用域计时、不需要显式结束宏时，也可以直接创建会话：

```cpp
#include <po_core/ProfilerManager.hpp>

void ProcessBlock()
{
    PoSDK::Profiler::ProfilerSession session(
        "process_block", PoSDK::Profiler::MetricsType::TIME);
    // 离开作用域时自动提交计时结果。
}
```

## 保存 CSV

GUI 运行会自动收集性能文件。独立 C++ 程序可在会话结束后导出：

```cpp
auto &profiler = PoSDK::Profiler::ProfilerManager::GetInstance();
profiler.ExportToCSV("profile.csv");
```

主表保存在 `profile.csv`，阶段表保存在 `profile_stages.csv`。GUI 导出还会生成保存墙钟时间的 `profile.meta.json`；移动或导入结果时保留这三个文件。批量结果同时保留方法／核心分类和实验维度。

旧版 CSV 仍可读取；旧文件未保存的 CPU 样本数按会话数兼容处理，已经丢失的阶段或 CPU 数据需要重新运行才能补齐。

## 快速功能导航

### 快速入门
(quick-start-profiler)=

- **基础宏**: [`PROFILER_START_AUTO()`](#profiler-start-auto) | [`PROFILER_END()`](#profiler-end) | [`PROFILER_PRINT_STATS()`](#profiler-print-stats)
- **高级宏**: [`POSDK_START()`](#posdk-start) | [`POSDK_SYSTEM()`](#posdk-system) | [`PROFILER_STAGE()`](#profiler-stage)
- **管理函数**: [`GetInstance()`](#getinstance) | [`SetProfilingEnabled()`](#setprofilingenabled) | [`ExportToCSV()`](#exporttocsv)

### 性能分析宏分类
(profiler-macros-navigation)=

| 宏分类           | 核心宏                                                                | 说明                   |
| ---------------- | --------------------------------------------------------------------- | ---------------------- |
| **自动标签宏**   | [`PROFILER_START_AUTO()`](#profiler-start-auto)                       | 自动使用方法预设标签   |
|                  | [`PROFILER_SCOPE()`](#profiler-scope)                                 | RAII风格自动作用域分析 |
| **手动标签宏**   | [`PROFILER_START()`](#profiler-start)                                 | 使用字符串标签         |
|                  | [`PROFILER_START_STRUCTURED()`](#profiler-start-structured)           | 使用结构化标签         |
| **高级配置宏**   | [`POSDK_START()`](#posdk-start)                                       | 可配置指标类型         |
|                  | [`PROFILER_START_WITH_METRICS()`](#profiler-start-with-metrics)       | 指定特定指标           |
|                  | [`PROFILER_START_WITH_SUBPROCESS()`](#profiler-start-with-subprocess) | 包含子进程监控         |
| **系统命令宏**   | [`POSDK_SYSTEM()`](#posdk-system)                                     | 性能感知的系统命令     |
|                  | [`POSDK_SYSTEM_AUTO()`](#posdk-system-auto)                           | 自动会话管理           |
| **阶段分析宏**   | [`PROFILER_STAGE()`](#profiler-stage)                                 | 阶段检查点             |
| **结束和输出宏** | [`PROFILER_END()`](#profiler-end)                                     | 结束会话               |
|                  | [`PROFILER_PRINT_STATS()`](#profiler-print-stats)                     | 打印当前会话统计       |
|                  | [`PM_PRINT_STATS()`](#pm-print-stats)                                 | 打印累积统计           |

### ProfilerManager管理函数
(profiler-manager-navigation)=

| 功能分类     | 核心函数                                                | 说明         |
| ------------ | ------------------------------------------------------- | ------------ |
| **实例管理** | [`GetInstance()`](#getinstance)                         | 获取单例实例 |
| **状态控制** | [`IsProfilingEnabled()`](#isprofilingenabled)           | 检查启用状态 |
|              | [`SetProfilingEnabled()`](#setprofilingenabled)         | 设置启用状态 |
| **数据提交** | [`SubmitSessionData()`](#submitsessiondata)             | 提交会话数据 |
| **数据查询** | [`GetTotalProfilerInfo()`](#gettotalprofilerinfo)       | 获取累积数据 |
| **数据导出** | [`ExportToCSV()`](#exporttocsv)                         | 导出CSV文件  |
| **数据管理** | [`DisplayAllProfilingData()`](#displayallprofilingdata) | 显示所有数据 |
|              | [`Clear()`](#clear)                                     | 清空数据     |

### ProfilerSession会话函数
(profiler-session-navigation)=

| 功能分类       | 核心函数                                                            | 说明           |
| -------------- | ------------------------------------------------------------------- | -------------- |
| **会话状态**   | [`IsActive()`](#isactive)                                           | 检查会话状态   |
|                | [`IsMetricsEnabled()`](#ismetricsenabled)                           | 检查指标启用   |
|                | [`IsSubprocessMonitoringEnabled()`](#issubprocessmonitoringenabled) | 检查子进程监控 |
| **子进程控制** | [`EnableSubprocessMonitoring()`](#enablesubprocessmonitoring)       | 启用子进程监控 |
|                | [`DisableSubprocessMonitoring()`](#disablesubprocessmonitoring)     | 禁用子进程监控 |
|                | [`NotifySubprocessStartup()`](#notifysubprocessstartup)             | 通知子进程启动 |
| **阶段管理**   | [`AddStageCheckpoint()`](#addstagecheckpoint)                       | 添加阶段检查点 |
| **系统命令**   | [`ExecuteSystemCommand()`](#executesystemcommand)                   | 执行系统命令   |
| **数据访问**   | [`GetSessionData()`](#getsessiondata)                               | 获取会话数据   |

---

## 完整函数与宏参考

### ProfilerManager 核心函数
(profiler-manager-functions)=

#### `GetInstance()`
(getinstance)=

获取ProfilerManager单例实例

```cpp
static ProfilerManager &GetInstance();
```

**返回值**: `ProfilerManager &` - 单例实例的引用

**使用示例**:
```cpp
auto& manager = ProfilerManager::GetInstance();
manager.SetProfilingEnabled("my_algorithm", true);
```

#### `IsProfilingEnabled()`
(isprofilingenabled)=

检查方法类型是否启用了性能分析

```cpp
bool IsProfilingEnabled(const std::string &method_type) const;
```

**参数**:
- `method_type` `const std::string &`：要检查的方法类型

**返回值**: `bool` - 如果启用了性能分析则返回true

#### `SetProfilingEnabled()`
(setprofilingenabled)=

为方法类型启用或禁用性能分析

```cpp
void SetProfilingEnabled(const std::string &method_type, bool enabled);
```

**参数**:
- `method_type` `const std::string &`：方法类型
- `enabled` `bool`：是否启用性能分析

#### `SubmitSessionData()`
(submitsessiondata)=

提交完成的性能分析会话数据

```cpp
void SubmitSessionData(const ProfilerInfo &session_data);
```

**参数**:
- `session_data` `const ProfilerInfo &`：完整的会话数据

**说明**: 这是唯一修改全局状态的函数，每个会话只调用一次

#### `GetTotalProfilerInfo()`
(gettotalprofilerinfo)=

获取标签的累积性能分析数据

```cpp
TotalProfilerInfo GetTotalProfilerInfo(const std::string &label_str) const;
```

**参数**:
- `label_str` `const std::string &`：性能分析标签

**返回值**: `TotalProfilerInfo` - 累积性能分析数据的副本

#### `DisplayAllProfilingData()`
(displayallprofilingdata)=

显示所有累积的性能分析数据

```cpp
void DisplayAllProfilingData() const;
```

**说明**: 显示所有已分析会话的性能统计

#### `ExportToCSV()`
(exporttocsv)=

导出性能分析数据到CSV文件

```cpp
bool ExportToCSV(const std::string &file_path) const;
```

**参数**:
- `file_path` `const std::string &`：CSV文件路径

**返回值**: `bool` - 如果导出成功则返回true

#### `Clear()`
(clear)=

清空所有性能分析数据

```cpp
void Clear();
```

**说明**: 重置所有累积统计

### ProfilerSession 核心函数
(profiler-session-functions)=

#### `IsActive()`
(isactive)=

检查会话是否活跃

```cpp
bool IsActive() const;
```

**返回值**: `bool` - 如果会话活跃则返回true

#### `IsMetricsEnabled()`
(ismetricsenabled)=

检查特定指标类型是否启用

```cpp
bool IsMetricsEnabled(MetricsType type) const;
```

**参数**:
- `type` `MetricsType`：要检查的指标类型

**返回值**: `bool` - 如果启用则返回true

#### `IsSubprocessMonitoringEnabled()`
(issubprocessmonitoringenabled)=

检查子进程监控是否启用

```cpp
bool IsSubprocessMonitoringEnabled() const;
```

**返回值**: `bool` - 如果启用则返回true

#### `EnableSubprocessMonitoring()`
(enablesubprocessmonitoring)=

动态启用子进程监控

```cpp
void EnableSubprocessMonitoring();
```

**说明**: 从当前进程监控切换到进程树监控

#### `DisableSubprocessMonitoring()`
(disablesubprocessmonitoring)=

禁用子进程监控

```cpp
void DisableSubprocessMonitoring();
```

**说明**: 临时禁用子进程监控而不改变其他指标

#### `AddStageCheckpoint()`
(addstagecheckpoint)=

添加智能阶段检查点

```cpp
void AddStageCheckpoint(const std::string &stage_name);
```

**参数**:
- `stage_name` `const std::string &`：阶段名称

**说明**: 自动在此检查点捕获性能指标（无锁操作）

#### `GetSessionData()`
(getsessiondata)=

获取当前会话数据副本

```cpp
ProfilerInfo GetSessionData() const;
```

**返回值**: `ProfilerInfo` - 当前会话数据的副本

**说明**: 返回session_data_的副本，用于会话结束后使用

#### `NotifySubprocessStartup()`
(notifysubprocessstartup)=

通知会话子进程启动

```cpp
void NotifySubprocessStartup(const std::string &command);
```

**参数**:
- `command` `const std::string &`：正在执行的命令

**说明**: 子进程启动时触发立即采样

#### `ExecuteSystemCommand()`
(executesystemcommand)=

执行带性能分析感知的系统命令

```cpp
int ExecuteSystemCommand(const std::string &command);
```

**参数**:
- `command` `const std::string &`：要执行的命令

**返回值**: `int` - 命令退出码

**说明**: 如果启用则自动触发子进程监控

### 性能分析宏详细参考
(profiler-macros-reference)=

#### `PROFILER_START_AUTO()`
(profiler-start-auto)=

为当前方法开始性能分析会话并自动选择标签

```cpp
#define PROFILER_START_AUTO(enable_profiling)
```

**参数**:
- `enable_profiling` `bool`：是否启用性能分析

**说明**:
- 必须在继承自MethodPresetProfiler的方法中调用
- 自动使用方法中的结构化标签或字符串标签
- 默认只统计时间，节省开销

**使用示例**:
```cpp
PROFILER_START_AUTO(true);  // 自动使用方法配置的标签
```

#### `PROFILER_START()`
(profiler-start)=

使用字符串标签开始性能分析会话（无锁）

```cpp
#define PROFILER_START(label_str, enable_profiling)
```

**参数**:
- `label_str` `const char*` / `std::string`：性能分析标签字符串
- `enable_profiling` `bool`：是否启用性能分析

**说明**: 使用指定字符串标签创建无锁会话

**使用示例**:
```cpp
PROFILER_START("experiment_1", true);  // 创建"experiment_1"会话
```

#### `POSDK_START()`
(posdk-start)=

使用可配置指标开始性能分析会话（无锁）

```cpp
// 一个参数版本（默认metrics为"time"）
#define POSDK_START(enable_profiling)

// 两个参数版本
#define POSDK_START(enable_profiling, metrics_config)
```

**参数**:
- `enable_profiling` `bool`：是否启用性能分析
- `metrics_config` `const char*` / `std::string`：指标配置字符串（可选，默认"time"）

**支持的指标配置**:
- `"time"` - 只统计时间
- `"time|memory"` - 时间+内存
- `"time|memory|cpu"` - 时间+内存+CPU
- `"ALL"` - 所有指标

**使用示例**:
```cpp
POSDK_START(true);                      // 默认只统计时间
POSDK_START(true, "time|memory");       // 时间+内存
POSDK_START(true, "time|memory|cpu");   // 时间+内存+CPU
```

#### `PROFILER_START_STRUCTURED()`
(profiler-start-structured)=

使用结构化标签开始性能分析会话（无锁）

```cpp
#define PROFILER_START_STRUCTURED(structured_labels, enable_profiling)
```

**参数**:
- `structured_labels` `std::unordered_map<std::string, std::string>`：结构化标签映射
- `enable_profiling` `bool`：是否启用性能分析

**使用示例**:
```cpp
std::unordered_map<std::string, std::string> labels = {
    {"pipeline", "PoSDK"},
    {"dataname", "test_data1"}
};
PROFILER_START_STRUCTURED(labels, true);
```

#### `PROFILER_START_WITH_METRICS()`
(profiler-start-with-metrics)=

使用特定指标开始性能分析会话（无锁）

```cpp
#define PROFILER_START_WITH_METRICS(label_str, enable_profiling, metrics_config)
```

**参数**:
- `label_str` `const char*` / `std::string`：性能分析标签字符串
- `enable_profiling` `bool`：是否启用性能分析
- `metrics_config` `const char*` / `std::string`：指标配置字符串

#### `PROFILER_START_WITH_SUBPROCESS()`
(profiler-start-with-subprocess)=

使用子进程监控开始性能分析会话（无锁）

```cpp
#define PROFILER_START_WITH_SUBPROCESS(label_str, enable_profiling)
```

**参数**:
- `label_str` `const char*` / `std::string`：性能分析标签字符串
- `enable_profiling` `bool`：是否启用性能分析

**说明**: 监控当前进程+所有子进程（例如外部可执行程序如COLMAP、GLOMAP）

**使用示例**:
```cpp
PROFILER_START_WITH_SUBPROCESS("glomap_pipeline", true);  // 包含colmap+glomap资源使用
```

#### `PROFILER_END()`
(profiler-end)=

结束性能分析会话（无锁）

```cpp
#define PROFILER_END()
```

**说明**:
- 自动调用当_profiler_session_离开作用域时
- 保存会话数据到_current_profiler_info

#### `PROFILER_STAGE()`
(profiler-stage)=

智能阶段检查点（无锁）

```cpp
#define PROFILER_STAGE(stage_name)
```

**参数**:
- `stage_name` `const char*` / `std::string`：用于自动区间分析的阶段名称

**说明**:
- 自动测量从上一个检查点到当前阶段的性能指标（无锁操作）
- 计算区间：[START, STAGE1], [STAGE1, STAGE2], ..., [STAGEn, END]

**使用示例**:
```cpp
PROFILER_STAGE("initialization");    // 测量[START, initialization]
PROFILER_STAGE("computation");       // 测量[initialization, computation]
PROFILER_STAGE("finalization");      // 测量[computation, finalization]
PROFILER_END();                      // 测量[finalization, END]
```

#### `PROFILER_SCOPE()`
(profiler-scope)=

整个方法作用域的自动性能分析器（无锁）

```cpp
#define PROFILER_SCOPE(enable_profiling)
```

**参数**:
- `enable_profiling` `bool`：是否启用性能分析

**说明**:
- 创建RAII性能分析会话，作用域退出时自动结束
- 在Run()方法开始处使用此宏进行完整的方法性能分析
- 自动使用GetProfilerLabel()/GetProfilerLabels()中的标签

**使用示例**:
```cpp
PROFILER_SCOPE(true);  // 使用方法配置的标签
```

#### `POSDK_SYSTEM()`
(posdk-system)=

执行带可选子进程监控的系统命令

```cpp
// 一个参数版本（默认不启用子进程监控）
#define POSDK_SYSTEM(command)

// 两个参数版本
#define POSDK_SYSTEM(command, enable_subprocess_monitoring)
```

**参数**:
- `command` `const char*` / `std::string`：要执行的命令字符串
- `enable_subprocess_monitoring` `bool`：是否启用子进程监控（可选，默认false）

**返回值**: `int` - 命令退出码

**说明**:
- 当存在活跃ProfilerSession时，使用会话的指标配置
- 如果enable_subprocess_monitoring=true，临时启用子进程监控

**使用示例**:
```cpp
int result = POSDK_SYSTEM("./command");              // 使用会话指标，无子进程监控
int result = POSDK_SYSTEM("./command", true);        // 使用会话指标+子进程监控
```

#### `POSDK_SYSTEM_AUTO()`
(posdk-system-auto)=

执行带自动性能分析会话管理的系统命令

```cpp
#define POSDK_SYSTEM_AUTO(command, enable_profiling)
```

**参数**:
- `command` `const char*` / `std::string`：要执行的命令字符串
- `enable_profiling` `bool`：是否启用性能分析

**返回值**: `int` - 命令退出码

**说明**:
- 自动创建只统计时间的ProfilerSession，执行命令，并结束会话
- 等价于：PROFILER_START_AUTO + POSDK_SYSTEM + PROFILER_END
- 使用会话的指标配置，默认不启用子进程监控

**使用示例**:
```cpp
int result = POSDK_SYSTEM_AUTO("./intensive_workload 3 150 2 4 2", true);  // 完整性能分析会话
```

#### `PROFILER_PRINT_STATS()`
(profiler-print-stats)=

打印当前会话的性能分析统计

```cpp
#define PROFILER_PRINT_STATS(enable_profiling)
```

**参数**:
- `enable_profiling` `bool`：是否启用了性能分析

**说明**:
- 打印_current_profiler_info中的当前会话数据
- 必须在PROFILER_END()之后调用

**使用示例**:
```cpp
PROFILER_PRINT_STATS(true);
```

#### `PM_PRINT_STATS()`
(pm-print-stats)=

打印ProfilerManager中的累积性能统计

```cpp
#define PM_PRINT_STATS(label_str, enable_profiling)
```

**参数**:
- `label_str` `const char*` / `std::string`：要查询的性能分析标签
- `enable_profiling` `bool`：是否启用了性能分析

**说明**: 打印ProfilerManager中指定标签的累积性能分析数据

**使用示例**:
```cpp
PM_PRINT_STATS("core_build_time", true);
```

#### `PROFILER_GET_CURRENT_TIME()`
(profiler-get-current-time)=

获取当前会话持续时间

```cpp
#define PROFILER_GET_CURRENT_TIME(enable_profiling)
```

**参数**:
- `enable_profiling` `bool`：是否启用了性能分析

**返回值**: `double` - 当前会话持续时间（毫秒），不可用时返回0.0

**说明**:
- 返回_current_profiler_info中的持续时间
- 必须在PROFILER_END()之后调用

#### `PM_GET_AVG_TIME()`
(pm-get-avg-time)=

从ProfilerManager获取累积平均持续时间

```cpp
#define PM_GET_AVG_TIME(label_str, enable_profiling)
```

**参数**:
- `label_str` `const char*` / `std::string`：要查询的性能分析标签
- `enable_profiling` `bool`：是否启用了性能分析

**返回值**: `double` - 平均持续时间（毫秒），无数据时返回0.0

**说明**: 返回指定标签所有会话的累积平均持续时间

---

## 性能指标说明

- **TIME**：会话起止之间的经过时间；阶段区间按检查点顺序累计。
- **MEMORY**：采样到的绝对峰值进程内存。启用子进程监控时包含所监控的后代进程；很短的峰值可能落在采样间隔之间。
- **CPU**：进程 CPU 时间增量与会话经过时间的比值；多个有效会话按 `CPU Sample Count` 汇总，不用逻辑运行次数作分母。
- **THREADS**：进程线程数及采样到的峰值。
- **SUBPROCESS**：外部程序的进程树资源监控。调用外部程序时，使用 `POSDK_SYSTEM(command, true)` 开启。

---

## 典型使用示例

### 基础性能分析使用
```cpp
// 在MethodPresetProfiler派生类中使用
class MyAlgorithm : public MethodPresetProfiler {
public:
    DataPtr Run() override {
        // 自动开始性能分析（使用预设标签，默认只统计时间）
        PROFILER_START_AUTO(true);

        // 添加阶段检查点
        InitializeData();
        PROFILER_STAGE("initialization");

        ProcessData();
        PROFILER_STAGE("processing");

        // 结束分析并打印统计
        PROFILER_END();
        PROFILER_PRINT_STATS(true);

        return result;
    }
};
```
详见: [`PROFILER_START_AUTO()`](#profiler-start-auto) | [`PROFILER_STAGE()`](#profiler-stage) | [`PROFILER_END()`](#profiler-end) | [`PROFILER_PRINT_STATS()`](#profiler-print-stats)

### 高级性能分析配置
```cpp
// 使用可配置指标的性能分析
POSDK_START(true, "time|memory|cpu");  // 统计时间、内存、CPU

// 处理数据...
ComplexComputation();

// 子进程监控（调用外部工具时）
int result = POSDK_SYSTEM("./external_tool", true);  // 启用子进程监控

PROFILER_END();
PROFILER_PRINT_STATS(true);
```
详见: [`POSDK_START()`](#posdk-start) | [`POSDK_SYSTEM()`](#posdk-system)

### 结构化标签与阶段分析
```cpp
// 使用结构化标签进行实验对比
std::unordered_map<std::string, std::string> labels = {
    {"algorithm", "advanced_sfm"},
    {"dataset", "dataset_A"},
    {"experiment", "comparison_2025"}
};
PROFILER_START_STRUCTURED(labels, true);

// 多阶段性能分析
PROFILER_STAGE("feature_extraction");   // [START, feature_extraction]
PROFILER_STAGE("feature_matching");     // [feature_extraction, feature_matching]
PROFILER_STAGE("reconstruction");       // [feature_matching, reconstruction]

PROFILER_END();  // [reconstruction, END]
```
详见: [`PROFILER_START_STRUCTURED()`](#profiler-start-structured)

### 系统命令性能监控
```cpp
// 自动性能分析的系统命令
int result = POSDK_SYSTEM_AUTO("./intensive_workload", true);

// 或手动管理会话
PROFILER_START_AUTO(true);
int result1 = POSDK_SYSTEM("./command1");              // 不启用子进程监控
int result2 = POSDK_SYSTEM("./command2", true);        // 启用子进程监控
PROFILER_END();
```
详见: [`POSDK_SYSTEM_AUTO()`](#posdk-system-auto)

### 累积统计与数据导出
```cpp
// 查看累积性能统计
PM_PRINT_STATS("my_algorithm", true);

// 获取平均执行时间
double avg_time = PM_GET_AVG_TIME("my_algorithm", true);

// 导出性能数据到CSV
ProfilerManager::GetInstance().ExportToCSV("performance_results.csv");

// 清空统计数据
ProfilerManager::GetInstance().Clear();
```
详见: [`PM_PRINT_STATS()`](#pm-print-stats) | [`PM_GET_AVG_TIME()`](#pm-get-avg-time) | [`ExportToCSV()`](#exporttocsv) | [`Clear()`](#clear)

### 作用域自动管理
```cpp
class MyMethod : public MethodPresetProfiler {
public:
    DataPtr Run() override {
        // 整个方法的自动性能分析（RAII风格）
        PROFILER_SCOPE(true);

        // 方法结束时自动统计并提交
        return ProcessData();
    }
};
```
详见: [`PROFILER_SCOPE()`](#profiler-scope)

---

## 性能分析宏使用指南

### 基础使用宏

#### 1. `PROFILER_START_AUTO(enable_profiling)`
**功能**: 自动使用方法预设的标签开始性能分析
**使用场景**: 继承自 `MethodPresetProfiler` 的方法中
**特点**: 自动选择字符串或结构化标签，**优化后默认不启用子进程监控**（节省开销）

```cpp
// 在 MethodPresetProfiler 派生类中使用
class MyAlgorithm : public MethodPresetProfiler {
public:
    DataPtr Run() override {
        PROFILER_START_AUTO(true);  // 自动使用设置的标签，默认不启用子进程监控
        
        // 算法实现...
        auto result = ProcessData();
        
        PROFILER_END();
        PROFILER_PRINT_STATS(true);
        return result;
    }
};
```

**性能优化说明**: 
- **默认行为**: 不启用子进程监控，避免不必要的系统调用开销
- **动态启用**: 只有在调用 `POSDK_SYSTEM` 时才会自动启用子进程监控
- **向后兼容**: 现有代码无需修改，自动获得性能提升

#### 2. `PROFILER_START(label_str, enable_profiling)`
**功能**: 使用指定字符串标签开始性能分析
**参数**: 
- `label_str`: 性能分析标签字符串
- `enable_profiling`: 是否启用分析(bool)

```cpp
void MyFunction() {
    PROFILER_START("my_custom_algorithm", true);
    
    // 执行耗时操作
    ComplexComputation();
    
    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

#### 3. `PROFILER_START_STRUCTURED(structured_labels, enable_profiling)`
**功能**: 使用结构化标签开始性能分析，支持CSV导出
**用途**: 生成带自定义列头的CSV统计表格

```cpp
void ExperimentFunction() {
    std::unordered_map<std::string, std::string> labels = {
        {"pipeline", "PoSDK"},
        {"dataset", "synthetic_data"},
        {"algorithm", "advanced_sfm"},
        {"version", "v2.1"}
    };
    
    PROFILER_START_STRUCTURED(labels, true);
    
    // 实验代码
    RunExperiment();
    
    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

### 高级功能宏

#### 4. `PROFILER_START_WITH_METRICS(label_str, enable_profiling, metrics_config)`
**功能**: 选择性启用特定性能指标
**参数**:
- `metrics_config`: 指标配置字符串，用 `|` 分隔
  - 可选值: `"time"`, `"memory"`, `"cpu"`, `"threads"`, `"subprocess"`, `"ALL"`

```cpp
void LightweightProfiling() {
    // 只统计时间和内存，跳过CPU和线程统计
    PROFILER_START_WITH_METRICS("lightweight_test", true, "time|memory");
    
    FastOperation();
    
    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

#### 5. `PROFILER_START_WITH_SUBPROCESS(label_str, enable_profiling)`
**功能**: 包含子进程的完整资源监控
**应用场景**: 调用外部程序时的完整性能统计

```cpp
void CallExternalTool() {
    PROFILER_START_WITH_SUBPROCESS("colmap_pipeline", true);
    
    // 调用外部工具，会统计子进程资源
    POSDK_SYSTEM("COLMAP feature_extractor --database_path database.db");
    POSDK_SYSTEM("COLMAP exhaustive_matcher --database_path database.db");
    
    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

### 阶段分析宏

#### 6. `PROFILER_STAGE(stage_name)`
**功能**: 添加阶段检查点，自动计算区间性能
**特点**: 无锁操作，自动生成区间统计

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

### 作用域分析宏

#### 7. `PROFILER_SCOPE(enable_profiling)`
**功能**: RAII风格的自动作用域性能分析
**特点**: 方法结束时自动提交统计数据

```cpp
class MyAlgorithm : public MethodPresetProfiler {
public:
    DataPtr Run() override {
        PROFILER_SCOPE(true);  // 整个方法的自动分析
        
        // 方法实现...
        auto result = ProcessComplexData();
        
        // 方法结束时自动统计并提交
        return result;
    }
};
```

### 系统命令宏

#### 8. `POSDK_SYSTEM(command [, enable_subprocess_monitoring])`
**功能**: 带性能分析感知的系统命令执行
**参数**:
- `command`: 要执行的命令字符串
- `enable_subprocess_monitoring`: 可选，是否启用子进程监控（默认为false）
**特点**: **自动动态启用子进程监控**，进行执行前后采样

```cpp
void RunExternalCommands() {
    PROFILER_START_AUTO(true);  // 默认不启用子进程监控，节省开销
    
    // POSDK_SYSTEM会自动启用子进程监控，进行执行前后采样
    int result1 = POSDK_SYSTEM("./heavy_computation --input data.txt");
    int result2 = POSDK_SYSTEM("python process_results.py", true);
    int result3 = POSDK_SYSTEM("echo 'simple command'", false);  // 不启用监控
    
    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

**智能优化**: 
- **按需启用**: 只有在调用 `POSDK_SYSTEM` 时才启用子进程监控
- **自动切换**: 从当前进程监控自动切换到进程树监控
- **性能提升**: 避免在不需要时进行昂贵的子进程系统调用

#### 9. `POSDK_SYSTEM_AUTO(command, enable_profiling)`
**功能**: 自动创建性能分析会话并执行命令
**特点**: 等价于 `PROFILER_START_AUTO` + `POSDK_SYSTEM` + `PROFILER_END`

```cpp
void QuickSystemCommand() {
    // 自动创建会话、执行命令、打印统计
    int result = POSDK_SYSTEM_AUTO("./intensive_workload 3 150 2 4 2", true);
}
```

### 统计显示宏

#### 10. `PROFILER_PRINT_STATS(enable_profiling)`
**功能**: 打印当前会话的性能统计
**使用**: 必须在 `PROFILER_END()` 之后调用

#### 11. `PM_PRINT_STATS(label_str, enable_profiling)`
**功能**: 打印指定标签的累积性能统计
**特点**: 显示多次运行的聚合数据

```cpp
void ShowAccumulatedStats() {
    // 显示 "my_algorithm" 标签的累积统计
    PM_PRINT_STATS("my_algorithm", true);
}
```

## MethodPresetProfiler 集成示例

### 基础集成
```cpp
class MySfMAlgorithm : public MethodPresetProfiler 
{
public:
    MySfMAlgorithm() {
        // 设置性能分析标签
        SetProfilerLabel("advanced_sfm_v2.1");
        
        // 或使用结构化标签（推荐用于实验对比）
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
        // 方法1：自动标签 + 完整监控
        PROFILER_START_AUTO(true);
        
        // 初始化
        auto cameras = InitializeCameras();
        PROFILER_STAGE("camera_initialization");
        
        // 特征提取
        auto features = ExtractFeatures();
        PROFILER_STAGE("feature_extraction");
        
        // 特征匹配
        auto matches = MatchFeatures(features);
        PROFILER_STAGE("feature_matching");
        
        // 调用外部工具进行重建
        int result = POSDK_SYSTEM("COLMAP mapper --database_path temp.db");
        PROFILER_STAGE("external_reconstruction");
        
        // 后处理
        auto final_result = PostProcessing();
        PROFILER_STAGE("post_processing");
        
        PROFILER_END();
        PROFILER_PRINT_STATS(true);
        
        return final_result;
    }
    
private:
    DataPtr InitializeCameras() { /* 实现... */ }
    DataPtr ExtractFeatures() { /* 实现... */ }
    DataPtr MatchFeatures(const DataPtr& features) { /* 实现... */ }
    DataPtr PostProcessing() { /* 实现... */ }
};
```

### 实验对比示例
```cpp
class ExperimentRunner {
public:
    void RunComparison() {
        // 实验1：基础算法
        {
            MySfMAlgorithm algo1;
            algo1.SetProfilerLabels({
                {"algorithm", "basic_sfm"},
                {"dataset", "dataset_A"},
                {"experiment", "comparison_2025"}
            });
            auto result1 = algo1.Build(input_data);
        }
        
        // 实验2：改进算法
        {
            MySfMAlgorithm algo2;
            algo2.SetProfilerLabels({
                {"algorithm", "advanced_sfm"},
                {"dataset", "dataset_A"}, 
                {"experiment", "comparison_2025"}
            });
            auto result2 = algo2.Build(input_data);
        }
        
        // 导出对比数据
        ProfilerManager::GetInstance().ExportToCSV("experiment_results.csv");
    }
};
```

## 性能统计输出说明

### 主表

主表包含动态标签列、`Label`、`Profile Scope`、`Average Time Count`、`Total Time(ms)`、`Average Time(ms)`、`Session Count` 和 `Logical Run Count`。按采集配置增加内存、CPU、线程和子进程列。CPU 列同时提供 `CPU Sample Count`。

`Profile Scope` 保存 `method` 或 `core` 分类；GUI 批量导出还保存 `Experiment Dimensions`。平均耗时使用导出记录的分母，CPU 使用有效采样会话数，峰值指标取最大值。

### 阶段表

阶段表包含标签、分类、`Interval Name`、`Total Duration(ms)`、`Average Duration(ms)` 和 `Count`，批量运行保留实验维度。

同一 `A_to_B` 区间出现多次时，累计每次耗时并增加 `Count`；平均值为累计耗时除以实际出现次数。

### 选择采集范围

只比较运行时间时，使用默认时间指标。需要资源统计时，再启用内存、CPU、线程或子进程采样。比较两套算法时保持采集配置一致，并用 GUI 单独记录的墙钟时间比较端到端耗时。
