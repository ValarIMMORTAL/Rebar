// TieRebarFaceDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "resource.h"
#include "GalleryIntelligentRebar.h"
#include "TieRebarFaceDlg.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"


// TieRebarFaceDlg 对话框

IMPLEMENT_DYNAMIC(TieRebarFaceDlg, CDialogEx)

TieRebarFaceDlg::TieRebarFaceDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_TieRebarFace1, pParent)
{
	m_tieRebarInfo.tieRebarMethod = 0;
	m_tieRebarInfo.rebarType = 0;
	strcpy(m_tieRebarInfo.rebarSize, CT2A(_T("12mm")));
	m_pTieRebarAssembly = NULL;
}

TieRebarFaceDlg::~TieRebarFaceDlg()
{
}

BOOL TieRebarFaceDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// TODO:  在此添加额外的初始化
	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	m_uFaceId = 0;

	//for each (auto var in g_listTieRebarStyle)
	m_tieRebarStyle.AddString(L"对扣拉筋");
	m_tieRebarStyle.AddString(L"单根对扣");
	m_tieRebarStyle.SetCurSel(0);
	m_angle1.SetWindowTextW(L"135");
	m_angle2.SetWindowTextW(L"100");

	for each (auto var in g_listRebarType)
		m_tieRebarType.AddString(var);
	m_tieRebarType.SetCurSel(0);
	for each (auto var in g_listRebarSize)
		m_TieRebarDiameter.AddString(var);
	m_TieRebarDiameter.SetCurSel(2);

	((CButton *)GetDlgItem(IDC_RADIO1))->SetCheck(1);

	InitRebarSetsAndehSel();

	ElementId contid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	if (contid > 0)
	{
		GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		GetElementXAttribute(contid, sizeof(TieReBarInfo), m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);
	}

	return TRUE;
}

void TieRebarFaceDlg::SetFaceId(ElementId faceid)
{
	m_uFaceId = faceid;
}

void TieRebarFaceDlg::InitRebarSetsAndehSel()
{
	RebarSet * rebset = nullptr;
	EditElementHandle start(m_selectrebars[0], m_selectrebars[0]->GetDgnModelP());
	if (RebarElement::IsRebarElement(start))
	{
		RebarElementP rep = RebarElement::Fetch(start);
		rebset = rep->GetRebarSet(ACTIVEMODEL);

		ElementId conid;
		int rebar_cage_type;
		conid = rebset->GetConcreteOwner(ACTIVEMODEL, rebar_cage_type);
		RebarAssemblies reas;
		RebarAssembly::GetRebarAssemblies(conid, reas);
		m_pRebarAssembly = nullptr;
		for (int i = 0; i < reas.GetSize(); i++)
		{
			RebarAssembly* rebaras = reas.GetAt(i);
			if (rebaras->GetCallerId() == rebset->GetCallerId())
			{
				m_pRebarAssembly = rebaras;
			}
		}

		if (m_pRebarAssembly != nullptr)
		{
			DgnModelRefP        modelRef = ACTIVEMODEL;
			ElementId tmpid = m_pRebarAssembly->GetSelectedElement();
			if (tmpid == 0)
			{
				return;
			}
			DgnModelRefP modelp = m_pRebarAssembly->GetSelectedModel();
			EditElementHandle			ehSel;

			if (modelp == nullptr)
			{
				if (ehSel.FindByID(tmpid, ACTIVEMODEL) != SUCCESS)
				{
					ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
					for (DgnModelRefP modelRef : modelRefCol)
					{
						if (ehSel.FindByID(tmpid, modelRef) == SUCCESS)
						{
							modelp = modelRef;
							break;
						}
					}
				}
			}
			else
			{
				ehSel.FindByID(tmpid, modelp);
			}

			m_ehSel = ehSel;
		}
	}
}

void TieRebarFaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_tieRebarStyle);
	DDX_Control(pDX, IDC_COMBO2, m_tieRebarType);
	DDX_Control(pDX, IDC_COMBO3, m_TieRebarDiameter);
	//  DDX_Control(pDX, IDC_EDIT1, m_ange1);
	//  DDX_Control(pDX, IDC_EDIT19, m_angel2);
	DDX_Control(pDX, IDC_EDIT1, m_angle1);
	DDX_Control(pDX, IDC_EDIT18, m_angle2);
}


BEGIN_MESSAGE_MAP(TieRebarFaceDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &TieRebarFaceDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDOK, &TieRebarFaceDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &TieRebarFaceDlg::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO1, &TieRebarFaceDlg::OnSelchangeCombo1)
END_MESSAGE_MAP()


// TieRebarFaceDlg 消息处理程序


void TieRebarFaceDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	CreateFaceTool::InstallNewInstance(CMDNAME_TieRebarFace, this);
}


void TieRebarFaceDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
		//wchar_t ch[256] = { 0 };
		//swprintf(ch,L"%d",m_uFaceId);
		//MessageBox(ch, L"11", MB_OK);
	if (m_uFaceId == 0)
	{
		MessageBox(L"请先绘制拉筋所在的面", L"提示", MB_OK);
		return;
	}
	CDialogEx::OnOK();
	DgnModelRefP        modelRef = ACTIVEMODEL;

	int rebarPlaceStyle = 0;

	//m_TableTieDlg.GetTieRebarData(m_tieRebarInfo);
	m_tieRebarInfo.tieRebarMethod = m_tieRebarStyle.GetCurSel();

	if (((CButton *)GetDlgItem(IDC_RADIO1))->GetCheck() == 1)
	{
		m_tieRebarInfo.tieRebarStyle = 0;
	}
	else
	{
		m_tieRebarInfo.tieRebarStyle = 1;
	}

	CString str;
	m_TieRebarDiameter.GetWindowText(str);
	char size[1024];
	wsprintfA(size, "%ls", str);
	strcpy(m_tieRebarInfo.rebarSize, size);
	m_tieRebarInfo.rebarType = m_tieRebarType.GetCurSel();
	//m_tieRebarInfo.tieRebarStyle = 0;
	//m_tieRebarInfo.isPatch = false;
	m_tieRebarInfo.rowInterval = 0;
	m_tieRebarInfo.colInterval = 0;
	//int	tieRebarStyle = m_tieRebarInfo.tieRebarStyle;

	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());
	if (m_pTieRebarAssembly == NULL)
	{
		m_pTieRebarAssembly = REA::Create<CFaceTieRebarToolAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	}

	ElementId contid = 0;
	Transform trans;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	GetElementXAttribute(contid, sizeof(Transform), trans, UcsMatrixXAttribute, ACTIVEMODEL);

	if ((int)m_vecRebarData.size() > 3)
	{
		m_pTieRebarAssembly->SetposSpacing1(m_vecRebarData[0].spacing);
		m_pTieRebarAssembly->SetposSpacing2(m_vecRebarData[3].spacing);

		m_pTieRebarAssembly->SetrevSpacing1(m_vecRebarData[1].spacing);
		m_pTieRebarAssembly->SetrevSpacing2(m_vecRebarData[2].spacing);

	}
	m_pTieRebarAssembly->InitReabrData();
	m_pTieRebarAssembly->Settrans(trans);
	m_pTieRebarAssembly->SetCover(500.0);
	m_pTieRebarAssembly->SetFaceId(m_uFaceId);

	CString strAngle1;
	m_angle1.GetWindowText(strAngle1);
	CString strAngle2;
	m_angle2.GetWindowText(strAngle2);
	double dAngle1 = _ttof(strAngle1);
	double dAngle2 = _ttof(strAngle2);
	m_pTieRebarAssembly->SetEndAngle(dAngle1, dAngle2);

	m_pTieRebarAssembly->AnalyzingWallGeometricData(eeh);
	m_pTieRebarAssembly->GetEleNameAndType(eeh);
	// m_pTieRebarAssembly->SetCallerId(m_pRebarAssembly->GetCallerId());
	m_pTieRebarAssembly->SetTieRebarInfo(m_tieRebarInfo);
	m_pTieRebarAssembly->CalculateSelectRebarInfo(m_selectrebars, modelRef);
	m_pTieRebarAssembly->SortAllVecRebar();
	m_pTieRebarAssembly->TraveAllRebar(m_pRebarAssembly, modelRef);
	m_pTieRebarAssembly->SortAllVecRebar();
	m_pTieRebarAssembly->MakeRebars(modelRef);
	m_pTieRebarAssembly->Save(modelRef);
	contid = m_pTieRebarAssembly->GetConcreteOwner();
	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);
	CDialogEx::OnOK();
}


void TieRebarFaceDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}


void TieRebarFaceDlg::OnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	int selectsel = m_tieRebarStyle.GetCurSel();
	switch (selectsel)
	{
	case 0:
	{
		m_angle1.SetWindowTextW(L"135");
		m_angle2.SetWindowTextW(L"100");
		break;
	}
	case 1:
	{
		m_angle1.SetWindowTextW(L"180");
		m_angle2.SetWindowTextW(L"180");
		break;
	}
	default:
		break;
	}
}

void CreateFaceTool::_OnPostInstall()
{
	AccuSnap::GetInstance().EnableSnap(true); // Enable snapping for create tools.

	__super::_OnPostInstall();
}

void CreateFaceTool::_OnRestartTool()
{
	CreateFaceTool::InstallNewInstance(__super::GetToolId(), m_pTie);
}

bool CreateFaceTool::_OnDataButton(DgnButtonEventCR ev)
{
	if (0 == m_points.size())
	{
		_BeginDynamics();
		//EnableUndoPreviousStep(); 
	}

	m_points.push_back(*ev.GetPoint());
	//CreateAcceptedSegmentsTransient();

	if (2 < m_points.size())
	{
		Dpoint3d ptSart = *(m_points.begin());
		if (ptSart.AlmostEqual(*ev.GetPoint()))
		{
			if (m_eeh.IsValid())
			{
				m_eeh.DeleteFromModel();
			}

			DPoint3d *shapePts = new DPoint3d[m_points.size()];
			if (!m_points.empty())
			{
				memcpy(shapePts, &m_points[0], m_points.size() * sizeof(DPoint3d));
			}
			ShapeHandler::CreateShapeElement(m_eeh, NULL, shapePts, m_points.size(), ACTIVEMODEL->Is3d(), *ACTIVEMODEL);
			ElementPropertyUtils::ApplyActiveSettings(m_eeh);
			m_eeh.AddToModel();
			m_pTie->SetFaceId(m_eeh.GetElementId());
			__super::_ExitTool();
			return true;
		}
	}


	if (m_eeh.IsValid())
	{
		m_eeh.DeleteFromModel();
	}

	CreateElement(m_eeh, m_points);
	m_eeh.AddToModel();
	return false;
}

bool CreateFaceTool::_OnResetButton(DgnButtonEventCR ev)
{
	if (m_eeh.IsValid())
	{
		m_eeh.DeleteFromModel();
	}
	_OnReinitialize();
	return true;
}

void CreateFaceTool::_OnDynamicFrame(DgnButtonEventCR ev)
{
	if (m_points.size() < 1)
		return;

	bvector<DPoint3d>   tmpPts;
	EditElementHandle   eeh;

	tmpPts.push_back(m_points.back()); // Only draw the current segment in dynamics, the accepted segments drawn as transients.
	tmpPts.push_back(*ev.GetPoint());

	if (!CreateElement(eeh, tmpPts))
		return;

	RedrawElems redrawElems;

	redrawElems.SetDynamicsViews(IViewManager::GetActiveViewSet(), ev.GetViewport()); // Display in all views, draws to cursor view first...
	redrawElems.SetDrawMode(DRAW_MODE_TempDraw);
	redrawElems.SetDrawPurpose(DrawPurpose::Dynamics);

	redrawElems.DoRedraw(eeh);
}

bool CreateFaceTool::CreateElement(EditElementHandleR eeh, bvector<DPoint3d> const & points)
{
	if (points.size() < 2)
		return false;

	// NOTE: Create a line, linestring, or complex string based on point count.
	if (SUCCESS != DraftingElementSchema::ToElement(eeh, *ICurvePrimitive::CreateLineString(points), NULL, ACTIVEMODEL->Is3d(), *ACTIVEMODEL))
		return false;

	ElementPropertyUtils::ApplyActiveSettings(eeh);

	return true;
}

void CreateFaceTool::InstallNewInstance(int toolId, TieRebarFaceDlg * pTie)
{
	CreateFaceTool* tool = new CreateFaceTool(toolId, pTie);
	tool->InstallTool();
}
