#include "_ustation.h"
#include <SelectionRebar.h>
#include "TieRebar.h"
#include "BentlyCommonfile.h"
#include "ExtractFacesTool.h"
#include "PITRebarEndType.h"
#include "PITRebarCurve.h"

extern bool PreviewButtonDown;//墙预览按钮标志
extern bool SlabPreviewButtonDown;//板预览按钮标志

// 取外层钢筋方向
void TieRebarMaker::CalaMainAnthorVec(const vector <DSegment3d> &vecPositiveStartEnd_M, const vector <DSegment3d> &vecPositiveStartEnd_A)
{
	if (vecPositiveStartEnd_M.size() > 0)
	{
		DPoint3d pt[2];
		vecPositiveStartEnd_M[0].GetStartPoint(pt[0]);
		vecPositiveStartEnd_M[0].GetEndPoint(pt[1]);
		m_MainVec = pt[1] - pt[0];
		m_MainVec.Normalize();
	}

	if (vecPositiveStartEnd_A.size() > 0)
	{
		DPoint3d pt[2];
		vecPositiveStartEnd_A[0].GetStartPoint(pt[0]);
		vecPositiveStartEnd_A[0].GetEndPoint(pt[1]);
		m_AnthorVec = pt[1] - pt[0];
		m_AnthorVec.Normalize();
	}
}

void TieRebarMaker::MoveRebarLine(EditElementHandle& eehLine, DVec3d vec)
{
	mdlCurrTrans_begin();
	Transform tMatrix;
	mdlTMatrix_getIdentity(&tMatrix);
	mdlTMatrix_setTranslation(&tMatrix, &vec);
	TransformInfo transinfo(tMatrix);
	eehLine.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehLine, transinfo);
	mdlCurrTrans_end();
}

void TieRebarMaker::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

// 弧线钢筋取交点方式
void TieRebarMaker::CalRebarIntersectionPointArc(RebarInsectionPt & rebarInsec, DgnModelRefP modelRef)
{
	if (m_arcVecStartEnd.size() != 4)
	{
		MessageBox(MSWIND, L"提示", L"未设置钢筋网的起点终点！", MB_OK);
		return;
	}
	if (!modelRef)
	{
		return;
	}

	const vector <RebarPoint> &vecPositiveStartEnd_M = m_arcVecStartEnd[0]; // 正面第一层
	const vector <RebarPoint> &vecPositiveStartEnd_A = m_arcVecStartEnd[1]; // 正面第二层
	const vector <RebarPoint> &vecReverseStartEnd_M = m_arcVecStartEnd[3];	 // 背面最后一层
	const vector <RebarPoint> &vecReverseStartEnd_A = m_arcVecStartEnd[2];  // 背面倒数第二层

	double dimPositive_H = RebarCode::GetBarDiameter(m_faceRebarDataArray.posRebarData.HRebarData.rebarSize, modelRef);
	double dimPositive_V = RebarCode::GetBarDiameter(m_faceRebarDataArray.posRebarData.VRebarData.rebarSize, modelRef);
	double dimReserve_H = RebarCode::GetBarDiameter(m_faceRebarDataArray.revRebarData.HRebarData.rebarSize, modelRef);
	double dimReserve_V = RebarCode::GetBarDiameter(m_faceRebarDataArray.revRebarData.VRebarData.rebarSize, modelRef);

	int nTmp = (int)(vecPositiveStartEnd_A.size() - vecReverseStartEnd_A.size());

	double diameter = RebarCode::GetBarDiameter(m_tieRebarSize, modelRef);

	vector<DSegment3d> vecPositiveStartEndSeg_M;
	for (RebarPoint st : vecPositiveStartEnd_M)
	{
		vecPositiveStartEndSeg_M.push_back({ st.ptstr, st.ptend });
	}

	vector<DSegment3d> vecPositiveStartEndSeg_A;
	for (RebarPoint st : vecPositiveStartEnd_A)
	{
		vecPositiveStartEndSeg_A.push_back({ st.ptstr, st.ptend });
	}

	m_MainVec.FromZero();
	m_AnthorVec.FromZero();
	// 取外层钢筋方向
	CalaMainAnthorVec(vecPositiveStartEndSeg_M, vecPositiveStartEndSeg_A);

	EditElementHandle eehLine, eehArc;
	for (size_t i = 0; i < vecReverseStartEnd_M.size(); ++i)
	{
		vector<TieRebarPt> ptIntersecH;
		RebarPoint	stRebarPt = vecReverseStartEnd_M[i];
		const DPoint3d& ptStart_H = stRebarPt.ptstr;
		const DPoint3d& ptEnd_H = stRebarPt.ptend;
		CVector3D vec_H = ptEnd_H - ptStart_H;

		DPoint3d ptArcStr, ptArcEnd, ptArcMid;
		DPoint3d ptLineStr, ptLineEnd;

		if (stRebarPt.vecDir == 0) // 弧形钢筋
		{
			ptArcStr = stRebarPt.ptstr;
			ptArcMid = stRebarPt.ptmid;
			ptArcEnd = stRebarPt.ptend;
		}
		else
		{
			ptLineStr = stRebarPt.ptstr;
			ptLineEnd = stRebarPt.ptend;
		}

		for (size_t j = 0; j < vecReverseStartEnd_A.size(); ++j)
		{
			if (vecReverseStartEnd_A[j].vecDir == 0) // 弧形钢筋
			{
				ptArcStr = vecReverseStartEnd_A[j].ptstr;
				ptArcMid = vecReverseStartEnd_A[j].ptmid;
				ptArcEnd = vecReverseStartEnd_A[j].ptend;
			}
			else
			{
				ptLineStr = vecReverseStartEnd_A[j].ptstr;
				ptLineEnd = vecReverseStartEnd_A[j].ptend;
			}

			double dLenZ = ptArcStr.z;
			ptLineStr.z = dLenZ;
			ptLineEnd = m_arcCenter;
			ptLineEnd.z = dLenZ;

			CVector3D vec = ptLineStr - ptLineEnd;
			movePoint(vec, ptLineStr, (dimReserve_H + dimReserve_V) * 2);

			ArcHandler::CreateArcElement(eehArc, nullptr, DEllipse3d::FromPointsOnArc(ptArcStr, ptArcMid, ptArcEnd), true, *ACTIVEMODEL);

			LineHandler::CreateLineElement(eehLine, nullptr, DSegment3d::From(ptLineStr, ptLineEnd), true, *ACTIVEMODEL);

			DPoint3d ptIntersec;
			DPoint3d ptIntersecTmp;
			if (0 != mdlIntersect_allBetweenElms(&ptIntersec, &ptIntersecTmp, 1, eehLine.GetElementDescrP(), eehArc.GetElementDescrP(), NULL, 1e-6))
			{
				TieRebarPt tieRebarPt;
				tieRebarPt.isInHole = ISPointInHoles(m_vecHoles, ptIntersec);

				double distance_H = dimReserve_V / 2 + diameter / 2;
				double distance_V = (dimReserve_H / 2 + diameter / 2);
				double distance_X = dimReserve_H / 2 + diameter;

				double dis2 = 0.00;
				mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, eehArc.GetElementDescrP(), &ptIntersec, 0.1);

				DEllipse3d seg = DEllipse3d::FromPointsOnArc(ptArcStr, ptArcMid, ptArcEnd);

				ICurvePrimitivePtr curve = ICurvePrimitive::CreateArc(seg);

				CurveLocationDetail arcDetail;
				curve->PointAtSignedDistanceFromFraction(0, dis2 - distance_V, false, arcDetail);

				ptIntersec = arcDetail.point;

				//根据拉筋尺寸将交点偏移
				CVector3D vecNormal = ptLineStr - ptLineEnd;
				vecNormal.Normalize();
				vecNormal.Negate();
				vecNormal.Scale(distance_X);
				ptIntersec.Add(vecNormal);

				//vec_H.Normalize();
				//vec_H.Negate();
				//vec_H.Scale(distance_V);
				//ptIntersec.Add(vec_H);
				ptIntersec.z += distance_H;  // 垂直方向偏移

				tieRebarPt.pt = ptIntersec;
				// end
				ptIntersecH.push_back(tieRebarPt);
			}
		}
		rebarInsec.vecInsecPtReverse.push_back(ptIntersecH);
	}

	for (size_t i = 0; i < vecPositiveStartEnd_M.size(); ++i)
	{
		RebarPoint	stRebarPt = vecPositiveStartEnd_M[i];
		const DPoint3d& ptStart_H = stRebarPt.ptstr;
		const DPoint3d& ptEnd_H = stRebarPt.ptstr;
		CVector3D vec_H = ptEnd_H - ptStart_H;
		vector<TieRebarPt> ptIntersecH;

		DPoint3d ptArcStr, ptArcEnd, ptArcMid;
		DPoint3d ptLineStr, ptLineEnd;

		if (stRebarPt.vecDir == 0) // 弧形钢筋
		{
			ptArcStr = stRebarPt.ptstr;
			ptArcMid = stRebarPt.ptmid;
			ptArcEnd = stRebarPt.ptend;
		}
		else
		{
			ptLineStr = stRebarPt.ptstr;
			ptLineEnd = stRebarPt.ptend;
		}

		int nTTmp = nTmp;
		for (size_t j = 0; j < vecPositiveStartEnd_A.size(); ++j)
		{
			if (vecPositiveStartEnd_A.size() > 2 && j >= vecPositiveStartEnd_A.size() / 2 && nTTmp > 0) // 中间留
			{
				nTTmp--;
				continue;
			}

			if (vecPositiveStartEnd_A[i].vecDir == 0) // 弧形钢筋
			{
				ptArcStr = vecPositiveStartEnd_A[j].ptstr;
				ptArcMid = vecPositiveStartEnd_A[j].ptmid;
				ptArcEnd = vecPositiveStartEnd_A[j].ptend;
			}
			else
			{
				ptLineStr = vecPositiveStartEnd_A[j].ptstr;
				ptLineEnd = vecPositiveStartEnd_A[j].ptend;
			}

			double dLenZ = ptArcStr.z;
			ptLineStr.z = dLenZ;
			ptLineEnd = m_arcCenter;
			ptLineEnd.z = dLenZ;

			CVector3D vec = ptLineStr - ptLineEnd;
			movePoint(vec, ptLineStr, (dimPositive_H + dimPositive_V) * 2);

			ArcHandler::CreateArcElement(eehArc, nullptr, DEllipse3d::FromPointsOnArc(ptArcStr, ptArcMid, ptArcEnd), true, *ACTIVEMODEL);

			LineHandler::CreateLineElement(eehLine, nullptr, DSegment3d::From(ptLineStr, ptLineEnd), true, *ACTIVEMODEL);

			DPoint3d ptIntersec;
			DPoint3d ptIntersecTmp;
			if (0 != mdlIntersect_allBetweenElms(&ptIntersec, &ptIntersecTmp, 1, eehLine.GetElementDescrP(), eehArc.GetElementDescrP(), NULL, 1e-6))
			{
				TieRebarPt tieRebarPt;
				tieRebarPt.isInHole = ISPointInHoles(m_vecHoles, ptIntersec);

				double distance_H = dimPositive_V / 2 + diameter / 2;
				double distance_V = (dimReserve_H / 2 + diameter / 2);
				double distance_X = dimPositive_H / 2 + diameter;

				double dis2 = 0.00;
				mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, eehArc.GetElementDescrP(), &ptIntersec, 0.1);

				DEllipse3d seg = DEllipse3d::FromPointsOnArc(ptArcStr, ptArcMid, ptArcEnd);

				ICurvePrimitivePtr curve = ICurvePrimitive::CreateArc(seg);

				CurveLocationDetail arcDetail;
				curve->PointAtSignedDistanceFromFraction(0, dis2 - distance_V, false, arcDetail);

				ptIntersec = arcDetail.point;

				CVector3D vecNormal = ptLineStr - ptLineEnd;
				vecNormal.Normalize();
				//根据拉筋尺寸将交点偏移
				vecNormal.Scale(distance_X);
				ptIntersec.Add(vecNormal);

				//vec_H.Normalize();
				//vec_H.Negate();
				//vec_H.Scale(distance_V);
				//ptIntersec.Add(vec_H);

				ptIntersec.z += distance_H; // 垂直方向偏移

				tieRebarPt.pt = ptIntersec;
				ptIntersecH.push_back(tieRebarPt);
			}
		}
		rebarInsec.vecInsecPtPositive.push_back(ptIntersecH);
	}
	
}

void TieRebarMaker::CalRebarIntersectionPoint(RebarInsectionPt & rebarInsec, DgnModelRefP modelRef)
{
	if (m_vecStartEnd.size() != 4)
	{
		MessageBox(MSWIND, L"提示", L"未设置钢筋网的起点终点！", MB_OK);
		return;
	}
	if (!modelRef)
	{
		return;
	}

	const vector <DSegment3d> &vecPositiveStartEnd_M = m_vecStartEnd[0]; // 正面第一层
	const vector <DSegment3d> &vecPositiveStartEnd_A = m_vecStartEnd[1]; // 正面第二层
	const vector <DSegment3d> &vecReverseStartEnd_M = m_vecStartEnd[3];	 // 背面最后一层
	const vector <DSegment3d> &vecReverseStartEnd_A = m_vecStartEnd[2];  // 背面倒数第二层


	m_MainVec.FromZero();
	m_AnthorVec.FromZero();
	CalaMainAnthorVec(vecPositiveStartEnd_M, vecPositiveStartEnd_A);

	double dimPositive_H = RebarCode::GetBarDiameter(m_faceRebarDataArray.posRebarData.HRebarData.rebarSize, modelRef);
	double dimPositive_V = RebarCode::GetBarDiameter(m_faceRebarDataArray.posRebarData.VRebarData.rebarSize, modelRef);
	double dimReserve_H = RebarCode::GetBarDiameter(m_faceRebarDataArray.revRebarData.HRebarData.rebarSize, modelRef);
	double dimReserve_V = RebarCode::GetBarDiameter(m_faceRebarDataArray.revRebarData.VRebarData.rebarSize, modelRef);

	double diameter = RebarCode::GetBarDiameter(m_tieRebarSize, modelRef);

// 	double fractionA, fractionB;
// 	DPoint3d ptA, ptB;
	EditElementHandle eehH, eehV;
	DSegment3d lastMainSeg = vecPositiveStartEnd_M[0];; //上一根主方向线
	for (size_t i = 0; i < vecPositiveStartEnd_M.size(); ++i)
	{
		vector<TieRebarPt> ptIntersecH;
		const DSegment3d& seg_H = vecPositiveStartEnd_M[i];
		const DPoint3d& ptStart_H = seg_H.point[0];
		const DPoint3d& ptEnd_H = seg_H.point[1];
		CVector3D vec_H = ptEnd_H - ptStart_H;

		//判断当前线与上一根线是否是截断的关系，是截断关系的存一起
		DPoint3d lastMainStrPt = lastMainSeg.point[0];
		DPoint3d curMainStrProPt = { 0,0,0 };
		mdlVec_projectPointToPlane(&curMainStrProPt, &ptStart_H, &lastMainStrPt, &m_MainVec);
		if (curMainStrProPt.AlmostEqual(lastMainStrPt))
		{
			if (rebarInsec.vecInsecPtPositive.size() > 0)
			{
				ptIntersecH = *rebarInsec.vecInsecPtPositive.rbegin();
				auto eraseIt = rebarInsec.vecInsecPtPositive.end();
				eraseIt--;
				rebarInsec.vecInsecPtPositive.erase(eraseIt);
			}		
		}
		lastMainSeg = vecPositiveStartEnd_M[i];
//		LineHandler::CreateLineElement(eehH, NULL, DSegment3d::From(vecPositiveStartEnd_H[i].first, vecPositiveStartEnd_H[i].second), modelRef->Is3d(), *modelRef);
		for (size_t j = 0; j < vecPositiveStartEnd_A.size(); ++j)
		{
//			LineHandler::CreateLineElement(eehV, NULL, DSegment3d::From(vecPositiveStartEnd_V[i].first, vecPositiveStartEnd_V[i].second), modelRef->Is3d(), *modelRef);

// 			DPoint3d ptIntersec;
// 			if (mdlIntersect_allBetweenElm(&ptIntersec, NULL, 1, eehH.GetElementDescrP(), eehV.GetElementDescrP(), NULL, 1e-6))
// 			{
// 				ptIntersecH.push_back(ptIntersec);
// 			}
 			const DSegment3d& seg_V = vecPositiveStartEnd_A[j];
			const DPoint3d& ptStart_V = seg_V.point[0];
			const DPoint3d& ptEnd_V = seg_V.point[1];
			CVector3D vecNormal;
			m_DownVec.Normalize();
			vec_H.Normalize();
			if (m_modelType == 0)
			{
				if (vec_H.IsEqual(m_MainVec, 0.1))
				{
					vecNormal = m_AnthorVec.CrossProduct(vec_H);
				}
				else
				{
					vecNormal = m_MainVec.CrossProduct(vec_H);
				}
			}
			else
			{
				vecNormal = CVector3D::kZaxis;
			}
			
			vecNormal.Normalize();
			vecNormal.Negate();
			DPoint3d ptProStart_V, ptProEnd_V;
			mdlVec_projectPointToPlane(&ptProStart_V, &ptStart_V, &ptStart_H, &vecNormal);
			mdlVec_projectPointToPlane(&ptProEnd_V, &ptEnd_V, &ptEnd_H, &vecNormal);

//			vecNormal.Normalize();
// 			vecNormal.Scale(dimPositive_H / 2 + dimPositive_V / 2);
// 			ptStart_V.Add(vecNormal);
// 			ptEnd_V.Add(vecNormal);
			DSegment3d segPro_V = DSegment3d::From(ptProStart_V, ptProEnd_V);
			DPoint3d ptIntersec;
			if (SUCCESS == mdlVec_intersect(&ptIntersec, &seg_H, &segPro_V))
			{
				bool isStr = false;
				if (!EFT::IsPointInLine(ptIntersec, ptProStart_V, ptProEnd_V, modelRef, isStr))
				{
					continue;
				}
				if (!EFT::IsPointInLine(ptIntersec, ptStart_H, ptEnd_H, modelRef, isStr))
				{
					continue;
				}

				double distance_H = dimPositive_V / 2 + diameter / 2;
				double distance_V = dimPositive_H / 2 + diameter / 2;
				double distance_X = dimPositive_H / 2 + diameter;

				TieRebarPt tieRebarPt;
				tieRebarPt.isInHole = ISPointInHoles(m_vecHoles, ptIntersec);
				//根据拉筋尺寸将交点偏移
				if (m_modelType == 1)
				{
					vecNormal = m_AnthorVec;
					vecNormal.Normalize();
					vecNormal.Scale(distance_H);
					ptIntersec.Add(vecNormal);

					vec_H.Normalize();
					vec_H.Scale(distance_V);
					ptIntersec.Add(vec_H);

					ptIntersec.z -= distance_X;
				}
				else
				{
					vecNormal.Scale(distance_X);
					ptIntersec.Add(vecNormal);

					vec_H.Normalize();
					vec_H.Scale(distance_V);
					ptIntersec.Add(vec_H);

					ptIntersec.z += distance_H;
				}

				tieRebarPt.pt = ptIntersec;
				ptIntersecH.push_back(tieRebarPt);
			}
		}
		rebarInsec.vecInsecPtPositive.push_back(ptIntersecH);
	}

	lastMainSeg = vecReverseStartEnd_M[0];; //上一根主方向线
	for (size_t i = 0; i < vecReverseStartEnd_M.size(); ++i)
	{
		vector<TieRebarPt> ptIntersecH;
		const DSegment3d& seg_H = vecReverseStartEnd_M[i];
		const DPoint3d& ptStart_H = seg_H.point[0];
		const DPoint3d& ptEnd_H = seg_H.point[1];
		CVector3D vec_H = ptEnd_H - ptStart_H;

		//判断当前线与上一根线是否是截断的关系，是截断关系的存一起
		DPoint3d lastMainStrPt = lastMainSeg.point[0];
		DPoint3d curMainStrProPt = { 0,0,0 };
		mdlVec_projectPointToPlane(&curMainStrProPt, &ptStart_H, &lastMainStrPt, &m_MainVec);
		if (curMainStrProPt.AlmostEqual(lastMainStrPt))
		{
			if (rebarInsec.vecInsecPtReverse.size() > 0)
			{
				ptIntersecH = *rebarInsec.vecInsecPtReverse.rbegin();
				auto eraseIt = rebarInsec.vecInsecPtReverse.end();
				eraseIt--;
				rebarInsec.vecInsecPtReverse.erase(eraseIt);
			}
		}
		lastMainSeg = vecReverseStartEnd_M[i];

		for (size_t j = 0; j < vecReverseStartEnd_A.size(); ++j)
		{
			const DSegment3d& seg_V = vecReverseStartEnd_A[j];
			const DPoint3d& ptStart_V = seg_V.point[0];
			const DPoint3d& ptEnd_V = seg_V.point[1];
			CVector3D vecNormal;
			m_DownVec.Normalize();
			vec_H.Normalize();
			vecNormal = CVector3D::kZaxis;
			if (m_modelType == 0)
			{
				if (vec_H.IsEqual(m_MainVec, 0.1))
				{
					vecNormal = m_AnthorVec.CrossProduct(vec_H);
				}
				else
				{
					vecNormal = m_MainVec.CrossProduct(vec_H);
				}
			}		
			vecNormal.Normalize();
//			vecNormal.Negate();
			CPoint3D ptProStart_V, ptProEnd_V;
			mdlVec_projectPointToPlane(&ptProStart_V, &ptStart_V, &ptStart_H, &vecNormal);
			mdlVec_projectPointToPlane(&ptProEnd_V, &ptEnd_V, &ptEnd_H, &vecNormal);
			DSegment3d segPro_V = DSegment3d::From(ptProStart_V, ptProEnd_V);

			DPoint3d ptIntersec;
			if (SUCCESS == mdlVec_intersect(&ptIntersec, &seg_H, &segPro_V))
			{
				bool isStr = false;
				if (!EFT::IsPointInLine(ptIntersec, DPoint3d(ptProStart_V), DPoint3d(ptProEnd_V), modelRef, isStr))
				{
					continue;
				}
				if (!EFT::IsPointInLine(ptIntersec, ptStart_H, ptEnd_H, modelRef, isStr))
				{
					continue;
				}

				double distance_H = dimPositive_V / 2 + diameter / 2;
				double distance_V = dimPositive_H / 2 + diameter / 2;
				double distance_X = dimPositive_H / 2 + diameter;

				TieRebarPt tieRebarPt;
				tieRebarPt.isInHole = ISPointInHoles(m_vecHoles, ptIntersec);

				//根据拉筋尺寸将交点偏移
				if (m_modelType == 1)
				{
					vecNormal = m_AnthorVec;
					vecNormal.Normalize();
					vecNormal.Scale(distance_H);
					ptIntersec.Add(vecNormal);

					vec_H.Normalize();
					vec_H.Scale(distance_V);
					ptIntersec.Add(vec_H);

					ptIntersec.z += distance_X;
				}
				else
				{
					vecNormal.Scale(distance_X);
					ptIntersec.Add(vecNormal);

					vec_H.Normalize();
					vec_H.Scale(distance_V);
					ptIntersec.Add(vec_H);

					ptIntersec.z += distance_H;
				}

				tieRebarPt.pt = ptIntersec;
				ptIntersecH.push_back(tieRebarPt);
			}
		}
		rebarInsec.vecInsecPtReverse.push_back(ptIntersecH);
	}
}

void TieRebarMaker::CalRebarIntersectionPoint(RebarInsectionPt &rebarInsec, RebarInsectionPt &rebarInsec2, int tieRebarMethod, DgnModelRefP modelRef /*= ACTIVEMODEL*/)
{
	if (m_vecStartEnd.size() != 4)
	{
		MessageBox(MSWIND, L"提示", L"未设置钢筋网的起点终点！", MB_OK);
		return;
	}
	if (!modelRef)
	{
		return;
	}
	double bendRadius = RebarCode::GetPinRadius(m_tieRebarSize, modelRef, false);
	const vector <DSegment3d> &vecPositiveStartEnd_M = m_vecStartEnd[0]; // 正面第一层
	const vector <DSegment3d> &vecPositiveStartEnd_A = m_vecStartEnd[1]; // 正面第二层
	const vector <DSegment3d> &vecReverseStartEnd_M = m_vecStartEnd[3];	 // 背面最后一层
	const vector <DSegment3d> &vecReverseStartEnd_A = m_vecStartEnd[2];  // 背面倒数第二层


	m_MainVec.FromZero();
	m_AnthorVec.FromZero();
	CalaMainAnthorVec(vecPositiveStartEnd_M, vecPositiveStartEnd_A);

	double dimPositive_H = RebarCode::GetBarDiameter(m_faceRebarDataArray.posRebarData.HRebarData.rebarSize, modelRef);
	double dimPositive_V = RebarCode::GetBarDiameter(m_faceRebarDataArray.posRebarData.VRebarData.rebarSize, modelRef);
	double dimReserve_H = RebarCode::GetBarDiameter(m_faceRebarDataArray.revRebarData.HRebarData.rebarSize, modelRef);
	double dimReserve_V = RebarCode::GetBarDiameter(m_faceRebarDataArray.revRebarData.VRebarData.rebarSize, modelRef);

	double diameter = RebarCode::GetBarDiameter(m_tieRebarSize, modelRef);

	// 	double fractionA, fractionB;
	// 	DPoint3d ptA, ptB;
	EditElementHandle eehH, eehV;
	DSegment3d lastMainSeg = vecPositiveStartEnd_M[0];; //上一根主方向线
	for (size_t i = 0; i < vecPositiveStartEnd_M.size(); ++i)
	{
		vector<TieRebarPt> ptIntersecH;
		vector<TieRebarPt> ptIntersecH2;
		const DSegment3d& seg_H = vecPositiveStartEnd_M[i];
		const DPoint3d& ptStart_H = seg_H.point[0];
		const DPoint3d& ptEnd_H = seg_H.point[1];
		CVector3D vec_H = ptEnd_H - ptStart_H;

		//判断当前线与上一根线是否是截断的关系，是截断关系的存一起
		DPoint3d lastMainStrPt = lastMainSeg.point[0];
		DPoint3d curMainStrProPt = { 0,0,0 };
		mdlVec_projectPointToPlane(&curMainStrProPt, &ptStart_H, &lastMainStrPt, &m_MainVec);
		if (curMainStrProPt.AlmostEqual(lastMainStrPt))
		{
			if (rebarInsec.vecInsecPtPositive.size() > 0)
			{
				ptIntersecH = *rebarInsec.vecInsecPtPositive.rbegin();
				auto eraseIt = rebarInsec.vecInsecPtPositive.end();
				eraseIt--;
				rebarInsec.vecInsecPtPositive.erase(eraseIt);
			}
			if (rebarInsec2.vecInsecPtPositive.size() > 0)
			{
				ptIntersecH2 = *rebarInsec2.vecInsecPtPositive.rbegin();
				auto eraseIt = rebarInsec2.vecInsecPtPositive.end();
				eraseIt--;
				rebarInsec2.vecInsecPtPositive.erase(eraseIt);
			}
		}
		lastMainSeg = vecPositiveStartEnd_M[i];
		//		LineHandler::CreateLineElement(eehH, NULL, DSegment3d::From(vecPositiveStartEnd_H[i].first, vecPositiveStartEnd_H[i].second), modelRef->Is3d(), *modelRef);
		for (size_t j = 0; j < vecPositiveStartEnd_A.size(); ++j)
		{
			//			LineHandler::CreateLineElement(eehV, NULL, DSegment3d::From(vecPositiveStartEnd_V[i].first, vecPositiveStartEnd_V[i].second), modelRef->Is3d(), *modelRef);

			// 			DPoint3d ptIntersec;
			// 			if (mdlIntersect_allBetweenElm(&ptIntersec, NULL, 1, eehH.GetElementDescrP(), eehV.GetElementDescrP(), NULL, 1e-6))
			// 			{
			// 				ptIntersecH.push_back(ptIntersec);
			// 			}
			const DSegment3d& seg_V = vecPositiveStartEnd_A[j];
			const DPoint3d& ptStart_V = seg_V.point[0];
			const DPoint3d& ptEnd_V = seg_V.point[1];
			CVector3D vecNormal;
			m_DownVec.Normalize();
			vec_H.Normalize();
			if (m_modelType == 0)
			{
				if (vec_H.IsEqual(m_MainVec, 0.1))
				{
					vecNormal = m_AnthorVec.CrossProduct(vec_H);
				}
				else
				{
					vecNormal = m_MainVec.CrossProduct(vec_H);
				}
			}
			else
			{
				vecNormal = CVector3D::kZaxis;
			}

			vecNormal.Normalize();
			//vecNormal.Negate();
			DPoint3d ptProStart_V, ptProEnd_V;
			mdlVec_projectPointToPlane(&ptProStart_V, &ptStart_V, &ptStart_H, &vecNormal);
			mdlVec_projectPointToPlane(&ptProEnd_V, &ptEnd_V, &ptEnd_H, &vecNormal);

			//			vecNormal.Normalize();
			// 			vecNormal.Scale(dimPositive_H / 2 + dimPositive_V / 2);
			// 			ptStart_V.Add(vecNormal);
			// 			ptEnd_V.Add(vecNormal);
			DSegment3d segPro_V = DSegment3d::From(ptProStart_V, ptProEnd_V);
			DPoint3d ptIntersec;
			DPoint3d ptIntersec2 = DPoint3d::FromZero();
			if (SUCCESS == mdlVec_intersect(&ptIntersec, &seg_H, &segPro_V))
			{
				bool isStr = false;
				if (!EFT::IsPointInLine(ptIntersec, ptProStart_V, ptProEnd_V, modelRef, isStr))
				{
					continue;
				}
				if (!EFT::IsPointInLine(ptIntersec, ptStart_H, ptEnd_H, modelRef, isStr))
				{
					continue;
				}

				double distance_H = dimPositive_V / 2 + diameter / 2;
				double distance_V = dimPositive_H / 2 + diameter / 2;
				double distance_X = dimPositive_H / 2 + diameter;

				TieRebarPt tieRebarPt;
				tieRebarPt.isInHole = ISPointInHoles(m_vecHoles, ptIntersec);
				TieRebarPt tieRebarPt2 = tieRebarPt;
				//根据拉筋尺寸将交点偏移
				ptIntersec2 = ptIntersec;
				if (m_modelType == 1)
				{
					vecNormal = m_AnthorVec;
					vecNormal.Normalize();
					vecNormal.Scale(distance_H);
					ptIntersec.Add(vecNormal);
					ptIntersec2.Add(vecNormal);

					vec_H.Normalize();
					vec_H.Scale(distance_V);
					
					ptIntersec.Add(vec_H);
					ptIntersec2.Add(vec_H);

					ptIntersec.z -= distance_X;
					ptIntersec2.z -= 2*distance_X;
				}
				else
				{
					vecNormal.Scale(distance_X);
					ptIntersec.Add(vecNormal);
					ptIntersec2.Add(vecNormal);

					vec_H.Normalize();
					vec_H.Scale(distance_V);
					
					ptIntersec.Add(vec_H);
					ptIntersec2.Add(vec_H);

					ptIntersec.z += distance_H;
					ptIntersec2.z += 2*distance_H;
				}

				tieRebarPt.pt = ptIntersec;
				ptIntersecH.push_back(tieRebarPt);
				tieRebarPt2.pt = ptIntersec2;
				ptIntersecH2.push_back(tieRebarPt2);
			}
		}
		rebarInsec.vecInsecPtPositive.push_back(ptIntersecH);
		rebarInsec2.vecInsecPtPositive.push_back(ptIntersecH2);
	}

	lastMainSeg = vecReverseStartEnd_M[0];; //上一根主方向线
	for (size_t i = 0; i < vecReverseStartEnd_M.size(); ++i)
	{
		vector<TieRebarPt> ptIntersecH;
		vector<TieRebarPt> ptIntersecH2;
		const DSegment3d& seg_H = vecReverseStartEnd_M[i];
		const DPoint3d& ptStart_H = seg_H.point[0];
		const DPoint3d& ptEnd_H = seg_H.point[1];
		CVector3D vec_H = ptEnd_H - ptStart_H;
		//判断当前线与上一根线是否是截断的关系，是截断关系的存一起
		DPoint3d lastMainStrPt = lastMainSeg.point[0];
		DPoint3d curMainStrProPt = { 0,0,0 };
		mdlVec_projectPointToPlane(&curMainStrProPt, &ptStart_H, &lastMainStrPt, &m_MainVec);
		if (curMainStrProPt.AlmostEqual(lastMainStrPt))
		{
			if (rebarInsec.vecInsecPtReverse.size() > 0)
			{
				ptIntersecH = *rebarInsec.vecInsecPtReverse.rbegin();
				auto eraseIt = rebarInsec.vecInsecPtReverse.end();
				eraseIt--;
				rebarInsec.vecInsecPtReverse.erase(eraseIt);
			}

			if (rebarInsec2.vecInsecPtReverse.size() > 0)
			{
				ptIntersecH = *rebarInsec2.vecInsecPtReverse.rbegin();
				auto eraseIt = rebarInsec2.vecInsecPtReverse.end();
				eraseIt--;
				rebarInsec2.vecInsecPtReverse.erase(eraseIt);
			}
		}
		lastMainSeg = vecReverseStartEnd_M[i];

		for (size_t j = 0; j < vecReverseStartEnd_A.size(); ++j)
		{
			const DSegment3d& seg_V = vecReverseStartEnd_A[j];
			const DPoint3d& ptStart_V = seg_V.point[0];
			const DPoint3d& ptEnd_V = seg_V.point[1];
			CVector3D vecNormal;
			m_DownVec.Normalize();
			vec_H.Normalize();
			vecNormal = CVector3D::kZaxis;
			if (m_modelType == 0)
			{
				if (vec_H.IsEqual(m_MainVec, 0.1))
				{
					vecNormal = m_AnthorVec.CrossProduct(vec_H);
				}
				else
				{
					vecNormal = m_MainVec.CrossProduct(vec_H);
				}
			}
			vecNormal.Normalize();
			vecNormal.Negate();
			CPoint3D ptProStart_V, ptProEnd_V;
			mdlVec_projectPointToPlane(&ptProStart_V, &ptStart_V, &ptStart_H, &vecNormal);
			mdlVec_projectPointToPlane(&ptProEnd_V, &ptEnd_V, &ptEnd_H, &vecNormal);
			DSegment3d segPro_V = DSegment3d::From(ptProStart_V, ptProEnd_V);

			DPoint3d ptIntersec;
			DPoint3d ptIntersec2 = DPoint3d::FromZero();
			if (SUCCESS == mdlVec_intersect(&ptIntersec, &seg_H, &segPro_V))
			{
				bool isStr = false;
				if (!EFT::IsPointInLine(ptIntersec, DPoint3d(ptProStart_V), DPoint3d(ptProEnd_V), modelRef, isStr))
				{
					continue;
				}
				if (!EFT::IsPointInLine(ptIntersec, ptStart_H, ptEnd_H, modelRef, isStr))
				{
					continue;
				}

				double distance_H = dimPositive_V / 2 + diameter / 2;
				double distance_V = dimPositive_H / 2 + diameter / 2;
				double distance_X = dimPositive_H / 2 + diameter;

				TieRebarPt tieRebarPt;
				tieRebarPt.isInHole = ISPointInHoles(m_vecHoles, ptIntersec);
				TieRebarPt tieRebarPt2 = tieRebarPt;

				ptIntersec2 = ptIntersec;
				//根据拉筋尺寸将交点偏移
				if (m_modelType == 1)
				{
					vecNormal = m_AnthorVec;
					vecNormal.Normalize();
					vecNormal.Scale(distance_H);
					ptIntersec.Add(vecNormal);
					ptIntersec2.Add(vecNormal);

					vec_H.Normalize();
					vec_H.Scale(distance_V);
					
					ptIntersec.Add(vec_H);
					ptIntersec2.Add(vec_H);

					ptIntersec.z += distance_X;
					ptIntersec2.z += 2*distance_X;
				}
				else
				{
					vecNormal.Scale(distance_X);
					ptIntersec.Add(vecNormal);
					ptIntersec2.Add(vecNormal);

					vec_H.Normalize();
					vec_H.Scale(distance_V);
					
					ptIntersec.Add(vec_H);
					ptIntersec2.Add(vec_H);

					ptIntersec.z += distance_H;
					ptIntersec2.z += 2*distance_H;
				}

				tieRebarPt.pt = ptIntersec;
				ptIntersecH.push_back(tieRebarPt);
				tieRebarPt2.pt = ptIntersec2;
				ptIntersecH2.push_back(tieRebarPt2);
			}
		}
		rebarInsec.vecInsecPtReverse.push_back(ptIntersecH);
		rebarInsec2.vecInsecPtReverse.push_back(ptIntersecH2);
	}
}

RebarCurve TieRebarMaker::MakeOneTieRebar(DPoint3d ptStart, DPoint3d ptEnd, RebarEndTypes const& endTypes, const CVector3D& endNormal,DgnModelRefP modelRef)
{
	PIT::PITRebarCurve rebar;

	double bendRadius = RebarCode::GetPinRadius(m_tieRebarSize, modelRef, false);	//乘以了30
	double dia = RebarCode::GetBarDiameter(m_tieRebarSize, modelRef);
	double bendLen = dia * 10;
	double bendLne1 = dia * 12;

	//拉筋长度减少2倍拉筋直径
	DPoint3d vecStrEnd = ptEnd - ptStart;
	vecStrEnd.Normalize();
	vecStrEnd.Scale(dia/2);
	ptStart.Add(vecStrEnd);
	vecStrEnd.Scale(-1);
	ptEnd.Add(vecStrEnd);


	PIT::PITRebarEndType strEndType, endEndType;
	strEndType.SetbendLen(bendLen);
	strEndType.SetbendRadius(bendRadius);
	strEndType.SetendNormal(endNormal);
	strEndType.SetptOrgin(ptStart);
	strEndType.SetType(PIT::PITRebarEndType::kCog);

	endEndType.SetbendLen(bendLne1);
	endEndType.SetbendRadius(bendRadius);
	endEndType.SetendNormal(endNormal);
	endEndType.SetptOrgin(ptEnd);
	endEndType.SetType(PIT::PITRebarEndType::kTie);
	PIT::PITRebarEndTypes endtypes = { strEndType, endEndType };

	RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptStart);
	vex->SetType(RebarVertex::kStart);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptEnd);
	vex->SetType(RebarVertex::kEnd);

	rebar.EvaluateEndTypes(endtypes);

	return rebar;
	/************************************************************************/
	/* 两边弯钩角度（135）和弯曲长度一样的                                                                     */
	/************************************************************************/
	//RebarCurve rebar;
	//RebarVertexP vex;

	//vex = &rebar.PopVertices().NewElement();
	//vex->SetIP(ptStart);
	//vex->SetType(RebarVertex::kStart);

	//vex = &rebar.PopVertices().NewElement();
	//vex->SetIP(ptEnd);
	//vex->SetType(RebarVertex::kEnd);

	//double bendRadius = RebarCode::GetPinRadius(m_tieRebarSize, modelRef, false);	//乘以了30
	////double bendLen = RebarCode::GetBendLength(m_tieRebarSize, endTypes.beg, modelRef);	//乘以了100
	//double dia = RebarCode::GetBarDiameter(m_tieRebarSize, modelRef);
	//double bendLen = dia * 10;

	//rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen, &endNormal);

	//return rebar;

}

RebarCurve TieRebarMaker::MakeOneTieRebar(DPoint3d ptStart, DPoint3d ptEnd, double angle1, double angle2, const CVector3D& endNormal, int tieRebarMetohd, bool bdouble, DgnModelRefP modelRef /*= ACTIVEMODEL*/)
{
	PIT::PITRebarCurve rebar;

	double bendRadius =RebarCode::GetPinRadius(m_tieRebarSize, modelRef, false);	//乘以了30
	double dia = RebarCode::GetBarDiameter(m_tieRebarSize, modelRef);
	double bendLen = dia * 10;
	double bendLne1 = dia * 12;

	//拉筋长度减少2倍拉筋直径
	DPoint3d vecStrEnd = ptEnd - ptStart;
	
	if (tieRebarMetohd == 0)
	{
		vecStrEnd.Normalize();
		vecStrEnd.Scale(dia / 2);
		ptStart.Add(vecStrEnd);
		vecStrEnd.Scale(-1);
		ptEnd.Add(vecStrEnd);
	}
	else
	{
		DVec3d vecTem = DVec3d::From(vecStrEnd.x, vecStrEnd.y, vecStrEnd.z);
		vecStrEnd.Normalize();
		vecStrEnd.Scale(dia / 2);
		vecTem.Scale(0.33);//两边各缩小三分之一
		if (!bdouble)
		{
			ptStart.Add(vecStrEnd);
			vecTem.Scale(-1);
			ptEnd.Add(vecTem);
		}
		else
		{
			//vecTem.Scale(-1);
			ptStart.Add(vecTem);
			vecStrEnd.Scale(-1);
			ptEnd.Add(vecStrEnd);
		}
	}
		
	


	PIT::PITRebarEndType strEndType, endEndType;
	strEndType.SetbendLen(bendLen);
	strEndType.SetbendRadius(bendRadius);
	strEndType.SetendNormal(endNormal);
	strEndType.SetptOrgin(ptStart);
	if (tieRebarMetohd == 0)
	{
		strEndType.SetType(PIT::PITRebarEndType::kTie);
	}
	else 
	{
		if (!bdouble)
		{
			strEndType.SetType(PIT::PITRebarEndType::kTie);
		}
		else
		{
			strEndType.SetType(PIT::PITRebarEndType::kNone);
		}
	}
	

	endEndType.SetbendLen(bendLne1);
	endEndType.SetbendRadius(bendRadius);
	endEndType.SetendNormal(endNormal);
	endEndType.SetptOrgin(ptEnd);
	if (tieRebarMetohd == 0)
	{
		endEndType.SetType(PIT::PITRebarEndType::kTie);
	}
	else 
	{
		if (!bdouble)
		{
			endEndType.SetType(PIT::PITRebarEndType::kNone);
		}
		else
		{
			endEndType.SetType(PIT::PITRebarEndType::kTie);
		}
	}

	PIT::PITRebarEndTypes endtypes = { strEndType, endEndType };

	RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptStart);
	vex->SetType(RebarVertex::kStart);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptEnd);
	vex->SetType(RebarVertex::kEnd);

	rebar.EvaluateEndTypes(endtypes,angle1, angle2);

	return rebar;
}

bool TieRebarMaker::MakeTieRebar(std::vector<RebarCurve>& vecRebarCurve,const RebarInsectionPt& rebarInsec, RebarEndTypes const& endTypes, DgnModelRefP modelRef)
{
	if (!modelRef)
		return false;

	if (rebarInsec.vecInsecPtPositive.empty() || rebarInsec.vecInsecPtReverse.empty())
	{
		MessageBox(MSWIND, L"正面或反面钢筋网无交点！", L"提示", MB_OK);
		return false;
	}

	int nMinSize = (int)rebarInsec.vecInsecPtPositive.size();

	if (rebarInsec.vecInsecPtPositive.size() != rebarInsec.vecInsecPtReverse.size())
	{
		int nPrositive = (int)rebarInsec.vecInsecPtPositive.size();
		int nReverse = (int)rebarInsec.vecInsecPtReverse.size();
		nMinSize = nPrositive > nReverse ? nReverse : nPrositive;
	}
	vecRebarCurve.clear();

	// 间隔方式
	int iInterval_H = 0, iInterval_V = 0;
	int iLastRebar = 0;
	switch (m_style)
	{
	case XX:
		iInterval_H = 1;
		iInterval_V = 1;
		iLastRebar = 1;
		break;
	case XX2:
		iInterval_H = 1;
		if ((int)rebarInsec.vecInsecPtReverse.size() > iInterval_H)
		{
			iInterval_V = rebarInsec.vecInsecPtReverse.at(iInterval_H).size() == 1 ? 1 : 2;
		}
		else
		{
			iInterval_V = 1;
		}
		iLastRebar = 1;
		break;
	case X2X:
		iInterval_V = 1;
		if ((int)rebarInsec.vecInsecPtReverse.size() > iInterval_H)
		{
			iInterval_H = rebarInsec.vecInsecPtReverse.at(iInterval_H).size() == 1 ? 1 : 2;
		}
		else
		{
			iInterval_H = 1;
		}
		iLastRebar = 2;
		break;
	case X2X2:
		if ((int)rebarInsec.vecInsecPtReverse.size() > iInterval_H)
		{
			iInterval_H = rebarInsec.vecInsecPtReverse.at(iInterval_H).size() == 1 ? 1 : 2;
		}
		else
		{
			iInterval_H = 1;
		}
		if ((int)rebarInsec.vecInsecPtReverse.size() > iInterval_H)
		{
			iInterval_V = rebarInsec.vecInsecPtReverse.at(iInterval_H).size() == 1 ? 1 : 2;
		}
		else
		{
			iInterval_V = 1;
		}
		iLastRebar = 2;
		break;
	case Custom:
	{
		if (0 >= m_rowInterval)
			m_rowInterval = 1;
		if (0 >= m_colInterval)
			m_colInterval = 1;
		iInterval_H = m_rowInterval;
		iInterval_V = m_colInterval;
		if (iInterval_H % iInterval_V == 0 || iInterval_V % iInterval_H == 0)
		{
			iLastRebar = iInterval_H;
		}
	}
		break;
	default:
		break;
	}
	// end

// 	double dimPositive_H = RebarCode::GetBarDiameter(m_faceRebarDataArray.posRebarData.HRebarData.rebarSize, modelRef);
// 	double dimPositive_V = RebarCode::GetBarDiameter(m_faceRebarDataArray.posRebarData.VRebarData.rebarSize, modelRef);
// 	double dimReserve_H = RebarCode::GetBarDiameter(m_faceRebarDataArray.revRebarData.VRebarData.rebarSize, modelRef);
// 	double dimReserve_V = RebarCode::GetBarDiameter(m_faceRebarDataArray.revRebarData.VRebarData.rebarSize, modelRef);
// 
// 	double diameter = RebarCode::GetBarDiameter(m_tieRebarSize, modelRef);
	int iFirstRebar = 0;
	m_vecRebarStartEnd.clear();
	for (size_t i = 0; i < nMinSize; i+=iInterval_H)
	{
		const vector<TieRebarPt>& vecPtPositive_V = rebarInsec.vecInsecPtPositive[i];
		const vector<TieRebarPt>& vecPtReverse_V = rebarInsec.vecInsecPtReverse[i];
		
		//通过比对投影后的点，过滤找不到对应点的点，适配正反点位不一致情况
		CVector3D normalVec = m_MainVec.CrossProduct(m_AnthorVec);
		vector<TieRebarPt> positivePt;
		for (auto it : vecPtPositive_V)
		{
			DPoint3d tmpPt = {0,0,0};
			mdlVec_projectPointToPlane(&tmpPt, &it.pt, &vecPtReverse_V.at(0).pt, normalVec);

			for (auto cmpIt : vecPtReverse_V)
			{
				if (tmpPt.AlmostEqual(cmpIt.pt))
				{
					positivePt.push_back(it);
					break;
				}
			}
		}
		vector<TieRebarPt> reversePt;
		for (auto it : vecPtReverse_V)
		{
			DPoint3d tmpPt = { 0,0,0 };
			mdlVec_projectPointToPlane(&tmpPt, &it.pt, &vecPtPositive_V.at(0).pt, normalVec);

			for (auto cmpIt : vecPtPositive_V)
			{
				if (tmpPt.AlmostEqual(cmpIt.pt))
				{
					reversePt.push_back(it);
					break;
				}
			}
		}

		int nMin = (int)vecPtPositive_V.size() > (int)vecPtReverse_V.size() ? (int)vecPtReverse_V.size() : (int)vecPtPositive_V.size();
		for (size_t j = iFirstRebar; j < positivePt.size(); j+=iInterval_V)
		{
			TieRebarPt origPtStart = positivePt[j];
			TieRebarPt origPtEnd = reversePt[j];

// 			EditElementHandle eh;
// 			LineHandler::CreateLineElement(eh, NULL, DSegment3d::From(origPtStart, origPtEnd), true, *ACTIVEMODEL);
// 			eh.AddToModel();
			if (origPtStart.isInHole || origPtEnd.isInHole)
				continue;

			if(IsLineIntersectWithAllHoleSoild(m_vecHoles,origPtStart.pt,origPtEnd.pt,m_sideCover,&m_trans))
				continue;

			double dRotateAngle = 125/*225*/;
			CVector3D vec = origPtEnd.pt - origPtStart.pt;
			CVector3D endNormal;	//端部弯钩方向
			vec.Normalize();
			endNormal = vec.Perpendicular();
			if (m_modelType == 1)
			{
				endNormal = vec.CrossProduct(m_MainVec);
			}

			//m_RotateVec.Normalize();
			//double dot = mdlVec_dotProduct(endNormal, m_RotateVec);
			//double dAngle = acos(dot);
			//if (COMPARE_VALUES_EPS(dAngle, (90.0 * PI / 180.0), EPS) < 0)
			//{
			//	dRotateAngle = 360.0 - dRotateAngle;
			//}
			endNormal.Rotate(dRotateAngle * PI / 180, vec);	//以钢筋方向为轴旋转

			RebarCurve rebarCurve = MakeOneTieRebar(origPtStart.pt, origPtEnd.pt, endTypes, endNormal, modelRef);
			vecRebarCurve.push_back(rebarCurve);

			vector<DSegment3d> vecStartEnd;
			vecStartEnd.push_back(DSegment3d::From(origPtStart.pt, origPtEnd.pt));
			m_vecRebarStartEnd.push_back(vecStartEnd);//存储所有直线点，用于预览
		}
		iFirstRebar++;
		if (iFirstRebar >= iInterval_V)
		{
			iFirstRebar = 0;
		}
	}

	return true;
}

bool TieRebarMaker::MakeTieRebar(std::vector<RebarCurve>& vecRebarCurve, const RebarInsectionPt& rebarInsec, double angle1,double angle2, DgnModelRefP modelRef,int TieRebarMethod, bool bdoubleTie)
{
	if (!modelRef)
		return false;

	if (rebarInsec.vecInsecPtPositive.empty() || rebarInsec.vecInsecPtReverse.empty())
	{
		MessageBox(MSWIND, L"正面或反面钢筋网无交点！", L"提示", MB_OK);
		return false;
	}

	int nMinSize = (int)rebarInsec.vecInsecPtPositive.size();

	if (rebarInsec.vecInsecPtPositive.size() != rebarInsec.vecInsecPtReverse.size())
	{
		int nPrositive = (int)rebarInsec.vecInsecPtPositive.size();
		int nReverse = (int)rebarInsec.vecInsecPtReverse.size();
		nMinSize = nPrositive > nReverse ? nReverse : nPrositive;
	}
	vecRebarCurve.clear();

	// 间隔方式
	int iInterval_H = 1, iInterval_V = 1;
	int iLastRebar = 1;

	int iFirstRebar = 0;
	m_vecRebarStartEnd.clear();
	for (size_t i = 0; i < nMinSize; i += iInterval_H)
	{
		const vector<TieRebarPt>& vecPtPositive_V = rebarInsec.vecInsecPtPositive[i];
		const vector<TieRebarPt>& vecPtReverse_V = rebarInsec.vecInsecPtReverse[i];

		//通过比对投影后的点，过滤找不到对应点的点，适配正反点位不一致情况
		CVector3D normalVec = m_MainVec.CrossProduct(m_AnthorVec);
		vector<TieRebarPt> positivePt;
		for (auto it : vecPtPositive_V)
		{
			DPoint3d tmpPt = { 0,0,0 };
			mdlVec_projectPointToPlane(&tmpPt, &it.pt, &vecPtReverse_V.at(0).pt, normalVec);

			for (auto cmpIt : vecPtReverse_V)
			{
				//由于斜面，不会相等，所以先屏蔽
				//if (tmpPt.AlmostEqual(cmpIt.pt))
				//{
					positivePt.push_back(it);
					break;
				//}
			}
		}
		vector<TieRebarPt> reversePt;
		for (auto it : vecPtReverse_V)
		{
			DPoint3d tmpPt = { 0,0,0 };
			mdlVec_projectPointToPlane(&tmpPt, &it.pt, &vecPtPositive_V.at(0).pt, normalVec);

			for (auto cmpIt : vecPtPositive_V)
			{
				//由于斜面，不会相等，所以先屏蔽
				/*if (dDistance < 10 * UOR_PER_MilliMeter)
				{*/
					reversePt.push_back(it);
					break;
				//}
			}
		}

		int nMin = (int)vecPtPositive_V.size() > (int)vecPtReverse_V.size() ? (int)vecPtReverse_V.size() : (int)vecPtPositive_V.size();
		for (size_t j = iFirstRebar; j < positivePt.size(); j += iInterval_V)
		{
			TieRebarPt origPtStart = positivePt[j];
			TieRebarPt origPtEnd = reversePt[j];

			// 			EditElementHandle eh;
			// 			LineHandler::CreateLineElement(eh, NULL, DSegment3d::From(origPtStart, origPtEnd), true, *ACTIVEMODEL);
			// 			eh.AddToModel();
			if (origPtStart.isInHole || origPtEnd.isInHole)
				continue;

			if (IsLineIntersectWithAllHoleSoild(m_vecHoles, origPtStart.pt, origPtEnd.pt, m_sideCover, &m_trans))
				continue;

			double dRotateAngle = 45/*225*/;

			CVector3D vec = origPtEnd.pt - origPtStart.pt;
			CVector3D endNormal;	//端部弯钩方向
			vec.Normalize();
			endNormal = vec.Perpendicular();
			if (m_modelType == 1)
			{
				endNormal = vec.CrossProduct(m_MainVec);
			}

			endNormal.Rotate(dRotateAngle * PI / 180, vec);	//以钢筋方向为轴旋转

			RebarCurve rebarCurve = MakeOneTieRebar(origPtStart.pt, origPtEnd.pt, angle1,angle2, endNormal,TieRebarMethod,bdoubleTie, modelRef);
			vecRebarCurve.push_back(rebarCurve);

			vector<DSegment3d> vecStartEnd;
			vecStartEnd.push_back(DSegment3d::From(origPtStart.pt, origPtEnd.pt));
			m_vecRebarStartEnd.push_back(vecStartEnd);//存储所有直线点，用于预览
		}
		iFirstRebar++;
		if (iFirstRebar >= iInterval_V)
		{
			iFirstRebar = 0;
		}
	}

	return true;
}

bool TieRebarMaker::SetCustomStyle(int rowInterval, int colInterval)
{
	m_rowInterval = rowInterval;
	m_colInterval = colInterval;
	return false;
}

void TieRebarMaker::SetTrans(Transform trans)
{
	m_trans = trans;
}

void TieRebarMaker::SetHoles(vector<EditElementHandle*> vecHoles)
{
	m_vecHoles = vecHoles;
}

void TieRebarMaker::SetHoleCover(double cover)
{
	m_sideCover = cover;
}

RebarSetTag * TieRebarMaker::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	RebarInsectionPt rebarInsec;
	if (m_arcVecStartEnd.size() > 0)
	{
		// 弧形钢筋求交点
		CalRebarIntersectionPointArc(rebarInsec, modelRef);
	}
	else
	{
		// 直线钢筋做交点
		CalRebarIntersectionPoint(rebarInsec, modelRef);
	}
	if (rebarInsec.vecInsecPtPositive.empty() || rebarInsec.vecInsecPtReverse.empty())
	{
		MessageBox(MSWIND, L"提示",L"正面或反面无钢筋交点！",MB_OK);
		return NULL;
	}

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(m_CallerId);
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kCog);
	endTypeEnd.SetType(RebarEndType::kCustom);
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };

	std::vector<RebarCurve> vecRebarCurve;
	MakeTieRebar(vecRebarCurve, rebarInsec, endTypes, modelRef);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(m_tieRebarSize, modelRef);

	RebarSymbology symb;
	string str(m_tieRebarSize);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(LevelName/*TEXT_TIE_REBAR*/);

	for (size_t i = 0; i < vecRebarCurve.size(); ++i)
	{
		RebarElementP rebarElement = NULL;
		if ((!PreviewButtonDown) && (!SlabPreviewButtonDown))
		{//若不是预览状态下则生成钢筋
			rebarElement = rebarSet->AssignRebarElement((int)i, (int)vecRebarCurve.size(), symb, modelRef);
		}
		RebarCurve rebarCurve = vecRebarCurve[i];
		if (NULL != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)m_tieRebarSize);
			shape.SetIsStirrup(false);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string Stype = "TieRebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
	}

	RebarSetData setdata;
	setdata.SetNumber((int)vecRebarCurve.size());
	setdata.SetNominalSpacing(m_faceRebarDataArray.posRebarData.HRebarData.rebarSpacing / uor_per_mm);
	setdata.SetAverageSpacing(m_faceRebarDataArray.posRebarData.HRebarData.rebarSpacing / uor_per_mm);

	rebarSet->FinishUpdate(setdata, modelRef);

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);

	return tag;
}

RebarSetTag* TieRebarMaker::MakeRebarforFace(ElementId rebarSetId, ElementId faceId, double angle1, double angle2,int tiRebarMethod ,DgnModelRefP modelRef /*= ACTIVEMODEL*/)
{
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	RebarInsectionPt rebarInsec;
	RebarInsectionPt rebarInsec2;
	if (m_arcVecStartEnd.size() > 0)
	{
		// 弧形钢筋求交点
		CalRebarIntersectionPointArc(rebarInsec, modelRef);
	}
	else
	{
		// 直线钢筋做交点
		//CalRebarIntersectionPoint(rebarInsec, modelRef);
		CalRebarIntersectionPoint(rebarInsec, rebarInsec2, tiRebarMethod, modelRef);
	}
	if (rebarInsec.vecInsecPtPositive.empty() || rebarInsec.vecInsecPtReverse.empty())
	{
		MessageBox(MSWIND, L"提示", L"正面或反面无钢筋交点！", MB_OK);
		return NULL;
	}

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(m_CallerId);
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kCog);
	endTypeEnd.SetType(RebarEndType::kCustom);
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };

	std::vector<RebarCurve> vecRebarCurve;
	MakeTieRebar(vecRebarCurve, rebarInsec, angle1,angle2, modelRef,tiRebarMethod,false);
	std::vector<RebarCurve> vecRebarCurve2;
	MakeTieRebar(vecRebarCurve2, rebarInsec2, angle1, angle2, modelRef, tiRebarMethod, true);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(m_tieRebarSize, modelRef);

	RebarSymbology symb;
	string str(m_tieRebarSize);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_TIE_REBAR);

	for (size_t i = 0; i < vecRebarCurve.size(); ++i)
	{
		RebarElementP rebarElement = NULL;
		if ((!PreviewButtonDown) && (!SlabPreviewButtonDown))
		{//若不是预览状态下则生成钢筋
			rebarElement = rebarSet->AssignRebarElement((int)i, (int)vecRebarCurve.size(), symb, modelRef);
		}
		RebarCurve rebarCurve = vecRebarCurve[i];
		if (NULL != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)m_tieRebarSize);
			shape.SetIsStirrup(false);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string Stype = "TieRebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

			
			//设置面ID
			SetElementXAttribute(eleid, sizeof(ElementId), &faceId, TieRebarFaceId, ACTIVEMODEL);
			EditElementHandle eehface(faceId,ACTIVEMODEL);
			
			double dArea = 0.0;
			mdlMeasure_elmDscrArea(&dArea,NULL, eehface.GetElementDescrP());
			dArea /= 100000000;//得到平方米
			//计算钢筋数量
			double rebarnum = 0;
			if (tiRebarMethod == 0)
			{
				if (m_TieFaceStyle == TWOFOUR)
				{
					 rebarnum = dArea * 12.5;
				}
				else 
				{
					rebarnum = dArea * 6.25;
				}
				int num = ceil(rebarnum);
				SetElementXAttribute(eleid, sizeof(int), &num, TieFaceRebarNum, ACTIVEMODEL);//写入干净数量
			}
			else 
			{
				if (m_TieFaceStyle == TWOFOUR)
				{
					 rebarnum = dArea * 25;
				}
				else
				{
					rebarnum = dArea * 12.5;
				}
				rebarnum = ceil(rebarnum);
				rebarnum = rebarnum / 2;//对扣的时候，两根拉筋的数量分别写入
				int num = ceil(rebarnum);
				SetElementXAttribute(eleid, sizeof(int), &num, TieFaceRebarNum, ACTIVEMODEL);//写入干净数量
			}		
			
			
			mdlElmdscr_setVisible(eehface.GetElementDescrP(),false);
			eehface.ReplaceInModel(eehface.GetElementRef());
		
		}
	
		//由于只需要一根钢筋，所以多余的不需要
		break;		
	}

	if (tiRebarMethod == 1)
	{
		for (size_t i = 0; i < vecRebarCurve2.size(); ++i)
		{
			RebarElementP rebarElement = NULL;
			if ((!PreviewButtonDown) && (!SlabPreviewButtonDown))
			{//若不是预览状态下则生成钢筋
				//+1 按面配筋中对扣有两根钢筋
				rebarElement = rebarSet->AssignRebarElement((int)i + 1, (int)vecRebarCurve2.size(), symb, modelRef);

			}
			RebarCurve rebarCurve = vecRebarCurve2[i];
			if (NULL != rebarElement)
			{
				RebarShapeData shape;
				shape.SetSizeKey((LPCTSTR)m_tieRebarSize);
				shape.SetIsStirrup(false);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

				ElementId eleid = rebarElement->GetElementId();
				EditElementHandle tmprebar(eleid, ACTIVEMODEL);
				string Stype = "TieRebar";
				ElementRefP oldref = tmprebar.GetElementRef();
				SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
				tmprebar.ReplaceInModel(oldref);


				//设置面ID
				SetElementXAttribute(eleid, sizeof(ElementId), &faceId, TieRebarFaceId, ACTIVEMODEL);
				EditElementHandle eehface(faceId, ACTIVEMODEL);

				double dArea = 0.0;
				mdlMeasure_elmDscrArea(&dArea, NULL, eehface.GetElementDescrP());
				dArea /= 100000000;//得到平方米
				//计算钢筋数量
				double rebarnum = 0;
				if (tiRebarMethod == 0)
				{
					if (m_TieFaceStyle == TWOFOUR)
					{
						rebarnum = dArea * 12.5;
					}
					else
					{
						rebarnum = dArea * 6.25;
					}
					int num = ceil(rebarnum);
					SetElementXAttribute(eleid, sizeof(int), &num, TieFaceRebarNum, ACTIVEMODEL);//写入干净数量
				}
				else
				{
					if (m_TieFaceStyle == TWOFOUR)
					{
						rebarnum = dArea * 25;
					}
					else
					{
						rebarnum = dArea * 12.5;
					}
					rebarnum = ceil(rebarnum);
					rebarnum = rebarnum / 2;//对扣的时候，两根拉筋的数量分别写入
					int num = ceil(rebarnum);;
					SetElementXAttribute(eleid, sizeof(int), &num, TieFaceRebarNum, ACTIVEMODEL);//写入干净数量
				}
				

				mdlElmdscr_setVisible(eehface.GetElementDescrP(), false);
				eehface.ReplaceInModel(eehface.GetElementRef());
			}
			break;
		}

	}
	


	RebarSetData setdata;
	setdata.SetNumber((int)vecRebarCurve.size());//先留着，后续再看需不需要改
	setdata.SetNominalSpacing(m_faceRebarDataArray.posRebarData.HRebarData.rebarSpacing / uor_per_mm);
	setdata.SetAverageSpacing(m_faceRebarDataArray.posRebarData.HRebarData.rebarSpacing / uor_per_mm);

	rebarSet->FinishUpdate(setdata, modelRef);

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);
	

	return tag;
}
