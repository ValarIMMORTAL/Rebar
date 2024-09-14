#pragma once

#include "CommonFile.h"

namespace Gallery 
{
    /// @brief 2d�����Χ��
    class OBB2d
    {
    public:
        /// @brief �ӵ㼯����OBB2d��Χ��
        static OBB2d from_points(const DPoint2d *points, size_t count);

        /// @brief �����С��
        const DPoint2d &get_min() const { return this->m_min; }

        /// @brief �������
        const DPoint2d &get_max() const { return this->m_max; }

        /// @brief ��ð�Χ�е�������
        const DVec2d &direction() const { return this->m_direction; }

        /// @brief ��ð�Χ�еĴη�����������������
        const DVec2d &sub_direction() const { return this->m_sub_direction; }

        /// @brief ����任����, ���Խ���ǰ��m_min, m_max�任��XOY
        RotMatrix calculate_transform() const;

    private:
        OBB2d() = default;

    private:
        DPoint2d m_min;
        DPoint2d m_max;
        DVec2d m_direction;
        DVec2d m_sub_direction;

    private:
        /// @brief ����㼯��Э�������2x2��
        /// @param points 
        /// @param count 
        /// @return 
        static RotMatrix cov_matrix(const DPoint2d *points, size_t count);

        /// @brief ʹ�ü򵥵ĵ���������������������
        /// @details �÷���ֻ�ܵõ�һ����������
        /// @param matrix 2x2�ľ���
        /// @param loop_count ��������
        /// @return 
        static DVec3d eigenvector_simple(const RotMatrix &matrix, size_t loop_count = 1000);
    };
}

