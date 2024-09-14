// CWallRebarAssociatedComponent.cpp: 实现文件
//

#include "_USTATION.h"
#include "CWallRebarAssociatedComponentDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include "ScanIntersectTool.h"
#include "CommonFile.h"
#include "AssociatedComponent.h"
#include "ElementAttribute.h"

#define WM_UPDATELIST  WM_USER+1   // do something  

// CWallRebarAssociatedComponentDlg 对话框
IMPLEMENT_DYNAMIC(CWallRebarAssociatedComponentDlg, CDialogEx)

CWallRebarAssociatedComponentDlg::CWallRebarAssociatedComponentDlg(ElementHandleCR eeh, CWnd * pParent)
	: CDialogEx(IDD_DIALOG_WallRebar_AssociatedComponent, pParent),m_ehSel(eeh),m_pACC(new CAssociatedComponent(eeh))
{
	InitACCData();
}

CWallRebarAssociatedComponentDlg::~CWallRebarAssociatedComponentDlg()
{
	
}

void CWallRebarAssociatedComponentDlg::InitACCData()
{
	if (m_vecAC.size() > 0)
	{
		return;
	}

	EleIntersectDatas Same_Eles;
	Same_Eles.nowEleh = m_ehSel;
	Same_Eles.InSDatas = m_pACC->GetACCData();
//	const vector<IntersectEle>& vecBothEndACCData = m_pACC->GetACCData();
	const vector<IntersectEle>& vecIntersectSlab = m_pACC->GetIntersectSlab();
	m_ACCData.clear();
	m_ACCData.shrink_to_fit();
	m_ACCData.insert(m_ACCData.begin(), Same_Eles.InSDatas.begin(), Same_Eles.InSDatas.end());
//	m_ACCData.insert(m_ACCData.end(), vecIntersectSlab.begin(), vecIntersectSlab.end());
	ElementId curConcreteId = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), curConcreteId, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	vector<PIT::AssociatedComponent> vecACData;
	GetElementXAttribute(curConcreteId, vecACData, vecACDataXAttribute, ACTIVEMODEL);
	for (size_t i = 0; i < Same_Eles.InSDatas.size(); ++i)
	{
		const IntersectEle& tmphd = Same_Eles.InSDatas[i];
		PIT::AssociatedComponent oneACData;
		string strElmName, strElmType;
		ElementHandle eeh(m_ehSel);
		GetEleNameAndType(eeh, strElmName, strElmType);
		strcpy(oneACData.CurrentWallName, strElmName.c_str());
		strcpy(oneACData.associatedComponentName, tmphd.EleName.c_str());
		oneACData.mutualRelation = tmphd.m_EleTyp;
		//			oneACData.rebarLevel = 0;
		if (vecACData.size() == Same_Eles.InSDatas.size())
		{
			oneACData.associatedRelation = vecACData[i].associatedRelation;
			oneACData.anchoringMethod = vecACData[i].anchoringMethod;
			oneACData.endOffset = vecACData[i].endOffset;
			oneACData.acPositon = vecACData[i].acPositon;
			oneACData.startL0 = vecACData[i].startL0;
			oneACData.endL0 = vecACData[i].endL0;
			oneACData.La = vecACData[i].La;
			oneACData.isReverse = vecACData[i].isReverse;
			oneACData.isCut = vecACData[i].isCut;
		}
		else
		{
			oneACData.associatedRelation = 0;
			oneACData.anchoringMethod = 0;
			oneACData.endOffset = 0;
			oneACData.acPositon = -1;
			oneACData.startL0 = 0;
			oneACData.endL0 = 0;
			oneACData.La = 0;
			oneACData.isReverse = 0;
			oneACData.isCut = 0;
		}
		//		string strElmName = tmphd.EleName;
		if (std::find_if(m_ACCData.begin(), m_ACCData.end(), [&tmphd](const IntersectEle &tmphd1) {return tmphd1.EleName == tmphd.EleName; }) != m_ACCData.end())
			oneACData.isEndBothACC = 1;
		else
			oneACData.isEndBothACC = 0;

		//设置端部偏移
		vector<PIT::ConcreteRebar> vecRebarInfo;
		ElementId testid = 0;
		GetElementXAttribute(tmphd.Eh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, tmphd.Eh.GetModelRef());
		GetElementXAttribute(testid, vecRebarInfo, vecRebarDataXAttribute, ACTIVEMODEL);
		if (vecRebarInfo.size() > 0)
		{
			double tmpOffset = 0;
			for (int k =0;k<vecRebarInfo.size()/2;k++)
			{
				BrString rebarsize(vecRebarInfo[k].rebarSize);
				int rtype = vecRebarInfo[k].rebarType;
				GetDiameterAddType(rebarsize, rtype);
				double diameter = RebarCode::GetBarDiameter(rebarsize, ACTIVEMODEL);
				tmpOffset = tmpOffset + diameter/UOR_PER_MilliMeter;
			}
			oneACData.endOffset = tmpOffset;
		}
		//end

		m_vecAC.push_back(oneACData);
	}
}

void CWallRebarAssociatedComponentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_AC, m_ListAC);
	DDX_Control(pDX, IDC_STATIC_Anchoring, m_ctrlAnchoring);
}

void CWallRebarAssociatedComponentDlg::InitUIData()
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
	m_ListAC.SetExtendedStyle(dwStyle);					// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_ListAC.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_ListAC.InsertColumn(0, _T("待配筋墙"), (int)(width / 5), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListAC.InsertColumn(1, _T("关联构件"), (int)(width / 5), ListCtrlEx::Button, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListAC.InsertColumn(2, _T("相互关系"), (int)(width / 5), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListAC.InsertColumn(3, _T("关联关系"), (int)(width / 5), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListAC.InsertColumn(4, _T("锚固连接方式"), (int)((width / 5) * 0.95), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

//	vector<PIT::AssociatedComponent> vecACData;
// 	if (m_vecAC.size() == 0)
// 	{
// 		InitACCData();
// 	}
	
//	SetListRowData(g_vecACData);

	UpdateACList();
}

void CWallRebarAssociatedComponentDlg::SetListDefaultData()
{
}

void CWallRebarAssociatedComponentDlg::UpdateACList()
{
	m_ListAC.DeleteAllItems();
	SetListDefaultData();

	for (int i = 0; i < (int)m_vecAC.size(); ++i)
	{
		m_ListAC.InsertItem(i, _T("")); // 插入行
		m_ListAC.SetItemText(i, 0, CString(m_vecAC[i].CurrentWallName));
		for (int k = 1; k < 6; ++k)
		{
			CString strValue;
			switch (k)
			{
			case 1:
				strValue = m_vecAC[i].associatedComponentName;;
//				m_ListAC.SetCellEnabled(i, k, m_vecAC[i].isEndBothACC);
				break;
			case 2:
			{
				ListCtrlEx::CStrList strlist = g_listACRelation;
				m_ListAC.SetCellStringList(i, k, strlist);
				auto it = strlist.begin();
				advance(it, m_vecAC[i].mutualRelation);
				strValue = *it;
			}
				break;
			case 3:
			{
				ListCtrlEx::CStrList strlist = g_listACRebarRelation;
				m_ListAC.SetCellStringList(i, k, strlist);
				auto it = strlist.begin();
				advance(it, m_vecAC[i].associatedRelation);
				strValue = *it;
//				m_ListAC.SetCellEnabled(i, k, m_vecAC[i].isEndBothACC);
			}
				break;
			case 4:
			{
				ListCtrlEx::CStrList strlist = g_listACMethod;
				m_ListAC.SetCellStringList(i, k, strlist);
				auto it = strlist.begin();
				advance(it, m_vecAC[i].anchoringMethod);
				strValue = *it;
//				m_ListAC.SetCellEnabled(i, k, m_vecAC[i].isEndBothACC);
			}
				break;
			default:
				break;
			}
			m_ListAC.SetItemText(i, k, strValue);
		}
	}
//	m_ListAC.SetShowProgressPercent(TRUE);
//	m_ListAC.SetSupportSort(TRUE);
}


BEGIN_MESSAGE_MAP(CWallRebarAssociatedComponentDlg, CDialogEx)
	ON_MESSAGE(WM_ListCtrlEx_BUTTON_LBUTTONDOWN, &CWallRebarAssociatedComponentDlg::OnACCButtonDown)
	ON_MESSAGE(WM_ListCtrlEx_LBUTTONDOWN, &CWallRebarAssociatedComponentDlg::OnACCCtrlListLButtonDown)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CWallRebarAssociatedComponentDlg 消息处理程序


BOOL CWallRebarAssociatedComponentDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	InitUIData();
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

LRESULT CWallRebarAssociatedComponentDlg::OnACCButtonDown(WPARAM wParam, LPARAM lParam)
{
	ListCtrlEx::ButtonCellMsg* msg = (ListCtrlEx::ButtonCellMsg*)lParam;
	CString strACCName = m_ListAC.GetItemText(msg->m_nRow, msg->m_nColumn);
	ACCConcrete wallACConcrete = {0,0,0};
	auto it = std::find_if(m_ACCData.begin(), m_ACCData.end(), [&strACCName](const IntersectEle &tmphd) {return tmphd.EleName.c_str() == strACCName; });
	if (it != m_ACCData.end())
	{
		//取出关联构件元素终存储的数据
		int ret = GetElementXAttribute((*it).Eh.GetElementId(), sizeof(ACCConcrete), wallACConcrete, ConcreteCoverXAttribute, (*it).Eh.GetModelRef());
		CACCDataSetDlg accdlg((*it).Eh, wallACConcrete, m_vecAC[msg->m_nRow].endOffset, this);	//使用默认值50
		m_ListAC.GetAllRebarData(m_vecAC);
		accdlg.SetAnchoringMethod(m_vecAC[msg->m_nRow].anchoringMethod);
		if(1 == m_vecAC[msg->m_nRow].acPositon)
			accdlg.SetL0(m_vecAC[msg->m_nRow].endL0);
		else
			accdlg.SetL0(m_vecAC[msg->m_nRow].startL0);
		if (8 == m_vecAC[msg->m_nRow].anchoringMethod || 9 == m_vecAC[msg->m_nRow].anchoringMethod)
		{
			accdlg.SetLa(m_vecAC[msg->m_nRow].La);
			accdlg.SetIsReverse(m_vecAC[msg->m_nRow].isReverse);
		}
		accdlg.SetIsCut(m_vecAC[msg->m_nRow].isCut);

		if (IDOK == accdlg.DoModal())
		{
			wallACConcrete = accdlg.GetACConcreteData();
			m_vecAC[msg->m_nRow].endOffset = accdlg.GetOffset();
			if (0 == m_vecAC[msg->m_nRow].acPositon)
				m_vecAC[msg->m_nRow].startL0 = accdlg.GetL0();
			else if (1 == m_vecAC[msg->m_nRow].acPositon)
				m_vecAC[msg->m_nRow].endL0 = accdlg.GetL0();
			else
			{
				m_vecAC[msg->m_nRow].startL0 = accdlg.GetL0();
				m_vecAC[msg->m_nRow].endL0 = accdlg.GetL0();
			}

			m_vecAC[msg->m_nRow].La = accdlg.GetLa();
			m_vecAC[msg->m_nRow].isReverse = accdlg.GetIsReverse();
			m_vecAC[msg->m_nRow].isCut = accdlg.GetIsCut();
			SetElementXAttribute((*it).Eh.GetElementId(), sizeof(ACCConcrete), &wallACConcrete, ConcreteCoverXAttribute, (*it).Eh.GetModelRef());
		}
	}

	return 0;
}

LRESULT CWallRebarAssociatedComponentDlg::OnACCCtrlListLButtonDown(WPARAM wParam, LPARAM lParam)
{
	ListCtrlEx::CListCtrlEx::CellIndex *cellIndex = (ListCtrlEx::CListCtrlEx::CellIndex*)lParam;
	//动态设置锚固方式示意图
	vector<CString> vecData;
	m_ListAC.GetRowData(cellIndex->first, vecData);
	CBitmap bitmap;  // CBitmap对象，用于加载位图   
	HBITMAP hBmp;    // 保存CBitmap加载的位图的句柄   

	UINT bitmapId = 0;
	if (vecData.size())
	{
		auto find = std::find(g_listACMethod.begin(), g_listACMethod.end(), vecData.back());
		if (find == g_listACMethod.end())
			return 0;
		int iImageIndex = (int)std::distance(g_listACMethod.begin(), find);
		switch (iImageIndex)
		{
		case 0: bitmapId = IDB_BITMAP1; break;
		case 1: bitmapId = IDB_BITMAP2; break;
		case 2: bitmapId = IDB_BITMAP3; break;
		case 3: bitmapId = IDB_BITMAP4; break;
		case 4: bitmapId = IDB_BITMAP5; break;
		case 5: bitmapId = IDB_BITMAP6; break;
		case 6: bitmapId = IDB_BITMAP7; break;
		case 7: bitmapId = IDB_BITMAP8; break;
		case 8: bitmapId = IDB_BITMAP9; break;
		case 9: bitmapId = IDB_BITMAP11; break;
//		case 10: bitmapId = IDB_BITMAP11; break;
		default:
			break;
		}
	}
	bitmap.LoadBitmap(bitmapId);
	// 将位图IDB_BITMAP1加载到bitmap   
	hBmp = (HBITMAP)bitmap.GetSafeHandle();  // 获取bitmap加载位图的句柄   
	m_ctrlAnchoring.SetBitmap(hBmp);
	return 0;
}

void CWallRebarAssociatedComponentDlg::OnDestroy()
{
	m_ListAC.affectedElements.ClearHilite();
	m_ListAC.affectedElements.Clear();
	m_ListAC.GetAllRebarData(m_vecAC);

	for (size_t i = 0; i < m_ACCData.size(); ++i)
	{
		string strACCName = m_ACCData[i].EleName;
		auto it = std::find_if(m_vecAC.begin(), m_vecAC.end(), [&strACCName](const PIT::AssociatedComponent &tmphd) {return string(tmphd.associatedComponentName) == strACCName; });
		if (it != m_vecAC.end())
			(*it).isEndBothACC = 1;
	}
	//	m_vecAC = g_vecACData;
	CDialogEx::OnDestroy();
	// TODO: 在此处添加消息处理程序代码
	//写入数据
//	SetElementXAttribute(m_ehSel.GetElementId(), m_vecAC, ConcreteCoverXAttribute, m_ehSel.GetModelRef());
}


void CWallRebarAssociatedComponentDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_ListAC.affectedElements.ClearHilite();
	m_ListAC.affectedElements.Clear();
	m_ListAC.GetAllRebarData(m_vecAC);
	for (size_t i = 0; i < m_ACCData.size(); ++i)
	{
		string strACCName = m_ACCData[i].EleName;
		auto it = std::find_if(m_vecAC.begin(), m_vecAC.end(), [&strACCName](const PIT::AssociatedComponent &tmphd) {return string(tmphd.associatedComponentName) == strACCName; });
		if (it != m_vecAC.end())
			(*it).isEndBothACC = 1;
	}

	ElementId curConcreteId = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), curConcreteId, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	SetElementXAttribute(curConcreteId, m_vecAC, vecACDataXAttribute, ACTIVEMODEL);

	CDialogEx::OnClose();
}


//BOOL CWallRebarAssociatedComponentDlg::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
// {
//	if (message == WM_SHOWWINDOW)
//	{
//		InitACCData();
//		UpdateACList();
//	}
//	return CDialogEx::OnWndMsg(message, wParam, lParam, pResult);
//}