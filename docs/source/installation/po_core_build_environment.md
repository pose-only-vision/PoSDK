# po_core Build Environment and Version Information

This document describes PoCore core library automated workflow compilation environment configuration and dependency version requirements.

## Overview

po_core is PoSDK's core library, built and released through automated workflows on different platforms. This document lists:

- po_core supported build platforms
- Build tool version requirements
- Dependency library version information

## Build Platform Support

po_core currently supports automated builds on the following platforms:

| Platform   | Architecture | Version                    | Status          |
| ---------- | ------------ | -------------------------- | --------------- |
| **Ubuntu** | x86_64       | 24.04.3 LTS (Noble Numbat) | Fully Supported |
| **Ubuntu** | arm64        | 24.04.3 LTS (Noble Numbat) | Fully Supported |
| **macOS**  | arm64        | 15.3+                      | Fully Supported |

## Build Tool Version Requirements

### Compiler

| Platform     | Compiler  | Minimum Version | Recommended Version | Notes                       |
| ------------ | --------- | --------------- | ------------------- | --------------------------- |
| Ubuntu 24.04 | GCC       | 11.0            | 13.3.0              | C++17 support               |
| macOS        | GCC/Clang | 13.0            | 16.0.0              | Apple Clang or Homebrew GCC |

### CMake

| Platform      | Minimum Version | Recommended Version | Notes                                                 |
| ------------- | --------------- | ------------------- | ----------------------------------------------------- |
| All Platforms | 3.28            | 3.28+               | Required for some dependency libraries (e.g., GLOMAP) |

## Dependency Library Version Information

### Key Dependency Libraries

| Dependency Library | Ubuntu 24.04                              | macOS                                     |
| ------------------ | ----------------------------------------- | ----------------------------------------- |
| **Suite-Sparse**   | system_installed                          | 7.11.0 (Homebrew)                         |
| **METIS**          | system_installed                          | 5.1.0 (Homebrew)                          |
| **OpenSSL**        | 3.0.13 (statically linked)                | 3.6.0 (statically linked)                 |
| **Ceres Solver**   | 2.2.0-local (statically linked, miniglog) | 2.2.0-local (statically linked, miniglog) |

### Other Dependency Libraries

| Dependency Library | Ubuntu 24.04         | macOS           | Notes                                     |
| ------------------ | -------------------- | --------------- | ----------------------------------------- |
| **CURL**           | 8.5.0                | 8.7.1           | Dynamically linked                        |
| **OpenMP**         | libgomp (gcc-13.3.0) | libomp (21.1.3) | Dynamically linked                        |
| **OpenCV**         | 4.x-local            | 4.x-local       | Local build, path: `opencv/install_local` |

## Related Documentation

- [Basic Installation Guide](installation.md)
- [Using Precompiled Libraries](using_precompiled.md)
- [Return to Installation Documentation Home](index.md)
