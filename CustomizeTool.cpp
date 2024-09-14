#include "_ustation.h"
#include "CustomizeTool.h"
#include "ExtractFacesTool.h"
#include "SelectRebarTool.h"
#include "PITRebarCurve.h"
#include "ArcRebarTool.h"

bool CustomRebar::CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
{
	bool ret = false;

	BeArcSeg arc(begPt, midPt, endPt);

	CPoint3D cen;
	arc.GetCenter(cen);

	if (arc.GetCenter(cen))
	{
		CPoint3D beg = begPt;
		CPoint3D med = midPt;
		CPoint3D end = endPt;

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

		if (curve.PopVertices().GetSize() == 1)
		{
			vex->SetType(RebarVertex::kStart);      // first IP
		}
		else
		{
			vex->SetType(RebarVertex::kIP); 
		}

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
		vex->SetType(RebarVertex::kIP);      // last IP

		ret = true;
	}

	return ret;
}

bool CustomRebar::CalculateArc(RebarVertices&  vers, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
{
	bool ret = false;
	PIT::PITRebarCurve curve;
	BeArcSeg arc(begPt, midPt, endPt);

	CPoint3D cen;
	arc.GetCenter(cen);

	if (arc.GetCenter(cen))
	{
		CPoint3D beg = begPt;
		CPoint3D med = midPt;
		CPoint3D end = endPt;

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
		if (curve.PopVertices().GetSize() == 1)
		{
			vex->SetType(RebarVertex::kStart);      // first IP
		}
		else
		{
			vex->SetType(RebarVertex::kIP);
		}
		vers.Add(vex);
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
				vers.Add(vex);


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
				vers.Add(vex);
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
				vers.Add(vex);
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
			vers.Add(vex);
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
			vers.Add(vex);
		}

		vex = &curve.PopVertices().NewElement();
		vex->SetIP(end);
		vex->SetType(RebarVertex::kIP);      // last IP
		vers.Add(vex);
		ret = true;
	}

	return ret;
}

void CustomRebar::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

void CustomRebar::Create(RebarSetR rebarSet)
{
	DgnModelRefP modelRef = rebarSet.GetModelRef();
	BrString strRebarSize = m_CustomRebarInfo.rebarSize;
	if (strRebarSize.Find(L"mm") != WString::npos)
	{
		strRebarSize.Replace(L"mm", L"");
	}


	strcpy(m_CustomRebarInfo.rebarSize, CT2A(strRebarSize));
	GetDiameterAddType(m_CustomRebarInfo.rebarSize, m_CustomRebarInfo.rebarType);//附加型号

	std::vector<PIT::PITRebarCurve> vecRebarCurve;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dim = RebarCode::GetBarDiameter(strRebarSize, modelRef);
	double bendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);

//	RebarVertices  vers;
//	bvector<DPoint3d> allPts;
// 	for (int j = 0; j < m_vecRebarPts.size(); j++)//存储线上所有不重复的点
// 	{
// 		if (allPts.size() == 0)
// 		{
// 			if (m_vecRebarPts[j].curType == ICurvePrimitive::CURVE_PRIMITIVE_TYPE_Arc)
// 			{
// 				double dis1;
// 				dis1 = 0;
// 				EditElementHandle arceeh;
// 				ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromArcCenterStartEnd(m_vecRebarPts[j].ptCenter, m_vecRebarPts[j].ptStr, m_vecRebarPts[j].ptEnd), true, *ACTIVEMODEL);
// 				arceeh.AddToModel();
// 				mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[j].ptEnd, 0.1);
// 				DPoint3d tmpMid;
// 				double dis2 = 0.00;
// 				mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[j].ptEnd, 0.1);
// 				dis2 /= 2;
// 				mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis2, arceeh.GetElementDescrP(), 0.1);
// 				
// 				BeArcSeg arc(m_vecRebarPts[j].ptStr, tmpMid, m_vecRebarPts[j].ptEnd);
// 				CPoint3D cen;
// 				arc.GetCenter(cen);
// 				double radius = arc.GetRadius();
// 				CPoint3D Miden;
// 				if (arc.GetCenter(cen))
// 				{
// 					CPoint3D beg = m_vecRebarPts[j].ptStr;
// 					CPoint3D med = tmpMid;
// 					CPoint3D end = m_vecRebarPts[j].ptEnd;
// 
// 					CVector3D tan1 = arc.GetTangentVector(beg);
// 					CVector3D tan2 = arc.GetTangentVector(end);
// 
// 					CPointVect pv1(beg, tan1);
// 					CPointVect pv2(end, tan2);
// 
// 					CPoint3D ip;
// 					bool isIntersect = pv1.Intersect(ip, pv2);
// 					CPoint3D mid = (beg + end) / 2.0;
// 					CVector3D midVec(cen, mid);
// 					midVec.Normalize();
// 
// 					if (isIntersect)
// 					{
// 						mid = cen + midVec * radius;
// 						CPoint3D mid1 = cen - midVec * radius;
// 
// 						double d1 = med.Distance(mid1);
// 						double d2 = med.Distance(mid);
// 
// 						if (d1 < d2)
// 						{
// 							mid = mid1;
// 							midVec = -midVec;
// 							// this is big arc we need 4 ips
// 
// 							CVector3D midTan = midVec.Perpendicular();
// 							CPointVect pvm(mid, midTan);
// 							pv1.Intersect(ip, pvm);
// 							Miden = ip;							
// 						}
// 						else
// 						{
// 							Miden = ip;
// 						}
// 					}
// 				}
// 
// 				allPts.push_back(m_vecRebarPts[j].ptStr);
// 				allPts.push_back(Miden);
// 				allPts.push_back(m_vecRebarPts[j].ptEnd);
// 			}
// 			else
// 			{
// 				allPts.push_back(m_vecRebarPts[j].ptStr);
// 				allPts.push_back(m_vecRebarPts[j].ptEnd);
// 			}
// 		}
// 		else
// 		{
// 			if (m_vecRebarPts[j].curType == ICurvePrimitive::CURVE_PRIMITIVE_TYPE_Arc)
// 			{
// 				double dis1;
// 				dis1 = 0;
// 				EditElementHandle arceeh;
// 				ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromArcCenterStartEnd(m_vecRebarPts[j].ptCenter, m_vecRebarPts[j].ptStr, m_vecRebarPts[j].ptEnd), true, *ACTIVEMODEL);
// 				arceeh.AddToModel();
// 				mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[j].ptEnd, 0.1);
// 				DPoint3d tmpMid;
// 				double dis2 = 0.00;
// 				mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[j].ptEnd, 0.1);
// 				dis2 /= 2;
// 				mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis2, arceeh.GetElementDescrP(), 0.1);
// 				
// 				BeArcSeg arc(m_vecRebarPts[j].ptStr, tmpMid, m_vecRebarPts[j].ptEnd);
// 				CPoint3D cen;
// 				arc.GetCenter(cen);
// 				double radius = arc.GetRadius();
// 				CPoint3D Miden;
// 				if (arc.GetCenter(cen))
// 				{
// 					CPoint3D beg = m_vecRebarPts[j].ptStr;
// 					CPoint3D med = tmpMid;
// 					CPoint3D end = m_vecRebarPts[j].ptEnd;
// 
// 					CVector3D tan1 = arc.GetTangentVector(beg);
// 					CVector3D tan2 = arc.GetTangentVector(end);
// 
// 					CPointVect pv1(beg, tan1);
// 					CPointVect pv2(end, tan2);
// 
// 					CPoint3D ip;
// 					bool isIntersect = pv1.Intersect(ip, pv2);
// 					CPoint3D mid = (beg + end) / 2.0;
// 					CVector3D midVec(cen, mid);
// 					midVec.Normalize();
// 
// 					if (isIntersect)
// 					{
// 						mid = cen + midVec * radius;
// 						CPoint3D mid1 = cen - midVec * radius;
// 
// 						double d1 = med.Distance(mid1);
// 						double d2 = med.Distance(mid);
// 
// 						if (d1 < d2)
// 						{
// 							mid = mid1;
// 							midVec = -midVec;
// 							// this is big arc we need 4 ips
// 
// 							CVector3D midTan = midVec.Perpendicular();
// 							CPointVect pvm(mid, midTan);
// 
// 							pv1.Intersect(ip, pvm);
// 							Miden = ip;
// 						}
// 						else
// 						{
// 							Miden = ip;
// 						}
// 					}
// 				}
// 
// 				allPts.push_back(Miden);
// 				allPts.push_back(m_vecRebarPts[j].ptEnd);
// 			}
// 			else
// 			{
// 				allPts.push_back(m_vecRebarPts[j].ptEnd);
// 			}
// 		}
// 
// 	}

	vector <vector<pointInfo>>vctCustom;

	vctCustom.push_back(m_vecRebarPts);
	pointInfo CustomTemp;

	double spacing = m_CustomRebarInfo.rebarSpacing * uor_per_mm;
	CVector3D vec;
	std::string strDir(m_CustomRebarInfo.rebarArrayDir);
	if (strDir == "X")
	{
		vec = CVector3D::kXaxis;
	}
	else if (strDir == "Y")
	{
		vec = CVector3D::kYaxis;
	}
	else
	{
		vec = CVector3D::kZaxis;
	}
	vector<pointInfo> vctTemp;
	for (int a = 0;a < m_CustomRebarInfo.rebarArrayNum - 1;a++)
	{
		if (a == 0)
		{
			for (int b = 0; b < m_vecRebarPts.size(); b++)
			{
				CustomTemp.curType = m_vecRebarPts[b].curType;
				CustomTemp.ptCenter = m_vecRebarPts[b].ptCenter;
				CustomTemp.ptEnd = m_vecRebarPts[b].ptEnd;
				CustomTemp.ptStr = m_vecRebarPts[b].ptStr;

				movePoint(vec, CustomTemp.ptStr, spacing);
				movePoint(vec, CustomTemp.ptEnd, spacing);
				movePoint(vec, CustomTemp.ptCenter, spacing);
				vctTemp.push_back(CustomTemp);
			}
		}
		else
		{
	
			vector<pointInfo> vctTTemp;
			vctTTemp.insert(vctTTemp.begin(), vctTemp.begin(), vctTemp.end());
			vctTemp.clear();

			for (int b = 0; b < vctTTemp.size(); b++)
			{
				CustomTemp = vctTTemp.at(b);

				movePoint(vec, CustomTemp.ptStr, spacing);
				movePoint(vec, CustomTemp.ptEnd, spacing);
				movePoint(vec, CustomTemp.ptCenter, spacing);
				vctTemp.push_back(CustomTemp);
			}
		}

		vctCustom.push_back(vctTemp);
	}

	for (auto it = vctCustom.begin();it!= vctCustom.end();it++)
	{
		PIT::PITRebarCurve m_Curve;
		if (m_linestyle == 0)//全为直线
		{
			vector<MSElementDescrP> descrs;
			for (int j = 0; j < (*it).size(); j++)//存储线上所有不重复的点
			{
				if ((*it)[j].curType == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Line)
				{
					EditElementHandle lineeeh;
					LineHandler::CreateLineElement(lineeeh, nullptr, DSegment3d::From((*it)[j].ptStr, (*it)[j].ptEnd), true, *ACTIVEMODEL);
					MSElementDescrP linedescr = lineeeh.ExtractElementDescr();
					descrs.push_back(linedescr);
				}
				else
				{
					EditElementHandle arceeh;
					ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc((*it)[j].ptStr, (*it)[j].ptMid, (*it)[j].ptEnd), true, *ACTIVEMODEL);
					MSElementDescrP arcdescr = arceeh.ExtractElementDescr();
					descrs.push_back(arcdescr);
				}
			}
			RebarVertices&  vers = m_Curve.PopVertices();
			ArcRebarTool::GetArcOrLineRebarVertices(vers, descrs, bendRadius);
			for (MSElementDescrP tmpdescr : descrs)
			{
				mdlElmdscr_freeAll(&tmpdescr);
				tmpdescr = NULL;
			}
			/*RebarVertices  vers;
			bvector<DPoint3d> allPts;
			for (int i = 0; i < (*it).size(); i++)
			{
				if (allPts.size() == 0)
				{
					allPts.push_back((*it)[i].ptStr);
					allPts.push_back((*it)[i].ptEnd);
				}
				else
				{
					allPts.push_back((*it)[i].ptEnd);
				}
			}
			GetRebarVerticesFromPoints(vers, allPts, bendRadius);
			m_Curve.SetVertices(vers);*/
		}
		else if (m_linestyle == 1)//全为弧线
		{
			vector<MSElementDescrP> descrs;
			for (int j = 0; j < (*it).size(); j++)//存储线上所有不重复的点
			{
				if ((*it)[j].curType == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Line)
				{
					EditElementHandle lineeeh;
					LineHandler::CreateLineElement(lineeeh, nullptr, DSegment3d::From((*it)[j].ptStr, (*it)[j].ptEnd), true, *ACTIVEMODEL);
					MSElementDescrP linedescr = lineeeh.ExtractElementDescr();
					descrs.push_back(linedescr);
				}
				else
				{
					EditElementHandle arceeh;
					ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc((*it)[j].ptStr, (*it)[j].ptMid, (*it)[j].ptEnd), true, *ACTIVEMODEL);
					MSElementDescrP arcdescr = arceeh.ExtractElementDescr();
					descrs.push_back(arcdescr);
				}
			}
			RebarVertices&  vers = m_Curve.PopVertices();
			ArcRebarTool::GetArcOrLineRebarVertices(vers, descrs, bendRadius);
			for (MSElementDescrP tmpdescr : descrs)
			{
				mdlElmdscr_freeAll(&tmpdescr);
				tmpdescr = NULL;
			}
			//for (int j = 0; j < (*it).size(); j++)//存储线上所有不重复的点
			//{
			//	double dis1;
			//	dis1 = 0;
			//	EditElementHandle arceeh;
			//	//ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromArcCenterStartEnd((*it)[j].ptCenter, (*it)[j].ptStr, (*it)[j].ptEnd), true, *ACTIVEMODEL);
			//	ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc((*it)[j].ptStr, (*it)[j].ptMid, (*it)[j].ptEnd), true, *ACTIVEMODEL);
			//	//arceeh.AddToModel();
			//	mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &(*it)[j].ptEnd, 0.1);
			//	DPoint3d tmpMid;

			//	double dis2 = 0.00;

			//	mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, arceeh.GetElementDescrP(), &(*it)[j].ptEnd, 0.1);

			//	dis2 /= 2;

			//	mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis2, arceeh.GetElementDescrP(), 0.1);

			//	CalculateArc(m_Curve, (*it)[j].ptStr, tmpMid, (*it)[j].ptEnd);
			//}
		}
		else
		{
			vector<MSElementDescrP> descrs;
			for (int j = 0; j < (*it).size(); j++)//存储线上所有不重复的点
			{
				if ((*it)[j].curType == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Line)
				{
					EditElementHandle lineeeh;
					LineHandler::CreateLineElement(lineeeh, nullptr, DSegment3d::From((*it)[j].ptStr, (*it)[j].ptEnd), true, *ACTIVEMODEL);
					MSElementDescrP linedescr = lineeeh.ExtractElementDescr();
					descrs.push_back(linedescr);
				}
				else
				{
					EditElementHandle arceeh;
					ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc((*it)[j].ptStr, (*it)[j].ptMid, (*it)[j].ptEnd), true, *ACTIVEMODEL);
					MSElementDescrP arcdescr = arceeh.ExtractElementDescr();
					descrs.push_back(arcdescr);
				}
			}
			RebarVertices&  vers = m_Curve.PopVertices();
			ArcRebarTool::GetArcOrLineRebarVertices(vers,descrs,bendRadius);
			for (MSElementDescrP tmpdescr:descrs)
			{
				mdlElmdscr_freeAll(&tmpdescr);
				tmpdescr = NULL;
			}
		}

// 	else if{//有直线及弧线
// 		EditElementHandle lineEeh;
// 		ChainHeaderHandler::CreateChainHeaderElement(lineEeh, NULL, false, true, *modelRef);
// 		RebarVertices  vers;
// 		for (int i = 0; i < m_vecRebarPts.size(); i++)
// 		{
// 			if (m_vecRebarPts[i].curType == ICurvePrimitive::CURVE_PRIMITIVE_TYPE_Arc)
// 			{
// 				double dis1;
// 				dis1 = 0;
// 				EditElementHandle arceeh;
// 				ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromArcCenterStartEnd(m_vecRebarPts[i].ptCenter, m_vecRebarPts[i].ptStr, m_vecRebarPts[i].ptEnd), true, *ACTIVEMODEL);
// 				arceeh.AddToModel();
// 				mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[i].ptEnd, 0.1);
// 				DPoint3d tmpMid;
// 				double dis2 = 0.00;
// 				mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[i].ptEnd, 0.1);
// 				dis2 /= 2;
// 				mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis2, arceeh.GetElementDescrP(), 0.1);
// 				CalculateArc(vers, m_vecRebarPts[i].ptStr, tmpMid, m_vecRebarPts[i].ptEnd);
// 			}
// 			else if (m_vecRebarPts[i].curType == ICurvePrimitive::CURVE_PRIMITIVE_TYPE_Line)
// 			{
// //				EditElementHandle Lineeh;
// //				LineHandler::CreateLineElement(Lineeh, nullptr, DSegment3d::From(m_vecRebarPts[i].ptStr, m_vecRebarPts[i].ptEnd), true, *ACTIVEMODEL);
// 				// Lineeh.AddToModel();
// //				ChainHeaderHandler::AddComponentElement(lineEeh, Lineeh);
// 
// 				DPoint3d ptVertex1;
// 				DPoint3d ptVertex2;
// 				DPoint3d ptVertex3;
// //				RebarVertices  vers;
// 				RebarVertex*   vertmp;
// 				vertmp = new RebarVertex();
// 				if (i == 0)
// 				{
// 					ptVertex1 = m_vecRebarPts[i].ptStr;
// 					vertmp->SetType(RebarVertex::kStart);
// 					vertmp->SetIP(ptVertex1);
// 					vers.Add(vertmp);
// 				}
// 				else if (i == m_vecRebarPts.size() - 1)
// 				{
// 
// 					vertmp->SetType(RebarVertex::kEnd);
// 					vertmp->SetIP(m_vecRebarPts[i].ptEnd);
// 					vers.Add(vertmp);
// 				}
// 				else
// 				{
// 					ptVertex1 = m_vecRebarPts[i].ptStr;
// 					ptVertex2 = m_vecRebarPts[i].ptEnd;
// 					ptVertex3 = m_vecRebarPts[i + 1].ptStr;
// 					DPoint3d vec1 = ptVertex1 - ptVertex2;
// 					DPoint3d vec2 = ptVertex3 - ptVertex2;
// 					vec1.Normalize();
// 					vec2.Normalize();
// 
// 					RebarArcData arcdata;
// 					IntersectionPointToArcDataRebar(arcdata, ptVertex2, ptVertex1, ptVertex3, bendRadius);
// 					vertmp->SetType(RebarVertex::kIP);
// 					vertmp->SetIP(ptVertex2);
// 					vertmp->SetArcPt(0, arcdata.ptArcBegin);
// 					vertmp->SetArcPt(1, arcdata.ptArcMid);
// 					vertmp->SetArcPt(2, arcdata.ptArcEnd);
// 					vertmp->SetRadius(bendRadius);
// 					vertmp->SetCenter(arcdata.ptArcCenter);
// 					vers.Add(vertmp);
// 				}
// 			}


		vecRebarCurve.push_back(m_Curve);
	}

	int index = 0;
	int rebarNum = (int)vecRebarCurve.size();
	for (PIT::PITRebarCurve rebarCurve : vecRebarCurve)
	{
		RebarVertexP vex2;
		vex2 = rebarCurve.PopVertices().GetAt(rebarCurve.PopVertices().GetSize() - 1);
		vex2->SetType(RebarVertex::kEnd);

		RebarSymbology symb;
		string str(strRebarSize);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);

		RebarElementP rebarElement = rebarSet.AssignRebarElement(index, rebarNum, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)strRebarSize);
			shape.SetIsStirrup(false);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndType endType;
			endType.SetType(RebarEndType::kNone);
			RebarEndTypes endTypes = { endType,endType };
			rebarElement->Update(rebarCurve, dim, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string Stype(m_CustomRebarInfo.rebarbsType);
			string Level(m_CustomRebarInfo.rebarLevel);
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Level, m_CustomRebarInfo.rebarType, Stype, modelRef);
			SetRebarHideData(tmprebar, spacing/uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
		index++;
	}
	



// 	RebarVertexP vex2;
// 	vex2 = m_Curve.PopVertices().GetAt(m_Curve.PopVertices().GetSize() - 1);
// 	vex2->SetType(RebarVertex::kEnd);

// 	int index = (int)rebarSet.GetChildElementCount();
// 	int rebarNum = index + 1;
// 
// 	RebarSymbology symb;
// 	symb.SetRebarColor(-1);
// 	symb.SetRebarLevel(TEXT_USERDEFINE_REBAR);
// 	RebarElementP rebarElement = rebarSet.AssignRebarElement(index, rebarNum, symb, modelRef);
// 	if (nullptr != rebarElement)
// 	{
// 		RebarShapeData rebarData;
// 		rebarData.SetSizeKey((LPCTSTR)strRebarSize);
// 		rebarData.SetIsStirrup(false);
// 		rebarData.SetLength(m_Curve.GetLength() / uor_per_mm);
// 		RebarEndType endType;
// 		endType.SetType(RebarEndType::kNone);
// 		RebarEndTypes endTypes = { endType,endType };
// 		rebarElement->Update(m_Curve, dim, endTypes, rebarData, modelRef, false);
// 	}

// 	if (m_linestyle == 0)//全为直线
// 	{
// 		RebarVertices  vers;
// 		bvector<DPoint3d> allPts;
// 		for (int i = 0; i < m_vecRebarPts.size(); i++)//存储线上所有不重复的点
// 		{
// 			if (allPts.size() == 0)
// 			{
// 				allPts.push_back(m_vecRebarPts[i].ptStr);
// 				allPts.push_back(m_vecRebarPts[i].ptEnd);
// 			}
// 			else
// 			{
// 				allPts.push_back(m_vecRebarPts[i].ptEnd);
// 			}
// 		}
// 		GetRebarVerticesFromPoints(vers, allPts, bendRadius);
// 		m_Curve.SetVertices(vers);
// 	}
// 	else if (m_linestyle == 1)//全为弧线
// 	{
// 		for (int j = 0; j < m_vecRebarPts.size(); j++)//存储线上所有不重复的点
// 		{
// 			double dis1;
// 			dis1 = 0;
// 			EditElementHandle arceeh;
// 			ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromArcCenterStartEnd(m_vecRebarPts[j].ptCenter, m_vecRebarPts[j].ptStr, m_vecRebarPts[j].ptEnd), true, *ACTIVEMODEL);
// 			arceeh.AddToModel();
// 			mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[j].ptEnd, 0.1);
// 			DPoint3d tmpMid;
// 
// 			double dis2 = 0.00;
// 
// 			mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[j].ptEnd, 0.1);
// 
// 			dis2 /= 2;
// 
// 			mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis2, arceeh.GetElementDescrP(), 0.1);
// 
// 			CalculateArc(m_Curve, m_vecRebarPts[j].ptStr, tmpMid, m_vecRebarPts[j].ptEnd);
// 		}
// 	}
// 	else
// 	{//有直线及弧线
// //		EditElementHandle lineEeh;
// //		ChainHeaderHandler::CreateChainHeaderElement(lineEeh, NULL, false, true, *modelRef);
// 		RebarVertices  vers;
// 		for (int i = 0; i < m_vecRebarPts.size(); i++)
// 		{
// 			if (m_vecRebarPts[i].curType == ICurvePrimitive::CURVE_PRIMITIVE_TYPE_Arc)
// 			{
// 				double dis1;
// 				dis1 = 0;
// 				EditElementHandle arceeh;
// 				ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromArcCenterStartEnd(m_vecRebarPts[i].ptCenter, m_vecRebarPts[i].ptStr, m_vecRebarPts[i].ptEnd), true, *ACTIVEMODEL);
// 				arceeh.AddToModel();
// 				mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[i].ptEnd, 0.1);
// 				DPoint3d tmpMid;
// 				double dis2 = 0.00;
// 				mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[i].ptEnd, 0.1);
// 				dis2 /= 2;
// 				mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis2, arceeh.GetElementDescrP(), 0.1);
// 				CalculateArc(vers, m_vecRebarPts[i].ptStr, tmpMid, m_vecRebarPts[i].ptEnd);
// 			}
// 			else if (m_vecRebarPts[i].curType == ICurvePrimitive::CURVE_PRIMITIVE_TYPE_Line)
// 			{
// //				EditElementHandle Lineeh;
// //				LineHandler::CreateLineElement(Lineeh, nullptr, DSegment3d::From(m_vecRebarPts[i].ptStr, m_vecRebarPts[i].ptEnd), true, *ACTIVEMODEL);
// 				// Lineeh.AddToModel();
// //				ChainHeaderHandler::AddComponentElement(lineEeh, Lineeh);
// 
// 				DPoint3d ptVertex1;
// 				DPoint3d ptVertex2;
// 				DPoint3d ptVertex3;
// //				RebarVertices  vers;
// 				RebarVertex*   vertmp;
// 				vertmp = new RebarVertex();
// 				if (i == 0)
// 				{
// 					ptVertex1 = m_vecRebarPts[i].ptStr;
// 					vertmp->SetType(RebarVertex::kStart);
// 					vertmp->SetIP(ptVertex1);
// 					vers.Add(vertmp);
// 				}
// 				else if (i == m_vecRebarPts.size() - 1)
// 				{
// 
// 					vertmp->SetType(RebarVertex::kEnd);
// 					vertmp->SetIP(m_vecRebarPts[i].ptEnd);
// 					vers.Add(vertmp);
// 				}
// 				else
// 				{
// 					ptVertex1 = m_vecRebarPts[i].ptStr;
// 					ptVertex2 = m_vecRebarPts[i].ptEnd;
// 					ptVertex3 = m_vecRebarPts[i + 1].ptStr;
// 					DPoint3d vec1 = ptVertex1 - ptVertex2;
// 					DPoint3d vec2 = ptVertex3 - ptVertex2;
// 					vec1.Normalize();
// 					vec2.Normalize();
// 
// 					RebarArcData arcdata;
// 					IntersectionPointToArcDataRebar(arcdata, ptVertex2, ptVertex1, ptVertex3, bendRadius);
// 					vertmp->SetType(RebarVertex::kIP);
// 					vertmp->SetIP(ptVertex2);
// 					vertmp->SetArcPt(0, arcdata.ptArcBegin);
// 					vertmp->SetArcPt(1, arcdata.ptArcMid);
// 					vertmp->SetArcPt(2, arcdata.ptArcEnd);
// 					vertmp->SetRadius(bendRadius);
// 					vertmp->SetCenter(arcdata.ptArcCenter);
// 					vers.Add(vertmp);
// 				}
// 			}
// 			else
// 			{;}
// 		}
// 
// 		m_Curve.SetVertices(vers);
// // 		ChainHeaderHandler::AddComponentComplete(lineEeh);
// // 		lineEeh.AddToModel();
// 	}

// 	EditElementHandle lineEeh;
// 	ChainHeaderHandler::CreateChainHeaderElement(lineEeh, NULL, false, true, *modelRef);
//  	for (int i = 0; i < m_vecRebarPts.size(); i++)
//  	{
//  		if (m_vecRebarPts[i].curType == ICurvePrimitive::CURVE_PRIMITIVE_TYPE_Arc)
//  		{
//  			double dis1;
//  			dis1 = 0;
//  			EditElementHandle arceeh;
//  			ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromArcCenterStartEnd(m_vecRebarPts[i].ptCenter, m_vecRebarPts[i].ptStr, m_vecRebarPts[i].ptEnd), true, *ACTIVEMODEL);
//  			arceeh.AddToModel();
//  			mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[i].ptEnd, 0.1);
//  			DPoint3d tmpMid;
//  
//  			double dis2 = 0.00;
//  
//  			mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, arceeh.GetElementDescrP(), &m_vecRebarPts[i].ptEnd, 0.1);
//  
//  			dis2 /= 2;
//  
//  			mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis2, arceeh.GetElementDescrP(), 0.1);
//  
//  			CalculateArc(m_Curve, m_vecRebarPts[i].ptStr, tmpMid, m_vecRebarPts[i].ptEnd);
//  		}
// 		else if (m_vecRebarPts[i].curType == ICurvePrimitive::CURVE_PRIMITIVE_TYPE_Line)
// 		{
// 			EditElementHandle Lineeh;
// 			LineHandler::CreateLineElement(Lineeh, nullptr, DSegment3d::From(m_vecRebarPts[i].ptStr, m_vecRebarPts[i].ptEnd), true, *ACTIVEMODEL);
// 			// Lineeh.AddToModel();
// 			ChainHeaderHandler::AddComponentElement(lineEeh, Lineeh);
// 			RebarVertices  vers;
// 			bvector<DPoint3d> allPts;
// 			for (int j = 0; j < m_vecRebarPts.size(); j++)
// 			{
// 				if (allPts.size() == 0)
// 				{
// 					allPts.push_back(m_vecRebarPts[j].ptStr);
// 					allPts.push_back(m_vecRebarPts[j].ptEnd);
// 				}
// 				else
// 				{
// 					allPts.push_back(m_vecRebarPts[j].ptEnd);
// 				}
// 			}
// 			GetRebarVerticesFromPoints(vers, allPts, bendRadius);
// 			m_Curve.SetVertices(vers);
// 			break;
// // 			RebarVertexP vex;
// // 			RebarVertexP vex1;
// // 			RebarVertexP vex3;
// // 			if (m_Curve.PopVertices().GetSize() == 0)
// // 			{
// // 				vex = &m_Curve.PopVertices().NewElement();
// // 				vex->SetIP(m_vecRebarPts[i].ptStr);
// // 				vex->SetType(RebarVertex::kStart);
// // 
// // 				vex1 = &m_Curve.PopVertices().NewElement();
// // 				vex1->SetIP(m_vecRebarPts[i].ptEnd);
// // 				vex1->SetRadius(bendRadius);
// // 				vex1->SetType(RebarVertex::kIP);
// // 			}
// // 			else
// // 			{
// // 				vex3 = &m_Curve.PopVertices().NewElement();
// // 				vex3->SetIP(m_vecRebarPts[i].ptEnd);
// // 				vex3->SetRadius(bendRadius);
// // 				vex3->SetType(RebarVertex::kIP);
// // 
// // 			}
// 		}
// 		else
// 		{;}
// 	}
// 
// 	ChainHeaderHandler::AddComponentComplete(lineEeh);
// 	lineEeh.AddToModel();

// 	RebarVertexP vex2;
// 	vex2 = m_Curve.PopVertices().GetAt(m_Curve.PopVertices().GetSize() - 1);
// 	vex2->SetType(RebarVertex::kEnd);

//for (int i = 1; i < m_Curve.PopVertices().GetSize() - 1; i++)
	//{
	//	m_Curve.PopVertices()[i]->EvaluateBend(*m_Curve.PopVertices()[i - 1], *m_Curve.PopVertices()[i + 1]);
	//}
	
// 	int index = (int)rebarSet.GetChildElementCount();
// 	int rebarNum = index + 1;
// 
// 	RebarSymbology symb;
// 	symb.SetRebarColor(-1);
// 	symb.SetRebarLevel(TEXT_USERDEFINE_REBAR);
// 	RebarElementP rebarElement = rebarSet.AssignRebarElement(index, rebarNum, symb, modelRef);
// 	if (nullptr != rebarElement)
// 	{
// 		RebarShapeData rebarData;
// 		rebarData.SetSizeKey((LPCTSTR)strRebarSize);
// 		rebarData.SetIsStirrup(false);
// 		rebarData.SetLength(m_Curve.GetLength() / uor_per_mm);
// 		RebarEndType endType;
// 		endType.SetType(RebarEndType::kNone);
// 		RebarEndTypes endTypes = { endType,endType };
// 		rebarElement->Update(m_Curve, dim, endTypes, rebarData, modelRef, false);
// 	}
// 	return rebarElement;
}







RebarSetTag * CustomRebarAssembly::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return nullptr;
	}

	RebarSetP rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->StartUpdate(modelRef);

	if (rebarSet == nullptr)
	{
		return nullptr;
	}

	int rebarNum = (int)m_vecCustomRebar.size();
	for (int i = 0; i < rebarNum; ++i)
	{
		m_vecCustomRebar[i]->Create(*rebarSet);
	}

	RebarSetTag *tag = new RebarSetTag;
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);
	return tag;
}


