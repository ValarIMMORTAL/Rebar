/*--------------------------------------------------------------------------------------+
|
|     $Source: CInsertRebarAssemblyWall.cpp $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/

#include "_ustation.h"
#include "RebarDetailElement.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "resource.h"
#include "ExtractFacesTool.h"
#include "TieRebar.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "XmlHelper.h"
#include "CInsertRebarMainDlg.h"
#include "CInsertRebarAssemblyWall.h"

CInsertRebarAssemblySTWall::CInsertRebarAssemblySTWall(ElementId id, DgnModelRefP modelRef) :
	RebarAssembly(id, modelRef)
{
	Init();
}

CInsertRebarAssemblySTWall::~CInsertRebarAssemblySTWall()
{
	if (m_pOldElm != NULL)
	{
		delete m_pOldElm;
		m_pOldElm = NULL;
	}

	for (int j = 0; j < m_Holeehs.size(); j++)
	{

		if (m_Holeehs.at(j) != nullptr)
		{
			delete m_Holeehs.at(j);
			m_Holeehs.at(j) = nullptr;
		}
	}
}

void CInsertRebarAssemblySTWall::Init()
{
	m_pInsertRebarMainDlg = NULL;
	m_bFirstLong = true;
	m_pOldElm = NULL;
}


void CInsertRebarAssemblySTWall::SetLayerRebars()
{
	m_mpLayerRebars.clear();
	int numFront = 0;
	int numBack = 0;
	int numMid = 0;
	//统计前，中，后层数
	for (RebarPoint pt : m_rebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			if (pt.Layer > numFront)
			{
				numFront = pt.Layer;
			}
		}
		else if (pt.DataExchange == 1)//中间
		{
			if (pt.Layer > numMid)
			{
				numMid = pt.Layer;
			}
		}
		else //背面 
		{
			if (pt.Layer > numBack)
			{
				numBack = pt.Layer;
			}
		}
	}
	for (RebarPoint pt : m_rebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			m_mpLayerRebars[pt.Layer - 1].push_back(pt);
		}
		else if (pt.DataExchange == 1)//中间
		{
			m_mpLayerRebars[pt.Layer + numFront - 1].push_back(pt);
		}
		else //背面 
		{
			m_mpLayerRebars[numFront + numMid + numBack - pt.Layer].push_back(pt);
		}
	}
	
}

void CInsertRebarAssemblySTWall::SetConcreteData(const PIT::Concrete& concreteData)
{
	m_PositiveCover = concreteData.postiveCover;
	m_ReverseCover = concreteData.reverseCover;
	m_SideCover = concreteData.sideCover;
	m_RebarLevelNum = concreteData.rebarLevelNum;
}

void CInsertRebarAssemblySTWall::SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas)
{
	int i = 0;
	m_vecDirSize.resize(wallRebarDatas.size());
	for (PIT::ConcreteRebar rebwall : wallRebarDatas)
	{
		m_vecDirSize[i++] = rebwall.rebarSize;
	}

}

void CInsertRebarAssemblySTWall::SetRebarEndTypes(vector<PIT::EndType> const & vecEndTypes)
{
	if (vecEndTypes.size())
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

// 计算板的厚度
double CInsertRebarAssemblySTWall::CalcSlabThickness(ElementHandleCR eh)
{
	double dThickness = 0.00;

	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &dThickness);

	return (dThickness * uor_now / uor_ref) / uor_ref;
}

void CInsertRebarAssemblySTWall::CalculateUseHoles(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	m_useHoleehs.clear();
	double dSideCover = GetSideCover()*uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

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
		ElementCopyContext copier(ACTIVEMODEL);
		copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
		copier.SetTransformToDestination(true);
		copier.SetWriteElements(false);
		copier.DoCopy(*m_Holeehs.at(j));
		PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
		m_useHoleehs.push_back(m_Holeehs.at(j));
	}

	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}
}

bool CInsertRebarAssemblySTWall::AnalyzingWallGeometricData(ElementHandleCR eh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	DPoint3d FrontStr, FrontEnd;
	DPoint3d BackStr, BackEnd;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	if (m_pOldElm == NULL)
	{
		m_pOldElm = new EditElementHandle();
	}
	m_pOldElm->Duplicate(Eleeh);
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &m_STwallData.height);
	if (vecDownFaceLine.empty() || vecDownFontLine.empty() || vecDownBackLine.empty())
		return false;

	DPoint3d pt1[2], pt2[2];
	mdlLinear_extract(pt1, NULL, &vecDownFontLine[0]->el, model);//暂时使用当前激活MODEL，如有问题再修改
	mdlLinear_extract(pt2, NULL, &vecDownBackLine[0]->el, model);

	if (pt1[0].Distance(pt2[0]) < pt1[0].Distance(pt2[1]))//前面线段法向和后面线段法向法向相反
	{
		DPoint3d tmpPt = pt2[0];
		pt2[0] = pt2[1];
		pt2[1] = tmpPt;
	}

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_STwallData.height);

	m_STwallData.height = m_STwallData.height*uor_now / uor_ref;
	m_STwallData.width = FrontStr.Distance(BackEnd)*uor_now / uor_ref;
	m_STwallData.length = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;
	m_STwallData.ptStart = FrontStr;
	m_STwallData.ptEnd = FrontEnd;

	m_Holeehs = Holeehs;

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());

	CVector3D  xVec(FrontStr, FrontEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
	//	CVector3D  yVecNegate = yVec;
	//	yVecNegate.Negate();
	//	yVecNegate.Normalize();
	//	yVecNegate.ScaleToLength(m_STwallData.width);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	//	ptStart.Add(yVecNegate);
	//	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(FrontStr, FrontEnd);
	BeMatrix   placement = CMatrix3D::Ucs(FrontStr, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	return true;
}

bool CInsertRebarAssemblySTWall::IsSmartSmartFeature(EditElementHandle& eeh)
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

//将钢筋点转换到ACS坐标系下
void  CInsertRebarAssemblySTWall::TransFromRebarPts(vector<RebarPoint>&  rebarPts)
{
	TransformInfo transinfo(GetTrans());
	for (int i = 0; i < rebarPts.size(); i++)
	{
		DPoint3d ptstr = rebarPts.at(i).ptstr;
		DPoint3d ptend = rebarPts.at(i).ptend;
		EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
		// eeh.AddToModel();
		if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
		{
			rebarPts.at(i).ptstr = ptstr;
			rebarPts.at(i).ptend = ptend;
		}
	}
}

bool sortCmd(DPoint3d pt1, DPoint3d pt2)
{
	if (pt2.z > pt1.z)
	{
		return true;
	}
	return false;
}

RebarSetTag* CInsertRebarAssemblySTWall::MakeRebars
(
	vector<RebarPoint>&				rebarPts,
	vector<PIT::EndType>					vecEndtype,
	InsertRebarInfo::WallInfo		wallData,
	ElementId&						rebarSetId,
	BrStringCR						sizeKey,
	DgnModelRefP					modelRef,
	int&							rebarLevel
)
{
	if (rebarPts[0].vecDir == 0) // 只绘制Z轴方向的钢筋
	{
		return NULL;
	}
	rebarLevel++;

	vector<RebarPoint> tmprebarPts;
	tmprebarPts.insert(tmprebarPts.begin(), rebarPts.begin(), rebarPts.end());
	// TransFromRebarPts(tmprebarPts);

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	switch (vecEndtype[1].endType)
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

	switch (vecEndtype[0].endType)
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

	bool const isStirrup = false;
	int numRebar = static_cast<int>(rebarPts.size());
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
	double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
	double dRebarLen = (wallData.embedLength + wallData.expandLength) * uor_per_mm; // 钢筋长度
	if (m_staggered == 2)
	{
		m_bFirstLong = !m_bFirstLong;
	}
	if (m_staggered == 3 && COMPARE_VALUES(dRebarLen, 0.00) == 0)
	{
		return NULL;
	}

	vector<RebarCurve>     rebarCurvesNum;
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd }; // 端部设置

	bool bFlag = true;
	for (int i = 0; i < numRebar; i++)
	{
		Transform inversMat = GetTrans();
		inversMat.InverseOf(inversMat);
		TransformInfo transinfo(inversMat);

		if (i > 0 && m_staggered == 2)
		{
			if (bFlag)
			{
				dRebarLen = (wallData.embedLength + wallData.expandLength * 0.5) * uor_per_mm;
			}
			else
			{
				dRebarLen = (wallData.embedLength + wallData.expandLength) * uor_per_mm;
			}
			bFlag = !bFlag;
		}
		else if (m_staggered == 2) // i == 0
		{
			if (!m_bFirstLong)
			{
				dRebarLen = (wallData.embedLength + wallData.expandLength * 0.5) * uor_per_mm;
				bFlag = false;
			}
			else
			{
				bFlag = true;
			}
		}

		DPoint3d ptstr = (rebarPts.at(i).ptstr.z - rebarPts.at(i).ptend.z) > EPS == 1 ? rebarPts.at(i).ptstr : rebarPts.at(i).ptend;
		DPoint3d ptend = (rebarPts.at(i).ptstr.z - rebarPts.at(i).ptend.z) > EPS == 1 ? rebarPts.at(i).ptend : rebarPts.at(i).ptstr;

		//haizrongzketjitwolcaolnilnaianaia
		ptstr.z = ptend.z - m_ReverseCover * uor_per_mm - wallData.embedLength * uor_per_mm;

		ptend.z = ptstr.z + dRebarLen;
		// ptend.z = ptstr.z + dRebarLen;

		double dSideCover = GetSideCover() * uor_per_mm;

		vector<DPoint3d> tmppts;
		Transform matrix;
		GetPlacement().AssignTo(matrix);
		GetIntersectPointsWithHolesByInsert(tmppts, m_useHoleehs, ptstr, ptend, dSideCover, matrix);

		if (tmppts.size() > 0)
		{
			sort(tmppts.begin(), tmppts.end(), sortCmd);
			ptend = tmppts.at(0);

			if (!ISPointInElement(m_pOldElm, ptend))
			{
				continue;
			}
		}

		RebarCurve     rebarCurves;
		EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		// eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
		if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
		{
			makeRebarCurve(rebarCurves, bendRadius, bendLen, vecEndtype[1].rotateAngle, endTypes, ptstr, ptend);
			rebarCurvesNum.push_back(rebarCurves);
		}
	}

	RebarSymbology symb;
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_INSERTION_REBAR);
	}
	numRebar = (int)rebarCurvesNum.size();
	int j = 0;
	for (RebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);

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
			string Stype = "InsertRebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, 200, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(200);
	setdata.SetAverageSpacing(200);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

bool CInsertRebarAssemblySTWall::makeRebarCurve
(
	RebarCurve&				rebar,
	double                  bendRadius,
	double                  bendLen,
	double					dRotateAngle,
	RebarEndTypes const&    endTypes,
	CPoint3D const&         ptstr,
	CPoint3D const&         ptend
)
{
	RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptstr);
	vex->SetType(RebarVertex::kStart);


	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptend);
	vex->SetType(RebarVertex::kEnd);

	CVector3D	endNormal;	//端部弯钩方向
	endNormal = m_STwallData.ptStart - m_STwallData.ptEnd;
	endNormal.Normalize();
	endNormal.Rotate(dRotateAngle * PI / 180, CVector3D::kZaxis);	//以钢筋方向为轴旋转

	rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, endNormal);
	//rebar.DoMatrix(mat);
	return true;
}

bool CInsertRebarAssemblySTWall::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素

	CalculateUseHoles(modelRef);

	RebarSetTag* tag = NULL;
	int tagID = 0;
	int tagIDTmp = -1;

	if (m_vvecEndType.size() != m_RebarLevelNum || m_vecWallData.size() != m_RebarLevelNum ||
		m_vecDirSize.size() != m_RebarLevelNum || m_mpLayerRebars.size() != m_RebarLevelNum)
	{
		return true;
	}

	for (int i = 0; i < GetRebarLevelNum(); i++)
	{
		if (tagIDTmp != tagID  && m_vecSetId.size() < tagID + 1)
		{
			m_vecSetId.resize(tagID + 1);
			m_vecSetId[tagID] = 0;
			tagIDTmp = tagID;
		}
		if (i == 0)
		{
			m_bFirstLong = false;
		}
		vector<RebarPoint> vecLayer = m_mpLayerRebars[i];
		tag = MakeRebars(vecLayer, GetvvecEndType().at(i), PopvecWallData().at(i), PopvecSetId().at(tagID), GetvecDirSize().at(i), modelRef, tagID);
		if (NULL != tag)
		{
			tag->SetBarSetTag(tagID);
			rsetTags.Add(tag);
		}
	}
	m_vecSetId.resize(tagID);

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

bool CInsertRebarAssemblySTWall::OnDoubleClick()
{
	ElementId testid = FetchConcrete();
	ElementId tmpid = GetSelectedElement();
	if (tmpid == 0)
	{
		return false;
	}
	DgnModelRefP modelp = GetSelectedModel();
	EditElementHandle ehSel;
	EditElementHandle basis;
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
	m_pInsertRebarMainDlg = new CInsertRebarMainDlg;
	m_pInsertRebarMainDlg->SetFirstItem(1);
	m_pInsertRebarMainDlg->SetSelectElement(ehSel);
	m_pInsertRebarMainDlg->SetConcreteId(FetchConcrete());
	m_pInsertRebarMainDlg->Create(IDD_DIALOG_InsertRebarMain);
	m_pInsertRebarMainDlg->ShowWindow(SW_SHOW);
	m_pInsertRebarMainDlg->m_PageWallInsertRebar.SetWallAssemblyPtr(this);
	return true;
}

bool CInsertRebarAssemblySTWall::Rebuild()
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
	MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();
	//SetElementXAttribute(ehSel.GetElementId(), g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ehSel.GetModelRef());
	//eeh2.AddToModel();
	return true;
}

long CInsertRebarAssemblySTWall::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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