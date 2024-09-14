// CSlabTieRebarSetDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CSlabTieRebarSetDlg.h"
#include "afxdialogex.h"

#include "resource.h"
#include "ConstantsDef.h"
#include "CWallRebarDlg.h"

// CSlabTieRebarSetDlg 对话框

IMPLEMENT_DYNAMIC(CSlabTieRebarSetDlg, CDialogEx)

CSlabTieRebarSetDlg::CSlabTieRebarSetDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_SlabRebar_TieRebarSet, pParent)
{

}

CSlabTieRebarSetDlg::~CSlabTieRebarSetDlg()
{
}

void CSlabTieRebarSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_S_COMBO1, m_ComboRebarType);
	DDX_Control(pDX, IDC_S_COMBO2, m_ComboRebarSize);
	DDX_Control(pDX, IDC_S_CHECK1, m_CheckPatch);
	DDX_Control(pDX, IDC_S_COMBO_RebarStyle, m_ComboTieRebarMethod);
	DDX_Control(pDX, IDC_S_STATIC_RebarSpace1, m_staticSpace1);
	DDX_Control(pDX, IDC_S_STATIC_RebarSpace2, m_staticSpace2);
}

void CSlabTieRebarSetDlg::InitUIData()
{
	// SetTieRebarData(g_tieRebarInfo);

	m_ComboTieRebarMethod.ResetContent();
	m_ComboRebarType.ResetContent();
	m_ComboRebarSize.ResetContent();

	if (m_ComboTieRebarMethod)
	for each (auto var in g_listTieRebarStyle)
		m_ComboTieRebarMethod.AddString(var);

	for each (auto var in g_listRebarType)
		m_ComboRebarType.AddString(var);

	for each (auto var in g_listRebarSize)
		m_ComboRebarSize.AddString(var);

	CString strTextRebarSpace1, strTextRebarSpace2;
	SetDefaultRebarData();
	if (m_vecRebarData.size())
	{
		if (CString(m_tieRebarInfo.rebarSize) == "")
			strcpy(m_tieRebarInfo.rebarSize, m_vecRebarData[0].rebarSize);
		strTextRebarSpace1.Format(_T("D1 = %.2f"), m_vecRebarData[0].spacing);
		strTextRebarSpace2.Format(_T("D2 = %.2f"), m_vecRebarData[m_vecRebarData.size() - 1].spacing);
		m_ComboRebarType.SetCurSel(m_tieRebarInfo.rebarType);
		CString strRebarSize(m_tieRebarInfo.rebarSize);
		if (strRebarSize.Find(L"mm") == -1)
			strRebarSize += "mm";

		int nIndex = m_ComboRebarSize.FindStringExact(0, strRebarSize);
		m_ComboRebarSize.SetCurSel(nIndex);
	}
	else
	{
		strTextRebarSpace1 = _T("0.00");
		strTextRebarSpace2 = _T("0.00");
	}

	m_staticSpace1.SetWindowText(strTextRebarSpace1);
	m_staticSpace2.SetWindowText(strTextRebarSpace2);

	m_ComboTieRebarMethod.SetCurSel(m_tieRebarInfo.tieRebarMethod);	//设置拉筋方式
	m_CheckPatch.SetCheck(m_tieRebarInfo.isPatch);					//默认设置补齐拉筋

	CString strRow, strCol;
	strRow.Format(L"%d", m_tieRebarInfo.rowInterval);
	strCol.Format(L"%d", m_tieRebarInfo.colInterval);
	GetDlgItem(IDC_S_EDIT1)->SetWindowText(strRow);
	GetDlgItem(IDC_S_EDIT2)->SetWindowText(strCol);

	// 	SetDlgItemText(IDC_STATIC_RebarSpace1, strTextRebarSpace1);
	// 	SetDlgItemText(IDC_STATIC_RebarSpace2, strTextRebarSpace2);
	UINT itemId = IDC_S_RADIO1;
	itemId = m_tieRebarInfo.tieRebarStyle == 0 ? IDC_S_RADIO1 : m_tieRebarInfo.tieRebarStyle == 1 ? IDC_S_RADIO2 : m_tieRebarInfo.tieRebarStyle == 2 ? IDC_S_RADIO3 : m_tieRebarInfo.tieRebarStyle == 3 ? IDC_S_RADIO4 : IDC_S_RADIO5;
	((CButton*)GetDlgItem(itemId))->SetCheck(true);	//默认设置X*X
}

void CSlabTieRebarSetDlg::SetDefaultRebarData()
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
				levelSpace = ((i + 1) & 0x01) * 2000.0;
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
				levelSpace = dir * 2000.0;
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
}

void CSlabTieRebarSetDlg::GetTieRebarData(TieReBarInfo & tieRebarInfo)
{
	tieRebarInfo = m_tieRebarInfo;
}

void CSlabTieRebarSetDlg::SetTieRebarData(const TieReBarInfo & tieRebarInfo)
{
	m_tieRebarInfo = tieRebarInfo;
}
BEGIN_MESSAGE_MAP(CSlabTieRebarSetDlg, CDialogEx)
	//ON_NOTIFY(LVN_ITEMCHANGED, IDC_S_LIST_TieRebar, &CSlabTieRebarSetDlg::OnLvnItemchangedListTierebar)
	ON_BN_CLICKED(IDC_BUTTON1, &CSlabTieRebarSetDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_S_BUTTON1, &CSlabTieRebarSetDlg::OnBnClickedSButton1)
	ON_EN_CHANGE(IDC_S_EDIT1, &CSlabTieRebarSetDlg::OnEnChangeSEdit1)
	ON_CBN_SELCHANGE(IDC_S_COMBO_RebarStyle, &CSlabTieRebarSetDlg::OnCbnSelchangeSComboRebarstyle)
	ON_CBN_SELCHANGE(IDC_S_COMBO1, &CSlabTieRebarSetDlg::OnCbnSelchangeSCombo1)
	ON_CBN_SELCHANGE(IDC_S_COMBO2, &CSlabTieRebarSetDlg::OnCbnSelchangeSCombo2)
	ON_BN_CLICKED(IDC_S_CHECK1, &CSlabTieRebarSetDlg::OnBnClickedSCheck1)
	ON_BN_CLICKED(IDC_S_RADIO1, &CSlabTieRebarSetDlg::OnBnClickedSRadio1)
	ON_BN_CLICKED(IDC_S_RADIO2, &CSlabTieRebarSetDlg::OnBnClickedSRadio2)
	ON_BN_CLICKED(IDC_S_RADIO3, &CSlabTieRebarSetDlg::OnBnClickedSRadio3)
	ON_BN_CLICKED(IDC_S_RADIO4, &CSlabTieRebarSetDlg::OnBnClickedSRadio4)
	ON_BN_CLICKED(IDC_S_RADIO5, &CSlabTieRebarSetDlg::OnBnClickedSRadio5)
	ON_EN_CHANGE(IDC_S_EDIT2, &CSlabTieRebarSetDlg::OnEnChangeSEdit2)
END_MESSAGE_MAP()


// CSlabTieRebarSetDlg 消息处理程序
BOOL CSlabTieRebarSetDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	InitUIData();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CSlabTieRebarSetDlg::OnLvnItemchangedListTierebar(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CSlabTieRebarSetDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CSlabTieRebarSetDlg::OnBnClickedSButton1()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CSlabTieRebarSetDlg::OnEnChangeSEdit1()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strRow;
	GetDlgItem(IDC_S_EDIT1)->GetWindowText(strRow);
	m_tieRebarInfo.rowInterval = atoi(CT2A(strRow));
}


void CSlabTieRebarSetDlg::OnCbnSelchangeSComboRebarstyle()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarMethod = m_ComboTieRebarMethod.GetCurSel();
}


void CSlabTieRebarSetDlg::OnCbnSelchangeSCombo1()
{
	m_tieRebarInfo.rebarType = m_ComboRebarType.GetCurSel();
}


void CSlabTieRebarSetDlg::OnCbnSelchangeSCombo2()
{
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboRebarSize.GetCurSel());
	strcpy(m_tieRebarInfo.rebarSize, CT2A(*it));
}



void CSlabTieRebarSetDlg::OnBnClickedSCheck1()
{
	m_tieRebarInfo.isPatch = true;
}


void CSlabTieRebarSetDlg::OnBnClickedSRadio1()
{
	m_tieRebarInfo.tieRebarStyle = 0;
	GetDlgItem(IDC_S_EDIT1)->SetWindowText(_T("0"));
	GetDlgItem(IDC_S_EDIT2)->SetWindowText(_T("0"));
}


void CSlabTieRebarSetDlg::OnBnClickedSRadio2()
{
	m_tieRebarInfo.tieRebarStyle = 1;
	GetDlgItem(IDC_S_EDIT1)->SetWindowText(_T("1"));
	GetDlgItem(IDC_S_EDIT2)->SetWindowText(_T("0"));
}


void CSlabTieRebarSetDlg::OnBnClickedSRadio3()
{
	m_tieRebarInfo.tieRebarStyle = 2;
	GetDlgItem(IDC_S_EDIT1)->SetWindowText(_T("0"));
	GetDlgItem(IDC_S_EDIT2)->SetWindowText(_T("1"));
}


void CSlabTieRebarSetDlg::OnBnClickedSRadio4()
{
	m_tieRebarInfo.tieRebarStyle = 3;
	GetDlgItem(IDC_S_EDIT1)->SetWindowText(_T("1"));
	GetDlgItem(IDC_S_EDIT2)->SetWindowText(_T("1"));
}


void CSlabTieRebarSetDlg::OnBnClickedSRadio5()
{
	m_tieRebarInfo.tieRebarStyle = 4;
}


void CSlabTieRebarSetDlg::OnEnChangeSEdit2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strCol;
	GetDlgItem(IDC_S_EDIT2)->GetWindowText(strCol);
	m_tieRebarInfo.colInterval = atoi(CT2A(strCol));
}
