#pragma once

#include <po_core.hpp>
#include <po_core/po_logger.hpp>
#include <filesystem>
#include <converter/converter_colmap_file.hpp>
// OpenMVG头文件

namespace PoSDKPlugin
{

    using namespace PoSDK;
    using namespace Interface;
    using namespace types;

    /**
     * @brief Glomap预处理插件
     * @details 使用Glomap工具链执行完整的SfM管道，流程包括：
     *          1. colmap_pipeline.py 使用colmap重建sfm_data.db
     *          2. glomap_pipeline.py 使用glomap进行重建
     *          3. export_global_poses_from_model.py 导出全局位姿
     *
     *          基础功能输出data_matches，global_poses两种数据
     */

    class GlomapPreprocess : public Interface::MethodPresetProfiler
    {
    public:
        GlomapPreprocess();
        ~GlomapPreprocess() override = default;

        // ✨ GetType() is automatically implemented by REGISTRATION_PLUGIN macro
        // ✨ GetType() 由 REGISTRATION_PLUGIN 宏自动实现
        const std::string &GetType() const override;

        DataPtr Run() override;

    private:
        /**
         * @brief 检查Glomap依赖库是否可执行
         * @param bin_path 二进制文件路径
         * @return 是否可执行
         */
        bool CheckGlomapBinary(const std::string &bin_path) const;

        /**
         * @brief 自动检测Glomap和OpenMVG二进制目录路径
         * @return bin目录路径，失败时返回空字符串
         */
        std::string DetectGlomapBinPath() const;
        std::string DetectColmapBinPath() const;
        std::string DetectOpenMVGBinPath() const;

        bool CheckColmapBinary(const std::string &bin_path) const;
        /**
         * @brief 创建工作目录
         * @return 是否成功
         */
        bool CreateWorkDirectories() const;

        /**
         * @brief 运行ColmapPipeline
         * @return 是否成功
         */
        bool RunColmapPipeline();

        /**
         * @brief 运行GlomapPipeline
         * @return 是否成功
         */
        bool RunGlomapPipeline();

        /**
         * @brief 运行ExportMatchesFromDB
         * @return 是否成功
         */
        bool RunExportMatchesFromDB();

        /**
         * @brief 运行ExportGlobalPosesFromModel
         * @return 是否成功
         */
        bool RunExportGlobalPosesFromModel();

        /**
         * @brief 运行EvalQuality质量评估（与真值对比）
         * @return 是否成功
         */
        bool RunEvalQuality();

        bool RunSfMInitImageListing();

    private:
        // glomap二进制文件目录
        std::string glomap_bin_folder_;
        // colmap二进制文件目录
        std::string colmap_bin_folder_;
        // OpenMVG二进制文件目录
        std::string OpenMVG_bin_folder_;
        // 工作目录
        std::string work_dir_;
        // 临时OpenMVG匹配目录
        std::string matches_dir_;
        // 图像目录
        std::string images_dir_;
        // 图像源文件夹
        std::string images_folder_;
        // sfm_json文件路径
        std::string sfm_json_path_;
        // sfm_db文件路径
        std::string sfm_db_path_;
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
    };

} // namespace PoSDKPlugin
