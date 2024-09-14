#include "_ustation.h"
#include "GeneralFaceRebarAssembly.h"
#include "ExtractFacesTool.h"
#include "PITRebarCurve.h"
#include "PolygonHelper.h"
#include <cmath>
#include <Point3D.h>
#include <Vector3D.h>

/// 定义一个较大的EPS，因为Bentley中单位为UOR，该单位大约是一个um，因此double类型可能精度不够1e-5，一般取1e1~1e-3即可
#define LARGE_EPS 1e-3

namespace Gallery
{

    namespace _local
    {
        /// @brief 以正常的方式扫描
        /// @details 参考ArrayMode::Normal
        std::vector<double> scan_in_range_normal(double spacing, double min, double max)
        {
            std::vector<double> values;

            // value等于max也可以
            for (double value = min; value < max || COMPARE_VALUES_EPS(value, max, LARGE_EPS) == 0; value += spacing)
            {
                values.push_back(value);
            }

            return values;
        }

        /// @brief 以正常的方式扫描
        /// @details 参考ArrayMode::Normal
        std::vector<double> scan_in_range_normal_with_extra(double spacing, double min, double max)
        {
            // 先按正常方式扫描
            auto values = scan_in_range_normal(spacing, min, max);

            // 如果最后一根不等于max
            // 说明没有正好排下
            if(COMPARE_VALUES_EPS(values.back(), max, LARGE_EPS) != 0)
            {
                // 补充一根新的钢筋
                values.push_back(max);
            }

            return values;
        }


        /// @brief 以自适应的方式扫描
        /// @details 参考ArrayMode::Fit
        std::vector<double> scan_in_range_fit(double spacing, double min, double max, double *out_spacing = nullptr)
        {
            auto length = max - min;
            // 间隔个数
            auto count = static_cast<int>(std::floor(length / spacing));
            // 计算余数
            auto remain = length - (count * spacing);
            // 余数为一个比较小的值，说明刚好能放下
            if (COMPARE_VALUES_EPS(remain, 0, LARGE_EPS) == 0)
            {
                // 直接按正常方式布置即可
                if (out_spacing != nullptr)
                {
                    *out_spacing = spacing;
                }
                return scan_in_range_normal(spacing, min, max);
            }

            // 给间隔个数加1
            count += 1;

            // 新的间距
            auto new_spacing = length / static_cast<double>(count);

            if (out_spacing != nullptr)
            {
                *out_spacing = new_spacing;
            }

            // 再以新的间距按正常的方式扫描即可
            return scan_in_range_normal(new_spacing, min, max);
        }

        /// @brief 从点集首尾相连构造出线段
        std::vector<DSegment3d> from_points_to_loop_segments(const std::vector<DPoint2d> &points)
        {
            std::vector<DSegment3d> segments;

            for (auto index = 0; index < points.size(); ++index)
            {
                // 求余防止溢出, 同时也可以实现首尾相连
                auto next_index = (index + 1) % points.size();

                const auto &current = points.at(index);
                const auto &next = points.at(next_index);

                segments.push_back(DSegment3d::From(current, next));
            }

            return segments;
        }

        /// @brief 计算所有线段与y=a的交点
        std::vector<DPoint2d> intersect_with_y(const std::vector<DSegment3d> &segments, double y)
        {
            auto line_y_start = DPoint3d::From(0, y, 0);
            auto line_y_end = DPoint3d::From(1, y, 0);

            std::vector<DPoint2d> cross_points;
            for (const auto &segment : segments)
            {
                const auto &segment_start = segment.point[0];
                const auto &segment_end = segment.point[1];

                // 是否重合判断, 即y相等
                if (COMPARE_VALUES_EPS(segment_start.y, y, LARGE_EPS) == 0 && COMPARE_VALUES_EPS(segment_end.y, y, LARGE_EPS) == 0)
                {
                    // 与y重合，将该线的端点作为交点
                    cross_points.push_back(DPoint2d::From(segment_start));
                    cross_points.push_back(DPoint2d::From(segment_end));

                    continue;
                }

                // 在当前线段上的位置
                double segment_param;
                // 在当前线段上的交点
                DPoint3d cross_point;

                auto result = mdlVec_intersectXYLines(
                    &cross_point,
                    &segment_param,
                    nullptr,
                    nullptr,
                    &segment_start,
                    &segment_end,
                    &line_y_start,
                    &line_y_end);

                if (result != SUCCESS)
                {
                    // 没有交点
                    // 由于mdlVec_intersectXYLines将线段视为直线
                    // 没有交点只可能为平行或重合
                    // 重合的情况前面考虑过了
                    continue;
                }

                // 检查交点是否在线段上
                if (segment_param < 0 || segment_param > 1)
                {
                    continue;
                }

                cross_points.push_back(DPoint2d::From(cross_point));
            }

            // 对结果按x大小排序
            std::sort(
                cross_points.begin(),
                cross_points.end(),
                [](const DPoint2d &a, const DPoint2d &b) -> bool
                {
                    return a.x < b.x;
                });

            // 去掉重合点
            auto new_end = std::unique(
                cross_points.begin(),
                cross_points.end(),
                [](const DPoint2d &a, const DPoint2d &b) -> bool
                {
                    return a.AlmostEqual(b);
                });

            return std::vector<DPoint2d>(cross_points.begin(), new_end);
        }

        /// @brief 转换交点到截线段
        /// @details 需要偶数个交点，两两相连产生线段
        std::vector<DSegment3d> cross_points_to_cross_segments(const std::vector<DPoint2d> points)
        {
            std::vector<DSegment3d> cross_segments;

            if (points.empty() || points.size() % 2 != 0)
            {
                // 如果交点个数为空，或者个数不是2的倍数，无法生成交线
                // 返回空的vec
                return cross_segments;
            }

            // 两个点一组生成线段
            for (auto i = 0, j = i + 1; j < points.size(); i += 2, j = i + 1)
            {
                const auto &start = points.at(i);
                const auto &end = points.at(j);

                auto segment = DSegment3d::From(start, end);

                // 过滤掉长度小于0的线段
                if (COMPARE_VALUES_EPS(segment.Length(), 0, LARGE_EPS) == 0)
                {
                    continue;
                }

                cross_segments.push_back(std::move(segment));
            }

            return cross_segments;
        }

        /// @brief 转换线段到rebar_curve
        /// @param segment 线段
        /// @param start_offset 钢筋的起始端长度偏移(start_offset), 大于0为延长，单位是UOR
        /// @param end_offset 钢筋的结束端长度偏移(start_offset), 大于0为延长，单位是UOR
        /// @param curve rebar_curve
        void from_segments_to_rebar_curve(const DSegment3d &segment, double start_offset, double end_offset , RebarCurve &curve)
        {
            // 复制端点, 之后需要修改
            DPoint3d start_point, end_point;
            segment.GetEndPoints(start_point, end_point);

            // 计算钢筋的方向
            auto rebar_dir = segment.point[1] - segment.point[0];
            // 单位化该方向
            rebar_dir.Normalize();
        
            // 终点的偏移变换
            auto end_transform = Transform::From(rebar_dir * end_offset);
            end_transform.Multiply(end_point);

            // 起点偏移变换
            rebar_dir.Negate();
            auto start_transform = Transform::From(rebar_dir * start_offset);
            start_transform.Multiply(start_point);

            // 只有起始点和终点
            auto &start_vertex = curve.PopVertices().NewElement();
            start_vertex.SetType(RebarVertex::PointType::kStart);
            start_vertex.SetIP(start_point);

            auto &end_vertex = curve.PopVertices().NewElement();
            end_vertex.SetType(RebarVertex::PointType::kEnd);
            end_vertex.SetIP(end_point);
        }

        /// @brief 转换到PIT rebar end type
        /// @param rebar_direction 钢筋方向, 用于计算弯曲时的法向
        /// @param default_normal 默认的法向，可以是钢筋的排列方向
        PIT::PITRebarEndType to_pit_rebar_end_type(const End &end, const DVec3d &rebar_direction, const DVec3d &default_normal)
        {
            PIT::PITRebarEndType pit_end_type;

            /// 用于旋转的临时函数
            /// angle rad是旋转的角度
            auto rotate_by = [&](const DVec3d &axis, double angle_deg)
            {
                // 转换到CVector3d
                CVector3D normal = CVector3D::From(default_normal.x, default_normal.y, default_normal.z);

                // 将默认法向绕轴向绕angle_deg度
                normal.Rotate(angle_deg * PI / 180, rebar_direction);

                return normal;
            };

            switch (end.type)
            {
            case EndType::None:
            case EndType::Bend:
            case EndType::Hook: 
            case EndType::PolygonalLine:
                pit_end_type.SetType(PIT::PITRebarEndType::kNone);
                break;
            case EndType::Hook90:
                pit_end_type.SetType(PIT::PITRebarEndType::kBend);
                pit_end_type.Setangle(end.angle);
                pit_end_type.SetbendRadius(end.data.hook_degree.radius);
                pit_end_type.SetbendLen(end.data.hook_degree.reserver_length);
                pit_end_type.SetendNormal(rotate_by(rebar_direction, end.angle));
                break;
            case EndType::Hook135:
                pit_end_type.SetType(PIT::PITRebarEndType::kCog);
                pit_end_type.Setangle(end.angle);
                pit_end_type.SetbendRadius(end.data.hook_degree.radius);
                pit_end_type.SetbendLen(end.data.hook_degree.reserver_length);
                pit_end_type.SetendNormal(rotate_by(rebar_direction, end.angle));
                break;
            case EndType::Hook180: 
                pit_end_type.SetType(PIT::PITRebarEndType::kHook);
                pit_end_type.Setangle(end.angle);
                pit_end_type.SetbendRadius(end.data.hook_degree.radius);
                pit_end_type.SetbendLen(end.data.hook_degree.reserver_length);
                pit_end_type.SetendNormal(rotate_by(rebar_direction, end.angle));
                break;
            case EndType::Straight: 
                pit_end_type.SetType(PIT::PITRebarEndType::kLap);
                pit_end_type.SetstraightAnchorLen(end.data.straight.length);
                break;
            case EndType::CustomBend:
                pit_end_type.SetType(PIT::PITRebarEndType::kCustom);
                pit_end_type.Setangle(end.angle);
                pit_end_type.SetbendRadius(end.data.custom_bend.radius);
                pit_end_type.SetbendLen(end.data.custom_bend.length);
                pit_end_type.SetendNormal(rotate_by(rebar_direction, end.angle));
                break;
            case EndType::Custom:
            default:
                pit_end_type.SetType(PIT::PITRebarEndType::kNone);
                break;
            }

            return pit_end_type;
        }


        /// @brief 为钢筋线添加端部形状
        /// @param curve 钢筋线
        /// @param start_end 起始端
        /// @param end_end 结束端
        /// @param normal 钢筋生成平面的法向
        /// @param end_types 输出的之前的端部样式
        void add_rebar_end(RebarCurve &curve, const End &start_end, const End &end_end, const DVec3d &normal, RebarEndTypes &end_types)
        {
            // 取两端点，计算钢筋方向
            CPoint3D start_point, end_point;
            curve.GetEndPoints(start_point, end_point);
            auto rebar_vec = end_point - start_point;

            // 转换到PITRebarCurve
            // 这一步转换总是成功，因为PITRebarCurve和RebarCurve内存布局相同
            auto pit_rebar_curve = static_cast<PIT::PITRebarCurve *>(&curve);

            // 转换Gallery::End到PITEndTypes
            PIT::PITRebarEndTypes pit_end_types;
            pit_end_types.beg = to_pit_rebar_end_type(start_end,DVec3d::From(rebar_vec), normal);
            pit_end_types.end = to_pit_rebar_end_type(end_end, DVec3d::From(-rebar_vec),normal);

            // 设置原点
            pit_end_types.beg.SetptOrgin(start_point);
            pit_end_types.end.SetptOrgin(end_point);

            // 输出到外部
            end_types.beg = pit_end_types.beg;
            end_types.end = pit_end_types.end;


            // CustomBend需要单独处理
            if(start_end.type == EndType::CustomBend)
            {
                // 手动生成起始端部
                pit_rebar_curve->RebarEndBendBeg(pit_end_types, rebar_vec, PI - start_end.data.custom_bend.angle / 180 * PI);
            }

            // 生成其它种类的钢筋
            // CustomBend不会在这里被处理
            pit_rebar_curve->EvaluateEndTypes(pit_end_types);

            if (end_end.type == EndType::CustomBend)
            {
                // 手动生成起结束端部
                pit_rebar_curve->RebarEndBendEnd(pit_end_types, -rebar_vec, PI - end_end.data.custom_bend.angle / 180 * PI);
            }

        }
    }

    GeneralFaceRebarAssembly *GeneralFaceRebarAssembly::from_concrete(ElementId concrete_id, DgnModelRefP model_ref)
    {
        RebarAssemblies assemblies;
        // 获得混凝土中所有assembly
        RebarAssembly::GetRebarAssemblies(concrete_id, assemblies, model_ref);

        // 查找当前类型的assembly
        for (auto i = 0; i < assemblies.GetSize(); ++i)
        {
            auto assembly_ptr = assemblies.GetAt(i);

            // 尝试转换成当前类型的指针
            auto this_ptr = dynamic_cast<GeneralFaceRebarAssembly *>(assembly_ptr);

            // 如果转换失败了
            if (this_ptr == nullptr)
            {
                continue;
            }

            // 成功直接返回
            return this_ptr;
        }

        return nullptr;
    }

    GeneralFaceRebarAssembly *GeneralFaceRebarAssembly::create(DgnModelRefP model_ref, ElementId element_id, DgnModelRefP element_model_ref)
    {
        auto p = RebarAssembly::Create<GeneralFaceRebarAssembly>(model_ref, element_id, element_model_ref);
        return p;
    }

    GeneralFaceRebarAssembly::GeneralFaceRebarAssembly(ElementId element_id, DgnModelRefP model_ref)
        : RebarAssembly(element_id, model_ref),
          m_set_tag_count(0),
          m_double_click_callback(nullptr){}

    bool GeneralFaceRebarAssembly::generate_layer(const LayerRebarData &layer_rebar_data)
    {
        const auto uor_per_mm = mdlModelRef_getUorPerMeter(this->GetModelRef()) / 1000;

        // 从size_key读取直径
        const auto diameter = layer_rebar_data.layer->get_rebar_diameter(this->GetModelRef());

        ElementId rebar_set_id = 0;
        // 读取rebar_set_id和rebar_set
        auto rebar_set_p = RebarSet::Fetch(rebar_set_id, this->GetModelRef());
        if (rebar_set_p == nullptr)
        {
            return false;
        }

        // 启动RebarSet的生成
        rebar_set_p->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
        rebar_set_p->SetCallerId(this->GetCallerId());
        {
            rebar_set_p->StartUpdate(this->GetModelRef());

            // 设置钢筋共有的符号参数
            RebarSymbology sym;
            sym.SetRebarLevel(layer_rebar_data.level_name);
            SetRebarColorBySize(layer_rebar_data.layer->size_str, sym);

            RebarEndTypes end_types;

            // 生成具体的钢筋线
            double new_spacing; // 实际的间距
            const auto rebar_curves = GeneralFaceRebarAssembly::gen_rebar_curves(layer_rebar_data, diameter, new_spacing, end_types);
            // 根据钢筋线生成RebarElement
            auto index = 0;
            for (const auto &rebar_curve : rebar_curves)
            {
                // rebar element 代表一根钢筋
                auto rebar_element = rebar_set_p->AssignRebarElement(index, (int)rebar_curves.size(), sym, this->GetModelRef());

                // 设置钢筋的参数, 例如尺寸
                RebarShapeData shape_data;
                shape_data.SetSizeKey(CString(layer_rebar_data.layer->get_full_size_key().c_str()));
                shape_data.SetIsStirrup(false);
                shape_data.SetLength(rebar_curve.GetLength() / uor_per_mm);

                rebar_element->Update(rebar_curve, diameter, end_types, shape_data, this->GetModelRef(), false);

                // 写入其它的参数
                char layer_str[32];
                sprintf(layer_str, "%d", layer_rebar_data.layer->layer_index);
                auto layer_string = std::string(layer_str);

                std::string rebar_type_string;
                switch(layer_rebar_data.layer->position)
                {
                case RebarPosition::Front:
                    rebar_type_string = "front";
                    break;
                case RebarPosition::Middle:
                    rebar_type_string = "midden";
                    break;
                case RebarPosition::Back:
                    rebar_type_string = "back";
                    break;
                default:
                    break;
                }

                // 需要转换到EEH
                EditElementHandle rebar_element_eeh(rebar_element->GetElementId(), this->GetModelRef());
                auto old_ref_ptr = rebar_element_eeh.GetElementRef();

                SetRebarLevelItemTypeValue(
                    rebar_element_eeh, 
                    std::move(layer_string), 
                    layer_rebar_data.layer->type, 
                    std::move(rebar_type_string),
                    this->GetModelRef());
                SetRebarHideData(rebar_element_eeh, layer_rebar_data.layer->spacing,this->GetModelRef());

				ElementPropertiesSetterPtr propEle = ElementPropertiesSetter::Create();
				/*if (layer_rebar_data.layer->rebarColor > -1)
				{
					propEle->SetColor(layer_rebar_data.layer->rebarColor);
				}*/
				propEle->SetLinestyle(layer_rebar_data.layer->rebarLineStyle, NULL);
				propEle->SetWeight(layer_rebar_data.layer->rebarWeight);
				propEle->Apply(rebar_element_eeh);

                rebar_element_eeh.ReplaceInModel(old_ref_ptr);

                ++index;
            }

            RebarSetData rebar_set_data;
            rebar_set_data.SetNumber((int)rebar_curves.size());
            // 转换到mm
            // 写入设置的间距
            auto spacing_mm = layer_rebar_data.layer->spacing;
            CString spacing_mm_text;
            spacing_mm_text.Format(L"%lf", spacing_mm);
            rebar_set_data.SetNominalSpacing(spacing_mm);
            rebar_set_data.SetSpacingString(spacing_mm_text);
            // 写入实际的间距
            rebar_set_data.SetAverageSpacing(new_spacing / uor_per_mm);

            // 结束生成
            rebar_set_p->FinishUpdate(rebar_set_data, this->GetModelRef());
            rebar_set_p->SetSetData(rebar_set_data);
            // this->AddRebarSet(*rebar_set_p, m_set_tag_count, end_types, false, this->GetModelRef());

            auto rebar_set_tag = new RebarSetTag();
            rebar_set_tag->SetBarSetTag(m_set_tag_count);
            rebar_set_tag->SetEndTypes(end_types);
            rebar_set_tag->SetIsStirrup(false);
            rebar_set_tag->SetRset(rebar_set_p);

            this->m_set_tags.Add(rebar_set_tag);
        }
        // this->AddRebarSet(*rebar_set_p, 1, end_types, false, this->GetModelRef());
        ++m_set_tag_count;

        return true;
    }

    std::vector<RebarCurve> GeneralFaceRebarAssembly::gen_rebar_curves(const LayerRebarData &layer_rebar_data, double diameter, double &new_spacing, RebarEndTypes &end_types)
    {
        // 转换face到polygon
        PolygonHelper::Polygon face_polygon;
        Transform face_polygon_transform;
        // 使用钢筋方向作为该平面的x轴
        PolygonHelper::from_3d_face(*layer_rebar_data.face, layer_rebar_data.rebar_dir, face_polygon_transform, face_polygon);

        // 修改该多边形，使之向内缩小一个保护层(side_cover)的距离
        PolygonHelper::offset_edges(face_polygon, -layer_rebar_data.side_cover * UOR_PER_MilliMeter); 

        // 计算扫描的范围
        auto min_max_iter = std::minmax_element(
            face_polygon.points.cbegin(), face_polygon.points.cend(),
            [](const DPoint2d &pa, const DPoint2d &pb) -> bool
            {
                return pa.y < pb.y;
            });
        auto min =
            min_max_iter.first == face_polygon.points.cend() ? 0 : min_max_iter.first->y;
        auto max =
            min_max_iter.second == face_polygon.points.cend() ? 100 : min_max_iter.second->y;

        // min_max是钢筋侧边的方向
        // 因为保护层是墙边到钢筋边界的距离，min max范围需要缩小一个半径
        min += diameter / 2.0;
        max -= diameter / 2.0;

        // 产生扫描位置
        std::vector<double> scan_positions;
        auto spacing = layer_rebar_data.layer->spacing * UOR_PER_MilliMeter;
        // 根据不同的方式选择扫描函数
        switch (layer_rebar_data.array_mode)
        {
        case ArrayMode::Normal:
            scan_positions = _local::scan_in_range_normal(spacing, min, max);
            new_spacing = spacing;
            break;
        case ArrayMode::NormalWithExtra:
            scan_positions = _local::scan_in_range_normal_with_extra(spacing, min, max);
            new_spacing = spacing;
            break;
        case ArrayMode::Fit:
            scan_positions = _local::scan_in_range_fit(spacing, min, max, &new_spacing);
            break;
        default:
            break;
        }

        // 错开钢筋
        std::for_each(
            scan_positions.begin(), std::prev(scan_positions.end()),
            [&](double &position)
            {
                position += layer_rebar_data.collision_offset;
            });
        // 最后一个点向内偏移
        scan_positions.back() -= layer_rebar_data.collision_offset;

        // 根据点集产生线段
        const auto shape_segments = PolygonHelper::to_segments(face_polygon);

        // 根据扫描位置计算交点并产生钢筋线段
        std::vector<DSegment3d> rebar_segments;
        /*for (const auto position : scan_positions)
        {
            const auto cross_points = _local::intersect_with_y(shape_segments, position);
            const auto cross_segments = _local::cross_points_to_cross_segments(cross_points);

            for (const auto &segment : cross_segments)
            {
                rebar_segments.push_back(segment);
            }
        }*/

		for (int i = 0;i < scan_positions.size(); i++)
		{
			auto position = scan_positions[i];
			if (layer_rebar_data.layer->direction == Gallery::RebarDirection::Horizontal)
			{
				// 横向钢筋最上面的钢筋往下移动一个直径的距离
				// 最下面的钢筋往上移动一个直径的距离
				auto diameter = layer_rebar_data.layer->get_rebar_diameter();
				if (i == 0)
					position += diameter;
				if (i == scan_positions.size() - 1)
					position -= diameter;
			}
			const auto cross_points = _local::intersect_with_y(shape_segments, position);
			const auto cross_segments = _local::cross_points_to_cross_segments(cross_points);

			for (const auto &segment : cross_segments)
			{
				rebar_segments.push_back(segment);
			}
		}

        // 变换线段到3d平面
        for (auto &rebar_segment : rebar_segments)
        {
            face_polygon_transform.Multiply(rebar_segment);
        }

        // 获得平面的法向
        // 通过对Z轴作用transform的rotation分量即可
        // 取transform的z列向量即可
        auto normal = layer_rebar_data.wall_normal;

        // 产生rebar_curve
        std::vector<RebarCurve> curves;
        for (const auto &rebar_segment : rebar_segments)
        {
            curves.push_back(RebarCurve());
            auto &rebar_curve = curves.back();

            // 根据端部调整偏移
            // 因为保护层指的是钢筋侧面到墙边的距离
            // 所有的弯锚还需要多偏移一个钢筋半径的宽度
            auto add_offset_by_end_type = [&](const End &end) -> double {
                switch(end.type)
                {
                case EndType::Hook90:
                case EndType::Hook135:
                case EndType::Hook180: 
                case EndType::CustomBend:
                    return end.offset * UOR_PER_MilliMeter - diameter / 2.0;
                default:
                    return end.offset * UOR_PER_MilliMeter;
                }
            };

            // 从线段生rebar curve
            _local::from_segments_to_rebar_curve(
                rebar_segment, 
                add_offset_by_end_type(layer_rebar_data.layer->starting_end_type),
                add_offset_by_end_type(layer_rebar_data.layer->ending_end_type),
                rebar_curve);

            // 为钢筋线添加端部
            _local::add_rebar_end(
                rebar_curve, 
                layer_rebar_data.layer->starting_end_type, 
                layer_rebar_data.layer->ending_end_type, 
                normal, 
                end_types);
        }

        return curves;
    }

    long GeneralFaceRebarAssembly::GetStreamMap(BeStreamMap &map, int type_of /* = 0 */, int version_of /* = -1 */)
    {
        switch (type_of)
        {
        case 0:
            return RebarExtendedElement::GetStreamMap(map, type_of, version_of);
        case 1:
            return RebarAssembly::GetStreamMap(map, type_of, version_of);
        case 2:
            return 0;
        default:
            return -1;
        }
    }

    void GeneralFaceRebarAssembly::remove_rebar(void)
    {
        RebarSets rebar_sets;
        this->GetRebarSets(rebar_sets, this->GetModelRef());
        for (auto i = 0; i < rebar_sets.GetSize(); ++i)
        {
            auto rebar_set_ptr = &rebar_sets.At(i);
            auto id = rebar_set_ptr->GetElementId();
            EditElementHandle eeh(id, this->GetModelRef());
            eeh.DeleteFromModel();
        }
    }

    bool GeneralFaceRebarAssembly::OnDoubleClick()
    {
        if(this->m_double_click_callback)
        {
            return this->m_double_click_callback(*this);
        }
		else
		{
			EditElementHandle ehSel;
			if (!PIT::GetAssemblySelectElement(ehSel, this))
			{
				return false;
			}
			//return this->m_double_click_callback(*this);
		}
        return false;
    }
}
