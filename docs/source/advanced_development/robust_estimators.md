# Robust Estimator Framework

PoMVG provides a robust estimator framework that currently supports RANSAC and GNC-IRLS.

## `DataSample<TSample>` Data Container

- **Purpose**: Manages sample data for robust estimation, particularly supporting efficient subset sampling without copying original data (zero-copy).
- **Template Parameter `TSample`**: Must be of type `std::vector<ValueType>`, where `ValueType` is the type of a single element in the sample (e.g., `BearingPair`).
- **Core Features**:
  - **Zero-copy Subsets**: `GetRandomSubset(size)` and `GetSubset(indices)` return a new `DataSample` object that shares the original data (`data_map_ptr_`), but stores subset indices through `subset_indices_`.
  - **STL-Compatible Interface**: Provides `begin()`, `end()`, `operator[]`, `size()`, `empty()` and other interfaces, allowing sample data (whether full sample or subset) to be accessed like `std::vector`. Iterators and `operator[]` automatically handle index mapping.
- **Usage**:
  - Typically used as input data type for `RobustEstimator`.
  - `RobustEstimator` internally uses `GetRandomSubset` to obtain random samples for model estimation.
  - `GetInlierSubset` is used to obtain inlier subsets for final model optimization.
- **Example**:
  ```cpp
  // Assume we have original data std::vector<MySampleType> original_data;
  auto data_sample = std::make_shared<DataSample<std::vector<MySampleType>>>(original_data);

  // Get random subset
  auto random_subset = data_sample->GetRandomSubset(10);
  std::cout << "Random subset size: " << random_subset->size() << std::endl;
  if (!random_subset->empty()) {
      // Access subset elements
      MySampleType first_element = (*random_subset)[0];
      // Iterate subset
      for (const auto& element : *random_subset) {
          // ... process element ...
      }
  }
  ```

## `RobustEstimator` Base Class

- **Inheritance**: `RobustEstimator` inherits from `MethodPresetProfiler`. It is a non-template class.
- **Core Components**:
  - **Model Estimator (`model_estimator_ptr_`)**: Inherits from `MethodPreset`, responsible for estimating model parameters from a minimal sample set (e.g., LiRP method estimates relative pose). Its type is specified via the `model_estimator_type` configuration option.
  - **Cost Evaluator (`cost_evaluator_ptr_`)**: Inherits from `MethodPreset`, responsible for calculating residuals (costs) of each sample point relative to a given model. Its type is specified via the `cost_evaluator_type` configuration option.
- **Public API**:
  - `RobustEstimator()`: Constructor.
  - `void Initialize()`: Initializes the estimator, creating internal model estimator and cost evaluator.
  - `void SetModelEstimatorOptions(const MethodOptions &options)`: Sets parameters for the internal model estimator.
  - `void SetCostEvaluatorOptions(const MethodOptions &options)`: Sets parameters for the internal cost evaluator.
  - `DataPtr Build(const DataPtr &material_ptr = nullptr)`: Executes the robust estimation process by initializing the estimator and calling `Run`.
- **Protected API**:
  - `DataPtr GetPopulationPtr() const`: Retrieves population sample data from `required_package_`.
  - `void CheckParameterConsistency()`: Checks parameter consistency between model estimator and cost evaluator (e.g., `identify_mode` and `residual_type`).
  - `std::shared_ptr<std::vector<size_t>> GetBestInliers() const`: Returns the best inlier indices.
- **Protected Members**:
  - `std::vector<size_t> best_inliers_`: Stores inlier indices.
  - `MethodPresetPtr model_estimator_ptr_`: Model estimator pointer.
  - `MethodPresetPtr cost_evaluator_ptr_`: Cost evaluator pointer.
  - `MethodsConfig method_configs_`: Stores parameter configurations for estimators (`model_estimator_type`, `cost_evaluator_type`).
- **Workflow**:
  1. Create `RobustEstimator` instance (e.g., `RANSACEstimator` or `GNCIRLSEstimator`) via `FactoryMethod::Create`.
  2. Set `model_estimator_type` and `cost_evaluator_type` options.
  3. (Optional) Set specific parameters for internal model estimator and cost evaluator via `SetModelEstimatorOptions` and `SetCostEvaluatorOptions`.
  4. Set input data containing `DataSample` via `SetRequiredData`.
  5. Call `Build()` to start robust estimation process.
- **Derived Classes**:
  - `RANSACEstimator`: Implements standard RANSAC algorithm.
  - `GNCIRLSEstimator`: Implements GNC-IRLS algorithm, typically more robust but computationally more expensive.
- **Configuration**: Configure robust estimator parameters (such as iteration count, confidence, inlier threshold) and internal model estimator and cost evaluator types via `.ini` files or `SetMethodOptions`.

## Usage Example

```cpp
// 1. Create RANSAC estimator instance
auto ransac_estimator = std::dynamic_pointer_cast<RobustEstimator>(
    FactoryMethod::Create("ransac_estimator")
);
ASSERT_TRUE(ransac_estimator != nullptr);

// 2. Set RANSAC parameters
MethodOptions ransac_options {
    {"model_estimator_type", "method_LiRP"},         // Use LiRP to estimate model
    {"cost_evaluator_type", "method_relative_cost"}, // Use relative pose cost evaluation
    {"max_iterations", "1000"},
    {"confidence", "0.99"},
    {"inlier_threshold", "1e-4"}, // RANSAC inlier threshold
    {"min_sample_size", "8"}      // LiRP requires 8 point pairs
};
ransac_estimator->SetMethodOptions(ransac_options);

// 3. (Optional) Set specific parameters for LiRP and Cost Evaluator
MethodOptions lirp_options { {"identify_mode", "PPO"} /* ... other LiRP options ... */ };
ransac_estimator->SetModelEstimatorOptions(lirp_options);

MethodOptions cost_options { {"residual_type", "sampson"} /* ... other Cost options ... */ };
ransac_estimator->SetCostEvaluatorOptions(cost_options);

// 4. Prepare input data (DataSample<BearingPairs>)
BearingPairs bearing_pairs_data = GenerateBearingPairs(); // Assume function generates data
auto sample_data = std::make_shared<DataSample<BearingPairs>>(bearing_pairs_data);

// 5. Set input data
DataPackage input_package;
input_package.AddData("data_sample", sample_data); // RobustEstimator requires data_sample
// If model_estimator or cost_evaluator requires other data (e.g., camera model), also add to package
// input_package.AddData("data_camera_models", camera_model_data);
ransac_estimator->SetRequiredData(input_package); // Or pass directly to Build

// 6. Execute RANSAC
auto result = ransac_estimator->Build();

// 7. Process result (typically DataMap<RelativePose>)
if (result) {
    auto pose = GetDataPtr<RelativePose>(result);
    // ... use estimated pose ...
}
```
