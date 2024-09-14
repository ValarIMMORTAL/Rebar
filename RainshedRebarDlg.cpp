// RainshedRebarDlg.cpp: 实现文件
//
#include "_USTATION.h"
#include "CWallRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "RainshedRebarDlg.h"
#include "RainshedRebar.h"
#include "ElementAttribute.h"
#include "RebarElements.h"
#include "ACCRebarMaker.h"


// RainshedRebarDlg 对话框

IMPLEMENT_DYNAMIC(RainshedRebarDlg, CDialogEx)

RainshedRebarDlg::RainshedRebarDlg(ElementHandleCR eh, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_RainshedRebar, pParent), m_ConcreteId(0), ehSel(eh)
{
	// m_Concrete.rebarLevelNum = 5;
	m_Concrete.postiveCover = 50.0;
	m_Concrete.reverseCover = 50.0;
	m_Concrete.sideCover = 50.0;

	m_RainshedType = 0;
	//ISolidKernelEntityPtr entityPtr;
	//if (SolidUtil::Convert::ElementToBody(entityPtr, ehSel) == SUCCESS)
	//{
	//	bvector<ISubEntityPtr> subEntities;
	//	size_t iSubEntityNum = SolidUtil::GetBodyFaces(&subEntities, *entityPtr);
	//	size_t iSize = subEntities.size();
	//	if (iSize == 6)
	//		m_Concrete.rebarLevelNum = 4;
	//	else
	//		m_Concrete.rebarLevelNum = 5;
	//	SetListDefaultData();
	//}
}

RainshedRebarDlg::~RainshedRebarDlg()
{
}


void RainshedRebarDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

void RainshedRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_R_EDIT1, m_EditPositive);
	DDX_Control(pDX, IDC_R_LIST1, m_listMainRebar);
	DDX_Control(pDX, IDC_R_EDIT2, m_EditSide);
	DDX_Control(pDX, IDC_R_EDIT3, m_EditReverse);
	DDX_Control(pDX, IDC_COMBO1, m_CombRainshedType);
}


BEGIN_MESSAGE_MAP(RainshedRebarDlg, CDialogEx)
	ON_EN_CHANGE(IDC_R_EDIT1, &RainshedRebarDlg::OnEnChangeREdit1)
	ON_EN_CHANGE(IDC_R_EDIT2, &RainshedRebarDlg::OnEnChangeREdit2)
	ON_EN_CHANGE(IDC_R_EDIT3, &RainshedRebarDlg::OnEnChangeREdit3)
	ON_BN_CLICKED(IDOK, &RainshedRebarDlg::OnBnClickedOk)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_R_LIST1, &RainshedRebarDlg::OnLvnItemchangedRList1)

	ON_CBN_SELCHANGE(IDC_COMBO1, &RainshedRebarDlg::OnCbnSelchangeCombo1)
END_MESSAGE_MAP()


// RainshedRebarDlg 消息处理程序



void RainshedRebarDlg::InitUIData()
{
	m_ehSel = ehSel;

	ElementId contid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	if (contid > 0)
	{
		GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		ACCConcrete ACconcrete;
		GetElementXAttribute(contid, sizeof(ACCConcrete), ACconcrete, ConcreteCoverXAttribute, ACTIVEMODEL);
		m_Concrete.postiveCover = ACconcrete.postiveOrTopCover;
		m_Concrete.reverseCover = ACconcrete.reverseOrBottomCover;
		m_Concrete.sideCover = ACconcrete.sideCover;
	}

	m_CombRainshedType.AddString(_T("SPY1"));
	m_CombRainshedType.AddString(_T("SPY2"));
	m_CombRainshedType.AddString(_T("SPY3"));

	m_CombRainshedType.SetCurSel(0);
	m_Concrete.rebarLevelNum = 4;
	if (m_vecRebarData.size() == 5)
	{
		if (strstr(m_vecRebarData[0].rebarSize, "16") != NULL)
		{
			m_CombRainshedType.SetCurSel(1);
		}
		else
		{
			m_CombRainshedType.SetCurSel(2);
		}
		m_Concrete.rebarLevelNum = (int)m_vecRebarData.size();
	}


	CString strLevel, strPositive, strSide, strReverse, strMissHoleSize;
	strLevel.Format(_T("%d"), m_Concrete.rebarLevelNum);

	strPositive.Format(_T("%.2f"), m_Concrete.postiveCover);
	strReverse.Format(_T("%.2f"), m_Concrete.reverseCover);
	strSide.Format(_T("%.2f"), m_Concrete.sideCover);

	m_EditPositive.SetWindowText(strPositive);                          //顶部保护层
	m_EditReverse.SetWindowText(strReverse);                             //底部保护层
	m_EditSide.SetWindowText(strSide);    
	//侧面保护层
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
	m_listMainRebar.InsertColumn(0, _T("层"), 50, ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_listMainRebar.InsertColumn(1, _T("方向"),(int)((width - 50) / 7), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(2, _T("直径"), (int)((width - 50) / 7), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(3, _T("类型"), (int)((width - 50) / 7), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(4, _T("间距"), (int)((width - 50) / 7), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(5, _T("起点偏移"), (int)((width - 50) / 7), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(6, _T("终点偏移"), (int)((width - 50) / 7), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(7, _T("与前层间距"), (int)((width - 50) / 7), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

	//	m_listMainRebar.SetShowProgressPercent(TRUE);
	//	m_listMainRebar.SetSupportSort(TRUE);

	SetListRowData(m_vecRebarData);                    //m_vecRebarData = vecListData;
	UpdateRebarList();

}
void RainshedRebarDlg::SetListDefaultData()                   //添加钢筋的默认层的信息
{
	if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < m_Concrete.rebarLevelNum; ++i)
		{
			ConcreteRebar oneRebarData;

			oneRebarData = { i,0,"12",0,200,0,0,0 };
			if (m_RainshedType == 0)
			{
				double dSpace = 0.00;

				if (i == 0 || i == 3)
				{
					oneRebarData = { i, 1, "14", 2, 200, 50, 50, dSpace};
				}
				else
				{
					if (i == 2)
					{
						dSpace = 200000.0;
					}
					oneRebarData = { i, 0, "8", 0, 200, 50, 50, dSpace };
				}
			}
			else if (m_RainshedType == 1)
			{
				double dSpace = 0.00;
				if (i == 0 || i == 3)
				{
					oneRebarData = { i, 1, "16", 2, 200, 50, 50, dSpace };
				}
				else
				{
					if (i == 2)
					{
						dSpace = 200000.0;
					}
					else if (i == 4)
					{
						dSpace = 50;
					}

					oneRebarData = { i, 0, "8", 0, 200, 50, 50, dSpace };
				}
			}
			else if (m_RainshedType == 2)
			{
				double dSpace = 0.00;
				if (i == 0 || i == 3)
				{
					oneRebarData = { i, 1, "20", 2, 200, 50, 50, dSpace };
				}
				else
				{
					if (i == 2)
					{
						dSpace = 200000.0;
					}
					else if (i == 4)
					{
						dSpace = 50;
					}

					oneRebarData = { i, 0, "8", 0, 200, 50, 50, dSpace };
				}
			}
			m_vecRebarData.push_back(oneRebarData);
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = m_Concrete.rebarLevelNum - (int)m_vecRebarData.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				ConcreteRebar oneRebarData;
				oneRebarData = { i,0,"8",0,200,0,0,0 };
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
}

void RainshedRebarDlg::UpdateRebarList()                      //更新
{
	m_listMainRebar.DeleteAllItems();
	SetListDefaultData();
	for (int i = 0; i < m_Concrete.rebarLevelNum; ++i)
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
		int nSubCnt = m_listMainRebar.GetColumnCount() - 1;                    //返回此表中从 1 开始的列号。
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

BOOL RainshedRebarDlg::OnInitDialog()                 //初始化
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	// TODO:  在此添加额外的初始化
	InitUIData();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}



void RainshedRebarDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();

	m_listMainRebar.GetAllRebarData(m_vecRebarData);
	if (g_ConcreteId)
	{
		g_ConcreteId = 0;
		return;
	}

	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = m_ehSel.GetModelRef();
	tmpModel = tmpModel;

	WallRebarAssembly*  rainshedRebar = NULL;

	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());
	ElementId testid = 0;
	GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
	
	WallRebarAssembly::IsSmartSmartFeature(eeh);

	RebarAssembly* rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class STRainshedRebarAssembly");
	rainshedRebar = dynamic_cast<STRainshedRebarAssembly*>(rebaras);
	if (rainshedRebar == nullptr)
	{
		rainshedRebar = REA::Create<STRainshedRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	}
	rainshedRebar->SetConcreteData(m_Concrete);  //设置三个保护层信息和层数//SetConcreteData函数用引用方式把g_wallRebarInfo.concrete值传到wallRebar
	rainshedRebar->SetWallData(eeh);                     //设置墙坐标
	rainshedRebar->SetRebarData(m_vecRebarData);               //设置钢筋信息
	rainshedRebar->MakeRebars(modelRef);
	rainshedRebar->Save(modelRef); // must save after creating rebars

	ElementId contid = rainshedRebar->FetchConcrete();
	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);
	//eeh2.AddToModel();

	ACCConcrete ACconcrete;

	//修改为最新配筋的保护层数据
	ACconcrete.postiveOrTopCover = m_Concrete.postiveCover;
	ACconcrete.reverseOrBottomCover = m_Concrete.reverseCover;
	ACconcrete.sideCover = m_Concrete.sideCover;
	SetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, sizeof(ACCConcrete), &ACconcrete, ConcreteCoverXAttribute, ACTIVEMODEL);
	DestroyWindow();
}

void RainshedRebarDlg::OnLvnItemchangedRList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}

void RainshedRebarDlg::OnEnChangeREdit1()
{
	CString strPostiveCover;
	m_EditPositive.GetWindowText(strPostiveCover);
	m_Concrete.postiveCover = atof(CT2A(strPostiveCover));
}


void RainshedRebarDlg::OnEnChangeREdit2()
{// TODO: 在此添加控件通知处理程序代码
	CString strSideCover;
	m_EditSide.GetWindowText(strSideCover);
	m_Concrete.sideCover = atof(CT2A(strSideCover));
}


void RainshedRebarDlg::OnEnChangeREdit3()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strReverseCover;
	m_EditReverse.GetWindowText(strReverseCover);
	m_Concrete.reverseCover = atof(CT2A(strReverseCover));
}


void RainshedRebarDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
	DestroyWindow();
}

void RainshedRebarDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	m_RainshedType = m_CombRainshedType.GetCurSel();
	if (m_RainshedType == 0)
	{
		m_Concrete.rebarLevelNum = 4;
	}
	else
	{
		m_Concrete.rebarLevelNum = 5;
	}
	m_vecRebarData.clear();
	UpdateRebarList();
}
