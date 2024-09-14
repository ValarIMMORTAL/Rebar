#include "_ustation.h"
#include "CStarisRebarDlog.h"
#include "StairsRebarAssembly.h"
#include "ExtractFacesTool.h"
#include "resource.h"
#include "XmlHelper.h"
#include "SelectRebarTool.h"
#include "CPointTool.h"

using namespace PITCommonTool;
CStairsRebarAssembly::CStairsRebarAssembly(ElementId id, DgnModelRefP modelRef) :
	RebarAssembly(id, modelRef)
{
	m_pStarisRebarDlg = NULL;
	m_StairsDownInfo.height = 0.00;
	m_StairsDownInfo.length = 0.00;

	m_StairsSideInfo.maxDis_Down = 0.00;
	m_StairsSideInfo.minDis_Down = 0.00;
	m_StairsSideInfo.StepHeight = 0.00;
	m_StairsSideInfo.StepLength = 0.00;
	m_StairsSideInfo.StepWidth = 0.00;


	m_stepDiameter = 0.00;
	//	m_downDiameter = 0.00;
}

CStairsRebarAssembly::~CStairsRebarAssembly()
{

}

void CStairsRebarAssembly::SetUcsMatrix(DPoint3d ptStart, DPoint3d ptEnd)
{
	CVector3D  xVec(ptStart, ptEnd);

	// 返回两个向量的（向量）叉积 
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
}

bool CmdFun(DSegment3d seStr, DSegment3d seEnd)
{
	DPoint3d pt1, pt2;
	seStr.GetEndPoint(pt1);
	seEnd.GetEndPoint(pt2);

	if (COMPARE_VALUES_EPS(pt1.z, pt2.z, 10) > 0)
	{
		return true;
	}

	if (COMPARE_VALUES_EPS(pt1.z, pt2.z, 10) == 0)
	{
		seStr.GetStartPoint(pt1);
		seEnd.GetStartPoint(pt2);
		if (COMPARE_VALUES_EPS(pt1.z, pt2.z, 10) > 0)
		{
			return true;
		}
	}

	return false;
}


// 解析楼梯实体的特征参数
void CStairsRebarAssembly::GetStairsFeatureParam(ElementHandleCR eh)
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

	// 去楼梯底面前面的线和后面的线
	EFT::GetStairsFrontBackLinePoint(testeeh, vecDownFontLine, vecDownBackLine, &m_StairsDownInfo.height);
	if (vecDownFontLine.empty() || vecDownBackLine.empty())
	{
		return;
	}

	DPoint3d pt1[2];
	vecDownFontLine[0].GetStartPoint(pt1[0]);
	vecDownFontLine[0].GetEndPoint(pt1[1]);
	
	DPoint3d pt2[2];
	vecDownBackLine[0].GetStartPoint(pt2[0]);
	vecDownBackLine[0].GetEndPoint(pt2[1]);
	
	if (vecDownBackLine.size() > 1 || vecDownFontLine.size() > 1)
	{
		GetMaxDownFacePts(vecDownFontLine, vecDownBackLine, pt1, pt2);
	}
	//CPointTool::DrowOneLine(pt1[0], pt1[1], 1);
	//CPointTool::DrowOneLine(pt2[0], pt2[1], 2);
	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_StairsDownInfo.height);

	m_StairsDownInfo.height = m_StairsDownInfo.height * uor_now / uor_ref; // 台阶的垂直高度
	m_StairsSideInfo.StepLength = FrontStr.Distance(BackStr) * uor_now / uor_ref; // 每级台阶的长
	m_StairsDownInfo.length = FrontStr.Distance(FrontEnd) * uor_now / uor_ref; // 台阶底面的长度
	DPoint3d vec1 = FrontEnd - FrontStr;
	vec1.Normalize();
	/*movePoint(vec1, FrontStr, 520);
	movePoint(vec1, BackStr, 520);*/
// 	EditElementHandle eeh;
// 	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(FrontStr, FrontEnd), true, *ACTIVEMODEL);
// 	eeh.AddToModel();
	m_StairsDownInfo.ptStart = FrontStr; // 底面最长线方向
	m_StairsDownInfo.ptEnd = FrontEnd;
	//CPointTool::DrowOneLine(FrontStr, FrontEnd, 1);
	m_StairsSideInfo.ptFrontStart = FrontStr; // 楼梯往里方向
	m_StairsSideInfo.ptBackStart = BackStr;
	//CPointTool::DrowOneLine(FrontStr, BackStr, 3);

	SetUcsMatrix(FrontStr, FrontEnd);

	DVec3d compareVec;
	if (COMPARE_VALUES_EPS(FrontEnd.z, FrontStr.z, 10) > 0)
	{
		compareVec = FrontEnd - FrontStr;
	}
	else
	{
		compareVec = FrontStr - FrontEnd;
	}
	compareVec.z = 0;
	compareVec.Normalize();
	m_vecSideZLine.clear();
	m_vecSideXYLine.clear();

	bool bFlag = false;
	DVec3d compareVecTmp = BackStr - FrontStr;
	compareVecTmp.Normalize();
	if (COMPARE_VALUES_EPS(compareVecTmp.x, 0, EPS) > 0 || COMPARE_VALUES_EPS(compareVecTmp.y, 0, EPS) > 0)
	{
		bFlag = true;
	}

	if (COMPARE_VALUES_EPS(compareVecTmp.x, 0, EPS) < 0 || COMPARE_VALUES_EPS(compareVecTmp.y, 0, EPS) < 0)
	{
		bFlag = false;
	}

	// 解析楼梯侧面的线，将垂直的线和水平的线按由高到低的顺序存起来
	EditElementHandle eehSideFace;
	EFT::GetStairFace(testeeh, eehSideFace, &m_StairsDownInfo.height, 1, bFlag);
	eehSideFace.AddToModel();
	EFT::GetLineStairSideFace(eehSideFace, m_vecSideXYLine, m_vecSideZLine, m_SideMaxLine, compareVec, model);
	// end 

	sort(m_vecSideXYLine.begin(), m_vecSideXYLine.end(), CmdFun);
	sort(m_vecSideZLine.begin(), m_vecSideZLine.end(), CmdFun);
	// end

	//for (int i=0; i < m_vecSideXYLine.size();i++)
	//{
	//	CPointTool::DrowOneLine(m_vecSideXYLine[i], 2);//1绿，2黄，3红
	//}

	//for (int i = 0; i < m_vecSideZLine.size(); i++)
	//{
	//	CPointTool::DrowOneLine(m_vecSideZLine[i], 4);//1绿，2黄，3红
	//}

	if (m_vecSideXYLine.size() > 2 && m_vecSideZLine.size() > 3)
	{
		DPoint3d ptStr, ptEnd;
		m_vecSideXYLine[1].GetStartPoint(ptStr);
		m_vecSideXYLine[1].GetEndPoint(ptEnd);
		m_StairsSideInfo.StepWidth = ptStr.Distance(ptEnd);
		m_vecSideZLine[2].GetStartPoint(ptStr);
		m_vecSideZLine[2].GetEndPoint(ptEnd);
		m_StairsSideInfo.StepHeight = ptStr.Distance(ptEnd);

		DPoint3d ptMaxStr, ptMaxEnd;
		m_SideMaxLine.GetStartPoint(ptMaxStr);
		m_SideMaxLine.GetEndPoint(ptMaxEnd);

		bool bFlag = false;
		DPoint3d ptProject1;	//投影点
		mdlVec_projectPointToLine(&ptProject1, NULL, &ptStr, &ptMaxStr, &ptMaxEnd);
		if (EFT::IsPointInLine(ptProject1, ptMaxStr, ptMaxEnd, model, bFlag))
		{
			m_StairsSideInfo.maxDis_Down = ptProject1.Distance(ptStr); // 台阶最高的点到底面的距离
		}

		mdlVec_projectPointToLine(&ptProject1, NULL, &ptEnd, &ptMaxStr, &ptMaxEnd);
		if (EFT::IsPointInLine(ptProject1, ptMaxStr, ptMaxEnd, model, bFlag))
		{
			m_StairsSideInfo.minDis_Down = ptProject1.Distance(ptEnd); // 台阶最低的点到底面的距离
			m_StairsSideInfo.ptStart = ptProject1; // 侧面上边到下边的方向
			m_StairsSideInfo.ptEnd = ptEnd;
		}
	}

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());

	return;
}

bool CStairsRebarAssembly::IsSmartSmartFeature(EditElementHandle& eeh)
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
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}

		}
		return true;
	}
}

bool CStairsRebarAssembly::makeRebarCurve
(
	PIT::PITRebarCurve&			rebar,
	PIT::PITRebarEndTypes&		endTypes,
	vector<PIT::EndType>&			vecEndtype,
	vector<CPoint3D>&			vecPoint,
	double						bendRadius,
	bool						bFlag
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

	endTypes.beg.SetptOrgin(vecPoint[0]);
	endTypes.end.SetptOrgin(vecPoint[vecPoint.size() - 1]);

	for (int i = 1; i < rebar.PopVertices().GetSize() - 1; i++)
	{
		rebar.PopVertices()[i]->EvaluateBend(*rebar.PopVertices()[i - 1], *rebar.PopVertices()[i + 1]);
	}

	rebar.EvaluateEndTypesStirrup(endTypes);

	return true;
}

bool CStairsRebarAssembly::makeRebarCurve
(
	PIT::PITRebarCurve&			rebar,
	PIT::PITRebarEndTypes&		endTypes,
	CPoint3D const&         ptstr,
	CPoint3D const&         ptend
)
{
	RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptstr);
	vex->SetType(RebarVertex::kStart);

	endTypes.beg.SetptOrgin(ptstr);
	endTypes.end.SetptOrgin(ptend);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptend);
	vex->SetType(RebarVertex::kEnd);

	rebar.EvaluateEndTypes(endTypes);
	//rebar.DoMatrix(mat);
	return true;
}

RebarSetTag* CStairsRebarAssembly::MakeTieRebars
(
	vector<PIT::EndType> vecEndtype,
	vector<vector<CPoint3D>> vvecTieRebarPts,
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
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false) * 0.5;	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef) * 0.5;	//乘以了100
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
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false) * 0.5;	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef) * 0.5;	//乘以了100
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	bool const isStirrup = false;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30

	vector<PIT::PITRebarCurve>     rebarCurvesNum;

	PIT::PITRebarEndType start, end;
	start.SetType((PIT::PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(vecEndtype[0].rotateAngle);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetstraightAnchorLen(begStraightAnchorLen);

	end.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(vecEndtype[1].rotateAngle);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetstraightAnchorLen(endStraightAnchorLen);

	double startOffset = 0.00 * uor_per_mm;
	double endOffset = 0.00 * uor_per_mm;

	double spacing = 100 * uor_per_mm;		// 间距
	double adjustedXLen = m_StairsSideInfo.StepLength - m_Cover * 2 - startOffset - endOffset - diameter;
	int rebarNum = (int)ceil(adjustedXLen / spacing) + 1;
	spacing = adjustedXLen / (rebarNum - 1);

	vector<CVector3D> vecEndNormal(2);
	CVector3D	endNormal;	//端部弯钩方向

	for (unsigned int i = 0; i < 2; i++)
	{
		CPoint3D pt[2];
		if (m_vecSideXYLine.size() > 1)
		{
			m_vecSideXYLine[1].GetStartPoint(pt[0]);
			m_vecSideXYLine[1].GetEndPoint(pt[1]);
		}
		if (i == 0)
		{
			double dRotateAngle = vecEndtype[i].rotateAngle;
			CVector3D rebarVec = pt[0] - pt[1];
			endNormal = CVector3D::From(0, 0, -1);
			endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
		}
		else
		{
			double dRotateAngle = vecEndtype[i].rotateAngle;
			CVector3D rebarVec = CVector3D::From(0, 0, -1);
			endNormal = pt[0] - pt[1];
			endNormal.Normalize();
			endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
		}

		vecEndNormal[i] = endNormal;
	}

	start.SetendNormal(vecEndNormal[0]);
	end.SetendNormal(vecEndNormal[1]);
	PIT::PITRebarEndTypes   endTypes = { start, end };
	//	PIT::PITRebarEndTypes   endTypesBak = endTypes;

	for (size_t i = 0; i < rebarNum; i++)
	{
		if (i > 0)
		{
			for (size_t j = 0; j < m_vvecRebarPts.size(); j++)
			{
				for (size_t k = 0; k < m_vvecRebarPts[j].size(); k++)
				{
					DPoint3d vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
					movePoint(vec, m_vvecRebarPts[j][k], spacing);
				}
			}
		}

		for (size_t j = 0; j < m_vvecRebarPts.size(); j++)
		{
			if (j == 0)
			{
				if (COMPARE_VALUES_EPS(spacing * i + m_Cover + diameter * 0.5, (m_StairsSideInfo.StepLength - 550 * uor_per_mm) * 0.5, uor_per_mm) < 0 ||
					COMPARE_VALUES_EPS(spacing * i + m_Cover + diameter * 0.5, (m_StairsSideInfo.StepLength - 550 * uor_per_mm) * 0.5 + 550 * uor_per_mm, uor_per_mm) > 0)
				{
					AttachRebarInfo stAttachInfo;
					stAttachInfo.strEndType = RebarEndType::kHook;
					stAttachInfo.endEndType = RebarEndType::kNone;

					for (vector<CPoint3D>::reverse_iterator itr = m_vvecRebarPts[j].rbegin(); itr != m_vvecRebarPts[j].rend(); itr++)
					{
						stAttachInfo.vecPoint.push_back(*itr);
					}

					if (stAttachInfo.vecPoint.size() == 0)
					{
						continue;
					}
					DPoint3d ptStr = stAttachInfo.vecPoint.at(stAttachInfo.vecPoint.size() - 1);
					DPoint3d ptEnd = ptStr;
					ptEnd.z -= 1000 * uor_per_mm;

					DPoint3d ptSeg[2];
					ptSeg[0] = m_ptMainRebarDown[0];
					ptSeg[1] = m_ptMainRebarDown[1];

					// 将两条线段移到同一个平面上
					if (COMPARE_VALUES_EPS(m_ptMainRebarDown[0].x, m_ptMainRebarDown[1].x, EPS) == 0)
					{
						ptSeg[0].x = ptStr.x;
						ptSeg[1].x = ptStr.x;
					}
					else if (COMPARE_VALUES_EPS(m_ptMainRebarDown[0].y, m_ptMainRebarDown[1].y, EPS) == 0)
					{
						ptSeg[0].y = ptStr.y;
						ptSeg[1].y = ptStr.y;
					}

					DSegment3d seg_H = { ptSeg[0], ptSeg[1] };
					DSegment3d seg_V = { ptStr, ptEnd };
					DPoint3d ptIntersec;
					if (SUCCESS == mdlVec_intersect(&ptIntersec, &seg_H, &seg_V))
					{
						stAttachInfo.vecPoint.push_back(ptIntersec);

						CVector3D moveVec = m_ptMainRebarDown[1] - m_ptMainRebarDown[0];

						movePoint(moveVec, ptIntersec, m_StairsSideInfo.StepWidth);
						stAttachInfo.vecPoint.push_back(ptIntersec);

						DPoint3d ptTemp = ptIntersec;

						movePoint(moveVec, ptTemp, 604.1523 * uor_per_mm);

						double dot = 600.0 / 604.1523;

						CVector3D vecDot = m_ptMainRebarDown[1] - m_ptMainRebarDown[0];
						DPoint3d vec1 = m_ptMainRebarDown[1] - m_ptMainRebarDown[0];
						DPoint3d vec2 = m_vecTailPoint.at(4) - m_vecTailPoint.at(3);
						double dotAngle = mdlVec_dotProduct(&vec1, &vec2);
						CVector3D vecCorss = m_StairsSideInfo.ptEnd - m_StairsSideInfo.ptStart;
						vecDot.Normalize();
						vecCorss.Normalize();
						CVector3D dotVec = vecDot.Perpendicular(vecCorss);
						CMatrix3D mat = CMatrix3D::Rotate(ptIntersec, acos(dot), dotVec);

						Transform trans;
						mat.AssignTo(trans);
						TransformInfo transinfo(trans);
						EditElementHandle eehLine;
						LineHandler::CreateLineElement(eehLine, nullptr, DSegment3d::From(ptIntersec, ptTemp), true, *ACTIVEMODEL);
						eehLine.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehLine, transinfo);

						DPoint3d pt1[2];
						mdlLinear_extract(pt1, NULL, eehLine.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改
						stAttachInfo.vecPoint.push_back(pt1[1]);
						stAttachInfo.vecEndNormal = vecEndNormal[1];
						stAttachInfo.vecEndNormalTmp = vecEndNormal[0];
					}

					m_vvecAttach_10mm.push_back(stAttachInfo);
				}
				else
				{
					AttachRebarInfo stAttachInfo;
					stAttachInfo.strEndType = RebarEndType::kHook;
					stAttachInfo.endEndType = RebarEndType::kHook;

					for (CPoint3D ptTmp : m_vvecRebarPts[j])
					{
						stAttachInfo.vecPoint.push_back(ptTmp);
					}
					stAttachInfo.vecEndNormal = vecEndNormal[0];
					stAttachInfo.vecEndNormalTmp = vecEndNormal[1];
					m_vvecAttach_10mm.push_back(stAttachInfo);
				}
				continue;
			}

			PIT::PITRebarCurve     rebarTieCurves;
			makeRebarCurve(rebarTieCurves, endTypes, vecEndtype, m_vvecRebarPts[j], bendRadius);
			rebarCurvesNum.push_back(rebarTieCurves);
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
	int numRebar = (int)rebarCurvesNum.size();
	int j = 0;
	for (PIT::PITRebarCurve rebarCurve : rebarCurvesNum)
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

void CStairsRebarAssembly::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

RebarSetTag* CStairsRebarAssembly::MakeStepRebars
(
	vector<PIT::EndType> vecEndtype,
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
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false) * 0.5;	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef) * 0.5;	//乘以了100
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
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false) * 0.5;	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef) * 0.5;	//乘以了100
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	bool const isStirrup = false;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double dRebarLen = m_StairsDownInfo.length - m_Cover * 2 - 110 * uor_per_mm;

	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30

	vector<CVector3D> vecEndNormal(2);
	CVector3D	endNormal;	//端部弯钩方向

	for (unsigned int i = 0; i < 2; i++)
	{
		double dRotateAngle = vecEndtype[i].rotateAngle;
		//CVector3D rebarVec = CVector3D::kZaxis;
		//endNormal = m_StairsInfo.ptEnd - m_StairsInfo.ptStart;
		//endNormal.Normalize();

		//endNormal = rebarVec.CrossProduct(vec);
		//endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转

		CVector3D rebarVec = m_StairsDownInfo.ptEnd - m_StairsDownInfo.ptStart;
		endNormal = CVector3D::From(0, 0, -1);

		endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转

		vecEndNormal[i] = endNormal;
	}

	vector<PIT::PITRebarCurve>     rebarCurvesNum;

	PIT::PITRebarEndType start, end;
	start.SetType((PIT::PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(vecEndtype[0].rotateAngle);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);
	start.SetstraightAnchorLen(begStraightAnchorLen);

	end.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(vecEndtype[1].rotateAngle);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);
	end.SetstraightAnchorLen(endStraightAnchorLen);

	PIT::PITRebarEndTypes   endTypes = { start, end };

	double startOffset = 0.00 * uor_per_mm; // 起点偏移
	double dPos = startOffset + m_Cover;
	double spacing = 100 * uor_per_mm;		// 间距

	double diameter_8 = RebarCode::GetBarDiameter(L"8A", modelRef);
	double diameter_12 = RebarCode::GetBarDiameter(L"12C", modelRef);
	double diameter_Tie = RebarCode::GetBarDiameter(L"10A", modelRef); // 拉筋的尺寸

	BrString sizeKeyMain = m_StairsRebarInfo.rebarSize;
	sizeKeyMain.Replace(L"mm", L"");
	double diameterMain = RebarCode::GetBarDiameter(sizeKeyMain, modelRef);

	DPoint3d vecXY;
	vector<DPoint3d> vecPtSteps;
	DPoint3d firstTie;
	int iIndex = 0;
	for (DSegment3d seg : m_vecSideXYLine) // 楼梯垂直边的点筋
	{
		//EditElementHandle lineEeh;
		//LineHandler::CreateLineElement(lineEeh, nullptr, seg, true, *ACTIVEMODEL);
		//lineEeh.AddToModel();
		CPoint3D pt[2];
		seg.GetStartPoint(pt[0]);
		seg.GetEndPoint(pt[1]);

		dRebarLen = m_StairsSideInfo.StepLength - 2 * m_Cover;
		DPoint3d	ptStr = pt[1];

		DPoint3d vec = pt[0] - pt[1];
		movePoint(vec, ptStr, m_Cover + 0.5 * diameter); // 水平方向移动

		DPoint3d vecZ = DPoint3d::From(0, 0, -1); // 垂直方向移动
		movePoint(vecZ, ptStr, m_Cover + 0.5 * diameter_Tie);

		vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart; // 楼梯往里方向
		movePoint(vec, ptStr, m_Cover);

		vecPtSteps.push_back(ptStr);

		vec = pt[0] - pt[1];
		vec.Normalize();
		vecZ = DPoint3d::From(0, 0, -1);
		mdlVec_add(&vecXY, &vecZ, &vec);
		movePoint(vecXY, ptStr, diameter * 1.7);

		DPoint3d ptEnd = ptStr;
		vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
		movePoint(vec, ptEnd, dRebarLen);

		if (iIndex == 1 || iIndex == m_vecSideXYLine.size() - 2)
		{
			DPoint3d ptTmp = ptStr;
			vec = pt[0] - pt[1];
			movePoint(vec, ptTmp, 110 * uor_per_mm - m_Cover);
			DPoint3d ptTmpEnd = ptTmp;
			vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
			movePoint(vec, ptTmpEnd, dRebarLen);

			AttachRebarInfo stAttachInfo;
			stAttachInfo.vecPoint.clear();
			stAttachInfo.vecPoint.push_back(ptTmp);
			stAttachInfo.vecPoint.push_back(ptTmpEnd);

			stAttachInfo.strEndType = RebarEndType::kHook;
			stAttachInfo.endEndType = RebarEndType::kHook;
			stAttachInfo.vecEndNormal = CVector3D::From(0, 0, -1);
			stAttachInfo.vecEndNormalTmp = CVector3D::From(0, 0, -1);
			m_vvecAttach_10mm.push_back(stAttachInfo);

			vec = pt[0] - pt[1];
			movePoint(vec, ptTmp, 60 * uor_per_mm - diameter_Tie);
			ptTmpEnd = ptTmp;
			vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
			movePoint(vec, ptTmpEnd, dRebarLen);

			stAttachInfo.vecPoint.clear();
			stAttachInfo.vecPoint.push_back(ptTmp);
			stAttachInfo.vecPoint.push_back(ptTmpEnd);

			stAttachInfo.strEndType = RebarEndType::kHook;
			stAttachInfo.endEndType = RebarEndType::kHook;
			stAttachInfo.vecEndNormal = CVector3D::From(0, 0, -1);
			stAttachInfo.vecEndNormalTmp = CVector3D::From(0, 0, -1);
			m_vvecAttach_10mm.push_back(stAttachInfo);


			vec = pt[0] - pt[1];
			movePoint(vec, ptTmp, 50 * uor_per_mm + diameter_Tie);
			ptTmpEnd = ptTmp;
			vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
			movePoint(vec, ptTmpEnd, dRebarLen);

			stAttachInfo.vecPoint.clear();
			stAttachInfo.vecPoint.push_back(ptTmp);
			stAttachInfo.vecPoint.push_back(ptTmpEnd);

			stAttachInfo.strEndType = RebarEndType::kHook;
			stAttachInfo.endEndType = RebarEndType::kHook;
			stAttachInfo.vecEndNormal = CVector3D::From(0, 0, -1);
			stAttachInfo.vecEndNormalTmp = CVector3D::From(0, 0, -1);
			m_vvecAttach_10mm.push_back(stAttachInfo);

		}

		if (iIndex == m_vecSideXYLine.size() - 1)
		{
			AttachRebarInfo stAttachInfo;
			stAttachInfo.vecPoint.clear();
			stAttachInfo.vecPoint.push_back(ptStr);
			stAttachInfo.vecPoint.push_back(ptEnd);

			stAttachInfo.strEndType = RebarEndType::kHook;
			stAttachInfo.endEndType = RebarEndType::kHook;
			stAttachInfo.vecEndNormal = CVector3D::From(0, 0, -1);
			stAttachInfo.vecEndNormalTmp = CVector3D::From(0, 0, -1);
			m_vvecAttach_12mm.push_back(stAttachInfo);
			m_vecTailPoint.push_back(ptStr);

			DPoint3d vec = pt[0] - pt[1];
			movePoint(vec, ptStr, 110 * uor_per_mm + diameter_12);
			movePoint(vec, ptEnd, 110 * uor_per_mm + diameter_12);

			stAttachInfo.vecPoint.clear();
			stAttachInfo.vecPoint.push_back(ptStr);
			stAttachInfo.vecPoint.push_back(ptEnd);
			stAttachInfo.strEndType = RebarEndType::kHook;
			stAttachInfo.endEndType = RebarEndType::kHook;
			stAttachInfo.vecEndNormal = CVector3D::From(0, 0, -1);
			stAttachInfo.vecEndNormalTmp = CVector3D::From(0, 0, -1);
			m_vvecAttach_12mm.push_back(stAttachInfo);
			m_vecTailPoint.push_back(ptStr);

			movePoint(vec, ptStr, 65 * uor_per_mm + diameter);
			movePoint(vec, ptEnd, 65 * uor_per_mm + diameter);

			PIT::PITRebarCurve     rebarCurvesTmp;
			makeRebarCurve(rebarCurvesTmp, endTypes, ptStr, ptEnd);
			rebarCurvesNum.push_back(rebarCurvesTmp);

			m_vecTailPoint.push_back(ptStr);

			DPoint3d ptAnthorStr = ptStr;
			DPoint3d ptAnthorEnd = ptStr;
			movePoint(pt[0] - pt[1], ptAnthorStr, 2000 * uor_per_mm);

			DPoint3d ptSegTmp[2];
			ptSegTmp[0] = m_ptMainRebarDown[0];
			ptSegTmp[1] = m_ptMainRebarDown[1];

			// 将两条线段移到同一个平面上
			if (COMPARE_VALUES_EPS(m_ptMainRebarUp[0].x, m_ptMainRebarUp[1].x, EPS) == 0)
			{
				ptSegTmp[0].x = ptAnthorStr.x;
				ptSegTmp[1].x = ptAnthorStr.x;
			}
			else if (COMPARE_VALUES_EPS(m_ptMainRebarUp[0].y, m_ptMainRebarUp[1].y, EPS) == 0)
			{
				ptSegTmp[0].y = ptAnthorStr.y;
				ptSegTmp[1].y = ptAnthorStr.y;
			}

			DSegment3d seg_Anthor_H = { ptAnthorStr, ptAnthorEnd };
			DSegment3d seg_Anthor_V = { ptSegTmp[0], ptSegTmp[1] };
			DPoint3d ptIntersecTmp;
			if (SUCCESS == mdlVec_intersect(&ptIntersecTmp, &seg_Anthor_H, &seg_Anthor_V))
			{
				ptAnthorEnd = ptIntersecTmp;
				movePoint(pt[1] - pt[0], ptAnthorEnd, diameterMain * 2);
			}

			DPoint3d ptIndex = ptStr;
			ptIndex.z -= 1000 * uor_per_mm;

			DPoint3d ptSeg[2];
			ptSeg[0] = m_ptMainRebarUp[0];
			ptSeg[1] = m_ptMainRebarUp[1];

			// 将两条线段移到同一个平面上
			if (COMPARE_VALUES_EPS(m_ptMainRebarUp[0].x, m_ptMainRebarUp[1].x, EPS) == 0)
			{
				ptSeg[0].x = ptStr.x;
				ptSeg[1].x = ptStr.x;
			}
			else if (COMPARE_VALUES_EPS(m_ptMainRebarUp[0].y, m_ptMainRebarUp[1].y, EPS) == 0)
			{
				ptSeg[0].y = ptStr.y;
				ptSeg[1].y = ptStr.y;
			}

			DSegment3d seg_H = { ptSeg[0], ptSeg[1] };
			DSegment3d seg_V = { ptStr, ptIndex };

			DPoint3d ptIntersec;
			if (SUCCESS == mdlVec_intersect(&ptIntersec, &seg_H, &seg_V))
			{
				vec = CVector3D::From(0, 0, 1);
				movePoint(vec, ptIntersec, diameter * 5.0);
				if (COMPARE_VALUES_EPS(ptIntersec.z, ptStr.z, 1) == 1)
				{
					CVector3D tmpVec = CVector3D::From(0, 0, -1);
					movePoint(tmpVec, ptIntersec, diameter * 10.0);
				}

				ptStr = ptIntersec;
				ptEnd = ptStr;
				vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
				movePoint(vec, ptEnd, dRebarLen);

				PIT::PITRebarCurve     rebarCurvesTTmp;
				makeRebarCurve(rebarCurvesTTmp, endTypes, ptStr, ptEnd);
				rebarCurvesNum.push_back(rebarCurvesTTmp);
				m_vecTailPoint.push_back(ptStr);

				DPoint3d ptTemp = m_ptMainRebarDown[1];
				// 将两条线段移到同一个平面上
				if (COMPARE_VALUES_EPS(m_ptMainRebarDown[0].x, m_ptMainRebarDown[1].x, EPS) == 0)
				{
					ptTemp.x = ptStr.x;
				}
				else if (COMPARE_VALUES_EPS(m_ptMainRebarDown[0].y, m_ptMainRebarDown[1].y, EPS) == 0)
				{
					ptTemp.y = ptStr.y;
				}

				movePoint(m_ptMainRebarDown[0] - m_ptMainRebarDown[1], ptTemp, diameter * 1.5);

				m_vecTailPoint.push_back(ptTemp);
				movePoint(m_StairsSideInfo.ptEnd - m_StairsSideInfo.ptStart, ptTemp, diameterMain * 0.5 + diameter * 0.5);
				ptStr = ptTemp;
				ptEnd = ptTemp;
				vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
				movePoint(vec, ptEnd, dRebarLen);

				PIT::PITRebarCurve     rebarCurvesTTTmp;
				makeRebarCurve(rebarCurvesTTTmp, endTypes, ptStr, ptEnd);
				rebarCurvesNum.push_back(rebarCurvesTTTmp);

				ptStr = ptAnthorEnd;
				ptEnd = ptAnthorEnd;
				vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
				movePoint(vec, ptEnd, dRebarLen);

				m_vecTailPoint.push_back(ptStr);
			}

			//EditElementHandle shapeEeh;
			//DPoint3d pts[6] = { m_vecTailPoint.at(0), m_vecTailPoint.at(1), m_vecTailPoint.at(2),
			//m_vecTailPoint.at(3), m_vecTailPoint.at(4), m_vecTailPoint.at(5) };
			//ShapeHandler::CreateShapeElement(shapeEeh, nullptr, pts, 6, true, *modelRef);
			//shapeEeh.AddToModel();
		}

		PIT::PITRebarCurve     rebarCurves;
		makeRebarCurve(rebarCurves, endTypes, ptStr, ptEnd);
		rebarCurvesNum.push_back(rebarCurves);

		// 12 毫米的两根点筋
		if (iIndex == 0)
		{
			AttachRebarInfo stAttachInfo;

			ptStr = pt[0];
			vec = pt[1] - pt[0];
			movePoint(vec, ptStr, 55 * uor_per_mm); // 水平方向移动
			movePoint(vecZ, ptStr, m_Cover + 0.5 * diameter_Tie);	// 往下

			vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart; // 楼梯往里方向
			movePoint(vec, ptStr, m_Cover);

			firstTie = ptStr;
			vec = pt[0] - pt[1];
			movePoint(vec, firstTie, diameter_Tie * 2);

			vec = CVector3D::From(0, 0, -1);
			movePoint(vec, ptStr, (diameter_12 * 0.5 + diameter_Tie * 0.5) * 1.25);
			vec = pt[0] - pt[1];
			movePoint(vec, ptStr, diameter_Tie * 0.85);

			ptEnd = ptStr;
			vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
			movePoint(vec, ptEnd, dRebarLen);

			stAttachInfo.endEndType = RebarEndType::kHook;
			stAttachInfo.strEndType = RebarEndType::kHook;
			stAttachInfo.vecPoint.push_back(ptStr);
			stAttachInfo.vecPoint.push_back(ptEnd);
			stAttachInfo.vecEndNormal = CVector3D::From(0, 0, -1);
			stAttachInfo.vecEndNormalTmp = CVector3D::From(0, 0, -1);
			m_vvecAttach_12mm.push_back(stAttachInfo);

			vec = pt[1] - pt[0];
			movePoint(vec, ptStr, 110 * uor_per_mm); // 水平方向移动

			vec = CVector3D::From(0, 0, 1);
			movePoint(vec, ptStr, (diameter_12 * 0.5 + diameter_Tie * 0.5) * 0.25);

			ptEnd = ptStr;
			vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
			movePoint(vec, ptEnd, dRebarLen);

			stAttachInfo.vecPoint.clear();
			stAttachInfo.endEndType = RebarEndType::kHook;
			stAttachInfo.strEndType = RebarEndType::kHook;
			stAttachInfo.vecPoint.push_back(ptStr);
			stAttachInfo.vecPoint.push_back(ptEnd);
			stAttachInfo.vecEndNormal = CVector3D::From(0, 0, -1);
			stAttachInfo.vecEndNormalTmp = CVector3D::From(0, 0, -1);
			m_vvecAttach_12mm.push_back(stAttachInfo);

			vec = pt[1] - pt[0];
			movePoint(vec, ptStr, 73.0 * uor_per_mm + diameter + diameter_Tie * 0.5); // 水平方向移动

			vec = CVector3D::From(0, 0, 1);
			movePoint(vec, ptStr, diameter - diameter_Tie);

			ptEnd = ptStr;
			vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
			movePoint(vec, ptEnd, dRebarLen);

			PIT::PITRebarCurve     rebarCurvesTTTmp;
			makeRebarCurve(rebarCurvesTTTmp, endTypes, ptStr, ptEnd);
			rebarCurvesNum.push_back(rebarCurvesTTTmp);
		}

		iIndex++;
	}

	// 算拉筋的点
	m_vvecRebarPts.clear();
	vector<CPoint3D> TieRebarPts;
	for (int i = 0; i < vecPtSteps.size() - 1; i++)
	{
		TieRebarPts.clear();
		DPoint3d PtIP1 = vecPtSteps[i];
		DPoint3d ptIP2 = vecPtSteps[i + 1];

		DPoint3d pt[2];
		m_vecSideXYLine[i].GetStartPoint(pt[0]);
		m_vecSideXYLine[i].GetEndPoint(pt[1]);

		DPoint3d vec1 = ptIP2 - PtIP1;
		vec1.Normalize();
		DPoint3d vec2 = DPoint3d::From(0, 0, -1);
		double dot = mdlVec_dotProduct(&vec1, &vec2);

		double dHypotenuseLen = ptIP2.Distance(PtIP1); // 斜边
		double dOppositeLen = dHypotenuseLen * dot; // 对边

		DPoint3d ptStr = PtIP1;
		movePoint(vec2, ptStr, dOppositeLen);
		movePoint(vecXY, ptStr, diameter * 1.5);

		//if (i == vecPtSteps.size() - 2)
		//{
		//	movePoint(CVector3D::From(0, 0, -1), ptStr, diameter * 0.5);
		//}

		DPoint3d ptEnd = ptStr;
		DPoint3d vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
		movePoint(vec, ptEnd, dRebarLen);


		vecEndNormal.at(0) = CVector3D::From(0, 0, -1);
		vecEndNormal.at(1) = CVector3D::From(0, 0, -1);
		start.SetendNormal(vecEndNormal[0]);
		end.SetendNormal(vecEndNormal[1]);
		endTypes = { start, end };

		PIT::PITRebarCurve     rebarCurves;
		makeRebarCurve(rebarCurves, endTypes, ptStr, ptEnd); // 垂直中间点
		rebarCurvesNum.push_back(rebarCurves);


		vecEndNormal.at(0) = CVector3D::From(0, 0, 1);
		vecEndNormal.at(1) = CVector3D::From(0, 0, 1);
		start.SetendNormal(vecEndNormal[0]);
		end.SetendNormal(vecEndNormal[1]);
		endTypes = { start, end };

		DPoint3d ptProject1;	//投影点
		mdlVec_projectPointToLine(&ptProject1, NULL, &PtIP1, &m_ptMainRebarDown[0], &m_ptMainRebarDown[1]);

		//EditElementHandle eeh;
		//LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptProject1, PtIP1), true, *ACTIVEMODEL);
		//eeh.AddToModel();

		vec1 = ptProject1 - PtIP1;
		vec1.Normalize();

		vec2 = pt[0] - pt[1];
		vec2.Normalize();

		double dEdgeLen = ptProject1.Distance(PtIP1); // 临边
		dot = mdlVec_dotProduct(&vec1, &vec2);
		dHypotenuseLen = dEdgeLen / dot;

		ptStr = PtIP1;
		movePoint(vec2, ptStr, dHypotenuseLen - 3 * diameter);
		DPoint3d ptBackUp;
		if (i != 0)
		{
			ptBackUp = ptStr;
			vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
			movePoint(vec, ptBackUp, 0.5 * diameter);
			TieRebarPts.push_back(ptBackUp); // 拉筋水平起点
		}
		else
		{
			ptBackUp = firstTie;
			vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
			movePoint(vec, ptBackUp, 0.5 * diameter);
			TieRebarPts.push_back(ptBackUp);// 拉筋水平起点
		}

		movePoint(pt[0] - pt[1], ptStr, 1 * diameter);
		vec2 = DPoint3d::From(0, 0, -1);
		movePoint(vec2, ptStr, diameter * 1.25);
		vec = pt[1] - pt[0];
		//		movePoint(vec, ptStr, diameter + m_downDiameter);
		movePoint(vec, ptStr, diameter + m_stepDiameter);

		ptEnd = ptStr;
		vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
		movePoint(vec, ptEnd, dRebarLen);

		PIT::PITRebarCurve     rebarCurvesTmp;
		if (i > 0)
		{
			makeRebarCurve(rebarCurvesTmp, endTypes, ptStr, ptEnd); // 水平端的点
			rebarCurvesNum.push_back(rebarCurvesTmp);
		}

		vec2 = DPoint3d::From(0, 0, -1);
		dot = mdlVec_dotProduct(&vec1, &vec2);
		dHypotenuseLen = dEdgeLen / dot;

		ptStr = PtIP1;
		movePoint(vec2, ptStr, dHypotenuseLen - 1 * diameter);

		ptBackUp = PtIP1;
		vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
		movePoint(vec, ptBackUp, 0.5 * diameter);
		TieRebarPts.push_back(ptBackUp); // 拉筋中间点

		ptBackUp = ptStr;
		vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
		movePoint(vec, ptBackUp, 0.5 * diameter);
		TieRebarPts.push_back(ptBackUp); // 拉筋垂直终点

		vec = pt[0] - pt[1];
		movePoint(vec, ptStr, diameter * 1.25);
		vec = DPoint3d::From(0, 0, 1);
		//		movePoint(vec, ptStr, diameter + m_downDiameter);
		movePoint(vec, ptStr, m_stepDiameter);

		ptEnd = ptStr;
		vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
		movePoint(vec, ptEnd, dRebarLen);

		PIT::PITRebarCurve     rebarCurvesTTmp;
		makeRebarCurve(rebarCurvesTTmp, endTypes, ptStr, ptEnd); // 垂直端的点
		rebarCurvesNum.push_back(rebarCurvesTTmp);

		m_vvecRebarPts.push_back(TieRebarPts);
	}
	// end

	RebarSymbology symb;
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	int numRebar = (int)rebarCurvesNum.size();
	int j = 0;
	for (PIT::PITRebarCurve rebarCurve : rebarCurvesNum)
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

RebarSetTag* CStairsRebarAssembly::MakeRebars
(
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
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef) * 0.5;	//乘以了100
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
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef) * 0.5;	//乘以了100
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	bool const isStirrup = false;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30

	vector<CVector3D> vecEndNormal(2);
	CVector3D	endNormal;	//端部弯钩方向

	for (unsigned int i = 0; i < 2; i++)
	{
		double dRotateAngle = vecEndtype[i].rotateAngle;
		CVector3D rebarVec = m_StairsDownInfo.ptEnd - m_StairsDownInfo.ptStart;
		endNormal = m_StairsSideInfo.ptEnd - m_StairsSideInfo.ptStart;
		endNormal.Normalize();
		endNormal.Negate();

		endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转

		vecEndNormal[i] = endNormal;
	}

	vector<PIT::PITRebarCurve>     rebarCurvesNum;

	PIT::PITRebarEndType start, end;
	start.SetType((PIT::PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(vecEndtype[0].rotateAngle);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);
	start.SetstraightAnchorLen(begStraightAnchorLen);

	end.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(vecEndtype[1].rotateAngle);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);
	end.SetstraightAnchorLen(endStraightAnchorLen);

	endTypeStart.SetType(RebarEndType::kNone);
	end.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());


	PIT::PITRebarEndTypes   endTypes = { start, end };

	double startOffset = 0.00 * uor_per_mm; // 起点偏移
	double endOffset = 0.00 * uor_per_mm; // 起点偏移
	double dPos = startOffset + m_Cover;

	double spacing = 100 * uor_per_mm;		// 间距
	double adjustedXLen = m_StairsSideInfo.StepLength - m_Cover * 2 - startOffset - endOffset - diameter;
	int numRebar = (int)ceil(adjustedXLen / spacing) + 1;
	spacing = adjustedXLen / (numRebar - 1);

	double dRebarLen = m_StairsDownInfo.length - m_Cover * 2;
	for (int i = 0; i < numRebar; i++)
	{
		CPoint3D ptstr;
		CPoint3D ptend;

		ptstr = CPoint3D::From(-dRebarLen / 2.0, dPos, 0);
		ptend = CPoint3D::From(dRebarLen / 2.0, dPos, 0);

		// 矩阵变换
		EditElementHandle eeh;
		Transform trans;
		mat.AssignTo(trans);
		TransformInfo transinfo(trans);
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
		//eeh.AddToModel();
		if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
		{
			if (ptstr.z < ptend.z)//保证第一个点的Z轴在第二个点之上
			{
				CPoint3D temp;
				temp = ptend;
				ptend = ptstr;
				ptstr = temp;
			}

			DPoint3d pt[2];
			DSegment3d seg = m_vecSideXYLine.at(0);
			seg.GetStartPoint(pt[0]);
			seg.GetEndPoint(pt[1]);

			DPoint3d ptTmpStr = pt[0];
			movePoint(pt[1] - pt[0], ptTmpStr, 200 * uor_per_mm);
			DPoint3d ptTmpEnd = ptTmpStr;
			ptTmpEnd.z -= 1000 * uor_per_mm;

			DSegment3d seg_H = { ptstr, ptend };
			if (COMPARE_VALUES_EPS(ptstr.x, ptend.x, EPS) == 0)
			{
				ptTmpStr.x = ptstr.x;
				ptTmpEnd.x = ptstr.x;
			}
			else if (COMPARE_VALUES_EPS(ptstr.y, ptend.y, EPS) == 0)
			{
				ptTmpStr.y = ptstr.y;
				ptTmpEnd.y = ptstr.y;
			}
			DSegment3d segPro_V = { ptTmpStr, ptTmpEnd };

			DPoint3d ptUpStr = ptstr;
			DPoint3d ptUpEnd = ptend;

			DPoint3d ptIntersec;
			if (SUCCESS == mdlVec_intersect(&ptIntersec, &seg_H, &segPro_V))
			{
				ptstr = ptIntersec;
				CVector3D vecMove = ptend - ptstr;
				vecMove.Normalize();
				CVector3D vec = pt[1] - pt[0];
				vec.Normalize();
				// 楼梯首尾有两个缺口
				if (COMPARE_VALUES_EPS(dPos, (m_StairsSideInfo.StepLength - 550 * uor_per_mm) * 0.5, uor_per_mm) < 0 ||
					COMPARE_VALUES_EPS(dPos, (m_StairsSideInfo.StepLength - 550 * uor_per_mm) * 0.5 + 550 * uor_per_mm, uor_per_mm) > 0)
				{
					double dot = mdlVec_dotProduct(&vecMove, &vec);
					double moveLen = (35 * uor_per_mm) / dot;
					movePoint(vecMove, ptstr, moveLen, false);
					ptUpStr = ptstr;

					moveLen = (110 * uor_per_mm) / dot;
					movePoint(vecMove, ptstr, moveLen, false);
				}
				else
				{
					ptUpStr = ptstr;
					double dot = mdlVec_dotProduct(&vecMove, &vec);
					double moveLen = (30 * uor_per_mm) / dot;
					movePoint(vecMove, ptUpStr, moveLen);
				}
			}

			// 200， 550
			if (i == 0)
			{
				m_ptMainRebarDown[0] = ptstr;
				m_ptMainRebarDown[1] = ptend;

				//EditElementHandle lineEeh;
				//LineHandler::CreateLineElement(lineEeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
				//lineEeh.AddToModel();
			}
			PIT::PITRebarCurve     rebarCurves;
			endTypeEnd.SetType(RebarEndType::kNone);
			start.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());
			end.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());
			PIT::PITRebarEndTypes   endTypes = { start, end };
			makeRebarCurve(rebarCurves, endTypes, ptstr, ptend);
			rebarCurvesNum.push_back(rebarCurves);

			DPoint3d vec = m_StairsSideInfo.ptEnd - m_StairsSideInfo.ptStart; // 延侧面往上
			double moveLen = m_StairsSideInfo.minDis_Down - m_Cover - diameter * 0.5;
			moveLen -= sqrt(2.5 * (m_Cover) * (m_Cover));

			movePoint(vec, ptUpStr, moveLen);
			movePoint(vec, ptUpEnd, moveLen);

			if (i == 0)
			{
				m_ptMainRebarUp[0] = ptUpStr;
				m_ptMainRebarUp[1] = ptUpEnd;
			}

			PIT::PITRebarCurve     rebarCurvesTmp;
			endTypeEnd.SetType(RebarEndType::kHook);
			start.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());
			end.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());
			endTypes = { start, end };
			makeRebarCurve(rebarCurvesTmp, endTypes, ptUpStr, ptUpEnd);
			rebarCurvesNum.push_back(rebarCurvesTmp);
		}
		dPos += spacing;
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
	for (PIT::PITRebarCurve rebarCurve : rebarCurvesNum)
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

// 添加楼梯首尾附加钢筋
RebarSetTag* CStairsRebarAssembly::MakeAttachRebars
(
	vector<AttachRebarInfo>& vvecRebarInfo,
	vector<PIT::EndType>& vecEndtype,
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
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef) * 0.5;	//乘以了100
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
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef) * 0.5;	//乘以了100
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	bool const isStirrup = false;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30

	vector<CVector3D> vecEndNormal(2);
	CVector3D	endNormal;	//端部弯钩方向

	for (unsigned int i = 0; i < 2; i++)
	{
		double dRotateAngle = vecEndtype[i].rotateAngle;
		CVector3D rebarVec = CVector3D::From(0, 0, 0);
		endNormal = CVector3D::From(0, 0, 0);
		vecEndNormal[i] = endNormal;
	}

	vector<PIT::PITRebarCurve>     rebarCurvesNum;

	PIT::PITRebarEndType start, end;
	start.SetType((PIT::PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(vecEndtype[0].rotateAngle);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);
	start.SetstraightAnchorLen(begStraightAnchorLen);

	end.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(vecEndtype[1].rotateAngle);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);
	end.SetstraightAnchorLen(endStraightAnchorLen);

	endTypeStart.SetType(RebarEndType::kNone);
	end.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());

	PIT::PITRebarEndTypes   endTypes = { start, end };

	for (int i = 0; i < vvecRebarInfo.size(); i++)
	{
		RebarVertices  vers;
		bvector<DPoint3d> allpts;
		allpts.clear();
		for (DPoint3d pt : vvecRebarInfo.at(i).vecPoint)
		{
			allpts.push_back(pt);
		}
		GetRebarVerticesFromPoints(vers, allpts, diameter);
		PIT::PITRebarCurve rebar;
		rebar.SetVertices(vers);

		start.SetType((PIT::PITRebarEndType::Type)RebarEndType::kNone);
		end.SetType((PIT::PITRebarEndType::Type)RebarEndType::kNone);
		if (vvecRebarInfo.at(i).strEndType != RebarEndType::kNone)
		{
			endTypeStart.SetType(vvecRebarInfo.at(i).strEndType);
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false) * 0.5;	//乘以了30
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef) * 0.5;	//乘以了100

			start.SetbendLen(startbendLen);
			start.SetbendRadius(startbendRadius);
			start.SetType((PIT::PITRebarEndType::Type)vvecRebarInfo.at(i).strEndType);
			start.SetendNormal(vvecRebarInfo.at(i).vecEndNormal);
		}

		if (vvecRebarInfo.at(i).endEndType != RebarEndType::kNone)
		{
			endTypeEnd.SetType(vvecRebarInfo.at(i).endEndType);
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false) * 0.5;	//乘以了30
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef) * 0.5;	//乘以了100

			end.SetbendLen(endbendLen);
			end.SetbendRadius(endbendRadius);
			end.SetType((PIT::PITRebarEndType::Type)vvecRebarInfo.at(i).endEndType);
			end.SetendNormal(vvecRebarInfo.at(i).vecEndNormalTmp);
		}

		endTypes = { start, end };
		if (vers.GetSize() > 2)
		{
			rebar.EvaluateEndTypesStirrup(endTypes);
		}
		else if (vers.GetSize() == 2)
		{
			endTypes.beg.SetptOrgin(vvecRebarInfo.at(i).vecPoint.at(0));
			endTypes.end.SetptOrgin(vvecRebarInfo.at(i).vecPoint.at(1));
			rebar.EvaluateEndTypes(endTypes);
		}
		rebarCurvesNum.push_back(rebar);
	}

	RebarSymbology symb;
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	int numRebar = (int)rebarCurvesNum.size();
	int j = 0;
	for (PIT::PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);

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
	setdata.SetNominalSpacing(200.00);
	setdata.SetAverageSpacing(200.00);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

void CStairsRebarAssembly::CalculateTransform(CVector3D& transform, BrStringCR sizeKey, DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef); // 钢筋直径

	CVector3D	zTrans(0.0, 0.0, 0.0);

	zTrans.x = m_StairsDownInfo.length * 0.5;
	zTrans.z = m_Cover + diameter * 0.5;
	zTrans.y = diameter * 0.5;
	transform = zTrans;

}

// 附加钢筋处理
void CStairsRebarAssembly::PushAttachRebarInfo(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素
	AttachRebarInfo stAttachInfo;

	double diameter_8 = RebarCode::GetBarDiameter(L"8A", modelRef);
	double diameter_12 = RebarCode::GetBarDiameter(L"12C", modelRef);
	double diameter_10 = RebarCode::GetBarDiameter(L"10A", modelRef); // 拉筋的尺寸

	BrString sizeKeyMain = m_StairsRebarInfo.rebarSize;
	sizeKeyMain.Replace(L"mm", L"");
	double diameterMain = RebarCode::GetBarDiameter(sizeKeyMain, modelRef);

	double startOffset = 0.0 * uor_per_mm;
	double endOffset = 0.0 * uor_per_mm;
	double spacing = 100 * uor_per_mm;		// 间距
	double adjustedXLen = m_StairsSideInfo.StepLength - m_Cover * 2 - startOffset - endOffset - diameter_8;
	int numRebar = (int)ceil(adjustedXLen / spacing) + 1;
	spacing = adjustedXLen / (numRebar - 1);

	double xPos = m_Cover + diameter_8 * 0.5;

	// 头部附加钢筋
	for (int i = 0; i < numRebar; i++)
	{
		// 首尾部楼梯的缺口
		if (COMPARE_VALUES_EPS(xPos, (m_StairsSideInfo.StepLength - 550 * uor_per_mm) * 0.5, uor_per_mm) < 0 ||
			COMPARE_VALUES_EPS(xPos, (m_StairsSideInfo.StepLength - 550 * uor_per_mm) * 0.5 + 550 * uor_per_mm, uor_per_mm) > 0)
		{
			DPoint3d pt[2];
			m_vecSideXYLine.at(0).GetStartPoint(pt[0]);	// 台阶非垂直交点
			m_vecSideXYLine.at(0).GetEndPoint(pt[1]); // 台阶垂直交点

			DPoint3d ptStr = pt[0];
			CVector3D vec = pt[1] - pt[0];

			movePoint(vec, ptStr, 230 * uor_per_mm + diameter_8 * 0.5);

			vec = CVector3D::From(0, 0, -1);
			movePoint(vec, ptStr, m_Cover + diameter_8 * 0.5); // 往下走

			DPoint3d ptIndex = ptStr;
			ptIndex.z -= 1000 * uor_per_mm;

			DPoint3d ptSeg[2];
			ptSeg[0] = m_ptMainRebarDown[0];
			ptSeg[1] = m_ptMainRebarDown[1];

			// 将两条线段移到同一个平面上
			if (COMPARE_VALUES_EPS(m_ptMainRebarDown[0].x, m_ptMainRebarDown[1].x, EPS) == 0)
			{
				ptSeg[0].x = ptStr.x;
				ptSeg[1].x = ptStr.x;
			}
			else if (COMPARE_VALUES_EPS(m_ptMainRebarDown[0].y, m_ptMainRebarDown[1].y, EPS) == 0)
			{
				ptSeg[0].y = ptStr.y;
				ptSeg[1].y = ptStr.y;
			}

			DSegment3d seg_H = { ptSeg[0], ptSeg[1] };
			DSegment3d seg_V = { ptStr, ptIndex };
			DPoint3d ptIntersec;
			if (SUCCESS == mdlVec_intersect(&ptIntersec, &seg_H, &seg_V))
			{
				stAttachInfo.vecPoint.clear();

				vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
				movePoint(vec, ptIntersec, xPos); // 往里移动合适位置

				movePoint(vec, ptStr, xPos);
				movePoint(vec, ptIndex, xPos);

				stAttachInfo.vecPoint.push_back(ptStr);
				ptIntersec.z -= diameterMain * 0.5;
				stAttachInfo.vecPoint.push_back(ptIntersec);

				vec = pt[1] - pt[0];
				vec.Normalize();
				stAttachInfo.vecEndNormal = vec;
				stAttachInfo.vecEndNormalTmp = vec;
				stAttachInfo.strEndType = RebarEndType::kHook;
				stAttachInfo.endEndType = RebarEndType::kCog;

				m_vvecAttach_8mm.push_back(stAttachInfo);
			}
		}
		else
		{
			DPoint3d pt[2];
			m_vecSideXYLine.at(0).GetStartPoint(pt[0]);	// 台阶非垂直交点
			m_vecSideXYLine.at(0).GetEndPoint(pt[1]); // 台阶垂直交点

			DPoint3d ptStr = pt[0];
			CVector3D vec = pt[1] - pt[0];

			movePoint(vec, ptStr, 230 * uor_per_mm + diameter_8 * 0.5);

			vec = CVector3D::From(0, 0, -1);
			movePoint(vec, ptStr, m_Cover + diameter_8 * 0.5); // 往下走

			DPoint3d ptIndex = ptStr;
			ptIndex.z -= 1000 * uor_per_mm;

			DPoint3d ptSeg[2];
			ptSeg[0] = m_ptMainRebarDown[0];
			ptSeg[1] = m_ptMainRebarDown[1];

			// 将两条线段移到同一个平面上
			if (COMPARE_VALUES_EPS(m_ptMainRebarDown[0].x, m_ptMainRebarDown[1].x, EPS) == 0)
			{
				ptSeg[0].x = ptStr.x;
				ptSeg[1].x = ptStr.x;
			}
			else if (COMPARE_VALUES_EPS(m_ptMainRebarDown[0].y, m_ptMainRebarDown[1].y, EPS) == 0)
			{
				ptSeg[0].y = ptStr.y;
				ptSeg[1].y = ptStr.y;
			}

			DSegment3d seg_H = { ptSeg[0], ptSeg[1] };
			DSegment3d seg_V = { ptStr, ptIndex };
			DPoint3d ptIntersec;
			if (SUCCESS == mdlVec_intersect(&ptIntersec, &seg_H, &seg_V))
			{
				DPoint3d vecTmp1 = m_ptMainRebarDown[1] - m_ptMainRebarDown[0];
				DPoint3d vecTmp2 = m_vecTailPoint.at(4) - m_vecTailPoint.at(3);
				double dotAngle = mdlVec_dotProduct(&vecTmp1, &vecTmp2);

				stAttachInfo.vecPoint.clear();

				vec = m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart;
				movePoint(vec, ptIntersec, xPos); // 往里移动合适位置

				movePoint(vec, ptStr, xPos);
				movePoint(vec, ptIndex, xPos);

				stAttachInfo.vecPoint.push_back(ptStr);
				ptIntersec.z -= diameterMain * 0.5;
				stAttachInfo.vecPoint.push_back(ptIntersec);

				CVector3D vec1 = pt[1] - pt[0];
				vec1.Normalize();
				CVector3D vec2 = m_ptMainRebarDown[1] - m_ptMainRebarDown[0];
				vec2.Normalize();

				double dot = mdlVec_dotProduct(vec1, vec2);
				double moveLen = 0.00;

				moveLen = (m_StairsSideInfo.StepWidth * 1.5 - m_Cover * 2) / dot;
				CVector3D moveVec = m_ptMainRebarDown[1] - m_ptMainRebarDown[0];

				moveVec.Normalize();
				movePoint(moveVec, ptIntersec, moveLen);
				stAttachInfo.vecPoint.push_back(ptIntersec);

				DPoint3d ptTemp = ptIntersec;
				movePoint(moveVec, ptTemp, 302.65491900843 * uor_per_mm);

				dot = 300.0 / 302.65491900843;

				CVector3D vecDot = m_ptMainRebarDown[1] - m_ptMainRebarDown[0];
				CVector3D vecCorss = m_StairsSideInfo.ptEnd - m_StairsSideInfo.ptStart;
				vecDot.Normalize();
				vecCorss.Normalize();
				CVector3D dotVec = vecDot.Perpendicular(vecCorss);
				CMatrix3D mat = CMatrix3D::Rotate(ptIntersec, acos(dot), dotVec);

				Transform trans;
				mat.AssignTo(trans);
				TransformInfo transinfo(trans);
				EditElementHandle eehLine;
				LineHandler::CreateLineElement(eehLine, nullptr, DSegment3d::From(ptIntersec, ptTemp), true, *ACTIVEMODEL);
				eehLine.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehLine, transinfo);

				DPoint3d pt1[2];
				mdlLinear_extract(pt1, NULL, eehLine.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改
				stAttachInfo.vecPoint.push_back(pt1[1]);

				vec = pt[1] - pt[0];
				vec.Normalize();
				stAttachInfo.vecEndNormal = vec;
				stAttachInfo.vecEndNormalTmp = CVector3D::FromZero();
				stAttachInfo.strEndType = RebarEndType::kHook;
				stAttachInfo.endEndType = RebarEndType::kNone;

				m_vvecAttach_8mm.push_back(stAttachInfo);
			}
		}

		xPos += spacing;
	}

	// 尾部附加钢筋
	xPos = diameter_8 * 0.5;
	for (int i = 0; i < numRebar; i++)
	{
		// 首尾部楼梯的缺口
		if (COMPARE_VALUES_EPS(xPos, (m_StairsSideInfo.StepLength - 550 * uor_per_mm) * 0.5, uor_per_mm) < 0 ||
			COMPARE_VALUES_EPS(xPos, (m_StairsSideInfo.StepLength - 550 * uor_per_mm) * 0.5 + 550 * uor_per_mm, uor_per_mm) > 0)
		{
			if (m_vecTailPoint.size() > 5)
			{
				stAttachInfo.vecPoint.clear();
				DPoint3d ptStr = m_vecTailPoint.at(2);
				movePoint(m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart, ptStr, xPos);

				DPoint3d pt[2];
				m_vecSideXYLine.at(0).GetStartPoint(pt[0]);	// 台阶非垂直交点
				m_vecSideXYLine.at(0).GetEndPoint(pt[1]); // 台阶垂直交点

				movePoint(pt[1] - pt[0], ptStr, diameter_8 * 0.5 + diameterMain * 0.5);
				movePoint(CVector3D::kZaxis, ptStr, diameterMain);

				stAttachInfo.vecPoint.push_back(ptStr);

				DPoint3d ptTemp = m_vecTailPoint.at(3);
				movePoint(m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart, ptTemp, xPos);
				movePoint(pt[1] - pt[0], ptTemp, diameter_8 * 0.5 + diameterMain * 0.5);
				movePoint(CVector3D::From(0, 0, -1), ptTemp, diameterMain);
				stAttachInfo.vecPoint.push_back(ptTemp);

				stAttachInfo.strEndType = RebarEndType::kHook;
				stAttachInfo.endEndType = RebarEndType::kCog;
				CVector3D vec = pt[0] - pt[1];
				vec.Normalize();
				stAttachInfo.vecEndNormal = vec;
				stAttachInfo.vecEndNormalTmp = vec;

				m_vvecAttach_8mm.push_back(stAttachInfo);

				stAttachInfo.vecPoint.clear();

				ptStr = m_vecTailPoint.at(5);
				movePoint(CVector3D::kZaxis, ptStr, diameter_12 * 0.5 + diameter_10 * 0.5);
				movePoint(pt[0] - pt[1], ptStr, diameterMain * 1.3);
				movePoint(m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart, ptStr, xPos);

				stAttachInfo.vecPoint.push_back(ptStr);

				ptStr = m_vecTailPoint.at(0);
				movePoint(CVector3D::kZaxis, ptStr, diameter_12 * 0.5 + diameter_10 * 0.5);
				movePoint(pt[1] - pt[0], ptStr, diameter_12 * 1.5);
				movePoint(m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart, ptStr, xPos);
				stAttachInfo.vecPoint.push_back(ptStr);

				movePoint(CVector3D::From(0, 0, -1), ptStr, 30 * uor_per_mm);
				stAttachInfo.vecPoint.push_back(ptStr);

				DVec3d referVec = m_ptMainRebarDown[1] - m_ptMainRebarDown[0];
				DVec3d negaVec = m_ptMainRebarDown[0] - m_ptMainRebarDown[1];
				DPoint3d vec1 = m_ptMainRebarDown[1] - m_ptMainRebarDown[0];
				DPoint3d vec2 = m_vecTailPoint.at(4) - m_vecTailPoint.at(3);
				double dotAngle = mdlVec_dotProduct(&vec1, &vec2);
				if (dotAngle > 0) //夹角<90
				{
					referVec = m_ptMainRebarDown[0] - m_ptMainRebarDown[1];
					negaVec = m_ptMainRebarDown[1] - m_ptMainRebarDown[0];
				}

				ptTemp = m_vecTailPoint.at(4);
				movePoint(m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart, ptTemp, xPos);
				//movePoint(m_ptMainRebarDown[1] - m_ptMainRebarDown[0], ptTemp, diameterMain * 0.5);
				movePoint(referVec, ptTemp, diameterMain * 0.5);
				stAttachInfo.vecPoint.push_back(ptTemp);

				movePoint(m_ptMainRebarDown[0] - m_ptMainRebarDown[1], ptTemp, m_StairsSideInfo.StepWidth * 2.5);
				stAttachInfo.vecPoint.push_back(ptTemp);

				DPoint3d ptIndex = ptTemp;
				movePoint(m_ptMainRebarDown[0] - m_ptMainRebarDown[1], ptIndex, 155.2417469626 * uor_per_mm);

				double dot = 150 / 155.2417469626;

				//CVector3D vecDot = m_ptMainRebarDown[0] - m_ptMainRebarDown[1];
				CVector3D vecDot = negaVec;
				CVector3D vecCorss = m_StairsSideInfo.ptEnd - m_StairsSideInfo.ptStart;
				vecDot.Normalize();
				vecCorss.Normalize();
				CVector3D dotVec = vecDot.Perpendicular(vecCorss);
				double angle = acos(dot);
				CMatrix3D mat = CMatrix3D::Rotate(ptTemp, angle, dotVec);

				Transform trans;
				mat.AssignTo(trans);
				TransformInfo transinfo(trans);
				EditElementHandle eehLine;
				LineHandler::CreateLineElement(eehLine, nullptr, DSegment3d::From(ptTemp, ptIndex), true, *ACTIVEMODEL);
				eehLine.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehLine, transinfo);

				DPoint3d pt1[2];
				mdlLinear_extract(pt1, NULL, eehLine.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改
				//eehLine.AddToModel();

				if (dotAngle < 0) //夹角>90
				{
					stAttachInfo.vecPoint.push_back(pt1[1]);
				}
				//EditElementHandle shapeEeh;
				//DPoint3d pts[6] = { stAttachInfo.vecPoint.at(0), stAttachInfo.vecPoint.at(1), stAttachInfo.vecPoint.at(2),
				//stAttachInfo.vecPoint.at(3), stAttachInfo.vecPoint.at(4), stAttachInfo.vecPoint.at(5) };
				//ShapeHandler::CreateShapeElement(shapeEeh, nullptr, pts, 6, true, *modelRef);
				//shapeEeh.AddToModel();
				stAttachInfo.strEndType = RebarEndType::kHook;
				stAttachInfo.endEndType = RebarEndType::kNone;
				stAttachInfo.vecEndNormal = CVector3D::From(0, 0, -1);
				stAttachInfo.vecEndNormalTmp = CVector3D::From(0, 0, 0);
				m_vvecAttach_10mm.push_back(stAttachInfo);
			}
		}
		else
		{
			if (m_vecTailPoint.size() > 5)
			{
				DPoint3d vec1 = m_ptMainRebarDown[1] - m_ptMainRebarDown[0];
				DPoint3d vec2 = m_vecTailPoint.at(4) - m_vecTailPoint.at(3);
				double dotAngle = mdlVec_dotProduct(&vec1, &vec2);

				stAttachInfo.vecPoint.clear();
				DPoint3d ptStr = m_vecTailPoint.at(2);
				movePoint(m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart, ptStr, xPos);

				DPoint3d pt[2];
				m_vecSideXYLine.at(0).GetStartPoint(pt[0]);	// 台阶非垂直交点
				m_vecSideXYLine.at(0).GetEndPoint(pt[1]); // 台阶垂直交点

				movePoint(pt[1] - pt[0], ptStr, diameter_8 * 0.5 + diameterMain * 0.5);
				movePoint(CVector3D::kZaxis, ptStr, diameterMain);

				stAttachInfo.vecPoint.push_back(ptStr);

				DPoint3d ptTemp = m_vecTailPoint.at(3);
				movePoint(m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart, ptTemp, xPos);
				movePoint(pt[1] - pt[0], ptTemp, diameter_8 * 0.5 + diameterMain * 0.5);
				movePoint(CVector3D::From(0, 0, -1), ptTemp, diameterMain * 0.5);
				stAttachInfo.vecPoint.push_back(ptTemp);

				if (dotAngle > 0) //夹角<90
				{
					DPoint3d tmpPt = ptTemp; tmpPt.z += m_StairsDownInfo.height;
					DSegment3d seg1 = DSegment3d::From(ptTemp, tmpPt);
					DSegment3d seg2 = DSegment3d::From(m_ptMainRebarDown[0], m_ptMainRebarDown[1]);
					mdlVec_intersect(&ptTemp, &seg1, &seg2);
					movePoint(m_ptMainRebarDown[0] - m_ptMainRebarDown[1], ptTemp, m_StairsSideInfo.StepWidth * 0.5);
					stAttachInfo.vecPoint.push_back(ptTemp);
				}
				else
				{
					ptTemp = m_vecTailPoint.at(4);
					movePoint(m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart, ptTemp, xPos);
					movePoint(m_ptMainRebarDown[1] - m_ptMainRebarDown[0], ptTemp, diameterMain * 0.5);
					stAttachInfo.vecPoint.push_back(ptTemp);

				}

				movePoint(m_ptMainRebarDown[0] - m_ptMainRebarDown[1], ptTemp, m_StairsSideInfo.StepWidth * 2.2);
				stAttachInfo.vecPoint.push_back(ptTemp);

				DPoint3d ptIndex = ptTemp;
				movePoint(m_ptMainRebarDown[0] - m_ptMainRebarDown[1], ptIndex, 107.703 * uor_per_mm);

				double dot = 100 / 107.703;

				CVector3D vecDot = m_ptMainRebarDown[0] - m_ptMainRebarDown[1];
				CVector3D vecCorss = m_StairsSideInfo.ptEnd - m_StairsSideInfo.ptStart;
				vecDot.Normalize();
				vecCorss.Normalize();
				CVector3D dotVec = vecDot.Perpendicular(vecCorss);
				dotVec.Normalize();
				CMatrix3D mat = CMatrix3D::Rotate(ptTemp, acos(dot), dotVec);

				Transform trans;
				mat.AssignTo(trans);
				TransformInfo transinfo(trans);
				EditElementHandle eehLine;
				LineHandler::CreateLineElement(eehLine, nullptr, DSegment3d::From(ptTemp, ptIndex), true, *ACTIVEMODEL);
				eehLine.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehLine, transinfo);

				DPoint3d pt1[2];
				mdlLinear_extract(pt1, NULL, eehLine.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改

				stAttachInfo.vecPoint.push_back(pt1[1]);
				stAttachInfo.strEndType = RebarEndType::kHook;
				stAttachInfo.endEndType = RebarEndType::kNone;

				CVector3D vec = pt[0] - pt[1];
				vec.Normalize();
				stAttachInfo.vecEndNormal = vec;
				stAttachInfo.vecEndNormalTmp = CVector3D::From(0, 0, 0);

				m_vvecAttach_8mm.push_back(stAttachInfo);

				stAttachInfo.vecPoint.clear();
				ptStr = m_vecTailPoint.at(0);
				movePoint(CVector3D::kZaxis, ptStr, diameter_12 * 0.5 + diameter_10 * 0.5);
				movePoint(pt[1] - pt[0], ptStr, diameter_12 * 1.5);
				movePoint(m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart, ptStr, xPos);

				DPoint3d ptEnd = m_vecTailPoint.at(5);
				movePoint(CVector3D::kZaxis, ptEnd, diameter_12 * 0.5 + diameter_10 * 0.5);
				movePoint(pt[0] - pt[1], ptEnd, diameterMain * 1.5);
				movePoint(m_StairsSideInfo.ptBackStart - m_StairsSideInfo.ptFrontStart, ptEnd, xPos);
				stAttachInfo.vecPoint.push_back(ptStr);
				stAttachInfo.vecPoint.push_back(ptEnd);
				stAttachInfo.strEndType = RebarEndType::kHook;
				stAttachInfo.endEndType = RebarEndType::kHook;
				stAttachInfo.vecEndNormal = CVector3D::From(0, 0, -1);
				stAttachInfo.vecEndNormalTmp = CVector3D::From(0, 0, -1);
				m_vvecAttach_10mm.push_back(stAttachInfo);
			}
		}

		xPos += spacing;
	}
}

bool CStairsRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素

	RebarSetTag* tag = NULL;

	m_vvecAttach_8mm.clear();
	m_vvecAttach_10mm.clear();
	m_vvecAttach_12mm.clear();
	m_vecTailPoint.clear();

	//SetCover(30.0 * uor_per_mm);
	SetCover(m_StairsRebarInfo.StairsCover * uor_per_mm);
	m_vecSetId.resize(6);
	m_vecSetId[0] = 0;
	m_vecSetId[1] = 0;
	m_vecSetId[2] = 0;
	m_vecSetId[3] = 0;
	m_vecSetId[4] = 0;
	m_vecSetId[5] = 0;

	BrString sizeKey = m_StairsRebarInfo.rebarSize;
	sizeKey.Replace(L"mm", L"");
	m_stepDiameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	CMatrix3D   mat;
	//BrString sizeKey = "12";
	//BrString sizeStepKey = "6";
	//m_downDiameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	//m_stepDiameter = RebarCode::GetBarDiameter(sizeStepKey, modelRef);

	PIT::EndType strEndType;
	PIT::EndType endEndType;
	strEndType.endType = RebarEndType::kNone;
	strEndType.rotateAngle = 0.00;
	strEndType.offset = 0.00;

	endEndType.endType = RebarEndType::kNone;
	endEndType.rotateAngle = 0.00;
	endEndType.offset = 0.00;

	vector<PIT::EndType> vecEndtype;
	vecEndtype.push_back(strEndType);
	vecEndtype.push_back(endEndType);

	CVector3D	transform(0.0, 0.0, 0.0);
	CalculateTransform(transform, sizeKey, modelRef);

	mat.SetTranslation(transform);
	mat = GetPlacement() * mat;

	vecEndtype[0].endType = 6;
	vecEndtype[1].endType = 6;
	tag = MakeRebars(vecEndtype, mat, m_vecSetId[0], sizeKey, modelRef); //侧面由上至下的钢筋
	if (NULL != tag)
	{
		tag->SetBarSetTag(1);
		rsetTags.Add(tag);
	}

	vecEndtype[0].endType = 6;
	vecEndtype[1].endType = 6;
	sizeKey = "10A";
	tag = MakeStepRebars(vecEndtype, m_vecSetId[1], sizeKey, modelRef); //点筋 从侧面方向看
	if (NULL != tag)
	{
		tag->SetBarSetTag(2);
		rsetTags.Add(tag);
	}

	//10mm 一级钢
	vecEndtype[0].endType = 6;
	vecEndtype[1].endType = 6;
	sizeKey = "10A";
	tag = MakeTieRebars(vecEndtype, m_vvecRebarPts, m_vecSetId[2], sizeKey, modelRef);//拉筋
	if (NULL != tag)
	{
		tag->SetBarSetTag(3);
		rsetTags.Add(tag);
	}

	// 添加首尾一系列附加钢筋
	PushAttachRebarInfo(modelRef);

	// 添加首尾两端附加钢筋的信息
	//8mm 一级钢
	vecEndtype[0].endType = RebarEndType::kNone;
	vecEndtype[1].endType = RebarEndType::kNone;
	sizeKey = "8A";
	tag = MakeAttachRebars(m_vvecAttach_8mm, vecEndtype, m_vecSetId[3], sizeKey, modelRef); // 8mm附加钢筋
	if (NULL != tag)
	{
		tag->SetBarSetTag(4);
		rsetTags.Add(tag);
	}

	// 添加首尾两端附加钢筋的信息
	//10mm 一级钢
	vecEndtype[0].endType = RebarEndType::kNone;
	vecEndtype[1].endType = RebarEndType::kNone;
	sizeKey = "10A";
	tag = MakeAttachRebars(m_vvecAttach_10mm, vecEndtype, m_vecSetId[4], sizeKey, modelRef); // 10mm附加钢筋
	if (NULL != tag)
	{
		tag->SetBarSetTag(4);
		rsetTags.Add(tag);
	}

	// 添加首尾两端附加钢筋的信息
	//12mm 三级钢
	vecEndtype[0].endType = RebarEndType::kHook;
	vecEndtype[1].endType = RebarEndType::kHook;
	sizeKey = "12C";
	tag = MakeAttachRebars(m_vvecAttach_12mm, vecEndtype, m_vecSetId[5], sizeKey, modelRef); // 12mm附加钢筋
	if (NULL != tag)
	{
		tag->SetBarSetTag(4);
		rsetTags.Add(tag);
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

bool CStairsRebarAssembly::Rebuild()
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

bool CStairsRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	SetSelectedModel(ehSel.GetModelRef());

	DgnModelRefP modelRef = ACTIVEMODEL;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_pStarisRebarDlg = new CStarisRebarDlog(ehSel);
	m_pStarisRebarDlg->Create(IDD_DIALOG_Stairs);
	m_pStarisRebarDlg->ShowWindow(SW_SHOW);
	m_pStarisRebarDlg->SetRebarAssembly(this);
	return true;
}

long CStairsRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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