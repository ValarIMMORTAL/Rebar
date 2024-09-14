// CDomeRebarMainDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CDomeRebarMainDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ElementAttribute.h"
#include "ConstantsDef.h"


// CDomeRebarMainDlg 对话框

IMPLEMENT_DYNAMIC(CDomeRebarMainDlg, CDialogEx)

CDomeRebarMainDlg::CDomeRebarMainDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_DomeMainDlg, pParent)
{
	m_CurSelTab = 0;

	m_circleCenter = DPoint3d::From(0, 0, 0);
	m_stDomeCoverInfo.dCover = 30.0;
	m_stDomeCoverInfo.eehId = 0;
}

CDomeRebarMainDlg::~CDomeRebarMainDlg()
{
}

void CDomeRebarMainDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

BOOL CDomeRebarMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端

	ElementId contid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	if (contid > 0)
	{
		vector<PIT::DomeLevelDetailInfo> vecDomeLevelDetailInfo;
		GetElementXAttribute(contid, m_vecDomeLevelInfo, vecDomeLevelAttribute, ACTIVEMODEL);
		GetElementXAttribute(contid, vecDomeLevelDetailInfo, vecDomeDetailLevelAttribute, ACTIVEMODEL);
		GetElementXAttribute(contid, sizeof(m_stDomeCoverInfo), m_stDomeCoverInfo, stDomeCoverInfoAttribute, ACTIVEMODEL);

		m_mapDomeLevelDetailInfo.clear();
		for (PIT::DomeLevelDetailInfo stLevelDetailInfo : vecDomeLevelDetailInfo)
		{
			auto itr = m_mapDomeLevelDetailInfo.find(stLevelDetailInfo.nNumber);
			if (itr == m_mapDomeLevelDetailInfo.end())
			{
				vector<PIT::DomeLevelDetailInfo> vecTemp;
				vecTemp.push_back(stLevelDetailInfo);
				m_mapDomeLevelDetailInfo.insert(make_pair(stLevelDetailInfo.nNumber, vecTemp));
			}
			else
			{
				itr->second.push_back(stLevelDetailInfo);
			} 
		}

		m_pageDomeRebarDlg.SetCover(m_stDomeCoverInfo.dCover);
		m_pageDomeRebarDlg.SetNumber((int)m_vecDomeLevelInfo.size());
		m_pageDomeRebarDlg.SetvecDomeLevelInfo(m_vecDomeLevelInfo);
		m_pageDomeRebarDlg.SetmapDomeLevelDetailInfo(m_mapDomeLevelDetailInfo);
	}

	m_tab.InsertItem(0, _T("主要配筋"));

	m_pageDomeRebarDlg.SetSelectElement(m_ehSel);
	m_pageDomeRebarDlg.Create(IDD_DIALOG_DomeRebarDlg, &m_tab);

	//设定在Tab内显示的范围
	CRect rc;
	m_tab.GetClientRect(rc);
	rc.top += 20;
	rc.bottom -= 0;
	rc.left -= 20;
	rc.right -= 0;
	m_pageDomeRebarDlg.MoveWindow(&rc);

	//把对话框对象指针保存起来
	pDialog[0] = &m_pageDomeRebarDlg;

	//显示初始页面
	pDialog[0]->ShowWindow(SW_SHOW);

	//保存当前选择
	m_CurSelTab = 0;

	return true;
}

void CDomeRebarMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_tab);
}


BEGIN_MESSAGE_MAP(CDomeRebarMainDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDomeRebarMainDlg::OnBnClickedOk)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CDomeRebarMainDlg::OnTcnSelchangeTab1)
	ON_BN_CLICKED(IDCANCEL, &CDomeRebarMainDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CDomeRebarMainDlg 消息处理程序


void CDomeRebarMainDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();

	// m_pageDomeRebarDlg.DeleteRound();
	m_pageDomeRebarDlg.GetCover(m_stDomeCoverInfo.dCover);
	m_pageDomeRebarDlg.GetCircleCenter(m_circleCenter);
	m_pageDomeRebarDlg.GetScaleLength(m_dScaleLength);
	m_pageDomeRebarDlg.GetvecVertex(m_vecVertex);
	m_pageDomeRebarDlg.GettargetTrans(m_targetTrans);

	if (m_vecVertex.size() == 0)
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"该模型没有包含穹顶配筋所需数据，请使用模型组装导出的模型", MessageBoxIconType::Warning);
		return;
	}

	m_pageDomeRebarDlg.GetvecDomeLevelInfo(m_vecDomeLevelInfo);
	m_pageDomeRebarDlg.GetmapDomeLevelDetailInfo(m_mapDomeLevelDetailInfo);

	DgnModelRefP        modelRef = ACTIVEMODEL;
	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());

	// m_pageDomeRoundDlg.m_listCtl.GetAllRebarData(m_vecRoundDomeLevel);
	// m_pageDomeRoundDlg.GetstRoundDomeInfo(m_stRoundDomeInfo);

	DPoint3d minPt, maxPt;
	mdlElmdscr_computeRange(&minPt, &maxPt, eeh.GetElementDescrP(), NULL); // 计算指定元素描述符中元素的范围
	EditElementHandle eehSolid(m_stDomeCoverInfo.eehId, ACTIVEMODEL);
	eeh.Duplicate(eehSolid);
	eeh.SetModelRef(eehSolid.GetModelRef());
	if (!eehSolid.IsValid())
	{
		double dHight = maxPt.z - minPt.z;

		//以两个点绘制矩形底面
		DPoint3d ptShape[4];
		ptShape[0] = maxPt;
		ptShape[1] = minPt;
		ptShape[1].y = ptShape[0].y;
		ptShape[2] = minPt;
		ptShape[3] = maxPt;
		ptShape[3].y = minPt.y;
		
		ptShape[0].z = minPt.z;
		ptShape[1].z = minPt.z;
		ptShape[2].z = minPt.z;
		ptShape[3].z = minPt.z;
		EditElementHandle eehMaxShape;
		ShapeHandler::CreateShapeElement(eehMaxShape, NULL, ptShape, 4, true, *ACTIVEMODEL);
		ISolidKernelEntityPtr ptarget;
		SolidUtil::Convert::ElementToBody(ptarget, eehMaxShape);
		SolidUtil::Modify::ThickenSheet(ptarget, dHight, 0.0);
		if (SUCCESS != SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL))
		{
			return;
		}

		eeh.Duplicate(eehSolid);
		eeh.SetModelRef(eehSolid.GetModelRef());
		eeh.AddToModel();

		m_stDomeCoverInfo.eehId = eeh.GetElementId();
	}

	ElementId testid = 0;
	GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());

	RebarAssembly* rebaras = PIT::PITRebarAssembly::GetRebarAssembly(testid, "class CDomeRebarAssembly");
	CDomeRebarAssembly* pDomeRebarAssembly = nullptr;
	pDomeRebarAssembly = dynamic_cast<CDomeRebarAssembly*>(rebaras);
	if (pDomeRebarAssembly == nullptr)
	{
		pDomeRebarAssembly = REA::Create<CDomeRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	}

	minPt.x = maxPt.x;
	minPt.z = maxPt.z;
	pDomeRebarAssembly->SetCover(m_stDomeCoverInfo.dCover);
	pDomeRebarAssembly->SetdomeRadius(minPt.Distance(maxPt) * 0.5);
	pDomeRebarAssembly->SetdomeHight(m_dScaleLength);
	pDomeRebarAssembly->SetCircleCenter(m_circleCenter);
	pDomeRebarAssembly->SetvecDomeLevelInfo(m_vecDomeLevelInfo);
	pDomeRebarAssembly->SetmapDomeLevelDetailInfo(m_mapDomeLevelDetailInfo);
	pDomeRebarAssembly->SetCuteFacePoint(m_vecVertex);
	pDomeRebarAssembly->GetDomeFeatureParam(eeh, maxPt);
	pDomeRebarAssembly->MakeRebars(modelRef);
	pDomeRebarAssembly->Save(modelRef);

	ElementId contid = pDomeRebarAssembly->FetchConcrete();
	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);

	EditElementHandle eeh3(eeh.GetElementId(), ACTIVEMODEL);
	mdlElmdscr_setVisible(eeh3.GetElementDescrP(), false);
	oldRef = eeh.GetElementRef();
	eeh3.ReplaceInModel(oldRef);

	SetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	SetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, eeh.GetModelRef());
	UInt32 xAttId = 1111;
	SetElementXAttribute(eeh.GetElementId(), m_vecVertex, xAttId, eeh.GetModelRef());
	UInt32 xAttId2 = 1112;
	mdlTMatrix_getIdentity(&m_targetTrans);
	SetElementXAttribute(eeh.GetElementId(), (int)sizeof(Transform), &m_targetTrans, xAttId2, eeh.GetModelRef());

	vector<PIT::DomeLevelDetailInfo> vecDomeLevelDetailInfo;
	for (auto itr = m_mapDomeLevelDetailInfo.begin(); itr != m_mapDomeLevelDetailInfo.end(); itr++)
	{
		vecDomeLevelDetailInfo.insert(vecDomeLevelDetailInfo.end(), itr->second.begin(), itr->second.end());
	}
	SetElementXAttribute(contid, m_vecDomeLevelInfo, vecDomeLevelAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, vecDomeLevelDetailInfo, vecDomeDetailLevelAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, sizeof(m_stDomeCoverInfo), &m_stDomeCoverInfo, stDomeCoverInfoAttribute, ACTIVEMODEL);
	DestroyWindow();
}


void CDomeRebarMainDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码

	pDialog[m_CurSelTab]->ShowWindow(SW_HIDE);
	//得到新的页面索引
	m_CurSelTab = m_tab.GetCurSel();

	//把新的页面显示出来
	pDialog[m_CurSelTab]->ShowWindow(SW_SHOW);
	*pResult = 0;
}


void CDomeRebarMainDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();

	m_pageDomeRebarDlg.DeleteRound();
	DestroyWindow();

	return;
}
