#include "_ustation.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "CFacesRebarDlg.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "CamberedSurfaceRebarAssembly.h"
#include <CModelTool.h>
#include <CPointTool.h>

#include "PITMSCECommon.h"
#include "XmlHelper.h"
#include "SelectRebarTool.h"
#include "RebarHelper.h"
#include "CFaceTool.h"
#include "CElementTool.h"
#include "WallHelper.h"
#include "SlabHelper.h"
#include "CSolidTool.h"

#define  Waterstops_Width (60*UOR_PER_MilliMeter)

using namespace Gallery;

using namespace PIT;
extern bool FacePreviewButtonsDown;


//弧面配筋---begin
long CamberedSurfaceRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool CamberedSurfaceRebarAssembly::CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
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
			mid = cen + tan1 * radius;
			DEllipse3d circle = DEllipse3d::FromCenterRadiusXY(cen, radius);

			double angle_start = circle.PointToAngle(beg);
			double angle_mid = circle.PointToAngle(mid);

			double angle = (angle_start + angle_mid) / 2;
			CPoint3D mid1;
			circle.Evaluate(&mid1, 0, angle);

			vex = &curve.PopVertices().NewElement();
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

bool CamberedSurfaceRebarAssembly::makeLineRebarCurve(vector<PITRebarCurve>& rebar, ArcSegment arcSeg, double dLen, double space, double startOffset, double endOffset, PITRebarEndTypes & endTypes)
{
	// 	DEllipse3d seg = DEllipse3d::FromPointsOnArc(arcSeg.ptStart, arcSeg.ptMid, arcSeg.ptEnd);
	// 
	// 	ICurvePrimitivePtr curve = ICurvePrimitive::CreateArc(seg);
	// 
	// 	CurveLocationDetail arcDetail;
	// 	curve->PointAtSignedDistanceFromFraction(0, space, false, arcDetail);
	// 	DPoint3d ptStart = arcDetail.point;
	// 	DPoint3d vec = { 0,0,1 };
	// 	vec.Scale(startOffset);
	// 	ptStart.Add(vec);
	// 
	// 	RebarVertexP vex;
	// 	vex = &rebar.PopVertices().NewElement();
	// 	vex->SetIP(ptStart);
	// 	vex->SetType(RebarVertex::kStart);
	// 	endTypes.beg.SetptOrgin(ptStart);
	// 
	// 	DPoint3d ptEnd = arcDetail.point;
	// 	ptEnd.z += dLen;
	// 	vec.Normalize();
	// 	vec.Negate();
	// 	vec.Scale(endOffset);
	// 	ptEnd.Add(vec);
	// 	endTypes.end.SetptOrgin(ptEnd);
	// 
	// 	vex = &rebar.PopVertices().NewElement();
	// 	vex->SetIP(ptEnd);
	// 	vex->SetType(RebarVertex::kEnd);
	// 
	// 	rebar.EvaluateEndTypes(endTypes);
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
	double dSideCover = GetConcrete().sideCover*uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, ptStart, ptEnd, dSideCover, matrix);

	/*EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
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

bool CamberedSurfaceRebarAssembly::makeArcWallRebarCurve(vector<PIT::PITRebarCurve>& rebar, ArcSegment arcSeg, double space, double startOffset, double endOffset, PITRebarEndTypes& endTypes)
{
	// 	DPoint3d ptArcStart = arcSeg.ptStart;
	// 	ptArcStart.z += space;
	// 	DPoint3d ptArcEnd = arcSeg.ptEnd;
	// 	ptArcEnd.z += space;
	// 	DPoint3d ptArcCenter = arcSeg.ptCenter;
	// 	ptArcCenter.z += space;
	// 	DPoint3d ptArcMid = arcSeg.ptMid;
	// 	ptArcMid.z += space;
	// 
	// 	return CalculateArc(rebar, ptArcStart, ptArcMid, ptArcEnd);
	DPoint3d ptArcStart = arcSeg.ptStart;
	ptArcStart.z += space;
	DPoint3d ptArcEnd = arcSeg.ptEnd;
	ptArcEnd.z += space;
	DPoint3d ptArcCenter = arcSeg.ptCenter;
	ptArcCenter.z += space;
	DPoint3d ptArcMid = arcSeg.ptMid;
	ptArcMid.z += space;


	EditElementHandle arceeh;
	ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(ptArcStart, ptArcMid, ptArcEnd), true, *ACTIVEMODEL);
	//arceeh.AddToModel();
	vector<DPoint3d> pts;
	GetARCIntersectPointsWithHoles(pts, m_useHoleehs, ptArcStart, ptArcEnd, ptArcMid);


	if (pts.size() > 0)
	{
		/*EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(pts[0], pts[1]), true, *ACTIVEMODEL);
		eeh.AddToModel();*/
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

		if (ISPointInHoles(m_Holeehs, tmpMid))
		{
			if (ISPointInHoles(m_Holeehs, tmpstr) && ISPointInHoles(m_Holeehs, tmpend))
			{
				continue;
			}
		}
		if (CalculateArc(trebar, tmpstr, tmpMid, tmpend))
		{
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(tmpstr, tmpMid, tmpend), true, *ACTIVEMODEL);
			if (FacePreviewButtonsDown)
			{//预览状态下画线,并存储线
				arceeh1.AddToModel();
				m_allLines.push_back(arceeh1.GetElementRef());
			}
			rebar.push_back(trebar);
		}
	}
	return true;
}


bool CamberedSurfaceRebarAssembly::OnDoubleClick()
{
	ElementId tmpid = GetSelectedElement();
	if (tmpid == 0)
	{
		return false;
	}
	DgnModelRefP modelp = GetSelectedModel();
	EditElementHandle ehSel;
	if (modelp == nullptr)
	{
		if (ehSel.FindByID(tmpid, ACTIVEMODEL) != SUCCESS)
		{
			ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
			for (DgnModelRefP modelRef : modelRefCol)
			{
				if (ehSel.FindByID(tmpid, modelRef) == SUCCESS)
				{
					modelp = modelRef;
					break;
				}

			}
		}
	}
	else
	{
		ehSel.FindByID(tmpid, modelp);
	}
	// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// 	CFacesRebarDlg dlg(ehSel,faces CWnd::FromHandle(MSWIND));
	// 	dlg.SetConcreteId(GetConcreteOwner());
	// 	if (IDCANCEL == dlg.DoModal())
	// 		return false;

	return true;
}

bool CamberedSurfaceRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();

	//	SetWallData(ehWall);
	MakeRebars(modelRef);
	Save(modelRef);
	return true;
}

bool CamberedSurfaceRebarAssembly::AnalyzingFaceGeometricData(EditElementHandleR eeh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eeh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	if (m_Solid != NULL)
	{
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(*m_Solid, Eleeh, Holeehs);
		m_Holeehs = Holeehs;

		vector<MSElementDescrP> vecDownFaceLine;
		vector<MSElementDescrP> vecDownFontLine;
		vector<MSElementDescrP> vecDownBackLine;
		ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine, vecDownBackLine, NULL);

		for (size_t i = 0; i < vecDownFaceLine.size(); i++)
		{
			if (vecDownFaceLine[i] != nullptr)
			{
				if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
				{
					DEllipse3d ellipsePro;
					mdlArc_extractDEllipse3d(&ellipsePro, &vecDownFaceLine[i]->el);
					DPoint3d ptStart, ptEnd;
					ellipsePro.EvaluateEndPoints(ptStart, ptEnd);
					double dRadius = ptStart.Distance(ellipsePro.center);
					if (dRadius > _dOuterArcRadius)
					{
						_dOuterArcRadius = dRadius;
					}
				}
			}
		}
	}

	ElementAgenda agenda;//存放打散之后的元素
	DropGeometryPtr pDropGeometry = DropGeometry::Create();//创建一个DropGeometry实例来设置几何选项
	if (eeh.GetElementType() == CMPLX_SHAPE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Complex);
	}
	else if (eeh.GetElementType() == SURFACE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Solids);
	}
	else
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_LinearSegments);
	}
	if (SUCCESS != eeh.GetDisplayHandler()->Drop(eeh, agenda, *pDropGeometry))
	{
		agenda.Clear();
		mdlOutput_printf(MSG_STATUS, L"打散面失败！");
		return false;
	}

	DEllipse3d ellipsePro;
	ArcSegment arcSeg;
	vector<LineSegment> vecLineSeg;
	vector<ArcSegment> vecArcSeg;
	for (EditElementHandleR LineEeh : agenda)
	{
		if (LineEeh.GetElementType() == ARC_ELM)
		{
			mdlArc_extractDEllipse3d(&ellipsePro, LineEeh.GetElementP());
			ellipsePro.EvaluateEndPoints(arcSeg.ptStart, arcSeg.ptEnd);
			arcSeg.dRadius = arcSeg.ptStart.Distance(ellipsePro.center);
			arcSeg.ptCenter = ellipsePro.center;
			arcSeg.dLen = ellipsePro.ArcLength();
			mdlElmdscr_pointAtDistance(&arcSeg.ptMid, NULL, arcSeg.dLen / 2, LineEeh.GetElementDescrP(), 1e-6);
			vecArcSeg.push_back(arcSeg);
		}

		if (LineEeh.GetElementType() == LINE_ELM)
		{
			DPoint3d pt[2];
			mdlLinear_extract(pt, NULL, LineEeh.GetElementP(), eeh.GetModelRef());
			LineSegment lineSeg(pt[0], pt[1]);
			vecLineSeg.push_back(lineSeg);
		}
	}

	if (vecArcSeg.size() != 2 && vecLineSeg.size() != 2)
	{
		return false;
	}

	if (COMPARE_VALUES_EPS(vecArcSeg[0].dRadius, vecArcSeg[1].dRadius, 1e-6) != 0)//圆环面
	{
		m_height = fabs(vecArcSeg[0].dRadius - vecArcSeg[1].dRadius);
		//弧线取圆环内弧
		m_ArcSeg = vecArcSeg[0].dRadius < vecArcSeg[1].dRadius ? vecArcSeg[0] : vecArcSeg[1];
		//		m_arcType = ArcType::Torus;
	}
	else	//侧面
	{
		m_height = vecLineSeg[0].GetLength();	//任意取一条直角边即为
		//弧线取z值最小的弧线
		m_ArcSeg = vecArcSeg[0].ptCenter.z < vecArcSeg[1].ptCenter.z ? vecArcSeg[0] : vecArcSeg[1];
		//		m_arcType = ArcType::CurvedSurface;
	}
	// mdlElmdscr_extractNormal(&PopfaceNormal(), NULL, eeh.GetElementDescrCP(), NULL);

	return true;
}

bool CamberedSurfaceRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	CalculateUseHoles(modelRef);

	RebarSetTagArray rsetTags;
	m_vecRebarStartEnd.clear();
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
	double dLevelSpace = 0;
	double dSideCover = GetConcrete().sideCover * uor_per_mm;


	int iRebarSetTag = 0;
	int iRebarLevelNum = GetConcrete().rebarLevelNum;
	int iTwinbarSetIdIndex = 0;
	vector<PIT::EndType> vecEndType;
	double newRadius = m_ArcSeg.dRadius;
	double diameterPre = 0.0;

	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		RebarSetTag* tag = NULL;
		vector<PIT::EndType> vecEndType;
		if (GetvecEndTypes().empty())		//没有设置端部样式，设置默认值
		{
			EndType endType;
			memset(&endType, 0, sizeof(endType));
			vecEndType = { { 0,0,0 },{0,0,0} };
		}
		else
		{
			vecEndType = GetvecEndTypes().at(i);
		}
		BrString strRebarSize(GetMainRebars().at(i).rebarSize);
		double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
		double spacing = GetMainRebars().at(i).spacing * uor_per_mm;
		double startOffset = GetMainRebars().at(i).startOffset * uor_per_mm;
		double endOffset = GetMainRebars().at(i).endOffset * uor_per_mm;
		int	rebarDir = GetMainRebars().at(i).rebarDir;
		int DataExchange = GetMainRebars().at(i).datachange;
		double levelspacing = GetMainRebars().at(i).levelSpace * uor_per_mm;

		double radiusOffset = 0;
		if (0 == i)	//首层偏移当前钢筋半径
		{
			radiusOffset += GetConcrete().postiveCover * uor_per_mm;
			radiusOffset += diameter * 0.5;
		}
		else
		{
			diameterPre = RebarCode::GetBarDiameter(GetMainRebars().at(i - 1).rebarSize, modelRef);	//上一层钢筋直径
			radiusOffset += diameterPre * 0.5;	//偏移上一层钢筋的半径
			radiusOffset += diameter * 0.5;	//偏移当前层钢筋的半径
		}

		radiusOffset += GetMainRebars().at(i).levelSpace * uor_per_mm;	//偏移层间距

		ArcSegment newArc = m_ArcSeg;
		// 		DPoint3d vecArc = newArc.ptCenter - newArc.ptStart;
		// 		vecArc.Normalize();
		// 		vecArc.z = 0;
		// 		DPoint3d ptLine1 = newArc.ptStart;
		// 		DPoint3d ptLine2 = newArc.ptStart;
		// 		DPoint3d ptNormal = GetfaceNormal();
		// 		ptNormal.z = 0;
		// 		double dot = vecArc.DotProduct(ptNormal);
		if (COMPARE_VALUES_EPS(m_ArcSeg.dRadius, _dOuterArcRadius, 10) == 0/*COMPARE_VALUES(dot, 0) > 0*/)  //外弧
		{
			newRadius -= radiusOffset;
		}
		else	//内弧
		{
			newRadius += radiusOffset;
		}

		newArc.ScaleToRadius(newRadius);		//整体缩放
// 		newArc.CutArc(m_ArcWallData.OuterArc.ptStart, m_ArcWallData.InnerArc.ptEnd);
// 		newArc.CutArc(m_ArcWallData.OuterArc.ptEnd, m_ArcWallData.InnerArc.ptStart);

		vector<CVector3D> vecEndNormal(2);
		if (0 == GetMainRebars().at(i).rebarDir)	//水平弧形钢筋
		{
			newArc.Shorten(dSideCover, false);		//起点缩短
			newArc.Shorten(dSideCover, true);		//终点缩短
			newArc.OffsetByAxisZ(diameter * 0.5 + dSideCover/* + diameterTie*/);
			PopSetIds().push_back(0);
			tag = MakeRebars_Arc(PopSetIds().back(), strRebarSize, newArc, spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();
		}
		else
		{
			newArc.Shorten(dSideCover + diameter * 0.5, false);		//起点缩短
			newArc.Shorten(dSideCover + diameter * 0.5, true);		//终点缩短
			newArc.OffsetByAxisZ(dSideCover/* + diameterTie*/);
			double dLen = m_height - (dSideCover/* + diameterTie*/) * 2;
			PopSetIds().push_back(0);
			tag = MakeRebars_Line(PopSetIds().back(), strRebarSize, newArc, dLen, spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();
		}
	}

	if (FacePreviewButtonsDown)
	{
		for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)//这里只画了直线，弧线在MakeRebars_Arc里面画完了
		{
			vector<DSegment3d> vcttemp(*it);
			for (int x = 0; x < vcttemp.size(); x++)
			{
				DPoint3d strPoint = DPoint3d::From(vcttemp[x].point[0].x, vcttemp[x].point[0].y, vcttemp[x].point[0].z);
				DPoint3d endPoint = DPoint3d::From(vcttemp[x].point[1].x, vcttemp[x].point[1].y, vcttemp[x].point[1].z);
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allLines.push_back(eeh.GetElementRef());//存储所有直线
			}
		}
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

RebarSetTag* CamberedSurfaceRebarAssembly::MakeRebars_Line
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	ArcSegment			arcSeg,
	double              dLen,
	double              spacing,
	double              startOffset,
	double              endOffset,
	int					level,
	int					grade,
	int					DataExchange,
	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
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
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double adjustedXLen, adjustedSpacing;

	double sideCov = GetConcrete().sideCover * uor_per_mm;
	int numRebar = 0;
	adjustedXLen = arcSeg.dLen - sideCov - diameter - startOffset - endOffset;

	numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);

	double xPos = startOffset;
	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);

	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);

	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };
	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);

	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurve;
		//		RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
		makeLineRebarCurve(rebarCurve, arcSeg, dLen, xPos, endTypeStartOffset, endTypEendOffset, endTypes);

		xPos += adjustedSpacing;

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurve.begin(), rebarCurve.end());
	}
	numRebar = (int)rebarCurvesNum.size();

	vector<DSegment3d> vecStartEnd;
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
		vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));

		RebarElementP rebarElement = NULL;
		if (!FacePreviewButtonsDown)
		{
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}

		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (DataExchange == 0)
			{
				Stype = "front";
			}
			else if (DataExchange == 1)
			{
				Stype = "midden";
			}
			else
			{
				Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
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


RebarSetTag * CamberedSurfaceRebarAssembly::MakeRebars_Arc(ElementId & rebarSetId, BrStringCR sizeKey, ArcSegment arcSeg, double spacing, double startOffset, double endOffset, int level, int grade, int DataExchange, vector<PIT::EndType> const & endType, vector<CVector3D> const & vecEndNormal, DgnModelRefP modelRef)
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
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double adjustedXLen, adjustedSpacing;

	// 	BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
	// 	if (strTieRebarSize.Find(L"mm") != string::npos)
	// 		strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm
	// 	double diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//拉筋直径

	double sideCov = GetConcrete().sideCover * uor_per_mm;
	int numRebar = 0;
	adjustedXLen = m_height - sideCov * 2 - diameter /*- diameterTie*2*/ - startOffset - endOffset;
	numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);

	double xPos = startOffset;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };
	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);

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

	//	vector<DSegment3d> vecStartEnd;
	for (int j = 0; j < (int)rebarCurvesNum.size(); ++j)
	{
		PITRebarCurve rebarCurve = rebarCurvesNum[j];

		RebarElementP rebarElement = NULL;
		if (!FacePreviewButtonsDown)//预览标志
		{
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (DataExchange == 0)
			{
				Stype = "front";
			}
			else if (DataExchange == 1)
			{
				Stype = "midden";
			}
			else
			{
				Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
	}

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

void CamberedSurfaceRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
{
	m_useHoleehs.clear();

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetConcrete().sideCover*uor_per_mm;

	if (GetConcrete().isHandleHole)//计算需要处理的孔洞
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
			DPoint3d ptcenter = m_ArcSeg.ptCenter;
			DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());

			ptcenter.z = ptele.z;
			CVector3D yVec = ptcenter - ptele;
			yVec.Normalize();

			CVector3D  xVec = CVector3D::kZaxis.CrossProduct(yVec);

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

			if (isNeed)
			{
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*m_Holeehs.at(j));
				PlusSideCover(*m_Holeehs.at(j), dSideCover, trans);
				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}
}