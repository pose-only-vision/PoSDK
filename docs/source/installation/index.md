
# Installation Guide

This document describes PoSDK installation requirements and procedures.


---

## Quick Start

```{tip}
**Quick Installation:**

```bash
git clone https://github.com/pose-only-vision/PoSDK.git
cd PoSDK
./install.sh
```

For all prompts during installation, press Enter to use default values.
```

### Reviewer installation: use the PoSDK GUI DMG

Reviewers who only need to run a released workflow do not need to build PoSDK
from source. Download the macOS arm64 PoSDK GUI DMG and its adjacent checksum
from the [GitHub Releases page](https://github.com/pose-only-vision/PoSDK/releases),
verify the checksum, and drag `PoSDK GUI.app` to `/Applications`.

The DMG contains the built-in LiRPpaper workflows. The datasets are not bundled
with the application; download them separately and configure the dataset-loader
node after creating the workflow project. Follow the step-by-step procedure in
[Running the LiRPpaper Workflows](../lirppaper_workflows.md).

The source installation below is intended for developers who want to build
PoSDK or its plugins, not for a reviewer who only needs to reproduce the
published workflow.

---

## Installation Guide Navigation

### 1. [Complete Installation Guide](installation.md)

Complete PoSDK installation workflow documentation, including:
- System requirements and dependency checks
- One-click installation script usage instructions
- Detailed manual installation steps
- Dependency installation order and skip strategies
- Detailed installation prompt explanations
- Troubleshooting and common issues

### 2. [Using Precompiled Libraries](using_precompiled.md)

How to use precompiled po_core core library in your project:
- Core library structure description
- CMake integration methods (integrated mode vs non-integrated mode)
- Basic usage examples
- Runtime library lookup troubleshooting

### 3. [Build Environment and Version Information](po_core_build_environment.md)

po_core core library build environment configuration and dependency version requirements:
- Supported build platforms (Ubuntu 24.04/18.04, macOS)
- Build tool version requirements (GCC, Clang, CMake)
- Dependency library version information (Boost, Ceres, OpenCV, etc.)

### 4. [LiRPpaper Workflow Guide](../lirppaper_workflows.md)

How to install the GUI release, prepare the Strecha and ETH3D datasets, create
an editable workflow project, run the two LiRPpaper workflows, and inspect
Evaluator/Profiler outputs.

---

## Installation Overview

```text
┌─────────────────────────────────────────────────────────┐
│  Phase 1: System Dependencies                           │
│  └─ ./install.sh (automatically install system packages)│
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│  Phase 2: Build Dependencies                            │
│  └─ ./dependencies/install_dependencies.sh              │
│     (download and build 13 dependency libraries)         │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│  Phase 3: Build PoSDK                                    │
│  └─ cmake .. && make -j$(nproc)                         │
└─────────────────────────────────────────────────────────┘
```

For detailed instructions, please refer to [Complete Installation Guide](installation.md).

---

## Recommended Reading Order

**First-time Installation Users**:
1. If you are a reviewer → [LiRPpaper Workflow Guide](../lirppaper_workflows.md)
2. [Complete Installation Guide](installation.md) - Understand system requirements and installation process
3. If encountering network issues → [Using Precompiled Libraries](using_precompiled.md)
4. If needing dependency version information → [Build Environment and Version Information](po_core_build_environment.md)

**Experienced Users**:
- Run `./install.sh` directly, refer to relevant documentation when encountering issues

---

## Quick Links to Common Issues

- [Qt Version Conflicts on macOS](installation.md#1-qt-version-conflicts-on-macos)
- [Dependency Installation Failures](installation.md#2-dependency-library-installation-failures)
- [CMake Version Too Low](installation.md#3-cmake-version-too-low)
- [Network Download Failures](installation.md#4-network-issues-causing-dependency-download-failures)

---

## Document List

```{toctree}
:maxdepth: 2

installation
using_precompiled
po_core_build_environment
```

---

Documentation is continuously updated. For questions or issues, please submit feedback at [GitHub Issues](https://github.com/pose-only-vision/PoSDK/issues).

