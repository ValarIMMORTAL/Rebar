#include "_ustation.h"
#include "PolygonHelper.h"
#include <Mstn/MdlApi/MdlApi.h>
#include "CElementTool.h"

using namespace Bentley;
using namespace Bentley::DgnPlatform;

namespace Gallery{
	bool PolygonHelper::from_3d_face(const ElementHandle &face, const DVec3d &face_x, Transform &out_transform, PolygonHelper::Polygon &out_polygon)
    {
        // �����ķ���
        DPoint3d normal;
        auto z_axis = DPoint3d::From(0,0,1);
        auto x_axis = DPoint3d::From(1, 0, 0);
        mdlElmdscr_extractNormal(&normal, nullptr, face.GetElementDescrCP(), &z_axis);
        if(normal.IsParallelTo(z_axis))
        {
            // ȷ����ȷʵ��ƽ����z��
            mdlElmdscr_extractNormal(&normal, nullptr, face.GetElementDescrCP(), &x_axis);
            if(normal.IsParallelTo(x_axis))
            {
                // ����ȡ���ķ�������һ��
                return false;
            }
        }

        // �����ı߽�
        std::vector<MSElementDescrP> edges;
        PITCommonTool::CElementTool::GetALLEdges(face, edges);
        
        // ��ñ߽�����е�
        // �����Ǵ�����  �߽�=>(��㣬�յ�)  ��ӳ��
        std::map<MSElementDescrP, std::pair<DPoint3d, DPoint3d>> points;
        for(auto edge: edges)
        {
            // ��ñ߽����ֹ��
            DPoint3d start, end;
            mdlElmdscr_extractEndPoints(&start, nullptr, &end, nullptr, edge, face.GetModelRef());

            points[edge] = std::make_pair(start, end);
        }

        // �Ա߽�㰴��β������˳������ȥ��
        std::vector<DPoint3d> sorted_points;
        // �ӵ�һ���߿�ʼ
        auto edge = points.cbegin();
        // ����ʼ��ӵ�sorted_points��
        sorted_points.push_back(edge->second.first);

        for (;;)
        {
            // ɾ������ǰ��
            points.erase(edge);

            // ������ʣ�µĺ��иռ���ĵ�ı�,
            // ��Ϊ��ǰ���Ѿ���ɾ����
            // ʣ�µı����һ��
            const auto &current_point = sorted_points.back();
            // �ɹ��ҵ������ıߵı�־
            bool successful = false;
            for (auto remain_edge = points.cbegin(); remain_edge != points.cend(); ++remain_edge)
            {
                const auto &end_points = remain_edge->second;
                const auto start = end_points.first;
                const auto end = end_points.second;

                // if(start.IsEqual(current_point))
                if(
                    COMPARE_VALUES_EPS(start.x, current_point.x, 1e-5) == 0 && 
                    COMPARE_VALUES_EPS(start.y, current_point.y, 1e-5) == 0 &&
                    COMPARE_VALUES_EPS(start.z, current_point.z, 1e-5) == 0)
                {
                    // ��ʼ���뵱ǰ����ͬ
                    // ���յ����
                    sorted_points.push_back(end);
                    edge = remain_edge;
                    successful = true;
                    break;
                }

                // if(end.IsEqual(current_point))
                if(
                    COMPARE_VALUES_EPS(end.x, current_point.x, 1e-5) == 0 && 
                    COMPARE_VALUES_EPS(end.y, current_point.y, 1e-5) == 0 &&
                    COMPARE_VALUES_EPS(end.z, current_point.z, 1e-5) == 0)
                {
                    // �յ��뵱ǰ����ͬ
                    // ����ʼ�����
                    sorted_points.push_back(start);
                    edge = remain_edge;
                    successful = true;
                    break;
                }
            }
            
            if(!successful)
            {
                // û���ҵ������ı�, ���Խ�һ������
                // 1.ͼ�ο��ܲ����
                // 2.�Ѿ������ˣ��߼�Ϊ��

                break;
            }
        }

        // ����㼯�ı任�� 
        RotMatrix z_rot;
        mdlRMatrix_fromVectorToVector(&z_rot, &normal, &z_axis);

        // ���ֽ��任��XOY��
        RotMatrix x_rot;
        auto plane_x_axis = DPoint3d(face_x);
		z_rot.Multiply(plane_x_axis);
        mdlRMatrix_fromVectorToVector(&x_rot, &plane_x_axis, &x_axis);

        // ���������ת
        RotMatrix rot_matrix = x_rot * z_rot;
        // auto rot_matrix = z_rot * x_rot;

        // ��ȡһ����Ϊԭ��
        // ����ƫ��
        auto translation = sorted_points.front();
        translation.Negate();
        // ��Ҫ���ƶ���ԭ������ת
        auto translation_transform = Transform::From(translation);

        // ����任��XOY
        std::vector<DPoint2d> sorted_2d_points;
        for (const auto &point: sorted_points)
        {
            DPoint3d new_point;
            // ���ƶ�
            translation_transform.Multiply(new_point, point);
            // ����ת
            rot_matrix.Multiply(new_point);
            // д���ȥ
            sorted_2d_points.push_back(DPoint2d::From(new_point));
        }

        // ��������
        out_polygon.points = std::move(sorted_2d_points);

        // ����任
        rot_matrix.Invert();
        translation.Negate();
        out_transform = Transform::From(rot_matrix, translation);

        return true;
    }

    bool PolygonHelper::to_3d_face(const Polygon &polygon, const Transform &transform, EditElementHandle &face)
    {
        // ת��2d�㵽3d
        std::vector<DPoint3d> points;
        std::transform(
            polygon.points.cbegin(), polygon.points.cend(),
            std::back_inserter(points),
            [&](const DPoint2d &point_2d) -> DPoint3d
            {
                auto point_3d = DPoint3d::From(point_2d);

                transform.Multiply(point_3d);

                return point_3d;
            });

        // ����face
        if(ShapeHandler::CreateShapeElement(
            face, 
            nullptr, 
            points.data(), 
            points.size(), 
            ACTIVEMODEL->Is3d(), 
            *ACTIVEMODEL) != SUCCESS)
        {
            return false;
        }

        return true;
    }

    std::vector<DSegment3d> PolygonHelper::to_segments(const Polygon &polygon)
    {
        std::vector<DSegment3d> segments;
        for (auto index = 0; index < polygon.points.size(); ++index)
        {
            auto next_index = (index + 1) % polygon.points.size();
            segments.push_back(DSegment3d::From(polygon.points[index], polygon.points[next_index]));
        }

        return segments;
    }


    bool PolygonHelper::is_clock_wise(const Polygon &polygon)
    {
        const auto &points = polygon.points;

        // �ҵ�x��С�ĵ�
        auto left_point_iter =
            std::min_element(
                points.cbegin(), points.cend(),
                [](const DPoint2d &p1, const DPoint2d &p2)
                {
                    return p1.x < p2.x;
                });
        
        if(left_point_iter == points.cend()) 
        {
            return false;
        }

        // ǰһ����
        auto prev_point_iter =
            left_point_iter == points.cbegin() ?
            // û��ǰһ�����ˣ���ȡ���һ����
            std::prev(points.cend()) : 
            std::prev(left_point_iter);

        // ��һ����
        auto next_point_iter = std::next(left_point_iter);
        next_point_iter =
            next_point_iter == points.cend() ? points.cbegin() : next_point_iter;

        auto left_point = DPoint3d::From(*left_point_iter);
        auto prev_point = DPoint3d::From(*prev_point_iter);
        auto next_point = DPoint3d::From(*next_point_iter);

        // ���������ߵ�����
        auto v1 = left_point - prev_point;
        auto v2 = next_point - left_point;

        // ������������Ĳ��
        auto ori = DVec3d::FromCrossProduct(v1, v2);

        // �ж�����0�Ĵ�С��ϵ
        auto result = COMPARE_VALUES_EPS(ori.z, 0, 1e-2);

        if(result == 0)
        {
            // �����СΪ0��˵������
            // ��һ������y�Ĵ�С�ж�
            return prev_point.y < next_point.y;
        }

        /// ���С��0��Ϊ˳ʱ��
        return result < 0;
    }

    void PolygonHelper::offset_edges(Polygon &polygon, double distance)
    {
        // ���жϵ���˳ʱ�뻹����ʱ��
        auto is_ccw = is_clock_wise(polygon);

        // ת�����߶�
        auto segments = to_segments(polygon);

        const auto z_axis = DVec3d::UnitZ();

        // ƽ��ÿ���߶�
        for(auto &segment: segments)
        {
            DPoint3d start, end;
            segment.GetEndPoints(start, end);

            // ����߶εķ���
            auto direction = end - start;

            // ����Ϊ��z���˵Ľ��
            // Ӧ�������ַ���
            auto normal = DVec3d::FromCrossProduct(direction, z_axis);

            // ����ƽ�Ʒ���
            if(is_ccw)
            {
                // ˳ʱ��ȡ���ַ���
                // ȡ��
                normal.Negate();
            }

            // ��һ��
            normal.Normalize();

            // ���ó���
            normal.Scale(distance);

            // ת����Transform
            auto transform = Transform::From(normal);

            // ƽ���߶ε��µ�λ��
            transform.Multiply(segment);
        }

        // �����µ��߶εĽ���
        polygon.points.clear();

        for (auto segment_iter = segments.cbegin(); segment_iter != segments.cend(); ++segment_iter)
        {
            // ��һ����
            auto next_iter = std::next(segment_iter);
            next_iter = next_iter == segments.cend() ? segments.cbegin() : next_iter;

            // ����߶εĶ˵�
            DPoint3d current_start, current_end;
            DPoint3d next_start, next_end;
            segment_iter->GetEndPoints(current_start, current_end);
            next_iter->GetEndPoints(next_start, next_end);

            // ��ʱ������û�н�������
            DPoint3d cross_point;
            mdlVec_intersectXYLines(
                &cross_point, nullptr,
                nullptr, nullptr,
                &current_start, &current_end,
                &next_start, &next_end
            );

            // ������������εĶ��㼯����
            polygon.points.push_back(DPoint2d::From(cross_point));
        }
    }
}