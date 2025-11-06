#pragma once

// 首先包含OpenMVG的转换器，这会引入Eigen的STL容器特化
#include <common/converter/converter_openmvg_file.hpp>
#include <po_core.hpp>
#include <filesystem>
#include <po_core/po_logger.hpp>
// OpenMVG头文件

namespace PoSDKPlugin
{

    using namespace PoSDK;
    using namespace Interface;
    using namespace types;
    using namespace Converter;

    /**
     * @brief OpenMVG预处理插件
     * @details 使用OpenMVG工具链执行完整的SfM管道，流程包括：
     *          1. SfMInit_ImageListing: 初始化SfM数据和图像列表
     *          2. ComputeFeatures: 提取特征点和描述子
     *          3. ComputeMatches: 计算特征匹配
     *          4. GeometricFilter: 几何过滤匹配对
     *          5. PairGenerator: 生成图像对列表（可选）
     *          6. SfM: 三维重建（可选，支持INCREMENTAL/GLOBAL/STELLAR引擎）
     *          7. ComputeSfM_DataColor: 点云着色（可选）
     *          8. EvalQuality: 质量评估与真值对比（可选，支持Strecha等数据集）
     *
     *          基础功能输出data_images，data_features和data_matches三种数据
     *          完整管道可生成三维重建结果、着色点云和质量评估报告
     */
    class OpenMVGPipeline : public Interface::MethodPresetProfiler
    {
    public:
        OpenMVGPipeline();
        ~OpenMVGPipeline() override = default;

        // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
        const std::string &GetType() const override;

        DataPtr Run() override;

    private:
        /**
         * @brief 检查OpenMVG二进制文件是否可执行
         * @param bin_path 二进制文件路径
         * @return 是否可执行
         */
        bool CheckOpenMVGBinary(const std::string &bin_path) const;

        /**
         * @brief 自动检测OpenMVG二进制目录路径
         * @return OpenMVG bin目录路径，失败时返回空字符串
         */
        std::string DetectOpenMVGBinPath() const;

        /**
         * @brief 创建工作目录
         * @return 是否成功
         */
        bool CreateWorkDirectories() const;

        /**
         * @brief 运行SfMInit_ImageListing
         * @return 是否成功
         */
        bool RunSfMInitImageListing();

        /**
         * @brief 运行ComputeFeatures
         * @return 是否成功
         */
        bool RunComputeFeatures();

        /**
         * @brief 运行ComputeMatches
         * @return 是否成功
         */
        bool RunComputeMatches();

        /**
         * @brief 运行GeometricFilter
         * @return 是否成功
         */
        bool RunGeometricFilter();

        /**
         * @brief 运行PairGenerator（可选步骤）
         * @return 是否成功
         */
        bool RunPairGenerator();

        /**
         * @brief 运行SfM重建
         * @return 是否成功
         */
        bool RunSfM();

        /**
         * @brief 运行ComputeSfM_DataColor点云着色
         * @return 是否成功
         */
        bool RunComputeSfM_DataColor();

        /**
         * @brief 运行EvalQuality质量评估（与真值对比）
         * @return 是否成功
         */
        bool RunEvalQuality();

    private:
        // OpenMVG二进制文件目录
        std::string bin_folder_;
        // 工作目录
        std::string work_dir_;
        // 临时OpenMVG匹配目录
        std::string matches_dir_;
        // 图像目录
        std::string images_dir_;
        // 图像源文件夹
        std::string images_folder_;
        // sfm_data文件路径
        std::string sfm_data_path_;
        // 最终匹配文件路径
        std::string final_matches_path_;
        // 推测匹配文件的完整路径
        std::string putative_matches_path_;
        // 原始图像文件路径列表
        std::vector<std::string> image_paths_;
        // pairs文件路径
        std::string pairs_path_;
        // SfM重建输出目录
        std::string reconstruction_dir_;
        // 最终SfM数据文件路径
        std::string final_sfm_data_path_;
        // 着色点云文件路径
        std::string colored_ply_path_;
        // 质量评估输出目录
        std::string eval_output_dir_;

    private:
    };

} // namespace PoSDKPlugin
