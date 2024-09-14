// CWallInsertRebarDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CWallInsertRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "ListCtrlEx.h"
#include "ElementAttribute.h"

// CWallInsertRebarDlg 对话框

IMPLEMENT_DYNAMIC(CWallInsertRebarDlg, CDialogEx)

CWallInsertRebarDlg::CWallInsertRebarDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_InsertRebarWall, pParent)
{
	m_pInsertWallAssembly = NULL;
	m_ConcreteId = 0;
	m_staggeredStyle = 0;
}

CWallInsertRebarDlg::~CWallInsertRebarDlg()
{
}

void CWallInsertRebarDlg::InitUIData()
{
	GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
	GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
	// GetDlgItem(IDC_CHECK2)->ShowWindow(SW_HIDE);

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
	m_listMainRebar.InsertColumn(0, _T("层"), (int)(width / 11 * 0.75), ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_listMainRebar.InsertColumn(1, _T("方向"), (int)(width / 11 * 0.75), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(2, _T("直径"), (int)(width / 11 * 0.75), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(3, _T("类型"), (int)(width / 11), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(4, _T("间距"), (int)(width / 11 * 1.25), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(5, _T("起点偏移"), (int)(width / 11 * 1.25), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(6, _T("终点偏移"), (int)(width / 11 * 1.25), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(7, _T("与前层间距"), (int)(width / 11 * 1.25), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(8, _T("数据交换"), (int)(width / 11 * 0.75), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(9, _T("埋置长度"), (int)(width / 11), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(10, _T("拓展长度"), (int)(width / 11), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

//	m_listMainRebar.SetShowProgressPercent(TRUE);
//	m_listMainRebar.SetSupportSort(TRUE);

	ElementId testid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	GetElementXAttribute(testid, m_RebarPts, vecRebarPointsXAttribute, ACTIVEMODEL);
	GetElementXAttribute(testid, g_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);

	GetElementXAttribute(testid, sizeof(Transform), m_trans, UcsMatrixXAttribute, ACTIVEMODEL);
	m_trans.InverseOf(m_trans);
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ACCConcrete), m_acconcrete, ConcreteCoverXAttribute, m_ehSel.GetModelRef());
	m_vecWallInsertInfo.clear();
	GetElementXAttribute(testid, m_vecWallInsertInfo, vecInsertRebarInfoXAttribute, ACTIVEMODEL);

	GetElementXAttribute(testid, sizeof(PIT::WallRebarInfo), m_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);

	SetListRowData(g_vecRebarData);
	SetListDefaultData();
	double uor_per_mm = m_ehSel.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素
	for (size_t i = 0; i < m_vecWallInsertInfo.size(); i++)
	{
		BrString sizeKey = g_vecRebarData[i].rebarSize;
		sizeKey.Replace(L"mm", L"");
		m_diameter = RebarCode::GetBarDiameter(sizeKey, m_ehSel.GetModelRef()) / uor_per_mm; // 钢筋直径

		if (m_basis.IsValid())
		{
			ElementId baseid = 0;
			std::vector<PIT::ConcreteRebar>	vecRebarDataTmp;
			GetElementXAttribute(m_basis.GetElementId(), sizeof(ElementId), baseid, ConcreteIDXAttribute, m_basis.GetModelRef());
			if (baseid > 0)
			{
				GetElementXAttribute(baseid, vecRebarDataTmp, vecRebarDataXAttribute, ACTIVEMODEL);
				GetElementXAttribute(baseid, sizeof(PIT::WallRebarInfo), m_slabRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
			}
			if (m_slabRebarInfo.concrete.postiveCover - 0.00 > 0)
			{
				m_vecWallInsertInfo[i].postiveCover = m_slabRebarInfo.concrete.postiveCover;
			}
			m_vecWallInsertInfo[i].slabDistance = 0.00;
			for (int j = (int)vecRebarDataTmp.size() - 1; j >= 0 && j > (int)vecRebarDataTmp.size() - 3; j--)
			{
				BrString sizeKey = vecRebarDataTmp[j].rebarSize;
				sizeKey.Replace(L"mm", L"");
				double slabDiameter = RebarCode::GetBarDiameter(sizeKey, m_basis.GetModelRef()); // 钢筋直径
				m_vecWallInsertInfo[i].slabDistance += slabDiameter / uor_per_mm;
			}
			m_vecWallInsertInfo[i].slabThickness = CInsertRebarAssemblySTWall::CalcSlabThickness(m_basis); // 板厚
			m_vecWallInsertInfo[i].embedLength = m_vecWallInsertInfo[i].slabThickness - m_vecWallInsertInfo[i].postiveCover - m_vecWallInsertInfo[i].slabDistance - m_diameter * 0.5;
		}
		
	}

	if (m_vecWallInsertInfo.size() == 0)
	{
		m_staggeredStyle = 0;
		m_wallTopType = 0;
	}
	else
	{
		m_staggeredStyle = m_vecWallInsertInfo[0].staggeredStyle;
		m_wallTopType = m_vecWallInsertInfo[0].wallTopType;
	}
	if (m_staggeredStyle == 1)
	{
		m_staggered_check1.SetCheck(1); // 勾选
	}
	else if (m_staggeredStyle == 2)
	{
		m_staggered_check2.SetCheck(1); // 勾选
	}

	UpdateRebarList();

	if (m_wallTopType == 1) // 上墙变宽
	{
		m_staggered_check3.SetCheck(1); // 勾选
		m_staggered_check1.EnableWindow(FALSE);
		//for (size_t i = 0; i < m_vecWallInsertInfo.size(); i++)
		//{
		//	if (i != m_vecFilterLevel[0])
		//	{
		//		m_vecWallInsertInfo[i].embedLength = 0.00;
		//		m_vecWallInsertInfo[i].expandLength = 0.00;
		//	}
		//}
	}
	else if (m_wallTopType == 2) // 上墙变窄
	{
		//for (size_t i = 0; i < m_vecWallInsertInfo.size(); i++)
		//{
		//	if (i != m_vecFilterLevel[m_vecFilterLevel.size() - 1])
		//	{
		//		m_vecWallInsertInfo[i].embedLength = 0.00;
		//		m_vecWallInsertInfo[i].expandLength = 0.00;
		//	}
		//}
		m_staggered_check4.SetCheck(1); // 勾选
		m_staggered_check1.EnableWindow(FALSE);
	}

	m_vecWallInsertInfoBak.clear();
	for (auto var : m_vecWallInsertInfo)
	{
		m_vecWallInsertInfoBak.push_back(var);
	}

	UpdateRebarList();

	lStyle = GetWindowLong(m_ListEndType.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_ListEndType.m_hWnd, GWL_STYLE, lStyle);//设置style

	dwStyle = m_ListEndType.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_ListEndType.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	// 设置列表控件的报表显示方式
	// m_ListEndType.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRectt;
	m_ListEndType.GetClientRect(&stRect);
	double Endwidth = stRect.right - stRect.left;
	// 在列表控件中插入列
	m_ListEndType.InsertColumn(0, _T("位置"), (int)(Endwidth / 5), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_ListEndType.InsertColumn(1, _T("类型"), (int)(Endwidth / 5), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(2, _T("端点属性"), (int)(Endwidth / 5), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(3, _T("偏移"), (int)(Endwidth / 5), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListEndType.InsertColumn(4, _T("旋转角"), (int)(Endwidth / 5), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	// m_ListEndType.InsertColumn(5, _T("清空"), LVCFMT_CENTER, 100);

	m_vecEndType.clear();
	std::vector<PIT::EndType> vecEndTypeTmp;
	GetElementXAttribute(testid, vecEndTypeTmp, vecEndTypeDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(testid, m_vecEndType, vecInsertEndTypeDataXAttribute, ACTIVEMODEL);
	if (m_vecEndType.size() == 0 && vecEndTypeTmp.size() > 0)
	{
		for (PIT::EndType endtype : vecEndTypeTmp)
		{
			m_vecEndType.push_back(endtype);
		}
	}

	SetListRowDataEndType(m_vecEndType);
	SetListDefaultDataEndType();

	UpdateEndTypeList();
	
	m_vecEndTypeBak.clear();
	for (auto var : m_vecEndType)
	{
		m_vecEndTypeBak.push_back(var);
	}


	m_pInsertWallAssembly = NULL;

}

BOOL CWallInsertRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	InitUIData();
	// TODO:  在此添加额外的初始化
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CWallInsertRebarDlg::SetListDefaultData()
{
	if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
		{
			PIT::ConcreteRebar oneRebarData;
			if (0 == i)
				oneRebarData = { i,0,"12",0,200,0,0,0 };
			else if (1 == i)
				oneRebarData = { i,1,"12",0,200,0,0,0 };
			else
			{
				int dir = (i + 1) & 0x01;
				double levelSpace;
				levelSpace = ((i + 1) & 0x01) * 200.0;
				oneRebarData = { i,dir,"12",0,200,0,0,levelSpace };
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
				PIT::ConcreteRebar oneRebarData = { i,dir,"12",0,200,0,0,levelSpace };
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
	g_vecRebarData = m_vecRebarData;

	if (m_vecWallInsertInfo.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
		{
			InsertRebarInfo::WallInfo oneRebarData;
			oneRebarData = { 0, 0, 0.00, 0.00, 0.00, 500.00, 800.00 };
			m_vecWallInsertInfo.push_back(oneRebarData);
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = g_wallRebarInfo.concrete.rebarLevelNum - (int)m_vecWallInsertInfo.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				InsertRebarInfo::WallInfo oneRebarData = { 0, 0, 0.00, 0.00, 0.00, 500.00, 800.00};
				m_vecWallInsertInfo.push_back(oneRebarData);
			}
		}
		if (iOffset < 0)
		{
			iOffset *= -1;
			for (int i = 0; i < iOffset; i++)
			{
				m_vecWallInsertInfo.pop_back();
			}
		}
	}
	g_vecWallInsertInfo = m_vecWallInsertInfo;
}

void CWallInsertRebarDlg::SetListDefaultDataEndType()
{
	if (m_vecEndType.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum * 2; i++)
		{
			PIT::EndType oneEndTypeData = { 0,0,0 };
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

void CWallInsertRebarDlg::UpdateEndTypeList()
{
	m_ListEndType.DeleteAllItems();
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
			if (find(m_vecFilterLevel.begin(), m_vecFilterLevel.end(), (i | 0x1) / 2) == m_vecFilterLevel.end())
			{
				m_ListEndType.SetCellEnabled(i, j, FALSE);
			}
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
				ListCtrlEx::CStrList strlist = g_listRotateAngle;
				m_ListEndType.SetCellStringList(i, j, strlist);
				strValue.Format(_T("%.f°"), m_vecEndType[i].rotateAngle);
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

void CWallInsertRebarDlg::UpdateRebarList()
{
	m_listMainRebar.DeleteAllItems();
	SetListDefaultData();
	m_vecFilterLevel.clear();
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
		bool  bFlag = true;
		for (int j = 1; j <= nSubCnt; ++j)
		{
			m_listMainRebar.SetCellEnabled(i, j, FALSE);
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
				if (strValue.Compare(*g_listRebarDir.begin()) == 0) // 水平方向过滤
				{
					bFlag = false;
					m_vecWallInsertInfo[i].expandLength = 0.00;
					m_vecWallInsertInfo[i].embedLength = 0.00;
				}
				else
				{
					m_vecFilterLevel.push_back(i);
				}
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
			case 9:
			{
				if (bFlag)
				{
					m_listMainRebar.SetCellEnabled(i, j, TRUE);
				}
				strValue.Format(_T("%.2f"), m_vecWallInsertInfo[i].embedLength);
				break;
			}
			case 10:
			{
				if (bFlag)
				{
					m_listMainRebar.SetCellEnabled(i, j, TRUE);
				}
				strValue.Format(_T("%.2f"), m_vecWallInsertInfo[i].expandLength);
				break;
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


void CWallInsertRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_CHECK1, m_staggered_check1);	// 按行交错
	DDX_Control(pDX, IDC_CHECK2, m_staggered_check2);   // 按钢筋交错
	DDX_Control(pDX, IDC_CHECK3, m_staggered_check3);	// 上墙变宽
	DDX_Control(pDX, IDC_CHECK4, m_staggered_check4);	// 上墙变窄
	DDX_Control(pDX, IDC_LIST2,  m_listMainRebar);		// 主筋数据
	DDX_Control(pDX, IDC_LIST1,  m_ListEndType);		// 主筋端部数据
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CWallInsertRebarDlg, CDialogEx)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST2, &CWallInsertRebarDlg::OnLvnItemchangedList2)
	ON_BN_CLICKED(IDC_CHECK1, &CWallInsertRebarDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_CHECK2, &CWallInsertRebarDlg::OnBnClickedCheck2)
	ON_BN_CLICKED(IDOK, &CWallInsertRebarDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHECK3, &CWallInsertRebarDlg::OnBnClickedCheck3)
	ON_BN_CLICKED(IDC_CHECK4, &CWallInsertRebarDlg::OnBnClickedCheck4)
END_MESSAGE_MAP()


// CWallInsertRebarDlg 消息处理程序


void CWallInsertRebarDlg::OnLvnItemchangedList2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CWallInsertRebarDlg::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_staggered_check1.GetCheck() == 0)
	{
		m_staggeredStyle = 0;
		m_vecWallInsertInfo.clear();
		for (auto var : m_vecWallInsertInfoBak)
		{
			m_vecWallInsertInfo.push_back(var);
		}
		UpdateRebarList();
		return;
	}
	m_staggered_check2.SetCheck(0); // 不勾选
	m_staggeredStyle = 1;
	
	m_listMainRebar.GetAllRebarData(m_vecWallInsertInfo);

	bool bFirst = true;
	bool bFlag = true;
	double dExpandLength = 0.00; // 拓展长度
	for (unsigned int i = 0; i < m_vecWallInsertInfo.size(); i++)
	{
		if (find(m_vecFilterLevel.begin(), m_vecFilterLevel.end(), i) == m_vecFilterLevel.end())
		{
			continue;
		}

		m_vecWallInsertInfo[i].embedLength = m_vecWallInsertInfoBak[i].embedLength;

		if (bFirst)
		{
			dExpandLength = m_vecWallInsertInfoBak[i].expandLength;
			bFirst = false;
		}
		else
		{
			m_vecWallInsertInfo[i].expandLength = dExpandLength;
			if (bFlag)
			{
				m_vecWallInsertInfo[i].expandLength = dExpandLength * 0.5;
			}
			bFlag = !bFlag;
		}
	}

	UpdateRebarList();

}


void CWallInsertRebarDlg::OnBnClickedCheck2()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_staggered_check2.GetCheck() == 0)
	{
		m_staggeredStyle = 0;
		m_vecWallInsertInfo.clear();
		for (auto var : m_vecWallInsertInfoBak)
		{
			m_vecWallInsertInfo.push_back(var);
		}
		return;
	}
	m_staggeredStyle = 2;
	m_staggered_check1.SetCheck(0); // 不勾选

	//m_listMainRebar.GetAllRebarData(m_vecWallInsertInfo);

	//bool bFirst = true;
	//double dExpandLength = 0.00; // 拓展长度
	//for (unsigned int i = 0; i < m_vecWallInsertInfo.size(); i++)
	//{
	//	if (find(m_vecFilterLevel.begin(), m_vecFilterLevel.end(), i) == m_vecFilterLevel.end())
	//	{
	//		continue;
	//	}
	//	if (bFirst)
	//	{
	//		dExpandLength = m_vecWallInsertInfoBak[i].expandLength;
	//		bFirst = false;
	//	}
	//	else
	//	{
	//		m_vecWallInsertInfo[i].expandLength = dExpandLength;
	//	}
	//}

	//for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
	//{
	//	int nSubCnt = m_listMainRebar.GetColumnCount() - 1;
	//	for (int j = 1; j <= nSubCnt; ++j)
	//	{
	//		CString strValue = CString();
	//		if (j == 10)
	//		{
	//			strValue.Format(_T("%.2f"), m_vecWallInsertInfo[i].expandLength);
	//			m_listMainRebar.SetItemText(i, j, strValue);
	//		}
	//	}
	//}
}

void CWallInsertRebarDlg::CoverOnBnClickedOk()
{
	OnBnClickedOk();
}


void CWallInsertRebarDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = m_ehSel.GetModelRef();
	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());

	m_listMainRebar.GetAllRebarData(m_vecWallInsertInfo);
	// m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_ListEndType.GetAllRebarData(m_vecEndType);
	CInsertRebarAssemblySTWall::IsSmartSmartFeature(eeh);
	if (m_pInsertWallAssembly == NULL)
	{
		m_pInsertWallAssembly = REA::Create<CInsertRebarAssemblySTWall>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	}
	m_pInsertWallAssembly->SetTrans(m_trans);
	m_pInsertWallAssembly->AnalyzingWallGeometricData(eeh);
	m_pInsertWallAssembly->SetvecWallData(m_vecWallInsertInfo);
	m_pInsertWallAssembly->SetVecDirSizeData(m_vecRebarData);
	m_pInsertWallAssembly->SetRebarEndTypes(m_vecEndType);
	m_pInsertWallAssembly->SetrebarPts(m_RebarPts); // 主筋中的点
	m_pInsertWallAssembly->SetLayerRebars();
	m_pInsertWallAssembly->SetaccConcrete(m_acconcrete);
	m_pInsertWallAssembly->SetConcreteData(m_wallRebarInfo.concrete);
	m_pInsertWallAssembly->Setstaggered(m_staggeredStyle);
	m_pInsertWallAssembly->MakeRebars(modelRef);
	m_pInsertWallAssembly->Save(modelRef); // must save after creating rebars

	// 将插筋修改的数据存起来
	if (m_vecWallInsertInfo.size() > 0)
	{
		m_vecWallInsertInfo[0].staggeredStyle = m_staggeredStyle;
		m_vecWallInsertInfo[0].wallTopType = m_wallTopType;
	}
	SetElementXAttribute(m_pInsertWallAssembly->FetchConcrete(), m_vecWallInsertInfo, vecInsertRebarInfoXAttribute, ACTIVEMODEL);
	SetElementXAttribute(m_pInsertWallAssembly->FetchConcrete(), m_vecEndType, vecInsertEndTypeDataXAttribute, ACTIVEMODEL);
}


void CWallInsertRebarDlg::OnBnClickedCheck3()
{
	m_staggered_check1.SetCheck(0);
	if (m_staggered_check3.GetCheck() == 0)
	{
		m_wallTopType = 0;
		m_staggered_check1.EnableWindow(TRUE);
		m_vecWallInsertInfo.clear();
		for (auto var : m_vecWallInsertInfoBak)
		{
			m_vecWallInsertInfo.push_back(var);
		}
		m_vecEndType.clear();
		for (auto var : m_vecEndTypeBak)
		{
			m_vecEndType.push_back(var);
		}
		UpdateRebarList();
		UpdateEndTypeList();
		return ;
	}

	m_staggered_check4.SetCheck(0); // 不勾选
	m_wallTopType = 1; // 上墙变宽
	m_staggered_check1.EnableWindow(FALSE);

	m_listMainRebar.GetAllRebarData(m_vecWallInsertInfo);
	m_ListEndType.GetAllRebarData(m_vecEndType);
	// TODO: 在此添加控件通知处理程序代码
	for (size_t i = 0; i < m_vecWallInsertInfo.size(); i++)
	{
		if (i != m_vecFilterLevel[0])
		{
			m_vecWallInsertInfo[i].embedLength = 0.00;
			m_vecWallInsertInfo[i].expandLength = 0.00;
			m_vecEndType[i * 2 + 1].rotateAngle = m_vecEndTypeBak[i * 2 + 1].rotateAngle;
		}
		else
		{
			double uor_per_mm = m_ehSel.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素
			BrString sizeKey = g_vecRebarData[i].rebarSize;
			sizeKey.Replace(L"mm", L"");
			m_diameter = RebarCode::GetBarDiameter(sizeKey, m_ehSel.GetModelRef()) / uor_per_mm; // 钢筋直径
			m_vecWallInsertInfo[i].embedLength = m_vecWallInsertInfo[i].slabThickness - m_vecWallInsertInfo[i].postiveCover - m_vecWallInsertInfo[i].slabDistance - m_diameter * 0.5;
			m_vecWallInsertInfo[i].expandLength = m_vecWallInsertInfoBak[i].expandLength;
			m_vecEndType[i * 2 + 1].rotateAngle = 180.0;
		}
	}

	UpdateRebarList();
	UpdateEndTypeList();
}


void CWallInsertRebarDlg::OnBnClickedCheck4()
{
	// TODO: 在此添加控件通知处理程序代码
	m_staggered_check1.SetCheck(0);
	if (m_staggered_check4.GetCheck() == 0)
	{
		m_wallTopType = 0;
		m_vecWallInsertInfo.clear();
		for (auto var : m_vecWallInsertInfoBak)
		{
			m_vecWallInsertInfo.push_back(var);
		}
		m_vecEndType.clear();
		for (auto var : m_vecEndTypeBak)
		{
			m_vecEndType.push_back(var);
		}
		m_staggered_check1.SetCheck(0);
		m_staggered_check1.EnableWindow(TRUE);

		UpdateRebarList();
		UpdateEndTypeList();
		return;
	}
	m_staggered_check3.SetCheck(0); // 不勾选
	m_wallTopType = 2; // 上墙变窄
	m_staggered_check1.EnableWindow(FALSE);
	m_listMainRebar.GetAllRebarData(m_vecWallInsertInfo);
	m_ListEndType.GetAllRebarData(m_vecEndType);
	for (size_t i = 0; i < m_vecWallInsertInfo.size(); i++)
	{
		if (i != m_vecFilterLevel[m_vecFilterLevel.size() - 1])
		{
			m_vecWallInsertInfo[i].embedLength = 0.00;
			m_vecWallInsertInfo[i].expandLength = 0.00;
			m_vecEndType[i * 2 + 1].rotateAngle = m_vecEndTypeBak[i * 2 + 1].rotateAngle;
		}
		else
		{
			double uor_per_mm = m_ehSel.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素
			BrString sizeKey = g_vecRebarData[i].rebarSize;
			sizeKey.Replace(L"mm", L"");
			m_diameter = RebarCode::GetBarDiameter(sizeKey, m_ehSel.GetModelRef()) / uor_per_mm; // 钢筋直径
			m_vecWallInsertInfo[i].embedLength = m_vecWallInsertInfo[i].slabThickness - m_vecWallInsertInfo[i].postiveCover - m_vecWallInsertInfo[i].slabDistance - m_diameter * 0.5;
			m_vecWallInsertInfo[i].expandLength = m_vecWallInsertInfoBak[i].expandLength;
			m_vecEndType[i * 2 + 1].rotateAngle = 180.0;
		}
	}

	UpdateRebarList();
	UpdateEndTypeList();
}
