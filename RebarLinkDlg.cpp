// RebarLinkDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "RebarLinkDlg.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"
#include "resource.h"
#include "ExtractFacesTool.h"
#include "CCombineRebardlg.h"
#include "SelectRebarTool.h"
#include "SingleRebarAssembly.h"
#include "XmlHelper.h"


// RebarLinkDlg 对话框
extern GlobalParameters g_globalpara;	//全局参数

IMPLEMENT_DYNAMIC(RebarLinkDlg, CDialogEx)

RebarLinkDlg::RebarLinkDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_REBARLINK, pParent)
{

}

RebarLinkDlg::~RebarLinkDlg()
{
}

void RebarLinkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_ComboLinkMethod);
	DDX_Control(pDX, IDC_CHECK7, m_Checkmiss);
	DDX_Control(pDX, IDC_EDIT1, m_EidtMissLen);
	DDX_Control(pDX, IDC_BUTTON1, m_ChangeButton);
}


BEGIN_MESSAGE_MAP(RebarLinkDlg, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &RebarLinkDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &RebarLinkDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBO1, &RebarLinkDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON1, &RebarLinkDlg::OnBnClickedButton1)
	ON_EN_CHANGE(IDC_EDIT1, &RebarLinkDlg::OnEnChangeEdit1)
	ON_BN_CLICKED(IDC_CHECK7, &RebarLinkDlg::OnBnClickedCheck7)
	ON_BN_CLICKED(IDC_BUTTON2, &RebarLinkDlg::OnBnClickedButton2)
END_MESSAGE_MAP()



BOOL RebarLinkDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	for each (auto var in g_RebarLinkMethod)
		m_ComboLinkMethod.AddString(var);

	m_RebarLinkInfo.missflg = false;
	m_RebarLinkInfo.ChangeFlg = false;
	m_RebarLinkInfo.tranLenthFlg = false;
	InitUIData();
	return true;
}

void RebarLinkDlg::InitUIData()
{
// 	CString strLen;
// 	strLen.Format(L"%.2f", m_RebarLinkInfo.MissLen);
// 	m_EidtMissLen.SetWindowText(strLen);
}



void RebarLinkDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}


void RebarLinkDlg::OnBnClickedCancel()
{
	ClearLines();
	CDialogEx::OnCancel();
	DestroyWindow();
}

void RebarLinkDlg::MakeRebar(EditElementHandleR start, vector<RebarVertices>& m_rebarPts, vector<BrString>& m_vecDir,ElementId conid)
{

	RebarSet * rebset = nullptr;
	if (RebarElement::IsRebarElement(start))
	{
		RebarElementP rep = RebarElement::Fetch(start);
		rebset = rep->GetRebarSet(ACTIVEMODEL);
		if (rebset != nullptr&&m_rebarPts.size() > 0 && m_rebarPts.size() == m_vecDir.size())
		{
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
				slabRebar->SetConcreteOwner(conid);
				slabRebar->SetSelectedRebar(start);
				slabRebar->SetSlabData(ehSel);
				slabRebar->SetvecDirSize(m_vecDir);
				slabRebar->SetrebarPts(m_rebarPts);
				slabRebar->Setspacing(rebset->GetSetData().GetNominalSpacing());
				slabRebar->MakeRebars(modelRef);
				slabRebar->Save(modelRef); // must save after creating rebars
			}
		}
	}
}


void RebarLinkDlg::OnBnClickedOk()
{
	auto it = m_mapselectrebars.begin();
	if (it->second.size() != m_AllvecDir.size())
		return;
	if (it->second.size() != m_AllPts.size())
		return;

	auto itr = m_AllPts.begin();
	for (int x = 0 ;x< it->second.size();x++)
	{
		EditElementHandle start(it->second[x], it->second[x]->GetDgnModelP());
		MakeRebar(start, itr->second, m_AllvecDir[x], itr->first);
		itr++;
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
	m_mapselectrebars.clear();

	ClearLines();
	CDialogEx::OnOK();
	DestroyWindow();

	mdlInput_sendSynchronizedKeyin(L"proconcrete delete rebar", 0, INPUTQ_EOQ, NULL);
	mdlInput_sendSynchronizedKeyin(L"choose element", 0, INPUTQ_EOQ, NULL);
}





void RebarLinkDlg::SorSelcetRebar()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (m_selectrebars.size() > 0)
	{
		for (ElementRefP ref : m_selectrebars)
		{
			EditElementHandle eeh(ref, ref->GetDgnModelP());
			DPoint3d center = getCenterOfElmdescr(eeh.GetElementDescrP());
			int posz = (int)(center.z / (uor_per_mm * 10));
			m_mapselectrebars[posz].push_back(ref);
		}
	}
}


void RebarLinkDlg::ClearLines()
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
	m_AllPts.clear();
	m_AllvecDir.clear();
	m_Editrebars.clear();

}

//void RebarLinkDlg::CalculateVertexAndDrawLines()
//{
//	ClearLines();
//	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
//
//	for (map<int, vector<ElementRefP>>::iterator itr = m_mapselectrebars.begin(); itr != m_mapselectrebars.end(); itr++)
//	{
//		if (itr->second.size() == 2)
//		{
//			EditElementHandle start(itr->second[0], itr->second[0]->GetDgnModelP());
//			EditElementHandle end1(itr->second[1], itr->second[1]->GetDgnModelP());
//			if (RebarElement::IsRebarElement(start) && RebarElement::IsRebarElement(end1))
//			{
//				vector<DPoint3d> pts; vector<DPoint3d> pts1; vector<double> bendradius; vector<double> bendradius1;
//
//				RebarCurve curve;
//				RebarElementP rep = RebarElement::Fetch(start);
//				double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
//				RebarShape * rebarshape = rep->GetRebarShape(start.GetModelRef());
//				if (rebarshape == nullptr)
//				{
//					continue;
//				}
//				rebarshape->GetRebarCurve(curve);
//				BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
//				double diameter = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);
//				double bendRadius = RebarCode::GetPinRadius(Sizekey, ACTIVEMODEL, false);
//				CMatrix3D tmp3d(rep->GetLocation());
//				curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
//				curve.DoMatrix(rep->GetLocation());
//				bvector<DPoint3d> ips;
//				curve.GetIps(ips);
//				DPoint3d* ipsvector = new DPoint3d[ips.size()];
//				memset(ipsvector, 0, ips.size() * sizeof(DPoint3d));
//				memcpy(ipsvector, ips.begin(), ips.size() * sizeof(DPoint3d));
//				EditElementHandle linestring1, linestring2;
//				LineStringHandler::CreateLineStringElement(linestring1, nullptr, ipsvector, ips.size(), true, *ACTIVEMODEL);
////				linestring1.AddToModel();
//				//统计所有弧形的点和直径
//				GetPtsWithBendRadius(curve, pts, bendradius);
//
//
//				RebarCurve curve1;
//				RebarElementP rep1 = RebarElement::Fetch(end1);
//				RebarShape * rebarshape1 = rep1->GetRebarShape(end1.GetModelRef());
//				if (rebarshape1 == nullptr)
//				{
//					continue;
//				}
//				rebarshape1->GetRebarCurve(curve1);
//				BrString Sizekey1(rebarshape1->GetShapeData().GetSizeKey());
//				double diameter1 = RebarCode::GetBarDiameter(Sizekey1, ACTIVEMODEL);
//				double bendRadius1 = RebarCode::GetPinRadius(Sizekey1, ACTIVEMODEL, false);
//
//				CMatrix3D tmp3d1(rep1->GetLocation());
//				curve1.MatchRebarCurve(tmp3d1, curve1, uor_per_mm);
//				curve1.DoMatrix(rep1->GetLocation());
//				bvector<DPoint3d> ips1;
//				curve1.GetIps(ips1);
//				DPoint3d* ipsvector1 = new DPoint3d[ips1.size()];
//				memset(ipsvector1, 0, ips1.size() * sizeof(DPoint3d));
//				memcpy(ipsvector1, ips1.begin(), ips1.size() * sizeof(DPoint3d));
//				LineStringHandler::CreateLineStringElement(linestring2, nullptr, ipsvector1, ips1.size(), true, *ACTIVEMODEL);
////				linestring2.AddToModel();
//				DPoint3d intresectpt1, intresectpt2;
//				if (ips1.size() < 2 || ips.size() < 2)
//				{
//					continue;
//				}
//
//				CVector3D vecRe;
//				if (ips[0].Distance(ips1[0]) < ips[0].Distance(ips1[ips1.size() - 1]))//离起点较近
//				{
//					if (ips1[0].Distance(ips[0]) < ips1[0].Distance(ips[ips.size() - 1]))
//					{
//						mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(),
//							linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[0], &ips[0]);
//
//						vecRe = LinkDirAndLen(ips1[0], ips[0]);
//						
//					}
//					else
//					{
//						mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(),
//							linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[0], &ips[ips.size() - 1]);
//
//						vecRe = LinkDirAndLen(ips1[0], ips[ips.size() - 1]);
//					}
//				}
//				else//离终点较近
//				{
//					if (ips1[ips1.size() - 1].Distance(ips[0]) < ips1[ips1.size() - 1].Distance(ips[ips.size() - 1]))
//					{
//						mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(),
//							linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[ips1.size() - 1], &ips[0]);
//
//						vecRe = LinkDirAndLen(ips1[ips1.size() - 1], ips[0]);
//					}
//					else
//					{
//						mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(),
//							linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[ips1.size() - 1], &ips[ips.size() - 1]);
//
//						vecRe = LinkDirAndLen(ips1[ips1.size() - 1], ips[ips.size() - 1]);
//					}
//				}
//
//				//统计所有弧形的点和直径
//				GetPtsWithBendRadius(curve1, pts1, bendradius1);
//
//				RebarVertices  vers,vers1;
//				if (m_RebarLinkInfo.ChangeFlg && m_RebarLinkInfo.missflg)//点击了切换按钮，第二根钢筋长。并错开
//				{
//					m_Editrebars.push_back(itr->second[1]);
//			
//					//CVector3D vecmiss = vecRe.Perpendicular(CVector3D::kZaxis);
//					CVector3D vecmiss ={0,0,1};
//					for (int x = 0; x < ips1.size(); x++)
//					{
//						movePoint(vecmiss, ips1[x], diameter);
//					}
//					for (int i = 0; i < pts1.size(); i++)
//					{
//						movePoint(vecmiss, pts1[i], diameter);
//					}
//		
//					GetRebarVerticesFromPointsAndBendRadius(vers, ips1, bendRadius, pts1, bendradius1);
//					GetRebarVerticesFromPointsAndBendRadius(vers1,ips, bendRadius, pts, bendradius);
//				}
//				else if ((!m_RebarLinkInfo.ChangeFlg) && m_RebarLinkInfo.missflg)//没有点击切换按钮，第一根钢筋长，并错开
//				{
//					m_Editrebars.push_back(itr->second[0]);
//
//					//CVector3D vecmiss = CVector3D::kZaxis.Perpendicular(vecRe);
//					CVector3D vecmiss = { 0,0,1 };
//					for (int x = 0; x < ips.size(); x++)
//					{
//						movePoint(vecmiss, ips[x], diameter);
//					}
//					for (int i = 0; i < pts.size(); i++)
//					{
//						movePoint(vecmiss, pts[i], diameter);
//					}
//
//					GetRebarVerticesFromPointsAndBendRadius(vers, ips, bendRadius, pts, bendradius);
//					GetRebarVerticesFromPointsAndBendRadius(vers1, ips1, bendRadius, pts1, bendradius1);
//				}
//				else if (m_RebarLinkInfo.ChangeFlg && (!m_RebarLinkInfo.missflg))//点击了切换按钮，第二根钢筋长，不错开
//				{
//					m_Editrebars.push_back(itr->second[1]);
//					GetRebarVerticesFromPointsAndBendRadius(vers, ips1, bendRadius, pts1, bendradius1);
//					GetRebarVerticesFromPointsAndBendRadius(vers1, ips, bendRadius, pts, bendradius);
//				}
//				else//没有点击切换按钮，第一根钢筋长，不错开
//				{
//					m_Editrebars.push_back(itr->second[0]);
//					GetRebarVerticesFromPointsAndBendRadius(vers, ips, bendRadius, pts, bendradius);
//					GetRebarVerticesFromPointsAndBendRadius(vers1, ips1, bendRadius, pts1, bendradius1);
//				}
//
//
//				m_rebarPts.push_back(vers);
//				m_rebarPts.push_back(vers1);
//				m_vecDir.push_back(Sizekey);
//				m_vecDir.push_back(Sizekey1);
//
//				delete[] ipsvector;
//				ipsvector = nullptr;
//				delete[] ipsvector1;
//				ipsvector1 = nullptr;
//
//			}
//		}
//	}
//	for (int i = 0; i < m_rebarPts.size(); i++)
//	{
//		RebarVertices vers = m_rebarPts.at(i);
//		for (int i = 0; i < vers.GetSize() - 1; i++)
//		{
//			RebarVertex   ver1 = vers.At(i);
//			RebarVertex   ver2 = vers.At(i + 1);
//			CPoint3D const&     pt1 = ver1.GetIP();
//			CPoint3D const&     pt2 = ver2.GetIP();
//
//			DPoint3d tpt1 = DPoint3d::From(pt1.x, pt1.y, pt1.z);
//			DPoint3d tpt2 = DPoint3d::From(pt2.x, pt2.y, pt2.z);
//			EditElementHandle eeh;
//			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(tpt1, tpt2), true, *ACTIVEMODEL);
//			eeh.AddToModel();
//			m_allLines.push_back(eeh.GetElementRef());
//		}
//	}
//}

// CVector3D RebarLinkDlg::LinkDirAndLen(DPoint3d& pt2, DPoint3d& pt1)
// {
// 
// 	CVector3D vec = pt2 - pt1;
// 	vec.Normalize();
// 	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
// 	double dislen = pt2.Distance(pt1) + m_RebarLinkInfo.MissLen * uor_per_mm;
// 	if (!m_RebarLinkInfo.ChangeFlg)//不点切换 默认第一根长
// 	{
// 		DPoint3d ptend = pt1;
// 		movePoint(vec, ptend, dislen);
// 		pt1 = ptend;
// 	}
// 	else
// 	{
// 		DPoint3d ptend = pt2;
// 		movePoint(vec, ptend, dislen, false);
// 		pt2 = ptend;
// 	}
// 	return vec;
// }


// CVector3D RebarLinkDlg::LinkDirAndLen(DPoint3d& pt2, DPoint3d& pt1, bool& Jugeflg, BrString& Sizekey)
// {
// 	CVector3D vec = pt2 - pt1;
// 	DPoint3d pt1end = pt1;
// 	DPoint3d pt2end = pt2;
// 	double distancelen = pt2.Distance(pt1);
// 	if (m_RebarLinkInfo.missflg)
// 	{
// 		bool juge = Jugeflg;//用于判断错开情况：余数为0第一根长，第二根短；余数为1第一根短，第二根长
// 		int itemp = atoi(Sizekey);
// 		double L0len = g_globalpara.m_laplenth[itemp];
// 		double SecondLinkMethod = 400 * 10;
// 
// 		if (m_RebarLinkInfo.RebarLinkMethod == 0)//搭接连接方式：长的钢筋多长：L0 + 0.3L0
// 		{
// 			if (!juge)//偶数次进来，默认第一根长，第二根短
// 			{
// 				if (distancelen > (0.3 * L0len))
// 				{
// 					double x = (distancelen - 0.3*L0len) / 2;
// 					movePoint(vec, pt1end, (0.3 * L0len + x + L0len));
// 					movePoint(vec, pt2end, x, false);
// 				}
// 				else
// 				{
// 					double x = (0.3*L0len - distancelen) / 2;
// 					movePoint(vec, pt1end, (distancelen + x + L0len));
// 					movePoint(vec, pt2end, x);
// 				}
// 				pt1 = pt1end;
// 				pt2 = pt2end;
// 			}
// 			else//奇数次进来，默认第二根长，第一根短
// 			{
// 				if (distancelen > (0.3 * L0len))
// 				{
// 					double x = (distancelen - 0.3*L0len) / 2;
// 					movePoint(vec, pt2end, (0.3 * L0len + x + L0len), false);
// 					movePoint(vec, pt1end, x);
// 				}
// 				else
// 				{
// 					double x = (0.3*L0len - distancelen) / 2;
// 					movePoint(vec, pt2end, (distancelen + x + L0len), false);
// 					movePoint(vec, pt1end, x, false);
// 				}
// 				pt2 = pt2end;
// 				pt1 = pt1end;
// 			}
// 		}
// 		else
// 		{//机械连接方式：长的钢筋多长：400
// 
// 			if (!juge)//偶数次进来，默认第一根长，第二根短
// 			{
// 				if (distancelen > SecondLinkMethod)
// 				{
// 					double x = (distancelen - SecondLinkMethod) / 2;
// 					movePoint(vec, pt1end, (SecondLinkMethod + x));
// 					movePoint(vec, pt2end, x, false);
// 				}
// 				else
// 				{
// 					double x = (SecondLinkMethod - distancelen) / 2;
// 					movePoint(vec, pt1end, (distancelen + x));
// 					movePoint(vec, pt2end, x);
// 				}
// 				pt1 = pt1end;
// 				pt2 = pt2end;
// 			}
// 			else//奇数次进来，默认第二根长，第一根短
// 			{
// 				if (distancelen > SecondLinkMethod)
// 				{
// 					double x = (distancelen - SecondLinkMethod) / 2;
// 					movePoint(vec, pt2end, (SecondLinkMethod + x), false);
// 					movePoint(vec, pt1end, x);
// 				}
// 				else
// 				{
// 					double x = (SecondLinkMethod - distancelen) / 2;
// 					movePoint(vec, pt2end, (distancelen + x), false);
// 					movePoint(vec, pt1end, x, false);
// 				}
// 				pt2 = pt2end;
// 				pt1 = pt1end;
// 			}
// 		}
// 	}
// 	else
// 	{//不错开的情况,直接延长相隔的距离
// 		if (!m_RebarLinkInfo.ChangeFlg)//没有点击切换按钮，左边钢筋长，右边钢筋短
// 		{
// 			movePoint(vec, pt1end, distancelen);
// 			pt1 = pt1end;
// 		}
// 		else
// 		{
// 			movePoint(vec, pt2end, distancelen,false);
// 			pt2 = pt2end;
// 		}
// 	}
// 	return vec;
// }



CVector3D RebarLinkDlg::LinkDirAndLen(DPoint3d& pt2, DPoint3d& pt1, bool& Jugeflg, BrString& Sizekey)
{
	CVector3D vec = pt2 - pt1;
// 	if (m_RebarLinkInfo.ChangeFlg)//点击了切换，两点的位置互换，方向取反
// 	{
// 		DPoint3d temp = pt1;
// 		pt1 = pt2;
// 		pt2 = temp;
// 		vec.Negate();
// 	}

	DPoint3d pt1end = pt1;
	DPoint3d pt2end = pt2;
	double distancelen = pt2.Distance(pt1);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	bool juge = Jugeflg;//用于判断：余数为0第一根长，第二根短；余数为1第一根短，第二根长
//	int itemp = atoi(Sizekey);
	double L0len = g_globalpara.m_laplenth[(string)m_SSizeKey] * uor_per_mm;


	if (m_RebarLinkInfo.missflg)
	{	
		if (m_RebarLinkInfo.RebarLinkMethod == 0)//搭接连接方式：长的钢筋默认多长：dis + 2.3*L0
		{
			double x1 = distancelen + m_RebarLinkInfo.MissLen * uor_per_mm;
			double x2 = m_RebarLinkInfo.MissLen * uor_per_mm - L0len;//m_RebarLinkInfo.MissLen为2.5L0时，退1.5L0
			if (!juge)//偶数次进来，默认第一根长，第二根短
			{
				movePoint(vec, pt1end, x1);
				movePoint(vec, pt2end, x2);
				pt1 = pt1end;
				pt2 = pt2end;
			}
			else//奇数次进来，默认第一根长，第二根短
			{
				movePoint(vec, pt1end, (distancelen + L0len));
// 				movePoint(vec, pt2end, x1, false);
// 				movePoint(vec, pt1end, x2,false);
// 				pt2 = pt2end;
 				pt1 = pt1end;
			}
		}
		else
		{//机械连接方式：长的钢筋多长：400

			double MecheLen1 = distancelen + 2 * m_RebarLinkInfo.MissLen * uor_per_mm;//机械连接的距离变为2倍
			double MecheLen2 = distancelen + m_RebarLinkInfo.MissLen * uor_per_mm;
			if (!juge)//偶数次进来，默认第一根长2倍，第二根短2倍
			{
		
				movePoint(vec, pt1end, MecheLen1);
				movePoint(vec, pt2end, 2 * m_RebarLinkInfo.MissLen * uor_per_mm);
				pt1 = pt1end;
				pt2 = pt2end;
			}
			else//奇数次进来，默认第一根长1倍，第二根短1倍
			{
				movePoint(vec, pt1end, MecheLen2);
				movePoint(vec, pt2end, m_RebarLinkInfo.MissLen * uor_per_mm);
				pt1 = pt1end;
				pt2 = pt2end;
// 				movePoint(vec, pt2end, MecheLen,false);
// 				movePoint(vec, pt1end, 2 * m_RebarLinkInfo.MissLen * uor_per_mm,false);
// 				pt1 = pt1end;
// 				pt2 = pt2end;
			}
		}
	}
	else
	{//不错开的情况
		if (m_RebarLinkInfo.RebarLinkMethod == 0)
		{//搭接连接
			double x = distancelen + m_RebarLinkInfo.MissLen * uor_per_mm;
//			if (!m_RebarLinkInfo.tranLenthFlg)//不交换延长的钢筋组
			{
				movePoint(vec, pt1end, x);
				pt1 = pt1end;
			}
// 			else
// 			{
// 				movePoint(vec, pt2end, x,false);
// 				pt2 = pt2end;
// 			}
		}
		else
		{//机械连接
			double x = distancelen + m_RebarLinkInfo.MissLen * uor_per_mm;
//			if (!m_RebarLinkInfo.tranLenthFlg)//不交换延长的钢筋组，默认第一根长，第二根短
			{
				movePoint(vec, pt1end, x);
				movePoint(vec, pt2end, m_RebarLinkInfo.MissLen * uor_per_mm);
				pt1 = pt1end;
				pt2 = pt2end;
			}
// 			else
// 			{
// 				movePoint(vec, pt2end, x, false);
// 				movePoint(vec, pt1end, m_RebarLinkInfo.MissLen * uor_per_mm, false);
// 				pt1 = pt1end;
// 				pt2 = pt2end;
// 			}
		}

	}
	return vec;
}





void RebarLinkDlg::CalculateVertexAndDrawLines()
{
	ClearLines();
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (m_mapselectrebars.size() == 0)
		return;

	auto it = m_mapselectrebars.begin();
	ElementId conid, conid1 = 0;

	if (it->second.size() == 2)
	{
		EditElementHandle start(it->second[0], it->second[0]->GetDgnModelP());
		EditElementHandle end1(it->second[1], it->second[1]->GetDgnModelP());
		if (RebarElement::IsRebarElement(start) && RebarElement::IsRebarElement(end1))
		{
			RebarElementP rep = RebarElement::Fetch(start);		
			RebarSet * rebset = rep->GetRebarSet(ACTIVEMODEL);
			RebarModel *rmv = RMV;
			BeConcreteData condata;
			int rebar_cage_type;
			if (rmv != nullptr)
			{
				rmv->GetConcreteData(*rep, rep->GetModelRef(), condata, rebar_cage_type);
			}
			conid = condata.GetRexId().GetElementId();


			RebarElementP rep1 = RebarElement::Fetch(end1);
			RebarSet * rebset1 = rep1->GetRebarSet(ACTIVEMODEL);
			RebarModel *rmv1 = RMV;
			BeConcreteData condata1;
			int rebar_cage_type1;
			if (rmv1 != nullptr)
			{
				rmv1->GetConcreteData(*rep1, rep1->GetModelRef(), condata1, rebar_cage_type1);
			}
			conid1 = condata1.GetRexId().GetElementId();
		}
	}


	int num = 0;
	vector<BrString> vecDir;
	vector<BrString> vecDir1;
	vector<RebarVertices> rebarPts;
	vector<RebarVertices> rebarPts1;
	for (map<int, vector<ElementRefP>>::iterator itr = m_mapselectrebars.begin(); itr != m_mapselectrebars.end(); itr++)
	{
		if (itr->second.size() == 2)
		{
			EditElementHandle start(itr->second[0], itr->second[0]->GetDgnModelP());
			EditElementHandle end1(itr->second[1], itr->second[1]->GetDgnModelP());
			if (RebarElement::IsRebarElement(start) && RebarElement::IsRebarElement(end1))
			{
				vector<DPoint3d> pts; vector<DPoint3d> pts1; vector<double> bendradius; vector<double> bendradius1;
				RebarCurve curve;
				RebarElementP rep = RebarElement::Fetch(start);

				double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
				RebarShape * rebarshape = rep->GetRebarShape(start.GetModelRef());
				if (rebarshape == nullptr)
				{
					continue;
				}
				rebarshape->GetRebarCurve(curve);
				BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
				double diameter = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);
				double bendRadius = RebarCode::GetPinRadius(Sizekey, ACTIVEMODEL, false);
				CMatrix3D tmp3d(rep->GetLocation());
				curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
				curve.DoMatrix(rep->GetLocation());
				bvector<DPoint3d> ips;
				curve.GetIps(ips);
				DPoint3d* ipsvector = new DPoint3d[ips.size()];
				memset(ipsvector, 0, ips.size() * sizeof(DPoint3d));
				memcpy(ipsvector, ips.begin(), ips.size() * sizeof(DPoint3d));
				EditElementHandle linestring1, linestring2;
				LineStringHandler::CreateLineStringElement(linestring1, nullptr, ipsvector, ips.size(), true, *ACTIVEMODEL);
				//linestring1.AddToModel();
				//统计所有弧形的点和直径
				GetPtsWithBendRadius(curve, pts, bendradius);


				RebarCurve curve1;
				RebarElementP rep1 = RebarElement::Fetch(end1);
				RebarShape * rebarshape1 = rep1->GetRebarShape(end1.GetModelRef());
				if (rebarshape1 == nullptr)
				{
					continue;
				}
				rebarshape1->GetRebarCurve(curve1);
				BrString Sizekey1(rebarshape1->GetShapeData().GetSizeKey());
				double diameter1 = RebarCode::GetBarDiameter(Sizekey1, ACTIVEMODEL);
				double bendRadius1 = RebarCode::GetPinRadius(Sizekey1, ACTIVEMODEL, false);

				CMatrix3D tmp3d1(rep1->GetLocation());
				curve1.MatchRebarCurve(tmp3d1, curve1, uor_per_mm);
				curve1.DoMatrix(rep1->GetLocation());
				bvector<DPoint3d> ips1;
				curve1.GetIps(ips1);
				DPoint3d* ipsvector1 = new DPoint3d[ips1.size()];
				memset(ipsvector1, 0, ips1.size() * sizeof(DPoint3d));
				memcpy(ipsvector1, ips1.begin(), ips1.size() * sizeof(DPoint3d));
				LineStringHandler::CreateLineStringElement(linestring2, nullptr, ipsvector1, ips1.size(), true, *ACTIVEMODEL);

				DPoint3d intresectpt1, intresectpt2;
				if (ips1.size() < 2 || ips.size() < 2)
				{
					continue;
				}


				if (m_RebarLinkInfo.tranLenthFlg)//点击了延长的钢筋组，钢筋的每个点的位置互换，方向取反
				{
					ips.swap(ips1);
				}

				CVector3D vecRe;
				bool Jugeflg = num % 2;//用于判断当前是奇数次进来还是偶数次进来，默认偶数情况第一根长，第二根短；
				if (ips[0].Distance(ips1[0]) < ips[0].Distance(ips1[ips1.size() - 1]))
				{
					if (ips1[0].Distance(ips[0]) < ips1[0].Distance(ips[ips.size() - 1]))
					{
						mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(),
							linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[0], &ips[0]);

						if (m_RebarLinkInfo.ChangeFlg)//若点击了切换：相当于默认情况下的奇数列与偶数列相交换，即Jugeflg取反
							Jugeflg = !Jugeflg;

						vecRe = LinkDirAndLen(ips1[0], ips[0], Jugeflg, Sizekey);
					}
					else
					{
						mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(),
							linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[0], &ips[ips.size() - 1]);
						
						if (m_RebarLinkInfo.ChangeFlg)
							Jugeflg = !Jugeflg;
							
						vecRe = LinkDirAndLen(ips1[0], ips[ips.size() - 1], Jugeflg, Sizekey);

					}
				}
				else//离终点较近
				{
					if (ips1[ips1.size() - 1].Distance(ips[0]) < ips1[ips1.size() - 1].Distance(ips[ips.size() - 1]))
					{
						mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(),
							linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[ips1.size() - 1], &ips[0]);

						if (m_RebarLinkInfo.ChangeFlg)
							Jugeflg = !Jugeflg;

						vecRe = LinkDirAndLen(ips1[ips1.size() - 1], ips[0], Jugeflg, Sizekey);
					}
					else
					{
						mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(),
							linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[ips1.size() - 1], &ips[ips.size() - 1]);

						if (m_RebarLinkInfo.ChangeFlg)
							Jugeflg = !Jugeflg;

						vecRe = LinkDirAndLen(ips1[ips1.size() - 1], ips[ips.size() - 1], Jugeflg, Sizekey);
					}
				}


				//统计所有弧形的点和直径
				GetPtsWithBendRadius(curve1, pts1, bendradius1);

				RebarVertices  vers, vers1;
				if (m_RebarLinkInfo.tranLenthFlg && m_RebarLinkInfo.missflg &&(m_RebarLinkInfo.RebarLinkMethod ==0))//点击了切换按钮，第二根钢筋长。并错开
				{
					m_Editrebars.push_back(itr->second[1]);

					CVector3D vecmiss = { 0,0,1 };
					for (int x = 0; x < ips1.size(); x++)
					{
						movePoint(vecmiss, ips1[x], diameter);
					}
					for (int i = 0; i < pts1.size(); i++)
					{
						movePoint(vecmiss, pts1[i], diameter);
					}
				}
				else if ((!m_RebarLinkInfo.tranLenthFlg) && m_RebarLinkInfo.missflg && (m_RebarLinkInfo.RebarLinkMethod == 0))//没有点击切换按钮，第一根钢筋长，并错开
				{
					m_Editrebars.push_back(itr->second[0]);

					CVector3D vecmiss = { 0,0,1 };
					for (int x = 0; x < ips.size(); x++)
					{
						movePoint(vecmiss, ips[x], diameter);
					}
					for (int i = 0; i < pts.size(); i++)
					{
						movePoint(vecmiss, pts[i], diameter);
					}

				}
				else if (m_RebarLinkInfo.tranLenthFlg && (!m_RebarLinkInfo.missflg))//点击了切换按钮，第二根钢筋长，不错开
				{
					m_Editrebars.push_back(itr->second[1]);

				}
				else//没有点击切换按钮，第一根钢筋长，不错开
				{
					m_Editrebars.push_back(itr->second[0]);
				}

				GetRebarVerticesFromPointsAndBendRadius(vers, ips, bendRadius, pts, bendradius);
				GetRebarVerticesFromPointsAndBendRadius(vers1, ips1, bendRadius, pts1, bendradius1);
				rebarPts.push_back(vers);
				rebarPts1.push_back(vers1);
				vecDir.push_back(Sizekey);
				vecDir1.push_back(Sizekey1);

				delete[] ipsvector;
				ipsvector = nullptr;
				delete[] ipsvector1;
				ipsvector1 = nullptr;

			}
		}
		num++;
	}
	m_AllvecDir.push_back(vecDir);
	m_AllvecDir.push_back(vecDir1);
	m_AllPts.insert(make_pair(conid, rebarPts));
	m_AllPts.insert(make_pair(conid1, rebarPts1));

	for (int i = 0; i < rebarPts.size(); i++)
	{
		RebarVertices vers = rebarPts.at(i);
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
			m_allLines.push_back(eeh.GetElementRef());
		}
	}
	for (int i = 0; i < rebarPts1.size(); i++)
	{
		RebarVertices vers = rebarPts1.at(i);
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
			m_allLines.push_back(eeh.GetElementRef());
		}
	}
}




void RebarLinkDlg::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

void RebarLinkDlg::OnCbnSelchangeCombo1()//连接方式
{
	m_RebarLinkInfo.RebarLinkMethod = m_ComboLinkMethod.GetCurSel();

	CString strLen = L"";
	if ((m_RebarLinkInfo.RebarLinkMethod == 0) && (m_RebarLinkInfo.missflg))
	{
		strLen = L"0.3L0";
	}
	else if ((m_RebarLinkInfo.RebarLinkMethod == 0) && (!m_RebarLinkInfo.missflg))
	{
		strLen = L"L0";
	}
	else 
	{
		m_RebarLinkInfo.MissLen = 400;
		strLen.Format(L"%.2f", m_RebarLinkInfo.MissLen);
	}
	m_EidtMissLen.SetWindowText(strLen);
}


bool flag = false;
void RebarLinkDlg::OnBnClickedButton1()//选择延长的钢筋组按钮
{
	if (!flag)
	{
		m_RebarLinkInfo.tranLenthFlg = true;
		flag = true;
	}
	else
	{
		m_RebarLinkInfo.tranLenthFlg = false;
		flag = false;
	}
	CalculateVertexAndDrawLines();
}


void RebarLinkDlg::OnEnChangeEdit1()//错开长度
{
	CString	strTemp = CString();
	m_EidtMissLen.GetWindowText(strTemp);

	if (strTemp.Find(L"L0") != -1 )//有L0为搭接连接的方式， 直接输入L0的倍数
	{
		if (m_RebarLinkInfo.missflg)
		{//错开情况下，0.3L0 -> 2.3L0
			CString	 cstr = strTemp.Left(strTemp.Find(_T("L0")));
			if (cstr == L"")
			{//值为L0的情况，需要加上1在前面
				cstr = L"1";
			}
			m_RebarLinkInfo.MissLen = (atof(CT2A(cstr)) * g_globalpara.m_laplenth[(string)m_SSizeKey]);
			m_RebarLinkInfo.MissLen += 2 * g_globalpara.m_laplenth[(string)m_SSizeKey];
		}
		else
		{//不错开的情况下，L0 -> L0
			CString	 cstr = strTemp.Left(strTemp.Find(_T("L0")));
			if (cstr == L"")
			{//值为L0的情况，需要加上1在前面
				cstr = L"1";
			}
			m_RebarLinkInfo.MissLen = (atof(CT2A(cstr)) * g_globalpara.m_laplenth[(string)m_SSizeKey]);
		}

	}
	else
	{
		m_RebarLinkInfo.MissLen = atof(CT2A(strTemp));
	}
	CalculateVertexAndDrawLines();
}



void RebarLinkDlg::OnBnClickedCheck7()//错开按钮
{

	if (m_Checkmiss.GetCheck())
	{
		m_RebarLinkInfo.missflg = true;//错开
		CString strLen = L"";

		if (m_RebarLinkInfo.RebarLinkMethod == 0)
		{
			strLen = L"0.3L0";
			m_EidtMissLen.SetWindowText(strLen);
		}
		else
		{
			m_RebarLinkInfo.MissLen = 400;
			strLen.Format(L"%.2f", m_RebarLinkInfo.MissLen);
			m_EidtMissLen.SetWindowText(strLen);
		}
	}
	else
	{
		m_RebarLinkInfo.missflg = false;
		CString strLen = L"";
		if (m_RebarLinkInfo.RebarLinkMethod == 0)
		{
			strLen = L"L0";
			m_EidtMissLen.SetWindowText(strLen);
		}
		else
		{
			m_RebarLinkInfo.MissLen = 400;
			strLen.Format(L"%.2f", m_RebarLinkInfo.MissLen);
			m_EidtMissLen.SetWindowText(strLen);
		}
	}
}




bool change = false;
void RebarLinkDlg::OnBnClickedButton2()//切换
{
	if (!change)
	{
		m_RebarLinkInfo.ChangeFlg = true;
		change = true;
	}
	else
	{
		m_RebarLinkInfo.ChangeFlg = false;
		change = false;
	}
	CalculateVertexAndDrawLines();
}
