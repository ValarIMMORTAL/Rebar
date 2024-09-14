// CStretchStirrupRebarToolDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CStretchStirrupRebarToolDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include "SingleRebarAssembly.h"
#include "ExtractFacesTool.h"
#include "SelectRebarTool.h"
#include "BentlyCommonfile.h"
#include "ReadRebarTool.h"

// CStretchStirrupRebarToolDlg 对话框

IMPLEMENT_DYNAMIC(CStretchStirrupRebarToolDlg, CDialogEx)

CStretchStirrupRebarToolDlg::CStretchStirrupRebarToolDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_STRETCHSTIRRUPREBAR, pParent)
{

}

CStretchStirrupRebarToolDlg::~CStretchStirrupRebarToolDlg()
{
}

void CStretchStirrupRebarToolDlg::SaveRebarPts(vector< vector<CPoint3D>>& Vctpts)
{
	m_vctpts = Vctpts;
	for (int x = 0; x < m_vctpts.size(); x++)
	{
		vector<CPoint3D>& vctpts = m_vctpts[x];
		vctpts.erase(vctpts.begin());//删除第一个点
		vctpts.push_back(vctpts[0]);//将第一个点再存储一次
	}
}

void CStretchStirrupRebarToolDlg::SaveRebarData(BrString& RebarSize, vector <ElementId>&vecElm)
{
	strcpy(m_SizeKey, RebarSize);
	m_vecElm_H = vecElm;
}

void CStretchStirrupRebarToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_ComboType);
	DDX_Control(pDX, IDC_COMBO_RABARDIA, m_ComboSize);
	DDX_Control(pDX, IDC_DIRECTION, m_ComboDirection);
}


BOOL CStretchStirrupRebarToolDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	//SetWindowText("箍筋拉伸");

	for each (auto var in g_listRebarType)
		m_ComboType.AddString(var);

	for each (auto var in g_listRebarSize)
		m_ComboSize.AddString(var);

	m_ComboDirection.InsertString(0, CString("正向"));
	m_ComboDirection.InsertString(1, CString("反向"));
	m_ComboDirection.SetCurSel(0);

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
			strcpy(m_rebarSize, CT2A(RebarSize));
		}
		int nIndex = m_ComboSize.FindStringExact(0, RebarSize);
		m_ComboSize.SetCurSel(nIndex);//尺寸
		m_ComboType.SetCurSel(m_rebarType);//型号
	}
	GetDlgItem(IDC_EDIT)->SetWindowText(L"0");

	PreviewLines();

	return TRUE;
}

void CStretchStirrupRebarToolDlg::PreviewLines()
{
	//清空已有线条
	clearLines();
	//获取比例
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	//设置端部样式135°
	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kCog);
	endTypeEnd.SetType(RebarEndType::kCog);
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
	BrString strRebarSize = m_SizeKey;
	double endLen = PIT::PITRebarCode::GetBendLength(strRebarSize, endTypeStart, ACTIVEMODEL);;
	double bendRadius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);

	for (int i = 0; i < m_vctpts.size(); i++)
	{

		//CVector3D endNormal = CVector3D::From(0, 0, 0);
		//计算顶点
		//PIT::PITRebarCurve  curveTmp;
		//curveTmp.makeStirrupURebarWithNormal(m_vctpts[i], bendRadius, endLen * uor_per_mm, endTypes, endNormal);
		//获取顶点
		//RebarVertices  vers = curveTmp.PopVertices();

		RebarVertices  vers;
		bvector<DPoint3d> allpts;
		for (auto it : m_vctpts[i])
		{
			allpts.push_back(it);
		}
		GetRebarVerticesFromPoints(vers, allpts, 0);

		std::vector<ElementRefP> lineEehs;
		EditElementHandle lineEeh;
		//创建一个新的复杂线条或形状的EditElementHandle的链式结构，返回该链式结构的头部
		//在创建链头之后，应用程序应该使用AddComponentElement向链添加开放曲线元素，
		//并在添加完所有组件之后使用AddComponentComplete完成链的创建。
		ChainHeaderHandler::CreateChainHeaderElement(lineEeh, NULL, false, true, *ACTIVEMODEL);
		//根据顶点绘制线条组合
		for (int i = 0; i < (int)vers.GetSize() - 1; i++)
		{
			RebarVertexP ver = vers.GetAt(i); //当前顶点
			RebarVertexP ver_next = vers.GetAt(i + 1); //下一个顶点

			// 当前点是弧线
			if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) != 0)
			{
				EditElementHandle eehArc;
				//画圆弧
				if (SUCCESS != ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(ver->GetCenter(), ver->GetArcPt()[0], ver->GetArcPt()[2]), (ACTIVEMODEL)->Is3d(), *ACTIVEMODEL))
				{
					return;
				}
				eehArc.AddToModel();
				lineEehs.push_back(eehArc.GetElementRef());
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
				eehTmp.AddToModel();
				lineEehs.push_back(eehTmp.GetElementRef());
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
				eehTmp.AddToModel();
				lineEehs.push_back(eehTmp.GetElementRef());
				ChainHeaderHandler::AddComponentElement(lineEeh, eehTmp);
			}
		}
		//完成线条链的创建
		ChainHeaderHandler::AddComponentComplete(lineEeh);
		//绘制
		//lineEeh.AddToModel();
		//m_allLines.push_back(lineEeh.GetElementRef());
		m_allLines.push_back(lineEehs);
	}
}

void CStretchStirrupRebarToolDlg::clearLines()
{
	for (std::vector<ElementRefP> tmpeehs : m_allLines)
	{
		for (ElementRefP tmpeeh : tmpeehs)
		{
			if (tmpeeh != nullptr)
			{
				EditElementHandle eeh(tmpeeh, tmpeeh->GetDgnModelP());
				eeh.DeleteFromModel();
			}
		}		
	}
	m_allLines.clear();
}

void CStretchStirrupRebarToolDlg::ProcessRebarData()
{
	RebarSymbology rebarSymb; //钢筋符号
	//设置颜色
	SetRebarColorBySize(m_rebarSize, rebarSymb);
	//设置level
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
	//把rebarsize和type拼起来
	GetDiameterAddType(rebarData.rebarSize, nType);

	for (int x = 0; x < m_vctpts.size(); x++)
	{
		//构造箍筋并保存
		m_pStirrupRebars.push_back(shared_ptr<PIT::StirrupRebar>(new PIT::StirrupRebar(m_vctpts[x], rebarData)));
	}
}

void CStretchStirrupRebarToolDlg::SetEditLinePoint(DPoint3dCR startPoint, DPoint3dCR endPoint)
{
	//拿到与点1相交但不与点2相交的线
	if (m_allLines.size() == 0)
		return;

	DVec3d vec; //移动方向向量
	for (ElementRefP tmpeeh : m_allLines[0])
	{
		if (tmpeeh != nullptr)
		{
			EditElementHandle ehLine(tmpeeh, tmpeeh->GetDgnModelP());
			CurveVectorPtr lineCurve = ICurvePathQuery::ElementToCurveVector(ehLine);
			DSegment3dCP segment1 = nullptr;
			if (lineCurve.IsValid())
			{
				for (ICurvePrimitivePtr linePrimitive : *lineCurve)
				{
					segment1 = linePrimitive->GetLineCP();
					if (segment1 != nullptr)
					{
						DPoint3d pt1, pt2;
						segment1->GetEndPoints(pt1, pt2);
						if ((pt1.IsEqual(startPoint) && !pt2.IsEqual(endPoint)) ||
							pt2.IsEqual(startPoint) && !pt1.IsEqual(endPoint))
						{
							vec = pt2 - pt1;
							vec.Normalize();
							break;
						}
					}
				}		
			}		
			
		}
	}
	if (!m_dirIsPositive)
	{
		vec.Negate();
	}
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	vec.ScaleToLength(m_stretchLen * uor_per_mm);
	//找到需要变化的在箍筋中的位置
	int pt1_index = -1, pt2_index = -1, pt3_index = -1;
	for (int x = 0; x < m_vctpts.size(); x++)
	{
		std::vector<CPoint3D> pts = m_vctpts[x];
		auto it = std::find(pts.begin(), pts.end(), CPoint3D(startPoint));
		if (it != pts.end())
		{
			pt1_index = it - pts.begin();
			it = std::find(pts.begin(), pts.end(), CPoint3D(endPoint));
			pt2_index = it - pts.begin();
			if (pt1_index == 0 || pt2_index == 0)
				pt3_index = pts.size() - 1;
			break;
		}
	}
	//沿向量移动点
	for (int x = 0; x < m_vctpts.size(); x++)
	{
		std::vector<CPoint3D> pts = m_vctpts[x];
		if (pt1_index != -1)
		{
			DPoint3d tmp = DPoint3d(pts[pt1_index]);
			tmp.Add(vec);
			pts[pt1_index] = tmp;
		}
		if (pt2_index != -1)
		{
			DPoint3d tmp = DPoint3d(pts[pt2_index]);
			tmp.Add(vec);
			pts[pt2_index] = tmp;
		}
		if (pt3_index != -1)
		{
			DPoint3d tmp = DPoint3d(pts[pt3_index]);
			tmp.Add(vec);
			pts[pt3_index] = tmp;
		}
		m_vctpts[x] = pts;
	}
}

void CStretchStirrupRebarToolDlg::OnEnChangeStretchLen()
{
	CString strStretchLen;
	GetDlgItem(IDC_EDIT)->GetWindowTextW(strStretchLen);
	m_stretchLen = atof(CT2A(strStretchLen));
}

void CStretchStirrupRebarToolDlg::OnBnClickedPreview()
{
	/*
	for (int x = 0; x < m_vctpts.size(); x++)
	{
		vector<CPoint3D>& vctpts = m_vctpts[x];
		if (strStretchDir == "X")
		{
			vctpts[2].x += m_stretchLen * uor_per_mm;
			vctpts[3].x += m_stretchLen * uor_per_mm;
			continue;
		}
		if (strStretchDir == "Y")
		{
			vctpts[2].y += m_stretchLen * uor_per_mm;
			vctpts[3].y += m_stretchLen * uor_per_mm;
			continue;
		}
		if (strStretchDir == "-X")
		{
			vctpts[0].x -= m_stretchLen * uor_per_mm;
			vctpts[1].x -= m_stretchLen * uor_per_mm;
			vctpts[4].x -= m_stretchLen * uor_per_mm;
			continue;
		}
		if (strStretchDir == "-Y")
		{
			vctpts[0].y -= m_stretchLen * uor_per_mm;
			vctpts[1].y -= m_stretchLen * uor_per_mm;
			vctpts[4].y -= m_stretchLen * uor_per_mm;
			continue;
		}
	}
	*/
	PreviewLines();
}

BEGIN_MESSAGE_MAP(CStretchStirrupRebarToolDlg, CDialogEx)
	ON_EN_CHANGE(IDC_EDIT, &CStretchStirrupRebarToolDlg::OnEnChangeStretchLen)
	ON_BN_CLICKED(IDOK, &CStretchStirrupRebarToolDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CStretchStirrupRebarToolDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDPREVIEW, &CStretchStirrupRebarToolDlg::OnBnClickedPreview)
	ON_BN_CLICKED(IDCHOOSEEDITLINE, &CStretchStirrupRebarToolDlg::OnBnClickedChooseEditLine)
	ON_CBN_SELCHANGE(IDC_DIRECTION, &CStretchStirrupRebarToolDlg::OnCbnSelchangeDirection)
	ON_CBN_SELCHANGE(IDC_COMBO_RABARDIA, &CStretchStirrupRebarToolDlg::OnCbnSelchangeComboRabardia)
END_MESSAGE_MAP()


// CStretchStirrupRebarToolDlg 消息处理程序


void CStretchStirrupRebarToolDlg::OnBnClickedCancel()
{
	clearLines();
	CDialogEx::OnCancel();
	DestroyWindow();
}

void CStretchStirrupRebarToolDlg::OnBnClickedOk()
{
	ProcessRebarData();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	vector<ElementId> vecElementID;
	vecElementID.push_back(0);

	ElementId conid = 0; //混凝土id
	RebarSetP tmpset = nullptr; //钢筋集合  
	if (m_vecElm_H.size() == 0)
	{
		return;
	}
	EditElementHandle tmprebar(m_vecElm_H[0], ACTIVEMODEL);
	for (size_t i = 0; i < m_vecElm_H.size(); i++)
	{
		ElementHandle eh(m_vecElm_H[i], modelRef);
		if (RebarElement::IsRebarElement(eh))
		{
			RebarElementP rep = RebarElement::Fetch(eh);
			RebarSetP pRebarSet = rep->GetRebarSet(modelRef); //钢筋所在的集合
			if (pRebarSet != nullptr)
			{
				if (i == 0)
				{
					tmpset = pRebarSet;
				}
				int rebar_cage_type = 0;
				conid = pRebarSet->GetConcreteOwner(ACTIVEMODEL, rebar_cage_type);
				if (conid != 0)
					break;
			}
		}
	}


	RebarSetTagArray rsetTags; //钢筋集合的标签集合
	RebarSetP rebarSet = RebarSet::Fetch(vecElementID[0], modelRef);
	if (rebarSet == nullptr)
	{
		return;
	}
	//设置钢筋显示模式为圆柱体
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(vecElementID[0]);
	rebarSet->StartUpdate(modelRef);
	int rebarNum = (int)m_pStirrupRebars.size(); //构建的箍筋数量
	for (int i = 0; i < rebarNum; ++i)
	{
		//绘制箍筋
		m_pStirrupRebars[i]->Create(*rebarSet);
	}

	RebarSetTag *tag = new RebarSetTag; //钢筋集合标签
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
		RebarAssemblies reas; //钢筋构建集合
		RebarAssembly::GetRebarAssemblies(conid, reas);
		RebarAssembly *rebarasb = reas.GetAt(0); //钢筋构建
		if (rebarasb != nullptr)
		{

			SingleRebarAssembly*  rebarAsm = REA::Create<SingleRebarAssembly>(modelRef);
			//获取被选中的元素id
			ElementId tmpid = rebarasb->GetSelectedElement(); 
			if (tmpid == 0)
			{
				return;
			}
			//获取被选中的模型
			DgnModelRefP modelp = rebarasb->GetSelectedModel();
			EditElementHandle ehSel;
			if (modelp == nullptr)
			{
				//在当前模型中找对应id的元素
				if (ehSel.FindByID(tmpid, ACTIVEMODEL) != SUCCESS)	//没有找到
				{
					//获取当前dgn里的模型集合
					ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
					for (DgnModelRefP modelRef : modelRefCol) //遍历所有模型找有对应id元素的模型
					{
						if (ehSel.FindByID(tmpid, modelRef) == SUCCESS)
						{
							modelp = modelRef;
							break;
						}

					}
				}
			}
			else //在选中模型中找元素
			{
				ehSel.FindByID(tmpid, modelp);
			}

			//设置构建需要的信息
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

	//清除预览线
	clearLines();
	CDialogEx::OnOK();
	DestroyWindow();
	mdlInput_sendSynchronizedKeyin(L"proconcrete delete rebar", 0, INPUTQ_EOQ, NULL);
	mdlInput_sendSynchronizedKeyin(L"choose element", 0, INPUTQ_EOQ, NULL);
}


void CStretchStirrupRebarToolDlg::OnBnClickedChooseEditLine()
{
	PITStretchStirrupRebarTool::InstallStretchStirrupRebarInstance(0, this);
}


void CStretchStirrupRebarToolDlg::OnCbnSelchangeDirection()
{
	int index = m_ComboDirection.GetCurSel();
	if (index == 0)
	{
		m_dirIsPositive = true;
	}
	else
	{
		m_dirIsPositive = false;
	}
}


void CStretchStirrupRebarToolDlg::OnCbnSelchangeComboRabardia()
{
	int nIndex = m_ComboSize.GetCurSel();
	CString rebarDiaStr;
	m_ComboSize.GetLBText(nIndex, rebarDiaStr);
	strcpy(m_rebarSize, CT2A(rebarDiaStr));
}
