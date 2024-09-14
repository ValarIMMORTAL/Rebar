// CDoorHoleDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "resource.h"
#include "CDoorHoleDlg.h"
#include "CHoleRebarListCtrl.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"
#include "GalleryIntelligentRebarids.h"
#include "ScanIntersectTool.h"
#include "ElementAttribute.h"
#include "HoleRebarAssembly.h"
#include "DoorHoleRebarAssembly.h"
#include "XmlHelper.h"
#include "PITMSCECommon.h"
#include "CHoleRebar_ReinforcingDlg.h"
#include "PITRebarAssembly.h"
#include "WallRebarAssembly.h"
#include "PITMSCECommon.h"
// CDoorHoleDlg 对话框

IMPLEMENT_DYNAMIC(CDoorHoleDlg, CDialogEx)

CDoorHoleDlg::CDoorHoleDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_DOORHOLE, pParent)
{
	m_HoleRebar = nullptr;
}

CDoorHoleDlg::~CDoorHoleDlg()
{
}

void CDoorHoleDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}


void CDoorHoleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DoorHole, m_list_doorhole);
}
void CDoorHoleDlg::InitUIData()
{

	// TODO:  在此添加额外的初始化
	LONG lStyle;
	lStyle = GetWindowLong(m_list_doorhole.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_list_doorhole.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_list_doorhole.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_list_doorhole.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_list_doorhole.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_list_doorhole.InsertColumn(0, _T("孔洞名称"), (int)(width / 4.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_list_doorhole.InsertColumn(1, _T("孔洞大小"), (int)(width / 4.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_list_doorhole.InsertColumn(2, _T("偏离主筋距离"), (int)(width / 4.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_list_doorhole.InsertColumn(3, _T("生成"), (int)(width / 4.0), ListCtrlEx::CheckBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	//	vector<PIT::AssociatedComponent> vecACData;
	SetListDefaultData();
	ExTractHoleDatas();
	UpdateACList();
	g_ConcreteId = m_ConcreteId;
	//m_HoleRebar = nullptr;
	//m_ArcHoleRebar = nullptr;
}
bool CDoorHoleDlg::GetArcCenterPoint(std::vector<RebarPoint>& RebarPts, DPoint3d& ptCenter, EditElementHandle* holedescr)
{
	vector<EditElementHandle*> allHoles;
	allHoles.push_back(holedescr);
	if (holedescr == nullptr)
	{
		return false;
	}
	for (RebarPoint pt : RebarPts)
	{
		if (pt.vecDir == 0)
		{
			vector<DPoint3d> interPoints;
			GetARCIntersectPointsWithHoles(interPoints, allHoles, pt.ptstr, pt.ptend, pt.ptmid);
			if (interPoints.size() == 2)
			{
				ptCenter = interPoints[0];
				ptCenter.Add(interPoints[1]);
				ptCenter.Scale(0.5);
				return true;
			}
		}
	}
	return false;
}
void CDoorHoleDlg::SetListDefaultData()
{
	m_list_doorhole.m_Dlg = this;
	m_nowHolename = "";
	m_nowHoleNum = 0;
	ScanWallDoorHoles(m_holeidAndmodel,m_NEGholeidAndmodel, m_ehSel);
	m_list_doorhole.m_holeidAndmodel.insert(m_holeidAndmodel.begin(), m_holeidAndmodel.end());
	ElementId unionid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), unionid, UnionWallIDXAttribute, m_ehSel.GetModelRef());
	ElementId testid = 0;
	if (unionid != 0)
	{
		EditElementHandle eeh(unionid, ACTIVEMODEL);
		GetElementXAttribute(unionid, sizeof(ElementId), testid, ConcreteIDXAttribute, ACTIVEMODEL);
	}
	if (testid == 0)
	{
		GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	}
	GetElementXAttribute(testid, m_RebarPts, vecRebarPointsXAttribute, ACTIVEMODEL);
	GetConcreteXAttribute(testid, ACTIVEMODEL);

	GetElementXAttribute(testid, m_FrontPts, FrontPtsXAttribute, ACTIVEMODEL);
	GetElementXAttribute(testid, sizeof(Transform), m_trans, UcsMatrixXAttribute, ACTIVEMODEL);
	m_trans.InverseOf(m_trans);

	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ACCConcrete), m_acconcrete, ConcreteCoverXAttribute, m_ehSel.GetModelRef());

	/*if (tmpPts.size()>1)
	{
		for (int i = 0; i < tmpPts.size() - 1; i++)
		{
			int j = i + 1;
			DPoint3d ptstr = tmpPts.at(i).pt;
			DPoint3d ptend = tmpPts.at(j).pt;
			if (tmpPts.at(i).Layer == tmpPts.at(j).Layer)
			{
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
				eeh.AddToModel();
			}
			i++;
		}
	}*/

}

void CDoorHoleDlg::UpdateACList()
{
	int nItemNum = 0;
	m_list_doorhole.DeleteAllItems();
	for (int j = 0; j < m_vecReinF.size(); ++j)
	{
		/*if (m_vecReinF[j].isUnionChild == true)
		{
			continue;
		}*/
		m_list_doorhole.InsertItem(nItemNum, _T("")); // 插入行
		m_list_doorhole.SetItemText(nItemNum, 0, CString(m_vecReinF[j].Hname));
		for (int k = 1; k < 4; ++k)
		{
			CString strValue;
			switch (k)
			{
			case 1:
			{
				strValue.Format(_T("%f"), m_vecReinF[j].Hsize);
				m_list_doorhole.SetItemText(nItemNum, k, strValue);
				break;
			}
			case 2:
			{
				strValue.Format(_T("%f"), m_vecReinF[j].MainRebarDis);
				m_list_doorhole.SetItemText(nItemNum, k, strValue);
				break;
			}
				
			case 3:
			{
				m_list_doorhole.SetCellChecked(nItemNum, k, m_vecReinF[j].isGenerate);
				break;
			}
			default:
				break;
			}
			
		}
		nItemNum++;
	}
	//m_list_holeReinforcing.SetShowProgressPercent(TRUE);
	//m_list_holeReinforcing.SetSupportSort(TRUE);
}

BEGIN_MESSAGE_MAP(CDoorHoleDlg, CDialogEx)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDOK, &CDoorHoleDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDoorHoleDlg::OnBnClickedCancel)
	ON_NOTIFY(HDN_ITEMCLICK, 0, &CDoorHoleDlg::OnHdnItemclickListDoorhole)
END_MESSAGE_MAP()


// CDoorHoleDlg 消息处理程序
BOOL CDoorHoleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	mdlSelect_freeAll();
	mdlView_updateSingle(0);
	InitUIData();
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CDoorHoleDlg::OnDestroy()
{
	m_list_doorhole.DeleteElements();
	CDialogEx::OnDestroy();
	// TODO: 在此处添加消息处理程序代码
}

void CDoorHoleDlg::InitReinForcingInfoData(HoleRebarInfo::ReinForcingInfo& refdata)
{
	refdata.v1 = 0;
	refdata.v2 = 0;
	refdata.h3 = 0;
	refdata.h4 = 0;

	refdata.numv1 = 1;
	refdata.numv2 = 1;
	refdata.numh3 = 1;
	refdata.numh4 = 1;

	refdata.isUnionChild = false;
	refdata.isUnionHole = false;

	refdata.spacingv1 = 150;//g_globalpara.Getrebarspacing();
	refdata.spacingv2 = 150;//g_globalpara.Getrebarspacing();
	refdata.spacingh3 = 150;//g_globalpara.Getrebarspacing();
	refdata.spacingh4 = 150;//g_globalpara.Getrebarspacing();
	

}
void CDoorHoleDlg::UpdateHoleDataView(string holename)
{
	m_nowHolename = holename;
	for (HoleRebarInfo::ReinForcingInfo testInfo : m_vecReinF)
	{
		string tHolename(testInfo.Hname);
		if (tHolename != holename)
		{
			continue;
		}
	}


}
void CDoorHoleDlg::GetNowHoleNum()
{

	for (int i = 0; i < m_vecReinF.size(); i++)
	{
		string tHolename(m_vecReinF.at(i).Hname);
		if (tHolename != m_nowHolename)
		{
			continue;
		}
		m_nowHoleNum = i;
		break;
	}


}
void CDoorHoleDlg::ExTractHoleDatas()
{
	ElementId testid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	GetElementXAttribute(testid, m_vecReinF, DoorInfoXAttribute, ACTIVEMODEL);
	GetConcreteXAttribute(testid, ACTIVEMODEL);
	if (m_vecReinF.size() < 1)
	{
		std::map<std::string, IDandModelref>::iterator itr = m_holeidAndmodel.begin();
		if (isArcwall == false)//不是弧形墙时处理方式
		{
			for (; itr != m_holeidAndmodel.end(); itr++)
			{
				HoleRebarInfo::ReinForcingInfo testInfo;
				EditElementHandle eeh(itr->second.ID, itr->second.tModel);

				ISolidKernelEntityPtr entityPtr;
				if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
				{
					SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(eeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(eeh);
				}
				double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
				DPoint3d ptcenter = getCenterOfElmdescr(eeh.GetElementDescrP());
				for (int i = 0; i < (int)m_FrontPts.size() - 1; i++)
				{

					vector<DPoint3d> interpts;
					DPoint3d tmpStr, tmpEnd;
					tmpStr = m_FrontPts[i];
					tmpEnd = m_FrontPts[i + 1];
					tmpStr.z = tmpEnd.z = ptcenter.z;
					GetIntersectPointsWithHole(interpts, &eeh, tmpStr, tmpEnd);
					if (interpts.size() > 0)
					{
						DPoint3d ptStart = m_FrontPts[i];
						DPoint3d ptEnd = m_FrontPts[i + 1];

						CVector3D  xVec(ptStart, ptEnd);

						CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);

						CVector3D  xVecNew(ptStart, ptEnd);
						BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴

						placement.AssignTo(m_trans);
						m_trans.InverseOf(m_trans);

						break;
					}
				}
				TransformInfo transinfo(m_trans);
				eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
				DPoint3d minP;
				DPoint3d maxP;
				//计算指定元素描述符中元素的范围。
				mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
				DRange3d       vecRange;
				vecRange.low = minP;
				vecRange.high = maxP;
				testInfo.Hsize = (vecRange.ZLength() > vecRange.XLength()) ? vecRange.ZLength() : vecRange.XLength();
				testInfo.Hsize = testInfo.Hsize / uor_per_mm;
				size_t tpos = itr->first.find('@');
				if (tpos == string::npos)
				{
					tpos = itr->first.size();
				}
				itr->first.copy(testInfo.Hname, tpos, 0);
				InitReinForcingInfoData(testInfo);
				/*if (testInfo.Hsize < 300 && testInfo.Hsize>200)
				{
					testInfo.v1 = 1;
					testInfo.h3 = 1;
				}
				else if (testInfo.Hsize > 300)
				{
					testInfo.v1 = 1;
					testInfo.h3 = 1;
					testInfo.v2 = 1;
					testInfo.h4 = 1;
				}*/
				testInfo.L0Lenth = 200;
				testInfo.MainRebarDis = 10;
				m_vecReinF.push_back(testInfo);


			}

		}
		else
		{
			for (; itr != m_holeidAndmodel.end(); itr++)
			{
				HoleRebarInfo::ReinForcingInfo testInfo;
				EditElementHandle eeh(itr->second.ID, itr->second.tModel);

				ISolidKernelEntityPtr entityPtr;
				if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
				{
					SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(eeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(eeh);
				}
				double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
				DPoint3d ptcenter;
				if (!GetArcCenterPoint(m_RebarPts, ptcenter, &eeh))
				{
					continue;
				}
				DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());

				ptcenter.z = ptele.z;
				CVector3D yVec = ptcenter - ptele;
				yVec.Normalize();

				CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

				DPoint3d ptStart = ptcenter;
				BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴

				placement.AssignTo(m_trans);
				m_trans.InverseOf(m_trans);

				TransformInfo transinfo(m_trans);
				eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
				DPoint3d minP;
				DPoint3d maxP;
				//计算指定元素描述符中元素的范围。
				mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
				DRange3d       vecRange;
				vecRange.low = minP;
				vecRange.high = maxP;
				testInfo.Hsize = (vecRange.ZLength() > vecRange.XLength()) ? vecRange.ZLength() : vecRange.XLength();
				testInfo.Hsize = testInfo.Hsize / uor_per_mm;
				size_t tpos = itr->first.find('@');
				if (tpos == string::npos)
				{
					tpos = itr->first.size();
				}
				itr->first.copy(testInfo.Hname, tpos, 0);
				InitReinForcingInfoData(testInfo);
				testInfo.L0Lenth = 200;
				testInfo.MainRebarDis = 10;
				m_vecReinF.push_back(testInfo);


			}
		}

	}


}
void CDoorHoleDlg::RefreshDoorRebars(ElementId conid, EditElementHandleR eeh)
{
	PIT::PITRebarAssembly::DeleteRebarsFromAssembly(conid, "class DoorHoleRebarAssembly");
	if (RebarElement::IsRebarElement(eeh))
	{
		return;
	}
	WallRebarAssembly::ElementType eleType = WallRebarAssembly::JudgeElementType(eeh);
	bool isarcwall = false;
	bool isfloor = false;
	if (WallRebarAssembly::JudgeElementType(eeh) != WallRebarAssembly::FLOOR)
	{

		WallRebarAssembly::WallType wallType = WallRebarAssembly::JudgeWallType(eeh);
		if (wallType == WallRebarAssembly::ARCWALL)
		{
			isarcwall = true;
		}
	}
	else
	{
		isfloor = true;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CDoorHoleDlg* pHoleDlgtmp = NULL;
	pHoleDlgtmp = new CDoorHoleDlg(CWnd::FromHandle(MSWIND));
	pHoleDlgtmp->isArcwall = isarcwall;
	pHoleDlgtmp->SetSelectElement(eeh);
	pHoleDlgtmp->Create(IDD_DIALOG_DOORHOLE);
	pHoleDlgtmp->OnBnClickedOk();
	//delete pHoleDlgtmp;
}
void CDoorHoleDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	m_list_doorhole.DeleteElements();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = m_ehSel.GetModelRef();
	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());
	m_list_doorhole.GetAllRebarData(m_vecReinF);
	/***********************************给sizekey附加型号******************************************************/
	auto it = g_vecRebarData.begin();
	for (; it != g_vecRebarData.end(); it++)
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
	/***********************************给sizekey附加型号******************************************************/
	if (!isArcwall&&m_vecReinF.size()>0)
	{
		std::vector<EditElementHandle*> Holeehs;
		vector<EditElementHandle*> useHoleehs;
		CHoleRebar_ReinforcingDlg::GetUseHoles(eeh, m_FrontPts, Holeehs, useHoleehs, modelRef);
		HoleRFRebarAssembly::IsSmartSmartFeature(eeh);
		if (m_HoleRebar == nullptr)
		{
			ElementId testid = 0;
			GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
			RebarAssembly* rebaras = HoleRFRebarAssembly::GetRebarAssembly(testid, "class DoorHoleRebarAssembly");
			m_HoleRebar = dynamic_cast<DoorHoleRebarAssembly*>(rebaras);
			if (m_HoleRebar == nullptr)
			{
				m_HoleRebar = REA::Create<DoorHoleRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
				m_HoleRebar->SetConcreteOwner(testid);
			}
		}
		if (m_HoleRebar == nullptr)
		{
			m_HoleRebar = REA::Create<DoorHoleRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		}
		m_HoleRebar->ClearData();
		m_HoleRebar->SetTrans(m_trans);
		m_HoleRebar->SetaccConcrete(m_acconcrete);
		m_HoleRebar->SetVecDirSizeData(g_vecRebarData);
		m_HoleRebar->SetrebarPts(m_RebarPts);
		m_HoleRebar->SetvecFrontPts(m_FrontPts);
		m_HoleRebar->SetvecReinF(m_vecReinF);
		m_HoleRebar->SetSelectedElement(m_ehSel.GetElementId());
		m_HoleRebar->SetSelectedModel(m_ehSel.GetModelRef());
		m_HoleRebar->m_useHoleehs.clear();
		m_HoleRebar->m_useHoleehs.insert(m_HoleRebar->m_useHoleehs.begin(), useHoleehs.begin(), useHoleehs.end());
		m_HoleRebar->m_holeidAndmodel.insert(m_holeidAndmodel.begin(), m_holeidAndmodel.end());
		m_HoleRebar->m_NEGholeidAndmodel.insert(m_NEGholeidAndmodel.begin(), m_NEGholeidAndmodel.end());
		m_HoleRebar->m_strElmName = m_nowHolename;
		m_HoleRebar->MakeRebars(modelRef);
		m_HoleRebar->Save(modelRef); // must save after creating rebars
		ElementId contid = m_HoleRebar->FetchConcrete();
		EditElementHandle eeh2(contid, ACTIVEMODEL);
		ElementRefP oldRef = eeh2.GetElementRef();
		mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
		eeh2.ReplaceInModel(oldRef);
		SetElementXAttribute(m_HoleRebar->FetchConcrete(), m_vecReinF, DoorInfoXAttribute, ACTIVEMODEL);
		FreeAll(Holeehs);
	}
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	auto it2 = g_vecRebarData.begin();
	for (; it2 != g_vecRebarData.end(); it2++)
	{
		BrString strRebarSize = it2->rebarSize;
		strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
		strcpy(it2->rebarSize, CT2A(strRebarSize));
	}
	DestroyWindow();
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	//else
	//{
	//	if (m_ArcHoleRebar == nullptr)
	//	{
	//		ElementId testid = 0;
	//		GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	//		RebarAssembly* rebaras = HoleArcRFRebarAssembly::GetRebarAssembly(testid, "class HoleArcRFRebarAssembly");
	//		m_ArcHoleRebar = dynamic_cast<HoleArcRFRebarAssembly*>(rebaras);
	//		if (m_ArcHoleRebar == nullptr)
	//		{
	//			m_ArcHoleRebar = REA::Create<HoleArcRFRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	//		}
	//	}
	//	if (m_ArcHoleRebar == nullptr)
	//	{
	//		m_ArcHoleRebar = REA::Create<HoleArcRFRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	//	}
	//	m_ArcHoleRebar->ClearData();
	//	m_ArcHoleRebar->SetTrans(m_trans);
	//	m_ArcHoleRebar->SetaccConcrete(m_acconcrete);
	//	m_ArcHoleRebar->SetVecDirSizeData(g_vecRebarData);
	//	m_ArcHoleRebar->SetrebarPts(m_RebarPts);
	//	m_ArcHoleRebar->SetvecFrontPts(m_FrontPts);
	//	m_ArcHoleRebar->SetvecReinF(m_vecReinF);
	//	m_ArcHoleRebar->m_holeidAndmodel.insert(m_holeidAndmodel.begin(), m_holeidAndmodel.end());
	//	m_ArcHoleRebar->MakeRebars(modelRef);
	//	m_ArcHoleRebar->Save(modelRef); // must save after creating rebars
	//	ElementId contid = m_ArcHoleRebar->GetConcreteOwner();
	//	EditElementHandle eeh2(contid, ACTIVEMODEL);
	//	ElementRefP oldRef = eeh2.GetElementRef();
	//	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	//	eeh2.ReplaceInModel(oldRef);
	//	SetElementXAttribute(m_ArcHoleRebar->GetConcreteOwner(), m_vecReinF, DoorInfoXAttribute, ACTIVEMODEL);
	//}
}


void CDoorHoleDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	m_list_doorhole.DeleteElements();
	CDialogEx::OnCancel();
	DestroyWindow();
}


void CDoorHoleDlg::OnHdnItemclickListDoorhole(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	CString			strValidChars;
	strValidChars = m_list_doorhole.GetItemText(phdr->iItem, 1);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}
