# Anaconda环境变量屏蔽检查报告

## 概述

本文档检查了所有安装脚本是否在安装过程中屏蔽了Anaconda/Conda库对系统环境变量的影响。

## 检查标准

参考 `CMakeLists.txt` (32-117行) 的处理方式，脚本应该：
1. 从 `PATH` 中移除 Anaconda/Conda 路径
2. 从 `LD_LIBRARY_PATH` 中移除 Anaconda 路径
3. 从 `CMAKE_PREFIX_PATH` 中移除 Anaconda 路径
4. 处理 `PKG_CONFIG_PATH`（如果使用）
5. 取消设置 `CONDA_PREFIX` 环境变量
6. 保存原始环境变量（可选，用于恢复）

## 检查结果

| 脚本名称                    | 状态     | Anaconda清理位置       | 备注                                                       |
| --------------------------- | -------- | ---------------------- | ---------------------------------------------------------- |
| **install_absl.sh**         | ✅ 已处理 | 88-124行               | 处理PATH、LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、CONDA_PREFIX |
| **install_boost.sh**        | ✅ 已处理 | 297-333行              | 处理PATH、LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、CONDA_PREFIX |
| **install_ceres.sh**        | ✅ 已处理 | 222-258行              | 处理PATH、LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、CONDA_PREFIX |
| **install_cmake.sh**        | ✅ 已处理 | 218-254行              | 处理PATH、LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、CONDA_PREFIX |
| **install_colmap.sh**       | ✅ 已处理 | 348-380行<br>516-548行 | 两处都有处理（Ubuntu和Mac分支）                            |
| **install_dependencies.sh** | ❌ 不处理 | -                      | 主脚本，仅调用其他脚本，依赖子脚本处理                     |
| **install_glomap.sh**       | ✅ 已处理 | 595-660行<br>939-988行 | 两处都有处理（Ubuntu和Mac分支），还处理PKG_CONFIG_PATH     |
| **install_graphoptim.sh**   | ✅ 已处理 | 347-379行<br>571-600行 | 两处都有处理（内部COLMAP和GraphOptim）                     |
| **install_magsac.sh**       | ✅ 已处理 | 300-336行              | 处理PATH、LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、CONDA_PREFIX |
| **install_nlohmann.sh**     | ✅ 已处理 | 97-133行               | 处理PATH、LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、CONDA_PREFIX |
| **install_opencv.sh**       | ✅ 已处理 | 552-620行              | 完整处理，包括PKG_CONFIG_PATH和Python路径检查              |
| **install_opengv.sh**       | ✅ 已处理 | 116-152行              | 处理PATH、LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、CONDA_PREFIX |
| **install_openmvg.sh**      | ✅ 已处理 | 160-196行              | 处理PATH、LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、CONDA_PREFIX |
| **install_poselib.sh**      | ✅ 已处理 | 115-151行              | 处理PATH、LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、CONDA_PREFIX |
| **install_protobuf.sh**     | ✅ 已处理 | 106-142行              | 处理PATH、LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、CONDA_PREFIX |

## 详细分析

### ✅ 已正确处理的脚本

#### 1. install_absl.sh
```bash
# 300-336行
ORIGINAL_PATH="${PATH}"
ORIGINAL_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
ORIGINAL_CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}"

if [[ "${PATH}" == *"anaconda"* ]] || [[ "${PATH}" == *"conda"* ]]; then
    CLEAN_PATH=$(echo "${PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
    export PATH="${CLEAN_PATH}"
fi
# ... 类似处理LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、CONDA_PREFIX
```
**状态**: ✅ 完整处理

#### 2. install_boost.sh
**状态**: ✅ 完整处理（297-333行）

#### 3. install_ceres.sh
**状态**: ✅ 完整处理（222-258行）

#### 4. install_cmake.sh
**状态**: ✅ 完整处理（218-254行）

#### 5. install_colmap.sh
**状态**: ✅ 完整处理
- Ubuntu分支：348-380行
- Mac分支：516-548行

#### 6. install_glomap.sh
**状态**: ✅ 完整处理，**额外处理PKG_CONFIG_PATH**
- Ubuntu分支：595-660行
- Mac分支：939-988行
- 还检测Anaconda基础目录（CONDA_PREFIX、常见位置）

#### 7. install_graphoptim.sh
**状态**: ✅ 完整处理
- 内部COLMAP构建：347-379行
- GraphOptim构建：571-600行

#### 8. install_magsac.sh
**状态**: ✅ 完整处理（300-336行）

#### 9. install_nlohmann.sh
**状态**: ✅ 完整处理（97-133行）

#### 10. install_opencv.sh
**状态**: ✅ 完整处理，**最全面的处理**
- 552-620行：处理PATH、LD_LIBRARY_PATH、CMAKE_PREFIX_PATH、PKG_CONFIG_PATH、CONDA_PREFIX
- 额外检查Python可执行文件是否来自Anaconda
- 检查FFMPEG和HDF5是否来自Anaconda

#### 11. install_opengv.sh
**状态**: ✅ 完整处理（116-152行）

#### 12. install_openmvg.sh
**状态**: ✅ 完整处理（160-196行）

#### 13. install_poselib.sh
**状态**: ✅ 完整处理（115-151行）

#### 14. install_protobuf.sh
**状态**: ✅ 完整处理（106-142行）

### ❌ 不处理的脚本

#### install_dependencies.sh
**状态**: ❌ 不直接处理（但这是设计预期）
- 这是主脚本，只负责调用其他安装脚本
- 依赖各个子脚本自行处理Anaconda环境变量
- **建议**: 可以在主脚本开始时添加全局环境清理，作为额外保障

## 处理模式总结

所有已处理的脚本都遵循相同的模式：

```bash
# 1. 保存原始环境变量（可选）
ORIGINAL_PATH="${PATH}"
ORIGINAL_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
ORIGINAL_CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}"

# 2. 清理PATH
if [[ "${PATH}" == *"anaconda"* ]] || [[ "${PATH}" == *"conda"* ]]; then
    print_info "检测到Anaconda/Conda在PATH中，临时移除以避免污染"
    CLEAN_PATH=$(echo "${PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
    export PATH="${CLEAN_PATH}"
fi

# 3. 清理LD_LIBRARY_PATH
if [[ -n "${LD_LIBRARY_PATH}" ]] && [[ "${LD_LIBRARY_PATH}" == *"anaconda"* ]]; then
    print_info "检测到Anaconda在LD_LIBRARY_PATH中，临时移除"
    CLEAN_LD_LIBRARY_PATH=$(echo "${LD_LIBRARY_PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
    if [ -n "${CLEAN_LD_LIBRARY_PATH}" ]; then
        export LD_LIBRARY_PATH="${CLEAN_LD_LIBRARY_PATH}"
    else
        unset LD_LIBRARY_PATH
    fi
fi

# 4. 清理CMAKE_PREFIX_PATH
if [ -n "${CMAKE_PREFIX_PATH}" ]; then
    CLEAN_CMAKE_PREFIX_PATH=$(echo "${CMAKE_PREFIX_PATH}" | tr ':' '\n' | grep -v -E "(anaconda|conda)" | tr '\n' ':' | sed 's/:$//')
    if [ -n "${CLEAN_CMAKE_PREFIX_PATH}" ] && [ "${CLEAN_CMAKE_PREFIX_PATH}" != "${CMAKE_PREFIX_PATH}" ]; then
        print_info "从CMAKE_PREFIX_PATH中移除Anaconda路径"
        export CMAKE_PREFIX_PATH="${CLEAN_CMAKE_PREFIX_PATH}"
    fi
fi

# 5. 取消设置CONDA_PREFIX
if [ -n "${CONDA_PREFIX}" ]; then
    print_info "临时取消CONDA_PREFIX环境变量: ${CONDA_PREFIX}"
    unset CONDA_PREFIX
fi
```

## 额外处理

### install_opencv.sh
- ✅ 处理 `PKG_CONFIG_PATH`
- ✅ 检查Python可执行文件是否来自Anaconda
- ✅ 检查FFMPEG和HDF5是否来自Anaconda

### install_glomap.sh
- ✅ 处理 `PKG_CONFIG_PATH`
- ✅ 检测Anaconda基础目录（CONDA_PREFIX、常见位置）

## 建议

### 1. install_dependencies.sh 增强
可以在主脚本开始时添加全局环境清理：

```bash
# Clean environment globally before running installation scripts
# 在运行安装脚本之前全局清理环境
clean_anaconda_environment() {
    # ... 清理逻辑
}

# 在调用子脚本之前调用
clean_anaconda_environment
```

### 2. 统一清理函数
可以创建一个共享的清理函数，所有脚本都调用它：

```bash
# 在共同的脚本文件中定义
clean_anaconda_environment() {
    # ... 清理逻辑
}

# 各个脚本中调用
source "${SCRIPT_DIR}/common_environment.sh"
clean_anaconda_environment
```

### 3. 恢复机制（可选）
目前脚本在子shell中运行，退出后环境变量自动恢复。如果需要在脚本内部恢复，可以使用保存的原始值。

## 总结

✅ **14/15 脚本已正确处理Anaconda环境变量**
- 所有直接编译和安装的脚本都正确处理了Anaconda环境变量
- `install_dependencies.sh`作为主脚本不直接处理，但依赖的子脚本都已处理

✅ **处理方式统一**
- 所有脚本遵循相同的清理模式
- 与 `CMakeLists.txt` 的处理方式一致

✅ **部分脚本有额外处理**
- `install_opencv.sh` 和 `install_glomap.sh` 还处理了 `PKG_CONFIG_PATH`
- `install_opencv.sh` 额外检查Python、FFMPEG、HDF5是否来自Anaconda

## 结论

**所有安装脚本都已正确屏蔽Anaconda对系统环境变量的影响** ✅

建议：
1. 考虑在 `install_dependencies.sh` 中添加全局清理作为额外保障
2. 考虑提取公共清理函数到共享脚本中以提高可维护性

