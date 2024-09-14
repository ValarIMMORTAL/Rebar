// CAddVerticalRebarDlg.cpp: 实现文件
//


#include "_USTATION.h"
#include "afxdialogex.h"
#include "resource.h"
#include "SingleRebarAssembly.h"
#include <RebarCatalog.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "PITMSCECommon.h"
#include "ElementAttribute.h"
#include "SelectRebarTool.h"
#include "XmlHelper.h"
#include "CAddVerticalRebarDlg.h"
#include "PITRebarAssembly.h"
#include "ExtractFacesTool.h"
// CAddVerticalRebarDlg 对话框

IMPLEMENT_DYNAMIC(CAddVerticalRebarDlg, CDialogEx)

CAddVerticalRebarDlg::CAddVerticalRebarDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_AddVerticalRebar_DIALOG, pParent)
{

}

CAddVerticalRebarDlg::~CAddVerticalRebarDlg()
{
}
// CCombineRebardlg 消息处理程序
// CRebarEditDlg 消息处理程序
// CInsertRebarDlg 消息处理程序
BOOL CAddVerticalRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	m_check_isavespacing.SetCheck(TRUE);
	m_setdata.isuseavespacing = true;
	m_edit_spacingdis.SetWindowTextW(L"200");
	m_setdata.spacing = 200 * Get_uor_per_mm;
	m_edit_rebardiameter.SetWindowTextW(L"12");
	m_setdata.diameter = 12.0*Get_uor_per_mm;
	m_setdata.refline = nullptr;


	m_setdata.leftdis = m_setdata.diameter/2;
	m_setdata.rightdis = m_setdata.diameter/2;
	m_setdata.downdis = m_setdata.diameter/2;
	m_setdata.upDis = m_setdata.diameter/2;
	m_edit_upmove.SetWindowTextW(L"6");
	m_edit_downmove.SetWindowTextW(L"6");
	m_rightmove.SetWindowTextW(L"6");
	m_edit_leftmove.SetWindowTextW(L"6");

	if (m_orgdata.selectrebars.size()==1)//只有一根时，只有上段可以处理
	{
		m_check_uprebar.SetCheck(TRUE);
		m_check_leftrebar.EnableWindow(FALSE);
		m_check_rightrebar.EnableWindow(FALSE);
		m_check_downrebar.EnableWindow(FALSE);
		m_edit_leftmove.EnableWindow(FALSE);
		m_rightmove.EnableWindow(FALSE);
		m_edit_downmove.EnableWindow(FALSE);


		m_setdata.genuprebar = true;
		m_setdata.gendownrebar = false;
		m_setdata.genleftrebar = false;
		m_setdata.genrightrebar = false;
	}
	else if (m_orgdata.selectrebars.size() == 2)//2根横筋时,默认为上左
	{
		m_check_uprebar.SetCheck(TRUE);
		m_check_leftrebar.SetCheck(TRUE);
		m_check_rightrebar.EnableWindow(FALSE);
		m_check_downrebar.EnableWindow(FALSE);
		m_rightmove.EnableWindow(FALSE);
		m_edit_downmove.EnableWindow(FALSE);

		m_setdata.genuprebar = true;
		m_setdata.gendownrebar = false;
		m_setdata.genleftrebar = true;
		m_setdata.genrightrebar = false;
	}
	else if (m_orgdata.selectrebars.size()==4)
	{
		m_check_uprebar.SetCheck(TRUE);
		m_check_leftrebar.SetCheck(TRUE);
		m_check_downrebar.SetCheck(TRUE);
		m_check_rightrebar.SetCheck(TRUE);

		m_setdata.genuprebar = true;
		m_setdata.gendownrebar = true;
		m_setdata.genleftrebar = true;
		m_setdata.genrightrebar = true;

		m_button_selcetLine.EnableWindow(FALSE);

	}
	else
	{
		m_check_uprebar.EnableWindow(FALSE);
		m_check_leftrebar.EnableWindow(FALSE);
		m_check_rightrebar.EnableWindow(FALSE);
		m_check_downrebar.EnableWindow(FALSE);
		m_edit_upmove.EnableWindow(FALSE);
		m_edit_leftmove.EnableWindow(FALSE);
		m_rightmove.EnableWindow(FALSE);
		m_edit_downmove.EnableWindow(FALSE);

		m_button_selcetLine.EnableWindow(FALSE);
		m_button_selectRebar.EnableWindow(FALSE);

		m_setdata.genuprebar = false;
		m_setdata.gendownrebar = false;
		m_setdata.genleftrebar = false;
		m_setdata.genrightrebar = false;
	}
	
	for each (auto va in g_listRebarType)
		m_type.AddString(va);
	m_type.SetCurSel(0);
	OnCbnSelchangeCombo1();

	return TRUE;  // return TRUE unless you set the focus to a control
			  // 异常: OCX 属性页应返回 FALSE
}

void CAddVerticalRebarDlg::GetSetDataFromWindow()//从界面取得设置数据
{
	m_setdata.genuprebar = m_check_uprebar.GetCheck();
	m_setdata.gendownrebar = m_check_downrebar.GetCheck();
	m_setdata.genleftrebar = m_check_leftrebar.GetCheck();
	m_setdata.genrightrebar = m_check_rightrebar.GetCheck();

	CString tmpvalue;
	m_edit_upmove.GetWindowTextW(tmpvalue);
	m_setdata.upDis = atof(CT2A(tmpvalue))*Get_uor_per_mm;

	m_edit_downmove.GetWindowTextW(tmpvalue);
	m_setdata.downdis = atof(CT2A(tmpvalue))*Get_uor_per_mm;

	m_edit_leftmove.GetWindowTextW(tmpvalue);
	m_setdata.leftdis = atof(CT2A(tmpvalue))*Get_uor_per_mm;

	m_rightmove.GetWindowTextW(tmpvalue);
	m_setdata.rightdis = atof(CT2A(tmpvalue))*Get_uor_per_mm;

	m_setdata.isuseavespacing = m_check_isavespacing.GetCheck();
	m_edit_spacingdis.GetWindowTextW(tmpvalue);
	m_setdata.spacing = atof(CT2A(tmpvalue))*Get_uor_per_mm;

	m_edit_rebardiameter.GetWindowTextW(tmpvalue);
	m_setdata.diameter = atof(CT2A(tmpvalue))*Get_uor_per_mm;



}

void CAddVerticalRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_AT_EDIT_UPMOVE, m_edit_upmove);
	DDX_Control(pDX, IDC_AT_EDIT_LEFTMOVE, m_edit_leftmove);
	DDX_Control(pDX, IDC_AT_EDIT_RIGHTMOVE, m_rightmove);
	DDX_Control(pDX, IDC_AT_EDIT_DOWNMOVE, m_edit_downmove);
	DDX_Control(pDX, IDC_AT_CHECK_ISAVESPACING, m_check_isavespacing);
	DDX_Control(pDX, IDC_AT_EDIT_SPACINGDIS, m_edit_spacingdis);
	DDX_Control(pDX, IDC_AT_BUTTON_SELECTREFREBAR, m_button_selectRebar);
	DDX_Control(pDX, IDC_AT_BUTTON_SELECTREFLINE, m_button_selcetLine);
	DDX_Control(pDX, IDC_AT_EDIT_REBARDIAMETER, m_edit_rebardiameter);
	DDX_Control(pDX, IDC_AD_CHECK_UPREBAR, m_check_uprebar);
	DDX_Control(pDX, IDC_AD_CHECK_DOWNREBAR, m_check_downrebar);
	DDX_Control(pDX, IDC_AD_CHECK_LEFTREBAR, m_check_leftrebar);
	DDX_Control(pDX, IDC_AD_CHECK_RIGHTREBAR, m_check_rightrebar);
	DDX_Control(pDX, IDC_COMBO1, m_type);
}


BEGIN_MESSAGE_MAP(CAddVerticalRebarDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CAddVerticalRebarDlg::OnBnClickedOk)
	ON_EN_KILLFOCUS(IDC_AT_EDIT_REBARDIAMETER, &CAddVerticalRebarDlg::OnEnKillfocusAtEditRebardiameter)
	ON_EN_KILLFOCUS(IDC_AT_EDIT_UPMOVE, &CAddVerticalRebarDlg::OnEnKillfocusAtEditUpmove)
	ON_EN_KILLFOCUS(IDC_AT_EDIT_LEFTMOVE, &CAddVerticalRebarDlg::OnEnKillfocusAtEditLeftmove)
	ON_EN_KILLFOCUS(IDC_AT_EDIT_DOWNMOVE, &CAddVerticalRebarDlg::OnEnKillfocusAtEditDownmove)
	ON_EN_KILLFOCUS(IDC_AT_EDIT_RIGHTMOVE, &CAddVerticalRebarDlg::OnEnKillfocusAtEditRightmove)
	ON_BN_CLICKED(IDC_AD_CHECK_UPREBAR, &CAddVerticalRebarDlg::OnBnClickedAdCheckUprebar)
	ON_BN_CLICKED(IDC_AD_CHECK_DOWNREBAR, &CAddVerticalRebarDlg::OnBnClickedAdCheckDownrebar)
	ON_BN_CLICKED(IDC_AD_CHECK_LEFTREBAR, &CAddVerticalRebarDlg::OnBnClickedAdCheckLeftrebar)
//	ON_BN_KILLFOCUS(IDC_AD_CHECK_RIGHTREBAR, &CAddVerticalRebarDlg::OnBnKillfocusAdCheckRightrebar)
	ON_BN_CLICKED(IDC_AD_CHECK_RIGHTREBAR, &CAddVerticalRebarDlg::OnBnClickedAdCheckRightrebar)
	ON_BN_CLICKED(IDC_AT_CHECK_ISAVESPACING, &CAddVerticalRebarDlg::OnBnClickedAtCheckIsavespacing)
	ON_EN_KILLFOCUS(IDC_AT_EDIT_SPACINGDIS, &CAddVerticalRebarDlg::OnEnKillfocusAtEditSpacingdis)
	ON_BN_CLICKED(IDC_AT_BUTTON_SELECTREFREBAR, &CAddVerticalRebarDlg::OnBnClickedAtButtonSelectrefrebar)
	ON_BN_CLICKED(IDC_AT_BUTTON_SELECTREFLINE, &CAddVerticalRebarDlg::OnBnClickedAtButtonSelectrefline)
	ON_BN_CLICKED(IDCANCEL, &CAddVerticalRebarDlg::OnBnClickedCancel)
	ON_WM_CLOSE()
	ON_CBN_SELCHANGE(IDC_COMBO1, &CAddVerticalRebarDlg::OnCbnSelchangeCombo1)
END_MESSAGE_MAP()


// CAddVerticalRebarDlg 消息处理程序


void CAddVerticalRebarDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	DrawVerticalReabr();
	ClearLines();
	if (m_setdata.refline != nullptr)
	{
		EditElementHandle eeh(m_setdata.refline, m_setdata.refline->GetDgnModelP());
		eeh.DeleteFromModel();
	}
	CDialogEx::OnOK();
}
void CAddVerticalRebarDlg::ClearLines()
{
	for (ElementRefP tmpeeh : m_caldata.allLines)
	{
		if (tmpeeh != nullptr)
		{
			EditElementHandle eeh(tmpeeh, tmpeeh->GetDgnModelP());
			eeh.DeleteFromModel();
		}
	}
	m_caldata.allLines.clear();
	m_caldata.rebarPts.clear();
	m_caldata.vecDir.clear();
}
//计算参考点筋的点信息
void CAddVerticalRebarDlg::CalculateRefRebarPoints()
{
	if (m_setdata.refdotrebar.size()>0)
	{
		m_caldata.vecrefpt.clear();
		for (ElementRefP ele:m_setdata.refdotrebar)
		{
			EditElementHandle eeh(ele, ele->GetDgnModelP());
			if (RebarElement::IsRebarElement(eeh))
			{
				double diameter = 0;
				DPoint3d PtStar; DPoint3d PtEnd;
				GetStartEndPointFromRebar(&eeh, PtStar, PtEnd, diameter);
				if (diameter>m_setdata.diameter)
				{
					m_setdata.diameter = diameter;
				}
				RebarPoint rbpt;
				rbpt.ptstr = PtStar;
				rbpt.ptend = PtEnd;
				m_caldata.vecrefpt.push_back(rbpt);
			}
			
		}
	}
}
//选择了1根横筋时的处理方法
void CAddVerticalRebarDlg::CalculateVertexAndDrawLinesWhenOneRebar()
{
	m_orgdata.mapselectrebars.clear();
	EditElementHandle eeh(m_orgdata.selectrebars[0], m_orgdata.selectrebars[0]->GetDgnModelP());
	RebarPoint ptpoint;
	double diameter;
	GetStartEndPointFromRebar(&eeh, ptpoint.ptstr, ptpoint.ptend, diameter);
	ptpoint.sec = 0;
	EditElementHandle eehrebar;
	LineHandler::CreateLineElement(eehrebar, nullptr, DSegment3d::From(ptpoint.ptstr, ptpoint.ptend), true, *ACTIVEMODEL);

	int dia = (int)(diameter / (Get_uor_per_mm));
	if (m_setdata.refline != nullptr)
	{
		EditElementHandle eehline(m_setdata.refline, m_setdata.refline->GetDgnModelP());
		DPoint3d ptstr, ptend;
		mdlElmdscr_extractEndPoints(&ptstr, NULL, &ptend, NULL, eehline.GetElementDescrP(), ACTIVEMODEL);
		ptstr.z = ptend.z = ptpoint.ptstr.z;
		ExtendLine(ptstr, ptend, diameter / Get_uor_per_mm);
		DPoint3d strproject,endproject;
		mdlProject_perpendicular(&strproject, NULL, NULL, eehrebar.GetElementDescrP(), ACTIVEMODEL, &ptstr, NULL, 0.001);
		mdlProject_perpendicular(&endproject, NULL, NULL, eehrebar.GetElementDescrP(), ACTIVEMODEL, &ptend, NULL, 0.001);
		RebarPoint rebardown;
		rebardown.ptstr = ptstr;
		rebardown.ptend = ptend;
		rebardown.sec = 1;

		RebarPoint rebarleft,rebarright;
		rebarleft.ptstr = strproject;
		rebarleft.ptend = ptstr;
		rebarleft.sec = 2;

		rebarright.ptstr = endproject;
		rebarright.ptend = ptend;
		rebarright.sec = 3;
		m_orgdata.mapselectrebars[dia].push_back(ptpoint);
		m_orgdata.mapselectrebars[dia].push_back(rebardown);
		m_orgdata.mapselectrebars[dia].push_back(rebarleft);
		m_orgdata.mapselectrebars[dia].push_back(rebarright);
	}
	else if (m_setdata.refdotrebar.size()==2)
	{
		EditElementHandle eehreb1(m_setdata.refdotrebar[0], m_setdata.refdotrebar[0]->GetDgnModelP());
		EditElementHandle eehreb2(m_setdata.refdotrebar[1], m_setdata.refdotrebar[1]->GetDgnModelP());

		RebarPoint ptleft,ptright;
		GetStartEndPointFromRebar(&eehreb1, ptleft.ptstr, ptleft.ptend, diameter);
		GetStartEndPointFromRebar(&eehreb2, ptright.ptstr, ptright.ptend, diameter);

		ptleft.ptstr.z = ptright.ptstr.z = ptpoint.ptstr.z;

		ExtendLine(ptleft.ptstr, ptright.ptstr, diameter/Get_uor_per_mm);
		mdlProject_perpendicular(&ptleft.ptend, NULL, NULL, eehrebar.GetElementDescrP(), ACTIVEMODEL, &ptleft.ptstr, NULL, 0.001);
		mdlProject_perpendicular(&ptright.ptend, NULL, NULL, eehrebar.GetElementDescrP(), ACTIVEMODEL, &ptright.ptstr, NULL, 0.001);
		ptleft.sec = 2;
		ptright.sec = 3;

		RebarPoint ptdown;
		ptdown.ptstr = ptleft.ptstr;
		ptdown.ptend = ptright.ptstr;
		ptdown.sec = 1;
		m_orgdata.mapselectrebars[dia].push_back(ptpoint);
		m_orgdata.mapselectrebars[dia].push_back(ptdown);
		m_orgdata.mapselectrebars[dia].push_back(ptleft);
		m_orgdata.mapselectrebars[dia].push_back(ptright);
	}
	else
	{
		return;
	}
	CalculateVertexAndDrawLinesWhenFourRebar();

}
void  CAddVerticalRebarDlg::CalculateVertexAndDrawLinesWhenTwoRebar()
{
	m_orgdata.mapselectrebars.clear();
	int tmpnum = 0;//0,上；1，下；2，左；3，右；
	for (ElementRefP ref : m_orgdata.selectrebars)
	{
		EditElementHandle eeh(ref, ref->GetDgnModelP());
		double diameter = 0;
		RebarPoint ptpoint;
		ptpoint.sec = tmpnum++;
		GetStartEndPointFromRebar(&eeh, ptpoint.ptstr, ptpoint.ptend, diameter);

		RebarPoint tmppt = ptpoint;
		MoveRebarPointByNormal(tmppt, 500);
		tmppt.sec = tmpnum++;
		int dia = (int)(diameter / (Get_uor_per_mm));
		m_orgdata.mapselectrebars[dia].push_back(ptpoint);
		m_orgdata.mapselectrebars[dia].push_back(tmppt);
	}
	CalculateVertexAndDrawLinesWhenFourRebar();
	
}
void CAddVerticalRebarDlg::CalculateVertexAndDrawLinesWhenFourRebar()
{
	vector<DPoint3d> interpts; 
	GetInterSectPointsByRebarmap(m_orgdata.mapselectrebars, interpts);
	if (m_caldata.vecrefpt.size() > 0)
	{
		DgnModelRefP        modelRef = ACTIVEMODEL;
		double minZ = (m_caldata.vecrefpt[0].ptstr.z < m_caldata.vecrefpt[0].ptend.z) ? m_caldata.vecrefpt[0].ptstr.z : m_caldata.vecrefpt[0].ptend.z;
		double maxZ = (m_caldata.vecrefpt[0].ptstr.z > m_caldata.vecrefpt[0].ptend.z) ? m_caldata.vecrefpt[0].ptstr.z : m_caldata.vecrefpt[0].ptend.z;
		CString tmpdiameter;
		m_edit_rebardiameter.GetWindowTextW(tmpdiameter);
		BrString Sizekey(tmpdiameter);
		int i = 0;
		for (DPoint3d tmppt : interpts)
		{
			bool isContinue = true;
			if (i==0&&m_setdata.genuprebar&&m_setdata.genleftrebar)
			{
				isContinue = false;
			}
			else if (i==1&&m_setdata.genuprebar&&m_setdata.genrightrebar)
			{
				isContinue = false;
			}
			else if (i==2&&m_setdata.gendownrebar&&m_setdata.genrightrebar)
			{
				isContinue = false;
			}
			else if (i==3&&m_setdata.gendownrebar&&m_setdata.genleftrebar)
			{
				isContinue = false;
			}
			if (m_setdata.refline != nullptr&&(i==0||i==1))//如果是只有一根横筋，且参考线段不为空时
			{
				isContinue = false;
			}
			if (!isContinue)
			{
				RebarVertices  vers;
				RebarVertex*   vertmp1; RebarVertex* vertmp2;
				tmppt.z = minZ;
				vertmp1 = new RebarVertex();
				vertmp1->SetType(RebarVertex::kStart);
				vertmp1->SetIP(tmppt);
				vers.Add(vertmp1);

				tmppt.z = maxZ;
				vertmp2 = new RebarVertex();
				vertmp2->SetType(RebarVertex::kEnd);
				vertmp2->SetIP(tmppt);
				vers.Add(vertmp2);

				m_caldata.rebarPts.push_back(vers);
				m_caldata.vecDir.push_back(Sizekey);
			}
			i++;
		}
		//计算除开四个角的其他点筋
		for (int i = 0; i < interpts.size(); i++)
		{
			bool isContinue = true;
			if (i == 0 && m_setdata.genuprebar)
			{
				isContinue = false;
			}
			else if (i == 1 && m_setdata.genrightrebar)
			{
				isContinue = false;
			}
			else if (i == 2 && m_setdata.gendownrebar)
			{
				isContinue = false;
			}
			else if (i == 3 &&m_setdata.genleftrebar)
			{
				isContinue = false;
			}
			if (!isContinue)
			{
				int j = i + 1;
				if (j == interpts.size())
				{
					j = 0;
				}
				DPoint3d ptstrline = interpts[i];
				DPoint3d ptendline = interpts[j];

				DPoint3d vectmp = ptendline - ptstrline;
				vectmp.Normalize();

				int numbar = 0;
				numbar = (int)(ptstrline.Distance(ptendline) / m_setdata.spacing);
				double sideSpacing = m_setdata.spacing;
				if (m_setdata.isuseavespacing)//使用平均间距
				{
					sideSpacing = ptstrline.Distance(ptendline) / (numbar + 1);
				}
				for (int k = 0; k < numbar; k++)
				{
					DPoint3d tmpPt = ptstrline;
					vectmp.Scale((k + 1)*sideSpacing);
					mdlVec_addPoint(&tmpPt, &tmpPt, &vectmp);
					vectmp.Normalize();
					RebarVertices  vers;
					RebarVertex*   vertmp1; RebarVertex* vertmp2;
					tmpPt.z = minZ;
					vertmp1 = new RebarVertex();
					vertmp1->SetType(RebarVertex::kStart);
					vertmp1->SetIP(tmpPt);
					vers.Add(vertmp1);

					tmpPt.z = maxZ;
					vertmp2 = new RebarVertex();
					vertmp2->SetType(RebarVertex::kEnd);
					vertmp2->SetIP(tmpPt);
					vers.Add(vertmp2);

					m_caldata.rebarPts.push_back(vers);
					m_caldata.vecDir.push_back(Sizekey);
				}
			}
		}
	}
		
	for (RebarVertices vers : m_caldata.rebarPts)
	{
		for (int i = 0; i < vers.GetSize() - 1; i++)
		{
			RebarVertex   ver1 = vers.At(i);
			RebarVertex   ver2 = vers.At(i + 1);
			CPoint3D const&     pt1 = ver1.GetIP();
			CPoint3D const&     pt2 = ver2.GetIP();
			DPoint3d tpt1 = DPoint3d::From(pt1.x, pt1.y, pt1.z);
			DPoint3d tpt2 = DPoint3d::From(pt2.x, pt2.y, pt2.z);
			EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(tpt1, tpt2), true, *ACTIVEMODEL);
			eeh.AddToModel();

			EditElementHandle eeharc;
			ArcHandler::CreateArcElement(eeharc, nullptr, DEllipse3d::FromCenterRadiusXY(tpt1, m_setdata.diameter/2), true, *ACTIVEMODEL);
			eeharc.AddToModel();

			m_caldata.allLines.push_back(eeharc.GetElementRef());
			m_caldata.allLines.push_back(eeh.GetElementRef());
		}
	}
}
void CAddVerticalRebarDlg::CalculateVertexAndDrawLines()
{
	GetSetDataFromWindow();
	ClearLines();
	if (m_orgdata.selectrebars.size() < 0)
	{
		return;
	}
	if (m_orgdata.selectrebars.size()==1)//1根横筋时的处理
	{

		CalculateVertexAndDrawLinesWhenOneRebar();

	}else if (m_orgdata.selectrebars.size() == 2)//2根横筋时的处理
	{

		CalculateVertexAndDrawLinesWhenTwoRebar();

	}
	else if (m_orgdata.selectrebars.size() == 4)//4根横筋时的处理
	{

		CalculateVertexAndDrawLinesWhenFourRebar();

	}
	
}
//计算得到计算参数
void CAddVerticalRebarDlg::CalculatecalData()
{
	m_caldata.vecrefpt.clear();
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (m_orgdata.selectrebars.size() > 0)
	{
		int tmpnum = 0;//0,上；1，下；2，左；3，右；
		for (ElementRefP ref : m_orgdata.selectrebars)
		{
			EditElementHandle eeh(ref, ref->GetDgnModelP());
			double diameter = 0;
			RebarPoint ptpoint;
			ptpoint.sec = tmpnum++;
			GetStartEndPointFromRebar(&eeh, ptpoint.ptstr, ptpoint.ptend, diameter);
			int dia = (int)(diameter / (uor_per_mm));
			m_orgdata.mapselectrebars[dia].push_back(ptpoint);
		}
		//设置点筋的直径为选中的直径中最大的直径
		double DiameterLong = m_orgdata.mapselectrebars.rbegin()->first*uor_per_mm;
		char tmpDiam[256];
		sprintf(tmpDiam, "%d", (int)(DiameterLong / uor_per_mm));
		BrString Sizekey(tmpDiam);
		m_edit_rebardiameter.SetWindowTextW(Sizekey.Get());
		m_setdata.diameter = DiameterLong;
		//计算点筋直径，如果没有点筋，使用默认的直径
		RebarSet * rebset = nullptr;
		EditElementHandle start(m_orgdata.selectrebars[0], m_orgdata.selectrebars[0]->GetDgnModelP());
		if (RebarElement::IsRebarElement(start))
		{
			RebarElementP rep = RebarElement::Fetch(start);
			rebset = rep->GetRebarSet(ACTIVEMODEL);
			if (rebset != nullptr)
			{
				ElementId conid;
				int rebar_cage_type;
				conid = rebset->GetConcreteOwner(ACTIVEMODEL, rebar_cage_type);
				RebarAssemblies reas;
				RebarAssembly::GetRebarAssemblies(conid, reas);
				RebarAssembly* rebarasb = nullptr;
				for (int i = 0; i < reas.GetSize(); i++)
				{
					RebarAssembly* rebaras = reas.GetAt(i);
					if (rebaras->GetCallerId() == rebset->GetCallerId())
					{
						rebarasb = rebaras;
					}

				}
				if (rebarasb != nullptr)
				{
					DgnModelRefP        modelRef = ACTIVEMODEL;
					SingleRebarAssembly*  slabRebar = REA::Create<SingleRebarAssembly>(modelRef);

					ElementId tmpid = rebarasb->GetSelectedElement();
					if (tmpid == 0)
					{
						return;
					}
					DgnModelRefP modelp = rebarasb->GetSelectedModel();
					if (modelp == nullptr)
					{
						if (m_orgdata.ehSel.FindByID(tmpid, ACTIVEMODEL) != SUCCESS)
						{
							ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
							for (DgnModelRefP modelRef : modelRefCol)
							{
								if (m_orgdata.ehSel.FindByID(tmpid, modelRef) == SUCCESS)
								{
									modelp = modelRef;
									break;
								}

							}
						}
					}
					else
					{
						m_orgdata.ehSel.FindByID(tmpid, modelp);
					}
					std::vector<RebarPoint> RebarPts;
					GetElementXAttribute(conid, RebarPts, vecRebarPointsXAttribute, ACTIVEMODEL);
					//取最小Z和最大Z值
					double minZ = 0; double maxZ = 0;
					for (RebarPoint tmppt : RebarPts)
					{
						if (tmppt.vecDir == 1)//竖着的钢筋
						{
							m_caldata.vecrefpt.push_back(tmppt);
							break;
						}
					}				
				}
			}
		}
	
	}
}
void CAddVerticalRebarDlg::ExtendLine(DPoint3d& ptstr, DPoint3d& ptend, double dis)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	DPoint3d vecnormal = ptend - ptstr;
	vecnormal.Normalize();
	vecnormal.Scale(dis * uor_per_mm);
	mdlVec_addPoint(&ptend, &ptend, &vecnormal);
	vecnormal.Scale(-1);
	mdlVec_addPoint(&ptstr, &ptstr, &vecnormal);
}
//计算钢筋最长线段之间的交点
void CAddVerticalRebarDlg::GetIntersetPointsRebarWithRebar(vector<RebarPoint>& rebarpts, vector<DPoint3d>& interpts)
{

	if (rebarpts.size()!=4)
	{
		return;
	}
	double PosZ = rebarpts[0].ptstr.z;
	for (int i = 0; i < rebarpts.size(); i++)
	{
		rebarpts[i].ptstr.z = rebarpts[i].ptend.z = PosZ;
		ExtendLine(rebarpts[i].ptstr, rebarpts[i].ptend, 500);
	}
	RebarPoint upPt, downPt, leftPt, rightPt;
	upPt = rebarpts[0];
	downPt = rebarpts[1];
	leftPt = rebarpts[2];
	rightPt = rebarpts[3];

	DPoint3d intersectpt;
	if (SUCCESS == mdlVec_intersect(&intersectpt, &DSegment3d::From(upPt.ptstr, upPt.ptend), &DSegment3d::From(leftPt.ptstr, leftPt.ptend)))//点1，上左
	{
		interpts.push_back(intersectpt);
	}
	if (SUCCESS == mdlVec_intersect(&intersectpt, &DSegment3d::From(upPt.ptstr, upPt.ptend), &DSegment3d::From(rightPt.ptstr, rightPt.ptend)))//点2，上右
	{
		interpts.push_back(intersectpt);
	}
	if (SUCCESS == mdlVec_intersect(&intersectpt, &DSegment3d::From(downPt.ptstr, downPt.ptend), &DSegment3d::From(rightPt.ptstr, rightPt.ptend)))//点3，下右
	{
		interpts.push_back(intersectpt);
	}
	if (SUCCESS == mdlVec_intersect(&intersectpt, &DSegment3d::From(downPt.ptstr, downPt.ptend), &DSegment3d::From(leftPt.ptstr, leftPt.ptend)))//点4，下左
	{
		interpts.push_back(intersectpt);
	}

	/*double PosZ = rebarpts[0].ptstr.z;
	for (int i = 0; i < rebarpts.size(); i++)
	{
		if (i == 0)
		{
			rebarpts[i].ptstr.z = rebarpts[i].ptend.z = PosZ;
			ExtendLine(rebarpts[i].ptstr, rebarpts[i].ptend, 500);
		}
		for (int j = i + 1; j < rebarpts.size(); j++)
		{
			if (i == 0)
			{
				rebarpts[j].ptstr.z = rebarpts[j].ptend.z = PosZ;
				ExtendLine(rebarpts[j].ptstr, rebarpts[j].ptend, 500);
			}
			DPoint3d intersectpt;
			if (SUCCESS == mdlVec_intersect(&intersectpt, &DSegment3d::From(rebarpts[i].ptstr, rebarpts[i].ptend), &DSegment3d::From(rebarpts[j].ptstr, rebarpts[j].ptend)))
			{
				interpts.push_back(intersectpt);
			}
		}
	}
	if (interpts.size() == 4)
	{
		if (interpts[1].Distance(interpts[2]) > interpts[1].Distance(interpts[3]))
		{
			DPoint3d tmpPt = interpts[2];
			interpts[2] = interpts[3];
			interpts[3] = tmpPt;
		}
	}*/

}
void CAddVerticalRebarDlg::GetInterSectPointsByRebarmap(map<int, vector<RebarPoint>>& mapselectrebars, vector<DPoint3d>& interpts)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	map<int, vector<RebarPoint>>::iterator itr = mapselectrebars.begin();
	vector<RebarPoint> allpts;
	if (itr->second.size()>0)
	{
		RebarPoint tmppt = itr->second[0];
		for (int i=0;i<4;i++)
		{
			allpts.push_back(tmppt);
		}
		
	}
	for (; itr != mapselectrebars.end(); itr++)
	{
		if (itr->second.size() > 0)
		{		
			for (RebarPoint rbpt:itr->second)
			{
				allpts[rbpt.sec] = rbpt;
			}
			
		}
	}
	vector<DPoint3d> tmpinterpts;
	GetIntersetPointsRebarWithRebar(allpts, tmpinterpts);

	double DiameterLong = m_setdata.diameter;
	if (tmpinterpts.size() == 4)
	{
		//计算四条线相交区域的中心点
		DPoint3d Centerpt;
		Centerpt = tmpinterpts[0];
		Centerpt.Add(tmpinterpts[1]);
		Centerpt.Scale(0.5);
		DPoint3d tmppt = tmpinterpts[2];
		tmppt.Add(tmpinterpts[3]);
		tmppt.Scale(0.5);
		Centerpt.Add(tmppt);
		Centerpt.Scale(0.5);

		//将四条线操中心移动钢筋直径的距离，使得线上点即为钢筋的中心点
		
		vector<RebarPoint> allptsmove;
		itr = mapselectrebars.begin();
		for (; itr != mapselectrebars.end(); itr++)
		{
			if (itr->second.size() > 0)
			{
				for (RebarPoint tmppt : itr->second)
				{
					DPoint3d vecLine = tmppt.ptend - tmppt.ptstr;
					vecLine.Normalize();
					DPoint3d vecnormal = DPoint3d::From(0, 0, 1);
					vecnormal.CrossProduct(vecnormal, vecLine);

					DPoint3d midpt = tmppt.ptstr;
					midpt.Add(tmppt.ptend);
					midpt.Scale(0.5);
					midpt = Centerpt - midpt;
					midpt.z = 0;
					midpt.Normalize();

					if (midpt.DotProduct(vecnormal) < 0)//如果法相与中心点不在一侧，将法相反向
					{
						vecnormal.Scale(-1);
					}
					double moveDis;
                    if (tmppt.sec==0)//上
                    {
					   moveDis = (itr->first*uor_per_mm / 2 + abs(m_setdata.upDis))*m_setdata.upDis/abs(m_setdata.upDis);
                    }
					else if (tmppt.sec == 1)//下
					{
						moveDis = (itr->first*uor_per_mm / 2 + abs(m_setdata.downdis))*m_setdata.downdis / abs(m_setdata.downdis);
					}
					else if (tmppt.sec ==2)//左
					{
						moveDis = (itr->first*uor_per_mm / 2 + abs(m_setdata.leftdis))*m_setdata.leftdis / abs(m_setdata.leftdis);
					}
					else
					{
						moveDis = (itr->first*uor_per_mm / 2 + abs(m_setdata.rightdis))*m_setdata.rightdis / abs(m_setdata.rightdis);
					}
					vecnormal.Scale(moveDis);
					ExtendLine(tmppt.ptstr, tmppt.ptend, 500);
					mdlVec_addPoint(&tmppt.ptstr, &tmppt.ptstr, &vecnormal);
					mdlVec_addPoint(&tmppt.ptend, &tmppt.ptend, &vecnormal);
					allptsmove.push_back(tmppt);

				}

			}
		}
		GetIntersetPointsRebarWithRebar(allptsmove, interpts);
	}

}
void  CAddVerticalRebarDlg::DrawVerticalReabr()
{
	if (m_orgdata.selectrebars.size() < 1)
	{
		return;
	}
	for (int i = 0; i < m_caldata.vecDir.size();i++)
	{
		int temp=Typenum;
		GetDiameterAddType(m_caldata.vecDir[i], temp);
	}
//画点筋
	RebarSet * rebset = nullptr;
	EditElementHandle start(m_orgdata.selectrebars[0], m_orgdata.selectrebars[0]->GetDgnModelP());
	if (RebarElement::IsRebarElement(start))
	{
		RebarElementP rep = RebarElement::Fetch(start);
		rebset = rep->GetRebarSet(ACTIVEMODEL);
		if (rebset != nullptr&&m_caldata.rebarPts.size()>0)
		{
			DgnModelRefP        modelRef = ACTIVEMODEL;
			SingleRebarAssembly*  slabRebar = REA::Create<SingleRebarAssembly>(modelRef);

			slabRebar->SetSelectedRebar(start,true);
			slabRebar->SetSlabData(m_orgdata.ehSel);
			slabRebar->SetvecDirSize(m_caldata.vecDir);
			slabRebar->Settypenum(Typenum);
			slabRebar->SetrebarPts(m_caldata.rebarPts);
			slabRebar->Setspacing(rebset->GetSetData().GetNominalSpacing());
			slabRebar->MakeRebars(modelRef);
			slabRebar->Save(modelRef); // must save after creating rebars
			m_caldata.rebarPts.clear();
			m_caldata.vecDir.clear();
			m_orgdata.selectrebars.clear();

		}
	}
}

void CAddVerticalRebarDlg::OnEnKillfocusAtEditRebardiameter()
{
	CString tmpvale;
	m_edit_rebardiameter.GetWindowTextW(tmpvale);
	m_setdata.diameter = atof(CT2A(tmpvale));

	char tmpDiam[256];
	sprintf(tmpDiam, "%f", (float)(m_setdata.diameter / 2.0));
	CString tmpdis(tmpDiam);
	m_edit_upmove.SetWindowTextW(tmpdis);
	m_edit_downmove.SetWindowTextW(tmpdis);
	m_edit_leftmove.SetWindowTextW(tmpdis);
	m_rightmove.SetWindowTextW(tmpdis);
	GetSetDataFromWindow();
	CalculateVertexAndDrawLines();
	// TODO: 在此添加控件通知处理程序代码
}


void CAddVerticalRebarDlg::OnEnKillfocusAtEditUpmove()
{
	// TODO: 在此添加控件通知处理程序代码
	GetSetDataFromWindow();
	CalculateVertexAndDrawLines();
}


void CAddVerticalRebarDlg::OnEnKillfocusAtEditLeftmove()
{
	// TODO: 在此添加控件通知处理程序代码
	GetSetDataFromWindow();
	CalculateVertexAndDrawLines();
}


void CAddVerticalRebarDlg::OnEnKillfocusAtEditDownmove()
{
	// TODO: 在此添加控件通知处理程序代码
	GetSetDataFromWindow();
	CalculateVertexAndDrawLines();
}


void CAddVerticalRebarDlg::OnEnKillfocusAtEditRightmove()
{
	// TODO: 在此添加控件通知处理程序代码
	
	GetSetDataFromWindow();
	CalculateVertexAndDrawLines();
}


void CAddVerticalRebarDlg::OnBnClickedAdCheckUprebar()
{
	// TODO: 在此添加控件通知处理程序代码
	GetSetDataFromWindow();
	CalculateVertexAndDrawLines();
	if (m_check_uprebar.GetCheck())
	{
		m_edit_upmove.EnableWindow(TRUE);
	}
	else
	{
		m_edit_upmove.EnableWindow(FALSE);
	}
}


void CAddVerticalRebarDlg::OnBnClickedAdCheckDownrebar()
{
	// TODO: 在此添加控件通知处理程序代码
	GetSetDataFromWindow();
	CalculateVertexAndDrawLines();
	if (m_check_downrebar.GetCheck())
	{
		m_edit_downmove.EnableWindow(TRUE);
	}
	else
	{
		m_edit_downmove.EnableWindow(FALSE);
	}
}


void CAddVerticalRebarDlg::OnBnClickedAdCheckLeftrebar()
{
	// TODO: 在此添加控件通知处理程序代码
	GetSetDataFromWindow();
	CalculateVertexAndDrawLines();
	if (m_check_leftrebar.GetCheck())
	{
		m_edit_leftmove.EnableWindow(TRUE);
	}
	else
	{
		m_edit_leftmove.EnableWindow(FALSE);
	}
}


//void CAddVerticalRebarDlg::OnBnKillfocusAdCheckRightrebar()
//{
//	
//}


void CAddVerticalRebarDlg::OnBnClickedAdCheckRightrebar()
{
	// TODO: 在此添加控件通知处理程序代码
	GetSetDataFromWindow();
	CalculateVertexAndDrawLines();
	if (m_check_rightrebar.GetCheck())
	{
		m_rightmove.EnableWindow(TRUE);
	}
	else
	{
		m_rightmove.EnableWindow(FALSE);
	}
}


void CAddVerticalRebarDlg::OnBnClickedAtCheckIsavespacing()
{
	// TODO: 在此添加控件通知处理程序代码
	GetSetDataFromWindow();
	CalculateVertexAndDrawLines();
}


void CAddVerticalRebarDlg::OnEnKillfocusAtEditSpacingdis()
{
	// TODO: 在此添加控件通知处理程序代码
	GetSetDataFromWindow();
	CalculateVertexAndDrawLines();
}


void CAddVerticalRebarDlg::OnBnClickedAtButtonSelectrefrebar()
{
	// TODO: 在此添加控件通知处理程序代码
	SelectRebarTool::InstallNewInstanceVertical(CMDNAME_RebarSDKReadRebar, this);
}


void CAddVerticalRebarDlg::OnBnClickedAtButtonSelectrefline()
{
	// TODO: 在此添加控件通知处理程序代码
	SelectLineTool::InstallNewInstance(this);
}


void CAddVerticalRebarDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	ClearLines();
	CDialogEx::OnCancel();
}


void CAddVerticalRebarDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	ClearLines();
	CDialogEx::OnClose();
}


void CAddVerticalRebarDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0; //根据当前dgn模型获取比例因子
	int nIndex = m_type.GetCurSel();
	Typenum = nIndex;
	CString rebarDiaStr;
	m_type.GetLBText(nIndex, rebarDiaStr);
}
