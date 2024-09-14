#include "_ustation.h"
#include "PITRebarCurve.h"
#include "SelectRebarTool.h"
#include "ScanIntersectTool.h"
#include "RebarHelper.h"

using namespace PIT;
bool PIT::GetAssemblySelectElement(EditElementHandleR ehSel, RebarAssembly* assem)
{
	ElementId conid = assem->GetConcreteOwner();
	ElementId tmpid = assem->GetSelectedElement();
	if (tmpid == 0)
	{
		return false;
	}
	DgnModelRefP  modelp = assem->GetSelectedModel();
	if (modelp == nullptr)
	{
		if (ehSel.FindByID(tmpid, ACTIVEMODEL) != SUCCESS)
		{
			ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
			for (DgnModelRefP modelRef : modelRefCol)
			{
				EditElementHandle tmpeeh(tmpid, modelRef);
				if (tmpeeh.IsValid())
				{
					ElementId testid = 0;
					GetElementXAttribute(tmpid, sizeof(ElementId), testid, ConcreteIDXAttribute, modelRef);
					if (testid == conid)
					{
						modelp = modelRef;
						break;
					}
				}

			}
		}
		else
		{
			ElementId testid = 0;
			GetElementXAttribute(tmpid, sizeof(ElementId), testid, ConcreteIDXAttribute, ACTIVEMODEL);
			if (testid == conid)
			{
				modelp = ACTIVEMODEL;
			}
			else
			{
				ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
				for (DgnModelRefP modelRef : modelRefCol)
				{
					EditElementHandle tmpeeh(tmpid, modelRef);
					if (tmpeeh.IsValid())
					{
						ElementId testidtmp = 0;
						GetElementXAttribute(tmpid, sizeof(ElementId), testidtmp, ConcreteIDXAttribute, modelRef);
						if (testidtmp == conid)
						{
							modelp = modelRef;
							break;
						}
					}

				}
			}
		}
	}
	if (modelp == nullptr)
	{
		EditElementHandle coneeh(conid, ACTIVEMODEL);
		std::vector<IDandModelref>  Same_Eles;
		GetNowIntersectDatas(coneeh, Same_Eles);
		for (IDandModelref ele:Same_Eles)
		{
			EditElementHandle tmpeeh(ele.ID, ele.tModel);
			if (tmpeeh.IsValid())
			{
				ElementId testidtmp = 0;
				GetElementXAttribute(ele.ID, sizeof(ElementId), testidtmp, ConcreteIDXAttribute, ele.tModel);
				if (testidtmp == conid)
				{
					tmpid = ele.ID;
					modelp = ele.tModel;
					break;
				}
			}
		}
	}
	ehSel.FindByID(tmpid, modelp);
	if (ehSel.IsValid())
	{
		return true;
	}
	return false;
}
void PIT::SetLevelidByRebarData(std::vector<PIT::ConcreteRebar>& vecRebarData)
{
	std::vector<PIT::ConcreteRebar> backdata;
	for (PIT::ConcreteRebar data : vecRebarData)
	{
		if (data.datachange == 2)
		{
			backdata.push_back(data);
		}
	}
	int frontid = 0;
	int midid = 0;
	int endid = (int)backdata.size() + 1;
	for (PIT::ConcreteRebar& data : vecRebarData)
	{
		if (data.datachange == 0)
		{
			data.rebarLevel = frontid + 1;
			frontid = data.rebarLevel;
		}
		else if (data.datachange == 1)
		{
			data.rebarLevel = midid + 1;
			midid = data.rebarLevel;
		}
		else
		{
			data.rebarLevel = endid - 1;
			endid = data.rebarLevel;
		}
	}

}
void PIT::PITRebarCurve::GenerateArc(BeArcSegR seg, DPoint3dCR ptOrgin,DPoint3dCR ptVec, CVector3D endNormal, double angle,double bendRadius) const
{
	DPoint3d vec = ptVec;
	vec.Normalize();
	double disH;
	if (angle == PI)
	{
		disH = bendRadius;
	}
	else
	{
		disH = bendRadius / tan(angle / 2);
	}
	vec.Scale(disH);

	CPoint3D ptBend, ptMid, ptEnd;

	ptMid = ptOrgin;
	ptEnd = ptMid;
	ptEnd.Add(vec);

	CPoint3D ptNormal = endNormal;
	if (angle == PI)
	{
		ptNormal.Scale(disH*2);
	}
	else
	{
		ptNormal.Scale(disH*sin(angle));
	}
	ptBend = ptMid;
	ptBend.Add(ptNormal);
	CPoint3D ptVecH = ptVec;
	ptVecH.Normalize();
	if (angle == PI)
	{
		ptVecH.Scale(disH);
	}
	else
	{		
		ptVecH.Scale(disH*cos(angle));	
	}
	ptBend.Add(ptVecH);

	/*DEllipse3d delp;
	if (angle == PI)
	{
        vec = ptVec;
		vec.Normalize();
		vec.Negate();
		vec.Scale(disH);
		ptMid = ptBend;
		ptMid.Add(vec);
		ptNormal = endNormal;
		ptNormal.Negate();
		ptNormal.Scale(disH);
		ptMid.Add(ptNormal);
		delp = DEllipse3d::FromPointsOnArc(ptBend, ptMid, ptEnd);
	}
	else
	{
		CPoint3D ptCenter = endNormal;
		ptCenter.Scale(bendRadius);
		ptCenter.Add(ptEnd);
		delp = DEllipse3d::FromArcCenterStartEnd(ptCenter, ptBend, ptEnd);
	}*/

	CPoint3D ptCenter = endNormal;
	ptCenter.Scale(bendRadius);
	ptCenter.Add(ptEnd);
	DVec3d center2Orgin = DVec3d::FromStartEnd(ptCenter, ptOrgin);
	center2Orgin.ScaleToLength(bendRadius);
	ptCenter.Add(center2Orgin);
	if (angle == PI)
	{
		ptCenter = ptOrgin;
	}

	//EditElementHandle arcEeh;
	//ArcHandler::CreateArcElement(arcEeh, nullptr, delp, true, *ACTIVEMODEL);
	//arcEeh.AddToModel();

// ;	EditElementHandle eeh1;
// 	DPoint3d pt[3] = {ptBend,ptMid,ptEnd};
// 	LineStringHandler::CreateLineStringElement(eeh1, NULL, pt, 3, true, *ACTIVEMODEL);
// 	eeh1.AddToModel();
	//delp.Evaluate(seg.mid, sin(PI/4), cos(PI / 4));
	/*seg.start = ptBend;
	seg.end = ptEnd*/;
	seg.mid = ptCenter;
	seg.start = ptBend;
	seg.end = ptEnd;
//   	DEllipse3d delp1 = DEllipse3d::FromPointsOnArc(ptBend, seg.mid, ptEnd);
// 	EditElementHandle eehArc;
// 	ArcHandler::CreateArcElement(eehArc, NULL, delp1, true, *ACTIVEMODEL);
// 	eehArc.AddToModel();
}

// void PITRebarCurve::GetEndTypeVertex(std::vector<CPoint3D> &vex, BeArcSegCR seg, CVector3D endNormal, double len) const
// {
// 	double bendRadius = seg.GetRadius();
// 
// 	CPoint3D ptBend, ptMid, ptEnd;
// 
// 	ptMid = seg.GetMid();
// 	ptEnd = seg.end;
// 	DPoint3d pt1 = endNormal;
// 	pt1.Scale(bendRadius + len);
// 	pt1.Add(ptMid);
// 
// 
// 	// 	CPoint3D ptNormal = endNormal;
// 	// 	ptNormal.Scale(bendRadius);
// 	// 	ptBend = ptMid;
// 	// 	ptBend.Add(ptNormal);
// 
// 	ptBend = seg.start;
// 
// 	vex = { pt1,ptBend,ptMid,ptEnd };
// }

void PIT::PITRebarCurve::RebarEndBendBeg_Arc(const PITRebarEndTypes & endType, DPoint3d rebarVec, double angle, bool isStirrup)
{
	double bendLen = endType.beg.GetbendLen();
	double bendRadius = endType.beg.GetbendRadius();
	CVector3D normal = endType.beg.GetendNormal();
	DPoint3d ptOrgin = endType.beg.GetptOrgin();

	BeArcSeg arcSeg;
	GenerateArc(arcSeg, ptOrgin, rebarVec, normal, angle, bendRadius);

	rebarVec.Normalize();
	rebarVec.Scale(bendRadius);

	DPoint3d pt1;
	if (angle == PI)
	{
		pt1 = rebarVec;
	}
	else
	{
		DPoint3d vecExtend = rebarVec;
		vecExtend.Negate();
		double dis = arcSeg.start.Distance(ptOrgin);
		vecExtend.ScaleToLength(dis - bendRadius);
		ptOrgin.Add(vecExtend);
		GenerateArc(arcSeg, ptOrgin, rebarVec, normal, angle, bendRadius);
		pt1 = DPoint3d(arcSeg.start) - ptOrgin;
	}
	pt1.Normalize();
	pt1.Scale(bendLen);
	pt1.Add(arcSeg.start);

	// 钢筋顶点后移一位
	PopVertices().NewElement();
	for (int i = (int)PopVertices().GetSize() - 1; i > 0; i--)
	{
		PopVertices().At(i) = PopVertices().At(i - 1);
	}

	RebarVertexP vex;
	vex = &PopVertices().At(0);
	vex->SetIP(pt1);
	vex->SetType(RebarVertex::kStart);

	CPoint3D ptCenter = normal;
	ptCenter.Scale(bendRadius);
	ptCenter.Add(arcSeg.end);

	vex = &PopVertices().At(1);
	vex->SetIP(ptOrgin);
	vex->SetType(RebarVertex::kIP);
	vex->SetRadius(bendRadius);
	vex->SetCenter(ptCenter);
	vex->SetArcPt(0, arcSeg.start);
	vex->SetArcPt(1, arcSeg.mid);
	vex->SetArcPt(2, arcSeg.end); // 起点的弧

	for (int i = 2; i < PopVertices().GetSize(); i++)
	{
		vex = &PopVertices().At(i);
		if (i == PopVertices().GetSize() - 1 && endType.end.GetType() != PITRebarEndType::Type::kNone)
		{
			vex->SetType(RebarVertex::kIP);
		}
		else if (i == 2)
		{
			vex->SetArcPt(0, arcSeg.end);
		}
	}

	// 	EditElementHandle eehArc;
	// 	ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromPointsOnArc(arcSeg.start, arcSeg.mid, arcSeg.end), true, *ACTIVEMODEL);
	// 	eehArc.AddToModel();
}

void PIT::PITRebarCurve::RebarEndBendBeg(const PITRebarEndTypes & endType,DPoint3d rebarVec, double angle, bool isStirrup)
{
	DPoint3d vecLine = rebarVec;
	vecLine.Normalize();
	vecLine.Scale(-1);
	double bendLen = endType.beg.GetbendLen();
	double bendRadius = endType.beg.GetbendRadius();
	CVector3D normal = endType.beg.GetendNormal();
	DPoint3d ptOrgin = endType.beg.GetptOrgin();

	if (angle > PI / 2)
	{
		CVector3D tmpVec = vecLine;
		tmpVec.Negate();
		CVector3D faceNormal = tmpVec.CrossProduct(CVector3D(normal));
		faceNormal.Normalize();	
		normal = tmpVec;
		normal.Rotate(PI / 2, faceNormal);
	}
		
	BeArcSeg arcSeg;
	GenerateArc(arcSeg, ptOrgin, rebarVec, normal, angle, bendRadius);

	rebarVec.Normalize();
	rebarVec.Scale(bendRadius);

	DPoint3d pt1;
	if (angle == PI)
	{
		pt1 = rebarVec;
	}
	else
	{
		DPoint3d vecExtend = rebarVec;
		vecExtend.Negate();
		double dis = arcSeg.start.Distance(ptOrgin);
		vecExtend.ScaleToLength(dis - bendRadius);
		ptOrgin.Add(vecExtend);
		GenerateArc(arcSeg, ptOrgin, rebarVec, normal, angle, bendRadius);
		pt1 = DPoint3d(arcSeg.start) - ptOrgin;
	}
	pt1.Normalize();
	pt1.Scale(bendLen);
	pt1.Add(arcSeg.start);

	RebarVertexP vex;
	vex = &PopVertices().At(0);
	vex->SetIP(pt1);
	vex->SetType(RebarVertex::kStart);

	CPoint3D ptCenter = normal;
	ptCenter.Scale(bendRadius);
	ptCenter.Add(arcSeg.end);

	vex = &PopVertices().At(1);
	vex->SetIP(ptOrgin);
	vex->SetType(RebarVertex::kIP);
	vex->SetRadius(bendRadius);
	vex->SetCenter(ptCenter);
	vex->SetArcPt(0, arcSeg.start);
	vex->SetArcPt(1, arcSeg.mid);
	vex->SetArcPt(2, arcSeg.end); // 起点的弧

	DPoint3d tanpt1, tanpt2;
	tanpt1 = arcSeg.start;
	tanpt2 = arcSeg.end;

	DPoint3d vecTan = ptOrgin - pt1;
	vecTan.Normalize();
	vecTan.Scale(bendRadius);
	tanpt1.Add(vecTan);
	vecLine.Scale(bendRadius);
	tanpt2.Add(vecLine);

	vex->SetTanPt(0, tanpt1);
	vex->SetTanPt(1, tanpt2);

	
	DPoint3d pt2 = endType.end.GetptOrgin();
	vex = &PopVertices().NewElement();
	if (!isStirrup)
	{
		if (endType.end.GetType() == PITRebarEndType::Type::kNone)
		{
			vex->SetType(RebarVertex::kEnd);
			vex->SetIP(pt2);
		}
		else
		{
			//中点
			rebarVec.Normalize();
			rebarVec.Negate();
			rebarVec.Scale(endType.end.GetbendRadius());
			pt2.Add(rebarVec);
			vex->SetIP(pt2);
			vex->SetType(RebarVertex::kIP);
		}
	}
	else
	{
		for (int i = 2; i < PopVertices().GetSize(); i++)
		{
			vex = &PopVertices().At(i);
			if (i == PopVertices().GetSize() - 1 && endType.end.GetType() == PITRebarEndType::Type::kNone)
			{
				vex->SetType(RebarVertex::kEnd);
				vex->SetIP(pt2);
			}
 			else
			{
				DPoint3d ptTmp = vex->GetIP();
				vex->SetIP(pt2);
				vex->SetRadius(bendRadius);
				vex->SetType(RebarVertex::kIP);
				pt2 = ptTmp;
			}
		}
		for (int i = 2; i < PopVertices().GetSize() - 1; i++)
		{
			PopVertices()[i]->EvaluateBend(*PopVertices()[i - 1], *PopVertices()[i + 1]);
		}
	}

// 	EditElementHandle eehArc;
// 	ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromPointsOnArc(arcSeg.start, arcSeg.mid, arcSeg.end), true, *ACTIVEMODEL);
// 	eehArc.AddToModel();
}

void PIT::PITRebarCurve::RebarEndBendEnd(const PITRebarEndTypes & endType, DPoint3d rebarVec, double angle, bool isArcRebar)
{
	DPoint3d vecLine = rebarVec;
	vecLine.Normalize();
	vecLine.Scale(-1);


	double bendLen = endType.end.GetbendLen();
	double bendRadius = endType.end.GetbendRadius();
	CVector3D normal = endType.end.GetendNormal();
	DPoint3d ptOrgin = endType.end.GetptOrgin();
	if (angle > PI / 2)
	{
		CVector3D tmpVec = vecLine;
		tmpVec.Negate();
		CVector3D faceNormal = tmpVec.CrossProduct(CVector3D(normal));
		faceNormal.Normalize();
		normal = tmpVec;
		normal.Rotate(PI / 2, faceNormal);
	}
	BeArcSeg arcSeg;
	GenerateArc(arcSeg, endType.end.GetptOrgin(), rebarVec, normal, angle, bendRadius);

	rebarVec.Normalize();
	rebarVec.Scale(bendRadius);

	DPoint3d pt1;
	if (angle == PI)
	{
		pt1 = rebarVec;
	}
	else
	{
		DPoint3d vecExtend = rebarVec;
		vecExtend.Negate();
		double dis = arcSeg.start.Distance(ptOrgin);
		//	double len = bendRadius * sin(angle);
		vecExtend.ScaleToLength(dis - bendRadius);
		ptOrgin.Add(vecExtend);
		GenerateArc(arcSeg, ptOrgin, rebarVec, normal, angle, bendRadius);
		pt1 = DPoint3d(arcSeg.start) - ptOrgin;
	}
	pt1.Normalize();
	pt1.Scale(bendLen);
	pt1.Add(arcSeg.start);

	CPoint3D ptCenter = normal;
	ptCenter.Scale(bendRadius);
	ptCenter.Add(arcSeg.end);

	RebarVertexP vex;
	if (isArcRebar)
	{
		vex = &PopVertices().At(PopVertices().GetSize() - 2);
		vex->SetArcPt(2, arcSeg.end);
	}
	
	vex = &PopVertices().At(PopVertices().GetSize() - 1);
	//if (endType.beg.GetType() == PITRebarEndType::Type::kNone)	//起点没有端部样式
	//{
	//	//vex = &PopVertices().At(0);
	//	//vex->SetIP(endType.beg.GetptOrgin());
	//	//vex->SetType(RebarVertex::kStart);
	//	vex = &PopVertices().At(PopVertices().GetSize() - 1);
	//}
	//else
	//{
	//	vex = &PopVertices().NewElement();
	//}

	vex->SetIP(ptOrgin);
	vex->SetType(RebarVertex::kIP);
	vex->SetRadius(bendRadius);
	vex->SetCenter(ptCenter);

	vex->SetArcPt(0, arcSeg.end);
	vex->SetArcPt(1, arcSeg.mid);
	vex->SetArcPt(2, arcSeg.start);

	DPoint3d tanpt1, tanpt2;
	tanpt1 = arcSeg.end;
	vecLine.Scale(bendRadius);
	tanpt1.Add(vecLine);
	tanpt2 = arcSeg.start;
	DPoint3d vecTan = ptOrgin - pt1;
	vecTan.Normalize();
	vecTan.Scale(bendRadius);
	tanpt2.Add(vecTan);
	vex->SetTanPt(0, tanpt1);
	vex->SetTanPt(1, tanpt2);


	vex = &PopVertices().NewElement();
	vex->SetIP(pt1);
	vex->SetType(RebarVertex::kEnd);

// 	EditElementHandle eehArc;
// 	ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromPointsOnArc(arcSeg.start, arcSeg.mid, arcSeg.end), true, *ACTIVEMODEL);
// 	eehArc.AddToModel();
}

void PIT::PITRebarCurve::EvaluateEndTypesArc(PITRebarEndTypes endType)
{
	if (PopVertices().GetSize() < 2)
	{
		return;
	}

	DPoint3d ptLineStart = PopVertices().At(0).GetIP();
	DPoint3d ptLineIP1 = PopVertices().At(1).GetIP();

	vector<DPoint3d> vecPt;
	DPoint3d vec = ptLineIP1 - ptLineStart;
	vec.Normalize();

	endType.beg.SetptOrgin(ptLineStart);

	switch (endType.beg.GetType())
	{
	case PITRebarEndType::Type::kNone:
		break;
	case PITRebarEndType::Type::kBend:	//90
		RebarEndBendBeg_Arc(endType, vec, PI / 2);
		break;
	case PITRebarEndType::Type::kCog:	//135
		RebarEndBendBeg_Arc(endType, vec, PI / 4);
		break;
	case PITRebarEndType::Type::kHook:	//180
		RebarEndBendBeg_Arc(endType, vec, PI);
		break;
	case PITRebarEndType::Type::kLap:	//直锚暂时在这里处理
		RebarEndStraightAnchor(endType, true, true);
		break;
	case PITRebarEndType::Type::kCustom:
		break;
	case PITRebarEndType::Type::kSplice:
		break;
	case PITRebarEndType::Type::kTerminator:
		break;
	default:
		break;
	}

	DPoint3d ptLineIP2 = PopVertices().At(PopVertices().GetSize() - 2).GetIP();
	DPoint3d ptLineEnd = PopVertices().At(PopVertices().GetSize() - 1).GetIP();
	vec = ptLineIP2 - ptLineEnd;
	vec.Normalize();
	endType.end.SetptOrgin(ptLineEnd);
	switch (endType.end.GetType())
	{
	case PITRebarEndType::Type::kNone:
		break;
	case PITRebarEndType::Type::kBend:
		RebarEndBendEnd(endType, vec, PI / 2, true);
		break;
	case PITRebarEndType::Type::kCog:
		RebarEndBendEnd(endType, vec, PI / 4, true);
		break;
	case PITRebarEndType::Type::kHook:
		RebarEndBendEnd(endType, vec, PI, true);
		break;
	case PITRebarEndType::Type::kLap:
		RebarEndStraightAnchor(endType, false);
		break;
	case PITRebarEndType::Type::kCustom:
		break;
	case PITRebarEndType::Type::kSplice:
		break;
	case PITRebarEndType::Type::kTerminator:
		break;
	default:
		break;
	}
}

void PIT::PITRebarCurve::EvaluateEndTypesStirrup(PITRebarEndTypes endType)
{
	if (PopVertices().GetSize() < 2)
	{
		return;
	}

	DPoint3d ptLineStart = PopVertices().At(0).GetIP();
	DPoint3d ptLineIP1 = PopVertices().At(1).GetIP();

	vector<DPoint3d> vecPt;
	DPoint3d vec = ptLineIP1 - ptLineStart;
	vec.Normalize();

	endType.beg.SetptOrgin(ptLineStart);
	endType.end.SetptOrgin(ptLineIP1);
	switch (endType.beg.GetType())
	{
	case PITRebarEndType::Type::kNone:
		break;
	case PITRebarEndType::Type::kBend:	//90
		RebarEndBendBeg(endType, vec, PI / 2, true);
		break;
	case PITRebarEndType::Type::kCog:	//135
		RebarEndBendBeg(endType, vec, PI / 4, true);
		break;
	case PITRebarEndType::Type::kHook:	//180
		RebarEndBendBeg(endType, vec, PI, true);
		break;
	case PITRebarEndType::Type::kLap:	//直锚暂时在这里处理
		RebarEndStraightAnchor(endType, true, true);
		break;
	case PITRebarEndType::Type::kCustom:
		break;
	case PITRebarEndType::Type::kSplice:
		break;
	case PITRebarEndType::Type::kTerminator:
		break;
	default:
		break;
	}

	DPoint3d ptLineIP2 = PopVertices().At(PopVertices().GetSize() - 2).GetIP();
	DPoint3d ptLineEnd = PopVertices().At(PopVertices().GetSize() - 1).GetIP();
	vec = ptLineEnd - ptLineIP2;
	vec.Normalize();
	vec.Negate();
	endType.beg.SetptOrgin(ptLineIP2);
	endType.end.SetptOrgin(ptLineEnd);
	switch (endType.end.GetType())
	{
	case PITRebarEndType::Type::kNone:
		break;
	case PITRebarEndType::Type::kBend:
		RebarEndBendEnd(endType, vec, PI / 2);
		break;
	case PITRebarEndType::Type::kCog:
		RebarEndBendEnd(endType, vec, PI / 4);
		break;
	case PITRebarEndType::Type::kHook:
		RebarEndBendEnd(endType, vec, PI);
		break;
	case PITRebarEndType::Type::kLap:
		RebarEndStraightAnchor(endType, false, true);
		break;
	case PITRebarEndType::Type::kCustom:
		break;
	case PITRebarEndType::Type::kSplice:
		break;
	case PITRebarEndType::Type::kTerminator:
		break;
	default:
		break;
	}
// 	bvector<DPoint3d> pts;
// 	GetIps(pts);
// 	EditElementHandle eeh;
// 	LineStringHandler::CreateLineStringElement(eeh, nullptr, &pts[0], pts.size(), true, *ACTIVEMODEL);
// 	eeh.AddToModel();

}

void PIT::PITRebarCurve::RebarEndStraightAnchor(const PITRebarEndTypes & endType,bool bBegin, bool isStirrup)
{
	DPoint3d ptStart = endType.beg.GetptOrgin();
	DPoint3d ptEnd = endType.end.GetptOrgin();
	double	 dStraightAnchorLenBeg = endType.beg.GetstraightAnchorLen();
	double	 dStraightAnchorLenEnd = endType.end.GetstraightAnchorLen();

	DPoint3d rebarVec = ptEnd - ptStart;
	rebarVec.Normalize();

	RebarVertexP vex;
	if (bBegin)
	{
		DPoint3d pt1;
		pt1 = rebarVec;
		pt1.Negate();
		pt1.Scale(dStraightAnchorLenBeg);
		pt1.Add(ptStart);

		vex = &PopVertices().At(0);
		vex->SetIP(pt1);
		vex->SetType(RebarVertex::kStart);

		vex = &PopVertices().At(1);
		vex->SetIP(ptStart);
		vex->SetType(RebarVertex::kIP);

		if (!isStirrup)
		{
			if (endType.end.GetType() == PITRebarEndType::Type::kNone)	//终点没有端部样式
			{
				vex = &PopVertices().NewElement();
				vex->SetIP(ptEnd);
				vex->SetType(RebarVertex::kEnd);
			}
		}
		else
		{
			DPoint3d pt2 = ptEnd;
			PopVertices().NewElement();
			for (int i = 2; i < PopVertices().GetSize(); i++)
			{
				vex = &PopVertices().At(i);
				if (i == PopVertices().GetSize() - 1 && endType.end.GetType() == PITRebarEndType::Type::kNone)
				{
					vex->SetType(RebarVertex::kEnd);
					vex->SetIP(pt2);
				}
				else
				{
					DPoint3d ptTmp = vex->GetIP();
					vex->SetIP(pt2);
					vex->SetRadius(endType.beg.GetbendRadius());
					vex->SetType(RebarVertex::kIP);
					pt2 = ptTmp;
				}
			}
			for (int i = 2; i < PopVertices().GetSize() - 1; i++)
			{
				PopVertices()[i]->EvaluateBend(*PopVertices()[i - 1], *PopVertices()[i + 1]);
			}
		}
// 		EditElementHandle eh;
// 		LineHandler::CreateLineElement(eh, NULL, DSegment3d::From(pt1, ptStart), true, *ACTIVEMODEL);
// 		eh.AddToModel();
	}
	else
	{
		if (endType.beg.GetType() == PITRebarEndType::Type::kNone)	//起点没有端部样式
		{
			vex = &PopVertices().At(0);
			vex->SetIP(ptStart);
			vex->SetType(RebarVertex::kStart);
			vex = &PopVertices().At(1);
			vex->SetIP(ptEnd);
			vex->SetType(RebarVertex::kIP);
		}
		else
		{
			vex = &PopVertices().NewElement();
			vex->SetIP(ptEnd);
			vex->SetType(RebarVertex::kIP);
		}

		DPoint3d pt2 = rebarVec;
		pt2.Scale(dStraightAnchorLenEnd);
		pt2.Add(ptEnd);
		vex = &PopVertices().NewElement();
		vex->SetIP(pt2);
		vex->SetType(RebarVertex::kEnd);
	}
}

void PIT::PITRebarCurve::EvaluateEndTypes(const PITRebarEndTypes & endType)
{
	if (PopVertices().GetSize() < 2)
	{
		return;
	}

	DPoint3d ptLineStart = PopVertices().At(0).GetIP();
	DPoint3d ptLineEnd = PopVertices().At(1).GetIP();

// 	DPoint3d ptStr[2] = { ptLineStart,ptLineEnd};
// 	EditElementHandle eeh;
// 	LineStringHandler::CreateLineStringElement(eeh, NULL, ptStr, 2, true, *ACTIVEMODEL);
// 	eeh.AddToModel();
	EditElementHandle eehChain;
	ChainHeaderHandler::CreateChainHeaderElement(eehChain, NULL, false, true, *ACTIVEMODEL);

	vector<DPoint3d> vecPt;
	DPoint3d vec = ptLineEnd - ptLineStart;
	vec.Normalize();
	switch (endType.beg.GetType())
	{
	case PITRebarEndType::Type::kNone:
		break;
	case PITRebarEndType::Type::kBend:	//90
		RebarEndBendBeg(endType, vec, PI / 2);
		break;
	case PITRebarEndType::Type::kCog:	//135
		RebarEndBendBeg(endType, vec, PI / 4);		
		break;
	case PITRebarEndType::Type::kHook:	//180
		RebarEndBendBeg(endType, vec, PI);
		break;
	case PITRebarEndType::Type::kLap:	//直锚暂时在这里处理
		RebarEndStraightAnchor(endType, true);
		break;
	case PITRebarEndType::Type::kCustom:
		RebarEndBendBeg(endType, vec, endType.beg.Getangle());
		break;
	case PITRebarEndType::Type::kSplice:
		break;
	case PITRebarEndType::Type::kTerminator:
		break;
	case PITRebarEndType::kTie:	//100
		RebarEndBendBeg(endType, vec, 4*PI/9);
		break;
	default:
		break;
	}

	ptLineStart = PopVertices().At(GetNumberOfVertices()-1).GetIP();
	ptLineEnd = PopVertices().At(GetNumberOfVertices()-2).GetIP();

	vec = ptLineEnd - ptLineStart;
	vec.Normalize();
	switch (endType.end.GetType())
	{
	case PITRebarEndType::Type::kNone:
		break;
	case PITRebarEndType::Type::kBend:
		RebarEndBendEnd(endType, vec, PI / 2);
		break;
	case PITRebarEndType::Type::kCog:
		RebarEndBendEnd(endType, vec, PI / 4);
		break;
	case PITRebarEndType::Type::kHook:
		RebarEndBendEnd(endType, vec, PI);
		break;
	case PITRebarEndType::Type::kLap:
		RebarEndStraightAnchor(endType,false);
		break;
	case PITRebarEndType::Type::kCustom:
		RebarEndBendEnd(endType, vec, endType.end.Getangle());
		break;
	case PITRebarEndType::Type::kSplice:
		break;
	case PITRebarEndType::Type::kTerminator:
		break;
	case PITRebarEndType::kTie:	//100
		RebarEndBendEnd(endType, vec, 4 * PI / 9);
		break;
	default:
		break;
	}

	/*EditElementHandle eeh;
	LineStringHandler::CreateLineStringElement(eeh, NULL, &vecPt[0], vecPt.size(), true, *ACTIVEMODEL);
	eeh.AddToModel();*/
}

void PIT::PITRebarCurve::EvaluateEndTypes(const PITRebarEndTypes & endType, double angel1, double angel2)
{
	if (PopVertices().GetSize() < 2)
	{
		return;
	}

	DPoint3d ptLineStart = PopVertices().At(0).GetIP();
	DPoint3d ptLineEnd = PopVertices().At(1).GetIP();

	// 	DPoint3d ptStr[2] = { ptLineStart,ptLineEnd};
	// 	EditElementHandle eeh;
	// 	LineStringHandler::CreateLineStringElement(eeh, NULL, ptStr, 2, true, *ACTIVEMODEL);
	// 	eeh.AddToModel();
	EditElementHandle eehChain;
	ChainHeaderHandler::CreateChainHeaderElement(eehChain, NULL, false, true, *ACTIVEMODEL);

	vector<DPoint3d> vecPt;
	DPoint3d vec = ptLineEnd - ptLineStart;
	vec.Normalize();
	switch (endType.beg.GetType())
	{
	case PITRebarEndType::Type::kNone:
		break;
	case PITRebarEndType::kTie:	//100
	{

		double dTemp1 = angel1 / 180.0;
		if (COMPARE_VALUES_EPS(dTemp1, 1.0, EPS) == 0)
		{
			RebarEndBendBeg(endType, vec, PI);
		}
		else
		{
			RebarEndBendBeg(endType, vec, (1 - dTemp1)*PI);
		}
		break;
	}
	default:
		break;
	}

	ptLineStart = PopVertices().At(GetNumberOfVertices() - 1).GetIP();
	ptLineEnd = PopVertices().At(GetNumberOfVertices() - 2).GetIP();

	vec = ptLineEnd - ptLineStart;
	vec.Normalize();
	switch (endType.end.GetType())
	{
	case PITRebarEndType::Type::kNone:
		break;
	case PITRebarEndType::kTie:	//100
	{
		double dTemp2 = angel2 / 180.0;
		if (COMPARE_VALUES_EPS(dTemp2, 1.0, 0.1) == 0)
		{
			RebarEndBendEnd(endType, vec, PI);
		}
		else
		{
			RebarEndBendEnd(endType, vec, (1 - dTemp2)*PI);
		}
		break;
	}

	default:
		break;
	}

}


bool PIT::PITRebarCurve::makeURebarCurve(vector<CPoint3D> const& vecRebarVertex, double bendRadius)
{
	if (vecRebarVertex.size() < 4)
		return false;

	/*RebarVertexP vex;
	for (size_t i = 0; i < vecRebarVertex.size(); i++)
	{
		vex = &PopVertices().NewElement();
		vex->SetIP(vecRebarVertex[i]);
		if (0 == i)
			vex->SetType(RebarVertex::kStart);
		else if (vecRebarVertex.size() - 1 == i)
			vex->SetType(RebarVertex::kEnd);
		else
			vex->SetType(RebarVertex::kIP);
	}
	EvaluateBend(bendRadius);*/


	DPoint3d ptVertex1;
	DPoint3d ptVertex2;
	DPoint3d ptVertex3;
	for (int i = 0; i < vecRebarVertex.size(); i++)
	{
		RebarVertex*   vertmp;
		vertmp = &PopVertices().NewElement();
		if (i == 0)
		{
			ptVertex1 = vecRebarVertex[i];
			vertmp->SetType(RebarVertex::kStart);
			vertmp->SetIP(ptVertex1);
		}
		else if (i == vecRebarVertex.size() - 1)
		{

			vertmp->SetType(RebarVertex::kEnd);
			vertmp->SetIP(vecRebarVertex[i]);
		}
		else
		{

			ptVertex1 = vecRebarVertex[i - 1];
			ptVertex2 = vecRebarVertex[i];
			ptVertex3 = vecRebarVertex[i + 1];
			DPoint3d vec1 = ptVertex1 - ptVertex2;
			DPoint3d vec2 = ptVertex3 - ptVertex2;
			vec1.Normalize();
			vec2.Normalize();
			if (COMPARE_VALUES_EPS(vec1.DotProduct(vec2), 1.0, 0.1) == 0)//如果平行时
			{
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(vecRebarVertex[i]);
			}
			else
			{
				RebarHelper::RebarArcData arcdata;
				RebarHelper::IntersectionPointToArcDataRebar(arcdata, ptVertex2, ptVertex1, ptVertex3, bendRadius);
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(ptVertex2);
				vertmp->SetArcPt(0, arcdata.ptArcBegin);
				vertmp->SetArcPt(1, arcdata.ptArcMid);
				vertmp->SetArcPt(2, arcdata.ptArcEnd);
				vertmp->SetRadius(bendRadius);
				vertmp->SetCenter(arcdata.ptArcCenter);
			}

		}
	}

	return true;
}

bool PIT::PITRebarCurve::makeStirrupRebarCurve(vector<CPoint3D> const& vecRebarVertex, double bendRadius, PITRebarEndTypes endTypes)
{
	if (vecRebarVertex.size() < 5)
		return false;

	for (size_t i = 0; i < vecRebarVertex.size(); i++)
	{
		RebarVertexP vex;
		vex = &PopVertices().NewElement();
		vex->SetIP(vecRebarVertex[i]);
		if (i == 0)
		{
			vex->SetType(RebarVertex::kStart);
		}
		else if (i == vecRebarVertex.size() - 1)
		{
			vex->SetType(RebarVertex::kEnd);
		}
		else
		{
			vex->SetRadius(bendRadius);
			vex->SetType(RebarVertex::kIP);
		}
	}

// 	for (int i = 1; i < PopVertices().GetSize() - 2; i++)
// 	{
// 		PopVertices()[i]->EvaluateBend(*PopVertices()[i - 1], *PopVertices()[i + 1]);
// 	}

	EvaluateEndTypesStirrup(endTypes);
	return true;
}

bool PIT::PITRebarCurve::makeILURebarCurve(vector<CPoint3D> const& vecRebarVertex, double bendRadius)
{
	if (vecRebarVertex.size() < 2)
	{
		return false;
	}
	DPoint3d ptVertex1;
	DPoint3d ptVertex2;
	DPoint3d ptVertex3;
	for (int i = 0; i < vecRebarVertex.size(); i++)
	{
		RebarVertex*   vertmp;
		vertmp = &PopVertices().NewElement();
		if (i == 0)
		{
			ptVertex1 = vecRebarVertex[i];
			vertmp->SetType(RebarVertex::kStart);
			vertmp->SetIP(ptVertex1);
		}
		else if (i == vecRebarVertex.size() - 1)
		{

			vertmp->SetType(RebarVertex::kEnd);
			vertmp->SetIP(vecRebarVertex[i]);
		}
		else
		{

			ptVertex1 = vecRebarVertex[i - 1];
			ptVertex2 = vecRebarVertex[i];
			ptVertex3 = vecRebarVertex[i + 1];
			DPoint3d vec1 = ptVertex1 - ptVertex2;
			DPoint3d vec2 = ptVertex3 - ptVertex2;
			vec1.Normalize();
			vec2.Normalize();
			if (COMPARE_VALUES_EPS(vec1.DotProduct(vec2), 1.0, 0.1) == 0)//如果平行时
			{
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(vecRebarVertex[i]);
			}
			else
			{
				RebarHelper::RebarArcData arcdata;
				RebarHelper::IntersectionPointToArcDataRebar(arcdata, ptVertex2, ptVertex1, ptVertex3, bendRadius);
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(ptVertex2);
				vertmp->SetArcPt(0, arcdata.ptArcBegin);
				vertmp->SetArcPt(1, arcdata.ptArcMid);
				vertmp->SetArcPt(2, arcdata.ptArcEnd);
				vertmp->SetRadius(bendRadius);
				vertmp->SetCenter(arcdata.ptArcCenter);
			}

		}
	}

	return true;
}

bool PIT::PITRebarCurve::makeStraightRebarCurve(vector<CPoint3D> const& vecRebarVertex)
{
	if (vecRebarVertex.size() < 2)
		return false;

	DPoint3d ptVertex1;
	// DPoint3d ptVertex2;
	// DPoint3d ptVertex3;
	for (int i = 0; i < vecRebarVertex.size(); i++)
	{
		RebarVertex*   vertmp;
		vertmp = &PopVertices().NewElement();
		if (i == 0)
		{
			ptVertex1 = vecRebarVertex[i];
			vertmp->SetType(RebarVertex::kStart);
			vertmp->SetIP(ptVertex1);
		}
		else if (i == vecRebarVertex.size() - 1)
		{

			vertmp->SetType(RebarVertex::kEnd);
			vertmp->SetIP(vecRebarVertex[i]);
		}
	}

	return true;
}

bool PIT::PITRebarCurve::makeRebarCurve
(
	double                  bendRadius,
	double                  bendLen,
	RebarEndTypes const&    endTypes,
	CPoint3D const&        ptstr,
	CPoint3D const&        ptend
)
{
	RebarVertexP vex;
	vex = &PopVertices().NewElement();
	vex->SetIP(ptstr);
	vex->SetType(RebarVertex::kStart);


	vex = &PopVertices().NewElement();
	vex->SetIP(ptend);
	vex->SetType(RebarVertex::kEnd);

	CVector3D   endNormal(-1.0, 0.0, 0.0);
	__super::EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, endNormal);
	//rebar.DoMatrix(mat);
	return true;
}

bool PITRebarCurve::makeRebarCurveWithNormal
(
	double                  bendRadius,
	double                  bendLen,
	RebarEndTypes const&    endTypes,
	CPoint3D const&        ptstr,
	CPoint3D const&        ptend,
	CVector3D   endNormal,
	CMatrix3D const&        mat
)
{
	RebarVertexP vex;
	vex = &PopVertices().NewElement();
	vex->SetIP(ptstr);
	vex->SetType(RebarVertex::kStart);


	vex = &PopVertices().NewElement();
	vex->SetIP(ptend);
	vex->SetType(RebarVertex::kEnd);

	__super::EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, endNormal);
	DoMatrix(mat);
	return true;
}


bool PITRebarCurve::makeURebarWithNormal(vector<CPoint3D>& vecRebarVertex, 
	double                  bendRadius,
	double                  bendLen,
	RebarEndTypes const&    endTypes,
	CVector3D   endNormal,
	CMatrix3D const&     mat)
{
	if (vecRebarVertex.size() < 3)
		return false;

	DPoint3d ptVertex1;
	DPoint3d ptVertex2;
	DPoint3d ptVertex3;
	for (int i = 0; i < vecRebarVertex.size(); i++)
	{
		RebarVertex*   vertmp;
		vertmp = &PopVertices().NewElement();;
		if (i == 0)
		{
			ptVertex1 = vecRebarVertex[i];
			vertmp->SetType(RebarVertex::kStart);
			vertmp->SetIP(ptVertex1);
		}
		else if (i == vecRebarVertex.size() - 1)
		{

			vertmp->SetType(RebarVertex::kEnd);
			vertmp->SetIP(vecRebarVertex[i]);
		}
		else
		{

			ptVertex1 = vecRebarVertex[i - 1];
			ptVertex2 = vecRebarVertex[i];
			ptVertex3 = vecRebarVertex[i + 1];
			DPoint3d vec1 = ptVertex1 - ptVertex2;
			DPoint3d vec2 = ptVertex3 - ptVertex2;
			vec1.Normalize();
			vec2.Normalize();
			if (COMPARE_VALUES_EPS(vec1.DotProduct(vec2), 1.0, 0.1) == 0)//如果平行时
			{
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(vecRebarVertex[i]);
			}
			else
			{
				RebarHelper::RebarArcData arcdata;
				RebarHelper::IntersectionPointToArcDataRebar(arcdata, ptVertex2, ptVertex1, ptVertex3, bendRadius);
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(ptVertex2);
				vertmp->SetArcPt(0, arcdata.ptArcBegin);
				vertmp->SetArcPt(1, arcdata.ptArcMid);
				vertmp->SetArcPt(2, arcdata.ptArcEnd);
				vertmp->SetRadius(bendRadius);
				vertmp->SetCenter(arcdata.ptArcCenter);
			}

		}
	}

	__super::EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, endNormal);
	DoMatrix(mat);


	return true;
}
bool PITRebarCurve::makeStirrupURebarWithNormal(vector<CPoint3D>& vecRebarVertex,
	double                  bendRadius,
	double                  bendLen,
	RebarEndTypes const&    endTypes,
	CVector3D   endNormal)
{
	if (vecRebarVertex.size() < 5)
		return false;

	DPoint3d vec1 = vecRebarVertex[1] - vecRebarVertex[0];
	DPoint3d vec2 = vecRebarVertex[2] - vecRebarVertex[1];
	vec1.Normalize();
	vec2.Normalize();
	vec1.CrossProduct(vec1, vec2);
	endNormal = vec1;


	DPoint3d ptVertex1;
	DPoint3d ptVertex2;
	DPoint3d ptVertex3;
	for (int i = 0; i < vecRebarVertex.size(); i++)
	{
		RebarVertex*   vertmp;
		vertmp = &PopVertices().NewElement();;
		if (i == 0)
		{
			ptVertex1 = vecRebarVertex[i];
			vertmp->SetType(RebarVertex::kStart);
			vertmp->SetIP(ptVertex1);
		}
		else if (i == vecRebarVertex.size() - 1)
		{

			vertmp->SetType(RebarVertex::kEnd);
			vertmp->SetIP(vecRebarVertex[i]);
		}
		else
		{

			ptVertex1 = vecRebarVertex[i - 1];
			ptVertex2 = vecRebarVertex[i];
			ptVertex3 = vecRebarVertex[i + 1];
			DPoint3d vec1 = ptVertex1 - ptVertex2;
			DPoint3d vec2 = ptVertex3 - ptVertex2;
			vec1.Normalize();
			vec2.Normalize();
			if (COMPARE_VALUES_EPS(vec1.DotProduct(vec2), 1.0, 0.1) == 0)//如果平行时
			{
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(vecRebarVertex[i]);
			}
			else
			{
				RebarHelper::RebarArcData arcdata;
				RebarHelper::IntersectionPointToArcDataRebar(arcdata, ptVertex2, ptVertex1, ptVertex3, bendRadius);
				vertmp->SetType(RebarVertex::kIP);
				vertmp->SetIP(ptVertex2);
				vertmp->SetArcPt(0, arcdata.ptArcBegin);
				vertmp->SetArcPt(1, arcdata.ptArcMid);
				vertmp->SetArcPt(2, arcdata.ptArcEnd);
				vertmp->SetRadius(bendRadius);
				vertmp->SetCenter(arcdata.ptArcCenter);
			}

		}
	}

	__super::EvaluateEndTypes(endTypes, bendRadius, bendLen/* + bendRadius*/, endNormal);
	//DoMatrix(mat);


	return true;
}
bool PIT::PITRebarCurve::GetMaxLenLine(RebarCurveCR rebarCurve, LineSegment &lineMax)
{
	bvector<DPoint3d> ips;
	rebarCurve.GetIps(ips);

	if (ips.empty())
	{
		return false;
	}
	double dis = 0;
	for (int i = 0; i < (int)ips.size() - 1; ++i)
	{
		if (ips[i].Distance(ips[i + 1]) > dis)
		{
			lineMax.SetLineSeg(DSegment3d::From(ips[i], ips[i + 1]));
			dis = lineMax.GetLength();
		}
	}
	return true;
}
