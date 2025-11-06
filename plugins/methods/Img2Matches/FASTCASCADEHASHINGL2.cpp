#include "FASTCASCADEHASHINGL2.hpp"
#include <random>
#include <bitset>
#include <unordered_set>
#include <algorithm>
#include <cmath>

namespace PluginMethods
{
    // ===== 哈希描述子数据结构 =====

    /**
     * @brief 单个描述子的哈希表示
     */
    struct HashedDescription
    {
        std::vector<uint64_t> hash_code;  // 哈希码（使用位操作优化）
        std::vector<uint16_t> bucket_ids; // 每个bucket组中的bucket ID
    };

    /**
     * @brief 描述子集合的哈希表示
     */
    struct HashedDescriptions
    {
        std::vector<HashedDescription> hashed_desc; // 哈希描述子列表

        using Bucket = std::vector<int>;
        // buckets[bucket_group][bucket_id] = bucket (包含描述子ID的容器)
        std::vector<std::vector<Bucket>> buckets;
    };

    // ===== FastCascadeHashingL2Matcher 实现 =====

    FastCascadeHashingL2Matcher::FastCascadeHashingL2Matcher(float dist_ratio)
        : dist_ratio_(dist_ratio), cascade_hasher_(std::make_unique<CascadeHasher>()), hashed_database_(nullptr)
    {
    }

    FastCascadeHashingL2Matcher::~FastCascadeHashingL2Matcher() = default;

    bool FastCascadeHashingL2Matcher::IsCompatible(const cv::Mat &descriptors)
    {
        return descriptors.type() == CV_32F && descriptors.rows > 0 && descriptors.cols > 0;
    }

    bool FastCascadeHashingL2Matcher::BuildIndex(const cv::Mat &descriptors)
    {
        if (!IsCompatible(descriptors))
        {
            return false;
        }

        database_descriptors_ = descriptors.clone();

        // 初始化Cascade Hasher (与OpenMVG完全一致的调用方式)
        if (!cascade_hasher_->Init(descriptors.cols))
        {
            return false;
        }

        // 计算零均值描述子
        zero_mean_descriptor_ = cascade_hasher_->GetZeroMeanDescriptor(descriptors);

        // 创建哈希描述子
        hashed_database_ = cascade_hasher_->CreateHashedDescriptions(descriptors, zero_mean_descriptor_);

        return hashed_database_ != nullptr;
    }

    bool FastCascadeHashingL2Matcher::MatchDescriptors(const cv::Mat &query_descriptors,
                                                       std::vector<cv::DMatch> &matches,
                                                       bool cross_check)
    {
        if (!hashed_database_ || !IsCompatible(query_descriptors))
        {
            return false;
        }

        matches.clear();

        // 创建查询描述子的哈希表示
        auto hashed_query = cascade_hasher_->CreateHashedDescriptions(query_descriptors, zero_mean_descriptor_);
        if (!hashed_query)
        {
            return false;
        }

        // 执行哈希匹配
        std::vector<float> distances;
        cascade_hasher_->MatchHashedDescriptions(*hashed_database_, database_descriptors_,
                                                 *hashed_query, query_descriptors,
                                                 matches, distances, 2);

        // 应用距离比率测试
        std::vector<cv::DMatch> filtered_matches;
        ApplyDistanceRatioFilter(matches, distances, filtered_matches);

        // 移除重复匹配
        RemoveDuplicateMatches(filtered_matches);

        matches = std::move(filtered_matches);

        // 如果启用交叉检查，进行反向匹配验证
        if (cross_check && !matches.empty())
        {
            std::vector<cv::DMatch> reverse_matches;
            auto hashed_database_query = cascade_hasher_->CreateHashedDescriptions(database_descriptors_, zero_mean_descriptor_);

            std::vector<float> reverse_distances;
            cascade_hasher_->MatchHashedDescriptions(*hashed_database_query, query_descriptors,
                                                     *hashed_database_, database_descriptors_,
                                                     reverse_matches, reverse_distances, 2);

            // 保留双向匹配的结果
            std::vector<cv::DMatch> cross_checked_matches;
            for (const auto &match : matches)
            {
                for (const auto &reverse_match : reverse_matches)
                {
                    if (match.queryIdx == reverse_match.trainIdx &&
                        match.trainIdx == reverse_match.queryIdx)
                    {
                        cross_checked_matches.push_back(match);
                        break;
                    }
                }
            }
            matches = std::move(cross_checked_matches);
        }

        return true;
    }

    bool FastCascadeHashingL2Matcher::KnnMatch(const cv::Mat &query_descriptors,
                                               std::vector<std::vector<cv::DMatch>> &matches,
                                               int k)
    {
        if (!hashed_database_ || !IsCompatible(query_descriptors))
        {
            return false;
        }

        matches.clear();
        matches.resize(query_descriptors.rows);

        // 创建查询描述子的哈希表示
        auto hashed_query = cascade_hasher_->CreateHashedDescriptions(query_descriptors, zero_mean_descriptor_);
        if (!hashed_query)
        {
            return false;
        }

        // 对每个查询描述子进行匹配
        for (int i = 0; i < query_descriptors.rows; ++i)
        {
            cv::Mat single_query = query_descriptors.row(i);
            auto single_hashed_query = cascade_hasher_->CreateHashedDescriptions(single_query, zero_mean_descriptor_);

            std::vector<cv::DMatch> single_matches;
            std::vector<float> distances;
            cascade_hasher_->MatchHashedDescriptions(*hashed_database_, database_descriptors_,
                                                     *single_hashed_query, single_query,
                                                     single_matches, distances, k);

            // 调整匹配索引
            for (auto &match : single_matches)
            {
                match.queryIdx = i;
            }

            matches[i] = std::move(single_matches);
        }

        return true;
    }

    bool FastCascadeHashingL2Matcher::Match(const cv::Mat &descriptors1,
                                            const cv::Mat &descriptors2,
                                            std::vector<cv::DMatch> &matches,
                                            float dist_ratio,
                                            bool cross_check)
    {
        FastCascadeHashingL2Matcher matcher(dist_ratio);

        if (!matcher.BuildIndex(descriptors2))
        {
            return false;
        }

        return matcher.MatchDescriptors(descriptors1, matches, cross_check);
    }

    cv::Mat FastCascadeHashingL2Matcher::ComputeZeroMeanDescriptor(const cv::Mat &descriptors)
    {
        return CascadeHasher::GetZeroMeanDescriptor(descriptors);
    }

    void FastCascadeHashingL2Matcher::ApplyDistanceRatioFilter(const std::vector<cv::DMatch> &raw_matches,
                                                               const std::vector<float> &distances,
                                                               std::vector<cv::DMatch> &filtered_matches)
    {
        filtered_matches.clear();

        // 使用与OpenMVG完全一致的NNdistanceRatio算法
        const int NN = 2; // 固定使用2个最近邻
        std::vector<int> ratio_ok_indices;

        // OpenMVG的NNdistanceRatio逻辑：对每NN个距离进行比率测试
        ratio_ok_indices.clear();
        ratio_ok_indices.reserve(distances.size() / NN);

        for (size_t i = 0; i < distances.size(); i += NN)
        {
            if (i + 1 < distances.size())
            {
                float dist1 = distances[i];
                float dist2 = distances[i + 1];

                // 与OpenMVG一致的比率测试：dist1 < dist_ratio_ * dist2
                if (dist1 < dist_ratio_ * dist2)
                {
                    ratio_ok_indices.push_back(static_cast<int>(i / NN));
                }
            }
        }

        // 构建过滤后的匹配结果
        filtered_matches.reserve(ratio_ok_indices.size());
        for (const int idx : ratio_ok_indices)
        {
            const int match_idx = idx * NN; // 对应的原始匹配索引
            if (match_idx < raw_matches.size())
            {
                cv::DMatch match = raw_matches[match_idx];
                match.distance = distances[match_idx];
                filtered_matches.push_back(match);
            }
        }
    }

    void FastCascadeHashingL2Matcher::RemoveDuplicateMatches(std::vector<cv::DMatch> &matches)
    {
        std::sort(matches.begin(), matches.end(),
                  [](const cv::DMatch &a, const cv::DMatch &b)
                  {
                      if (a.queryIdx != b.queryIdx)
                          return a.queryIdx < b.queryIdx;
                      if (a.trainIdx != b.trainIdx)
                          return a.trainIdx < b.trainIdx;
                      return a.distance < b.distance;
                  });

        auto new_end = std::unique(matches.begin(), matches.end(),
                                   [](const cv::DMatch &a, const cv::DMatch &b)
                                   {
                                       return a.queryIdx == b.queryIdx && a.trainIdx == b.trainIdx;
                                   });
        matches.erase(new_end, matches.end());
    }

    // ===== CascadeHasher 实现 =====

    CascadeHasher::CascadeHasher()
        : nb_bits_per_bucket_(0), nb_hash_code_(0), nb_bucket_groups_(0), nb_buckets_per_group_(0)
    {
        // 参数将通过Init函数设置，与OpenMVG的行为完全一致
    }

    CascadeHasher::~CascadeHasher() = default;

    bool CascadeHasher::Init(
        const int descriptor_length,
        const uint8_t nb_bucket_groups,
        const uint8_t nb_bits_per_bucket,
        const unsigned random_seed)
    {
        // 与OpenMVG完全一致的参数设置
        nb_bucket_groups_ = nb_bucket_groups;
        nb_hash_code_ = descriptor_length; // 哈希码维度等于描述子维度
        nb_bits_per_bucket_ = nb_bits_per_bucket;
        nb_buckets_per_group_ = 1 << nb_bits_per_bucket; // 2^nb_bits_per_bucket

        InitializeProjectionMatrices(random_seed);
        return true;
    }

    void CascadeHasher::InitializeProjectionMatrices(unsigned random_seed)
    {
        // 使用与OpenMVG完全一致的随机数生成
        std::mt19937 gen(random_seed);
        std::normal_distribution<float> normal_dist(0.0f, 1.0f);

        // 初始化主要哈希投影矩阵 (与OpenMVG完全一致: nb_hash_code × nb_hash_code)
        primary_hash_projection_ = cv::Mat::zeros(nb_hash_code_, nb_hash_code_, CV_32F);
        for (int i = 0; i < nb_hash_code_; ++i)
        {
            for (int j = 0; j < nb_hash_code_; ++j)
            {
                primary_hash_projection_.at<float>(i, j) = normal_dist(gen);
            }
        }

        // 初始化次要哈希投影矩阵 (与OpenMVG完全一致: nb_bits_per_bucket × nb_hash_code)
        secondary_hash_projection_.resize(nb_bucket_groups_);
        for (int g = 0; g < nb_bucket_groups_; ++g)
        {
            secondary_hash_projection_[g] = cv::Mat::zeros(nb_bits_per_bucket_, nb_hash_code_, CV_32F);
            for (int i = 0; i < nb_bits_per_bucket_; ++i)
            {
                for (int j = 0; j < nb_hash_code_; ++j)
                {
                    secondary_hash_projection_[g].at<float>(i, j) = normal_dist(gen);
                }
            }
        }
    }

    cv::Mat CascadeHasher::GetZeroMeanDescriptor(const cv::Mat &descriptors)
    {
        if (descriptors.empty() || descriptors.rows == 0)
        {
            return cv::Mat();
        }

        // 与OpenMVG的colwise().mean()逻辑完全一致
        cv::Mat zero_mean = cv::Mat::zeros(1, descriptors.cols, CV_32F);
        cv::reduce(descriptors, zero_mean, 0, cv::REDUCE_AVG); // 按行计算平均值
        return zero_mean;
    }

    std::unique_ptr<HashedDescriptions> CascadeHasher::CreateHashedDescriptions(
        const cv::Mat &descriptors,
        const cv::Mat &zero_mean_descriptor)
    {
        if (descriptors.empty() || zero_mean_descriptor.empty())
        {
            return nullptr;
        }

        auto hashed_desc = std::make_unique<HashedDescriptions>();
        hashed_desc->hashed_desc.resize(descriptors.rows);

        // 计算每个描述子的哈希码
        ComputeHashCodes(descriptors, zero_mean_descriptor, *hashed_desc);

        // 构建bucket结构
        BuildBuckets(*hashed_desc);

        return hashed_desc;
    }

    void CascadeHasher::ComputeHashCodes(const cv::Mat &descriptors,
                                         const cv::Mat &zero_mean_descriptor,
                                         HashedDescriptions &hashed_desc)
    {
        for (int desc_idx = 0; desc_idx < descriptors.rows; ++desc_idx)
        {
            cv::Mat desc = descriptors.row(desc_idx) - zero_mean_descriptor;
            HashedDescription &hash_desc = hashed_desc.hashed_desc[desc_idx];

            // 计算主要哈希码 (与OpenMVG算法完全一致)
            cv::Mat primary_hash = primary_hash_projection_ * desc.t();

            // 与OpenMVG完全一致的哈希码存储方式
            hash_desc.hash_code.resize(nb_hash_code_);

            for (int i = 0; i < nb_hash_code_; ++i)
            {
                hash_desc.hash_code[i] = primary_hash.at<float>(i, 0) > 0.0f; // 与OpenMVG逻辑完全一致
            }

            // 计算bucket ID (与OpenMVG算法完全一致)
            hash_desc.bucket_ids.resize(nb_bucket_groups_);
            for (int g = 0; g < nb_bucket_groups_; ++g)
            {
                cv::Mat secondary_hash = secondary_hash_projection_[g] * desc.t();
                uint16_t bucket_id = 0;

                // 使用左移位操作构建bucket_id (与OpenMVG一致)
                for (int b = 0; b < nb_bits_per_bucket_; ++b)
                {
                    bucket_id = (bucket_id << 1) + (secondary_hash.at<float>(b, 0) > 0.0f ? 1 : 0);
                }
                hash_desc.bucket_ids[g] = bucket_id;
            }
        }
    }

    void CascadeHasher::BuildBuckets(HashedDescriptions &hashed_desc)
    {
        hashed_desc.buckets.resize(nb_bucket_groups_);
        for (int g = 0; g < nb_bucket_groups_; ++g)
        {
            hashed_desc.buckets[g].resize(nb_buckets_per_group_);
        }

        // 将描述子分配到相应的bucket中
        for (int desc_idx = 0; desc_idx < hashed_desc.hashed_desc.size(); ++desc_idx)
        {
            const HashedDescription &hash_desc = hashed_desc.hashed_desc[desc_idx];
            for (int g = 0; g < nb_bucket_groups_; ++g)
            {
                uint16_t bucket_id = hash_desc.bucket_ids[g];
                hashed_desc.buckets[g][bucket_id].push_back(desc_idx);
            }
        }
    }

    void CascadeHasher::MatchHashedDescriptions(
        const HashedDescriptions &hashed_database,
        const cv::Mat &database_descriptors,
        const HashedDescriptions &hashed_query,
        const cv::Mat &query_descriptors,
        std::vector<cv::DMatch> &matches,
        std::vector<float> &distances,
        int NN)
    {
        matches.clear();
        distances.clear();

        for (int query_idx = 0; query_idx < hashed_query.hashed_desc.size(); ++query_idx)
        {
            const HashedDescription &query_hash = hashed_query.hashed_desc[query_idx];
            cv::Mat query_desc = query_descriptors.row(query_idx);

            // 收集候选描述子 (与OpenMVG一致的逻辑)
            std::vector<int> candidate_descriptors;
            candidate_descriptors.reserve(hashed_database.hashed_desc.size());

            // 使用used_descriptor来避免重复候选者 (与OpenMVG一致)
            static std::vector<bool> used_descriptor;
            used_descriptor.assign(hashed_database.hashed_desc.size(), false);

            for (int g = 0; g < nb_bucket_groups_; ++g)
            {
                uint16_t bucket_id = query_hash.bucket_ids[g];
                const auto &bucket = hashed_database.buckets[g][bucket_id];
                for (int desc_idx : bucket)
                {
                    candidate_descriptors.push_back(desc_idx);
                    used_descriptor[desc_idx] = false;
                }
            }

            // 跳过匹配如果候选数量不足 (与OpenMVG逻辑一致)
            static const int NN = 2; // 最近邻数量
            if (candidate_descriptors.size() <= NN)
            {
                continue;
            }

            // 1. 使用汉明距离进行快速候选过滤 (与OpenMVG完全一致)
            std::vector<std::pair<int, int>> hamming_candidates; // <hamming_distance, candidate_idx>
            for (int candidate_idx : candidate_descriptors)
            {
                // 避免选择同一候选者多次 (与OpenMVG一致)
                if (!used_descriptor[candidate_idx])
                {
                    used_descriptor[candidate_idx] = true;

                    const HashedDescription &candidate_hash = hashed_database.hashed_desc[candidate_idx];

                    // 计算汉明距离 (与OpenMVG的实现完全一致)
                    int hamming_distance = 0;
                    for (size_t i = 0; i < query_hash.hash_code.size(); ++i)
                    {
                        if (query_hash.hash_code[i] != candidate_hash.hash_code[i])
                        {
                            hamming_distance++;
                        }
                    }
                    hamming_candidates.push_back({hamming_distance, candidate_idx});
                }
            }

            // 按汉明距离排序，只保留前kNumTopCandidates个候选者
            static const int kNumTopCandidates = 10; // 与OpenMVG一致
            std::sort(hamming_candidates.begin(), hamming_candidates.end());
            int num_hamming_candidates = std::min(kNumTopCandidates, static_cast<int>(hamming_candidates.size()));

            // 2. 对过滤后的候选者使用L2距离进行精确匹配 (与OpenMVG完全一致)
            std::vector<std::pair<float, int>> candidate_euclidean_distances;
            candidate_euclidean_distances.reserve(kNumTopCandidates);

            for (int i = 0; i < num_hamming_candidates; ++i)
            {
                int candidate_idx = hamming_candidates[i].second;
                cv::Mat candidate_desc = database_descriptors.row(candidate_idx);
                cv::Mat diff = query_desc - candidate_desc;
                float distance = cv::norm(diff, cv::NORM_L2);
                candidate_euclidean_distances.push_back({distance, candidate_idx});
            }

            // 确保每个查询至少有NN个检索邻居 (与OpenMVG一致)
            if (candidate_euclidean_distances.size() >= NN)
            {
                // 找到基于欧几里得距离的前NN个候选者 (与OpenMVG一致)
                std::partial_sort(candidate_euclidean_distances.begin(),
                                  candidate_euclidean_distances.begin() + NN,
                                  candidate_euclidean_distances.end());

                // 保存结果邻居 (与OpenMVG完全一致)
                for (int l = 0; l < NN; ++l)
                {
                    distances.push_back(candidate_euclidean_distances[l].first);
                    matches.emplace_back(query_idx, candidate_euclidean_distances[l].second, candidate_euclidean_distances[l].first);
                }
            }
            // else -> 候选太少... (不保存任何匹配，与OpenMVG一致)
        }
    }

} // namespace PluginMethods
