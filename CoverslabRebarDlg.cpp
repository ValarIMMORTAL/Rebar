// CoverslabRebarDlg.cpp: 实现文件
//
#include "_USTATION.h"

#include "GalleryIntelligentRebar.h"
#include "CoverslabRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "afxdialogex.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "CommonFile.h"
#include "ElementAttribute.h"
#include "ACCRebarMaker.h"
// CoverslabRebarDlg 对话框

IMPLEMENT_DYNAMIC(CoverslabRebarDlg, CDialogEx)

CoverslabRebarDlg::CoverslabRebarDlg(ElementHandleCR eh, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CoverslabRebar, pParent), m_PageMainRebar(eh, pParent), m_PageEndType(eh, pParent)
{
	ehSel = eh;
	m_vecRebarData.clear();
	m_vecRebarData.shrink_to_fit();

	//m_wallRebarInfo.concrete.rebarLevelNum = 3;
//	m_vecLapOptionData.clear();
//	m_vecLapOptionData.shrink_to_fit();
	m_vecEndTypeData.clear();
	m_vecEndTypeData.shrink_to_fit();
//	m_vecACData.clear();
//	m_vecACData.shrink_to_fit();
	m_vecTwinBarData.clear();
	m_vecTwinBarData.shrink_to_fit();
	memset(&m_twinBarInfo, 0, sizeof(TwinBarSet::TwinBarInfo));

	m_tieRebarInfo.tieRebarMethod = 0;
	m_isDoubleClick = false;
}

CoverslabRebarDlg::~CoverslabRebarDlg()
{
}

void CoverslabRebarDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

void CoverslabRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_CoverslabRebar, m_tab);
}


BEGIN_MESSAGE_MAP(CoverslabRebarDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CoverslabRebarDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CoverslabRebarDlg::OnBnClickedCancel)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_CoverslabRebar, &CoverslabRebarDlg::OnTcnSelchangeTabCoverslabrebar)
END_MESSAGE_MAP()


// CoverslabRebarDlg 消息处理程序

BOOL CoverslabRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	ElementId contid = 0;
	GetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, ehSel.GetModelRef());
	if (contid > 0 && m_isDoubleClick)
	{
		GetElementXAttribute(contid, sizeof(WallRebarInfo), m_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		//	GetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
		GetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
		//	GetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
			//GetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
		GetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);

		ACCConcrete ACconcrete;
		GetElementXAttribute(ehSel.GetElementId(), sizeof(ACCConcrete), ACconcrete, ConcreteCoverXAttribute, ehSel.GetModelRef());
		m_wallRebarInfo.concrete.postiveCover = ACconcrete.postiveOrTopCover;
		m_wallRebarInfo.concrete.reverseCover = ACconcrete.reverseOrBottomCover;
		m_wallRebarInfo.concrete.sideCover = ACconcrete.sideCover;
		m_wallRebarInfo.concrete.rebarLevelNum = (int)m_vecRebarData.size();
		m_PageMainRebar.SetConcreteData(m_wallRebarInfo.concrete);
	}

	m_PageMainRebar.SetListRowData(m_vecRebarData);
	m_PageEndType.SetListRowData(m_vecEndTypeData);
	// m_PageTwinBars.SetListRowData(m_vecTwinBarData);
	// m_PageTwinBars.m_vecRebarData = m_vecRebarData;


	// TODO:  在此添加额外的初始化
	//为Tab Control增加两个页面
	m_tab.InsertItem(0, _T("主要配筋"));
	m_tab.InsertItem(1, _T("端部样式"));
	// m_tab.InsertItem(2, _T("并筋设置"));
	//	m_tab.InsertItem(3, _T("关联构件"));
	//创建两个对话框
	m_PageMainRebar.Create(IDD_DIALOG_CoverslabRebar_MainRebar, &m_tab);        //主要配筋
	m_PageEndType.Create(IDD_DIALOG_CoverslabRebar_EndType, &m_tab);                //端部
	// m_PageTwinBars.Create(IDD_DIALOG_CoverslabRebar_TwinBarSet, &m_tab);            //并筋
	//	m_PageLapOption.Create(IDD_DIALOG_WallRebar_LapOption, &m_tab);
	//	m_PageAssociatedComponent.Create(IDD_DIALOG_WallRebar_AssociatedComponent, &m_tab);

		//设定在Tab内显示的范围
	CRect rc;
	m_tab.GetClientRect(rc);
	rc.top += 20;
	rc.bottom -= 0;
	rc.left += 0;
	rc.right -= 0;
	m_PageMainRebar.MoveWindow(&rc);
	m_PageEndType.MoveWindow(&rc);
	// m_PageTwinBars.MoveWindow(&rc);

	//把对话框对象指针保存起来
	pDialog[0] = &m_PageMainRebar;
	pDialog[1] = &m_PageEndType;
	// pDialog[2] = &m_PageTwinBars;

	//显示初始页面
	pDialog[0]->ShowWindow(SW_SHOW);
	pDialog[1]->ShowWindow(SW_HIDE);
	// pDialog[2]->ShowWindow(SW_HIDE);

	//保存当前选择
	m_CurSelTab = 0;

	g_ConcreteId = m_ConcreteId;
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CoverslabRebarDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();

	m_PageMainRebar.GetConcreteData(m_wallRebarInfo.concrete);
	vector<PIT::ConcreteRebar> vecRebarData;
	m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
	m_PageEndType.SetListRowData(m_vecEndTypeData);
	// m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
	// m_PageTwinBars.SetListRowData(m_vecTwinBarData);

	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = ehSel.GetModelRef();
	EditElementHandle eeh(ehSel, ehSel.GetModelRef());

	tmpModel = tmpModel;
	CoverslabRebarAssembly*  CoverslabRebar = NULL;

	ElementId testid = 0;
	GetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, ehSel.GetModelRef());

	CoverslabRebarAssembly::IsSmartSmartFeature(eeh);
	CoverslabRebarAssembly::CoverSlabType CoverslabType = CoverslabRebarAssembly::JudgeSlabType(ehSel);
	switch (CoverslabType)
	{
	case CoverslabRebarAssembly::SICoverSlab:
	{
		RebarAssembly* rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class SICoverslabRebarAssembly");
		CoverslabRebar = dynamic_cast<SICoverslabRebarAssembly*>(rebaras);
		if (CoverslabRebar == nullptr)
		{
			CoverslabRebar = REA::Create<SICoverslabRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		}
	}
	break;
	case CoverslabRebarAssembly::STCoverSlab:
	case CoverslabRebarAssembly::STCoverSlab_Ten:
	{
		RebarAssembly* rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class STCoverslabRebarAssembly");
		CoverslabRebar = dynamic_cast<STCoverslabRebarAssembly*>(rebaras);
		if (CoverslabRebar == nullptr)
		{
			CoverslabRebar = REA::Create<STCoverslabRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());

		}
	}
	break;
	case CoverslabRebarAssembly::SZCoverSlab:	
	{
		RebarAssembly* rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class SZCoverslabRebarAssembly");
		CoverslabRebar = dynamic_cast<SZCoverslabRebarAssembly*>(rebaras);
		if (CoverslabRebar == nullptr)
		{
			CoverslabRebar = REA::Create<SZCoverslabRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		}
	}
		break;
	case CoverslabRebarAssembly::OtherCoverSlab:
		return;
	default:
		break;
	}

	CoverslabRebar->SetCoverslabType(CoverslabType);            //设置墙类型
	CoverslabRebar->SetCoverSlabData(eeh);                    //设置墙坐标
	CoverslabRebar->SetConcreteData(m_wallRebarInfo.concrete);  //设置三个保护层信息和层数//SetConcreteData函数用引用方式把m_wallRebarInfo.concrete值传到wallRebar
	CoverslabRebar->SetRebarData(m_vecRebarData);               //设置钢筋信息
//	slabRebar->SetvecLapOptions(g_vecLapOptionData);
	CoverslabRebar->SetRebarEndTypes(m_vecEndTypeData);
	CoverslabRebar->SetvecTwinRebarLevel(m_vecTwinBarData);
	CoverslabRebar->InitRebarSetId();
	CoverslabRebar->SetTieRebarInfo(m_tieRebarInfo);

	CoverslabRebar->MakeRebars(modelRef);
	CoverslabRebar->Save(modelRef); // must save after creating rebars

	ElementId contid = CoverslabRebar->FetchConcrete();
	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);
	//eeh2.AddToModel();

	ACCConcrete ACconcrete;
	ACconcrete.postiveOrTopCover = m_wallRebarInfo.concrete.postiveCover;
	ACconcrete.reverseOrBottomCover = m_wallRebarInfo.concrete.reverseCover;
	ACconcrete.sideCover = m_wallRebarInfo.concrete.sideCover;
	SetElementXAttribute(ehSel.GetElementId(), sizeof(ACCConcrete), &ACconcrete, ConcreteCoverXAttribute, ehSel.GetModelRef());

	SetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, ehSel.GetModelRef());
	SetElementXAttribute(contid, sizeof(WallRebarInfo), &m_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
//	SetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
//	SetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), &m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	DestroyWindow();
}


void CoverslabRebarDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	DestroyWindow();
}


void CoverslabRebarDlg::OnTcnSelchangeTabCoverslabrebar(NMHDR *pNMHDR, LRESULT *pResult)
{	// TODO: 在此添加控件通知处理程序代码
	//把当前的页面隐藏起来
	pDialog[m_CurSelTab]->ShowWindow(SW_HIDE);
	//得到新的页面索引
	m_CurSelTab = m_tab.GetCurSel();

	switch (m_CurSelTab)
	{
	case 0:
	{
		m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
		m_PageEndType.SetListRowData(m_vecEndTypeData);
		// m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
		// m_PageTwinBars.SetListRowData(m_vecTwinBarData);
		m_PageMainRebar.UpdateRebarList();
	}
	break;
	case 1:
	{
		m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
		m_PageMainRebar.SetListRowData(m_vecRebarData);
		// m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
		// m_PageTwinBars.SetListRowData(m_vecTwinBarData);
		// m_PageTwinBars.m_vecRebarData = m_vecRebarData;
		m_PageEndType.UpdateEndTypeList();

		//		m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);

		// 		CWallRebarEndTypeDlg* dlg = (CWallRebarEndTypeDlg*)pDialog[m_CurSelTab];
		// 		dlg->UpdateEndTypeList();
		//		m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
		// 		m_PageEndType.SetListRowData(m_vecEndTypeData);
		// 		m_PageEndType.UpdateEndTypeList();
	}
	break;
	case 2:
	{
		m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
		m_PageMainRebar.SetListRowData(m_vecRebarData);
		m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
		m_PageEndType.SetListRowData(m_vecEndTypeData);
		// m_PageTwinBars.m_vecRebarData = m_vecRebarData;
		// m_PageTwinBars.m_vecRebarData = m_vecRebarData;
		// m_PageTwinBars.UpdateTwinBarsList();

		//		CTwinBarSetDlg* dlg = (CTwinBarSetDlg*)pDialog[m_CurSelTab];
		// 		m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
		// 		m_PageTwinBars.SetListRowData(m_vecTwinBarData);
	}
	break;
	case 3:
	{
		m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
		m_PageMainRebar.SetListRowData(m_vecRebarData);
		m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
		m_PageEndType.SetListRowData(m_vecEndTypeData);
		// m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
		// m_PageTwinBars.SetListRowData(m_vecTwinBarData);
		// m_PageTwinBars.m_vecRebarData = m_vecRebarData;
		m_PageMainRebar.UpdateRebarList();
	}
	break;
	default:
		break;
	}
	//把新的页面显示出来
	pDialog[m_CurSelTab]->ShowWindow(SW_SHOW);
	*pResult = 0;
}
