# 序列化存储与读取 (`PbDataIO`)

- **目的**: 提供一种标准化的方式来保存和加载插件数据，利用 Protobuf 实现跨平台和版本兼容。
- **基类**: 继承 `Interface::PbDataIO`。
- **核心虚函数**:
  - **`virtual std::unique_ptr<google::protobuf::Message> CreateProtoMessage() const = 0;`**: 创建一个空的 Protobuf 消息对象，用于反序列化。
  - **`virtual std::unique_ptr<google::protobuf::Message> ToProto() const = 0;`**: 将插件内部数据转换为 Protobuf 消息对象并返回。
  - **`virtual bool FromProto(const google::protobuf::Message& message) = 0;`**: 从 Protobuf 消息对象中解析数据并填充到插件内部。
- **自动文件 I/O**: `PbDataIO` 的 `Save` 和 `Load` 方法会自动处理文件读写和 Protobuf 的序列化/反序列化流程。开发者只需实现上述三个核心函数。
- **辅助宏**:
  - **`PROTO_SET_BASIC(proto_msg, field, value)`**: 设置基础类型字段 (int, float, bool, string等)。
  - **`PROTO_GET_BASIC(proto_msg, field, value)`**: 获取基础类型字段。
  - **`PROTO_SET_VECTOR2F/D(proto_msg, field, vec)`**: 设置 Eigen Vector2f/d。
  - **`PROTO_GET_VECTOR2F/D(proto_msg, field, vec)`**: 获取 Eigen Vector2f/d。
  - **`PROTO_SET_VECTOR3D(proto_msg, field, vec)`**: 设置 Eigen Vector3d。
  - **`PROTO_GET_VECTOR3D(proto_msg, field, vec)`**: 获取 Eigen Vector3d。
  - **`PROTO_SET_MATRIX3D(proto_msg, field, mat)`**: 设置 Eigen Matrix3d (按列优先)。
  - **`PROTO_GET_MATRIX3D(proto_msg, field, mat)`**: 获取 Eigen Matrix3d。
  - **`PROTO_SET_ARRAY(proto_msg, field, array)`**: 设置 `std::vector<基础类型>`。
  - **`PROTO_GET_ARRAY(proto_msg, field, array)`**: 获取 `std::vector<基础类型>`。
  - **`PROTO_SET_ENUM(proto_msg, field, value)`**: 设置枚举类型字段。
  - **`PROTO_GET_ENUM(proto_msg, field, value)`**: 获取枚举类型字段。
- **使用流程**:
  1. 定义数据的 `.proto` 文件。
  2. 在 CMake 中配置 Protobuf 代码生成。
  3. 插件类继承 `PbDataIO`。
  4. 实现 `GetType`, `GetData`。
  5. 实现 `CreateProtoMessage`, `ToProto`, `FromProto`，使用辅助宏进行字段映射。
  6. (可选) 实现 `CopyData`。
  7. 在 `.cpp` 文件中使用 `REGISTRATION_PLUGIN` 注册。
- **存储路径**:
  - `SetStorageFolder(path)`: 设置默认保存/加载目录。
  - `Save(folder, filename, ext)`:
    - 如果 `folder` 为空，使用 `storage_dir_`。
    - 如果 `filename` 为空，使用 `GetType() + "_default"`。
    - 如果 `extension` 为空，使用 `.pb`。
  - `Load(filepath, file_type)`:
    - 如果 `filepath` 为空，使用 `storage_dir_ / (GetType() + "_default.pb")`。
    - 如果 `filepath` 没有扩展名，默认添加 `.pb`。 