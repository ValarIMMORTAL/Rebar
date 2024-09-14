#include "_ustation.h"
#include "WallHelper.h"
#include "SlabHelper.h"
#include "ScanIntersectTool.h"
#include "CFaceTool.h"
#include "CElementTool.h"
#include "CModelTool.h"
#include "OBB.h"
#include <string>
#include <map>

namespace Gallery
{
	bool WallHelper::combineTop = false;
	bool WallHelper::combineBottom = false;

	bool WallHelper::is_wall(const ElementHandle &element)
	{
		std::string _name, type;
		if (!GetEleNameAndType(const_cast<ElementHandleR>(element), _name, type))
		{
			return false;
		}
		auto result_pos_wall = type.find("WALL");
		return result_pos_wall != std::string::npos;
	}

	bool WallHelper::analysis_geometry(const ElementHandle &wall, WallGeometryInfo &out_geometry_info)
	{
		/*if (!is_wall(wall))
		{
			return false;
		}*/

		// ��ȡǽ������
		std::vector<EditElementHandleP> wall_faces;
		PITCommonTool::CFaceTool::GetElementAllFaces(wall, wall_faces);

		// ����������еĵ㲢ͶӰ��XOYƽ����
		std::vector<DPoint2d> wall_points;
		for (auto face : wall_faces)
		{
			// ���ǽ�����еı�
			std::vector<MSElementDescrP> edges;
			PITCommonTool::CElementTool::GetALLEdges(*face, edges);

			// ���ǽ�ߵĶ˵�
			for (auto edge : edges)
			{
				DPoint3d start_point, end_point;
				mdlElmdscr_extractEndPoints(&start_point, nullptr, &end_point, nullptr, edge, wall.GetModelRef());

				// ͶӰ��XOY������wall_points
				wall_points.push_back(DPoint2d::From(start_point));
				wall_points.push_back(DPoint2d::From(end_point));
			}
		}

		// ������Щ���OBB��Χ��
		auto obb = OBB2d::from_points(wall_points.data(), wall_points.size());

		// ���obb��Χ�еĳ��߷���(�ж�����x/y�������ı߸���)
		auto obb_length_x = obb.get_max().x - obb.get_min().x;
		auto obb_length_y = obb.get_max().y - obb.get_min().y;
		auto longest_edge_vec =
			obb_length_x > obb_length_y ? DVec3d::From(obb_length_x, 0, 0) : DVec3d::From(0, obb_length_y, 0);
		// �̱߼�Ϊǽ�ĺ��
#undef min //������bentley����һ��min�ĺ�
		auto thickness = std::min(obb_length_x, obb_length_y);

		// �任��������XOY��
		obb.calculate_transform().Multiply(longest_edge_vec);

		// �������������ֱ��������Ϊ����
		auto z_axis = DVec3d::From(0, 0, 1);
		auto wall_normal = DVec3d::FromCrossProduct(longest_edge_vec, z_axis);

		// ���˳�ǽ�Ĳ���
		std::vector<EditElementHandleP> side_faces;
		for (const auto face_p : wall_faces)
		{
			const auto face_desc_p = face_p->GetElementDescrCP();

			// �����ķ���
			DPoint3d normal;
			auto z_axis = DPoint3d::From(0, 0, 1);
			mdlElmdscr_extractNormal(&normal, nullptr, face_desc_p, &z_axis);

			if (normal.IsParallelTo(z_axis))
			{
				// �޷�ȷ�����򣬻��߷������z��
				continue;
			}

			if (COMPARE_VALUES_EPS(normal.DotProduct(z_axis), 0, 1e-5) == 0)
			{
				// ��z�ᴹֱ, ��Ϊǽ����
				side_faces.push_back(face_p);
			}
		}

		// �ҵ������뷨��н�С��45�ȵ��棬�������������ͱ���
		std::vector<EditElementHandleP> front_or_back_faces;
		std::copy_if(
			side_faces.cbegin(), side_faces.cend(),
			std::back_inserter(front_or_back_faces),
			[&](const EditElementHandleP &face) -> bool {
				// �����ķ���
				DPoint3d normal;
				auto z_axis = DPoint3d::From(0, 0, 1);
				mdlElmdscr_extractNormal(&normal, nullptr, face->GetElementDescrCP(), &z_axis);

				// ���㷴�ķ���
				auto reverse_normal = normal;
				reverse_normal.Negate();

				// �ֱ��������������ļн�
				auto angle = wall_normal.AngleTo(DVec3d::From(normal));
				auto angle_reverse = wall_normal.AngleTo(DVec3d::From(reverse_normal));


				// ����һ������С��45�ȼ���
				return (std::abs(angle) < PI / 4) || (std::abs(angle_reverse) < PI / 4);
			}
		);

		// ������Щ���Χ���е㣬�е��ڷ�����ͶӰ������������Ϊ���棬��С��Ϊ����
		// �������Ա�֤ǽ������ǽ�������ָ��ǽ���ⲿ
		auto min_max_face_iter =
			std::minmax_element(
				front_or_back_faces.cbegin(), front_or_back_faces.cend(),
				[&](const EditElementHandleP &face_a, const EditElementHandleP &face_b) -> bool
				{
					DRange3d range_a, range_b;
					mdlElmdscr_computeRange(&range_a.low, &range_a.high, face_a->GetElementDescrCP(), nullptr);
					mdlElmdscr_computeRange(&range_b.low, &range_b.high, face_b->GetElementDescrCP(), nullptr);

					auto mid_point_a = range_mid_point(range_a);
					auto mid_point_b = range_mid_point(range_b);

					// ����Щ�㿴��������(point - (0,0,0))
					// ���ݷ����ϵ�ͶӰ���򼴿�
					auto projection_a = mid_point_a.DotProduct(wall_normal);
					auto projection_b = mid_point_b.DotProduct(wall_normal);

					return projection_a < projection_b;
				});

		if (min_max_face_iter.first == front_or_back_faces.cend() || min_max_face_iter.second == front_or_back_faces.cend())
		{
			return false;
		}
		auto front_face = *min_max_face_iter.second;
		auto back_face = *min_max_face_iter.first;

		// �������ĵױ߻򶥱�
		// ȡ�ñ߷���Ϊǽ�ķ���
		// �����ı߽�
		std::vector<MSElementDescrP> front_face_edges;
		PITCommonTool::CElementTool::GetALLEdges(*front_face, front_face_edges);

		// ��ñ߽�����е�
		auto direction = longest_edge_vec;
		for (auto edge : front_face_edges)
		{
			// ��ñ߽����ֹ��
			DPoint3d start, end;
			mdlElmdscr_extractEndPoints(&start, nullptr, &end, nullptr, edge, front_face->GetModelRef());

			// �ߵķ���
			auto edge_dir = end - start;

			// ���˵�ƽ��z��
			if (edge_dir.IsParallelTo(DVec3d::UnitZ()))
			{
				continue;
			}

			// ��Щǽ�ı���һ�����б��ƽ��z�����޷�����
			// ������z�ļнǣ�С��10��Ҳ��ƽ��
			if (edge_dir.AngleTo(DVec3d::UnitZ()) < 10.0 / 180.0 * PI)
			{
				continue;
			}

			// ʣ�µ���ȡһ������
			direction = edge_dir;
			break;
		}
		direction.Normalize();

		// ����͵������faces - side_faces (�)
		// std::set_difference��Ҫ�����򼯺ϲ���
		std::sort(wall_faces.begin(), wall_faces.end());
		std::sort(side_faces.begin(), side_faces.end());

		std::vector<EditElementHandle *> bottom_and_top_faces;
		std::set_difference(
			wall_faces.cbegin(), wall_faces.cend(),
			side_faces.cbegin(), side_faces.cend(),
			std::inserter(bottom_and_top_faces, bottom_and_top_faces.begin()));

		// ���ݰ�Χ�и߶����ֶ���͵���
		auto min_max_face = std::minmax_element(
			bottom_and_top_faces.cbegin(),
			bottom_and_top_faces.cend(),
			[](const EditElementHandleP &face_a, const EditElementHandleP &face_b) -> bool
			{
				// �����İ�Χ��
				DPoint3d range_min_a, range_min_b;
				DPoint3d range_max_a, range_max_b;
				mdlElmdscr_computeRange(&range_min_a, &range_max_a, face_a->GetElementDescrCP(), nullptr);
				mdlElmdscr_computeRange(&range_min_b, &range_max_b, face_b->GetElementDescrCP(), nullptr);

				// �����Χ����z�����ϵľ�ֵ���Ը�ֵ���Ƚ�
				auto z_average_a = (range_max_a.z - range_min_a.z) / 2.0;
				auto z_average_b = (range_max_b.z - range_min_b.z) / 2.0;

				return z_average_a < z_average_b;
			});
		auto bottom_iter = min_max_face.first;
		auto top_iter = min_max_face.second;

		if (bottom_iter == bottom_and_top_faces.end() || top_iter == bottom_and_top_faces.end())
		{
			// û���ҵ���������
			return false;
		}

		// ȡ���˶���͵���
		auto bottom_face = *bottom_iter;
		auto top_face = *top_iter;

		// ��һ�����������
		wall_normal.Normalize();
		longest_edge_vec.Normalize();

		// ����ǽ�ĸ߶�
		DRange3d wall_range;
		mdlElmdscr_computeRange(&wall_range.low, &wall_range.high, wall.GetElementDescrCP(), nullptr);

		// д�뵽info
		out_geometry_info.wall = &wall;
		out_geometry_info.normal = wall_normal;
		out_geometry_info.direction = direction;
		out_geometry_info.front_face = front_face;
		out_geometry_info.back_face = back_face;
		out_geometry_info.top_face = top_face;
		out_geometry_info.bottom_face = bottom_face;
		out_geometry_info.height = wall_range.high.z - wall_range.low.z;
		out_geometry_info.thickness = thickness;

		out_geometry_info.faces_count = wall_faces.size();
		for (auto i = 0; i < wall_faces.size(); ++i)
		{
			out_geometry_info.faces[i] = wall_faces[i];
		}

		return true;
	}

	bool WallHelper::analysis_associated(const ElementHandle &wall, std::vector<Associated> &associated, WallGeometryInfo &wall_geometry_info)
	{
		// ��õ�ǰԪ�صİ�Χ��
		DRange3d wall_range;
		mdlElmdscr_computeRange(&wall_range.low, &wall_range.high, wall.GetElementDescrCP(), nullptr);

		double __EPS__ = 1 * UOR_PER_MilliMeter;
		// ����ɨ��������ǽ��range
		// ���ں���������
		// ��Сz,��ֹɨ����
		auto assoc_wall_range = wall_range;

		assoc_wall_range.low.x -= __EPS__;
		assoc_wall_range.low.y -= __EPS__;
		assoc_wall_range.low.z += __EPS__;

		assoc_wall_range.high.x += __EPS__;
		assoc_wall_range.high.y += __EPS__;
		assoc_wall_range.high.z -= __EPS__;

		// ɨ�踽����ǽ
		auto assoc_walls =
			scan_elements_in_range(
				assoc_wall_range,
				[&](const ElementHandle &eh) -> bool {
					if (eh.GetElementId() == wall.GetElementId())
					{
						// ���˵��Լ�
						return false;
					}
					// ֻ��Ҫǽ
					return is_wall(eh);
				});

		// ���뵽�����,��Ҫת����Associated
		std::transform(
			assoc_walls.cbegin(), assoc_walls.cend(),
			std::back_inserter(associated),
			[](const ElementHandle &handle) -> Associated {
				Associated assoc;
				assoc.element = handle;
				assoc.type = AssociatedType::Wall;
				return assoc;
			}
		);
		/*/////////////////////////*/
		// ����ɨ�������İ��range
		// ��������������
		// ��Сx��y��ֹɨ�����ϵİ��ǽ
		// addnew : ��ǽ�ķ��߷�����������ɨ��ķ�Χ������������壬��������ϲ���һ���壬����ǽ֮����T��ǽ����
		auto assoc_slab_range = wall_range;
		DPoint3d my_normal{ wall_geometry_info.normal };//ǽ�ķ�����
		DPoint3d vec_x = DPoint3d::From(assoc_slab_range.high.x - assoc_slab_range.low.x, 0, 0);
		DPoint3d vec_y = DPoint3d::From(0, assoc_slab_range.high.y - assoc_slab_range.low.y, 0);
		vec_x.Normalize();
		vec_y.Normalize();
		if (fabs(fabs(my_normal.DotProduct(vec_x)) - 1) < 1e-6)
		{
			assoc_slab_range.low.x -= __EPS__;
			assoc_slab_range.low.y += 340 * __EPS__;
			assoc_slab_range.low.z -= __EPS__;

			assoc_slab_range.high.x += __EPS__;
			assoc_slab_range.high.y -= 340 * __EPS__;
			assoc_slab_range.high.z += __EPS__;
		}
		else if (fabs(fabs(my_normal.DotProduct(vec_y)) - 1) < 1e-6)
		{
			assoc_slab_range.low.x += 340 * __EPS__;
			assoc_slab_range.low.y -= __EPS__;
			assoc_slab_range.low.z -= __EPS__;

			assoc_slab_range.high.x -= 340 * __EPS__;
			assoc_slab_range.high.y += __EPS__;
			assoc_slab_range.high.z += __EPS__;
		}
		else
		{
			assoc_wall_range.low.x += 850 * __EPS__;
			assoc_wall_range.low.y += 850 * __EPS__;
			assoc_wall_range.low.z -= 5 * __EPS__;

			assoc_wall_range.high.x -= 850 * __EPS__;
			assoc_wall_range.high.y -= 850 * __EPS__;
			assoc_wall_range.high.z += 5 * __EPS__;
		}
		/*////////////////////////////*/

		// ����ɨ�������İ��range
		// ��������������
		// ��Сx��y��ֹɨ�����ϵİ��ǽ
		/*auto assoc_slab_range = wall_range;

		assoc_slab_range.low.x += __EPS__;
		assoc_slab_range.low.y += __EPS__;
		assoc_slab_range.low.z -= __EPS__;

		assoc_slab_range.high.x -= __EPS__;
		assoc_slab_range.high.y -= __EPS__;
		assoc_slab_range.high.z += __EPS__;*/

		// ɨ�踽���İ�
		auto assoc_slabs =
			scan_elements_in_range(
				assoc_slab_range,
				[&](const ElementHandle &eh) -> bool {
					if (eh.GetElementId() == wall.GetElementId())
					{
						// ���˵��Լ�
						return false;
					}
					// ֻ��Ҫ��
					return SlabHelper::is_slab(eh);
				});
		/*///////////////////*/
		// ���ɨ�赽�İ��������˵��ǽ��������֮�䣬��������Ҫ����T��ǽ����
		// ��������װ�ֱ�ϲ�֮�󽫺ϲ�֮��İ��ټ��뵽 assoc_slabs
		if (assoc_slabs.size() > 2)
		{
			auto wall_middle = (wall_range.high.z + wall_range.low.z) / 2.0;
			vector<ElementHandle> top_slabs;//ɸѡɨ�赽�İ�֮���ȡ����
			vector<ElementHandle> bottom_slabs;//ɸѡɨ�赽�İ�֮���ȡ�װ�
			for (auto handle : assoc_slabs)
			{
				DRange3d slab_range;
				mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, handle.GetElementDescrCP(), nullptr);

				auto slab_z = (slab_range.high.z + slab_range.low.z) / 2.0;
				if (slab_z > wall_middle)
				{
					top_slabs.push_back(handle);
				}
				else
				{
					bottom_slabs.push_back(handle);
				}
			}
			assoc_slabs.clear();
			vector<MSElementDescrP> top_slab_msep;		//��ȡ���������MSElementDescrP,��ΪSolidBollOperation�Ĳ�����
			vector<MSElementDescrP> bottom_slabs_msep;  //��ȡ�����װ��MSElementDescrP,��ΪSolidBollOperation�Ĳ�����

			if (top_slabs.size() > 0)
			{
				if (top_slabs.size() > 1)
				{
					vector<EditElementHandle*> allFloors;
					EditElementHandle combinefloor;
					vector<EditElementHandle*> holes;
					for (int i = 0; i < top_slabs.size(); ++i)
					{
						EditElementHandle *tmpeeh = new EditElementHandle(top_slabs[i], true);
						allFloors.push_back(tmpeeh);
						tmpeeh = nullptr;
						delete tmpeeh;
					}
					if (CreateFloorDownFaceAndSetData(allFloors, combinefloor, holes))
					{
						combineTop = true;
						ElementHandle tmp1(combinefloor.GetElementId(), combinefloor.GetModelRef());
						assoc_slabs.push_back(tmp1);
					}
					else
					{
						mdlOutput_error(L"�ϲ�������ʧ��");
						return false;
					}

					//for (auto top_slab_eh : top_slabs)
					//{
					//	MSElementDescrP msdp = const_cast<MSElementDescrP>(top_slab_eh.GetElementDescrCP());
					//	top_slab_msep.push_back(msdp);
					//}
					//MSElementDescrP final_top_slab = const_cast<MSElementDescrP>(top_slabs.front().GetElementDescrCP());
					//if (SUCCESS != SolidBoolOperation(final_top_slab, top_slab_msep, BOOLOPERATION_UNITE, ACTIVEMODEL))
					//	return false;
					//DRange3d slab_range;//�����Ƿ�ϲ��ɹ�
					//mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, final_top_slab, nullptr);
					//ElementHandle fslab(final_top_slab, false);
					//assoc_slabs.push_back(fslab);
				}
				else if (top_slabs.size() == 1)
				{
					assoc_slabs.push_back(top_slabs.front());
				}
			}
			if (bottom_slabs.size() > 0)
			{
				if (bottom_slabs.size() > 1)
				{
					vector<EditElementHandle*> allFloors;
					EditElementHandle combinefloor;
					vector<EditElementHandle*> holes;
					for (int i = 0; i < bottom_slabs.size(); ++i)
					{
						EditElementHandle *tmpeehP = new EditElementHandle(bottom_slabs[i], true);
						allFloors.push_back(tmpeehP);
						tmpeehP = nullptr;
						delete tmpeehP;
					}
					if (CreateFloorDownFaceAndSetData(allFloors, combinefloor, holes))
					{
						combineBottom = true;
						ElementHandle tmp1(combinefloor.GetElementId(), combinefloor.GetModelRef());
						assoc_slabs.push_back(tmp1);
					}
					else
					{
						mdlOutput_error(L"�ϲ��ײ���ʧ��");
						return false;
					}

					//for (auto bottom_slab_eh : bottom_slabs)
					//{
					//	MSElementDescrP msdp = const_cast<MSElementDescrP>(bottom_slab_eh.GetElementDescrCP());
					//	bottom_slabs_msep.push_back(msdp);
					//}
					//MSElementDescrP final_bottom_slab = const_cast<MSElementDescrP>(bottom_slabs.back().GetElementDescrCP());
					//if (SUCCESS != SolidBoolOperation(final_bottom_slab, top_slab_msep, BOOLOPERATION_UNITE, ACTIVEMODEL))
					//	return false;
					//DRange3d slab_range;//�����Ƿ�ϲ��ɹ�
					//mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, final_bottom_slab, nullptr);
					//ElementHandle fslab(final_bottom_slab, false);
					//assoc_slabs.push_back(fslab);
				}
				else if (bottom_slabs.size() == 1)
				{
					assoc_slabs.push_back(bottom_slabs.front());
				}
			}
		}
		/*///////////////////*/
		// ǽ���������ڸ߶�
		auto wall_z = (wall_range.high.z + wall_range.low.z) / 2.0;

		// ���뵽�����,��Ҫת����Associated
		std::transform(
			assoc_slabs.cbegin(), assoc_slabs.cend(),
			std::back_inserter(associated),
			[=](const ElementHandle &handle) -> Associated {
				// ��������ڵĸ߶ȣ��ж��Ƿ�Ϊ����
				DRange3d slab_range;
				mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, handle.GetElementDescrCP(), nullptr);

				auto slab_z = (slab_range.high.z + slab_range.low.z) / 2.0;

				Associated assoc;
				assoc.element = handle;
				if (slab_z > wall_z)
				{
					assoc.type = AssociatedType::TopFloor;
				}
				else
				{
					assoc.type = AssociatedType::BottomFloor;
				}

				return assoc;
			}
		);

		return true;
	}

	bool WallHelper::GetAssociatedWallAndFloor(const ElementHandle & wall, std::vector<Associated>& associated, WallGeometryInfo & wall_geometry_info)
	{
		// ��õ�ǰԪ�صİ�Χ��
		DRange3d wall_range;
		mdlElmdscr_computeRange(&wall_range.low, &wall_range.high, wall.GetElementDescrCP(), nullptr);

		double __EPS__ = 1 * UOR_PER_MilliMeter;
		// ����ɨ��������ǽ��range
		// ���ں���������
		// ��Сz,��ֹɨ����
		auto assoc_wall_range = wall_range;

		assoc_wall_range.low.x -= __EPS__;
		assoc_wall_range.low.y -= __EPS__;
		assoc_wall_range.low.z += __EPS__;

		assoc_wall_range.high.x += __EPS__;
		assoc_wall_range.high.y += __EPS__;
		assoc_wall_range.high.z -= __EPS__;

		// ɨ�踽����ǽ
		auto assoc_walls =
			scan_elements_in_range(
				assoc_wall_range,
				[&](const ElementHandle &eh) -> bool {
			if (eh.GetElementId() == wall.GetElementId())
			{
				// ���˵��Լ�
				return false;
			}
			// ֻ��Ҫǽ
			return is_wall(eh);
		});

		// ���뵽�����,��Ҫת����Associated
		std::transform(
			assoc_walls.cbegin(), assoc_walls.cend(),
			std::back_inserter(associated),
			[](const ElementHandle &handle) -> Associated {
			Associated assoc;
			assoc.element = handle;
			assoc.type = AssociatedType::Wall;
			return assoc;
		}
		);
		/*/////////////////////////*/
		// ����ɨ�������İ��range
		// ��������������
		// ��Сx��y��ֹɨ�����ϵİ��ǽ
		// addnew : ��ǽ�ķ��߷�����������ɨ��ķ�Χ������������壬��������ϲ���һ���壬����ǽ֮����T��ǽ����
		auto assoc_slab_range = wall_range;
		DPoint3d my_normal{ wall_geometry_info.normal };//ǽ�ķ�����
		DPoint3d vec_x = DPoint3d::From(assoc_slab_range.high.x - assoc_slab_range.low.x, 0, 0);
		DPoint3d vec_y = DPoint3d::From(0, assoc_slab_range.high.y - assoc_slab_range.low.y, 0);
		vec_x.Normalize();
		vec_y.Normalize();
		if (fabs(fabs(my_normal.DotProduct(vec_x)) - 1) < 1e-6)
		{
			assoc_slab_range.low.x -= __EPS__;
			assoc_slab_range.low.y += 340 * __EPS__;
			assoc_slab_range.low.z -= __EPS__;

			assoc_slab_range.high.x += __EPS__;
			assoc_slab_range.high.y -= 340 * __EPS__;
			assoc_slab_range.high.z += __EPS__;
		}
		else if (fabs(fabs(my_normal.DotProduct(vec_y)) - 1) < 1e-6)
		{
			assoc_slab_range.low.x += 340 * __EPS__;
			assoc_slab_range.low.y -= __EPS__;
			assoc_slab_range.low.z -= __EPS__;

			assoc_slab_range.high.x -= 340 * __EPS__;
			assoc_slab_range.high.y += __EPS__;
			assoc_slab_range.high.z += __EPS__;
		}
		else
		{
			assoc_wall_range.low.x += 850 * __EPS__;
			assoc_wall_range.low.y += 850 * __EPS__;
			assoc_wall_range.low.z -= 5 * __EPS__;

			assoc_wall_range.high.x -= 850 * __EPS__;
			assoc_wall_range.high.y -= 850 * __EPS__;
			assoc_wall_range.high.z += 5 * __EPS__;
		}
		/*////////////////////////////*/

		// ����ɨ�������İ��range
		// ��������������
		// ��Сx��y��ֹɨ�����ϵİ��ǽ
		/*auto assoc_slab_range = wall_range;

		assoc_slab_range.low.x += __EPS__;
		assoc_slab_range.low.y += __EPS__;
		assoc_slab_range.low.z -= __EPS__;

		assoc_slab_range.high.x -= __EPS__;
		assoc_slab_range.high.y -= __EPS__;
		assoc_slab_range.high.z += __EPS__;*/

		// ɨ�踽���İ�
		auto assoc_slabs =
			scan_elements_in_range(
				assoc_slab_range,
				[&](const ElementHandle &eh) -> bool {
			if (eh.GetElementId() == wall.GetElementId())
			{
				// ���˵��Լ�
				return false;
			}
			// ֻ��Ҫ��
			return SlabHelper::is_slab(eh);
		});
		/*///////////////////*/
		// ���ɨ�赽�İ��������˵��ǽ��������֮��
		if (assoc_slabs.size() > 2)
		{
			auto wall_middle = (wall_range.high.z + wall_range.low.z) / 2.0;
			vector<ElementHandle> top_slabs;//ɸѡɨ�赽�İ�֮���ȡ����
			vector<ElementHandle> bottom_slabs;//ɸѡɨ�赽�İ�֮���ȡ�װ�
			for (auto handle : assoc_slabs)
			{
				DRange3d slab_range;
				mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, handle.GetElementDescrCP(), nullptr);

				auto slab_z = (slab_range.high.z + slab_range.low.z) / 2.0;
				if (slab_z > wall_middle)
				{
					top_slabs.push_back(handle);
				}
				else
				{
					bottom_slabs.push_back(handle);
				}
			}
			assoc_slabs.clear();
			vector<MSElementDescrP> top_slab_msep;		//��ȡ���������MSElementDescrP,��ΪSolidBollOperation�Ĳ�����
			vector<MSElementDescrP> bottom_slabs_msep;  //��ȡ�����װ��MSElementDescrP,��ΪSolidBollOperation�Ĳ�����

			if (top_slabs.size() > 0)
			{
				for (size_t i = 0; i < top_slabs.size(); i++)
				{
					assoc_slabs.push_back(top_slabs[i]);
				}

				//if (top_slabs.size() > 1)
				//{
				//	vector<EditElementHandle*> allFloors;
				//	EditElementHandle combinefloor;
				//	vector<EditElementHandle*> holes;
				//	for (int i = 0; i < top_slabs.size(); ++i)
				//	{
				//		EditElementHandle *tmpeeh = new EditElementHandle(top_slabs[i], true);
				//		allFloors.push_back(tmpeeh);
				//		tmpeeh = nullptr;
				//		delete tmpeeh;
				//	}
				//	if (CreateFloorDownFaceAndSetData(allFloors, combinefloor, holes))
				//	{
				//		combineTop = true;
				//		ElementHandle tmp1(combinefloor.GetElementId(), combinefloor.GetModelRef());
				//		assoc_slabs.push_back(tmp1);
				//	}
				//	else
				//	{
				//		mdlOutput_error(L"�ϲ�������ʧ��");
				//		return false;
				//	}

				//	//for (auto top_slab_eh : top_slabs)
				//	//{
				//	//	MSElementDescrP msdp = const_cast<MSElementDescrP>(top_slab_eh.GetElementDescrCP());
				//	//	top_slab_msep.push_back(msdp);
				//	//}
				//	//MSElementDescrP final_top_slab = const_cast<MSElementDescrP>(top_slabs.front().GetElementDescrCP());
				//	//if (SUCCESS != SolidBoolOperation(final_top_slab, top_slab_msep, BOOLOPERATION_UNITE, ACTIVEMODEL))
				//	//	return false;
				//	//DRange3d slab_range;//�����Ƿ�ϲ��ɹ�
				//	//mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, final_top_slab, nullptr);
				//	//ElementHandle fslab(final_top_slab, false);
				//	//assoc_slabs.push_back(fslab);
				//}
				//else if (top_slabs.size() == 1)
				{
					assoc_slabs.push_back(top_slabs.front());
				}
			}
			if (bottom_slabs.size() > 0)
			{
				for (size_t i = 0; i < bottom_slabs.size(); i++)
				{
					assoc_slabs.push_back(bottom_slabs[i]);
				}

				//if (bottom_slabs.size() > 1)
				//{
				//	vector<EditElementHandle*> allFloors;
				//	EditElementHandle combinefloor;
				//	vector<EditElementHandle*> holes;
				//	for (int i = 0; i < bottom_slabs.size(); ++i)
				//	{
				//		EditElementHandle *tmpeehP = new EditElementHandle(bottom_slabs[i], true);
				//		allFloors.push_back(tmpeehP);
				//		tmpeehP = nullptr;
				//		delete tmpeehP;
				//	}
				//	if (CreateFloorDownFaceAndSetData(allFloors, combinefloor, holes))
				//	{
				//		combineBottom = true;
				//		ElementHandle tmp1(combinefloor.GetElementId(), combinefloor.GetModelRef());
				//		assoc_slabs.push_back(tmp1);
				//	}
				//	else
				//	{
				//		mdlOutput_error(L"�ϲ��ײ���ʧ��");
				//		return false;
				//	}

				//	//for (auto bottom_slab_eh : bottom_slabs)
				//	//{
				//	//	MSElementDescrP msdp = const_cast<MSElementDescrP>(bottom_slab_eh.GetElementDescrCP());
				//	//	bottom_slabs_msep.push_back(msdp);
				//	//}
				//	//MSElementDescrP final_bottom_slab = const_cast<MSElementDescrP>(bottom_slabs.back().GetElementDescrCP());
				//	//if (SUCCESS != SolidBoolOperation(final_bottom_slab, top_slab_msep, BOOLOPERATION_UNITE, ACTIVEMODEL))
				//	//	return false;
				//	//DRange3d slab_range;//�����Ƿ�ϲ��ɹ�
				//	//mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, final_bottom_slab, nullptr);
				//	//ElementHandle fslab(final_bottom_slab, false);
				//	//assoc_slabs.push_back(fslab);
				//}
				//else if (bottom_slabs.size() == 1)
				//{
				//	assoc_slabs.push_back(bottom_slabs.front());
				//}
			}
		}
		/*///////////////////*/
		// ǽ���������ڸ߶�
		auto wall_z = (wall_range.high.z + wall_range.low.z) / 2.0;

		// ���뵽�����,��Ҫת����Associated
		std::transform(
			assoc_slabs.cbegin(), assoc_slabs.cend(),
			std::back_inserter(associated),
			[=](const ElementHandle &handle) -> Associated {
			// ��������ڵĸ߶ȣ��ж��Ƿ�Ϊ����
			DRange3d slab_range;
			mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, handle.GetElementDescrCP(), nullptr);

			auto slab_z = (slab_range.high.z + slab_range.low.z) / 2.0;

			Associated assoc;
			assoc.element = handle;
			if (slab_z > wall_z)
			{
				assoc.type = AssociatedType::TopFloor;
			}
			else
			{
				assoc.type = AssociatedType::BottomFloor;
			}

			return assoc;
		}
		);

		return true;
	}

	bool WallHelper::analysis_slab_isTop(const ElementHandle& slab)
	{
		// ��õ�ǰԪ�صİ�Χ��
		DRange3d slab_range;
		mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, slab.GetElementDescrCP(), nullptr);

		double __EPS__ = 1 * UOR_PER_MilliMeter;
		// ����ɨ�������İ��range
	   // ��������������
	   // ��Сx��y��ֹɨ�����ϵİ��ǽ
		auto assoc_slab_range = slab_range;

		assoc_slab_range.low.x -= __EPS__;
		assoc_slab_range.low.y -= __EPS__;
		assoc_slab_range.low.z -= __EPS__;

		assoc_slab_range.high.x -= __EPS__;
		assoc_slab_range.high.y -= __EPS__;
		assoc_slab_range.high.z += __EPS__;

		// ɨ�踽����ǽ
		auto assoc_walls =
			scan_elements_in_range(
				assoc_slab_range,
				[&](const ElementHandle &eh) -> bool {
					if (eh.GetElementId() == slab.GetElementId())
					{
						// ���˵��Լ�
						return false;
					}
					// ֻ��Ҫǽ
					return is_wall(eh);
				});

		/*///////////////////*/
		 // ���������ڸ߶�
		auto slab_z = (slab_range.high.z + slab_range.low.z) / 2.0;

		// ���뵽�����,��Ҫת����Associated
		for (auto it = assoc_walls.begin(); it != assoc_walls.end(); it++)
		{
			DRange3d slab_range;
			mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, it->GetElementDescrCP(), nullptr);
			auto wall_z = (slab_range.high.z + slab_range.low.z) / 2.0;

			Associated assoc;
			assoc.element = *it;
			if (slab_z > wall_z)
			{
				assoc.type = AssociatedType::TopFloor;
				return true;
			}
			else
			{
				assoc.type = AssociatedType::BottomFloor;
				return false;
			}

			
		}
		return false;
	}


	bool WallHelper::analysis_slab_position_with_wall(const ElementHandle &wall, const DVec3d &wall_normal, const ElementHandle &slab, DVec3d &out_slab_direction)
	{
		// ����ǽ�ķ�����RotMatrix
		// �����ǰ�x��ת��ǽ�ķ�������ȥ,
		// �ھ�������任���AABB��Χ�в���׼ȷ��
		RotMatrix rot_matrix;
		auto x_axis = DVec3d::From(1, 0, 0);
		auto wall_normal_point = DPoint3d(wall_normal);
		mdlRMatrix_fromVectorToVector(&rot_matrix, &wall_normal_point, &x_axis);

		// ����ǽ��AABB��Χ�У����һ����������rot_matrix
		DRange3d wall_range;
		mdlElmdscr_computeRange(&wall_range.low, &wall_range.high, wall.GetElementDescrCP(), &rot_matrix);

		// �����İ�Χ�У�ͬ��
		DRange3d slab_range;
		mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, slab.GetElementDescrCP(), &rot_matrix);

		// ��Ϊ������ת����x�ᣬ������Ҫ�жϰ�Χ�е�x��λ�ù�ϵ
		auto wall_min = wall_range.low.x;
		auto wall_max = wall_range.high.x;
		auto slab_min = slab_range.low.x;
		auto slab_max = slab_range.high.x;

		if (slab_max < wall_max || COMPARE_VALUES_EPS(slab_max, wall_max, 1e-1) == 0)
		{
			out_slab_direction = wall_normal;
			out_slab_direction.Negate();
			return true;
		}

		if (wall_min < slab_min || COMPARE_VALUES_EPS(wall_min, slab_min, 1e-1) == 0)
		{
			out_slab_direction = wall_normal;
			return true;
		}

		if (slab_min < wall_min && slab_max > wall_max)
		{
			out_slab_direction = DVec3d::FromZero();
			return true;
		}

		return false;
	}

	bool WallHelper::analysis_inside_outside_faces_by_slab(const WallGeometryInfo &wall_geometry, const ElementHandle &slab, WallInsideOutsideFaces &out_faces)
	{
		const auto &wall = *wall_geometry.wall;

		// ����ǽ����λ�ù�ϵ
		DVec3d slab_direction;
		if (!analysis_slab_position_with_wall(wall_geometry, slab, slab_direction))
		{
			return false;
		}

		// ��������ʾT��ǽ
		if (slab_direction.IsZero())
		{
			out_faces.inside_faces.push_back(wall_geometry.front_face);
			out_faces.inside_faces.push_back(wall_geometry.back_face);
			return true;
		}

		// ����ǽ�ķ�����RotMatrix
		// �����ǰ�ǽ�ķ���ת��x����ȥ����Ϊǽ������б�ŵ�
		// ֱ�����Χ�л�õ�����ȷ�Ľ�� 
		// �ھ�������任���AABB��Χ�в���׼ȷ��
		RotMatrix rot_matrix;
		auto x_axis = DVec3d::From(1, 0, 0);
		auto wall_normal_point = DPoint3d(wall_geometry.normal);
		mdlRMatrix_fromVectorToVector(&rot_matrix, &wall_normal_point, &x_axis);

		// ����������Ҫ�ж�front_face��back_face�ڸ�����ͶӰ�ϵ�λ�ù�ϵ
		// �����������Χ�е��е���Ϊλ�õ��ж�
		DRange3d front_range, back_range;
		mdlElmdscr_computeRange(&front_range.low, &front_range.high, wall_geometry.front_face->GetElementDescrCP(), &rot_matrix);
		mdlElmdscr_computeRange(&back_range.low, &back_range.high, wall_geometry.back_face->GetElementDescrCP(), &rot_matrix);

		// �����е�
		// �������е�����ת������е�
		auto front_mid_point = range_mid_point(front_range);
		auto back_mid_point = range_mid_point(back_range);

		// �԰�ĳ������ø���ת
		rot_matrix.Multiply(slab_direction);

		// ����Щ�㿴��������(point - (0,0,0))
		// ͶӰ����ĳ�����
		auto projection_front = front_mid_point.DotProduct(slab_direction);
		auto projection_back = back_mid_point.DotProduct(slab_direction);

		// ͶӰ���������ڲ�
		if (projection_front > projection_back)
		{
			out_faces.inside_faces.push_back(wall_geometry.front_face);
			out_faces.outside_faces.push_back(wall_geometry.back_face);
		}
		else
		{
			out_faces.inside_faces.push_back(wall_geometry.back_face);
			out_faces.outside_faces.push_back(wall_geometry.front_face);
		}

		return true;
	}

	bool WallHelper::is_inside_face(const WallInsideOutsideFaces &faces, const EditElementHandle *face)
	{
		auto result = std::find(faces.inside_faces.cbegin(), faces.inside_faces.cend(), face);
		return result != faces.inside_faces.cend();
	}

	bool WallHelper::is_outside_face(const WallInsideOutsideFaces &faces, const EditElementHandle *face)
	{
		auto result = std::find(faces.outside_faces.cbegin(), faces.outside_faces.cend(), face);
		return result != faces.outside_faces.cend();
	}

	std::vector<ElementHandle> WallHelper::scan_elements_in_range(const DRange3d &range, std::function<bool(const ElementHandle &)> filter)
	{
		std::vector<ElementHandle> ehs;

		std::map<ElementId, IComponent *> components;
		std::map<ElementId, MSElementDescrP> descriptions;

		/// ɨ���ڰ�Χ�з�Χ�ڵĹ���
		if (!PITCommonTool::CModelTool::AnalysisModelGetElements(range, components, descriptions))
		{
			// ���ؿյļ���
			return ehs;
		}

		const auto PLUS_ID = 1000000;

		auto model_refs = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
		for (const auto &kv : components)
		{
			auto id = kv.first;
			// ���ﷵ�ص�id���ܼ���һ��PlusID�Է�ֹ�ظ�
			if (id >= PLUS_ID)
			{
				id -= PLUS_ID;
			}
			const auto component = kv.second;
			// const auto desc = kv.second;

			// ɨ�����е�model_ref���ҵ���Ԫ�����ڵ�model_ref
			for (auto model_ref : model_refs)
			{
				if (model_ref->GetDgnModelP()->FindElementByID(id) == nullptr)
				{
					continue;
				}

				// �ҵ�Ԫ����
				// ����element handle
				auto eh = ElementHandle(id, model_ref);
				if (filter(eh))
				{
					// �������������뵽��� 
					ehs.push_back(std::move(eh));
				}

			}
		}

		// ����+ȥ��
		std::sort(
			ehs.begin(), ehs.end(),
			[](const ElementHandle &eh_a, const ElementHandle &eh_b) -> bool
			{
				return eh_a.GetElementId() < eh_b.GetElementId();
			});

		auto new_end = std::unique(
			ehs.begin(), ehs.end(),
			[](const ElementHandle &eh_a, const ElementHandle &eh_b) -> bool
			{
				return eh_a.GetElementId() == eh_b.GetElementId();
			});

		return std::vector<ElementHandle>(ehs.begin(), new_end);
	}

	DPoint3d WallHelper::range_mid_point(const DRange3d &range)
	{
		DPoint3d point;

		point.x = (range.low.x + range.high.x) / 2.0;
		point.y = (range.low.y + range.high.y) / 2.0;
		point.z = (range.low.z + range.high.z) / 2.0;

		return point;
	}

	WallHelper::WallGeometryInfo::~WallGeometryInfo()
	{
		for (auto eeh_ptr = this->faces; eeh_ptr != this->faces + this->faces_count; ++eeh_ptr)
		{
			delete (*eeh_ptr);
		}
	}
}
