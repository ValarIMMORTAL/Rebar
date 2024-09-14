// CustomRebarDlag.cpp: 实现文件
//
#include "_USTATION.h"
#include "resource.h"
#include "GalleryIntelligentRebar.h"
#include "CustomRebarDlag.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"
#include "ElementAttribute.h"
#include <memory.h>
#include "SingleRebarAssembly.h"
#include "ExtractFacesTool.h"



// CustomRebarDlag 对话框

IMPLEMENT_DYNAMIC(CustomRebarDlag, CDialogEx)

CustomRebarDlag::CustomRebarDlag(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CustomizeRebar, pParent)
{

}

CustomRebarDlag::~CustomRebarDlag()
{
}

void CustomRebarDlag::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_ComboSize);
	DDX_Control(pDX, IDC_COMBO2, m_ComboType);
	DDX_Control(pDX, IDC_COMBO3, m_ComboArrayDir);
	DDX_Control(pDX, IDC_EDIT1, m_EditArrayNum);
	DDX_Control(pDX, IDC_EDIT_SPACING, m_EditSpcing);
	DDX_Control(pDX, IDC_EDIT_ZDYLEVEL, m_edit_level);
	DDX_Control(pDX, IDC_EDIT_ZDYTYPE, m_edit_type);
}


BEGIN_MESSAGE_MAP(CustomRebarDlag, CDialogEx)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CustomRebarDlag::OnCbnSelchangeCombo1)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CustomRebarDlag::OnCbnSelchangeCombo2)
	ON_BN_CLICKED(IDOK, &CustomRebarDlag::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT1, &CustomRebarDlag::OnEnChangeEdit1)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CustomRebarDlag::OnCbnSelchangeCombo3)
	ON_BN_CLICKED(IDC_BUTTON1, &CustomRebarDlag::OnBnClickedButton1)
	ON_EN_CHANGE(IDC_EDIT_SPACING, &CustomRebarDlag::OnEnChangeEditSpacing)
END_MESSAGE_MAP()


// CustomRebarDlag 消息处理程序



BOOL CustomRebarDlag::OnInitDialog()
{

	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	for each (auto var in g_listRebarSize)
		m_ComboSize.AddString(var);

	for each (auto var in g_listRebarType)
		m_ComboType.AddString(var);

	for each (auto var in g_listCustomizeDir)
		m_ComboArrayDir.AddString(var);


	ElementId contid = 0;
	GetElementXAttribute(m_WallehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, m_WallehSel.GetModelRef());
	if (contid > 0)
	{
		GetElementXAttribute(contid, sizeof(StairRebarInfo), m_CustomizeRebarInfo, CustomRebarAttribute, ACTIVEMODEL);
	}
	strcpy(m_CustomizeRebarInfo.rebarSize, "6mm");

	CString strRebarSize(m_CustomizeRebarInfo.rebarSize);
	if (strRebarSize.Find(L"mm") == -1)
		strRebarSize += "mm";
	int nIndex = m_ComboSize.FindStringExact(0, strRebarSize);

	CString strArrayDir(m_CustomizeRebarInfo.rebarArrayDir);
	int nIndexDir = m_ComboArrayDir.FindStringExact(0, strArrayDir);

	CString strArrayNum;
	strArrayNum.Format(L"%.2f", m_CustomizeRebarInfo.rebarArrayNum);//阵列数量

	CString strSpacing;
	strSpacing.Format(L"%.2f", m_CustomizeRebarInfo.rebarSpacing);//钢筋间距

	m_ComboSize.SetCurSel(nIndex);//尺寸
	m_ComboType.SetCurSel(m_CustomizeRebarInfo.rebarType);//型号
	m_ComboArrayDir.SetCurSel(nIndexDir);	//阵列方向
	m_EditArrayNum.SetWindowText(strArrayNum);//阵列数量
	m_EditSpcing.SetWindowText(strSpacing);	//钢筋间距

	m_ComboSize.SetCurSel(1);//尺寸
	m_ComboType.SetCurSel(m_CustomizeRebarInfo.rebarType);//型号
	m_ComboArrayDir.SetCurSel(2);	//阵列方向
	m_EditArrayNum.SetWindowText(L"1");//阵列数量
	m_EditSpcing.SetWindowText(L"200");	//钢筋间距
	m_edit_level.SetWindowText(L"1");
	m_edit_type.SetWindowText(L"正面");

	return true;
}





void CustomRebarDlag::OnCbnSelchangeCombo1()
{
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboSize.GetCurSel());
	strcpy(m_CustomizeRebarInfo.rebarSize, CT2A(*it));
}


void CustomRebarDlag::OnCbnSelchangeCombo2()
{
	m_CustomizeRebarInfo.rebarType = m_ComboType.GetCurSel();
}


void CustomRebarDlag::OnBnClickedOk()
{
	CDialogEx::OnOK();
	DgnModelRefP   modelRef = ACTIVEMODEL;
	RebarSetTagArray rsetTags;
	/***********************************给sizekey附加型号******************************************************/
	GetDiameterAddType(m_CustomizeRebarInfo.rebarSize, m_CustomizeRebarInfo.rebarType);
	CString	wstrtype = CString();
	m_edit_type.GetWindowText(wstrtype);

	CString	wstrLevel = CString();
	m_edit_level.GetWindowText(wstrLevel);

	CString	wstrSpacing = CString();
	m_EditSpcing.GetWindowText(wstrSpacing);
	double spacing = atof(CT2A(wstrSpacing));

	m_CustomizeRebarInfo.rebarSpacing = spacing;
	strcpy(m_CustomizeRebarInfo.rebarbsType, CT2A(wstrtype));
	strcpy(m_CustomizeRebarInfo.rebarLevel, CT2A(wstrLevel));
	/***********************************给sizekey附加型号******************************************************/
	m_PcustomAssembly = new CustomRebarAssembly(m_ehSel);
 	m_Pcustom = std::shared_ptr<CustomRebar>(new CustomRebar(m_vctLineinfo));
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


void CustomRebarDlag::OnCbnSelchangeCombo3()
{
	auto it = g_listCustomizeDir.begin();
	advance(it, m_ComboArrayDir.GetCurSel());
	strcpy(m_CustomizeRebarInfo.rebarArrayDir, CT2A(*it));
}


void CustomRebarDlag::OnEnChangeEdit1()
{
	CString	strTemp = CString();
	m_EditArrayNum.GetWindowText(strTemp);
	m_CustomizeRebarInfo.rebarArrayNum = atoi(CT2A(strTemp));
}


void CustomRebarDlag::ParsingElementPro(ElementHandleCR eeh)
{
	CurveVectorPtr profile = ICurvePathQuery::ElementToCurveVector(eeh);
	pointInfo oneLineInfo;
	vector<pointInfo> vctPoint = m_vctLineinfo;
	if (profile!=NULL)
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
				priPtr->PointAtSignedDistanceFromFraction(0, spaceing/2, false, arcDetail);
				DPoint3d arcMid = arcDetail.point;
				oneLineInfo.curType = ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Arc;
				oneLineInfo.ptStr = arcStart;
				oneLineInfo.ptEnd = arcEnd;
				oneLineInfo.ptCenter = ptr->center;
				oneLineInfo.ptMid = arcMid;
			}
			vctPoint.push_back(oneLineInfo);
		}
	}
	m_vctLineinfo = vctPoint;

	int i = 0,j = 0;
	for (int x = 0;x< m_vctLineinfo.size();x++)
	{
		if (m_vctLineinfo[x].curType == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Arc)
			i++;
		else if (m_vctLineinfo[x].curType == ICurvePrimitive::CurvePrimitiveType::CURVE_PRIMITIVE_TYPE_Line)
			j++;
		else
		{;}
	}
	if (m_vctLineinfo.size() == i)
		m_Linetype = 1;//全为弧线
	else if (m_vctLineinfo.size() == j)
		m_Linetype = 0;//全为直线
	else 
		m_Linetype = -1;

	m_ehSel = eeh;
}

void CustomRebarDlag::OnBnClickedButton1()//选择线串
{
	SelectLineTools::InstallNewInstance(0, this);
}



void SelectLineTools::InstallNewInstance(int toolId, CustomRebarDlag* Linedlg)
{
	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	SelectLineTools* tool = new SelectLineTools(toolId);
 	tool->m_Linedlg = Linedlg;
 	tool->InstallTool();
}


bool SelectLineTools::_OnDataButton(DgnButtonEventCR ev)
{
	auto& ssm = SelectionSetManager::GetManager();
	HitPathCP   path = _DoLocate(ev, true, ComponentMode::Innermost);
	if (path == NULL)
		return false;
	ElementHandle Lineh(mdlDisplayPath_getElem((DisplayPathP)path, 0), mdlDisplayPath_getPathRoot((DisplayPathP)path));
	ssm.AddElement(Lineh.GetElementRef(), ACTIVEMODEL);

	m_Linedlg->ParsingElementPro(Lineh);

	return true;
}

bool SelectLineTools::_OnModifyComplete(DgnButtonEventCR ev)
{ 
	ElementAgenda selectedElement;
	selectedElement = GetElementAgenda();
	
	return true;
}

// bool SelectLineTools::_OnModifierKeyTransition(bool wentDown, int key)
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

bool SelectLineTools::_OnPostLocate(HitPathCP path, WStringR cantAcceptReason) 
{
	 if (!__super::_OnPostLocate(path, cantAcceptReason))
		 return false;
	 return true;
 }

bool SelectLineTools::_WantAdditionalLocate(DgnButtonEventCP ev)
{
	if (NULL == ev)
		return true; // This is a multi-locate tool...

	return GetElementAgenda().GetCount() != 2;
}

void CustomRebarDlag::OnEnChangeEditSpacing()
{
	CString	strTemp = CString();
	m_EditSpcing.GetWindowText(strTemp);
	m_CustomizeRebarInfo.rebarSpacing = atoi(CT2A(strTemp));
}
