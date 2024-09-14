/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/ReadRebarTool.cpp $
|
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include "ReadRebarTool.h"
#include "PITArcSegment.h"

bool PITSelectRebarTool::_FilterAgendaEntries ()
{
    bool                changed = false;
    EditElementHandleP  start = GetElementAgenda ().GetFirstP ();
    EditElementHandleP  end = start + GetElementAgenda ().GetCount ();

    for (EditElementHandleP curr = start; curr < end; curr++)
     {
        if (!(RebarElement::IsRebarElement (*curr) || IsRebarDetailSet (*curr)))
        {
            curr->Invalidate ();
            changed = true;
        }
    }
    return changed;
}

bool PITSelectRebarTool::_OnPostLocate (HitPathCP path, WStringR cantAcceptReason)
{
    if (!__super::_OnPostLocate (path, cantAcceptReason))
        return false;

    ElementHandle eh (path->GetHeadElem (), path->GetRoot ());
    return RebarElement::IsRebarElement (eh) || IsRebarDetailSet (eh);
}



bool PITSelectTwinRebarTool::_OnModifyComplete(DgnButtonEventCR ev)
{
	if (GetElementAgenda().size() > 0 && m_TwinRebardlg != nullptr)
	{
		EditElementHandleP last = GetElementAgenda().GetLastP();
		ElementId id = last->GetElementId();
		id = 0;
		if (RebarElement::IsRebarElement(*last))
		{
			RebarElementP rebar = RebarElement::Fetch(*last);
			RebarCurve rebarCurve;
			BrString strSize;
			bvector<DPoint3d> ips;
			rebar->GetRebarCurve(rebarCurve, strSize, ACTIVEMODEL);
			rebarCurve.GetIps(ips);

			PIT::LineSegment lineMax;
			double dis = 0;
			for (int i = 0; i < (int)ips.size()-1; ++i)
			{
				if (ips[i].Distance(ips[i + 1]) > dis)
				{
					lineMax.SetLineSeg(DSegment3d::From(ips[i], ips[i + 1]));
					dis = lineMax.GetLength();
				}
			}

			m_TwinRebardlg->SetTwinRebarVec(lineMax.GetLineVec());
			m_TwinRebardlg->DrawRefLine();
		}

		_ExitTool();
	}
	return true;
}

void PITSelectTwinRebarTool::InstallTwinRebarInstance(int toolId, CTwinBarToolDlg* editdlg)
{
	PITSelectTwinRebarTool* tool = new PITSelectTwinRebarTool(toolId);
	tool->m_TwinRebardlg = editdlg;
	tool->InstallTool();
}
//////////////////////////////////////////////////////////////////////////

bool PITSelectURebarTool::_OnModifyComplete(DgnButtonEventCR ev)
{
	if (GetElementAgenda().size() == 2 && m_URebardlg != nullptr)
	{
		ElementHandle eeh_V1 = GetElementAgenda().at(0);
		ElementHandle eeh_V2 = GetElementAgenda().at(1);
		if (RebarElement::IsRebarElement(eeh_V1) && RebarElement::IsRebarElement(eeh_V2))
		{
			m_URebardlg->SetElementId_V1(eeh_V1.GetElementId());
			m_URebardlg->SetElementId_V2(eeh_V2.GetElementId());
			m_URebardlg->DrawRefLine();
		}

		_ExitTool();
	}
	else if (GetElementAgenda().size() == 1 && m_URebardlg != nullptr)
	{
		ElementHandle eeh_V1 = GetElementAgenda().at(0);
		if (RebarElement::IsRebarElement(eeh_V1))
		{
			EditElementHandle rebarEeh(eeh_V1.GetElementId(), eeh_V1.GetDgnModelP());
			DPoint3d ptstr, ptend;
			double diameter = 0;
			GetStartEndPointFromRebar(&rebarEeh, ptstr, ptend, diameter);
			if (ptend.Distance(m_endRebarPt) - ptstr.Distance(m_endRebarPt) < 1)
			{
				m_URebardlg->SetIsEnd(true);
			}
			m_URebardlg->SetIsLRebar(true);
			m_URebardlg->DrawRefLine();
		}

		_ExitTool();
	}
	return true;
}

BENTLEY_NAMESPACE_NAME::EditElementHandleP PITSelectURebarTool::_BuildLocateAgenda(HitPathCP path, DgnButtonEventCP ev)
{
	EditElementHandleP eh = __super::_BuildLocateAgenda(path, ev);
	mdlHitPath_getHitPoint(path, m_endRebarPt);
	return eh;
}

void PITSelectURebarTool::InstallURebarInstance(int toolId, CURebarToolDlg * editdlg)
{
	PITSelectURebarTool* tool = new PITSelectURebarTool(toolId);
	tool->m_URebardlg = editdlg;
	tool->InstallTool();
}

bool PITSelectStirrupRebarTool::_OnModifyComplete(DgnButtonEventCR ev)
{
	if (GetElementAgenda().size() > 3 && m_StirrupRebardlg != nullptr)
	{
		ElementHandle eeh_V1 = GetElementAgenda().at(0);
		ElementHandle eeh_V2 = GetElementAgenda().at(1);
		ElementHandle eeh_V3 = GetElementAgenda().at(2);
		ElementHandle eeh_V4 = GetElementAgenda().at(3);
		if (RebarElement::IsRebarElement(eeh_V1) && RebarElement::IsRebarElement(eeh_V2) && 
			RebarElement::IsRebarElement(eeh_V3) && RebarElement::IsRebarElement(eeh_V4))
		{
			m_StirrupRebardlg->SetElementId_Vs(std::vector<ElementId> {eeh_V1.GetElementId(), eeh_V2.GetElementId(), eeh_V3.GetElementId(), eeh_V4.GetElementId()});
			m_StirrupRebardlg->DrawRefLine();
		}

		_ExitTool();
	}
	return true;
}

void PITSelectStirrupRebarTool::InstallStirrupRebarInstance(int toolId, CStirrupToolDlg * editdlg)
{
	PITSelectStirrupRebarTool* tool = new PITSelectStirrupRebarTool(toolId);
	tool->m_StirrupRebardlg = editdlg;
	tool->InstallTool();
}


bool PITStretchStirrupRebarTool::_OnDataButton(DgnButtonEventCR ev)
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
	m_StretchStirrupRebardlg->SetEditLinePoint(pointStart, pointEnd);
}

bool PITStretchStirrupRebarTool::_OnPostLocate(HitPathCP path, WStringR cantAcceptReason)
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

void PITStretchStirrupRebarTool::InstallStretchStirrupRebarInstance(int toolId, CStretchStirrupRebarToolDlg* editdlg)
{
	PITStretchStirrupRebarTool* tool = new PITStretchStirrupRebarTool(toolId);
	tool->m_StretchStirrupRebardlg = editdlg;
	tool->InstallTool();
}
