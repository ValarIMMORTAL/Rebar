// CBeamBaseInfoDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CBeamBaseInfoDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "ElementAttribute.h"
#include "CBeamRebarAssembly.h"


// CBeamBaseInfoDlg 对话框

IMPLEMENT_DYNAMIC(CBeamBaseInfoDlg, CDialogEx)

CBeamBaseInfoDlg::CBeamBaseInfoDlg(ElementHandle ehSel, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_BeamInfo, pParent)
{
	m_ehSel = ehSel;
}

CBeamBaseInfoDlg::~CBeamBaseInfoDlg()
{
}

void CBeamBaseInfoDlg::SetDefaultData(BeamRebarInfo::BeamBaseData& stBeamBaseData)
{
	stBeamBaseData.dWidth = 0.00;
	stBeamBaseData.dDepth = 0.00;
	stBeamBaseData.dLeftSide = 0.00;
	stBeamBaseData.dRightSide = 0.00;
	stBeamBaseData.dLeftXLen = 0.00;
	stBeamBaseData.dRightXLen = 0.00;
	stBeamBaseData.dNetSpan = 0.00;		// 净跨度
	stBeamBaseData.dAxisToAxis = 0.00;  // 轴线到轴线
}

void CBeamBaseInfoDlg::SetBeamBaseData(ElementId contid)
{
	SetElementXAttribute(contid, sizeof(BeamRebarInfo::BeamBaseData), &m_stBeamBaseData, stBeamBaseXAttribute, ACTIVEMODEL);
}

BOOL CBeamBaseInfoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetDefaultData(m_stBeamBaseData);

	ElementId testid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	GetElementXAttribute(testid, sizeof(BeamRebarInfo::BeamBaseData), m_stBeamBaseData, stBeamBaseXAttribute, ACTIVEMODEL);

	if (COMPARE_VALUES(m_stBeamBaseData.dAxisToAxis, 0.00) <= 0)
	{
		CBeamRebarAssembly objBeamRebarAssembly;
		objBeamRebarAssembly.CalcBeamBaseInfo(m_ehSel);
		m_stBeamBaseData.dAxisToAxis = objBeamRebarAssembly.GetBeamInfo().length;
		m_stBeamBaseData.dWidth = objBeamRebarAssembly.GetBeamInfo().width;
		m_stBeamBaseData.dDepth = objBeamRebarAssembly.GetBeamInfo().height;
	}

	CString sTemp = CString();
	sTemp.Format(_T("%.2f"), m_stBeamBaseData.dWidth);
	m_EditWidth.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamBaseData.dDepth);
	m_EditDepth.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamBaseData.dLeftSide);
	m_EditLeftSide.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamBaseData.dRightSide);
	m_EditRightSide.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamBaseData.dLeftXLen);
	m_EditLeftXLen.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamBaseData.dRightXLen);
	m_EditRightXLen.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamBaseData.dNetSpan);
	m_EditNetSpan.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamBaseData.dAxisToAxis);
	m_EditAxisToAxis.SetWindowText(sTemp);

	return TRUE;
}

void CBeamBaseInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT1, m_EditWidth);
	DDX_Control(pDX, IDC_EDIT2, m_EditDepth);
	DDX_Control(pDX, IDC_EDIT3, m_EditLeftSide);
	DDX_Control(pDX, IDC_EDIT4, m_EditRightSide);
	DDX_Control(pDX, IDC_EDIT5, m_EditLeftXLen);
	DDX_Control(pDX, IDC_EDIT6, m_EditRightXLen);
	DDX_Control(pDX, IDC_EDIT7, m_EditNetSpan);
	DDX_Control(pDX, IDC_EDIT8, m_EditAxisToAxis);
}



// CBeamBaseInfoDlg 消息处理程序
BEGIN_MESSAGE_MAP(CBeamBaseInfoDlg, CDialogEx)
	ON_EN_CHANGE(IDC_EDIT1, &CBeamBaseInfoDlg::OnEnChangeEdit1)
	ON_EN_CHANGE(IDC_EDIT2, &CBeamBaseInfoDlg::OnEnChangeEdit2)
	ON_EN_CHANGE(IDC_EDIT3, &CBeamBaseInfoDlg::OnEnChangeEdit3)
	ON_EN_CHANGE(IDC_EDIT4, &CBeamBaseInfoDlg::OnEnChangeEdit4)
	ON_EN_CHANGE(IDC_EDIT5, &CBeamBaseInfoDlg::OnEnChangeEdit5)
	ON_EN_CHANGE(IDC_EDIT6, &CBeamBaseInfoDlg::OnEnChangeEdit6)
	ON_EN_CHANGE(IDC_EDIT7, &CBeamBaseInfoDlg::OnEnChangeEdit7)
	ON_EN_CHANGE(IDC_EDIT8, &CBeamBaseInfoDlg::OnEnChangeEdit8)
END_MESSAGE_MAP()


void CBeamBaseInfoDlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditWidth.GetWindowText(sTemp);
	m_stBeamBaseData.dWidth = atof(CT2A(sTemp));
}

void CBeamBaseInfoDlg::OnEnChangeEdit2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码

	CString sTemp = CString();
	m_EditDepth.GetWindowText(sTemp);
	m_stBeamBaseData.dDepth = atof(CT2A(sTemp));
}

void CBeamBaseInfoDlg::OnEnChangeEdit3()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditLeftSide.GetWindowText(sTemp);
	m_stBeamBaseData.dLeftSide = atof(CT2A(sTemp));
}


void CBeamBaseInfoDlg::OnEnChangeEdit4()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditRightSide.GetWindowText(sTemp);
	m_stBeamBaseData.dRightSide = atof(CT2A(sTemp));
}


void CBeamBaseInfoDlg::OnEnChangeEdit5()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditLeftXLen.GetWindowText(sTemp);
	m_stBeamBaseData.dLeftXLen = atof(CT2A(sTemp));
}


void CBeamBaseInfoDlg::OnEnChangeEdit6()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditRightSide.GetWindowText(sTemp);
	m_stBeamBaseData.dRightSide = atof(CT2A(sTemp));
}


void CBeamBaseInfoDlg::OnEnChangeEdit7()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditNetSpan.GetWindowText(sTemp);
	m_stBeamBaseData.dNetSpan = atof(CT2A(sTemp));
}


void CBeamBaseInfoDlg::OnEnChangeEdit8()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditAxisToAxis.GetWindowText(sTemp);
	m_stBeamBaseData.dAxisToAxis = atof(CT2A(sTemp));
}
