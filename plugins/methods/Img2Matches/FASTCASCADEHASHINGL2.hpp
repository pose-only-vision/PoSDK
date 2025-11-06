#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <vector>
#include <memory>

namespace PluginMethods
{
    // Forward declarations
    struct HashedDescription;
    struct HashedDescriptions;
    class CascadeHasher;

    /**
     * @brief FAST CASCADE HASHING L2匹配器
     *
     * 基于OpenMVG的Cascade Hashing算法实现，专为SIFT等浮点描述子优化
     * 提供比FLANN更快的匹配速度，同时保持良好的匹配精度
     */
    class FastCascadeHashingL2Matcher
    {
    public:
        /**
         * @brief 构造函数
         * @param dist_ratio Lowe's比率测试阈值，用于过滤虚假匹配
         */
        explicit FastCascadeHashingL2Matcher(float dist_ratio = 0.8f);

        /**
         * @brief 析构函数
         */
        ~FastCascadeHashingL2Matcher();

        /**
         * @brief 构建索引
         * @param descriptors 描述子矩阵 (CV_32F类型)
         * @return 是否成功构建索引
         */
        bool BuildIndex(const cv::Mat &descriptors);

        /**
         * @brief 使用距离比率测试进行匹配
         * @param query_descriptors 查询描述子
         * @param matches 输出匹配结果
         * @param cross_check 是否启用交叉检查
         * @return 是否匹配成功
         */
        bool MatchDescriptors(const cv::Mat &query_descriptors,
                              std::vector<cv::DMatch> &matches,
                              bool cross_check = false);

        /**
         * @brief KNN匹配（返回k个最近邻）
         * @param query_descriptors 查询描述子
         * @param matches 输出匹配结果
         * @param k 最近邻数量（通常为2，用于比率测试）
         * @return 是否匹配成功
         */
        bool KnnMatch(const cv::Mat &query_descriptors,
                      std::vector<std::vector<cv::DMatch>> &matches,
                      int k = 2);

        /**
         * @brief 静态方法：两个描述子集合之间的匹配
         * @param descriptors1 第一组描述子
         * @param descriptors2 第二组描述子
         * @param matches 输出匹配结果
         * @param dist_ratio 距离比率阈值
         * @param cross_check 是否启用交叉检查
         * @return 是否匹配成功
         */
        static bool Match(const cv::Mat &descriptors1,
                          const cv::Mat &descriptors2,
                          std::vector<cv::DMatch> &matches,
                          float dist_ratio = 0.8f,
                          bool cross_check = false);

        /**
         * @brief 获取匹配器名称
         */
        static std::string GetMatcherName() { return "FASTCASCADEHASHINGL2"; }

        /**
         * @brief 检查描述子类型是否兼容
         * @param descriptors 描述子矩阵
         * @return 是否兼容（必须是CV_32F类型）
         */
        static bool IsCompatible(const cv::Mat &descriptors);

    private:
        float dist_ratio_;                                    // 距离比率阈值
        std::unique_ptr<CascadeHasher> cascade_hasher_;       // Cascade Hashing核心算法
        std::unique_ptr<HashedDescriptions> hashed_database_; // 哈希化的数据库描述子
        cv::Mat database_descriptors_;                        // 原始数据库描述子
        cv::Mat zero_mean_descriptor_;                        // 零均值描述子

        /**
         * @brief 计算描述子的零均值向量
         * @param descriptors 输入描述子
         * @return 零均值向量
         */
        cv::Mat ComputeZeroMeanDescriptor(const cv::Mat &descriptors);

        /**
         * @brief 应用距离比率测试过滤匹配
         * @param raw_matches 原始匹配结果
         * @param distances 对应的距离
         * @param filtered_matches 过滤后的匹配结果
         */
        void ApplyDistanceRatioFilter(const std::vector<cv::DMatch> &raw_matches,
                                      const std::vector<float> &distances,
                                      std::vector<cv::DMatch> &filtered_matches);

        /**
         * @brief 移除重复的匹配
         * @param matches 输入输出匹配结果
         */
        void RemoveDuplicateMatches(std::vector<cv::DMatch> &matches);
    };

    /**
     * @brief Cascade Hashing核心算法类
     *
     * 实现OpenMVG风格的Cascade Hashing算法
     * 用于快速近似最近邻搜索
     */
    class CascadeHasher
    {
    public:
        CascadeHasher();
        ~CascadeHasher();

        /**
         * @brief 初始化哈希器 (与OpenMVG完全兼容的接口)
         * @param descriptor_length 描述子维度（哈希码维度将等于此值）
         * @param nb_bucket_groups bucket组数（默认6）
         * @param nb_bits_per_bucket 每个bucket的比特数（默认10）
         * @param random_seed 随机种子
         * @return 是否初始化成功
         *
         * 注意：这个接口与OpenMVG完全一致
         */
        bool Init(
            const int descriptor_length,
            const uint8_t nb_bucket_groups = 6,
            const uint8_t nb_bits_per_bucket = 10,
            const unsigned random_seed = 5489U); // 与OpenMVG一致的默认种子

        /**
         * @brief 创建描述子的哈希表示
         * @param descriptors 输入描述子矩阵
         * @param zero_mean_descriptor 零均值描述子
         * @return 哈希化的描述子集合
         */
        std::unique_ptr<HashedDescriptions> CreateHashedDescriptions(
            const cv::Mat &descriptors,
            const cv::Mat &zero_mean_descriptor);

        /**
         * @brief 在哈希描述子中进行匹配搜索
         * @param hashed_database 哈希化的数据库
         * @param database_descriptors 原始数据库描述子
         * @param hashed_query 哈希化的查询描述子
         * @param query_descriptors 原始查询描述子
         * @param matches 输出匹配索引
         * @param distances 输出距离
         * @param NN 最近邻数量
         */
        void MatchHashedDescriptions(
            const HashedDescriptions &hashed_database,
            const cv::Mat &database_descriptors,
            const HashedDescriptions &hashed_query,
            const cv::Mat &query_descriptors,
            std::vector<cv::DMatch> &matches,
            std::vector<float> &distances,
            int NN = 2);

        /**
         * @brief 计算零均值描述子
         * @param descriptors 输入描述子
         * @return 零均值向量
         */
        static cv::Mat GetZeroMeanDescriptor(const cv::Mat &descriptors);

    private:
        // Cascade Hashing参数 (与OpenMVG完全一致)
        int nb_bits_per_bucket_;   // 每个bucket的比特数
        int nb_hash_code_;         // 哈希码数量
        int nb_bucket_groups_;     // bucket组数量
        int nb_buckets_per_group_; // 每组的bucket数量

        // 哈希投影矩阵
        cv::Mat primary_hash_projection_;                // 主要哈希投影
        std::vector<cv::Mat> secondary_hash_projection_; // 次要哈希投影

        // 私有方法
        void InitializeProjectionMatrices(unsigned random_seed);
        void ComputeHashCodes(const cv::Mat &descriptors,
                              const cv::Mat &zero_mean_descriptor,
                              HashedDescriptions &hashed_desc);
        void BuildBuckets(HashedDescriptions &hashed_desc);
    };

} // namespace PluginMethods
