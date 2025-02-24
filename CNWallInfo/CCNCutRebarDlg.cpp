// D:\work\02代码\CGNdllCode\GalleryIntelligentRebar\CNWallInfo\CCNCutRebarDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CCNCutRebarDlg.h"
#include "afxdialogex.h"
#include "../../resource.h"
#include "../../ConstantsDef.h"
#include "../../CommonFile.h"
#include "../../AssociatedComponent.h"
#include "ElementAttribute.h"
#include "../../PITRebarCurve.h"
#include "CCNCutRebarAssembly.h"
#include "../PITACCRebarAssembly.h"

// CCNCutRebarDlg 对话框

IMPLEMENT_DYNAMIC(CCNCutRebarDlg, CDialogEx)

CCNCutRebarDlg::CCNCutRebarDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CNCutRebarDlg, pParent)
{
	m_nIndex = 2;
	m_rebarLength = 0.0;
}

CCNCutRebarDlg::~CCNCutRebarDlg()
{
}

BOOL CCNCutRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CString strTemp;
	strTemp.Format(_T("%d"), m_nIndex);
	m_EditNumber.SetWindowText(strTemp);

	LONG lStyle;
	lStyle = GetWindowLong(m_ListCutRebar.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_ListCutRebar.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_ListCutRebar.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_ListCutRebar.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_ListCutRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_ListCutRebar.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_ListCutRebar.InsertColumn(0, _T("截断序号"), (int)(width / 2 - 50), ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_ListCutRebar.InsertColumn(1, _T("长度"), (int)(width / 2 + 50), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

	if (m_selectrebars.size() > 0)
	{
		EditElementHandle eehRebar(m_selectrebars.at(0), ACTIVEMODEL);
		RebarElementP rebarPtr = RebarElement::Fetch(eehRebar);
		if (rebarPtr != NULL)
		{
			m_rebarLength = CalaRebarLength(rebarPtr, ACTIVEMODEL);
		}
	}

	UpdateCutList();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

double CCNCutRebarDlg::CalaRebarLength(RebarElementP pRebar, DgnModelRefP modelRef)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dLength = 0.0;
	if (pRebar == NULL)
	{
		return dLength;
	}

	RebarShape* rebarshape = pRebar->GetRebarShape(modelRef);
	if (rebarshape == nullptr)
	{
		return dLength;
	}

	Transform trans;
	mdlTMatrix_getIdentity(&trans);
	CurveVectorPtr profile = CurveVector::Create(CurveVector::BOUNDARY_TYPE_None);
	rebarshape->GenerateCenterline(*profile, uor_per_mm, &trans);

	for (unsigned int i = 0; i < profile->size(); i++)
	{
		ICurvePrimitivePtr priPtr = profile->at(i);

		if (priPtr->GetCurvePrimitiveType() == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Line)
		{
			DSegment3dCP seg = priPtr->GetLineCP();
			dLength += seg->Length();
		}
		else if (priPtr->GetCurvePrimitiveType() == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_LineString)
		{
			bvector<DPoint3d> const* ptr = priPtr->GetLineStringCP();
			for (int j = 0; j < (int)ptr->size() - 1; j++)
			{
				DPoint3d ptStr, ptEnd;
				ptStr = ptr->at(j);
				ptEnd = ptr->at(j + 1);
				dLength += ptStr.Distance(ptEnd);
			}
		}
		else if (priPtr->GetCurvePrimitiveType() == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Arc)
		{
			DEllipse3dCP ptr = priPtr->GetArcCP();
			dLength += ptr->ArcLength();
		}
	}

	return dLength / uor_per_mm;
}

void CCNCutRebarDlg::SetListDefaultData()
{
	if (m_vecCutInfo.empty())//无数据时根据层数添加默认数据
	{
		if (m_nIndex > 0)
		{
			for (int i = 0; i < m_nIndex; ++i)
			{
				double dTemp = 8950.0;
				if (COMPARE_VALUES_EPS(dTemp, m_rebarLength, EPS) > 0)
				{
					dTemp = 5980.0;
					if (COMPARE_VALUES_EPS(dTemp, m_rebarLength, EPS) > 0)
					{
						dTemp = 3980.0;
						if (COMPARE_VALUES_EPS(dTemp, m_rebarLength, EPS) > 0)
						{
							dTemp = m_rebarLength / 2.0;
						}
					}
				}
				CNCutRebarInfo oneCutInfo;
				oneCutInfo = { i + 1, dTemp, 0 };

				m_vecCutInfo.push_back(oneCutInfo);
			}
		}

	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = m_nIndex - (int)m_vecCutInfo.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				CNCutRebarInfo oneCutInfo = { (int)m_vecCutInfo.size() + 1, 3950, 0};
				m_vecCutInfo.push_back(oneCutInfo);
			}
		}
		if (iOffset < 0)
		{
			iOffset *= -1;
			for (int i = 0; i < iOffset; i++)
			{
				m_vecCutInfo.pop_back();
			}
		}
	}

	double curLength = 0.0;
	for (CNCutRebarInfo stCutInfo : m_vecCutInfo)
	{
		curLength += stCutInfo.dLength;
	}

	m_vecCutInfo.push_back({ (int)m_vecCutInfo.size() + 1, m_rebarLength - curLength, 0 });
}


void CCNCutRebarDlg::UpdateCutList()
{
	m_ListCutRebar.DeleteAllItems();
	SetListDefaultData();
	for (int i = 0; i < m_vecCutInfo.size(); ++i)
	{
		CString strValue;

		strValue.Format(_T("%dL"), m_vecCutInfo[i].nIndex);

		m_ListCutRebar.InsertItem(i, _T("")); // 插入行
		m_ListCutRebar.SetItemText(i, 0, strValue);
		int nSubCnt = m_ListCutRebar.GetColumnCount() - 1;
		for (int j = 1; j <= nSubCnt; ++j)
		{
			CString strList;
			switch (j)
			{
			case 1:
			{
				strList.Format(_T("%.2f"), m_vecCutInfo.at(i).dLength);
			}
			break;
			case 2:
			{
				ListCtrlEx::CStrList strlist = g_listCNCutVec;
				m_ListCutRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecCutInfo[i].nVecType);
				strList = *it;
			}
			break;
			default:
				break;
			}
			m_ListCutRebar.SetItemText(i, j, strList);
		}
	}
}


void CCNCutRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_EditNumber);
	DDX_Control(pDX, IDC_LIST1, m_ListCutRebar);
	DDX_Control(pDX, IDC_CHECK1, m_CheckReserve); // 是否反向截断
}


BEGIN_MESSAGE_MAP(CCNCutRebarDlg, CDialogEx)
ON_EN_KILLFOCUS(IDC_EDIT1, &CCNCutRebarDlg::OnEnKillfocusEdit1)
ON_BN_CLICKED(IDOK, &CCNCutRebarDlg::OnBnClickedOk)
ON_BN_CLICKED(IDC_CHECK1, &CCNCutRebarDlg::OnBnClickedCheck1)
END_MESSAGE_MAP()

void CCNCutRebarDlg::OnEnKillfocusEdit1()
{
	// TODO: 在此添加控件通知处理程序代码

	CString strNumber;
	m_EditNumber.GetWindowText(strNumber);
	m_nIndex = atof(CT2A(strNumber));

	m_ListCutRebar.GetAllRebarData(m_vecCutInfo);
	UpdateCutList();
}

void CCNCutRebarDlg::InitRebarSetsAndehSel()
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
		m_pRebaras = nullptr;
		for (int i = 0; i < reas.GetSize(); i++)
		{
			RebarAssembly* rebaras = reas.GetAt(i);
			if (rebaras->GetCallerId() == rebset->GetCallerId())
			{
				m_pRebaras = rebaras;
			}
		}

		if (m_pRebaras != nullptr)
		{
			DgnModelRefP        modelRef = ACTIVEMODEL;
			ElementId tmpid = m_pRebaras->GetSelectedElement();
			if (tmpid == 0)
			{
				return;
			}
			DgnModelRefP modelp = m_pRebaras->GetSelectedModel();
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


// CCNCutRebarDlg 消息处理程序


void CCNCutRebarDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();

	DgnModelRefP modelRef = ACTIVEMODEL;

	InitRebarSetsAndehSel();
	if (!m_ehSel.IsValid())
	{
		return ;
	}

	bool isReserveCut = false;

	if (m_CheckReserve.GetCheck() == 1)
	{
		isReserveCut = true;
	}

	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());

	m_ListCutRebar.GetAllRebarData(m_vecCutInfo);

	WallRebarAssemblyNew*  wallRebar = REA::Create<CCutRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());

	wallRebar->m_pOldRebaras = m_pRebaras;
	wallRebar->m_selectrebars = m_selectrebars;
	wallRebar->SetvecCutInfo(m_vecCutInfo);
	wallRebar->SetisReserveCut(isReserveCut);
	wallRebar->SetWallData(eeh);
	wallRebar->MakeRebars(modelRef);
	wallRebar->Save(modelRef); // must save after creating rebars
	ElementId contid = wallRebar->FetchConcrete();
	EditElementHandle eeh2(contid, modelRef);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);
}




void CCNCutRebarDlg::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
}
