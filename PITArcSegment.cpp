#include "_ustation.h"
#include "PITArcSegment.h"
#include "BentlyCommonfile.h"

void PIT::ArcSegment::ScaleToRadius(double dLength)
{
	DPoint3d ptCenterStart = ptStart - ptCenter;
	ptCenterStart.Normalize();
	ptCenterStart.ScaleToLength(dLength);
	ptCenterStart.Add(ptCenter);
	ptStart = ptCenterStart;

	DPoint3d ptCenterEnd = ptEnd - ptCenter;
	ptCenterEnd.Normalize();
	ptCenterEnd.ScaleToLength(dLength);
	ptCenterEnd.Add(ptCenter);
	ptEnd = ptCenterEnd;

	DPoint3d ptCenterMid = ptMid - ptCenter;
	ptCenterMid.Normalize();
	ptCenterMid.ScaleToLength(dLength);
	ptCenterMid.Add(ptCenter);
	ptMid = ptCenterMid;

	dRadius = dLength;

	DEllipse3d seg = DEllipse3d::FromPointsOnArc(ptStart, ptMid, ptEnd);
	dLen = seg.ArcLength();
// 	EditElementHandle eeh;
// 	ArcHandler::CreateArcElement(eeh, NULL, seg, true, *ACTIVEMODEL);
// 	eeh.AddToModel();
}

void PIT::ArcSegment::Shorten(double dLength, bool start)
{
	DEllipse3d seg = DEllipse3d::FromPointsOnArc(ptStart, ptMid, ptEnd);

	ICurvePrimitivePtr curve = ICurvePrimitive::CreateArc(seg);

	CurveLocationDetail arcDetail;
	bool bAllowExtend = COMPARE_VALUES(dLength, 0) > 0 ? false : true;

	if (start)
	{
		curve->PointAtSignedDistanceFromFraction(0, dLength, bAllowExtend, arcDetail);
		ptStart = arcDetail.point;
	}
	else
	{
		curve->PointAtSignedDistanceFromFraction(0, dLen - dLength, bAllowExtend, arcDetail);
		ptEnd = arcDetail.point;
	}

	seg = DEllipse3d::FromPointsOnArc(ptStart, ptMid, ptEnd);
	dLen = seg.ArcLength();

	curve = ICurvePrimitive::CreateArc(seg);
	curve->PointAtSignedDistanceFromFraction(0, dLen * 0.5, bAllowExtend, arcDetail);
	ptMid = arcDetail.point;
}

void PIT::ArcSegment::OffsetByAxisZ(double dLength)
{
	ptStart.z += dLength;
	ptMid.z = ptStart.z;
	ptEnd.z = ptStart.z;
	ptCenter.z = ptStart.z;
}

void PIT::ArcSegment::CutArc(DPoint3d ptLineStart, DPoint3d ptLineEnd, ArcSegment * minArc)
{
	EditElementHandle eehLine;
	DSegment3d seg = DSegment3d::From(ptLineStart, ptLineEnd);
	LineHandler::CreateLineElement(eehLine, nullptr, seg, ACTIVEMODEL->Is3d(), *ACTIVEMODEL);
	EditElementHandle eehArc;
	DEllipse3d elp = DEllipse3d::FromPointsOnArc(ptStart, ptMid, ptEnd);
	ICurvePrimitivePtr arc = ICurvePrimitive::CreateArc(elp);
	DraftingElementSchema::ToElement(eehArc, *arc, nullptr, ACTIVEMODEL->Is3d(), *ACTIVEMODEL);
	DPoint3d ptIntersect;
	if (0 == mdlIntersect_allBetweenElms(&ptIntersect, NULL, 1, eehLine.GetElementDescrP(), eehArc.GetElementDescrP(), NULL, 1e-6) || ptIntersect == ptStart || ptIntersect == ptEnd)
		return;

	DEllipse3d elp1 = DEllipse3d::FromArcCenterStartEnd(ptCenter, ptStart, ptIntersect);
	DEllipse3d elp2 = DEllipse3d::FromArcCenterStartEnd(ptCenter, ptIntersect, ptEnd);

	double dLen1 = elp1.ArcLength();
	double dLen2 = elp2.ArcLength();
	DEllipse3d segMin, segMax;
	if (dLen1 > dLen2)
	{
		segMin = elp2;
		segMax = elp1;
		ptEnd = ptIntersect;
		if (minArc != NULL)
		{
			(*minArc).ptStart = ptIntersect;
		}
	}
	else
	{
		segMin = elp1;
		segMax = elp2;
		ptStart = ptIntersect;
		if (minArc != NULL)
		{
			(*minArc).ptEnd = ptIntersect;
		}
	}

	if (minArc != NULL)
	{
		(*minArc) = *this;
		(*minArc).dLen = segMin.ArcLength();
		ICurvePrimitivePtr curve = ICurvePrimitive::CreateArc(segMin);
		CurveLocationDetail arcDetail;
		curve->PointAtSignedDistanceFromFraction(0, (*minArc).dLen * 0.5, false, arcDetail);
		(*minArc).ptMid = arcDetail.point;
	}

	dLen = segMax.ArcLength();
	ICurvePrimitivePtr curve = ICurvePrimitive::CreateArc(segMax);
	CurveLocationDetail arcDetail;
	curve->PointAtSignedDistanceFromFraction(0, dLen * 0.5, false, arcDetail);
	ptMid = arcDetail.point;
}

PIT::LineSegment::LineSegment(DPoint3dCR ptStart, DPoint3dCR ptEnd)
{
	seg = DSegment3d::From(ptStart, ptEnd);
}

PIT::LineSegment::LineSegment(DSegment3dCR segIn)
{
	seg = segIn;
}

void PIT::LineSegment::Shorten(double dLength, bool start)
{
	if (COMPARE_VALUES(dLength, 0) == 0)
	{
		return;
	}
	ICurvePrimitivePtr curve = ICurvePrimitive::CreateLine(seg);
	double dLen = seg.Length();
	CurveLocationDetail arcDetail;
	bool bAllowExtend = COMPARE_VALUES(dLength, 0) > 0 ? false : true;

	if (start)
	{
		curve->PointAtSignedDistanceFromFraction(0, dLength, bAllowExtend, arcDetail);
		seg.SetStartPoint(arcDetail.point);
	}
	else
	{
		curve->PointAtSignedDistanceFromFraction(0, dLen - dLength, bAllowExtend, arcDetail);
		seg.SetEndPoint(arcDetail.point);
	}
}

void PIT::LineSegment::PerpendicularOffset(double dLength, DVec3d vec)
{
	DPoint3d &ptStart = seg.point[0];
	DPoint3d &ptEnd = seg.point[1];

	DVec3d vecStartEnd = ptEnd - ptStart;
	if ((COMPARE_VALUES(fabs(vecStartEnd.x), 5) < 0))
	{
		vecStartEnd.x = 0;
	}
	if ((COMPARE_VALUES(fabs(vecStartEnd.y), 5) < 0))
	{
		vecStartEnd.y = 0;
	}
	if ((COMPARE_VALUES(fabs(vecStartEnd.z), 5) < 0))
	{
		vecStartEnd.z = 0;
	}
	vecStartEnd.Normalize();
	double dot = vecStartEnd.DotProduct(vec);
	if (COMPARE_VALUES_EPS(dot,0,0.1/*1e-4*/) != 0)//Îó²î¸ÄÎª0.1£»
	{
		return;
	}
// 	if (!vecStartEnd.IsPerpendicularTo(vec))
// 	{
// 		return;
// 	}

	vec.ScaleToLength(dLength);
	ptStart.Add(vec);
	ptEnd.Add(vec);
}

void PIT::LineSegment::CutLine(DSegment3dCR segCut, LineSegment * minArc)
{
	EditElementHandle eehLine;
	LineHandler::CreateLineElement(eehLine, nullptr, seg, ACTIVEMODEL->Is3d(), *ACTIVEMODEL);
	
	EditElementHandle eehLineCut;
	LineHandler::CreateLineElement(eehLineCut, nullptr, segCut, ACTIVEMODEL->Is3d(), *ACTIVEMODEL);

	DPoint3d ptIntersect;
	if (0 == mdlIntersect_allBetweenElms(&ptIntersect, NULL, 1, eehLine.GetElementDescrP(), eehLineCut.GetElementDescrP(), NULL, 1e-6) || ptIntersect == segCut.point[0] || ptIntersect == segCut.point[1])
		return;

	DSegment3d line1 = DSegment3d::From(segCut.point[0], ptIntersect);
	DSegment3d line2 = DSegment3d::From(ptIntersect, segCut.point[1]);

	double dLen1 = line1.Length();
	double dLen2 = line2.Length();
	if (dLen1 > dLen2)
	{
		seg = line1;
		if (minArc != NULL)
		{
			(*minArc).seg = line2;
		}
	}
	else
	{
		seg = line2;
		if (minArc != NULL)
		{
			(*minArc).seg = line1;
		}
	}

	if (minArc != NULL)
	{
		(*minArc) = *this;
	}
}

const DVec3d PIT::LineSegment::GetLineVec() const
{
	DVec3d vec((seg.point[1] - seg.point[0]));
	if ((COMPARE_VALUES(fabs(vec.x), 5) < 0))
	{
		vec.x = 0;
	}
	if ((COMPARE_VALUES(fabs(vec.y), 5) < 0))
	{
		vec.y = 0;
	}
	if ((COMPARE_VALUES(fabs(vec.z), 5) < 0))
	{
		vec.z = 0;
	}
	vec.Normalize();
	return vec;
}

const DPoint3d PIT::LineSegment::GetCenter() const
{
	DPoint3d ptCenter;
	ptCenter.x = seg.point[0].x + seg.point[1].x;
	ptCenter.y = seg.point[0].y + seg.point[1].y;
	ptCenter.z = seg.point[0].z + seg.point[1].z;
	return ptCenter;
}

const DSegment3d PIT::LineSegment::GetLineSeg() const
{
	return seg;
}

void PIT::LineSegment::SetLineSeg(const DSegment3d & segNew)
{
	seg = segNew;
}

const double PIT::LineSegment::GetLength() const
{
	return GetLineStartPoint().Distance(GetLineEndPoint());
}

const DPoint3d PIT::LineSegment::GetLineStartPoint() const
{
	return seg.point[0];
}

void PIT::LineSegment::SetLineStartPoint(DPoint3dCR pt)
{
	seg.SetStartPoint(pt);
}

const DPoint3d PIT::LineSegment::GetLineEndPoint() const
{
	return seg.point[1];
}

void PIT::LineSegment::SetLineEndPoint(DPoint3dCR pt)
{
	seg.SetEndPoint(pt);
}

DPoint3d PIT::LineSegment::Intersect(const PIT::LineSegment& segOther)
{
	DPoint3d ptIntersect;
	mdlVec_intersect(&ptIntersect, &this->seg, &segOther.seg);
	return ptIntersect;
}

bool PIT::LineSegment::hasSamePoint(const PIT::LineSegment & segCmp)
{
	return (seg.point[0] == segCmp.GetLineStartPoint() || seg.point[0] == segCmp.GetLineStartPoint() || seg.point[1] == segCmp.GetLineStartPoint() || seg.point[1] == segCmp.GetLineStartPoint());
}

bool PIT::LineSegment::IsEqual(const PIT::LineSegment & segCmp)
{
	Dpoint3d ptstartself = this->GetLineStartPoint();
	Dpoint3d ptendself = this->GetLineEndPoint();
	Dpoint3d ptstartComp = segCmp.GetLineStartPoint();
	Dpoint3d ptendselComp = segCmp.GetLineEndPoint();
	if ((ptstartself.Distance(ptstartComp) < 10 && ptendself.Distance(ptendselComp) < 10)
		|| (ptstartself.Distance(ptendselComp) < 10 && ptendself.Distance(ptstartComp) < 10))
	{
		return true;
	}
	return false;
}

int PIT::LineSegment::compareLength(const PIT::LineSegment & segCmp)
{
	return GetLength() > segCmp.GetLength();
}

bool PIT::LineSegment::compare(const PIT::LineSegment & seg1, const PIT::LineSegment & seg2)
{
	double z1 = seg1.GetLineStartPoint().z + seg1.GetLineEndPoint().z;
	double z2 = seg2.GetLineStartPoint().z + seg2.GetLineEndPoint().z;
	if (COMPARE_VALUES(z1, z2) < 0)
	{
		return true;
	}
	else if (COMPARE_VALUES(z1, z2) == 0)
	{
		double x1 = seg1.GetLineStartPoint().x + seg1.GetLineEndPoint().x;
		double x2 = seg2.GetLineStartPoint().x + seg2.GetLineEndPoint().x;
		if (COMPARE_VALUES(z1, z2) < 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

// bool PIT::LineSegment::comparePosY(const PIT::LineSegment & segCmp)
// {
// 	double dot1 = GetLineVec().DotProduct(DVec3d::From(0, 1, 0));
// 	double dot2 = segCmp.GetLineVec().DotProduct(DVec3d::From(0, 1, 0));
// 
// // 	if (!hasSamePoint(segCmp))
// // 	{
// // 		map<int, PIT::LineSegment> sort = { make_pair((int)seg.point[0].y,seg),make_pair((int)seg.point[1].y,seg),make_pair((int)segCmp.GetLineStartPoint().y,segCmp),make_pair((int)segCmp.GetLineEndPoint().y,segCmp) };
// // 		return true;
// // 	}
// 
// 	return fabs(dot1) < fabs(dot2);
// }
