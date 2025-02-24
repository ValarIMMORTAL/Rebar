// CWallRebar.cpp: 实现文件
//
#include "_USTATION.h"
#include "CWallRebarDlgNew.h"
#include "afxdialogex.h"
#include "../../resource.h"
#include "WallRebarAssemblyNew.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "../../CommonFile.h"
#include "ElementAttribute.h"
#include "ACCRebarMakerNew.h"
#include "../../ConstantsDef.h"

// CWallRebar 对话框

bool PreviewButtonDownNew = false;//主要配筋界面的预览按钮

IMPLEMENT_DYNAMIC(CWallRebarDlgNew, CDialogEx)

CWallRebarDlgNew::CWallRebarDlgNew(ElementHandleCR eh,CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CNWallRebar,pParent),ehSel(eh),m_ConcreteId(0)
{
	m_WallACCRebarLinesPtr = NULL;
	m_WallRebarLinesPtr = NULL;
	m_vecRebarData.clear();
	m_vecRebarData.shrink_to_fit();
	m_vecLapOptionData.clear();
	m_vecLapOptionData.shrink_to_fit();
	m_vecEndTypeData.clear();
	m_vecEndTypeData.shrink_to_fit();
	m_vecACData.clear();
	m_vecACData.shrink_to_fit();
	m_vecTwinBarData.clear();
	m_vecTwinBarData.shrink_to_fit();
	memset(&m_twinBarInfo, 0, sizeof(TwinBarSet::TwinBarInfo));
	memset(&m_tieRebarInfo, 0, sizeof(TieReBarInfo));
	m_tieRebarInfo.rebarType = 2;

	m_stCutRebarInfo.dCutLength1 = 0.0;
	m_stCutRebarInfo.dCutLength2 = 0.0;
	m_stCutRebarInfo.dCutLength3 = 0.0;
	m_stCutRebarInfo.isCutRebar = false;
}

// CWallRebarDlg::CWallRebarDlg(UINT Id, CWnd* pParent /*=nullptr*/)
// 	: CBModelessDialog(Id, pParent), m_ConcreteId(0)
// {
// 
// }
CWallRebarDlgNew::~CWallRebarDlgNew()
{
	if (m_WallRebarLinesPtr != nullptr)
	{
		delete m_WallRebarLinesPtr;
		m_WallRebarLinesPtr = NULL;
	}

// 	if (m_WallACCRebarLinesPtr != nullptr)
// 	{
// 		delete m_WallACCRebarLinesPtr;
// 		m_WallACCRebarLinesPtr = NULL;
// 	}

}

void CWallRebarDlgNew::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

void CWallRebarDlgNew::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_WallRebar, m_tab);
	DDX_Control(pDX, IDC_COMBO2, m_ComboSize);
	DDX_Control(pDX, IDC_COMBO3, m_ComboType);
	DDX_Control(pDX, IDC_EDIT1, m_EditSpace);
	DDX_Control(pDX, IDC_STATIC_WALLNAME, m_static_wallname);
}

// StatusInt CWallRebarDlg::Create()
// {
// 	CDialog::Create(IDC_TAB_WallRebar, NULL);
// 	return 0;
// }

// void CWallRebarDlg::SetACInfo(const vector<PIT::AssociatedComponent>& vecACData)
// {
// 	m_PageAssociatedComponent.SetListRowData(vecACData);
// }


BEGIN_MESSAGE_MAP(CWallRebarDlgNew, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_WallRebar, &CWallRebarDlgNew::OnTcnSelchangeTabWallrebar)
	ON_BN_CLICKED(IDOK, &CWallRebarDlgNew::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CWallRebarDlgNew::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CWallRebarDlgNew::OnCbnSelchangeCombo2)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CWallRebarDlgNew::OnCbnSelchangeCombo3)
	ON_EN_CHANGE(IDC_EDIT1, &CWallRebarDlgNew::OnEnChangeEdit1)
	ON_STN_CLICKED(IDC_STATIC_WALLNAME, &CWallRebarDlgNew::OnStnClickedStaticWallname)
END_MESSAGE_MAP()


// CWallRebar 消息处理程序


BOOL CWallRebarDlgNew::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	string elename, eletype;
	GetEleNameAndType(ehSel.GetElementId(), ehSel.GetModelRef(), elename, eletype);
	wall_name = elename;
	CString wallname(elename.c_str());
	wallname = L"墙名:" + wallname;
	m_static_wallname.SetWindowTextW(wallname);
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	for each (auto var in g_listRebarSize)
		m_ComboSize.AddString(var);
	for each (auto var in g_listRebarType)
		m_ComboType.AddString(var);

	ElementId contid = 0;
	GetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, ehSel.GetModelRef());

	GetElementXAttribute(contid, sizeof(WallRebarInfo), g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, sizeof(TieReBarInfo), m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);
	if (contid > 0)
	{
		GetElementXAttribute(contid, sizeof(m_stCutRebarInfo), m_stCutRebarInfo, stCNWallCutRebarInfo, ACTIVEMODEL);
	}

	if (m_vecRebarData.size() == 0)
	{
		m_WallSetInfo.rebarType = 2;
	}

	CString strRebarSize(m_WallSetInfo.rebarSize);
	if (strRebarSize.Find(L"mm") == -1)
		strRebarSize += "mm";
	int nIndex = m_ComboSize.FindStringExact(0, strRebarSize);
	m_ComboSize.SetCurSel(nIndex);//尺寸
	m_ComboType.SetCurSel(m_WallSetInfo.rebarType);//型号
	CString strSpace;
	strSpace.Format(L"%.2f", m_WallSetInfo.spacing);//保护层
//	m_EditSpace.SetWindowText(strSpace);

// 	m_PageMainRebar.getTieRebarData(m_tieRebarInfo);//将拉筋及并筋信息传入m_PageMainRebar
// 	m_PageMainRebar.getTwinRowData(m_vecTwinBarData);


	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	m_PageEndType.SetListRowData(m_vecEndTypeData);
	m_PageEndType.m_vecRebarData = m_vecRebarData;
	m_PageTwinBars.SetListRowData(m_vecTwinBarData);
	m_PageTwinBars.m_vecRebarData = m_vecRebarData;
	m_PageTieRebar.SetTieRebarData(m_tieRebarInfo);

	m_PageMainRebar.SavePtr(this);
	m_PageMainRebar.SetCutRebarInfo(m_stCutRebarInfo);

	// TODO:  在此添加额外的初始化
	//为Tab Control增加两个页面
	m_tab.InsertItem(0, _T("主要配筋"));
	m_tab.InsertItem(1, _T("端部样式"));
	m_tab.InsertItem(2, _T("并筋设置"));
	m_tab.InsertItem(3, _T("拉筋布置"));
	//	m_tab.InsertItem(3, _T("关联构件"));
	//创建两个对话框
	m_PageMainRebar.Create(IDD_DIALOG_CNWallRebar_MainRebar, &m_tab);
	m_PageMainRebar.SetSelectElement(ehSel);
	m_PageEndType.Create(IDD_DIALOG_CNWallRebar_EndType, &m_tab);
	m_PageTwinBars.Create(IDD_DIALOG_CNWallRebar_TwinBarSet, &m_tab);
	m_PageTieRebar.Create(IDD_DIALOG_CNWallRebar_TieRebarSet, &m_tab);
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
	m_PageTwinBars.MoveWindow(&rc);
	m_PageTieRebar.MoveWindow(&rc);

	//把对话框对象指针保存起来
	pDialog[0] = &m_PageMainRebar;
	pDialog[1] = &m_PageEndType;
	pDialog[2] = &m_PageTwinBars;
	pDialog[3] = &m_PageTieRebar;

	//显示初始页面
	pDialog[0]->ShowWindow(SW_SHOW);
	pDialog[1]->ShowWindow(SW_HIDE);
	pDialog[2]->ShowWindow(SW_HIDE);
	pDialog[3]->ShowWindow(SW_HIDE);

	//保存当前选择
	m_CurSelTab = 0;

	g_ConcreteId = m_ConcreteId;

	ACCConcrete wallACConcrete;
	int ret = GetElementXAttribute(ehSel.GetElementId(), sizeof(ACCConcrete), wallACConcrete, ConcreteCoverXAttribute, ehSel.GetModelRef());
	if (ret == SUCCESS)	//关联构件配筋时存储过数据,优先使用关联构件设置的保护层
	{
		g_wallRebarInfo.concrete.postiveCover = wallACConcrete.postiveOrTopCover;
		g_wallRebarInfo.concrete.reverseCover = wallACConcrete.reverseOrBottomCover;
		g_wallRebarInfo.concrete.sideCover = wallACConcrete.sideCover;
	}
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CWallRebarDlgNew::OnTcnSelchangeTabWallrebar(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码
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
		m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
		m_PageTwinBars.SetListRowData(m_vecTwinBarData);
		m_PageMainRebar.UpdateRebarList();
	}
		break;
	case 1:
	{
		m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
		m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
		m_PageMainRebar.SetListRowData(m_vecRebarData);
		m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
		m_PageTwinBars.SetListRowData(m_vecTwinBarData);
		m_PageEndType.m_vecRebarData = m_vecRebarData;
		m_PageEndType.UpdateEndTypeList();

//		m_PageEndType.m_ListEndType.GetAllRebarData(g_vecEndTypeData);

// 		CWallRebarEndTypeDlg* dlg = (CWallRebarEndTypeDlg*)pDialog[m_CurSelTab];
// 		dlg->UpdateEndTypeList();
//		m_PageEndType.m_ListEndType.GetAllRebarData(g_vecEndTypeData);
// 		m_PageEndType.SetListRowData(g_vecEndTypeData);
// 		m_PageEndType.UpdateEndTypeList();
	}
		break;
	case 2:
	{
		m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
		m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
		m_PageMainRebar.SetListRowData(m_vecRebarData);
		m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
		m_PageEndType.SetListRowData(m_vecEndTypeData);
		m_PageTwinBars.m_vecRebarData = m_vecRebarData;
		m_PageTwinBars.UpdateTwinBarsList();

//		CTwinBarSetDlg* dlg = (CTwinBarSetDlg*)pDialog[m_CurSelTab];
// 		m_PageTwinBars.m_ListTwinBars.GetAllRebarData(g_vecTwinBarData);
// 		m_PageTwinBars.SetListRowData(g_vecTwinBarData);
	}
		break;
	case 3:
	{
		m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
		m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
		m_PageMainRebar.SetListRowData(m_vecRebarData);
		m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
		m_PageEndType.SetListRowData(m_vecEndTypeData);
		m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
		m_PageTwinBars.SetListRowData(m_vecTwinBarData);
		m_PageMainRebar.UpdateRebarList();
		m_PageTieRebar.m_vecRebarData = m_vecRebarData;
		m_PageTieRebar.OnInitDialog();
	}
		break;
	default:
		break;
	}

	

	//把新的页面显示出来
	pDialog[m_CurSelTab]->ShowWindow(SW_SHOW);
	*pResult = 0;
}
void CWallRebarDlgNew::RefreshWallRebars(ElementId conid,EditElementHandleR eeh)
{

	std::vector<PIT::ConcreteRebar>					m_vecRebarData;
	std::vector<PIT::LapOptions>					m_vecLapOptionData;
	std::vector<PIT::EndType>						m_vecEndTypeData;
	std::vector<PIT::AssociatedComponent>			m_vecACData;
	std::vector<TwinBarSet::TwinBarLevelInfo>	m_vecTwinBarData;
	TwinBarSet::TwinBarInfo						m_twinBarInfo;
	TieReBarInfo								m_tieRebarInfo;
	GetElementXAttribute(conid, sizeof(WallRebarInfo), g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, sizeof(TwinBarSet::TwinBarInfo), m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, sizeof(TieReBarInfo), m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);
	bool bACRebar = false;
	for (size_t i = 0; i < m_vecACData.size(); ++i)
	{
		if (m_vecACData[i].associatedRelation == 1 || m_vecACData[i].associatedRelation == 2)	//存在一个为锚入即进行关联配筋
		{
			bACRebar = true;
			break;
		}
	}
	if (!bACRebar)
	{
		WallRebarAssemblyNew::IsSmartSmartFeature(eeh);
		WallRebarAssemblyNew*  wallRebar = NULL;
		ElementId testid = 0;
		GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());

		WallRebarAssemblyNew::WallType wallType = WallRebarAssemblyNew::JudgeWallType(eeh);
		switch (wallType)
		{
		case WallRebarAssemblyNew::STWALL:
		{
			PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class PIT::ACCSTWallRebarAssemblyNew");
			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class STWallRebarAssemblyNew");
			wallRebar = dynamic_cast<STWallRebarAssemblyNew*>(rebaras);
			if (wallRebar == nullptr)
			{
				wallRebar = REA::Create<STWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
		}
		break;
		case WallRebarAssemblyNew::GWALL:
			wallRebar = REA::Create<GWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			break;
		case WallRebarAssemblyNew::ARCWALL:	//弧形墙
		{
			// PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class PIT::ACCArcWallRebarAssembly");
			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class ArcWallRebarAssemblyNew");
			wallRebar = dynamic_cast<ArcWallRebarAssemblyNew*>(rebaras);
			if (wallRebar == nullptr)
			{
				wallRebar = REA::Create<ArcWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
			break;
		}
		case WallRebarAssemblyNew::ELLIPSEWall:	// 椭圆形墙
		{
			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class ELLWallRebarAssemblyNew");
			wallRebar = dynamic_cast<ELLWallRebarAssemblyNew*>(rebaras);
			if (wallRebar == nullptr)
			{
				wallRebar = REA::Create<ELLWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
		}
		break;
		case WallRebarAssemblyNew::Other:
			return;
		default:
			break;
		}
		wallRebar->PopvecFrontPts().clear();
		wallRebar->SetwallType(wallType);
		wallRebar->SetWallData(eeh);
		wallRebar->SetConcreteData(g_wallRebarInfo.concrete);
		wallRebar->SetRebarData(m_vecRebarData);
		wallRebar->SetvecLapOptions(m_vecLapOptionData);
		wallRebar->SetRebarEndTypes(m_vecEndTypeData);
		wallRebar->SetvecTwinRebarLevel(m_vecTwinBarData);
		//		wallRebar->InitRebarSetId();
		wallRebar->SetTieRebarInfo(m_tieRebarInfo);
		wallRebar->MakeRebars(ACTIVEMODEL);
		wallRebar->Save(ACTIVEMODEL); // must save after creating rebars
		ElementId contid = wallRebar->FetchConcrete();
		EditElementHandle eeh2(contid, ACTIVEMODEL);
		ElementRefP oldRef = eeh2.GetElementRef();
		mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
		eeh2.ReplaceInModel(oldRef);

		SetConcreteXAttribute(contid, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), &m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TieReBarInfo), &m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);

		SetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, eeh.GetModelRef());
		Transform trans;
		wallRebar->GetPlacement().AssignTo(trans);
		SetElementXAttribute(contid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, wallRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);

	}
	else
	{
		ACCRebarAssemblyNew::IsSmartSmartFeature(eeh);
		ElementId testid = 0;
		GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
		PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class STWallRebarAssembly");
		ACCRebarAssemblyNew*  rebar = NULL;
		Concrete concrete;
		PIT::ACCRebarAssemblyNew::ComponentType wallType = ACCRebarAssemblyNew::JudgeWallType(eeh);
		switch (wallType)
		{
		case ACCRebarAssemblyNew::ComponentType::SLAB:
		{
			// 			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class PIT::ACCSTWallRebarAssemblyNew");
			// 			rebar = dynamic_cast<ACCSTWallRebarAssemblyNew*>(rebaras);
			// 			if (rebar == nullptr)
			// 			{
			// 				rebar = REA::Create<ACCSTWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			// 			}
		}
		break;
		case ACCRebarAssemblyNew::ComponentType::STWALL:
		{
			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class PIT::ACCSTWallRebarAssemblyNew");
			rebar = dynamic_cast<ACCSTWallRebarAssemblyNew*>(rebaras);
			if (rebar == nullptr)
			{
				rebar = REA::Create<ACCSTWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
			concrete = g_wallRebarInfo.concrete;
		}
		break;
		case ACCRebarAssemblyNew::ComponentType::GWALL:
			//			wallRebar = REA::Create<GWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			break;
		case ACCRebarAssemblyNew::ComponentType::Other:
			return;
		default:
			break;
		}
		rebar->PopvecFrontPts().clear();
		rebar->SetcpType(wallType);
		rebar->SetComponentData(eeh);
		rebar->SetConcreteData(concrete);
		rebar->SetRebarData(m_vecRebarData);
		//		rebar->SetvecLapOptions(g_vecLapOptionData);
		rebar->SetRebarEndTypes(m_vecEndTypeData);
		rebar->SetvecTwinRebarLevel(m_vecTwinBarData);
		//		rebar->InitRebarSetId();
		rebar->SetTieRebarInfo(m_tieRebarInfo);
		rebar->SetACCRebarMethod(1);
		rebar->SetvecAC(m_vecACData);
		ACCRebarMakerNew::CreateACCRebar(rebar, eeh, m_vecRebarData, ACTIVEMODEL);
		rebar->Save(ACTIVEMODEL); // must save after creating rebars
		ElementId contid = rebar->FetchConcrete();
		EditElementHandle eeh2(contid, ACTIVEMODEL);
		ElementRefP oldRef = eeh2.GetElementRef();
		mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
		eeh2.ReplaceInModel(oldRef);
		//eeh2.AddToModel();
		SetConcreteXAttribute(contid, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), &m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TieReBarInfo), &m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);
		ACCConcrete ACconcrete;
		//先取出之前存储的数据
		GetElementXAttribute(eeh.GetElementId(), sizeof(ACCConcrete), ACconcrete, ConcreteCoverXAttribute, eeh.GetModelRef());
		//修改为最新配筋的保护层数据
		ACconcrete.postiveOrTopCover = concrete.postiveCover;
		ACconcrete.reverseOrBottomCover = concrete.reverseCover;
		ACconcrete.sideCover = concrete.sideCover;
		SetElementXAttribute(eeh.GetElementId(), sizeof(ACCConcrete), &ACconcrete, ConcreteCoverXAttribute, eeh.GetModelRef());
		SetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, eeh.GetModelRef());
		Transform trans;
		rebar->GetPlacement().AssignTo(trans);
		SetElementXAttribute(contid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, rebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
	}
}

void CWallRebarDlgNew::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = ehSel.GetModelRef();
	EditElementHandle eeh(ehSel, ehSel.GetModelRef());

	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	m_PageMainRebar.GetConcreteData(g_wallRebarInfo.concrete);
	m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	m_PageEndType.GetListRowData(m_vecEndTypeData);	//主要获取端部样式中端部属性的设置的新数据
	m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);	//主要获取端部样式中列表新数据
	m_PageEndType.SetListRowData(m_vecEndTypeData);	//修改为新的数据
	m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
	m_PageTwinBars.SetListRowData(m_vecTwinBarData);
	m_PageTieRebar.GetTieRebarData(m_tieRebarInfo);

	m_PageMainRebar.GetCutRebarInfo(m_stCutRebarInfo);

	if (m_PageMainRebar.m_assodlg)
	{
		m_PageMainRebar.m_assodlg->GetListRowData(m_vecACData);
	}

	bool bACRebar = false;
	for (size_t i = 0; i <m_vecACData.size(); ++i)
	{
		if (m_vecACData[i].associatedRelation == 1 || m_vecACData[i].associatedRelation == 2)	//存在一个为锚入即进行关联配筋
		{
			bACRebar = true;
			break;
		}
	}
	if (!bACRebar)
	{
		WallRebarAssemblyNew::IsSmartSmartFeature(eeh);
		WallRebarAssemblyNew*  wallRebar = NULL;
		ElementId testid = 0;
		GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());;

		WallRebarAssemblyNew::WallType wallType = WallRebarAssemblyNew::JudgeWallType(ehSel);
		switch (wallType)
		{
		case WallRebarAssemblyNew::STWALL:
		case WallRebarAssemblyNew::GWALL:
		{
			PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class PIT::ACCSTWallRebarAssemblyNew");
			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class STWallRebarAssemblyNew");
			wallRebar = dynamic_cast<STWallRebarAssemblyNew*>(rebaras);
			if (wallRebar == nullptr)
			{
				wallRebar = REA::Create<STWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
		}
		break;
		/*case WallRebarAssembly::GWALL:
			wallRebar = REA::Create<GWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			break;*/
		case WallRebarAssemblyNew::ARCWALL:	//弧形墙
		{
			// PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class PIT::ACCArcWallRebarAssembly");
			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class ArcWallRebarAssemblyNew");
			wallRebar = dynamic_cast<ArcWallRebarAssemblyNew*>(rebaras);
			if (wallRebar == nullptr)
			{
				wallRebar = REA::Create<ArcWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
			break;
		}
		case WallRebarAssemblyNew::ELLIPSEWall:	// 椭圆形墙
		{
			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class ELLWallRebarAssemblyNew");
			wallRebar = dynamic_cast<ELLWallRebarAssemblyNew*>(rebaras);
			if (wallRebar == nullptr)
			{
				wallRebar = REA::Create<ELLWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
		}
		break;
		case WallRebarAssemblyNew::Other:
			return;
		default:
			break;
		}
		wallRebar->SetstCutRebarInfo(m_stCutRebarInfo);
		wallRebar->SetwallName(wall_name);
		wallRebar->PopvecFrontPts().clear();
		wallRebar->SetwallType(wallType);
		wallRebar->SetWallData(eeh);
		wallRebar->SetConcreteData(g_wallRebarInfo.concrete);
		wallRebar->SetRebarData(m_vecRebarData);
		wallRebar->SetvecLapOptions(m_vecLapOptionData);
		wallRebar->SetRebarEndTypes(m_vecEndTypeData);
		wallRebar->SetvecTwinRebarLevel(m_vecTwinBarData);
//		wallRebar->InitRebarSetId();
		wallRebar->SetTieRebarInfo(m_tieRebarInfo);
		wallRebar->MakeRebars(modelRef);
		wallRebar->Save(modelRef); // must save after creating rebars
		ElementId contid = wallRebar->FetchConcrete();
		EditElementHandle eeh2(contid, ACTIVEMODEL);
		ElementRefP oldRef = eeh2.GetElementRef();
		mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
		eeh2.ReplaceInModel(oldRef);
		SetConcreteXAttribute(contid, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), &m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TieReBarInfo), &m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);

		SetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, ehSel.GetModelRef());
		Transform trans;
		wallRebar->GetPlacement().AssignTo(trans);
		SetElementXAttribute(contid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, wallRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(m_stCutRebarInfo), &m_stCutRebarInfo, stCNWallCutRebarInfo, ACTIVEMODEL);

	}
	else
	{

		ACCRebarAssemblyNew::IsSmartSmartFeature(eeh);
		ElementId testid = 0;
		GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
		PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class STWallRebarAssemblyNew");
		ACCRebarAssemblyNew*  rebar = NULL;
		Concrete concrete;
		PIT::ACCRebarAssemblyNew::ComponentType wallType = ACCRebarAssemblyNew::JudgeWallType(ehSel);
		switch (wallType)
		{
		case ACCRebarAssemblyNew::ComponentType::SLAB:
		{
// 			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class PIT::ACCSTWallRebarAssemblyNew");
// 			rebar = dynamic_cast<ACCSTWallRebarAssemblyNew*>(rebaras);
// 			if (rebar == nullptr)
// 			{
// 				rebar = REA::Create<ACCSTWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
// 			}
		}
		break;		
		case ACCRebarAssemblyNew::ComponentType::STWALL:
		{
			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class PIT::ACCSTWallRebarAssemblyNew");
			rebar = dynamic_cast<ACCSTWallRebarAssemblyNew*>(rebaras);
			if (rebar == nullptr)
			{
				rebar = REA::Create<ACCSTWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
			concrete = g_wallRebarInfo.concrete;
		}
		break;
		case ACCRebarAssemblyNew::ComponentType::GWALL:
//			wallRebar = REA::Create<GWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			break;
		case ACCRebarAssemblyNew::ComponentType::Other:
			return;
		default:
			break;
		}
		rebar->PopvecFrontPts().clear();
		rebar->SetcpType(wallType);
		rebar->SetComponentData(eeh);
		rebar->SetConcreteData(concrete);
		rebar->SetRebarData(m_vecRebarData);
//		rebar->SetvecLapOptions(g_vecLapOptionData);
		rebar->SetRebarEndTypes(m_vecEndTypeData);
		rebar->SetvecTwinRebarLevel(m_vecTwinBarData);
//		rebar->InitRebarSetId();
		rebar->SetTieRebarInfo(m_tieRebarInfo);
		rebar->SetACCRebarMethod(1);
		rebar->SetvecAC(m_vecACData);
		ACCRebarMakerNew::CreateACCRebar(rebar, ehSel, m_vecRebarData, modelRef);
		rebar->Save(modelRef); // must save after creating rebars
		ElementId contid = rebar->FetchConcrete();
		EditElementHandle eeh2(contid, ACTIVEMODEL);
		ElementRefP oldRef = eeh2.GetElementRef();
		mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
		eeh2.ReplaceInModel(oldRef);
		//eeh2.AddToModel();
    	SetConcreteXAttribute(contid, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), &m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TieReBarInfo), &m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);

		ACCConcrete ACconcrete;
		//先取出之前存储的数据
		GetElementXAttribute(ehSel.GetElementId(), sizeof(ACCConcrete), ACconcrete, ConcreteCoverXAttribute, ehSel.GetModelRef());
		//修改为最新配筋的保护层数据
		ACconcrete.postiveOrTopCover = concrete.postiveCover;
		ACconcrete.reverseOrBottomCover = concrete.reverseCover;
		ACconcrete.sideCover = concrete.sideCover;
// 		if (ACconcrete.offset < concrete.postiveCover)	//如果钢筋偏移小于保护层厚度
// 		{
// 			ACconcrete.offset = concrete.postiveCover;
// 		}
		SetElementXAttribute(ehSel.GetElementId(), sizeof(ACCConcrete), &ACconcrete, ConcreteCoverXAttribute, ehSel.GetModelRef());

		ElementId id = ehSel.GetElementId();
		SetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, ehSel.GetModelRef());
		Transform trans;
		rebar->GetPlacement().AssignTo(trans);
		SetElementXAttribute(contid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, rebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
	}

	//m_WallRebarLinesPtr存储的是预览函数中的线，在OK函数中清除预览函数画出来的线
	if (m_WallRebarLinesPtr)
	{
		m_WallRebarLinesPtr->ClearLines();
	}
	if (m_WallACCRebarLinesPtr)
	{
		m_WallACCRebarLinesPtr->ClearLines();
	}
	DestroyWindow();
}

void CWallRebarDlgNew::PreviewRebarLines()
{
	PreviewButtonDownNew = true;
	if (m_WallRebarLinesPtr)
	{
		m_WallRebarLinesPtr->ClearLines();
	}
	if (m_WallACCRebarLinesPtr)
	{
		m_WallACCRebarLinesPtr->ClearLines();
	}

	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = ehSel.GetModelRef();
	EditElementHandle eeh(ehSel, ehSel.GetModelRef());

	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	m_PageMainRebar.GetConcreteData(g_wallRebarInfo.concrete);
	m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	m_PageEndType.GetListRowData(m_vecEndTypeData);	//主要获取端部样式中端部属性的设置的新数据
	m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);	//主要获取端部样式中列表新数据
	m_PageEndType.SetListRowData(m_vecEndTypeData);	//修改为新的数据
	m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
	m_PageTwinBars.SetListRowData(m_vecTwinBarData);
	m_PageTieRebar.GetTieRebarData(m_tieRebarInfo);

	if (m_PageMainRebar.m_assodlg)
	{
		m_PageMainRebar.m_assodlg->GetListRowData(m_vecACData);
	}

	bool bACRebar = false;
	for (size_t i = 0; i < m_vecACData.size(); ++i)
	{
		if (m_vecACData[i].associatedRelation == 1 || m_vecACData[i].associatedRelation == 2)	//存在一个为锚入即进行关联配筋
		{
			bACRebar = true;
			break;
		}
	}
	if (!bACRebar)
	{
		WallRebarAssemblyNew::IsSmartSmartFeature(eeh);
		ElementId testid = 0;
		GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());

		WallRebarAssemblyNew::WallType wallType = WallRebarAssemblyNew::JudgeWallType(ehSel);
		switch (wallType)
		{
		case WallRebarAssemblyNew::STWALL:
		{
// 			PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class PIT::ACCSTWallRebarAssemblyNew");
// 			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class STWallRebarAssembly");
// 			wallRebar = dynamic_cast<STWallRebarAssembly*>(rebaras);
// 			if (wallRebar == nullptr)
// 			{
// 				wallRebar = REA::Create<STWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
// 			}
			if (m_WallRebarLinesPtr == NULL)
			{
				m_WallRebarLinesPtr = new STWallRebarAssemblyNew(eeh.GetElementId(), eeh.GetModelRef());
			}
		}
		break;
		case WallRebarAssemblyNew::GWALL:
			if (m_WallRebarLinesPtr == NULL)
			{
				m_WallRebarLinesPtr = new GWallRebarAssemblyNew(eeh.GetElementId(), eeh.GetModelRef());
			}
			break;
		case WallRebarAssemblyNew::ARCWALL:	//弧形墙
		{
			if (m_WallRebarLinesPtr == NULL)
			{
				m_WallRebarLinesPtr = new ArcWallRebarAssemblyNew(eeh.GetElementId(), eeh.GetModelRef());
			}
			break;
		}
		case WallRebarAssemblyNew::ELLIPSEWall:	// 椭圆形墙
		{
			if (m_WallRebarLinesPtr == nullptr)
			{
				m_WallRebarLinesPtr = new ELLWallRebarAssemblyNew(eeh.GetElementId(), eeh.GetModelRef());
			}
			break;
		}
		case WallRebarAssemblyNew::Other:
			return;
		default:
			break;
		}
		m_WallRebarLinesPtr->SetwallName(wall_name);
		m_WallRebarLinesPtr->PopvecFrontPts().clear();
		m_WallRebarLinesPtr->SetwallType(wallType);
		m_WallRebarLinesPtr->SetWallData(eeh);
		m_WallRebarLinesPtr->SetConcreteData(g_wallRebarInfo.concrete);
		m_WallRebarLinesPtr->SetRebarData(m_vecRebarData);
		m_WallRebarLinesPtr->SetvecLapOptions(m_vecLapOptionData);
		m_WallRebarLinesPtr->SetRebarEndTypes(m_vecEndTypeData);
		m_WallRebarLinesPtr->SetvecTwinRebarLevel(m_vecTwinBarData);
		m_WallRebarLinesPtr->SetTieRebarInfo(m_tieRebarInfo);
		m_WallRebarLinesPtr->MakeRebars(modelRef);

	}
	else
	{
		ACCRebarAssemblyNew::IsSmartSmartFeature(eeh);
		ElementId testid = 0;
		GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
		PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class STWallRebarAssemblyNew");
		Concrete concrete;
		PIT::ACCRebarAssemblyNew::ComponentType wallType = ACCRebarAssemblyNew::JudgeWallType(ehSel);
		switch (wallType)
		{
		case ACCRebarAssemblyNew::ComponentType::SLAB:
		{

		}
		break;
		case ACCRebarAssemblyNew::ComponentType::STWALL:
		{
			RebarAssembly* rebaras = ACCRebarAssemblyNew::GetRebarAssembly(testid, "class PIT::ACCSTWallRebarAssemblyNew");
			if (m_WallACCRebarLinesPtr == NULL)
			{
				m_WallACCRebarLinesPtr = dynamic_cast<ACCSTWallRebarAssemblyNew*>(rebaras);
				if (m_WallACCRebarLinesPtr == nullptr)
				{
					m_WallACCRebarLinesPtr = REA::Create<ACCSTWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
				}
			}

//			m_WallACCRebarLinesPtr = new ACCRebarAssemblyNew(eeh.GetElementId(), eeh.GetModelRef());
// 			rebar = dynamic_cast<ACCSTWallRebarAssemblyNew*>(rebaras);
// 			if (rebar == nullptr)
// 			{
// 				rebar = REA::Create<ACCSTWallRebarAssemblyNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
// 			}
			concrete = g_wallRebarInfo.concrete;
		}
		break;
		case ACCRebarAssemblyNew::ComponentType::GWALL:
			break;
		case ACCRebarAssemblyNew::ComponentType::Other:
			return;
		default:
			break;
		}
		m_WallACCRebarLinesPtr->PopvecFrontPts().clear();
		m_WallACCRebarLinesPtr->SetcpType(wallType);
		m_WallACCRebarLinesPtr->SetComponentData(eeh);
		m_WallACCRebarLinesPtr->SetConcreteData(concrete);
		m_WallACCRebarLinesPtr->SetRebarData(m_vecRebarData);
		m_WallACCRebarLinesPtr->SetRebarEndTypes(m_vecEndTypeData);
		m_WallACCRebarLinesPtr->SetvecTwinRebarLevel(m_vecTwinBarData);
		m_WallACCRebarLinesPtr->SetTieRebarInfo(m_tieRebarInfo);
		m_WallACCRebarLinesPtr->SetACCRebarMethod(1);
		m_WallACCRebarLinesPtr->SetvecAC(m_vecACData);
		ACCRebarMakerNew::CreateACCRebar(m_WallACCRebarLinesPtr, ehSel, m_vecRebarData, modelRef);
	}
	PreviewButtonDownNew = false;
}


void CWallRebarDlgNew::OnBnClickedCancel()
{

	CDialogEx::OnCancel();

	//清除非关联构建情况 预览画的线段
	if (m_WallRebarLinesPtr)
	{
		m_WallRebarLinesPtr->ClearLines();
	}

	//清除非联构建情况 预览画的线段
	if (m_WallACCRebarLinesPtr)
	{
		m_WallACCRebarLinesPtr->ClearLines();
	}
	DestroyWindow();
}


void CWallRebarDlgNew::OnCbnSelchangeCombo2()//直径
{
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboSize.GetCurSel());
 	strcpy(m_WallSetInfo.rebarSize, CT2A(*it));
	BrString str = *it;
	if (str != L"")
	{
		if (str.Find(L"mm") != -1)
		{
			str.Replace(L"mm", L"");
		}
	}
	strcpy(m_WallSetInfo.rebarSize, CT2A(str));
	m_PageMainRebar.UpdateRebarList();
}

void CWallRebarDlgNew::OnCbnSelchangeCombo3()//类型
{
	auto it = g_listRebarType.begin();
	advance(it, m_ComboType.GetCurSel());
	m_WallSetInfo.rebarType = m_ComboType.GetCurSel();
	m_PageMainRebar.ChangeRebarTypedata(m_WallSetInfo.rebarType);
	m_PageMainRebar.UpdateRebarList();
}


void CWallRebarDlgNew::OnEnChangeEdit1()//间距
{
	CString	strTemp = CString();
	m_EditSpace.GetWindowText(strTemp);
	m_WallSetInfo.spacing = atof(CT2A(strTemp));

	m_PageMainRebar.ChangeRebarSpacedata(m_WallSetInfo.spacing);
	m_PageMainRebar.UpdateRebarList();
}


void CWallRebarDlgNew::OnStnClickedStaticWallname()
{
	// TODO: 在此添加控件通知处理程序代码
}
