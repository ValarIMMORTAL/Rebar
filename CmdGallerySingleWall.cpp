#include "_ustation.h"
#include "CmdGallerySingleWall.h"
#include "PickElementTool.h"
#include "GallerySettings.h"
#include "Mstn/MdlApi/MdlApi.h"
#include "CModelTool.h"
#include "CFaceTool.h"
#include "CElementTool.h"
#include "WallRebarAssembly.h"
#include "PITACCRebarAssembly.h"
#include "ScanIntersectTool.h"
#include "WallHelper.h"
#include "SlabHelper.h"
#include "PolygonHelper.h"
#include "GeneralFaceRebarAssembly.h"
#include "CSolidTool.h"
#include "GallerySettingsDialog.h"
#include "XmlHelper.h"
#include <vector>
#include <map>
#include <set>
#include <memory>

using namespace PIT;

BEGIN_NAMESPACE_GALLERY_SINGLE_WALL

namespace _local
{
    /// @brief 原地缩放元素
    /// @details 原点取的是eeh的中心点
    void scale_element_in_place(EditElementHandle &eeh, const DVec3d &scale_vec);

    /// @brief 获得混凝土id
    ElementId get_concrete_id(ElementHandleCR eh);

    /// @brief 写入新的混凝土id
    void set_concrete_id(ElementHandleR eh, ElementId concrete_id);

    /// @brief 在给定位置上生成钢筋（正面、背面、中间）
    /// @param wall 墙本身
    /// @param assembly
    /// @param settings 配筋参数，用于读取保护层等参数
    /// @param wall_geometry_info 墙的几何信息
    /// @param top_slab 顶板
    /// @param bottom_slab 底板
    /// @param layers 钢筋层，（这里只有正面、背面、中间同一位置的钢筋）
    /// @param spacing 同一层钢筋之间的间距单位UOR
    /// @param array_mode 排列方式
    void make_rebar_in_position(
        EditElementHandleR wall,
        GeneralFaceRebarAssembly &assembly,
        const GallerySettings &settings,
        const WallHelper::WallGeometryInfo &wall_geometry_info,
        const std::vector<WallHelper::Associated> &associated,
        const std::vector<GallerySettings::Layer> &layers,
        GeneralFaceRebarAssembly::ArrayMode array_mode);
}

void cmd(WCharCP)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    auto dialog = new SettingsDialog(CWnd::FromHandle(MSWIND));
    dialog->on_ok = [=](const GallerySettings &settings, const ElementAgenda &agenda)
    {
        for(auto &entry : agenda)
        {
            // 过滤掉不是墙的
            auto &elem = const_cast<EditElementHandleR>(static_cast<EditElementHandleCR>(entry));
            if(!WallHelper::is_wall(elem))
                continue;

            execute_make_rebar(elem);
            // 暂时只给第一个配筋
            // 因为选中的可能有多个模型（墙是在组中的，直接选一面墙会将其它墙加入选择集中）
            //break;
        }

        // 配筋结束后关闭对话框
        // 如果选中点选模型按钮再去选中墙配筋， m_ClickedFlag被设置为1，界面将不会关闭
        /*if (dialog->m_ClickedFlag == 0)
        {
            dialog->SendMessage(WM_CLOSE);
			if (nullptr != dialog)
				delete dialog;
        }*/
        dialog->m_ClickedFlag = 0;
        
    };
    dialog->Create(IDD_DIALOG_Gallery_Settings);
    dialog->ShowWindow(SW_SHOW);
    // 隐藏“选择模型”按钮
    //dialog->GetDlgItem(IDC_SELECT_MODEL)->ShowWindow(SW_HIDE);
}

void _local::scale_element_in_place(EditElementHandle &eeh, const DVec3d &scale_vec)
{
    // 构造缩放transform
    // 需要先移动到原点，再缩放
    auto midpoint = PITCommonTool::CElementTool::getCenterOfElmdescr(eeh.GetElementDescrP());
    midpoint.Negate();
    auto move_to_origin_transform = Transform::From(midpoint);
    // 移动到原点
    eeh.GetHandler().ApplyTransform(eeh, TransformInfo(move_to_origin_transform));
    // 放大
    auto scale_matrix = RotMatrix::FromScaleFactors(scale_vec.x, scale_vec.y, scale_vec.z);
    auto scale_transform = Transform::From(scale_matrix);
    eeh.GetHandler().ApplyTransform(eeh, TransformInfo(scale_transform));
    // 移动回来
    midpoint.Negate();
    move_to_origin_transform.SetTranslation(midpoint);
    eeh.GetHandler().ApplyTransform(eeh, TransformInfo(move_to_origin_transform));
}

ElementId _local::get_concrete_id(ElementHandleCR eh)
{
    ElementId concrete_id = 0;
    GetElementXAttribute(eh.GetElementId(), sizeof(ElementId), concrete_id, ConcreteIDXAttribute, eh.GetModelRef());
    return concrete_id;
}

void _local::set_concrete_id(ElementHandleR eh, ElementId concrete_id)
{
    SetElementXAttribute(eh.GetElementId(), sizeof(ElementId), &concrete_id, ConcreteIDXAttribute, eh.GetModelRef());
}

void execute_make_rebar(EditElementHandleR wall)
{
    CString str;
    /// 读取墙上存的相关信息
    GallerySettings gallery_settings;
    if (!GallerySettings::from_element(wall, gallery_settings))
    {
        mdlOutput_error(L"读取墙配置数据失败");
        return;
    }

    // 分析墙的几何参数
    WallHelper::WallGeometryInfo wall_geometry_info;
    if (!WallHelper::analysis_geometry(wall, wall_geometry_info))
    {
        mdlOutput_error(L"获得墙几何信息失败");
        return;
    }

    /// 扫描与当前墙相连的元素
    std::vector<WallHelper::Associated> associated;
    if (!WallHelper::analysis_associated(wall, associated, wall_geometry_info))
    {
        mdlOutput_error(L"扫描相连构件失败");
        return;
    }

    auto top_slab_iter =
        std::find_if(
            associated.cbegin(), associated.cend(),
            [](const WallHelper::Associated &assoc)
            {
                return assoc.type == WallHelper::AssociatedType::TopFloor;
            });
    auto bottom_slab_iter =
        std::find_if(
            associated.cbegin(), associated.cend(),
            [](const WallHelper::Associated &assoc)
            {
                return assoc.type == WallHelper::AssociatedType::BottomFloor;
            });

    /*if (top_slab_iter == associated.cend() && bottom_slab_iter == associated.cend())
    {
        mdlOutput_error(L"未找到顶板和底板");
        return;
    }*/

    for (auto i = 0; i < gallery_settings.layer_count; ++i)
    {
        auto &layer = gallery_settings.layers[i];

        // 横向钢筋不需要锚入
        if (layer.direction != RebarDirection::Vertical)
        {
            continue;
        }

        // 获得项目中钢筋尺寸对应的相关参数
        auto bend_radius = layer.get_pin_radius();
        auto bend_length = layer.get_bend_length(layer.starting_end_type);
        auto diameter = layer.get_rebar_diameter();
        auto lae = layer.get_lae();
        // 没找到该尺寸钢筋对应的lae参数
        // 使用默认值 
        lae = lae < 0 ? bend_length : lae;
		string tmp(layer.size_str);
		double La_d = stod(tmp);
		auto la_Inside = 15 * La_d * UOR_PER_MilliMeter;

        // 起点为底端，终点为顶端
		if (layer.position == Gallery::RebarPosition::Front)
		{
			layer.starting_end_type.type = EndType::Hook90;
			layer.starting_end_type.data.hook_degree.radius = bend_radius;
			layer.starting_end_type.data.hook_degree.reserver_length = la_Inside;
			layer.ending_end_type.type = EndType::Hook90;
			layer.ending_end_type.data.hook_degree.radius = bend_radius;
			layer.ending_end_type.data.hook_degree.reserver_length = la_Inside;
			layer.starting_end_type.angle = 180;
			layer.ending_end_type.angle = 180;
		}
		else
		{
			layer.starting_end_type.type = EndType::Hook90;
			layer.starting_end_type.data.hook_degree.radius = bend_radius;
			layer.starting_end_type.data.hook_degree.reserver_length = la_Inside;
			layer.ending_end_type.type = EndType::Hook90;
			layer.ending_end_type.data.hook_degree.radius = bend_radius;
			layer.ending_end_type.data.hook_degree.reserver_length = la_Inside;
			layer.starting_end_type.angle = 0;
			layer.ending_end_type.angle = 0;
		}

    }

    if (top_slab_iter != associated.cend())//有顶板
    {
        EditElementHandle top_slab(top_slab_iter->element, true);
        DVec3d top_slab_dir;
        if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, top_slab, top_slab_dir))
        {
            mdlOutput_error(L"分析墙的顶板方向失败");
            return;
        }
        auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
        WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, top_slab, top_wall_inside_outside_faces);
        int flag = 0;//标志，T型墙时第二次的竖筋的锚入方向与第一次的方向需要相反
		int front_flag = 1;// 标志，如果墙的前面一层钢筋与孔洞相交需要更换前面一层锚入方向
        for (auto i = 0; i < gallery_settings.layer_count; ++i)
        {
            auto &layer = gallery_settings.layers[i];

            // 横向钢筋不需要锚入
            if (layer.direction != RebarDirection::Vertical)
            {
                continue;
            }
            // 获得项目中钢筋尺寸对应的相关参数
            auto bend_radius = layer.get_pin_radius();
            auto bend_length = layer.get_bend_length(layer.starting_end_type);
            auto diameter = layer.get_rebar_diameter();
            auto lae = layer.get_lae();//锚固
            auto la0 = layer.get_la0();//搭接
			auto d = layer.size_str;
            // 没找到该尺寸钢筋对应的lae参数
            // 使用默认值 
            /*lae = lae < 0 ? bend_length : lae;
            la0 = la0 < 0 ? bend_length : la0;*/
			string tmp(layer.size_str);
			double La_d = stod(tmp);
			auto la_Inside = 15 * La_d * UOR_PER_MilliMeter;
            if (lae > 0)
                //lae = lae * diameter / UOR_PER_MilliMeter;
				lae = lae * La_d;
            else
                lae = bend_length;
            if (la0 > 0)
                //la0 = la0 * diameter / UOR_PER_MilliMeter;
				la0 = la0 * La_d;
            else
                la0 = bend_length;

            // 起点为底端，终点为顶端
            layer.ending_end_type.type = EndType::Hook90;
            layer.ending_end_type.data.hook_degree.radius = bend_radius;

            EditElementHandle *face;
            switch (layer.position)
            {
            case RebarPosition::Front:
                face = wall_geometry_info.front_face;
                break;
            case RebarPosition::Back:
                face = wall_geometry_info.back_face;
                break;
            case RebarPosition::Middle:
                break;
            default:
                break;
            }

			//////////分析孔洞////////////////
			EditElementHandle Eleeh;
			std::vector<EditElementHandle*> Holeehs;
			EFT::GetSolidElementAndSolidHoles(top_slab, Eleeh, Holeehs);
			std::vector<EditElementHandle*> useHoleehs;
			CalculateMyUseHoles(useHoleehs, Holeehs, gallery_settings, top_slab.GetModelRef());
			bool is_touch = false;//墙是否会接触到孔洞
			bool is_front = false;//是否是前层接触到孔洞
			for (auto hole_eeh : useHoleehs)
			{
				DPoint3d minP_hole;
				DPoint3d maxP_hole;
				//计算板中孔洞元素描述符中元素的范围。
				mdlElmdscr_computeRange(&minP_hole, &maxP_hole, hole_eeh->GetElementDescrP(), NULL);

				DPoint3d minP_wall;
				DPoint3d maxP_wall;
				//计算墙元素描述符中元素的范围。
				mdlElmdscr_computeRange(&minP_wall, &maxP_wall, wall.GetElementDescrP(), NULL);

				DVec3d vec_holeX = DVec3d::From(maxP_hole.x - minP_hole.x, 0, 0);
				vec_holeX.Normalize();
				//if (fabs(fabs(tmpVec.DotProduct(dimVec)) - 1) < 1e-6)
				if (fabs(fabs(wall_geometry_info.normal.DotProduct(vec_holeX)) - 1) < 1e-6)
				{
					if ((abs(maxP_hole.x - minP_wall.x) < la_Inside) || (abs(minP_hole.x - maxP_wall.x) < la_Inside))
					{
						is_touch = true;
						if ((abs(maxP_hole.x - minP_wall.x) < la_Inside))
							is_front = false;
						else if ((abs(minP_hole.x - maxP_wall.x) < la_Inside))
							is_front = true;

					}
				}
				else
				{
					if ((abs(maxP_hole.y - minP_wall.y) < la_Inside) || (abs(minP_hole.y - maxP_wall.y) < la_Inside))
					{
						is_touch = true;
						if ((abs(maxP_hole.y - minP_wall.y) < la_Inside))
							is_front = true;
						else if ((abs(minP_hole.y - maxP_wall.y) < la_Inside))
							is_front = false;

					}
				}

			}
			//////////分析孔洞////////////////

            // 根据面是否为内侧设置钢筋锚入长度
            auto set_length =
                [&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
            {
                // 内侧面锚入长度使用15d, d为钢筋直径
                // 改：内侧面为锚固长度即lae
                if (WallHelper::is_inside_face(inside_outside_faces, face))
                {
                    //end.data.hook_degree.reserver_length = 15 * diameter;
                    end.data.hook_degree.reserver_length = la_Inside;
                    // 内侧面钢筋两端还需要缩短两个钢筋直径长度，为板、外侧钢筋预留空间
                    end.offset = -diameter / UOR_PER_MilliMeter * 2;
                }
                // 外侧面设置为Lae
                // 改：外侧面为搭接长度即la0
                else if (WallHelper::is_outside_face(inside_outside_faces, face))
                {
                    //end.data.hook_degree.reserver_length = lae;
                    end.data.hook_degree.reserver_length = lae;
                }
                // 其余设置为默认
                else
                {
                    end.data.hook_degree.reserver_length = bend_length;
                }
            };

            // 结束端根据顶面设置
            set_length(top_wall_inside_outside_faces, face, layer.ending_end_type);

            // 计算锚入的旋转角度
            // 即该面的法向与板方向的夹角
            auto top_angle = top_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

            // 转换到角度并设置到端部中
            layer.ending_end_type.angle = top_angle / PI * 180;
			if (is_touch && is_front && front_flag)
			{
				layer.ending_end_type.angle = 180;
			}
			front_flag--;
			if (is_touch && !is_front && (i>1))
			{
				layer.ending_end_type.angle = 0;
			}
            if ((flag) && (top_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
            {
                layer.ending_end_type.angle = 180;
				if (is_touch && !is_front)
				{
					layer.ending_end_type.angle = 0;
				}
            }
            flag++;
        }

    }

    if (bottom_slab_iter != associated.cend())//有底板
    {
        EditElementHandle bottom_slab(bottom_slab_iter->element, true);
        DVec3d bottom_slab_dir;
        if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, bottom_slab, bottom_slab_dir))
        {
            mdlOutput_error(L"分析墙的底板方向失败");
            return;
        }
        auto bottom_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
        WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, bottom_slab, bottom_wall_inside_outside_faces);
        int flag = 0;//标志，T型墙时第二次的竖筋的锚入方向与第一次的方向需要相反
		int front_flag = 1; // 标志，如果墙的前面一层钢筋与孔洞相交需要更换前面一层锚入方向
        for (auto i = 0; i < gallery_settings.layer_count; ++i)
        {
            auto &layer = gallery_settings.layers[i];

            // 横向钢筋不需要锚入
            if (layer.direction != RebarDirection::Vertical)
            {
                continue;
            }

            // 获得项目中钢筋尺寸对应的相关参数
            auto bend_radius = layer.get_pin_radius();
            auto bend_length = layer.get_bend_length(layer.starting_end_type);
            auto diameter = layer.get_rebar_diameter();
            auto lae = layer.get_lae();//锚固长度
            auto la0 = layer.get_la0();//搭接长度
            // 没找到该尺寸钢筋对应的lae参数
            // 使用默认值 
            /*lae = lae < 0 ? bend_length : lae;
            la0 = la0 < 0 ? bend_length : la0;*/
			string tmp(layer.size_str);
			double La_d = stod(tmp);
			auto la_Inside = 15 * La_d * UOR_PER_MilliMeter;
            if (lae > 0)
                //lae = lae * diameter / UOR_PER_MilliMeter;
				lae = lae * La_d;
            else
                lae = bend_length;
            if (la0 > 0)
                //la0 = la0 * diameter / UOR_PER_MilliMeter;
				la0 = la0 * La_d;
            else
                la0 = bend_length;

            // 起点为底端，终点为顶端
            layer.starting_end_type.type = EndType::Hook90;
            layer.starting_end_type.data.hook_degree.radius = bend_radius;

            EditElementHandle *face;
            switch (layer.position)
            {
            case RebarPosition::Front:
                face = wall_geometry_info.front_face;
                break;
            case RebarPosition::Back:
                face = wall_geometry_info.back_face;
                break;
            case RebarPosition::Middle:
                break;
            default:
                break;
            }

			//////////分析孔洞////////////////
			EditElementHandle Eleeh;
			std::vector<EditElementHandle*> Holeehs;
			EFT::GetSolidElementAndSolidHoles(bottom_slab, Eleeh, Holeehs);
			std::vector<EditElementHandle*> useHoleehs;
			CalculateMyUseHoles(useHoleehs, Holeehs, gallery_settings, bottom_slab.GetModelRef());
			bool is_touch = false;//墙是否会接触到孔洞
			bool is_front = false;//是否是前层接触到孔洞
			for (auto hole_eeh : useHoleehs)
			{
				DPoint3d minP_hole;
				DPoint3d maxP_hole;
				//计算板中孔洞元素描述符中元素的范围。
				mdlElmdscr_computeRange(&minP_hole, &maxP_hole, hole_eeh->GetElementDescrP(), NULL);

				DPoint3d minP_wall;
				DPoint3d maxP_wall;
				//计算墙元素描述符中元素的范围。
				mdlElmdscr_computeRange(&minP_wall, &maxP_wall, wall.GetElementDescrP(), NULL);

				DVec3d vec_holeX = DVec3d::From(maxP_hole.x - minP_hole.x, 0, 0);
				vec_holeX.Normalize();
				//if (fabs(fabs(tmpVec.DotProduct(dimVec)) - 1) < 1e-6)
				if (fabs(fabs(wall_geometry_info.normal.DotProduct(vec_holeX)) - 1) < 1e-6)
				{
					if ((abs(maxP_hole.x - minP_wall.x) < la_Inside) || (abs(minP_hole.x - maxP_wall.x) < la_Inside))
					{
						is_touch = true;
						if ((abs(maxP_hole.x - minP_wall.x) < la_Inside))
							is_front = false;
						else if ((abs(minP_hole.x - maxP_wall.x) < la_Inside))
							is_front = true;

					}
				}
				else
				{
					if ((abs(maxP_hole.y - minP_wall.y) < la_Inside) || (abs(minP_hole.y - maxP_wall.y) < la_Inside))
					{
						is_touch = true;
						if ((abs(maxP_hole.y - minP_wall.y) < la_Inside))
							is_front = true;
						else if ((abs(minP_hole.y - maxP_wall.y) < la_Inside))
							is_front = false;

					}
				}

			}
			//////////分析孔洞////////////////

            // 根据面是否为内侧设置钢筋锚入长度
            auto set_length =
                [&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
            {
                // 内侧面锚入长度使用15d, d为钢筋直径
                // 改：内侧面为锚固长度lae
                if (WallHelper::is_inside_face(inside_outside_faces, face))
                {
                    //end.data.hook_degree.reserver_length = 15 * diameter;
                    end.data.hook_degree.reserver_length = la_Inside;
                    // 内侧面钢筋两端还需要缩短两个钢筋直径长度，为板、外侧钢筋预留空间
                    end.offset = -diameter / UOR_PER_MilliMeter * 2;
                }
                // 外侧面设置为Lae
                // 改：外侧面为搭接长度la0
                else if (WallHelper::is_outside_face(inside_outside_faces, face))
                {
                    //end.data.hook_degree.reserver_length = lae;
                    end.data.hook_degree.reserver_length = lae;
                }
                // 其余设置为默认
                else
                {
                    end.data.hook_degree.reserver_length = bend_length;
                }
            };

            // 起始端根据底面设置
            set_length(bottom_wall_inside_outside_faces, face, layer.starting_end_type);

            // 计算锚入的旋转角度
            // 即该面的法向与板方向的夹角
            auto bottom_angle = bottom_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

            // 转换到角度并设置到端部中
            layer.starting_end_type.angle = bottom_angle / PI * 180;
			if (is_touch && is_front && front_flag)
			{
				layer.starting_end_type.angle = 180;
			}
			front_flag--;
			if (is_touch && !is_front && (i > 1))
			{
				layer.starting_end_type.angle = 0;
			}
            if ((flag) && (bottom_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
            {
                layer.starting_end_type.angle = 180;
				if (is_touch && !is_front)
				{
					layer.starting_end_type.angle = 0;
				}
            }
            flag++;

        }
    }
    /*
    if (top_slab_iter == associated.cend() || bottom_slab_iter == associated.cend())
    {
        mdlOutput_error(L"未找到顶板或底板");
        return;
    }

    EditElementHandle top_slab(top_slab_iter->element, true);
    EditElementHandle bottom_slab(bottom_slab_iter->element, true);

    // 计算顶板与底板的朝向
    DVec3d top_slab_dir;
    if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, top_slab, top_slab_dir))
    {
        mdlOutput_error(L"分析墙的顶板方向失败");
        return;
    }
    DVec3d bottom_slab_dir;
    if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, bottom_slab, bottom_slab_dir))
    {
        mdlOutput_error(L"分析墙的底板方向失败");
        return;
    }

    // 分析对顶板、底板的内侧面和外侧面
    auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
    auto bottom_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
    WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, top_slab, top_wall_inside_outside_faces);
    WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, bottom_slab, bottom_wall_inside_outside_faces);

    // 根据内侧和外侧设置锚入
    // 这里正反都一样的，而且只需要考虑竖向的锚入，所以可以提前处理好
    int flag = 0;//标志，T型墙时第二次的竖筋的锚入方向与第一次的方向需要相反
    for (auto i = 0; i < gallery_settings.layer_count; ++i)
    {
        auto &layer = gallery_settings.layers[i];

        // 横向钢筋不需要锚入
        if (layer.direction != RebarDirection::Vertical)
        {
            continue;
        }


        // 获得项目中钢筋尺寸对应的相关参数
        auto bend_radius = layer.get_pin_radius();
        auto bend_length = layer.get_bend_length(layer.starting_end_type);
        auto diameter = layer.get_rebar_diameter();
        auto lae = layer.get_lae();
        // 没找到该尺寸钢筋对应的lae参数
        // 使用默认值 
        lae = lae < 0 ? bend_length : lae;

        // 起点为底端，终点为顶端
        layer.starting_end_type.type = EndType::Hook90;
        layer.starting_end_type.data.hook_degree.radius = bend_radius;
        layer.ending_end_type.type = EndType::Hook90;
        layer.ending_end_type.data.hook_degree.radius = bend_radius;

        EditElementHandle *face;
        switch (layer.position)
        {
        case RebarPosition::Front:
            face = wall_geometry_info.front_face;
            break;
        case RebarPosition::Back:
            face = wall_geometry_info.back_face;
            break;
        case RebarPosition::Middle:
            break;
        default:
            break;
        }

        // 根据面是否为内侧设置钢筋锚入长度
        auto set_length = 
            [&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
            {
                // 内侧面锚入长度使用15d, d为钢筋直径
                if (WallHelper::is_inside_face(inside_outside_faces, face))
                {
                    end.data.hook_degree.reserver_length = 15 * diameter;
                    // 内侧面钢筋两端还需要缩短两个钢筋直径长度，为板、外侧钢筋预留空间
                    end.offset = -diameter / UOR_PER_MilliMeter * 2;
                }
                // 外侧面设置为Lae
                else if(WallHelper::is_outside_face(inside_outside_faces, face))
                {
                    end.data.hook_degree.reserver_length = lae;
                }
                // 其余设置为默认
                else
                {
                    end.data.hook_degree.reserver_length = bend_length;
                }
            };

        // 起始端根据底面设置
        set_length(bottom_wall_inside_outside_faces, face, layer.starting_end_type);
        // 结束端根据顶面设置
        set_length(top_wall_inside_outside_faces, face, layer.ending_end_type);

        // 计算锚入的旋转角度
        // 即该面的法向与板方向的夹角
        auto top_angle = top_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));
        auto bottom_angle = bottom_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

        // 转换到角度并设置到端部中
        layer.starting_end_type.angle = bottom_angle / PI * 180;
        layer.ending_end_type.angle = top_angle / PI * 180;
        if ((flag) && (top_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
        {
            layer.starting_end_type.angle = 180;
            layer.ending_end_type.angle = 180;
        }
        flag++;
    }
    */

    // 对钢筋按位置分类
    std::vector<GallerySettings::Layer> front_layers;
    std::vector<GallerySettings::Layer> middle_layers;
    std::vector<GallerySettings::Layer> back_layers;

    for (auto layer_ptr = gallery_settings.layers;
         layer_ptr != gallery_settings.layers + gallery_settings.layer_count;
         ++layer_ptr)
    {
        const auto &layer = *layer_ptr;
        switch (layer.position)
        {
        case RebarPosition::Front:
            front_layers.push_back(layer);
            break;
        case RebarPosition::Middle:
            middle_layers.push_back(layer);
            break;
        case RebarPosition::Back:
            back_layers.push_back(layer);
            break;
        default:
            break;
        }
    }

    auto concrete_id = _local::get_concrete_id(wall);

    auto rebar_assembly = GeneralFaceRebarAssembly::from_concrete(concrete_id, ACTIVEMODEL);
    if (rebar_assembly == nullptr)
    {
        rebar_assembly = GeneralFaceRebarAssembly::create(ACTIVEMODEL, wall.GetElementId(), wall.GetModelRef());
    }

    if (rebar_assembly == nullptr)
    {
        // 错误处理
        mdlOutput_error(L"创建assembly失败");
        return;
    }

    // rebar_assembly->remove_rebar();
    rebar_assembly->begin_generate();
    rebar_assembly->on_double_click(
        [](GeneralFaceRebarAssembly &assembly) -> bool
        {
            // 获得钢筋所在的墙
            auto wall_id = assembly.GetSelectedElement();
            auto wall_model = assembly.GetSelectedModel();
            auto wall = ElementHandle(wall_id, wall_model);
            // 清除之前的选择
            auto &manager = SelectionSetManager::GetManager();
            manager.EmptyAll();
            // 将墙加入选择集中
            manager.AddElement(wall.GetElementRef(), wall.GetModelRef());

            // 再次打开配筋窗体
            // 该窗体会读取选择集中选中的墙中存储的参数
            cmd(L"");

            return true;
        });

    // 按不同的钢筋位置生成钢筋
    if (front_layers.size() > 0)
    {
        _local::make_rebar_in_position(
            wall,
            *rebar_assembly,
            gallery_settings,
            wall_geometry_info,
            associated,
            front_layers,
            GeneralFaceRebarAssembly::ArrayMode::Fit);
    }
    if (back_layers.size() > 0)
    {
        _local::make_rebar_in_position(
            wall,
            *rebar_assembly,
            gallery_settings,
            wall_geometry_info,
            associated,
            back_layers,
            GeneralFaceRebarAssembly::ArrayMode::Fit);
    }
    rebar_assembly->end_generate();

    // 获得新的混凝土id
    // auto concrete_id = wall_rebar_assembly->FetchConcrete();
    concrete_id = rebar_assembly->FetchConcrete();
    EditElementHandle concrete(concrete_id, ACTIVEMODEL);

    // 设置为不可见
    auto old_concrete_ref = concrete.GetElementRef();
    mdlElmdscr_setVisible(concrete.GetElementDescrP(), false);
    concrete.ReplaceInModel(old_concrete_ref);

    // 写入新的混凝土id
    _local::set_concrete_id(wall, concrete_id);

    if (WallHelper::combineTop == true)
    {
        WallHelper::combineTop = false;
        for (auto combinefloor : associated)
        {
            if (combinefloor.type == WallHelper::AssociatedType::TopFloor)
            {
                EditElementHandle combinedTopfloor(combinefloor.element, true);
                combinedTopfloor.DeleteFromModel();
            }
        }
    }
    if (WallHelper::combineBottom == true)
    {
        WallHelper::combineBottom = false;
        for (auto combinefloor : associated)
        {
            if (combinefloor.type == WallHelper::AssociatedType::BottomFloor)
            {
                EditElementHandle combinedBottomfloor(combinefloor.element, true);
                combinedBottomfloor.DeleteFromModel();
            }
        }
    }
}


/// 计算得到符合条件的孔洞集合 useHoleehs
/// 当孔洞大小大于钢筋间距时即符合条件
void CalculateMyUseHoles(vector<EditElementHandle*> &useHoleehs, vector<EditElementHandle*> Holeehs, GallerySettings gallery_settings, DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * gallery_settings.layers->spacing;
	useHoleehs.clear();
	//double dSideCover = gallery_settings.side_cover * uor_per_mm;
	/*Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);*/
	if (1)//计算需要处理的孔洞
	{
		for (int j = 0; j < Holeehs.size(); j++)
		{
			EditElementHandle eeh;
			eeh.Duplicate(*Holeehs.at(j));

			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
			}

			//TransformInfo transinfo(trans);
			//eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			DPoint3d minP;
			DPoint3d maxP;
			//计算指定元素描述符中元素的范围。
			mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
			DRange3d range;
			range.low = minP;
			range.high = maxP;
			bool isNeed = false;
			if (range.XLength() > misssize || range.YLength() > misssize)
			{
				isNeed = true;
			}

			if (isNeed)
			{
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*Holeehs.at(j));
				//PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
				useHoleehs.push_back(Holeehs.at(j));
			}
		}
	}
	if (useHoleehs.size() > 1)
	{
		UnionIntersectHoles(useHoleehs, Holeehs);
	}

}

void execute_make_rebar(EditElementHandleR wall, GallerySettings &gallery_settings)
{
	CString str;
	/// 读取墙上存的相关信息
	//GallerySettings gallery_settings;
	//if (!GallerySettings::from_element(wall, gallery_settings))
	//{
	//	mdlOutput_error(L"读取墙配置数据失败");
	//	return;
	//}

	// 分析墙的几何参数
	WallHelper::WallGeometryInfo wall_geometry_info;
	if (!WallHelper::analysis_geometry(wall, wall_geometry_info))
	{
		mdlOutput_error(L"获得墙几何信息失败");
		return;
	}

	/// 扫描与当前墙相连的元素
	std::vector<WallHelper::Associated> associated;
	if (!WallHelper::analysis_associated(wall, associated, wall_geometry_info))
	{
		mdlOutput_error(L"扫描相连构件失败");
		return;
	}

	auto top_slab_iter =
		std::find_if(
			associated.cbegin(), associated.cend(),
			[](const WallHelper::Associated &assoc)
			{
				return assoc.type == WallHelper::AssociatedType::TopFloor;
			});
	auto bottom_slab_iter =
		std::find_if(
			associated.cbegin(), associated.cend(),
			[](const WallHelper::Associated &assoc)
			{
				return assoc.type == WallHelper::AssociatedType::BottomFloor;
			});

	/*if (top_slab_iter == associated.cend() && bottom_slab_iter == associated.cend())
	{
		mdlOutput_error(L"未找到顶板和底板");
		return;
	}*/

	for (auto i = 0; i < gallery_settings.layer_count; ++i)
	{
		auto &layer = gallery_settings.layers[i];

		// 横向钢筋不需要锚入
		if (layer.direction != RebarDirection::Vertical)
		{
			continue;
		}

		// 获得项目中钢筋尺寸对应的相关参数
		auto bend_radius = layer.get_pin_radius();
		auto bend_length = layer.get_bend_length(layer.starting_end_type);
		auto diameter = layer.get_rebar_diameter();
		auto lae = layer.get_lae();
		// 没找到该尺寸钢筋对应的lae参数
		// 使用默认值 
		lae = lae < 0 ? bend_length : lae;

		// 起点为底端，终点为顶端
		layer.starting_end_type.type = EndType::None;
		layer.ending_end_type.type = EndType::None;
		layer.starting_end_type.angle = 0;
		layer.ending_end_type.angle = 0;
	}

	if (top_slab_iter != associated.cend())//有顶板
	{
		EditElementHandle top_slab(top_slab_iter->element, true);
		DVec3d top_slab_dir;
		if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, top_slab, top_slab_dir))
		{
			mdlOutput_error(L"分析墙的顶板方向失败");
			return;
		}
		auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
		WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, top_slab, top_wall_inside_outside_faces);
		int flag = 0;//标志，T型墙时第二次的竖筋的锚入方向与第一次的方向需要相反
		for (auto i = 0; i < gallery_settings.layer_count; ++i)
		{
			auto &layer = gallery_settings.layers[i];

			// 横向钢筋不需要锚入
			if (layer.direction != RebarDirection::Vertical)
			{
				continue;
			}
			// 获得项目中钢筋尺寸对应的相关参数
			auto bend_radius = layer.get_pin_radius();
			auto bend_length = layer.get_bend_length(layer.starting_end_type);
			auto diameter = layer.get_rebar_diameter();
			auto lae = layer.get_lae();//锚固
			auto la0 = layer.get_la0();//搭接
			auto d = layer.size_str;
			// 没找到该尺寸钢筋对应的lae参数
			// 使用默认值 
			/*lae = lae < 0 ? bend_length : lae;
			la0 = la0 < 0 ? bend_length : la0;*/
			string tmp(layer.size_str);
			double La_d = stod(tmp);

			if (lae > 0)
				//lae = lae * diameter / UOR_PER_MilliMeter;
				lae = lae * La_d;
			else
				lae = bend_length;
			if (la0 > 0)
				//la0 = la0 * diameter / UOR_PER_MilliMeter;
				la0 = la0 * La_d;
			else
				la0 = bend_length;

			// 起点为底端，终点为顶端
			layer.ending_end_type.type = EndType::Hook90;
			layer.ending_end_type.data.hook_degree.radius = bend_radius;

			EditElementHandle *face;
			switch (layer.position)
			{
			case RebarPosition::Front:
				face = wall_geometry_info.front_face;
				break;
			case RebarPosition::Back:
				face = wall_geometry_info.back_face;
				break;
			case RebarPosition::Middle:
				break;
			default:
				break;
			}

			// 根据面是否为内侧设置钢筋锚入长度
			auto set_length =
				[&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
			{
				// 内侧面锚入长度使用15d, d为钢筋直径
				// 改：内侧面为锚固长度即lae
				if (WallHelper::is_inside_face(inside_outside_faces, face))
				{
					//end.data.hook_degree.reserver_length = 15 * diameter;
					end.data.hook_degree.reserver_length = lae;
					// 内侧面钢筋两端还需要缩短两个钢筋直径长度，为板、外侧钢筋预留空间
					end.offset = -diameter / UOR_PER_MilliMeter * 2;
				}
				// 外侧面设置为Lae
				// 改：外侧面为搭接长度即la0
				else if (WallHelper::is_outside_face(inside_outside_faces, face))
				{
					//end.data.hook_degree.reserver_length = lae;
					end.data.hook_degree.reserver_length = la0;
				}
				// 其余设置为默认
				else
				{
					end.data.hook_degree.reserver_length = bend_length;
				}
			};

			// 结束端根据顶面设置
			set_length(top_wall_inside_outside_faces, face, layer.ending_end_type);

			// 计算锚入的旋转角度
			// 即该面的法向与板方向的夹角
			auto top_angle = top_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

			// 转换到角度并设置到端部中
			layer.ending_end_type.angle = top_angle / PI * 180;
			if ((flag) && (top_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
			{
				layer.ending_end_type.angle = 180;
			}
			flag++;
		}

	}

	if (bottom_slab_iter != associated.cend())//有底板
	{
		EditElementHandle bottom_slab(bottom_slab_iter->element, true);
		DVec3d bottom_slab_dir;
		if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, bottom_slab, bottom_slab_dir))
		{
			mdlOutput_error(L"分析墙的底板方向失败");
			return;
		}
		auto bottom_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
		WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, bottom_slab, bottom_wall_inside_outside_faces);
		int flag = 0;//标志，T型墙时第二次的竖筋的锚入方向与第一次的方向需要相反
		for (auto i = 0; i < gallery_settings.layer_count; ++i)
		{
			auto &layer = gallery_settings.layers[i];

			// 横向钢筋不需要锚入
			if (layer.direction != RebarDirection::Vertical)
			{
				continue;
			}

			// 获得项目中钢筋尺寸对应的相关参数
			auto bend_radius = layer.get_pin_radius();
			auto bend_length = layer.get_bend_length(layer.starting_end_type);
			auto diameter = layer.get_rebar_diameter();
			auto lae = layer.get_lae();//锚固长度
			auto la0 = layer.get_la0();//搭接长度
			// 没找到该尺寸钢筋对应的lae参数
			// 使用默认值 
			/*lae = lae < 0 ? bend_length : lae;
			la0 = la0 < 0 ? bend_length : la0;*/
			string tmp(layer.size_str);
			double La_d = stod(tmp);

			if (lae > 0)
				//lae = lae * diameter / UOR_PER_MilliMeter;
				lae = lae * La_d;
			else
				lae = bend_length;
			if (la0 > 0)
				//la0 = la0 * diameter / UOR_PER_MilliMeter;
				la0 = la0 * La_d;
			else
				la0 = bend_length;

			// 起点为底端，终点为顶端
			layer.starting_end_type.type = EndType::Hook90;
			layer.starting_end_type.data.hook_degree.radius = bend_radius;

			EditElementHandle *face;
			switch (layer.position)
			{
			case RebarPosition::Front:
				face = wall_geometry_info.front_face;
				break;
			case RebarPosition::Back:
				face = wall_geometry_info.back_face;
				break;
			case RebarPosition::Middle:
				break;
			default:
				break;
			}

			// 根据面是否为内侧设置钢筋锚入长度
			auto set_length =
				[&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
			{
				// 内侧面锚入长度使用15d, d为钢筋直径
				// 改：内侧面为锚固长度lae
				if (WallHelper::is_inside_face(inside_outside_faces, face))
				{
					//end.data.hook_degree.reserver_length = 15 * diameter;
					end.data.hook_degree.reserver_length = lae;
					// 内侧面钢筋两端还需要缩短两个钢筋直径长度，为板、外侧钢筋预留空间
					end.offset = -diameter / UOR_PER_MilliMeter * 2;
				}
				// 外侧面设置为Lae
				// 改：外侧面为搭接长度la0
				else if (WallHelper::is_outside_face(inside_outside_faces, face))
				{
					//end.data.hook_degree.reserver_length = lae;
					end.data.hook_degree.reserver_length = la0;
				}
				// 其余设置为默认
				else
				{
					end.data.hook_degree.reserver_length = bend_length;
				}
			};

			// 起始端根据底面设置
			set_length(bottom_wall_inside_outside_faces, face, layer.starting_end_type);

			// 计算锚入的旋转角度
			// 即该面的法向与板方向的夹角
			auto bottom_angle = bottom_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

			// 转换到角度并设置到端部中
			layer.starting_end_type.angle = bottom_angle / PI * 180;
			if ((flag) && (bottom_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
			{
				layer.starting_end_type.angle = 180;
			}
			flag++;
		}
	}
	/*
	if (top_slab_iter == associated.cend() || bottom_slab_iter == associated.cend())
	{
		mdlOutput_error(L"未找到顶板或底板");
		return;
	}

	EditElementHandle top_slab(top_slab_iter->element, true);
	EditElementHandle bottom_slab(bottom_slab_iter->element, true);

	// 计算顶板与底板的朝向
	DVec3d top_slab_dir;
	if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, top_slab, top_slab_dir))
	{
		mdlOutput_error(L"分析墙的顶板方向失败");
		return;
	}
	DVec3d bottom_slab_dir;
	if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, bottom_slab, bottom_slab_dir))
	{
		mdlOutput_error(L"分析墙的底板方向失败");
		return;
	}

	// 分析对顶板、底板的内侧面和外侧面
	auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
	auto bottom_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
	WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, top_slab, top_wall_inside_outside_faces);
	WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, bottom_slab, bottom_wall_inside_outside_faces);

	// 根据内侧和外侧设置锚入
	// 这里正反都一样的，而且只需要考虑竖向的锚入，所以可以提前处理好
	int flag = 0;//标志，T型墙时第二次的竖筋的锚入方向与第一次的方向需要相反
	for (auto i = 0; i < gallery_settings.layer_count; ++i)
	{
		auto &layer = gallery_settings.layers[i];

		// 横向钢筋不需要锚入
		if (layer.direction != RebarDirection::Vertical)
		{
			continue;
		}


		// 获得项目中钢筋尺寸对应的相关参数
		auto bend_radius = layer.get_pin_radius();
		auto bend_length = layer.get_bend_length(layer.starting_end_type);
		auto diameter = layer.get_rebar_diameter();
		auto lae = layer.get_lae();
		// 没找到该尺寸钢筋对应的lae参数
		// 使用默认值
		lae = lae < 0 ? bend_length : lae;

		// 起点为底端，终点为顶端
		layer.starting_end_type.type = EndType::Hook90;
		layer.starting_end_type.data.hook_degree.radius = bend_radius;
		layer.ending_end_type.type = EndType::Hook90;
		layer.ending_end_type.data.hook_degree.radius = bend_radius;

		EditElementHandle *face;
		switch (layer.position)
		{
		case RebarPosition::Front:
			face = wall_geometry_info.front_face;
			break;
		case RebarPosition::Back:
			face = wall_geometry_info.back_face;
			break;
		case RebarPosition::Middle:
			break;
		default:
			break;
		}

		// 根据面是否为内侧设置钢筋锚入长度
		auto set_length =
			[&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
			{
				// 内侧面锚入长度使用15d, d为钢筋直径
				if (WallHelper::is_inside_face(inside_outside_faces, face))
				{
					end.data.hook_degree.reserver_length = 15 * diameter;
					// 内侧面钢筋两端还需要缩短两个钢筋直径长度，为板、外侧钢筋预留空间
					end.offset = -diameter / UOR_PER_MilliMeter * 2;
				}
				// 外侧面设置为Lae
				else if(WallHelper::is_outside_face(inside_outside_faces, face))
				{
					end.data.hook_degree.reserver_length = lae;
				}
				// 其余设置为默认
				else
				{
					end.data.hook_degree.reserver_length = bend_length;
				}
			};

		// 起始端根据底面设置
		set_length(bottom_wall_inside_outside_faces, face, layer.starting_end_type);
		// 结束端根据顶面设置
		set_length(top_wall_inside_outside_faces, face, layer.ending_end_type);

		// 计算锚入的旋转角度
		// 即该面的法向与板方向的夹角
		auto top_angle = top_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));
		auto bottom_angle = bottom_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

		// 转换到角度并设置到端部中
		layer.starting_end_type.angle = bottom_angle / PI * 180;
		layer.ending_end_type.angle = top_angle / PI * 180;
		if ((flag) && (top_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
		{
			layer.starting_end_type.angle = 180;
			layer.ending_end_type.angle = 180;
		}
		flag++;
	}
	*/

	// 对钢筋按位置分类
	std::vector<GallerySettings::Layer> front_layers;
	std::vector<GallerySettings::Layer> middle_layers;
	std::vector<GallerySettings::Layer> back_layers;

	for (auto layer_ptr = gallery_settings.layers;
		layer_ptr != gallery_settings.layers + gallery_settings.layer_count;
		++layer_ptr)
	{
		const auto &layer = *layer_ptr;
		switch (layer.position)
		{
		case RebarPosition::Front:
			front_layers.push_back(layer);
			break;
		case RebarPosition::Middle:
			middle_layers.push_back(layer);
			break;
		case RebarPosition::Back:
			back_layers.push_back(layer);
			break;
		default:
			break;
		}
	}

	auto concrete_id = _local::get_concrete_id(wall);

	auto rebar_assembly = GeneralFaceRebarAssembly::from_concrete(concrete_id, ACTIVEMODEL);
	if (rebar_assembly == nullptr)
	{
		rebar_assembly = GeneralFaceRebarAssembly::create(ACTIVEMODEL, wall.GetElementId(), wall.GetModelRef());
	}

	if (rebar_assembly == nullptr)
	{
		// 错误处理
		mdlOutput_error(L"创建assembly失败");
		return;
	}

	// rebar_assembly->remove_rebar();
	rebar_assembly->begin_generate();
	rebar_assembly->on_double_click(
		[](GeneralFaceRebarAssembly &assembly) -> bool
		{
			// 获得钢筋所在的墙
			auto wall_id = assembly.GetSelectedElement();
			auto wall_model = assembly.GetSelectedModel();
			auto wall = ElementHandle(wall_id, wall_model);
			// 清除之前的选择
			auto &manager = SelectionSetManager::GetManager();
			manager.EmptyAll();
			// 将墙加入选择集中
			manager.AddElement(wall.GetElementRef(), wall.GetModelRef());

			// 再次打开配筋窗体
			// 该窗体会读取选择集中选中的墙中存储的参数
			cmd(L"");

			return true;
		});

	// 按不同的钢筋位置生成钢筋
	if (front_layers.size() > 0)
	{
		_local::make_rebar_in_position(
			wall,
			*rebar_assembly,
			gallery_settings,
			wall_geometry_info,
			associated,
			front_layers,
			GeneralFaceRebarAssembly::ArrayMode::Fit);
	}
	if (back_layers.size() > 0)
	{
		_local::make_rebar_in_position(
			wall,
			*rebar_assembly,
			gallery_settings,
			wall_geometry_info,
			associated,
			back_layers,
			GeneralFaceRebarAssembly::ArrayMode::Fit);
	}
	rebar_assembly->end_generate();

	// 获得新的混凝土id
	// auto concrete_id = wall_rebar_assembly->FetchConcrete();
	concrete_id = rebar_assembly->FetchConcrete();
	EditElementHandle concrete(concrete_id, ACTIVEMODEL);

	// 设置为不可见
	auto old_concrete_ref = concrete.GetElementRef();
	mdlElmdscr_setVisible(concrete.GetElementDescrP(), false);
	concrete.ReplaceInModel(old_concrete_ref);

	// 写入新的混凝土id
	_local::set_concrete_id(wall, concrete_id);

	if (WallHelper::combineTop == true)
	{
		WallHelper::combineTop = false;
		for (auto combinefloor : associated)
		{
			if (combinefloor.type == WallHelper::AssociatedType::TopFloor)
			{
				EditElementHandle combinedTopfloor(combinefloor.element, true);
				combinedTopfloor.DeleteFromModel();
			}
		}
	}
	if (WallHelper::combineBottom == true)
	{
		WallHelper::combineBottom = false;
		for (auto combinefloor : associated)
		{
			if (combinefloor.type == WallHelper::AssociatedType::BottomFloor)
			{
				EditElementHandle combinedBottomfloor(combinefloor.element, true);
				combinedBottomfloor.DeleteFromModel();
			}
		}
	}
}

void _local::make_rebar_in_position(
    EditElementHandle &wall,
    GeneralFaceRebarAssembly &assembly,
    const GallerySettings &settings,
    const WallHelper::WallGeometryInfo &wall_geometry_info,
    const std::vector<WallHelper::Associated> &associated,
    const std::vector<GallerySettings::Layer> &_layers,
    GeneralFaceRebarAssembly::ArrayMode array_mode)
{
    // 复制层参数并按层号排序
    auto layers = _layers;
    std::sort(
        layers.begin(), layers.end(),
        [](const GallerySettings::Layer &layer_a, const GallerySettings::Layer &layer_b) -> bool
        {
            return layer_a.layer_index < layer_b.layer_index;
        });

    // 取出所有的顶板、底板
    std::vector<std::unique_ptr<EditElementHandle>> top_slabs;
    std::vector<std::unique_ptr<EditElementHandle>> bottom_slabs;
    for (const auto &assoc : associated)
    {
        switch (assoc.type)
        {
        case WallHelper::AssociatedType::TopFloor:
        {
            auto eeh = new EditElementHandle(assoc.element, true);
            auto ptr = std::make_unique<EditElementHandle>();
            ptr.reset(eeh);
            top_slabs.push_back(std::move(ptr));
            break;
        }
        case WallHelper::AssociatedType::BottomFloor:
        {
            auto eeh = new EditElementHandle(assoc.element, true);
            auto ptr = std::make_unique<EditElementHandle>();
            ptr.reset(eeh);
            bottom_slabs.push_back(std::move(ptr));
            break;
        }
        default:
            break;
        }
    }

    // 根据layers第一个元素判断当前是从哪个面开始配筋
    // 每层配筋结束后都会偏移面, 因为不同层的钢筋需要错开, 这里是偏移的方向
    DVec3d face_offset_dir;
    // 剖切面
    std::unique_ptr<EditElementHandle> cut_face;
    // 选中的需要配筋的面，这个用来判断在内侧还是在外侧
    // 中间层的话，为nullptr
    EditElementHandle *face = nullptr;
    // 保护层偏移量， 单位UOR
    double cover_offset = 0;
    MSElementDescrP desc_p = nullptr;
    switch (layers.front().position)
    {
    case RebarPosition::Front:
        // 正面的话是沿法向的相反方向生成不同层的钢筋
        face_offset_dir = wall_geometry_info.normal;
        face_offset_dir.Negate();
        cover_offset = settings.front_cover * UOR_PER_MilliMeter;
        face = wall_geometry_info.front_face;
        // 复制一个面
        face->GetElementDescrCP()->Duplicate(&desc_p);
        cut_face.reset(new EditElementHandle(desc_p, true, false, ACTIVEMODEL));
        break;
    case RebarPosition::Back:
        face_offset_dir = wall_geometry_info.normal;
        face = wall_geometry_info.back_face;
        cover_offset = settings.back_cover * UOR_PER_MilliMeter;
        // 复制一个面
        face->GetElementDescrCP()->Duplicate(&desc_p);
        cut_face.reset(new EditElementHandle(desc_p, true, false, ACTIVEMODEL));
        break;
    case RebarPosition::Middle:
        // 中间层与正面钢筋相同
        // 中间层需要单独处理，现在不做这个处理
        // face_offset_dir = -wall_geometry_info.normal;
        break;
    default:
        break;
    }

    // 分析当前面和顶板、底板的位置关系(内外)
    WallHelper::WallInsideOutsideFaces top_inside_outside_faces;
    WallHelper::WallInsideOutsideFaces bottom_inside_outside_faces;
    if (top_slabs.size() > 0)
    {
        if (!WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, *top_slabs.front(), top_inside_outside_faces))
        {
            mdlOutput_error(L"分析墙面与板的位置关系失败");
            return;
        }
    }
    if (bottom_slabs.size() > 0)
    {
        if (!WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, *bottom_slabs.front(), bottom_inside_outside_faces))
        {
            mdlOutput_error(L"分析墙面与板的位置关系失败");
            return;
        }
    }
    // 分析当前面和顶板、底板的位置关系(内外)
    //WallHelper::WallInsideOutsideFaces top_inside_outside_faces;
    //WallHelper::WallInsideOutsideFaces bottom_inside_outside_faces;
    //// 随便取一个面
    //if (!WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, *top_slabs.front(), top_inside_outside_faces))
    //{
    //    mdlOutput_error(L"分析墙面与板的位置关系失败");
    //    return;
    //}
    //if (!WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, *bottom_slabs.front(), bottom_inside_outside_faces))
    //{
    //    mdlOutput_error(L"分析墙面与板的位置关系失败");
    //    return;
    //}

    // 水平钢筋方向就是墙的方向
    auto horizontal_rebar_dir = wall_geometry_info.direction;

    // 将剖切面沿z轴扩大
    // 在z方向上放大10倍
    scale_element_in_place(*cut_face, DVec3d::From(1, 1, 10));

    // 创建一个用于移动剖切面的变换
    // 先将该剖切面移动一个保护层距离
    auto translation_transform = Transform::From(0, 0, 0);
    auto cover_translation = face_offset_dir * cover_offset;
    translation_transform.SetTranslation(cover_translation);
    cut_face->GetHandler().ApplyTransform(*cut_face, TransformInfo(translation_transform));
	//cut_face->AddToModel();
    // 生成该面的每一层钢筋
    for(const auto &layer : layers)
    {
        // 读取该层钢筋的直径
        auto diameter = layer.get_rebar_diameter();

        // 平移一个钢筋半径
        auto layer_offset = face_offset_dir * diameter / 2.0;
        translation_transform.SetTranslation(layer_offset);

        // 平移一下这个剖切面
        cut_face->GetHandler().ApplyTransform(*cut_face, TransformInfo(translation_transform));

        // 计算横向、纵向钢筋的钢筋方向
        auto rebar_dir =
            layer.direction == RebarDirection::Horizontal ? horizontal_rebar_dir : DVec3d::UnitZ();

        // 竖筋需要错开一根钢筋的距离防止与板中的钢筋碰撞
        auto collision_offset =
            layer.direction == RebarDirection::Horizontal ? 0.0 : diameter;

        // 计算需要剖切的构件
        std::vector<const EditElementHandle *> to_be_cut_elements;
        // 墙是肯定要参与剖切的
        to_be_cut_elements.push_back(&wall);
        if (layer.direction == RebarDirection::Vertical)
        {
            // 竖直钢筋需要锚入到板，所以顶板、底板都需要参与剖切
            if (top_slabs.size() > 0)
            {
                std::transform(
                    top_slabs.cbegin(), top_slabs.cend(), std::back_inserter(to_be_cut_elements),
                    [](const std::unique_ptr<EditElementHandle> &ptr)
                { return ptr.get(); });
            }
            if (bottom_slabs.size() > 0)
            {
                std::transform(
                    bottom_slabs.cbegin(), bottom_slabs.cend(), std::back_inserter(to_be_cut_elements),
                    [](const std::unique_ptr<EditElementHandle> &ptr)
                { return ptr.get(); });
            }
            
        }
        else
        {
            // 横向需要根据内外侧判断是否需要在板中添加
            // 外侧需要加入板中
            if (top_slabs.size() > 0)
            {
                if (WallHelper::is_outside_face(top_inside_outside_faces, face))
                {
                    std::transform(
                        top_slabs.cbegin(), top_slabs.cend(), std::back_inserter(to_be_cut_elements),
                        [](const std::unique_ptr<EditElementHandle> &ptr)
                    { return ptr.get(); });
                }
            }
            if (bottom_slabs.size() > 0)
            {
                if (WallHelper::is_outside_face(bottom_inside_outside_faces, face))
                {
                    std::transform(
                        bottom_slabs.cbegin(), bottom_slabs.cend(), std::back_inserter(to_be_cut_elements),
                        [](const std::unique_ptr<EditElementHandle> &ptr)
                    { return ptr.get(); });
                }
            }
            
        }

        // 计算剖切结果
        std::vector<EditElementHandle *> cut_result_faces;
        for (const auto element_ptr : to_be_cut_elements)
        {
            const auto &element = *element_ptr;
            auto cut_result_face_ptr = new EditElementHandle();
            if (!PITCommonTool::CSolidTool::SolidBoolWithFace(*cut_result_face_ptr, *cut_face, element, BOOLOPERATION::BOOLOPERATION_INTERSECT))
            {
                mdlOutput_error(L"剖切配筋面失败");
                return;
            }
		
			ElementAgenda agenda;//存放打散之后的元素
			DropGeometryPtr pDropGeometry = DropGeometry::Create();//创建一个DropGeometry实例来设置几何选项
			int type = cut_result_face_ptr->GetElementType();
        	if (mdlSolid_isSmartSheetElement(cut_result_face_ptr->GetElementDescrP(), cut_result_face_ptr->GetDgnModelP()))//如果是智能suface对象就需要打散面，然后再合并成一个新的面
			{
				pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Solids);
				pDropGeometry->SetSolidsOptions(DropGeometry::SOLID_Surfaces);
				if (SUCCESS != cut_result_face_ptr->GetDisplayHandler()->Drop(*cut_result_face_ptr, agenda, *pDropGeometry))
				{
					agenda.Clear();
					mdlOutput_printf(MSG_STATUS, L"打散智能曲面失败，无法获取面!");
					continue;
				}
				// 拆分面后的容器
				std::vector<EditElementHandle *> faces;
				for(int i=0;i<agenda.size();i++)
				{
					EditElementHandle* eehtemp=new EditElementHandle(agenda[i], agenda[i].GetModelRef());
					faces.push_back(eehtemp);
				}
				
				// 放大万分之一, 防止模型之间有缝，导致合并剖切面失败
				std::for_each(
					faces.cbegin(), faces.cend(),
					[](EditElementHandle *face)
					{
						_local::scale_element_in_place(*face, DVec3d::From(1, 1, 1));
					});

				// 合并所有剖切结果到一个面
				// 转换到desc
				auto faces_desc = std::vector<MSElementDescrP>();
				std::transform(
					faces.begin(), faces.end(),
					std::back_inserter(faces_desc),
					[](EditElementHandle *eeh)
					{
						return eeh->GetElementDescrP();
					});
				MSElementDescrP combined_face_desc = nullptr;
				PITCommonTool::CFaceTool::CombineFaces(combined_face_desc, faces_desc, ACTIVEMODEL, false);
				EditElementHandle* combined_face=new EditElementHandle(combined_face_desc, true, false, ACTIVEMODEL);
				//combined_face->AddToModel();
				cut_result_faces.push_back(combined_face);
			}
			else
			{

				//cut_result_face_ptr->AddToModel();
				cut_result_faces.push_back(cut_result_face_ptr);
			}
        }

        // 放大万分之一, 防止模型之间有缝，导致合并剖切面失败
        std::for_each(
            cut_result_faces.cbegin(), cut_result_faces.cend(),
            [](EditElementHandle *face)
            {
               _local::scale_element_in_place(*face, DVec3d::From(1, 1, 1.008));
                //_local::scale_element_in_place(*face, DVec3d::From(1, 1, 1.1));
            });

        // 合并所有剖切结果到一个面
        // 转换到desc
        auto cut_result_faces_desc = std::vector<MSElementDescrP>();
        std::transform(
            cut_result_faces.begin(), cut_result_faces.end(),
            std::back_inserter(cut_result_faces_desc),
            [](EditElementHandle *eeh)
            {
                return eeh->GetElementDescrP();
            });

        MSElementDescrP combined_face_desc = nullptr;
        PITCommonTool::CFaceTool::CombineFaces(combined_face_desc, cut_result_faces_desc, ACTIVEMODEL, false);

        if (combined_face_desc == nullptr)
        {
            mdlOutput_error(L"合并剖切面失败");
            return;
        }

        // mdlElmdscr_addByModelRef(combined_face_desc, ACTIVEMODEL);

        EditElementHandle combined_face(combined_face_desc, true, false, ACTIVEMODEL);

        //combined_face.AddToModel();

        // 钢筋层的参数
        GeneralFaceRebarAssembly::LayerRebarData layer_rebar_data;
        
        layer_rebar_data.layer = &layer;
        layer_rebar_data.level_name = TEXT_MAIN_REBAR;
        layer_rebar_data.array_mode = array_mode;
        layer_rebar_data.face = &combined_face;
        layer_rebar_data.side_cover = settings.side_cover;
        layer_rebar_data.wall_normal = wall_geometry_info.normal;
        layer_rebar_data.rebar_dir = rebar_dir;
        layer_rebar_data.collision_offset = collision_offset;

        assembly.generate_layer(layer_rebar_data);

        // 释放中间产生的面
        for (auto cut_result_face_ptr : cut_result_faces)
        {
            delete cut_result_face_ptr;
        }

        // 再次平移一下这个剖切面(一个半径)
        // 这两次循环中的偏移可以保证不同直径的钢筋之间不会穿模
        cut_face->GetHandler().ApplyTransform(*cut_face, TransformInfo(translation_transform));
    }
}

END_NAMESPACE_GALLERY_SINGLE_WALL