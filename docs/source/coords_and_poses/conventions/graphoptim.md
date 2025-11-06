# GraphOptim Pose Conventions

GraphOptim is primarily used for rotation averaging.
**（To be completed in v2 version）**
<!--
## Input (Relative Poses)

GraphOptim's input consists of pairwise relative rotation matrices $R_{ij}$ and relative translation vectors $t_{ij}$. These represent the transformation relationship from camera $i$'s coordinate system to camera $j$'s coordinate system.

A point $P_i$ in camera $i$'s coordinate system can be transformed to a point $P_j$ in camera $j$'s coordinate system as follows:

\[ P_j = R_{ij} P_i + t_{ij} \]

where
* $R_{ij}$ is a $3 \times 3$ rotation matrix.
* $t_{ij}$ is a $3 \times 1$ translation vector.

## Output (Rotation Part of Global Poses)

GraphOptim's output is the global rotation matrix $R_w$ (usually represented as $R_{wi}$, i.e., rotation from world coordinate system to camera $i$'s coordinate system). It does not directly output global translations.

In GraphOptim's context, the first camera's pose is usually considered as the world coordinate system, i.e., $R_{w0} = I$ (identity matrix) and $t_{w0} = \mathbf{0}$ (zero vector). Other cameras' global rotations $R_{wi}$ are relative to this implicitly defined first camera (world) coordinate system.

Therefore, for any camera $i$, its rotation in the world coordinate system is $R_{wi}$. The rotation part of transforming a point $P_w$ in the world coordinate system to a point $P_i$ in camera $i$'s coordinate system can be expressed as

\[ P_i = R_{wi} P_w \]

Note that GraphOptim primarily focuses on optimizing the rotation part, while the translation part is usually handled by other modules (such as translation averaging or global BA).
-->
