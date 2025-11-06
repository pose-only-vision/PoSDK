# Minimal makefile for Sphinx documentation
# Sphinx文档的最小化makefile - 保持与原系统兼容并支持GitHub协作

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     ?= source
BUILDDIR      ?= build

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile benchmark-tables html-with-tables livehtml clean serve

# ==================== Original Custom Targets (保持原有功能) ====================

# Generate benchmark comparison tables
benchmark-tables:
	@echo "=================================================="
	@echo "Generating benchmark comparison tables..."
	@echo "生成基准测试对比表格..."
	@echo "=================================================="
	@if [ -f "$(SOURCEDIR)/benchmark_comparison/scripts/generate_tables.py" ]; then \
		python3 $(SOURCEDIR)/benchmark_comparison/scripts/generate_tables.py; \
		echo "Benchmark tables generated successfully!"; \
		echo "基准测试表格生成成功！"; \
	else \
		echo "Warning: Benchmark table generator not found, skipping..."; \
		echo "警告: 未找到基准测试表格生成器，跳过..."; \
	fi
	@echo ""

# Generate tables and build HTML documentation in one step
html-with-tables: benchmark-tables html
	@echo ""
	@echo "=================================================="
	@echo "Documentation built with updated tables!"
	@echo "文档已构建，表格已更新！"
	@echo "=================================================="
	@echo ""

# ==================== New Development Targets (新增开发用目标) ====================

# Live reload server for development
# 开发用的实时重载服务器
livehtml:
	sphinx-autobuild "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O) \
		--host 0.0.0.0 --port 8000 --open-browser

# Clean build directory
# 清理构建目录
clean:
	rm -rf "$(BUILDDIR)"

# Build and serve locally
# 本地构建并服务
serve: html
	@echo "Serving documentation at http://localhost:8000"
	@echo "在 http://localhost:8000 提供文档服务"
	cd $(BUILDDIR)/html && python -m http.server 8000

# ==================== GitHub Collaboration Targets (GitHub协作目标) ====================

# Quick build for GitHub Actions or CI
# 快速构建用于GitHub Actions或CI
ci-build: benchmark-tables html
	@echo "CI build completed successfully!"
	@echo "CI构建成功完成！"

# Check documentation for issues
# 检查文档问题
check:
	@echo "Checking documentation for issues..."
	@echo "检查文档问题..."
	@$(SPHINXBUILD) -b linkcheck "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

# ========================================================

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)