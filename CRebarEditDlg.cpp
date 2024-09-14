// CRebarEditDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CRebarEditDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "SingleRebarAssembly.h"
#include <RebarCatalog.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "ElementAttribute.h"
#include "SelectRebarTool.h"
#include "XmlHelper.h"
#include "PITRebarCurve.h"
#include "ExtractFacesTool.h"
#include "HoleRebarAssembly.h"

// CRebarEditDlg 对话框

IMPLEMENT_DYNAMIC(CRebarEditDlg, CDialogEx)

CRebarEditDlg::CRebarEditDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_REBAREDIT, pParent)
{
	
}

CRebarEditDlg::~CRebarEditDlg()
{
	for (int i = 0; i < m_Holeehs.size();)
	{
		if (m_Holeehs.at(i) != nullptr)
		{
			delete m_Holeehs.at(i);
			m_Holeehs.at(i) = nullptr;
		}
	}
	m_Holeehs.clear();
}

void CRebarEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_RMOVELENTH, m_edit_movelenth);
	DDX_Control(pDX, IDC_EDIT_RL0Lenth, m_edit_L0Lenth);
	DDX_Control(pDX, IDC_COMBO_RTYPE, m_combo_type);
	DDX_Control(pDX, IDC_CHECK_RUseEnd, m_check_useend);
	DDX_Control(pDX, IDC_EDIT_RRotate, m_edit_rotateangle);
}


BEGIN_MESSAGE_MAP(CRebarEditDlg, CDialogEx)
	ON_EN_KILLFOCUS(IDC_EDIT_RMOVELENTH, &CRebarEditDlg::OnEnKillfocusEditRmovelenth)
	ON_EN_KILLFOCUS(IDC_EDIT_RL0Lenth, &CRebarEditDlg::OnEnKillfocusEditRl0lenth)
	ON_CBN_SELCHANGE(IDC_COMBO_RTYPE, &CRebarEditDlg::OnCbnSelchangeComboRtype)
	ON_BN_CLICKED(IDC_BUTTON_RSELECTREBARS, &CRebarEditDlg::OnBnClickedButtonRselectrebars)
	ON_BN_CLICKED(IDOK, &CRebarEditDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CRebarEditDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_CHECK_RUseEnd, &CRebarEditDlg::OnBnClickedCheckRuseend)
	ON_BN_CLICKED(IDC_RBUTTON_ROTATE, &CRebarEditDlg::OnBnClickedRbuttonRotate)
	ON_EN_KILLFOCUS(IDC_EDIT_RRotate, &CRebarEditDlg::OnEnKillfocusEditRrotate)
END_MESSAGE_MAP()


// CRebarEditDlg 消息处理程序
// CInsertRebarDlg 消息处理程序
BOOL CRebarEditDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	m_rebareditdata.l0lenth = 500;
	m_rebareditdata.movelenth = 24;
	m_rebareditdata.isend = false;
	m_rebareditdata.type = AnchorInType::Curve;
	m_rebareditdata.angle = 90;
	m_rebareditdata.isArc = false;
	for each (auto var in g_listRebarAnchorInType)
		m_combo_type.AddString(var);
	UpdateWindow();
	// m_EditElementLength.SetWindowText();
	// 长度
	return TRUE;  // return TRUE unless you set the focus to a control
			  // 异常: OCX 属性页应返回 FALSE
}
void CRebarEditDlg::UpdateWindow()
{
	CString sTemp = CString();
	sTemp.Format(_T("%.3f"), m_rebareditdata.l0lenth);
	m_edit_L0Lenth.SetWindowText(sTemp);

	sTemp.Empty();
	sTemp.Format(_T("%.3f"), m_rebareditdata.movelenth);
	m_edit_movelenth.SetWindowText(sTemp);

	sTemp.Empty();
	sTemp.Format(_T("%.3f"), m_rebareditdata.angle);
	m_edit_rotateangle.SetWindowText(sTemp);

	m_combo_type.SetCurSel(m_rebareditdata.type);
}
void CRebarEditDlg::ClearLines()
{
	for (ElementRefP tmpeeh : m_allLines)
	{
		if (tmpeeh != nullptr)
		{
			EditElementHandle eeh(tmpeeh, tmpeeh->GetDgnModelP());
			eeh.DeleteFromModel();
		}
	}
	m_allLines.clear();
	m_rebarPts.clear();
	m_vecDir.clear();
}

void CRebarEditDlg::SetIsEnd(bool isEnd)
{
	m_rebareditdata.isend = isEnd;
	m_check_useend.SetCheck(isEnd);
	CalculateVertexAndDrawLines();
}

double GetVertexsLenth(RebarVertices& vers)
{
	double Lenth = 0;
	RebarCurve tmpCurve;
	for (int i = 0; i < vers.GetSize(); i++)
	{
		RebarVertexP vex;
		vex = &tmpCurve.PopVertices().NewElement();
		vex->SetIP(vers.At(i).GetIP());
		vex->SetArcPt(0, vers.At(i).GetArcPt(0));
		vex->SetArcPt(1, vers.At(i).GetArcPt(1));
		vex->SetArcPt(2, vers.At(i).GetArcPt(2));
		vex->SetCenter(vers.At(i).GetCenter());
		vex->SetRadius(vers.At(i).GetRadius());
		vex->SetTanPt(0, vers.At(i).GetTanPt(0));
		vex->SetTanPt(1, vers.At(i).GetTanPt(1));
		vex->SetType(vers.At(i).GetType());
	}
	
	return tmpCurve.GetLength();

}


void CRebarEditDlg::CalculateVertexAndDrawLines()
{
	
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (m_Anchorinrebars.size() < 2 || m_selectrebars.size() == 0)
		return;

	    ClearLines();
		RebarCurve curve;
		EditElementHandle start(m_selectrebars[0], m_selectrebars[0]->GetDgnModelP());
		

		EditElementHandle end1(m_Anchorinrebars[0], m_Anchorinrebars[0]->GetDgnModelP());
		EditElementHandle end2(m_Anchorinrebars[1], m_Anchorinrebars[1]->GetDgnModelP());
		vector<EditElementHandleP> endEehs = { &end1, &end2 };
		vector<DPoint3d> NormalPts;
		if (!GetStartEndAndRadius(&start, endEehs, m_rebareditdata.movelenth*uor_per_mm, NormalPts))
		{
			return ;
		}
		RebarSet * rebset = nullptr;
		int firstnum = 0;
		for (int i = 0; i< m_selectrebars.size(); i++)
		{
			EditElementHandle elementToModify(m_selectrebars[i], m_selectrebars[i]->GetDgnModelP());
			if (RebarElement::IsRebarElement(elementToModify))
			{
				RebarElementP rep = RebarElement::Fetch(elementToModify);
				double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
				RebarShape * rebarshape = rep->GetRebarShape(elementToModify.GetModelRef());
				if (rebarshape == nullptr)
				{
					continue;
				}
				rebarshape->GetRebarCurve(curve);
				BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
				double diameter = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);
				double bendRadius = RebarCode::GetPinRadius(Sizekey, ACTIVEMODEL, false);
				//double bendLen = RebarCode::GetBendLength(L"32", endType, ACTIVEMODEL);

				CMatrix3D tmp3d(rep->GetLocation());
				curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
				curve.DoMatrix(rep->GetLocation());
				RebarVertices  vers = curve.PopVertices();
				double MoveLenth = 0;
				if (m_rebareditdata.type== AnchorInType::Curve)
				{
					double SouceLenth = GetVertexsLenth(vers);
					double disLenth = m_rebareditdata.l0lenth*uor_per_mm + bendRadius;
					GetRebarVertices(vers, curve, NormalPts, bendRadius,
						disLenth, m_rebareditdata.angle, m_rebareditdata.isend);
					double NowLenth = GetVertexsLenth(vers);
					double LaLenth = g_globalpara.m_alength[(string)Sizekey] * uor_per_mm + 50*uor_per_mm;
					if (LaLenth > NowLenth - SouceLenth)
					{
						MoveLenth = LaLenth - (NowLenth - SouceLenth);
					}
				}
				else
				{
					GetRebarVerticesWhenStraightLenth(vers, curve, NormalPts, m_rebareditdata.l0lenth*uor_per_mm, m_rebareditdata.isend);
				}
				
				for (int i = 0; i < vers.GetSize() - 1; i++)
				{
					RebarVertex   ver1 = vers.At(i);
					RebarVertex   ver2 = vers.At(i + 1);
					CPoint3D     pt1 = ver1.GetIP();
					CPoint3D     pt2  = ver2.GetIP();
					if (i==vers.GetSize()-2&&MoveLenth>0)//将尾部延长
					{
						DPoint3d vecEnd = pt2 - pt1;
						vecEnd.Normalize();
						vecEnd.Scale(MoveLenth);
						pt2.Add(vecEnd);
						vers.At(i + 1).SetIP(pt2);
					}

					/*规避孔洞处理*/
						if (i==0)
						{
							DPoint3d tmpptstr = pt1;
							DPoint3d tmpend = pt2;
							CalculateIntersetPtWithHolesWithRebarCuve(tmpptstr, tmpend, tmpend, m_Holeehs);
							pt1 = tmpptstr;
							pt2 = tmpend;
							vers.At(i).SetIP(pt1);
							vers.At(i + 1).SetIP(pt2);
						}
						else if (i == vers.GetSize() - 2)
						{
							DPoint3d tmpptstr = pt1;
							DPoint3d tmpend = pt2;
							CalculateIntersetPtWithHolesWithRebarCuve(tmpptstr, tmpend, tmpptstr, m_Holeehs);
							pt1 = tmpptstr;
							pt2 = tmpend;
							vers.At(i).SetIP(pt1);
							vers.At(i + 1).SetIP(pt2);
						}
					/*规避孔洞处理*/
					/*CPoint3D const&     pt1 = (ver1.GetType() == RebarVertex::kStart || ver1.GetType() == RebarVertex::kEnd) ? ver1.GetIP() : ver1.GetArcPt(2);
					CPoint3D const&     pt2 = (ver2.GetType() == RebarVertex::kStart || ver2.GetType() == RebarVertex::kEnd) ? ver2.GetIP() : ver2.GetArcPt(0);*/

					DPoint3d tpt1 = DPoint3d::From(pt1.x, pt1.y, pt1.z);
					DPoint3d tpt2 = DPoint3d::From(pt2.x, pt2.y, pt2.z);					
					EditElementHandle eeh;
					LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(tpt1, tpt2), true, *ACTIVEMODEL);
					eeh.AddToModel();
					m_allLines.push_back(eeh.GetElementRef());
				}
				if (firstnum == 0)
				{
					rebset = rep->GetRebarSet(ACTIVEMODEL);
					firstnum++;
				}
				m_rebarPts.push_back(vers);
				m_vecDir.push_back(Sizekey);
			}

		}
	
}

void CRebarEditDlg::CalculterArcVertexAndDrawLines()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (m_Anchorinrebars.size() < 3 || m_selectrebars.size() == 0 || !m_rebareditdata.isArc)
		return;

	ClearLines();
	RebarCurve curve;
	EditElementHandle start(m_selectrebars[0], m_selectrebars[0]->GetDgnModelP());

	EditElementHandle end1(m_Anchorinrebars[0], m_Anchorinrebars[0]->GetDgnModelP());
	EditElementHandle end2(m_Anchorinrebars[1], m_Anchorinrebars[1]->GetDgnModelP());
	EditElementHandle end3(m_Anchorinrebars[2], m_Anchorinrebars[2]->GetDgnModelP());
	vector<EditElementHandleP> endEehs = { &end1, &end2, &end3};
	vector<DPoint3d> NormalPts;
	if (!GetStartEndAndRadius(&start, endEehs, m_rebareditdata.movelenth*uor_per_mm, NormalPts))
	{
		return;
	}
	RebarSet * rebset = nullptr;
	int firstnum = 0;
	for (int i = 0; i < m_selectrebars.size(); i++)
	{
		EditElementHandle elementToModify(m_selectrebars[i], m_selectrebars[i]->GetDgnModelP());
		if (RebarElement::IsRebarElement(elementToModify))
		{
			RebarElementP rep = RebarElement::Fetch(elementToModify);
			double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
			RebarShape * rebarshape = rep->GetRebarShape(elementToModify.GetModelRef());
			if (rebarshape == nullptr)
			{
				continue;
			}
			rebarshape->GetRebarCurve(curve);
			BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
			double diameter = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);
			double bendRadius = RebarCode::GetPinRadius(Sizekey, ACTIVEMODEL, false);
			//double bendLen = RebarCode::GetBendLength(L"32", endType, ACTIVEMODEL);

			CMatrix3D tmp3d(rep->GetLocation());
			curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
			curve.DoMatrix(rep->GetLocation());
			RebarVertices  vers = curve.PopVertices();
			double MoveLenth = 0;
			if (m_rebareditdata.type == AnchorInType::Curve)
			{
				double SouceLenth = GetVertexsLenth(vers);
				double disLenth = m_rebareditdata.l0lenth*uor_per_mm + bendRadius;
				GetArcRebarVertices(vers, curve, NormalPts, bendRadius,
					disLenth, m_rebareditdata.angle, m_rebareditdata.isend);
				double NowLenth = GetVertexsLenth(vers);
				double LaLenth = g_globalpara.m_alength[(string)Sizekey] * uor_per_mm + 50 * uor_per_mm;
				if (LaLenth > NowLenth - SouceLenth)
				{
					MoveLenth = LaLenth - (NowLenth - SouceLenth);
				}
			}
			else
			{
				GetRebarVerticesWhenStraightLenth(vers, curve, NormalPts, m_rebareditdata.l0lenth*uor_per_mm, m_rebareditdata.isend);
			}

			for (int i = 0; i < vers.GetSize() - 2; i++)
			{
				if (i == 0 || i == 1)
				{
					RebarVertex   ver1 = vers.At(i);
					RebarVertex   ver2 = vers.At(i + 1);
					CPoint3D     pt1 = ver1.GetIP();
					CPoint3D     pt2 = ver2.GetIP();
					/*规避孔洞处理*/
					DPoint3d tmpptstr = pt1;
					DPoint3d tmpend = pt2;
					CalculateIntersetPtWithHolesWithRebarCuve(tmpptstr, tmpend, tmpend, m_Holeehs);
					pt1 = tmpptstr;
					pt2 = tmpend;
					vers.At(i).SetIP(pt1);
					vers.At(i + 1).SetIP(pt2);
					DPoint3d tpt1 = DPoint3d::From(pt1.x, pt1.y, pt1.z);
					DPoint3d tpt2 = DPoint3d::From(pt2.x, pt2.y, pt2.z);
					EditElementHandle eeh;
					LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(tpt1, tpt2), true, *ACTIVEMODEL);
					eeh.AddToModel();
					m_allLines.push_back(eeh.GetElementRef());
				}
				if (i == vers.GetSize() - 3)
				{
					RebarVertex ver1 = vers.At(i);
					RebarVertex ver2 = vers.At(i + 1);
					RebarVertex ver3 = vers.At(i + 2);
					CPoint3D arcStrPt = ver1.GetIP();
					CPoint3D arcMidPt = ver2.GetIP();
					CPoint3D arcEndPt = ver3.GetIP();
					//double lineLen = vers.At(i - 1).GetIP().Distance(arcStrPt);
					if (MoveLenth > 0)//将尾部延长
					{
						DEllipse3d ellipse = DEllipse3d::FromPointsOnArc(arcStrPt, arcMidPt, arcEndPt);
						EditElementHandle ellipseEeh;
						ArcHandler::CreateArcElement(ellipseEeh, nullptr, ellipse, true, *ACTIVEMODEL);
						double arcLen = ellipse.ArcLength();
						double startAngle = 0, sweepAngle = 0, radius = 0;
						DPoint3d centerPt = { 0,0,0 };
						mdlArc_extract(nullptr, &startAngle, &sweepAngle, &radius, nullptr, nullptr, &centerPt, ellipseEeh.GetElementP());
						GetPositiveAngle(startAngle, centerPt, arcStrPt);
						double starR = (360 - startAngle) / 180 * fc_pi;
						double angle = (MoveLenth + arcLen) * 180 / radius / PI / 10;
						MSElement outArc;
						mdlArc_create(&outArc, nullptr, &centerPt, radius, radius, nullptr, starR, angle);
						//DEllipse3d newEllipse = DEllipse3d::FromFractionInterval(ellipse, 0, (MoveLenth + arcLen - lineLen) / arcLen);
						//newEllipse.EvaluateEndPoints(arcStrPt, arcEndPt);
						DPoint3d arcPts[2];
						mdlArc_extract(arcPts, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &outArc);
						vers.At(i + 2).SetIP(arcEndPt[1]);
						//EditElementHandle eeh(&outArc, ACTIVEMODEL);
						//ArcHandler::CreateArcElement(eeh, nullptr, newEllipse, true, *ACTIVEMODEL);
						//eeh.AddToModel();
					}
					/*规避孔洞处理*/
					HoleArcRFRebarAssembly::CalculateIntersetPtWithHolesWithARCRebarCuve(arcStrPt, arcEndPt, arcMidPt, m_Holeehs);
					vers.At(i).SetIP(arcStrPt);
					vers.At(i + 1).SetIP(arcMidPt);
					vers.At(i + 2).SetIP(arcEndPt);
					EditElementHandle arcEeh;
					ArcHandler::CreateArcElement(arcEeh, nullptr, DEllipse3d::FromPointsOnArc(arcStrPt, arcMidPt, arcEndPt), true, *ACTIVEMODEL);
					arcEeh.AddToModel();
					m_allLines.push_back(arcEeh.GetElementRef());
				}
			}
			if (firstnum == 0)
			{
				rebset = rep->GetRebarSet(ACTIVEMODEL);
				firstnum++;
			}
			m_rebarPts.push_back(vers);
			m_vecDir.push_back(Sizekey);
		}

	}
}

void CRebarEditDlg::UpdateDataAndWindow()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (m_selectrebars.size() < 1 || m_Anchorinrebars.size() < 2)
	{
		return;
	}


	RebarElementP selectrebar, anchorrebar;
	double diameterselect, diameteranchorin;
	EditElementHandle eeh(m_selectrebars[0], m_selectrebars[0]->GetDgnModelP());
	EditElementHandle anchoreeh(m_Anchorinrebars[0], m_Anchorinrebars[0]->GetDgnModelP());

	selectrebar = RebarElement::Fetch(eeh);
	anchorrebar = RebarElement::Fetch(anchoreeh);
	if (selectrebar == nullptr||anchorrebar==nullptr)
	{
		return;
	}
	RebarShape * rebarshapeselect = selectrebar->GetRebarShape(ACTIVEMODEL);
	if (rebarshapeselect==nullptr)
	{
		return;
	}
	BrString Sizekey(rebarshapeselect->GetShapeData().GetSizeKey());
	diameterselect = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);
	//m_rebareditdata.l0lenth = g_globalpara.m_laplenth[BeStringUtilities::Wtoi(Sizekey)]/2;
	if (m_rebareditdata.type == AnchorInType::Curve)
	{
		m_rebareditdata.l0lenth = 12 * diameterselect / uor_per_mm;
	}
	else
	{
		m_rebareditdata.l0lenth = g_globalpara.m_alength[(string)Sizekey]*uor_per_mm + 50*uor_per_mm;
	}
	if (m_rebareditdata.l0lenth==0)
	{
		m_rebareditdata.l0lenth = 500;
	}
	RebarShape * rebarshapeanchor = anchorrebar->GetRebarShape(ACTIVEMODEL);
	if (rebarshapeanchor == nullptr)
	{
		return;
	}
	Sizekey = rebarshapeanchor->GetShapeData().GetSizeKey();
	diameteranchorin = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);
	m_rebareditdata.movelenth = (diameteranchorin/2+diameterselect/2)/uor_per_mm;
	
	UpdateWindow();
	if (!m_rebareditdata.isArc)
	{
		CalculateVertexAndDrawLines();
	}
	else
	{
		CalculterArcVertexAndDrawLines();
	}
}
void CRebarEditDlg::OnEnKillfocusEditRmovelenth()
{
	// TODO: 在此添加控件通知处理程序代码
	CString str;
	m_edit_movelenth.GetWindowText(str);
	m_rebareditdata.movelenth = atof(CT2A(str));
	if (!m_rebareditdata.isArc)
	{
		CalculateVertexAndDrawLines();
	}
	else
	{
		CalculterArcVertexAndDrawLines();
	}
}


void CRebarEditDlg::OnEnKillfocusEditRl0lenth()
{
	// TODO: 在此添加控件通知处理程序代码
	CString str;
	m_edit_L0Lenth.GetWindowText(str);
	m_rebareditdata.l0lenth = atof(CT2A(str));
	if (!m_rebareditdata.isArc)
	{
		CalculateVertexAndDrawLines();
	}
	else
	{
		CalculterArcVertexAndDrawLines();
	}
}


void CRebarEditDlg::OnCbnSelchangeComboRtype()
{
	// TODO: 在此添加控件通知处理程序代码
	switch (m_combo_type.GetCurSel())
	{
	case 1:
		m_rebareditdata.type = AnchorInType::Curve;
		break;
	case 0:
		m_rebareditdata.type = AnchorInType::Straight;
		break;
	default:
		break;
	}
	if (!m_rebareditdata.isArc)
	{
		CalculateVertexAndDrawLines();
	}
	else
	{
		CalculterArcVertexAndDrawLines();
	}
	
}
void CRebarEditDlg::CalcWallHoles()
{
	for (int i = 0; i < m_Holeehs.size();)
	{
		if (m_Holeehs.at(i) != nullptr)
		{
			delete m_Holeehs.at(i);
			m_Holeehs.at(i) = nullptr;
		}
	}
	m_Holeehs.clear();

	if ((int)m_Anchorinrebars.size() == 0)
	{
		return;
	}
	ElementHandle eehRebar(m_Anchorinrebars[0], ACTIVEMODEL);
	EditElementHandle Editeeh(eehRebar, ACTIVEMODEL);
	RebarElementP repTmp = RebarElement::Fetch(eehRebar);
	RebarModel *rmv = RMV;
	BeConcreteData condata;
	int rebar_cage_type;
	if (rmv != nullptr)
	{
		rmv->GetConcreteData(*repTmp, repTmp->GetModelRef(), condata, rebar_cage_type);
	}

	RebarSetP rebset = nullptr;
	rebset = repTmp->GetRebarSet(repTmp->GetModelRef());
	RebarShape * rebarshapeTmp = repTmp->GetRebarShape(m_Anchorinrebars[0]->GetDgnModelP());
	BrString Sizekey(rebarshapeTmp->GetShapeData().GetSizeKey());
	if (rebset != nullptr)
	{
		RebarSetP rootrebset = nullptr;
		rootrebset = rebset->GetParentRebarSet(repTmp->GetModelRef());
		rootrebset = rootrebset;
		RebarSets lapped_rebar_sets;
		rebset->GenerateLappedRebarSets(lapped_rebar_sets, repTmp->GetModelRef());
	}
	ElementId conid = condata.GetRexId().GetElementId();
	RebarAssemblies area;
	REA::GetRebarAssemblies(conid, area);
	RebarAssembly* rebarasb = nullptr;
	for (int i = 0; i < area.GetSize(); i++)
	{
		RebarAssembly* rebaras = area.GetAt(i);
		if (rebaras->GetCallerId() == rebset->GetCallerId())
		{
			rebarasb = rebaras;
		}
	}
	EditElementHandle ehSel; //墙
	PIT::GetAssemblySelectElement(ehSel, rebarasb);

	EditElementHandle Eleeh;
	EFT::GetSolidElementAndSolidHoles(ehSel, Eleeh, m_Holeehs);
	Transform trans;
	GetElementXAttribute(conid, sizeof(Transform), trans, UcsMatrixXAttribute, ACTIVEMODEL);
	PIT::WallRebarInfo tmpinfo;
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (SUCCESS == GetElementXAttribute(conid, sizeof(PIT::WallRebarInfo), tmpinfo, WallRebarInfoXAttribute, ACTIVEMODEL))
	{
		for (int i = 0; i < m_Holeehs.size(); i++)
		{
			ElementCopyContext copier(ACTIVEMODEL);
			copier.SetSourceModelRef(m_Holeehs.at(i)->GetModelRef());
			copier.SetTransformToDestination(true);
			copier.SetWriteElements(false);
			copier.DoCopy(*m_Holeehs.at(i));
			PlusSideCover(*m_Holeehs.at(i), tmpinfo.concrete.sideCover*uor_per_mm, trans, false);
		}
	}

}

void CRebarEditDlg::OnBnClickedButtonRselectrebars()
{
	// TODO: 在此添加控件通知处理程序代码
	// RebarSDK_ReadRebar(this);
}


void CRebarEditDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
//#define  FirstMethod

#ifdef FirstMethod     //如果是将锚入后的钢筋放入到之前选中的钢筋组中，使用此方法
	int i = 0;
	RebarEndType endType;
	endType.SetType(RebarEndType::kNone);
	RebarEndTypes   endTypes = { endType, endType };
	RebarModel *rmv = RMV;
	RebarSymbology symb;
	symb.SetRebarColor(1);
	symb.SetRebarLevel(TEXT_REBARANCHORIN_REBAR);
	for (ElementRefP eleref : m_selectrebars)
	{
		EditElementHandle eeh(eleref, eleref->GetDgnModelP());
		DgnModelRefP modelRef = eleref->GetDgnModelP();

		if (RebarElement::IsRebarElement(eeh))
		{
			RebarElementP rep = RebarElement::Fetch(eeh);
			rep->SetRebarElementSymbology(symb);
			BrString   sizeKey = m_vecDir.at(i);
			double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
			double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);
			double bendLen = RebarCode::GetBendLength(sizeKey, endType, modelRef);
			RebarCurve      rebarCurve;
			for (int j = 0; j < m_rebarPts.at(i).GetSize(); j++)
			{
				RebarVertexP vex;
				vex = &rebarCurve.PopVertices().NewElement();
				vex->SetIP(m_rebarPts.at(i).At(j).GetIP());
				vex->SetArcPt(0, m_rebarPts.at(i).At(j).GetArcPt(0));
				vex->SetArcPt(1, m_rebarPts.at(i).At(j).GetArcPt(1));
				vex->SetArcPt(2, m_rebarPts.at(i).At(j).GetArcPt(2));
				vex->SetCenter(m_rebarPts.at(i).At(j).GetCenter());
				vex->SetRadius(m_rebarPts.at(i).At(j).GetRadius());
				vex->SetTanPt(0, m_rebarPts.at(i).At(j).GetTanPt(0));
				vex->SetTanPt(1, m_rebarPts.at(i).At(j).GetTanPt(1));
				vex->SetType(m_rebarPts.at(i).At(j).GetType());
				//vex->BeCopy(*m_rebarPts.at(i).At(j));
			}
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(false);
			shape.SetLength(rebarCurve.GetLength() / Get_uor_per_mm);
			rep->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
			if (rmv != nullptr)
			{
				BeConcreteData data;
				int rebar_cage_type;
				rmv->GetConcreteData(*rep, rep->GetModelRef(),data,rebar_cage_type);
				rmv->SaveRebar(*rep, rep->GetModelRef(), true);
			}
			

		}
		i++;
	}
	ClearLines();
	m_selectrebars.clear();
	m_Anchorinrebars.clear();
#else  //如果是将锚入后的钢筋放入到和之前选中的钢筋组不一样的组中，使用此方法
	RebarSet * rebset = nullptr;
	EditElementHandle start(m_selectrebars[0], m_selectrebars[0]->GetDgnModelP());
	/*获取钢筋的所有属性值str*/
	string Level = "1";
	string Grade = "A";
	string type = "";
	string markname = "";
	string ReplacedMark = "";
	GetRebarCodeItemTypeValue(start, markname,ReplacedMark);
	GetRebarLevelItemTypeValue(start, Level, type, Grade);//获取选中钢筋的属性，写入U形筋中		
	int Spacing = GetRebarHideData(start, ACTIVEMODEL);
	/*获取钢筋的所有属性值end*/
	if (RebarElement::IsRebarElement(start))
	{
		RebarElementP rep = RebarElement::Fetch(start);
		rebset = rep->GetRebarSet(ACTIVEMODEL);
		if (rebset != nullptr&&m_rebarPts.size() > 0 && m_rebarPts.size() == m_vecDir.size())
		{

			RebarModel *rmv = RMV;
			BeConcreteData condata;
			int rebar_cage_type;
			if (rmv != nullptr)
			{
				rmv->GetConcreteData(*rep, rep->GetModelRef(), condata, rebar_cage_type);
			}

			ElementId conid = 0;
			conid = condata.GetRexId().GetElementId();
			if (conid==0)
			{
				conid = rebset->GetConcreteOwner(ACTIVEMODEL, rebar_cage_type);
				if (conid==0)
				{
					return;
				}
			}
			//int rebar_cage_type;
			//conid = rebset->GetConcreteOwner(ACTIVEMODEL, rebar_cage_type);
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
			if (rebarasb==nullptr&&reas.GetSize()>0)
			{
				rebarasb = reas.GetAt(0);
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
				EditElementHandle ehSel;
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

				slabRebar->SetSelectedRebar(start);
				slabRebar->SetSlabData(ehSel);
				slabRebar->SetvecDirSize(m_vecDir);
				slabRebar->SetrebarPts(m_rebarPts);
				slabRebar->SetEcDatas(type, Level, Grade, markname, ReplacedMark);
				slabRebar->Setspacing(Spacing);
				slabRebar->MakeRebars(modelRef);
				slabRebar->Save(modelRef); // must save after creating rebars			
				ElementId contid = slabRebar->FetchConcrete();
				EditElementHandle eeh2(contid, ACTIVEMODEL);
				ElementRefP oldRef = eeh2.GetElementRef();
				mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
				eeh2.ReplaceInModel(oldRef);
			}

			ClearLines();
			m_Anchorinrebars.clear();
			{//删除钢筋处理
				mdlSelect_freeAll();
				for (ElementRefP tref : m_selectrebars)
				{
					EditElementHandle eh(tref, tref->GetDgnModelP());
					RebarElementP rep = RebarElement::Fetch(eh);
					if (rep == nullptr)
					{
						continue;
					}
					RebarModel *rmv = RMV;
					if (rmv != nullptr)
					{
						rmv->Delete(*rep, ACTIVEMODEL);
					}
					//SelectionSetManager::GetManager().AddElement(tmpeeh.GetElementRef(), ACTIVEMODEL);
				}
			}
			m_selectrebars.clear();
			
		}
	}
#endif
	CDialogEx::OnOK();
	//mdlInput_sendSynchronizedKeyin(L"proconcrete delete rebar", 0, INPUTQ_EOQ, NULL);
	//mdlInput_sendSynchronizedKeyin(L"choose element", 0, INPUTQ_EOQ, NULL);
}


void CRebarEditDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	ClearLines();
	CDialogEx::OnCancel();
}


void CRebarEditDlg::OnBnClickedCheckRuseend()
{
	m_rebareditdata.isend = m_check_useend.GetCheck();
	if (!m_rebareditdata.isArc)
	{
		CalculateVertexAndDrawLines();
	}
	else
	{
		CalculterArcVertexAndDrawLines();
	}
}


void CRebarEditDlg::OnBnClickedRbuttonRotate()
{
	// TODO: 在此添加控件通知处理程序代码
	CString str;
	m_edit_rotateangle.GetWindowText(str);
	double tmpAngle = 90 + atof(CT2A(str));
	if (tmpAngle>360)
	{
		tmpAngle = tmpAngle - 360;
	}
	m_rebareditdata.angle = tmpAngle;
	CString sTemp = CString();
	sTemp.Format(_T("%.3f"), tmpAngle);
	m_edit_rotateangle.SetWindowText(sTemp);
	if (!m_rebareditdata.isArc)
	{
		CalculateVertexAndDrawLines();
	}
	else
	{
		CalculterArcVertexAndDrawLines();
	}
}


void CRebarEditDlg::OnEnKillfocusEditRrotate()
{
	// TODO: 在此添加控件通知处理程序代码
	CString str;
	m_edit_rotateangle.GetWindowText(str);
	m_rebareditdata.angle = atof(CT2A(str));
	if (!m_rebareditdata.isArc)
	{
		CalculateVertexAndDrawLines();
	}
	else
	{
		CalculterArcVertexAndDrawLines();
	}
}
