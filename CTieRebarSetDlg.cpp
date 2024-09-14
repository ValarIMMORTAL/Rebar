// CTieRebarSetDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CTieRebarSetDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include "CWallRebarDlg.h"
#include "ElementAttribute.h"


// CTieRebarSetDlg 对话框

IMPLEMENT_DYNAMIC(CTieRebarSetDlg, CDialogEx)

CTieRebarSetDlg::CTieRebarSetDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_WallRebar_TieRebarSet, pParent)
{
	m_isDisplayCheck = false;
	m_isContin = false;
}

CTieRebarSetDlg::~CTieRebarSetDlg()
{
}

void CTieRebarSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_ComboRebarType);
	DDX_Control(pDX, IDC_COMBO2, m_ComboRebarSize);
	DDX_Control(pDX, IDC_CHECK1, m_CheckPatch);
	DDX_Control(pDX, IDC_COMBO_RebarStyle, m_ComboTieRebarMethod);
	DDX_Control(pDX, IDC_STATIC_RebarSpace1, m_staticSpace1);
	DDX_Control(pDX, IDC_STATIC_RebarSpace2, m_staticSpace2);
	DDX_Control(pDX, IDC_CHECK2, m_IsContinCheck);
}


// CTieRebarSetDlg 消息处理程序
void CTieRebarSetDlg::InitUIData()
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
		strTextRebarSpace2.Format(_T("D2 = %.2f"), m_vecRebarData[m_vecRebarData.size()-1].spacing);
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

	if (!m_isDisplayCheck)
	{
		GetDlgItem(IDC_CHECK2)->ShowWindow(SW_HIDE);
		m_IsContinCheck.SetCheck(m_isContin);
	}

	m_ComboTieRebarMethod.SetCurSel(m_tieRebarInfo.tieRebarMethod);	//设置拉筋方式
	m_CheckPatch.SetCheck(m_tieRebarInfo.isPatch);					//默认设置补齐拉筋

	CString strRow, strCol;
	strRow.Format(L"%d", m_tieRebarInfo.rowInterval);
	strCol.Format(L"%d", m_tieRebarInfo.colInterval);
	GetDlgItem(IDC_EDIT1)->SetWindowText(strRow);
	GetDlgItem(IDC_EDIT2)->SetWindowText(strCol);

// 	SetDlgItemText(IDC_STATIC_RebarSpace1, strTextRebarSpace1);
// 	SetDlgItemText(IDC_STATIC_RebarSpace2, strTextRebarSpace2);
	UINT itemId = IDC_RADIO1;
	itemId = m_tieRebarInfo.tieRebarStyle == 0 ? IDC_RADIO1 : m_tieRebarInfo.tieRebarStyle == 1 ? IDC_RADIO2 : m_tieRebarInfo.tieRebarStyle == 2 ? IDC_RADIO3 : m_tieRebarInfo.tieRebarStyle == 3 ? IDC_RADIO4 : IDC_RADIO5;
	((CButton*)GetDlgItem(itemId))->SetCheck(true);	//默认设置X*X
}

void CTieRebarSetDlg::GetTieRebarData(TieReBarInfo & tieRebarInfo)
{
	tieRebarInfo = m_tieRebarInfo;
}

void CTieRebarSetDlg::SetTieRebarData(const TieReBarInfo & tieRebarInfo)
{
	m_tieRebarInfo = tieRebarInfo;
}

BEGIN_MESSAGE_MAP(CTieRebarSetDlg, CDialogEx)
	ON_BN_CLICKED(IDC_RADIO1, &CTieRebarSetDlg::OnBnClickedRadio1)
	ON_BN_CLICKED(IDC_RADIO3, &CTieRebarSetDlg::OnBnClickedRadio3)
	ON_BN_CLICKED(IDC_RADIO2, &CTieRebarSetDlg::OnBnClickedRadio2)
	ON_BN_CLICKED(IDC_RADIO4, &CTieRebarSetDlg::OnBnClickedRadio4)
	ON_BN_CLICKED(IDC_CHECK1, &CTieRebarSetDlg::OnBnClickedCheck1)
//	ON_BN_CLICKED(IDC_CHECK_IsTieRebar, &CTieRebarSetDlg::OnBnClickedCheckIstierebar)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CTieRebarSetDlg::OnCbnSelchangeCombo1)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CTieRebarSetDlg::OnCbnSelchangeCombo2)
	ON_EN_KILLFOCUS(IDC_EDIT1, &CTieRebarSetDlg::OnEnKillfocusEdit1)
	ON_EN_KILLFOCUS(IDC_EDIT2, &CTieRebarSetDlg::OnEnKillfocusEdit2)
	ON_BN_CLICKED(IDC_RADIO5, &CTieRebarSetDlg::OnBnClickedRadio5)
	ON_CBN_SELCHANGE(IDC_COMBO_RebarStyle, &CTieRebarSetDlg::OnCbnSelchangeComboRebarstyle)
	ON_BN_CLICKED(IDC_CHECK2, &CTieRebarSetDlg::OnBnClickedCheck2)
END_MESSAGE_MAP()


// CWallRebarEndType 消息处理程序
BOOL CTieRebarSetDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	InitUIData();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CTieRebarSetDlg::OnBnClickedRadio1()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarStyle = 0;
	GetDlgItem(IDC_EDIT1)->SetWindowText(_T("0"));
	GetDlgItem(IDC_EDIT2)->SetWindowText(_T("0"));
	m_tieRebarInfo.rowInterval = 0;
	m_tieRebarInfo.colInterval = 0;
}


void CTieRebarSetDlg::OnBnClickedRadio3()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarStyle = 2;
	GetDlgItem(IDC_EDIT1)->SetWindowText(_T("0"));
	GetDlgItem(IDC_EDIT2)->SetWindowText(_T("1"));
	m_tieRebarInfo.rowInterval = 0;
	m_tieRebarInfo.colInterval = 1;
}


void CTieRebarSetDlg::OnBnClickedRadio2()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarStyle = 1;
	GetDlgItem(IDC_EDIT1)->SetWindowText(_T("1"));
	GetDlgItem(IDC_EDIT2)->SetWindowText(_T("0"));
	m_tieRebarInfo.rowInterval = 1;
	m_tieRebarInfo.colInterval = 0;
}


void CTieRebarSetDlg::OnBnClickedRadio4()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarStyle = 3;
	GetDlgItem(IDC_EDIT1)->SetWindowText(_T("1"));
	GetDlgItem(IDC_EDIT2)->SetWindowText(_T("1"));
	m_tieRebarInfo.rowInterval = 1;
	m_tieRebarInfo.colInterval = 1;
}

//自定义
void CTieRebarSetDlg::OnBnClickedRadio5()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarStyle = 4;
}


void CTieRebarSetDlg::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.isPatch = true;
}


void CTieRebarSetDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.rebarType = m_ComboRebarType.GetCurSel();
}


void CTieRebarSetDlg::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboRebarSize.GetCurSel());
	strcpy(m_tieRebarInfo.rebarSize, CT2A(*it));
}


void CTieRebarSetDlg::OnEnKillfocusEdit1()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strRow;
	GetDlgItem(IDC_EDIT1)->GetWindowText(strRow);
	m_tieRebarInfo.rowInterval = atoi(CT2A(strRow));
}


void CTieRebarSetDlg::OnEnKillfocusEdit2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strCol;
	GetDlgItem(IDC_EDIT2)->GetWindowText(strCol);
	m_tieRebarInfo.colInterval = atoi(CT2A(strCol));
}


void CTieRebarSetDlg::OnCbnSelchangeComboRebarstyle()
{
	// TODO: 在此添加控件通知处理程序代码
	m_tieRebarInfo.tieRebarMethod = m_ComboTieRebarMethod.GetCurSel();
}


void CTieRebarSetDlg::OnBnClickedCheck2()
{
	if (m_IsContinCheck.GetCheck() == 0)
	{
		m_isContin = false;
	}
	else
	{
		m_isContin = true;
	}
}
