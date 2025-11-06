(copyright-tracking)=
# Copyright Tracking

PoSDK's built-in **automatic copyright tracking** automatically collects, manages, and displays copyright information for all plugins and algorithms at runtime, maintaining transparency and traceability of intellectual property.

## How It Works

```{figure} _static/copyright.png
:alt: Copyright Tracking Workflow
:align: center
:width: 90%

Copyright Tracking Feature Diagram
```

As shown in the figure above, when Method A depends on multiple algorithms (a/b/c/d/e/f), the system automatically tracks the call chain, collects copyright information (copyright_1 to copyright_5), automatically deduplicates (e.g., copyright_2 is used by multiple algorithms but displayed only once), and presents a unified summary.

## Features

### Automatic Copyright Collection and Display

When the application runs, the system automatically:

- **Collects Copyright Information**: Automatically records copyright information of called plugins or methods 

- **Instant Display**: Displays copyright declarations when methods are first used  

- **Dependency Tracking**: Automatically identifies and records call relationships between methods  

- **Deduplication**: Copyright information for the same method is displayed only once  

- **Summary Report**: Generates complete copyright list and dependency graph at the end of execution  

### Copyright Information Display Examples

#### Instant Display When Method is First Loaded

```text
============= Plugin Copyright Information =============
Method Type: SIFT_FeatureExtractor
Plugin Version: 1.2.3
Build Time: 2025-01-15 10:30:45
Copyright Identifier: OpenCV
Copyright (c) 2000-2025, Intel Corporation, all rights reserved.
Copyright (c) 2009-2011, Willow Garage Inc., all rights reserved.
Third-party copyrights are property of their respective owners.
License: Apache License 2.0
=====================================
```

#### Copyright Summary at End of Process

```text
============= Copyright Information List =============
[ Copyright.1 ]
Copyright Identifier: PoSDK
Copyright (c) 2025 PoSDK/PoCore author: Qi Cai.
Based on Pose-only Imaging Geometry Theory.
License: Proprietary
------------------------------

[ Copyright.2 ]
Copyright Identifier: OpenCV
Copyright (c) 2000-2025, Intel Corporation, all rights reserved.
Copyright (c) 2009-2011, Willow Garage Inc., all rights reserved.
License: Apache License 2.0
------------------------------

[ Copyright.3 ]
Copyright Identifier: Eigen
Copyright (c) Eigen Authors
License: MPL2 (Mozilla Public License 2.0)
------------------------------
==========================================

============= Dependency Graph =============
[Method: FeatureMatchingPipeline | Copyright Identifier: PoSDK]
     << OpenCV | Eigen

[Method: SIFT_Detector | Copyright Identifier: OpenCV]
     << (No dependencies)

[Method: BundleAdjustment | Copyright Identifier: PoSDK]
     << Ceres | Eigen
==========================================
```

## How to Enable Copyright Tracking

PoSDK provides two ways to enable copyright tracking for plugins, depending on which base class your Method derives from.

### Method 1: Derive from `MethodPreset` (Recommended)

**This is the simplest way**. If your Method inherits from `MethodPreset` or its derived classes (such as `MethodPresetProfiler`), copyright tracking is **automatically enabled**.

You only need to:

1. **Override the `Copyright()` function** to return your copyright information
2. Nothing else needs to be done!

```cpp
class MyMethod : public Interface::MethodPreset {
public:
    // Just override the Copyright() function
    Interface::CopyrightData Copyright() const override {
        return {
            "MyOrganization",
            "Copyright (c) 2025 MyOrganization\n"
            "License: MIT"
        };
    }
    
    // Other method implementations...
};
```

`MethodPreset::Build()` automatically handles starting and stopping copyright tracking.

### Method 2: Derive Directly from `Method`

If your Method **directly inherits from the `Method` base class**, you need to **manually manage copyright tracking**:

1. **Override the `Copyright()` function**
2. Call `StartCopyrightTracking()` and `StopCopyrightTracking()` in the `Build()` method

```cpp
class MyMethod : public Interface::Method {
public:
    // 1. Override the Copyright() function
    Interface::CopyrightData Copyright() const override {
        return {
            "MyOrganization",
            "Copyright (c) 2025 MyOrganization\n"
            "License: MIT"
        };
    }
    
    // 2. Manually manage copyright tracking in Build()
    Interface::DataPtr Build() override {
        // Important: Start copyright tracking before processing
        StartCopyrightTracking();
        
        try {
            // Your algorithm implementation...
            Interface::DataPtr result = ProcessData();
            
            // Important: Stop copyright tracking before returning
            StopCopyrightTracking();
            return result;
        }
        catch (const std::exception& e) {
            // Also stop tracking in exception handling
            StopCopyrightTracking();
            throw;
        }
    }
    
    // Other method implementations...
};
```

#### Important Warning

If you derive directly from `Method` but **do not call** `StartCopyrightTracking()` and `StopCopyrightTracking()`:

- ✗ **Your plugin will not be tracked by the copyright manager**
- ✗ **Copyright information will be lost**
- ✗ **Your plugin will not appear in the dependency graph**
- ✗ **At your own risk**: This may lead to copyright compliance issues

### Copyright Tracking Function Documentation

```cpp
// Functions provided in the Method base class
protected:
    /**
     * @brief Start copyright tracking
     * @details Adds the current method to the copyright manager's tracking stack
     *          Called at the beginning of Build()
     */
    void StartCopyrightTracking();
    
    /**
     * @brief Stop copyright tracking
     * @details Removes the current method from the copyright manager's tracking stack
     *          Called at the end of Build() (including exception cases)
     */
    void StopCopyrightTracking();
```

### Selection Recommendations

| Derivation Method                 | Copyright Tracking         | Recommended Scenarios                      |
| --------------------------------- | -------------------------- | ------------------------------------------ |
| `MethodPreset` or derived classes | Automatically enabled      | ✅ **Recommended**: Most plugin development |
| Direct derivation from `Method`   | Manual management required | Use only when special requirements exist   |

**Summary**: Unless there are special reasons, you should derive from `MethodPreset`, so copyright tracking is handled automatically.

### Reference Implementation

You can refer to the following files for implementation details:

- `/po_core/src/internal/interfaces.hpp` - `Method` base class definition
- `/po_core/src/internal/interfaces.cpp` - Copyright tracking function implementation
- `/po_core/src/internal/interfaces_preset.hpp` - `MethodPreset` automatic copyright tracking implementation

---

## How to Define Copyright Information in Plugins

Plugin developers need to **override the `Copyright()` function** in their method class:

### Basic Usage

```cpp
// In your Method-derived class
class MyFeatureExtractor : public Interface::Method {
public:
    // Override Copyright() function to return copyright info
    Interface::CopyrightData Copyright() const override {
        return {
            "YourOrganization",  // Copyright identifier

            // Detailed copyright information
            "Copyright (c) 2025 YourOrganization, All rights reserved.\n"
            "License: MIT License\n"
            "Author: Your Name\n"
            "Email: your.email@example.com"
        };
    }

    // Other method implementations...
};
```

### Default Behavior

If this function is not overridden, the default implementation returns:
```cpp
return std::make_pair("Unknown", "");
```

This means the plugin will be classified as **no copyright identifier type**.

### Copyright Information Writing Guidelines

It is recommended to include the following content in copyright information:

1. **Copyright Statement** - Copyright owner and year
2. **License Type** - Open source license used (such as MIT, Apache 2.0, GPL, etc.)
3. **Author Information** - Author name and contact information
4. **Citation Information** - If the plugin is based on academic papers, include citation format (optional)
5. **Patent Statement** - Algorithms involving patent protection (optional)

### Complete Example

```cpp
Interface::CopyrightData Copyright() const override {
    return {
        "MyResearchLab",  // Concise identifier for dependency graph display

        // Multi-line detailed information
        "Copyright (c) 2025 MyResearchLab, University of Example\n"
        "License: BSD 3-Clause License\n"
        "\n"
        "Author: Dr. Example Researcher\n"
        "Email: researcher@example.edu\n"
        "\n"
        "Citation:\n"
        "  E. Researcher et al., \"Novel Algorithm for Feature Extraction,\"\n"
        "  IEEE Conference on Computer Vision, 2025.\n"
        "\n"
        "Patent: US 10,123,456 (Method and System for...)"
    };
}
```

(third-party-copyright-important)=
### Copyright Declaration When Using Third-Party Libraries Important

**Current Version Limitation**: The system currently **only supports each Method returning one copyright information pair** (one identifier + one information string).

If a plugin uses **multiple third-party libraries**, you need to list all dependency library copyright information in **the same copyright information string**:

#### Recommended Format (Multi-Dependency Library Copyright Template)

```cpp
Interface::CopyrightData Copyright() const override {
    return {
        "YourOrganization",  // Main copyright identifier (use your organization name)

        // ===== Main Copyright Information =====
        "Copyright (c) 2025 YourOrganization Ltd.\n"
        "License: MIT License\n"
        "Author: Your Name\n"
        "\n"
        // ===== Third-Party Library Copyright Declarations =====
        "This plugin uses the following third-party libraries:\n"
        "\n"
        "[1] OpenCV 4.x\n"
        "    Copyright (c) 2000-2025, Intel Corporation\n"
        "    Copyright (c) 2009-2011, Willow Garage Inc.\n"
        "    License: Apache License 2.0\n"
        "    URL: https://opencv.org\n"
        "\n"
        "[2] Eigen 3.x\n"
        "    Copyright (c) Eigen Authors\n"
        "    License: MPL2 (Mozilla Public License 2.0)\n"
        "    URL: https://eigen.tuxfamily.org\n"
        "\n"
        "[3] FLANN 1.9.x\n"
        "    Copyright (c) 2008-2011, Marius Muja and David G. Lowe\n"
        "    License: BSD License\n"
        "    URL: https://github.com/mariusmuja/flann"
    };
}
```

#### Simplified Format (For Fewer Dependency Libraries)

```cpp
Interface::CopyrightData Copyright() const override {
    return {
        "MyCompany",

        "Copyright (c) 2025 MyCompany Ltd. | License: Commercial\n"
        "\n"
        "Third-party dependencies:\n"
        "- OpenCV (Apache 2.0): Copyright (c) OpenCV contributors\n"
        "- Eigen (MPL2): Copyright (c) Eigen Authors\n"
        "- Ceres (BSD): Copyright (c) Ceres Solver Team"
    };
}
```

#### Important Notes

1. **Copyright Identifier Selection**:
   - Use **your organization name** as the main copyright identifier (first parameter)
   - Do not use third-party library names (e.g., "OpenCV"), as this will mislead the dependency graph

2. **Completeness Requirements**:
   - **Must list all directly used third-party libraries**
   - Include library copyright statements and license types
   - Recommended to include official library URLs

3. **Clear Format**:
   - Use separators or numbering to distinguish different libraries
   - Keep format clean and readable

4. **License Compatibility**:
   - Ensure licenses used are compatible with dependency library licenses
   - For example: GPL libraries cannot be used in MIT or commercially licensed plugins

## Application Scenarios

### Academic Research

- Automatically generate citation lists for paper writing
- Track algorithm sources and versions used
- Ensure compliant academic citations

### Commercial Applications

- Copyright compliance auditing to meet legal requirements
- Open source license management and compatibility checking
- Third-party library usage tracking

### Open Source Projects

- Contributor attribution and credit display
- Automatic license compatibility checking
- Community transparency and trust building

## Best Practices

### Recommended Practices

1. **Concise Identifiers**: Use organization or project names as copyright identifiers (e.g., "OpenCV", "PoSDK")
2. **Complete Information**: Include copyright statements, license types, author information
3. **Third-Party Declarations**: Clearly list third-party libraries used and their licenses
4. **Academic Citations**: If based on papers, provide standard citation format

### Practices to Avoid

1. **Overly Long Text**: Avoid including large code blocks or documentation in copyright information
2. **Hiding Copyright**: Do not return empty strings or "Unknown"
3. **Incorrect Information**: Ensure copyright information is accurate and truthful
4. **Missing License**: Must clearly specify the license type used

## Frequently Asked Questions

### Q: What if I don't want to display copyright information?

A: **All plugins must provide copyright information**. This is PoSDK's design principle to protect all developers' intellectual property. If the plugin is open source, you can choose permissive licenses such as MIT or Apache.

### Q: Will copyright information affect performance?

A: No. Copyright information is displayed only once when the method is **first created**, and subsequent calls will not repeat the display or affect performance.

### Q: Can I modify other people's copyright information?

A: **No**. Each plugin's copyright information is defined by the `Copyright()` function in its source code, and only the plugin author can modify it. Copyright information in compiled plugins cannot be changed externally.

### Q: My plugin uses multiple third-party libraries. Do I need to list them all?

A: **Yes, you must list all directly used third-party libraries and their licenses**. This is best practice for open source compliance and is required by laws and regulations.

**Note**: The current system only supports returning one copyright information pair, so you need to list all dependency libraries in the same string. Please refer to the template in the "[Copyright Declaration When Using Third-Party Libraries](third-party-copyright-important)" section above.

### Q: Why can't I return independent copyright information for each third-party library?

A: This is a technical limitation of the current version. The `Copyright()` function can only return one `std::pair<std::string, std::string>` (copyright identifier + copyright information).

**Future Improvement Plan**: We are considering supporting multiple copyright information pairs in future versions, where each dependency library can have independent copyright identifiers and information.

**Current Solution**: Use clear formatting (such as numbering, separators) in the copyright information string to distinguish different libraries, ensuring information is complete and readable.

### Q: What happens if I forget to list a third-party library's copyright information?

A: This may lead to:
- **Legal Risks**: Violating open source license requirements (e.g., Apache 2.0, GPL all require retaining copyright statements)
- **Compliance Issues**: Commercial applications may fail copyright audits
- **Community Trust**: Reduces the open source community's trust in your plugin

**Recommendation**: Maintain a dependency library list during development and verify each one when writing copyright information.

---

**Copyright tracking ensures that every contribution in the PoSDK ecosystem is recorded and respected, protecting developers' intellectual property while ensuring users understand the sources and licensing requirements of the algorithms used.**
