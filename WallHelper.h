#pragma once

#include "CommonFile.h"
#include <vector>
#include <functional>
#include "obb.h"

namespace Gallery
{
    class WallHelper
    {
    public:
        /// @brief 相关构件的类型
        enum class AssociatedType
        {
            /// @brief 顶板
            TopFloor,
            /// @brief 底板
            BottomFloor,
            /// @brief 其它的墙
            Wall
        };

        /// @brief 和墙相连的构件
        struct Associated {
            /// @brief 关联构件的类型
            AssociatedType type;
            /// @brief 关联构件的handle
            ElementHandle element;
        };

        /// @brief 墙的几何信息
        /// @details 这其中所有的指针不需要释放, 该类析构时自动释放
        struct WallGeometryInfo 
        {
            /// @brief 墙本身的引用，不需要释放
            const ElementHandle *wall;

            /// @brief 墙的面数
            size_t faces_count;

            /// @brief 墙的所有的面
            EditElementHandle *faces[32];

            /// @brief 墙的顶面
            EditElementHandle *top_face;

            /// @brief 墙的底面
            EditElementHandle *bottom_face;

            /// @brief 墙的正面
            EditElementHandle *front_face;

            /// @brief 墙的反面
            EditElementHandle *back_face;

            /// @brief 墙整体的朝向（即front_face的法向）
            /// @details 该法向应认为是指向墙的外侧
            DVec3d normal;

            /// @brief 墙本身的方向（与normal垂直）, 墙可能是斜的，该向量不一定位于XOY平面
            DVec3d direction;

            /// @brief 墙的厚度
            /// @details OBB包围盒在法向上的长度
            double thickness;

            /// @brief 墙的高度
            /// @details 包围盒在z轴上的高度
            double height;

            ~WallGeometryInfo();
        };

        /// @brief 墙的内侧和外侧面
        /// @details 这其中指针来自于WallGeometryInfo的指针
        struct WallInsideOutsideFaces {
            /// @brief 内侧面
            std::vector<EditElementHandle *> inside_faces;
            /// @brief 外侧面
            std::vector<EditElementHandle *> outside_faces;
        };

    public:
        /// @brief 检查一个元素是不是墙
        /// @param elem 
        /// @return 
        static bool is_wall(const ElementHandle &element);


        /// @brief 分析墙的几何信息
        /// @details 目前不支持STGWall
        /// @param wall 待分析的墙
        /// @param out_geometry_info 输出的几何信息
        /// @return 返回true表示成功分析
        static bool analysis_geometry(const ElementHandle &wall, WallGeometryInfo &out_geometry_info);

        /// @brief 分析和墙相关连的构件
        /// @param wall 
        /// @param associated 
        /// @return 
        static bool analysis_associated(const ElementHandle &wall, std::vector<Associated> &associated, WallGeometryInfo &wall_geometry_info);

		static bool GetAssociatedWallAndFloor (const ElementHandle &wall, std::vector<Associated> &associated, WallGeometryInfo &wall_geometry_info);

		/// @brief 判断板是否为顶板,false代表为底板
	   /// @param slab 
	   /// @param bool   
	   /// @return 
		static bool analysis_slab_isTop(const ElementHandle &slab);

        /// @brief 分析墙和板的相对位置关系
        /// @details 墙板有如下三种关系
        ///          ------     -----     ------
        ///               |     |           |
        ///               |     |           |
        ///               |     |           |
        /// @param wall 
        /// @param wall_normal 墙的法向
        /// @param slab 需要分析的板
        /// @param out_slab_direction 输出的位置关系向量，如果墙和板是呈90度角，则返回
        ///                           墙的法向或者法向的反方向，与板的位置关系有关
        ///                           如板与墙呈T型, 则返回0向量
        /// @return 获得失败返回false
        static bool analysis_slab_position_with_wall(const ElementHandle &wall, const DVec3d &wall_normal, const ElementHandle &slab, DVec3d &out_slab_direction);

        static bool analysis_slab_position_with_wall(const WallGeometryInfo &geometry, const ElementHandle &slab, DVec3d &out_slab_direction)
        {
            return analysis_slab_position_with_wall(*geometry.wall, geometry.normal, slab, out_slab_direction);
        }

        /// @brief 根据墙板的位置关系获得墙面（内侧面，外侧面）
        /// @details 墙板有如下三种关系
        ///          ------          -----     ------
        ///               |         |            |
        ///            内 | 外   外 | 内       内|内
        ///               |        |            |
        /// @param wall_geometry 
        /// @param slab 
        /// @param out_faces 
        /// @return 
        static bool analysis_inside_outside_faces_by_slab(const WallGeometryInfo &wall_geometry, const ElementHandle &slab, WallInsideOutsideFaces &out_faces);

        /// @brief 判断一个face是不是内侧面
        /// @param faces 
        /// @param face 
        /// @return 
        static bool is_inside_face(const WallInsideOutsideFaces &faces, const EditElementHandle *face);

        /// @brief 判断一个face是不是外侧面
        /// @param faces 
        /// @param face 
        /// @return 
        static bool is_outside_face(const WallInsideOutsideFaces &faces, const EditElementHandle *face);

		static bool combineTop;//是否是合成的顶板

		static bool combineBottom;//是否是合成的底板。默认都是false;

    private:
        /// @brief 扫描range内的所有元素
        /// @param range 
        /// @param filter 过滤函数
        /// @return 
        static std::vector<ElementHandle> scan_elements_in_range(const DRange3d &range, std::function<bool(const ElementHandle &)> filter);

        /// @brief 计算一个Range的中点
        /// @param range 
        /// @return 
        static DPoint3d range_mid_point(const DRange3d &range);
    };
}

