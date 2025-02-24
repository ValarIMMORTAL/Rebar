#include "_USTATION.h"
#include "../resource.h"
#include <SelectionRebar.h>
#include "../GalleryIntelligentRebar.h"
#include "CCNCutRebarAssembly.h"
#include "CCNCutRebarDlg.h"
#include "ExtractFacesTool.h"
#include "ElementAttribute.h"
#include "PITMSCECommon.h"
#include "ExtractFacesTool.h"
#include "../XmlHelper.h"
#include "../SelectRebarTool.h"
#include "ElementAttribute.h"

void CCutRebarAssembly::CalcRebarSet()
{
	RebarSet * rebset = nullptr;
	for (ElementRefP curRef : m_selectrebars)
	{
		EditElementHandle curElement(curRef, curRef->GetDgnModelP());
		if (RebarElement::IsRebarElement(curElement))
		{
			RebarElementP rep = RebarElement::Fetch(curElement);
			rebset = rep->GetRebarSet(ACTIVEMODEL);

			auto itr_Find = map_RebarSet.find(rebset->GetElementId());
			if (itr_Find == map_RebarSet.end())
			{
				std::vector<ElementRefP> vecElement;
				vecElement.push_back(curRef);

				map_RebarSet.insert(make_pair(rebset->GetElementId(), vecElement));
			}
			else
			{
				itr_Find->second.push_back(curRef);
			}
		}
	}
}

void CCutRebarAssembly::CalaRebarStartEnd(RebarElementP rep, DPoint3d& PtStar, DPoint3d& PtEnd, double& diameter, DgnModelRefP modelRef)
{
	RebarCurve curve;
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	RebarShape * rebarshape = rep->GetRebarShape(modelRef);
	if (rebarshape == nullptr)
	{
		return;
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
}

double CCutRebarAssembly::CalaRebarLength(RebarElementP pRebar, DgnModelRefP modelRef)
{
	double dLength = 0.0;
	if (pRebar == NULL)
	{
		return dLength;
	}

	RebarShape* rebarshape = pRebar->GetRebarShape(modelRef);
	if (rebarshape == nullptr)
	{
		return dLength;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	Transform trans;
	mdlTMatrix_getIdentity(&trans);
	CurveVectorPtr profile = CurveVector::Create(CurveVector::BOUNDARY_TYPE_None);
	rebarshape->GenerateCenterline(*profile, uor_per_mm, &trans);

	for (unsigned int i = 0; i < profile->size(); i++)
	{
		ICurvePrimitivePtr priPtr = profile->at(i);

		if (priPtr->GetCurvePrimitiveType() == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Line)
		{
			DSegment3dCP seg = priPtr->GetLineCP();
			dLength += seg->Length();
		}
		else if (priPtr->GetCurvePrimitiveType() == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_LineString)
		{
			bvector<DPoint3d> const* ptr = priPtr->GetLineStringCP();
			for (int j = 0; j < (int)ptr->size() - 1; j++)
			{
				DPoint3d ptStr, ptEnd;
				ptStr = ptr->at(j);
				ptEnd = ptr->at(j + 1);
				dLength += ptStr.Distance(ptEnd);
			}
		}
		else if (priPtr->GetCurvePrimitiveType() == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Arc)
		{
			DEllipse3dCP ptrArc = priPtr->GetArcCP();
			// priPtr->TryGetArc(ptrArc);

			EditElementHandle eehArc;
			//画圆弧
			if (SUCCESS != ArcHandler::CreateArcElement(eehArc, NULL, *ptrArc, true, *modelRef))
			{
				return dLength;
			}

			DPoint3d ArcDPs[2];
			mdlArc_extract(ArcDPs, NULL, NULL, NULL, NULL, NULL, NULL, &eehArc.GetElementDescrP()->el);
			// 取弧线弧长
			double arcLength = 0.0;
			mdlElmdscr_distanceAtPoint(&arcLength, nullptr, nullptr, eehArc.GetElementDescrP(), &ArcDPs[1], 0.1);

			dLength += arcLength;

		}
	}

	return dLength;
}

// 取rebarCur的GetIP()点
bool CCutRebarAssembly::GetArcIpPoint(DPoint3dR ptIP, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
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
			}
			ptIP = ip;
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

			ptIP = ip;
		}
		ret = true;
	}

	return ret;
}

//********************************************************************
// Method:    MakeCutRebarCureOneLen
// FullName:  CCutRebarAssembly::MakeCutRebarCureOneLen 
// Access:    public 
// Returns:   void
// Qualifier: 生成一段截断的钢筋
// Parameter: vector<PIT::PITRebarCurve> & rebarCurvesNum
// Parameter: RebarElementP pRebar	原来的钢筋
// Parameter: double startLen		截断起点
// Parameter: double endLen			截断终点
//*******************************************************************
void CCutRebarAssembly::MakeCutRebarCureOneLen(vector<PIT::PITRebarCurve>& rebarCurvesNum, RebarElementP pRebar, double startLen, double endLen)
{
	if (pRebar == NULL)
	{
		return;
	}

	DgnModelRefP modelRef = ACTIVEMODEL;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	// 长度太小
	if (COMPARE_VALUES_EPS(endLen - startLen, uor_per_mm, EPS) <= 0)
	{
		return;
	}

	RebarCurve curveOld;
	RebarShape* rebarshape = pRebar->GetRebarShape(modelRef);
	if (rebarshape == nullptr)
	{
		return;
	}
	rebarshape->GetRebarCurve(curveOld);
	CMatrix3D tmp3d(pRebar->GetLocation());
	curveOld.MatchRebarCurve(tmp3d, curveOld, uor_per_mm);
	curveOld.DoMatrix(pRebar->GetLocation());

	RebarVertices  versOld;   // 钢筋顶点
	versOld = curveOld.GetVertices();

	RebarVertices  versNew;   // 钢筋顶点
	PIT::PITRebarCurve curveNew;

	double dCurrLength = 0.0; // 当前钢筋长度
	RebarVertexP verNode = NULL;
	for (int i = 0; i < (int)versOld.GetSize() - 1; i++)
	{
		RebarVertexP ver = versOld.GetAt(i);
		RebarVertexP ver_next = versOld.GetAt(i + 1);

		// 当前点是弧线
		if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) != 0)
		{
			DEllipse3d ptrArc = DEllipse3d::FromArcCenterStartEnd(ver->GetCenter(), ver->GetArcPt()[0], ver->GetArcPt()[2]);

			EditElementHandle eehArc;
			//画圆弧
			if (SUCCESS != ArcHandler::CreateArcElement(eehArc, NULL, ptrArc, true, *modelRef))
			{
				return;
			}

			// 取弧线弧长
			double arcLength = 0.0;
			DPoint3d arcEnd = ver->GetArcPt()[2];
			mdlElmdscr_distanceAtPoint(&arcLength, nullptr, nullptr, eehArc.GetElementDescrP(), &arcEnd, 0.1);
			//end

			if (COMPARE_VALUES_EPS(dCurrLength + arcLength, startLen, EPS) < 0) // 当前钢筋长度小于起点长度
			{
				dCurrLength += arcLength;
				continue;
			}

			double moveStr = 0.0;
			if (COMPARE_VALUES_EPS(startLen, dCurrLength, EPS) > 0)
			{
				moveStr = startLen - dCurrLength; // 移动到截断弧起点的距离
			}
	
			if (versNew.GetSize() == 0)
			{
				verNode = new RebarVertex();
				DPoint3d ptArcMid = DPoint3d::FromZero();
				mdlElmdscr_pointAtDistance(&ptArcMid, nullptr, moveStr, eehArc.GetElementDescrP(), 0.1);

				verNode->SetIP(ptArcMid);
				verNode->SetType(RebarVertex::kStart);
				versNew.Add(verNode);
			}

			verNode = new RebarVertex();
			verNode = ver;

			dCurrLength += arcLength;
			// 当前钢筋长度大于终点长度 或 截断弧起点在中间 就需要重新画弧
			if (COMPARE_VALUES_EPS(moveStr, 0.0, EPS) > 0 || COMPARE_VALUES_EPS(dCurrLength, endLen, EPS) >= 0)
			{
				double moveLength = 0.0;
				if (COMPARE_VALUES_EPS(dCurrLength, endLen, EPS) >= 0)
				{
					moveLength = arcLength - (dCurrLength - endLen) - moveStr; // 截断钢筋的弧长
				}
				else
				{
					moveLength = arcLength - moveStr; // 截断钢筋的弧长
				}

				DPoint3d arcStr = DPoint3d::FromZero();	// 截断弧起点
				mdlElmdscr_pointAtDistance(&arcStr, nullptr, moveStr, eehArc.GetElementDescrP(), 0.1);
				verNode->SetArcPt(0, arcStr);

				DPoint3d arcMid = DPoint3d::FromZero(); // 截断弧中点
				mdlElmdscr_pointAtDistance(&arcMid, nullptr, moveLength * 0.5 + moveStr, eehArc.GetElementDescrP(), 0.1);
				verNode->SetArcPt(1, arcMid);

				DPoint3d arcEnd = DPoint3d::FromZero(); // 截断弧终点
				mdlElmdscr_pointAtDistance(&arcEnd, nullptr, moveLength + moveStr, eehArc.GetElementDescrP(), 0.1);
				verNode->SetArcPt(2, arcEnd);

				// 计算SetIP点
				DPoint3d ptIPoint = DPoint3d::FromZero();
				GetArcIpPoint(ptIPoint, arcStr, arcMid, arcEnd);
				verNode->SetIP(ptIPoint);
				// end

				//EditElementHandle eehArcTemp;
				//if (SUCCESS == ArcHandler::CreateArcElement(eehArcTemp, NULL, DEllipse3d::FromPointsOnArc(arcStr, arcMid, arcEnd), true, *modelRef))
				//{
				//	eehArcTemp.AddToModel();
				//}

				verNode->SetType(RebarVertex::kIP);
				versNew.Add(verNode);

				if (COMPARE_VALUES_EPS(dCurrLength, endLen, EPS) >= 0)
				{
					verNode = new RebarVertex();
					verNode->SetIP(arcEnd);
					verNode->SetType(RebarVertex::kEnd);
					versNew.Add(verNode);
					break;
				}
			}
			else
			{
				versNew.Add(verNode);
			}
		}
		// 前后两段 直线 -- 直线
		else if (COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) == 0)
		{
			EditElementHandle eehTmp;
			if (SUCCESS != LineHandler::CreateLineElement(eehTmp, NULL, DSegment3d::From(ver->GetIP(), ver_next->GetIP()), true, *modelRef))
			{
				return;
			}
			
			double curLength = ver->GetIP().Distance(ver_next->GetIP()); // 当前段长度
			if (COMPARE_VALUES_EPS(dCurrLength + curLength, startLen, EPS) < 0) // 当前钢筋长度小于起点长度
			{
				dCurrLength += curLength;
				continue;
			}

			double moveStr = 0.0;
			if (COMPARE_VALUES_EPS(startLen, dCurrLength, EPS) > 0)
			{
				moveStr = startLen - dCurrLength; // 移动到截断弧起点的距离
			}

			if (versNew.GetSize() == 0)
			{
				verNode = new RebarVertex();
				DPoint3d ptTemp = ver->GetIP();
				movePoint(ver_next->GetIP() - ver->GetIP(), ptTemp, moveStr);
				verNode->SetIP(ptTemp);
				verNode->SetType(RebarVertex::kStart);
				versNew.Add(verNode);
			}

			dCurrLength += curLength;
			verNode = new RebarVertex();
			verNode = ver;

			// 当前钢筋长度大于终点长度
			if (COMPARE_VALUES_EPS(dCurrLength, endLen, EPS) >= 0)
			{
				DPoint3d ptTemp = ver->GetIP();
				double moveLength = curLength - (dCurrLength - endLen) - moveStr;
				movePoint(ver_next->GetIP() - ver->GetIP(), ptTemp, moveLength + moveStr);

				verNode = new RebarVertex();
				verNode->SetIP(ptTemp);
				verNode->SetType(RebarVertex::kEnd);
				versNew.Add(verNode);
				break;
			}
			versNew.Add(verNode);
		}
		// 前后两段 直线 -- 弧线
		else if (COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) != 0)
		{
			EditElementHandle eehTmp;
			if (SUCCESS != LineHandler::CreateLineElement(eehTmp, NULL, DSegment3d::From(ver->GetIP(), ver_next->GetArcPt(0)), true, *modelRef))
			{
				return;
			}

			double curLength = ver->GetIP().Distance(ver_next->GetArcPt(0)); // 当前段长度
			if (COMPARE_VALUES_EPS(dCurrLength + curLength, startLen, EPS) < 0) // 当前钢筋长度小于起点长度
			{
				dCurrLength += curLength;
				continue;
			}

			double moveStr = 0.0;
			if (COMPARE_VALUES_EPS(startLen, dCurrLength, EPS) > 0)
			{
				moveStr = startLen - dCurrLength; // 移动到截断弧起点的距离
			}

			if (versNew.GetSize() == 0)
			{
				verNode = new RebarVertex();
				DPoint3d ptTemp = ver->GetIP();
				movePoint(ver_next->GetArcPt(0) - ver->GetIP(), ptTemp, moveStr);
				verNode->SetIP(ptTemp);
				verNode->SetType(RebarVertex::kStart);
				versNew.Add(verNode);
			}

			dCurrLength += curLength;
			verNode = new RebarVertex();
			verNode = ver;

			// 当前钢筋长度大于终点长度
			if (COMPARE_VALUES_EPS(dCurrLength, endLen, EPS) >= 0)
			{
				DPoint3d ptTemp = ver->GetIP();
				double moveLength = curLength - (dCurrLength - endLen) - moveStr;
				movePoint(ver_next->GetArcPt(0) - ver->GetIP(), ptTemp, moveLength + moveStr);

				verNode = new RebarVertex();
				verNode->SetIP(ptTemp);
				verNode->SetType(RebarVertex::kEnd);
				versNew.Add(verNode);
				break;
			}
			// 后面一段是弧线，所以这个钢筋顶点不用在Add
			if (versNew.GetSize() != 1)
			{
				versNew.Add(verNode);
			}
		}

		if (ver_next->GetType() == RebarVertex::kEnd)
		{
			if (COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) != 0) // 画弧线
			{
				DEllipse3d ptrArc = DEllipse3d::FromArcCenterStartEnd(ver_next->GetCenter(), ver_next->GetArcPt()[0], ver_next->GetArcPt()[2]);;
				EditElementHandle eehArc;
				//画圆弧
				if (SUCCESS != ArcHandler::CreateArcElement(eehArc, NULL, ptrArc, true, *modelRef))
				{
					return;
				}
				// 取弧线弧长
				double arcLength = 0.0;
				DPoint3d arcEnd = ver_next->GetArcPt()[2];
				mdlElmdscr_distanceAtPoint(&arcLength, nullptr, nullptr, eehArc.GetElementDescrP(), &arcEnd, 0.1);
				//end

				// 当前钢筋长度小于起点长度
				if (COMPARE_VALUES_EPS(dCurrLength + arcLength, startLen, EPS) < 0)
				{
					dCurrLength += arcLength;
					continue;
				}
				double moveStr = 0.0;
				if (COMPARE_VALUES_EPS(startLen, dCurrLength, EPS) > 0)
				{
					moveStr = startLen - dCurrLength; // 移动到截断弧起点的距离
				}
				if (versNew.GetSize() == 0)
				{
					verNode = new RebarVertex();
					DPoint3d ptArcMid = DPoint3d::FromZero();
					mdlElmdscr_pointAtDistance(&ptArcMid, nullptr, moveStr, eehArc.GetElementDescrP(), 0.1);

					verNode->SetIP(ptArcMid);
					verNode->SetType(RebarVertex::kStart);
					versNew.Add(verNode);
				}

				verNode = new RebarVertex();
				verNode = ver_next;

				dCurrLength += arcLength;
				// 当前钢筋长度大于终点长度 或 截断弧起点在中间 就需要重新画弧
				if (COMPARE_VALUES_EPS(moveStr, 0.0, EPS) > 0 || COMPARE_VALUES_EPS(dCurrLength, endLen, EPS) >= 0)
				{
					double moveLength = 0.0;
					if (COMPARE_VALUES_EPS(dCurrLength, endLen, EPS) >= 0)
					{
						moveLength = arcLength - (dCurrLength - endLen) - moveStr; // 截断钢筋的弧长
					}
					else
					{
						moveLength = arcLength - moveStr; // 截断钢筋的弧长
					}

					DPoint3d arcStr = DPoint3d::FromZero();	// 截断弧起点
					mdlElmdscr_pointAtDistance(&arcStr, nullptr, moveStr, eehArc.GetElementDescrP(), 0.1);
					verNode->SetArcPt(0, arcStr);

					DPoint3d arcMid = DPoint3d::FromZero(); // 截断弧中点
					mdlElmdscr_pointAtDistance(&arcMid, nullptr, moveLength * 0.5 + moveStr, eehArc.GetElementDescrP(), 0.1);
					verNode->SetArcPt(1, arcMid);

					DPoint3d arcEnd = DPoint3d::FromZero(); // 截断弧终点
					mdlElmdscr_pointAtDistance(&arcEnd, nullptr, moveLength + moveStr, eehArc.GetElementDescrP(), 0.1);
					verNode->SetArcPt(2, arcEnd);

					// 计算SetIP点
					DPoint3d ptIPoint = DPoint3d::FromZero();
					GetArcIpPoint(ptIPoint, arcStr, arcMid, arcEnd);
					verNode->SetIP(ptIPoint);
					// end

					//EditElementHandle eehArcTemp;
					//if (SUCCESS == ArcHandler::CreateArcElement(eehArcTemp, NULL, DEllipse3d::FromPointsOnArc(arcStr, arcMid, arcEnd), true, *modelRef))
					//{
					//	eehArcTemp.AddToModel();
					//}

					verNode->SetType(RebarVertex::kIP);
					versNew.Add(verNode);

					if (COMPARE_VALUES_EPS(dCurrLength, endLen, EPS) >= 0)
					{
						verNode = new RebarVertex();
						verNode->SetIP(arcEnd);
						verNode->SetType(RebarVertex::kEnd);
						versNew.Add(verNode);
						break;
					}
				}
				else
				{
					versNew.Add(verNode);
				}
			}
			else if(COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) != 0) // 前后两段 弧线 -- 直线
			{
				EditElementHandle eehTmp;
				if (SUCCESS != LineHandler::CreateLineElement(eehTmp, NULL, DSegment3d::From(ver->GetArcPt(2), ver_next->GetIP()), true, *modelRef))
				{
					return;
				}

				double curLength = ver->GetArcPt(2).Distance(ver_next->GetIP()); // 当前段长度
				if (COMPARE_VALUES_EPS(dCurrLength + curLength, startLen, EPS) < 0) // 当前钢筋长度小于起点长度
				{
					dCurrLength += curLength;
					continue;
				}

				double moveStr = 0.0;
				if (COMPARE_VALUES_EPS(startLen, dCurrLength, EPS) > 0)
				{
					moveStr = startLen - dCurrLength; // 移动到截断弧起点的距离
				}

				if (versNew.GetSize() == 0)
				{
					verNode = new RebarVertex();
					DPoint3d ptTemp = ver->GetArcPt(2);
					movePoint(ver_next->GetIP() - ver->GetArcPt(2), ptTemp, moveStr);
					verNode->SetIP(ptTemp);
					verNode->SetType(RebarVertex::kStart);

					versNew.Add(verNode);
				}

				dCurrLength += curLength;
				verNode = new RebarVertex();
				verNode = ver;

				// 当前钢筋长度小于起点长度
				if (COMPARE_VALUES_EPS(dCurrLength, endLen, EPS) >= 0)
				{
					DPoint3d ptTemp = ver->GetArcPt(2);
					double moveLength = curLength - (dCurrLength - endLen) - moveStr;
					movePoint(ver_next->GetIP() - ver->GetArcPt(2), ptTemp, moveLength + moveStr);

					verNode = new RebarVertex();
					verNode->SetIP(ptTemp);
					verNode->SetType(RebarVertex::kEnd);
					versNew.Add(verNode);
					break;
				}
				versNew.Add(verNode);
			}
		}
	}

	curveNew.SetVertices(versNew);
	rebarCurvesNum.push_back(curveNew);

}

void CCutRebarAssembly::MakeCutRebarCure
(
	vector<CNCutRebarInfo>& vecCutInfo, 
	vector<PIT::PITRebarCurve>& rebarCurvesNum, 
	DPoint3d ptStr,
	DPoint3d ptEnd, 
	double diameter, 
	bool bFlag
)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	DPoint3d ptTemp = ptStr;
	CVector3D vecMove = ptEnd - ptStr;
	for (CNCutRebarInfo stCutInfo : vecCutInfo)
	{
		if (COMPARE_VALUES_EPS(stCutInfo.dLength, 0.0, EPS) == 0)
		{
			continue;
		}

		movePoint(vecMove, ptTemp, stCutInfo.dLength);

		PIT::PITRebarCurve curveNew;
		RebarVertices  vers;
		bvector<DPoint3d> allpts;
		allpts.push_back(ptStr);
		allpts.push_back(ptTemp);

		GetRebarVerticesFromPoints(vers, allpts, diameter);
		curveNew.SetVertices(vers);

		rebarCurvesNum.push_back(curveNew);

		ptStr = ptTemp;
	}
}

RebarSetTag* CCutRebarAssembly::MakeCutRebar(RebarSetP rebarSetOld, std::vector<ElementRefP>& vecRebarRef, ElementId rebarSetId, DgnModelRefP modelRef)
{
	bool const isStirrup = false;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);


	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);

	vector<PIT::PITRebarCurve>     rebarCurvesNum;

	vector<CWriteRebarInfo> vecWriteRebarInfo;

	// 遍历每根需要截断的钢筋
	for (unsigned int i = 0; i < vecRebarRef.size(); i++)
	{
		EditElementHandle eehRebar(vecRebarRef.at(i), modelRef);
		RebarElementP pRebar = RebarElement::Fetch(eehRebar);
		if (pRebar == NULL)
		{
			continue;
		}
		//RebarCurve curve;
		RebarShape * rebarshape = pRebar->GetRebarShape(modelRef);
		if (rebarshape == nullptr)
		{
			return false;
		}

		// 取钢筋直径和尺寸
		//rebarshape->GetRebarCurve(curve);
		BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
		double diameter = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);

		// 钢筋长度 中心线
		double dRebarLength = CalaRebarLength(pRebar, modelRef);

		// 存入钢筋截断信息，防止长度越界 此处长度 乘以了单位像素
		std::vector<CNCutRebarInfo> vecCutInfo;
		vecCutInfo = GetvecCutInfo();
		for (int j = 0; j < vecCutInfo.size(); j++)
		{
			if (COMPARE_VALUES_EPS(vecCutInfo.at(j).dLength * uor_per_mm, dRebarLength, EPS) < 0)
			{
				dRebarLength -= vecCutInfo.at(j).dLength * uor_per_mm;
				vecCutInfo.at(j).dLength = vecCutInfo.at(j).dLength * uor_per_mm;
			}
			else // 已截段长度大于钢筋长度剩余长度
			{
				vecCutInfo.at(j).dLength = dRebarLength;
				dRebarLength = 0.0;
			}
		}
		// 最后还剩余长度
		if (COMPARE_VALUES_EPS(dRebarLength, 0.0, EPS) > 0 && vecCutInfo.size() > 0)
		{
			vecCutInfo.push_back({ (int)vecCutInfo.size() + 1,  dRebarLength , vecCutInfo.at(0).nVecType });
		}
		// end

		if (vecCutInfo.size() == 0)
		{
			continue;
		}

		// 反向截断，将vecCutInfo的长度反过来
		if (GetisReserveCut() && vecCutInfo.size() > 0)
		{
			int nLeft = 0;
			int nRight = vecCutInfo.size() - 1;
			while (nRight > nLeft)
			{
				double dTemp = vecCutInfo[nRight].dLength;
				vecCutInfo[nRight].dLength = vecCutInfo[nLeft].dLength;
				vecCutInfo[nLeft].dLength = dTemp;

				nLeft++;
				nRight--;
			}
		}
		// end

		//// 存储每根钢筋写入信息 与 rebarCurvesNum 顺序保持一致
		//for (CNCutRebarInfo stCutInfo : vecCutInfo)
		//{
		//	if (COMPARE_VALUES_EPS(stCutInfo.dLength, 0.0, EPS) == 0)
		//	{
		//		continue;
		//	}
		//	CWriteRebarInfo stWriteInfo;
		//	stWriteInfo.rebarlevel = pRebar->GetRebarElementSymbology().GetRebarLevel();
		//	stWriteInfo.diameter = diameter;
		//	stWriteInfo.sizeKey = Sizekey;
		//	GetRebarLevelItemTypeValue(eehRebar, stWriteInfo.Level, stWriteInfo.rebarType, stWriteInfo.rebarGrade);
		//	vecWriteRebarInfo.push_back(stWriteInfo);
		//}
		//// end

		double startLen = 0.0;
		double endLen = 0.0;
		for (CNCutRebarInfo stCutInfo : vecCutInfo)
		{	
			if (COMPARE_VALUES_EPS(stCutInfo.dLength, 0.0, EPS) == 0)
			{
				continue;
			}
			startLen = endLen;
			endLen += stCutInfo.dLength;

			// 通过钢筋顶点断开钢筋
			MakeCutRebarCureOneLen(rebarCurvesNum, pRebar, startLen, endLen);

			// 存储每根钢筋写入信息 与 rebarCurvesNum 顺序保持一致
			CWriteRebarInfo stWriteInfo;
			stWriteInfo.rebarlevel = pRebar->GetRebarElementSymbology().GetRebarLevel();
			stWriteInfo.diameter = diameter;
			stWriteInfo.sizeKey = Sizekey;
			GetRebarLevelItemTypeValue(eehRebar, stWriteInfo.Level, stWriteInfo.rebarType, stWriteInfo.rebarGrade);
			vecWriteRebarInfo.push_back(stWriteInfo);
			// end 
		}

		// MakeCutRebarCure(vecCutInfo, rebarCurvesNum, ptStr, ptEnd, diameter);
	}

	int numRebar = (int)rebarCurvesNum.size();

	int j = 0;
	for (PIT::PITRebarCurve rebarCurve : rebarCurvesNum)
	{

		RebarSymbology symb;
		SetRebarColorBySize(vecWriteRebarInfo.at(j).sizeKey, symb);
		symb.SetRebarLevel((LPCTSTR)vecWriteRebarInfo.at(j).rebarlevel);

		RebarElementP rebarElement = NULL;
		RebarShapeData shape;
		rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			shape.SetSizeKey((LPCTSTR)vecWriteRebarInfo.at(j).sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);

			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, vecWriteRebarInfo.at(j).diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			int grade = atoi(vecWriteRebarInfo.at(j).rebarGrade.c_str());
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, vecWriteRebarInfo.at(j).Level, grade, 
				vecWriteRebarInfo.at(j).rebarGrade, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(rebarSetOld->GetSetData().GetNominalSpacing());
	setdata.SetAverageSpacing(rebarSetOld->GetSetData().GetAverageSpacing());

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

bool CCutRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	RebarSetTag* tag = NULL;
	bool  isStirrup = false;

	CalcRebarSet();
	
	int nIndex = 0;
	for (auto itr = map_RebarSet.begin(); itr != map_RebarSet.end(); itr++) // 需要截断钢筋所在钢筋组
	{
		PopvecSetId().push_back(0);
		ElementId setId = itr->first;
		RebarSetP   rebarSetP = RebarSet::Fetch(setId, modelRef);
		if (NULL == rebarSetP)
		{
			nIndex++;
			continue;
		}
		tag = MakeCutRebar(rebarSetP, itr->second, GetvecSetId().at(nIndex), modelRef);
		if (NULL != tag)
		{
			tag->SetBarSetTag(nIndex + 1);
			rsetTags.Add(tag);
		}
		nIndex++;
	}

	for (auto itr = map_RebarSet.begin(); itr != map_RebarSet.end(); itr++) // 需要截断钢筋所在钢筋组
	{
		for (unsigned int i = 0; i < itr->second.size(); i++)
		{
			EditElementHandle eehRebar(itr->second.at(i), ACTIVEMODEL);
			eehRebar.DeleteFromModel();
		}
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

bool CCutRebarAssembly::OnDoubleClick()
{
	return true;
}

bool CCutRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();

	SetWallData(ehWall);
	MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();
	return true;
}

long CCutRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}