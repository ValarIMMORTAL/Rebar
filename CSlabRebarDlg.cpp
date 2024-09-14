// CSlabRebarDlg.cpp: 实现文件
//
#include "_USTATION.h"
#include "CSlabRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "afxdialogex.h"
#include "MySlabRebarAssembly.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "CommonFile.h"
#include "ElementAttribute.h"
#include "XmlHelper.h"
#include "ConstantsDef.h"
#include "ACCRebarMaker.h"
#include "LDSlabRebarAssembly.h"
#include "PickElementTool.h"
#include "WallHelper.h"

namespace _local {
	/// @brief 判断一个元素是不是或板  
	/// @details 这个是看PDMS参数中的Type中有没有WALL字样  NOLINT(clang-diagnostic-null-character)
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

	/// @brief 从选中的元素中读取设置
	/// @param settings 
	/// @return 成功返回true
	/*bool read_settings_from_selected_elements(const ElementAgenda &agenda, GallerySettings &settings)
	{
		for (auto &element : agenda)
		{
			if (GallerySettings::from_element(element, settings))
			{
				return true;
			}
		}

		return false;
	}*/
}

// CSlabRebarDlg 对话框
extern GlobalParameters g_globalpara;	//全局参数

bool SlabPreviewButtonDown = false;//板主要配筋界面的预览按钮

using namespace Gallery;
IMPLEMENT_DYNAMIC(CSlabRebarDlg, CDialogEx)

CSlabRebarDlg::CSlabRebarDlg(ElementHandleCR eh, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_SlabRebar, pParent), m_ConcreteId(0)
{
	m_slabRebar = nullptr;
	ehSel = eh;
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

CSlabRebarDlg::~CSlabRebarDlg()
{
	// 	if (m_slabRebar)
	// 	{
	// 		delete m_slabRebar;
	// 		m_slabRebar = NULL;
	// 	}
}


void CSlabRebarDlg::ArcRebarMethod(EditElementHandleR eh)//用户选择了放射配筋方式:但是没有选择线段画弧线时可以直接解析板件的底面，找打弧线
{
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;
	double height;
	EditElementHandle Eleeh;
	EditElementHandle testeeh(eh, false);
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);

	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &height);
	for (int i = 0; i < vecDownFaceLine.size(); i++)
	{
		if (vecDownFaceLine[i] != nullptr)
		{
			if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
			{
				double starR, sweepR;
				double radius;
				DPoint3d ArcDPs[2];
				RotMatrix rotM;
				DPoint3d centerpt;
				mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &vecDownFaceLine[i]->el);
				double ArcLen;
				mdlElmdscr_distanceAtPoint(&ArcLen, nullptr, nullptr, vecDownFaceLine[i], &ArcDPs[1], 0.1);
				DPoint3d midPt = { 0,0,0 };
				mdlElmdscr_pointAtDistance(&midPt, nullptr, ArcLen / 2, vecDownFaceLine[i], 0.1);
				m_PageMainRebar.mArcLine.centerpt = centerpt;
				m_PageMainRebar.mArcLine.ptStart = ArcDPs[0];
				m_PageMainRebar.mArcLine.ptEnd = ArcDPs[1];
				m_PageMainRebar.mArcLine.radius = radius;
				m_PageMainRebar.m_height = centerpt.z;//当用户画弧线时，没有画在贴合底面的位置：直接将获取到的圆心坐标Z值赋给弧线的z值
				m_PageMainRebar.mArcLine.ArcLen = ArcLen;
				m_PageMainRebar.mArcLine.ptMid = midPt;
				m_PageMainRebar.mArcLine.isCircle = false;
				if (ArcDPs[0].AlmostEqual(ArcDPs[1])) //圆
				{
					MSElement newDescr;
					mdlArc_create(&newDescr, NULL, &centerpt, radius, radius, nullptr, 0, 2 * PI - 0.05);
					DPoint3d newArcDPs[2];
					RotMatrix rotM;
					mdlArc_extract(newArcDPs, nullptr, nullptr, &radius, NULL, &rotM, &centerpt, &newDescr);
					MSElementDescrP ptmp = MSElementDescr::Allocate(newDescr);
					//mdlElmdscr_add(ptmp);
					m_PageMainRebar.mArcLine.ptStart = newArcDPs[0];
					m_PageMainRebar.mArcLine.ptEnd = newArcDPs[1];
					//mdlElmdscr_distanceAtPoint(&m_PageMainRebar.mArcLine.ArcLen, nullptr, nullptr, vecDownFaceLine[i], &newArcDPs[1], 0.1);
					m_PageMainRebar.mArcLine.isCircle = true;
				}
				break;
			}
		}
	}

	if (vecDownFaceLine.size() == 0)
	{
		vector<MSElementDescrP> vecEllipse;
		EFT::GetEllipseDownFace(Eleeh, vecEllipse, Holeehs, &height);

		double minRadius = 0.0;
		for (MSElementDescrP ms : vecEllipse)
		{
			double dRadius = 0.0;
			DPoint3d centerpt;
			DPoint3d ArcDPs[2];

			mdlArc_extract(ArcDPs, NULL, NULL, &dRadius, NULL, NULL, &centerpt, &ms->el);
			DPoint3d midPt = { 0,0,0 };
			mdlElmdscr_pointAtDistance(&midPt, nullptr, dRadius * PI, ms, 0.1);
			mdlElmdscr_freeAll(&ms);

			if (COMPARE_VALUES_EPS(dRadius, m_PageMainRebar.mArcLine.radius, EPS) > 0) // 最大半径的圆
			{
				m_PageMainRebar.mArcLine.radius = dRadius;
				m_PageMainRebar.mArcLine.ptStart = ArcDPs[0];
				m_PageMainRebar.mArcLine.ptEnd = ArcDPs[1];
				m_PageMainRebar.mArcLine.centerpt = centerpt;
				m_PageMainRebar.mArcLine.ArcLen = 2.0 * dRadius * PI;
				m_PageMainRebar.mArcLine.ptMid = midPt;
				m_PageMainRebar.m_height = centerpt.z;
				m_PageMainRebar.mArcLine.isCircle = false;
				if (ArcDPs[0].AlmostEqual(ArcDPs[1])) //圆
				{
					MSElement newDescr;
					mdlArc_create(&newDescr, NULL, &centerpt, dRadius, dRadius, nullptr, 0, 2 * PI - 0.05);
					DPoint3d newArcDPs[2];
					RotMatrix rotM;
					mdlArc_extract(newArcDPs, nullptr, nullptr, &dRadius, NULL, &rotM, &centerpt, &newDescr);
					MSElementDescrP ptmp = MSElementDescr::Allocate(newDescr);
					//mdlElmdscr_add(ptmp);
					m_PageMainRebar.mArcLine.ptStart = newArcDPs[0];
					m_PageMainRebar.mArcLine.ptEnd = newArcDPs[1];
					//mdlElmdscr_distanceAtPoint(&m_PageMainRebar.mArcLine.ArcLen, nullptr, nullptr, ptmp, &newArcDPs[1], 0.1);
					m_PageMainRebar.mArcLine.isCircle = true;
				}
			}
		}
	}

}


void CSlabRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO2, m_ComboSize);
	DDX_Control(pDX, IDC_COMBO3, m_ComboType);
	DDX_Control(pDX, IDC_EDIT1, m_EditSpace);
	DDX_Control(pDX, IDC_TAB_SlabRebar, m_tab);
	DDX_Control(pDX, IDC_STATIC_PANELNAME, m_static_panelname);
	DDX_Control(pDX, IDC_CHECK1, m_repeat);
}


BEGIN_MESSAGE_MAP(CSlabRebarDlg, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_SlabRebar, &CSlabRebarDlg::OnTcnSelchangeTabSlabrebar)
	ON_BN_CLICKED(IDOK, &CSlabRebarDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSlabRebarDlg::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CSlabRebarDlg::OnCbnSelchangeCombo2)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CSlabRebarDlg::OnCbnSelchangeCombo3)
	ON_EN_CHANGE(IDC_EDIT1, &CSlabRebarDlg::OnEnChangeEdit1)
	ON_BN_CLICKED(IDC_CHECK1, &CSlabRebarDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_SELECT_MODEL, &CSlabRebarDlg::OnBnClickedSelectModel)
	ON_BN_CLICKED(IDC_SELECT_MODEL, &CSlabRebarDlg::OnBnClickedSelectModel)
	ON_BN_CLICKED(IDC_SELECT_MODEL2, &CSlabRebarDlg::OnBnClickedSelectModel2)
END_MESSAGE_MAP()


// CSlabRebarDlg 消息处理程序

BOOL CSlabRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	this->SetWindowText(L"廊道板配筋");
	if (m_isCombineSlab)
		this->SetWindowText(L"廊道板合并配筋");
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	for each (auto var in g_listRebarSize)
	{
		m_ComboSize.AddString(var);
	}
	for each (auto var in g_listRebarType)
	{
		m_ComboType.AddString(var);
	}
	//设置提示字体
	CWnd* pWnd = GetDlgItem(IDC_STATIC_Prompt);
	m_font.CreatePointFont(120, _T("宋体"), NULL);
	pWnd->SetFont(&m_font);

	// 双击配过的钢筋或者点选这个板会读取这个板的配筋参数
	// 点选多个板时只会显示默认参数
	int flag = 1;
	ElementAgenda agenda;
	if (m_isCmdSlab)//如果不是双击钢筋打开配筋表需要检测是否选中构建
	{
		GetSelectAgenda(agenda, L"请选择廊道中的墙和板");
		if (agenda.GetCount() == 1)
		{
			g_SelectedElm = agenda[0];
			SetSelectElement(g_SelectedElm);
		}
		else
			flag = 0;
	}
	string elename, eletype;
	//GetEleNameAndType(ehSel.GetElementId(), ehSel.GetModelRef(), elename, eletype);
	//CString panelname(elename.c_str());
	//panelname = L"板名:" + panelname;
	//m_static_panelname.SetWindowTextW(panelname);
	ElementId contid = 0;
	if (nullptr != ehSel.GetElementRef() && flag)
		GetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, ehSel.GetModelRef());
	GetElementXAttribute(contid, sizeof(WallRebarInfo), g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, sizeof(TieReBarInfo), m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);

	if (m_vecRebarData.size() == 0)
	{
		m_SlabSetInfo.rebarType = 2;
	}

	CString strRebarSize(m_SlabSetInfo.rebarSize);
	if (strRebarSize.Find(L"mm") == -1)
		strRebarSize += "mm";
	int nIndex = m_ComboSize.FindStringExact(0, strRebarSize);
	m_ComboSize.SetCurSel(nIndex);//尺寸
	m_ComboType.SetCurSel(m_SlabSetInfo.rebarType);//型号
	CString strSpace;
	strSpace.Format(L"%.2f", m_SlabSetInfo.spacing);//保护层

	m_PageMainRebar.m_isFloor = true;
	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	m_PageMainRebar.SavePtr(this);

	// TODO:  在此添加额外的初始化
	//为Tab Control增加两个页面
	m_tab.InsertItem(0, _T("主要配筋"));
	//创建两个对话框
	m_PageMainRebar.Create(IDD_DIALOG_WallRebar_MainRebar, &m_tab);
	m_PageMainRebar.SetSelectElement(ehSel);

		//设定在Tab内显示的范围
	CRect rc;
	m_tab.GetClientRect(rc);
	rc.top += 20;
	rc.bottom -= 0;
	rc.left += 0;
	rc.right -= 0;
	m_PageMainRebar.MoveWindow(&rc);

	//把对话框对象指针保存起来
	pDialog[0] = &m_PageMainRebar;

	//显示初始页面
	pDialog[0]->ShowWindow(SW_SHOW);

	pDialog[0]->GetDlgItem(IDC_BUTTON1)->ShowWindow(FALSE);

	//保存当前选择
	m_CurSelTab = 0;

	g_ConcreteId = m_ConcreteId;
	m_repeat.SetCheck(TRUE);//沿用廊道配筋规则
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CSlabRebarDlg::OnTcnSelchangeTabSlabrebar(NMHDR *pNMHDR, LRESULT *pResult)
{
	//// TODO: 在此添加控件通知处理程序代码
	////把当前的页面隐藏起来
	//pDialog[m_CurSelTab]->ShowWindow(SW_HIDE);
	////得到新的页面索引
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
	//	m_PageMainRebar.getWallSetInfo(m_SlabSetInfo);
	//	//m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	//	m_PageMainRebar.GetAllRebarData(m_vecRebarData);
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
	//	m_PageMainRebar.getWallSetInfo(m_SlabSetInfo);
	//	//m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	//	m_PageMainRebar.GetAllRebarData(m_vecRebarData);
	//	m_PageMainRebar.SetListRowData(m_vecRebarData);
	//	m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
	//	m_PageEndType.SetListRowData(m_vecEndTypeData);
	//	m_PageTwinBars.m_vecRebarData = m_vecRebarData;
	//	m_PageTwinBars.m_vecRebarData = m_vecRebarData;
	//	m_PageTwinBars.UpdateTwinBarsList();

	//	//		CTwinBarSetDlg* dlg = (CTwinBarSetDlg*)pDialog[m_CurSelTab];
	//	// 		m_PageTwinBars.m_ListTwinBars.GetAllRebarData(g_vecTwinBarData);
	//	// 		m_PageTwinBars.SetListRowData(g_vecTwinBarData);
	//}
	//break;
	//case 3:
	//{
	//	m_PageMainRebar.getWallSetInfo(m_SlabSetInfo);
	//	//m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	//	m_PageMainRebar.GetAllRebarData(m_vecRebarData);
	//	m_PageMainRebar.SetListRowData(m_vecRebarData);
	//	m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
	//	m_PageEndType.SetListRowData(m_vecEndTypeData);
	//	m_PageTwinBars.m_ListTwinBars.GetAllRebarData(m_vecTwinBarData);
	//	m_PageTwinBars.SetListRowData(m_vecTwinBarData);
	//	m_PageTwinBars.m_vecRebarData = m_vecRebarData;
	//	m_PageMainRebar.UpdateRebarList();
	//	m_PageTieRebar.OnInitDialog();
	//	m_PageTieRebar.m_vecRebarData = m_vecRebarData;
	//}
	//break;
	//default:
	//	break;
	//}
	////把新的页面显示出来
	//pDialog[m_CurSelTab]->ShowWindow(SW_SHOW);
	//*pResult = 0;
}
void CSlabRebarDlg::RefreshSlabRebars(ElementId conid, EditElementHandleR eeh)
{
	std::vector<PIT::ConcreteRebar>					m_vecRebarData;
	std::vector<PIT::LapOptions>						m_vecLapOptionData;
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
	MySlabRebarAssembly*  slabRebar = NULL;
	auto itt = m_vecTwinBarData.begin();//给sizekey附加型号
	for (; itt != m_vecTwinBarData.end(); itt++)
	{
		GetDiameterAddType(itt->rebarSize, itt->rebarType);
	}
	GetDiameterAddType(m_tieRebarInfo.rebarSize, m_tieRebarInfo.rebarType);
	/***********************************给sizekey附加型号******************************************************/
	SlabType slabType = MySlabRebarAssembly::JudgeSlabType(eeh);
	RebarAssembly* rebaras = ACCRebarAssembly::GetRebarAssembly(conid, "class STSlabRebarAssembly");
	slabRebar = dynamic_cast<STSlabRebarAssembly*>(rebaras);
	if (slabRebar == nullptr)
	{
		slabRebar = REA::Create<STSlabRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	}

	slabRebar->SetslabType(slabType);                  //设置墙类型
	slabRebar->SetSlabData(eeh);                       //设置墙坐标
	slabRebar->SetConcreteData(g_wallRebarInfo.concrete);  //设置三个保护层信息和层数//SetConcreteData函数用引用方式把g_wallRebarInfo.concrete值传到wallRebar
	slabRebar->SetRebarData(m_vecRebarData);               //设置钢筋信息
	slabRebar->SetvecLapOptions(g_vecLapOptionData);
	slabRebar->SetRebarEndTypes(m_vecEndTypeData);
	slabRebar->SetvecTwinRebarLevel(m_vecTwinBarData);
	slabRebar->InitRebarSetId();
	slabRebar->SetTieRebarInfo(m_tieRebarInfo);
	slabRebar->MakeRebars(ACTIVEMODEL);
	slabRebar->Save(ACTIVEMODEL); // must save after creating rebars
	ElementId contid = slabRebar->FetchConcrete();
	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);
	//eeh2.AddToModel();
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	it = m_vecRebarData.begin();
	for (; it != m_vecRebarData.end(); it++)
	{
		BrString strRebarSize = it->rebarSize;
		strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
		strcpy(it->rebarSize, CT2A(strRebarSize));
	}

	itt = m_vecTwinBarData.begin();//给sizekey附加型号
	for (; itt != m_vecTwinBarData.end(); itt++)
	{
		BrString strTwinSize = itt->rebarSize;
		strTwinSize = strTwinSize.Left(strTwinSize.GetLength() - 1);	//删掉型号
		strcpy(itt->rebarSize, CT2A(strTwinSize));
	}

	BrString strTieSize = m_tieRebarInfo.rebarSize;
	strTieSize = strTieSize.Left(strTieSize.GetLength() - 1);	//删掉型号
	strcpy(m_tieRebarInfo.rebarSize, CT2A(strTieSize));
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	SetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, eeh.GetModelRef());
	SetElementXAttribute(contid, sizeof(WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), &m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, sizeof(TieReBarInfo), &m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);

	Transform trans;
	slabRebar->GetPlacement().AssignTo(trans);
	SetElementXAttribute(contid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, slabRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
}
void CSlabRebarDlg::OnBnClickedOk()
{
	ElementAgenda tmpagenda;
	ElementAgenda agenda;
	agenda.clear();
	if(m_isCmdSlab)//如果不是双击钢筋打开配筋表需要检测是否选中构建
	{
		/*if (!GetSelectAgenda(tmpagenda, L"请选择廊道中的墙和板"))
		{
			return;
		}*/
		GetSelectAgenda(tmpagenda, L"请选择廊道中的墙和板");
		for (int i = 0; i < tmpagenda.GetCount(); ++i)
		{
			auto tmpehR = tmpagenda[i];
			std::string name, type;
			GetEleNameAndType(tmpehR, name, type);
			if (type == "FLOOR")
			{
				agenda.push_back(tmpehR);
			}
		}

		for (int i = 0; i < agenda.GetCount(); ++i)
		{
			//如果是单独配筋
			if (!m_isCombineSlab)
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
				this->ArcRebarMethod(eehtemp);
			}

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
void CSlabRebarDlg::on_Clicke_maker()
{
	//CDialogEx::OnOK();

	m_PageMainRebar.getWallSetInfo(m_SlabSetInfo);
	m_PageMainRebar.GetConcreteData(g_wallRebarInfo.concrete);
	//m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);

	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = ehSel.GetModelRef();
	tmpModel = tmpModel;
	EditElementHandle eeh(ehSel, ehSel.GetModelRef());
	ElementId testid = 0;
	GetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
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

	auto itt = m_vecTwinBarData.begin();//给sizekey附加型号
	for (; itt != m_vecTwinBarData.end(); itt++)
	{
		GetDiameterAddType(itt->rebarSize, itt->rebarType);
	}

	GetDiameterAddType(m_tieRebarInfo.rebarSize, m_tieRebarInfo.rebarType);
	/***********************************给sizekey附加型号******************************************************/
	if (repeate)//如果应用廊道板配筋时
	{
		//repeate = 0;
		MySlabRebarAssembly*  slabRebar = NULL;
		SlabType slabType = MySlabRebarAssembly::JudgeSlabType(ehSel);
		//switch (slabType)
		//{
		//case STSLAB:
		RebarAssembly* rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class Gallery::LDSlabRebarAssembly");
		slabRebar = dynamic_cast<LDSlabRebarAssembly*>(rebaras);
		if (slabRebar == nullptr)
		{
			slabRebar = REA::Create<LDSlabRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		}
		/*else
		{
			if (repeate != 0)
			{
				slabRebar = REA::Create<LDSlabRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
		}*/
		if (m_isCombineSlab)
			slabRebar->SetisCombineFloor(true);
		slabRebar->SetslabType(slabType);                  //设置墙类型
		slabRebar->SetConcreteData(g_wallRebarInfo.concrete);  //设置三个保护层信息和层数//SetConcreteData函数用引用方式把g_wallRebarInfo.concrete值传到wallRebar
		slabRebar->SetSlabRebarDir(m_PageMainRebar.m_seg, m_PageMainRebar.mArcLine);//设置板的配筋方向后相关的信息
		slabRebar->SetSlabData(eeh);                       //设置墙坐标
		slabRebar->SetRebarData(m_vecRebarData);               //设置钢筋信息
		slabRebar->SetvecLapOptions(g_vecLapOptionData);
		slabRebar->SetRebarEndTypes(m_vecEndTypeData);
		slabRebar->SetvecTwinRebarLevel(m_vecTwinBarData);
		slabRebar->InitRebarSetId();
		slabRebar->SetTieRebarInfo(m_tieRebarInfo);
		slabRebar->MakeRebars(modelRef);
		slabRebar->Save(modelRef); // must save after creating rebars
		ElementId contid = slabRebar->FetchConcrete();
		EditElementHandle eeh2(contid, ACTIVEMODEL);
		ElementRefP oldRef = eeh2.GetElementRef();
		mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
		eeh2.ReplaceInModel(oldRef);

		/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
		it = m_vecRebarData.begin();
		for (; it != m_vecRebarData.end(); it++)
		{
			BrString strRebarSize = it->rebarSize;
			strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
			strcpy(it->rebarSize, CT2A(strRebarSize));
		}

		itt = m_vecTwinBarData.begin();//给sizekey附加型号
		for (; itt != m_vecTwinBarData.end(); itt++)
		{
			BrString strTwinSize = itt->rebarSize;
			strTwinSize = strTwinSize.Left(strTwinSize.GetLength() - 1);	//删掉型号
			strcpy(itt->rebarSize, CT2A(strTwinSize));
		}

		BrString strTieSize = m_tieRebarInfo.rebarSize;
		strTieSize = strTieSize.Left(strTieSize.GetLength() - 1);	//删掉型号
		strcpy(m_tieRebarInfo.rebarSize, CT2A(strTieSize));
		/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
		SetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, ehSel.GetModelRef());

		ACCConcrete concrete;
		concrete.postiveOrTopCover = g_wallRebarInfo.concrete.postiveCover;
		concrete.reverseOrBottomCover = g_wallRebarInfo.concrete.reverseCover;
		concrete.sideCover = g_wallRebarInfo.concrete.sideCover;
		SetElementXAttribute(ehSel.GetElementId(), sizeof(ACCConcrete), &concrete, ConcreteCoverXAttribute, ehSel.GetModelRef());
		SetElementXAttribute(contid, sizeof(WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), &m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TieReBarInfo), &m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);

		Transform trans;
		slabRebar->GetPlacement().AssignTo(trans);
		SetElementXAttribute(contid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);

		SetElementXAttribute(contid, slabRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
		//SetElementXAttribute(contid, g_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		if (m_slabRebar)//清除预览的画线
		{
			m_slabRebar->ClearLines();
		}
		if (this->m_ClickedFlag == 0)
		{
			//DestroyWindow();
			//this->SendMessage(WM_CLOSE);
			//delete this;
		}
		this->m_ClickedFlag = 0;
	}
	else
	{
		MySlabRebarAssembly*  slabRebar = NULL;
		SlabType slabType = MySlabRebarAssembly::JudgeSlabType(ehSel);
		//switch (slabType)
		//{
		//case STSLAB:
		RebarAssembly* rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class STSlabRebarAssembly");
		slabRebar = dynamic_cast<STSlabRebarAssembly*>(rebaras);
		if (slabRebar == nullptr)
		{
			slabRebar = REA::Create<STSlabRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		}
		slabRebar->SetslabType(slabType);                  //设置墙类型
		slabRebar->SetConcreteData(g_wallRebarInfo.concrete);  //设置三个保护层信息和层数//SetConcreteData函数用引用方式把g_wallRebarInfo.concrete值传到wallRebar
		slabRebar->SetSlabRebarDir(m_PageMainRebar.m_seg, m_PageMainRebar.mArcLine);//设置板的配筋方向后相关的信息
		slabRebar->SetSlabData(eeh);                       //设置墙坐标
		slabRebar->SetRebarData(m_vecRebarData);               //设置钢筋信息
		slabRebar->SetvecLapOptions(g_vecLapOptionData);
		slabRebar->SetRebarEndTypes(m_vecEndTypeData);
		slabRebar->SetvecTwinRebarLevel(m_vecTwinBarData);
		slabRebar->InitRebarSetId();
		slabRebar->SetTieRebarInfo(m_tieRebarInfo);
		slabRebar->MakeRebars(modelRef);
		slabRebar->Save(modelRef); // must save after creating rebars
		ElementId contid = slabRebar->FetchConcrete();
		EditElementHandle eeh2(contid, ACTIVEMODEL);
		ElementRefP oldRef = eeh2.GetElementRef();
		mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
		eeh2.ReplaceInModel(oldRef);
		//eeh2.AddToModel();
		/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
		it = m_vecRebarData.begin();
		for (; it != m_vecRebarData.end(); it++)
		{
			BrString strRebarSize = it->rebarSize;
			strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
			strcpy(it->rebarSize, CT2A(strRebarSize));
		}

		itt = m_vecTwinBarData.begin();//给sizekey附加型号
		for (; itt != m_vecTwinBarData.end(); itt++)
		{
			BrString strTwinSize = itt->rebarSize;
			strTwinSize = strTwinSize.Left(strTwinSize.GetLength() - 1);	//删掉型号
			strcpy(itt->rebarSize, CT2A(strTwinSize));
		}

		BrString strTieSize = m_tieRebarInfo.rebarSize;
		strTieSize = strTieSize.Left(strTieSize.GetLength() - 1);	//删掉型号
		strcpy(m_tieRebarInfo.rebarSize, CT2A(strTieSize));
		/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
		SetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, ehSel.GetModelRef());

		ACCConcrete concrete;
		concrete.postiveOrTopCover = g_wallRebarInfo.concrete.postiveCover;
		concrete.reverseOrBottomCover = g_wallRebarInfo.concrete.reverseCover;
		concrete.sideCover = g_wallRebarInfo.concrete.sideCover;
		SetElementXAttribute(ehSel.GetElementId(), sizeof(ACCConcrete), &concrete, ConcreteCoverXAttribute, ehSel.GetModelRef());
		SetElementXAttribute(contid, sizeof(WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecLapOptionData, vecLapOptionDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecACData, vecACDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TwinBarSet::TwinBarInfo), &m_twinBarInfo, twinBarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecTwinBarData, vecTwinBarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(TieReBarInfo), &m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);

		Transform trans;
		slabRebar->GetPlacement().AssignTo(trans);
		SetElementXAttribute(contid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);

		SetElementXAttribute(contid, slabRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
		//SetElementXAttribute(contid, g_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		if (m_slabRebar)//清除预览的画线
		{
			m_slabRebar->ClearLines();
		}
		DestroyWindow();
	}

}

void CSlabRebarDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

void CSlabRebarDlg::OnBnClickedCancel()
{
	if (m_slabRebar)
	{
		m_slabRebar->ClearLines();//清除预览画线
	}
	CDialogEx::OnCancel();
	DestroyWindow();
}


void CSlabRebarDlg::OnCbnSelchangeCombo2()//直径
{
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboSize.GetCurSel());
	strcpy(m_SlabSetInfo.rebarSize, CT2A(*it));
	BrString str = *it;
	if (str != L"")
	{
		if (str.Find(L"mm") != -1)
		{
			str.Replace(L"mm", L"");
		}
	}
	strcpy(m_SlabSetInfo.rebarSize, CT2A(str));
	m_PageMainRebar.ChangeRebarSizedata(m_SlabSetInfo.rebarSize);
	m_PageMainRebar.UpdateRebarList();
}


void CSlabRebarDlg::OnCbnSelchangeCombo3()//型号
{
	auto it = g_listRebarType.begin();
	advance(it, m_ComboType.GetCurSel());
	m_SlabSetInfo.rebarType = m_ComboType.GetCurSel();
	m_PageMainRebar.ChangeRebarTypedata(m_SlabSetInfo.rebarType);
	m_PageMainRebar.UpdateRebarList();
}


void CSlabRebarDlg::OnEnChangeEdit1()//间距
{
	CString	strTemp = CString();
	m_EditSpace.GetWindowText(strTemp);
	m_SlabSetInfo.spacing = atof(CT2A(strTemp));

	m_PageMainRebar.ChangeRebarSpacedata(m_SlabSetInfo.spacing);
	m_PageMainRebar.UpdateRebarList();
}



void CSlabRebarDlg::PreviewRebarLines()
{
	SlabPreviewButtonDown = true;
	if (!ehSel.IsValid()) return;

	if (m_slabRebar)
	{
		m_slabRebar->ClearLines();
	}
	m_PageMainRebar.getWallSetInfo(m_SlabSetInfo);
	m_PageMainRebar.GetConcreteData(g_wallRebarInfo.concrete);
	//m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);

	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = ehSel.GetModelRef();
	tmpModel = tmpModel;
	EditElementHandle eeh(ehSel, ehSel.GetModelRef());


	ElementId testid = 0;
	GetElementXAttribute(ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
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

	auto itt = m_vecTwinBarData.begin();//给sizekey附加型号
	for (; itt != m_vecTwinBarData.end(); itt++)
	{
		GetDiameterAddType(itt->rebarSize, itt->rebarType);
	}

	GetDiameterAddType(m_tieRebarInfo.rebarSize, m_tieRebarInfo.rebarType);
	/***********************************给sizekey附加型号******************************************************/
	SlabType slabType = MySlabRebarAssembly::JudgeSlabType(ehSel);
	RebarAssembly * rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "Gallery::LDSlabRebarAssembly");
	m_slabRebar = dynamic_cast<LDSlabRebarAssembly*>(rebaras);
	if (m_slabRebar == nullptr)
	{
		m_slabRebar = REA::Create<LDSlabRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	}


	m_slabRebar->SetslabType(slabType);                  //设置墙类型
	m_slabRebar->SetSlabData(eeh);                       //设置墙坐标
	m_slabRebar->SetConcreteData(g_wallRebarInfo.concrete);  //设置三个保护层信息和层数//SetConcreteData函数用引用方式把g_wallRebarInfo.concrete值传到wallRebar
	m_slabRebar->SetRebarData(m_vecRebarData);               //设置钢筋信息
	m_slabRebar->SetvecLapOptions(g_vecLapOptionData);
	m_slabRebar->SetRebarEndTypes(m_vecEndTypeData);
	m_slabRebar->SetvecTwinRebarLevel(m_vecTwinBarData);
	m_slabRebar->InitRebarSetId();
	m_slabRebar->SetTieRebarInfo(m_tieRebarInfo);
	//slabRebar->SetbACCRebar(true);
	//slabRebar->SetACCRebarMethod(1);
	//slabRebar->SetvecTwinRebarLevel(g_vecTwinBarData);
	//slabRebar->SetTwinbarInfo(g_twinBarInfo);
	m_slabRebar->MakeRebars(modelRef);
	m_slabRebar->Save(modelRef); // must save after creating rebars
	ElementId contid = m_slabRebar->FetchConcrete();
	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);
	//eeh2.AddToModel();

	SlabPreviewButtonDown = false;

	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	it = m_vecRebarData.begin();
	for (; it != m_vecRebarData.end(); it++)
	{
		BrString strRebarSize = it->rebarSize;
		strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
		strcpy(it->rebarSize, CT2A(strRebarSize));
	}

	itt = m_vecTwinBarData.begin();//给sizekey附加型号
	for (; itt != m_vecTwinBarData.end(); itt++)
	{
		BrString strTwinSize = itt->rebarSize;
		strTwinSize = strTwinSize.Left(strTwinSize.GetLength() - 1);	//删掉型号
		strcpy(itt->rebarSize, CT2A(strTwinSize));
	}

	BrString strTieSize = m_tieRebarInfo.rebarSize;
	strTieSize = strTieSize.Left(strTieSize.GetLength() - 1);	//删掉型号
	strcpy(m_tieRebarInfo.rebarSize, CT2A(strTieSize));
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/

}


void CSlabRebarDlg::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	repeate = m_repeat.GetCheck();
}

void CSlabRebarDlg::OnBnClickedSelectModel()
{
	// TODO: 在此添加控件通知处理程序代码

	m_ClickedFlag = 0;
	auto select_wall_tool = new PickElementTool(
		// on_filter
		_local::isslab,
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

				ElementId contid = 0;
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
				m_PageMainRebar.SavePtr(this);

				repeate = 1;
				this->on_Clicke_maker();
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


//void CSlabRebarDlg::OnBnClickedRebartemplate()
//{ 
//	// TODO: 在此添加控件通知处理程序代码
//	this->m_dlgData = m_PageMainRebar.m_dlgData;//获取保护层等数据
//	m_RebarTemplate.Set_m_dlgData(m_dlgData);//保存保护层等数据
//
//	this->m_PageMainRebar.GetAllRebarData(m_vecRebarData);//获取列表里面的值等数据
//	m_RebarTemplate.Set_m_vecRebarData(m_vecRebarData);//保护列表里面的值等数据
//
//	AFX_MANAGE_STATE(AfxGetStaticModuleState());
//	m_RebarTemplate.Create(IDD_DIALOG_RebarTemplate);
//	m_RebarTemplate.SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);//使窗口总是在最前面
//	m_RebarTemplate.ShowWindow(SW_SHOW);
//}


void CSlabRebarDlg::OnBnClickedSelectModel2()
{
	// TODO: 在此添加控件通知处理程序代码
	/*ElementAgenda agenda;
	if (!GetSelectAgenda(agenda, L"请选择廊道中的墙和板"))
	{
		return;
	}
	if(agenda.size()<1)
		return;
	g_SelectedElm = agenda[0];
	EditElementHandle eehtemp(g_SelectedElm, g_SelectedElm.GetModelRef());
	
	this->SetSelectElement(g_SelectedElm);
	this->ArcRebarMethod(eehtemp);

	PreviewRebarLines();*/


	auto select_wall_tool = new PickElementTool(
		// on_filter
		_local::isslab,
		// on_complete
		[this](const ElementAgenda &agenda)
		{
			for (auto &entry : agenda)
			{
				
				ElementHandle tmpeeh(entry.GetElementId(), entry.GetModelRef());
				this->SetSelectElement(tmpeeh);
				g_SelectedElm = tmpeeh;
				this->SetSelectElement(g_SelectedElm);
				//this->ArcRebarMethod(eehtemp);
			
				this->PreviewRebarLines();

				/*repeate = 1;
				this->on_Clicke_maker();*/
			}
			// 设置当前窗口焦点
			this->SetFocus();
		},
		// on_cancel
			[this](void)
		{
			if (m_slabRebar)
			{
				m_slabRebar->ClearLines();
			}
		});

	select_wall_tool->InstallTool();
}
