// EditListCtrl.cpp : implementation file  
//  

#include "_ustation.h"
#include "CEditListCtrl.h"  
#include "GalleryIntelligentRebarids.h"
#include "ConstantsDef.h"

// CEditListCtrl  
#define WM_UPDATELIST  WM_USER+1   // do something  

extern std::vector<PIT::ConcreteRebar> g_vecRebarData;

IMPLEMENT_DYNAMIC(CEditListCtrl, CListCtrl)

CEditListCtrl::CEditListCtrl()
{

}

CEditListCtrl::~CEditListCtrl()
{
}


BEGIN_MESSAGE_MAP(CEditListCtrl, CListCtrl)
	ON_WM_LBUTTONDBLCLK()
	ON_CBN_SELCHANGE(IDC_CELL_COMBOBOX, &CEditListCtrl::OnCbnSelchangeCbCellComboBox)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
END_MESSAGE_MAP()



// CEditListCtrl message handlers  

// 当编辑完成后同步数据  
void CEditListCtrl::DisposeEdit(void)
{
	int nIndex = GetSelectionMark();
	//同步更新处理数据  
	CString sLable;

	if (0 == m_nCol) //edit控件  
	{
		CString szLable;
		m_edit.GetWindowText(szLable);
		SetItemText(m_nRow, m_nCol, szLable);

		m_edit.ShowWindow(SW_HIDE);
	}
	else if ((1 == m_nCol) || (2 == m_nCol) || (8 == m_nCol))
	{
		m_comboBox.ShowWindow(SW_HIDE);
	}
}

// 设置当前窗口的风格  
void CEditListCtrl::SetStyle(void)
{
	LONG lStyle;
	lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK; //清除显示方式  
	lStyle |= LVS_REPORT; //list模式  
	lStyle |= LVS_SINGLESEL; //单选  
	SetWindowLong(m_hWnd, GWL_STYLE, lStyle);

	//扩展模式  
	DWORD dwStyle = GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT; //选中某行使其整行高亮  
	dwStyle |= LVS_EX_GRIDLINES; //网格线  
	SetExtendedStyle(dwStyle); //设置扩展风格  
}

void CEditListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default  
	CListCtrl::OnLButtonDblClk(nFlags, point);
}


void CEditListCtrl::OnCbnSelchangeCbCellComboBox()
{
	//同步更新处理数据  
// 	_PersistItem* pPersistItem = (_PersistItem*)GetItemData(m_nRow);
// 	if (NULL == pPersistItem) return;

	if (1 == m_nCol || 2 == m_nRow || 3 == m_nCol || 8 == m_nRow)
	{
		CString szLable;
		int nindex = m_comboBox.GetCurSel();
		m_comboBox.GetLBText(nindex, szLable);
		SetItemText(m_nRow, m_nCol, szLable);
		m_comboBox.ShowWindow(SW_HIDE);
	}
}

//发送失效消息  
void CEditListCtrl::SendInvalidateMsg()
{
	//Edit单元格  
	if ((0 == m_nCol))
	{

		if (NULL == m_edit.m_hWnd) return;
		BOOL bShow = m_edit.IsWindowVisible();
		if (bShow)
			::SendMessage(m_edit.m_hWnd, WM_KILLFOCUS, (WPARAM)0, (LPARAM)0);

	}
	else if ((1 == m_nCol)) //combobox  
	{
		if (NULL == m_comboBox.m_hWnd) return;

		BOOL bShow = m_comboBox.IsWindowVisible();
		if (bShow)
			::SendMessage(m_comboBox.m_hWnd, WM_KILLFOCUS, (WPARAM)0, (LPARAM)0);
	}
}

void CEditListCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SendInvalidateMsg();

	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CEditListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SendInvalidateMsg();

	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}


IMPLEMENT_DYNAMIC(CWallRebarEditListCtrl, CListCtrl)
BEGIN_MESSAGE_MAP(CWallRebarEditListCtrl, CListCtrl)
	ON_WM_LBUTTONDBLCLK()
	ON_CBN_SELCHANGE(IDC_CELL_COMBOBOX, &CWallRebarEditListCtrl::OnCbnSelchangeCbCellComboBox)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
// 	ON_WM_LBUTTONDOWN()
// 	ON_WM_RBUTTONDOWN()
// 	ON_WM_MBUTTONDOWN()
END_MESSAGE_MAP()

void CWallRebarEditListCtrl::GetAllRebarData(std::vector<PIT::ConcreteRebar>& vecListData)
{
	vecListData.clear();
	for (int i = 0; i < GetItemCount(); ++i)
	{
		PIT::ConcreteRebar oneRebarInfo;
		for (int j = 0; j < 8; ++j)
		{
			switch (j)
			{
			case 0:
				oneRebarInfo.rebarLevel = i;
				break;
			case 1:
			{
				CString strCellValue = GetItemText(i, j);
				auto find = std::find(g_vecRebarDir.begin(), g_vecRebarDir.end(), strCellValue);
				int nIndex = (int)std::distance(g_vecRebarDir.begin(), find);
				oneRebarInfo.rebarDir = nIndex;
			}
				break;
			case 2:
			{
				CString strRebarSize = GetItemText(i, j);
				strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);	//删掉mm
				strcpy(oneRebarInfo.rebarSize, CT2A(strRebarSize.GetBuffer()));
				break;
			}
			case 3:
				oneRebarInfo.rebarType = atoi(CT2A(GetItemText(i, j)));
				break;
			case 4:
				oneRebarInfo.spacing = atof(CT2A(GetItemText(i, j)));
				break;
			case 5:
				oneRebarInfo.startOffset = atof(CT2A(GetItemText(i, j)));
				break;
			case 6:
				oneRebarInfo.endOffset = atof(CT2A(GetItemText(i, j)));
				break;
			case 7:
				oneRebarInfo.levelSpace = atof(CT2A(GetItemText(i, j)));
				break;
			case 8:
				oneRebarInfo.datachange = 0;
				break;
			}
		}
		vecListData.push_back(oneRebarInfo);
	}
}
void CWallRebarEditListCtrl::DisposeEdit(void)
{
	int nIndex = GetSelectionMark();
	//同步更新处理数据  
	CString sLable;
// 	WallRebarInfo::WallRebar* pWallRebarData = (WallRebarInfo::WallRebar*)GetItemData(m_nRow);
// 	if (NULL == pWallRebarData) return;

	if (4 == m_nCol || 5 == m_nCol || 6 == m_nCol || 7 == m_nCol)
	{
		CString szLable;
		m_edit.GetWindowText(szLable);
		SetItemText(m_nRow, m_nCol, szLable);
		m_edit.ShowWindow(SW_HIDE);
	}
	if (1 == m_nCol || 2 == m_nCol || 3 == m_nCol || 8 == m_nCol)
	{
		m_comboBox.ShowWindow(SW_HIDE);
	}
	if (8 == m_nCol)
	{
		int select = m_comboBox.GetCurSel();
		GetAllRebarData(g_vecRebarData);
		WallRebarInfo::WallRebar rebarData;
		rebarData = g_vecRebarData[m_nRow];
		g_vecRebarData[m_nRow] = g_vecRebarData[select];
		g_vecRebarData[select] = rebarData;
		GetParent()->SendMessage(WM_UPDATELIST);
	}
}

// 设置当前窗口的风格  
void CWallRebarEditListCtrl::SetStyle(void)
{
	LONG lStyle;
	lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK; //清除显示方式  
	lStyle |= LVS_REPORT; //list模式  
	lStyle |= LVS_SINGLESEL; //单选  
	SetWindowLong(m_hWnd, GWL_STYLE, lStyle);

	//扩展模式  
	DWORD dwStyle = GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT; //选中某行使其整行高亮  
	dwStyle |= LVS_EX_GRIDLINES; //网格线  
	SetExtendedStyle(dwStyle); //设置扩展风格  
}

void CWallRebarEditListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default  
	CListCtrl::OnLButtonDblClk(nFlags, point);

	LVHITTESTINFO info;
	info.pt = point;
	info.flags = LVHT_ONITEMLABEL;

	if (SubItemHitTest(&info) >= 0)
	{
		m_nRow = info.iItem;
		m_nCol = info.iSubItem;

		CRect rect;
		GetSubItemRect(m_nRow, m_nCol, LVIR_LABEL, rect);

		CString strValue;
		strValue = GetItemText(m_nRow, m_nCol);

		if(4 == m_nCol || 5 == m_nCol || 6 == m_nCol || 7 == m_nCol)
		{
			if (NULL == m_edit)
			{
				//创建Edit控件  
				// ES_WANTRETURN 使多行编辑器接收回车键输入并换行。如果不指定该风格,按回车键会选择缺省的命令按钮,这往往会导致对话框的关闭。  
				m_edit.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_WANTRETURN | ES_LEFT,
					CRect(0, 0, 0, 0), this, IDC_CELL_EDIT);
			}
			m_edit.MoveWindow(rect);
			m_edit.SetWindowText(strValue);
			m_edit.ShowWindow(SW_SHOW);
			m_edit.SetSel(0, -1);
			m_edit.SetFocus();
			UpdateWindow();
		}
		else
		{
			if (NULL == m_comboBox)
			{
				//创建Combobox控件  
				//CBS_DROPDOWNLIST : 下拉式组合框，但是输入框内不能进行输入  
				//WS_CLIPCHILDREN  :  其含义就是，父窗口不对子窗口区域进行绘制。默认情况下父窗口会对子窗口背景是进行绘制的，但是如果父窗口设置了WS_CLIPCHILDREN属性，父亲窗口不在对子窗口背景绘制.  
				m_comboBox.Create(WS_CHILD | WS_VISIBLE | CBS_SORT | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | CBS_AUTOHSCROLL,
					CRect(0, 0, 0, 0), this, IDC_CELL_COMBOBOX);
			}
			m_Font.CreateFont(18, 0, 0, 0, FW_NORMAL,
				FALSE, FALSE, FALSE, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_ROMAN, CString(_T("Arial")).GetBuffer(CString(_T("Arial")).GetLength()));

			m_comboBox.SetFont(&m_Font);
			m_comboBox.ResetContent();	//清空下拉框
			std::vector<CString> vecComboBoxData;
			switch (m_nCol)
			{
			case 1:
				vecComboBoxData = g_vecRebarDir;
				break;
			case 2:
				vecComboBoxData = g_vecRebarSize;
				break;
			case 3:
			{
				vecComboBoxData = g_vecRebarType;

				CFont  *pfont = m_comboBox.GetFont();
				LOGFONT       logfont;
				pfont->GetLogFont(&logfont);
				wcscpy_s(logfont.lfFaceName, L"SJQY");//
				pfont->CreateFontIndirect(&logfont);
				m_comboBox.SetFont(pfont);
			}
				break;
			case 8:
			{
				vecComboBoxData.clear();
				for (int i = 0; i < GetItemCount(); ++i)
				{
					CString strValue;
					if (0 == i)
						strValue = _T("1XL");
					else
						strValue.Format(_T("%dL"), i);
					vecComboBoxData.push_back(strValue);
				}
			}
			break;
			}
			for (size_t i = 0; i < vecComboBoxData.size(); ++i)
			{
				int index = m_comboBox.AddString(vecComboBoxData[i]);
				m_comboBox.SetItemData(index, m_nRow);
			}
			m_comboBox.MoveWindow(rect);
			m_comboBox.ShowWindow(SW_SHOW);
			//当前cell的值  
			CString strCellValue = GetItemText(m_nRow, m_nCol);
			int nIndex = m_comboBox.FindStringExact(0, strCellValue);
			if (CB_ERR == nIndex)
				m_comboBox.SetCurSel(0); //TODO 设置为当前值的Item  
			else
				m_comboBox.SetCurSel(nIndex);

			m_comboBox.SetFocus();
			UpdateWindow();
		}
	}
}

void CWallRebarEditListCtrl::OnCbnSelchangeCbCellComboBox()
{
	//同步更新处理数据  
	if ((1 == m_nCol) || (2 == m_nCol) || (3 == m_nCol) || (8 == m_nCol)) //combobox  
	{
		CString szLable;
		int nindex = m_comboBox.GetCurSel();
		m_comboBox.GetLBText(nindex, szLable);
		SetItemText(m_nRow, m_nCol, szLable);
		m_comboBox.ShowWindow(SW_HIDE);
	}
}

//发送失效消息  
void CWallRebarEditListCtrl::SendInvalidateMsg()
{
	//Edit单元格  
	if ((4 == m_nCol) || (5 == m_nCol) || (6 == m_nCol) || (7 == m_nCol))
	{

		if (NULL == m_edit.m_hWnd) return;
		BOOL bShow = m_edit.IsWindowVisible();
		if (bShow)
			::SendMessage(m_edit.m_hWnd, WM_KILLFOCUS, (WPARAM)0, (LPARAM)0);

	}
	else if ((1 == m_nCol) || (2 == m_nCol) || (3 == m_nCol) || (8 == m_nCol)) //combobox  
	{
		if (NULL == m_comboBox.m_hWnd) return;

		BOOL bShow = m_comboBox.IsWindowVisible();
		if (bShow)
			::SendMessage(m_comboBox.m_hWnd, WM_KILLFOCUS, (WPARAM)0, (LPARAM)0);
	}
}

void CWallRebarEditListCtrl::SetListEditShowWindow(UINT state)
{
	m_edit.ShowWindow(state);
}

void CWallRebarEditListCtrl::SetListComboBoxShowWindow(UINT state)
{
	m_comboBox.ShowWindow(state);
}

void CWallRebarEditListCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SendInvalidateMsg();

	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CWallRebarEditListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SendInvalidateMsg();

	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}


IMPLEMENT_DYNAMIC(CRebarLapOptionEditListCtrl, CListCtrl)
BEGIN_MESSAGE_MAP(CRebarLapOptionEditListCtrl, CListCtrl)
	ON_WM_LBUTTONDBLCLK()
	ON_CBN_SELCHANGE(IDC_CELL_COMBOBOX, &CRebarLapOptionEditListCtrl::OnCbnSelchangeCbCellComboBox)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

void CRebarLapOptionEditListCtrl::GetAllRebarData(std::vector<WallRebarInfo::LapOptions>& vecListData)
{
	vecListData.clear();
	for (int i = 0; i < GetItemCount(); ++i)
	{
		WallRebarInfo::LapOptions oneLapOptions;
		for (int j = 0; j < 7; ++j)
		{
			switch (j)
			{
			case 0:
				oneLapOptions.rebarLevel = i;
				break;
			case 1:
			{
				CString strCellValue = GetItemText(i, j);
				auto find = std::find(g_vecRebarLapOption.begin(), g_vecRebarLapOption.end(), strCellValue);
				int nIndex = (int)std::distance(g_vecRebarLapOption.begin(), find);
				oneLapOptions.connectMethod = nIndex;
			}
			break;
			case 2:
				oneLapOptions.lapLength = atoi(CT2A(GetItemText(i, j)));
				break;
			case 3:
				oneLapOptions.stockLength = atoi(CT2A(GetItemText(i, j)));
				break;
			case 4:
				oneLapOptions.millLength = atoi(CT2A(GetItemText(i, j)));
				break;
			case 5:
			{
				CString strCellValue = GetItemText(i, j);
				auto find = std::find(g_vecNoYes.begin(), g_vecNoYes.end(), strCellValue);
				int nIndex = (int)std::distance(g_vecNoYes.begin(), find);
				oneLapOptions.isStaggered = nIndex;
			}
			break;
			case 6:
				oneLapOptions.staggeredLength = atoi(CT2A(GetItemText(i, j)));
				break;
			}
		}
		vecListData.push_back(oneLapOptions);
	}
}
void CRebarLapOptionEditListCtrl::DisposeEdit(void)
{
	int nIndex = GetSelectionMark();
	//同步更新处理数据  
	CString sLable;
	if (2 == m_nCol || 3 == m_nCol || 4 == m_nCol || 6 == m_nCol)
	{
		CString szLable;
		m_edit.GetWindowText(szLable);
		SetItemText(m_nRow, m_nCol, szLable);
		m_edit.ShowWindow(SW_HIDE);
	}
	if (1 == m_nCol || 5 == m_nCol)
	{
		m_comboBox.ShowWindow(SW_HIDE);
	}

}

// 设置当前窗口的风格  
void CRebarLapOptionEditListCtrl::SetStyle(void)
{
	LONG lStyle;
	lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK; //清除显示方式  
	lStyle |= LVS_REPORT; //list模式  
	lStyle |= LVS_SINGLESEL; //单选  
	SetWindowLong(m_hWnd, GWL_STYLE, lStyle);

	//扩展模式  
	DWORD dwStyle = GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT; //选中某行使其整行高亮  
	dwStyle |= LVS_EX_GRIDLINES; //网格线  
	SetExtendedStyle(dwStyle); //设置扩展风格  
}

void CRebarLapOptionEditListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default  
	CListCtrl::OnLButtonDblClk(nFlags, point);

	LVHITTESTINFO info;
	info.pt = point;
	info.flags = LVHT_ONITEMLABEL;

	if (SubItemHitTest(&info) >= 0)
	{
		m_nRow = info.iItem;
		m_nCol = info.iSubItem;

		CRect rect;
		GetSubItemRect(m_nRow, m_nCol, LVIR_LABEL, rect);

		CString strValue;
		strValue = GetItemText(m_nRow, m_nCol);

		if (2 == m_nCol || 3 == m_nCol || 4 == m_nCol || 6 == m_nCol)
		{
			if (NULL == m_edit)
			{
				//创建Edit控件  
				// ES_WANTRETURN 使多行编辑器接收回车键输入并换行。如果不指定该风格,按回车键会选择缺省的命令按钮,这往往会导致对话框的关闭。  
				m_edit.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_WANTRETURN | ES_LEFT,
					CRect(0, 0, 0, 0), this, IDC_CELL_EDIT);
			}
			m_edit.MoveWindow(rect);
			m_edit.SetWindowText(strValue);
			m_edit.ShowWindow(SW_SHOW);
			m_edit.SetSel(0, -1);
			m_edit.SetFocus();
			UpdateWindow();
		}
		else
		{
			if (NULL == m_comboBox)
			{
				//创建Combobox控件  
				//CBS_DROPDOWNLIST : 下拉式组合框，但是输入框内不能进行输入  
				//WS_CLIPCHILDREN  :  其含义就是，父窗口不对子窗口区域进行绘制。默认情况下父窗口会对子窗口背景是进行绘制的，但是如果父窗口设置了WS_CLIPCHILDREN属性，父亲窗口不在对子窗口背景绘制.  
				m_comboBox.Create(WS_CHILD | WS_VISIBLE | CBS_SORT | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | CBS_AUTOHSCROLL,
					CRect(0, 0, 0, 0), this, IDC_CELL_COMBOBOX);
				m_Font.CreateFont(18, 0, 0, 0, FW_NORMAL,
					FALSE, FALSE, FALSE, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, DEFAULT_PITCH | FF_ROMAN, CString(_T("Arial")).GetBuffer(CString(_T("Arial")).GetLength()));
				m_comboBox.SetFont(&m_Font);
			}
			m_comboBox.ResetContent();	//清空下拉框
			std::vector<CString> vecComboBoxData;
			switch (m_nCol)
			{
			case 1:
				vecComboBoxData = g_vecRebarLapOption;
				break;
			case 5:
				vecComboBoxData = g_vecNoYes;
				break;
			break;
			}
			for (size_t i = 0; i < vecComboBoxData.size(); ++i)
			{
				int index = m_comboBox.AddString(vecComboBoxData[i]);
				m_comboBox.SetItemData(index, m_nRow);
			}
			m_comboBox.MoveWindow(rect);
			m_comboBox.ShowWindow(SW_SHOW);
			//当前cell的值  
			CString strCellValue = GetItemText(m_nRow, m_nCol);
			int nIndex = m_comboBox.FindStringExact(0, strCellValue);
			if (CB_ERR == nIndex)
				m_comboBox.SetCurSel(0); //TODO 设置为当前值的Item  
			else
				m_comboBox.SetCurSel(nIndex);

			m_comboBox.SetFocus();
			UpdateWindow();
		}
	}
}

void CRebarLapOptionEditListCtrl::OnCbnSelchangeCbCellComboBox()
{
	//同步更新处理数据  
	if ((1 == m_nCol) || (5 == m_nCol)) //combobox  
	{
		CString szLable;
		int nindex = m_comboBox.GetCurSel();
		m_comboBox.GetLBText(nindex, szLable);
		SetItemText(m_nRow, m_nCol, szLable);
		m_comboBox.ShowWindow(SW_HIDE);
	}
}

//发送失效消息  
void CRebarLapOptionEditListCtrl::SendInvalidateMsg()
{
	//Edit单元格  
	if (2 == m_nCol || 3 == m_nCol || 4 == m_nCol || 6 == m_nCol)
	{

		if (NULL == m_edit.m_hWnd) return;
		BOOL bShow = m_edit.IsWindowVisible();
		if (bShow)
			::SendMessage(m_edit.m_hWnd, WM_KILLFOCUS, (WPARAM)0, (LPARAM)0);

	}
	else if ((1 == m_nCol) || (5 == m_nCol)) //combobox  
	{
		if (NULL == m_comboBox.m_hWnd) return;

		BOOL bShow = m_comboBox.IsWindowVisible();
		if (bShow)
			::SendMessage(m_comboBox.m_hWnd, WM_KILLFOCUS, (WPARAM)0, (LPARAM)0);
	}
}

void CRebarLapOptionEditListCtrl::SetListEditShowWindow(UINT state)
{
	m_edit.ShowWindow(state);
}

void CRebarLapOptionEditListCtrl::SetListComboBoxShowWindow(UINT state)
{
	m_comboBox.ShowWindow(state);
}

void CRebarLapOptionEditListCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SendInvalidateMsg();

	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CRebarLapOptionEditListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SendInvalidateMsg();

	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}



IMPLEMENT_DYNAMIC(CRebarEndTypeEditListCtrl, CListCtrl)
BEGIN_MESSAGE_MAP(CRebarEndTypeEditListCtrl, CListCtrl)
	ON_WM_LBUTTONDBLCLK()
	ON_CBN_SELCHANGE(IDC_CELL_COMBOBOX, &CRebarEndTypeEditListCtrl::OnCbnSelchangeCbCellComboBox)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()


static int m_StaticBtId = 0;
void CRebarEndTypeEditListCtrl::AddEndPropButton(int nItem, int nSubItem, const CString & btnText, HWND hMain)
{
	CRect rect;
	GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rect);
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | BS_OWNERDRAW;
	CListCtrlBtn *btn = new CListCtrlBtn(0, nItem, nSubItem, rect, hMain);
	btn->Create(btnText, dwStyle, rect, this, m_StaticBtId);
	m_vecBtnEndProp.push_back(btn);
}
void CRebarEndTypeEditListCtrl::AddClearButton(int nItem, int nSubItem, const CString & btnText, HWND hMain)
{
	m_StaticBtId++;
	CRect rect;
	GetSubItemRect(nItem, nSubItem, LVIR_BOUNDS, rect);
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | BS_OWNERDRAW;
	CListCtrlBtn *btn = new CListCtrlBtn(0, nItem, nSubItem, rect, hMain);
	btn->Create(btnText, dwStyle, rect, this, m_StaticBtId);
	m_vecBtnClear.push_back(btn);
}

void CRebarEndTypeEditListCtrl::GetAllRebarData(std::vector<WallRebarInfo::EndType>& vecListData)
{
	vecListData.clear();
	for (int i = 0; i < GetItemCount(); ++i)
	{
		WallRebarInfo::EndType oneEndType;
		for (int j = 0; j < 7; ++j)
		{
			switch (j)
			{
			case 0:
				break;
			case 1:
			{
				CString strCellValue = GetItemText(i, j);
				auto find = std::find(g_vecEndType.begin(), g_vecEndType.end(), strCellValue);
				int nIndex = (int)std::distance(g_vecEndType.begin(), find);
				oneEndType.endType = nIndex;
			}
				break;
			case 2:
				break;
			case 3:
				oneEndType.offset = atof(CT2A(GetItemText(i, j)));
				break;
			case 4:
				oneEndType.rotateAngle = atof(CT2A(GetItemText(i, j)));
				break;
			case 5:
			break;
			}
		}
		vecListData.push_back(oneEndType);
	}
}
void CRebarEndTypeEditListCtrl::DisposeEdit(void)
{
	int nIndex = GetSelectionMark();
	//同步更新处理数据  
	CString sLable;
	if (3 == m_nCol || 4 == m_nCol)
	{
		CString szLable;
		m_edit.GetWindowText(szLable);
		SetItemText(m_nRow, m_nCol, szLable);
		m_edit.ShowWindow(SW_HIDE);
	}
	if (1 == m_nCol)
	{
		m_comboBox.ShowWindow(SW_HIDE);
	}

}

// 设置当前窗口的风格  
void CRebarEndTypeEditListCtrl::SetStyle(void)
{
	LONG lStyle;
	lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK; //清除显示方式  
	lStyle |= LVS_REPORT; //list模式  
	lStyle |= LVS_SINGLESEL; //单选  
	SetWindowLong(m_hWnd, GWL_STYLE, lStyle);

	//扩展模式  
	DWORD dwStyle = GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT; //选中某行使其整行高亮  
	dwStyle |= LVS_EX_GRIDLINES; //网格线  
	SetExtendedStyle(dwStyle); //设置扩展风格  
}

void CRebarEndTypeEditListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default  
	CListCtrl::OnLButtonDblClk(nFlags, point);

	LVHITTESTINFO info;
	info.pt = point;
	info.flags = LVHT_ONITEMLABEL;

	if (SubItemHitTest(&info) >= 0)
	{
		m_nRow = info.iItem;
		m_nCol = info.iSubItem;

		CRect rect;
		GetSubItemRect(m_nRow, m_nCol, LVIR_LABEL, rect);

		CString strValue;
		strValue = GetItemText(m_nRow, m_nCol);

		if (3 == m_nCol || 4 == m_nCol)
		{
			if (NULL == m_edit)
			{
				//创建Edit控件  
				// ES_WANTRETURN 使多行编辑器接收回车键输入并换行。如果不指定该风格,按回车键会选择缺省的命令按钮,这往往会导致对话框的关闭。  
				m_edit.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_WANTRETURN | ES_LEFT,
					CRect(0, 0, 0, 0), this, IDC_CELL_EDIT);
			}
			m_edit.MoveWindow(rect);
			m_edit.SetWindowText(strValue);
			m_edit.ShowWindow(SW_SHOW);
			m_edit.SetSel(0, -1);
			m_edit.SetFocus();
			UpdateWindow();
		}
		else if (1 == m_nCol)
		{
			if (NULL == m_comboBox)
			{
				//创建Combobox控件  
				//CBS_DROPDOWNLIST : 下拉式组合框，但是输入框内不能进行输入  
				//WS_CLIPCHILDREN  :  其含义就是，父窗口不对子窗口区域进行绘制。默认情况下父窗口会对子窗口背景是进行绘制的，但是如果父窗口设置了WS_CLIPCHILDREN属性，父亲窗口不在对子窗口背景绘制.  
				m_comboBox.Create(WS_CHILD | WS_VISIBLE | CBS_SORT | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | CBS_AUTOHSCROLL,
					CRect(0, 0, 0, 0), this, IDC_CELL_COMBOBOX);
				m_Font.CreateFont(18, 0, 0, 0, FW_NORMAL,
					FALSE, FALSE, FALSE, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, DEFAULT_PITCH | FF_ROMAN, CString(_T("Arial")).GetBuffer(CString(_T("Arial")).GetLength()));
				m_comboBox.SetFont(&m_Font);
			}
			m_comboBox.ResetContent();	//清空下拉框
			for (size_t i = 0; i < g_vecEndType.size(); ++i)
			{
				int index = m_comboBox.AddString(g_vecEndType[i]);
				m_comboBox.SetItemData(index, m_nRow);
			}
			m_comboBox.MoveWindow(rect);
			m_comboBox.ShowWindow(SW_SHOW);
			//当前cell的值  
			CString strCellValue = GetItemText(m_nRow, m_nCol);
			int nIndex = m_comboBox.FindStringExact(0, strCellValue);
			if (CB_ERR == nIndex)
				m_comboBox.SetCurSel(0); //TODO 设置为当前值的Item  
			else
				m_comboBox.SetCurSel(nIndex);

			m_comboBox.SetFocus();
			UpdateWindow();
		}
	}
}

void CRebarEndTypeEditListCtrl::OnCbnSelchangeCbCellComboBox()
{
	//同步更新处理数据  
	if (1 == m_nCol) //combobox  
	{
		CString szLable;
		int nindex = m_comboBox.GetCurSel();
		m_comboBox.GetLBText(nindex, szLable);
		SetItemText(m_nRow, m_nCol, szLable);
		m_comboBox.ShowWindow(SW_HIDE);
	}
}

//发送失效消息  
void CRebarEndTypeEditListCtrl::SendInvalidateMsg()
{
	//Edit单元格  
	if (3 == m_nCol || 4 == m_nCol)
	{

		if (NULL == m_edit.m_hWnd) return;
		BOOL bShow = m_edit.IsWindowVisible();
		if (bShow)
			::SendMessage(m_edit.m_hWnd, WM_KILLFOCUS, (WPARAM)0, (LPARAM)0);

	}
	else if (1 == m_nCol) //combobox  
	{
		if (NULL == m_comboBox.m_hWnd) return;

		BOOL bShow = m_comboBox.IsWindowVisible();
		if (bShow)
			::SendMessage(m_comboBox.m_hWnd, WM_KILLFOCUS, (WPARAM)0, (LPARAM)0);
	}
}

void CRebarEndTypeEditListCtrl::SetListEditShowWindow(UINT state)
{
	m_edit.ShowWindow(state);
}

void CRebarEndTypeEditListCtrl::SetListComboBoxShowWindow(UINT state)
{
	m_comboBox.ShowWindow(state);
}

void CRebarEndTypeEditListCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SendInvalidateMsg();

	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CRebarEndTypeEditListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SendInvalidateMsg();

	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}


IMPLEMENT_DYNAMIC(CACListCtrl, CListCtrl)
BEGIN_MESSAGE_MAP(CACListCtrl, CListCtrl)
	ON_WM_LBUTTONDBLCLK()
	ON_CBN_SELCHANGE(IDC_CELL_COMBOBOX, &CACListCtrl::OnCbnSelchangeCbCellComboBox)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()


void CACListCtrl::GetAllRebarData(std::vector<WallRebarInfo::AssociatedComponent>& vecListData)
{
	vecListData.clear();
	for (int i = 0; i < GetItemCount(); ++i)
	{
		WallRebarInfo::AssociatedComponent oneAC;
		for (int j = 0; j < 7; ++j)
		{
			switch (j)
			{
			case 0:
			{
				CString strCellValue = GetItemText(i, j);
				strcpy(oneAC.CurrentWallName, CT2A(strCellValue.GetBuffer()));
			}
				break;
			case 1:
			{
				CString strCellValue = GetItemText(i, j);
				strcpy(oneAC.associatedComponentName, CT2A(strCellValue.GetBuffer()));
			}
			break;
			case 2:
			{
				CString strCellValue = GetItemText(i, j);
				auto find = std::find(g_vecACRelation.begin(), g_vecACRelation.end(), strCellValue);
				int nIndex = (int)std::distance(g_vecACRelation.begin(), find);
				oneAC.mutualRelation = nIndex;
			}
				break;
			case 3:
				oneAC.rebarLevel = atoi(CT2A(GetItemText(i, j)));
				break;
			case 4:
			{
				CString strCellValue = GetItemText(i, j);
				auto find = std::find(g_vecACRebarRelation.begin(), g_vecACRebarRelation.end(), strCellValue);
				int nIndex = (int)std::distance(g_vecACRebarRelation.begin(), find);
				oneAC.associatedRelation = nIndex;
			}
				break;
			case 5:
			{
				CString strCellValue = GetItemText(i, j);
				auto find = std::find(g_vecACMethod.begin(), g_vecACMethod.end(), strCellValue);
				int nIndex = (int)std::distance(g_vecACMethod.begin(), find);
				oneAC.anchoringMethod = nIndex;
			}
				break;
			}
		}
		vecListData.push_back(oneAC);
	}
}
void CACListCtrl::DisposeEdit(void)
{
	int nIndex = GetSelectionMark();
	//同步更新处理数据  
//	CString sLable;
// 	if (3 == m_nCol || 4 == m_nCol)
// 	{
// 		CString szLable;
// 		m_edit.GetWindowText(szLable);
// 		SetItemText(m_nRow, m_nCol, szLable);
// 		m_edit.ShowWindow(SW_HIDE);
// 	}
	if (4 == m_nCol || 5 == m_nCol)
	{
		m_comboBox.ShowWindow(SW_HIDE);
	}

}

// 设置当前窗口的风格  
void CACListCtrl::SetStyle(void)
{
	LONG lStyle;
	lStyle = GetWindowLong(m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK; //清除显示方式  
	lStyle |= LVS_REPORT; //list模式  
	lStyle |= LVS_SINGLESEL; //单选  
	SetWindowLong(m_hWnd, GWL_STYLE, lStyle);

	//扩展模式  
	DWORD dwStyle = GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT; //选中某行使其整行高亮  
	dwStyle |= LVS_EX_GRIDLINES; //网格线  
	SetExtendedStyle(dwStyle); //设置扩展风格  
}

void CACListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default  
	CListCtrl::OnLButtonDblClk(nFlags, point);

	LVHITTESTINFO info;
	info.pt = point;
	info.flags = LVHT_ONITEMLABEL;

	if (SubItemHitTest(&info) >= 0)
	{
		m_nRow = info.iItem;
		m_nCol = info.iSubItem;

		CRect rect;
		GetSubItemRect(m_nRow, m_nCol, LVIR_LABEL, rect);

// 		CString strValue;
// 		strValue = GetItemText(m_nRow, m_nCol);

// 		if (3 == m_nCol || 4 == m_nCol)
// 		{
// 			if (NULL == m_edit)
// 			{
// 				//创建Edit控件  
// 				// ES_WANTRETURN 使多行编辑器接收回车键输入并换行。如果不指定该风格,按回车键会选择缺省的命令按钮,这往往会导致对话框的关闭。  
// 				m_edit.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_WANTRETURN | ES_LEFT,
// 					CRect(0, 0, 0, 0), this, IDC_CELL_EDIT);
// 			}
// 			m_edit.MoveWindow(rect);
// 			m_edit.SetWindowText(strValue);
// 			m_edit.ShowWindow(SW_SHOW);
// 			m_edit.SetSel(0, -1);
// 			m_edit.SetFocus();
// 			UpdateWindow();
// 		}
// 		else if (1 == m_nCol)
		if (4 == m_nCol || 5 == m_nCol)
		{
			if (NULL == m_comboBox)
			{
				//创建Combobox控件  
				//CBS_DROPDOWNLIST : 下拉式组合框，但是输入框内不能进行输入  
				//WS_CLIPCHILDREN  :  其含义就是，父窗口不对子窗口区域进行绘制。默认情况下父窗口会对子窗口背景是进行绘制的，但是如果父窗口设置了WS_CLIPCHILDREN属性，父亲窗口不在对子窗口背景绘制.  
				m_comboBox.Create(WS_CHILD | WS_VISIBLE | CBS_SORT | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | CBS_AUTOHSCROLL,
					CRect(0, 0, 0, 0), this, IDC_CELL_COMBOBOX);
				m_Font.CreateFont(18, 0, 0, 0, FW_NORMAL,
					FALSE, FALSE, FALSE, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, DEFAULT_PITCH | FF_ROMAN, CString(_T("Arial")).GetBuffer(CString(_T("Arial")).GetLength()));
				m_comboBox.SetFont(&m_Font);
			}
			m_comboBox.ResetContent();	//清空下拉框
			std::vector<CString> vecComboBoxData;
			switch (m_nCol)
			{
			case 4:
				vecComboBoxData = g_vecACRebarRelation;
				break;
			case 5:
				vecComboBoxData = g_vecACMethod;
				break;
				break;
			}
			for (size_t i = 0; i < vecComboBoxData.size(); ++i)
			{
				int index = m_comboBox.AddString(vecComboBoxData[i]);
				m_comboBox.SetItemData(index, m_nRow);
			}
			m_comboBox.MoveWindow(rect);
			m_comboBox.ShowWindow(SW_SHOW);
			//当前cell的值  
			CString strCellValue = GetItemText(m_nRow, m_nCol);
			int nIndex = m_comboBox.FindStringExact(0, strCellValue);
			if (CB_ERR == nIndex)
				m_comboBox.SetCurSel(0); //TODO 设置为当前值的Item  
			else
				m_comboBox.SetCurSel(nIndex);

			m_comboBox.SetFocus();
			UpdateWindow();
		}
	}
}

void CACListCtrl::OnCbnSelchangeCbCellComboBox()
{
	//同步更新处理数据  
	if (4 == m_nCol || 5 == m_nCol) //combobox  
	{
		CString szLable;
		int nindex = m_comboBox.GetCurSel();
		m_comboBox.GetLBText(nindex, szLable);
		SetItemText(m_nRow, m_nCol, szLable);
		m_comboBox.ShowWindow(SW_HIDE);
	}
}

//发送失效消息  
void CACListCtrl::SendInvalidateMsg()
{
	//Edit单元格  
// 	if (3 == m_nCol || 4 == m_nCol)
// 	{
// 		if (NULL == m_edit.m_hWnd) return;
// 		BOOL bShow = m_edit.IsWindowVisible();
// 		if (bShow)
// 			::SendMessage(m_edit.m_hWnd, WM_KILLFOCUS, (WPARAM)0, (LPARAM)0);
// 
// 	}
//	else if (1 == m_nCol) //combobox  
	if(4 == m_nCol || 5 == m_nCol)
	{
		if (NULL == m_comboBox.m_hWnd) return;

		BOOL bShow = m_comboBox.IsWindowVisible();
		if (bShow)
			::SendMessage(m_comboBox.m_hWnd, WM_KILLFOCUS, (WPARAM)0, (LPARAM)0);
	}
}

// void CACListCtrl::SetListEditShowWindow(UINT state)
// {
// 	m_edit.ShowWindow(state);
// }

void CACListCtrl::SetListComboBoxShowWindow(UINT state)
{
	m_comboBox.ShowWindow(state);
}

void CACListCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SendInvalidateMsg();

	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CACListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SendInvalidateMsg();

	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}
