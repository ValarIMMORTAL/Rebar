/*--------------------------------------------------------------------------------------+
|
|     $Source: WallRebarAssembly.cpp $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include "RebarDetailElement.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "HoleRebarAssembly.h"
#include "resource.h"
#include "WallRebarDlg.h"
#include "ExtractFacesTool.h"
#include "CWallRebarDlg.h"
#include "TieRebar.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "CHoleRebar_ReinforcingDlg.h"
#include "CHoleRebar_StructualDlg.h"
#include "XmlHelper.h"
#include "PITRebarCurve.h"
#include "PITMSCECommon.h"
#include "ExtractFacesTool.h"
#include "WallRebarAssembly.h"
#include "URebar.h"
void GetRebarIDSIntesectWithHole(int& minid, int& maxid, EditElementHandleR eehHole, vector<RebarPoint>& rebarPts)
{
	int i = 0;
	DPoint3d minP = { 0 }, maxP = { 0 };
	mdlElmdscr_computeRange(&minP, &maxP, eehHole.GetElementDescrP(), NULL);

	for (RebarPoint rpts : rebarPts)
	{

		if (rpts.vecDir == 1)
		{
			if (rpts.ptstr.x<minP.x || rpts.ptstr.x >maxP.x)
			{
				i++;
				continue;
			}
		}
		else
		{
			if (rpts.ptstr.z<minP.z || rpts.ptstr.z >maxP.z)
			{
				i++;
				continue;
			}
		}
		vector<DPoint3d> ItrPts;
		DPoint3d ptstr = rpts.ptstr;
		DPoint3d ptend = rpts.ptend;
		GetIntersectPointsWithHole(ItrPts, &eehHole, ptstr, ptend);
		if (ItrPts.size() > 0)
		{
			if (minid == -1)
			{
				minid = i;
			}
			else if (minid > i)
			{
				minid = i;
			}
			if (maxid < i)
			{
				maxid = i;
			}
		}
		i++;
	}
}
//将所有孔洞和钢筋点转换到ACS坐标系下
HoleRFRebarAssembly::HoleRFRebarAssembly(ElementId id, DgnModelRefP modelRef) :
	RebarAssembly(id, modelRef)
{
	m_holedlg = nullptr;
	Init();
}

void HoleRFRebarAssembly::Init()
{
	
}

void HoleRFRebarAssembly::ClearData()
{
	m_Trans.InitIdentity();
	m_vecDirSize.clear();
	m_vecRebarLevel.clear();
	m_rebarPts.clear();
	m_vecReinF.clear();
	m_holeidAndmodel.clear();
	m_LayerRebars.clear();
	m_holedlg = nullptr;
}
void HoleRFRebarAssembly::CalculateTransByFrontPts(HoleRebarInfo::ReinForcingInfo tmpinfo)
{
	if (tmpinfo.isUnionHole)
	{
		
		string uname(tmpinfo.Hname);
		vector<string> vecUnionchildname;
		for (HoleRebarInfo::ReinForcingInfo tmpinfo : m_vecReinF)
		{
			if (tmpinfo.isUnionChild)
			{
				string tuname(tmpinfo.Uname);
				if (tuname.find(uname) != string::npos)
				{
					string hname(tmpinfo.Hname);
					vecUnionchildname.push_back(hname);
				}
			}
		}
		int i = 0;
		for (string tname : vecUnionchildname)
		{
			if (m_holeidAndmodel[tname].ID != 0 && m_holeidAndmodel[tname].tModel != nullptr)
			{
				EditElementHandle eeh(m_holeidAndmodel[tname].ID, m_holeidAndmodel[tname].tModel);
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
				Transform trans;	
				CalculateHoleTransByFrontPoints(eeh, m_vecFrontPts, trans, isfloor);				
				SetTrans(trans);
			
			}
		}
	}
	else
	{
		if (m_holeidAndmodel[tmpinfo.Hname].ID != 0 && m_holeidAndmodel[tmpinfo.Hname].tModel != nullptr)
		{
			EditElementHandle eeh(m_holeidAndmodel[tmpinfo.Hname].ID, m_holeidAndmodel[tmpinfo.Hname].tModel);
			ElementCopyContext copier2(ACTIVEMODEL);
			copier2.SetSourceModelRef(eeh.GetModelRef());
			copier2.SetTransformToDestination(true);
			copier2.SetWriteElements(false);
			copier2.DoCopy(eeh);		
			Transform trans;
			CalculateHoleTransByFrontPoints(eeh, m_vecFrontPts, trans, isfloor);
			SetTrans(trans);
		}
		
	}
}
bool HoleRFRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	if (m_vecFrontPts.size() == 0)
	{
		return false;
	}
	m_vecSetId.resize(m_vecDirSize.size());
	for (size_t i = 0; i < m_vecSetId.size(); i++)
	{
		m_vecSetId[i] = 0;
	}
	SetLayerRebars();

	EditElementHandle eeh(GetSelectedElement(), GetSelectedModel());
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(eeh, Eleeh, Holeehs);
	SetSelectedElement(Eleeh.GetElementId());
	NewRebarAssembly(modelRef);
	SetSelectedElement(eeh.GetElementId());

	RebarSetTagArray rsetTags;
	int tagID = 1;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	for (int j = 0;j<m_vecReinF.size();j++)
	{
		HoleRebarInfo::ReinForcingInfo tmpinfo;
		tmpinfo = m_vecReinF.at(j);
		//if (tmpinfo.isUnionChild)//联合孔洞
		//{
		//	continue;
		//}
		if (!tmpinfo.v1&& !tmpinfo.v2&&!tmpinfo.h3&& !tmpinfo.h4)
		{
			continue;
		}
		EditElementHandle eehHole;
		CalculateTransByFrontPts(tmpinfo);
		if (tmpinfo.isUnionHole)
		{
			double hsize;
			string uname(tmpinfo.Hname);
			HoleRFRebarAssembly::GetUnionHoleeehAndSize(&eehHole, hsize,
				m_vecReinF, GetTrans(), uname, m_holeidAndmodel);
		}
		else
		{
			if (m_holeidAndmodel[tmpinfo.Hname].ID!=0&& m_holeidAndmodel[tmpinfo.Hname].tModel!=nullptr)
			{
				EditElementHandle eeh(m_holeidAndmodel[tmpinfo.Hname].ID, m_holeidAndmodel[tmpinfo.Hname].tModel);
				eehHole.Duplicate(eeh);
				ISolidKernelEntityPtr entityPtr;
				if (SolidUtil::Convert::ElementToBody(entityPtr, eehHole) == SUCCESS)
				{
					SolidUtil::Convert::BodyToElement(eehHole, *entityPtr, nullptr, *eehHole.GetModelRef());
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(eehHole.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(eehHole);
				}
			}
			else
			{
				continue;
			}
		}
	
		

		EditElementHandle eehHolePlus;//放大后的孔洞
		eehHolePlus.Duplicate(eehHole);
		PlusSideCover(eehHolePlus, g_wallRebarInfo.concrete.sideCover*uor_per_mm, GetTrans());

		//eehHole.AddToModel();
		//eehHolePlus.AddToModel();
		DPoint3d ptcenter = getCenterOfElmdescr(eehHole.GetElementDescrP());

		int theSec = 0;
		if (!isfloor)//只有墙时才需要算，板只有一段
		{
			
			for (int k = 0; k < m_vecFrontPts.size() - 1; k++)
			{
				DPoint3d tmpstr = m_vecFrontPts.at(k);
				DPoint3d tmpend = m_vecFrontPts.at(k + 1);

				vector<DPoint3d> intresectpts;
				tmpstr.z = tmpend.z = ptcenter.z;
				GetIntersectPointsWithHole(intresectpts, &eehHole, tmpstr, tmpend);
				if (intresectpts.size() > 0)
				{
					break;
				}
				if (k>0)
				{
					DPoint3d tmpstrfront = m_vecFrontPts.at(0);
					DPoint3d tmpendfront = m_vecFrontPts.at(1);

					DPoint3d tmpvecfront = tmpendfront - tmpstrfront;
					tmpvecfront.z = 0;
					tmpvecfront.Normalize();
					DPoint3d nowvec = tmpend - tmpstr;
					nowvec.z = 0;
					nowvec.Normalize();

					double dotvalue = nowvec.DotProduct(tmpvecfront);
					if (abs(dotvalue)<0.01)//垂直
					{
						continue;
					}

				}

				theSec++;
			}
		}
		

		TransformInfo transinfo(GetTrans());
		eehHole.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHole, transinfo);
		eehHolePlus.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHolePlus, transinfo);
		//eehHole.AddToModel();
		//eehHolePlus.AddToModel();
		
		
	 for (int i = 0; i < m_vecDirSize.size(); ++i)
	 {
			 RebarSetTag* tag = NULL;
			// double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;
			 PopvecSetId().at(i) = i;
			 vector<RebarPoint> tmpPoints;
			 std::map<int, vector<RebarPoint>> tmpLayerRebars;//通过段数筛分钢筋线
			 for (RebarPoint tmppt: m_LayerRebars[i])//筛选掉和孔洞没有交的主筋线，主要针对多段墙时
			 {
				 tmpLayerRebars[tmppt.sec].push_back(tmppt);
			 }
			 if (tmpLayerRebars.size()<theSec)
			 {
				 continue;
			 }
			 tmpPoints = tmpLayerRebars[theSec];
			 tag = MakeRebars(tmpinfo,eehHole,eehHolePlus, tmpPoints,PopvecSetId().at(i), GetvecDirSize().at(i), ACTIVEMODEL, GetvecRebarLevel().at(i));
			 if (NULL != tag)
			 {
				 tag->SetBarSetTag(tagID++);
				 rsetTags.Add(tag);
			 }
		 	
		//		}
	 }
	}

	


	

	//end
	/*AddRebarSets(rsetTags);
	RebarSets rebar_sets;
	GetRebarSets(rebar_sets, ACTIVEMODEL);
	return true;*/
	//return true;
	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;

}
void HoleRFRebarAssembly::SetLayerRebars()
{
	m_LayerRebars.clear();
	int numFront = 0;
	int numBack = 0;
	int numMid = 0;
	//统计前，中，后层数
	for (RebarPoint pt : m_rebarPts)
	{
		if (pt.DataExchange==0)//前面
		{
			if (pt.Layer>numFront)
			{
				numFront = pt.Layer;
			}
		}
		else if (pt.DataExchange==1)//中间
		{
			if (pt.Layer > numMid)
			{
				numMid = pt.Layer;
			}
		}
		else //背面 
		{
			if (pt.Layer > numBack)
			{
				numBack = pt.Layer;
			}
		}
	}
	for (RebarPoint pt:m_rebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
		   m_LayerRebars[pt.Layer-1].push_back(pt);
		}
		else if (pt.DataExchange == 1)//中间
		{
			m_LayerRebars[pt.Layer + numFront-1].push_back(pt);
		}
		else //背面 
		{
			m_LayerRebars[numFront + numMid + numBack - pt.Layer].push_back(pt);
		}
	}

}

void  HoleRFRebarAssembly::TransFromRebarPts(vector<RebarPoint>&  rebarPts)
{
	TransformInfo transinfo(GetTrans());
	for (int i = 0;i<rebarPts.size();i++)
	{
		DPoint3d ptstr = rebarPts.at(i).ptstr;
		DPoint3d ptend = rebarPts.at(i).ptend;
		EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
		//eeh.AddToModel();
		if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
		{
			rebarPts.at(i).ptstr = ptstr;
			rebarPts.at(i).ptend = ptend;
		}
		
	}
	
}


RebarSetTag* HoleRFRebarAssembly::MakeRebars
(
	HoleRebarInfo::ReinForcingInfo tmpinfo,
	EditElementHandle& eehHole,
	EditElementHandle& eehHolePlus,
	vector<RebarPoint>&  rebarPts,
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	DgnModelRefP        modelRef,
	int DataExchange
)
{
	int layernow = 0;//当前主筋的层号
	int DiamUsizeKey = 0;
	double DiameterU;
	for (int i=0;i<m_vecDirSize.size();i++)
	{
		int tmpSize = BeStringUtilities::Wtoi(m_vecDirSize.at(i));
		if (tmpSize>DiamUsizeKey)
		{
			DiamUsizeKey = tmpSize;
		}
	}
	if (DiamUsizeKey>25)
	{
		DiameterU = RebarCode::GetBarDiameter(L"20C", modelRef);
	}
	else 
	{
		DiameterU = RebarCode::GetBarDiameter(L"16C", modelRef);
	}
	
	vector<RebarPoint> tmprebarPts;
	tmprebarPts.insert(tmprebarPts.begin(),rebarPts.begin(), rebarPts.end());
	TransFromRebarPts(tmprebarPts);

	bool const isStirrup = false;
	ElementId tmpID = rebarSetId;
	rebarSetId = 0;
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;
	switch (0)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeStart.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (0)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeEnd.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}
	//	}
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
	double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100

	DiameterU = bendRadius + DiameterU / 2;

	 tmpinfo.L0Lenth = g_globalpara.m_laplenth[(string)sizeKey];//搭接长度
	 if (tmpinfo.L0Lenth==0)
	 {
		 tmpinfo.L0Lenth = 200;
	 }
	    vector<RebarCurve>     rebarCurvesNum;
		RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
		if (tmprebarPts.size()>0)
		{
			layernow = tmprebarPts[0].Layer;
			DPoint3d minP = { 0 }, maxP = { 0 };
			mdlElmdscr_computeRange(&minP, &maxP, eehHole.GetElementDescrP(), NULL);
			int minXID = -1; int maxXID = -1;
			GetRebarIDSIntesectWithHole(minXID, maxXID, eehHolePlus, tmprebarPts);
			double minxpos, maxxpos, minzpos, maxzpos;
			minxpos = (tmprebarPts.begin()->ptstr.x < tmprebarPts.rbegin()->ptstr.x) ? tmprebarPts.begin()->ptstr.x : tmprebarPts.rbegin()->ptstr.x;
			maxxpos = (tmprebarPts.begin()->ptstr.x > tmprebarPts.rbegin()->ptstr.x) ? tmprebarPts.begin()->ptstr.x : tmprebarPts.rbegin()->ptstr.x;
			minzpos = (tmprebarPts.begin()->ptstr.z < tmprebarPts.rbegin()->ptstr.z) ? tmprebarPts.begin()->ptstr.z : tmprebarPts.rbegin()->ptstr.z;
			maxzpos = (tmprebarPts.begin()->ptstr.z > tmprebarPts.rbegin()->ptstr.z) ? tmprebarPts.begin()->ptstr.z : tmprebarPts.rbegin()->ptstr.z;
			if (minXID==-1||maxXID==-1)
			{
				return false;
			}
			
			Transform inversMat = GetTrans();
			inversMat.InverseOf(inversMat);
			TransformInfo transinfo(inversMat);	
			if (minXID == 1)//如果刚好为最上面一根，取下面一根钢筋来配置加强筋
			{
				minXID = 2;
			}
			if (maxXID == tmprebarPts.size() - 1)//刚好为最下面一根，取前一根钢筋配置加强筋
			{
				maxXID = tmprebarPts.size() - 2;
			}
				if (tmprebarPts[0].vecDir == 0)//X方向钢筋
				{
					if (maxXID + 1 < tmprebarPts.size() &&tmpinfo.v1)//上边
					{
						int numRebar = tmpinfo.numv1;
						
						for (int i=0;i<numRebar;i++)
						{
							double spacing = i * tmpinfo.spacingv1*uor_per_mm;
							RebarPoint rebrptmin = tmprebarPts.at(maxXID + 1);
							double Posz;
							if (rebrptmin.ptstr.z - maxP.z > 150 * uor_per_mm)
							{
								if (!isfloor)
									Posz = maxP.z + g_wallRebarInfo.concrete.sideCover*uor_per_mm + DiameterU + spacing;
								else
									Posz = maxP.z - (g_wallRebarInfo.concrete.sideCover*uor_per_mm + DiameterU + spacing);
							}
							else
							{
								if (!isfloor)
									Posz = rebrptmin.ptstr.z + tmpinfo.MainRebarDis*uor_per_mm + diameter + spacing;
								else
									Posz = rebrptmin.ptstr.z - (tmpinfo.MainRebarDis*uor_per_mm + diameter + spacing);
							}
							if (Posz > maxzpos)
							{
								continue;
							}
							DPoint3d ptstr = rebrptmin.ptstr;
							DPoint3d ptend = rebrptmin.ptstr;
							ptstr.x = minP.x; ptend.x = maxP.x;
							double minXPos = (rebrptmin.ptstr.x < rebrptmin.ptend.x) ? rebrptmin.ptstr.x : rebrptmin.ptend.x;
							double maxXPos = (rebrptmin.ptstr.x > rebrptmin.ptend.x) ? rebrptmin.ptstr.x : rebrptmin.ptend.x;
							ptstr.x = ptstr.x - 200 * uor_per_mm - tmpinfo.L0Lenth*uor_per_mm;
							ptend.x = ptend.x + 200 * uor_per_mm + tmpinfo.L0Lenth*uor_per_mm;
							if (ptstr.x < minXPos)
							{
								ptstr.x = minXPos;
							}
							if (ptend.x > maxXPos)
							{
								ptend.x = maxXPos;
							}
							ptstr.z = Posz; ptend.z = Posz;
							RebarCurve     rebarCurves;
							EditElementHandle eeh;
							LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
							eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
							if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
							{
								makeRebarCurve(rebarCurves, bendRadius, bendLen, endTypes, ptstr, ptend);
								rebarCurvesNum.push_back(rebarCurves);
							}
						}
						
					}
					if (minXID - 1 > 0 &&tmpinfo.v2)//下边
					{
						int numRebar = tmpinfo.numv2;

						for (int i = 0; i < numRebar; i++)
						{
							double spacing = i * tmpinfo.spacingv2*uor_per_mm;
							RebarPoint rebrptmin = tmprebarPts.at(minXID - 1);
							
							double Posz;
							if (minP.z - rebrptmin.ptstr.z > 150 * uor_per_mm)
							{
								if (!isfloor)
									Posz = minP.z - g_wallRebarInfo.concrete.sideCover*uor_per_mm - DiameterU - spacing;
								else
									Posz = minP.z + (g_wallRebarInfo.concrete.sideCover*uor_per_mm + DiameterU + spacing);
							}
							else
							{
								if(!isfloor)
									Posz = rebrptmin.ptstr.z - tmpinfo.MainRebarDis * uor_per_mm - diameter - spacing;
								else
									Posz = rebrptmin.ptstr.z + (tmpinfo.MainRebarDis * uor_per_mm + diameter + spacing);
							}
							if (Posz < minzpos)
							{
								continue;
							}
							DPoint3d ptstr = rebrptmin.ptstr;
							DPoint3d ptend = rebrptmin.ptstr;
							ptstr.x = minP.x; ptend.x = maxP.x;
							double minXPos = (rebrptmin.ptstr.x < rebrptmin.ptend.x) ? rebrptmin.ptstr.x : rebrptmin.ptend.x;
							double maxXPos = (rebrptmin.ptstr.x > rebrptmin.ptend.x) ? rebrptmin.ptstr.x : rebrptmin.ptend.x;
							ptstr.x = ptstr.x - 200 * uor_per_mm - tmpinfo.L0Lenth*uor_per_mm;
							ptend.x = ptend.x + 200 * uor_per_mm + tmpinfo.L0Lenth*uor_per_mm;
							if (ptstr.x < minXPos)
							{
								ptstr.x = minXPos;
							}
							if (ptend.x > maxXPos)
							{
								ptend.x = maxXPos;
							}
							ptstr.z = Posz; ptend.z = Posz;
							RebarCurve     rebarCurves;

							EditElementHandle eeh;
							LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
							eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);

							if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
							{
								makeRebarCurve(rebarCurves, bendRadius, bendLen, endTypes, ptstr, ptend);
								rebarCurvesNum.push_back(rebarCurves);
							}
						}
					}
				}
				else//Z方向钢筋
				{
					if (minXID - 1 > 0 &&tmpinfo.h3)//左边
					{
						int numRebar = tmpinfo.numh3;

						for (int i = 0; i < numRebar; i++)
						{
							double spacing = i * tmpinfo.spacingh3*uor_per_mm;
							RebarPoint rebrptmin = tmprebarPts.at(minXID - 1);
							double Posx;
							if (minP.x - rebrptmin.ptstr.x > 150 * uor_per_mm)
							{
								Posx = minP.x - g_wallRebarInfo.concrete.sideCover*uor_per_mm - DiameterU  - spacing;
							}
							else
							{
								Posx = rebrptmin.ptstr.x - tmpinfo.MainRebarDis * uor_per_mm - diameter - spacing;
							}
							if (Posx < minxpos)
							{
								continue;
							}
							DPoint3d ptstr = rebrptmin.ptstr;
							DPoint3d ptend = rebrptmin.ptstr;
							ptstr.z = minP.z; ptend.z = maxP.z;
							double minZPos = (rebrptmin.ptstr.z < rebrptmin.ptend.z) ? rebrptmin.ptstr.z : rebrptmin.ptend.z;
							double maxZPos = (rebrptmin.ptstr.z > rebrptmin.ptend.z) ? rebrptmin.ptstr.z : rebrptmin.ptend.z;
							ptstr.z = ptstr.z - 200 * uor_per_mm - tmpinfo.L0Lenth*uor_per_mm;
							ptend.z = ptend.z + 200 * uor_per_mm + tmpinfo.L0Lenth*uor_per_mm;
							if (ptstr.z < minZPos)
							{
								ptstr.z = minZPos;
							}
							if (ptend.z > maxZPos)
							{
								ptend.z = maxZPos;
							}
							ptstr.x = Posx; ptend.x = Posx;
							RebarCurve     rebarCurves;
							EditElementHandle eeh;
							LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
							eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
							if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
							{
								makeRebarCurve(rebarCurves, bendRadius, bendLen, endTypes, ptstr, ptend);
								rebarCurvesNum.push_back(rebarCurves);
							}
						}

					}
					if (maxXID + 1 < tmprebarPts.size() && tmpinfo.h4)//右边
					{
						int numRebar = tmpinfo.numh4;

						for (int i = 0; i < numRebar; i++)
						{
							double spacing = i * tmpinfo.spacingh4*uor_per_mm;
							RebarPoint rebrptmin = tmprebarPts.at(maxXID + 1);
							double Posx;
							if (rebrptmin.ptstr.x - maxP.x > 150 * uor_per_mm)
							{
								Posx = maxP.x + g_wallRebarInfo.concrete.sideCover*uor_per_mm + DiameterU  + spacing;
							}
							else
							{
								Posx = rebrptmin.ptstr.x + tmpinfo.MainRebarDis * uor_per_mm + diameter + spacing;
							}
							if (Posx > maxxpos)
							{
								continue;
							}
							DPoint3d ptstr = rebrptmin.ptstr;
							DPoint3d ptend = rebrptmin.ptstr;
							ptstr.z = minP.z; ptend.z = maxP.z;
							double minZPos = (rebrptmin.ptstr.z < rebrptmin.ptend.z) ? rebrptmin.ptstr.z : rebrptmin.ptend.z;
							double maxZPos = (rebrptmin.ptstr.z > rebrptmin.ptend.z) ? rebrptmin.ptstr.z : rebrptmin.ptend.z;
							ptstr.z = ptstr.z - 200 * uor_per_mm - tmpinfo.L0Lenth*uor_per_mm;
							ptend.z = ptend.z + 200 * uor_per_mm + tmpinfo.L0Lenth*uor_per_mm;
							if (ptstr.z < minZPos)
							{
								ptstr.z = minZPos;
							}
							if (ptend.z > maxZPos)
							{
								ptend.z = maxZPos;
							}
							ptstr.x = Posx; ptend.x = Posx;
							RebarCurve     rebarCurves;
							EditElementHandle eeh;
							LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
							eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
							if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
							{
								makeRebarCurve(rebarCurves, bendRadius, bendLen, endTypes, ptstr, ptend);
								rebarCurvesNum.push_back(rebarCurves);
							}
						}
					}
				}
			
			/*for (int i = 0; i < m_rebarPts.size()-1; i++)
			{
				int j = i + 1;
				CPoint3D ptstr(m_rebarPts.at(i).pt);
				CPoint3D ptend(m_rebarPts.at(j).pt);
				if (m_rebarPts.at(i).Layer == tmpID && m_rebarPts.at(i).Layer == m_rebarPts.at(j).Layer)
				{
					RebarCurve     rebarCurves;
					TransformInfo transinfo(GetTrans());
					EditElementHandle eeh;
					LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
					eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
					eeh.AddToModel();
					makeRebarCurve(rebarCurves, bendRadius, bendLen, endTypes, ptstr, ptend);
					rebarCurvesNum.push_back(rebarCurves);
				}
				i++;
			}*/
		}
		


	RebarSymbology symb;
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_CAVE_REBAR);
	}
	int	numRebar = (int)rebarCurvesNum.size();
	int j = 0;
	char nowLayerc[256];
	sprintf(nowLayerc, "%d", layernow);
	string nowLayerstring(nowLayerc);
	for (RebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
	    
		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string strname = tmpinfo.Hname;

			string RebarType;
			if (DataExchange == 0)
			{
				RebarType = "/HoleForcefrontRebar";
			}
			else if (DataExchange == 1)
			{
				RebarType = "/HoleForcemiddenRebar";
			}
			else
			{
				RebarType = "/HoleForcebackRebar";
			}


			string Stype = strname + RebarType;
			ElementRefP oldref = tmprebar.GetElementRef();
			int num = Gettypenum();
			SetRebarLevelItemTypeValue(tmprebar, nowLayerstring, num, Stype, modelRef);
			//SetRebarLevelItemTypeValue(tmprebar,Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(200);
	setdata.SetAverageSpacing(200);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}
bool HoleRFRebarAssembly::makeRebarCurve
(
    RebarCurve&     rebar,
	double                  bendRadius,
	double                  bendLen,
	RebarEndTypes const&    endTypes,
	CPoint3D const&        ptstr,
	CPoint3D const&        ptend
)
{
	DPoint3d midPos = ptstr;
	midPos.Add(ptend);
	midPos.Scale(0.5);

	DPoint3d dstr = ptstr;
	DPoint3d dend = ptend;

	CalculateIntersetPtWithHolesWithRebarCuve(dstr, dend, midPos, m_useHoleehs);
	CVector3D ptstr2(dstr);
	CVector3D ptend2(dend);
	RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptstr2);
	vex->SetType(RebarVertex::kStart);


	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptend2);
	vex->SetType(RebarVertex::kEnd);

	CVector3D   endNormal(-1.0, 0.0, 0.0);
	rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, endNormal);
	//rebar.DoMatrix(mat);
	return true;
}
long HoleRFRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}

bool HoleRFRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	SetSelectedModel(ehSel.GetModelRef());
	GetConcreteXAttribute(GetConcreteOwner(), ACTIVEMODEL);

	DgnModelRefP modelRef = ACTIVEMODEL;


	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_holedlg = new CHoleRebar_ReinforcingDlg;
	m_holedlg->SetSelectElement(ehSel);
	m_holedlg->Create(IDD_DIALOG_HoleRebar_Reinforcing, CWnd::FromHandle(mdlNativeWindow_getMainHandle(0)));
	m_holedlg->ShowWindow(SW_SHOW);
	//m_holedlg->m_HoleRebar = this;
	return true;
}

bool HoleRFRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();
	MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();
	//SetElementXAttribute(ehSel.GetElementId(), g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ehSel.GetModelRef());
	//eeh2.AddToModel();
	return true;
}
RebarAssembly* HoleRFRebarAssembly::GetRebarAssembly(ElementId concreteid,string assemblyname)
{
	RebarAssemblies area;
	REA::GetRebarAssemblies(concreteid, area);
	for (int i = 0; i < area.GetSize(); i++)
	{
		RebarAssembly* rebaras = area.GetAt(i);
		string tesname = typeid(*rebaras).name();
		if (tesname == assemblyname)
		{
			return rebaras;
		}
	}
	return nullptr;
}
void HoleRFRebarAssembly::GetUnionHoleeehAndSize(EditElementHandle* eehhole,double& holesize,
	                          vector<HoleRebarInfo::ReinForcingInfo>& vecReininfo, Transform trans,string UnionName,
	                          std::map<std::string, IDandModelref>& m_holeidAndmodel)
{
	vector<string> vecUnionchildname;
	for (HoleRebarInfo::ReinForcingInfo tmpinfo:vecReininfo)
	{
		if (tmpinfo.Uname != "")
		{
			string uname(tmpinfo.Uname);
				if (uname.find(UnionName)!=string::npos)
				{
					string hname(tmpinfo.Hname);
					vecUnionchildname.push_back(hname);
				}
		}
		
	
	}
	TransformInfo tinfo(trans);
	DPoint3d ptmin = { 0 }, ptmax = {0};
	int i = 0;
	for (string tname:vecUnionchildname)
	{
		if (m_holeidAndmodel[tname].ID!=0&&m_holeidAndmodel[tname].tModel!=nullptr)
		{
			EditElementHandle eeh(m_holeidAndmodel[tname].ID, m_holeidAndmodel[tname].tModel);
			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
				eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh,tinfo);

				DPoint3d minP = { 0 }, maxP = { 0 };
				mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
				if (i==0)
				{
					ptmin = minP;
					ptmax = maxP;
					i++;
				}
				else
				{
					ptmin.x = (ptmin.x < minP.x) ? ptmin.x : minP.x;
					ptmin.y = (ptmin.y < minP.y) ? ptmin.y : minP.y;
					ptmin.z = (ptmin.z < minP.z) ? ptmin.z : minP.z;

					ptmax.x = (ptmax.x > maxP.x) ? ptmax.x : maxP.x;
					ptmax.y = (ptmax.y > maxP.y) ? ptmax.y : maxP.y;
					ptmax.z = (ptmax.z > maxP.z) ? ptmax.z : maxP.z;
				}

				
			}

		}
	}
	if (ptmin.IsEqual(ptmax))
	{
		holesize = 0;
		return;
	}
	DRange3d range;
	range.high = ptmax;
	range.low = ptmin;

	holesize = (range.XLength() > range.ZLength()) ? range.XLength() : range.ZLength();
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	holesize = holesize / uor_per_mm;
	if (eehhole!=nullptr)
	{
		DPoint3d ptCenter = range.low;
		ptCenter.x = ptCenter.x + range.XLength() / 2;
		ptCenter.y = ptCenter.y + range.YLength() / 2;
		ptCenter.z = ptCenter.z + range.ZLength() / 2;
		DPoint3dCR ptSize = { range.XLength(), range.YLength(),range.ZLength() };

		DgnBoxDetail dgnBoxDetail = DgnBoxDetail::InitFromCenterAndSize(ptCenter, ptSize, true);
		ISolidPrimitivePtr solidPtr = ISolidPrimitive::CreateDgnBox(dgnBoxDetail);
		if (SUCCESS != DraftingElementSchema::ToElement(*eehhole, *solidPtr, NULL, *ACTIVEMODEL))
		{
			holesize = 0;
			return;
		}
		trans.InverseOf(trans);
		TransformInfo inversInfo(trans);
		eehhole->GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(*eehhole, inversInfo);
	}

}
//void HoleRFRebarAssembly::GetUnionHoleeehAndSize(EditElementHandle* eehhole, double& holesize,
//	vector<HoleRebarInfo::StructualInfo>& vecStructualinfo, Transform trans, string UnionName,
//	std::map<std::string, IDandModelref>& m_holeidAndmodel)
//{
//	vector<string> vecUnionchildname;
//	for (HoleRebarInfo::StructualInfo tmpinfo : vecStructualinfo)
//	{
//		if (tmpinfo.isUnionChild)
//		{
//			string uname(tmpinfo.Uname);
//			if (uname == UnionName)
//			{
//				string hname(tmpinfo.Hname);
//				vecUnionchildname.push_back(hname);
//			}
//		}
//
//
//	}
//	TransformInfo tinfo(trans);
//	DPoint3d ptmin = { 0 }, ptmax = { 0 };
//	int i = 0;
//	for (string tname : vecUnionchildname)
//	{
//		if (m_holeidAndmodel[tname].ID != 0 && m_holeidAndmodel[tname].tModel != nullptr)
//		{
//			EditElementHandle eeh(m_holeidAndmodel[tname].ID, m_holeidAndmodel[tname].tModel);
//			ISolidKernelEntityPtr entityPtr;
//			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
//			{
//				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
//				ElementCopyContext copier2(ACTIVEMODEL);
//				copier2.SetSourceModelRef(eeh.GetModelRef());
//				copier2.SetTransformToDestination(true);
//				copier2.SetWriteElements(false);
//				copier2.DoCopy(eeh);
//				eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, tinfo);
//
//				DPoint3d minP = { 0 }, maxP = { 0 };
//				mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
//				if (i == 0)
//				{
//					ptmin = minP;
//					ptmax = maxP;
//					i++;
//				}
//				else
//				{
//					ptmin.x = (ptmin.x < minP.x) ? ptmin.x : minP.x;
//					ptmin.y = (ptmin.y < minP.y) ? ptmin.y : minP.y;
//					ptmin.z = (ptmin.z < minP.z) ? ptmin.z : minP.z;
//
//					ptmax.x = (ptmax.x > maxP.x) ? ptmax.x : maxP.x;
//					ptmax.y = (ptmax.y > maxP.y) ? ptmax.y : maxP.y;
//					ptmax.z = (ptmax.z > maxP.z) ? ptmax.z : maxP.z;
//				}
//
//
//			}
//
//		}
//	}
//	if (ptmin.IsEqual(ptmax))
//	{
//		holesize = 0;
//		return;
//	}
//	DRange3d range;
//	range.high = ptmax;
//	range.low = ptmin;
//
//	holesize = (range.XLength() > range.ZLength()) ? range.XLength() : range.ZLength();
//	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
//	holesize = holesize / uor_per_mm;
//	if (eehhole != nullptr)
//	{
//		DPoint3d ptCenter = range.low;
//		ptCenter.x = ptCenter.x + range.XLength() / 2;
//		ptCenter.y = ptCenter.y + range.YLength() / 2;
//		ptCenter.z = ptCenter.z + range.ZLength() / 2;
//		DPoint3dCR ptSize = { range.XLength(), range.YLength(),range.ZLength() };
//
//		DgnBoxDetail dgnBoxDetail = DgnBoxDetail::InitFromCenterAndSize(ptCenter, ptSize, true);
//		ISolidPrimitivePtr solidPtr = ISolidPrimitive::CreateDgnBox(dgnBoxDetail);
//		if (SUCCESS != DraftingElementSchema::ToElement(*eehhole, *solidPtr, NULL, *ACTIVEMODEL))
//		{
//			holesize = 0;
//			return;
//		}
//		trans.InverseOf(trans);
//		TransformInfo inversInfo(trans);
//		eehhole->GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(*eehhole, inversInfo);
//	}
//
//}
void HoleRFRebarAssembly::GetUnionHoleeeh(EditElementHandle* eehhole,vector<string>& vecUnionchildname, double& holesize, Transform trans,
	std::map<std::string, IDandModelref>& m_holeidAndmodel)
{
	if (eehhole==nullptr)
	{
		return;
	}
	TransformInfo tinfo(trans);
	DPoint3d ptmin = { 0 }, ptmax = { 0 };
	int i = 0;
	for (string tname : vecUnionchildname)
	{
		if (m_holeidAndmodel[tname].ID != 0 && m_holeidAndmodel[tname].tModel != nullptr)
		{
			EditElementHandle eeh(m_holeidAndmodel[tname].ID, m_holeidAndmodel[tname].tModel);
			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
				eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, tinfo);

				DPoint3d minP = { 0 }, maxP = { 0 };
				mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
				if (i == 0)
				{
					ptmin = minP;
					ptmax = maxP;
					i++;
				}
				else
				{
					ptmin.x = (ptmin.x < minP.x) ? ptmin.x : minP.x;
					ptmin.y = (ptmin.y < minP.y) ? ptmin.y : minP.y;
					ptmin.z = (ptmin.z < minP.z) ? ptmin.z : minP.z;

					ptmax.x = (ptmax.x > maxP.x) ? ptmax.x : maxP.x;
					ptmax.y = (ptmax.y > maxP.y) ? ptmax.y : maxP.y;
					ptmax.z = (ptmax.z > maxP.z) ? ptmax.z : maxP.z;
				}


			}

		}
	}
	if (ptmin.IsEqual(ptmax))
	{
		holesize = 0;
		return;
	}
	DRange3d range;
	range.high = ptmax;
	range.low = ptmin;

	holesize = (range.XLength() > range.ZLength()) ? range.XLength() : range.ZLength();
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	holesize = holesize / uor_per_mm;
	if (eehhole != nullptr)
	{
		DPoint3d ptCenter = range.low;
		ptCenter.x = ptCenter.x + range.XLength() / 2;
		ptCenter.y = ptCenter.y + range.YLength() / 2;
		ptCenter.z = ptCenter.z + range.ZLength() / 2;
		DPoint3dCR ptSize = { range.XLength(), range.YLength(),range.ZLength() };

		DgnBoxDetail dgnBoxDetail = DgnBoxDetail::InitFromCenterAndSize(ptCenter, ptSize, true);
		ISolidPrimitivePtr solidPtr = ISolidPrimitive::CreateDgnBox(dgnBoxDetail);
		if (SUCCESS != DraftingElementSchema::ToElement(*eehhole, *solidPtr, NULL, *ACTIVEMODEL))
		{
			holesize = 0;
			return;
		}
		trans.InverseOf(trans);
		TransformInfo inversInfo(trans);
		eehhole->GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(*eehhole, inversInfo);
	}
}
bool HoleRFRebarAssembly::IsSmartSmartFeature(EditElementHandle& eeh)
{
	SmartFeatureNodePtr pFeatureNode;
	if (SmartFeatureElement::IsSmartFeature(eeh))
	{
		return true;
	}
	else
	{
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			if (SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef()) == SUCCESS)
			{
				eeh.AddToModel();
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}

		}
		return true;
	}
}

void HoleRFRebarAssembly::SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas)
{
	int i = 0;
	m_vecDirSize.resize(wallRebarDatas.size());
	m_vecRebarLevel.resize(wallRebarDatas.size());
	for (ConcreteRebar rebwall:wallRebarDatas)
	{
		m_vecDirSize[i] = rebwall.rebarSize;
		m_vecRebarLevel[i] = rebwall.datachange;
		i++;
	}
	
}


HoleSTRebarAssembly::HoleSTRebarAssembly(ElementId id, DgnModelRefP modelRef) :
	RebarAssembly(id, modelRef)
{
	m_holedlg = nullptr;
	m_FrontdiameterSide = 0.0;
	Init();
}

void HoleSTRebarAssembly::Init()
{

}

void HoleSTRebarAssembly::ClearData()
{

	m_Trans.InitIdentity();
	m_vecDirSize.clear();
	m_vecRebarLevel.clear();
	m_LayerRebars.clear();
	m_LayerTwinRebars.clear();
	m_rebarPts.clear();
	m_vecReinF.clear();
	m_holeidAndmodel.clear();
	m_LayerRebars.clear();
	m_holedlg = nullptr;
	isfloor = false;
}

bool HoleSTRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	if (m_vecFrontPts.size() == 0)
	{
		return false;
	}
	m_vecSetId.resize(m_vecDirSize.size());
	for (size_t i = 0; i < m_vecSetId.size(); i++)
	{
		m_vecSetId[i] = 0;
	}
	SetLayerRebars();
	if (m_LayerRebars.size() < 2)
	{
		return false;
	}
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	vector<RebarPoint> frontRpts;
	vector<RebarPoint> backRpts;
	frontRpts.insert(frontRpts.begin(), m_LayerRebars.begin()->second.begin(), m_LayerRebars.begin()->second.end());
	backRpts.insert(backRpts.begin(), m_LayerRebars.rbegin()->second.begin(), m_LayerRebars.rbegin()->second.end());
	double Wthickness;
	if (!isfloor)
	{
		DPoint3d ptStart = m_vecFrontPts[0];
		DPoint3d ptEnd = m_vecFrontPts[1];

		CVector3D  xVec(ptStart, ptEnd);

		CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
		CVector3D  xVecNew(ptStart, ptEnd);
		BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
		Transform trans;
		placement.AssignTo(trans);
		trans.InverseOf(trans);
		SetTrans(trans);
		
		TransFromRebarPts(frontRpts);
		TransFromRebarPts(backRpts);
	    Wthickness = abs(backRpts.at(0).ptstr.y - frontRpts.at(0).ptstr.y) + g_wallRebarInfo.concrete.postiveCover*uor_per_mm + g_wallRebarInfo.concrete.reverseCover*uor_per_mm;

	}
	else //如果是板件时，直接算高度差再加上保护层就是厚度
	{
		Wthickness = abs(backRpts.at(0).ptstr.z - frontRpts.at(0).ptstr.z) + g_wallRebarInfo.concrete.postiveCover*uor_per_mm + g_wallRebarInfo.concrete.reverseCover*uor_per_mm;
	}
	EditElementHandle eeh(GetSelectedElement(), GetSelectedModel());
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(eeh, Eleeh, Holeehs);
	SetSelectedElement(Eleeh.GetElementId());
	NewRebarAssembly(modelRef);
	SetSelectedElement(eeh.GetElementId());
	RebarSetTagArray rsetTags;
	int tagID = 1;
	

	for (int j = 0; j < m_vecReinF.size(); j++)
	{
		HoleRebarInfo::ReinForcingInfo tmpinfo;
		tmpinfo = m_vecReinF.at(j);
		m_FrontdiameterSide = 0.0;
		//if (tmpinfo.isUnionChild)//联合孔洞
		//{
		//	continue;
		//}
		if (!tmpinfo.v1 && !tmpinfo.v2 && !tmpinfo.h3 && !tmpinfo.h4)
		{
			continue;
		}
		CalculateTransByFrontPts(tmpinfo);
		TransformInfo transinfo(GetTrans());
		if (tmpinfo.isUnionHole)
		{
			string UnionName(tmpinfo.Hname);
			vector<string> vecUnionchildname;
			for (HoleRebarInfo::ReinForcingInfo tmpinfo : m_vecReinF)
			{
				if (tmpinfo.isUnionChild)
				{
					string tuname(tmpinfo.Uname);
					if (tuname.find(UnionName) != string::npos)
					{
						string hname(tmpinfo.Hname);
						vecUnionchildname.push_back(hname);
					}
				}
			}
			EditElementHandle eehhole;
			for (string tname : vecUnionchildname)//计算合并孔洞中的一个子孔洞位置
			{
				if (m_holeidAndmodel[tname].ID != 0 && m_holeidAndmodel[tname].tModel != nullptr)
				{
					EditElementHandle eeh(m_holeidAndmodel[tname].ID, m_holeidAndmodel[tname].tModel);
					eehhole.Duplicate(eeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(eeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(eehhole);
					break;
				}
			}
			DPoint3d ptcenter = getCenterOfElmdescr(eehhole.GetElementDescrP());
			/*if (g_vecTwinBarData.size()> m_vecDirSize.size() / 2)
			{*/
			int theSec = 0;
			if (!isfloor)
			{
				for (int k = 0; k < m_vecFrontPts.size() - 1; k++)
				{
					DPoint3d tmpstr = m_vecFrontPts.at(k);
					DPoint3d tmpend = m_vecFrontPts.at(k + 1);
					vector<DPoint3d> intresectpts;
					tmpstr.z = tmpend.z = ptcenter.z;
					GetIntersectPointsWithHole(intresectpts, &eehhole, tmpstr, tmpend);
					if (intresectpts.size() > 0)
					{
						break;
					}
					if (k > 0)
					{
						DPoint3d tmpstrfront = m_vecFrontPts.at(0);
						DPoint3d tmpendfront = m_vecFrontPts.at(1);

						DPoint3d tmpvecfront = tmpendfront - tmpstrfront;
						tmpvecfront.z = 0;
						tmpvecfront.Normalize();
						DPoint3d nowvec = tmpend - tmpstr;
						nowvec.z = 0;
						nowvec.Normalize();

						double dotvalue = nowvec.DotProduct(tmpvecfront);
						if (abs(dotvalue) < 0.01)//垂直
						{
							continue;
						}

					}
					theSec++;
				}
			}
				for (int i = 0; i < m_vecDirSize.size() / 2; ++i)
				{
					if (i > 1)//先屏蔽多层钢筋处理
					{
						continue;
					}
					if (m_vecTwinBarData.size() != m_vecDirSize.size())
					{
						memset(&m_twinbarinfo, 0, sizeof(m_twinbarinfo));
					}
					else
					{
						m_twinbarinfo = m_vecTwinBarData[i];//当前层的并筋信息
					}

					int j = (int)(m_vecDirSize.size() - 1 - i);
					if (j < 0)
					{
						break;
					}
					RebarSetTag* tag = NULL;
					// double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;

					PopvecSetId().at(i) = i;
					std::map<int, vector<RebarPoint>> tmpPointsi;
					for (RebarPoint tmppt : m_LayerRebars[i])//筛选掉和孔洞没有交的主筋线，主要针对多段墙时
					{
						tmpPointsi[tmppt.sec].push_back(tmppt);
					}
					std::map<int, vector<RebarPoint>> tmpPointsj;
					for (RebarPoint tmppt : m_LayerRebars[j])//筛选掉和孔洞没有交的主筋线，主要针对多段墙时
					{
						tmpPointsj[tmppt.sec].push_back(tmppt);
					}
					RebarSetTag* tagS = new RebarSetTag();
					tag = MakeRebars(Wthickness, tmpinfo, vecUnionchildname, tmpPointsi[theSec], tmpPointsj[theSec], PopvecSetId().at(i), GetvecDirSize().at(i), ACTIVEMODEL, tagS);
					if (NULL != tag && NULL != tagS)
					{
						tag->SetBarSetTag(tagID++);
						rsetTags.Add(tag);

						tagS->SetBarSetTag(tagID++);
						rsetTags.Add(tagS);
					}

				}			
		}
		else 
		{
			if (m_holeidAndmodel[tmpinfo.Hname].ID==0|| m_holeidAndmodel[tmpinfo.Hname].tModel==nullptr)
			{
				continue;
			}
			EditElementHandle eehHole(m_holeidAndmodel[tmpinfo.Hname].ID, m_holeidAndmodel[tmpinfo.Hname].tModel);
			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eehHole) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eehHole, *entityPtr, nullptr, *eehHole.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eehHole.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eehHole);
			}

			EditElementHandle eehHolePlus;//放大后的孔洞
			eehHolePlus.Duplicate(eehHole);
			PlusSideCover(eehHolePlus, g_wallRebarInfo.concrete.sideCover*uor_per_mm, GetTrans());
			DPoint3d ptcenter = getCenterOfElmdescr(eehHole.GetElementDescrP());
			int theSec = 0;
			if (!isfloor)
			{
				for (int k = 0; k < m_vecFrontPts.size() - 1; k++)
				{
					DPoint3d tmpstr = m_vecFrontPts.at(k);
					DPoint3d tmpend = m_vecFrontPts.at(k + 1);
					vector<DPoint3d> intresectpts;
					tmpstr.z = tmpend.z = ptcenter.z;
					GetIntersectPointsWithHole(intresectpts, &eehHole, tmpstr, tmpend);
					if (intresectpts.size() > 0)
					{
						break;
					}
					if (k > 0)
					{
						DPoint3d tmpstrfront = m_vecFrontPts.at(0);
						DPoint3d tmpendfront = m_vecFrontPts.at(1);

						DPoint3d tmpvecfront = tmpendfront - tmpstrfront;
						tmpvecfront.z = 0;
						tmpvecfront.Normalize();
						DPoint3d nowvec = tmpend - tmpstr;
						nowvec.z = 0;
						nowvec.Normalize();

						double dotvalue = nowvec.DotProduct(tmpvecfront);
						if (abs(dotvalue) < 0.01)//垂直
						{
							continue;
						}

					}
					theSec++;
				}
			}
			TransformInfo transinfo(GetTrans());
			eehHole.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHole, transinfo);
			eehHolePlus.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHolePlus, transinfo);
			//eehHole.AddToModel();
			//eehHolePlus.AddToModel();
			/*if (g_vecTwinBarData.size()> m_vecDirSize.size() / 2)
			{*/
			
				for (int i = 0; i < m_vecDirSize.size() / 2; ++i)
				{
					if (i>1)//先屏蔽多层钢筋处理
					{
						continue;
					}
					if (m_vecTwinBarData.size() != m_vecDirSize.size())
					{
						memset(&m_twinbarinfo, 0, sizeof(m_twinbarinfo));
					}
					else
					{
						m_twinbarinfo = m_vecTwinBarData[i];//当前层的并筋信息
					}
					int j = (int)(m_vecDirSize.size() - 1 - i);
					if (j < 0)
					{
						break;
					}
					RebarSetTag* tag = NULL;
					// double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;
					PopvecSetId().at(i) = i;
					std::map<int, vector<RebarPoint>> tmpPointsi;
					for (RebarPoint tmppt : m_LayerRebars[i])//筛选掉和孔洞没有交的主筋线，主要针对多段墙时
					{
						tmpPointsi[tmppt.sec].push_back(tmppt);
					}
					std::map<int, vector<RebarPoint>> tmpPointsj;
					for (RebarPoint tmppt : m_LayerRebars[j])//筛选掉和孔洞没有交的主筋线，主要针对多段墙时
					{
						tmpPointsj[tmppt.sec].push_back(tmppt);
					}
					RebarSetTag* tagS = new RebarSetTag();
					tag = MakeRebars(Wthickness, tmpinfo, eehHole, eehHolePlus, tmpPointsi[theSec], tmpPointsj[theSec], PopvecSetId().at(i), GetvecDirSize().at(i), ACTIVEMODEL, tagS);
					if (NULL != tag && NULL != tagS)
					{
						tag->SetBarSetTag(tagID++);
						rsetTags.Add(tag);

						tagS->SetBarSetTag(tagID++);
						rsetTags.Add(tagS);
					}

					//		}
				}
			//}
			
		}

		
	}






	//end
	/*AddRebarSets(rsetTags);
	RebarSets rebar_sets;
	GetRebarSets(rebar_sets, ACTIVEMODEL);
	return true;*/
	//return true;
	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;

}
void HoleSTRebarAssembly::SetLayerRebars()
{
	m_LayerRebars.clear();
	m_LayerTwinRebars.clear();
	int numFront = 0;
	int numBack = 0;
	int numMid = 0;
	//统计前，中，后层数
	for (RebarPoint pt : m_rebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			if (pt.Layer > numFront)
			{
				numFront = pt.Layer;
			}
		}
		else if (pt.DataExchange == 1)//中间
		{
			if (pt.Layer > numMid)
			{
				numMid = pt.Layer;
			}
		}
		else //背面 
		{
			if (pt.Layer > numBack)
			{
				numBack = pt.Layer;
			}
		}
	}
	for (RebarPoint pt : m_rebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			m_LayerRebars[pt.Layer - 1].push_back(pt);
		}
		else if (pt.DataExchange == 1)//中间
		{
			m_LayerRebars[pt.Layer + numFront - 1].push_back(pt);
		}
		else //背面 
		{
			m_LayerRebars[numFront + numMid + numBack - pt.Layer].push_back(pt);
		}
	}
	numFront = 0;
	numBack = 0;
	numMid = 0;
	//统计前，中，后层数
	for (RebarPoint pt : m_twinrebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			if (pt.Layer > numFront)
			{
				numFront = pt.Layer;
			}
		}
		else if (pt.DataExchange == 1)//中间
		{
			if (pt.Layer > numMid)
			{
				numMid = pt.Layer;
			}
		}
		else //背面 
		{
			if (pt.Layer > numBack)
			{
				numBack = pt.Layer;
			}
		}
	}
	for (RebarPoint pt : m_twinrebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			m_LayerTwinRebars[pt.Layer - 1].push_back(pt);
		}
		else if (pt.DataExchange == 1)//中间
		{
			m_LayerTwinRebars[pt.Layer + numFront - 1].push_back(pt);
		}
		else //背面 
		{
			m_LayerTwinRebars[numFront + numMid + numBack - pt.Layer].push_back(pt);
		}
	}
	
}
//将所有孔洞和钢筋点转换到ACS坐标系下
void  HoleSTRebarAssembly::TransFromRebarPts(vector<RebarPoint>&  rebarPts)
{
	TransformInfo transinfo(GetTrans());
	for (int i = 0; i < rebarPts.size(); i++)
	{
		DPoint3d ptstr = rebarPts.at(i).ptstr;
		DPoint3d ptend = rebarPts.at(i).ptend;
		EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
		//eeh.AddToModel();
		if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
		{
			rebarPts.at(i).ptstr = ptstr;
			rebarPts.at(i).ptend = ptend;
		}

	}

}
void HoleSTRebarAssembly::CreateURebars(DPoint3d pt,double diameter,double diameterU,double L0Lenth,double distance,double uor_per_mm,
	                          TransformInfo transinfo,DPoint3d minP,DPoint3d maxP,DPoint3d ptstr,Direction dir,PosValue Pvalue,
	                           double bendLen,double bendRadius, RebarEndTypes endTypes, vector<PITRebarCurve>&   rebarCurvesNum,bool ishaveTwinBar)
{
	
	DPoint3d pts[4];
	if (dir== Direction::Left|| dir == Direction::Right)
	{
		if (pt.x <Pvalue.minx + 1.0 ||pt.x>Pvalue.maxx-1.0)
		{
			return;
		}
		int Neg = 1;
		if (dir== Direction::Left)
		{
			Neg = -1;
		}

		if (ishaveTwinBar)
		{
			pts[0].z = ptstr.z + (diameter / 2 + diameterU / 2) +2*diameter;
		}
		else
		{
			pts[0].z = ptstr.z - (diameter / 2 + diameterU / 2);
		}
		pts[0].y = ptstr.y + (diameterU/2 - diameter/2);
		pts[0].x = pt.x + L0Lenth * uor_per_mm*Neg + diameterU / 2 * Neg;

		pts[1] = pts[0];
		pts[1].x = pts[1].x - L0Lenth * uor_per_mm*Neg;

		pts[2] = pts[1];
		pts[2].y = pts[2].y + distance - diameterU + diameter;

		pts[3] = pts[2];
		pts[3].x = pts[3].x + L0Lenth * uor_per_mm*Neg;


		if (pts[0].x<Pvalue.minx&&dir==Direction::Left)
		{
			pts[0].x = Pvalue.minx;
			pts[3].x = Pvalue.minx;
		}
		if (pts[0].x > Pvalue.maxx&&dir == Direction::Right)
		{
			pts[0].x = Pvalue.maxx;
			pts[3].x = Pvalue.maxx;
		}

		EditElementHandle eeh2;
		LineStringHandler::CreateLineStringElement(eeh2, nullptr, pts, 4, true, *ACTIVEMODEL);
		eeh2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh2, transinfo);
		
		vector<DPoint3d> dpts;
		ExtractLineStringPoint(eeh2.GetElementDescrP(), dpts);
		if (dpts.size()!=4)
		{
			return;
		}
		CalculateIntersetPtWithHolesWithRebarCuve(dpts[0], dpts[1],dpts[1], m_useHoleehs);
		CalculateIntersetPtWithHolesWithRebarCuve(dpts[2], dpts[3], dpts[2], m_useHoleehs);

		vector<CPoint3D> cpts;
		ExchangePoints(dpts, cpts);

		PITRebarCurve pitcurve;
		pitcurve.makeURebarCurve(cpts, bendRadius);
		rebarCurvesNum.push_back(pitcurve);
		//eeh2.AddToModel();
		/*	RebarCurve     rebarCurves;
			makeRebarCurve(rebarCurves, bendRadius, bendLen, endTypes, ptstr, ptend);
			rebarCurvesNum.push_back(rebarCurves);*/
	}
	else 
	{
		if (pt.z <Pvalue.minz + 1.0 || pt.z>Pvalue.maxz - 1.0)
		{
			return;
		}
		int twinNeg = 1;
		if (ishaveTwinBar)
		{
			twinNeg = -1;
		}
		int Neg = 1;
		if (dir == Direction::Down)
		{
			Neg = -1;
		}
		//孔洞上边U形筋生成
		pts[0].x = ptstr.x + (diameter/2+diameterU/2)*twinNeg;
		pts[0].y =ptstr.y + (diameterU / 2 - diameter / 2);
		pts[0].z = pt.z + L0Lenth*uor_per_mm*Neg + diameterU / 2 * Neg;

		pts[1] = pts[0];
		pts[1].z = pts[1].z - L0Lenth*uor_per_mm*Neg;

		pts[2] = pts[1];
		pts[2].y = pts[2].y + distance - diameterU + diameter;

		pts[3] = pts[2];
		pts[3].z = pts[3].z + L0Lenth*uor_per_mm*Neg;
		if (pts[0].z < Pvalue.minz&&dir == Direction::Down)
		{
			pts[0].z = Pvalue.minz;
			pts[3].z = Pvalue.minz;
		}
		if (pts[0].z > Pvalue.maxz&&dir == Direction::Up)
		{
			pts[0].z = Pvalue.maxz;
			pts[3].z = Pvalue.maxz;
		}
		EditElementHandle eeh2;
		LineStringHandler::CreateLineStringElement(eeh2, nullptr, pts, 4, true, *ACTIVEMODEL);
		eeh2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh2, transinfo);
		
		vector<DPoint3d> dpts;
		ExtractLineStringPoint(eeh2.GetElementDescrP(), dpts);
		if (dpts.size() != 4)
		{
			return;
		}
		CalculateIntersetPtWithHolesWithRebarCuve(dpts[0], dpts[1], dpts[1], m_useHoleehs);
		CalculateIntersetPtWithHolesWithRebarCuve(dpts[2], dpts[3], dpts[2], m_useHoleehs);
		vector<CPoint3D> cpts;
		ExchangePoints(dpts, cpts);

		PITRebarCurve pitcurve;
		pitcurve.makeURebarCurve(cpts, bendRadius);
		rebarCurvesNum.push_back(pitcurve);
		//eeh2.AddToModel();
	}
	
}
void HoleSTRebarAssembly::CreateSideRebars(vector<DPoint3d>& pts,int sideNum,double sideSpacing,double LaLenth,
	TransformInfo transinfo, DPoint3d minP, DPoint3d maxP, double diameter, double diameterSide, double uor_per_mm, Direction dir, PosValue Pvalue,
	double bendLen, double bendRadius, RebarEndTypes endTypes, vector<PITRebarCurve>&   rebarCurvesNum)
{
	double frontDiam = 0;
	if (m_FrontdiameterSide!=0)
	{
		diameterSide = (diameterSide > m_FrontdiameterSide) ? diameterSide : m_FrontdiameterSide;
		frontDiam = diameterSide;
	}
	
	if (dir==Direction::Left||dir==Direction::Right)
	{
		
		double originX;
		double minX, maxX;
		int i = 0; int Neg = 1;
		for (DPoint3d pt:pts)
		{
			if (i==0)
			{
				minX = pt.x;
				maxX = pt.x;
				i++;
			}
			else 
			{
				if (minX > pt.x)
				    minX = pt.x;
				if (maxX<pt.x)
				{
					maxX = pt.x;
				}
			}
		}
		if (minX <Pvalue.minx + 1.0 || maxX>Pvalue.maxx - 1.0)
		{
			return;
		}
		if (dir==Direction::Left)
		{
			originX = minX;		
			Neg = -1;
		}
		else
		{
			originX = maxX;
		}
		
		for (int n = 1; n < sideNum + 1; n++)
		{
			double tmpSpacing = n * sideSpacing;
			DPoint3d tmpstr, tmpend;
			tmpstr.z = minP.z - LaLenth * uor_per_mm;
			tmpstr.x = originX + diameter*Neg  + diameterSide / 2 * Neg;
			if (m_FrontdiameterSide != 0)
			{
				if (sideNum>1)
				{
					tmpstr.y = pts[0].y + tmpSpacing -m_Maindiameter/2-m_FrontMaindiameter/2 + (frontDiam +diameterSide)/2
						+ (m_FrontMaindiameter + m_Maindiameter)*n / (sideNum + 1);
				}
				else
				{
					tmpstr.y = pts[0].y + tmpSpacing + frontDiam / 2 + diameterSide / 2;
				}
				
			}
			else
			{
				tmpstr.y = pts[0].y + tmpSpacing;
			}		
			tmpend = tmpstr;
			tmpend.z = maxP.z + LaLenth * uor_per_mm;
			if (tmpstr.z < Pvalue.minz)
			{
				tmpstr.z = Pvalue.minz;
			}
			if (tmpend.z > Pvalue.maxz)
			{
				tmpend.z = Pvalue.maxz;
			}
			EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(tmpstr, tmpend), true, *ACTIVEMODEL);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			mdlElmdscr_extractEndPoints(&tmpstr, nullptr, &tmpend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef());

			DPoint3d midPos = tmpstr;
			midPos.SumOf(midPos, tmpend, 0.5);
			CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_useHoleehs);
			midPos = tmpstr;
			midPos.Add(tmpend);
			midPos.Scale(0.5);
			CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_useHoleehs);
			PITRebarCurve pitcurve;
			pitcurve.makeRebarCurve(bendRadius,bendLen,endTypes,CPoint3D(tmpstr),CPoint3D(tmpend));
			rebarCurvesNum.push_back(pitcurve);
		}
	}
	else
	{
		double originZ;
		double minZ, maxZ;
		int i = 0; int Neg = 1;
		for (DPoint3d pt : pts)
		{
			if (i == 0)
			{
				minZ = pt.z;
				maxZ = pt.z;
				i++;
			}
			else
			{
				if (minZ > pt.z)
					minZ = pt.z;
				if (maxZ < pt.z)
				{
					maxZ = pt.z;
				}
			}
		}
		if (minZ <Pvalue.minz + 1.0 || maxZ>Pvalue.maxz - 1.0)
		{
			return;
		}
		if (dir == Direction::Down)
		{
			originZ = minZ;
			Neg = -1;
		}
		else
		{
			originZ = maxZ;
		}
		for (int n = 1; n < sideNum + 1; n++)
		{
			double tmpSpacing = n * sideSpacing;
			DPoint3d tmpstr, tmpend;
			tmpstr.x = minP.x - LaLenth * uor_per_mm;
			tmpstr.z = originZ + diameter * Neg + diameterSide / 2 * Neg;
			if (m_FrontdiameterSide !=0)
			{

				if (sideNum > 1)
				{
					tmpstr.y = pts[0].y + tmpSpacing - m_Maindiameter / 2 - m_FrontMaindiameter / 2 + (frontDiam + diameterSide) / 2
						+ (m_FrontMaindiameter + m_Maindiameter)*n / (sideNum + 1);
				}
				else
				{
					tmpstr.y = pts[0].y + tmpSpacing + frontDiam / 2 + diameterSide / 2;
				}
			}
			else
			{
				tmpstr.y = pts[0].y + tmpSpacing;
			}
			

			tmpend = tmpstr;
			tmpend.x = maxP.x + LaLenth * uor_per_mm;
			if (tmpstr.x < Pvalue.minx)
			{
				tmpstr.x = Pvalue.minx;
			}
			if (tmpend.x > Pvalue.maxx)
			{
				tmpend.x = Pvalue.maxx;
			}
			EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(tmpstr, tmpend), true, *ACTIVEMODEL);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			mdlElmdscr_extractEndPoints(&tmpstr, nullptr, &tmpend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef());
			DPoint3d midPos = tmpstr;
			midPos.SumOf(midPos, tmpend, 0.5);
			CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_useHoleehs);
			midPos = tmpstr;
			midPos.Add(tmpend);
			midPos.Scale(0.5);
			CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_useHoleehs);
			PITRebarCurve pitcurve;
			pitcurve.makeRebarCurve(bendRadius, bendLen, endTypes, CPoint3D(tmpstr), CPoint3D(tmpend));
			rebarCurvesNum.push_back(pitcurve);
		}
	}


	
}
RebarSetTag* HoleSTRebarAssembly::MakeRebars
(
	double Wthickness,
	HoleRebarInfo::ReinForcingInfo tmpinfo,
	vector<string>& holenames,
	vector<RebarPoint>&  rebarPtsF,
	vector<RebarPoint>&  rebarPtsB,
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	DgnModelRefP        modelRef,
	RebarSetTag*		tagS,
	bool                Holestrucal
)
{
	int layernow = 1;
	if (rebarPtsF.size()<1|| rebarPtsB.size()<1)
	{
		return nullptr;
	}
	vector<RebarPoint> tmprebarPtsF;
	vector<RebarPoint> tmprebarPtsB;
	if (!isfloor)
	{
		tmprebarPtsF.insert(tmprebarPtsF.begin(), rebarPtsF.begin(), rebarPtsF.end());
		tmprebarPtsB.insert(tmprebarPtsB.begin(), rebarPtsB.begin(), rebarPtsB.end());
	}
	else //板件时，第0层为最下面一层，所以需要互换
	{
		tmprebarPtsB.insert(tmprebarPtsB.begin(), rebarPtsF.begin(), rebarPtsF.end());
		tmprebarPtsF.insert(tmprebarPtsF.begin(), rebarPtsB.begin(), rebarPtsB.end());
	}
	

	TransFromRebarPts(tmprebarPtsF);
	TransFromRebarPts(tmprebarPtsB);
	double distance = abs(tmprebarPtsB.at(0).ptstr.y - tmprebarPtsF.at(0).ptstr.y);

	bool const isStirrup = false;
	ElementId tmpID = rebarSetId;
	rebarSetId = 0;
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	rebarSetId = 2;
	RebarSetP   rebarSetS = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSetS)
		return NULL;
	rebarSetS->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSetS->SetCallerId(GetCallerId());
	rebarSetS->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	switch (0)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeStart.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (0)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeEnd.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}
	//	}
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
	double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100

	
	BrString          sizeKeyTwin(m_twinbarinfo.rebarSize);
	sizeKeyTwin = sizeKeyTwin + "C";
	double Twindiameter = RebarCode::GetBarDiameter(sizeKeyTwin, modelRef);
	

	//计算U形钢筋直径和LO长度
	BrString          sizeKeyU;
	double diameterU, L0Lenth, bendRadiusU, bendLenU;
	double LaLenth, diameterSide, bendRadiusS, bendLenS;
	BrString          sizeKeyS;
	
	
	if (diameter / uor_per_mm > 25)
	{
		diameterU = RebarCode::GetBarDiameter("20C", modelRef);;
		L0Lenth = g_globalpara.m_laplenth["20C"];
		bendRadiusU = RebarCode::GetPinRadius("20C", modelRef, false);
		bendLenU = RebarCode::GetBendLength("20C", endTypeStart, modelRef);
		sizeKeyU = "20C";
	}
	else
	{
		diameterU = RebarCode::GetBarDiameter("16C", modelRef);;
		L0Lenth = g_globalpara.m_laplenth["16C"];
		bendRadiusU = RebarCode::GetPinRadius("16C", modelRef, false);
		bendLenU = RebarCode::GetBendLength("16C", endTypeStart, modelRef);
		sizeKeyU = "16C";
	}
	
	if (tmpinfo.Hsize > 300 && tmpinfo.Hsize < 500)
	{
		diameterSide = RebarCode::GetBarDiameter("20C", modelRef);;
		LaLenth = g_globalpara.m_alength["20C"];
		bendRadiusS = RebarCode::GetPinRadius("20C", modelRef, false);
		bendLenS = RebarCode::GetBendLength("20C", endTypeStart, modelRef);
		sizeKeyS = "20C";
	}
	else
	{
		diameterSide = diameter;
		LaLenth = g_globalpara.m_alength[(string)sizeKey];
		bendRadiusS = bendRadius;
		bendLenS = bendLen;
		sizeKeyS = sizeKey;
	}

	if (m_FrontdiameterSide > diameterSide)
	{
		int tmpSize = (int)(m_FrontdiameterSide / uor_per_mm);
		char tmpchar[256];
		sprintf(tmpchar, "%d", tmpSize);
		sizeKeyS = tmpchar;
	}
	
	

	//计算侧向构造筋直径和LA长度
	

	//计算侧向构造筋根数
	
	int sideNum = 0;//侧面构造筋根数
	Wthickness = (Wthickness + diameter)/ uor_per_mm;
	distance = distance / uor_per_mm;
	if (Wthickness<400)
	{
		sideNum = 0;
	}else if (Wthickness>=400&&Wthickness<700)
	{
		sideNum = 1;
	}else if (Wthickness>=700&&Wthickness<1000)
	{
		sideNum = 2;
	}else if (Wthickness>=1000&&Wthickness<=1200)
	{
		sideNum = 3;
	}
	else
	{
		sideNum = (int)(distance / 250);
	}

	double sideSpacing = (distance / (sideNum + 1))*uor_per_mm;


	vector<PITRebarCurve>     rebarCurvesNumU;
	vector<PITRebarCurve>     rebarCurvesNumS;
	vector<int>					producerebar;
	producerebar = Getproduce();
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
	if (tmprebarPtsF.size() > 0)
	{
		layernow = tmprebarPtsF[0].Layer;
		DPoint3d minP = { 0 }, maxP = { 0 };
		EditElementHandle eehHole, eehHolePlus;
		double hsize;
		HoleRFRebarAssembly::GetUnionHoleeeh(&eehHole, holenames, hsize, GetTrans(), m_holeidAndmodel);
		eehHolePlus.Duplicate(eehHole);
		PlusSideCover(eehHolePlus, g_wallRebarInfo.concrete.sideCover*uor_per_mm, GetTrans());

		TransformInfo transinfo1(GetTrans());
		eehHole.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHole, transinfo1);
		eehHolePlus.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHolePlus, transinfo1);
		mdlElmdscr_computeRange(&minP, &maxP, eehHolePlus.GetElementDescrP(), NULL);



		int minXID = -1; int maxXID = -1;
		GetRebarIDSIntesectWithHole(minXID, maxXID, eehHolePlus, tmprebarPtsF);
		PosValue Pvalue;
		if (minXID == -1 || maxXID == -1)
		{
			return false;
		}

		Transform inversMat = GetTrans();
		inversMat.InverseOf(inversMat);
		TransformInfo transinfo(inversMat);
		if (tmprebarPtsF[0].vecDir == 0)//X方向钢筋
		{
			Pvalue.minx = (tmprebarPtsF.begin()->ptstr.x < tmprebarPtsF.begin()->ptend.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.begin()->ptend.x;
			Pvalue.maxx = (tmprebarPtsF.begin()->ptstr.x > tmprebarPtsF.begin()->ptend.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.begin()->ptend.x;
			Pvalue.minz = (tmprebarPtsF.begin()->ptstr.z < tmprebarPtsF.rbegin()->ptstr.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.rbegin()->ptstr.z;
			Pvalue.maxz = (tmprebarPtsF.begin()->ptstr.z > tmprebarPtsF.rbegin()->ptstr.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.rbegin()->ptstr.z;
			Pvalue.minz = Pvalue.minz - diameter / 2;
			Pvalue.maxz = Pvalue.maxz + diameter / 2;
			vector<DPoint3d> Letfpts, Rightpts;
			int tmpi = 0;
			for (int i = 0;i< tmprebarPtsF.size();i++)
			{
				if (i<minXID||i>maxXID)
				{
					tmpi++;
					continue;
				}
				RebarPoint rebrptmin = tmprebarPtsF.at(i);
				bool ishavetwin = false;
				if (m_twinbarinfo.hasTwinbars)
				{
					if (tmpi%(m_twinbarinfo.interval+1)==0)
					{
						ishavetwin = true;
					}
				}
				

				if (rebrptmin.ptstr.z<minP.z || rebrptmin.ptstr.z >maxP.z)
				{		
					tmpi++;
					continue;
				}
				vector<DPoint3d> ItrPts;
				DPoint3d ptstr = rebrptmin.ptstr;
				DPoint3d ptend = rebrptmin.ptend;
				GetIntersectPointsWithHole(ItrPts, holenames, m_holeidAndmodel, ptstr, ptend, transinfo1, g_wallRebarInfo.concrete.sideCover*uor_per_mm);
				if (ItrPts.size() == 1)//只有一个交点
				{
					DPoint3d pt = ItrPts.at(0);
					if (pt.x > (minP.x + maxP.x)/2 && tmpinfo.h4)//交点在右边
					{
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Right, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinh4)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;							
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Right, Pvalue,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
						
						Rightpts.push_back(pt);
					}
					else if(tmpinfo.h3)//交点在左边
					{
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Left, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						Letfpts.push_back(pt);
						if (ishavetwin&&tmpinfo.have_twinh3)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Left, Pvalue,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
					
				}
				else if (ItrPts.size()==2)
				{
					DPoint3d ptleft, ptright;
					if (ItrPts[0].x>ItrPts[1].x)
					{
						ptleft = ItrPts[1];
						ptright = ItrPts[0];
					}
					else
					{
						ptleft = ItrPts[0];
						ptright = ItrPts[1];
					}
					Letfpts.push_back(ptleft);
					Rightpts.push_back(ptright);
					if (tmpinfo.h3)
					{
						if (producerebar[1])
						{
							CreateURebars(ptleft, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Left, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinh3)//画并筋的U形筋
						{
							DPoint3d twinPt = ptleft;
							DPoint3d tmpptstr = rebrptmin.ptstr;
							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Left, Pvalue,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
					if (tmpinfo.h4)
					{
						//孔洞右边U形筋生成
						if (producerebar[1])
						{
							CreateURebars(ptright, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Right, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinh4)//画并筋的U形筋
						{
							DPoint3d twinPt = ptright;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Right, Pvalue,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
			
				}
				tmpi++;
			}
			if (Letfpts.size()>0&&tmpinfo.h3)//左边的侧面构造筋
			{
				m_Maindiameter = diameter;
				if (producerebar[0])
				{
					CreateSideRebars(Letfpts, sideNum, sideSpacing, LaLenth, transinfo, minP, maxP, diameterU, diameterSide, uor_per_mm, Direction::Left, Pvalue
						, bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (Rightpts.size()>0&&tmpinfo.h4)
			{
				m_Maindiameter = diameter;
				if (producerebar[1])
				{
					CreateSideRebars(Rightpts, sideNum, sideSpacing, LaLenth, transinfo, minP, maxP, diameterU, diameterSide, uor_per_mm, Direction::Right, Pvalue
						, bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (m_FrontdiameterSide==0.0)
			{
				m_FrontMaindiameter = diameter;
				m_FrontdiameterSide = diameterSide;
			}
			
		}
		else//Z方向钢筋
		{
		Pvalue.minx = (tmprebarPtsF.begin()->ptstr.x < tmprebarPtsF.rbegin()->ptstr.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.rbegin()->ptstr.x;
		Pvalue.maxx = (tmprebarPtsF.begin()->ptstr.x > tmprebarPtsF.rbegin()->ptstr.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.rbegin()->ptstr.x;
		Pvalue.minz = (tmprebarPtsF.begin()->ptstr.z < tmprebarPtsF.begin()->ptend.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.begin()->ptend.z;
		Pvalue.maxz = (tmprebarPtsF.begin()->ptstr.z > tmprebarPtsF.begin()->ptend.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.begin()->ptend.z;
		Pvalue.minx = Pvalue.minx - diameter / 2;
		Pvalue.maxx = Pvalue.maxx + diameter / 2;
		    vector<DPoint3d> Uppts, Downpts;
			int tmpi = 0;
			 for (int i=0;i<tmprebarPtsF.size();i++)
			 {
				
				
				 if (i<minXID || i>maxXID)
				 {
					 tmpi++;
					 continue;
				 }
				 RebarPoint rebrptmin = tmprebarPtsF.at(i);
					if (rebrptmin.ptstr.x<minP.x || rebrptmin.ptstr.x >maxP.x)
					{
						tmpi++;
						continue;
					}
					bool ishavetwin = false;
					if (m_twinbarinfo.hasTwinbars)
					{
						if (tmpi % (m_twinbarinfo.interval + 1) == 0)
						{
							ishavetwin = true;
						}
					}
			
				vector<DPoint3d> ItrPts;
				DPoint3d ptstr = rebrptmin.ptstr;
				DPoint3d ptend = rebrptmin.ptend;
				GetIntersectPointsWithHole(ItrPts, holenames, m_holeidAndmodel, ptstr, ptend, transinfo1, g_wallRebarInfo.concrete.sideCover*uor_per_mm);
		
				if (ItrPts.size() == 1)//只有一个交点
				{
					DPoint3d pt = ItrPts.at(0);
					if (pt.z > (minP.z + maxP.z) / 2&&tmpinfo.v1)//交点在上边
					{
						Uppts.push_back(pt);
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Up, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv1)//画并筋的U形筋
						{
							
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Up, Pvalue,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
					else if(tmpinfo.v2)//交点在下边
					{
						Downpts.push_back(pt);
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Down, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv2)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Down, Pvalue,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}

					}

				}
				else if (ItrPts.size() == 2)
				{
					DPoint3d ptdown, ptup;
					if (ItrPts[0].z > ItrPts[1].z)
					{
						ptdown = ItrPts[1];
						ptup = ItrPts[0];
					}
					else
					{
						ptdown = ItrPts[0];
						ptup = ItrPts[1];
					}
					Downpts.push_back(ptdown);
					Uppts.push_back(ptup);
					if (tmpinfo.v1)
					{
						if (producerebar[1])
						{
							CreateURebars(ptup, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Up, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv1)//画并筋的U形筋
						{
							DPoint3d twinPt = ptup;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Up, Pvalue,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
					if (tmpinfo.v2)
					{
						if (producerebar[1])
						{
							CreateURebars(ptdown, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Down, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv2)//画并筋的U形筋
						{
							DPoint3d twinPt = ptdown;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Down, Pvalue,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
					
				}

				tmpi++;
			 }
			 if (Uppts.size() > 0&&tmpinfo.v1)//上边的侧面构造筋
			 {
				 m_Maindiameter = diameter;
				 if (producerebar[0])
				 {
					 CreateSideRebars(Uppts, sideNum, sideSpacing, LaLenth, transinfo, minP, maxP, diameterU, diameterSide, uor_per_mm, Direction::Up, Pvalue
						 , bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
				 }
			 }
			 if (Downpts.size() > 0&&tmpinfo.v2)
			 {
				 m_Maindiameter = diameter;
				 if (producerebar[0])
				 {
					 CreateSideRebars(Downpts, sideNum, sideSpacing, LaLenth, transinfo, minP, maxP, diameterU, diameterSide, uor_per_mm, Direction::Down, Pvalue
						 , bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
				 }
			 }
			 if (m_FrontdiameterSide == 0.0)
			 {
				 m_FrontMaindiameter = diameter;
				 m_FrontdiameterSide = diameterSide;
			 }
		}

	}



	RebarSymbology symb;
	{
		string str(sizeKeyU);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_CAVE_REBAR);
	}
	int	numRebar = (int)rebarCurvesNumU.size();
//	numRebar = numRebar + (int)rebarCurvesNumS.size();
	int j = 0;
	char nowLayerc[256];
	sprintf(nowLayerc, "%d", layernow);
	string nowLayerstring(nowLayerc);
	for (PITRebarCurve rebarCurve : rebarCurvesNumU)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);


		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKeyU);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameterU, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string strname = tmpinfo.Hname;
			string Stype = strname + "/HoleStructualfrontRebar" + "_URebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, nowLayerstring, 0, Stype, modelRef);
			//SetRebarLevelItemTypeValue(tmprebar,Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(200);
	setdata.SetAverageSpacing(200);
	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数
	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);


	int	numRebarS = (int)rebarCurvesNumS.size();
	int a = 0;
	for (PITRebarCurve rebarCurve : rebarCurvesNumS)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);

		RebarElementP rebarElement = rebarSet->AssignRebarElement(a, numRebarS, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKeyS);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameterSide, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string strname = tmpinfo.Hname;
			string Stype = strname + "/HoleStructualMidRebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, nowLayerstring, 0, Stype, modelRef);
			//SetRebarLevelItemTypeValue(tmprebar,Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		a++;
	}

	RebarSetData setdataS;
	setdataS.SetNumber(numRebarS);
	setdataS.SetNominalSpacing(200);
	setdataS.SetAverageSpacing(200);
	int retS = -1;
	retS = rebarSetS->FinishUpdate(setdataS, modelRef);	//返回的是钢筋条数

	tagS->SetRset(rebarSetS);
	tagS->SetIsStirrup(isStirrup);


	return tag;
}

void HoleSTRebarAssembly::CalculateUSizeAndSpacing(DgnModelRefP modelRef,HoleRebarInfo::ReinForcingInfo tmpinfo, BrStringCR  sizeKey,
	double distance,double Wthickness, bool flag)
{
	
	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
	double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100

		if (diameter / uor_per_mm > 25)
		{
			m_stdata.diameterU = RebarCode::GetBarDiameter("20C", modelRef);;
			m_stdata.L0Lenth = g_globalpara.m_laplenth["20C"];
			m_stdata.bendRadiusU = RebarCode::GetPinRadius("20C", modelRef, false);
			m_stdata.bendLenU = RebarCode::GetBendLength("20C", endTypeStart, modelRef);
			m_stdata.sizeKeyU = "20C";
		}
		else
		{
			m_stdata.diameterU = RebarCode::GetBarDiameter("16C", modelRef);;
			m_stdata.L0Lenth = g_globalpara.m_laplenth["16C"];
			m_stdata.bendRadiusU = RebarCode::GetPinRadius("16C", modelRef, false);
			m_stdata.bendLenU = RebarCode::GetBendLength("16C", endTypeStart, modelRef);
			m_stdata.sizeKeyU = "16C";
		}
		if (tmpinfo.Hsize > 300 && tmpinfo.Hsize < 500)
		{
			m_stdata.diameterSide = RebarCode::GetBarDiameter("20C", modelRef);;
			m_stdata.LaLenth = g_globalpara.m_alength["20C"];
			m_stdata.bendRadiusS = RebarCode::GetPinRadius("20C", modelRef, false);
			m_stdata.bendLenS = RebarCode::GetBendLength("20C", endTypeStart, modelRef);
			m_stdata.sizeKeyS = "20C";
		}
		else
		{
			m_stdata.diameterSide = diameter;
			m_stdata.LaLenth = g_globalpara.m_alength[(string)sizeKey];
			m_stdata.bendRadiusS = bendRadius;
			m_stdata.bendLenS = bendLen;
			m_stdata.sizeKeyS = sizeKey;
		}

		if (m_FrontdiameterSide > m_stdata.diameterSide)
		{
			int tmpSize = (int)(m_FrontdiameterSide / uor_per_mm);
			char tmpchar[256];
			sprintf(tmpchar, "%d", tmpSize);
			m_stdata.sizeKeyS = tmpchar;
		}
	
	
	

	//计算侧向构造筋根数

	m_stdata.sideNum = 0;//侧面构造筋根数
	Wthickness = (Wthickness + diameter) / uor_per_mm;
	distance = distance / uor_per_mm;
	if (Wthickness < 400)
	{
		m_stdata.sideNum = 0;
	}
	else if (Wthickness >= 400 && Wthickness < 700)
	{
		m_stdata.sideNum = 1;
	}
	else if (Wthickness >= 700 && Wthickness < 1000)
	{
		m_stdata.sideNum = 2;
	}
	else if (Wthickness >= 1000 && Wthickness <= 1200)
	{
		m_stdata.sideNum = 3;
	}
	else
	{
		m_stdata.sideNum = (int)(distance / 250);
	}

	m_stdata.sideSpacing = (distance / (m_stdata.sideNum + 1))*uor_per_mm;

}

void HoleSTRebarAssembly::CalculateMinMaxPos(PosValue& Pvalue, vector<RebarPoint>& tmprebarPtsF, vector<RebarPoint> tmprebarPtsB,double diameter,int dir)
{
	for (int i = 0; i < tmprebarPtsF.size(); i++)
	{
		if (i == 0)
		{
			Pvalue.minx = tmprebarPtsF.at(i).ptstr.x;
			Pvalue.maxx = tmprebarPtsF.at(i).ptend.x;
			Pvalue.minz = tmprebarPtsF.at(i).ptstr.z;
			Pvalue.maxz = tmprebarPtsF.at(i).ptend.z;
		}
		else
		{
			if (tmprebarPtsF.at(i).ptstr.x < Pvalue.minx)
			{
				Pvalue.minx = tmprebarPtsF.at(i).ptstr.x;
			}
			if (tmprebarPtsF.at(i).ptend.x < Pvalue.minx)
			{
				Pvalue.minx = tmprebarPtsF.at(i).ptend.x;
			}
			if (tmprebarPtsF.at(i).ptstr.x > Pvalue.maxx)
			{
				Pvalue.maxx = tmprebarPtsF.at(i).ptstr.x;
			}
			if (tmprebarPtsF.at(i).ptend.x > Pvalue.maxx)
			{
				Pvalue.maxx = tmprebarPtsF.at(i).ptend.x;
			}
			if (tmprebarPtsF.at(i).ptstr.z < Pvalue.minz)
			{
				Pvalue.minz = tmprebarPtsF.at(i).ptstr.z;
			}
			if (tmprebarPtsF.at(i).ptend.z < Pvalue.minz)
			{
				Pvalue.minz = tmprebarPtsF.at(i).ptend.z;
			}
			if (tmprebarPtsF.at(i).ptend.z > Pvalue.maxz)
			{
				Pvalue.maxz = tmprebarPtsF.at(i).ptend.z;
			}
			if (tmprebarPtsF.at(i).ptstr.z > Pvalue.maxz)
			{
				Pvalue.maxz = tmprebarPtsF.at(i).ptstr.z;
			}
		}

	}
	if (dir == 0)//横向钢筋
	{
		Pvalue.minz = Pvalue.minz - diameter / 2;
		Pvalue.maxz = Pvalue.maxz + diameter / 2;
	}
	else
	{
		Pvalue.minx = Pvalue.minx - diameter / 2;
		Pvalue.maxx = Pvalue.maxx + diameter / 2;
	}

	
}


RebarSetTag* HoleSTRebarAssembly::MakeRebars
(
	double Wthickness,
	HoleRebarInfo::ReinForcingInfo tmpinfo,
	EditElementHandle& eehHole,
	EditElementHandle& eehHolePlus,
	vector<RebarPoint>&  rebarPtsF,
	vector<RebarPoint>&  rebarPtsB,
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	DgnModelRefP        modelRef,
	RebarSetTag*		tagS,
	bool                Holestrucal
)
{
	rebarSetId = 2;
	RebarSetP   rebarSetS = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSetS)
		return NULL;
	rebarSetS->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSetS->SetCallerId(GetCallerId());
	rebarSetS->StartUpdate(modelRef);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	
	
	int layernow = 1;
	if (rebarPtsF.size() < 1 || rebarPtsB.size() < 1)
	{
		return nullptr;
	}
	vector<RebarPoint> tmprebarPtsF;
	vector<RebarPoint> tmprebarPtsB;
	if (!isfloor)
	{
		tmprebarPtsF.insert(tmprebarPtsF.begin(), rebarPtsF.begin(), rebarPtsF.end());
		tmprebarPtsB.insert(tmprebarPtsB.begin(), rebarPtsB.begin(), rebarPtsB.end());
	}
	else //板件时，第0层为最下面一层，所以需要互换
	{
		tmprebarPtsB.insert(tmprebarPtsB.begin(), rebarPtsF.begin(), rebarPtsF.end());
		tmprebarPtsF.insert(tmprebarPtsF.begin(), rebarPtsB.begin(), rebarPtsB.end());
	}
	TransFromRebarPts(tmprebarPtsF);
	TransFromRebarPts(tmprebarPtsB);
	double distance = abs(tmprebarPtsB.at(0).ptstr.y - tmprebarPtsF.at(0).ptstr.y);
	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	CalculateUSizeAndSpacing(modelRef, tmpinfo, sizeKey,distance, Wthickness, Holestrucal);

	bool const isStirrup = false;
	ElementId tmpID = rebarSetId;
	rebarSetId = 0;
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);
	BrString          sizeKeyTwin(m_twinbarinfo.rebarSize);
	sizeKeyTwin = sizeKeyTwin + "A";
	double Twindiameter = RebarCode::GetBarDiameter(sizeKeyTwin, modelRef);


	vector<PITRebarCurve>     rebarCurvesNumU;
	vector<PITRebarCurve>     rebarCurvesNumS;
	vector<int>producerebar;
	producerebar = Getproduce();
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
	if (tmprebarPtsF.size() > 0)
	{
		layernow = tmprebarPtsF[0].Layer;
		DPoint3d minP = { 0 }, maxP = { 0 };
		mdlElmdscr_computeRange(&minP, &maxP, eehHolePlus.GetElementDescrP(), NULL);
		int minXID = -1; int maxXID = -1;
		GetRebarIDSIntesectWithHole(minXID, maxXID, eehHolePlus, tmprebarPtsF);
		PosValue Pvalue;
		if (minXID == -1 || maxXID == -1)
		{
			return false;
		}

		Transform inversMat = GetTrans();
		inversMat.InverseOf(inversMat);
		TransformInfo transinfo(inversMat);
		if (tmprebarPtsF[0].vecDir == 0)//X方向钢筋
		{
			CalculateMinMaxPos(Pvalue, tmprebarPtsF, tmprebarPtsB, diameter, tmprebarPtsF[0].vecDir);
			vector<DPoint3d> Letfpts, Rightpts;
			int tmpi = 0;
			for (RebarPoint rebrptmin : tmprebarPtsF)
			{
				bool ishavetwin = false;
				if (m_twinbarinfo.hasTwinbars)
				{
					if (tmpi % (m_twinbarinfo.interval + 1) == 0)
					{
						ishavetwin = true;
					}
				}


				if (rebrptmin.ptstr.z<minP.z || rebrptmin.ptstr.z >maxP.z)
				{
					tmpi++;
					continue;
				}
				vector<DPoint3d> ItrPts;
				DPoint3d ptstr = rebrptmin.ptstr;
				DPoint3d ptend = rebrptmin.ptend;
				GetIntersectPointsWithHole(ItrPts, &eehHolePlus, ptstr, ptend);
				if (ItrPts.size() == 1)//只有一个交点
				{
					DPoint3d pt = ItrPts.at(0);
					if (pt.x > (minP.x + maxP.x) / 2 && tmpinfo.h4)//交点在右边
					{
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Right, Pvalue,
								m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, false);
						}
						if (ishavetwin&&tmpinfo.have_twinh4)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Right, Pvalue,
									m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, true);
							}
						}

						Rightpts.push_back(pt);
					}
					else if (tmpinfo.h3)//交点在左边
					{
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Left, Pvalue,
								m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, false);
						}
						Letfpts.push_back(pt);
						if (ishavetwin&&tmpinfo.have_twinh3)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Left, Pvalue,
									m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, true);
							}
						}
					}

				}
				else if (ItrPts.size() == 2)
				{
					DPoint3d ptleft, ptright;
					if (ItrPts[0].x > ItrPts[1].x)
					{
						ptleft = ItrPts[1];
						ptright = ItrPts[0];
					}
					else
					{
						ptleft = ItrPts[0];
						ptright = ItrPts[1];
					}
					Letfpts.push_back(ptleft);
					Rightpts.push_back(ptright);
					if (tmpinfo.h3)
					{
						if (producerebar[1])
						{
							CreateURebars(ptleft, diameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Left, Pvalue,
								m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, false);
						}
						if (ishavetwin&&tmpinfo.have_twinh3)//画并筋的U形筋
						{
							DPoint3d twinPt = ptleft;
							DPoint3d tmpptstr = rebrptmin.ptstr;
							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Left, Pvalue,
									m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
							}
						}
					}
					if (tmpinfo.h4)
					{
						//孔洞右边U形筋生成
						if (producerebar[1])
						{
							CreateURebars(ptright, diameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Right, Pvalue,
								m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, false);
						}
						if (ishavetwin&&tmpinfo.have_twinh4)//画并筋的U形筋
						{
							DPoint3d twinPt = ptright;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Right, Pvalue,
									m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
							}
						}
					}

				}
				tmpi++;
			}
			if (Letfpts.size() > 0 && tmpinfo.h3)//左边的侧面构造筋
			{
				m_Maindiameter = diameter;
				if (producerebar[0])
				{
					CreateSideRebars(Letfpts, m_stdata.sideNum, m_stdata.sideSpacing, m_stdata.LaLenth, transinfo, minP, maxP, m_stdata.diameterU, m_stdata.diameterSide, uor_per_mm, Direction::Left, Pvalue
						, m_stdata.bendLenS, m_stdata.bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (Rightpts.size() > 0 && tmpinfo.h4)
			{
				m_Maindiameter = diameter;
				if (producerebar[0])
				{
					CreateSideRebars(Rightpts, m_stdata.sideNum, m_stdata.sideSpacing, m_stdata.LaLenth, transinfo, minP, maxP, m_stdata.diameterU, m_stdata.diameterSide, uor_per_mm, Direction::Right, Pvalue
						, m_stdata.bendLenS, m_stdata.bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (m_FrontdiameterSide == 0.0)
			{
				m_FrontMaindiameter = diameter;
				m_FrontdiameterSide = m_stdata.diameterSide;
			}

		}
		else//Z方向钢筋
		{
		    CalculateMinMaxPos(Pvalue, tmprebarPtsF, tmprebarPtsB, diameter, tmprebarPtsF[0].vecDir);
			vector<DPoint3d> Uppts, Downpts;
			int tmpi = 0;
			for (RebarPoint rebrptmin : tmprebarPtsF)
			{

				if (rebrptmin.ptstr.x<minP.x || rebrptmin.ptstr.x >maxP.x)
				{
					tmpi++;
					continue;
				}
				bool ishavetwin = false;
				if (m_twinbarinfo.hasTwinbars)
				{
					if (tmpi % (m_twinbarinfo.interval + 1) == 0)
					{
						ishavetwin = true;
					}
				}

				vector<DPoint3d> ItrPts;
				DPoint3d ptstr = rebrptmin.ptstr;
				DPoint3d ptend = rebrptmin.ptend;
				GetIntersectPointsWithHole(ItrPts, &eehHolePlus, ptstr, ptend);

				if (ItrPts.size() == 1)//只有一个交点
				{
					DPoint3d pt = ItrPts.at(0);
					if (pt.z > (minP.z + maxP.z) / 2 && tmpinfo.v1)//交点在上边
					{
						Uppts.push_back(pt);
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Up, Pvalue,
								m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv1)//画并筋的U形筋
						{

							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Up, Pvalue,
									m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
					else if (tmpinfo.v2)//交点在下边
					{
						Downpts.push_back(pt);
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Down, Pvalue,
								m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv2)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Down, Pvalue,
									m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}

					}

				}
				else if (ItrPts.size() == 2)
				{
					DPoint3d ptdown, ptup;
					if (ItrPts[0].z > ItrPts[1].z)
					{
						ptdown = ItrPts[1];
						ptup = ItrPts[0];
					}
					else
					{
						ptdown = ItrPts[0];
						ptup = ItrPts[1];
					}
					Downpts.push_back(ptdown);
					Uppts.push_back(ptup);
					if (tmpinfo.v1)
					{
						if (producerebar[1])
						{
							CreateURebars(ptup, diameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Up, Pvalue,
								m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv1)//画并筋的U形筋
						{
							DPoint3d twinPt = ptup;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Up, Pvalue,
									m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
					if (tmpinfo.v2)
					{
						if (producerebar[1])
						{
							CreateURebars(ptdown, diameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Down, Pvalue,
								m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv2)//画并筋的U形筋
						{
							DPoint3d twinPt = ptdown;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, m_stdata.diameterU, m_stdata.L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Down, Pvalue,
									m_stdata.bendLenU, m_stdata.bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}

				}

				tmpi++;
			}
			if (Uppts.size() > 0 && tmpinfo.v1)//上边的侧面构造筋
			{
				m_Maindiameter = diameter;
				if (producerebar[0])
				{
					CreateSideRebars(Uppts, m_stdata.sideNum, m_stdata.sideSpacing, m_stdata.LaLenth, transinfo, minP, maxP, m_stdata.diameterU, m_stdata.diameterSide, uor_per_mm, Direction::Up, Pvalue
						, m_stdata.bendLenS, m_stdata.bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (Downpts.size() > 0 && tmpinfo.v2)
			{
				m_Maindiameter = diameter;
				if (producerebar[0])
				{
					CreateSideRebars(Downpts, m_stdata.sideNum, m_stdata.sideSpacing, m_stdata.LaLenth, transinfo, minP, maxP, m_stdata.diameterU, m_stdata.diameterSide, uor_per_mm, Direction::Down, Pvalue
						, m_stdata.bendLenS, m_stdata.bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (m_FrontdiameterSide == 0.0)
			{
				m_FrontMaindiameter = diameter;
				m_FrontdiameterSide = m_stdata.diameterSide;
			}
		}

	}



	RebarSymbology symb;
	{
		string str(m_stdata.sizeKeyU);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_CAVE_REBAR);
	}
	int	numRebar = (int)rebarCurvesNumU.size();
//	numRebar = numRebar + (int)rebarCurvesNumS.size();
	int j = 0;
	char nowLayerc[256];
	sprintf(nowLayerc, "%d", layernow);
	string nowLayerstring(nowLayerc);
	for (PITRebarCurve rebarCurve : rebarCurvesNumU)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);

		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)m_stdata.sizeKeyU);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, m_stdata.diameterU, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string strname = tmpinfo.Hname;
			string Stype = strname + "/HoleStructualfrontRebar" + "_URebar";//U型钢筋都为正面
			ElementRefP oldref = tmprebar.GetElementRef();
			int num = Gettypenum();
			SetRebarLevelItemTypeValue(tmprebar, nowLayerstring, num, Stype, modelRef);
			//SetRebarLevelItemTypeValue(tmprebar,Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(200);
	setdata.SetAverageSpacing(200);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);



	int	numRebarS = (int)rebarCurvesNumS.size();
	int a = 0;
	for (PITRebarCurve rebarCurve : rebarCurvesNumS)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);

		RebarElementP rebarElement = rebarSetS->AssignRebarElement(a, numRebarS, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)m_stdata.sizeKeyS);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, m_stdata.diameterSide, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string strname = tmpinfo.Hname;
			string Stype = strname + "/HoleStructualMidRebar";//其它构造钢筋都为中间
			ElementRefP oldref = tmprebar.GetElementRef();
			int num = Gettypenum();
			SetRebarLevelItemTypeValue(tmprebar, nowLayerstring, num, Stype, modelRef);
			//SetRebarLevelItemTypeValue(tmprebar,Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		a++;
	}
	RebarSetData setdataS;
	setdataS.SetNumber(numRebarS);
	setdataS.SetNominalSpacing(200);
	setdataS.SetAverageSpacing(200);
	int retS = -1;
	retS = rebarSetS->FinishUpdate(setdataS, modelRef);	//返回的是钢筋条数
	tagS->SetRset(rebarSetS);
	tagS->SetIsStirrup(isStirrup);

	return tag;
}
void HoleSTRebarAssembly::CalculateTransByFrontPts(HoleRebarInfo::ReinForcingInfo tmpinfo)
{
	if (tmpinfo.isUnionHole)
	{

		string uname(tmpinfo.Hname);
		vector<string> vecUnionchildname;
		for (HoleRebarInfo::ReinForcingInfo tmpinfo : m_vecReinF)
		{
			if (tmpinfo.isUnionChild)
			{
				string tuname(tmpinfo.Uname);
				if (tuname.find(uname) != string::npos)
				{
					string hname(tmpinfo.Hname);
					vecUnionchildname.push_back(hname);
				}
			}
		}
		int i = 0;
		for (string tname : vecUnionchildname)
		{
			if (m_holeidAndmodel[tname].ID != 0 && m_holeidAndmodel[tname].tModel != nullptr)
			{
				EditElementHandle eeh(m_holeidAndmodel[tname].ID, m_holeidAndmodel[tname].tModel);
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
				Transform trans;
				CalculateHoleTransByFrontPoints(eeh, m_vecFrontPts, trans, isfloor);
				SetTrans(trans);
				//DPoint3d ptcenter = getCenterOfElmdescr(eeh.GetElementDescrP());
				//for (int i = 0; i < m_vecFrontPts.size() - 1; i++)
				//{
				//	vector<DPoint3d> interpts;
				//	DPoint3d tmpStr, tmpEnd;
				//	tmpStr = m_vecFrontPts[i];
				//	tmpEnd = m_vecFrontPts[i + 1];
				//	tmpStr.z = tmpEnd.z = ptcenter.z;
				//	GetIntersectPointsWithHole(interpts, &eeh, tmpStr, tmpEnd);
				//	if (interpts.size() > 0)
				//	{
				//		DPoint3d ptStart = m_vecFrontPts[i];
				//		DPoint3d ptEnd = m_vecFrontPts[i + 1];
				//		CVector3D  xVec(ptStart, ptEnd);
				//		CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
				//		CVector3D  xVecNew(ptStart, ptEnd);
				//		BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
				//		Transform trans;
				//		placement.AssignTo(trans);
				//		trans.InverseOf(trans);
				//		SetTrans(trans);
				//		return;
				//	}
				//}

			}
		}
	}
	else
	{
		if (m_holeidAndmodel[tmpinfo.Hname].ID != 0 && m_holeidAndmodel[tmpinfo.Hname].tModel != nullptr)
		{
			EditElementHandle eeh(m_holeidAndmodel[tmpinfo.Hname].ID, m_holeidAndmodel[tmpinfo.Hname].tModel);
			ElementCopyContext copier2(ACTIVEMODEL);
			copier2.SetSourceModelRef(eeh.GetModelRef());
			copier2.SetTransformToDestination(true);
			copier2.SetWriteElements(false);
			copier2.DoCopy(eeh);
			Transform trans;
			CalculateHoleTransByFrontPoints(eeh, m_vecFrontPts, trans, isfloor);
			SetTrans(trans);
		}

	}
}
long HoleSTRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}

bool HoleSTRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	SetSelectedModel(ehSel.GetModelRef());
	GetConcreteXAttribute(GetConcreteOwner(), ACTIVEMODEL);

	DgnModelRefP modelRef = ACTIVEMODEL;
	WallRebarAssembly::ElementType eleType = WallRebarAssembly::JudgeElementType(ehSel);
	bool isarcwall = false;
	bool isfloor = false;
	if (WallRebarAssembly::JudgeElementType(ehSel) != WallRebarAssembly::FLOOR)
	{

		WallRebarAssembly::WallType wallType = WallRebarAssembly::JudgeWallType(ehSel);
		if (wallType == WallRebarAssembly::ARCWALL)
		{
			isarcwall = true;
		}
	}
	else
	{
		isfloor = true;
	}
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_holedlg = new CHoleRebar_StructualDlg;
	m_holedlg->isFloor = isfloor;
	m_holedlg->isArcwall = isarcwall;
	m_holedlg->SetSelectElement(ehSel);
	m_holedlg->Create(IDD_DIALOG_HoleRebar_Structural, CWnd::FromHandle(mdlNativeWindow_getMainHandle(0)));
	m_holedlg->ShowWindow(SW_SHOW);
	m_holedlg->m_HoleRebar = this;
	return true;
}

bool HoleSTRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();
	MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();
	//SetElementXAttribute(ehSel.GetElementId(), g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ehSel.GetModelRef());
	//eeh2.AddToModel();
	return true;
}


void HoleSTRebarAssembly::SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas)
{
	int i = 0;
	m_vecDirSize.resize(wallRebarDatas.size());
	m_vecRebarLevel.resize(wallRebarDatas.size());
	for (ConcreteRebar rebwall : wallRebarDatas)
	{
		m_vecDirSize[i] = rebwall.rebarSize;
		m_vecRebarLevel[i] = rebwall.datachange;
		i++;
	}

}

//将所有孔洞和钢筋点转换到ACS坐标系下
HoleArcRFRebarAssembly::HoleArcRFRebarAssembly(ElementId id, DgnModelRefP modelRef) :
	RebarAssembly(id, modelRef)
{
	m_holedlg = nullptr;
	Init();
}

void HoleArcRFRebarAssembly::Init()
{

}

void HoleArcRFRebarAssembly::ClearData()
{
	m_Trans.InitIdentity();
	m_vecDirSize.clear();
	m_vecRebarLevel.clear();
	m_rebarPts.clear();
	m_vecReinF.clear();
	m_holeidAndmodel.clear();
	m_LayerRebars.clear();
	m_holedlg = nullptr;
}
void HoleArcRFRebarAssembly::CalculateTransByFrontPts(HoleRebarInfo::ReinForcingInfo tmpinfo)
{
	if (tmpinfo.isUnionHole)
	{

		string uname(tmpinfo.Hname);
		vector<string> vecUnionchildname;
		for (HoleRebarInfo::ReinForcingInfo tmpinfo : m_vecReinF)
		{
			if (tmpinfo.isUnionChild)
			{
				string tuname(tmpinfo.Uname);
				if (tuname.find(uname) != string::npos)
				{
					string hname(tmpinfo.Hname);
					vecUnionchildname.push_back(hname);
				}
			}
		}
		int findtran = 0;
		for (string tname : vecUnionchildname)
		{
			if (findtran)
			{
				break;
			}
			if (m_holeidAndmodel[tname].ID != 0 && m_holeidAndmodel[tname].tModel != nullptr)
			{
				EditElementHandle eeh(m_holeidAndmodel[tname].ID, m_holeidAndmodel[tname].tModel);
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
				DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());
				
				DPoint3d ptcenter;
				if (!CHoleRebar_ReinforcingDlg::GetArcCenterPoint(m_rebarPts, ptcenter, &eeh))
				{
						continue;
				}
				ptcenter.z = ptele.z;
				CVector3D yVec = ptcenter - ptele;
				yVec.Normalize();

				CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

				DPoint3d ptStart = ptcenter;
				BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴

				Transform trans;
				placement.AssignTo(trans);
				trans.InverseOf(trans);
				SetTrans(trans);
				findtran = 1;
				break;
			}
		}
	}
	else
	{
		if (m_holeidAndmodel[tmpinfo.Hname].ID != 0 && m_holeidAndmodel[tmpinfo.Hname].tModel != nullptr)
		{
			EditElementHandle eeh(m_holeidAndmodel[tmpinfo.Hname].ID, m_holeidAndmodel[tmpinfo.Hname].tModel);
			ElementCopyContext copier2(ACTIVEMODEL);
			copier2.SetSourceModelRef(eeh.GetModelRef());
			copier2.SetTransformToDestination(true);
			copier2.SetWriteElements(false);
			copier2.DoCopy(eeh);
			DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());

			DPoint3d ptcenter;
			if (!CHoleRebar_ReinforcingDlg::GetArcCenterPoint(m_rebarPts, ptcenter, &eeh))
			{
				return;
			}
			ptcenter.z = ptele.z;
			CVector3D yVec = ptcenter - ptele;
			yVec.Normalize();

			CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

			DPoint3d ptStart = ptcenter;
			BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴

			Transform trans;
			placement.AssignTo(trans);
			trans.InverseOf(trans);
			SetTrans(trans);
		}

	}
}
bool HoleArcRFRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	/*if (m_vecFrontPts.size() == 0)
	{
		return false;
	}*/
	m_vecSetId.resize(m_vecDirSize.size());
	for (size_t i = 0; i < m_vecSetId.size(); i++)
	{
		m_vecSetId[i] = 0;
	}
	SetLayerRebars();

	EditElementHandle eeh(GetSelectedElement(), GetSelectedModel());
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(eeh, Eleeh, Holeehs);
	SetSelectedElement(Eleeh.GetElementId());
	NewRebarAssembly(modelRef);
	SetSelectedElement(eeh.GetElementId());

	RebarSetTagArray rsetTags;
	int tagID = 1;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	for (int j = 0; j < m_vecReinF.size(); j++)
	{
		HoleRebarInfo::ReinForcingInfo tmpinfo;
		tmpinfo = m_vecReinF.at(j);
		//if (tmpinfo.isUnionChild)//联合孔洞
		//{
		//	continue;
		//}
		if (!tmpinfo.v1 && !tmpinfo.v2 && !tmpinfo.h3 && !tmpinfo.h4)
		{
			continue;
		}
		EditElementHandle eehHole;
		CalculateTransByFrontPts(tmpinfo);
		if (tmpinfo.isUnionHole)
		{
			double hsize;
			string uname(tmpinfo.Hname);
			HoleRFRebarAssembly::GetUnionHoleeehAndSize(&eehHole, hsize,
				m_vecReinF, GetTrans(), uname, m_holeidAndmodel);
		}
		else
		{
			if (m_holeidAndmodel[tmpinfo.Hname].ID != 0 && m_holeidAndmodel[tmpinfo.Hname].tModel != nullptr)
			{
				EditElementHandle eeh(m_holeidAndmodel[tmpinfo.Hname].ID, m_holeidAndmodel[tmpinfo.Hname].tModel);
				eehHole.Duplicate(eeh);
				ISolidKernelEntityPtr entityPtr;
				if (SolidUtil::Convert::ElementToBody(entityPtr, eehHole) == SUCCESS)
				{
					SolidUtil::Convert::BodyToElement(eehHole, *entityPtr, nullptr, *eehHole.GetModelRef());
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(eehHole.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(eehHole);
				}
			}
			else
			{
				continue;
			}
		}



		EditElementHandle eehHolePlus;//放大后的孔洞
		eehHolePlus.Duplicate(eehHole);
		PlusSideCover(eehHolePlus, g_wallRebarInfo.concrete.sideCover*uor_per_mm, GetTrans());

		//eehHole.AddToModel();
		//eehHolePlus.AddToModel();
		DPoint3d ptcenter = getCenterOfElmdescr(eehHole.GetElementDescrP());

		TransformInfo transinfo(GetTrans());
		eehHole.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHole, transinfo);
		eehHolePlus.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHolePlus, transinfo);
		//eehHole.AddToModel();
		//eehHolePlus.AddToModel();


		for (int i = 0; i < m_vecDirSize.size(); ++i)
		{
			RebarSetTag* tag = NULL;
			// double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;
			PopvecSetId().at(i) = i;		
			tag = MakeRebars(tmpinfo, eehHole, eehHolePlus, m_LayerRebars[i], PopvecSetId().at(i), GetvecDirSize().at(i), ACTIVEMODEL, GetvecRebarLevel().at(i));
			if (NULL != tag)
			{
				tag->SetBarSetTag(tagID++);
				rsetTags.Add(tag);
			}

			//		}
		}
	}






	//end
	/*AddRebarSets(rsetTags);
	RebarSets rebar_sets;
	GetRebarSets(rebar_sets, ACTIVEMODEL);
	return true;*/
	//return true;
	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;

}
void HoleArcRFRebarAssembly::SetLayerRebars()
{
	m_LayerRebars.clear();
	int numFront = 0;
	int numBack = 0;
	int numMid = 0;
	//统计前，中，后层数
	for (RebarPoint pt : m_rebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			if (pt.Layer > numFront)
			{
				numFront = pt.Layer;
			}
		}
		else if (pt.DataExchange == 1)//中间
		{
			if (pt.Layer > numMid)
			{
				numMid = pt.Layer;
			}
		}
		else //背面 
		{
			if (pt.Layer > numBack)
			{
				numBack = pt.Layer;
			}
		}
	}
	for (RebarPoint pt : m_rebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			m_LayerRebars[pt.Layer - 1].push_back(pt);
		}
		else if (pt.DataExchange == 1)//中间
		{
			m_LayerRebars[pt.Layer + numFront - 1].push_back(pt);
		}
		else //背面 
		{
			m_LayerRebars[ numFront + numMid + numBack - pt.Layer].push_back(pt);
		}
	}

}
void HoleArcRFRebarAssembly::GetIDSIntesectWithHole(int& minid, int& maxid, EditElementHandleR eehHole, vector<RebarPoint>& rebarPts)
{
	int i = 0;
	DPoint3d minP = { 0 }, maxP = { 0 };
	mdlElmdscr_computeRange(&minP, &maxP, eehHole.GetElementDescrP(), NULL);

	for (RebarPoint rpts : rebarPts)
	{

		if (rpts.vecDir == 1)
		{
			if (rpts.ptstr.x<minP.x || rpts.ptstr.x >maxP.x)
			{
				i++;
				continue;
			}
		}
		else
		{
			if (rpts.ptstr.z<minP.z || rpts.ptstr.z >maxP.z)
			{
				i++;
				continue;
			}
		}
		vector<DPoint3d> ItrPts;
		if (rpts.vecDir == 0)
		{
			vector<EditElementHandle*> allHoles;
			allHoles.push_back(&eehHole);
			GetARCIntersectPointsWithHoles(ItrPts, allHoles, rpts.ptstr, rpts.ptend, rpts.ptmid);
			EditElementHandle arceeh;
			ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(rpts.ptstr, rpts.ptmid, rpts.ptend), true, *ACTIVEMODEL);
			//arceeh.AddToModel();
			//eehHole.AddToModel();
		}
		else
		{
			DPoint3d ptstr = rpts.ptstr;
			DPoint3d ptend = rpts.ptend;
			GetIntersectPointsWithHole(ItrPts, &eehHole, ptstr, ptend);
		}
		if (ItrPts.size() > 0)
		{
			if (minid == -1)
			{
				minid = i;
			}
			else if (minid > i)
			{
				minid = i;
			}
			if (maxid < i)
			{
				maxid = i;
			}
		}
		i++;
	}
}
void  HoleArcRFRebarAssembly::TransFromRebarPts(vector<RebarPoint>&  rebarPts)
{
	TransformInfo transinfo(GetTrans());
	for (int i = 0; i < rebarPts.size(); i++)
	{
		if (rebarPts.at(i).vecDir == 0)//x轴方向为弧形钢筋
		{
			EditElementHandle arceeh;
			DPoint3d ptstr = rebarPts.at(i).ptstr;
			DPoint3d ptend = rebarPts.at(i).ptend;
			DPoint3d ptmid = rebarPts.at(i).ptmid;
			ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(ptstr, ptmid, ptend), true, *ACTIVEMODEL);
			arceeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(arceeh, transinfo);
			//arceeh.AddToModel();
			DEllipse3d ellipseRev;
			mdlArc_extractDEllipse3d(&ellipseRev, &arceeh.GetElementDescrP()->el);

			ellipseRev.EvaluateEndPoints(ptstr, ptend);			
			double dLen = ellipseRev.ArcLength();
			mdlElmdscr_pointAtDistance(&ptmid, NULL, dLen / 2, arceeh.GetElementDescrP(), 1e-6);
			rebarPts.at(i).ptstr = ptstr;
			rebarPts.at(i).ptmid = ptmid;
			rebarPts.at(i).ptend = ptend;
		}
		else
		{
			DPoint3d ptstr = rebarPts.at(i).ptstr;
			DPoint3d ptend = rebarPts.at(i).ptend;
			EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			//eeh.AddToModel();
			if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
			{
				rebarPts.at(i).ptstr = ptstr;
				rebarPts.at(i).ptend = ptend;
			}
		}
		

	}

}
bool CalculateArc(RebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
{
	bool ret = false;

	BeArcSeg arc(begPt, midPt, endPt);

	CMatrix3D mat;
	CPoint3D cen;
	arc.GetCenter(cen);

	if (arc.GetPlanarMatrix(mat) && arc.GetCenter(cen))
	{
		CPoint3D beg = begPt;
		CPoint3D med = midPt;
		CPoint3D end = endPt;

		beg.Transform(mat);
		med.Transform(mat);
		end.Transform(mat);
		cen.Transform(mat);
		arc.Transform(mat);

		CVector3D tan1 = arc.GetTangentVector(beg);
		CVector3D tan2 = arc.GetTangentVector(end);

		CPointVect pv1(beg, tan1);
		CPointVect pv2(end, tan2);

		CPoint3D ip;
		bool isIntersect = pv1.Intersect(ip, pv2);

		double radius = arc.GetRadius();

		RebarVertexP vex;
		vex = &(curve.PopVertices()).NewElement();
		vex->SetIP(beg);
		vex->SetType(RebarVertex::kStart);      // first IP

		CPoint3D mid = (beg + end) / 2.0;
		CVector3D midVec(cen, mid);
		midVec.Normalize();

		if (isIntersect)
		{
			mid = cen + midVec * radius;

			// it can be on the other size
			CPoint3D mid1 = cen - midVec * radius;

			double d1 = med.Distance(mid1);
			double d2 = med.Distance(mid);

			if (d1 < d2)
			{
				mid = mid1;
				midVec = -midVec;
				// this is big arc we need 4 ips

				CVector3D midTan = midVec.Perpendicular();
				CPointVect pvm(mid, midTan);

				pv1.Intersect(ip, pvm);
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(ip);
				vex->SetType(RebarVertex::kIP);      // 2nd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				mid1 = (beg + mid) / 2.0;
				midVec = mid1 - cen;
				midVec.Normalize();
				mid1 = cen + midVec * radius;

				vex->SetArcPt(0, beg);
				vex->SetArcPt(1, mid1);
				vex->SetArcPt(2, mid);



				pv1.Intersect(ip, pvm);
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(beg);
				vex->SetType(RebarVertex::kIP);      // 3rd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				mid1 = (end + mid) / 2.0;
				midVec = mid1 - cen;
				midVec.Normalize();
				mid1 = cen + midVec * radius;

				vex->SetArcPt(0, mid);
				vex->SetArcPt(1, mid1);
				vex->SetArcPt(2, end);

			}
			else
			{
				// this is less than 90 or equal we need 3 ips
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(ip);
				vex->SetType(RebarVertex::kIP);      // 2nd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				vex->SetArcPt(0, beg);
				vex->SetArcPt(1, mid);
				vex->SetArcPt(2, end);
			}
		}
		else
		{
			// this is half circle - we need 4 ips
			midVec = arc.GetTangentVector(med);
			midVec.Normalize();
			DPoint3d ptMedTan = midVec;
			ptMedTan.Scale(radius);
			ptMedTan.Add(med);
			DPoint3d ptBegTan = tan1;
			ptBegTan.Scale(radius);
			ptBegTan.Add(beg);
			mdlVec_intersect(ip, &DSegment3d::From(beg, ptBegTan), &DSegment3d::From(med, ptMedTan));
			// 			EditElementHandle eeh;
			// 			LineHandler::CreateLineElement(eeh, NULL, DSegment3d::From(beg, ptBegTan), true, *ACTIVEMODEL);
			// 			eeh.AddToModel();
			// 			EditElementHandle eeh1;
			// 			LineHandler::CreateLineElement(eeh1, NULL, DSegment3d::From(med, ptMedTan), true, *ACTIVEMODEL);
			// 			eeh1.AddToModel();
			// 			CPointVect pvm(med, midVec);
			// 			pvm.Intersect(ip, tan1);
			// 			tan1 = ip - beg;
			// 			tan1.Normalize();
			// 
			// 			ip = beg + tan1 * radius;
			// 			midVec = ip - cen;
			// 			midVec.Normalize();
			//			midVec = ip - cen;
			mid = cen + tan1 * radius;
			DEllipse3d circle = DEllipse3d::FromCenterRadiusXY(cen, radius);

			double angle_start = circle.PointToAngle(beg);
			double angle_mid = circle.PointToAngle(mid);

			double angle = (angle_start + angle_mid) / 2;
			// 			if (angle < angle_mid)
			// 				angle += _PI;
			CPoint3D mid1;
			circle.Evaluate(&mid1, 0, angle);

			//			CPoint3D mid1 = cen + midVec;

			vex = &curve.PopVertices().NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 2nd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, beg);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, mid);

			// 			ip = end + tan1 * radius; // tan1 and tan2 parallel but tan1 has now the correct direction, do not change to tan2
			// 			midVec = ip - cen;
			// 			midVec.Normalize();
			// 			mid1 = cen + midVec * radius;

			//			midVec.Negate();
			DPoint3d ptEndTan = tan2;
			ptEndTan.Scale(radius);
			ptEndTan.Add(end);
			mdlVec_intersect(ip, &DSegment3d::From(end, ptEndTan), &DSegment3d::From(med, ptMedTan));
			// 			midVec = ip - cen;
			// 			mid1 = cen + midVec;

			// 			EditElementHandle eeh2;
			// 			LineHandler::CreateLineElement(eeh2, NULL, DSegment3d::From(end, ptEndTan), true, *ACTIVEMODEL);
			// 			eeh2.AddToModel();
			double angle_end = circle.PointToAngle(end);

			angle = (angle_end + angle_mid) / 2;
			// 			if (angle < angle_end)
			// 				angle += _PI;

			circle.Evaluate(&mid1, 0, angle);

			vex = &curve.PopVertices().NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 3rd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, mid);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, end);
		}

		vex = &curve.PopVertices().NewElement();
		vex->SetIP(end);
		vex->SetType(RebarVertex::kEnd);      // last IP

		mat = mat.Inverse();
		curve.DoMatrix(mat);              // transform back

		ret = true;
	}

	return ret;
}

void HoleArcRFRebarAssembly::CalculateIntersetPtWithHolesWithARCRebarCuve(DPoint3d& ptStr, DPoint3d& PtEnd, DPoint3d MidPt, vector<EditElementHandleP>& allholes)
{
	EditElementHandle arceeh;
	ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(ptStr, MidPt, PtEnd), true, *ACTIVEMODEL);
	//arceeh.AddToModel();
	vector<DPoint3d> pts;
	GetARCIntersectPointsWithHoles(pts, allholes, ptStr, PtEnd, MidPt);


	if (pts.size() > 0)
	{
		/*EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(pts[0], pts[1]), true, *ACTIVEMODEL);
		eeh.AddToModel();*/
	}
	map<int, DPoint3d> map_pts;
	bool isStr = false;
	double dislenth;
	dislenth = 0;
	mdlElmdscr_distanceAtPoint(&dislenth, nullptr, nullptr, arceeh.GetElementDescrP(), &PtEnd, 0.1);
	for (DPoint3d pt : pts)
	{
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &pt, 0.1);
		if (dis1 > 10 && dis1 <= dislenth + 10)
		{
			int dis = (int)dis1;
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = ptStr;
	}
	else
	{
		map_pts[0] = ptStr;
	}
	int dis = (int)dislenth;
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = PtEnd;
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = PtEnd;
	}



	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{

		PITRebarCurve trebar;
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &itr->second, 0.1);
		DPoint3d tmpMid, tmpstr, tmpend;

		tmpstr = itr->second;
		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		tmpend = itrplus->second;
		double dis2;
		dis2 = 0;
		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, arceeh.GetElementDescrP(), &tmpend, 0.1);

		dis1 = dis1 + abs(dis2 - dis1) / 2;

		mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis1, arceeh.GetElementDescrP(), 0.1);

		if (dis1<dislenth/2&&dis2>dislenth/2)//如果中点在此弧段上，就取此段弧
		{
			ptStr = tmpstr;
			PtEnd = tmpend;
			return;
		}
		
	}

	

}

bool HoleArcRFRebarAssembly::CalculateUpAndDownRebarcurve(EditElementHandle* eehHole,DPoint3d minP,DPoint3d maxP,
	RebarPoint rebrptmin,double Posz, HoleRebarInfo::ReinForcingInfo tmpinfo, vector<RebarCurve>& rebarCurvesNum)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	Transform inversMat = GetTrans();
	inversMat.InverseOf(inversMat);
	TransformInfo transinfo(inversMat);

	vector<EditElementHandle*> allHoles;
	allHoles.push_back(eehHole);
	vector<DPoint3d> ItrPts;
	rebrptmin.ptstr.z = (minP.z + maxP.z) / 2;
	rebrptmin.ptmid.z = (minP.z + maxP.z) / 2;
	rebrptmin.ptend.z = (minP.z + maxP.z) / 2;
	GetARCIntersectPointsWithHoles(ItrPts, allHoles, rebrptmin.ptstr, rebrptmin.ptend, rebrptmin.ptmid);
	if (ItrPts.size() != 2)
	{
		return false;
	}
	DEllipse3d ellipsePro = DEllipse3d::FromPointsOnArc(rebrptmin.ptstr, rebrptmin.ptmid, rebrptmin.ptend);
	DEllipse3d ellipsePro2 = DEllipse3d::FromPointsOnArc(rebrptmin.ptstr, rebrptmin.ptmid, rebrptmin.ptend);
	ellipsePro.SetStartEnd(ItrPts[0], ItrPts[1], true);
	ellipsePro2.SetStartEnd(ItrPts[0], ItrPts[1], false);
	if (ellipsePro2.ArcLength() < ellipsePro.ArcLength())
	{
		ellipsePro = ellipsePro2;
	}

	EditElementHandle arceeh;
	ArcHandler::CreateArcElement(arceeh, nullptr, ellipsePro, true, *ACTIVEMODEL);
	PIT::ArcSegment arcSource;
	arcSource.ptCenter = ellipsePro.center;
	arcSource.ptStart = ItrPts[0];
	arcSource.ptEnd = ItrPts[1];
	arcSource.dRadius = ellipsePro.center.Distance(ItrPts[0]);
	arcSource.dLen = ellipsePro.ArcLength();
	mdlElmdscr_pointAtDistance(&arcSource.ptMid, NULL, arcSource.dLen / 2, arceeh.GetElementDescrP(), 1e-6);

	{//计算延长的长度
		double distanceStr = (arcSource.ptStart.Distance(rebrptmin.ptstr) < arcSource.ptStart.Distance(rebrptmin.ptend)) ? 
			                   arcSource.ptStart.Distance(rebrptmin.ptstr) : arcSource.ptStart.Distance(rebrptmin.ptend);
		double distanceEnd = (arcSource.ptEnd.Distance(rebrptmin.ptstr) < arcSource.ptEnd.Distance(rebrptmin.ptend)) ?
			arcSource.ptEnd.Distance(rebrptmin.ptstr) : arcSource.ptEnd.Distance(rebrptmin.ptend);

		if (distanceStr> 200 * uor_per_mm + tmpinfo.L0Lenth*uor_per_mm)
		{
			distanceStr = 200 * uor_per_mm + tmpinfo.L0Lenth*uor_per_mm;
		}
		if (distanceEnd > 200 * uor_per_mm + tmpinfo.L0Lenth*uor_per_mm)
		{
			distanceEnd = 200 * uor_per_mm + tmpinfo.L0Lenth*uor_per_mm;
		}
		arcSource.Shorten(-distanceStr, true);
		arcSource.Shorten(-distanceEnd, false);
	}
	
	//arcSource.Shorten(-200 * uor_per_mm - tmpinfo.L0Lenth*uor_per_mm, true);
	//arcSource.Shorten(-200 * uor_per_mm - tmpinfo.L0Lenth*uor_per_mm, false);


	arcSource.ptStart.z = Posz; arcSource.ptMid.z = Posz; arcSource.ptEnd.z = Posz;
	EditElementHandle arceeh2;
	ArcHandler::CreateArcElement(arceeh2, nullptr, DEllipse3d::FromPointsOnArc(arcSource.ptStart, arcSource.ptMid, arcSource.ptEnd), true, *ACTIVEMODEL);
	arceeh2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(arceeh2, transinfo);
	//arceeh2.AddToModel();
	DPoint3d ArcDPs[2];
	RotMatrix rotM;
	double radius;
	mdlArc_extract(ArcDPs, nullptr, nullptr, &radius, NULL, &rotM, &arcSource.ptCenter, &arceeh2.GetElementDescrP()->el);
	mdlElmdscr_pointAtDistance(&arcSource.ptMid, NULL, arcSource.dLen / 2, arceeh2.GetElementDescrP(), 1e-6);


	CalculateIntersetPtWithHolesWithARCRebarCuve(ArcDPs[0], ArcDPs[1], arcSource.ptMid, m_useHoleehs);
	RebarCurve     rebarCurves;
	if (CalculateArc(rebarCurves, ArcDPs[0], arcSource.ptMid, ArcDPs[1]))
	{
		rebarCurvesNum.push_back(rebarCurves);
	}
	return true;
}


RebarSetTag* HoleArcRFRebarAssembly::MakeRebars
(
	HoleRebarInfo::ReinForcingInfo tmpinfo,
	EditElementHandle& eehHole,
	EditElementHandle& eehHolePlus,
	vector<RebarPoint>&  rebarPts,
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	DgnModelRefP        modelRef,
	int DataExchange
)
{
	int layernow = 0;
	int DiamUsizeKey = 0;
	double DiameterU;
	for (int i = 0; i < m_vecDirSize.size(); i++)
	{
		int tmpSize = BeStringUtilities::Wtoi(m_vecDirSize.at(i));
		if (tmpSize > DiamUsizeKey)
		{
			DiamUsizeKey = tmpSize;
		}
	}
	if (DiamUsizeKey > 25)
	{
		DiameterU = RebarCode::GetBarDiameter(L"20A", modelRef);
	}
	else
	{
		DiameterU = RebarCode::GetBarDiameter(L"16A", modelRef);
	}

	vector<RebarPoint> tmprebarPts;
	tmprebarPts.insert(tmprebarPts.begin(), rebarPts.begin(), rebarPts.end());
	TransFromRebarPts(tmprebarPts);
	bool const isStirrup = false;
	ElementId tmpID = rebarSetId;
	rebarSetId = 0;
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	switch (0)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeStart.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (0)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeEnd.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}
	//	}
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
	double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100

	DiameterU = bendRadius + DiameterU / 2;

	tmpinfo.L0Lenth = g_globalpara.m_laplenth[(string)sizeKey];//搭接长度
	if (tmpinfo.L0Lenth == 0)
	{
		tmpinfo.L0Lenth = 200;
	}
	vector<RebarCurve>     rebarCurvesNum;
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
	if (tmprebarPts.size() > 0)
	{
		layernow = tmprebarPts[0].Layer;
		DPoint3d minP = { 0 }, maxP = { 0 };
		mdlElmdscr_computeRange(&minP, &maxP, eehHole.GetElementDescrP(), NULL);
		int minXID = -1; int maxXID = -1;
		GetIDSIntesectWithHole(minXID, maxXID, eehHolePlus, tmprebarPts);
		double minxpos, maxxpos, minzpos, maxzpos;
		minxpos = (tmprebarPts.begin()->ptstr.x < tmprebarPts.rbegin()->ptstr.x) ? tmprebarPts.begin()->ptstr.x : tmprebarPts.rbegin()->ptstr.x;
		maxxpos = (tmprebarPts.begin()->ptstr.x > tmprebarPts.rbegin()->ptstr.x) ? tmprebarPts.begin()->ptstr.x : tmprebarPts.rbegin()->ptstr.x;
		minzpos = (tmprebarPts.begin()->ptstr.z < tmprebarPts.rbegin()->ptstr.z) ? tmprebarPts.begin()->ptstr.z : tmprebarPts.rbegin()->ptstr.z;
		maxzpos = (tmprebarPts.begin()->ptstr.z > tmprebarPts.rbegin()->ptstr.z) ? tmprebarPts.begin()->ptstr.z : tmprebarPts.rbegin()->ptstr.z;
		if (minXID == -1 || maxXID == -1)
		{
			return false;
		}

		Transform inversMat = GetTrans();
		inversMat.InverseOf(inversMat);
		TransformInfo transinfo(inversMat);
		if (tmprebarPts[0].vecDir == 0)//X方向钢筋
		{
			int tmpmin = 0;
			int tmpmax = 0;

			if (tmprebarPts.at(minXID).ptstr.z < tmprebarPts.at(maxXID).ptstr.z)//如果是从下往上布置的，交换min和max
			{
				tmpmin = maxXID + 1;
				tmpmax = minXID - 1;
			}
			else
			{
				tmpmin = minXID - 1;
				tmpmax = maxXID + 1;
			}

			if (tmpmin > 0 && tmpinfo.v1&&tmpmin< tmprebarPts.size())//上边
			{
				int numRebar = tmpinfo.numv1;

				for (int i = 0; i < numRebar; i++)
				{
					double spacing = i * tmpinfo.spacingv1*uor_per_mm;
					RebarPoint rebrptmin = tmprebarPts.at(tmpmin);
					double Posz;
					if (rebrptmin.ptstr.z - maxP.z > 150 * uor_per_mm)
					{
						Posz = maxP.z + g_wallRebarInfo.concrete.sideCover*uor_per_mm + DiameterU + spacing;
					}
					else
					{
						Posz = rebrptmin.ptstr.z + tmpinfo.MainRebarDis*uor_per_mm + diameter + spacing;
					}
					/*if (Posz > maxzpos)
					{
						continue;
					}*/
					CalculateUpAndDownRebarcurve(&eehHole, minP, maxP, rebrptmin, Posz, tmpinfo, rebarCurvesNum);
				}

			}
			if (tmpmax < tmprebarPts.size() && tmpinfo.v2&&tmpmax>0)//下边
			{
				int numRebar = tmpinfo.numv2;

				for (int i = 0; i < numRebar; i++)
				{
					double spacing = i * tmpinfo.spacingv2*uor_per_mm;
					RebarPoint rebrptmin = tmprebarPts.at(tmpmax);
					double Posz;
					if (minP.z - rebrptmin.ptstr.z > 150 * uor_per_mm)
					{
						Posz = minP.z - g_wallRebarInfo.concrete.sideCover*uor_per_mm - DiameterU - spacing;
					}
					else
					{
						Posz = rebrptmin.ptstr.z - tmpinfo.MainRebarDis * uor_per_mm - diameter - spacing;
					}
					/*if (Posz < minzpos)
					{
						continue;
					}*/
					CalculateUpAndDownRebarcurve(&eehHole, minP, maxP, rebrptmin, Posz, tmpinfo, rebarCurvesNum);
				}
			}
		}
		else//Z方向钢筋
		{
		int tmpmin = 0;
		int tmpmax = 0;

		    if (tmprebarPts.at(minXID).ptstr.x > tmprebarPts.at(maxXID).ptstr.x)//如果是从右往左布置的，交换min和max
		    {
				tmpmin = maxXID + 1;
				tmpmax = minXID -1;
		    }
			else
			{
				tmpmin = minXID - 1;
				tmpmax = maxXID + 1;
			}

			if (tmpmin > 0 && tmpinfo.h3&&tmpmin< tmprebarPts.size())//左边
			{
				int numRebar = tmpinfo.numh3;

				for (int i = 0; i < numRebar; i++)
				{
					double spacing = i * tmpinfo.spacingh3*uor_per_mm;
					RebarPoint rebrptmin = tmprebarPts.at(tmpmin);
					double Posx;
					if (minP.x - rebrptmin.ptstr.x > 150 * uor_per_mm)
					{
						Posx = minP.x - g_wallRebarInfo.concrete.sideCover*uor_per_mm - DiameterU - spacing;
					}
					else
					{
						Posx = rebrptmin.ptstr.x - tmpinfo.MainRebarDis * uor_per_mm - diameter - spacing;
					}
					/*if (Posx < minxpos)
					{
						continue;
					}*/
					DPoint3d ptstr = rebrptmin.ptstr;
					DPoint3d ptend = rebrptmin.ptstr;
					ptstr.z = minP.z; ptend.z = maxP.z;
					double minZPos = (rebrptmin.ptstr.z < rebrptmin.ptend.z) ? rebrptmin.ptstr.z : rebrptmin.ptend.z;
					double maxZPos = (rebrptmin.ptstr.z > rebrptmin.ptend.z) ? rebrptmin.ptstr.z : rebrptmin.ptend.z;
					ptstr.z = ptstr.z - 200 * uor_per_mm - tmpinfo.L0Lenth*uor_per_mm;
					ptend.z = ptend.z + 200 * uor_per_mm + tmpinfo.L0Lenth*uor_per_mm;
					if (ptstr.z < minZPos)
					{
						ptstr.z = minZPos;
					}
					if (ptend.z > maxZPos)
					{
						ptend.z = maxZPos;
					}
					ptstr.x = Posx; ptend.x = Posx;
					RebarCurve     rebarCurves;
					EditElementHandle eeh;
					LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
					eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
					if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
					{
						makeRebarCurve(rebarCurves, bendRadius, bendLen, endTypes, ptstr, ptend);
						rebarCurvesNum.push_back(rebarCurves);
					}
				}

			}
			if (tmpmax < tmprebarPts.size() && tmpinfo.h4&&tmpmax>0)//右边
			{
				int numRebar = tmpinfo.numh4;

				for (int i = 0; i < numRebar; i++)
				{
					double spacing = i * tmpinfo.spacingh4*uor_per_mm;
					RebarPoint rebrptmin = tmprebarPts.at(tmpmax);
					double Posx;
					if (rebrptmin.ptstr.x - maxP.x > 150 * uor_per_mm)
					{
						Posx = maxP.x + g_wallRebarInfo.concrete.sideCover*uor_per_mm + DiameterU + spacing;
					}
					else
					{
						Posx = rebrptmin.ptstr.x + tmpinfo.MainRebarDis * uor_per_mm + diameter + spacing;
					}
					/*if (Posx > maxxpos)
					{
						continue;
					}*/
					DPoint3d ptstr = rebrptmin.ptstr;
					DPoint3d ptend = rebrptmin.ptstr;
					ptstr.z = minP.z; ptend.z = maxP.z;
					double minZPos = (rebrptmin.ptstr.z < rebrptmin.ptend.z) ? rebrptmin.ptstr.z : rebrptmin.ptend.z;
					double maxZPos = (rebrptmin.ptstr.z > rebrptmin.ptend.z) ? rebrptmin.ptstr.z : rebrptmin.ptend.z;
					ptstr.z = ptstr.z - 200 * uor_per_mm - tmpinfo.L0Lenth*uor_per_mm;
					ptend.z = ptend.z + 200 * uor_per_mm + tmpinfo.L0Lenth*uor_per_mm;
					if (ptstr.z < minZPos)
					{
						ptstr.z = minZPos;
					}
					if (ptend.z > maxZPos)
					{
						ptend.z = maxZPos;
					}
					ptstr.x = Posx; ptend.x = Posx;
					RebarCurve     rebarCurves;
					EditElementHandle eeh;
					LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
					eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
					if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
					{
						makeRebarCurve(rebarCurves, bendRadius, bendLen, endTypes, ptstr, ptend);
						rebarCurvesNum.push_back(rebarCurves);
					}
				}		
			}
		}

		/*for (int i = 0; i < m_rebarPts.size()-1; i++)
		{
			int j = i + 1;
			CPoint3D ptstr(m_rebarPts.at(i).pt);
			CPoint3D ptend(m_rebarPts.at(j).pt);
			if (m_rebarPts.at(i).Layer == tmpID && m_rebarPts.at(i).Layer == m_rebarPts.at(j).Layer)
			{
				RebarCurve     rebarCurves;
				TransformInfo transinfo(GetTrans());
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
				eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
				eeh.AddToModel();
				makeRebarCurve(rebarCurves, bendRadius, bendLen, endTypes, ptstr, ptend);
				rebarCurvesNum.push_back(rebarCurves);
			}
			i++;
		}*/
	}



	RebarSymbology symb;
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_CAVE_REBAR);
	}
	int	numRebar = (int)rebarCurvesNum.size();
	int j = 0;
	char nowLayerc[256];
	sprintf(nowLayerc, "%d", layernow);
	string nowLayerstring(nowLayerc);
	for (RebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);

		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string strname = tmpinfo.Hname;

			string RebarType;
			if (DataExchange == 0)
			{
				RebarType = "/HoleForcefrontRebar";
			}
			else if (DataExchange == 1)
			{
				RebarType = "/HoleForcemiddenRebar";
			}
			else
			{
				RebarType = "/HoleForcebackRebar";
			}
			string Stype = strname + RebarType;
			ElementRefP oldref = tmprebar.GetElementRef();
			int num = Gettypenum();
			SetRebarLevelItemTypeValue(tmprebar, nowLayerstring, num, Stype, modelRef);
			//SetRebarLevelItemTypeValue(tmprebar,Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(200);
	setdata.SetAverageSpacing(200);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}
bool HoleArcRFRebarAssembly::makeRebarCurve
(
	RebarCurve&     rebar,
	double                  bendRadius,
	double                  bendLen,
	RebarEndTypes const&    endTypes,
	CPoint3D const&        ptstr,
	CPoint3D const&        ptend
)
{
	DPoint3d midPos = ptstr;
	midPos.Add(ptend);
	midPos.Scale(0.5);

	DPoint3d dstr = ptstr;
	DPoint3d dend = ptend;

	CalculateIntersetPtWithHolesWithRebarCuve(dstr, dend, midPos, m_useHoleehs);
	RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(dstr);
	vex->SetType(RebarVertex::kStart);


	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(dend);
	vex->SetType(RebarVertex::kEnd);

	CVector3D   endNormal(-1.0, 0.0, 0.0);
	rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, endNormal);
	//rebar.DoMatrix(mat);
	return true;
}
long HoleArcRFRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}

bool HoleArcRFRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	SetSelectedModel(ehSel.GetModelRef());
	GetConcreteXAttribute(GetConcreteOwner(), ACTIVEMODEL);

	DgnModelRefP modelRef = ACTIVEMODEL;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_holedlg = new CHoleRebar_ReinforcingDlg;
	m_holedlg->isArcwall = true;
	m_holedlg->SetSelectElement(ehSel);
	m_holedlg->Create(IDD_DIALOG_HoleRebar_Reinforcing, CWnd::FromHandle(mdlNativeWindow_getMainHandle(0)));
	m_holedlg->ShowWindow(SW_SHOW);
	//m_holedlg->m_HoleRebar = this;
	return true;
}

bool HoleArcRFRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();
	MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();
	//SetElementXAttribute(ehSel.GetElementId(), g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ehSel.GetModelRef());
	//eeh2.AddToModel();
	return true;
}
RebarAssembly* HoleArcRFRebarAssembly::GetRebarAssembly(ElementId concreteid, string assemblyname)
{
	RebarAssemblies area;
	REA::GetRebarAssemblies(concreteid, area);
	for (int i = 0; i < area.GetSize(); i++)
	{
		RebarAssembly* rebaras = area.GetAt(i);
		string tesname = typeid(*rebaras).name();
		if (tesname == assemblyname)
		{
			return rebaras;
		}
	}
	return nullptr;
}
void HoleArcRFRebarAssembly::GetUnionHoleeehAndSize(EditElementHandle* eehhole, double& holesize,
	vector<HoleRebarInfo::ReinForcingInfo>& vecReininfo, Transform trans, string UnionName,
	std::map<std::string, IDandModelref>& m_holeidAndmodel)
{
	vector<string> vecUnionchildname;
	for (HoleRebarInfo::ReinForcingInfo tmpinfo : vecReininfo)
	{
		if (tmpinfo.isUnionChild)
		{
			string tuname(tmpinfo.Uname);
			if (tuname.find(UnionName) != string::npos)
			{
				string hname(tmpinfo.Hname);
				vecUnionchildname.push_back(hname);
			}
		}


	}
	TransformInfo tinfo(trans);
	DPoint3d ptmin = { 0 }, ptmax = { 0 };
	int i = 0;
	for (string tname : vecUnionchildname)
	{
		if (m_holeidAndmodel[tname].ID != 0 && m_holeidAndmodel[tname].tModel != nullptr)
		{
			EditElementHandle eeh(m_holeidAndmodel[tname].ID, m_holeidAndmodel[tname].tModel);
			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
				eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, tinfo);

				DPoint3d minP = { 0 }, maxP = { 0 };
				mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
				if (i == 0)
				{
					ptmin = minP;
					ptmax = maxP;
					i++;
				}
				else
				{
					ptmin.x = (ptmin.x < minP.x) ? ptmin.x : minP.x;
					ptmin.y = (ptmin.y < minP.y) ? ptmin.y : minP.y;
					ptmin.z = (ptmin.z < minP.z) ? ptmin.z : minP.z;

					ptmax.x = (ptmax.x > maxP.x) ? ptmax.x : maxP.x;
					ptmax.y = (ptmax.y > maxP.y) ? ptmax.y : maxP.y;
					ptmax.z = (ptmax.z > maxP.z) ? ptmax.z : maxP.z;
				}


			}

		}
	}
	if (ptmin.IsEqual(ptmax))
	{
		holesize = 0;
		return;
	}
	DRange3d range;
	range.high = ptmax;
	range.low = ptmin;

	holesize = (range.XLength() > range.ZLength()) ? range.XLength() : range.ZLength();
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	holesize = holesize / uor_per_mm;
	if (eehhole != nullptr)
	{
		DPoint3d ptCenter = range.low;
		ptCenter.x = ptCenter.x + range.XLength() / 2;
		ptCenter.y = ptCenter.y + range.YLength() / 2;
		ptCenter.z = ptCenter.z + range.ZLength() / 2;
		DPoint3dCR ptSize = { range.XLength(), range.YLength(),range.ZLength() };

		DgnBoxDetail dgnBoxDetail = DgnBoxDetail::InitFromCenterAndSize(ptCenter, ptSize, true);
		ISolidPrimitivePtr solidPtr = ISolidPrimitive::CreateDgnBox(dgnBoxDetail);
		if (SUCCESS != DraftingElementSchema::ToElement(*eehhole, *solidPtr, NULL, *ACTIVEMODEL))
		{
			holesize = 0;
			return;
		}
		trans.InverseOf(trans);
		TransformInfo inversInfo(trans);
		eehhole->GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(*eehhole, inversInfo);
	}

}
void HoleArcRFRebarAssembly::GetUnionHoleeeh(EditElementHandle* eehhole, vector<string>& vecUnionchildname, double& holesize, Transform trans,
	std::map<std::string, IDandModelref>& m_holeidAndmodel)
{
	if (eehhole == nullptr)
	{
		return;
	}
	TransformInfo tinfo(trans);
	DPoint3d ptmin = { 0 }, ptmax = { 0 };
	int i = 0;
	for (string tname : vecUnionchildname)
	{
		if (m_holeidAndmodel[tname].ID != 0 && m_holeidAndmodel[tname].tModel != nullptr)
		{
			EditElementHandle eeh(m_holeidAndmodel[tname].ID, m_holeidAndmodel[tname].tModel);
			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
				eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, tinfo);

				DPoint3d minP = { 0 }, maxP = { 0 };
				mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
				if (i == 0)
				{
					ptmin = minP;
					ptmax = maxP;
					i++;
				}
				else
				{
					ptmin.x = (ptmin.x < minP.x) ? ptmin.x : minP.x;
					ptmin.y = (ptmin.y < minP.y) ? ptmin.y : minP.y;
					ptmin.z = (ptmin.z < minP.z) ? ptmin.z : minP.z;

					ptmax.x = (ptmax.x > maxP.x) ? ptmax.x : maxP.x;
					ptmax.y = (ptmax.y > maxP.y) ? ptmax.y : maxP.y;
					ptmax.z = (ptmax.z > maxP.z) ? ptmax.z : maxP.z;
				}


			}

		}
	}
	if (ptmin.IsEqual(ptmax))
	{
		holesize = 0;
		return;
	}
	DRange3d range;
	range.high = ptmax;
	range.low = ptmin;

	holesize = (range.XLength() > range.ZLength()) ? range.XLength() : range.ZLength();
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	holesize = holesize / uor_per_mm;
	if (eehhole != nullptr)
	{
		DPoint3d ptCenter = range.low;
		ptCenter.x = ptCenter.x + range.XLength() / 2;
		ptCenter.y = ptCenter.y + range.YLength() / 2;
		ptCenter.z = ptCenter.z + range.ZLength() / 2;
		DPoint3dCR ptSize = { range.XLength(), range.YLength(),range.ZLength() };

		DgnBoxDetail dgnBoxDetail = DgnBoxDetail::InitFromCenterAndSize(ptCenter, ptSize, true);
		ISolidPrimitivePtr solidPtr = ISolidPrimitive::CreateDgnBox(dgnBoxDetail);
		if (SUCCESS != DraftingElementSchema::ToElement(*eehhole, *solidPtr, NULL, *ACTIVEMODEL))
		{
			holesize = 0;
			return;
		}
		trans.InverseOf(trans);
		TransformInfo inversInfo(trans);
		eehhole->GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(*eehhole, inversInfo);
	}
}
bool HoleArcRFRebarAssembly::IsSmartSmartFeature(EditElementHandle& eeh)
{
	SmartFeatureNodePtr pFeatureNode;
	if (SmartFeatureElement::IsSmartFeature(eeh))
	{
		return true;
	}
	else
	{
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			if (SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef()) == SUCCESS)
			{
				eeh.AddToModel();
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}

		}
		return true;
	}
}

void HoleArcRFRebarAssembly::SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas)
{
	int i = 0;
	m_vecDirSize.resize(wallRebarDatas.size());
	m_vecRebarLevel.resize(wallRebarDatas.size());
	for (ConcreteRebar rebwall : wallRebarDatas)
	{
		m_vecDirSize[i] = rebwall.rebarSize;
		m_vecRebarLevel[i] = rebwall.datachange;
		i++;
	}

}


HoleArcSTRebarAssembly::HoleArcSTRebarAssembly(ElementId id, DgnModelRefP modelRef) :
	RebarAssembly(id, modelRef)
{
	m_holedlg = nullptr;
	m_FrontdiameterSide = 0.0;
	Init();
}

void HoleArcSTRebarAssembly::Init()
{

}

void HoleArcSTRebarAssembly::ClearData()
{

	m_Trans.InitIdentity();
	m_vecDirSize.clear();
	m_vecRebarLevel.clear();
	m_LayerRebars.clear();
	m_LayerTwinRebars.clear();
	m_rebarPts.clear();
	m_vecReinF.clear();
	m_holeidAndmodel.clear();
	m_LayerRebars.clear();
	m_holedlg = nullptr;
}

bool HoleArcSTRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	/*if (m_vecFrontPts.size() == 0)
	{
		return false;
	}*/
	m_vecSetId.resize(m_vecDirSize.size());
	for (size_t i = 0; i < m_vecSetId.size(); i++)
	{
		m_vecSetId[i] = 0;
	}
	SetLayerRebars();
	if (m_LayerRebars.size() < 2)
	{
		return false;
	}
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	if (m_vecReinF.size()<1)
	{
		return false;
	}

	HoleRebarInfo::ReinForcingInfo tmpinfo;
	tmpinfo = m_vecReinF.at(0);
	CalculateTransByFrontPts(tmpinfo);

	vector<RebarPoint> frontRpts;
	vector<RebarPoint> backRpts;

	frontRpts.push_back(*m_LayerRebars.begin()->second.begin());
	backRpts.push_back(*m_LayerRebars.rbegin()->second.begin());
	TransFromRebarPts(frontRpts);
	TransFromRebarPts(backRpts);
	//double Wthickness = abs(backRpts.at(0).ptstr.y - frontRpts.at(0).ptstr.y) + g_wallRebarInfo.concrete.postiveCover*uor_per_mm + g_wallRebarInfo.concrete.reverseCover*uor_per_mm;

	double Wthickness = abs(m_OutRadius - m_InRadius) + g_wallRebarInfo.concrete.postiveCover*uor_per_mm + g_wallRebarInfo.concrete.reverseCover*uor_per_mm;

	EditElementHandle eeh(GetSelectedElement(), GetSelectedModel());
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(eeh, Eleeh, Holeehs);
	SetSelectedElement(Eleeh.GetElementId());
	NewRebarAssembly(modelRef);
	SetSelectedElement(eeh.GetElementId());

	RebarSetTagArray rsetTags;
	int tagID = 1;


	for (int j = 0; j < m_vecReinF.size(); j++)
	{
		HoleRebarInfo::ReinForcingInfo tmpinfo;
		tmpinfo = m_vecReinF.at(j);
		m_FrontdiameterSide = 0.0;
		//if (tmpinfo.isUnionChild)//联合孔洞
		//{
		//	continue;
		//}
		if (!tmpinfo.v1 && !tmpinfo.v2 && !tmpinfo.h3 && !tmpinfo.h4)
		{
			continue;
		}
		CalculateTransByFrontPts(tmpinfo);
		TransformInfo transinfo(GetTrans());
		if (tmpinfo.isUnionHole)
		{
			string UnionName(tmpinfo.Hname);
			vector<string> vecUnionchildname;
			for (HoleRebarInfo::ReinForcingInfo tmpinfo : m_vecReinF)
			{
				if (tmpinfo.isUnionChild)
				{
					string uname(tmpinfo.Uname);
					if (uname.find(UnionName)!= string::npos)
					{
						string hname(tmpinfo.Hname);
						vecUnionchildname.push_back(hname);
					}
				}
			}
			EditElementHandle eehhole;
			double hsize;
			string uname(tmpinfo.Hname);
			HoleRFRebarAssembly::GetUnionHoleeehAndSize(&eehhole, hsize,
				m_vecReinF, GetTrans(), uname, m_holeidAndmodel);
			
			DPoint3d ptele = getCenterOfElmdescr(eehhole.GetElementDescrP());

				DPoint3d ptcenter;
				if (!CHoleRebar_ReinforcingDlg::GetArcCenterPoint(m_rebarPts, ptcenter, &eehhole))
				{
					continue;
				}
				ptcenter.z = ptele.z;
				//CVector3D yVec = ptcenter - ptele;
				CVector3D yVec = ptele - ptcenter;
				yVec.Normalize();

				CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

				DPoint3d ptStart = ptcenter;
				BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴

				Transform trans;
				placement.AssignTo(trans);
				trans.InverseOf(trans);
				SetTrans(trans);
				for (int i = 0; i < m_vecDirSize.size() / 2; ++i)
				{
					if (i > 1)//先屏蔽多层钢筋处理
					{
						continue;
					}
					if (m_vecTwinBarData.size() != m_vecDirSize.size())
					{
						memset(&m_twinbarinfo, 0, sizeof(m_twinbarinfo));
					}
					else
					{
						m_twinbarinfo = m_vecTwinBarData[i];//当前层的并筋信息
					}

					int j = (int)(m_vecDirSize.size() - 1 - i);
					if (j < 0)
					{
						break;
					}
					RebarSetTag* tag = NULL;
					// double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;

					PopvecSetId().at(i) = i;
					tag = MakeRebars(Wthickness, tmpinfo, vecUnionchildname, m_LayerRebars[i], m_LayerRebars[j], PopvecSetId().at(i), GetvecDirSize().at(i), ACTIVEMODEL);
					if (NULL != tag)
					{
						tag->SetBarSetTag(tagID++);
						rsetTags.Add(tag);
					}

				//		}
			}
			//}

		}
		else
		{
			if (m_holeidAndmodel[tmpinfo.Hname].ID == 0 || m_holeidAndmodel[tmpinfo.Hname].tModel == nullptr)
			{
				continue;
			}
			EditElementHandle eehHole(m_holeidAndmodel[tmpinfo.Hname].ID, m_holeidAndmodel[tmpinfo.Hname].tModel);
			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eehHole) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eehHole, *entityPtr, nullptr, *eehHole.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eehHole.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eehHole);
			}

			EditElementHandle eehHolePlus;//放大后的孔洞
			eehHolePlus.Duplicate(eehHole);
			PlusSideCover(eehHolePlus, g_wallRebarInfo.concrete.sideCover*uor_per_mm, GetTrans());
			DPoint3d ptcenter = getCenterOfElmdescr(eehHole.GetElementDescrP());
			TransformInfo transinfo(GetTrans());
			eehHole.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHole, transinfo);
			eehHolePlus.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHolePlus, transinfo);
			//eehHole.AddToModel();
			//eehHolePlus.AddToModel();
			/*if (g_vecTwinBarData.size()> m_vecDirSize.size() / 2)
			{*/

			for (int i = 0; i < m_vecDirSize.size() / 2; ++i)
			{
				if (i > 1)//先屏蔽多层钢筋处理
				{
					continue;
				}
				if (m_vecTwinBarData.size() != m_vecDirSize.size())
				{
					memset(&m_twinbarinfo, 0, sizeof(m_twinbarinfo));
				}
				else
				{
					m_twinbarinfo = m_vecTwinBarData[i];//当前层的并筋信息
				}
				int j = (int)(m_vecDirSize.size() - 1 - i);
				if (j < 0)
				{
					break;
				}
				RebarSetTag* tag = NULL;
				// double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;
				PopvecSetId().at(i) = i;			
				tag = MakeRebars(Wthickness, tmpinfo, eehHole, eehHolePlus, m_LayerRebars[i], m_LayerRebars[j], PopvecSetId().at(i), GetvecDirSize().at(i), ACTIVEMODEL);
				if (NULL != tag)
				{
					tag->SetBarSetTag(tagID++);
					rsetTags.Add(tag);
				}

				//		}
			}
			//}

		}


	}






	//end
	/*AddRebarSets(rsetTags);
	RebarSets rebar_sets;
	GetRebarSets(rebar_sets, ACTIVEMODEL);
	return true;*/
	//return true;
	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;

}
void HoleArcSTRebarAssembly::SetLayerRebars()
{
	m_LayerRebars.clear();
	m_LayerTwinRebars.clear();
	int numFront = 0;
	int numBack = 0;
	int numMid = 0;
	//统计前，中，后层数
	for (RebarPoint pt : m_rebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			if (pt.Layer > numFront)
			{
				numFront = pt.Layer;
			}
		}
		else if (pt.DataExchange == 1)//中间
		{
			if (pt.Layer > numMid)
			{
				numMid = pt.Layer;
			}
		}
		else //背面 
		{
			if (pt.Layer > numBack)
			{
				numBack = pt.Layer;
			}
		}
	}
	for (RebarPoint pt : m_rebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			m_LayerRebars[pt.Layer - 1].push_back(pt);
		}
		else if (pt.DataExchange == 1)//中间
		{
			m_LayerRebars[pt.Layer + numFront - 1].push_back(pt);
		}
		else //背面 
		{
			m_LayerRebars[numFront + numMid + numBack - pt.Layer].push_back(pt);
		}
	}
	numFront = 0;
	numBack = 0;
	numMid = 0;
	//统计前，中，后层数
	for (RebarPoint pt : m_twinrebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			if (pt.Layer > numFront)
			{
				numFront = pt.Layer;
			}
		}
		else if (pt.DataExchange == 1)//中间
		{
			if (pt.Layer > numMid)
			{
				numMid = pt.Layer;
			}
		}
		else //背面 
		{
			if (pt.Layer > numBack)
			{
				numBack = pt.Layer;
			}
		}
	}
	for (RebarPoint pt : m_twinrebarPts)
	{
		if (pt.DataExchange == 0)//前面
		{
			m_LayerTwinRebars[pt.Layer - 1].push_back(pt);
		}
		else if (pt.DataExchange == 1)//中间
		{
			m_LayerTwinRebars[pt.Layer + numFront - 1].push_back(pt);
		}
		else //背面 
		{
			m_LayerTwinRebars[numFront + numMid + numBack - pt.Layer].push_back(pt);
		}
	}
	if (m_LayerRebars.size()!=m_vecDirSize.size())
	{
		return;
	}
	RebarPoint outArc,inArc;
	for (int i = 0;i<m_vecDirSize.size();i++)
	{
		if (m_LayerRebars[i].size()>0)
		{
			if (m_LayerRebars[i].begin()->vecDir==0)
			{
				outArc = *(m_LayerRebars[i].begin());
				break;
			}
		}
	}
	for (int i = m_vecDirSize.size()-1; i > 0; i--)
	{
		if (m_LayerRebars[i].size() > 0)
		{
			if (m_LayerRebars[i].begin()->vecDir == 0)
			{
				inArc = *(m_LayerRebars[i].begin());
				break;
			}
		}
	}
	EditElementHandle arceeh;
	ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(outArc.ptstr, outArc.ptmid, outArc.ptend), true, *ACTIVEMODEL);

	EditElementHandle arceeh2;
	ArcHandler::CreateArcElement(arceeh2, nullptr, DEllipse3d::FromPointsOnArc(inArc.ptstr, inArc.ptmid, inArc.ptend), true, *ACTIVEMODEL);
	DPoint3d ArcDPs1[2];
	RotMatrix rotM;
	double radius1;
	mdlArc_extract(ArcDPs1, nullptr, nullptr, &m_OutRadius, NULL, &rotM, &m_CenterPt, &arceeh.GetElementDescrP()->el);

	DPoint3d ArcDPs2[2];
	double radius2;
	mdlArc_extract(ArcDPs2, nullptr, nullptr, &m_InRadius, NULL, &rotM, &m_CenterPt, &arceeh2.GetElementDescrP()->el);
	
}
//将所有孔洞和钢筋点转换到ACS坐标系下
void  HoleArcSTRebarAssembly::TransFromRebarPts(vector<RebarPoint>&  rebarPts)
{
	TransformInfo transinfo(GetTrans());
	for (int i = 0; i < rebarPts.size(); i++)
	{
		if (rebarPts.at(i).vecDir == 0)//x轴方向为弧形钢筋
		{
			EditElementHandle arceeh;
			DPoint3d ptstr = rebarPts.at(i).ptstr;
			DPoint3d ptend = rebarPts.at(i).ptend;
			DPoint3d ptmid = rebarPts.at(i).ptmid;
			ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(ptstr, ptmid, ptend), true, *ACTIVEMODEL);
			arceeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(arceeh, transinfo);
			//arceeh.AddToModel();
			DEllipse3d ellipseRev;
			mdlArc_extractDEllipse3d(&ellipseRev, &arceeh.GetElementDescrP()->el);

			ellipseRev.EvaluateEndPoints(ptstr, ptend);
			double dLen = ellipseRev.ArcLength();
			mdlElmdscr_pointAtDistance(&ptmid, NULL, dLen / 2, arceeh.GetElementDescrP(), 1e-6);
			rebarPts.at(i).ptstr = ptstr;
			rebarPts.at(i).ptmid = ptmid;
			rebarPts.at(i).ptend = ptend;
		}
		else
		{
			DPoint3d ptstr = rebarPts.at(i).ptstr;
			DPoint3d ptend = rebarPts.at(i).ptend;
			EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			//eeh.AddToModel();
			if (SUCCESS == mdlElmdscr_extractEndPoints(&ptstr, nullptr, &ptend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef()))
			{
				rebarPts.at(i).ptstr = ptstr;
				rebarPts.at(i).ptend = ptend;
			}
		}


	}

}

void HoleArcSTRebarAssembly::CalculateArcLenth(DPoint3d& ptstr,DPoint3d& ptend,double L0Lenth,
	Direction dir, PosValue Pvalue,double uor_per_mm, TransformInfo transinfo)
{//计算弧形部分钢筋长度
	DPoint3d vecLine = ptstr - ptend;
	vecLine.Normalize();
	vecLine.Scale((L0Lenth - 10) * uor_per_mm);
	ptend = ptstr;
	ptend.Add(vecLine);


	if (ptend.x < Pvalue.minx&&dir == Direction::Left)
	{
		ptend.x = Pvalue.minx;
	}
	if (ptend.x > Pvalue.maxx&&dir == Direction::Right)
	{
		ptend.x = Pvalue.maxx;
	}

	EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
	eehline.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline, transinfo);
	DPoint3d tmppts[2];
	mdlLinear_extract(tmppts, NULL, eehline.GetElementCP(), ACTIVEMODEL);
	ptstr = tmppts[0];
	ptend = tmppts[1];
	CalculateIntersetPtWithHolesWithRebarCuve(ptstr, ptend, ptstr, m_useHoleehs);
	//eehline.AddToModel();
}

void HoleArcSTRebarAssembly::CreateURebars(DPoint3d pt, double diameter, double diameterU, double L0Lenth, double distance, double uor_per_mm,
	TransformInfo transinfo, DPoint3d minP, DPoint3d maxP, DPoint3d ptstr, Direction dir, PosValue PvalueF, PosValue PvalueB,
	double bendLen, double bendRadius, RebarEndTypes endTypes, vector<PITRebarCurve>&   rebarCurvesNum, bool ishaveTwinBar)
{
	
	DPoint3d pts[4];
	if (dir == Direction::Left || dir == Direction::Right)
	{
		distance = m_OutRadius - m_InRadius;
		/*if (pt.x <Pvalue.minx + 1.0 || pt.x>Pvalue.maxx - 1.0)
		{
			return;
		}*/

		int twinNeg = -1;
		if (ishaveTwinBar)
		{
			twinNeg = 1;
		}
		int Neg = 1;
		if (dir == Direction::Left)
		{
			Neg = -1;
		}

		pts[0].z = ptstr.z + (diameter / 2 + diameterU / 2)*twinNeg;
		pts[0].y = ptstr.y + (diameterU / 2 - diameter / 2);
		pts[0].x = pt.x + (bendRadius + 10* uor_per_mm) *Neg + diameterU / 2 * Neg;

		pts[1] = pts[0];
		pts[1].x = pts[1].x - (bendRadius + 1 * uor_per_mm) *Neg;

		pts[2] = pts[1];
		pts[2].y = pts[2].y + distance - diameterU + 1.5*diameter;

		pts[3] = pts[2];
		pts[3].x = pts[3].x + (bendRadius + 1 * uor_per_mm) *Neg;

		
		DPoint3d ptstr = pts[0];
		DPoint3d ptend = pts[1];

		DPoint3d ptstrB = pts[3];
		DPoint3d ptendB = pts[2];
		
		CalculateArcLenth(ptstr, ptend, L0Lenth, dir, PvalueF,uor_per_mm,transinfo);
		CalculateArcLenth(ptstrB, ptendB, L0Lenth, dir, PvalueB,uor_per_mm, transinfo);
	
		
		EditElementHandle eeh2;
		LineStringHandler::CreateLineStringElement(eeh2, nullptr, pts, 4, true, *ACTIVEMODEL);
		eeh2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh2, transinfo);
		//eeh2.AddToModel();
		vector<DPoint3d> dpts;
		ExtractLineStringPoint(eeh2.GetElementDescrP(), dpts);
		CalculateIntersetPtWithHolesWithRebarCuve(dpts[2], dpts[3], dpts[2], m_useHoleehs);
		vector<CPoint3D> cpts;
		ExchangePoints(dpts, cpts);

		PITRebarCurve pitcurve;
		pitcurve.makeURebarCurve(cpts, bendRadius);
		PIT::PITRebarCurve lastCurve;
		{
			DVec3d m_rebarVec_V;
			DPoint3d vec1 = dpts[0] - dpts[1];
			DPoint3d vec2 = dpts[2] - dpts[1];
			vec2.CrossProduct(vec2, vec1);
			vec2.Normalize();
			m_rebarVec_V = DVec3d::From(vec2.x, vec2.y, vec2.z);

			DPlane3d faceH = DPlane3d::FromOriginAndNormal(dpts[1], m_rebarVec_V);
			faceH.ProjectPoint(m_CenterPt, m_CenterPt);

			
			PIT::PITRebarCurve tmpCurve;
			CalculateArcCuve(tmpCurve, dpts[0], dpts[1],ptend.Distance(ptstr), m_CenterPt, m_rebarVec_V, true);
			PIT::PITRebarCurve tmpCurveEnd;
			CalculateArcCuve(tmpCurveEnd, dpts[3], dpts[2], ptendB.Distance(ptstrB), m_CenterPt, m_rebarVec_V, false);
			for (int i = 0; i < tmpCurve.GetVertices().GetSize(); i++)
			{
				RebarVertexP vex;
				RebarVertexP Tmpvex = &tmpCurve.PopVertices().At(i);
				vex = &lastCurve.PopVertices().NewElement();
				*vex = *Tmpvex;
				if (i == 0)
				{
					vex->SetType(RebarVertex::kStart);
				}
				else
				{
					vex->SetType(RebarVertex::kIP);
				}
			}
			for (int i = 0; i < pitcurve.GetVertices().GetSize(); i++)
			{
				RebarVertexP vex;
				RebarVertexP Tmpvex = &pitcurve.PopVertices().At(i);
				vex = &lastCurve.PopVertices().NewElement();
				*vex = *Tmpvex;
				vex->SetType(RebarVertex::kIP);
			}
			for (int i = 0; i < tmpCurveEnd.GetVertices().GetSize(); i++)
			{
				RebarVertexP vex;
				RebarVertexP Tmpvex = &tmpCurveEnd.PopVertices().At(i);
				vex = &lastCurve.PopVertices().NewElement();
				*vex = *Tmpvex;
				if (i == tmpCurveEnd.GetVertices().GetSize() - 1)
				{
					vex->SetType(RebarVertex::kEnd);
				}
				else
				{
					vex->SetType(RebarVertex::kIP);
				}
			}
		}
		rebarCurvesNum.push_back(lastCurve);
		//eeh2.AddToModel();
		/*	RebarCurve     rebarCurves;
			makeRebarCurve(rebarCurves, bendRadius, bendLen, endTypes, ptstr, ptend);
			rebarCurvesNum.push_back(rebarCurves);*/
	}
	else
	{
	    distance = m_OutRadius - m_InRadius - 2 * diameter;
		/*if (pt.z <Pvalue.minz + 1.0 || pt.z>Pvalue.maxz - 1.0)
		{
			return;
		}*/
		int twinNeg = 1;
		if (ishaveTwinBar)
		{
			twinNeg = -1;
		}
		int Neg = 1;
		if (dir == Direction::Down)
		{
			Neg = -1;
		}
		//孔洞上边U形筋生成
		pts[0].x = ptstr.x + (diameter / 2 + diameterU / 2)*twinNeg;
		pts[0].y = ptstr.y + (diameterU / 2 - diameter / 2);
		pts[0].z = pt.z + L0Lenth * uor_per_mm*Neg + diameterU / 2 * Neg;

		pts[1] = pts[0];
		pts[1].z = pts[1].z - L0Lenth * uor_per_mm*Neg;

		pts[2] = pts[1];
		pts[2].y = pts[2].y + distance - diameterU + diameter;

		pts[3] = pts[2];
		pts[3].z = pts[3].z + L0Lenth * uor_per_mm*Neg;
		if (pts[0].z < PvalueF.minz&&dir == Direction::Down)
		{
			pts[0].z = PvalueF.minz;
			pts[3].z = PvalueF.minz;
		}
		if (pts[0].z > PvalueF.maxz&&dir == Direction::Up)
		{
			pts[0].z = PvalueF.maxz;
			pts[3].z = PvalueF.maxz;
		}
		EditElementHandle eeh2;
		LineStringHandler::CreateLineStringElement(eeh2, nullptr, pts, 4, true, *ACTIVEMODEL);
		eeh2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh2, transinfo);

		vector<DPoint3d> dpts;
		ExtractLineStringPoint(eeh2.GetElementDescrP(), dpts);
		CalculateIntersetPtWithHolesWithRebarCuve(dpts[0], dpts[1], dpts[1], m_useHoleehs);
		CalculateIntersetPtWithHolesWithRebarCuve(dpts[2], dpts[3], dpts[2], m_useHoleehs);
		vector<CPoint3D> cpts;
		ExchangePoints(dpts, cpts);

		PITRebarCurve pitcurve;
		pitcurve.makeURebarCurve(cpts, bendRadius);
		rebarCurvesNum.push_back(pitcurve);
		//eeh2.AddToModel();
	}

}
void HoleArcSTRebarAssembly::CreateSideRebars(vector<DPoint3d>& pts, int sideNum, double sideSpacing, double LaLenth,
	TransformInfo transinfo, DPoint3d minP, DPoint3d maxP, double diameter, double diameterSide, double uor_per_mm, Direction dir, 
	PosValue PvalueF, PosValue PvalueB,
	double bendLen, double bendRadius, RebarEndTypes endTypes, vector<PITRebarCurve>&   rebarCurvesNum)
{
	double frontDiam = 0;
	if (m_FrontdiameterSide != 0)
	{
		diameterSide = (diameterSide > m_FrontdiameterSide) ? diameterSide : m_FrontdiameterSide;
		frontDiam = diameterSide;
	}

	if (dir == Direction::Left || dir == Direction::Right)
	{

		double originX;
		double minX, maxX;
		int i = 0; int Neg = 1;
		for (DPoint3d pt : pts)
		{
			if (i == 0)
			{
				minX = pt.x;
				maxX = pt.x;
				i++;
			}
			else
			{
				if (minX > pt.x)
					minX = pt.x;
				if (maxX < pt.x)
				{
					maxX = pt.x;
				}
			}
		}
		/*if (minX <Pvalue.minx + 1.0 || maxX>Pvalue.maxx - 1.0)
		{
			return;
		}*/
		if (dir == Direction::Left)
		{
			originX = minX;
			Neg = -1;
		}
		else
		{
			originX = maxX;
		}

		for (int n = 1; n < sideNum + 1; n++)
		{
			double tmpSpacing = n * sideSpacing;
			DPoint3d tmpstr, tmpend;
			tmpstr.z = minP.z - LaLenth * uor_per_mm;
			tmpstr.x = originX + diameter * Neg + 1.2*diameterSide * Neg;
			if (m_FrontdiameterSide != 0)
			{
				if (sideNum > 1)
				{
					tmpstr.y = pts[0].y + tmpSpacing - m_Maindiameter / 2 - m_FrontMaindiameter / 2 + (frontDiam + diameterSide) / 2
						+ (m_FrontMaindiameter + m_Maindiameter)*n / (sideNum + 1);
				}
				else
				{
					tmpstr.y = pts[0].y + tmpSpacing + frontDiam / 2 + diameterSide / 2;
				}

			}
			else
			{
				tmpstr.y = pts[0].y + tmpSpacing;
			}
			tmpend = tmpstr;
			tmpend.z = maxP.z + LaLenth * uor_per_mm;
			if (tmpstr.z < PvalueF.minz)
			{
				tmpstr.z = PvalueF.minz;
			}
			if (tmpend.z > PvalueF.maxz)
			{
				tmpend.z = PvalueF.maxz;
			}
			EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(tmpstr, tmpend), true, *ACTIVEMODEL);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			mdlElmdscr_extractEndPoints(&tmpstr, nullptr, &tmpend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef());

			DPoint3d midPos = tmpstr;
			midPos.Add(tmpend);
			midPos.Scale(0.5);

			DPoint3d dstr = tmpstr;
			DPoint3d dend = tmpend;

			CalculateIntersetPtWithHolesWithRebarCuve(dstr, dend, midPos, m_useHoleehs);
			
			PITRebarCurve pitcurve;
			pitcurve.makeRebarCurve(bendRadius, bendLen, endTypes, CPoint3D(dstr), CPoint3D(dend));
			rebarCurvesNum.push_back(pitcurve);
		}
	}
	else
	{
		double originZ;
		double minZ, maxZ;
		int i = 0; int Neg = 1;
		for (DPoint3d pt : pts)
		{
			if (i == 0)
			{
				minZ = pt.z;
				maxZ = pt.z;
				i++;
			}
			else
			{
				if (minZ > pt.z)
					minZ = pt.z;
				if (maxZ < pt.z)
				{
					maxZ = pt.z;
				}
			}
		}
		/*if (minZ <Pvalue.minz + 1.0 || maxZ>Pvalue.maxz - 1.0)
		{
			return;
		}*/
		if (dir == Direction::Down)
		{
			originZ = minZ;
			Neg = -1;
		}
		else
		{
			originZ = maxZ;
		}

		for (int n = 1; n < sideNum + 1; n++)
		{
			double tmpSpacing = n * sideSpacing;
			DPoint3d tmpstr, tmpend;
			tmpstr.x = minP.x - LaLenth * uor_per_mm ;
			tmpstr.z = originZ + diameter * Neg + diameterSide / 2 * Neg;
			if (m_FrontdiameterSide != 0)
			{

				if (sideNum > 1)
				{
					tmpstr.y = pts[0].y + tmpSpacing - m_Maindiameter / 2 - m_FrontMaindiameter / 2 + (frontDiam + diameterSide) / 2
						+ (m_FrontMaindiameter + m_Maindiameter)*n / (sideNum + 1) - diameterSide;
				}
				else
				{
					tmpstr.y = pts[0].y + tmpSpacing + frontDiam / 2 + diameterSide / 2 - diameterSide;
				}
			}
			else
			{
				tmpstr.y = pts[0].y + tmpSpacing - diameterSide;
			}


			tmpend = tmpstr;
			tmpend.x = maxP.x + LaLenth * uor_per_mm;

			double minxx = (PvalueF.minx + PvalueB.minx) / 2;
			double maxx = (PvalueF.maxx + PvalueB.maxx) / 2;

			if (tmpstr.x < minxx)
			{
				tmpstr.x = minxx;
			}
			if (tmpend.x > maxx)
			{
				tmpend.x = maxx;
			}
			EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(tmpstr, tmpend), true, *ACTIVEMODEL);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			mdlElmdscr_extractEndPoints(&tmpstr, nullptr, &tmpend, nullptr, eeh.GetElementDescrP(), eeh.GetModelRef());
			DPoint3d midPos = tmpstr;
			midPos.Add(tmpend);
			midPos.Scale(0.5);

			DPoint3d dstr = tmpstr;
			DPoint3d dend = tmpend;

			CalculateIntersetPtWithHolesWithRebarCuve(dstr, dend, midPos, m_useHoleehs);
			
			DVec3d m_rebarVec_V = DVec3d::From(0,0,1);
			DPlane3d faceH = DPlane3d::FromOriginAndNormal(dstr, m_rebarVec_V);
			faceH.ProjectPoint(m_CenterPt, m_CenterPt);
			PIT::PITRebarCurve tmpCurve;
			CalculateArcCuve(tmpCurve, dend , dstr, -dend.Distance(dstr), m_CenterPt, m_rebarVec_V, true);

			//PITRebarCurve pitcurve;
			//pitcurve.makeRebarCurve(bendRadius, bendLen, endTypes, CPoint3D(dstr), CPoint3D(dend));
			rebarCurvesNum.push_back(tmpCurve);
		}
	}



}
RebarSetTag* HoleArcSTRebarAssembly::MakeRebars
(
	double Wthickness,
	HoleRebarInfo::ReinForcingInfo tmpinfo,
	vector<string>& holenames,
	vector<RebarPoint>&  rebarPtsF,
	vector<RebarPoint>&  rebarPtsB,
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	DgnModelRefP        modelRef,
	bool                flag 
)
{
	int layernow = 0;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30

	if (rebarPtsF.size() < 1 || rebarPtsB.size() < 1)
	{
		return nullptr;
	}
	vector<RebarPoint> tmprebarPtsF;
	tmprebarPtsF.insert(tmprebarPtsF.begin(), rebarPtsF.begin(), rebarPtsF.end());
	TransFromRebarPts(tmprebarPtsF);
	vector<RebarPoint> tmprebarPtsB;
	tmprebarPtsB.insert(tmprebarPtsB.begin(), rebarPtsB.begin(), rebarPtsB.end());
	TransFromRebarPts(tmprebarPtsB);
	double distance;
	if (tmprebarPtsB.at(0).vecDir == 0)//横向钢筋
	{
		EditElementHandle arceeh;
		ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(tmprebarPtsB.at(0).ptstr, tmprebarPtsB.at(0).ptmid, tmprebarPtsB.at(0).ptend), true, *ACTIVEMODEL);

		EditElementHandle arceeh2;
		ArcHandler::CreateArcElement(arceeh2, nullptr, DEllipse3d::FromPointsOnArc(tmprebarPtsF.at(0).ptstr, tmprebarPtsF.at(0).ptmid, tmprebarPtsF.at(0).ptend), true, *ACTIVEMODEL);
		DPoint3d ArcDPs1[2];
		RotMatrix rotM;
		double radius1;
		mdlArc_extract(ArcDPs1, nullptr, nullptr, &radius1, NULL, &rotM, nullptr, &arceeh.GetElementDescrP()->el);

		DPoint3d ArcDPs2[2];
		double radius2;
		mdlArc_extract(ArcDPs2, nullptr, nullptr, &radius2, NULL, &rotM, nullptr, &arceeh2.GetElementDescrP()->el);

		distance = abs(radius1 - radius2);
	}
	else
	{
		double dis1 = tmprebarPtsB.at(0).ptstr.Distance(tmprebarPtsF.at(0).ptstr);
		double dis2 = tmprebarPtsB.at(0).ptstr.Distance(tmprebarPtsF.at(0).ptend);

		distance = (dis1 < dis2) ? dis1 : dis2;
		if (distance > Wthickness)
		{
			distance = Wthickness - g_wallRebarInfo.concrete.postiveCover*uor_per_mm - g_wallRebarInfo.concrete.reverseCover*uor_per_mm - 2 * diameter;
		}
	}
	bool const isStirrup = false;
	ElementId tmpID = rebarSetId;
	rebarSetId = 0;
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	switch (0)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeStart.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (0)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeEnd.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}
	//	}
	
	double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100



	BrString          sizeKeyTwin(m_twinbarinfo.rebarSize);
	sizeKeyTwin = sizeKeyTwin + "C";
	double Twindiameter = RebarCode::GetBarDiameter(sizeKeyTwin, modelRef);


	//计算U形钢筋直径和LO长度
	BrString          sizeKeyU;
	double diameterU, L0Lenth, bendRadiusU, bendLenU;
	double LaLenth, diameterSide, bendRadiusS, bendLenS;
	BrString          sizeKeyS;
	
	if (diameter / uor_per_mm > 25)
	{
		diameterU = RebarCode::GetBarDiameter("20C", modelRef);
		L0Lenth = g_globalpara.m_laplenth["20C"];
		bendRadiusU = RebarCode::GetPinRadius("20C", modelRef, false);
		bendLenU = RebarCode::GetBendLength("20C", endTypeStart, modelRef);
		sizeKeyU = "20C";
	}
	else
	{
		diameterU = RebarCode::GetBarDiameter("16C", modelRef);
		L0Lenth = g_globalpara.m_laplenth["16C"];
		bendRadiusU = RebarCode::GetPinRadius("16C", modelRef, false);
		bendLenU = RebarCode::GetBendLength("16C", endTypeStart, modelRef);
		sizeKeyU = "16C";
	}
	if (tmpinfo.Hsize > 300 && tmpinfo.Hsize < 500)
	{
		diameterSide = RebarCode::GetBarDiameter("20C", modelRef);;
		LaLenth = g_globalpara.m_alength["20C"];
		bendRadiusS = RebarCode::GetPinRadius("20C", modelRef, false);
		bendLenS = RebarCode::GetBendLength("20C", endTypeStart, modelRef);
		sizeKeyS = "20C";
	}
	else
	{
		diameterSide = diameter;
		LaLenth = g_globalpara.m_alength[(string)sizeKey];
		bendRadiusS = bendRadius;
		bendLenS = bendLen;
		sizeKeyS = sizeKey;
	}

	if (m_FrontdiameterSide > diameterSide)
	{
		int tmpSize = (int)(m_FrontdiameterSide / uor_per_mm);
		char tmpchar[256];
		sprintf(tmpchar, "%d", tmpSize);
		sizeKeyS = tmpchar;
	}
	
	
	//计算侧向构造筋直径和LA长度

	

	//计算侧向构造筋根数

	int sideNum = 0;//侧面构造筋根数
	Wthickness = (Wthickness + diameter) / uor_per_mm;
	distance = distance / uor_per_mm;
	if (Wthickness < 400)
	{
		sideNum = 0;
	}
	else if (Wthickness >= 400 && Wthickness < 700)
	{
		sideNum = 1;
	}
	else if (Wthickness >= 700 && Wthickness < 1000)
	{
		sideNum = 2;
	}
	else if (Wthickness >= 1000 && Wthickness <= 1200)
	{
		sideNum = 3;
	}
	else
	{
		sideNum = (int)(distance / 250);
	}

	double sideSpacing = (distance / (sideNum + 1))*uor_per_mm;


	vector<PITRebarCurve>     rebarCurvesNumU;
	vector<PITRebarCurve>     rebarCurvesNumS;
	vector<int>producerebar;
	producerebar = Getproducerebar();
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
	if (tmprebarPtsF.size() > 0)
	{
		layernow = tmprebarPtsF[0].Layer;
		DPoint3d minP = { 0 }, maxP = { 0 };
		EditElementHandle eehHole, eehHolePlus;
		double hsize;
		HoleRFRebarAssembly::GetUnionHoleeeh(&eehHole, holenames, hsize, GetTrans(), m_holeidAndmodel);
		eehHolePlus.Duplicate(eehHole);
		PlusSideCover(eehHolePlus, g_wallRebarInfo.concrete.sideCover*uor_per_mm, GetTrans());

		TransformInfo transinfo1(GetTrans());
		eehHole.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHole, transinfo1);
		eehHolePlus.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHolePlus, transinfo1);
		mdlElmdscr_computeRange(&minP, &maxP, eehHolePlus.GetElementDescrP(), NULL);


		int minXID = -1; int maxXID = -1;
		HoleArcRFRebarAssembly::GetIDSIntesectWithHole(minXID, maxXID, eehHolePlus, tmprebarPtsF);
		PosValue PvalueF, PvalueB;
		if (minXID == -1 || maxXID == -1)
		{
			return false;
		}
		Transform inversMat = GetTrans();
		inversMat.InverseOf(inversMat);
		TransformInfo transinfo(inversMat);
		if (tmprebarPtsF[0].vecDir == 0)//X方向钢筋
		{
			PvalueF.minx = (tmprebarPtsF.begin()->ptstr.x < tmprebarPtsF.begin()->ptend.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.begin()->ptend.x;
			PvalueF.maxx = (tmprebarPtsF.begin()->ptstr.x > tmprebarPtsF.begin()->ptend.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.begin()->ptend.x;
			PvalueF.minz = (tmprebarPtsF.begin()->ptstr.z < tmprebarPtsF.rbegin()->ptstr.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.rbegin()->ptstr.z;
			PvalueF.maxz = (tmprebarPtsF.begin()->ptstr.z > tmprebarPtsF.rbegin()->ptstr.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.rbegin()->ptstr.z;
			PvalueF.minz = PvalueF.minz - diameter / 2;
			PvalueF.maxz = PvalueF.maxz + diameter / 2;

			PvalueB.minx = (tmprebarPtsB.begin()->ptstr.x < tmprebarPtsB.begin()->ptend.x) ? tmprebarPtsB.begin()->ptstr.x : tmprebarPtsB.begin()->ptend.x;
			PvalueB.maxx = (tmprebarPtsB.begin()->ptstr.x > tmprebarPtsB.begin()->ptend.x) ? tmprebarPtsB.begin()->ptstr.x : tmprebarPtsB.begin()->ptend.x;
			PvalueB.minz = (tmprebarPtsB.begin()->ptstr.z < tmprebarPtsB.rbegin()->ptstr.z) ? tmprebarPtsB.begin()->ptstr.z : tmprebarPtsB.rbegin()->ptstr.z;
			PvalueB.maxz = (tmprebarPtsB.begin()->ptstr.z > tmprebarPtsB.rbegin()->ptstr.z) ? tmprebarPtsB.begin()->ptstr.z : tmprebarPtsB.rbegin()->ptstr.z;
			PvalueB.minz = PvalueB.minz - diameter / 2;
			PvalueB.maxz = PvalueB.maxz + diameter / 2;

			vector<DPoint3d> Letfpts, Rightpts;
			int tmpi = 0;
			for (RebarPoint rebrptmin : tmprebarPtsF)
			{
				bool ishavetwin = false;
				if (m_twinbarinfo.hasTwinbars)
				{
					if (tmpi % (m_twinbarinfo.interval + 1) == 0)
					{
						ishavetwin = true;
					}
				}


				if (rebrptmin.ptstr.z<minP.z || rebrptmin.ptstr.z >maxP.z)
				{
					tmpi++;
					continue;
				}
				vector<DPoint3d> ItrPts;
				DPoint3d ptstr = rebrptmin.ptstr;
				DPoint3d ptend = rebrptmin.ptend;
				DPoint3d ptmid = rebrptmin.ptmid;
				//GetIntersectPointsWithHole(ItrPts, &eehHolePlus, ptstr, ptend);


				vector<EditElementHandle*> useholes;
				useholes.push_back(&eehHolePlus);
				GetARCIntersectPointsWithHoles(ItrPts, useholes, ptstr, ptend, ptmid);

				if (ItrPts.size() == 1)//只有一个交点
				{
					DPoint3d pt = ItrPts.at(0);
					if (pt.x > (minP.x + maxP.x) / 2 && tmpinfo.h4)//交点在右边
					{
						//CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							//transinfo, minP, maxP, rebrptmin.ptstr, Direction::Right, Pvalue,
							//bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, pt, Direction::Right, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinh4)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Right, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}

						Rightpts.push_back(pt);
					}
					else if (tmpinfo.h3)//交点在左边
					{
						/*CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Left, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);*/
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, pt, Direction::Left, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						Letfpts.push_back(pt);
						if (ishavetwin&&tmpinfo.have_twinh3)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Left, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}

				}
				else if (ItrPts.size() == 2)
				{
					DPoint3d ptleft, ptright;
					if (ItrPts[0].x > ItrPts[1].x)
					{
						ptleft = ItrPts[1];
						ptright = ItrPts[0];
					}
					else
					{
						ptleft = ItrPts[0];
						ptright = ItrPts[1];
					}
					Letfpts.push_back(ptleft);
					Rightpts.push_back(ptright);
					if (tmpinfo.h3)
					{
						/*CreateURebars(ptleft, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Left, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);*/
						if (producerebar[1])
						{
							CreateURebars(ptleft, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, ptleft, Direction::Left, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinh3)//画并筋的U形筋
						{
							DPoint3d twinPt = ptleft;
							DPoint3d tmpptstr = rebrptmin.ptstr;
							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Left, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
						if (tmpinfo.h4)
						{
							////孔洞右边U形筋生成
							//CreateURebars(ptright, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							//	transinfo, minP, maxP, rebrptmin.ptstr, Direction::Right, Pvalue,
							//	bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
							if (producerebar[1])
							{
								CreateURebars(ptright, diameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, ptright, Direction::Right, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
							}
							if (ishavetwin&&tmpinfo.have_twinh4)//画并筋的U形筋
							{
								DPoint3d twinPt = ptright;
								DPoint3d tmpptstr = rebrptmin.ptstr;

								twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
								tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
								if (producerebar[1])
								{
									CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
										transinfo, minP, maxP, tmpptstr, Direction::Right, PvalueF, PvalueB,
										bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
								}
							}
						}

					}
					tmpi++;
				}
				if (Letfpts.size() > 0 && tmpinfo.h3)//左边的侧面构造筋
				{
					m_Maindiameter = diameter;
					if (producerebar[0])
					{
						CreateSideRebars(Letfpts, sideNum, sideSpacing, LaLenth, transinfo, minP, maxP, diameterU, diameterSide, uor_per_mm, Direction::Left, PvalueF, PvalueB
							, bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
					}
				}
				if (Rightpts.size() > 0 && tmpinfo.h4)
				{
					m_Maindiameter = diameter;
					if (producerebar[0])
					{
						CreateSideRebars(Rightpts, sideNum, sideSpacing, LaLenth, transinfo, minP, maxP, diameterU, diameterSide, uor_per_mm, Direction::Right, PvalueF, PvalueB
							, bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
					}
				}
				if (m_FrontdiameterSide == 0.0)
				{
					m_FrontMaindiameter = diameter;
					m_FrontdiameterSide = diameterSide;
				}
			
		}
		else//Z方向钢筋
		{
			PvalueF.minx = (tmprebarPtsF.begin()->ptstr.x < tmprebarPtsF.begin()->ptend.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.begin()->ptend.x;
			PvalueF.maxx = (tmprebarPtsF.begin()->ptstr.x > tmprebarPtsF.begin()->ptend.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.begin()->ptend.x;
			PvalueF.minz = (tmprebarPtsF.begin()->ptstr.z < tmprebarPtsF.rbegin()->ptstr.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.rbegin()->ptstr.z;
			PvalueF.maxz = (tmprebarPtsF.begin()->ptstr.z > tmprebarPtsF.rbegin()->ptstr.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.rbegin()->ptstr.z;
			PvalueF.minz = PvalueF.minz - diameter / 2;
			PvalueF.maxz = PvalueF.maxz + diameter / 2;

			PvalueB.minx = (tmprebarPtsB.begin()->ptstr.x < tmprebarPtsB.begin()->ptend.x) ? tmprebarPtsB.begin()->ptstr.x : tmprebarPtsB.begin()->ptend.x;
			PvalueB.maxx = (tmprebarPtsB.begin()->ptstr.x > tmprebarPtsB.begin()->ptend.x) ? tmprebarPtsB.begin()->ptstr.x : tmprebarPtsB.begin()->ptend.x;
			PvalueB.minz = (tmprebarPtsB.begin()->ptstr.z < tmprebarPtsB.rbegin()->ptstr.z) ? tmprebarPtsB.begin()->ptstr.z : tmprebarPtsB.rbegin()->ptstr.z;
			PvalueB.maxz = (tmprebarPtsB.begin()->ptstr.z > tmprebarPtsB.rbegin()->ptstr.z) ? tmprebarPtsB.begin()->ptstr.z : tmprebarPtsB.rbegin()->ptstr.z;
			PvalueB.minz = PvalueB.minz - diameter / 2;
			PvalueB.maxz = PvalueB.maxz + diameter / 2;
			vector<DPoint3d> Uppts, Downpts;
			int tmpi = 0;
			for (int i = 0; i < tmprebarPtsF.size(); i++)
			{


				if (i<minXID || i>maxXID)
				{
					tmpi++;
					continue;
				}
				RebarPoint rebrptmin = tmprebarPtsF.at(i);
				if (rebrptmin.ptstr.x<minP.x || rebrptmin.ptstr.x >maxP.x)
				{
					tmpi++;
					continue;
				}
				bool ishavetwin = false;
				if (m_twinbarinfo.hasTwinbars)
				{
					if (tmpi % (m_twinbarinfo.interval + 1) == 0)
					{
						ishavetwin = true;
					}
				}

				vector<DPoint3d> ItrPts;
				DPoint3d ptstr = rebrptmin.ptstr;
				DPoint3d ptend = rebrptmin.ptend;
				GetIntersectPointsWithHole(ItrPts, holenames, m_holeidAndmodel, ptstr, ptend, transinfo1, g_wallRebarInfo.concrete.sideCover*uor_per_mm);

				if (ItrPts.size() == 1)//只有一个交点
				{
					DPoint3d pt = ItrPts.at(0);
					if (pt.z > (minP.z + maxP.z) / 2 && tmpinfo.v1)//交点在上边
					{
						Uppts.push_back(pt);
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Up, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv1)//画并筋的U形筋
						{

							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, tmpptstr, Direction::Up, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
						}
					}
					else if (tmpinfo.v2)//交点在下边
					{
						Downpts.push_back(pt);
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Down, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv2)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Down, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}

					}

				}
				else if (ItrPts.size() == 2)
				{
					DPoint3d ptdown, ptup;
					if (ItrPts[0].z > ItrPts[1].z)
					{
						ptdown = ItrPts[1];
						ptup = ItrPts[0];
					}
					else
					{
						ptdown = ItrPts[0];
						ptup = ItrPts[1];
					}
					Downpts.push_back(ptdown);
					Uppts.push_back(ptup);
					if (tmpinfo.v1)
					{
						if (producerebar[1])
						{
							CreateURebars(ptup, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Up, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv1)//画并筋的U形筋
						{
							DPoint3d twinPt = ptup;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Up, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
					if (tmpinfo.v2)
					{
						if (producerebar[1])
						{
							CreateURebars(ptdown, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Down, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv2)//画并筋的U形筋
						{
							DPoint3d twinPt = ptdown;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Down, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}

				}

				tmpi++;
			}
			if (Uppts.size() > 0 && tmpinfo.v1)//上边的侧面构造筋
			{
				m_Maindiameter = diameter;
				if (producerebar[0])
				{
					CreateSideRebars(Uppts, sideNum, sideSpacing, LaLenth, transinfo, minP, maxP,
						diameterU, diameterSide, uor_per_mm, Direction::Up, PvalueF, PvalueB
						, bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (Downpts.size() > 0 && tmpinfo.v2)
			{
				m_Maindiameter = diameter;
				if (producerebar[0])
				{
					CreateSideRebars(Downpts, sideNum, sideSpacing, LaLenth,
						transinfo, minP, maxP, diameterU, diameterSide, uor_per_mm, Direction::Down, PvalueF, PvalueB
						, bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (m_FrontdiameterSide == 0.0)
			{
				m_FrontMaindiameter = diameter;
				m_FrontdiameterSide = diameterSide;
			}
		}

	}



	RebarSymbology symb;
	{
		string str(sizeKeyU);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_CAVE_REBAR);
	}
	int	numRebar = (int)rebarCurvesNumU.size();
	numRebar = numRebar + (int)rebarCurvesNumS.size();
	int j = 0;
	char nowLayerc[256];
	sprintf(nowLayerc, "%d", layernow);
	string nowLayerstring(nowLayerc);
	for (PITRebarCurve rebarCurve : rebarCurvesNumU)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);


		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKeyU);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameterU, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string strname = tmpinfo.Hname;
			string Stype = strname + "/HoleStructualfrontRebar" + "_URebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			int num = Gettypenum();
			SetRebarLevelItemTypeValue(tmprebar, nowLayerstring, num, Stype, modelRef);
			//SetRebarLevelItemTypeValue(tmprebar,Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);


		}
		j++;
	}
	for (PITRebarCurve rebarCurve : rebarCurvesNumS)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);




		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKeyS);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameterSide, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string strname = tmpinfo.Hname;
			string Stype = strname + "/HoleStructualMidRebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			int num = Gettypenum();
			SetRebarLevelItemTypeValue(tmprebar, nowLayerstring, num, Stype, modelRef);
			//SetRebarLevelItemTypeValue(tmprebar,Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(200);
	setdata.SetAverageSpacing(200);


	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}
RebarSetTag* HoleArcSTRebarAssembly::MakeRebars
(
	double Wthickness,
	HoleRebarInfo::ReinForcingInfo tmpinfo,
	EditElementHandle& eehHole,
	EditElementHandle& eehHolePlus,
	vector<RebarPoint>&  rebarPtsF,
	vector<RebarPoint>&  rebarPtsB,
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	DgnModelRefP        modelRef,
	bool                flag 
)
{
	int layernow = 0;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30


	if (rebarPtsF.size() < 1 || rebarPtsB.size() < 1)
	{
		return nullptr;
	}
	vector<RebarPoint> tmprebarPtsF;
	tmprebarPtsF.insert(tmprebarPtsF.begin(), rebarPtsF.begin(), rebarPtsF.end());
	TransFromRebarPts(tmprebarPtsF);
	vector<RebarPoint> tmprebarPtsB;
	tmprebarPtsB.insert(tmprebarPtsB.begin(), rebarPtsB.begin(), rebarPtsB.end());
	TransFromRebarPts(tmprebarPtsB);
	double distance ;
	if (tmprebarPtsB.at(0).vecDir==0)//横向钢筋
	{
		EditElementHandle arceeh;
		ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(tmprebarPtsB.at(0).ptstr, tmprebarPtsB.at(0).ptmid, tmprebarPtsB.at(0).ptend), true, *ACTIVEMODEL);
	
		EditElementHandle arceeh2;
		ArcHandler::CreateArcElement(arceeh2, nullptr, DEllipse3d::FromPointsOnArc(tmprebarPtsF.at(0).ptstr, tmprebarPtsF.at(0).ptmid, tmprebarPtsF.at(0).ptend), true, *ACTIVEMODEL);
		DPoint3d ArcDPs1[2];
		RotMatrix rotM;
		double radius1;
		mdlArc_extract(ArcDPs1, nullptr, nullptr, &radius1, NULL, &rotM, nullptr, &arceeh.GetElementDescrP()->el);

		DPoint3d ArcDPs2[2];
		double radius2;
		mdlArc_extract(ArcDPs2, nullptr, nullptr, &radius2, NULL, &rotM, nullptr, &arceeh2.GetElementDescrP()->el);

		distance = abs(radius1 - radius2);
	}
	else
	{
		double dis1 = tmprebarPtsB.at(0).ptstr.Distance(tmprebarPtsF.at(0).ptstr);
		double dis2 = tmprebarPtsB.at(0).ptstr.Distance(tmprebarPtsF.at(0).ptend);

		distance = (dis1 < dis2) ? dis1 : dis2;
		if (distance>Wthickness)
		{
			distance = Wthickness - g_wallRebarInfo.concrete.postiveCover*uor_per_mm - g_wallRebarInfo.concrete.reverseCover*uor_per_mm - 2 * diameter;
		}
	}


	bool const isStirrup = false;
	ElementId tmpID = rebarSetId;
	rebarSetId = 0;
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	switch (0)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeStart.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (0)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeEnd.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}
	//	}
	


	double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
	BrString          sizeKeyTwin(m_twinbarinfo.rebarSize);
	sizeKeyTwin = sizeKeyTwin + "C";
	double Twindiameter = RebarCode::GetBarDiameter(sizeKeyTwin, modelRef);


	//计算U形钢筋直径和LO长度
	BrString          sizeKeyU;
	double diameterU, L0Lenth, bendRadiusU, bendLenU;
	double LaLenth, diameterSide, bendRadiusS, bendLenS;
	BrString          sizeKeyS;
	
	if (diameter / uor_per_mm > 25)
	{
		diameterU = RebarCode::GetBarDiameter("20C", modelRef);;
		L0Lenth = g_globalpara.m_laplenth["20C"];
		bendRadiusU = RebarCode::GetPinRadius("20C", modelRef, false);
		bendLenU = RebarCode::GetBendLength("20C", endTypeStart, modelRef);
		sizeKeyU = "20C";
	}
	else
	{
		diameterU = RebarCode::GetBarDiameter("16C", modelRef);;
		L0Lenth = g_globalpara.m_laplenth["16C"];
		bendRadiusU = RebarCode::GetPinRadius("16C", modelRef, false);
		bendLenU = RebarCode::GetBendLength("16C", endTypeStart, modelRef);
		sizeKeyU = "16C";
	}

	//计算侧向构造筋直径和LA长度

	if (tmpinfo.Hsize > 300 && tmpinfo.Hsize < 500)
	{
		diameterSide = RebarCode::GetBarDiameter("20C", modelRef);;
		LaLenth = g_globalpara.m_alength["20C"];
		bendRadiusS = RebarCode::GetPinRadius("20C", modelRef, false);
		bendLenS = RebarCode::GetBendLength("20C", endTypeStart, modelRef);
		sizeKeyS = "20C";
	}
	else
	{
		diameterSide = diameter;
		LaLenth = g_globalpara.m_alength[(string)sizeKey];
		bendRadiusS = bendRadius;
		bendLenS = bendLen;
		sizeKeyS = sizeKey;
	}

	if (m_FrontdiameterSide > diameterSide)
	{
		int tmpSize = (int)(m_FrontdiameterSide / uor_per_mm);
		char tmpchar[256];
		sprintf(tmpchar, "%d", tmpSize);
		sizeKeyS = tmpchar;
	}
	
	

	//计算侧向构造筋根数

	int sideNum = 0;//侧面构造筋根数
	Wthickness = (Wthickness + diameter) / uor_per_mm;
	distance = distance / uor_per_mm;
	if (Wthickness < 400)
	{
		sideNum = 0;
	}
	else if (Wthickness >= 400 && Wthickness < 700)
	{
		sideNum = 1;
	}
	else if (Wthickness >= 700 && Wthickness < 1000)
	{
		sideNum = 2;
	}
	else if (Wthickness >= 1000 && Wthickness <= 1200)
	{
		sideNum = 3;
	}
	else
	{
		sideNum = (int)(distance / 250);
	}

	double sideSpacing = (distance / (sideNum + 1))*uor_per_mm;

	vector<PITRebarCurve>     rebarCurvesNumU;
	vector<PITRebarCurve>     rebarCurvesNumS;
	vector<int> producerebar;
	producerebar = Getproducerebar();
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
	if (tmprebarPtsF.size() > 0)
	{
		layernow = tmprebarPtsF[0].Layer;
		DPoint3d minP = { 0 }, maxP = { 0 };
		mdlElmdscr_computeRange(&minP, &maxP, eehHolePlus.GetElementDescrP(), NULL);
		int minXID = -1; int maxXID = -1;
		//GetRebarIDSIntesectWithHole(minXID, maxXID, eehHolePlus, tmprebarPtsF);
		HoleArcRFRebarAssembly::GetIDSIntesectWithHole(minXID, maxXID, eehHolePlus, tmprebarPtsF);
		PosValue PvalueF, PvalueB;
		if (minXID == -1 || maxXID == -1)
		{
			return false;
		}

		Transform inversMat = GetTrans();
		inversMat.InverseOf(inversMat);
		TransformInfo transinfo(inversMat);
		if (tmprebarPtsF[0].vecDir == 0)//X方向钢筋
		{
			PvalueF.minx = (tmprebarPtsF.begin()->ptstr.x < tmprebarPtsF.begin()->ptend.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.begin()->ptend.x;
			PvalueF.maxx = (tmprebarPtsF.begin()->ptstr.x > tmprebarPtsF.begin()->ptend.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.begin()->ptend.x;
			PvalueF.minz = (tmprebarPtsF.begin()->ptstr.z < tmprebarPtsF.rbegin()->ptstr.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.rbegin()->ptstr.z;
			PvalueF.maxz = (tmprebarPtsF.begin()->ptstr.z > tmprebarPtsF.rbegin()->ptstr.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.rbegin()->ptstr.z;
			PvalueF.minz = PvalueF.minz - diameter / 2;
			PvalueF.maxz = PvalueF.maxz + diameter / 2;

			PvalueB.minx = (tmprebarPtsB.begin()->ptstr.x < tmprebarPtsB.begin()->ptend.x) ? tmprebarPtsB.begin()->ptstr.x :tmprebarPtsB.begin()->ptend.x;
			PvalueB.maxx = (tmprebarPtsB.begin()->ptstr.x > tmprebarPtsB.begin()->ptend.x) ? tmprebarPtsB.begin()->ptstr.x :tmprebarPtsB.begin()->ptend.x;
			PvalueB.minz = (tmprebarPtsB.begin()->ptstr.z < tmprebarPtsB.rbegin()->ptstr.z) ? tmprebarPtsB.begin()->ptstr.z : tmprebarPtsB.rbegin()->ptstr.z;
			PvalueB.maxz = (tmprebarPtsB.begin()->ptstr.z > tmprebarPtsB.rbegin()->ptstr.z) ? tmprebarPtsB.begin()->ptstr.z : tmprebarPtsB.rbegin()->ptstr.z;
			PvalueB.minz = PvalueB.minz - diameter / 2;
			PvalueB.maxz = PvalueB.maxz + diameter / 2;

			vector<DPoint3d> Letfpts, Rightpts;
			int tmpi = 0;
			for (RebarPoint rebrptmin : tmprebarPtsF)
			{
				bool ishavetwin = false;
				if (m_twinbarinfo.hasTwinbars)
				{
					if (tmpi % (m_twinbarinfo.interval + 1) == 0)
					{
						ishavetwin = true;
					}
				}


				if (rebrptmin.ptstr.z<minP.z || rebrptmin.ptstr.z >maxP.z)
				{
					tmpi++;
					continue;
				}
				vector<DPoint3d> ItrPts;
				DPoint3d ptstr = rebrptmin.ptstr;
				DPoint3d ptend = rebrptmin.ptend;
				DPoint3d ptmid = rebrptmin.ptmid;
				//GetIntersectPointsWithHole(ItrPts, &eehHolePlus, ptstr, ptend);

				
				vector<EditElementHandle*> useholes;
				useholes.push_back(&eehHolePlus);
				GetARCIntersectPointsWithHoles(ItrPts, useholes, ptstr, ptend, ptmid);

				if (ItrPts.size() == 1)//只有一个交点
				{
					DPoint3d pt = ItrPts.at(0);
					if (pt.x > (minP.x + maxP.x) / 2 && tmpinfo.h4)//交点在右边
					{
						//CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							//transinfo, minP, maxP, rebrptmin.ptstr, Direction::Right, Pvalue,
							//bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, pt, Direction::Right, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinh4)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Right, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}

						Rightpts.push_back(pt);
					}
					else if (tmpinfo.h3)//交点在左边
					{
						/*CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Left, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);*/
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, pt, Direction::Left, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						Letfpts.push_back(pt);
						if (ishavetwin&&tmpinfo.have_twinh3)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Left, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}

				}
				else if (ItrPts.size() == 2)
				{
					DPoint3d ptleft, ptright;
					if (ItrPts[0].x > ItrPts[1].x)
					{
						ptleft = ItrPts[1];
						ptright = ItrPts[0];
					}
					else
					{
						ptleft = ItrPts[0];
						ptright = ItrPts[1];
					}
					Letfpts.push_back(ptleft);
					Rightpts.push_back(ptright);
					if (tmpinfo.h3)
					{
						/*CreateURebars(ptleft, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Left, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);*/
						if (producerebar[1])
						{
							CreateURebars(ptleft, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, ptleft, Direction::Left, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinh3)//画并筋的U形筋
						{
							DPoint3d twinPt = ptleft;
							DPoint3d tmpptstr = rebrptmin.ptstr;
							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Left, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
					if (tmpinfo.h4)
					{
						////孔洞右边U形筋生成
						//CreateURebars(ptright, diameter, diameterU, L0Lenth, distance, uor_per_mm,
						//	transinfo, minP, maxP, rebrptmin.ptstr, Direction::Right, Pvalue,
						//	bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						if (producerebar[1])
						{
							CreateURebars(ptright, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, ptright, Direction::Right, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinh4)//画并筋的U形筋
						{
							DPoint3d twinPt = ptright;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Right, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}

				}
				tmpi++;
			}
			if (Letfpts.size() > 0 && tmpinfo.h3)//左边的侧面构造筋
			{
				m_Maindiameter = diameter;
				if (producerebar[0])
				{
					CreateSideRebars(Letfpts, sideNum, sideSpacing, LaLenth, transinfo, minP, maxP, diameterU,
						diameterSide, uor_per_mm, Direction::Left, PvalueF, PvalueB
						, bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (Rightpts.size() > 0 && tmpinfo.h4)
			{
				m_Maindiameter = diameter;
				if (producerebar[0])
				{
					CreateSideRebars(Rightpts, sideNum, sideSpacing, LaLenth, transinfo, minP, maxP, diameterU,
						diameterSide, uor_per_mm, Direction::Right, PvalueF, PvalueB
						, bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (m_FrontdiameterSide == 0.0)
			{
				m_FrontMaindiameter = diameter;
				m_FrontdiameterSide = diameterSide;
			}

		}
		else//Z方向钢筋
		{
		     PvalueF.minx = (tmprebarPtsF.begin()->ptstr.x < tmprebarPtsF.rbegin()->ptstr.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.rbegin()->ptstr.x;
		     PvalueF.maxx = (tmprebarPtsF.begin()->ptstr.x > tmprebarPtsF.rbegin()->ptstr.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.rbegin()->ptstr.x;
		     PvalueF.minz = (tmprebarPtsF.begin()->ptstr.z < tmprebarPtsF.begin()->ptend.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.begin()->ptend.z;
		     PvalueF.maxz = (tmprebarPtsF.begin()->ptstr.z > tmprebarPtsF.begin()->ptend.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.begin()->ptend.z;
		     PvalueF.minx = PvalueF.minx - diameter / 2;
		     PvalueF.maxx = PvalueF.maxx + diameter / 2;

			 PvalueB.minx = (tmprebarPtsB.begin()->ptstr.x < tmprebarPtsB.rbegin()->ptend.x) ? tmprebarPtsB.begin()->ptstr.x : tmprebarPtsB.rbegin()->ptend.x;
			 PvalueB.maxx = (tmprebarPtsB.begin()->ptstr.x > tmprebarPtsB.rbegin()->ptend.x) ? tmprebarPtsB.begin()->ptstr.x : tmprebarPtsB.rbegin()->ptend.x;
			 PvalueB.minz = (tmprebarPtsB.begin()->ptstr.z < tmprebarPtsB.begin()->ptstr.z) ? tmprebarPtsB.begin()->ptstr.z : tmprebarPtsB.begin()->ptstr.z;
			 PvalueB.maxz = (tmprebarPtsB.begin()->ptstr.z > tmprebarPtsB.begin()->ptstr.z) ? tmprebarPtsB.begin()->ptstr.z : tmprebarPtsB.begin()->ptstr.z;
			 PvalueB.minz = PvalueB.minz - diameter / 2;
			 PvalueB.maxz = PvalueB.maxz + diameter / 2;

			vector<DPoint3d> Uppts, Downpts;
			int tmpi = 0;
			for (RebarPoint rebrptmin : tmprebarPtsF)
			{

				if (rebrptmin.ptstr.x<minP.x || rebrptmin.ptstr.x >maxP.x)
				{
					tmpi++;
					continue;
				}
				bool ishavetwin = false;
				if (m_twinbarinfo.hasTwinbars)
				{
					if (tmpi % (m_twinbarinfo.interval + 1) == 0)
					{
						ishavetwin = true;
					}
				}

				vector<DPoint3d> ItrPts;
				DPoint3d ptstr = rebrptmin.ptstr;
				DPoint3d ptend = rebrptmin.ptend;
				GetIntersectPointsWithHole(ItrPts, &eehHolePlus, ptstr, ptend);

				if (ItrPts.size() == 1)//只有一个交点
				{
					DPoint3d pt = ItrPts.at(0);
					if (pt.z > (minP.z + maxP.z) / 2 && tmpinfo.v1)//交点在上边
					{
						Uppts.push_back(pt);
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Up, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv1)//画并筋的U形筋
						{

							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Up, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
					else if (tmpinfo.v2)//交点在下边
					{
						Downpts.push_back(pt);
						if (producerebar[1])
						{
							CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Down, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv2)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Down, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}

					}

				}
				else if (ItrPts.size() == 2)
				{
					DPoint3d ptdown, ptup;
					if (ItrPts[0].z > ItrPts[1].z)
					{
						ptdown = ItrPts[1];
						ptup = ItrPts[0];
					}
					else
					{
						ptdown = ItrPts[0];
						ptup = ItrPts[1];
					}
					Downpts.push_back(ptdown);
					Uppts.push_back(ptup);
					if (tmpinfo.v1)
					{
						if (producerebar[1])
						{
							CreateURebars(ptup, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Up, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv1)//画并筋的U形筋
						{
							DPoint3d twinPt = ptup;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Up, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}
					if (tmpinfo.v2)
					{
						if (producerebar[1])
						{
							CreateURebars(ptdown, diameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, rebrptmin.ptstr, Direction::Down, PvalueF, PvalueB,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						}
						if (ishavetwin&&tmpinfo.have_twinv2)//画并筋的U形筋
						{
							DPoint3d twinPt = ptdown;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							if (producerebar[1])
							{
								CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
									transinfo, minP, maxP, tmpptstr, Direction::Down, PvalueF, PvalueB,
									bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
							}
						}
					}

				}

				tmpi++;
			}
			if (Uppts.size() > 0 && tmpinfo.v1)//上边的侧面构造筋
			{
				m_Maindiameter = diameter;
				if (producerebar[0])
				{
					CreateSideRebars(Uppts, sideNum, sideSpacing, LaLenth, transinfo, minP, maxP, diameterU, diameterSide,
						uor_per_mm, Direction::Up, PvalueF, PvalueB
						, bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (Downpts.size() > 0 && tmpinfo.v2)
			{
				m_Maindiameter = diameter;
				if (producerebar[0])
				{
					CreateSideRebars(Downpts, sideNum, sideSpacing, LaLenth, transinfo, minP, maxP, diameterU,
						diameterSide, uor_per_mm, Direction::Down, PvalueF, PvalueB
						, bendLenS, bendRadiusS, endTypes, rebarCurvesNumS);
				}
			}
			if (m_FrontdiameterSide == 0.0)
			{
				m_FrontMaindiameter = diameter;
				m_FrontdiameterSide = diameterSide;
			}
		}

	}



	RebarSymbology symb;
	{
		string str(sizeKeyU);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_CAVE_REBAR);
	}
	int	numRebar = (int)rebarCurvesNumU.size();
	numRebar = numRebar + (int)rebarCurvesNumS.size();
	int j = 0;
	char nowLayerc[256];
	sprintf(nowLayerc, "%d", layernow);
	string nowLayerstring(nowLayerc);
	for (PITRebarCurve rebarCurve : rebarCurvesNumU)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);




		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKeyU);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameterU, endTypes, shape, modelRef, false);
			
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string strname = tmpinfo.Hname;
			string Stype = strname + "/HoleStructualfrontRebar" + "_URebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			int num = Gettypenum();
			SetRebarLevelItemTypeValue(tmprebar, nowLayerstring, num, Stype, modelRef);
			//SetRebarLevelItemTypeValue(tmprebar,Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
		j++;
	}
	for (PITRebarCurve rebarCurve : rebarCurvesNumS)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);




		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKeyS);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameterSide, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string strname = tmpinfo.Hname;
			string Stype = strname + "/HoleStructualMidRebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			int num = Gettypenum();
			SetRebarLevelItemTypeValue(tmprebar, nowLayerstring, num, Stype, modelRef);
			//SetRebarLevelItemTypeValue(tmprebar,Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(200);
	setdata.SetAverageSpacing(200);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}
void HoleArcSTRebarAssembly::CalculateTransByFrontPts(HoleRebarInfo::ReinForcingInfo tmpinfo)
{
	if (tmpinfo.isUnionHole)
	{

		string uname(tmpinfo.Hname);
		vector<string> vecUnionchildname;
		for (HoleRebarInfo::ReinForcingInfo tmpinfo : m_vecReinF)
		{
			if (tmpinfo.isUnionChild)
			{
				string tuname(tmpinfo.Uname);
				if (tuname.find(uname) != string::npos)
				{
					string hname(tmpinfo.Hname);
					vecUnionchildname.push_back(hname);
				}
			}
		}
		int findtran = 0;
		for (string tname : vecUnionchildname)
		{
			if (findtran)
			{
				break;
			}
			if (m_holeidAndmodel[tname].ID != 0 && m_holeidAndmodel[tname].tModel != nullptr)
			{
				EditElementHandle eeh(m_holeidAndmodel[tname].ID, m_holeidAndmodel[tname].tModel);
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
				DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());

				DPoint3d ptcenter;
				if (!CHoleRebar_ReinforcingDlg::GetArcCenterPoint(m_rebarPts, ptcenter, &eeh))
				{
					continue;
				}
				ptcenter.z = ptele.z;
				//CVector3D yVec = ptcenter - ptele;
				CVector3D yVec = ptele-ptcenter  ;
				yVec.Normalize();

				CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

				DPoint3d ptStart = ptcenter;
				BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴

				Transform trans;
				placement.AssignTo(trans);
				trans.InverseOf(trans);
				SetTrans(trans);
				findtran = 1;
				break;
			}
		}
	}
	else
	{
		if (m_holeidAndmodel[tmpinfo.Hname].ID != 0 && m_holeidAndmodel[tmpinfo.Hname].tModel != nullptr)
		{
			EditElementHandle eeh(m_holeidAndmodel[tmpinfo.Hname].ID, m_holeidAndmodel[tmpinfo.Hname].tModel);
			ElementCopyContext copier2(ACTIVEMODEL);
			copier2.SetSourceModelRef(eeh.GetModelRef());
			copier2.SetTransformToDestination(true);
			copier2.SetWriteElements(false);
			copier2.DoCopy(eeh);
			DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());

			DPoint3d ptcenter;
			if (!CHoleRebar_ReinforcingDlg::GetArcCenterPoint(m_rebarPts, ptcenter, &eeh))
			{
				return;
			}
			ptcenter.z = ptele.z;
			//CVector3D yVec = ptcenter - ptele;
			CVector3D yVec = ptele - ptcenter;
			yVec.Normalize();

			CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

			DPoint3d ptStart = ptcenter;
			BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴

			Transform trans;
			placement.AssignTo(trans);
			trans.InverseOf(trans);
			SetTrans(trans);
		}

	}
}
long HoleArcSTRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}

bool HoleArcSTRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	SetSelectedModel(ehSel.GetModelRef());
	GetConcreteXAttribute(GetConcreteOwner(), ACTIVEMODEL);

	DgnModelRefP modelRef = ACTIVEMODEL;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_holedlg = new CHoleRebar_StructualDlg;
	m_holedlg->isArcwall = true;
	m_holedlg->SetSelectElement(ehSel);
	m_holedlg->Create(IDD_DIALOG_HoleRebar_Structural, CWnd::FromHandle(mdlNativeWindow_getMainHandle(0)));
	m_holedlg->ShowWindow(SW_SHOW);
	m_holedlg->m_ArcHoleRebar = this;
	return true;
}

bool HoleArcSTRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();
	MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();
	//SetElementXAttribute(ehSel.GetElementId(), g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ehSel.GetModelRef());
	//eeh2.AddToModel();
	return true;
}


void HoleArcSTRebarAssembly::SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas)
{
	int i = 0;
	m_vecDirSize.resize(wallRebarDatas.size());
	m_vecRebarLevel.resize(wallRebarDatas.size());
	for (ConcreteRebar rebwall : wallRebarDatas)
	{
		m_vecDirSize[i] = rebwall.rebarSize;
		m_vecRebarLevel[i] = rebwall.datachange;
		i++;
	}
	
}



