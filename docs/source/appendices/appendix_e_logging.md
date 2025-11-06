# Logging Management

PoSDK provides a high-performance bilingual logging system supporting Chinese and English log output, with thread safety, synchronous output, file output, and other features.

## Log Levels

The logging system defines four log levels:

- `DEBUG`: Debug information, can be disabled at compile time via `PO_DISABLE_DEBUG_LOG` macro
- `INFO`: General information logs
- `WARNING`: Warning messages
- `ERROR`: Error messages

Log levels are controlled globally via `SET_LOG_LEVEL(level)`, logs below the set level will be filtered.

## Bilingual Log Macros

### Basic Bilingual Log Macros

Supports explicit language specification, affected by language environment filtering:

```cpp
BILINGUAL_LOG_DEBUG(lang)  // lang: ZH or EN
BILINGUAL_LOG_INFO(lang)
BILINGUAL_LOG_WARNING(lang)
BILINGUAL_LOG_ERROR(lang)
```

**Usage Example**:
```cpp
BILINGUAL_LOG_INFO(ZH) << "处理了 " << count << " 个特征点";
BILINGUAL_LOG_INFO(EN) << "Processed " << count << " feature points";
```

### Always Output Log Macros

Forced output, not affected by language environment filtering, suitable for system status, error messages, and other critical logs:

```cpp
BILINGUAL_LOG_ALWAYS_DEBUG
BILINGUAL_LOG_ALWAYS_INFO
BILINGUAL_LOG_ALWAYS_WARNING
BILINGUAL_LOG_ALWAYS_ERROR
```

**Usage Example**:
```cpp
BILINGUAL_LOG_ALWAYS_INFO << "系统初始化完成";
BILINGUAL_LOG_ALWAYS_ERROR << "Critical system error occurred";
```

### Shortcut Log Macros

Simplified macros for common scenarios:

**Chinese Log Shortcuts**:
```cpp
LOG_DEBUG_ZH    // Equivalent to BILINGUAL_LOG_DEBUG(ZH)
LOG_INFO_ZH     // Equivalent to BILINGUAL_LOG_INFO(ZH)
LOG_WARNING_ZH  // Equivalent to BILINGUAL_LOG_WARNING(ZH)
LOG_ERROR_ZH    // Equivalent to BILINGUAL_LOG_ERROR(ZH)
```

**English Log Shortcuts**:
```cpp
LOG_DEBUG_EN    // Equivalent to BILINGUAL_LOG_DEBUG(EN)
LOG_INFO_EN     // Equivalent to BILINGUAL_LOG_INFO(EN)
LOG_WARNING_EN  // Equivalent to BILINGUAL_LOG_WARNING(EN)
LOG_ERROR_EN    // Equivalent to BILINGUAL_LOG_ERROR(EN)
```

**Always Output Shortcuts**:
```cpp
LOG_DEBUG_ALL   // Equivalent to BILINGUAL_LOG_ALWAYS_DEBUG
LOG_INFO_ALL    // Equivalent to BILINGUAL_LOG_ALWAYS_INFO
LOG_WARNING_ALL // Equivalent to BILINGUAL_LOG_ALWAYS_WARNING
LOG_ERROR_ALL   // Equivalent to BILINGUAL_LOG_ALWAYS_ERROR
```

**Usage Example**:
```cpp
LOG_INFO_ZH << "快速中文日志";
LOG_ERROR_EN << "Quick English error log";
LOG_INFO_ALL << "Always output log";
```

### Conditional Log Macros

Output logs only when condition is true:

```cpp
BILINGUAL_LOG_DEBUG_IF(lang, condition)
BILINGUAL_LOG_INFO_IF(lang, condition)
BILINGUAL_LOG_WARNING_IF(lang, condition)
BILINGUAL_LOG_ERROR_IF(lang, condition)
```

**Usage Example**:
```cpp
BILINGUAL_LOG_DEBUG_IF(ZH, debug_mode) << "调试信息: " << debug_value;
```

## Log Level Control

### Set Log Level

```cpp
SET_LOG_LEVEL(level)  // level: DEBUG, INFO, WARNING, ERROR
```

**Usage Example**:
```cpp
SET_LOG_LEVEL(WARNING);  // Only display WARNING and ERROR level logs
```

### Get Current Log Level

```cpp
LogLevel current_level = GET_LOG_LEVEL();
```

### Check If Should Output Specified Level

```cpp
bool should_output = SHOULD_LOG(INFO);
```

## File Output Management

### Enable Log File Output

```cpp
ENABLE_LOG_FILE(file_path)  // Automatically create directory
```

**Usage Example**:
```cpp
ENABLE_LOG_FILE("logs/app.log");
```

### Disable Log File Output

```cpp
DISABLE_LOG_FILE()
```

### Check File Output Status

```cpp
bool enabled = IS_LOG_FILE_ENABLED();
```

### Get Current Log File Path

```cpp
std::string path = GET_LOG_FILE_PATH();
```

## Source File Location Configuration

By default, logs do not display source file location. Can be enabled or disabled via the following macros:

### Enable Source File Location Display

```cpp
ENABLE_SOURCE_LOC()
```

After enabling, log output format is: `[PoSDK | INFO | file.cpp:42] Message content`

### Disable Source File Location Display

```cpp
DISABLE_SOURCE_LOC()  // Disabled by default
```

### Check Source File Location Display Status

```cpp
bool show_loc = IS_SOURCE_LOC_ENABLED();
```

**Usage Example**:
```cpp
ENABLE_SOURCE_LOC();
LOG_INFO_ZH << "这条日志会显示文件位置";
// Output: [PoSDK | INFO | test.cpp:42] 这条日志会显示文件位置

DISABLE_SOURCE_LOC();
LOG_INFO_ZH << "这条日志不显示文件位置";
// Output: [PoSDK | INFO] 这条日志不显示文件位置
```

## Performance Statistics

### Get Processed Log Count

```cpp
size_t count = GET_LOG_PROCESSED_COUNT();
```

### Get Dropped Log Count

```cpp
size_t dropped = GET_LOG_DROPPED_COUNT();  // Always 0 in synchronous mode
```

### Force Flush All Buffers

```cpp
FLUSH_ALL_LOGS()
```

## Language Environment Settings

Set global language environment via `LanguageEnvironment` class:

```cpp
PoSDK::Interface::LanguageEnvironment::SetChinese();   // Set Chinese environment
PoSDK::Interface::LanguageEnvironment::SetEnglish();  // Set English environment
```

Language environment affects output filtering of `BILINGUAL_LOG_XXX(ZH/EN)` macros, but does not affect `BILINGUAL_LOG_ALWAYS_XXX` macros.

## DEBUG Log Compile-Time Disable

Can completely disable DEBUG level logs at compile time by defining `PO_DISABLE_DEBUG_LOG` macro:

```cpp
#define PO_DISABLE_DEBUG_LOG
```

After disabling, all DEBUG-related macros become no-ops at compile time, producing no runtime overhead.

## Output Format

### Basic Format

All log messages use the following format:

```
[PoSDK | LEVEL] Message content
```

### Format After Enabling Source File Location

```
[PoSDK | LEVEL | file.cpp:line] Message content
```

### Example

```cpp
LOG_INFO_ZH << "处理完成";
// Output: [PoSDK | INFO] 处理完成

ENABLE_SOURCE_LOC();
LOG_INFO_ZH << "处理完成";
// Output: [PoSDK | INFO | myfile.cpp:123] 处理完成
```


## Complete Example

```cpp
#include <po_core/po_logger.hpp>

int main() {
    // Configure logging system
    ENABLE_LOG_FILE("logs/app.log");
    ENABLE_SOURCE_LOC();
    SET_LOG_LEVEL(INFO);
    
    // Set language environment
    PoSDK::Interface::LanguageEnvironment::SetChinese();
    
    // Use logging
    LOG_INFO_ZH << "应用程序启动";
    LOG_INFO_EN << "Application started";
    
    // Conditional logging
    bool debug_mode = true;
    BILINGUAL_LOG_DEBUG_IF(ZH, debug_mode) << "调试模式已启用";
    
    // Always output critical logs
    LOG_INFO_ALL << "系统状态正常";
    
    // View statistics
    std::cout << "已处理日志: " << GET_LOG_PROCESSED_COUNT() << std::endl;
    
    return 0;
}
```
