# PoSDK 精度评估体系

PoSDK 提供了完整的精度评估体系，用于衡量相对位姿、全局位姿和全局位移的估计精度。本章节详细介绍各种评估方法的数学原理、实现细节和使用方式。

## 评估体系概览

PoSDK 的精度评估体系包含以下三个主要组件：

### 1. [相对位姿评估](relative_poses.md)
- **评估对象**: 两个相机视图之间的相对位姿 $(R_{ij}, \mathbf{t}_{ij})$
- **主要指标**: 旋转误差、平移角度误差
- **适用场景**: 双视图几何、立体视觉、增量式SfM等

### 2. [全局位姿评估](global_poses.md)  
- **评估对象**: 相机在世界坐标系中的全局位姿 $(R_w^c, \mathbf{t}_w)$
- **主要指标**: 旋转误差、位置误差
- **评估方法**: 
  - 基于相似变换的完整对齐评估
  - 仅旋转部分的评估 (EvaluateRotations)
  - SVD方法的旋转评估 (EvaluateRotationsSVD)

### 3. [全局位移评估](global_translations.md)
- **评估对象**: 相机光心在世界坐标系中的位置 $\mathbf{t}_w$
- **主要指标**: 位置误差、方向误差
- **适用场景**: 全局SfM、位移估计验证等

## 评估流程

所有评估都遵循统一的流程：

1. **数据预处理**: 格式转换、坐标系对齐
2. **几何对齐**: 计算最优变换参数 (相似变换、刚体变换等)
3. **误差计算**: 基于对齐结果计算各项误差指标
4. **结果输出**: 通过 `EvaluatorStatus` 统一输出评估结果

## 数学基础

### 旋转误差计算
对于旋转矩阵 $R_{gt}$ (真值) 和 $R_{est}$ (估计值)，旋转误差定义为：

$$\text{Rotation Error} = \arccos\left(\frac{\text{trace}(R_{gt}^T R_{est}) - 1}{2}\right) \times \frac{180°}{\pi}$$

### 位置误差计算  
对于位置向量 $\mathbf{p}_{gt}$ (真值) 和 $\mathbf{p}_{est}$ (估计值)，位置误差定义为：

$$\text{Position Error} = ||\mathbf{p}_{gt} - \mathbf{p}_{est}||_2$$

### 相似变换对齐
为了消除尺度、旋转和平移的影响，PoSDK 使用7自由度相似变换进行对齐：

$$\mathbf{p}_{aligned} = s \cdot R_{align} \cdot \mathbf{p}_{est} + \mathbf{t}_{align}$$

其中 $s$ 是尺度因子，$R_{align}$ 是对齐旋转，$\mathbf{t}_{align}$ 是对齐平移。

## 使用示例

```cpp
#include <po_core/data/data_global_poses.hpp>

// 创建全局位姿数据对象
DataGlobalPoses estimated_poses;
DataGlobalPoses ground_truth_poses;

// 执行评估
EvaluatorStatus eval_result = estimated_poses.Evaluate(ground_truth_poses);

if (eval_result.is_successful) {
    // 获取旋转误差
    auto rotation_errors = eval_result.GetResults("rotation_error_deg");
    
    // 获取位置误差  
    auto position_errors = eval_result.GetResults("translation_error");
    
    // 打印统计信息
    std::cout << "平均旋转误差: " << eval_result.GetMeanError("rotation_error_deg") << "°" << std::endl;
    std::cout << "平均位置误差: " << eval_result.GetMeanError("translation_error") << std::endl;
}
```

## 注意事项

1. **坐标系一致性**: 确保真值和估计值使用相同的坐标系约定
2. **位姿格式**: PoSDK 默认使用 `PoseFormat::RwTw` 格式，评估前会自动进行格式转换
3. **数据对应关系**: 评估时会自动匹配对应的相机视图，未匹配的视图将被忽略
4. **数值稳定性**: 所有计算都包含数值稳定性检查，异常值会被记录并处理

```{toctree}
:maxdepth: 2
:caption: 评估方法详解

relative_poses
global_poses
global_translations
```

---

**参考文献**:
- Horn, B. K. P. (1987). Closed-form solution of absolute orientation using unit quaternions
- Umeyama, S. (1991). Least-squares estimation of transformation parameters between two point patterns
- Hartley, R., & Zisserman, A. (2003). Multiple view geometry in computer vision
