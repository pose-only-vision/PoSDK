# PoSDK Standard Pose Conventions

To ensure consistency in library internal operations and clarity when interacting with external data, PoSDK follows the following standard conventions for camera pose representation.

## 1. Global Camera Pose (`types::GlobalPoses`)

In PoSDK, the global pose of a single camera (i.e., the camera's orientation and position relative to the world coordinate system) is defined by a rotation matrix $R$ and a translation vector $\mathbf{t}$. By default, PoSDK uses the **`PoseFormat::RwTw`** format.

### PoseFormat::RwTw (Default)

-   **Meaning**: A 3D feature point $\mathbf{X}_w$ in the world coordinate system is transformed to a normalized image feature point $\mathbf{x}_c$ in the camera coordinate system using the formula:

$$\mathbf{x}_c \sim R_w^c \cdot (\mathbf{X}_w - \mathbf{t}_w)$$

-   **Description**:
    -   $R_w^c$ (`types::GlobalPoses::rotations[i]`):
        -   A 3x3 rotation matrix.
        -   Represents the rotation transformation from **World coordinate system** to **Camera coordinate system**.
    -   $\mathbf{t}_w$ (`types::GlobalPoses::translations[i]`):
        -   A 3x1 translation vector.
        -   Represents **the position of the camera center in the world coordinate system**.
        -   For the $i$-th camera, the pose will be written as [$R_w^i, \mathbf{t}_w^i$].

### PoseFormat::RwTc

PoSDK also supports another common pose representation format **`PoseFormat::RwTc`** and provides conversion functions between the two formats.

-   **Meaning**: A 3D feature point $\mathbf{X}_w$ in the world coordinate system is transformed to a normalized image feature point $\mathbf{x}_c$ in the camera coordinate system using the formula:

$$\mathbf{x}_c \sim R_w^c \cdot \mathbf{X}_w + \mathbf{t}_c$$

-   **Description**:
    -   $R_w^c$: Same definition as in `RwTw`, representing rotation from world to camera.
    -   $\mathbf{t}_c$: A 3x1 translation vector representing **the coordinates of the world coordinate system origin in the camera coordinate system**. Note this differs from $\mathbf{t}_w$ in `RwTw`.


-   **Relationship with RwTw**: $\mathbf{t}_c = -R_w^c \cdot \mathbf{t}_w$

>> The `types::GlobalPoses` class provides member function `ConvertPoseFormat(target_format, ...)`, as well as convenience functions `RwTw_to_RwTc(...)` and `RwTc_to_RwTw(...)` to convert between these two formats.

## 2. Relative Camera Pose (`types::RelativePose`)

The relative pose between two cameras (view $i$ and view $j$) is defined by a rotation matrix $R_{ij}$ and a translation vector $\mathbf{t}_{ij}$.

-   **Meaning**: A normalized image feature observation $\mathbf{x}_i$ in camera $i$ coordinate system is transformed to the corresponding point $\mathbf{x}_j$ in camera $j$ coordinate system using the formula:

$$\mathbf{x}_j \sim R_{ij} \cdot \mathbf{x}_i + \mathbf{t}_{ij}$$

-   **Description**:
    -   $R_{ij}$ (`types::RelativePose::Rij`):
        -   A 3x3 rotation matrix.
        -   Represents the rotation transformation from **Camera $i$ coordinate system** to **Camera $j$ coordinate system**.
    -   $\mathbf{t}_{ij}$ (`types::RelativePose::tij`):
        -   A 3x1 translation vector.
        -   Represents **the coordinates of camera $i$'s origin in camera $j$ coordinate system**.

## 3. Convert `GlobalPoses` to `RelativePose`

If the global poses of two cameras are known (using PoSDK's default `RwTw` format as an example):
-   Camera $i$: $(R_w^i, \mathbf{t}_w^i)$
-   Camera $j$: $(R_w^j, \mathbf{t}_w^j)$

Then their relative pose $(R_{ij}, \mathbf{t}_{ij})$ is calculated as follows:

1.  **Relative rotation $R_{ij}$**:

$$R_{ij} = R_w^j \cdot (R_w^i)^T$$

2.  **Relative translation $\mathbf{t}_{ij}$**:

$$\mathbf{t}_{ij} = R_w^j \cdot (\mathbf{t}_w^i - \mathbf{t}_w^j)$$


## 4. Image Coordinate Forms

### 4.1 Image Pixel Coordinates (2D)

-   **Pixel coordinates**: $\mathbf{p} = (u, v)^T$, origin at top-left corner of image, $u$ to the right, $v$ downward.
    -   Units are pixels
    -   In PoSDK typically represented as `std::vector<cv::Point2f>` or `Eigen::Matrix<double, 2, N>`

### 4.2 Homogeneous Pixel Coordinates (3D)

-   **Homogeneous pixel coordinates**: $\mathbf{p}_h = (u, v, 1)^T$, homogeneous representation of pixel coordinates.
    -   Convenient for matrix multiplication transformations
    -   After transformation, normalization is required through the third component
  
     $$\mathbf{p}_c \sim K \cdot [R_w^c | -R_w^c \cdot t_w] \cdot \begin{bmatrix} X_w \\ 1 \end{bmatrix}$$

     Here $K$ is the camera intrinsic matrix
     $K = \begin{bmatrix}
        f_x & 0 & c_x \\
        0 & f_y & c_y \\
        0 & 0 & 1
    \end{bmatrix}$
    
    where $f_x, f_y$ are focal lengths, $c_x, c_y$ are principal point coordinates.

### 4.3 Normalized Image Coordinates (3D)

-   **Normalized image coordinates**: $\mathbf{x} = (x, y, 1)^T$, representing a vector from the camera center pointing to a point on the image.
    -   Units are normalized distances (related to focal length)
    -   Obtained from pixel coordinates through the inverse transformation of camera intrinsic matrix $K$:
    
    $$\mathbf{x} \sim K^{-1} \cdot \mathbf{p}_h$$
    

### 4.4 Bearing Vector (3D)

-   **Bearing Vector**: $\mathbf{b} = \frac{\mathbf{x}}{||\mathbf{x}||} $
    -   Result of normalizing the normalized image coordinates $\mathbf{x}$
    -   Unit vector with constant magnitude of 1
    -   Represents the unit direction vector from camera center to image feature point
    -   In PoSDK, `types::BearingVectors` is an `Eigen::Matrix<double, 3, Eigen::Dynamic>`, where each column is a 3D bearing vector
