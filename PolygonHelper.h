#pragma once

#include "CommonFile.h"
#include <vector>

namespace Gallery{
    /// @brief 用于多边形相关的计算
    class PolygonHelper
    {
    public:
        /// @brief 代表一个多边形
        struct Polygon {
            /// @brief 多边形的顶点，顺次，首尾相连即为多边形
            std::vector<DPoint2d> points;
        };
    public:
        /// @brief 从一个三维的面构造多边形
        /// @details 多边形是2维上的概念，因此该面会被变换到XOY平面上
        /// @param face_x 平面的x轴方向，用于确定点的二维坐标
        /// @param face 
        /// @param transform 用于将多边形的点变换回3d平面的变换
        /// @param polygon
        /// @return 
        static bool from_3d_face(const ElementHandle &face, const DVec3d &face_x, Transform &out_transform, Polygon &out_polygon);

        /// @brief 转换Polygon到三维的平面图形
        /// @param polygon 
        /// @param transform 是一个将2维点（XOY平面上的点）映射到一个3维平面的变换，这一步可以使用from_3d_face中返回的transform
        /// @param face 输出的face
        /// @return 
        static bool to_3d_face(const Polygon &polygon, const Transform &transform, EditElementHandle &face);

        /// @brief 获得多边形的边线段
        /// @param polygon 
        /// @return 
        static std::vector<DSegment3d> to_segments(const Polygon &polygon);

        /// @brief 判断一个多边形是顺时针的还是逆时针的
        /// @param polygon 
        /// @return 
        static bool is_clock_wise(const Polygon &polygon);


        /// @brief 偏移多边形的每条边, 可以用来放大或缩小多边形
        /// @param polygon 
        /// @param distance 偏移量，为正是向外，为负是向内
        static void offset_edges(Polygon &polygon, double distance);
    };
}

