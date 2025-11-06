# OpenGV约定

OpenGV库在其 `experiment_helpers` 中提供了一些工具函数，可以生成带有真值的相机位姿、3D点以及对应的2D观测（bearing vectors）。理解这些仿真数据中的位姿约定对于将其正确导入PoSDK进行测试和评估至关重要。

## 1. OpenGV仿真中的位姿约定

### 1.1 全局位姿 (相机位姿)

在OpenGV的仿真函数如 `generateRandom2D3DCorrespondences` 或 `generateRandom2D2DCorrespondences` 中，通常使用 `position` 和 `rotation` 参数来定义一个相机（或相机系统的参考载具）相对于世界坐标系的位姿：

-   `position` (`translation_t`): 表示相机（或载具）坐标系原点在**世界坐标系中的位置**，记为 $\mathbf{t}_w$ (world to body/camera_rig)
-   `rotation` (`rotation_t`): 表示从相机（或载具）坐标系到**世界坐标系的旋转矩阵**，记为 $R_b^w$ (body/camera_rig to world)

-   **含义**: 世界坐标系中的一个3D特征点 $\mathbf{X}_w$ 转换到相机（或载具）归一化图像坐标的点 $\mathbf{x}_i$ 的公式为

$$R_i^w \cdot \mathbf{x}_i \sim \mathbf{X}_w - \mathbf{t}_w$$

-   **说明**:
    -   $R_i^w$ 是 $(R_w^i)^T$ (从世界到载具的旋转)
    -   这符合 **PoseFormat::RwTw** 的位姿表示格式
    -   如果存在多相机系统（`camOffsets`, `camRotations`），则上述 $\mathbf{X}_b$ 会进一步通过这些相机相对于载具的内外参转换到具体某个相机的坐标系下得到观测

### 1.2 相对位姿

OpenGV的 `extractRelativePose(pos1, rot1, pos2, rot2, relPos, relRot)` 函数用于从两个绝对位姿计算它们之间的相对位姿:   
在双视图$(i,j)$中，相机$i$的观测 $\mathbf{x}_{i}$ 转换到相机$j$坐标系下的对应点 $\mathbf{x}_{j}$ 的公式为：
$$x_{i} \sim R_{ji} \cdot x_{j} + \mathbf{t}_{ji} \triangleq R\cdot x_{j} + \mathbf{t} $$
-   **说明**:
    -   相机$i$的绝对位姿为 $(R_{i}^w,\mathbf{t}_w^i)$
    -   相机$j$的绝对位姿为 $(R_{j}^w,\mathbf{t}_w^j)$
  

## 2. Convert `GlobalPoses` to `RelativePose`

OpenGV中利用绝对位姿计算的相对位姿 $(R_{ij}, \mathbf{t}_{ij})$ 计算如下：

1.  **相对旋转 $R ( = R_{ji})$**:

$$R = ({R_i^w})^T \cdot R_j^w$$

2.  **相对平移 $\mathbf{t} ( = \mathbf{t}_{ji})$**:

 $$\mathbf{t} = (R_i^w)^T \cdot (\mathbf{t}_j - \mathbf{t}_i)$$

```{important} PoSDK的标准位姿约定
>
> 为清晰起见，我们单独在 [PoSDK标准位姿约定](./posdk.md) 中详细描述PoSDK的定义。
>
>
``` 

## 3. 从OpenGV仿真数据到PoSDK的转换

### 3.1 全局位姿转换

假设从OpenGV仿真中获得了单个相机的全局位姿 `position_opengv` $(\mathbf{t}_w)$ 和 `rotation_opengv` $(R_b^w)$:

-   PoSDK全局位姿初始化为 `PoseFormat::RwTw` 格式，即 $\mathbf{x}_c \sim R_w^c \cdot (\mathbf{X}_w - \mathbf{t}_w)$
-   OpenGV的 全局位姿初始化为 `PoseFormat::RcTw` 格式，即 $\mathbf{x}_c \sim (R_c^w)^T \cdot (\mathbf{X}_w - \mathbf{t}_w)$

-   **转换**:
    -   $R_{po} = (R_{opengv})^T$
    -   $\mathbf{t}_{po} = \mathbf{t}_{opengv}$

### 3.2 相对位姿转换

假设从OpenGV `extractRelativePose` 获得了相对位姿$(R_{opengv}, \mathbf{t}_{opengv})$:

-   PoSDK的 `RelativePose` 定义是 $\mathbf{x}_{j} \sim R_{po} \cdot \mathbf{x}_{i} + \mathbf{t}$，其中:
-  OpenGV的相对位姿定义是 $\mathbf{x}_{i} \sim R_{opengv} \cdot \mathbf{x}_{j} + \mathbf{t}_{opengv}$，其中:

-   **转换**:
    -   $R_{po} = (R_{opengv})^T$
    -   $\mathbf{t}_{po} = -(R_{opengv})^T \cdot\mathbf{t}_{opengv}$


