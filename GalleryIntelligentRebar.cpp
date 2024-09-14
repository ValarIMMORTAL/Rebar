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
//读取搭接与锚固长度存到全局变量 g_globalpara
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
//刷新系统参数
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

// 墙配筋
CWallRebarDlg *pWallRebarDlg = NULL;
Public void WallRebarSetting(WCharCP unparsed)
{
	//ElementAgenda selectedElement;
	//if (!GetSelectAgenda(selectedElement, L"请选择墙"))
	//{
	//	return;
	//}

	//// if (selectedElement.GetCount() != 1)
	////{
	////	//不能多选，只能处理一面墙
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

//合并墙配筋
Public void CustomWallRebar(WCharCP unparsed)
{
	EditElementHandle combinewall;
	ElementAgenda selectedElement;
	vector<EditElementHandle*> allWalls;
	vector<EditElementHandle*> holes;
	vector<ElementId> allids;
	map<ElementId, EditElementHandle*> tmpselcets;
	if (!GetSelectAgenda(selectedElement, L"请选择墙或板"))
	{
		return;
	}
	if (selectedElement.GetCount() < 2)
	{
		return;
	}

	string Ename, Ename1; string Etype;
	vector<string> vec;
	GetEleNameAndType(selectedElement[0].GetElementId(), selectedElement[0].GetModelRef(), Ename, Etype);//取出  Ename, Etype  数据  

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
				//	将所有构件都设置联合id,再通过联合id存取所有子构件的id
				ElementId tmpID = combinewall.GetElementId();
				SetElementXAttribute(tmpID, vec_walls, UnionVecWallIDXAttribute, combinewall.GetModelRef());

				AFX_MANAGE_STATE(AfxGetStaticModuleState());
				pWallRebarDlg = new CWallRebarDlg(combinewall, CWnd::FromHandle(MSWIND));
				pWallRebarDlg->SetIsCMD(true);
				pWallRebarDlg->m_isCombineWall = true;//合并配筋
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
	if (!GetSelectAgenda(selectedElement, L"请选择板"))
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
// 联合配筋
CUniteRebarDlg *pUniteRebarDlg = NULL;
Public void UniteRebarSetting(WCharCP unparsed)
{
	ElementAgenda selectedElement;
	if (!GetSelectAgenda(selectedElement, L"请选择板"))
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


// 板配筋
CSlabRebarDlg *pSlabRebarDlg = NULL;
Public void SlabRebarSetting(WCharCP unparsed)
{
	//ElementAgenda selectedElement;
	//if (!GetSelectAgenda(selectedElement, L"请选择板"))
	//{
	//	return;
	//}
	//*if (selectedElement.GetCount() != 1)
	//{
	//	return;
	//}*/
	//g_SelectedElm = selectedElement[0];
	//EditElementHandle eeh(g_SelectedElm, g_SelectedElm.GetModelRef());
	////如果板联合配筋过，重新设置联合id
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

//合并板配筋
CSlabRebarDlg *pCombineSlabRebarDlg = NULL;
Public void CombineSlabRebar(WCharCP unparsed)
{
	ElementAgenda selectedElement;
	if (!GetSelectAgenda(selectedElement, L"请选择板"))
	{
		mdlDialog_dmsgsPrint(L"请选择板");
		return;
	}
	if (selectedElement.GetCount() < 2)
	{
		mdlDialog_dmsgsPrint(L"请选择两个即以上的板");
		return;
	}

	string Ename, Ename1; string Etype;
	GetEleNameAndType(selectedElement[0].GetElementId(), selectedElement[0].GetModelRef(), Ename, Etype);
	if (Etype != "FLOOR")
		return;
	else
	{
		ElementAgenda selectedElement;
		if (!GetSelectAgenda(selectedElement, L"请选择板"))
		{
			mdlDialog_dmsgsPrint(L"请选择板");
			return;
		}
		if (selectedElement.GetCount() < 2)
		{
			mdlDialog_dmsgsPrint(L"请选择两个即以上的板");
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
					// 将所有的构件都设置联合ID,再通过联合ID存取所有子构件的id
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

//面配筋工具
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
	{(CmdHandler)WallRebarSetting, CMD_LDREBAR_PLACE_WALLREBAR},	   // 墙配筋
	//{(CmdHandler)text, CMD_LDREBAR_PLACE_SLABREBAR},						  // 板配筋
	{(CmdHandler)Gallery::cmd_gallery_settings, CMD_LDREBAR_GALLERY_SETTINGS }, //廊道设置
	{(CmdHandler)Gallery::SingleWall::cmd, CMD_LDREBAR_GALLERY_SINGLE_WALL }, //廊道墙单独配筋
	{(CmdHandler)UniteRebarSetting, CMD_LDREBAR_GALLERY_WALL_AND_FLOOR}, // 墙板联合配筋
	{(CmdHandler)CustomWallRebar, CMD_LDREBAR_GALLERY_COMBINEWALLREBAR},	   // 合并墙配筋
	{(CmdHandler)SlabRebarSetting, CMD_LDREBAR_GALLERY_SINGLE_FLOOR }, //廊道板单独配筋
	{(CmdHandler)RefreshDataSetscmd, CMD_LDREBAR_EDIT_REFRESHDATA }, //刷新系统参数
	{(CmdHandler)CombineSlabRebar, CMD_LDREBAR_GALLERY_COMBINE_FLOOR },//合并板配筋
	{(CmdHandler)SetLaeAndLa0, CMD_LDREBAR_GALLERY_SET_LAEANDLA0 },//设置搭接与锚固长度
	{(CmdHandler)startFaceSelectTool, CMD_LDREBAR_GALLERY_FACEREBAR }, //面配筋
	{(CmdHandler)Gallery::cmd_gallery_buttress, CMD_LDREBAR_GALLERY_SINGLE_BUTTRESS },//支墩配筋
	{(CmdHandler)Gallery::cmd_Edge_settings, CMD_LDREBAR_GALLERY_EDAGEREBAR },//沿边加强筋
	{(CmdHandler)Gallery::TieRebarFaceTools, CMD_LDREBAR_GALLERY_FACETIEREBAR },//按面拉筋
	{(CmdHandler)Gallery::StairsRebarSetting, CMD_LDREBAR_GALLERY_STAIRSREBAR },//楼梯配筋
	{(CmdHandler)Gallery::HoleReinForcingRebarSetting, CMD_LDREBAR_GALLERY_HOLEREINFORCINGREBAR },//孔洞加强筋
	{(CmdHandler)DeleteProstheses, CMD_LDREBAR_EDIT_DELETEPROSTHESES }, //删除面配筋假体
	{(CmdHandler)Gallery::cmd_gallery_Catchpit, CMD_LDREBAR_GALLERY_CATCHPITREBAR },//集水坑配筋
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

	// 这里需要注册该类，否则在再次打开项目时，双击钢筋无效
	RebarRexFactory::Register(s_rex_factory);

	SystemCallback::SetUnloadProgramFunction(on_unload);

	//读取搭接与锚固长度存到全局变量 g_globalpara
	SetLa0andLaeValue();

};