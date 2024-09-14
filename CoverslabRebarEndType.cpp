// CoverslabRebarEndType.cpp: 实现文件
//

#include "_USTATION.h"
#include "CoverslabRebarEndType.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "CoverslabRebarAssembly.h"

// CoverslabRebarEndType 对话框

IMPLEMENT_DYNAMIC(CoverslabRebarEndType, CDialogEx)

CoverslabRebarEndType::CoverslabRebarEndType(ElementHandleCR eh, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CoverslabRebar_EndType, pParent), m_ehSel(eh)
{
	m_CoverslabType = CoverslabRebarAssembly::JudgeSlabType(m_ehSel);
}

CoverslabRebarEndType::~CoverslabRebarEndType()
{
}

void CoverslabRebarEndType::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_C_LIST_EndType, m_ListEndType);
}

void CoverslabRebarEndType::InitUIData()
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
	m_ListEndType.InsertColumn(0, _T("位置"), (int)(width / 5), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_ListEndType.InsertColumn(1, _T("类型"), (int)(width / 5), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(2, _T("端点属性"), (int)(width / 5), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(3, _T("偏移"), (int)(width / 5), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(4, _T("旋转角"), (int)(width / 5), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	//	m_ListEndType.InsertColumn(5, _T("清空"), LVCFMT_CENTER, 100);

	SetListRowData(m_vecEndType);

	UpdateEndTypeList();
}

void CoverslabRebarEndType::SetListDefaultData()
{
	if (m_vecEndType.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum * 2; i++)
		{
			int endType = 0;
			if (m_CoverslabType == CoverslabRebarAssembly::SZCoverSlab)
			{
				if (i < 4 || i == 13 || i == 12)
				{
					endType = 5;
				}

			}
			else if (m_CoverslabType == CoverslabRebarAssembly::SICoverSlab)
			{
				if (i < 2) // 箍筋
				{
					endType = 5;
				}
				else // 点筋
				{
					endType = 6;
				}
			}
			else if (m_CoverslabType == CoverslabRebarAssembly::STCoverSlab_Ten)
			{
				if (i == 0 || i == 1)
				{
					endType = 5;
				}

				if (i == 4 || i == 5)
				{
					endType = 6;
				}
			}
			else if (m_CoverslabType == CoverslabRebarAssembly::STCoverSlab)
			{
				if (i == 0 || i == 1 || i == 8 || i == 9)
				{
					endType = 5;
				}
			}
			PIT::EndType oneEndTypeData = { endType,0,0 };
			m_vecEndType.push_back(oneEndTypeData);
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = g_wallRebarInfo.concrete.rebarLevelNum * 2 - (int)m_vecEndType.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				PIT::EndType oneEndTypeData = { 0,0,0 };
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

void CoverslabRebarEndType::UpdateEndTypeList()
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

		for (int j = 1; j < 5; ++j)
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
			// 			case 5:
			// 				m_ListEndType.AddClearButton(i, j, _T("清空"), m_ListEndType.m_hWnd);
							//				strValue = _T("清空");
			break;
			default:
				break;
			}
			m_ListEndType.SetItemText(i, j, strValue);
		}
	}
	//	m_ListEndType.SetShowProgressPercent(TRUE);
	//	m_ListEndType.SetSupportSort(TRUE);
}

BEGIN_MESSAGE_MAP(CoverslabRebarEndType, CDialogEx)
	
END_MESSAGE_MAP()


// CoverslabRebarEndType 消息处理程序


BOOL CoverslabRebarEndType::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	InitUIData();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}