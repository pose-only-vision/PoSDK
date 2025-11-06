# PoSDK Dependencies | PoSDK依赖库

此目录用于存放PoSDK编译所需的所有依赖库。为了控制仓库大小，依赖库已单独打包发布。

This directory contains all dependencies required for PoSDK compilation. To control repository size, dependencies are packaged separately for release.

## 🚀 自动安装 | Automatic Installation

**推荐方式 | Recommended**: 使用根目录的install.sh脚本自动下载和安装所有依赖：

```bash
# 在PoSDK根目录运行 | Run in PoSDK root directory
./install.sh
```

此脚本将自动：
- 检测您的平台 (arm64-macos, x86_64-macos, arm64-ubuntu24.04, x86_64-ubuntu24.04)
- 从GitHub Releases下载对应的dependencies包
- 解压到此目录
- 自动编译安装所有依赖

This script will automatically:
- Detect your platform (arm64-macos, x86_64-macos, arm64-ubuntu24.04, x86_64-ubuntu24.04)
- Download the corresponding dependencies package from GitHub Releases
- Extract to this directory
- Automatically compile and install all dependencies

## 📦 手动安装 | Manual Installation

如果需要手动下载和安装：

1. **下载dependencies包 | Download dependencies package**:
   访问 [PoSDK Releases](https://github.com/pose-only-vision/PoSDK/releases) 页面，下载适合您平台的dependencies包：

   ```bash
   # 例如，对于arm64-macos平台：
   wget https://github.com/pose-only-vision/PoSDK/releases/download/deps-v1.0.0/PoSDK_dependencies_arm64-macos_deps-v1.0.0.tar.gz
   ```

2. **解压到dependencies目录 | Extract to dependencies directory**:
   ```bash
   cd /path/to/PoSDK/src
   tar -xzf PoSDK_dependencies_*_deps-v*.tar.gz
   mv posdk_deps_staging/* dependencies/
   rmdir posdk_deps_staging
   ```

3. **安装依赖 | Install dependencies**:
   ```bash
   cd dependencies
   ./install_dependencies.sh
   ```

---

## 📋 依赖库分类与版本

### ✅ PoSDK Pipeline 必需依赖（必须安装）

这些依赖库是 PoSDK Pipeline 功能所必需的：

| 依赖库               | 版本       | 安装路径                      | 用途                               |
| -------------------- | ---------- | ----------------------------- | ---------------------------------- |
| **Eigen3**           | 3.4.0+     | 系统安装                      | 线性代数库，所有几何计算的基础     |
| **CMake**            | 3.28+      | `cmake/install_local`         | 构建系统（GLOMAP 要求 3.28+）      |
| **Boost**            | 1.85.0     | `boost_1_85_0/install_local`  | C++ 标准库扩展                     |
| **Abseil**           | 20250814.1 | `abseil-cpp/install_local`    | Google C++ 基础库集合              |
| **Protocol Buffers** | 33.0.0     | `protobuf/install_local`      | 数据序列化格式库                   |
| **nlohmann/json**    | 最新       | `nlohmann/json` (header-only) | JSON 解析库                        |
| **Cereal**           | 1.3.2      | `cereal` (header-only)        | C++ 序列化库                       |
| **OpenCV**           | 4.x        | `opencv/install_local`        | 计算机视觉库（图像处理、特征提取） |
| **GraphOptim**       | 2.1.3      | `GraphOptim/build_scripted`   | 图优化库（Pipeline 优化）          |

**安装顺序**: 按照 `install_dependencies.sh` 中的顺序依次安装。

---

### 🔌 可选插件依赖（按需安装）

#### 1. **BarathTwoViewEstimator 插件**

| 依赖库             | 版本  | 安装路径                      | 说明            |
| ------------------ | ----- | ----------------------------- | --------------- |
| **MAGSAC**         | 1.0.0 | `magsac-master/install_local` | 鲁棒估计算法库  |
| **GraphCutRANSAC** | -     | `magsac-master/build_local`   | RANSAC 变体算法 |

**依赖关系**:
```
BarathTwoViewEstimator
  └── MAGSAC (required)
       └── GraphCutRANSAC (bundled with MAGSAC)
```

**安装方法**:
```bash
cd dependencies
./install_magsac.sh
```

---

#### 2. **OpenMVGPipeline 插件**

| 依赖库      | 版本  | 安装路径                | 说明         |
| ----------- | ----- | ----------------------- | ------------ |
| **OpenMVG** | 2.0.0 | `openMVG/install_local` | 多视图几何库 |

**依赖关系**:
```
OpenMVGPipeline
  └── OpenMVG (required for image listing and feature extraction)
```

**安装方法**:
```bash
cd dependencies
./install_openmvg.sh
```

---

#### 3. **COLMAP/GLOMAP 插件**

包含两个插件：
- `colmap_preprocess` - COLMAP 增量式 SfM 流程
- `glomap_preprocess` - GLOMAP 全局 SfM 优化流程

| 依赖库           | 版本  | 安装路径                           | 说明                             |
| ---------------- | ----- | ---------------------------------- | -------------------------------- |
| **COLMAP**       | 3.8.0 | `colmap/install_local`             | 三维重建库                       |
| **GLOMAP**       | -     | `glomap/install_local`             | 全局 SfM 优化库                  |
| **PoseLib**      | 1.0.5 | `PoseLib/install_local`            | 位姿估计库（COLMAP/GLOMAP 依赖） |
| **Ceres Solver** | 2.2.0 | `ceres-solver-2.2.0/install_local` | 非线性优化库（COLMAP 依赖）      |

**依赖关系**:
```
colmap_preprocess / glomap_preprocess
  └── COLMAP (required for colmap_preprocess)
       └── Ceres Solver 2.2.0 (required)
            └── Suite-Sparse (system dependency)
            └── METIS (system dependency)
  └── GLOMAP (required for glomap_preprocess)
       └── Ceres Solver 2.2.0 (required)
       └── PoseLib 1.0.5 (required)
            └── Ceres Solver (indirect dependency)
```

**安装方法**:
```bash
cd dependencies
# 安装 Ceres（COLMAP 和 GLOMAP 都依赖）
./install_ceres.sh

# 安装 PoseLib（GLOMAP 依赖）
./install_poselib.sh

# 安装 COLMAP
./install_colmap.sh

# 安装 GLOMAP
./install_glomap.sh
```

**注意**: COLMAP 和 GraphOptim 安装脚本支持自动重试机制（处理网络问题）。

---

#### 4. **OpenGVModelEstimator 插件**

| 依赖库     | 版本 | 安装路径               | 说明       |
| ---------- | ---- | ---------------------- | ---------- |
| **OpenGV** | 1.0  | `opengv/install_local` | 几何视觉库 |

**依赖关系**:
```
OpenGVModelEstimator
  └── OpenGV (required)
```

**安装方法**:
```bash
cd dependencies
./install_opengv.sh
```

---

#### 5. **PoseLibModelEstimator 插件**

| 依赖库      | 版本  | 安装路径                | 说明       |
| ----------- | ----- | ----------------------- | ---------- |
| **PoseLib** | 1.0.5 | `PoseLib/install_local` | 位姿估计库 |

**依赖关系**:
```
PoseLibModelEstimator
  └── PoseLib (required)
```

**安装方法**:
```bash
cd dependencies
./install_poselib.sh
```

---

## 🔗 完整依赖关系图

```
PoSDK Pipeline (必需)
├── Eigen3 3.4.0+
├── CMake 3.28+
├── Boost 1.85.0
├── Abseil 20250814.1
├── Protocol Buffers 33.0.0
│   └── Abseil (dependency)
├── nlohmann/json (header-only)
├── Cereal 1.3.2 (header-only)
├── OpenCV 4.x
└── GraphOptim 2.1.3
    └── PoseLib 1.0.5
    └── Boost 1.85.0

可选插件:
├── BarathTwoViewEstimator
│   └── MAGSAC 1.0.0
│
├── OpenMVGPipeline
│   └── OpenMVG 2.0.0
│
├── COLMAP/GLOMAP Preprocess
│   ├── COLMAP 3.8.0
│   │   └── Ceres Solver 2.2.0
│   │       └── Suite-Sparse (system)
│   │       └── METIS (system)
│   └── GLOMAP
│       └── Ceres Solver 2.2.0 (shared with COLMAP)
│       └── PoseLib 1.0.5 (shared with GraphOptim)
│
├── OpenGVModelEstimator
│   └── OpenGV 1.0
│
└── PoseLibModelEstimator
    └── PoseLib 1.0.5 (shared with GLOMAP/GraphOptim)
```

---

## 📊 依赖库版本总结表

### 必需依赖（PoSDK Pipeline）

| 依赖库           | 版本       | 安装目录                     | 备注                 |
| ---------------- | ---------- | ---------------------------- | -------------------- |
| Eigen3           | 3.4.0+     | 系统安装                     | 通过系统包管理器安装 |
| CMake            | 3.28+      | `cmake/install_local`        | GLOMAP 要求 3.28+    |
| Boost            | 1.85.0     | `boost_1_85_0/install_local` | C++ 库扩展           |
| Abseil           | 20250814.1 | `abseil-cpp/install_local`   | Google C++ 基础库    |
| Protocol Buffers | 33.0.0     | `protobuf/install_local`     | 数据序列化           |
| nlohmann/json    | 最新       | `nlohmann/json`              | Header-only          |
| Cereal           | 1.3.2      | `cereal`                     | Header-only          |
| OpenCV           | 4.x        | `opencv/install_local`       | 计算机视觉库         |
| GraphOptim       | 2.1.3      | `GraphOptim/build_scripted`  | 图优化库             |

### 可选依赖（按插件）

| 插件                       | 依赖库       | 版本  | 安装目录                           |
| -------------------------- | ------------ | ----- | ---------------------------------- |
| **BarathTwoViewEstimator** | MAGSAC       | 1.0.0 | `magsac-master/install_local`      |
| **OpenMVGPipeline**        | OpenMVG      | 2.0.0 | `openMVG/install_local`            |
| **colmap_preprocess**      | COLMAP       | 3.8.0 | `colmap/install_local`             |
| **colmap_preprocess**      | Ceres Solver | 2.2.0 | `ceres-solver-2.2.0/install_local` |
| **glomap_preprocess**      | GLOMAP       | -     | `glomap/install_local`             |
| **glomap_preprocess**      | Ceres Solver | 2.2.0 | `ceres-solver-2.2.0/install_local` |
| **glomap_preprocess**      | PoseLib      | 1.0.5 | `PoseLib/install_local`            |
| **OpenGVModelEstimator**   | OpenGV       | 1.0   | `opengv/install_local`             |
| **PoseLibModelEstimator**  | PoseLib      | 1.0.5 | `PoseLib/install_local`            |

---

## 🛠️ 安装路径说明

所有本地构建的依赖库统一使用 `install_local` 目录结构：

```
dependencies/
├── abseil-cpp/
│   └── install_local/      # Abseil 安装位置
├── boost_1_85_0/
│   └── install_local/     # Boost 安装位置
├── protobuf/
│   └── install_local/     # Protobuf 安装位置
├── opencv/
│   └── install_local/     # OpenCV 安装位置（统一路径）
├── ceres-solver-2.2.0/
│   └── install_local/     # Ceres 安装位置
├── PoseLib/
│   └── install_local/     # PoseLib 安装位置
├── opengv/
│   └── install_local/     # OpenGV 安装位置
├── openMVG/
│   └── install_local/     # OpenMVG 安装位置
├── colmap/
│   └── install_local/     # COLMAP 安装位置
└── glomap/
    └── install_local/     # GLOMAP 安装位置
```

**注意**: OpenCV 路径已统一为 `opencv/install_local`（之前为 `opencv_install`）。

---

## ⚠️ 重要说明 | Important Notes

### 版本兼容性 | Version Compatibility

- **Ceres Solver**: 所有使用 Ceres 的插件（COLMAP、GLOMAP）必须使用相同版本（2.2.0）
- **PoseLib**: GraphOptim、GLOMAP、PoseLibModelEstimator 共享同一 PoseLib 安装
- **Boost**: GraphOptim 要求 Boost 1.85.0
- **CMake**: GLOMAP 要求 CMake 3.28+

### 系统依赖 | System Dependencies

某些依赖库需要系统级依赖：

| 依赖库           | 系统依赖            | Ubuntu 安装命令                                        |
| ---------------- | ------------------- | ------------------------------------------------------ |
| **Ceres Solver** | Suite-Sparse, METIS | `sudo apt-get install libsuitesparse-dev libmetis-dev` |
| **COLMAP**       | Qt5, SQLite3        | `sudo apt-get install qt5-default libsqlite3-dev`      |
| **OpenCV**       | FFMPEG, Python 3.8+ | 自动检测并安装                                         |

### 构建产物 | Build Artifacts

- 所有依赖库在本地构建（不在发布包中）
- 构建产物不包含在 Git 仓库中
- 每次安装都会重新编译以确保最佳兼容性

---

## 🔧 故障排除 | Troubleshooting

### 下载失败 | Download Failed
```bash
# 检查网络连接和GitHub访问
# Check network connection and GitHub access
curl -I https://github.com/pose-only-vision/PoSDK/releases

# 手动下载后使用本地文件
# Use local file after manual download
tar -xzf your_downloaded_file.tar.gz
```

### 编译失败 | Compilation Failed
```bash
# 清理并重新安装特定依赖
# Clean and reinstall specific dependency
cd dependencies
rm -rf problematic_library/build*
./install_<library>.sh
```

### COLMAP/GraphOptim 安装失败 | COLMAP/GraphOptim Installation Failed
这两个库的安装脚本支持自动重试机制（处理网络问题）：
```bash
# 脚本会自动重试最多2次
# Scripts will automatically retry up to 2 times
cd dependencies
./install_colmap.sh    # 自动重试
./install_graphoptim.sh  # 自动重试
```

### OpenCV 路径问题 | OpenCV Path Issue
如果 CMake 找不到 OpenCV：
```bash
# 检查 OpenCV 是否安装在正确位置
ls dependencies/opencv/install_local/lib/cmake/opencv4/

# 如果不存在，重新安装 OpenCV
cd dependencies
./install_opencv.sh
```

### 平台检测错误 | Platform Detection Error
```bash
# 手动指定平台下载对应包
# Manually specify platform and download corresponding package
uname -m  # 查看架构
uname -s  # 查看系统
```

---

## 📝 插件开发指南

如果您要开发新的 PoSDK 插件，需要了解：

### 必需依赖（所有插件）
- Eigen3
- Boost
- OpenCV
- po_core (通过 PoSDK 提供)

### 可选依赖（根据插件功能）
- **几何优化**: Ceres Solver
- **位姿估计**: PoseLib, OpenGV
- **鲁棒估计**: MAGSAC
- **SfM 流程**: COLMAP, GLOMAP, OpenMVG
- **图优化**: GraphOptim

### 依赖管理建议
1. 优先使用 PoSDK 统一管理的依赖版本
2. 避免引入新的外部依赖（除非必要）
3. 静态链接关键依赖以避免版本冲突
4. 文档化插件的依赖要求

---

## 🔍 验证安装

安装完成后，可以验证依赖是否正确安装：

```bash
# 检查必需依赖
ls dependencies/opencv/install_local/lib/cmake/opencv4/
ls dependencies/protobuf/install_local/lib/cmake/protobuf/
ls dependencies/abseil-cpp/install_local/lib/cmake/absl/

# 检查可选依赖（根据您使用的插件）
ls dependencies/ceres-solver-2.2.0/install_local/lib/cmake/Ceres/
ls dependencies/PoseLib/install_local/lib/
```

---

📧 如有问题，请访问 [PoSDK Issues](https://github.com/pose-only-vision/PoSDK/issues) 提交反馈。

For questions, please visit [PoSDK Issues](https://github.com/pose-only-vision/PoSDK/issues) to submit feedback.
