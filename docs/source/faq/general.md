# Frequently Asked Questions (FAQ)

Below are common questions and answers when using PoSDK. If your question is not listed here, please refer to the detailed documentation or submit an issue on GitHub.

## Installation-Related Issues

### Q: How to resolve dependency installation errors?

A: Most dependency installation errors can be resolved through the following steps:
- Ensure your system meets the minimum version requirements
- Install dependencies using package managers (such as apt, brew)
- For specific version requirements, consider compiling from source

### Q: Can PoSDK be used on macOS/Windows?

A: The current version mainly supports Linux environments. We are working to provide full support for macOS and Windows. Please follow project updates.

## Plugin Development Issues

### Q: How to create custom data plugins?

A: Please refer to the "Data Plugins" section in the "Basic Development" chapter, which provides detailed development steps and example code.

### Q: Why is my plugin not recognized by the system?

A: Common causes include:
- Plugin registration macro not used correctly
- GetType() return value inconsistent with registered type string
- Plugin library not properly loaded into the system


---

We continuously update the FAQ list. If you have new questions or suggestions, please submit feedback.
