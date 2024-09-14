// CCombineRebardlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CCombineRebardlg.h"
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


// CCombineRebardlg 对话框

IMPLEMENT_DYNAMIC(CCombineRebardlg, CDialogEx)

CCombineRebardlg::CCombineRebardlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_COMBINEREBAR_DIALOG, pParent)
{

}

CCombineRebardlg::~CCombineRebardlg()
{
}

void CCombineRebardlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_GENVREBAR, m_check_genvrebar);
	DDX_Control(pDX, IDC_BUTTON_SELECT_VERREBAR, m_button_selectVrebar);
	DDX_Control(pDX, IDC_CHECK_USEINTERSECT, m_check_useinters);
}


BEGIN_MESSAGE_MAP(CCombineRebardlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CCombineRebardlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CCombineRebardlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_CHECK_GENVREBAR, &CCombineRebardlg::OnBnClickedCheckGenvrebar)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_VERREBAR, &CCombineRebardlg::OnBnClickedButtonSelectVerrebar)
	ON_BN_CLICKED(IDC_CHECK_USEINTERSECT, &CCombineRebardlg::OnBnClickedCheckUseintersect)
END_MESSAGE_MAP()


// CCombineRebardlg 消息处理程序
// CRebarEditDlg 消息处理程序
// CInsertRebarDlg 消息处理程序
BOOL CCombineRebardlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	m_check_genvrebar.SetCheck(TRUE);
	m_check_useinters.SetCheck(TRUE);
	isuseinters = true;
	// m_EditElementLength.SetWindowText();
	// 长度


	return TRUE;  // return TRUE unless you set the focus to a control
			  // 异常: OCX 属性页应返回 FALSE
}
void CCombineRebardlg::SorSelcetRebar()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (m_selectrebars.size()>0)
	{
		EditElementHandle feeh(m_selectrebars[0], m_selectrebars[0]->GetDgnModelP());
		DPoint3d ptstr, ptend;
		double dam;
		if (GetStartEndPointFromRebar(&feeh, ptstr, ptend, dam))
		{
			if (abs(ptstr.z - ptend.z)<5)//判断是竖向钢筋
			{
				for (ElementRefP ref : m_selectrebars)
				{
					EditElementHandle eeh(ref, ref->GetDgnModelP());
					DPoint3d center = getCenterOfElmdescr(eeh.GetElementDescrP());
					int posz = (int)(center.z / (uor_per_mm * 10));
					m_mapselectrebars[posz].push_back(ref);
				}
			}
			else
			{
				DPoint3d fcenter = getCenterOfElmdescr(feeh.GetElementDescrP());
				for (ElementRefP ref : m_selectrebars)
				{
					EditElementHandle eeh(ref, ref->GetDgnModelP());
					DPoint3d center = getCenterOfElmdescr(eeh.GetElementDescrP());
					int posd = (int)(center.DistanceXY(fcenter) / (uor_per_mm * 10));
					m_mapselectrebars[posd].push_back(ref);
				}
			}
		}

		
	}
}
void CCombineRebardlg::ClearLines()
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
	m_Editrebars.clear();
}

void GetPtsWithBendRadius(RebarCurve curve,vector<DPoint3d>& pts,vector<double>& bendradius)
{
	RebarVertices  vers = curve.PopVertices();

	for (int i = 0; i < vers.GetSize(); i++)
	{
		RebarVertex   ver = vers.At(i);	
		if (ver.GetRadius()>0)
		{
			pts.push_back(ver.GetIP());
			bendradius.push_back(ver.GetRadius());
		}
	}
}

void CCombineRebardlg::CalculateVertexAndDrawLines()
{

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	ClearLines();
	for (map<int,vector<ElementRefP>>::iterator itr = m_mapselectrebars.begin();itr!=m_mapselectrebars.end();itr++)
	{
		if (itr->second.size()==2)
		{
			EditElementHandle start(itr->second[0],itr->second[0]->GetDgnModelP());
			EditElementHandle end1(itr->second[1], itr->second[1]->GetDgnModelP());
			if (RebarElement::IsRebarElement(start) && RebarElement::IsRebarElement(end1))
			{
				vector<DPoint3d> pts; vector<double> bendradius;

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
				delete[] ipsvector;
				ipsvector = nullptr;
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
				if (ips1.size()<2||ips.size()<2)
				{
					continue;
				}
				if (isuseinters)//如果需要使用延长线交点
				{

					if (ips[0].Distance(ips1[0]) < ips[0].Distance(ips1[ips1.size() - 1]))//离起点较劲
					{
						if (ips1[0].Distance(ips[0])<ips1[0].Distance(ips[ips.size()-1]))
						{
							mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(), 
								linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[0], &ips[0]);
						}
						else
						{
							mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(),
								linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[0], &ips[ips.size() - 1]);
						}
					}
					else//离终点较近
					{
						if (ips1[ips1.size() - 1].Distance(ips[0]) < ips1[ips1.size() - 1].Distance(ips[ips.size() - 1]))
						{
							mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(),
								linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[ips1.size() - 1], &ips[0]);
						}
						else
						{
							mdlIntersect_allBetweenExtendedElms(&intresectpt1, &intresectpt2, 1, linestring1.GetElementDescrP(),
								linestring2.GetElementDescrP(), nullptr, 0.1, &ips1[ips1.size() - 1], &ips[ips.size() - 1]);
						}
					}
				}
				else//直接使用最近点串联
				{
					if (ips[0].Distance(ips1[0]) < ips[0].Distance(ips1[ips1.size() - 1]))//离起点较劲
					{
						intresectpt1 = ips1[0];
					}
					else//离终点较近
					{
						intresectpt1 = ips1[ips1.size() - 1];
					}

				}


				

				delete[] ipsvector1;
				ipsvector1 = nullptr;
				//统计所有弧形的点和直径
				GetPtsWithBendRadius(curve1, pts, bendradius);

				/*EditElementHandle tmpeeh1, tmpeeh2;
				LineHandler::CreateLineElement(tmpeeh1, nullptr, DSegment3d::From(intresectpt1, ips[0]), true, *ACTIVEMODEL);
				tmpeeh1.AddToModel();
				LineHandler::CreateLineElement(tmpeeh2, nullptr, DSegment3d::From(intresectpt2, ips[0]), true, *ACTIVEMODEL);
				tmpeeh2.AddToModel();
*/
				bvector<DPoint3d> allpts;
				int thenum = 0;
				if (isuseinters)
				{
					thenum = GetAllCombinePtsUseIntersect(allpts, ips, ips1, intresectpt1, linestring1, linestring2);
				}
				else
				{
					thenum = GetAllCombinePtsUseEndPoint(allpts, ips, ips1, intresectpt1, linestring1, linestring2);
				}
				RebarVertices  vers;
				GetRebarVerticesFromPointsAndBendRadius(vers, allpts, bendRadius,pts,bendradius);
				

				m_rebarPts.push_back(vers);
				m_vecDir.push_back(Sizekey);
				m_Editrebars.push_back(itr->second[0]);
				RebarVertex   Rebarver = vers.At(thenum);
				if (itr == m_mapselectrebars.begin() && m_check_genvrebar.GetCheck()&&m_Verticalrebars.size()==2)//画点筋
				{
					EditElementHandle end2(m_Verticalrebars[0],m_Verticalrebars[0]->GetDgnModelP());
					EditElementHandle end3(m_Verticalrebars[1], m_Verticalrebars[1]->GetDgnModelP());
					CalculateAddVerticalRebars(&end2, &end3, Rebarver, diameter, allpts, m_rebarPts, m_vecDir, 150);
				}
				
			}
			


		}
		
	}
	for (int i = 0;i<m_rebarPts.size();i++)
	{
		RebarVertices vers = m_rebarPts.at(i);
		for (int i = 0; i < vers.GetSize() - 1; i++)
		{
			RebarVertex   ver1 = vers.At(i);
			RebarVertex   ver2 = vers.At(i + 1);
			CPoint3D const&     pt1 = ver1.GetIP();
			CPoint3D const&     pt2 = ver2.GetIP();


			/*CPoint3D const&     pt1 = (ver1.GetType() == RebarVertex::kStart || ver1.GetType() == RebarVertex::kEnd) ? ver1.GetIP() : ver1.GetArcPt(2);
			CPoint3D const&     pt2 = (ver2.GetType() == RebarVertex::kStart || ver2.GetType() == RebarVertex::kEnd) ? ver2.GetIP() : ver2.GetArcPt(0);*/

			DPoint3d tpt1 = DPoint3d::From(pt1.x, pt1.y, pt1.z);
			DPoint3d tpt2 = DPoint3d::From(pt2.x, pt2.y, pt2.z);
			EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(tpt1, tpt2), true, *ACTIVEMODEL);
			eeh.AddToModel();
			m_allLines.push_back(eeh.GetElementRef());
		}
	}
	
	
	
}
void CCombineRebardlg::UpdateDataAndWindow()
{
	CalculateVertexAndDrawLines();
}

void CCombineRebardlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CalculateVertexAndDrawLines();
//#define FirstMethod
#ifdef FirstMethod     //如果是将合并后的钢筋放入到之前选中的钢筋组中，使用此方法
	int i = 0;
	RebarEndType endType;
	endType.SetType(RebarEndType::kNone);
	RebarEndTypes   endTypes = { endType, endType };
	RebarModel *rmv = RMV;
	RebarSymbology symb;
	symb.SetRebarColor(1);
	symb.SetRebarLevel(TEXT_REBARCOMBINE_REBAR);
	for (ElementRefP eleref : m_Editrebars)
	{
		vector<ElementRefP>::iterator itrfind = std::find(m_selectrebars.begin(), m_selectrebars.end(), eleref);
		if (itrfind != m_selectrebars.end())
		{
			m_selectrebars.erase(itrfind);
		}
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
			RebarShapeDataP shape = const_cast<RebarShapeDataP>(rep->GetShapeData(modelRef));
			if (shape == nullptr)
			{
				continue;
			}
			rep->Update(rebarCurve, diameter, endTypes, *shape, modelRef, false);
			if (rmv != nullptr)
			{
				rmv->SaveRebar(*rep, rep->GetModelRef(), true);
			}


		}
		i++;
	}
	ClearLines();
	for (ElementRefP tref : m_selectrebars)
	{
		EditElementHandle tmpeeh(tref, tref->GetDgnModelP());
		tmpeeh.DeleteFromModel();
	}
	m_selectrebars.clear();
	m_Verticalrebars.clear();
	m_mapselectrebars.clear();
#else  //如果是将合并后的钢筋放入到和之前选中的钢筋组不一样的组中，使用此方法
	RebarSet * rebset = nullptr;
	EditElementHandle start(m_selectrebars[0], m_selectrebars[0]->GetDgnModelP());
	/*获取钢筋的所有属性值str*/
	string Level = "1";
	string Grade = "A";
	string type = "";
	string markname = "";
	string ReplacedMark = "";
	GetRebarCodeItemTypeValue(start, markname, ReplacedMark);
	GetRebarLevelItemTypeValue(start, Level, type, Grade);//获取选中钢筋的属性，写入U形筋中		
	int Spacing = GetRebarHideData(start, ACTIVEMODEL);
	/*获取钢筋的所有属性值end*/
	if (RebarElement::IsRebarElement(start))
	{
		RebarElementP rep = RebarElement::Fetch(start);
		rebset = rep->GetRebarSet(ACTIVEMODEL);
		if (rebset != nullptr&&m_rebarPts.size() > 0 && m_rebarPts.size() == m_vecDir.size())
		{
			/*ElementId conid;
			int rebar_cage_type;
			conid = rebset->GetConcreteOwner(ACTIVEMODEL, rebar_cage_type);*/
			RebarModel *rmv = RMV;
			BeConcreteData condata;
			int rebar_cage_type;
			if (rmv != nullptr)
			{
				rmv->GetConcreteData(*rep, rep->GetModelRef(), condata, rebar_cage_type);
			}

			ElementId conid = 0;
			conid = condata.GetRexId().GetElementId();
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

				slabRebar->SetSelectedRebar(start);
				slabRebar->SetSlabData(ehSel);
				slabRebar->SetvecDirSize(m_vecDir);
				slabRebar->SetrebarPts(m_rebarPts);
				slabRebar->SetEcDatas(type, Level, Grade, markname, ReplacedMark);
				slabRebar->Setspacing(Spacing);
				slabRebar->SetConcreteOwner(conid);
				slabRebar->MakeRebars(modelRef);
				slabRebar->Save(modelRef); // must save after creating rebars
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
			ClearLines();
		}
	}
#endif
	CDialogEx::OnOK();
	mdlInput_sendSynchronizedKeyin(L"proconcrete delete rebar", 0, INPUTQ_EOQ, NULL);
	mdlInput_sendSynchronizedKeyin(L"choose element", 0, INPUTQ_EOQ, NULL);
}


void CCombineRebardlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	ClearLines();
	m_selectrebars.clear();
	m_Verticalrebars.clear();
	m_mapselectrebars.clear();
	CDialogEx::OnCancel();
}


void CCombineRebardlg::OnBnClickedCheckGenvrebar()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_check_genvrebar.GetCheck())
	{
		m_button_selectVrebar.EnableWindow(TRUE);
	}
	else
	{
		m_button_selectVrebar.EnableWindow(FALSE);
		m_Verticalrebars.clear();
		UpdateDataAndWindow();
	}
}


void CCombineRebardlg::OnBnClickedButtonSelectVerrebar()
{
	// TODO: 在此添加控件通知处理程序代码
	SelectRebarTool::InstallNewInstance2(CMDNAME_RebarSDKReadRebar, this);
}


void CCombineRebardlg::OnBnClickedCheckUseintersect()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_check_useinters.GetCheck())
	{
		isuseinters = true;
	}
	else
	{
		isuseinters = false;
	}
	UpdateDataAndWindow();
}
