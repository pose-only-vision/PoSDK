# OpenGV Conventions

The OpenGV library provides some utility functions in its `experiment_helpers` that can generate camera poses, 3D points, and corresponding 2D observations (bearing vectors) with ground truth. Understanding the pose conventions in these simulated data is crucial for correctly importing them into PoSDK for testing and evaluation.

## 1. Pose Conventions in OpenGV Simulation

### 1.1 Global Pose (Camera Pose)

In OpenGV simulation functions such as `generateRandom2D3DCorrespondences` or `generateRandom2D2DCorrespondences`, usually `position` and `rotation` parameters are used to define a camera's (or camera system's reference vehicle's) pose relative to the world coordinate system:

-   `position` (`translation_t`): Represents the position of the camera (or vehicle) coordinate system origin in **world coordinate system**, denoted as $\mathbf{t}_w$ (world to body/camera_rig)
-   `rotation` (`rotation_t`): Represents the rotation matrix from camera (or vehicle) coordinate system to **world coordinate system**, denoted as $R_b^w$ (body/camera_rig to world)

-   **Meaning**: A 3D feature point $\mathbf{X}_w$ in the world coordinate system is transformed to a point $\mathbf{x}_i$ in camera (or vehicle) normalized image coordinates using the formula:

$$R_i^w \cdot \mathbf{x}_i \sim \mathbf{X}_w - \mathbf{t}_w$$

-   **Description**:
    -   $R_i^w$ is $(R_w^i)^T$ (rotation from world to vehicle)
    -   This conforms to the **PoseFormat::RwTw** pose representation format
    -   If there is a multi-camera system (`camOffsets`, `camRotations`), the above $\mathbf{X}_b$ will be further transformed through these cameras' intrinsics and extrinsics relative to the vehicle to get observations in a specific camera's coordinate system

### 1.2 Relative Pose

OpenGV's `extractRelativePose(pos1, rot1, pos2, rot2, relPos, relRot)` function is used to calculate the relative pose between two absolute poses:   
In a two-view $(i,j)$ pair, the formula for transforming camera $i$'s observation $\mathbf{x}_{i}$ to the corresponding point $\mathbf{x}_{j}$ in camera $j$'s coordinate system is:
$$x_{i} \sim R_{ji} \cdot x_{j} + \mathbf{t}_{ji} \triangleq R\cdot x_{j} + \mathbf{t} $$
-   **Description**:
    -   Camera $i$'s absolute pose is $(R_{i}^w,\mathbf{t}_w^i)$
    -   Camera $j$'s absolute pose is $(R_{j}^w,\mathbf{t}_w^j)$
  

## 2. Convert `GlobalPoses` to `RelativePose`

The relative pose $(R_{ij}, \mathbf{t}_{ij})$ calculated from absolute poses in OpenGV is computed as follows:

1.  **Relative rotation $R ( = R_{ji})$**:

$$R = ({R_i^w})^T \cdot R_j^w$$

2.  **Relative translation $\mathbf{t} ( = \mathbf{t}_{ji})$**:

 $$\mathbf{t} = (R_i^w)^T \cdot (\mathbf{t}_j - \mathbf{t}_i)$$

```{important} PoSDK Standard Pose Conventions
>
> For clarity, we separately describe PoSDK's definitions in detail in [PoSDK Standard Pose Conventions](./posdk.md).
>
>
``` 

## 3. Conversion from OpenGV Simulation Data to PoSDK

### 3.1 Global Pose Conversion

Assume we obtain a single camera's global pose `position_opengv` $(\mathbf{t}_w)$ and `rotation_opengv` $(R_b^w)$ from OpenGV simulation:

-   PoSDK global pose is initialized in `PoseFormat::RwTw` format, i.e., $\mathbf{x}_c \sim R_w^c \cdot (\mathbf{X}_w - \mathbf{t}_w)$
-   OpenGV's global pose is initialized in `PoseFormat::RcTw` format, i.e., $\mathbf{x}_c \sim (R_c^w)^T \cdot (\mathbf{X}_w - \mathbf{t}_w)$

-   **Conversion**:
    -   $R_{po} = (R_{opengv})^T$
    -   $\mathbf{t}_{po} = \mathbf{t}_{opengv}$

### 3.2 Relative Pose Conversion

Assume we obtain relative pose $(R_{opengv}, \mathbf{t}_{opengv})$ from OpenGV `extractRelativePose`:

-   PoSDK's `RelativePose` definition is $\mathbf{x}_{j} \sim R_{po} \cdot \mathbf{x}_{i} + \mathbf{t}$, where:
-  OpenGV's relative pose definition is $\mathbf{x}_{i} \sim R_{opengv} \cdot \mathbf{x}_{j} + \mathbf{t}_{opengv}$, where:

-   **Conversion**:
    -   $R_{po} = (R_{opengv})^T$
    -   $\mathbf{t}_{po} = -(R_{opengv})^T \cdot\mathbf{t}_{opengv}$

