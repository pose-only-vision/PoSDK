# LightGlue Environment Setup Guide

## 问题诊断

从错误日志可以看到：
```
Missing dependencies: No module named 'numpy'
No environment configuration script found
```

这说明：
1. 系统Python环境缺少必要的依赖包
2. 脚本没有找到环境配置脚本

## 解决方案

### 方案1：快速解决（推荐）
直接安装必要的Python包：

```bash
# 使用pip安装
pip install torch torchvision numpy opencv-python

# 或使用conda安装
conda install pytorch torchvision numpy opencv -c pytorch
```

### 方案2：使用drawer环境
如果已经配置了drawer环境：

```bash
# 1. 进入drawer目录
cd po_core/drawer

# 2. 运行环境配置脚本
bash configure_drawer_env.sh

# 3. 验证环境
./conda_env/bin/python -c "import torch, numpy, cv2; print('Environment OK')"
```

### 方案3：手动配置LightGlue环境
```bash
# 1. 进入Img2Features目录
cd src/plugins/methods/Img2Features

# 2. 运行LightGlue环境配置
bash configure_lightglue_env.sh

# 3. 验证安装
./conda_env/bin/python -c "import torch, numpy, cv2; print('LightGlue Environment Ready')"
```

## 验证环境

运行以下命令验证环境配置成功：

```bash
# 基础包验证
python3 -c "import torch, numpy, cv2; print('Basic packages OK')"

# LightGlue验证
python3 -c "
import sys
sys.path.append('src/dependencies/LightGlue-main')
from lightglue import SuperPoint, LightGlue
print('LightGlue OK')
"
```

## 环境优先级

系统会按以下优先级查找Python环境：

1. **Drawer环境**: `/Users/caiqi/Documents/PoMVG/po_core/drawer/conda_env/bin/python`
2. **本地LightGlue环境**: `src/plugins/methods/Img2Features/conda_env/bin/python`  
3. **系统Python**: `python3` 或 `python`

## 常见问题

### Q: 为什么SuperPoint失败但仍显示特征提取成功？
A: SuperPoint失败时会自动降级到SIFT，所以仍能提取特征，但不是SuperPoint特征。

### Q: 如何确认使用的是SuperPoint而不是SIFT？
A: 查看日志中的描述子维度：
- SuperPoint: 256维描述子
- SIFT: 128维描述子

### Q: 如何避免每次都重新配置环境？
A: 配置一次drawer环境后，所有PoSDK组件都会复用该环境。

## 推荐配置

最简单的方式是配置drawer环境，它会被所有PoSDK组件共享：

```bash
cd /Users/caiqi/Documents/PoMVG/po_core/drawer
bash configure_drawer_env.sh
```

然后在drawer的requirements.txt中添加LightGlue依赖：
```txt
torch>=1.9.0
torchvision>=0.10.0
numpy>=1.21.0
opencv-python>=4.5.0
```
