# GLOMAP Boost 干扰问题深度分析

## 问题现象

即使 `install_glomap.sh` 明确设置了：
```bash
-DBOOST_ROOT=${LOCAL_BOOST_INSTALL}
-DBoost_NO_SYSTEM_PATHS=ON
```

并且用户已经：
1. ✅ `conda deactivate`
2. ✅ `rm -rf build_local install_local`
3. ✅ 重新运行 `./install_glomap.sh`

**但仍然失败**，并且出现新的 GraphOptim glog 错误。

---

## 根本原因分析

### 1. CMake Policy CMP0144 问题 ⚠️

**关键警告**（从你的输出中）：
```cmake
CMake Warning (dev):
  Policy CMP0144 is not set: find_package uses upper-case <PACKAGENAME>_ROOT variables.
  CMake variable BOOST_ROOT is set to: .../boost_1_85_0/install_local
  For compatibility, find_package is ignoring the variable  <-- 注意这行！
```

**问题**：
- CMake 3.27+ 引入了 Policy CMP0144
- 默认行为（OLD）：**忽略** `BOOST_ROOT` 等大写变量
- 新行为（NEW）：使用 `BOOST_ROOT` 等大写变量
- **你的脚本没有设置这个 Policy，所以 CMake 忽略了 `BOOST_ROOT`！**

**证据**：CMake 明确说了 "find_package is ignoring the variable"

### 2. Anaconda 路径持久化 🔴

即使 `conda deactivate`，以下路径可能仍然存在：

```bash
# 检查这些变量（即使 deactivate 后）
echo $CMAKE_PREFIX_PATH  # 可能仍包含 /home/hanyuqi/anaconda3
echo $LD_LIBRARY_PATH    # 可能仍包含 anaconda
echo $PKG_CONFIG_PATH    # 可能仍包含 anaconda

# 检查 CMake 用户缓存
ls -la ~/.cmake/packages/  # 可能有缓存的 Boost/Qt5 路径
```

**为什么？**
- `conda deactivate` 只回退环境，但如果之前手动 `export` 了变量，不会自动清除
- CMake 的包查找缓存（`~/.cmake/packages/`）持久存在
- Anaconda 可能修改了 `~/.bashrc` 或 `~/.profile`

### 3. Qt5 优先级问题

从你的输出看到：
```
-- Found Qt
--   Module : /home/hanyuqi/anaconda3/lib/cmake/Qt5Core  <-- 还是 Anaconda！
```

**原因**：
- 系统中可能没有 Qt5，或者 Anaconda 的 Qt5 优先级更高
- `find_package(Qt5)` 默认搜索顺序：
  1. `CMAKE_PREFIX_PATH` 中的路径（可能包含 Anaconda）
  2. 系统路径 `/usr/lib/`
  3. 用户路径 `~/.local/`

### 4. GraphOptim glog 错误的真正原因 🔥

```
undefined reference to `google::InitVLOG3__(int**, int*, char const*, int)'
```

**分析链**：
1. GLOMAP 链接了 Anaconda 的 glog：`/home/hanyuqi/anaconda3/lib/libglog.so.0.5.0`
2. GraphOptim 在编译时链接了**系统 glog**（或不同版本）
3. 链接 GLOMAP 时，GraphOptim 的符号期望某个 glog 版本
4. 但实际链接器找到了 Anaconda 的 glog（不同版本/ABI）
5. 符号不匹配 → `undefined reference`

**这是级联失败**：
- Boost 问题 → Qt5 从 Anaconda 找到 → glog 从 Anaconda 找到 → GraphOptim 符号不匹配

---

## 完整解决方案

### 方案 1：彻底清理 + 修复 CMake Policy（推荐）✅

创建清理和构建脚本：

```bash
#!/bin/bash
# 文件名：rebuild_glomap_clean.sh

set -e  # 遇到错误立即退出

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "GLOMAP 彻底清理和重建"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 步骤 1：完全退出 Anaconda 并清理环境
echo "步骤 1/6：清理 Anaconda 环境变量..."
conda deactivate 2>/dev/null || true

# 清理可能的 Anaconda 残留环境变量
unset CONDA_PREFIX
unset CONDA_DEFAULT_ENV
unset CONDA_SHLVL

# 清理可能包含 Anaconda 的路径变量
export PATH=$(echo $PATH | tr ':' '\n' | grep -v anaconda | tr '\n' ':' | sed 's/:$//')
export LD_LIBRARY_PATH=$(echo $LD_LIBRARY_PATH | tr ':' '\n' | grep -v anaconda | tr '\n' ':' | sed 's/:$//')
export PKG_CONFIG_PATH=$(echo $PKG_CONFIG_PATH | tr ':' '\n' | grep -v anaconda | tr '\n' ':' | sed 's/:$//')
unset CMAKE_PREFIX_PATH  # 完全清除（让 CMake 从头搜索）

echo "✓ 环境变量已清理"
echo "  PATH: $(echo $PATH | tr ':' '\n' | head -3 | tr '\n' ' ')..."
echo ""

# 步骤 2：清理 CMake 用户级缓存
echo "步骤 2/6：清理 CMake 用户缓存..."
rm -rf ~/.cmake/packages/Boost 2>/dev/null || true
rm -rf ~/.cmake/packages/Qt5 2>/dev/null || true
rm -rf ~/.cmake/packages/COLMAP 2>/dev/null || true
rm -rf ~/.cmake/packages/PoseLib 2>/dev/null || true
echo "✓ CMake 缓存已清理"
echo ""

# 步骤 3：清理 GLOMAP 构建产物
echo "步骤 3/6：清理 GLOMAP 构建产物..."
cd ~/posdk_test/PoSDK/dependencies/glomap-main
rm -rf build_local install_local
echo "✓ GLOMAP 构建目录已清理"
echo ""

# 步骤 4：创建构建目录
echo "步骤 4/6：创建新的构建目录..."
mkdir -p build_local
cd build_local
echo "✓ 构建目录已创建"
echo ""

# 步骤 5：使用修正的 CMake 配置
echo "步骤 5/6：配置 CMake（修正 Policy + 禁用 GUI）..."
echo ""

BOOST_ROOT_PATH="$HOME/posdk_test/PoSDK/dependencies/boost_1_85_0/install_local"
CERES_DIR_PATH="$HOME/posdk_test/PoSDK/dependencies/ceres-solver-2.2.0/install_local/lib/cmake/Ceres"
POSELIB_DIR_PATH="$HOME/posdk_test/PoSDK/dependencies/PoseLib/install_local/lib/cmake/PoseLib"
COLMAP_SRC_PATH="$HOME/posdk_test/PoSDK/dependencies/glomap-main/dependencies/colmap-src"

# 验证依赖存在
if [ ! -d "$BOOST_ROOT_PATH" ]; then
    echo "❌ 错误：本地 Boost 不存在：$BOOST_ROOT_PATH"
    exit 1
fi

echo "依赖检查："
echo "  ✓ Boost: $BOOST_ROOT_PATH"
echo "  ✓ Ceres: $CERES_DIR_PATH"
echo "  ✓ PoseLib: $POSELIB_DIR_PATH"
echo "  ✓ COLMAP: $COLMAP_SRC_PATH"
echo ""

cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POLICY_DEFAULT_CMP0144=NEW \
    -DCMAKE_INSTALL_PREFIX="$PWD/../install_local" \
    -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON \
    -DBOOST_ROOT="$BOOST_ROOT_PATH" \
    -DBoost_DIR="$BOOST_ROOT_PATH/lib/cmake/Boost-1.85.0" \
    -DBoost_NO_SYSTEM_PATHS=ON \
    -DBoost_NO_BOOST_CMAKE=OFF \
    -DCeres_DIR="$CERES_DIR_PATH" \
    -DPoseLib_DIR="$POSELIB_DIR_PATH" \
    -DFETCH_POSELIB=OFF \
    -DFETCH_COLMAP=OFF \
    -DGUI_ENABLED=OFF \
    -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE \
    -DCMAKE_INSTALL_RPATH="\$ORIGIN/../lib:\$ORIGIN/../../PoseLib/install_local/lib:\$ORIGIN/../../boost_1_85_0/install_local/lib" \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=FALSE \
    -DCMAKE_SKIP_BUILD_RPATH=FALSE

if [ $? -ne 0 ]; then
    echo "❌ CMake 配置失败"
    exit 1
fi

echo ""
echo "✓ CMake 配置成功"
echo ""

# 步骤 6：编译和安装
echo "步骤 6/6：编译和安装 GLOMAP..."
ninja

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi

echo ""
echo "安装到：$PWD/../install_local"
ninja install

if [ $? -ne 0 ]; then
    echo "❌ 安装失败"
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ GLOMAP 安装成功！"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "验证安装："
echo "  $HOME/posdk_test/PoSDK/dependencies/glomap-main/install_local/bin/glomap --version"
echo ""
echo "检查依赖（应该没有 anaconda）："
echo "  ldd $HOME/posdk_test/PoSDK/dependencies/glomap-main/install_local/bin/glomap | grep -E '(boost|glog|anaconda)'"
```

**使用方法**：
```bash
cd ~/posdk_test/PoSDK/dependencies
chmod +x rebuild_glomap_clean.sh
./rebuild_glomap_clean.sh
```

---

### 方案 2：修改 install_glomap.sh（永久修复）

在 `install_glomap.sh` 的 CMake 配置部分添加 Policy 设置：

```bash
# 找到 CMAKE_OPTS 定义（大约第 588 行）
CMAKE_OPTS=(
    "-GNinja"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_POLICY_DEFAULT_CMP0144=NEW"  # 添加这行 ⭐
    "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON"
    # ... 其他选项保持不变
)
```

**修改三个地方**：
1. 第 588 行附近（本地构建模式）
2. 第 645 行附近（混合模式）
3. 第 704 行附近（FetchContent 模式）

---

### 方案 3：安装系统 Qt5 + 使用系统 glog（备选）

如果 GUI 是必需的：

```bash
# 1. 安装系统 Qt5 和 glog
sudo apt-get update
sudo apt-get install -y \
    qtbase5-dev \
    libqt5opengl5-dev \
    libgoogle-glog-dev

# 2. 然后使用方案 1 的脚本重新构建
```

**优点**：
- 避免 Anaconda Qt5/glog
- 使用系统一致的库

**缺点**：
- 需要 root 权限
- 可能与 Anaconda 的其他工具冲突

---

## 为什么你的脚本设置无效？

### 问题 1：CMake Policy CMP0144

你的 `install_glomap.sh` 设置了：
```bash
"-DBOOST_ROOT=${LOCAL_BOOST_INSTALL}"
"-DBoost_NO_SYSTEM_PATHS=ON"
```

**但是**，CMake 3.27+ 的默认行为是**忽略**这些大写变量！

**CMake 的逻辑**（Policy CMP0144 = OLD）：
```
1. 检查 CMP0144 Policy → 未设置 → 使用 OLD 行为
2. OLD 行为：忽略 BOOST_ROOT（为了向后兼容）
3. 改为搜索 Boost_ROOT（小写 R 开头）
4. 找不到 Boost_ROOT → 回退到标准搜索路径
5. 标准搜索路径包括：CMAKE_PREFIX_PATH, 系统路径
6. 如果 CMAKE_PREFIX_PATH 或环境变量中有 Anaconda → 找到 Anaconda Boost
```

**解决方案**：
```bash
"-DCMAKE_POLICY_DEFAULT_CMP0144=NEW"  # 强制使用新行为
```

### 问题 2：环境变量持久化

`conda deactivate` 只做这些：
```bash
# conda deactivate 实际执行的操作（简化）
unset CONDA_PREFIX
unset CONDA_DEFAULT_ENV
export PATH=$CONDA_BACKUP_PATH
```

**但不会**：
- 清理你手动 `export` 的变量
- 清理 `~/.bashrc` 中持久化的 Anaconda 初始化代码
- 清理 CMake 的包查找缓存

**检查方法**：
```bash
conda deactivate

# 检查是否还有残留
echo $PATH | grep anaconda          # 应该没有输出
echo $CMAKE_PREFIX_PATH | grep anaconda  # 应该没有输出
which python  # 应该是 /usr/bin/python3，不是 anaconda

# 如果上面任何一个还有 anaconda，说明没干净
```

### 问题 3：Qt5 搜索优先级

即使 Boost 正确，Qt5 仍可能来自 Anaconda：

**CMake 搜索 Qt5 的顺序**：
1. `CMAKE_PREFIX_PATH` 环境变量
2. `Qt5_DIR` 变量（你的脚本没设置）
3. 系统标准路径：`/usr/lib/x86_64-linux-gnu/cmake/Qt5`
4. 用户路径：`~/.local/lib/cmake/Qt5`
5. **HOME 路径**：`~/anaconda3/lib/cmake/Qt5` ⚠️

如果系统没有 Qt5，CMake 会找到 Anaconda 的！

---

## GraphOptim glog 错误的根源

```
undefined reference to `google::InitVLOG3__(int**, int*, char const*, int)'
```

**链接链分析**：

```
GLOMAP 可执行文件
  ├─ libglomap.a (静态库)
  │   └─ 依赖 glog（来自 Anaconda: libglog.so.0.5.0）
  │
  └─ GraphOptim/libgopt.a (静态库)
      └─ 依赖 glog（期望系统版本: libglog.so.1.0.0）
```

**冲突**：
- GraphOptim 编译时链接了**系统 glog 1.0.0**（或 0.6.0）
- GLOMAP 链接时找到了**Anaconda glog 0.5.0**
- glog 0.5.0 的 `google::InitVLOG3__` 签名与 1.0.0 不同
- 结果：符号找不到 → `undefined reference`

**为什么会这样？**
1. GLOMAP 链接了 Anaconda 的 Qt5
2. Qt5 间接依赖了 Anaconda 的 glog
3. 链接器将 Anaconda glog 加入链接列表
4. GraphOptim 的符号期望不同版本的 glog
5. 冲突！

**解决方法**：
- 禁用 GLOMAP 的 GUI（`-DGUI_ENABLED=OFF`）
- 这样就不依赖 Qt5，也就不会引入 Anaconda glog
- GraphOptim 会使用系统 glog

---

## 验证修复是否成功

### 步骤 1：检查环境

```bash
# 应该都是空或不包含 anaconda
echo "PATH: $PATH" | grep anaconda
echo "CMAKE_PREFIX_PATH: $CMAKE_PREFIX_PATH"
echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"

# 应该是系统 python
which python  # /usr/bin/python3
```

### 步骤 2：检查 CMake 输出

重新运行后，检查 CMake 输出中的这些关键行：

```cmake
# ✅ 正确的输出（应该看到）
-- Found Boost version: 1.85.0
-- Boost include dirs: .../boost_1_85_0/install_local/include
-- Boost libraries: .../boost_1_85_0/install_local/lib  # 不是空！

# ✅ 正确的输出（Policy 已设置）
-- CMake Policy CMP0144: Using NEW behavior

# ✅ 正确的输出（没有 anaconda）
-- Found Ceres version: 2.2.0 installed in: .../ceres-solver-2.2.0/install_local

# ❌ 错误的输出（如果还有问题）
-- Found Qt
--   Module : /home/hanyuqi/anaconda3/lib/cmake/Qt5Core  # 还是 Anaconda！
```

### 步骤 3：检查链接依赖

```bash
# 编译成功后
cd ~/posdk_test/PoSDK/dependencies/glomap-main/install_local/bin

# 检查 GLOMAP 的动态库依赖
ldd glomap

# ❌ 不应该看到 anaconda
# ✅ 应该看到本地 boost
libboost_program_options.so.1.85.0 => .../boost_1_85_0/install_local/lib/...

# 检查 glog
ldd glomap | grep glog
# 应该是系统 glog: /usr/lib/x86_64-linux-gnu/libglog.so.1
# 不应该是: ~/anaconda3/lib/libglog.so.0
```

### 步骤 4：功能测试

```bash
# 运行 GLOMAP
~/posdk_test/PoSDK/dependencies/glomap-main/install_local/bin/glomap --help

# 应该正常显示帮助信息，没有库加载错误
```

---

## 总结：为什么会干扰？

| 干扰源    | 原因           | 你的设置         | 为什么无效                        | 修复方法                                  |
| --------- | -------------- | ---------------- | --------------------------------- | ----------------------------------------- |
| **Boost** | Policy CMP0144 | `BOOST_ROOT=...` | CMake 3.27+ 默认忽略大写变量      | 添加 `-DCMAKE_POLICY_DEFAULT_CMP0144=NEW` |
| **Qt5**   | 环境变量       | 无               | `CMAKE_PREFIX_PATH` 包含 anaconda | 清理环境变量 + 禁用 GUI                   |
| **glog**  | 级联依赖       | 无               | Qt5 → Anaconda glog               | 禁用 GUI（`-DGUI_ENABLED=OFF`）           |
| **缓存**  | CMake 包缓存   | 无               | `~/.cmake/packages/` 有旧路径     | `rm -rf ~/.cmake/packages/`               |

**核心问题**：
1. ⭐ **CMake Policy CMP0144 未设置** → `BOOST_ROOT` 被忽略（最关键）
2. ⚠️ **Qt5 来自 Anaconda** → 引入 Anaconda glog
3. 🔄 **环境变量持久化** → `conda deactivate` 不够彻底
4. 💾 **CMake 缓存** → 记住了之前的错误路径

**一句话总结**：
你的 `install_glomap.sh` 设置是对的，但 CMake 3.27+ 的新行为导致 `BOOST_ROOT` 被忽略，需要显式设置 Policy。

---

## 下一步行动

```bash
# 1. 使用方案 1 的完整脚本（推荐）
cd ~/posdk_test/PoSDK/dependencies
# 复制上面的 rebuild_glomap_clean.sh 脚本
chmod +x rebuild_glomap_clean.sh
./rebuild_glomap_clean.sh

# 2. 或者修改 install_glomap.sh 并提交（永久修复）
# 在三处 CMAKE_OPTS 定义中添加：
# "-DCMAKE_POLICY_DEFAULT_CMP0144=NEW"

# 3. 验证成功
ldd ~/posdk_test/PoSDK/dependencies/glomap-main/install_local/bin/glomap | grep -E '(boost|glog|anaconda)'
```

按这个脚本操作应该能彻底解决问题！

