// CSlabMainRebarDlg.cpp: 实现文件
//
#include "_USTATION.h"
#include "CSlabMainRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include "CSlabRebarDlg.h"
#include "CSlabRebarAssociatedComponent.h"
#include "CommonFile.h"
#include "PITRebarCurve.h"
// CSlabMainRebarDlg 对话框
#define WM_UPDATELIST  WM_USER+1   // do something  
#define WM_LEVELCHANGE  WM_USER+2   // do something  

IMPLEMENT_DYNAMIC(CSlabMainRebarDlg, CDialogEx)

CSlabMainRebarDlg::CSlabMainRebarDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_SlabRebar_MainRebar, pParent)
{
	pm_MainPageRebar = NULL;
}

CSlabMainRebarDlg::~CSlabMainRebarDlg()
{
	if (pm_MainPageRebar != nullptr)
	{
		delete pm_MainPageRebar;
		pm_MainPageRebar = nullptr;
	}
}

void CSlabMainRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_S_LIST1, m_listMainRebar);
	DDX_Control(pDX, IDC_S_EDIT1, m_EditPositive);
	DDX_Control(pDX, IDC_S_EDIT2, m_EditSide);
	DDX_Control(pDX, IDC_S_EDIT3, m_EditReverse);
	DDX_Control(pDX, IDC_S_EDIT4, m_EditLevel);
	DDX_Control(pDX, IDC_S_HOLE_CHECK, m_hole_check);
	DDX_Control(pDX, IDC_S_MHOLESIZE_EDIT, m_mholesize_edit);
}


BEGIN_MESSAGE_MAP(CSlabMainRebarDlg, CDialogEx)
	ON_EN_KILLFOCUS(IDC_S_EDIT1, &CSlabMainRebarDlg::OnEnKillfocusEdit1)
	ON_EN_KILLFOCUS(IDC_S_EDIT2, &CSlabMainRebarDlg::OnEnKillfocusEdit2)
	ON_EN_KILLFOCUS(IDC_S_EDIT3, &CSlabMainRebarDlg::OnEnKillfocusEdit3)
	ON_EN_KILLFOCUS(IDC_S_EDIT4, &CSlabMainRebarDlg::OnEnKillfocusEdit4)
	ON_MESSAGE(WM_UPDATELIST, &CSlabMainRebarDlg::OnComboBoxDataChange)
	ON_BN_CLICKED(IDC_S_BUTTON1, &CSlabMainRebarDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_S_HOLE_CHECK, &CSlabMainRebarDlg::OnBnClickedHoleCheck)
	ON_EN_KILLFOCUS(IDC_S_MHOLESIZE_EDIT, &CSlabMainRebarDlg::OnEnKillfocusMholesizeEdit)
	ON_BN_CLICKED(IDC_BUTTON2, &CSlabMainRebarDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CSlabMainRebarDlg 消息处理程序
void CSlabMainRebarDlg::InitUIData()
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
		m_mholesize_edit.SetReadOnly(FALSE);
	}
	else
	{
		m_hole_check.SetCheck(false);
		m_mholesize_edit.SetReadOnly(TRUE);
	}
	m_EditLevel.SetWindowText(strLevel);                               //钢筋层数
	m_EditPositive.SetWindowText(strPositive);                          //顶部保护层
	m_EditReverse.SetWindowText(strReverse);                             //底部保护层
	m_EditSide.SetWindowText(strSide);                                 //侧面保护层
	m_mholesize_edit.SetWindowText(strMissHoleSize);

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
	m_listMainRebar.InsertColumn(0, _T("层"), (int)(width / 9 * 0.75), ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_listMainRebar.InsertColumn(1, _T("方向"), (int)(width / 9 * 0.75), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(2, _T("直径"), (int)(width / 9 * 0.75), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(3, _T("类型"), (int)(width / 9), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(4, _T("间距"), (int)(width / 9 * 1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(5, _T("起点偏移"), (int)(width / 9 * 1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(6, _T("终点偏移"), (int)(width / 9 * 1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(7, _T("与前层间距"), (int)(width / 9 * 1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(8, _T("位置"), (int)(width / 9 * 0.75), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

//	m_listMainRebar.SetShowProgressPercent(TRUE);
//	m_listMainRebar.SetSupportSort(TRUE);

	// SetListRowData(g_vecRebarData);                        //m_vecRebarData = vecListData;

	UpdateRebarList();
}
void CSlabMainRebarDlg::getSlabSetInfo(WallSetInfo& Slabsetinfo)
{
	strcpy(m_SlabsetInfo.rebarSize, Slabsetinfo.rebarSize);
	m_SlabsetInfo.rebarType = Slabsetinfo.rebarType;
	m_SlabsetInfo.spacing = Slabsetinfo.spacing;
}
void CSlabMainRebarDlg::SetListDefaultData()                   //添加钢筋的默认层的信息
{
	if (g_wallRebarInfo.concrete.rebarLevelNum - (int)m_vecRebarData.size() != 0)
	{
		m_vecRebarData.clear();//添加了墙后位置后，添加此代码
	}
	if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
	{
		if (g_wallRebarInfo.concrete.rebarLevelNum > 0)
		{
			int midpos = g_wallRebarInfo.concrete.rebarLevelNum / 2;
			for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
			{
				PIT::ConcreteRebar oneRebarData;
				if (i < midpos)//前半部分
				{
					int dir = (i) & 0x01;
					double levelSpace;
					levelSpace = 0;
					oneRebarData = { i + 1,dir,"12",2,200,0,0,levelSpace };
					oneRebarData.datachange = 0;
				}
				else//后半部分
				{
					int dir = (i + 1) & 0x01;
					double levelSpace;
					levelSpace = 0;
					oneRebarData = { i,dir,"12",2,200,0,0,levelSpace };
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
				PIT::ConcreteRebar oneRebarData = { i,dir,"12",2,200,0,0,levelSpace };
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
	PIT::SetLevelidByRebarData(m_vecRebarData);
	g_vecRebarData = m_vecRebarData;
}  

void CSlabMainRebarDlg::UpdateRebarList()                      //更新
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
				ListCtrlEx::CStrList strlist = g_listRebarSize;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				strValue = m_vecRebarData[i].rebarSize;
				strValue += _T("mm");
			}
			break;
			case 3:
			{
				ListCtrlEx::CStrList strlist = g_listRebarType;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecRebarData[i].rebarType);
				strValue = *it;
			}
			break;
			case 4:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].spacing);
				break;
			case 5:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].startOffset);
				break;
			case 6:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].endOffset);
				break;
			case 7:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].levelSpace);
				break;
			case 8:
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
				ListCtrlEx::CStrList strlist = g_listfloorRebarPose;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
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
}

void CSlabMainRebarDlg::GetConcreteData(PIT::Concrete& concreteData)
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

BOOL CSlabMainRebarDlg::OnInitDialog()                 //初始化
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	InitUIData();


	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CSlabMainRebarDlg::OnEnKillfocusEdit1()                                    //读取顶部保护层
{
	// TODO: 在此添加控件通知处理程序代码

	CString strPostiveCover;
	m_EditPositive.GetWindowText(strPostiveCover);
	g_wallRebarInfo.concrete.postiveCover = atof(CT2A(strPostiveCover));
}


void CSlabMainRebarDlg::OnEnKillfocusEdit2()                                     //读取侧面保护层
{
	// TODO: 在此添加控件通知处理程序代码
	CString strSideCover;
	m_EditSide.GetWindowText(strSideCover);
	g_wallRebarInfo.concrete.sideCover = atof(CT2A(strSideCover));
}


void CSlabMainRebarDlg::OnEnKillfocusEdit3()                                     //读取底部保护层
{
	// TODO: 在此添加控件通知处理程序代码
	CString strReverseCover;
	m_EditReverse.GetWindowText(strReverseCover);
	g_wallRebarInfo.concrete.reverseCover = atof(CT2A(strReverseCover));
}


void CSlabMainRebarDlg::OnEnKillfocusEdit4()                                    
{
	// TODO: 在此添加控件通知处理程序代码
	CString strLevelNum;
	m_EditLevel.GetWindowText(strLevelNum);                       //读取钢筋层数
	g_wallRebarInfo.concrete.rebarLevelNum = atoi(CT2A(strLevelNum));
	UpdateRebarList();
}


LRESULT CSlabMainRebarDlg::OnComboBoxDataChange(WPARAM wParam, LPARAM lParam)
{
	SetListRowData(g_vecRebarData);
	UpdateRebarList();
	return 0;
}

//关联构件
void CSlabMainRebarDlg::OnBnClickedButton1()
{
//	ScanAllElements();
	// TODO: 在此添加控件通知处理程序代码
	//if (m_assodlg != NULL)
	//{
	//	m_assodlg->Create(IDD_DIALOG_SlabRebar_AssociatedComponent, this);
	//	m_assodlg->ShowWindow(SW_SHOW);
	//}
}

                        





void CSlabMainRebarDlg::OnBnClickedHoleCheck()
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


void CSlabMainRebarDlg::OnEnKillfocusMholesizeEdit()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strMissHoleSize;
	m_mholesize_edit.GetWindowText(strMissHoleSize);
	g_wallRebarInfo.concrete.MissHoleSize = atof(CT2A(strMissHoleSize));
}


void CSlabMainRebarDlg::OnBnClickedButton2()//预览按钮
{
	pm_MainPageRebar->PreviewRebarLines();
	// TODO: 在此添加控件通知处理程序代码
}
