// UtoStirrupToolDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "UtoStirrupToolDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "XmlHelper.h"
#include <RebarElements.h>
#include "ConstantsDef.h"
#include "SingleRebarAssembly.h"
#include "ExtractFacesTool.h"


// UtoStirrupToolDlg 对话框

IMPLEMENT_DYNAMIC(UtoStirrupToolDlg, CDialogEx)

UtoStirrupToolDlg::UtoStirrupToolDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_UTOSTIRRUP, pParent)
{

}

UtoStirrupToolDlg::~UtoStirrupToolDlg()
{
}

void UtoStirrupToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_ComboType);
	DDX_Control(pDX, IDC_COMBO12, m_ComboSize);
}

void UtoStirrupToolDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}


BEGIN_MESSAGE_MAP(UtoStirrupToolDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &UtoStirrupToolDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDOK, &UtoStirrupToolDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &UtoStirrupToolDlg::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO1, &UtoStirrupToolDlg::OnCbnSelchangeCombo1)
	ON_CBN_SELCHANGE(IDC_COMBO12, &UtoStirrupToolDlg::OnCbnSelchangeCombo12)
	ON_BN_CLICKED(IDC_BUTTON2, &UtoStirrupToolDlg::OnBnClickedButton2)
END_MESSAGE_MAP()



BOOL UtoStirrupToolDlg::OnInitDialog()      
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端

	for each (auto var in g_listRebarType)
		m_ComboType.AddString(var);

	for each (auto var in g_listRebarSize)
		m_ComboSize.AddString(var);

	if (m_SizeKey)//设置U型筋的初始值
	{
		CString strRebarSize(m_SizeKey);
		CString RebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);
		CString RebarType = strRebarSize.Right(1);

		if (RebarType == L"A")
			m_rebarType = 0;
		else if (RebarType == L"B")
			m_rebarType = 1;
		else if (RebarType == L"C")
			m_rebarType = 2;
		else
			m_rebarType = 3;

		if (RebarSize.Find(L"mm") == -1)
		{
			RebarSize += "mm";
			strcpy(m_rebarSize,CT2A(RebarSize));
		}
		int nIndex = m_ComboSize.FindStringExact(0, RebarSize);
		m_ComboSize.SetCurSel(nIndex);//尺寸
		m_ComboType.SetCurSel(m_rebarType);//型号
	}
	return TRUE;
}



// UtoStirrupToolDlg 消息处理程序

void UtoStirrupToolDlg::OnBnClickedButton1()//切换方向
{
	for (int x = 0; x < m_vctpts.size(); x++)
	{
		vector<CPoint3D>& vctpts = m_vctpts[x];
		vctpts.erase(vctpts.begin());//删除第一个点
		vctpts.push_back(vctpts[0]);//将第一个点再存储一次
	}
	PreviewLines();
}

void UtoStirrupToolDlg::OnCbnSelchangeCombo1()//等级
{
	auto it = g_listRebarType.begin();
	advance(it, m_ComboType.GetCurSel());
	m_rebarType = m_ComboType.GetCurSel();
	PreviewLines();
}


void UtoStirrupToolDlg::clearLines()
{
	for (ElementRefP tmpeeh : m_allLines)
	{
		if (tmpeeh != nullptr)
		{
			EditElementHandle eeh(tmpeeh, tmpeeh->GetDgnModelP());
			eeh.DeleteFromModel();
		}
	}
	m_allLines.clear();
}


void UtoStirrupToolDlg ::PreviewLines()
{
	clearLines();
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kCog);
	endTypeEnd.SetType(RebarEndType::kCog);
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
	BrString strRebarSize = m_SizeKey;
	double endLen = PIT::PITRebarCode::GetBendLength(strRebarSize, endTypeStart, ACTIVEMODEL);;
	double bendRadius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);

	for (int i = 0; i < m_vctpts.size(); i++)
	{

		CVector3D endNormal = CVector3D::From(0, 0, 0);

		PIT::PITRebarCurve  curveTmp;
		curveTmp.makeStirrupURebarWithNormal(m_vctpts[i], bendRadius, endLen * uor_per_mm, endTypes, endNormal);

		RebarVertices  vers = curveTmp.PopVertices();

		EditElementHandle lineEeh;
		ChainHeaderHandler::CreateChainHeaderElement(lineEeh, NULL, false, true, *ACTIVEMODEL);
		for (int i = 0; i < (int)vers.GetSize() - 1; i++)
		{
			RebarVertexP ver = vers.GetAt(i);
			RebarVertexP ver_next = vers.GetAt(i + 1);

			// 当前点是弧线
			if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) != 0)
			{
				EditElementHandle eehArc;
				//画圆弧
				if (SUCCESS != ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(ver->GetCenter(), ver->GetArcPt()[0], ver->GetArcPt()[2]), (ACTIVEMODEL)->Is3d(), *ACTIVEMODEL))
				{
					return;
				}
				ChainHeaderHandler::AddComponentElement(lineEeh, eehArc);
			}
			// 前后两段 直线 -- 直线
			else if (COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) == 0)
			{
				EditElementHandle eehTmp;
				if (SUCCESS != LineHandler::CreateLineElement(eehTmp, NULL, DSegment3d::From(ver->GetIP(), ver_next->GetIP()), true, *ACTIVEMODEL))
				{
					return;
				}
				ChainHeaderHandler::AddComponentElement(lineEeh, eehTmp);
			}
			// 前后两段 直线 -- 弧线
			else if (COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) != 0)
			{
				EditElementHandle eehTmp;
				if (SUCCESS != LineHandler::CreateLineElement(eehTmp, NULL, DSegment3d::From(ver->GetIP(), ver_next->GetArcPt(0)), true, *ACTIVEMODEL))
				{
					return;
				}
				ChainHeaderHandler::AddComponentElement(lineEeh, eehTmp);
			}

			if (ver_next->GetType() == RebarVertex::kEnd)
			{
				if (COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) != 0) // 画弧线
				{
					EditElementHandle eehArc;
					//画圆弧
					if (SUCCESS != ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(ver_next->GetCenter(), ver_next->GetArcPt()[0], ver_next->GetArcPt()[2]), (ACTIVEMODEL)->Is3d(), *ACTIVEMODEL))
					{
						return;
					}
					ChainHeaderHandler::AddComponentElement(lineEeh, eehArc);
				}
				else
				{
					// 前后两段 弧线 -- 直线
					if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) != 0)
					{
						EditElementHandle eehTmp;
						if (SUCCESS != LineHandler::CreateLineElement(eehTmp, NULL, DSegment3d::From(ver->GetArcPt(2), ver_next->GetIP()), true, *ACTIVEMODEL))
						{
							return;
						}
						ChainHeaderHandler::AddComponentElement(lineEeh, eehTmp);
					}
				}
			}
		}
		ChainHeaderHandler::AddComponentComplete(lineEeh);
		lineEeh.AddToModel();
		m_allLines.push_back(lineEeh.GetElementRef());
	}
}



void UtoStirrupToolDlg::OnCbnSelchangeCombo12()//直径
{
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboSize.GetCurSel());
	strcpy(m_rebarSize, CT2A(*it));
	PreviewLines();
}

void UtoStirrupToolDlg::ProcessRebarData()
{
	RebarSymbology rebarSymb;
	SetRebarColorBySize(m_rebarSize, rebarSymb);
	rebarSymb.SetRebarLevel(TEXT_OTHER_REBAR);

	EditElementHandle start(m_selectrebars[0], m_selectrebars[0]->GetDgnModelP());
	GetRebarLevelItemTypeValue(start, mSelectedRebarType);//获取选中钢筋的属性，写入点筋中

	PIT::StirrupRebarData rebarData;
	BrString rebarType(m_rebarType);
	rebarData.rebarType = rebarType;
	rebarData.rebarSymb = rebarSymb;

	
	rebarData.beg = { 5,0,0 };
	rebarData.end = { 5,0,0 };
	rebarData.rebarSize = m_rebarSize;
	RebarEndType endType;
	endType.SetType(RebarEndType::kHook);
    rebarData.beg.endPtInfo.value3 = RebarCode::GetBendLength(rebarData.rebarSize, endType, ACTIVEMODEL);
	rebarData.beg.endPtInfo.value1 = RebarCode::GetPinRadius(rebarData.rebarSize, ACTIVEMODEL, true);

	rebarData.end.endPtInfo.value3 = RebarCode::GetBendLength(rebarData.rebarSize, endType, ACTIVEMODEL);
	rebarData.end.endPtInfo.value1 = RebarCode::GetPinRadius(rebarData.rebarSize, ACTIVEMODEL, true);
	
	
	strcpy(rebarData.SelectedRebarType, mSelectedRebarType.c_str());

	int nType = atoi(CT2A(rebarData.rebarType));
	GetDiameterAddType(rebarData.rebarSize, nType);

	for (int x = 0; x < m_vctpts.size(); x++)
	{
		m_pStirrupRebars.push_back(shared_ptr<UtoStirrupToolRebar>(new UtoStirrupToolRebar(m_vctpts[x], rebarData)));
	}
}



void UtoStirrupToolDlg::OnBnClickedOk()
{
	ProcessRebarData();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	vector<ElementId> vecElementID;
	vecElementID.push_back(0);

	ElementId conid = 0;
	RebarSetP tmpset = nullptr;
	if (m_vecElm_H.size()==0)
	{
		return;
	}
	EditElementHandle tmprebar(m_vecElm_H[0],ACTIVEMODEL);
	for (size_t i = 0; i < m_vecElm_H.size(); i++)
	{
		ElementHandle eh(m_vecElm_H[i], modelRef);
		if (RebarElement::IsRebarElement(eh))
		{
			RebarElementP rep = RebarElement::Fetch(eh);
			RebarSetP pRebarSet = rep->GetRebarSet(modelRef);
			if (pRebarSet != nullptr)
			{
				if (i==0)
				{
					tmpset = pRebarSet;
				}
				int rebar_cage_type;
				conid = pRebarSet->GetConcreteOwner(ACTIVEMODEL, rebar_cage_type);
				if (conid != 0)
					break;
			}
		}
	}
	

	RebarSetTagArray rsetTags;
	RebarSetP rebarSet = RebarSet::Fetch(vecElementID[0], modelRef);
 	if (rebarSet == nullptr)
 	{
 		return;
	}
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(vecElementID[0]);
	rebarSet->StartUpdate(modelRef);
	int rebarNum = (int)m_pStirrupRebars.size();
	for (int i = 0; i < rebarNum; ++i)
	{
		m_pStirrupRebars[i]->Create(*rebarSet);
	}

	RebarSetTag *tag = new RebarSetTag;
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);

	if (NULL != tag)
	{
		tag->SetBarSetTag(1);
		rsetTags.Add(tag);
	}

	DgnModelRefP modelp = nullptr;
	if (rebarSet != nullptr)
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

	{//删除钢筋处理
		mdlSelect_freeAll();
		for (ElementRefP tref : m_selectrebars)
		{
			EditElementHandle tmpeeh(tref, tref->GetDgnModelP());
			SelectionSetManager::GetManager().AddElement(tmpeeh.GetElementRef(), ACTIVEMODEL);
		}
	}
	m_selectrebars.clear();

	clearLines();
	CDialogEx::OnOK();
	DestroyWindow();
	mdlInput_sendSynchronizedKeyin(L"proconcrete delete rebar", 0, INPUTQ_EOQ, NULL);
	mdlInput_sendSynchronizedKeyin(L"choose element", 0, INPUTQ_EOQ, NULL);
}


void UtoStirrupToolDlg::OnBnClickedCancel()
{
	clearLines();
	CDialogEx::OnCancel();
	DestroyWindow();
}


void UtoStirrupToolDlg::SaveRebarPts(vector< vector<CPoint3D>>& Vctpts)
{
	m_vctpts = Vctpts;
}



void UtoStirrupToolDlg::SaveRebarData(BrString& RebarSize, vector <ElementId>&vecElm)
{
	strcpy(m_SizeKey, RebarSize);
	m_vecElm_H = vecElm;
}





UtoStirrupToolRebar::UtoStirrupToolRebar(const vector<CPoint3D> &vecRebarPts, PIT::StirrupRebarDataCR rebarData)
	: PIT::StirrupRebar(vecRebarPts, rebarData), m_vecRebarPt(vecRebarPts), m_rebarData(rebarData)
{
}



void UtoStirrupToolDlg::OnBnClickedButton2()//预览
{
	PreviewLines();
	// TODO: 在此添加控件通知处理程序代码
}
