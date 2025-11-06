# 平台对比测试结果

本文档展示了 PoSDK 与当前的主流三维重建平台在标准数据集上的性能和精度对比测试结果。


```{figure} ../_static/pipeline.png
:alt: 纯位姿平台与其他平台对比框架图
:align: center
:width: 90%

纯位姿平台与其他平台对比框架图
```

## 对比平台介绍

### PoSDK
- **项目主页**: [PoSDK Documentation](https://posdk.readthedocs.io/)
- **仓库地址**: [https://github.com/pose-only-vision/PoSDK](https://github.com/pose-only-vision/PoSDK)
- **技术特点**: 基于纯位姿成像几何理论的高效位姿估计平台
- **核心算法**: GlobalSfM管道，支持多种预处理器和算法对比
- **配置详情**: 详见 [GlobalSfM管道插件配置](../basic_development/plugin_list.md#globalsfm-pipeline-posdk-globalsfm管道-⭐)
- **开发团队**: [上海交通大学 VINF 课题组](https://isn.sjtu.edu.cn/web/personal-page/yxwu)
- **许可证**: cc-by-sa-4.0 (Creative Commons Attribution Share Alike 4.0 International)

```{tip}
**PoSDK运行参数配置**

PoSDK通过 `globalsfm_pipeline` 插件执行对比测试，主要参数：

- **预处理类型**: `preprocess_type=posdk` （也支持openmvg、opencv）
- **评估模式**: `evaluation_print_mode=comparison`
- **对比算法**: `compared_pipelines=openmvg,COLMAP,GLOMAP`
- **性能分析**: `enable_profiling=true`

完整参数说明请参考 [插件配置文档](../basic_development/plugin_list.md#基础配置参数)。
```

### OpenMVG (Open Multiple View Geometry)
- **项目主页**: [https://github.com/openMVG/openMVG](https://github.com/openMVG/openMVG)
- **仓库地址**: [https://github.com/openMVG/openMVG](https://github.com/openMVG/openMVG)
- **技术特点**:
  - 开源的多视图几何库
  - 提供完整的 SfM (Structure from Motion) 解决方案
  - 支持增量式和全局式两种重建方法
- **开发团队**: Pierre Moulon 及开源社区贡献者
- **许可证**: MPL2 (Mozilla Public License 2.0)

### COLMAP
- **项目主页**: [https://COLMAP.github.io/](https://COLMAP.github.io/)
- **仓库地址**: [https://github.com/COLMAP/COLMAP](https://github.com/COLMAP/COLMAP)
- **技术特点**:
  - 业界领先的三维重建系统
  - 支持增量式 SfM、稠密重建和 MVS
  - 提供图形化界面和命令行工具
  - 高精度位姿估计和点云重建
- **开发团队**: Johannes Schönberger, ETH 团队
- **许可证**: BSD License

### GLOMAP (Global Mapping)
- **项目主页**: [https://github.com/COLMAP/GLOMAP](https://github.com/COLMAP/GLOMAP)
- **仓库地址**: [https://github.com/COLMAP/GLOMAP](https://github.com/COLMAP/GLOMAP)
- **技术特点**:
  - 新一代全局 SfM 系统
  - 专注于大规模场景快速重建
  - 基于全局优化的位姿估计方法
  - 与 COLMAP 兼容的数据格式
- **开发团队**: ETH 团队
- **许可证**: BSD License

## 测试环境

- **操作系统**: Ubuntu 24.04 LTS
- **处理器**: Intel/AMD x86_64
- **测试数据集**: Strecha 标准数据集
  - fountain-P11 (11 张图像)
  - castle-P19/P30 (19/30 张图像)
  - entry-P10 (10 张图像)
  - Herz-Jesus-P8/P25 (8/25 张图像)

---

## 稀疏点云展示

以下展示了不同平台在 Strecha (castle-P30) 数据集上的稀疏点云重建结果对比：

```{raw} html
<table style="width: 100%; border-collapse: collapse;">
  <tr>
    <td style="width: 50%; text-align: center; padding: 10px; vertical-align: top;">
      <img src="../_static/COLMAP.gif" alt="COLMAP 稀疏点云重建结果" style="width: 100%; max-width: 500px;">
      <p><strong>COLMAP</strong></p>
    </td>
    <td style="width: 50%; text-align: center; padding: 10px; vertical-align: top;">
      <img src="../_static/GLOMAP.gif" alt="GLOMAP 稀疏点云重建结果" style="width: 100%; max-width: 500px;">
      <p><strong>GLOMAP</strong></p>
    </td>
  </tr>
  <tr>
    <td style="width: 50%; text-align: center; padding: 10px; vertical-align: top;">
      <img src="../_static/openmvg.gif" alt="OpenMVG 稀疏点云重建结果" style="width: 100%; max-width: 500px;">
      <p><strong>OpenMVG</strong></p>
    </td>
    <td style="width: 50%; text-align: center; padding: 10px; vertical-align: top;">
      <img src="../_static/ours.gif" alt="PoSDK 稀疏点云重建结果" style="width: 100%; max-width: 500px;">
      <p><strong>PoSDK (Ours)</strong></p>
    </td>
  </tr>
</table>
```

**说明**：
- 所有点云均在同一数据集下进行对比展示
- 点云颜色表示特征点的 RGB 信息
- GIF 动图展示点云的旋转视角，便于观察重建质量


```{note}
表格数据来源于平台测试结果。表格中：
- <span style="color: #d32f2f; font-weight: bold;">红色加粗</span>：同一数据集下的最优结果
- **黑色加粗**：同一数据集下的次优结果
```

## 总运行时间对比

下表展示了不同数据集上的总运行时间（单位：毫秒）：

```{raw} html
:file: processed/performance_comparison.html
```
---

## 精度对比结果

### 1. 全局位姿旋转误差

全局位姿旋转误差统计（单位：度）：

```{raw} html
:file: processed/global_rotation_error.html
```


### 2. 全局位姿平移误差

全局位姿平移误差统计（单位：归一化距离）：

```{raw} html
:file: processed/global_translation_error.html
```


### 3. 相对位姿旋转误差

相对位姿旋转误差统计（单位：度）：

```{raw} html
:file: processed/relative_rotation_error.html
```



---

## 数据集详细信息

### Strecha 数据集

Strecha 数据集是由瑞士联邦理工学院洛桑分校（EPFL）提供的经典多视图立体视觉（MVS）评估数据集，包含：

- **fountain**: 喷泉场景，包含复杂的几何结构和丰富的纹理
- **castle**: 城堡场景，大视角跨度，具有挑战性的遮挡区域
- **entry**: 入口场景，中等规模，建筑物细节丰富
- **Herz-Jesus**: 教堂场景，包含精细的建筑细节和光照变化

每个数据集提供：
- 高分辨率彩色图像
- 标定的相机内参矩阵
- 真值相机位姿（Ground Truth）
- 真值三维点云（用于精度评估）

---


**参考文献**：

1. Strecha, C., et al. "On Benchmarking Camera Calibration and Multi-View Stereo for High Resolution Imagery." CVPR 2008.
2. Moulon, P., et al. "OpenMVG: Open Multiple View Geometry." ICCV 2013 Workshop.
3. Cai, Q., et al. "A pose-only solution to visual reconstruction and navigation." TPAMI 2023.