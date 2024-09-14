// CParapetDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CParapetDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "WallRebarAssembly.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "CommonFile.h"
#include "ElementAttribute.h"
#include "ACCRebarMaker.h"
#include "ConstantsDef.h"
#include "ParapetRebarAssembly.h"

// CParapetDlg 对话框

IMPLEMENT_DYNAMIC(CParapetDlg, CDialogEx)

CParapetDlg::CParapetDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_Parapet, pParent)
{
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
}

CParapetDlg::~CParapetDlg()
{
}

void CParapetDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

void CParapetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_STATIC_PMaindlg, m_static_maindlg);
	DDX_Control(pDX, IDC_EDIT_PUlenth, m_edit_ulenth);
}
BOOL CParapetDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
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

	CString strValue;
	strValue.Format(_T("%f"), g_wallRebarInfo.lapoption.udLength);
	m_edit_ulenth.SetWindowTextW(strValue);
	CString strRebarSize(m_WallSetInfo.rebarSize);
	if (strRebarSize.Find(L"mm") == -1)
		strRebarSize += "mm";
	CString strSpace;
	strSpace.Format(L"%.2f", m_WallSetInfo.spacing);//保护层

	g_wallRebarInfo.concrete.rebarLevelNum = 4;
	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	
	CRect IFramerect;
	CStatic* pStatic;
	pStatic = (CStatic*)GetDlgItem(IDC_STATIC_PMaindlg);
	m_PageMainRebar.Create(IDD_DIALOG_WallRebar_MainRebar, pStatic);
	m_PageMainRebar.GetWindowRect(IFramerect);
	m_PageMainRebar.SetWindowPos(&wndTop, 0, 0, IFramerect.Width(), IFramerect.Height(), SWP_SHOWWINDOW);
	m_PageMainRebar.ShowWindow(SW_SHOW);
	m_PageMainRebar.GetDlgItem(IDC_BUTTON1)->ShowWindow(FALSE);
	m_PageMainRebar.GetDlgItem(IDC_EDIT4)->EnableWindow(FALSE);
	
	m_PageMainRebar.SetSelectElement(ehSel);
	m_PageMainRebar.GetListRowData(m_vecRebarData);
	{
		m_vecRebarData.at(0).rebarDir = 1;
		m_vecRebarData.at(1).rebarDir = 0;
		m_vecRebarData.at(2).rebarDir = 0;
		m_vecRebarData.at(3).rebarDir = 1;
	}
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	m_PageMainRebar.UpdateRebarList();


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

BEGIN_MESSAGE_MAP(CParapetDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CParapetDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CParapetDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CParapetDlg 消息处理程序
void CParapetDlg::SetDefaultData()
{
	for (int i = 0; i < 4 * 2; i++)
	{
		PIT::EndType::RebarEndPointInfo endPtInfo;
		memset(&endPtInfo, 0, sizeof(endPtInfo));
		PIT::EndType oneEndTypeData = { 0,0,0 ,endPtInfo };
		m_vecEndTypeData.push_back(oneEndTypeData);
	}
	for (int i = 0; i < m_vecRebarData.size(); i++)
	{
		CString strTwinBar = L"";
		TwinBarSet::TwinBarLevelInfo oneTwinBarData;
		strcpy(oneTwinBarData.levelName, CT2A(strTwinBar.GetBuffer()));
		oneTwinBarData.hasTwinbars = 0;
		strcpy(oneTwinBarData.rebarSize, m_vecRebarData[i].rebarSize);
		oneTwinBarData.rebarType = m_vecRebarData[i].rebarType;
		oneTwinBarData.interval = 0;
		m_vecTwinBarData.push_back(oneTwinBarData);
	}
	memset(&m_tieRebarInfo, 0, sizeof(TieReBarInfo));
	memset(&m_twinBarInfo, 0, sizeof(TwinBarSet::TwinBarInfo));
}

void CParapetDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = ehSel.GetModelRef();
	EditElementHandle eeh(ehSel, ehSel.GetModelRef());

	m_PageMainRebar.getWallSetInfo(m_WallSetInfo);
	m_PageMainRebar.GetConcreteData(g_wallRebarInfo.concrete);
	m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	SetDefaultData();
	CString strulenth;
	m_edit_ulenth.GetWindowTextW(strulenth);
	double ulenth = atof(CT2A(strulenth));
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
		WallRebarAssembly::IsSmartSmartFeature(eeh);
		ParapetRebarAssembly*  wallRebar = NULL;
		ElementId testid = 0;
		GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());

		WallRebarAssembly::WallType wallType = WallRebarAssembly::JudgeWallType(ehSel);
		switch (wallType)
		{
		case WallRebarAssembly::STWALL:
		{
			RebarAssembly* rebaras = ACCRebarAssembly::GetRebarAssembly(testid, "class ParapetRebarAssembly");
			wallRebar = dynamic_cast<ParapetRebarAssembly*>(rebaras);
			if (wallRebar == nullptr)
			{
				wallRebar = REA::Create<ParapetRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			}
		}
		break;
		default:
			return;
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
		wallRebar->InitRebarParam(ulenth);
		wallRebar->MakeRebars(modelRef);
		wallRebar->Save(modelRef); // must save after creating rebars
		ElementId contid = wallRebar->FetchConcrete();
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

		g_wallRebarInfo.lapoption.udLength = ulenth;
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
		DestroyWindow();
}


void CParapetDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	DestroyWindow();
}
