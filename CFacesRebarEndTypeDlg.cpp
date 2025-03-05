﻿// CFacesRebarEndTypeDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CFacesRebarEndTypeDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "CRebarEndPointSetDlg.h"
#include "ElementAttribute.h"
#include "LineDrawingTool.h"

// CFacesRebarEndType 对话框

IMPLEMENT_DYNAMIC(CFacesRebarEndTypeDlg, CDialogEx)

CFacesRebarEndTypeDlg::CFacesRebarEndTypeDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_WallRebar_EndType, pParent)
{
	m_rebarLevelNum = 2;
}

CFacesRebarEndTypeDlg::~CFacesRebarEndTypeDlg()
{
}

void CFacesRebarEndTypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_EndType, m_ListEndType);
}

void CFacesRebarEndTypeDlg::InitUIData()
{
	LONG lStyle;
	lStyle = GetWindowLong(m_ListEndType.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_ListEndType.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_ListEndType.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_ListEndType.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_ListEndType.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_ListEndType.InsertColumn(0, _T("位置"), (int)(width / 8.0 * 1.25), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_ListEndType.InsertColumn(1, _T("类型"), (int)(width / 8.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(2, _T("端点属性"), (int)(width / 8.0 * 0.75), ListCtrlEx::Button, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(3, _T("偏移"), (int)(width / 8.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(4, _T("旋转角"), (int)(width / 8.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(5, _T("+90°"), (int)(width / 8.0), ListCtrlEx::Button, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(6, _T("-90°"), (int)(width / 8.0), ListCtrlEx::Button, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(7, _T("手绘偏移"), (int)(width / 8.0), ListCtrlEx::Button, LVCFMT_CENTER, ListCtrlEx::SortByString);

	UpdateEndTypeList();
}

void CFacesRebarEndTypeDlg::SetListDefaultData()
{
	if (m_vecEndType.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < m_rebarLevelNum * 2; i++)
		{
			PIT::EndType::RebarEndPointInfo endPtInfo;
			memset(&endPtInfo, 0, sizeof(endPtInfo));
			// 			endPtInfo.value1 = RebarCode::GetPinRadius(m_vecRebarData[i / 2].rebarSize, ACTIVEMODEL,false);
			// 			endPtInfo.value3 = RebarCode::GetBendLength(m_vecRebarData[i / 2].rebarSize, ACTIVEMODEL, false);
			PIT::EndType oneEndTypeData = { 0,0,0 ,endPtInfo };
			m_vecEndType.push_back(oneEndTypeData);
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = m_rebarLevelNum * 2 - (int)m_vecEndType.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				PIT::EndType::RebarEndPointInfo endPtInfo;
				memset(&endPtInfo, 0, sizeof(endPtInfo));
				PIT::EndType oneEndTypeData = { 0,0,0 ,endPtInfo };
				m_vecEndType.push_back(oneEndTypeData);
			}
		}
		if (iOffset < 0)
		{
			iOffset *= -1;
			for (int i = 0; i < iOffset; i++)
			{
				m_vecEndType.pop_back();
			}
		}
	}
}

void CFacesRebarEndTypeDlg::UpdateEndTypeList()
{
	m_ListEndType.DeleteAllItems();
	SetListDefaultData();
	for (int i = 0; i < g_vecRebarData.size() * 2; ++i)
	{
		int index = i / 2;
		int datachange = g_vecRebarData[index].datachange;
		WString posWstr = L"";
		switch (datachange)
		{
		case 0:
			posWstr = L"正面";
			break;
		case 1:
			posWstr = L"中间";
			break;
		case 2:
			posWstr = L"背面";
			break;
		}
		if (i & 0x01)
		{
			CString strIndex;
			strIndex.Format(L"%dL%s终端", g_vecRebarData[index].rebarLevel, posWstr);
			m_ListEndType.InsertItem(i, _T("")); // 插入行
			m_ListEndType.SetItemText(i, 0, strIndex);
		}
		else
		{
			CString strIndex;
			strIndex.Format(L"%dL%s始端", g_vecRebarData[index].rebarLevel, posWstr);
			m_ListEndType.InsertItem(i, _T("")); // 插入行
			m_ListEndType.SetItemText(i, 0, strIndex);
		}

		for (int j = 1; j < 8; ++j)
		{
			CString strValue;
			switch (j)
			{
			case 1:
			{
				ListCtrlEx::CStrList strlist = g_listEndType;
				m_ListEndType.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecEndType[i].endType);
				strValue = *it;
				m_ListEndType.SetItemText(i, j, strValue);
			}
			break;
			case 2:
				strValue = _T("...");
				break;
			case 3:
			{
				strValue.Format(_T("%.2f"), m_vecEndType[i].offset);
			}
			break;
			case 4:
			{
				strValue.Format(_T("%.2f"), m_vecEndType[i].rotateAngle);
			}
			break;
			case 5:
				strValue = _T("+90°");
				break;
			case 6:
				strValue = _T("-90°");
				break;
			case 7:
				strValue = _T("...");
				break;
			default:
				break;
			}
			m_ListEndType.SetItemText(i, j, strValue);
		}
	}
}


BEGIN_MESSAGE_MAP(CFacesRebarEndTypeDlg, CDialogEx)
	ON_MESSAGE(WM_ListCtrlEx_BUTTON_LBUTTONDOWN, &CFacesRebarEndTypeDlg::OnEndTypeButtonDown)
END_MESSAGE_MAP()


// CWallRebarEndType 消息处理程序


BOOL CFacesRebarEndTypeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	InitUIData();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CFacesRebarEndTypeDlg::SetOffsetValue(int row, double length)
{
	if (row >= 0 && row < m_vecEndType.size())
	{
		m_vecEndType[row].offset = length;  // 更新偏移值，假设 offset 是成员变量
		UpdateEndTypeList();  // 刷新表格显示
	}
}

LRESULT CFacesRebarEndTypeDlg::OnEndTypeButtonDown(WPARAM wParam, LPARAM lParam)
{
	ListCtrlEx::ButtonCellMsg* msg = (ListCtrlEx::ButtonCellMsg*)lParam;
	CString strValue = m_ListEndType.GetItemText(msg->m_nRow, msg->m_nColumn - 1);

	auto find = std::find(g_listEndType.begin(), g_listEndType.end(), strValue);
	int nIndex = (int)std::distance(g_listEndType.begin(), find);
	// 触发后立刻保存表格数据
	std::vector<PIT::EndType> tmpvecEndTypeData;
	m_ListEndType.GetAllRebarData(tmpvecEndTypeData);
	for (int i = 0; i < tmpvecEndTypeData.size(); i++)
	{
		tmpvecEndTypeData[i].endPtInfo = m_vecEndType[i].endPtInfo;
	}
	m_vecEndType = tmpvecEndTypeData;

	if (2 == msg->m_nColumn)
	{
		CRebarEndPointSetDlg dlg(nIndex, this);
		dlg.SetRebarEndPointInfo(m_vecEndType[msg->m_nRow].endPtInfo);
		dlg.SetRebarSize(m_vecRebarData[msg->m_nRow / 2].rebarSize);
		if (IDOK == dlg.DoModal())
		{
			m_vecEndType[msg->m_nRow].endPtInfo = dlg.GetRebarEndPointInfo();
		}
	}
	else if (5 == msg->m_nColumn)
	{
		m_vecEndType[msg->m_nRow].rotateAngle += 90;

		UpdateEndTypeList();
	}
	else if (6 == msg->m_nColumn)
	{
		m_vecEndType[msg->m_nRow].rotateAngle -= 90;

		UpdateEndTypeList();
	}
	else if (7 == msg->m_nColumn)  // 手绘偏移
	{
		// 保存当前编辑的行索引
		m_nCurrentRow = msg->m_nRow;
		// 创建并启动交互工具
		LineDrawingTool* tool = new LineDrawingTool(0, 0, this);
		tool->InstallTool();  // 启动手动绘制偏移长度工具
	}

	return 0;
}