# 性能分析器 (Profiler)

PoSDK内置了高性能的代码执行监控工具，帮助开发者找到程序运行中的性能瓶颈。

## 什么是性能标签？

**标签（Label）** 是程序段的属性标签，用于统计不同函数或代码段的性能表现。就像给代码块贴上名字标签一样，方便后续查看哪个部分运行得快或慢。

例如：
- `"特征提取"` - 标记特征检测代码段
- `"图像匹配"` - 标记图像匹配算法代码段
- `"三角化重建"` - 标记3D点重建代码段

## 基本使用方法

### 1. 自动测量函数性能

```cpp
void 特征提取函数()
{
    PROFILER_START_AUTO(true);  // 自动使用函数名作为标签进行性能测量

    // 你的特征提取代码...

    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

### 2. 测量特定代码段性能

```cpp
void 图像处理函数()
{
    PROFILER_START_AUTO(true);

    // 测量图像预处理部分
    InitializeData();
    PROFILER_STAGE("图像预处理");

    // 测量特征检测部分
    ExtractFeatures();
    PROFILER_STAGE("SIFT特征检测");

    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

### 3. 启用分析器并查看结果

```cpp
int main()
{
    // 启用性能分析器（对特定方法）
    PM_SET_PROFILING_ENABLED("my_algorithm", true);

    // 运行你的程序
    特征提取函数();
    图像处理函数();

    // 生成性能报告文件
    PM_EXPORT_TO_CSV("性能报告.csv");

    // 或者直接打印到控制台查看
    PM_DISPLAY_ALL_PROFILING_DATA();

    return 0;
}
```

## 性能报告输出示例

运行上述代码后，你会看到类似这样的性能统计表：

### 控制台输出
```
=== 特征提取函数 当前会话性能统计 ===
持续时间: 125.43 ms
内存增量: 34.2 MB (相对开始时)
CPU使用率: 187.4%
线程数 (开始/结束): 1/4

=== 阶段区间统计 ===
  START_to_图像预处理: 34.21 ms
  图像预处理_to_SIFT特征检测: 55.46 ms
  SIFT特征检测_to_END: 35.76 ms
```

### CSV文件输出 (性能报告.csv)

| 标签名称 (label_str) | 会话次数 | 总执行时间 (ms) | 平均时间 (ms) | 最短时间 (ms) | 最长时间 (ms) | 峰值内存 (MB) | 平均CPU (%) |
| -------------------- | -------- | --------------- | ------------- | ------------- | ------------- | ------------- | ----------- |
| 特征提取函数         | 1        | 125.43          | 125.43        | 125.43        | 125.43        | 34.2          | 187.4       |
| 图像处理函数         | 1        | 89.67           | 89.67         | 89.67         | 89.67         | 28.1          | 156.8       |

**表格说明：**
- **标签名称**: 就是你在代码中设置的标签字符串（label_str）
- **会话次数**: 该代码段被执行了多少次
- **总执行时间**: 该代码段累计运行了多长时间
- **平均时间**: 平均每次执行的时间
- **峰值内存**: 该代码段运行期间相对开始时的最大内存增量
- **平均CPU**: 该代码段的平均CPU使用率

## 进阶使用技巧

### 使用不同性能指标

```cpp
void 数据处理函数()
{
    // 只统计时间（最快，适合频繁调用的函数）
    POSDK_START(true, "time");

    // 时间+内存统计（适合内存敏感的算法）
    POSDK_START(true, "time|memory");

    // 完整统计（时间+内存+CPU+线程数）
    POSDK_START(true, "time|memory|cpu");

    // 你的算法代码...

    PROFILER_END();
    PROFILER_PRINT_STATS(true);
}
```

### 监控外部程序性能

```cpp
void 调用外部工具()
{
    PROFILER_START_AUTO(true);

    // 执行外部程序时自动监控子进程资源使用
    int result = POSDK_SYSTEM("python train_model.py --epochs 100");

    PROFILER_END();
    PROFILER_PRINT_STATS(true);  // 会显示包含子进程的总资源消耗
}
```

### 实验对比分析

```cpp
void 算法对比实验()
{
    // 实验1：基础算法
    std::unordered_map<std::string, std::string> 标签1 = {
        {"算法类型", "基础SfM"},
        {"数据集", "室内场景"},
        {"实验批次", "2025年1月"}
    };
    PROFILER_START_STRUCTURED(标签1, true);
    运行基础算法();
    PROFILER_END();

    // 实验2：改进算法
    std::unordered_map<std::string, std::string> 标签2 = {
        {"算法类型", "改进SfM"},
        {"数据集", "室内场景"},
        {"实验批次", "2025年1月"}
    };
    PROFILER_START_STRUCTURED(标签2, true);
    运行改进算法();
    PROFILER_END();

    // 导出对比结果到CSV，方便用Excel分析
    PM_EXPORT_TO_CSV("算法对比结果.csv");
}
```

## 实用建议

1. **给标签起有意义的名字**: 使用描述性的中文名称，如"特征匹配"、"位姿估计"等
2. **测量关键代码段**: 重点关注可能耗时的算法和循环
3. **定期查看报告**: 通过性能报告找出程序瓶颈，优化最耗时的部分
4. **发布时可关闭**: 在最终发布版本中可以通过编译选项关闭性能分析，避免影响运行速度
5. **选择合适的指标**: 频繁调用的函数只统计时间，内存敏感的算法增加内存监控

## 系统配置

性能分析器默认是禁用状态，需要手动启用：

```cpp
// 启用特定方法的性能分析器
PM_SET_PROFILING_ENABLED("my_algorithm", true);

// 检查是否启用
bool 分析器已启用 = PM_IS_PROFILING_ENABLED("my_algorithm");

// 清空历史统计数据
PM_CLEAR();
```

这样，开发者就可以轻松监控代码性能，找出程序运行的瓶颈所在，进而优化代码提升整体性能。

---

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

### 时间指标 (TIME)
- **统计内容**: 从 `PROFILER_START` 到 `PROFILER_END` 的墙钟时间
- **精度**: 毫秒级，使用 `std::chrono::high_resolution_clock`
- **应用场景**: 算法执行耗时、函数调用时长分析

### 内存指标 (MEMORY) - **重要：现为增量统计**
- **统计内容**: **相对于开始时的峰值内存增量**（不是绝对内存值）
- **计算方式**: 
  ```
  峰值增量 = max(监控期间观察到的最大内存) - 开始时内存
  ```
- **统计范围**: 
  - 默认：仅当前进程的RSS内存
  - 启用子进程监控：当前进程 + 所有子进程的内存总和
- **平台实现**:
  - **Windows**: `WorkingSetSize` (物理内存)
  - **macOS**: `resident_size` (RSS内存)
  - **Linux**: `/proc/self/status` 中的 `VmRSS` (RSS内存)

### CPU指标 (CPU)
- **统计内容**: 进程CPU时间占用率（用户态+内核态）
- **计算方式**: 
  ```
  CPU使用率 = (结束CPU时间 - 开始CPU时间) / 墙钟时间 × 100%
  ```
- **注意**: 多线程程序可能超过100%
- **平台实现**:
  - **Windows**: 基于进程句柄的CPU时间（暂时返回0）
  - **macOS**: `getrusage(RUSAGE_SELF)` 获取用户态+系统态时间
  - **Linux**: `/proc/self/stat` 中的utime+stime字段

### 线程数指标 (THREADS)
- **统计内容**: 进程的线程数量（开始/结束/峰值）
- **平台实现**:
  - **Windows**: `CreateToolhelp32Snapshot` + `THREADENTRY32`
  - **macOS**: `task_threads()` 获取准确线程数
  - **Linux**: `/proc/self/status` 中的 `Threads` 字段

### 子进程监控 (SUBPROCESS)
- **统计内容**: 当前进程树的所有后代进程的资源使用
- **应用场景**: 调用外部工具（如COLMAP、GLOMAP）时的完整资源统计
- **实现方式**:
  - **Windows**: `CreateToolhelp32Snapshot` 枚举进程树
  - **macOS/Linux**: `ps` 命令 + `awk` 递归查找后代进程

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

#### 8. `POSDK_SYSTEM(command [, enable_profiling])`
**功能**: 带性能分析感知的系统命令执行
**参数**: 
- `command`: 要执行的命令字符串
- `enable_profiling`: 可选，默认为true
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

### 控制台输出示例
```
=== advanced_sfm_v2.1 当前会话性能统计 ===
持续时间: 2847.32 ms
内存增量: 156.8 MB (相对开始时)
CPU使用率: 287.4%
线程数 (开始/结束): 1/8
子进程数 (检测到/峰值): 3/5

=== 阶段区间统计 ===
  START_to_camera_initialization: 145.23 ms
  camera_initialization_to_feature_extraction: 856.45 ms
  feature_extraction_to_feature_matching: 1243.67 ms
  feature_matching_to_external_reconstruction: 445.89 ms
  external_reconstruction_to_post_processing: 156.08 ms
  post_processing_to_END: 45.67 ms
```

### CSV导出格式

#### 主文件 (experiment_results.csv)
| algorithm    | dataset   | experiment      | Label                                                               | Session Count | Profiling Threads | Total Time(ms) | Average Time(ms) | Min Time(ms) | Max Time(ms) | Peak Memory(MB) | Average CPU(%) | Process Peak Threads | Total Subprocesses | Peak Subprocesses |
| ------------ | --------- | --------------- | ------------------------------------------------------------------- | ------------- | ----------------- | -------------- | ---------------- | ------------ | ------------ | --------------- | -------------- | -------------------- | ------------------ | ----------------- |
| basic_sfm    | dataset_A | comparison_2025 | algorithm=basic_sfm_dataset=dataset_A_experiment=comparison_2025    | 5             | 2                 | 12847.32       | 2569.46          | 2234.12      | 2847.32      | 156.8           | 245.6          | 8                    | 15                 | 5                 |
| advanced_sfm | dataset_A | comparison_2025 | algorithm=advanced_sfm_dataset=dataset_A_experiment=comparison_2025 | 5             | 2                 | 11234.67       | 2246.93          | 2156.78      | 2334.12      | 142.3           | 267.8          | 12                   | 18                 | 6                 |

#### 阶段文件 (experiment_results_stages.csv) 
| Label                                                            | Interval Name                               | Total Duration(ms) | Average Duration(ms) | Count |
| ---------------------------------------------------------------- | ------------------------------------------- | ------------------ | -------------------- | ----- |
| algorithm=basic_sfm_dataset=dataset_A_experiment=comparison_2025 | START_to_camera_initialization              | 725.65             | 145.13               | 5     |
| algorithm=basic_sfm_dataset=dataset_A_experiment=comparison_2025 | camera_initialization_to_feature_extraction | 4234.56            | 846.91               | 5     |

### CSV表头说明

#### 主统计文件表头
- **动态结构化标签列** (如 algorithm, dataset, experiment): 用户自定义的结构化标签
- **Label**: 性能分析标签（结构化标签组合或用户设置的字符串）
- **Session Count**: 该标签的总会话数
- **Profiling Threads**: 参与性能分析的不同线程数
- **Total Time(ms)**: 所有会话的总耗时（毫秒）
- **Average Time(ms)**: 平均每会话耗时（毫秒）
- **Min Time(ms)**: 最短会话耗时（毫秒）
- **Max Time(ms)**: 最长会话耗时（毫秒）
- **Peak Memory(MB)**: 峰值内存增量（MB，相对开始时）
- **Average CPU(%)**: 平均CPU使用率（%，可超过100%）
- **Process Peak Threads**: 进程峰值线程数
- **Total Subprocesses**: 检测到的子进程总数
- **Peak Subprocesses**: 峰值子进程数

#### 阶段统计文件表头
- **Label**: 性能分析标签
- **Interval Name**: 区间名称（如 START_to_stage1, stage1_to_stage2）
- **Total Duration(ms)**: 该区间的总耗时（所有会话累计）
- **Average Duration(ms)**: 该区间的平均耗时
- **Count**: 该区间的统计次数

## 重要注意事项

### 内存统计说明
- **峰值内存现在是增量统计**，不是绝对内存值
- 计算方式：`峰值内存 = max(监控期间内存) - 开始时内存`
- 更适合分析算法实际内存消耗，排除程序基础内存影响

### 性能开销
- 启用全部指标时有轻微性能开销（通常<1-2%）
- 子进程监控会增加额外开销
- 建议在发布版本中禁用详细监控

### 平台兼容性
- 所有功能在 macOS、Linux 上完全支持
- Windows 平台的 CPU 监控功能有限
- 子进程监控在不同平台实现方式不同，但接口一致

### 使用建议
1. **开发阶段**: 使用 `PROFILER_START_AUTO` + 完整指标监控（已优化，默认不启用子进程监控）
2. **性能调优**: 使用 `PROFILER_STAGE` 进行细粒度分析
3. **实验对比**: 使用结构化标签 + CSV导出
4. **生产环境**: 选择性启用关键指标，避免全面监控
5. **外部工具调用**: 使用 `POSDK_SYSTEM` 自动启用子进程监控，获得完整资源统计

通过合理使用这些性能分析工具，可以有效识别性能瓶颈，优化算法实现，并进行科学的实验对比分析。