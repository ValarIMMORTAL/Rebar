#pragma once

#include "CommonFile.h"

namespace Gallery 
{
    /// @brief 2d有向包围盒
    class OBB2d
    {
    public:
        /// @brief 从点集创建OBB2d包围盒
        static OBB2d from_points(const DPoint2d *points, size_t count);

        /// @brief 获得最小点
        const DPoint2d &get_min() const { return this->m_min; }

        /// @brief 获得最大点
        const DPoint2d &get_max() const { return this->m_max; }

        /// @brief 获得包围盒的主方向
        const DVec2d &direction() const { return this->m_direction; }

        /// @brief 获得包围盒的次方向（与主方向正交）
        const DVec2d &sub_direction() const { return this->m_sub_direction; }

        /// @brief 计算变换矩阵, 可以将当前的m_min, m_max变换到XOY
        RotMatrix calculate_transform() const;

    private:
        OBB2d() = default;

    private:
        DPoint2d m_min;
        DPoint2d m_max;
        DVec2d m_direction;
        DVec2d m_sub_direction;

    private:
        /// @brief 计算点集的协方差矩阵（2x2）
        /// @param points 
        /// @param count 
        /// @return 
        static RotMatrix cov_matrix(const DPoint2d *points, size_t count);

        /// @brief 使用简单的迭代法计算矩阵的特征向量
        /// @details 该方法只能得到一个特征向量
        /// @param matrix 2x2的矩阵
        /// @param loop_count 迭代次数
        /// @return 
        static DVec3d eigenvector_simple(const RotMatrix &matrix, size_t loop_count = 1000);
    };
}

