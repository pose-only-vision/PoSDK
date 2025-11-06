/**
 * @file method_rotation_averaging_Chatterjee.cpp
 * @brief Implementation of Chatterjee rotation averaging | Chatterjee旋转平均的实现
 */

#include <cassert>   // For assert | 用于assert
#include <cstddef>   // For size_t | 用于size_t
#include <iostream>  // For std::cout, std::cerr | 用于std::cout, std::cerr
#include <algorithm> // For std::transform, std::nth_element, etc. | 用于std::transform, std::nth_element 等
#include <numeric>   // For std::accumulate | 用于std::accumulate
#include <vector>    // For std::vector | 用于std::vector
#include <set>       // For std::set | 用于std::set
#include <map>       // For std::map | 用于std::map
#include <queue>     // For std::queue | 用于std::queue
#include <limits>    // For std::numeric_limits | 用于std::numeric_limits

#include "lemon/adaptors.h"
#include "lemon/dfs.h"
#include "lemon/kruskal.h"
#include "lemon/list_graph.h"
#include "lemon/path.h"
using namespace lemon;
#include "method_rotation_averaging_Chatterjee.hpp"
#include "chatterjee/L1ADMM.h"
#include "chatterjee/rotation.h"
#include <po_core/po_logger.hpp>

namespace PluginMethods
{
    template <typename T>
    inline T Square(T x)
    {
        return x * x;
    }

    struct Link
    {
        uint32_t ID;       // node index | 节点索引
        uint32_t parentID; // parent link | 父链接
        inline Link(uint32_t ID_ = 0, uint32_t parentID_ = 0) : ID(ID_), parentID(parentID_) {}
    };
    using LinkQue = std::queue<Link>;
    using sMat = Eigen::SparseMatrix<double>;
    using Mat = Eigen::MatrixXd;

    template <typename Type, typename DataInputIterator>
    bool minMaxMeanMedian(DataInputIterator begin, DataInputIterator end,
                          Type &min, Type &max, Type &mean, Type &median)
    {
        if (std::distance(begin, end) < 1)
        {
            return false;
        }

        std::vector<Type> vec_val(begin, end);

        // Get the median value: | 获取中位数：
        const auto middle = vec_val.begin() + vec_val.size() / 2;
        std::nth_element(vec_val.begin(), middle, vec_val.end());
        median = *middle;
        min = *std::min_element(vec_val.begin(), middle);
        max = *std::max_element(middle, vec_val.end());
        mean = std::accumulate(vec_val.cbegin(), vec_val.cend(), Type(0)) / static_cast<Type>(vec_val.size());
        return true;
    }

    template <typename TMat>
    inline double FrobeniusNorm(const TMat &A)
    {
        return A.norm();
    }

    /////////////////////////

    // given an array of values, compute the X84 threshold as in: | 给定一个值数组，计算X84阈值，如：
    // Hampel FR, Rousseeuw PJ, Ronchetti EM, Stahel WA
    // "Robust Statistics: the Approach Based on Influence Functions"
    // Wiley Series in Probability and Mathematical Statistics, John Wiley & Sons, 1986
    // returns the pair(median,trust_region) | 返回对(median,trust_region)
    // upper-bound threshold = median+trust_region | 上限阈值 = median+trust_region
    // lower-bound threshold = median-trust_region | 下限阈值 = median-trust_region
    template <typename TYPE>
    inline std::pair<TYPE, TYPE>
    ComputeX84Threshold(const TYPE *const values, uint32_t size, TYPE mul = TYPE(5.2))
    {
        assert(size > 0);
        typename std::vector<TYPE> data(values, values + size);
        typename std::vector<TYPE>::iterator mid = data.begin() + size / 2;
        std::nth_element(data.begin(), mid, data.end());
        const TYPE median = *mid;
        // threshold = 5.2 * MEDIAN(ABS(values-median)); | 阈值 = 5.2 * MEDIAN(ABS(values-median));
        for (size_t i = 0; i < size; ++i)
            data[i] = std::abs(values[i] - median);
        std::nth_element(data.begin(), mid, data.end());
        return {median, mul * (*mid)};
    } // ComputeX84Threshold

    // find the shortest cycle for the given graph and starting vertex | 为给定图和起始顶点查找最短循环
    using IndexArr = std::vector<uint32_t>; // Define IndexArr type | 定义IndexArr类型

    struct Node
    {
        using InternalType = IndexArr;
        InternalType edges; // array of vertex indices | 顶点索引数组
    };

    using NodeArr = std::vector<Node>;

    using graph_t = lemon::ListGraph;
    using map_EdgeMap = graph_t::EdgeMap<double>;
    using MapEdgeIJ2R = std::map<std::pair<uint32_t, uint32_t>, Matrix3x3>;
    using MapEdgeIJ2R = std::map<std::pair<uint32_t, uint32_t>, Matrix3x3>;

    // Look for the maximum spanning tree along the graph of relative rotations | 沿相对旋转图查找最大生成树
    // since we look for the maximum spanning tree using a minimum spanning tree algorithm | 因为我们使用最小生成树算法查找最大生成树
    // weight are negated. | 权重被取反。
    uint32_t FindMaximumSpanningTree(const RelativeRotations &RelRs, graph_t &g, MapEdgeIJ2R &mapIJ2R,
                                     NodeArr &minGraph)
    {
        assert(!RelRs.empty());

        // A-- Compute the number of node we need | A-- 计算我们需要的节点数
        std::set<uint32_t> setNodes;
        for (size_t p = 0; p < RelRs.size(); ++p)
        {
            const RelativeRotation *relR = RelRs[p];
            setNodes.insert(relR->GetViewIdI());
            setNodes.insert(relR->GetViewIdJ());
        }

        // B-- Create a node graph for each element of the set | B-- 为集合的每个元素创建节点图
        using map_NodeT = std::map<uint32_t, graph_t::Node>;
        map_NodeT map_index_to_node;
        std::map<int, uint32_t> node_id_to_index; // Map from graph node ID to index | 图节点ID到索引的映射

        // Create graph nodes for each node and build mapping | 为每个节点创建图节点并建立映射
        for (const auto &iter : setNodes)
        {
            graph_t::Node node = g.addNode();
            map_index_to_node[iter] = node;
            node_id_to_index[g.id(node)] = iter;
        }

        // C-- Create a graph from RelRs with weighted edges | C-- 从RelRs创建带权重的图
        map_EdgeMap map_edgeMap(g);
        for (size_t p = 0; p < RelRs.size(); ++p)
        {
            const RelativeRotation *relR = RelRs[p];
            mapIJ2R[{relR->GetViewIdI(), relR->GetViewIdJ()}] = relR->GetRotation();
            mapIJ2R[{relR->GetViewIdJ(), relR->GetViewIdI()}] = relR->GetRotation().transpose();

            // add edge to the graph | 添加边到图
            graph_t::Edge edge = g.addEdge(map_index_to_node[relR->GetViewIdI()], map_index_to_node[relR->GetViewIdJ()]);
            map_edgeMap[edge] = -relR->GetWeight();
        }

        // D-- Compute the MST of the graph | D-- 计算图的MST
        std::vector<graph_t::Edge> tree_edge_vec;
        lemon::kruskal(g, map_edgeMap, std::back_inserter(tree_edge_vec));

        // E-- Create custom node array | E-- 创建自定义节点数组
        minGraph.resize(setNodes.size());

        // F-- Export computed MST to custom node structure | F-- 导出计算的MST到自定义节点结构
        for (size_t i = 0; i < tree_edge_vec.size(); i++)
        {
            graph_t::Node u = g.u(tree_edge_vec[i]);
            graph_t::Node v = g.v(tree_edge_vec[i]);

            uint32_t u_index = node_id_to_index[g.id(u)];
            uint32_t v_index = node_id_to_index[g.id(v)];

            minGraph[u_index].edges.push_back(v_index);
            minGraph[v_index].edges.push_back(u_index);
        }
        return tree_edge_vec.size();
    }

    // Filter the given relative rotations using the known global rotations | 使用已知全局旋转过滤给定的相对旋转
    // returns the number of inliers | 返回内点数
    unsigned int RotationAveragingChatterjee::FilterRelativeRotations(
        const RelativeRotations &RelRs,
        const Matrix3x3Arr &Rs,
        float threshold,
        std::vector<bool> *vec_inliers)
    {
        assert(!RelRs.empty() && !Rs.empty());
        assert(threshold >= 0);
        // compute errors for each relative rotation | 计算每个相对旋转的错误
        std::vector<float> errors(RelRs.size());
        for (size_t r = 0; r < RelRs.size(); ++r)
        {
            const RelativeRotation *relR = RelRs[r];
            const Matrix3x3 &Ri = Rs[relR->GetViewIdI()];
            const Matrix3x3 &Rj = Rs[relR->GetViewIdJ()];
            const Matrix3x3 &Rij = relR->GetRotation();
            const Mat3 eRij(Rj.transpose() * Rij * Ri);
            const Vec3 erij;
            ceres::RotationMatrixToAngleAxis((const double *)eRij.data(), (double *)erij.data());
            errors[r] = (float)erij.norm();
        }
        if (threshold == 0)
        {
            // estimate threshold | 估计阈值
            const std::pair<float, float> res = ComputeX84Threshold(&errors[0], errors.size());
            threshold = res.first + res.second;
        }
        if (vec_inliers)
        {
            vec_inliers->resize(RelRs.size());
        }
        // mark outliers | 标记外点
        unsigned int nInliers = 0;
        for (size_t r = 0; r < errors.size(); ++r)
        {
            const bool bInlier = errors[r] < threshold;
            if (vec_inliers)
                (*vec_inliers)[r] = bInlier;
            if (bInlier)
                ++nInliers;
        }
        return nInliers;
    } // FilterRelativeRotations
    //----------------------------------------------------------------

    double RelRotationAvgError(
        const RelativeRotations &RelRs,
        const Matrix3x3Arr &Rs,
        double *pMin = nullptr,
        double *pMax = nullptr)
    {
#ifdef HAVE_BOOST
        boost::accumulators::accumulator_set<double,
                                             boost::accumulators::stats<
                                                 boost::accumulators::tag::min,
                                                 boost::accumulators::tag::mean,
                                                 boost::accumulators::tag::max>>
            acc;

        for (int i = 0; i < RelRs.size(); ++i)
        {
            const RelativeRotation *relR = RelRs[i];
            acc(FrobeniusNorm(relR->GetRotation() - (Rs[relR->GetViewIdJ()] * Rs[relR->GetViewIdI()].transpose())));
        }
        if (pMin)
            *pMin = boost::accumulators::min(acc);
        if (pMax)
            *pMax = boost::accumulators::max(acc);
        return boost::accumulators::mean(acc);
#else
        std::vector<double> vec_err(RelRs.size(), 0.0);
        for (size_t i = 0; i < RelRs.size(); ++i)
        {
            const RelativeRotation *relR = RelRs[i];
            vec_err[i] = FrobeniusNorm(relR->GetRotation() - (Rs[relR->GetViewIdJ()] * Rs[relR->GetViewIdI()].transpose()));
        }
        float min, max, mean, median;
        minMaxMeanMedian(vec_err.begin(), vec_err.end(), min, max, mean, median);
        if (pMin)
            *pMin = min;
        if (pMax)
            *pMax = max;
        return mean;
#endif
    }

    //----------------------------------------------------------------

    void RotationAveragingChatterjee::InitRotationsMST(
        const RelativeRotations &RelRs,
        Matrix3x3Arr &Rs,
        const uint32_t nMainViewID)
    {
        assert(!Rs.empty());

        // -- Compute coarse global rotation estimates: | -- 计算粗略全局旋转估计：
        //   - by finding the maximum spanning tree and linking the relative rotations |   - 通过查找最大生成树并链接相对旋转
        //   - Initial solution is driven by relative rotations data confidence. |   - 初始解由相对旋转数据置信度驱动。
        graph_t g;
        MapEdgeIJ2R mapIJ2R;
        NodeArr minGraph;
        // find the Maximum Spanning Tree | 查找最大生成树
        FindMaximumSpanningTree(RelRs, g, mapIJ2R, minGraph);
        g.clear();

        // Ensure the node corresponding to nMainViewID exists in minGraph | 确保minGraph中存在nMainViewID对应的节点
        if (nMainViewID >= minGraph.size())
        {
            LOG_ERROR_ZH << "[RotationAveragingChatterjee] 错误: 主视图ID超出范围";
            LOG_ERROR_EN << "[RotationAveragingChatterjee] Error: Main view ID is out of bounds";
            return;
        }

        // start from the main view and link all views using the relative rotation estimates | 从主视图开始，使用相对旋转估计链接所有视图
        LinkQue stack;
        stack.push(Link(nMainViewID, uint32_t(0)));
        Rs[nMainViewID] = Matrix3x3::Identity();
        do
        {
            const Link &link = stack.front();
            const Node &node = minGraph[link.ID];

            for (Node::InternalType::const_iterator pEdge = node.edges.begin();
                 pEdge != node.edges.end(); ++pEdge)
            {
                const size_t edge = *pEdge;
                if (edge == link.parentID)
                {
                    // compute the global rotation for the current node | 计算当前节点的全局旋转
                    assert(mapIJ2R.find({link.parentID, link.ID}) != mapIJ2R.end());
                    const Matrix3x3 &Rij = mapIJ2R[{link.parentID, link.ID}];
                    Rs[link.ID] = Rij * Rs[link.parentID];
                }
                else
                {
                    // add edge to the processing queue | 添加边到处理队列
                    stack.push(Link(edge, link.ID));
                }
            }
            stack.pop();
        } while (!stack.empty());
    }

    // Robustly estimate global rotations from relative rotations as in: | 如以下文献中稳健估计全局旋转：
    // "Efficient and Robust Large-Scale Rotation Averaging", Chatterjee and Govindu, 2013
    // and detect outliers relative rotations and return them with 0 in arrInliers | 并检测外点相对旋转，并在arrInliers中返回0
    bool RotationAveragingChatterjee::GlobalRotationsRobust(
        const RelativeRotations &RelRs,
        Matrix3x3Arr &Rs,
        const uint32_t nMainViewID,
        float threshold,
        std::vector<bool> *vec_Inliers)
    {
        assert(!RelRs.empty() && !Rs.empty());

        // -- Compute coarse global rotation estimates: | -- 计算粗略全局旋转估计：
        InitRotationsMST(RelRs, Rs, nMainViewID);

        // refine global rotations based on the relative rotations | 基于相对旋转细化全局旋转
        const bool bOk = RefineRotationsAvgL1IRLS(RelRs, Rs, nMainViewID);

        // find outlier relative rotations | 查找外点相对旋转
        if (threshold >= 0 && vec_Inliers)
        {
            FilterRelativeRotations(RelRs, Rs, threshold, vec_Inliers);
        }

        return bOk;
    } // GlobalRotationsRobust
    //----------------------------------------------------------------

    namespace internal
    {
        // build A in Ax=b | 构建A in Ax=b
        inline void FillMappingMatrix(
            const RelativeRotations &RelRs,
            const uint32_t nMainViewID,
            sMat &A)
        {
            A.reserve(A.rows() * 2); // estimate of the number of non-zeros (optional) | 非零元素的估计（可选）
            sMat::Index i = 0, j = 0;
            for (size_t r = 0; r < RelRs.size(); ++r)
            {
                const RelativeRotation *relR = RelRs[r];
                if (relR->GetViewIdI() != nMainViewID)
                {
                    j = 3 * (relR->GetViewIdI() < nMainViewID ? relR->GetViewIdI() : relR->GetViewIdI() - 1);
                    A.insert(i + 0, j + 0) = -1.0;
                    A.insert(i + 1, j + 1) = -1.0;
                    A.insert(i + 2, j + 2) = -1.0;
                }
                if (relR->GetViewIdJ() != nMainViewID)
                {
                    j = 3 * (relR->GetViewIdJ() < nMainViewID ? relR->GetViewIdJ() : relR->GetViewIdJ() - 1);
                    A.insert(i + 0, j + 0) = 1.0;
                    A.insert(i + 1, j + 1) = 1.0;
                    A.insert(i + 2, j + 2) = 1.0;
                }
                i += 3;
            }
            A.makeCompressed();
        }

        // compute errors for each relative rotation | 计算每个相对旋转的错误
        inline void FillErrorMatrix(
            const RelativeRotations &RelRs,
            const Matrix3x3Arr &Rs,
            Vec &b)
        {
            for (size_t r = 0; r < RelRs.size(); ++r)
            {
                const RelativeRotation *relR = RelRs[r];
                const Matrix3x3 &Ri = Rs[relR->GetViewIdI()];
                const Matrix3x3 &Rj = Rs[relR->GetViewIdJ()];
                const Matrix3x3 &Rij = relR->GetRotation();
                const Mat3 eRij(Rj.transpose() * Rij * Ri);
                const Vec3 erij;
                ceres::RotationMatrixToAngleAxis((const double *)eRij.data(), (double *)erij.data());
                b.block<3, 1>(3 * r, 0) = erij;
            }
        }

        // apply correction to global rotations | 应用校正到全局旋转
        inline void CorrectMatrix(
            const Mat &x,
            const uint32_t nMainViewID,
            Matrix3x3Arr &Rs)
        {
            for (size_t r = 0; r < Rs.size(); ++r)
            {
                if (r == nMainViewID)
                    continue;
                Matrix3x3 &Ri = Rs[r];
                const uint32_t i = (r < nMainViewID ? r : r - 1);
                const Vec3 eRid = Vec3(x.block<3, 1>(3 * i, 0));
                const Mat3 eRi;
                ceres::AngleAxisToRotationMatrix((const double *)eRid.data(), (double *)eRi.data());
                Ri = Ri * eRi;
            }
        }

        // L1RA -> L1 Rotation Averaging implementation | L1RA -> L1旋转平均实现
        bool SolveL1RA(
            const RelativeRotations &RelRs,
            Matrix3x3Arr &Rs,
            const sMat &A,
            const unsigned int nMainViewID)
        {
            const unsigned nObss = (unsigned)RelRs.size();
            const unsigned nVars = (unsigned)Rs.size() - 1; // one view is kept constant | 一个视图保持不变
            const unsigned m = nObss * 3;
            const unsigned n = nVars * 3;

            // init x with 0 that corresponds to trusting completely the initial Ri guess | 用0初始化x，对应完全信任初始Ri猜测
            Vec x(Vec::Zero(n)), b(m);

            // Current error and the previous one | 当前错误和前一个错误
            double e = std::numeric_limits<double>::max(), ep;
            unsigned iter = 0;
            // L1RA iterate optimization till the desired precision is reached | L1RA迭代优化直到达到期望精度
            do
            {
                // compute errors for each relative rotation | 计算每个相对旋转的错误
                FillErrorMatrix(RelRs, Rs, b);

                // solve the linear system using l1 norm | 使用l1范数求解线性系统
                PoSDK::L1Solver<sMat>::Options options;
                PoSDK::L1Solver<sMat> l1_solver(options, A);
                l1_solver.Solve(b, &x);

                ep = e;
                e = x.norm();
                if (ep < e)
                    break;
                // apply correction to global rotations | 应用校正到全局旋转
                CorrectMatrix(x, nMainViewID, Rs);
            } while (++iter < 32 && e > 1e-5 && (ep - e) / e > 1e-2);

            LOG_INFO_ZH << "[RotationAveragingChatterjee] L1RA在 " << iter << " 次迭代中收敛.";
            LOG_INFO_EN << "[RotationAveragingChatterjee] L1RA Converged in " << iter << " iterations.";

            return true;
        }

        // Iteratively Reweighted Least Squares (IRLS) implementation | 迭代重加权最小二乘（IRLS）实现
        bool SolveIRLS(
            const RelativeRotations &RelRs,
            Matrix3x3Arr &Rs,
            const sMat &A,
            const unsigned int nMainViewID,
            const double sigma)
        {
            const unsigned nObss = (unsigned)RelRs.size();
            const unsigned nVars = (unsigned)Rs.size() - 1; // one view is kept constant | 一个视图保持不变
            const unsigned m = nObss * 3;
            const unsigned n = nVars * 3;

            // init x with 0 that corresponds to trusting completely the initial Ri guess | 用0初始化x，对应完全信任初始Ri猜测
            Vec x(Vec::Zero(n)), b(m);

            // Since the sparsity pattern will not change with each linear solve | 由于稀疏模式不会随每个线性求解改变
            //  compute it once to speed up the solution time. |  计算一次以加速求解时间。
            using Linear_Solver_T = Eigen::SimplicialLDLT<sMat>;

            Linear_Solver_T linear_solver;
            linear_solver.analyzePattern(A.transpose() * A);
            if (linear_solver.info() != Eigen::Success)
            {
                LOG_ERROR_ZH << "[RotationAveragingChatterjee] Cholesky分解失败.";
                LOG_ERROR_EN << "[RotationAveragingChatterjee] Cholesky decomposition failed.";
                return false;
            }

            const double sigmaSq(Square(sigma));

            Eigen::ArrayXd errors, weights;
            Vec xp(n);
            // current error and the previous one | 当前错误和前一个错误
            double e = std::numeric_limits<double>::max(), ep;
            unsigned int iter = 0;
            do
            {
                xp = x;
                // compute errors for each relative rotation | 计算每个相对旋转的错误
                FillErrorMatrix(RelRs, Rs, b);

                // Compute the weights for each error term | 计算每个错误项的权重
                errors = (A * x - b).array();

                // compute robust errors using the Huber-like loss function | 使用Huber-like损失函数计算稳健错误
                weights = sigmaSq / (errors.square() + sigmaSq).square();

                // Update the factorization for the weighted values | 更新加权值的因子分解
                const sMat at_weight = A.transpose() * weights.matrix().asDiagonal();
                linear_solver.factorize(at_weight * A);
                if (linear_solver.info() != Eigen::Success)
                {
                    LOG_ERROR_ZH << "[RotationAveragingChatterjee] 无法因子化最小二乘系统.";
                    LOG_ERROR_EN << "[RotationAveragingChatterjee] Failed to factorize the least squares system.";
                    return false;
                }

                // Solve the least squares problem | 求解最小二乘问题
                x = linear_solver.solve(at_weight * b);
                if (linear_solver.info() != Eigen::Success)
                {
                    LOG_ERROR_ZH << "[RotationAveragingChatterjee] 无法求解最小二乘系统.";
                    LOG_ERROR_EN << "[RotationAveragingChatterjee] Failed to solve the least squares system.";
                    return false;
                }

                // apply correction to global rotations | 应用校正到全局旋转
                CorrectMatrix(x, nMainViewID, Rs);

                ep = e;
                e = (xp - x).norm();
            } while (++iter < 32 && e > 1e-5 && (ep - e) / e > 1e-2);

            LOG_INFO_ZH << "[RotationAveragingChatterjee] IRLS在 " << iter << " 次迭代中收敛.";
            LOG_INFO_EN << "[RotationAveragingChatterjee] IRLS Converged in " << iter << " iterations.";

            return true;
        }
    } // namespace internal

    // Refine the global rotations using to the given relative rotations, similar to: | 使用给定的相对旋转细化全局旋转，类似于：
    // "Efficient and Robust Large-Scale Rotation Averaging", Chatterjee and Govindu, 2013
    // L1 Rotation Averaging (L1RA) and Iteratively Reweighted Least Squares (IRLS) implementations combined | L1旋转平均（L1RA）和迭代重加权最小二乘（IRLS）实现的组合
    bool RotationAveragingChatterjee::RefineRotationsAvgL1IRLS(
        const RelativeRotations &RelRs,
        Matrix3x3Arr &Rs,
        const uint32_t nMainViewID,
        const double sigma)
    {
        assert(!RelRs.empty() && !Rs.empty());

        double fMinBefore, fMaxBefore;
        const double fMeanBefore = RelRotationAvgError(RelRs, Rs, &fMinBefore, &fMaxBefore);

        const unsigned nObss = (unsigned)RelRs.size();
        const unsigned nVars = (unsigned)Rs.size() - 1; // main view is kept constant | 主视图保持不变
        const unsigned m = nObss * 3;
        const unsigned n = nVars * 3;

        // build mapping matrix A in Ax=b | 构建映射矩阵A in Ax=b
        sMat A(m, n);
        internal::FillMappingMatrix(RelRs, nMainViewID, A);

        if (!internal::SolveL1RA(RelRs, Rs, A, nMainViewID))
        {
            LOG_ERROR_ZH << "[RotationAveragingChatterjee] 无法求解L1回归步骤.";
            LOG_ERROR_EN << "[RotationAveragingChatterjee] Could not solve the L1 regression step.";
            return false;
        }

        if (!internal::SolveIRLS(RelRs, Rs, A, nMainViewID, sigma))
        {
            LOG_ERROR_ZH << "[RotationAveragingChatterjee] 无法求解ILRS步骤.";
            LOG_ERROR_EN << "[RotationAveragingChatterjee] Could not solve the ILRS step.";
            return false;
        }

        double fMinAfter, fMaxAfter;
        const double fMeanAfter = RelRotationAvgError(RelRs, Rs, &fMinAfter, &fMaxAfter);

        LOG_INFO_ZH << "[RotationAveragingChatterjee] 使用L1RA-IRLS和 " << nObss << " 个相对旋转细化全局旋转:\n"
                      << " 错误从 " << fMeanBefore << "(" << fMinBefore << " min, " << fMaxBefore << " max)\n"
                      << " 减少到 " << fMeanAfter << "(" << fMinAfter << "min," << fMaxAfter << "max)";
        LOG_INFO_EN << "[RotationAveragingChatterjee] Refine global rotations using L1RA-IRLS and " << nObss << " relative rotations:\n"
                      << " error reduced from " << fMeanBefore << "(" << fMinBefore << " min, " << fMaxBefore << " max)\n"
                      << " to " << fMeanAfter << "(" << fMinAfter << "min," << fMaxAfter << "max)";

        return true;
    } // RefineRotationsAvgL1IRLS

    DataPtr RotationAveragingChatterjee::Run()
    {
        // Select corresponding processing function based on computation mode | 根据计算模式选择对应的处理函数
        // std::string compute_mode = GetOptionAsString(method_options_, "compute_mode", "direct");
        //- Solve the global rotation estimation problem: | - 求解全局旋转估计问题：
        const size_t nMainViewID = 0; // arbitrary choice | 任意选择
        std::vector<bool> vec_inliers;

        DisplayConfigInfo();

        // Get input data | 获取输入数据
        auto relative_poses_ptr = GetDataPtr<RelativePoses>(required_package_["data_relative_poses"]);
        if (!relative_poses_ptr)
        {
            LOG_ERROR_ZH << "[RotationAveragingChatterjee] 无相对位姿数据";
            LOG_ERROR_EN << "[RotationAveragingChatterjee] No relative poses data";
            return nullptr;
        }

        // Create global pose data as output | 创建全局位姿数据作为输出
        auto global_poses_data = FactoryData::Create("data_global_poses");
        if (!global_poses_data)
        {
            LOG_ERROR_ZH << "[RotationAveragingChatterjee] 创建全局位姿数据失败";
            LOG_ERROR_EN << "[RotationAveragingChatterjee] Failed to create global poses data";
            return nullptr;
        }

        auto global_poses_ptr = GetDataPtr<GlobalPoses>(global_poses_data);
        if (!global_poses_ptr)
        {
            LOG_ERROR_ZH << "[RotationAveragingChatterjee] 获取GlobalPoses指针失败";
            LOG_ERROR_EN << "[RotationAveragingChatterjee] Failed to get GlobalPoses pointer";
            return nullptr;
        }

        // Convert RelativePose to RelativeRotation | 将RelativePose转换为RelativeRotation
        RelativeRotations relative_rotations;
        relative_rotations.reserve(relative_poses_ptr->size());

        // Calculate number of cameras by finding the maximum camera index | 通过查找最大相机索引计算相机数量
        unsigned int max_camera_index = 0;
        for (const auto &pose : *relative_poses_ptr)
        {
            max_camera_index = std::max(max_camera_index, std::max(pose.GetViewIdI(), pose.GetViewIdJ()));
            RelativeRotation rot(pose.GetViewIdI(),
                                 pose.GetViewIdJ(),
                                 pose.GetRotation(),
                                 pose.GetWeight());
            relative_rotations.push_back(rot);
        }
        unsigned int num_cameras = max_camera_index + 1; // +1 because indices are 0-based | +1 因为索引从0开始
        Matrix3x3Arr vec_globalR(num_cameras);
        bool bSuccess = GlobalRotationsRobust(
            relative_rotations, vec_globalR, nMainViewID, 0.0f, &vec_inliers);

        for (auto gR : vec_globalR)
        {
            global_poses_ptr->GetRotations().push_back(gR);
        }
        return global_poses_data;
    }

} // namespace PluginMethods

// ✅ 使用单参数模式，自动从 CMake 读取 PLUGIN_NAME（实现单一信息源）
REGISTRATION_PLUGIN(PluginMethods::RotationAveragingChatterjee)
