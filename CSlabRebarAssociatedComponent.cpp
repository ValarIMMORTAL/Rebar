// CSlabRebarAssociatedComponent.cpp: 实现文件 关联构件
//
#include "_USTATION.h"
#include "CSlabRebarAssociatedComponent.h"
#include "afxdialogex.h"


#include "resource.h"
#include "ConstantsDef.h"
#include "ScanIntersectTool.h"
#include "CommonFile.h"

// CSlabRebarAssociatedComponent 对话框

IMPLEMENT_DYNAMIC(CSlabRebarAssociatedComponent, CDialogEx)

CSlabRebarAssociatedComponent::CSlabRebarAssociatedComponent(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_SlabRebar_AssociatedComponent, pParent)
{

}

CSlabRebarAssociatedComponent::~CSlabRebarAssociatedComponent()
{
}

void CSlabRebarAssociatedComponent::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_S_LIST_AC, m_ListAC);
}

void CSlabRebarAssociatedComponent::InitUIData()
{
	// TODO:  在此添加额外的初始化
	LONG lStyle;
	lStyle = GetWindowLong(m_ListAC.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_ListAC.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_ListAC.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_ListAC.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_ListAC.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_ListAC.InsertColumn(0, _T("待配筋墙"), (int)(width / 5), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListAC.InsertColumn(1, _T("关联构件"), (int)(width / 5), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListAC.InsertColumn(2, _T("相互关系"), (int)(width / 5), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	// m_ListAC.InsertColumn(3, _T("待配筋钢筋层"), (int)(width / 6), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListAC.InsertColumn(3, _T("关联关系"), (int)(width / 5), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListAC.InsertColumn(4, _T("锚固连接方式"), (int)(width / 5 ), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

	vector<PIT::AssociatedComponent> vecACData;
	EleIntersectDatas Same_Eles;
	GetSameIntersectDatas(m_ehSel, Same_Eles);
	for (IntersectEle tmphd : Same_Eles.InSDatas)
	{
		PIT::AssociatedComponent oneACData;
		string strElmName, strElmType;
		ElementHandle eeh(m_ehSel);
		GetEleNameAndType(eeh, strElmName, strElmType);
		strcpy(oneACData.CurrentWallName, strElmName.c_str());
		strcpy(oneACData.associatedComponentName, tmphd.EleName.c_str());
		oneACData.mutualRelation = tmphd.m_EleTyp;
		if (oneACData.mutualRelation == 0 && (tmphd.EleName.find("EB") != string::npos || tmphd.EleName.find("NB") != string::npos || tmphd.EleName.find("DB") != string::npos))
			continue;
//		oneACData.rebarLevel = 0;
		oneACData.associatedRelation = 0;
		oneACData.anchoringMethod = 0;
		vecACData.push_back(oneACData);
	}

	EleIntersectDatas Up_Eles;
	EleIntersectDatas Down_Eles;
	GetUpAndDownIntersectDatas(m_ehSel, Up_Eles, Down_Eles);
	for (IntersectEle tmphd : Up_Eles.InSDatas)
	{
		PIT::AssociatedComponent oneACData;
		string strElmName, strElmType;
		ElementHandle eeh(m_ehSel);
		GetEleNameAndType(eeh, strElmName, strElmType);
		strcpy(oneACData.CurrentWallName, strElmName.c_str());
		strcpy(oneACData.associatedComponentName, tmphd.EleName.c_str());
		oneACData.mutualRelation = tmphd.m_EleTyp;
		if (oneACData.mutualRelation == 0 && (tmphd.EleName.find("EB") != string::npos || tmphd.EleName.find("NB") != string::npos))
			continue;
//		oneACData.rebarLevel = 0;
		oneACData.associatedRelation = 0;
		oneACData.anchoringMethod = 0;
		vecACData.push_back(oneACData);
	}

	for (IntersectEle tmphd : Down_Eles.InSDatas)
	{
		PIT::AssociatedComponent oneACData;
		string strElmName, strElmType;
		ElementHandle eeh(m_ehSel);
		GetEleNameAndType(eeh, strElmName, strElmType);
		strcpy(oneACData.CurrentWallName, strElmName.c_str());
		strcpy(oneACData.associatedComponentName, tmphd.EleName.c_str());
		oneACData.mutualRelation = tmphd.m_EleTyp;
		if (oneACData.mutualRelation == 0 && (tmphd.EleName.find("EB") != string::npos || tmphd.EleName.find("NB") != string::npos))
			continue;
//		oneACData.rebarLevel = 0;
		oneACData.associatedRelation = 0;
		oneACData.anchoringMethod = 0;
		vecACData.push_back(oneACData);
	}
	SetListRowData(vecACData);

	UpdateACList();
}

void CSlabRebarAssociatedComponent::SetListDefaultData()
{
}

void CSlabRebarAssociatedComponent::UpdateACList()
{
	m_ListAC.DeleteAllItems();
	SetListDefaultData();

	int nItemNum = 0;
	for (int i = 0; i < (int)m_vecAC.size(); ++i)
	{
		for (int j = 0; j < g_wallRebarInfo.concrete.rebarLevelNum; ++j)
		{
			m_ListAC.InsertItem(nItemNum, _T("")); // 插入行
			m_ListAC.SetItemText(nItemNum, 0, CString(m_vecAC[i].CurrentWallName));
			for (int k = 1; k < 6; ++k)
			{
				CString strValue;
				switch (k)
				{
				case 1:
					strValue = m_vecAC[i].associatedComponentName;;
					break;
				case 2:
				{
					ListCtrlEx::CStrList strlist = g_listACRelation;
					m_ListAC.SetCellStringList(i, j, strlist);
					auto it = strlist.begin();
					advance(it, m_vecAC[i].mutualRelation);
					strValue = *it;
				}
				break;
				case 3:
				{
					switch (m_vecAC[i].mutualRelation)
					{
					case 0:
						strValue.Format(_T("水平%dL主筋"), j + 1);
						break;
					case 1:
					case 2:
					case 3:
					case 4:
						strValue.Format(_T("竖向%dL主筋"), j + 1);
						break;
					default:
						break;
					}
				}
				break;
				case 4:
				{
					ListCtrlEx::CStrList strlist = g_listACRebarRelation;
					m_ListAC.SetCellStringList(i, j, strlist);
					auto it = strlist.begin();
					advance(it, m_vecAC[i].associatedRelation);
					strValue = *it;
				}
				break;
				case 5:
				{
					ListCtrlEx::CStrList strlist = g_listACMethod;
					m_ListAC.SetCellStringList(i, j, strlist);
					auto it = strlist.begin();
					advance(it, m_vecAC[i].anchoringMethod);
					strValue = *it;
				}
				break;
				default:
					break;
				}
				m_ListAC.SetItemText(nItemNum, k, strValue);
			}
			nItemNum++;
		}

	}
//	m_ListAC.SetShowProgressPercent(TRUE);
//	m_ListAC.SetSupportSort(TRUE);
}


BEGIN_MESSAGE_MAP(CSlabRebarAssociatedComponent, CDialogEx)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_S_LIST_AC, &CSlabRebarAssociatedComponent::OnLvnItemchangedSListAc)
END_MESSAGE_MAP()


// CWallRebarAssociatedComponentDlg 消息处理程序


BOOL CSlabRebarAssociatedComponent::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	InitUIData();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CSlabRebarAssociatedComponent::OnLvnItemchangedSListAc(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}
