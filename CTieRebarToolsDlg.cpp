// CTieRebarToolsDlg.cpp: 实现文件
//
#include "_USTATION.h"
#include "resource.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "ElementAttribute.h"
#include "GalleryIntelligentRebar.h"
#include "CTieRebarToolsDlg.h"
#include "afxdialogex.h"
#include "PITMSCECommon.h"
#include "TieRebar.h"
#include "ReadRebarTool.h"

// CTieRebarToolsDlg 对话框

IMPLEMENT_DYNAMIC(CTieRebarToolsDlg, CDialogEx)

CTieRebarToolsDlg::CTieRebarToolsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_TieRebarTools, pParent)
{
	m_tieRebarInfo.tieRebarMethod = 0;
	m_tieRebarInfo.rebarType = 0;
	strcpy(m_tieRebarInfo.rebarSize, CT2A(_T("12mm")));

	m_pTieRebarAssembly = NULL;
}

CTieRebarToolsDlg::~CTieRebarToolsDlg()
{
}

// 按中心点z值坐标给钢筋排序
void CTieRebarToolsDlg::SorSelcetRebar()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (m_selectrebars.size() > 0)
	{
		for (ElementRefP ref : m_selectrebars)
		{
			EditElementHandle eeh(ref, ref->GetDgnModelP());
			DPoint3d center = getCenterOfElmdescr(eeh.GetElementDescrP());
			int posz = (int)(center.z / (uor_per_mm * 10) + 0.5);
			m_mapselectrebars[posz].push_back(ref);
		}
	}
}

// 通过钢筋取当前实体的ElementHandle
void CTieRebarToolsDlg::InitRebarSetsAndehSel()
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


BOOL CTieRebarToolsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	InitRebarSetsAndehSel();

	ElementId contid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	if (contid > 0)
	{
		GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		GetElementXAttribute(contid, sizeof(TieReBarInfo), m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);
	}
	if (m_vecRebarData.size() > 0)
	{
		m_TableTieDlg.m_vecRebarData = m_vecRebarData;
	}

	m_TableTieDlg.SetDisplayCheck(true);
	m_TableTieDlg.SetContin(false);
	m_TableTieDlg.SetTieRebarData(m_tieRebarInfo);
	m_TableTieDlg.Create(IDD_DIALOG_WallRebar_TieRebarSet, &m_tab);

	CRect rc;
	m_tab.GetClientRect(rc);
	rc.top += 20;
	rc.bottom -= 0;
	rc.left += 0;
	rc.right -= 0;
	m_TableTieDlg.MoveWindow(&rc);

	m_tab.InsertItem(0, _T("拉筋配置"));
	p_Dialog[0] = &m_TableTieDlg;
	// 显示初始页面
	p_Dialog[0]->ShowWindow(SW_SHOW);

	m_CurSelTab = 0;

	// SorSelcetRebar();
	return true;
}

void CTieRebarToolsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_TAB1, m_tab);
}


BEGIN_MESSAGE_MAP(CTieRebarToolsDlg, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CTieRebarToolsDlg::OnTcnSelchangeTab1)
	ON_BN_CLICKED(IDOK, &CTieRebarToolsDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CTieRebarToolsDlg 消息处理程序


void CTieRebarToolsDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CTieRebarToolsDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	DgnModelRefP        modelRef = ACTIVEMODEL;

	m_TableTieDlg.GetTieRebarData(m_tieRebarInfo);
	int	tieRebarStyle = m_tieRebarInfo.tieRebarStyle;

	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());
	if (m_pTieRebarAssembly == NULL)
	{
		m_pTieRebarAssembly = REA::Create<CTieRebarToolAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
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
	// 是否连续配筋
	m_pTieRebarAssembly->SetisContinRebar(m_TableTieDlg.GetContin());
	// end
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
	// SetElementXAttribute(contid, sizeof(TieReBarInfo), &m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);
}
