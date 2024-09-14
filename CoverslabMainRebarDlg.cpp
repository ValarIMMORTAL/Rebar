// CoverslabMainRebarDlg.cpp: 实现文件
//
#include "_USTATION.h"
#include "CoverslabMainRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "CoverslabRebarAssembly.h"
#include "ElementAttribute.h"


// CoverslabMainRebarDlg 对话框
IMPLEMENT_DYNAMIC(CoverslabMainRebarDlg, CDialogEx)

CoverslabMainRebarDlg::CoverslabMainRebarDlg(ElementHandleCR eh, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CoverslabRebar_MainRebar, pParent), m_ehSel(eh)
{
	m_concrete.postiveCover = 30;
	m_concrete.reverseCover = 30;
	m_concrete.sideCover = 30;
	m_CoverslabType = CoverslabRebarAssembly::JudgeSlabType(m_ehSel);

	switch (m_CoverslabType)
	{
	case CoverslabRebarAssembly::SICoverSlab:
		m_concrete.rebarLevelNum = 3;
		break;
	case CoverslabRebarAssembly::STCoverSlab:
		m_concrete.rebarLevelNum = 5;
		break;
	case CoverslabRebarAssembly::STCoverSlab_Ten:
		m_concrete.rebarLevelNum = 4;
		break;
	case CoverslabRebarAssembly::SZCoverSlab:
		m_concrete.rebarLevelNum = 7;
		break;
	case CoverslabRebarAssembly::OtherCoverSlab:
		m_concrete.rebarLevelNum = 0;
		return;
	default:
		break;
	}

	g_wallRebarInfo.concrete.rebarLevelNum = m_concrete.rebarLevelNum;
}

CoverslabMainRebarDlg::~CoverslabMainRebarDlg()
{
}

void CoverslabMainRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_C_EDIT1, m_EditPositive);
	DDX_Control(pDX, IDC_C_EDIT2, m_EditSide);
	DDX_Control(pDX, IDC_C_EDIT3, m_EditReverse);
	DDX_Control(pDX, IDC_C_EDIT4, m_EditLevel);
 	DDX_Control(pDX, IDC_C_LIST1, m_listMainRebar);
}


BEGIN_MESSAGE_MAP(CoverslabMainRebarDlg, CDialogEx)
	ON_EN_CHANGE(IDC_C_EDIT1, &CoverslabMainRebarDlg::OnEnChangeCEdit1)
	ON_EN_CHANGE(IDC_C_EDIT2, &CoverslabMainRebarDlg::OnEnChangeCEdit2)
	ON_EN_CHANGE(IDC_C_EDIT3, &CoverslabMainRebarDlg::OnEnChangeCEdit3)
	ON_EN_CHANGE(IDC_C_EDIT4, &CoverslabMainRebarDlg::OnEnChangeCEdit4)
END_MESSAGE_MAP()

void CoverslabMainRebarDlg::InitUIData()
{
	CString strLevel, strPositive, strSide, strReverse, strMissHoleSize;
	strLevel.Format(_T("%d"), m_concrete.rebarLevelNum);
	strPositive.Format(_T("%.2f"), m_concrete.postiveCover);
	strReverse.Format(_T("%.2f"), m_concrete.reverseCover);
	strSide.Format(_T("%.2f"), m_concrete.sideCover);
	strMissHoleSize.Format(_T("%.2f"), m_concrete.MissHoleSize);

	m_EditPositive.SetWindowText(strPositive);                          //顶部保护层
	m_EditReverse.SetWindowText(strReverse);                             //底部保护层
	m_EditSide.SetWindowText(strSide);                                 //侧面保护层

	m_EditLevel.SetWindowText(strLevel);                               //钢筋层数
	m_EditLevel.EnableWindow(FALSE);

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
	m_listMainRebar.InsertColumn(0, _T("层"),(int)(width / 8.0*0.75), ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_listMainRebar.InsertColumn(1, _T("方向"),(int)(width / 8.0*0.75), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(2, _T("直径"),(int)(width / 8.0*0.75), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(3, _T("类型"), (int)(width / 8.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(4, _T("间距"), (int)(width / 8.0*1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(5, _T("起点偏移"),(int)(width / 8.0*1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(6, _T("终点偏移"),(int)(width / 8.0*1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(7, _T("与前层间距"),(int)(width / 8.0*1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
//	m_listMainRebar.InsertColumn(8, _T("数据交换"),(int)(width / 9.0*0.75), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

	//	m_listMainRebar.SetShowProgressPercent(TRUE);
	//	m_listMainRebar.SetSupportSort(TRUE);

	// SetListRowData(g_vecRebarData);                        //m_vecRebarData = vecListData;

	UpdateRebarList();
}

void CoverslabMainRebarDlg::SetConcreteData(PIT::Concrete& concreteData)
{
	m_concrete = concreteData;
}

void CoverslabMainRebarDlg::GetConcreteData(PIT::Concrete& concreteData)
{
	// 	CString strLevelNum,strPostiveCover,strSideCover,strReverseCover;
	// 	m_EditLevel.GetWindowText(strLevelNum);
	// 	concreteData.rebarLevelNum = atoi(CT2A(strLevelNum));
	// 	concreteData.isTwinbars = m_ComboIsTwinBars.GetCurSel();
	// 	m_EditPositive.GetWindowText(strPostiveCover);
	// 	m_concrete.postiveCover = atof(CT2A(strPostiveCover));
	// 	m_EditSide.GetWindowText(strSideCover);
	// 	m_concrete.sideCover = atof(CT2A(strSideCover));
	// 	m_EditReverse.GetWindowText(strReverseCover);
	// 	m_concrete.reverseCover = atof(CT2A(strReverseCover));
	concreteData = m_concrete;
}

BOOL CoverslabMainRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	InitUIData();
	return TRUE;
}

void CoverslabMainRebarDlg::SetListDefaultData()                   //添加钢筋的默认层的信息
{
	if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < m_concrete.rebarLevelNum; ++i)
		{
			PIT::ConcreteRebar oneRebarData;
			oneRebarData = { i,0,"12",0,200,0,0,0 };

			if (m_CoverslabType == CoverslabRebarAssembly::SICoverSlab)
			{
				double dLevelSpace = 0.00;
				if (i == 2)
				{
					dLevelSpace = 2000.0;
				}

				oneRebarData = { i, 1, "12", 0, 200, 50, 50, dLevelSpace };
				if (0 == i)
				{
					oneRebarData = { i,0,"12",0,200,0,0,0 };
				}
			}
			else if (m_CoverslabType == CoverslabRebarAssembly::STCoverSlab_Ten)
			{
				if (i == 0)
				{
					oneRebarData = { i,1,"12",0,200,0,0,0 };
				}
				else if (i == 1)
				{
					oneRebarData = { i,0,"12",0,200,0,0,0 };
				}
				else if (i == 2)
				{
					oneRebarData = { i,1,"12",0,200,0,0,2000.0 };
				}
				else if (i == 3)
				{
					oneRebarData = { i,0,"12",0,200,0,0,0 };
				}
			}
			else if (m_CoverslabType == CoverslabRebarAssembly::STCoverSlab)
			{
				if (i == 0)
				{
					oneRebarData = { i,1,"12",0,200,0,0,0 };
				}
				else if (i == 1)
				{
					oneRebarData = { i,0,"12",0,200,0,0,0 };
				}
				else if (i == 2)
				{
					oneRebarData = { i,0,"12",0,200,0,0,2000.0 };
				}
				else if (i == 3)
				{
					oneRebarData = { i,0,"12",0,200,0,0,0 };
				}
				else if (i == 4)
				{
					oneRebarData = { i,1,"12",0,200,0,0,0 };
				}
			}
			else if (m_CoverslabType == CoverslabRebarAssembly::SZCoverSlab)
			{
				if (i == 0)
				{
					oneRebarData = { i,1,"8",0,200,0,0,0 };
				}
				else if (i == 1)
				{
					oneRebarData = { i,1,"8",0,200,0,0,0 };
				}
				else if (i == 2)
				{
					oneRebarData = { i,0,"8",0,200,0,0,0 };
				}
				else if (i == 3)
				{
					oneRebarData = { i,0,"8",0,200,0,0,0 };
				}
				else if (i == 4)
				{
					oneRebarData = { i,0,"8",0,200,0,0,2000.0 };
				}
				else if (i == 5)
				{
					oneRebarData = { i,0,"8",0,200,0,0,0 };
				}
				else if (i == 6)
				{
					oneRebarData = { i,1,"8",0,200,0,0,0 };
				}
			}
			m_vecRebarData.push_back(oneRebarData);
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = m_concrete.rebarLevelNum - (int)m_vecRebarData.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				int dir = i & 0x01;
				double levelSpace;
				levelSpace = dir * 2000.0;
				ConcreteRebar oneRebarData = { i,dir,"12",0,200,0,0,levelSpace };
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

// CoverslabMainRebarDlg 消息处理程序
void CoverslabMainRebarDlg::UpdateRebarList()                      //更新
{
	m_listMainRebar.DeleteAllItems();
	SetListDefaultData();
	for (int i = 0; i < m_concrete.rebarLevelNum; ++i)
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
// 				ListCtrlEx::CStrList strlist = g_listRebarPose;
// 				m_listMainRebar.SetCellStringList(i, j, strlist);
// 				auto it = strlist.begin();
// 				if (m_vecRebarData[i].datachange > 2)
// 				{
// 					m_vecRebarData[i].datachange = 0;
// 				}
// 				advance(it, m_vecRebarData[i].datachange);
// 				strValue = *it;
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

void CoverslabMainRebarDlg::OnEnChangeCEdit1()
{
	CString strPostiveCover;
	m_EditPositive.GetWindowText(strPostiveCover);
	m_concrete.postiveCover = atof(CT2A(strPostiveCover));
}


void CoverslabMainRebarDlg::OnEnChangeCEdit2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strSideCover;
	m_EditSide.GetWindowText(strSideCover);
	m_concrete.sideCover = atof(CT2A(strSideCover));
}


void CoverslabMainRebarDlg::OnEnChangeCEdit3()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strReverseCover;
	m_EditReverse.GetWindowText(strReverseCover);
	m_concrete.reverseCover = atof(CT2A(strReverseCover));
}

void CoverslabMainRebarDlg::OnEnChangeCEdit4()
{
	// TODO: 在此添加控件通知处理程序代码
	//CString strLevelNum;
	//m_EditLevel.GetWindowText(strLevelNum);                       //读取钢筋层数
	//m_concrete.rebarLevelNum = atoi(CT2A(strLevelNum));
	//g_wallRebarInfo.concrete.rebarLevelNum = m_concrete.rebarLevelNum;
	//UpdateRebarList();
	//CTabCtrl* pTabCtrl = (CTabCtrl*)GetParent();
//	if (pTabCtrl)
//	{
//		CoverslabRebarDlg* pDlg = (CoverslabRebarDlg*)pTabCtrl->GetParent();
//		if (pDlg)
//		{
//			//			pDlg->m_PageLapOption.UpdateLapOptionList();
//			pDlg->m_PageEndType.UpdateEndTypeList();
//			pDlg->m_PageTwinBars.UpdateTwinBarsList();
//		}
//	}
}
