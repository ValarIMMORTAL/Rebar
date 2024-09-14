#include "_USTATION.h"
#include "BaseRebarAssembly.h"
#include "BentlyCommonfile.h"
#include "PITPSPublic.h"
#include "XmlHelper.h"
#include "BaseRebarDlg.h"
#include "SingleRebarAssembly.h"
#include "resource.h"
// BaseType AbanurusRebarAssembly::JudgeBaseType(ElementHandleR eeh)
// {
// 
// }

AbanurusRebarAssembly::AbanurusRebarAssembly(ElementId id, DgnModelRefP modelRef) :
	STSlabRebarAssembly(id, modelRef),
	m_Len(0)
{
	Init();
	m_pOldElm = NULL;
	m_pBaseRebarMainDlg = nullptr;
	isarc = false;
}
void AbanurusRebarAssembly::InitRebarParam(double ulenth)
{
	if (ulenth <= 0)
	{
		ulenth = 240;
	}
	double uor_per_mm = GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	int iRebarLevelNum = GetRebarLevelNum();
	if (GetRebarLevelNum() != 2 && GetvecDirSize().size() != 2&& GetvecDir().size() != 2)
	{
		return;
	}
	double radius1, radius2, radiusStirrup;
	radius1 = RebarCode::GetBarDiameter(GetvecDirSize().at(0), ACTIVEMODEL) / 2;
	radius2 = RebarCode::GetBarDiameter(GetvecDirSize().at(1), ACTIVEMODEL) / 2;
	radiusStirrup = RebarCode::GetBarDiameter(GetStirrupData().rebarSize, ACTIVEMODEL) / 2;
	RebarEndType endType;
	endType.SetType(RebarEndType::kBend);

	double benradius1, benradius2;
	benradius1 = RebarCode::GetPinRadius(GetvecDirSize().at(0), ACTIVEMODEL,false);
	benradius2 = RebarCode::GetPinRadius(GetvecDirSize().at(1), ACTIVEMODEL,false);

	PopSideCover() = PopSideCover() + radiusStirrup * 2/ uor_per_mm;
	double dSideCover = GetSideCover()*uor_per_mm;
	double dpositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double disV,disH;//横筋的偏移和纵筋的偏移
	if (GetvecDir().at(0) == 1 && GetvecDir().at(1) == 0)//第一层为横筋，第二层为纵筋
	{
		disV =   benradius2 + radius2;
		disH =   benradius1 + radius1;
		disV = disV / uor_per_mm;
		disH = disH / uor_per_mm;
	}
	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		vector<PIT::EndType> vecEndType;
		if (GetvvecEndType().size() + 1 < i)
		{
			PIT::EndType endType;
			memset(&endType, 0, sizeof(PIT::EndType));
			vecEndType = { { 0,0,0 },{0,0,0} ,endType };
			PopvvecEndType().push_back(vecEndType);
		}
		if (PopvvecEndType().at(i).size() > 1&&PopvecDir().size()>1)
		{
			if (PopvecDir().at(i) == 1)
			{
				PopvvecEndType().at(i).at(0).endType = 4;
				PopvvecEndType().at(i).at(0).offset = 0;
				PopvvecEndType().at(i).at(0).rotateAngle = -90;
				PopvvecEndType().at(i).at(0).endPtInfo.value1 = 0;
				PopvvecEndType().at(i).at(0).endPtInfo.value3 = ulenth * uor_per_mm;

				PopvvecEndType().at(i).at(1).endType = 4;
				PopvvecEndType().at(i).at(1).offset = 0;
				PopvvecEndType().at(i).at(1).rotateAngle = -90;
				PopvvecEndType().at(i).at(1).endPtInfo.value1 = 0;
				PopvvecEndType().at(i).at(1).endPtInfo.value3 = ulenth * uor_per_mm;
			}
			else if (PopvecDir().at(i) == 0)
			{
				PopvvecEndType().at(i).at(0).endType = 4;
				PopvvecEndType().at(i).at(0).offset = 0;
				PopvvecEndType().at(i).at(0).rotateAngle = 0;
				PopvvecEndType().at(i).at(0).endPtInfo.value1 = 0;
				PopvvecEndType().at(i).at(0).endPtInfo.value3 = ulenth * uor_per_mm;

				PopvvecEndType().at(i).at(1).endType = 4;
				PopvvecEndType().at(i).at(1).offset = 0;
				PopvvecEndType().at(i).at(1).rotateAngle = 0;
				PopvvecEndType().at(i).at(1).endPtInfo.value1 = 0;
				PopvvecEndType().at(i).at(1).endPtInfo.value3 = ulenth * uor_per_mm;
			}
		}
		
	}

	if (PopvecEndOffset().size()!=2)
	{
		PopvecEndOffset().clear();
		PopvecEndOffset().push_back(disV);
		PopvecEndOffset().push_back(disH);

		PopvecStartOffset().clear();
		PopvecStartOffset().push_back(disV);
		PopvecStartOffset().push_back(disH);
	}
	else
	{
		PopvecEndOffset().at(0) = PopvecEndOffset().at(0)+disV;
		PopvecEndOffset().at(1) = PopvecEndOffset().at(1)+disH;

		PopvecStartOffset().at(0) = PopvecStartOffset().at(0) + disV;
		PopvecStartOffset().at(1) = PopvecStartOffset().at(1) + disH;
	}

}
bool AbanurusRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_pBaseRebarMainDlg = new BaseRebarDlg(CWnd::FromHandle(MSWIND));
	m_pBaseRebarMainDlg->m_ehSel = ehSel;
	m_pBaseRebarMainDlg->Create(IDD_DIALOG_BaseRebar);
	m_pBaseRebarMainDlg->ShowWindow(SW_SHOW);


// 	BaseRebarDlg dlg(CWnd::FromHandle(MSWIND));
// 	dlg.m_ehSel = ehSel;
// 	if (IDCANCEL == dlg.DoModal())
// 		return false;
	return true;
}
void AbanurusRebarAssembly::Init()            //重新指定容器的长度为m_RebarLevelNum
{
	PopvecDir().resize(2);            //方向,0表示x轴，1表示z轴
	PopvecDirSize().resize(2);         //尺寸
	PopvecDirSpacing().resize(2);       //间隔
 	PopvecRebarType().resize(2);        //钢筋类型
 	PopvecStartOffset().resize(2);       //起点偏移
 	PopvecEndOffset().resize(2);         //终点偏移
 	PopvecSetId().resize(2);            //SetId


	//根据需求并筋需要设置不一样的层
	for (size_t i = 0; i < PopvecSetId().size(); i++)
	{
		PopvecSetId()[i] = 0;
	}
}

bool AbanurusRebarAssembly::IsSmartSmartFeature(EditElementHandle& eeh)
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
				//				eeh.AddToModel();
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}

		}
		return true;
	}
}

void AbanurusRebarAssembly::SetBaseType(BaseType basetype)
{

}

long AbanurusRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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
void AbanurusRebarAssembly::SetConcreteData(PIT::Concrete const& concreteData, StirrupData const& stirrpinfo,double& Len, Abanurus_PTRebarData const& ptRebarinfo)    //设置三个保护层信息
{
	PopPositiveCover() = concreteData.postiveCover;
	PopReverseCover() = concreteData.reverseCover;
	PopSideCover() = concreteData.sideCover;
	SetStirrupData(stirrpinfo);
	SetAbanurus_PTRebarData(ptRebarinfo);
}


void AbanurusRebarAssembly::SetRebarData(vector<PIT::ConcreteRebar> const& vecData)     //用vector数组存每层钢筋的信息
{
	
	for (int i = 0; i < vecData.size(); i++)
	{
		PIT::EndType::RebarEndPointInfo endPtInfo;
		memset(&endPtInfo, 0, sizeof(endPtInfo));
		PIT::EndType oneEndTypeData = { 0,0,0 ,endPtInfo };
		vector<PIT::EndType> tmpVec;
		tmpVec.push_back(oneEndTypeData);
		tmpVec.push_back(oneEndTypeData);
		PopvvecEndType().push_back(tmpVec);
	}
	for (int i = 0; i < vecData.size(); i++)
	{
		CString strTwinBar = L"";
		TwinBarSet::TwinBarLevelInfo oneTwinBarData;
		strcpy(oneTwinBarData.levelName, CT2A(strTwinBar.GetBuffer()));
		oneTwinBarData.hasTwinbars = 0;
		strcpy(oneTwinBarData.rebarSize, vecData[i].rebarSize);
		oneTwinBarData.rebarType = vecData[i].rebarType;
		oneTwinBarData.interval = 0;
		PopvecTwinRebarLevel().push_back(oneTwinBarData);
	}
	TieReBarInfo tmptie;
	//TwinBarSet::TwinBarInfo tmptwin;
	memset(&tmptie, 0, sizeof(TieReBarInfo));
	//memset(&tmptwin, 0, sizeof(TwinBarSet::TwinBarInfo));
	SetTieRebarInfo(tmptie);
	PopRebarLevelNum() = 2;
	__super::SetRebarData(vecData);
}


bool AbanurusRebarAssembly::SetSlabData(ElementHandleCR eh)
{
	m_ehSel = eh;
	vector<DSegment3d> vecDownFontLine;
	vector<DSegment3d> vecDownBackLine;
	EditElementHandle testeeh(m_ehSel, false);
	
	EditElementHandle Eleeh;
	double width;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &width);
	if (vecDownBackLine.empty()||vecDownFontLine.empty())//圆形设备基础时，缩小设备基础
	{
		isarc = true;
		BrString sizeKey = m_StirrupData.rebarSize;
		sizeKey.Replace(L"mm", L"");
		double SRadius = (RebarCode::GetBarDiameter(sizeKey, m_ehSel.GetModelRef()));	//箍筋直径

		double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		Transform matrix;
		DPoint3d minP = { 0 }, maxP = { 0 };
		mdlElmdscr_computeRange(&minP, &maxP, Eleeh.GetElementDescrP(), NULL); // 计算指定元素描述符中元素的范围 

		double lenth = maxP.x - minP.x;
		double width = maxP.y - minP.y;
		double height = maxP.z - minP.z;
		DPoint3d ptCenter = minP;
		ptCenter.Add(maxP);
		ptCenter.Scale(0.5);
		mdlTMatrix_getIdentity(&matrix); // 将 transformP 设置为身份转换 
		// 按 xScale、yScale 和 zScale 缩放 inTransP 矩阵部分的各个列。
		// 这等效于形成矩阵乘积(inTransP * scaleTransform)，其中 scaleTransform 是在其矩阵对角线上具有给定尺度且平移为零的变换。
		mdlTMatrix_scale(&matrix, &matrix, (1 - (2 * GetSideCover()*uor_now + 2*SRadius) / lenth), (1 - (2 * GetSideCover()*uor_now + 2*SRadius) / width), 1);
		//设置transformP 的平移部分，使originP 成为变换的固定点。
		//也就是说，变换将 originP 移回原来的位置。 这与在 mdlTMatrix_setTranslation 中完成的简单设置翻译不同。
		mdlTMatrix_setOrigin(&matrix, &ptCenter);
		TransformInfo tInfo(matrix);
		Eleeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(Eleeh, tInfo);

	}
	bool bRet = AnalyzingSlabGeometricData(Eleeh);
	if (!bRet)
		return false;

	//DPoint3d ptStart = PopSTwallData().ptStart;            //原点
	//DPoint3d ptEnd = PopSTwallData().ptEnd;

	//CVector3D  xVec(ptStart, ptEnd);

	//CVector3D  yVec = CVector3D::kZaxis;     //返回两个向量的（标量）叉积。y                                          //标成单位大小为-1

	//CVector3D  xVecNew(ptEnd, ptStart);
	//BeMatrix   placement = CMatrix3D::Ucs(ptEnd, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	//SetPlacement(placement);
	//PopvecFrontPts().push_back(ptStart);
	//PopvecFrontPts().push_back(ptEnd);
	PopvecFrontPts().push_back(m_STslabData.ptStart);
	PopvecFrontPts().push_back(m_STslabData.ptEnd);
	DPoint3d ptStart, ptEnd;
	DPoint3d ptOrgin = m_STslabData.ptStart;

	ptStart = m_STslabData.ptStart;
	ptEnd = m_STslabData.ptEnd;

	DVec3d tmpz = m_STslabData.vecZ;
	tmpz.Normalize();

	CVector3D  yVec = tmpz;     //返回两个向量的（标量）叉积。y  	
	yVec.Scale(-1);
	CVector3D  xVecNew(ptStart, ptEnd);
	xVecNew.Normalize();
	bool isXtY = false;
	tmpz.Scale(m_STslabData.width);
	ptOrgin.Add(tmpz);
	BeMatrix   placement = CMatrix3D::Ucs(ptOrgin, xVecNew, yVec, isXtY);		//方向为X轴，水平垂直方向为Y轴
	//placement.SetScaleFactors(1, 1, -1);
	SetPlacement(placement);
	PopvecFrontPts().push_back(ptStart);
	PopvecFrontPts().push_back(ptEnd);
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}


bool AbanurusRebarAssembly::AnalyzingSlabGeometricData(ElementHandleCR eh)
{
	__super::AnalyzingSlabGeometricData(eh);
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	m_model = model;
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);

	if (!Eleeh.IsValid())
	{
		mdlDialog_dmsgsPrint(L"非法的基础实体!");
		return false;
	}
	if (m_pOldElm == NULL)
	{
		m_pOldElm = new EditElementHandle();
	}
	m_pOldElm->Duplicate(Eleeh);
	// m_pOldElm->AddToModel();
	DPoint3d minP, maxP;
	//计算指定元素描述符中元素的范围。
	mdlElmdscr_computeRange(&minP, &maxP, Eleeh.GetElementDescrP(), NULL);

	PopSTwallData().height = (maxP.y - minP.y)*uor_now / uor_ref;
	PopSTwallData().length = (maxP.x - minP.x)*uor_now / uor_ref;
	PopSTwallData().width = (maxP.z - minP.z)*uor_now / uor_ref;
	PopSTwallData().ptStart = minP;
	PopSTwallData().ptEnd = minP;
	PopSTwallData().ptEnd.x = maxP.x;
	m_height = PopSTwallData().width;
// 	m_STwallData.height = m_BaseData.height;
// 	m_STwallData.length = m_BaseData.length;
// 	m_STwallData.width = m_BaseData.width;
// 	m_STwallData.ptStart = m_BaseData.ptStart;
// 	m_STwallData.ptEnd = m_BaseData.ptEnd;
	m_Holeehs = Holeehs;
	return true;
}


bool AbanurusRebarAssembly::SetBaseData(ElementHandleCR eh)
{
	bool bRet = AnalyzingSlabGeometricData(eh);
	if (!bRet)
		return false;

	DPoint3d ptStart = PopSTwallData().ptStart;            //原点
	DPoint3d ptEnd = PopSTwallData().ptEnd;
	ptStart.z += PopSTwallData().width;		//取板上面的面为原点
	ptEnd.z += PopSTwallData().width;

	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis;     //返回两个向量的（标量）叉积。y                                          //标成单位大小为-1

	CVector3D  xVecNew(ptEnd, ptStart);
	BeMatrix   placement = CMatrix3D::Ucs(ptEnd, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);
	PopvecFrontPts().push_back(ptStart);
	PopvecFrontPts().push_back(ptEnd);
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}



bool AbanurusRebarAssembly::MakeRebars(DgnModelRefP modelRef)      // 创建箍筋
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double sidecover = GetSideCover();
	if (isarc==true)
	{
		BrString sizeKey = m_StirrupData.rebarSize;
		sizeKey.Replace(L"mm", L"");
		double SRadius = (RebarCode::GetBarDiameter(sizeKey, m_ehSel.GetModelRef()));	//箍筋半径
		SetSideCover(SRadius/uor_per_mm);
	}
	int tmprebarstyle = g_globalpara.Getrebarstyle();
	g_globalpara.Setrebarstyle(0);
	m_isAbanurus = true;//设置父类是支墩配筋
	__super::MakeRebars(modelRef);
	g_globalpara.Setrebarstyle(tmprebarstyle);
	SetSideCover(sidecover);
	if (g_globalpara.Getrebarstyle() != 0)
	{
		NewRebarAssembly(modelRef);
	}
	RebarSetTagArray tmprsetTags = this->rsetTags;
	if (tmprsetTags.GetSize() > 0)
	{
		vector<ElementId> rebar_Vs;
		vector<ElementId> rebarId_Hs;

		PIT::StirrupRebarData rebarData;
		rebarData.beg = { 6,0,0 };
		rebarData.end = { 6,0,0 };
		rebarData.rebarSize = m_StirrupData.rebarSize;
		BrString rebarType(m_StirrupData.rebarType);
		rebarData.rebarType = rebarType;

		RebarSymbology rebarSymb;

		rebarSymb.SetRebarLevel(TEXT_OTHER_REBAR);
		rebarData.rebarSymb = rebarSymb;

		m_PStirrupRebarMaker = new BaseStirrupRebarMaker(rebar_Vs, rebarId_Hs, rebarData);

		vector<CPoint3D> pts;
		
		bool up = true;
		double sideCover = GetSideCover()*uor_per_mm;
		double PositiveCover = GetPositiveCover()*uor_per_mm;
		BrString sizeKey = rebarData.rebarSize;
		sizeKey.Replace(L"mm", L"");

		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, rebarSymb);

		double SRadius = (RebarCode::GetBarDiameter(sizeKey, m_ehSel.GetModelRef())) / 2.0;	//箍筋半径
		double AllLen = PopSTwallData().width - SRadius * 2 * m_StirrupData.rebarNum; //可以用来分布箍筋层的Z值长度
		double dLevelSpace = AllLen / (m_StirrupData.rebarNum + 1); //根据箍筋个数算出他的层间距是等距离的
		m_PStirrupRebarMaker->CalStirrupRebarPts(pts, rebarData, PopSTwallData(), sideCover, dLevelSpace, m_StirrupData.rebarStartOffset, up, m_ehSel);//直接偏移1个层间距，不考虑正面保护层

// 		double offset = RebarCode::GetBarDiameter(sizeKey, m_ehSel.GetModelRef());	//箍筋直径
// 		m_PStirrupRebarMaker->CalStirrupRebarPts(pts, rebarData, PopSTwallData(), sideCover, PositiveCover, m_StirrupData.rebarStartOffset, up, m_ehSel);
// 		double AllLen = PopSTwallData().width - GetPositiveCover() * uor_per_mm - GetReverseCover() * uor_per_mm - m_StirrupData.rebarStartOffset * uor_per_mm - m_StirrupData.rebarEndOffset * uor_per_mm - offset;//可以用来分布箍筋层的Z值长度
// 		double dLevelSpace = AllLen / (m_StirrupData.rebarNum - 1); //根据箍筋个数算出他的层间距是等距离的
		vector<ElementId> vecElementID;
		vecElementID.push_back(0);

		if (m_PStirrupRebarMaker->IsArcBase)
		{//为圆形基础

			RebarSetP   rebarSet = RebarSet::Fetch(vecElementID[0], modelRef);
			if (NULL == rebarSet)
			{
				return NULL;
			}
			rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
			rebarSet->SetCallerId(GetCallerId());
			rebarSet->StartUpdate(modelRef);

			RebarEndType endTypeStart, endTypeEnd;
			endTypeStart.SetType(RebarEndType::kNone);
			endTypeEnd.SetType(RebarEndType::kNone);
			double startbendRadius, endbendRadius;
			double startbendLen, endbendLen;
			double begStraightAnchorLen, endStraightAnchorLen;

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
			vector<PITRebarCurve>     rebarCurvesNum;
			m_PStirrupRebarMaker->makeRoundRebarCurve(rebarCurvesNum, endTypes, modelRef, dLevelSpace, m_StirrupData.rebarNum,SRadius*2);

			int numRebar = (int)rebarCurvesNum.size();

			vector<DSegment3d> vecStartEnd;
			for (int j = 0; j < (int)rebarCurvesNum.size(); ++j)
			{
				PITRebarCurve rebarCurve = rebarCurvesNum[j];
				RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, rebarSymb, modelRef);
				if (nullptr != rebarElement)
				{
					RebarShapeData shape;
					shape.SetSizeKey((LPCTSTR)sizeKey);
//					shape.SetIsStirrup(isStirrup);
					shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
					RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
					rebarElement->Update(rebarCurve, SRadius*2, endTypes, shape, modelRef, false);
				}
			}

			RebarSetData setdata;
			setdata.SetNumber(numRebar);
			setdata.SetNominalSpacing(100.00);
			setdata.SetAverageSpacing(100.00);

			int ret = -1;
			ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

			RebarSetTag* tag = new RebarSetTag();
			tag->SetRset(rebarSet);
			tag->SetIsStirrup(false);
			tag->SetBarSetTag(tmprsetTags.GetSize() + 1);			
			tmprsetTags.Add(tag);
// 			tag->SetRset(rebarSet);
// 			return tag;
		}
		else
		{//方形基础	
			//for (int x = 0; x < m_StirrupData.rebarNum - 1; x++)
			//{//对pts做往下的偏移
			//	for (auto it = pts.begin(); it < pts.end(); it++)
			//	{
			//		it->z -= dLevelSpace;//往下偏移层间距
			//		it->z -= SRadius * 2;//往下偏移钢筋直径
			//	}
			//	m_PStirrupRebarMaker->m_vecStirrupRebarPts.push_back(pts);
			//	m_PStirrupRebarMaker->m_pStirrupRebars.push_back(shared_ptr<BaseStirrupRebar>(new BaseStirrupRebar(pts, rebarData)));
			//}
			/*根据点筋位置设置箍筋的五个点*/
			int pt_Hnum = GetAbanurus_PTRebarData().ptHNum;
			int pt_Vnum = GetAbanurus_PTRebarData().ptVNum;
			int pt_Vanum = pt_Vnum - 2;//左右两排钢筋存储的实际的数目，减去了上下两根
			int strrrupHNum = pt_Hnum / 2;
			int strrrupVNum = pt_Vnum / 2;
			WString ptSizekey(GetAbanurus_PTRebarData().ptrebarSize);
			double pt_radius = RebarCode::GetBarDiameter(ptSizekey, ACTIVEMODEL) / 2;//点筋半径
			WString strpSizekey(GetStirrupData().rebarSize);
			double strp_radius = RebarCode::GetBarDiameter(strpSizekey, ACTIVEMODEL) / 2;//箍筋半径
			vector<CPoint3D> tmppts;//存储箍筋的五个点
			EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());

			DRange3d eeh_range;//支墩的range
			mdlElmdscr_computeRange(&eeh_range.low, &eeh_range.high, testeeh.GetElementDescrCP(), nullptr);
			double highZ = eeh_range.high.z;
			
			//设置横纵箍筋的起始点并配筋
			SetStirrupPts(pt_Hnum, pt_Vnum, pt_Vanum, strrrupHNum, strrrupVNum, pt_radius, strp_radius, highZ, dLevelSpace, rebarData,tmprsetTags,modelRef);

			/* H拉筋 */
			if (pt_Hnum % 2)
			{
				vector<vector<Dpoint3d>> tieRebars;
				tieRebars.clear();
				vector<Dpoint3d> tieRebar;
				for (int i = 1; i < m_StirrupData.rebarNum + 1; ++i)
				{
					tieRebar.clear();
					Dpoint3d tieStr = m_mapRroundPts[1].front();
					Dpoint3d tieEnd = m_mapRroundPts[2].front();
					tieStr.x += pt_radius + strp_radius;
					tieStr.y -= (pt_radius + strp_radius);
					//tieStr.z = highZ - (dLevelSpace + 2 * strp_radius) * i;
					tieStr.z = highZ - (dLevelSpace * i + strp_radius * (2 * i - 1));
					tieEnd.x += pt_radius + strp_radius;
					tieEnd.y += (pt_radius + strp_radius);
					tieEnd.z = highZ - (dLevelSpace * i + strp_radius * (2 * i - 1));
					tieRebar.emplace_back(tieStr);
					tieRebar.emplace_back(tieEnd);
					tieRebars.emplace_back(tieRebar); //-(dLevelSpace * x + strp_radius * (2 * x - 1))
				}
				
				m_isHtieRebar = true;
				RebarSetTag* tag = MakeTieRebar(tieRebars);
				m_isHtieRebar = false;
				tmprsetTags.Add(tag);
			}
			/* V拉筋 */
			if (pt_Vnum % 2)
			{
				vector<vector<Dpoint3d>> tieRebars;
				tieRebars.clear();
				vector<Dpoint3d> tieRebar;
				for (int i = 1; i < m_StirrupData.rebarNum + 1; ++i)
				{
					tieRebar.clear();
					Dpoint3d tieStr = m_mapRroundPts[3].front();
					Dpoint3d tieEnd = m_mapRroundPts[2].front();
					tieStr.x -= (pt_radius + strp_radius);
					tieStr.y += (pt_radius + strp_radius);
					tieStr.z = highZ - (dLevelSpace * i + strp_radius * (2 * i - 1));
					tieEnd.x += pt_radius + strp_radius;
					tieEnd.y += (pt_radius + strp_radius);
					tieEnd.z = highZ - (dLevelSpace * i + strp_radius * (2 * i - 1));
					tieRebar.emplace_back(tieStr);
					tieRebar.emplace_back(tieEnd);
					tieRebars.emplace_back(tieRebar);
				}
				
				m_isVtieRebar = true;
				RebarSetTag* tag = MakeTieRebar(tieRebars);
				m_isVtieRebar = false;
				tmprsetTags.Add(tag);
			}
		}
	}


	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(tmprsetTags));
	}

	return true;
}

void AbanurusRebarAssembly::SetStirrupPts(int & pt_Hnum, int pt_Vnum, int & pt_Vanum, int & strrrupHNum, int & strrrupVNum, double & pt_radius, double & strp_radius, double & highZ, double &dLevelSpace, PIT::StirrupRebarData &rebarData, RebarSetTagArray &tmprsetTags, DgnModelRefP modelRef)
{
	vector<CPoint3D> tmppts;//存储箍筋的五个点
	WString stirrupstrSize(GetStirrupData().rebarSize);
	if (stirrupstrSize.find(L"mm") != WString::npos)
	{
		stirrupstrSize.ReplaceAll(L"mm", L"");
	}
	double stirrup_radius = 0;/*RebarCode::GetBarDiameter(stirrupstrSize, ACTIVEMODEL);*/
	//RebarSetTagArray tmprsetTags = this->rsetTags;
	//横向
	for (int i = 0; i < strrrupHNum; ++i)
	{
		m_PStirrupRebarMaker->m_vecStirrupRebarPts.clear();
		m_PStirrupRebarMaker->m_pStirrupRebars.clear();
		for (int x = 1; x < m_StirrupData.rebarNum + 1; x++)
		{
			tmppts.clear();
			CPoint3D pt0 = m_mapAllRebarpts[0].at(2 * i).front();
			CPoint3D pt1 = m_mapAllRebarpts[0].at(2 * i + 1).front();
			CPoint3D pt2 = m_mapAllRebarpts[2].at(pt_Hnum - 2 * (1 + i)).front();
			CPoint3D pt3 = m_mapAllRebarpts[2].at(pt_Hnum - 1 - 2 * i).front();
			CPoint3D pt4 = m_mapAllRebarpts[0].at(2 * i).front();
			pt0.x = pt0.x - pt_radius - strp_radius - stirrup_radius;
			pt0.y = pt0.y - pt_radius - strp_radius - stirrup_radius;
			//pt0.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
			pt0.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
			pt1.x = pt1.x + pt_radius + strp_radius + stirrup_radius;
			pt1.y = pt1.y - pt_radius - strp_radius - stirrup_radius;
			pt1.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
			pt2.x = pt2.x + pt_radius + strp_radius + stirrup_radius;
			pt2.y = pt2.y + pt_radius + strp_radius + stirrup_radius;
			pt2.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
			pt3.x = pt3.x - pt_radius - strp_radius - stirrup_radius;
			pt3.y = pt3.y + pt_radius + strp_radius + stirrup_radius;
			pt3.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
			pt4.x = pt4.x - pt_radius - strp_radius - stirrup_radius;
			pt4.y = pt4.y - pt_radius - strp_radius - stirrup_radius;
			pt4.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
			tmppts.emplace_back(pt0);
			tmppts.emplace_back(pt1);
			tmppts.emplace_back(pt2);
			tmppts.emplace_back(pt3);
			tmppts.emplace_back(pt4);

			m_PStirrupRebarMaker->m_vecStirrupRebarPts.push_back(tmppts);
			m_PStirrupRebarMaker->m_pStirrupRebars.push_back(shared_ptr<BaseStirrupRebar>(new BaseStirrupRebar(tmppts, rebarData)));

		}
		RebarSetTag* tag = m_PStirrupRebarMaker->MakeRebar(0/*vecElementID[0]*/, modelRef);
		tag->SetBarSetTag(tmprsetTags.GetSize() + 1);
		tmprsetTags.Add(tag);
	}
	if (!(pt_Vnum == 2 && pt_Hnum == 2))//如果横纵都为2 不需要额外生成纵向箍筋
	{
		//纵向
		if (pt_Vnum == 2)
		{
			m_PStirrupRebarMaker->m_vecStirrupRebarPts.clear();
			m_PStirrupRebarMaker->m_pStirrupRebars.clear();
			for (int x = 1; x < m_StirrupData.rebarNum + 1; x++)
			{
				tmppts.clear();
				CPoint3D pt0 = m_mapAllRebarpts[0].at(0).front();
				CPoint3D pt1 = m_mapAllRebarpts[0].at(m_mapAllRebarpts[0].size() - 1).front();
				CPoint3D pt2 = m_mapAllRebarpts[2].at(0).front();
				CPoint3D pt3 = m_mapAllRebarpts[2].at(m_mapAllRebarpts[2].size() - 1).front();
				CPoint3D pt4 = pt0;
				pt0.x = pt0.x - pt_radius - strp_radius - stirrup_radius;
				pt0.y = pt0.y - pt_radius - strp_radius - stirrup_radius;
				pt0.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
				pt1.x = pt1.x + pt_radius + strp_radius + stirrup_radius;
				pt1.y = pt1.y - pt_radius - strp_radius - stirrup_radius;
				pt1.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
				pt2.x = pt2.x + pt_radius + strp_radius + stirrup_radius;
				pt2.y = pt2.y + pt_radius + strp_radius + stirrup_radius;
				pt2.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
				pt3.x = pt3.x - pt_radius - strp_radius - stirrup_radius;
				pt3.y = pt3.y + pt_radius + strp_radius + stirrup_radius;
				pt3.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
				pt4.x = pt4.x - pt_radius - strp_radius - stirrup_radius;
				pt4.y = pt4.y - pt_radius - strp_radius - stirrup_radius;
				pt4.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
				tmppts.emplace_back(pt0);
				tmppts.emplace_back(pt1);
				tmppts.emplace_back(pt2);
				tmppts.emplace_back(pt3);
				tmppts.emplace_back(pt4);

				m_PStirrupRebarMaker->m_vecStirrupRebarPts.push_back(tmppts);
				m_PStirrupRebarMaker->m_pStirrupRebars.push_back(shared_ptr<BaseStirrupRebar>(new BaseStirrupRebar(tmppts, rebarData)));

			}
			RebarSetTag* tag = m_PStirrupRebarMaker->MakeRebar(0, modelRef);
			tag->SetBarSetTag(tmprsetTags.GetSize() + 1);
			tmprsetTags.Add(tag);
		}
		else
		{
			if (pt_Vnum % 2)//纵向钢筋为单数
			{
				for (int i = 0; i < strrrupVNum; ++i)
				{
					m_PStirrupRebarMaker->m_vecStirrupRebarPts.clear();
					m_PStirrupRebarMaker->m_pStirrupRebars.clear();
					if (i == 0)
					{
						for (int x = 1; x < m_StirrupData.rebarNum + 1; x++)
						{
							tmppts.clear();
							CPoint3D pt0 = m_mapAllRebarpts[0].at(0).front();
							CPoint3D pt1 = m_mapAllRebarpts[0].at(m_mapAllRebarpts[0].size() - 1).front();
							CPoint3D pt2 = m_mapAllRebarpts[1].at(0).front();
							CPoint3D pt3 = m_mapAllRebarpts[3].at(m_mapAllRebarpts[3].size() - 1).front();
							CPoint3D pt4 = pt0;
							pt0.x = pt0.x - pt_radius - strp_radius - stirrup_radius;
							pt0.y = pt0.y - pt_radius - strp_radius - stirrup_radius;
							pt0.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt1.x = pt1.x + pt_radius + strp_radius + stirrup_radius;
							pt1.y = pt1.y - pt_radius - strp_radius - stirrup_radius;
							pt1.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt2.x = pt2.x + pt_radius + strp_radius + stirrup_radius;
							pt2.y = pt2.y + pt_radius + strp_radius + stirrup_radius;
							pt2.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt3.x = pt3.x - pt_radius - strp_radius - stirrup_radius;
							pt3.y = pt3.y + pt_radius + strp_radius + stirrup_radius;
							pt3.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt4.x = pt4.x - pt_radius - strp_radius - stirrup_radius;
							pt4.y = pt4.y - pt_radius - strp_radius - stirrup_radius;
							pt4.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							tmppts.emplace_back(pt0);
							tmppts.emplace_back(pt1);
							tmppts.emplace_back(pt2);
							tmppts.emplace_back(pt3);
							tmppts.emplace_back(pt4);

							m_PStirrupRebarMaker->m_vecStirrupRebarPts.push_back(tmppts);
							m_PStirrupRebarMaker->m_pStirrupRebars.push_back(shared_ptr<BaseStirrupRebar>(new BaseStirrupRebar(tmppts, rebarData)));

						}
					}
					else
					{
						for (int x = 1; x < m_StirrupData.rebarNum + 1; x++)
						{
							tmppts.clear();
							CPoint3D pt0 = m_mapAllRebarpts[3].at(pt_Vanum - 2 * i).front();
							CPoint3D pt1 = m_mapAllRebarpts[1].at(2 * i - 1).front();
							CPoint3D pt2 = m_mapAllRebarpts[1].at(2 * i).front();
							CPoint3D pt3 = m_mapAllRebarpts[3].at(pt_Vanum - 2 * i - 1).front();
							CPoint3D pt4 = pt0;
							pt0.x = pt0.x - pt_radius - strp_radius - stirrup_radius;
							pt0.y = pt0.y - pt_radius - strp_radius - stirrup_radius;
							pt0.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt1.x = pt1.x + pt_radius + strp_radius + stirrup_radius;
							pt1.y = pt1.y - pt_radius - strp_radius - stirrup_radius;
							pt1.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt2.x = pt2.x + pt_radius + strp_radius + stirrup_radius;
							pt2.y = pt2.y + pt_radius + strp_radius + stirrup_radius;
							pt2.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt3.x = pt3.x - pt_radius - strp_radius - stirrup_radius;
							pt3.y = pt3.y + pt_radius + strp_radius + stirrup_radius;
							pt3.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt4.x = pt4.x - pt_radius - strp_radius - stirrup_radius;
							pt4.y = pt4.y - pt_radius - strp_radius - stirrup_radius;
							pt4.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							tmppts.emplace_back(pt0);
							tmppts.emplace_back(pt1);
							tmppts.emplace_back(pt2);
							tmppts.emplace_back(pt3);
							tmppts.emplace_back(pt4);

							m_PStirrupRebarMaker->m_vecStirrupRebarPts.push_back(tmppts);
							m_PStirrupRebarMaker->m_pStirrupRebars.push_back(shared_ptr<BaseStirrupRebar>(new BaseStirrupRebar(tmppts, rebarData)));

						}
					}
					RebarSetTag* tag = m_PStirrupRebarMaker->MakeRebar(0, modelRef);
					tag->SetBarSetTag(tmprsetTags.GetSize() + 1);
					tmprsetTags.Add(tag);
				}
			}
			else//纵向钢筋复数
			{
				for (int i = 0; i < strrrupVNum; ++i)
				{
					m_PStirrupRebarMaker->m_vecStirrupRebarPts.clear();
					m_PStirrupRebarMaker->m_pStirrupRebars.clear();
					if (i == 0)
					{
						for (int x = 1; x < m_StirrupData.rebarNum + 1; x++)
						{
							tmppts.clear();
							CPoint3D pt0 = m_mapAllRebarpts[0].at(0).front();
							CPoint3D pt1 = m_mapAllRebarpts[0].at(m_mapAllRebarpts[0].size() - 1).front();
							CPoint3D pt2 = m_mapAllRebarpts[1].at(0).front();
							CPoint3D pt3 = m_mapAllRebarpts[3].at(m_mapAllRebarpts[3].size() - 1).front();
							CPoint3D pt4 = pt0;
							pt0.x = pt0.x - pt_radius - strp_radius - stirrup_radius;
							pt0.y = pt0.y - pt_radius - strp_radius - stirrup_radius;
							pt0.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt1.x = pt1.x + pt_radius + strp_radius + stirrup_radius;
							pt1.y = pt1.y - pt_radius - strp_radius - stirrup_radius;
							pt1.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt2.x = pt2.x + pt_radius + strp_radius + stirrup_radius;
							pt2.y = pt2.y + pt_radius + strp_radius + stirrup_radius;
							pt2.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt3.x = pt3.x - pt_radius - strp_radius - stirrup_radius;
							pt3.y = pt3.y + pt_radius + strp_radius + stirrup_radius;
							pt3.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt4.x = pt4.x - pt_radius - strp_radius - stirrup_radius;
							pt4.y = pt4.y - pt_radius - strp_radius - stirrup_radius;
							pt4.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							tmppts.emplace_back(pt0);
							tmppts.emplace_back(pt1);
							tmppts.emplace_back(pt2);
							tmppts.emplace_back(pt3);
							tmppts.emplace_back(pt4);

							m_PStirrupRebarMaker->m_vecStirrupRebarPts.push_back(tmppts);
							m_PStirrupRebarMaker->m_pStirrupRebars.push_back(shared_ptr<BaseStirrupRebar>(new BaseStirrupRebar(tmppts, rebarData)));

						}
					}
					else if (i == strrrupVNum - 1)
					{
						for (int x = 1; x < m_StirrupData.rebarNum + 1; x++)
						{
							tmppts.clear();
							CPoint3D pt0 = m_mapAllRebarpts[3].at(0).front();
							CPoint3D pt1 = m_mapAllRebarpts[1].at(m_mapAllRebarpts[1].size() - 1).front();
							CPoint3D pt2 = m_mapAllRebarpts[2].at(0).front();
							CPoint3D pt3 = m_mapAllRebarpts[2].at(m_mapAllRebarpts[2].size() - 1).front();
							CPoint3D pt4 = pt0;
							pt0.x = pt0.x - pt_radius - strp_radius - stirrup_radius;
							pt0.y = pt0.y - pt_radius - strp_radius - stirrup_radius;
							pt0.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt1.x = pt1.x + pt_radius + strp_radius + stirrup_radius;
							pt1.y = pt1.y - pt_radius - strp_radius - stirrup_radius;
							pt1.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt2.x = pt2.x + pt_radius + strp_radius + stirrup_radius;
							pt2.y = pt2.y + pt_radius + strp_radius + stirrup_radius;
							pt2.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt3.x = pt3.x - pt_radius - strp_radius - stirrup_radius;
							pt3.y = pt3.y + pt_radius + strp_radius + stirrup_radius;
							pt3.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt4.x = pt4.x - pt_radius - strp_radius - stirrup_radius;
							pt4.y = pt4.y - pt_radius - strp_radius - stirrup_radius;
							pt4.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							tmppts.emplace_back(pt0);
							tmppts.emplace_back(pt1);
							tmppts.emplace_back(pt2);
							tmppts.emplace_back(pt3);
							tmppts.emplace_back(pt4);

							m_PStirrupRebarMaker->m_vecStirrupRebarPts.push_back(tmppts);
							m_PStirrupRebarMaker->m_pStirrupRebars.push_back(shared_ptr<BaseStirrupRebar>(new BaseStirrupRebar(tmppts, rebarData)));

						}
					}
					else
					{
						for (int x = 1; x < m_StirrupData.rebarNum + 1; x++)
						{
							tmppts.clear();
							CPoint3D pt0 = m_mapAllRebarpts[3].at(pt_Vanum - 2 * i).front();
							CPoint3D pt1 = m_mapAllRebarpts[1].at(2 * i - 1).front();
							CPoint3D pt2 = m_mapAllRebarpts[1].at(2 * i).front();
							CPoint3D pt3 = m_mapAllRebarpts[3].at(pt_Vanum - 2 * i - 1).front();
							CPoint3D pt4 = pt0;
							pt0.x = pt0.x - pt_radius - strp_radius - stirrup_radius;
							pt0.y = pt0.y - pt_radius - strp_radius - stirrup_radius;
							pt0.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt1.x = pt1.x + pt_radius + strp_radius + stirrup_radius;
							pt1.y = pt1.y - pt_radius - strp_radius - stirrup_radius;
							pt1.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt2.x = pt2.x + pt_radius + strp_radius + stirrup_radius;
							pt2.y = pt2.y + pt_radius + strp_radius + stirrup_radius;
							pt2.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt3.x = pt3.x - pt_radius - strp_radius - stirrup_radius;
							pt3.y = pt3.y + pt_radius + strp_radius + stirrup_radius;
							pt3.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							pt4.x = pt4.x - pt_radius - strp_radius - stirrup_radius;
							pt4.y = pt4.y - pt_radius - strp_radius - stirrup_radius;
							pt4.z = highZ - (dLevelSpace * x + strp_radius * (2 * x - 1));
							tmppts.emplace_back(pt0);
							tmppts.emplace_back(pt1);
							tmppts.emplace_back(pt2);
							tmppts.emplace_back(pt3);
							tmppts.emplace_back(pt4);

							m_PStirrupRebarMaker->m_vecStirrupRebarPts.push_back(tmppts);
							m_PStirrupRebarMaker->m_pStirrupRebars.push_back(shared_ptr<BaseStirrupRebar>(new BaseStirrupRebar(tmppts, rebarData)));

						}
					}
					RebarSetTag* tag = m_PStirrupRebarMaker->MakeRebar(0, modelRef);
					tag->SetBarSetTag(tmprsetTags.GetSize() + 1);
					tmprsetTags.Add(tag);
				}
			}

		}
	}
}

RebarSetTag * AbanurusRebarAssembly::MakeTieRebar(vector<vector<Dpoint3d>> & rebarPts)
{
	BrString sizekey = GetStirrupData().rebarSize;
	double diameter = RebarCode::GetBarDiameter(sizekey, ACTIVEMODEL);		//乘以了10
	double radius = diameter / 2;//点筋的半径
	int setCount = 0;

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kCog);
	endTypeEnd.SetType(RebarEndType::kCog);
	double bendLength = RebarCode::GetBendLength(sizekey, endTypeEnd, ACTIVEMODEL);
	double bendRadius = RebarCode::GetPinRadius(sizekey, ACTIVEMODEL, false);
	RebarEndTypes endtypes{ endTypeStart, endTypeEnd };
	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(0);
	start.SetbendLen(bendLength);
	start.SetbendRadius(bendRadius);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(0);
	end.SetbendLen(bendLength);
	end.SetbendRadius(bendRadius);
	CVector3D  strNormal = CVector3D::From(0, 0, 0),endNormal = CVector3D::From(0, 0, 0);
	if (m_isHtieRebar)
	{
		strNormal = CVector3D::From(-1, 0, 0);
		endNormal = CVector3D::From(-1, 0, 0);
	}
	else
	{
		strNormal = CVector3D::From(0, -1, 0);
		endNormal = CVector3D::From(0, -1, 0);
	}
	start.SetendNormal(strNormal);
	end.SetendNormal(endNormal);

	PITRebarEndTypes PITendTypes = { start,end };

	ElementId tmpId = 0;
	RebarSetP   rebarSet = RebarSet::Fetch(tmpId, ACTIVEMODEL);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(ACTIVEMODEL);

	vector<PITRebarCurve>     rebarCurvesNum;
	vector<PITRebarCurve>     rebarCurves;
	PITRebarEndTypes		tmpendTypes;

	for (auto it : rebarPts)
	{
		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(it.front());
		vex->SetType(RebarVertex::kStart);
		tmpendTypes.beg = PITendTypes.beg;
		tmpendTypes.beg.SetptOrgin(it.front());

		tmpendTypes.end = PITendTypes.end;
		tmpendTypes.end.SetptOrgin(it.back());
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(it.back());
		vex->SetType(RebarVertex::kEnd);

		rebar.EvaluateEndTypes(tmpendTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebarCurves.push_back(rebar);
	}
	
	rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	
	int numRebar = (int)rebarCurvesNum.size();
	//BrString sizeKey = GetAbanurus_PTRebarData().ptrebarSize;
	RebarSymbology symb;
	{
		string str(sizekey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_TIE_REBAR);
	}
	int j = 0;
	bool isStirrup = false;
	double spacing = 0.0;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		RebarElementP rebarElement = nullptr;
		rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, ACTIVEMODEL);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizekey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / UOR_PER_MilliMeter);
			rebarElement->Update(rebarCurve, diameter, endtypes, shape, ACTIVEMODEL, false);


			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string Stype = "TieRebar";
			string Level = to_string(0);
			int grade = GetAbanurus_PTRebarData().ptrebarType;
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Level, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / UOR_PER_MilliMeter, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}
	RebarSetData setdata;
	setdata.SetNumber(numRebar);

	CString spacingstring;
	spacingstring.Format(_T("%lf"), spacing / UOR_PER_MilliMeter);
	setdata.SetSpacingString(spacingstring);
	setdata.SetNominalSpacing(spacing / UOR_PER_MilliMeter);
	setdata.SetAverageSpacing(spacing / UOR_PER_MilliMeter);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, ACTIVEMODEL);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);
	
	return tag;
}



//void AbanurusRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
//{
//	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
//	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
//	m_useHoleehs.clear();
//	double dSideCover = GetSideCover()*uor_per_mm;
//	Transform matrix;
//	GetPlacement().AssignTo(matrix);
//
//	Transform trans;
//	GetPlacement().AssignTo(trans);
//	trans.InverseOf(trans);
//
//	if (g_wallRebarInfo.concrete.isHandleHole)//计算需要处理的孔洞
//	{
//		for (int j = 0; j < m_Holeehs.size(); j++)
//		{
//			EditElementHandle eeh;
//			eeh.Duplicate(*m_Holeehs.at(j));
//
//			ISolidKernelEntityPtr entityPtr;
//			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
//			{
//				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
//				ElementCopyContext copier2(ACTIVEMODEL);
//				copier2.SetSourceModelRef(eeh.GetModelRef());
//				copier2.SetTransformToDestination(true);
//				copier2.SetWriteElements(false);
//				copier2.DoCopy(eeh);
//			}
//
//			TransformInfo transinfo(trans);
//			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
//			DPoint3d minP;
//			DPoint3d maxP;
//			//计算指定元素描述符中元素的范围。
//			mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
//			DRange3d range;
//			range.low = minP;
//			range.high = maxP;
//			bool isNeed = false;
//			if (range.XLength() > misssize || range.ZLength() > misssize)
//			{
//				isNeed = true;
//			}
//
//			if (isNeed)
//			{
//				ElementCopyContext copier(ACTIVEMODEL);
//				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
//				copier.SetTransformToDestination(true);
//				copier.SetWriteElements(false);
//				copier.DoCopy(*m_Holeehs.at(j));
//				PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
//				m_useHoleehs.push_back(m_Holeehs.at(j));
//			}
//		}
//	}
//	if (m_useHoleehs.size() > 1)
//	{
//		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
//	}
//}


//void AbanurusRebarAssembly::CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransStirrup, DgnModelRefP modelRef)
//{
//	if (modelRef == NULL)
//		return;
//
//	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
//
//	vTransform.clear();
//	vTransStirrup.clear();
//	double dSideCover = GetSideCover()*uor_per_mm;
//	double dPositiveCover = GetPositiveCover()*uor_per_mm;
//	double dReverseCover = GetReverseCover()*uor_per_mm;
//	double dLevelSpace = 0;
//
//	double diameterStirrup = 0.0;
//	BrString strStirrupRebarSize(m_StirrupRebaSize);
//	if (strStirrupRebarSize != L"")
//	{
//		if (strStirrupRebarSize.Find(L"mm") != -1)
//		{
//			strStirrupRebarSize.Replace(L"mm", L"");
//		}
//		diameterStirrup = RebarCode::GetBarDiameter(strStirrupRebarSize, modelRef);	//箍筋直径
//	}
//
//	double dOffset = dPositiveCover;
//	for (size_t i = 0; i < 2; i++)
//	{
//		WString strSize = GetvecDirSize().at(i);
//		if (strSize.find(L"mm") != WString::npos)
//		{
//			strSize.ReplaceAll(L"mm", L"");
//		}
//
//		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10
//// 		double diameterTb = 0.0;
//// 		if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
//// 		{
//// 			diameterTb = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(i).rebarSize, modelRef);		//乘以了10
//// 		}
//
//		if (diameter > BE_TOLERANCE)
//		{
//			CVector3D	zTrans(0.0, 0.0, 0.0);
//			if (GetvecDir().at(i) == 0) //水平
//			{
//				zTrans.z = m_BaseData.height - dSideCover - diameterStirrup - diameter * 0.5;//直钢筋位置还需偏移一个箍筋直径，不是箍筋位置
//				zTrans.x = m_BaseData.length * 0.5;
//			}
//			else
//			{
//				zTrans.z = m_BaseData.height * 0.5;
//				zTrans.x = dSideCover + diameterStirrup + diameter * 0.5;//直钢筋位置还需偏移一个箍筋直径，不是箍筋位置
//			}
//
//			WString strSizePre;
//			if (i != 0)
//			{
//				strSizePre = WString(GetvecDirSize().at(i - 1).Get());
//				if (strSizePre.find(L"mm") != WString::npos)
//				{
//					strSizePre.ReplaceAll(L"mm", L"");
//				}
//			}
//
//			double diameterPre = RebarCode::GetBarDiameter(strSizePre, modelRef);		//乘以了10
//			if (0 == i)
//			{
//				dOffset += diameter / 2.0;	//偏移首层钢筋半径
////				dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm;
//			}
//			else
//			{
//				dOffset += diameter;
////				dLevelSpace = GetSpace().at(i) * uor_per_mm + diameter * 0.5 + diameterPre * 0.5;//层间距加上当前钢筋直径
//			}
//
//			//				dOffset += dLevelSpace;
//			//				dOffsetTb = dOffset;
//			// 				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
//			// 				{
//			// 					if (diameterTb > diameter)//并筋层的钢筋比主筋直径大
//			// 						dOffsetTb += (diameterTb / 2.0 - diameter / 2.0);
//			// 					else
//			// 						dOffsetTb -= (diameter / 2.0 - diameterTb / 2.0);
//			// 				}
//			if (COMPARE_VALUES(m_BaseData.width - dOffset, dReverseCover) < 0)		//当前钢筋层已嵌入到了反面保护层中时，实际布置的钢筋层间距就不再使用设置的与上层间距，而是使用保护层进行限制
//			{
//				zTrans.y = m_BaseData.width - dReverseCover - diameter / 2.0;
//				//					zTransTb.y = zTrans.y;
//				// 					if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
//				// 					{
//				// 						if (diameterTb > diameter)//并筋层的钢筋比主筋直径大
//				// 							zTransTb.y += (diameterTb / 2.0 - diameter / 2.0);
//				// 						else
//				// 							zTransTb.y -= (diameter / 2.0 - diameterTb / 2.0);
//				// 					}
//									//判断：如果上一层的zTrans.y与当前层的zTrans.y相同，则上一层减去当前层的钢筋直径。（防止钢筋碰撞）
//				double compare = zTrans.y;
//				if (vTransform.size() > 0)
//				{
//					double reverseOffset = diameter;
//					for (int j = (int)vTransform.size() - 1; j >= 0; j--)
//					{
//						WString strSize1 = GetvecDirSize().at(j);
//						if (strSize1.find(L"mm") != WString::npos)
//						{
//							strSize1.ReplaceAll(L"mm", L"");
//						}
//						diameterPre = RebarCode::GetBarDiameter(strSize1, modelRef);		//乘以了10
//						if (COMPARE_VALUES(vTransform[j].y + diameterPre * 0.5, compare - diameter * 0.5) > 0)	//嵌入了下一根钢筋中
//						{
//							vTransform[j].y -= reverseOffset;
//							//								vTransformTb[j].y = vTransform[j].y;
//							// 								if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(j).hasTwinbars)
//							// 								{
//							// 									double diameterTbPre = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(j).rebarSize, modelRef);		//乘以了10
//							// 
//							// 									if (diameterTbPre > diameterPre)//并筋层的钢筋比主筋直径大
//							// 										vTransformTb[j].y -= (diameterTbPre / 2.0 - diameterPre / 2.0);
//							// 									else
//							// 										vTransformTb[j].y += (diameterPre / 2.0 - diameterTbPre / 2.0);
//							// 								}
//							compare = vTransform[j].y;
//							diameter = diameterPre;
//						}
//					}
//				}
//			}
//			else
//			{
//				zTrans.y = dOffset;
//				//					zTransTb.y = dOffsetTb;
//									// 					if (GetvecTwinRebarLevel().at(i).hasTwinbars && diameterTb > diameter)	//并筋层的钢筋比主筋直径大
//									// 					{
//									// 						zTrans.y -= (diameterTb / 2.0 - diameter / 2.0) * 2;
//									// 						zTransTb.y -= (diameterTb / 2.0 - diameter / 2.0);
//									// 					}
//			}
//			vTransform.push_back(zTrans);
//		}
//	}
//
//	dOffset = dPositiveCover;
//	dLevelSpace = 0;
//	for (int x = 0;x < m_StirrupRebaNum; x++)//箍筋侧面偏移量
//	{
//		CVector3D	zTransStirrup;
//		zTransStirrup.z = m_BaseData.height - dSideCover - diameterStirrup * 0.5;
//		zTransStirrup.x = m_BaseData.length * 0.5;
//		if (x == 0)
//		{
//			dOffset += diameterStirrup / 2.0;	//偏移首层箍筋半径
//			dLevelSpace = dOffset;
//		}
//		else
//		{
//			dLevelSpace = (m_BaseData.width - GetPositiveCover() - GetReverseCover() - GetStirrupStartOffset() - GetStirrupEndOffset()) / m_StirrupRebaNum - diameterStirrup;
//		}
//		dOffset += dLevelSpace;
//		if (COMPARE_VALUES(m_BaseData.width - dOffset, dReverseCover) < 0)		//当前钢筋层已嵌入到了反面保护层中时，实际布置的钢筋层间距就不再使用设置的与上层间距，而是使用保护层进行限制
//		{
//			zTransStirrup.y = m_BaseData.width - dReverseCover - diameterStirrup / 2.0;
//			double compare = zTransStirrup.y;
//			if (vTransStirrup.size() > 0)
//			{
// 				double reverseOffset = diameterStirrup;
// 				for (int j = (int)vTransStirrup.size() - 1; j >= 0; j--)
// 				{
// 					double diameterPre = diameterStirrup;		//乘以了10
// 					if (COMPARE_VALUES(vTransStirrup[j].y + diameterPre * 0.5, compare - diameterStirrup * 0.5) > 0)	//嵌入了下一根钢筋终
// 					{
// 						vTransform[j].y -= reverseOffset;
// 						compare = vTransform[j].y;
////						diameterStirrup = diameterPre;
// 					}
// 				}
//			}
//		}
//		else
//		{
//			zTransStirrup.y = dOffset;
//		}
//		vTransStirrup.push_back(zTransStirrup);
//	}
//}


//RebarSetTag* AbanurusRebarAssembly::MakeRebars
//(
//	ElementId&          rebarSetId,
//	BrStringCR          sizeKey,
//	double              xLen,
//	double              width,
//	double              spacing,
//	double              startOffset,
//	double              endOffset,
//
//	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
//	CMatrix3D const&    mat,
//	DgnModelRefP        modelRef
//)
//{
//	bool const isStirrup = false;
//
//	double diameterStirrup = 0.0;
//	BrString strStirrupRebarSize(m_StirrupRebaSize);
//	if (strStirrupRebarSize != L"")
//	{
//		if (strStirrupRebarSize.Find(L"mm") != -1)
//		{
//			strStirrupRebarSize.Replace(L"mm", L"");
//		}
//		diameterStirrup = RebarCode::GetBarDiameter(strStirrupRebarSize, modelRef);	//箍筋直径
//	}
//
//	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
//	if (NULL == rebarSet)
//		return NULL;
//
//	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
//	rebarSet->SetCallerId(GetCallerId());
//	rebarSet->StartUpdate(modelRef);
//
//	RebarEndType endTypeStart, endTypeEnd;
//
//	switch (endType[0].endType)
//	{
//	case 0:	//无
//	case 1:	//弯曲
//	case 2:	//吊钩
//	case 3:	//折线
//		endTypeStart.SetType(RebarEndType::kNone);
//		break;
//	case 7:	//直锚
//		endTypeStart.SetType(RebarEndType::kLap);
//		break;
//	case 4:	//90度弯钩
//		endTypeStart.SetType(RebarEndType::kBend);
//		break;
//	case 5:	//135度弯钩
//		endTypeStart.SetType(RebarEndType::kCog);
//		break;
//	case 6:	//180度弯钩
//		endTypeStart.SetType(RebarEndType::kHook);
//		break;
//	case 8:	//用户
//		endTypeStart.SetType(RebarEndType::kCustom);
//		break;
//	default:
//		break;
//	}
//
//	switch (endType[1].endType)
//	{
//	case 0:	//无
//	case 1:	//弯曲
//	case 2:	//吊钩
//	case 3:	//折线
//		endTypeEnd.SetType(RebarEndType::kNone);
//		break;
//	case 7:	//直锚
//		endTypeEnd.SetType(RebarEndType::kLap);
//		break;
//	case 4:	//90度弯钩
//		endTypeEnd.SetType(RebarEndType::kBend);
//		break;
//	case 5:	//135度弯钩
//		endTypeEnd.SetType(RebarEndType::kCog);
//		break;
//	case 6:	//180度弯钩
//		endTypeEnd.SetType(RebarEndType::kHook);
//		break;
//	case 8:	//用户
//		endTypeEnd.SetType(RebarEndType::kCustom);
//		break;
//	default:
//		break;
//	}
////由于板配筋和墙配筋方向不同改保护层正反侧面
//	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
//	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
//	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
//	double startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
//	double endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
//	int numRebar = 0;
//	double adjustedXLen, adjustedSpacing;
//
//
//	double leftSideCov = GetSideCover()*uor_per_mm;
//	double rightSideCov = GetSideCover()*uor_per_mm;
//	double allSideCov = leftSideCov + rightSideCov;
//
//	adjustedXLen = xLen - 2.0 * GetSideCover()*uor_per_mm - diameter - startOffset - endOffset;
//	adjustedXLen = adjustedXLen - diameterStirrup * 2;
//	
//	numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;
//	adjustedSpacing = spacing;
//	if (numRebar > 1)
//		adjustedSpacing = adjustedXLen / (numRebar - 1);
//	
//	double xPos = startOffset;
//	vector<PIT::PITRebarCurve>     rebarCurvesNum;
//	int j = 0;
//	double endTypeStartOffset = endType[0].offset * uor_per_mm;
//	double endTypEendOffset = endType[1].offset * uor_per_mm;
//	if (endType[0].endType != 0)	//端部弯曲时额外偏移钢筋半径
//		endTypeStartOffset += diameter * 0.5;
//	if (endType[1].endType != 0)	//端部弯曲时额外偏移钢筋半径
//		endTypEendOffset += diameter * 0.5;
//
//	PIT::PITRebarEndType start, end;
//	start.SetType((PIT::PITRebarEndType::Type)endTypeStart.GetType());
//	start.Setangle(endType[0].rotateAngle);
//	start.SetbendLen(startbendLen);
//	start.SetbendLen(endbendLen);
//	start.SetbendRadius(bendRadius);
//
//
//	end.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());
//	end.Setangle(endType[1].rotateAngle);
//	end.SetbendLen(endbendLen);
//	end.SetbendRadius(bendRadius);
//
//	PIT::PITRebarEndTypes   endTypes = { start, end };
//
//	for (int i = 0; i < numRebar; i++)//钢筋属性
//	{
//		bool tag;
//		vector<PIT::PITRebarCurve>     rebarCurves;
//
//		tag = true;
//		makeRebarCurve_G(rebarCurves, xPos, width, endTypeStartOffset, endTypEendOffset, endTypes, mat, tag);
//
//
//		xPos += adjustedSpacing;
//
//		if (!tag)
//			continue;
//		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
//	}//rebarset里面rebarelement初步建立完成
//	//钢筋组
//	numRebar = (int)rebarCurvesNum.size();
//
//	RebarSymbology symb;
//	symb.SetRebarColor(-1);
//	symb.SetRebarLevel(TEXT_SLAB_REBAR);
//
//	vector<DSegment3d> vecStartEnd;
//	for (PIT::PITRebarCurve rebarCurve : rebarCurvesNum)
//	{
//		CPoint3D ptstr, ptend;
//		rebarCurve.GetEndPoints(ptstr, ptend);
//		DPoint3d midPos = ptstr;
//		midPos.Add(ptend);
//		midPos.Scale(0.5);
//		if (ISPointInHoles(m_Holeehs, midPos))
//		{
//			if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
//			{
//				continue;
//			}
//		}
//
//		vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));
//
//		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
//		if (nullptr != rebarElement)
//		{
//			RebarShapeData shape;
//
//			shape.SetSizeKey((LPCTSTR)sizeKey);
//			shape.SetIsStirrup(isStirrup);
//			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
//			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
//			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
//
//		}
//		j++;
//	}
//	m_vecAllRebarStartEnd.push_back(vecStartEnd);
//	RebarSetData setdata;
//	setdata.SetNumber(numRebar);
//	setdata.SetNominalSpacing(spacing / uor_per_mm);
//	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);
//
//	int ret = -1;
//	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数
//
//	RebarSetTag* tag = new RebarSetTag();
//	tag->SetRset(rebarSet);
//	tag->SetIsStirrup(isStirrup);
//
//	return tag;
//}



//bool AbanurusRebarAssembly::makeRebarCurve_G
//(
//	vector<PIT::PITRebarCurve>&     rebars,
//	double                  xPos,
//	double                  yLen,
//	double					EndtypeStartOffset,
//	double					EndtypeEndOffset,
//	PIT::PITRebarEndTypes&		endTypes,
//	CMatrix3D const&        mat,
//	bool&                tag
//)
//{
//	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
//	CPoint3D  startPt;
//	CPoint3D  endPt;
//
//	double diameterStirrup = 0.0;
//	BrString strStirrupRebarSize(m_StirrupRebaSize);
//	if (strStirrupRebarSize != L"")
//	{
//		if (strStirrupRebarSize.Find(L"mm") != -1)
//		{
//			strStirrupRebarSize.Replace(L"mm", L"");
//		}
//		diameterStirrup = RebarCode::GetBarDiameter(strStirrupRebarSize, m_modelRef);	//箍筋直径
//	}
//
//
//	double z1 = -yLen / 2.0 + EndtypeStartOffset + GetSideCover() + diameterStirrup;
//	double z2 = yLen / 2.0 - EndtypeEndOffset - GetSideCover() - diameterStirrup;
//	double x1 = xPos+ GetSideCover()+ diameterStirrup;
//	double x2 = xPos + GetSideCover() + diameterStirrup;
//
//	startPt = CPoint3D::From(x1, 0.0, z1);
//	endPt = CPoint3D::From(x2, 0.0, z2);
//
//	Transform trans;
//	mat.AssignTo(trans);
//	TransformInfo transinfo(trans);
//	EditElementHandle eeh;
//	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
//	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
////	eeh.AddToModel();
//
//
//
//	DPoint3d pt1[2];
//	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改
//
//	//确保起点终点是从小到大---begin
//	DVec3d vec = pt1[1] - pt1[0];
//	DVec3d vecX = DVec3d::From(1, 0, 0);
//	vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
//	vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
//	vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
//	vec.Normalize();
//	if (vec.IsPerpendicularTo(vecX))
//	{
//		if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) > 0)
//		{
//			DPoint3d ptTmp = pt1[0];
//			pt1[0] = pt1[1];
//			pt1[1] = ptTmp;
//		}
//	}
//	else
//	{
//		if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) > 0)
//		{
//			DPoint3d ptTmp = pt1[0];
//			pt1[0] = pt1[1];
//			pt1[1] = ptTmp;
//		}
//	}
//
//
//	CVector3D  Vec(pt1[0], pt1[1]);
//	CVector3D  nowVec = Vec;
//
//	nowVec.Normalize();
//
//	nowVec.ScaleToLength(EndtypeStartOffset);
//	pt1[0].Add(nowVec);
//	nowVec.Negate();
//	nowVec.ScaleToLength(EndtypeEndOffset);
//	pt1[1].Add(nowVec);
//
//	DVec3d vec1 = pt1[1] - pt1[0];
//	DVec3d vecX1 = DVec3d::From(1, 0, 0);
//	vec1.x = COMPARE_VALUES_EPS(abs(vec1.x), 0, 10) == 0 ? 0 : vec1.x;
//	vec1.y = COMPARE_VALUES_EPS(abs(vec1.y), 0, 10) == 0 ? 0 : vec1.y;
//	vec1.z = COMPARE_VALUES_EPS(abs(vec1.z), 0, 10) == 0 ? 0 : vec1.z;
//	vec1.Normalize();
//	if (vec.IsPerpendicularTo(vecX1))
//	{
//		if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) > 0)
//		{
//			tag = false;
//		}
//	}
//	else
//	{
//		if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) > 0)
//		{
//			tag = false;
//		}
//	}
//
//
//	m_vecRebarPtsLayer.push_back(pt1[0]);
//	m_vecRebarPtsLayer.push_back(pt1[1]);
//
//// 	if (m_isPushTieRebar)
//// 	{
//// 		m_vecTieRebarPtsLayer.push_back(pt1[0]);
//// 		m_vecTieRebarPtsLayer.push_back(pt1[1]);
//// 	}
//
//
//	// EditElementHandle eeh2;
//	// GetContractibleeeh(eeh2);//获取减去保护层的端部缩小的实体
//
//	double dSideCover = GetSideCover() * uor_per_mm;
//	vector<DPoint3d> tmppts;
//	Transform matrix;
//	GetPlacement().AssignTo(matrix);
//	vector<vector<DPoint3d>> vecPtRebars;
//	vector<DPoint3d> tmpptsTmp;
//	vecPtRebars.clear();
//	tmpptsTmp.clear();
// 	if (m_pOldElm != NULL) // 与原实体相交(无孔洞)
// 	{
// 		GetIntersectPointsWithOldElm(tmpptsTmp, m_pOldElm, pt1[0], pt1[1], dSideCover, matrix);
// 
// 		if (tmpptsTmp.size() > 1)
// 		{
// 			// 存在交点为两个以上的情况
// 			GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, pt1[0], pt1[1], m_pOldElm, dSideCover);
// 		}
// 	}
//
//	if (tmpptsTmp.size() < 2 && vecPtRebars.size() == 0)
//	{
//		vector<DPoint3d> vecPt;
//		vecPt.push_back(pt1[0]);
//		vecPt.push_back(pt1[1]);
//
//		vecPtRebars.push_back(vecPt);
//	}
//
//	for (size_t i = 0; i < vecPtRebars.size(); i++)
//	{
//		pt1[0] = vecPtRebars.at(i).at(0);
//		pt1[1] = vecPtRebars.at(i).at(1);
//		GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);
//
//		map<int, DPoint3d> map_pts;
//		bool isStr = false;
//		for (DPoint3d pt : tmppts)
//		{
//			if (ExtractFacesTool::IsPointInLine(pt, pt1[0], pt1[1], ACTIVEMODEL, isStr))
//			{
//				int dis = (int)pt1[0].Distance(pt);
//				if (map_pts.find(dis) != map_pts.end())
//				{
//					dis = dis + 1;
//				}
//				map_pts[dis] = pt;
//			}
//		}
//
//		DPoint3d FirstPoint = pt1[1];
//		DVec3d Zzvec = DVec3d::From(0, 0, -1);
//		Zzvec.ScaleToLength(m_Len);
//		FirstPoint.Add(Zzvec);
//		map_pts[-1] = FirstPoint;
//
//		if (map_pts.find(0) != map_pts.end())
//		{
//			map_pts[1] = pt1[0];
//		}
//		else
//		{
//			map_pts[0] = pt1[0];
//		}
//		int dis = (int)pt1[0].Distance(pt1[1]);
//		if (map_pts.find(dis) == map_pts.end())
//		{
//			map_pts[dis] = pt1[1];
//		}
//		else
//		{
//			dis = dis + 1;
//			map_pts[dis] = pt1[1];
//		}
//
//		DPoint3d ThridPoint = pt1[1];
//		DVec3d Zvec = DVec3d::From(0, 0, -1);
//		Zvec.ScaleToLength(m_Len);
//		ThridPoint.Add(Zvec);
//		map_pts[dis+1] = ThridPoint;//加1存储
//
//// 		if (map_pts.find(m_Len) == map_pts.end())
//// 		{
//// 			map_pts[m_Len] = ThridPoint;
//// 		}
//// 		else
//// 		{
//// 			double temp = m_Len + 1;
//// 			map_pts[temp] = ThridPoint;
//// 		}
//		PIT::PITRebarCurve m_Curve;
//		RebarVertices  vers;
//		bvector<DPoint3d> allPts;
//		double bendRadius = endTypes.beg.GetbendRadius();
//		for (map<int, DPoint3d>::iterator it = map_pts.begin(); it != map_pts.end(); it++)
//		{
//			allPts.push_back(it->second);
//		}
//		GetRebarVerticesFromPoints(vers, allPts, bendRadius);
//		m_Curve.SetVertices(vers);
//		m_Curve.EvaluateEndTypes(endTypes);
//		rebars.push_back(m_Curve);
//
//// 		for (map<int, DPoint3d>::iterator it = map_pts.begin(); it != map_pts.end(); it++)
//// 		{
//// 			RebarVertices  vers;
//// 			bvector<DPoint3d> allPts;
//// 			for (int i = 0; i < (*it).size(); i++)
//// 			{
//// 				if (allPts.size() == 0)
//// 				{
//// 					allPts.push_back((*it)[i].ptStr);
//// 					allPts.push_back((*it)[i].ptEnd);
//// 				}
//// 				else
//// 				{
//// 					allPts.push_back((*it)[i].ptEnd);
//// 				}
//// 			}
//
//
//// 			PIT::PITRebarCurve rebar;
//// 			RebarVertexP vex;
//// 			vex = &rebar.PopVertices().NewElement();
//// 			vex->SetIP(itr->second);
//// 			vex->SetType(RebarVertex::kStart);
//// 			endTypes.beg.SetptOrgin(itr->second);
//// 
//// 			map<int, DPoint3d>::iterator itrplus = ++itr;
//// 			if (itrplus == map_pts.end())
//// 			{
//// 				break;
//// 			}
//// 
//// 			endTypes.end.SetptOrgin(itrplus->second);
//// 
//// 			vex = &rebar.PopVertices().NewElement();
//// 			vex->SetIP(itrplus->second);
//// 			vex->SetType(RebarVertex::kIP);
//// 
//// // 			map<int, DPoint3d>::iterator itrThird = ++itr;
//// // 			vex = &rebar.PopVertices().NewElement();
//// // 			vex->SetIP(itrThird->second);
//// // 			vex->SetType(RebarVertex::kEnd);
//// 
//// 			rebar.EvaluateEndTypes(endTypes);
//// 			rebars.push_back(rebar);
////		}
//
//	}
//	return true;
//}




BaseStirrupRebar::BaseStirrupRebar(const vector<CPoint3D> &vecRebarPts, PIT::StirrupRebarDataCR rebarData)
	: PIT::StirrupRebar(vecRebarPts, rebarData), m_vecRebarPt(vecRebarPts), m_rebarData(rebarData)
{
}

BaseStirrupRebarMaker::BaseStirrupRebarMaker(const vector<ElementId>& rebar_Vs, const vector<ElementId>  &rebarId_Hs, PIT::StirrupRebarDataCR rebarData, bool bUp, DgnModelRefP modeRef)
	: StirrupRebarMaker(rebar_Vs, rebarId_Hs, rebarData), m_bUp(bUp)
{
	IsArcBase = false;
}

bool BaseStirrupRebarMaker::CalStirrupRebarPts(vector<CPoint3D> &pts, PIT::StirrupRebarDataCR rebarData, STWallGeometryInfo& baseData, double& SideCover, double& PositiveCover, double& StartOffset, bool up, ElementHandleR m_ehSel)
{
	double uor_per_mm = m_ehSel.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	vector<DSegment3d> vecDownFontLine;
	vector<DSegment3d> vecDownBackLine;
	EditElementHandle Eleeh(m_ehSel, false);
	BrString sizeKey = rebarData.rebarSize;
	sizeKey.Replace(L"mm", L"");
	double StirrupRadius = (RebarCode::GetBarDiameter(sizeKey, m_ehSel.GetModelRef())) / 2.0;	//箍筋半径

	EditElementHandle testeeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(Eleeh, testeeh, Holeehs);
	EFT::GetFrontBackLinePoint(testeeh, vecDownFontLine, vecDownBackLine, &baseData.width);

	double Sideoffset = SideCover - StirrupRadius;//侧面偏移
	double Positiveoffset = PositiveCover + StirrupRadius;//正面偏移
	if (vecDownFontLine.empty() || vecDownBackLine.empty())
	{ //取不到前面及后面的线表示选择的为圆形的基础  //用IsArcBase为True来表示选择的圆形基础
		IsArcBase = true;
		EditElementHandle eehDownFace;
		ExtractFacesTool::GetDownFace(testeeh, eehDownFace, &baseData.width);
		DPoint3d minP = { 0 }, maxP = { 0 };
		mdlElmdscr_computeRange(&minP, &maxP, eehDownFace.GetElementDescrP(), NULL);//求得底面圆的最大及最小点	
		DPoint3d ArcCenter = minP;
		ArcCenter.Add(maxP);
		ArcCenter.Scale(0.5);
		stArcPoint.centerPtr = ArcCenter;
		stArcPoint.ArcRadius = ArcCenter.y - minP.y;
		stArcPoint.ptStr = ArcCenter;
		stArcPoint.ptStr.x = stArcPoint.ptStr.x - stArcPoint.ArcRadius;
		stArcPoint.ptEnd = ArcCenter;
		stArcPoint.ptEnd.y = stArcPoint.ptEnd.y + stArcPoint.ArcRadius;

		DPoint3d arcMid;
		EditElementHandle eehArc;
		//以ptA为圆心,dRadius为半径，ptIn1为弧的起点，ptIn2为弧的终点画弧
		ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(ArcCenter, stArcPoint.ptStr, stArcPoint.ptEnd), true, *ACTIVEMODEL);
		double dis2 = 0.00;
		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, eehArc.GetElementDescrP(), &stArcPoint.ptEnd, 0.1);
		dis2 /= 2;
		mdlElmdscr_pointAtDistance(&arcMid, nullptr, dis2, eehArc.GetElementDescrP(), 0.1);
		stArcPoint.ptMid = arcMid;

		//stArcPoint整体向上偏移板的高度
		stArcPoint.centerPtr.z += baseData.width;
		stArcPoint.ptStr.z += baseData.width;
		stArcPoint.ptEnd.z += baseData.width;
		stArcPoint.ptMid.z += baseData.width;

		//第一次先正面偏移一个层间距及箍筋半径
		stArcPoint.centerPtr.z -= Positiveoffset;
		stArcPoint.ptStr.z -= Positiveoffset;
		stArcPoint.ptEnd.z -= Positiveoffset;
		stArcPoint.ptMid.z -= Positiveoffset;

		//侧边偏移一个保护层及箍筋半径
		stArcPoint.ptStr.x += Sideoffset;
		stArcPoint.ptEnd.y -= Sideoffset;
		stArcPoint.ptMid.x += Sideoffset;
		stArcPoint.ptMid.y -= Sideoffset;

		stArcPoint.ArcRadius = stArcPoint.ArcRadius - Sideoffset;
		return false;
	}

	DPoint3d pt1[2];
	vecDownFontLine[0].GetStartPoint(pt1[0]);
	vecDownFontLine[0].GetEndPoint(pt1[1]);
	DPoint3d pt2[2];
	vecDownBackLine[0].GetStartPoint(pt2[0]);
	vecDownBackLine[0].GetEndPoint(pt2[1]);

	//将底面的4个点变为顶面的4个点
	pt1[0].z += baseData.width;
	pt1[1].z += baseData.width;
	pt2[0].z += baseData.width;
	pt2[1].z += baseData.width;

	//偏移侧边保护层+箍筋半径

//	double Positiveoffset = PositiveCover + StartOffset * uor_per_mm + StirrupRadius;//正面偏移

//往里缩小得到箍筋的四个点，PositiveCover是一个层间距dLevelSpace,没有考虑正面保护层

	CVector3D vecx = pt1[0] - pt1[1];
	vecx.Normalize();//X负方向

	CVector3D vecy = vecx;//Y正方向
	vecy = vecy.CrossProduct(CVector3D::kZaxis);

	CVector3D vecz = vecx;//Z负方向
	vecz = vecz.CrossProduct(vecz.Perpendicular());


	movePoint(vecx, pt1[0], Sideoffset, false);
	movePoint(vecy, pt1[0], Sideoffset, true);
	movePoint(vecz, pt1[0], Positiveoffset, true);

	movePoint(vecx, pt1[1], Sideoffset, true);
	movePoint(vecy, pt1[1], Sideoffset, true);
	movePoint(vecz, pt1[1], Positiveoffset, true);
	
	movePoint(vecx, pt2[0], Sideoffset, false);
	movePoint(vecy, pt2[0], Sideoffset, false);
	movePoint(vecz, pt2[0], Positiveoffset, true);

	movePoint(vecx, pt2[1], Sideoffset, true);
	movePoint(vecy, pt2[1], Sideoffset, false);
	movePoint(vecz, pt2[1], Positiveoffset, true);

// 	pt1[0].x += Sideoffset;
// 	pt1[0].y += Sideoffset;
// 	pt1[0].z -= Positiveoffset;
// 
// 	pt1[1].x -= Sideoffset;
// 	pt1[1].y += Sideoffset;
// 	pt1[1].z -= Positiveoffset;
// 
// 	pt2[0].x += Sideoffset;
// 	pt2[0].y -= Sideoffset;
// 	pt2[0].z -= Positiveoffset;

// 	pt2[1].x -= Sideoffset;
// 	pt2[1].y -= Sideoffset;
// 	pt2[1].z -= Positiveoffset;

	pts = { pt1[0], pt1[1], pt2[1], pt2[0], pt1[0] };
	m_vecStirrupRebarPts.push_back(pts);
	m_pStirrupRebars.push_back(shared_ptr<BaseStirrupRebar>(new BaseStirrupRebar(pts, rebarData)));

	return true;
}



RebarSetTag * BaseStirrupRebarMaker::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return nullptr;
	}

	//添加并筋
	RebarSetP rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (rebarSet == nullptr)
	{
		return nullptr;
	}
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(rebarSetId);
	rebarSet->StartUpdate(modelRef);
	int rebarNum = (int)m_pStirrupRebars.size();
	for (int i = 0; i < rebarNum; ++i)
	{
		m_pStirrupRebars[i]->Create(*rebarSet);
	}

	RebarSetTag *tag = new RebarSetTag;
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);
	return tag;
}



bool BaseStirrupRebarMaker::makeRoundRebarCurve(vector<PIT::PITRebarCurve>& rebar,PIT::PITRebarEndTypes& endTypes, DgnModelRefP modelRef, double& levelSpacing, int& rebarNum, double diameter)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	int nRebarNum = rebarNum;
	double dSpace = levelSpacing + diameter;//Z轴上偏移一个层间距及箍筋直径

	DPoint3d	centerPoint = stArcPoint.centerPtr;
	for (int i = 0; i < nRebarNum; i++)
	{
		PITRebarCurve rebarCurve;
		if (CalculateRound(rebarCurve, stArcPoint.ptStr, stArcPoint.ptMid, stArcPoint.ptEnd, 1))
		{
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(stArcPoint.ptStr, stArcPoint.ptMid, stArcPoint.ptEnd), true, *ACTIVEMODEL);
//			arceeh1.AddToModel();
		}

		DPoint3d arcStr = stArcPoint.ptEnd;
		// 画后三段弧
		for (int i = 0; i < 3; i++)
		{
			DPoint3d arcEnd = centerPoint;

			CVector3D vecNormal = arcStr - centerPoint;
			vecNormal.Normalize();

			vecNormal = vecNormal.Perpendicular();
			movePoint(vecNormal, arcEnd, centerPoint.Distance(arcStr));

			//以ptA为圆心,dRadius为半径，ptIn1为弧的起点，ptIn2为弧的终点画弧
			EditElementHandle eehArc;
			ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(centerPoint, arcStr, arcEnd), true, *ACTIVEMODEL);

			double dis2 = 0.00;
			DPoint3d arcMid;

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
				EditElementHandle arceeh1;
				ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(arcStr, arcMid, arcEnd), true, *ACTIVEMODEL);
//				arceeh1.AddToModel();
			}
			arcStr = arcEnd;
		}
		rebar.push_back(rebarCurve);

		centerPoint.z -= dSpace;
		stArcPoint.ptStr.z -= dSpace;
		stArcPoint.ptEnd.z -= dSpace;
		stArcPoint.ptMid.z -= dSpace;
	}

	return true;
}




bool BaseStirrupRebarMaker::CalculateRound(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt, int nStep)
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

void BaseStirrupRebarMaker::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}
