# OpenMVG Pose Conventions

**（To be completed in v2 version）**
<!--
OpenMVG (Open Multiple View Geometry) is a popular multi-view geometry library commonly used for Structure from Motion (SfM) and SLAM.

## Global Poses

In OpenMVG, the `openMVG::geometry::Pose3` class is used to represent rigid transformations, i.e., camera poses in the world coordinate system.

A `Pose3` object contains:

*   **Rotation**: Represented as an `openMVG::Mat3` (essentially Eigen::Matrix3d), denoted as $R$.
*   **Camera Center**: Represented as an `openMVG::Vec3` (essentially Eigen::Vector3d), denoted as $C$.

This representation defines the **transformation from world coordinate system to camera coordinate system** $T_{cw}$. The formula for transforming a point $P_w$ in the world coordinate system to a point $P_c$ in the camera coordinate system is:

\[ P_c = R (P_w - C) \]

Expanded as:

\[ P_c = R P_w - R C \]

Therefore, the transformation matrix $T_{cw}$ (from world to camera) can be represented as:

\[ T_{cw} = \begin{bmatrix} R & -RC \\ \mathbf{0}^T & 1 \end{bmatrix} \]

where:
* $R$ is a $3 \times 3$ rotation matrix representing the camera coordinate system's orientation relative to the world coordinate system.
* $C$ is a $3 \times 1$ translation vector representing the camera center's position in the world coordinate system.
* $-RC$ is the translation vector $t$ such that $P_c = R P_w + t$.

## Relative Poses

For two cameras $i$ and $j$, their poses in the world coordinate system are $T_{iw} = (R_i, C_i)$ and $T_{jw} = (R_j, C_j)$ respectively.
Note that for convenience, we use $T_{iw}$ to represent the transformation from world to camera $i$, so $R_i$ is $R_{iw}$ and $C_i$ is $C_{iw}$.

The relative pose $T_{ij}$ (from camera $i$ to camera $j$) can be calculated as follows:

World to camera $i$: $P_i = R_i (P_w - C_i)$
World to camera $j$: $P_j = R_j (P_w - C_j)$

From $P_i = R_i P_w - R_i C_i$, we can get $P_w = R_i^T P_i + C_i$.
Substituting into the second equation:
\[ P_j = R_j ( (R_i^T P_i + C_i) - C_j ) \]
\[ P_j = R_j R_i^T P_i + R_j (C_i - C_j) \]

So, the relative rotation $R_{ji}$ (from $i$ to $j$) is $R_j R_i^T$, and the relative translation (coordinates of camera $i$'s origin in camera $j$'s coordinate system) $t_{ji}$ is $R_j (C_i - C_j)$.
OpenMVG usually directly stores and uses this $R, C$ form of global poses.

## Conversion with PoSDK

PoSDK uses $R_{wc}$ (from camera to world) and $t_{wc}$ (camera center position in world coordinate system) or $R_{cw}$ (from world to camera) and $t_{cw}$ (world origin coordinates in camera coordinate system).

If PoSDK uses $R_{cw}$ and $t_{cw}$ (world origin coordinates in camera $i$):
*   $R_{PoSDK} = R_{OpenMVG}$
*   $t_{PoSDK} = -R_{OpenMVG} C_{OpenMVG}$

If PoSDK uses $R_{wc}$ and $C_{wc}$ (camera center position in world coordinate system, similar to OpenMVG's $C$, but rotation is $R_{wc}$):
*   $R_{PoSDK} = R_{OpenMVG}^T$
*   $C_{PoSDK} = C_{OpenMVG}$

When converting, special attention should be paid to the rotation matrix definition direction and whether it's camera center $C$ or translation vector $t$. 
-->
