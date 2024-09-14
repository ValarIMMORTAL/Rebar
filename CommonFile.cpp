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

std::vector<RebarPoint>                   g_vecRebarPtsNoHole;//�׶��ж�֮ǰ�ĸֽ���ʼ��
std::vector<RebarPoint>                   g_vecTwinRebarPtsNoHole;//�׶��ж�֮ǰ�Ĳ�����ʼ��
std::vector<RebarPoint>					  g_vecTieRebarPtsNoHole; //�׶��ж�֮ǰ��������ʼ��

map<int, vector<RebarPoint>> g_wallRebarPtsNoHole; //ǽ��׶��ж�֮ǰ�ĸֽ���ʼ��

TwinBarSet::TwinBarInfo g_twinBarInfo;
std::vector<TwinBarSet::TwinBarLevelInfo>		g_vecTwinBarData;
TieReBarInfo	g_tieRebarInfo;

ElementHandle g_InsertElm;	// ������
ElementHandle g_ColumnElm;	// �����ʵ��

InsertRebarInfo				g_InsertRebarInfo;	// ��������Ϣ
std::vector<InsertRebarInfo::WallInfo> g_vecWallInsertInfo; // ǽ��������Ϣ

std::map<std::string, IDandModelref> g_mapidAndmodel;//�ڵ�������άģ�ͺ󣬳�ͼ֮ǰ��������еİ��ID��model
int CoverType;

bool GetConcreteXAttribute(ElementId elmID, DgnModelRefP pModel)
{
	EditElementHandle eeh;
	if (SUCCESS == eeh.FindByID(elmID, pModel))
	{
		int size = 0;
		EditElementHandle eeh1(elmID, pModel); //���attribute��Ԫ��
#pragma region WallRebarInfo����
		size = (int)sizeof(PIT::WallRebarInfo);
		GetElementXAttribute(elmID, size, g_wallRebarInfo, WallRebarInfoXAttribute, pModel);
#pragma endregion WallRebarInfo����

#pragma region g_vecRebarData����
		//�洢��attributedata��С
		GetElementXAttribute(elmID, g_vecRebarData, vecRebarDataXAttribute, pModel);
#pragma endregion g_vecRebarData����

#pragma region g_vecRebarData����
		//�洢��attributedata��С
		GetElementXAttribute(elmID, g_vecLapOptionData, vecLapOptionDataXAttribute, pModel);
#pragma endregion g_vecRebarData����

#pragma region g_vecRebarData����
		//�洢��attributedata��С
		GetElementXAttribute(elmID, g_vecEndTypeData, vecEndTypeDataXAttribute, pModel);
#pragma endregion g_vecRebarData����

#pragma region g_vecRebarData����
		//�洢��attributedata��С
		GetElementXAttribute(elmID, g_vecACData, vecACDataXAttribute, pModel);
#pragma endregion g_vecRebarData����


#pragma region g_vecRebarData����
		size = (int)sizeof(TwinBarSet::TwinBarInfo);
		GetElementXAttribute(elmID, size, g_twinBarInfo, twinBarInfoXAttribute, pModel);
#pragma endregion g_vecRebarData����

#pragma region g_vecRebarData����
		//�洢��attributedata��С
		GetElementXAttribute(elmID, g_vecTwinBarData, vecTwinBarDataXAttribute, pModel);
#pragma endregion g_vecRebarData����
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
		EditElementHandle eeh1(elmID, pModel); //���attribute��Ԫ��
#pragma region WallRebarInfo����
		size = (int)sizeof(PIT::WallRebarInfo);
		SetElementXAttribute(elmID, size, &g_wallRebarInfo, WallRebarInfoXAttribute, pModel);
#pragma endregion WallRebarInfo����

#pragma region g_vecRebarData����
		//�洢��attributedata��С
		SetElementXAttribute(elmID, g_vecRebarData, vecRebarDataXAttribute, pModel);
#pragma endregion g_vecRebarData����

#pragma region g_vecRebarData����
		//�洢��attributedata��С
		SetElementXAttribute(elmID, g_vecLapOptionData, vecLapOptionDataXAttribute, pModel);
#pragma endregion g_vecRebarData����

#pragma region g_vecRebarData����
		//�洢��attributedata��С
		SetElementXAttribute(elmID, g_vecEndTypeData, vecEndTypeDataXAttribute, pModel);
#pragma endregion g_vecRebarData����

#pragma region g_vecRebarData����
		//�洢��attributedata��С
		SetElementXAttribute(elmID, g_vecACData, vecACDataXAttribute, pModel);
#pragma endregion g_vecRebarData����


#pragma region g_vecRebarData����
		size = (int)sizeof(TwinBarSet::TwinBarInfo);
		SetElementXAttribute(elmID, size, &g_twinBarInfo, twinBarInfoXAttribute, pModel);
#pragma endregion g_vecRebarData����

#pragma region g_vecRebarData����
		//�洢��attributedata��С
		SetElementXAttribute(elmID, g_vecTwinBarData, vecTwinBarDataXAttribute, pModel);
#pragma endregion g_vecRebarData����
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
	//�ж��û��ڵ����ť֮ǰ�Ƿ��Ѿ�ѡ����Ԫ��
	SelectionSetManager::GetManager().BuildAgenda(selectset);
	if (selectset.GetCount() == 0)
	{
		return false;
	}
	//�ж��û�ѡ�����ʲôԪ�أ��ֽ�Ԫ���򷵻�
	EditElementHandleR testEeh = selectset[0];
	return IsValidAndPromout([&]() {return RebarElement::IsRebarElement(testEeh); }, outStr);
}

