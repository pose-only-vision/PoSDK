# Installation Guide

This document describes PoSDK installation requirements and procedures.

## System Requirements

- **Operating System**:
  - Primary Support: macOS | Ubuntu 24.04 LTS (Recommended)
  - Secondary Support: Other Linux distributions (Ubuntu 20.04+/CentOS 7)
  - Planned Support: Windows (future versions)
- **Compiler**: C++17 compatible compiler
  - GCC 11+ (Recommended for Linux)
  - Clang 13+ (Recommended for macOS)
- **Build Tools**: CMake 3.28+ (3.28+ required for some dependency libraries)

## One-Click Installation

PoSDK provides an automated installation script that can install all required system dependencies and plugin dependencies with one command:

```bash
cd /path/to/PoSDK/
chmod +x install.sh
./install.sh
```

The installation script automatically completes the following steps:
1. Update system package list
2. Install PoSDK framework development dependencies
3. Install dependencies required by current plugins

## PoSDK Framework Development Dependencies

The following are system dependency libraries required for normal operation of the PoSDK framework:

| Dependency Library  | Minimum Version | Purpose                                                    | Installation Command                                                                 |
| ------------------- | --------------- | ---------------------------------------------------------- | ------------------------------------------------------------------------------------ |
| **build-essential** | -               | C++ compiler and build tools                               | `sudo apt install -y build-essential`                                                |
| **CMake**           | 3.28+           | Build system generator (3.28+ required for some libraries) | Built from source (see install_cmake.sh) or `sudo apt install -y cmake`              |
| **Eigen3**          | 3.3+            | Linear algebra library for matrix and vector ops           | `sudo apt install -y libeigen3-dev`                                                  |
| **SuiteSparse**     | 7.0+            | Sparse matrix computation library (includes CHOLMOD)       | `sudo apt install -y libsuitesparse-dev`                                             |
| **OpenBLAS**        | 0.3.20+         | Optimized BLAS library for high-performance linear algebra | `sudo apt install -y libopenblas-dev`                                                |
| **Boost**           | 1.85.0          | C++ extension library (mainly filesystem and system)       | Built from source (see install_boost.sh)                                             |
| **Protobuf**        | 4.33.0          | Data serialization library                                 | Built from source (see install_protobuf.sh)                                          |
| **nlohmann_json**   | 3.11.2          | C++ JSON parsing library                                   | Built from source (see install_nlohmann.sh)                                          |
| **gflags**          | 2.2+            | Command-line argument parsing library                      | `sudo apt install -y libgflags-dev`                                                  |
| **GTest**           | 1.10+           | Unit testing framework                                     | `sudo apt install -y libgtest-dev`                                                   |
| **OpenCV**          | 4.x             | Computer vision library                                    | Built from source (see install_opencv.sh)                                            |
| **glog**            | -               | Google logging library                                     | `sudo apt install -y libgoogle-glog-dev`                                             |
| **Abseil**          | -               | Google base library                                        | Built from source (see install_absl.sh)                                              |
| **Ceres Solver**    | 2.2.0           | Nonlinear optimization library                             | Built from source (see install_ceres.sh)                                             |
| **CURL**            | -               | HTTP/HTTPS request library                                 | `sudo apt install -y libcurl4-openssl-dev`                                           |
| **OpenGL**          | -               | 3D graphics rendering library                              | `sudo apt install -y libgl1-mesa-dev mesa-common-dev libglu1-mesa-dev freeglut3-dev` |
| **GLFW3**           | -               | Window and input management library                        | `sudo apt install -y libglfw3-dev`                                                   |
| **Qt6**             | -               | GUI development framework (optional)                       | `sudo apt install qt6-base-dev qt6-declarative-dev`                                  |



(installation-flow-explained)=
## Detailed Installation Process

This section details PoSDK's multi-stage installation process, helping you understand the purpose of each installation step and when to skip redundant installations.

### Installation Process Overview

PoSDK installation is divided into three main stages:

```text
┌─────────────────────────────────────────────────────────────┐
│  Stage 1: System Dependency Installation (install.sh)        │
│  ├─ System packages (apt/brew)                              │
│  ├─ Build tools (cmake, make, gcc/clang)                   │
│  └─ System libraries (Eigen, Boost, OpenCV, etc.)          │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│  Stage 2: Dependency Download and Source Installation        │
│        (install_dependencies.sh)                            │
│  ├─ download_dependencies.sh → Download dependencies package│
│  ├─ download_po_core.sh → Download po_core binaries        │
│  └─ Individual library installations (see below)            │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│  Stage 3: PoSDK Main Project Build (cmake + make)            │
│  ├─ Configure with CMake                                    │
│  ├─ Build PoSDK core and plugins                           │
│  └─ Deploy output/ directory                               │
└─────────────────────────────────────────────────────────────┘
```

### Dependency Installation Order and Skip Strategy

`install_dependencies.sh` installs 13 dependency libraries in the following order, with each library checking for existing installations:

| Step | Dependency Library   | Purpose                                               | Installation Script     | Skip Detection                                          | When to Skip                                           |
| ---- | -------------------- | ----------------------------------------------------- | ----------------------- | ------------------------------------------------------- | ------------------------------------------------------ |
| 0    | **CMake**            | Build system (optional if version meets requirements) | `install_cmake.sh`      | CMake version check (requires 3.28+ for some libraries) | Version meets requirements (3.28+)                     |
| 1    | **Boost**            | C++ library (requires 1.85.0)                         | `install_boost.sh`      | Installation directory exists with headers and libs     | Installed with correct version (1.85.0)                |
| 2    | **Abseil**           | Google base library (C++ stdlib extension)            | `install_absl.sh`       | CMake config file (`abslConfig.cmake`)                  | After first installation, unless version update needed |
| 3    | **nlohmann/json**    | JSON parsing library (header-only)                    | `install_nlohmann.sh`   | CMake config file (`nlohmann_jsonConfig.cmake`)         | Installed and functioning properly                     |
| 4    | **Protocol Buffers** | Data serialization library                            | `install_protobuf.sh`   | CMake config file (`protobuf-config.cmake`)             | Installed and compatible with Abseil version           |
| 5    | **OpenCV**           | Computer vision library                               | `install_opencv.sh`     | CMake config file (`OpenCVConfig.cmake`) + version info | Installed with version meeting requirements            |
| 6    | **Ceres Solver**     | Nonlinear optimization library                        | `install_ceres.sh`      | CMake config file (`CeresConfig.cmake`) + library files | Installed with correct version (2.2.0)                 |
| 7    | **MAGSAC**           | Robust estimation algorithm library                   | `install_magsac.sh`     | Library files (`.so`/`.dylib`/`.a`)                     | Installed and functioning properly                     |
| 8    | **OpenGV**           | Geometric vision library                              | `install_opengv.sh`     | Library files (`.so`/`.dylib`/`.a`)                     | Installed and functioning properly                     |
| 9    | **PoseLib**          | Pose estimation library                               | `install_poselib.sh`    | Library files (`.so`/`.dylib`/`.a`)                     | Installed and functioning properly                     |
| 10   | **OpenMVG**          | Multi-view geometry library                           | `install_openmvg.sh`    | Executable files (`openMVG_main_*`)                     | Installed and functioning properly                     |
| 11   | **COLMAP**           | SfM library                                           | `install_colmap.sh`     | Executable file (`bin/COLMAP`) + version info           | Installed and functioning properly                     |
| 12   | **GraphOptim**       | Graph optimization library (depends on COLMAP)        | `install_graphoptim.sh` | Executable or library files (`rotation_estimator`)      | Installed and COLMAP not updated                       |
| 13   | **GLOMAP**           | Global SfM library                                    | `install_glomap.sh`     | Executable file (`GLOMAP` command)                      | Installed and functioning properly                     |

**Note**:
- These dependency libraries are automatically compiled and installed from source code in the `dependencies/` directory
- Installation scripts install in the order above to ensure correct dependency relationships
- All dependency source code should be pre-placed in corresponding subdirectories under `dependencies/`

### Detailed Installation Prompt Explanations

During installation, you will encounter the following prompt points. The meaning and recommendations for each are as follows:

(installation-prompt-table)=

| Stage              | Script File                | Prompt Message                                                         | Default | Meaning                                                                | Recommended Action                                                  |
| ------------------ | -------------------------- | ---------------------------------------------------------------------- | ------- | ---------------------------------------------------------------------- | ------------------------------------------------------------------- |
| **System Deps**    | `install.sh`               | `Would you like to upgrade Qt now?`                                    | `[Y/n]` | Asks if upgrade Qt when version is too old                             | Press Enter (upgrade Qt to avoid conflicts)                         |
| **System Deps**    | `install.sh`               | `Would you like to fix this automatically?`                            | `[y/N]` | Asks if auto-fix when Qt version conflict detected                     | Press Enter (don't auto-fix, manual handling safer)                 |
| **System Deps**    | `install.sh`               | `Continue with Qt6 installation?`                                      | `[y/N]` | Asks if continue installing Qt6 when Qt5 detected                      | Press Enter (skip Qt6 installation to avoid conflicts)              |
| **Download**       | `download_dependencies.sh` | `Skip download and use local package?`                                 | `[Y/n]` | Asks if skip download and use local package                            | Press Enter (use local package, extract all and overwrite existing) |
| **Download**       | `download_po_core.sh`      | `Skip reinstall and use existing po_core installation?`                | `[Y/n]` | Asks if skip reinstall when po_core detected                           | Press Enter (skip installation to save time)                        |
| **Download**       | `download_po_core.sh`      | `Skip download and use local package?`                                 | `[Y/n]` | Asks if skip download when local po_core package detected              | Press Enter (use local package to save bandwidth)                   |
| **Test Data**      | `download_Strecha.sh`      | `Do you want to re-download test data?`                                | `[y/N]` | Asks if re-download when test data exists                              | Press Enter (skip re-download)                                      |
| **Test Data**      | `download_Strecha.sh`      | `Skip download and use existing local package?`                        | `[Y/n]` | Asks if skip download when local test data package detected            | Press Enter (use local package)                                     |
| **Build Deps**     | `install_cmake.sh`         | `Version sufficient, skip installation?`                               | `[Y/n]` | Asks if skip installation when CMake version sufficient                | Press Enter (skip installation to save time)                        |
| **Build Deps**     | `install_boost.sh`         | `Skip rebuild and use existing Boost installation?`                    | `[Y/n]` | Asks if skip rebuild when Boost detected                               | Press Enter (skip rebuild)                                          |
| **Build Deps**     | `install_absl.sh`          | `Do you want to skip this installation and use the existing build?`    | `[Y/n]` | Asks if skip installation when Abseil detected                         | Press Enter (skip installation)                                     |
| **Build Deps**     | `install_nlohmann.sh`      | `Do you want to skip this installation and use the existing build?`    | `[Y/n]` | Asks if skip installation when nlohmann/json detected                  | Press Enter (skip installation)                                     |
| **Build Deps**     | `install_protobuf.sh`      | `Do you want to skip this installation and use the existing build?`    | `[Y/n]` | Asks if skip installation when Protocol Buffers detected               | Press Enter (skip installation)                                     |
| **Build Deps**     | `install_opencv.sh`        | (No skip prompt, automatically detects and uses existing installation) | -       | OpenCV installation script auto-detects and uses existing installation | No action needed (auto-handled)                                     |
| **Build Deps**     | `install_ceres.sh`         | `Skip rebuild and use existing Ceres Solver installation?`             | `[Y/n]` | Asks if skip rebuild when Ceres detected                               | Press Enter (skip rebuild to save time)                             |
| **Build Deps**     | `install_magsac.sh`        | `Skip rebuild and use existing MAGSAC installation?`                   | `[Y/n]` | Asks if skip rebuild when MAGSAC detected                              | Press Enter (skip rebuild)                                          |
| **Build Deps**     | `install_opengv.sh`        | `Skip rebuild and use existing OpenGV installation?`                   | `[Y/n]` | Asks if skip rebuild when OpenGV detected                              | Press Enter (skip rebuild)                                          |
| **Build Deps**     | `install_poselib.sh`       | `Skip rebuild and use existing PoseLib installation?`                  | `[Y/n]` | Asks if skip rebuild when PoseLib detected                             | Press Enter (skip rebuild)                                          |
| **Build Deps**     | `install_openmvg.sh`       | `Skip rebuild and use existing OpenMVG installation?`                  | `[Y/n]` | Asks if skip rebuild when OpenMVG detected                             | Press Enter (skip rebuild)                                          |
| **Build Deps**     | `install_colmap.sh`        | `Skip rebuild and use existing COLMAP installation?`                   | `[Y/n]` | Asks if skip rebuild when COLMAP detected                              | Press Enter (skip rebuild)                                          |
| **Build Deps**     | `install_graphoptim.sh`    | `Skip rebuild and use existing GraphOptim build?`                      | `[Y/n]` | Asks if skip rebuild when GraphOptim detected                          | Press Enter (skip rebuild)                                          |
| **Build Deps**     | `install_graphoptim.sh`    | `Skip building internal COLMAP support (build core only)?`             | `[Y/n]` | Asks if skip building internal COLMAP support                          | Press Enter (skip COLMAP support, build core only)                  |
| **Build Deps**     | `install_glomap.sh`        | `Skip this installation and use existing build? [Y/n]`                 | `[Y/n]` | Asks if skip installation when GLOMAP detected                         | Press Enter (skip installation)                                     |
| **Build Deps**     | `install_glomap.sh`        | `Use local dependencies for build?`                                    | `[Y/n]` | Asks if use local dependencies to build GLOMAP                         | Press Enter (use local dependencies)                                |
| **Error Handling** | `install_dependencies.sh`  | `Continue with remaining installations?`                               | `[Y/n]` | Asks if continue with remaining when a dependency fails                | Press Enter (continue installing other dependencies)                |

```{important}
**Default Values Usage:**

For first-time installation and normal use cases, press Enter to use default values for all prompts.


Default value design principles:
- `[Y/n]` Default skip redundant operations to save time and resources
- `[y/N]` Default conservative actions to avoid accidental overwrites or conflicts

Manual selection needed in the following cases:
- **Force Rebuild**: Suspect a dependency installation issue, enter `n` to force reinstall
- **Version Update**: Need to update to latest version, enter `n` to redownload and rebuild
- **Troubleshooting**: Errors during installation, need to reinstall specific components
```
  
```{tip}
**Skip Detection Notes**:
- **CMake Config Files**: Check if standard CMake config files exist (e.g., `OpenCVConfig.cmake`) to ensure libraries can be found by CMake
- **Library Files**: Check if build artifacts (`.so`, `.dylib`, `.a`) exist in installation directory
- **Executable Files**: Check if program binary files exist and are executable
- **Version Information**: Some libraries (OpenCV, COLMAP) additionally display installed version numbers
```



## Quick Start

You can quickly launch the PoSDK Global SfM pipeline by following these steps:

### Prerequisites

- CMake 3.28+ (required for some dependency libraries)
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
**Default Values:**

For normal installation: Press Enter (default values) to complete the installation, no additional selection needed.

```

```{note}
**Installation Prompts:**

During installation, you'll see multiple `[Y/n]` or `[y/N]` prompts to detect existing installations and ask whether to skip redundant installations.


**Prompt Format:**
- `[Y/n]` - Default is **Y** (skip), just press Enter
- `[y/N]` - Default is **N** (continue), just press Enter

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
│       ├── castle-P30/         # Castle dataset (30 images)
│       ├── entry-P10/          # Entry dataset (10 images)
│       ├── fountain-P11/        # Fountain dataset (11 images)
│       ├── Herz-Jesus-P8/      # Church dataset (8 images)
│       └── Herz-Jesus-P25/     # Church dataset (25 images)
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

### Troubleshooting

If encountering runtime issues:

1. **Check Dependency Paths**:
   ```bash
   ldd output/bin/PoSDK  # Linux: check dynamic library dependencies
   otool -L output/bin/PoSDK  # macOS: check dynamic library dependencies
   ```

2. **Check Configuration Files**:
   Ensure `output/configs/methods/globalsfm_pipeline.ini` exists and path configuration is correct

3. **Common Issues**:
   - **OpenGV not found**: Run `cd src/dependencies && ./install_opengv.sh`
   - **Configuration file not found**: Ensure complete cmake build process was run


## FAQ and Troubleshooting

### Common Installation Issues

#### 1. Qt Version Conflicts on macOS

**Symptoms**:
```
Error: The `brew link` step did not complete successfully
Could not symlink share/qt/metatypes/qt6bluetooth_release_metatypes.json
Target /opt/homebrew/share/qt/metatypes/qt6bluetooth_release_metatypes.json
is a symlink belonging to qt. You can unlink it:
  brew unlink qt
```

**Cause**:
- Qt module version (e.g., 6.9.3) required by OpenCV conflicts with system-installed Qt version (e.g., 6.8.2)
- Simultaneous installation of `qt@5` and `qt` causes symlink conflicts

**Solutions**:

Solution 1: Upgrade Qt to latest version
```bash
brew upgrade qt
```
This automatically upgrades qt and related dependencies (opencv, pyqt, vtk, etc.)

Solution 2: Manually resolve link conflicts
```bash
# If both qt@5 and qt exist
brew unlink qt@5           # Unlink qt@5
brew unlink qt             # Temporarily unlink qt
brew link --overwrite qt   # Re-link qt (overwrite conflicts)
```

Solution 3: Uninstall qt@5 (if not needed)
```bash
# Check which packages depend on qt@5
brew uses --installed qt@5
# If only pyqt@5 depends and not needed
brew uninstall pyqt@5 qt@5
```

**Verify Qt Version**:
```bash
brew list --versions qt    # View installed qt versions
brew info qt               # View latest stable qt version
```

#### 2. Dependency Library Installation Failures

**Issue**: A dependency library compilation/installation failed

**Solution**:
- Check error logs to identify specific failure cause
- Ensure prerequisites for that dependency library are met
- Can reinstall the dependency individually:
  ```bash
  cd dependencies
  ./install_<dependency_name>.sh
  ```

#### 3. CMake Version Too Low

**Issue**: CMake version is below 3.28

**Solution**:

Ubuntu:
```bash
# Solution 1: Install latest using snap
sudo snap install cmake --classic

# Solution 2: Download from official website
wget https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0-linux-x86_64.sh
sudo bash cmake-3.27.0-linux-x86_64.sh --prefix=/usr/local --skip-license
```

macOS:
```bash
brew upgrade cmake
```

#### 4. Network Issues Causing Dependency Download Failures

**Solution**:
- Manually download dependency packages from GitHub Releases: [PoSDK Releases](https://github.com/pose-only-vision/PoSDK/releases)
- Place downloaded files in corresponding directories:
  - `po_core` + dependency packages → `./dependencies/`
  - Test data → `./tests/`
- Re-run `./install.sh` (script will skip already downloaded content)

### System Requirements Check

If encountering installation issues, first check:

- **CMake Version**: Ensure CMake version is 3.28 or higher
  ```bash
  cmake --version
  ```

- **Dependency Library Paths**: Check if all source directories exist under `dependencies/` directory
  ```bash
  ls dependencies/
  ```

- **Build Tools**: Ensure gcc/g++ and make are correctly installed
  ```bash
  gcc --version    # Linux
  clang --version  # macOS
  make --version
  ```

- **Log Output**: Check installation script output logs to understand specific failure steps

### Getting Help

If the issue remains unresolved:
1. Check this FAQ for related issues
2. Refer to official documentation for each dependency library
3. Submit an issue at [PoSDK GitHub Issues](https://github.com/pose-only-vision/PoSDK/issues), providing:
   - Operating system and version
   - Error logs
   - Solutions already attempted
