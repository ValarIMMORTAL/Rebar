/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/SelectRebarTool.cpp $
|
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include "SelectRebarTool.h"
#include "ExtractFacesTool.h"
//SelectRebarTool::SelectRebarTool (int toolId) : DgnElementSetTool (toolId)
//    {
//	  m_editdlg = nullptr;
//	  m_combinerebardlg = nullptr;
//	  m_addverticaldlg = nullptr;
//	  m_modifyRebarToolDlg = nullptr;
//    }
//
//bool SelectRebarTool::_FilterAgendaEntries ()
//    {
//    bool                changed = false;
//    EditElementHandleP  start = GetElementAgenda ().GetFirstP ();
//    EditElementHandleP  end = start + GetElementAgenda ().GetCount ();
//
//    for (EditElementHandleP curr = start; curr < end; curr++)
//        {
//        if (!(RebarElement::IsRebarElement (*curr) || IsRebarDetailSet (*curr)))
//            {
//            curr->Invalidate ();
//            changed = true;
//            }
//        }
//    return changed;
//    }
//
//bool SelectRebarTool::_OnPostLocate (HitPathCP path, WStringR cantAcceptReason)
//    {
//    if (!__super::_OnPostLocate (path, cantAcceptReason))
//        return false;
//
//    ElementHandle eh (path->GetHeadElem (), path->GetRoot ());
//    return RebarElement::IsRebarElement (eh) || IsRebarDetailSet (eh);
//    }
//
//void SelectRebarTool::_SetupAndPromptForNextAction ()
//    {
//       SetupForLocate ();
//    }
//
//void SelectRebarTool::_OnRestartTool ()
//    {
//	     _ExitTool();
//    }
//
//BENTLEY_NAMESPACE_NAME::EditElementHandleP SelectRebarTool::_BuildLocateAgenda(HitPathCP path, DgnButtonEventCP ev)
//{
//	EditElementHandleP eh = __super::_BuildLocateAgenda(path, ev);
//	mdlHitPath_getHitPoint(path, m_endRebarPt);
//	return eh;
//}
//
//bool SelectRebarTool::_OnModifyComplete(DgnButtonEventCR ev)
//{
//	if (GetElementAgenda().size() == 3 && m_editdlg != nullptr)//����Ҫѡ��3���ֽǰ����ΪҪê�뵽�ĸֽ���һ��Ϊ�ж϶˲��ֽ�
//	{
//		m_editdlg->m_Anchorinrebars.clear();
//		m_editdlg->SetIsArc(false);
//		EditElementHandleP start = GetElementAgenda().GetFirstP();
//		EditElementHandleP last = start + GetElementAgenda().GetCount();
//		for (EditElementHandleP elementToModify = start; elementToModify < last; elementToModify++)
//		{
//			if (RebarElement::IsRebarElement(*elementToModify))
//			{
//				if (elementToModify == GetElementAgenda().GetLastP())
//				{
//					DPoint3d ptstr, ptend;
//					double diameter = 0;
//					GetStartEndPointFromRebar(elementToModify, ptstr, ptend, diameter);
//					if (ptend.Distance(m_endRebarPt) - ptstr.Distance(m_endRebarPt) < 1)
//					{
//						m_editdlg->SetIsEnd(true);
//					}
//				}
//				else
//				{
//					m_editdlg->m_Anchorinrebars.push_back(elementToModify->GetElementRef());
//				}
//				
//			}
//		}
//		
//		m_editdlg->ShowWindow(SW_SHOW);
//		m_editdlg->CalcWallHoles();
//		m_editdlg->UpdateDataAndWindow();
//		_ExitTool();
//	}
//	if (GetElementAgenda().size() == 4 && m_editdlg != nullptr)//����Ҫѡ��4���ֽǰ3��ΪҪê�뵽�ĸֽ�(����)�����һ��Ϊ�ж϶˲��ֽ�
//	{
//		m_editdlg->m_Anchorinrebars.clear();
//		m_editdlg->SetIsArc(true);
//		EditElementHandleP start = GetElementAgenda().GetFirstP();
//		EditElementHandleP last = start + GetElementAgenda().GetCount();
//		for (EditElementHandleP elementToModify = start; elementToModify < last; elementToModify++)
//		{
//			if (RebarElement::IsRebarElement(*elementToModify))
//			{
//				if (elementToModify == GetElementAgenda().GetLastP())
//				{
//					DPoint3d ptstr, ptend;
//					double diameter = 0;
//					GetStartEndPointFromRebar(elementToModify, ptstr, ptend, diameter);
//					if (ptend.Distance(m_endRebarPt) - ptstr.Distance(m_endRebarPt) < 1)
//					{
//						m_editdlg->SetIsEnd(true);
//					}
//				}
//				else
//				{
//					m_editdlg->m_Anchorinrebars.push_back(elementToModify->GetElementRef());
//				}
//
//			}
//		}
//
//		m_editdlg->ShowWindow(SW_SHOW);
//		m_editdlg->CalcWallHoles();
//		m_editdlg->UpdateDataAndWindow();
//		_ExitTool();
//	}
//	if (GetElementAgenda().size() > 1 && m_combinerebardlg != nullptr)
//	{
//		m_combinerebardlg->m_Verticalrebars.clear();
//		EditElementHandleP start = GetElementAgenda().GetFirstP();
//		EditElementHandleP last = start + GetElementAgenda().GetCount();
//		for (EditElementHandleP elementToModify = start; elementToModify < last; elementToModify++)
//		{
//			if (RebarElement::IsRebarElement(*elementToModify))
//			{
//				m_combinerebardlg->m_Verticalrebars.push_back(elementToModify->GetElementRef());
//			}
//
//		}
//
//		m_combinerebardlg->ShowWindow(SW_SHOW);
//		m_combinerebardlg->UpdateDataAndWindow();
//		_ExitTool();
//	}
//	if (GetElementAgenda().size() > 0 && m_addverticaldlg != nullptr)//����Ҫѡ��1���ֽ�
//	{
//		m_addverticaldlg->m_setdata.refdotrebar.clear();
//		EditElementHandleP start = GetElementAgenda().GetFirstP();
//		EditElementHandleP last = start + GetElementAgenda().GetCount();
//		for (EditElementHandleP elementToModify = start; elementToModify < last; elementToModify++)
//		{
//			if (RebarElement::IsRebarElement(*elementToModify))
//			{
//				m_addverticaldlg->m_setdata.refdotrebar.push_back(elementToModify->GetElementRef());
//			}
//
//		}
//
//		m_addverticaldlg->ShowWindow(SW_SHOW);
//		m_addverticaldlg->CalculateRefRebarPoints();
//		m_addverticaldlg->CalculateVertexAndDrawLines();
//		_ExitTool();
//	}
//	if (m_modifyRebarToolDlg != nullptr)
//	{
//		vector<ElementRefP> selectRebars;
//		EditElementHandleP start = GetElementAgenda().GetFirstP();
//		EditElementHandleP last = start + GetElementAgenda().GetCount();
//		for (EditElementHandleP elementToModify = start; elementToModify < last; elementToModify++)
//		{
//			if (RebarElement::IsRebarElement(*elementToModify))
//			{
//				selectRebars.push_back(elementToModify->GetElementRef());
//			}
//
//		}
//		m_modifyRebarToolDlg->SetSelectRebars(selectRebars);
//		m_modifyRebarToolDlg->ShowWindow(SW_SHOW);
//		_ExitTool();
//	}
//	return true;
//}
//
//size_t SelectRebarTool::_GetAdditionalLocateNumRequired()
//{
//	if (m_addverticaldlg != nullptr || m_combinerebardlg != nullptr)
//	{
//		return 2;
//	}
//	else
//	{
//		return 0;
//	}
//	//if (m_editdlg != nullptr)
//	//{
//	//	return 3;
//	//}
//	//else if (m_modifyRebarToolDlg != nullptr)
//	//{
//	//	return 0;
//	//}
//	//else
//	//{
//	//	return 2;
//	//}
//}
//
//StatusInt   SelectRebarTool::_OnElementModify(EditElementHandleR el)
//{
//	
//	return ERROR;
//}
//void SelectRebarTool::SetupForLocate ()
//    {
//    UInt32 msgId = PROMPT_AcceptOrReject;
//    bool doLocate = true;
//
//    if (SOURCE_SelectionSet == _GetElemSource ())
//        {
//        msgId = PROMPT_AcceptSelectionSet;
//        doLocate = false;
//        }
//    else if (GetElementAgenda ().GetCount () < 1)
//        {
//        msgId = PROMPT_SelectRebarElement;
//        doLocate = true;
//        }
//    else
//        {
//        msgId = PROMPT_AcceptOrReject;
//        doLocate = false;
//        }
//
//    AccuSnap::GetInstance ().EnableSnap (!doLocate);
//    _SetLocateCursor (doLocate);
//
//    mdlOutput_rscPrintf (MSG_PROMPT, NULL, STRINGLISTID_RebarSDKExampleTextMessages, msgId);
//    }
//
//void SelectRebarTool::InstallNewInstance (int toolId, CRebarEditDlg* editdlg)
//    {
//    SelectRebarTool* tool = new SelectRebarTool (toolId);
//	tool->m_editdlg = editdlg;
//    tool->InstallTool ();
//    }
//void SelectRebarTool::InstallNewInstance2(int toolId, CCombineRebardlg* editdlg)
//{
//	SelectRebarTool* tool = new SelectRebarTool(toolId);
//	tool->m_combinerebardlg = editdlg;
//	tool->InstallTool();
//}
//void  SelectRebarTool::InstallNewInstanceVertical(int toolId, CAddVerticalRebarDlg* editdlg)
//{
//	SelectRebarTool* tool = new SelectRebarTool(toolId);
//	tool->m_addverticaldlg = editdlg;
//	tool->InstallTool();
//}
//
//void SelectRebarTool::InstallNewInstanceModifyDlg(int toolId, ModifyRebarToolDlg* editdlg)
//{
//	auto& ssm = SelectionSetManager::GetManager();
//	ssm.EmptyAll();
//	SelectRebarTool* tool = new SelectRebarTool(toolId);
//	tool->m_modifyRebarToolDlg = editdlg;
//	tool->InstallTool();
//}
//
////////////////////////////////////////////////////////////////////////////
//
//SelectLineTool::SelectLineTool()
//{
//
//}
//
//void SelectLineTool::_OnRestartTool()
//{
//	return;
//}
//
//bool    SelectLineTool::_SetupForModify(DgnButtonEventCR ev, bool isDynamics)
//{
//	//DPoint3d ptOri;
//	//if (!_GetAnchorPoint(&ptOri))
//	//	return false;
//	//DPoint3d ptTranslation = *ev.GetPoint();
//	//ptTranslation.Subtract(ptOri);
//	//m_tranform.SetTranslation(ptTranslation);
//	return __super::_SetupForModify(ev, isDynamics);
//}
//
//StatusInt   SelectLineTool::_OnElementModify(EditElementHandleR el)
//{
//	/*TransformInfo tranInfo = TransformInfo(m_tranform);
//	el.GetHandler().ApplyTransform(el, tranInfo);*/
//	return SUCCESS;
//}
//bool    SelectLineTool::_OnModifyComplete(DgnButtonEventCR ev)
//{
//	ElementAgenda::iterator iter = GetElementAgenda().begin();
//	EditElementHandle eleLine(*iter, ACTIVEMODEL);
//	////��һ��������
//	if (GetElementAgenda().size() > 0 && m_addverticaldlg != nullptr)//����Ҫѡ��1���ֽ�
//	{
//		m_addverticaldlg->m_setdata.refline = eleLine.GetElementRef();
//		m_addverticaldlg->ShowWindow(SW_SHOW);
//		m_addverticaldlg->CalculateVertexAndDrawLines();
//	}
//	__super::_ExitTool();
//	return true;
//}
//void SelectLineTool::InstallNewInstance(CAddVerticalRebarDlg* editdlg)
//{
//	if (editdlg !=nullptr)
//	{
//		SelectLineTool* newToolP = new SelectLineTool();
//		newToolP->m_addverticaldlg = editdlg;
//		newToolP->InstallTool();
//	}
//	
//}
//
//bool    SelectLineTool::_WantAdditionalLocate(DgnButtonEventCP ev)
//{
//	if (NULL == ev)
//		return true;
//	if (ev->IsControlKey())
//		return true;
//	return false;
//}
//
//bool    SelectLineTool::_FilterAgendaEntries()
//{
//	bool isInvalid = false;
//	for (ElementAgenda::iterator iter = GetElementAgenda().begin(); iter != GetElementAgenda().end(); iter++)
//	{
//		if (ELLIPSE_ELM == (*iter).GetElementType())
//		{
//			(*iter).Invalidate();
//			isInvalid = true;
//		}
//	}
//	return isInvalid;
//}
//
//bool    SelectLineTool::_OnPostLocate(HitPathCP path, WStringR cantAcceptReason)
//{
//	int elementtype = path->GetHeadElem()->GetElementType();
//	if (LINE_ELM != elementtype && LINE_STRING_ELM != elementtype && ARC_ELM != elementtype)
//	{
//		cantAcceptReason = WString(L"��ѡ���߶�");
//		return false;
//	}
//	return __super::_OnPostLocate(path, cantAcceptReason);
//}
/////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//void RebarSDK_ReadRebar (CRebarEditDlg* editdlg)
//    {
//    SelectRebarTool::InstallNewInstance (CMDNAME_RebarSDKReadRebar, editdlg);
//    }

//����ֽ����߶ε������յ�����
bool GetStartEndPointFromRebar(EditElementHandleP start, DPoint3d& PtStar, DPoint3d& PtEnd, double& diameter)
{
	RebarCurve curve;
	if (start == nullptr)
	{
		return false;
	}
	if (RebarElement::IsRebarElement(*start))
	{
		RebarElementP rep = RebarElement::Fetch(*start);
		double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		RebarShape * rebarshape = rep->GetRebarShape(start->GetModelRef());
		if (rebarshape==nullptr)
		{
			return false;
		}
		rebarshape->GetRebarCurve(curve);
		BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
		diameter = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);

		CMatrix3D tmp3d(rep->GetLocation());
		curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
		curve.DoMatrix(rep->GetLocation());
		RebarVertices  vers = curve.PopVertices();

		double maxLenth = 0;
		for (int i = 0; i < vers.GetSize() - 1; i++)
		{
			RebarVertex   ver1 = vers.At(i);
			RebarVertex   ver2 = vers.At(i + 1);
			CPoint3D const&     pt1 = ver1.GetIP();
			CPoint3D const&     pt2 = ver2.GetIP();
			DPoint3d tpt1 = DPoint3d::From(pt1.x, pt1.y, pt1.z);
			DPoint3d tpt2 = DPoint3d::From(pt2.x, pt2.y, pt2.z);
			if (i == 0)
			{
				maxLenth = tpt1.Distance(tpt2);
				PtStar = tpt1;
				PtEnd = tpt2;
			}
			else if (maxLenth < tpt1.Distance(tpt2))
			{
				maxLenth = tpt1.Distance(tpt2);
				PtStar = tpt1;
				PtEnd = tpt2;
			}

		}
		return true;
	}
	else
	{
		return false;
	}
}
//����ê���ƽ�棬ƽ�����ĸ��������NormalPts,�õ���ƽ��������ֽ�ê�뵽��ƽ���ϣ�ֻ��Ҫ����ֽ��ߺʹ�ƽ��Ľ��㼴��
bool GetStartEndAndRadius(EditElementHandleP start, vector<EditElementHandleP> endEehs, double moveLenth,vector<DPoint3d>& NormalPts)
{
	if (endEehs.size() < 2)
	{
		return false;
	}
	EditElementHandleP end1 = endEehs.at(0);
	EditElementHandleP end2 = endEehs.at(1);

	double diameter = 0;
	DPoint3d ptStart;
	DPoint3d PtEnd;
	if (!GetStartEndPointFromRebar(start, ptStart, PtEnd, diameter))
	{
		return false;
	}


	double diameter1 = 0;
	DPoint3d ptStart1;
	DPoint3d PtEnd1;
	if (!GetStartEndPointFromRebar(end1, ptStart1, PtEnd1, diameter1))
	{
		return false;
	}
	double diameter2 = 0;
	DPoint3d ptStart2;
	DPoint3d PtEnd2;
	if (!GetStartEndPointFromRebar(end2, ptStart2, PtEnd2, diameter2))
	{
		return false;
	}

	DPoint3d vec1 = ptStart2 - ptStart1;
	DPoint3d vec2 = PtEnd1 - ptStart1;
	vec1.Normalize();
	vec2.Normalize();
	DPoint3d normal;
	mdlVec_crossProduct(&normal, &vec1, &vec2);

	DPoint3d project1,project2;//�㵽ƽ���ͶӰ
	mdlVec_projectPointToPlane(&project1, &ptStart, &ptStart1, &normal);
	mdlVec_projectPointToPlane(&project2, &PtEnd, &ptStart1, &normal);

	if (ptStart.Distance(project1) > PtEnd.Distance(project2))//�����ý�ʱ
	{
		ptStart = PtEnd;
	}
	DPoint3d ptProject1;	//ͶӰ��
	mdlVec_projectPointToLine(&ptProject1, NULL, &ptStart, &ptStart1, &PtEnd1);
	DPoint3d vecStart = ptStart - ptProject1;
	vecStart.Normalize();
	if (vecStart.DotProduct(normal) < 0)
	{
		normal.Scale(-moveLenth);
	}
	else
	{
		normal.Scale(moveLenth);
	}

	mdlVec_addPoint(&ptStart1, &ptStart1, &normal);
	mdlVec_addPoint(&PtEnd1, &PtEnd1, &normal);
	mdlVec_addPoint(&ptStart2, &ptStart2, &normal);
	mdlVec_addPoint(&PtEnd2, &PtEnd2, &normal);


	if (ptStart1.Distance(ptStart2) > ptStart1.Distance(PtEnd2))
	{
		DPoint3d tmpPt = ptStart2;
		ptStart2 = PtEnd2;
		PtEnd2 = tmpPt;
	}
	NormalPts.push_back(ptStart1);
	NormalPts.push_back(PtEnd1);
	NormalPts.push_back(PtEnd2);
	NormalPts.push_back(ptStart2);

	for (size_t i = 2; i < endEehs.size(); ++i)
	{
		double tmpDia = 0;
		DPoint3d tmpStrPt = {0,0,0};
		DPoint3d tmpEndPt = {0,0,0};
		if (!GetStartEndPointFromRebar(endEehs.at(i), tmpStrPt, tmpEndPt, tmpDia))
		{
			continue;
		}
		mdlVec_addPoint(&tmpStrPt, &tmpStrPt, &normal);
		mdlVec_addPoint(&tmpEndPt, &tmpEndPt, &normal);
		if (ptStart1.Distance(tmpStrPt) > ptStart1.Distance(tmpEndPt))
		{
			DPoint3d tmpPt = tmpStrPt;
			tmpStrPt = tmpEndPt;
			tmpEndPt = tmpPt;
		}
		NormalPts.push_back(tmpStrPt);
		NormalPts.push_back(tmpEndPt);
	}
	return true;
}

//ͨ�������ε������˵㣬�õ����л��Σ��м��ΪPTB
int IntersectionPointToArcDataRebar(RebarArcData &arcInfo, DPoint3d ptA, DPoint3d ptB, DPoint3d ptC, double dRadius)
{

	DVec3d dirAB, dirAC;
	mdlVec_subtractPoint(&dirAB, &ptA, &ptB);
	mdlVec_subtractPoint(&dirAC, &ptA, &ptC);

	DVec3d dirABBack = dirAB;
	DVec3d dirACBack = dirAC;


	dirABBack.Normalize();
	dirACBack.Normalize();

	double ddotProduct = mdlVec_dotProduct(&dirABBack, &dirACBack);
	double dAngle = acos(ddotProduct);

	double dRcenterA = dRadius / (sin(dAngle / 2));


	DPoint3d ptO;
	constIntersectionPoint(&ptO, &dRcenterA, &ptA, &ptB);
	
	DVec3d ABCNormal;
	mdlVec_crossProduct(&ABCNormal, &dirAB, &dirAC);


	//���ԽǶ�ΪA/2��ƽ�淨������ת���������Ƶ�Բ����Բ��
	EditElementHandle eelh;
	LineHandler::CreateLineElement(eelh, NULL, DSegment3d::From(ptA, ptO), true, *ACTIVEMODEL);
	MSElementDescr *peldsc = eelh.ExtractElementDescr();
	Transform trans;
	mdlCurrTrans_begin();
	mdlCurrTrans_identity();
	mdlTMatrix_getIdentity(&trans);
	mdlTMatrix_fromRotationAroundPointAndVector(&trans, &ptA, &ABCNormal, dAngle / 2);
	mdlElmdscr_transform(&peldsc, &trans);
	mdlCurrTrans_end();

	//��ȡ��ת����ߵ��յ㼴Բ����Բ��
	DPoint3d ptCenter;
	mdlElmdscr_extractEndPoints(NULL, NULL, &ptCenter, NULL, peldsc, ACTIVEMODEL);
	mdlElmdscr_freeAll(&peldsc);
	arcInfo.ptArcCenter = ptCenter;
	arcInfo.dArcRadius = dRadius;

	//�ֱ����AB,AC���Բ�ĵĽ���
	double dDistance = dRadius / (tan(dAngle / 2));

	double dAB = mdlVec_distance(&ptA, &ptB);
	arcInfo.ptArcBegin.x = (ptB.x - ptA.x) / dAB * dDistance + ptA.x;
	arcInfo.ptArcBegin.y = (ptB.y - ptA.y) / dAB * dDistance + ptA.y;
	arcInfo.ptArcBegin.z = (ptB.z - ptA.z) / dAB * dDistance + ptA.z;				//AB��Բ�Ľ���

	double dAC = mdlVec_distance(&ptA, &ptC);
	arcInfo.ptArcEnd.x = (ptC.x - ptA.x) / dAC * dDistance + ptA.x;
	arcInfo.ptArcEnd.y = (ptC.y - ptA.y) / dAC * dDistance + ptA.y;
	arcInfo.ptArcEnd.z = (ptC.z - ptA.z) / dAC * dDistance + ptA.z;;				//AC��Բ�Ľ���



	DPoint3d midPt = arcInfo.ptArcBegin;
	midPt.Add(arcInfo.ptArcEnd);
	midPt.Scale(0.5);

	DPoint3d vecDiameter = midPt - arcInfo.ptArcCenter;
	vecDiameter.Normalize();
	vecDiameter.Scale(dRadius);
	mdlVec_addPoint(&arcInfo.ptArcMid, &arcInfo.ptArcCenter, &vecDiameter);

	return SUCCESS;
}
//���㻡�κ�֮��Ľ��㣬ͨ���������¼���Բ���������յ㣬��ֹ�ڽ��㴦ת��ʱ�����θֽ���һ��
void GetArcRebarVertexIntersetLine(RebarVertex& ver,DPoint3d& linestr,DPoint3d lineend, DPoint3d vecNormal,double bendRadius)
{
	double radius = ver.GetRadius();
	DPoint3d centerpt = ver.GetCenter();
	DPoint3d ptStr = ver.GetArcPt(0);
	DPoint3d ptEnd = ver.GetArcPt(2);
	DPoint3d ptMid = ver.GetArcPt(1);

	//����ֱ���뻡�Ľ���
	EditElementHandle arceeh;
	ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(ptStr, ptMid, ptEnd), true, *ACTIVEMODEL);
	EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(linestr, lineend), true, *ACTIVEMODEL);
	DPoint3d arcinter[2];
	mdlIntersect_allBetweenExtendedElms(arcinter, nullptr, 2, arceeh.GetElementDescrP(), eehline.GetElementDescrP(), nullptr, 0.1, &ptEnd, &linestr);
	//������ֱ�ߵĽ���
	DPoint3d lastarcinter;
	if (linestr.Distance(arcinter[0]) < linestr.Distance(arcinter[1]))
	{
		lastarcinter = arcinter[0];
	}
	else
	{
		lastarcinter = arcinter[1];
	}
	linestr = lastarcinter;
	mdlVec_addPoint(&lineend, &linestr, &vecNormal);

	//ƫ��bendradius�����ֱ���뻡�Ľ���
	DPoint3d vecZ = DPoint3d::From(0, 0, 1);
	DPoint3d vecDis = vecNormal;
	vecDis.Normalize();
	vecDis.CrossProduct(vecDis, vecZ);
	
	DPoint3d tmpMid = ptStr;
	tmpMid.Add(ptEnd);
	tmpMid.Scale(0.5);
	DPoint3d vectmp = tmpMid - lastarcinter;
	vectmp.Normalize();
	if (vectmp.DotProduct(vecDis)<0)//���ƫ�Ʒ������е㲻��ͬһ�࣬����
	{
		vecDis.Scale(-1);
	}
	vecDis.Scale(bendRadius+100);
	mdlVec_addPoint(&lastarcinter, &lastarcinter, &vecDis);
	DPoint3d lastinter2;
	mdlVec_addPoint(&lastinter2, &lastarcinter, &vecNormal);
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(lastarcinter, lastinter2), true, *ACTIVEMODEL);
	mdlIntersect_allBetweenExtendedElms(arcinter, nullptr, 2, arceeh.GetElementDescrP(), eehline.GetElementDescrP(), nullptr, 0.1, &ptEnd, &linestr);
	if (linestr.Distance(arcinter[0]) < linestr.Distance(arcinter[1]))
	{
		lastarcinter = arcinter[0];
	}
	else
	{
		lastarcinter = arcinter[1];
	}

	//���¼���RebarVertex��ֵ
	if (lastarcinter.Distance(ptStr) > lastarcinter.Distance(ptEnd))
	{
		ptMid = ptStr;
		ptMid.Add(lastarcinter);
		ptMid.Scale(0.5);
		ptEnd = lastarcinter;
	}
	else
	{
		ptMid = ptEnd;
		ptMid.Add(lastarcinter);
		ptMid.Scale(0.5);
		ptStr = lastarcinter;
	}

	double dis = ptMid.Distance(centerpt);
	//tmpipΪ���¼�����IP��
	DPoint3d tmpIp = ptMid;
	tmpIp = tmpIp - centerpt;
	tmpIp.Normalize();
	tmpIp.Scale(radius*radius / dis);
	tmpIp.Add(centerpt);

	ptMid = ptMid - centerpt;
	ptMid.Normalize();
	ptMid.Scale(radius);
	ptMid.Add(centerpt);

	ver.SetIP(tmpIp);
	ver.SetArcPt(0, ptStr);
	ver.SetArcPt(1, ptMid);
	ver.SetArcPt(2, ptEnd);
}

void MoveRebarPointByNormal(RebarPoint& tmppt,double movdis,DPoint3d vecNormal)
{
	if (vecNormal.IsEqual(DPoint3d::From(0,0,0)))//���û�д���ƫ�Ʒ��򣬼���ֱ�ߵķ��࣬�ط������ƶ�
	{
		DPoint3d tmpVec = tmppt.ptend - tmppt.ptstr;
		tmpVec.Normalize();
		DPoint3d vecZ = DPoint3d::From(0, 0, 1);
		vecNormal.CrossProduct(vecZ, tmpVec);
		vecNormal.Normalize();
	}
	vecNormal.Scale(movdis);
	mdlVec_addPoint(&tmppt.ptstr, &tmppt.ptstr, &vecNormal);
	mdlVec_addPoint(&tmppt.ptend, &tmppt.ptend, &vecNormal);
}

//ֱêʱ����ê�뵽ƽ��NormalPts��ĸֽ�ϲ���RebarVertices
bool GetRebarVerticesWhenStraightLenth(RebarVertices&  vers, RebarCurve& curve, vector<DPoint3d>& NormalPts, double L0Lenth, bool isEnd)
{
	if (vers.GetSize() < 1)
	{
		return false;
	}
	CPoint3D rebarstar, rebarend;
	curve.GetEndPoints(rebarstar, rebarend);

	DPoint3d ptss[4];
	ptss[0] = NormalPts[0];
	ptss[1] = NormalPts[1];
	ptss[2] = NormalPts[2];
	ptss[3] = NormalPts[3];
	DPoint3d vec1 = ptss[1] - ptss[0];
	DPoint3d vec2 = ptss[2] - ptss[1];
	vec1.Normalize();
	vec2.Normalize();
	DPoint3d normal;
	mdlVec_crossProduct(&normal, &vec2, &vec1);


	DPoint3d project1, project2;//�㵽ƽ���ͶӰ
	mdlVec_projectPointToPlane(&project1, &rebarstar, &ptss[0], &normal);
	mdlVec_projectPointToPlane(&project2, &rebarend, &ptss[0], &normal);


	if (isEnd/*rebarstar.Distance(project1) < rebarend.Distance(project2)*/)//��������Ͻ�����Ҫ����
	{
		vers.Reverse();
		for (int i = 0; i < vers.GetSize(); i++)
		{
			RebarVertex*   ver1 = vers.GetAt(i);
			CPoint3D  pt1 = ver1->PopArcPt(0);
			ver1->SetArcPt(0, ver1->GetArcPt(2));
			ver1->SetArcPt(1, ver1->GetArcPt(1));
			ver1->SetArcPt(2, pt1);
			vers.SetAt(i, *ver1);
		}
		vers.At(0).SetType(RebarVertex::kStart);
		vers.At(vers.GetSize() - 1).SetType(RebarVertex::kEnd);
		curve.SetVertices(vers);
	}

	CPoint3D pt;
	CVector3D vectmp;
	curve.GetLastPoint(pt, vectmp);

	DPoint3d strpt(pt);
	DPoint3d endpt;//�˵�Ϊĩβ���ӳ����빹���ê��ƽ��Ľ���
	vectmp.Normalize();
	vectmp.Scale(100);
	mdlVec_addPoint(&endpt, &strpt, &vectmp);
	vectmp.Normalize();
	vectmp.Scale(-1);
	GetIntersectedPointBtwPlaneAndLine(&endpt, ptss, strpt, endpt);
	double MinLenth = endpt.Distance(strpt);
	if (MinLenth<L0Lenth)
	{
		vectmp.Scale(MinLenth);
	}
	else
	{
		vectmp.Scale(L0Lenth);
	}
	mdlVec_addPoint(&endpt, &strpt, &vectmp);
	vers.At(vers.GetSize() - 1).SetIP(endpt);

	return true;
}


//ͨ���ֽ���������curve,����ê�뵽ƽ��NormalPts��ĸֽ�ϲ���RebarVertices
bool GetRebarVertices(RebarVertices&  vers, RebarCurve& curve, vector<DPoint3d>& NormalPts, 
	double bendRadius, double L0Lenth,double rotateangle, bool isEnd)
{
	if (vers.GetSize() < 1)
	{
		return false;
	}
	if (isEnd/*rebarstar.Distance(project1) < rebarend.Distance(project2)*/)//��������Ͻ�����Ҫ����
	{
		vers.Reverse();
		for (int i = 0; i < vers.GetSize(); i++)
		{
			RebarVertex*   ver1 = vers.GetAt(i);
			CPoint3D  pt1 = ver1->PopArcPt(0);
			ver1->SetArcPt(0, ver1->GetArcPt(2));
			ver1->SetArcPt(1, ver1->GetArcPt(1));
			ver1->SetArcPt(2, pt1);
			vers.SetAt(i, *ver1);
		}
		vers.At(0).SetType(RebarVertex::kStart);
		vers.At(vers.GetSize() - 1).SetType(RebarVertex::kEnd);
		curve.SetVertices(vers);
	}

	CPoint3D rebarstar, rebarend;
	curve.GetEndPoints(rebarstar, rebarend);

	DPoint3d ptss[4];
	ptss[0] = NormalPts[0];
	ptss[1] = NormalPts[1];
	ptss[2] = NormalPts[2];
	ptss[3] = NormalPts[3];
	DPoint3d vec1 = ptss[1] - ptss[0];
	DPoint3d vec2 = ptss[2] - ptss[1];
	vec1.Normalize();
	vec2.Normalize();
	DPoint3d normal;
	mdlVec_crossProduct(&normal, &vec2, &vec1);


	DPoint3d project1, project2;//�㵽ƽ���ͶӰ
	mdlVec_projectPointToPlane(&project1, &rebarstar, &ptss[0], &normal);
	mdlVec_projectPointToPlane(&project2, &rebarend, &ptss[0], &normal);

	
	

	CPoint3D pt;
	CVector3D vectmp;
	curve.GetLastPoint(pt, vectmp);

	DPoint3d strpt(pt);
	DPoint3d endpt;//�˵�Ϊĩβ���ӳ����빹���ê��ƽ��Ľ���
	vectmp.Normalize();
	vectmp.Scale(100);
	mdlVec_addPoint(&endpt, &strpt, &vectmp);
	
	GetIntersectedPointBtwPlaneAndLine(&endpt, ptss, strpt, endpt);
	bool istr = false;
	bool tmpstr = false;
	if (ExtractFacesTool::IsPointInLine(endpt, vers.At(vers.GetSize() - 1).GetIP(),vers.At(vers.GetSize() - 2).GetIP(),ACTIVEMODEL, tmpstr)==true)
	{
		istr = true;
	}

	vectmp = endpt - strpt;

	DPoint3d vecNormal;//�˷���Ϊê������һ�����۵ķ���
	vecNormal = NormalPts[0] - NormalPts[1];
	vecNormal.Normalize();
	vectmp.Normalize();
	Transform tmptrans;
	tmptrans.InitIdentity();
	double tmpA = rotateangle / 180.0 * PI;
	CVector3D tmpNorvec = normal;
	CVector3D tmpVec = vecNormal;
	tmpVec.Rotate(tmpA, tmpNorvec);
	/*mdlTMatrix_fromRotationAroundPointAndVector(&tmptrans, &endpt, &normal, tmpA);
	mdlTMatrix_transformPoint(&vecNormal, &tmptrans);
	vecNormal.Normalize();*/
	vecNormal = tmpVec;
	vecNormal.Normalize();
	vecNormal.Scale(L0Lenth);
	/*if (vecNormal.DotProduct(vectmp) > 0)
	{
		vecNormal.Scale(-1);
	}
	vecNormal.Scale(L0Lenth);
	if (!isFront)
	{
		vecNormal.Scale(-1);
	}
	if (isrotate)
	{
		vecNormal.CrossProduct(vecNormal, normal);
	}*/
	DPoint3d endpt2;
	mdlVec_addPoint(&endpt2, &endpt, &vecNormal);
	if (istr)//������������ϣ����ж����ǰһ����Ϊ���ߣ����¼��㻡��IP�����ĵ���յ�
	{
		if (vers.At(vers.GetSize() - 2).GetRadius()>0&&vers.At(vers.GetSize() - 2).GetRadius()>bendRadius+10)//�ǻ���
		{
			GetArcRebarVertexIntersetLine(vers.At(vers.GetSize() - 2), endpt, endpt2, vecNormal, bendRadius);
			RebarVertex*   vertmp;
			vertmp = &vers.At(vers.GetSize() - 1);
			vertmp->SetType(RebarVertex::kIP);
			vertmp->SetIP(vers.At(vers.GetSize() - 2).GetArcPt(2));
			if (vers.GetSize() - 3>=0&&vers.GetSize()-3<vers.GetSize()-2)
			{
				RebarVertex*   vertmp2;
				vertmp2 = &vers.At(vers.GetSize() - 3);
				vertmp2->SetIP(vers.At(vers.GetSize() - 2).GetArcPt(0));
			}
		}
	}

	strpt = vers.At(vers.GetSize() - 1).GetIP();
	if (vers.GetSize()>1)//2��������ʱ��strptҪ�жϽ����Ƿ�������KIP��֮�䣬�����֮��Ҫȡvers.GetSize() - 1��ǰһ����
	{
		bool isStr;
		DPoint3d tmppt = vers.At(vers.GetSize() - 2).GetIP();
		if (ExtractFacesTool::IsPointInLine(endpt, tmppt, strpt, ACTIVEMODEL, isStr))
		{
			strpt = tmppt;
			//if (isStr)//������ڶ�����Ͻ������㻡ʱҪȡǰһ����
			//{
			//	strpt = tmppt;
			//}
		}
	}
	//ͨ��3���˵���㻡��������ͻ���Բ�ĵ㣬��ͨ���õ��Ļ�����RebarVertex
	RebarArcData arcdata;
	IntersectionPointToArcDataRebar(arcdata, endpt, strpt, endpt2, bendRadius);
	RebarVertex*   vertmp;
	if (vers.GetSize() == 2)
	{
		vertmp = &vers.At(vers.GetSize() - 1);
		vertmp->SetType(RebarVertex::kIP);
		vertmp->SetIP(endpt);
		vertmp->SetArcPt(0, arcdata.ptArcBegin);
		vertmp->SetArcPt(1, arcdata.ptArcMid);
		vertmp->SetArcPt(2, arcdata.ptArcEnd);
		vertmp->SetRadius(bendRadius);
		vertmp->SetCenter(arcdata.ptArcCenter);
	}
	else
	{
		vertmp = new RebarVertex();
		vertmp->SetType(RebarVertex::kIP);
		vertmp->SetIP(endpt);
		vertmp->SetArcPt(0, arcdata.ptArcBegin);
		vertmp->SetArcPt(1, arcdata.ptArcMid);
		vertmp->SetArcPt(2, arcdata.ptArcEnd);
		vertmp->SetRadius(bendRadius);
		vertmp->SetCenter(arcdata.ptArcCenter);
		vers.Add(vertmp);
	}
	RebarVertex*   verend = new RebarVertex();
	verend->SetIP(endpt2);
	verend->SetType(RebarVertex::kEnd);
	vers.Add(verend);
	return true;
}

bool GetArcRebarVertices(RebarVertices& vers, RebarCurve& curve, vector<DPoint3d>& NormalPts, double bendRadius, double L0Lenth, double rotateangle, bool isEnd)
{
	if (vers.GetSize() < 1 || NormalPts.size() < 6)
	{
		return false;
	}
	if (isEnd)//��������Ͻ�����Ҫ����
	{
		vers.Reverse();
		for (int i = 0; i < vers.GetSize(); i++)
		{
			RebarVertex*   ver1 = vers.GetAt(i);
			CPoint3D  pt1 = ver1->PopArcPt(0);
			ver1->SetArcPt(0, ver1->GetArcPt(2));
			ver1->SetArcPt(1, ver1->GetArcPt(1));
			ver1->SetArcPt(2, pt1);
			vers.SetAt(i, *ver1);
		}
		vers.At(0).SetType(RebarVertex::kStart);
		vers.At(vers.GetSize() - 1).SetType(RebarVertex::kEnd);
		curve.SetVertices(vers);
	}

	CPoint3D rebarstar, rebarend;
	curve.GetEndPoints(rebarstar, rebarend);

	DPoint3d ptss[4];
	ptss[0] = NormalPts[0];
	ptss[1] = NormalPts[1];
	ptss[2] = NormalPts[2];
	ptss[3] = NormalPts[3];
	DPoint3d vec1 = ptss[1] - ptss[0];
	DPoint3d vec2 = ptss[2] - ptss[1];
	vec1.Normalize();
	vec2.Normalize();
	DPoint3d normal;
	mdlVec_crossProduct(&normal, &vec2, &vec1);

	DPoint3d project1, project2;//�㵽ƽ���ͶӰ
	mdlVec_projectPointToPlane(&project1, &rebarstar, &ptss[0], &normal);
	mdlVec_projectPointToPlane(&project2, &rebarend, &ptss[0], &normal);

	CPoint3D pt;
	CVector3D vectmp;
	curve.GetLastPoint(pt, vectmp);

	DPoint3d strpt(pt);
	DPoint3d endpt;//�˵�Ϊĩβ���ӳ����빹���ê��ƽ��Ľ���
	vectmp.Normalize();
	vectmp.Scale(100);
	mdlVec_addPoint(&endpt, &strpt, &vectmp);

	GetIntersectedPointBtwPlaneAndLine(&endpt, ptss, strpt, endpt);
	//����ѡ�е�3���ֽ�ȷ��һ�λ�
	DPoint3d arcStrPt = NormalPts.at(0);
	DPoint3d arcMidPt = NormalPts.at(2);
	DPoint3d arcEndPt = NormalPts.at(4);
	mdlVec_projectPointToPlane(&arcStrPt, &arcStrPt, vers.At(0).GetIP(), &vec1);
	mdlVec_projectPointToPlane(&arcMidPt, &arcMidPt, vers.At(0).GetIP(), &vec1);
	mdlVec_projectPointToPlane(&arcEndPt, &arcEndPt, vers.At(0).GetIP(), &vec1);
	DEllipse3d ellipse = DEllipse3d::FromPointsOnArc(arcStrPt, arcMidPt, arcEndPt);
	EditElementHandle arcEeh;
	ArcHandler::CreateArcElement(arcEeh, nullptr, ellipse, true, *ACTIVEMODEL);
	//���㻡��ê��ֱ�ߵĽ��㣬��Ϊ������ʼ��
	EditElementHandle lineEeh;
	LineHandler::CreateLineElement(lineEeh, nullptr, DSegment3d::From(rebarstar, rebarend), true, *ACTIVEMODEL);
	DPoint3d interPt[2];
	int num = mdlIntersect_allBetweenExtendedElms(interPt, nullptr, 2, lineEeh.GetElementDescrP(), arcEeh.GetElementDescrP(), nullptr, 1, nullptr, nullptr);
	double dis = INT_MAX;
	DVec3d arcVec = arcEndPt - arcStrPt;
	DPoint3d newStrPt = { 0,0,0 };
	for (int i = 0; i < num; ++i)
	{
		bool isStr = false;
		if (interPt[i].Distance(arcStrPt) < dis)
		{
			//newStrPt = interPt[i];
			//arcVec.Normalize();
			//arcVec.Scale(bendRadius);
			//newStrPt.Add(arcVec);
			endpt = interPt[i];
			//vectmp.Normalize();
			//vectmp.Scale(bendRadius);
			//endpt.Add(vectmp);
			dis = interPt[i].Distance(arcStrPt);
		}
	}
	//arcStrPt = newStrPt;

	bool istr = false;
	bool tmpstr = false;
	if (ExtractFacesTool::IsPointInLine(endpt, vers.At(vers.GetSize() - 1).GetIP(), vers.At(vers.GetSize() - 2).GetIP(), ACTIVEMODEL, tmpstr) == true)
	{
		istr = true;
	}

	vectmp = endpt - strpt;

	DPoint3d vecNormal;//�˷���Ϊê������һ�����۵ķ���
	vecNormal = NormalPts[0] - NormalPts[1];
	vecNormal.Normalize();
	vectmp.Normalize();
	Transform tmptrans;
	tmptrans.InitIdentity();
	double tmpA = rotateangle / 180.0 * PI;
	CVector3D tmpNorvec = normal;
	CVector3D tmpVec = vecNormal;
	tmpVec.Rotate(tmpA, tmpNorvec);
	vecNormal = tmpVec;
	vecNormal.Normalize();
	vecNormal.Scale(L0Lenth);

	DPoint3d endpt2;
	mdlVec_addPoint(&endpt2, &endpt, &vecNormal);
	if (istr)//������������ϣ����ж����ǰһ����Ϊ���ߣ����¼��㻡��IP�����ĵ���յ�
	{
		if (vers.At(vers.GetSize() - 2).GetRadius() > 0 && vers.At(vers.GetSize() - 2).GetRadius() > bendRadius + 10)//�ǻ���
		{
			GetArcRebarVertexIntersetLine(vers.At(vers.GetSize() - 2), endpt, endpt2, vecNormal, bendRadius);
			RebarVertex*   vertmp;
			vertmp = &vers.At(vers.GetSize() - 1);
			vertmp->SetType(RebarVertex::kIP);
			vertmp->SetIP(vers.At(vers.GetSize() - 2).GetArcPt(2));
			if (vers.GetSize() - 3 >= 0 && vers.GetSize() - 3 < vers.GetSize() - 2)
			{
				RebarVertex*   vertmp2;
				vertmp2 = &vers.At(vers.GetSize() - 3);
				vertmp2->SetIP(vers.At(vers.GetSize() - 2).GetArcPt(0));
			}
		}
	}

	strpt = vers.At(vers.GetSize() - 1).GetIP();
	if (vers.GetSize() > 1)//2��������ʱ��strptҪ�жϽ����Ƿ�������KIP��֮�䣬�����֮��Ҫȡvers.GetSize() - 1��ǰһ����
	{
		bool isStr;
		DPoint3d tmppt = vers.At(vers.GetSize() - 2).GetIP();
		if (ExtractFacesTool::IsPointInLine(endpt, tmppt, strpt, ACTIVEMODEL, isStr))
		{
			strpt = tmppt;
		}
	}
	//ͨ��3���˵���㻡��������ͻ���Բ�ĵ㣬��ͨ���õ��Ļ�����RebarVertex
	RebarArcData arcdata;
	IntersectionPointToArcDataRebar(arcdata, endpt, strpt, endpt2, bendRadius);
	RebarVertex*   vertmp;
	if (vers.GetSize() == 2)
	{
		vertmp = &vers.At(vers.GetSize() - 1);
		vertmp->SetType(RebarVertex::kIP);
		vertmp->SetIP(endpt);
		vertmp->SetArcPt(0, arcdata.ptArcBegin);
		vertmp->SetArcPt(1, arcdata.ptArcMid);
		vertmp->SetArcPt(2, arcdata.ptArcEnd);
		vertmp->SetRadius(bendRadius);
		vertmp->SetCenter(arcdata.ptArcCenter);
	}
	else
	{
		vertmp = new RebarVertex();
		vertmp->SetType(RebarVertex::kIP);
		vertmp->SetIP(endpt);
		vertmp->SetArcPt(0, arcdata.ptArcBegin);
		vertmp->SetArcPt(1, arcdata.ptArcMid);
		vertmp->SetArcPt(2, arcdata.ptArcEnd);
		vertmp->SetRadius(bendRadius);
		vertmp->SetCenter(arcdata.ptArcCenter);
		vers.Add(vertmp);
	}
	arcStrPt = arcdata.ptArcEnd;

	//����ѡ�е�3���ֽ�ȷ��һ�λ�
	//DPoint3d arcStrPt = NormalPts.at(0);
	//DPoint3d arcMidPt = NormalPts.at(2);
	//DPoint3d arcEndPt = NormalPts.at(4);
	//mdlVec_projectPointToPlane(&arcStrPt, &arcStrPt, vers.At(0).GetIP(), &vec1);
	//mdlVec_projectPointToPlane(&arcMidPt, &arcMidPt, vers.At(0).GetIP(), &vec1);
	//mdlVec_projectPointToPlane(&arcEndPt, &arcEndPt, vers.At(0).GetIP(), &vec1);
	//DEllipse3d ellipse = DEllipse3d::FromPointsOnArc(arcStrPt, arcMidPt, arcEndPt);
	//EditElementHandle arcEeh;
	//ArcHandler::CreateArcElement(arcEeh, nullptr, ellipse, true, *ACTIVEMODEL);
	//���㻡��ê��ֱ�ߵĽ��㣬��Ϊ������ʼ��
	//EditElementHandle lineEeh;
	//LineHandler::CreateLineElement(lineEeh, nullptr, DSegment3d::From(vers.At(0).GetIP(), vers.At(1).GetIP()), true, *ACTIVEMODEL);
	//mdlIntersect_allBetweenExtendedElms(&arcStrPt, nullptr, 1, lineEeh.GetElementDescrP(), arcEeh.GetElementDescrP(), nullptr, 1, nullptr, nullptr);
	//ȷ���µĻ�
	DEllipse3d newEllipse = DEllipse3d::FromPointsOnArc(arcStrPt, arcMidPt, arcEndPt);
	EditElementHandle newArcEeh;
	ArcHandler::CreateArcElement(newArcEeh, nullptr, newEllipse, true, *ACTIVEMODEL);
	//mdlElmdscr_add(arcEeh1.GetElementDescrP());
	double arcLen = ellipse.ArcLength();
	//��ȡê�볤�ȵĻ�
	double startAngle = 0, sweepAngle = 0, radius = 0;
	DPoint3d centerPt = {0,0,0};
	mdlArc_extract(nullptr, &startAngle, &sweepAngle, &radius, nullptr, nullptr, &centerPt, newArcEeh.GetElementP());
	GetPositiveAngle(startAngle, centerPt, arcStrPt);
	GetPositiveAngle(sweepAngle, centerPt, arcEndPt);
	double starR = (360 - startAngle) / 180 * fc_pi;
	double angle = (L0Lenth - bendRadius) * 180 / radius / PI / 10;
	if (COMPARE_VALUES_EPS(sweepAngle, startAngle, 1e-6) == 1)
	{
		angle = -angle;
	}
	MSElement outArc;
	mdlArc_create(&outArc, nullptr, &centerPt, radius, radius, nullptr, starR, angle);
	//DEllipse3d finalEllipe = DEllipse3d::FromFractionInterval(ellipse, 0, (L0Lenth - bendRadius) / arcLen);
	//�õ����ջ��ϵ�������
	//finalEllipe.EvaluateEndPoints(arcStrPt, arcEndPt);
	//arcMidPt = finalEllipe.FractionToPoint(0.5);
	DPoint3d arcPts[2];
	mdlArc_extract(arcPts, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &outArc);
	DPoint3d midPt = { 0,0,0 };
	EditElementHandle tmpArc(&outArc, ACTIVEMODEL);
	mdlElmdscr_pointAtDistance(&midPt, nullptr, (L0Lenth - bendRadius) / 2, tmpArc.GetElementDescrP(), 0.1);
	RebarVertices tmpVers;
	CalculateArc(tmpVers, arcPts[0], midPt, arcPts[1]);
	DEllipse3d tmp = DEllipse3d::FromPointsOnArc(arcPts[0], midPt, arcPts[1]);
	//EditElementHandle tmpEeh;
	//ArcHandler::CreateArcElement(tmpEeh, nullptr, tmp, true, *ACTIVEMODEL);
	//tmpEeh.AddToModel();
	for (size_t i = 0; i < tmpVers.GetSize(); ++i)
	{
		RebarVertexP vex = &vers.NewElement();
		vex->SetIP(tmpVers.At(i).GetIP());
		vex->SetType(tmpVers.At(i).GetType());    
		vex->SetRadius(tmpVers.At(i).GetRadius());
		vex->SetCenter(tmpVers.At(i).GetCenter());

		vex->SetArcPt(0, tmpVers.At(i).GetArcPt(0));
		vex->SetArcPt(1, tmpVers.At(i).GetArcPt(1));
		vex->SetArcPt(2, tmpVers.At(i).GetArcPt(2));
	}
	return true;
}

bool CalculateArc(RebarVertices& vers, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
{
	bool ret = false;

	BeArcSeg arc(begPt, midPt, endPt);

	CMatrix3D mat;
	CPoint3D cen;
	arc.GetCenter(cen);

	if (arc.GetPlanarMatrix(mat) && arc.GetCenter(cen))
	{
		CPoint3D beg = begPt;
		CPoint3D med = midPt;
		CPoint3D end = endPt;

		beg.Transform(mat);
		med.Transform(mat);
		end.Transform(mat);
		cen.Transform(mat);
		arc.Transform(mat);

		CVector3D tan1 = arc.GetTangentVector(beg);
		CVector3D tan2 = arc.GetTangentVector(end);

		CPointVect pv1(beg, tan1);
		CPointVect pv2(end, tan2);

		CPoint3D ip;
		bool isIntersect = pv1.Intersect(ip, pv2);

		double radius = arc.GetRadius();

		RebarVertexP vex = &vers.NewElement();
		vex->SetIP(beg);
		vex->SetType(RebarVertex::kStart);      // first IP

		CPoint3D mid = (beg + end) / 2.0;
		CVector3D midVec(cen, mid);
		midVec.Normalize();

		if (isIntersect)
		{
			mid = cen + midVec * radius;

			// it can be on the other size
			CPoint3D mid1 = cen - midVec * radius;

			double d1 = med.Distance(mid1);
			double d2 = med.Distance(mid);

			if (d1 < d2)
			{
				mid = mid1;
				midVec = -midVec;
				// this is big arc we need 4 ips

				CVector3D midTan = midVec.Perpendicular();
				CPointVect pvm(mid, midTan);

				pv1.Intersect(ip, pvm);
				vex = &vers.NewElement();
				vex->SetIP(ip);
				vex->SetType(RebarVertex::kIP);      // 2nd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				mid1 = (beg + mid) / 2.0;
				midVec = mid1 - cen;
				midVec.Normalize();
				mid1 = cen + midVec * radius;

				vex->SetArcPt(0, beg);
				vex->SetArcPt(1, mid1);
				vex->SetArcPt(2, mid);



				pv1.Intersect(ip, pvm);
				vex = &vers.NewElement();
				vex->SetIP(beg);
				vex->SetType(RebarVertex::kIP);      // 3rd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				mid1 = (end + mid) / 2.0;
				midVec = mid1 - cen;
				midVec.Normalize();
				mid1 = cen + midVec * radius;

				vex->SetArcPt(0, mid);
				vex->SetArcPt(1, mid1);
				vex->SetArcPt(2, end);

			}
			else
			{
				// this is less than 90 or equal we need 3 ips
				vex = &vers.NewElement();
				vex->SetIP(ip);
				vex->SetType(RebarVertex::kIP);      // 2nd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				vex->SetArcPt(0, beg);
				vex->SetArcPt(1, mid);
				vex->SetArcPt(2, end);
			}
		}
		else
		{
			// this is half circle - we need 4 ips
			midVec = arc.GetTangentVector(med);
			midVec.Normalize();
			DPoint3d ptMedTan = midVec;
			ptMedTan.Scale(radius);
			ptMedTan.Add(med);
			DPoint3d ptBegTan = tan1;
			ptBegTan.Scale(radius);
			ptBegTan.Add(beg);
			mdlVec_intersect(ip, &DSegment3d::From(beg, ptBegTan), &DSegment3d::From(med, ptMedTan));

			mid = cen + tan1 * radius;
			DEllipse3d circle = DEllipse3d::FromCenterRadiusXY(cen, radius);

			double angle_start = circle.PointToAngle(beg);
			double angle_mid = circle.PointToAngle(mid);

			double angle = (angle_start + angle_mid) / 2;

			CPoint3D mid1;
			circle.Evaluate(&mid1, 0, angle);

			vex = &vers.NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 2nd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, beg);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, mid);

			DPoint3d ptEndTan = tan2;
			ptEndTan.Scale(radius);
			ptEndTan.Add(end);
			mdlVec_intersect(ip, &DSegment3d::From(end, ptEndTan), &DSegment3d::From(med, ptMedTan));

			double angle_end = circle.PointToAngle(end);

			angle = (angle_end + angle_mid) / 2;

			circle.Evaluate(&mid1, 0, angle);

			vex = &vers.NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 3rd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, mid);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, end);
		}

		vex = &vers.NewElement();
		vex->SetIP(end);
		vex->SetType(RebarVertex::kEnd);      // last IP

		mat = mat.Inverse();
		vers.DoMatrix(mat);              // transform back

		ret = true;
	}

	return ret;
}

//ͨ���㼯�ϣ�����ֽ��������Щ���п���Ϊ�ߴ������۲��ְ뾶ΪBendRadius
bool GetRebarVerticesFromPoints(RebarVertices&  vers, bvector<DPoint3d>& allPts, double bendRadius)
{
	if (allPts.size() < 1)
	{
		return false;
	}
	DPoint3d ptVertex1;
	DPoint3d ptVertex2;
	DPoint3d ptVertex3;
	for (int i = 0; i < allPts.size(); i++)
	{
		RebarVertex*   vertmp;
		vertmp = new RebarVertex();
		if (i == 0)
		{
			ptVertex1 = allPts[i];
			vertmp->SetType(RebarVertex::kStart);
			vertmp->SetIP(ptVertex1);
			vers.Add(vertmp);
		}
		else if (i == allPts.size() - 1)
		{

			vertmp->SetType(RebarVertex::kEnd);
			vertmp->SetIP(allPts[i]);
			vers.Add(vertmp);
		}
		else
		{

			ptVertex1 = allPts[i - 1];
			ptVertex2 = allPts[i];
			ptVertex3 = allPts[i + 1];
			DPoint3d vec1 = ptVertex1 - ptVertex2;
			DPoint3d vec2 = ptVertex3 - ptVertex2;
			vec1.Normalize();
			vec2.Normalize();
			if (COMPARE_VALUES_EPS(vec1.DotProduct(vec2), 1.0, 0.1) == 0)//���ƽ��ʱ(������ͬһ��ֱ����)
			{
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(allPts[i]);
				vers.Add(vertmp);

			}
			else
			{
				RebarArcData arcdata;
				IntersectionPointToArcDataRebar(arcdata, ptVertex2, ptVertex1, ptVertex3, bendRadius);
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(ptVertex2);
				vertmp->SetArcPt(0, arcdata.ptArcBegin);
				vertmp->SetArcPt(1, arcdata.ptArcMid);
				vertmp->SetArcPt(2, arcdata.ptArcEnd);
				vertmp->SetRadius(bendRadius);
				vertmp->SetCenter(arcdata.ptArcCenter);
				vers.Add(vertmp);
			}

		}
	}
	return true;
}
//ͨ������ȡ�˵��Ӧ��֮ǰ�Ļ�ֱ����û�д˵㷵��0
double GetStoreBendRadius(DPoint3d ptip, vector<DPoint3d>& vecpts, vector<double>& vecbendradius)
{
	
	for (int i=0;i<vecpts.size();i++)
	{
		if (vecpts.at(i).Distance(ptip)<=10.0)
		{
			return vecbendradius[i];
		}
	}
	return 0.0;
}

//ͨ���㼯�ϣ�����ֽ��������Щ���п���Ϊ�ߴ������۲��ְ뾶ΪBendRadius
bool GetRebarVerticesFromPointsAndBendRadius(RebarVertices&  vers, bvector<DPoint3d>& allPts, double bendRadius,vector<DPoint3d>& vecpts,vector<double>& vecbendradius)
{
	if (allPts.size() < 1)
	{
		return false;
	}
	DPoint3d ptVertex1;
	DPoint3d ptVertex2;
	DPoint3d ptVertex3;
	for (int i = 0; i < allPts.size(); i++)
	{
		RebarVertex*   vertmp;
		vertmp = new RebarVertex();
		if (i == 0)
		{
			ptVertex1 = allPts[i];
			vertmp->SetType(RebarVertex::kStart);
			vertmp->SetIP(ptVertex1);
			vers.Add(vertmp);
		}
		else if (i == allPts.size() - 1)
		{

			vertmp->SetType(RebarVertex::kEnd);
			vertmp->SetIP(allPts[i]);
			vers.Add(vertmp);
		}
		else
		{

			ptVertex1 = allPts[i - 1];
			ptVertex2 = allPts[i];
			ptVertex3 = allPts[i + 1];
			double totalLenth = ptVertex1.Distance(ptVertex2) + ptVertex2.Distance(ptVertex3);
			double comLenth = ptVertex1.Distance(ptVertex3);
			if (COMPARE_VALUES_EPS(totalLenth, comLenth, 1.0) == 0)//���2����1��3��������ʱ,�����˵�
			{
				continue;
			}
			DPoint3d vec1 = ptVertex1 - ptVertex2;
			DPoint3d vec2 = ptVertex3 - ptVertex2;
			vec1.Normalize();
			vec2.Normalize();
			double dotvalue = vec1.DotProduct(vec2);
			if (COMPARE_VALUES_EPS(abs(dotvalue), 1.0, 0.001) == 0)//���ƽ��ʱ
			{
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(allPts[i]);
				vers.Add(vertmp);

			}
			else
			{
				double tmpRadius = 0;
				tmpRadius = GetStoreBendRadius(ptVertex2,vecpts,vecbendradius);
				if (tmpRadius<bendRadius)
				{
					tmpRadius = bendRadius;
				}
				else
				{
					tmpRadius = tmpRadius - 10;
				}
				RebarArcData arcdata;
				IntersectionPointToArcDataRebar(arcdata, ptVertex2, ptVertex1, ptVertex3, tmpRadius);
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(ptVertex2);
				vertmp->SetArcPt(0, arcdata.ptArcBegin);
				vertmp->SetArcPt(1, arcdata.ptArcMid);
				vertmp->SetArcPt(2, arcdata.ptArcEnd);
				vertmp->SetRadius(tmpRadius);
				vertmp->SetCenter(arcdata.ptArcCenter);

				if ((i+2)< allPts.size()&&tmpRadius>bendRadius)//���i+1��i+2֮����ֱ��,��ֱ����benradiusҪ��ʱ,��ֹ���һ�أ����¼��㻡�������յ�
				{
					DPoint3d vecNormal = ptVertex3 - allPts[i+2];
					GetArcRebarVertexIntersetLine(*vertmp, ptVertex3, allPts[i + 2], vecNormal, bendRadius);
				}


				vers.Add(vertmp);
			}

		}
	}
	return true;
}
int GetAllCombinePtsUseEndPoint(bvector<DPoint3d>& allpts, bvector<DPoint3d>& pts1, bvector<DPoint3d>& pts2, DPoint3d intersectpt, EditElementHandleR linestring1, EditElementHandleR linestring2)
{
	int theNum = 0;
	//�ȼ����һ���߶εĵ�
	double startdis, enddis, interdis;
	mdlElmdscr_distanceAtPoint(&startdis, nullptr, nullptr, linestring1.GetElementDescrP(), pts1.begin(), 0.1);
	mdlElmdscr_distanceAtPoint(&enddis, nullptr, nullptr, linestring1.GetElementDescrP(), pts1.begin() + pts1.size() - 1, 0.1);
	mdlElmdscr_distanceAtPoint(&interdis, nullptr, nullptr, linestring1.GetElementDescrP(), &intersectpt, 0.1);
	if (abs(startdis - interdis)<abs(enddis - interdis))//��������Ͻ�
	{
		std::reverse(pts1.begin(), pts1.end());//����
		allpts.insert(allpts.begin(), pts1.begin(), pts1.end());
	}
	else 
	{
		allpts.insert(allpts.begin(), pts1.begin(), pts1.end());
	}
	theNum = (int)allpts.size();//�ڼ���Ԫ��
	allpts.push_back(intersectpt);
	//����ڶ����ߴ��ĵ�
	mdlElmdscr_distanceAtPoint(&startdis, nullptr, nullptr, linestring2.GetElementDescrP(), pts2.begin(), 0.1);
	mdlElmdscr_distanceAtPoint(&enddis, nullptr, nullptr, linestring2.GetElementDescrP(), pts2.begin() + pts2.size() - 1, 0.1);
	mdlElmdscr_distanceAtPoint(&interdis, nullptr, nullptr, linestring2.GetElementDescrP(), &intersectpt, 0.1);
	if (abs(startdis - interdis) < abs(enddis - interdis))//��������
	{
		pts2.erase(pts2.begin());//ɾ����һ����
		allpts.insert(allpts.end(), pts2.begin(), pts2.end());

	}
	else //������յ�
	{
		std::reverse(pts2.begin(), pts2.end());//����
		pts2.erase(pts2.begin());//ɾ����һ����
		allpts.insert(allpts.end(), pts2.begin(), pts2.end());
	}
	return theNum;
}

//ͨ�������ߴ�������������ߴ��Ľ��㣬���ų������ཻ��Χ�ڵĵ㣬���õ��ߴ��ϲ�������е㼯��
int GetAllCombinePtsUseIntersect(bvector<DPoint3d>& allpts, bvector<DPoint3d>& pts1, bvector<DPoint3d>& pts2, DPoint3d intersectpt, EditElementHandleR linestring1, EditElementHandleR linestring2)
{
	int theNum = 0;
	//�ȼ����һ���߶εĵ�
	double startdis, enddis, interdis;
	mdlElmdscr_distanceAtPoint(&startdis, nullptr, nullptr, linestring1.GetElementDescrP(), pts1.begin(), 0.1);
	mdlElmdscr_distanceAtPoint(&enddis, nullptr, nullptr, linestring1.GetElementDescrP(), pts1.begin() + pts1.size() - 1, 0.1);
	mdlElmdscr_distanceAtPoint(&interdis, nullptr, nullptr, linestring1.GetElementDescrP(), &intersectpt, 0.1);
	if (COMPARE_VALUES_EPS(startdis, interdis, 0.1) == 0)//������߶�1������������ӳ�����
	{
		std::reverse(pts1.begin(), pts1.end());//����
		pts1.erase(pts1.end() - 1);//ɾ�����һ����
		allpts.insert(allpts.begin(), pts1.begin(), pts1.end());
	}
	else if (COMPARE_VALUES_EPS(enddis, interdis, 0.1) == 0)//������߶�1���յ�����յ��ӳ�����
	{
		pts1.erase(pts1.end() - 1);//ɾ�����һ����
		allpts.insert(allpts.begin(), pts1.begin(), pts1.end());
	}
	else //�������ߴ���
	{
		if (interdis < enddis / 2)//��ǰ�����
		{
			std::reverse(pts1.begin(), pts1.end());//����
			for (DPoint3d tmppt : pts1)
			{
				double tmpdis;
				mdlElmdscr_distanceAtPoint(&tmpdis, nullptr, nullptr, linestring1.GetElementDescrP(), &tmppt, 0.1);
				if (tmpdis > interdis)
				{
					allpts.push_back(tmppt);
				}
			}
		}
		else
		{
			for (DPoint3d tmppt : pts1)
			{
				double tmpdis;
				mdlElmdscr_distanceAtPoint(&tmpdis, nullptr, nullptr, linestring1.GetElementDescrP(), &tmppt, 0.1);
				if (tmpdis < interdis)
				{
					allpts.push_back(tmppt);
				}
			}
		}

	}
	theNum = (int)allpts.size();//�ڼ���Ԫ��
	allpts.push_back(intersectpt);
	//����ڶ����ߴ��ĵ�
	mdlElmdscr_distanceAtPoint(&startdis, nullptr, nullptr, linestring2.GetElementDescrP(), pts2.begin(), 0.1);
	mdlElmdscr_distanceAtPoint(&enddis, nullptr, nullptr, linestring2.GetElementDescrP(), pts2.begin() + pts2.size() - 1, 0.1);
	mdlElmdscr_distanceAtPoint(&interdis, nullptr, nullptr, linestring2.GetElementDescrP(), &intersectpt, 0.1);
	if (COMPARE_VALUES_EPS(startdis, interdis, 0.1) == 0)//������߶�1������������ӳ�����
	{
		pts2.erase(pts2.begin());//ɾ����һ����
		allpts.insert(allpts.end(), pts2.begin(), pts2.end());

	}
	else if (COMPARE_VALUES_EPS(enddis, interdis, 0.1) == 0)//������߶�1���յ�����յ��ӳ�����
	{
		std::reverse(pts2.begin(), pts2.end());//����
		pts2.erase(pts2.begin());//ɾ����һ����
		allpts.insert(allpts.end(), pts2.begin(), pts2.end());
	}
	else //�������ߴ���
	{
		if (interdis < enddis / 2)//��ǰ�����
		{
			for (DPoint3d tmppt : pts2)
			{
				double tmpdis;
				mdlElmdscr_distanceAtPoint(&tmpdis, nullptr, nullptr, linestring2.GetElementDescrP(), &tmppt, 0.1);
				if (tmpdis > interdis)
				{
					allpts.push_back(tmppt);
				}
			}
		}
		else
		{
			std::reverse(pts2.begin(), pts2.end());//����
			for (DPoint3d tmppt : pts2)
			{
				double tmpdis;
				mdlElmdscr_distanceAtPoint(&tmpdis, nullptr, nullptr, linestring2.GetElementDescrP(), &tmppt, 0.1);
				if (tmpdis < interdis)
				{
					allpts.push_back(tmppt);
				}
			}

		}

	}

	return theNum;

}
//�ںϲ��ֽ�ʱ��������Ľ���ĵ��������������ĵ��֮��ĵ����˾������200ʱ��
void CalculateAddVerticalRebars(EditElementHandleP end1, EditElementHandleP end2, RebarVertex Rebarver,
	double diameter, bvector<DPoint3d>& allpts, vector<RebarVertices>& rebarPts, vector<BrString>& vecDir, double spacinglenth)
{
	{//����������
		if (RebarElement::IsRebarElement(*end1) && RebarElement::IsRebarElement(*end2))
		{
			RebarCurve curve;
			RebarElementP rep = RebarElement::Fetch(*end1);
			double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
			RebarShape * rebarshape = rep->GetRebarShape(end1->GetModelRef());
			if (rebarshape == nullptr)
			{
				return;
			}
			rebarshape->GetRebarCurve(curve);
			BrString Sizekeydj(rebarshape->GetShapeData().GetSizeKey());
			double diameterdj;
			DPoint3d ptstr, ptend;
			GetStartEndPointFromRebar(end1, ptstr, ptend, diameterdj);


			DPoint3d vecCenter = Rebarver.GetCenter() - Rebarver.GetArcPt(1);
			vecCenter.Normalize();
			if (!vecCenter.IsEqual(DPoint3d::From(0, 0, 0)))
			{
				//��ת��ĵط����һ�����
				vecCenter.Scale(diameter / 2 + diameterdj / 2);
				DPoint3d ptcenterstr;
				mdlVec_addPoint(&ptcenterstr, &Rebarver.GetArcPt(1), &vecCenter);
				DPoint3d midPtstr, midPtend;
				midPtstr = ptstr;
				midPtend = ptend;
				midPtstr.x = ptcenterstr.x;
				midPtstr.y = ptcenterstr.y;

				midPtend.x = ptcenterstr.x;
				midPtend.y = ptcenterstr.y;
				RebarVertices  vers2;
				RebarVertex*   vertmp1, *vertmp2;
				vertmp1 = new RebarVertex();
				vertmp1->SetType(RebarVertex::kStart);
				vertmp1->SetIP(midPtstr);
				vers2.Add(vertmp1);

				vertmp2 = new RebarVertex();
				vertmp2->SetType(RebarVertex::kEnd);
				vertmp2->SetIP(midPtend);
				vers2.Add(vertmp2);

				rebarPts.push_back(vers2);
				vecDir.push_back(Sizekeydj);

				EditElementHandle combineline;
				DPoint3d* comVecs = new DPoint3d[allpts.size()];
				memset(comVecs, 0, allpts.size() * sizeof(DPoint3d));
				memcpy(comVecs, allpts.begin(), allpts.size() * sizeof(DPoint3d));
				LineStringHandler::CreateLineStringElement(combineline, nullptr, comVecs, allpts.size(), true, *ACTIVEMODEL);
				DPoint3d ptcentertdescr;
				ptcentertdescr = getCenterOfElmdescr(combineline.GetElementDescrP());
				//�����һ�����֮���Ƿ���Ҫ���м����һ�����
				if (midPtstr.DistanceXY(ptstr) > spacinglenth * uor_per_mm + diameter / 2 + diameterdj / 2)
				{
					DPoint3d tmpstr, tmpend;
					tmpstr = midPtstr;
					tmpend = midPtend;
					tmpstr.Add(ptstr);
					tmpstr.Scale(0.5);

					DPoint3d project1;
					ptcentertdescr.x = tmpstr.x;
					ptcentertdescr.y = tmpstr.y;
					mdlProject_perpendicular(&project1, NULL, NULL, combineline.GetElementDescrP(), ACTIVEMODEL, &ptcentertdescr, NULL, 0.001);
					DPoint3d vectmp = ptcentertdescr - project1;
					vectmp.Normalize();
					vectmp.Scale(diameter / 2 + diameterdj / 2);
					DPoint3d  addpt;
					mdlVec_addPoint(&addpt, &project1, &vectmp);
					addpt.z = tmpstr.z;
					tmpend.x = addpt.x;
					tmpend.y = addpt.y;


					vers2[0]->SetIP(addpt);
					vers2[1]->SetIP(tmpend);

					rebarPts.push_back(vers2);
					vecDir.push_back(Sizekeydj);
				}
				DPoint3d ptstr2, ptend2;
				GetStartEndPointFromRebar(end2, ptstr2, ptend2, diameterdj);
				//����ڶ������֮���Ƿ���Ҫ���м����һ�����
				if (midPtstr.DistanceXY(ptstr2) > spacinglenth * uor_per_mm + diameter / 2 + diameterdj / 2)
				{
					DPoint3d tmpstr, tmpend;
					tmpstr = midPtstr;
					tmpend = midPtend;
					tmpstr.z = ptstr2.z;
					tmpend.z = ptend2.z;
					tmpstr.Add(ptstr2);
					tmpstr.Scale(0.5);
					DPoint3d project1;
					ptcentertdescr.x = tmpstr.x;
					ptcentertdescr.y = tmpstr.y;
					mdlProject_perpendicular(&project1, NULL, NULL, combineline.GetElementDescrP(), ACTIVEMODEL, &ptcentertdescr, NULL, 0.001);
					EditElementHandle eeh;
					DPoint3d vectmp = ptcentertdescr - project1;
					vectmp.Normalize();
					vectmp.Scale(diameter / 2 + diameterdj / 2);
					DPoint3d  addpt;
					mdlVec_addPoint(&addpt, &project1, &vectmp);
					addpt.z = tmpstr.z;
					tmpend.x = addpt.x;
					tmpend.y = addpt.y;
					vers2[0]->SetIP(addpt);
					vers2[1]->SetIP(tmpend);
					rebarPts.push_back(vers2);
					vecDir.push_back(Sizekeydj);
				}


				delete[] comVecs;
				comVecs = nullptr;
			}


		}

	}
}


