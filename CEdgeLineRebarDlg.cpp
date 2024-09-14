// CEdgeLineRebarDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CEdgeLineRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "CommonFile.h"
#include "ConstantsDef.h"
#include "PickElementTool.h"
#include "ScanIntersectTool.h"
#include "SelectLineTool.h"
#include "PITRebarCurve.h"
#include "EdgeRebarAssembly.h"
#include "ExtractFacesTool.h"
#include "WallHelper.h"
#include "XmlHelper.h"

extern GlobalParameters g_globalpara;
using namespace Gallery;

namespace _local {
	/// @brief 判断一个元素是不是墙
	/// @details 这个是看PDMS参数中的Type中有没有FLOOR字样
	bool isswal(EditElementHandleCR element)
	{
		std::string _name, type;
		GetEleNameAndType(element.GetElementId(), element.GetModelRef()/*ACTIVEMODEL*/, _name, type);
		if (type == "GWALL")
			return true;
		else
			return false;

	}
}

// CEdgeLineRebarDlg 对话框

IMPLEMENT_DYNAMIC(CEdgeLineRebarDlg, CDialogEx)

CEdgeLineRebarDlg::CEdgeLineRebarDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_EdgeRebar, pParent)
{

}

CEdgeLineRebarDlg::~CEdgeLineRebarDlg()
{
}

void CEdgeLineRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_CombRebarSize);
	DDX_Control(pDX, IDC_COMBO12, m_CombRebarType);
	DDX_Control(pDX, IDC_COMBO14, m_CombRebarStyle);
}


BEGIN_MESSAGE_MAP(CEdgeLineRebarDlg, CDialogEx)
	ON_CBN_KILLFOCUS(IDC_COMBO1, &CEdgeLineRebarDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON3, &CEdgeLineRebarDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON1, &CEdgeLineRebarDlg::OnBnClickedButton1)
	ON_EN_CHANGE(IDC_EDIT13, &CEdgeLineRebarDlg::OnEnChangeEdit13)
	ON_BN_CLICKED(IDC_BUTTON2, &CEdgeLineRebarDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


BOOL CEdgeLineRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	 //TODO:  在此添加额外的初始化
	m_CombRebarSize.ResetContent();
	m_CombRebarType.ResetContent();
	mdlOutput_prompt(L"OnInitDialogstr！");
	for each (auto var in g_listRebarType)
		m_CombRebarType.AddString(var);

	for each (auto var in g_listRebarSize)
		m_CombRebarSize.AddString(var);

	std::list<CString> g_listRebarStyle = { _T("直钢筋"),_T("方钢筋") };

	for each (auto var in g_listRebarStyle)
		m_CombRebarStyle.AddString(var);

	m_CombRebarSize.SetCurSel(1);
	m_CombRebarType.SetCurSel(0);
	m_CombRebarStyle.SetCurSel(0);

	CString cstrSize;
	m_CombRebarSize.GetWindowTextW(cstrSize);
	if (cstrSize.Find(L"mm") != -1)
		cstrSize.Replace(L"mm", L"");
	char tmpchar[256];
	strcpy(tmpchar, CT2A(cstrSize));
	int i = m_CombRebarType.GetCurSel();
	GetDiameterAddType(tmpchar, i);
	std::string stRebarsize(tmpchar);

	double length = g_globalpara.m_laplenth[stRebarsize];
	//length = length * 12;
	char a[10];
	itoa(length, a, 10);
	CString LOleng(a);
	GetDlgItem(IDC_EDIT15)->SetWindowText(LOleng);
	GetDlgItem(IDC_EDIT16)->SetWindowText(LOleng);
	GetDlgItem(IDC_EDIT13)->SetWindowText(L"150");
	return TRUE;
}

void CEdgeLineRebarDlg::OnEnChangeEdit13()
{
}


// CEdgeLineRebarDlg 消息处理程序

void CEdgeLineRebarDlg::OnCbnSelchangeCombo1()
{
	double length = 0;
	CString strRebarSize;
	m_CombRebarSize.GetWindowTextW(strRebarSize);
	int i = m_CombRebarType.GetCurSel();
	//GetDiameterAddType(strRebarSize, nType);
	char tmpchar[256];
	strcpy(tmpchar, CT2A(strRebarSize));
	GetDiameterAddType(tmpchar, i);
	std::string stRebarsize(tmpchar);
	int diameter;
	diameter = (int)atoi(stRebarsize.c_str());
	length = g_globalpara.m_laplenth[stRebarsize];
	//length = diameter * 12;
	//GetDlgItem(IDC_EDIT15)->SetWindowText(L"600");
	char a[10];
	itoa(length, a, 10);
	CString LOleng(a);
	GetDlgItem(IDC_EDIT15)->SetWindowText(LOleng);
	GetDlgItem(IDC_EDIT16)->SetWindowText(LOleng);
}



void CEdgeLineRebarDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	//m_bMonitor = true;
	SelectionSetManager::GetManager().EmptyAll();
	auto select_wall_tool = new PickElementTool(
		_local::isswal,
		[this](const ElementAgenda &agenda)
	{
		for (auto &entry : agenda)
		{
			auto &elem = const_cast<EditElementHandleR>(static_cast<EditElementHandleCR>(entry));
			this->SetSelElement(elem);
			SelectLineTool::InstallNewInstance(1, 1, this);
			break;
		}

	}
		,
		[this](void)
	{
		ClearLine();
	});
	select_wall_tool->InstallTool();
}

void CEdgeLineRebarDlg::OnBnClickedButton1()
{

	DgnModelRefP modelRef = ACTIVEMODEL;
	EdgeRebarAssembly*  RebarLinesPtr = REA::Create<EdgeRebarAssembly>(modelRef);

	RebarSetTag *tag = NULL;
	ElementId conid = m_ehSel.GetElementId();

	RebarLinesPtr->SetSlabData(m_ehSel);
	RebarLinesPtr->SetVerticesData(rebarVerticesNum);
	RebarLinesPtr->Setspacing(m_ErebarData.spacing);
	RebarLinesPtr->SetsizeKey(m_ErebarData.sizeKey);
	bool isSce = RebarLinesPtr->MakeRebars(modelRef);
	RebarLinesPtr->Save(modelRef);
	ElementId contid = RebarLinesPtr->FetchConcrete();
	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);

	if (isSce)
		ClearLine();

}

void CEdgeLineRebarDlg::CalaRebarPoint()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	//获得钢筋直径
	CString strRebarSize;
	m_CombRebarSize.GetWindowTextW(strRebarSize);
	char tmpchar[256];
	strcpy(tmpchar, CT2A(strRebarSize));
	std::string stRebarsize(tmpchar);
	int diameter = (int)atoi(stRebarsize.c_str()) * uor_per_mm;
	//长度，间隔
	double adjustedXLen, adjustedSpacing, spacing, leg1, leg2;
	int numRebar = 0;
	//获得间隔数据
	CString strInterval;
	GetDlgItem(IDC_EDIT13)->GetWindowTextW(strInterval);
	spacing = atoi(CT2A(strInterval)) * uor_per_mm;
	CString leglen1, leglen2;
	GetDlgItem(IDC_EDIT15)->GetWindowTextW(leglen1);
	leg1 = atoi(CT2A(leglen1));
	GetDlgItem(IDC_EDIT16)->GetWindowTextW(leglen2);
	leg2 = atoi(CT2A(leglen2));
	//保护层计算
	double leftSideCov, rightSideCov, allSideCov,wallSideCov,falSideCov;//前后保护层，墙保护层，板保护层

	wallSideCov = falSideCov = leftSideCov = rightSideCov = 50 * uor_per_mm;
	//计算长度
	allSideCov = leftSideCov + rightSideCov;
	double xLen = ptLine[0].DistanceXY(ptLine[1]);

	adjustedXLen = xLen - allSideCov - diameter;

	numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	double wallNess, FallNess;//板，墙厚度
	GetFallThickness(FallNess, m_ehFal);
	GetWallThickness(wallNess);
	//GetWallThickness(wallNess, m_ehSel);
	movePoint(Linevec, StrPtr, leftSideCov);
	if (wallNess == 0)
		wallNess = FallNess;
	if (abs(FallNess - wallNess) / uor_per_mm < 100)
	{
		if (FallNess > wallNess)
			FallNess = wallNess;
		else
			wallNess = FallNess;
	}
	rebarVerticesNum.clear();
	double arcradius = 225 * uor_per_mm;
	m_ErebarData.spacing = adjustedSpacing;
	int i = m_CombRebarType.GetCurSel();
	/***********************************给sizekey附加型号******************************************************/
	GetDiameterAddType(tmpchar, i);
	/***********************************给sizekey附加型号******************************************************/
	BrString Sizekey(tmpchar);
	m_ErebarData.sizeKey = Sizekey;
	for (int i = 0; i < numRebar; i++)
	{
		vector<RebarVertices>     rebarVertices;
		//起点终点
		DPoint3d ptrs[4];
		DPoint3d strpt, endpt;
		if (i == numRebar - 1)//如果是最后一根，要判断当前还有多少距离,符合距离要求就要再布置一根
		{
			double sDis = adjustedXLen - (numRebar - 2)*adjustedSpacing;
			if (sDis > 30 * uor_per_mm)
			{
				//符合要求继续
			}
			else
			{
				continue;
			}
		}
		strpt = endpt = StrPtr;
		strpt.z += wallNess;//加墙的厚度
		endpt.z = endpt.z-FallNess+falSideCov;//减板的厚度
		movePoint(Wallvec, strpt, wallNess - wallSideCov, false);
		movePoint(Wallvec, endpt, FallNess);

		ptrs[0] = ptrs[1] = strpt;
		ptrs[2] = ptrs[3] = endpt;
		ptrs[0].z += leg1;
		movePoint(Wallvec, ptrs[3], leg2);
		EditElementHandle ehLine;

		//LineHandler::CreateLineElement(ehLine, NULL, DSegment3d::From(strpt, endpt), true, *ACTIVEMODEL);

		LineStringHandler::CreateLineStringElement(ehLine, NULL, ptrs, 4, true, *ACTIVEMODEL);
		ehLine.AddToModel();
		m_vecLineId.push_back(ehLine.GetElementId());
		//创建包围钢筋组编号的圆
// 		EditElementHandle arceeh;
// 		ArcHandler::CreateArcElement(arceeh, NULL, ptLine[1], arcradius, arcradius, 0, 0, 2 * PI, true, *ACTIVEMODEL);
// 		arceeh.AddToModel();
		movePoint(Linevec, StrPtr, adjustedSpacing);

		makeRebarCurve(rebarVertices, ptrs, 4);
		rebarVerticesNum.insert(rebarVerticesNum.end(), rebarVertices.begin(), rebarVertices.end());
	}


}


//删除示意线条
void CEdgeLineRebarDlg::ClearLine()
{
	for (auto id : m_vecLineId)
	{
		EditElementHandle eeh(id, ACTIVEMODEL);
		eeh.DeleteFromModel();
	}
	m_vecLineId.clear();
}


bool CEdgeLineRebarDlg::makeRebarCurve(vector<RebarVertices>& m_rebarPts, DPoint3dCP points, size_t numVerts)
{

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	RebarEndType endTypeStart, endTypeEnd;
	double startbendLen, startbendLenTb, endbendLen;
	double startbendRadius, endbendRadius;
	CVector3D strVec, endVec;
	strVec = points[0] - points[1];
	endVec = points[4] - points[3];

	endTypeStart.SetType(RebarEndType::kCog);


	startbendRadius = RebarCode::GetPinRadius(m_ErebarData.sizeKey, ACTIVEMODEL, false);	//乘以了30
	startbendLen = RebarCode::GetBendLength(m_ErebarData.sizeKey, endTypeStart, ACTIVEMODEL);	//乘以了100
	PIT::PITRebarEndType start, end;
	start.SetType(PIT::PITRebarEndType::Type::kCog);
	start.Setangle(135);
	start.SetbendLen(720);
	start.SetbendRadius(420);
	start.SetendNormal(strVec.Normalize());

	end.SetType(PIT::PITRebarEndType::Type::kCog);
	end.Setangle(135);
	end.SetbendLen(720);
	end.SetbendRadius(420);
	end.SetendNormal(endVec.Normalize());


	PIT::PITRebarEndTypes   endTypes;
	PIT::PITRebarCurve rebar;
	RebarVertexP vex;
	endTypes.beg = start;
	endTypes.end = end;

	endTypes.beg.SetptOrgin(points[1]);
	endTypes.end.SetptOrgin(points[2]);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(points[0]);
	vex->SetType(RebarVertex::kStart);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(points[1]);
	vex->SetType(RebarVertex::kIP);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(points[2]);
	vex->SetType(RebarVertex::kIP);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(points[3]);
	vex->SetType(RebarVertex::kEnd);

	//rebar.EvaluateEndTypes(endTypes);
	rebar.EvaluateBend(420);

	m_rebarPts.push_back(rebar.GetVertices());

	return true;
}

bool CEdgeLineRebarDlg::GetFallThickness(double& slabHeight, ElementHandleCR eh)
{
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle testeeh(eh, false);

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &slabHeight);
	for (size_t i = 0; i < vecDownFaceLine.size(); ++i)
	{
		mdlElmdscr_freeAll(&vecDownFaceLine[i]);
		vecDownFaceLine[i] = NULL;
	}
	return true;

}

//获得墙厚度
void CEdgeLineRebarDlg::GetWallThickness(double &thickness, ElementHandle ehSel)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	DgnModelRefP modelRef = ACTIVEMODEL;
	DgnModelRefP tmpModel = ehSel.GetModelRef();
	EditElementHandle eeh(ehSel, ehSel.GetModelRef());
	double height = 0;
	EditElementHandle eea;
	eea.Duplicate(eeh);

	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownBackLine;

	std::vector<EditElementHandle *> SHoleeh;
	EditElementHandle Eleeh;
	EFT::GetSolidElementAndSolidHoles(eea, Eleeh, SHoleeh);
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &height);
	map<double, double> mapthickness;
	CalculateThickness(vecDownFontLine, vecDownBackLine, mapthickness);

	if (mapthickness.size() > 0)
	{
		thickness = mapthickness.begin()->second;
		thickness = thickness + 0.1;
	}
	else
	{
		thickness = 0;
	}

	for (MSElementDescrP tmpDescr : vecDownFaceLine)
	{
		mdlElmdscr_freeAll(&tmpDescr);
		tmpDescr = nullptr;
	}
}
////获得墙厚度
void CEdgeLineRebarDlg::GetWallThickness(double &thickness)
{
	// 分析墙的几何参数
	WallHelper::WallGeometryInfo wall_geometry_info;
	DVec3d top_slab_dir;
	if (!WallHelper::analysis_geometry(m_ehSel, wall_geometry_info))
	{
		mdlOutput_error(L"获得墙几何信息失败");
		return;
	}
	if (!WallHelper::analysis_slab_position_with_wall(m_ehSel, wall_geometry_info.normal,m_ehFal , top_slab_dir))
	{
		mdlOutput_error(L"分析墙的顶板方向失败");
		return;
	}
	GetWallThickness(thickness, m_ehSel);
	Wallvec = top_slab_dir;
	if (COMPARE_VALUES_EPS(Wallvec.x, 0, 1e-1)==0 && COMPARE_VALUES_EPS(Wallvec.y, 0, 1e-1)==0)
		Wallvec = wall_geometry_info.normal;
	StrPtr= ptLine[0];
}


void CEdgeLineRebarDlg::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

void CEdgeLineRebarDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	ClearLine();
}
