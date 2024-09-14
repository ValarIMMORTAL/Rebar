﻿// CDomeRoundDlgEx.cpp: 实现文件
//
#include "_USTATION.h"
#include "CDomeRoundDlgEx.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ElementAttribute.h"
#include "CDomeRebarAssembly.h"
#include "ConstantsDef.h"


// CDomeRoundDlgEx 对话框

IMPLEMENT_DYNAMIC(CDomeRoundDlgEx, CDialogEx)

CDomeRoundDlgEx::CDomeRoundDlgEx(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_DomeRoundDlg, pParent)
{
}

CDomeRoundDlgEx::~CDomeRoundDlgEx()
{
}

BOOL CDomeRoundDlgEx::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	return true;
}


void CDomeRoundDlgEx::SetListDefaultData()
{
	return;
}


void CDomeRoundDlgEx::UpdateRebarList()
{
	return;
}

void CDomeRoundDlgEx::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT1, m_EditCover);
	DDX_Control(pDX, IDC_EDIT2, m_EditZSpace);
	DDX_Control(pDX, IDC_EDIT4, m_EditRebarLevel);

	DDX_Control(pDX, IDC_LIST1, m_listCtl);
}


BEGIN_MESSAGE_MAP(CDomeRoundDlgEx, CDialogEx)
	ON_EN_CHANGE(IDC_EDIT1, &CDomeRoundDlgEx::OnEnChangeEdit1)
	ON_EN_CHANGE(IDC_EDIT2, &CDomeRoundDlgEx::OnEnChangeEdit2)
	ON_EN_CHANGE(IDC_EDIT4, &CDomeRoundDlgEx::OnEnChangeEdit4)
END_MESSAGE_MAP()


// CDomeRoundDlgEx 消息处理程序


void CDomeRoundDlgEx::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString strTmp = CString();
	m_EditCover.GetWindowText(strTmp);
}


void CDomeRoundDlgEx::OnEnChangeEdit2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString strTmp = CString();
	m_EditZSpace.GetWindowText(strTmp);
}


void CDomeRoundDlgEx::OnEnChangeEdit4()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString strTmp = CString();
	m_EditRebarLevel.GetWindowText(strTmp);
}

