// CWallRebar.cpp: 实现文件
//
#include "_USTATION.h"
#include <afxdialogex.h>
#include <RebarDetailElement.h>
#include <RebarElements.h>
#include <ElementAttribute.h>
#include "CWallRebarDlg.h"

#include "ArcWallRebarAssembly.h"
#include "resource.h"
#include "WallRebarAssembly.h"
#include "CommonFile.h"
#include "ConstantsDef.h"
#include "ELLWallRebarAssembly.h"
#include "GWallRebarAssembly.h"
#include "PickElementTool.h"
#include "STGWallRebarAssembly.h"
#include "STWallRebarAssembly.h"


namespace _Wallrebar {
	/// @brief 判断一个元素是不是或板
	/// @details 这个是看PDMS参数中的Type中有没有WALL字样 
	bool is_wall_or_slab(EditElementHandleCR element)
	{
		std::string _name, type;
		if (!GetEleNameAndType(const_cast<EditElementHandleR>(element), _name, type))
		{
			return false;
		}
		auto result_pos_wall = type.find("WALL");
		auto result_pos_slab = type.find("FLOOR");
		if (result_pos_wall != std::string::npos || result_pos_slab != std::string::npos)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	/// @brief 判断一个元素是不是板
	/// @details 这个是看PDMS参数中的Type中有没有FLOOR字样
	bool isslab(EditElementHandleCR element)
	{
		std::string _name, type;
		GetEleNameAndType(element.GetElementId(), element.GetModelRef()/*ACTIVEMODEL*/, _name, type);
		if (type == "FLOOR")
			return true;
		else
			return false;

	}

	/// @brief 判断一个元素是不是墙
	/// @details 这个是看PDMS参数中的Type中有没有FLOOR字样
	bool iswall(EditElementHandleCR element)
	{
		std::string _name, type;
		GetEleNameAndType(element.GetElementId(), element.GetModelRef()/*ACTIVEMODEL*/, _name, type);
		if ((type == "STWALL") || (type == "GWALL") || (type == "ARCWALL") || (type == "ELLIPSEWALL") || (type == "STGWALL"))
			return true;
		else
			return false;

	}
}

using namespace PIT;

// CWallRebar 对话框

bool PreviewButtonDown = false; // 主要配筋界面的预览按钮

IMPLEMENT_DYNAMIC(CWallRebarDlg, CDialogEx)

CWallRebarDlg::CWallRebarDlg(ElementHandleCR eh, CWnd *pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_WallRebar, pParent), ehSel(eh), m_ConcreteId(0)
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
	m_ClickedFlag = 0;
}

// CWallRebarDlg::CWallRebarDlg(UINT Id, CWnd* pParent /*=nullptr*/)
// 	: CBModelessDialog(Id, pParent), m_ConcreteId(0)
// {
//
// }
CWallRebarDlg::~CWallRebarDlg()
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

void CWallRebarDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

void CWallRebarDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_WallRebar, m_tab);
	DDX_Control(pDX, IDC_COMBO2, m_ComboSize);
	DDX_Control(pDX, IDC_COMBO3, m_ComboType);
	DDX_Control(pDX, IDC_EDIT1, m_EditSpace);
	DDX_Control(pDX, IDC_STATIC_WALLNAME, m_static_wallname);
	DDX_Control(pDX, IDCANCEL, m_thickness);
	DDX_Control(pDX, IDC_THICKNESS, m_wallthickness);
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

BEGIN_MESSAGE_MAP(CWallRebarDlg, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_WallRebar, &CWallRebarDlg::OnTcnSelchangeTabWallrebar)
	ON_BN_CLICKED(IDOK, &CWallRebarDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CWallRebarDlg::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CWallRebarDlg::OnCbnSelchangeCombo2)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CWallRebarDlg::OnCbnSelchangeCombo3)
	ON_EN_CHANGE(IDC_EDIT1, &CWallRebarDlg::OnEnChangeEdit1)
	ON_STN_CLICKED(IDC_STATIC_WALLNAME, &CWallRebarDlg::OnStnClickedStaticWallname)
	ON_BN_CLICKED(IDC_SELECT_MODEL, &CWallRebarDlg::OnBnClickedSelectModel)
END_MESSAGE_MAP()

// CWallRebar 消息处理程序
void CWallRebarDlg::GetThickness(double &thickness)
{

	DgnModelRefP modelRef = ACTIVEMODEL;
	DgnModelRefP tmpModel = ehSel.GetModelRef();
	EditElementHandle eeh(ehSel, ehSel.GetModelRef());
	double height = 0;
	EditElementHandle eea;
	eea.Duplicate(eeh);

	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownBackLine;

	std::vector<EditElementHandle *> SHoleeh;
	EditElementHandle Eleeh;
	EFT::GetSolidElementAndSolidHoles(eea, Eleeh, SHoleeh);
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &height);
	map<double, double> mapthickness;
	CalculateThickness(vecDownFontLine, vecDownBackLine, mapthickness);
	if (mapthickness.size() > 0)
	{
		thickness = mapthickness.begin()->second;
		thickness = thickness + 0.1;
	}
	else
	{
		thickness = 0;
	}

	for (MSElementDescrP tmpDescr : vecDownFaceLine)
	{
		mdlElmdscr_freeAll(&tmpDescr);
		tmpDescr = nullptr;
	}
}

BOOL CWallRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	//设置提示字体
	CWnd* pWnd = GetDlgItem(IDC_STATIC_Prompt);
	m_font.CreatePointFont(120, _T("宋体"), NULL);
	this->SetWindowText(L"墙配筋");
	if (m_isCombineWall)
		this->SetWindowText(L"廊道墙合并配筋");
	pWnd->SetFont(&m_font);

	// 双击配过的钢筋或者点选这个墙会读取这个墙的配筋参数
	// 点选多个墙时只会显示默认参数
	int flag = 1;
	ElementAgenda agenda;
	if (m_isCmdWall)//如果不是双击钢筋打开配筋表需要检测是否选中构建
	{
		GetSelectAgenda(agenda, L"请选择廊道中的墙和板");
		if (agenda.GetCount() == 1)
		{
			if (GetSelectAgenda(agenda, L"请选择廊道中的墙和板"))
			{
				g_SelectedElm = agenda[0];
				SetSelectElement(g_SelectedElm);
				string elename, eletype;
				GetEleNameAndType(ehSel.GetElementId(), ehSel.GetModelRef(), elename, eletype);
				wall_name = elename;
				CString wallname(elename.c_str());
				wallname = L"墙名:" + wallname;
				m_static_wallname.SetWindowTextW(wallname);
				double thickness = 0;
				GetThickness(thickness);
				double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
				thickness /= uor_per_mm;
				char a[10];
				itoa((int)thickness, a, 10);
				CString Thickness(a);
				Thickness = L"墙厚:" + Thickness + L"mm";
				m_wallthickness.SetWindowTextW(Thickness);
			}

		}
		else
			flag = 0;
	}
	else
	{
		string elename, eletype;
		GetEleNameAndType(ehSel.GetElementId(), ehSel.GetModelRef(), elename, eletype);
		wall_name = elename;
		CString wallname(elename.c_str());
		wallname = L"墙名:" + wallname;
		m_static_wallname.SetWindowTextW(wallname);
		double thickness = 0;
		GetThickness(thickness);
		double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		thickness /= uor_per_mm;
		char a[10];
		itoa((int)thickness, a, 10);
		CString Thickness(a);
		Thickness = L"墙厚:" + Thickness + L"mm";
		m_wallthickness.SetWindowTextW(Thickness);
	}
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE); // 让对话框界面显示在最前端
	for each (auto var in g_listRebarSize)
		m_ComboSize.AddString(var);
	for each (auto var in g_listRebarType)
		m_ComboType.AddString(var);

	ElementId contid = 0;
	if (nullptr != ehSel.GetElementRef() && flag)
		GetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, ehSel.GetModelRef());
	GetElementXAttribute(contid, sizeof(PIT::WallRebarInfo), g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, sizeof(TieReBarInfo), m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);
	vector<PIT::BreakAngleData> breakAngleData;
	GetElementXAttribute(contid, breakAngleData, vecBreakDataXAttribute, ACTIVEMODEL);

	if (m_vecRebarData.size() == 0)
	{
		m_WallSetInfo.rebarType = 2;
	}

	CString strRebarSize(m_WallSetInfo.rebarSize);
	if (strRebarSize.Find(L"mm") == -1)
		strRebarSize += "mm";
	int nIndex = m_ComboSize.FindStringExact(0, strRebarSize);
	m_ComboSize.SetCurSel(nIndex);					// 尺寸
	m_ComboType.SetCurSel(m_WallSetInfo.rebarType); // 型号
	CString strSpace;
	strSpace.Format(L"%.2f", m_WallSetInfo.spacing); // 保护层
	//	m_EditSpace.SetWindowText(strSpace);

	// 	m_PageMainRebar.getTieRebarData(m_tieRebarInfo);//将拉筋及并筋信息传入m_PageMainRebar
	// 	m_PageMainRebar.getTwinRowData(m_vecTwinBarData);

	m_PageMainRebar.m_isWall = true;
	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	m_PageMainRebar.SetBreakAngleData(breakAngleData);

	m_PageMainRebar.SavePtr(this);

	// TODO:  在此添加额外的初始化
	// 为Tab Control增加两个页面
	m_tab.InsertItem(0, _T("主要配筋"));
	// 创建两个对话框
	WallRebarAssembly::WallType wallType = WallRebarAssembly::STWALL;
	if (wallType == WallRebarAssembly::WallType::Other) {
		mdlOutput_printf(MSG_STATUS, L"无法获取墙类型");
		this->SendMessage(WM_CLOSE);
		return FALSE;
	}
	if (wallType != WallRebarAssembly::ARCWALL && wallType != WallRebarAssembly::ELLIPSEWall)
	{
		g_wallRebarInfo.concrete.m_SlabRebarMethod = 0;
	}
	m_PageMainRebar.Create(IDD_DIALOG_WallRebar_MainRebar, &m_tab);
	m_PageMainRebar.SetSelectElement(ehSel);
	if (wallType != WallRebarAssembly::ARCWALL && wallType != WallRebarAssembly::ELLIPSEWall)
	{
		m_PageMainRebar.GetDlgItem(IDC_COMBO1)->ShowWindow(SW_HIDE);
		m_PageMainRebar.GetDlgItem(IDC_STATIC_SlabRebarMehod)->ShowWindow(SW_HIDE); // 隐藏板的配筋方式
	}
	m_PageMainRebar.GetDlgItem(IDC_BUTTON3)->ShowWindow(SW_HIDE); // 隐藏选择墙按钮
	if (wallType == WallRebarAssembly::ELLIPSEWall)
	{
		m_PageMainRebar.GetDlgItem(IDC_BUTTON_BreakEllipseWall)->ShowWindow(SW_SHOW);
	}

	// 设定在Tab内显示的范围
	CRect rc;
	m_tab.GetClientRect(rc);
	rc.top += 20;
	rc.bottom -= 0;
	rc.left += 0;
	rc.right -= 0;
	m_PageMainRebar.MoveWindow(&rc);

	// 把对话框对象指针保存起来
	pDialog[0] = &m_PageMainRebar;

	// 显示初始页面
	pDialog[0]->ShowWindow(SW_SHOW);

	// 保存当前选择
	m_CurSelTab = 0;

	g_ConcreteId = m_ConcreteId;

	ACCConcrete wallACConcrete;
	if (nullptr != ehSel.GetElementRef())
	{
		int ret = GetElementXAttribute(ehSel.GetElementId(), sizeof(ACCConcrete), wallACConcrete, ConcreteCoverXAttribute, ehSel.GetModelRef());
		if (ret == SUCCESS) // 关联构件配筋时存储过数据,优先使用关联构件设置的保护层
		{
			g_wallRebarInfo.concrete.postiveCover = wallACConcrete.postiveOrTopCover;
			g_wallRebarInfo.concrete.reverseCover = wallACConcrete.reverseOrBottomCover;
			g_wallRebarInfo.concrete.sideCover = wallACConcrete.sideCover;
		}
	}

	return TRUE; // return TRUE unless you set the focus to a control
				 // 异常: OCX 属性页应返回 FALSE
}

void CWallRebarDlg::OnTcnSelchangeTabWallrebar(NMHDR *pNMHDR, LRESULT *pResult)
{
	//// TODO: 在此添加控件通知处理程序代码
	//// 把当前的页面隐藏起来
	//pDialog[m_CurSelTab]->ShowWindow(SW_HIDE);
	//// 得到新的页面索引
	//m_CurSelTab = m_tab.GetCurSel();

	//switch (m_CurSelTab)
	//{
	//case 0:
	//{
	//	m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
	//	m_PageEndType.SetListRowData(m_vecEndTypeData);
	//	m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
	//	m_PageTwinBars.SetListRowData(m_vecTwinBarData);
	//	m_PageMainRebar.UpdateRebarList();
	//}
	//break;
	//case 1:
	//{
	//	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	//	// m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	//	m_PageMainRebar.GetAllWallRebarData(m_vecRebarData);
	//	m_PageMainRebar.SetListRowData(m_vecRebarData);
	//	m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
	//	m_PageTwinBars.SetListRowData(m_vecTwinBarData);
	//	m_PageEndType.m_vecRebarData = m_vecRebarData;
	//	m_PageEndType.UpdateEndTypeList();

	//	//		m_PageEndType.m_ListEndType.GetAllRebarData(g_vecEndTypeData);

	//	// 		CWallRebarEndTypeDlg* dlg = (CWallRebarEndTypeDlg*)pDialog[m_CurSelTab];
	//	// 		dlg->UpdateEndTypeList();
	//	//		m_PageEndType.m_ListEndType.GetAllRebarData(g_vecEndTypeData);
	//	// 		m_PageEndType.SetListRowData(g_vecEndTypeData);
	//	// 		m_PageEndType.UpdateEndTypeList();
	//}
	//break;
	//case 2:
	//{
	//	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	//	// m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	//	m_PageMainRebar.GetAllWallRebarData(m_vecRebarData);
	//	m_PageMainRebar.SetListRowData(m_vecRebarData);
	//	m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
	//	m_PageEndType.SetListRowData(m_vecEndTypeData);
	//	m_PageTwinBars.m_vecRebarData = m_vecRebarData;
	//	m_PageTwinBars.UpdateTwinBarsList();

	//	//		CTwinBarSetDlg* dlg = (CTwinBarSetDlg*)pDialog[m_CurSelTab];
	//	// 		m_PageTwinBars.m_ListTwinBars.GetAllRebarData(g_vecTwinBarData);
	//	// 		m_PageTwinBars.SetListRowData(g_vecTwinBarData);
	//}
	//break;
	//case 3:
	//{
	//	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	//	// m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	//	m_PageMainRebar.GetAllWallRebarData(m_vecRebarData);
	//	m_PageMainRebar.SetListRowData(m_vecRebarData);
	//	m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
	//	m_PageEndType.SetListRowData(m_vecEndTypeData);
	//	m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
	//	m_PageTwinBars.SetListRowData(m_vecTwinBarData);
	//	m_PageMainRebar.UpdateRebarList();
	//	m_PageTieRebar.m_vecRebarData = m_vecRebarData;
	//	m_PageTieRebar.OnInitDialog();
	//}
	//break;
	//default:
	//	break;
	//}

	//// 把新的页面显示出来
	//pDialog[m_CurSelTab]->ShowWindow(SW_SHOW);
	//*pResult = 0;
}
void CWallRebarDlg::RefreshWallRebars(ElementId conid, EditElementHandleR eeh)
{

	std::vector<PIT::ConcreteRebar> m_vecRebarData;
	std::vector<PIT::LapOptions> m_vecLapOptionData;
	std::vector<PIT::EndType> m_vecEndTypeData;
	std::vector<PIT::AssociatedComponent> m_vecACData;
	std::vector<TwinBarSet::TwinBarLevelInfo> m_vecTwinBarData;
	TwinBarSet::TwinBarInfo m_twinBarInfo;
	TieReBarInfo m_tieRebarInfo;
	GetElementXAttribute(conid, sizeof(PIT::WallRebarInfo), g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, sizeof(TwinBarSet::TwinBarInfo), m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(conid, sizeof(TieReBarInfo), m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);

	/***********************************给sizekey附加型号******************************************************/
	auto it = m_vecRebarData.begin();
	for (; it != m_vecRebarData.end(); it++)
	{
		BrString strRebarSize = it->rebarSize;
		if (strRebarSize != L"")
		{
			if (strRebarSize.Find(L"mm") != -1)
			{
				strRebarSize.Replace(L"mm", L"");
			}
		}
		else
		{
			strRebarSize = XmlManager::s_alltypes[it->rebarType];
		}
		strcpy(it->rebarSize, CT2A(strRebarSize));
		GetDiameterAddType(it->rebarSize, it->rebarType);
	}

	auto itt = m_vecTwinBarData.begin(); // 给sizekey附加型号
	for (; itt != m_vecTwinBarData.end(); itt++)
	{
		GetDiameterAddType(itt->rebarSize, itt->rebarType);
	}

	GetDiameterAddType(m_tieRebarInfo.rebarSize, m_tieRebarInfo.rebarType);
	/***********************************给sizekey附加型号******************************************************/
	bool bACRebar = false;
	for (size_t i = 0; i < m_vecACData.size(); ++i)
	{
		if (m_vecACData[i].associatedRelation == 1 || m_vecACData[i].associatedRelation == 2) // 存在一个为锚入即进行关联配筋
		{
			bACRebar = true;
			break;
		}
	}
	if (!bACRebar)
	{
		WallRebarAssembly::IsSmartSmartFeature(eeh);
		WallRebarAssembly *wallRebar = NULL;
		ElementId testid = 0;
		GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());

		WallRebarAssembly::WallType wallType = WallRebarAssembly::JudgeWallType(eeh);
		switch (wallType)
		{
		case WallRebarAssembly::STWALL:
		{
			PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class PIT::ACCSTWallRebarAssembly");
			RebarAssembly *rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class STWallRebarAssembly");
			wallRebar = dynamic_cast<STWallRebarAssembly *>(rebaras);
			if (wallRebar == nullptr)
			{
				wallRebar = REA::Create<STWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
		}
		break;
		case WallRebarAssembly::GWALL:
			wallRebar = REA::Create<GWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			break;
		case WallRebarAssembly::ARCWALL: // 弧形墙
		{
			// PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class PIT::ACCArcWallRebarAssembly");
			RebarAssembly *rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class ArcWallRebarAssembly");
			wallRebar = dynamic_cast<ArcWallRebarAssembly *>(rebaras);
			if (wallRebar == nullptr)
			{
				wallRebar = REA::Create<ArcWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
			break;
		}
		case WallRebarAssembly::ELLIPSEWall: // 椭圆形墙
		{
			RebarAssembly *rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class ELLWallRebarAssembly");
			wallRebar = dynamic_cast<ELLWallRebarAssembly *>(rebaras);
			if (wallRebar == nullptr)
			{
				wallRebar = REA::Create<ELLWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
		}
		break;
		case WallRebarAssembly::Other:
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

		/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
		auto it = m_vecRebarData.begin();
		for (; it != m_vecRebarData.end(); it++)
		{
			BrString strRebarSize = it->rebarSize;
			strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1); // 删掉型号
			strcpy(it->rebarSize, CT2A(strRebarSize));
		}

		auto itt = m_vecTwinBarData.begin(); // 给sizekey附加型号
		for (; itt != m_vecTwinBarData.end(); itt++)
		{
			BrString strTwinSize = itt->rebarSize;
			strTwinSize = strTwinSize.Left(strTwinSize.GetLength() - 1); // 删掉型号
			strcpy(itt->rebarSize, CT2A(strTwinSize));
		}

		BrString strTieSize = m_tieRebarInfo.rebarSize;
		strTieSize = strTieSize.Left(strTieSize.GetLength() - 1); // 删掉型号
		strcpy(m_tieRebarInfo.rebarSize, CT2A(strTieSize));
		/***********************************给sizekey去除型号再保存到模型中 ******************************************************/

		SetConcreteXAttribute(contid, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(PIT::WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), &m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TieReBarInfo), &m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);

		ACCConcrete ACconcrete;
		// 先取出之前存储的数据
		GetElementXAttribute(eeh.GetElementId(), sizeof(ACCConcrete), ACconcrete, ConcreteCoverXAttribute, eeh.GetModelRef());
		// 修改为最新配筋的保护层数据
		ACconcrete.postiveOrTopCover = g_wallRebarInfo.concrete.postiveCover;
		ACconcrete.reverseOrBottomCover = g_wallRebarInfo.concrete.reverseCover;
		ACconcrete.sideCover = g_wallRebarInfo.concrete.sideCover;
		SetElementXAttribute(eeh.GetElementId(), sizeof(ACCConcrete), &ACconcrete, ConcreteCoverXAttribute, eeh.GetModelRef());

		SetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, eeh.GetModelRef());
		Transform trans;
		wallRebar->GetPlacement().AssignTo(trans);
		SetElementXAttribute(contid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, wallRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
	}
	else
	{
		// 暂时不需要关联构件配筋
		// ACCRebarAssembly::IsSmartSmartFeature(eeh);
		// ElementId testid = 0;
		// GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
		// PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class STWallRebarAssembly");
		// ACCRebarAssembly *rebar = NULL;
		// Concrete concrete;
		// PIT::ACCRebarAssembly::ComponentType wallType = ACCRebarAssembly::JudgeWallType(eeh);
		// switch (wallType)
		// {
		// case ACCRebarAssembly::ComponentType::SLAB:
		// {
		// 		RebarAssembly* rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class PIT::ACCSTWallRebarAssembly");
		// 		rebar = dynamic_cast<ACCSTWallRebarAssembly*>(rebaras);
		// 		if (rebar == nullptr)
		// 		{
		// 			rebar = REA::Create<ACCSTWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		// 		}
		// }
		// break;
		// case ACCRebarAssembly::ComponentType::STWALL:
		// {
		// 	RebarAssembly *rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class PIT::ACCSTWallRebarAssembly");
		// 	rebar = dynamic_cast<ACCSTWallRebarAssembly *>(rebaras);
		// 	if (rebar == nullptr)
		// 	{
		// 		rebar = REA::Create<ACCSTWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		// 	}
		// 	concrete = g_wallRebarInfo.concrete;
		// }
		// break;
		// case ACCRebarAssembly::ComponentType::GWALL:
		// 	//			wallRebar = REA::Create<GWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		// 	break;
		// case ACCRebarAssembly::ComponentType::Other:
		// 	return;
		// default:
		// 	break;
		// }
		// rebar->PopvecFrontPts().clear();
		// rebar->SetcpType(wallType);
		// rebar->SetComponentData(eeh);
		// rebar->SetConcreteData(concrete);
		// rebar->SetRebarData(m_vecRebarData);
		// //		rebar->SetvecLapOptions(g_vecLapOptionData);
		// rebar->SetRebarEndTypes(m_vecEndTypeData);
		// rebar->SetvecTwinRebarLevel(m_vecTwinBarData);
		// //		rebar->InitRebarSetId();
		// rebar->SetTieRebarInfo(m_tieRebarInfo);
		// rebar->SetACCRebarMethod(1);
		// rebar->SetvecAC(m_vecACData);
		// ACCRebarMaker::CreateACCRebar(rebar, eeh, m_vecRebarData, ACTIVEMODEL);
		// rebar->Save(ACTIVEMODEL); // must save after creating rebars
		// ElementId contid = rebar->FetchConcrete();
		// EditElementHandle eeh2(contid, ACTIVEMODEL);
		// ElementRefP oldRef = eeh2.GetElementRef();
		// mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
		// eeh2.ReplaceInModel(oldRef);
		// // eeh2.AddToModel();

		// /***********************************给sizekey去除型号再保存到模型中 ******************************************************/
		// auto it = m_vecRebarData.begin(); // 给sizekey去除型号再保存到模型中
		// for (; it != m_vecRebarData.end(); it++)
		// {
		// 	BrString strRebarSize = it->rebarSize;
		// 	strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1); // 删掉型号
		// 	strcpy(it->rebarSize, CT2A(strRebarSize));
		// }

		// auto itt = m_vecTwinBarData.begin(); // 给sizekey附加型号
		// for (; itt != m_vecTwinBarData.end(); itt++)
		// {
		// 	BrString strTwinSize = itt->rebarSize;
		// 	strTwinSize = strTwinSize.Left(strTwinSize.GetLength() - 1); // 删掉型号
		// 	strcpy(itt->rebarSize, CT2A(strTwinSize));
		// }

		// BrString strTieSize = m_tieRebarInfo.rebarSize;
		// strTieSize = strTieSize.Left(strTieSize.GetLength() - 1); // 删掉型号
		// strcpy(m_tieRebarInfo.rebarSize, CT2A(strTieSize));
		// /***********************************给sizekey去除型号再保存到模型中 ******************************************************/

		// SetConcreteXAttribute(contid, ACTIVEMODEL);
		// SetElementXAttribute(contid, sizeof(WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		// SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		// SetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
		// SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
		// SetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
		// SetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), &m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
		// SetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
		// SetElementXAttribute(contid, sizeof(TieReBarInfo), &m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);
		// ACCConcrete ACconcrete;
		// // 先取出之前存储的数据
		// GetElementXAttribute(eeh.GetElementId(), sizeof(ACCConcrete), ACconcrete, ConcreteCoverXAttribute, eeh.GetModelRef());
		// // 修改为最新配筋的保护层数据
		// ACconcrete.postiveOrTopCover = concrete.postiveCover;
		// ACconcrete.reverseOrBottomCover = concrete.reverseCover;
		// ACconcrete.sideCover = concrete.sideCover;
		// SetElementXAttribute(eeh.GetElementId(), sizeof(ACCConcrete), &ACconcrete, ConcreteCoverXAttribute, eeh.GetModelRef());
		// SetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, eeh.GetModelRef());
		// Transform trans;
		// rebar->GetPlacement().AssignTo(trans);
		// SetElementXAttribute(contid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
		// SetElementXAttribute(contid, rebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
		// SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
	}
}

void CWallRebarDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	//CDialogEx::OnOK();
	ElementAgenda tmpagenda;
	ElementAgenda agenda;
	agenda.clear();
	if (m_isCmdWall)//如果不是双击钢筋打开配筋表需要检测是否选中构建
	{
		/*if (!GetSelectAgenda(tmpagenda, L"请选择廊道中的墙和板"))
		{
			return;
		}*/
		if (m_isCombineWall)
		{
			this->m_ClickedFlag = 0;
			g_SelectedElm = ehSel;
			m_PageMainRebar.m_hole_check.SetCheck(false);
			g_wallRebarInfo.concrete.isHandleHole = 0;
			this->SetSelectElement(ehSel);
			this->on_Clicke_maker();
			g_wallRebarInfo.concrete.isHandleHole = 1;
			return;
		}
		GetSelectAgenda(tmpagenda, L"请选择廊道中的墙和板");
		this->m_ClickedFlag = 1;
		for (int i = 0; i < tmpagenda.GetCount(); ++i)
		{
			auto tmpehR = tmpagenda[i];
			std::string name, type;
			GetEleNameAndType(tmpehR, name, type);
			if ((type == "STWALL") || (type == "GWALL") || (type == "ARCWALL") || (type == "ELLIPSEWALL") || (type == "STGWALL"))
			{
				agenda.push_back(tmpehR);
			}
			else
			{
				std::vector<IDandModelref>  Same_Eles;
				GetNowScanElems(tmpehR, Same_Eles);
				bool up = false, down = false;
				DPoint3d minP2 = { 0 }, maxP2 = { 0 };
				mdlElmdscr_computeRange(&minP2, &maxP2, tmpehR.GetElementDescrP(), NULL);
				minP2.z -= 500; maxP2.z += 500;
				for (auto itr : Same_Eles)
				{
					EditElementHandle tmpeeh2(itr.ID, itr.tModel);
					DPoint3d minP = { 0 }, maxP = { 0 };
					mdlElmdscr_computeRange(&minP, &maxP, tmpeeh2.GetElementDescrP(), NULL);
					DRange3d range;
					range.low = minP;
					range.high = maxP;

					if (range.IsContained(maxP2))
						up = true;
					if (range.IsContained(minP2))
						down = true;
				}
				if (down&&up)
				{
					agenda.push_back(tmpehR);
				}

			}
		}

		for (int i = 0; i < agenda.GetCount(); ++i)
		{
			//如果是单独配筋
			if (!m_isCombineWall)
			{
				g_SelectedElm = agenda[i];
				EditElementHandle eehtemp(g_SelectedElm, g_SelectedElm.GetModelRef());
				/*单独板配筋时，如果板联合配筋过，重新设置联合id ---modify by hzh str*/
				ElementId eehId = g_SelectedElm.GetElementId();
				ElementId unionId = 0;
				int ret = GetElementXAttribute(eehId, sizeof(ElementId), unionId, UnionWallIDXAttribute, g_SelectedElm.GetModelRef());
				if (ret == SUCCESS)
				{
					SetElementXAttribute(eehId, sizeof(ElementId), &eehId, UnionWallIDXAttribute, g_SelectedElm.GetModelRef());
				}
				/*单独板配筋时，如果板联合配筋过，重新设置联合id ---modify by hzh end*/
				this->SetSelectElement(g_SelectedElm);
				//this->ArcRebarMethod(eehtemp);
			}
			g_SelectedElm = agenda[i];
			this->SetSelectElement(g_SelectedElm);
			this->on_Clicke_maker();
		}
		//delete this;
	}
	else
	{
		this->on_Clicke_maker();
		//delete this;
	}

}

void CWallRebarDlg::on_Clicke_maker()
{
	DgnModelRefP modelRef = ACTIVEMODEL;
	DgnModelRefP tmpModel = ehSel.GetModelRef();
	EditElementHandle eeh(ehSel, ehSel.GetModelRef());

	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	m_PageMainRebar.GetConcreteData(g_wallRebarInfo.concrete);
	m_PageMainRebar.GetAllWallRebarData(m_vecRebarData);
	// m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);

	/***********************************给sizekey附加型号******************************************************/
	auto it = m_vecRebarData.begin();
	for (; it != m_vecRebarData.end(); it++)
	{
		BrString strRebarSize = it->rebarSize;
		if (strRebarSize != L"")
		{
			if (strRebarSize.Find(L"mm") != -1)
			{
				strRebarSize.Replace(L"mm", L"");
			}
		}
		else
		{
			strRebarSize = XmlManager::s_alltypes[it->rebarType];
		}
		strcpy(it->rebarSize, CT2A(strRebarSize));
		GetDiameterAddType(it->rebarSize, it->rebarType);
	}

	auto itt = m_vecTwinBarData.begin(); // 给sizekey附加型号
	for (; itt != m_vecTwinBarData.end(); itt++)
	{
		GetDiameterAddType(itt->rebarSize, itt->rebarType);
	}

	GetDiameterAddType(m_tieRebarInfo.rebarSize, m_tieRebarInfo.rebarType);
	/***********************************给sizekey附加型号******************************************************/
	if (m_PageMainRebar.m_assodlg)
	{
		m_PageMainRebar.m_assodlg->GetListRowData(m_vecACData);
	}

	bool bACRebar = false;
	for (size_t i = 0; i < m_vecACData.size(); ++i)
	{
		if (m_vecACData[i].associatedRelation == 1 || m_vecACData[i].associatedRelation == 2 || (bool)(m_vecACData[i].isCut) == true) // 存在一个为锚入即进行关联配筋
		{
			bACRebar = true;
			break;
		}
	}
	if (!bACRebar)
	{
		WallRebarAssembly::IsSmartSmartFeature(eeh);
		WallRebarAssembly *wallRebar = NULL;
		ElementId testid = 0;
		GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());

		WallRebarAssembly::WallType wallType = WallRebarAssembly::JudgeWallType(ehSel);
		wallType = WallRebarAssembly::WallType::STWALL;
		{
			PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class PIT::ACCSTWallRebarAssembly");
			RebarAssembly *rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class STWallRebarAssembly");
			wallRebar = dynamic_cast<STWallRebarAssembly *>(rebaras);
			if (wallRebar == nullptr)
			{
				wallRebar = REA::Create<STWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
		}

		wallRebar->SetwallName(wall_name);
		wallRebar->PopvecFrontPts().clear();
		wallRebar->SetwallType(wallType);
		wallRebar->SetWallData(eeh);
		wallRebar->SetConcreteData(g_wallRebarInfo.concrete);
		wallRebar->SetRebarData(m_vecRebarData);
		wallRebar->SetvecLapOptions(m_vecLapOptionData);
		wallRebar->SetRebarEndTypes(m_vecEndTypeData);
		wallRebar->SetvecTwinRebarLevel(m_vecTwinBarData);
		wallRebar->InitRebarSetId();
		wallRebar->SetTieRebarInfo(m_tieRebarInfo);
		wallRebar->MakeRebars(modelRef);
		wallRebar->Save(modelRef); // must save after creating rebars
		ElementId contid = wallRebar->FetchConcrete();
		EditElementHandle eeh2(contid, ACTIVEMODEL);
		ElementRefP oldRef = eeh2.GetElementRef();
		mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
		eeh2.ReplaceInModel(oldRef);

		if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2) // 放射配筋
		{
			vector<PIT::ConcreteRebar> newVecData;
			wallRebar->GetRebarData(newVecData);
			m_PageMainRebar.SetListRowData(newVecData);
		}

		/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
		auto it = m_vecRebarData.begin();
		for (; it != m_vecRebarData.end(); it++)
		{
			BrString strRebarSize = it->rebarSize;
			strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1); // 删掉型号
			strcpy(it->rebarSize, CT2A(strRebarSize));
		}

		auto itt = m_vecTwinBarData.begin(); // 给sizekey附加型号
		for (; itt != m_vecTwinBarData.end(); itt++)
		{
			BrString strTwinSize = itt->rebarSize;
			strTwinSize = strTwinSize.Left(strTwinSize.GetLength() - 1); // 删掉型号
			strcpy(itt->rebarSize, CT2A(strTwinSize));
		}

		BrString strTieSize = m_tieRebarInfo.rebarSize;
		strTieSize = strTieSize.Left(strTieSize.GetLength() - 1); // 删掉型号
		strcpy(m_tieRebarInfo.rebarSize, CT2A(strTieSize));
		/***********************************给sizekey去除型号再保存到模型中 ******************************************************/

		SetConcreteXAttribute(contid, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(PIT::WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), &m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TieReBarInfo), &m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);
		ACCConcrete ACconcrete;
		// 先取出之前存储的数据
		GetElementXAttribute(eeh.GetElementId(), sizeof(ACCConcrete), ACconcrete, ConcreteCoverXAttribute, eeh.GetModelRef());
		// 修改为最新配筋的保护层数据
		ACconcrete.postiveOrTopCover = g_wallRebarInfo.concrete.postiveCover;
		ACconcrete.reverseOrBottomCover = g_wallRebarInfo.concrete.reverseCover;
		ACconcrete.sideCover = g_wallRebarInfo.concrete.sideCover;
		SetElementXAttribute(eeh.GetElementId(), sizeof(ACCConcrete), &ACconcrete, ConcreteCoverXAttribute, eeh.GetModelRef());
		SetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, ehSel.GetModelRef());
		Transform trans;
		wallRebar->GetPlacement().AssignTo(trans);
		SetElementXAttribute(contid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, wallRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_PageMainRebar.GetBreakAngleData(), vecBreakDataXAttribute, ACTIVEMODEL);
	}

	// m_WallRebarLinesPtr存储的是预览函数中的线，在OK函数中清除预览函数画出来的线
	if (m_WallRebarLinesPtr)
	{
		m_WallRebarLinesPtr->ClearLines();
	}
	if (m_WallACCRebarLinesPtr)
	{
		m_WallACCRebarLinesPtr->ClearLines();
	}
	//DestroyWindow();
	if (this->m_ClickedFlag == 0)
	{
		this->SendMessage(WM_CLOSE);
		delete this;
	}
	//this->m_ClickedFlag = 0;
}

void CWallRebarDlg::PreviewRebarLines()
{
	PreviewButtonDown = true;
	if (m_WallRebarLinesPtr)
	{
		m_WallRebarLinesPtr->ClearLines();
	}
	if (m_WallACCRebarLinesPtr)
	{
		m_WallACCRebarLinesPtr->ClearLines();
	}

	DgnModelRefP modelRef = ACTIVEMODEL;
	DgnModelRefP tmpModel = ehSel.GetModelRef();
	EditElementHandle eeh(ehSel, ehSel.GetModelRef());

	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	m_PageMainRebar.GetConcreteData(g_wallRebarInfo.concrete);
	// m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.GetAllWallRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);

	/***********************************给sizekey附加型号******************************************************/
	auto it = m_vecRebarData.begin();
	for (; it != m_vecRebarData.end(); it++)
	{
		BrString strRebarSize = it->rebarSize;
		if (strRebarSize != L"")
		{
			if (strRebarSize.Find(L"mm") != -1)
			{
				strRebarSize.Replace(L"mm", L"");
			}
		}
		else
		{
			strRebarSize = XmlManager::s_alltypes[it->rebarType];
		}
		strcpy(it->rebarSize, CT2A(strRebarSize));
		GetDiameterAddType(it->rebarSize, it->rebarType);
	}

	auto itt = m_vecTwinBarData.begin(); // 给sizekey附加型号
	for (; itt != m_vecTwinBarData.end(); itt++)
	{
		GetDiameterAddType(itt->rebarSize, itt->rebarType);
	}

	GetDiameterAddType(m_tieRebarInfo.rebarSize, m_tieRebarInfo.rebarType);
	/***********************************给sizekey附加型号******************************************************/

	if (m_PageMainRebar.m_assodlg)
	{
		m_PageMainRebar.m_assodlg->GetListRowData(m_vecACData);
	}

	bool bACRebar = false;
	for (size_t i = 0; i < m_vecACData.size(); ++i)
	{
		if (m_vecACData[i].associatedRelation == 1 || m_vecACData[i].associatedRelation == 2) // 存在一个为锚入即进行关联配筋
		{
			bACRebar = true;
			break;
		}
	}
	if (!bACRebar)
	{
		WallRebarAssembly::IsSmartSmartFeature(eeh);
		ElementId testid = 0;
		GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());

		WallRebarAssembly::WallType wallType = WallRebarAssembly::JudgeWallType(ehSel);
		switch (wallType)
		{
		case WallRebarAssembly::STWALL:
		{
			// 			PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class PIT::ACCSTWallRebarAssembly");
			// 			RebarAssembly* rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class STWallRebarAssembly");
			// 			wallRebar = dynamic_cast<STWallRebarAssembly*>(rebaras);
			// 			if (wallRebar == nullptr)
			// 			{
			// 				wallRebar = REA::Create<STWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			// 			}
			if (m_WallRebarLinesPtr == NULL)
			{
				m_WallRebarLinesPtr = new STWallRebarAssembly(eeh.GetElementId(), eeh.GetModelRef());
			}
		}
		break;
		case WallRebarAssembly::GWALL:
			if (m_WallRebarLinesPtr == NULL)
			{
				m_WallRebarLinesPtr = new GWallRebarAssembly(eeh.GetElementId(), eeh.GetModelRef());
			}
			break;
		case WallRebarAssembly::STGWALL:
			if (m_WallRebarLinesPtr == NULL)
			{
				m_WallRebarLinesPtr = new STGWallRebarAssembly(eeh.GetElementId(), eeh.GetModelRef());
			}
			break;
		case WallRebarAssembly::ARCWALL: // 弧形墙
		{
			if (m_WallRebarLinesPtr == NULL)
			{
				m_WallRebarLinesPtr = new ArcWallRebarAssembly(eeh.GetElementId(), eeh.GetModelRef());
			}
			break;
		}
		case WallRebarAssembly::ELLIPSEWall: // 椭圆形墙
		{
			if (m_WallRebarLinesPtr == nullptr)
			{
				m_WallRebarLinesPtr = new ELLWallRebarAssembly(eeh.GetElementId(), eeh.GetModelRef());
			}
			break;
		}
		case WallRebarAssembly::Other:
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
		m_WallRebarLinesPtr->InitRebarSetId();
		m_WallRebarLinesPtr->MakeRebars(modelRef);
	}
	else
	{
		// 暂时不需要关联构件配筋 
		// ACCRebarAssembly::IsSmartSmartFeature(eeh);
		// ElementId testid = 0;
		// GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
		// PIT::PITRebarAssembly::DeleteRebarsFromAssembly(testid, "class STWallRebarAssembly");
		// Concrete concrete;
		// PIT::ACCRebarAssembly::ComponentType wallType = ACCRebarAssembly::JudgeWallType(ehSel);
		// switch (wallType)
		// {
		// case ACCRebarAssembly::ComponentType::SLAB:
		// {
		// }
		// break;
		// case ACCRebarAssembly::ComponentType::STWALL:
		// {
		// 	RebarAssembly *rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class PIT::ACCSTWallRebarAssembly");
		// 	if (m_WallACCRebarLinesPtr == NULL)
		// 	{
		// 		m_WallACCRebarLinesPtr = dynamic_cast<ACCSTWallRebarAssembly *>(rebaras);
		// 		if (m_WallACCRebarLinesPtr == nullptr)
		// 		{
		// 			m_WallACCRebarLinesPtr = REA::Create<ACCSTWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		// 		}
		// 	}

		// 	//			m_WallACCRebarLinesPtr = new ACCRebarAssembly(eeh.GetElementId(), eeh.GetModelRef());
		// 	// 			rebar = dynamic_cast<ACCSTWallRebarAssembly*>(rebaras);
		// 	// 			if (rebar == nullptr)
		// 	// 			{
		// 	// 				rebar = REA::Create<ACCSTWallRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		// 	// 			}
		// 	concrete = g_wallRebarInfo.concrete;
		// }
		// break;
		// case ACCRebarAssembly::ComponentType::GWALL:
		// 	break;
		// case ACCRebarAssembly::ComponentType::Other:
		// 	return;
		// default:
		// 	break;
		// }
		// m_WallACCRebarLinesPtr->PopvecFrontPts().clear();
		// m_WallACCRebarLinesPtr->SetcpType(wallType);
		// m_WallACCRebarLinesPtr->SetComponentData(eeh);
		// m_WallACCRebarLinesPtr->SetConcreteData(concrete);
		// m_WallACCRebarLinesPtr->SetRebarData(m_vecRebarData);
		// m_WallACCRebarLinesPtr->SetRebarEndTypes(m_vecEndTypeData);
		// m_WallACCRebarLinesPtr->SetvecTwinRebarLevel(m_vecTwinBarData);
		// m_WallACCRebarLinesPtr->SetTieRebarInfo(m_tieRebarInfo);
		// m_WallACCRebarLinesPtr->SetACCRebarMethod(1);
		// m_WallACCRebarLinesPtr->SetvecAC(m_vecACData);
		// ACCRebarMaker::CreateACCRebar(m_WallACCRebarLinesPtr, ehSel, m_vecRebarData, modelRef);
	}
	PreviewButtonDown = false;
}

void CWallRebarDlg::OnBnClickedCancel()
{

	CDialogEx::OnCancel();

	// 清除非关联构建情况 预览画的线段
	if (m_WallRebarLinesPtr)
	{
		m_WallRebarLinesPtr->ClearLines();
	}

	// 清除非联构建情况 预览画的线段
	if (m_WallACCRebarLinesPtr)
	{
		m_WallACCRebarLinesPtr->ClearLines();
	}
	DestroyWindow();
}

void CWallRebarDlg::OnCbnSelchangeCombo2() // 直径
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
	m_PageMainRebar.ChangeRebarSizedata(m_WallSetInfo.rebarSize);
	m_PageMainRebar.UpdateRebarList();
}

void CWallRebarDlg::OnCbnSelchangeCombo3() // 类型
{
	auto it = g_listRebarType.begin();
	advance(it, m_ComboType.GetCurSel());
	m_WallSetInfo.rebarType = m_ComboType.GetCurSel();
	m_PageMainRebar.ChangeRebarTypedata(m_WallSetInfo.rebarType);
	m_PageMainRebar.UpdateRebarList();
}

void CWallRebarDlg::OnEnChangeEdit1() // 间距
{
	CString strTemp = CString();
	m_EditSpace.GetWindowText(strTemp);
	m_WallSetInfo.spacing = atof(CT2A(strTemp));

	m_PageMainRebar.ChangeRebarSpacedata(m_WallSetInfo.spacing);
	m_PageMainRebar.UpdateRebarList();
}

void CWallRebarDlg::OnStnClickedStaticWallname()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CWallRebarDlg::OnBnClickedSelectModel()
{
	// TODO: 在此添加控件通知处理程序代码
	m_ClickedFlag = 0;

	auto select_wall_tool = new PickElementTool(
		// on_filter
		_Wallrebar::iswall,
		// on_complete
		[this](const ElementAgenda &agenda)
		{
			for (auto &entry : agenda)
			{
				/*CString str;
				str.Format(L"id = %d", entry.GetElementId());
				mdlDialog_dmsgsPrint(str);

				mdlDialog_dmsgsPrintA("\n");*/

				ElementHandle tmpeeh(entry.GetElementId(), entry.GetModelRef());
				this->SetSelectElement(tmpeeh);
				string elename, eletype;
				GetEleNameAndType(ehSel.GetElementId(), ehSel.GetModelRef(), elename, eletype);
				wall_name = elename;
				CString wallname(elename.c_str());
				wallname = L"墙名:" + wallname;
				m_static_wallname.SetWindowTextW(wallname);
				double thickness = 0;
				GetThickness(thickness);
				double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
				thickness /= uor_per_mm;
				char a[10];
				itoa((int)thickness, a, 10);
				CString Thickness(a);
				Thickness = L"墙厚:" + Thickness + L"mm";
				m_wallthickness.SetWindowTextW(Thickness);
				/*ElementId contid = 0;
				GetElementXAttribute(tmpeeh.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, tmpeeh.GetModelRef());
				GetElementXAttribute(contid, sizeof(WallRebarInfo), g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
				GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
				GetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
				GetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
				GetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
				GetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
				GetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
				GetElementXAttribute(contid, sizeof(TieReBarInfo), m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);

				m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
				m_PageMainRebar.SetListRowData(m_vecRebarData);
				m_PageEndType.SetListRowData(m_vecEndTypeData);
				m_PageEndType.m_vecRebarData = m_vecRebarData;
				m_PageTwinBars.SetListRowData(m_vecTwinBarData);
				m_PageTwinBars.m_vecRebarData = m_vecRebarData;
				m_PageTieRebar.SetTieRebarData(m_tieRebarInfo);

				m_PageTieRebar.ShowWindow(SW_HIDE);
				m_PageMainRebar.SavePtr(this);*/

				this->OnBnClickedOk();
			}
			// 设置当前窗口焦点
			this->SetFocus();
		},
		// on_cancel
			[this](void)
		{

			//this->SetFocus();
		});

	select_wall_tool->InstallTool();
	m_ClickedFlag = 1;
}
