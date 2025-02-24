// CWallMainRebar.cpp: 实现文件
//

#include "_USTATION.h"
#include "CWallMainRebarDlgNew.h"
#include "afxdialogex.h"
#include "../../resource.h"
#include "../../ConstantsDef.h"
#include "CWallRebarDlgNew.h"
#include "../../CommonFile.h"
#include "../../AssociatedComponent.h"
#include "ElementAttribute.h"
#include "../../PITRebarCurve.h"
#include "../../ACCRebarMaker.h"

#define WM_UPDATELIST  WM_USER+1   // do something  
#define WM_LEVELCHANGE  WM_USER+2   // do something  



// CWallMainRebar 对话框

IMPLEMENT_DYNAMIC(CWallMainRebarDlgNew, CDialogEx)

CWallMainRebarDlgNew::CWallMainRebarDlgNew(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CNWallRebar_MainRebar, pParent), m_assodlg(NULL)
{
	pm_MainPageRebar = nullptr;

	mArcLine.radius = 0.0;
	mArcLine.ArcLen = 0.0;
	mArcLine.ptStart = DPoint3d::FromZero();
	mArcLine.ptEnd = DPoint3d::FromZero();
	mArcLine.centerpt = DPoint3d::FromZero();

}

CWallMainRebarDlgNew::~CWallMainRebarDlgNew()
{
	if (m_assodlg != nullptr)
	{
		delete m_assodlg;
		m_assodlg = nullptr;
	}

}

void CWallMainRebarDlgNew::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_listMainRebar);
	DDX_Control(pDX, IDC_EDIT1, m_EditPositive);
	DDX_Control(pDX, IDC_EDIT2, m_EditSide);
	DDX_Control(pDX, IDC_EDIT3, m_EditReverse);
	DDX_Control(pDX, IDC_EDIT4, m_EditLevel);
	DDX_Control(pDX, IDC_HOLE_CHECK, m_hole_check);
	DDX_Control(pDX, IDC_MHOLESIZE_EDIT, m_mholesize_edit);

	DDX_Control(pDX, IDC_CHECK1, m_CheckIsCutRebar);
	DDX_Control(pDX, IDC_EDIT5, m_EditCutLength1);
	DDX_Control(pDX, IDC_EDIT6, m_EditCutLength2);
	DDX_Control(pDX, IDC_EDIT7, m_EditCutLength3);
}


BEGIN_MESSAGE_MAP(CWallMainRebarDlgNew, CDialogEx)
	ON_EN_KILLFOCUS(IDC_EDIT1, &CWallMainRebarDlgNew::OnEnKillfocusEdit1)
	ON_EN_KILLFOCUS(IDC_EDIT2, &CWallMainRebarDlgNew::OnEnKillfocusEdit2)
	ON_EN_KILLFOCUS(IDC_EDIT3, &CWallMainRebarDlgNew::OnEnKillfocusEdit3)
	ON_EN_KILLFOCUS(IDC_EDIT4, &CWallMainRebarDlgNew::OnEnKillfocusEdit4)
	ON_MESSAGE(WM_UPDATELIST, &CWallMainRebarDlgNew::OnComboBoxDataChange)
	ON_BN_CLICKED(IDC_BUTTON1, &CWallMainRebarDlgNew::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_HOLE_CHECK, &CWallMainRebarDlgNew::OnBnClickedHoleCheck)
	ON_EN_KILLFOCUS(IDC_MHOLESIZE_EDIT, &CWallMainRebarDlgNew::OnEnKillfocusMholesizeEdit)
	ON_BN_CLICKED(IDC_BUTTON2, &CWallMainRebarDlgNew::OnBnClickedButton2)
	//ON_CBN_SELCHANGE(IDC_COMBO1, &CWallMainRebarDlgNew::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_CHECK1, &CWallMainRebarDlgNew::OnBnClickedCheck1)
	ON_EN_CHANGE(IDC_EDIT7, &CWallMainRebarDlgNew::OnEnChangeEdit7)
	ON_EN_CHANGE(IDC_EDIT6, &CWallMainRebarDlgNew::OnEnChangeEdit6)
	ON_EN_CHANGE(IDC_EDIT5, &CWallMainRebarDlgNew::OnEnChangeEdit5)
END_MESSAGE_MAP()


// CWallMainRebar 消息处理程序

void CWallMainRebarDlgNew::InitUIData()
{
	CString strLevel, strPositive, strSide, strReverse,strMissHoleSize;
	strLevel.Format(_T("%d"), g_wallRebarInfo.concrete.rebarLevelNum);
	strPositive.Format(_T("%.2f"), g_wallRebarInfo.concrete.postiveCover);
	strReverse.Format(_T("%.2f"), g_wallRebarInfo.concrete.reverseCover);
	strSide.Format(_T("%.2f"), g_wallRebarInfo.concrete.sideCover);
	strMissHoleSize.Format(_T("%.2f"), g_wallRebarInfo.concrete.MissHoleSize);

	//for each (auto var in g_listSlabRebarMethod)
	//	m_ComRebarMethod.AddString(var);

	if (g_wallRebarInfo.concrete.isHandleHole)
	{
		m_hole_check.SetCheck(true);
		m_mholesize_edit.SetReadOnly(FALSE);
	}
	else
	{
		m_hole_check.SetCheck(false);
		m_mholesize_edit.SetReadOnly(TRUE);
	}
	m_EditLevel.SetWindowText(strLevel);
	m_EditPositive.SetWindowText(strPositive);
	m_EditReverse.SetWindowText(strReverse);
	m_EditSide.SetWindowText(strSide);
	m_mholesize_edit.SetWindowText(strMissHoleSize);

	// m_ComRebarMethod.SetCurSel(g_wallRebarInfo.concrete.m_SlabRebarMethod);//配筋方式

	if (COMPARE_VALUES_EPS(m_stCutRebarInfo.dCutLength1, 0.0, EPS) <= 0)
	{
		m_stCutRebarInfo.dCutLength1 = 3.95;
	}
	if (COMPARE_VALUES_EPS(m_stCutRebarInfo.dCutLength2, 0.0, EPS) <= 0)
	{
		m_stCutRebarInfo.dCutLength2 = 5.95;
	}
	if (COMPARE_VALUES_EPS(m_stCutRebarInfo.dCutLength3, 0.0, EPS) <= 0)
	{
		m_stCutRebarInfo.dCutLength3 = 8.95;
	}

	if (m_stCutRebarInfo.isCutRebar)
	{
		m_CheckIsCutRebar.SetCheck(1);
	}
	else
	{
		m_CheckIsCutRebar.SetCheck(0);
	}

	CString strTemp;
	strTemp.Format(_T("%.2f"), m_stCutRebarInfo.dCutLength1);
	m_EditCutLength1.SetWindowText(strTemp);

	strTemp.Format(_T("%.2f"), m_stCutRebarInfo.dCutLength2);
	m_EditCutLength2.SetWindowText(strTemp);

	strTemp.Format(_T("%.2f"), m_stCutRebarInfo.dCutLength3);
	m_EditCutLength3.SetWindowText(strTemp);

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
	m_listMainRebar.InsertColumn(2, _T("类型"), (int)(width / 9.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(3, _T("间距"), (int)(width / 9.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(4, _T("起点偏移"), (int)(width / 9.0*1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(5, _T("终点偏移"), (int)(width / 9.0*1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(6, _T("与前层间距"), (int)(width / 9.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(7, _T("位置"), (int)(width / 9.0*0.75), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

//	m_listMainRebar.SetShowProgressPercent(TRUE);
//	m_listMainRebar.SetSupportSort(TRUE);

//	SetListRowData(g_vecRebarDataCN);

	UpdateRebarList();
}

void SetLevelidByRebarData2(std::vector<PIT::ConcreteRebar>& vecRebarData)
{
	std::vector<PIT::ConcreteRebar> backdata;
	for (PIT::ConcreteRebar data : vecRebarData)
	{
		if (data.datachange == 2)
		{
			backdata.push_back(data);
		}
	}
	int frontid = 0;
	int midid = 0;
	int endid = backdata.size() + 1;
	for (PIT::ConcreteRebar& data : vecRebarData)
	{
		if (data.datachange == 0)
		{
			data.rebarLevel = frontid + 1;
			frontid = data.rebarLevel;
		}
		else if (data.datachange == 1)
		{
			data.rebarLevel = midid + 1;
			midid = data.rebarLevel;
		}
		else
		{
			data.rebarLevel = endid - 1;
			endid = data.rebarLevel;
		}
	}
}
void CWallMainRebarDlgNew::SetListDefaultData()
{
	if (g_wallRebarInfo.concrete.rebarLevelNum - (int)m_vecRebarData.size()!=0)
	{
		m_vecRebarData.clear();//添加了墙后位置后，添加此代码
	}
	if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
	{
		if (g_wallRebarInfo.concrete.rebarLevelNum>0)
		{
			    int midpos = g_wallRebarInfo.concrete.rebarLevelNum / 2;
				for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
				{
					PIT::ConcreteRebar oneRebarData;				
					if (i<midpos)//前半部分
					{
						int dir = (i) & 0x01;
						double levelSpace;
						levelSpace = 0;
						oneRebarData = { i+1,dir,"",0,200,0,0,levelSpace };
						oneRebarData.datachange = 0;
					}
					else//后半部分
					{
						int dir = (i+1) & 0x01;
						double levelSpace;
						levelSpace = 0;
						oneRebarData = { i,dir,"",2,200,0,0,levelSpace };
						oneRebarData.datachange = 0;
						oneRebarData.rebarLevel = g_wallRebarInfo.concrete.rebarLevelNum - i;
						oneRebarData.datachange = 2;
					}

					m_vecRebarData.push_back(oneRebarData);
				}
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
				levelSpace = dir * 2000.0;
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
	SetLevelidByRebarData2(m_vecRebarData);
	for (int i = 0; i < m_vecRebarData.size(); i++)
	{
		string tmpstring = CT2A(XmlManager::s_alltypes[m_vecRebarData.at(i).rebarType]);
		sprintf(m_vecRebarData.at(i).rebarSize, "%s", tmpstring);
	}
	g_vecRebarData = m_vecRebarData;
}


void CWallMainRebarDlgNew::getWallSetInfo(WallSetInfo& wallsetinfo)
{
	strcpy(m_WallsetInfo.rebarSize, wallsetinfo.rebarSize);
	m_WallsetInfo.rebarType = wallsetinfo.rebarType;
	m_WallsetInfo.spacing = wallsetinfo.spacing;
}

bool  CWallMainRebarDlgNew::ParsingLineDir(EditElementHandleR ehLine)
{
	DPoint3d ptStr, ptEnd;
	if (SUCCESS == mdlElmdscr_extractEndPoints(&ptStr, nullptr, &ptEnd, nullptr, ehLine.GetElementDescrP(), ehLine.GetModelRef()))
	{
		m_seg = DSegment3d::From(ptStr, ptEnd);
		return  true;
	}
	else
		return false;

}


bool  CWallMainRebarDlgNew::ParsingArcLineDir(EditElementHandleR ehLine)
{
	MSElementDescrP ArcMse = ehLine.GetElementDescrP();
	if (ArcMse == nullptr)
		return false;

	double starR, sweepR = 0.00;
	double radius = 0.00;
	DPoint3d ArcDPs[2] = {0};
	RotMatrix rotM;
	DPoint3d centerpt = {0};
	mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &ArcMse->el);
	double ArcLen;
	mdlElmdscr_distanceAtPoint(&ArcLen, nullptr, nullptr, ArcMse, &ArcDPs[1], 0.1);
	
	//将弧线的Z轴坐标移动到与板件底面Z坐标一致
	ArcDPs[0].z = m_height;
	mArcLine.ptStart = ArcDPs[0];
	ArcDPs[1].z = m_height;
	mArcLine.ptEnd = ArcDPs[1];
	centerpt.z = m_height;
	mArcLine.centerpt = centerpt;
	mArcLine.radius = radius;

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (ArcDPs[0].Distance(ArcDPs[1]) < uor_per_mm)
	{
		mArcLine.ArcLen = 2 * PI * radius;
	}
	else
	{
		mArcLine.ArcLen = ArcLen;
	}
	return true;
}

void CWallMainRebarDlgNew::UpdateRebarList()
{
	m_listMainRebar.DeleteAllItems();
	SetListDefaultData();
	for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
	{
		CString strValue;
		strValue.Format(_T("%dL"), m_vecRebarData[i].rebarLevel);
		m_listMainRebar.InsertItem(i, _T("")); // 插入行
		m_listMainRebar.SetItemText(i, 0, strValue);
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
				//之前为数据交换的功能
				/*ListCtrlEx::CStrList strlist;
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
				strValue = _T("无");*/
				ListCtrlEx::CStrList strlist = g_listRebarPose;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				if (m_vecRebarData[i].datachange>2)
				{
					m_vecRebarData[i].datachange = 0;
				}
				advance(it, m_vecRebarData[i].datachange);
				strValue = *it;
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

void CWallMainRebarDlgNew::GetConcreteData(PIT::Concrete& concreteData)
{
// 	CString strLevelNum,strPostiveCover,strSideCover,strReverseCover;
// 	m_EditLevel.GetWindowText(strLevelNum);
// 	concreteData.rebarLevelNum = atoi(CT2A(strLevelNum));
// 	concreteData.isTwinbars = m_ComboIsTwinBars.GetCurSel();
// 	m_EditPositive.GetWindowText(strPostiveCover);
// 	g_wallRebarInfo.concrete.postiveCover = atof(CT2A(strPostiveCover));
// 	m_EditSide.GetWindowText(strSideCover);
// 	g_wallRebarInfo.concrete.sideCover = atof(CT2A(strSideCover));
// 	m_EditReverse.GetWindowText(strReverseCover);
// 	g_wallRebarInfo.concrete.reverseCover = atof(CT2A(strReverseCover));
	concreteData = g_wallRebarInfo.concrete;
}

BOOL CWallMainRebarDlgNew::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	InitUIData();


	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CWallMainRebarDlgNew::OnEnKillfocusEdit1()
{
	// TODO: 在此添加控件通知处理程序代码

	CString strPostiveCover;
	m_EditPositive.GetWindowText(strPostiveCover);
	g_wallRebarInfo.concrete.postiveCover = atof(CT2A(strPostiveCover));
}


void CWallMainRebarDlgNew::OnEnKillfocusEdit2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strSideCover;
	m_EditSide.GetWindowText(strSideCover);
	g_wallRebarInfo.concrete.sideCover = atof(CT2A(strSideCover));
}


void CWallMainRebarDlgNew::OnEnKillfocusEdit3()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strReverseCover;
	m_EditReverse.GetWindowText(strReverseCover);
	g_wallRebarInfo.concrete.reverseCover = atof(CT2A(strReverseCover));
}


void CWallMainRebarDlgNew::OnEnKillfocusEdit4()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strLevelNum;
	m_EditLevel.GetWindowText(strLevelNum);
	g_wallRebarInfo.concrete.rebarLevelNum = atoi(CT2A(strLevelNum));
	UpdateRebarList();
	CTabCtrl* pTabCtrl = dynamic_cast<CTabCtrl*>(GetParent());
	if (pTabCtrl!=nullptr) 
	{
		CWallRebarDlgNew* pDlg = dynamic_cast<CWallRebarDlgNew*>(pTabCtrl->GetParent());
		if (pDlg)
		{
//			pDlg->m_PageLapOption.UpdateLapOptionList();
			pDlg->m_PageEndType.UpdateEndTypeList();
			pDlg->m_PageTwinBars.UpdateTwinBarsList();
		}
	}
}


LRESULT CWallMainRebarDlgNew::OnComboBoxDataChange(WPARAM wParam, LPARAM lParam)
{
	SetListRowData(g_vecRebarData);
	UpdateRebarList();
	return 0;
}

//关联构件
void CWallMainRebarDlgNew::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_assodlg == NULL)
		ScanAllElements(g_mapidAndmodel);

	if (m_ehSel.IsValid())
	{
		ElementId testid = 0;
		GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
		if (testid != 0)
		{
			m_listMainRebar.GetAllRebarData(g_vecRebarData);
			SetElementXAttribute(testid, g_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		}
	}

	vector<PIT::AssociatedComponent> vecAC;
	if (m_assodlg != NULL)
	{
		m_assodlg->DestroyWindow();
		m_assodlg->GetListRowData(vecAC);
		delete m_assodlg;
		m_assodlg = NULL;
//		vecAC = g_vecACData;
	}

	m_assodlg = new CWallRebarAssociatedComponentDlgNew(m_ehSel, this);
	if (vecAC.size())
	{
		m_assodlg->SetListRowData(vecAC);
	}
	m_assodlg->Create(IDD_DIALOG_WallRebar_AssociatedComponent, this);
	m_assodlg->ShowWindow(SW_NORMAL);
	m_assodlg->UpdateWindow();
}


void CWallMainRebarDlgNew::OnBnClickedHoleCheck()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_hole_check.GetCheck())
	{
		m_mholesize_edit.SetReadOnly(FALSE);
		g_wallRebarInfo.concrete.isHandleHole = 1;
		
	}
	else
	{
		m_mholesize_edit.SetReadOnly(TRUE);
		g_wallRebarInfo.concrete.isHandleHole = 0;
	}
}


void CWallMainRebarDlgNew::OnEnKillfocusMholesizeEdit()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strMissHoleSize;
	m_mholesize_edit.GetWindowText(strMissHoleSize);
	g_wallRebarInfo.concrete.MissHoleSize = atof(CT2A(strMissHoleSize));
}



void CWallMainRebarDlgNew::OnBnClickedButton2()
{
	CWallRebarDlgNew* dlg = dynamic_cast<CWallRebarDlgNew*>(pm_MainPageRebar);
	dlg->PreviewRebarLines();

}


//void CWallMainRebarDlgNew::OnCbnSelchangeCombo1()//配筋方式（板才有）
//{
//	// g_wallRebarInfo.concrete.m_SlabRebarMethod = m_ComRebarMethod.GetCurSel();
//}


void CWallMainRebarDlgNew::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_CheckIsCutRebar.GetCheck() == 0)
	{
		m_stCutRebarInfo.isCutRebar = false;
	}
	else
	{
		m_stCutRebarInfo.isCutRebar = true;
	}
}

void CWallMainRebarDlgNew::OnEnChangeEdit5()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString strTemp;
	m_EditCutLength1.GetWindowText(strTemp);
	m_stCutRebarInfo.dCutLength1 = atof(CT2A(strTemp));
}

void CWallMainRebarDlgNew::OnEnChangeEdit6()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString strTemp;
	m_EditCutLength2.GetWindowText(strTemp);
	m_stCutRebarInfo.dCutLength2 = atof(CT2A(strTemp));
}

void CWallMainRebarDlgNew::OnEnChangeEdit7()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString strTemp;
	m_EditCutLength3.GetWindowText(strTemp);
	m_stCutRebarInfo.dCutLength3 = atof(CT2A(strTemp));
}
