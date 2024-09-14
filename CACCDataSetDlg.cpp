// CACCDataSetDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CACCDataSetDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ElementAttribute.h"
#include "CommonFile.h"

// CACCDataSetDlg 对话框

IMPLEMENT_DYNAMIC(CACCDataSetDlg, CDialogEx)

CACCDataSetDlg::CACCDataSetDlg(ElementHandleCR eh , const ACCConcrete &concrete, double offset,CWnd * pParent)
	: CDialogEx(IDD_DIALOG_ACC_INFO, pParent), m_eh(eh), m_concrete(concrete), m_offset(offset), m_anchoringMethod(0),m_L0(0.0),m_La(0.0),m_IsReverse(0), m_IsCut(0)
{
}

CACCDataSetDlg::~CACCDataSetDlg()
{
}

void CACCDataSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_ACC1, m_ctrlCheck1);
	DDX_Control(pDX, IDC_CHECK_ACC2, m_ctrlCheck2);
}


BEGIN_MESSAGE_MAP(CACCDataSetDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CACCDataSetDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CACCDataSetDlg 消息处理程序


BOOL CACCDataSetDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CString strCover1, strCover2, strCover3,strOffset;
	if (COMPARE_VALUES(m_concrete.postiveOrTopCover, 0) == 0
		&& COMPARE_VALUES(m_concrete.reverseOrBottomCover, 0) == 0
		&& COMPARE_VALUES(m_concrete.postiveOrTopCover, 0) == 0)
	{
		m_concrete = {50,50,50};
	}

	strCover1.Format(L"%.2f", m_concrete.postiveOrTopCover);
	strCover2.Format(L"%.2f", m_concrete.reverseOrBottomCover);
	strCover3.Format(L"%.2f", m_concrete.sideCover);
	strOffset.Format(L"%.2f", m_offset);
	GetDlgItem(IDC_EDIT_ACC1)->SetWindowTextW(strCover1);
	GetDlgItem(IDC_EDIT_ACC2)->SetWindowTextW(strCover2);
	GetDlgItem(IDC_EDIT_ACC3)->SetWindowTextW(strCover3);
	GetDlgItem(IDC_EDIT_ACC4)->SetWindowTextW(strOffset);//默认偏移为正面保护层的值

	m_ctrlCheck1.ShowWindow(FALSE);
	m_ctrlCheck2.ShowWindow(FALSE);
	GetDlgItem(IDC_EDIT_ACC6)->ShowWindow(FALSE);
	GetDlgItem(IDC_STATIC_La)->ShowWindow(FALSE);


	switch (m_anchoringMethod)
	{
	case 0:
		GetDlgItem(IDC_EDIT_ACC5)->SetWindowTextW(L"0");
		GetDlgItem(IDC_EDIT_ACC5)->EnableWindow(FALSE);
		break;
	case 3:
		GetDlgItem(IDC_EDIT_ACC5)->SetWindowTextW(L"0");
		GetDlgItem(IDC_EDIT_ACC5)->EnableWindow(FALSE);
		m_ctrlCheck2.ShowWindow(TRUE);
		m_ctrlCheck2.SetCheck(m_IsCut);
		break;
	case 1:
	{
		CString strL0;
		strL0.Format(L"%.2f", m_L0);
		GetDlgItem(IDC_EDIT_ACC5)->SetWindowTextW(strL0);
		GetDlgItem(IDC_EDIT_ACC5)->EnableWindow(TRUE);
	}
	case 2:
	{
		CString strL0;
		strL0.Format(L"%.2f", m_L0);
		GetDlgItem(IDC_EDIT_ACC5)->SetWindowTextW(strL0);
		GetDlgItem(IDC_EDIT_ACC4)->EnableWindow(TRUE);
	}
	break;
	case 6:
	{
		CString strL0;
		strL0.Format(L"%.2f", m_L0);
		GetDlgItem(IDC_EDIT_ACC5)->SetWindowTextW(strL0);
		GetDlgItem(IDC_EDIT_ACC4)->EnableWindow(TRUE);
	}
	break;
	case 8:
	case 9:
	{
		CString strL0,strLa;
		strL0.Format(L"%.2f", m_L0);
		strLa.Format(L"%.2f", m_La);
		GetDlgItem(IDC_EDIT_ACC5)->SetWindowTextW(strL0);
		GetDlgItem(IDC_EDIT_ACC4)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT_ACC6)->ShowWindow(TRUE);
		GetDlgItem(IDC_STATIC_La)->ShowWindow(TRUE);
		GetDlgItem(IDC_EDIT_ACC6)->SetWindowTextW(strLa);
		m_ctrlCheck1.ShowWindow(TRUE);
		m_ctrlCheck1.SetCheck(m_IsReverse);
	}
	break;
	default:
		break;
	}
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CACCDataSetDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strCover1, strCover2, strCover3, strOffset;
	GetDlgItem(IDC_EDIT_ACC1)->GetWindowTextW(strCover1);
	GetDlgItem(IDC_EDIT_ACC2)->GetWindowTextW(strCover2);
	GetDlgItem(IDC_EDIT_ACC3)->GetWindowTextW(strCover3);
	GetDlgItem(IDC_EDIT_ACC4)->GetWindowTextW(strOffset);

	m_concrete.postiveOrTopCover = atof(CT2A(strCover1));
	m_concrete.reverseOrBottomCover = atof(CT2A(strCover2));
	m_concrete.sideCover = atof(CT2A(strCover3));
	m_offset = atof(CT2A(strOffset));
	switch (m_anchoringMethod)
	{
	case 0:
		break;
	case 1:
	case 2:
	case 6:
	{
		CString strL0;
		GetDlgItem(IDC_EDIT_ACC5)->GetWindowTextW(strL0);
		m_L0 = atof(CT2A(strL0));
	}
		break;
	case 3:
		m_IsCut = m_ctrlCheck2.GetCheck();
	break;
	case 8:
	case 9:
	{
		CString strL0,strLa;
		GetDlgItem(IDC_EDIT_ACC5)->GetWindowTextW(strL0);
		GetDlgItem(IDC_EDIT_ACC6)->GetWindowTextW(strLa);
		m_L0 = atof(CT2A(strL0));
		m_La = atof(CT2A(strLa));
		m_IsReverse = m_ctrlCheck1.GetCheck();
	}
	break;
	default:
		break;
	}

	CDialogEx::OnOK();
}
