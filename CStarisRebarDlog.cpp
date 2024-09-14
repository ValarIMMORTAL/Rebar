// CStarisRebarDlog.cpp: 实现文件
//
#include "_USTATION.h"
#include "resource.h"
#include "GalleryIntelligentRebar.h"
#include "CStarisRebarDlog.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"
#include "ElementAttribute.h"
#include "StairsRebarAssembly.h"

// CStarisRebarDlog 对话框

IMPLEMENT_DYNAMIC(CStarisRebarDlog, CDialogEx)

CStarisRebarDlog::CStarisRebarDlog(ElementHandleCR eh, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_Stairs, pParent)
{
	ehSel = eh;
	m_StairsRebarInfo.StairsStyle = 0;
	m_StairsRebarInfo.rebarType = 0;
	m_StairsRebarInfo.StairsCover = 30.00;
	strcpy(m_StairsRebarInfo.rebarSize, CT2A(_T("16mm")));
	m_pStairsRebar = NULL;

}

CStarisRebarDlog::~CStarisRebarDlog()
{
}

void CStarisRebarDlog::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

void CStarisRebarDlog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO12, m_ComboStyle);
	DDX_Control(pDX, IDC_COMBO2, m_ComboType);
	DDX_Control(pDX, IDC_COMBO13, m_ComboSize);
	DDX_Control(pDX, IDC_EDIT15, m_EditCover);

}


BOOL CStarisRebarDlog::OnInitDialog()
{

	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	for each (auto var in g_listStairsRebarStyle)
		m_ComboStyle.AddString(var);

	for each (auto var in g_listRebarType)
		m_ComboType.AddString(var);

	for each (auto var in g_listRebarSize)
		m_ComboSize.AddString(var);


	ElementId contid = 0;
	GetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, ehSel.GetModelRef());
	if (contid > 0)
	{
		GetElementXAttribute(contid, sizeof(StairRebarInfo), m_StairsRebarInfo, StairsAttribute, ACTIVEMODEL);
		//		GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	}

	CString strRebarSize(m_StairsRebarInfo.rebarSize);
	if (strRebarSize.Find(L"mm") == -1)
		strRebarSize += "mm";
	int nIndex = m_ComboSize.FindStringExact(0, strRebarSize);
	CString strCover;
	strCover.Format(L"%.2f", m_StairsRebarInfo.StairsCover);//保护层
	//GetDlgItem(IDC_EDIT15)->SetWindowText(strCover);


	m_ComboSize.SetCurSel(nIndex);//尺寸
	m_ComboType.SetCurSel(m_StairsRebarInfo.rebarType);//型号
	m_ComboStyle.SetCurSel(m_StairsRebarInfo.StairsStyle);	//设置楼梯样式
	m_EditCover.SetWindowText(strCover);
	return true;
}

BEGIN_MESSAGE_MAP(CStarisRebarDlog, CDialogEx)
	ON_EN_CHANGE(IDC_EDIT15, &CStarisRebarDlog::OnEnChangeEdit15)
	ON_CBN_SELCHANGE(IDC_COMBO12, &CStarisRebarDlog::OnCbnSelchangeCombo12)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CStarisRebarDlog::OnCbnSelchangeCombo2)
	ON_CBN_SELCHANGE(IDC_COMBO13, &CStarisRebarDlog::OnCbnSelchangeCombo13)
	ON_BN_CLICKED(IDOK, &CStarisRebarDlog::OnBnClickedOk)
END_MESSAGE_MAP()


void CStarisRebarDlog::OnEnChangeEdit15()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString	strTemp = CString();

	m_EditCover.GetWindowText(strTemp);
	m_StairsRebarInfo.StairsCover = atof(CT2A(strTemp));
}


void CStarisRebarDlog::OnCbnSelchangeCombo12()
{
	// TODO: 在此添加控件通知处理程序代码
	m_StairsRebarInfo.StairsStyle = m_ComboStyle.GetCurSel();
}


void CStarisRebarDlog::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
	m_StairsRebarInfo.rebarType = m_ComboType.GetCurSel();
}


void CStarisRebarDlog::OnCbnSelchangeCombo13()
{
	// TODO: 在此添加控件通知处理程序代码
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboSize.GetCurSel());
	strcpy(m_StairsRebarInfo.rebarSize, CT2A(*it));
}


void CStarisRebarDlog::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
	DestroyWindow();
}

void CStarisRebarDlog::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	DgnModelRefP        modelRef = ACTIVEMODEL;

	if (m_pStairsRebar == NULL)
	{
		m_pStairsRebar = REA::Create<CStairsRebarAssembly>(ACTIVEMODEL, ehSel.GetElementId(), ehSel.GetModelRef());
	}
	m_pStairsRebar->GetStairsFeatureParam(ehSel);
	m_pStairsRebar->GetRebarInfo(m_StairsRebarInfo);
	m_pStairsRebar->MakeRebars(modelRef);
	m_pStairsRebar->Save(modelRef);
	ElementId contid = m_pStairsRebar->FetchConcrete();

	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);

	SetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, ehSel.GetModelRef());
	SetElementXAttribute(contid, sizeof(StairRebarInfo), &m_StairsRebarInfo, StairsAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecRebarData, StairsAttribute, ACTIVEMODEL);
	DestroyWindow();
}
