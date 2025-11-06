# Strecha Dataset Pose Conventions

The Strecha dataset is widely used for multi-view stereo vision and SLAM algorithm evaluation. Understanding the format of its camera poses and projection files is crucial for correctly using this dataset.

## File Format

Each scene in the Strecha dataset typically contains a series of `.camera` files corresponding to image files. For example, for image `0001.jpg`, its corresponding camera parameters are in the `0001.jpg.camera` file.

Each `.camera` file contains the following information in order:

1.  **Intrinsic matrix $K$** (3 rows): A $3 \times 3$ camera intrinsic matrix.

    $$K = \begin{bmatrix} f_x & s & c_x \\ 0 & f_y & c_y \\ 0 & 0 & 1 \end{bmatrix}$$

    where $f_x$, $f_y$ are focal lengths, $c_x$, $c_y$ are principal point coordinates, $s$ is the skew coefficient (usually 0).

2.  **Empty line** (1 row): An empty (zero) line used to separate intrinsics and extrinsics.

3.  **Rotation matrix $R^i_w$** (3 rows): A $3 \times 3$ rotation matrix<span style="color:red">(row-major storage)</span>. This represents the rotation from **world coordinate system to camera $i$ coordinate system**.

4.  **Camera center $\mathbf{t}^i_w$** (1 row): A $3 \times 1$ translation vector<span style="color:red">(row vector storage)</span>. This represents **the position of camera $i$'s center in the world coordinate system**.

## Global Camera Pose

According to the above file format, for camera $i$, we directly read from the file:
*   Rotation: $R^i_w$ (rotation from **world coordinate system to camera $i$ coordinate system**)
*   Camera center: $\mathbf{t}^i_w$ (3D coordinates of camera $i$'s center in the **world coordinate system**)

A 3D point $P_w$ in the world coordinate system can be transformed to a point $P_{c,i}$ in camera $i$'s coordinate system using the following formula:

$$P_i = R^i_w (P_w - \mathbf{t}^i_w)$$

This is equivalent to using the augmented matrix $[R^i_w | -R^i_w \mathbf{t}^i_w]$ for transformation.

Therefore, reading from Strecha ground truth files will load into PoSDK `RwTw` format.

## Relative Camera Pose

Consistent with posdk conventions (see [posdk.md](../conventions/posdk.md)), the Strecha dataset's relative poses are also stored in `RwTw` format.
