#include "CFacesRebarDlgEx.h"
// CFacesRebarDlgEx.cpp: 实现文件
//

#include "GalleryIntelligentRebar.h"
#include "afxdialogex.h"
#include "resource.h"

#include "ConstantsDef.h"
#include "FacesRebarAssemblyEx.h"
#include "PITBimMSCEConvert.h"
#include "SelectFaceToolEx.h"

// CFacesRebarDlgEx 对话框

bool g_FacePreviewButtonsDown = false;//面配筋界面的预览按钮

IMPLEMENT_DYNAMIC(CFacesRebarDlgEx, CDialogEx)

CFacesRebarDlgEx::CFacesRebarDlgEx(ElementHandleCR ehOld, ElementId ehnew, const bvector<ISubEntityPtr>& faces, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_FacesRebarEx, pParent), _ehOld(ehOld), _ehNew(ehnew), m_selectfaces(faces), m_ConcreteId(0)
{
	m_FaceRebarPtr = NULL;
	m_vecRebarData.clear();
	m_vecRebarData.shrink_to_fit();
	m_vecEndTypeData.clear();
	m_vecEndTypeData.shrink_to_fit();

	m_isHide = false;

	m_WallSetInfo.rebarType = 2;
}

CFacesRebarDlgEx::~CFacesRebarDlgEx()
{
}

void CFacesRebarDlgEx::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_FacesRebarEx, m_tab);
	DDX_Control(pDX, IDC_COMBO2, m_ComboSize);
	DDX_Control(pDX, IDC_COMBO3, m_ComboType);
	DDX_Control(pDX, IDC_EDIT1, m_EditSpace);
	DDX_Control(pDX, IDC_STATIC_WALLNAME, m_static_wallname);
}

// CFacesRebarDlgEx 消息处理程序
BEGIN_MESSAGE_MAP(CFacesRebarDlgEx, CDialogEx)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CFacesRebarDlgEx::OnCbnSelchangeCombo2)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CFacesRebarDlgEx::OnCbnSelchangeCombo3)
	ON_EN_CHANGE(IDC_EDIT1, &CFacesRebarDlgEx::OnEnChangeEdit1)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_FacesRebarEx, &CFacesRebarDlgEx::OnTcnSelchangeTabFacerebarex)
	ON_STN_CLICKED(IDC_STATIC_WALLNAME, &CFacesRebarDlgEx::OnStnClickedStaticWallname)
	ON_BN_CLICKED(IDC_BUTTON1, &CFacesRebarDlgEx::OnBnClickedButton1)
	ON_BN_CLICKED(IDOK, &CFacesRebarDlgEx::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CFacesRebarDlgEx::OnBnClickedCancel)
END_MESSAGE_MAP()

BOOL CFacesRebarDlgEx::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	if (m_FaceDlgType == 1)
	{
		this->SetWindowText(_T("多板联合配筋"));
	}

	if (!_ehOld.IsValid())
	{
		_ehOld = _ehOlds[0];
	}
	ElementId contid = 0;
	GetElementXAttribute(_ehOld.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, _ehOld.GetModelRef());

	GetElementXAttribute(contid, sizeof(PIT::Concrete), m_Concrete, ConcreteXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	m_vecEndTypeData.clear();
	GetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
	
	m_PageMainRebar.SetHide(m_isHide);
	m_PageMainRebar.SetDlgType(m_FaceDlgType);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	g_vecRebarData = m_vecRebarData;
	m_PageMainRebar.SetSelectfaces(m_selectfaces);
	m_PageEndType.SetListRowData(m_vecEndTypeData);
	m_PageEndType.m_vecRebarData = m_vecRebarData;

	m_PageMainRebar.SavePrt(this);

	string elename, eletype;
	GetEleNameAndType(_ehOld.GetElementId(), _ehOld.GetModelRef(), elename, eletype);
	wall_name = elename;
	CString wallname(elename.c_str());
	wallname = L"墙名:" + wallname;
	m_static_wallname.SetWindowTextW(wallname);

	for each (auto var in g_listRebarSize)
		m_ComboSize.AddString(var);
	for each (auto var in g_listRebarType)
		m_ComboType.AddString(var);

	CString strRebarSize(m_WallSetInfo.rebarSize);
	if (strRebarSize.Find(L"mm") == -1)
		strRebarSize += "mm";
	int nIndex = m_ComboSize.FindStringExact(0, strRebarSize);
	m_ComboSize.SetCurSel(nIndex);//尺寸
	m_ComboType.SetCurSel(m_WallSetInfo.rebarType);//型号

	// TODO:  在此添加额外的初始化
	//为Tab Control增加两个页面
	m_tab.InsertItem(0, _T("主要配筋"));
	m_tab.InsertItem(1, _T("端部样式"));
	//创建两个对话框
	m_PageMainRebar.Create(IDD_DIALOG_FacesRebar_MainRebarEx, &m_tab);
	m_PageMainRebar.SetSelectElement(_ehOld);
	m_PageEndType.Create(IDD_DIALOG_FacesRebar_EndType, &m_tab);

	//设定在Tab内显示的范围
	CRect rc;
	m_tab.GetClientRect(rc);
	rc.top += 20;
	rc.bottom -= 0;
	rc.left += 0;
	rc.right -= 0;
	m_PageMainRebar.MoveWindow(&rc);
	m_PageEndType.MoveWindow(&rc);

	//把对话框对象指针保存起来
	pDialog[0] = &m_PageMainRebar;
	pDialog[1] = &m_PageEndType;

	//显示初始页面
	pDialog[0]->ShowWindow(SW_SHOW);
	pDialog[1]->ShowWindow(SW_HIDE);

	//保存当前选择
	m_CurSelTab = 0;

	g_ConcreteId = m_ConcreteId;

	ACCConcrete wallACConcrete;
	int ret = GetElementXAttribute(_ehOld.GetElementId(), sizeof(ACCConcrete), wallACConcrete, ConcreteCoverXAttribute, _ehOld.GetModelRef());
	if (ret == SUCCESS)	//关联构件配筋时存储过数据,优先使用关联构件设置的保护层
	{
		m_Concrete.postiveCover = wallACConcrete.postiveOrTopCover;
		m_Concrete.reverseCover = wallACConcrete.reverseOrBottomCover;
		m_Concrete.sideCover = wallACConcrete.sideCover;
	}
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CFacesRebarDlgEx::multiSlabOrWallRebar(EditElementHandleR eeh, DgnModelRefP modelRef)
{
}

void CFacesRebarDlgEx::multiDiffThickSlabOrWallRebar(EditElementHandleR eeh, DgnModelRefP modelRef)
{
}

void CFacesRebarDlgEx::perpendicularFaceReabr(vector<double>& vecDis, ElementId & contid, EditElementHandleR eeh, DgnModelRefP modelRef)
{
}

void CFacesRebarDlgEx::normalFaceRebar(ElementId& contid, EditElementHandleR eeh, DgnModelRefP modelRef)
{
	FacesRebarAssemblyEx*  faceRebar = NULL;
	int iIndex = 0;

	vector<ElementRefP> vctAllLines;
	for (ISubEntityPtr face : m_selectfaces)
	{
		EditElementHandle eehFace;
		if (!PIT::ConvertToElement::SubEntityToElement(eehFace, face, modelRef))
		{
			iIndex++;
			continue;
		}
		if (!eehFace.IsValid())
		{
			continue;
		}
		DVec3d ptNormal;
		DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
		mdlElmdscr_extractNormal(&ptNormal, nullptr, eehFace.GetElementDescrCP(), &ptDefault);

		MSElementDescrP newFace = nullptr;
		//eehFace.AddToModel();
		if (abs(ptDefault.DotProduct(ptNormal)) > 0.9/*ptDefault.IsParallelTo(ptNormal)*/)
		{
			ExtractFacesTool::GetOutLineFace(eehFace.GetElementDescrP(), newFace);
		}
		else
		{
			ExtractFacesTool::GetFaceByHoleSubtractFace(eehFace.GetElementDescrP(), newFace);
		}
		if (newFace == nullptr)
		{
			return;
		}
		if (m_Concrete.isHandleHole == 0 && newFace != nullptr)
		{
			//mdlElmdscr_add(newFace);
			eehFace.ReplaceElementDescr(newFace);
		}
		//mdlElmdscr_add(newFace);
		EditElementHandle newFaceEeh(newFace, true, false, ACTIVEMODEL);
		FacesRebarAssemblyEx::FaceType faceType = FacesRebarAssemblyEx::JudgeFaceType(newFaceEeh, modelRef);
		switch (faceType)
		{
		case FacesRebarAssemblyEx::other:
		case FacesRebarAssemblyEx::Plane:
			faceRebar = REA::Create<PlaneRebarAssemblyEx>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			break;
		// case FacesRebarAssemblyEx::CamberedSurface:
		// 	faceRebar = REA::Create<CamberedSurfaceRebarAssemblyEx>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		// 	break;
		default:
			return;
		}
		{
			if (ptDefault.IsParallelTo(ptNormal))
			{
				faceRebar->m_UseXOYDir = true;
			}
		}

		if (g_FacePreviewButtonsDown)
		{
			faceRebar->m_allPreViewEehs.clear();
		}
		CVector3D vecTmp = m_PageMainRebar.GetvecFaceNormal(iIndex);
		DVec3d dvec = DVec3d::From(vecTmp.x, vecTmp.y, vecTmp.z);
		faceRebar->m_face = eehFace;
		faceRebar->m_Solid = &eeh;
		faceRebar->SetfaceType(faceType);
		faceRebar->SetfaceNormal(dvec);
		faceRebar->AnalyzingFaceGeometricData(newFaceEeh);
		faceRebar->SetConcrete(m_Concrete);
		faceRebar->SetMainRebars(m_vecRebarData);
		faceRebar->SetRebarEndTypes(m_vecEndTypeData);
		faceRebar->MakeRebars(modelRef);
		faceRebar->Save(modelRef); // must save after creating rebars
		contid = faceRebar->FetchConcrete();
		iIndex++;
		std::copy(faceRebar->m_allPreViewEehs.begin(), faceRebar->m_allPreViewEehs.end(), std::back_inserter(vctAllLines));//将多个面的画线依次存起来，最后赋值给m_FaceRebarPtr->m_AllLines
		if (newFace != nullptr)
		{
			mdlElmdscr_freeAll(&newFace);
			newFace = nullptr;
		}
	}
	if (g_FacePreviewButtonsDown)
	{
		m_FaceRebarPtr = faceRebar;
		m_FaceRebarPtr->m_allPreViewEehs = vctAllLines;
	}
	if (faceRebar != nullptr)
	{
		SetElementXAttribute(contid, faceRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
	}
}

void CFacesRebarDlgEx::multiFaceInlineRebar(ElementId& contid, EditElementHandleR eeh, DgnModelRefP modelRef)
{
}

void CFacesRebarDlgEx::PreviewRebarLines()
{
	g_FacePreviewButtonsDown = true;
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = _ehOld.GetModelRef();
	EditElementHandle eeh(_ehOld, _ehOld.GetModelRef());

	m_Concrete = m_PageMainRebar.GetConcreteData();
	m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	/*m_PageEndType.GetListRowData(m_vecEndTypeData);	//主要获取端部样式中端部属性的设置的新数据
	m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);	//主要获取端部样式中列表新数据
	m_PageEndType.SetListRowData(m_vecEndTypeData);	//修改为新的数据*/

	// 清空上次的预览线
	if (m_FaceRebarPtr)
	{
		m_FaceRebarPtr->ClearLines();
	}
	/**********************给sizekey附加型号******************************/
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
	/******************************给sizekey附加型号*****************************/

	ElementId testid = 0;
	GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
	vector<double> vecDis;

	// 多面联合配筋 || 多板联合配筋
	if (!m_vvecUpFace.empty() || !m_vvecDownFace.empty())
	{
		//multiSlabOrWallRebar(eeh, modelRef);
		std::vector<PIT::ConcreteRebar> frontRebarDatas;
		std::vector<PIT::ConcreteRebar> backRebarDatas;
		for (auto it : m_vecRebarData)
		{
			if (it.datachange == 0)
			{
				frontRebarDatas.push_back(it);
			}
			if (it.datachange == 2)
			{
				backRebarDatas.push_back(it);
			}
		}
		if (frontRebarDatas.size() > 0)
		{
			m_Concrete.isSlabUpFaceUnionRebar = 1;
			m_Concrete.rebarLevelNum = frontRebarDatas.size();
			m_vecRebarData = frontRebarDatas;
			multiDiffThickSlabOrWallRebar(eeh, modelRef);
		}
		if (backRebarDatas.size() > 0)
		{
			m_Concrete.isSlabUpFaceUnionRebar = 0;
			m_Concrete.rebarLevelNum = backRebarDatas.size();
			m_vecRebarData = backRebarDatas;
			multiDiffThickSlabOrWallRebar(eeh, modelRef);
		}
		g_FacePreviewButtonsDown = false;
		return;
	}

	if (m_selectfaces.size() > 2)
	{
		EditElementHandle eehFace1;
		DPoint3d ptNormal1;
		DPoint3d ptInFace1;
		bool ret1 = PIT::ConvertToElement::SubEntityToElement(eehFace1, m_selectfaces[0], modelRef);
		mdlElmdscr_extractNormal(&ptNormal1, &ptInFace1, eehFace1.GetElementDescrCP(), NULL);
		for (int i = 1; i < (int)m_selectfaces.size() - 1; i += 2)
		{
			EditElementHandle eehFace2, eehFace3;
			bool ret2 = PIT::ConvertToElement::SubEntityToElement(eehFace2, m_selectfaces[i], modelRef);
			bool ret3 = PIT::ConvertToElement::SubEntityToElement(eehFace3, m_selectfaces[i + 1], modelRef);
			if (ret2 && ret3)
			{
				DPoint3d ptNormal2, ptNormal3;
				DPoint3d ptInFace2, ptInFace3;
				mdlElmdscr_extractNormal(&ptNormal2, &ptInFace2, eehFace2.GetElementDescrCP(), NULL);
				mdlElmdscr_extractNormal(&ptNormal3, &ptInFace3, eehFace3.GetElementDescrCP(), NULL);
				if (ptNormal1.IsPerpendicularTo(ptNormal2) && ptNormal1.IsParallelTo(ptNormal3))
				{
					//计算第三个面与第一个面的距离
					DPoint3d ptPro;
					mdlVec_projectPointToPlane(&ptPro, &ptInFace1, &ptInFace3, &ptNormal3);
					double dis = ptInFace1.Distance(ptPro);
					vecDis.push_back(dis);
				}

				ptNormal1 = ptNormal3;
				ptInFace1 = ptInFace3;
			}
		}
	}

	ElementId contid;
	if (vecDis.size()) // 互相垂直面配筋（3个面及以上）
	{
		perpendicularFaceReabr(vecDis, contid, eeh, modelRef);
	}
	else
	{
		if (this->m_PageMainRebar.GetIsMergeRebar() && m_selectfaces.size() > 1) // 多个面
		{
			multiFaceInlineRebar(contid, eeh, modelRef);
		}
		else
		{
			normalFaceRebar(contid, eeh, modelRef); // 一个面
		}

	}

	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);

	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	auto it2 = m_vecRebarData.begin();
	for (; it2 != m_vecRebarData.end(); it2++)
	{
		BrString strRebarSize = it2->rebarSize;
		strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
		strcpy(it2->rebarSize, CT2A(strRebarSize));
	}
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/

	m_PageMainRebar.DeleteFaceLine();
	g_FacePreviewButtonsDown = false;
}

void CFacesRebarDlgEx::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
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



void CFacesRebarDlgEx::OnTcnSelchangeTabFacerebarex(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	//把当前的页面隐藏起来
	pDialog[m_CurSelTab]->ShowWindow(SW_HIDE);
	//得到新的页面索引
	m_CurSelTab = m_tab.GetCurSel();

	switch (m_CurSelTab)
	{
	case 0:
	{
		std::vector<PIT::EndType>	tmpvecEndTypeData;
		m_PageEndType.GetListRowData(tmpvecEndTypeData);
		m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
		for (int i = 0; i < tmpvecEndTypeData.size(); i++)
		{
			if (m_vecEndTypeData.size() >= i + 1)
			{
				m_vecEndTypeData[i].endPtInfo = tmpvecEndTypeData[i].endPtInfo;
			}
		}
		m_PageEndType.SetListRowData(m_vecEndTypeData);
		m_PageMainRebar.UpdateRebarList();
	}
	break;
	case 1:
	{
		m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
		m_PageMainRebar.SetListRowData(m_vecRebarData);
		m_Concrete = m_PageMainRebar.GetConcreteData();
		m_PageEndType.SetRearLevelNum(m_Concrete.rebarLevelNum);
		m_PageEndType.SetListRowData(m_vecEndTypeData);
		m_PageEndType.m_vecRebarData = m_vecRebarData;
		m_PageEndType.UpdateEndTypeList();
	}
	break;
	default:
		break;
	}
	//把新的页面显示出来
	pDialog[m_CurSelTab]->ShowWindow(SW_SHOW);
	*pResult = 0;
}


void CFacesRebarDlgEx::OnStnClickedStaticWallname()
{
	// TODO: 在此添加控件通知处理程序代码
}

void CFacesRebarDlgEx::OnCbnSelchangeCombo3() // 类型
{
	// TODO: 在此添加控件通知处理程序代码
	auto it = g_listRebarType.begin();
	advance(it, m_ComboType.GetCurSel());
	m_WallSetInfo.rebarType = m_ComboType.GetCurSel();
	m_PageMainRebar.ChangeRebarTypedata(m_WallSetInfo.rebarType);
	m_PageMainRebar.UpdateRebarList();
}

void CFacesRebarDlgEx::OnEnChangeEdit1() // 间距
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString	strTemp = CString();
	m_EditSpace.GetWindowText(strTemp);
	m_WallSetInfo.spacing = atof(CT2A(strTemp));

	m_PageMainRebar.ChangeRebarSpacedata(m_WallSetInfo.spacing);
	m_PageMainRebar.UpdateRebarList();
}

void CFacesRebarDlgEx::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	SelectAnchorFaceTool* tool = new SelectAnchorFaceTool(1, 1, _ehOld, _ehNew, this);
	tool->InstallTool();
}


void CFacesRebarDlgEx::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = _ehOld.GetModelRef();
	EditElementHandle eeh(_ehOld, _ehOld.GetModelRef());

	m_Concrete = m_PageMainRebar.GetConcreteData();
	m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	// m_PageEndType.SetListRowData(m_vecEndTypeData);	//主要获取端部样式中端部属性的设置的新数据


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
		else if (XmlManager::s_alltypes.size() > 0)
		{
			strRebarSize = XmlManager::s_alltypes[it->rebarType];
		}
		strcpy(it->rebarSize, CT2A(strRebarSize));
		GetDiameterAddType(it->rebarSize, it->rebarType);
	}
	/***********************************给sizekey附加型号******************************************************/

	ElementId testid = 0;
	GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
	vector<double> vecDis;


	// 多面联合配筋 || 多板联合配筋
	if (!m_vvecUpFace.empty() || !m_vvecDownFace.empty())
	{
		//multiSlabOrWallRebar(eeh, modelRef);

		std::vector<PIT::ConcreteRebar> frontRebarDatas;
		std::vector<PIT::ConcreteRebar> backRebarDatas;
		for (auto it : m_vecRebarData)
		{
			if (it.datachange == 0)
			{
				frontRebarDatas.push_back(it);
			}
			if (it.datachange == 2)
			{
				backRebarDatas.push_back(it);
			}
		}
		if (frontRebarDatas.size() > 0)
		{
			m_Concrete.isSlabUpFaceUnionRebar = 1;
			m_Concrete.rebarLevelNum = frontRebarDatas.size();
			m_vecRebarData = frontRebarDatas;
			multiDiffThickSlabOrWallRebar(eeh, modelRef);
		}
		if (backRebarDatas.size() > 0)
		{
			m_Concrete.isSlabUpFaceUnionRebar = 0;
			m_Concrete.rebarLevelNum = backRebarDatas.size();
			m_vecRebarData = backRebarDatas;
			multiDiffThickSlabOrWallRebar(eeh, modelRef);
		}
		if (m_FaceRebarPtr)
		{
			m_FaceRebarPtr->ClearLines();
		}
		return;
	}

	if (m_selectfaces.size() > 2)
	{
		EditElementHandle eehFace1;
		DPoint3d ptNormal1;
		DPoint3d ptInFace1;
		bool ret1 = PIT::ConvertToElement::SubEntityToElement(eehFace1, m_selectfaces[0], modelRef);
		mdlElmdscr_extractNormal(&ptNormal1, &ptInFace1, eehFace1.GetElementDescrCP(), NULL);
		for (int i = 1; i < (int)m_selectfaces.size() - 1; i += 2)
		{
			EditElementHandle eehFace2, eehFace3;
			bool ret2 = PIT::ConvertToElement::SubEntityToElement(eehFace2, m_selectfaces[i], modelRef);
			bool ret3 = PIT::ConvertToElement::SubEntityToElement(eehFace3, m_selectfaces[i + 1], modelRef);
			if (ret2 && ret3)
			{
				DPoint3d ptNormal2, ptNormal3;
				DPoint3d ptInFace2, ptInFace3;
				mdlElmdscr_extractNormal(&ptNormal2, &ptInFace2, eehFace2.GetElementDescrCP(), NULL);
				mdlElmdscr_extractNormal(&ptNormal3, &ptInFace3, eehFace3.GetElementDescrCP(), NULL);
				if (ptNormal1.IsPerpendicularTo(ptNormal2) && ptNormal1.IsParallelTo(ptNormal3))
				{
					//计算第三个面与第一个面的距离
					DPoint3d ptPro;
					mdlVec_projectPointToPlane(&ptPro, &ptInFace1, &ptInFace3, &ptNormal3);
					double dis = ptInFace1.Distance(ptPro);
					vecDis.push_back(dis);
				}

				ptNormal1 = ptNormal3;
				ptInFace1 = ptInFace3;
			}
		}
	}

	ElementId contid;
	//if (vecDis.size()) // 互相垂直面配筋（3个面及以上) 
	//{
	//	perpendicularFaceReabr(vecDis, contid, eeh, modelRef);
	//}
	//else
	{
		if (this->m_PageMainRebar.GetIsMergeRebar() && m_selectfaces.size() > 1)
		{
			// 多面配筋 且 合并钢筋
			multiFaceInlineRebar(contid, eeh, modelRef);
		}
		else
		{
			// 单面或多面配筋
			normalFaceRebar(contid, eeh, modelRef);
		}

	}

	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);

	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	auto it2 = m_vecRebarData.begin();
	for (; it2 != m_vecRebarData.end(); it2++)
	{
		BrString strRebarSize = it2->rebarSize;
		strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
		strcpy(it2->rebarSize, CT2A(strRebarSize));
	}
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/

	SetConcreteXAttribute(contid, ACTIVEMODEL);
	//SetElementXAttribute(contid, sizeof(PIT::WallRebarInfo), &m_Concrete, WallRebarInfoXAttribute, ACTIVEMODEL);
	//SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(_ehOld.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, _ehOld.GetModelRef());
	SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
	ElementId unionId = -1;
	GetElementXAttribute(_ehOld.GetElementId(), sizeof(ElementId), unionId, UnionWallIDXAttribute, _ehOld.GetModelRef());
	if (unionId != -1)
	{
		SetElementXAttribute(unionId, sizeof(ElementId), &contid, ConcreteIDXAttribute, ACTIVEMODEL);
	}

	m_PageMainRebar.DeleteFaceLine();

	if (m_FaceRebarPtr)
	{
		m_FaceRebarPtr->ClearLines();
	}
	SelectFaceToolEx* tool = new SelectFaceToolEx(1, 1, _ehOld, _ehNew);
	tool->InstallTool();
}


void CFacesRebarDlgEx::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	if (m_FaceRebarPtr)
	{
		m_FaceRebarPtr->ClearLines();
	}
	m_PageMainRebar.DeleteFaceLine();
}
