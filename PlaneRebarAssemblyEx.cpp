#include "_ustation.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "CFacesRebarDlgEx.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "PlaneRebarAssemblyEx.h"
#include <CPointTool.h>
#include "PITMSCECommon.h"
#include "XmlHelper.h"
#include "SelectRebarTool.h"

using namespace PIT;
extern bool g_FacePreviewButtonsDown;

//平面配筋--begin
PlaneRebarAssemblyEx::PlaneRebarAssemblyEx(ElementId id, DgnModelRefP modelRef) :FacesRebarAssemblyEx(id, modelRef)
{
	m_vecRebarStartEnd.clear();
}

PlaneRebarAssemblyEx::~PlaneRebarAssemblyEx()
{
}

bool PlaneRebarAssemblyEx::makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, const PIT::PITRebarEndTypes& endTypes, double endTypeStartOffset, double endTypeEndOffset)
{
	DPoint3d  startPt = endTypes.beg.GetptOrgin();
	DPoint3d  endPt = endTypes.end.GetptOrgin();
	DPoint3d  ptStartBack = endTypes.beg.GetptOrgin();
	DPoint3d  ptEndBack = endTypes.end.GetptOrgin();
	//确保起点终点是从小到大---begin
	DVec3d vec = endPt - startPt;
	DVec3d vecX = DVec3d::From(1, 0, 0);
	vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
	vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
	vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
	vec.Normalize();

	if (GetIntersectPointsWithNegs(m_Negs, startPt, endPt))
	{
		return false;
	}

	m_vecRebarPtsLayer.push_back(startPt);
	m_vecRebarPtsLayer.push_back(endPt);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetConcrete().sideCover * uor_per_mm;
	vector<DPoint3d> tmppts;
	vector<vector<DPoint3d>> vecPtRebars;
	vector<DPoint3d> tmpptsTmp;
	if (m_face.IsValid()) // 与原实体相交(无孔洞)
	{
		//将原平面往法向方向拉伸为一个实体
		EditElementHandle eehSolid;
		ISolidKernelEntityPtr ptarget;
		if (SolidUtil::Convert::ElementToBody(ptarget, m_face, true, true, true) == SUCCESS &&
			SolidUtil::Modify::ThickenSheet(ptarget, 5000.0 * uor_per_mm, 5000 * uor_per_mm) == SUCCESS &&
			SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL) == SUCCESS)
		{
			DRange3d range;
			mdlElmdscr_computeRange(&range.low, &range.high, eehSolid.GetElementDescrP(), NULL);
			DVec3d tmpVec = endPt - startPt;
			tmpVec.ScaleToLength(5000 * UOR_PER_MilliMeter);
			endPt += tmpVec;
			tmpVec.Negate();
			startPt += tmpVec;
			GetIntersectPointsWithOldElm(tmpptsTmp, &eehSolid, startPt, endPt, dSideCover);
			if (tmpptsTmp.size() > 1)
			{
				// 存在交点为两个以上的情况
				GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, startPt, endPt, &eehSolid, dSideCover);
			}
		}
	}

	if (tmpptsTmp.size() < 2)
	{
		vector<DPoint3d> vecPt;
		vecPt.push_back(startPt);
		vecPt.push_back(endPt);

		vecPtRebars.push_back(vecPt);
	}

	/*EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
	eehline.AddToModel();*/

	for (size_t i = 0; i < vecPtRebars.size(); i++)
	{
		startPt = vecPtRebars.at(i).at(0);
		endPt = vecPtRebars.at(i).at(1);
		GetIntersectPointsWithHoles(tmppts, m_useHoleehs, startPt, endPt, dSideCover);
		map<int, DPoint3d> map_pts;
		bool isStr = false;
		for (DPoint3d pt : tmppts)
		{
			if (ExtractFacesTool::IsPointInLine(pt, startPt, endPt, ACTIVEMODEL, isStr))
			{
				int dis = (int)startPt.Distance(pt);
				if (map_pts.find(dis) != map_pts.end())
				{
					dis = dis + 1;
				}
				map_pts[dis] = pt;
			}
		}
		if (map_pts.find(0) != map_pts.end())
		{
			map_pts[1] = startPt;
		}
		else
		{
			map_pts[0] = startPt;
		}
		int dis = (int)startPt.Distance(endPt);
		if (map_pts.find(dis) == map_pts.end())
		{
			map_pts[dis] = endPt;
		}
		else
		{
			dis = dis + 1;
			map_pts[dis] = endPt;
		}

		DVec3d vecTmp;
		vecTmp.DifferenceOf(startPt, endPt);
		vecTmp.Normalize();
		// 遍历 map_pts 生成钢筋段
		for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
		{
			PITRebarEndTypes endTypesTmp = endTypes;
			PITRebarCurve rebar;
			RebarVertexP vex;
			DPoint3d ptStart(itr->second);
			// 钢筋起点端部偏移
			if (ptStart.IsEqual(map_pts.begin()->second) && i == 0)
				ptStart.SumOf(ptStart, vecTmp, endTypeStartOffset);
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(ptStart);
			vex->SetType(RebarVertex::kStart);
			endTypesTmp.beg.SetptOrgin(ptStart);

			auto itrplus = ++itr;
			if (itrplus == map_pts.end())
			{
				break;
			}

			vecTmp.Negate();
			DPoint3d ptEnd(itrplus->second);
			// 钢筋终点端部偏移
			if (ptEnd.IsEqual((--map_pts.end())->second) && i == vecPtRebars.size() - 1)
				ptEnd.SumOf(ptEnd, vecTmp, endTypeEndOffset);
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(ptEnd);
			vex->SetType(RebarVertex::kEnd);
			endTypesTmp.end.SetptOrgin(ptEnd);

			// EditElementHandle eeh;
			// LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptStart, ptEnd), true, *ACTIVEMODEL);
			// eeh.AddToModel();
			// 生成钢筋段并添加到列表
			rebar.EvaluateEndTypes(endTypesTmp);
			rebars.push_back(rebar);
		}
	}
	return true;
}

RebarSetTag* PlaneRebarAssemblyEx::MakeRebars
(
	ElementId&          rebarSetId,
	LineSegment			rebarLine,
	LineSegment			vec,
	int					dir,
	BrStringCR          sizeKey,
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

	if (endType.size() != vecEndNormal.size() || endType.size() == 0)
	{
		return NULL;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, endbendLen;
	double begStraightAnchorLen = 0, endStraightAnchorLen = 0;
	auto ProcessEndType = [&](RebarEndType& endType, PIT::EndType const& srcEndType, BrStringCR sizeKey, DgnModelRefP modelRef, double& bendRadius, double& bendLen, double& straightAnchorLen)
	{
		switch (srcEndType.endType)
		{
		case 0: //无
		case 1: //弯曲
		case 2: //吊钩
		case 3: //折线
			endType.SetType(RebarEndType::kNone);
			break;
		case 7: //直锚
			endType.SetType(RebarEndType::kLap);
			straightAnchorLen = srcEndType.endPtInfo.value1; //锚入长度
			break;
		case 4: //90度弯钩
		case 5: //135度弯钩
		case 6: //180度弯钩
		{
			if (srcEndType.endType == 4) endType.SetType(RebarEndType::kBend);
			else if (srcEndType.endType == 5) endType.SetType(RebarEndType::kCog);
			else if (srcEndType.endType == 6) endType.SetType(RebarEndType::kHook);
			bendRadius = srcEndType.endPtInfo.value1;
			if (COMPARE_VALUES(bendRadius, 0) == 0)
			{
				bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false); //乘以了30
			}
			bendLen = srcEndType.endPtInfo.value3; //预留长度
			if (COMPARE_VALUES(bendLen, 0) == 0)
			{
				bendLen = RebarCode::GetBendLength(sizeKey, endType, modelRef); //乘以了100
			}
		}
		break;
		case 8: //用户
			endType.SetType(RebarEndType::kCustom);
			break;
		default:
			break;
		}
	};
	// 使用新函数处理起始端和结束端
	ProcessEndType(endTypeStart, endType[0], sizeKey, modelRef, startbendRadius, startbendLen, begStraightAnchorLen);
	ProcessEndType(endTypeEnd, endType[1], sizeKey, modelRef, endbendRadius, endbendLen, endStraightAnchorLen);

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double adjustedXLen, adjustedSpacing;
	double sideCov = GetConcrete().sideCover*uor_per_mm;
	double updownSideCover = 50 * uor_per_mm;
	rebarLine.PerpendicularOffset(startOffset, vec.GetLineVec());
	if (GetConcrete().isFaceUnionRebar && !_ehCrossPlanePre.IsValid() && !_ehCrossPlaneNext.IsValid())
	{
		double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);

		rebarLine.PerpendicularOffset(diameter + spacing, vec.GetLineVec());
		rebarLine.PerpendicularOffset(diameter, PopfaceNormal());
		vec.Shorten(sideCov * 2 + diameter + spacing, true);
		vec.Shorten(-sideCov * 2 - diameter * 2, false);
		adjustedXLen = vec.GetLength() - diameter - startOffset - endOffset;
	}
	else
	{
		adjustedXLen = vec.GetLength() - updownSideCover * 2 - startOffset - endOffset;
	}
	int numRebar = 0;
	numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	adjustedSpacing = spacing;
	//	rebarLine.PerpendicularOffset(startOffset, vec.GetLineVec());
	//	rebarLine.PerpendicularOffset(sideCov, vec.GetLineVec());
	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypeEndOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypeEndOffset += diameter * 0.5;

	// 	EditElementHandle eeh;
	// 	LineHandler::CreateLineElement(eeh, NULL, rebarLine.GetLineSeg(), true, *ACTIVEMODEL);
	// 	eeh.AddToModel();


	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);
	//rebarLine.Shorten(sideCov + endTypeStartOffset,true);
	//rebarLine.Shorten(/*sideCov + */endTypeStartOffset, true);
	start.SetptOrgin(rebarLine.GetLineStartPoint());
	//rebarLine.Shorten(sideCov + endTypEendOffset, false);
	//rebarLine.Shorten(/*sideCov + */endTypeEndOffset, false);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);
	end.SetptOrgin(rebarLine.GetLineEndPoint());

	PITRebarEndTypes   endTypes = { start, end };
	//	rebarLine.PerpendicularOffset(sideCov+diameter*0.5, vec.GetLineVec());
	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurves;
		if (i == numRebar - 1)//如果是最后一根，要判断当前还有多少距离,符合距离要求就要再布置一根
		{
			double sDis = adjustedXLen - (numRebar - 2)*adjustedSpacing;
			if (sDis > spacing)
			{
				DVec3d dvec = vec.GetLineVec();
				rebarLine.PerpendicularOffset((sDis - spacing), dvec);
			}
			else
			{
				DVec3d dvec = vec.GetLineVec();
				dvec.Negate();
				rebarLine.PerpendicularOffset((spacing - sDis), dvec);
			}
		}
		endTypes.beg.SetptOrgin(rebarLine.GetLineStartPoint());
		endTypes.end.SetptOrgin(rebarLine.GetLineEndPoint());

		makeRebarCurve(rebarCurves, endTypes, endTypeStartOffset, endTypeEndOffset);

		//		xPos += adjustedSpacing
		DVec3d offsetVec = vec.GetLineVec();
		rebarLine.PerpendicularOffset(adjustedSpacing, offsetVec);

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);
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
		// 		EditElementHandle eeh;
		// 		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		// 		eeh.AddToModel();

		RebarElementP rebarElement = NULL;
		if (!g_FacePreviewButtonsDown)//预览标志，预览状态下不要生成钢筋
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

	m_vecRebarCurvePt.push_back(rebarCurvesNum);
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

bool PlaneRebarAssemblyEx::MakeRebars(DgnModelRefP modelRef)
{
	_d = 1.0;

	EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());
	//testeeh.AddToModel();
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	SetSelectedElement(Eleeh.GetElementId());
	if (m_Holeehs.size() == 0 && Holeehs.size() > 0)

	{
		m_Holeehs = Holeehs;
	}
	if (g_globalpara.Getrebarstyle() != 0)
	{
		NewRebarAssembly(modelRef);
	}
	SetSelectedElement(testeeh.GetElementId());
	CalculateUseHoles(modelRef);

	RebarSetTagArray rsetTags;
	m_vecRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
	double dLevelSpace = 0;
	double updownSideCover = 50 * uor_per_mm;
	double dSideCover = GetConcrete().sideCover * uor_per_mm;
	double dPositiveCover = GetConcrete().postiveCover * uor_per_mm;
	double offset = dPositiveCover;

	int iRebarSetTag = 0;
	int iRebarLevelNum = GetConcrete().rebarLevelNum;
	vector<PIT::EndType> vecEndType;

	// 孔洞规避前 面配筋钢筋点 按层存储，之前不同层的不能删掉
	std::vector<RebarPoint> vecRebarPoint;
	ElementId contid = this->FetchConcrete();
	GetElementXAttribute(contid, vecRebarPoint, vecRebarPointsXAttribute, ACTIVEMODEL);
	std::map<int, vector<RebarPoint>> mapRebarPoint;
	for (RebarPoint stRebarPt : vecRebarPoint)
	{
		auto iter = mapRebarPoint.find(stRebarPt.DataExchange);
		if (iter != mapRebarPoint.end())
		{
			iter->second.push_back(stRebarPt);
		}
		else
		{
			vector<RebarPoint> vecTemp;
			vecTemp.push_back(stRebarPt);
			mapRebarPoint[stRebarPt.DataExchange] = vecTemp;
		}
	}
	// end

	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		PopSetIds().push_back(0);
		RebarSetTag* tag = NULL;
		vector<PIT::EndType> vecEndType;
		if (GetvecEndTypes().empty())		//没有设置端部样式，设置默认值
		{
			EndType endType;
			memset(&endType, 0, sizeof(endType));
			vecEndType = { { 0,0,0 } ,endType };
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
		double levelspacing = GetMainRebars().at(i).levelSpace * uor_per_mm;
		int DataExchange = GetMainRebars().at(i).datachange;
		m_vecRebarPtsLayer.clear();
		m_vecTwinRebarPtsLayer.clear();
		LineSegment lineSeg1 = m_LineSeg1;
		LineSegment lineSeg2 = m_LineSeg2;
		vector<CVector3D> vecEndNormal(2);
		CVector3D	endNormal;	//端部弯钩方向
		if (rebarDir == 1)	//纵向钢筋
		{
			if (GetConcrete().isFaceUnionRebar && !_ehCrossPlanePre.IsValid() && !_ehCrossPlaneNext.IsValid())
			{
				continue;
			}
			if (GetvecEndTypes().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = GetfaceNormal();
					CVector3D rebarVec = m_LineSeg2.GetLineVec();
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}

			DVec3d vec = m_LineSeg1.GetLineVec();
			offset += levelspacing;
			if (0 == i)	//首层偏移当前钢筋直径
			{
				offset += diameter * 0.5;
			}
			else
			{
				double diameterPre = RebarCode::GetBarDiameter(GetMainRebars().at(i - 1).rebarSize, modelRef);	//上一层钢筋直径
				offset += diameterPre * 0.5;	//偏移上一层钢筋的半径
				offset += diameter * 0.5;		//偏移当前层钢筋的半径
			}
			lineSeg2.PerpendicularOffset(offset, GetfaceNormal());
			lineSeg2.PerpendicularOffset(updownSideCover, vec);
			tag = MakeRebars(PopSetIds().at(i), lineSeg2, m_LineSeg1, rebarDir, strRebarSize, spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();
		}
		else
		{
			if (GetvecEndTypes().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					CVector3D rebarVec = m_LineSeg1.GetLineVec();
					endNormal = GetfaceNormal();
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}

			DVec3d vec = m_LineSeg2.GetLineVec();
			offset += levelspacing;
			if (0 == i)	//首层偏移当前钢筋直径
			{
				offset += diameter * 0.5;
			}
			else
			{
				double diameterPre = RebarCode::GetBarDiameter(GetMainRebars().at(i - 1).rebarSize, modelRef);	//上一层钢筋直径
				offset += diameterPre * 0.5;	//偏移上一层钢筋的半径
				offset += diameter * 0.5;		//偏移当前层钢筋的半径
			}
			lineSeg1.PerpendicularOffset(offset, GetfaceNormal());
			lineSeg1.PerpendicularOffset(updownSideCover, vec);
			if (_ehCrossPlanePre.IsValid())
			{
				vecEndType[_anchorPos].endType = 7;
				vecEndType[_anchorPos].endPtInfo.value1 = _d;
			}
			if (_ehCrossPlaneNext.IsValid())
			{
				vecEndType[_bendPos].endType = 7;
				vecEndType[_bendPos].endPtInfo.value1 = _d;
				//vecEndType[_bendPos].endType = 4;
				//vecEndNormal[_bendPos] = PopfaceNormal();
				//vecEndType[_bendPos].endPtInfo.value3 = _d*2;
			}
			tag = MakeRebars(PopSetIds().at(i), lineSeg1, m_LineSeg2, rebarDir, strRebarSize, spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}

			vecEndType.clear();
		}

		// 删除属于当前成的钢筋
		auto itrTemp = mapRebarPoint.find(DataExchange);
		if (itrTemp != mapRebarPoint.end())
		{
			mapRebarPoint.erase(itrTemp);
		}
		// end

		if (m_vecRebarPtsLayer.size() > 1)
		{
			for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
			{
				int n = m + 1;
				RebarPoint rbPt;
				rbPt.Layer = GetMainRebars().at(i).rebarLevel;
				rbPt.vecDir = rebarDir;
				rbPt.ptstr = m_vecRebarPtsLayer.at(m);
				rbPt.ptend = m_vecRebarPtsLayer.at(n);
				rbPt.DataExchange = DataExchange;
				g_vecRebarPtsNoHole.push_back(rbPt);
				m++;
			}

		}
		if (m_vecTwinRebarPtsLayer.size() > 1)
		{
			for (int m = 0; m < m_vecTwinRebarPtsLayer.size() - 1; m++)
			{
				int n = m + 1;
				RebarPoint rbPt;
				rbPt.Layer = GetMainRebars().at(i).rebarLevel;
				rbPt.vecDir = rebarDir;
				rbPt.ptstr = m_vecTwinRebarPtsLayer.at(m);
				rbPt.ptend = m_vecTwinRebarPtsLayer.at(n);
				rbPt.DataExchange = DataExchange;
				g_vecTwinRebarPtsNoHole.push_back(rbPt);
				m++;
			}
		}
	}

	for (auto itr = mapRebarPoint.begin(); itr != mapRebarPoint.end(); itr++)
	{
		g_vecRebarPtsNoHole.insert(g_vecRebarPtsNoHole.end(), itr->second.begin(), itr->second.end());
	}

	DrawPreviewLines();

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}


bool PlaneRebarAssemblyEx::OnDoubleClick()
{
	ElementId testid = GetElementId();
	PlaneRebarAssemblyEx* faceRebar = REA::Load<PlaneRebarAssemblyEx>(testid, ACTIVEMODEL);

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
	// 	CFacesRebarDlg dlg(ehSel,CWnd::FromHandle(MSWIND));
	// 	
	// 	dlg.SetConcreteId(GetConcreteOwner());
	// 	if (IDCANCEL == dlg.DoModal())
	// 		return false;

	return true;
}

bool PlaneRebarAssemblyEx::Rebuild()
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

bool PlaneRebarAssemblyEx::AnalyzingFaceGeometricData(EditElementHandleR eeh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eeh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	if (GetfaceType() == FaceType::other)
	{
		MSElementDescrP msDscp = eeh.GetElementDescrP();
		if (msDscp == NULL)
		{
			return false;
		}

		// 取一个元素的范围
		DPoint3d minP = { 0 }, maxP = { 0 };
		mdlElmdscr_computeRange(&minP, &maxP, msDscp, NULL); // 计算指定元素描述符中元素的范围

		DVec3d ptVec = maxP - minP;
		DPoint3d ptEnd1 = DPoint3d::FromZero();
		DPoint3d ptEnd2 = DPoint3d::FromZero();
		if (fabs(ptVec.z - 0.0) < uor_ref) // XOY 平面
		{
			if (COMPARE_VALUES_EPS(maxP.x - minP.x, maxP.y - minP.y < 0, EPS) >= 0)
			{
				ptEnd1 = maxP;
				ptEnd1.y = minP.y;

				ptEnd2 = maxP;
				ptEnd2.x = minP.x;
			}
			else
			{
				ptEnd1 = maxP;
				ptEnd1.x = minP.x;

				ptEnd2 = maxP;
				ptEnd2.y = minP.y;
			}

		}
		else if (fabs(ptVec.x - 0.0) < uor_ref) // YOZ平面
		{
			if (COMPARE_VALUES_EPS(maxP.y - minP.y, maxP.z - minP.z < 0, EPS) >= 0)
			{
				ptEnd1 = maxP;
				ptEnd1.z = minP.z;

				ptEnd2 = maxP;
				ptEnd2.y = minP.y;
			}
			else
			{
				ptEnd1 = maxP;
				ptEnd1.y = minP.y;

				ptEnd2 = maxP;
				ptEnd2.z = minP.z;
			}
		}
		else if (fabs(ptVec.y - 0.0) < uor_ref) // XOZ平面
		{
			if (COMPARE_VALUES_EPS(maxP.x - minP.x, maxP.z - minP.z < 0, EPS) >= 0)
			{
				ptEnd1 = maxP;
				ptEnd1.z = minP.z;

				ptEnd2 = maxP;
				ptEnd2.x = minP.x;
			}
			else
			{
				ptEnd1 = maxP;
				ptEnd1.x = minP.x;

				ptEnd2 = maxP;
				ptEnd2.z = minP.z;
			}
		}
		else
		{
			return false;
		}

		m_LineSeg1.SetLineSeg(DSegment3d::From(minP, ptEnd1));
		m_LineSeg2.SetLineSeg(DSegment3d::From(minP, ptEnd2));

		return true;
	}

	if (m_Solid != NULL && m_vecElm.empty())
	{
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(*m_Solid, Eleeh, Holeehs);
		m_Holeehs = Holeehs;
	}

	for (size_t i = 0; i < m_vecElm.size(); ++i)
	{
		//将孔洞上有板的孔洞过滤
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(m_vecElm[i], Eleeh, Holeehs);
		for (auto it : Holeehs)
		{
			double areap = 0;
			mdlMeasure_areaProperties(nullptr, &areap, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, it->GetElementDescrP(), 0);
			if (areap > BIGHOLEAREA)
			{
				m_Holeehs.push_back(it);
				continue;
			}
			//mdlElmdscr_addByModelRef(it->GetElementDescrP(), ACTIVEMODEL);
			DPoint3d holeMinPt = { 0,0,0 }, holeMaxPt = { 0,0,0 };
			mdlElmdscr_computeRange(&holeMinPt, &holeMaxPt, it->GetElementDescrP(), nullptr);
			DRange3d holeRange = DRange3d::From(holeMinPt, holeMaxPt);
			bool isValid = true;
			vector<MSElementDescrP> elmDecrP;
			for (size_t j = 0; j < m_vecElm.size(); ++j)
			{
				if (j == i)
				{
					continue;
				}
				EditElementHandle elmEeh(m_vecElm[j], m_vecElm[j].GetDgnModelP());
				DPoint3d elmMinPt = { 0,0,0 }, elmMaxPt = { 0,0,0 };
				mdlElmdscr_computeRange(&elmMinPt, &elmMaxPt, elmEeh.GetElementDescrP(), nullptr);
				DPoint3d midPt = elmMinPt; midPt.Add(elmMaxPt); midPt.Scale(0.5);
				if (holeRange.IsContained(midPt))
				{
					isValid = false;
					break;
				}
			}
			if (isValid)
			{
				m_Holeehs.push_back(it);
			}
		}
		//m_Holeehs.insert(m_Holeehs.end(), Holeehs.begin(), Holeehs.end());
	}
	//先转到xoy平面
// 	
// 	RotMatrix rMatrix;
// 	Transform trans;
// 	DPoint3d vecz = DPoint3d::From(0, 0, 1);
// 	mdlRMatrix_getIdentity(&rMatrix);
// 	mdlRMatrix_fromVectorToVector(&rMatrix, &GetfaceNormal(), &vecz);//旋转到xoy平面
// 	mdlTMatrix_fromRMatrix(&trans, &rMatrix);
// 	mdlTMatrix_setOrigin(&trans, &DPoint3d::From(0,0,0));
// 	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, TransformInfo(trans));
	EditElementHandle tmpeeh;
	if (m_UseXOYDir)//如果是板平面，以面的范围面作为基准面
	{
		DPoint3d minP, maxP;
		mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
		DPoint3d PTS[4];
		PTS[0] = PTS[1] = minP;
		PTS[1].x = maxP.x;
		PTS[2] = PTS[1]; PTS[2].y = maxP.y;
		PTS[3] = PTS[2]; PTS[3].x = minP.x;
		ShapeHandler::CreateShapeElement(tmpeeh, NULL, PTS, 4, true, *ACTIVEMODEL);
	}
	else
	{
		tmpeeh.Duplicate(eeh);
	}

	ElementAgenda agenda;//存放打散之后的元素
	DropGeometryPtr pDropGeometry = DropGeometry::Create();//创建一个DropGeometry实例来设置几何选项
	if (tmpeeh.GetElementType() == CMPLX_SHAPE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Complex);
	}
	else if (tmpeeh.GetElementType() == SURFACE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Solids);
	}
	else
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_LinearSegments);
	}
	if (SUCCESS != tmpeeh.GetDisplayHandler()->Drop(tmpeeh, agenda, *pDropGeometry))
	{
		agenda.Clear();
		mdlOutput_printf(MSG_STATUS, L"打散面失败！");
		return false;
	}

	class lineSort {
	public:
		bool operator()(const LineSegment &line1, const LineSegment &line2) const {
			return line1.GetLength() > line2.GetLength();
		}
	};
	std::set < LineSegment, lineSort > lineSegs;
	vector<ArcSegment> vecArcSeg;
	for (EditElementHandleR LineEeh : agenda)
	{
		if (LineEeh.GetElementType() == LINE_ELM)
		{
			DPoint3d pt[2];
			mdlLinear_extract(pt, NULL, LineEeh.GetElementP(), tmpeeh.GetModelRef());
			LineSegment lineSeg(pt[0], pt[1]);
			lineSegs.insert(lineSeg);
		}
	}

	// 取长度最长的边
	// mdlElmdscr_extractNormal(&PopfaceNormal(), NULL, tmpeeh.GetElementDescrCP(), NULL);
	//	PopfaceNormal().Negate();
	LineSegment maxLine = *lineSegs.begin();

	//针对斜切墙体，最长线可能是斜边，需要重新获取最长先段
	DVec3d vecmax = maxLine.GetLineVec();
	if (!m_UseXOYDir)
	{
		vecmax.Normalize();
		if (COMPARE_VALUES_EPS(vecmax.z, 0.00, 0.1) != 0)
		{
			maxLine = *(++lineSegs.begin());
		}
	}

	CVector3D  xVec(maxLine.GetLineVec());
	CVector3D  yVec = CVector3D(PopfaceNormal()).CrossProduct(xVec);
	BeMatrix   matrix = CMatrix3D::Ucs(maxLine.GetLineStartPoint(), xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	Transform trans;
	matrix.AssignTo(trans);
	trans.InverseOf(trans);
	tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, TransformInfo(trans));
	//eeh.AddToModel();
	//tmpeeh.AddToModel();
// 
	if (m_vecElm.size())
	{
		if (m_slabUpFace)
		{
			SetfaceNormal(DVec3d::From(0, 0, -1));
		}
		else
		{
			SetfaceNormal(DVec3d::From(0, 0, 1));
		}
	}
	//计算平面的最大范围
	DPoint3d ptMin, ptMax;
	mdlElmdscr_computeRange(&ptMin, &ptMax, tmpeeh.GetElementDescrCP(), NULL);

	DPoint3d ptLineSeg1End = ptMax;
	DPoint3d ptLineSeg2End = ptMin;
	ptLineSeg1End.y = ptMin.y;
	ptLineSeg2End.y = ptMax.y;

	DSegment3d lineSeg1 = DSegment3d::From(ptMin, ptLineSeg1End);
	DSegment3d lineSeg2 = DSegment3d::From(ptMin, ptLineSeg2End);

	EditElementHandle ehLine1, ehLine2;
	LineHandler::CreateLineElement(ehLine1, NULL, lineSeg1, true, *ACTIVEMODEL);
	LineHandler::CreateLineElement(ehLine2, NULL, lineSeg2, true, *ACTIVEMODEL);
	// 	ehLine1.AddToModel();
	// 	ehLine2.AddToModel();

	trans.InverseOf(trans);
	ehLine1.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(ehLine1, TransformInfo(trans));
	ehLine2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(ehLine2, TransformInfo(trans));

	DPoint3d ptLine1[2];
	DPoint3d ptLine2[2];
	mdlLinear_extract(ptLine1, NULL, ehLine1.GetElementCP(), ACTIVEMODEL);
	mdlLinear_extract(ptLine2, NULL, ehLine2.GetElementCP(), ACTIVEMODEL);

	m_LineSeg1.SetLineSeg(DSegment3d::From(ptLine1[0], ptLine1[1]));
	m_LineSeg2.SetLineSeg(DSegment3d::From(ptLine2[0], ptLine2[1]));

	if (/*m_LineSeg1.GetLineVec().IsParallelTo(DVec3d::From(0,0,1))*/ abs(m_LineSeg1.GetLineVec().DotProduct(DVec3d::From(0, 0, 1))) > 0.9
		|| (abs(m_LineSeg1.GetLineVec().DotProduct(DVec3d::From(0, 1, 0))) > 0.9 && abs(m_LineSeg2.GetLineVec().DotProduct(DVec3d::From(1, 0, 0))) > 0.9))
	{
		LineSegment segTmp = m_LineSeg2;
		m_LineSeg2 = m_LineSeg1;
		m_LineSeg1 = segTmp;
	}
	else if (COMPARE_VALUES_EPS(m_LineSeg1.GetLineStartPoint().x, m_LineSeg1.GetLineEndPoint().x, EPS) == 0
		&& COMPARE_VALUES_EPS(m_LineSeg1.GetLineStartPoint().y, m_LineSeg1.GetLineEndPoint().y, EPS) != 0
		&& m_LineSeg2.GetLineVec().IsParallelTo(DVec3d::From(1, 0, 0)))
	{
		//Added by chenxuan 面配筋横向竖向方向不对
		LineSegment segTmp = m_LineSeg2;
		m_LineSeg2 = m_LineSeg1;
		m_LineSeg1 = segTmp;
	}

	if (_ehCrossPlanePre.IsValid())
	{
		DPoint3d ptNormal;
		DPoint3d ptInFace;
		mdlElmdscr_extractNormal(&ptNormal, &ptInFace, _ehCrossPlanePre.GetElementDescrCP(), NULL);
		if (ptNormal.IsParallelTo(m_LineSeg2.GetLineVec()))
		{
			LineSegment seg = m_LineSeg1;
			m_LineSeg1 = m_LineSeg2;
			m_LineSeg2 = seg;
		}

		//判断垂直面在起点处还是终点处
		double dis1 = ptInFace.Distance(m_LineSeg1.GetLineStartPoint());
		double dis2 = ptInFace.Distance(m_LineSeg1.GetLineEndPoint());
		_anchorPos = dis1 > dis2 ? 1 : 0;
	}
	if (_ehCrossPlaneNext.IsValid())
	{
		DPoint3d ptNormal;
		DPoint3d ptInFace;
		mdlElmdscr_extractNormal(&ptNormal, &ptInFace, _ehCrossPlaneNext.GetElementDescrCP(), NULL);
		if (ptNormal.IsParallelTo(m_LineSeg2.GetLineVec()))
		{
			LineSegment seg = m_LineSeg1;
			m_LineSeg1 = m_LineSeg2;
			m_LineSeg2 = seg;
		}

		//判断垂直面在起点处还是终点处
		double dis1 = ptInFace.Distance(m_LineSeg1.GetLineStartPoint());
		double dis2 = ptInFace.Distance(m_LineSeg1.GetLineEndPoint());
		_bendPos = dis1 > dis2 ? 1 : 0;
	}
	// start确保向量是从绝对坐标系最低往最高
	auto SwapLineSeg = [&](PIT::LineSegment& lineSeg1, PIT::LineSegment& lineSeg2,
		Dpoint3d& ptSeg1Start, Dpoint3d& ptSeg1End, Dpoint3d& ptSeg2Start, Dpoint3d& ptSeg2End)
	{
		swap(ptSeg1Start, ptSeg1End);
		lineSeg1.SetLineStartPoint(ptSeg1Start);
		lineSeg1.SetLineEndPoint(ptSeg1End);
		lineSeg2.SetLineStartPoint(ptSeg2Start);
		lineSeg2.SetLineEndPoint(ptSeg2End);
	};
	auto StandardStartAndEnd = [&](PIT::LineSegment& lineSeg1, PIT::LineSegment& lineSeg2)
	{
		DVec3d vec1 = lineSeg1.GetLineVec();
		DVec3d vec2 = lineSeg2.GetLineVec();
		double length1 = lineSeg1.GetLength();
		double length2 = lineSeg2.GetLength();
		Dpoint3d ptSeg1Start = lineSeg1.GetLineStartPoint();
		Dpoint3d ptSeg1End = lineSeg1.GetLineEndPoint();
		Dpoint3d ptSeg2Start = lineSeg2.GetLineStartPoint();
		Dpoint3d ptSeg2End = lineSeg2.GetLineEndPoint();
		if (vec1.DotProduct(DVec3d::From(-1, 0, 0)) > 0.9)
		{
			ptSeg2Start = ptSeg1End;
			movePoint(vec1, ptSeg2End, length1);
			SwapLineSeg(lineSeg1, lineSeg2, ptSeg1Start, ptSeg1End, ptSeg2Start, ptSeg2End);
		}
		else if (vec1.DotProduct(DVec3d::From(0, -1, 0)) > 0.9)
		{
			ptSeg2Start = ptSeg1End;
			movePoint(vec1, ptSeg2End, length1);
			SwapLineSeg(lineSeg1, lineSeg2, ptSeg1Start, ptSeg1End, ptSeg2Start, ptSeg2End);
		}
		else if (vec1.DotProduct(DVec3d::From(0, 0, -1)) > 0.9)
		{
			ptSeg2Start = ptSeg1End;
			movePoint(vec1, ptSeg2End, length1);
			SwapLineSeg(lineSeg1, lineSeg2, ptSeg1Start, ptSeg1End, ptSeg2Start, ptSeg2End);
		}
	};
	StandardStartAndEnd(m_LineSeg1, m_LineSeg2);
	StandardStartAndEnd(m_LineSeg2, m_LineSeg1);
	// end确保向量是从绝对坐标系最低往最高
	PopvecFrontPts().push_back(m_LineSeg1.GetLineStartPoint());
	PopvecFrontPts().push_back(m_LineSeg1.GetLineEndPoint());
	return true;
}

void PlaneRebarAssemblyEx::CalculateUseHoles(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetConcrete().sideCover*uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

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
				PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}
}
//平面配筋---end
long PlaneRebarAssemblyEx::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		map << _d;
		return 0;
	}
	default:
		break;
	}
	return -1;
}
