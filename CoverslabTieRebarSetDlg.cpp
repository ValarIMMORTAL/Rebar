// CoverslabTieRebarSetDlg.cpp: 实现文件
//
#include "_USTATION.h"
#include "CoverslabTieRebarSetDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include "CoverslabRebarDlg.h"
#include "ElementAttribute.h"
// CoverslabTieRebarSetDlg 对话框

IMPLEMENT_DYNAMIC(CoverslabTieRebarSetDlg, CDialogEx)

CoverslabTieRebarSetDlg::CoverslabTieRebarSetDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CoverslabRebar_TieRebarSet, pParent)
{

}

CoverslabTieRebarSetDlg::~CoverslabTieRebarSetDlg()
{
}

void CoverslabTieRebarSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_C_COMBO1, m_ComboRebarType);
	DDX_Control(pDX, IDC_C_COMBO2, m_ComboRebarSize);
	DDX_Control(pDX, IDC_C_CHECK1, m_CheckPatch);
	DDX_Control(pDX, IDC_C_COMBO_RebarStyle, m_ComboTieRebarMethod);
	DDX_Control(pDX, IDC_C_STATIC_RebarSpace1, m_staticSpace1);
	DDX_Control(pDX, IDC_C_STATIC_RebarSpace2, m_staticSpace2);
}

void CoverslabTieRebarSetDlg::InitUIData()
{
	//	SetTieRebarData(g_tieRebarInfo);
	m_ComboTieRebarMethod.ResetContent();
	m_ComboRebarType.ResetContent();
	m_ComboRebarSize.ResetContent();

	for each (auto var in g_listTieRebarStyle)
		m_ComboTieRebarMethod.AddString(var);

	for each (auto var in g_listRebarType)
		m_ComboRebarType.AddString(var);

	for each (auto var in g_listRebarSize)
		m_ComboRebarSize.AddString(var);

	CString strTextRebarSpace1, strTextRebarSpace2;
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
	GetDlgItem(IDC_C_EDIT1)->SetWindowText(strRow);
	GetDlgItem(IDC_C_EDIT2)->SetWindowText(strCol);

	// 	SetDlgItemText(IDC_STATIC_RebarSpace1, strTextRebarSpace1);
	// 	SetDlgItemText(IDC_STATIC_RebarSpace2, strTextRebarSpace2);
	UINT itemId = IDC_C_RADIO1;
	itemId = m_tieRebarInfo.tieRebarStyle == 0 ? IDC_C_RADIO1 : m_tieRebarInfo.tieRebarStyle == 1 ? IDC_C_RADIO2 : m_tieRebarInfo.tieRebarStyle == 2 ? IDC_C_RADIO3 : m_tieRebarInfo.tieRebarStyle == 3 ? IDC_C_RADIO4 : IDC_C_RADIO5;
	((CButton*)GetDlgItem(itemId))->SetCheck(true);	//默认设置X*X
}

void CoverslabTieRebarSetDlg::GetTieRebarData(TieReBarInfo & tieRebarInfo)
{
	tieRebarInfo = m_tieRebarInfo;
}

void CoverslabTieRebarSetDlg::SetTieRebarData(const TieReBarInfo & tieRebarInfo)
{
	m_tieRebarInfo = tieRebarInfo;
}
BEGIN_MESSAGE_MAP(CoverslabTieRebarSetDlg, CDialogEx)
	ON_CBN_SELCHANGE(IDC_C_COMBO_RebarStyle, &CoverslabTieRebarSetDlg::OnCbnSelchangeCComboRebarstyle)
	ON_CBN_SELCHANGE(IDC_C_COMBO1, &CoverslabTieRebarSetDlg::OnCbnSelchangeCCombo1)
	ON_CBN_SELCHANGE(IDC_C_COMBO2, &CoverslabTieRebarSetDlg::OnCbnSelchangeCCombo2)
	ON_BN_CLICKED(IDC_C_RADIO1, &CoverslabTieRebarSetDlg::OnBnClickedCRadio1)
	ON_BN_CLICKED(IDC_C_RADIO2, &CoverslabTieRebarSetDlg::OnBnClickedCRadio2)
	ON_BN_CLICKED(IDC_C_RADIO3, &CoverslabTieRebarSetDlg::OnBnClickedCRadio3)
	ON_BN_CLICKED(IDC_C_RADIO4, &CoverslabTieRebarSetDlg::OnBnClickedCRadio4)
	ON_BN_CLICKED(IDC_C_RADIO5, &CoverslabTieRebarSetDlg::OnBnClickedCRadio5)
	ON_BN_CLICKED(IDC_C_CHECK1, &CoverslabTieRebarSetDlg::OnBnClickedCCheck1)
	ON_EN_CHANGE(IDC_C_EDIT1, &CoverslabTieRebarSetDlg::OnEnChangeCEdit1)
	ON_EN_CHANGE(IDC_C_EDIT2, &CoverslabTieRebarSetDlg::OnEnChangeCEdit2)
	ON_BN_CLICKED(IDC_C_BUTTON1, &CoverslabTieRebarSetDlg::OnBnClickedCButton1)
END_MESSAGE_MAP()

BOOL CoverslabTieRebarSetDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	InitUIData();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}



void CoverslabTieRebarSetDlg::OnCbnSelchangeCComboRebarstyle()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarMethod = m_ComboTieRebarMethod.GetCurSel();
}


void CoverslabTieRebarSetDlg::OnCbnSelchangeCCombo1()
{
	m_tieRebarInfo.rebarType = m_ComboRebarType.GetCurSel();
}


void CoverslabTieRebarSetDlg::OnCbnSelchangeCCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboRebarSize.GetCurSel());
	strcpy(m_tieRebarInfo.rebarSize, CT2A(*it));
}


void CoverslabTieRebarSetDlg::OnBnClickedCRadio1()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarStyle = 0;
	GetDlgItem(IDC_C_EDIT1)->SetWindowText(_T("0"));
	GetDlgItem(IDC_C_EDIT2)->SetWindowText(_T("0"));

}


void CoverslabTieRebarSetDlg::OnBnClickedCRadio2()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarStyle = 1;
	GetDlgItem(IDC_C_EDIT1)->SetWindowText(_T("1"));
	GetDlgItem(IDC_C_EDIT2)->SetWindowText(_T("0"));
}


void CoverslabTieRebarSetDlg::OnBnClickedCRadio3()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarStyle = 2;
	GetDlgItem(IDC_C_EDIT1)->SetWindowText(_T("0"));
	GetDlgItem(IDC_C_EDIT2)->SetWindowText(_T("1"));
}


void CoverslabTieRebarSetDlg::OnBnClickedCRadio4()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarStyle = 3;
	GetDlgItem(IDC_C_EDIT1)->SetWindowText(_T("1"));
	GetDlgItem(IDC_C_EDIT2)->SetWindowText(_T("1"));
}


void CoverslabTieRebarSetDlg::OnBnClickedCRadio5()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarStyle = 4;
}


void CoverslabTieRebarSetDlg::OnBnClickedCCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.isPatch = true;
}


void CoverslabTieRebarSetDlg::OnEnChangeCEdit1()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strRow;
	GetDlgItem(IDC_C_EDIT1)->GetWindowText(strRow);
	m_tieRebarInfo.rowInterval = atoi(CT2A(strRow));
}


void CoverslabTieRebarSetDlg::OnEnChangeCEdit2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strCol;
	GetDlgItem(IDC_C_EDIT2)->GetWindowText(strCol);
	m_tieRebarInfo.colInterval = atoi(CT2A(strCol));
}


void CoverslabTieRebarSetDlg::OnBnClickedCButton1()
{
	// TODO: 在此添加控件通知处理程序代码
}
