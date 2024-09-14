#include "_ustation.h"
#include "RebarDetailElement.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "resource.h"
#include "ExtractFacesTool.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "XmlHelper.h"
#include "CBeamRebarAssembly.h"
#include "CBeamRebarMainDlg.h"

CBeamRebarAssembly::CBeamRebarAssembly(ElementId id, DgnModelRefP modelRef) :
	RebarAssembly(id, modelRef)
{
	m_pBeamRebarMainDlg = NULL;
}

CBeamRebarAssembly::~CBeamRebarAssembly()
{
	if (m_pBeamRebarMainDlg)
	{
		delete m_pBeamRebarMainDlg;
		m_pBeamRebarMainDlg = nullptr;
	}
}

void CBeamRebarAssembly::CalculateTransform(CVector3D& transform, BrStringCR sizeKey, BeamRebarInfo::BeamAreaVertical stBeamAreaVertical, DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素

	double dHigth = m_BeamInfo.height;
	double dWidth = m_BeamInfo.width;
	double dLength = m_BeamInfo.length;

	double dTopCover = m_stDefaultInfo.dTop * uor_per_mm; // 上方保护层
	double dUnderCover = m_stDefaultInfo.dUnder * uor_per_mm; // 下方保护层
	double dLeftCover = m_stDefaultInfo.dLeft * uor_per_mm; // 左面保护层
	double dRightCover = m_stDefaultInfo.dRight * uor_per_mm; // 右面保护层
	double dSpace = 0.00;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef); // 钢筋直径

	CVector3D	zTrans(0.0, 0.0, 0.0);

	dSpace = stBeamAreaVertical.dSpace * uor_per_mm;

	if (stBeamAreaVertical.nPosition == 0) // 上方
	{
		zTrans.z = dHigth - dTopCover - diameter * 0.5 - dSpace;
		zTrans.x = dLength / 2;
		zTrans.y = diameter * 0.5;
	}
	else if (stBeamAreaVertical.nPosition == 1) // 下方
	{
		zTrans.z = dUnderCover + diameter * 0.5 + dSpace;
		zTrans.x = dLength / 2;
		zTrans.y = diameter * 0.5;
	}
	else if (stBeamAreaVertical.nPosition == 2) // 左侧
	{
		zTrans.z = diameter * 0.5;
		zTrans.x = dLength / 2;
		zTrans.y = dWidth - dLeftCover - diameter * 0.5 - dSpace;
	}
	else if (stBeamAreaVertical.nPosition == 3) // 右侧
	{
		zTrans.z = diameter * 0.5;
		zTrans.x = dLength / 2;
		zTrans.y = diameter * 0.5 + dRightCover + dSpace;
	}
	transform = zTrans;
}

bool CBeamRebarAssembly::GetRectFromHeight(double& width, double& depth, double height, double cover)
{
	depth = GetDepth() - cover * 2;;

	double capWidth = GetCapWidth() - cover * 2;
	double capHeight = GetCapHeight();
	double footHeight = GetFootHeight();
	double footWidth = GetFootWidth() - cover * 2;

	//line 1
	DPoint3d startL1 = DPoint3d::From(capWidth / 2, GetHeight(), 0);
	DPoint3d endL1 = DPoint3d::From(capWidth / 2, GetFootHeight(), 0);

	//arc
	DPoint3d startArc = DPoint3d::From(GetFootWidth() / 2, GetFootHeight(), 0);
	DPoint3d endArc = DPoint3d::From(GetCapWidth() / 2, GetHeight() - GetCapHeight(), 0);
	DEllipse3d offsetArc;
	offsetArc.InitArcFromPointTangentPoint(startArc, DVec3d::From(0.0, 1.0, 0.0), endArc);
	offsetArc = DEllipse3d::FromScaledVectors(offsetArc, cover / offsetArc.vector0.Magnitude() + 1);

	DPoint3d localCenter[2];
	double   lineFraction[2];
	double   ellipseAngle[2];
	DPoint3d cosineSineZ[2];

	int numIntersection = offsetArc.IntersectXYLine(localCenter, lineFraction, cosineSineZ, ellipseAngle,
		startL1, endL1);
	capHeight = lineFraction[0] * (GetHeight() - GetFootHeight());

	// < 0 && >= -GetCapHeight()
	if (height < 0 && height >= -capHeight)
	{
		width = capWidth;

		return true;
	}
	// < -GetCapHeight() && > GetFootHeight() - GetHeight()
	else if (height < -capHeight && height > footHeight - GetHeight())
	{
		double yHeight = GetHeight() - footHeight - capHeight;
		double pHeight = GetHeight() - footHeight + height;

		//line
		DPoint3d start = DPoint3d::From(0.0, pHeight, 0);
		DPoint3d end = DPoint3d::From(capWidth / 2, pHeight, 0);

		//arc
		DPoint3d startArc = DPoint3d::From(footWidth / 2, 0, 0);
		DPoint3d endArc = DPoint3d::From(capWidth / 2, yHeight, 0);


		DEllipse3d arc;
		arc.InitArcFromPointTangentPoint(startArc, DVec3d::From(0.0, 1.0, 0.0), endArc);

		int numIntersection = arc.IntersectXYLine(localCenter, lineFraction, cosineSineZ, ellipseAngle,
			start, end);

		width = lineFraction[0] * capWidth;

		return true;
	}
	// <= GetFootHeight() - GetHeight() && > -GetHeight()
	else if (height <= footHeight - GetHeight() && height > -GetHeight())
	{
		width = footWidth;

		return true;
	}

	return false;
}

bool CBeamRebarAssembly::GetHeightFromWidth(double& height, double width, double diameter) const
{
	double capWidth = GetCapWidth() - 2.0 * GetCover() - diameter;
	double footWidth = GetFootWidth() - 2.0 * GetCover() - diameter;
	double coverOffset = GetCover() + diameter / 2.0;

	DSegment3d  capLine2d;
	capLine2d.point[0] = DPoint3d::From(width, 0.0, 0.0);
	capLine2d.point[1] = DPoint3d::From(width, -GetHeight(), 0.0);

	if (DoubleCompare(width, footWidth / 2.0) < 0 && DoubleCompare(width, -footWidth / 2.0) > 0)
	{
		height = -(GetHeight() - GetCover());
		return true;
	}
	else if (DoubleCompare(width, footWidth / 2.0) > 0 && DoubleCompare(width, capWidth / 2.0) < 0)
	{
		DPoint3d    arcStartPt2d = DPoint3d::From(GetFootWidth() / 2.0, -(GetHeight() - GetFootHeight()), 0.0);
		DPoint3d    arcEndPt2d = DPoint3d::From(GetCapWidth() / 2.0, -GetCapHeight(), 0.0);

		DEllipse3d  vaseArc2d;
		vaseArc2d.InitArcFromPointTangentPoint(arcStartPt2d, DVec3d::From(0.0, 1.0, 0.0), arcEndPt2d);
		vaseArc2d = DEllipse3d::FromScaledVectors(vaseArc2d, coverOffset / vaseArc2d.vector0.Magnitude() + 1.0);

		DPoint3d    intersectPt2d[2];
		double      lineParams[2];
		DPoint3d    ellipseCoffs2d[2];
		double      ellipseAngle[2];
		int         nIntersections = vaseArc2d.IntersectXYLine(intersectPt2d, lineParams, ellipseCoffs2d, ellipseAngle, capLine2d.point[0], capLine2d.point[1]);
		if (nIntersections > 0)
		{
			height = intersectPt2d[0].y;
			return true;
		}
	}
	else if (DoubleCompare(width, -footWidth / 2.0) < 0 && DoubleCompare(width, -capWidth / 2.0) > 0)
	{
		DPoint3d    arcStartPt2d = DPoint3d::From(-GetFootWidth() / 2.0, -(GetHeight() - GetFootHeight()), 0.0);
		DPoint3d    arcEndPt2d = DPoint3d::From(-GetCapWidth() / 2.0, -GetCapHeight(), 0.0);

		DEllipse3d  vaseArc2d;
		vaseArc2d.InitArcFromPointTangentPoint(arcStartPt2d, DVec3d::From(0.0, 1.0, 0.0), arcEndPt2d);
		vaseArc2d = DEllipse3d::FromScaledVectors(vaseArc2d, coverOffset / vaseArc2d.vector0.Magnitude() + 1.0);

		DPoint3d    intersectPt2d[2];
		double      lineParams[2];
		DPoint3d    ellipseCoffs2d[2];
		double      ellipseAngle[2];
		int         nIntersections = vaseArc2d.IntersectXYLine(intersectPt2d, lineParams, ellipseCoffs2d, ellipseAngle, capLine2d.point[0], capLine2d.point[1]);
		if (nIntersections > 0)
		{
			height = intersectPt2d[0].y;
			return true;
		}
	}
	assert(false);
	return false;
}

int CBeamRebarAssembly::MakeStirrup(RebarCurve &rebar, double z, double width, double depth, double cover, double diameter, double bendRadius, double bendLen, const RebarEndTypes &endTypes) const
{
	CPoint3D wid(width, depth, 0.0);
	CPoint3D off(diameter / 2.0, diameter / 2.0, 0.0);

	rebar.CalculateRectangular(wid, off, bendRadius);

	rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius);

	CMatrix3D mat;
	CVector3D vec(0.0, cover, -z);
	mat.SetTranslation(vec);
	rebar.DoMatrix(mat);
	rebar.DoMatrix(m_Placement);

	return SUCCESS;
}

RebarSetTag *CBeamRebarAssembly::MakeStirrups(DgnModelRefP modelRef)
{
	//NewRebarAssembly(modelRef);

	RebarSetP rebar_set = RebarSet::Fetch(m_stirrup_set_id, modelRef);
	RebarSetTag *rebar_set_tag = nullptr;

	if (rebar_set != nullptr)
	{
		rebar_set->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
		rebar_set->SetCallerId(GetElementId());

		rebar_set->StartUpdate(modelRef);

		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		double cover = GetCover();
		double spacing = GetShearSpacing();
		BrString sizeKey = GetShearBarSize();
		sizeKey.Replace(L"mm", L"");

		double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
		double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, true);
		RebarEndType endType;
		switch (GetTieType())
		{
		case kBend:
			endType.SetType(RebarEndType::kBend);
			break;
		case kCog:
			endType.SetType(RebarEndType::kCog);
			break;
		case kHook:
			endType.SetType(RebarEndType::kHook);
			break;
		default:
			break;
		}
		double bendLen = RebarCode::GetBendLength(sizeKey, endType, modelRef);

		double h = GetHeight() - 2.0 * cover - diameter;

		RebarShapeData shape;
		shape.SetSizeKey((LPCTSTR)sizeKey);
		shape.SetIsStirrup(true);

		int num_stirrups = (int)floor(h / spacing + 0.5) + 1;

		RebarSetData setdata;
		setdata.SetNumber(num_stirrups);
		setdata.SetNominalSpacing(spacing / uor_per_mm);
		if (num_stirrups > 1)
			spacing = h / (num_stirrups - 1);
		setdata.SetAverageSpacing(spacing / uor_per_mm);

		double stirrup_z = cover + diameter / 2.0;
		RebarSymbology symb;
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_OTHER_REBAR);

		RebarEndTypes endTypes = { endType, endType };

		for (int i = 0; i < num_stirrups; i++)
		{
			RebarCurve rebar;
			double width, depth;
			GetRectFromHeight(width, depth, -stirrup_z, cover);

			MakeStirrup(rebar, stirrup_z, width, depth, cover, diameter, bendRadius, bendLen, endTypes);

			RebarElementP rebar_elem = rebar_set->AssignRebarElement(i, num_stirrups, symb, modelRef);
			if (rebar_elem != nullptr)
			{
				shape.SetLength(rebar.GetLength() / uor_per_mm);
				rebar_elem->Update(rebar, diameter, endTypes, shape, modelRef, false);
				//AddRebarElement(*rebar_elem, modelRef);
			}

			stirrup_z += spacing;
		}
		//int barSetTag = 0;          // this is an identifier of this RebarSet in ProConcrete RebarBeam object
									// for every different RebarSet give a unique identifier.
									// if you give the same identifier to another set, it will replace the existing one
		//AddRebarSet(*rebar_set, barSetTag, true, modelRef);        // add the RebarSet to the RebarAssembly after all its RebarElements have been assigned. Better performance

		rebar_set->FinishUpdate(setdata, modelRef);

		rebar_set_tag = new RebarSetTag;
		rebar_set_tag->SetRset(rebar_set);
		rebar_set_tag->SetIsStirrup(true);
		rebar_set_tag->SetBarSetTag(0);
	}

	return rebar_set_tag;
}

int CBeamRebarAssembly::MakeVerticalXDirBar(RebarCurve &rebar, double xPos, double height, double diameter, double bendRadius, double bendLen, RebarEndTypes const& endTypes, bool isFront) const
{
	double yBack = GetDepth() - (GetCover() + diameter / 2.0);
	double yFront = GetCover() + diameter / 2.0;
	double zTop = -GetCover() - diameter / 2.0;
	double zBottom = height + diameter / 2.0;

	CPoint3D    ipPts[2];
	if (isFront)
	{
		ipPts[0].Reset(xPos, yFront, zBottom);
		ipPts[1].Reset(xPos, yFront, zTop);
	}
	else
	{
		ipPts[0].Reset(xPos, yBack, zBottom);
		ipPts[1].Reset(xPos, yBack, zTop);
	}

	RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ipPts[0]);
	vex->SetType(RebarVertex::kStart);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ipPts[1]);
	vex->SetType(RebarVertex::kEnd);

	CVector3D   endNormal = isFront ? CVector3D(-1.0, 0.0, 0.0) : CVector3D(1.0, 0.0, 0.0);

	rebar.EvaluateBend(bendRadius);
	rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, endNormal);
	rebar.DoMatrix(m_Placement);

	return SUCCESS;
}

RebarSetTag* CBeamRebarAssembly::MakeVerticalXDirBars(ElementId& setId, bool isFront, DgnModelRefP modelRef)
{
	//NewRebarAssembly(modelRef);

	bool const isStirrup = false;
	RebarSetP rebar_set = RebarSet::Fetch(setId, modelRef);
	RebarSetTag *rebar_set_tag = nullptr;

	if (NULL != rebar_set)
	{
		rebar_set->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
		rebar_set->SetCallerId(GetCallerId());

		rebar_set->StartUpdate(modelRef);

		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		double spacing = GetLongitudinalSpacing();
		BrString sizeKey = GetLongitudinalBarSize();
		sizeKey.Replace(L"mm", L"");

		double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
		double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);

		RebarEndType endType;
		switch (GetTieType())
		{
		case kBend:
			endType.SetType(RebarEndType::kBend);
			break;
		case kCog:
			endType.SetType(RebarEndType::kCog);
			break;
		case kHook:
			endType.SetType(RebarEndType::kHook);
			break;
		default:
			break;
		}
		RebarEndTypes endTypes = { endType, endType };
		double bendLen = RebarCode::GetBendLength(sizeKey, endType, modelRef);

		RebarShapeData shape;
		shape.SetSizeKey((LPCTSTR)sizeKey);
		shape.SetIsStirrup(isStirrup);

		double width = GetCapWidth() - 2.0 * spacing - 2.0 * GetCover() - diameter;
		int num_rebars = (int)floor(width / spacing + 0.5) + 1;

		RebarSetData setdata;
		setdata.SetNumber(num_rebars);
		setdata.SetNominalSpacing(spacing / uor_per_mm);

		if (num_rebars > 1)
			spacing = width / (num_rebars - 1);
		setdata.SetAverageSpacing(spacing / uor_per_mm);

		RebarSymbology symb;
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);

		double xPos = -width / 2.0;
		for (int i = 0; i < num_rebars; i++)
		{
			double height;
			RebarCurve rebar;
			GetHeightFromWidth(height, xPos, diameter);
			MakeVerticalXDirBar(rebar, xPos, height, diameter, bendRadius, bendLen, endTypes, isFront);

			RebarElementP rebar_elem = rebar_set->AssignRebarElement(i, num_rebars, symb, modelRef);
			if (NULL != rebar_elem)
			{
				shape.SetLength(rebar.GetLength() / uor_per_mm);
				rebar_elem->Update(rebar, diameter, endTypes, shape, modelRef, false);
			}

			xPos += spacing;
		}

		int barSetTag = isFront ? 1 : 2;
		//AddRebarSet(*rebar_set, barSetTag, isStirrup, modelRef);

		rebar_set->FinishUpdate(setdata, modelRef);

		rebar_set_tag = new RebarSetTag;
		rebar_set_tag->SetRset(rebar_set);
		rebar_set_tag->SetIsStirrup(isStirrup);
		rebar_set_tag->SetBarSetTag(barSetTag);
	}
	return rebar_set_tag;
}

bool CBeamRebarAssembly::GetVerticalYDirPoints(bvector<BePoint3D>& points, double& vaseRadius, double yPos, double barDiameter, double bendRadius) const
{
	double      capWidth = GetCapWidth() - 2.0 * GetCover() - barDiameter;
	double      footWidth = GetFootWidth() - 2.0 * GetCover() - barDiameter;
	double      coverOffset = GetCover() + barDiameter / 2.0;

	DSegment3d  capLine2d;
	capLine2d.point[0] = DPoint3d::From(capWidth / 2.0, 0.0, 0.0);
	capLine2d.point[1] = DPoint3d::From(capWidth / 2.0, -GetHeight(), 0.0);

	DSegment3d  footLine2d;
	footLine2d.point[0] = DPoint3d::From(footWidth / 2.0, 0.0, 0.0);
	footLine2d.point[1] = DPoint3d::From(footWidth / 2.0, -GetHeight(), 0.0);

	DEllipse3d  vaseArc2d;
	DPoint3d    arcStartPt2d = DPoint3d::From(GetFootWidth() / 2.0, -(GetHeight() - GetFootHeight()), 0.0);
	DPoint3d    arcEndPt2d = DPoint3d::From(GetCapWidth() / 2.0, -GetCapHeight(), 0.0);
	vaseArc2d.InitArcFromPointTangentPoint(arcStartPt2d, DVec3d::From(0.0, 1.0, 0.0), arcEndPt2d);
	vaseArc2d = DEllipse3d::FromScaledVectors(vaseArc2d, coverOffset / vaseArc2d.vector0.Magnitude() + 1.0);

	DEllipse3d  offsetVaseArc2d = DEllipse3d::FromScaledVectors(vaseArc2d, bendRadius / vaseArc2d.vector0.Magnitude() + 1.0);
	DSegment3d  offsetCapLine2d = DSegment3d::From(capLine2d.point[0], capLine2d.point[1]);
	offsetCapLine2d.point[0].x -= bendRadius;
	offsetCapLine2d.point[1].x -= bendRadius;

	DPoint3d    intersectPts2d[2];
	double      lineParams[2];
	DPoint3d    ellipseCoffs2d[2];
	double      ellipseAngle[2];
	int         nIntersections = offsetVaseArc2d.IntersectXYLine(intersectPts2d, lineParams, ellipseCoffs2d, ellipseAngle, offsetCapLine2d.point[0], offsetCapLine2d.point[1]);
	if (nIntersections <= 0)
	{
		assert(false);
		return false;
	}
	DPoint3d    bendArcCenter2d = intersectPts2d[0];

	nIntersections = vaseArc2d.IntersectXYLine(intersectPts2d, lineParams, ellipseCoffs2d, ellipseAngle, bendArcCenter2d, vaseArc2d.center);
	if (nIntersections <= 0)
	{
		assert(false);
		return false;
	}

	DPoint3d    vaseArcPt2d[3];
	vaseArcPt2d[2] = intersectPts2d[0];
	vaseArcPt2d[0] = vaseArc2d.FractionToPoint(0.0);
	vaseArc2d.InitArcFromPointTangentPoint(vaseArcPt2d[0], DVec3d::From(0.0, 1.0, 0.0), vaseArcPt2d[2]);
	vaseArcPt2d[1] = vaseArc2d.FractionToPoint(0.5);

	DPoint3d    bendArcPt2d[3];
	bendArcPt2d[0] = vaseArcPt2d[2];
	bendArcPt2d[2] = DPoint3d::From(capWidth / 2.0, bendArcCenter2d.y, 0.0);

	DEllipse3d  bendArc2d;
	bendArc2d.InitArcFromPointTangentPoint(bendArcPt2d[2], DVec3d::From(0.0, -1.0, 0.0), bendArcPt2d[0]);
	bendArcPt2d[1] = bendArc2d.FractionToPoint(0.5);

	DSegment3d  tanentLine2d;
	DVec3d      tangent2d = DVec3d::FromStartEnd(bendArc2d.center, vaseArc2d.center);
	tangent2d.Normalize();
	tangent2d.RotateXY(PI / 2.0);
	tanentLine2d.InitFromOriginAndDirection(vaseArcPt2d[2], tangent2d);

	DPoint3d    vaseIpPt2d, bendIpPt2d;
	if (!DSegment3d::IntersectXY(lineParams[0], lineParams[1], vaseIpPt2d, intersectPts2d[0], footLine2d, tanentLine2d) ||
		!DSegment3d::IntersectXY(lineParams[0], lineParams[1], bendIpPt2d, intersectPts2d[0], capLine2d, tanentLine2d))
	{
		assert(false);
		return false;
	}

	CPoint3D    ipPts[4];
	ipPts[0].Reset(footWidth / 2.0, yPos, -(GetHeight() - GetCover() - barDiameter / 2.0));
	ipPts[1].Reset(vaseIpPt2d.x, yPos, vaseIpPt2d.y);
	ipPts[2].Reset(bendIpPt2d.x, yPos, bendIpPt2d.y);
	ipPts[3].Reset(capWidth / 2.0, yPos, -(GetCover() + barDiameter / 2.0));

	points.push_back(ipPts[0]);
	points.push_back(ipPts[1]);
	points.push_back(ipPts[2]);
	points.push_back(ipPts[3]);

	vaseRadius = vaseArc2d.vector0.Magnitude();

	return true;
}

int CBeamRebarAssembly::MakeVerticalYDirBar(RebarCurve &rebar, double yPos, double barDiameter, double bendRadius, double bendLen, RebarEndTypes const& endTypes, bool isRight) const
{
	double              vaseRadius = 0.0;
	bvector<BePoint3D>  ipPts;
	if (!GetVerticalYDirPoints(ipPts, vaseRadius, yPos, barDiameter, bendRadius) || ipPts.size() < 4)
		return ERROR;

	if (!isRight)
	{
		ipPts[0].x *= -1.0;
		ipPts[1].x *= -1.0;
		ipPts[2].x *= -1.0;
		ipPts[3].x *= -1.0;
	}

	RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ipPts[0]);
	vex->SetType(RebarVertex::kStart);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ipPts[1]);
	vex->SetRadius(vaseRadius);
	vex->SetType(RebarVertex::kIP);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ipPts[2]);
	vex->SetRadius(bendRadius);
	vex->SetType(RebarVertex::kIP);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ipPts[3]);
	vex->SetType(RebarVertex::kEnd);

	rebar.PopVertices()[1]->EvaluateBend(*rebar.PopVertices()[0], *rebar.PopVertices()[2]);
	rebar.PopVertices()[2]->EvaluateBend(*rebar.PopVertices()[1], *rebar.PopVertices()[3]);

	CVector3D   endNormal = isRight ? CVector3D(0.0, -1.0, 0.0) : CVector3D(0.0, 1.0, 0.0);
	rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, endNormal);

	rebar.DoMatrix(m_Placement);
	return SUCCESS;
}

RebarSetTag* CBeamRebarAssembly::MakeVerticalYDirBars(ElementId& setId, bool isRight, DgnModelRefP modelRef)
{
	//NewRebarAssembly(modelRef);           // ypu only need this once

	bool const isStirrup = false;
	RebarSetP rebar_set = RebarSet::Fetch(setId, modelRef);
	RebarSetTag *rebar_set_tag = nullptr;

	if (NULL != rebar_set)
	{
		rebar_set->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
		rebar_set->SetCallerId(GetCallerId());

		rebar_set->StartUpdate(modelRef);

		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		double spacing = GetLongitudinalSpacing();
		BrString sizeKey = GetLongitudinalBarSize();
		sizeKey.Replace(L"mm", L"");

		double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
		double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);

		RebarEndType endType;
		switch (GetTieType())
		{
		case kBend:
			endType.SetType(RebarEndType::kBend);
			break;
		case kCog:
			endType.SetType(RebarEndType::kCog);
			break;
		case kHook:
			endType.SetType(RebarEndType::kHook);
			break;
		default:
			break;
		}
		RebarEndTypes endTypes = { endType, endType };
		double bendLen = RebarCode::GetBendLength(sizeKey, endType, modelRef);

		RebarShapeData shape;
		shape.SetSizeKey((LPCTSTR)sizeKey);
		shape.SetIsStirrup(isStirrup);

		double depth = GetDepth() - 2.0 * GetCover() - diameter;
		int num_rebars = (int)floor(depth / spacing + 0.5) + 1;

		RebarSetData setdata;
		setdata.SetNumber(num_rebars);
		setdata.SetNominalSpacing(spacing / uor_per_mm);

		if (num_rebars > 1)
			spacing = depth / (num_rebars - 1);
		setdata.SetAverageSpacing(spacing / uor_per_mm);

		RebarSymbology symb;
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);

		double yPos = GetCover() + diameter / 2.0;
		for (int i = 0; i < num_rebars; i++)
		{
			RebarCurve rebar;
			MakeVerticalYDirBar(rebar, yPos, diameter, bendRadius, bendLen, endTypes, isRight);

			RebarElementP rebar_elem = rebar_set->AssignRebarElement(i, num_rebars, symb, modelRef);
			if (NULL != rebar_elem)
			{
				shape.SetLength(rebar.GetLength() / uor_per_mm);
				rebar_elem->Update(rebar, diameter, endTypes, shape, modelRef, false);
			}

			yPos += spacing;
		}

		int barSetTag = isRight ? 3 : 4;
		//AddRebarSet(*rebar_set, barSetTag, isStirrup, modelRef);

		rebar_set->FinishUpdate(setdata, modelRef);

		rebar_set_tag = new RebarSetTag;
		rebar_set_tag->SetRset(rebar_set);
		rebar_set_tag->SetIsStirrup(isStirrup);
		rebar_set_tag->SetBarSetTag(barSetTag);
	}
	return rebar_set_tag;
}

int CBeamRebarAssembly::MakeRebarsTmp(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);     // you need to do this once at the start of creating/modifying the RebarAssembly
	RebarSetTagArray rset_tags;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	for (size_t i = 0; i < m_vecBeamCommHoop.size(); i++)
	{
		SetCover(m_stDefaultInfo.dMargin * uor_per_mm);
		SetShearSpacing(m_vecBeamCommHoop[i].dSpacing * uor_per_mm);
		SetCapWidth(m_stBeamBaseData.dWidth * uor_per_mm);
		SetHeight(m_stBeamBaseData.dDepth * uor_per_mm);
		SetDepth(m_stBeamBaseData.dAxisToAxis * uor_per_mm);
		SetShearBarSize(m_vecBeamCommHoop[i].rebarSize);

		RebarSetTag *rebar_set_tag = MakeStirrups(modelRef);
		if (rebar_set_tag != nullptr)
		{
			rset_tags.Add(rebar_set_tag);
		}
		SetLongitudinalSpacing(m_vecBeamCommHoop[i].dSpacing * uor_per_mm);
		SetLongitudinalBarSize(m_vecBeamCommHoop[i].rebarSize);
		rebar_set_tag = MakeVerticalXDirBars(m_front_set_id, true, modelRef);
		if (rebar_set_tag != nullptr)
		{
			rset_tags.Add(rebar_set_tag);
		}

		rebar_set_tag = MakeVerticalXDirBars(m_back_set_id, false, modelRef);
		if (rebar_set_tag != nullptr)
		{
			rset_tags.Add(rebar_set_tag);
		}

		rebar_set_tag = MakeVerticalYDirBars(m_right_set_id, true, modelRef);
		if (rebar_set_tag != nullptr)
		{
			rset_tags.Add(rebar_set_tag);
		}

		rebar_set_tag = MakeVerticalYDirBars(m_left_set_id, false, modelRef);
		if (rebar_set_tag != nullptr)
		{
			rset_tags.Add(rebar_set_tag);
		}
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		AddRebarSets(rset_tags, modelRef);
	}

	return SUCCESS;
}

bool CBeamRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素

	RebarSetTag* tag = NULL;

	CMatrix3D const rot90_x = CMatrix3D::Rotate(PI * 1.5, CVector3D::kXaxis); // 顺时针绕X轴转270度

	CMatrix3D const rot90_y = CMatrix3D::Rotate(PI * 0.5, CVector3D::kYaxis); // 顺时针绕Y轴转90度

	if (m_vecBeamRebarVertical.size() != m_vecBeamAreaVertical.size())
	{
		return false;
	}

	m_vecSetId.resize((int)m_vecBeamAreaVertical.size() + (int)m_vecBeamCommHoop.size());
	for (size_t i = 0; i < m_vecSetId.size(); i++)
	{
		m_vecSetId[i] = 0;
	}

	int nRebarSetId = 0;
	m_vecPointHoop.clear();
	for (int i = 0; i < (int)m_vecBeamAreaVertical.size(); i++)
	{
		CMatrix3D   mat;
		if (m_vecBeamAreaVertical[i].nPosition == 0 || m_vecBeamAreaVertical[i].nPosition == 1)
		{
			mat = rot90_x;
		}

		BrString sizeKey = m_vecBeamRebarVertical[i].rebarSize;
		sizeKey.Replace(L"mm", L"");

		CVector3D	transform(0.0, 0.0, 0.0);
		CalculateTransform(transform, sizeKey, m_vecBeamAreaVertical[i], modelRef);

		mat.SetTranslation(transform);
		mat = GetPlacement() * mat;

		EndType strEndType;
		EndType endEndType;
		strEndType.endType = m_vecBeamRebarVertical[i].nLeftEndStyle;
		strEndType.rotateAngle = m_vecBeamRebarVertical[i].dLeftRotateAngle;

		endEndType.endType = m_vecBeamRebarVertical[i].nRightEndStyle;
		endEndType.rotateAngle = m_vecBeamRebarVertical[i].dRightRotateAngle;

		vector<PIT::EndType> vecEndtype;
		vecEndtype.push_back(strEndType);
		vecEndtype.push_back(endEndType);

		tag = MakeRebars(m_vecBeamAreaVertical[i], m_vecBeamRebarVertical[i], vecEndtype, mat, m_vecSetId[i], sizeKey, modelRef);

		if (NULL != tag)
		{
			nRebarSetId = i + 1;
			tag->SetBarSetTag(i + 1);
			rsetTags.Add(tag);
		}
	}

	for (size_t i = 0; i < m_vecBeamCommHoop.size(); i++)
	{
		CMatrix3D   mat;
		mat = rot90_y;
		BrString sizeKey = m_vecBeamCommHoop[i].rebarSize;
		sizeKey.Replace(L"mm", L"");
		CVector3D	transform(0.0, 0.0, 0.0);
		// mat.SetTranslation(transform);
		mat = GetPlacement() * mat;

		EndType strEndType;
		EndType endEndType;
		strEndType.endType = m_vecBeamRebarHoop[i].nStartEndType;
		strEndType.rotateAngle = m_vecBeamRebarHoop[i].dStartRotate;

		endEndType.endType = m_vecBeamRebarHoop[i].nFnishEndType;
		endEndType.rotateAngle = m_vecBeamRebarHoop[i].dEndRotate;

		vector<PIT::EndType> vecEndtype;
		vecEndtype.push_back(strEndType);
		vecEndtype.push_back(endEndType);

		tag = MakeRebars(m_vecBeamCommHoop[i], m_vecBeamRebarHoop[i], vecEndtype, mat, m_vecSetId[nRebarSetId], sizeKey, modelRef);

		if (NULL != tag)
		{
			nRebarSetId++;
			tag->SetBarSetTag(nRebarSetId);
			rsetTags.Add(tag);
		}

	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

RebarSetTag* CBeamRebarAssembly::MakeRebars
(
	BeamRebarInfo::BeamCommHoop& stBeamCommHoop,
	BeamRebarInfo::BeamRebarHoop& stBeamRebarHoop,
	vector<PIT::EndType> vecEndtype,
	CMatrix3D const&    mat,
	ElementId& rebarSetId,
	BrStringCR sizeKey,
	DgnModelRefP modelRef
)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSpacing = stBeamCommHoop.dSpacing * uor_per_mm;
	if (COMPARE_VALUES(dSpacing, 0.00) <= 0)
	{
		return NULL;
	}

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	double startbendLen, endbendLen;
	RebarEndType endTypeStart, endTypeEnd;

	double startbendRadius, endbendRadius;
	double begStraightAnchorLen = 0.00;
	double endStraightAnchorLen = 0.00;

	switch (vecEndtype[0].endType)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeStart.SetType(RebarEndType::kLap);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
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

	switch (vecEndtype[1].endType)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeEnd.SetType(RebarEndType::kLap);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		endStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}


	bool isStirrup = false;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(vecEndtype[0].rotateAngle);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetstraightAnchorLen(begStraightAnchorLen);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(vecEndtype[1].rotateAngle);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetstraightAnchorLen(endStraightAnchorLen);

	vector<PITRebarCurve>     rebarCurvesNum;
	PITRebarEndTypes   endTypes = { start, end };

	double startOffset = stBeamCommHoop.dStartPos * uor_per_mm;
	double endOffset = stBeamCommHoop.dEndPos * uor_per_mm;

	int numRebar = (int)((m_stBeamBaseData.dAxisToAxis - startOffset - endOffset) / dSpacing);

	dSpacing = (m_stBeamBaseData.dAxisToAxis - startOffset - endOffset) / numRebar;

	double dRightCover = m_stDefaultInfo.dRight * uor_per_mm;
	double dTopCover = m_stDefaultInfo.dTop * uor_per_mm;
	double dLeftCover = m_stDefaultInfo.dLeft * uor_per_mm;
	double dUnderCover = m_stDefaultInfo.dUnder * uor_per_mm;

	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30

	double dPos = 0.00;
	for (int i = 0; i < numRebar; i++)
	{
		if (m_vecPointHoop.size() < 3)
		{
			return NULL;
		}

		DPoint3d vec = m_BeamInfo.ptEnd - m_BeamInfo.ptStart;
		vec.ScaleToLength(dPos + startOffset);
		dPos += dSpacing;

		double diameterTmp = m_vecPointHoop[0].diameter;
		CPoint3D pointTmp = m_vecPointHoop[0].ptPoint;
		vector<CPoint3D> vecPointHoop;
		CPoint3D ptTmp = CPoint3D::From(pointTmp.x, pointTmp.y - diameterTmp * 0.5 - diameter * 0.5, pointTmp.z + diameterTmp * 0.5 + diameter * 0.5);
		ptTmp.Add(vec);
		vecPointHoop.push_back(ptTmp);

	    diameterTmp = m_vecPointHoop[1].diameter;
		pointTmp = m_vecPointHoop[1].ptPoint;
		ptTmp = CPoint3D::From(pointTmp.x, pointTmp.y + diameterTmp * 0.5 + diameter * 0.5, pointTmp.z + diameterTmp * 0.5 + diameter * 0.5);
		ptTmp.Add(vec);
		vecPointHoop.push_back(ptTmp);

		if (m_vecPointHoop.size() > 3)
		{
			diameterTmp = m_vecPointHoop[3].diameter;
			pointTmp = m_vecPointHoop[3].ptPoint;
			ptTmp = CPoint3D::From(pointTmp.x, pointTmp.y + diameterTmp * 0.5 + diameter * 0.5, pointTmp.z - diameterTmp * 0.5 - diameter * 0.5);
			ptTmp.Add(vec);
			vecPointHoop.push_back(ptTmp);
		}

		diameterTmp = m_vecPointHoop[2].diameter;
		pointTmp = m_vecPointHoop[2].ptPoint;
		ptTmp = CPoint3D::From(pointTmp.x, pointTmp.y - diameterTmp * 0.5 - diameter * 0.5, pointTmp.z - diameterTmp * 0.5 - diameter * 0.5);
		ptTmp.Add(vec);
		vecPointHoop.push_back(ptTmp);

		diameterTmp = m_vecPointHoop[0].diameter;
		pointTmp = m_vecPointHoop[0].ptPoint;
		ptTmp = CPoint3D::From(pointTmp.x, pointTmp.y - diameterTmp * 0.5 - diameter * 0.5, pointTmp.z + diameterTmp * 0.5 + diameter * 0.5);
		ptTmp.Add(vec);
		vecPointHoop.push_back(ptTmp);

		PITRebarCurve     rebarCurves;
		EditElementHandle eeh;
		// Transform trans;
		// mat.AssignTo(trans);
		// TransformInfo transinfo(trans);
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(vecPointHoop[0], vecPointHoop[0]), true, *ACTIVEMODEL);
		// eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
		// eeh.AddToModel();
		if (SUCCESS == mdlElmdscr_extractEndPoints(&vecPointHoop[0], nullptr, &vecPointHoop[0], nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
		{
			makeRebarCurve(rebarCurves, endTypes, vecEndtype, vecPointHoop, bendRadius);
			rebarCurvesNum.push_back(rebarCurves);
		}
	}

	RebarSymbology symb;
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	numRebar = (int)rebarCurvesNum.size();
	int j = 0;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		// EditElementHandle eeh;
		// LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		// eeh.AddToModel();

		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypesTmp = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypesTmp, shape, modelRef, false);
		}
		j++;
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(dSpacing);
	setdata.SetAverageSpacing(dSpacing);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}


RebarSetTag* CBeamRebarAssembly::MakeRebars
(
	BeamRebarInfo::BeamAreaVertical& stBeamAreaVertical,
	BeamRebarInfo::BeamRebarVertical& stBeamRebarVertical,
	vector<PIT::EndType> vecEndtype,
	CMatrix3D const&    mat,
	ElementId& rebarSetId,
	BrStringCR sizeKey,
	DgnModelRefP modelRef
)
{
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	double startbendLen, endbendLen;
	RebarEndType endTypeStart, endTypeEnd;
	double begStraightAnchorLen = 0.00;
	double endStraightAnchorLen = 0.00;

	double startbendRadius, endbendRadius;

	switch (vecEndtype[0].endType)
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

	switch (vecEndtype[1].endType)
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
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	bool const isStirrup = false;
	int numRebar = stBeamAreaVertical.nTotNum;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);

	double dRebarLen = m_BeamInfo.length;
	double dWidth = m_BeamInfo.width;
	double dDepth = m_BeamInfo.height;
	double dLength = m_BeamInfo.length;
	double dTopCover = m_stDefaultInfo.dTop * uor_per_mm; // 上方保护层
	double dUnderCover = m_stDefaultInfo.dUnder * uor_per_mm; // 下方保护层
	double dLeftCover = m_stDefaultInfo.dLeft * uor_per_mm; // 左面保护层
	double dRightCover = m_stDefaultInfo.dRight * uor_per_mm; // 右面保护层

	double startOffset = stBeamAreaVertical.dStartOffset * uor_per_mm; // 起点偏移
	double endOffset = stBeamAreaVertical.dEndOffset * uor_per_mm; // 终点偏移

	vector<CVector3D> vecEndNormal(2);
	CVector3D	endNormal;	//端部弯钩方向

	for (unsigned int i = 0; i < 2; i++)
	{
		double dRotateAngle = vecEndtype[i].rotateAngle;
		CVector3D rebarVec = m_BeamInfo.ptEnd - m_BeamInfo.ptStart;
		endNormal = CVector3D::From(0, 0, -1);

		/*					endNormal = rebarVec.CrossProduct(vec);*/
		endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
		if (stBeamAreaVertical.nPosition == 1) // 下方
		{
			endNormal.Negate();
		}

		vecEndNormal[i] = endNormal;
	}

	vector<PITRebarCurve>     rebarCurvesNum;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(vecEndtype[0].rotateAngle);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);
	start.SetstraightAnchorLen(begStraightAnchorLen);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(vecEndtype[1].rotateAngle);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);
	end.SetstraightAnchorLen(endStraightAnchorLen);

	PITRebarEndTypes   endTypes = { start, end };

	double dPos = startOffset;
	double spacing = 0.00;				// 间距
	if (stBeamAreaVertical.nPosition == 0 || stBeamAreaVertical.nPosition == 1) // 上方 || 下方
	{
		dPos += dLeftCover;
		spacing = (dWidth - dLeftCover - dRightCover - diameter - endOffset - startOffset) / (numRebar - 1);
	}
	else if (stBeamAreaVertical.nPosition == 2 || stBeamAreaVertical.nPosition == 3) // 左侧 || 右侧
	{
		dPos += dTopCover;
		spacing = (dDepth - dTopCover - dUnderCover - diameter - endOffset - startOffset) / (numRebar - 1);
	}

	for (int i = 0; i < numRebar; i++)
	{
		CPoint3D ptstr;
		CPoint3D ptend;

		HoopPointInfo stHoopPoint;
		stHoopPoint.nPosition = stBeamAreaVertical.nPosition;

		ptstr = CPoint3D::From(-dRebarLen / 2.0, 0.0, dPos);
		ptend = CPoint3D::From(dRebarLen / 2.0, 0.0, dPos);

		dPos += spacing;

		PITRebarCurve     rebarCurves;
		EditElementHandle eeh;
		Transform trans;
		mat.AssignTo(trans);
		TransformInfo transinfo(trans);
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
		// eeh.AddToModel();
		if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
		{
			makeRebarCurve(rebarCurves, endTypes, ptstr, ptend);
			rebarCurvesNum.push_back(rebarCurves);

			if (i == 0 || i == numRebar - 1)
			{
				if (m_vecPointHoop.size() < 4)
				{
					stHoopPoint.ptPoint = ptstr;
					stHoopPoint.diameter = diameter;
					m_vecPointHoop.push_back(stHoopPoint);
				}
			}
		}
	}


	RebarSymbology symb;
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	numRebar = (int)rebarCurvesNum.size();
	int j = 0;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		// EditElementHandle eeh;
		// LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		// eeh.AddToModel();

		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypesTmp = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypesTmp, shape, modelRef, false);
		}
		j++;
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing);
	setdata.SetAverageSpacing(spacing);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;

}

bool CBeamRebarAssembly::makeRebarCurve
(
	PITRebarCurve&				rebar,
	PITRebarEndTypes&			endTypes,
	vector<PIT::EndType>&			vecEndtype,
	vector<CPoint3D>&			vecPoint,
	double						bendRadius
)
{
	if (vecPoint.size() < 2)
	{
		return false;
	}
	for (size_t i = 0; i < vecPoint.size(); i++)
	{
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(vecPoint[i]);
		if (i == 0)
		{
			vex->SetType(RebarVertex::kStart);
		}
		else if (i == vecPoint.size() - 1)
		{
			vex->SetType(RebarVertex::kEnd);
		}
		else
		{
			vex->SetRadius(bendRadius);
			vex->SetType(RebarVertex::kIP);
		}
	}

	for (int i = 1; i < rebar.PopVertices().GetSize() - 1; i++)
	{
		rebar.PopVertices()[i]->EvaluateBend(*rebar.PopVertices()[i - 1], *rebar.PopVertices()[i + 1]);
	}

	vector<CVector3D> vecEndNormal(2);
	CVector3D	endNormal;	//端部弯钩方向

	for (unsigned int i = 0; i < 2; i++)
	{
		if (i == 0)
		{
			double dRotateAngle = vecEndtype[i].rotateAngle;
			CVector3D rebarVec = vecPoint[1] - vecPoint[0];
			endNormal = CVector3D::From(0, 0, 1);
			if (COMPARE_VALUES_EPS(vecPoint[2].z, vecPoint[1].z, 10) < 0)
			{
				endNormal.Negate();
			}
			endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
			vecEndNormal[i] = endNormal;
		}
		else
		{
			double dRotateAngle = vecEndtype[i].rotateAngle;
			CVector3D rebarVec = vecPoint[vecPoint.size() - 1] - vecPoint[vecPoint.size() - 2];
			endNormal = CVector3D::From(0, 1, 0);
			endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
			vecEndNormal[i] = endNormal;
		}
	}

	endTypes.beg.SetendNormal(vecEndNormal[0]);
	endTypes.end.SetendNormal(vecEndNormal[1]);

	rebar.EvaluateEndTypesStirrup(endTypes);

	return true;
}

bool CBeamRebarAssembly::makeRebarCurve
(
	PITRebarCurve&				rebar,
	PITRebarEndTypes&			endTypes,
	CPoint3D const&				ptstr,
	CPoint3D const&				ptend
)
{
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);

	DPoint3d pt1[2];
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改

	//确保起点终点是从小到大---begin
	DVec3d vec = pt1[1] - pt1[0];
	DVec3d vecX = DVec3d::From(1, 0, 0);
	vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
	vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
	vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
	vec.Normalize();
	if (vec.IsPerpendicularTo(vecX))
	{
		if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
	}
	else
	{
		if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
	}
	//---end

	RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptstr);
	vex->SetType(RebarVertex::kStart);

	endTypes.beg.SetptOrgin(pt1[0]);
	endTypes.end.SetptOrgin(pt1[1]);


	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptend);
	vex->SetType(RebarVertex::kEnd);

	rebar.EvaluateEndTypes(endTypes);
	//rebar.DoMatrix(mat);
	return true;
}


bool CBeamRebarAssembly::CalcBeamBaseInfo(ElementHandleCR eh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	DPoint3d FrontStr, FrontEnd;
	DPoint3d BackStr, BackEnd;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	vector<DSegment3d> vecDownFontLine;
	vector<DSegment3d> vecDownBackLine;

	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &m_BeamInfo.height);

	if (vecDownFontLine.empty() || vecDownBackLine.empty())
	{
		return false;
	}

	DPoint3d pt1[2];
	vecDownFontLine[0].GetStartPoint(pt1[0]);
	vecDownFontLine[0].GetEndPoint(pt1[1]);

	DPoint3d pt2[2];
	vecDownBackLine[0].GetStartPoint(pt2[0]);
	vecDownBackLine[0].GetEndPoint(pt2[1]);

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_BeamInfo.height);

	m_BeamInfo.height = m_BeamInfo.height * uor_now / uor_ref;
	m_BeamInfo.width = FrontStr.Distance(BackStr) * uor_now / uor_ref;
	m_BeamInfo.length = FrontStr.Distance(FrontEnd) * uor_now / uor_ref;
	m_BeamInfo.ptStart = FrontStr;
	m_BeamInfo.ptEnd = FrontEnd;

	DPoint3d ptStart = m_BeamInfo.ptStart;
	DPoint3d ptEnd = m_BeamInfo.ptEnd;

	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
	//	CVector3D  yVecNegate = yVec;
	//	yVecNegate.Negate();
	//	yVecNegate.Normalize();
	//	yVecNegate.ScaleToLength(m_STwallData.width);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	//	ptStart.Add(yVecNegate);
	//	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(ptStart, ptEnd);
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}

bool CBeamRebarAssembly::IsSmartSmartFeature(EditElementHandle& eeh)
{
	SmartFeatureNodePtr pFeatureNode;
	if (SmartFeatureElement::IsSmartFeature(eeh))
	{
		return true;
	}
	else
	{
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			if (SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef()) == SUCCESS)
			{
				eeh.AddToModel();
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}

		}
		return true;
	}
}

bool CBeamRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	SetSelectedModel(ehSel.GetModelRef());
	GetConcreteXAttribute(GetConcreteOwner(), ACTIVEMODEL);

	DgnModelRefP modelRef = ACTIVEMODEL;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_pBeamRebarMainDlg = new CBeamRebarMainDlg(ehSel);
	m_pBeamRebarMainDlg->Create(IDD_DIALOG_BeamRebarMainDlg);
	m_pBeamRebarMainDlg->ShowWindow(SW_SHOW);
	m_pBeamRebarMainDlg->SetRebarAssembly(this);
	return true;
}

bool CBeamRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
	{
		return false;
	}

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
	{
		return false;
	}

	DgnModelRefP modelRef = ehWall.GetModelRef();
	// MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();
	//SetElementXAttribute(ehSel.GetElementId(), g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ehSel.GetModelRef());
	//eeh2.AddToModel();
	return true;
}

long CBeamRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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
