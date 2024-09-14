#include "_ustation.h"
#include "URebar.h"
#include "CommonFile.h"
#include "ExtractFacesTool.h"
#include "SelectRebarTool.h"

PIT::URebar::URebar(const vector<CPoint3D> &vecRebarPts, URebarDataCR rebarData)
	:m_vecRebarPts(vecRebarPts),m_rebarData(rebarData)
{
}

PIT::URebar::~URebar()
{
// 	if (m_pOrgRebar != nullptr)
// 	{
// 		EditElementHandle eh(m_refLineId, m_pOrgRebar->GetModelRef());
// 		eh.DeleteFromModel();
// 	}

}

bool CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
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

		RebarVertexP vex;
		vex = &(curve.PopVertices()).NewElement();
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
				vex = &curve.PopVertices().NewElement();
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
				vex = &curve.PopVertices().NewElement();
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
				vex = &curve.PopVertices().NewElement();
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
			// 			EditElementHandle eeh;
			// 			LineHandler::CreateLineElement(eeh, NULL, DSegment3d::From(beg, ptBegTan), true, *ACTIVEMODEL);
			// 			eeh.AddToModel();
			// 			EditElementHandle eeh1;
			// 			LineHandler::CreateLineElement(eeh1, NULL, DSegment3d::From(med, ptMedTan), true, *ACTIVEMODEL);
			// 			eeh1.AddToModel();
			// 			CPointVect pvm(med, midVec);
			// 			pvm.Intersect(ip, tan1);
			// 			tan1 = ip - beg;
			// 			tan1.Normalize();
			// 
			// 			ip = beg + tan1 * radius;
			// 			midVec = ip - cen;
			// 			midVec.Normalize();
			//			midVec = ip - cen;
			mid = cen + tan1 * radius;
			DEllipse3d circle = DEllipse3d::FromCenterRadiusXY(cen, radius);

			double angle_start = circle.PointToAngle(beg);
			double angle_mid = circle.PointToAngle(mid);

			double angle = (angle_start + angle_mid) / 2;
			// 			if (angle < angle_mid)
			// 				angle += _PI;
			CPoint3D mid1;
			circle.Evaluate(&mid1, 0, angle);

			//			CPoint3D mid1 = cen + midVec;

			vex = &curve.PopVertices().NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 2nd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, beg);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, mid);

			// 			ip = end + tan1 * radius; // tan1 and tan2 parallel but tan1 has now the correct direction, do not change to tan2
			// 			midVec = ip - cen;
			// 			midVec.Normalize();
			// 			mid1 = cen + midVec * radius;

			//			midVec.Negate();
			DPoint3d ptEndTan = tan2;
			ptEndTan.Scale(radius);
			ptEndTan.Add(end);
			mdlVec_intersect(ip, &DSegment3d::From(end, ptEndTan), &DSegment3d::From(med, ptMedTan));
			// 			midVec = ip - cen;
			// 			mid1 = cen + midVec;

			// 			EditElementHandle eeh2;
			// 			LineHandler::CreateLineElement(eeh2, NULL, DSegment3d::From(end, ptEndTan), true, *ACTIVEMODEL);
			// 			eeh2.AddToModel();
			double angle_end = circle.PointToAngle(end);

			angle = (angle_end + angle_mid) / 2;
			// 			if (angle < angle_end)
			// 				angle += _PI;

			circle.Evaluate(&mid1, 0, angle);

			vex = &curve.PopVertices().NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 3rd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, mid);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, end);
		}

		vex = &curve.PopVertices().NewElement();
		vex->SetIP(end);
		vex->SetType(RebarVertex::kEnd);      // last IP

		mat = mat.Inverse();
		curve.DoMatrix(mat);              // transform back

		ret = true;
	}

	return ret;
}


void CalculateArcCuve(PIT::PITRebarCurve& tmpCurve, DPoint3d PtStr, DPoint3d PtEnd, double Lenth, DPoint3d CenterPt, DVec3d rebarVec_V, bool isStr)
{
	double radius = CenterPt.Distance(PtStr);
	DPoint3d vecLine = PtStr - PtEnd;
	vecLine.ScaleToLength(Lenth);

	DPoint3d tmpLine = vecLine;
	tmpLine.Add(PtStr);
	DPoint3d midPos = tmpLine;
	midPos.Add(PtStr);
	midPos.Scale(0.5);

	EditElementHandle eeharc;
	DEllipse3d dellip = DEllipse3d::FromCenterNormalRadius(CenterPt, rebarVec_V, radius);
	ArcHandler::CreateArcElement(eeharc, nullptr, dellip, true, *ACTIVEMODEL);

	//计算直线与弧的交点
	EditElementHandle eehlineend;
	LineHandler::CreateLineElement(eehlineend, nullptr, DSegment3d::From(CenterPt, tmpLine), true, *ACTIVEMODEL);
	EditElementHandle eehlinemid;
	LineHandler::CreateLineElement(eehlinemid, nullptr, DSegment3d::From(CenterPt, midPos), true, *ACTIVEMODEL);

	DPoint3d arcinter[2];
	mdlIntersect_allBetweenExtendedElms(arcinter, nullptr, 2, eeharc.GetElementDescrP(), eehlineend.GetElementDescrP(), nullptr, 0.1, &tmpLine, &tmpLine);
	//弧线与直线的交点
	DPoint3d lastarcinter, midArcPos;
	if (tmpLine.Distance(arcinter[0]) < tmpLine.Distance(arcinter[1]))
	{
		lastarcinter = arcinter[0];
	}
	else
	{
		lastarcinter = arcinter[1];
	}

	mdlIntersect_allBetweenExtendedElms(arcinter, nullptr, 2, eeharc.GetElementDescrP(), eehlinemid.GetElementDescrP(), nullptr, 0.1, &midPos, &midPos);
	if (midPos.Distance(arcinter[0]) < midPos.Distance(arcinter[1]))
	{
		midArcPos = arcinter[0];
	}
	else
	{
		midArcPos = arcinter[1];
	}

	if (isStr)
	{
		CalculateArc(tmpCurve, lastarcinter, midArcPos, PtStr);
	}
	else
	{
		CalculateArc(tmpCurve, PtStr, midArcPos, lastarcinter);
	}

}


ElementId PIT::URebar::DrawRefArcStrLine(DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return 0;
	}
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
    DPoint3d	CenterPt = GetrebarData().arcCenter;
	DPlane3d faceH = DPlane3d::FromOriginAndNormal(m_vecRebarPts[1], m_rebarVec_V);
	faceH.ProjectPoint(CenterPt, CenterPt);

	PIT::PITRebarCurve tmpCurve;
	CalculateArcCuve(tmpCurve, m_vecRebarPts[0], m_vecRebarPts[1], m_rebarData.legLength1*uor_per_mm, CenterPt, m_rebarVec_V, true);
	if (tmpCurve.GetVertices().GetSize()<3)
	{
		return 0;
	}
	EditElementHandle arceeh;
	ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(tmpCurve.GetVertices().At(1).GetArcPt(0),
		tmpCurve.GetVertices().At(1).GetArcPt(1), tmpCurve.GetVertices().At(1).GetArcPt(2)), true, *modelRef);
	arceeh.AddToModel();
	return arceeh.GetElementId();
}

ElementId PIT::URebar::DrawRefArcEndLine(DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return 0;
	}
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	DPoint3d	CenterPt = GetrebarData().arcCenter;
	DPlane3d faceH = DPlane3d::FromOriginAndNormal(m_vecRebarPts[1], m_rebarVec_V);
	faceH.ProjectPoint(CenterPt, CenterPt);

	PIT::PITRebarCurve tmpCurveEnd;
	CalculateArcCuve(tmpCurveEnd, m_vecRebarPts[m_vecRebarPts.size() - 1], m_vecRebarPts[m_vecRebarPts.size() - 2], m_rebarData.legLength2*uor_per_mm, CenterPt, m_rebarVec_V, false);
	if (tmpCurveEnd.GetVertices().GetSize() < 3)
	{
		return 0;
	}
	EditElementHandle arceeh;
	ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(tmpCurveEnd.GetVertices().At(1).GetArcPt(0),
		tmpCurveEnd.GetVertices().At(1).GetArcPt(1), tmpCurveEnd.GetVertices().At(1).GetArcPt(2)), true, *modelRef);
	arceeh.AddToModel();
	return arceeh.GetElementId();
}




ElementId PIT::URebar::DrawRefLine(DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return 0;
	}

	StatusInt ret = -1;
	EditElementHandle eeh;
	LineStringHandler::CreateLineStringElement(eeh, nullptr, &m_vecRebarPts[0], m_vecRebarPts.size(), true,  *modelRef);
	ret = eeh.AddToModel();

	return eeh.GetElementId();
}





RebarElement * PIT::URebar::Create(RebarSetR rebarSet)
{
	DgnModelRefP modelRef = rebarSet.GetModelRef();

	BrString strRebarSize = m_rebarData.rebarSize;
	if (strRebarSize.Find(L"mm") != WString::npos)
	{
		strRebarSize.Replace(L"mm", L"");
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dim = RebarCode::GetBarDiameter(strRebarSize, modelRef);
	double bendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);

	vector<CPoint3D> tmpPts = m_vecRebarPts;
	tmpPts.erase(std::unique(tmpPts.begin(), tmpPts.end(), [](const CPoint3D& pt1, const CPoint3D& pt2) {
		return pt1.IsEqual(pt2);
	}), tmpPts.end());
	m_Curve.makeILURebarCurve(tmpPts, bendRadius);
	//if (GetrebarData().legLength1 == 0 && GetrebarData().legLength2 == 0)
	//{
	//	vector<CPoint3D> tmpPts = m_vecRebarPts;
	//	tmpPts.erase(std::unique(tmpPts.begin(), tmpPts.end(), [](const CPoint3D& pt1, const CPoint3D& pt2) {
	//		return pt1.IsEqual(pt2);
	//	}), tmpPts.end());
	//	m_Curve.makeStraightRebarCurve(tmpPts);
	//}
	//else
	//{
	//	m_Curve.makeURebarCurve(m_vecRebarPts, bendRadius);
	//}
	
	//if (m_vecRebarPts.size()<4)
	//{
	//	return nullptr;
	//}

	DPoint3d CenterPt;
	PIT::PITRebarCurve lastCurve;
	if (GetrebarData().isArc)
	{
		CenterPt = GetrebarData().arcCenter;
		DPlane3d faceH = DPlane3d::FromOriginAndNormal(m_vecRebarPts[1], m_rebarVec_V);
		faceH.ProjectPoint(CenterPt, CenterPt);
	
		PIT::PITRebarCurve tmpCurve;
		CalculateArcCuve(tmpCurve, m_vecRebarPts[0], m_vecRebarPts[1], m_rebarData.legLength1*uor_per_mm, CenterPt, m_rebarVec_V, true);
		PIT::PITRebarCurve tmpCurveEnd;
		CalculateArcCuve(tmpCurveEnd, m_vecRebarPts[m_vecRebarPts.size()-1], m_vecRebarPts[m_vecRebarPts.size() - 2], m_rebarData.legLength2*uor_per_mm, CenterPt, m_rebarVec_V, false);
		for (int i = 0;i<tmpCurve.GetVertices().GetSize();i++)
		{
			RebarVertexP vex;
			RebarVertexP Tmpvex = &tmpCurve.PopVertices().At(i);
			vex = &lastCurve.PopVertices().NewElement();
			*vex = *Tmpvex;
			if (i==0)
			{
				vex->SetType(RebarVertex::kStart);
			}
			else
			{
				vex->SetType(RebarVertex::kIP);
			}
		}
		for (int i = 0; i < m_Curve.GetVertices().GetSize(); i++)
		{
			RebarVertexP vex;
			RebarVertexP Tmpvex = &m_Curve.PopVertices().At(i);
			vex = &lastCurve.PopVertices().NewElement();
			*vex = *Tmpvex;
			vex->SetType(RebarVertex::kIP);
		}
		for (int i = 0; i < tmpCurveEnd.GetVertices().GetSize(); i++)
		{
			RebarVertexP vex;
			RebarVertexP Tmpvex = &tmpCurveEnd.PopVertices().At(i);
			vex = &lastCurve.PopVertices().NewElement();
			*vex = *Tmpvex;
			if (i == tmpCurveEnd.GetVertices().GetSize()-1)
			{
				vex->SetType(RebarVertex::kEnd);
			}
			else
			{
				vex->SetType(RebarVertex::kIP);
			}
		}
	}
	else
	{
		for (int i = 0; i < m_Curve.GetVertices().GetSize(); i++)
		{
			RebarVertexP vex;
			RebarVertexP Tmpvex = &m_Curve.PopVertices().At(i);
			vex = &lastCurve.PopVertices().NewElement();
			*vex = *Tmpvex;
		}
	}

	int Grade = 2;
	string tmpGrade(m_rebarData.SelectedRebarGrade);
	if (tmpGrade == "A")
		Grade = 0;
	else if (tmpGrade == "B")
		Grade = 1;
	else if (tmpGrade == "C")
		Grade = 2;
	else
		Grade = 3;

	int index = (int)rebarSet.GetChildElementCount();
	int rebarNum = index + 1;
	RebarElementP rebarElement = rebarSet.AssignRebarElement(index, rebarNum, m_rebarData.rebarSymb, modelRef);
	if (nullptr != rebarElement)
	{
		RebarShapeData rebarData;
		rebarData.SetSizeKey((LPCTSTR)strRebarSize);
		rebarData.SetIsStirrup(false);
		rebarData.SetLength(lastCurve.GetLength() / uor_per_mm);
		RebarEndType endType;
		endType.SetType(RebarEndType::kNone);
		RebarEndTypes endTypes = { endType,endType };
		rebarElement->Update(lastCurve, dim, endTypes, rebarData, modelRef, false);

		ElementId eleid = rebarElement->GetElementId();
		EditElementHandle tmprebar(eleid, ACTIVEMODEL);
		string Stype = m_rebarData.SelectedRebarType;
		Stype = "URebar" + Stype;
		ElementRefP oldref = tmprebar.GetElementRef();
		SetRebarLevelItemTypeValue(tmprebar,m_rebarData.SelectedRebarLevel,Grade,Stype, ACTIVEMODEL);
		SetRebarHideData(tmprebar, m_rebarData.SelectedSpacing, ACTIVEMODEL);
		tmprebar.ReplaceInModel(oldref);
	}
	return rebarElement;
}

RebarElement * PIT::URebar::CreateURebar(RebarSetR rebarSet, const vector<CPoint3D>& vecRebarPts, PIT::URebarDataCR rebarData)
{
	return URebar(vecRebarPts, rebarData).Create(rebarSet);
}

PIT::URebarMaker::URebarMaker(ElementId  rebar_V1, ElementId  rebar_V2, const vector<ElementId>  &rebarId_H,
	PIT::URebarDataR rebarData, vector<EditElementHandle*> holeEehs, bool bNegate, bool bUp, bool bInner, DgnModelRefP modelRef)
	:m_bNegate(bNegate),m_bUp(bUp), m_holes(holeEehs),m_bInner(bInner)
{
	for (size_t i = 0; i < rebarId_H.size(); ++i)
	{
		CalURebarLeftRightDownPts(rebar_V1, rebar_V2, rebarId_H[i], rebarData, modelRef);
	}
	UpdateURebars(rebarData, modelRef);
}

PIT::URebarMaker::URebarMaker(const vector<ElementId> &rebarId_H, PIT::URebarDataR rebarData,
	vector<EditElementHandle*> holeEehs, const DVec3d& rebarVec, bool bNegate /*= false*/, 
	bool bUp /*= false*/, bool isEnd /*= false*/, bool bInner, DgnModelRefP modelRef /*= ACTIVEMODEL*/)
	:m_bNegate(bNegate), m_bUp(bUp), m_holes(holeEehs),m_bInner(bInner)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	m_rebarVec_H = DVec3d::From(rebarVec);
	for (size_t i = 0; i < rebarId_H.size(); ++i)
	{
		EditElementHandle rebarEeh(rebarId_H.at(i), modelRef);
		DPoint3d ptstr, ptend; //钢筋起始点和终点
		double dia = 0;
		GetStartEndPointFromRebar(&rebarEeh, ptstr, ptend, dia);
		DPoint3d rebarPt = ptstr; //L型筋的中间点
		DVec3d hRebarVec = ptend - ptstr;	//钢筋方向
		if (isEnd) //如果选中的倚靠点时尾端，翻转钢筋方向
		{
			rebarPt = ptend;
			hRebarVec = ptstr - ptend;
		}
		hRebarVec.Normalize();

		BrString strRebarSize = rebarData.rebarSize;
		if (strRebarSize.Find(L"mm") != WString::npos)
			strRebarSize.Replace(L"mm", L"");
		double dimUReabr = RebarCode::GetBarDiameter(strRebarSize, modelRef);
		double dimH = dia;
		double bendradius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);

		hRebarVec.Scale(dimUReabr / 2);
		rebarPt.Add(hRebarVec);
		
		DVec3d moveVec = rebarVec; //钢筋往所在面的移动方向
		moveVec.Normalize();
		if (bNegate)
		{
			moveVec.Negate();
		}
		moveVec.Scale(dimH / 2 + dimUReabr / 2);
		rebarPt.Add(moveVec);

		DPoint3d pt1 = rebarPt, pt2 = rebarPt, pt3 = rebarPt;
		hRebarVec.Normalize();
		hRebarVec.Scale(rebarData.legLength1 * uor_per_mm + bendradius);
		pt1.Add(hRebarVec);

		DVec3d vVec = DVec3d::From(0, 0, -1); //钢筋面垂直方向
		vVec.CrossProduct(hRebarVec, rebarVec);
		if (bUp)
		{
			vVec.Negate();
		}
		vVec.Normalize();
		vVec.Scale(rebarData.legLength2 * uor_per_mm + bendradius);
		pt3.Add(vVec);

		//DPoint3d tmpPt = pt2;
		//vector<DPoint3d> intersectPts;
		//GetIntersectPointsWithHoles(intersectPts, m_holes, tmpPt, pt1);
		//sort(intersectPts.begin(), intersectPts.end(), [&](const DPoint3d& ptA, const DPoint3d& ptB) {
		//	double dis1 = ptA.Distance(pt2);
		//	double dis2 = ptB.Distance(pt2);
		//	return COMPARE_VALUES_EPS(dis1, dis2, 1) == -1;
		//});
		//if (intersectPts.size() > 0)
		//{
		//	pt1 = intersectPts.at(0);
		//}

		//intersectPts;
		//GetIntersectPointsWithHoles(intersectPts, m_holes, tmpPt, pt3);
		//sort(intersectPts.begin(), intersectPts.end(), [&](const DPoint3d& ptA, const DPoint3d& ptB) {
		//	double dis1 = ptA.Distance(pt2);
		//	double dis2 = ptB.Distance(pt2);
		//	return COMPARE_VALUES_EPS(dis1, dis2, 1) == -1;
		//});
		//if (intersectPts.size() > 0)
		//{
		//	pt3 = intersectPts.at(0);
		//	pt2 = pt3;
		//	pt3.Add(vVec);
		//}

		vector<CPoint3D> rebarPts; //钢筋点
		rebarPts = { pt1, pt2, pt3 };
		m_vecURebarPts.push_back(rebarPts);

		m_vecURebar.push_back(shared_ptr<PIT::URebar>(new PIT::URebar(rebarPts, rebarData)));
	}
}

PIT::URebarMaker::~URebarMaker()
{

}

void PIT::URebarMaker::GetArcCenter(PIT::URebarDataR rebarData, RebarCurve curve_H,double BendRadius)
{
	RebarVertices  vers = curve_H.PopVertices();
	for (int i = 0; i < vers.GetSize(); i++)
	{
		if (vers.At(i).GetRadius() > BendRadius *2)
		{
			rebarData.isArc = true;
			rebarData.arcCenter = DPoint3d::From(vers.At(i).GetCenter().x, vers.At(i).GetCenter().y, vers.At(i).GetCenter().z);
		}
	}
}


//bool PIT::URebarMaker::CalURebarLeftRightDownPts(ElementId rebar_V1, ElementId rebar_V2, ElementId rebar_H, PIT::URebarDataCR rebarData, DgnModelRefP modelRef)
//{
//	if (modelRef == nullptr)
//	{
//		return false;
//	}
//
//	EditElementHandle eehRebar_V1(rebar_V1, modelRef);
//	RebarElementP pRebar_V1 = RebarElement::Fetch(eehRebar_V1);
//	EditElementHandle eehRebar_V2(rebar_V2, modelRef);
//	RebarElementP pRebar_V2 = RebarElement::Fetch(eehRebar_V2);
//	if (pRebar_V1 == nullptr || pRebar_V2 == nullptr)
//	{
//		return false;
//	}
//
//	vector<CPoint3D> vecPts;
//	RebarCurve curve_V1,curve_V2;
//	pRebar_V1->GetRebarCurve(curve_V1, m_RebarSize_V1, modelRef);
//	pRebar_V2->GetRebarCurve(curve_V2, m_RebarSize_V2, modelRef);
//	LineSegment maxLineSeg_V1,maxLineSeg_V2;
//	if (!PITRebarCurve::GetMaxLenLine(curve_V1, maxLineSeg_V1) || !PITRebarCurve::GetMaxLenLine(curve_V2, maxLineSeg_V2))
//	{
//		return false;
//	}
//
//	//计算横向钢筋与纵向钢筋的3维交点
//	EditElementHandle eehRebar_H(rebar_H, modelRef);
//	RebarElementP pRebar_H = RebarElement::Fetch(eehRebar_H);
//	if (pRebar_H == nullptr)
//	{
//		return false;
//	}
//	RebarCurve curve_H;
//	pRebar_H->GetRebarCurve(curve_H, m_RebarSize_H, modelRef);
//	LineSegment maxLineSeg_H;
//	if (!PITRebarCurve::GetMaxLenLine(curve_H, maxLineSeg_H))
//	{
//		return false;
//	}
//	m_rebarVec_H = maxLineSeg_H.GetLineVec();
//	BrString strRebarSize = rebarData.rebarSize;
//	if (strRebarSize.Find(L"mm") != WString::npos)
//		strRebarSize.Replace(L"mm", L"");
//	double dimUReabr = RebarCode::GetBarDiameter(strRebarSize, modelRef);
//	double dimH = RebarCode::GetBarDiameter(m_RebarSize_H, modelRef);
//	DPoint3d ptIntersect = maxLineSeg_V1.Intersect(maxLineSeg_H);
//	m_rebarVec_V = maxLineSeg_V1.GetLineVec();
//// 	DVec3d vec_V = m_rebarVec_V;
//// 	vec_V.ScaleToLength(dimH * 0.5 + dimUReabr * 0.5);
//// 	ptIntersect.Add(vec_V);
//
//
//	//计算U形钢筋左下角点
//	DVec3d vecV2V1 = maxLineSeg_V1.GetLineStartPoint() - maxLineSeg_V2.GetLineStartPoint();
//	DPoint3d  ptLeftDown = ptIntersect;
//	double dimV1 = RebarCode::GetBarDiameter(m_RebarSize_V1, modelRef);
//	double dimV2 = RebarCode::GetBarDiameter(m_RebarSize_V2, modelRef);
//	double offset = dimV1 > dimV2 ? dimV1 : dimV2;
//// 	vec_H.ScaleToLength(offset * 0.5 + dimUReabr * 0.5);
//// 	ptLeftDown.Add(vec_H);
//	vecV2V1.ScaleToLength(dimV1 * 0.5 + dimUReabr * 0.5);
//	ptLeftDown.Add(vecV2V1);
//	m_ptLeftDowns.push_back(ptLeftDown);
//
//
//	//计算U形钢筋右下角点
//	DVec3d vecV1V2 = maxLineSeg_V2.GetLineStartPoint() - maxLineSeg_V1.GetLineStartPoint();
//	DPoint3d ptRightDown = ptIntersect;
//	ptRightDown.Add(vecV1V2);
//	vecV1V2.ScaleToLength(dimV2 * 0.5 + dimUReabr * 0.5);
//	ptRightDown.Add(vecV1V2);
//	m_ptRightDowns.push_back(ptRightDown);
//
//// 	EditElementHandle eeh;
//// 	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptLeftDown, ptRightDown), true, *modelRef);
//// 	eeh.AddToModel();
//	return true;
//}
bool PIT::URebarMaker::CalURebarLeftRightDownPts(ElementId rebar_V1, ElementId rebar_V2, ElementId rebar_H, PIT::URebarDataR rebarData, DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return false;
	}

	EditElementHandle eehRebar_V1(rebar_V1, modelRef);
	RebarElementP pRebar_V1 = RebarElement::Fetch(eehRebar_V1);
	EditElementHandle eehRebar_V2(rebar_V2, modelRef);
	RebarElementP pRebar_V2 = RebarElement::Fetch(eehRebar_V2);
	if (pRebar_V1 == nullptr || pRebar_V2 == nullptr)
	{
		return false;
	}

	vector<CPoint3D> vecPts;
	RebarCurve curve_V1, curve_V2;
	pRebar_V1->GetRebarCurve(curve_V1, m_RebarSize_V1, modelRef);
	pRebar_V2->GetRebarCurve(curve_V2, m_RebarSize_V2, modelRef);
	LineSegment maxLineSeg_V1, maxLineSeg_V2;
	if (!PITRebarCurve::GetMaxLenLine(curve_V1, maxLineSeg_V1) || !PITRebarCurve::GetMaxLenLine(curve_V2, maxLineSeg_V2))
	{
		return false;
	}

	//计算横向钢筋与纵向钢筋的3维交点
	EditElementHandle eehRebar_H(rebar_H, modelRef);
	RebarElementP pRebar_H = RebarElement::Fetch(eehRebar_H);
	if (pRebar_H == nullptr)
	{
		return false;
	}
	RebarCurve curve_H;
	pRebar_H->GetRebarCurve(curve_H, m_RebarSize_H, modelRef);
	LineSegment maxLineSeg_H;
	if (!PITRebarCurve::GetMaxLenLine(curve_H, maxLineSeg_H))
	{
		return false;
	}
	DPoint3d prjV1, prjV2;

	DPlane3d faceH = DPlane3d::FromOriginAndNormal(maxLineSeg_H.GetLineStartPoint(), maxLineSeg_V1.GetLineVec());
	faceH.ProjectPoint(prjV1, maxLineSeg_V1.GetLineStartPoint());
	faceH.ProjectPoint(prjV2, maxLineSeg_V2.GetLineStartPoint());

	BrString strRebarSize = rebarData.rebarSize;
	if (strRebarSize.Find(L"mm") != WString::npos)
		strRebarSize.Replace(L"mm", L"");
	double dimUReabr = RebarCode::GetBarDiameter(strRebarSize, modelRef);
	double dimH = RebarCode::GetBarDiameter(m_RebarSize_H, modelRef);
	DVec3d tmpDown = maxLineSeg_V1.GetLineVec();
	/*DPoint3d ptIntersect = maxLineSeg_V1.Intersect(maxLineSeg_H);
	m_rebarVec_V = maxLineSeg_V1.GetLineVec();*/
	// 	DVec3d vec_V = m_rebarVec_V;
	// 	vec_V.ScaleToLength(dimH * 0.5 + dimUReabr * 0.5);
	// 	ptIntersect.Add(vec_V);

	tmpDown.ScaleToLength(dimH * 0.5 + dimUReabr * 0.5);
	if (m_bUp)
	{
		tmpDown.Scale(-1);
	}
	m_rebarVec_V = tmpDown;
	prjV1.Add(tmpDown);
	prjV2.Add(tmpDown);
	double dimV1 = RebarCode::GetBarDiameter(m_RebarSize_V1, modelRef);
	double dimV2 = RebarCode::GetBarDiameter(m_RebarSize_V2, modelRef);
	double bendradiusV1 = RebarCode::GetPinRadius(strRebarSize, modelRef, false);
	double bendradiusV2 = RebarCode::GetPinRadius(strRebarSize, modelRef, false);

	GetArcCenter(rebarData, curve_H, bendradiusV1);
	double disV1, disV2;
	double tmpDisV1, tmpDisV2;
	tmpDisV1 = (bendradiusV1 - dimUReabr / 2 - dimV1 / 2)*0.70710678118655;
	tmpDisV2 = (bendradiusV2 - dimUReabr / 2 - dimV2 / 2)*0.70710678118655;
	disV1 = bendradiusV1 - tmpDisV1;
	disV2 = bendradiusV2 - tmpDisV2;
	if (!rebarData.isArc)
	{
		disV1 = dimV1 / 2 + dimH / 2;
		disV2 = dimV2 / 2 + dimH / 2;
	}
	//计算U形钢筋左下角点
	DVec3d vecV1V2 = prjV1 - prjV2;
	vecV1V2.Normalize();
	DPoint3d vecFace = faceH.normal;
	vecFace.CrossProduct(vecV1V2, vecFace);
	DPoint3d vecOut = vecFace;
	if (m_bNegate)
	{
		vecOut.Scale(-1);
	}
	m_rebarVec_H = DVec3d::From(vecOut);
	m_rebarVec_H.Scale(-1);
	DPoint3d  ptLeftDown = prjV1;
	vecOut.ScaleToLength(disV1);
	if (!m_bInner)
	{
		double uV1Dis = (dimUReabr - dimV1) / 2;
		vecV1V2.ScaleToLength(disV1 - uV1Dis);
	}
	else
	{
		double uV1Dis = dimH / 2 + dimV1 + dimUReabr / 2;
		vecV1V2.ScaleToLength(disV1 - uV1Dis);
	}
	
	ptLeftDown.Add(vecV1V2);
	ptLeftDown.Add(vecOut);
	m_ptLeftDowns.push_back(ptLeftDown);

	//计算U形钢筋右下角点
	DVec3d vecV2V1 = prjV2 - prjV1;
	DPoint3d ptRightDown = prjV2;
	vecOut.ScaleToLength(disV2);
	if (!m_bInner)
	{
		double uV2Dis = (dimUReabr - dimV2) / 2;
		vecV2V1.ScaleToLength(disV2 - uV2Dis);
	}
	else
	{
		double uV2Dis = dimH / 2 + dimV2 + dimUReabr / 2;
		vecV2V1.ScaleToLength(disV2 - uV2Dis);
	}
	ptRightDown.Add(vecV2V1);
	ptRightDown.Add(vecOut);
	m_ptRightDowns.push_back(ptRightDown);

	return true;
}
bool PIT::URebarMaker::CalURebarPts(vector<CPoint3D>& pts, DPoint3d ptLeftDown, DPoint3d ptRightDown, PIT::URebarDataCR rebarData, bool negate, bool up, DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return false;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	BrString strRebarSize = rebarData.rebarSize;
	if (strRebarSize.Find(L"mm") != WString::npos)
		strRebarSize.Replace(L"mm", L"");

	double dimUReabr = RebarCode::GetBarDiameter(strRebarSize, modelRef);
	double dimV1 = RebarCode::GetBarDiameter(m_RebarSize_V1, modelRef);
	double dimV2 = RebarCode::GetBarDiameter(m_RebarSize_V2, modelRef);
	double Bendradius = RebarCode::GetPinRadius(strRebarSize, modelRef,false);

	double offset = dimV1 > dimV2 ? dimV1 : dimV2;

	DVec3d vec_H = m_rebarVec_H;

	//if (negate)
	//{
	//	vec_H.Negate();
	//}

	DPoint3d pt1, pt2, pt3, pt4;
	vec_H.ScaleToLength(offset * 0.5 + dimUReabr * 0.5);
	//ptLeftDown.Add(vec_H);
	//ptRightDown.Add(vec_H);

// 	EditElementHandle eeh;
// 	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptLeftDown, ptRightDown), true, *modelRef);
// 	eeh.AddToModel();
	
	
	double Lenth1, Lenth2;
	if (rebarData.isArc)
	{
		Lenth1 = 1;
		Lenth2 = 1;

		DPoint3d vecLine = ptLeftDown - ptRightDown;
		vecLine.ScaleToLength(dimV1 / 5 + dimUReabr / 5);
		ptRightDown.Add(vecLine);
		vecLine.Scale(-1);
		ptLeftDown.Add(vecLine);
	}
	else
	{
		Lenth1 = rebarData.legLength1;
		Lenth2 = rebarData.legLength2;
	}
	pt2 = ptLeftDown;
	pt1 = ptLeftDown;
	pt3 = ptRightDown;
	pt4 = ptRightDown;
	//vec_H.Negate();
	if (Lenth1 > 0)
	{
		vec_H.ScaleToLength(Lenth1 * uor_per_mm + Bendradius);
		pt1.Add(vec_H);
	}	
	
	vector<DPoint3d> intersectPts;
	GetIntersectPointsWithHoles(intersectPts, m_holes, pt2, pt1);
	sort(intersectPts.begin(), intersectPts.end(), [&](const DPoint3d& ptA, const DPoint3d& ptB) {
		double dis1 = ptA.Distance(pt2);
		double dis2 = ptB.Distance(pt2);
		return COMPARE_VALUES_EPS(dis1, dis2, 1) == -1;
	});
	if (intersectPts.size() > 0)
	{
		pt1 = intersectPts.at(0);
	}
	if (Lenth2 > 0)
	{
		vec_H.ScaleToLength(Lenth2 * uor_per_mm + Bendradius);
		pt4.Add(vec_H);
	}
	intersectPts.clear();
	GetIntersectPointsWithHoles(intersectPts, m_holes, pt3, pt4);
	sort(intersectPts.begin(), intersectPts.end(), [&](const DPoint3d& ptA, const DPoint3d& ptB) {
		double dis1 = ptA.Distance(pt3);
		double dis2 = ptB.Distance(pt3);
		return COMPARE_VALUES_EPS(dis1, dis2, 1) == -1;
	});
	if (intersectPts.size() > 0)
	{
		pt4 = intersectPts.at(0);
	}

	DVec3d vec_V = m_rebarVec_V;
	double dim_H = RebarCode::GetBarDiameter(m_RebarSize_H, modelRef);
	vec_V.ScaleToLength(dim_H * 0.5 + dimUReabr * 0.5);
	/*pt1.Add(vec_V);
	pt2.Add(vec_V);
	pt3.Add(vec_V);
	pt4.Add(vec_V);*/


	pts = { pt1,pt2,pt3,pt4 };
	return true;
}


RebarSetTag * PIT::URebarMaker::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return nullptr;
	}


	RebarSetP rebarSet = RebarSet::Fetch(rebarSetId,modelRef);
	if (rebarSet == nullptr)
	{
		return nullptr;
	}
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(rebarSetId);
	rebarSet->StartUpdate(modelRef);

	int rebarNum = (int)m_vecURebar.size();
	for (int i = 0; i < rebarNum; ++i)
	{
		m_vecURebar[i]->Create(*rebarSet);
	}

	RebarSetTag *tag = new RebarSetTag;
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);
	return tag;
}

bool PIT::URebarMaker::UpdateURebar(vector<CPoint3D>& pts, DPoint3d ptLeftDown, DPoint3d ptRightDown, PIT::URebarDataCR rebarData, bool negate, bool up, int rebarIndex, DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return 0;
	}
	pts.clear();
	if (!CalURebarPts(pts, ptLeftDown, ptRightDown, rebarData, negate, up, modelRef))
		return false;

	if (m_vecURebar.empty() || (int)m_vecURebar.size() <= rebarIndex)
	{
		m_vecURebar.push_back(shared_ptr<PIT::URebar>(new PIT::URebar(pts, rebarData)));
		m_vecURebar[m_vecURebar.size() - 1]->m_rebarVec_V = m_rebarVec_V;
	}
	else
	{
		m_vecURebar[rebarIndex] = shared_ptr<PIT::URebar>(new PIT::URebar(pts, rebarData));
		m_vecURebar[rebarIndex]->m_rebarVec_V = m_rebarVec_V;
	}

	return true;
}

bool PIT::URebarMaker::UpdateURebars(PIT::URebarDataCR rebarData, DgnModelRefP modeRef)
{
	if (modeRef == nullptr || m_ptLeftDowns.empty() || m_ptLeftDowns.size() != m_ptRightDowns.size())
	{
		return false;
	}

	m_vecURebar.clear();
	for (int i = 0; i < (int)m_ptLeftDowns.size(); ++i)
	{
		vector<CPoint3D> rebarPts;
		UpdateURebar(rebarPts, m_ptLeftDowns[i], m_ptRightDowns[i], rebarData, m_bNegate, m_bUp, i, modeRef);
		m_vecURebarPts.push_back(rebarPts);
	}

	return false;
}

bool PIT::URebarMaker::DrawRefLine(ElementId & id, int index, DgnModelRefP modelRef)
{
	if (index >= m_vecURebar.size() || modelRef == nullptr)
	{
		return false;
	}

	id = m_vecURebar[index]->DrawRefLine(modelRef);
	return true;
}

bool PIT::URebarMaker::DrawRefLine(vector<ElementId>& ids, DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return false;
	}

	ids.clear();
	for (auto ptr : m_vecURebar)
	{
		if (ptr != nullptr)
		{
			ids.push_back(ptr->DrawRefLine(modelRef));
			if (ptr->GetrebarData().isArc)
			{
				ids.push_back(ptr->DrawRefArcStrLine(modelRef));
				ids.push_back(ptr->DrawRefArcEndLine(modelRef));
			}
		}
	}
	return true;
}

