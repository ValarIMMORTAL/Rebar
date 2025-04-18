/*--------------------------------------------------------------------------------------+
|
|     $Source: MstnExamples/Elements/exampleSolids/exampleModifyFaceTool.cpp $
|
|  $Copyright: (c) 2015 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "GalleryIntelligentRebar.h"
#include <DgnView\LocateSubEntityTool.h>   
#include <PSolid\PSolidCoreAPI.h>
#include "PITMSCECommon.h"
#include <RebarDetailElement.h>
#include "ExtractFacesTool.h"
#include "ScanIntersectTool.h"
#include "PITBimMSCEConvert.h"

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
struct          SelectLineTool : LocateSubEntityTool
{
	ElementHandle	_eh;
	ElementId		_ehNew;
	CEdgeLineRebarDlg* _EdgeDlg;
	int				_selectNum;
protected:


	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	SelectLineTool(int cmdName, int prompt, ElementHandleCR ehOld, ElementId ehNew, CEdgeLineRebarDlg* prt ,int selectNum = 1) :_eh(ehOld), _ehNew(ehNew), _selectNum(selectNum),_EdgeDlg(prt)
	{
		SetCmdName(cmdName, prompt);
	}

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
	virtual ISubEntity::SubEntityType _GetSubEntityTypeMask() override { return ISubEntity::SubEntityType_Edge; }
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
		bvector<ISubEntityPtr> Lines = GetAcceptedSubEntities();
		//IsSolidKernelSubEntity(*Lines);
		//_eh.AddToModel();
		//mdlElmdscr_add(_eh.GetElementDescrCP())
		if (Lines.size() < 1)
			return SUCCESS;
		EditElementHandle eehLine;
		PIT::ConvertToElement::SubEntityToElement(eehLine, Lines[0], ACTIVEMODEL);
		if (eehLine.GetElementType() == LINE_ELM)
		{
			DPoint3d pt[2];
			mdlLinear_extract(pt, NULL, eehLine.GetElementP(), ACTIVEMODEL);
			_EdgeDlg->ptLine[0] = pt[0];
			_EdgeDlg->ptLine[1] = pt[1]; 
			_EdgeDlg->Linevec = pt[1] - pt[0];
			_EdgeDlg->Linevec.Normalize();
			_EdgeDlg->CalaRebarPoint();
		}
		//eehLine.AddToModel();
		
 		//FacesRebar(_eh, faces);

		mdlSelect_freeAll();

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

	/*---------------------------------------------------------------------------------**//**
	* Method to create and install a new instance of the tool. If InstallTool returns ERROR,
	* the new tool instance will be freed/invalid. Never call delete on RefCounted classes.
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	static void InstallNewInstance(int toolId, int prompt, CEdgeLineRebarDlg* Ptr)
	{
		ElementAgenda selectedElement;
		SelectionSetManager::GetManager().BuildAgenda(selectedElement);
		if (selectedElement.GetCount() > 0)
		{
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
			SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), ACTIVEMODEL);
			//mdlInput_sendSynchronizedKeyin(L"reference display off all", 0, INPUTQ_EOQ, NULL);
			mdlInput_sendSynchronizedKeyin(L"displayset set selection", 0, INPUTQ_HEAD, NULL);
			SelectLineTool* tool = new SelectLineTool(toolId, prompt, testeeh, ehNew,Ptr);
			tool->InstallTool();
		}
	}
};

//选中实体中的线条
//Public void startLineSelectTool(WCharCP unparsed)
//{
//	// NOTE: Call the method to create/install the tool, RefCounted classes don't have public constructors...
//	SelectLineTool::InstallNewInstance(1, 1);
//}