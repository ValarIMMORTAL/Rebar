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
    /// @brief ԭ������Ԫ��
    /// @details ԭ��ȡ����eeh�����ĵ�
    void scale_element_in_place(EditElementHandle &eeh, const DVec3d &scale_vec);

    /// @brief ��û�����id
    ElementId get_concrete_id(ElementHandleCR eh);

    /// @brief д���µĻ�����id
    void set_concrete_id(ElementHandleR eh, ElementId concrete_id);

    /// @brief �ڸ���λ�������ɸֽ���桢���桢�м䣩
    /// @param wall ǽ����
    /// @param assembly
    /// @param settings �����������ڶ�ȡ������Ȳ���
    /// @param wall_geometry_info ǽ�ļ�����Ϣ
    /// @param top_slab ����
    /// @param bottom_slab �װ�
    /// @param layers �ֽ�㣬������ֻ�����桢���桢�м�ͬһλ�õĸֽ
    /// @param spacing ͬһ��ֽ�֮��ļ�൥λUOR
    /// @param array_mode ���з�ʽ
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
            // ���˵�����ǽ��
            auto &elem = const_cast<EditElementHandleR>(static_cast<EditElementHandleCR>(entry));
            if(!WallHelper::is_wall(elem))
                continue;

            execute_make_rebar(elem);
            // ��ʱֻ����һ�����
            // ��Ϊѡ�еĿ����ж��ģ�ͣ�ǽ�������еģ�ֱ��ѡһ��ǽ�Ὣ����ǽ����ѡ���У�
            //break;
        }

        // ��������رնԻ���
        // ���ѡ�е�ѡģ�Ͱ�ť��ȥѡ��ǽ�� m_ClickedFlag������Ϊ1�����潫����ر�
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
    // ���ء�ѡ��ģ�͡���ť
    //dialog->GetDlgItem(IDC_SELECT_MODEL)->ShowWindow(SW_HIDE);
}

void _local::scale_element_in_place(EditElementHandle &eeh, const DVec3d &scale_vec)
{
    // ��������transform
    // ��Ҫ���ƶ���ԭ�㣬������
    auto midpoint = PITCommonTool::CElementTool::getCenterOfElmdescr(eeh.GetElementDescrP());
    midpoint.Negate();
    auto move_to_origin_transform = Transform::From(midpoint);
    // �ƶ���ԭ��
    eeh.GetHandler().ApplyTransform(eeh, TransformInfo(move_to_origin_transform));
    // �Ŵ�
    auto scale_matrix = RotMatrix::FromScaleFactors(scale_vec.x, scale_vec.y, scale_vec.z);
    auto scale_transform = Transform::From(scale_matrix);
    eeh.GetHandler().ApplyTransform(eeh, TransformInfo(scale_transform));
    // �ƶ�����
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
    /// ��ȡǽ�ϴ�������Ϣ
    GallerySettings gallery_settings;
    if (!GallerySettings::from_element(wall, gallery_settings))
    {
        mdlOutput_error(L"��ȡǽ��������ʧ��");
        return;
    }

    // ����ǽ�ļ��β���
    WallHelper::WallGeometryInfo wall_geometry_info;
    if (!WallHelper::analysis_geometry(wall, wall_geometry_info))
    {
        mdlOutput_error(L"���ǽ������Ϣʧ��");
        return;
    }

    /// ɨ���뵱ǰǽ������Ԫ��
    std::vector<WallHelper::Associated> associated;
    if (!WallHelper::analysis_associated(wall, associated, wall_geometry_info))
    {
        mdlOutput_error(L"ɨ����������ʧ��");
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
        mdlOutput_error(L"δ�ҵ�����͵װ�");
        return;
    }*/

    for (auto i = 0; i < gallery_settings.layer_count; ++i)
    {
        auto &layer = gallery_settings.layers[i];

        // ����ֽ��Ҫê��
        if (layer.direction != RebarDirection::Vertical)
        {
            continue;
        }

        // �����Ŀ�иֽ�ߴ��Ӧ����ز���
        auto bend_radius = layer.get_pin_radius();
        auto bend_length = layer.get_bend_length(layer.starting_end_type);
        auto diameter = layer.get_rebar_diameter();
        auto lae = layer.get_lae();
        // û�ҵ��óߴ�ֽ��Ӧ��lae����
        // ʹ��Ĭ��ֵ 
        lae = lae < 0 ? bend_length : lae;
		string tmp(layer.size_str);
		double La_d = stod(tmp);
		auto la_Inside = 15 * La_d * UOR_PER_MilliMeter;

        // ���Ϊ�׶ˣ��յ�Ϊ����
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

    if (top_slab_iter != associated.cend())//�ж���
    {
        EditElementHandle top_slab(top_slab_iter->element, true);
        DVec3d top_slab_dir;
        if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, top_slab, top_slab_dir))
        {
            mdlOutput_error(L"����ǽ�Ķ��巽��ʧ��");
            return;
        }
        auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
        WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, top_slab, top_wall_inside_outside_faces);
        int flag = 0;//��־��T��ǽʱ�ڶ��ε������ê�뷽�����һ�εķ�����Ҫ�෴
		int front_flag = 1;// ��־�����ǽ��ǰ��һ��ֽ���׶��ཻ��Ҫ����ǰ��һ��ê�뷽��
        for (auto i = 0; i < gallery_settings.layer_count; ++i)
        {
            auto &layer = gallery_settings.layers[i];

            // ����ֽ��Ҫê��
            if (layer.direction != RebarDirection::Vertical)
            {
                continue;
            }
            // �����Ŀ�иֽ�ߴ��Ӧ����ز���
            auto bend_radius = layer.get_pin_radius();
            auto bend_length = layer.get_bend_length(layer.starting_end_type);
            auto diameter = layer.get_rebar_diameter();
            auto lae = layer.get_lae();//ê��
            auto la0 = layer.get_la0();//���
			auto d = layer.size_str;
            // û�ҵ��óߴ�ֽ��Ӧ��lae����
            // ʹ��Ĭ��ֵ 
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

            // ���Ϊ�׶ˣ��յ�Ϊ����
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

			//////////�����׶�////////////////
			EditElementHandle Eleeh;
			std::vector<EditElementHandle*> Holeehs;
			EFT::GetSolidElementAndSolidHoles(top_slab, Eleeh, Holeehs);
			std::vector<EditElementHandle*> useHoleehs;
			CalculateMyUseHoles(useHoleehs, Holeehs, gallery_settings, top_slab.GetModelRef());
			bool is_touch = false;//ǽ�Ƿ��Ӵ����׶�
			bool is_front = false;//�Ƿ���ǰ��Ӵ����׶�
			for (auto hole_eeh : useHoleehs)
			{
				DPoint3d minP_hole;
				DPoint3d maxP_hole;
				//������п׶�Ԫ����������Ԫ�صķ�Χ��
				mdlElmdscr_computeRange(&minP_hole, &maxP_hole, hole_eeh->GetElementDescrP(), NULL);

				DPoint3d minP_wall;
				DPoint3d maxP_wall;
				//����ǽԪ����������Ԫ�صķ�Χ��
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
			//////////�����׶�////////////////

            // �������Ƿ�Ϊ�ڲ����øֽ�ê�볤��
            auto set_length =
                [&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
            {
                // �ڲ���ê�볤��ʹ��15d, dΪ�ֽ�ֱ��
                // �ģ��ڲ���Ϊê�̳��ȼ�lae
                if (WallHelper::is_inside_face(inside_outside_faces, face))
                {
                    //end.data.hook_degree.reserver_length = 15 * diameter;
                    end.data.hook_degree.reserver_length = la_Inside;
                    // �ڲ���ֽ����˻���Ҫ���������ֽ�ֱ�����ȣ�Ϊ�塢���ֽ�Ԥ���ռ�
                    end.offset = -diameter / UOR_PER_MilliMeter * 2;
                }
                // ���������ΪLae
                // �ģ������Ϊ��ӳ��ȼ�la0
                else if (WallHelper::is_outside_face(inside_outside_faces, face))
                {
                    //end.data.hook_degree.reserver_length = lae;
                    end.data.hook_degree.reserver_length = lae;
                }
                // ��������ΪĬ��
                else
                {
                    end.data.hook_degree.reserver_length = bend_length;
                }
            };

            // �����˸��ݶ�������
            set_length(top_wall_inside_outside_faces, face, layer.ending_end_type);

            // ����ê�����ת�Ƕ�
            // ������ķ�����巽��ļн�
            auto top_angle = top_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

            // ת�����ǶȲ����õ��˲���
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
            if ((flag) && (top_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()��ǽ���ΪT��ǽ
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

    if (bottom_slab_iter != associated.cend())//�еװ�
    {
        EditElementHandle bottom_slab(bottom_slab_iter->element, true);
        DVec3d bottom_slab_dir;
        if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, bottom_slab, bottom_slab_dir))
        {
            mdlOutput_error(L"����ǽ�ĵװ巽��ʧ��");
            return;
        }
        auto bottom_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
        WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, bottom_slab, bottom_wall_inside_outside_faces);
        int flag = 0;//��־��T��ǽʱ�ڶ��ε������ê�뷽�����һ�εķ�����Ҫ�෴
		int front_flag = 1; // ��־�����ǽ��ǰ��һ��ֽ���׶��ཻ��Ҫ����ǰ��һ��ê�뷽��
        for (auto i = 0; i < gallery_settings.layer_count; ++i)
        {
            auto &layer = gallery_settings.layers[i];

            // ����ֽ��Ҫê��
            if (layer.direction != RebarDirection::Vertical)
            {
                continue;
            }

            // �����Ŀ�иֽ�ߴ��Ӧ����ز���
            auto bend_radius = layer.get_pin_radius();
            auto bend_length = layer.get_bend_length(layer.starting_end_type);
            auto diameter = layer.get_rebar_diameter();
            auto lae = layer.get_lae();//ê�̳���
            auto la0 = layer.get_la0();//��ӳ���
            // û�ҵ��óߴ�ֽ��Ӧ��lae����
            // ʹ��Ĭ��ֵ 
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

            // ���Ϊ�׶ˣ��յ�Ϊ����
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

			//////////�����׶�////////////////
			EditElementHandle Eleeh;
			std::vector<EditElementHandle*> Holeehs;
			EFT::GetSolidElementAndSolidHoles(bottom_slab, Eleeh, Holeehs);
			std::vector<EditElementHandle*> useHoleehs;
			CalculateMyUseHoles(useHoleehs, Holeehs, gallery_settings, bottom_slab.GetModelRef());
			bool is_touch = false;//ǽ�Ƿ��Ӵ����׶�
			bool is_front = false;//�Ƿ���ǰ��Ӵ����׶�
			for (auto hole_eeh : useHoleehs)
			{
				DPoint3d minP_hole;
				DPoint3d maxP_hole;
				//������п׶�Ԫ����������Ԫ�صķ�Χ��
				mdlElmdscr_computeRange(&minP_hole, &maxP_hole, hole_eeh->GetElementDescrP(), NULL);

				DPoint3d minP_wall;
				DPoint3d maxP_wall;
				//����ǽԪ����������Ԫ�صķ�Χ��
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
			//////////�����׶�////////////////

            // �������Ƿ�Ϊ�ڲ����øֽ�ê�볤��
            auto set_length =
                [&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
            {
                // �ڲ���ê�볤��ʹ��15d, dΪ�ֽ�ֱ��
                // �ģ��ڲ���Ϊê�̳���lae
                if (WallHelper::is_inside_face(inside_outside_faces, face))
                {
                    //end.data.hook_degree.reserver_length = 15 * diameter;
                    end.data.hook_degree.reserver_length = la_Inside;
                    // �ڲ���ֽ����˻���Ҫ���������ֽ�ֱ�����ȣ�Ϊ�塢���ֽ�Ԥ���ռ�
                    end.offset = -diameter / UOR_PER_MilliMeter * 2;
                }
                // ���������ΪLae
                // �ģ������Ϊ��ӳ���la0
                else if (WallHelper::is_outside_face(inside_outside_faces, face))
                {
                    //end.data.hook_degree.reserver_length = lae;
                    end.data.hook_degree.reserver_length = lae;
                }
                // ��������ΪĬ��
                else
                {
                    end.data.hook_degree.reserver_length = bend_length;
                }
            };

            // ��ʼ�˸��ݵ�������
            set_length(bottom_wall_inside_outside_faces, face, layer.starting_end_type);

            // ����ê�����ת�Ƕ�
            // ������ķ�����巽��ļн�
            auto bottom_angle = bottom_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

            // ת�����ǶȲ����õ��˲���
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
            if ((flag) && (bottom_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()��ǽ���ΪT��ǽ
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
        mdlOutput_error(L"δ�ҵ������װ�");
        return;
    }

    EditElementHandle top_slab(top_slab_iter->element, true);
    EditElementHandle bottom_slab(bottom_slab_iter->element, true);

    // ���㶥����װ�ĳ���
    DVec3d top_slab_dir;
    if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, top_slab, top_slab_dir))
    {
        mdlOutput_error(L"����ǽ�Ķ��巽��ʧ��");
        return;
    }
    DVec3d bottom_slab_dir;
    if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, bottom_slab, bottom_slab_dir))
    {
        mdlOutput_error(L"����ǽ�ĵװ巽��ʧ��");
        return;
    }

    // �����Զ��塢�װ���ڲ���������
    auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
    auto bottom_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
    WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, top_slab, top_wall_inside_outside_faces);
    WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, bottom_slab, bottom_wall_inside_outside_faces);

    // �����ڲ���������ê��
    // ����������һ���ģ�����ֻ��Ҫ���������ê�룬���Կ�����ǰ�����
    int flag = 0;//��־��T��ǽʱ�ڶ��ε������ê�뷽�����һ�εķ�����Ҫ�෴
    for (auto i = 0; i < gallery_settings.layer_count; ++i)
    {
        auto &layer = gallery_settings.layers[i];

        // ����ֽ��Ҫê��
        if (layer.direction != RebarDirection::Vertical)
        {
            continue;
        }


        // �����Ŀ�иֽ�ߴ��Ӧ����ز���
        auto bend_radius = layer.get_pin_radius();
        auto bend_length = layer.get_bend_length(layer.starting_end_type);
        auto diameter = layer.get_rebar_diameter();
        auto lae = layer.get_lae();
        // û�ҵ��óߴ�ֽ��Ӧ��lae����
        // ʹ��Ĭ��ֵ 
        lae = lae < 0 ? bend_length : lae;

        // ���Ϊ�׶ˣ��յ�Ϊ����
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

        // �������Ƿ�Ϊ�ڲ����øֽ�ê�볤��
        auto set_length = 
            [&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
            {
                // �ڲ���ê�볤��ʹ��15d, dΪ�ֽ�ֱ��
                if (WallHelper::is_inside_face(inside_outside_faces, face))
                {
                    end.data.hook_degree.reserver_length = 15 * diameter;
                    // �ڲ���ֽ����˻���Ҫ���������ֽ�ֱ�����ȣ�Ϊ�塢���ֽ�Ԥ���ռ�
                    end.offset = -diameter / UOR_PER_MilliMeter * 2;
                }
                // ���������ΪLae
                else if(WallHelper::is_outside_face(inside_outside_faces, face))
                {
                    end.data.hook_degree.reserver_length = lae;
                }
                // ��������ΪĬ��
                else
                {
                    end.data.hook_degree.reserver_length = bend_length;
                }
            };

        // ��ʼ�˸��ݵ�������
        set_length(bottom_wall_inside_outside_faces, face, layer.starting_end_type);
        // �����˸��ݶ�������
        set_length(top_wall_inside_outside_faces, face, layer.ending_end_type);

        // ����ê�����ת�Ƕ�
        // ������ķ�����巽��ļн�
        auto top_angle = top_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));
        auto bottom_angle = bottom_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

        // ת�����ǶȲ����õ��˲���
        layer.starting_end_type.angle = bottom_angle / PI * 180;
        layer.ending_end_type.angle = top_angle / PI * 180;
        if ((flag) && (top_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()��ǽ���ΪT��ǽ
        {
            layer.starting_end_type.angle = 180;
            layer.ending_end_type.angle = 180;
        }
        flag++;
    }
    */

    // �Ըֽλ�÷���
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
        // ������
        mdlOutput_error(L"����assemblyʧ��");
        return;
    }

    // rebar_assembly->remove_rebar();
    rebar_assembly->begin_generate();
    rebar_assembly->on_double_click(
        [](GeneralFaceRebarAssembly &assembly) -> bool
        {
            // ��øֽ����ڵ�ǽ
            auto wall_id = assembly.GetSelectedElement();
            auto wall_model = assembly.GetSelectedModel();
            auto wall = ElementHandle(wall_id, wall_model);
            // ���֮ǰ��ѡ��
            auto &manager = SelectionSetManager::GetManager();
            manager.EmptyAll();
            // ��ǽ����ѡ����
            manager.AddElement(wall.GetElementRef(), wall.GetModelRef());

            // �ٴδ�����
            // �ô�����ȡѡ����ѡ�е�ǽ�д洢�Ĳ���
            cmd(L"");

            return true;
        });

    // ����ͬ�ĸֽ�λ�����ɸֽ�
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

    // ����µĻ�����id
    // auto concrete_id = wall_rebar_assembly->FetchConcrete();
    concrete_id = rebar_assembly->FetchConcrete();
    EditElementHandle concrete(concrete_id, ACTIVEMODEL);

    // ����Ϊ���ɼ�
    auto old_concrete_ref = concrete.GetElementRef();
    mdlElmdscr_setVisible(concrete.GetElementDescrP(), false);
    concrete.ReplaceInModel(old_concrete_ref);

    // д���µĻ�����id
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


/// ����õ����������Ŀ׶����� useHoleehs
/// ���׶���С���ڸֽ���ʱ����������
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
	if (1)//������Ҫ����Ŀ׶�
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
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
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
	/// ��ȡǽ�ϴ�������Ϣ
	//GallerySettings gallery_settings;
	//if (!GallerySettings::from_element(wall, gallery_settings))
	//{
	//	mdlOutput_error(L"��ȡǽ��������ʧ��");
	//	return;
	//}

	// ����ǽ�ļ��β���
	WallHelper::WallGeometryInfo wall_geometry_info;
	if (!WallHelper::analysis_geometry(wall, wall_geometry_info))
	{
		mdlOutput_error(L"���ǽ������Ϣʧ��");
		return;
	}

	/// ɨ���뵱ǰǽ������Ԫ��
	std::vector<WallHelper::Associated> associated;
	if (!WallHelper::analysis_associated(wall, associated, wall_geometry_info))
	{
		mdlOutput_error(L"ɨ����������ʧ��");
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
		mdlOutput_error(L"δ�ҵ�����͵װ�");
		return;
	}*/

	for (auto i = 0; i < gallery_settings.layer_count; ++i)
	{
		auto &layer = gallery_settings.layers[i];

		// ����ֽ��Ҫê��
		if (layer.direction != RebarDirection::Vertical)
		{
			continue;
		}

		// �����Ŀ�иֽ�ߴ��Ӧ����ز���
		auto bend_radius = layer.get_pin_radius();
		auto bend_length = layer.get_bend_length(layer.starting_end_type);
		auto diameter = layer.get_rebar_diameter();
		auto lae = layer.get_lae();
		// û�ҵ��óߴ�ֽ��Ӧ��lae����
		// ʹ��Ĭ��ֵ 
		lae = lae < 0 ? bend_length : lae;

		// ���Ϊ�׶ˣ��յ�Ϊ����
		layer.starting_end_type.type = EndType::None;
		layer.ending_end_type.type = EndType::None;
		layer.starting_end_type.angle = 0;
		layer.ending_end_type.angle = 0;
	}

	if (top_slab_iter != associated.cend())//�ж���
	{
		EditElementHandle top_slab(top_slab_iter->element, true);
		DVec3d top_slab_dir;
		if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, top_slab, top_slab_dir))
		{
			mdlOutput_error(L"����ǽ�Ķ��巽��ʧ��");
			return;
		}
		auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
		WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, top_slab, top_wall_inside_outside_faces);
		int flag = 0;//��־��T��ǽʱ�ڶ��ε������ê�뷽�����һ�εķ�����Ҫ�෴
		for (auto i = 0; i < gallery_settings.layer_count; ++i)
		{
			auto &layer = gallery_settings.layers[i];

			// ����ֽ��Ҫê��
			if (layer.direction != RebarDirection::Vertical)
			{
				continue;
			}
			// �����Ŀ�иֽ�ߴ��Ӧ����ز���
			auto bend_radius = layer.get_pin_radius();
			auto bend_length = layer.get_bend_length(layer.starting_end_type);
			auto diameter = layer.get_rebar_diameter();
			auto lae = layer.get_lae();//ê��
			auto la0 = layer.get_la0();//���
			auto d = layer.size_str;
			// û�ҵ��óߴ�ֽ��Ӧ��lae����
			// ʹ��Ĭ��ֵ 
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

			// ���Ϊ�׶ˣ��յ�Ϊ����
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

			// �������Ƿ�Ϊ�ڲ����øֽ�ê�볤��
			auto set_length =
				[&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
			{
				// �ڲ���ê�볤��ʹ��15d, dΪ�ֽ�ֱ��
				// �ģ��ڲ���Ϊê�̳��ȼ�lae
				if (WallHelper::is_inside_face(inside_outside_faces, face))
				{
					//end.data.hook_degree.reserver_length = 15 * diameter;
					end.data.hook_degree.reserver_length = lae;
					// �ڲ���ֽ����˻���Ҫ���������ֽ�ֱ�����ȣ�Ϊ�塢���ֽ�Ԥ���ռ�
					end.offset = -diameter / UOR_PER_MilliMeter * 2;
				}
				// ���������ΪLae
				// �ģ������Ϊ��ӳ��ȼ�la0
				else if (WallHelper::is_outside_face(inside_outside_faces, face))
				{
					//end.data.hook_degree.reserver_length = lae;
					end.data.hook_degree.reserver_length = la0;
				}
				// ��������ΪĬ��
				else
				{
					end.data.hook_degree.reserver_length = bend_length;
				}
			};

			// �����˸��ݶ�������
			set_length(top_wall_inside_outside_faces, face, layer.ending_end_type);

			// ����ê�����ת�Ƕ�
			// ������ķ�����巽��ļн�
			auto top_angle = top_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

			// ת�����ǶȲ����õ��˲���
			layer.ending_end_type.angle = top_angle / PI * 180;
			if ((flag) && (top_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()��ǽ���ΪT��ǽ
			{
				layer.ending_end_type.angle = 180;
			}
			flag++;
		}

	}

	if (bottom_slab_iter != associated.cend())//�еװ�
	{
		EditElementHandle bottom_slab(bottom_slab_iter->element, true);
		DVec3d bottom_slab_dir;
		if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, bottom_slab, bottom_slab_dir))
		{
			mdlOutput_error(L"����ǽ�ĵװ巽��ʧ��");
			return;
		}
		auto bottom_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
		WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, bottom_slab, bottom_wall_inside_outside_faces);
		int flag = 0;//��־��T��ǽʱ�ڶ��ε������ê�뷽�����һ�εķ�����Ҫ�෴
		for (auto i = 0; i < gallery_settings.layer_count; ++i)
		{
			auto &layer = gallery_settings.layers[i];

			// ����ֽ��Ҫê��
			if (layer.direction != RebarDirection::Vertical)
			{
				continue;
			}

			// �����Ŀ�иֽ�ߴ��Ӧ����ز���
			auto bend_radius = layer.get_pin_radius();
			auto bend_length = layer.get_bend_length(layer.starting_end_type);
			auto diameter = layer.get_rebar_diameter();
			auto lae = layer.get_lae();//ê�̳���
			auto la0 = layer.get_la0();//��ӳ���
			// û�ҵ��óߴ�ֽ��Ӧ��lae����
			// ʹ��Ĭ��ֵ 
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

			// ���Ϊ�׶ˣ��յ�Ϊ����
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

			// �������Ƿ�Ϊ�ڲ����øֽ�ê�볤��
			auto set_length =
				[&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
			{
				// �ڲ���ê�볤��ʹ��15d, dΪ�ֽ�ֱ��
				// �ģ��ڲ���Ϊê�̳���lae
				if (WallHelper::is_inside_face(inside_outside_faces, face))
				{
					//end.data.hook_degree.reserver_length = 15 * diameter;
					end.data.hook_degree.reserver_length = lae;
					// �ڲ���ֽ����˻���Ҫ���������ֽ�ֱ�����ȣ�Ϊ�塢���ֽ�Ԥ���ռ�
					end.offset = -diameter / UOR_PER_MilliMeter * 2;
				}
				// ���������ΪLae
				// �ģ������Ϊ��ӳ���la0
				else if (WallHelper::is_outside_face(inside_outside_faces, face))
				{
					//end.data.hook_degree.reserver_length = lae;
					end.data.hook_degree.reserver_length = la0;
				}
				// ��������ΪĬ��
				else
				{
					end.data.hook_degree.reserver_length = bend_length;
				}
			};

			// ��ʼ�˸��ݵ�������
			set_length(bottom_wall_inside_outside_faces, face, layer.starting_end_type);

			// ����ê�����ת�Ƕ�
			// ������ķ�����巽��ļн�
			auto bottom_angle = bottom_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

			// ת�����ǶȲ����õ��˲���
			layer.starting_end_type.angle = bottom_angle / PI * 180;
			if ((flag) && (bottom_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()��ǽ���ΪT��ǽ
			{
				layer.starting_end_type.angle = 180;
			}
			flag++;
		}
	}
	/*
	if (top_slab_iter == associated.cend() || bottom_slab_iter == associated.cend())
	{
		mdlOutput_error(L"δ�ҵ������װ�");
		return;
	}

	EditElementHandle top_slab(top_slab_iter->element, true);
	EditElementHandle bottom_slab(bottom_slab_iter->element, true);

	// ���㶥����װ�ĳ���
	DVec3d top_slab_dir;
	if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, top_slab, top_slab_dir))
	{
		mdlOutput_error(L"����ǽ�Ķ��巽��ʧ��");
		return;
	}
	DVec3d bottom_slab_dir;
	if (!WallHelper::analysis_slab_position_with_wall(wall, wall_geometry_info.normal, bottom_slab, bottom_slab_dir))
	{
		mdlOutput_error(L"����ǽ�ĵװ巽��ʧ��");
		return;
	}

	// �����Զ��塢�װ���ڲ���������
	auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
	auto bottom_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
	WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, top_slab, top_wall_inside_outside_faces);
	WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, bottom_slab, bottom_wall_inside_outside_faces);

	// �����ڲ���������ê��
	// ����������һ���ģ�����ֻ��Ҫ���������ê�룬���Կ�����ǰ�����
	int flag = 0;//��־��T��ǽʱ�ڶ��ε������ê�뷽�����һ�εķ�����Ҫ�෴
	for (auto i = 0; i < gallery_settings.layer_count; ++i)
	{
		auto &layer = gallery_settings.layers[i];

		// ����ֽ��Ҫê��
		if (layer.direction != RebarDirection::Vertical)
		{
			continue;
		}


		// �����Ŀ�иֽ�ߴ��Ӧ����ز���
		auto bend_radius = layer.get_pin_radius();
		auto bend_length = layer.get_bend_length(layer.starting_end_type);
		auto diameter = layer.get_rebar_diameter();
		auto lae = layer.get_lae();
		// û�ҵ��óߴ�ֽ��Ӧ��lae����
		// ʹ��Ĭ��ֵ
		lae = lae < 0 ? bend_length : lae;

		// ���Ϊ�׶ˣ��յ�Ϊ����
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

		// �������Ƿ�Ϊ�ڲ����øֽ�ê�볤��
		auto set_length =
			[&](const WallHelper::WallInsideOutsideFaces &inside_outside_faces, EditElementHandle *face, End &end)
			{
				// �ڲ���ê�볤��ʹ��15d, dΪ�ֽ�ֱ��
				if (WallHelper::is_inside_face(inside_outside_faces, face))
				{
					end.data.hook_degree.reserver_length = 15 * diameter;
					// �ڲ���ֽ����˻���Ҫ���������ֽ�ֱ�����ȣ�Ϊ�塢���ֽ�Ԥ���ռ�
					end.offset = -diameter / UOR_PER_MilliMeter * 2;
				}
				// ���������ΪLae
				else if(WallHelper::is_outside_face(inside_outside_faces, face))
				{
					end.data.hook_degree.reserver_length = lae;
				}
				// ��������ΪĬ��
				else
				{
					end.data.hook_degree.reserver_length = bend_length;
				}
			};

		// ��ʼ�˸��ݵ�������
		set_length(bottom_wall_inside_outside_faces, face, layer.starting_end_type);
		// �����˸��ݶ�������
		set_length(top_wall_inside_outside_faces, face, layer.ending_end_type);

		// ����ê�����ת�Ƕ�
		// ������ķ�����巽��ļн�
		auto top_angle = top_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));
		auto bottom_angle = bottom_slab_dir.AngleTo(DVec3d::From(wall_geometry_info.normal));

		// ת�����ǶȲ����õ��˲���
		layer.starting_end_type.angle = bottom_angle / PI * 180;
		layer.ending_end_type.angle = top_angle / PI * 180;
		if ((flag) && (top_slab_dir == DVec3d::FromZero()))//top_slab_dir == DVec3d::FromZero()��ǽ���ΪT��ǽ
		{
			layer.starting_end_type.angle = 180;
			layer.ending_end_type.angle = 180;
		}
		flag++;
	}
	*/

	// �Ըֽλ�÷���
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
		// ������
		mdlOutput_error(L"����assemblyʧ��");
		return;
	}

	// rebar_assembly->remove_rebar();
	rebar_assembly->begin_generate();
	rebar_assembly->on_double_click(
		[](GeneralFaceRebarAssembly &assembly) -> bool
		{
			// ��øֽ����ڵ�ǽ
			auto wall_id = assembly.GetSelectedElement();
			auto wall_model = assembly.GetSelectedModel();
			auto wall = ElementHandle(wall_id, wall_model);
			// ���֮ǰ��ѡ��
			auto &manager = SelectionSetManager::GetManager();
			manager.EmptyAll();
			// ��ǽ����ѡ����
			manager.AddElement(wall.GetElementRef(), wall.GetModelRef());

			// �ٴδ�����
			// �ô�����ȡѡ����ѡ�е�ǽ�д洢�Ĳ���
			cmd(L"");

			return true;
		});

	// ����ͬ�ĸֽ�λ�����ɸֽ�
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

	// ����µĻ�����id
	// auto concrete_id = wall_rebar_assembly->FetchConcrete();
	concrete_id = rebar_assembly->FetchConcrete();
	EditElementHandle concrete(concrete_id, ACTIVEMODEL);

	// ����Ϊ���ɼ�
	auto old_concrete_ref = concrete.GetElementRef();
	mdlElmdscr_setVisible(concrete.GetElementDescrP(), false);
	concrete.ReplaceInModel(old_concrete_ref);

	// д���µĻ�����id
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
    // ���Ʋ���������������
    auto layers = _layers;
    std::sort(
        layers.begin(), layers.end(),
        [](const GallerySettings::Layer &layer_a, const GallerySettings::Layer &layer_b) -> bool
        {
            return layer_a.layer_index < layer_b.layer_index;
        });

    // ȡ�����еĶ��塢�װ�
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

    // ����layers��һ��Ԫ���жϵ�ǰ�Ǵ��ĸ��濪ʼ���
    // ÿ���������󶼻�ƫ����, ��Ϊ��ͬ��ĸֽ���Ҫ��, ������ƫ�Ƶķ���
    DVec3d face_offset_dir;
    // ������
    std::unique_ptr<EditElementHandle> cut_face;
    // ѡ�е���Ҫ�����棬��������ж����ڲ໹�������
    // �м��Ļ���Ϊnullptr
    EditElementHandle *face = nullptr;
    // ������ƫ������ ��λUOR
    double cover_offset = 0;
    MSElementDescrP desc_p = nullptr;
    switch (layers.front().position)
    {
    case RebarPosition::Front:
        // ����Ļ����ط�����෴�������ɲ�ͬ��ĸֽ�
        face_offset_dir = wall_geometry_info.normal;
        face_offset_dir.Negate();
        cover_offset = settings.front_cover * UOR_PER_MilliMeter;
        face = wall_geometry_info.front_face;
        // ����һ����
        face->GetElementDescrCP()->Duplicate(&desc_p);
        cut_face.reset(new EditElementHandle(desc_p, true, false, ACTIVEMODEL));
        break;
    case RebarPosition::Back:
        face_offset_dir = wall_geometry_info.normal;
        face = wall_geometry_info.back_face;
        cover_offset = settings.back_cover * UOR_PER_MilliMeter;
        // ����һ����
        face->GetElementDescrCP()->Duplicate(&desc_p);
        cut_face.reset(new EditElementHandle(desc_p, true, false, ACTIVEMODEL));
        break;
    case RebarPosition::Middle:
        // �м��������ֽ���ͬ
        // �м����Ҫ�����������ڲ����������
        // face_offset_dir = -wall_geometry_info.normal;
        break;
    default:
        break;
    }

    // ������ǰ��Ͷ��塢�װ��λ�ù�ϵ(����)
    WallHelper::WallInsideOutsideFaces top_inside_outside_faces;
    WallHelper::WallInsideOutsideFaces bottom_inside_outside_faces;
    if (top_slabs.size() > 0)
    {
        if (!WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, *top_slabs.front(), top_inside_outside_faces))
        {
            mdlOutput_error(L"����ǽ������λ�ù�ϵʧ��");
            return;
        }
    }
    if (bottom_slabs.size() > 0)
    {
        if (!WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, *bottom_slabs.front(), bottom_inside_outside_faces))
        {
            mdlOutput_error(L"����ǽ������λ�ù�ϵʧ��");
            return;
        }
    }
    // ������ǰ��Ͷ��塢�װ��λ�ù�ϵ(����)
    //WallHelper::WallInsideOutsideFaces top_inside_outside_faces;
    //WallHelper::WallInsideOutsideFaces bottom_inside_outside_faces;
    //// ���ȡһ����
    //if (!WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, *top_slabs.front(), top_inside_outside_faces))
    //{
    //    mdlOutput_error(L"����ǽ������λ�ù�ϵʧ��");
    //    return;
    //}
    //if (!WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, *bottom_slabs.front(), bottom_inside_outside_faces))
    //{
    //    mdlOutput_error(L"����ǽ������λ�ù�ϵʧ��");
    //    return;
    //}

    // ˮƽ�ֽ�����ǽ�ķ���
    auto horizontal_rebar_dir = wall_geometry_info.direction;

    // ����������z������
    // ��z�����ϷŴ�10��
    scale_element_in_place(*cut_face, DVec3d::From(1, 1, 10));

    // ����һ�������ƶ�������ı任
    // �Ƚ����������ƶ�һ�����������
    auto translation_transform = Transform::From(0, 0, 0);
    auto cover_translation = face_offset_dir * cover_offset;
    translation_transform.SetTranslation(cover_translation);
    cut_face->GetHandler().ApplyTransform(*cut_face, TransformInfo(translation_transform));
	//cut_face->AddToModel();
    // ���ɸ����ÿһ��ֽ�
    for(const auto &layer : layers)
    {
        // ��ȡ�ò�ֽ��ֱ��
        auto diameter = layer.get_rebar_diameter();

        // ƽ��һ���ֽ�뾶
        auto layer_offset = face_offset_dir * diameter / 2.0;
        translation_transform.SetTranslation(layer_offset);

        // ƽ��һ�����������
        cut_face->GetHandler().ApplyTransform(*cut_face, TransformInfo(translation_transform));

        // �����������ֽ�ĸֽ��
        auto rebar_dir =
            layer.direction == RebarDirection::Horizontal ? horizontal_rebar_dir : DVec3d::UnitZ();

        // ������Ҫ��һ���ֽ�ľ����ֹ����еĸֽ���ײ
        auto collision_offset =
            layer.direction == RebarDirection::Horizontal ? 0.0 : diameter;

        // ������Ҫ���еĹ���
        std::vector<const EditElementHandle *> to_be_cut_elements;
        // ǽ�ǿ϶�Ҫ�������е�
        to_be_cut_elements.push_back(&wall);
        if (layer.direction == RebarDirection::Vertical)
        {
            // ��ֱ�ֽ���Ҫê�뵽�壬���Զ��塢�װ嶼��Ҫ��������
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
            // ������Ҫ����������ж��Ƿ���Ҫ�ڰ������
            // �����Ҫ�������
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

        // �������н��
        std::vector<EditElementHandle *> cut_result_faces;
        for (const auto element_ptr : to_be_cut_elements)
        {
            const auto &element = *element_ptr;
            auto cut_result_face_ptr = new EditElementHandle();
            if (!PITCommonTool::CSolidTool::SolidBoolWithFace(*cut_result_face_ptr, *cut_face, element, BOOLOPERATION::BOOLOPERATION_INTERSECT))
            {
                mdlOutput_error(L"���������ʧ��");
                return;
            }
		
			ElementAgenda agenda;//��Ŵ�ɢ֮���Ԫ��
			DropGeometryPtr pDropGeometry = DropGeometry::Create();//����һ��DropGeometryʵ�������ü���ѡ��
			int type = cut_result_face_ptr->GetElementType();
        	if (mdlSolid_isSmartSheetElement(cut_result_face_ptr->GetElementDescrP(), cut_result_face_ptr->GetDgnModelP()))//���������suface�������Ҫ��ɢ�棬Ȼ���ٺϲ���һ���µ���
			{
				pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Solids);
				pDropGeometry->SetSolidsOptions(DropGeometry::SOLID_Surfaces);
				if (SUCCESS != cut_result_face_ptr->GetDisplayHandler()->Drop(*cut_result_face_ptr, agenda, *pDropGeometry))
				{
					agenda.Clear();
					mdlOutput_printf(MSG_STATUS, L"��ɢ��������ʧ�ܣ��޷���ȡ��!");
					continue;
				}
				// �����������
				std::vector<EditElementHandle *> faces;
				for(int i=0;i<agenda.size();i++)
				{
					EditElementHandle* eehtemp=new EditElementHandle(agenda[i], agenda[i].GetModelRef());
					faces.push_back(eehtemp);
				}
				
				// �Ŵ����֮һ, ��ֹģ��֮���з죬���ºϲ�������ʧ��
				std::for_each(
					faces.cbegin(), faces.cend(),
					[](EditElementHandle *face)
					{
						_local::scale_element_in_place(*face, DVec3d::From(1, 1, 1));
					});

				// �ϲ��������н����һ����
				// ת����desc
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

        // �Ŵ����֮һ, ��ֹģ��֮���з죬���ºϲ�������ʧ��
        std::for_each(
            cut_result_faces.cbegin(), cut_result_faces.cend(),
            [](EditElementHandle *face)
            {
               _local::scale_element_in_place(*face, DVec3d::From(1, 1, 1.008));
                //_local::scale_element_in_place(*face, DVec3d::From(1, 1, 1.1));
            });

        // �ϲ��������н����һ����
        // ת����desc
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
            mdlOutput_error(L"�ϲ�������ʧ��");
            return;
        }

        // mdlElmdscr_addByModelRef(combined_face_desc, ACTIVEMODEL);

        EditElementHandle combined_face(combined_face_desc, true, false, ACTIVEMODEL);

        //combined_face.AddToModel();

        // �ֽ��Ĳ���
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

        // �ͷ��м��������
        for (auto cut_result_face_ptr : cut_result_faces)
        {
            delete cut_result_face_ptr;
        }

        // �ٴ�ƽ��һ�����������(һ���뾶)
        // ������ѭ���е�ƫ�ƿ��Ա�֤��ֱͬ���ĸֽ�֮�䲻�ᴩģ
        cut_face->GetHandler().ApplyTransform(*cut_face, TransformInfo(translation_transform));
    }
}

END_NAMESPACE_GALLERY_SINGLE_WALL