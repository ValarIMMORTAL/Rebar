// CustomRebarlDlag.cpp: 实现文件
//
#include "_USTATION.h"
#include "CSlabMainRebarDlg.h"
#include "resource.h"
#include "GalleryIntelligentRebar.h"
#include "CustomRebarlDlag.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"
#include "ElementAttribute.h"
#include <memory.h>
#include "SingleRebarAssembly.h"
#include "CustomizeTool.h"
#include "RainshedREbar.h"
#include "ExtractFacesTool.h"
#include  <cmath>

// CustomRebarlDlag 对话框

IMPLEMENT_DYNAMIC(CustomRebarlDlag, CDialogEx)

CustomRebarlDlag::CustomRebarlDlag(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CustomizeRebar1, pParent)
{

}

CustomRebarlDlag::~CustomRebarlDlag()
{
}

void CustomRebarlDlag::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_ComboSize);
	DDX_Control(pDX, IDC_COMBO2, m_ComboType);
	DDX_Control(pDX, IDC_COMBO3, m_ComboArrayDir);
	DDX_Control(pDX, IDC_EDIT1, m_EditArrayNum);
	DDX_Control(pDX, IDC_LIST1, m_listCtrll);
}


BEGIN_MESSAGE_MAP(CustomRebarlDlag, CDialogEx)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CustomRebarlDlag::OnCbnSelchangeCombo1)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CustomRebarlDlag::OnCbnSelchangeCombo2)
	ON_BN_CLICKED(IDOK, &CustomRebarlDlag::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT1, &CustomRebarlDlag::OnEnChangeEdit1)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CustomRebarlDlag::OnCbnSelchangeCombo3)
	ON_BN_CLICKED(IDC_BUTTON1, &CustomRebarlDlag::OnBnClickedButton1)
END_MESSAGE_MAP()


// CustomRebarlDlag 消息处理程序



BOOL CustomRebarlDlag::OnInitDialog()
{

	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	for (auto var : g_listRebarSize)
	m_ComboSize.AddString(var);
	for (auto var : g_listRebarType)
		m_ComboType.AddString(var);
	for (auto var : g_listCustomizeDir)
		m_ComboArrayDir.AddString(var);
	ElementId contid = 0;
	GetElementXAttribute(m_WallehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, m_WallehSel.GetModelRef());
	if (contid > 0)
	{
		GetElementXAttribute(contid, sizeof(StairRebarInfo), m_CustomizeRebarInfo, CustomRebarAttribute, ACTIVEMODEL);
	}
	CString strRebarSize(m_CustomizeRebarInfo.rebarSize);
	if (strRebarSize.Find(L"mm") == -1)
		strRebarSize += "mm";
	int nIndex = m_ComboSize.FindStringExact(0, strRebarSize);
	CString strArrayDir(m_CustomizeRebarInfo.rebarArrayDir);
	int nIndexDir = m_ComboArrayDir.FindStringExact(2, strArrayDir);
	CString strArrayNum;
	strcpy(m_CustomizeRebarInfo.rebarSize, "10mm");
	strcpy(m_CustomizeRebarInfo.rebarArrayDir, "Z");
	m_CustomizeRebarInfo.rebarArrayNum = 1;
	m_CustomizeRebarInfo.rebarType = 0;
	strArrayNum.Format(_T("%d"), m_CustomizeRebarInfo.rebarArrayNum);//阵列数量
	m_ComboSize.SetCurSel(0);//尺寸
	m_ComboType.SetCurSel(m_CustomizeRebarInfo.rebarType);//型号
	m_ComboArrayDir.SetCurSel(2);	//阵列方向
	m_EditArrayNum.SetWindowText(strArrayNum);//阵列数量
	UpdateData(TRUE);
	LONG lStyle;
	lStyle = GetWindowLong(m_listCtrll.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_listCtrll.m_hWnd, GWL_STYLE, lStyle);//设置style
	DWORD dwStyle = m_listCtrll.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_listCtrll.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	tagRECT stRect;
	m_listCtrll.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	m_listCtrll.InsertColumn(0, _T("编号"), 55, ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_listCtrll.InsertColumn(1, _T("钢筋长度"), 120, ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listCtrll.InsertColumn(2, _T("钢筋类型"), 110, ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	UpdateData(FALSE);
	return true;
}

void CustomRebarlDlag::OnCbnSelchangeCombo1()
{
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboSize.GetCurSel());
	strcpy(m_CustomizeRebarInfo.rebarSize, CT2A(*it));
}


void CustomRebarlDlag::OnCbnSelchangeCombo2()
{
	m_CustomizeRebarInfo.rebarType = m_ComboType.GetCurSel();
}


void CustomRebarlDlag::OnBnClickedOk()
{
	CDialogEx::OnOK();
	DgnModelRefP   modelRef = ACTIVEMODEL;
	RebarSetTagArray rsetTags;
	m_PcustomAssembly = new CustomRebarAssembly(m_ehSel);
	m_Pcustom = std::shared_ptr<CustomRebar>(new CustomRebar(m_vctLineinfo));

	m_listCtrll.GetAllRebarData(vecWCustomRebarl);
	int i = 0;
	for (auto it = vecWCustomRebarl.begin(); it != vecWCustomRebarl.end(); it++)
	{
		m_vctLineinfo[i].vec.x = m_vctLineinfo[i].ptEnd.x - m_vctLineinfo[i].ptStr.x;
		m_vctLineinfo[i].vec.y = m_vctLineinfo[i].ptEnd.y - m_vctLineinfo[i].ptStr.y;
		m_vctLineinfo[i].vec.z = m_vctLineinfo[i].ptEnd.z - m_vctLineinfo[i].ptStr.z;
		i++;
	}

	DPoint3d movePt;
	STRainshedRebarAssembly Rebarmove;
	vector<CenterlineLength> Centerlinel, Centerlinell;
	CenterlineLength Centerl, Centerll;
	for (int j = 0; j < i; j++)
	{	
		if (vecWCustomRebarl[j].lengthtype == 1)
		{
			if (vecWCustomRebarl[j].lengthtype == 1 && j < i - 1)
			{
				BrString strRebarSize = m_CustomizeRebarInfo.rebarSize;
				if (strRebarSize != L"")
				{
					if (strRebarSize.Find(L"mm") != -1)
					{
						strRebarSize.Replace(L"mm", L"");
					}
				}
				GetDiameterAddType(strRebarSize, m_CustomizeRebarInfo.rebarType);
				RebarEndType endTypeStart;
				endTypeStart.SetType(RebarEndType::kNone);
				double bendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);        //乘以了30

				CVector3D vec1 = m_vctLineinfo[j].vec;
				CVector3D vec2 = m_vctLineinfo[j + 1].vec;
				vec1.Normalize();
				vec2.Normalize();
				vec1.Negate();
				double dot = mdlVec_dotProduct(&vec1, &vec2);
				dot = acos(dot);
				dot = fabs(dot);
				Centerl.Centerline = bendRadius / tan(dot / 2.0);
				Centerlinel.push_back(Centerl);
				if (j == 0)
				{
					vecWCustomRebarl[j].Rebarlength += Centerlinel[j].Centerline;
				}
				else
				{
					vecWCustomRebarl[j].Rebarlength += Centerlinel[j].Centerline + Centerlinel[j - 1].Centerline;
				}
			}
			if (vecWCustomRebarl[j].lengthtype == 1 && j == i - 1)//最后一根钢筋
			{
				vecWCustomRebarl[j].Rebarlength += Centerlinel[j - 1].Centerline;
			}
		}
		else
		{
			BrString strRebarSize = m_CustomizeRebarInfo.rebarSize;
			if (strRebarSize != L"")
			{
				if (strRebarSize.Find(L"mm") != -1)
				{
					strRebarSize.Replace(L"mm", L"");
				}
			}
			double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
			if (vecWCustomRebarl[j].lengthtype == 0 && j < i - 1)
			{
				BrString strRebarSize = m_CustomizeRebarInfo.rebarSize;
				if (strRebarSize != L"")
				{
					if (strRebarSize.Find(L"mm") != -1)
					{
						strRebarSize.Replace(L"mm", L"");
					}
				}
				GetDiameterAddType(strRebarSize, m_CustomizeRebarInfo.rebarType);
				RebarEndType endTypeStart;
				endTypeStart.SetType(RebarEndType::kNone);
				double bendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);        //乘以了30
				CVector3D vec1 = m_vctLineinfo[j].vec;
				CVector3D vec2 = m_vctLineinfo[j + 1].vec;
				vec1.Normalize();
				vec2.Normalize();
				vec1.Negate();
				double dot = mdlVec_dotProduct(&vec1, &vec2);
				dot = acos(dot);
				dot = fabs(dot);
				Centerll.Centerline = bendRadius / tan(dot / 2.0);
				Centerlinell.push_back(Centerll);
				bendRadius += (diameter / 2.0);
				 vec1 = m_vctLineinfo[j].vec;
				 vec2 = m_vctLineinfo[j + 1].vec;
				vec1.Normalize();
				vec2.Normalize();
				vec1.Negate();
				 dot = mdlVec_dotProduct(&vec1, &vec2);
				dot = acos(dot);
				dot = fabs(dot);
				Centerl.Centerline = bendRadius / tan(dot / 2.0);
				Centerlinel.push_back(Centerl);
				if (j == 0)
				{
					vecWCustomRebarl[j].Rebarlength -= (Centerlinel[j].Centerline- Centerlinell[j].Centerline);
				}
				else
				{
					vecWCustomRebarl[j].Rebarlength -= ((Centerlinel[j].Centerline - Centerlinell[j].Centerline) + (Centerlinel[j-1].Centerline - Centerlinell[j-1].Centerline));
				}
			}
			if (vecWCustomRebarl[j].lengthtype == 0 && j == i - 1)//最后一根钢筋
			{
				vecWCustomRebarl[j].Rebarlength -= (Centerlinel[j-1].Centerline - Centerlinell[j-1].Centerline);
			}
		}
		movePt = m_vctLineinfo[j].ptStr;
		Rebarmove.movePoint(m_vctLineinfo[j].vec, movePt, vecWCustomRebarl[j].Rebarlength, true);
		m_vctLineinfo[j].ptEnd = movePt;
		if (j < i - 1)
		{
			m_vctLineinfo[j + 1].ptStr = movePt;
		}		
	}

	m_Pcustom->m_vecRebarPts = m_vctLineinfo;
	m_Pcustom->m_CustomRebarInfo = m_CustomizeRebarInfo;
	m_Pcustom->m_linestyle = m_Linetype;
	vector<ElementId> vecElementID;
	vecElementID.push_back(0);
	m_PcustomAssembly->Push_CustomRebar(m_Pcustom);
	RebarSetTag* tag = m_PcustomAssembly->MakeRebar(vecElementID[0], modelRef);
	tag->SetBarSetTag(1);
	rsetTags.Add(tag);
	SingleRebarAssembly*  rebarAsm = REA::Create<SingleRebarAssembly>(modelRef);
	rebarAsm->SetSlabData(m_WallehSel);
	rebarAsm->NewRebarAssembly(modelRef);
	rebarAsm->AddRebarSets(rsetTags);
	rebarAsm->Save(modelRef);
	ElementId conid = rebarAsm->GetConcreteOwner();
	EditElementHandle eeh2(conid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);
}


void CustomRebarlDlag::OnCbnSelchangeCombo3()
{
	auto it = g_listCustomizeDir.begin();
	advance(it, m_ComboArrayDir.GetCurSel());
	strcpy(m_CustomizeRebarInfo.rebarArrayDir, CT2A(*it));
}


void CustomRebarlDlag::OnEnChangeEdit1()
{
	CString	strTemp = CString();
	m_EditArrayNum.GetWindowText(strTemp);
	m_CustomizeRebarInfo.rebarArrayNum = atoi(CT2A(strTemp));
}


void CustomRebarlDlag::ParsingElementPro(ElementHandleCR eeh)
{
	CurveVectorPtr profile = ICurvePathQuery::ElementToCurveVector(eeh);
	pointInfo oneLineInfo;
	vector<pointInfo> vctPoint;
	if (profile != NULL)
	{
		for (unsigned int i = 0; i < profile->size(); i++)
		{
			ICurvePrimitivePtr priPtr = profile->at(i);
			if (priPtr->GetCurvePrimitiveType() == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Line)//直线
			{
				DPoint3d ptStr, ptEnd;
				DSegment3dCP seg = priPtr->GetLineCP();
				double dLen = seg->Length();
				seg->GetStartPoint(ptStr);
				seg->GetEndPoint(ptEnd);
				oneLineInfo.ptStr = ptStr;
				oneLineInfo.ptEnd = ptEnd;
				oneLineInfo.curType = ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Line;

			}
			else if (priPtr->GetCurvePrimitiveType() == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_LineString)//多段直线
			{
				DPoint3d ptStr, ptEnd;
				bvector<DPoint3d> const* ptr = priPtr->GetLineStringCP();
				for (int i = 0; i < (int)ptr->size() - 1; i++)
				{
					ptStr = ptr->at(i);
					ptEnd = ptr->at(i + 1);
					oneLineInfo.ptStr = ptStr;
					oneLineInfo.ptEnd = ptEnd;
					oneLineInfo.curType = ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Line;
					vctPoint.push_back(oneLineInfo);
				}
				continue;
			}
			else if (priPtr->GetCurvePrimitiveType() == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Arc)
			{
				DEllipse3dCP ptr = priPtr->GetArcCP();

				double spaceing = 0.00;
				CurveLocationDetail arcDetail;
				priPtr->PointAtSignedDistanceFromFraction(0, spaceing, false, arcDetail);
				DPoint3d arcStart = arcDetail.point;

				spaceing = ptr->ArcLength();
				priPtr->PointAtSignedDistanceFromFraction(0, spaceing, false, arcDetail);
				DPoint3d arcEnd = arcDetail.point;

				oneLineInfo.curType = ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Arc;
				oneLineInfo.ptStr = arcStart;
				oneLineInfo.ptEnd = arcEnd;
				oneLineInfo.ptCenter = ptr->center;
			}
			vctPoint.push_back(oneLineInfo);
		}
	}
	m_vctLineinfo = vctPoint;

	int i = 0, j = 0;
	for (int x = 0; x < m_vctLineinfo.size(); x++)
	{
		if (m_vctLineinfo[x].curType == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Arc)
			i++;
		else if (m_vctLineinfo[x].curType == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Line)
			j++;
		else
		{
			;
		}
	}
	if (m_vctLineinfo.size() == i)
		m_Linetype = 1;//全为弧线
	else if (m_vctLineinfo.size() == j)
		m_Linetype = 0;//全为直线
	else
		m_Linetype = -1;

	m_ehSel = eeh;
}

void CustomRebarlDlag::OnBnClickedButton1()//选择线串
{
	SelectLineToolsl::InstallNewInstance(0, this);
}



void SelectLineToolsl::InstallNewInstance(int toolId, CustomRebarlDlag* Linedlg)
{
	SelectLineToolsl* tool = new SelectLineToolsl(toolId);
	tool->m_Linedlg = Linedlg;
	tool->InstallTool();
}


bool SelectLineToolsl::_OnDataButton(DgnButtonEventCR ev)
{
	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	HitPathCP   path = _DoLocate(ev, true, ComponentMode::Innermost);
	if (path == NULL)
		return false;
	ElementHandle Lineh(mdlDisplayPath_getElem((DisplayPathP)path, 0), mdlDisplayPath_getPathRoot((DisplayPathP)path));
	ssm.AddElement(Lineh.GetElementRef(), ACTIVEMODEL);
	m_Linedlg->ParsingElementPro(Lineh);

	//这里操作
	m_Linedlg->UpdateData(TRUE); 
	std::shared_ptr<CustomRebar>(new CustomRebar(m_Linedlg->m_vctLineinfo));
	 int number= 1;
	double dis;
	CustomRebarl vecRebarl;
	m_Linedlg->m_listCtrll.DeleteAllItems();
	for (auto it = m_Linedlg->m_vctLineinfo.begin(); it != m_Linedlg->m_vctLineinfo.end(); it++)
	{
		vecRebarl.lengthtype = 0; //长度类型
          vecWCustomRebarl.push_back(vecRebarl);
		int iLine;
		if (it->curType == 1)
		{ 
			CString str;

			CustomRebarl   mRebarl;
			CString strValue;


			dis = it->ptStr.Distance(it->ptEnd);
			iLine = m_Linedlg->m_listCtrll.GetItemCount();
			str.Format(_T("%d"), number);
			m_Linedlg->m_listCtrll.InsertItem(iLine, str); // 插入行
			str.Format(_T("%.2f"), dis);
			m_Linedlg->m_listCtrll.SetItemText(iLine,1, str);

			ListCtrlEx::CStrList strlist = RebartypeDir;
			m_Linedlg->m_listCtrll.SetCellStringList(iLine, 2, strlist);
			auto it = strlist.begin();
			advance(it, vecWCustomRebarl[iLine].lengthtype);
			strValue = *it;
			m_Linedlg->m_listCtrll.SetItemText(iLine,2, strValue);
		}	
		number++;
	}
	number++;
	m_Linedlg->UpdateData(FALSE);
	return true;
}

bool SelectLineToolsl::_OnModifyComplete(DgnButtonEventCR ev)
{
	ElementAgenda selectedElement;
	selectedElement = GetElementAgenda();

	return true;
}

// bool SelectLineToolsl::_OnModifierKeyTransition(bool wentDown, int key)
// {
// 	if (CTRLKEY != key)
// 		return false;
// 	if (GetElementAgenda().GetCount() < 2)
// 		return false;
// 	if (wentDown)
// 	{
// 		__super::_SetLocateCursor(true);
// 	}
// 	else {
// 		__super::_SetLocateCursor(false);
// 	}
// 	return true;
// }

bool SelectLineToolsl::_OnPostLocate(HitPathCP path, WStringR cantAcceptReason)
{
	if (!__super::_OnPostLocate(path, cantAcceptReason))
		return false;
	return true;
}

bool SelectLineToolsl::_WantAdditionalLocate(DgnButtonEventCP ev)
{
	if (NULL == ev)
		return true; // This is a multi-locate tool...

	return GetElementAgenda().GetCount() != 2;
}


