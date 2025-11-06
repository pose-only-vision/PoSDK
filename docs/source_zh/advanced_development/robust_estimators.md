# 鲁棒估计器框架

PoSDK 提供了一个鲁棒估计器的框架，目前支持 RANSAC 和 GNC-IRLS。

## `DataSample<TSample>` 数据容器

- **目的**: 管理用于鲁棒估计的样本数据，特别是支持高效的子集采样而无需复制原始数据（零拷贝）。
- **模板参数 `TSample`**: 必须是 `std::vector<ValueType>` 类型，其中 `ValueType` 是样本中单个元素的类型（例如，`BearingPair`）。
- **核心特性**:
  - **零拷贝子集**: `GetRandomSubset(size)` 和 `GetSubset(indices)` 返回一个新的 `DataSample` 对象，该对象共享原始数据 (`data_map_ptr_`)，但通过 `subset_indices_` 存储子集的索引。
  - **STL 兼容接口**: 提供 `begin()`, `end()`, `operator[]`, `size()`, `empty()` 等接口，使得可以像使用 `std::vector` 一样访问样本数据（无论是完整样本还是子集）。迭代器和 `operator[]` 会自动处理索引映射。
- **使用**:
  - 通常作为 `RobustEstimator` 的输入数据类型。
  - `RobustEstimator` 内部会使用 `GetRandomSubset` 来获取随机样本进行模型估计。
  - `GetInlierSubset` 用于获取内点子集以进行最终模型优化。
- **示例**:
  ```cpp
  // 假设有原始数据 std::vector<MySampleType> original_data;
  auto data_sample = std::make_shared<DataSample<std::vector<MySampleType>>>(original_data);

  // 获取随机子集
  auto random_subset = data_sample->GetRandomSubset(10);
  std::cout << "Random subset size: " << random_subset->size() << std::endl;
  if (!random_subset->empty()) {
      // 访问子集元素
      MySampleType first_element = (*random_subset)[0];
      // 迭代子集
      for (const auto& element : *random_subset) {
          // ... process element ...
      }
  }
  ```

## `RobustEstimator<TSample>` 基类

- **模板参数 `TSample`**: 定义了估计器处理的样本数据类型（通常是 `BearingPairs` 或类似结构）。
- **核心组件**:
  - **模型估计器 (`model_estimator_ptr_`)**: 继承自 `MethodPreset`，负责从一个最小样本集估计模型参数（例如，LiRP 方法估计相对位姿）。其类型通过 `model_estimator_type` 配置选项指定。
  - **代价评估器 (`cost_evaluator_ptr_`)**: 继承自 `MethodPreset`，负责计算每个样本点相对于给定模型的残差（代价）。其类型通过 `cost_evaluator_type` 配置选项指定。
- **工作流程**:
  1. 通过 `FactoryMethod::Create` 创建 `RobustEstimator` 实例 (如 `RANSACEstimator` 或 `GNCIRLSEstimator`)。
  2. 设置 `model_estimator_type` 和 `cost_evaluator_type` 选项。
  3. (可选) 通过 `SetModelEstimatorOptions` 和 `SetCostEvaluatorOptions` 为内部的模型估计器和代价评估器设置特定参数。
  4. 通过 `SetRequiredData` 设置包含 `DataSample<TSample>` 的输入数据。
  5. 调用 `Build()` 启动鲁棒估计流程。
- **派生类**:
  - `RANSACEstimator<TSample>`: 实现标准的 RANSAC 算法。
  - `GNCIRLSEstimator<TSample>`: 实现 GNC-IRLS 算法，通常更鲁棒但计算量更大。
- **配置**: 通过 `.ini` 文件或 `SetMethodOptions` 配置鲁棒估计器的参数（如迭代次数、置信度、内点阈值）以及内部模型估计器和代价评估器的类型。

## 使用示例 (`test_ransac_lirp.cpp` 简化版)

```cpp
// 1. 创建 RANSAC 估计器实例
auto ransac_estimator = std::dynamic_pointer_cast<RobustEstimator>(
    FactoryMethod::Create("ransac_estimator") // 假设 TSample 是 BearingPairs
);
ASSERT_TRUE(ransac_estimator != nullptr);

// 2. 设置 RANSAC 参数
MethodOptions ransac_options {
    {"model_estimator_type", "method_LiRP"},         // 使用 LiRP 估计模型
    {"cost_evaluator_type", "method_relative_cost"}, // 使用相对位姿代价评估
    {"max_iterations", "1000"},
    {"confidence", "0.99"},
    {"inlier_threshold", "1e-4"}, // RANSAC 内点阈值
    {"min_sample_size", "8"}      // LiRP 需要 8 对点
};
ransac_estimator->SetMethodOptions(ransac_options);

// 3. (可选) 设置 LiRP 和 Cost Evaluator 的特定参数
MethodOptions lirp_options { {"identify_mode", "PPO"} /* ... 其他 LiRP 选项 ... */ };
ransac_estimator->SetModelEstimatorOptions(lirp_options);

MethodOptions cost_options { {"residual_type", "sampson"} /* ... 其他 Cost 选项 ... */ };
ransac_estimator->SetCostEvaluatorOptions(cost_options);

// 4. 准备输入数据 (DataSample<BearingPairs>)
BearingPairs bearing_pairs_data = GenerateBearingPairs(); // 假设有函数生成数据
auto sample_data = std::make_shared<DataSample<BearingPairs>>(bearing_pairs_data);

// 5. 设置输入数据
DataPackage input_package;
input_package.AddData("data_sample", sample_data); // RobustEstimator 需要 data_sample
// 如果 model_estimator 或 cost_evaluator 需要其他数据 (如相机模型)，也需添加到包中
// input_package.AddData("data_camera_model", camera_model_data);
ransac_estimator->SetRequiredData(input_package); // 或者直接传入 Build

// 6. 执行 RANSAC
auto result = ransac_estimator->Build();

// 7. 处理结果 (通常是 DataMap<RelativePose>)
if (result) {
    auto pose = GetDataPtr<RelativePose>(result);
    // ... 使用估计的位姿 ...
}
``` 