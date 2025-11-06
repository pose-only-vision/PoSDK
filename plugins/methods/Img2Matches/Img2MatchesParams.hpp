/**
 * @file Img2MatchesParams.hpp
 * @brief 图像特征匹配插件参数配置系统
 * @copyright Copyright (c) 2024 PoSDK
 */

#pragma once

#include <po_core.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <po_core/po_logger.hpp>

namespace PluginMethods
{
    using namespace PoSDK;
    using namespace Interface;

    // ==================== 匹配器类型枚举 ====================

    /**
     * @brief 匹配器类型枚举
     */
    enum class MatcherType
    {
        FASTCASCADEHASHINGL2, // Fast Cascade Hashing L2匹配器（OpenMVG风格，适用于SIFT等浮点描述子）
        FLANN,                // FLANN匹配器（适用于浮点描述子如SIFT）
        BF,                   // 暴力匹配器（L2距离）
        BF_NORM_L1,           // L1范数暴力匹配器
        BF_HAMMING,           // 汉明距离暴力匹配器（适用于二进制描述子）
        LIGHTGLUE             // LightGlue深度学习匹配器（支持多种特征类型）
    };

    /**
     * @brief 运行模式枚举
     */
    enum class RunMode
    {
        Fast,  // 快速模式
        Viewer // 可视化模式
    };

    /**
     * @brief 数据类型模式枚举 | Data types mode enumeration
     */
    enum class DataTypesMode
    {
        Full,  // 全量模式：所有特征存储在内存中，输出data_features + data_matches | Full mode: all features in memory, output data_features + data_matches
        Single // 单文件模式：流式处理，输出单个DataFeature文件 + data_matches | Single mode: stream processing, output individual DataFeature files + data_matches
    };

    // ==================== SIFT预设配置枚举 ====================

    /**
     * @brief SIFT预设配置类型
     */
    enum class SIFTPreset
    {
        NORMAL, // 标准配置：peak_threshold=0.04, first_octave=0
        HIGH,   // 高质量配置：peak_threshold=0.01, first_octave=0
        ULTRA,  // 超高质量配置：peak_threshold=0.01, first_octave=-1 (上采样)
        CUSTOM  // 自定义配置：使用详细参数
    };

    // ==================== LightGlue配置枚举 ====================

    /**
     * @brief LightGlue支持的特征类型
     */
    enum class LightGlueFeatureType
    {
        SUPERPOINT, // SuperPoint特征提取器（256维）
        DISK,       // DISK特征提取器（128维）
        SIFT,       // SIFT特征提取器（128维）
        ALIKED,     // ALIKED特征提取器（128维）
        DOGHARDNET  // DoGHardNet特征提取器（128维）
    };

    // ==================== 参数结构体定义 ====================

    /**
     * @brief SIFT特征检测器参数
     */
    struct SIFTParameters
    {
        // === OpenCV风格参数 ===
        int nfeatures = 0;                   // 检测特征点数量，0表示不限制
        int nOctaveLayers = 3;               // octave层数（对应OpenMVG的num_scales）
        double contrastThreshold = 0.04;     // 对比度阈值（对应OpenMVG的peak_threshold）
        double edgeThreshold = 10.0;         // 边缘阈值，防止提取边缘特征点
        double sigma = 1.6;                  // 高斯模糊系数，输入图像假定的模糊水平
        bool enable_precise_upscale = false; // 是否启用精确上采样（OpenCV特有）

        // === OpenMVG风格扩展参数 ===
        int first_octave = 0;  // 起始octave层级: -1=上采样, 0=原始图像, 1=下采样
        int num_octaves = 6;   // 最大octave数量，自动根据图像尺寸限制
        bool root_sift = true; // 是否使用RootSIFT归一化（提升匹配性能）

        // === 预设配置支持 ===
        SIFTPreset preset = SIFTPreset::CUSTOM; // SIFT预设配置

        /**
         * @brief 应用SIFT预设配置
         */
        void ApplyPreset()
        {
            switch (preset)
            {
            case SIFTPreset::NORMAL:
                contrastThreshold = 0.04; // 减小此值→检测更多特征点（更敏感）；增大此值→检测更少特征点（更严格）
                first_octave = 0;
                break;
            case SIFTPreset::HIGH:
                contrastThreshold = 0.03; // 较小阈值，检测更多高质量特征点（约2万特征点，与OpenMVG对齐）
                first_octave = 0;
                break;
            case SIFTPreset::ULTRA:
                contrastThreshold = 0.01; // 保持高敏感度，结合上采样检测更多细节特征（约4-6万特征点）
                first_octave = -1;        // 启用上采样：图像放大2倍→检测更多小尺度特征点
                break;
            case SIFTPreset::CUSTOM:
            default:
                // 使用配置文件中的详细参数
                break;
            }
        }
    };

    /**
     * @brief SURF特征检测器参数
     */
    struct SURFParameters
    {
        // === OpenCV SURF参数 ===
        double hessianThreshold = 100.0; // Hessian关键点检测器阈值（越大检测到的特征点越少）
        int nOctaves = 4;                // 金字塔octave数量
        int nOctaveLayers = 3;           // 每个octave内的层数
        bool extended = false;           // 扩展描述子标志（true=128维，false=64维）
        bool upright = false;            // 直立特征标志（true=不计算方向，更快但不具备旋转不变性）

        /**
         * @brief 获取描述子维度
         * @return 64或128
         */
        int GetDescriptorSize() const
        {
            return extended ? 128 : 64;
        }

        /**
         * @brief 是否具备旋转不变性
         * @return true表示具备旋转不变性
         */
        bool IsRotationInvariant() const
        {
            return !upright;
        }
    };

    /**
     * @brief ORB特征提取器参数
     */
    struct ORBParameters
    {
        int nfeatures = 1000;     // 特征点数量，0表示不限制
        double scaleFactor = 1.2; // 金字塔比例因子
        int nlevels = 8;          // 金字塔层数
        int edgeThreshold = 31;   // 边缘阈值
        int firstLevel = 0;       // 第一层级
        int WTA_K = 2;            // 用于计算BRIEF描述子的点对数
        int patchSize = 31;       // 特征点周围区域大小
        int fastThreshold = 20;   // FAST检测器阈值
        // scoreType固定为HARRIS_SCORE，不配置
    };

    /**
     * @brief SuperPoint特征提取器参数
     */
    struct SuperPointParameters
    {
        int max_keypoints = 2048;                  // 最大特征点数量
        double detection_threshold = 0.0005;       // 检测阈值
        int nms_radius = 4;                        // 非极大值抑制半径
        int remove_borders = 4;                    // 移除图像边界的像素数
        std::string python_executable = "python3"; // Python可执行文件路径
    };

    /**
     * @brief LightGlue深度学习匹配器参数
     */
    struct LightGlueParameters
    {
        // === 基础配置 ===
        LightGlueFeatureType feature_type = LightGlueFeatureType::SUPERPOINT; // 特征类型
        int max_num_keypoints = 2048;                                         // 最大特征点数量
        float depth_confidence = 0.95f;                                       // 深度置信度（控制早停）
        float width_confidence = 0.99f;                                       // 宽度置信度（控制点剪枝）
        float filter_threshold = 0.1f;                                        // 匹配置信度阈值

        // === 性能优化 ===
        bool flash_attention = true;  // 是否启用FlashAttention
        bool mixed_precision = false; // 是否启用混合精度
        bool compile_model = false;   // 是否编译模型（PyTorch 2.0+）

        // === 环境配置 ===
        std::string python_executable = "python3"; // Python可执行文件路径
        std::string script_path = "";              // 脚本路径（自动设置）
    };

    /**
     * @brief 基础配置参数
     */
    struct BaseParameters
    {
        std::string profile_commit;                          // 配置修改说明
        bool enable_profiling = false;                       // 是否启用性能分析
        bool enable_evaluator = false;                       // 是否启用评估
        int log_level = 2;                                   // 日志级别: 2=详细, 1=正常, 0=无
        RunMode run_mode = RunMode::Fast;                    // 运行模式
        DataTypesMode data_types_mode = DataTypesMode::Full; // 数据类型模式：Full=全量存储，Single=单文件流式处理 | Data types mode: Full=store all in memory, Single=single file stream processing
        std::string detector_type = "SIFT";                  // 特征检测器类型
        int num_threads = 4;                                 // 多线程数量（特征提取并行化）
    };

    /**
     * @brief 特征输出控制参数
     */
    struct FeatureExportParameters
    {
        bool export_features = true;                      // 是否输出特征文件
        std::string export_fea_path = "storage/features"; // 特征输出路径
    };

    /**
     * @brief 匹配结果导出参数
     */
    struct MatchesExportParameters
    {
        bool export_matches = true;                        // 是否导出匹配结果
        std::string export_match_path = "storage/matches"; // 匹配结果输出路径
    };

    /**
     * @brief FLANN算法类型枚举
     */
    enum class FLANNAlgorithm
    {
        AUTO,      // 自动选择算法
        KDTREE,    // KDTree算法（适用于浮点描述子）
        LSH,       // LSH算法（适用于二进制描述子）
        KMEANS,    // KMeans算法（通用聚类）
        COMPOSITE, // 组合算法
        LINEAR     // 线性搜索
    };

    /**
     * @brief FLANN质量预设枚举
     */
    enum class FLANNPreset
    {
        FAST,     // 快速匹配
        BALANCED, // 平衡质量和速度
        ACCURATE, // 高精度匹配
        CUSTOM    // 完全自定义
    };

    /**
     * @brief KMeans中心初始化方式枚举
     */
    enum class FLANNCentersInit
    {
        CENTERS_RANDOM,   // 随机初始化
        CENTERS_GONZALES, // Gonzales算法
        CENTERS_KMEANSPP  // KMeans++算法
    };

    /**
     * @brief FLANN匹配器参数
     */
    struct FLANNParameters
    {
        // === 控制开关 ===
        bool use_advanced_control = true; // 是否使用高级FLANN参数控制（false=使用OpenCV默认方式）

        // === 算法选择 ===
        FLANNAlgorithm algorithm = FLANNAlgorithm::AUTO; // FLANN算法类型

        // === KDTree算法参数（适用于SIFT/SURF等浮点描述子）===
        int trees = 8; // KDTree数量，范围[1-16]

        // === LSH算法参数（适用于ORB/BRIEF等二进制描述子）===
        int table_number = 12;     // 哈希表数量
        int key_size = 20;         // 哈希键长度
        int multi_probe_level = 2; // 多探测级别

        // === KMeans算法参数（通用聚类算法）===
        int branching = 32;                                               // 分支因子
        int iterations = 11;                                              // 迭代次数
        FLANNCentersInit centers_init = FLANNCentersInit::CENTERS_RANDOM; // 中心初始化方式

        // === 搜索参数（影响搜索精度和速度）===
        int checks = 100;       // 搜索检查次数
        double eps = 0.0;       // 搜索精度，0.0为精确搜索
        bool sorted = true;     // 是否按距离排序结果
        int max_neighbors = -1; // 最大邻居数，-1表示不限制

        // === 质量控制预设 ===
        FLANNPreset preset = FLANNPreset::BALANCED; // FLANN预设配置

        /**
         * @brief 应用FLANN预设配置
         */
        void ApplyPreset()
        {
            switch (preset)
            {
            case FLANNPreset::FAST:
                trees = 4;
                checks = 32;
                table_number = 6;
                key_size = 12;
                multi_probe_level = 1;
                break;
            case FLANNPreset::BALANCED:
                trees = 8;
                checks = 100;
                table_number = 12;
                key_size = 20;
                multi_probe_level = 2;
                break;
            case FLANNPreset::ACCURATE:
                trees = 12;
                checks = 300;
                table_number = 20;
                key_size = 32;
                multi_probe_level = 2;
                break;
            case FLANNPreset::CUSTOM:
            default:
                // 使用配置文件中的详细参数
                break;
            }
        }

        /**
         * @brief 根据描述子类型自动选择最佳算法
         * @param descriptor_type 描述子类型（如"SIFT", "ORB"等）
         */
        void AutoSelectAlgorithm(const std::string &descriptor_type)
        {
            if (algorithm == FLANNAlgorithm::AUTO)
            {
                if (descriptor_type == "SIFT" || descriptor_type == "SURF" || descriptor_type == "KAZE")
                {
                    algorithm = FLANNAlgorithm::KDTREE; // 浮点描述子使用KDTree
                }
                else if (descriptor_type == "ORB" || descriptor_type == "BRIEF" ||
                         descriptor_type == "BRISK" || descriptor_type == "AKAZE")
                {
                    algorithm = FLANNAlgorithm::LSH; // 二进制描述子使用LSH
                }
                else
                {
                    algorithm = FLANNAlgorithm::KDTREE; // 默认使用KDTree
                }
            }
        }
    };

    /**
     * @brief 匹配器配置参数
     */
    struct MatchingParameters
    {
        MatcherType matcher_type = MatcherType::FASTCASCADEHASHINGL2; // 匹配器类型
        bool cross_check = false;                                     // 是否启用交叉检查
        float ratio_thresh = 0.8f;                                    // Lowe's比率测试阈值
        size_t max_matches = 0;                                       // 最大匹配数，0表示不限制
    };

    /**
     * @brief 可视化参数
     */
    struct VisualizationParameters
    {
        size_t show_view_pair_i = 0; // 第一幅图像索引
        size_t show_view_pair_j = 1; // 第二幅图像索引
    };

    /**
     * @brief 总的插件参数容器
     */
    struct Img2MatchesParameters
    {
        BaseParameters base;
        SIFTParameters sift;             // SIFT特征检测器参数
        SURFParameters surf;             // SURF特征检测器参数
        ORBParameters orb;               // ORB特征检测器参数
        SuperPointParameters superpoint; // SuperPoint特征检测器参数
        FLANNParameters flann;           // FLANN匹配器参数
        LightGlueParameters lightglue;   // LightGlue匹配器参数
        FeatureExportParameters feature_export;
        MatchesExportParameters matches_export;
        MatchingParameters matching;
        VisualizationParameters visualization;

        /**
         * @brief 从配置文件加载参数
         * @param config_loader 配置加载器
         */
        void LoadFromConfig(Interface::MethodPreset *config_loader);

        /**
         * @brief 验证参数有效性
         * @param method_ptr 方法指针（用于日志输出）
         * @return 是否有效
         */
        bool Validate(Interface::MethodPreset *method_ptr = nullptr) const;

        /**
         * @brief 打印参数摘要
         * @param method_ptr 方法指针（用于日志输出）
         */
        void PrintSummary(Interface::MethodPreset *method_ptr = nullptr) const;
    };

    // ==================== 参数转换工具 ====================

    /**
     * @brief 参数转换工具类
     */
    class Img2MatchesParameterConverter
    {
    public:
        /**
         * @brief 将参数转换为SetMethodOptions格式
         */
        static std::unordered_map<std::string, std::string> ToMethodOptions(const Img2MatchesParameters &params);

        /**
         * @brief 将匹配器类型枚举转换为字符串
         */
        static std::string MatcherTypeToString(MatcherType type);

        /**
         * @brief 从字符串转换为匹配器类型枚举
         */
        static MatcherType StringToMatcherType(const std::string &str);

        /**
         * @brief 将运行模式枚举转换为字符串
         */
        static std::string RunModeToString(RunMode mode);

        /**
         * @brief 从字符串转换为运行模式枚举
         */
        static RunMode StringToRunMode(const std::string &str);

        /**
         * @brief 将数据类型模式枚举转换为字符串 | Convert data types mode enum to string
         */
        static std::string DataTypesModeToString(DataTypesMode mode);

        /**
         * @brief 从字符串转换为数据类型模式枚举 | Convert string to data types mode enum
         */
        static DataTypesMode StringToDataTypesMode(const std::string &str);

        /**
         * @brief 将SIFT预设枚举转换为字符串
         */
        static std::string SIFTPresetToString(SIFTPreset preset);

        /**
         * @brief 从字符串转换为SIFT预设枚举
         */
        static SIFTPreset StringToSIFTPreset(const std::string &str);

        /**
         * @brief 将FLANN算法枚举转换为字符串
         */
        static std::string FLANNAlgorithmToString(FLANNAlgorithm algorithm);

        /**
         * @brief 从字符串转换为FLANN算法枚举
         */
        static FLANNAlgorithm StringToFLANNAlgorithm(const std::string &str);

        /**
         * @brief 将FLANN预设枚举转换为字符串
         */
        static std::string FLANNPresetToString(FLANNPreset preset);

        /**
         * @brief 从字符串转换为FLANN预设枚举
         */
        static FLANNPreset StringToFLANNPreset(const std::string &str);

        /**
         * @brief 将FLANN中心初始化方式枚举转换为字符串
         */
        static std::string FLANNCentersInitToString(FLANNCentersInit centers_init);

        /**
         * @brief 从字符串转换为FLANN中心初始化方式枚举
         */
        static FLANNCentersInit StringToFLANNCentersInit(const std::string &str);

        /**
         * @brief 将LightGlue特征类型枚举转换为字符串
         */
        static std::string LightGlueFeatureTypeToString(LightGlueFeatureType type);

        /**
         * @brief 从字符串转换为LightGlue特征类型枚举
         */
        static LightGlueFeatureType StringToLightGlueFeatureType(const std::string &str);
    };

} // namespace PluginMethods
