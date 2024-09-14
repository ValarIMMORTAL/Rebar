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
#include "CInsertRebarAssemblyWallNew.h"
#include "CInsertRebarDlgNew.h"
#include "PITRebarCurve.h"
#include "ScanIntersectTool.h"
#include "SelectRebarTool.h"

CInsertRebarAssemblySTWallNew::CInsertRebarAssemblySTWallNew(ElementId id, DgnModelRefP modelRef) :
	RebarAssembly(id, modelRef)
{
	pInsertDoubleRebarDlg = NULL;
	Init();
}

CInsertRebarAssemblySTWallNew::~CInsertRebarAssemblySTWallNew()
{
	for (int j = 0; j < m_Holeehs.size(); j++)
	{

		if (m_Holeehs.at(j) != nullptr)
		{
			delete m_Holeehs.at(j);
			m_Holeehs.at(j) = nullptr;
		}
	}
}

void CInsertRebarAssemblySTWallNew::Init()
{
	m_pInsertRebarDlgNew = NULL;
}


void CInsertRebarAssemblySTWallNew::SetConcreteData(const PIT::Concrete& concreteData)
{
	m_PositiveCover = concreteData.postiveCover;
	m_ReverseCover = concreteData.reverseCover;
	m_SideCover = concreteData.sideCover;
}

// 计算墙底部方向
bool CInsertRebarAssemblySTWallNew::CalaWallNormalVec(ElementHandleCR eh, CVector3DR vecNormal)
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

	double dHigth;
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &dHigth);
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

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, dHigth);

	vecNormal = FrontEnd - FrontStr;
	vecNormal.Normalize();

	return true;
}


// 计算板的厚度
double CInsertRebarAssemblySTWallNew::CalcSlabThickness(ElementHandleCR eh)
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
	DPoint3d minP;
	DPoint3d maxP;
	//计算指定元素描述符中元素的范围。
	mdlElmdscr_computeRange(&minP, &maxP, testeeh.GetElementDescrP(), NULL);


	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &dThickness);

	dThickness = (maxP.z - minP.z) < dThickness ? (maxP.z - minP.z) : dThickness;

	return (dThickness * uor_now / uor_ref) / uor_ref;
}

void CInsertRebarAssemblySTWallNew::filterHoleehs(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素
	double dSideCover = GetSideCover() * uor_per_mm;

	m_useHoleehs.clear();
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	for (EditElementHandleP eeh : m_Holeehs) // 过滤副实体
	{
		string htype;
		DgnECManagerR ecMgr = DgnECManager::GetManager();
		FindInstancesScopePtr scope = FindInstancesScope::CreateScope(*eeh, FindInstancesScopeOption(DgnECHostType::Element));
		ECQueryPtr            ecQuery = ECQuery::CreateQuery(ECQUERY_PROCESS_SearchAllClasses);
		ecQuery->SetSelectProperties(true);
		bool bFlag = true;
		bool isdoorNeg = false;
		for (DgnECInstancePtr instance : ecMgr.FindInstances(*scope, *ecQuery))
		{
			DgnElementECInstanceP elemInst = instance->GetAsElementInstance();
			if (elemInst->GetClass().GetDisplayLabel() == L"PARADATA")//如果有PARADATA的EC属性，读取唯一NAME值
			{
				ECN::ECValue ecVal;
				elemInst->GetValue(ecVal, L"Type");
				char tType[1024];
				ecVal.ToString().ConvertToLocaleChars(tType);
				htype = tType;
				if (htype == "DOOR")
				{
					isdoorNeg = true;
				}
				if (htype == "NEG")
				{
					bFlag = false;
					break;
				}
			}
		}
		if (bFlag)
		{
			if (isdoorNeg)
			{
				// PlusSideCover(*eeh, dSideCover, matrix, isdoorNeg, m_STwallData.width);
			}
			else
			{
				PlusSideCover(*eeh, dSideCover, matrix);
			}
			m_useHoleehs.push_back(eeh);
		}
	}
}

bool CInsertRebarAssemblySTWallNew::AnalyzingWallGeometricData(ElementHandleCR eh)
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

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());

	return true;
}

bool CInsertRebarAssemblySTWallNew::IsSmartSmartFeature(EditElementHandle& eeh)
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

bool CInsertRebarAssemblySTWallNew::makeRebarCurve
(
	RebarCurve&				rebar,
	double                  bendRadius,
	double                  bendLen,
	double					dRotateAngle,
	RebarEndTypes const&    endTypes,
	DPoint3d&         ptstr,
	DPoint3d&         ptend
)
{
	DPoint3d pt1[2] = { ptstr, ptend };
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetSideCover() * uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);

	ptstr = pt1[0];
	ptend = pt1[1];
	for (size_t i = 0; i < tmppts.size(); i++)
	{
		if (COMPARE_VALUES_EPS(tmppts.at(i).z, ptstr.z, EPS) < 0)
		{
			ptstr = tmppts.at(i);
		}
	}

	RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptstr);
	vex->SetType(RebarVertex::kStart);


	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptend);
	vex->SetType(RebarVertex::kEnd);

	CVector3D	endNormal;	//端部弯钩方向
	endNormal = m_STwallData.ptEnd - m_STwallData.ptStart;
	endNormal.Normalize();
	endNormal.Rotate(dRotateAngle * PI / 180, CVector3D::kZaxis);	//以钢筋方向为轴旋转

	rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, endNormal);
	//rebar.DoMatrix(mat);
	return true;
}


bool CInsertRebarAssemblySTWallNew::MakeRebars(DgnModelRefP modelRef)
{
	if (m_vecRebarPts.size() == 0)
	{
		return false;
	}
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素

	//按rebarsetid筛选钢筋
	map<ElementId, vector<InsertRebarPoint>> res_elements;
	for (InsertRebarPoint tmppt: m_vecRebarPts)
	{
		res_elements[tmppt.Rsid].push_back(tmppt);
	}
	map<ElementId, vector<InsertRebarPoint>>::iterator itr = res_elements.begin();
	CVector3D sortVec = m_STwallData.ptEnd - m_STwallData.ptStart;
	sortVec.Normalize();
	filterHoleehs(modelRef);
	int i = 1;
	for (;itr!=res_elements.end();itr++)
	{
		RebarSetTag* tag = NULL;
		vector<InsertRebarPoint> tmpts = itr->second;
		SortVecRebar(tmpts, sortVec);
		if (m_vecSetId.size() == 0)
		{
			m_vecSetId.resize(1);
		}
		m_vecSetId[0] = 0;
		tag = MakeRebars(tmpts, m_stWallInfo, m_vecSetId[0], modelRef);
		if (NULL != tag)
		{
			tag->SetBarSetTag(i++);
			rsetTags.Add(tag);
		}
	}
	//tag->SetBarSetTag(2);
	//rsetTags.Add(NULL);
	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

void CInsertRebarAssemblySTWallNew::SortVecRebar(vector<InsertRebarPoint>& vecPoint, CVector3D& sortVec)
{
	for (int i = 0; i < (int)vecPoint.size() - 1; i++)
	{
		for (int j = 0; j < (int)vecPoint.size() - 1 - i; j++)
		{
			DPoint3d pt1, pt2;
			pt1 = vecPoint[j].ptstr;
			pt2 = vecPoint[j + 1].ptstr;
			if (COMPARE_VALUES_EPS(sortVec.z, 0.00, EPS) != 0)
			{
				CVector3D vec = CVector3D::From(0, 0, pt2.z - pt1.z);
				vec.Normalize();
				if (COMPARE_VALUES_EPS(sortVec.z, vec.z, EPS) != 0)
				{
					InsertRebarPoint ptTmp = vecPoint[j];
					vecPoint[j] = vecPoint[j + 1];
					vecPoint[j + 1] = ptTmp;
				}
			}
			else
			{
				CVector3D vec = CVector3D::From(pt2.x - pt1.x, pt2.y - pt1.y, 0);
				vec.Normalize();
				if (!(COMPARE_VALUES_EPS(sortVec.x, vec.x, EPS) == 0 && COMPARE_VALUES_EPS(sortVec.y, vec.y, EPS) == 0))
				{
					InsertRebarPoint ptTmp = vecPoint[j];
					vecPoint[j] = vecPoint[j + 1];
					vecPoint[j + 1] = ptTmp;
				}
			}

		}
	}
}

void CInsertRebarAssemblySTWallNew::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}


RebarSetTag* CInsertRebarAssemblySTWallNew::MakeRebars
(
	vector<InsertRebarPoint>&		vecRebarPts,
	InsertRebarInfo::WallInfo&		wallData,
	ElementId&						rebarSetId,
	DgnModelRefP					modelRef
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

	RebarEndType endTypeStart, endTypeEnd;

	endTypeStart.SetType(RebarEndType::kNone);

	switch (wallData.endType)
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
	int numRebar = static_cast<int>(vecRebarPts.size());
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	double dRebarLen = (wallData.embedLength + wallData.expandLength) * uor_per_mm; // 钢筋长度


	vector<RebarCurve>     rebarCurvesNum;
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd }; // 端部设置

	bool bFlag = true;
	for (int i = 0; i < numRebar; i++)
	{
		BrString sizeKey = vecRebarPts.at(i).sizeKey;
		double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
		double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		double dExpandLength = 0.00;

		DPoint3d ptstr = vecRebarPts.at(i).ptstr;
		DPoint3d ptend = vecRebarPts.at(i).ptend;

		RebarCurve     rebarCurves;
		bvector<DPoint3d> allpts;
		if (vecRebarPts.at(i).isMid)
		{
			if (m_stWallInfo.connectStyle == InsertRebarInfo::ConnectStyle::StaggerdJoint)
			{
				movePoint(m_STwallData.ptEnd - m_STwallData.ptStart, vecRebarPts.at(i).ptstr, m_stWallInfo.mainDiameter);
				movePoint(m_STwallData.ptEnd - m_STwallData.ptStart, vecRebarPts.at(i).ptmid, m_stWallInfo.mainDiameter);
				movePoint(m_STwallData.ptEnd - m_STwallData.ptStart, vecRebarPts.at(i).ptend, m_stWallInfo.mainDiameter);
			}	
			allpts.push_back(vecRebarPts.at(i).ptstr);
			allpts.push_back(vecRebarPts.at(i).ptmid);
			allpts.push_back(vecRebarPts.at(i).ptend);
		}
		else
		{
			if (m_stWallInfo.connectStyle == InsertRebarInfo::ConnectStyle::StaggerdJoint)
			{
				movePoint(m_STwallData.ptEnd - m_STwallData.ptStart, vecRebarPts.at(i).ptstr, m_stWallInfo.mainDiameter);
				movePoint(m_STwallData.ptEnd - m_STwallData.ptStart, vecRebarPts.at(i).ptend, m_stWallInfo.mainDiameter);
			}		
			allpts.push_back(vecRebarPts.at(i).ptstr);
			allpts.push_back(vecRebarPts.at(i).ptend);
		}

		RebarVertices  vers;
		GetRebarVerticesFromPoints(vers, allpts, bendRadius);
		rebarCurves.SetVertices(vers);

		if (allpts.size() == 2)
		{
			CVector3D	endNormal;	//端部弯钩方向
			endNormal = m_STwallData.ptEnd - m_STwallData.ptStart;

			if (COMPARE_VALUES_EPS(endNormal.x, 0.0, uor_per_mm) > 0 && COMPARE_VALUES_EPS(endNormal.y, 0.0, uor_per_mm) < 0)
			{
				endNormal.Negate();
			}

			endNormal.Normalize();
			if (m_stWallInfo.isBack)
			{
				endNormal.Negate();
			}
			endNormal.Rotate(m_stWallInfo.rotateAngle * PI / 180, CVector3D::kZaxis);	//以钢筋方向为轴旋转
			rebarCurves.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, endNormal);
		}

		rebarCurvesNum.push_back(rebarCurves);
	}

	RebarSymbology symb;
	{
		symb.SetRebarLevel(TEXT_INSERTION_REBAR);
	}
	numRebar = (int)rebarCurvesNum.size();
	if (numRebar != (int)vecRebarPts.size())
	{
		return NULL;
	}
	int j = 0;
	for (RebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);

		string str(vecRebarPts.at(j).sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);

		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			BrString sizeKey = vecRebarPts.at(j).sizeKey;
			BrString tmpGrade = sizeKey;
			int grade = 0;
			tmpGrade = tmpGrade.Right(1);
			if (tmpGrade == "A")
			{
				grade = 0;
			}
			else if (tmpGrade == "B")
			{
				grade = 1;
			}
			else if (tmpGrade == "C")
			{
				grade = 2;
			}
			else if (tmpGrade == "D")
			{
				grade = 3;
			}
			double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string Stype = "InsertRebar";
			string Level(vecRebarPts.at(j).level); 
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar,Level,grade ,Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, GetNormalSpace(), ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(GetNormalSpace());
	setdata.SetAverageSpacing(GetAverageSpace());

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}


bool CInsertRebarAssemblySTWallNew::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	pInsertDoubleRebarDlg = new CInsertRebarDlgNew(CWnd::FromHandle(MSWIND));
	pInsertDoubleRebarDlg->SetSelectElement(ehSel);
	pInsertDoubleRebarDlg->Create(IDD_DIALOG_Insert);
	pInsertDoubleRebarDlg->m_pInsertWallAssemblyNew = this;
	pInsertDoubleRebarDlg->ShowWindow(SW_SHOW);


// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
// 	CInsertRebarDlgNew dlg(CWnd::FromHandle(MSWIND));
// 	dlg.SetSelectElement(ehSel);
// 	dlg.m_pInsertWallAssemblyNew = this;
// 	if (IDCANCEL == dlg.DoModal())
// 	{
// 		return false;
// 	}
	return true;
}

bool CInsertRebarAssemblySTWallNew::Rebuild()
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

long CInsertRebarAssemblySTWallNew::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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