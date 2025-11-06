# OpenCV 位姿约定


**（v2版本完善）**
<!--
OpenCV 是一个广泛使用的计算机视觉库，其位姿表示方式与其他库可能有所不同。

## 相机外参 (Extrinsics)

在OpenCV中，相机位姿通常通过一个旋转向量 `rvec` (Rodrigues向量) 和一个平移向量 `tvec` 来表示。

*   **`rvec`**: 一个 $3 \times 1$ 的旋转向量。它表示了一个围绕轴 $ (r_x, r_y, r_z) $ 旋转 $\theta$ 角度的旋转。旋转轴的方向是 $ (r_x, r_y, r_z) $，旋转角度 $\theta$ 是该向量的模 $ \sqrt{r_x^2 + r_y^2 + r_z^2} $。
    可以使用 `cv::Rodrigues` 函数将旋转向量转换为 $3 \times 3$ 的旋转矩阵 $R$，反之亦然。

*   **`tvec`**: 一个 $3 \times 1$ 的平移向量。

这两个向量共同定义了从世界坐标系到相机坐标系的转换。一个在世界坐标系下的点 $P_w = (X_w, Y_w, Z_w)^T$ 可以通过以下方式转换到相机坐标系下的点 $P_c = (X_c, Y_c, Z_c)^T$

\[ P_c = R P_w + tvec \]

或者，如果使用齐次坐标表示，变换矩阵 $T_{cw}$ (从世界到相机) 可以写为

\[ T_{cw} = \begin{bmatrix} R & tvec \\ \mathbf{0}^T & 1 \end{bmatrix} \]

其中 $R$ 是由 `rvec` 转换得到的旋转矩阵。

## 投影过程

相机坐标系下的点 $P_c$ 随后通过相机内参矩阵 $K$ 投影到图像平面：

令 $P_c = (X_c, Y_c, Z_c)^T$。
首先进行归一化，得到归一化图像坐标 $(x', y')$:

\[ x' = X_c / Z_c \]
\[ y' = Y_c / Z_c \]

然后，如果考虑畸变，会应用畸变模型。假设 $ (x_d, y_d) $ 是畸变校正后的归一化坐标。

最后，通过相机内参矩阵 $K$ 转换为像素坐标 $(u, v)$:

\[ K = \begin{bmatrix} f_x & s & c_x \\ 0 & f_y & c_y \\ 0 & 0 & 1 \end{bmatrix} \]

\[ \begin{bmatrix} u \\ v \\ 1 \end{bmatrix} = K \begin{bmatrix} x_d \\ y_d \\ 1 \end{bmatrix} \]

其中：
* $f_x, f_y$ 是焦距。
* $c_x, c_y$ 是主点坐标。
* $s$ 是扭曲系数 (通常为0)。

## 注意事项

*   OpenCV 的位姿表示的是 **从世界坐标系到相机坐标系的变换**。
*   `tvec` 表示的是世界坐标系原点在相机坐标系下的坐标，或者等效地，相机坐标系原点相对于世界坐标系原点的平移向量，然后经过 $R$ 旋转。
    更准确地说，如果 $T_{wc}$ 是从相机到世界的变换 (即相机在世界中的位姿)，其形式为 $T_{wc} = \begin{bmatrix} R_w & t_w \\ \mathbf{0}^T & 1 \end{bmatrix}$，则 $R = R_w^T$ 且 $tvec = -R_w^T t_w$。
-->
