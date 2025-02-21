/*--------------------------------------------------------------------------------------+
|
|     $Source: MstnExamples/Elements/exampleSolids/exampleModifyFaceTool.cpp $
|
|  $Copyright: (c) 2015 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#pragma once

#include "GalleryIntelligentRebar.h"
#include <DgnView\LocateSubEntityTool.h>   
#include <PSolid\PSolidCoreAPI.h>
#include "PITMSCECommon.h"
#include <RebarDetailElement.h>
#include "ExtractFacesTool.h"
#include "ScanIntersectTool.h"
#include "CSolidTool.h"

USING_NAMESPACE_BENTLEY_DGNPLATFORM;
USING_NAMESPACE_BENTLEY_MSTNPLATFORM;
USING_NAMESPACE_BENTLEY_MSTNPLATFORM_ELEMENT;


//bvector<ISubEntityPtr> g_vecEntity;

/*=================================================================================**//**
* Example showing how to use LocateSubEntityTool to write a tool for applying
* local operations to faces using the solids kernel api.
*
* @bsiclass                                                               Bentley Systems
+===============+===============+===============+===============+===============+======*/
struct          SelectFaceTool : LocateSubEntityTool
{
	ElementHandle	_eh;
	ElementId		_ehNew;
	int				_selectNum;
public:
	SelectFaceTool(int cmdName, int prompt, ElementHandleCR ehOld, ElementId ehNew, int selectNum = 1) :_eh(ehOld), _ehNew(ehNew), _selectNum(selectNum)
	{
		SetCmdName(cmdName, prompt);
	}

protected:


	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	

	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual bool _CollectCurves() override { return false; } // Tool does not support wire bodies...wire bodies won't be collected.
	virtual bool _CollectSurfaces() override { return false; } // Tool does not support sheet bodies...sheet bodies won't be collected.

	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual BentleyStatus _OnProcessSolidPrimitive(ISolidPrimitivePtr& geomPtr, DisplayPathCR path) override { return ERROR; } // Promote capped surface to solid body...
	virtual BentleyStatus _OnProcessPolyface(PolyfaceHeaderPtr& geomPtr, DisplayPathCR path) override { return SUCCESS; } // Don't convert a closed mesh to a BRep (and don't collect), can be expensive for large meshes...

	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual ISubEntity::SubEntityType _GetSubEntityTypeMask() override { return ISubEntity::SubEntityType_Face; }
	virtual bool _RequireSubEntitySupport() override { return true; } // Require solid w/at least 1 face...
	virtual bool _AcceptIdentifiesSubEntity() { return false; } // Solid accept point may also accept first face (except hollow which can apply to entire body)...
	virtual bool _AllowMissToAccept(DgnButtonEventCR ev) { return __super::_AllowMissToAccept(ev); } // Don't require face for hollow...

	/*---------------------------------------------------------------------------------**//**
	* Return true if this element should be accepted for the modify operation.
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual bool _IsElementValidForOperation(ElementHandleCR eh, HitPathCP path, WStringR cantAcceptReason) override
	{
		// Base class implementation returns true if geometry cache isn't empty, which in this case means the cache contains at least 1 BRep solid.
		// To be valid for modification element should be fully represented by a single solid; reject if there are multiple solid bodies or missing geometry.
		// NOTE: Simple count test is sufficient (w/o also checking TryGetAsBRep) as override of _Collect and _OnProcess methods have tool only collecting BRep solids.
		return (__super::_IsElementValidForOperation(eh, path, cantAcceptReason) && 1 == GetElementGraphicsCacheCount(eh) && !IsGeometryMissing(eh));
	}

	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	double GetDistance()
	{
		// For testing purposes just use some % of element's range. This value would normally be supplied via tool settings.
		ScanRangeCP elRange = &GetElementAgenda().GetFirstP()->GetElementCP()->hdr.dhdr.range;
		DPoint3d    range[2];

		range[0].x = (double)elRange->xlowlim;
		range[0].y = (double)elRange->ylowlim;
		range[0].z = (double)elRange->zlowlim;
		range[1].x = (double)elRange->xhighlim;
		range[1].y = (double)elRange->yhighlim;
		range[1].z = (double)elRange->zhighlim;

		return range[0].Distance(range[1]) * 0.1;
	}

	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	double GetAngle()
	{
		// For testing purposes just use 10 degrees. This value would normally be supplied via tool settings.
		return Angle::DegreesToRadians(10.0);
	}
	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual StatusInt _OnElementModify(EditElementHandleR eeh) override
	{
		//bvector<IElementGraphicsPtr>  geomCache;

		//// NOTE: Since we setup tool to only collect a single brep, we can just grab first cache entry...
		//ISolidKernelEntityPtr   entityPtr = (SUCCESS == GetElementGraphicsCache(eeh, geomCache) ? TryGetAsBRep(geomCache.front()) : NULL);

		//if (!entityPtr.IsValid())
		//	return ERROR;
		bvector<ISubEntityPtr> faces = GetAcceptedSubEntities();
// 		if (_selectNum > 1)	
// 		{
// 			if (faces.size() > 0)
// 			{
// 				g_vecEntity.push_back(faces[0]);
// 			}
// 
// 			if (g_vecEntity.size() == _selectNum)
// 			{
// 				FacesRebar(_eh, g_vecEntity,true);
// 				EditElementHandle eehDel(_ehNew, ACTIVEMODEL);
// 				eehDel.DeleteFromModel();
// 				mdlInput_sendSynchronizedKeyin(L"reference display on all", 0, INPUTQ_EOQ, NULL);
// 				g_vecEntity.clear();
// 			}
// 		}
// 		else
// 		{
			//_ExitTool();
			/*testBegin*/
			/*ElementId ehid = _eh.GetElementId();
			EditElementHandle face(_ehNew, ACTIVEMODEL);
			mdlElmdscr_add(face.GetElementDescrP());*/
			/*testEnd*/
			FacesRebar(_eh,_ehNew, faces);
			//EditElementHandle eehDel(_ehNew, ACTIVEMODEL);
			//eehDel.DeleteFromModel();
			mdlSelect_freeAll();
			//mdlInput_sendSynchronizedKeyin(L"reference display on all", 0, INPUTQ_EOQ, NULL);	
//		}

		return ERROR;
	}

	/*---------------------------------------------------------------------------------**//**
	* Install a new instance of the tool. Will be called in response to external events
	* such as undo or by the base class from _OnReinitialize when the tool needs to be
	* reset to it's initial state.
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual void _OnRestartTool() override
	{
		bvector<ISubEntityPtr> faces = GetAcceptedSubEntities();
		if (faces.size() == 0)
		{
			EditElementHandle eehDel(_ehNew, ACTIVEMODEL);
			eehDel.DeleteFromModel();
			mdlInput_sendSynchronizedKeyin(L"displayset clear", 0, INPUTQ_EOQ, NULL);
			_ExitTool();
			return;
		}
	}

	virtual bool _OnResetButton(DgnButtonEventCR  ev) override
	{
		EditElementHandle eehDel(_ehNew, ACTIVEMODEL);
		eehDel.DeleteFromModel();
		mdlInput_sendSynchronizedKeyin(L"displayset clear", 0, INPUTQ_EOQ, NULL);
		_ExitTool();
		return true;
	}

	bool    IsRebarDetailSet(ElementHandleCR eh) { return NULL != RebarDetailSet::Fetch(eh); }

	bool	_IsModifyOriginal() override { return false; }//使能选择参考文件中的元素
// 	virtual void    _OnPostInstall() override
// 	{
// 		ElementCopyContextP pElmcopy = _GetCopyContext();
// 		pElmcopy->AddHandler()
// 	}

	virtual bool _OnPostLocate(HitPathCP path, WStringR cantAcceptReason) override
	{
		if (!DgnElementSetTool::_OnPostLocate(path, cantAcceptReason))
			return false;

		ElementHandle eh(path->GetHeadElem(), path->GetRoot());

		return !(RebarElement::IsRebarElement(eh) || IsRebarDetailSet(eh));
	}

public:
	static void GetElementAllRebar(ElementAgenda& agenda, EditElementHandleR eeh, DgnModelRefP modelRef)
	{
		ElementId concreteid = 0;
		int eehId = eeh.GetElementId();
		GetElementXAttribute(eeh.GetElementId(), sizeof(concreteid), concreteid, ConcreteIDXAttribute, eeh.GetModelRef());
		EditElementHandle coneeh;
		vector<ElementId> vecRebars;
		if (!coneeh.FindByID(concreteid, modelRef))
		{//modelRef中找到了
			GetElementXAttribute(concreteid, vecRebars, RebarIdXAttribute, modelRef);//将混凝土中存好的RebarId取出来
		}
		else
		{
			GetElementXAttribute(concreteid, vecRebars, RebarIdXAttribute, eeh.GetModelRef());//将混凝土中存好的RebarId取出来
		}
		//获取组装实体中的RebarSetid
		RebarAssemblies area;
		REA::GetRebarAssemblies(concreteid, area);
		vector<ElementId>vctRebarSetid;
		GetVectorRebarSetsFromAssemblies(vctRebarSetid, area);
		//将相同的RebarSetid移除
		std::sort(vctRebarSetid.begin(), vctRebarSetid.end());
		vctRebarSetid.erase(std::unique(vctRebarSetid.begin(), vctRebarSetid.end()), vctRebarSetid.end());
		for (int x = 0; x < vctRebarSetid.size(); x++)
		{
			RebarSetP rebarSet = RebarSet::Fetch(vctRebarSetid[x], modelRef);
			int nNum = (int)rebarSet->GetChildElementCount(); // 钢筋组中钢筋数量
			for (int j = 0; j < nNum; j++)
			{
				RebarElementP pRebar = rebarSet->GetChildElement(j);
				ElementId id = pRebar->GetElementId();
				vecRebars.push_back(id);
			}
		}
		std::sort(vecRebars.begin(), vecRebars.end());
		vecRebars.erase(std::unique(vecRebars.begin(), vecRebars.end()), vecRebars.end());
		for (auto it : vecRebars)
		{
			EditElementHandle eeh(it, modelRef);
			agenda.Insert(eeh);
		}

		if (agenda.GetCount() == 0)
		{
			vector<ElementId> vec_floors;
			ElementId tmpId = eeh.GetElementId();
			//GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), tmpId, UnionWallIDXAttribute, eeh.GetModelRef());
			if (tmpId != -1)
			{
				EditElementHandle unionFloor(tmpId, ACTIVEMODEL);
				GetElementXAttribute(tmpId, vec_floors, UnionVecWallIDXAttribute, unionFloor.GetModelRef());
			}
			if (vec_floors.size() == 0)
			{
				return;
			}
			for (auto floorid : vec_floors)
			{
				GetElementXAttribute(floorid, sizeof(concreteid), concreteid, ConcreteIDXAttribute, eeh.GetModelRef());

				EditElementHandle coneeh;
				vector<ElementId> vecRebars;
				if (!coneeh.FindByID(concreteid, modelRef))
				{//modelRef中找到了
					GetElementXAttribute(concreteid, vecRebars, RebarIdXAttribute, modelRef);//将混凝土中存好的RebarId取出来
				}
				else
				{
					GetElementXAttribute(concreteid, vecRebars, RebarIdXAttribute, modelRef);//将混凝土中存好的RebarId取出来
				}
				//获取组装实体中的RebarSetid
				RebarAssemblies area;
				REA::GetRebarAssemblies(concreteid, area);
				vector<ElementId>vctRebarSetid;
				GetVectorRebarSetsFromAssemblies(vctRebarSetid, area);
				//将相同的RebarSetid移除
				std::sort(vctRebarSetid.begin(), vctRebarSetid.end());
				vctRebarSetid.erase(std::unique(vctRebarSetid.begin(), vctRebarSetid.end()), vctRebarSetid.end());
				for (int x = 0; x < vctRebarSetid.size(); x++)
				{
					RebarSetP rebarSet = RebarSet::Fetch(vctRebarSetid[x], modelRef);
					int nNum = (int)rebarSet->GetChildElementCount(); // 钢筋组中钢筋数量
					for (int j = 0; j < nNum; j++)
					{
						RebarElementP pRebar = rebarSet->GetChildElement(j);
						ElementId id = pRebar->GetElementId();
						vecRebars.push_back(id);
					}
				}
				std::sort(vecRebars.begin(), vecRebars.end());
				vecRebars.erase(std::unique(vecRebars.begin(), vecRebars.end()), vecRebars.end());
				for (auto it : vecRebars)
				{
					EditElementHandle eeh(it, modelRef);
					agenda.Insert(eeh);
				}
				if (agenda.GetCount() > 0)
				{
					return;
				}
			}

		}
	}
	/*---------------------------------------------------------------------------------**//**
	* Method to create and install a new instance of the tool. If InstallTool returns ERROR,
	* the new tool instance will be freed/invalid. Never call delete on RefCounted classes.
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	static void InstallNewInstance(int toolId, int prompt)
	{
		ElementAgenda selectedElement;
		SelectionSetManager::GetManager().BuildAgenda(selectedElement);
		

		if (selectedElement.GetCount() > 0)
		{
			if (selectedElement.GetCount()!=1)
			{
				mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"当前面配筋不支持选择多个体，若需要，您可继续操作，钢筋布置完成后可能需要手动修改", MessageBoxIconType::Information);
				EditElementHandleP firstP = selectedElement.GetFirstP();
				EditElementHandleP endp = firstP + selectedElement.GetCount();
				MSElementDescrP mainDP = firstP->GetElementDescrP();
				firstP++;
				vector<MSElementDescrP> boolDP;
				for (; firstP != endp; firstP++)
				{
					boolDP.push_back(firstP->GetElementDescrP());
				}
			
				PITCommonTool::CSolidTool::SolidBoolOperation(mainDP, boolDP,
					BOOLOPERATION::BOOLOPERATION_UNITE, ACTIVEMODEL);

				EditElementHandle testeeh(mainDP, true,false,ACTIVEMODEL);
				//testeeh.AddToModel();
				EditElementHandle Eleeh;
				std::vector<EditElementHandle*> Holeehs;
				if (!EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs))
					return;
				std::map<std::string, IDandModelref> alldoorholes;
				std::map<std::string, IDandModelref> allnegholes;
				ScanWallDoorHoles(alldoorholes, allnegholes, testeeh);

				vector<MSElementDescrP> holeDecrPs;
				for (auto it : Holeehs)
				{
					holeDecrPs.push_back(it->GetElementDescrP());
				}
				MSElementDescrP mainDecrP = Eleeh.GetElementDescrP();
				SolidBoolOperation(mainDecrP, holeDecrPs, BOOLOPERATION_SUBTRACT, ACTIVEMODEL);
				Eleeh.ReplaceElementDescr(mainDecrP);

				ElementCopyContext copier(ACTIVEMODEL);
				EditElementHandle eehCopy;
				copier.SetSourceModelRef(Eleeh.GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(Eleeh);
				ISolidKernelEntityPtr ptarget;
				SolidUtil::Convert::ElementToBody(ptarget, Eleeh);
				SolidUtil::Convert::BodyToElement(Eleeh, *ptarget, NULL, *ACTIVEMODEL);
				int ret = Eleeh.AddToModel();
				EditElementHandle eeh;
				if (allnegholes.size() > 0)
				{
#pragma region 参数化创建墙和孔洞
					if (NULL == mdlSystem_findMdlDesc(L"SMARTFEATURE"))
						mdlSystem_loadMdlProgram(L"SMARTFEATURE", L"SMARTFEATURE", L"");

					T_ChildElementToControlFlagsMap _childElementToControlFlagsMap;
					T_ControlFlagsVector flagVec1;
					flagVec1.push_back(false); // visible  
					flagVec1.push_back(false); // temporary
					flagVec1.push_back(false); // profile
					_childElementToControlFlagsMap[Eleeh.GetElementRef()] = flagVec1;
					for (std::map<std::string, IDandModelref>::iterator itr = allnegholes.begin(); itr != allnegholes.end(); itr++)
					{
						if (itr->second.ID != 0 && itr->second.tModel != nullptr)
						{
							EditElementHandle holeeeh(itr->second.ID, itr->second.tModel);
							copier.DoCopy(holeeeh);
							holeeeh.AddToModel();
							T_ControlFlagsVector flagVec2;
							flagVec2.push_back(false); // visible  
							flagVec2.push_back(false); // temporary
							flagVec2.push_back(false); // profile
							_childElementToControlFlagsMap[holeeeh.GetElementRef()] = flagVec2;
						}
					}

					SmartFeatureNodePtr _unioNode;
					if (SUCCESS != FeatureCreate::CreateDifferenceFeature(_unioNode)) return;
					if ((SUCCESS != SmartFeatureElement::CreateAndWriteSmartFeatureElement(eeh, Eleeh, Eleeh.GetDgnModelP(), *_unioNode, _childElementToControlFlagsMap, true)) && (allnegholes.size() > 0))
					{
						mdlDialog_dmsgsPrint(L"canshuhuashibai!");
						return;
					}

					mdlDependency_processAffected();
#pragma endregion 参数化创建墙和孔洞
				}
				else
				{
					eeh.Duplicate(Eleeh);
				}
				ElementId ehNew = eeh.GetElementId();
				mdlSelect_freeAll();
				ElementAgenda selectset;//用户选择集
				SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), ACTIVEMODEL);
				//mdlInput_sendSynchronizedKeyin(L"reference display off all", 0, INPUTQ_EOQ, NULL);
				mdlInput_sendSynchronizedKeyin(L"displayset set selection", 0, INPUTQ_HEAD, NULL);
				SelectFaceTool* tool = new SelectFaceTool(toolId, prompt, eeh, ehNew);
				tool->InstallTool();
			}
			else 
			{
				EditElementHandleP firstp = selectedElement.GetFirstP();

				//获取已有钢筋
				ElementAgenda agenda;
				GetElementAllRebar(agenda, *firstp, ACTIVEMODEL);
				if (agenda.GetCount() == 0)
				{
					ChangeSourceeeh(*firstp);
					ElementId concreteid = 0;
					GetElementXAttribute(firstp->GetElementId(), sizeof(concreteid), concreteid, ConcreteIDXAttribute, firstp->GetModelRef()/*modelPtr*/);
					if (concreteid != 0)
					{
						GetElementAllRebar(agenda, *firstp, ACTIVEMODEL);
					}
				}
				//end


				EditElementHandle testeeh(selectedElement[0], false);
				EditElementHandle Eleeh;
				std::vector<EditElementHandle*> Holeehs;
				if (!EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs))
					return;
				std::map<std::string, IDandModelref> alldoorholes;
				std::map<std::string, IDandModelref> allnegholes;
				ScanWallDoorHoles(alldoorholes, allnegholes, testeeh);

				vector<MSElementDescrP> holeDecrPs;
				for (auto it : Holeehs)
				{
					holeDecrPs.push_back(it->GetElementDescrP());
				}
				MSElementDescrP mainDecrP = Eleeh.GetElementDescrP();
				SolidBoolOperation(mainDecrP, holeDecrPs, BOOLOPERATION_SUBTRACT, ACTIVEMODEL);
				Eleeh.ReplaceElementDescr(mainDecrP);

				ElementCopyContext copier(ACTIVEMODEL);
				EditElementHandle eehCopy;
				copier.SetSourceModelRef(Eleeh.GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(Eleeh);
				ISolidKernelEntityPtr ptarget;
				SolidUtil::Convert::ElementToBody(ptarget, Eleeh);
				SolidUtil::Convert::BodyToElement(Eleeh, *ptarget, NULL, *ACTIVEMODEL);
				int ret = Eleeh.AddToModel();
				EditElementHandle eeh;
				if (allnegholes.size() > 0)
				{
#pragma region 参数化创建墙和孔洞
					if (NULL == mdlSystem_findMdlDesc(L"SMARTFEATURE"))
						mdlSystem_loadMdlProgram(L"SMARTFEATURE", L"SMARTFEATURE", L"");

					T_ChildElementToControlFlagsMap _childElementToControlFlagsMap;
					T_ControlFlagsVector flagVec1;
					flagVec1.push_back(false); // visible  
					flagVec1.push_back(false); // temporary
					flagVec1.push_back(false); // profile
					_childElementToControlFlagsMap[Eleeh.GetElementRef()] = flagVec1;
					for (std::map<std::string, IDandModelref>::iterator itr = allnegholes.begin(); itr != allnegholes.end(); itr++)
					{
						if (itr->second.ID != 0 && itr->second.tModel != nullptr)
						{
							EditElementHandle holeeeh(itr->second.ID, itr->second.tModel);
							copier.DoCopy(holeeeh);
							holeeeh.AddToModel();
							T_ControlFlagsVector flagVec2;
							flagVec2.push_back(false); // visible  
							flagVec2.push_back(false); // temporary
							flagVec2.push_back(false); // profile
							_childElementToControlFlagsMap[holeeeh.GetElementRef()] = flagVec2;
						}
					}

					SmartFeatureNodePtr _unioNode;
					if (SUCCESS != FeatureCreate::CreateDifferenceFeature(_unioNode)) return;
					if ((SUCCESS != SmartFeatureElement::CreateAndWriteSmartFeatureElement(eeh, Eleeh, Eleeh.GetDgnModelP(), *_unioNode, _childElementToControlFlagsMap, true)) && (allnegholes.size() > 0))
					{
						mdlDialog_dmsgsPrint(L"canshuhuashibai!");
						return;
					}

					mdlDependency_processAffected();
#pragma endregion 参数化创建墙和孔洞
				}
				else
				{
					eeh.Duplicate(Eleeh);
				}
				ElementId ehNew = eeh.GetElementId();
				mdlSelect_freeAll();
				ElementAgenda selectset;//用户选择集
				bool BisFaceBody = true;
				SetElementXAttribute(ehNew, sizeof(bool), &BisFaceBody, FaceBody, eeh.GetModelRef());
				SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), ACTIVEMODEL);
				EditElementHandleP firstRebar = agenda.GetFirstP();
				EditElementHandleP endRebar = firstRebar + agenda.GetCount();
				for (; firstRebar!= endRebar; firstRebar++)
				{
					SelectionSetManager::GetManager().AddElement(firstRebar->GetElementRef(), ACTIVEMODEL);
				}
				//mdlInput_sendSynchronizedKeyin(L"reference display off all", 0, INPUTQ_EOQ, NULL);
				mdlInput_sendSynchronizedKeyin(L"displayset set selection", 0, INPUTQ_HEAD, NULL);
				SelectFaceTool* tool = new SelectFaceTool(toolId, prompt, testeeh, ehNew);
				tool->InstallTool();
			}
		
		}
	}
};

