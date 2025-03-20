#include "_ustation.h"
#include "resource.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "ArcWallRebarAssembly.h"
#include "WallRebarDlg.h"
#include "ExtractFacesTool.h"
#include "CWallRebarDlg.h"
#include "TieRebar.h"
#include "ElementAttribute.h"
#include "PITMSCECommon.h"
#include "ExtractFacesTool.h"
#include "XmlHelper.h"
#include "CSolidTool.h"
#include "CFaceTool.h"
#include <unordered_set>
// #include "SelectRebarTool.h"
#include <RebarHelper.h>
// #include "HoleRebarAssembly.h"
#include <CPointTool.h>

#include "MakeRebarHelper.h"

extern bool PreviewButtonDown;//主要配筋界面的预览按钮
extern map<int, vector<RebarPoint>> g_wallRebarPtsNoHole;

using namespace PIT;

bool ArcWallRebarAssembly::AnalyzingWallGeometricDataARC(ElementHandleCR eh, PIT::ArcSegment &arcFront, PIT::ArcSegment &arcBack)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle testeeh(eh, false);

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &m_ArcWallData.height);

	double frontRadius, backRadius;
	frontRadius = backRadius = 0;
	int j = 0;
	for (int i = 0; i < vecDownFaceLine.size(); i++)
	{
		if (vecDownFaceLine[i] != nullptr)
		{
			if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
			{
				double starR, sweepR;
				double radius;
				DPoint3d ArcDPs[2];
				RotMatrix rotM;
				DPoint3d centerpt;
				mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &vecDownFaceLine[i]->el);
				if (j == 0)
				{
					frontRadius = backRadius = radius;
					j++;
				}
				else
				{
					if (frontRadius > radius)
					{
						frontRadius = radius;
					}
					if (backRadius < radius)
					{
						backRadius = radius;
					}
				}
			}
		}
	}
	double starangleF, endangleF;
	double starangleB, endangleB;
	DPoint3d centerpt;
	CalcuteStrEndAngleAndRadius(starangleF, endangleF, frontRadius, centerpt, vecDownFaceLine);
	CalcuteStrEndAngleAndRadius(starangleB, endangleB, backRadius, centerpt, vecDownFaceLine);

	bool scalef = false;
	if (starangleF > endangleF)
	{
		double tmpangle;
		tmpangle = endangleF;
		endangleF = starangleF;
		starangleF = tmpangle;
		scalef = true;
	}
	if (starangleB > endangleB)
	{
		double tmpangle;
		tmpangle = endangleB;
		endangleB = starangleB;
		starangleB = tmpangle;
	}
	//PIT::ArcSegment arcFront, arcBack;
	{//计算外弧改变之前的起点和终点
		double sweepAngle = endangleB - starangleB;
		double sweepR;
		if (scalef)
		{
			sweepAngle = 360 - sweepAngle;
			sweepR = -(sweepAngle) / 180 * fc_pi;
		}
		else
		{
			sweepR = (sweepAngle) / 180 * fc_pi;
		}
		//开始角度延扫描角度反方向移动，扫描角度加大；
		double starR = (360 - starangleB) / 180 * fc_pi;


		MSElement  newarcOut;
		mdlArc_create(&newarcOut, NULL, &centerpt, backRadius, backRadius, NULL, starR, -sweepR);
		DPoint3d ArcDPs[2];
		RotMatrix rotM;
		double radius;
		mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcOut);
		arcBack.FptStart = ArcDPs[0];
		arcBack.FptEnd = ArcDPs[1];
	}



	double sweepAngle = endangleF - starangleF;
	if (sweepAngle < 180)
	{
		if (starangleF < starangleB)//外弧取扫掠角度最大时的弧，和内弧比较时
		{
			starangleB = starangleF;
		}
		if (endangleF > endangleB)
		{
			endangleB = endangleF;
		}
	}
	else
	{
		if (starangleF > starangleB)//外弧取扫掠角度最大时的弧，和内弧比较时
		{
			starangleB = starangleF;
		}
		if (endangleF < endangleB)
		{
			endangleB = endangleF;
		}
	}
	double sweepR;
	if (scalef)
	{
		sweepAngle = 360 - sweepAngle;
		sweepR = -(sweepAngle) / 180 * fc_pi;
	}
	else
	{
		sweepR = (sweepAngle) / 180 * fc_pi;
	}
	//开始角度延扫描角度反方向移动，扫描角度加大；
	double starR = (360 - starangleF) / 180 * fc_pi;


	MSElement newarcIn, newarcOut;
	mdlArc_create(&newarcIn, NULL, &centerpt, frontRadius, frontRadius, NULL, starR, -sweepR);



	sweepAngle = endangleB - starangleB;
	if (scalef)
	{
		sweepAngle = 360 - sweepAngle;
		sweepR = -(sweepAngle) / 180 * fc_pi;
	}
	else
	{
		sweepR = (sweepAngle) / 180 * fc_pi;
	}
	//开始角度延扫描角度反方向移动，扫描角度加大；
	starR = (360 - starangleB) / 180 * fc_pi;

	mdlArc_create(&newarcOut, NULL, &centerpt, backRadius, backRadius, NULL, starR, -sweepR);


	MSElementDescrP msedIn, exmseOut;
	mdlElmdscr_new(&msedIn, NULL, &newarcIn);
	mdlElmdscr_new(&exmseOut, NULL, &newarcOut);

	//mdlElmdscr_add(msedIn);
	//mdlElmdscr_add(exmseOut);


	DEllipse3d ellipsePro;
	DPoint3d ArcDPs[2];
	RotMatrix rotM;
	double radius;
	mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcIn);
	mdlArc_extractDEllipse3d(&ellipsePro, &newarcIn);
	arcFront.ptStart = ArcDPs[0];
	arcFront.ptEnd = ArcDPs[1];
	arcFront.dRadius = frontRadius;
	arcFront.ptCenter = centerpt;
	arcFront.dLen = ellipsePro.ArcLength();
	mdlElmdscr_pointAtDistance(&arcFront.ptMid, NULL, arcFront.dLen / 2, msedIn, 1e-6);

	//外弧计算
	DEllipse3d ellipseRev;
	DPoint3d ArcoDPs[2];
	mdlArc_extract(ArcoDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcOut);
	mdlArc_extractDEllipse3d(&ellipseRev, &newarcOut);
	arcBack.ptStart = ArcoDPs[0];
	arcBack.ptEnd = ArcoDPs[1];
	arcBack.dRadius = backRadius;
	arcBack.ptCenter = centerpt;
	arcBack.dLen = ellipseRev.ArcLength();
	mdlElmdscr_pointAtDistance(&arcBack.ptMid, NULL, arcBack.dLen / 2, exmseOut, 1e-6);

	m_ArcWallData.thickness = fabs(arcBack.dRadius - arcFront.dRadius);
	m_width = m_ArcWallData.thickness;
	m_ArcWallData.OuterArc = arcBack;
	m_ArcWallData.InnerArc = arcFront;

	DPoint3d tmppt = m_ArcWallData.InnerArc.ptStart;
	m_ArcWallData.InnerArc.ptStart = m_ArcWallData.InnerArc.ptEnd;
	m_ArcWallData.InnerArc.ptEnd = tmppt;

	m_Holeehs = Holeehs;
	mdlElmdscr_freeAll(&exmseOut);
	mdlElmdscr_freeAll(&msedIn);
	return true;
}


void ArcWallRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
{
	m_useHoleehs.clear();
	//m_useHoleehs.insert(m_useHoleehs.begin(), m_Holeehs.begin(), m_Holeehs.end());


	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetSideCover()*uor_per_mm;



	if (g_wallRebarInfo.concrete.isHandleHole)//计算需要处理的孔洞
	{
		for (int j = 0; j < m_Holeehs.size(); j++)
		{
			EditElementHandle eeh;
			eeh.Duplicate(*m_Holeehs.at(j));

			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
			}
			DPoint3d ptcenter = m_ArcWallData.InnerArc.ptCenter;
			DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());

			ptcenter.z = ptele.z;
			CVector3D yVec = ptcenter - ptele;
			yVec.Normalize();

			CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

			DPoint3d ptStart = ptcenter;
			BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴

			Transform trans;
			placement.AssignTo(trans);
			Transform intrans = trans;
			intrans.InverseOf(intrans);

			TransformInfo transinfo(trans);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			DPoint3d minP;
			DPoint3d maxP;
			//计算指定元素描述符中元素的范围。
			mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
			DRange3d range;
			range.low = minP;
			range.high = maxP;
			bool isNeed = false;
			if (range.XLength() > misssize || range.ZLength() > misssize)
			{
				isNeed = true;
			}

			/*if (isNeed)
			{
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*m_Holeehs.at(j));
				PlusSideCover(*m_Holeehs.at(j), dSideCover, trans);
				m_useHoleehs.push_back(m_Holeehs.at(j));
			}*/
			if (isNeed)
			{
				if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//如果是门洞
				{
					continue;
				}
				bool isdoorNeg = false;//判断是否为门洞NEG
				isdoorNeg = IsDoorHoleNeg(m_Holeehs.at(j), m_doorsholes);
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*m_Holeehs.at(j));
				if (isdoorNeg)
				{
					PlusSideCover(*m_Holeehs.at(j), dSideCover, trans, isdoorNeg, m_ArcWallData.thickness);
				}
				else
				{
					PlusSideCover(*m_Holeehs.at(j), dSideCover, trans);
				}

				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}



}
long ArcWallRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool ArcWallRebarAssembly::CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
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

bool ArcWallRebarAssembly::makeLineRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::ArcSegment arcSeg, double dLen, double space, double startOffset, double endOffset, PIT::PITRebarEndTypes & endTypes)
{
	DEllipse3d seg = DEllipse3d::FromPointsOnArc(arcSeg.ptStart, arcSeg.ptMid, arcSeg.ptEnd);

	ICurvePrimitivePtr curve = ICurvePrimitive::CreateArc(seg);

	CurveLocationDetail arcDetail;
	curve->PointAtSignedDistanceFromFraction(0, space, false, arcDetail);
	DPoint3d ptStart = arcDetail.point;
	DPoint3d vec = { 0,0,1 };
	vec.Scale(startOffset);
	ptStart.Add(vec);

	DPoint3d ptEnd = arcDetail.point;
	ptEnd.z += dLen;
	vec.Normalize();
	vec.Negate();
	vec.Scale(endOffset);
	ptEnd.Add(vec);

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetSideCover()*uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	m_vecRebarPtsLayer.push_back(ptStart);
	m_vecRebarPtsLayer.push_back(ptEnd);
	if (m_isPushTieRebar)
	{
		m_vecTieRebarPtsLayer.push_back(ptStart);
		m_vecTieRebarPtsLayer.push_back(ptEnd);
	}
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, ptStart, ptEnd, dSideCover, matrix);

	/*EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(ptStart, ptEnd), true, *ACTIVEMODEL);
	eehline.AddToModel();*/

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt : tmppts)
	{
		if (ExtractFacesTool::IsPointInLine(pt, ptStart, ptEnd, ACTIVEMODEL, isStr))
		{
			int dis = (int)ptStart.Distance(pt);
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = ptStart;
	}
	else
	{
		map_pts[0] = ptStart;
	}
	int dis = (int)ptStart.Distance(ptEnd);
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = ptEnd;
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = ptEnd;
	}



	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{
		PITRebarCurve tmprebar;
		RebarVertexP vex;
		vex = &tmprebar.PopVertices().NewElement();
		vex->SetIP(itr->second);
		vex->SetType(RebarVertex::kStart);
		endTypes.beg.SetptOrgin(itr->second);

		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}

		endTypes.end.SetptOrgin(itrplus->second);

		vex = &tmprebar.PopVertices().NewElement();
		vex->SetIP(itrplus->second);
		vex->SetType(RebarVertex::kEnd);

		tmprebar.EvaluateEndTypes(endTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebar.push_back(tmprebar);
	}

	/*RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptStart);
	vex->SetType(RebarVertex::kStart);
	endTypes.beg.SetptOrgin(ptStart);

	DPoint3d ptEnd = arcDetail.point;
	ptEnd.z += dLen;
	vec.Normalize();
	vec.Negate();
	vec.Scale(endOffset);
	ptEnd.Add(vec);
	endTypes.end.SetptOrgin(ptEnd);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptEnd);
	vex->SetType(RebarVertex::kEnd);

	rebar.EvaluateEndTypes(endTypes);*/

	return false;
}

bool ArcWallRebarAssembly::makeArcWallRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::ArcSegment arcSeg, double space, double startOffset, double endOffset, PIT::PITRebarEndTypes& endTypes)
{

	DPoint3d ptArcStart = arcSeg.ptStart;
	ptArcStart.z += space;
	DPoint3d ptArcEnd = arcSeg.ptEnd;
	ptArcEnd.z += space;
	DPoint3d ptArcCenter = arcSeg.ptCenter;
	ptArcCenter.z += space;
	DPoint3d ptArcMid = arcSeg.ptMid;
	ptArcMid.z += space;

	m_vecRebarPtsLayer.push_back(ptArcStart);
	m_vecRebarPtsLayer.push_back(ptArcMid);
	m_vecRebarPtsLayer.push_back(ptArcEnd);
	if (m_isPushTieRebar)
	{
		m_vecTieRebarPtsLayer.push_back(ptArcStart);
		m_vecTieRebarPtsLayer.push_back(ptArcMid);
		m_vecTieRebarPtsLayer.push_back(ptArcEnd);
	}

	EditElementHandle arceeh;
	ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(ptArcStart, ptArcMid, ptArcEnd), true, *ACTIVEMODEL);
	//arceeh.AddToModel();

	vector<DPoint3d> pts;
	GetARCIntersectPointsWithHoles(pts, m_Holeehs, ptArcStart, ptArcEnd, ptArcMid);


	if (pts.size() > 0)
	{
		/*for (int i =0;i<pts.size()-1;i++)
		{
		EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(pts[i], pts[i+1]), true, *ACTIVEMODEL);
		eeh.AddToModel();
		}*/

	}
	map<int, DPoint3d> map_pts;
	bool isStr = false;
	double dislenth;
	dislenth = 0;
	mdlElmdscr_distanceAtPoint(&dislenth, nullptr, nullptr, arceeh.GetElementDescrP(), &ptArcEnd, 0.1);
	for (DPoint3d pt : pts)
	{
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &pt, 0.1);
		if (dis1 > 10 && dis1 <= dislenth + 10)
		{
			int dis = (int)dis1;
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = ptArcStart;
	}
	else
	{
		map_pts[0] = ptArcStart;
	}
	int dis = (int)dislenth;
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = ptArcEnd;
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = ptArcEnd;
	}



	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{

		PITRebarCurve trebar;
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &itr->second, 0.1);
		DPoint3d tmpMid, tmpstr, tmpend;

		tmpstr = itr->second;
		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		tmpend = itrplus->second;
		double dis2;
		dis2 = 0;
		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, arceeh.GetElementDescrP(), &tmpend, 0.1);

		dis1 = dis1 + abs(dis2 - dis1) / 2;

		mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis1, arceeh.GetElementDescrP(), 0.1);

		if (ISPointInHoles(m_useHoleehs, tmpMid))
		{
			if (ISPointInHoles(m_useHoleehs, tmpstr) && ISPointInHoles(m_useHoleehs, tmpend))
			{
				itr--;
				continue;
			}
		}
		if (CalculateArc(trebar, tmpstr, tmpMid, tmpend))
		{
			trebar.EvaluateEndTypesArc(endTypes);
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(tmpstr, tmpMid, tmpend), true, *ACTIVEMODEL);
			//arceeh1.AddToModel();
			rebar.push_back(trebar);
		}
	}


	return true;
}


bool ArcWallRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pArcWallDoubleRebarDlg = new CWallRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pArcWallDoubleRebarDlg->SetSelectElement(ehSel);
	pArcWallDoubleRebarDlg->Create(IDD_DIALOG_WallRebar);
	pArcWallDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pArcWallDoubleRebarDlg->ShowWindow(SW_SHOW);



	// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// 	CWallRebarDlg dlg(ehSel, CWnd::FromHandle(MSWIND));
	// 	dlg.SetConcreteId(FetchConcrete());
	// 	if (IDCANCEL == dlg.DoModal())
	// 		return false;

	return true;
}

bool ArcWallRebarAssembly::Rebuild()
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
	return true;
}



bool ArcWallRebarAssembly::AnalyzingWallGeometricData(ElementHandleCR eh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle testeeh(eh, false);

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);

	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &m_ArcWallData.height);

	double frontRadius, backRadius;
	frontRadius = backRadius = 0;
	int j = 0;
	for (int i = 0; i < vecDownFaceLine.size(); i++)
	{
		if (vecDownFaceLine[i] != nullptr)
		{
			if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
			{
				double starR, sweepR;
				double radius;
				DPoint3d ArcDPs[2];
				RotMatrix rotM;
				DPoint3d centerpt;
				mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &vecDownFaceLine[i]->el);
				if (j == 0)
				{
					frontRadius = backRadius = radius;
					j++;
				}
				else
				{
					if (frontRadius > radius)
					{
						frontRadius = radius;
					}
					if (backRadius < radius)
					{
						backRadius = radius;
					}
				}
			}
		}
	}
	double starangleF, endangleF;
	double starangleB, endangleB;
	DPoint3d centerpt;
	CalcuteStrEndAngleAndRadius(starangleF, endangleF, frontRadius, centerpt, vecDownFaceLine);
	CalcuteStrEndAngleAndRadius(starangleB, endangleB, backRadius, centerpt, vecDownFaceLine);

	bool scalef = false;
	if (starangleF > endangleF)
	{
		double tmpangle;
		tmpangle = endangleF;
		endangleF = starangleF;
		starangleF = tmpangle;
		scalef = true;
	}
	if (starangleB > endangleB)
	{
		double tmpangle;
		tmpangle = endangleB;
		endangleB = starangleB;
		starangleB = tmpangle;
	}

	double strAngel = starangleF, endAngel = endangleF;
	if (strAngel > starangleB)
	{
		strAngel = starangleB;
	}
	if (endAngel < endangleB)
	{
		endangleB = endangleB;
	}
	//if (starangleF < starangleB)
	//{
	//	starangleB = starangleF;
	//}
	//else
	//{
	//	starangleF = starangleB;
	//}
	//if (endangleF > endangleB)
	//{
	//	endangleB = endangleF;
	//}
	//else
	//{
	//	endangleF = endangleB;
	//}
	PIT::ArcSegment arcFront, arcBack;
	{//计算外弧改变之前的起点和终点
		double sweepAngle = endangleB - starangleB;
		double sweepR;
		if (scalef)
		{
			sweepAngle = 360 - sweepAngle;
			sweepR = -(sweepAngle) / 180 * fc_pi;
		}
		else
		{
			sweepR = (sweepAngle) / 180 * fc_pi;
		}
		//开始角度延扫描角度反方向移动，扫描角度加大；
		double starR = (360 - starangleB) / 180 * fc_pi;


		MSElement  newarcOut;
		mdlArc_create(&newarcOut, NULL, &centerpt, backRadius, backRadius, NULL, starR, -sweepR);
		DPoint3d ArcDPs[2];
		RotMatrix rotM;
		double radius;
		mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcOut);
		arcBack.FptStart = ArcDPs[0];
		arcBack.FptEnd = ArcDPs[1];
	}



	double sweepAngle = endangleF - starangleF;
	//if (sweepAngle < 180)
	//{
	//	if (starangleF < starangleB)//外弧取扫掠角度最大时的弧，和内弧比较时
	//	{
	//		starangleB = starangleF;
	//	}
	//	if (endangleF > endangleB)
	//	{
	//		endangleB = endangleF;
	//	}
	//}
	//else
	//{
	//	if (starangleF > starangleB)//外弧取扫掠角度最大时的弧，和内弧比较时
	//	{
	//		starangleB = starangleF;
	//	}
	//	if (endangleF < endangleB)
	//	{
	//		endangleB = endangleF;
	//	}
	//}
	double sweepR;
	if (scalef)
	{
		sweepAngle = 360 - sweepAngle;
		sweepR = -(sweepAngle) / 180 * fc_pi;
	}
	else
	{
		sweepR = (sweepAngle) / 180 * fc_pi;
	}
	//开始角度延扫描角度反方向移动，扫描角度加大；
	double starR = (360 - starangleF) / 180 * fc_pi;



	MSElement newarcIn, newarcOut;
	mdlArc_create(&newarcIn, NULL, &centerpt, frontRadius, frontRadius, NULL, starR, -sweepR);
	MSElementDescrP msedIn;
	mdlElmdscr_new(&msedIn, NULL, &newarcIn);
	DEllipse3d ellipsePro;
	DPoint3d ArcDPs[2];
	RotMatrix rotM;
	double radius;
	mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcIn);
	mdlArc_extractDEllipse3d(&ellipsePro, &newarcIn);
	arcFront.ptStart = ArcDPs[0];
	arcFront.ptEnd = ArcDPs[1];
	arcFront.dRadius = frontRadius;
	arcFront.ptCenter = centerpt;
	arcFront.dLen = ellipsePro.ArcLength();
	mdlElmdscr_pointAtDistance(&arcFront.ptMid, NULL, arcFront.dLen / 2, msedIn, 1e-6);


	sweepAngle = endangleB - starangleB;
	if (scalef)
	{
		sweepAngle = 360 - sweepAngle;
		sweepR = -(sweepAngle) / 180 * fc_pi;
	}
	else
	{
		sweepR = (sweepAngle) / 180 * fc_pi;
	}
	//开始角度延扫描角度反方向移动，扫描角度加大；
	starR = (360 - starangleB) / 180 * fc_pi;

	mdlArc_create(&newarcOut, NULL, &centerpt, backRadius, backRadius, NULL, starR, -sweepR);


	MSElementDescrP  exmseOut;
	mdlElmdscr_new(&exmseOut, NULL, &newarcOut);

	//mdlElmdscr_add(msedIn);
	//mdlElmdscr_add(exmseOut);

	//外弧计算
	DEllipse3d ellipseRev;
	DPoint3d ArcoDPs[2];
	mdlArc_extract(ArcoDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcOut);
	mdlArc_extractDEllipse3d(&ellipseRev, &newarcOut);
	arcBack.ptStart = ArcoDPs[0];
	arcBack.ptEnd = ArcoDPs[1];
	arcBack.dRadius = backRadius;
	arcBack.ptCenter = centerpt;
	arcBack.dLen = ellipseRev.ArcLength();
	mdlElmdscr_pointAtDistance(&arcBack.ptMid, NULL, arcBack.dLen / 2, exmseOut, 1e-6);

	m_ArcWallData.thickness = fabs(arcBack.dRadius - arcFront.dRadius);
	m_width = m_ArcWallData.thickness;
	m_ArcWallData.OuterArc = arcBack;
	m_ArcWallData.InnerArc = arcFront;

	DPoint3d tmppt = m_ArcWallData.InnerArc.ptStart;
	m_ArcWallData.InnerArc.ptStart = m_ArcWallData.InnerArc.ptEnd;
	m_ArcWallData.InnerArc.ptEnd = tmppt;

	//最大外弧计算
	m_outMaxArc = arcBack;
	sweepAngle = endangleF - starangleF;
	if (scalef)
	{
		sweepAngle = 360 - sweepAngle;
		sweepR = -(sweepAngle) / 180 * fc_pi;
	}
	else
	{
		sweepR = (sweepAngle) / 180 * fc_pi;
	}
	//开始角度延扫描角度反方向移动，扫描角度加大；
	starR = (360 - starangleF) / 180 * fc_pi;
	MSElement newarcMax;
	mdlArc_create(&newarcMax, NULL, &centerpt, backRadius, backRadius, NULL, starR, -sweepR);
	MSElementDescrP msedMax;
	mdlElmdscr_new(&msedMax, NULL, &newarcMax);
	DEllipse3d ellipseMax;
	DPoint3d maxArcDPs[2];
	RotMatrix maxRotM;
	mdlArc_extract(maxArcDPs, &starR, &sweepR, &radius, NULL, &maxRotM, &centerpt, &newarcMax);
	mdlArc_extractDEllipse3d(&ellipseMax, &newarcMax);
	m_outMaxArc.ptStart = maxArcDPs[0];
	m_outMaxArc.ptEnd = maxArcDPs[1];
	m_outMaxArc.dRadius = backRadius;
	m_outMaxArc.ptCenter = centerpt;
	m_outMaxArc.dLen = ellipseMax.ArcLength();
	mdlElmdscr_pointAtDistance(&m_outMaxArc.ptMid, NULL, m_outMaxArc.dLen / 2, msedMax, 1e-6);

	m_sideCoverAngle = 0;

	m_Holeehs = Holeehs;
	mdlElmdscr_freeAll(&exmseOut);
	mdlElmdscr_freeAll(&msedIn);


	/*if (vecDownFontLine.size() > 1 || vecDownBackLine.size() > 1)
	{
		std::for_each(vecDownFaceLine.begin(), vecDownFaceLine.end(), [](MSElementDescrP &ms) { mdlElmdscr_freeAll(&ms); });
		return false;
	}
	DEllipse3d ellipsePro;
	ArcSegment arcFront, arcBack;
	if (vecDownFontLine[0]->el.ehdr.type == ARC_ELM)
	{
		mdlArc_extractDEllipse3d(&ellipsePro, &vecDownFontLine[0]->el);
		ellipsePro.EvaluateEndPoints(arcFront.ptStart, arcFront.ptEnd);
		arcFront.dRadius = arcFront.ptStart.Distance(ellipsePro.center);
		arcFront.ptCenter = ellipsePro.center;
		arcFront.dLen = ellipsePro.ArcLength();
		mdlElmdscr_pointAtDistance(&arcFront.ptMid, NULL, arcFront.dLen / 2, vecDownFontLine[0], 1e-6);
	}
	DEllipse3d ellipseRev;
	for (size_t i = 0; i < vecDownBackLine.size(); i++)
	{
		if (vecDownBackLine[i]->el.ehdr.type == ARC_ELM)
		{
			mdlArc_extractDEllipse3d(&ellipseRev, &vecDownBackLine[i]->el);

			ellipseRev.EvaluateEndPoints(arcBack.ptStart, arcBack.ptEnd);
			arcBack.dRadius = arcBack.ptStart.Distance(ellipseRev.center);
			arcBack.ptCenter = ellipseRev.center;
			arcBack.dLen = ellipseRev.ArcLength();
			mdlElmdscr_pointAtDistance(&arcBack.ptMid, NULL, arcBack.dLen / 2, vecDownBackLine[i], 1e-6);
		}
	}
	m_ArcWallData.thickness = fabs(arcFront.dRadius - arcBack.dRadius);
	m_ArcWallData.OuterArc = arcFront.dRadius > arcBack.dRadius ? arcFront : arcBack;
	m_ArcWallData.InnerArc = arcFront.dRadius > arcBack.dRadius ? arcBack : arcFront;
	m_Holeehs = Holeehs;*/

	return true;
}

bool ArcWallRebarAssembly::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}
bool ArcWallRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	vector<int> vctemp;
	vctemp = GetvecDataExchange();
	NewRebarAssembly(modelRef);
	CalculateUseHoles(modelRef);
	RebarSetTagArray rsetTags;
	m_vecRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
	g_vecTieRebarPtsNoHole.clear();
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
	double dLevelSpace = 0;
	double dSideCover = GetSideCover()*uor_per_mm;


	double dArcLen = m_ArcWallData.OuterArc.dLen;
	double dWidth = m_ArcWallData.thickness;

	int iRebarSetTag = 0;
	int iRebarLevelNum = GetRebarLevelNum();
	int iTwinbarSetIdIndex = 0;
	vector<PIT::EndType> vecEndType;
	BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
	if (strTieRebarSize.Find(L"mm") != string::npos)
		strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm
	double diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//拉筋直径
	double newRadius = m_ArcWallData.OuterArc.dRadius - GetPositiveCover() * uor_per_mm - diameterTie;
	double diameterPre = 0.0;

	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		m_vecRebarPtsLayer.clear();
		m_vecTieRebarPtsLayer.clear();
		RebarSetTag* tag = NULL;
		vector<PIT::EndType> vecEndType;
		if (GetvvecEndType().empty())		//没有设置端部样式，设置默认值
		{
			EndType endType;
			memset(&endType, 0, sizeof(endType));
			vecEndType = { { 0,0,0 },{0,0,0} ,endType };
		}
		else
		{
			vecEndType = GetvvecEndType().at(i);
		}

		double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef);	//当前层钢筋直径

		PIT::ArcSegment newArc = m_ArcWallData.OuterArc;
		if (0 == i)	//首层偏移当前钢筋直径
		{
			newRadius -= diameter * 0.5;
		}
		else
		{
			diameterPre = RebarCode::GetBarDiameter(GetvecDirSize().at(i - 1), modelRef);	//上一层钢筋直径
			newRadius -= diameterPre * 0.5;	//偏移上一层钢筋的半径
			newRadius -= diameter * 0.5;	//偏移当前层钢筋的半径
		}

		newRadius -= GetvecLevelSpace().at(i) * uor_per_mm;	//偏移层间距
		double offset = m_ArcWallData.OuterArc.dRadius - newRadius;	//总偏移长度
		double reverseCover = GetReverseCover()*uor_per_mm + diameter * 0.5 + diameterTie;	//反面实际需预留宽度
		if (offset > m_ArcWallData.thickness - reverseCover)	//偏移出了墙
		{
			double reverseOffset = 0.0;
			for (int j = iRebarLevelNum - 1; j > i; --j)
			{
				double diameterLater = RebarCode::GetBarDiameter(GetvecDirSize().at(j), modelRef);	//后续层钢筋直径
				reverseOffset += diameter;
			}
			offset = m_ArcWallData.thickness - reverseCover - reverseOffset;
			newRadius = m_ArcWallData.OuterArc.dRadius - offset;
		}
		newArc.ScaleToRadius(newRadius);		//整体缩放
		newArc.CutArc(m_ArcWallData.OuterArc.FptStart, m_ArcWallData.InnerArc.ptEnd);
		newArc.CutArc(m_ArcWallData.OuterArc.FptEnd, m_ArcWallData.InnerArc.ptStart);

		//PopvecSetId().push_back(0);
		vector<CVector3D> vecEndNormal(2);
		if (0 == GetvecDir().at(i))	//水平弧形钢筋
		{
			CVector3D	endNormal;	//端部弯钩方向
			if (GetvvecEndType().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					CVector3D rebarVec = m_ArcWallData.OuterArc.FptEnd - m_ArcWallData.OuterArc.FptStart;
					endNormal = CVector3D::From(0, 0, -1);
					if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
					{
						endNormal.Negate();
					}
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}
			newArc.Shorten(dSideCover, false);		//起点缩短
			newArc.Shorten(dSideCover, true);		//终点缩短
			newArc.OffsetByAxisZ(diameter * 0.5 + GetSideCover()*uor_per_mm + diameterTie);
			//绘制并筋--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
			{
				m_isPushTieRebar = true;
				//先绘制非并筋层
				PopvecSetId().push_back(0);
				tag = MakeRebars_Arc(PopvecSetId().back(), GetvecDirSize().at(i), newArc, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}

				m_isPushTieRebar = false;
				PopvecSetId().push_back(0);
				//绘制并筋层
				tag = MakeRebars_Arc(PopvecSetId().back(), GetvecDirSize().at(i), newArc, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
			else //当前层未设置并筋
			{
				m_isPushTieRebar = true;
				TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
				PopvecSetId().push_back(0);
				tag = MakeRebars_Arc(PopvecSetId().back(), GetvecDirSize().at(i), newArc, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}
			}
			vecEndType.clear();
		}
		else
		{
			CVector3D	endNormal;	//端部弯钩方向
			if (GetvvecEndType().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = m_ArcWallData.OuterArc.FptEnd - m_ArcWallData.OuterArc.FptStart;
					endNormal.Normalize();
					if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
					{
						endNormal.Negate();
					}
					CVector3D rebarVec = CVector3D::kZaxis;
					/*					endNormal = rebarVec.CrossProduct(vec);*/
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}

			newArc.Shorten(dSideCover + diameter * 0.5, false);		//起点缩短
			newArc.Shorten(dSideCover + diameter * 0.5, true);		//终点缩短
			newArc.OffsetByAxisZ(GetSideCover()*uor_per_mm + diameterTie);
			ArcSegment newArcSeg = newArc;
			double newSpacing = GetvecDirSpacing().at(i);
			if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2) //放射配筋
			{
				GetNewArcAndSpacing(newArc, newArcSeg, GetvecAngle().at(i), newSpacing);
				if (m_sideCoverAngle == 0)
				{
					m_sideCoverAngle = (dSideCover + diameter * 0.5) / (newArcSeg.dRadius / 10 * PI / 180);
				}
				double shortenLen = newArcSeg.dRadius / 10 * PI / 180 * m_sideCoverAngle;
				newArcSeg.Shorten(shortenLen, false);		//起点缩短
				newArcSeg.Shorten(shortenLen, true);		//终点缩短
				vector<double> vecSpacing = GetvecDirSpacing();
				vecSpacing[i] = newSpacing;
				SetvecDirSpacing(vecSpacing);
			}
			double dLen = m_ArcWallData.height - (GetSideCover()*uor_per_mm + diameterTie) * 2;
			//绘制并筋--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
			{
				m_isPushTieRebar = false;
				//先绘制非并筋层
				PopvecSetId().push_back(0);
				tag = MakeRebars_Line(PopvecSetId().back(), GetvecDirSize().at(i), newArcSeg, dLen, newSpacing*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}

				m_isPushTieRebar = true;
				//绘制并筋层
				PopvecSetId().push_back(0);
				tag = MakeRebars_Line(PopvecSetId().back(), GetvecDirSize().at(i), newArcSeg, dLen, newSpacing*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
			else //当前层未设置并筋
			{
				m_isPushTieRebar = true;
				TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
				PopvecSetId().push_back(0);
				tag = MakeRebars_Line(PopvecSetId().back(), GetvecDirSize().at(i), newArcSeg, dLen, newSpacing*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}
			}
			vecEndType.clear();
		}
		if (m_vecRebarPtsLayer.size() > 1)
		{
			if (GetvecDir().at(i) == 0)//X轴时，存储了三个点
			{
				for (int m = 0; m < m_vecRebarPtsLayer.size() - 2; m++)
				{
					int n = m + 1;
					int k = n + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecRebarPtsLayer.at(m);
					rbPt.ptmid = m_vecRebarPtsLayer.at(n);
					rbPt.ptend = m_vecRebarPtsLayer.at(k);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecRebarPtsNoHole.push_back(rbPt);
					m = m + 2;
				}
			}
			else
			{
				for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecRebarPtsLayer.at(m);
					rbPt.ptend = m_vecRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
		}
		if (m_vecTieRebarPtsLayer.size() > 1)
		{
			if (GetvecDir().at(i) == 0)//X轴时，存储了三个点
			{
				for (int m = 0; m < m_vecTieRebarPtsLayer.size() - 2; m++)
				{
					int n = m + 1;
					int k = n + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTieRebarPtsLayer.at(m);
					rbPt.ptmid = m_vecTieRebarPtsLayer.at(n);
					rbPt.ptend = m_vecTieRebarPtsLayer.at(k);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTieRebarPtsNoHole.push_back(rbPt);
					m = m + 2;
				}
			}
			else
			{
				for (int m = 0; m < m_vecTieRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTieRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTieRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTieRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
		}
	}
	/*for (RebarPoint pt:g_vecRebarPtsNoHole)
	{
		if (pt.vecDir==0)
		{
			EditElementHandle arceeh;
			ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(pt.ptstr, pt.ptmid, pt.ptend), true, *ACTIVEMODEL);
			arceeh.AddToModel();
		}
		else
		{
			EditElementHandle tmpeeh;
			LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(pt.ptstr, pt.ptend), true, *ACTIVEMODEL);
			tmpeeh.AddToModel();
		}
	}*/



	//添加拉筋--begin
	vector<vector<DSegment3d>> vctTieRebarLines;//存储所有的拉筋直线信息，用于预览
	if (0 != GetTieRebarInfo().tieRebarMethod/* && (m_vecAllRebarStartEnd.size() >= 4)*/)
	{
		FaceRebarDataArray faceDataArray;
		faceDataArray.posRebarData.HRebarData.rebarSize = GetvecDirSize().at(0);
		faceDataArray.posRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(0);
		faceDataArray.posRebarData.VRebarData.rebarSize = GetvecDirSize().at(1);
		faceDataArray.posRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(1);

		faceDataArray.revRebarData.HRebarData.rebarSize = GetvecDirSize().at(GetvecDirSize().size() - 1);
		faceDataArray.revRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecDirSize().size() - 1);
		faceDataArray.revRebarData.VRebarData.rebarSize = GetvecDirSize().at(GetvecDirSize().size() - 2);
		faceDataArray.revRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecDirSize().size() - 2);

		vector<vector<RebarPoint> > vecStartEnd;		//只存储1，2层和倒数第1，2层
		vector<vector<RebarPoint> > vvecSeg;
		int index = 0;
		vector<RebarPoint> vecSeg;
		for (size_t i = 0; i < g_vecTieRebarPtsNoHole.size(); ++i)
		{
			if (index != g_vecTieRebarPtsNoHole[i].iIndex)
			{
				vvecSeg.push_back(vecSeg);
				vecSeg.clear();
			}
			DSegment3d seg = DSegment3d::From(g_vecTieRebarPtsNoHole[i].ptstr, g_vecTieRebarPtsNoHole[i].ptend);
			vecSeg.push_back(g_vecTieRebarPtsNoHole[i]);
			index = g_vecTieRebarPtsNoHole[i].iIndex;
			if (i == g_vecTieRebarPtsNoHole.size() - 1)
			{
				vvecSeg.push_back(vecSeg);
			}
		}

		if (vvecSeg.size() < 4)
		{
			if (g_globalpara.Getrebarstyle() != 0)
			{
				return (SUCCESS == AddRebarSets(rsetTags));
			}
			return true;
		}

		vecStartEnd.push_back(vvecSeg[0]);
		vecStartEnd.push_back(vvecSeg[1]);
		vecStartEnd.push_back(vvecSeg[vvecSeg.size() - 2]);
		vecStartEnd.push_back(vvecSeg[vvecSeg.size() - 1]);
		BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
		int	tieRebarStyle = GetTieRebarInfo().tieRebarStyle;
		if (strTieRebarSize.Find(L"mm") != string::npos)
			strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm

		PopvecSetId().push_back(0);

		vector<vector<DSegment3d> > vecStartEndTmp;		//只存储1，2层和倒数第1，2层
		TieRebarMaker tieRebarMaker(faceDataArray, vecStartEndTmp, (TieRebarStyle)tieRebarStyle, strTieRebarSize);
		tieRebarMaker.m_CallerId = GetCallerId();
		tieRebarMaker.SetArcStartEnd(vecStartEnd);
		tieRebarMaker.SetCustomStyle(GetTieRebarInfo().rowInterval, GetTieRebarInfo().colInterval);
		tieRebarMaker.SetModeType(0);
		Transform trans;
		GetPlacement().AssignTo(trans);
		tieRebarMaker.SetTrans(trans);
		vector<EditElementHandle*> vecAllSolid;
		vecAllSolid.insert(vecAllSolid.begin(), m_Holeehs.begin(), m_Holeehs.end());
		tieRebarMaker.SetDownVec(m_ArcWallData.OuterArc.ptStart, m_ArcWallData.OuterArc.ptEnd);
		tieRebarMaker.SetArcPoint(m_ArcWallData.OuterArc.ptStart, m_ArcWallData.OuterArc.ptEnd, m_ArcWallData.OuterArc.ptCenter);
		tieRebarMaker.SetHoles(vecAllSolid);
		tieRebarMaker.SetHoleCover(GetSideCover()*uor_per_mm);
		RebarSetTag* tag = tieRebarMaker.MakeRebar(PopvecSetId().back(), modelRef);
		tieRebarMaker.GetRebarPts(vctTieRebarLines);//取出所有的拉筋直线信息
		if (NULL != tag && (!PreviewButtonDown))
		{
			tag->SetBarSetTag(iRebarLevelNum + 1);
			rsetTags.Add(tag);
		}
	}

	if (PreviewButtonDown)//预览按钮按下，则画主筋线
	{
		m_allLines.clear();
		for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)
		{

			vector<vector<DPoint3d>> faceLinePts = *it;
			for (auto it : faceLinePts)
			{
				vector<DPoint3d> linePts = it;
				if (linePts.size() == 2)
				{
					DPoint3d strPoint = *linePts.begin();
					DPoint3d endPoint = *linePts.rbegin();
					CVector3D vecIndex = endPoint - strPoint;
					if (COMPARE_VALUES_EPS(vecIndex.z, 0.0, uor_per_mm) == 0)
					{
						DPoint3d arcCenter = m_ArcWallData.OuterArc.ptCenter;
						arcCenter.z = strPoint.z;
						EditElementHandle arceeh;
						ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromArcCenterStartEnd(arcCenter, strPoint, endPoint), true, *ACTIVEMODEL);
						arceeh.AddToModel();
						m_allLines.push_back(arceeh.GetElementRef());
					}
					else
					{
						EditElementHandle eeh;
						LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
						eeh.AddToModel();
						m_allLines.push_back(eeh.GetElementRef());
					}
				}
				else
				{
					EditElementHandle eeh;
					LineStringHandler::CreateLineStringElement(eeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
					eeh.AddToModel();
					m_allLines.push_back(eeh.GetElementRef());
				}

			}
		}

		for (auto itr = vctTieRebarLines.begin(); itr != vctTieRebarLines.end(); itr++)
		{
			vector<DSegment3d> vcttemp(*itr);
			for (int y = 0; y < vcttemp.size(); y++)
			{
				DPoint3d strPoint = DPoint3d::From(vcttemp[y].point[0].x, vcttemp[y].point[0].y, vcttemp[y].point[0].z);
				DPoint3d endPoint = DPoint3d::From(vcttemp[y].point[1].x, vcttemp[y].point[1].y, vcttemp[y].point[1].z);
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allLines.push_back(eeh.GetElementRef());
			}
		}
		return true;
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}


RebarSetTag* ArcWallRebarAssembly::MakeRebars_Line
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	PIT::ArcSegment			arcSeg,
	double              dLen,
	double              spacing,
	double              startOffset,
	double              endOffset,
	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
	TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
	int level,
	int grade,
	int DataExchange,
	bool				bTwinbarLevel,
	DgnModelRefP        modelRef
)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, startbendLenTb, endbendLen, endbendLenTb;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);	//乘以了30
	double adjustedXLen, adjustedSpacing;

	double sideCov = GetSideCover()*uor_per_mm;
	int numRebar = 0;
	if (twinBarInfo.hasTwinbars)	//并筋
		adjustedXLen = arcSeg.dLen - sideCov - diameter - diameterTb - startOffset - endOffset;
	else
		adjustedXLen = arcSeg.dLen - sideCov - diameter - startOffset - endOffset;
	if (bTwinbarLevel)				//并筋层钢筋条数
	{
		numRebar = (int)floor(adjustedXLen / (spacing * (twinBarInfo.interval + 1)) + 0.85) + 1;
		int numRebar1 = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar1 > 1 && g_wallRebarInfo.concrete.m_SlabRebarMethod != 2) //非放射配筋
		{
			adjustedSpacing = adjustedXLen / (numRebar1 - 1);	//非并筋层平均间距
			adjustedSpacing *= (twinBarInfo.interval + 1);		//并筋层实际间距需乘以钢筋间隔
		}
	}
	else
	{
		numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar > 1 && g_wallRebarInfo.concrete.m_SlabRebarMethod != 2)
			adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	double xPos = startOffset;
	if (bTwinbarLevel)				//并筋层需偏移一段距离
	{
		xPos += diameter * 0.5;
		if (diameterTb > diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radiusTb - radius)*(radiusTb - radius), 0.5) - radius;
			xPos += dOffset;
		}
		else if (diameterTb < diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radius - radiusTb)*(radius - radiusTb), 0.5) - radius;
			xPos += dOffset;
		}
		else
		{
			xPos += diameter * 0.5;
		}
	}
	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

	switch (endType[0].endType)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeStart.SetType(RebarEndType::kLap);
		begStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 8:	//用户
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (endType[1].endType)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeEnd.SetType(RebarEndType::kLap);
		endStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	if (bTwinbarLevel)				//并筋层
	{
		start.SetbendLen(startbendLenTb);
		start.SetbendRadius(bendRadiusTb);
	}
	else
	{
		start.SetbendLen(startbendLen);
		start.SetbendRadius(startbendRadius);
	}
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	if (bTwinbarLevel)				//并筋层
	{
		end.SetbendLen(endbendLenTb);
		end.SetbendRadius(bendRadiusTb);
	}
	else
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };
	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinBarInfo.rebarSize, symb);
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}
	else
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}

	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurve;
		//RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
		makeLineRebarCurve(rebarCurve, arcSeg, dLen, xPos, endTypeStartOffset, endTypEendOffset, endTypes);

		xPos += adjustedSpacing;

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurve.begin(), rebarCurve.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	vector<vector<DPoint3d>> vecStartEnd;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (ISPointInHoles(m_Holeehs, midPos))
		{
			if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
			{
				continue;
			}
		}

		vector<DPoint3d> linePts;
		RebarVertices vertices = rebarCurve.GetVertices();
		for (size_t i = 0; i < (size_t)vertices.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &vertices.At(i);
			linePts.push_back(tmpVertex->GetIP());
		}
		/*EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.AddToModel();
*/
		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//若不是预览状态下则生成钢筋
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinBarInfo.rebarSize));
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameterTb, endTypes, shape, modelRef, false);
			}
			else
			{
				shape.SetSizeKey((LPCTSTR)sizeKey);
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
			}
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (DataExchange == 0)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinback";
				else
					Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
		j++;
		vecStartEnd.push_back(linePts);
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

RebarSetTag * ArcWallRebarAssembly::MakeRebars_Arc(ElementId & rebarSetId, BrStringCR sizeKey, PIT::ArcSegment arcSeg, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const & endType, vector<CVector3D> const & vecEndNormal, TwinBarSet::TwinBarLevelInfo const & twinBarInfo, int level, int grade, int DataExchange, bool bTwinbarLevel, DgnModelRefP modelRef)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, startbendLenTb, endbendLen, endbendLenTb;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);
	double adjustedXLen, adjustedSpacing;

	BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
	if (strTieRebarSize.Find(L"mm") != string::npos)
		strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm
	double diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//拉筋直径

	double sideCov = GetSideCover()*uor_per_mm;
	int numRebar = 0;
	if (twinBarInfo.hasTwinbars)	//并筋
		adjustedXLen = m_ArcWallData.height - sideCov * 2 - diameter - diameterTie * 2 - diameterTb - startOffset - endOffset;
	else
		adjustedXLen = m_ArcWallData.height - sideCov * 2 - diameter - diameterTie * 2 - startOffset - endOffset;
	if (bTwinbarLevel)				//并筋层钢筋条数
	{
		numRebar = (int)floor(adjustedXLen / (spacing * (twinBarInfo.interval + 1)) + 0.85) + 1;
		int numRebar1 = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar1 > 1)
		{
			adjustedSpacing = adjustedXLen / (numRebar1 - 1);	//非并筋层平均间距
			adjustedSpacing *= (twinBarInfo.interval + 1);		//并筋层实际间距需乘以钢筋间隔
		}
	}
	else
	{
		numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar > 1)
			adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	double xPos = startOffset;
	if (bTwinbarLevel)				//并筋层需偏移一段距离
	{
		xPos += diameter * 0.5;
		if (diameterTb > diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radiusTb - radius)*(radiusTb - radius), 0.5) - radius;
			xPos += dOffset;
		}
		else if (diameterTb < diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radius - radiusTb)*(radius - radiusTb), 0.5) - radius;
			xPos += dOffset;
		}
		else
		{
			xPos += diameter * 0.5;
		}
	}
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

	switch (endType[0].endType)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeStart.SetType(RebarEndType::kLap);
		begStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 8:	//用户
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (endType[1].endType)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeEnd.SetType(RebarEndType::kLap);
		endStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	if (bTwinbarLevel)				//并筋层
	{
		start.SetbendLen(startbendLenTb);
		start.SetbendRadius(bendRadiusTb);
	}
	else
	{
		start.SetbendLen(startbendLen);
		start.SetbendRadius(startbendRadius);
	}
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	if (bTwinbarLevel)				//并筋层
	{
		end.SetbendLen(endbendLenTb);
		end.SetbendRadius(bendRadiusTb);
	}
	else
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };
	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinBarInfo.rebarSize, symb);
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}
	else
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	vector<PITRebarCurve>     rebarCurvesNum;
	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurve;
		//		RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
		makeArcWallRebarCurve(rebarCurve, arcSeg, xPos, endTypeStartOffset, endTypEendOffset, endTypes);

		xPos += adjustedSpacing;

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurve.begin(), rebarCurve.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	vector<vector<DPoint3d>> vecStartEnd;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (ISPointInHoles(m_Holeehs, midPos))
		{
			if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
			{
				continue;
			}
		}

		vector<DPoint3d> linePts;
		linePts.push_back(ptstr);
		linePts.push_back(ptend);
		//vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));
		/*	EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			eeh.AddToModel();
*/
		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//若不是预览状态下则生成钢筋
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinBarInfo.rebarSize));
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameterTb, endTypes, shape, modelRef, false);
			}
			else
			{
				shape.SetSizeKey((LPCTSTR)sizeKey);
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
			}
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);

			string Stype;
			if (DataExchange == 0)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinback";
				else
					Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
		j++;
		vecStartEnd.push_back(linePts);
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}


void ArcWallRebarAssembly::GetNewArcAndSpacing(PIT::ArcSegment oldArc, PIT::ArcSegment& newArc, double angle, double& newSpacing)
{
	//画出旧的弧线
	EditElementHandle oldArcEeh;
	ArcHandler::CreateArcElement(oldArcEeh, nullptr, DEllipse3d::FromCenterRadiusXY(oldArc.ptCenter, oldArc.dRadius), true, *ACTIVEMODEL);

	//外弧开始点到中心点的直线
	EditElementHandle strLineEeh;
	LineHandler::CreateLineElement(strLineEeh, nullptr, DSegment3d::From(m_outMaxArc.ptStart, m_outMaxArc.ptCenter), true, *ACTIVEMODEL);

	//外弧结束点到中心点的直线
	EditElementHandle endLineEeh;
	LineHandler::CreateLineElement(endLineEeh, nullptr, DSegment3d::From(m_outMaxArc.ptEnd, m_outMaxArc.ptCenter), true, *ACTIVEMODEL);

	//外弧中间点到中心点的直线
	EditElementHandle midLineEeh;
	LineHandler::CreateLineElement(midLineEeh, nullptr, DSegment3d::From(m_outMaxArc.ptMid, m_outMaxArc.ptCenter), true, *ACTIVEMODEL);

	//旧的弧线和两根直线的交点为新弧线的起始点和终点
	mdlIntersect_allBetweenElms(&newArc.ptStart, nullptr, 1, oldArcEeh.GetElementDescrP(), strLineEeh.GetElementDescrP(), nullptr, 1/*, nullptr, nullptr*/);
	mdlIntersect_allBetweenElms(&newArc.ptEnd, nullptr, 1, oldArcEeh.GetElementDescrP(), endLineEeh.GetElementDescrP(), nullptr, 1/*, nullptr, nullptr*/);
	mdlIntersect_allBetweenElms(&newArc.ptMid, nullptr, 1, oldArcEeh.GetElementDescrP(), midLineEeh.GetElementDescrP(), nullptr, 1/*, nullptr, nullptr*/);
	newArc.ptCenter = oldArc.ptCenter;

	//新的弧线
	DEllipse3d newEllipse = DEllipse3d::FromPointsOnArc(newArc.ptStart, newArc.ptMid, newArc.ptEnd);
	EditElementHandle newArcEeh;
	ArcHandler::CreateArcElement(newArcEeh, nullptr, newEllipse, true, *ACTIVEMODEL);
	RotMatrix rotM;
	// double radius;
	mdlArc_extract(nullptr, nullptr, nullptr, &newArc.dRadius, NULL, &rotM, nullptr, newArcEeh.GetElementCP());
	newArc.dLen = newEllipse.ArcLength();

	newSpacing = newArc.dRadius / 10 * PI / 180 * angle;
}
