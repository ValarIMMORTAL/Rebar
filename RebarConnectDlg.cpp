// RebarConnectDlg.cpp: 实现文件
//
#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "RebarConnectDlg.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"
#include "resource.h"
#include "ExtractFacesTool.h"
#include "CCombineRebardlg.h"
/*#include "SelectRebarTool.h"*/
#include "SingleRebarAssembly.h"
#include "XmlHelper.h"
#include "SelectRebarConnectLineTool.h"

// RebarConnectDlg 对话框

IMPLEMENT_DYNAMIC(RebarConnectDlg, CDialogEx)
extern GlobalParameters g_globalpara;
RebarConnectDlg::RebarConnectDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_RebarConnect, pParent)
{

}

RebarConnectDlg::~RebarConnectDlg()
{
}

BOOL RebarConnectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	for each (auto var in g_RebarLinkMethod)
		m_ComboConnectMethod.AddString(var);

	m_RebarConnectInfo.missflg = false;
	m_RebarConnectInfo.ChangeFlg = false;
	//m_RebarConnectInfo.tranLenthFlg = false;
	//InitUIData();
	return true;
}

//void RebarConnectDlg::SorSelcetRebar()
//{
//	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
//	if (m_selectrebars.size() > 0)
//	{
//		for (ElementRefP ref : m_selectrebars)
//		{
//			EditElementHandle eeh(ref, ref->GetDgnModelP());
//			DPoint3d center = getCenterOfElmdescr(eeh.GetElementDescrP());
//			int posz = (int)(center.z / (uor_per_mm * 10));
//			m_mapselectrebars[posz].push_back(ref);
//		}
//	}
//}

void RebarConnectDlg::CalcRebarSet()
{
	map_vecHighRebarSet.clear();
	map_vecLowRebarSet.clear();
	RebarSet * rebset = nullptr;
	map<ElementId, std::vector<ElementRefP>> map_RebarSet;
	DevidRebarGroup(map_RebarSet, m_selectrebars);
	/*for (ElementRefP curRef : m_selectrebars)
	{
		EditElementHandle curElement(curRef, curRef->GetDgnModelP());
		if (RebarElement::IsRebarElement(curElement))
		{
			RebarElementP rep = RebarElement::Fetch(curElement);
			rebset = rep->GetRebarSet(ACTIVEMODEL);

			auto itr_Find = map_RebarSet.find(rebset->GetElementId());
			if (itr_Find == map_RebarSet.end())
			{
				std::vector<ElementRefP> vecElement;
				vecElement.push_back(curRef);

				map_RebarSet.insert(make_pair(rebset->GetElementId(), vecElement));
			}
			else
			{
				itr_Find->second.push_back(curRef);
			}
		}
	}*/
	for (map<ElementId, std::vector<ElementRefP>>::iterator iterhigh = map_RebarSet.begin(); iterhigh != map_RebarSet.end(); iterhigh++)
	{
		ElementRefP elementRefp = iterhigh->second[0];//只需要一组钢筋就可以判断该钢筋是否是竖直方向
		DgnModelRefP model = elementRefp->GetDgnModelP();
		EditElementHandle eehRebar(elementRefp, model);
		RebarElementP pRebar = RebarElement::Fetch(eehRebar);
		if (pRebar == NULL)
		{
			continue;
		}
		Dpoint3d pStart, pEnd;
		double diamter = 0.0;
		CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
		//x,y不同，Z相同就是竖直方向
		if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) != 0 && COMPARE_VALUES_EPS(pStart.x, pEnd.x, 10) == 0 &&
			COMPARE_VALUES_EPS(pStart.y, pEnd.y, 10) == 0)
		{
			if (COMPARE_VALUES_EPS(pStart.z, SelectRebarConnectLineTool::m_dLineHigh, 10) < 0 && COMPARE_VALUES_EPS(pEnd.z, SelectRebarConnectLineTool::m_dLineHigh, 10)< 0)
			{
				map_vecLowRebarSet.insert(*iterhigh);
			}
			else if ((COMPARE_VALUES_EPS(pStart.z, SelectRebarConnectLineTool::m_dLineHigh, 10) < 0 && COMPARE_VALUES_EPS(pEnd.z, SelectRebarConnectLineTool::m_dLineHigh, 10) > 0) ||
				COMPARE_VALUES_EPS(pStart.z, SelectRebarConnectLineTool::m_dLineHigh, 10) > 0 && COMPARE_VALUES_EPS(pEnd.z, SelectRebarConnectLineTool::m_dLineHigh, 10) < 0)
			{
				map_vecHighRebarSet.insert(*iterhigh);
			}
			
		}
	}
	//下面钢筋只需要顶点再最上面的钢筋
	//先求出下面钢筋的最大顶点
	bool bLowFirst = true;
	for (map<ElementId, std::vector<ElementRefP>>::iterator iterhigh = map_vecLowRebarSet.begin(); iterhigh != map_vecLowRebarSet.end(); iterhigh++)
	{
		//ElementId key = iter->first;
		vector<ElementRefP> rebarRef = iterhigh->second;
		for (int i = 0; i < rebarRef.size();i++)
		{
			DgnModelRefP model = rebarRef[i]->GetDgnModelP();
			EditElementHandle eehRebar(rebarRef[i], model);
			RebarElementP pRebar = RebarElement::Fetch(eehRebar);
			Dpoint3d pStart, pEnd;
			double diamter = 0.0;
			CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
			double dHighZ = pStart.z > pEnd.z ? pStart.z : pEnd.z;
			if (bLowFirst)
			{
				m_maxlowZ = dHighZ;
				bLowFirst = false;
			}
			else
			{
				if (dHighZ > m_maxlowZ)
				{
					m_maxlowZ = dHighZ;
				}
			}
		}
	}
	//钢筋顶点等于最大顶点的则保留下来
	for (map<ElementId, std::vector<ElementRefP>>::iterator iterhigh = map_vecLowRebarSet.begin(); iterhigh != map_vecLowRebarSet.end(); iterhigh++)
	{
		ElementId key = iterhigh->first;
		vector<ElementRefP> rebarRef = iterhigh->second;
		vector<ElementRefP> newRebarRef;
		for (int i = 0; i < rebarRef.size(); i++)
		{
			DgnModelRefP model = rebarRef[i]->GetDgnModelP();
			EditElementHandle eehRebar(rebarRef[i], model);
			RebarElementP pRebar = RebarElement::Fetch(eehRebar);
			Dpoint3d pStart, pEnd;
			double diamter = 0.0;
			CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
			double dHighZ = pStart.z > pEnd.z ? pStart.z : pEnd.z;
			if (COMPARE_VALUES_EPS(dHighZ, m_maxlowZ, 10) == 0)
			{
				newRebarRef.push_back(rebarRef[i]);
			}
		}
		map_vecLowRebarSet[key] = newRebarRef;
	}

	bool bHighFirst = true;
	for (map<ElementId, std::vector<ElementRefP>>::iterator iterhigh = map_vecHighRebarSet.begin(); iterhigh != map_vecHighRebarSet.end(); iterhigh++)
	{
		//ElementId key = iter->first;
		vector<ElementRefP> rebarRef = iterhigh->second;
		for (int i = 0; i < rebarRef.size(); i++)
		{
			DgnModelRefP model = rebarRef[i]->GetDgnModelP();
			EditElementHandle eehRebar(rebarRef[i], model);
			RebarElementP pRebar = RebarElement::Fetch(eehRebar);
			Dpoint3d pStart, pEnd;
			double diamter = 0.0;
			CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
			double dLowZ = pStart.z < pEnd.z ? pStart.z : pEnd.z;
			if (bHighFirst)
			{
				m_minHighZ = dLowZ;
				bHighFirst = false;
			}
			else
			{
				if (dLowZ < m_minHighZ)
				{
					m_minHighZ = dLowZ;
				}
			}
		}
	}
	//钢筋顶点等于最小顶点的则保留下来
	for (map<ElementId, std::vector<ElementRefP>>::iterator iterhigh = map_vecHighRebarSet.begin(); iterhigh != map_vecHighRebarSet.end(); iterhigh++)
	{
		ElementId key = iterhigh->first;
		vector<ElementRefP> rebarRef = iterhigh->second;
		vector<ElementRefP> newRebarRef;
		for (int i = 0; i < rebarRef.size(); i++)
		{
			DgnModelRefP model = rebarRef[i]->GetDgnModelP();
			EditElementHandle eehRebar(rebarRef[i], model);
			RebarElementP pRebar = RebarElement::Fetch(eehRebar);
			Dpoint3d pStart, pEnd;
			double diamter = 0.0;
			CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
			double dHighZ = pStart.z < pEnd.z ? pStart.z : pEnd.z;
			if (COMPARE_VALUES_EPS(dHighZ, m_minHighZ, 10) == 0)
			{
				newRebarRef.push_back(rebarRef[i]);
			}
		}
		map_vecHighRebarSet[key] = newRebarRef;
	}

	DeleteAsymmetricalRebar(map_vecLowRebarSet, map_vecHighRebarSet);
	//对每组钢筋按进行排序
	for (map<ElementId, std::vector<ElementRefP>>::iterator iterhigh = map_vecHighRebarSet.begin(); iterhigh != map_vecHighRebarSet.end(); iterhigh++)
	{
		ElementId key = iterhigh->first;
		vector<ElementRefP> rebarRef = iterhigh->second;
		sort(rebarRef.begin(), rebarRef.end(), RebarConnectDlg::sortRebarElementbyXorYpos);
		map_vecHighRebarSet[key] = rebarRef;
	}
	for (map<ElementId, std::vector<ElementRefP>>::iterator iterhigh = map_vecLowRebarSet.begin(); iterhigh != map_vecLowRebarSet.end(); iterhigh++)
	{
		ElementId key = iterhigh->first;
		vector<ElementRefP> rebarRef = iterhigh->second;
		sort(rebarRef.begin(), rebarRef.end(), RebarConnectDlg::sortRebarElementbyXorYpos);
		map_vecLowRebarSet[key] = rebarRef;
	}


}

void RebarConnectDlg::DeleteAsymmetricalRebar(map<ElementId, std::vector<ElementRefP>>& map_vecLowRebarSet, map<ElementId, std::vector<ElementRefP>>& map_vecHighRebarSet)
{
	//去除上下不对称的钢筋
	vector<ElementRefP> rebarAllHighRef;
	for (map<ElementId, std::vector<ElementRefP>>::iterator iterhigh = map_vecHighRebarSet.begin(); iterhigh != map_vecHighRebarSet.end(); iterhigh++)
	{
		ElementId key = iterhigh->first;
		vector<ElementRefP> rebarRef = iterhigh->second;
		for (int i = 0; i < rebarRef.size(); i++)
		{
			rebarAllHighRef.push_back(rebarRef[i]);
		}
	}
	vector<ElementRefP> rebarAllLowRef;
	for (map<ElementId, std::vector<ElementRefP>>::iterator iterhigh = map_vecLowRebarSet.begin(); iterhigh != map_vecLowRebarSet.end(); iterhigh++)
	{
		ElementId key = iterhigh->first;
		vector<ElementRefP> rebarRef = iterhigh->second;
		for (int i = 0; i < rebarRef.size(); i++)
		{
			rebarAllLowRef.push_back(rebarRef[i]);
		}
	}
	vector<ElementRefP> lowRef;
	vector<ElementRefP> HighRef;
	for (int i = 0; i < rebarAllLowRef.size(); i++)
	{
		DgnModelRefP model = rebarAllLowRef[i]->GetDgnModelP();
		EditElementHandle eehRebar(rebarAllLowRef[i], model);
		RebarElementP pRebar = RebarElement::Fetch(eehRebar);
		Dpoint3d pLowStart, pLowEnd;
		double diamter = 0.0;
		CalaRebarStartEnd(pRebar, pLowStart, pLowEnd, diamter, model);
		for (int j = 0; j < rebarAllHighRef.size(); j++)
		{
			DgnModelRefP model = rebarAllHighRef[j]->GetDgnModelP();
			EditElementHandle eehRebar(rebarAllHighRef[j], model);
			RebarElementP pRebar = RebarElement::Fetch(eehRebar);
			Dpoint3d pHighStart, pHighEnd;
			double diamter = 0.0;
			CalaRebarStartEnd(pRebar, pHighStart, pHighEnd, diamter, model);
			if (COMPARE_VALUES_EPS(pLowStart.x, pHighStart.x, 10) == 0 && COMPARE_VALUES_EPS(pLowStart.y, pHighStart.y, 10) == 0)
			{
				lowRef.push_back(rebarAllLowRef[i]);
				HighRef.push_back(rebarAllHighRef[j]);
			}
		}
	}
	DevidRebarGroup(map_vecLowRebarSet, lowRef);
	DevidRebarGroup(map_vecHighRebarSet, HighRef);	
}

void RebarConnectDlg::DevidRebarGroup(map<ElementId, std::vector<ElementRefP>>& map_result, std::vector<ElementRefP> selectRef)
{
	map_result.clear();
	RebarSet * rebset = nullptr;
	for (ElementRefP curRef : selectRef)
	{
		EditElementHandle curElement(curRef, curRef->GetDgnModelP());
		if (RebarElement::IsRebarElement(curElement))
		{
			RebarElementP rep = RebarElement::Fetch(curElement);
			rebset = rep->GetRebarSet(ACTIVEMODEL);

			auto itr_Find = map_result.find(rebset->GetElementId());
			if (itr_Find == map_result.end())
			{
				std::vector<ElementRefP> vecElement;
				vecElement.push_back(curRef);

				map_result.insert(make_pair(rebset->GetElementId(), vecElement));
			}
			else
			{
				itr_Find->second.push_back(curRef);
			}
		}
	}
}

void RebarConnectDlg::CalaRebarStartEnd(RebarElementP rep, DPoint3d& PtStar, DPoint3d& PtEnd, double& diameter, DgnModelRefP modelRef)
{
	RebarCurve curve;
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	RebarShape * rebarshape = rep->GetRebarShape(modelRef);
	if (rebarshape == nullptr)
	{
		return;
	}

	rebarshape->GetRebarCurve(curve);
	BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
	diameter = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);

	CMatrix3D tmp3d(rep->GetLocation());
	curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
	curve.DoMatrix(rep->GetLocation());
	RebarVertices  vers = curve.PopVertices();

	double maxLenth = 0;
	for (int i = 0; i < vers.GetSize() - 1; i++)
	{
		RebarVertex   ver1 = vers.At(i);
		RebarVertex   ver2 = vers.At(i + 1);
		CPoint3D const&     pt1 = ver1.GetIP();
		CPoint3D const&     pt2 = ver2.GetIP();
		DPoint3d tpt1 = DPoint3d::From(pt1.x, pt1.y, pt1.z);
		DPoint3d tpt2 = DPoint3d::From(pt2.x, pt2.y, pt2.z);
		if (i == 0)
		{
			maxLenth = tpt1.Distance(tpt2);
			PtStar = tpt1;
			PtEnd = tpt2;
		}
		else if (maxLenth < tpt1.Distance(tpt2))
		{
			maxLenth = tpt1.Distance(tpt2);
			PtStar = tpt1;
			PtEnd = tpt2;
		}
	}
}

void RebarConnectDlg::UpdateVecRebar()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	
	if (m_selectrebars.size() > 0)
	{
		//vector<RebarElementP > vecRebar;
		if (m_RebarConnectInfo.RebarLinkMethod == 1)//机械连接
		{
			//下层
			for (map<ElementId, vector<ElementRefP>>::iterator iter = map_vecLowRebarSet.begin(); iter != map_vecLowRebarSet.end(); iter++)
			{

				std::vector<ElementRefP> elementRef = iter->second;
				vector<PITRebarCurve> rebars;
				for (unsigned int i = 0; i < elementRef.size(); i++)
				{
					DgnModelRefP model = elementRef.at(i)->GetDgnModelP();
					EditElementHandle eehRebar(elementRef.at(i), model);
					RebarElementP pRebar = RebarElement::Fetch(eehRebar);
					if (pRebar == NULL)
					{
						continue;
					}
					Dpoint3d pStart, pEnd;
					double diamter = 0.0;
					CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
					if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) != 0 && COMPARE_VALUES_EPS(pStart.x, pEnd.x, 10) == 0 &&
						COMPARE_VALUES_EPS(pStart.y, pEnd.y, 10) == 0)
					{
						//vecRebar.push_back(pRebar); 

						if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) > 0)
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
								else
								{

									pStart.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
								else
								{

									
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
							}
							
						}
						else
						{
							if (!m_RebarConnectInfo.ChangeFlg)
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
								else
								{

									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
							}
							else 
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
								else
								{									
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
							}							
						}
						PITRebarEndTypes endTypes;
						endTypes.beg.SetType(PITRebarEndType::kNone);
						endTypes.end.SetType(PITRebarEndType::kNone);
						makeRebarCurve(pStart, pEnd, rebars, endTypes, 0);
						RebarEndType endTypeStart, endTypeEnd;
						endTypeStart.SetType(RebarEndType::kNone);
						endTypeEnd.SetType(RebarEndType::kNone);
						RebarEndTypes endTypes1 = { endTypeStart ,endTypeEnd };
						for (size_t i = 0; i < rebars.size(); ++i)
						{
							RebarShapeData Pshape = /*const_cast<RebarShapeData*>*/*(pRebar->GetShapeData(model));
							PITRebarCurve rebarCurve = rebars[i];
							Pshape.SetLength(rebarCurve.GetLength() / uor_per_mm);
							pRebar->Update(rebarCurve, diamter, endTypes1, Pshape, model, false);/* rebarCurve, diamter, endTypes, Pshape, model, false*/
						}
					}
				}
				
			}
			//上层
			for (map<ElementId, vector<ElementRefP>>::iterator iter = map_vecHighRebarSet.begin(); iter != map_vecHighRebarSet.end(); iter++)
			{

				std::vector<ElementRefP> elementRef = iter->second;
				vector<PITRebarCurve> rebars;
				for (unsigned int i = 0; i < elementRef.size(); i++)
				{
					DgnModelRefP model = elementRef.at(i)->GetDgnModelP();
					EditElementHandle eehRebar(elementRef.at(i), model);
					RebarElementP pRebar = RebarElement::Fetch(eehRebar);
					if (pRebar == NULL)
					{
						continue;
					}
					Dpoint3d pStart, pEnd;
					double diamter = 0.0;
					CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
					if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) != 0 && COMPARE_VALUES_EPS(pStart.x, pEnd.x, 10) == 0 &&
						COMPARE_VALUES_EPS(pStart.y, pEnd.y, 10) == 0)
					{
						//vecRebar.push_back(pRebar);

						if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) > 0)
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
								else
								{

									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
								else
								{									
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
							}
						}
						else
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
								else
								{

									pStart.z = SelectRebarConnectLineTool::m_dLineHigh;
								}

							}	
							else 
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
								else
								{									
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
							}
						}
						PITRebarEndTypes endTypes;
						endTypes.beg.SetType(PITRebarEndType::kNone);
						endTypes.end.SetType(PITRebarEndType::kNone);
						makeRebarCurve(pStart, pEnd, rebars, endTypes, 0);
						RebarEndType endTypeStart, endTypeEnd;
						endTypeStart.SetType(RebarEndType::kNone);
						endTypeEnd.SetType(RebarEndType::kNone);
						RebarEndTypes endTypes1 = { endTypeStart ,endTypeEnd };
						for (size_t i = 0; i < rebars.size(); ++i)
						{
							RebarShapeData Pshape = /*const_cast<RebarShapeData*>*/*(pRebar->GetShapeData(model));
							PITRebarCurve rebarCurve = rebars[i];
							Pshape.SetLength(rebarCurve.GetLength() / uor_per_mm);
							pRebar->Update(rebarCurve, diamter, endTypes1, Pshape, model, false);
						}
					}
				}
			}
		}
		else if (m_RebarConnectInfo.RebarLinkMethod == 0)//搭接
		{
			//下层操作和机械操作一样
			for (map<ElementId, vector<ElementRefP>>::iterator iter = map_vecLowRebarSet.begin(); iter != map_vecLowRebarSet.end(); iter++)
			{

				std::vector<ElementRefP> elementRef = iter->second;
				vector<PITRebarCurve> rebars;
				for (unsigned int i = 0; i < elementRef.size(); i++)
				{
					DgnModelRefP model = elementRef.at(i)->GetDgnModelP();
					EditElementHandle eehRebar(elementRef.at(i), model);
					RebarElementP pRebar = RebarElement::Fetch(eehRebar);
					if (pRebar == NULL)
					{
						continue;
					}
					Dpoint3d pStart, pEnd;
					double diamter = 0.0;
					CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
					if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) != 0 && COMPARE_VALUES_EPS(pStart.x, pEnd.x, 10) == 0 &&
						COMPARE_VALUES_EPS(pStart.y, pEnd.y, 10) == 0)
					{
						//vecRebar.push_back(pRebar); 

						if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) > 0)
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen  * uor_per_mm;
								}
								else
								{

									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.LapLen * uor_per_mm;
								}
							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.LapLen * uor_per_mm;
								}
								else
								{


									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
							}

						}
						else
						{
							if (!m_RebarConnectInfo.ChangeFlg)
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen  * uor_per_mm;
								}
								else
								{

									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.LapLen * uor_per_mm;
								}
							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.LapLen * uor_per_mm;
								}
								else
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen  * uor_per_mm;
								}
							}
						}
						PITRebarEndTypes endTypes;
						endTypes.beg.SetType(PITRebarEndType::kNone);
						endTypes.end.SetType(PITRebarEndType::kNone);
						makeRebarCurve(pStart, pEnd, rebars, endTypes, 0);
						RebarEndType endTypeStart, endTypeEnd;
						endTypeStart.SetType(RebarEndType::kNone);
						endTypeEnd.SetType(RebarEndType::kNone);
						RebarEndTypes endTypes1 = { endTypeStart ,endTypeEnd };
						for (size_t i = 0; i < rebars.size(); ++i)
						{
							RebarShapeData Pshape = /*const_cast<RebarShapeData*>*/*(pRebar->GetShapeData(model));
							PITRebarCurve rebarCurve = rebars[i];
							Pshape.SetLength(rebarCurve.GetLength() / uor_per_mm);
							pRebar->Update(rebarCurve, diamter, endTypes1, Pshape, model, false);/* rebarCurve, diamter, endTypes, Pshape, model, false*/
						}
					}
				}

			}
			//上层
			for (map<ElementId, vector<ElementRefP>>::iterator iter = map_vecHighRebarSet.begin(); iter != map_vecHighRebarSet.end(); iter++)
			{

				std::vector<ElementRefP> elementRef = iter->second;
				vector<PITRebarCurve> rebars;
				for (unsigned int i = 0; i < elementRef.size(); i++)
				{
					DgnModelRefP model = elementRef.at(i)->GetDgnModelP();
					EditElementHandle eehRebar(elementRef.at(i), model);
					RebarElementP pRebar = RebarElement::Fetch(eehRebar);
					if (pRebar == NULL)
					{
						continue;
					}
					Dpoint3d pStart, pEnd;
					double diamter = 0.0;
					CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
					if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) != 0 && COMPARE_VALUES_EPS(pStart.x, pEnd.x, 10) == 0 &&
						COMPARE_VALUES_EPS(pStart.y, pEnd.y, 10) == 0)
					{
						//vecRebar.push_back(pRebar);

						if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) > 0)
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + (m_RebarConnectInfo.MissLen - m_RebarConnectInfo.LapLen) * uor_per_mm;
								}
								else
								{

									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh ;
								}
							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh ;
								}
								else
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + (m_RebarConnectInfo.MissLen - m_RebarConnectInfo.LapLen)* uor_per_mm;
								}
							}
						}
						else
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + (m_RebarConnectInfo.MissLen - m_RebarConnectInfo.LapLen) * uor_per_mm;
								}
								else
								{

									pStart.z = SelectRebarConnectLineTool::m_dLineHigh ;
								}

							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh ;
								}
								else
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + (m_RebarConnectInfo.MissLen - m_RebarConnectInfo.LapLen) * uor_per_mm;
								}
							}
						}
						PITRebarEndTypes endTypes;
						endTypes.beg.SetType(PITRebarEndType::kNone);
						endTypes.end.SetType(PITRebarEndType::kNone);
						makeRebarCurve(pStart, pEnd, rebars, endTypes, 0);
						RebarEndType endTypeStart, endTypeEnd;
						endTypeStart.SetType(RebarEndType::kNone);
						endTypeEnd.SetType(RebarEndType::kNone);
						RebarEndTypes endTypes1 = { endTypeStart ,endTypeEnd };
						for (size_t i = 0; i < rebars.size(); ++i)
						{
							RebarShapeData Pshape = /*const_cast<RebarShapeData*>*/*(pRebar->GetShapeData(model));
							PITRebarCurve rebarCurve = rebars[i];
							Pshape.SetLength(rebarCurve.GetLength() / uor_per_mm);
							pRebar->Update(rebarCurve, diamter, endTypes1, Pshape, model, false);
						}
					}
				}
			}
		}
		else//没选择直接跳过 
		{
		}
		
	}
}

bool RebarConnectDlg::makeRebarCurve(DPoint3d ptstr, DPoint3d ptend, vector<PITRebarCurve>& rebars, PITRebarEndTypes& endTypes, double dSideCover)
{
	DPoint3d pt1[2];
	pt1[0] = ptstr;
	pt1[1] = ptend;
	//确保起点终点是从小到大---begin
	DVec3d vec = pt1[1] - pt1[0];
	DVec3d vecX = DVec3d::From(1, 0, 0);
	vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
	vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
	vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
	vec.Normalize();
	if (vec.IsPerpendicularTo(vecX))
	{
		if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
	}
	else
	{
		if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
	}
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	vector<DPoint3d> tmppts;
	Transform matrix;
	vector<EditElementHandle*> allHoles;
	GetIntersectPointsWithHoles(tmppts, allHoles, pt1[0], pt1[1], dSideCover, matrix,true);
	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt : tmppts)
	{
		if (ExtractFacesTool::IsPointInLine(pt, pt1[0], pt1[1], ACTIVEMODEL, isStr))
		{
			int dis = (int)pt1[0].Distance(pt);
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = pt1[0];
	}
	else
	{
		map_pts[0] = pt1[0];
	}
	int dis = (int)pt1[0].Distance(pt1[1]);
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = pt1[1];
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = pt1[1];
	}



	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{
		PITRebarEndTypes		tmpendTypes;

		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itr->second);
		vex->SetType(RebarVertex::kStart);
		if (pt1[0].Distance(itr->second) < 10)
		{
			tmpendTypes.beg = endTypes.beg;
		}
		tmpendTypes.beg.SetptOrgin(itr->second);
		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		if (pt1[1].Distance(itrplus->second) < 10)
		{
			tmpendTypes.end = endTypes.end;
		}

		tmpendTypes.end.SetptOrgin(itrplus->second);

		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itrplus->second);
		vex->SetType(RebarVertex::kEnd);

		rebar.EvaluateEndTypes(tmpendTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebars.push_back(rebar);
	}


	//rebar.DoMatrix(mat);
	return true;
}

bool RebarConnectDlg::sortRebarElementbyXorYpos(ElementRefP elmentA, ElementRefP elmentB)
{
	DgnModelRefP model = elmentA->GetDgnModelP();
	EditElementHandle eehRebar(elmentA, model);
	RebarElementP pRebar = RebarElement::Fetch(eehRebar);
	if (pRebar == NULL)
	{
		return false;
	}
	Dpoint3d pStart, pEnd;
	double diamter = 0.0;
	CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);

	DgnModelRefP model1 = elmentB->GetDgnModelP();
	EditElementHandle eehRebar1(elmentB, model);
	RebarElementP pRebar1 = RebarElement::Fetch(eehRebar1);
	if (pRebar1 == NULL)
	{
		return false;
	}
	Dpoint3d pStart1, pEnd1;
	double diamter1 = 0.0;
	CalaRebarStartEnd(pRebar1, pStart1, pEnd1, diamter1, model1);
	if (COMPARE_VALUES_EPS(pStart.x, pStart1.x, 10) == 0)//X相等，则比较Y
	{
		return COMPARE_VALUES_EPS(pStart.y, pStart1.y, 10) == 1;
	}
	else 
	{
		return COMPARE_VALUES_EPS(pStart.x, pStart1.x, 10) == 1;
	}
	
}

void RebarConnectDlg::DrawPreviewLine()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (m_vecAllLines.size() > 0)
	{
		//清楚预览线
		for (int i = 0; i < m_vecAllLines.size(); i++)
		{
			EditElementHandle eeh(m_vecAllLines[i], m_vecAllLines[i]->GetDgnModelP());
			eeh.DeleteFromModel();
		}
		m_vecAllLines.clear();
	}
	m_vecAllLines.clear();
	if (m_selectrebars.size() > 0)
	{
		//vector<RebarElementP > vecRebar;
		if (m_RebarConnectInfo.RebarLinkMethod == 1)//机械连接
		{
			//下层
			for (map<ElementId, vector<ElementRefP>>::iterator iter = map_vecLowRebarSet.begin(); iter != map_vecLowRebarSet.end(); iter++)
			{
				std::vector<ElementRefP> elementRef = iter->second;
				vector<PITRebarCurve> rebars;
				for (unsigned int i = 0; i < elementRef.size(); i++)
				{
					DgnModelRefP model = elementRef.at(i)->GetDgnModelP();
					EditElementHandle eehRebar(elementRef.at(i), model);
					RebarElementP pRebar = RebarElement::Fetch(eehRebar);
					if (pRebar == NULL)
					{
						continue;
					}
					Dpoint3d pStart, pEnd;
					double diamter = 0.0;
					CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
					if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) != 0 && COMPARE_VALUES_EPS(pStart.x, pEnd.x, 10) == 0 &&
						COMPARE_VALUES_EPS(pStart.y, pEnd.y, 10) == 0)
					{
						//vecRebar.push_back(pRebar); 

						if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) > 0)
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
								else
								{

									pStart.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
								else
								{


									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
							}

						}
						else
						{
							if (!m_RebarConnectInfo.ChangeFlg)
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
								else
								{

									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
								else
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
							}
						}
						EditElementHandle eeh;
						LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(pStart, pEnd), true, *ACTIVEMODEL);
						ElementPropertiesSetterPtr interlinepropEle = ElementPropertiesSetter::Create();
						interlinepropEle->SetWeight(2);
						interlinepropEle->SetColor(3);//绿色
						interlinepropEle->Apply(eeh);
						eeh.AddToModel();
						m_vecAllLines.push_back(eeh.GetElementRef());
					}
				}
			}

			//上层
			for (map<ElementId, vector<ElementRefP>>::iterator iter = map_vecHighRebarSet.begin(); iter != map_vecHighRebarSet.end(); iter++)
			{

				std::vector<ElementRefP> elementRef = iter->second;
				vector<PITRebarCurve> rebars;
				for (unsigned int i = 0; i < elementRef.size(); i++)
				{
					DgnModelRefP model = elementRef.at(i)->GetDgnModelP();
					EditElementHandle eehRebar(elementRef.at(i), model);
					RebarElementP pRebar = RebarElement::Fetch(eehRebar);
					if (pRebar == NULL)
					{
						continue;
					}
					Dpoint3d pStart, pEnd;
					double diamter = 0.0;
					CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
					if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) != 0 && COMPARE_VALUES_EPS(pStart.x, pEnd.x, 10) == 0 &&
						COMPARE_VALUES_EPS(pStart.y, pEnd.y, 10) == 0)
					{
						//vecRebar.push_back(pRebar);

						if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) > 0)
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
								else
								{

									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
								else
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
							}
						}
						else
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
								else
								{

									pStart.z = SelectRebarConnectLineTool::m_dLineHigh;
								}

							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
								else
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
							}
						}
						EditElementHandle eeh;
						LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(pStart, pEnd), true, *ACTIVEMODEL);
						ElementPropertiesSetterPtr interlinepropEle = ElementPropertiesSetter::Create();
						interlinepropEle->SetWeight(2);
						interlinepropEle->SetColor(5);
						interlinepropEle->Apply(eeh);
						eeh.AddToModel();
						m_vecAllLines.push_back(eeh.GetElementRef());
					}
				}
			}
		}
		else if (m_RebarConnectInfo.RebarLinkMethod == 0)//搭接
		{
		//下层操作和机械操作一样
			for (map<ElementId, vector<ElementRefP>>::iterator iter = map_vecLowRebarSet.begin(); iter != map_vecLowRebarSet.end(); iter++)
			{

				std::vector<ElementRefP> elementRef = iter->second;
				vector<PITRebarCurve> rebars;
				for (unsigned int i = 0; i < elementRef.size(); i++)
				{
					DgnModelRefP model = elementRef.at(i)->GetDgnModelP();
					EditElementHandle eehRebar(elementRef.at(i), model);
					RebarElementP pRebar = RebarElement::Fetch(eehRebar);
					if (pRebar == NULL)
					{
						continue;
					}
					Dpoint3d pStart, pEnd;
					double diamter = 0.0;
					CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
					if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) != 0 && COMPARE_VALUES_EPS(pStart.x, pEnd.x, 10) == 0 &&
						COMPARE_VALUES_EPS(pStart.y, pEnd.y, 10) == 0)
					{
						//vecRebar.push_back(pRebar); 

						if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) > 0)
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen  * uor_per_mm;
								}
								else
								{

									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.LapLen * uor_per_mm;
								}
							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.LapLen * uor_per_mm;
								}
								else
								{


									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen * uor_per_mm;
								}
							}

						}
						else
						{
							if (!m_RebarConnectInfo.ChangeFlg)
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen  * uor_per_mm;
								}
								else
								{

									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.LapLen * uor_per_mm;
								}
							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.LapLen * uor_per_mm;
								}
								else
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + m_RebarConnectInfo.MissLen  * uor_per_mm;
								}
							}
						}
						EditElementHandle eeh;
						LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(pStart, pEnd), true, *ACTIVEMODEL);
						ElementPropertiesSetterPtr interlinepropEle = ElementPropertiesSetter::Create();
						interlinepropEle->SetWeight(2);
						interlinepropEle->SetColor(3);
						interlinepropEle->Apply(eeh);
						eeh.AddToModel();
						m_vecAllLines.push_back(eeh.GetElementRef());
					}
				}

			}
			//上层
			for (map<ElementId, vector<ElementRefP>>::iterator iter = map_vecHighRebarSet.begin(); iter != map_vecHighRebarSet.end(); iter++)
			{

				std::vector<ElementRefP> elementRef = iter->second;
				vector<PITRebarCurve> rebars;
				for (unsigned int i = 0; i < elementRef.size(); i++)
				{
					DgnModelRefP model = elementRef.at(i)->GetDgnModelP();
					EditElementHandle eehRebar(elementRef.at(i), model);
					RebarElementP pRebar = RebarElement::Fetch(eehRebar);
					if (pRebar == NULL)
					{
						continue;
					}
					Dpoint3d pStart, pEnd;
					double diamter = 0.0;
					CalaRebarStartEnd(pRebar, pStart, pEnd, diamter, model);
					if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) != 0 && COMPARE_VALUES_EPS(pStart.x, pEnd.x, 10) == 0 &&
						COMPARE_VALUES_EPS(pStart.y, pEnd.y, 10) == 0)
					{
						//vecRebar.push_back(pRebar);

						if (COMPARE_VALUES_EPS(pStart.z, pEnd.z, 10) > 0)
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + (m_RebarConnectInfo.MissLen - m_RebarConnectInfo.LapLen) * uor_per_mm;
								}
								else
								{

									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
								else
								{
									pEnd.z = SelectRebarConnectLineTool::m_dLineHigh + (m_RebarConnectInfo.MissLen - m_RebarConnectInfo.LapLen)* uor_per_mm;
								}
							}
						}
						else
						{
							if (!m_RebarConnectInfo.ChangeFlg)//切换决定第一根变长还是第二根长
							{
								if (i % 2 != 0 && m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + (m_RebarConnectInfo.MissLen - m_RebarConnectInfo.LapLen) * uor_per_mm;
								}
								else
								{

									pStart.z = SelectRebarConnectLineTool::m_dLineHigh;
								}

							}
							else
							{
								if (i % 2 != 0 || !m_RebarConnectInfo.missflg)
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh;
								}
								else
								{
									pStart.z = SelectRebarConnectLineTool::m_dLineHigh + (m_RebarConnectInfo.MissLen - m_RebarConnectInfo.LapLen) * uor_per_mm;
								}
							}
						}
						EditElementHandle eeh;
						LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(pStart, pEnd), true, *ACTIVEMODEL);
						ElementPropertiesSetterPtr interlinepropEle = ElementPropertiesSetter::Create();
						interlinepropEle->SetWeight(2);
						interlinepropEle->SetColor(5);
						interlinepropEle->Apply(eeh);
						eeh.AddToModel();
						m_vecAllLines.push_back(eeh.GetElementRef());
					}
				}
			}
		}
		else//没选择直接跳过 
		{
		}
	}
}

void RebarConnectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_ConnectType, m_ComboConnectMethod);
	DDX_Control(pDX, IDC_CHECK_CrossConnect, m_Checkmiss);
	DDX_Control(pDX, IDC_EDIT_CrossLength, m_EidtMissLen);
	DDX_Control(pDX, IDC_BUTTON_CrossChange, m_ChangeButton);
}


void RebarConnectDlg::OnBnClickedCancel()
{	
	CDialogEx::OnCancel();
	DestroyWindow();
}

void RebarConnectDlg::OnBnClickedOk()
{
	//CalaVecRebar();

	CDialogEx::OnOK();
	DestroyWindow();
	if (m_vecAllLines.size() > 0)
	{
		//清楚预览线
		for (int i =0;i < m_vecAllLines.size();i++)
		{
			EditElementHandle eeh(m_vecAllLines[i], m_vecAllLines[i]->GetDgnModelP());
			eeh.DeleteFromModel();
		}
		m_vecAllLines.clear();
	}
	CalcRebarSet();
	UpdateVecRebar();
}

void RebarConnectDlg::OnCbnSelchangeCombo1()
{
	m_RebarConnectInfo.RebarLinkMethod = m_ComboConnectMethod.GetCurSel();
	CString strLen = L"";
	if ((m_RebarConnectInfo.RebarLinkMethod == 0) && (m_RebarConnectInfo.missflg))
	{
		strLen = L"0.3L0";
	}
	else if ((m_RebarConnectInfo.RebarLinkMethod == 0) && (!m_RebarConnectInfo.missflg))
	{
		strLen = L"L0";
	}
	else
	{
		m_RebarConnectInfo.MissLen = 400;
		strLen.Format(L"%.2f", m_RebarConnectInfo.MissLen);
	}
	m_EidtMissLen.SetWindowText(strLen);
}

void RebarConnectDlg::OnBnClickedSelectCorssPos()
{
	SelectRebarConnectLineTool::InstallNewInstance(CMD_REBAR_EDIT_REBARCONNECT);
}

void RebarConnectDlg::OnEnChangeEditCrossLength()
{
	CString	strTemp = CString();
	m_EidtMissLen.GetWindowText(strTemp);

	if (strTemp.Find(L"L0") != -1)//有L0为搭接连接的方式， 直接输入L0的倍数
	{
		if (m_RebarConnectInfo.missflg)
		{//错开情况下，0.3L0 -> 2.3L0
			CString	 cstr = strTemp.Left(strTemp.Find(_T("L0")));
			if (cstr == L"")
			{//值为L0的情况，需要加上1在前面
				cstr = L"1";
			}
			m_RebarConnectInfo.MissLen = (atof(CT2A(cstr)) * g_globalpara.m_laplenth[(string)m_SSizekey]);
			m_RebarConnectInfo.MissLen += 1 * g_globalpara.m_laplenth[(string)m_SSizekey];//2 * g_globalpara.m_laplenth[m_sizekey];
			m_RebarConnectInfo.LapLen = g_globalpara.m_laplenth[(string)m_SSizekey];
		}
		else
		{//不错开的情况下，L0 -> L0
			CString	 cstr = strTemp.Left(strTemp.Find(_T("L0")));
			if (cstr == L"")
			{//值为L0的情况，需要加上1在前面
				cstr = L"1";
			}
			m_RebarConnectInfo.MissLen = (atof(CT2A(cstr)) * g_globalpara.m_laplenth[(string)m_SSizekey]);
			m_RebarConnectInfo.LapLen = g_globalpara.m_laplenth[(string)m_SSizekey];
		}

	}
	else
	{
		m_RebarConnectInfo.MissLen = atof(CT2A(strTemp));
	}
	//CalculateVertexAndDrawLines();
}

void RebarConnectDlg::OnBnClickedCrossCheck()
{
	if (m_Checkmiss.GetCheck())
	{
		m_RebarConnectInfo.missflg = true;//错开
		CString strLen = L"";

		if (m_RebarConnectInfo.RebarLinkMethod == 0)
		{
			strLen = L"0.3L0";
			m_EidtMissLen.SetWindowText(strLen);
		}
		else
		{
			m_RebarConnectInfo.MissLen = 400;
			strLen.Format(L"%.2f", m_RebarConnectInfo.MissLen);
			m_EidtMissLen.SetWindowText(strLen);
		}
	}
	else
	{
		m_RebarConnectInfo.missflg = false;
		CString strLen = L"";
		if (m_RebarConnectInfo.RebarLinkMethod == 0)
		{
			strLen = L"L0";
			m_EidtMissLen.SetWindowText(strLen);
		}
		else
		{
			m_RebarConnectInfo.MissLen = 400;
			strLen.Format(L"%.2f", m_RebarConnectInfo.MissLen);
			m_EidtMissLen.SetWindowText(strLen);
		}
	}
}

void RebarConnectDlg::OnBnClickedCrossChange()
{
	if (!m_RebarConnectInfo.ChangeFlg)
	{
		m_RebarConnectInfo.ChangeFlg = true;
		//change = true;
	}
	else
	{
		m_RebarConnectInfo.ChangeFlg = false;
		//change = false;
	}
	//CalculateVertexAndDrawLines();
}

BEGIN_MESSAGE_MAP(RebarConnectDlg, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &RebarConnectDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &RebarConnectDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBO_ConnectType, &RebarConnectDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON_SelectCrossPos, &RebarConnectDlg::OnBnClickedSelectCorssPos)
	ON_EN_CHANGE(IDC_EDIT_CrossLength, &RebarConnectDlg::OnEnChangeEditCrossLength)
	ON_BN_CLICKED(IDC_CHECK_CrossConnect, &RebarConnectDlg::OnBnClickedCrossCheck)
	ON_BN_CLICKED(IDC_BUTTON_CrossChange, &RebarConnectDlg::OnBnClickedCrossChange)
	ON_BN_CLICKED(IDC_BUTTON_RebarConnectPreView, &RebarConnectDlg::OnBnClickedButtonRebarconnectpreview)
END_MESSAGE_MAP()


// RebarConnectDlg 消息处理程序


void RebarConnectDlg::OnBnClickedButtonRebarconnectpreview()
{
	// TODO: 在此添加控件通知处理程序代码
	CalcRebarSet();
	DrawPreviewLine();
}
