#include "_ustation.h"
#include "resource.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "ELLWallRebarAssembly.h"
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


bool ELLWallRebarAssembly::AnalyzingWallGeometricData(ElementHandleCR eh)//解析特征参数
{
	ELLWallGeometryInfo m_ELLWallDatarc;
	GetElementXAttribute(eh.GetElementId(), sizeof(m_ELLWallDatarc), m_ELLWallDatarc, RoundrebarGroup, ACTIVEMODEL);
	if (m_ELLWallDatarc.type == ELLIPSEWall)
	{
		m_ELLWallData = m_ELLWallDatarc;
		return  true;
	}

	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	// DPoint3d FrontStr, FrontEnd;
	// DPoint3d BackStr, BackEnd;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	vector<DSegment3d> vecDownFontLine;
	vector<DSegment3d> vecDownBackLine;

	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();
	m_doorsholes.clear();
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	GetDoorHoles(Holeehs, m_doorsholes);

	// 取底面 环形底面
	vector<MSElementDescrP> vecEllipse;
	EFT::GetEllipseDownFace(Eleeh, vecEllipse, Holeehs, &m_ELLWallData.dHeight);

	double minRadius = 0.0;
	for (MSElementDescrP& ms : vecEllipse)
	{
		double dRadius = 0.0;
		DPoint3d centerpt;
		DPoint3d ArcDPs[2];

		mdlArc_extract(ArcDPs, NULL, NULL, &dRadius, NULL, NULL, &centerpt, &ms->el);
		//mdlElmdscr_freeAll(&ms);

		if (COMPARE_VALUES_EPS(dRadius, m_ELLWallData.dRadiusOut, EPS) > 0) // 最大半径的圆
		{
			m_ELLWallData.dRadiusOut = dRadius;
			m_ELLWallData.ArcDPs[0] = ArcDPs[0];
			m_ELLWallData.ArcDPs[1] = ArcDPs[1];
			m_ELLWallData.centerpt = centerpt;
		}

		//if (fabs(minRadius - 0.0) < 10 || COMPARE_VALUES_EPS(minRadius, dRadius, EPS) > 0) // 最小半径的圆
		//{
		//	minRadius = dRadius;
		//}
	}
	for (MSElementDescrP& ms : vecEllipse)
	{
		double dRadius = 0.0;
		DPoint3d centerpt;
		DPoint3d ArcDPs[2];

		mdlArc_extract(ArcDPs, NULL, NULL, &dRadius, NULL, NULL, &centerpt, &ms->el);
		mdlElmdscr_freeAll(&ms);
		if (dRadius == 0 || !centerpt.AlmostEqual(m_ELLWallData.centerpt))
		{
			continue;
		}

		if (fabs(minRadius - 0.0) < 10 || COMPARE_VALUES_EPS(minRadius, dRadius, EPS) > 0) // 最小半径的圆
		{
			minRadius = dRadius;
		}
	}
	m_ELLWallData.dRadiusInn = minRadius;

	m_Holeehs = Holeehs;
	return true;
}


bool ELLWallRebarAssembly::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}


void ELLWallRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetSideCover()*uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

	DPoint3d zeropt = DPoint3d::From(0, 0, 0);
	DPoint3d tmpvecZ = DPoint3d::From(0, 0, 1);
	Transform tran;			//构造投影矩阵
	mdlTMatrix_computeFlattenTransform(&tran, &zeropt, &tmpvecZ);
	vector<CurveVectorPtr> curveVectors = CreateBreakArcRange(tran);

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

			TransformInfo transinfo(trans);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			DPoint3d minP;
			DPoint3d maxP;
			//计算指定元素描述符中元素的范围。
			MSElementDescrP pHole = eeh.GetElementDescrP();
			mdlElmdscr_computeRange(&minP, &maxP, pHole, NULL);
			DRange3d range;
			range.low = minP;
			range.high = maxP;
			if (COMPARE_VALUES_EPS(range.XLength(), misssize, 1e-6) == -1 &&
				COMPARE_VALUES_EPS(range.ZLength(), misssize, 1e-6) == -1)
			{
				continue;
			}
			//bool isNeed = false;
			//if (range.XLength() > misssize || range.ZLength() > misssize)
			//{
			//	isNeed = true;
			//}
			mdlElmdscr_transform(&pHole, &tran);
			mdlElmdscr_computeRange(&minP, &maxP, pHole, NULL);
			DPoint3d midPt = minP; midPt.Add(maxP); midPt.Scale(0.5);
			double dis = midPt.Distance(m_ELLWallData.centerpt);
			if (COMPARE_VALUES_EPS(dis, m_ELLWallData.dRadiusInn, 1) == -1)
			{
				continue;
			}





			//if (isNeed)
			//{
				//if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//如果是门洞
				//{
				//	continue;
				//}
			bool isdoorNeg = false;//判断是否为门洞NEG
			isdoorNeg = IsDoorHoleNeg(m_Holeehs.at(j), m_doorsholes);
			ElementCopyContext copier(ACTIVEMODEL);
			copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
			copier.SetTransformToDestination(true);
			copier.SetWriteElements(false);
			copier.DoCopy(*m_Holeehs.at(j));

			if (isdoorNeg)
			{
				PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix, isdoorNeg);
			}
			else
			{
				PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
			}
			int index = 0;
			for (auto it : curveVectors)
			{
				CurveVector::InOutClassification pos1 = it->PointInOnOutXY(minP);
				CurveVector::InOutClassification pos2 = it->PointInOnOutXY(maxP);
				CurveVector::InOutClassification pos3 = it->PointInOnOutXY(midPt);
				if (CurveVector::INOUT_In == pos1 ||
					CurveVector::INOUT_In == pos2 ||
					CurveVector::INOUT_In == pos3)
				{
					m_vecUseHoles[index].push_back(m_Holeehs.at(j));
				}
				++index;
			}
			m_useHoleehs.push_back(m_Holeehs.at(j));
			//}
		}
	}
	for (auto& it : m_vecUseHoles)
	{
		if (it.second.size() > 1)
		{
			UnionIntersectHoles(it.second, m_Holeehs);
		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}

}

long ELLWallRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

// 生成点筋
RebarSetTag* ELLWallRebarAssembly::MakeRebar_Vertical
(
	ElementId& rebarSetId,
	BrString sizeKey,
	DgnModelRefP modelRef,
	double startOffset,  // 起始偏移
	double endOffset,    // 终点偏移
	double spacing,		 // 间距
	double dRoundRadius, // 圆的半径
	double rebarLen,		 // 钢筋长度
	int level,
	int grade,
	int DataExchange,
	bool isTwinRebar // 是否是并筋
)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	vector<PIT::EndType> endType; // 端部样式
	PIT::EndType endFir;
	PIT::EndType endSec;
	endFir.rotateAngle = 0.00;
	endFir.endType = 0;
	endFir.offset = 0.00;

	endSec.rotateAngle = 0.00;
	endSec.endType = 0;
	endSec.offset = 0.00;

	endType.push_back(endFir);
	endType.push_back(endSec);

	vector<CVector3D>  vecEndNormal; // 端部弯钩方向
	CVector3D vecFir = CVector3D::From(0, 0, 0);
	CVector3D vecSec = CVector3D::From(0, 0, 0);
	vecEndNormal.push_back(vecFir);
	vecEndNormal.push_back(vecSec);

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);

	EditElementHandle eehRound;
	if (SUCCESS != EllipseHandler::CreateEllipseElement(eehRound, NULL, m_ELLWallData.centerpt, dRoundRadius, dRoundRadius, 0, true, *modelRef))
	{
		return NULL;
	}

	DPoint3d ArcDPs[2];
	mdlArc_extract(ArcDPs, NULL, NULL, NULL, NULL, NULL, NULL, &eehRound.GetElementDescrP()->el);

	double adjustedSpacing = 0.00;
	double adjustedXLen = 0.00;

	// mdlElmdscr_distanceAtPoint(&adjustedXLen, nullptr, nullptr, eehRound.GetElementDescrP(), &ArcDPs[1], 0.1);
	adjustedXLen = 2 * dRoundRadius * PI - diameter - startOffset - endOffset;

	int numRebar = (int)floor(adjustedXLen / spacing + 0.85);
	adjustedSpacing = spacing;
	if (numRebar > 1 && g_wallRebarInfo.concrete.m_SlabRebarMethod != 2)
	{
		adjustedSpacing = adjustedXLen / (numRebar);
	}

	if (numRebar > 1 && g_wallRebarInfo.concrete.m_SlabRebarMethod != 2)
	{
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	if (isTwinRebar)
	{
		numRebar = m_reabrTwinData.rebarNum;
		adjustedSpacing = m_reabrTwinData.spacing;
	}
	else
	{
		m_reabrTwinData.rebarNum = numRebar;
		m_reabrTwinData.spacing = adjustedSpacing;
		m_reabrTwinData.diameter = diameter;
	}

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
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);
	PITRebarEndTypes   endTypes = { start, end };

	vector<PITRebarCurve>     rebarCurvesNum;

	double xPos = startOffset + diameter * 0.5;
	for (int i = 0; i < numRebar; i++)
	{
		DPoint3d ptStr = DPoint3d::FromZero();
		// 移动指定弧长距离
		mdlElmdscr_pointAtDistance(&ptStr, nullptr, xPos, eehRound.GetElementDescrP(), 0.1);
		movePoint(CVector3D::kZaxis, ptStr, GetSideCover() * uor_per_mm);

		makeRebarCurve(rebarCurvesNum, endTypes, ptStr, rebarLen - GetSideCover() * uor_per_mm * 2);

		if (isTwinRebar && i == numRebar - 2)
		{
			xPos += adjustedSpacing - m_reabrTwinData.diameter - diameter;
			continue;
		}

		xPos += adjustedSpacing;

	}

	eehRound.DeleteFromModel();

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);

	if (!isTwinRebar)
	{
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	else
	{
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}

	vector<vector<DPoint3d>> vecStartEnd;
	for (int j = 0; j < (int)rebarCurvesNum.size(); ++j)
	{
		PITRebarCurve rebarCurve = rebarCurvesNum[j];
		RebarElementP rebarElement = nullptr;
		if (!PreviewButtonDown)
		{//若不是预览状态下则生成钢筋
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}

		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);

		vector<DPoint3d> linePts;
		RebarVertices vertices = rebarCurve.GetVertices();
		for (size_t i = 0; i < (size_t)vertices.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &vertices.At(i);
			linePts.push_back(tmpVertex->GetIP());
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
				if (isTwinRebar)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (isTwinRebar)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (isTwinRebar)
					Stype = "Twinback";
				else
					Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
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


// 生成圆形钢筋
RebarSetTag* ELLWallRebarAssembly::MakeRebar_Round
(
	ElementId& rebarSetId,
	BrString sizeKey,
	DgnModelRefP modelRef,
	double startOffset, // 起始偏移
	double endOffset,   // 终点偏移
	double spacing,		// 间距
	double dRoundRadius,// 圆的半径
	int level,
	int grade,
	int DataExchange,
	bool isTwinRebar
)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	vector<PIT::EndType> endType; // 端部样式
	PIT::EndType endFir;
	PIT::EndType endSec;
	endFir.rotateAngle = 0.00;
	endFir.endType = 0;
	endFir.offset = 0.00;

	endSec.rotateAngle = 0.00;
	endSec.endType = 0;
	endSec.offset = 0.00;

	endType.push_back(endFir);
	endType.push_back(endSec);

	vector<CVector3D>  vecEndNormal; // 端部弯钩方向
	CVector3D vecFir = CVector3D::From(0, 0, 0);
	CVector3D vecSec = CVector3D::From(0, 0, 0);
	vecEndNormal.push_back(vecFir);
	vecEndNormal.push_back(vecSec);

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);

	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	if (!isTwinRebar)
	{
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	else
	{
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}

	// 偏移距离，圆形按垂直间距偏移
	double adjustedSpacing = 0.0;
	double adjustedXLen = m_ELLWallData.dHeight - diameter - startOffset - endOffset - GetSideCover() * uor_per_mm * 2;
	int numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
	adjustedSpacing = spacing;
	// end

	if (numRebar > 1)
	{
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	if (isTwinRebar) // 并筋处理
	{
		numRebar = m_reabrTwinData.rebarNum;
		adjustedSpacing = m_reabrTwinData.spacing;
	}
	else
	{
		// 圆形墙布筋范围太大，按之前的算法布并筋有点问题，此处记录主筋的偏移量和间距，数量
		// 布并筋时 ：在主筋的基础上再偏移 主筋半径 + 并筋半径
		m_reabrTwinData.rebarNum = numRebar;
		m_reabrTwinData.spacing = adjustedSpacing;
		m_reabrTwinData.diameter = diameter;
	}

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
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };

	double xPos = startOffset + diameter * 0.5 + GetSideCover() * uor_per_mm;

	vector<PITRebarCurve>   rebarCurvesNum;
	DPoint3d centerPt = m_ELLWallData.centerpt;
	movePoint(CVector3D::kZaxis, centerPt, xPos);

	for (int i = 0; i < numRebar; i++)
	{
		if (m_vecBreakData.size() <= 1)
		{
			EditElementHandle eehRound;
			if (SUCCESS != EllipseHandler::CreateEllipseElement(eehRound, NULL, centerPt, dRoundRadius, dRoundRadius, 0, true, *modelRef))
			{
				return false;
			}
			vector<MSElementDescrP> vecTmp;
			vector<DPoint3d> vecArcIntersect;
			// 圆形钢筋与孔洞交，得到弧形钢筋（只有一个缺口的弧）和 相交点 vecArcIntersect
			EFT::IntersectHoleEllWall(eehRound, centerPt, m_useHoleehs, vecTmp, vecArcIntersect);
			if (vecTmp.size() == 0) // 没有被孔洞切割 -- 圆形
			{
				makeRoundRebarCurve(rebarCurvesNum, endTypes, centerPt, dRoundRadius);
			}
			else // 被孔洞切割成弧形
			{
				// 画每一段弧
				makeArcRebarCurve(rebarCurvesNum, vecTmp.at(0), endTypes, vecArcIntersect);
			}

		}
		else //画弧
		{
			int index = 0;
			for (auto it : m_vecBreakData)
			{
				//MSElement arcElem;
				//double strRadian = it.beginAngle / 180 * PI;
				double sweepR = (it.endAngle - it.beginAngle)/* / 180 * PI*/;
				//mdlArc_create(&arcElem, NULL, &centerPt, dRoundRadius, dRoundRadius, NULL, strRadian, sweepR);
				MSElementDescrP arcElem = nullptr;
				CreateArc(&arcElem, centerPt, sweepR, it.beginAngle, it.endAngle, dRoundRadius / UOR_PER_MilliMeter);
				DPoint3d startDP, endDP, halfDP;
				mdlElmdscr_extractEndPoints(&startDP, NULL, &endDP, NULL, arcElem, ACTIVEMODEL);
				double Len;
				mdlElmdscr_distanceAtPoint(&Len, NULL, NULL, arcElem, &endDP, 1e-10);
				mdlElmdscr_pointAtDistance(&halfDP, NULL, Len / 2, arcElem, 1e-10);

				//EditElementHandle eehRound(&arcElem, modelRef);
				EditElementHandle eehRound(arcElem, false, false, modelRef);
				vector<MSElementDescrP> vecTmp;
				vector<DPoint3d> vecArcIntersect;
				// 圆形钢筋与孔洞交，得到弧形钢筋（只有一个缺口的弧）和 相交点 vecArcIntersect
				//EFT::IntersectHoleEllWall(eehRound, centerPt, m_useHoleehs, vecTmp, vecArcIntersect);
				GetARCIntersectPointsWithHoles(vecArcIntersect, m_vecUseHoles[index], startDP, endDP, halfDP);
				makeBreakArcRebarCurve(rebarCurvesNum, eehRound.GetElementDescrP(), endTypes, vecArcIntersect);
				++index;
				//if (vecArcIntersect.size() == 0) // 没有被孔洞切割 -- 圆形
				//{
				//	makeArcRebarCurve(rebarCurvesNum, eehRound.GetElementDescrP(), endTypes, vecArcIntersect);							
				//}
				//else // 被孔洞切割成弧形
				//{
				//	// 画每一段弧
				//	makeArcRebarCurve(rebarCurvesNum, eehRound.GetElementDescrP(), endTypes, vecArcIntersect);
				//}
			}
		}
		if (isTwinRebar && i == numRebar - 2)
		{
			movePoint(CVector3D::kZaxis, centerPt, adjustedSpacing - diameter - m_reabrTwinData.diameter);
			continue;
		}

		movePoint(CVector3D::kZaxis, centerPt, adjustedSpacing);
	}

	numRebar = (int)rebarCurvesNum.size();

	vector<DSegment3d> vecStartEnd;
	for (int j = 0; j < (int)rebarCurvesNum.size(); ++j)
	{
		PITRebarCurve rebarCurve = rebarCurvesNum[j];
		RebarElementP rebarElement = nullptr;
		if (!PreviewButtonDown)
		{//若不是预览状态下则生成钢筋
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
				if (isTwinRebar)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (isTwinRebar)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (isTwinRebar)
					Stype = "Twinback";
				else
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

bool ELLWallRebarAssembly::makeArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP mscArc, PIT::PITRebarEndTypes& endTypes, const vector<DPoint3d>& pts)
{
	DPoint3d ArcDPs[2];
	DPoint3d centerpt;
	mdlArc_extract(ArcDPs, NULL, NULL, NULL, NULL, NULL, &centerpt, &mscArc->el);

	DPoint3d ptArcStart = ArcDPs[0];
	DPoint3d ptArcEnd = ArcDPs[1];

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	double dislenth;
	dislenth = 0;
	mdlElmdscr_distanceAtPoint(&dislenth, nullptr, nullptr, mscArc, &ptArcEnd, 0.1);
	for (DPoint3d pt : pts)
	{
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, mscArc, &pt, 0.1);
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
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, mscArc, &itr->second, 0.1);
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
		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, mscArc, &tmpend, 0.1);

		dis1 = dis1 + abs(dis2 - dis1) / 2;

		mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis1, mscArc, 0.1);

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
			ArcSegment arcSeg;
			arcSeg.ptStart = tmpstr;
			arcSeg.ptMid = tmpMid;
			arcSeg.ptEnd = tmpend;

			m_vecArcSeg.push_back(arcSeg);

			trebar.EvaluateEndTypesArc(endTypes);
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(tmpstr, tmpMid, tmpend), true, *ACTIVEMODEL);
			//arceeh1.AddToModel();
			rebar.push_back(trebar);
		}
	}
	return true;
}


bool ELLWallRebarAssembly::makeBreakArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP mscArc, PIT::PITRebarEndTypes& endTypes, const vector<DPoint3d>& pts)
{
	DPoint3d startDP, endDP;
	mdlElmdscr_extractEndPoints(&startDP, NULL, &endDP, NULL, mscArc, ACTIVEMODEL);
	double Len;
	mdlElmdscr_distanceAtPoint(&Len, NULL, NULL, mscArc, &endDP, 1e-10);

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt : pts)
	{
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, mscArc, &pt, 0.1);
		if (dis1 > 10 && dis1 <= Len + 10)
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
		map_pts[1] = startDP;
	}
	else
	{
		map_pts[0] = startDP;
	}
	int dis = (int)Len;
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = endDP;
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = endDP;
	}

	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{

		PITRebarCurve trebar;
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, mscArc, &itr->second, 0.1);
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
		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, mscArc, &tmpend, 0.1);

		dis1 = dis1 + abs(dis2 - dis1) / 2;

		mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis1, mscArc, 0.1);

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

bool ELLWallRebarAssembly::makeRebarCurve(vector<PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d ptStr, double dRebarLength)
{
	DPoint3d ptEnd = ptStr;
	movePoint(CVector3D::kZaxis, ptEnd, dRebarLength);

	DPoint3d pt1[2] = { ptStr, ptEnd };
	//---end
	DPoint3d tmpstr, tmpend;
	tmpstr = ptStr;
	tmpend = ptEnd;
	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetSideCover() * uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt : tmppts)
	{
		if (ExtractFacesTool::IsPointInLine(pt, pt1[0], pt1[1], ACTIVEMODEL, isStr))
		{
			int dis = (int)pt1[0].Distance(pt);
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = pt1[0];
	}
	else
	{
		map_pts[0] = pt1[0];
	}
	int dis = (int)pt1[0].Distance(pt1[1]);
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = pt1[1];
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = pt1[1];
	}

	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{
		map<int, DPoint3d>::iterator itrplus = itr;
		itrplus++;
		if (itrplus == map_pts.end())
		{
			break;
		}

		DPoint3d tmpMid = itr->second; tmpMid.Add(itrplus->second); tmpMid.Scale(0.5);
		if (ISPointInHoles(m_useHoleehs, tmpMid))
		{
			if (ISPointInHoles(m_useHoleehs, itr->second) && ISPointInHoles(m_useHoleehs, itrplus->second))
			{
				continue;
			}
		}

		PITRebarEndTypes		tmpendTypes;

		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itr->second);
		vex->SetType(RebarVertex::kStart);
		if (tmpstr.Distance(itr->second) < 10)
		{
			tmpendTypes.beg = endTypes.beg;
		}
		tmpendTypes.beg.SetptOrgin(itr->second);


		if (tmpend.Distance(itrplus->second) < 10)
		{
			tmpendTypes.end = endTypes.end;
		}

		tmpendTypes.end.SetptOrgin(itrplus->second);

		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itrplus->second);
		vex->SetType(RebarVertex::kEnd);

		rebar.EvaluateEndTypes(tmpendTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebarCurvesNum.push_back(rebar);
	}
	//rebar.DoMatrix(mat);
	return true;
}


// 圆形状钢筋
bool ELLWallRebarAssembly::makeRoundRebarCurve(vector<PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d	centerPoint, double dRoundRadius)
{
	DPoint3d arcStr;
	DPoint3d arcEnd;
	DPoint3d arcMid;
	CVector3D vec = CVector3D::kXaxis;
	arcStr = centerPoint;
	arcEnd = centerPoint;

	ArcSegment arcSeg;

	movePoint(vec, arcStr, dRoundRadius);
	vec = vec.Perpendicular();
	movePoint(vec, arcEnd, dRoundRadius);
	// 画第一段弧
	EditElementHandle eehArc;
	//以ptA为圆心,dRadius为半径，ptIn1为弧的起点，ptIn2为弧的终点画弧
	ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(centerPoint, arcStr, arcEnd), true, *ACTIVEMODEL);
	double dis2 = 0.00;

	mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, eehArc.GetElementDescrP(), &arcEnd, 0.1);

	dis2 /= 2;

	mdlElmdscr_pointAtDistance(&arcMid, nullptr, dis2, eehArc.GetElementDescrP(), 0.1);

	PITRebarCurve rebarCurve;
	if (CalculateRound(rebarCurve, arcStr, arcMid, arcEnd, 1))
	{
		arcSeg.ptCenter = centerPoint;
		arcSeg.ptStart = arcStr;
		arcSeg.dRadius = dRoundRadius;

		EditElementHandle arceeh1;
		ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(arcStr, arcMid, arcEnd), true, *ACTIVEMODEL);
	}

	// 画后三段弧
	for (int i = 0; i < 3; i++)
	{
		arcStr = arcEnd;
		arcEnd = centerPoint;

		vec = vec.Perpendicular();
		movePoint(vec, arcEnd, dRoundRadius);

		//以ptA为圆心,dRadius为半径，ptIn1为弧的起点，ptIn2为弧的终点画弧
		ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(centerPoint, arcStr, arcEnd), true, *ACTIVEMODEL);

		dis2 = 0.00;

		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, eehArc.GetElementDescrP(), &arcEnd, 0.1);

		dis2 /= 2;

		mdlElmdscr_pointAtDistance(&arcMid, nullptr, dis2, eehArc.GetElementDescrP(), 0.1);
		int nStep = 0;
		if (i == 2)
		{
			nStep = 2;
		}
		if (CalculateRound(rebarCurve, arcStr, arcMid, arcEnd, nStep))
		{
			if (i == 0)
			{
				arcSeg.ptMid = arcEnd;
			}
			else if (i == 2)
			{
				arcSeg.ptEnd = arcEnd;
			}
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(arcStr, arcMid, arcEnd), true, *ACTIVEMODEL);
		}
	}
	m_vecArcSeg.push_back(arcSeg);
	rebarCurvesNum.push_back(rebarCurve);

	return true;
}


void ELLWallRebarAssembly::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}


vector<CurveVectorPtr> ELLWallRebarAssembly::CreateBreakArcRange(const Transform& tran)
{
	vector<CurveVectorPtr> curveVectors;
	for (auto it : m_vecBreakData)
	{
		//内弧
		MSElementDescrP inArc = nullptr;
		CreateArc(&inArc, m_ELLWallData.centerpt, it.endAngle - it.beginAngle, it.beginAngle, it.endAngle, m_ELLWallData.dRadiusInn / UOR_PER_MilliMeter);
		EditElementHandle arcEehIn(inArc, true, false, ACTIVEMODEL);
		DPoint3d startDP, endDP;
		mdlElmdscr_extractEndPoints(&startDP, NULL, &endDP, NULL, arcEehIn.GetElementDescrP(), ACTIVEMODEL);
		//arcEehIn.AddToModel();
		//外弧
		MSElementDescrP outArc = nullptr;
		CreateArc(&outArc, m_ELLWallData.centerpt, it.beginAngle - it.endAngle, it.endAngle, it.beginAngle, m_ELLWallData.dRadiusOut / UOR_PER_MilliMeter);
		EditElementHandle arcEehOut(outArc, true, false, ACTIVEMODEL);
		DPoint3d startDPOut, endDPOut;
		mdlElmdscr_extractEndPoints(&startDPOut, NULL, &endDPOut, NULL, arcEehOut.GetElementDescrP(), ACTIVEMODEL);
		//arcEehOut.AddToModel();

		//startDP.z = endDP.z = startDPOut.z = endDPOut.z = 0;
		//两边的线
		EditElementHandle lineEeh1;
		LineHandler::CreateLineElement(lineEeh1, nullptr, DSegment3d::From(endDPOut, startDP), true, *ACTIVEMODEL);
		//lineEeh1.AddToModel();
		EditElementHandle lineEeh2;
		LineHandler::CreateLineElement(lineEeh2, nullptr, DSegment3d::From(endDP, startDPOut), true, *ACTIVEMODEL);
		//lineEeh2.AddToModel();
		//组合
		EditElementHandle shapeEeh;
		ChainHeaderHandler::CreateChainHeaderElement(shapeEeh, nullptr, true, true, *ACTIVEMODEL);
		ComplexShapeHandler::AddComponentElement(shapeEeh, arcEehIn);
		ComplexShapeHandler::AddComponentElement(shapeEeh, lineEeh2);
		ComplexShapeHandler::AddComponentElement(shapeEeh, arcEehOut);
		ComplexShapeHandler::AddComponentElement(shapeEeh, lineEeh1);
		MSElementDescrP pFace = shapeEeh.GetElementDescrP();
		mdlElmdscr_transform(&pFace, &tran);
		//mdlElmdscr_add(pFace);
		CurveVectorPtr curvePtr = ICurvePathQuery::ElementToCurveVector(shapeEeh);
		curveVectors.push_back(curvePtr);
	}
	return curveVectors;
}

bool ELLWallRebarAssembly::CalculateRound(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt, int nStep)
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
		if (nStep == 1) // 圆的首段弧
		{
			vex = &(curve.PopVertices()).NewElement();
			vex->SetIP(beg);
			vex->SetType(RebarVertex::kStart);      // first IP
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

			if (COMPARE_VALUES_EPS(d1, d2, EPS) < 0)
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

		if (nStep == 2) // 圆的最后一段弧
		{
			vex = &curve.PopVertices().NewElement();
			vex->SetIP(end);
			vex->SetType(RebarVertex::kEnd);      // last IP
		}
		else
		{
			//vex = &curve.PopVertices().NewElement();
			//vex->SetIP(end);
			//vex->SetType(RebarVertex::kIP);      // last IP
		}

		ret = true;
	}

	return ret;
}


bool ELLWallRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	CalculateUseHoles(modelRef);

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	int iRebarLevelNum = GetRebarLevelNum();
	vector<PIT::EndType> vecEndType;

	// 所有钢筋层的直径和
	double totalDiameter = 0.00;
	for (BrString sizeKey : GetvecDirSize())
	{
		totalDiameter += RebarCode::GetBarDiameter(sizeKey, modelRef);
	}
	// end

	vector<int> vecDataChange = GetvecDataExchange();
	vector<double> vecLevelSpace = GetvecLevelSpace();

	double levelSpacingTol = 0.0; // 已配置的层间距和

	// 计算层偏移距离
	map<int, vector<LevelInfo>> mapLevelSapce;
	for (int i = 0; i < vecDataChange.size(); i++)
	{
		if (mapLevelSapce.find(vecDataChange.at(i)) != mapLevelSapce.end())
		{
			LevelInfo levelInfo;
			levelInfo.rebarLevel = i;
			levelInfo.LevelSpacing = vecLevelSpace.at(i) * uor_per_mm;
			mapLevelSapce[vecDataChange.at(i)].push_back(levelInfo);
		}
		else
		{
			LevelInfo levelInfo;
			levelInfo.rebarLevel = i;
			levelInfo.LevelSpacing = vecLevelSpace.at(i) * uor_per_mm;

			vector<LevelInfo> vecBack;
			vecBack.push_back(levelInfo);
			mapLevelSapce.insert(make_pair(vecDataChange.at(i), vecBack));
		}

		// 大于两弧的半径差就不算，只能接受微调
		if (COMPARE_VALUES_EPS(vecLevelSpace.at(i) * uor_per_mm, m_ELLWallData.dRadiusOut - m_ELLWallData.dRadiusInn, EPS) < 0)
		{
			levelSpacingTol += vecLevelSpace.at(i) * uor_per_mm;
		}

	}
	// end

	// 计算正面、背面、中间 的层间距
	double levelSpacing = m_ELLWallData.dRadiusOut - m_ELLWallData.dRadiusInn - GetPositiveCover() * uor_per_mm - GetReverseCover() * uor_per_mm;
	levelSpacing -= totalDiameter - levelSpacingTol;

	for (auto itr = mapLevelSapce.begin(); itr != mapLevelSapce.end(); itr++)
	{
		if (itr->first == 1) // 中间
		{
			if (itr->second.size() > 0)
			{
				itr->second.at(0).LevelSpacing += levelSpacing * 0.5;
				levelSpacing *= 0.5;
			}
		}
		else if (itr->first == 2) // 背面
		{
			if (itr->second.size() > 0)
			{
				itr->second.at(0).LevelSpacing += levelSpacing;
			}
		}
	}
	// end

	m_vecArcSeg.clear();
	m_vecRebarStartEnd.clear();

	int iTwinbarSetIdIndex = 0;
	//levelSpacing = m_ELLWallData.dRadiusOut - GetPositiveCover() * uor_per_mm;
	levelSpacing = m_ELLWallData.dRadiusInn + GetPositiveCover() * uor_per_mm;
	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		BrString sizeKey = GetvecDirSize().at(i);
		double diamter = RebarCode::GetBarDiameter(sizeKey, modelRef);

		// 记录钢筋属性
		vector<LevelInfo> vecDis = mapLevelSapce[GetvecDataExchange().at(i)];
		for (LevelInfo info : vecDis)
		{
			if (info.rebarLevel == i)
			{
				//levelSpacing -= info.LevelSpacing;
				levelSpacing += info.LevelSpacing;
				break;
			}
		}

		//levelSpacing -= diamter * 0.5;
		levelSpacing += diamter * 0.5;

		PopvecSetId().push_back(0);
		if (GetvecDir().at(i) == 0)
		{
			RebarSetTag* tag = MakeRebar_Round(GetvecSetId().back(), GetvecDirSize().at(i), modelRef,
				GetvecStartOffset().at(i) * uor_per_mm, GetvecEndOffset().at(i) * uor_per_mm,
				GetvecDirSpacing().at(i) * uor_per_mm, levelSpacing, GetvecRebarLevel().at(i),
				GetvecRebarType().at(i), GetvecDataExchange().at(i));
			if (NULL != tag && (!PreviewButtonDown))
			{
				tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
				rsetTags.Add(tag);
			}

			//绘制并筋--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
			{
				PopvecSetId().push_back(0);

				double diamterTwin = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(i).rebarSize, modelRef);
				//绘制并筋层
				tag = MakeRebar_Round(GetvecSetId().back(), GetvecTwinRebarLevel().at(i).rebarSize, modelRef,
					GetvecStartOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5,
					GetvecEndOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5,
					GetvecDirSpacing().at(i) * uor_per_mm, levelSpacing, GetvecRebarLevel().at(i),
					GetvecRebarType().at(i), GetvecDataExchange().at(i), true);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
		}
		else
		{
			double spacing = GetvecDirSpacing().at(i);
			if (g_wallRebarInfo.concrete.m_SlabRebarMethod = 2) //放射配筋
			{
				spacing = levelSpacing / 10 * PI / 180 * GetvecAngle().at(i);
			}
			// 点筋
			RebarSetTag* tag = MakeRebar_Vertical(GetvecSetId().at(i), GetvecDirSize().at(i), modelRef,
				GetvecStartOffset().at(i) * uor_per_mm, GetvecEndOffset().at(i) * uor_per_mm,
				spacing* uor_per_mm, levelSpacing, m_ELLWallData.dHeight, GetvecRebarLevel().at(i),
				GetvecRebarType().at(i), GetvecDataExchange().at(i));
			if (NULL != tag && (!PreviewButtonDown))
			{
				tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
				rsetTags.Add(tag);
			}
			//绘制并筋--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
			{
				PopvecSetId().push_back(0);

				double diamterTwin = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(i).rebarSize, modelRef);
				//绘制并筋层
				tag = MakeRebar_Vertical(GetvecSetId().back(), GetvecTwinRebarLevel().at(i).rebarSize, modelRef,
					GetvecStartOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5,
					GetvecEndOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5,
					spacing * uor_per_mm, levelSpacing, m_ELLWallData.dHeight,
					GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
		}

		levelSpacing += diamter * 0.5;
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
				EditElementHandle eeh;
				LineStringHandler::CreateLineStringElement(eeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allLines.push_back(eeh.GetElementRef());
			}
		}
		for (ArcSegment arcSeg : m_vecArcSeg)
		{
			if (arcSeg.ptStart.IsEqual(arcSeg.ptEnd, EPS)) // 圆形
			{
				EditElementHandle arceeh;
				if (SUCCESS == ArcHandler::CreateArcElement(arceeh, NULL, arcSeg.ptCenter, arcSeg.dRadius, arcSeg.dRadius, 0, 0, 2 * PI, true, *ACTIVEMODEL))
				{
					arceeh.AddToModel();
					m_allLines.push_back(arceeh.GetElementRef());
				}
			}
			else // 弧形
			{
				EditElementHandle arceeh;
				if (SUCCESS == ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(arcSeg.ptStart, arcSeg.ptMid, arcSeg.ptEnd), true, *ACTIVEMODEL))
				{
					arceeh.AddToModel();
					m_allLines.push_back(arceeh.GetElementRef());
				}
			}
		}
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

bool ELLWallRebarAssembly::CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
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


bool ELLWallRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pEllWallDoubleRebarDlg = new CWallRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pEllWallDoubleRebarDlg->SetSelectElement(ehSel);
	pEllWallDoubleRebarDlg->Create(IDD_DIALOG_WallRebar);
	pEllWallDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pEllWallDoubleRebarDlg->ShowWindow(SW_SHOW);
	return true;
}

bool ELLWallRebarAssembly::Rebuild()
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