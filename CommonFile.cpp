#include "_ustation.h"
#include "CommonFile.h"

ElementHandle g_SelectedElm;
PIT::WallRebarInfo g_wallRebarInfo;
ElementId g_ConcreteId;
std::vector<PIT::ConcreteRebar>	g_vecRebarData;
std::vector<PIT::LapOptions>	g_vecLapOptionData;
std::vector<PIT::EndType>		g_vecEndTypeData;
std::vector<PIT::AssociatedComponent>	g_vecACData;
std::list<CString> g_listRebarType = { _T("HPB300"),_T("HPB335"), _T("HRB400"), _T("HPR500") };
std::list<CString> g_listRebarType2 = { _T("A"),_T("B"), _T("C"), _T("D") };

std::vector<RebarPoint>                   g_vecRebarPtsNoHole;//孔洞切断之前的钢筋起始点
std::vector<RebarPoint>                   g_vecTwinRebarPtsNoHole;//孔洞切断之前的并筋起始点
std::vector<RebarPoint>					  g_vecTieRebarPtsNoHole; //孔洞切断之前的拉筋起始点

map<int, vector<RebarPoint>> g_wallRebarPtsNoHole; //墙与孔洞切断之前的钢筋起始点

TwinBarSet::TwinBarInfo g_twinBarInfo;
std::vector<TwinBarSet::TwinBarLevelInfo>		g_vecTwinBarData;
TieReBarInfo	g_tieRebarInfo;

ElementHandle g_InsertElm;	// 插筋基础
ElementHandle g_ColumnElm;	// 插筋柱实体

InsertRebarInfo				g_InsertRebarInfo;	// 插筋相关信息
std::vector<InsertRebarInfo::WallInfo> g_vecWallInsertInfo; // 墙插筋相关信息

std::map<std::string, IDandModelref> g_mapidAndmodel;//在导入了三维模型后，出图之前，存放所有的板件ID和model
int CoverType;

bool GetConcreteXAttribute(ElementId elmID, DgnModelRefP pModel)
{
	EditElementHandle eeh;
	if (SUCCESS == eeh.FindByID(elmID, pModel))
	{
		int size = 0;
		EditElementHandle eeh1(elmID, pModel); //添加attribute的元素
#pragma region WallRebarInfo设置
		size = (int)sizeof(PIT::WallRebarInfo);
		GetElementXAttribute(elmID, size, g_wallRebarInfo, WallRebarInfoXAttribute, pModel);
#pragma endregion WallRebarInfo设置

#pragma region g_vecRebarData设置
		//存储的attributedata大小
		GetElementXAttribute(elmID, g_vecRebarData, vecRebarDataXAttribute, pModel);
#pragma endregion g_vecRebarData设置

#pragma region g_vecRebarData设置
		//存储的attributedata大小
		GetElementXAttribute(elmID, g_vecLapOptionData, vecLapOptionDataXAttribute, pModel);
#pragma endregion g_vecRebarData设置

#pragma region g_vecRebarData设置
		//存储的attributedata大小
		GetElementXAttribute(elmID, g_vecEndTypeData, vecEndTypeDataXAttribute, pModel);
#pragma endregion g_vecRebarData设置

#pragma region g_vecRebarData设置
		//存储的attributedata大小
		GetElementXAttribute(elmID, g_vecACData, vecACDataXAttribute, pModel);
#pragma endregion g_vecRebarData设置


#pragma region g_vecRebarData设置
		size = (int)sizeof(TwinBarSet::TwinBarInfo);
		GetElementXAttribute(elmID, size, g_twinBarInfo, twinBarInfoXAttribute, pModel);
#pragma endregion g_vecRebarData设置

#pragma region g_vecRebarData设置
		//存储的attributedata大小
		GetElementXAttribute(elmID, g_vecTwinBarData, vecTwinBarDataXAttribute, pModel);
#pragma endregion g_vecRebarData设置
		return true;

	}
	return false;
}

bool SetConcreteXAttribute(ElementId elmID, DgnModelRefP pModel)
{
	EditElementHandle eeh;
	if (SUCCESS == eeh.FindByID(elmID, pModel))
	{
		int size = 0;
		EditElementHandle eeh1(elmID, pModel); //添加attribute的元素
#pragma region WallRebarInfo设置
		size = (int)sizeof(PIT::WallRebarInfo);
		SetElementXAttribute(elmID, size, &g_wallRebarInfo, WallRebarInfoXAttribute, pModel);
#pragma endregion WallRebarInfo设置

#pragma region g_vecRebarData设置
		//存储的attributedata大小
		SetElementXAttribute(elmID, g_vecRebarData, vecRebarDataXAttribute, pModel);
#pragma endregion g_vecRebarData设置

#pragma region g_vecRebarData设置
		//存储的attributedata大小
		SetElementXAttribute(elmID, g_vecLapOptionData, vecLapOptionDataXAttribute, pModel);
#pragma endregion g_vecRebarData设置

#pragma region g_vecRebarData设置
		//存储的attributedata大小
		SetElementXAttribute(elmID, g_vecEndTypeData, vecEndTypeDataXAttribute, pModel);
#pragma endregion g_vecRebarData设置

#pragma region g_vecRebarData设置
		//存储的attributedata大小
		SetElementXAttribute(elmID, g_vecACData, vecACDataXAttribute, pModel);
#pragma endregion g_vecRebarData设置


#pragma region g_vecRebarData设置
		size = (int)sizeof(TwinBarSet::TwinBarInfo);
		SetElementXAttribute(elmID, size, &g_twinBarInfo, twinBarInfoXAttribute, pModel);
#pragma endregion g_vecRebarData设置

#pragma region g_vecRebarData设置
		//存储的attributedata大小
		SetElementXAttribute(elmID, g_vecTwinBarData, vecTwinBarDataXAttribute, pModel);
#pragma endregion g_vecRebarData设置
		return true;

	}
	return false;
}

bool IsValidAndPromout(std::function<bool()> condition, WString outStr)
{
	if (condition())
	{
		mdlOutput_prompt(outStr.data());
		return false;
	}
	return true;
}

bool GetSelectAgenda(ElementAgendaR selectset, WString outStr)
{
	//判断用户在点击按钮之前是否已经选择了元素
	SelectionSetManager::GetManager().BuildAgenda(selectset);
	if (selectset.GetCount() == 0)
	{
		return false;
	}
	//判断用户选择的是什么元素，钢筋元素则返回
	EditElementHandleR testEeh = selectset[0];
	return IsValidAndPromout([&]() {return RebarElement::IsRebarElement(testEeh); }, outStr);
}

