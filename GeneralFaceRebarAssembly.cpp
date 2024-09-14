#include "_ustation.h"
#include "GeneralFaceRebarAssembly.h"
#include "ExtractFacesTool.h"
#include "PITRebarCurve.h"
#include "PolygonHelper.h"
#include <cmath>
#include <Point3D.h>
#include <Vector3D.h>

/// ����һ���ϴ��EPS����ΪBentley�е�λΪUOR���õ�λ��Լ��һ��um�����double���Ϳ��ܾ��Ȳ���1e-5��һ��ȡ1e1~1e-3����
#define LARGE_EPS 1e-3

namespace Gallery
{

    namespace _local
    {
        /// @brief �������ķ�ʽɨ��
        /// @details �ο�ArrayMode::Normal
        std::vector<double> scan_in_range_normal(double spacing, double min, double max)
        {
            std::vector<double> values;

            // value����maxҲ����
            for (double value = min; value < max || COMPARE_VALUES_EPS(value, max, LARGE_EPS) == 0; value += spacing)
            {
                values.push_back(value);
            }

            return values;
        }

        /// @brief �������ķ�ʽɨ��
        /// @details �ο�ArrayMode::Normal
        std::vector<double> scan_in_range_normal_with_extra(double spacing, double min, double max)
        {
            // �Ȱ�������ʽɨ��
            auto values = scan_in_range_normal(spacing, min, max);

            // ������һ��������max
            // ˵��û����������
            if(COMPARE_VALUES_EPS(values.back(), max, LARGE_EPS) != 0)
            {
                // ����һ���µĸֽ�
                values.push_back(max);
            }

            return values;
        }


        /// @brief ������Ӧ�ķ�ʽɨ��
        /// @details �ο�ArrayMode::Fit
        std::vector<double> scan_in_range_fit(double spacing, double min, double max, double *out_spacing = nullptr)
        {
            auto length = max - min;
            // �������
            auto count = static_cast<int>(std::floor(length / spacing));
            // ��������
            auto remain = length - (count * spacing);
            // ����Ϊһ���Ƚ�С��ֵ��˵���պ��ܷ���
            if (COMPARE_VALUES_EPS(remain, 0, LARGE_EPS) == 0)
            {
                // ֱ�Ӱ�������ʽ���ü���
                if (out_spacing != nullptr)
                {
                    *out_spacing = spacing;
                }
                return scan_in_range_normal(spacing, min, max);
            }

            // �����������1
            count += 1;

            // �µļ��
            auto new_spacing = length / static_cast<double>(count);

            if (out_spacing != nullptr)
            {
                *out_spacing = new_spacing;
            }

            // �����µļ�ఴ�����ķ�ʽɨ�輴��
            return scan_in_range_normal(new_spacing, min, max);
        }

        /// @brief �ӵ㼯��β����������߶�
        std::vector<DSegment3d> from_points_to_loop_segments(const std::vector<DPoint2d> &points)
        {
            std::vector<DSegment3d> segments;

            for (auto index = 0; index < points.size(); ++index)
            {
                // �����ֹ���, ͬʱҲ����ʵ����β����
                auto next_index = (index + 1) % points.size();

                const auto &current = points.at(index);
                const auto &next = points.at(next_index);

                segments.push_back(DSegment3d::From(current, next));
            }

            return segments;
        }

        /// @brief ���������߶���y=a�Ľ���
        std::vector<DPoint2d> intersect_with_y(const std::vector<DSegment3d> &segments, double y)
        {
            auto line_y_start = DPoint3d::From(0, y, 0);
            auto line_y_end = DPoint3d::From(1, y, 0);

            std::vector<DPoint2d> cross_points;
            for (const auto &segment : segments)
            {
                const auto &segment_start = segment.point[0];
                const auto &segment_end = segment.point[1];

                // �Ƿ��غ��ж�, ��y���
                if (COMPARE_VALUES_EPS(segment_start.y, y, LARGE_EPS) == 0 && COMPARE_VALUES_EPS(segment_end.y, y, LARGE_EPS) == 0)
                {
                    // ��y�غϣ������ߵĶ˵���Ϊ����
                    cross_points.push_back(DPoint2d::From(segment_start));
                    cross_points.push_back(DPoint2d::From(segment_end));

                    continue;
                }

                // �ڵ�ǰ�߶��ϵ�λ��
                double segment_param;
                // �ڵ�ǰ�߶��ϵĽ���
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
                    // û�н���
                    // ����mdlVec_intersectXYLines���߶���Ϊֱ��
                    // û�н���ֻ����Ϊƽ�л��غ�
                    // �غϵ����ǰ�濼�ǹ���
                    continue;
                }

                // ��齻���Ƿ����߶���
                if (segment_param < 0 || segment_param > 1)
                {
                    continue;
                }

                cross_points.push_back(DPoint2d::From(cross_point));
            }

            // �Խ����x��С����
            std::sort(
                cross_points.begin(),
                cross_points.end(),
                [](const DPoint2d &a, const DPoint2d &b) -> bool
                {
                    return a.x < b.x;
                });

            // ȥ���غϵ�
            auto new_end = std::unique(
                cross_points.begin(),
                cross_points.end(),
                [](const DPoint2d &a, const DPoint2d &b) -> bool
                {
                    return a.AlmostEqual(b);
                });

            return std::vector<DPoint2d>(cross_points.begin(), new_end);
        }

        /// @brief ת�����㵽���߶�
        /// @details ��Ҫż�������㣬�������������߶�
        std::vector<DSegment3d> cross_points_to_cross_segments(const std::vector<DPoint2d> points)
        {
            std::vector<DSegment3d> cross_segments;

            if (points.empty() || points.size() % 2 != 0)
            {
                // ����������Ϊ�գ����߸�������2�ı������޷����ɽ���
                // ���ؿյ�vec
                return cross_segments;
            }

            // ������һ�������߶�
            for (auto i = 0, j = i + 1; j < points.size(); i += 2, j = i + 1)
            {
                const auto &start = points.at(i);
                const auto &end = points.at(j);

                auto segment = DSegment3d::From(start, end);

                // ���˵�����С��0���߶�
                if (COMPARE_VALUES_EPS(segment.Length(), 0, LARGE_EPS) == 0)
                {
                    continue;
                }

                cross_segments.push_back(std::move(segment));
            }

            return cross_segments;
        }

        /// @brief ת���߶ε�rebar_curve
        /// @param segment �߶�
        /// @param start_offset �ֽ����ʼ�˳���ƫ��(start_offset), ����0Ϊ�ӳ�����λ��UOR
        /// @param end_offset �ֽ�Ľ����˳���ƫ��(start_offset), ����0Ϊ�ӳ�����λ��UOR
        /// @param curve rebar_curve
        void from_segments_to_rebar_curve(const DSegment3d &segment, double start_offset, double end_offset , RebarCurve &curve)
        {
            // ���ƶ˵�, ֮����Ҫ�޸�
            DPoint3d start_point, end_point;
            segment.GetEndPoints(start_point, end_point);

            // ����ֽ�ķ���
            auto rebar_dir = segment.point[1] - segment.point[0];
            // ��λ���÷���
            rebar_dir.Normalize();
        
            // �յ��ƫ�Ʊ任
            auto end_transform = Transform::From(rebar_dir * end_offset);
            end_transform.Multiply(end_point);

            // ���ƫ�Ʊ任
            rebar_dir.Negate();
            auto start_transform = Transform::From(rebar_dir * start_offset);
            start_transform.Multiply(start_point);

            // ֻ����ʼ����յ�
            auto &start_vertex = curve.PopVertices().NewElement();
            start_vertex.SetType(RebarVertex::PointType::kStart);
            start_vertex.SetIP(start_point);

            auto &end_vertex = curve.PopVertices().NewElement();
            end_vertex.SetType(RebarVertex::PointType::kEnd);
            end_vertex.SetIP(end_point);
        }

        /// @brief ת����PIT rebar end type
        /// @param rebar_direction �ֽ��, ���ڼ�������ʱ�ķ���
        /// @param default_normal Ĭ�ϵķ��򣬿����Ǹֽ�����з���
        PIT::PITRebarEndType to_pit_rebar_end_type(const End &end, const DVec3d &rebar_direction, const DVec3d &default_normal)
        {
            PIT::PITRebarEndType pit_end_type;

            /// ������ת����ʱ����
            /// angle rad����ת�ĽǶ�
            auto rotate_by = [&](const DVec3d &axis, double angle_deg)
            {
                // ת����CVector3d
                CVector3D normal = CVector3D::From(default_normal.x, default_normal.y, default_normal.z);

                // ��Ĭ�Ϸ�����������angle_deg��
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


        /// @brief Ϊ�ֽ�����Ӷ˲���״
        /// @param curve �ֽ���
        /// @param start_end ��ʼ��
        /// @param end_end ������
        /// @param normal �ֽ�����ƽ��ķ���
        /// @param end_types �����֮ǰ�Ķ˲���ʽ
        void add_rebar_end(RebarCurve &curve, const End &start_end, const End &end_end, const DVec3d &normal, RebarEndTypes &end_types)
        {
            // ȡ���˵㣬����ֽ��
            CPoint3D start_point, end_point;
            curve.GetEndPoints(start_point, end_point);
            auto rebar_vec = end_point - start_point;

            // ת����PITRebarCurve
            // ��һ��ת�����ǳɹ�����ΪPITRebarCurve��RebarCurve�ڴ沼����ͬ
            auto pit_rebar_curve = static_cast<PIT::PITRebarCurve *>(&curve);

            // ת��Gallery::End��PITEndTypes
            PIT::PITRebarEndTypes pit_end_types;
            pit_end_types.beg = to_pit_rebar_end_type(start_end,DVec3d::From(rebar_vec), normal);
            pit_end_types.end = to_pit_rebar_end_type(end_end, DVec3d::From(-rebar_vec),normal);

            // ����ԭ��
            pit_end_types.beg.SetptOrgin(start_point);
            pit_end_types.end.SetptOrgin(end_point);

            // ������ⲿ
            end_types.beg = pit_end_types.beg;
            end_types.end = pit_end_types.end;


            // CustomBend��Ҫ��������
            if(start_end.type == EndType::CustomBend)
            {
                // �ֶ�������ʼ�˲�
                pit_rebar_curve->RebarEndBendBeg(pit_end_types, rebar_vec, PI - start_end.data.custom_bend.angle / 180 * PI);
            }

            // ������������ĸֽ�
            // CustomBend���������ﱻ����
            pit_rebar_curve->EvaluateEndTypes(pit_end_types);

            if (end_end.type == EndType::CustomBend)
            {
                // �ֶ�����������˲�
                pit_rebar_curve->RebarEndBendEnd(pit_end_types, -rebar_vec, PI - end_end.data.custom_bend.angle / 180 * PI);
            }

        }
    }

    GeneralFaceRebarAssembly *GeneralFaceRebarAssembly::from_concrete(ElementId concrete_id, DgnModelRefP model_ref)
    {
        RebarAssemblies assemblies;
        // ��û�����������assembly
        RebarAssembly::GetRebarAssemblies(concrete_id, assemblies, model_ref);

        // ���ҵ�ǰ���͵�assembly
        for (auto i = 0; i < assemblies.GetSize(); ++i)
        {
            auto assembly_ptr = assemblies.GetAt(i);

            // ����ת���ɵ�ǰ���͵�ָ��
            auto this_ptr = dynamic_cast<GeneralFaceRebarAssembly *>(assembly_ptr);

            // ���ת��ʧ����
            if (this_ptr == nullptr)
            {
                continue;
            }

            // �ɹ�ֱ�ӷ���
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

        // ��size_key��ȡֱ��
        const auto diameter = layer_rebar_data.layer->get_rebar_diameter(this->GetModelRef());

        ElementId rebar_set_id = 0;
        // ��ȡrebar_set_id��rebar_set
        auto rebar_set_p = RebarSet::Fetch(rebar_set_id, this->GetModelRef());
        if (rebar_set_p == nullptr)
        {
            return false;
        }

        // ����RebarSet������
        rebar_set_p->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
        rebar_set_p->SetCallerId(this->GetCallerId());
        {
            rebar_set_p->StartUpdate(this->GetModelRef());

            // ���øֽ�еķ��Ų���
            RebarSymbology sym;
            sym.SetRebarLevel(layer_rebar_data.level_name);
            SetRebarColorBySize(layer_rebar_data.layer->size_str, sym);

            RebarEndTypes end_types;

            // ���ɾ���ĸֽ���
            double new_spacing; // ʵ�ʵļ��
            const auto rebar_curves = GeneralFaceRebarAssembly::gen_rebar_curves(layer_rebar_data, diameter, new_spacing, end_types);
            // ���ݸֽ�������RebarElement
            auto index = 0;
            for (const auto &rebar_curve : rebar_curves)
            {
                // rebar element ����һ���ֽ�
                auto rebar_element = rebar_set_p->AssignRebarElement(index, (int)rebar_curves.size(), sym, this->GetModelRef());

                // ���øֽ�Ĳ���, ����ߴ�
                RebarShapeData shape_data;
                shape_data.SetSizeKey(CString(layer_rebar_data.layer->get_full_size_key().c_str()));
                shape_data.SetIsStirrup(false);
                shape_data.SetLength(rebar_curve.GetLength() / uor_per_mm);

                rebar_element->Update(rebar_curve, diameter, end_types, shape_data, this->GetModelRef(), false);

                // д�������Ĳ���
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

                // ��Ҫת����EEH
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
            // ת����mm
            // д�����õļ��
            auto spacing_mm = layer_rebar_data.layer->spacing;
            CString spacing_mm_text;
            spacing_mm_text.Format(L"%lf", spacing_mm);
            rebar_set_data.SetNominalSpacing(spacing_mm);
            rebar_set_data.SetSpacingString(spacing_mm_text);
            // д��ʵ�ʵļ��
            rebar_set_data.SetAverageSpacing(new_spacing / uor_per_mm);

            // ��������
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
        // ת��face��polygon
        PolygonHelper::Polygon face_polygon;
        Transform face_polygon_transform;
        // ʹ�øֽ����Ϊ��ƽ���x��
        PolygonHelper::from_3d_face(*layer_rebar_data.face, layer_rebar_data.rebar_dir, face_polygon_transform, face_polygon);

        // �޸ĸö���Σ�ʹ֮������Сһ��������(side_cover)�ľ���
        PolygonHelper::offset_edges(face_polygon, -layer_rebar_data.side_cover * UOR_PER_MilliMeter); 

        // ����ɨ��ķ�Χ
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

        // min_max�Ǹֽ��ߵķ���
        // ��Ϊ��������ǽ�ߵ��ֽ�߽�ľ��룬min max��Χ��Ҫ��Сһ���뾶
        min += diameter / 2.0;
        max -= diameter / 2.0;

        // ����ɨ��λ��
        std::vector<double> scan_positions;
        auto spacing = layer_rebar_data.layer->spacing * UOR_PER_MilliMeter;
        // ���ݲ�ͬ�ķ�ʽѡ��ɨ�躯��
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

        // ���ֽ�
        std::for_each(
            scan_positions.begin(), std::prev(scan_positions.end()),
            [&](double &position)
            {
                position += layer_rebar_data.collision_offset;
            });
        // ���һ��������ƫ��
        scan_positions.back() -= layer_rebar_data.collision_offset;

        // ���ݵ㼯�����߶�
        const auto shape_segments = PolygonHelper::to_segments(face_polygon);

        // ����ɨ��λ�ü��㽻�㲢�����ֽ��߶�
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
				// ����ֽ�������ĸֽ������ƶ�һ��ֱ���ľ���
				// ������ĸֽ������ƶ�һ��ֱ���ľ���
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

        // �任�߶ε�3dƽ��
        for (auto &rebar_segment : rebar_segments)
        {
            face_polygon_transform.Multiply(rebar_segment);
        }

        // ���ƽ��ķ���
        // ͨ����Z������transform��rotation��������
        // ȡtransform��z����������
        auto normal = layer_rebar_data.wall_normal;

        // ����rebar_curve
        std::vector<RebarCurve> curves;
        for (const auto &rebar_segment : rebar_segments)
        {
            curves.push_back(RebarCurve());
            auto &rebar_curve = curves.back();

            // ���ݶ˲�����ƫ��
            // ��Ϊ������ָ���Ǹֽ���浽ǽ�ߵľ���
            // ���е���ê����Ҫ��ƫ��һ���ֽ�뾶�Ŀ��
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

            // ���߶���rebar curve
            _local::from_segments_to_rebar_curve(
                rebar_segment, 
                add_offset_by_end_type(layer_rebar_data.layer->starting_end_type),
                add_offset_by_end_type(layer_rebar_data.layer->ending_end_type),
                rebar_curve);

            // Ϊ�ֽ�����Ӷ˲�
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
