// CircleAndSquare.cpp: 实现文件
//
#include "_USTATION.h"
#include "resource.h"
#include "GalleryIntelligentRebar.h"
#include "CircleAndSquare.h"
#include "afxdialogex.h"
//
#include "CommonFile.h"
#include "ConstantsDef.h"


// CircleAndSquare 对话框

IMPLEMENT_DYNAMIC(CircleAndSquare, CDialogEx)

CircleAndSquare::CircleAndSquare(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CircleAndSquare, pParent)
{

}

CircleAndSquare::~CircleAndSquare()
{
}

void CircleAndSquare::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_HDiameter, m_HDiameter);
	DDX_Control(pDX, IDC_COMBO_HGrade, m_HGrade);
	DDX_Control(pDX, IDC_COMBO_CDiameter, m_CDiameter);
	DDX_Control(pDX, IDC_COMBO_CGrade, m_CGrade);
	DDX_Control(pDX, IDC_EDIT_Side, m_Edit_Side);
	DDX_Control(pDX, IDC_EDIT_Front, m_Edit_Positive);
	DDX_Control(pDX, IDC_EDIT_HSpacing, m_Edit_HSpacing);
	DDX_Control(pDX, IDC_EDIT_PSpacing, m_Edit_PSpacing);
	DDX_Control(pDX, IDC_EDIT_CSpacing, m_Edit_CSpacing);
	DDX_Control(pDX, IDC_EDIT_PAngle, m_Edit_CAngle);
}

BOOL CircleAndSquare::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端

	//添加钢筋直径
	for each (auto var in g_listRebarSize)
	{
		m_HDiameter.AddString(var);
		m_CDiameter.AddString(var);
	}
	//添加钢筋等级
	for each (auto var in g_listRebarType)
	{
		m_HGrade.AddString(var);
		m_CGrade.AddString(var);
	}

	return TRUE;
	
}


BEGIN_MESSAGE_MAP(CircleAndSquare, CDialogEx)
	ON_BN_CLICKED(IDOK, &CircleAndSquare::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CircleAndSquare::OnBnClickedCancel)
END_MESSAGE_MAP()


// CircleAndSquare 消息处理程序


void CircleAndSquare::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}


void CircleAndSquare::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}
