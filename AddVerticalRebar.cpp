#include "_USTATION.h"
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
#include "AddVerticalRebar.h"

// AddVerticalRebar 

AddVerticalRebar::AddVerticalRebar()
{
}

AddVerticalRebar::~AddVerticalRebar()
{
}
void AddVerticalRebar::SorSelcetRebar()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (m_selectrebars.size() > 0)
	{
		for (ElementRefP ref : m_selectrebars)
		{
			EditElementHandle eeh(ref, ref->GetDgnModelP());
			double diameter = 0;
			RebarPoint ptpoint;
			GetStartEndPointFromRebar(&eeh, ptpoint.ptstr, ptpoint.ptend, diameter);
			int dia = (int)(diameter / (uor_per_mm));		
			m_mapselectrebars[dia].push_back(ptpoint);
		}
	}
}
void ExtendLine(DPoint3d& ptstr,DPoint3d& ptend,double dis)
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
void GetIntersetPointsRebarWithRebar(vector<RebarPoint>& rebarpts,vector<DPoint3d>& interpts)
{
	
	if (rebarpts.size()<2)
	{
		return;
	}
	double PosZ = rebarpts[0].ptstr.z;
	for ( int i = 0;i<rebarpts.size();i++)
	{
		if (i==0)
		{
			rebarpts[i].ptstr.z = rebarpts[i].ptend.z = PosZ;
			ExtendLine(rebarpts[i].ptstr, rebarpts[i].ptend, 500);
		}
		for (int j = i+1;j<rebarpts.size();j++)
		{
			if (i==0)
			{
				rebarpts[j].ptstr.z = rebarpts[j].ptend.z = PosZ;
				ExtendLine(rebarpts[j].ptstr, rebarpts[j].ptend, 500);
			}
			DPoint3d intersectpt;
			if (SUCCESS == mdlVec_intersect(&intersectpt,&DSegment3d::From(rebarpts[i].ptstr, rebarpts[i].ptend),&DSegment3d::From(rebarpts[j].ptstr, rebarpts[j].ptend)))
			{
				interpts.push_back(intersectpt);
			}
		}
	}
	if (interpts.size()==4)
	{
		if (interpts[1].Distance(interpts[2])>interpts[1].Distance(interpts[3]))
		{
			DPoint3d tmpPt = interpts[2];
			interpts[2] = interpts[3];
			interpts[3] = tmpPt;
		}
	}

}
void GetInterSectPointsByRebarmap(map<int, vector<RebarPoint>>& mapselectrebars, vector<DPoint3d>& interpts, double& DiameterLong)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	map<int, vector<RebarPoint>>::iterator itr = mapselectrebars.begin();
	vector<RebarPoint> allpts;
	for (; itr != mapselectrebars.end(); itr++)
	{
		if (itr->second.size() > 0)
		{
			/*for (RebarPoint tmppt:itr->second)
			{
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(tmppt.ptstr, tmppt.ptend), true, *ACTIVEMODEL);
				eeh.AddToModel();
			}*/
			allpts.insert(allpts.begin(), itr->second.begin(), itr->second.end());
		}
	}
	vector<DPoint3d> tmpinterpts;
	GetIntersetPointsRebarWithRebar(allpts, tmpinterpts);

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
		DiameterLong = mapselectrebars.rbegin()->first*uor_per_mm;
		vector<RebarPoint> allptsmove;
		itr = mapselectrebars.begin();
		for (; itr != mapselectrebars.end(); itr++)
		{
			if (itr->second.size() > 0)
			{
				for (RebarPoint& tmppt : itr->second)
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
					double moveDis = itr->first*uor_per_mm / 2 + DiameterLong / 2;
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
void  AddVerticalRebar::DrawVerticalReabr()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (m_selectrebars.size()<2)
	{
		return;
	}
	vector<DPoint3d> interpts; double DiameterLong = 0.0;
	GetInterSectPointsByRebarmap(m_mapselectrebars, interpts, DiameterLong);



		//DPoint3d* pts = new DPoint3d[(int)interpts.size()];
		//int i = 0;
		//for (DPoint3d tmppt : interpts)
		//{
		//	pts[i++] = tmppt;
		//}
		//EditElementHandle eeh;
		//LineStringHandler::CreateLineStringElement(eeh, nullptr, pts, interpts.size(), true, *ACTIVEMODEL);
		//eeh.AddToModel();
		//delete[] pts;
		//pts = nullptr;

	//画点筋

		RebarSet * rebset = nullptr;
		EditElementHandle start(m_selectrebars[0], m_selectrebars[0]->GetDgnModelP());
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
					std::vector<RebarPoint> RebarPts;
					GetElementXAttribute(conid, RebarPts, vecRebarPointsXAttribute, ACTIVEMODEL);
					//取最小Z和最大Z值
					double minZ = 0; double maxZ = 0;
					for (RebarPoint tmppt:RebarPts)
					{
						if (tmppt.vecDir == 1)//竖着的钢筋
						{
							minZ = (tmppt.ptstr.z < tmppt.ptend.z) ? tmppt.ptstr.z : tmppt.ptend.z;
							maxZ = (tmppt.ptstr.z > tmppt.ptend.z) ? tmppt.ptstr.z : tmppt.ptend.z;
							break;
						}
					}
					char tmpDiam[256];
					sprintf(tmpDiam, "%d", (int)(DiameterLong / uor_per_mm));
					BrString Sizekey(tmpDiam);
					for (DPoint3d tmppt: interpts)
					{
						RebarVertices  vers;
						RebarVertex*   vertmp1;RebarVertex* vertmp2;
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

						m_rebarPts.push_back(vers);
						m_vecDir.push_back(Sizekey);
					}
					//计算除开四个角的其他点筋
					for (int i = 0;i<interpts.size();i++)
					{
						int j = i + 1;
						if (j== interpts.size())
						{
							j = 0;
						}
						DPoint3d ptstrline = interpts[i];
						DPoint3d ptendline = interpts[j];

						DPoint3d vectmp = ptendline - ptstrline;
						vectmp.Normalize();

						int numbar = 0;
						numbar = (int)(ptstrline.Distance(ptendline) / (200 * uor_per_mm));
						double sideSpacing = ptstrline.Distance(ptendline) / (numbar + 1);
						for (int k = 0;k<numbar;k++)
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

							m_rebarPts.push_back(vers);
							m_vecDir.push_back(Sizekey);

						}


					}


					slabRebar->SetSlabData(ehSel);
					slabRebar->SetvecDirSize(m_vecDir);
					slabRebar->SetrebarPts(m_rebarPts);
					slabRebar->Setspacing(rebset->GetSetData().GetNominalSpacing());
					slabRebar->SetConcreteOwner(conid);
					slabRebar->MakeRebars(modelRef);
					slabRebar->Save(modelRef); // must save after creating rebars
					m_rebarPts.clear();
					m_vecDir.clear();
				}

				m_selectrebars.clear();
				m_mapselectrebars.clear();

			}
		}


}