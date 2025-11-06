# Colmap 格式分析文档

## 1. Colmap 数据结构位置

### 1.1 Point2D 结构定义
**位置**: `src/colmap/scene/point2d.h`

```cpp
struct Point2D {
  Eigen::Vector2d xy = Eigen::Vector2d::Zero();
  point3D_t point3D_id = kInvalidPoint3DId;  // 默认值
  bool HasPoint3D() const { return point3D_id != kInvalidPoint3DId; }
};
```

**关键点**:
- `point3D_id` 类型为 `point3D_t` (uint64_t)
- 无效值使用 `kInvalidPoint3DId`，值为 `std::numeric_limits<uint64_t>::max()` (0xFFFFFFFFFFFFFFFF)
- `HasPoint3D()` 通过比较 `point3D_id != kInvalidPoint3DId` 来判断

### 1.2 Image 结构中的 Points2D
**位置**: `src/colmap/scene/image.h`

```cpp
class Image {
  std::vector<struct Point2D> points2D_;  // 完整的特征点列表
  const Point2D& Point2D(point2D_t point2D_idx) const;
  void SetPoint3DForPoint2D(point2D_t point2D_idx, point3D_t point3D_id);
};
```

**关键点**:
- 每个图像包含**完整的**特征点列表（points2D）
- `point2D_idx` 就是特征点在 `points2D_` 向量中的索引
- 每个 2D 点都有一个 `point3D_id`，如果未三角化则为 `kInvalidPoint3DId`

### 1.3 Point3D 结构和 Track
**位置**: `src/colmap/scene/point3d.h` (引用)

```cpp
struct Point3D {
  Eigen::Vector3d xyz;
  Eigen::Vector3ub color;
  double error;
  Track track;  // 包含 (image_id, point2D_idx) 对
};
```

**Track 结构**:
```cpp
struct TrackElement {
  image_t image_id;
  point2D_t point2D_idx;  // 在 image.Points2D() 中的索引
};
```

### 1.4 二进制文件写入格式

#### WriteImagesBinary (reconstruction_io_binary.cc:400-432)
```cpp
WriteBinaryLittleEndian<uint64_t>(&stream, image.NumPoints2D());
for (const Point2D& point2D : image.Points2D()) {
  WriteBinaryLittleEndian<double>(&stream, point2D.xy(0));
  WriteBinaryLittleEndian<double>(&stream, point2D.xy(1));
  WriteBinaryLittleEndian<point3D_t>(&stream, point2D.point3D_id);  // uint64_t
}
```

#### WritePoints3DBinary (reconstruction_io_binary.cc:441-465)
```cpp
WriteBinaryLittleEndian<point3D_t>(&stream, point3D_id);  // uint64_t
WriteBinaryLittleEndian<double>(&stream, point3D.xyz(0));
// ... 其他字段 ...
WriteBinaryLittleEndian<uint64_t>(&stream, point3D.track.Length());
for (const TrackElement& track_el : point3D.track.Elements()) {
  WriteBinaryLittleEndian<image_t>(&stream, track_el.image_id);
  WriteBinaryLittleEndian<point2D_t>(&stream, track_el.point2D_idx);  // uint32_t
}
```

### 1.5 类型定义 (types.h:119-120)
```cpp
typedef uint64_t point3D_t;
constexpr point3D_t kInvalidPoint3DId = std::numeric_limits<uint64_t>::max();
```

## 2. 数据关联关系

### 2.1 images.bin 和 points3D.bin 的关联

1. **images.bin**:
   - 存储每个图像的完整特征点列表
   - 每个特征点 `point2D` 有 `(x, y, point3D_id)`
   - `point3D_id` 可以是：
     - 有效的 3D 点 ID（uint64_t，从 0 开始）
     - `kInvalidPoint3DId` (0xFFFFFFFFFFFFFFFF) 表示未三角化

2. **points3D.bin**:
   - 存储每个 3D 点的信息
   - 每个 3D 点包含一个 `track`，记录所有观测到这个 3D 点的 2D 特征
   - `track` 中的每个元素是 `(image_id, point2D_idx)` 对
   - `point2D_idx` 是特征点在 `images[image_id].Points2D()` 中的索引

### 2.2 数据一致性要求

- **完整性**: 每个图像必须包含完整的特征点列表
- **索引一致性**: `point2D_idx` 必须是 `images[image_id].Points2D()` 中的有效索引
- **双向关联**: 
  - `images[i].point3D_ids[j] = k` 表示图像 i 的第 j 个特征对应 3D 点 k
  - `points3D[k].track` 必须包含 `(i, j)` 元素

