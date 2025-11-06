# OpenCV Pose Conventions


**（To be completed in v2 version）**
<!--
OpenCV is a widely used computer vision library, and its pose representation may differ from other libraries.

## Camera Extrinsics

In OpenCV, camera poses are usually represented by a rotation vector `rvec` (Rodrigues vector) and a translation vector `tvec`.

*   **`rvec`**: A $3 \times 1$ rotation vector. It represents a rotation around axis $ (r_x, r_y, r_z) $ by angle $\theta$. The direction of the rotation axis is $ (r_x, r_y, r_z) $, and the rotation angle $\theta$ is the magnitude of the vector $ \sqrt{r_x^2 + r_y^2 + r_z^2} $.
    The `cv::Rodrigues` function can be used to convert the rotation vector to a $3 \times 3$ rotation matrix $R$, and vice versa.

*   **`tvec`**: A $3 \times 1$ translation vector.

These two vectors together define the transformation from world coordinate system to camera coordinate system. A point $P_w = (X_w, Y_w, Z_w)^T$ in the world coordinate system can be transformed to a point $P_c = (X_c, Y_c, Z_c)^T$ in the camera coordinate system as follows:

\[ P_c = R P_w + tvec \]

Or, if using homogeneous coordinates, the transformation matrix $T_{cw}$ (from world to camera) can be written as:

\[ T_{cw} = \begin{bmatrix} R & tvec \\ \mathbf{0}^T & 1 \end{bmatrix} \]

where $R$ is the rotation matrix converted from `rvec`.

## Projection Process

Points $P_c$ in camera coordinate system are then projected onto the image plane through the camera intrinsic matrix $K$:

Let $P_c = (X_c, Y_c, Z_c)^T$.
First normalize to get normalized image coordinates $(x', y')$:

\[ x' = X_c / Z_c \]
\[ y' = Y_c / Z_c \]

Then, if distortion is considered, a distortion model is applied. Assume $ (x_d, y_d) $ are the normalized coordinates after distortion correction.

Finally, convert to pixel coordinates $(u, v)$ through camera intrinsic matrix $K$:

\[ K = \begin{bmatrix} f_x & s & c_x \\ 0 & f_y & c_y \\ 0 & 0 & 1 \end{bmatrix} \]

\[ \begin{bmatrix} u \\ v \\ 1 \end{bmatrix} = K \begin{bmatrix} x_d \\ y_d \\ 1 \end{bmatrix} \]

where:
* $f_x, f_y$ are focal lengths.
* $c_x, c_y$ are principal point coordinates.
* $s$ is the skew coefficient (usually 0).

## Notes

*   OpenCV's pose represents **transformation from world coordinate system to camera coordinate system**.
*   `tvec` represents the coordinates of the world coordinate system origin in the camera coordinate system, or equivalently, the translation vector of the camera coordinate system origin relative to the world coordinate system origin, then rotated by $R$.
    More precisely, if $T_{wc}$ is the transformation from camera to world (i.e., camera pose in world), with the form $T_{wc} = \begin{bmatrix} R_w & t_w \\ \mathbf{0}^T & 1 \end{bmatrix}$, then $R = R_w^T$ and $tvec = -R_w^T t_w$.
-->
