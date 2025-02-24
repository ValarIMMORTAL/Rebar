// CTwinBarSet.cpp: 实现文件
//

#include "_USTATION.h"
#include "CTwinBarSetDlgNew.h"
#include "afxdialogex.h"
#include "../../resource.h"
#include "../../ConstantsDef.h"
#include "CWallRebarDlgNew.h"
#include "../../CommonFile.h"

// CTwinBarSet 对话框

IMPLEMENT_DYNAMIC(CTwinBarSetDlgNew, CDialogEx)

CTwinBarSetDlgNew::CTwinBarSetDlgNew(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CNWallRebar_TwinBarSet, pParent)
{

}

CTwinBarSetDlgNew::~CTwinBarSetDlgNew()
{
}

void CTwinBarSetDlgNew::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
//	DDX_Control(pDX, IDC_CHECK_1, m_CheckIsTwinBars);
//	DDX_Control(pDX, IDC_CHECK_2, m_CheckIsStaggered);
	DDX_Control(pDX, IDC_LIST_TwinBarSet, m_ListTwinBars);
}


BEGIN_MESSAGE_MAP(CTwinBarSetDlgNew, CDialogEx)
//	ON_BN_KILLFOCUS(IDC_CHECK_1, &CTwinBarSetDlg::OnBnKillfocusCheck1)
//	ON_BN_KILLFOCUS(IDC_CHECK_2, &CTwinBarSetDlg::OnBnKillfocusCheck2)
//	ON_BN_CLICKED(IDC_CHECK_1, &CTwinBarSetDlg::OnBnClickedCheck1)
//	ON_BN_CLICKED(IDC_CHECK_2, &CTwinBarSetDlg::OnBnClickedCheck2)
END_MESSAGE_MAP()


// CTwinBarSet 消息处理程序
void CTwinBarSetDlgNew::InitUIData()
{
//	m_CheckIsTwinBars.SetCheck(g_twinBarInfo.isTwinbars);
//	m_CheckIsStaggered.SetCheck(g_twinBarInfo.isStaggered);

	LONG lStyle;
	lStyle = GetWindowLong(m_ListTwinBars.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_ListTwinBars.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_ListTwinBars.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_CHECKBOXES;
	m_ListTwinBars.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);


	tagRECT stRect;
	m_ListTwinBars.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_ListTwinBars.InsertColumn(0, _T("序号"), (int)(width /7.0*0.75), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_ListTwinBars.InsertColumn(1, _T("钢筋层名称"), (int)(width / 7.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListTwinBars.InsertColumn(2, _T("并筋层名称"), (int)(width / 7.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListTwinBars.InsertColumn(3, _T("并筋"), (int)(width / 7.0*0.75), ListCtrlEx::CheckBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListTwinBars.InsertColumn(4, _T("并筋直径"), (int)(width / 7.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListTwinBars.InsertColumn(5, _T("并筋级别"), (int)(width / 7.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListTwinBars.InsertColumn(6, _T("并筋间距（X*主筋间距）"), (int)(width / 7.0*1.5), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

	//	m_ListEndType.InsertColumn(5, _T("清空"), LVCFMT_CENTER, 100);

//	SetListRowData(g_vecTwinBarData);

	UpdateTwinBarsList();
}

void CTwinBarSetDlgNew::SetListDefaultData()
{
	if (m_vecTwinBar.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < m_vecRebarData.size(); i++)
		{
			CString strTwinBar = GetTwinBarLevelName(i);
			TwinBarSet::TwinBarLevelInfo oneTwinBarData;
			strcpy(oneTwinBarData.levelName, CT2A(strTwinBar.GetBuffer()));
			oneTwinBarData.hasTwinbars = 0;
			
			oneTwinBarData.rebarType = m_vecRebarData[i].rebarType;
			oneTwinBarData.interval = 0;
			m_vecTwinBar.push_back(oneTwinBarData);
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = (int)m_vecRebarData.size() - (int)m_vecTwinBar.size();
		if (iOffset > 0)
		{
			int iOldSize = (int)m_vecTwinBar.size();
			for (int i = 0; i < iOffset; i++)
			{
				CString strTwinBar = GetTwinBarLevelName((int)m_vecTwinBar.size());
				TwinBarSet::TwinBarLevelInfo oneTwinBarData;
				strcpy(oneTwinBarData.levelName, CT2A(strTwinBar.GetBuffer()));
				oneTwinBarData.hasTwinbars = 0;
				oneTwinBarData.rebarType = m_vecRebarData[iOldSize +i].rebarType;
				oneTwinBarData.interval = 0;
				m_vecTwinBar.push_back(oneTwinBarData);
			}
		}
		if (iOffset < 0)
		{
			iOffset *= -1;
			for (int i = 0; i < iOffset; i++)
			{
				m_vecTwinBar.pop_back();
			}
		}
	}
}

void CTwinBarSetDlgNew::UpdateTwinBarsList()
{
	m_ListTwinBars.DeleteAllItems();
	SetListDefaultData();
	CString strValue;
	for (int i = 0; i < (int)m_vecTwinBar.size(); ++i)
	{
		strValue.Format(_T("%d"), i);
		m_ListTwinBars.InsertItem(i, _T("")); // 插入行
		m_ListTwinBars.SetItemText(i, 0, strValue);
		int nSubCnt = m_ListTwinBars.GetColumnCount() - 1;
		for (int j = 1; j <= nSubCnt; ++j)
		{
			switch (j)
			{
			case 1:
			{
				strValue = GetTwinBarLevelName(i);
				m_ListTwinBars.SetItemText(i, j, strValue);
			}	
			break;
			case 2:
			{
				strValue = m_vecTwinBar[i].levelName;
				m_ListTwinBars.SetItemText(i, j, strValue);
			}
			break;
			case 3:
			{
				m_ListTwinBars.SetCellChecked(i,j,m_vecTwinBar[i].hasTwinbars);
			}
			break;
			case 4:
			{
				ListCtrlEx::CStrList strlist = g_listRebarSize;
				m_ListTwinBars.SetCellStringList(i, j, strlist);
				strValue = m_vecTwinBar[i].rebarSize;
				strValue += _T("mm");
				m_ListTwinBars.SetItemText(i, j, strValue);
			}
			break;
			case 5:
			{
				ListCtrlEx::CStrList strlist = g_listRebarType;
				m_ListTwinBars.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecTwinBar[i].rebarType);
				strValue = *it;
				m_ListTwinBars.SetItemText(i, j, strValue);
			}
			break;
			case 6:
			{
				strValue.Format(_T("%d"), m_vecTwinBar[i].interval);
				m_ListTwinBars.SetItemText(i, j, strValue);
			}
			break;
			default:
				break;
			}
		}

	}
//	m_ListTwinBars.SetShowProgressPercent(TRUE);
//	m_ListTwinBars.SetSupportSort(TRUE);
}

BOOL CTwinBarSetDlgNew::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	InitUIData();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

CString CTwinBarSetDlgNew::GetTwinBarLevelName(int rebarLevel)
{
	CString strValue;
	if (m_vecRebarData.size() <= rebarLevel)
		return _T("");
	if (m_vecRebarData.size() == 4)
	{
		if (0 == rebarLevel || 1 == rebarLevel)
		{
			if (0 == m_vecRebarData[rebarLevel].rebarDir)
				strValue = _T("1H");
			else
				strValue = _T("1V");
		}
		else
		{
			if (0 == m_vecRebarData[rebarLevel].rebarDir)
				strValue = _T("2H");
			else
				strValue = _T("2V");
		}
	}
	else
	{
		if (0 == rebarLevel || 1 == rebarLevel)
		{
			if (0 == m_vecRebarData[rebarLevel].rebarDir)
				strValue = _T("1H");
			else
				strValue = _T("1V");
		}
		else
		{
			CString strExtend;
			if (0 == m_vecRebarData[rebarLevel].rebarDir)
				strExtend = _T("H");
			else
				strExtend = _T("V");
			int iMod = rebarLevel / 2;
			int iCenterLevel = g_wallRebarInfo.concrete.rebarLevelNum / 2;
			if (iMod < iCenterLevel)
			{
				strValue.Format(_T("%d"), iMod + 2);
			}
			else if (iMod == iCenterLevel)
			{
				int iNum = iMod + 2;
				if (!(rebarLevel&0x01))
					iNum ++;
				strValue.Format(_T("%d"), iNum);
			}
			else
			{
				iMod = ((int)m_vecRebarData.size() - rebarLevel) / 2;
				strValue.Format(_T("%d"), iMod + 2);
			}
			strValue += strExtend;
		}
	}

// 	CString strValue;
// 	if (g_vecRebarData.size() <= rebarLevel)
// 		return _T("");
// 	if (g_vecRebarData.size() == 4)
// 	{
// 		if (0 == rebarLevel || 1 == rebarLevel)
// 		{
// 			if (0 == g_vecRebarData[rebarLevel].rebarDir)
// 				strValue = _T("1H");
// 			else
// 				strValue = _T("1V");
// 		}
// 		else
// 		{
// 			if (0 == g_vecRebarData[rebarLevel].rebarDir)
// 				strValue = _T("2H");
// 			else
// 				strValue = _T("2V");
// 		}
// 	}
// 	else
// 	{
// 		if (0 == rebarLevel || 1 == rebarLevel)
// 		{
// 			if (0 == g_vecRebarData[rebarLevel].rebarDir)
// 				strValue = _T("1H");
// 			else
// 				strValue = _T("1V");
// 		}
// 		else
// 		{
// 			CString strExtend;
// 			if (0 == g_vecRebarData[rebarLevel].rebarDir)
// 				strExtend = _T("H");
// 			else
// 				strExtend = _T("V");
// 			int iMod = rebarLevel / 2;
// 			int iCenterLevel = g_wallRebarInfo.concrete.rebarLevelNum / 2;
// 			if (iMod <= iCenterLevel)
// 			{
// 				strValue.Format(_T("%d"), iMod + 2);
// 			}
// 			else
// 			{
// 				iMod = ((int)g_vecRebarData.size() - rebarLevel) / 2;
// 				strValue.Format(_T("%d"), iMod + 2);
// 			}
// 			strValue += strExtend;
// 		}
// 	}
	
	return strValue;
}

// void CTwinBarSetDlg::GetTwinBarInfo(TwinBarSet::TwinBarInfo & twInfo)
// {
// 	twInfo.isTwinbars = g_twinBarInfo.isTwinbars;
// 	twInfo.isStaggered = g_twinBarInfo.isStaggered;
// }

//是否为并筋
// void CTwinBarSetDlg::OnBnKillfocusCheck1()
// {
// 	// TODO: 在此添加控件通知处理程序代码
// 	g_twinBarInfo.isTwinbars = m_CheckIsTwinBars.GetCheck();
// }
// 
// //是否交错
// void CTwinBarSetDlg::OnBnKillfocusCheck2()
// {
// 	// TODO: 在此添加控件通知处理程序代码
// 	g_twinBarInfo.isStaggered = m_CheckIsStaggered.GetCheck();
// }


// void CTwinBarSetDlg::OnBnClickedCheck1()
// {
// 	// TODO: 在此添加控件通知处理程序代码
// 	g_twinBarInfo.isTwinbars = m_CheckIsTwinBars.GetCheck();
// }
// 
// 
// void CTwinBarSetDlg::OnBnClickedCheck2()
// {
// 	// TODO: 在此添加控件通知处理程序代码
// 	g_twinBarInfo.isStaggered = m_CheckIsStaggered.GetCheck();
// }
