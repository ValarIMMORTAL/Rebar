// CStirrupToolDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CStirrupToolDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include <RebarCatalog.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "StirrupRebar.h"
#include "ReadRebarTool.h"
#include "SingleRebarAssembly.h"
#include "CRebarEndPointSetDlg.h"
#include "ExtractFacesTool.h"
#include "SelectRebarTool.h"

// CStirrupToolDlg 对话框
extern CStirrupToolDlg *pStirrupRebarDlg;

IMPLEMENT_DYNAMIC(CStirrupToolDlg, CDialogEx)

CStirrupToolDlg::CStirrupToolDlg(const vector<ElementId> &vecElm, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_StittupRebarTool, pParent), m_vecElm_H(vecElm)
{

}

CStirrupToolDlg::~CStirrupToolDlg()
{
}

void CStirrupToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_CombRebarSize);
	DDX_Control(pDX, IDC_COMBO12, m_CombRebarType);
	DDX_Control(pDX, IDC_CHECK5, m_CheckUp);
	DDX_Control(pDX, IDC_COMBO2, m_CombBegType);
	DDX_Control(pDX, IDC_COMBO13, m_CombEndType);
}


void CStirrupToolDlg::InitRebarEndPointData()
{
	switch (m_dlgData.rebarData.beg.endType)
	{
	case 0:
		break;
	case 1:		//弯曲
	{
		m_dlgData.rebarData.beg.endPtInfo.value1 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
	}
	break;
	case 2:		//吊钩
	{
		m_dlgData.rebarData.beg.endPtInfo.value1 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
		m_dlgData.rebarData.beg.endPtInfo.value4 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
	}
	break;
	case 3:		//折线
		break;
	case 4:		//90度弯钩
	case 5:		//135度弯钩
	case 6:		//180度弯钩
	{
		RebarEndType endType;
		endType.SetType(RebarEndType::kHook);
		m_dlgData.rebarData.beg.endPtInfo.value3 = RebarCode::GetBendLength(m_dlgData.rebarData.rebarSize, endType, ACTIVEMODEL);
		m_dlgData.rebarData.beg.endPtInfo.value1 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
	}
	break;
	case 7:		//直锚
		break;
	case 8:		//用户
	{
		m_dlgData.rebarData.beg.endPtInfo.value1 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
		m_dlgData.rebarData.beg.endPtInfo.value4 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
	}
	break;
	default:
		break;
	}

	switch (m_dlgData.rebarData.end.endType)
	{
	case 0:
		break;
	case 1:		//弯曲
	{
		m_dlgData.rebarData.end.endPtInfo.value1 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
	}
	break;
	case 2:		//吊钩
	{
		m_dlgData.rebarData.end.endPtInfo.value1 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
		m_dlgData.rebarData.end.endPtInfo.value4 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
	}
	break;
	case 3:		//折线
		break;
	case 4:		//90度弯钩
	case 5:		//135度弯钩
	case 6:		//180度弯钩
	{
		RebarEndType endType;
		endType.SetType(RebarEndType::kHook);
		m_dlgData.rebarData.end.endPtInfo.value3 = RebarCode::GetBendLength(m_dlgData.rebarData.rebarSize, endType, ACTIVEMODEL);
		m_dlgData.rebarData.end.endPtInfo.value1 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
	}
	break;
	case 7:		//直锚
		break;
	case 8:		//用户
	{
		m_dlgData.rebarData.end.endPtInfo.value1 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
		m_dlgData.rebarData.end.endPtInfo.value4 = RebarCode::GetPinRadius(m_dlgData.rebarData.rebarSize, ACTIVEMODEL, true);
	}
	break;
	default:
		break;
	}
}

BOOL CStirrupToolDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	// TODO:  在此添加额外的初始化
	m_CombRebarSize.ResetContent();
	m_CombRebarType.ResetContent();
	m_CombBegType.ResetContent();
	m_CombEndType.ResetContent();

	for each (auto var in g_listRebarType)
		m_CombRebarType.AddString(var);

	for each (auto var in g_listRebarSize)
		m_CombRebarSize.AddString(var);

	for each (auto var in g_listEndType)
		m_CombBegType.AddString(var);

	for each (auto var in g_listEndType)
		m_CombEndType.AddString(var);

	CString cstrSize = m_dlgData.rebarData.rebarSize + L"mm";
	auto find = std::find(g_listRebarSize.begin(), g_listRebarSize.end(), cstrSize);
	m_CombRebarSize.SetCurSel((int)std::distance(g_listRebarSize.begin(), find));

	CString cstrType = m_dlgData.rebarData.rebarType;
	auto find1 = std::find(g_listRebarType.begin(), g_listRebarType.end(), cstrType);
	m_CombRebarType.SetCurSel((int)std::distance(g_listRebarType.begin(), find1));

	m_CombRebarType.SetCurSel((int)std::distance(g_listRebarType.begin(), find1));
	m_CombBegType.SetCurSel(5);
	m_CombEndType.SetCurSel(5);
	m_dlgData.rebarData.beg.endType = 5;
	m_dlgData.rebarData.end.endType = 5;
	InitRebarEndPointData();

	GetDlgItem(IDC_EDIT1)->SetWindowText(TEXT_OTHER_REBAR);
	GetDlgItem(IDC_EDIT13)->SetWindowText(L"0");
	GetDlgItem(IDC_EDIT2)->SetWindowText(L"0");
	GetDlgItem(IDC_EDIT3)->SetWindowText(L"0");

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CStirrupToolDlg::DrawRefLine()
{
	DgnModelRefP modelRef = ACTIVEMODEL;

	if (!UpdateRebarData())
	{
		return;
	}

	if (m_dlgData.interval <= 0)
	{
		m_dlgData.interval = 1;
	}

	vector<ElementId> reabarIds;
	for (size_t i = 0; i < m_vecElm_H.size(); i += m_dlgData.interval)
	{
		ElementHandle eh(m_vecElm_H[i], modelRef);
		if (RebarElement::IsRebarElement(eh))
		{
			reabarIds.push_back(m_vecElm_H[i]);
		}
	}

	for (auto id : m_refLineIds)
	{
		EditElementHandle eh(id, ACTIVEMODEL);
		eh.DeleteFromModel();
	}
	m_refLineIds.clear();

	EditElementHandle start(m_selectrebars[0], m_selectrebars[0]->GetDgnModelP());
	string Level = "1";
	string Grade = "A";
	if (RebarElement::IsRebarElement(start))
	{
		GetRebarLevelItemTypeValue(start, Level, mSelectedRebarType, Grade);//获取选中钢筋的属性，写入U形筋中		
	}
	strcpy(m_dlgData.rebarData.SelectedRebarType, mSelectedRebarType.c_str());
	strcpy(m_dlgData.rebarData.SelectedRebarLevel, Level.c_str());
	strcpy(m_dlgData.rebarData.SelectedRebarGrade, Grade.c_str());
	PIT::StirrupRebarMaker rebar(m_vecElm_V, reabarIds, m_dlgData.rebarData, m_dlgData.bUp, modelRef);
	rebar.DrawRefLine(m_refLineIds, modelRef);
//	rebar.mSelectedRebarType = mSelectedRebarType;
}

bool CStirrupToolDlg::UpdateRebarData()
{
	CString strRebarSize;
	m_CombRebarSize.GetWindowTextW(strRebarSize);
	if (strRebarSize == "")
	{
		return false;
	}

	CString strRebarType;
	m_CombRebarType.GetWindowTextW(strRebarType);
	if (strRebarType == "")
	{
		return false;
	}

	CString strInterval;
	GetDlgItem(IDC_EDIT13)->GetWindowTextW(strInterval);
	if (strInterval != "")
	{
		m_dlgData.interval = atoi(CT2A(strInterval)) + 1;
	}

	CString strRebarLevel;
	GetDlgItem(IDC_EDIT1)->GetWindowTextW(strRebarLevel);

	PIT::PITRebarEndTypes endTypes;

	RebarSymbology rebarSymb;
	char ccolar[20] = { 0 };
	strcpy(ccolar,CT2A(strRebarSize));
	SetRebarColorBySize(ccolar, rebarSymb);
	rebarSymb.SetRebarLevel(strRebarLevel);
	PIT::StirrupRebarData rebarData;
	m_dlgData.rebarData.rebarSize = strRebarSize;
	m_dlgData.rebarData.rebarType = strRebarType;
	m_dlgData.rebarData.rebarSymb = rebarSymb;


	CString strAngel1,strAngel2;
	GetDlgItem(IDC_EDIT2)->GetWindowTextW(strAngel1);
	GetDlgItem(IDC_EDIT3)->GetWindowTextW(strAngel2);
	if (strAngel1 != "")
	{
		m_dlgData.rebarData.beg.rotateAngle = atof(CT2A(strAngel1));
	}

	if (strAngel2 != "")
	{
		m_dlgData.rebarData.end.rotateAngle = atof(CT2A(strAngel2));
	}
	return true;
}


BEGIN_MESSAGE_MAP(CStirrupToolDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON3, &CStirrupToolDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON1, &CStirrupToolDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CStirrupToolDlg::OnBnClickedButton2)
	ON_EN_CHANGE(IDC_EDIT13, &CStirrupToolDlg::OnEnChangeEdit13)
	ON_BN_CLICKED(IDC_CHECK5, &CStirrupToolDlg::OnBnClickedCheck5)
	ON_BN_CLICKED(IDC_BUTTON5, &CStirrupToolDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON9, &CStirrupToolDlg::OnBnClickedButton9)
	ON_BN_CLICKED(IDC_BUTTON6, &CStirrupToolDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CStirrupToolDlg::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_BUTTON4, &CStirrupToolDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CStirrupToolDlg 消息处理程序


void CStirrupToolDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	m_bMonitor = true;
	SelectionSetManager::GetManager().EmptyAll();
	PITSelectStirrupRebarTool::InstallStirrupRebarInstance(CMDNAME_RebarSDKReadRebar, this);
}


void CStirrupToolDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	DgnModelRefP modelRef = ACTIVEMODEL;

	if (m_dlgData.interval <= 0)
	{
		m_dlgData.interval = 1;
	}

	ElementId conid = 0; //钢筋组的混凝土id
	RebarSetTag *tag = NULL;
	multimap<RebarSetP, RebarElementP> mlt_Rebar; //横向钢筋所在组与钢筋的映射
	for (size_t i = 0; i < m_vecElm_H.size(); i += m_dlgData.interval)
	{
		ElementHandle eh(m_vecElm_H[i], modelRef);
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

	multimap<RebarSetP, vector<ElementId> > mut_StirrupRebar;
	ElementId preId = 0; //横向钢筋集合id
	RebarSetP pRebarSet = nullptr;
	vector<ElementId> vecRebarId;  //横向钢筋id数组
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
			mut_StirrupRebar.insert(make_pair(pRebarSet, vecRebarId));	//存储所需并筋set与对于钢筋数量
			vecRebarId.clear(); 
		}
		flag++;
	}

	//添加U形钢筋
	UpdateRebarData();

	int nType = m_CombRebarType.FindStringExact(0, m_dlgData.rebarData.rebarType);
	GetDiameterAddType(m_dlgData.rebarData.rebarSize, nType);

	RebarSetTagArray rsetTags;
	vector<RebarArcData> rebarArcDatas;
	for (auto it = mut_StirrupRebar.begin(); it != mut_StirrupRebar.end(); ++it)
	{
		RebarSetP rebarset = it->first;
		PIT::StirrupRebarMaker uRebar(m_vecElm_V, it->second, m_dlgData.rebarData, m_dlgData.bUp, modelRef);
		RebarSetTag* pTag = uRebar.MakeRebar(rebarset->GetElementId(), modelRef);
		if (NULL != pTag)
		{
			pTag->SetBarSetTag((int)std::distance(mut_StirrupRebar.begin(),it)+1);
			rsetTags.Add(pTag);
		}
		rebarArcDatas = uRebar.GetRebarArcDatas();
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
// 		RebarModel *rmv = RMV;
// 		if (rmv != nullptr)
// 		{
// 			rmv->SaveRebar(*rebarset, rebarset->GetModelRef(), true);
// 		}
	}

	for (auto id : m_refLineIds)
	{
		EditElementHandle eh(id, modelRef);
		eh.DeleteFromModel();
	}

	//更新竖直钢筋
	//UpdateVRebars(rebarArcDatas, modelRef);

	CDialogEx::OnOK();
	delete pStirrupRebarDlg;
	pStirrupRebarDlg = nullptr;
}


void CStirrupToolDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	for (auto id : m_refLineIds)
	{
		EditElementHandle eh(id, ACTIVEMODEL);
		eh.DeleteFromModel();
	}

	CDialogEx::OnCancel();
	delete pStirrupRebarDlg;
	pStirrupRebarDlg = nullptr;
}


void CStirrupToolDlg::OnEnChangeEdit13()
{
	if (m_bMonitor)
	{
		DrawRefLine();
	}
}


void CStirrupToolDlg::OnBnClickedCheck5()
{
	// TODO: 在此添加控件通知处理程序代码
	m_dlgData.bUp = m_CheckUp.GetCheck() ? true : false;
	DrawRefLine();
}


//端部样式始端
void CStirrupToolDlg::OnBnClickedButton5()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strRebarSize;
	m_CombRebarSize.GetWindowTextW(strRebarSize);
	strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);	//删掉mm
	CRebarEndPointSetDlg dlg(m_CombBegType.GetCurSel(),this);
//	dlg.SetRebarEndPointInfo(m_vecEndType[msg->m_nRow].endPtInfo);
	dlg.SetRebarSize(BrString(strRebarSize));
	if (IDOK == dlg.DoModal())
	{
		m_dlgData.rebarData.beg.endPtInfo = dlg.GetRebarEndPointInfo();
		m_dlgData.rebarData.beg.endType = m_CombBegType.GetCurSel();
	}
}

//端部样式终端
void CStirrupToolDlg::OnBnClickedButton9()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strRebarSize;
	m_CombRebarSize.GetWindowTextW(strRebarSize);
	strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);	//删掉mm
	CRebarEndPointSetDlg dlg(m_CombEndType.GetCurSel(),this);
//	dlg.SetRebarEndPointInfo(m_vecEndType[msg->m_nRow].endPtInfo);
	dlg.SetRebarSize(BrString(strRebarSize));
	if (IDOK == dlg.DoModal())
	{
		m_dlgData.rebarData.end.endPtInfo = dlg.GetRebarEndPointInfo();
		m_dlgData.rebarData.end.endType = m_CombEndType.GetCurSel();
	}
}

//旋转90度始端
void CStirrupToolDlg::OnBnClickedButton6()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strAngle;
	GetDlgItem(IDC_EDIT2)->GetWindowTextW(strAngle);
	if (strAngle != "")
	{
		double angle = atof(CT2A(strAngle)) + 90;
		strAngle.Format(L"%.2f", angle);
		GetDlgItem(IDC_EDIT2)->SetWindowTextW(strAngle);
	}
}

//旋转90度终端
void CStirrupToolDlg::OnBnClickedButton7()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strAngle;
	GetDlgItem(IDC_EDIT3)->GetWindowTextW(strAngle);
	double angle = atof(CT2A(strAngle)) + 90;
	strAngle.Format(L"%.2f", angle);
	GetDlgItem(IDC_EDIT3)->SetWindowTextW(strAngle);
}

//交换数据
void CStirrupToolDlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strAngle1,strAngle2;
	GetDlgItem(IDC_EDIT2)->GetWindowTextW(strAngle1);
	GetDlgItem(IDC_EDIT3)->GetWindowTextW(strAngle2);
	GetDlgItem(IDC_EDIT2)->SetWindowTextW(strAngle2);
	GetDlgItem(IDC_EDIT3)->SetWindowTextW(strAngle1);

	m_CombBegType.SetCurSel(m_CombEndType.GetCurSel());
	m_CombEndType.SetCurSel(m_CombBegType.GetCurSel());

	PIT::EndType endType = m_dlgData.rebarData.beg;
	m_dlgData.rebarData.beg = m_dlgData.rebarData.end;
	m_dlgData.rebarData.end = endType;
}

void CStirrupToolDlg::UpdateVRebars(const vector<RebarArcData>& rebarAcrDatas, DgnModelRefP modelRef)
{
	BrString strRebarSize = m_dlgData.rebarData.rebarSize;
	if (strRebarSize.Find(L"mm") != WString::npos)
		strRebarSize.Replace(L"mm", L"");
	double uRebarDia = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL); //箍筋直径
	for (size_t i = 0; i < m_vecElm_V.size(); ++i)
	{
		ElementHandle eeh(m_vecElm_V.at(i), modelRef);
		if (RebarElement::IsRebarElement(eeh)) //是否为钢筋元素
		{
			//从EditElementHandle获取元素
			RebarElementP rep = RebarElement::Fetch(eeh);
			//(*elementToModify).GetModelRef()：从ElementAgenda的元素中提取DgnModelRef
			//DgnModelRef提供了对Bentley::DgnPlatform::DgnFile中的模型的访问
			//获取钢筋模板对象
			RebarShape * rebarshape = rep->GetRebarShape((eeh).GetModelRef());
			if (rebarshape == nullptr)
				continue;
			//获取钢筋模板中的线条形状
			RebarCurve curve;
			rebarshape->GetRebarCurve(curve);
			
			//获取钢筋的尺寸数据
			double diameter = 0;
			DPoint3d ptstart; DPoint3d ptend;
			EditElementHandle editEeh(eeh, eeh.GetDgnModelP());
			GetStartEndPointFromRebar(&editEeh, ptstart, ptend, diameter);
			double minZ = (ptstart.z < ptend.z) ? ptstart.z : ptend.z;
			double maxZ = (ptstart.z > ptend.z) ? ptstart.z : ptend.z;

			//根据弯曲处数据修改点筋的位置信息
			RebarArcData arcData = rebarAcrDatas.at(i);
			DVec3d moveVec = arcData.ptArcCenter - arcData.ptArcMid;
			moveVec.Normalize();
			moveVec.Scale(uRebarDia / 2 + diameter / 2);
			DPoint3d rebarPt = arcData.ptArcMid;
			rebarPt.Add(moveVec);

			RebarVertices  vers;
			RebarVertex*   vertmp1; RebarVertex* vertmp2;
			rebarPt.z = minZ;
			vertmp1 = new RebarVertex();
			vertmp1->SetType(RebarVertex::kStart);
			vertmp1->SetIP(rebarPt);
			vers.Add(vertmp1);

			rebarPt.z = maxZ;
			vertmp2 = new RebarVertex();
			vertmp2->SetType(RebarVertex::kEnd);
			vertmp2->SetIP(rebarPt);
			vers.Add(vertmp2);
			curve.SetVertices(vers);

			RebarEndType endType;
			endType.SetType(RebarEndType::kNone);
			RebarEndTypes   endTypes = { endType, endType };

			//设置点筋的颜色和图层
			BrString sizeKey = (rebarshape->GetShapeData().GetSizeKey());
			RebarSymbology symb;
			string str(sizeKey);
			char ccolar[20] = { 0 };
			strcpy(ccolar, str.c_str());
			SetRebarColorBySize(ccolar, symb);			
			symb.SetRebarLevel(TEXT_MAIN_REBAR);//画的是点筋则设置为主筋图层
			rep->SetRebarElementSymbology(symb);
			
			RebarShapeData shape = rebarshape->GetShapeData();
			rep->Update(curve, diameter, endTypes, shape, rep->GetModelRef(), false);
			RebarModel *rmv = RMV;
			if (rmv != nullptr)
			{
				rmv->SaveRebar(*rep, rep->GetModelRef(), true);
			}
		}
	}
}
