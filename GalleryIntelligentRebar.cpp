/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/RebarSDKExample.cpp $
|
|  $Copyright: (c) 2019 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/

#include "_ustation.h"
#include "GalleryIntelligentRebar.h"

#include <CPointTool.h>

#include "GalleryIntelligentRebarids.h"
#include "GalleryIntelligentRebarcmd.h"
#include "CommonFile.h"
#include "XmlHelper.h"
#include "resource.h"
#include "CWallRebarDlg.h"
#include "CSlabRebarDlg.h"
#include "CFacesRebarDlg.h"
#include "SetParam.h"
#include "CmdGallerySettings.h"
#include "CmdGallerySingleWall.h"
#include "CmdGalleryWallAndSlab.h"
#include "RebarRexFactory.h"
#include "CSetLa0.h"
#include "CSetLae.h"
#include "CSetLaeAndLa0Dlg.h"
#include "CUniteRebarDlg.h"
#include "HoleRebarAssembly.h"
#include "SelectFaceTool.h"
#include "CCheckLicenseTool.h"

extern StrLa0 g_StrLa0;
extern StrLae g_StrLae;
//��ȡ�����ê�̳��ȴ浽ȫ�ֱ��� g_globalpara
void SetLa0andLaeValue()
{
	CSetLa0 *La0Data = new CSetLa0;
	La0Data->readXML();

	std::string La0_RebarGrade(CW2A(g_StrLa0.g_StrLa0_RebarGrade));
	if (La0_RebarGrade == "HPB300")
		La0_RebarGrade = "A";
	else if (La0_RebarGrade == "HRB335")
		La0_RebarGrade = "B";
	else if (La0_RebarGrade == "HRB400")
		La0_RebarGrade = "C";
	else if (La0_RebarGrade == "HRB500")
		La0_RebarGrade = "D";

	g_globalpara.m_laplenth[La0_RebarGrade] = g_StrLa0.g_db_La0Value;

	CSetLae *LaeData = new CSetLae;
	LaeData->readXML();
	std::string Lae_RebarGrade(CW2A(g_StrLae.g_StrLae_RebarGrade));
	if (Lae_RebarGrade == "HPB300")
		Lae_RebarGrade = "A";
	else if (Lae_RebarGrade == "HRB400")
		Lae_RebarGrade = "C";
	else if (Lae_RebarGrade == "HRB500")
		Lae_RebarGrade = "D";

	g_globalpara.m_alength[Lae_RebarGrade] = g_StrLae.g_db_LaeValue;

	La0Data = nullptr; LaeData = nullptr;
	delete La0Data;
	delete LaeData;

}
CFacesRebarDlg *pFaceRebarDlg = NULL;
void FacesRebar(ElementHandleCR eeh, ElementId eehnew, const bvector<ISubEntityPtr> &faces)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (!pFaceRebarDlg == NULL)
	{
		delete pFaceRebarDlg;
		pFaceRebarDlg = NULL;
	}
	pFaceRebarDlg = new CFacesRebarDlg(eeh, eehnew, faces, CWnd::FromHandle(MSWIND));
	pFaceRebarDlg->SetIsHide(true);
	pFaceRebarDlg->SetDlgType(0);
	pFaceRebarDlg->Create(IDD_DIALOG_FacesRebar);
	pFaceRebarDlg->ShowWindow(SW_SHOW);
}

extern RebarXmlInfo g_rebarXmlInfo;
//ˢ��ϵͳ����
Public void RefreshDataSetscmd(WCharCP unparsed)
{
	readXML();
	mdlInput_sendSynchronizedKeyin(L"proconcrete manage refreshcodes", 0, INPUTQ_EOQ, NULL);
}

Public void DeleteProstheses(WCharCP unparsed)
{
	DgnModelRefP dgnModelRef = ISessionMgr::GetActiveDgnModelRefP();
	DgnModel::ElementsCollection elmCol = dgnModelRef->AsDgnModelP()->GetElementsCollection();
	int size = 0;
	for each (ElementRefP elementRef in elmCol)
	{
		size++;
		bool bIsInSet = elementRef_isInDisplaySet(elementRef,ACTIVEMODEL);
		if (bIsInSet)
		{
			bool bisBody = false;
			GetElementXAttribute(elementRef->GetElementId(), sizeof(bool), bisBody, FaceBody,ACTIVEMODEL);
			if (bisBody)
			{
				EditElementHandle eeh(elementRef, ACTIVEMODEL);
				eeh.DeleteFromModel();
			}
		}
	}
	mdlInput_sendSynchronizedKeyin(L"displayset clear", 0, INPUTQ_EOQ, NULL);
}

// ǽ���
CWallRebarDlg *pWallRebarDlg = NULL;
Public void WallRebarSetting(WCharCP unparsed)
{
	//ElementAgenda selectedElement;
	//if (!GetSelectAgenda(selectedElement, L"��ѡ��ǽ"))
	//{
	//	return;
	//}

	//// if (selectedElement.GetCount() != 1)
	////{
	////	//���ܶ�ѡ��ֻ�ܴ���һ��ǽ
	////	return;
	//// }
	//g_SelectedElm = selectedElement[0];
	//		WallRebarHook::Initialize();
	// 		MSDialog *pDialog = mdlDialog_find(DIALOGID_WallRebar, NULL);
	// 		if (pDialog)
	// 			mdlDialog_show(pDialog);
	// 		else
	//			mdlDialog_open(NULL,DIALOGID_WallRebar);
	//		mdlDialog_openModal(NULL,NULL, DIALOGID_WallRebar);
	/*ElementId hide_id = 0;
	GetElementXAttribute(g_SelectedElm.GetElementId(), sizeof(ElementId), hide_id, HideElementIdXAttribute, g_SelectedElm.GetModelRef());
	if (hide_id != 0)
	{
		EditElementHandle eeh(hide_id, ACTIVEMODEL);
		g_SelectedElm = eeh;
	}*/
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	pWallRebarDlg = new CWallRebarDlg(g_SelectedElm, CWnd::FromHandle(MSWIND));
	pWallRebarDlg->SetIsCMD(true);
	//		pWallRebarDlg->SetSelectElement(g_SelectedElm);
	pWallRebarDlg->Create(IDD_DIALOG_WallRebar);
	//		dlg.m_PageAssociatedComponent.SetListRowData(vecACData);
	pWallRebarDlg->ShowWindow(SW_SHOW);
}

//�ϲ�ǽ���
Public void CustomWallRebar(WCharCP unparsed)
{
	EditElementHandle combinewall;
	ElementAgenda selectedElement;
	vector<EditElementHandle*> allWalls;
	vector<EditElementHandle*> holes;
	vector<ElementId> allids;
	map<ElementId, EditElementHandle*> tmpselcets;
	if (!GetSelectAgenda(selectedElement, L"��ѡ��ǽ���"))
	{
		return;
	}
	if (selectedElement.GetCount() < 2)
	{
		return;
	}

	string Ename, Ename1; string Etype;
	vector<string> vec;
	GetEleNameAndType(selectedElement[0].GetElementId(), selectedElement[0].GetModelRef(), Ename, Etype);//ȡ��  Ename, Etype  ����  

	if (Ename == "")
	{
		return;
	}
	if (Etype.find("WALL") != string::npos)
	{
		for (EditElementHandleR eeh : selectedElement)
		{
			if (eeh.IsValid())
			{
				DgnModelRefP tmpmodel = eeh.GetModelRef();
				if (tmpmodel->IsDgnAttachment() != true && tmpmodel != ACTIVEMODEL)
				{
					continue;
				}
				if (RebarElement::IsRebarElement(eeh))
				{
					continue;
				}
				ElementId tmpid = eeh.GetElementId();
				if (tmpid != 0)
				{
					tmpselcets[tmpid] = &eeh;
				}
			}
		}
		for (map<ElementId, EditElementHandle*>::iterator itr = tmpselcets.begin(); itr != tmpselcets.end(); itr++)
		{
			if (itr->second == nullptr)
			{
				continue;
			}
			if (HoleRFRebarAssembly::IsSmartSmartFeature(*itr->second))
			{
				ElementId tmpId = -1;
				GetElementXAttribute(itr->second->GetElementId(), sizeof(ElementId), tmpId, UnionWallIDXAttribute, itr->second->GetModelRef());
				if (tmpId != -1)
				{
					vector<ElementId>::iterator it = find(allids.begin(), allids.end(), tmpId);
					if (it == allids.end())
						allids.push_back(tmpId);
				}
				allWalls.push_back(itr->second);
			}
		}
	}
	{
		for (int i = 0; i < allids.size(); i++)
		{
			EditElementHandle teeh(allids.at(i), ACTIVEMODEL);
			if (teeh.IsValid())
			{
				teeh.DeleteFromModel();
			}
		}
		vector<ElementId> vec_walls;
		if (allWalls.size() > 0)
		{
			if (CreateDownFaceAndSetData(allWalls, combinewall, holes))
			{
				for (EditElementHandle* eeh : allWalls)
				{
					ElementId tmpId = combinewall.GetElementId();
					SetElementXAttribute(eeh->GetElementId(), sizeof(ElementId), &tmpId, UnionWallIDXAttribute, eeh->GetModelRef());
					vec_walls.push_back(eeh->GetElementId());
				}

				ElementRefP oldRef = combinewall.GetElementRef();
				mdlElmdscr_setVisible(combinewall.GetElementDescrP(), false);
				combinewall.ReplaceInModel(oldRef);
				//	�����й�������������id,��ͨ������id��ȡ�����ӹ�����id
				ElementId tmpID = combinewall.GetElementId();
				SetElementXAttribute(tmpID, vec_walls, UnionVecWallIDXAttribute, combinewall.GetModelRef());

				AFX_MANAGE_STATE(AfxGetStaticModuleState());
				pWallRebarDlg = new CWallRebarDlg(combinewall, CWnd::FromHandle(MSWIND));
				pWallRebarDlg->SetIsCMD(true);
				pWallRebarDlg->m_isCombineWall = true;//�ϲ����
				//		pWallRebarDlg->SetSelectElement(g_SelectedElm);
				pWallRebarDlg->Create(IDD_DIALOG_WallRebar);
				//		dlg.m_PageAssociatedComponent.SetListRowData(vecACData);
				pWallRebarDlg->ShowWindow(SW_SHOW);
			}
		}

	}
	
}

Public void text(WCharCP unparsed)
{
	ElementAgenda selectedElement;
	if (!GetSelectAgenda(selectedElement, L"��ѡ���"))
	{
		return;
	}
	if (selectedElement.GetCount() < 1)
	{
		return;
	}
	vector<vector<DPoint3d>> allLines;
	ExtractFacesTool::GetVecToCurve(allLines, selectedElement[0].GetElementDescrP(), selectedElement[1].GetElementDescrP(), 200, 50, 100);
}
// �������
CUniteRebarDlg *pUniteRebarDlg = NULL;
Public void UniteRebarSetting(WCharCP unparsed)
{
	ElementAgenda selectedElement;
	if (!GetSelectAgenda(selectedElement, L"��ѡ���"))
	{
		return;
	}
	if (selectedElement.GetCount() < 1)
	{
		return;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	pUniteRebarDlg = new CUniteRebarDlg(g_SelectedElm, CWnd::FromHandle(MSWIND));
	pUniteRebarDlg->SetSelectElement(selectedElement);
	pUniteRebarDlg->Create(IDD_DIALOG_Combine_Setting);
	pUniteRebarDlg->ShowWindow(SW_SHOW);
}


// �����
CSlabRebarDlg *pSlabRebarDlg = NULL;
Public void SlabRebarSetting(WCharCP unparsed)
{
	//ElementAgenda selectedElement;
	//if (!GetSelectAgenda(selectedElement, L"��ѡ���"))
	//{
	//	return;
	//}
	//*if (selectedElement.GetCount() != 1)
	//{
	//	return;
	//}*/
	//g_SelectedElm = selectedElement[0];
	//EditElementHandle eeh(g_SelectedElm, g_SelectedElm.GetModelRef());
	////���������������������������id
	//ElementId eehId = g_SelectedElm.GetElementId();
	//ElementId unionId = 0;
	//int ret = GetElementXAttribute(eehId, sizeof(ElementId), unionId, UnionWallIDXAttribute, g_SelectedElm.GetModelRef());
	//if (ret == SUCCESS)
	//{
	//	SetElementXAttribute(eehId, sizeof(ElementId), &eehId, UnionWallIDXAttribute, g_SelectedElm.GetModelRef());
	//}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	pSlabRebarDlg = new CSlabRebarDlg(g_SelectedElm, CWnd::FromHandle(MSWIND));
	pSlabRebarDlg->SetIsCombineSlab(false);
	pSlabRebarDlg->SetIsCMD(true);
	pSlabRebarDlg->Create(IDD_DIALOG_SlabRebar);
	pSlabRebarDlg->ShowWindow(SW_SHOW);
}

bool IsSmartSmartFeature(EditElementHandle& eeh)
{
	SmartFeatureNodePtr pFeatureNode;
	if (SmartFeatureElement::IsSmartFeature(eeh))
	{
		return true;
	}
	else
	{
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			if (SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef()) == SUCCESS)
			{
				eeh.AddToModel();
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}

		}
		return true;
	}
}

//�ϲ������
CSlabRebarDlg *pCombineSlabRebarDlg = NULL;
Public void CombineSlabRebar(WCharCP unparsed)
{
	ElementAgenda selectedElement;
	if (!GetSelectAgenda(selectedElement, L"��ѡ���"))
	{
		mdlDialog_dmsgsPrint(L"��ѡ���");
		return;
	}
	if (selectedElement.GetCount() < 2)
	{
		mdlDialog_dmsgsPrint(L"��ѡ�����������ϵİ�");
		return;
	}

	string Ename, Ename1; string Etype;
	GetEleNameAndType(selectedElement[0].GetElementId(), selectedElement[0].GetModelRef(), Ename, Etype);
	if (Etype != "FLOOR")
		return;
	else
	{
		ElementAgenda selectedElement;
		if (!GetSelectAgenda(selectedElement, L"��ѡ���"))
		{
			mdlDialog_dmsgsPrint(L"��ѡ���");
			return;
		}
		if (selectedElement.GetCount() < 2)
		{
			mdlDialog_dmsgsPrint(L"��ѡ�����������ϵİ�");
			return;
		}

		vector<EditElementHandle*> allFloors;
		vector<EditElementHandle*> holes;
		vector<ElementId> allids;
		map<ElementId, EditElementHandle*> tmpselcets;
		for (EditElementHandleR eeh : selectedElement)
		{
			if (eeh.IsValid())
			{
				DgnModelRefP tmpmodel = eeh.GetModelRef();
				if (tmpmodel->IsDgnAttachment() != true && tmpmodel != ACTIVEMODEL)
				{
					continue;
				}
				if (RebarElement::IsRebarElement(eeh))
				{
					continue;
				}
				ElementId tmpid = eeh.GetElementId();
				if (tmpid != 0)
				{
					tmpselcets[tmpid] = &eeh;
				}
			}
		}
		for (map<ElementId, EditElementHandle*>::iterator itr = tmpselcets.begin(); itr != tmpselcets.end(); itr++)
		{
			if (itr->second == nullptr)
			{
				continue;
			}
			if (IsSmartSmartFeature(*itr->second))
			{
				ElementId tmpId = -1;
				GetElementXAttribute(itr->second->GetElementId(), sizeof(ElementId), tmpId, UnionWallIDXAttribute, itr->second->GetModelRef());
				if (tmpId != -1)
				{
					vector<ElementId>::iterator it = find(allids.begin(), allids.end(), tmpId);
					if (it == allids.end())
						allids.push_back(tmpId);
				}
				allFloors.push_back(itr->second);
			}
		}
		{
			for (int i = 0; i < allids.size(); i++)
			{
				EditElementHandle teeh(allids.at(i), ACTIVEMODEL);
				if (teeh.IsValid())
				{
					teeh.DeleteFromModel();
				}
			}
			EditElementHandle combinefloor;
			vector<ElementId> vec_floors;
			if (allFloors.size() > 0)
			{
				if (CreateFloorDownFaceAndSetData(allFloors, combinefloor, holes))
				{
					for (EditElementHandle* eeh : allFloors)
					{
						ElementId tmpId = combinefloor.GetElementId();
						SetElementXAttribute(eeh->GetElementId(), sizeof(ElementId), &tmpId, UnionWallIDXAttribute, eeh->GetModelRef());
						vec_floors.push_back(eeh->GetElementId());
					}

					ElementRefP oldRef = combinefloor.GetElementRef();
					mdlElmdscr_setVisible(combinefloor.GetElementDescrP(), false);
					combinefloor.ReplaceInModel(oldRef);
					// �����еĹ�������������ID,��ͨ������ID��ȡ�����ӹ�����id
					ElementId tmpID = combinefloor.GetElementId();
					SetElementXAttribute(tmpID, vec_floors, UnionVecWallIDXAttribute, combinefloor.GetModelRef());

					AFX_MANAGE_STATE(AfxGetStaticModuleState());
					pCombineSlabRebarDlg = new CSlabRebarDlg(combinefloor, CWnd::FromHandle(MSWIND));
					pCombineSlabRebarDlg->SetIsCombineSlab(true);
					//		pWallRebarDlg->SetSelectElement(g_SelectedElm);
					pCombineSlabRebarDlg->Create(IDD_DIALOG_SlabRebar);
					//		dlg.m_PageAssociatedComponent.SetListRowData(vecACData);
					pCombineSlabRebarDlg->ShowWindow(SW_SHOW);
				}

			}

		}
	}

}

CSetLaeAndLa0Dlg * pLaeAndLa0Dlg = nullptr;
Public void SetLaeAndLa0(WCharCP unparsed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pLaeAndLa0Dlg = new CSetLaeAndLa0Dlg(CWnd::FromHandle(MSWIND));
	pLaeAndLa0Dlg->Create(IDD_DIALOG_SetLaeAndLa0);
	pLaeAndLa0Dlg->ShowWindow(SW_SHOW);

}

//������
Public void startFaceSelectTool(WCharCP unparsed)
{
	// NOTE: Call the method to create/install the tool, RefCounted classes don't have public constructors...
	SelectFaceTool::InstallNewInstance(1, 1);
}

// Public void RebarSDK_Test(WCharCP unparsed)
// {
// 	mdlDialog_openModal(NULL, GetResourceHandle(), DIALOGID_WallRebar);
// }
/*---------------------------------------------------------------------------------**/ /**
 * Map: key-in to function
 * Commands
 +---------------+---------------+---------------+---------------+---------------+--*/
static MdlCommandNumber s_commandNumbers[] =
{
	{(CmdHandler)WallRebarSetting, CMD_LDREBAR_PLACE_WALLREBAR},	   // ǽ���
	//{(CmdHandler)text, CMD_LDREBAR_PLACE_SLABREBAR},						  // �����
	{(CmdHandler)Gallery::cmd_gallery_settings, CMD_LDREBAR_GALLERY_SETTINGS }, //�ȵ�����
	{(CmdHandler)Gallery::SingleWall::cmd, CMD_LDREBAR_GALLERY_SINGLE_WALL }, //�ȵ�ǽ�������
	{(CmdHandler)UniteRebarSetting, CMD_LDREBAR_GALLERY_WALL_AND_FLOOR}, // ǽ���������
	{(CmdHandler)CustomWallRebar, CMD_LDREBAR_GALLERY_COMBINEWALLREBAR},	   // �ϲ�ǽ���
	{(CmdHandler)SlabRebarSetting, CMD_LDREBAR_GALLERY_SINGLE_FLOOR }, //�ȵ��嵥�����
	{(CmdHandler)RefreshDataSetscmd, CMD_LDREBAR_EDIT_REFRESHDATA }, //ˢ��ϵͳ����
	{(CmdHandler)CombineSlabRebar, CMD_LDREBAR_GALLERY_COMBINE_FLOOR },//�ϲ������
	{(CmdHandler)SetLaeAndLa0, CMD_LDREBAR_GALLERY_SET_LAEANDLA0 },//���ô����ê�̳���
	{(CmdHandler)startFaceSelectTool, CMD_LDREBAR_GALLERY_FACEREBAR }, //�����
	{(CmdHandler)Gallery::cmd_gallery_buttress, CMD_LDREBAR_GALLERY_SINGLE_BUTTRESS },//֧�����
	{(CmdHandler)Gallery::cmd_Edge_settings, CMD_LDREBAR_GALLERY_EDAGEREBAR },//�ر߼�ǿ��
	{(CmdHandler)Gallery::TieRebarFaceTools, CMD_LDREBAR_GALLERY_FACETIEREBAR },//��������
	{(CmdHandler)Gallery::StairsRebarSetting, CMD_LDREBAR_GALLERY_STAIRSREBAR },//¥�����
	{(CmdHandler)Gallery::HoleReinForcingRebarSetting, CMD_LDREBAR_GALLERY_HOLEREINFORCINGREBAR },//�׶���ǿ��
	{(CmdHandler)DeleteProstheses, CMD_LDREBAR_EDIT_DELETEPROSTHESES }, //ɾ����������
	{(CmdHandler)Gallery::cmd_gallery_Catchpit, CMD_LDREBAR_GALLERY_CATCHPITREBAR },//��ˮ�����
	{NULL, 0}
};

// static MdlCommandName s_commandNames[] =
// 	{
// 		{RebarSDK_Test, "RebarSDK_Test"},
// 		//     { RebarSDK_Metro                , "RebarSDK_Metro" },
// 		// 	{ RebarSDK_KRB                  , "RebarSDK_KRB" },
// 		// 	{ RebarSDK_ARMA                 , "RebarSDK_ARMA" },
// 		// 	{ RebarSDK_PierVase             , "RebarSDK_PierVase" },
// 		{NULL, 0}};

static void InitAndPublishComplexVariable()
{
	// double uor_per_mm = ACTIVEMODEL->GetModelInfoCP ()->GetUorPerMeter () / 1000.0;
	readXML();
	memset(&g_wallRebarInfo, 0, sizeof(g_wallRebarInfo));
	g_wallRebarInfo.concrete.postiveCover = 50;
	g_wallRebarInfo.concrete.reverseCover = 50;
	g_wallRebarInfo.concrete.sideCover = 50;
	g_wallRebarInfo.concrete.rebarLevelNum = 4;
	g_wallRebarInfo.concrete.MissHoleSize = 0;
	g_wallRebarInfo.concrete.isHandleHole = 1;

	SymbolSet *setP = mdlCExpression_initializeSet(VISIBILITY_DIALOG_BOX, 0, true);
	mdlDialog_publishComplexVariable(setP, "WallRebarInfo", "g_wallRebarInfo", &g_wallRebarInfo);

	mdlInput_sendSynchronizedKeyin(L"proconcrete manage refreshcodes", 0, INPUTQ_EOQ, NULL);
}

/*---------------------------------------------------------------------------------**/ /**
 * MdlMain

 * @bsimethod                                                              Bentley Systems
 +---------------+---------------+---------------+---------------+---------------+--*/
static RscFileHandle s_rfHandle = NULL;
WString g_taskID;

RscFileHandle GetResourceHandle()
{
	return s_rfHandle;
}

static RebarRexFactory s_rex_factory;

bool on_unload(UnloadProgramReason reason)
{
	RexFactory::UnRegister(s_rex_factory);
	return true;
}

#define CHECKLICENCE flase
extern "C" DLLEXPORT void MdlMain(int argc, WCharCP argv[])
{
#if CHECKLICENCE
	/* Open the resource file.*/
	if (false == CCheckLicenseTool::CheckLicense("GalleryIntelligentRebar", L"CGNGalleryRebar"))
	{
		mdlInput_sendKeyin(L"mdl unload GalleryIntelligentRebar", 0, INPUTQ_EOQ, NULL);
		return;
	}
#endif

	if (0)
	{
		XmlManager::ReadXmlFile("C:\\ProgramData\\Bentley\\ProStructures CONNECT Edition\\Configuration\\WorkSpaces\\China\\Standards\\ProStructures\\Rebar\\Codes\\RebarCode_HL.xml");
		XmlManager::GetAllRebarDatas();
		g_listRebarType.clear();
		for (int i = 0; i < XmlManager::s_alltypes.size(); i++)
		{
			g_listRebarType.push_back(XmlManager::s_alltypes.at(i));
		}
	}
	else
	{
		g_listRebarType.clear();
		g_listRebarType.push_back(_T("HPB300"));
		g_listRebarType.push_back(_T("HPB335"));
		g_listRebarType.push_back(_T("HRB400"));
		g_listRebarType.push_back(_T("HRB500"));
	}

	mdlResource_openFile(&s_rfHandle, nullptr, RSC_READONLY);
	mdlState_registerStringIds(STRINGLISTID_RebarSDKExampleCommandNames, STRINGLISTID_RebarSDKExampleTextMessages);

	/* Register commands */
	mdlSystem_registerCommandNumbers(s_commandNumbers);
	// mdlSystem_registerCommandNames(s_commandNames);

	mdlParse_loadCommandTable(nullptr);

	g_taskID = mdlSystem_getCurrTaskID();
	loadPathResultDlgParams();
	WString xmlName = g_rebarXmlInfo.xmlName;
	if (xmlName.empty())
	{
		xmlName = L"RebarCode_zhongguangheC40.xml";
		wmemcpy(g_rebarXmlInfo.xmlName, xmlName.GetWCharCP(), 512);
		savePathResultDlgParams();
	}
	InitAndPublishComplexVariable();

	// ������Ҫע����࣬�������ٴδ���Ŀʱ��˫���ֽ���Ч
	RebarRexFactory::Register(s_rex_factory);

	SystemCallback::SetUnloadProgramFunction(on_unload);

	//��ȡ�����ê�̳��ȴ浽ȫ�ֱ��� g_globalpara
	SetLa0andLaeValue();

};