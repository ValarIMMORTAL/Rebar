// CBeamRebarMainDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CBeamRebarMainDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "ElementAttribute.h"
#include "CBeamRebarAssembly.h"


// CBeamRebarMainDlg 对话框

IMPLEMENT_DYNAMIC(CBeamRebarMainDlg, CDialogEx)

CBeamRebarMainDlg::CBeamRebarMainDlg(ElementHandle ehSel, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_BeamRebarMainDlg, pParent),m_PageHoopRebarDlg(ehSel, pParent),m_PageVerticalRebarDlg(ehSel, pParent),
	  m_PageDefaultDlg(ehSel, pParent),m_PageBaseDlg(ehSel, pParent)
{
	m_ehSel = ehSel;
	m_pBeamRebarAssembly = NULL;
}

CBeamRebarMainDlg::~CBeamRebarMainDlg()
{
}

void CBeamRebarMainDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

BOOL CBeamRebarMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	m_tab.InsertItem(0, _T("梁信息"));
	m_tab.InsertItem(1, _T("梁默认值"));
	m_tab.InsertItem(2, _T("纵筋"));
	m_tab.InsertItem(3, _T("箍筋"));

	// 添加两个对话框
	m_PageBaseDlg.Create(IDD_DIALOG_BeamInfo, &m_tab);
	m_PageDefaultDlg.Create(IDD_DIALOG_BeamDefault, &m_tab);
	m_PageVerticalRebarDlg.Create(IDD_DIALOG_BeamVerticalRebar, &m_tab);
	m_PageHoopRebarDlg.Create(IDD_DIALOG_BeamHoopRebar, &m_tab);

	// 设定在Tab内显示的范围
	CRect rc;
	m_tab.GetClientRect(rc);
	rc.top += 20;
	rc.bottom -= 0;
	rc.left += 0; 
	rc.right -= 0;

	m_PageBaseDlg.MoveWindow(&rc);
	m_PageDefaultDlg.MoveWindow(&rc);
	m_PageVerticalRebarDlg.MoveWindow(&rc);
	m_PageHoopRebarDlg.MoveWindow(&rc);

	p_Dialog[0] = &m_PageBaseDlg;
	p_Dialog[1] = &m_PageDefaultDlg;
	p_Dialog[2] = &m_PageVerticalRebarDlg;
	p_Dialog[3] = &m_PageHoopRebarDlg;

	// 显示初始页面
	p_Dialog[0]->ShowWindow(SW_SHOW);
	p_Dialog[1]->ShowWindow(SW_HIDE);
	p_Dialog[2]->ShowWindow(SW_HIDE);
	p_Dialog[3]->ShowWindow(SW_HIDE);
	m_CurSelTab = 0;

	return TRUE;
}

void CBeamRebarMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TAB1, m_tab); // 标签
}


BEGIN_MESSAGE_MAP(CBeamRebarMainDlg, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CBeamRebarMainDlg::OnTcnSelchangeTab1)
	ON_BN_CLICKED(IDOK, &CBeamRebarMainDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CBeamRebarMainDlg 消息处理程序


void CBeamRebarMainDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码

	//把当前的页面隐藏起来
	p_Dialog[m_CurSelTab]->ShowWindow(SW_HIDE);
	//得到新的页面索引
	m_CurSelTab = m_tab.GetCurSel();
	//把新的页面显示出来
	p_Dialog[m_CurSelTab]->ShowWindow(SW_SHOW);

	*pResult = 0;
}

void CBeamRebarMainDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
	DestroyWindow();
}
void CBeamRebarMainDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = m_ehSel.GetModelRef();
	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());

	m_PageVerticalRebarDlg.m_ListBeamAreaInfo.GetAllRebarData(m_PageVerticalRebarDlg.m_vecBeamAreaData);

	CBeamRebarAssembly::IsSmartSmartFeature(eeh);
	if (m_pBeamRebarAssembly == NULL)
	{
		m_pBeamRebarAssembly = REA::Create<CBeamRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	}

	m_pBeamRebarAssembly->CalcBeamBaseInfo(eeh);
	m_PageBaseDlg.m_stBeamBaseData.dDepth = m_pBeamRebarAssembly->GetBeamInfo().height;
	m_PageBaseDlg.m_stBeamBaseData.dWidth = m_pBeamRebarAssembly->GetBeamInfo().width;
	m_PageBaseDlg.m_stBeamBaseData.dAxisToAxis = m_pBeamRebarAssembly->GetBeamInfo().length;
	m_pBeamRebarAssembly->SetstDefaultInfo(m_PageDefaultDlg.m_stBeamDefaultData);
	m_pBeamRebarAssembly->SetstBeamBaseData(m_PageBaseDlg.m_stBeamBaseData);
	m_pBeamRebarAssembly->SetvecBeamAreaVertical(m_PageVerticalRebarDlg.m_vecBeamAreaData);
	m_pBeamRebarAssembly->SetvecBeamCommHoop(m_PageHoopRebarDlg.m_vecBeamCommHoop);
	m_pBeamRebarAssembly->SetvecBeamRebarHoop(m_PageHoopRebarDlg.m_vecBeamRebarHoop);
	m_pBeamRebarAssembly->SetvecBeamRebarVertical(m_PageVerticalRebarDlg.m_vecBeamRebarData);
	m_pBeamRebarAssembly->MakeRebars(modelRef);
	m_pBeamRebarAssembly->Save(modelRef);
	ElementId contid = m_pBeamRebarAssembly->FetchConcrete();
	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);
	//eeh2.AddToModel();

	SetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	m_PageBaseDlg.SetBeamBaseData(contid);
	m_PageDefaultDlg.SetBeamDefaultData(contid);
	m_PageVerticalRebarDlg.SetBeamVerticalData(contid);
	m_PageHoopRebarDlg.SetBeamHoopData(contid);
	DestroyWindow();
}