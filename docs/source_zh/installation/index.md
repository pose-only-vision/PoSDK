
# 安装指南

本文档说明 PoSDK 的安装要求和步骤。


---

## 快速开始 | Quick Start

```{tip}
**快速安装方式 | Quick Installation:**

```bash
git clone https://github.com/pose-only-vision/PoSDK.git
cd PoSDK
./install.sh
```

安装过程中的提示可直接按回车键使用默认值。

For all prompts during installation, press Enter to use default values.
```

---

## 安装文档导航 | Installation Guide Navigation

### 1. [完整安装指南](installation.md)

完整的 PoSDK 安装流程文档，包括：
- 系统要求和依赖检查
- 一键安装脚本使用说明
- 手动安装步骤详解
- 依赖库安装顺序和跳过策略
- 详细的安装提示节点说明
- 故障排除和常见问题

**Complete PoSDK installation guide:**
- System requirements and dependency checks
- One-click installation script usage
- Manual installation steps
- Dependency installation order and skip strategies
- Detailed installation prompt explanations
- Troubleshooting and FAQ

### 2. [使用预编译库](using_precompiled.md)

如何在项目中使用预编译的 po_core 核心库：
- 核心库结构说明
- CMake 集成方式（集成模式 vs 非集成模式）
- 基本使用示例
- 运行时库查找问题解决

**How to use precompiled po_core libraries:**
- Core library structure
- CMake integration methods
- Basic usage examples
- Runtime library troubleshooting

### 3. [编译环境与版本信息](po_core_build_environment.md)

po_core 核心库的构建环境配置和依赖版本要求：
- 支持的构建平台（Ubuntu 24.04/18.04, macOS）
- 构建工具版本要求（GCC, Clang, CMake）
- 依赖库版本信息（Boost, Ceres, OpenCV 等）

**po_core build environment and dependency versions:**
- Supported platforms
- Build tool requirements
- Dependency version information

---

## 安装流程概览 | Installation Overview

```text
┌─────────────────────────────────────────────────────────┐
│  Phase 1: 系统依赖安装 | System Dependencies           │
│  └─ ./install.sh (自动安装系统包)                       │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│  Phase 2: 依赖库构建 | Build Dependencies              │
│  └─ ./dependencies/install_dependencies.sh              │
│     (下载并构建 13 个依赖库)                             │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│  Phase 3: PoSDK 构建 | Build PoSDK                     │
│  └─ cmake .. && make -j$(nproc)                         │
└─────────────────────────────────────────────────────────┘
```

详细说明请参考 [完整安装指南](installation.md)。

---

## 推荐阅读顺序 | Recommended Reading Order

**首次安装用户**：
1. [完整安装指南](installation.md) - 了解系统要求和安装流程
2. 如遇网络问题 → [使用预编译库](using_precompiled.md)
3. 如需了解依赖版本 → [编译环境与版本信息](po_core_build_environment.md)

**已有经验用户**：
- 直接运行 `./install.sh`，遇到问题时查阅相应文档

---

## 常见问题快速链接 | Quick Links

- [Qt 版本冲突](installation.md#1-macos上qt版本冲突错误)
- [依赖库安装失败](installation.md#2-依赖库安装失败)
- [CMake 版本过低](installation.md#3-cmake版本过低)
- [网络下载失败](installation.md#4-网络问题导致依赖下载失败)

---

## 文档列表 | Document List

```{toctree}
:maxdepth: 2

installation
using_precompiled
po_core_build_environment
```

---

文档持续更新中。如有疑问或发现问题，请在 [GitHub Issues](https://github.com/pose-only-vision/PoSDK/issues) 提交反馈。

Documentation is continuously updated. For questions or issues, please submit feedback at [GitHub Issues](https://github.com/pose-only-vision/PoSDK/issues).

