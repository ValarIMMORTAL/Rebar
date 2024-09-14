// HoleRebar_Reinforcing.cpp: 实现文件
//

#include "_USTATION.h"
#include "resource.h"
#include "CHoleRebar_ReinforcingDlg.h"
#include "CHoleRebarListCtrl.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"
#include "GalleryIntelligentRebarids.h"
#include "ScanIntersectTool.h"
#include "ElementAttribute.h"
#include "HoleRebarAssembly.h"
#include "XmlHelper.h"
#include "PITMSCECommon.h"
#include "WallRebarAssembly.h"
#include "PITRebarAssembly.h"
// HoleRebar_Reinforcing 对话框

extern bool g_closeDlg;
IMPLEMENT_DYNAMIC(CHoleRebar_ReinforcingDlg, CDialogEx)

CHoleRebar_ReinforcingDlg::CHoleRebar_ReinforcingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_HoleRebar_Reinforcing, pParent)
{
	isArcwall = false;

}

CHoleRebar_ReinforcingDlg::~CHoleRebar_ReinforcingDlg()
{
}

void CHoleRebar_ReinforcingDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

void CHoleRebar_ReinforcingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_HoleReinforcing, m_list_holeReinforcing);
	DDX_Control(pDX, IDC_CHECK_REINV1, m_Check_reinv1);
	DDX_Control(pDX, IDC_CHECK_REINV2, m_Check_revinv2);
	DDX_Control(pDX, IDC_CHECK_REINH3, m_Check_revinh3);
	DDX_Control(pDX, IDC_CHECK_REINH4, m_Check_revinh4);
	DDX_Control(pDX, IDC_EDIT_REINNUMV1, m_Edit_revinnumv1);
	DDX_Control(pDX, IDC_EDIT_REINNUMV2, m_Edit_revinnumv2);
	DDX_Control(pDX, IDC_EDIT_REINNUMH3, m_Edit_revinnumh3);
	DDX_Control(pDX, IDC_EDIT_REINNUMH4, m_Edit_revinnumh4);
	DDX_Control(pDX, IDC_EDIT_REINSPACINGV1, m_Edit_reinspacingv1);
	DDX_Control(pDX, IDC_EDIT_REINSPACINGV2, m_Edit_reinspacingv2);
	DDX_Control(pDX, IDC_EDIT_REINSPACINGH3, m_Edit_reinspacingh3);
	DDX_Control(pDX, IDC_EDIT_REINSPACINGH4, m_Edit_reinspacingh4);
	DDX_Control(pDX, IDC_CHECK_REINSECLECTALL, m_check_selectall);
	DDX_Control(pDX, IDC_COMBO1, m_diameter);
	DDX_Control(pDX, IDC_COMBO2, m_type);
}


void CHoleRebar_ReinforcingDlg::InitUIData()
{

	// TODO:  在此添加额外的初始化
	LONG lStyle;
	lStyle = GetWindowLong(m_list_holeReinforcing.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_list_holeReinforcing.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_list_holeReinforcing.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_list_holeReinforcing.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_list_holeReinforcing.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_list_holeReinforcing.InsertColumn(0, _T("孔洞名称"), (int)(width / 3.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_list_holeReinforcing.InsertColumn(1, _T("孔洞大小"), (int)(width / 3.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_list_holeReinforcing.InsertColumn(2, _T("偏离主筋距离"), (int)(width / 3.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

	//	vector<PIT::AssociatedComponent> vecACData;
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
bool CHoleRebar_ReinforcingDlg::GetArcCenterPoint(std::vector<RebarPoint>& RebarPts, DPoint3d& ptCenter, EditElementHandle* holedescr)
{
	vector<EditElementHandle*> allHoles;
	allHoles.push_back(holedescr);
	if (holedescr == nullptr)
	{
		return false;
	}
	for (RebarPoint pt : RebarPts)
	{
		if (pt.vecDir == 0)
		{
			vector<DPoint3d> interPoints;
			GetARCIntersectPointsWithHoles(interPoints, allHoles, pt.ptstr, pt.ptend, pt.ptmid);
			if (interPoints.size() == 2)
			{
				ptCenter = interPoints[0];
				ptCenter.Add(interPoints[1]);
				ptCenter.Scale(0.5);
				return true;
			}
		}
	}
	return false;
}
void CHoleRebar_ReinforcingDlg::SetListDefaultData()
{
	m_list_holeReinforcing.m_Dlg = this;
	m_nowHolename = "";
	m_nowHoleNum = 0;
	ScanWallAndFloorHoles(m_holeidAndmodel, m_ehSel);
	m_list_holeReinforcing.m_holeidAndmodel.insert(m_holeidAndmodel.begin(), m_holeidAndmodel.end());
	ElementId unionid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), unionid, UnionWallIDXAttribute, m_ehSel.GetModelRef());
	ElementId testid = 0;
	if (unionid != 0)
	{
		EditElementHandle eeh(unionid, ACTIVEMODEL);
		GetElementXAttribute(unionid, sizeof(ElementId), testid, ConcreteIDXAttribute, ACTIVEMODEL);
	}
	if (testid == 0)
	{
		GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	}
	GetElementXAttribute(testid, m_RebarPts, vecRebarPointsXAttribute, ACTIVEMODEL);
	GetConcreteXAttribute(testid, ACTIVEMODEL);

	GetElementXAttribute(testid, m_FrontPts, FrontPtsXAttribute, ACTIVEMODEL);
	GetElementXAttribute(testid, sizeof(Transform), m_trans, UcsMatrixXAttribute, ACTIVEMODEL);
	m_trans.InverseOf(m_trans);

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

void CHoleRebar_ReinforcingDlg::UpdateACList()
{
	int nItemNum = 0;
	m_list_holeReinforcing.DeleteAllItems();
	for (int j = 0; j < m_vecReinF.size(); ++j)
	{
		/*if (m_vecReinF[j].isUnionChild==true)
		{
			continue;
		}*/
		m_list_holeReinforcing.InsertItem(nItemNum, _T("")); // 插入行
		m_list_holeReinforcing.SetItemText(nItemNum, 0, CString(m_vecReinF[j].Hname));

		m_ListIndexAndName.insert(make_pair(std::string(m_vecReinF[j].Hname), nItemNum));
		for (int k = 1; k < 3; ++k)
		{
			CString strValue;
			switch (k)
			{
			case 1:
				strValue.Format(_T("%f"), m_vecReinF[j].Hsize);
				break;
			case 2:
				strValue.Format(_T("%f"), m_vecReinF[j].MainRebarDis);
				break;
			default:
				break;
			}
			m_list_holeReinforcing.SetItemText(nItemNum, k, strValue);
		}
		nItemNum++;
	}
	//m_list_holeReinforcing.SetShowProgressPercent(TRUE);
	//m_list_holeReinforcing.SetSupportSort(TRUE);
}


BEGIN_MESSAGE_MAP(CHoleRebar_ReinforcingDlg, CDialogEx)
	ON_NOTIFY(HDN_ITEMCLICK, 0, &CHoleRebar_ReinforcingDlg::OnHdnItemclickListAc)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDOK, &CHoleRebar_ReinforcingDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHECK_REINV1, &CHoleRebar_ReinforcingDlg::OnBnClickedCheckReinv1)
	ON_BN_CLICKED(IDC_CHECK_REINV2, &CHoleRebar_ReinforcingDlg::OnBnClickedCheckReinv2)
	ON_BN_CLICKED(IDC_CHECK_REINH3, &CHoleRebar_ReinforcingDlg::OnBnClickedCheckReinh3)
	ON_BN_CLICKED(IDC_CHECK_REINH4, &CHoleRebar_ReinforcingDlg::OnBnClickedCheckReinh4)
	ON_EN_CHANGE(IDC_EDIT_REINNUMV1, &CHoleRebar_ReinforcingDlg::OnEnChangeEditReinnumv1)
	ON_EN_CHANGE(IDC_EDIT_REINNUMV2, &CHoleRebar_ReinforcingDlg::OnEnChangeEditReinnumv2)
	ON_EN_CHANGE(IDC_EDIT_REINNUMH3, &CHoleRebar_ReinforcingDlg::OnEnChangeEditReinnumh3)
	ON_EN_CHANGE(IDC_EDIT_REINNUMH4, &CHoleRebar_ReinforcingDlg::OnEnChangeEditReinnumh4)
	ON_EN_CHANGE(IDC_EDIT_REINSPACINGV1, &CHoleRebar_ReinforcingDlg::OnEnChangeEditReinspacingv1)
	ON_EN_CHANGE(IDC_EDIT_REINSPACINGV2, &CHoleRebar_ReinforcingDlg::OnEnChangeEditReinspacingv2)
	ON_EN_CHANGE(IDC_EDIT_REINSPACINGH3, &CHoleRebar_ReinforcingDlg::OnEnChangeEditReinspacingh3)
	ON_EN_CHANGE(IDC_EDIT_REINSPACINGH4, &CHoleRebar_ReinforcingDlg::OnEnChangeEditReinspacingh4)
	ON_BN_CLICKED(IDCANCEL, &CHoleRebar_ReinforcingDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_SET_UNIONHOLE_BUTTON, &CHoleRebar_ReinforcingDlg::OnBnClickedSetUnionholeButton)
	ON_BN_CLICKED(IDC_BUTTON_DISSUNION, &CHoleRebar_ReinforcingDlg::OnBnClickedButtonDissunion)
	//	ON_BN_CLICKED(IDC_CHECK1, &CHoleRebar_ReinforcingDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_CHECK_REINSECLECTALL, &CHoleRebar_ReinforcingDlg::OnBnClickedCheckReinseclectall)
	ON_BN_CLICKED(IDC_BUTTON1, &CHoleRebar_ReinforcingDlg::OnBnClickedButton1)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CHoleRebar_ReinforcingDlg::OnCbnSelchangeCombo1)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CHoleRebar_ReinforcingDlg::OnCbnSelchangeCombo2)
END_MESSAGE_MAP()


// CWallRebarAssociatedComponentDlg 消息处理程序


BOOL CHoleRebar_ReinforcingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	mdlSelect_freeAll();
	mdlView_updateSingle(0);
	InitUIData();
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CHoleRebar_ReinforcingDlg::OnHdnItemclickListAc(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	CString			strValidChars;
	strValidChars = m_list_holeReinforcing.GetItemText(phdr->iItem, 1);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CHoleRebar_ReinforcingDlg::OnDestroy()
{
	clearSelectHoles();
	m_list_holeReinforcing.DeleteElements();
	CDialogEx::OnDestroy();
	// TODO: 在此处添加消息处理程序代码
}


void CHoleRebar_ReinforcingDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_list_holeReinforcing.DeleteElements();
	CDialogEx::OnClose();
}
void CHoleRebar_ReinforcingDlg::InitReinForcingInfoData(HoleRebarInfo::ReinForcingInfo& refdata)
{
	refdata.v1 = 0;
	refdata.v2 = 0;
	refdata.h3 = 0;
	refdata.h4 = 0;

	refdata.numv1 = 1;
	refdata.numv2 = 1;
	refdata.numh3 = 1;
	refdata.numh4 = 1;

	refdata.isUnionChild = false;
	refdata.isUnionHole = false;

	refdata.spacingv1 = 150;//g_globalpara.Getrebarspacing();
	refdata.spacingv2 = 150;//g_globalpara.Getrebarspacing();
	refdata.spacingh3 = 150;//g_globalpara.Getrebarspacing();
	refdata.spacingh4 = 150;//g_globalpara.Getrebarspacing();
	memset(refdata.Uname, 0, 256);

}
void CHoleRebar_ReinforcingDlg::UpdateHoleDataView(string holename)
{
	m_nowHolename = holename;
	for (HoleRebarInfo::ReinForcingInfo testInfo : m_vecReinF)
	{
		string tHolename(testInfo.Hname);
		if (tHolename != holename)
		{
			continue;
		}
		m_Check_reinv1.SetCheck(testInfo.v1);
		m_Check_revinv2.SetCheck(testInfo.v2);
		m_Check_revinh3.SetCheck(testInfo.h3);
		m_Check_revinh4.SetCheck(testInfo.h4);

		if (testInfo.v1&&testInfo.v2&&testInfo.h3&&testInfo.h4)
		{
			m_check_selectall.SetCheck(true);
		}


		CString strnumv1, strnumv2, strnumh3, strnumh4;
		strnumv1.Format(L"%d", testInfo.numv1);
		strnumv2.Format(L"%d", testInfo.numv2);
		strnumh3.Format(L"%d", testInfo.numh3);
		strnumh4.Format(L"%d", testInfo.numh4);
		if (!isFloor)//墙的顺序正常，板的上下的数量和间距是反过来的，因此是板的时候需要调换
		{
			m_Edit_revinnumv1.SetWindowTextW(strnumv1);
			m_Edit_revinnumv2.SetWindowTextW(strnumv2);
		}
		else
		{
			m_Edit_revinnumv1.SetWindowTextW(strnumv2);
			m_Edit_revinnumv2.SetWindowTextW(strnumv1);
		}
		m_Edit_revinnumh3.SetWindowTextW(strnumh3);
		m_Edit_revinnumh4.SetWindowTextW(strnumh4);

		CString strspacingv1, strspacingv2, strspacingh3, strspacingh4;
		strspacingv1.Format(L"%f", testInfo.spacingv1);
		strspacingv2.Format(L"%f", testInfo.spacingv2);
		strspacingh3.Format(L"%f", testInfo.spacingh3);
		strspacingh4.Format(L"%f", testInfo.spacingh4);
		if (!isFloor)
		{
			m_Edit_reinspacingv1.SetWindowTextW(strspacingv1);
			m_Edit_reinspacingv2.SetWindowTextW(strspacingv2);
		}
		else
		{
			m_Edit_reinspacingv1.SetWindowTextW(strspacingv2);
			m_Edit_reinspacingv2.SetWindowTextW(strspacingv1);
		}
		m_Edit_reinspacingh3.SetWindowTextW(strspacingh3);
		m_Edit_reinspacingh4.SetWindowTextW(strspacingh4);
	}


}
void CHoleRebar_ReinforcingDlg::GetNowHoleNum()
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
void CHoleRebar_ReinforcingDlg::ExTractHoleDatas()
{
	ElementId testid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	GetElementXAttribute(testid, m_vecReinF, ReinForcingInfoXAttribute, ACTIVEMODEL);
	GetConcreteXAttribute(testid, ACTIVEMODEL);
	if (m_vecReinF.size() < 1)
	{
		std::map<std::string, IDandModelref>::iterator itr = m_holeidAndmodel.begin();
		if (isArcwall == false)//不是弧形墙时处理方式
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
				//eeh.AddToModel();
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
				InitReinForcingInfoData(testInfo);
				/*if (testInfo.Hsize < 300 && testInfo.Hsize>200)
				{
					testInfo.v1 = 1;
					testInfo.h3 = 1;
				}
				else if (testInfo.Hsize > 300)
				{
					testInfo.v1 = 1;
					testInfo.h3 = 1;
					testInfo.v2 = 1;
					testInfo.h4 = 1;
				}*/
				testInfo.L0Lenth = 200;
				testInfo.MainRebarDis = 10;
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
				if (!GetArcCenterPoint(m_RebarPts, ptcenter, &eeh))
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
				InitReinForcingInfoData(testInfo);
				testInfo.L0Lenth = 200;
				testInfo.MainRebarDis = 10;
				m_vecReinF.push_back(testInfo);


			}
		}

	}


}

// HoleRebar_Reinforcing 消息处理程序
void CHoleRebar_ReinforcingDlg::GetUseHoles(EditElementHandleR m_ehSel, vector<DPoint3d>& m_FrontPts,
	vector<EditElementHandle*>& Holeehs, vector<EditElementHandle*>& useHoleehs, DgnModelRefP modelRef, bool isfoor)
{
	EditElementHandle Eleeh;
	EFT::GetSolidElementAndSolidHoles(m_ehSel, Eleeh, Holeehs);
	std::map<EditElementHandle*, EditElementHandle*> doorsholes;
	GetDoorHoles(Holeehs, doorsholes);
	double dSideCover = g_wallRebarInfo.concrete.sideCover*Get_uor_per_mm;
	double Wthickness;
	CopyToActivemodel(Eleeh);
	DPoint3d minP;
	DPoint3d maxP;
	//计算指定元素描述符中元素的范围。
	mdlElmdscr_computeRange(&minP, &maxP, Eleeh.GetElementDescrP(), NULL);
	Wthickness = maxP.y - minP.y;//这里求得的板厚一般肯定是大于实际板厚的，但是用于放大门洞的负实体不影响
	CalculateUseHoles(modelRef, Holeehs, useHoleehs, dSideCover, m_FrontPts, doorsholes, Wthickness, isfoor);
}
// HoleRebar_Reinforcing 消息处理程序
void CHoleRebar_ReinforcingDlg::GetARCUseHoles(EditElementHandleR m_ehSel, vector<EditElementHandle*>& Holeehs, vector<EditElementHandle*>& useHoleehs, DgnModelRefP modelRef)
{

	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle Eleeh;
	EFT::GetSolidElementAndSolidHoles(m_ehSel, Eleeh, Holeehs);

	double height;
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &height);
	DPoint3d centerpt;
	for (int i = 0; i < vecDownFaceLine.size(); i++)
	{
		if (vecDownFaceLine[i] != nullptr)
		{
			if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
			{
				double starR, sweepR;
				double radius;
				DPoint3d ArcDPs[2];
				RotMatrix rotM;

				mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &vecDownFaceLine[i]->el);
				break;
			}
		}
	}


	std::map<EditElementHandle*, EditElementHandle*> doorsholes;
	GetDoorHoles(Holeehs, doorsholes);
	double dSideCover = g_wallRebarInfo.concrete.sideCover*Get_uor_per_mm;
	double Wthickness;
	CopyToActivemodel(Eleeh);
	DPoint3d minP;
	DPoint3d maxP;
	//计算指定元素描述符中元素的范围。
	mdlElmdscr_computeRange(&minP, &maxP, Eleeh.GetElementDescrP(), NULL);
	Wthickness = maxP.y - minP.y;//这里求得的板厚一般肯定是大于实际板厚的，但是用于放大门洞的负实体不影响

	useHoleehs.clear();
	for (int j = 0; j < Holeehs.size(); j++)
	{
		EditElementHandle eeh;
		eeh.Duplicate(*Holeehs.at(j));

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
		DPoint3d ptcenter = centerpt;
		DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());

		ptcenter.z = ptele.z;
		CVector3D yVec = ptcenter - ptele;
		yVec.Normalize();

		CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

		DPoint3d ptStart = ptcenter;
		BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴

		Transform trans;
		placement.AssignTo(trans);
		Transform intrans = trans;
		intrans.InverseOf(intrans);

		TransformInfo transinfo(trans);
		//if (doorsholes[Holeehs.at(j)] != nullptr)//如果是门洞
		//{
		//	continue;
		//}
		//bool isdoorNeg = false;//判断是否为门洞NEG
		//isdoorNeg = IsDoorHoleNeg(Holeehs.at(j), doorsholes);
		ElementCopyContext copier(ACTIVEMODEL);
		copier.SetSourceModelRef(Holeehs.at(j)->GetModelRef());
		copier.SetTransformToDestination(true);
		copier.SetWriteElements(false);
		copier.DoCopy(*Holeehs.at(j));
		/*if (isdoorNeg)
		{
			PlusSideCover(*Holeehs.at(j), dSideCover, intrans, isdoorNeg, Wthickness);
		}
		else*/
		{
			PlusSideCover(*Holeehs.at(j), dSideCover, intrans);
		}
		useHoleehs.push_back(Holeehs.at(j));
	}
	/*if (useHoleehs.size() > 1)
	{
		UnionIntersectHoles(useHoleehs, Holeehs);
	}*/
}

void CHoleRebar_ReinforcingDlg::RefreshReinforcingRebars(ElementId conid, EditElementHandleR eeh)
{
	PIT::PITRebarAssembly::DeleteRebarsFromAssembly(conid, "class HoleRFRebarAssembly");
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
	CHoleRebar_ReinforcingDlg* pHoleReinForcingDlgtmp = NULL;
	pHoleReinForcingDlgtmp = new CHoleRebar_ReinforcingDlg(CWnd::FromHandle(MSWIND));
	pHoleReinForcingDlgtmp->isArcwall = isarcwall;
	pHoleReinForcingDlgtmp->isFloor = isfloor;
	pHoleReinForcingDlgtmp->SetSelectElement(eeh);
	pHoleReinForcingDlgtmp->Create(IDD_DIALOG_HoleRebar_Reinforcing);
	pHoleReinForcingDlgtmp->OnBnClickedOk();
	//delete pHoleReinForcingDlgtmp;
}


void CHoleRebar_ReinforcingDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	m_list_holeReinforcing.DeleteElements();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = m_ehSel.GetModelRef();
	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());
	m_list_holeReinforcing.GetAllRebarData(m_vecReinF);
	/***********************************给sizekey附加型号******************************************************/

	auto it = m_vecRebarData.begin();
	for (; it != m_vecRebarData.end(); it++)
	{
		BrString strRebarSize = m_rebarDia;
		if (strRebarSize != L"")
		{
			if (strRebarSize.Find(L"mm") != -1)
			{
				strRebarSize.Replace(L"mm", L"");
			}
		}
		else
		{
			strRebarSize = XmlManager::s_alltypes[Typenum];
		}
		strcpy(it->rebarSize, CT2A(strRebarSize));
		it->rebarType = Typenum;
		GetDiameterAddType(it->rebarSize, it->rebarType);
	}
	/***********************************给sizekey附加型号******************************************************/
	if (m_vecReinF.size() > 0)
	{
		if (!isArcwall)
		{
			std::vector<EditElementHandle*> Holeehs;
			vector<EditElementHandle*> useHoleehs;
			GetUseHoles(eeh, m_FrontPts, Holeehs, useHoleehs, modelRef, isFloor);
			HoleRFRebarAssembly::IsSmartSmartFeature(eeh);
			if (m_HoleRebar == nullptr)
			{
				ElementId testid = 0;
				GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
				RebarAssembly* rebaras = HoleRFRebarAssembly::GetRebarAssembly(testid, "class HoleRFRebarAssembly");
				m_HoleRebar = dynamic_cast<HoleRFRebarAssembly*>(rebaras);
				if (m_HoleRebar == nullptr)
				{
					m_HoleRebar = REA::Create<HoleRFRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
					m_HoleRebar->SetConcreteOwner(testid);
				}
			}
			if (m_HoleRebar == nullptr)
			{
				m_HoleRebar = REA::Create<HoleRFRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
			m_HoleRebar->ClearData();
			m_HoleRebar->SetTrans(m_trans);
			m_HoleRebar->SetaccConcrete(m_acconcrete);
			m_HoleRebar->Settypenum(Typenum);
			//m_HoleRebar->SetVecDirSizeData(g_vecRebarData);
			m_HoleRebar->SetVecDirSizeData(m_vecRebarData);
			m_HoleRebar->SetrebarPts(m_RebarPts);
			m_HoleRebar->SetvecFrontPts(m_FrontPts);
			m_HoleRebar->SetvecReinF(m_vecReinF);
			m_HoleRebar->SetSelectedElement(eeh.GetElementId());
			m_HoleRebar->SetSelectedModel(eeh.GetModelRef());
			m_HoleRebar->isfloor = isFloor;
			m_HoleRebar->m_useHoleehs.clear();
			m_HoleRebar->m_useHoleehs.insert(m_HoleRebar->m_useHoleehs.begin(), useHoleehs.begin(), useHoleehs.end());
			m_HoleRebar->m_holeidAndmodel.insert(m_holeidAndmodel.begin(), m_holeidAndmodel.end());
			m_HoleRebar->m_strElmName = m_nowHolename;
			m_HoleRebar->MakeRebars(modelRef);
			m_HoleRebar->Save(modelRef); // must save after creating rebars
			ElementId contid = m_HoleRebar->FetchConcrete();
			EditElementHandle eeh2(contid, ACTIVEMODEL);
			ElementRefP oldRef = eeh2.GetElementRef();
			mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
			eeh2.ReplaceInModel(oldRef);
			SetElementXAttribute(m_HoleRebar->FetchConcrete(), m_vecReinF, ReinForcingInfoXAttribute, ACTIVEMODEL);

			FreeAll(Holeehs);
			//FreeAll(useHoleehs);
		}
		else
		{
			std::vector<EditElementHandle*> Holeehs;
			vector<EditElementHandle*> useHoleehs;
			GetARCUseHoles(eeh, Holeehs, useHoleehs, modelRef);
			if (m_ArcHoleRebar == nullptr)
			{
				ElementId testid = 0;
				GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
				RebarAssembly* rebaras = HoleArcRFRebarAssembly::GetRebarAssembly(testid, "class HoleArcRFRebarAssembly");
				m_ArcHoleRebar = dynamic_cast<HoleArcRFRebarAssembly*>(rebaras);
				if (m_ArcHoleRebar == nullptr)
				{
					m_ArcHoleRebar = REA::Create<HoleArcRFRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
					m_ArcHoleRebar->SetConcreteOwner(testid);
				}
			}
			if (m_ArcHoleRebar == nullptr)
			{
				m_ArcHoleRebar = REA::Create<HoleArcRFRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
			m_ArcHoleRebar->ClearData();
			m_ArcHoleRebar->SetTrans(m_trans);
			m_ArcHoleRebar->SetaccConcrete(m_acconcrete);
			m_ArcHoleRebar->SetVecDirSizeData(m_vecRebarData);
			m_ArcHoleRebar->Settypenum(Typenum);
			//m_ArcHoleRebar->SetVecDirSizeData(g_vecRebarData);
			m_ArcHoleRebar->SetrebarPts(m_RebarPts);
			m_ArcHoleRebar->SetvecFrontPts(m_FrontPts);
			m_ArcHoleRebar->SetvecReinF(m_vecReinF);
			m_ArcHoleRebar->m_useHoleehs.clear();
			m_ArcHoleRebar->m_useHoleehs.insert(m_ArcHoleRebar->m_useHoleehs.begin(), useHoleehs.begin(), useHoleehs.end());
			m_ArcHoleRebar->m_holeidAndmodel.insert(m_holeidAndmodel.begin(), m_holeidAndmodel.end());
			m_ArcHoleRebar->m_strElmName = m_nowHolename;
			m_ArcHoleRebar->MakeRebars(modelRef);
			m_ArcHoleRebar->Save(modelRef); // must save after creating rebars
			ElementId contid = m_ArcHoleRebar->FetchConcrete();
			EditElementHandle eeh2(contid, ACTIVEMODEL);
			ElementRefP oldRef = eeh2.GetElementRef();
			mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
			eeh2.ReplaceInModel(oldRef);
			SetElementXAttribute(m_ArcHoleRebar->FetchConcrete(), m_vecReinF, ReinForcingInfoXAttribute, ACTIVEMODEL);

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


void CHoleRebar_ReinforcingDlg::OnBnClickedCheckReinv1()
{
	m_vecReinF[m_nowHoleNum].v1 = m_Check_reinv1.GetCheck();
	// TODO: 在此添加控件通知处理程序代码
}


void CHoleRebar_ReinforcingDlg::OnBnClickedCheckReinv2()
{
	// TODO: 在此添加控件通知处理程序代码
	m_vecReinF[m_nowHoleNum].v2 = m_Check_revinv2.GetCheck();
}


void CHoleRebar_ReinforcingDlg::OnBnClickedCheckReinh3()
{
	// TODO: 在此添加控件通知处理程序代码
	m_vecReinF[m_nowHoleNum].h3 = m_Check_revinh3.GetCheck();
}


void CHoleRebar_ReinforcingDlg::OnBnClickedCheckReinh4()
{
	// TODO: 在此添加控件通知处理程序代码
	m_vecReinF[m_nowHoleNum].h4 = m_Check_revinh4.GetCheck();
}


void CHoleRebar_ReinforcingDlg::OnEnChangeEditReinnumv1()
{
	CString strNum;
	m_Edit_revinnumv1.GetWindowTextW(strNum);
	if(isFloor)//由于板的上下钢筋数量是相反的，需要调换
		m_vecReinF[m_nowHoleNum].numv2 = atoi(CT2A(strNum));
	else
		m_vecReinF[m_nowHoleNum].numv1 = atoi(CT2A(strNum));
}


void CHoleRebar_ReinforcingDlg::OnEnChangeEditReinnumv2()
{
	CString strNum;
	m_Edit_revinnumv2.GetWindowTextW(strNum);
	if(isFloor)//由于板的上下钢筋数量是相反的，需要调换
		m_vecReinF[m_nowHoleNum].numv1 = atoi(CT2A(strNum));
	else
		m_vecReinF[m_nowHoleNum].numv2 = atoi(CT2A(strNum));
}


void CHoleRebar_ReinforcingDlg::OnEnChangeEditReinnumh3()
{
	CString strNum;
	m_Edit_revinnumh3.GetWindowTextW(strNum);
	m_vecReinF[m_nowHoleNum].numh3 = atoi(CT2A(strNum));
}


void CHoleRebar_ReinforcingDlg::OnEnChangeEditReinnumh4()
{
	CString strNum;
	m_Edit_revinnumh4.GetWindowTextW(strNum);
	m_vecReinF[m_nowHoleNum].numh4 = atoi(CT2A(strNum));
}


void CHoleRebar_ReinforcingDlg::OnEnChangeEditReinspacingv1()
{
	CString strNum;
	m_Edit_reinspacingv1.GetWindowTextW(strNum);
	if(isFloor)
		m_vecReinF[m_nowHoleNum].spacingv2 = atof(CT2A(strNum));
	else 
		m_vecReinF[m_nowHoleNum].spacingv1 = atof(CT2A(strNum));
}


void CHoleRebar_ReinforcingDlg::OnEnChangeEditReinspacingv2()
{
	CString strNum;
	m_Edit_reinspacingv2.GetWindowTextW(strNum);
	if(isFloor)
		m_vecReinF[m_nowHoleNum].spacingv1 = atof(CT2A(strNum));
	else
		m_vecReinF[m_nowHoleNum].spacingv2 = atof(CT2A(strNum));
}


void CHoleRebar_ReinforcingDlg::OnEnChangeEditReinspacingh3()
{
	CString strNum;
	m_Edit_reinspacingh3.GetWindowTextW(strNum);
	m_vecReinF[m_nowHoleNum].spacingh3 = atof(CT2A(strNum));
}


void CHoleRebar_ReinforcingDlg::OnEnChangeEditReinspacingh4()
{
	CString strNum;
	m_Edit_reinspacingh4.GetWindowTextW(strNum);
	m_vecReinF[m_nowHoleNum].spacingh4 = atof(CT2A(strNum));
}


void CHoleRebar_ReinforcingDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	clearSelectHoles();
	m_list_holeReinforcing.DeleteElements();
	DestroyWindow();
}


void CHoleRebar_ReinforcingDlg::OnBnClickedSetUnionholeButton()
{
	m_list_holeReinforcing.DeleteElements();
	// TODO: 在此添加控件通知处理程序代码
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CHoleRebar_AddUnionHoleDlg* adduniondlg = new CHoleRebar_AddUnionHoleDlg();
	adduniondlg->m_Reinforcingdlg = this;
	adduniondlg->m_trans = m_trans;
	adduniondlg->m_FrontPts = m_FrontPts;
	adduniondlg->m_RebarPts = m_RebarPts;
	adduniondlg->m_type = CHoleRebar_AddUnionHoleDlg::ReinForcing;
	adduniondlg->SetHoleData(m_holeidAndmodel);
	adduniondlg->SetListRowData(m_vecReinF);
	adduniondlg->Create(IDD_DIALOG_ADDUNIONHOLE, this);
	adduniondlg->ShowWindow(SW_SHOW);
}


void CHoleRebar_ReinforcingDlg::OnBnClickedButtonDissunion()
{
	// TODO: 在此添加控件通知处理程序代码
	HoleRebarInfo::ReinForcingInfo tmpInfo = m_vecReinF[m_nowHoleNum];
	if (tmpInfo.isUnionHole)
	{
		std::string unionName(tmpInfo.Hname);
		for (HoleRebarInfo::ReinForcingInfo& tinfo : m_vecReinF)
		{
			if (tinfo.Uname != "")
			{
				std::string tuname(tinfo.Uname);
				if (tuname.find(unionName) != string::npos)
				{
					vector<string> vecTemp;
					StringOperator::Modify::SplitStr(tuname, ",", vecTemp);
					if (vecTemp.size() < 2)
					{
						tinfo.isUnionChild = false;
						memset(tinfo.Uname, 0, 256);
					}
					else
					{
						string nowuname = "";
						for (int i = 0; i < vecTemp.size(); i++)
						{
							if (vecTemp.at(i) != unionName)
							{
								if (nowuname == "")
								{
									nowuname = vecTemp.at(i);
								}
								else
								{
									nowuname = nowuname + "," + vecTemp.at(i);
								}
							}
						}
						sprintf(tinfo.Uname, "%s", nowuname.c_str());
					}
				}
			}

		}
		m_vecReinF.erase(m_vecReinF.begin() + m_nowHoleNum);
	}

	UpdateACList();

	m_list_holeReinforcing.DeleteElements();

}


//void CHoleRebar_ReinforcingDlg::OnBnClickedCheck1()
//{
//	// TODO: 在此添加控件通知处理程序代码
//}


void CHoleRebar_ReinforcingDlg::OnBnClickedCheckReinseclectall()
{
	// TODO: 在此添加控件通知处理程序代码
	m_Check_reinv1.SetCheck(m_check_selectall.GetCheck());
	m_Check_revinv2.SetCheck(m_check_selectall.GetCheck());
	m_Check_revinh3.SetCheck(m_check_selectall.GetCheck());
	m_Check_revinh4.SetCheck(m_check_selectall.GetCheck());
	m_vecReinF[m_nowHoleNum].v1 = m_check_selectall.GetCheck();
	m_vecReinF[m_nowHoleNum].v2 = m_check_selectall.GetCheck();
	m_vecReinF[m_nowHoleNum].h3 = m_check_selectall.GetCheck();
	m_vecReinF[m_nowHoleNum].h4 = m_check_selectall.GetCheck();
}


void CHoleRebar_ReinforcingDlg::clearSelectHoles()
{
	for (ElementRefP tmpeeh : m_vctSelectHoles)
	{
		EditElementHandle eeh(tmpeeh, tmpeeh->GetDgnModelP());
		eeh.DeleteFromModel();
	}
	m_vctSelectHoles.clear();
	m_NewHoleElements.clear();
}

void CHoleRebar_ReinforcingDlg::OnBnClickedButton1()//点选孔洞按钮
{
	clearSelectHoles();
	m_NewHoleElements.clear();
	for (auto it = m_holeidAndmodel.begin(); it != m_holeidAndmodel.end(); it++)
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


void CHoleRebar_ReinforcingDlg::GetSeclectElement(EditElementHandleR HoleEeh)
{
	ElementId HoleID = HoleEeh.GetElementId();
	string holename = "";

	if (m_NewHoleElements.size() == 0)
		return;
	for (auto it = m_NewHoleElements.begin(); it != m_NewHoleElements.end(); it++)
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
		m_list_holeReinforcing.SetFocus();
		m_list_holeReinforcing.SetItemState(index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_nowHolename = holename;
		GetNowHoleNum();
		UpdateHoleDataView(holename);
	}
}


void CHoleRebar_ReinforcingDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0; //根据当前dgn模型获取比例因子
	int nIndex = m_diameter.GetCurSel();
	CString rebarDiaStr;
	m_diameter.GetLBText(nIndex, rebarDiaStr);
	m_rebarDia = rebarDiaStr;
}


void CHoleRebar_ReinforcingDlg::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0; //根据当前dgn模型获取比例因子
	int nIndex = m_type.GetCurSel();
	Typenum = nIndex;
	CString rebarDiaStr;
	m_type.GetLBText(nIndex, rebarDiaStr);
}
