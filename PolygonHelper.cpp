#include "_ustation.h"
#include "PolygonHelper.h"
#include <Mstn/MdlApi/MdlApi.h>
#include "CElementTool.h"

using namespace Bentley;
using namespace Bentley::DgnPlatform;

namespace Gallery{
	bool PolygonHelper::from_3d_face(const ElementHandle &face, const DVec3d &face_x, Transform &out_transform, PolygonHelper::Polygon &out_polygon)
    {
        // 获得面的法向
        DPoint3d normal;
        auto z_axis = DPoint3d::From(0,0,1);
        auto x_axis = DPoint3d::From(1, 0, 0);
        mdlElmdscr_extractNormal(&normal, nullptr, face.GetElementDescrCP(), &z_axis);
        if(normal.IsParallelTo(z_axis))
        {
            // 确认其确实是平行于z轴
            mdlElmdscr_extractNormal(&normal, nullptr, face.GetElementDescrCP(), &x_axis);
            if(normal.IsParallelTo(x_axis))
            {
                // 两次取出的法向量不一样
                return false;
            }
        }

        // 获得面的边界
        std::vector<MSElementDescrP> edges;
        PITCommonTool::CElementTool::GetALLEdges(face, edges);
        
        // 获得边界的所有点
        // 这里是储存了  边界=>(起点，终点)  的映射
        std::map<MSElementDescrP, std::pair<DPoint3d, DPoint3d>> points;
        for(auto edge: edges)
        {
            // 获得边界的起止点
            DPoint3d start, end;
            mdlElmdscr_extractEndPoints(&start, nullptr, &end, nullptr, edge, face.GetModelRef());

            points[edge] = std::make_pair(start, end);
        }

        // 对边界点按首尾相连的顺序排序并去重
        std::vector<DPoint3d> sorted_points;
        // 从第一条边开始
        auto edge = points.cbegin();
        // 将起始点加到sorted_points中
        sorted_points.push_back(edge->second.first);

        for (;;)
        {
            // 删除掉当前边
            points.erase(edge);

            // 查找与剩下的含有刚加入的点的边,
            // 因为当前边已经被删除了
            // 剩下的边最多一条
            const auto &current_point = sorted_points.back();
            // 成功找到这样的边的标志
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
                    // 起始点与当前点相同
                    // 将终点加入
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
                    // 终点与当前点相同
                    // 将起始点加入
                    sorted_points.push_back(start);
                    edge = remain_edge;
                    successful = true;
                    break;
                }
            }
            
            if(!successful)
            {
                // 没有找到这样的边, 可以进一步处理
                // 1.图形可能不封闭
                // 2.已经找完了，边集为空

                break;
            }
        }

        // 计算点集的变换， 
        RotMatrix z_rot;
        mdlRMatrix_fromVectorToVector(&z_rot, &normal, &z_axis);

        // 将钢筋方向变换到XOY上
        RotMatrix x_rot;
        auto plane_x_axis = DPoint3d(face_x);
		z_rot.Multiply(plane_x_axis);
        mdlRMatrix_fromVectorToVector(&x_rot, &plane_x_axis, &x_axis);

        // 组合两次旋转
        RotMatrix rot_matrix = x_rot * z_rot;
        // auto rot_matrix = z_rot * x_rot;

        // 任取一点作为原点
        // 计算偏移
        auto translation = sorted_points.front();
        translation.Negate();
        // 需要先移动到原点再旋转
        auto translation_transform = Transform::From(translation);

        // 将点变换到XOY
        std::vector<DPoint2d> sorted_2d_points;
        for (const auto &point: sorted_points)
        {
            DPoint3d new_point;
            // 先移动
            translation_transform.Multiply(new_point, point);
            // 再旋转
            rot_matrix.Multiply(new_point);
            // 写入回去
            sorted_2d_points.push_back(DPoint2d::From(new_point));
        }

        // 输出多边形
        out_polygon.points = std::move(sorted_2d_points);

        // 输出变换
        rot_matrix.Invert();
        translation.Negate();
        out_transform = Transform::From(rot_matrix, translation);

        return true;
    }

    bool PolygonHelper::to_3d_face(const Polygon &polygon, const Transform &transform, EditElementHandle &face)
    {
        // 转换2d点到3d
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

        // 创建face
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

        // 找到x最小的点
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

        // 前一个点
        auto prev_point_iter =
            left_point_iter == points.cbegin() ?
            // 没有前一个点了，就取最后一个点
            std::prev(points.cend()) : 
            std::prev(left_point_iter);

        // 后一个点
        auto next_point_iter = std::next(left_point_iter);
        next_point_iter =
            next_point_iter == points.cend() ? points.cbegin() : next_point_iter;

        auto left_point = DPoint3d::From(*left_point_iter);
        auto prev_point = DPoint3d::From(*prev_point_iter);
        auto next_point = DPoint3d::From(*next_point_iter);

        // 计算两条边的向量
        auto v1 = left_point - prev_point;
        auto v2 = next_point - left_point;

        // 计算这两个点的叉积
        auto ori = DVec3d::FromCrossProduct(v1, v2);

        // 判断它与0的大小关系
        auto result = COMPARE_VALUES_EPS(ori.z, 0, 1e-2);

        if(result == 0)
        {
            // 叉积大小为0，说明共线
            // 进一步根据y的大小判断
            return prev_point.y < next_point.y;
        }

        /// 叉积小于0则为顺时针
        return result < 0;
    }

    void PolygonHelper::offset_edges(Polygon &polygon, double distance)
    {
        // 先判断点是顺时针还是逆时针
        auto is_ccw = is_clock_wise(polygon);

        // 转换到线段
        auto segments = to_segments(polygon);

        const auto z_axis = DVec3d::UnitZ();

        // 平移每根线段
        for(auto &segment: segments)
        {
            DPoint3d start, end;
            segment.GetEndPoints(start, end);

            // 获得线段的方向
            auto direction = end - start;

            // 法向即为与z轴叉乘的结果
            // 应当是左手方向
            auto normal = DVec3d::FromCrossProduct(direction, z_axis);

            // 计算平移方向
            if(is_ccw)
            {
                // 顺时针取右手方向
                // 取反
                normal.Negate();
            }

            // 归一化
            normal.Normalize();

            // 设置长度
            normal.Scale(distance);

            // 转换到Transform
            auto transform = Transform::From(normal);

            // 平移线段到新的位置
            transform.Multiply(segment);
        }

        // 计算新的线段的交点
        polygon.points.clear();

        for (auto segment_iter = segments.cbegin(); segment_iter != segments.cend(); ++segment_iter)
        {
            // 下一条边
            auto next_iter = std::next(segment_iter);
            next_iter = next_iter == segments.cend() ? segments.cbegin() : next_iter;

            // 获得线段的端点
            DPoint3d current_start, current_end;
            DPoint3d next_start, next_end;
            segment_iter->GetEndPoints(current_start, current_end);
            next_iter->GetEndPoints(next_start, next_end);

            // 暂时不考虑没有交点的情况
            DPoint3d cross_point;
            mdlVec_intersectXYLines(
                &cross_point, nullptr,
                nullptr, nullptr,
                &current_start, &current_end,
                &next_start, &next_end
            );

            // 将交点加入多边形的顶点集合中
            polygon.points.push_back(DPoint2d::From(cross_point));
        }
    }
}