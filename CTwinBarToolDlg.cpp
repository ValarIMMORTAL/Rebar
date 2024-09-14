// CTwinBarToolDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CTwinBarToolDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include <RebarCatalog.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "ReadRebarTool.h"
#include "TwinBar.h"
#include "SingleRebarAssembly.h"
#include "ExtractFacesTool.h"
#include "ElementAttribute.h"
// CTwinBarToolDlg 对话框

IMPLEMENT_DYNAMIC(CTwinBarToolDlg, CDialogEx)

CTwinBarToolDlg::CTwinBarToolDlg(vector<ElementId> vecElm, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_TwinBarTool, pParent), m_vecElm(vecElm), m_bMonitor(false)
{
	if (m_vecElm.size() > 1)
	{
		EditElementHandle firstEeh(m_vecElm.at(0), ACTIVEMODEL);
		double diameter = 0;
		DPoint3d firstStrPt; DPoint3d firstEndPt;
		GetStartEndPointFromRebar(&firstEeh, firstStrPt, firstEndPt, diameter);

		EditElementHandle secondEeh(m_vecElm.at(1), ACTIVEMODEL);
		DPoint3d secondStrPt; DPoint3d secondEndPt;
		GetStartEndPointFromRebar(&secondEeh, secondStrPt, secondEndPt, diameter);

		m_vec = firstStrPt - secondStrPt;
	}
}

CTwinBarToolDlg::~CTwinBarToolDlg()
{
}

void CTwinBarToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_CombRebarSize);
	DDX_Control(pDX, IDC_COMBO12, m_CombRebarType);
	DDX_Control(pDX, IDC_CHECK1, m_Check);
}


BEGIN_MESSAGE_MAP(CTwinBarToolDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CTwinBarToolDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_CHECK1, &CTwinBarToolDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_BUTTON3, &CTwinBarToolDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON2, &CTwinBarToolDlg::OnBnClickedButton2)
	ON_EN_CHANGE(IDC_EDIT13, &CTwinBarToolDlg::OnEnChangeEdit13)
END_MESSAGE_MAP()



BOOL CTwinBarToolDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	// TODO:  在此添加额外的初始化
	m_CombRebarSize.ResetContent();
	m_CombRebarType.ResetContent();

	for each (auto var in g_listRebarType)
		m_CombRebarType.AddString(var);
	m_CombRebarType.SetCurSel(0);
	for each (auto var in g_listRebarSize)
		m_CombRebarSize.AddString(var);
	m_CombRebarSize.SetCurSel(0);
	
	GetDlgItem(IDC_EDIT13)->SetWindowText(L"0");

	CString cstrSize = m_rebarSize + L"mm";
	auto find = std::find(g_listRebarSize.begin(), g_listRebarSize.end(), cstrSize);
	m_CombRebarSize.SetCurSel((int)std::distance(g_listRebarSize.begin(), find));

	CString cstrType = m_rebarType;
	auto find1 = std::find(g_listRebarType2.begin(), g_listRebarType2.end(), cstrType);
	m_CombRebarType.SetCurSel((int)std::distance(g_listRebarType2.begin(), find1));

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CTwinBarToolDlg::DrawRefLine(bool negate)
{
	DgnModelRefP modelRef = ACTIVEMODEL;

	CString strRebarSize;
	m_CombRebarSize.GetWindowTextW(strRebarSize);

	CString strRebarType;
	m_CombRebarType.GetWindowTextW(strRebarType);

	CString strInterval;
	GetDlgItem(IDC_EDIT13)->GetWindowTextW(strInterval);
	int interval = atoi(CT2A(strInterval)) + 1;

// 	BrString rebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);
// 	double dim = RebarCode::GetBarDiameter(rebarSize, modelRef);
// 	double dimOrg = RebarCode::GetBarDiameter(m_rebarSize, modelRef);

	DVec3d pt = m_vec;
	if (negate)
	{
		pt.Negate();
	}


	for (auto id : m_refLineIds)
	{
		EditElementHandle eh(id, ACTIVEMODEL);
		eh.DeleteFromModel();
	}

 	m_refLineIds.clear();
// 	pt.ScaleToLength(dim*0.5 + dimOrg);
// 	Transform trans;
// 	mdlTMatrix_getIdentity(&trans);
// 	mdlTMatrix_setTranslation(&trans, &pt);
	RebarSymbology rebarSymb;
	char ccolar[20] = { 0 };
	strcpy(ccolar, CT2A(strRebarSize));
	SetRebarColorBySize(ccolar, rebarSymb);
	rebarSymb.SetRebarLevel(TEXT_TWIN_REBAR);
	PIT::RebarData rebarData = { strRebarSize,strRebarType,rebarSymb };
	for (int i = 0; i < m_vecElm.size(); i+=interval)
	{
		ElementHandle eh(m_vecElm[i], ACTIVEMODEL);
		RebarElementP rebar = RebarElement::Fetch(eh);
		m_refLineIds.push_back(PIT::TwinRebar(rebar, rebarData, pt).DrawRefLine());

// 		ElementHandle eh(m_vecElm[i], ACTIVEMODEL);
// 		RebarElementP rebar = RebarElement::Fetch(eh);
// 		CurveVectorPtr rebarCurve = CurveVector::Create(CurveVector::BOUNDARY_TYPE_None);
// 		rebar->GetCurveVector(*rebarCurve, ACTIVEMODEL);
// 		EditElementHandle eeh;
// 		if (SUCCESS == DraftingElementSchema::ToElement(eeh, *rebarCurve, NULL, true, *ACTIVEMODEL))
// 		{
// 			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, (TransformInfo)trans);
// 			eeh.AddToModel();
// 			m_refLineIds.push_back(eeh.GetElementId());
// 		}
 	}
}

// CTwinBarToolDlg 消息处理程序
void CTwinBarToolDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	DgnModelRefP modelRef = ACTIVEMODEL;

	CString strRebarSize;
	m_CombRebarSize.GetWindowTextW(strRebarSize);

	CString strRebarType;
	m_CombRebarType.GetWindowTextW(strRebarType);
	int nType = m_CombRebarType.GetCurSel();

	GetDiameterAddType(strRebarSize, nType);

	CString strInterval;
	GetDlgItem(IDC_EDIT13)->GetWindowTextW(strInterval);
	int interval = atoi(CT2A(strInterval)) + 1;

	RebarSetTag *tag = NULL;
	RebarModel rebarmodel;
	ElementId conid = 0;
	multimap<RebarSetP, RebarElementP> mlt_Rebar;
	for (int i = 0; i < m_vecElm.size(); i += interval)
	{
		ElementId id = m_vecElm[i];
		ElementHandle eh(id, modelRef);
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

	multimap<RebarSetP, vector<RebarElementP> > mut_TwinRebar;
	ElementId preId = 0;
	RebarSetP pRebarSet = nullptr;
	vector<RebarElementP> vecRebar;
	int flag = 0;
	for (auto it = mlt_Rebar.begin(); it != mlt_Rebar.end(); ++it)
	{
		vecRebar.push_back(it->second);
		preId = (it->first)->GetElementId();
		ElementId curId = preId;
		if (curId != preId || flag == mlt_Rebar.size()-1)
		{
			preId = curId;
			ElementId rebarSetId = 0;
			pRebarSet = RebarSet::Fetch(rebarSetId, modelRef);
			mut_TwinRebar.insert(make_pair(pRebarSet, vecRebar));	//存储所需并筋set与对于钢筋数量
			vecRebar.clear();
		}
		flag++;
	}

	DVec3d pt = m_vec;
	if (m_Check.GetCheck())
	{
		pt.Negate();
	}

	RebarSetTagArray rsetTags;
	int nTag = 0;
	//添加并筋
	for (auto it = mut_TwinRebar.begin(); it != mut_TwinRebar.end(); ++it)
	{
		RebarSetP rebarset = it->first;
		vector<pair<ElementId, PIT::RebarData> > vecRebarData;
		for (int i = 0; i < (it->second).size(); ++i)
		{
			if ((it->second).at(i)==nullptr)
			{
				continue;
			}
			
			RebarSymbology rebarSymb;
			char ccolar[20] = { 0 };
			strcpy(ccolar, CT2A(strRebarSize));
			SetRebarColorBySize(ccolar, rebarSymb);
			rebarSymb.SetRebarLevel(TEXT_TWIN_REBAR);
			PIT::RebarData rebarData = { strRebarSize,strRebarType,rebarSymb };
			ElementId tmpid = (it->second).at(i)->GetElementId();
			EditElementHandle rebareeh(tmpid, ACTIVEMODEL);
			string Level = "1";
			string Grade = "A";
			string type = "";
			GetRebarLevelItemTypeValue(rebareeh, Level, type, Grade);//获取选中钢筋的属性，写入U形筋中		
			rebarData.SelectedSpacing = GetRebarHideData(rebareeh, ACTIVEMODEL);
			strcpy(rebarData.SelectedRebarType, type.c_str());
			strcpy(rebarData.SelectedRebarLevel, Level.c_str());
			strcpy(rebarData.SelectedRebarGrade, Grade.c_str());
			vecRebarData.push_back(make_pair((it->second).at(i)->GetElementId(), rebarData));
		}
		PIT::TwinBarMaker twinRebar(vecRebarData, pt, modelRef);
		//twinRebar.MakeRebar(rebarset->GetElementId(), modelRef);
		//RebarModel *rmv = RMV;
		//if (rmv != nullptr)
		//{
		//	rmv->SaveRebar(*rebarset, rebarset->GetModelRef(), true);
		//}

		RebarSetTag* tag = twinRebar.MakeRebar(rebarset->GetElementId(), modelRef);
		if (NULL != tag)
		{
			tag->SetBarSetTag(nTag + 1);
			rsetTags.Add(tag);
			nTag++;
		}
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

// 	BrString rebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);
// 	double dim = RebarCode::GetBarDiameter(rebarSize, modelRef);
// 	double dimOrg = RebarCode::GetBarDiameter(m_rebarSize, modelRef);
// 
// 	DPoint3d pt = m_vec;
// 	pt.ScaleToLength(dim*0.5 + dimOrg);
// 	Transform trans;
// 	mdlTMatrix_getIdentity(&trans);
// 	mdlTMatrix_setTranslation(&trans, &pt);

// 	RebarSetTag *tag = NULL;
// 	RebarModel rebarmodel;
// 	multimap<RebarSetP, RebarElementP> mlt_Rebar;
// 	for (ElementId id : m_vecElm)
// 	{
// 		ElementHandle eh(id, ACTIVEMODEL);
// 		if (RebarElement::IsRebarElement(eh))
// 		{
// 			RebarElementP rep = RebarElement::Fetch(eh);
// 			RebarSetP pRebarSet = rep->GetRebarSet(ACTIVEMODEL);
// 			if (pRebarSet != nullptr)
// 			{
// 				mlt_Rebar.insert(make_pair(pRebarSet, rep));
// 			}
// 		}
// 	}
// 
// 	if (mlt_Rebar.empty())
// 	{
// 		return;
// 	}
// 
// 	multimap<RebarSetP, vector<RebarElementP> > mut_TwinRebar;
// 	ElementId preId = 0;
// 	RebarSetP pRebarSet = nullptr;
// 	vector<RebarElementP> vecRebar;
// 	int flag = 0;
// 	for (auto it = mlt_Rebar.begin(); it != mlt_Rebar.end(); ++it)
// 	{
// 		vecRebar.push_back(it->second);
// 		preId = (it->first)->GetElementId();
// 		ElementId curId = preId;
// 		if (curId != preId || flag == mlt_Rebar.size()-1)
// 		{
// 			preId = curId;
// 			ElementId rebarSetId = 0;
// 			pRebarSet = RebarSet::Fetch(rebarSetId, ACTIVEMODEL);
// 			mut_TwinRebar.insert(make_pair(pRebarSet, vecRebar));	//存储所需并筋set与对于钢筋数量
// 			vecRebar.clear();
// 		}
// 		flag++;
// 	}
// 
// 	//添加并筋
// 	for (auto it = mut_TwinRebar.begin(); it != mut_TwinRebar.end(); ++it)
// 	{
// 		RebarSetP pRebarSet = it->first;
// 		int rebarNum = (int)it->second.size();
// 		vector<RebarElementP> &vecRebar = it->second;
// 
// 		for (int i = 0; i < rebarNum; i+=interval)
// 		{
// 			RebarSymbology symb;
// 			symb.SetRebarColor(-1);
// 			symb.SetRebarLevel(TEXT_TWIN_REBAR);
// 			RebarElementP rebarElement = pRebarSet->AssignRebarElement(i, rebarNum, symb, modelRef);
// 			if (nullptr != rebarElement)
// 			{
// 				RebarShapeData twinRebarData = *(vecRebar[i]->GetShapeData(modelRef));
// 				twinRebarData.SetSizeKey((LPCTSTR)rebarSize);
// 				RebarCurve rebarCurve;
// 				BrString strRebarSize;
// 				vecRebar[i]->GetRebarCurve(rebarCurve, strRebarSize, modelRef);
// 				rebarCurve.DoTransform(trans);
// 				RebarEndType endType;
// 				endType.SetType(RebarEndType::kNone);
// 				RebarEndTypes endTypes = { endType,endType };
// 				rebarElement->Update(rebarCurve, dim, endTypes, twinRebarData, modelRef, false);
// 
// 				RebarModel *rmv = RMV;
// 				if (rmv != nullptr)
// 				{
// 					rmv->SaveRebar(*pRebarSet, rebarElement->GetModelRef(), true);
// 				}
// 			}
// 		}
// 	}
// 
// 	for (auto id : m_refLineIds)
// 	{
// 		EditElementHandle eh(id, ACTIVEMODEL);
// 		eh.DeleteFromModel();
// 	}
	CDialogEx::OnOK();
}


void CTwinBarToolDlg::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	m_Check.GetCheck() ? DrawRefLine(true) : DrawRefLine(false);
}



void CTwinBarToolDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	m_bMonitor = true;
	SelectionSetManager::GetManager().EmptyAll();
	PITSelectTwinRebarTool::InstallTwinRebarInstance(CMDNAME_RebarSDKReadRebar, this);
}


void CTwinBarToolDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	for (auto id : m_refLineIds)
	{
		EditElementHandle eh(id, ACTIVEMODEL);
		eh.DeleteFromModel();
	}
	CDialogEx::OnCancel();
}


void CTwinBarToolDlg::OnEnChangeEdit13()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_bMonitor)
	{
		m_Check.GetCheck() ? DrawRefLine(true) : DrawRefLine(false);
	}
}
