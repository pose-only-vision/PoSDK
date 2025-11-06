# 安装指南

本文档说明 PoSDK 的安装要求和步骤。

## 系统要求

- **操作系统**:
  - 主要支持: macOS | Ubuntu 24.04 LTS（推荐）
  - 次要支持: 其他Linux发行版（Ubuntu 20.04+/CentOS 7）
  - 计划支持: Windows（未来版本）
- **编译器**: 支持 C++17 标准的编译器
  - GCC 11+ (Linux推荐)
  - Clang 13+ (macOS推荐)
- **构建工具**: CMake 3.28+（某些依赖库需要3.28+版本）

## 一键安装

PoSDK 提供了自动安装脚本，可以一键安装所有必需的系统依赖和插件依赖：

```bash
cd /path/to/PoSDK/src
chmod +x install.sh
./install.sh
```

安装脚本会自动完成以下步骤：
1. 更新系统包列表
2. 安装PoSDK框架开发依赖
3. 安装当前插件所需依赖

## PoSDK框架开发依赖

以下是PoSDK框架正常运行所需的系统依赖库：

| 依赖库              | 最低版本 | 用途                                    | 安装命令                                                                             |
| ------------------- | -------- | --------------------------------------- | ------------------------------------------------------------------------------------ |
| **build-essential** | -        | C++编译器和构建工具                     | `sudo apt install -y build-essential`                                                |
| **CMake**           | 3.28+    | 构建系统生成工具（某些库需要3.28+）     | 从源码构建（参见install_cmake.sh）或 `sudo apt install -y cmake`                     |
| **Eigen3**          | 3.3+     | 线性代数库，提供矩阵和向量运算          | `sudo apt install -y libeigen3-dev`                                                  |
| **SuiteSparse**     | 7.0+     | 稀疏矩阵计算库（包含CHOLMOD）           | `sudo apt install -y libsuitesparse-dev`                                             |
| **OpenBLAS**        | 0.3.20+  | 优化的BLAS库，提供高性能线性代数运算    | `sudo apt install -y libopenblas-dev`                                                |
| **Boost**           | 1.85.0   | C++扩展库（主要使用filesystem和system） | 从源码构建（参见install_boost.sh）                                                   |
| **Protobuf**        | 4.33.0   | 数据序列化库                            | 从源码构建（参见install_protobuf.sh）                                                |
| **nlohmann_json**   | 3.11.2   | C++ JSON解析库                          | 从源码构建（参见install_nlohmann.sh）                                                |
| **gflags**          | 2.2+     | 命令行参数解析库                        | `sudo apt install -y libgflags-dev`                                                  |
| **GTest**           | 1.10+    | 单元测试框架                            | `sudo apt install -y libgtest-dev`                                                   |
| **OpenCV**          | 4.x      | 计算机视觉库                            | 从源码构建（参见install_opencv.sh）                                                  |
| **glog**            | -        | Google日志库                            | `sudo apt install -y libgoogle-glog-dev`                                             |
| **Abseil**          | -        | Google基础库                            | 从源码构建（参见install_absl.sh）                                                    |
| **Ceres Solver**    | 2.2.0    | 非线性优化库                            | 从源码构建（参见install_ceres.sh）                                                   |
| **CURL**            | -        | HTTP/HTTPS请求库                        | `sudo apt install -y libcurl4-openssl-dev`                                           |
| **OpenGL**          | -        | 3D图形渲染库                            | `sudo apt install -y libgl1-mesa-dev mesa-common-dev libglu1-mesa-dev freeglut3-dev` |
| **GLFW3**           | -        | 窗口和输入管理库                        | `sudo apt install -y libglfw3-dev`                                                   |
| **Qt6**             | -        | GUI开发框架（可选）                     | `sudo apt install qt6-base-dev qt6-declarative-dev`                                  |




(installation-flow-explained)=
## 安装流程详解

本节详细说明 PoSDK 的多阶段安装流程，帮助您理解每个安装步骤的作用以及何时应该跳过重复安装。

### 安装流程总览

PoSDK 安装分为三个主要阶段：

```text
┌─────────────────────────────────────────────────────────────┐
│  Stage 1: 系统依赖安装 (install.sh)                          │
│  ├─ System packages (apt/brew)                              │
│  ├─ Build tools (cmake, make, gcc/clang)                   │
│  └─ System libraries (Eigen, Boost, OpenCV, etc.)          │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│  Stage 2: 依赖下载与源码安装 (install_dependencies.sh)        │
│  ├─ download_dependencies.sh → Download dependencies package│
│  ├─ download_po_core.sh → Download po_core binaries        │
│  └─ Individual library installations (see below)            │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│  Stage 3: PoSDK 主项目构建 (cmake + make)                    │
│  ├─ Configure with CMake                                    │
│  ├─ Build PoSDK core and plugins                           │
│  └─ Deploy output/ directory                               │
└─────────────────────────────────────────────────────────────┘
```

### 依赖库安装顺序与跳过策略

`install_dependencies.sh` 按照以下顺序安装 13 个依赖库，每个库都会检测现有安装：

| 步骤 | 依赖库               | 用途                                 | 安装脚本                | 跳过检测                                        | 何时应该跳过                 |
| ---- | -------------------- | ------------------------------------ | ----------------------- | ----------------------------------------------- | ---------------------------- |
| 0    | **CMake**            | 构建系统（可选，如果已满足版本要求） | `install_cmake.sh`      | CMake 版本检查（需要 3.28+ 用于某些库）         | 版本已满足要求（3.28+）      |
| 1    | **Boost**            | C++ 库（需要 1.85.0）                | `install_boost.sh`      | 安装目录存在且包含头文件和库文件                | 已安装且版本正确（1.85.0）   |
| 2    | **Abseil**           | Google基础库 (C++标准库扩展)         | `install_absl.sh`       | CMake配置文件 (`abslConfig.cmake`)              | 首次安装后，除非需要更新版本 |
| 3    | **nlohmann/json**    | JSON解析库（仅头文件）               | `install_nlohmann.sh`   | CMake配置文件 (`nlohmann_jsonConfig.cmake`)     | 已安装且功能正常             |
| 4    | **Protocol Buffers** | 数据序列化库                         | `install_protobuf.sh`   | CMake配置文件 (`protobuf-config.cmake`)         | 已安装且与 Abseil 版本兼容   |
| 5    | **OpenCV**           | 计算机视觉库                         | `install_opencv.sh`     | CMake配置文件 (`OpenCVConfig.cmake`) + 版本信息 | 已安装且版本满足要求         |
| 6    | **Ceres Solver**     | 非线性优化库                         | `install_ceres.sh`      | CMake配置文件 (`CeresConfig.cmake`) + 库文件    | 已安装且版本正确（2.2.0）    |
| 7    | **MAGSAC**           | 鲁棒估计算法库                       | `install_magsac.sh`     | 库文件 (`.so`/`.dylib`/`.a`)                    | 已安装且功能正常             |
| 8    | **OpenGV**           | 几何视觉库                           | `install_opengv.sh`     | 库文件 (`.so`/`.dylib`/`.a`)                    | 已安装且功能正常             |
| 9    | **PoseLib**          | 位姿估计库                           | `install_poselib.sh`    | 库文件 (`.so`/`.dylib`/`.a`)                    | 已安装且功能正常             |
| 10   | **OpenMVG**          | 多视图几何库                         | `install_openmvg.sh`    | 可执行文件 (`openMVG_main_*`)                   | 已安装且功能正常             |
| 11   | **COLMAP**           | SfM库                                | `install_colmap.sh`     | 可执行文件 (`bin/COLMAP`) + 版本信息            | 已安装且功能正常             |
| 12   | **GraphOptim**       | 图优化库 (依赖 COLMAP)               | `install_graphoptim.sh` | 可执行文件或库文件 (`rotation_estimator`)       | 已安装且 COLMAP 未更新       |
| 13   | **GLOMAP**           | 全局 SfM 库                          | `install_glomap.sh`     | 可执行文件 (`GLOMAP` 命令)                      | 已安装且功能正常             |

**注意**：
- 这些依赖库会自动从 `dependencies/` 目录下的源码编译安装
- 安装脚本会按照上述顺序依次安装，确保依赖关系正确
- 所有依赖库源码应预先放置在 `dependencies/` 对应的子目录中

### 详细的安装提示节点说明 | Detailed Installation Prompt Explanations

安装过程中您会遇到以下提示节点，每个节点的含义和建议如下：

(installation-prompt-table)=

| 阶段         | 脚本文件                   | 提示信息                                                              | 默认值  | 含义                                           | 建议操作                                   |
| ------------ | -------------------------- | --------------------------------------------------------------------- | ------- | ---------------------------------------------- | ------------------------------------------ |
| **系统依赖** | `install.sh`               | `Would you like to upgrade Qt now? 是否现在升级Qt?`                   | `[Y/n]` | Qt版本过旧时询问是否升级                       | 按回车（升级Qt避免冲突）                   |
| **系统依赖** | `install.sh`               | `Would you like to fix this automatically? 是否自动修复?`             | `[y/N]` | 检测到Qt版本冲突时询问是否自动修复             | 按回车（不自动修复，手动处理更安全）       |
| **系统依赖** | `install.sh`               | `Continue with Qt6 installation? 继续安装Qt6吗?`                      | `[y/N]` | 检测到Qt5时询问是否继续安装Qt6                 | 按回车（跳过Qt6安装，避免冲突）            |
| **下载阶段** | `download_dependencies.sh` | `Skip download and use local package?`                                | `[Y/n]` | 检测到本地依赖包时询问是否跳过下载并解压使用   | 按回车（使用本地包，解压所有内容覆盖现有） |
| **下载阶段** | `download_po_core.sh`      | `Skip reinstall and use existing po_core installation?`               | `[Y/n]` | 检测到po_core已安装时询问是否跳过重新安装      | 按回车（跳过安装，节省时间）               |
| **下载阶段** | `download_po_core.sh`      | `Skip download and use local package?`                                | `[Y/n]` | 检测到本地po_core压缩包时询问是否跳过下载      | 按回车（使用本地包，节省网络流量）         |
| **测试数据** | `download_Strecha.sh`      | `Do you want to re-download test data?`                               | `[y/N]` | 检测到测试数据已存在时询问是否重新下载         | 按回车（跳过重新下载）                     |
| **测试数据** | `download_Strecha.sh`      | `Skip download and use existing local package?`                       | `[Y/n]` | 检测到本地测试数据包时询问是否跳过下载         | 按回车（使用本地包）                       |
| **依赖构建** | `install_cmake.sh`         | `版本已满足要求，跳过安装？\| Version sufficient, skip installation?` | `[Y/n]` | CMake版本满足要求时询问是否跳过安装            | 按回车（跳过安装，节省时间）               |
| **依赖构建** | `install_boost.sh`         | `Skip rebuild and use existing Boost installation?`                   | `[Y/n]` | 检测到Boost已安装时询问是否跳过重建            | 按回车（跳过重建）                         |
| **依赖构建** | `install_absl.sh`          | `Do you want to skip this installation and use the existing build?`   | `[Y/n]` | 检测到Abseil已安装时询问是否跳过安装           | 按回车（跳过安装）                         |
| **依赖构建** | `install_nlohmann.sh`      | `Do you want to skip this installation and use the existing build?`   | `[Y/n]` | 检测到nlohmann/json已安装时询问是否跳过安装    | 按回车（跳过安装）                         |
| **依赖构建** | `install_protobuf.sh`      | `Do you want to skip this installation and use the existing build?`   | `[Y/n]` | 检测到Protocol Buffers已安装时询问是否跳过安装 | 按回车（跳过安装）                         |
| **依赖构建** | `install_opencv.sh`        | （无跳过提示，自动检测并使用现有安装）                                | -       | OpenCV安装脚本会自动检测并使用现有安装         | 无需操作（自动处理）                       |
| **依赖构建** | `install_ceres.sh`         | `Skip rebuild and use existing Ceres Solver installation?`            | `[Y/n]` | 检测到Ceres已安装时询问是否跳过重建            | 按回车（跳过重建，节省时间）               |
| **依赖构建** | `install_magsac.sh`        | `Skip rebuild and use existing MAGSAC installation?`                  | `[Y/n]` | 检测到MAGSAC已安装时询问是否跳过重建           | 按回车（跳过重建）                         |
| **依赖构建** | `install_opengv.sh`        | `Skip rebuild and use existing OpenGV installation?`                  | `[Y/n]` | 检测到OpenGV已安装时询问是否跳过重建           | 按回车（跳过重建）                         |
| **依赖构建** | `install_poselib.sh`       | `Skip rebuild and use existing PoseLib installation?`                 | `[Y/n]` | 检测到PoseLib已安装时询问是否跳过重建          | 按回车（跳过重建）                         |
| **依赖构建** | `install_openmvg.sh`       | `Skip rebuild and use existing OpenMVG installation?`                 | `[Y/n]` | 检测到OpenMVG已构建时询问是否跳过重建          | 按回车（跳过重建）                         |
| **依赖构建** | `install_colmap.sh`        | `Skip rebuild and use existing COLMAP installation?`                  | `[Y/n]` | 检测到COLMAP已安装时询问是否跳过重建           | 按回车（跳过重建）                         |
| **依赖构建** | `install_graphoptim.sh`    | `Skip rebuild and use existing GraphOptim build?`                     | `[Y/n]` | 检测到GraphOptim已构建时询问是否跳过重建       | 按回车（跳过重建）                         |
| **依赖构建** | `install_graphoptim.sh`    | `跳过构建内部 COLMAP 支持（仅构建核心功能）？`                        | `[Y/n]` | 询问是否跳过构建内部COLMAP支持                 | 按回车（跳过COLMAP支持，仅构建核心功能）   |
| **依赖构建** | `install_glomap.sh`        | `是否跳过此次安装并使用现有构建？[Y/n]`                               | `[Y/n]` | 检测到Glomap已构建时询问是否跳过安装           | 按回车（跳过安装）                         |
| **依赖构建** | `install_glomap.sh`        | `是否使用本地依赖构建? \| Use local dependencies for build?`          | `[Y/n]` | 询问是否使用本地依赖构建Glomap                 | 按回车（使用本地依赖）                     |
| **错误处理** | `install_dependencies.sh`  | `Continue with remaining installations?`                              | `[Y/n]` | 某个依赖安装失败时询问是否继续后续安装         | 按回车（继续安装其他依赖）                 |

```{important}
**默认值使用说明 | Default Values:**

对于初次安装和正常使用场景，所有提示可直接按回车键使用默认值。

For first-time installation and normal use cases, press Enter to use default values for all prompts.

默认值的设计原则：
- `[Y/n]` 默认跳过重复操作，节省时间和资源
- `[y/N]` 默认采取保守操作，避免意外覆盖或冲突

以下情况需要手动选择：
- **强制重建**：怀疑某个依赖安装有问题，输入 `n` 强制重新安装
- **版本更新**：需要更新到最新版本时，输入 `n` 重新下载构建
- **故障排除**：安装过程中出现错误，需要重新安装特定组件
```
  
```{tip}
**跳过检测说明**：
- **CMake配置文件**：检查标准CMake配置文件是否存在（如 `OpenCVConfig.cmake`），确保库可被CMake正确找到
- **库文件**：检查编译产物（`.so`、`.dylib`、`.a`）是否存在于安装目录
- **可执行文件**：检查程序二进制文件是否存在且可执行
- **版本信息**：部分库（OpenCV、COLMAP）会额外显示已安装版本号
```



## Quick Start

You can quickly launch the PoSDK Global SfM pipeline by following these steps:

### Prerequisites

- CMake 3.28+（某些依赖库需要3.28+版本）
- C++17 compatible compiler
- Git

### Step 1: Clone Repository and Install

```bash
# 1. Clone repository
git clone https://github.com/pose-only-vision/PoSDK.git
cd PoSDK

# 2. One-click installation (supports resume on timeout)
./install.sh
```

**Features:**
- **Resume Installation** - Re-run `./install.sh` if network timeouts occur
- **Automated Setup** - Downloads dependencies, builds PoSDK, and prepares test data
- **Skip Detection** - Automatically detects existing installations and prompts to skip (default: Y)

```{tip}
**默认值使用 | Default Values:**

对于正常安装：直接按回车键（默认值）即可完成安装，无需额外选择。

For normal installation: Press Enter (default values) to complete the installation.
```

```{note}
**Installation Prompts | 安装提示说明:**

安装过程中会出现多个 `[Y/n]` 或 `[y/N]` 提示，用于检测已有安装并询问是否跳过重复安装。
During installation, you'll see multiple `[Y/n]` or `[y/N]` prompts to detect existing installations and ask whether to skip redundant installations.

**提示格式说明 | Prompt Format:**
- `[Y/n]` - 默认为 **Y** (跳过)，直接按回车即可
- `[y/N]` - 默认为 **N** (继续)，直接按回车即可

**正常情况建议：一路按回车键使用默认值**
**Normal recommendation: Press Enter all the way to use default values**

For detailed explanation of each installation step, see [](#installation-flow-explained).
```

```{note}
**For Poor Network Conditions:**
If automatic download fails, manually download from [GitHub Releases](https://github.com/pose-only-vision/PoSDK/releases/tag/v1.0.0) and copy files directly (no extraction needed):
- `po_core` + `PoSDK dependencies` → `./dependencies/`
- `PoSDK_test_data` → `./tests/`
```

Then run `./install.sh` to complete setup.

### Step 2: Build PoSDK (if not done during installation)

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Build Output Directory Structure
(posdk-build-output-structure)=

After successful build, PoSDK generates an `output` directory in the build folder with the following structure:

```text
output/
├── bin/                          # Executable files directory
│   ├── PoSDK                    # PoSDK main program
│   └── globalsfm_pipeline_work/ # Working directory (generated at runtime)
├── lib/                          # Library files directory
│   ├── libposdk_agg.dylib/.so   # PoSDK aggregate library
│   └── cmake/                   # CMake configuration files
├── include/                      # Header files directory
│   ├── po_core/                 # po_core core headers
│   ├── pomvg/                   # PoSDK version info headers
│   ├── common/                  # PoSDK common headers
│   ├── plugins/                 # Plugin interface headers
│   ├── cereal/                  # Serialization library headers
│   └── lemon/                   # Graph optimization library headers
├── configs/                      # Configuration files directory
│   ├── methods/                 # Method plugin configurations
│   │   ├── globalsfm_pipeline.ini      # Global SfM pipeline config
│   │   ├── method_img2features.ini     # Feature extraction config
│   │   ├── method_img2matches.ini      # Feature matching config
│   │   ├── method_rotation_averaging.ini # Rotation averaging config
│   │   ├── TwoViewEstimator.ini        # Two-view estimator config
│   │   └── ...                         # Other method configs
│   └── data/                    # Data plugin configurations (optional)
├── plugins/                      # Plugin libraries directory
│   ├── methods/                 # Method plugins
│   │   ├── libglobalsfm_pipeline.dylib/.so       # Global SfM pipeline plugin
│   │   ├── libimg2features_pipeline.dylib/.so    # Feature extraction plugin
│   │   ├── libimg2matches_pipeline.dylib/.so     # Feature matching plugin
│   │   ├── libmethod_rotation_averaging.dylib/.so # Rotation averaging plugin
│   │   ├── libTwoViewEstimator.dylib/.so         # Two-view estimator plugin
│   │   └── ...                                   # Other method plugins
│   └── data/                    # Data plugins (optional)
├── python/                       # Python tools directory
│   ├── conda_env/               # Conda environment configuration
│   ├── colmap_pipeline/       # COLMAP preprocessing tools
│   ├── glomap_pipeline/       # GLOMAP preprocessing tools
│   ├── img2features_pipeline/   # Feature extraction pipeline tools
│   └── img2matches_pipeline/    # Feature matching pipeline tools
├── tests/                        # Test data directory (CMake auto-copied)
│   └── Strecha/                 # Strecha dataset
│       ├── castle-P19/          # Castle dataset (19 images)
│       ├── castle-P30/          # Castle dataset (30 images)
│       ├── entry-P10/           # Entry dataset (10 images)
│       ├── fountain-P11/        # Fountain dataset (11 images)
│       ├── Herz-Jesus-P8/       # Church dataset (8 images)
│       └── Herz-Jesus-P25/      # Church dataset (25 images)
└── common/                       # Common resources directory
    └── converter/               # Data converters
```

```{note}
**Directory Descriptions**:
- **bin/**: Contains all executable files and runtime-generated data directories
- **lib/**: Contains PoSDK aggregate library and CMake configurations
- **include/**: Contains all header files needed for development (public interfaces)
- **configs/**: Contains all plugin configuration files (read at runtime)
- **plugins/**: Contains all dynamically loaded plugin libraries
- **python/**: Contains Python auxiliary tools and scripts
- **tests/**: Contains test datasets (automatically copied from source directory)
- **common/**: Contains common resources and data definitions
```

```{tip}
**Development Tips**:
1. Modify configuration files directly in `output/configs/methods/` without rebuilding
2. Test data default path is `{exe_dir}/../tests/Strecha`, use relative paths in config files
```

### Step 3: Run PoSDK

```bash
# Run PoSDK from build directory
./output/bin/PoSDK
```

### Step 4: View Results

PoSDK generates results in the working directory:

#### Directory Structure
- **`globalsfm_pipeline_work/`** - Main working directory (configured in `globalsfm_pipeline.ini`)
- **`[dataset_name]/`** - Individual dataset results with 3D reconstruction and MeshLab outputs
- **`summary/`** - Performance analysis and evaluation reports

#### Summary Reports
| Report File                                                 | Description                                                                             |
| ----------------------------------------------------------- | --------------------------------------------------------------------------------------- |
| `profiler_performance_summary.csv`                          | **Performance Metrics** - CPU, memory usage, and execution timing                       |
| `summary_GlobalPoses_rotation_error_deg_ALL_STATS.csv`      | **Global Rotation Accuracy** - Statistical analysis of rotation estimation errors       |
| `summary_GlobalPoses_translation_error_ALL_STATS.csv`       | **Global Translation Accuracy** - Statistical analysis of translation estimation errors |
| `summary_RelativePoses_rotation_error_deg_ALL_STATS.csv`    | **Relative Rotation Accuracy** - Pairwise rotation error statistics                     |
| `summary_RelativePoses_translation_error_deg_ALL_STATS.csv` | **Relative Translation Accuracy** - Pairwise translation error statistics               |

### Verification

After successful installation and run, you should see:

- **Console Output**: Step-by-step progress and performance statistics
- **Working Directory**: `globalsfm_pipeline_work/` containing:
  - `storage/features/` - Feature extraction results
  - `storage/matches/` - Feature matching results
  - `storage/poses/` - Pose estimation results
  - `storage/logs/` - Detailed log files
- **Evaluation Reports**: Accuracy evaluation and comparative analysis results

### 故障排除

如果遇到运行问题：

1. **检查依赖路径**：
   ```bash
   ldd output/bin/PoSDK  # Linux查看动态库依赖
   otool -L output/bin/PoSDK  # macOS查看动态库依赖
   ```

2. **检查配置文件**：
   确保`output/configs/methods/globalsfm_pipeline.ini`存在且路径配置正确

3. **常见问题**：
   - **OpenGV未找到**：运行`cd src/dependencies && ./install_opengv.sh`
   - **配置文件未找到**：确保运行了完整的cmake构建流程


## FAQ 与故障排查

### 常见安装问题

#### 1. macOS上Qt版本冲突

**问题症状**：
```
Error: The `brew link` step did not complete successfully
Could not symlink share/qt/metatypes/qt6bluetooth_release_metatypes.json
Target /opt/homebrew/share/qt/metatypes/qt6bluetooth_release_metatypes.json
is a symlink belonging to qt. You can unlink it:
  brew unlink qt
```

**原因**：
- OpenCV依赖的Qt模块版本（如6.9.3）与系统安装的Qt版本（如6.8.2）不一致
- 同时安装了`qt@5`和`qt`导致符号链接冲突

**解决方案**：

方案1：升级Qt到最新版本
```bash
brew upgrade qt
```
这会自动升级qt和相关依赖（opencv、pyqt、vtk等）

方案2：手动解决链接冲突
```bash
# 如果同时存在qt@5和qt
brew unlink qt@5           # 取消qt@5的链接
brew unlink qt             # 临时取消qt的链接
brew link --overwrite qt   # 重新链接qt（覆盖冲突）
```

方案3：卸载qt@5（如果不需要）
```bash
# 检查哪些包依赖qt@5
brew uses --installed qt@5
# 如果只有pyqt@5依赖且不需要
brew uninstall pyqt@5 qt@5
```

**验证Qt版本**：
```bash
brew list --versions qt    # 查看已安装的qt版本
brew info qt               # 查看qt的最新稳定版本
```

#### 2. 依赖库安装失败

**问题**：某个依赖库编译安装失败

**解决方案**：
- 检查错误日志，查看具体失败原因
- 确保满足该依赖库的前置依赖
- 可单独重新安装该依赖：
  ```bash
  cd dependencies
  ./install_<dependency_name>.sh
  ```

#### 3. CMake版本过低

**问题**：CMake版本低于3.28

**解决方案**：

Ubuntu:
```bash
# 方案1: 使用snap安装最新版
sudo snap install cmake --classic

# 方案2: 从官网下载
wget https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0-linux-x86_64.sh
sudo bash cmake-3.27.0-linux-x86_64.sh --prefix=/usr/local --skip-license
```

macOS:
```bash
brew upgrade cmake
```

#### 4. 网络问题导致依赖下载失败

**解决方案**：
- 从GitHub Releases手动下载依赖包：[PoSDK Releases](https://github.com/pose-only-vision/PoSDK/releases)
- 将下载的文件放到相应目录：
  - `po_core` + 依赖包 → `./dependencies/`
  - 测试数据 → `./tests/`
- 重新运行 `./install.sh`（脚本会跳过已下载的内容）

### 系统要求检查

如果遇到安装问题，请先检查：

- **CMake版本**：确保CMake版本为3.28或更高
  ```bash
  cmake --version
  ```

- **依赖库路径**：检查所有源码目录是否存在于 `dependencies/` 目录下
  ```bash
  ls dependencies/
  ```

- **构建工具**：确保gcc/g++和make已正确安装
  ```bash
  gcc --version    # Linux
  clang --version  # macOS
  make --version
  ```

- **日志输出**：查看安装脚本的输出日志，了解具体失败步骤

### 获取帮助

如果问题仍未解决：
1. 检查本FAQ是否有相关问题
2. 查看各个依赖库的官方文档
3. 在[PoSDK GitHub Issues](https://github.com/pose-only-vision/PoSDK/issues)提交问题，提供：
   - 操作系统和版本
   - 错误日志
   - 已尝试的解决方案
