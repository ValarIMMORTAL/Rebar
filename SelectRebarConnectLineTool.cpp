#include "_ustation.h"
#include "SelectRebarConnectLineTool.h"

 double SelectRebarConnectLineTool::m_dLineHigh = 0;
void SelectRebarConnectLineTool::InstallNewInstance(int toolId)
{
	mdlSelect_freeAll();
	//清除已选中的元素
	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	//启动工具
	SelectRebarConnectLineTool* tool = new SelectRebarConnectLineTool(toolId);
	tool->InstallTool();
	m_dLineHigh = 0.0;
}

BENTLEY_NAMESPACE_NAME::StatusInt SelectRebarConnectLineTool::_OnElementModify(EditElementHandleR el)
{
	return ERROR;
}

void SelectRebarConnectLineTool::_OnRestartTool()
{
	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	_ExitTool();
}

void SelectRebarConnectLineTool::_SetupAndPromptForNextAction()
{

}

bool SelectRebarConnectLineTool::_WantAdditionalLocate(DgnButtonEventCP ev)
{
	if (NULL == ev)
		return true;
	return (GetElementAgenda().GetCount() < 1 || ev->IsControlKey());
}

bool SelectRebarConnectLineTool::_OnModifyComplete(DgnButtonEventCR ev)
{
	return true;
}

bool SelectRebarConnectLineTool::_DoGroups()
{
	return false;
}

bool SelectRebarConnectLineTool::_WantDynamics()
{
	return false;
}

bool SelectRebarConnectLineTool::_OnDataButton(DgnButtonEventCR ev)
{
	HitPathCP   path = _DoLocate(ev, true, ComponentMode::Innermost);
	if (path == NULL)
		return false;

	ElementHandle theElementHandle(mdlDisplayPath_getElem((DisplayPathP)path, 0), mdlDisplayPath_getPathRoot((DisplayPathP)path));
	EditElementHandle ehLine(theElementHandle, ACTIVEMODEL);//选择的线段

	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	SelectionSetManager::GetManager().AddElement(ehLine.GetElementRef(), ACTIVEMODEL);//加入选择集高亮
	CurveVectorPtr lineCurve = ICurvePathQuery::ElementToCurveVector(ehLine);
	DSegment3dCP segment1 = nullptr;
	if (lineCurve.IsValid())
	{
		ICurvePrimitivePtr& linePrimitive = lineCurve->front();
		segment1 = linePrimitive->GetLineCP();
	}
	if (segment1 == nullptr)
	{
		return false;
	}
	DPoint3d pointStart, pointEnd;
	segment1->GetEndPoints(pointStart, pointEnd);
	if (COMPARE_VALUES_EPS(pointStart.z, pointEnd.z,10) == 0)
	{
		m_dLineHigh = pointEnd.z;
		_ExitTool();
	}
	else 
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK,
			L"线段是不水平，Z值不同，请重新选择线段", MessageBoxIconType::Warning);
	}
}

bool SelectRebarConnectLineTool::_OnModifierKeyTransition(bool wentDown, int key)
{
	if (CTRLKEY != key)
		return false;
	if (GetElementAgenda().GetCount() < 2)
		return false;
	if (wentDown)
	{
		__super::_SetLocateCursor(true);
	}
	else {
		__super::_SetLocateCursor(false);
	}
	return true;
}

bool SelectRebarConnectLineTool::_OnPostLocate(HitPathCP path, WStringR cantAcceptReason)
{
	if (!DgnElementSetTool::_OnPostLocate(path, cantAcceptReason))
		return false;

	ElementHandle eh(path->GetHeadElem(), path->GetRoot());

	if (eh.IsValid() && (eh.GetElementType() == LINE_ELM))
	{
		return true;
	}
	return false;
}

bool SelectRebarConnectLineTool::_OnResetButton(DgnButtonEventCR ev)
{
	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	_ExitTool();
	return true;
}

BENTLEY_NAMESPACE_NAME::EditElementHandleP SelectRebarConnectLineTool::_BuildLocateAgenda(HitPathCP path, DgnButtonEventCP ev)
{
	return __super::_BuildLocateAgenda(path, ev);
}
