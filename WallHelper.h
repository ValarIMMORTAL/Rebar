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
        /// @brief ��ع���������
        enum class AssociatedType
        {
            /// @brief ����
            TopFloor,
            /// @brief �װ�
            BottomFloor,
            /// @brief ������ǽ
            Wall
        };

        /// @brief ��ǽ�����Ĺ���
        struct Associated {
            /// @brief ��������������
            AssociatedType type;
            /// @brief ����������handle
            ElementHandle element;
        };

        /// @brief ǽ�ļ�����Ϣ
        /// @details ���������е�ָ�벻��Ҫ�ͷ�, ��������ʱ�Զ��ͷ�
        struct WallGeometryInfo 
        {
            /// @brief ǽ��������ã�����Ҫ�ͷ�
            const ElementHandle *wall;

            /// @brief ǽ������
            size_t faces_count;

            /// @brief ǽ�����е���
            EditElementHandle *faces[32];

            /// @brief ǽ�Ķ���
            EditElementHandle *top_face;

            /// @brief ǽ�ĵ���
            EditElementHandle *bottom_face;

            /// @brief ǽ������
            EditElementHandle *front_face;

            /// @brief ǽ�ķ���
            EditElementHandle *back_face;

            /// @brief ǽ����ĳ��򣨼�front_face�ķ���
            /// @details �÷���Ӧ��Ϊ��ָ��ǽ�����
            DVec3d normal;

            /// @brief ǽ����ķ�����normal��ֱ��, ǽ������б�ģ���������һ��λ��XOYƽ��
            DVec3d direction;

            /// @brief ǽ�ĺ��
            /// @details OBB��Χ���ڷ����ϵĳ���
            double thickness;

            /// @brief ǽ�ĸ߶�
            /// @details ��Χ����z���ϵĸ߶�
            double height;

            ~WallGeometryInfo();
        };

        /// @brief ǽ���ڲ�������
        /// @details ������ָ��������WallGeometryInfo��ָ��
        struct WallInsideOutsideFaces {
            /// @brief �ڲ���
            std::vector<EditElementHandle *> inside_faces;
            /// @brief �����
            std::vector<EditElementHandle *> outside_faces;
        };

    public:
        /// @brief ���һ��Ԫ���ǲ���ǽ
        /// @param elem 
        /// @return 
        static bool is_wall(const ElementHandle &element);


        /// @brief ����ǽ�ļ�����Ϣ
        /// @details Ŀǰ��֧��STGWall
        /// @param wall ��������ǽ
        /// @param out_geometry_info ����ļ�����Ϣ
        /// @return ����true��ʾ�ɹ�����
        static bool analysis_geometry(const ElementHandle &wall, WallGeometryInfo &out_geometry_info);

        /// @brief ������ǽ������Ĺ���
        /// @param wall 
        /// @param associated 
        /// @return 
        static bool analysis_associated(const ElementHandle &wall, std::vector<Associated> &associated, WallGeometryInfo &wall_geometry_info);

		static bool GetAssociatedWallAndFloor (const ElementHandle &wall, std::vector<Associated> &associated, WallGeometryInfo &wall_geometry_info);

		/// @brief �жϰ��Ƿ�Ϊ����,false����Ϊ�װ�
	   /// @param slab 
	   /// @param bool   
	   /// @return 
		static bool analysis_slab_isTop(const ElementHandle &slab);

        /// @brief ����ǽ�Ͱ�����λ�ù�ϵ
        /// @details ǽ�����������ֹ�ϵ
        ///          ------     -----     ------
        ///               |     |           |
        ///               |     |           |
        ///               |     |           |
        /// @param wall 
        /// @param wall_normal ǽ�ķ���
        /// @param slab ��Ҫ�����İ�
        /// @param out_slab_direction �����λ�ù�ϵ���������ǽ�Ͱ��ǳ�90�Ƚǣ��򷵻�
        ///                           ǽ�ķ�����߷���ķ���������λ�ù�ϵ�й�
        ///                           �����ǽ��T��, �򷵻�0����
        /// @return ���ʧ�ܷ���false
        static bool analysis_slab_position_with_wall(const ElementHandle &wall, const DVec3d &wall_normal, const ElementHandle &slab, DVec3d &out_slab_direction);

        static bool analysis_slab_position_with_wall(const WallGeometryInfo &geometry, const ElementHandle &slab, DVec3d &out_slab_direction)
        {
            return analysis_slab_position_with_wall(*geometry.wall, geometry.normal, slab, out_slab_direction);
        }

        /// @brief ����ǽ���λ�ù�ϵ���ǽ�棨�ڲ��棬����棩
        /// @details ǽ�����������ֹ�ϵ
        ///          ------          -----     ------
        ///               |         |            |
        ///            �� | ��   �� | ��       ��|��
        ///               |        |            |
        /// @param wall_geometry 
        /// @param slab 
        /// @param out_faces 
        /// @return 
        static bool analysis_inside_outside_faces_by_slab(const WallGeometryInfo &wall_geometry, const ElementHandle &slab, WallInsideOutsideFaces &out_faces);

        /// @brief �ж�һ��face�ǲ����ڲ���
        /// @param faces 
        /// @param face 
        /// @return 
        static bool is_inside_face(const WallInsideOutsideFaces &faces, const EditElementHandle *face);

        /// @brief �ж�һ��face�ǲ��������
        /// @param faces 
        /// @param face 
        /// @return 
        static bool is_outside_face(const WallInsideOutsideFaces &faces, const EditElementHandle *face);

		static bool combineTop;//�Ƿ��ǺϳɵĶ���

		static bool combineBottom;//�Ƿ��Ǻϳɵĵװ塣Ĭ�϶���false;

    private:
        /// @brief ɨ��range�ڵ�����Ԫ��
        /// @param range 
        /// @param filter ���˺���
        /// @return 
        static std::vector<ElementHandle> scan_elements_in_range(const DRange3d &range, std::function<bool(const ElementHandle &)> filter);

        /// @brief ����һ��Range���е�
        /// @param range 
        /// @return 
        static DPoint3d range_mid_point(const DRange3d &range);
    };
}

