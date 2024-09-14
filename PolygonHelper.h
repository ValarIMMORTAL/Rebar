#pragma once

#include "CommonFile.h"
#include <vector>

namespace Gallery{
    /// @brief ���ڶ������صļ���
    class PolygonHelper
    {
    public:
        /// @brief ����һ�������
        struct Polygon {
            /// @brief ����εĶ��㣬˳�Σ���β������Ϊ�����
            std::vector<DPoint2d> points;
        };
    public:
        /// @brief ��һ����ά���湹������
        /// @details �������2ά�ϵĸ����˸���ᱻ�任��XOYƽ����
        /// @param face_x ƽ���x�᷽������ȷ����Ķ�ά����
        /// @param face 
        /// @param transform ���ڽ�����εĵ�任��3dƽ��ı任
        /// @param polygon
        /// @return 
        static bool from_3d_face(const ElementHandle &face, const DVec3d &face_x, Transform &out_transform, Polygon &out_polygon);

        /// @brief ת��Polygon����ά��ƽ��ͼ��
        /// @param polygon 
        /// @param transform ��һ����2ά�㣨XOYƽ���ϵĵ㣩ӳ�䵽һ��3άƽ��ı任����һ������ʹ��from_3d_face�з��ص�transform
        /// @param face �����face
        /// @return 
        static bool to_3d_face(const Polygon &polygon, const Transform &transform, EditElementHandle &face);

        /// @brief ��ö���εı��߶�
        /// @param polygon 
        /// @return 
        static std::vector<DSegment3d> to_segments(const Polygon &polygon);

        /// @brief �ж�һ���������˳ʱ��Ļ�����ʱ���
        /// @param polygon 
        /// @return 
        static bool is_clock_wise(const Polygon &polygon);


        /// @brief ƫ�ƶ���ε�ÿ����, ���������Ŵ����С�����
        /// @param polygon 
        /// @param distance ƫ������Ϊ�������⣬Ϊ��������
        static void offset_edges(Polygon &polygon, double distance);
    };
}

