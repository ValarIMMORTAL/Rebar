#include "CFacesRebarDlgEx.h"
// CFacesRebarDlgEx.cpp: 实现文件
//

#include "GalleryIntelligentRebar.h"
#include "afxdialogex.h"
#include "resource.h"

#include "ConstantsDef.h"
#include "PITBimMSCEConvert.h"

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
	//GetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);

	m_PageMainRebar.SetHide(m_isHide);
	m_PageMainRebar.SetDlgType(m_FaceDlgType);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	g_vecRebarData = m_vecRebarData;
	m_PageMainRebar.SetSelectfaces(m_selectfaces);

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
	//创建两个对话框
	m_PageMainRebar.Create(IDD_DIALOG_FacesRebar_MainRebarEx, &m_tab);
	m_PageMainRebar.SetSelectElement(_ehOld);

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
	//*pResult = 0;
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
	SelectFaceTool3* tool = new SelectFaceTool3(1, 1, _ehOld, _ehNew, this);
	tool->InstallTool();
}


void CFacesRebarDlgEx::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	mdlDialog_dmsgsPrint(L"面配筋出图");
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
