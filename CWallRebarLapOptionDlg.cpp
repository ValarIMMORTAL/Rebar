// CWallRebarLapOption.cpp: 实现文件
//

#include "_USTATION.h"
#include "CWallRebarLapOptionDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include "CommonFile.h"


// CWallRebarLapOption 对话框

IMPLEMENT_DYNAMIC(CWallRebarLapOptionDlg, CDialogEx)

CWallRebarLapOptionDlg::CWallRebarLapOptionDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_WallRebar_LapOption, pParent)
{

}

CWallRebarLapOptionDlg::~CWallRebarLapOptionDlg()
{
}

void CWallRebarLapOptionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LapOption, m_ListLapOption);
}

void CWallRebarLapOptionDlg::InitUIData()
{
	// TODO:  在此添加额外的初始化
	LONG lStyle;
	lStyle = GetWindowLong(m_ListLapOption.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_ListLapOption.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_ListLapOption.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_ListLapOption.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_ListLapOption.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_ListLapOption.InsertColumn(0, _T("层"), (int)(width / 7), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_ListLapOption.InsertColumn(1, _T("连接方式"), (int)(width / 7), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListLapOption.InsertColumn(2, _T("搭接长度"), (int)(width / 7), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListLapOption.InsertColumn(3, _T("库存长度"), (int)(width / 7), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListLapOption.InsertColumn(4, _T("加工长度"), (int)(width / 7), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListLapOption.InsertColumn(5, _T("是否交错"), (int)(width / 7), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListLapOption.InsertColumn(6, _T("交错长度"), (int)(width / 7), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

//	SetListRowData(g_vecLapOptionData);

	UpdateLapOptionList();
}

void CWallRebarLapOptionDlg::SetListDefaultData()
{
	if (m_vecLapOption.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; i++)
		{
			PIT::LapOptions oneEndTypeData = { i,0,0,12000,12000,0,0 };
			m_vecLapOption.push_back(oneEndTypeData);
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = g_wallRebarInfo.concrete.rebarLevelNum - (int)m_vecLapOption.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				PIT::LapOptions oneEndTypeData = { i,0,0,12000,12000,0,0 };
				m_vecLapOption.push_back(oneEndTypeData);
			}
		}
		if (iOffset < 0)
		{
			iOffset *= -1;
			for (int i = 0; i < iOffset; i++)
			{
				m_vecLapOption.pop_back();
			}
		}
	}
}

void CWallRebarLapOptionDlg::UpdateLapOptionList()
{
	m_ListLapOption.DeleteAllItems();
	SetListDefaultData();
	for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
	{
		if (0 == i)
		{
			m_ListLapOption.InsertItem(i, _T("")); // 插入行
			m_ListLapOption.SetItemText(i, 0, _T("1LX"));
		}
		else
		{
			CString strValue;
			strValue.Format(_T("%dL"), i);
			m_ListLapOption.InsertItem(i, _T("")); // 插入行
			m_ListLapOption.SetItemText(i, 0, strValue);
		}
		for (int j = 1; j < 7; ++j)
		{
			CString strValue;
			switch (j)
			{
			case 1:
			{
				ListCtrlEx::CStrList strlist = g_listRebarLapOption;
				m_ListLapOption.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecLapOption[i].connectMethod);
				strValue = *it;
			}
				break;
			case 2:
				strValue.Format(_T("%.2f"), m_vecLapOption[i].lapLength);
				break;
			case 3:
				strValue.Format(_T("%.2f"), m_vecLapOption[i].stockLength);
				break;
			case 4:
				strValue.Format(_T("%.2f"), m_vecLapOption[i].millLength);
				break;
			case 5:
			{
				ListCtrlEx::CStrList strlist = g_listNoYes;
				m_ListLapOption.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecLapOption[i].isStaggered);
				strValue = *it;
			}
				break;
			case 6:
				strValue.Format(_T("%.2f"), m_vecLapOption[i].staggeredLength);
				break;
			default:
				break;
			}
			m_ListLapOption.SetItemText(i, j, strValue);
		}
	}
//	m_ListLapOption.SetShowProgressPercent(TRUE);
//	m_ListLapOption.SetSupportSort(TRUE);
}


BEGIN_MESSAGE_MAP(CWallRebarLapOptionDlg, CDialogEx)
END_MESSAGE_MAP()


// CWallRebarLapOption 消息处理程序


BOOL CWallRebarLapOptionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	InitUIData();
	
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
