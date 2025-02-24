// CGWallRebarDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "ElementAttribute.h"
#include "CGWallRebarDlgNew.h"
#include "afxdialogex.h"
#include "WallRebarAssemblyNew.h"
#include "../../resource.h"
#include "../../ConstantsDef.h"
#include "../../CWallRebarDlg.h"
#include "../../CommonFile.h"
#include "../../AssociatedComponent.h"
#include "../../PITACCRebarAssembly.h"
// CGWallRebarDlg 对话框

IMPLEMENT_DYNAMIC(CGWallRebarDlgNew, CDialogEx)

CGWallRebarDlgNew::CGWallRebarDlgNew(ElementHandleCR eh, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CNGwall, pParent)
{
	m_ehSel = eh;

	m_WallSetInfo.rebarType = 2;
}

CGWallRebarDlgNew::~CGWallRebarDlgNew()
{
}

void CGWallRebarDlgNew::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_GPositive, m_editPositive);
	DDX_Control(pDX, IDC_EDIT_GSide, m_editSide);
	DDX_Control(pDX, IDC_EDIT_GReverse, m_editReverse);
	DDX_Control(pDX, IDC_HOLE_GCHECK, m_hole_check);
	DDX_Control(pDX, IDC_MHOLESIZE_GEDIT, m_holesize_edit);
	DDX_Control(pDX, IDC_EDIT_GLevel, m_editLevel);
	DDX_Control(pDX, IDC_LIST_GMainRebar, m_listMainRebar);

	DDX_Control(pDX, IDC_COMBO2, m_ComboSize);
	DDX_Control(pDX, IDC_COMBO3, m_ComboType);
	DDX_Control(pDX, IDC_EDIT1, m_EditSpace);
	DDX_Control(pDX, IDC_STATIC_WALLNAME, m_static_wallname);
}


BEGIN_MESSAGE_MAP(CGWallRebarDlgNew, CDialogEx)
	ON_BN_CLICKED(IDOK_Gwall, &CGWallRebarDlgNew::OnBnOKClickedGwall)
	ON_EN_KILLFOCUS(IDC_EDIT_GPositive, &CGWallRebarDlgNew::OnEnKillfocusEditGpositive)
	ON_EN_KILLFOCUS(IDC_EDIT_GSide, &CGWallRebarDlgNew::OnEnKillfocusEditGside)
	ON_EN_KILLFOCUS(IDC_EDIT_GReverse, &CGWallRebarDlgNew::OnEnKillfocusEditGreverse)
	ON_BN_CLICKED(IDC_HOLE_GCHECK, &CGWallRebarDlgNew::OnBnClickedHoleGcheck)
	ON_EN_KILLFOCUS(IDC_MHOLESIZE_GEDIT, &CGWallRebarDlgNew::OnEnKillfocusMholesizeGedit)
	ON_EN_KILLFOCUS(IDC_EDIT_GLevel, &CGWallRebarDlgNew::OnEnKillfocusEditGlevel)
	ON_BN_CLICKED(IDCANCEL_Gwall, &CGWallRebarDlgNew::OnBnClickedGwall)
	ON_EN_CHANGE(IDC_EDIT1, &CGWallRebarDlgNew::OnEnChangeEdit1)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CGWallRebarDlgNew::OnCbnSelchangeCombo3)
	ON_STN_CLICKED(IDC_STATIC_WALLNAME, &CGWallRebarDlgNew::OnStnClickedStaticWallname)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CGWallRebarDlgNew::OnCbnSelchangeCombo2)
END_MESSAGE_MAP()


// CGWallRebarDlg 消息处理程序

void CGWallRebarDlgNew::InitUIData()
{
	CString strLevel, strPositive, strSide, strReverse, strMissHoleSize;
	strLevel.Format(_T("%d"), g_wallRebarInfo.concrete.rebarLevelNum);
	strPositive.Format(_T("%.2f"), g_wallRebarInfo.concrete.postiveCover);
	strReverse.Format(_T("%.2f"), g_wallRebarInfo.concrete.reverseCover);
	strSide.Format(_T("%.2f"), g_wallRebarInfo.concrete.sideCover);
	strMissHoleSize.Format(_T("%.2f"), g_wallRebarInfo.concrete.MissHoleSize);
	if (g_wallRebarInfo.concrete.isHandleHole)
	{
		m_hole_check.SetCheck(true);
		m_holesize_edit.SetReadOnly(FALSE);
	}
	else
	{
		m_hole_check.SetCheck(false);
		m_holesize_edit.SetReadOnly(TRUE);
	}
	m_editLevel.SetWindowText(strLevel);
	m_editPositive.SetWindowText(strPositive);
	m_editReverse.SetWindowText(strReverse);
	m_editSide.SetWindowText(strSide);
	m_holesize_edit.SetWindowText(strMissHoleSize);

	string elename, eletype;
	GetEleNameAndType(m_ehSel.GetElementId(), m_ehSel.GetModelRef(), elename, eletype);
	wall_name = elename;
	CString wallname(elename.c_str());
	wallname = L"墙名:" + wallname;
	m_static_wallname.SetWindowTextW(wallname);

	for each (auto var in g_listRebarSize)
		m_ComboSize.AddString(var);
	for each (auto var in g_listRebarType)
		m_ComboType.AddString(var);

	CString strRebarSize(m_WallSetInfo.rebarSize);
	if (strRebarSize.Find(L"mm") == -1)
		strRebarSize += "mm";
	int nIndex = m_ComboSize.FindStringExact(0, strRebarSize);
	m_ComboSize.SetCurSel(nIndex);//尺寸
	m_ComboType.SetCurSel(m_WallSetInfo.rebarType);//型号

	LONG lStyle;
	lStyle = GetWindowLong(m_listMainRebar.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_listMainRebar.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_listMainRebar.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_listMainRebar.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_listMainRebar.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_listMainRebar.InsertColumn(0, _T("层"), (int)(width / 9.0*0.75), ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_listMainRebar.InsertColumn(1, _T("方向"), (int)(width / 9.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(2, _T("直径"), (int)(width / 9.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(3, _T("类型"), (int)(width / 9.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(4, _T("间距"), (int)(width / 9.0*1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(5, _T("起点偏移"), (int)(width / 9.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(6, _T("终点偏移"), (int)(width / 9.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(7, _T("与前层间距"), (int)(width / 9.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(8, _T("位置"), (int)(width / 9.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

	//	m_listMainRebar.SetShowProgressPercent(TRUE);
	//	m_listMainRebar.SetSupportSort(TRUE);

	//	SetListRowData(g_vecRebarData);

	UpdateRebarList();
}


void CGWallRebarDlgNew::SetListDefaultData()
{
	if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
		{
			PIT::ConcreteRebar oneRebarData;
			if (0 == i)
				oneRebarData = { i,0,"",0,200,0,0,0 };
			else if (1 == i)
				oneRebarData = { i,0,"",0,200,0,0,0 };
			else
			{
				int dir = (i + 1) & 0x01;
				double levelSpace;
				levelSpace = ((i + 1) & 0x01) * 200.0;
				oneRebarData = { i,dir,"",0,200,0,0,levelSpace };
			}
			m_vecRebarData.push_back(oneRebarData);
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = g_wallRebarInfo.concrete.rebarLevelNum - (int)m_vecRebarData.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				int dir = (i + 1) & 0x01;
				double levelSpace;
				levelSpace = dir * 200.0;
				PIT::ConcreteRebar oneRebarData = { i,dir,"",0,200,0,0,levelSpace };
				m_vecRebarData.push_back(oneRebarData);
			}
		}
		if (iOffset < 0)
		{
			iOffset *= -1;
			for (int i = 0; i < iOffset; i++)
			{
				m_vecRebarData.pop_back();
			}
		}
	}
	for (int i = 0;i<m_vecRebarData.size();i++)
	{
		string tmpstring = CT2A(XmlManager::s_alltypes[m_vecRebarData.at(i).rebarType]);
	  sprintf(m_vecRebarData.at(i).rebarSize, "%s", tmpstring);
	}
	g_vecRebarData = m_vecRebarData;
}

void CGWallRebarDlgNew::UpdateRebarList()
{
	m_listMainRebar.DeleteAllItems();
	SetListDefaultData();
	for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
	{
		if (0 == i)
		{
			m_listMainRebar.InsertItem(i, _T("")); // 插入行
			m_listMainRebar.SetItemText(i, 0, _T("1LX"));
		}
		else
		{
			CString strValue;
			strValue.Format(_T("%dL"), i);
			m_listMainRebar.InsertItem(i, _T("")); // 插入行
			m_listMainRebar.SetItemText(i, 0, strValue);

		}
		int nSubCnt = m_listMainRebar.GetColumnCount() - 1;
		for (int j = 1; j <= nSubCnt; ++j)
		{
			CString strValue;
			switch (j)
			{
			case 1:
			{
				ListCtrlEx::CStrList strlist = g_listRebarDir;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecRebarData[i].rebarDir);
				strValue = *it;
			}
			break;
			case 2:
			{
				ListCtrlEx::CStrList strlist = g_listRebarType;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecRebarData[i].rebarType);
				strValue = *it;
			}
			break;
			case 3:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].spacing);
				break;
			case 4:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].startOffset);
				break;
			case 5:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].endOffset);
				break;
			case 6:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].levelSpace);
				break;
			case 7:
			{
				ListCtrlEx::CStrList strlist;
				strlist.push_back(_T("无"));
				for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
				{
					CString strValue;
					if (0 == i)
						strValue = _T("1XL");
					else
						strValue.Format(_T("%dL"), i);
					strlist.push_back(strValue);
				}
				m_listMainRebar.SetCellStringList(i, j, strlist);
				strValue = _T("无");
			}

			break;
			default:
				break;
			}
			m_listMainRebar.SetItemText(i, j, strValue);
		}

	}
	//	m_listMainRebar.SetShowProgressPercent(TRUE);
	//	m_listMainRebar.SetSupportSort(TRUE);
}

void CGWallRebarDlgNew::GetConcreteData(PIT::Concrete& concreteData)
{
	// 	CString strLevelNum,strPostiveCover,strSideCover,strReverseCover;
	// 	m_EditLevel.GetWindowText(strLevelNum);
	// 	concreteData.rebarLevelNum = atoi(CT2A(strLevelNum));
	// 	concreteData.isTwinbars = m_ComboIsTwinBars.GetCurSel();
	// 	m_editPositive.GetWindowText(strPostiveCover);
	// 	g_wallRebarInfo.concrete.postiveCover = atof(CT2A(strPostiveCover));
	// 	m_editSide.GetWindowText(strSideCover);
	// 	g_wallRebarInfo.concrete.sideCover = atof(CT2A(strSideCover));
	// 	m_editReverse.GetWindowText(strReverseCover);
	// 	g_wallRebarInfo.concrete.reverseCover = atof(CT2A(strReverseCover));
	concreteData = g_wallRebarInfo.concrete;
}

BOOL CGWallRebarDlgNew::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	InitUIData();


	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CGWallRebarDlgNew::OnBnOKClickedGwall()
{
	// TODO: 在此添加控件通知处理程序代码
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = m_ehSel.GetModelRef();
	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());

	   GetConcreteData(g_wallRebarInfo.concrete);
	   m_listMainRebar.GetAllRebarData(m_vecRebarData);
	   SetListRowData(m_vecRebarData);
	

		WallRebarAssemblyNew::IsSmartSmartFeature(eeh);
		WallRebarAssemblyNew*  wallRebar = NULL;
		ElementId testid = 0;
		GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());

		WallRebarAssemblyNew::WallType wallType = WallRebarAssemblyNew::GWALL;

		RebarAssembly* rebaras = PIT::ACCRebarAssembly::GetRebarAssembly(testid, "class GWallRebarAssemblyNew");
		wallRebar = dynamic_cast<GWallRebarAssemblyNew*>(rebaras);
		if (wallRebar == nullptr)
		{
			wallRebar = REA::Create<GWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		}

		wallRebar->SetwallType(wallType);
		wallRebar->SetWallData(eeh);
		
		wallRebar->SetConcreteData(g_wallRebarInfo.concrete);
		wallRebar->SetRebarData(m_vecRebarData);
		wallRebar->InitRebarSetId();
		wallRebar->MakeRebars(modelRef);
		wallRebar->Save(modelRef); // must save after creating rebars
		ElementId contid = wallRebar->FetchConcrete();
		EditElementHandle eeh2(contid, ACTIVEMODEL);
		ElementRefP oldRef = eeh2.GetElementRef();
		mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
		eeh2.ReplaceInModel(oldRef);
		//eeh2.AddToModel();

		SetConcreteXAttribute(contid, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(PIT::WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);	

		SetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
		Transform trans;
		wallRebar->GetPlacement().AssignTo(trans);
		SetElementXAttribute(contid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, wallRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
}

void CGWallRebarDlgNew::OnEnKillfocusEditGpositive()
{
	// TODO: 在此添加控件通知处理程序代码
	
	CString strPostiveCover;
	m_editPositive.GetWindowText(strPostiveCover);
	g_wallRebarInfo.concrete.postiveCover = atof(CT2A(strPostiveCover));
}

void CGWallRebarDlgNew::OnEnKillfocusEditGside()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strSideCover;
	m_editSide.GetWindowText(strSideCover);
	g_wallRebarInfo.concrete.sideCover = atof(CT2A(strSideCover));
}


void CGWallRebarDlgNew::OnEnKillfocusEditGreverse()
{
	// TODO: 在此添加控件通知处理程序代码
		// TODO: 在此添加控件通知处理程序代码
	CString strReverseCover;
	m_editReverse.GetWindowText(strReverseCover);
	g_wallRebarInfo.concrete.reverseCover = atof(CT2A(strReverseCover));
}


void CGWallRebarDlgNew::OnBnClickedHoleGcheck()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_hole_check.GetCheck())
	{
		m_holesize_edit.SetReadOnly(FALSE);
		g_wallRebarInfo.concrete.isHandleHole = 1;

	}
	else
	{
		m_holesize_edit.SetReadOnly(TRUE);
		g_wallRebarInfo.concrete.isHandleHole = 0;
	}
}


void CGWallRebarDlgNew::OnEnKillfocusMholesizeGedit()
{
	// TODO: 在此添加控件通知处理程序代码
	// TODO: 在此添加控件通知处理程序代码
	CString strMissHoleSize;
	m_holesize_edit.GetWindowText(strMissHoleSize);
	g_wallRebarInfo.concrete.MissHoleSize = atof(CT2A(strMissHoleSize));
}


void CGWallRebarDlgNew::OnEnKillfocusEditGlevel()
{
	// TODO: 在此添加控件通知处理程序代码
		// TODO: 在此添加控件通知处理程序代码
	CString strLevelNum;
	m_editLevel.GetWindowText(strLevelNum);
	g_wallRebarInfo.concrete.rebarLevelNum = atoi(CT2A(strLevelNum));
	UpdateRebarList();
	//CTabCtrl* pTabCtrl = (CTabCtrl*)GetParent();
	//if (pTabCtrl)
	//{
	//	CWallRebarDlg* pDlg = (CWallRebarDlg*)pTabCtrl->GetParent();
	//	if (pDlg)
	//	{
	//		//			pDlg->m_PageLapOption.UpdateLapOptionList();
	//		pDlg->m_PageEndType.UpdateEndTypeList();
	//		pDlg->m_PageTwinBars.UpdateTwinBarsList();
	//	}
	//}
}


void CGWallRebarDlgNew::OnBnClickedGwall()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}


void CGWallRebarDlgNew::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString	strTemp = CString();
	m_EditSpace.GetWindowText(strTemp);
	m_WallSetInfo.spacing = atof(CT2A(strTemp));

	ChangeRebarSpacedata(m_WallSetInfo.spacing);
	UpdateRebarList();
}


void CGWallRebarDlgNew::OnCbnSelchangeCombo3() // 类型
{
	// TODO: 在此添加控件通知处理程序代码
	auto it = g_listRebarType.begin();
	advance(it, m_ComboType.GetCurSel());
	m_WallSetInfo.rebarType = m_ComboType.GetCurSel();
	ChangeRebarTypedata(m_WallSetInfo.rebarType);
	UpdateRebarList();
}


void CGWallRebarDlgNew::OnStnClickedStaticWallname()
{
}


void CGWallRebarDlgNew::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
		// TODO: 在此添加控件通知处理程序代码
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboSize.GetCurSel());
	strcpy(m_WallSetInfo.rebarSize, CT2A(*it));
	BrString str = *it;
	if (str != L"")
	{
		if (str.Find(L"mm") != -1)
		{
			str.Replace(L"mm", L"");
		}
	}
	strcpy(m_WallSetInfo.rebarSize, CT2A(str));
	UpdateRebarList();
}
