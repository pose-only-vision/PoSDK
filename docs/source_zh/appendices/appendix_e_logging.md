# 日志管理 (Logging)

PoSDK 提供高性能双语日志系统，支持中文和英文日志输出，具备线程安全、同步输出、文件输出等功能。

## 日志级别

日志系统定义了四个日志级别：

- `DEBUG`: 调试信息，可通过 `PO_DISABLE_DEBUG_LOG` 宏在编译时禁用
- `INFO`: 普通信息日志
- `WARNING`: 警告信息
- `ERROR`: 错误信息

日志级别通过 `SET_LOG_LEVEL(level)` 全局控制，低于设置级别的日志将被过滤。

## 双语日志宏

### 基础双语日志宏

支持明确指定语言，受语言环境过滤影响：

```cpp
BILINGUAL_LOG_DEBUG(lang)  // lang: ZH 或 EN
BILINGUAL_LOG_INFO(lang)
BILINGUAL_LOG_WARNING(lang)
BILINGUAL_LOG_ERROR(lang)
```

**使用示例**:
```cpp
BILINGUAL_LOG_INFO(ZH) << "处理了 " << count << " 个特征点";
BILINGUAL_LOG_INFO(EN) << "Processed " << count << " feature points";
```

### 总是输出日志宏

强制输出，不受语言环境过滤影响，适用于系统状态、错误信息等关键日志：

```cpp
BILINGUAL_LOG_ALWAYS_DEBUG
BILINGUAL_LOG_ALWAYS_INFO
BILINGUAL_LOG_ALWAYS_WARNING
BILINGUAL_LOG_ALWAYS_ERROR
```

**使用示例**:
```cpp
BILINGUAL_LOG_ALWAYS_INFO << "系统初始化完成";
BILINGUAL_LOG_ALWAYS_ERROR << "Critical system error occurred";
```

### 快捷日志宏

为常用场景提供的简化宏：

**中文日志快捷宏**:
```cpp
LOG_DEBUG_ZH    // 等价于 BILINGUAL_LOG_DEBUG(ZH)
LOG_INFO_ZH     // 等价于 BILINGUAL_LOG_INFO(ZH)
LOG_WARNING_ZH   // 等价于 BILINGUAL_LOG_WARNING(ZH)
LOG_ERROR_ZH     // 等价于 BILINGUAL_LOG_ERROR(ZH)
```

**英文日志快捷宏**:
```cpp
LOG_DEBUG_EN    // 等价于 BILINGUAL_LOG_DEBUG(EN)
LOG_INFO_EN     // 等价于 BILINGUAL_LOG_INFO(EN)
LOG_WARNING_EN  // 等价于 BILINGUAL_LOG_WARNING(EN)
LOG_ERROR_EN    // 等价于 BILINGUAL_LOG_ERROR(EN)
```

**总是输出快捷宏**:
```cpp
LOG_DEBUG_ALL   // 等价于 BILINGUAL_LOG_ALWAYS_DEBUG
LOG_INFO_ALL    // 等价于 BILINGUAL_LOG_ALWAYS_INFO
LOG_WARNING_ALL // 等价于 BILINGUAL_LOG_ALWAYS_WARNING
LOG_ERROR_ALL   // 等价于 BILINGUAL_LOG_ALWAYS_ERROR
```

**使用示例**:
```cpp
LOG_INFO_ZH << "快速中文日志";
LOG_ERROR_EN << "Quick English error log";
LOG_INFO_ALL << "Always output log";
```

### 条件日志宏

仅在条件为真时输出日志：

```cpp
BILINGUAL_LOG_DEBUG_IF(lang, condition)
BILINGUAL_LOG_INFO_IF(lang, condition)
BILINGUAL_LOG_WARNING_IF(lang, condition)
BILINGUAL_LOG_ERROR_IF(lang, condition)
```

**使用示例**:
```cpp
BILINGUAL_LOG_DEBUG_IF(ZH, debug_mode) << "调试信息: " << debug_value;
```

## 日志级别控制

### 设置日志级别

```cpp
SET_LOG_LEVEL(level)  // level: DEBUG, INFO, WARNING, ERROR
```

**使用示例**:
```cpp
SET_LOG_LEVEL(WARNING);  // 只显示 WARNING 和 ERROR 级别的日志
```

### 获取当前日志级别

```cpp
LogLevel current_level = GET_LOG_LEVEL();
```

### 检查是否应输出指定级别

```cpp
bool should_output = SHOULD_LOG(INFO);
```

## 文件输出管理

### 启用日志文件输出

```cpp
ENABLE_LOG_FILE(file_path)  // 自动创建目录
```

**使用示例**:
```cpp
ENABLE_LOG_FILE("logs/app.log");
```

### 禁用日志文件输出

```cpp
DISABLE_LOG_FILE()
```

### 检查文件输出状态

```cpp
bool enabled = IS_LOG_FILE_ENABLED();
```

### 获取当前日志文件路径

```cpp
std::string path = GET_LOG_FILE_PATH();
```

## 源文件位置配置

默认情况下，日志不显示源文件位置。可通过以下宏启用或禁用：

### 启用源文件位置显示

```cpp
ENABLE_SOURCE_LOC()
```

启用后，日志输出格式为：`[PoSDK | INFO | file.cpp:42] 消息内容`

### 禁用源文件位置显示

```cpp
DISABLE_SOURCE_LOC()  // 默认禁用
```

### 检查源文件位置显示状态

```cpp
bool show_loc = IS_SOURCE_LOC_ENABLED();
```

**使用示例**:
```cpp
ENABLE_SOURCE_LOC();
LOG_INFO_ZH << "这条日志会显示文件位置";
// 输出: [PoSDK | INFO | test.cpp:42] 这条日志会显示文件位置

DISABLE_SOURCE_LOC();
LOG_INFO_ZH << "这条日志不显示文件位置";
// 输出: [PoSDK | INFO] 这条日志不显示文件位置
```

## 性能统计

### 获取已处理日志数量

```cpp
size_t count = GET_LOG_PROCESSED_COUNT();
```

### 获取丢弃日志数量

```cpp
size_t dropped = GET_LOG_DROPPED_COUNT();  // 同步模式下始终为0
```

### 强制刷新所有缓冲区

```cpp
FLUSH_ALL_LOGS()
```

## 语言环境设置

通过 `LanguageEnvironment` 类设置全局语言环境：

```cpp
PoSDK::Interface::LanguageEnvironment::SetChinese();   // 设置中文环境
PoSDK::Interface::LanguageEnvironment::SetEnglish();  // 设置英文环境
```

语言环境影响 `BILINGUAL_LOG_XXX(ZH/EN)` 宏的输出过滤，但不影响 `BILINGUAL_LOG_ALWAYS_XXX` 宏。

## DEBUG 日志编译时禁用

可通过定义 `PO_DISABLE_DEBUG_LOG` 宏在编译时完全禁用 DEBUG 级别日志：

```cpp
#define PO_DISABLE_DEBUG_LOG
```

禁用后，所有 DEBUG 相关宏在编译时变为空操作，不会产生任何运行时开销。

## 输出格式

### 基础格式

所有日志消息使用以下格式：

```
[PoSDK | LEVEL] 消息内容
```

### 启用源文件位置后的格式

```
[PoSDK | LEVEL | file.cpp:line] 消息内容
```

### 示例

```cpp
LOG_INFO_ZH << "处理完成";
// 输出: [PoSDK | INFO] 处理完成

ENABLE_SOURCE_LOC();
LOG_INFO_ZH << "处理完成";
// 输出: [PoSDK | INFO | myfile.cpp:123] 处理完成
```


## 完整示例

```cpp
#include <po_core/po_logger.hpp>

int main() {
    // 配置日志系统
    ENABLE_LOG_FILE("logs/app.log");
    ENABLE_SOURCE_LOC();
    SET_LOG_LEVEL(INFO);
    
    // 设置语言环境
    PoSDK::Interface::LanguageEnvironment::SetChinese();
    
    // 使用日志
    LOG_INFO_ZH << "应用程序启动";
    LOG_INFO_EN << "Application started";
    
    // 条件日志
    bool debug_mode = true;
    BILINGUAL_LOG_DEBUG_IF(ZH, debug_mode) << "调试模式已启用";
    
    // 总是输出的关键日志
    LOG_INFO_ALL << "系统状态正常";
    
    // 查看统计信息
    std::cout << "已处理日志: " << GET_LOG_PROCESSED_COUNT() << std::endl;
    
    return 0;
}
```
