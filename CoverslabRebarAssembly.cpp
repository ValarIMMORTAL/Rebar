#include "_ustation.h"
#include "resource.h"
#include <SelectionRebar.h>
#include "CoverslabRebarAssembly.h"
#include "CoverslabRebarDlg.h"
#include "ExtractFacesTool.h"
#include "SelectRebarTool.h"
#include "TieRebar.h"
#include "ElementAttribute.h"
#include "PITMSCECommon.h"
#include "BentlyCommonfile.h"
#include "PITRebarCurve.h"
#include "CBeamRebarAssembly.h"
#include "XmlHelper.h"

int cover_direction;
ElementHandle m_eeh1;
CoverslabRebarAssembly::CoverslabRebarAssembly(ElementId id, DgnModelRefP modelRef) :     // 构造函数初始化一些值
	PIT::PITRebarAssembly(id, modelRef),
	m_PositiveCover(30),                          //正面保护层
	m_ReverseCover(30),                           //反面保护层
	m_SideCover(30),                              //反面保护层
	m_RebarLevelNum(3)                            //钢筋层数
{
	Init();
}

void CoverslabRebarAssembly::Init()            //重新指定容器的长度为m_RebarLevelNum
{
	m_vecDir.resize(m_RebarLevelNum);            //方向,0表示x轴，1表示z轴
	m_vecDirSize.resize(m_RebarLevelNum);         //尺寸
	m_vecDirSpacing.resize(m_RebarLevelNum);       //间隔
	m_vecRebarType.resize(m_RebarLevelNum);        //钢筋类型
	m_vecStartOffset.resize(m_RebarLevelNum);       //起点偏移
	m_vecEndOffset.resize(m_RebarLevelNum);         //终点偏移
	m_vecLevelSpace.resize(m_RebarLevelNum);       //与前层间距
	m_vecSetId.resize(m_RebarLevelNum);            //SetId
	int twinRebarLevel = 0;

	//根据需求并筋需要设置不一样的层
	//for (size_t i = 0; i < GetvecTwinRebarLevel().size(); i++)
	//{
	//	if (GetvecTwinRebarLevel().at(i).hasTwinbars)
	//	{
	//		twinRebarLevel++;
	//	}
	//}
	//m_vecSetId.resize(m_RebarLevelNum + twinRebarLevel);
	for (size_t i = 0; i < m_vecSetId.size(); i++)
	{
		m_vecSetId[i] = 0;
	}
}

void CoverslabRebarAssembly::SetConcreteData(Concrete const& concreteData)    //设置三个保护层信息和层数
{
	m_PositiveCover = concreteData.postiveCover;
	m_ReverseCover = concreteData.reverseCover;
	m_SideCover = concreteData.sideCover;
	m_RebarLevelNum = concreteData.rebarLevelNum;
}

void CoverslabRebarAssembly::SetRebarData(vector<PIT::ConcreteRebar> const& vecData)     //用vector数组存每层钢筋的信息
{
	if (vecData.empty())
	{
		return;
	}
	if (vecData.size() != m_RebarLevelNum)
	{
		return;
	}

	for (size_t i = 0; i < vecData.size(); i++)
	{
		if (i < m_vecDir.size())
		{
			m_vecDir[i] = vecData[i].rebarDir;                       //方向
			m_vecDirSize[i] = vecData[i].rebarSize;                   //钢筋尺寸
			m_vecRebarType[i] = vecData[i].rebarType;                 //钢筋型号
			m_vecDirSpacing[i] = vecData[i].spacing;                //钢筋间距
			m_vecStartOffset[i] = vecData[i].startOffset;           //起点偏移
			m_vecEndOffset[i] = vecData[i].endOffset;              //终点偏移
			m_vecLevelSpace[i] = vecData[i].levelSpace;           //钢筋层间隔
		}
		else                                                    //减少
		{
			m_vecDir.push_back(vecData[i].rebarDir);
			m_vecDirSize.push_back(vecData[i].rebarSize);
			m_vecRebarType.push_back(vecData[i].rebarType);
			m_vecDirSpacing.push_back(vecData[i].spacing);
			m_vecStartOffset.push_back(vecData[i].startOffset);
			m_vecEndOffset.push_back(vecData[i].endOffset);
			m_vecLevelSpace.push_back(vecData[i].levelSpace);
			m_vecSetId.push_back(0);
		}
	}

}

bool CoverslabRebarAssembly::IsSmartSmartFeature(EditElementHandle& eeh)
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

void CoverslabRebarAssembly::SetRebarEndTypes(vector<PIT::EndType> const & vecEndTypes)       //处理端部
{
	// 	if (vecEndTypes.empty())
	// 	{
	// 		m_vvecEndType = { { {0,0,0} }, { {0,0,0} } };
	// 		return;
	// 	}
	if (vecEndTypes.size())                   //设置端部，如果有东西清空
		m_vvecEndType.clear();

	vector<PIT::EndType> vec;
	vec.reserve(2);
	for (size_t i = 0; i < vecEndTypes.size(); i++)
	{
		if (i & 0x01)
		{
			vec.push_back(vecEndTypes[i]);
			m_vvecEndType.push_back(vec);
			vec.clear();
		}
		else
		{
			vec.push_back(vecEndTypes[i]);
		}
	}
}

void CoverslabRebarAssembly::InitRebarSetId()
{
	int twinRebarLevel = 0;
	//根据需求并筋需要设置不一样的层
	//for (size_t i = 0; i < m_vecTwinRebarLevel.size(); i++)
	//{
	//	if (m_vecTwinRebarLevel[i].hasTwinbars)
	//	{
	//		twinRebarLevel++;
	//	}
	//}

	if (m_vecSetId.size() != m_RebarLevelNum + twinRebarLevel)
	{
		m_vecSetId.resize(m_RebarLevelNum + twinRebarLevel);
		for (size_t i = 0; i < m_vecSetId.size(); ++i)
			m_vecSetId[i] = 0;
	}
}

void CoverslabRebarAssembly::GetConcreteData(Concrete& concreteData)             //获得三个保护层信息和层数信息
{
	concreteData.postiveCover = m_PositiveCover;
	concreteData.reverseCover = m_ReverseCover;
	concreteData.sideCover = m_SideCover;
	concreteData.rebarLevelNum = m_RebarLevelNum;
	//	concreteData.isTwinbars = m_Twinbars;
}

void CoverslabRebarAssembly::GetRebarData(vector<PIT::ConcreteRebar>& vecData) const
{
	if (0 == m_RebarLevelNum || m_vecDir.empty() || m_vecDirSize.empty() || m_vecDirSpacing.empty())
	{
		return;
	}
	for (size_t i = 0; i < m_RebarLevelNum; i++)
	{
		ConcreteRebar rebarData;
		// 		rebarData.postiveCover = m_PositiveCover;
		// 		rebarData.reverseCover = m_ReverseCover;
		// 		rebarData.reverseCover = m_SideCover;
		rebarData.rebarDir = m_vecDir[i];
		strcpy(rebarData.rebarSize, m_vecDirSize[i]);
		rebarData.rebarType = m_vecRebarType[i];
		rebarData.spacing = m_vecDirSpacing[i];
		rebarData.startOffset = m_vecStartOffset[i];
		rebarData.endOffset = m_vecEndOffset[i];
		rebarData.levelSpace = m_vecLevelSpace[i];
		vecData.push_back(rebarData);
	}
}

void CoverslabRebarAssembly::SetTieRebarInfo(TieReBarInfo const & tieRebarInfo)
{
	m_tieRebarInfo = tieRebarInfo;
}

const TieReBarInfo CoverslabRebarAssembly::GetTieRebarInfo() const
{
	return m_tieRebarInfo;
}

CoverslabRebarAssembly::CoverSlabType CoverslabRebarAssembly::JudgeSlabType(ElementHandleCR eh)
{
	//	return STWALL;
		//通过分解元素来判断元素的类型
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;
	EditElementHandle testeeh(eh, false);
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	if (!EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs))
	{
		return CoverSlabType::OtherCoverSlab;
	}
	for (int j = 0; j < Holeehs.size(); j++)
	{
		delete Holeehs.at(j);
		Holeehs.at(j) = nullptr;
	}
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine, vecDownBackLine, NULL);

	if (vecDownFaceLine.empty())
		return OtherCoverSlab;
	ISolidKernelEntityPtr entityPtr;
	if (SolidUtil::Convert::ElementToBody(entityPtr, eh) == SUCCESS)
	{
		bvector<ISubEntityPtr> subEntities;
		size_t iSubEntityNum = SolidUtil::GetBodyFaces(&subEntities, *entityPtr);
		size_t iSize = subEntities.size();
		if (6 == iSize)
			return SICoverSlab;
		else if (12 == iSize)
			return SZCoverSlab;
		else if (11 == iSize)
			return STCoverSlab;
		else if (10 == iSize)
			return STCoverSlab_Ten;
		else
			return OtherCoverSlab;
	}
	return OtherCoverSlab;
}

void SICoverslabRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
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

bool SICoverslabRebarAssembly::makeRebarCurve
(
	PITRebarCurve&				rebar,
	PITRebarEndTypes&			endTypes,
	const vector<PIT::EndType>&	vecEndtype,
	vector<DPoint3d>&			vecPoint,
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

bool SICoverslabRebarAssembly::makeRebarCurve
(
	vector<PITRebarCurve>&     rebars,
	double                  xPos,
	double                  yLen,
	double					startOffset,
	double					endOffset,
	double					bendRadius,
	const vector<PIT::EndType>&	vecEndtype,
	PITRebarEndTypes&		endTypes,
	CMatrix3D const&        mat,
	bool isTwin
)
{
	CPoint3D  startPt;
	CPoint3D  endPt;

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	if (1 == GetvecDir().at(cover_direction))
	{
		startPt = CPoint3D::From(xPos, -yLen / 2.0 + startOffset, 0.0);
		endPt = CPoint3D::From(xPos, yLen / 2.0 - endOffset, 0.0);
	}
	else
	{
		startPt = CPoint3D::From(xPos, -yLen / 2.0 + startOffset, 0.0);
		endPt = CPoint3D::From(xPos, yLen / 2.0, 0.0);
	}

	
	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	//eeh.AddToModel();

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

	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}
	if (!isTwin)
	{
		m_vecRebarPtsLayer.push_back(pt1[0]);
		m_vecRebarPtsLayer.push_back(pt1[1]);
	}
	

	// EditElementHandle eeh2;
	// GetContractibleeeh(eeh2);//获取减去保护层的端部缩小的实体

	double dSideCover = GetSideCover()*uor_per_mm;
	vector<DPoint3d> tmppts;
	vector<DPoint3d> tmpptsTmp;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
//	GetIntersectPointsWithOldElm(tmpptsTmp, m_pOldElm, pt1[0], pt1[1], dSideCover, matrix);
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);

	map<int, DPoint3d> map_pts;
	for (DPoint3d pt : tmppts)
	{
		int dis = (int)pt1[0].Distance(pt);
		if (dis == 0 || dis == (int)pt1[0].Distance(pt1[1]))//防止过滤掉起点和终点
		{
			dis = dis + 1;
		}

		map_pts[dis] = pt;
	}
	map_pts[0] = pt1[0];
	map_pts[(int)pt1[0].Distance(pt1[1])] = pt1[1];

	RebarVertices  vers;
	bvector<DPoint3d> allpts;

	if (0 == GetvecDir().at(cover_direction))
	{
		DPoint3d temp;
	
		double dPositiveCover = GetPositiveCover()*uor_per_mm;//下
		double dReverseCover = GetReverseCover()*uor_per_mm;//上
		double dSideCover = GetSideCover()*uor_per_mm;//侧

		CVector3D  tempVec(pt1[0], pt1[1]);
		CVector3D  arcVec = tempVec;
		arcVec.Normalize();
		arcVec.ScaleToLength( m_diameter1*0.5);
		(pt1[1]).Add(arcVec);

		temp = pt1[0];

		double dPinRadius = 0.00;
		if (GetvvecEndType().at(2).at(0).endType != 0 || GetvvecEndType().at(2).at(1).endType != 0)
		{
			WString strSizeTmp1 = GetvecDirSize().at(2);
			if (strSizeTmp1.find(L"mm") != WString::npos)
			{
				strSizeTmp1.ReplaceAll(L"mm", L"");
			}
			dPinRadius = RebarCode::GetPinRadius(strSizeTmp1, ACTIVEMODEL, false) * 0.5 + RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) * 0.5;
			dPinRadius *= 2;
		}

		temp.z += (m_SICoverSlabData.height - dReverseCover - dPositiveCover - m_diameter1 - dPinRadius);// 弯曲长度=减去钢筋直接和上面保护层
		allpts.push_back(temp);

		arcVec.Normalize();
		arcVec.ScaleToLength(pt1[1].Distance(pt1[0]) - m_diameter1);
		temp.Add(arcVec);
		allpts.push_back(temp);

		temp.z -= (m_SICoverSlabData.height - dReverseCover - dPositiveCover - m_diameter1 - dPinRadius);// 弯曲长度=减去钢筋直接和上面保护层
		allpts.push_back(temp);

		arcVec.Normalize();
		arcVec.Negate();
		double dLength = 0.00;
		if (COMPARE_VALUES_EPS(endOffset, 0.00, EPS) > 0)
		{
			dLength = pt1[1].Distance(pt1[0]) - m_diameter1 * 1.0;
		}
		else
		{
			dLength = pt1[1].Distance(pt1[0]) - m_diameter1 * 1.5;
		}
		arcVec.ScaleToLength(dLength);
		temp.Add(arcVec);
		allpts.push_back(temp);

		temp.z += (m_SICoverSlabData.height - dReverseCover - dPositiveCover - m_diameter1 * 0.5 - dPinRadius - endOffset);// 弯曲长度=减去钢筋直接和上面保护层
		allpts.push_back(temp);

		//vector<DPoint3d> vecPoint;
		//for (DPoint3d pt : allpts)
		//{
		//	vecPoint.push_back(pt);
		//}

		//endTypes.beg.SetptOrgin(vecPoint[0]);
		//endTypes.end.SetptOrgin(vecPoint[vecPoint.size() - 1]);

		RebarVertices  versTmp;
		GetRebarVerticesFromPoints(versTmp, allpts, m_diameter1);
		PITRebarCurve rebarTmp;
		rebarTmp.SetVertices(versTmp);
		if (allpts.size() > 2)
		{
			CVector3D vecTmp = allpts[1] - allpts[0];
			vecTmp.Normalize();
			endTypes.end.SetendNormal(vecTmp);
		}
		rebarTmp.EvaluateEndTypesStirrup(endTypes);
		rebars.push_back(rebarTmp);

		//PITRebarCurve rebarTmp;
		//makeRebarCurve(rebarTmp, endTypes, vecEndtype, vecPoint, bendRadius);
		//rebarTmp.EvaluateEndTypesStirrup(endTypes);
		//rebars.push_back(rebarTmp);
	}
	else
	{	
		allpts.push_back(pt1[0]);
		allpts.push_back(pt1[1]);

		endTypes.beg.SetptOrgin(allpts[0]);
		endTypes.end.SetptOrgin(allpts[allpts.size() - 1]);

		GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
		PITRebarCurve rebar;
		rebar.SetVertices(vers);
		rebar.EvaluateEndTypes(endTypes);

		rebars.push_back(rebar);
	}

	return true;
}

RebarSetTag* SICoverslabRebarAssembly::MakeRebars
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	double              xLen,
	double              width,
	double              spacing,
	double              startOffset,
	double              endOffset,

	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
	CMatrix3D const&    mat,
	DgnModelRefP        modelRef
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

	RebarEndType endTypeStart, endTypeEnd;
	
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
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
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
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}
	//	}//由于板配筋和墙配筋方向不同改保护层正反侧面
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	m_diameter1 = diameter;
//	double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false) * 0.5;	//乘以了30
//	double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);	//乘以了30
	double startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef) * 0.5;	//乘以了100
//	double startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//乘以了100
	double endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef) * 0.5;	//乘以了100
//	double endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//乘以了100
	int numRebar = 0;
	double adjustedXLen, adjustedSpacing;

	double allSideCov = GetSideCover() * uor_per_mm * 2;

	if(1== GetvecDir().at(cover_direction))
		adjustedXLen = xLen - allSideCov - startOffset - endOffset - diameter;
	else
		adjustedXLen = xLen - allSideCov - diameter - startOffset - endOffset;
	numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);

	double xPos = startOffset;
	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(bendRadius);
	
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	
	end.SetbendLen(endbendLen);
	end.SetbendRadius(bendRadius);
	
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };

	for (int i = 0; i < numRebar; i++)//钢筋属性
	{
		vector<PITRebarCurve>     rebarCurves;

		makeRebarCurve(rebarCurves, xPos, width - allSideCov, endTypeStartOffset, endTypEendOffset, bendRadius, endType, endTypes, mat, m_IsTwinrebar);

		xPos += adjustedSpacing;
		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}//rebarset里面rebarelement初步建立完成
	//钢筋组
	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);
	
	vector<DSegment3d> vecStartEnd;
	for (RebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);

		vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));
		//EditElementHandle eeh;
		//LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		//eeh.AddToModel();

		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
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
			string Stype = "front";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}
	m_vecAllRebarStartEnd.push_back(vecStartEnd);
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

bool SICoverslabRebarAssembly::MakeRebars(DgnModelRefP modelRef)      // 创建钢筋
{
	RebarSetTagArray rsetTags;
	m_vecAllRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
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
	//CalculateUseHoles(modelRef);
	int iTwinbarSetIdIndex = 0;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kZaxis);
	double dLevelSpace = 0;
	double dSideCover = GetSideCover()*uor_per_mm;//反面保护层
	if ((COMPARE_VALUES(dSideCover, m_SICoverSlabData.length) >= 0) || (COMPARE_VALUES(dSideCover, m_SICoverSlabData.width) >= 0))	//如果侧面保护层大于等于墙的长度（板配筋改成高度）
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"侧面保护层大于等于  板的长度或宽度 ,无法创建钢筋层", MessageBoxIconType::Information);
		return false;
	}
	vector<CVector3D> vTrans;
	//计算侧面整体偏移量
	CalculateTransform(vTrans, modelRef);
	if (vTrans.size() != GetRebarLevelNum())
	{
		return false;
	}
	//高当作宽，墙面
	double dLength = m_SICoverSlabData.length;
	double dWidth = m_SICoverSlabData.width;

#ifdef PDMSIMPORT
	dLength *= modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	dWidth *= modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
#endif

	int iRebarSetTag = 0;
	int iRebarLevelNum = GetRebarLevelNum();
	vector<PIT::EndType> vecEndType;
	for (int i = 0; i < iRebarLevelNum; i++)
	{
		cover_direction = i;
		RebarSetTag* tag = NULL;
		CMatrix3D   mat;

		vecEndType = GetvvecEndType().at(i);
		
		//搭接，此时根据搭接选项的数据进行钢筋截断，生成多组钢筋
		double dActualWidth = dWidth;
		double dActualLength = dLength;
		int iRebarSetNum = 1;
		double overLength = 0.0;

 		if (GetvecDir().at(i) == 0)	
		{
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向

			for (size_t k = 0; k < vecEndNormal.size(); ++k)
			{
				double dRotateAngle = vecEndType.at(k).rotateAngle;

				endNormal = CVector3D::From(0, 0, -1);
				CVector3D  yVecNegate = m_LineNormal;
				if (k == 1)
				{
					yVecNegate = CVector3D::kZaxis;
					endNormal = m_LineNormal;
				}
				endNormal.Rotate(dRotateAngle * PI / 180, yVecNegate);	//以钢筋方向为轴旋转
				vecEndNormal[k] = endNormal;
			}
			
			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;
			
			tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i) * uor_per_mm,
				GetvecEndOffset().at(i) * uor_per_mm, GetvvecEndType().at(i), vecEndNormal, mat, modelRef);
			vecEndType.clear();
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			
			vecEndType.clear();
		}
		else
		{
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向
			
			for (size_t k = 0; k < vecEndNormal.size(); ++k)
			{
				double dRotateAngle = vecEndType.at(k).rotateAngle;
				endNormal = CVector3D::kZaxis;
				endNormal.Normalize();
				if (i == 1)
				{
					endNormal.Negate();
				}

				CVector3D rebarVec = m_SICoverSlabData.ptEnd - m_SICoverSlabData.ptStart;
				//					endNormal.Rotate(-90 * PI / 180, rebarVec);
				endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
				vecEndNormal[k] = endNormal;
			}
			
			mat = rot90;
			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;
		
			tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dWidth, dLength, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i) * uor_per_mm,
				GetvecEndOffset().at(i) * uor_per_mm, GetvvecEndType().at(i), vecEndNormal, mat, modelRef);
			vecEndType.clear();
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			
			vecEndType.clear();
		}
		if (m_vecRebarPtsLayer.size() > 1)
		{
			for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
			{
				int n = m + 1;
				RebarPoint rbPt;
				rbPt.Layer = i;
				rbPt.vecDir = GetvecDir().at(i);
				rbPt.ptstr = m_vecRebarPtsLayer.at(m);
				rbPt.ptend = m_vecRebarPtsLayer.at(n);
//				rbPt.DataExchange = GetvecDataExchange().at(i);
				g_vecRebarPtsNoHole.push_back(rbPt);
				m++;
			}
		}
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

void SICoverslabRebarAssembly::CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef)         //计算转换
{
	if (modelRef == NULL)
	{
		return;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;             //单位

	vTransform.clear();
	double dSideCover = GetSideCover()*uor_per_mm;//侧
	double dPositiveCover = GetPositiveCover()*uor_per_mm;//正
	double dReverseCover = GetReverseCover()*uor_per_mm;//反
	double dLevelSpace = 0;
	double dOffset = dPositiveCover;

	double zLen = dReverseCover;
	if (GetvvecEndType().at(1).at(0).endType != 0 || GetvvecEndType().at(1).at(1).endType != 0)
	{
		WString strSizeTmp = GetvecDirSize().at(1);
		if (strSizeTmp.find(L"mm") != WString::npos)
		{
			strSizeTmp.ReplaceAll(L"mm", L"");
		}
		zLen += RebarCode::GetPinRadius(strSizeTmp, modelRef, false) * 0.5 + RebarCode::GetBarDiameter(strSizeTmp, modelRef) * 0.5;
	}

	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}
		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10

		if (diameter > BE_TOLERANCE)
		{
			CVector3D	zTrans(0.0, 0.0, 0.0);
			// m_SICoverSlabData.width
			double levelSpacing = GetvecLevelSpace().at(i) * uor_per_mm;

			zLen += diameter * 0.5;
			if (COMPARE_VALUES_EPS(zLen + levelSpacing, m_SICoverSlabData.height, EPS) > 0)
			{
				double diameter_Tol = 0.00;
				for (int j = 0; j < GetRebarLevelNum(); j++)
				{
					WString strSizeTmp = GetvecDirSize().at(j);
					if (strSizeTmp.find(L"mm") != WString::npos)
					{
						strSizeTmp.ReplaceAll(L"mm", L"");
					}
					
					if (GetvecDir().at(j) == 0)
					{
						diameter_Tol += RebarCode::GetBarDiameter(strSizeTmp, modelRef);
					}

					if (j > i)
					{
						diameter_Tol += RebarCode::GetBarDiameter(strSizeTmp, modelRef);
					}
				}

				zLen += m_SICoverSlabData.height - zLen - diameter_Tol - dPositiveCover - diameter * 0.5;
				if (GetvvecEndType().at(2).at(0).endType != 0 || GetvvecEndType().at(2).at(1).endType != 0)
				{
					WString strSizeTmp1 = GetvecDirSize().at(2);
					if (strSizeTmp1.find(L"mm") != WString::npos)
					{
						strSizeTmp1.ReplaceAll(L"mm", L"");
					}
					zLen -= (RebarCode::GetPinRadius(strSizeTmp1, modelRef, false) * 0.5 + RebarCode::GetBarDiameter(strSizeTmp1, modelRef) * 0.5);
				}
			}
			else
			{
				zLen += levelSpacing;
			}
			
			if (GetvecDir().at(i) == 1)
			{
				zTrans.y = dSideCover + diameter * 0.5;
				zTrans.x = m_SICoverSlabData.length * 0.5;
				zTrans.z = zLen;
			}
			else
			{
				zTrans.y = m_SICoverSlabData.width * 0.5;
				zTrans.x = dSideCover + diameter * 0.5;
				zTrans.z = zLen;
			}

			zLen += diameter * 0.5;
			vTransform.push_back(zTrans);
		}
	}
}

long SICoverslabRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool SICoverslabRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	pCoverslabDoubleRebarDlg = new CoverslabRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pCoverslabDoubleRebarDlg->Create(IDD_DIALOG_CoverslabRebar);
	pCoverslabDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pCoverslabDoubleRebarDlg->ShowWindow(SW_SHOW);

	return true;
}

bool SICoverslabRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehslab(GetSelectedElement(), GetSelectedModel());
	if (!ehslab.IsValid())
		return false;

	DgnModelRefP modelRef = ehslab.GetModelRef();

	SetCoverSlabData(ehslab);

	MakeRebars(modelRef);//调用创建钢筋
	Save(modelRef);

	ElementId contid = FetchConcrete();
	return true;
}

bool SICoverslabRebarAssembly::SetCoverSlabData(ElementHandleCR eh)
{
#ifdef PDMSIMPORT
	WString strName;
	WString strType;
	DgnECManagerR ecMgr = DgnECManager::GetManager();
	FindInstancesScopePtr scope = FindInstancesScope::CreateScope(eh, FindInstancesScopeOption(DgnECHostType::Element));
	ECQueryPtr            ecQuery = ECQuery::CreateQuery(ECQUERY_PROCESS_SearchAllClasses);
	//ECQUERY_PROCESS_SearchAllExtrinsic will only search ECXAttr
	ecQuery->SetSelectProperties(true);

	for (DgnECInstancePtr instance : ecMgr.FindInstances(*scope, *ecQuery))
	{
		DgnElementECInstanceP elemInst = instance->GetAsElementInstance();
		for (ECPropertyP ecProp : elemInst->GetClass().GetProperties())
		{
			WString strLabel = ecProp->GetDisplayLabel().GetWCharCP();
			if (strLabel != L"NAME" && strLabel != L"TYPE")
			{
				continue;
			}

			WString valStr;
			ECValue ecVal;
			elemInst->GetValue(ecVal, ecProp->GetName().GetWCharCP());
			IDgnECTypeAdapterR typeAdapter = IDgnECTypeAdapter::GetForProperty(*ecProp);
			IDgnECTypeAdapterContextPtr typeContext = IDgnECTypeAdapterContext::Create(*ecProp, *elemInst, ecProp->GetName().GetWCharCP());
			typeAdapter.ConvertToString(valStr, ecVal, *typeContext);

			if (strLabel == L"NAME")
				strName = valStr;
			else
				strType = valStr;
		}
	}
	string name = StringOperator::Convert::WStringToString(strName.c_str());
	//读取数据
//	ComponentDataManage dataManage("C:\\Users\\Power-Itech-LX\\Desktop\\data\\1BFX15--VB-.bim"); // 构件数据管理
	ComponentDataManage dataManage("C:\\Users\\Power-Itech-LX\\Desktop\\1BFX-----CW\\1BFX-----B\\data\\1BFX2078VB-.bim"); // 构件数据管理
	dataManage.GetAllComponentData();

	vector<IComponent*> vecComponent = ComponentDataManage::GetAllComponent();
	for (size_t i = 0; i < vecComponent.size(); i++)
	{
		IComponent *Component = vecComponent[i];
		string strComponentName = Component->GetEleName();
		string strType = Component->GetEleType();
		if (strType == "STWALL")
		{
			STWallComponent* stWall = dynamic_cast<STWallComponent*>(Component);
			if (stWall == NULL)
			{
				continue;
			}
			string strComponentName = stWall->GetEleName();
			if (name == strComponentName)
			{
				pWall = stWall;
			}
		}
	}
	if (pWall == NULL || !eh.IsValid())
	{
		return false;
	}

	double uor_per_mm = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	m_STwallData.length = pWall->GetPtStart().Distance(pWall->GetPtEnd()) * uor_per_mm;
	m_STwallData.width = pWall->GetWidth() * uor_per_mm;
	m_STwallData.height = pWall->GetHeight() * uor_per_mm;

	DPoint3d ptStart = pWall->GetPtStart();
	ptStart.x *= uor_per_mm;
	ptStart.y *= uor_per_mm;
	ptStart.z *= uor_per_mm;
	DPoint3d ptEnd = pWall->GetPtEnd();
	ptEnd.x *= uor_per_mm;
	ptEnd.y *= uor_per_mm;
	ptEnd.z *= uor_per_mm;


	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
	//由于该线为底面中心线，需将原点进行偏移1/2的厚度
	//以yVec的反反向进行偏移1/2的墙厚
	CVector3D  yVecNegate = yVec;
	yVecNegate.Negate();
	yVecNegate.Normalize();
	yVecNegate.ScaleToLength(m_STwallData.width * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	ptStart.Add(yVecNegate);
	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(ptStart, ptEnd);

	// 	CVector3D  xVecNegate = xVecNew;
	// 	xVecNegate.Normalize();
	// 	xVecNegate.ScaleToLength(m_STwallData.length * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	// 	ptStart.Add(xVecNegate);
	// 	CVector3D  zVecNegate = CVector3D::kZaxis;
	// 	zVecNegate.Normalize();
	// 	zVecNegate.ScaleToLength(m_STwallData.height * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	// 	ptStart.Add(zVecNegate);
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
#else
	//	m_eeh1 = eh;
	bool bRet = AnalyzingSICoverSlabData(eh);
	if (!bRet)
		return false;

	DPoint3d ptStart = m_SICoverSlabData.ptStart;            //原点
	DPoint3d ptEnd = m_SICoverSlabData.ptEnd;
	CVector3D  xVec(ptStart, ptEnd);
	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);     //返回两个向量的（标量）叉积。y
	m_LineNormal = yVec;
	m_LineNormal.Normalize();

	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴
//	BeMatrix   placement = CMatrix3D::Ucs(ptStart, CVector3D::kXaxis, CVector3D::kYaxis, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
#endif
}

bool SICoverslabRebarAssembly::AnalyzingSICoverSlabData(ElementHandleCR eh)
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
	EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &m_SICoverSlabData.height);


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

	if (vecDownBackLine.size() > 1 || vecDownFontLine.size() > 1)
	{
		GetMaxDownFacePts(vecDownFontLine, vecDownBackLine, pt1, pt2);
	}

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_SICoverSlabData.height);

	m_SICoverSlabData.height = m_SICoverSlabData.height*uor_now / uor_ref;
	m_SICoverSlabData.width = FrontStr.Distance(BackStr)*uor_now / uor_ref;
	m_SICoverSlabData.length = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;
	m_SICoverSlabData.ptStart = FrontStr;
	m_SICoverSlabData.ptEnd = FrontEnd;

	FrontStr.Add(FrontEnd);
	FrontStr.Scale(0.5);
	BackStr.Add(BackEnd);
	BackStr.Scale(0.5);
	m_LineNormal = BackStr - FrontStr;
	m_LineNormal.Normalize();

	m_Negs = Negs;

	if (Negs.size() > 0)//STWALL有斜边
	{

		DPoint3d vecBack = pt2[0] - pt2[1];
		DPoint3d vecLeft = pt1[0] - pt2[0];
		DPoint3d vecRight = pt1[1] - pt2[1];

		vecBack.Normalize();
		vecLeft.Normalize();
		vecRight.Normalize();

		m_angle_left = vecBack.AngleTo(vecLeft);
		if (m_angle_left > PI / 2)
		{
			m_angle_left = PI - m_angle_left;
		}
		m_angle_right = vecBack.AngleTo(vecRight);
		if (m_angle_right > PI / 2)
		{
			m_angle_right = PI - m_angle_right;
		}
	}
	else
	{
		m_angle_left = PI / 2;
		m_angle_right = PI / 2;
	}
	m_Holeehs = Holeehs;

	return true;
}

bool STCoverslabRebarAssembly::makeRebarCurve
(
	vector<PITRebarCurve>&     rebars,
	double                  xPos,
	double                  yLen,
	double					startOffset,
	double					endOffset,
	const vector<PIT::EndType>&		vecEndtype,
	PITRebarEndTypes&		endTypes,
	CMatrix3D const&        mat,
	bool isTwin
)
{

	CPoint3D  startPt;
	CPoint3D  endPt;

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	startPt = CPoint3D::From(xPos, -yLen / 2.0 + startOffset, 0.0);
	endPt = CPoint3D::From(xPos, yLen / 2.0 - endOffset, 0.0);

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
//	eeh.AddToModel();



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


	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}
	if (!isTwin)
	{
		m_vecRebarPtsLayer.push_back(pt1[0]);
		m_vecRebarPtsLayer.push_back(pt1[1]);
	}
	

	// EditElementHandle eeh2;
	// GetContractibleeeh(eeh2);//获取减去保护层的端部缩小的实体

	double dSideCover = GetSideCover()*uor_per_mm;
	vector<DPoint3d> tmppts;
	vector<DPoint3d> tmpptsTmp;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	//	GetIntersectPointsWithOldElm(tmpptsTmp, m_pOldElm, pt1[0], pt1[1], dSideCover, matrix);
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);

	//EditElementHandle eehline;
	//LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
	//eehline.AddToModel();

	map<int, DPoint3d> map_pts;
	for (DPoint3d pt : tmppts)
	{
		int dis = (int)pt1[0].Distance(pt);
		if (dis == 0 || dis == (int)pt1[0].Distance(pt1[1]))//防止过滤掉起点和终点
		{
			dis = dis + 1;
		}

		map_pts[dis] = pt;
	}
	map_pts[0] = pt1[0];
	map_pts[(int)pt1[0].Distance(pt1[1])] = pt1[1];

	RebarVertices  vers;
	bvector<DPoint3d> allpts;

	DPoint3d temp;

	double dPositiveCover = GetPositiveCover()*uor_per_mm;//上
	double dReverseCover = GetReverseCover()*uor_per_mm;//下

	CVector3D  tempVec(pt1[1], pt1[0]);
	CVector3D  arcVec = tempVec;

	if (0 == GetvecDir().at(cover_direction))
	{
		if (cover_direction < 2)
		{
			temp = pt1[0];

			WString strSizeTmp1 = GetvecDirSize().at(0);
			if (strSizeTmp1.find(L"mm") != WString::npos)
			{
				strSizeTmp1.ReplaceAll(L"mm", L"");
			}
			temp.z += CoverSlabLowDate.height - m_diameter1 * 0.5 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) + dReverseCover;
			allpts.push_back(temp);
			allpts.push_back(pt1[0]);

			temp = pt1[0];
			arcVec.Normalize();
			arcVec.Negate();
			arcVec.ScaleToLength(pt1[0].Distance(pt1[1]) - m_diameter1);
			temp.Add(arcVec);
			allpts.push_back(temp);

			temp.z += CoverSlabLowDate.height - m_diameter1 * 0.5 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) + dReverseCover;
			allpts.push_back(temp);

			GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
			PITRebarCurve rebar;
			rebar.SetVertices(vers);
			rebars.push_back(rebar);

			bvector<DPoint3d> allptsTmp;

			temp = pt1[0];
			temp.z += m_STCoverSlabData.height - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) * 2 - dReverseCover - dPositiveCover - m_diameter1;
			DPoint3d ptTemp = temp;
			temp.z -= CoverSlabHighDate.height - m_diameter1 * 0.5 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) - dReverseCover;
			allptsTmp.push_back(temp);
			allptsTmp.push_back(ptTemp);
			temp = ptTemp;

			arcVec.Normalize();
			arcVec.ScaleToLength(pt1[0].Distance(pt1[1]) - m_diameter1);
			temp.Add(arcVec);
			allptsTmp.push_back(temp);

			temp.z -= CoverSlabHighDate.height - m_diameter1 * 0.5 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) - dReverseCover;
			allptsTmp.push_back(temp);

			RebarVertices  versTmp;
			GetRebarVerticesFromPoints(versTmp, allptsTmp, m_diameter1);
			PITRebarCurve rebar1;
			rebar1.SetVertices(versTmp);
			rebars.push_back(rebar1);

			return true;
		}
		else if (cover_direction == 2)
		{
			allpts.push_back(pt1[0]);
			allpts.push_back(pt1[1]);

			endTypes.beg.SetptOrgin(allpts[0]);
			endTypes.end.SetptOrgin(allpts[allpts.size() - 1]);

			GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
			PITRebarCurve rebar;
			rebar.SetVertices(vers);
			rebar.EvaluateEndTypes(endTypes);
			rebars.push_back(rebar);

			allpts.clear();

			double dLength = m_zLen - CoverSlabLowDate.height - dReverseCover - m_diameter1 * 0.5;
			if (GetRebarLevelNum() > 4)
			{
				WString strSizeTmp = GetvecDirSize().at(4);
				if (strSizeTmp.find(L"mm") != WString::npos)
				{
					strSizeTmp.ReplaceAll(L"mm", L"");
				}
				double diameterFive = RebarCode::GetBarDiameter(strSizeTmp, ACTIVEMODEL);
				dLength -= diameterFive;
			}

			DPoint3d ptTmp = pt1[0];
			ptTmp.z -= dLength;
			allpts.push_back(ptTmp);

			ptTmp = pt1[1];
			ptTmp.z -= dLength;
			allpts.push_back(ptTmp);

			endTypes.beg.SetptOrgin(allpts[0]);
			endTypes.end.SetptOrgin(allpts[allpts.size() - 1]);

			CVector3D vecTmp = endTypes.beg.GetendNormal();
			vecTmp.Negate();
			endTypes.beg.SetendNormal(vecTmp);
			endTypes.end.SetendNormal(vecTmp);

			RebarVertices  versTmp;
			GetRebarVerticesFromPoints(versTmp, allpts, m_diameter1);
			PITRebarCurve rebarTmp;
			rebarTmp.SetVertices(versTmp);
			rebarTmp.EvaluateEndTypes(endTypes);
			rebars.push_back(rebarTmp);

			vecTmp.Negate();
			endTypes.beg.SetendNormal(vecTmp);
			endTypes.end.SetendNormal(vecTmp);

			return true;
		}
		else
		{
			arcVec.Normalize();
			arcVec.Negate();
			double dLength = m_diameter1 * 0.5;

			double diameter_five = 0.00;
			if (GetRebarLevelNum() > 4)
			{
				WString strSizeTmp = GetvecDirSize().at(4);
				if (strSizeTmp.find(L"mm") != WString::npos)
				{
					strSizeTmp.ReplaceAll(L"mm", L"");
				}
				diameter_five = RebarCode::GetBarDiameter(strSizeTmp, ACTIVEMODEL);
			}
			//if (GetRebarLevelNum() > 4)
			//{
			//	dLength += diameter_five;
			//}

			arcVec.ScaleToLength(dLength);
			(pt1[0]).Add(arcVec);

			DPoint3d temp2;
			temp2 = pt1[0];
			temp2.z -= m_zLen - CoverSlabLowDate.height - dReverseCover - m_diameter1 * 0.5;

			if (GetvvecEndType().at(2).at(0).endType != 0 || GetvvecEndType().at(2).at(1).endType != 0)
			{
				WString strSizeTmp1 = GetvecDirSize().at(2);
				if (strSizeTmp1.find(L"mm") != WString::npos)
				{
					strSizeTmp1.ReplaceAll(L"mm", L"");
				}
				temp2.z += (RebarCode::GetPinRadius(strSizeTmp1, ACTIVEMODEL, false) * 0.5 + RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) * 0.5);
			}

			if (GetRebarLevelNum() > 4)
			{
				temp2.z += diameter_five;
			}

			temp = temp2;
			arcVec.Normalize();
			arcVec.ScaleToLength(CoverSlabLowDate.length * 0.25);
			temp.Add(arcVec);
			allpts.push_back(temp);
			allpts.push_back(temp2);
			allpts.push_back(pt1[0]);

			temp = pt1[0];
			arcVec.Normalize();
			arcVec.ScaleToLength(CoverSlabLowDate.length * 0.25);
			temp.Add(arcVec);
			allpts.push_back(temp);

			GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
			PITRebarCurve rebar;
			rebar.SetVertices(vers);
			rebars.push_back(rebar);

			allpts.clear();

			arcVec.Normalize();
			arcVec.Negate();

			dLength = m_diameter1 * 0.5;
			//if (GetRebarLevelNum() > 4)
			//{
			//	dLength += diameter_five;
			//}
			arcVec.ScaleToLength(dLength);
			(pt1[1]).Add(arcVec);

			temp2 = pt1[1];
			temp2.z -= m_zLen - CoverSlabLowDate.height - dReverseCover - m_diameter1 * 0.5;

			if (GetvvecEndType().at(2).at(0).endType != 0 || GetvvecEndType().at(2).at(1).endType != 0)
			{
				WString strSizeTmp1 = GetvecDirSize().at(2);
				if (strSizeTmp1.find(L"mm") != WString::npos)
				{
					strSizeTmp1.ReplaceAll(L"mm", L"");
				}
				temp2.z += (RebarCode::GetPinRadius(strSizeTmp1, ACTIVEMODEL, false) * 0.5 + RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) * 0.5);
			}

			if (GetRebarLevelNum() > 4)
			{
				temp2.z += diameter_five;
			}

			temp = temp2;
			arcVec.Normalize();
			arcVec.ScaleToLength(CoverSlabLowDate.length * 0.25);
			temp.Add(arcVec);
			allpts.push_back(temp);
			allpts.push_back(temp2);
			allpts.push_back(pt1[1]);

			temp = pt1[1];
			arcVec.Normalize();
			arcVec.ScaleToLength(CoverSlabLowDate.length * 0.25);
			temp.Add(arcVec);
			allpts.push_back(temp);

			RebarVertices  versTmp;
			GetRebarVerticesFromPoints(versTmp, allpts, m_diameter1);
			PITRebarCurve rebar1;
			rebar1.SetVertices(versTmp);
			rebars.push_back(rebar1);

			return true;
		}
	}
	else
	{
		if (cover_direction < 2)
		{
			//arcVec.Normalize();
			//arcVec.ScaleToLength(m_diameter1*0.5);
			//(pt1[1]).Add(arcVec);

			temp = pt1[1];

			temp.z += (m_STCoverSlabData.height - dReverseCover - dPositiveCover - m_diameter1);// 弯曲长度=减去钢筋直径和上面保护层
			allpts.push_back(temp);

			arcVec.Normalize();
			arcVec.ScaleToLength(pt1[1].Distance(pt1[0]) - m_diameter1 * 0.5 + endOffset);
			temp.Add(arcVec);
			allpts.push_back(temp);

			temp.z -= (m_STCoverSlabData.height - dReverseCover - dPositiveCover - m_diameter1);// 弯曲长度=减去钢筋直接和上面保护层
			allpts.push_back(temp);

			arcVec.Normalize();
			arcVec.Negate();
			arcVec.ScaleToLength(pt1[1].Distance(pt1[0]) - m_diameter1 + endOffset + startOffset);
			temp.Add(arcVec);
			allpts.push_back(temp);

			temp.z += (m_STCoverSlabData.height - dReverseCover - dPositiveCover - m_diameter1 * 0.5 - endOffset);// 弯曲长度=减去钢筋直接和上面保护层
			allpts.push_back(temp);

		}
		else if (cover_direction == 2)
		{
			allpts.push_back(pt1[0]);
			allpts.push_back(pt1[1]);

			endTypes.beg.SetptOrgin(allpts[0]);
			endTypes.end.SetptOrgin(allpts[allpts.size() - 1]);

			GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
			PITRebarCurve rebar;
			rebar.SetVertices(vers);
			rebar.EvaluateEndTypes(endTypes);
			rebars.push_back(rebar);

			WString strSizeTmp = GetvecDirSize().at(3);
			if (strSizeTmp.find(L"mm") != WString::npos)
			{
				strSizeTmp.ReplaceAll(L"mm", L"");
			}

			double diameterFour = RebarCode::GetBarDiameter(strSizeTmp, ACTIVEMODEL);

			double dLength = m_zLen - CoverSlabLowDate.height - dReverseCover - diameterFour * 1.5 - m_diameter1;

			allpts.clear();

			if (GetRebarLevelNum() > 4)
			{
				WString strSizeTmp = GetvecDirSize().at(4);
				if (strSizeTmp.find(L"mm") != WString::npos)
				{
					strSizeTmp.ReplaceAll(L"mm", L"");
				}
				double diameter_five = RebarCode::GetBarDiameter(strSizeTmp, ACTIVEMODEL);
				dLength -= diameter_five;
			}

			if (GetvvecEndType().at(2).at(0).endType != 0 || GetvvecEndType().at(2).at(1).endType != 0)
			{
				WString strSizeTmp1 = GetvecDirSize().at(2);
				if (strSizeTmp1.find(L"mm") != WString::npos)
				{
					strSizeTmp1.ReplaceAll(L"mm", L"");
				}
				dLength -= (RebarCode::GetPinRadius(strSizeTmp1, ACTIVEMODEL, false) * 0.5 + RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) * 0.5);
			}

			DPoint3d ptTmp = pt1[0];
			ptTmp.z -= dLength;
			allpts.push_back(ptTmp);

			ptTmp = pt1[1];
			ptTmp.z -= dLength;
			allpts.push_back(ptTmp);

			endTypes.beg.SetptOrgin(allpts[0]);
			endTypes.end.SetptOrgin(allpts[allpts.size() - 1]);

			CVector3D vecTmp = endTypes.beg.GetendNormal();
			vecTmp.Negate();
			endTypes.beg.SetendNormal(vecTmp);
			endTypes.end.SetendNormal(vecTmp);

			RebarVertices  versTmp;
			GetRebarVerticesFromPoints(versTmp, allpts, m_diameter1);
			PITRebarCurve rebarTmp;
			rebarTmp.SetVertices(versTmp);
			rebarTmp.EvaluateEndTypes(endTypes);
			rebars.push_back(rebarTmp);

			vecTmp.Negate();
			endTypes.beg.SetendNormal(vecTmp);
			endTypes.end.SetendNormal(vecTmp);

			return true;
		}
		else
		{
			allpts.push_back(pt1[1]);

			DPoint3d ptTemp = pt1[1];
			arcVec.Normalize();
			arcVec.ScaleToLength(pt1[1].Distance(pt1[0]) - m_diameter1 * 0.5 + startOffset);
			ptTemp.Add(arcVec);
			allpts.push_back(ptTemp);

			ptTemp.z -= CoverSlabHighDate.height - dReverseCover - dPositiveCover - m_diameter1;
			allpts.push_back(ptTemp);

			arcVec.Negate();
			arcVec.Normalize();
			arcVec.ScaleToLength(pt1[1].Distance(pt1[0]) + startOffset - m_diameter1 + endOffset);
			ptTemp.Add(arcVec);
			allpts.push_back(ptTemp);


			ptTemp.z += CoverSlabHighDate.height - dReverseCover - dPositiveCover - m_diameter1 * 0.5 - endOffset;
			allpts.push_back(ptTemp);
		}
	}

	if (!allpts.empty())
	{
		endTypes.beg.SetptOrgin(allpts[0]);
		endTypes.end.SetptOrgin(allpts[allpts.size() - 1]);
	}

	GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
	PITRebarCurve rebar;
	rebar.SetVertices(vers);
	if (vers.GetSize() == 2)
	{
		rebar.EvaluateEndTypes(endTypes);
	}
	else
	{
		vector<CVector3D> vecEndNormal(2);
		CVector3D	endNormal;	//端部弯钩方向
		for (unsigned int i = 0; i < 2; i++)
		{
			CVector3D vec1 = allpts[1] - allpts[0];
			CVector3D vec2 = allpts[allpts.size() - 2] - allpts[allpts.size() - 1];
			vec1.Normalize();
			vec2.Normalize();
			if (vec1.IsPerpendicularTo(vec2))
			{
				if (i == 0)
				{
					double dRotateAngle = vecEndtype[i].rotateAngle;
					CVector3D rebarVec = vec1;
					endNormal = vec2;
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[i] = endNormal;
					endTypes.beg.SetendNormal(vecEndNormal[0]);
				}
				else
				{
					double dRotateAngle = vecEndtype[i].rotateAngle;
					CVector3D rebarVec = vec2;
					endNormal = vec1;
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[i] = endNormal;
					endTypes.end.SetendNormal(vecEndNormal[1]);
				}
			}
		}

		rebar.EvaluateEndTypesStirrup(endTypes);
	}

	rebars.push_back(rebar);

	//for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	//{
	//	PITRebarCurve rebar;
	//	RebarVertexP vex;
	//	vex = &rebar.PopVertices().NewElement();
	//	vex->SetIP(itr->second);
	//	vex->SetType(RebarVertex::kStart);
	//	endTypes.beg.SetptOrgin(itr->second);

	//	map<int, DPoint3d>::iterator itrplus = ++itr;
	//	if (itrplus == map_pts.end())
	//	{
	//		break;
	//	}

	//	endTypes.end.SetptOrgin(itrplus->second);

	//	vex = &rebar.PopVertices().NewElement();
	//	vex->SetIP(itrplus->second);
	//	vex->SetType(RebarVertex::kEnd);

	//	rebar.EvaluateEndTypes(endTypes);
	//	//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
	//	rebars.push_back(rebar);

	//	//LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(itr->second, itrplus->second), true, *ACTIVEMODEL);
	//	//eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	//	//eeh.AddToModel();
	//}


	//rebar.DoMatrix(mat);
	return true;
}

RebarSetTag* STCoverslabRebarAssembly::MakeRebars
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	double              xLen,
	double              width,
	double              spacing,
	double              startOffset,
	double              endOffset,

	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
	CMatrix3D const&    mat,
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
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
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
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}
	//	}//由于板配筋和墙配筋方向不同改保护层正反侧面
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	m_diameter1 = diameter;

	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false) * 0.5;	//乘以了30
	double startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef) * 0.5;	//乘以了100
	double endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef) * 0.5;	//乘以了100
	int numRebar = 0;
	double adjustedXLen, adjustedSpacing;

	double allSideCov = GetSideCover()*uor_per_mm * 2;

	if (cover_direction == 2)
	{
		numRebar = 2;
		WString strSizeTmp = GetvecDirSize().at(3);
		if (strSizeTmp.find(L"mm") != WString::npos)
		{
			strSizeTmp.ReplaceAll(L"mm", L"");
		}

		startOffset += RebarCode::GetBarDiameter(strSizeTmp, ACTIVEMODEL) * 0.5 + diameter * 0.5;
		endOffset += RebarCode::GetBarDiameter(strSizeTmp, ACTIVEMODEL) * 0.5 + diameter * 0.5;

		adjustedXLen = xLen - 2.0 * GetSideCover()*uor_per_mm - diameter - startOffset - endOffset;
	}
	else
	{
		adjustedXLen = xLen - 2.0 * GetSideCover()*uor_per_mm - diameter - startOffset - endOffset;
		numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;
	}

	adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);

	double xPos = startOffset;

	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(bendRadius);
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(bendRadius);
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };

	for (int i = 0; i < numRebar; i++)//钢筋属性
	{
		vector<PITRebarCurve>     rebarCurves;
		makeRebarCurve(rebarCurves, xPos, width - allSideCov, endTypeStartOffset, endTypEendOffset, endType, endTypes, mat,m_IsTwinrebar);
		xPos += adjustedSpacing;
		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}//rebarset里面rebarelement初步建立完成
	//钢筋组

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

		//EditElementHandle eeh;
		//LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		//eeh.AddToModel();

		vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));

		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
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
			string Stype = "front";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}
	m_vecAllRebarStartEnd.push_back(vecStartEnd);
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

bool STCoverslabRebarAssembly::MakeRebars(DgnModelRefP modelRef)      // 创建钢筋
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	CalculateUseHoles(modelRef);
	m_vecAllRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();

	int iTwinbarSetIdIndex = 0;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kZaxis);
	double dLevelSpace = 0;
	double dSideCover = GetSideCover()*uor_per_mm;//反面保护层
	if ((COMPARE_VALUES(dSideCover, m_STCoverSlabData.length) >= 0) || (COMPARE_VALUES(dSideCover, m_STCoverSlabData.width) >= 0))	//如果侧面保护层大于等于墙的长度（板配筋改成高度）
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"侧面保护层大于等于  板的长度或宽度 ,无法创建钢筋层", MessageBoxIconType::Information);
		return false;
	}
	vector<CVector3D> vTrans;
	//计算侧面整体偏移量
	CalculateTransform(vTrans, modelRef);
	if (vTrans.size() != GetRebarLevelNum())
	{
		return false;
	}
	//高当作宽，墙面
	//double dLength = m_STCoverSlabData.length;
	//double dWidth = m_STCoverSlabData.width;

#ifdef PDMSIMPORT
	dLength *= modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	dWidth *= modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
#endif

	int iRebarSetTag = 0;
	int iRebarLevelNum = GetRebarLevelNum();
	vector<PIT::EndType> vecEndType;
	for (int i = 0; i < iRebarLevelNum; i++)
	{
		cover_direction = i;
		RebarSetTag* tag = NULL;
		CMatrix3D   mat;

		//LapOptions lapOption;
		//if (GetvecLapOptions().empty())		//没有设置搭接选项，设置默认值
		//	lapOption = { 0,0,0,0,0,0,0 };
		//else
		//	lapOption = GetvecLapOptions().at(i);

		vector<PIT::EndType> vecEndType;
		if (GetvvecEndType().empty())		//没有设置端部样式，设置默认值
			vecEndType = { { 0,0,0 },{0,0,0} };
		else
			vecEndType = GetvvecEndType().at(i);

		double dActualWidth, dActualLength;
		//搭接，此时根据搭接选项的数据进行钢筋截断，生成多组钢筋
		if (i < 2)
		{
			 dActualWidth = CoverSlabLowDate.width;
			 dActualLength = CoverSlabLowDate.length;
		}
		else
		{
			dActualWidth = CoverSlabHighDate.width;
			dActualLength = CoverSlabHighDate.length;
		}
		int iRebarSetNum = 1;
		double overLength = 0.0;

		if (GetvecDir().at(i) == 1)	//纵向钢筋
		{
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向
			if (GetvvecEndType().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = CVector3D::kZaxis;
					CVector3D  yVecNegate = m_LineNormal;
					if (k == 1 && i == 0)
					{
						endNormal = m_LineNormal;
						endNormal.Negate();
						yVecNegate = CVector3D::kZaxis;
					}

					//endNormal.Rotate(-90 * PI / 180, yVecNegate);
					endNormal.Rotate(dRotateAngle * PI / 180, yVecNegate);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}
			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;

			tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dActualLength, dActualWidth, GetvecDirSpacing().at(i) * uor_per_mm, GetvecStartOffset().at(i) * uor_per_mm,
				GetvecEndOffset().at(i) * uor_per_mm, GetvvecEndType().at(i), vecEndNormal, mat, modelRef);
			vecEndType.clear();
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}

			vecEndType.clear();

		}
		else
		{
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向
			if (GetvvecEndType().size() > 0)
			{
				//double dRotateAngle = GetvvecEndType().at(i).at(0).rotateAngle;
				//CVector3D rebarVec = m_STslabData.ptEnd - m_STslabData.ptStart;

				//endNormal = CVector3D::From(-1, 0, 0);
				//if (i == iRebarLevelNum - 2|| i == iRebarLevelNum - 1)
				//{
				//	endNormal.Negate();
				//}
				//	
				//endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = CVector3D::kZaxis;
					CVector3D  rebarVec = m_LineNormal;
					if (k == 1)
					{
						endNormal = m_LineNormal;
						rebarVec = CVector3D::kZaxis;
					}
					// endNormal.Rotate(-90 * PI / 180, rebarVec);
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}
			mat = rot90;
			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;


			tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dActualWidth, dActualLength, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i) * uor_per_mm,
				GetvecEndOffset().at(i) * uor_per_mm, GetvvecEndType().at(i), vecEndNormal, mat, modelRef);
			vecEndType.clear();
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();
		}
	}
	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

void STCoverslabRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
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

bool STCoverslabRebarAssembly::OnDoubleClick()
{
	vector<PIT::ConcreteRebar> vecRebarData;
	vector<PIT::LapOptions> vecLaptionData;
	vector<PIT::EndType> vecEndTypeData;
	vector<TwinBarSet::TwinBarLevelInfo> vecTwinBarData;
	//	Concrete concreteData;
	//	TieReBarInfo tieRebarInfo;
		// 	GetRebarData(vecData);
		// 	GetConcreteData(concreteData);
	CoverSlabType slabType = GetCoverslabType();
	// 	int lastAction = ACTIONBUTTON_CANCEL;
	// 	if (SUCCESS != mdlDialog_openModal(&lastAction, GetResourceHandle(), DIALOGID_WallRebar) || lastAction != ACTIONBUTTON_OK)
	// 		return false;

	ElementId testid = FetchConcrete();
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

	SetSelectedModel(modelp);
	GetConcreteXAttribute(testid, ACTIVEMODEL);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	DgnModelRefP modelRef = ACTIVEMODEL;
	pSTCoverslabDoubleRebarDlg = new CoverslabRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pSTCoverslabDoubleRebarDlg->m_isDoubleClick = true;
	pSTCoverslabDoubleRebarDlg->Create(IDD_DIALOG_CoverslabRebar);
	pSTCoverslabDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pSTCoverslabDoubleRebarDlg->ShowWindow(SW_SHOW);

	// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// 	CoverslabRebarDlg dlg(ehSel, CWnd::FromHandle(MSWIND));
	// 	//	ElementHandle eh(GetSelectedElement(), modelRef);
	// 	dlg.SetSelectElement(ehSel);
	// 	dlg.SetConcreteId(FetchConcrete());
	// 	if (IDCANCEL == dlg.DoModal())
	// 		return false;

	return true;
}

bool STCoverslabRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehslab(GetSelectedElement(), GetSelectedModel());
	if (!ehslab.IsValid())
		return false;

	DgnModelRefP modelRef = ehslab.GetModelRef();

	SetCoverSlabData(ehslab);

	MakeRebars(modelRef);//调用创建钢筋
	Save(modelRef);

	ElementId contid = FetchConcrete();
	return true;
}

void STCoverslabRebarAssembly::CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef)         //计算转换
{
	if (modelRef == NULL)
	{
		return;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;             //单位

	vTransform.clear();
	double dSideCover = GetSideCover()*uor_per_mm;//侧
	double dPositiveCover = GetPositiveCover()*uor_per_mm;//正
	double dReverseCover = GetReverseCover()*uor_per_mm;//反
	double dLevelSpace = 0;
	double dOffset = dPositiveCover;

	double zLen = dReverseCover;
	if (GetvvecEndType().at(1).at(0).endType != 0 || GetvvecEndType().at(1).at(1).endType != 0)
	{
		WString strSizeTmp = GetvecDirSize().at(1);
		if (strSizeTmp.find(L"mm") != WString::npos)
		{
			strSizeTmp.ReplaceAll(L"mm", L"");
		}
		zLen += RebarCode::GetPinRadius(strSizeTmp, modelRef, false) * 0.5 + RebarCode::GetBarDiameter(strSizeTmp, modelRef) * 0.5;
	}

	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}
		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10

		if (diameter > BE_TOLERANCE)
		{
			CVector3D	zTrans(0.0, 0.0, 0.0);
			// m_SICoverSlabData.width
			double levelSpacing = GetvecLevelSpace().at(i) * uor_per_mm;

			zLen += diameter * 0.5;
			if (COMPARE_VALUES_EPS(zLen + levelSpacing, m_STCoverSlabData.height, EPS) > 0)
			{
				double diameter_Tol = 0.00;
				for (int j = 0; j < GetRebarLevelNum(); j++)
				{
					WString strSizeTmp = GetvecDirSize().at(j);
					if (strSizeTmp.find(L"mm") != WString::npos)
					{
						strSizeTmp.ReplaceAll(L"mm", L"");
					}

					if (GetvecDir().at(j) == 0)
					{
						diameter_Tol += RebarCode::GetBarDiameter(strSizeTmp, modelRef);
					}

					if (j > i)
					{
						diameter_Tol += RebarCode::GetBarDiameter(strSizeTmp, modelRef);
					}
				}

				zLen = m_STCoverSlabData.height - diameter_Tol - dPositiveCover - diameter * 0.5;
				if (GetvvecEndType().at(2).at(0).endType != 0 || GetvvecEndType().at(2).at(1).endType != 0)
				{
					WString strSizeTmp1 = GetvecDirSize().at(2);
					if (strSizeTmp1.find(L"mm") != WString::npos)
					{
						strSizeTmp1.ReplaceAll(L"mm", L"");
					}
					zLen -= (RebarCode::GetPinRadius(strSizeTmp1, modelRef, false) * 0.5 + RebarCode::GetBarDiameter(strSizeTmp1, modelRef) * 0.5);
				}
				else
				{
					WString strSizeSec = GetvecDirSize().at(2);
					if (strSizeSec.find(L"mm") != WString::npos)
					{
						strSizeSec.ReplaceAll(L"mm", L"");
					}
					zLen += RebarCode::GetBarDiameter(strSizeSec, modelRef);

					if (GetRebarLevelNum() > 4)
					{
						WString strSizeFir = GetvecDirSize().at(0);
						if (strSizeFir.find(L"mm") != WString::npos)
						{
							strSizeFir.ReplaceAll(L"mm", L"");
						}
						zLen += RebarCode::GetBarDiameter(strSizeFir, modelRef);
					}
				}
			}
			else
			{
				zLen += levelSpacing;
			}

			if (i == 0)
			{
				zTrans.x = dSideCover + diameter * 0.5 + (CoverSlabHighDate.length - CoverSlabLowDate.length) * 0.5;
				zTrans.y = CoverSlabHighDate.width * 0.5;
				zTrans.z = zLen;
			}
			else if (i == 1)
			{
				zTrans.x = CoverSlabHighDate.length * 0.5 + diameter * 0.5;
				zTrans.y = dSideCover + diameter * 0.5 + (CoverSlabHighDate.width - CoverSlabLowDate.width) * 0.5;
				zTrans.z = zLen;
			}
			else if (i == 2)
			{
				zTrans.x = dSideCover + diameter * 0.5;
				zTrans.y = CoverSlabHighDate.width * 0.5;
				zTrans.z = zLen;
				if (GetvvecEndType().at(i).at(0).endType != 0 || GetvvecEndType().at(i).at(1).endType != 0)
				{
					zTrans.z += RebarCode::GetPinRadius(strSize, modelRef, false) * 0.5 + RebarCode::GetBarDiameter(strSize, modelRef) * 0.5;
				}

				if (GetRebarLevelNum() > 4)
				{
					WString strSizeFour = GetvecDirSize().at(3);
					if (strSizeFour.find(L"mm") != WString::npos)
					{
						strSizeFour.ReplaceAll(L"mm", L"");
					}

					WString strSizeSec = GetvecDirSize().at(2);
					if (strSizeSec.find(L"mm") != WString::npos)
					{
						strSizeSec.ReplaceAll(L"mm", L"");
					}

					zTrans.x = CoverSlabHighDate.length * 0.5;
					zTrans.y = dSideCover + diameter * 0.5;
					zTrans.z = zLen + RebarCode::GetBarDiameter(strSizeFour, modelRef) + RebarCode::GetBarDiameter(strSizeSec, modelRef);
				}

			}
			else if (i == 3)
			{
				WString strSizeSec = GetvecDirSize().at(2);
				if (strSizeSec.find(L"mm") != WString::npos)
				{
					strSizeSec.ReplaceAll(L"mm", L"");
				}

				zTrans.x = CoverSlabLowDate.length * 0.5 + (CoverSlabHighDate.length - CoverSlabLowDate.length) * 0.5;
				zTrans.y = dSideCover + diameter * 0.5;
				zTrans.z = zLen;
				if (GetvvecEndType().at(2).at(0).endType != 0 || GetvvecEndType().at(2).at(1).endType != 0)
				{
					zTrans.z += RebarCode::GetPinRadius(strSizeSec, modelRef, false) * 0.5 + RebarCode::GetBarDiameter(strSizeSec, modelRef) * 0.5;
				}
				if (GetRebarLevelNum() > 4)
				{
					WString strSizeSec = GetvecDirSize().at(2);
					if (strSizeSec.find(L"mm") != WString::npos)
					{
						strSizeSec.ReplaceAll(L"mm", L"");
					}
					zTrans.z += RebarCode::GetBarDiameter(strSizeSec, modelRef);
				}

				m_zLen = zTrans.z;
			}
			else
			{
				zTrans.x = dSideCover + diameter * 0.5;
				zTrans.y = CoverSlabHighDate.width * 0.5;
				zTrans.z = zLen;

				WString strSizeSec = GetvecDirSize().at(2);
				if (strSizeSec.find(L"mm") != WString::npos)
				{
					strSizeSec.ReplaceAll(L"mm", L"");
				}
				zTrans.z = zLen + RebarCode::GetBarDiameter(strSizeSec, modelRef);
			}

			zLen += diameter * 0.5;
			vTransform.push_back(zTrans);
		}
	}
}

bool STCoverslabRebarAssembly::SetCoverSlabData(ElementHandleCR eh)
{
#ifdef PDMSIMPORT
	WString strName;
	WString strType;
	DgnECManagerR ecMgr = DgnECManager::GetManager();
	FindInstancesScopePtr scope = FindInstancesScope::CreateScope(eh, FindInstancesScopeOption(DgnECHostType::Element));
	ECQueryPtr            ecQuery = ECQuery::CreateQuery(ECQUERY_PROCESS_SearchAllClasses);
	//ECQUERY_PROCESS_SearchAllExtrinsic will only search ECXAttr
	ecQuery->SetSelectProperties(true);

	for (DgnECInstancePtr instance : ecMgr.FindInstances(*scope, *ecQuery))
	{
		DgnElementECInstanceP elemInst = instance->GetAsElementInstance();
		for (ECPropertyP ecProp : elemInst->GetClass().GetProperties())
		{
			WString strLabel = ecProp->GetDisplayLabel().GetWCharCP();
			if (strLabel != L"NAME" && strLabel != L"TYPE")
			{
				continue;
			}

			WString valStr;
			ECValue ecVal;
			elemInst->GetValue(ecVal, ecProp->GetName().GetWCharCP());
			IDgnECTypeAdapterR typeAdapter = IDgnECTypeAdapter::GetForProperty(*ecProp);
			IDgnECTypeAdapterContextPtr typeContext = IDgnECTypeAdapterContext::Create(*ecProp, *elemInst, ecProp->GetName().GetWCharCP());
			typeAdapter.ConvertToString(valStr, ecVal, *typeContext);

			if (strLabel == L"NAME")
				strName = valStr;
			else
				strType = valStr;
		}
	}
	string name = StringOperator::Convert::WStringToString(strName.c_str());
	//读取数据
//	ComponentDataManage dataManage("C:\\Users\\Power-Itech-LX\\Desktop\\data\\1BFX15--VB-.bim"); // 构件数据管理
	ComponentDataManage dataManage("C:\\Users\\Power-Itech-LX\\Desktop\\1BFX-----CW\\1BFX-----B\\data\\1BFX2078VB-.bim"); // 构件数据管理
	dataManage.GetAllComponentData();

	vector<IComponent*> vecComponent = ComponentDataManage::GetAllComponent();
	for (size_t i = 0; i < vecComponent.size(); i++)
	{
		IComponent *Component = vecComponent[i];
		string strComponentName = Component->GetEleName();
		string strType = Component->GetEleType();
		if (strType == "STWALL")
		{
			STWallComponent* stWall = dynamic_cast<STWallComponent*>(Component);
			if (stWall == NULL)
			{
				continue;
			}
			string strComponentName = stWall->GetEleName();
			if (name == strComponentName)
			{
				pWall = stWall;
			}
		}
	}
	if (pWall == NULL || !eh.IsValid())
	{
		return false;
	}

	double uor_per_mm = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	m_STwallData.length = pWall->GetPtStart().Distance(pWall->GetPtEnd()) * uor_per_mm;
	m_STwallData.width = pWall->GetWidth() * uor_per_mm;
	m_STwallData.height = pWall->GetHeight() * uor_per_mm;

	DPoint3d ptStart = pWall->GetPtStart();
	ptStart.x *= uor_per_mm;
	ptStart.y *= uor_per_mm;
	ptStart.z *= uor_per_mm;
	DPoint3d ptEnd = pWall->GetPtEnd();
	ptEnd.x *= uor_per_mm;
	ptEnd.y *= uor_per_mm;
	ptEnd.z *= uor_per_mm;


	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
	//由于该线为底面中心线，需将原点进行偏移1/2的厚度
	//以yVec的反反向进行偏移1/2的墙厚
	CVector3D  yVecNegate = yVec;
	yVecNegate.Negate();
	yVecNegate.Normalize();
	yVecNegate.ScaleToLength(m_STwallData.width * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	ptStart.Add(yVecNegate);
	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(ptStart, ptEnd);

	// 	CVector3D  xVecNegate = xVecNew;
	// 	xVecNegate.Normalize();
	// 	xVecNegate.ScaleToLength(m_STwallData.length * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	// 	ptStart.Add(xVecNegate);
	// 	CVector3D  zVecNegate = CVector3D::kZaxis;
	// 	zVecNegate.Normalize();
	// 	zVecNegate.ScaleToLength(m_STwallData.height * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	// 	ptStart.Add(zVecNegate);
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
#else
	//	m_eeh1 = eh;
	bool bRet = AnalyzingSICoverSlabData(eh);
	if (!bRet)
		return false;

	DPoint3d ptStart = m_STCoverSlabData.ptStart;            //原点
	DPoint3d ptEnd = m_STCoverSlabData.ptEnd;
	CVector3D  xVec(ptStart, ptEnd);
	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);     //返回两个向量的（标量）叉积。y

	m_LineNormal = yVec;
	m_LineNormal.Normalize();
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴
//	BeMatrix   placement = CMatrix3D::Ucs(ptStart, CVector3D::kXaxis, CVector3D::kYaxis, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
#endif
}

bool STCoverslabRebarAssembly::AnalyzingSICoverSlabData(ElementHandleCR eh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	DPoint3d FrontStr, FrontEnd,temp;
	DPoint3d BackStr, BackEnd;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();


	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	// EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	vector<DSegment3d> vecHighFontLine;
	vector<DSegment3d> vecDownBackLine;
	Eleeh.Duplicate(testeeh);
	GetHighorDownLine(Eleeh, vecHighFontLine, vecDownBackLine, &m_STCoverSlabData.height,true);
	AnalyzingAllFace(Eleeh);
	if (vecHighFontLine.empty() || vecDownBackLine.empty())
	{
		return false;
	}

	DPoint3d pt1[2];
	vecHighFontLine[0].GetStartPoint(pt1[0]);
	vecHighFontLine[0].GetEndPoint(pt1[1]);

	DPoint3d pt2[2];
	vecDownBackLine[0].GetStartPoint(pt2[0]);
	vecDownBackLine[0].GetEndPoint(pt2[1]);

	if (vecDownBackLine.size() > 1 || vecHighFontLine.size() > 1)
	{
		GetMaxDownFacePts(vecHighFontLine, vecDownBackLine, pt1, pt2);
	}

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_STCoverSlabData.height);

	m_STCoverSlabData.height = m_STCoverSlabData.height*uor_now / uor_ref;
	m_STCoverSlabData.width = FrontStr.Distance(BackStr)*uor_now / uor_ref;
	m_STCoverSlabData.length = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;
	CoverSlabHighDate.length = m_STCoverSlabData.length;
	CoverSlabHighDate.width = m_STCoverSlabData.width;
	temp = FrontStr;
	temp.z = temp.z - m_STCoverSlabData.height;
	m_STCoverSlabData.ptStart = temp;
	temp = FrontEnd;
	temp.z = temp.z - m_STCoverSlabData.height;
	m_STCoverSlabData.ptEnd = temp;

	vector<DSegment3d> vecHighFontLine1;
	vector<DSegment3d> vecDownBackLine1;

	GetHighorDownLine(Eleeh, vecHighFontLine1, vecDownBackLine1, &m_STCoverSlabData.height, false);//底面
	if (vecHighFontLine1.empty() || vecDownBackLine1.empty())
	{
		return false;
	}

	
	vecHighFontLine1[0].GetStartPoint(pt1[0]);
	vecHighFontLine1[0].GetEndPoint(pt1[1]);


	vecDownBackLine1[0].GetStartPoint(pt2[0]);
	vecDownBackLine1[0].GetEndPoint(pt2[1]);

	if (vecDownBackLine1.size() > 1 || vecHighFontLine1.size() > 1)
	{
		GetMaxDownFacePts(vecHighFontLine1, vecDownBackLine1, pt1, pt2);
	}

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_STCoverSlabData.height);

	CoverSlabLowDate.length = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;
	CoverSlabLowDate.width = FrontStr.Distance(BackStr)*uor_now / uor_ref;

	return true;
}
long STCoverslabRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool STCoverslabRebarAssembly::GetHighorDownLine(ElementHandleCR eeh, vector<DSegment3d>& vec_linefront, vector<DSegment3d>& vec_lineback, double* tHeight, bool chooseface)
{
	DPoint3d ptBegin, ptOver;
	vector<DPoint3d> vecPoints;
	vector<MSElementDescrP> vec_line;
	vector<MSElementDescrP> vec_line1;
	vector<MSElementDescrP> vec_line2;
	EditElementHandle eehFace;
	GetHighorDownFace(eeh, eehFace, tHeight,chooseface);
	if (!ExtractFacesTool::GetTwoLineFromDownFace(eehFace, vec_line, vec_line1, vec_line2, ptBegin, ptOver, vecPoints, eeh.GetModelRef()))
		return false;
	for (MSElementDescrP ms : vec_line)
		mdlElmdscr_freeAll(&ms);

	if (vec_line1.empty() || vec_line2.empty())
		return false;

	//	EditElementHandle eh(vec_line1[0], true, true, ACTIVEMODEL);
	//	eh.AddToModel();

	for (MSElementDescrP ms : vec_line1)
	{
		DPoint3d pt1[2];
		mdlLinear_extract(pt1, NULL, &ms->el, eeh.GetModelRef());
		DVec3d vec = pt1[1] - pt1[0];
		vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
		vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
		vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
		DVec3d vecX = DVec3d::From(1, 0, 0);
		vec.Normalize();
		if (vec.IsPerpendicularTo(vecX))
		{
			if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) < 0)
			{
				vec_linefront.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_linefront.push_back({ pt1[1],pt1[0] });
			}
		}
		else
		{
			if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) < 0)
			{
				vec_linefront.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_linefront.push_back({ pt1[1],pt1[0] });
			}
		}
		mdlElmdscr_freeAll(&ms);
	}

	for (MSElementDescrP ms : vec_line2)
	{
		DPoint3d pt1[2];
		mdlLinear_extract(pt1, NULL, &ms->el, eeh.GetModelRef());
		DVec3d vec = pt1[1] - pt1[0];
		vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
		vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
		vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
		DVec3d vecX = DVec3d::From(1, 0, 0);
		vec.Normalize();
		if (vec.IsPerpendicularTo(vecX))
		{
			if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) < 0)
			{
				vec_lineback.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_lineback.push_back({ pt1[1],pt1[0] });
			}
		}
		else
		{
			if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) < 0)
			{
				vec_lineback.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_lineback.push_back({ pt1[1],pt1[0] });
			}
		}
		mdlElmdscr_freeAll(&ms);
	}
	return true;
}

void STCoverslabRebarAssembly::GetHighorDownFace(ElementHandleCR eeh, EditElementHandleR DownFace, double* tHeight,bool chooseface)
{

	ISolidKernelEntityPtr entityPtr;
	if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS) // 从可以表示单个线、片或实体的元素创建实体
	{
		DRange3d range;
		Dpoint3d lowPt;
		Dpoint3d highPt;
		BentleyStatus nStatus = SolidUtil::GetEntityRange(range, *entityPtr); // 获取给定主体的轴对齐边界框
		if (SUCCESS == nStatus)
		{
			if (tHeight != NULL)
				*tHeight = range.ZLength();
			lowPt = range.low;
			highPt = range.high;
		}

		bvector<ISubEntityPtr> subEntities;
		size_t iSubEntityNum = SolidUtil::GetBodyFaces(&subEntities, *entityPtr); // 查询输入体的面集 

		if (iSubEntityNum > 0)
		{
			size_t iSize = subEntities.size();
			for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
			{
				ISubEntityPtr subEntity = subEntities[iIndex];

				DRange3d tmprange;
				nStatus = SolidUtil::GetSubEntityRange(tmprange, *subEntity); // 获取给定面或边的轴对齐边界框。 
				if (SUCCESS == nStatus)
				{

					if (chooseface)
					{
						if (abs(tmprange.low.z - range.high.z) < 10)//最底面
						{
							CurveVectorPtr  curves;
							// 创建给定子实体的简化 CurveVector 表示
							SolidUtil::Convert::SubEntityToCurveVector(curves, *subEntity);
							if (curves != NULL)
							{
								// 从表示点串、开放曲线、闭合曲线或区域的 CurveVector 创建单个元素 
								DraftingElementSchema::ToElement(DownFace, *curves, nullptr, eeh.GetModelRef()->Is3d(), *eeh.GetModelRef());
								if (DownFace.IsValid())
								{
									return;
								}
							}
							else
							{
								IGeometryPtr geom;
								// 创建给定子实体（非 BRep 几何体）的简化 IGeometryPtr 表示 
								SolidUtil::Convert::SubEntityToGeometry(geom, *subEntity, *eeh.GetModelRef());
								ISolidPrimitivePtr tmpPtr = geom->GetAsISolidPrimitive();
								if (tmpPtr != NULL)
								{

									if (SUCCESS == DraftingElementSchema::ToElement(DownFace, *tmpPtr, nullptr, *eeh.GetModelRef()))
									{
										return;
									}
								}
							}
						}
					}
					else
					{
						if (abs(tmprange.high.z - range.low.z) < 10)//最底面
						{
							CurveVectorPtr  curves;
							// 创建给定子实体的简化 CurveVector 表示
							SolidUtil::Convert::SubEntityToCurveVector(curves, *subEntity);
							if (curves != NULL)
							{
								// 从表示点串、开放曲线、闭合曲线或区域的 CurveVector 创建单个元素 
								DraftingElementSchema::ToElement(DownFace, *curves, nullptr, eeh.GetModelRef()->Is3d(), *eeh.GetModelRef());
								if (DownFace.IsValid())
								{
									return;
								}
							}
							else
							{
								IGeometryPtr geom;
								// 创建给定子实体（非 BRep 几何体）的简化 IGeometryPtr 表示 
								SolidUtil::Convert::SubEntityToGeometry(geom, *subEntity, *eeh.GetModelRef());
								ISolidPrimitivePtr tmpPtr = geom->GetAsISolidPrimitive();
								if (tmpPtr != NULL)
								{

									if (SUCCESS == DraftingElementSchema::ToElement(DownFace, *tmpPtr, nullptr, *eeh.GetModelRef()))
									{
										return;
									}
								}
							}
						}
					}
				}
			}
		}

	}





}

void STCoverslabRebarAssembly::AnalyzingAllFace(ElementHandleCR eeh)
{

	ISolidKernelEntityPtr entityPtr;
	if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS) // 从可以表示单个线、片或实体的元素创建实体
	{
		DRange3d range;
		BentleyStatus nStatus = SolidUtil::GetEntityRange(range, *entityPtr); // 获取给定主体的轴对齐边界框
		bvector<ISubEntityPtr> subEntities;
		size_t iSubEntityNum = SolidUtil::GetBodyFaces(&subEntities, *entityPtr); // 查询输入体的面集 

		if (iSubEntityNum > 0)
		{
			size_t iSize = subEntities.size();
			for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
			{
				ISubEntityPtr subEntity = subEntities[iIndex];

				DRange3d tmprange;
				nStatus = SolidUtil::GetSubEntityRange(tmprange, *subEntity); // 获取给定面或边的轴对齐边界框。 
				if (SUCCESS == nStatus)
				{
					if (abs(tmprange.high.z - tmprange.low.z) < 10) // 排除XOY面
					{
						continue;
					}
					if (abs(tmprange.high.z - range.high.z) < 10 && abs(tmprange.low.z - range.low.z) < 10)
					{
						continue;
					}

					if ((abs(tmprange.high.z - range.high.z) < 10)&& !(abs(tmprange.low.z - range.low.z) < 10))//最底面
					{
						CoverSlabHighDate.height = tmprange.high.z - tmprange.low.z;
					}

					if ((abs(tmprange.low.z - range.low.z) < 10) && !(abs(tmprange.high.z - range.high.z) < 10))//最底面
					{

						CoverSlabLowDate.height = tmprange.high.z - tmprange.low.z;
					}

				}
			}
		}

	}

}



bool SZCoverslabRebarAssembly::OnDoubleClick()
{
	vector<PIT::ConcreteRebar> vecRebarData;
	vector<PIT::LapOptions> vecLaptionData;
	vector<PIT::EndType> vecEndTypeData;
	vector<TwinBarSet::TwinBarLevelInfo> vecTwinBarData;
	//	Concrete concreteData;
	//	TieReBarInfo tieRebarInfo;
		// 	GetRebarData(vecData);
		// 	GetConcreteData(concreteData);
	CoverSlabType slabType = GetCoverslabType();
	// 	int lastAction = ACTIONBUTTON_CANCEL;
	// 	if (SUCCESS != mdlDialog_openModal(&lastAction, GetResourceHandle(), DIALOGID_WallRebar) || lastAction != ACTIONBUTTON_OK)
	// 		return false;

	ElementId testid = FetchConcrete();
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

	SetSelectedModel(modelp);
	GetConcreteXAttribute(testid, ACTIVEMODEL);

	DgnModelRefP modelRef = ACTIVEMODEL;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	pSZCoverslabDoubleRebarDlg = new CoverslabRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pSZCoverslabDoubleRebarDlg->Create(IDD_DIALOG_CoverslabRebar);
	pSZCoverslabDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pSZCoverslabDoubleRebarDlg->ShowWindow(SW_SHOW);

// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
// 	CoverslabRebarDlg dlg(ehSel, CWnd::FromHandle(MSWIND));
// 	//	ElementHandle eh(GetSelectedElement(), modelRef);
// 	dlg.SetSelectElement(ehSel);
// 	dlg.SetConcreteId(FetchConcrete());
// 	if (IDCANCEL == dlg.DoModal())
// 		return false;

	return true;
}

bool SZCoverslabRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehslab(GetSelectedElement(), GetSelectedModel());
	if (!ehslab.IsValid())
		return false;

	DgnModelRefP modelRef = ehslab.GetModelRef();

	SetCoverSlabData(ehslab);

	MakeRebars(modelRef);//调用创建钢筋
	Save(modelRef);

	ElementId contid = FetchConcrete();
	return true;
}

void SZCoverslabRebarAssembly::CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef)         //计算转换
{
	if (modelRef == NULL)
	{
		return;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;             //单位

	vTransform.clear();
	double dSideCover = GetSideCover() * uor_per_mm; //侧
	double dPositiveCover = GetPositiveCover() * uor_per_mm; //正
	double dReverseCover = GetReverseCover() * uor_per_mm; //反
	double dLevelSpace = 0;
	double dOffset = dPositiveCover;

	double zLen = dReverseCover;
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}
		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10

		if (diameter > BE_TOLERANCE)
		{
			CVector3D	zTrans(0.0, 0.0, 0.0);
			// m_SICoverSlabData.width
			double levelSpacing = GetvecLevelSpace().at(i) * uor_per_mm;

			zLen += diameter * 0.5;
			if (COMPARE_VALUES_EPS(zLen + levelSpacing, m_SZCoverSlabData.height, EPS) > 0)
			{
				double diameter_Tol = 0.00;
				for (int j = 0; j < GetRebarLevelNum(); j++)
				{
					WString strSizeTmp = GetvecDirSize().at(j);
					if (strSizeTmp.find(L"mm") != WString::npos)
					{
						strSizeTmp.ReplaceAll(L"mm", L"");
					}
					if (i == 0)
					{
						diameter_Tol += RebarCode::GetBarDiameter(strSizeTmp, modelRef);
					}

					if (j > i && j != 5)
					{
						diameter_Tol += RebarCode::GetBarDiameter(strSizeTmp, modelRef);
					}
				}
				zLen = m_SZCoverSlabData.height - diameter_Tol - dPositiveCover - diameter * 0.5;
			}
			else
			{
				zLen += levelSpacing;
			}
			bool bFlag = true;

			if (i == 0)
			{
				zTrans.x = dSideCover + diameter * 0.5;
				zTrans.y = CoverSlabHighDate.width * 0.5;
				zTrans.z = zLen;
				bFlag = false;
			}
			else if (i == 1)
			{
				zTrans.x = dSideCover + diameter * 0.5;
				zTrans.y = CoverSlabHighDate.width * 0.5;
				zTrans.z = zLen;
			}
			else if (i == 2)
			{
				zTrans.x = CoverSlabHighDate.length * 0.5;
				zTrans.y = dSideCover + diameter * 0.5 + CoverSlabHighDate.morewidth;
				zTrans.z = zLen;
				bFlag = false;
			}
			else if (i == 3)
			{
				zTrans.x = CoverSlabHighDate.length * 0.5 - CoverSlabHighDate.morewidth;
				zTrans.y = dSideCover + diameter * 0.5;
				zTrans.z = zLen;
			}
			else if (i == 4)
			{
				zTrans.x = CoverSlabLowDate.length * 0.5;
				zTrans.y = dSideCover + diameter * 0.5;
				if (m_SZCoverFlag)
				{
					zTrans.y += CoverSlabHighDate.morewidth;
				}
				else
				{
					zTrans.y -= CoverSlabHighDate.morewidth;
				}
				zTrans.z = zLen;
				bFlag = false;
				m_zLen = zTrans.z;
			}
			else if (i == 5)
			{
				WString strSizeSix = GetvecDirSize().at(6);
				if (strSizeSix.find(L"mm") != WString::npos)
				{
					strSizeSix.ReplaceAll(L"mm", L"");
				}
				zTrans.x = CoverSlabLowDate.length * 0.5;
				zTrans.y = CoverSlabHighDate.width - diameter * 0.5 - RebarCode::GetBarDiameter(strSizeSix, modelRef) - dSideCover;
				if (m_SZCoverFlag)
				{
					zTrans.y += CoverSlabHighDate.morewidth;
				}
				else
				{
					zTrans.y -= CoverSlabHighDate.morewidth;
				}
				zTrans.z = zLen;
			}
			else if (i == 6)
			{
				zTrans.x = dSideCover + diameter * 0.5 - CoverSlabHighDate.morewidth;
				zTrans.y = CoverSlabHighDate.width * 0.5;
				zTrans.z = zLen;
				if (m_SZCoverFlag)
				{
					zTrans.y += CoverSlabHighDate.morewidth;
				}
				else
				{
					zTrans.y -= CoverSlabHighDate.morewidth;
				}
			}

			if (bFlag)
			{
				zLen += diameter * 0.5;
			}
			else
			{
				zLen -= diameter * 0.5;
			}

			vTransform.push_back(zTrans);
		}
	}
}

bool SZCoverslabRebarAssembly::SetCoverSlabData(ElementHandleCR eh)
{
#ifdef PDMSIMPORT
	WString strName;
	WString strType;
	DgnECManagerR ecMgr = DgnECManager::GetManager();
	FindInstancesScopePtr scope = FindInstancesScope::CreateScope(eh, FindInstancesScopeOption(DgnECHostType::Element));
	ECQueryPtr            ecQuery = ECQuery::CreateQuery(ECQUERY_PROCESS_SearchAllClasses);
	//ECQUERY_PROCESS_SearchAllExtrinsic will only search ECXAttr
	ecQuery->SetSelectProperties(true);

	for (DgnECInstancePtr instance : ecMgr.FindInstances(*scope, *ecQuery))
	{
		DgnElementECInstanceP elemInst = instance->GetAsElementInstance();
		for (ECPropertyP ecProp : elemInst->GetClass().GetProperties())
		{
			WString strLabel = ecProp->GetDisplayLabel().GetWCharCP();
			if (strLabel != L"NAME" && strLabel != L"TYPE")
			{
				continue;
			}

			WString valStr;
			ECValue ecVal;
			elemInst->GetValue(ecVal, ecProp->GetName().GetWCharCP());
			IDgnECTypeAdapterR typeAdapter = IDgnECTypeAdapter::GetForProperty(*ecProp);
			IDgnECTypeAdapterContextPtr typeContext = IDgnECTypeAdapterContext::Create(*ecProp, *elemInst, ecProp->GetName().GetWCharCP());
			typeAdapter.ConvertToString(valStr, ecVal, *typeContext);

			if (strLabel == L"NAME")
				strName = valStr;
			else
				strType = valStr;
		}
	}
	string name = StringOperator::Convert::WStringToString(strName.c_str());
	//读取数据
//	ComponentDataManage dataManage("C:\\Users\\Power-Itech-LX\\Desktop\\data\\1BFX15--VB-.bim"); // 构件数据管理
	ComponentDataManage dataManage("C:\\Users\\Power-Itech-LX\\Desktop\\1BFX-----CW\\1BFX-----B\\data\\1BFX2078VB-.bim"); // 构件数据管理
	dataManage.GetAllComponentData();

	vector<IComponent*> vecComponent = ComponentDataManage::GetAllComponent();
	for (size_t i = 0; i < vecComponent.size(); i++)
	{
		IComponent *Component = vecComponent[i];
		string strComponentName = Component->GetEleName();
		string strType = Component->GetEleType();
		if (strType == "STWALL")
		{
			STWallComponent* stWall = dynamic_cast<STWallComponent*>(Component);
			if (stWall == NULL)
			{
				continue;
			}
			string strComponentName = stWall->GetEleName();
			if (name == strComponentName)
			{
				pWall = stWall;
			}
		}
	}
	if (pWall == NULL || !eh.IsValid())
	{
		return false;
	}

	double uor_per_mm = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	m_STwallData.length = pWall->GetPtStart().Distance(pWall->GetPtEnd()) * uor_per_mm;
	m_STwallData.width = pWall->GetWidth() * uor_per_mm;
	m_STwallData.height = pWall->GetHeight() * uor_per_mm;

	DPoint3d ptStart = pWall->GetPtStart();
	ptStart.x *= uor_per_mm;
	ptStart.y *= uor_per_mm;
	ptStart.z *= uor_per_mm;
	DPoint3d ptEnd = pWall->GetPtEnd();
	ptEnd.x *= uor_per_mm;
	ptEnd.y *= uor_per_mm;
	ptEnd.z *= uor_per_mm;


	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
	//由于该线为底面中心线，需将原点进行偏移1/2的厚度
	//以yVec的反反向进行偏移1/2的墙厚
	CVector3D  yVecNegate = yVec;
	yVecNegate.Negate();
	yVecNegate.Normalize();
	yVecNegate.ScaleToLength(m_STwallData.width * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	ptStart.Add(yVecNegate);
	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(ptStart, ptEnd);

	// 	CVector3D  xVecNegate = xVecNew;
	// 	xVecNegate.Normalize();
	// 	xVecNegate.ScaleToLength(m_STwallData.length * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	// 	ptStart.Add(xVecNegate);
	// 	CVector3D  zVecNegate = CVector3D::kZaxis;
	// 	zVecNegate.Normalize();
	// 	zVecNegate.ScaleToLength(m_STwallData.height * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	// 	ptStart.Add(zVecNegate);
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
#else
	m_eeh1 = eh;
	bool bRet = AnalyzingSICoverSlabData(eh);
	if (!bRet)
		return false;

	DPoint3d ptStart = m_SZCoverSlabData.ptStart;            //原点
	DPoint3d ptEnd = m_SZCoverSlabData.ptEnd;
	CVector3D  xVec(ptStart, ptEnd);
	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);     //返回两个向量的（标量）叉积。y

	m_LineNormal = yVec;
	m_LineNormal.Normalize();
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴
//	BeMatrix   placement = CMatrix3D::Ucs(ptStart, CVector3D::kXaxis, CVector3D::kYaxis, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
#endif
}

bool SZCoverslabRebarAssembly::AnalyzingSICoverSlabData(ElementHandleCR eh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	DPoint3d FrontStr, FrontEnd,tempFrontStr;
	DPoint3d BackStr, BackEnd;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();

	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();

	// EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	//EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	vector<DSegment3d> vecHighFontLine;
	vector<DSegment3d> vecDownBackLine;
	GetHighorDownLine(testeeh, vecHighFontLine, vecDownBackLine, &m_SZCoverSlabData.height, true);
	AnalyzingAllFace(testeeh);
	if (vecHighFontLine.empty() || vecDownBackLine.empty())
	{
		return false;
	}

	DPoint3d pt1[2];
	vecHighFontLine[0].GetStartPoint(pt1[0]);
	vecHighFontLine[0].GetEndPoint(pt1[1]);

	DPoint3d pt2[2];
	vecDownBackLine[0].GetStartPoint(pt2[0]);
	vecDownBackLine[0].GetEndPoint(pt2[1]);

	if (vecDownBackLine.size() > 1 || vecHighFontLine.size() > 1)
	{
		GetMaxDownFacePts(vecHighFontLine, vecDownBackLine, pt1, pt2);
	}

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_SZCoverSlabData.height);
	tempFrontStr = FrontStr;
	m_SZCoverSlabData.height = m_SZCoverSlabData.height*uor_now / uor_ref;
	m_SZCoverSlabData.width = FrontStr.Distance(BackStr)*uor_now / uor_ref;
	m_SZCoverSlabData.length = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;
	CoverSlabHighDate.length = m_SZCoverSlabData.length;
	CoverSlabHighDate.width = m_SZCoverSlabData.width;

	DPoint3d HightStr = FrontStr;
	

	vector<DSegment3d> vecHighFontLine1;
	vector<DSegment3d> vecDownBackLine1;

	GetHighorDownLine(testeeh, vecHighFontLine1, vecDownBackLine1, &m_SZCoverSlabData.height, false);//底面
	if (vecHighFontLine1.empty() || vecDownBackLine1.empty())
	{
		return false;
	}

	vecHighFontLine1[0].GetStartPoint(pt1[0]);
	vecHighFontLine1[0].GetEndPoint(pt1[1]);

	vecDownBackLine1[0].GetStartPoint(pt2[0]);
	vecDownBackLine1[0].GetEndPoint(pt2[1]);

	if (vecDownBackLine1.size() > 1 || vecHighFontLine1.size() > 1)
	{
		GetMaxDownFacePts(vecHighFontLine1, vecDownBackLine1, pt1, pt2);
	}

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_SZCoverSlabData.height);

	CoverSlabLowDate.length = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;
	CoverSlabLowDate.width = FrontStr.Distance(BackStr)*uor_now / uor_ref;

	m_SZCoverSlabData.ptStart = FrontStr;
	m_SZCoverSlabData.ptEnd = FrontEnd;

	HightStr.z = FrontStr.z;
	CVector3D vecH = HightStr - FrontStr;
	CVector3D vecL = FrontEnd - FrontStr;
	vecH.Normalize();
	vecL.Normalize();
	if (abs(vecH.x + vecL.x) == abs(vecH.x) + abs(vecL.x) || abs(vecH.y + vecL.y) == abs(vecH.y) + abs(vecL.y))
	{
		m_SZCoverFlag = false;
	}

	double tempmorewidth = tempFrontStr.x - FrontStr.x;
	if(tempmorewidth<0)
		CoverSlabHighDate.morewidth = -tempmorewidth;
	else
		CoverSlabHighDate.morewidth = tempmorewidth;
	return true;
}

long SZCoverslabRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool SZCoverslabRebarAssembly::GetHighorDownLine(ElementHandleCR eeh, vector<DSegment3d>& vec_linefront, vector<DSegment3d>& vec_lineback, double* tHeight, bool chooseface)
{
	
	DPoint3d ptBegin, ptOver;
	vector<DPoint3d> vecPoints;
	vector<MSElementDescrP> vec_line;
	vector<MSElementDescrP> vec_line1;
	vector<MSElementDescrP> vec_line2;
	EditElementHandle eehFace;
	GetHighorDownFace(eeh, eehFace, tHeight, chooseface);
	if (!ExtractFacesTool::GetTwoLineFromDownFace(eehFace, vec_line, vec_line1, vec_line2, ptBegin, ptOver, vecPoints, eeh.GetModelRef()))
		return false;
	for (MSElementDescrP ms : vec_line)
		mdlElmdscr_freeAll(&ms);

	if (vec_line1.empty() || vec_line2.empty())
		return false;

	//	EditElementHandle eh(vec_line1[0], true, true, ACTIVEMODEL);
	//	eh.AddToModel();

	for (MSElementDescrP ms : vec_line1)
	{
		DPoint3d pt1[2];
		mdlLinear_extract(pt1, NULL, &ms->el, eeh.GetModelRef());
		DVec3d vec = pt1[1] - pt1[0];
		vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
		vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
		vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
		DVec3d vecX = DVec3d::From(1, 0, 0);
		vec.Normalize();
		if (vec.IsPerpendicularTo(vecX))
		{
			if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) < 0)
			{
				vec_linefront.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_linefront.push_back({ pt1[1],pt1[0] });
			}
		}
		else
		{
			if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) < 0)
			{
				vec_linefront.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_linefront.push_back({ pt1[1],pt1[0] });
			}
		}
		mdlElmdscr_freeAll(&ms);
	}

	for (MSElementDescrP ms : vec_line2)
	{
		DPoint3d pt1[2];
		mdlLinear_extract(pt1, NULL, &ms->el, eeh.GetModelRef());
		DVec3d vec = pt1[1] - pt1[0];
		vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
		vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
		vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
		DVec3d vecX = DVec3d::From(1, 0, 0);
		vec.Normalize();
		if (vec.IsPerpendicularTo(vecX))
		{
			if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) < 0)
			{
				vec_lineback.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_lineback.push_back({ pt1[1],pt1[0] });
			}
		}
		else
		{
			if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) < 0)
			{
				vec_lineback.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_lineback.push_back({ pt1[1],pt1[0] });
			}
		}
		mdlElmdscr_freeAll(&ms);
	}
	return true;
}

void SZCoverslabRebarAssembly::GetHighorDownFace(ElementHandleCR eeh, EditElementHandleR DownFace, double* tHeight, bool chooseface)
{
	//通过chooseface选择获得上面还是下面的面true底面false上面
	ISolidKernelEntityPtr entityPtr;
	if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS) // 从可以表示单个线、片或实体的元素创建实体
	{
		DRange3d range;
		Dpoint3d lowPt;
		Dpoint3d highPt;
		BentleyStatus nStatus = SolidUtil::GetEntityRange(range, *entityPtr); // 获取给定主体的轴对齐边界框
		if (SUCCESS == nStatus)
		{
			if (tHeight != NULL)
				*tHeight = range.ZLength();
			lowPt = range.low;
			highPt = range.high;
		}

		bvector<ISubEntityPtr> subEntities;
		size_t iSubEntityNum = SolidUtil::GetBodyFaces(&subEntities, *entityPtr); // 查询输入体的面集 

		if (iSubEntityNum > 0)
		{
			size_t iSize = subEntities.size();
			for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
			{
				ISubEntityPtr subEntity = subEntities[iIndex];

				DRange3d tmprange;
				nStatus = SolidUtil::GetSubEntityRange(tmprange, *subEntity); // 获取给定面或边的轴对齐边界框。 
				if (SUCCESS == nStatus)
				{

					if (chooseface)
					{
						if (abs(tmprange.low.z - range.high.z) < 10)//最底面
						{
							CurveVectorPtr  curves;
							// 创建给定子实体的简化 CurveVector 表示
							SolidUtil::Convert::SubEntityToCurveVector(curves, *subEntity);
							if (curves != NULL)
							{
								// 从表示点串、开放曲线、闭合曲线或区域的 CurveVector 创建单个元素 
								DraftingElementSchema::ToElement(DownFace, *curves, nullptr, eeh.GetModelRef()->Is3d(), *eeh.GetModelRef());
								if (DownFace.IsValid())
								{
									return;
								}
							}
							else
							{
								IGeometryPtr geom;
								// 创建给定子实体（非 BRep 几何体）的简化 IGeometryPtr 表示 
								SolidUtil::Convert::SubEntityToGeometry(geom, *subEntity, *eeh.GetModelRef());
								ISolidPrimitivePtr tmpPtr = geom->GetAsISolidPrimitive();
								if (tmpPtr != NULL)
								{

									if (SUCCESS == DraftingElementSchema::ToElement(DownFace, *tmpPtr, nullptr, *eeh.GetModelRef()))
									{
										return;
									}
								}
							}
						}
					}
					else
					{
						if (abs(tmprange.high.z - range.low.z) < 10)//最底面
						{
							CurveVectorPtr  curves;
							// 创建给定子实体的简化 CurveVector 表示
							SolidUtil::Convert::SubEntityToCurveVector(curves, *subEntity);
							if (curves != NULL)
							{
								// 从表示点串、开放曲线、闭合曲线或区域的 CurveVector 创建单个元素 
								DraftingElementSchema::ToElement(DownFace, *curves, nullptr, eeh.GetModelRef()->Is3d(), *eeh.GetModelRef());
								if (DownFace.IsValid())
								{
									return;
								}
							}
							else
							{
								IGeometryPtr geom;
								// 创建给定子实体（非 BRep 几何体）的简化 IGeometryPtr 表示 
								SolidUtil::Convert::SubEntityToGeometry(geom, *subEntity, *eeh.GetModelRef());
								ISolidPrimitivePtr tmpPtr = geom->GetAsISolidPrimitive();
								if (tmpPtr != NULL)
								{

									if (SUCCESS == DraftingElementSchema::ToElement(DownFace, *tmpPtr, nullptr, *eeh.GetModelRef()))
									{
										return;
									}
								}
							}
						}
					}
				}
			}
		}

	}
}

void SZCoverslabRebarAssembly::AnalyzingAllFace(ElementHandleCR eeh)//分析上层和下层的板高度
{
	ISolidKernelEntityPtr entityPtr;
	if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS) // 从可以表示单个线、片或实体的元素创建实体
	{
		DRange3d range;
		BentleyStatus nStatus = SolidUtil::GetEntityRange(range, *entityPtr); // 获取给定主体的轴对齐边界框
		bvector<ISubEntityPtr> subEntities;
		size_t iSubEntityNum = SolidUtil::GetBodyFaces(&subEntities, *entityPtr); // 查询输入体的面集 

		if (iSubEntityNum > 0)
		{
			size_t iSize = subEntities.size();
			for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
			{
				ISubEntityPtr subEntity = subEntities[iIndex];

				DRange3d tmprange;
				nStatus = SolidUtil::GetSubEntityRange(tmprange, *subEntity); // 获取给定面或边的轴对齐边界框。 
				if (SUCCESS == nStatus)
				{
					if ((abs(tmprange.high.z - range.high.z) < 10) && !(abs(tmprange.low.z - range.high.z) < 10))//最底面
					{
						CoverSlabHighDate.height = tmprange.high.z - tmprange.low.z;
					}
					if ((abs(tmprange.low.z - range.low.z) < 10) && !(abs(tmprange.high.z - range.low.z) < 10))//最底面
					{
						CoverSlabLowDate.height = tmprange.high.z - tmprange.low.z;
					}
				}
			}
		}
	}

}

bool SZCoverslabRebarAssembly::makeRebarCurve_SZ
(
	vector<PITRebarCurve>&     rebars,
	double                  xPos,
	double                  yLen,
	double					startOffset,
	double					endOffset,
	PITRebarEndTypes&		endTypes,
	CMatrix3D const&        mat
	, bool isTwin
)
{

	CPoint3D  startPt;
	CPoint3D  endPt;

	//不允许为负值
// 	if (startPt < 0)
// 		startPt = 0;
// 	if (endOffset < 0)
// 		endOffset = 0;

		startPt = CPoint3D::From(0, -yLen / 2.0, xPos);
		endPt = CPoint3D::From(0, yLen / 2.0, xPos);

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	//eeh.AddToModel();

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




	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}
	if (!isTwin)
	{
		m_vecRebarPtsLayer.push_back(pt1[0]);
		m_vecRebarPtsLayer.push_back(pt1[1]);
	}
	
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetSideCover()*uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);

	/*EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
	eehline.AddToModel();*/

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
		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itr->second);
		vex->SetType(RebarVertex::kStart);
		endTypes.beg.SetptOrgin(itr->second);

		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}

		endTypes.end.SetptOrgin(itrplus->second);

		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itrplus->second);
		vex->SetType(RebarVertex::kEnd);

		rebar.EvaluateEndTypes(endTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebars.push_back(rebar);
	}


	//rebar.DoMatrix(mat);
	return true;
}

bool SZCoverslabRebarAssembly::makeRebarCurve
(
	vector<PITRebarCurve>&		rebars,
	double						xPos,
	double						yLen,
	double						startOffset,
	double						endOffset,
	const vector<PIT::EndType>&	vecEndtype,
	PITRebarEndTypes&			endTypes,
	CMatrix3D const&			mat
)
{
	CPoint3D  startPt;
	CPoint3D  endPt;

	if (cover_direction == 3)
	{
		WString strSizeTmp1 = GetvecDirSize().at(0);
		if (strSizeTmp1.find(L"mm") != WString::npos)
		{
			strSizeTmp1.ReplaceAll(L"mm", L"");
		}
		xPos += RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL);
	}

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	startPt = CPoint3D::From(xPos, -yLen / 2.0 + startOffset, 0.0);
	endPt = CPoint3D::From(xPos, yLen / 2.0 - endOffset, 0.0);

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	//	eeh.AddToModel();

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

	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}

	// EditElementHandle eeh2;
	// GetContractibleeeh(eeh2);//获取减去保护层的端部缩小的实体

	double dSideCover = GetSideCover()*uor_per_mm;
	vector<DPoint3d> tmppts;
	vector<DPoint3d> tmpptsTmp;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	//	GetIntersectPointsWithOldElm(tmpptsTmp, m_pOldElm, pt1[0], pt1[1], dSideCover, matrix);
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);

	//EditElementHandle eehline;
	//LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
	//eehline.AddToModel();

	map<int, DPoint3d> map_pts;
	for (DPoint3d pt : tmppts)
	{
		int dis = (int)pt1[0].Distance(pt);
		if (dis == 0 || dis == (int)pt1[0].Distance(pt1[1]))//防止过滤掉起点和终点
		{
			dis = dis + 1;
		}

		map_pts[dis] = pt;
	}
	map_pts[0] = pt1[0];
	map_pts[(int)pt1[0].Distance(pt1[1])] = pt1[1];

	RebarVertices  vers;
	bvector<DPoint3d> allpts;

	double dPositiveCover = GetPositiveCover()*uor_per_mm;//下
	double dReverseCover = GetReverseCover()*uor_per_mm;//上

	CVector3D  tempVec(pt1[1], pt1[0]);
	CVector3D  arcVec = tempVec;
	Dpoint3d temp;
	//每层钢筋单独弯曲画线
	switch (cover_direction)
	{
	case  0:
	{
		temp = pt1[0];
		arcVec.Negate();
		arcVec.Normalize();
		arcVec.ScaleToLength(CoverSlabHighDate.morewidth);
		temp.Add(arcVec);

		temp.z += (m_SZCoverSlabData.height - dReverseCover - dPositiveCover - m_diameter1);// 弯曲长度=减去钢筋直径和上面保护层
		allpts.push_back(temp);

		arcVec.Normalize();
		arcVec.ScaleToLength(pt1[0].Distance(pt1[1]) - CoverSlabHighDate.morewidth + endOffset - m_diameter1 * 0.5);
		temp.Add(arcVec);
		allpts.push_back(temp);

		temp.z -= (m_SZCoverSlabData.height - dReverseCover - dPositiveCover - m_diameter1);// 弯曲长度=减去钢筋直径和上面保护层
		allpts.push_back(temp);

		arcVec.Normalize();
		arcVec.Negate();
		arcVec.ScaleToLength(temp.Distance(pt1[0]) - CoverSlabHighDate.morewidth - m_diameter1 + startOffset + endOffset);
		temp.Add(arcVec);
		allpts.push_back(temp);

		temp.z += (m_SZCoverSlabData.height - dReverseCover - dPositiveCover - m_diameter1 * 0.5 - endOffset);// 弯曲长度=减去钢筋直径和上面保护层
		allpts.push_back(temp);
		break;
	}
	case 1:
	{
		temp = pt1[1];

		temp.z += (CoverSlabLowDate.height - dReverseCover - dPositiveCover - m_diameter1);// 弯曲长度=减去钢筋直径和上面保护层
		allpts.push_back(temp);

		arcVec.Normalize();
		arcVec.ScaleToLength(pt1[1].Distance(pt1[0]) + endOffset - m_diameter1 * 0.5);
		temp.Add(arcVec);
		allpts.push_back(temp);

		temp.z -= (CoverSlabLowDate.height - dReverseCover - dPositiveCover - m_diameter1);// 弯曲长度=减去钢筋直接和上面保护层
		allpts.push_back(temp);

		arcVec.Normalize();
		arcVec.Negate();
		arcVec.ScaleToLength(pt1[1].Distance(pt1[0]) - m_diameter1 + startOffset + endOffset);
		temp.Add(arcVec);
		allpts.push_back(temp);

		temp.z += (CoverSlabLowDate.height - dReverseCover - dPositiveCover - m_diameter1 * 0.5 - endOffset);// 弯曲长度=减去钢筋直接和上面保护层
		allpts.push_back(temp);
		break;
	}
	case 2:
	{
		arcVec.Normalize();
		arcVec.Negate();
		arcVec.ScaleToLength(m_diameter1 * 0.5);
		(pt1[0]).Add(arcVec);
		temp = pt1[0];

		WString strSizeTmp1 = GetvecDirSize().at(0);
		if (strSizeTmp1.find(L"mm") != WString::npos)
		{
			strSizeTmp1.ReplaceAll(L"mm", L"");
		}
		temp.z += CoverSlabLowDate.height - m_diameter1 * 0.5 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) + dReverseCover;
		allpts.push_back(temp);
		allpts.push_back(pt1[0]);

		temp = pt1[0];
		arcVec.Normalize();
		arcVec.ScaleToLength(pt1[0].Distance(pt1[1]) - m_diameter1 * 0.5 - CoverSlabHighDate.morewidth * 2);
		temp.Add(arcVec);
		allpts.push_back(temp);

		temp.z += CoverSlabLowDate.height - m_diameter1 * 0.5 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) + dReverseCover;
		allpts.push_back(temp);

		GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
		PITRebarCurve rebar;
		rebar.SetVertices(vers);
		rebars.push_back(rebar);

		bvector<DPoint3d> allptsTmp;

		temp = pt1[0];
		temp.z += m_SZCoverSlabData.height - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) * 2 - dReverseCover - dPositiveCover - m_diameter1;
		DPoint3d ptTemp = temp;
		temp.z -= CoverSlabHighDate.height - m_diameter1 * 0.5 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) - dReverseCover;
		allptsTmp.push_back(temp);
		allptsTmp.push_back(ptTemp);
		temp = ptTemp;

		arcVec.Normalize();
		arcVec.ScaleToLength(pt1[0].Distance(pt1[1]) - m_diameter1 * 0.5 - CoverSlabHighDate.morewidth * 2);
		temp.Add(arcVec);
		allptsTmp.push_back(temp);

		temp.z -= CoverSlabHighDate.height - m_diameter1 * 0.5 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) - dReverseCover;
		allptsTmp.push_back(temp);

		RebarVertices  versTmp;
		GetRebarVerticesFromPoints(versTmp, allptsTmp, m_diameter1);
		PITRebarCurve rebar1;
		rebar1.SetVertices(versTmp);
		rebars.push_back(rebar1);
		
		return true;
	}
	case 3:
	{
		allpts.push_back(pt1[0]);
		allpts.push_back(pt1[1]);

		endTypes.beg.SetptOrgin(allpts[0]);
		endTypes.end.SetptOrgin(allpts[allpts.size() - 1]);

		GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
		PITRebarCurve rebar;
		rebar.SetVertices(vers);
		rebar.EvaluateEndTypes(endTypes);
		rebars.push_back(rebar);
		allpts.clear();

		WString strSizeTmp1 = GetvecDirSize().at(0);
		if (strSizeTmp1.find(L"mm") != WString::npos)
		{
			strSizeTmp1.ReplaceAll(L"mm", L"");
		}
		pt1[0].z += CoverSlabLowDate.height - dReverseCover - dPositiveCover - m_diameter1 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) * 2;
		pt1[1].z += CoverSlabLowDate.height - dReverseCover - dPositiveCover - m_diameter1 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) * 2;

		allpts.push_back(pt1[0]);
		allpts.push_back(pt1[1]);

		endTypes.beg.SetptOrgin(allpts[0]);
		endTypes.end.SetptOrgin(allpts[allpts.size() - 1]);

		RebarVertices  versTmp;
		GetRebarVerticesFromPoints(versTmp, allpts, m_diameter1);
		PITRebarCurve rebarTmp;
		rebarTmp.SetVertices(versTmp);
		rebarTmp.EvaluateEndTypes(endTypes);
		rebars.push_back(rebarTmp);
		allpts.clear();
	
		return true;
	}
	case 4:
	{
		arcVec.Normalize();
		arcVec.Negate();
		double dLength = m_diameter1 * 0.5;
		arcVec.ScaleToLength(dLength);
		(pt1[0]).Add(arcVec);

		WString strSizeTmp = GetvecDirSize().at(6);
		if (strSizeTmp.find(L"mm") != WString::npos)
		{
			strSizeTmp.ReplaceAll(L"mm", L"");
		}

		DPoint3d temp2;
		temp2 = pt1[0];
		temp2.z -= m_zLen - CoverSlabLowDate.height - dReverseCover - m_diameter1 * 0.5 - RebarCode::GetBarDiameter(strSizeTmp, ACTIVEMODEL);

		temp = temp2;
		arcVec.Normalize();
		arcVec.ScaleToLength(CoverSlabLowDate.length * 0.25);
		temp.Add(arcVec);
		allpts.push_back(temp);
		allpts.push_back(temp2);
		allpts.push_back(pt1[0]);

		temp = pt1[0];
		arcVec.Normalize();
		arcVec.ScaleToLength(CoverSlabLowDate.length * 0.25);
		temp.Add(arcVec);
		allpts.push_back(temp);

		GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
		PITRebarCurve rebar;
		rebar.SetVertices(vers);
		rebars.push_back(rebar);

		allpts.clear();

		arcVec.Normalize();
		arcVec.Negate();

		dLength = m_diameter1 * 0.5;
		arcVec.ScaleToLength(dLength);
		(pt1[1]).Add(arcVec);

		temp2 = pt1[1];
		temp2.z -= m_zLen - CoverSlabLowDate.height - dReverseCover - m_diameter1 * 0.5 - RebarCode::GetBarDiameter(strSizeTmp, ACTIVEMODEL);

		temp = temp2;
		arcVec.Normalize();
		arcVec.ScaleToLength(CoverSlabLowDate.length * 0.25);
		temp.Add(arcVec);
		allpts.push_back(temp);
		allpts.push_back(temp2);
		allpts.push_back(pt1[1]);

		temp = pt1[1];
		arcVec.Normalize();
		arcVec.ScaleToLength(CoverSlabLowDate.length * 0.25);
		temp.Add(arcVec);
		allpts.push_back(temp);

		RebarVertices  versTmp;
		GetRebarVerticesFromPoints(versTmp, allpts, m_diameter1);
		PITRebarCurve rebar1;
		rebar1.SetVertices(versTmp);
		rebars.push_back(rebar1);

		return true;
	}
	case 5:
	{
		allpts.push_back(pt1[0]);
		allpts.push_back(pt1[1]);

		endTypes.beg.SetptOrgin(allpts[0]);
		endTypes.end.SetptOrgin(allpts[allpts.size() - 1]);

		GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
		PITRebarCurve rebar;
		rebar.SetVertices(vers);
		rebar.EvaluateEndTypes(endTypes);
		rebars.push_back(rebar);
		allpts.clear();

		WString strSizeTmp1 = GetvecDirSize().at(6);
		if (strSizeTmp1.find(L"mm") != WString::npos)
		{
			strSizeTmp1.ReplaceAll(L"mm", L"");
		}
		pt1[0].z -= CoverSlabHighDate.height - dReverseCover - dPositiveCover - m_diameter1 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) * 2;
		pt1[1].z -= CoverSlabHighDate.height - dReverseCover - dPositiveCover - m_diameter1 - RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL) * 2;

		allpts.push_back(pt1[0]);
		allpts.push_back(pt1[1]);

		endTypes.beg.SetptOrgin(allpts[0]);
		endTypes.end.SetptOrgin(allpts[allpts.size() - 1]);

		RebarVertices  versTmp;
		GetRebarVerticesFromPoints(versTmp, allpts, m_diameter1);
		PITRebarCurve rebarTmp;
		rebarTmp.SetVertices(versTmp);
		rebarTmp.EvaluateEndTypes(endTypes);
		rebars.push_back(rebarTmp);
		allpts.clear();

		return true;
	}
	case 6:
	{
		temp = pt1[0];
		allpts.push_back(temp);

		temp.z -= (CoverSlabHighDate.height - dReverseCover - dPositiveCover - m_diameter1);// 弯曲长度=减去钢筋直径和上面保护层
		allpts.push_back(temp);

		arcVec.Negate();
		arcVec.Normalize();
		arcVec.ScaleToLength(pt1[1].Distance(pt1[0]) - m_diameter1 * 0.5 + endOffset);
		temp.Add(arcVec);
		allpts.push_back(temp);

		temp.z += (CoverSlabHighDate.height - dReverseCover - dPositiveCover - m_diameter1);// 弯曲长度=减去钢筋直接和上面保护层
		allpts.push_back(temp);

		arcVec.Normalize();
		arcVec.Negate();
		arcVec.ScaleToLength(pt1[1].Distance(pt1[0]) - m_diameter1 + endOffset + startOffset);
		temp.Add(arcVec);
		allpts.push_back(temp);
		break;
	}
	default:
		break;
	}

	if (!allpts.empty())
	{
		endTypes.beg.SetptOrgin(allpts[0]);
		endTypes.end.SetptOrgin(allpts[allpts.size() - 1]);
	}

	GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
	PITRebarCurve rebar;
	rebar.SetVertices(vers);
	if (vers.GetSize() == 2)
	{
		rebar.EvaluateEndTypes(endTypes);
	}
	else
	{
		vector<CVector3D> vecEndNormal(2);
		CVector3D	endNormal;	//端部弯钩方向
		for (unsigned int i = 0; i < 2; i++)
		{
			CVector3D vec1 = allpts[1] - allpts[0];
			CVector3D vec2 = allpts[allpts.size() - 2] - allpts[allpts.size() - 1];
			vec1.Normalize();
			vec2.Normalize();
			if (vec1.IsPerpendicularTo(vec2))
			{
				if (i == 0)
				{
					double dRotateAngle = vecEndtype[i].rotateAngle;
					CVector3D rebarVec = vec1;
					endNormal = vec2;
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[i] = endNormal;
					endTypes.beg.SetendNormal(vecEndNormal[0]);
				}
				else
				{
					double dRotateAngle = vecEndtype[i].rotateAngle;
					CVector3D rebarVec = vec2;
					endNormal = vec1;
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[i] = endNormal;
					endTypes.end.SetendNormal(vecEndNormal[1]);
				}
			}
		}

		rebar.EvaluateEndTypesStirrup(endTypes);
	}

	rebars.push_back(rebar);
	return true;
}

RebarSetTag* SZCoverslabRebarAssembly::MakeRebars
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	double              xLen,
	double              width,
	double              spacing,
	double              startOffset,
	double              endOffset,

	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
	CMatrix3D const&    mat,
	DgnModelRefP        modelRef
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

	RebarEndType endTypeStart, endTypeEnd;
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
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
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
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}
	//	}//由于板配筋和墙配筋方向不同改保护层正反侧面
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	m_diameter1 = diameter;

	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false) * 0.5;	//乘以了30
	double startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef) * 0.5;	//乘以了100
	double endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef) * 0.5;	//乘以了100

	int numRebar = 0;
	double adjustedXLen, adjustedSpacing;

	double allSideCov = GetSideCover() * uor_per_mm * 2;
	double dPositiveCover = GetPositiveCover() * uor_per_mm;//正
	double dReverseCover = GetReverseCover() * uor_per_mm;//反

	if (cover_direction == 2)
	{
		startOffset += diameter;
		endOffset += diameter;
	}
	else if (cover_direction == 4)
	{
		WString strSizeTmp1 = GetvecDirSize().at(2);
		if (strSizeTmp1.find(L"mm") != WString::npos)
		{
			strSizeTmp1.ReplaceAll(L"mm", L"");
		}

		startOffset += diameter + RebarCode::GetBarDiameter(strSizeTmp1, ACTIVEMODEL);
		endOffset += diameter * 2;
	}


	adjustedXLen = xLen - 2.0 * GetSideCover() * uor_per_mm - diameter - startOffset - endOffset;

	numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
	{
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	double xPos = startOffset;

	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetbendLen(startbendLen);
	start.SetbendLen(endbendLen);
	start.SetbendRadius(bendRadius);

	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(bendRadius);
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };
	if (cover_direction == 2)
	{
		adjustedSpacing = (adjustedXLen - CoverSlabHighDate.morewidth) / (numRebar - 1);
	}
	else if (cover_direction == 3 || cover_direction == 5)
	{
		numRebar = 1;
	}
	for (int i = 0; i < numRebar; i++)//钢筋属性
	{
		vector<PITRebarCurve>     rebarCurves;
		makeRebarCurve(rebarCurves, xPos, width - allSideCov, endTypeStartOffset, endTypEendOffset, endType, endTypes, mat);
		
		xPos += adjustedSpacing;
		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}//rebarset里面rebarelement初步建立完成
	//钢筋组

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

		//EditElementHandle eeh;
		//LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		//eeh.AddToModel();

		vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));

		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
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
			string Stype = "front";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}
	m_vecAllRebarStartEnd.push_back(vecStartEnd);
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

bool SZCoverslabRebarAssembly::MakeRebars(DgnModelRefP modelRef)      // 创建钢筋
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
//	CalculateUseHoles(modelRef);
	m_vecAllRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();

	int iTwinbarSetIdIndex = 0;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kZaxis);
	double dLevelSpace = 0;
	double dSideCover = GetSideCover()*uor_per_mm;//反面保护层
	if ((COMPARE_VALUES(dSideCover, m_SZCoverSlabData.length) >= 0) || (COMPARE_VALUES(dSideCover, m_SZCoverSlabData.width) >= 0))	//如果侧面保护层大于等于墙的长度（板配筋改成高度）
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"侧面保护层大于等于  板的长度或宽度 ,无法创建钢筋层", MessageBoxIconType::Information);
		return false;
	}
	vector<CVector3D> vTrans;
	//计算侧面整体偏移量
	CalculateTransform(vTrans, modelRef);
	if (vTrans.size() != GetRebarLevelNum())
	{
		return false;
	}
	//高当作宽，墙面
	//double dLength = m_STCoverSlabData.length;
	//double dWidth = m_STCoverSlabData.width;

#ifdef PDMSIMPORT
	dLength *= modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	dWidth *= modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
#endif

	int iRebarSetTag = 0;
	int iRebarLevelNum = GetRebarLevelNum();
	vector<PIT::EndType> vecEndType;
	for (int i = 0; i < iRebarLevelNum; i++)
	{
		cover_direction = i;
		RebarSetTag* tag = NULL;
		CMatrix3D   mat;

		vector<PIT::EndType> vecEndType;
		if (GetvvecEndType().empty())		//没有设置端部样式，设置默认值
			vecEndType = { { 0,0,0 },{0,0,0} };
		else
			vecEndType = GetvvecEndType().at(i);

		double dActualWidth, dActualLength;
		//搭接，此时根据搭接选项的数据进行钢筋截断，生成多组钢筋
		if (i < 2)
		{
			if (GetvecDir().at(i) == 1)
			{
				dActualWidth = CoverSlabLowDate.width ;
				dActualLength = CoverSlabLowDate.length;
			}
			else
			{
				dActualWidth = CoverSlabLowDate.width - CoverSlabHighDate.morewidth;
				dActualLength = CoverSlabLowDate.length;
			}
		}
		else
		{
			if (4 == i)
			{
				dActualWidth = CoverSlabLowDate.width;
				dActualLength = CoverSlabHighDate.length;
			}
			else if (6 == i)
			{
				dActualWidth = CoverSlabHighDate.width;
				dActualLength = CoverSlabHighDate.length;
			}
			else if (3 == i)
			{
				dActualWidth = CoverSlabHighDate.width;
				dActualLength = CoverSlabLowDate.length;
			}
			else
			{
				dActualWidth = CoverSlabHighDate.width;
				dActualLength = CoverSlabHighDate.length;
			}
		}
		int iRebarSetNum = 1;
		double overLength = 0.0;

		if (GetvecDir().at(i) == 1)	//纵向钢筋
		{
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向
			if (GetvvecEndType().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = CVector3D::kYaxis;
					endNormal.Negate();
	
					CVector3D  yVecNegate = m_LineNormal;
					endNormal.Rotate(-90 * PI / 180, yVecNegate);
					endNormal.Rotate(dRotateAngle * PI / 180, yVecNegate);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}
			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;

			tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dActualLength, dActualWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i) * uor_per_mm,
				GetvecEndOffset().at(i) * uor_per_mm, vecEndType, vecEndNormal, mat, modelRef);
			vecEndType.clear();
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();

		}
		else
		{
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向
			if (GetvvecEndType().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					CVector3D rebarVec = m_SZCoverSlabData.ptEnd - m_SZCoverSlabData.ptStart;
					endNormal = CVector3D::kYaxis;
					endNormal.Negate();
					endNormal.Rotate(-90 * PI / 180, rebarVec);
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}
			mat = rot90;
			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;

			tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dActualWidth, dActualLength, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i) * uor_per_mm,
				GetvecEndOffset().at(i) * uor_per_mm, vecEndType, vecEndNormal, mat, modelRef);
			vecEndType.clear();
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();
		}
		if (m_vecRebarPtsLayer.size() > 1)
		{
			for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
			{
				int n = m + 1;
				RebarPoint rbPt;
				rbPt.Layer = i;
				rbPt.vecDir = GetvecDir().at(i);
				rbPt.ptstr = m_vecRebarPtsLayer.at(m);
				rbPt.ptend = m_vecRebarPtsLayer.at(n);
				g_vecRebarPtsNoHole.push_back(rbPt);
				m++;
			}
		}
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}