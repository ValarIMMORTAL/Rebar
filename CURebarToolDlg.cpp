// CURebarToolDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CURebarToolDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include <RebarCatalog.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "URebar.h"
#include "ReadRebarTool.h"
#include "SingleRebarAssembly.h"
#include "ExtractFacesTool.h"
#include "XmlHelper.h"
#include "CommonFile.h"
// CURebarToolDlg 对话框

extern CURebarToolDlg *pURebarDlg;

IMPLEMENT_DYNAMIC(CURebarToolDlg, CDialogEx)

CURebarToolDlg::CURebarToolDlg(const vector<ElementId> &vecElm, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_URebarTool, pParent),m_vecElm(vecElm)
{

}

CURebarToolDlg::~CURebarToolDlg()
{
	for (auto id : m_refLineIds)
	{
		EditElementHandle eh(id, ACTIVEMODEL);
		eh.DeleteFromModel();
	}
	for (int i = 0; i < m_Holeehs.size(); i++)
	{
		if (m_Holeehs.at(i) != nullptr)
		{
			delete m_Holeehs.at(i);
			m_Holeehs.at(i) = nullptr;
		}
	}
}

void CURebarToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_CombRebarSize);
	DDX_Control(pDX, IDC_COMBO12, m_CombRebarType);
	DDX_Control(pDX, IDC_CHECK1, m_CheckNeg);
	DDX_Control(pDX, IDC_CHECK5, m_CheckUp);
	DDX_Control(pDX, IDC_CHECK6, m_CheckLength);
	DDX_Control(pDX, IDC_CHECK_INNER, m_CheckInner);
}


BEGIN_MESSAGE_MAP(CURebarToolDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CURebarToolDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON3, &CURebarToolDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_CHECK1, &CURebarToolDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_CHECK5, &CURebarToolDlg::OnBnClickedCheck5)
	ON_EN_CHANGE(IDC_EDIT13, &CURebarToolDlg::OnEnChangeEdit13)
	ON_BN_CLICKED(IDC_BUTTON2, &CURebarToolDlg::OnBnClickedButton2)
	ON_EN_CHANGE(IDC_EDIT15, &CURebarToolDlg::OnEnChangeEdit15)
	ON_EN_CHANGE(IDC_EDIT16, &CURebarToolDlg::OnEnChangeEdit16)
	ON_BN_CLICKED(IDC_CHECK6, &CURebarToolDlg::OnBnClickedCheck6)
	
	ON_CBN_SELCHANGE(IDC_COMBO1, &CURebarToolDlg::OnCbnSelchangeCombo1)
	ON_CBN_KILLFOCUS(IDC_COMBO1, &CURebarToolDlg::OnCbnKillfocusCombo1)
	ON_BN_CLICKED(IDC_CHECK_INNER, &CURebarToolDlg::OnBnClickedCheckInner)
END_MESSAGE_MAP()


// CURebarToolDlg 消息处理程序

BOOL CURebarToolDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	// TODO:  在此添加额外的初始化
	m_CombRebarSize.ResetContent();
	m_CombRebarType.ResetContent();
	mdlOutput_prompt(L"OnInitDialogstr！");
	for each (auto var in g_listRebarType)
		m_CombRebarType.AddString(var);

	for each (auto var in g_listRebarSize)
		m_CombRebarSize.AddString(var);
	m_CombRebarSize.SetCurSel(0);
	if (m_dlgData.rebarData.rebarSize.Find(L"mm") != -1)
		m_dlgData.rebarData.rebarSize.Replace(L"mm",L"");
	CString cstrSize = m_dlgData.rebarData.rebarSize + L"mm";
	CString cstrType = m_dlgData.rebarData.rebarType;
	double length;
	std::string stRebarsize(m_dlgData.rebarData.rebarSize);
	std::string sttype(m_dlgData.rebarData.rebarType);
	if (sttype == "1") { sttype = "A"; }
	else if (sttype == "2") { sttype = "B"; }
	else if (sttype == "3") { sttype = "C"; }
	else { sttype = "D"; }
	stRebarsize = stRebarsize + sttype;
	length = g_globalpara.m_laplenth[stRebarsize];
	//GetDlgItem(IDC_EDIT15)->SetWindowText(L"600");
	char a[10];
	itoa(length, a, 10);
	CString LOleng(a);
	GetDlgItem(IDC_EDIT15)->SetWindowText(LOleng);
	GetDlgItem(IDC_EDIT16)->SetWindowText(LOleng);

	GetDlgItem(IDC_EDIT1)->SetWindowText(TEXT_OTHER_REBAR);
	GetDlgItem(IDC_EDIT13)->SetWindowText(L"0");

	auto find = std::find(g_listRebarSize.begin(), g_listRebarSize.end(), cstrSize);
	m_CombRebarSize.SetCurSel((int)std::distance(g_listRebarSize.begin(), find));
	CString gradetype = L"HRB400";
	if (cstrType == L"1") { gradetype = L"HPB300"; }
	else if (cstrType == L"2") { gradetype = L"HPB335"; }
	else if (cstrType == L"3") { gradetype = L"HRB400"; }
	else { gradetype = L"HRB500"; }
	auto find1 = std::find(g_listRebarType.begin(), g_listRebarType.end(), gradetype);
	m_CombRebarType.SetCurSel((int)std::distance(g_listRebarType.begin(), find1));
	mdlOutput_prompt(L"OnInitDialogend！");
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CURebarToolDlg::CalcWallHoles()
{
	if ((int)mSelsectedRebar.size() == 0)
	{
		return;
	}
	ElementHandle eehRebar(mSelsectedRebar[0], ACTIVEMODEL);
	EditElementHandle Editeeh(eehRebar, ACTIVEMODEL);
	string rebarType;
	GetRebarLevelItemTypeValue(Editeeh, rebarType);
	RebarElementP repTmp = RebarElement::Fetch(eehRebar);
	RebarModel *rmv = RMV;
	BeConcreteData condata;
	int rebar_cage_type;
	if (rmv != nullptr)
	{
		rmv->GetConcreteData(*repTmp, repTmp->GetModelRef(), condata, rebar_cage_type);
	}

	RebarSetP rebset = nullptr;
	rebset = repTmp->GetRebarSet(repTmp->GetModelRef());
	RebarShape * rebarshapeTmp = repTmp->GetRebarShape(mSelsectedRebar[0]->GetDgnModelP());
	BrString Sizekey(rebarshapeTmp->GetShapeData().GetSizeKey());

	if (rebset != nullptr)
	{
		RebarSetP rootrebset = nullptr;
		rootrebset = rebset->GetParentRebarSet(repTmp->GetModelRef());
		rootrebset = rootrebset;
		RebarSets lapped_rebar_sets;
		rebset->GenerateLappedRebarSets(lapped_rebar_sets, repTmp->GetModelRef());
	}

	ElementId conid = condata.GetRexId().GetElementId();

	RebarAssemblies area;
	REA::GetRebarAssemblies(conid, area);

	RebarAssembly* rebarasb = nullptr;
	for (int i = 0; i < area.GetSize(); i++)
	{
		RebarAssembly* rebaras = area.GetAt(i);
		if (rebaras->GetCallerId() == rebset->GetCallerId())
		{
			rebarasb = rebaras;
		}
	}

	EditElementHandle ehSel; //墙
	PIT::GetAssemblySelectElement(ehSel, rebarasb);

	EditElementHandle Eleeh;
	EFT::GetSolidElementAndSolidHoles(ehSel, Eleeh, m_Holeehs);
	Transform trans;
	GetElementXAttribute(conid, sizeof(Transform), trans, UcsMatrixXAttribute, ACTIVEMODEL);
	PIT::WallRebarInfo tmpinfo;
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (SUCCESS == GetElementXAttribute(conid, sizeof(PIT::WallRebarInfo), tmpinfo, WallRebarInfoXAttribute, ACTIVEMODEL))
	{
		for (int i = 0;i<m_Holeehs.size();i++)
		{
			ElementCopyContext copier(ACTIVEMODEL);
			copier.SetSourceModelRef(m_Holeehs.at(i)->GetModelRef());
			copier.SetTransformToDestination(true);
			copier.SetWriteElements(false);
			copier.DoCopy(*m_Holeehs.at(i));
			PlusSideCover(*m_Holeehs.at(i), tmpinfo.concrete.sideCover*uor_per_mm, trans, false);
		}
	}

}

void CURebarToolDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	DgnModelRefP modelRef = ACTIVEMODEL;

	if (m_dlgData.interval <= 0)
	{
		m_dlgData.interval = 1;
	}

	RebarSetTag *tag = NULL;
	ElementId conid = 0;
	multimap<RebarSetP, RebarElementP> mlt_Rebar;
	for (size_t i = 0; i < m_vecElm.size(); i += m_dlgData.interval)
	{
		ElementHandle eh(m_vecElm[i], modelRef);
		if (RebarElement::IsRebarElement(eh))
		{
			RebarElementP rep = RebarElement::Fetch(eh);
			RebarSetP pRebarSet = rep->GetRebarSet(modelRef);
			if (pRebarSet != nullptr)
			{
				mlt_Rebar.insert(make_pair(pRebarSet, rep));
				if (conid == 0)
				{
					int rebar_cage_type;
					conid = pRebarSet->GetConcreteOwner(ACTIVEMODEL, rebar_cage_type);
				}
			}
		}
	}

	if (mlt_Rebar.empty())
	{
		return;
	}

	multimap<RebarSetP, vector<ElementId> > mut_TwinRebar;
	ElementId preId = 0;
	RebarSetP pRebarSet = nullptr;
	vector<ElementId> vecRebarId;
	int flag = 0;
	for (auto it = mlt_Rebar.begin(); it != mlt_Rebar.end(); ++it)
	{
		vecRebarId.push_back(it->second->GetElementId());
		preId = (it->first)->GetElementId();
		ElementId curId = preId;
		if (curId != preId || flag == mlt_Rebar.size() - 1)
		{
			preId = curId;
			ElementId rebarSetId = 0;
			pRebarSet = RebarSet::Fetch(rebarSetId, modelRef);
			mut_TwinRebar.insert(make_pair(pRebarSet, vecRebarId));	//存储所需并筋set与对于钢筋数量
			vecRebarId.clear();
		}
		flag++;
	}

	//添加U形钢筋
	UpdateRebarData();

	int nType = atoi(CT2A(m_dlgData.rebarData.rebarType));
	GetDiameterAddType(m_dlgData.rebarData.rebarSize, nType);

	RebarSetTagArray rsetTags;
	int nTag = 0;
	for (auto it = mut_TwinRebar.begin(); it != mut_TwinRebar.end(); ++it)
	{
		RebarSetTag* tag = nullptr;
		RebarSetP rebarset = it->first;
		if (m_isLRebar && it->second.size() > 1)
		{
			EditElementHandle firstEeh(it->second.at(0), modelRef);
			DPoint3d strPt, endPt;
			double dim;
			GetStartEndPointFromRebar(&firstEeh, strPt, endPt, dim);
			EditElementHandle secondEeh(it->second.at(1), modelRef);
			DPoint3d strPt1, endPt1;
			GetStartEndPointFromRebar(&secondEeh, strPt1, endPt1, dim);
			DVec3d faceVec = strPt - strPt1;
			faceVec.Normalize();
			PIT::URebarMaker uRebar(it->second, m_dlgData.rebarData, m_Holeehs, faceVec,
				m_dlgData.bNegtive, m_dlgData.bUp, m_isEnd, m_dlgData.bInner, modelRef);
			tag = uRebar.MakeRebar(rebarset->GetElementId(), modelRef);
		}
		else
		{
			PIT::URebarMaker uRebar(m_elm_V1, m_elm_V2, it->second, m_dlgData.rebarData, m_Holeehs,
				m_dlgData.bNegtive, m_dlgData.bUp, m_dlgData.bInner, modelRef);
			tag = uRebar.MakeRebar(rebarset->GetElementId(), modelRef);
		}
		
		if (NULL != tag)
		{
			tag->SetBarSetTag(nTag + 1);
			rsetTags.Add(tag);
			nTag++;
		}
		//RebarModel *rmv = RMV;
		//if (rmv != nullptr)
		//{
		//	rmv->SaveRebar(*rebarset, rebarset->GetModelRef(), true);
		//}
		if (rebarset != nullptr)
		{
			DgnModelRefP        modelRef = ACTIVEMODEL;
			RebarAssemblies reas;
			RebarAssembly::GetRebarAssemblies(conid, reas);
			RebarAssembly *rebarasb = reas.GetAt(0);
			if (rebarasb != nullptr)
			{
				SingleRebarAssembly*  rebarAsm = REA::Create<SingleRebarAssembly>(modelRef);
				ElementId tmpid = rebarasb->GetSelectedElement();
				if (tmpid == 0)
				{
					return;
				}
				DgnModelRefP modelp = rebarasb->GetSelectedModel();
				EditElementHandle ehSel;
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
				rebarAsm->SetConcreteOwner(conid);
				rebarAsm->SetSlabData(ehSel);
				rebarAsm->NewRebarAssembly(modelRef);
				rebarAsm->AddRebarSets(rsetTags);
				rebarAsm->Save(modelRef);
			}
		}
	}

	for (auto id : m_refLineIds)
	{
		EditElementHandle eh(id, modelRef);
		eh.DeleteFromModel();
	}

	CDialogEx::OnOK();

	delete pURebarDlg;
	pURebarDlg = nullptr;
}


void CURebarToolDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	m_bMonitor = true;
	SelectionSetManager::GetManager().EmptyAll();
	PITSelectURebarTool::InstallURebarInstance(CMDNAME_RebarSDKReadRebar, this);
}


void CURebarToolDlg::DrawRefLine()
{
	DgnModelRefP modelRef = ACTIVEMODEL;

	UpdateRebarData();

	vector<ElementId> reabarIds;
	for (size_t i = 0; i < m_vecElm.size(); i+= m_dlgData.interval)
	{
		ElementHandle eh(m_vecElm[i], modelRef);
		if (RebarElement::IsRebarElement(eh))
		{
			reabarIds.push_back(m_vecElm[i]);
		}
	}

	for (auto id : m_refLineIds)
	{
		EditElementHandle eh(id, ACTIVEMODEL);
		eh.DeleteFromModel();
	}

	EditElementHandle start(mSelsectedRebar[0], mSelsectedRebar[0]->GetDgnModelP());
	string Level = "1";
	string Grade = "A";
	int spacing = 200;
	if (RebarElement::IsRebarElement(start))
	{
		GetRebarLevelItemTypeValue(start, Level, mSelectedRebarType, Grade);//获取选中钢筋的属性，写入U形筋中		
		spacing = GetRebarHideData(start, modelRef);
	}
	strcpy(m_dlgData.rebarData.SelectedRebarType, mSelectedRebarType.c_str());
	strcpy(m_dlgData.rebarData.SelectedRebarLevel, Level.c_str());
	strcpy(m_dlgData.rebarData.SelectedRebarGrade, Grade.c_str());
	m_dlgData.rebarData.SelectedSpacing = spacing;
	m_refLineIds.clear();
	if (m_isLRebar && reabarIds.size() > 1)
	{
		EditElementHandle firstEeh(reabarIds.at(0), modelRef);
		DPoint3d strPt, endPt;
		double dim;
		GetStartEndPointFromRebar(&firstEeh, strPt, endPt, dim);
		EditElementHandle secondEeh(reabarIds.at(1), modelRef);
		DPoint3d strPt1, endPt1;
		GetStartEndPointFromRebar(&secondEeh, strPt1, endPt1, dim);
		DVec3d faceVec = strPt - strPt1;
		faceVec.Normalize();
		PIT::URebarMaker uRebar(reabarIds, m_dlgData.rebarData, m_Holeehs, faceVec,
			m_dlgData.bNegtive, m_dlgData.bUp, m_isEnd, m_dlgData.bInner, modelRef);
		uRebar.DrawRefLine(m_refLineIds, modelRef);
	}
	else
	{
		PIT::URebarMaker uRebar(m_elm_V1, m_elm_V2, reabarIds, m_dlgData.rebarData, m_Holeehs,
			m_dlgData.bNegtive, m_dlgData.bUp, m_dlgData.bInner, modelRef);
		uRebar.DrawRefLine(m_refLineIds, modelRef);
	}
	
}

void CURebarToolDlg::UpdateRebarData()
{
	CString strRebarSize;
	m_CombRebarSize.GetWindowTextW(strRebarSize);

	auto it = g_listRebarType.begin();
	int rebarType = m_CombRebarType.GetCurSel();
	char a[10];
	itoa(rebarType, a,10);
	CString strRebarType(a);

	CString strInterval;
	GetDlgItem(IDC_EDIT13)->GetWindowTextW(strInterval);
	m_dlgData.interval = atoi(CT2A(strInterval)) + 1;

	CString strRebarLeg1;
	GetDlgItem(IDC_EDIT15)->GetWindowTextW(strRebarLeg1);
	double legLength1 = atof(CT2A(strRebarLeg1));
	//if (legLength1 < UREBAR_MINLEN)
	//{
	//	legLength1 = UREBAR_MINLEN;
	//}
	if (legLength1 > UREBAR_MAXLEN)
	{
		legLength1 = UREBAR_MAXLEN;
	}

	CString strRebarLeg2;
	GetDlgItem(IDC_EDIT16)->GetWindowTextW(strRebarLeg2);
	double legLength2 = atof(CT2A(strRebarLeg2));
	//if (legLength2 < UREBAR_MINLEN)
	//{
	//	legLength2 = UREBAR_MINLEN;
	//}
	if (legLength2 > UREBAR_MAXLEN)
	{
		legLength2 = UREBAR_MAXLEN;
	}

	CString strRebarLevel;
	GetDlgItem(IDC_EDIT1)->GetWindowTextW(strRebarLevel);

// 	m_dlgData.bNegtive = m_CheckNeg.GetCheck() ? true : false;
// 	m_dlgData.bUp = m_CheckUp.GetCheck() ? true : false;

	RebarSymbology rebarSymb;
	char ccolar[20] = { 0 };
	strcpy(ccolar, CT2A(strRebarSize));
	SetRebarColorBySize(ccolar, rebarSymb);
	rebarSymb.SetRebarLevel(strRebarLevel);
	PIT::URebarData rebarData;
	m_dlgData.rebarData.rebarSize = strRebarSize;
	m_dlgData.rebarData.rebarType = strRebarType;
	m_dlgData.rebarData.rebarSymb = rebarSymb;

	if (m_dlgData.bLegLenNeg)
	{
		m_dlgData.rebarData.legLength1 = legLength2;
		m_dlgData.rebarData.legLength2 = legLength1;
	}
	else
	{
		m_dlgData.rebarData.legLength1 = legLength1;
		m_dlgData.rebarData.legLength2 = legLength2;
	}
}



void CURebarToolDlg::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	m_dlgData.bNegtive = m_CheckNeg.GetCheck() ? true : false;
	DrawRefLine();
}


void CURebarToolDlg::OnBnClickedCheck5()
{
	// TODO: 在此添加控件通知处理程序代码
	m_dlgData.bUp = m_CheckUp.GetCheck() ? true : false;
	DrawRefLine();
}


void CURebarToolDlg::OnEnChangeEdit13()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_bMonitor)
	{
		DrawRefLine();
	}
}


void CURebarToolDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	for (auto id : m_refLineIds)
	{
		EditElementHandle eh(id, ACTIVEMODEL);
		eh.DeleteFromModel();
	}
	CDialogEx::OnCancel();

	delete pURebarDlg;
	pURebarDlg = nullptr;
}


void CURebarToolDlg::OnEnChangeEdit15()
{
	if (m_bMonitor)
	{
		DrawRefLine();
	}
}


void CURebarToolDlg::OnEnChangeEdit16()
{
	if (m_bMonitor)
	{
		DrawRefLine();
	}
}


void CURebarToolDlg::OnBnClickedCheck6()
{
	if (m_bMonitor)
	{
		m_dlgData.bLegLenNeg = m_CheckLength.GetCheck() ? true : false;
		DrawRefLine();
	}
}





void CURebarToolDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	//double length=0;
	//CString strRebarSize;
	//m_CombRebarSize.GetWindowTextW(strRebarSize);
	//int i = m_CombRebarType.GetCurSel();
	//if (i == 10)
	//{
	//	i=i-1;
	//}
	///*auto it = g_globalpara.m_laplenth.begin();
	//it = g_globalpara.m_laplenth.find(size);
	//if (it != g_globalpara.m_laplenth.end())
	//{
	//	length = it->second;
	//}*/
	//int nType = i;
	//GetDiameterAddType(strRebarSize, nType);
	//char tmpchar[256];
	//strcpy(tmpchar, CT2A(strRebarSize));
	//std::string stRebarsize(tmpchar);
	//length = g_globalpara.m_laplenth[stRebarsize];
	//
	////GetDlgItem(IDC_EDIT15)->SetWindowText(L"600");
	//char a[10];
	//itoa(length, a, 10);
	//CString LOleng(a);
	//GetDlgItem(IDC_EDIT15)->SetWindowText(LOleng);
	//GetDlgItem(IDC_EDIT16)->SetWindowText(LOleng);
}


void CURebarToolDlg::OnCbnKillfocusCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	double length = 0;
	CString strRebarSize;
	m_CombRebarSize.GetWindowTextW(strRebarSize);
	int i = m_CombRebarType.GetCurSel();
	if (i == 10)
	{
		i = i - 1;
	}
	/*auto it = g_globalpara.m_laplenth.begin();
	it = g_globalpara.m_laplenth.find(size);
	if (it != g_globalpara.m_laplenth.end())
	{
		length = it->second;
	}*/
	int nType = i;
	GetDiameterAddType(strRebarSize, nType);
	char tmpchar[256];
	strcpy(tmpchar, CT2A(strRebarSize));
	std::string stRebarsize(tmpchar);
	length = g_globalpara.m_laplenth[stRebarsize];

	//GetDlgItem(IDC_EDIT15)->SetWindowText(L"600");
	char a[10];
	itoa(length, a, 10);
	CString LOleng(a);
	GetDlgItem(IDC_EDIT15)->SetWindowText(LOleng);
	GetDlgItem(IDC_EDIT16)->SetWindowText(LOleng);
}


void CURebarToolDlg::OnBnClickedCheckInner()
{
	if (m_bMonitor)
	{
		m_dlgData.bInner = m_CheckInner.GetCheck() ? true : false;
		DrawRefLine();
	}
}
