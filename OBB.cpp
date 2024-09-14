#include "_ustation.h"
#include "OBB.h"
#include <algorithm>
#include <numeric>
#include <random>

namespace Gallery
{
    namespace _local {
        /// @brief 计算点集的协方差
        /// @tparam Iterator 
        /// @param points_begin 
        /// @param points_end 
        /// @return 
        template <class Iterator>
        double cov(Iterator points_begin, Iterator points_end)
        {
            auto count = std::distance(points_begin, points_end);

            //求和
            auto x_sum = std::accumulate(
                points_begin, points_end, 0.0,
                [](double sum, const DPoint2d &point) -> double
                {
                    return sum + point.x;
                });
            auto y_sum = std::accumulate(
                points_begin, points_end, 0.0,
                [](double sum, const DPoint2d &point) -> double
                {
                    return sum + point.y;
                });

            // 计算均值
            auto x_mean = x_sum / (double)count;
            auto y_mean = y_sum / (double)count;

            // 计算协方差
            // cov = Sum((x - x_mean) * (y - y_mean)) * (1 / (count - 1))
            auto sum = std::accumulate(
                points_begin, points_end, 0.0,
                [=](double sum, const DPoint2d &point)
                {
                    return sum + ((point.x - x_mean) * (point.y - y_mean));
                });

            return sum * (1.0 / static_cast<double>(count - 1));
        }

        /// @brief 计算点集的轴对齐包围盒
        /// @tparam Iterator 
        /// @param points_begin 
        /// @param points_end 
        /// @return 返回[min_point, max_point]
        template<class Iterator>
        std::pair<DPoint2d, DPoint2d> calc_aabb(Iterator points_begin, Iterator points_end)
        {
            auto min_x = std::min_element(
                points_begin, points_end,
                [](const DPoint3d &pa, const DPoint3d &pb) { return pa.x < pb.x; })->x;
            auto min_y = std::min_element(
                points_begin, points_end,
                [](const DPoint3d &pa, const DPoint3d &pb) { return pa.y < pb.y; })->y;

            auto max_x = std::max_element(
                points_begin, points_end,
                [](const DPoint3d &pa, const DPoint3d &pb) { return pa.x < pb.x; })->x;
            auto max_y = std::max_element(
                points_begin, points_end,
                [](const DPoint3d &pa, const DPoint3d &pb) { return pa.y < pb.y; })->y;

            return std::make_pair(DPoint2d::From(min_x, min_y), DPoint2d::From(max_x, max_y));
        }
    }

    OBB2d OBB2d::from_points(const DPoint2d *points, size_t count)
    {
        // 计算点集的cov matrix
        auto cov_mat = cov_matrix(points, count);
        // 计算cov_matrix的特征向量
        auto eigenvector = eigenvector_simple(cov_mat);
        // 计算一个与特征向量正交的同平面向量, 该方向即为sub_direction
        // 即，将特征向量看成x轴，与垂直平面的Z轴相叉乘，即可得到在当前平面中的y轴
        auto z_axis = DVec3d::From(0, 0, 1);
        auto sub_direction = DVec3d::FromCrossProduct(eigenvector, z_axis);
        // 构造一个新的变换矩阵
        // 将现有的点变换到这个坐标系下
        auto matrix = RotMatrix::FromRowValues(
            eigenvector.x, sub_direction.x, 0,
            eigenvector.y, sub_direction.y, 0,
            0, 0, 1);

        // 转换点集到3d并执行变换
        auto points_3d = std::vector<DPoint3d>();
        std::transform(
            points, points + count, std::back_inserter(points_3d),
            [&](const DPoint2d &point)
            {
                auto point_3d = DPoint3d::From(point);
                matrix.Multiply(point_3d);
                return point_3d;
            });
        
        // 计算这些点在新坐标系下的AABB
        auto min_max_points = _local::calc_aabb(points_3d.cbegin(), points_3d.cend());
        auto min_point = min_max_points.first;
        auto max_point = min_max_points.second;

        // 初始化到类成员
        auto obb2d = OBB2d();
        obb2d.m_max = max_point;
        obb2d.m_min = min_point;
        obb2d.m_direction = DVec2d::From(eigenvector.x, eigenvector.y);
        obb2d.m_direction.Normalize();
        obb2d.m_sub_direction = DVec2d::From(sub_direction.x, sub_direction.y);
        obb2d.m_sub_direction.Normalize();
        return obb2d;
    }

    RotMatrix OBB2d::calculate_transform() const
    {
        auto matrix = RotMatrix::FromRowValues(
            this->m_direction.x, this->m_sub_direction.x, 0,
            this->m_direction.y, this->m_sub_direction.y, 0,
            0, 0, 1);
        
        // 计算该矩阵的逆
        matrix.Invert();

        return matrix;
    }

    RotMatrix OBB2d::cov_matrix(const DPoint2d *points, size_t count)
    {
        auto points_begin = points;
        auto points_end = points + count;

        // 变换点集到xx xy yx yy
        std::vector<DPoint2d> xx_points;
        std::vector<DPoint2d> xy_points;
        std::vector<DPoint2d> yx_points;
        std::vector<DPoint2d> yy_points;

        std::transform(
            points_begin, points_end, std::back_inserter(xx_points),
            [](const DPoint2d &point)
            {
                return DPoint2d::From(point.x, point.x);
            });
        std::transform(
            points_begin, points_end, std::back_inserter(xy_points),
            [](const DPoint2d &point)
            {
                return DPoint2d::From(point.x, point.y);
            });
        std::transform(
            points_begin, points_end, std::back_inserter(yx_points),
            [](const DPoint2d &point)
            {
                return DPoint2d::From(point.y, point.x);
            });
        std::transform(
            points_begin, points_end, std::back_inserter(yy_points),
            [](const DPoint2d &point)
            {
                return DPoint2d::From(point.y, point.y);
            });

        // 计算Cov
        auto cov_xx = _local::cov(xx_points.cbegin(), xx_points.cend());
        auto cov_xy = _local::cov(xy_points.cbegin(), xy_points.cend());
        auto cov_yx = _local::cov(yx_points.cbegin(), yx_points.cend());
        auto cov_yy = _local::cov(yy_points.cbegin(), yy_points.cend());

        // 构造协方差矩阵
        return RotMatrix::FromRowValues(
            cov_xx, cov_xy, 0,
            cov_yx, cov_yy, 0,
            0, 0, 1
        );
    }

    DVec3d OBB2d::eigenvector_simple(const RotMatrix &matrix, size_t loop_count)
    {
        // 随机生成一个二维向量
        std::random_device device; // 使用硬件熵池
        std::default_random_engine engine(device()); // 硬件生成一个随机数作为种子
        std::uniform_real_distribution<double> uniform_dist(0.0, 1.0); // 均匀分布

        auto vec = DVec3d::From(uniform_dist(engine), uniform_dist(engine), 0);

        // 不断对向量作用该矩阵，最后会收敛到一个特征向量上
        for (auto i = 0; i < loop_count; ++i)
        {
            vec.Normalize();
            matrix.Multiply(vec);
        }

        return vec;
    }
}
