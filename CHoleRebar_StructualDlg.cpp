// CHoleRebar_StructualDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "resource.h"
#include "CHoleRebar_StructualDlg.h"
#include "CHoleRebarListCtrl.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"
#include "GalleryIntelligentRebarids.h"
#include "ScanIntersectTool.h"
#include "ElementAttribute.h"
#include "HoleRebarAssembly.h"
#include "CHoleRebar_AddUnionHoleDlg.h"
#include "XmlHelper.h"
#include "PITMSCECommon.h"
#include "CHoleRebar_ReinforcingDlg.h"
#include "PITRebarAssembly.h"
#include "WallRebarAssembly.h"

// CHoleRebar_StructualDlg 对话框

extern bool g_closeDlg ;
IMPLEMENT_DYNAMIC(CHoleRebar_StructualDlg, CDialogEx)

CHoleRebar_StructualDlg::CHoleRebar_StructualDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_HoleRebar_Structural, pParent)
{
	isArcwall = false;
	crosswise = 0;
	urebar = 0;
}

CHoleRebar_StructualDlg::~CHoleRebar_StructualDlg()
{
}

void CHoleRebar_StructualDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

void CHoleRebar_StructualDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_HoleStructual, m_list_holeStructual);
	DDX_Control(pDX, IDC_CHECK_STRUCTUALH3, m_check_Structualh3);
	DDX_Control(pDX, IDC_CHECK_STRUCTUALH4, m_check_structualh4);
	DDX_Control(pDX, IDC_CHECK_STRUCTUALV1, m_check_structualv1);
	DDX_Control(pDX, IDC_CHECK_STRUCTUALV2, m_check_structualv2);
	DDX_Control(pDX, IDC_CHECK_STRUCTUALTWINV1, m_check_structualtwinv1);
	DDX_Control(pDX, IDC_CHECK_STRUCTUALTWINV2, m_check_structualtwinv2);
	DDX_Control(pDX, IDC_CHECK_STRUCTUALTWINH3, m_check_structualtwinh3);
	DDX_Control(pDX, IDC_CHECK_STRUCTUALTWINH4, m_check_structualtwinh4);
	DDX_Control(pDX, IDC_CHECK_STRSELECTALL, m_check_seclectall);
	DDX_Control(pDX, IDC_COMBO1, m_diameter);
	DDX_Control(pDX, IDC_COMBO2, m_type);
	DDX_Control(pDX, IDC_CHECK1, m_crosswise);
	DDX_Control(pDX, IDC_CHECK2, m_urebar);
}


void CHoleRebar_StructualDlg::InitUIData()
{

	// TODO:  在此添加额外的初始化
	LONG lStyle;
	lStyle = GetWindowLong(m_list_holeStructual.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_list_holeStructual.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_list_holeStructual.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_list_holeStructual.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_list_holeStructual.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_list_holeStructual.InsertColumn(0, _T("孔洞名称"), (int)(width / 2.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_list_holeStructual.InsertColumn(1, _T("孔洞大小"), (int)(width / 2.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);

	//	vector<WallRebarInfo::AssociatedComponent> vecACData;
	SetListDefaultData();
	ExTractHoleDatas();
	UpdateACList();
	for each (auto var in g_listRebarSize)
		m_diameter.AddString(var);
	m_diameter.SetCurSel(0);
	OnCbnSelchangeCombo1();
	for each (auto va in g_listRebarType)
		m_type.AddString(va);
	m_type.SetCurSel(0);
	OnCbnSelchangeCombo2();
	m_vecRebarData.insert(m_vecRebarData.begin(), g_vecRebarData.begin(), g_vecRebarData.end());
	g_ConcreteId = m_ConcreteId;
	m_HoleRebar = nullptr;
	m_ArcHoleRebar = nullptr;
}

void CHoleRebar_StructualDlg::SetListDefaultData()
{
	m_list_holeStructual.m_Dlg = this;
	m_nowHolename = "";
	m_nowHoleNum = 0;
	ScanWallAndFloorHoles(m_holeidAndmodel, m_ehSel);
	m_list_holeStructual.m_holeidAndmodel.insert(m_holeidAndmodel.begin(), m_holeidAndmodel.end());
	ElementId unionid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), unionid, UnionWallIDXAttribute, m_ehSel.GetModelRef());
	ElementId testid = 0;
	if (unionid!=0)
	{
		EditElementHandle eeh(unionid, ACTIVEMODEL);
		GetElementXAttribute(unionid, sizeof(ElementId), testid, ConcreteIDXAttribute, ACTIVEMODEL);
	}
	if (testid==0)
	{
		GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	}
	
	GetElementXAttribute(testid, m_RebarPts, vecRebarPointsXAttribute, ACTIVEMODEL);
	GetElementXAttribute(testid, m_FrontPts, FrontPtsXAttribute, ACTIVEMODEL);
	GetElementXAttribute(testid, m_TwinRebarPts, vecTwinRebarPointsXAttribute, ACTIVEMODEL);
	GetElementXAttribute(testid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
	GetConcreteXAttribute(testid, ACTIVEMODEL);
	//GetElementXAttribute(testid, sizeof(Transform), m_trans, UcsMatrixXAttribute, ACTIVEMODEL);
	//m_trans.InverseOf(m_trans);

	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ACCConcrete), m_acconcrete, ConcreteCoverXAttribute, m_ehSel.GetModelRef());

	/*if (tmpPts.size()>1)
	{
		for (int i = 0; i < tmpPts.size() - 1; i++)
		{
			int j = i + 1;
			DPoint3d ptstr = tmpPts.at(i).pt;
			DPoint3d ptend = tmpPts.at(j).pt;
			if (tmpPts.at(i).Layer == tmpPts.at(j).Layer)
			{
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
				eeh.AddToModel();
			}
			i++;
		}
	}*/

}

void CHoleRebar_StructualDlg::UpdateACList()
{
	int nItemNum = 0;
	m_list_holeStructual.DeleteAllItems();
	for (int j = 0; j < m_vecReinF.size(); ++j)
	{
		/*if (m_vecReinF[j].isUnionChild == true)
		{
			continue;
		}*/
		m_list_holeStructual.InsertItem(nItemNum, _T("")); // 插入行
		m_list_holeStructual.SetItemText(nItemNum, 0, CString(m_vecReinF[j].Hname));

		m_ListIndexAndName.insert(make_pair(std::string(m_vecReinF[j].Hname), nItemNum));

		for (int k = 1; k < 2; ++k)
		{
			CString strValue;
			switch (k)
			{
			case 1:
				strValue.Format(_T("%f"), m_vecReinF[j].Hsize);
				break;
			default:
				break;
			}
			m_list_holeStructual.SetItemText(nItemNum, k, strValue);
		}
		nItemNum++;
	}
	//m_list_holeStructual.SetShowProgressPercent(TRUE);
	//m_list_holeStructual.SetSupportSort(TRUE);
}



// CWallRebarAssociatedComponentDlg 消息处理程序


BOOL CHoleRebar_StructualDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	InitUIData();
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}



void CHoleRebar_StructualDlg::OnDestroy()
{
	m_list_holeStructual.DeleteElements();
	CDialogEx::OnDestroy();
	// TODO: 在此处添加消息处理程序代码
}


void CHoleRebar_StructualDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	clearSelectHoles();
	m_list_holeStructual.DeleteElements();
	CDialogEx::OnClose();
}
void CHoleRebar_StructualDlg::InitStructualInfoData(HoleRebarInfo::ReinForcingInfo& refdata)
{
	refdata.v1 = 0;
	refdata.v2 = 0;
	refdata.h3 = 0;
	refdata.h4 = 0;

	refdata.have_twinv1 = 0;
	refdata.have_twinv2 = 0;
	refdata.have_twinh3 = 0;
	refdata.have_twinh4 = 0;

	refdata.isUnionChild = false;
	refdata.isUnionHole = false;
	memset(refdata.Uname, 0, 256);

}
void CHoleRebar_StructualDlg::UpdateHoleDataView(string holename)
{
	m_nowHolename = holename;
	for (HoleRebarInfo::ReinForcingInfo testInfo : m_vecReinF)
	{
		string tHolename(testInfo.Hname);
		if (tHolename != holename)
		{
			continue;
		}
		m_check_structualv1.SetCheck(testInfo.v1);
		m_check_structualv2.SetCheck(testInfo.v2);
		m_check_Structualh3.SetCheck(testInfo.h3);
		m_check_structualh4.SetCheck(testInfo.h4);

		if (testInfo.v1&&testInfo.v2&&testInfo.h3&&testInfo.h4)
		{
			m_check_seclectall.SetCheck(true);
		}

		m_check_structualtwinv1.EnableWindow(testInfo.v1);
		m_check_structualtwinv2.EnableWindow(testInfo.v2);
		m_check_structualtwinh3.EnableWindow(testInfo.h3);
		m_check_structualtwinh4.EnableWindow(testInfo.h4);

		m_check_structualtwinv1.SetCheck(testInfo.have_twinv1);
		m_check_structualtwinv2.SetCheck(testInfo.have_twinv2);
		m_check_structualtwinh3.SetCheck(testInfo.have_twinh3);
		m_check_structualtwinh4.SetCheck(testInfo.have_twinh4);
	}


}
void CHoleRebar_StructualDlg::GetNowHoleNum()
{

	for (int i = 0; i < m_vecReinF.size(); i++)
	{
		string tHolename(m_vecReinF.at(i).Hname);
		if (tHolename != m_nowHolename)
		{
			continue;
		}
		m_nowHoleNum = i;
		break;
	}


}
void CHoleRebar_StructualDlg::ExTractHoleDatas()
{
	ElementId testid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	
	GetElementXAttribute(testid, m_vecReinF, ReinForcingInfoXAttribute, ACTIVEMODEL);
	//SetElementXAttribute(testid, m_vecReinF, ReinForcingInfoXAttribute, ACTIVEMODEL);
	GetConcreteXAttribute(testid, ACTIVEMODEL);
	if (m_vecReinF.size() < 1)
	{
		std::map<std::string, IDandModelref>::iterator itr = m_holeidAndmodel.begin();
		if (isArcwall==false)//非弧形墙处理
		{
			for (; itr != m_holeidAndmodel.end(); itr++)
			{
				HoleRebarInfo::ReinForcingInfo testInfo;
				EditElementHandle eeh(itr->second.ID, itr->second.tModel);

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
				double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
				CalculateHoleTransByFrontPoints(eeh, m_FrontPts, m_trans, isFloor);
				TransformInfo transinfo(m_trans);
				eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
				DPoint3d minP;
				DPoint3d maxP;
				//计算指定元素描述符中元素的范围。
				mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
				DRange3d       vecRange;
				vecRange.low = minP;
				vecRange.high = maxP;
				testInfo.Hsize = (vecRange.ZLength() > vecRange.XLength()) ? vecRange.ZLength() : vecRange.XLength();
				testInfo.Hsize = testInfo.Hsize / uor_per_mm;

				size_t tpos = itr->first.find('@');
				if (tpos == string::npos)
				{
					tpos = itr->first.size();
				}
				itr->first.copy(testInfo.Hname, tpos, 0);
				InitStructualInfoData(testInfo);
				/*if (testInfo.Hsize > 200)
				{
					testInfo.v1 = 1;
					testInfo.h3 = 1;
					testInfo.v2 = 1;
					testInfo.h4 = 1;
				}*/
				m_vecReinF.push_back(testInfo);


			}
		}
		else
		{
			for (; itr != m_holeidAndmodel.end(); itr++)
			{
				HoleRebarInfo::ReinForcingInfo testInfo;
				EditElementHandle eeh(itr->second.ID, itr->second.tModel);

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
				double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
				DPoint3d ptcenter;
				if (!CHoleRebar_ReinforcingDlg::GetArcCenterPoint(m_RebarPts, ptcenter, &eeh))
				{
					continue;
				}
				DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());

				ptcenter.z = ptele.z;
				CVector3D yVec = ptcenter - ptele;
				yVec.Normalize();

				CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

				DPoint3d ptStart = ptcenter;
				BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴

				placement.AssignTo(m_trans);
				m_trans.InverseOf(m_trans);

				TransformInfo transinfo(m_trans);
				eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
				DPoint3d minP;
				DPoint3d maxP;
				//计算指定元素描述符中元素的范围。
				mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
				DRange3d       vecRange;
				vecRange.low = minP;
				vecRange.high = maxP;
				testInfo.Hsize = (vecRange.ZLength() > vecRange.XLength()) ? vecRange.ZLength() : vecRange.XLength();
				testInfo.Hsize = testInfo.Hsize / uor_per_mm;

				size_t tpos = itr->first.find('@');
				if (tpos == string::npos)
				{
					tpos = itr->first.size();
				}
				itr->first.copy(testInfo.Hname, tpos, 0);
				InitStructualInfoData(testInfo);				
				m_vecReinF.push_back(testInfo);


			}
		}
		
	}


}

// HoleRebar_Reinforcing 消息处理程序
void CHoleRebar_StructualDlg::RefreshStructualRebars(ElementId conid, EditElementHandleR eeh)
{
	PIT::PITRebarAssembly::DeleteRebarsFromAssembly(conid, "class HoleSTRebarAssembly");
	if (RebarElement::IsRebarElement(eeh))
	{
		return;
	}
	WallRebarAssembly::ElementType eleType = WallRebarAssembly::JudgeElementType(eeh);
	bool isarcwall = false;
	bool isfloor = false;
	if (WallRebarAssembly::JudgeElementType(eeh) != WallRebarAssembly::FLOOR)
	{

		WallRebarAssembly::WallType wallType = WallRebarAssembly::JudgeWallType(eeh);
		if (wallType == WallRebarAssembly::ARCWALL)
		{
			isarcwall = true;
		}
	}
	else
	{
		isfloor = true;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CHoleRebar_StructualDlg* pHoleDlgtmp = NULL;
	pHoleDlgtmp = new CHoleRebar_StructualDlg(CWnd::FromHandle(MSWIND));
	pHoleDlgtmp->isArcwall = isarcwall;
	pHoleDlgtmp->isFloor = isfloor;
	pHoleDlgtmp->SetSelectElement(eeh);
	pHoleDlgtmp->Create(IDD_DIALOG_HoleRebar_Structural);
	pHoleDlgtmp->OnBnClickedOk();
	//delete pHoleDlgtmp;
}

void CHoleRebar_StructualDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	vector<int>producerebar;
	producerebar.push_back(crosswise);
	producerebar.push_back(urebar);
	m_list_holeStructual.DeleteElements();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = m_ehSel.GetModelRef();
	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());
	m_list_holeStructual.GetAllRebarData(m_vecReinF);
	HoleRFRebarAssembly::IsSmartSmartFeature(eeh);
	/***********************************给sizekey附加型号******************************************************/
	/*auto it = g_vecRebarData.begin();
	for (; it != g_vecRebarData.end(); it++)
	{
		BrString strRebarSize = it->rebarSize;
		if (strRebarSize != L"")
		{
			if (strRebarSize.Find(L"mm") != -1)
			{
				strRebarSize.Replace(L"mm", L"");
			}
		}
		else
		{
			strRebarSize = XmlManager::s_alltypes[it->rebarType];
		}
		strcpy(it->rebarSize, CT2A(strRebarSize));
		GetDiameterAddType(it->rebarSize, it->rebarType);
	}*/
	auto it = m_vecRebarData.begin();
	for (; it != m_vecRebarData.end(); it++)
	{
		BrString strRebarSize = it->rebarSize;
		if (strRebarSize != L"")
		{
			if (strRebarSize.Find(L"mm") != -1)
			{
				strRebarSize.Replace(L"mm", L"");
			}
		}
		else
		{
			strRebarSize = XmlManager::s_alltypes[2];
		}
		strcpy(it->rebarSize, CT2A(strRebarSize));
		it->rebarType = 2;//HBR400等级
		GetDiameterAddType(it->rebarSize, it->rebarType);
	}
	/***********************************给sizekey附加型号******************************************************/
	if (m_vecReinF.size()>0)
	{
		if (!isArcwall)
		{
			std::vector<EditElementHandle*> Holeehs;
			vector<EditElementHandle*> useHoleehs;
			CHoleRebar_ReinforcingDlg::GetUseHoles(eeh, m_FrontPts, Holeehs, useHoleehs, modelRef, isFloor);
			/*	if (m_HoleRebar != nullptr)
				{
					ElementId testid = 0;
					GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
					RebarAssembly* rebaras = HoleRFRebarAssembly::GetRebarAssembly(testid, "class HoleSTRebarAssembly");
					m_HoleRebar = dynamic_cast<HoleSTRebarAssembly*>(rebaras);
					if (m_HoleRebar == nullptr)
					{
						m_HoleRebar = REA::Create<HoleSTRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
					}
				}*/
			if (m_HoleRebar == nullptr)
			{
				ElementId testid = 0;
				GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
				RebarAssembly* rebaras = HoleRFRebarAssembly::GetRebarAssembly(testid, "class HoleSTRebarAssembly");
				m_HoleRebar = dynamic_cast<HoleSTRebarAssembly*>(rebaras);
				if (m_HoleRebar == nullptr)
				{
					m_HoleRebar = REA::Create<HoleSTRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
					m_HoleRebar->SetConcreteOwner(testid);
				}
			}
			if (m_HoleRebar == nullptr)
			{
				m_HoleRebar = REA::Create<HoleSTRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
			m_HoleRebar->ClearData();
			m_HoleRebar->SetTrans(m_trans);
			m_HoleRebar->SetvecFrontPts(m_FrontPts);
			m_HoleRebar->SetaccConcrete(m_acconcrete);
			//m_HoleRebar->SetVecDirSizeData(g_vecRebarData);
			m_HoleRebar->SetVecDirSizeData(m_vecRebarData);
			m_HoleRebar->SetrebarPts(m_RebarPts);
			m_HoleRebar->Settypenum(2);//HBR400等级
			m_HoleRebar->Setproduce(producerebar);//HBR400等级
			m_HoleRebar->SettwinrebarPts(m_TwinRebarPts);
			m_HoleRebar->SetvecReinF(m_vecReinF);
			m_HoleRebar->SetSelectedElement(m_ehSel.GetElementId());
			m_HoleRebar->SetSelectedModel(m_ehSel.GetModelRef());
			m_HoleRebar->isfloor = isFloor;
			m_HoleRebar->m_useHoleehs.clear();
			m_HoleRebar->m_useHoleehs.insert(m_HoleRebar->m_useHoleehs.begin(), useHoleehs.begin(), useHoleehs.end());
			m_HoleRebar->m_vecTwinBarData = m_vecTwinBarData;
			m_HoleRebar->m_holeidAndmodel.insert(m_holeidAndmodel.begin(), m_holeidAndmodel.end());
			m_HoleRebar->m_strElmName = m_nowHolename;
			m_HoleRebar->MakeRebars(modelRef);
			m_HoleRebar->Save(modelRef); // must save after creating rebars
			ElementId contid = m_HoleRebar->FetchConcrete();
			EditElementHandle eeh2(contid, ACTIVEMODEL);
			ElementRefP oldRef = eeh2.GetElementRef();
			mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
			eeh2.ReplaceInModel(oldRef);
			SetElementXAttribute(m_HoleRebar->FetchConcrete(), m_vecReinF, StructualInfoXAttribute, ACTIVEMODEL);
			FreeAll(Holeehs);
		}
		else
		{
			std::vector<EditElementHandle*> Holeehs;
			vector<EditElementHandle*> useHoleehs;
			CHoleRebar_ReinforcingDlg::GetARCUseHoles(eeh, Holeehs, useHoleehs, modelRef);
			if (m_ArcHoleRebar == nullptr)
			{
				ElementId testid = 0;
				GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
				RebarAssembly* rebaras = HoleRFRebarAssembly::GetRebarAssembly(testid, "class HoleArcSTRebarAssembly");
				m_ArcHoleRebar = dynamic_cast<HoleArcSTRebarAssembly*>(rebaras);
				if (m_ArcHoleRebar == nullptr)
				{
					m_ArcHoleRebar = REA::Create<HoleArcSTRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
					m_ArcHoleRebar->SetConcreteOwner(testid);
				}
			}
			if (m_ArcHoleRebar == nullptr)
			{
				m_ArcHoleRebar = REA::Create<HoleArcSTRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
			m_ArcHoleRebar->ClearData();
			m_ArcHoleRebar->SetTrans(m_trans);
			m_ArcHoleRebar->SetvecFrontPts(m_FrontPts);
			m_ArcHoleRebar->SetaccConcrete(m_acconcrete);
			m_ArcHoleRebar->Settypenum(2);//HBR400等级
			m_ArcHoleRebar->Setproducerebar(producerebar); 
			//m_ArcHoleRebar->SetVecDirSizeData(g_vecRebarData);
			m_ArcHoleRebar->SetVecDirSizeData(m_vecRebarData);
			m_ArcHoleRebar->SetrebarPts(m_RebarPts);
			m_ArcHoleRebar->SettwinrebarPts(m_TwinRebarPts);
			m_ArcHoleRebar->SetvecReinF(m_vecReinF);
			m_ArcHoleRebar->m_useHoleehs.clear();
			m_ArcHoleRebar->m_useHoleehs.insert(m_ArcHoleRebar->m_useHoleehs.begin(), useHoleehs.begin(), useHoleehs.end());
			m_ArcHoleRebar->m_vecTwinBarData = m_vecTwinBarData;
			m_ArcHoleRebar->m_strElmName = m_nowHolename;
			m_ArcHoleRebar->m_holeidAndmodel.insert(m_holeidAndmodel.begin(), m_holeidAndmodel.end());
			m_ArcHoleRebar->MakeRebars(modelRef);
			m_ArcHoleRebar->Save(modelRef); // must save after creating rebars
		
			ElementId contid = m_ArcHoleRebar->FetchConcrete();
			EditElementHandle eeh2(contid, ACTIVEMODEL);
			ElementRefP oldRef = eeh2.GetElementRef();
			mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
			eeh2.ReplaceInModel(oldRef);
			SetElementXAttribute(m_ArcHoleRebar->FetchConcrete(), m_vecReinF, StructualInfoXAttribute, ACTIVEMODEL);
			FreeAll(Holeehs);
		}
	}
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	//auto it2 = g_vecRebarData.begin();
	//for (; it2 != g_vecRebarData.end(); it2++)
	//{
	//	BrString strRebarSize = it2->rebarSize;
	//	strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
	//	strcpy(it2->rebarSize, CT2A(strRebarSize));
	//}
	auto it2 = m_vecRebarData.begin();
	for (; it2 != m_vecRebarData.end(); it2++)
	{
		BrString strRebarSize = it2->rebarSize;
		strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
		strcpy(it2->rebarSize, CT2A(strRebarSize));
	}
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	clearSelectHoles();
	g_closeDlg = true;
	DestroyWindow();
}
void CHoleRebar_StructualDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	m_list_holeStructual.DeleteElements();
	clearSelectHoles();
	DestroyWindow();
}

BEGIN_MESSAGE_MAP(CHoleRebar_StructualDlg, CDialogEx)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDOK, &CHoleRebar_StructualDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CHoleRebar_StructualDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_CHECK_STRUCTUALV1, &CHoleRebar_StructualDlg::OnBnClickedCheckStructualv1)
	ON_BN_CLICKED(IDC_CHECK_STRUCTUALV2, &CHoleRebar_StructualDlg::OnBnClickedCheckStructualv2)
	ON_BN_CLICKED(IDC_CHECK_STRUCTUALH3, &CHoleRebar_StructualDlg::OnBnClickedCheckStructualh3)
	ON_BN_CLICKED(IDC_CHECK_STRUCTUALH4, &CHoleRebar_StructualDlg::OnBnClickedCheckStructualh4)
	ON_BN_CLICKED(IDC_CHECK_STRUCTUALTWINV1, &CHoleRebar_StructualDlg::OnBnClickedCheckStructualtwinv1)
	ON_BN_CLICKED(IDC_CHECK_STRUCTUALTWINV2, &CHoleRebar_StructualDlg::OnBnClickedCheckStructualtwinv2)
	ON_BN_CLICKED(IDC_CHECK_STRUCTUALTWINH3, &CHoleRebar_StructualDlg::OnBnClickedCheckStructualtwinh3)
	ON_BN_CLICKED(IDC_CHECK_STRUCTUALTWINH4, &CHoleRebar_StructualDlg::OnBnClickedCheckStructualtwinh4)
	ON_BN_CLICKED(IDC_SET_UNIONHOLEST_BUTTON, &CHoleRebar_StructualDlg::OnBnClickedSetUnionholestButton)
	ON_BN_CLICKED(IDC_BUTTON_DISSUNIONST, &CHoleRebar_StructualDlg::OnBnClickedButtonDissunionst)
	ON_BN_CLICKED(IDC_CHECK_STRSELECTALL, &CHoleRebar_StructualDlg::OnBnClickedCheckStrselectall)
	ON_BN_CLICKED(IDC_BUTTON1, &CHoleRebar_StructualDlg::OnBnClickedButton1)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CHoleRebar_StructualDlg::OnCbnSelchangeCombo1)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CHoleRebar_StructualDlg::OnCbnSelchangeCombo2)
	ON_BN_CLICKED(IDC_CHECK1, &CHoleRebar_StructualDlg::OnBnClickedCrosswiseRebar)
	ON_BN_CLICKED(IDC_CHECK2, &CHoleRebar_StructualDlg::OnBnClickedURebar)
END_MESSAGE_MAP()


// CHoleRebar_StructualDlg 消息处理程序


void CHoleRebar_StructualDlg::OnBnClickedCheckStructualv1()
{
	// TODO: 在此添加控件通知处理程序代码
	m_vecReinF[m_nowHoleNum].v1 = m_check_structualv1.GetCheck();
	if (m_check_structualv1.GetCheck())
	{
		m_check_structualtwinv1.EnableWindow(TRUE);
		
	}
	else
	{
		m_vecReinF[m_nowHoleNum].have_twinv1 = 0;
		m_check_structualtwinv1.SetCheck(FALSE);
		m_check_structualtwinv1.EnableWindow(FALSE);
	}
}


void CHoleRebar_StructualDlg::OnBnClickedCheckStructualv2()
{
	// TODO: 在此添加控件通知处理程序代码
	m_vecReinF[m_nowHoleNum].v2 = m_check_structualv2.GetCheck();
	if (m_check_structualv2.GetCheck())
	{
		m_check_structualtwinv2.EnableWindow(TRUE);

	}
	else
	{
		m_vecReinF[m_nowHoleNum].have_twinv2 = 0;
		m_check_structualtwinv2.SetCheck(FALSE);
		m_check_structualtwinv2.EnableWindow(FALSE);
	}
}


void CHoleRebar_StructualDlg::OnBnClickedCheckStructualh3()
{
	// TODO: 在此添加控件通知处理程序代码
	m_vecReinF[m_nowHoleNum].h3 = m_check_Structualh3.GetCheck();
	if (m_check_Structualh3.GetCheck())
	{
		m_check_structualtwinh3.EnableWindow(TRUE);

	}
	else
	{
		m_vecReinF[m_nowHoleNum].have_twinh3 = 0;
		m_check_structualtwinh3.SetCheck(FALSE);
		m_check_structualtwinh3.EnableWindow(FALSE);
	}
}


void CHoleRebar_StructualDlg::OnBnClickedCheckStructualh4()
{
	// TODO: 在此添加控件通知处理程序代码
	m_vecReinF[m_nowHoleNum].h4 = m_check_structualh4.GetCheck();
	if (m_check_structualh4.GetCheck())
	{
		m_check_structualtwinh4.EnableWindow(TRUE);

	}
	else
	{
		m_vecReinF[m_nowHoleNum].have_twinh4 = 0;
		m_check_structualtwinh4.SetCheck(FALSE);
		m_check_structualtwinh4.EnableWindow(FALSE);
	}
}


void CHoleRebar_StructualDlg::OnBnClickedCheckStructualtwinv1()
{
	// TODO: 在此添加控件通知处理程序代码
	m_vecReinF[m_nowHoleNum].have_twinv1 = m_check_structualtwinv1.GetCheck();
}


void CHoleRebar_StructualDlg::OnBnClickedCheckStructualtwinv2()
{
	// TODO: 在此添加控件通知处理程序代码
	m_vecReinF[m_nowHoleNum].have_twinv2 = m_check_structualtwinv2.GetCheck();
}


void CHoleRebar_StructualDlg::OnBnClickedCheckStructualtwinh3()
{
	// TODO: 在此添加控件通知处理程序代码
	m_vecReinF[m_nowHoleNum].have_twinh3 = m_check_structualtwinh3.GetCheck();
}


void CHoleRebar_StructualDlg::OnBnClickedCheckStructualtwinh4()
{
	// TODO: 在此添加控件通知处理程序代码
	m_vecReinF[m_nowHoleNum].have_twinh4 = m_check_structualtwinh4.GetCheck();
}


void CHoleRebar_StructualDlg::OnBnClickedSetUnionholestButton()
{
	
	m_list_holeStructual.DeleteElements();
	// TODO: 在此添加控件通知处理程序代码
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CHoleRebar_AddUnionHoleDlg* adduniondlg = new CHoleRebar_AddUnionHoleDlg();
	adduniondlg->m_Structualdlg = this;
	adduniondlg->m_trans = m_trans;
	adduniondlg->m_FrontPts = m_FrontPts;
	adduniondlg->m_RebarPts = m_RebarPts;
	adduniondlg->m_type = CHoleRebar_AddUnionHoleDlg::Structual;
	adduniondlg->SetHoleData(m_holeidAndmodel);
	adduniondlg->SetListRowData(m_vecReinF);
	adduniondlg->Create(IDD_DIALOG_ADDUNIONHOLE, this);
	adduniondlg->ShowWindow(SW_SHOW);


}


void CHoleRebar_StructualDlg::OnBnClickedButtonDissunionst()
{
	// TODO: 在此添加控件通知处理程序代码
	HoleRebarInfo::ReinForcingInfo tmpInfo = m_vecReinF[m_nowHoleNum];
	if (tmpInfo.isUnionHole)
	{
		std::string unionName(tmpInfo.Hname);
		for (HoleRebarInfo::ReinForcingInfo& tinfo : m_vecReinF)
		{
			if (tinfo.isUnionChild)
			{
				std::string tuname(tinfo.Uname);
				if (tuname == unionName)
				{
					tinfo.isUnionChild = false;
					memset(tinfo.Uname, 0, 256);
				}
			}

		}
		m_vecReinF.erase(m_vecReinF.begin() + m_nowHoleNum);
	}

	UpdateACList();

	m_list_holeStructual.DeleteElements();
}


void CHoleRebar_StructualDlg::OnBnClickedCheckStrselectall()
{
	// TODO: 在此添加控件通知处理程序代码
	m_check_structualv1.SetCheck(m_check_seclectall.GetCheck());
	m_check_structualv2.SetCheck(m_check_seclectall.GetCheck());
	m_check_Structualh3.SetCheck(m_check_seclectall.GetCheck());
	m_check_structualh4.SetCheck(m_check_seclectall.GetCheck());
	m_vecReinF[m_nowHoleNum].v1 = m_check_seclectall.GetCheck();
	m_vecReinF[m_nowHoleNum].v2 = m_check_seclectall.GetCheck();
	m_vecReinF[m_nowHoleNum].h3 = m_check_seclectall.GetCheck();
	m_vecReinF[m_nowHoleNum].h4 = m_check_seclectall.GetCheck();
	if (m_check_seclectall.GetCheck())
	{
		m_check_structualtwinv1.EnableWindow(TRUE);
		m_check_structualtwinv2.EnableWindow(TRUE);
		m_check_structualtwinh3.EnableWindow(TRUE);
		m_check_structualtwinh4.EnableWindow(TRUE);
	}
	else
	{
		m_vecReinF[m_nowHoleNum].have_twinv1 = 0;
		m_vecReinF[m_nowHoleNum].have_twinv2 = 0;
		m_vecReinF[m_nowHoleNum].have_twinh3 = 0;
		m_vecReinF[m_nowHoleNum].have_twinh4 = 0;
		m_check_structualtwinv1.SetCheck(FALSE);
		m_check_structualtwinv2.SetCheck(FALSE);
		m_check_structualtwinh3.SetCheck(FALSE);
		m_check_structualtwinh4.SetCheck(FALSE);
		m_check_structualtwinv1.EnableWindow(FALSE);
		m_check_structualtwinv2.EnableWindow(FALSE);
		m_check_structualtwinh3.EnableWindow(FALSE);
		m_check_structualtwinh4.EnableWindow(FALSE);
	}
}

void CHoleRebar_StructualDlg::clearSelectHoles()
{
	for (ElementRefP tmpeeh : m_vctSelectHoles)
	{
		EditElementHandle eeh(tmpeeh, tmpeeh->GetDgnModelP());
		eeh.DeleteFromModel();
	}
	m_vctSelectHoles.clear();
}


void CHoleRebar_StructualDlg::OnBnClickedButton1()//点选孔洞，点选后，所有孔洞都用实体画出来
{
	clearSelectHoles();
	m_NewHoleElements.clear();
	for (auto it = m_holeidAndmodel.begin(); it != m_holeidAndmodel.end();it++)
	{
		EditElementHandle eeh(it->second.ID, it->second.tModel);
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
			ElementCopyContext copier2(ACTIVEMODEL);
			copier2.SetSourceModelRef(eeh.GetModelRef());
			copier2.SetTransformToDestination(true);
			copier2.SetWriteElements(false);
			copier2.DoCopy(eeh);
			eeh.AddToModel();
			m_vctSelectHoles.push_back(eeh.GetElementRef());
			m_NewHoleElements.insert(make_pair(it->first, eeh.GetElementId()));
		}
	}
	mdlView_updateSingle(0);

	SelectHoleTools::InstallNewInstance(0, this);
}


void CHoleRebar_StructualDlg::GetSeclectElement(EditElementHandleR HoleEeh)
{
	ElementId HoleID = HoleEeh.GetElementId();
	string holename = "";

 	for (auto it = m_NewHoleElements.begin();it!= m_NewHoleElements.end();it++)
 	{
 		if (it->second == HoleID)
 		{
 			holename = it->first;
 			break;
 		}
 	}
	if (holename != "")
	{
		int index = m_ListIndexAndName[holename];
		m_list_holeStructual.SetFocus();
		m_list_holeStructual.SetItemState(index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_nowHolename = holename;
		GetNowHoleNum();
		UpdateHoleDataView(holename);
	}
}





void CHoleRebar_StructualDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0; //根据当前dgn模型获取比例因子
	int nIndex = m_diameter.GetCurSel();
	CString rebarDiaStr;
	m_diameter.GetLBText(nIndex, rebarDiaStr);
	m_rebarDia = rebarDiaStr;
	/*if (rebarDiaStr.Find(_T("mm")) != -1)
	{
		rebarDiaStr.Replace(_T("mm"), _T(""));
	}
	m_rebarDia = _ttof(rebarDiaStr) * uor_per_mm;*/
}


void CHoleRebar_StructualDlg::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0; //根据当前dgn模型获取比例因子
	int nIndex = m_type.GetCurSel();
	Typenum = nIndex;
	CString rebarDiaStr;
	m_type.GetLBText(nIndex, rebarDiaStr);
}


void CHoleRebar_StructualDlg::OnBnClickedCrosswiseRebar()
{
	// TODO: 在此添加控件通知处理程序代码
	crosswise=m_crosswise.GetCheck();
	
}


void CHoleRebar_StructualDlg::OnBnClickedURebar()
{
	// TODO: 在此添加控件通知处理程序代码
	urebar = m_urebar.GetCheck();
	
}
