/*--------------------------------------------------------------------------------------+
|
|     $Source: WallRebarAssembly.cpp $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include "RebarDetailElement.h"
#include "HoleRebarAssembly.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "DoorHoleRebarAssembly.h"
#include "resource.h"
#include "WallRebarDlg.h"
#include "ExtractFacesTool.h"
#include "CWallRebarDlg.h"
#include "TieRebar.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "CDoorHoleDlg.h"
#include "XmlHelper.h"
#include "PITRebarCurve.h"
#include "PITMSCECommon.h"
#include "ExtractFacesTool.h"
#include "WallRebarAssembly.h"

DoorHoleRebarAssembly::DoorHoleRebarAssembly(ElementId id, DgnModelRefP modelRef) :
	RebarAssembly(id, modelRef)
{
	m_holedlg = nullptr;
	m_FrontdiameterSide = 0.0;
	Init();
}

void DoorHoleRebarAssembly::Init()
{

}

void DoorHoleRebarAssembly::ClearData()
{

	m_Trans.InitIdentity();
	m_vecDirSize.clear();
	m_LayerRebars.clear();
	m_LayerTwinRebars.clear();
	m_rebarPts.clear();
	m_vecReinF.clear();
	m_holeidAndmodel.clear();
	m_LayerRebars.clear();
	m_holedlg = nullptr;
}

void DoorHoleRebarAssembly::MakeUrebar(EditElementHandleR  eehHole,int& tagID,DgnModelRefP modelRef, 
	RebarSetTagArray& rsetTags,double Wthickness, HoleRebarInfo::ReinForcingInfo tmpinfo,int theSec)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	Transform trans = GetTrans();
	trans.InverseOf(trans);
	EditElementHandle eehHolePlus;//放大后的孔洞
	eehHolePlus.Duplicate(eehHole);
	PlusSideCover(eehHolePlus, g_wallRebarInfo.concrete.sideCover*uor_per_mm, trans,true,Wthickness);
	TransformInfo transinfo = TransformInfo(GetTrans());
	eehHole.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHole, transinfo);
	eehHolePlus.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHolePlus, transinfo);
	//eehHole.AddToModel();
	//eehHolePlus.AddToModel();
	/*if (g_vecTwinBarData.size()> m_vecDirSize.size() / 2)
	{*/

	for (int i = 0; i < m_vecDirSize.size() / 2; ++i)
	{
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
		tag = MakeMainURebars(Wthickness, tmpinfo, eehHole, eehHolePlus, tmpPointsi[theSec], tmpPointsj[theSec], PopvecSetId().at(i), GetvecDirSize().at(i), ACTIVEMODEL);
		if (NULL != tag)
		{
			tag->SetBarSetTag(tagID++);
			rsetTags.Add(tag);
		}

		//		}
	}
}
//创建多个折线钢筋和小的点筋
void DoorHoleRebarAssembly::MakeLineStringsrebar(EditElementHandleR  eehHole, EditElementHandleR  NegeehHole, int& tagID, DgnModelRefP modelRef,
	RebarSetTagArray& rsetTags, double Wthickness, HoleRebarInfo::ReinForcingInfo tmpinfo, int theSec)
{
	
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	
	Transform trans = GetTrans();
	trans.InverseOf(trans);

	EditElementHandle eehHolePlus;//放大后的孔洞
	eehHolePlus.Duplicate(eehHole);
	PlusSideCover(eehHolePlus, g_wallRebarInfo.concrete.sideCover*uor_per_mm, trans,true, 2*Wthickness);

	EditElementHandle negHolePlus;//放大后的孔洞
	negHolePlus.Duplicate(NegeehHole);
	PlusSideCover(negHolePlus, g_wallRebarInfo.concrete.sideCover*uor_per_mm, trans,true,2*Wthickness);

	TransformInfo transinfo = TransformInfo(GetTrans());
	eehHole.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHole, transinfo);
	eehHolePlus.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHolePlus, transinfo);
	NegeehHole.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(NegeehHole, transinfo);
	negHolePlus.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(negHolePlus, transinfo);
	/*eehHole.AddToModel();
	eehHolePlus.AddToModel();
	NegeehHole.AddToModel();
	negHolePlus.AddToModel();*/
	/*if (g_vecTwinBarData.size()> m_vecDirSize.size() / 2)
	{*/

	for (int i = 0; i < m_vecDirSize.size() / 2; ++i)
	{
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
		RebarSetTag* tagD = new RebarSetTag;
		tag = MakeLineStringRebars(Wthickness, tmpinfo, eehHole, eehHolePlus,NegeehHole,negHolePlus, tmpPointsi[theSec], tmpPointsj[theSec], PopvecSetId().at(i), GetvecDirSize().at(i), ACTIVEMODEL, tagD);
		if (NULL != tag && NULL != tagD)
		{
			tag->SetBarSetTag(tagID++);
			rsetTags.Add(tag);

			tagD->SetBarSetTag(tagID++);
			rsetTags.Add(tagD);
		}

		//		}
	}
}
bool DoorHoleRebarAssembly::MakeRebars(DgnModelRefP modelRef)
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

	frontRpts.insert(frontRpts.begin(),m_LayerRebars.begin()->second.begin(), m_LayerRebars.begin()->second.end());
	backRpts.insert(backRpts.begin(), m_LayerRebars.rbegin()->second.begin(), m_LayerRebars.rbegin()->second.end());
	TransFromRebarPts(frontRpts);
	TransFromRebarPts(backRpts);
	double Wthickness = abs(backRpts.at(0).ptstr.y - frontRpts.at(0).ptstr.y) + g_wallRebarInfo.concrete.postiveCover*uor_per_mm+ g_wallRebarInfo.concrete.reverseCover*uor_per_mm;


	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	int tagID = 1;
	

	for (int j = 0; j < m_vecReinF.size(); j++)
	{
		HoleRebarInfo::ReinForcingInfo tmpinfo;
		tmpinfo = m_vecReinF.at(j);
		if (tmpinfo.isGenerate==false)
		{
			continue;
		}
		m_FrontdiameterSide = 0.0;
		tmpinfo.v1 = 1; 
		tmpinfo.v2 = 1;
		tmpinfo.h3 = 1;
		tmpinfo.h4 = 1;
		CalculateTransByFrontPts(tmpinfo);
		TransformInfo transinfo(GetTrans());
		if (m_holeidAndmodel[tmpinfo.Hname].ID == 0 || m_holeidAndmodel[tmpinfo.Hname].tModel == nullptr)
		{
			continue;
		}
		EditElementHandle eehHole(m_holeidAndmodel[tmpinfo.Hname].ID, m_holeidAndmodel[tmpinfo.Hname].tModel);
		EditElementHandle eehNegHole;
		ISolidKernelEntityPtr entity;
		SolidUtil::Convert::ElementToBody(entity, eehHole);
		DRange3d range;
		SolidUtil::GetEntityRange(range, *entity);
		for (std::map<std::string, IDandModelref>::iterator itr = m_NEGholeidAndmodel.begin(); itr != m_NEGholeidAndmodel.end(); itr++)
		{
			EditElementHandle tmpeeh(itr->second.ID, itr->second.tModel);
				ISolidKernelEntityPtr entity2;
				SolidUtil::Convert::ElementToBody(entity2, tmpeeh);
				DRange3d range2;
				BentleyStatus nStatus2 = SolidUtil::GetEntityRange(range2, *entity2);
				if (nStatus2 == SUCCESS)
				{
					if (range2.IntersectsWith(range))
					{
						eehNegHole.Duplicate(tmpeeh);
						break;
					}
				}
				/*if (SUCCESS == SolidUtil::Modify::BooleanIntersect(entity2, &entity, 1))
				{
					eehNegHole.Duplicate(tmpeeh);
					break;
				}*/
		}
		if (!eehNegHole.IsValid()||!eehHole.IsValid())
		{
			return false;
		}

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
		if (SolidUtil::Convert::ElementToBody(entityPtr, eehNegHole) == SUCCESS)
		{
			SolidUtil::Convert::BodyToElement(eehNegHole, *entityPtr, nullptr, *eehNegHole.GetModelRef());
			ElementCopyContext copier2(ACTIVEMODEL);
			copier2.SetSourceModelRef(eehNegHole.GetModelRef());
			copier2.SetTransformToDestination(true);
			copier2.SetWriteElements(false);
			copier2.DoCopy(eehNegHole);
		}
		/*eehHole.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehHole, transinfo);
		eehNegHole.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehNegHole, transinfo);
		eehHole.AddToModel();
		eehNegHole.AddToModel();
		return true;*/
		DPoint3d ptcenter = getCenterOfElmdescr(eehHole.GetElementDescrP());
		int theSec = 0;
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
			else
			{
				GetIntersectPointsWithHole(intresectpts, &eehNegHole, tmpstr, tmpend);
				if (intresectpts.size() > 0)
				{
					break;
				}
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
		GetNowUseHole(eehHole);
		EditElementHandle tmpeeh;
		tmpeeh.Duplicate(eehNegHole);
		MakeUrebar(eehNegHole, tagID, modelRef, rsetTags, Wthickness, tmpinfo,theSec);
		
		MakeLineStringsrebar(eehHole, tmpeeh, tagID, modelRef, rsetTags, Wthickness, tmpinfo, theSec);
		
			//}
			
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
void DoorHoleRebarAssembly::SetLayerRebars()
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
void  DoorHoleRebarAssembly::TransFromRebarPts(vector<RebarPoint>&  rebarPts)
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

void DoorHoleRebarAssembly::ApplyTransToHoles(Transform trans)
{
	for (EditElementHandleP tmpeeh:m_useHoleehs)
	{
		if (tmpeeh)
		{
			tmpeeh->GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(*tmpeeh,TransformInfo(trans));
		}
	}
}

void DoorHoleRebarAssembly::GetNowUseHole(EditElementHandleR eehHole)//获取当前使用的孔洞集合，排除当前的门洞
{
	m_NowuseHoleehs.clear();
	ISolidKernelEntityPtr entity;
	SolidUtil::Convert::ElementToBody(entity, eehHole);
	DRange3d range;
	SolidUtil::GetEntityRange(range, *entity);
	for (EditElementHandleP tmpeeh : m_useHoleehs)
	{
		ISolidKernelEntityPtr entity2;
		SolidUtil::Convert::ElementToBody(entity2, *tmpeeh);
		DRange3d range2;
		BentleyStatus nStatus2 = SolidUtil::GetEntityRange(range2, *entity2);
		if (nStatus2 == SUCCESS)
		{
			if (range2.IntersectsWith(range))
			{
				continue;
			}
		}
		m_NowuseHoleehs.push_back(tmpeeh);
	}
	
	
}


void DoorHoleRebarAssembly::CreateURebars(DPoint3d pt,double diameter,double diameterU,double L0Lenth,double distance,double uor_per_mm,
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

		int twinNeg = -1;
		if (ishaveTwinBar)
		{
			twinNeg = 1;
		}
		int Neg = 1;
		if (dir== Direction::Left)
		{
			Neg = -1;
		}

		pts[0].z = ptstr.z + (diameter/2+diameterU/2)*twinNeg;
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
		if (dpts.size() != 4)
		{
			return;
		}
		CalculateIntersetPtWithHolesWithRebarCuve(dpts[0], dpts[1], dpts[1], m_NowuseHoleehs);
		CalculateIntersetPtWithHolesWithRebarCuve(dpts[2], dpts[3], dpts[2], m_NowuseHoleehs);
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
		pts[0].x = ptstr.x - (diameter/2+diameterU/2)*twinNeg;
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
		CalculateIntersetPtWithHolesWithRebarCuve(dpts[0], dpts[1], dpts[1], m_NowuseHoleehs);
		CalculateIntersetPtWithHolesWithRebarCuve(dpts[2], dpts[3], dpts[2], m_NowuseHoleehs);
		vector<CPoint3D> cpts;
		ExchangePoints(dpts, cpts);

		PITRebarCurve pitcurve;
		pitcurve.makeURebarCurve(cpts, bendRadius);
		rebarCurvesNum.push_back(pitcurve);
		//eeh2.AddToModel();
	}
	
}
void DoorHoleRebarAssembly::CreateSideRebars(vector<DPoint3d>& pts,int sideNum,double sideSpacing,double LaLenth,
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
			CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_NowuseHoleehs);
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
			CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_NowuseHoleehs);
			PITRebarCurve pitcurve;
			pitcurve.makeRebarCurve(bendRadius, bendLen, endTypes, CPoint3D(tmpstr), CPoint3D(tmpend));
			rebarCurvesNum.push_back(pitcurve);
		}
	}


	
}

RebarSetTag* DoorHoleRebarAssembly::MakeMainURebars
(
	double Wthickness,
	HoleRebarInfo::ReinForcingInfo tmpinfo,
	EditElementHandle& eehHole,
	EditElementHandle& eehHolePlus,
	vector<RebarPoint>&  rebarPtsF,
	vector<RebarPoint>&  rebarPtsB,
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	DgnModelRefP        modelRef
)
{



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
	double Twindiameter = RebarCode::GetBarDiameter(sizeKeyTwin, modelRef);


	//计算U形钢筋直径和LO长度
	BrString          sizeKeyU;
	double diameterU, L0Lenth, bendRadiusU, bendLenU;
	Wthickness = Wthickness + diameter;
	if (Wthickness/uor_per_mm < 450)
	{
		diameterU = 12 * uor_per_mm;
		L0Lenth = g_globalpara.m_laplenth["12A"];
		bendRadiusU = RebarCode::GetPinRadius("12A", modelRef, false);
		bendLenU = RebarCode::GetBendLength("12A", endTypeStart, modelRef);
		sizeKeyU = "12A";
	}
	else
	{
		if (diameter/uor_per_mm<=25)
		{
			diameterU = 16 * uor_per_mm;
			L0Lenth = g_globalpara.m_laplenth["16A"];
			bendRadiusU = RebarCode::GetPinRadius("16A", modelRef, false);
			bendLenU = RebarCode::GetBendLength("16A", endTypeStart, modelRef);
			sizeKeyU = "16A";
		}
		else
		{
			diameterU = 20 * uor_per_mm;
			L0Lenth = g_globalpara.m_laplenth["20A"];
			bendRadiusU = RebarCode::GetPinRadius("20A", modelRef, false);
			bendLenU = RebarCode::GetBendLength("20A", endTypeStart, modelRef);
			sizeKeyU = "20A";
		}

		
	}
	vector<PITRebarCurve>     rebarCurvesNumU;
	vector<PITRebarCurve>     rebarCurvesNumS;
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
	if (tmprebarPtsF.size() > 0)
	{
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
			Pvalue.minx = (tmprebarPtsF.begin()->ptstr.x < tmprebarPtsF.begin()->ptend.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.begin()->ptend.x;
			Pvalue.maxx = (tmprebarPtsF.begin()->ptstr.x > tmprebarPtsF.begin()->ptend.x) ? tmprebarPtsF.begin()->ptstr.x : tmprebarPtsF.begin()->ptend.x;
			Pvalue.minz = (tmprebarPtsF.begin()->ptstr.z < tmprebarPtsF.rbegin()->ptstr.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.rbegin()->ptstr.z;
			Pvalue.maxz = (tmprebarPtsF.begin()->ptstr.z > tmprebarPtsF.rbegin()->ptstr.z) ? tmprebarPtsF.begin()->ptstr.z : tmprebarPtsF.rbegin()->ptstr.z;
			Pvalue.minz = Pvalue.minz - diameter / 2;
			Pvalue.maxz = Pvalue.maxz + diameter / 2;
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
						CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Right, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);

						if (ishavetwin&&tmpinfo.have_twinh4)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, tmpptstr, Direction::Right, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
						}

						Rightpts.push_back(pt);
					}
					else if (tmpinfo.h3)//交点在左边
					{
						CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Left, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						Letfpts.push_back(pt);
						if (ishavetwin&&tmpinfo.have_twinh3)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, tmpptstr, Direction::Left, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
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
						CreateURebars(ptleft, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Left, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						if (ishavetwin&&tmpinfo.have_twinh3)//画并筋的U形筋
						{
							DPoint3d twinPt = ptleft;
							DPoint3d tmpptstr = rebrptmin.ptstr;
							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);
							CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, tmpptstr, Direction::Left, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
						}
					}
					if (tmpinfo.h4)
					{
						//孔洞右边U形筋生成
						CreateURebars(ptright, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Right, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						if (ishavetwin&&tmpinfo.have_twinh4)//画并筋的U形筋
						{
							DPoint3d twinPt = ptright;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.z = twinPt.z - (diameter / 2 + Twindiameter / 2);
							tmpptstr.z = tmpptstr.z - (diameter / 2 + Twindiameter / 2);

							CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, tmpptstr, Direction::Right, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
						}
					}

				}
				tmpi++;
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
						CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Up, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						if (ishavetwin&&tmpinfo.have_twinv1)//画并筋的U形筋
						{

							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, tmpptstr, Direction::Up, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
						}
					}
					else if (tmpinfo.v2)//交点在下边
					{
						Downpts.push_back(pt);
						CreateURebars(pt, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Down, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						if (ishavetwin&&tmpinfo.have_twinv2)//画并筋的U形筋
						{
							DPoint3d twinPt = pt;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, tmpptstr, Direction::Down, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
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
						CreateURebars(ptup, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Up, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						if (ishavetwin&&tmpinfo.have_twinv1)//画并筋的U形筋
						{
							DPoint3d twinPt = ptup;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, tmpptstr, Direction::Up, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
						}
					}
					if (tmpinfo.v2)
					{
						CreateURebars(ptdown, diameter, diameterU, L0Lenth, distance, uor_per_mm,
							transinfo, minP, maxP, rebrptmin.ptstr, Direction::Down, Pvalue,
							bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, ishavetwin);
						if (ishavetwin&&tmpinfo.have_twinv2)//画并筋的U形筋
						{
							DPoint3d twinPt = ptdown;
							DPoint3d tmpptstr = rebrptmin.ptstr;

							twinPt.x = twinPt.x + (diameter / 2 + Twindiameter / 2);
							tmpptstr.x = tmpptstr.x + (diameter / 2 + Twindiameter / 2);
							CreateURebars(twinPt, Twindiameter, diameterU, L0Lenth, distance, uor_per_mm,
								transinfo, minP, maxP, tmpptstr, Direction::Down, Pvalue,
								bendLenU, bendRadiusU, endTypes, rebarCurvesNumU, false);
						}
					}

				}

				tmpi++;
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
			string Stype = m_strElmName + "/DoorHolefrontRebar" +"_URebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
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


void DoorHoleRebarAssembly::CtreateLineStringRebarCuve(DPoint3d ptIntersect, DPoint3d ptIntersectNeg, vector<PITRebarCurve>& rebarCurvesNumS,
	double diameterSide,double diameter,double Wthickness,double bendRadiusS,double bendLenS,
	DgnModelRefP modelRef,PosValue Pvalue,DPoint3d maxP,DPoint3d minP,
	double L0Lenth,double LaLenth, Direction dir,bool isNegInFront)
{
	int LRratio = -1;//用来设置左右方向不同时的系数
	if (dir == Direction::Left)
	{
		LRratio = -1;
	}
	else if (dir == Direction::Right)
	{
		LRratio = 1;
	}
	
	int NegFront = -1;//负实体在前边或者在后边不同时的系数
	if (isNegInFront)
	{
		NegFront = 1;
	}

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kCog);
	endTypeEnd.SetType(RebarEndType::kCog);
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };

	RebarEndType endTypeStart2, endTypeEnd2;
	endTypeStart2.SetType(RebarEndType::kNone);
	endTypeEnd2.SetType(RebarEndType::kNone);
	RebarEndTypes   endTypesLine = { endTypeStart2, endTypeEnd2 };


	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	Transform inversMat = GetTrans();
	inversMat.InverseOf(inversMat);
	TransformInfo transinfo(inversMat);
	CMatrix3D   mat;
	mat.AssignFrom(inversMat);

	ptIntersect.y = ptIntersect.y - (diameterSide + diameter) / 2;

	DPoint3d backpt = ptIntersect;
	backpt.y = backpt.y + (Wthickness - 2 * g_wallRebarInfo.concrete.postiveCover*uor_per_mm) + diameterSide + diameter;

	if (dir == Direction::Up)
	{
		ptIntersect.x = ptIntersect.x + diameter;//避开和主筋重叠
		backpt.x = ptIntersect.x;
	}
	else
	{
		ptIntersect.z = ptIntersect.z + diameter;//避开和主筋重叠
		backpt.z = ptIntersect.z;
	}


	if (isNegInFront)
	{
		DPoint3d tmpPoint = backpt;
		backpt = ptIntersect;
		ptIntersect = tmpPoint;
		ptIntersectNeg.y = ptIntersectNeg.y + (Wthickness - 2 * g_wallRebarInfo.concrete.postiveCover*uor_per_mm) + diameterSide + diameter;
	}

	//第一根小钢筋（Y方向钢筋）和点筋,附加U形筋和大点筋
	if (Wthickness / uor_per_mm <= 300)//大于300时，第一根为两个点，此时只有一个点筋
	{
		
		CVector3D   endNormal(-1*NegFront, 0.0, 0.0);//默认朝上
		if (dir == Direction::Left)//左边
		{
			endNormal = CVector3D::From(0, 0, -1 * NegFront);
		}
		else if (dir == Direction::Right)
		{
			endNormal = CVector3D::From(0, 0, NegFront);
		}
		//绘制靠近洞口的折线钢筋
		DPoint3d tmpstr = ptIntersect; DPoint3d tmpend = backpt;
		DPoint3d midPos = tmpstr;
		midPos.SumOf(midPos, tmpend, 0.5);
		CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_NowuseHoleehs,GetTrans());
		PITRebarCurve rebarcurve;
		rebarcurve.makeRebarCurveWithNormal(bendRadiusS, bendLenS, endTypes, tmpstr, tmpend, endNormal,mat);
		rebarCurvesNumS.push_back(rebarcurve);
		


		//绘制点筋
		DPoint3d dptdown = backpt;//负实体上点筋位置
		dptdown.y = dptdown.y + (10+6) * uor_per_mm* NegFront;	
		if (dir == Direction::Up)
		{
			dptdown.z = dptdown.z + (10 + 6) * uor_per_mm;
			dptdown.x = minP.x - L0Lenth * uor_per_mm;
		}
		else
		{
			dptdown.x = dptdown.x + (10 + 6) * uor_per_mm*LRratio;
			dptdown.z = Pvalue.minz;
		}
		
		DPoint3d dptup = dptdown;
		if (dir == Direction::Up)
		{
			dptup.x = maxP.x + L0Lenth * uor_per_mm;
		}
		else
		{
			dptup.z = maxP.z + L0Lenth * uor_per_mm;
		}
		tmpstr = dptdown;  tmpend = dptup;
		midPos = tmpstr;
		midPos.SumOf(midPos, tmpend, 0.5);
		CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_NowuseHoleehs,GetTrans());
		PITRebarCurve rebarcurve2;
		rebarcurve2.makeRebarCurveWithNormal(bendRadiusS, bendLenS, endTypesLine, tmpstr, tmpend, endNormal,mat);
		rebarCurvesNumS.push_back(rebarcurve2);
		

	}
	else//三个点的折线筋，此时有两个点筋
	{

		DPoint3d minLefty = ptIntersect;
		if (dir == Direction::Up)
		{
			minLefty.z = ptIntersectNeg.z + LaLenth * uor_per_mm;
		}
		else
		{
			minLefty.x = ptIntersectNeg.x + LaLenth * uor_per_mm*LRratio;
		}
		
		//绘制linestring的钢筋（3个点折线筋）
		DPoint3d pts[3];
		pts[0] = minLefty;
		pts[1] = ptIntersect;
		pts[2] = backpt;
		DPoint3d tmpstr = minLefty; DPoint3d tmpend = ptIntersect;
		DPoint3d midPos = tmpstr;
		midPos.SumOf(midPos, tmpend, 0.5);
		CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, ptIntersect, m_NowuseHoleehs, GetTrans());
		pts[0] = tmpstr;
		CVector3D   endNormal(-1 * NegFront, 0.0, 0.0);//默认朝上
		if (dir == Direction::Left)//左边
		{
			endNormal = CVector3D::From(0, 0, -1 * NegFront);
		}
		else if (dir == Direction::Right)
		{
			endNormal = CVector3D::From(0, 0, NegFront);
		}
		PITRebarCurve rebarcurve;
		vector<CPoint3D> cvPts;
		cvPts.push_back(pts[0]);
		cvPts.push_back(pts[1]);
		cvPts.push_back(pts[2]);
		rebarcurve.makeURebarWithNormal(cvPts,bendRadiusS, bendLenS, endTypes, endNormal, mat);
		//rebarcurve.EvaluateEndTypes(endTypespit);
		rebarCurvesNumS.push_back(rebarcurve);

		//绘制第一个点筋
		DPoint3d dptdown = backpt;//负实体上点筋位置
		dptdown.y = dptdown.y + (10 + 6) * uor_per_mm*NegFront;
		if (dir == Direction::Up)
		{
			dptdown.z = dptdown.z + (10 + 6) * uor_per_mm;
			dptdown.x = minP.x - L0Lenth * uor_per_mm;
		}
		else
		{
			dptdown.x = dptdown.x + (10 + 6) * uor_per_mm*LRratio;
			dptdown.z = Pvalue.minz;
		}
		

		DPoint3d dptup = dptdown;
		if (dir == Direction::Up)
		{
			dptup.x = maxP.x + L0Lenth * uor_per_mm;
		}
		else
		{
			dptup.z = maxP.z + L0Lenth * uor_per_mm;
		}
		tmpstr = dptdown; tmpend = dptup;
	    midPos = tmpstr;
		midPos.SumOf(midPos, tmpend, 0.5);
		CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_NowuseHoleehs, GetTrans());
		PITRebarCurve rebarcurve2;
	    rebarcurve2.makeRebarCurveWithNormal(bendRadiusS, bendLenS, endTypesLine, tmpstr, tmpend, endNormal, mat);
		rebarCurvesNumS.push_back(rebarcurve2);
		


		//绘制第二个点筋
		DPoint3d dptdown2, dptup2;
		dptdown2 = dptdown;
		dptup2 = dptup;
		dptdown2.y = ptIntersect.y - (10 + 6)* uor_per_mm*NegFront;
		dptup2.y = dptdown2.y;
		tmpstr = dptdown2; tmpend = dptup2;
		midPos = tmpstr;
		midPos.SumOf(midPos, tmpend, 0.5);
		CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_useHoleehs, GetTrans());
	    PITRebarCurve rebarcurve3;
		rebarcurve3.makeRebarCurveWithNormal(bendRadiusS, bendLenS, endTypesLine, tmpstr, tmpend, endNormal, mat);
		rebarCurvesNumS.push_back(rebarcurve3);
	}


	//靠前边或后边的折线筋
	DPoint3d minLefty = backpt;
	if (dir == Direction::Up)
	{
		minLefty.z = ptIntersectNeg.z + LaLenth * uor_per_mm;
	}
	else
	{
		minLefty.x = ptIntersectNeg.x + LaLenth * uor_per_mm*LRratio;
	}
	
	CVector3D   endNormal(0.0, 0.0, -1.0);//默认朝上
	if (dir == Direction::Up)
	{
		endNormal = CVector3D::From( NegFront, 0, 0);
	}
	else if (dir == Direction::Left)
	{
		endNormal = CVector3D::From(0.0, 0.0,  NegFront);
	}
	else
	{
		endNormal = CVector3D::From(0.0, 0.0,-1*NegFront);
	}
	DPoint3d tmpstr = minLefty; DPoint3d tmpend = backpt;
	DPoint3d midPos = tmpstr;
	midPos.SumOf(midPos, tmpend, 0.5);
	CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_NowuseHoleehs, GetTrans());
	PITRebarCurve rebarcurve;
	rebarcurve.makeRebarCurveWithNormal(bendRadiusS, bendLenS, endTypes, tmpstr, tmpend, endNormal,mat);
	rebarCurvesNumS.push_back(rebarcurve);



}

//添加附加U形筋和附加点筋
void DoorHoleRebarAssembly::CtreateAdditionalRebarCuve
(
	DPoint3d ptIntersect, DPoint3d ptIntersectNeg, vector<PITRebarCurve>& rebarCurvesNumAU,
	vector<PITRebarCurve>& rebarCurvesNumAD, double diameterU, double diameter,
	double Wthickness, double bendRadiusU, double bendLenU,
	DgnModelRefP modelRef, PosValue Pvalue, DPoint3d maxP, DPoint3d minP,
	double L0LenthU, Direction dir, bool isNegInFront,double disB
)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	char tmpstring[256];
	int isizekey = (int)(diameter / uor_per_mm);
	sprintf(tmpstring, "%d", isizekey);
	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
	BrString sizeKey(tmpstring);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
	double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100


	int LRratio = -1;//用来设置左右方向不同时的系数
	if (dir == Direction::Left)
	{
		LRratio = -1;
	}
	else if (dir == Direction::Right)
	{
		LRratio = 1;
	}

	int NegFront = -1;//负实体在前边或者在后边不同时的系数
	if (isNegInFront)
	{
		NegFront = 1;
	}

	Transform inversMat = GetTrans();
	inversMat.InverseOf(inversMat);
	TransformInfo transinfo(inversMat);
	CMatrix3D   mat;
	mat.AssignFrom(inversMat);

	if (dir == Direction::Up)//如果是竖着的钢筋，为第二层钢筋，要移动到第一层去
	{
		ptIntersect.y = ptIntersect.y - (diameter) / 2 - diameter - diameterU;
	}
	else
	{
		ptIntersect.y = ptIntersect.y - (diameter) / 2;
	}
	DPoint3d backpt = ptIntersect;//先计算前边线交点的对面点
	if (dir == Direction::Up)//如果是竖着的钢筋，为第二层钢筋，要移动到第一层去
	{
		backpt.y = backpt.y + (Wthickness - 2 * g_wallRebarInfo.concrete.postiveCover*uor_per_mm) + 2*diameterU;
	}
	else
	{
		backpt.y = backpt.y + (Wthickness - 2 * g_wallRebarInfo.concrete.postiveCover*uor_per_mm);
	}
	
	ptIntersect.y = ptIntersect.y + diameterU / 2;
	backpt.y = backpt.y - diameterU / 2;
	

	

	DPoint3d backptneg;
	if (isNegInFront)//负实体在前面
	{
		DPoint3d tmpPoint = backpt;
		backpt = ptIntersect;
		ptIntersect = tmpPoint;
		ptIntersectNeg.y = ptIntersectNeg.y + (Wthickness - 2 * g_wallRebarInfo.concrete.postiveCover*uor_per_mm)  + diameter;
	}
	//计算B上的背面对应点,离负实体较近的点
	if (dir == Direction::Up)
	{
		backpt = ptIntersect;
		backpt.y = backpt.y - NegFront * (disB - 20 * uor_per_mm - g_wallRebarInfo.concrete.postiveCover*uor_per_mm);
		backptneg = ptIntersectNeg;
		backptneg.y = backptneg.y - NegFront * (disB - 20 * uor_per_mm - g_wallRebarInfo.concrete.postiveCover*uor_per_mm);
	}
	else
	{
		backpt = ptIntersect;
		backpt.y = backpt.y - NegFront * (disB - 20 * uor_per_mm - g_wallRebarInfo.concrete.postiveCover*uor_per_mm - diameterU);
		backptneg = ptIntersectNeg;
		backptneg.y = backptneg.y - NegFront * (disB - 20 * uor_per_mm - g_wallRebarInfo.concrete.postiveCover*uor_per_mm - diameterU);
	}
	

	//画附加U形钢筋
	DPoint3d ptIntersectSide, backptSide;
	if (dir == Direction::Up)
	{
		ptIntersect.x = ptIntersect.x + diameterU/2  + 5*uor_per_mm + diameter;//避开和小U形筋重叠
		backpt.x = ptIntersect.x;
	}
	else
	{
		ptIntersect.z = ptIntersect.z + diameterU / 2 + 5 * uor_per_mm + diameter;//避开和小U形筋重叠
		backpt.z = ptIntersect.z;
	}
	ptIntersectSide = ptIntersect;
	backptSide = backpt;
	
	if (dir == Direction::Up)
	{
		ptIntersectSide.z = ptIntersectNeg.z + L0LenthU * uor_per_mm;//最上边的点		
		backptSide.z = ptIntersectSide.z;
		
		
	}
	else 
	{
		ptIntersectSide.x = ptIntersectNeg.x + LRratio * L0LenthU * uor_per_mm;//最左或最右的点
		backptSide.x = ptIntersectSide.x;
	}
	DPoint3d pt[4];
	pt[0] = ptIntersectSide;
	pt[1] = ptIntersect;
	pt[2] = backpt;
	pt[3] = backptSide;
	
	CalculateIntersetPtWithHolesWithRebarCuve(pt[0], pt[1], pt[1], m_NowuseHoleehs, GetTrans());
	CalculateIntersetPtWithHolesWithRebarCuve(pt[2], pt[3], pt[2], m_NowuseHoleehs, GetTrans());
	PITRebarCurve rebarcurve;
	vector<CPoint3D> ptsc;
	for (int i = 0;i<4;i++)
	{
		ptsc.push_back(pt[i]);
	}

	rebarcurve.makeURebarCurve(ptsc, bendRadiusU);
	rebarcurve.DoMatrix(mat);
	rebarCurvesNumAU.push_back(rebarcurve);

	EditElementHandle eeh;
	LineStringHandler::CreateLineStringElement(eeh, nullptr, pt, 4, true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	//eeh.AddToModel();

	
	//画附加点筋，首先画左右两端的2根点筋；点筋数量最少为2
	//X或Y的偏移量，使得刚好与拐角处相切
	double deltxory = (sqrtl(bendRadiusU* bendRadiusU * 2.0) - bendRadiusU + diameter / 2 + diameterU / 2)*0.707;
	DPoint3d OptInersect = ptIntersect;
	DPoint3d Optbackpt = backpt;
	if (dir==Direction::Up)
	{
		ptIntersect.z = ptIntersect.z +  deltxory;
		backpt.z = ptIntersect.z;
		ptIntersect.y = ptIntersect.y - NegFront * deltxory;
		backpt.y = backpt.y + NegFront * deltxory;
	}
	else
	{
		ptIntersect.x = ptIntersect.x + LRratio * deltxory;
		backpt.x = ptIntersect.x;
		ptIntersect.y = ptIntersect.y - NegFront * deltxory;
		backpt.y = backpt.y + NegFront * deltxory;
	}
	//第一根
	DPoint3d dptdown = ptIntersect;//负实体上点筋位置
	if (dir == Direction::Up)
	{
		dptdown.x = minP.x - L0LenthU * uor_per_mm;
	}
	else
	{
		dptdown.z = Pvalue.minz;
	}


	DPoint3d dptup = dptdown;
	if (dir == Direction::Up)
	{
		dptup.x = maxP.x + L0LenthU * uor_per_mm;
	}
	else
	{
		dptup.z = maxP.z + L0LenthU * uor_per_mm;
	}

	DPoint3d tmpstr = dptdown; DPoint3d tmpend = dptup;
	DPoint3d midPos = tmpstr;
	midPos.SumOf(midPos, tmpend, 0.5);
	CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_NowuseHoleehs, GetTrans());

	PITRebarCurve rebarcurve2;
	CVector3D endNormal(-1, 0, 0);
	rebarcurve2.makeRebarCurveWithNormal(bendRadius, bendLen, endTypes, tmpstr, tmpend, endNormal, mat);
	rebarCurvesNumAD.push_back(rebarcurve2);
	EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(dptdown, dptup), true, *ACTIVEMODEL);
	eehline.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline, transinfo);
	//eehline.AddToModel();
	//第二根
	if (dir == Direction::Up)
	{
		dptdown.z = backpt.z;
		dptdown.y = backpt.y;
		dptup.z = backpt.z;
		dptup.y = backpt.y;
	}
	else
	{
		dptdown.x = backpt.x;
		dptdown.y = backpt.y;
		dptup.x = backpt.x;
		dptup.y = backpt.y;
	}
	tmpstr = dptdown;  tmpend = dptup;
	midPos = tmpstr;
	midPos.SumOf(midPos, tmpend, 0.5);
	CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_NowuseHoleehs, GetTrans());
	PITRebarCurve rebarcurve3;
	rebarcurve3.makeRebarCurveWithNormal(bendRadius, bendLen, endTypes, tmpstr, tmpend, endNormal, mat);
	rebarCurvesNumAD.push_back(rebarcurve3);
	EditElementHandle eehline2;
	LineHandler::CreateLineElement(eehline2, nullptr, DSegment3d::From(dptdown, dptup), true, *ACTIVEMODEL);
	eehline2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline2, transinfo);
	//eehline2.AddToModel();

	//再画多余两根的点筋根据B的大小算：
	//500<=B<700,（3-2）根；
    //700<=B<1000,(4-2)根；
	//1000<=B<1200,(5-2);
	//B>1200时，（B/250+1 -2）根
	{
		 ptIntersect = OptInersect;
		 backpt = Optbackpt;
		int Num = 0;
		double B = disB / uor_per_mm;
		if (B < 700 && B >= 500)
		{
			Num = 1;
		}
		else if (700 <= B && B < 1000)
		{
			Num = 2;
		}
		else if (1000 <= B && B <= 1200)
		{
			Num = 3;
		}
		else if (B > 1200)
		{
			Num = ((int)B / 250 + 1 - 2);
		}
		if (Num > 0)
		{
			Num = 2;
			deltxory = diameterU / 2 + diameter / 2;
			if (dir == Direction::Up)
			{
				OptInersect.z = OptInersect.z + deltxory;
				Optbackpt.z = OptInersect.z;
				OptInersect.y = OptInersect.y - NegFront * deltxory;
				Optbackpt.y = Optbackpt.y + NegFront * deltxory;
			}
			else
			{
				OptInersect.x = OptInersect.x + LRratio * deltxory;
				Optbackpt.x = OptInersect.x;
				OptInersect.y = OptInersect.y - NegFront * deltxory;
				Optbackpt.y = Optbackpt.y + NegFront * deltxory;
			}
			double sideSpacing = OptInersect.Distance(Optbackpt) / (Num + 1);
			DPoint3d tmpstr;
			if (OptInersect.y < Optbackpt.y)
			{
				tmpstr = OptInersect;
			}
			else
			{
				tmpstr = Optbackpt;
			}
			for (int n = 1; n < Num + 1; n++)
			{
				double tmpSpacing = n * sideSpacing;
				DPoint3d  tmpup = tmpstr;
				tmpup.y = tmpSpacing + tmpup.y;
				DPoint3d dptdown = tmpup;//负实体上点筋位置
				if (dir == Direction::Up)
				{
					dptdown.x = minP.x - L0LenthU * uor_per_mm;
				}
				else
				{
					dptdown.z = Pvalue.minz;
				}

				DPoint3d dptup = dptdown;
				if (dir == Direction::Up)
				{
					dptup.x = maxP.x + L0LenthU * uor_per_mm;
				}
				else
				{
					dptup.z = maxP.z + L0LenthU * uor_per_mm;
				}
				tmpstr = dptdown;  tmpend = dptup;
				midPos = tmpstr;
				midPos.SumOf(midPos, tmpend, 0.5);
				CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_NowuseHoleehs, GetTrans());
				PITRebarCurve rebarcurve;
				rebarcurve.makeRebarCurveWithNormal(bendRadius, bendLen, endTypes, tmpstr, tmpend, endNormal, mat);
				rebarCurvesNumAD.push_back(rebarcurve);
			}



		}
	}
	


	//主筋U形筋和附加U形筋之间的点筋，当墙厚>=400时，增设一根，直径同主筋，当水平主筋为2层或者2层以上时，不需要增设
	if (Wthickness/uor_per_mm>=400)
	{
		double dimenterDisU;
		if (diameter / uor_per_mm >= 28)
		{
			dimenterDisU = RebarCode::GetBarDiameter("20A", modelRef);
		}
		else
		{
			dimenterDisU = RebarCode::GetBarDiameter("16A", modelRef);
		}
		deltxory = diameterU / 2 + diameter / 2;
		if (dir == Direction::Up)
		{
			ptIntersect.z = ptIntersectNeg.z + dimenterDisU + diameter/2;
			ptIntersect.z = ptIntersect.z;
			ptIntersect.y = ptIntersect.y - NegFront * deltxory;
			backpt.y = backpt.y + NegFront * deltxory;
		}
		else
		{
			ptIntersect.x = ptIntersectNeg.x + LRratio * (dimenterDisU + diameter / 2);
			backpt.x = ptIntersect.x;
			ptIntersect.y = ptIntersect.y - NegFront * deltxory;
			backpt.y = backpt.y + NegFront * deltxory;
		}
		DPoint3d dptdown = backpt;//负实体上点筋位置
		if (dir == Direction::Up)
		{
			dptdown.x = minP.x - L0LenthU * uor_per_mm;
		}
		else
		{
			dptdown.z = Pvalue.minz;
		}

		DPoint3d dptup = dptdown;
		if (dir == Direction::Up)
		{
			dptup.x = maxP.x + L0LenthU * uor_per_mm;
		}
		else
		{
			dptup.z = maxP.z + L0LenthU * uor_per_mm;
		}
		tmpstr = dptdown;  tmpend = dptup;
		midPos = tmpstr;
		midPos.SumOf(midPos, tmpend, 0.5);
		CalculateIntersetPtWithHolesWithRebarCuve(tmpstr, tmpend, midPos, m_NowuseHoleehs, GetTrans());
		PITRebarCurve rebarcurve;
		rebarcurve.makeRebarCurveWithNormal(bendRadius, bendLen, endTypes, tmpstr, tmpend, endNormal, mat);
		rebarCurvesNumAD.push_back(rebarcurve);

	}
	


}
RebarSetTag* DoorHoleRebarAssembly::MakeLineStringRebars
(
	double Wthickness,
	HoleRebarInfo::ReinForcingInfo tmpinfo,
	EditElementHandle& eehHole,
	EditElementHandle& eehHolePlus,
	EditElementHandleR NegeehHole,
	EditElementHandleR NegHolePlus,
	vector<RebarPoint>&  rebarPtsF,
	vector<RebarPoint>&  rebarPtsB,
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	DgnModelRefP        modelRef,
	RebarSetTag* tagD
)
{

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

	rebarSetId = 1;
	RebarSetP   rebarSetD = RebarSet::Fetch(rebarSetId, modelRef);//直钢筋与U型钢筋不能同时出图，应该分为正面洞口钢筋与中间洞口钢筋，故应使用不同的RebarSetID
	if (NULL == rebarSetD)
		return NULL;
	rebarSetD->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSetD->SetCallerId(GetCallerId());
	rebarSetD->StartUpdate(modelRef);


	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kCog);
	endTypeEnd.SetType(RebarEndType::kCog);
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };

	RebarEndType endTypeStartLine, endTypeEndLine;
	endTypeStartLine.SetType(RebarEndType::kNone);
	endTypeEndLine.SetType(RebarEndType::kNone);
	RebarEndTypes   endTypesLine = { endTypeStartLine, endTypeEndLine };
	

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
	double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100



	BrString          sizeKeyTwin(m_twinbarinfo.rebarSize);
	double Twindiameter = RebarCode::GetBarDiameter(sizeKeyTwin, modelRef);


	//计算小钢筋直径
	double LaLenth, L0Lenth,diameterSide, bendRadiusS, bendLenS;
	BrString          sizeKeyS;
	
	LaLenth = g_globalpara.m_alength["10C"];
	L0Lenth = g_globalpara.m_laplenth["10C"];
	bendRadiusS = RebarCode::GetPinRadius("10C", modelRef, false);
	bendLenS = RebarCode::GetBendLength("10C", endTypeStart, modelRef);
	sizeKeyS = "10C";
	diameterSide = RebarCode::GetBarDiameter("10C", modelRef);
	double LaLenthU, L0LenthU, diameterU, bendRadiusU, bendLenU;
	BrString          sizeKeyU;
	Wthickness = Wthickness + diameter;
	//计算U形钢筋直径和其他参数
	if (Wthickness/uor_per_mm>=450)
	{
		if (diameter/uor_per_mm<=25)
		{
			diameterU = RebarCode::GetBarDiameter("16C", modelRef);
			LaLenthU = g_globalpara.m_alength["16C"];
			L0LenthU = g_globalpara.m_laplenth["16C"];
			bendRadiusU = RebarCode::GetPinRadius("16C", modelRef, false);
			bendLenU = RebarCode::GetBendLength("16C", endTypeStart, modelRef);
			sizeKeyU = "16C";
		}
		else
		{
			diameterU = RebarCode::GetBarDiameter("20C", modelRef);
			LaLenthU = g_globalpara.m_alength["20C"];
			L0LenthU = g_globalpara.m_laplenth["20C"];
			bendRadiusU = RebarCode::GetPinRadius("20C", modelRef, false);
			bendLenU = RebarCode::GetBendLength("20C", endTypeStart, modelRef);
			sizeKeyU = "20C";
		}
	}
	else
	{
		diameterU = RebarCode::GetBarDiameter("12C", modelRef);
		LaLenthU = g_globalpara.m_alength["12C"];
		L0LenthU = g_globalpara.m_laplenth["12C"];
		bendRadiusU = RebarCode::GetPinRadius("12C", modelRef, false);
		bendLenU = RebarCode::GetBendLength("12C", endTypeStart, modelRef);
		sizeKeyU = "12C";
	}
	
	
	

	bool isNegInFront = true;
	//判断负实体是在前边还是后边
	for (RebarPoint rbpt: tmprebarPtsB)
	{
			vector<DPoint3d> ItrPts;
			DPoint3d ptstr = rbpt.ptstr;
			DPoint3d ptend = rbpt.ptend;
			GetIntersectPointsWithHole(ItrPts, &NegeehHole, ptstr, ptend);
			if (ItrPts.size()>0)
			{
				isNegInFront = false;
				break;
			}
	}
	double DisB;
	{//计算B的值
		DPoint3d minP = { 0 }, maxP = { 0 };
		mdlElmdscr_computeRange(&minP, &maxP, NegeehHole.GetElementDescrP(), NULL);
	    DisB = Wthickness - (maxP.y - minP.y);
	}

	vector<PITRebarCurve>     rebarCurvesNumS;
	vector<PITRebarCurve>     rebarCurvesNumU;
	vector<PITRebarCurve>     rebarCurvesNumD;
		if (tmprebarPtsF.size() > 0)
		{
			DPoint3d minP = { 0 }, maxP = { 0 };
			mdlElmdscr_computeRange(&minP, &maxP, eehHolePlus.GetElementDescrP(), NULL);

			/*	DPoint3d minP2 = { 0 }, maxP2 = { 0 };
				mdlElmdscr_computeRange(&minP2, &maxP2, eehHolePlus.GetElementDescrP(), NULL);
	*/
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
					vector<DPoint3d> itrptsneg;
					DPoint3d ptstr = rebrptmin.ptstr;
					DPoint3d ptend = rebrptmin.ptend;
					GetIntersectPointsWithHole(ItrPts, &eehHolePlus, ptstr, ptend);
					ptstr = rebrptmin.ptstr;
					ptend = rebrptmin.ptend;
					GetIntersectPointsWithHole(itrptsneg, &NegHolePlus, ptstr, ptend);				
					if (ItrPts.size() == 1)//只有一个交点
					{
						DPoint3d pt = ItrPts.at(0);
						DPoint3d ptneg = itrptsneg.at(0);
						if (pt.x > (minP.x + maxP.x) / 2)//交点在右边
						{
							CtreateLineStringRebarCuve(pt, ptneg, rebarCurvesNumS, diameterSide, diameter, 
								Wthickness,bendRadiusS,bendLenS, modelRef, Pvalue, maxP, minP, L0Lenth, LaLenth, Direction::Right,isNegInFront);

							CtreateAdditionalRebarCuve(pt, ptneg, rebarCurvesNumU, rebarCurvesNumD,diameterU, diameter,
								Wthickness, bendRadiusU, bendLenU, modelRef, Pvalue, maxP, minP, L0LenthU, Direction::Right, isNegInFront,DisB);

						}
						else //交点在左边
						{
							CtreateLineStringRebarCuve(pt, ptneg, rebarCurvesNumS, diameterSide, diameter,
								Wthickness, bendRadiusS, bendLenS, modelRef, Pvalue, maxP, minP, L0Lenth, LaLenth, Direction::Left, isNegInFront);
							CtreateAdditionalRebarCuve(pt, ptneg, rebarCurvesNumU, rebarCurvesNumD, diameterU, diameter,
								Wthickness, bendRadiusU, bendLenU, modelRef, Pvalue, maxP, minP, L0LenthU, Direction::Left, isNegInFront, DisB);
						}

					}
					else if (ItrPts.size() == 2&&itrptsneg.size()==2)
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

						DPoint3d ptleftNeg, ptrightNeg;
						if (itrptsneg[0].x > itrptsneg[1].x)
						{
							ptleftNeg = itrptsneg[1];
							ptrightNeg = itrptsneg[0];
						}
						else
						{
							ptleftNeg = itrptsneg[0];
							ptrightNeg = itrptsneg[1];
						}
						Letfpts.push_back(ptleft);
						Rightpts.push_back(ptright);	
						//左边
						CtreateLineStringRebarCuve(ptleft, ptleftNeg, rebarCurvesNumS, diameterSide, diameter,
							Wthickness, bendRadiusS, bendLenS, modelRef, Pvalue, maxP, minP, L0Lenth, LaLenth, Direction::Left, isNegInFront);
						CtreateAdditionalRebarCuve(ptleft, ptleftNeg, rebarCurvesNumU, rebarCurvesNumD, diameterU, diameter,
							Wthickness, bendRadiusU, bendLenU, modelRef, Pvalue, maxP, minP, L0LenthU, Direction::Left, isNegInFront, DisB);
						//右边
						CtreateLineStringRebarCuve(ptright, ptrightNeg, rebarCurvesNumS, diameterSide, diameter,
							Wthickness, bendRadiusS, bendLenS, modelRef, Pvalue, maxP, minP, L0Lenth, LaLenth, Direction::Right, isNegInFront);
						CtreateAdditionalRebarCuve(ptright, ptrightNeg, rebarCurvesNumU, rebarCurvesNumD, diameterU, diameter,
							Wthickness, bendRadiusU, bendLenU, modelRef, Pvalue, maxP, minP, L0LenthU, Direction::Right, isNegInFront, DisB);
						

					}
					tmpi++;
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
					vector<DPoint3d> itrptsneg;
					DPoint3d ptstr = rebrptmin.ptstr;
					DPoint3d ptend = rebrptmin.ptend;
					GetIntersectPointsWithHole(ItrPts, &eehHolePlus, ptstr, ptend);
					ptstr = rebrptmin.ptstr;
					ptend = rebrptmin.ptend;
					GetIntersectPointsWithHole(itrptsneg, &NegHolePlus, ptstr, ptend);

					if (ItrPts.size() == 1)//只有一个交点
					{
						DPoint3d pt = ItrPts[0];
						DPoint3d ptneg = itrptsneg[0];
						CtreateLineStringRebarCuve(pt, ptneg, rebarCurvesNumS, diameterSide, diameter,
							Wthickness, bendRadiusS, bendLenS, modelRef, Pvalue, maxP, minP, L0Lenth, LaLenth, Direction::Up, isNegInFront);
						CtreateAdditionalRebarCuve(pt, ptneg, rebarCurvesNumU, rebarCurvesNumD, diameterU, diameter,
							Wthickness, bendRadiusU, bendLenU, modelRef, Pvalue, maxP, minP, L0LenthU, Direction::Up, isNegInFront, DisB);
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
						DPoint3d ptdownNeg, ptupNeg;
						if (itrptsneg[0].z > itrptsneg[1].z)
						{
							ptdownNeg = ItrPts[1];
							ptupNeg = ItrPts[0];
						}
						else
						{
							ptdownNeg = ItrPts[0];
							ptupNeg = ItrPts[1];
						}
						Downpts.push_back(ptdown);
						Uppts.push_back(ptup);
						if (1)//上方
						{
							


							
						}
						

					}

					tmpi++;
				}
			}

		}


	RebarSymbology symb;
	{
		string str(sizeKeyS);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_CAVE_REBAR);
	}
//	int	numRebar = (int)rebarCurvesNumS.size() + (int)rebarCurvesNumU.size()+(int)rebarCurvesNumD.size();
	int	numRebar = (int)rebarCurvesNumS.size() + (int)rebarCurvesNumU.size();
	int j = 0;
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
			string Stype = m_strElmName + "/DoorHolefrontRebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}
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
			string Stype = m_strElmName + "/DoorHolefrontRebar" +"_URebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
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




	int	numRebarD = (int)rebarCurvesNumD.size();
	int a = 0;
	for (PITRebarCurve rebarCurve : rebarCurvesNumD)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		RebarElementP rebarElement = rebarSetD->AssignRebarElement(a, numRebarD, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);	
			RebarEndTypes   endTypes = { endTypeStartLine, endTypeEndLine };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string Stype = m_strElmName + "/DoorHoleMidRebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		a++;
	}

	RebarSetData setdataD;
	setdataD.SetNumber(numRebarD);
	setdataD.SetNominalSpacing(200);
	setdataD.SetAverageSpacing(200);
	int retD = -1;
	retD = rebarSetD->FinishUpdate(setdataD, modelRef);
	tagD->SetRset(rebarSetD);
	tagD->SetIsStirrup(isStirrup);

	return tag;
}
void DoorHoleRebarAssembly::CalculateTransByFrontPts(HoleRebarInfo::ReinForcingInfo tmpinfo)
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
				if (tuname.find(uname)!= string::npos)
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
				DPoint3d ptcenter = getCenterOfElmdescr(eeh.GetElementDescrP());
				for (int i = 0; i < m_vecFrontPts.size() - 1; i++)
				{

					vector<DPoint3d> interpts;
					DPoint3d tmpStr, tmpEnd;
					tmpStr = m_vecFrontPts[i];
					tmpEnd = m_vecFrontPts[i + 1];
					tmpStr.z = tmpEnd.z = ptcenter.z;
					GetIntersectPointsWithHole(interpts, &eeh, tmpStr, tmpEnd);
					if (interpts.size() > 0)
					{
						DPoint3d ptStart = m_vecFrontPts[i];
						DPoint3d ptEnd = m_vecFrontPts[i + 1];

						CVector3D  xVec(ptStart, ptEnd);

						CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);

						CVector3D  xVecNew(ptStart, ptEnd);
						BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
						Transform trans;
						placement.AssignTo(trans);
						trans.InverseOf(trans);
						SetTrans(trans);
						return;
					}
				}

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
			DPoint3d ptcenter = getCenterOfElmdescr(eeh.GetElementDescrP());
			for (int i = 0; i < (int)m_vecFrontPts.size() - 1; i++)
			{

				vector<DPoint3d> interpts;
				DPoint3d tmpStr, tmpEnd;
				tmpStr = m_vecFrontPts[i];
				tmpEnd = m_vecFrontPts[i + 1];
				tmpStr.z = tmpEnd.z = ptcenter.z;
				GetIntersectPointsWithHole(interpts, &eeh, tmpStr, tmpEnd);
				if (interpts.size() > 0)
				{
					DPoint3d ptStart = m_vecFrontPts[i];
					DPoint3d ptEnd = m_vecFrontPts[i + 1];

					CVector3D  xVec(ptStart, ptEnd);

					CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);

					CVector3D  xVecNew(ptStart, ptEnd);
					BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
					Transform trans;
					placement.AssignTo(trans);
					trans.InverseOf(trans);
					SetTrans(trans);
					return;
				}
			}
		}

	}
}
long DoorHoleRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool DoorHoleRebarAssembly::OnDoubleClick()
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
	m_holedlg = new CDoorHoleDlg;
	m_holedlg->SetSelectElement(ehSel);
	m_holedlg->Create(IDD_DIALOG_DOORHOLE, CWnd::FromHandle(mdlNativeWindow_getMainHandle(0)));
	m_holedlg->ShowWindow(SW_SHOW);
	m_holedlg->m_HoleRebar = this;
	return true;
}

bool DoorHoleRebarAssembly::Rebuild()
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


void DoorHoleRebarAssembly::SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas)
{
	int i = 0;
	m_vecDirSize.resize(wallRebarDatas.size());
	for (ConcreteRebar rebwall : wallRebarDatas)
	{
		m_vecDirSize[i++] = rebwall.rebarSize;
	}

}



