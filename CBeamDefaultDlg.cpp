// CBeamDefaultDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CBeamDefaultDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "ElementAttribute.h"


// CBeamDefaultDlg 对话框

IMPLEMENT_DYNAMIC(CBeamDefaultDlg, CDialogEx)

CBeamDefaultDlg::CBeamDefaultDlg(ElementHandle ehSel, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_BeamDefault, pParent)
{
	arrRebarCal[0] = _T("净跨度");
	arrRebarCal[1] = _T("轴线到轴线");
	m_ehSel = ehSel;
}

CBeamDefaultDlg::~CBeamDefaultDlg()
{
}

void CBeamDefaultDlg::SetDefaultData(BeamRebarInfo::BeamDefaultData& stDefaultInfo)
{
	m_stBeamDefaultData.nRebarCal = 0;
	m_stBeamDefaultData.dTop = 0.00;
	m_stBeamDefaultData.dLeft = 0.00;
	m_stBeamDefaultData.dRight = 0.00;
	m_stBeamDefaultData.dUnder = 0.00;
	m_stBeamDefaultData.dFloor = 0.00;
	m_stBeamDefaultData.dMargin = 0.00;
}

void CBeamDefaultDlg::SetBeamDefaultData(ElementId condit)
{
	SetElementXAttribute(condit, sizeof(BeamRebarInfo::BeamDefaultData), &m_stBeamDefaultData, stBeamDefaultXAttribute, ACTIVEMODEL);
}

BOOL CBeamDefaultDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetDefaultData(m_stBeamDefaultData);

	ElementId testid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	GetElementXAttribute(testid, sizeof(BeamRebarInfo::BeamDefaultData), m_stBeamDefaultData, stBeamDefaultXAttribute, ACTIVEMODEL);

	m_CombRebarCal.AddString(arrRebarCal[0]);
	m_CombRebarCal.AddString(arrRebarCal[1]);
	m_CombRebarCal.SetCurSel(m_stBeamDefaultData.nRebarCal);

	CString sTemp = CString();
	sTemp.Format(_T("%.2f"), m_stBeamDefaultData.dTop);
	m_EditTop.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamDefaultData.dLeft);
	m_EditLeft.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamDefaultData.dRight);
	m_EditRight.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamDefaultData.dUnder);
	m_EditUnder.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamDefaultData.dFloor);
	m_EditFloor.SetWindowText(sTemp);

	sTemp.Format(_T("%.2f"), m_stBeamDefaultData.dMargin);
	m_EditMargin.SetWindowText(sTemp);

	return TRUE;
}

void CBeamDefaultDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT1, m_EditTop);		   // 上方
	DDX_Control(pDX, IDC_EDIT2, m_EditLeft);	   // 左表面
	DDX_Control(pDX, IDC_EDIT5, m_EditRight);	   // 右表面
	DDX_Control(pDX, IDC_EDIT3, m_EditUnder);      // 下表面
	DDX_Control(pDX, IDC_EDIT4, m_EditMargin);	   // 边距
	DDX_Control(pDX, IDC_EDIT6, m_EditFloor);	   // 层	
	DDX_Control(pDX, IDC_COMBO1, m_CombRebarCal);  // 钢筋计算
}


BEGIN_MESSAGE_MAP(CBeamDefaultDlg, CDialogEx)
	ON_EN_CHANGE(IDC_EDIT6, &CBeamDefaultDlg::OnEnChangeEdit6)
	ON_EN_CHANGE(IDC_EDIT3, &CBeamDefaultDlg::OnEnChangeEdit3)
	ON_EN_CHANGE(IDC_EDIT1, &CBeamDefaultDlg::OnEnChangeEdit1)
	ON_EN_CHANGE(IDC_EDIT2, &CBeamDefaultDlg::OnEnChangeEdit2)
	ON_EN_CHANGE(IDC_EDIT5, &CBeamDefaultDlg::OnEnChangeEdit5)
	ON_EN_CHANGE(IDC_EDIT4, &CBeamDefaultDlg::OnEnChangeEdit4)
END_MESSAGE_MAP()


// CBeamDefaultDlg 消息处理程序


void CBeamDefaultDlg::OnEnChangeEdit6()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditFloor.GetWindowText(sTemp);
	m_stBeamDefaultData.dFloor = atof(CT2A(sTemp));
}


void CBeamDefaultDlg::OnEnChangeEdit3()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditUnder.GetWindowText(sTemp);
	m_stBeamDefaultData.dUnder = atof(CT2A(sTemp));
}


void CBeamDefaultDlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditTop.GetWindowText(sTemp);
	m_stBeamDefaultData.dTop = atof(CT2A(sTemp));
}


void CBeamDefaultDlg::OnEnChangeEdit2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditLeft.GetWindowText(sTemp);
	m_stBeamDefaultData.dLeft = atof(CT2A(sTemp));
}


void CBeamDefaultDlg::OnEnChangeEdit5()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditRight.GetWindowText(sTemp);
	m_stBeamDefaultData.dRight = atof(CT2A(sTemp));
}


void CBeamDefaultDlg::OnEnChangeEdit4()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString sTemp = CString();
	m_EditMargin.GetWindowText(sTemp);
	m_stBeamDefaultData.dMargin = atof(CT2A(sTemp));
}
