# OpenMVG 位姿约定

**（v2版本后续完善）**
<!--
OpenMVG (Open Multiple View Geometry) 是一个流行的多视图几何库，常用于 Structure from Motion (SfM) 和 SLAM。

## 全局位姿 (Poses)

在 OpenMVG 中，`openMVG::geometry::Pose3` 类用于表示刚体变换，即相机在世界坐标系中的位姿。

一个 `Pose3` 对象包含：

*   **旋转 (Rotation)**: 表示为一个 `openMVG::Mat3` (实质上是 Eigen::Matrix3d)，记为 $R$。
*   **相机中心 (Center)**: 表示为一个 `openMVG::Vec3` (实质上是 Eigen::Vector3d)，记为 $C$。

这种表示方式定义了 **从世界坐标系到相机坐标系的变换** $T_{cw}$。世界坐标系中的一个点 $P_w$ 转换到相机坐标系中的点 $P_c$ 的公式为：

\[ P_c = R (P_w - C) \]

展开后为：

\[ P_c = R P_w - R C \]

因此，变换矩阵 $T_{cw}$ (从世界到相机) 可以表示为：

\[ T_{cw} = \begin{bmatrix} R & -RC \\ \mathbf{0}^T & 1 \end{bmatrix} \]

其中：
* $R$ 是 $3 \times 3$ 的旋转矩阵，表示相机坐标系相对于世界坐标系的姿态。
* $C$ 是 $3 \times 1$ 的平移向量，表示相机光心在世界坐标系中的位置。
* $-RC$ 是平移向量 $t$，使得 $P_c = R P_w + t$。

## 相对位姿 (Relative Poses)

对于两个相机 $i$ 和 $j$，它们在世界坐标系中的位姿分别为 $T_{iw} = (R_i, C_i)$ 和 $T_{jw} = (R_j, C_j)$。
注意这里为了方便，我们使用 $T_{iw}$ 表示从世界到相机 $i$ 的变换，所以 $R_i$ 是 $R_{iw}$，$C_i$ 是 $C_{iw}$。

相对位姿 $T_{ij}$ (从相机 $i$ 到相机 $j$) 可以计算如下：

世界到相机 $i$: $P_i = R_i (P_w - C_i)$
世界到相机 $j$: $P_j = R_j (P_w - C_j)$

从 $P_i = R_i P_w - R_i C_i$，可以得到 $P_w = R_i^T P_i + C_i$。
代入第二个式子：
\[ P_j = R_j ( (R_i^T P_i + C_i) - C_j ) \]
\[ P_j = R_j R_i^T P_i + R_j (C_i - C_j) \]

所以，相对旋转 $R_{ji}$ (从 $i$ 到 $j$) 是 $R_j R_i^T$，相对平移(相机 $i$ 的原点在相机 $j$ 坐标系下的坐标) $t_{ji}$ 是 $R_j (C_i - C_j)$。
OpenMVG 通常直接存储和使用这种 $R, C$ 形式的全局位姿。

## 与PoSDK的转换

PoSDK 使用 $R_{wc}$ (从相机到世界) 和 $t_{wc}$ (相机光心在世界坐标系下的位置) 或 $R_{cw}$ (从世界到相机) 和 $t_{cw}$ (世界原点在相机坐标系下的位置)。

如果 PoSDK 使用 $R_{cw}$ 和 $t_{cw}$ (世界原点在相机 $i$ 下的坐标)：
*   $R_{PoSDK} = R_{OpenMVG}$
*   $t_{PoSDK} = -R_{OpenMVG} C_{OpenMVG}$

如果 PoSDK 使用 $R_{wc}$ 和 $C_{wc}$ (相机中心在世界坐标系中的位置，与OpenMVG的 $C$ 类似，但旋转是 $R_{wc}$):
*   $R_{PoSDK} = R_{OpenMVG}^T$
*   $C_{PoSDK} = C_{OpenMVG}$

转换时需要特别注意旋转矩阵的定义方向和是相机中心 $C$ 还是平移向量 $t$。 
-->