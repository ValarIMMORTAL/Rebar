// D:\work\02代码\CGNdllCode\GalleryIntelligentRebar\CDomeDetailDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CDomeDetailDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ElementAttribute.h"
#include "ConstantsDef.h"

// CDomeDetailDlg 对话框

IMPLEMENT_DYNAMIC(CDomeDetailDlg, CDialogEx)

CDomeDetailDlg::CDomeDetailDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_DomeDetail, pParent)
{
	m_LevelNum = 4;
	m_LayoutType = 0;
}

CDomeDetailDlg::~CDomeDetailDlg()
{
}

BOOL CDomeDetailDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端

	LONG lStyle;
	lStyle = GetWindowLong(m_listCtl.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_listCtl.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_listCtl.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_listCtl.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_listCtl.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_listCtl.InsertColumn(0, _T("层"), (int)(width / 7) * 0.7, ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_listCtl.InsertColumn(1, _T("形状"), (int)(width / 7) * 0.85, ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listCtl.InsertColumn(2, _T("直径"), (int)(width / 7), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listCtl.InsertColumn(3, _T("类型"), (int)(width / 7), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listCtl.InsertColumn(4, _T("角度"), (int)(width / 7), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listCtl.InsertColumn(5, _T("与前层间距"), (int)(width / 7), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listCtl.InsertColumn(6, _T("搭接偏移(径向)"), (int)(width / 7) * 1.45, ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

	m_LevelNum = (int)m_vecDomeLevelDetailInfo.size();
	UpdateRebarList();

	CString strTmp = CString();
	strTmp.Format(_T("%d"), m_LevelNum);
	m_EditLevelNum.SetWindowText(strTmp);

	if (m_LayoutType == 0)
	{
		this->SetWindowText(_T("XY正交"));
	}
	else if (m_LayoutType == 1)
	{
		this->SetWindowText(_T("环径正交"));
	}

	return true;
}

void CDomeDetailDlg::SetListDefaultData()
{
	if (m_vecDomeLevelDetailInfo.empty())//无数据时根据层数添加默认数据
	{
		m_LevelNum = 4;
		for (int i = 0; i < m_LevelNum; ++i)
		{
			PIT::DomeLevelDetailInfo  stDomeLevelInfo = { i, 0, i & 0x1, "12mm", 0, 0.5, 0.0, 0.0};
			if (m_LayoutType == 0)
			{
				stDomeLevelInfo.dAngleOrSpace = 3.0;
			}
			m_vecDomeLevelDetailInfo.push_back(stDomeLevelInfo);
		}
	}
	if (m_LevelNum > m_vecDomeLevelDetailInfo.size())
	{
		int nNumber = m_vecDomeLevelDetailInfo.at(0).nNumber;
		for (int i = m_vecDomeLevelDetailInfo.size(); i < m_LevelNum; i++)
		{
			PIT::DomeLevelDetailInfo  stDomeLevelInfo = { i, nNumber, i & 0x1, "12mm", 0, 0.5, 0.0, 0.0};
			if (m_LayoutType == 0)
			{
				stDomeLevelInfo.dAngleOrSpace = 3.0;
			}
			m_vecDomeLevelDetailInfo.push_back(stDomeLevelInfo);
		}
	}
	else if (m_LevelNum < m_vecDomeLevelDetailInfo.size())
	{
		int nSize = (int)m_vecDomeLevelDetailInfo.size();
		for (int i = m_LevelNum; i < nSize; i++)
		{
			m_vecDomeLevelDetailInfo.pop_back();
		}
	}
	return ;
}


void CDomeDetailDlg::UpdateRebarList()
{
	m_listCtl.DeleteAllItems();
	SetListDefaultData();

	m_LevelNum = (int)m_vecDomeLevelDetailInfo.size();
	for (int i = 0; i < m_LevelNum; ++i)
	{
		CString strValue;
		strValue.Format(_T("%dL"), m_vecDomeLevelDetailInfo[i].nLevel + 1);
		m_listCtl.InsertItem(i, _T("")); // 插入行
		m_listCtl.SetItemText(i, 0, strValue);
		int nSubCnt = m_listCtl.GetColumnCount() - 1;

		for (int j = 1; j <= nSubCnt; ++j)
		{
			CString strValue;
			switch (j)
			{
			case 1:
			{
				if (m_LayoutType == 0)
				{
					ListCtrlEx::CStrList strlist = g_listDomeRebarDirXY;
					m_listCtl.SetCellStringList(i, j, strlist);
					auto it = strlist.begin();
					advance(it, m_vecDomeLevelDetailInfo[i].rebarShape);
					strValue = *it;
				}
				else
				{
					ListCtrlEx::CStrList strlist = g_listDomeRebarDir;
					m_listCtl.SetCellStringList(i, j, strlist);
					auto it = strlist.begin();
					advance(it, m_vecDomeLevelDetailInfo[i].rebarShape);
					strValue = *it;
				}

			}
			break;
			case 2:
			{
				ListCtrlEx::CStrList strlist = g_listRebarSize;
				m_listCtl.SetCellStringList(i, j, strlist);
				strValue = m_vecDomeLevelDetailInfo[i].rebarSize;
				strValue += _T("mm");
			}
			break;
			case 3:
			{
				ListCtrlEx::CStrList strlist = g_listRebarType;
				m_listCtl.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecDomeLevelDetailInfo[i].rebarType);
				strValue = *it;
			}
			break;
			case 4:
				strValue.Format(_T("%.2f"), m_vecDomeLevelDetailInfo[i].dAngleOrSpace);
				break;
			case 5:
				strValue.Format(_T("%.2f"), m_vecDomeLevelDetailInfo[i].dSpacing);
				break;
			case 6:
				strValue.Format(_T("%.2f"), m_vecDomeLevelDetailInfo[i].dStartOffset);
				break;
			default:
				break;
			}
			m_listCtl.SetItemText(i, j, strValue);
		}

	}
	//	m_listMainRebar.SetShowProgressPercent(TRUE);
	//	m_listMainRebar.SetSupportSort(TRUE);
}

void CDomeDetailDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST1, m_listCtl);
	DDX_Control(pDX, IDC_EDIT1, m_EditLevelNum);
}


BEGIN_MESSAGE_MAP(CDomeDetailDlg, CDialogEx)
	ON_EN_CHANGE(IDC_EDIT1, &CDomeDetailDlg::OnEnChangeEdit1)
	ON_EN_KILLFOCUS(IDC_EDIT1, &CDomeDetailDlg::OnEnKillfocusEdit1)
END_MESSAGE_MAP()


// CDomeDetailDlg 消息处理程序


void CDomeDetailDlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}

void CDomeDetailDlg::OnEnKillfocusEdit1()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strLevelNum;
	m_EditLevelNum.GetWindowText(strLevelNum);
	m_LevelNum = atoi(CT2A(strLevelNum));
	if (m_LevelNum <= 0)
	{
		m_LevelNum = 2;
	}

	UpdateRebarList();
}
