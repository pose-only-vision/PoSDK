# PoSDK 标准位姿约定

为了确保库内部操作的一致性和与外部数据交互时的清晰性，PoSDK 对相机位姿的表示遵循以下标准约定。

## 1. 全局相机位姿 (`types::GlobalPoses`)

PoSDK 中，单个相机的全局位姿（即相机相对于世界坐标系的姿态和位置）由旋转矩阵 $R$ 和平移向量 $\mathbf{t}$ 定义。默认情况下，PoSDK 采用 **`PoseFormat::RwTw`** 格式。

### PoseFormat::RwTw (默认)

-   **含义**: 世界坐标系中的一个3D特征点 $\mathbf{X}_w$ 转换到相机坐标系下的归一化图像特征点 $\mathbf{x}_c$ 的公式为

$$\mathbf{x}_c \sim R_w^c \cdot (\mathbf{X}_w - \mathbf{t}_w)$$

-   **说明**:
    -   $R_w^c$ (`types::GlobalPoses::rotations[i]`):
        -   一个 3x3 的旋转矩阵。
        -   表示从 **世界坐标系 (World)** 到 **相机坐标系 (Camera)** 的旋转变换。
    -   $\mathbf{t}_w$ (`types::GlobalPoses::translations[i]`):
        -   一个 3x1 的平移向量。
        -   表示 **相机光心在世界坐标系中的位置**。
        -   对于第$i$个相机，位姿将写成【$R_w^i, \mathbf{t}_w^i$】。

### PoseFormat::RwTc

PoSDK 也支持另一种常见的位姿表示格式 **`PoseFormat::RwTc`**，并提供了两者之间的转换函数。

-   **含义**: 世界坐标系中的一个3D特征点 $\mathbf{X}_w$ 转换到相机坐标系下的归一化图像特征点 $\mathbf{x}_c$ 的公式为

$$\mathbf{x}_c \sim R_w^c \cdot \mathbf{X}_w + \mathbf{t}_c$$

-   **说明**:
    -   $R_w^c$: 与 `RwTw` 中的定义相同，表示从世界到相机的旋转。
    -   $\mathbf{t}_c$: 一个 3x1 的平移向量，表示 **世界坐标系原点在相机坐标系下的坐标**。注意这与 `RwTw` 中的 $\mathbf{t}_w$ 不同。


-   **与RwTw的关系**: $\mathbf{t}_c = -R_w^c \cdot \mathbf{t}_w$

>> `types::GlobalPoses` 类提供了成员函数 `ConvertPoseFormat(target_format, ...)`，以及便捷函数 `RwTw_to_RwTc(...)` 和 `RwTc_to_RwTw(...)` 来在这两种格式之间进行转换。

## 2. 相对相机位姿 (`types::RelativePose`)

两个相机（视图 $i$ 和视图 $j$）之间的相对位姿由旋转矩阵 $R_{ij}$ 和平移向量 $\mathbf{t}_{ij}$ 定义。

-   **含义**: 一个在相机 $i$ 坐标系下归一化图像特征观测 $\mathbf{x}_i$ 转换到相机 $j$ 坐标系下的对应点 $\mathbf{x}_j$ 的公式为

$$\mathbf{x}_j \sim R_{ij} \cdot \mathbf{x}_i + \mathbf{t}_{ij}$$

-   **说明**:
    -   $R_{ij}$ (`types::RelativePose::Rij`):
        -   一个 3x3 的旋转矩阵。
        -   表示从 **相机 $i$ 坐标系** 到 **相机 $j$ 坐标系** 的旋转变换。
    -   $\mathbf{t}_{ij}$ (`types::RelativePose::tij`):
        -   一个 3x1 的平移向量。
        -   表示 **相机 $i$ 的原点在相机 $j$ 坐标系下的坐标**。

## 3. Convert `GlobalPoses` to `RelativePose`

如果已知两个相机的全局位姿（以PoSDK默认的 `RwTw` 格式为例）：
-   相机 $i$: $(R_w^i, \mathbf{t}_w^i)$
-   相机 $j$: $(R_w^j, \mathbf{t}_w^j)$

则它们之间的相对位姿 $(R_{ij}, \mathbf{t}_{ij})$ 计算如下：

1.  **相对旋转 $R_{ij}$**:

$$R_{ij} = R_w^j \cdot (R_w^i)^T$$

2.  **相对平移 $\mathbf{t}_{ij}$**:

$$\mathbf{t}_{ij} = R_w^j \cdot (\mathbf{t}_w^i - \mathbf{t}_w^j)$$


## 4. 图像坐标形式

### 4.1 图像像素坐标 (2D)

-   **像素坐标**: $\mathbf{p} = (u, v)^T$，原点在图像左上角，$u$向右，$v$向下。
    -   单位为像素
    -   在PoSDK中通常用 `std::vector<cv::Point2f>` 或 `Eigen::Matrix<double, 2, N>` 表示

### 4.2 齐次像素坐标 (3D)

-   **齐次像素坐标**: $\mathbf{p}_h = (u, v, 1)^T$，是像素坐标的齐次表示。
    -   便于使用矩阵乘法进行变换
    -   变换后需要通过第三个分量进行归一化
  
     $$\mathbf{p}_c \sim K \cdot [R_w^c | -R_w^c \cdot t_w] \cdot \begin{bmatrix} X_w \\ 1 \end{bmatrix}$$

     这里$K$为相机内参矩阵
      $K = \begin{bmatrix}
        f_x & 0 & c_x \\
        0 & f_y & c_y \\
        0 & 0 & 1
    \end{bmatrix}$
    
    其中$f_x, f_y$ 是焦距，$c_x, c_y$ 是主点坐标。

### 4.3 归一化图像坐标 (3D)

-   **归一化图像坐标**: $\mathbf{x} = (x, y, 1)^T$，表示从相机光心指向图像上某一点的向量。
    -   单位为归一化距离（与焦距有关）
    -   从像素坐标通过相机内参矩阵 $K$ 的逆变换得到：
    
    $$\mathbf{x} \sim K^{-1} \cdot \mathbf{p}_h$$
    

### 4.4 Bearing Vector (3D)

-   **Bearing Vector**: $\mathbf{b} = \frac{\mathbf{x}}{||\mathbf{x}||} $
    -   是归一化图像坐标 $\mathbf{x}$ 进行单位化后的结果
    -   模长恒为1的单位向量
    -   表示从相机光心到图像特征点的单位方向向量
    -   在PoSDK中，`types::BearingVectors` 是一个 `Eigen::Matrix<double, 3, Eigen::Dynamic>`，每一列是一个3D的bearing vector
