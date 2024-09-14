#include "_ustation.h"
#include "GallerySettings.h"
#include "CSysTool.h"
#include "GalleryIntelligentRebarids.h"
#include "GalleryIntelligentRebar.h"
#include "XmlHelper.h"

using namespace PITCommonTool;

namespace Gallery
{
    std::string GallerySettings::Layer::get_full_size_key() const
    {
		return std::string(this->size_str) + std::string(1, (char)('A' + (char)this->type));
    }

    double GallerySettings::Layer::get_rebar_diameter(DgnModelRefP model_ref) const
    {
		if(model_ref == nullptr)
		{
			model_ref = ACTIVEMODEL;
		}

		//拼接size_key和钢筋等级
		auto full_key = this->get_full_size_key();

		return RebarCode::GetBarDiameter(full_key.c_str(), model_ref);
	}

    double GallerySettings::Layer::get_pin_radius(DgnModelRefP model_ref) const
    {
		if(model_ref == nullptr)
		{
			model_ref = ACTIVEMODEL;
		}

		auto full_size_key = this->get_full_size_key();
		return RebarCode::GetPinRadius(full_size_key.c_str(), model_ref, false);
    }

    double GallerySettings::Layer::get_bend_length(const End &end, DgnModelRefP model_ref) const
    {
		if(model_ref == nullptr)
		{
			model_ref = ACTIVEMODEL;
		}

		auto full_size_key = this->get_full_size_key();
		// 暂时只读取kBend的长度
        auto rebar_end_type = RebarEndType();
        rebar_end_type.SetType(RebarEndType::kBend); // 90度弯锚
		return RebarCode::GetBendLength(full_size_key.c_str(), rebar_end_type, model_ref);
	}

    double GallerySettings::Layer::get_lae() const
    {
		//auto full_key = this->get_full_size_key();
		//// Lae参数从全局XML参数中获得
		//auto iter = g_globalpara.m_alength.find(full_key);
		//if(iter == g_globalpara.m_alength.end())
		//{
		//	return -1.0;
		//}
		//else
		//{
		//	return iter->second * UOR_PER_MilliMeter;
		//}
		// Lae参数从全局XML参数中获得
		if (g_globalpara.m_alength.find("A") != g_globalpara.m_alength.end())
		{
			auto iter = g_globalpara.m_alength.find("A");
			return iter->second * UOR_PER_MilliMeter;
		}
		else if (g_globalpara.m_alength.find("B") != g_globalpara.m_alength.end())
		{
			auto iter = g_globalpara.m_alength.find("B");
			return iter->second * UOR_PER_MilliMeter;
		}
		else if (g_globalpara.m_alength.find("C") != g_globalpara.m_alength.end())
		{
			auto iter = g_globalpara.m_alength.find("C");
			return iter->second * UOR_PER_MilliMeter;
		}
		else if (g_globalpara.m_alength.find("D") != g_globalpara.m_alength.end())
		{
			auto iter = g_globalpara.m_alength.find("D");
			return iter->second * UOR_PER_MilliMeter;
		}
		else
		{
			return -1.0;
		}

	}

	double GallerySettings::Layer::get_la0() const
	{
		//auto full_key = this->get_full_size_key();
		//// Lae参数从全局XML参数中获得
		//auto iter = g_globalpara.m_laplenth.find(full_key);
		//if (iter == g_globalpara.m_laplenth.end())
		//{
		//	return -1.0;
		//}
		//else
		//{
		//	return iter->second * UOR_PER_MilliMeter;
		//}
		// Lae参数从全局XML参数中获得
		if (g_globalpara.m_laplenth.find("A") != g_globalpara.m_laplenth.end())
		{
			auto iter = g_globalpara.m_laplenth.find("A");
			return iter->second * UOR_PER_MilliMeter;
		}
		else if (g_globalpara.m_laplenth.find("C") != g_globalpara.m_laplenth.end())
		{
			auto iter = g_globalpara.m_laplenth.find("C");
			return iter->second * UOR_PER_MilliMeter;
		}
		else if (g_globalpara.m_laplenth.find("D") != g_globalpara.m_laplenth.end())
		{
			auto iter = g_globalpara.m_laplenth.find("D");
			return iter->second * UOR_PER_MilliMeter;
		}
		else
		{
			return -1.0;
		}
	}

    GallerySettings GallerySettings::from_default()
	{
		GallerySettings settings{
			50,	   // front cover
			50,	   // back cover
			50,	   // side cover
			4,	   // layer count
			true, // handle hole
			0,   // hole size,
			// layers, init later
			{}};

		/// 生成4层钢筋
		auto mid_pos = settings.layer_count / 2;
		for (auto i = 0; i < settings.layer_count; ++i)
		{
			auto &layer = settings.layers[i];
			if (i < mid_pos) // 前半部分
			{
				layer.layer_index = i + 1;
				layer.direction = static_cast<RebarDirection>((i+1) & 0x01);
				layer.position = RebarPosition::Front;

			}
			else // 后半部分
			{
				layer.layer_index = settings.layer_count - i;
				layer.direction = static_cast<RebarDirection>((i) & 0x01);
				layer.position = RebarPosition::Back;
			}

			// layer.size_str = "12";
			strcpy(layer.size_str, "12");
			layer.type = 2;
			layer.spacing = 150;
			/*layer.start_offset = 0;
			layer.end_offset = 0;
			layer.layer_space = 0;*/
			//layer.rebarColor = -1;//默认是-1，因为钢筋颜色会随着钢筋直径等变化，
								  //当自己选择为其他值的时候,钢筋颜色为自己表格设置的值
			layer.rebarLineStyle = 0;
			layer.rebarWeight = 0;
			layer.starting_end_type.angle = 0;
			layer.starting_end_type.offset = 0;
			layer.starting_end_type.type = EndType::None;
			layer.ending_end_type.angle = 0;
			layer.ending_end_type.offset = 0;
			layer.ending_end_type.type = EndType::None;
		}

		return settings;
	}

	bool GallerySettings::from_project(GallerySettings &settings)
	{
		return CSysTool::loadPathResultDlgParams(
				   RTYPE_GALLERY_SETTINGS,
				   RSCID_GALLERY_SETTINGS,
				   g_taskID,
				   settings) == SUCCESS;
	}

	bool GallerySettings::from_element(ElementId id, GallerySettings &settings, DgnModelRefP model)
	{
		return GetElementXAttribute(
				   id,
				   sizeof(GallerySettings),
				   settings,
				   ATTRIBUTE_GALLERY_SETTINGS,
				   model) == SUCCESS;
	}

	bool GallerySettings::from_element(EditElementHandleCR eeh, GallerySettings &settings)
	{
		return GallerySettings::from_element(eeh.GetElementId(), settings, eeh.GetModelRef());
	}

	bool GallerySettings::save_to_project() const
	{
		return CSysTool::savePathResultDlgParams(
				   RTYPE_GALLERY_SETTINGS,
				   RSCID_GALLERY_SETTINGS,
				   g_taskID,
				   *this) == SUCCESS;
	}

	bool GallerySettings::save_to_element(ElementId id, DgnModelRefP model) const
	{
		SetElementXAttribute(
			id,
			sizeof(GallerySettings),
			const_cast<GallerySettings *>(this),
			ATTRIBUTE_GALLERY_SETTINGS,
			model);

		// 上游函数未添加检查
		// 暂时返回true
		return true;
	}

	bool GallerySettings::save_to_element(EditElementHandleR eeh) const
	{
		return this->save_to_element(eeh.GetElementId(), eeh.GetModelRef());
	}

	void update_layer_index(GallerySettings::Layer *layers, int count)
	{
		auto back_count = 0;
		for (int i = 0; i < count; ++i)
		{
			const auto &layer = layers[i];
			/// 2是背面
			if (layer.position == RebarPosition::Back)
			{
				++back_count;
			}
		}

		auto front_id = 0;
		auto mid_id = 0;
		auto end_id = back_count + 1;

		for (int i = 0; i < count; ++i)
		{
			auto &layer = layers[i];
			switch (layer.position)
			{
			case 0:
				++front_id;
				layer.layer_index = front_id;
				break;
			case 1:
				++mid_id;
				layer.layer_index = mid_id;
				break;
			default:
				--end_id;
				layer.layer_index = end_id;
				break;
			}
		}
	}

	void GallerySettings::add_default_layer(int count)
	{
		auto new_count = this->layer_count + count;

		for (int i = 0; this->layer_count < new_count; ++this->layer_count, ++i)
		{
			auto &layer = this->layers[this->layer_count];

			layer.layer_index = i;
			layer.direction = static_cast<RebarDirection>((i + 1) & 0x01);
			strcpy(layer.size_str, "12");
			layer.type = 2;
			layer.spacing = 200;
			//layer.start_offset = 0;
			//layer.end_offset = 0;
			//// layer.layer_space = static_cast<int>(layer.direction) * 2000.0;
			//layer.layer_space = 0;
			layer.position = RebarPosition::Front;
			//layer.rebarColor = -1;
			layer.rebarLineStyle = 0;
			layer.rebarWeight = 0;

			layer.starting_end_type.angle = 0;
			layer.starting_end_type.offset = 0;
			layer.starting_end_type.type = EndType::None;
			layer.ending_end_type.angle = 0;
			layer.ending_end_type.offset = 0;
			layer.ending_end_type.type = EndType::None;
		}

		update_layer_index(this->layers, this->layer_count);
	}

	void GallerySettings::remove_layer(int count)
	{
		this->layer_count -= count;
		update_layer_index(this->layers, this->layer_count);
	}

    PIT::Concrete GallerySettings::to_pit_concrete() const
    {
		PIT::Concrete concrete;

		concrete.postiveCover = this->front_cover;
		concrete.sideCover = this->side_cover;
		concrete.reverseCover = this->back_cover;
		concrete.isHandleHole = (int)this->handle_hole;
		concrete.MissHoleSize = this->hole_size;
		concrete.rebarLevelNum = this->layer_count;

		return concrete;
	}
    std::vector<PIT::ConcreteRebar> GallerySettings::to_vec_concrete_rebar() const
    {
		std::vector<PIT::ConcreteRebar> rebar_vec;

		for (auto i = 0; i < this->layer_count; ++i)
		{
			const auto &layer = this->layers[i];

			rebar_vec.push_back(PIT::ConcreteRebar());
			auto &rebar = rebar_vec.back();

			rebar.rebarLevel = layer.layer_index;
			rebar.rebarType = layer.type;
			rebar.spacing = layer.spacing;
			/*rebar.levelSpace = layer.layer_space;
			rebar.startOffset = layer.start_offset;
			rebar.endOffset = layer.end_offset;*/
			rebar.datachange = static_cast<int>(layer.position);
			rebar.rebarDir = static_cast<int>(layer.direction);
			//rebar.rebarColor = layer.rebarColor;
			rebar.rebarLineStyle = layer.rebarLineStyle;
			rebar.rebarWeight = layer.rebarWeight;
			strcpy(rebar.rebarSize, layer.size_str);
		}

		return rebar_vec;
	}

	PIT::EndType::RebarEndPointInfo to_end_point_info(const End& end) {
		PIT::EndType::RebarEndPointInfo end_point_info;

		switch(end.type)
		{
		case EndType::None:break;
		case EndType::Bend:break;
		case EndType::Hook:break;
		case EndType::PolygonalLine:break;
		// [[fallthrough]]
		case EndType::Hook90:
		case EndType::Hook135:
		case EndType::Hook180:
			end_point_info.value1 = end.data.hook_degree.radius;
			end_point_info.value3 = end.data.hook_degree.reserver_length;
			break;
		// [[fallthrough]]
		case EndType::Straight:
			end_point_info.value1 = end.data.straight.length;
			break;
		case EndType::Custom: break;
		default: break;
		}

		return end_point_info;
	}

    std::vector<PIT::EndType> GallerySettings::to_vec_end_type() const
    {
		std::vector<PIT::EndType> end_types;

		for (auto i = 0; i < this->layer_count; ++i)
		{
			const auto &layer = this->layers[i];

			end_types.push_back(PIT::EndType());
			auto &start_end = end_types.back();

			start_end.endType = static_cast<int>(layer.starting_end_type.type);
			start_end.offset = layer.starting_end_type.offset;
			start_end.rotateAngle = layer.starting_end_type.angle;
			start_end.endPtInfo = to_end_point_info(layer.starting_end_type);

			end_types.push_back(PIT::EndType());
			auto &end_end = end_types.back();
			end_end.endType = static_cast<int>(layer.ending_end_type.type);
			end_end.offset = layer.ending_end_type.offset;
			end_end.rotateAngle = layer.ending_end_type.angle;
			end_end.endPtInfo = to_end_point_info(layer.ending_end_type);
		}

		return end_types;
	}

}