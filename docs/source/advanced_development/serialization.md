# Serialization Storage and Reading (`PbDataIO`)

- **Purpose**: Provides a standardized way to save and load plugin data, using Protobuf for cross-platform and version compatibility.
- **Base Class**: Inherit from `Interface::PbDataIO`.
- **Core Virtual Functions**:
  - **`virtual std::unique_ptr<google::protobuf::Message> CreateProtoMessage() const = 0;`**: Creates an empty Protobuf message object for deserialization.
  - **`virtual std::unique_ptr<google::protobuf::Message> ToProto() const = 0;`**: Converts plugin internal data to Protobuf message object and returns it.
  - **`virtual bool FromProto(const google::protobuf::Message& message) = 0;`**: Parses data from Protobuf message object and fills plugin internal data.
- **Automatic File I/O**: `PbDataIO`'s `Save` and `Load` methods automatically handle file reading/writing and Protobuf serialization/deserialization. Developers only need to implement the three core functions above.
- **Helper Macros**:
  - **`PROTO_SET_BASIC(proto_msg, field, value)`**: Set basic type fields (int, float, bool, string, etc.).
  - **`PROTO_GET_BASIC(proto_msg, field, value)`**: Get basic type fields.
  - **`PROTO_SET_VECTOR2F/D(proto_msg, field, vec)`**: Set Eigen Vector2f/d.
  - **`PROTO_GET_VECTOR2F/D(proto_msg, field, vec)`**: Get Eigen Vector2f/d.
  - **`PROTO_SET_VECTOR3D(proto_msg, field, vec)`**: Set Eigen Vector3d.
  - **`PROTO_GET_VECTOR3D(proto_msg, field, vec)`**: Get Eigen Vector3d.
  - **`PROTO_SET_MATRIX3D(proto_msg, field, mat)`**: Set Eigen Matrix3d (column-major).
  - **`PROTO_GET_MATRIX3D(proto_msg, field, mat)`**: Get Eigen Matrix3d.
  - **`PROTO_SET_ARRAY(proto_msg, field, array)`**: Set `std::vector<basic_type>`.
  - **`PROTO_GET_ARRAY(proto_msg, field, array)`**: Get `std::vector<basic_type>`.
  - **`PROTO_SET_ENUM(proto_msg, field, value)`**: Set enum type fields.
  - **`PROTO_GET_ENUM(proto_msg, field, value)`**: Get enum type fields.
- **Usage Flow**:
  1. Define `.proto` file for data.
  2. Configure Protobuf code generation in CMake.
  3. Plugin class inherits from `PbDataIO`.
  4. Implement `GetType`, `GetData`.
  5. Implement `CreateProtoMessage`, `ToProto`, `FromProto`, using helper macros for field mapping.
  6. (Optional) Implement `CopyData`.
  7. Use `REGISTRATION_PLUGIN` in `.cpp` file for registration.
- **Storage Paths**:
  - `SetStorageFolder(path)`: Set default save/load directory.
  - `Save(folder, filename, ext)`:
    - If `folder` is empty, use `storage_dir_`.
    - If `filename` is empty, use `GetType() + "_default"`.
    - If `extension` is empty, use `.pb`.
  - `Load(filepath, file_type)`:
    - If `filepath` is empty, use `storage_dir_ / (GetType() + "_default.pb")`.
    - If `filepath` has no extension, default to adding `.pb`.
