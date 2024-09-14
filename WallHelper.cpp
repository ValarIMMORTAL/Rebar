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

		// 获取墙所有面
		std::vector<EditElementHandleP> wall_faces;
		PITCommonTool::CFaceTool::GetElementAllFaces(wall, wall_faces);

		// 获得面上所有的点并投影到XOY平面上
		std::vector<DPoint2d> wall_points;
		for (auto face : wall_faces)
		{
			// 获得墙面所有的边
			std::vector<MSElementDescrP> edges;
			PITCommonTool::CElementTool::GetALLEdges(*face, edges);

			// 获得墙边的端点
			for (auto edge : edges)
			{
				DPoint3d start_point, end_point;
				mdlElmdscr_extractEndPoints(&start_point, nullptr, &end_point, nullptr, edge, wall.GetModelRef());

				// 投影到XOY并加入wall_points
				wall_points.push_back(DPoint2d::From(start_point));
				wall_points.push_back(DPoint2d::From(end_point));
			}
		}

		// 计算这些点的OBB包围盒
		auto obb = OBB2d::from_points(wall_points.data(), wall_points.size());

		// 获得obb包围盒的长边方向(判断其在x/y方向上哪边更长)
		auto obb_length_x = obb.get_max().x - obb.get_min().x;
		auto obb_length_y = obb.get_max().y - obb.get_min().y;
		auto longest_edge_vec =
			obb_length_x > obb_length_y ? DVec3d::From(obb_length_x, 0, 0) : DVec3d::From(0, obb_length_y, 0);
		// 短边即为墙的厚度
#undef min //这里是bentley定义一个min的宏
		auto thickness = std::min(obb_length_x, obb_length_y);

		// 变换该向量到XOY上
		obb.calculate_transform().Multiply(longest_edge_vec);

		// 计算与该向量垂直的向量即为法向
		auto z_axis = DVec3d::From(0, 0, 1);
		auto wall_normal = DVec3d::FromCrossProduct(longest_edge_vec, z_axis);

		// 过滤出墙的侧面
		std::vector<EditElementHandleP> side_faces;
		for (const auto face_p : wall_faces)
		{
			const auto face_desc_p = face_p->GetElementDescrCP();

			// 获得面的法向
			DPoint3d normal;
			auto z_axis = DPoint3d::From(0, 0, 1);
			mdlElmdscr_extractNormal(&normal, nullptr, face_desc_p, &z_axis);

			if (normal.IsParallelTo(z_axis))
			{
				// 无法确定法向，或者法向就是z轴
				continue;
			}

			if (COMPARE_VALUES_EPS(normal.DotProduct(z_axis), 0, 1e-5) == 0)
			{
				// 与z轴垂直, 即为墙侧面
				side_faces.push_back(face_p);
			}
		}

		// 找到所有与法向夹角小于45度的面，这里包括了正面和背面
		std::vector<EditElementHandleP> front_or_back_faces;
		std::copy_if(
			side_faces.cbegin(), side_faces.cend(),
			std::back_inserter(front_or_back_faces),
			[&](const EditElementHandleP &face) -> bool {
				// 获得面的法向
				DPoint3d normal;
				auto z_axis = DPoint3d::From(0, 0, 1);
				mdlElmdscr_extractNormal(&normal, nullptr, face->GetElementDescrCP(), &z_axis);

				// 计算反的法向
				auto reverse_normal = normal;
				reverse_normal.Negate();

				// 分别计算与两个法向的夹角
				auto angle = wall_normal.AngleTo(DVec3d::From(normal));
				auto angle_reverse = wall_normal.AngleTo(DVec3d::From(reverse_normal));


				// 任意一个方向小于45度即可
				return (std::abs(angle) < PI / 4) || (std::abs(angle_reverse) < PI / 4);
			}
		);

		// 计算这些面包围盒中点，中点在法向上投影进行排序，最大的为正面，最小的为背面
		// 这样可以保证墙法向与墙正面组合指向墙的外部
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

					// 将这些点看成是向量(point - (0,0,0))
					// 根据法向上的投影排序即可
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

		// 获得正面的底边或顶边
		// 取该边方向为墙的方向
		// 获得面的边界
		std::vector<MSElementDescrP> front_face_edges;
		PITCommonTool::CElementTool::GetALLEdges(*front_face, front_face_edges);

		// 获得边界的所有点
		auto direction = longest_edge_vec;
		for (auto edge : front_face_edges)
		{
			// 获得边界的起止点
			DPoint3d start, end;
			mdlElmdscr_extractEndPoints(&start, nullptr, &end, nullptr, edge, front_face->GetModelRef());

			// 边的方向
			auto edge_dir = end - start;

			// 过滤掉平行z的
			if (edge_dir.IsParallelTo(DVec3d::UnitZ()))
			{
				continue;
			}

			// 有些墙的边有一点点倾斜，平行z可能无法过滤
			// 计算与z的夹角，小于10度也算平行
			if (edge_dir.AngleTo(DVec3d::UnitZ()) < 10.0 / 180.0 * PI)
			{
				continue;
			}

			// 剩下的任取一个即可
			direction = edge_dir;
			break;
		}
		direction.Normalize();

		// 顶面和底面就是faces - side_faces (差集)
		// std::set_difference需要对有序集合操作
		std::sort(wall_faces.begin(), wall_faces.end());
		std::sort(side_faces.begin(), side_faces.end());

		std::vector<EditElementHandle *> bottom_and_top_faces;
		std::set_difference(
			wall_faces.cbegin(), wall_faces.cend(),
			side_faces.cbegin(), side_faces.cend(),
			std::inserter(bottom_and_top_faces, bottom_and_top_faces.begin()));

		// 根据包围盒高度区分顶面和底面
		auto min_max_face = std::minmax_element(
			bottom_and_top_faces.cbegin(),
			bottom_and_top_faces.cend(),
			[](const EditElementHandleP &face_a, const EditElementHandleP &face_b) -> bool
			{
				// 获得面的包围盒
				DPoint3d range_min_a, range_min_b;
				DPoint3d range_max_a, range_max_b;
				mdlElmdscr_computeRange(&range_min_a, &range_max_a, face_a->GetElementDescrCP(), nullptr);
				mdlElmdscr_computeRange(&range_min_b, &range_max_b, face_b->GetElementDescrCP(), nullptr);

				// 计算包围盒在z方向上的均值，以该值来比较
				auto z_average_a = (range_max_a.z - range_min_a.z) / 2.0;
				auto z_average_b = (range_max_b.z - range_min_b.z) / 2.0;

				return z_average_a < z_average_b;
			});
		auto bottom_iter = min_max_face.first;
		auto top_iter = min_max_face.second;

		if (bottom_iter == bottom_and_top_faces.end() || top_iter == bottom_and_top_faces.end())
		{
			// 没有找到顶面或底面
			return false;
		}

		// 取到了顶面和底面
		auto bottom_face = *bottom_iter;
		auto top_face = *top_iter;

		// 归一化法向和轴向
		wall_normal.Normalize();
		longest_edge_vec.Normalize();

		// 计算墙的高度
		DRange3d wall_range;
		mdlElmdscr_computeRange(&wall_range.low, &wall_range.high, wall.GetElementDescrCP(), nullptr);

		// 写入到info
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
		// 获得当前元素的包围盒
		DRange3d wall_range;
		mdlElmdscr_computeRange(&wall_range.low, &wall_range.high, wall.GetElementDescrCP(), nullptr);

		double __EPS__ = 1 * UOR_PER_MilliMeter;
		// 用于扫描相连的墙的range
		// 仅在横向上扩大
		// 减小z,防止扫到板
		auto assoc_wall_range = wall_range;

		assoc_wall_range.low.x -= __EPS__;
		assoc_wall_range.low.y -= __EPS__;
		assoc_wall_range.low.z += __EPS__;

		assoc_wall_range.high.x += __EPS__;
		assoc_wall_range.high.y += __EPS__;
		assoc_wall_range.high.z -= __EPS__;

		// 扫描附近的墙
		auto assoc_walls =
			scan_elements_in_range(
				assoc_wall_range,
				[&](const ElementHandle &eh) -> bool {
					if (eh.GetElementId() == wall.GetElementId())
					{
						// 过滤掉自己
						return false;
					}
					// 只需要墙
					return is_wall(eh);
				});

		// 加入到输出中,需要转换成Associated
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
		// 用于扫描相连的板的range
		// 仅在竖向上扩大
		// 减小x、y防止扫到边上的板和墙
		// addnew : 在墙的法线方向上面扩大扫描的范围，如果有两个板，则将两个板合并成一个板，板与墙之间做T型墙处理
		auto assoc_slab_range = wall_range;
		DPoint3d my_normal{ wall_geometry_info.normal };//墙的法向量
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

		// 用于扫描相连的板的range
		// 仅在竖向上扩大
		// 减小x、y防止扫到边上的板和墙
		/*auto assoc_slab_range = wall_range;

		assoc_slab_range.low.x += __EPS__;
		assoc_slab_range.low.y += __EPS__;
		assoc_slab_range.low.z -= __EPS__;

		assoc_slab_range.high.x -= __EPS__;
		assoc_slab_range.high.y -= __EPS__;
		assoc_slab_range.high.z += __EPS__;*/

		// 扫描附近的板
		auto assoc_slabs =
			scan_elements_in_range(
				assoc_slab_range,
				[&](const ElementHandle &eh) -> bool {
					if (eh.GetElementId() == wall.GetElementId())
					{
						// 过滤掉自己
						return false;
					}
					// 只需要板
					return SlabHelper::is_slab(eh);
				});
		/*///////////////////*/
		// 如果扫描到的板大于两个说明墙在两个板之间，这个情况需要按照T型墙处理
		// 将顶板与底板分别合并之后将合并之后的板再加入到 assoc_slabs
		if (assoc_slabs.size() > 2)
		{
			auto wall_middle = (wall_range.high.z + wall_range.low.z) / 2.0;
			vector<ElementHandle> top_slabs;//筛选扫描到的板之后存取顶板
			vector<ElementHandle> bottom_slabs;//筛选扫描到的板之后存取底板
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
			vector<MSElementDescrP> top_slab_msep;		//存取两个顶板的MSElementDescrP,作为SolidBollOperation的参数；
			vector<MSElementDescrP> bottom_slabs_msep;  //存取两个底板的MSElementDescrP,作为SolidBollOperation的参数；

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
						mdlOutput_error(L"合并顶部板失败");
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
					//DRange3d slab_range;//测试是否合并成功
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
						mdlOutput_error(L"合并底部板失败");
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
					//DRange3d slab_range;//测试是否合并成功
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
		// 墙的中心所在高度
		auto wall_z = (wall_range.high.z + wall_range.low.z) / 2.0;

		// 加入到输出中,需要转换成Associated
		std::transform(
			assoc_slabs.cbegin(), assoc_slabs.cend(),
			std::back_inserter(associated),
			[=](const ElementHandle &handle) -> Associated {
				// 计算板所在的高度，判断是否为顶板
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
		// 获得当前元素的包围盒
		DRange3d wall_range;
		mdlElmdscr_computeRange(&wall_range.low, &wall_range.high, wall.GetElementDescrCP(), nullptr);

		double __EPS__ = 1 * UOR_PER_MilliMeter;
		// 用于扫描相连的墙的range
		// 仅在横向上扩大
		// 减小z,防止扫到板
		auto assoc_wall_range = wall_range;

		assoc_wall_range.low.x -= __EPS__;
		assoc_wall_range.low.y -= __EPS__;
		assoc_wall_range.low.z += __EPS__;

		assoc_wall_range.high.x += __EPS__;
		assoc_wall_range.high.y += __EPS__;
		assoc_wall_range.high.z -= __EPS__;

		// 扫描附近的墙
		auto assoc_walls =
			scan_elements_in_range(
				assoc_wall_range,
				[&](const ElementHandle &eh) -> bool {
			if (eh.GetElementId() == wall.GetElementId())
			{
				// 过滤掉自己
				return false;
			}
			// 只需要墙
			return is_wall(eh);
		});

		// 加入到输出中,需要转换成Associated
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
		// 用于扫描相连的板的range
		// 仅在竖向上扩大
		// 减小x、y防止扫到边上的板和墙
		// addnew : 在墙的法线方向上面扩大扫描的范围，如果有两个板，则将两个板合并成一个板，板与墙之间做T型墙处理
		auto assoc_slab_range = wall_range;
		DPoint3d my_normal{ wall_geometry_info.normal };//墙的法向量
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

		// 用于扫描相连的板的range
		// 仅在竖向上扩大
		// 减小x、y防止扫到边上的板和墙
		/*auto assoc_slab_range = wall_range;

		assoc_slab_range.low.x += __EPS__;
		assoc_slab_range.low.y += __EPS__;
		assoc_slab_range.low.z -= __EPS__;

		assoc_slab_range.high.x -= __EPS__;
		assoc_slab_range.high.y -= __EPS__;
		assoc_slab_range.high.z += __EPS__;*/

		// 扫描附近的板
		auto assoc_slabs =
			scan_elements_in_range(
				assoc_slab_range,
				[&](const ElementHandle &eh) -> bool {
			if (eh.GetElementId() == wall.GetElementId())
			{
				// 过滤掉自己
				return false;
			}
			// 只需要板
			return SlabHelper::is_slab(eh);
		});
		/*///////////////////*/
		// 如果扫描到的板大于两个说明墙在两个板之间
		if (assoc_slabs.size() > 2)
		{
			auto wall_middle = (wall_range.high.z + wall_range.low.z) / 2.0;
			vector<ElementHandle> top_slabs;//筛选扫描到的板之后存取顶板
			vector<ElementHandle> bottom_slabs;//筛选扫描到的板之后存取底板
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
			vector<MSElementDescrP> top_slab_msep;		//存取两个顶板的MSElementDescrP,作为SolidBollOperation的参数；
			vector<MSElementDescrP> bottom_slabs_msep;  //存取两个底板的MSElementDescrP,作为SolidBollOperation的参数；

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
				//		mdlOutput_error(L"合并顶部板失败");
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
				//	//DRange3d slab_range;//测试是否合并成功
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
				//		mdlOutput_error(L"合并底部板失败");
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
				//	//DRange3d slab_range;//测试是否合并成功
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
		// 墙的中心所在高度
		auto wall_z = (wall_range.high.z + wall_range.low.z) / 2.0;

		// 加入到输出中,需要转换成Associated
		std::transform(
			assoc_slabs.cbegin(), assoc_slabs.cend(),
			std::back_inserter(associated),
			[=](const ElementHandle &handle) -> Associated {
			// 计算板所在的高度，判断是否为顶板
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
		// 获得当前元素的包围盒
		DRange3d slab_range;
		mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, slab.GetElementDescrCP(), nullptr);

		double __EPS__ = 1 * UOR_PER_MilliMeter;
		// 用于扫描相连的板的range
	   // 仅在竖向上扩大
	   // 减小x、y防止扫到边上的板和墙
		auto assoc_slab_range = slab_range;

		assoc_slab_range.low.x -= __EPS__;
		assoc_slab_range.low.y -= __EPS__;
		assoc_slab_range.low.z -= __EPS__;

		assoc_slab_range.high.x -= __EPS__;
		assoc_slab_range.high.y -= __EPS__;
		assoc_slab_range.high.z += __EPS__;

		// 扫描附近的墙
		auto assoc_walls =
			scan_elements_in_range(
				assoc_slab_range,
				[&](const ElementHandle &eh) -> bool {
					if (eh.GetElementId() == slab.GetElementId())
					{
						// 过滤掉自己
						return false;
					}
					// 只需要墙
					return is_wall(eh);
				});

		/*///////////////////*/
		 // 板中心所在高度
		auto slab_z = (slab_range.high.z + slab_range.low.z) / 2.0;

		// 加入到输出中,需要转换成Associated
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
		// 根据墙的法向构造RotMatrix
		// 这里是把x轴转到墙的法向方向上去,
		// 在经过这个变换后的AABB包围盒才是准确的
		RotMatrix rot_matrix;
		auto x_axis = DVec3d::From(1, 0, 0);
		auto wall_normal_point = DPoint3d(wall_normal);
		mdlRMatrix_fromVectorToVector(&rot_matrix, &wall_normal_point, &x_axis);

		// 计算墙的AABB包围盒，最后一个参数传入rot_matrix
		DRange3d wall_range;
		mdlElmdscr_computeRange(&wall_range.low, &wall_range.high, wall.GetElementDescrCP(), &rot_matrix);

		// 计算板的包围盒，同上
		DRange3d slab_range;
		mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, slab.GetElementDescrCP(), &rot_matrix);

		// 因为法向旋转到了x轴，所以需要判断包围盒的x的位置关系
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

		// 计算墙与板的位置关系
		DVec3d slab_direction;
		if (!analysis_slab_position_with_wall(wall_geometry, slab, slab_direction))
		{
			return false;
		}

		// 零向量表示T型墙
		if (slab_direction.IsZero())
		{
			out_faces.inside_faces.push_back(wall_geometry.front_face);
			out_faces.inside_faces.push_back(wall_geometry.back_face);
			return true;
		}

		// 根据墙的法向构造RotMatrix
		// 这里是把墙的法向转到x轴上去，因为墙可能是斜着的
		// 直接求包围盒会得到不正确的结果 
		// 在经过这个变换后的AABB包围盒才是准确的
		RotMatrix rot_matrix;
		auto x_axis = DVec3d::From(1, 0, 0);
		auto wall_normal_point = DPoint3d(wall_geometry.normal);
		mdlRMatrix_fromVectorToVector(&rot_matrix, &wall_normal_point, &x_axis);

		// 非零向量需要判断front_face和back_face在该向量投影上的位置关系
		// 以这两个面包围盒的中点作为位置的判断
		DRange3d front_range, back_range;
		mdlElmdscr_computeRange(&front_range.low, &front_range.high, wall_geometry.front_face->GetElementDescrCP(), &rot_matrix);
		mdlElmdscr_computeRange(&back_range.low, &back_range.high, wall_geometry.back_face->GetElementDescrCP(), &rot_matrix);

		// 计算中点
		// 这两个中点是旋转过后的中点
		auto front_mid_point = range_mid_point(front_range);
		auto back_mid_point = range_mid_point(back_range);

		// 对板的朝向作用该旋转
		rot_matrix.Multiply(slab_direction);

		// 将这些点看成是向量(point - (0,0,0))
		// 投影到板的朝向上
		auto projection_front = front_mid_point.DotProduct(slab_direction);
		auto projection_back = back_mid_point.DotProduct(slab_direction);

		// 投影距离大的是内侧
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

		/// 扫描在包围盒范围内的构件
		if (!PITCommonTool::CModelTool::AnalysisModelGetElements(range, components, descriptions))
		{
			// 返回空的即可
			return ehs;
		}

		const auto PLUS_ID = 1000000;

		auto model_refs = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
		for (const auto &kv : components)
		{
			auto id = kv.first;
			// 这里返回的id可能加了一个PlusID以防止重复
			if (id >= PLUS_ID)
			{
				id -= PLUS_ID;
			}
			const auto component = kv.second;
			// const auto desc = kv.second;

			// 扫描所有的model_ref，找到该元素所在的model_ref
			for (auto model_ref : model_refs)
			{
				if (model_ref->GetDgnModelP()->FindElementByID(id) == nullptr)
				{
					continue;
				}

				// 找到元素了
				// 构建element handle
				auto eh = ElementHandle(id, model_ref);
				if (filter(eh))
				{
					// 满足条件，加入到输出 
					ehs.push_back(std::move(eh));
				}

			}
		}

		// 排序+去重
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
