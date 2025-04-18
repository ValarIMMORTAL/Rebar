#include "_ustation.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "CFacesRebarDlg.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "PlaneRebarAssembly.h"
#include <CModelTool.h>
#include <CPointTool.h>

#include "PITMSCECommon.h"
#include "XmlHelper.h"
#include "SelectRebarTool.h"
#include "RebarHelper.h"
#include "CFaceTool.h"
#include "CElementTool.h"
#include "WallHelper.h"
#include "SlabHelper.h"
#include "CSolidTool.h"

#define  Waterstops_Width (60*UOR_PER_MilliMeter)

using namespace Gallery;

using namespace PIT;
extern bool FacePreviewButtonsDown;

//平面配筋--begin
PlaneRebarAssembly::PlaneRebarAssembly(ElementId id, DgnModelRefP modelRef) :FacesRebarAssembly(id, modelRef)
{
	m_vecRebarStartEnd.clear();
}

PlaneRebarAssembly::~PlaneRebarAssembly()
{
}

double PlaneRebarAssembly::InsideFace_OffsetLength(DPoint3dCR RebarlineMidPt)
{
	for (auto walleh : m_Allwalls)
	{
		DRange3d tmpWall_Range;
		mdlElmdscr_computeRange(&tmpWall_Range.low, &tmpWall_Range.high, walleh.GetElementDescrCP(), nullptr);
		if (tmpWall_Range.IsContainedXY(RebarlineMidPt))
		{
			// 获取在钢筋断点上面的墙的信息，得到钢筋信息
			vector<PIT::ConcreteRebar>	my_vecRebarData;
			WallRebarInfo my_wallRebarInfo;
			my_wallRebarInfo.concrete.reverseCover = 50;
			ElementId contid = 0;
			if (nullptr != walleh.GetElementRef())
				GetElementXAttribute(walleh.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, walleh.GetModelRef());
			GetElementXAttribute(contid, sizeof(PIT::WallRebarInfo), my_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
			GetElementXAttribute(contid, my_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
			//获取sizekey
			auto it = my_vecRebarData.begin();
			for (; it != my_vecRebarData.end(); it++)
			{
				BrString strRebarSize = it->rebarSize;
				if (strRebarSize != L"")
				{
					if (strRebarSize.Find(L"mm") != -1)
					{
						strRebarSize.Replace(L"mm", L"");
					}
				}
				else
				{
					strRebarSize = XmlManager::s_alltypes[it->rebarType];
				}
				strcpy(it->rebarSize, CT2A(strRebarSize));
				GetDiameterAddType(it->rebarSize, it->rebarType);
			}
			//获取sizekey
			//墙的背面的纵筋直径
			double diameterV = 0.0;
			char rebarSize[512] = "20C";
			diameterV = RebarCode::GetBarDiameter(rebarSize, ACTIVEMODEL);
			if ((contid != 0) && (my_vecRebarData.size() != 0))
				diameterV = RebarCode::GetBarDiameter(my_vecRebarData[my_vecRebarData.size() - 1].rebarSize, ACTIVEMODEL);

			return ((GetConcrete().sideCover + my_wallRebarInfo.concrete.reverseCover) * Get_uor_per_mm + diameterV);
		}

	}
	if (m_zCorner)
	{
		return ((GetConcrete().sideCover) * Get_uor_per_mm);
	}
	return 0;
}


bool PlaneRebarAssembly::makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, const PIT::PITRebarEndTypes& endTypes)
{
	DRange3d range_eeh;
	DRange3d range_CurFace;
	bool isNeedChange = false;//集水坑外侧面是否是否需要变换方向和锚固长度
	if (m_isCatchpit)
	{
		mdlElmdscr_computeRange(&range_eeh.low, &range_eeh.high, m_pOldElm->GetElementDescrCP(), nullptr);
		mdlElmdscr_computeRange(&range_CurFace.low, &range_CurFace.high, m_CurentFace->GetElementDescrCP(), nullptr);
		if (COMPARE_VALUES_EPS(range_eeh.low.z, range_CurFace.low.z, 300) == 0)
			isNeedChange = AnalyseOutsideFace(range_eeh, range_CurFace);
	}
	DPoint3d  startPt = endTypes.beg.GetptOrgin();
	DPoint3d  endPt = endTypes.end.GetptOrgin();
	DPoint3d  ptStartBack = endTypes.beg.GetptOrgin();
	DPoint3d  ptEndBack = endTypes.end.GetptOrgin();
	//确保起点终点是从小到大---begin
	DVec3d vec = endPt - startPt;
	DVec3d vecX = DVec3d::From(1, 0, 0);
	vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
	vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
	vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
	vec.Normalize();

	if (GetIntersectPointsWithNegs(m_Negs, startPt, endPt))
	{
		return false;
	}

	m_vecRebarPtsLayer.push_back(startPt);
	m_vecRebarPtsLayer.push_back(endPt);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetConcrete().sideCover * uor_per_mm;
	vector<DPoint3d> tmppts;
	vector<vector<DPoint3d>> vecPtRebars;
	vector<DPoint3d> tmpptsTmp;
	double bendDistance = dSideCover;
	bool bIntserctSuccess = true;
	bool isSuccess = true;//集水坑有一个内侧面创建失败
	if (m_face.IsValid()) // 与原实体相交(无孔洞)
	{
		//将原平面往法向方向拉伸为一个实体
		EditElementHandle eehSolid;
		ISolidKernelEntityPtr ptarget;
		SolidUtil::Convert::ElementToBody(ptarget, m_face, true, true, true);
		if (SUCCESS == SolidUtil::Modify::ThickenSheet(ptarget, 5000.0 * uor_per_mm, 5000 * uor_per_mm) && !m_zCorner)
		{
			if (SUCCESS == SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL))
			{

				MSElementDescrP mainDP = eehSolid.GetElementDescrP();
				vector<MSElementDescrP> boolDP;
				if (m_TopSlabEdp != nullptr)
				{
					boolDP.push_back(m_TopSlabEdp);
				}
				if (m_BottomSlabEdp != nullptr)
				{
					boolDP.push_back(m_BottomSlabEdp);
				}
				for (size_t i = 0; i < m_vecWallEdp.size(); i++)
				{
					boolDP.push_back(m_vecWallEdp[i]);
				}
				PITCommonTool::CSolidTool::SolidBoolOperation(mainDP, boolDP,
					BOOLOPERATION::BOOLOPERATION_UNITE, ACTIVEMODEL);

				DVec3d vec = endPt - startPt;
				vec.ScaleToLength(5000 * UOR_PER_MilliMeter);
				endPt.Add(vec);
				vec.Negate();
				startPt.Add(vec);
				EditElementHandle eehcombimSolid(mainDP, true, false, ACTIVEMODEL);
				//eehcombimSolid.AddToModel();
				bool result = GetIntersectPointsWithOldElmOwner(tmpptsTmp, &eehcombimSolid, startPt, endPt, dSideCover);
				//if (!result)
				//{
				//	//没有交点证明是超过范围的钢筋,直接缩小保护层
				//	EditElementHandle eehcombimSolid(mainDP, true, false, ACTIVEMODEL);
				//	result = GetIntersectPointsWithOldElmOwner(tmpptsTmp, &eehcombimSolid, startPt, endPt, dSideCover);
				//	ptStartBack = startPt;
				//	ptEndBack = endPt;
				//	//eehSolid.AddToModel();
				//	bool RESULT = GetIntersectPointsWithOldElm(tmpptsTmp, &eehcombimSolid, startPt, endPt, dSideCover);
				//	if (tmpptsTmp.size() > 1)
				//	{
				//		// 存在交点为两个以上的情况
				//		GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, startPt, endPt, &eehcombimSolid, dSideCover);
				//	}
				//}
				//else 
				{
					ptStartBack = startPt;
					ptEndBack = endPt;
					//eehSolid.AddToModel();
					bool RESULT = GetIntersectPointsWithOldElm(tmpptsTmp, &eehcombimSolid, startPt, endPt, dSideCover);

					//不知道为什么有相同的点,先删除
					RemoveRepeatPoint(tmpptsTmp);

					if (tmpptsTmp.size() > 1)
					{
						// 存在交点为两个以上的情况
						GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, startPt, endPt, &eehcombimSolid, dSideCover);
					}
				}


			}
		}
		else
		{
			isSuccess = false;
			//不能转换成实体就直接和原实体进行操作
			ISolidKernelEntityPtr ptarget;
			SolidUtil::Convert::ElementToBody(ptarget, *m_pOldElm, true, true, true);
			EditElementHandle eehSolid;
			SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL);

			//m_pOldElm->GetElementDescrP();
			MSElementDescrP mainDP = eehSolid.GetElementDescrP();
			vector<MSElementDescrP> boolDP;
			if (m_TopSlabEdp != nullptr)
			{
				boolDP.push_back(m_TopSlabEdp);
			}
			if (m_BottomSlabEdp != nullptr)
			{
				boolDP.push_back(m_BottomSlabEdp);
			}
			for (size_t i = 0; i < m_vecWallEdp.size(); i++)
			{
				boolDP.push_back(m_vecWallEdp[i]);
			}

			PITCommonTool::CSolidTool::SolidBoolOperation(mainDP, boolDP,
				BOOLOPERATION::BOOLOPERATION_UNITE, ACTIVEMODEL);
			DVec3d vec = endPt - startPt;
			vec.ScaleToLength(5000 * UOR_PER_MilliMeter);
			endPt.Add(vec);
			vec.Negate();
			startPt.Add(vec);
			EditElementHandle eehcombimSolid(mainDP, true, false, ACTIVEMODEL);
			//eehcombimSolid.AddToModel();
			GetIntersectPointsWithOldElmOwner(tmpptsTmp, &eehcombimSolid, startPt, endPt, dSideCover);
			ptStartBack = startPt;
			ptEndBack = endPt;
			if (eehcombimSolid.IsValid() /*!= NULL*/) // 与原实体相交(无孔洞)
			{
				GetIntersectPointsWithOldElm(tmpptsTmp, &eehcombimSolid, startPt, endPt, dSideCover);
				if (tmpptsTmp.size() > 1)
				{
					///存在交点为两个以上的情况
					GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, startPt, endPt, &eehcombimSolid, dSideCover);
				}
			}

		}
	}



	if (tmpptsTmp.size() < 2)
	{
		vector<DPoint3d> vecPt;
		vecPt.push_back(startPt);
		vecPt.push_back(endPt);

		vecPtRebars.push_back(vecPt);
	}


	/*EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
	eehline.AddToModel();*/

	for (size_t i = 0; i < vecPtRebars.size(); i++)
	{
		startPt = vecPtRebars.at(i).at(0);
		endPt = vecPtRebars.at(i).at(1);
		GetIntersectPointsWithHoles(tmppts, m_useHoleehs, startPt, endPt, dSideCover);
		map<int, DPoint3d> map_pts;
		bool isStr = false;
		for (DPoint3d pt : tmppts)
		{
			if (ExtractFacesTool::IsPointInLine(pt, startPt, endPt, ACTIVEMODEL, isStr))
			{
				int dis = (int)startPt.Distance(pt);
				if (map_pts.find(dis) != map_pts.end())
				{
					dis = dis + 1;
				}
				map_pts[dis] = pt;
			}
		}
		if (map_pts.find(0) != map_pts.end())
		{
			map_pts[1] = startPt;
		}
		else
		{
			map_pts[0] = startPt;
		}
		int dis = (int)startPt.Distance(endPt);
		if (map_pts.find(dis) == map_pts.end())
		{
			map_pts[dis] = endPt;
		}
		else
		{
			dis = dis + 1;
			map_pts[dis] = endPt;
		}
		//特殊集水坑有两个内侧面部分超出的钢筋部分锚入方向错误，当为true是调转锚入方向
		bool begFlag = false;
		bool endFlag = false;

		for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
		{
			map<int, DPoint3d>::iterator itrplus = ++itr;
			itr--;
			PITRebarEndTypes endTypesTmp = endTypes;
			PITRebarCurve rebar;
			RebarVertexP vex;
			DPoint3d ptStart(itr->second);

			DVec3d vec1 = itrplus->second - itr->second;
			vec1.Normalize();
			DVec3d vec2 = itr->second - itrplus->second;
			vec2.Normalize();
			double dis1 = ptStart.Distance(ptStartBack);
			double dvalue = 10;
			int typebeg = endTypesTmp.beg.GetType();
			if (COMPARE_VALUES_EPS(dis1, bendDistance, dvalue) != 0)
			{
				//endTypesTmp.beg.SetType(PITRebarEndType::kNone);
				if (m_solidType == 0)
				{
					GetHoleRebarAnchor(ptStart, itrplus->second, ptStart, endTypesTmp.beg);
					DVec3d vecTemp = DVec3d::From(vec1);
					vecTemp.ScaleToLength(0.5*m_curreDiameter);
					ptStart.Add(vecTemp);

				}
				else
				{
					endTypesTmp.beg.SetType(PITRebarEndType::kNone);
				}

			}
			else
			{
				if (m_sidetype == SideType::Nor  /*&& typebeg == PITRebarEndType::kBend*/)
				{
					if (typebeg == PITRebarEndType::kBend)
					{
						//先缩短0.5个赶紧距离
						vec1.ScaleToLength(0.5*m_curreDiameter);
						ptStart.Add(vec1);
						//end

						if (m_solidType == 1)
						{
							if (m_iCurRebarDir == 1)
							{
								if (m_bUpIsStatr)
								{
									//ptStart.z += m_dUpSlabThickness;
									if (m_wallTopFaceType == 0)
									{
										//ptStart.z -= m_dTopOffset;
										vec1.ScaleToLength(m_dTopOffset);
										ptStart.Add(vec1);
									}
								}
								else
								{
									//ptStart.z -= m_dbottomSlabThickness;
									if (m_wallBottomFaceType == 0)
									{
										//ptStart.z += m_dBottomOffset;
										vec1.ScaleToLength(m_dBottomOffset);
										ptStart.Add(vec1);
									}
								}
							}
							else if (m_iCurRebarDir == 0)
							{
								if (m_bStartType == 0)
								{
									vec2.ScaleToLength(m_dStartOffset);
									ptStart.Add(vec2);
								}
							}
						}
						else
						{
							if (m_verSlabFaceInfo.bStartAnhorsel)
							{
								vec2.ScaleToLength(m_verSlabFaceInfo.dStartanchoroffset);
								ptStart.Add(vec2);
							}
						}
					}
					else if (typebeg == PITRebarEndType::kCustom)
					{
						//斜面配筋弯钩锚入水平面
						vec2.ScaleToLength(dSideCover + m_curreDiameter);
						ptStart.Add(vec2);
						if (m_verSlabFaceInfo.bStartAnhorsel)
						{
							DVec3d vecTemp = vec2;
							vecTemp.ScaleToLength(m_verSlabFaceInfo.dStartanchoroffset);
							ptStart.Add(vecTemp);
							BrString strRebarSize(GetMainRebars().at(i).rebarSize);
							double Radius = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL) / 2;
							if (m_CatchpitType == 1 && GetMainRebars().at(m_curLevel).rebarDir == 0 && (ptStart.z - Radius) > -10400 * uor_per_mm)
							{
								if (ptStart.x < itrplus->second.x && COMPARE_VALUES_EPS(range_eeh.low.z, range_CurFace.low.z, 300) != 0)//特殊集水坑内侧面需要更改
								{
									begFlag = true;
									vec1.Negate();
									ptStart.Add(vec1);
									vec1.ScaleToLength(m_verSlabFaceInfo.dStartanchoroffset + m_curreDiameter / 2);
									ptStart.Add(vec1);
								}
							}

						}
					}


				}
				else if (m_sidetype == SideType::In)
				{
					if (typebeg == PITRebarEndType::kBend)
					{
						//先缩短0.5个赶紧距离
						vec1.ScaleToLength(0.5*m_curreDiameter);
						ptStart.Add(vec1);
						//end
						if (m_insidef.bStartAnhorsel)
						{
							DPoint3d ptTest = ptStart;
							DVec3d vecTest = vec2;
							vecTest.ScaleToLength(m_insidef.dStartanchoroffset);
							ptTest.Add(vecTest);
							if (!ISPointInHoles(m_Holeehs, ptTest))
							{
								vec2.ScaleToLength(m_insidef.dStartanchoroffset);
								ptStart.Add(vec2);
							}
							else
							{
								GetHoleRebarAnchor(ptStart, itrplus->second, ptStart, endTypesTmp.beg);
							}

						}
						else if (m_insidef.calLen > 1.0) //防止没值时把Vec变为0 
						{
							m_insidef.calLen = WallRebars_OffsetLength(ptStart);
							vec1.ScaleToLength(m_insidef.calLen);
							ptStart.Add(vec1);
						}

					}
					else if (typebeg == PITRebarEndType::kCustom)
					{
						if (m_insidef.bStartAnhorsel)
						{
							vec2.ScaleToLength(m_insidef.dStartanchoroffset);
							ptStart.Add(vec2);
						}
					}

				}
				else if (m_sidetype == SideType::Out)
				{
					if (typebeg == PITRebarEndType::kBend)
					{
						//先缩短0.5个赶紧距离
						vec1.ScaleToLength(0.5*m_curreDiameter);
						ptStart.Add(vec1);
						//end
						if (m_outsidef.bStartAnhorsel)
						{
							vec2.ScaleToLength(m_outsidef.dStartanchoroffset);
							ptStart.Add(vec2);
						}
						else if (m_outsidef.calLen > 1.0) //防止反正没值时把Vec变为0 
						{
							vec1.ScaleToLength(m_outsidef.calLen);
							ptStart.Add(vec1);
						}
					}
					else if (typebeg == PITRebarEndType::kCustom)
					{
						if (m_outsidef.bStartAnhorsel)
						{
							vec2.ScaleToLength(m_outsidef.dStartanchoroffset);
							ptStart.Add(vec2);
						}
					}
				}
			}
			//// 集水坑有一个内侧面由于创建实体失败导致与整个补过孔洞的集水坑实体取交点再延长使得钢筋延申出集水坑
			//if (!isSuccess && m_isCatchpit)
			//{
			//	if (GetMainRebars().at(m_curLevel).rebarDir == 0)//水平
			//	{
			//		if (ptStart.y < itrplus->second.y)
			//		{
			//			ptStart.y += m_twoFacesDistance;
			//		}
			//		else
			//		{
			//			ptStart.y -= m_twoFacesDistance;
			//		}
			//	}
			//	else//纵向
			//	{
			//		if (ptStart.z < itrplus->second.z)
			//		{
			//			ptStart.z += m_twoFacesDistance;
			//		}
			//	}
			//	
			//}
			//if (m_CatchpitType == 1 && m_isCatchpit)//特殊的集水坑
			//{
			//	DRange3d solidRange;
			//	mdlElmdscr_computeRange(&solidRange.low, &solidRange.high, m_pOldElm->GetElementDescrCP(), nullptr);
			//	if (GetMainRebars().at(m_curLevel).rebarDir == 1)
			//	{//使得纵向钢筋往上延申到板
			//		double zLength = solidRange.ZLength();
			//		double CoverSize =  GetConcrete().sideCover;
			//		if (ptStart.z > itrplus->second.z && abs(ptStart.z - itrplus->second.z) < zLength - 600 * uor_per_mm - CoverSize * uor_per_mm)
			//			ptStart.z += 600 * uor_per_mm;
			//	}
			//}
			//vex = &rebar.PopVertices().NewElement();
			//vex->SetIP(ptStart);
			//vex->SetType(RebarVertex::kStart);
			//endTypesTmp.beg.SetptOrgin(ptStart);

			vec1 = itrplus->second - itr->second;
			vec1.Normalize();
			vec2 = itr->second - itrplus->second;
			vec2.Normalize();
			if (itrplus == map_pts.end())
			{
				break;
			}

			++itr;

			DPoint3d ptEnd(itrplus->second);
			double dis2 = ptEnd.Distance(ptEndBack);
			int typeend = endTypesTmp.end.GetType();
			if (COMPARE_VALUES_EPS(dis2, bendDistance, dvalue) != 0)
			{
				//endTypesTmp.end.SetType(PITRebarEndType::kNone);
				if (m_solidType == 0)
				{
					GetHoleRebarAnchor(ptEnd, ptStart, ptEnd, endTypesTmp.end);
					DVec3d vecTemp = DVec3d::From(vec2);
					vecTemp.ScaleToLength(0.5*m_curreDiameter);
					ptEnd.Add(vecTemp);
				}
				else
				{
					endTypesTmp.end.SetType(PITRebarEndType::kNone);
				}

			}
			else
			{
				if (m_sidetype == SideType::Nor /*&& typeend == PITRebarEndType::kBend*/)
				{
					if (typeend == PITRebarEndType::kBend)
					{
						//先缩短0.5个赶紧距离
						vec2.ScaleToLength(0.5*m_curreDiameter);
						ptEnd.Add(vec2);
						//end
						if (m_solidType == 1)
						{
							if (m_iCurRebarDir == 1)
							{
								if (!m_bUpIsStatr)
								{
									if (m_wallTopFaceType == 0)
									{
										//ptEnd.z -= m_dTopOffset;
										vec2.ScaleToLength(m_dTopOffset);
										ptEnd.Add(vec2);
									}
								}
								else
								{
									if (m_wallBottomFaceType == 0)
									{
										//ptEnd.z += m_dBottomOffset;
										vec2.ScaleToLength(m_dBottomOffset);
										ptEnd.Add(vec2);
									}
								}
							}
							else if (m_iCurRebarDir == 1)
							{
								if (m_bEndInType == 0)
								{
									vec1.ScaleToLength(m_dEndOffset);
									ptEnd.Add(vec1);
								}
							}
						}
						else
						{
							if (m_verSlabFaceInfo.bEndAnhorsel)
							{
								DVec3d vecTemp = vec1;
								vecTemp.ScaleToLength(m_verSlabFaceInfo.dEndanchoroffset);
								ptEnd.Add(vecTemp);
							}
						}
					}
					else if (typeend == PITRebarEndType::kCustom)
					{
						vec1.ScaleToLength(dSideCover + m_curreDiameter);
						ptEnd.Add(vec1);
						if (m_verSlabFaceInfo.bEndAnhorsel)
						{
							vec1.ScaleToLength(m_verSlabFaceInfo.dEndanchoroffset);
							ptEnd.Add(vec1);

							BrString strRebarSize(GetMainRebars().at(i).rebarSize);
							double Radius = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL) / 2;
							if (m_CatchpitType == 1 && GetMainRebars().at(m_curLevel).rebarDir == 0 && (ptEnd.z - Radius) > -10400 * uor_per_mm)
							{
								if (ptEnd.x < ptStart.x && COMPARE_VALUES_EPS(range_eeh.low.z, range_CurFace.low.z, 300) != 0)
								{
									endFlag = true;
									vec1.Negate();
									ptEnd.Add(vec1);
									vec1.ScaleToLength(dSideCover + 1.5 * m_curreDiameter);
									ptEnd.Add(vec1);
								}
							}
						}
					}
				}
				else if (m_sidetype == SideType::In)
				{
					if (typeend == PITRebarEndType::kBend)
					{
						//先缩短0.5个赶紧距离
						vec2.ScaleToLength(0.5*m_curreDiameter);
						ptEnd.Add(vec2);
						//end
						if (m_insidef.bEndAnhorsel)
						{
							DPoint3d ptTest = ptEnd;
							DVec3d vecTest = vec1;
							vecTest.ScaleToLength(m_insidef.dEndanchoroffset);
							ptTest.Add(vecTest);
							if (!ISPointInHoles(m_Holeehs, ptTest))
							{
								vec1.ScaleToLength(m_insidef.dEndanchoroffset);
								ptEnd.Add(vec1);
							}
							else
							{
								GetHoleRebarAnchor(ptEnd, ptStart, ptEnd, endTypesTmp.end);
							}
						}
						else if (m_insidef.calLen > 1.0) //防止没值时把Vec变为0 
						{
							m_insidef.calLen = WallRebars_OffsetLength(ptEnd);
							vec2.ScaleToLength(m_insidef.calLen);
							ptEnd.Add(vec2);
						}
					}
					else if (typeend == PITRebarEndType::kCustom)
					{
						if (m_insidef.bEndAnhorsel)
						{
							vec1.ScaleToLength(m_insidef.dEndanchoroffset);
							ptEnd.Add(vec1);
						}
					}

				}
				else if (m_sidetype == SideType::Out)
				{
					if (typeend == PITRebarEndType::kBend)
					{
						//先缩短0.5个赶紧距离
						vec2.ScaleToLength(0.5*m_curreDiameter);
						ptEnd.Add(vec2);
						//end
						if (m_outsidef.bEndAnhorsel)
						{
							vec1.ScaleToLength(m_outsidef.dEndanchoroffset);
							ptEnd.Add(vec1);
						}
						else if (m_outsidef.calLen > 1.0) //防止没值时把Vec变为0 
						{
							vec2.ScaleToLength(m_outsidef.calLen);
							ptEnd.Add(vec2);
						}
					}
					else if (typeend == PITRebarEndType::kCustom)
					{
						if (m_outsidef.bEndAnhorsel)
						{
							vec1.ScaleToLength(m_outsidef.dEndanchoroffset);
							ptEnd.Add(vec1);
						}
					}
				}
			}

			// 集水坑有一个内侧面由于创建实体失败导致与整个补过孔洞的集水坑实体取交点再延长使得钢筋延申出集水坑
			if (!isSuccess && m_CatchpitType == 0)
			{
				if (GetMainRebars().at(m_curLevel).rebarDir == 0)//水平
				{
					if (ptStart.y < itrplus->second.y)
					{
						ptStart.y += m_twoFacesDistance;
					}
					else
					{
						ptStart.y -= m_twoFacesDistance;
					}
				}
				else//纵向
				{
					if (ptStart.z < itrplus->second.z)
					{
						ptStart.z += m_twoFacesDistance;
					}
				}

			}
			// 特殊的集水坑,有一部分凹陷，需要将纵向钢筋延申到板
			if (m_CatchpitType == 1 && m_isCatchpit)//特殊的集水坑
			{
				DRange3d solidRange;
				mdlElmdscr_computeRange(&solidRange.low, &solidRange.high, m_pOldElm->GetElementDescrCP(), nullptr);
				if (GetMainRebars().at(m_curLevel).rebarDir == 1)
				{//使得纵向钢筋往上延申到板
					double zLength = solidRange.ZLength();
					double CoverSize = GetConcrete().sideCover;
					if (ptStart.z > ptEnd.z && abs(ptStart.z - ptEnd.z) < zLength - 600 * uor_per_mm - CoverSize * uor_per_mm)
						ptStart.z += 600 * uor_per_mm;
				}
			}
			// 集水坑纵向钢筋锚入上面的底板。
			if (/*m_CatchpitType == 2*/m_isCatchpit && GetMainRebars().at(m_curLevel).rebarDir == 1)
			{
				for (auto it = m_mapFloorAndHeight.begin(); it != m_mapFloorAndHeight.end(); ++it)
				{
					DRange3d upFloorRange;
					mdlElmdscr_computeRange(&upFloorRange.low, &upFloorRange.high, it->first->GetElementDescrCP(), nullptr);
					//double midZ = (upFloorRange.low.z + upFloorRange.high.z) / 2;
					if (ptStart.z > ptEnd.z && upFloorRange.IsContainedXY(ptStart)/* && (midZ > ptStart.z)*/)
					{
						DPoint3d tmpPt = ptStart;
						tmpPt.z += it->second;
						if (ISPointInHoles(m_ScanedFloors, tmpPt))
						{
							ptStart.z += it->second;

							DVec3d faceNormal = GetfaceNormal();
							if (faceNormal.IsParallelTo(DVec3d::UnitX()) && COMPARE_VALUES_EPS(upFloorRange.YLength(), range_CurFace.YLength(), 100) == 0)
							{
								CVector3D begNormal = endTypesTmp.beg.GetendNormal();
								begNormal.Negate();
								endTypesTmp.beg.SetendNormal(begNormal);
								double begBendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * 15 * uor_per_mm;
								endTypesTmp.beg.SetbendLen(begBendLen);
							}
							else if (faceNormal.IsParallelTo(DVec3d::UnitY()) && COMPARE_VALUES_EPS(upFloorRange.XLength(), range_CurFace.XLength(), 100) == 0)
							{
								CVector3D begNormal = endTypesTmp.beg.GetendNormal();
								begNormal.Negate();
								endTypesTmp.beg.SetendNormal(begNormal);
								double begBendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * 15 * uor_per_mm;
								endTypesTmp.beg.SetbendLen(begBendLen);
							}
						}

					}
				}
				if (isNeedChange && (ptStart.z > ptEnd.z))
				{
					double insideBendlen = stod(GetMainRebars().at(m_curLevel).rebarSize) * 15 * uor_per_mm;
					double outsideBendlen = stod(GetMainRebars().at(m_curLevel).rebarSize) * GetLae();
					double beforeBendlen = endTypesTmp.beg.GetbendLen();
					if (COMPARE_VALUES_EPS(beforeBendlen, outsideBendlen, 5) == 0)
					{
						/*测试修改锚入之后的钢筋是否在构件中，如果不在则不进行修改*/
						DPoint3d ExtendPt = ptStart;
						CVector3D Normal = endTypesTmp.beg.GetendNormal();
						Normal.Normalize(); Normal.Negate();
						Normal.ScaleToLength(insideBendlen);
						ExtendPt.Add(Normal);
						if (ISPointInHoles(m_ScanedFloors, ExtendPt))/*测试修改锚入之后的钢筋是否在构件中，如果不在则不进行修改*/
						{
							endTypesTmp.beg.SetbendLen(insideBendlen);
							CVector3D begNormal = endTypesTmp.beg.GetendNormal();
							begNormal.Negate();
							endTypesTmp.beg.SetendNormal(begNormal);
						}
					}
				}
			}
			//特殊双集水坑有三个面的锚固需要修改
			if (m_CatchpitType == 2)
			{
				if (GetfaceNormal().IsParallelTo(DVec3d::UnitZ()) && COMPARE_VALUES_EPS(range_CurFace.XLength(), 600 * uor_per_mm, 50) == 0 && COMPARE_VALUES_EPS(range_CurFace.YLength(), 2600 * uor_per_mm, 50) == 0)
				{
					if (endTypesTmp.beg.GetType() == PITRebarEndType::Type::kNone)
					{
						endTypesTmp.beg.SetType(PITRebarEndType::Type::kBend);
						DVec3d normal = DVec3d::From(0, 0, -1);
						endTypesTmp.beg.SetendNormal(normal);
						double bendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * GetLae();
						BrString strRebarSize(GetMainRebars().at(m_curLevel).rebarSize);
						double bendRadius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);
						endTypesTmp.beg.SetbendRadius(bendRadius);
						endTypesTmp.beg.SetbendLen(bendLen);
						DVec3d rebarVec = ptEnd - ptStart;
						rebarVec.Normalize();
						rebarVec.ScaleToLength(m_curreDiameter / 2);
						ptStart.Add(rebarVec);
					}
				}
				else if (GetfaceNormal().IsParallelTo(DVec3d::UnitX()) && COMPARE_VALUES_EPS(range_CurFace.ZLength(), 1300 * uor_per_mm, 50) == 0 && COMPARE_VALUES_EPS(range_CurFace.YLength(), 2600 * uor_per_mm, 50) == 0)
				{
					if (endTypesTmp.beg.GetType() != PITRebarEndType::Type::kNone)
					{
						double bendlen = endTypesTmp.beg.GetbendLen();
						double inBendlen = stod(GetMainRebars().at(m_curLevel).rebarSize) * 15 * uor_per_mm;
						if (COMPARE_VALUES_EPS(bendlen, inBendlen, 30) == 0)
						{
							CVector3D begNormal = endTypesTmp.beg.GetendNormal();
							begNormal.Negate();
							endTypesTmp.beg.SetendNormal(begNormal);
						}
					}
					else if (endTypesTmp.beg.GetType() == PITRebarEndType::Type::kNone)
					{
						endTypesTmp.beg.SetType(PITRebarEndType::Type::kBend);
						DVec3d normal = DVec3d::From(-1, 0, 0);
						endTypesTmp.beg.SetendNormal(normal);
						double bendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * GetLae();
						BrString strRebarSize(GetMainRebars().at(m_curLevel).rebarSize);
						double bendRadius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);
						endTypesTmp.beg.SetbendRadius(bendRadius);
						endTypesTmp.beg.SetbendLen(bendLen);
						DVec3d rebarVec = ptEnd - ptStart;
						rebarVec.Normalize();
						rebarVec.ScaleToLength(m_curreDiameter / 2);
						ptStart.Add(rebarVec);
					}
				}
				else if (GetfaceNormal().IsParallelTo(DVec3d::UnitY()) && COMPARE_VALUES_EPS(range_CurFace.XLength(), 600 * uor_per_mm, 50) == 0 && COMPARE_VALUES_EPS(range_CurFace.ZLength(), 1300 * uor_per_mm, 50) == 0)
				{
					if (endTypesTmp.beg.GetType() == PITRebarEndType::Type::kNone)
					{
						endTypesTmp.beg.SetType(PITRebarEndType::Type::kBend);
						DVec3d normal = DVec3d::From(0, -1, 0);
						endTypesTmp.beg.SetendNormal(normal);
						double bendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * GetLae();
						BrString strRebarSize(GetMainRebars().at(m_curLevel).rebarSize);
						double bendRadius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);
						endTypesTmp.beg.SetbendRadius(bendRadius);
						endTypesTmp.beg.SetbendLen(bendLen);
						DVec3d rebarVec = ptEnd - ptStart;
						rebarVec.Normalize();
						rebarVec.ScaleToLength(m_curreDiameter / 2);
						ptStart.Add(rebarVec);
					}
				}
			}
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(ptStart);
			vex->SetType(RebarVertex::kStart);
			if (m_CatchpitType == 1 && begFlag)
			{
				CVector3D begNormal = endTypesTmp.beg.GetendNormal();
				begNormal.Negate();
				endTypesTmp.beg.SetendNormal(begNormal);
				double begBendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * GetLae();
				endTypesTmp.beg.SetbendLen(begBendLen);
			}

			//检查钢筋末端锚固关系
			CheckRebarAnchorage(ptStart, dSideCover, m_allEehs, endTypesTmp.beg);

			endTypesTmp.beg.SetptOrgin(ptStart);

			// 集水坑有一个内侧面由于创建实体失败导致与整个补过孔洞的集水坑实体取交点再延长使得钢筋延申出集水坑
			if (!isSuccess && m_CatchpitType == 0)
			{
				if (GetMainRebars().at(m_curLevel).rebarDir == 0)//水平
				{
					if (ptStart.y < ptEnd.y)
					{
						ptEnd.y -= m_twoFacesDistance;
					}
					else
					{
						ptEnd.y += m_twoFacesDistance;
					}
				}
				else
				{
					if (ptStart.z > ptEnd.z)
					{
						ptEnd.z += m_twoFacesDistance;
					}
				}
			}
			// 特殊的集水坑,有一部分凹陷，需要将纵向钢筋延申到板
			if (m_CatchpitType == 1 && m_isCatchpit)
			{
				DRange3d solidRange;
				mdlElmdscr_computeRange(&solidRange.low, &solidRange.high, m_pOldElm->GetElementDescrCP(), nullptr);
				if (GetMainRebars().at(m_curLevel).rebarDir == 1)
				{//使得纵向钢筋往上延申到板
					double zLength = solidRange.ZLength();
					double CoverSize = GetConcrete().sideCover;
					if (ptStart.z < ptEnd.z && abs(ptStart.z - ptEnd.z) < zLength - 600 * uor_per_mm - CoverSize * uor_per_mm)
						ptEnd.z += 600 * uor_per_mm;
				}
			}
			// 特殊集水坑处理
			if (m_CatchpitType == 1 && endFlag)
			{
				CVector3D endNormal = endTypesTmp.end.GetendNormal();
				endNormal.Negate();
				endTypesTmp.end.SetendNormal(endNormal);
				double endBendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * GetLae();
				endTypesTmp.end.SetbendLen(endBendLen);
			}
			// 集水坑纵向钢筋锚入上面的底板。
			if (/*m_CatchpitType == 2*/m_isCatchpit && GetMainRebars().at(m_curLevel).rebarDir == 1)
			{
				for (auto it = m_mapFloorAndHeight.begin(); it != m_mapFloorAndHeight.end(); ++it)
				{
					DRange3d upFloorRange;
					mdlElmdscr_computeRange(&upFloorRange.low, &upFloorRange.high, it->first->GetElementDescrCP(), nullptr);
					//double midZ = (upFloorRange.low.z + upFloorRange.high.z) / 2;
					if (ptStart.z < ptEnd.z && upFloorRange.IsContainedXY(ptEnd)/* && (midZ > ptEnd.z)*/)
					{
						DPoint3d tmpPt = ptEnd;
						tmpPt.z += it->second;
						if (ISPointInHoles(m_ScanedFloors, tmpPt))
						{
							ptEnd.z += it->second;

							DVec3d faceNormal = GetfaceNormal();
							if (faceNormal.IsParallelTo(DVec3d::UnitX()) && COMPARE_VALUES_EPS(upFloorRange.YLength(), range_CurFace.YLength(), 100) == 0)
							{
								CVector3D begNormal = endTypesTmp.end.GetendNormal();
								begNormal.Negate();
								endTypesTmp.end.SetendNormal(begNormal);
								double begBendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * 15 * uor_per_mm;
								endTypesTmp.end.SetbendLen(begBendLen);
							}
							else if (faceNormal.IsParallelTo(DVec3d::UnitY()) && COMPARE_VALUES_EPS(upFloorRange.XLength(), range_CurFace.XLength(), 100) == 0)
							{
								CVector3D begNormal = endTypesTmp.end.GetendNormal();
								begNormal.Negate();
								endTypesTmp.end.SetendNormal(begNormal);
								double begBendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * 15 * uor_per_mm;
								endTypesTmp.end.SetbendLen(begBendLen);
							}
						}

					}
				}
				if (isNeedChange && (ptStart.z < ptEnd.z))
				{
					double insideBendlen = stod(GetMainRebars().at(m_curLevel).rebarSize) * 15 * uor_per_mm;
					double outsideBendlen = stod(GetMainRebars().at(m_curLevel).rebarSize) * GetLae();
					double beforeBendlen = endTypesTmp.end.GetbendLen();
					if (COMPARE_VALUES_EPS(beforeBendlen, outsideBendlen, 5) == 0)
					{
						DPoint3d ExtendPt = ptEnd;
						CVector3D Normal = endTypesTmp.end.GetendNormal();
						Normal.Normalize(); Normal.Negate();
						Normal.ScaleToLength(insideBendlen);
						ExtendPt.Add(Normal);
						if (ISPointInHoles(m_ScanedFloors, ExtendPt))/*测试修改锚入之后的钢筋是否在构件中，如果不在则不进行修改*/
						{
							endTypesTmp.end.SetbendLen(insideBendlen);
							CVector3D endNormal = endTypesTmp.end.GetendNormal();
							endNormal.Negate();
							endTypesTmp.end.SetendNormal(endNormal);
						}

					}
				}
			}
			// 特殊双集水坑有三个面的锚固需要修改
			if (m_CatchpitType == 2)
			{
				if (GetfaceNormal().IsParallelTo(DVec3d::UnitZ()) && COMPARE_VALUES_EPS(range_CurFace.XLength(), 600 * uor_per_mm, 50) == 0 && COMPARE_VALUES_EPS(range_CurFace.YLength(), 2600 * uor_per_mm, 50) == 0)
				{
					if (endTypesTmp.end.GetType() == PITRebarEndType::Type::kNone)
					{
						endTypesTmp.end.SetType(PITRebarEndType::Type::kBend);
						DVec3d normal = DVec3d::From(0, 0, -1);
						endTypesTmp.end.SetendNormal(normal);
						double bendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * GetLae();
						BrString strRebarSize(GetMainRebars().at(m_curLevel).rebarSize);
						double bendRadius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);
						endTypesTmp.end.SetbendRadius(bendRadius);
						endTypesTmp.end.SetbendLen(bendLen);
						DVec3d rebarVec = ptEnd - ptStart;
						rebarVec.Normalize();
						rebarVec.Negate();
						rebarVec.ScaleToLength(m_curreDiameter / 2);
						ptEnd.Add(rebarVec);
					}
				}
				else if (GetfaceNormal().IsParallelTo(DVec3d::UnitX()) && COMPARE_VALUES_EPS(range_CurFace.ZLength(), 1300 * uor_per_mm, 50) == 0 && COMPARE_VALUES_EPS(range_CurFace.YLength(), 2600 * uor_per_mm, 50) == 0)
				{
					if (endTypesTmp.end.GetType() != PITRebarEndType::Type::kNone)
					{
						double bendlen = endTypesTmp.end.GetbendLen();
						double inBendlen = stod(GetMainRebars().at(m_curLevel).rebarSize) * 15 * uor_per_mm;
						if (COMPARE_VALUES_EPS(bendlen, inBendlen, 30) == 0)
						{
							CVector3D begNormal = endTypesTmp.end.GetendNormal();
							begNormal.Negate();
							endTypesTmp.end.SetendNormal(begNormal);
						}
					}
					else if (endTypesTmp.end.GetType() == PITRebarEndType::Type::kNone)
					{
						endTypesTmp.end.SetType(PITRebarEndType::Type::kBend);
						DVec3d normal = DVec3d::From(-1, 0, 0);
						endTypesTmp.end.SetendNormal(normal);
						double bendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * GetLae();
						endTypesTmp.end.SetbendLen(bendLen);
						BrString strRebarSize(GetMainRebars().at(m_curLevel).rebarSize);
						double bendRadius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);
						endTypesTmp.end.SetbendRadius(bendRadius);
						endTypesTmp.end.SetbendLen(bendLen);
						DVec3d rebarVec = ptEnd - ptStart;
						rebarVec.Normalize();
						rebarVec.Negate();
						rebarVec.ScaleToLength(m_curreDiameter / 2);
						ptEnd.Add(rebarVec);
					}
				}
				else if (GetfaceNormal().IsParallelTo(DVec3d::UnitY()) && COMPARE_VALUES_EPS(range_CurFace.XLength(), 600 * uor_per_mm, 50) == 0 && COMPARE_VALUES_EPS(range_CurFace.ZLength(), 1300 * uor_per_mm, 50) == 0)
				{
					if (endTypesTmp.end.GetType() == PITRebarEndType::Type::kNone)
					{
						endTypesTmp.end.SetType(PITRebarEndType::Type::kBend);
						DVec3d normal = DVec3d::From(0, -1, 0);
						endTypesTmp.end.SetendNormal(normal);
						double bendLen = stod(GetMainRebars().at(m_curLevel).rebarSize) * GetLae();
						endTypesTmp.end.SetbendLen(bendLen);
						BrString strRebarSize(GetMainRebars().at(m_curLevel).rebarSize);
						double bendRadius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);
						endTypesTmp.end.SetbendRadius(bendRadius);
						endTypesTmp.end.SetbendLen(bendLen);
						DVec3d rebarVec = ptEnd - ptStart;
						rebarVec.Normalize();
						rebarVec.Negate();
						rebarVec.ScaleToLength(m_curreDiameter / 2);
						ptEnd.Add(rebarVec);
					}
				}
			}

			//检查钢筋末端锚固关系
			CheckRebarAnchorage(ptEnd, dSideCover, m_allEehs, endTypesTmp.end);

			endTypesTmp.end.SetptOrgin(ptEnd);

			// START#61271 集水井用“集水井面配筋”功能配出来的钢筋锚固长度没有规避孔洞（已点选规避孔洞功能） ▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼
			//规避孔洞，修改锚入的长度来规避孔洞
			if (GetConcrete().isHandleHole && m_isCatchpit)
			{
				if (endTypesTmp.beg.GetType() != PITRebarEndType::Type::kNone)
				{
					double bendLen = endTypesTmp.beg.GetbendLen();
					CutRebarAnchorLeng(endTypesTmp.beg.GetptOrgin(), endTypesTmp.beg.GetendNormal(), 0, bendLen);
					endTypesTmp.beg.SetbendLen(bendLen);
				}
				if (endTypesTmp.end.GetType() != PITRebarEndType::Type::kNone)
				{
					double bendLen = endTypesTmp.end.GetbendLen();
					CutRebarAnchorLeng(endTypesTmp.end.GetptOrgin(), endTypesTmp.end.GetendNormal(), 0, bendLen);
					endTypesTmp.end.SetbendLen(bendLen);
				}
			}
			// END#61271 集水井用“集水井面配筋”功能配出来的钢筋锚固长度没有规避孔洞（已点选规避孔洞功能） ▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲▲

			// 外侧尝试反转锚入集水坑或装饰墙
			if (m_sidetype == SideType::Out)
			{
				auto checkAndAnchor = [&](PITRebarEndType &endTypeTmp) {
					if (endTypeTmp.GetType() != PITRebarEndType::kBend)
						return;
					CVector3D vector = endTypeTmp.GetendNormal();
					DPoint3d  endPtTemp = endTypeTmp.GetptOrgin();
					DPoint3d  endPtTempEx = endPtTemp;
					double length = endTypeTmp.GetbendLen() + endTypeTmp.GetbendRadius();
					bool canAcnhor = false;
					bool isAnchored = false;
					movePoint(vector, endPtTemp, length);
					vector.Negate();
					movePoint(vector, endPtTempEx, length);
					for (auto wall : m_Allwalls)
					{
						EditElementHandle eehWall(wall.GetElementRef(), wall.GetDgnModelP());
						if (ISPointInElement(&eehWall, endPtTemp))
							isAnchored = true;
						if (ISPointInElement(&eehWall, endPtTempEx))
							canAcnhor = true;
					}
					if (!isAnchored && canAcnhor)// 能锚入到其它实体
						endTypeTmp.SetendNormal(vector);
				};
				checkAndAnchor(endTypesTmp.beg);
				checkAndAnchor(endTypesTmp.end);
			}

			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(ptEnd);
			vex->SetType(RebarVertex::kEnd);

			// 			EditElementHandle eeh;
			// 			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptStart, ptEnd), true, *ACTIVEMODEL);
			// 			eeh.AddToModel();

			rebar.EvaluateEndTypes(endTypesTmp);

			//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
			rebars.push_back(rebar);
		}
	}
	return true;
}

RebarSetTag* PlaneRebarAssembly::MakeRebars
(
	ElementId&          rebarSetId,
	LineSegment			rebarLine,
	LineSegment			vec,
	int					dir,
	BrStringCR          sizeKey,
	double              xLen,
	double              spacing,
	double              startOffset,
	double              endOffset,
	int					level,
	int					grade,
	int					DataExchange,
	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
	DgnModelRefP        modelRef
)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	if (endType.size() != vecEndNormal.size() || endType.size() == 0)
	{
		return NULL;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius = 0, endbendRadius = 0;
	double startbendLen = 0, endbendLen = 0;
	double begStraightAnchorLen = 0, endStraightAnchorLen = 0;
	switch (endType[0].endType)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeStart.SetType(RebarEndType::kLap);
		begStraightAnchorLen = endType[0].endPtInfo.value1;	//锚入长度
		break;
	case 4:	//90度弯钩
	{
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		startbendLen = endType[0].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		}
	}
	break;
	case 5:	//135度弯钩
	{
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		startbendLen = endType[0].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		}
	}
	break;
	case 6:	//180度弯钩
	{
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		startbendLen = endType[0].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		}
	}
	break;
	case 8:	//用户
		endTypeStart.SetType(RebarEndType::kCustom);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		startbendLen = endType[0].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		}
		break;
	default:
		break;
	}

	switch (endType[1].endType)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeEnd.SetType(RebarEndType::kLap);
		endStraightAnchorLen = endType[1].endPtInfo.value1;	//锚入长度
		break;
	case 4:	//90度弯钩
	{
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		endbendLen = endType[1].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		}
	}
	break;
	case 5:	//135度弯钩
	{
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		endbendLen = endType[1].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		}
	}
	break;
	case 6:	//180度弯钩
	{
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		endbendLen = endType[1].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		}
	}
	break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		endbendLen = endType[1].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		}
		break;
	default:
		break;
	}

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double adjustedXLen, adjustedSpacing;
	double sideCov = GetConcrete().sideCover*uor_per_mm;
	double updownSideCover = 50 * uor_per_mm;
	rebarLine.PerpendicularOffset(startOffset, vec.GetLineVec());
	if (GetConcrete().isFaceUnionRebar && !_ehCrossPlanePre.IsValid() && !_ehCrossPlaneNext.IsValid() && !m_isCatchpit)
	{
		double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);

		rebarLine.PerpendicularOffset(diameter + spacing, vec.GetLineVec());
		rebarLine.PerpendicularOffset(diameter, PopfaceNormal());
		vec.Shorten(sideCov * 2 + diameter + spacing, true);
		vec.Shorten(-sideCov * 2 - diameter * 2, false);
		adjustedXLen = xLen - diameter - sideCov * 2/* - startOffset - endOffset*/;
	}
	else
	{
		adjustedXLen = xLen - diameter - sideCov * 2/* - startOffset - endOffset*/;
	}
	int numRebar = 0;
	//竖直采用墙间距计算参数0.85
	if (fabs(vec.GetLineVec().DotProduct(DVec3d::From(0, 0, 1))) > 0.9)
		numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
	else//横向采用板间距计算参数0.5
		numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	//adjustedSpacing = spacing;
	//	rebarLine.PerpendicularOffset(startOffset, vec.GetLineVec());
	//	rebarLine.PerpendicularOffset(sideCov, vec.GetLineVec());
	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;

	// 	EditElementHandle eeh;
	// 	LineHandler::CreateLineElement(eeh, NULL, rebarLine.GetLineSeg(), true, *ACTIVEMODEL);
	// 	eeh.AddToModel();


	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	if (endTypeStart.GetType() == PITRebarEndType::kCustom)
	{
		DVec3d vec = rebarLine.GetLineVec();
		double angel = vecEndNormal[0].AngleTo(vec);
		start.Setangle(angel);
	}
	else
	{
		start.Setangle(endType[0].rotateAngle);
	}

	start.SetstraightAnchorLen(begStraightAnchorLen);
	start.SetbendLen(startbendLen - startbendRadius);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);
	start.SetptOrgin(rebarLine.GetLineStartPoint());

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	if (endTypeEnd.GetType() == PITRebarEndType::kCustom)
	{
		DVec3d vec = rebarLine.GetLineVec();
		vec.Negate();
		double angel = vecEndNormal[1].AngleTo(vec);
		end.Setangle(angel);
	}
	else
	{
		end.Setangle(endType[1].rotateAngle);
	}

	end.SetstraightAnchorLen(endStraightAnchorLen);
	end.SetbendLen(endbendLen - endbendRadius);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);
	end.SetptOrgin(rebarLine.GetLineEndPoint());

	PITRebarEndTypes   endTypes = { start, end };
	//	rebarLine.PerpendicularOffset(sideCov+diameter*0.5, vec.GetLineVec());

		//EditElementHandle eehSolid;
		//ISolidKernelEntityPtr ptarget;
		//SolidUtil::Convert::ElementToBody(ptarget, m_face, true, true, true);
		//if (SUCCESS == SolidUtil::Modify::ThickenSheet(ptarget, 5000.0 * uor_per_mm, 5000 * uor_per_mm))
		//{
		//	if (SUCCESS == SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL))
		//	{
		//		eehSolid.AddToModel();
		//	}
		//}

	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve> rebarCurves;

		endTypes.beg.SetptOrgin(rebarLine.GetLineStartPoint());
		endTypes.end.SetptOrgin(rebarLine.GetLineEndPoint());
		makeRebarCurve(rebarCurves, endTypes);

		DVec3d offsetVec = vec.GetLineVec();
		rebarLine.PerpendicularOffset(adjustedSpacing, offsetVec);

		if (m_sidetype == SideType::Out || (m_sidetype == SideType::In && m_zCorner))//外侧面钢筋处理，或者Z型板处理
		{
			if (i == 0 && m_strDelete)
			{
				continue;
			}
			else if (i == numRebar - 1 && m_endDelete)
			{
				continue;
			}
		}
		if ((i == 0 || (i == (numRebar - 1))) && rebarCurves.empty())
			continue;
		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
		if (i != 0 && (i != (numRebar - 1)))
			rebarCurves.clear();
		if (m_sidetype == SideType::In || (m_sidetype == SideType::Out && !m_strDelete && !m_endDelete))//内侧面钢筋处理
		{
			double tmppos = 0;
			LineSegment tmpRebarLine = rebarLine;
			tmpRebarLine.PerpendicularOffset(-adjustedSpacing, offsetVec);//对于需要添加的情况，需要还原到边界内钢筋位置
			if ((i == 0 && m_insidef.strval && m_sidetype == SideType::In) || (i == 0 && m_outsidef.strval && m_sidetype == SideType::Out))
			{
				//在反方向添加一根钢筋
				auto  ptStr = rebarCurves.front().GetVertices().At(0).GetIP();
				auto  ptEnd = rebarCurves.front().GetVertices().At(1).GetIP();
				rebarCurves.clear();
				DPoint3d midPt = ptStr;
				midPt.Add(ptEnd);
				midPt.Scale(0.5);
				if (dir == 0)//x
					midPt.y -= 2 * GetConcrete().sideCover * uor_per_mm;
				else//y
					midPt.x -= 2 * GetConcrete().sideCover * uor_per_mm;
				double inside_Offset = InsideFace_OffsetLength(midPt) + diameter;//墙和板的保护层距离 + 墙的最边上的钢筋直径加上自身直径
				tmppos = tmppos - inside_Offset;
			}
			else if ((i == numRebar - 1 && m_insidef.endval && m_sidetype == SideType::In) || (i == numRebar - 1 && m_outsidef.endval && m_sidetype == SideType::Out))
			{
				//在反方向添加一根钢筋
				auto  ptStr = rebarCurves.front().GetVertices().At(0).GetIP();
				auto  ptEnd = rebarCurves.front().GetVertices().At(1).GetIP();
				rebarCurves.clear();
				DPoint3d midPt = ptStr;
				midPt.Add(ptEnd);
				midPt.Scale(0.5);
				if (dir == 0)//x
					midPt.y += 2 * GetConcrete().sideCover * uor_per_mm;
				else//y
					midPt.x += 2 * GetConcrete().sideCover * uor_per_mm;
				double inside_Offset = InsideFace_OffsetLength(midPt) + diameter;//墙和板的保护层距离 + 墙的最边上的钢筋直径加上自身直径
				tmppos = tmppos + inside_Offset;
			}
			if (tmppos != 0)
			{
				tmpRebarLine.PerpendicularOffset(tmppos, offsetVec);
				endTypes.beg.SetptOrgin(tmpRebarLine.GetLineStartPoint());
				endTypes.end.SetptOrgin(tmpRebarLine.GetLineEndPoint());
				makeRebarCurve(rebarCurves, endTypes);
				rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
			}
		}
		/*if (i == numRebar - 1)//如果是最后一根，要判断当前还有多少距离,符合距离要求就要再布置一根
		{
			double sDis = adjustedXLen - (numRebar - 2)*adjustedSpacing;
			if (sDis > spacing)
			{
				DVec3d dvec = vec.GetLineVec();
				rebarLine.PerpendicularOffset((sDis - spacing), dvec);
			}
			else
			{
				DVec3d dvec = vec.GetLineVec();
				dvec.Negate();
				rebarLine.PerpendicularOffset((spacing - sDis), dvec);
			}
		}*/
	}

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);
	vector<DSegment3d> vecStartEnd;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (ISPointInHoles(m_Holeehs, midPos))
		{
			if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
			{
				continue;
			}
		}

		vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));
		// 		EditElementHandle eeh;
		// 		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		// 		eeh.AddToModel();

		RebarElementP rebarElement = NULL;
		if (!FacePreviewButtonsDown)//预览标志，预览状态下不要生成钢筋
		{
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
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

			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (DataExchange == 0)
			{
				Stype = "front";
			}
			else if (DataExchange == 1)
			{
				Stype = "midden";
			}
			else
			{
				Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}

		j++;
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

bool PlaneRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	EditElementHandle eehface(m_face, m_face.GetDgnModelP());

	//将所有面转换到XOZ平面
	CVector3D ORIPT = GetPlacement().GetTranslation();
	CMatrix3D tmpmat = GetPlacement();
	Transform trans;
	tmpmat.AssignTo(trans);
	trans.InverseOf(trans);
	TransformInfo transinfo(trans);

	DPoint3d ptcenter = DPoint3d::From(0, 0, 0);
	DPoint3d facenormal = DPoint3d::From(0, 1, 0);
	Transform tran;
	mdlTMatrix_computeFlattenTransform(&tran, &ptcenter, &facenormal);
	TransformInfo tinfo(tran);
	EditElementHandle eehline1;
	EditElementHandle eehline2;
	LineHandler::CreateLineElement(eehline1, NULL, m_LineSeg1.GetLineSeg(), ACTIVEMODEL->Is3d(), *ACTIVEMODEL);
	LineHandler::CreateLineElement(eehline2, NULL, m_LineSeg2.GetLineSeg(), ACTIVEMODEL->Is3d(), *ACTIVEMODEL);
	//eehline1.AddToModel();
	//eehline2.AddToModel();
	eehline1.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline1, transinfo);
	eehline1.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline1, tinfo);
	DPoint3d pt1, pt2;
	mdlElmdscr_extractEndPoints(&pt1, nullptr, &pt2, nullptr, eehline1.GetElementDescrP(), ACTIVEMODEL);
	DVec3d vec1 = pt2 - pt1;
	double distance1 = pt2.Distance(pt1);
	vec1.Normalize();
	eehline2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline2, transinfo);
	eehline2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline2, tinfo);
	DPoint3d pt3, pt4;
	mdlElmdscr_extractEndPoints(&pt3, nullptr, &pt4, nullptr, eehline2.GetElementDescrP(), ACTIVEMODEL);
	DVec3d vec2 = pt4 - pt3;
	double distance2 = pt4.Distance(pt3);
	vec2.Normalize();
	{
		_d = 1.0;

		ElementId id = GetSelectedElement();
		EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());
		//testeeh.AddToModel();
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
		SetSelectedElement(Eleeh.GetElementId());
		if (m_Holeehs.size() == 0 && Holeehs.size() > 0)
		{
			m_Holeehs = Holeehs;
		}
		if (g_globalpara.Getrebarstyle() != 0)
		{
			NewRebarAssembly(modelRef);
		}
		SetSelectedElement(testeeh.GetElementId());
		CalculateUseHoles(modelRef);

		/*/////扫描板附近的所有墙/////*/
		DRange3d slab_range;
		mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, m_pOldElm->GetElementDescrCP(), NULL);
		slab_range.low.z -= 5 * UOR_PER_MilliMeter;
		slab_range.low.x += 5 * UOR_PER_MilliMeter;
		slab_range.low.y += 5 * UOR_PER_MilliMeter;
		slab_range.high.z += 5 * UOR_PER_MilliMeter;
		slab_range.high.x -= 5 * UOR_PER_MilliMeter;
		slab_range.high.y -= 5 * UOR_PER_MilliMeter;
		m_Allwalls = // 扫描附近的墙
			scan_elements_in_range(
				slab_range,
				[&](const ElementHandle &eh) -> bool {
			if (eh.GetElementId() == m_pOldElm->GetElementId())
			{
				// 过滤掉自己
				return false;
			}
			// 只需要墙
			return is_Wall(eh);
		});
		/*/////扫描板附近的所有墙/////*/

		//整理自身以及附近所有实体
		for (auto& wall : m_Allwalls)
		{
			EditElementHandle* eehWall = new EditElementHandle(wall.GetElementRef(), wall.GetDgnModelP());
			m_allEehs.push_back(eehWall);
		}
		EditElementHandle* selfEeh = new EditElementHandle();;
		selfEeh->Duplicate(testeeh);
		m_allEehs.push_back(selfEeh);

		RebarSetTagArray rsetTags;
		m_vecRebarStartEnd.clear();
		g_vecRebarPtsNoHole.clear();
		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
		double dLevelSpace = 0;
		double updownSideCover = 50 * uor_per_mm;
		double dSideCover = GetConcrete().sideCover * uor_per_mm;
		double dPositiveCover = GetConcrete().postiveCover * uor_per_mm;
		double offset = dPositiveCover;

		int iRebarSetTag = 0;
		int iRebarLevelNum = GetConcrete().rebarLevelNum;
		vector<PIT::EndType> vecEndType;

		// 孔洞规避前 面配筋钢筋点 按层存储，之前不同层的不能删掉
		std::vector<RebarPoint> vecRebarPoint;
		ElementId contid = this->FetchConcrete();
		GetElementXAttribute(contid, vecRebarPoint, vecRebarPointsXAttribute, ACTIVEMODEL);
		std::map<int, vector<RebarPoint>> mapRebarPoint;
		for (RebarPoint stRebarPt : vecRebarPoint)
		{
			auto iter = mapRebarPoint.find(stRebarPt.DataExchange);
			if (iter != mapRebarPoint.end())
			{
				iter->second.push_back(stRebarPt);
			}
			else
			{
				vector<RebarPoint> vecTemp;
				vecTemp.push_back(stRebarPt);
				mapRebarPoint[stRebarPt.DataExchange] = vecTemp;
			}
		}
		// end

		// 面配筋竖直面配置方式优化，调换层次数据
		if (abs(m_LineSeg2.GetLineVec().DotProduct(DVec3d::From(0, 0, -1))) > 0.9)
		{
			vector<PIT::ConcreteRebar> mainRebarsData = PopMainRebars();
			// 在反转之前，保存需要保持的数据
			vector<int> rebarLevels(mainRebarsData.size());
			vector<int> rebarDirs(mainRebarsData.size());
			bool isParallelToY = m_LineSeg1.GetLineVec().IsParallelTo(DVec3d::UnitY());
			for (size_t i = 0; i < mainRebarsData.size(); ++i) {
				rebarLevels[i] = mainRebarsData[i].rebarLevel;
				if (isParallelToY)
					rebarDirs[i] = mainRebarsData[i].rebarDir;
			}

			// 反转整个vector
			reverse(mainRebarsData.begin(), mainRebarsData.end());

			// 恢复被保存的字段
			for (size_t i = 0; i < mainRebarsData.size(); ++i) {
				mainRebarsData[i].rebarLevel = rebarLevels[i];
				if (isParallelToY)
					mainRebarsData[i].rebarDir = rebarDirs[i];
			}

			this->SetMainRebars(mainRebarsData);
		}

		for (int i = 0; i < iRebarLevelNum; ++i)
		{
			m_curLevel = i;
			m_holeRebarInfo.ClearData();
			DVec3d vecRebar = DVec3d::From(1, 0, 0);
			if (GetMainRebars().at(i).rebarDir == 1)	//纵向钢筋
			{
				vecRebar = DVec3d::From(0, 1, 0);
			}
			MSElementDescrP tmpupfaces[40] = { 0,0,0,0,0,0,0,0,0,0 };
			MSElementDescrP tmpdownfaces[40] = { 0,0,0,0,0,0,0,0,0,0 };
			for (int i = 0; i < m_ldfoordata.upnum; i++)
			{
				mdlElmdscr_duplicate(&tmpupfaces[i], m_ldfoordata.upfaces[i]);
			}
			for (int i = 0; i < m_ldfoordata.downnum; i++)
			{
				mdlElmdscr_duplicate(&tmpdownfaces[i], m_ldfoordata.downfaces[i]);
			}

			MSElementDescrP faceEdp = nullptr;
			mdlElmdscr_duplicate(&faceEdp, eehface.GetElementDescrP());
			//获取面类型，内面，外面还是其他
			int sideType = GetFaceType(faceEdp, tmpupfaces, m_ldfoordata.upnum, tmpdownfaces, m_ldfoordata.downnum, i, vecRebar);
			//if (abs(GetfaceNormal().DotProduct(0,0,1)) > 0.9) 
			//{
			//	sideType = SideType::Nor;
			//}
			BrString strRebarSize(GetMainRebars().at(i).rebarSize);
			m_holeRebarInfo.brstring = strRebarSize;
			double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
			m_curreDiameter = diameter;
			BrString strRebarSizePre(GetMainRebars().at(iRebarLevelNum - 1 - i).rebarSize);
			double diameterPre = RebarCode::GetBarDiameter(strRebarSizePre, ACTIVEMODEL);
			double spacing = GetMainRebars().at(i).spacing * uor_per_mm;
			double startOffset = GetMainRebars().at(i).startOffset * uor_per_mm;
			double endOffset = GetMainRebars().at(i).endOffset * uor_per_mm;
			int	rebarDir = GetMainRebars().at(i).rebarDir;
			double levelspacing = GetMainRebars().at(i).levelSpace * uor_per_mm;
			int DataExchange = GetMainRebars().at(i).datachange;
			m_vecRebarPtsLayer.clear();
			m_vecTwinRebarPtsLayer.clear();

			{
				MSElementDescrP tmpupfaces[40] = { 0,0,0,0,0,0,0,0,0,0 };
				MSElementDescrP tmpdownfaces[40] = { 0,0,0,0,0,0,0,0,0,0 };
				for (int i = 0; i < m_ldfoordata.upnum; i++)
				{
					mdlElmdscr_duplicate(&tmpupfaces[i], m_ldfoordata.upfaces[i]);
				}
				for (int i = 0; i < m_ldfoordata.downnum; i++)
				{
					mdlElmdscr_duplicate(&tmpdownfaces[i], m_ldfoordata.downfaces[i]);
				}

				MSElementDescrP faceEdp = nullptr;
				mdlElmdscr_duplicate(&faceEdp, eehface.GetElementDescrP());
				if (m_isCatchpit)
					sideType = SideType::Nor;
				switch (sideType)
				{

				case SideType::Nor:
				{
					m_sidetype = SideType::Nor;
					if (m_solidType == 1)
					{
						// 分析墙的几何参数
						AnalyzeFloorAndWall(*m_Solid, i, faceEdp);
						GetHoriEndType(m_LineSeg1, i);
					}
					else
					{
						if (!m_isCatchpit)
						{
							vector<MSElementDescrP> tmpAnchordescrs;
							for (ISubEntityPtr face1 : m_anchorFaces)
							{
								EditElementHandle eehFace;
								if (!PIT::ConvertToElement::SubEntityToElement(eehFace, face1, ACTIVEMODEL))
								{

									continue;
								}
								eehFace.GetElementDescrP();
								MSElementDescrP TempEdp = eehFace.ExtractElementDescr();
								//mdlElmdscr_add(TempEdp);
								tmpAnchordescrs.push_back(TempEdp);

							}
							double	bendradius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);
							LineSegment linseg = m_LineSeg1;
							if (rebarDir == 1)
							{
								linseg = m_LineSeg2;
							}
							double LaE = PlaneRebarAssembly::GetLae();
							if (LaE > 0)
							{
								LaE *= stod(GetMainRebars().at(i).rebarSize);
							}
							else
							{
								LaE = 15 * diameter;
							}
							double la0 = 15 * stod(GetMainRebars().at(i).rebarSize) * uor_per_mm;
							CreateAnchorBySelf(tmpAnchordescrs, linseg, bendradius, la0/*15 * diameter*/, LaE, diameter, diameterPre, i, false, m_bisSump);
							//AnalyzeFloorNorFaceData();
						}
						else
						{
							vector<MSElementDescrP> tmpAnchordescrs;
							//for (ISubEntityPtr face1 : m_anchorFaces)
							//{
							//	EditElementHandle eehFace;
							//	if (!PIT::ConvertToElement::SubEntityToElement(eehFace, face1, ACTIVEMODEL))
							//	{

							//		continue;
							//	}
							//	eehFace.GetElementDescrP();
							//	MSElementDescrP TempEdp = eehFace.ExtractElementDescr();
							//	//mdlElmdscr_add(TempEdp);
							//	tmpAnchordescrs.push_back(TempEdp);

							//}
							for (auto it : m_VeticalPlanes)
							{
								MSElementDescrP tmpEdp = nullptr;
								mdlElmdscr_duplicate(&tmpEdp, it->GetElementDescrCP());
								tmpAnchordescrs.emplace_back(tmpEdp);
							}
							double	bendradius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);
							bool isYdir = false;
							LineSegment linseg = m_LineSeg1;
							if (rebarDir == 1)
							{
								isYdir = true;
								linseg = m_LineSeg2;
							}
							double LaE = PlaneRebarAssembly::GetLae();
							if (LaE > 0)
							{
								LaE *= stod(GetMainRebars().at(i).rebarSize);
							}
							else
							{
								LaE = 15 * diameter;
							}
							double la0 = 15 * stod(GetMainRebars().at(i).rebarSize) * uor_per_mm;
							CreateCatchpitBySelf(tmpAnchordescrs, linseg, bendradius, la0/*15 * diameter*/, LaE, diameter, i, false, true, isYdir);
						}

					}

					break;
				}
				case SideType::Out:
				{
					m_facePlace = 1;
					m_outsidef.ClearData();
					m_sidetype = SideType::Out;
					CalculateOutSideData(faceEdp, tmpupfaces, tmpdownfaces, i, vecRebar);
					break;
				}
				case SideType::In:
				{
					m_facePlace = 0;
					/*DVec3d dvec = GetfaceNormal();
					dvec.Negate();
					SetfaceNormal(dvec);*/
					m_insidef.ClearData();
					m_sidetype = SideType::In;
					CalculateInSideData(faceEdp, tmpupfaces, tmpdownfaces, i, vecRebar);

					break;
				}
				default:
					break;
				}
			}
			LineSegment lineSeg1 = m_LineSeg1;
			LineSegment lineSeg2 = m_LineSeg2;
			PopSetIds().push_back(0);
			RebarSetTag* tag = NULL;
			vector<PIT::EndType> vecEndType;
			if (GetvecEndTypes().empty())		//没有设置端部样式，设置默认值
			{
				EndType endType;
				memset(&endType, 0, sizeof(endType));
				vecEndType = { { 0,0,0 },{0,0,0} };
			}
			else
			{
				vecEndType = GetvecEndTypes().at(i);
			}
			if (sideType == SideType::In)
			{
				vecEndType[0] = m_insidef.strtype;
				vecEndType[1] = m_insidef.endtype;
			}
			else if (sideType == SideType::Out)
			{
				vecEndType[0] = m_outsidef.strtype;
				vecEndType[1] = m_outsidef.endtype;
			}
			else
			{
				if (m_solidType == 1)
				{
					Dpoint3d ptstart = lineSeg2.GetLineStartPoint();
					Dpoint3d ptEnd = lineSeg2.GetLineEndPoint();
					if (ptstart.z > ptEnd.z)
					{
						m_bUpIsStatr = true;
					}
					else
					{
						m_bUpIsStatr = false;
					}

					m_iCurRebarDir = GetMainRebars().at(i).rebarDir;
					if (GetMainRebars().at(i).rebarDir == 1)	//纵向钢筋
					{

						if (m_bUpIsStatr)
						{
							vecEndType[0] = m_topEndinfo;
							vecEndType[1] = m_bottomEndinfo;
						}
						else
						{
							vecEndType[1] = m_topEndinfo;
							vecEndType[0] = m_bottomEndinfo;
						}
					}
					else
					{
						vecEndType[0] = m_HorStrType;
						vecEndType[1] = m_HorEndType;
					}
				}
				else
				{
					vecEndType[0] = m_verSlabFaceInfo.strtype;
					vecEndType[1] = m_verSlabFaceInfo.endtype;
				}

			}



			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向
			if (rebarDir == 1)	//纵向钢筋
			{
				if (GetConcrete().isFaceUnionRebar && !_ehCrossPlanePre.IsValid() && !_ehCrossPlaneNext.IsValid())
				{
					if (!m_isCatchpit)
						continue;
				}
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = GetfaceNormal();
					CVector3D rebarVec = m_LineSeg2.GetLineVec();
					if (k == 0 && m_bStartAnhorselslantedFace)
					{
						vecEndNormal[k] = m_vecEndNormalStart;
						m_bStartAnhorselslantedFace = false;
					}
					else if (k == 1 && m_bEndAnhorselslantedFace)
					{
						vecEndNormal[k] = m_vecEndNormalEnd;
						m_bEndAnhorselslantedFace = false;
					}
					else
					{
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
						DVec3d dvec = GetfaceNormal();
						if (sideType == SideType::In && (!m_insidef.m_bStartIsSlefAhol && !m_insidef.m_bEndIsSlefAhol))
							dvec.Negate();
						if (sideType == SideType::Nor ||
							(sideType == SideType::In && (m_insidef.m_bStartIsSlefAhol || m_insidef.m_bEndIsSlefAhol))
							|| (sideType == SideType::Out && (m_outsidef.m_bStartIsSlefAhol || m_outsidef.m_bEndIsSlefAhol)))
							dvec = DVec3d::From(endNormal.x, endNormal.y, endNormal.z);

						vecEndNormal[k] = CVector3D::From(dvec.x, dvec.y, dvec.z);
					}

				}

				DVec3d vec = m_LineSeg1.GetLineVec();
				offset += levelspacing;

				if (0 == i)	//首层偏移当前钢筋直径
				{
					offset += diameter * 0.5;
				}
				else
				{
					double diameterPre = RebarCode::GetBarDiameter(GetMainRebars().at(i - 1).rebarSize, modelRef);	//上一层钢筋直径
					offset += diameterPre * 0.5;	//偏移上一层钢筋的半径
					offset += diameter * 0.5;		//偏移当前层钢筋的半径
				}

				lineSeg2.PerpendicularOffset(offset, GetfaceNormal());
				//ChangeRebarLine(lineSeg2);
				lineSeg2.PerpendicularOffset(updownSideCover + diameter * 0.5, vec);//再偏移钢筋半个直径

				LineSegment linesegment1 = m_LineSeg1;
				/*Dpoint3d ptDirStart = linesegment1.GetLineStartPoint();
				Dpoint3d ptdirEnd = linesegment1.GetLineEndPoint();
				DVec3d vecrebarDir1 = ptdirEnd - ptDirStart;
				DVec3d vecrebarDir2 = ptdirEnd - ptDirStart;

				Dpoint3d ptRebarStart = lineSeg2.GetLineStartPoint();
				Dpoint3d ptReabrEnd = lineSeg2.GetLineEndPoint();*/

				/*if (m_sidetype == SideType::In)
				{
					if (vec1.DotProduct(DVec3d::From(1, 0, 0)) > 0.9)
					{
						if (COMPARE_VALUES_EPS(m_LineSeg2.GetLineStartPoint().x, m_LineSeg1.GetLineStartPoint().x, 10) == 0)
						{
							vecrebarDir1.ScaleToLength(m_insidef.pos[0].str / 10 * uor_per_mm);
							ptDirStart.Add(vecrebarDir1);

							ptRebarStart.Add(vecrebarDir1);
							ptReabrEnd.Add(vecrebarDir1);
							ptdirEnd = ptDirStart;
							double length = m_insidef.pos[m_insidef.posnum - 1].end - m_insidef.pos[0].str;
							vecrebarDir2.ScaleToLength(length / 10 * uor_per_mm);
							ptdirEnd.Add(vecrebarDir2);
						}
						else
						{
							vecrebarDir1.ScaleToLength(m_insidef.pos[0].str / 10 * uor_per_mm);
							ptDirStart.Add(vecrebarDir1);
							ptdirEnd = ptDirStart;
							double length = m_insidef.pos[m_insidef.posnum - 1].end - m_insidef.pos[0].str;
							vecrebarDir2.ScaleToLength(length / 10 * uor_per_mm);
							ptdirEnd.Add(vecrebarDir2);

							double lastposiontion = m_insidef.pos[m_insidef.posnum - 1].end;
							//double vecLen = vec1.Distance(DVec3d::FromZero());
							vecrebarDir2.Negate();
							vecrebarDir2.ScaleToLength(distance1 - lastposiontion);

							ptRebarStart.Add(vecrebarDir2);
							ptReabrEnd.Add(vecrebarDir2);
						}

						linesegment1.SetLineStartPoint(ptDirStart);
						linesegment1.SetLineEndPoint(ptdirEnd);

						lineSeg2.SetLineStartPoint(ptRebarStart);
						lineSeg2.SetLineEndPoint(ptReabrEnd);
					}
					else
					{
						vecrebarDir1.Negate();
						vecrebarDir1.ScaleToLength(m_insidef.pos[0].str / 10 * uor_per_mm);
						ptdirEnd.Add(vecrebarDir1);
						ptDirStart = ptdirEnd;
						double length = m_insidef.pos[m_insidef.posnum - 1].end - m_insidef.pos[0].str;
						vecrebarDir2.Negate();
						vecrebarDir2.ScaleToLength(length / 10 * uor_per_mm);
						ptDirStart.Add(vecrebarDir2);
						if (COMPARE_VALUES_EPS(m_LineSeg2.GetLineStartPoint().x, m_LineSeg1.GetLineStartPoint().x, 10) == 0)
						{
							double lastposiontion = m_insidef.pos[m_insidef.posnum - 1].end;
							//	double vecLen = vec2.Distance(DVec3d::FromZero());
							vecrebarDir2.Negate();
							vecrebarDir2.ScaleToLength(distance1 - lastposiontion);
							ptRebarStart.Add(vecrebarDir2);
							ptReabrEnd.Add(vecrebarDir2);
						}
						else
						{
							vecrebarDir2.ScaleToLength(m_insidef.pos[0].str / 10 * uor_per_mm);
							ptRebarStart.Add(vecrebarDir2);
							ptReabrEnd.Add(vecrebarDir2);
						}


						linesegment1.SetLineStartPoint(ptDirStart);
						linesegment1.SetLineEndPoint(ptdirEnd);

						lineSeg2.SetLineStartPoint(ptRebarStart);
						lineSeg2.SetLineEndPoint(ptReabrEnd);
					}
				}*/
				if (m_sidetype == SideType::In)
				{
					double maxLength = 0;
					for (auto it : m_insidef.pos)
					{
						double length = it.end - it.str;
						if (maxLength < length && COMPARE_VALUES_EPS(length, m_slabThickness, 50) != 0)
							maxLength = length;
					}
					for (int j = 0; j < m_insidef.posnum; j++)
					{
						double nowLen = m_insidef.pos[j].end - m_insidef.pos[j].str;
						double tmpStartOffset = startOffset;
						if (m_insidef.pos[j].str > 5 * UOR_PER_MilliMeter)
							tmpStartOffset += m_insidef.pos[j].str;
						m_insidef.strval = m_insidef.pos[j].strval;
						m_insidef.endval = m_insidef.pos[j].endval;
						if (m_zCorner && nowLen < 1201 * UOR_PER_MilliMeter && nowLen != maxLength)//最边缘的外侧面的起始和终端两条钢筋需要删除
						{
							m_strDelete = true;
							m_endDelete = true;
						}
						PopvecSetId().push_back(0);
						tag = MakeRebars(PopvecSetId().back(), lineSeg2, linesegment1, rebarDir, strRebarSize, nowLen, spacing, tmpStartOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
						if (NULL != tag)
						{
							tag->SetBarSetTag(j + 1);
							rsetTags.Add(tag);
						}
						m_strDelete = false;
						m_endDelete = false;
					}
				}
				else if (m_sidetype == SideType::Out)
				{
					double maxLength = 0;
					for (auto it : m_outsidef.pos)
					{
						double length = it.end - it.str;
						if (maxLength < length && COMPARE_VALUES_EPS(length, m_slabThickness, 50) != 0)
							maxLength = length;
					}
					for (int j = 0; j < m_outsidef.posnum; j++)
					{
						double nowLen = m_outsidef.pos[j].end - m_outsidef.pos[j].str;
						double tmpStartOffset = startOffset;
						if (m_outsidef.pos[j].str > 5 * UOR_PER_MilliMeter)
							tmpStartOffset += m_outsidef.pos[j].str;
						m_outsidef.strval = m_outsidef.pos[j].strval;
						m_outsidef.endval = m_outsidef.pos[j].endval;
						if (nowLen < 1201 * UOR_PER_MilliMeter && nowLen != maxLength)//最边缘的外侧面的起始和终端两条钢筋需要删除
						{
							m_strDelete = true;
							m_endDelete = true;
						}
						PopvecSetId().push_back(0);
						tag = MakeRebars(PopvecSetId().back(), lineSeg2, linesegment1, rebarDir, strRebarSize, nowLen, spacing, tmpStartOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
						if (NULL != tag)
						{
							tag->SetBarSetTag(j + 1);
							rsetTags.Add(tag);
						}
						m_strDelete = false;
						m_endDelete = false;
					}
				}
				else
				{
					tag = MakeRebars(PopSetIds().at(i), lineSeg2, linesegment1, rebarDir, strRebarSize, linesegment1.GetLength(), spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
					if (NULL != tag)
					{
						tag->SetBarSetTag(i + 1);
						rsetTags.Add(tag);
					}
				}
				vecEndType.clear();
			}
			else
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					CVector3D rebarVec = m_LineSeg1.GetLineVec();
					if (k == 0 && m_bStartAnhorselslantedFace)
					{
						vecEndNormal[k] = m_vecEndNormalStart;
						m_bStartAnhorselslantedFace = false;
					}
					else if (k == 1 && m_bEndAnhorselslantedFace)
					{
						vecEndNormal[k] = m_vecEndNormalEnd;
						m_bEndAnhorselslantedFace = false;
					}
					else
					{
						endNormal = GetfaceNormal();
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
						vecEndNormal[k] = endNormal;
					}
				}

				DVec3d vec = m_LineSeg2.GetLineVec();
				LineSegment linesegment2 = m_LineSeg2;
				/*Dpoint3d ptDirStart = linesegment2.GetLineStartPoint();
				Dpoint3d ptdirEnd = linesegment2.GetLineEndPoint();
				//DVec3d vecrebarDir = ptdirEnd - ptDirStart;
				DVec3d vecrebarDir1 = ptdirEnd - ptDirStart;
				DVec3d vecrebarDir2 = ptdirEnd - ptDirStart;

				Dpoint3d ptRebarStart = lineSeg1.GetLineStartPoint();
				Dpoint3d ptReabrEnd = lineSeg1.GetLineEndPoint();
				//改变内外侧钢筋布置起点
				if (m_sidetype == SideType::In && vec.DotProduct(DVec3d::From(0, -1, 0)) > 0.9)
				{
					ptRebarStart.y = ptdirEnd.y;
					ptReabrEnd.y = ptdirEnd.y;
					linesegment2.SetLineStartPoint(ptdirEnd);
					linesegment2.SetLineEndPoint(ptDirStart);
					lineSeg1.SetLineStartPoint(ptRebarStart);
					lineSeg1.SetLineEndPoint(ptReabrEnd);
					vec.Negate();
				}
				else if (m_sidetype == SideType::Out && vec.DotProduct(DVec3d::From(0, 1, 0)) > 0.9)
				{
					ptRebarStart.y = ptdirEnd.y;
					ptReabrEnd.y = ptdirEnd.y;
					linesegment2.SetLineStartPoint(ptdirEnd);
					linesegment2.SetLineEndPoint(ptDirStart);
					lineSeg1.SetLineStartPoint(ptRebarStart);
					lineSeg1.SetLineEndPoint(ptReabrEnd);
					vec.Negate();
				}*/

				/*if (m_sidetype == SideType::In)
				{
					if (vec2.DotProduct(DVec3d::From(0, 0, 1)) > 0.9)
					{
						vecrebarDir1.ScaleToLength(m_insidef.pos[0].str / 10 * uor_per_mm);
						ptDirStart.Add(vecrebarDir1);
						if (COMPARE_VALUES_EPS(m_LineSeg2.GetLineStartPoint().y, m_LineSeg1.GetLineStartPoint().y, 10) == 0)
						{
							ptRebarStart.Add(vecrebarDir1);
							ptReabrEnd.Add(vecrebarDir1);
							ptdirEnd = ptDirStart;
							double length = m_insidef.pos[m_insidef.posnum - 1].end - m_insidef.pos[0].str;
							vecrebarDir2.ScaleToLength(length / 10 * uor_per_mm);
							//vecrebarDir2.Negate();
							ptdirEnd.Add(vecrebarDir2);
						}
						else
						{

							double length = m_insidef.pos[m_insidef.posnum - 1].end - m_insidef.pos[0].str;
							//vecrebarDir2.Negate();
							vecrebarDir2.ScaleToLength(length / 10 * uor_per_mm);
							ptdirEnd.Add(vecrebarDir2);
							vecrebarDir2.Negate();
							double lastposiontion = m_insidef.pos[m_insidef.posnum - 1].end;
							//double vecLen = vec2.Distance(DVec3d::FromZero());
							vecrebarDir2.Negate();
							vecrebarDir2.ScaleToLength(distance2 - lastposiontion);
							ptRebarStart.Add(vecrebarDir2);
							ptReabrEnd.Add(vecrebarDir2);
						}
						linesegment2.SetLineStartPoint(ptDirStart);
						linesegment2.SetLineEndPoint(ptdirEnd);
						lineSeg1.SetLineStartPoint(ptRebarStart);
						lineSeg1.SetLineEndPoint(ptReabrEnd);
					}
					else
					{
						vecrebarDir1.Negate();
						vecrebarDir1.ScaleToLength(m_insidef.pos[0].str / 10 * uor_per_mm);
						ptdirEnd.Add(vecrebarDir1);
						ptDirStart = ptdirEnd;
						if (COMPARE_VALUES_EPS(m_LineSeg2.GetLineStartPoint().y, m_LineSeg1.GetLineStartPoint().y, 10) == 0)
						{
							double length = m_insidef.pos[m_insidef.posnum - 1].end - m_insidef.pos[0].str;
							vecrebarDir2.Negate();
							vecrebarDir2.ScaleToLength(length / 10 * uor_per_mm);
							ptDirStart.Add(vecrebarDir2);
							double lastposiontion = m_insidef.pos[m_insidef.posnum - 1].end;
							//	double vecLen = vec2.Distance(DVec3d::FromZero());
							vecrebarDir2.Negate();
							vecrebarDir2.ScaleToLength(distance2 - lastposiontion);
							ptRebarStart.Add(vecrebarDir2);
							ptReabrEnd.Add(vecrebarDir2);
						}
						else
						{
							double length = m_insidef.pos[m_insidef.posnum - 1].end - m_insidef.pos[0].str;
							vecrebarDir2.Negate();
							vecrebarDir2.ScaleToLength(length / 10 * uor_per_mm);
							ptDirStart.Add(vecrebarDir2);

							vecrebarDir2.ScaleToLength(m_insidef.pos[0].str / 10 * uor_per_mm);
							ptRebarStart.Add(vecrebarDir2);
							ptReabrEnd.Add(vecrebarDir2);
						}
						linesegment2.SetLineStartPoint(ptDirStart);
						linesegment2.SetLineEndPoint(ptdirEnd);
						lineSeg1.SetLineStartPoint(ptRebarStart);
						lineSeg1.SetLineEndPoint(ptReabrEnd);
					}
				}

				else */
				if (m_sidetype == SideType::Nor)
				{
					DealWintHoriRebar(lineSeg1, linesegment2, i);

				}
				offset += levelspacing;
				if (0 == i)	//首层偏移当前钢筋直径
				{
					offset += diameter * 0.5;
				}
				else
				{
					double diameterPre = RebarCode::GetBarDiameter(GetMainRebars().at(i - 1).rebarSize, modelRef);	//上一层钢筋直径
					offset += diameterPre * 0.5;	//偏移上一层钢筋的半径
					offset += diameter * 0.5;		//偏移当前层钢筋的半径
				}

				lineSeg1.PerpendicularOffset(offset, GetfaceNormal());

				//ChangeRebarLine(lineSeg1);

				lineSeg1.PerpendicularOffset(updownSideCover + diameter * 0.5, vec);//再偏移钢筋半个直径
				if (_ehCrossPlanePre.IsValid())
				{
					vecEndType[_anchorPos].endType = 7;
					vecEndType[_anchorPos].endPtInfo.value1 = _d;
				}
				if (_ehCrossPlaneNext.IsValid())
				{
					vecEndType[_bendPos].endType = 7;
					vecEndType[_bendPos].endPtInfo.value1 = _d;
				}

				if (m_sidetype == SideType::In)
				{
					double maxLength = 0;
					for (auto it : m_insidef.pos)
					{
						double length = it.end - it.str;
						if (maxLength < length && COMPARE_VALUES_EPS(length, m_slabThickness, 50) != 0)
							maxLength = length;
					}
					for (int j = 0; j < m_insidef.posnum; j++)
					{
						double nowLen = m_insidef.pos[j].end - m_insidef.pos[j].str;
						double tmpStartOffset = startOffset + m_insidef.pos[j].str;
						m_insidef.strval = m_insidef.pos[j].strval;
						m_insidef.endval = m_insidef.pos[j].endval;
						if (m_zCorner && nowLen < 1201 * UOR_PER_MilliMeter && nowLen != maxLength)//最边缘的外侧面的起始和终端两条钢筋需要删除
						{
							m_strDelete = true;
							m_endDelete = true;
						}
						PopvecSetId().push_back(0);
						tag = MakeRebars(PopvecSetId().back(), lineSeg1, linesegment2, rebarDir, strRebarSize, nowLen, spacing, tmpStartOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
						if (NULL != tag)
						{
							tag->SetBarSetTag(j + 1);
							rsetTags.Add(tag);
						}
						m_strDelete = false;
						m_endDelete = false;
					}
				}
				else if (m_sidetype == SideType::Out)
				{
					double maxLength = 0;
					for (auto it : m_outsidef.pos)
					{
						double length = it.end - it.str;
						if (maxLength < length && COMPARE_VALUES_EPS(length, m_slabThickness, 50) != 0)
							maxLength = length;
					}
					for (int j = 0; j < m_outsidef.posnum; j++)
					{
						double nowLen = m_outsidef.pos[j].end - m_outsidef.pos[j].str;
						double tmpStartOffset = startOffset + m_outsidef.pos[j].str;
						m_outsidef.strval = m_outsidef.pos[j].strval;
						m_outsidef.endval = m_outsidef.pos[j].endval;
						if (nowLen < 1201 * UOR_PER_MilliMeter && nowLen != maxLength)//最边缘的外侧面的起始和终端两条钢筋需要删除
						{
							m_strDelete = true;
							m_endDelete = true;
						}
						PopvecSetId().push_back(0);
						tag = MakeRebars(PopvecSetId().back(), lineSeg1, linesegment2, rebarDir, strRebarSize, nowLen, spacing, tmpStartOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
						if (NULL != tag)
						{
							tag->SetBarSetTag(j + 1);
							rsetTags.Add(tag);
						}
						m_strDelete = false;
						m_endDelete = false;
					}
				}
				else
				{
					for (int j = 0; j < m_norsidef.posnum; j++)
					{
						double nowLen = m_norsidef.pos[j].end - m_norsidef.pos[j].str;
						double tmpStartOffset = startOffset + m_norsidef.pos[j].str;
						PopvecSetId().push_back(0);
						tag = MakeRebars(PopvecSetId().back(), lineSeg1, linesegment2, rebarDir, strRebarSize, nowLen, spacing, tmpStartOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
						if (NULL != tag)
						{
							tag->SetBarSetTag(j + 1);
							rsetTags.Add(tag);
						}
					}
					// tag = MakeRebars(PopSetIds().at(i), lineSeg1, linesegment2, rebarDir, strRebarSize, linesegment2.GetLength(), spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
					// if (NULL != tag)
					// {
					// 	tag->SetBarSetTag(i + 1);
					// 	rsetTags.Add(tag);
					// }
				}

				vecEndType.clear();
			}

			m_zCorner = nullptr;
			// 删除属于当前成的钢筋
			auto itrTemp = mapRebarPoint.find(DataExchange);
			if (itrTemp != mapRebarPoint.end())
			{
				mapRebarPoint.erase(itrTemp);
			}
			// end

			if (m_vecRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetMainRebars().at(i).rebarLevel;
					rbPt.vecDir = rebarDir;
					rbPt.ptstr = m_vecRebarPtsLayer.at(m);
					rbPt.ptend = m_vecRebarPtsLayer.at(n);
					rbPt.DataExchange = DataExchange;
					g_vecRebarPtsNoHole.push_back(rbPt);
					m++;
				}

			}
			if (m_vecTwinRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecTwinRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetMainRebars().at(i).rebarLevel;
					rbPt.vecDir = rebarDir;
					rbPt.ptstr = m_vecTwinRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTwinRebarPtsLayer.at(n);
					rbPt.DataExchange = DataExchange;
					g_vecTwinRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
		}

		for (auto itr = mapRebarPoint.begin(); itr != mapRebarPoint.end(); itr++)
		{
			g_vecRebarPtsNoHole.insert(g_vecRebarPtsNoHole.end(), itr->second.begin(), itr->second.end());
		}

		if (FacePreviewButtonsDown)
		{
			for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)
			{
				vector<DSegment3d> vcttemp(*it);
				for (int x = 0; x < vcttemp.size(); x++)
				{
					DPoint3d strPoint = DPoint3d::From(vcttemp[x].point[0].x, vcttemp[x].point[0].y, vcttemp[x].point[0].z);
					DPoint3d endPoint = DPoint3d::From(vcttemp[x].point[1].x, vcttemp[x].point[1].y, vcttemp[x].point[1].z);
					EditElementHandle eeh;
					LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
					eeh.AddToModel();
					m_allLines.push_back(eeh.GetElementRef());//存储所有画线
				}
			}
		}

		// 在使用完 m_allEehs 后释放内存
		FreeAll(m_allEehs);
		m_allEehs.clear();  // 清空 vector

		if (g_globalpara.Getrebarstyle() != 0)
		{
			return (SUCCESS == AddRebarSets(rsetTags));
		}
		return true;
	}



}


std::vector<ElementHandle> PlaneRebarAssembly::scan_elements_in_range(const DRange3d & range, std::function<bool(const ElementHandle&)> filter)
{
	std::vector<ElementHandle> ehs;

	std::map<ElementId, IComponent *> components;
	std::map<ElementId, MSElementDescrP> descriptions;

	/// 扫描在包围盒范围内的构件
	if (!PITCommonTool::CModelTool::AnalysisModelGetElements(range, components, descriptions))
	{
		// 返回空的即可
		return ehs;
	}

	const auto PLUS_ID = 1000000;

	ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
	for (const auto &kv : descriptions)
	{
		if (kv.second == NULL) continue;
		auto id = kv.first;
		// 这里返回的id可能加了一个PlusID以防止重复
		if (id >= PLUS_ID)
		{
			id -= PLUS_ID;
		}
		//const auto component = kv.second;
		// const auto desc = kv.second;

		// 扫描所有的model_ref，找到该元素所在的model_ref

		for (DgnModelRefP modelRef : modelRefCol)
		{
			EditElementHandle tmpeeh;
			if (tmpeeh.FindByID(id, modelRef) == SUCCESS)
			{
				auto eh = ElementHandle(id, modelRef);
				if (filter(eh))
				{
					// 满足条件，加入到输出 
					ehs.push_back(std::move(eh));
				}
			}

		}
	}

	// 排序+去重
	std::sort(
		ehs.begin(), ehs.end(),
		[](const ElementHandle &eh_a, const ElementHandle &eh_b) -> bool
	{
		return eh_a.GetElementId() < eh_b.GetElementId();
	});

	auto new_end = std::unique(
		ehs.begin(), ehs.end(),
		[](const ElementHandle &eh_a, const ElementHandle &eh_b) -> bool
	{
		return eh_a.GetElementId() == eh_b.GetElementId();
	});

	return std::vector<ElementHandle>(ehs.begin(), new_end);
}

bool PlaneRebarAssembly::is_Wall(const ElementHandle & element)
{
	std::string _name, type;
	if (!GetEleNameAndType(const_cast<ElementHandleR>(element), _name, type))
	{
		return false;
	}
	auto result_pos_wall = type.find("WALL");
	return result_pos_wall != std::string::npos;
}

double PlaneRebarAssembly::WallRebars_OffsetLength(DPoint3dCR RebarPt)
{
	auto GetDiameter = [&](std::vector<PIT::ConcreteRebar>& my_vecRebarData, ElementId contid) -> double
	{
		//获取sizekey
		auto it = my_vecRebarData.begin();
		for (; it != my_vecRebarData.end(); it++)
		{
			BrString strRebarSize = it->rebarSize;
			if (strRebarSize != L"")
			{
				if (strRebarSize.Find(L"mm") != -1)
				{
					strRebarSize.Replace(L"mm", L"");
				}
			}
			else
			{
				strRebarSize = XmlManager::s_alltypes[it->rebarType];
			}
			strcpy(it->rebarSize, CT2A(strRebarSize));
			GetDiameterAddType(it->rebarSize, it->rebarType);
		}
		//获取sizekey
		//墙的背面的纵筋直径
		double diameter1 = 0.0;
		double diameter2 = 0.0;
		char rebarSize1[512] = "16C";
		char rebarSize2[512] = "20C";
		diameter1 = RebarCode::GetBarDiameter(rebarSize1, ACTIVEMODEL);
		diameter2 = RebarCode::GetBarDiameter(rebarSize2, ACTIVEMODEL);
		if ((contid != 0) && (my_vecRebarData.size() != 0))
		{
			diameter1 = RebarCode::GetBarDiameter(my_vecRebarData[0].rebarSize, ACTIVEMODEL);
			diameter2 = RebarCode::GetBarDiameter(my_vecRebarData[1].rebarSize, ACTIVEMODEL);
		}
		return diameter1 + diameter2;
	};
	for (auto walleh : m_Allwalls)
	{
		DRange3d tmpWall_Range;
		mdlElmdscr_computeRange(&tmpWall_Range.low, &tmpWall_Range.high, walleh.GetElementDescrCP(), nullptr);
		if (tmpWall_Range.IsContainedXY(RebarPt))
		{
			// 获取在钢筋断点上面的墙的信息，得到钢筋信息
			vector<PIT::ConcreteRebar>	my_vecRebarData;
			WallRebarInfo my_wallRebarInfo;
			my_wallRebarInfo.concrete.reverseCover = 50;
			ElementId contid = 0;
			if (nullptr != walleh.GetElementRef())
				GetElementXAttribute(walleh.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, walleh.GetModelRef());
			GetElementXAttribute(contid, sizeof(PIT::WallRebarInfo), my_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
			GetElementXAttribute(contid, my_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
			double diameter = GetDiameter(my_vecRebarData, contid);
			return diameter;
		}
	}
	DRange3d face_Range;
	mdlElmdscr_computeRange(&face_Range.low, &face_Range.high, m_face.GetElementDescrCP(), nullptr);
	if (!face_Range.IsContainedXY(RebarPt))
	{
		//Z型板拐角钢筋可能没有锚固实际墙体，暂时使用本身钢筋信息
		vector<PIT::ConcreteRebar> vecRebarData = GetMainRebars();
		ElementId contid = 0;
		GetElementXAttribute(m_pOldElm->GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, m_pOldElm->GetModelRef());
		double diameter = GetDiameter(vecRebarData, contid);
		return diameter;
	}

	return 0.0;
}

void PlaneRebarAssembly::RemoveRepeatPoint(vector<DPoint3d>& vecPoint)
{
	vector<DPoint3d> vectemp;
	vector<int> vecindex;
	for (size_t i = 0; i < vecPoint.size(); i++)
	{
		vector<int>::iterator it = std::find(vecindex.begin(), vecindex.end(), i);
		if (it == vecindex.end())
		{
			for (size_t j = i + 1; j < vecPoint.size(); j++)
			{
				if (vecPoint[i].Distance(vecPoint[j]) < 10)//相等则去除
				{
					vecindex.push_back(i);
					vecindex.push_back(j);
				}
			}
		}
	}
	for (size_t i = 0; i < vecPoint.size(); i++)
	{
		vector<int>::iterator it = std::find(vecindex.begin(), vecindex.end(), i);
		if (it == vecindex.end())
		{
			vectemp.push_back(vecPoint[i]);
		}
	}
	vecPoint.swap(vectemp);
}

double PlaneRebarAssembly::GetLae()
{
	if (g_globalpara.m_alength.find("A") != g_globalpara.m_alength.end())
	{
		auto iter = g_globalpara.m_alength.find("A");
		return iter->second * UOR_PER_MilliMeter;
	}
	else if (g_globalpara.m_alength.find("B") != g_globalpara.m_alength.end())
	{
		auto iter = g_globalpara.m_alength.find("B");
		return iter->second * UOR_PER_MilliMeter;
	}
	else if (g_globalpara.m_alength.find("C") != g_globalpara.m_alength.end())
	{
		auto iter = g_globalpara.m_alength.find("C");
		return iter->second * UOR_PER_MilliMeter;
	}
	else if (g_globalpara.m_alength.find("D") != g_globalpara.m_alength.end())
	{
		auto iter = g_globalpara.m_alength.find("D");
		return iter->second * UOR_PER_MilliMeter;
	}
	else
	{
		return -1.0;
	}
}



bool PlaneRebarAssembly::OnDoubleClick()
{

	ElementId testid = GetElementId();
	PlaneRebarAssembly* faceRebar = REA::Load<PlaneRebarAssembly>(testid, ACTIVEMODEL);



	ElementId tmpid = GetSelectedElement();
	if (tmpid == 0)
	{
		return false;
	}
	DgnModelRefP modelp = GetSelectedModel();
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
	// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// 	CFacesRebarDlg dlg(ehSel,CWnd::FromHandle(MSWIND));
	// 	
	// 	dlg.SetConcreteId(GetConcreteOwner());
	// 	if (IDCANCEL == dlg.DoModal())
	// 		return false;

	return true;
}

bool PlaneRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();

	//	SetWallData(ehWall);
	MakeRebars(modelRef);
	Save(modelRef);
	return true;
}

bool PlaneRebarAssembly::AnalyzingFaceGeometricData(EditElementHandleR eeh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eeh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	// 面配筋时求板的厚度
	// 遇到集水坑等被切割一部分的实体时，无法使用平行面距离作为厚度，厚度转移至AnalyzingFloorData计算
	/*MSElementDescrP msDscp1 = eeh.GetElementDescrP();
	if (msDscp1 == NULL)
	{
		return false;
	}

	bool isWall = is_Wall(*m_Solid);
	DVec3d vecNormal = DVec3d::From(0, 0, 1);
	DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
	DVec3d vecfacenor;
	mdlElmdscr_extractNormal(&vecfacenor, nullptr, msDscp1, &ptDefault);
	std::vector<EditElementHandle*> allFaces;//该元素所有的面
	std::vector<EditElementHandle*> allParalFaces;//与所选面平行的所有面
	ExtractFacesTool::GetFaces(*m_Solid, allFaces);
	for (size_t index = 0; index < allFaces.size(); index++)
	{
		DVec3d vecface;
		mdlElmdscr_extractNormal(&vecface, nullptr, allFaces[index]->GetElementDescrP(), &ptDefault);
		if ((!isWall && abs(vecface.DotProduct(vecNormal)) > 0.9) ||//板只采用法线平行Z轴的面
			(isWall && abs(vecface.DotProduct(vecfacenor)) > 0.9))//墙只采用选中面平行的面
		{
			allParalFaces.push_back(allFaces[index]);
		}
	}
	Dpoint3d ptmin, ptmax;
	if(!isWall && !allParalFaces.empty())//板只采用法线平行Z轴的面
		mdlElmdscr_computeRange(&ptmin, &ptmax, allParalFaces[0]->GetElementDescrCP(), nullptr);
	else
		mdlElmdscr_computeRange(&ptmin, &ptmax, msDscp1, nullptr);
	Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);
	double distance = 0.0;
	for (size_t index = 0; index < allParalFaces.size(); index++)
	{
		Dpoint3d ptfacemin, ptfacemax;
		mdlElmdscr_computeRange(&ptfacemin, &ptfacemax, allParalFaces[index]->GetElementDescrCP(), nullptr);
		Dpoint3d ptfaceMid = Dpoint3d::From((ptfacemin.x + ptfacemax.x) / 2, (ptfacemin.y + ptfacemax.y) / 2, (ptfacemin.z + ptfacemax.z) / 2);
		DVec3d vecface;
		mdlElmdscr_extractNormal(&vecface, nullptr, allParalFaces[index]->GetElementDescrP(), &ptDefault);
		Dpoint3d ptproject;
		mdlVec_projectPointToPlane(&ptproject, &ptmid, &ptfaceMid, &vecface);
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, *allParalFaces[index]) == SUCCESS)
		{
			if (!SolidUtil::IsPointInsideBody(*entityPtr, ptproject))
			{
				Dpoint3d ptOut = Dpoint3d::FromZero();
				StatusInt status = mdlMeasure_closestPointOnElement(&ptOut, allParalFaces[index], NULL, &ptproject);
				if (status == SUCCESS)
				{
					double dDistanceXY = ptOut.DistanceXY(ptproject);
					if ((abs(ptmid.x - ptOut.x) > 100 && abs(ptmid.y - ptOut.y) > 100) && dDistanceXY > 10 && (ptproject.z < ptfacemin.z || ptproject.z > ptfacemin.z))//有误差，所以不能完全判断是否是面的点
					{
						continue;
					}
				}
				else
				{
					continue;
				}
			}
		}

		double ditanceBetweenFace = ptmid.Distance(ptproject);
		if (ditanceBetweenFace < 100 * UOR_PER_MilliMeter)
		{
			continue;
		}
		else if (distance < 1 || ditanceBetweenFace < distance)
		{
			distance = ditanceBetweenFace;
		}

	}
	if (distance != 0)
		m_slabThickness = distance;
	//尽可能消除噪声
	m_slabThickness = round(m_slabThickness / 100.0) * 100.0;*/

	//面配筋时求板的厚度

	if (GetfaceType() == FaceType::other)
	{
		MSElementDescrP msDscp = eeh.GetElementDescrP();
		if (msDscp == NULL)
		{
			return false;
		}

		// 取一个元素的范围
		DPoint3d minP = { 0 }, maxP = { 0 };
		mdlElmdscr_computeRange(&minP, &maxP, msDscp, NULL); // 计算指定元素描述符中元素的范围

		DVec3d ptVec = maxP - minP;
		DPoint3d ptEnd1 = DPoint3d::FromZero();
		DPoint3d ptEnd2 = DPoint3d::FromZero();
		if (fabs(ptVec.z - 0.0) < uor_ref) // XOY 平面
		{
			if (COMPARE_VALUES_EPS(maxP.x - minP.x, maxP.y - minP.y < 0, EPS) >= 0)
			{
				ptEnd1 = maxP;
				ptEnd1.y = minP.y;

				ptEnd2 = maxP;
				ptEnd2.x = minP.x;
			}
			else
			{
				ptEnd1 = maxP;
				ptEnd1.x = minP.x;

				ptEnd2 = maxP;
				ptEnd2.y = minP.y;
			}

		}
		else if (fabs(ptVec.x - 0.0) < uor_ref) // YOZ平面
		{
			if (COMPARE_VALUES_EPS(maxP.y - minP.y, maxP.z - minP.z < 0, EPS) >= 0)
			{
				ptEnd1 = maxP;
				ptEnd1.z = minP.z;

				ptEnd2 = maxP;
				ptEnd2.y = minP.y;
			}
			else
			{
				ptEnd1 = maxP;
				ptEnd1.y = minP.y;

				ptEnd2 = maxP;
				ptEnd2.z = minP.z;
			}
		}
		else if (fabs(ptVec.y - 0.0) < uor_ref) // XOZ平面
		{
			if (COMPARE_VALUES_EPS(maxP.x - minP.x, maxP.z - minP.z < 0, EPS) >= 0)
			{
				ptEnd1 = maxP;
				ptEnd1.z = minP.z;

				ptEnd2 = maxP;
				ptEnd2.x = minP.x;
			}
			else
			{
				ptEnd1 = maxP;
				ptEnd1.x = minP.x;

				ptEnd2 = maxP;
				ptEnd2.z = minP.z;
			}
		}
		else
		{
			return false;
		}

		m_LineSeg1.SetLineSeg(DSegment3d::From(minP, ptEnd1));
		m_LineSeg2.SetLineSeg(DSegment3d::From(minP, ptEnd2));

		return true;
	}

	if (m_Solid != NULL && m_vecElm.empty())
	{
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(*m_Solid, Eleeh, Holeehs);
		m_Holeehs = Holeehs;
	}

	for (size_t i = 0; i < m_vecElm.size(); ++i)
	{
		//将孔洞上有板的孔洞过滤
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(m_vecElm[i], Eleeh, Holeehs);
		for (auto it : Holeehs)
		{
			double areap = 0;
			mdlMeasure_areaProperties(nullptr, &areap, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, it->GetElementDescrP(), 0);
			if (areap > BIGHOLEAREA)
			{
				m_Holeehs.push_back(it);
				continue;
			}
			//mdlElmdscr_addByModelRef(it->GetElementDescrP(), ACTIVEMODEL);
			DPoint3d holeMinPt = { 0,0,0 }, holeMaxPt = { 0,0,0 };
			mdlElmdscr_computeRange(&holeMinPt, &holeMaxPt, it->GetElementDescrP(), nullptr);
			DRange3d holeRange = DRange3d::From(holeMinPt, holeMaxPt);
			bool isValid = true;
			vector<MSElementDescrP> elmDecrP;
			for (size_t j = 0; j < m_vecElm.size(); ++j)
			{
				if (j == i)
				{
					continue;
				}
				EditElementHandle elmEeh(m_vecElm[j], m_vecElm[j].GetDgnModelP());
				DPoint3d elmMinPt = { 0,0,0 }, elmMaxPt = { 0,0,0 };
				mdlElmdscr_computeRange(&elmMinPt, &elmMaxPt, elmEeh.GetElementDescrP(), nullptr);
				DPoint3d midPt = elmMinPt; midPt.Add(elmMaxPt); midPt.Scale(0.5);
				if (holeRange.IsContained(midPt))
				{
					isValid = false;
					break;
				}
			}
			if (isValid)
			{
				m_Holeehs.push_back(it);
			}
		}
		//m_Holeehs.insert(m_Holeehs.end(), Holeehs.begin(), Holeehs.end());
	}
	//先转到xoy平面
// 	
// 	RotMatrix rMatrix;
// 	Transform trans;
// 	DPoint3d vecz = DPoint3d::From(0, 0, 1);
// 	mdlRMatrix_getIdentity(&rMatrix);
// 	mdlRMatrix_fromVectorToVector(&rMatrix, &GetfaceNormal(), &vecz);//旋转到xoy平面
// 	mdlTMatrix_fromRMatrix(&trans, &rMatrix);
// 	mdlTMatrix_setOrigin(&trans, &DPoint3d::From(0,0,0));
// 	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, TransformInfo(trans));
	EditElementHandle tmpeeh;
	if (m_UseXOYDir)//如果是板平面，以面的范围面作为基准面
	{
		DPoint3d minP, maxP;
		mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
		DPoint3d PTS[4];
		PTS[0] = PTS[1] = minP;
		PTS[1].x = maxP.x;
		PTS[2] = PTS[1]; PTS[2].y = maxP.y;
		PTS[3] = PTS[2]; PTS[3].x = minP.x;
		ShapeHandler::CreateShapeElement(tmpeeh, NULL, PTS, 4, true, *ACTIVEMODEL);
	}
	else
	{
		tmpeeh.Duplicate(eeh);
	}

	ElementAgenda agenda;//存放打散之后的元素
	DropGeometryPtr pDropGeometry = DropGeometry::Create();//创建一个DropGeometry实例来设置几何选项
	if (tmpeeh.GetElementType() == CMPLX_SHAPE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Complex);
	}
	else if (tmpeeh.GetElementType() == SURFACE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Solids);
	}
	else
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_LinearSegments);
	}
	if (SUCCESS != tmpeeh.GetDisplayHandler()->Drop(tmpeeh, agenda, *pDropGeometry))
	{
		agenda.Clear();
		mdlOutput_printf(MSG_STATUS, L"打散面失败！");
		return false;
	}

	class lineSort {
	public:
		bool operator()(const LineSegment &line1, const LineSegment &line2) {
			return line1.GetLength() > line2.GetLength();
		}
	};
	std::set < LineSegment, lineSort > lineSegs;
	vector<ArcSegment> vecArcSeg;
	for (EditElementHandleR LineEeh : agenda)
	{
		if (LineEeh.GetElementType() == LINE_ELM)
		{
			DPoint3d pt[2];
			mdlLinear_extract(pt, NULL, LineEeh.GetElementP(), tmpeeh.GetModelRef());
			LineSegment lineSeg(pt[0], pt[1]);
			lineSegs.insert(lineSeg);
		}
	}

	// 取长度最长的边
	// mdlElmdscr_extractNormal(&PopfaceNormal(), NULL, tmpeeh.GetElementDescrCP(), NULL);
	//	PopfaceNormal().Negate();
	LineSegment maxLine = *lineSegs.begin();
	CVector3D  xVec(maxLine.GetLineVec());
	CVector3D  yVec = CVector3D(PopfaceNormal()).CrossProduct(xVec);
	BeMatrix   matrix = CMatrix3D::Ucs(maxLine.GetLineStartPoint(), xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	Transform trans;
	matrix.AssignTo(trans);
	trans.InverseOf(trans);
	tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, TransformInfo(trans));
	//tmpeeh.AddToModel();
// 
	if (m_vecElm.size())
	{
		if (m_slabUpFace)
		{
			SetfaceNormal(DVec3d::From(0, 0, -1));
		}
		else
		{
			SetfaceNormal(DVec3d::From(0, 0, 1));
		}
	}
	//计算平面的最大范围
	DPoint3d ptMin, ptMax;
	mdlElmdscr_computeRange(&ptMin, &ptMax, tmpeeh.GetElementDescrCP(), NULL);


	Dpoint3d pt[4];
	int num = 0;
	mdlLinear_extract(pt, &num, tmpeeh.GetElementP(), ACTIVEMODEL);

	vector<Dpoint3d> vecPtDown;
	vector<Dpoint3d> vecPtUp;
	for (int i = 0; i < 4; i++)
	{
		if (COMPARE_VALUES_EPS(pt[i].y, ptMax.y, 10) == 0)
		{
			vecPtUp.push_back(pt[i]);
		}
		else if (COMPARE_VALUES_EPS(pt[i].y, ptMin.y, 10) == 0)
		{
			vecPtDown.push_back(pt[i]);
		}
	}
	DSegment3d lineSeg1;
	DSegment3d lineSeg2;
	if (vecPtDown.size() == 2 && vecPtUp.size() == 2)
	{
		lineSeg1 = vecPtDown[0].x < vecPtDown[1].x ? DSegment3d::From(vecPtDown[0], vecPtDown[1]) : DSegment3d::From(vecPtDown[1], vecPtDown[0]);
		Dpoint3d downpt = vecPtDown[0].x < vecPtDown[1].x ? vecPtDown[0] : vecPtDown[1];
		Dpoint3d uppt = vecPtUp[0].x < vecPtUp[1].x ? vecPtUp[0] : vecPtUp[1];
		lineSeg2 = DSegment3d::From(downpt, uppt);
	}
	else
	{
		DPoint3d ptLineSeg1End = ptMax;
		DPoint3d ptLineSeg2End = ptMin;
		ptLineSeg1End.y = ptMin.y;
		ptLineSeg2End.y = ptMax.y;

		lineSeg1 = DSegment3d::From(ptMin, ptLineSeg1End);
		lineSeg2 = DSegment3d::From(ptMin, ptLineSeg2End);
	}

	/*DPoint3d ptLineSeg1End = ptMax;
	DPoint3d ptLineSeg2End = ptMin;
	ptLineSeg1End.y = ptMin.y;
	ptLineSeg2End.y = ptMax.y;

	DSegment3d lineSeg1 = DSegment3d::From(ptMin, ptLineSeg1End);
	DSegment3d lineSeg2 = DSegment3d::From(ptMin, ptLineSeg2End);*/

	EditElementHandle ehLine1, ehLine2;
	LineHandler::CreateLineElement(ehLine1, NULL, lineSeg1, true, *ACTIVEMODEL);
	LineHandler::CreateLineElement(ehLine2, NULL, lineSeg2, true, *ACTIVEMODEL);
	//ehLine1.AddToModel();
	//ehLine2.AddToModel();

	trans.InverseOf(trans);
	ehLine1.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(ehLine1, TransformInfo(trans));
	ehLine2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(ehLine2, TransformInfo(trans));

	DPoint3d ptLine1[2];
	DPoint3d ptLine2[2];
	mdlLinear_extract(ptLine1, NULL, ehLine1.GetElementCP(), ACTIVEMODEL);
	mdlLinear_extract(ptLine2, NULL, ehLine2.GetElementCP(), ACTIVEMODEL);

	m_LineSeg1.SetLineSeg(DSegment3d::From(ptLine1[0], ptLine1[1]));
	m_LineSeg2.SetLineSeg(DSegment3d::From(ptLine2[0], ptLine2[1]));

	if (/*m_LineSeg1.GetLineVec().IsParallelTo(DVec3d::From(0,0,1))*/ abs(m_LineSeg1.GetLineVec().DotProduct(DVec3d::From(0, 0, 1))) > 0.9
		|| (abs(m_LineSeg1.GetLineVec().DotProduct(DVec3d::From(0, 1, 0))) > 0.9 && abs(m_LineSeg2.GetLineVec().DotProduct(DVec3d::From(1, 0, 0))) > 0.9))
	{
		LineSegment segTmp = m_LineSeg2;
		m_LineSeg2 = m_LineSeg1;
		m_LineSeg1 = segTmp;
	}
	else if (COMPARE_VALUES_EPS(m_LineSeg1.GetLineStartPoint().x, m_LineSeg1.GetLineEndPoint().x, EPS) == 0
		&& COMPARE_VALUES_EPS(m_LineSeg1.GetLineStartPoint().y, m_LineSeg1.GetLineEndPoint().y, EPS) != 0
		&& m_LineSeg2.GetLineVec().IsParallelTo(DVec3d::From(1, 0, 0)))
	{
		//Added by chenxuan 面配筋横向竖向方向不对
		LineSegment segTmp = m_LineSeg2;
		m_LineSeg2 = m_LineSeg1;
		m_LineSeg1 = segTmp;
	}
	//end


	if (_ehCrossPlanePre.IsValid())
	{
		DPoint3d ptNormal;
		DPoint3d ptInFace;
		mdlElmdscr_extractNormal(&ptNormal, &ptInFace, _ehCrossPlanePre.GetElementDescrCP(), NULL);
		if (ptNormal.IsParallelTo(m_LineSeg2.GetLineVec()))
		{
			LineSegment seg = m_LineSeg1;
			m_LineSeg1 = m_LineSeg2;
			m_LineSeg2 = seg;
		}

		//判断垂直面在起点处还是终点处
		double dis1 = ptInFace.Distance(m_LineSeg1.GetLineStartPoint());
		double dis2 = ptInFace.Distance(m_LineSeg1.GetLineEndPoint());
		_anchorPos = dis1 > dis2 ? 1 : 0;
	}
	if (_ehCrossPlaneNext.IsValid())
	{
		DPoint3d ptNormal;
		DPoint3d ptInFace;
		mdlElmdscr_extractNormal(&ptNormal, &ptInFace, _ehCrossPlaneNext.GetElementDescrCP(), NULL);
		if (ptNormal.IsParallelTo(m_LineSeg2.GetLineVec()))
		{
			LineSegment seg = m_LineSeg1;
			m_LineSeg1 = m_LineSeg2;
			m_LineSeg2 = seg;
		}

		//判断垂直面在起点处还是终点处
		double dis1 = ptInFace.Distance(m_LineSeg1.GetLineStartPoint());
		double dis2 = ptInFace.Distance(m_LineSeg1.GetLineEndPoint());
		_bendPos = dis1 > dis2 ? 1 : 0;
	}
	/*testBegin*/
	/*EditElementHandle ehLin1, ehLin2;
	LineHandler::CreateLineElement(ehLin1, NULL, DSegment3d::From(m_LineSeg1.GetLineStartPoint(), m_LineSeg1.GetLineEndPoint()), true, *ACTIVEMODEL);
	LineHandler::CreateLineElement(ehLin2, NULL, DSegment3d::From(m_LineSeg2.GetLineStartPoint(), m_LineSeg2.GetLineEndPoint()), true, *ACTIVEMODEL);
	ehLin1.AddToModel();
	ehLin2.AddToModel();*/
	/*testEnd*/

	// start确保向量是从绝对坐标系最低往最高
	auto SwapLineSeg = [&](PIT::LineSegment& lineSeg1, PIT::LineSegment& lineSeg2,
		Dpoint3d& ptSeg1Start, Dpoint3d& ptSeg1End, Dpoint3d& ptSeg2Start, Dpoint3d& ptSeg2End)
	{
		swap(ptSeg1Start, ptSeg1End);
		lineSeg1.SetLineStartPoint(ptSeg1Start);
		lineSeg1.SetLineEndPoint(ptSeg1End);
		lineSeg2.SetLineStartPoint(ptSeg2Start);
		lineSeg2.SetLineEndPoint(ptSeg2End);
	};
	auto StandardStartAndEnd = [&](PIT::LineSegment& lineSeg1, PIT::LineSegment& lineSeg2)
	{
		DVec3d vec1 = lineSeg1.GetLineVec();
		DVec3d vec2 = lineSeg2.GetLineVec();
		double length1 = lineSeg1.GetLength();
		double length2 = lineSeg2.GetLength();
		Dpoint3d ptSeg1Start = lineSeg1.GetLineStartPoint();
		Dpoint3d ptSeg1End = lineSeg1.GetLineEndPoint();
		Dpoint3d ptSeg2Start = lineSeg2.GetLineStartPoint();
		Dpoint3d ptSeg2End = lineSeg2.GetLineEndPoint();
		if (vec1.DotProduct(DVec3d::From(-1, 0, 0)) > 0.9)
		{
			ptSeg2Start = ptSeg1End;
			movePoint(vec1, ptSeg2End, length1);
			SwapLineSeg(lineSeg1, lineSeg2, ptSeg1Start, ptSeg1End, ptSeg2Start, ptSeg2End);
		}
		else if (vec1.DotProduct(DVec3d::From(0, -1, 0)) > 0.9)
		{
			ptSeg2Start = ptSeg1End;
			movePoint(vec1, ptSeg2End, length1);
			SwapLineSeg(lineSeg1, lineSeg2, ptSeg1Start, ptSeg1End, ptSeg2Start, ptSeg2End);
		}
		else if (vec1.DotProduct(DVec3d::From(0, 0, -1)) > 0.9)
		{
			ptSeg2Start = ptSeg1End;
			movePoint(vec1, ptSeg2End, length1);
			SwapLineSeg(lineSeg1, lineSeg2, ptSeg1Start, ptSeg1End, ptSeg2Start, ptSeg2End);
		}
	};
	StandardStartAndEnd(m_LineSeg1, m_LineSeg2);
	StandardStartAndEnd(m_LineSeg2, m_LineSeg1);
	// end确保向量是从绝对坐标系最低往最高

	PopvecFrontPts().push_back(m_LineSeg1.GetLineStartPoint());
	PopvecFrontPts().push_back(m_LineSeg1.GetLineEndPoint());
	return true;
}

void PlaneRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetConcrete().sideCover*uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

	if (GetConcrete().isHandleHole)//计算需要处理的孔洞
	{
		for (int j = 0; j < m_Holeehs.size(); j++)
		{
			EditElementHandle eeh;
			eeh.Duplicate(*m_Holeehs.at(j));

			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
			}

			TransformInfo transinfo(trans);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			DPoint3d minP;
			DPoint3d maxP;
			//计算指定元素描述符中元素的范围。
			mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
			DRange3d range;
			range.low = minP;
			range.high = maxP;
			bool isNeed = false;
			if (range.XLength() > misssize || range.ZLength() > misssize)
			{
				isNeed = true;
			}

			if (isNeed)
			{
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*m_Holeehs.at(j));
				PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}
}

void GetUpDownWallFaces(PlaneRebarAssembly::LDFloorData& m_ldfoordata)
{
	MSElementDescrP downface = nullptr;
	mdlElmdscr_duplicate(&downface, m_ldfoordata.facedes);

	DVec3d vecZ = DVec3d::From(0, 0, 1);
	double FHight = m_ldfoordata.Zlenth + 10;
	vecZ.Scale(FHight);
	MSElementDescrP upface = nullptr;
	mdlElmdscr_duplicate(&upface, m_ldfoordata.facedes);
	PITCommonTool::CFaceTool::MoveCenterFaceByMidPt(upface, vecZ);
	m_ldfoordata.facedesUp = upface;
	//mdlElmdscr_add(upface);
	//获取底面墙
	EditElementHandle eeh(downface, true, false, ACTIVEMODEL);
	Transform matrix;
	DPoint3d minP = { 0 }, maxP = { 0 };
	mdlElmdscr_computeRange(&minP, &maxP, downface, NULL);
	DPoint3d ptCenter = minP;
	ptCenter.Add(maxP);
	ptCenter.Scale(0.5);
	mdlTMatrix_getIdentity(&matrix);
	mdlTMatrix_scale(&matrix, &matrix, 0.99, 0.99, 1.0);
	mdlTMatrix_setOrigin(&matrix, &ptCenter);
	TransformInfo tInfo(matrix);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, tInfo);

	std::vector<IDandModelref>  Same_Eles;
	GetNowIntersectDatas(eeh, Same_Eles);
	for (int i = 0; i < Same_Eles.size(); i++)
	{
		string Ename, Etype;
		EditElementHandle tmpeeh(Same_Eles.at(i).ID, Same_Eles.at(i).tModel);
		if (GetEleNameAndType(tmpeeh, Ename, Etype))
		{
			if (Etype != "FLOOR")
			{
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(tmpeeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(tmpeeh);
				EditElementHandle upface;
				DPoint3d maxpt;

				EditElementHandle Eleeh;
				std::vector<EditElementHandle*> Holeehs;
				EFT::GetSolidElementAndSolidHoles(tmpeeh, Eleeh, Holeehs);
				//mdlElmdscr_add(tmpeeh.GetElementDescrP());
				DVec3d Normal = DVec3d::From(0, 0, -1);
				Normal.ScaleToLength(500);
				mdlCurrTrans_begin();
				Transform tMatrix;
				mdlTMatrix_getIdentity(&tMatrix);
				mdlTMatrix_setTranslation(&tMatrix, &Normal);
				MSElementDescrP newfaceEeh = nullptr;
				downface->Duplicate(&newfaceEeh);
				mdlElmdscr_transform(&newfaceEeh, &tMatrix);
				mdlCurrTrans_end();
				//mdlElmdscr_add(newfaceEeh);

				MSElementDescrP resultEdp = nullptr;
				PITCommonTool::CSolidTool::SolidBoolWithFace(resultEdp, newfaceEeh, Eleeh.GetElementDescrP(),
					BOOLOPERATION_INTERSECT);
				if (nullptr == resultEdp)
				{
					continue;
				}

				if (!ExtractFacesTool::GetFloorDownFaceForSlab(Eleeh, upface, maxpt, false, downface))
				{
					continue;
				}
				if (upface.IsValid())
				{
					//mdlElmdscr_add(upface.GetElementDescrP());
					m_ldfoordata.downfaces[m_ldfoordata.downnum++] = upface.ExtractElementDescr();
				}

			}
		}
	}

	//获取顶面墙
	EditElementHandle eehup(upface, true, false, ACTIVEMODEL);
	mdlElmdscr_computeRange(&minP, &maxP, upface, NULL);
	ptCenter = minP;
	ptCenter.Add(maxP);
	ptCenter.Scale(0.5);
	mdlTMatrix_getIdentity(&matrix);
	mdlTMatrix_scale(&matrix, &matrix, 0.99, 0.99, 1.0);
	mdlTMatrix_setOrigin(&matrix, &ptCenter);
	TransformInfo tInfo2(matrix);
	eehup.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehup, tInfo2);

	std::vector<IDandModelref>  Same_Elesup;
	GetNowIntersectDatas(eehup, Same_Elesup);
	for (int i = 0; i < Same_Elesup.size(); i++)
	{
		string Ename, Etype;
		EditElementHandle tmpeeh(Same_Elesup.at(i).ID, Same_Elesup.at(i).tModel);
		if (GetEleNameAndType(tmpeeh, Ename, Etype))
		{
			if (Etype != "FLOOR")
			{
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(tmpeeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(tmpeeh);
				EditElementHandle dface;//墙底面
				DPoint3d minpt;
				EditElementHandle Eleeh;
				std::vector<EditElementHandle*> Holeehs;
				EFT::GetSolidElementAndSolidHoles(tmpeeh, Eleeh, Holeehs);

				DVec3d Normal = DVec3d::From(0, 0, 1);
				Normal.ScaleToLength(500);
				mdlCurrTrans_begin();
				Transform tMatrix;
				mdlTMatrix_getIdentity(&tMatrix);
				mdlTMatrix_setTranslation(&tMatrix, &Normal);
				MSElementDescrP newfaceEeh = nullptr;
				upface->Duplicate(&newfaceEeh);
				mdlElmdscr_transform(&newfaceEeh, &tMatrix);
				mdlCurrTrans_end();
				//mdlElmdscr_add(newfaceEeh);

				MSElementDescrP resultEdp = nullptr;
				PITCommonTool::CSolidTool::SolidBoolWithFace(resultEdp, newfaceEeh, Eleeh.GetElementDescrP(),
					BOOLOPERATION_INTERSECT);
				if (nullptr == resultEdp)
				{
					continue;
				}

				if (!ExtractFacesTool::GetFloorDownFaceForSlab(Eleeh, dface, minpt, true, upface))
				{
					continue;
				}
				if (dface.IsValid())
				{
					//mdlElmdscr_add(dface.GetElementDescrP());
					m_ldfoordata.upfaces[m_ldfoordata.upnum++] = dface.ExtractElementDescr();
				}
			}
		}
	}
}

bool PlaneRebarAssembly::AnalyzingFloorData(ElementHandleCR eh)
{

	//分析选择的实体类型
	if (WallHelper::is_wall(eh))
	{
		m_solidType = 1;
	}
	else
	{
		m_solidType = 0;
	}

	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	//m_model = model;

	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);

	EditElementHandle copyEleeh;
	copyEleeh.Duplicate(Eleeh);
	ElementCopyContext copier2(ACTIVEMODEL);
	copier2.SetSourceModelRef(Eleeh.GetModelRef());
	copier2.SetTransformToDestination(true);
	copier2.SetWriteElements(false);
	copier2.DoCopy(copyEleeh);
	if (m_pOldElm == NULL)
	{
		m_pOldElm = new EditElementHandle();
	}
	m_pOldElm->Duplicate(Eleeh);
	//计算厚度
	Dpoint3d ptmin, ptmax;
	mdlElmdscr_computeRange(&ptmin, &ptmax, m_pOldElm->GetElementDescrP(), nullptr);
	Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);
	double maxLength = ptmin.Distance(ptmax);
	auto GetThickness = [&](DVec3d vec, Dpoint3d ptmid, double length) -> double
	{
		vector<DPoint3d> tmpptsTmp;
		DPoint3d startPt, endPt;
		startPt = endPt = ptmid;
		vec.ScaleToLength(length);
		endPt.Add(vec);
		vec.Negate();
		startPt.Add(vec);

		if (!GetIntersectPointsWithOldElmOwner(tmpptsTmp, m_pOldElm, startPt, endPt, 0))
			return -1;
		return startPt.Distance(endPt);
	};
	// 分别计算三个方向上的厚度
	DVec3d axisX = DVec3d::From(1, 0, 0);  // 平行于 X 轴
	DVec3d axisY = DVec3d::From(0, 1, 0);  // 平行于 Y 轴
	DVec3d axisZ = DVec3d::From(0, 0, 1);  // 平行于 Z 轴
	// 计算三个方向的厚度
	double thicknessX = GetThickness(axisX, ptmid, maxLength);
	double thicknessY = GetThickness(axisY, ptmid, maxLength);
	double thicknessZ = GetThickness(axisZ, ptmid, maxLength);
	// 选择最小的有效厚度
	double validThickness = -1;
	if (thicknessX >= 0 && (validThickness == -1 || thicknessX < validThickness))
	{
		validThickness = thicknessX;
	}
	if (thicknessY >= 0 && (validThickness == -1 || thicknessY < validThickness))
	{
		validThickness = thicknessY;
	}
	if (thicknessZ >= 0 && (validThickness == -1 || thicknessZ < validThickness))
	{
		validThickness = thicknessZ;
	}
	if (validThickness >= 0)
	{
		m_slabThickness = round(validThickness / 100.0) * 100.0;;
	}

	DPoint3d minP2, maxP2;
	//计算指定元素描述符中元素的范围。
	MSElementDescrP copyEdp = copyEleeh.GetElementDescrP();
	mdlElmdscr_computeRange(&minP2, &maxP2, copyEdp, NULL);
	if (!Eleeh.IsValid())
	{
		mdlDialog_dmsgsPrint(L"非法的板实体!");
		return false;
	}

	DPoint3d minPos;
	EditElementHandle downface;
	if (!ExtractFacesTool::GetFloorDownFaceForSlab(copyEleeh, downface, minPos, true, const_cast<MSElementDescrP>(m_face.GetElementDescrCP())))
	{
		return false;
	}
	Dpoint3d ptfaceStartdown, ptfaceEnddown;
	mdlElmdscr_computeRange(&ptfaceStartdown, &ptfaceEnddown, downface.GetElementDescrCP(), nullptr);
	Dpoint3d midDown = DPoint3d::From((ptfaceStartdown.x + ptfaceEnddown.x) / 2, (ptfaceStartdown.y + ptfaceEnddown.y) / 2, (ptfaceStartdown.z + ptfaceEnddown.z) / 2);

	Dpoint3d ptfaceStart, ptfaceEnd;
	mdlElmdscr_computeRange(&ptfaceStart, &ptfaceEnd, m_face.GetElementDescrCP(), nullptr);
	Dpoint3d mid = DPoint3d::From((ptfaceStart.x + ptfaceEnd.x) / 2, (ptfaceStart.y + ptfaceEnd.y) / 2, (ptfaceStart.z + ptfaceEnd.z) / 2);
	//Z型板取到的高度有误，改为在AnalyzingFaceGeometricData得到的板厚度m_slabThickness
	//double FHight = 0.0;
	if (mid.z > midDown.z)
	{
		DVec3d vecZ = DVec3d::From(0, 0, -1);
		//FHight = abs(mid.z - midDown.z);
		vecZ.Scale(m_slabThickness);
		MSElementDescrP upface = nullptr;
		m_face.GetElementDescrCP()->Duplicate(&upface);
		PITCommonTool::CFaceTool::MoveCenterFaceByMidPt(upface, vecZ);
		//m_ldfoordata.facedesUp = upface;
		downface.ReplaceElementDescr(upface);
	}
	/*else
	{
		EditElementHandle eehUP;
		if (!ExtractFacesTool::GetFloorDownFaceForSlab(copyEleeh, eehUP, minPos, false, const_cast<MSElementDescrP>(m_face.GetElementDescrCP())))
		{
			return false;
		}
		mdlElmdscr_computeRange(&ptfaceStartdown, &ptfaceEnddown, eehUP.GetElementDescrCP(), nullptr);
		Dpoint3d midUp = DPoint3d::From((ptfaceStartdown.x + ptfaceEnddown.x) / 2, (ptfaceStartdown.y + ptfaceEnddown.y) / 2, (ptfaceStartdown.z + ptfaceEnddown.z) / 2);

		//FHight = abs(mid.z - midUp.z);
	}*/

	//Z型板根据顶底面位置调整坐标，例如底面对应的”顶面“应该是本身z轴向上移动一个厚度得到
	if (COMPARE_VALUES_EPS(mid.z, minP2.z, 500) == 0 && COMPARE_VALUES_EPS(maxP2.z, mid.z + m_slabThickness, 500) > 0
		|| COMPARE_VALUES_EPS(mid.z + m_slabThickness, maxP2.z, 500) == 0 && COMPARE_VALUES_EPS(mid.z, minP2.z, 500) > 0)
	{
		minP2 = ptfaceStart;
		maxP2 = ptfaceEnd;
		maxP2.z += m_slabThickness;

		MSElementDescrP upface = nullptr;
		m_face.GetElementDescrCP()->Duplicate(&upface);
		downface.ReplaceElementDescr(upface);
	}
	else if (COMPARE_VALUES_EPS(mid.z, maxP2.z, 500) == 0 && COMPARE_VALUES_EPS(mid.z - m_slabThickness, minP2.z, 500) > 0
		|| COMPARE_VALUES_EPS(mid.z - m_slabThickness, minP2.z, 500) == 0 && COMPARE_VALUES_EPS(maxP2.z, mid.z, 500) > 0)
	{
		minP2 = ptfaceStart;
		minP2.z -= m_slabThickness;
		maxP2 = ptfaceEnd;

		DVec3d vecZ = DVec3d::From(0, 0, -1);
		vecZ.Scale(m_slabThickness);
		MSElementDescrP upface = nullptr;
		m_face.GetElementDescrCP()->Duplicate(&upface);
		PITCommonTool::CFaceTool::MoveCenterFaceByMidPt(upface, vecZ);
		downface.ReplaceElementDescr(upface);
	}

	DVec3d vecZ = DVec3d::UnitZ();
	DPoint3d facenormal;
	minPos = minP2;
	minPos.Add(maxP2);
	minPos.Scale(0.5);
	mdlElmdscr_extractNormal(&facenormal, nullptr, downface.GetElementDescrP(), NULL);
	facenormal.Scale(-1);
	RotMatrix rMatrix;
	Transform trans;
	mdlRMatrix_getIdentity(&rMatrix);
	mdlRMatrix_fromVectorToVector(&rMatrix, &facenormal, &vecZ);//旋转到xoy平面
	mdlTMatrix_fromRMatrix(&trans, &rMatrix);
	mdlTMatrix_setOrigin(&trans, &minPos);
	copyEleeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(copyEleeh, TransformInfo(trans));
	downface.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(downface, TransformInfo(trans));

	double tmpangle = facenormal.AngleTo(vecZ);
	if (tmpangle > PI / 2)
	{
		tmpangle = PI - tmpangle;
	}
	//计算指定元素描述符中元素的范围。
	//mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
	DPoint3d minP, maxP;
	//计算指定元素描述符中元素的范围。
	mdlElmdscr_computeRange(&minP, &maxP, downface.GetElementDescrP(), NULL);
	trans.InverseOf(trans);
	downface.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(downface, TransformInfo(trans));
	m_ldfoordata.Ylenth = (maxP.y - minP.y)*uor_now / uor_ref;
	m_ldfoordata.Xlenth = (maxP.x - minP.x)*uor_now / uor_ref;
	m_ldfoordata.Zlenth = m_slabThickness;//FHight;//(maxP2.z - minP2.z)*uor_now / uor_ref;
	m_ldfoordata.oriPt = minP;
	m_ldfoordata.Vec = DVec3d::From(facenormal.x, facenormal.y, facenormal.z);
	m_ldfoordata.facedes = downface.ExtractElementDescr();
	GetUpDownWallFaces(m_ldfoordata);


	//计算整个板的参数
	m_STslabData.height = (maxP.y - minP.y)*uor_now / uor_ref;
	m_STslabData.length = (maxP.x - minP.x)*uor_now / uor_ref;
	m_STslabData.width = m_slabThickness;// (maxP2.z - minP2.z)*uor_now / uor_ref;
	m_STslabData.ptStart = minP;
	m_STslabData.ptEnd = minP;
	m_STslabData.ptEnd.x = maxP.x;

	//计算当前Z坐标方向
	DPoint3d ptPreStr = m_STslabData.ptStart;
	DPoint3d ptPreEnd = m_STslabData.ptStart;
	ptPreEnd.z = ptPreEnd.z + m_STslabData.width*uor_now;

	mdlTMatrix_transformPoint(&m_ldfoordata.oriPt, &trans);
	mdlTMatrix_transformPoint(&m_STslabData.ptStart, &trans);
	mdlTMatrix_transformPoint(&m_STslabData.ptEnd, &trans);
	mdlTMatrix_transformPoint(&ptPreStr, &trans);
	mdlTMatrix_transformPoint(&ptPreEnd, &trans);

	//确保xy方向是从绝对坐标系最低往最高
	if (COMPARE_VALUES(m_STslabData.ptStart.x, m_STslabData.ptEnd.x) == 1)
	{
		swap(m_STslabData.ptStart, m_STslabData.ptEnd);
		swap(ptPreStr, ptPreEnd);
	}
	if (COMPARE_VALUES(m_STslabData.ptStart.y, minP.y) == 1)
	{
		m_STslabData.ptStart.y = minP.y;
		m_STslabData.ptEnd.y = minP.y;
		m_ldfoordata.oriPt.y = minP.y;
		ptPreStr.y = minP.y;
		ptPreEnd.y = minP.y;
		swap(ptPreStr.z, ptPreEnd.z);
	}

	m_STslabData.vecZ = ptPreEnd - ptPreStr;
	m_STslabData.vecZ.Normalize();


	m_STwallData.height = m_STslabData.height;
	m_STwallData.length = m_STslabData.length;
	m_STwallData.ptStart = m_STslabData.ptStart;
	m_STwallData.ptEnd = m_STslabData.ptEnd;
	m_STwallData.ptPreStr = ptPreStr;
	m_STwallData.ptPreEnd = ptPreEnd;
	//m_Holeehs = Holeehs;
	//m_height = m_STslabData.width;
	m_STwallData.width = m_STslabData.width;


	DPoint3d ptStart, ptEnd;
	ptStart = m_STslabData.ptStart;
	ptEnd = m_STslabData.ptEnd;

	CVector3D  xVecNew(ptStart, ptEnd);
	xVecNew.Normalize();

	DPoint3d ptOrgin = m_STslabData.ptStart;
	DVec3d tmpz = m_STslabData.vecZ;
	tmpz.Normalize();

	CVector3D  yVec = tmpz;     //返回两个向量的（标量）叉积。y  	
	yVec.Scale(-1);

	bool isXtY = false;
	tmpz.Scale(m_STslabData.width);
	ptOrgin.Add(tmpz);
	BeMatrix   placement = CMatrix3D::Ucs(ptOrgin, xVecNew, yVec, isXtY);		//方向为X轴，水平垂直方向为Y轴
	//placement.SetScaleFactors(1, 1, -1);
	SetPlacement(placement);

	return true;
}

void PlaneRebarAssembly::AnalyzeFloorAndWall(EditElementHandleCR eeh, int  i, MSElementDescrP EdpFace)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	WallHelper::WallGeometryInfo wall_geometry_info;
	if (!WallHelper::analysis_geometry(eeh, wall_geometry_info))
	{
		mdlOutput_error(L"获得墙几何信息失败");
		return;
	}
	std::vector<WallHelper::Associated> associated;
	if (!WallHelper::analysis_associated(eeh, associated, wall_geometry_info))
	{
		mdlOutput_error(L"扫描相连构件失败");
		return;
	}
	auto top_slab_iter =
		std::find_if(
			associated.cbegin(), associated.cend(),
			[](const WallHelper::Associated &assoc)
	{
		return assoc.type == WallHelper::AssociatedType::TopFloor;
	});
	auto bottom_slab_iter =
		std::find_if(
			associated.cbegin(), associated.cend(),
			[](const WallHelper::Associated &assoc)
	{
		return assoc.type == WallHelper::AssociatedType::BottomFloor;
	});

	m_associatedWall.clear();
	ElementId id = 0;
	for (int i = 0; i < associated.size(); i++)
	{
		if (associated[i].type == WallHelper::AssociatedType::Wall)
		{
			//EditElementHandle *tmpeeh = new EditElementHandle(associated[i].element, true);

			m_associatedWall.push_back(associated[i].element);

		}
	}

	BrString strRebarSize(GetMainRebars().at(i).rebarSize);
	double	bendradius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);		//乘以了10
	double diameter = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL);
	double la0 = 15 * stod(GetMainRebars().at(i).rebarSize) * uor_per_mm;//diameter * 15;
	double Lae = GetLae();
	if (Lae > 0)
	{
		Lae *= stod(GetMainRebars().at(i).rebarSize);
	}
	else
	{
		Lae = la0;
	}
	double endLength = la0;
	if (top_slab_iter != associated.cend())//有顶板
	{
		EditElementHandle top_slab(top_slab_iter->element, true);
		top_slab.GetElementDescrP()->Duplicate(&m_TopSlabEdp);
		DVec3d top_slab_dir;
		if (!WallHelper::analysis_slab_position_with_wall(eeh, wall_geometry_info.normal, top_slab, top_slab_dir))
		{
			mdlOutput_error(L"分析墙的顶板方向失败");
			return;
		}

		DPoint3d ptmin, ptmax;
		mdlElmdscr_computeRange(&ptmin, &ptmax, top_slab.GetElementDescrP(), nullptr);

		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(top_slab, Eleeh, Holeehs);
		DPoint3d ptmiddle = DPoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);
		Dpoint3d ptUp = ptmiddle;
		Dpoint3d ptDown = ptmiddle;
		ptDown.z -= 5000 * UOR_PER_MilliMeter;
		ptUp.z += 5000 * UOR_PER_MilliMeter;
		Dpoint3d ptUp1 = ptUp;
		Dpoint3d ptDown1 = ptDown;
		ptUp1.x = ptmin.x + 100, ptUp1.y = ptmin.y + 100;
		ptDown1.x = ptmin.x + 100, ptDown1.y = ptmin.y + 100;
		vector<Dpoint3d> vecPT;
		GetIntersectPointsWithOldElmOwner(vecPT, &Eleeh, ptUp, ptDown, 0);
		GetIntersectPointsWithOldElmOwner(vecPT, &Eleeh, ptUp1, ptDown1, 0);

		m_dUpSlabThickness = ptUp.Distance(ptDown) < ptUp1.Distance(ptDown1) ? ptUp.Distance(ptDown) : ptUp1.Distance(ptDown1);


		auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
		WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, top_slab, top_wall_inside_outside_faces);
		for (size_t i = 0; i < top_wall_inside_outside_faces.inside_faces.size(); i++)
		{
			MSElementDescrP insideface = top_wall_inside_outside_faces.inside_faces[i]->GetElementDescrP();
			MSElementDescrP insidefaceCopy;
			insideface->Duplicate(&insidefaceCopy);
			MSElementDescrP pResult1 = nullptr;
			MSElementDescrP pResult2 = nullptr;
			if (SUCCESS == mdlElmdscr_intersectShapes(&pResult1, &pResult2, insidefaceCopy, EdpFace, 0.1))
			{
				m_wallTopFaceType = 0;
				m_dTopOffset = 2 * diameter;
			}
		}
		for (size_t i = 0; i < top_wall_inside_outside_faces.outside_faces.size(); i++)
		{
			MSElementDescrP outsideface = top_wall_inside_outside_faces.outside_faces[i]->GetElementDescrP();
			MSElementDescrP outsidefaceCopy;
			outsideface->Duplicate(&outsidefaceCopy);
			MSElementDescrP pResult1 = nullptr;
			MSElementDescrP pResult2 = nullptr;
			if (SUCCESS == mdlElmdscr_intersectShapes(&pResult1, &pResult2, outsidefaceCopy, EdpFace, 0.1))
			{
				m_wallTopFaceType = 1;
				//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
				endLength = Lae;
			}
		}
		auto top_angle = top_slab_dir.AngleTo(GetfaceNormal());

		// 转换到角度并设置到端部中
		double angel = top_angle / PI * 180;
		if (top_slab_dir == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
		{
			angel = 180;
		}
		m_topEndinfo.endType = 4;
		m_topEndinfo.rotateAngle = angel;
		m_topEndinfo.endPtInfo.value1 = bendradius;
		m_topEndinfo.endPtInfo.value3 = endLength;
	}
	if (bottom_slab_iter != associated.cend())//有底板
	{
		EditElementHandle bottom_slab(bottom_slab_iter->element, true);
		bottom_slab.GetElementDescrP()->Duplicate(&m_BottomSlabEdp);
		DVec3d bottom_slab_dir;
		if (!WallHelper::analysis_slab_position_with_wall(eeh, wall_geometry_info.normal, bottom_slab, bottom_slab_dir))
		{
			mdlOutput_error(L"分析墙的顶板方向失败");
			return;
		}



		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(bottom_slab, Eleeh, Holeehs);

		DPoint3d ptmin, ptmax;
		mdlElmdscr_computeRange(&ptmin, &ptmax, bottom_slab.GetElementDescrP(), nullptr);
		DPoint3d ptmiddle = DPoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);
		Dpoint3d ptUp = ptmiddle;
		Dpoint3d ptDown = ptmiddle;
		ptDown.z -= 5000 * UOR_PER_MilliMeter;
		ptUp.z += 5000 * UOR_PER_MilliMeter;
		Dpoint3d ptUp1 = ptUp;
		Dpoint3d ptDown1 = ptDown;
		ptUp1.x = ptmin.x + 100, ptUp1.y = ptmin.y + 100;
		ptDown1.x = ptmin.x + 100, ptDown1.y = ptmin.y + 100;
		vector<Dpoint3d> vecPT;
		GetIntersectPointsWithOldElmOwner(vecPT, &Eleeh, ptUp, ptDown, 0);
		GetIntersectPointsWithOldElmOwner(vecPT, &Eleeh, ptUp1, ptDown1, 0);
		m_dbottomSlabThickness = ptUp.Distance(ptDown) < ptUp1.Distance(ptDown1) ? ptUp.Distance(ptDown) : ptUp1.Distance(ptDown1);

		auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
		WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, bottom_slab, top_wall_inside_outside_faces);
		for (size_t i = 0; i < top_wall_inside_outside_faces.inside_faces.size(); i++)
		{
			MSElementDescrP insideface = top_wall_inside_outside_faces.inside_faces[i]->GetElementDescrP();
			MSElementDescrP insidefaceCopy;
			insideface->Duplicate(&insidefaceCopy);
			MSElementDescrP pResult1 = nullptr;
			MSElementDescrP pResult2 = nullptr;
			if (SUCCESS == mdlElmdscr_intersectShapes(&pResult1, &pResult2, insidefaceCopy, EdpFace, 0.1))
			{
				m_wallBottomFaceType = 0;
				m_dBottomOffset = 2 * diameter;
			}
		}
		for (size_t i = 0; i < top_wall_inside_outside_faces.outside_faces.size(); i++)
		{
			MSElementDescrP outsideface = top_wall_inside_outside_faces.outside_faces[i]->GetElementDescrP();
			MSElementDescrP outsidefaceCopy;
			outsideface->Duplicate(&outsidefaceCopy);
			MSElementDescrP pResult1 = nullptr;
			MSElementDescrP pResult2 = nullptr;
			if (SUCCESS == mdlElmdscr_intersectShapes(&pResult1, &pResult2, outsidefaceCopy, EdpFace, 0.1))
			{
				m_wallBottomFaceType = 1;
				//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
				endLength = Lae;
			}
		}

		auto bottom_angle = bottom_slab_dir.AngleTo(GetfaceNormal());

		// 转换到角度并设置到端部中
		double angel = bottom_angle / PI * 180;
		if (bottom_slab_dir == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
		{
			angel = 180;
		}
		m_bottomEndinfo.endType = 4;
		m_bottomEndinfo.rotateAngle = angel;
		m_bottomEndinfo.endPtInfo.value1 = bendradius;
		m_bottomEndinfo.endPtInfo.value3 = endLength;

	}
	//删除相关处理
	for (WallHelper::Associated assoc : associated)
	{
		EditElementHandle eeh(assoc.element.GetElementId(), ACTIVEMODEL);
		if (eeh.IsValid())
		{
			eeh.DeleteFromModel();
		}
	}


}

void PlaneRebarAssembly::DealWintHoriRebar(LineSegment& lineSeg1, LineSegment& linesegment2, int rebarnum)
{
	m_norsidef.ClearData();

	//计算配筋区间值
	map<int, int>  tmpqj;
	tmpqj[0] = (int)(linesegment2.GetLength());//大面区间
	tmpqj[(int)(linesegment2.GetLength())] = 0;//大面区间

	if (m_solidType == 1)
	{
		Dpoint3d ptDirStart = linesegment2.GetLineStartPoint();
		Dpoint3d ptdirEnd = linesegment2.GetLineEndPoint();
		DVec3d vecrebarDir1 = ptdirEnd - ptDirStart;
		DVec3d vecrebarDir2 = ptdirEnd - ptDirStart;

		Dpoint3d ptRebarStart = lineSeg1.GetLineStartPoint();
		Dpoint3d ptReabrEnd = lineSeg1.GetLineEndPoint();
		if (m_wallTopFaceType == 1)
		{
			if (!m_bUpIsStatr)
			{
				vecrebarDir1.ScaleToLength(m_dUpSlabThickness);
				ptdirEnd.Add(vecrebarDir1);
				linesegment2.SetLineEndPoint(ptdirEnd);

				tmpqj[(int)(ptdirEnd.z - ptDirStart.z - m_dUpSlabThickness)] = (int)(ptdirEnd.z - ptDirStart.z);
				tmpqj[(int)(ptdirEnd.z - ptDirStart.z)] = 0;
			}
			else//从上开始排钢筋，已经不存在
			{
				vecrebarDir1.Negate();
				vecrebarDir1.ScaleToLength(m_dUpSlabThickness);
				ptDirStart.Add(vecrebarDir1);
				linesegment2.SetLineStartPoint(ptDirStart);
				ptRebarStart.Add(vecrebarDir1);
				ptReabrEnd.Add(vecrebarDir1);
			}

		}

		if (m_wallBottomFaceType == 1)
		{
			if (m_bUpIsStatr)//从上开始排钢筋，已经不存在
			{
				vecrebarDir2.ScaleToLength(m_dbottomSlabThickness);
				ptdirEnd.Add(vecrebarDir2);
				linesegment2.SetLineEndPoint(ptdirEnd);

			}
			else
			{
				// 重新计算起止区间
				tmpqj.erase(linesegment2.GetLength());
				vecrebarDir2.Negate();
				vecrebarDir2.ScaleToLength(m_dbottomSlabThickness);
				ptDirStart.Add(vecrebarDir2);
				linesegment2.SetLineStartPoint(ptDirStart);
				ptRebarStart.Add(vecrebarDir2);
				ptReabrEnd.Add(vecrebarDir2);

				tmpqj[(int)(linesegment2.GetLength())] = 0;
				tmpqj[0] = (int)m_dbottomSlabThickness;
				tmpqj[(int)m_dbottomSlabThickness] = 0;
			}

		}
		lineSeg1.SetLineStartPoint(ptRebarStart);
		lineSeg1.SetLineEndPoint(ptReabrEnd);
	}

	// 如果上下其中一个端点移动半数厚度位置，还在Z型板实体内，则另一方向需要分区
	Dpoint3d ptDirStart = linesegment2.GetLineStartPoint();
	Dpoint3d ptDirEnd = linesegment2.GetLineEndPoint();
	DVec3d vecRebar = linesegment2.GetLineVec();

	vecRebar.ScaleToLength(m_slabThickness / 2);
	ptDirEnd.Add(vecRebar);
	vecRebar.Negate();
	ptDirStart.Add(vecRebar);

	if (ISPointInElement(m_pOldElm, ptDirEnd)) // 上端点移动半数厚度位置，还在Z型板实体内，下方需要分区
	{
		if (m_solidType == 1 && m_wallBottomFaceType == 1)
		{
			tmpqj[int(m_dbottomSlabThickness)] = (int)(m_dbottomSlabThickness + m_slabThickness);
			tmpqj[(int)(m_dbottomSlabThickness + m_slabThickness)] = 0;
		}
		else
		{
			tmpqj[0] = (int)m_slabThickness;
			tmpqj[(int)m_slabThickness] = 0;
		}
	}
	else if (ISPointInElement(m_pOldElm, ptDirStart))// 下端点移动半数厚度位置，还在Z型板实体内，上方需要分区
	{
		double wallLength = linesegment2.GetLength();
		if (m_solidType == 1 && m_wallTopFaceType == 1)
		{
			tmpqj[int(wallLength - m_dUpSlabThickness - m_slabThickness)] = (int)(wallLength - m_dUpSlabThickness);
			tmpqj[(int)(wallLength - m_dUpSlabThickness)] = 0;
		}
		else
		{
			tmpqj[(int)(wallLength - m_slabThickness)] = (int)wallLength;
			tmpqj[(int)wallLength] = 0;
		}
	}

	map<int, int>::iterator itr = tmpqj.begin();
	for (; itr != tmpqj.end(); itr++)
	{
		map<int, int>::iterator itrnex = itr;
		itrnex++;
		if (itrnex == tmpqj.end())
		{
			break;
		}
		if (itrnex->first - itr->first < 3)
		{
			continue;
		}
		m_norsidef.pos[m_norsidef.posnum].str = itr->first;
		m_norsidef.pos[m_norsidef.posnum].end = itrnex->first;
		m_norsidef.posnum++;
	}
}

void PlaneRebarAssembly::GetHoriEndType(PIT::LineSegment lineSeg1, int rebarnum)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	ElementId id = 0;
	DVec3d facenormal = GetfaceNormal();

	EditElementHandle Eleehold;
	std::vector<EditElementHandle*> vecholesold;
	EFT::GetSolidElementAndSolidHoles(*m_pOldElm, Eleehold, vecholesold);
	vector<DSegment3d> vecDownFontLineold;
	vector<DSegment3d> vecDownBackLineold;
	double heightOld;
	EFT::GetFrontBackLinePoint(Eleehold, vecDownFontLineold, vecDownBackLineold, &heightOld);

	if (vecDownFontLineold.size() == 0 && vecDownFontLineold.size() == 0)
	{
		return;
	}
	DSegment3d segFontOld = vecDownFontLineold[0];
	DSegment3d segBackOld = vecDownBackLineold[0];
	//放如XOY平面
	DPoint3d ptStartFontold1, ptEndFontold1;
	segFontOld.GetEndPoints(ptStartFontold1, ptEndFontold1);
	DPoint3d ptStartBackold1, ptEndBackold1;
	segBackOld.GetEndPoints(ptStartBackold1, ptEndBackold1);
	ptStartFontold1.z = 0;
	ptEndFontold1.z = 0;
	ptStartBackold1.z = 0;
	ptEndBackold1.z = 0;
	segFontOld.SetStartPoint(ptStartFontold1);
	segFontOld.SetEndPoint(ptEndFontold1);
	segBackOld.SetStartPoint(ptStartBackold1);
	segBackOld.SetEndPoint(ptEndBackold1);
	//end


	for (size_t i = 0; i < m_associatedWall.size(); i++)
	{
		EditElementHandle eeh(m_associatedWall[i].GetElementId(), m_associatedWall[i].GetDgnModelP());
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> vecholes;
		EFT::GetSolidElementAndSolidHoles(eeh, Eleeh, vecholes);
		double height = 0;
		vector<DSegment3d> vecDownFontLine;
		vector<DSegment3d> vecDownBackLine;
		EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &height);
		if (vecDownFontLine.size() == 0 && vecDownBackLine.size() == 0)
		{
			continue;
		}
		Dpoint3d ptstatr, ptend;
		vecDownFontLine[0].GetEndPoints(ptstatr, ptend);
		DVec3d vecLine = ptend - ptstatr;
		vecLine.Normalize();
		DVec3d vecLineseg = lineSeg1.GetLineVec();

		if (abs(vecLineseg.DotProduct(vecLine)) > 0.9)
		{
			continue;
		}
		DSegment3d segmain = lineSeg1.GetLineSeg();
		DSegment3d segFont = vecDownFontLine[0];
		DSegment3d segBack = vecDownBackLine[0];
		DPoint3d fontstart, fontend;
		segFont.GetEndPoints(fontstart, fontend);
		DPoint3d backstart, backend;
		segBack.GetEndPoints(backstart, backend);
		DPoint3d ptStart = lineSeg1.GetLineStartPoint();
		DPoint3d ptEnd = lineSeg1.GetLineEndPoint();

		DPoint3d ptStartFont, ptEndFont;
		segFont.GetEndPoints(ptStartFont, ptEndFont);
		DPoint3d ptStartBack, ptEndBack;
		segBack.GetEndPoints(ptStartBack, ptEndBack);

		Dpoint3d ptFont, ptBack, ptFontold, ptBackold;
		//算交点时放在xoy水平面上
		DPoint3d ptStartFont1, ptEndFont1;
		segFont.GetEndPoints(ptStartFont1, ptEndFont1);
		DPoint3d ptStartBack1, ptEndBack1;
		segBack.GetEndPoints(ptStartBack1, ptEndBack1);
		ptStartFont1.z = 0;
		ptEndFont1.z = 0;
		ptStartBack1.z = 0;
		ptEndBack1.z = 0;
		DPoint3d ptStarMain1, ptEndMain1;
		segmain.GetEndPoints(ptStarMain1, ptEndMain1);
		ptStarMain1.z = 0;
		ptEndMain1.z = 0;
		segFont.SetStartPoint(ptStartFont1);
		segFont.SetEndPoint(ptEndFont1);
		segBack.SetStartPoint(ptStartBack1);
		segBack.SetEndPoint(ptEndBack1);
		segmain.SetStartPoint(ptStarMain1);
		segmain.SetEndPoint(ptEndMain1);
		//end
		mdlVec_intersect(&ptFont, &segFont, &segmain);//与主线的交点
		mdlVec_intersect(&ptBack, &segBack, &segmain);
		double DistanceAll = ptEnd.Distance(ptStart);
		double distnca = ptBack.DistanceXY(ptStart);
		if ((ptFont.DistanceXY(ptStart) > 10 && ptFont.DistanceXY(ptEnd) > 10)
			&& (ptBack.DistanceXY(ptStart) > 10 && ptBack.DistanceXY(ptEnd) > 10))
		{
			continue;
		}
		BrString strRebarSize(GetMainRebars().at(rebarnum).rebarSize);
		//double	diameter = RebarCode::GetBarDiameter(strSize1, ACTIVEMODEL);		//乘以了10
		double diameter = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL);
		//double	bendradius = RebarCode::GetPinRadius(strSize1, ACTIVEMODEL, false);		//乘以了10
		double	bendradius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);
		double La0 = 15 * stod(GetMainRebars().at(rebarnum).rebarSize) * uor_per_mm;//diameter * 15;//先为15d,如果判断外侧则修改
		double Lae = GetLae();
		double endLength = La0;
		if (Lae > 0)
		{
			Lae *= stod(GetMainRebars().at(rebarnum).rebarSize);
		}
		else
		{
			Lae = La0;
		}
		double DistanceThick = ptBack.Distance(ptFont);
		bool bStartIn = false;
		int bStartType = -1;
		int bEndInType = -1;
		if (ptFont.DistanceXY(ptStart) < 10)
		{
			double DistanceBack = ptBack.DistanceXY(ptEnd);
			mdlVec_intersect(&ptFontold, &segFontOld, &segFont);//当主线交点等于断点，则根据与原实体底线面判断
			mdlVec_intersect(&ptBackold, &segBackOld, &segFont);
			if (DistanceBack > DistanceAll)//在外面
			{
				bStartIn = false;
			}
			else //在里面
			{
				bStartIn = true;
			}
			if (bStartIn)
			{
				if (ptFontold.DistanceXY(ptStart) < 10)
				{
					if (ptFontold.DistanceXY(ptStartFont) > ptBackold.DistanceXY(ptStartFont))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptStart) < 10)
				{
					if (ptBackold.DistanceXY(ptStartFont) > ptFontold.DistanceXY(ptStartFont))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
			}
			else
			{
				if (ptFontold.DistanceXY(ptStart) < 10)
				{
					if ((ptFontold.DistanceXY(ptStartFont) < 10 || ptFontold.DistanceXY(ptEndFont) < 10)
						/*|| ptFontold.DistanceXY(ptStartFont) > ptBackold.DistanceXY(ptStartFont)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptStart) < 10)
				{
					if ((ptBackold.DistanceXY(ptStartFont) < 10 || ptBackold.DistanceXY(ptEndFont) < 10)
						/*||ptBackold.DistanceXY(ptStartFont) > ptFontold.DistanceXY(ptStartFont)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
			}

			DVec3d linvec = DVec3d::FromZero();
			if (ptFont.DistanceXY(fontstart) < ptFont.DistanceXY(fontend))
			{
				linvec = fontend - fontstart;
			}
			else
			{
				linvec = fontstart - fontend;
			}

			auto angle = linvec.AngleTo(GetfaceNormal());

			// 转换到角度并设置到端部中
			double angel = angle / PI * 180;
			if (linvec == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
			{
				angel = 180;
			}

			m_HorStrType.endType = 4;
			m_HorStrType.rotateAngle = angel;
			m_HorStrType.endPtInfo.value1 = bendradius;
			m_HorStrType.endPtInfo.value3 = endLength;
		}
		else if (ptFont.DistanceXY(ptEnd) < 10)
		{

			double DistanceFont = ptBack.DistanceXY(ptStart);
			mdlVec_intersect(&ptFontold, &segFontOld, &segFont);
			mdlVec_intersect(&ptBackold, &segBackOld, &segFont);
			if (DistanceFont > DistanceAll)//在外面
			{
				bStartIn = false;
			}
			else //在里面
			{
				bStartIn = true;
			}
			if (bStartIn)
			{
				if (ptFontold.DistanceXY(ptEnd) < 10)
				{
					if (ptFontold.DistanceXY(ptStartFont) > ptBackold.DistanceXY(ptStartFont))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptEnd) < 10)
				{
					if (ptBackold.DistanceXY(ptStartFont) > ptFontold.DistanceXY(ptStartFont))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
			}
			else
			{
				if (ptFontold.DistanceXY(ptEnd) < 10)
				{
					if ((ptFontold.DistanceXY(ptStartFont) < 10 || ptFontold.DistanceXY(ptEndFont) < 10)
						/*|| ptFontold.DistanceXY(ptStartFont) > ptBackold.DistanceXY(ptStartFont)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptEnd) < 10)
				{
					if ((ptBackold.DistanceXY(ptStartFont) < 10 || ptBackold.DistanceXY(ptEndFont) < 10)
						/*|| ptBackold.DistanceXY(ptStartFont) > ptFontold.DistanceXY(ptStartFont)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
			}
			/*
			if (ptFontold.DistanceXY(ptEnd) < 10)
			{
				if (ptFontold.DistanceXY(ptStartFont) > ptBackold.DistanceXY(ptStartFont))
				{
					LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
				}
			}
			else if (ptBackold.DistanceXY(ptEnd) < 10)
			{
				if (ptBackold.DistanceXY(ptStartFont) > ptFontold.DistanceXY(ptStartFont))
				{
					LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
				}
			}*/

			DVec3d linvec = DVec3d::FromZero();
			if (ptFont.DistanceXY(fontstart) < ptFont.DistanceXY(fontend))
			{
				linvec = fontend - fontstart;
			}
			else
			{
				linvec = fontstart - fontend;
			}

			auto angle = linvec.AngleTo(GetfaceNormal());

			// 转换到角度并设置到端部中
			double angel = angle / PI * 180;
			if (linvec == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
			{
				angel = 180;
			}

			m_HorEndType.endType = 4;
			m_HorEndType.rotateAngle = angel;
			m_HorEndType.endPtInfo.value1 = bendradius;
			m_HorEndType.endPtInfo.value3 = endLength;
		}
		else if (ptBack.DistanceXY(ptEnd) < 10)
		{

			double DistanceFont = ptFont.DistanceXY(ptStart);
			mdlVec_intersect(&ptFontold, &segFontOld, &segBack);
			mdlVec_intersect(&ptBackold, &segBackOld, &segBack);
			if (DistanceFont > DistanceAll)//在外面
			{
				bStartIn = false;
			}
			else //在里面
			{
				bStartIn = true;
			}
			if (bStartIn)
			{
				if (ptFontold.DistanceXY(ptEnd) < 10)
				{
					if (ptFontold.DistanceXY(ptStartBack) > ptBackold.DistanceXY(ptStartBack))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptEnd) < 10)
				{
					if (ptBackold.DistanceXY(ptStartBack) > ptFontold.DistanceXY(ptStartBack))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
			}
			else
			{
				if (ptFontold.DistanceXY(ptEnd) < 10)
				{
					if ((ptFontold.DistanceXY(ptStartBack) < 10 || ptFontold.DistanceXY(ptEndBack) < 10)
						/*|| ptFontold.DistanceXY(ptStartBack) > ptBackold.DistanceXY(ptStartBack)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptEnd) < 10)
				{
					if ((ptBackold.DistanceXY(ptStartBack) < 10 || ptBackold.DistanceXY(ptEndBack) < 10)
						/*|| ptBackold.DistanceXY(ptStartBack) > ptFontold.DistanceXY(ptStartBack)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
			}

			/*if (ptFontold.DistanceXY(ptEnd) < 10)
			{
				if (ptFontold.DistanceXY(ptStartFont) > ptBackold.DistanceXY(ptStartFont))
				{
					LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
				}
			}
			else if (ptBackold.DistanceXY(ptEnd) < 10)
			{
				if (ptBackold.DistanceXY(ptStartFont) > ptFontold.DistanceXY(ptStartFont))
				{
					LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
				}
			}*/

			DVec3d linvec = DVec3d::FromZero();
			if (ptBack.DistanceXY(backstart) < ptBack.DistanceXY(backend))
			{
				linvec = backend - backstart;
			}
			else
			{
				linvec = backstart - backend;
			}

			auto angle = linvec.AngleTo(GetfaceNormal());

			// 转换到角度并设置到端部中
			double angel = angle / PI * 180;
			if (linvec == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
			{
				angel = 180;
			}

			m_HorEndType.endType = 4;
			m_HorEndType.rotateAngle = angel;
			m_HorEndType.endPtInfo.value1 = bendradius;
			m_HorEndType.endPtInfo.value3 = endLength;
		}
		else if (ptBack.DistanceXY(ptStart) < 10)
		{

			double DistanceFont = ptFont.DistanceXY(ptEnd);
			mdlVec_intersect(&ptFontold, &segFontOld, &segBack);
			mdlVec_intersect(&ptBackold, &segBackOld, &segBack);
			if (DistanceFont > DistanceAll)//在外面
			{
				bStartIn = false;
			}
			else //在里面
			{
				bStartIn = true;
			}
			if (bStartIn)
			{
				if (ptFontold.DistanceXY(ptStart) < 10)
				{
					if (ptFontold.DistanceXY(ptStartBack) > ptBackold.DistanceXY(ptStartBack))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptStart) < 10)
				{
					if (ptBackold.DistanceXY(ptStartBack) > ptFontold.DistanceXY(ptStartBack))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
			}
			else
			{
				if (ptFontold.DistanceXY(ptStart) < 10)
				{
					if ((ptFontold.DistanceXY(ptStartBack) < 10 || ptFontold.DistanceXY(ptEndBack) < 10)
						/*|| ptFontold.DistanceXY(ptStartBack) > ptBackold.DistanceXY(ptStartBack)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptStart) < 10)
				{
					if ((ptBackold.DistanceXY(ptStartBack) < 10 || ptBackold.DistanceXY(ptEndBack) < 10)
						/*|| ptBackold.DistanceXY(ptStartBack) > ptFontold.DistanceXY(ptStartBack)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
			}
			//if (ptBack.DistanceXY(ptStartBack) > 10 && ptFont.DistanceXY(ptEndBack) > 10)
			//{
			//	LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
			//}	
			/*if (ptFontold.DistanceXY(ptStart) < 10)
			{
				if (ptFontold.DistanceXY(ptStartFont) > ptBackold.DistanceXY(ptStartFont))
				{
					LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
				}
			}
			else if (ptBackold.DistanceXY(ptStart) < 10)
			{
				if (ptBackold.DistanceXY(ptStartFont) > ptFontold.DistanceXY(ptStartFont))
				{
					LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
				}
			}*/

			DVec3d linvec = DVec3d::FromZero();
			if (ptBack.DistanceXY(backstart) < ptBack.DistanceXY(backend))
			{
				linvec = backend - backstart;
			}
			else
			{
				linvec = backstart - backend;
			}
			auto angle = linvec.AngleTo(GetfaceNormal());

			// 转换到角度并设置到端部中
			double angel = angle / PI * 180;
			if (linvec == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
			{
				angel = 180;
			}

			m_HorStrType.endType = 4;
			m_HorStrType.rotateAngle = angel;
			m_HorStrType.endPtInfo.value1 = bendradius;
			m_HorStrType.endPtInfo.value3 = endLength;
		}
		else
		{
			continue;
		}
		if (bStartType == 1)
		{
			m_bStartType = 1;
		}
		else
		{
			m_bStartType = 0;

			m_dStartOffset = -2 * diameter;

		}
		if (bEndInType == 1)
		{
			m_bEndInType = 1;

		}
		else
		{
			m_bEndInType = 0;

			m_dEndOffset = -2 * diameter;

		}
		EditElementHandle walleeh(m_associatedWall[i], true);
		MSElementDescrP wallEdp = nullptr;
		walleeh.GetElementDescrP()->Duplicate(&wallEdp);
		m_vecWallEdp.push_back(wallEdp);
	}


}

int PlaneRebarAssembly::GetFaceType(MSElementDescrP face, MSElementDescrP upface[40], int upfacenum, MSElementDescrP downface[40], int downfacenum, int i, DVec3d vecRebar)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
	DPoint3d ptFaceNormal;
	mdlElmdscr_extractNormal(&ptFaceNormal, nullptr, face, &ptDefault);
	DPoint3d ptUpNormal, ptDownNormal;
	if (upfacenum > 0)
	{
		mdlElmdscr_extractNormal(&ptUpNormal, nullptr, upface[0], &ptDefault);

		double dot = ptFaceNormal.DotProduct(ptUpNormal);
		if (abs(ptFaceNormal.DotProduct(ptUpNormal)) < 0.9)
		{
			return SideType::Nor;
		}
	}
	else if (downfacenum > 0)
	{
		mdlElmdscr_extractNormal(&ptDownNormal, nullptr, downface[0], &ptDefault);
		if (abs(ptFaceNormal.DotProduct(ptDownNormal)) < 0.9)
		{
			return SideType::Nor;
		}
	}
	else
	{
		return SideType::Nor;
	}

	CVector3D ORIPT = GetPlacement().GetTranslation();
	CMatrix3D tmpmat = GetPlacement();
	Transform trans;
	tmpmat.AssignTo(trans);
	trans.InverseOf(trans);
	TransformInfo transinfo(trans);

	DPoint3d ptcenter = DPoint3d::From(0, 0, 0);
	DPoint3d facenormal = DPoint3d::From(0, 1, 0);
	Transform tran;
	mdlTMatrix_computeFlattenTransform(&tran, &ptcenter, &facenormal);
	TransformInfo tinfo(tran);
	//mdlElmdscr_add(face);



	if (upfacenum > 0)
	{
		MSElementDescrP pResult1 = nullptr;
		MSElementDescrP pResult2 = nullptr;
		int result = -1;
		for (size_t i = 0; i < upfacenum; i++)
		{
			result = mdlElmdscr_intersectShapes(&pResult1, &pResult2, face, upface[i], 0.1);
			if (result == SUCCESS)
			{
				break;
			}
		}


		if (SUCCESS == result)
		{
			EditElementHandle faceeeh(face, false, false, ACTIVEMODEL);

			faceeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(faceeeh, transinfo);
			faceeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(faceeeh, tinfo);
			DPoint3d minP, maxP;
			//计算指定元素描述符中元素的范围。
			mdlElmdscr_computeRange(&minP, &maxP, faceeeh.GetElementDescrP(), NULL);
			//faceeeh.AddToModel();
			DPoint3d midpos = minP;
			midpos.Add(maxP);
			midpos.Scale(0.5);
			//mdlElmdscr_add(faceeeh.GetElementDescrP());
			vector<MSElementDescrP> tmpdescrs;
			for (int i = 0; i < upfacenum; i++)
			{
				EditElementHandle tmpeeh(upface[i], true, false, ACTIVEMODEL);
				tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, transinfo);
				tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, tinfo);
				tmpdescrs.push_back(tmpeeh.ExtractElementDescr());
			}


			DVec3d tmpVec = DVec3d::From(1, 0, 0);
			//m_outsidef.calLen = diameterPre;
			EditElementHandle eehline1;
			EditElementHandle eehline2;
			LineHandler::CreateLineElement(eehline1, NULL, m_LineSeg1.GetLineSeg(), ACTIVEMODEL->Is3d(), *ACTIVEMODEL);
			LineHandler::CreateLineElement(eehline2, NULL, m_LineSeg2.GetLineSeg(), ACTIVEMODEL->Is3d(), *ACTIVEMODEL);

			int verRe = 0;
			int verReStr = 0;//增加旁边两条线判断内外侧，任意0，则为外侧面。
			int verReEnd = 0;
			bool isdelstr = false;
			bool isdelend = false;
			if (GetMainRebars().at(i).rebarDir == 1)	//纵向钢筋,局部坐标系Z方向
			{
				tmpVec = DVec3d::From(0, 0, 1);
				//中间位置钢筋线,计算两端是否有垂直钢筋
				DPoint3d midstr = midpos;
				midstr.z = midpos.z - m_ldfoordata.Ylenth / 2 + GetConcrete().sideCover*uor_per_mm;
				DPoint3d midend = midstr;
				midend.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetConcrete().sideCover*uor_per_mm;
				verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size(), true);

				DPoint3d firstr = midpos;
				firstr.x = firstr.x - m_ldfoordata.Xlenth / 2 + GetConcrete().sideCover*uor_per_mm;
				firstr.z = midstr.z;
				DPoint3d firend = firstr;
				firend.z = midend.z;
				verReStr = IsHaveVerticalWall(firstr, firend, &tmpdescrs.at(0), tmpdescrs.size(), true);
				if (IsHaveParaWall(firstr, firend, &tmpdescrs.at(0), tmpdescrs.size(), true))
				{
					isdelstr = true;
				}

				DPoint3d secstr = midpos;
				secstr.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetConcrete().sideCover*uor_per_mm;
				secstr.z = midstr.z;
				DPoint3d secend = secstr;
				secend.z = midend.z;
				verReEnd = IsHaveVerticalWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size(), true);
				if (IsHaveParaWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size(), true))
				{
					isdelend = true;
				}
			}
			else
			{
				tmpVec = DVec3d::From(1, 0, 0);
				//中间位置钢筋线,计算两端是否有垂直钢筋
				DPoint3d midstr = midpos;
				midstr.x = midpos.x - m_ldfoordata.Xlenth / 2 + GetConcrete().sideCover*uor_per_mm;
				DPoint3d midend = midstr;
				midend.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetConcrete().sideCover*uor_per_mm;
				verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size(), true);

				DPoint3d firstr = midpos;
				firstr.z = firstr.z - m_ldfoordata.Ylenth / 2 + GetConcrete().sideCover*uor_per_mm;
				firstr.x = midstr.x;
				DPoint3d firend = firstr;
				firend.x = midend.x;
				verReStr = IsHaveVerticalWall(firstr, firend, &tmpdescrs.at(0), tmpdescrs.size(), true);
				if (IsHaveParaWall(firstr, firend, &tmpdescrs.at(0), tmpdescrs.size(), true))
				{
					isdelend = true;
				}
				EditElementHandle eehstr;
				LineHandler::CreateLineElement(eehstr, nullptr, DSegment3d::From(firstr, firend), true, *ACTIVEMODEL);

				//终止位置钢筋线，计算终止位置是否有平行钢筋
				DPoint3d secstr = midpos;
				secstr.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetConcrete().sideCover*uor_per_mm;
				secstr.x = midstr.x;
				DPoint3d secend = secstr;
				secend.x = midend.x;
				verReEnd = IsHaveVerticalWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size(), true);
				if (IsHaveParaWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size(), true))
				{
					isdelstr = true;
				}
			}
			if (!isdelstr && !isdelend && (verRe == 0 || verReStr == 0 || verReEnd == 0))
			{
				m_holeRebarInfo.bIsUpFace = true;
				return SideType::Out;
			}
			return SideType::In;


		}
	}


	if (downfacenum > 0)
	{
		MSElementDescrP pResult1 = nullptr;
		MSElementDescrP pResult2 = nullptr;
		int result = -1;// mdlElmdscr_intersectShapes(&pResult1, &pResult2, face, downface[0], 0.1);
		for (size_t i = 0; i < downfacenum; i++)
		{
			result = mdlElmdscr_intersectShapes(&pResult1, &pResult2, face, downface[i], 0.1);
			if (result == SUCCESS)
			{
				break;
			}
		}

		if (SUCCESS == result)
		{
			EditElementHandle faceeeh(face, false, false, ACTIVEMODEL);

			faceeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(faceeeh, transinfo);
			faceeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(faceeeh, tinfo);
			DPoint3d minP, maxP;
			//计算指定元素描述符中元素的范围。
			mdlElmdscr_computeRange(&minP, &maxP, faceeeh.GetElementDescrP(), NULL);
			//faceeeh.AddToModel();
			DPoint3d midpos = minP;
			midpos.Add(maxP);
			midpos.Scale(0.5);
			//mdlElmdscr_add(faceeeh.GetElementDescrP());
			vector<MSElementDescrP> tmpdescrs;
			for (int i = 0; i < downfacenum; i++)
			{
				EditElementHandle tmpeeh(downface[i], true, false, ACTIVEMODEL);
				tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, transinfo);
				tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, tinfo);
				tmpdescrs.push_back(tmpeeh.ExtractElementDescr());
			}


			DVec3d tmpVec = DVec3d::From(1, 0, 0);
			//m_outsidef.calLen = diameterPre;
			EditElementHandle eehline1;
			EditElementHandle eehline2;
			LineHandler::CreateLineElement(eehline1, NULL, m_LineSeg1.GetLineSeg(), ACTIVEMODEL->Is3d(), *ACTIVEMODEL);
			LineHandler::CreateLineElement(eehline2, NULL, m_LineSeg2.GetLineSeg(), ACTIVEMODEL->Is3d(), *ACTIVEMODEL);

			int verRe = 0;
			int verReStr = 0;//增加旁边两条线判断内外侧，任意0，则为外侧面。
			int verReEnd = 0;
			bool isdelstr = false;
			bool isdelend = false;
			if (GetMainRebars().at(i).rebarDir == 1)	//纵向钢筋,局部坐标系Z方向
			{
				tmpVec = DVec3d::From(0, 0, 1);
				//中间位置钢筋线,计算两端是否有垂直钢筋
				DPoint3d midstr = midpos;
				midstr.z = midpos.z - m_ldfoordata.Ylenth / 2 + GetConcrete().sideCover*uor_per_mm;
				DPoint3d midend = midstr;
				midend.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetConcrete().sideCover*uor_per_mm;
				verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size(), true);

				DPoint3d firstr = midpos;
				firstr.x = firstr.x - m_ldfoordata.Xlenth / 2 + GetConcrete().sideCover*uor_per_mm;
				firstr.z = midstr.z;
				DPoint3d firend = firstr;
				firend.z = midend.z;
				verReStr = IsHaveVerticalWall(firstr, firend, &tmpdescrs.at(0), tmpdescrs.size(), true);
				if (IsHaveParaWall(firstr, firend, &tmpdescrs.at(0), tmpdescrs.size(), true))
				{
					isdelstr = true;
				}

				DPoint3d secstr = midpos;
				secstr.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetConcrete().sideCover*uor_per_mm;
				secstr.z = midstr.z;
				DPoint3d secend = secstr;
				secend.z = midend.z;
				verReEnd = IsHaveVerticalWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size(), true);
				if (IsHaveParaWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size(), true))
				{
					isdelend = true;
				}
			}
			else
			{
				tmpVec = DVec3d::From(1, 0, 0);
				//中间位置钢筋线,计算两端是否有垂直钢筋
				DPoint3d midstr = midpos;
				midstr.x = midpos.x - m_ldfoordata.Xlenth / 2 + GetConcrete().sideCover*uor_per_mm;
				DPoint3d midend = midstr;
				midend.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetConcrete().sideCover*uor_per_mm;
				verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size(), true);

				DPoint3d firstr = midpos;
				firstr.z = firstr.z - m_ldfoordata.Ylenth / 2 + GetConcrete().sideCover*uor_per_mm;
				firstr.x = midstr.x;
				DPoint3d firend = firstr;
				firend.x = midend.x;
				verReStr = IsHaveVerticalWall(firstr, firend, &tmpdescrs.at(0), tmpdescrs.size(), true);
				if (IsHaveParaWall(firstr, firend, &tmpdescrs.at(0), tmpdescrs.size(), true))
				{
					isdelend = true;
				}
				EditElementHandle eehstr;
				LineHandler::CreateLineElement(eehstr, nullptr, DSegment3d::From(firstr, firend), true, *ACTIVEMODEL);

				//终止位置钢筋线，计算终止位置是否有平行钢筋
				DPoint3d secstr = midpos;
				secstr.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetConcrete().sideCover*uor_per_mm;
				secstr.x = midstr.x;
				DPoint3d secend = secstr;
				secend.x = midend.x;
				verReEnd = IsHaveVerticalWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size(), true);
				if (IsHaveParaWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size(), true))
				{
					isdelstr = true;
				}
			}
			if (!isdelstr && !isdelend && (verRe == 0 || verReStr == 0 || verReEnd == 0))
			{
				return SideType::Out;
			}
			m_holeRebarInfo.bIsUpFace = false;
			return SideType::In;
		}
	}
	//CalculateOutSideData(face, upface, downface, i, vecRebar);
	return SideType::Out;

}

void PlaneRebarAssembly::ChangeRebarLine(PIT::LineSegment& lineSeg)
{
	DPoint3d ptStart = lineSeg.GetLineStartPoint();
	DPoint3d ptEnd = lineSeg.GetLineEndPoint();
	DVec3d  vec = ptEnd - ptStart;
	vec.Normalize();
	vec.ScaleToLength(500 * UOR_PER_MilliMeter);
	ptEnd.Add(vec);
	vec.Negate();
	ptStart.Add(vec);
	EditElementHandle lineeeh;
	LineHandler::CreateLineElement(lineeeh, nullptr, DSegment3d::From(ptStart, ptEnd), true, *ACTIVEMODEL);
	vector<DPoint3d> interPoints;
	ISolidKernelEntityPtr LineentityPtr;
	if (SolidUtil::Convert::ElementToBody(LineentityPtr, lineeeh) == SUCCESS)
	{
		if (m_pOldElm->IsValid())
		{
			EditElementHandle tmpeeh;
			tmpeeh.Duplicate(*m_pOldElm);
			vector<DPoint3d>	vecresult;
			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, tmpeeh) == SUCCESS)
			{
				if (BodyIntersect(vecresult, LineentityPtr, entityPtr)) // 相交的点
				{
					//求出相交点后，获取与起始点最近的点最为起始点
					double minDistanceStart = ptStart.Distance(ptEnd);
					double minDistanceEnd = minDistanceStart;
					int startPtindex = 0;
					int endPtindex = 0;
					for (int i = 0; i < vecresult.size(); i++)
					{
						double startDistance = vecresult[i].Distance(lineSeg.GetLineStartPoint());
						double endDistance = vecresult[i].Distance(lineSeg.GetLineEndPoint());
						if (startDistance < minDistanceStart)
						{
							minDistanceStart = startDistance;
							startPtindex = i;
						}
						if (endDistance < minDistanceEnd)
						{
							minDistanceEnd = endDistance;
							endPtindex = i;
						}
					}
					lineSeg.SetLineStartPoint(vecresult[startPtindex]);
					lineSeg.SetLineEndPoint(vecresult[endPtindex]);
				}
			}
		}
	}
}



void PlaneRebarAssembly::CreateAnchorBySelf(vector<MSElementDescrP> tmpAnchordescrs, PIT::LineSegment Lineseg, double bendradius, double la0, double lae, double diameter, double diameterPre, int irebarlevel, bool isInface, bool bisSumps)
{
	m_verSlabFaceInfo.ClearData();
	double dSideCoverConcrete = GetConcrete().sideCover * UOR_PER_MilliMeter;
	DVec3d vec2;
	if (Lineseg.IsEqual(m_LineSeg1))
	{
		vec2 = m_LineSeg2.GetLineVec();
	}
	else
	{
		vec2 = m_LineSeg1.GetLineVec();
	}
	DVec3d vec1 = Lineseg.GetLineVec();
	Dpoint3d ptStart = Lineseg.GetLineStartPoint();
	Dpoint3d ptEnd = Lineseg.GetLineEndPoint();
	for (size_t i = 0; i < tmpAnchordescrs.size(); i++)
	{
		DVec3d vecfacenor;
		DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
		mdlElmdscr_extractNormal(&vecfacenor, nullptr, tmpAnchordescrs[i], &ptDefault);

		if (bisSumps) //集水坑锚固处理
		{
			if (abs(ptDefault.DotProduct(vecfacenor)) < 0.1)//垂直表示竖直面
			{
				//一个平行一个垂直 表示配筋面是水平面
				if (abs(vec1.DotProduct(vecfacenor)) > 0.9 &&  abs(ptDefault.DotProduct(vec2)) < 0.1)
				{
					Dpoint3d ptmin, ptmax;
					mdlElmdscr_computeRange(&ptmin, &ptmax, tmpAnchordescrs[i], nullptr);
					Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);

					EditElementHandle eehtest(m_Solid->GetElementRef(), m_Solid->GetModelRef());
					EditElementHandle Eleeh;
					std::vector<EditElementHandle*> Holeehs;
					EFT::GetSolidElementAndSolidHoles(eehtest, Eleeh, Holeehs);

					EditElementHandle copyEleeh;
					copyEleeh.Duplicate(Eleeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(Eleeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(copyEleeh);

					EditElementHandle copyEleeh2;
					copyEleeh2.Duplicate(eehtest);
					ElementCopyContext copier1(ACTIVEMODEL);
					copier1.SetSourceModelRef(Eleeh.GetModelRef());
					copier1.SetTransformToDestination(true);
					copier1.SetWriteElements(false);
					copier1.DoCopy(copyEleeh2);


					if (!Eleeh.IsValid())
					{
						mdlDialog_dmsgsPrint(L"非法的板实体!");

					}

					DPoint3d minPos;
					EditElementHandle downface;
					if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, true))
					{
						return;
					}

					EditElementHandle upface;
					if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, upface, minPos, false))
					{
						return;
					}

					Dpoint3d ptfaceStart, ptfaceEnd;
					mdlElmdscr_computeRange(&ptfaceStart, &ptfaceEnd, m_face.GetElementDescrCP(), nullptr);
					Dpoint3d mid = DPoint3d::From((ptfaceStart.x + ptfaceEnd.x) / 2, (ptfaceStart.y + ptfaceEnd.y) / 2, (ptfaceStart.z + ptfaceEnd.z) / 2);

					Dpoint3d ptfaceStartDown, ptfaceEndDown;
					mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
					Dpoint3d middown = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);

					Dpoint3d ptfaceStartUp, ptfaceEndUp;
					mdlElmdscr_computeRange(&ptfaceStartUp, &ptfaceEndUp, upface.GetElementDescrCP(), nullptr);
					Dpoint3d midup = DPoint3d::From((ptfaceStartUp.x + ptfaceEndUp.x) / 2, (ptfaceStartUp.y + ptfaceEndUp.y) / 2, (ptfaceStartUp.z + ptfaceEndUp.z) / 2);

					bool bIsin = false;//内外面，确定弯钩
					double angel = 0;

					DVec3d VecAngle = DVec3d::FromCrossProduct(vec2, vecfacenor);
					if (ptmid.z > ptStart.z) //上面
					{
						double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, 1));
						angel = thod / PI * 180;
						if (mid.z - middown.z > 10)
						{
							bIsin = true;
						}
						else
						{
							bIsin = false;
						}
						if (VecAngle.z < 0)
						{
							VecAngle.Negate();
						}
					}
					else //下面
					{
						double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, -1));
						angel = thod / PI * 180;
						if ((mid.z - midup.z) < 10)
						{
							bIsin = false;
						}
						else
						{
							bIsin = true;
							/*if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, false, const_cast<MSElementDescrP>(m_face.GetElementDescrCP())))
							{
								return;
							}*/
						}
						if (VecAngle.z > 0)
						{
							VecAngle.Negate();
						}
					}

					std::vector<EditElementHandle*> allFaces;
					std::vector<EditElementHandle*> allParalFaces;
					ExtractFacesTool::GetFaces(copyEleeh2, allFaces);
					for (size_t index = 0; index < allFaces.size(); index++)
					{
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allFaces[index]->GetElementDescrP(), &ptDefault);
						if (abs(vecface.DotProduct(vecfacenor)) > 0.9)
						{
							allParalFaces.push_back(allFaces[index]);
						}
					}

					double distance = 0.0;
					for (size_t index = 0; index < allParalFaces.size(); index++)
					{

						Dpoint3d ptfacemin, ptfacemax;
						mdlElmdscr_computeRange(&ptfacemin, &ptfacemax, allParalFaces[index]->GetElementDescrCP(), nullptr);
						Dpoint3d ptfaceMid = Dpoint3d::From((ptfacemin.x + ptfacemax.x) / 2, (ptfacemin.y + ptfacemax.y) / 2, (ptfacemin.z + ptfacemax.z) / 2);
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allParalFaces[index]->GetElementDescrP(), &ptDefault);
						Dpoint3d ptproject;
						mdlVec_projectPointToPlane(&ptproject, &ptmid, &ptfaceMid, &vecface);
						ISolidKernelEntityPtr entityPtr;
						if (SolidUtil::Convert::ElementToBody(entityPtr, *allParalFaces[index]) == SUCCESS)
						{
							if (!SolidUtil::IsPointInsideBody(*entityPtr, ptproject))
							{
								Dpoint3d ptOut = Dpoint3d::FromZero();
								StatusInt status = mdlMeasure_closestPointOnElement(&ptOut, allParalFaces[index], NULL, &ptproject);
								if (status == SUCCESS)
								{
									double dDistanceXY = ptOut.DistanceXY(ptproject);
									if (dDistanceXY > 10 && (ptproject.z < ptfacemin.z || ptproject.z > ptfacemin.z))//有误差，所以不能完全判断是否是面的点
									{
										continue;
									}
								}
								else
								{
									continue;
								}
							}
						}

						double ditanceBetweenFace = ptmid.Distance(ptproject);
						if (ditanceBetweenFace < 1)
						{
							continue;
						}
						else if (distance < 1 || ditanceBetweenFace < distance)
						{
							distance = ditanceBetweenFace;
						}

					}
					double dAnchorLeng = la0;
					if (!bIsin)
					{
						dAnchorLeng = lae;
					}

					if (ptmid.Distance(ptStart) < ptmid.Distance(ptEnd))
					{
						m_vecEndNormalStart = VecAngle;
						m_bStartAnhorselslantedFace = true;
						m_verSlabFaceInfo.strtype.endType = 8;
						m_verSlabFaceInfo.strtype.rotateAngle = angel;
						m_verSlabFaceInfo.strtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.strtype.endPtInfo.value3 = dAnchorLeng;
						if (bIsin)
						{
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;
						}
						else
						{
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = m_verSlabFaceInfo.dStartanchoroffset - dSideCoverConcrete - bendradius / 2;
						}
					}
					else
					{
						m_vecEndNormalEnd = VecAngle;
						m_bEndAnhorselslantedFace = true;
						m_verSlabFaceInfo.endtype.endType = 8;
						m_verSlabFaceInfo.endtype.rotateAngle = angel;
						m_verSlabFaceInfo.endtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.endtype.endPtInfo.value3 = dAnchorLeng;
						if (bIsin)
						{
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;

						}
						else
						{
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = m_verSlabFaceInfo.dEndanchoroffset - dSideCoverConcrete - bendradius / 2;
						}
					}


				}
				else if (abs(vec1.DotProduct(vecfacenor)) > 0.9 && abs(ptDefault.DotProduct(vec2)) > 0.9)//配筋方向平行，另一方向平行Z变时配筋面为竖直面
				{
					Dpoint3d ptmin, ptmax;
					mdlElmdscr_computeRange(&ptmin, &ptmax, tmpAnchordescrs[i], nullptr);
					Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);//锚入面中心点

					EditElementHandle eehtest(m_Solid->GetElementRef(), m_Solid->GetModelRef());
					EditElementHandle Eleeh;
					std::vector<EditElementHandle*> Holeehs;
					EFT::GetSolidElementAndSolidHoles(eehtest, Eleeh, Holeehs);

					EditElementHandle copyEleeh;
					copyEleeh.Duplicate(Eleeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(Eleeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(copyEleeh);

					EditElementHandle copyEleeh2;
					copyEleeh2.Duplicate(eehtest);
					ElementCopyContext copier1(ACTIVEMODEL);
					copier1.SetSourceModelRef(Eleeh.GetModelRef());
					copier1.SetTransformToDestination(true);
					copier1.SetWriteElements(false);
					copier1.DoCopy(copyEleeh2);

					std::vector<EditElementHandle*> allFaces;//未填充
					std::vector<EditElementHandle*> allParalFaces;
					ExtractFacesTool::GetFaces(copyEleeh2, allFaces);

					std::vector<EditElementHandle*> allFaces1;//填充
					std::vector<EditElementHandle*> allParalFaces1;
					ExtractFacesTool::GetFaces(copyEleeh, allFaces1);



					Dpoint3d ptMin, ptMax;
					mdlElmdscr_computeRange(&ptMin, &ptMax, m_face.GetElementDescrCP(), nullptr);
					Dpoint3d ptrebarFaceMid = Dpoint3d::From((ptMin.x + ptMax.x) / 2, (ptMin.y + ptMax.y) / 2, (ptMin.z + ptMax.z) / 2);//配筋面中心点

					//所有与配筋面平行的面
					DVec3d facenormal = GetfaceNormal();

					bool bIsin = true;
					//bool bflag = false;
					for (size_t index = 0; index < allFaces1.size(); index++)
					{
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allFaces1[index]->GetElementDescrP(), &ptDefault);
						if (abs(vecface.DotProduct(facenormal)) > 0.9)
						{
							//allParalFaces1.push_back(allFaces1[index]);
							Dpoint3d faceptMin, faceptMax;
							mdlElmdscr_computeRange(&faceptMin, &faceptMax, allFaces1[index]->GetElementDescrCP(), nullptr);
							Dpoint3d ptFaceMid = Dpoint3d::From((faceptMin.x + faceptMax.x) / 2, (faceptMin.y + faceptMax.y) / 2, (faceptMin.z + faceptMax.z) / 2);
							double distance = ptFaceMid.Distance(ptrebarFaceMid);
							if (COMPARE_VALUES_EPS(distance, 0.0, 10) == 0)
							{
								bIsin = false;
							}
							//else if (COMPARE_VALUES_EPS(ptFaceMid.x, ptrebarFaceMid.x, 10) == 0 && COMPARE_VALUES_EPS(ptFaceMid.y, ptrebarFaceMid.y, 10) == 0)
							//{
							//	//bflag = true;
							//	bIsin = false;
							//}
						}
					}

					Dpoint3d ptproject;
					mdlVec_projectPointToPlane(&ptproject, &ptrebarFaceMid, &ptmid, &vecfacenor);
					DVec3d VecAngle = ptmid - ptproject;
					VecAngle.Normalize();

					double dExtendDistance = 0;
					if (VecAngle.DotProduct(facenormal) < 0.9)
					{
						dExtendDistance = 2 * dSideCoverConcrete + diameter;
					}
					/*if (!bIsin)
					{
						VecAngle.Negate();
					}*/

					double thod = GetfaceNormal().AngleTo(VecAngle);
					double angel = thod / PI * 180;

					for (size_t index = 0; index < allFaces.size(); index++)
					{
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allFaces[index]->GetElementDescrP(), &ptDefault);
						if (abs(vecface.DotProduct(vecfacenor)) > 0.9)
						{
							allParalFaces.push_back(allFaces[index]);
						}
					}


					double distance = 0.0;
					for (size_t index = 0; index < allParalFaces.size(); index++)
					{

						Dpoint3d ptfacemin, ptfacemax;
						mdlElmdscr_computeRange(&ptfacemin, &ptfacemax, allParalFaces[index]->GetElementDescrCP(), nullptr);
						Dpoint3d ptfaceMid = Dpoint3d::From((ptfacemin.x + ptfacemax.x) / 2, (ptfacemin.y + ptfacemax.y) / 2, (ptfacemin.z + ptfacemax.z) / 2);
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allParalFaces[index]->GetElementDescrP(), &ptDefault);
						Dpoint3d ptproject;
						mdlVec_projectPointToPlane(&ptproject, &ptmid, &ptfaceMid, &vecface);
						ISolidKernelEntityPtr entityPtr;
						if (SolidUtil::Convert::ElementToBody(entityPtr, *allParalFaces[index]) == SUCCESS)
						{
							if (!SolidUtil::IsPointInsideBody(*entityPtr, ptproject))
							{
								Dpoint3d ptOut = Dpoint3d::FromZero();
								StatusInt status = mdlMeasure_closestPointOnElement(&ptOut, allParalFaces[index], NULL, &ptproject);
								if (status == SUCCESS)
								{
									double dDistanceXY = ptOut.DistanceXY(ptproject);
									if (dDistanceXY > 10 && (ptproject.z < ptfacemin.z || ptproject.z > ptfacemin.z))//有误差，所以不能完全判断是否是面的点
									{
										continue;
									}
								}
								else
								{
									continue;
								}
							}
						}

						double ditanceBetweenFace = ptmid.Distance(ptproject);
						if (ditanceBetweenFace < 1)
						{
							continue;
						}
						else if (distance < 1 || ditanceBetweenFace < distance)
						{
							distance = ditanceBetweenFace;
						}

					}
					double dAnchorLeng = la0;
					if (!bIsin)
					{
						dAnchorLeng = lae;
					}

					if (ptmid.Distance(ptStart) < ptmid.Distance(ptEnd))
					{
						m_vecEndNormalStart = VecAngle;
						m_bStartAnhorselslantedFace = true;
						m_verSlabFaceInfo.strtype.endType = 8;
						m_verSlabFaceInfo.strtype.rotateAngle = angel;
						m_verSlabFaceInfo.strtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.strtype.endPtInfo.value3 = dAnchorLeng;
						if (bIsin)
						{
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;
						}
						else
						{
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = m_verSlabFaceInfo.dStartanchoroffset - dSideCoverConcrete - bendradius / 2 + dExtendDistance;
						}
					}
					else
					{
						m_vecEndNormalEnd = VecAngle;
						m_bEndAnhorselslantedFace = true;
						m_verSlabFaceInfo.endtype.endType = 8;
						m_verSlabFaceInfo.endtype.rotateAngle = angel;
						m_verSlabFaceInfo.endtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.endtype.endPtInfo.value3 = dAnchorLeng;
						if (bIsin)
						{
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;

						}
						else
						{
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = m_verSlabFaceInfo.dEndanchoroffset - dSideCoverConcrete - bendradius / 2 + dExtendDistance;
						}
					}




				}
			}
			else  if (abs(ptDefault.DotProduct(vecfacenor)) > 0.9)//锚固面平行面
			{
				if (abs(vec1.DotProduct(vecfacenor)) > 0.9 &&  abs(ptDefault.DotProduct(vec2)) < 0.1)//配筋面为竖直面
				{
					Dpoint3d ptmin, ptmax;
					mdlElmdscr_computeRange(&ptmin, &ptmax, tmpAnchordescrs[i], nullptr);
					Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);

					EditElementHandle eehtest(m_Solid->GetElementRef(), m_Solid->GetModelRef());
					EditElementHandle Eleeh;
					std::vector<EditElementHandle*> Holeehs;
					EFT::GetSolidElementAndSolidHoles(eehtest, Eleeh, Holeehs);

					EditElementHandle copyEleeh;
					copyEleeh.Duplicate(Eleeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(Eleeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(copyEleeh);

					EditElementHandle copyEleeh2;
					copyEleeh2.Duplicate(eehtest);
					ElementCopyContext copier1(ACTIVEMODEL);
					copier1.SetSourceModelRef(Eleeh.GetModelRef());
					copier1.SetTransformToDestination(true);
					copier1.SetWriteElements(false);
					copier1.DoCopy(copyEleeh2);


					if (!Eleeh.IsValid())
					{
						mdlDialog_dmsgsPrint(L"非法的板实体!");

					}

					DPoint3d minPos;
					EditElementHandle downface;
					if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, true))
					{
						return;
					}

					EditElementHandle upface;
					if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, upface, minPos, false))
					{
						return;
					}

					Dpoint3d ptfaceStart, ptfaceEnd;
					mdlElmdscr_computeRange(&ptfaceStart, &ptfaceEnd, m_face.GetElementDescrCP(), nullptr);
					Dpoint3d mid = DPoint3d::From((ptfaceStart.x + ptfaceEnd.x) / 2, (ptfaceStart.y + ptfaceEnd.y) / 2, (ptfaceStart.z + ptfaceEnd.z) / 2);

					Dpoint3d ptfaceStartDown, ptfaceEndDown;
					mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
					Dpoint3d middown = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);

					Dpoint3d ptfaceStartUp, ptfaceEndUp;
					mdlElmdscr_computeRange(&ptfaceStartUp, &ptfaceEndUp, upface.GetElementDescrCP(), nullptr);
					Dpoint3d midup = DPoint3d::From((ptfaceStartUp.x + ptfaceEndUp.x) / 2, (ptfaceStartUp.y + ptfaceEndUp.y) / 2, (ptfaceStartUp.z + ptfaceEndUp.z) / 2);

					bool bIsin = false;//内外面，确定弯钩
					double angel = 0;

					DVec3d VecAngle = GetfaceNormal();//DVec3d::FromCrossProduct(vec2, vecfacenor);
					if (ptmid.z > mid.z) //上面
					{
						double thod = GetfaceNormal().AngleTo(VecAngle);
						angel = thod / PI * 180;
						/*if (mid.z - middown.z > 10)
						{
							bIsin = true;
						}
						else
						{
							bIsin = false;
						}
						if (VecAngle.z < 0)
						{
							VecAngle.Negate();
						}*/
					}
					else //下面
					{
						//VecAngle.Negate();

						if (ptmid.z - middown.z > 10)
						{
							bIsin = true;
							VecAngle.Negate();
						}
						else
						{
							bIsin = false;
							/*if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, false, const_cast<MSElementDescrP>(m_face.GetElementDescrCP())))
							{
								return;
							}*/
						}
						double thod = GetfaceNormal().AngleTo(VecAngle);
						angel = thod / PI * 180;
						//if (VecAngle.z > 0)
						//{
						//	VecAngle.Negate();
						//}
					}

					std::vector<EditElementHandle*> allFaces;
					std::vector<EditElementHandle*> allParalFaces;
					ExtractFacesTool::GetFaces(copyEleeh2, allFaces);
					for (size_t index = 0; index < allFaces.size(); index++)
					{
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allFaces[index]->GetElementDescrP(), &ptDefault);
						if (abs(vecface.DotProduct(vecfacenor)) > 0.9)
						{
							allParalFaces.push_back(allFaces[index]);
						}
					}

					double distance = 0.0;
					for (size_t index = 0; index < allParalFaces.size(); index++)
					{

						Dpoint3d ptfacemin, ptfacemax;
						mdlElmdscr_computeRange(&ptfacemin, &ptfacemax, allParalFaces[index]->GetElementDescrCP(), nullptr);
						Dpoint3d ptfaceMid = Dpoint3d::From((ptfacemin.x + ptfacemax.x) / 2, (ptfacemin.y + ptfacemax.y) / 2, (ptfacemin.z + ptfacemax.z) / 2);
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allParalFaces[index]->GetElementDescrP(), &ptDefault);
						Dpoint3d ptproject;
						mdlVec_projectPointToPlane(&ptproject, &ptmid, &ptfaceMid, &vecface);

						/*ISolidKernelEntityPtr entityPtr;
						if (SolidUtil::Convert::ElementToBody(entityPtr, *allParalFaces[index]) == SUCCESS)
						{
							if (!SolidUtil::IsPointInsideBody(*entityPtr, ptproject))
							{
								continue;
							}
						}
						double ditanceBetweenFace = ptmid.Distance(ptproject);
						if (ditanceBetweenFace < 1)
						{
							continue;
						}
						else if (distance < 1 || ditanceBetweenFace < distance)
						{
							distance = ditanceBetweenFace;
						}*/
						//斜面求得ptmid不在面上，导致后续求得点都不在面上，修改判断方式，直接以Z小于ptmid.z且distance最大得面为钢筋贴近面					
						if (ptmid.z - ptproject.z < 10)
						{
							continue;
						}
						double ditanceBetweenFace = ptmid.Distance(ptproject);
						if (ditanceBetweenFace < 1)
						{
							continue;
						}
						else if (distance < 1 || ditanceBetweenFace > distance)
						{
							distance = ditanceBetweenFace;
						}
					}
					double dAnchorLeng = la0;
					if (!bIsin)
					{
						dAnchorLeng = lae;
					}

					if (ptmid.Distance(ptStart) < ptmid.Distance(ptEnd))
					{
						m_vecEndNormalStart = VecAngle;
						m_bStartAnhorselslantedFace = true;
						m_verSlabFaceInfo.strtype.endType = 8;
						m_verSlabFaceInfo.strtype.rotateAngle = angel;
						m_verSlabFaceInfo.strtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.strtype.endPtInfo.value3 = dAnchorLeng;
						if (bIsin)
						{
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;
						}
						else
						{
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = m_verSlabFaceInfo.dStartanchoroffset - dSideCoverConcrete - bendradius / 2;
						}
					}
					else
					{
						m_vecEndNormalEnd = VecAngle;
						m_bEndAnhorselslantedFace = true;
						m_verSlabFaceInfo.endtype.endType = 8;
						m_verSlabFaceInfo.endtype.rotateAngle = angel;
						m_verSlabFaceInfo.endtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.endtype.endPtInfo.value3 = dAnchorLeng;
						if (bIsin)
						{
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;

						}
						else
						{
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = m_verSlabFaceInfo.dEndanchoroffset - dSideCoverConcrete - bendradius / 2;
						}
					}

				}
			}

		}
		else
		{
			if (abs(ptDefault.DotProduct(vecfacenor)) < 0.1)//垂直表示竖直墙
			{
				//平行表示配筋面是水平面
				if (abs(vec1.DotProduct(vecfacenor)) > 0.9)
				{
					Dpoint3d ptmin, ptmax;
					mdlElmdscr_computeRange(&ptmin, &ptmax, tmpAnchordescrs[i], nullptr);
					Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);

					EditElementHandle eehtest(m_Solid->GetElementRef(), m_Solid->GetModelRef());
					EditElementHandle Eleeh;
					std::vector<EditElementHandle*> Holeehs;
					EFT::GetSolidElementAndSolidHoles(eehtest, Eleeh, Holeehs);

					EditElementHandle copyEleeh;
					copyEleeh.Duplicate(Eleeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(Eleeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(copyEleeh);

					if (!Eleeh.IsValid())
					{
						mdlDialog_dmsgsPrint(L"非法的板实体!");

					}

					DPoint3d minPos;
					EditElementHandle downface;
					if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, true, const_cast<MSElementDescrP>(m_face.GetElementDescrCP())))
					{
						return;
					}
					Dpoint3d ptfaceStart, ptfaceEnd;
					mdlElmdscr_computeRange(&ptfaceStart, &ptfaceEnd, m_face.GetElementDescrCP(), nullptr);
					Dpoint3d mid = DPoint3d::From((ptfaceStart.x + ptfaceEnd.x) / 2, (ptfaceStart.y + ptfaceEnd.y) / 2, (ptfaceStart.z + ptfaceEnd.z) / 2);

					Dpoint3d ptfaceStartDown, ptfaceEndDown;
					mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
					Dpoint3d middown = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);

					bool bIsin = false;//内外面，确定弯钩
					double angel = 0;
					if (ptmid.z > ptStart.z) //上面
					{
						double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, 1));
						angel = thod / PI * 180;
						if (mid.z - middown.z > 10)
						{
							bIsin = true;
						}
						else
						{
							bIsin = false;
						}
					}
					else //下面
					{
						double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, -1));
						angel = thod / PI * 180;
						if (mid.z - middown.z > 10)
						{
							bIsin = false;
						}
						else
						{
							bIsin = true;
							if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, false, const_cast<MSElementDescrP>(m_face.GetElementDescrCP())))
							{
								return;
							}
						}
					}

					std::vector<EditElementHandle*> allFaces;
					std::vector<EditElementHandle*> allParalFaces;
					ExtractFacesTool::GetFaces(copyEleeh, allFaces);
					for (size_t index = 0; index < allFaces.size(); index++)
					{
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allFaces[index]->GetElementDescrP(), &ptDefault);
						if (abs(vecface.DotProduct(vecfacenor)) > 0.9)
						{
							allParalFaces.push_back(allFaces[index]);
						}
					}

					double distance = 0.0;
					for (size_t index = 0; index < allParalFaces.size(); index++)
					{

						Dpoint3d ptfacemin, ptfacemax;
						mdlElmdscr_computeRange(&ptfacemin, &ptfacemax, allParalFaces[index]->GetElementDescrCP(), nullptr);
						Dpoint3d ptfaceMid = Dpoint3d::From((ptfacemin.x + ptfacemax.x) / 2, (ptfacemin.y + ptfacemax.y) / 2, (ptfacemin.z + ptfacemax.z) / 2);
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allParalFaces[index]->GetElementDescrP(), &ptDefault);
						Dpoint3d ptproject;
						mdlVec_projectPointToPlane(&ptproject, &ptmid, &ptfaceMid, &vecface);
						double ditanceBetweenFace = ptmid.Distance(ptproject);
						if (ditanceBetweenFace < 1)
						{
							continue;
						}
						else if ((distance < 1 || ditanceBetweenFace < distance))
						{
							//找到离锚固端最近的面
							if (ptmid.DistanceXY(ptStart) < ptmid.DistanceXY(ptEnd))//锚固端为起始端
							{
								if (ptfaceMid.DistanceXY(ptStart) < ptfaceMid.DistanceXY(ptEnd))
								{

									distance = ditanceBetweenFace;
								}
							}
							else //锚固端为末端
							{
								if (ptfaceMid.DistanceXY(ptStart) > ptfaceMid.DistanceXY(ptEnd))
								{

									distance = ditanceBetweenFace;
								}
							}


						}

					}

					double dAnchorLeng = la0;
					if (!bIsin)
					{
						dAnchorLeng = lae;
					}

					if (isInface)
					{

						if (ptmid.DistanceXY(ptStart) < ptmid.DistanceXY(ptEnd))
						{
							m_insidef.strtype.endType = 4;
							m_insidef.strtype.rotateAngle = angel;
							m_insidef.strtype.endPtInfo.value1 = bendradius;
							m_insidef.strtype.endPtInfo.value3 = dAnchorLeng;
							m_insidef.m_bStartIsSlefAhol = true;
							if (bIsin)
							{
								m_insidef.bStartAnhorsel = true;
								m_insidef.dStartanchoroffset = distance - WallRebars_OffsetLength(ptStart);//diameter * 2;
							}
						}
						else
						{

							m_insidef.endtype.endType = 4;
							m_insidef.endtype.rotateAngle = angel;
							m_insidef.endtype.endPtInfo.value1 = bendradius;
							m_insidef.endtype.endPtInfo.value3 = dAnchorLeng;
							m_insidef.m_bEndIsSlefAhol = true;
							if (bIsin)
							{
								m_insidef.bEndAnhorsel = true;
								m_insidef.dEndanchoroffset = distance - WallRebars_OffsetLength(ptEnd);//diameter * 2;
							}
						}
					}
					else
					{
						if (ptmid.DistanceXY(ptStart) < ptmid.DistanceXY(ptEnd))
						{
							m_outsidef.strtype.endType = 4;
							m_outsidef.strtype.rotateAngle = angel;
							m_outsidef.strtype.endPtInfo.value1 = bendradius;
							m_outsidef.strtype.endPtInfo.value3 = dAnchorLeng;
							m_outsidef.m_bStartIsSlefAhol = true;
							if (bIsin)
							{
								m_outsidef.bStartAnhorsel = true;
								m_outsidef.dStartanchoroffset = distance - WallRebars_OffsetLength(ptStart);//diameter * 2;
							}
						}
						else
						{
							m_outsidef.endtype.endType = 4;
							m_outsidef.endtype.rotateAngle = angel;
							m_outsidef.endtype.endPtInfo.value1 = bendradius;
							m_outsidef.endtype.endPtInfo.value3 = dAnchorLeng;
							m_outsidef.m_bEndIsSlefAhol = true;
							if (bIsin)
							{
								m_outsidef.bEndAnhorsel = true;
								m_outsidef.dEndanchoroffset = distance - WallRebars_OffsetLength(ptEnd);//diameter * 2;
							}
						}
					}
				}
			}
			else if (abs(ptDefault.DotProduct(vecfacenor)) > 0.1 &&abs(ptDefault.DotProduct(vecfacenor)) < 0.9) //锚入面斜面
			{

				if (abs(GetfaceNormal().DotProduct(DVec3d::From(0, 0, 1))) > 0.9)//法向量平行表示配筋面为水平面
				{
					if (abs(vec1.DotProduct(vecfacenor)) < 0.1)
					{
						//垂直线返回
						return;
					}

					DVec3d VecAngle = DVec3d::FromCrossProduct(vec2, vecfacenor);

					Dpoint3d ptmin, ptmax;
					mdlElmdscr_computeRange(&ptmin, &ptmax, tmpAnchordescrs[i], nullptr);
					Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);
					EditElementHandle eehtest(m_Solid->GetElementRef(), m_Solid->GetModelRef());

					EditElementHandle Eleeh;
					std::vector<EditElementHandle*> Holeehs;
					EFT::GetSolidElementAndSolidHoles(eehtest, Eleeh, Holeehs);
					EditElementHandle copyEleeh;
					copyEleeh.Duplicate(Eleeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(Eleeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(copyEleeh);

					if (!Eleeh.IsValid())
					{
						mdlDialog_dmsgsPrint(L"非法的板实体!");

					}

					DPoint3d minPos;
					EditElementHandle downface;
					if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, true, const_cast<MSElementDescrP>(m_face.GetElementDescrCP())))
					{
						return;
					}
					Dpoint3d ptfaceStart, ptfaceEnd;
					mdlElmdscr_computeRange(&ptfaceStart, &ptfaceEnd, m_face.GetElementDescrCP(), nullptr);
					Dpoint3d mid = DPoint3d::From((ptfaceStart.x + ptfaceEnd.x) / 2, (ptfaceStart.y + ptfaceEnd.y) / 2, (ptfaceStart.z + ptfaceEnd.z) / 2);

					Dpoint3d ptfaceStartDown, ptfaceEndDown;
					mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
					Dpoint3d middown = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);
					bool bIsin = false;//内外面，确定弯钩
					double angel = 0;
					double angeTemp = 0;
					double thickNess = 0;
					if (ptmid.z > ptStart.z) //上面
					{

						if (mid.z - middown.z > 10)
						{
							DPoint3d ptproject;
							DVec3d vectemp = GetfaceNormal();
							mdlVec_projectPointToPlane(&ptproject, &mid, &middown, &vectemp);
							thickNess = mid.Distance(ptproject);
							bIsin = true;
						}
						else
						{
							bIsin = false;
						}
						if (VecAngle.z < 0)
						{
							VecAngle.Negate();
						}
						angeTemp = VecAngle.AngleTo(DVec3d::From(0, 0, 1));
					}
					else //下面
					{
						if (mid.z - middown.z > 10)
						{
							bIsin = false;
						}
						else
						{
							bIsin = true;
							if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, false, const_cast<MSElementDescrP>(m_face.GetElementDescrCP())))
							{
								return;
							}

							mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
							middown = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);
							DPoint3d ptproject;
							DVec3d vectemp = GetfaceNormal();
							mdlVec_projectPointToPlane(&ptproject, &mid, &middown, &vectemp);
							thickNess = mid.Distance(middown);
						}
						if (VecAngle.z > 0)
						{
							VecAngle.Negate();
						}
						angeTemp = VecAngle.AngleTo(DVec3d::From(0, 0, -1));
					}

					double thod = GetfaceNormal().AngleTo(VecAngle);
					angel = thod / PI * 180;
					//将所有面转换到XOZ平面
					CVector3D ORIPT = GetPlacement().GetTranslation();
					CMatrix3D tmpmat = GetPlacement();
					Transform trans;
					tmpmat.AssignTo(trans);
					trans.InverseOf(trans);
					TransformInfo transinfo(trans);

					DPoint3d ptcenter = DPoint3d::From(0, 0, 0);
					DPoint3d facenormal = DPoint3d::From(0, 1, 0);
					Transform tran;
					mdlTMatrix_computeFlattenTransform(&tran, &ptcenter, &facenormal);
					TransformInfo tinfo(tran);
					//mdlElmdscr_add(face);


					downface.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(downface, transinfo);
					downface.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(downface, tinfo);
					//downface.AddToModel();
					EditElementHandle eehselect(m_face, true);
					eehselect.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehselect, transinfo);
					eehselect.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehselect, tinfo);
					//eehselect.AddToModel();

					Dpoint3d ptSelectmin, ptSelectmax;
					mdlElmdscr_computeRange(&ptSelectmin, &ptSelectmax, eehselect.GetElementDescrCP(), nullptr);

					Dpoint3d ptfacemin, ptfacemax;
					mdlElmdscr_computeRange(&ptfacemin, &ptfacemax, downface.GetElementDescrCP(), nullptr);
					double distance = ptSelectmax.Distance(ptfacemax) + ptSelectmin.Distance(ptfacemin);



					double dAnchorLen = 0;
					double dAnchorLeng = la0;
					if (!bIsin)
					{
						dAnchorLeng = lae;
					}

					if (isInface)
					{
						if (ptmid.DistanceXY(ptStart) < ptmid.DistanceXY(ptEnd))
						{
							m_vecEndNormalStart = VecAngle;
							m_insidef.strtype.endType = 8;
							m_insidef.strtype.rotateAngle = angel;
							m_insidef.strtype.endPtInfo.value1 = bendradius;
							m_insidef.strtype.endPtInfo.value3 = dAnchorLeng;
							m_bStartAnhorselslantedFace = true;
							m_insidef.m_bStartIsSlefAhol = true;
							if (bIsin)
							{
								dAnchorLen = tan(angeTemp)* thickNess + distance;
								m_insidef.bStartAnhorsel = true;
								m_insidef.dStartanchoroffset = dAnchorLen - diameter * 2;
							}
						}
						else
						{
							m_vecEndNormalEnd = VecAngle;
							m_insidef.endtype.endType = 8;
							m_insidef.endtype.rotateAngle = angel;
							m_insidef.endtype.endPtInfo.value1 = bendradius;
							m_insidef.endtype.endPtInfo.value3 = dAnchorLeng;
							m_insidef.m_bEndIsSlefAhol = true;
							m_bEndAnhorselslantedFace = true;
							if (bIsin)
							{
								dAnchorLen = tan(angeTemp)* thickNess + distance;
								m_insidef.bEndAnhorsel = true;
								m_insidef.dEndanchoroffset = dAnchorLen - diameter * 2;
							}
						}
					}
					else
					{

						if (ptmid.DistanceXY(ptStart) < ptmid.DistanceXY(ptEnd))
						{
							m_vecEndNormalStart = VecAngle;
							m_outsidef.strtype.endType = 8;
							m_outsidef.strtype.rotateAngle = angel;
							m_outsidef.strtype.endPtInfo.value1 = bendradius;
							m_outsidef.strtype.endPtInfo.value3 = dAnchorLeng;
							m_outsidef.m_bStartIsSlefAhol = true;
							m_bStartAnhorselslantedFace = true;
							if (bIsin)
							{
								dAnchorLen = tan(angeTemp)* thickNess + distance;
								m_outsidef.bStartAnhorsel = true;
								m_outsidef.dStartanchoroffset = dAnchorLen - diameter * 2;
							}
						}
						else
						{
							m_vecEndNormalEnd = VecAngle;
							m_outsidef.endtype.endType = 8;
							m_outsidef.endtype.rotateAngle = angel;
							m_outsidef.endtype.endPtInfo.value1 = bendradius;
							m_outsidef.endtype.endPtInfo.value3 = dAnchorLeng;
							m_outsidef.m_bEndIsSlefAhol = true;
							m_bEndAnhorselslantedFace = true;
							if (bIsin)
							{
								dAnchorLen = tan(angeTemp)* thickNess + distance;
								m_outsidef.bEndAnhorsel = true;
								m_outsidef.dEndanchoroffset = dAnchorLen - diameter * 2;
							}
						}
					}


				}
			}
			else if (abs(ptDefault.DotProduct(vecfacenor)) > 0.9)//锚入水平面
			{

				DVec3d faceNormel = GetfaceNormal();
				if (abs(faceNormel.DotProduct(ptDefault)) < 0.1)//垂直，代表配筋面为竖直面
				{
					if (abs(vec1.DotProduct(ptDefault)) > 0.9) //Z方向才锚入 
					{
						Dpoint3d ptmin, ptmax;
						mdlElmdscr_computeRange(&ptmin, &ptmax, tmpAnchordescrs[i], nullptr);
						Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);

						EditElementHandle eehtest(m_Solid->GetElementRef(), m_Solid->GetModelRef());
						EditElementHandle Eleeh;
						std::vector<EditElementHandle*> Holeehs;
						EFT::GetSolidElementAndSolidHoles(eehtest, Eleeh, Holeehs);

						EditElementHandle copyEleeh;
						copyEleeh.Duplicate(Eleeh);
						ElementCopyContext copier2(ACTIVEMODEL);
						copier2.SetSourceModelRef(Eleeh.GetModelRef());
						copier2.SetTransformToDestination(true);
						copier2.SetWriteElements(false);
						copier2.DoCopy(copyEleeh);

						if (!Eleeh.IsValid())
						{
							mdlDialog_dmsgsPrint(L"非法的板实体!");

						}

						DPoint3d minPos;
						EditElementHandle downface;
						if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, true, tmpAnchordescrs[i]))
						{
							return;
						}
						Dpoint3d ptfaceStart, ptfaceEnd;
						mdlElmdscr_computeRange(&ptfaceStart, &ptfaceEnd, m_face.GetElementDescrCP(), nullptr);
						Dpoint3d mid = DPoint3d::From((ptfaceStart.x + ptfaceEnd.x) / 2, (ptfaceStart.y + ptfaceEnd.y) / 2, (ptfaceStart.z + ptfaceEnd.z) / 2);

						Dpoint3d ptfaceStartDown, ptfaceEndDown;
						mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
						Dpoint3d middown = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);
						bool bIsin = false;//内外面，确定弯钩
						double angel = 0;

						Dpoint3d pt1 = ptmid;
						pt1.z = 0;
						Dpoint3d pt2 = mid;
						pt2.z = 0;
						DVec3d vecRoate = pt1 - pt2;

						Dpoint3d ptuP, ptDown;
						if (ptStart.z > ptEnd.z)
						{
							ptuP = ptStart;
							ptDown = ptEnd;
						}
						else
						{
							ptuP = ptEnd;
							ptDown = ptStart;
						}
						double distance = 0.0;
						if (ptmid.z > mid.z) //上面
						{

							double thod = GetfaceNormal().AngleTo(vecRoate);
							angel = thod / PI * 180;
							if (ptmid.z - middown.z > 10)
							{
								bIsin = false;
							}
							else
							{
								bIsin = true;
								if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, false, tmpAnchordescrs[i]))
								{
									return;
								}
								mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
								Dpoint3d middown1 = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);
								DPoint3d ptprojec;
								mdlVec_projectPointToPlane(&ptprojec, &ptuP, &middown1, &vecfacenor);
								distance = ptuP.Distance(ptprojec);
							}
						}
						else //下面
						{
							double thod = GetfaceNormal().AngleTo(vecRoate);
							angel = thod / PI * 180;
							if (ptmid.z - middown.z > 10)
							{
								bIsin = true;
								if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, true, tmpAnchordescrs[i]))
								{
									return;
								}
								mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
								Dpoint3d middown1 = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);

								DPoint3d ptprojec;
								mdlVec_projectPointToPlane(&ptprojec, &ptDown, &middown1, &vecfacenor);
								distance = ptDown.Distance(ptprojec);
							}
							else
							{
								bIsin = false;

							}
						}

						double dAnchorLeng = la0;
						if (!bIsin)
						{
							dAnchorLeng = lae;
						}
						if (ptmid.Distance(ptStart) < ptmid.Distance(ptEnd))
						{
							m_verSlabFaceInfo.strtype.endType = 4;
							m_verSlabFaceInfo.strtype.rotateAngle = angel;
							m_verSlabFaceInfo.strtype.endPtInfo.value1 = bendradius;
							m_verSlabFaceInfo.strtype.endPtInfo.value3 = dAnchorLeng;
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = -diameterPre;
							if (bIsin)
							{
								//m_verSlabFaceInfo.bStartAnhorsel = true;
								m_verSlabFaceInfo.dStartanchoroffset = distance - WallRebars_OffsetLength(ptStart);//diameter * 2;
							}
						}
						else
						{

							m_verSlabFaceInfo.endtype.endType = 4;
							m_verSlabFaceInfo.endtype.rotateAngle = angel;
							m_verSlabFaceInfo.endtype.endPtInfo.value1 = bendradius;
							m_verSlabFaceInfo.endtype.endPtInfo.value3 = dAnchorLeng;
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = -diameterPre;
							if (bIsin)
							{
								//m_verSlabFaceInfo.bEndAnhorsel = true;
								m_verSlabFaceInfo.dEndanchoroffset = distance - WallRebars_OffsetLength(ptEnd);//diameter * 2;
							}
						}
					}
					else
					{

						double angel = 0;
						DPoint3d minP;
						DPoint3d maxP;
						DVec3d Normal = GetfaceNormal();
						Normal.Negate();
						Normal.ScaleToLength(500);

						bool isInt = false;
						std::vector<IDandModelref>  Same_Eles;
						GetNowScanElems(m_face, Same_Eles);
						for (auto it : Same_Eles)
						{
							string Ename, Etype;
							EditElementHandle tmpeeh(it.ID, it.tModel);
							if (GetEleNameAndType(tmpeeh, Ename, Etype))
							{
								if (Etype.find("WALL") != string::npos)
								{
									EditElementHandle eeh;
									MSElementDescrP interdescr = nullptr;
									eeh.Duplicate(tmpeeh);
									ElementCopyContext copier2(ACTIVEMODEL);
									copier2.SetSourceModelRef(tmpeeh.GetModelRef());
									copier2.SetTransformToDestination(true);
									copier2.SetWriteElements(false);
									copier2.DoCopy(eeh);
									EditElementHandle faceEeh(m_face, true);
									mdlCurrTrans_begin();
									Transform tMatrix;
									mdlTMatrix_getIdentity(&tMatrix);
									mdlTMatrix_setTranslation(&tMatrix, &Normal);
									MSElementDescrP newfaceEeh = faceEeh.GetElementDescrP();
									mdlElmdscr_transform(&newfaceEeh, &tMatrix);
									mdlCurrTrans_end();
									//mdlElmdscr_add(newfaceEeh);
									PITCommonTool::CSolidTool::SolidBoolWithFace(interdescr, newfaceEeh, tmpeeh.GetElementDescrP(), BOOLOPERATION_INTERSECT);
									if (interdescr != nullptr)
									{
										vector<EditElementHandle*> alltmpeeh;
										alltmpeeh.push_back(&tmpeeh);
										DPoint3d minP2 = { 0 }, maxP2 = { 0 };
										mdlElmdscr_computeRange(&minP2, &maxP2, newfaceEeh, NULL);
										minP2.z = (minP2.z + maxP2.z) / 2;
										maxP2.z = minP2.z;
										if (ISPointInHoles(alltmpeeh, minP2) || ISPointInHoles(alltmpeeh, maxP2))
										{
											isInt = true;
											break;
										}
									}

								}
							}
						}
						double dAnchorLeng = lae;
						if (isInt)
						{
							dAnchorLeng = la0;
							angel = 180;;
						}

						m_verSlabFaceInfo.strtype.endType = 4;
						m_verSlabFaceInfo.strtype.rotateAngle = angel;
						m_verSlabFaceInfo.strtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.strtype.endPtInfo.value3 = dAnchorLeng;
						m_verSlabFaceInfo.endtype.endType = 4;
						m_verSlabFaceInfo.endtype.rotateAngle = angel;
						m_verSlabFaceInfo.endtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.endtype.endPtInfo.value3 = dAnchorLeng;

					}

				}
				else //配筋面为斜面锚入水平面
				{
					if (abs(vec1.DotProduct(vecfacenor)) < 0.1)
					{
						double angel = 0;
						DPoint3d minP;
						DPoint3d maxP;
						DVec3d Normal = GetfaceNormal();
						Normal.Negate();
						Normal.ScaleToLength(500);

						bool isInt = false;
						std::vector<IDandModelref>  Same_Eles;
						GetNowScanElems(m_face, Same_Eles);
						for (auto it : Same_Eles)
						{
							string Ename, Etype;
							EditElementHandle tmpeeh(it.ID, it.tModel);
							if (GetEleNameAndType(tmpeeh, Ename, Etype))
							{
								if (Etype.find("WALL") != string::npos)
								{
									EditElementHandle eeh;
									MSElementDescrP interdescr = nullptr;
									eeh.Duplicate(tmpeeh);
									ElementCopyContext copier2(ACTIVEMODEL);
									copier2.SetSourceModelRef(tmpeeh.GetModelRef());
									copier2.SetTransformToDestination(true);
									copier2.SetWriteElements(false);
									copier2.DoCopy(eeh);
									EditElementHandle faceEeh(m_face, true);
									mdlCurrTrans_begin();
									Transform tMatrix;
									mdlTMatrix_getIdentity(&tMatrix);
									mdlTMatrix_setTranslation(&tMatrix, &Normal);
									MSElementDescrP newfaceEeh = faceEeh.GetElementDescrP();
									mdlElmdscr_transform(&newfaceEeh, &tMatrix);
									mdlCurrTrans_end();
									//mdlElmdscr_add(newfaceEeh);
									PITCommonTool::CSolidTool::SolidBoolWithFace(interdescr, newfaceEeh, tmpeeh.GetElementDescrP(), BOOLOPERATION_INTERSECT);
									if (interdescr != nullptr)
									{
										vector<EditElementHandle*> alltmpeeh;
										alltmpeeh.push_back(&tmpeeh);
										DPoint3d minP2 = { 0 }, maxP2 = { 0 };
										mdlElmdscr_computeRange(&minP2, &maxP2, newfaceEeh, NULL);
										minP2.z = (minP2.z + maxP2.z) / 2;
										maxP2.z = minP2.z;
										if (ISPointInHoles(alltmpeeh, minP2) || ISPointInHoles(alltmpeeh, maxP2))
										{
											isInt = true;
											break;
										}
									}

								}
							}
						}
						double dAnchorLeng = lae;
						if (isInt)
						{
							dAnchorLeng = la0;
							angel = 180;;
						}

						m_verSlabFaceInfo.strtype.endType = 4;
						m_verSlabFaceInfo.strtype.rotateAngle = angel;
						m_verSlabFaceInfo.strtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.strtype.endPtInfo.value3 = dAnchorLeng;
						m_verSlabFaceInfo.endtype.endType = 4;
						m_verSlabFaceInfo.endtype.rotateAngle = angel;
						m_verSlabFaceInfo.endtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.endtype.endPtInfo.value3 = dAnchorLeng;
						return;
					}

					DVec3d VecAngle = DVec3d::FromCrossProduct(vec2, vecfacenor);

					Dpoint3d ptmin, ptmax;
					mdlElmdscr_computeRange(&ptmin, &ptmax, tmpAnchordescrs[i], nullptr);
					//锚入面中心点
					Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);
					EditElementHandle eehtest(m_Solid->GetElementRef(), m_Solid->GetModelRef());

					EditElementHandle Eleeh;
					std::vector<EditElementHandle*> Holeehs;
					EFT::GetSolidElementAndSolidHoles(eehtest, Eleeh, Holeehs);
					EditElementHandle copyEleeh;
					copyEleeh.Duplicate(Eleeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(Eleeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(copyEleeh);

					if (!Eleeh.IsValid())
					{
						mdlDialog_dmsgsPrint(L"非法的板实体!");

					}

					DPoint3d minPos;
					EditElementHandle downface;
					if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, true, tmpAnchordescrs[i]))
					{
						return;
					}
					Dpoint3d ptfaceStart, ptfaceEnd;
					mdlElmdscr_computeRange(&ptfaceStart, &ptfaceEnd, m_face.GetElementDescrCP(), nullptr);
					//配筋面中心点
					Dpoint3d mid = DPoint3d::From((ptfaceStart.x + ptfaceEnd.x) / 2, (ptfaceStart.y + ptfaceEnd.y) / 2, (ptfaceStart.z + ptfaceEnd.z) / 2);

					Dpoint3d ptfaceStartDown, ptfaceEndDown;
					mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
					Dpoint3d middown = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);
					bool bIsin = false;//内外面，确定弯钩
					double angel = 0;
					double angeTemp = 0;
					double thickNess = 0;
					if (ptmid.z > mid.z) //上面
					{

						if (ptmid.z - middown.z > 10)
						{

							bIsin = false;
						}
						else
						{
							bIsin = true;
							if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, false, tmpAnchordescrs[i]))
							{
								return;
							}
							mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
							middown = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);
							Dpoint3d ptmidCopy = ptmid;
							ptmidCopy.z += 500;
							DPoint3d ptproject;
							GetIntersectedPointBtwFaceAndRay(&ptproject, downface.GetElementDescrP(), ptmid, ptmidCopy);
							/*DPoint3d ptproject;
							DVec3d vectemp = GetfaceNormal();
							mdlVec_projectPointToPlane(&ptproject, &ptmid, &middown, &vectemp);*/
							thickNess = ptmid.Distance(ptproject);


						}
						DVec3d vecCopy = vec1;
						DVec3d vecCopy1 = vec1;
						vecCopy.z = 0;
						vecCopy.Normalize();
						if (vec1.z < 0)
						{
							vecCopy.Negate();
							vecCopy1.Negate();
						}
						if (VecAngle.DotProduct(vecCopy) < -0.9)
						{
							VecAngle.Negate();
						}

						angeTemp = vecCopy1.AngleTo(DVec3d::From(0, 0, 1));

					}
					else //下面
					{
						if (ptmid.z - middown.z > 10)
						{
							bIsin = true;
							DVec3d vectemp = GetfaceNormal();
							Dpoint3d ptmidCopy = ptmid;
							ptmidCopy.z -= 500;
							Dpoint3d ptproject;
							GetIntersectedPointBtwFaceAndRay(&ptproject, downface.GetElementDescrP(), ptmid, ptmidCopy);
							//mdlVec_projectPointToPlane(&ptproject, &ptmid, &middown, &vectemp);
							thickNess = ptmid.Distance(ptproject);
						}
						else
						{
							bIsin = false;

						}
						DVec3d vecCopy = vec1;
						DVec3d vecCopy2 = vec1;
						vecCopy.z = 0;
						vecCopy.Normalize();
						if (vec1.z > 0)
						{
							vecCopy.Negate();
							vecCopy2.Negate();
						}
						if (VecAngle.DotProduct(vecCopy) < -0.9)
						{
							VecAngle.Negate();
						}
						angeTemp = vecCopy2.AngleTo(DVec3d::From(0, 0, -1));
					}

					double thod = GetfaceNormal().AngleTo(VecAngle);
					angel = thod / PI * 180;

					double distance = thickNess / cos(angeTemp);

					//斜面保护层便宜距离需要另外算
					double dSideCover = GetConcrete().sideCover * UOR_PER_MilliMeter;
					double dShortDistance = (dSideCover + 3 * diameter) / cos(angeTemp);
					double dSidedistance = dSideCover - diameter;
					double dSidedistance2 = dSideCover + 0.5*diameter;//不知道为什么
					while (irebarlevel > 0)//不知道为什么
					{
						BrString strRebarSize(GetMainRebars().at(irebarlevel - 1).rebarSize);
						m_holeRebarInfo.brstring = strRebarSize;
						double diameter = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL);
						dSidedistance += diameter;
						dSidedistance2 += diameter;
						irebarlevel--;
					}
					double dShortDistance2 = dSidedistance / tan(PI / 2 - angeTemp);

					//用于不是内面
					double dNotInFacedistanceDiff = dSidedistance2 * cos(PI / 2 - angeTemp) - dSideCover;

					double dDistanceDiffVec = dNotInFacedistanceDiff / cos(angeTemp);

					double dAnchorLen = 0;
					double dAnchorLeng = la0;
					if (!bIsin)
					{
						dAnchorLeng = lae;
					}

					if (ptmid.Distance(ptStart) < ptmid.Distance(ptEnd))
					{
						m_vecEndNormalStart = VecAngle;
						m_bStartAnhorselslantedFace = true;
						m_verSlabFaceInfo.strtype.endType = 8;
						m_verSlabFaceInfo.strtype.rotateAngle = angel;
						m_verSlabFaceInfo.strtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.strtype.endPtInfo.value3 = dAnchorLeng;
						if (bIsin)
						{
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = distance - dShortDistance - dShortDistance2;
						}
						else
						{
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = dDistanceDiffVec;
						}
					}
					else
					{
						m_vecEndNormalEnd = VecAngle;
						m_bEndAnhorselslantedFace = true;
						m_verSlabFaceInfo.endtype.endType = 8;
						m_verSlabFaceInfo.endtype.rotateAngle = angel;
						m_verSlabFaceInfo.endtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.endtype.endPtInfo.value3 = dAnchorLeng;
						if (bIsin)
						{
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = distance - dShortDistance - dShortDistance2;// -diameter * 2;

						}
						else
						{
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = dDistanceDiffVec;
						}
					}


				}


			}
		}



	}
}

bool PlaneRebarAssembly::GetHoleRebarAnchor(Dpoint3d ptstart, Dpoint3d ptend, Dpoint3d curPt, PITRebarEndType& endtype)
{
	//1、遇到孔洞需要往孔洞内互相锚入
	DRange3d range_CurFace;// 当前配筋面的范围
	mdlElmdscr_computeRange(&range_CurFace.low, &range_CurFace.high, m_CurentFace->GetElementDescrCP(), nullptr);
	double midZ = (range_CurFace.low.z + range_CurFace.high.z) / 2;// 当前配筋面的中心点的高度
	bool isUpRebar = false;// 是否是配筋面上面的钢筋
	if (COMPARE_VALUES(curPt.z, midZ) >= 0)
		isUpRebar = true;

	if (m_holeRebarInfo.bIsUpFace)// 外侧面
	{
		CVector3D vetor;
		if (isUpRebar)// 钢筋层在配筋面上面
		{
			vetor = CVector3D::From(0, 0, 1);
		}
		else// 钢筋层在配筋面下面
		{
			vetor = CVector3D::From(0, 0, -1);
		}
		endtype.SetType(PITRebarEndType::kBend);
		double startbendRadius = RebarCode::GetPinRadius(m_holeRebarInfo.brstring, ACTIVEMODEL, false);//value 1
		double diameter = RebarCode::GetBarDiameter(m_holeRebarInfo.brstring, ACTIVEMODEL);//value 3
		BrString strRebarSize(GetMainRebars().at(0).rebarSize);
		double lastDiameter = 0.0;//上一层的钢筋直径
		if (m_curLevel == 1)
			lastDiameter = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL);

		double startbendLen = m_slabThickness - (GetConcrete().postiveCover * 2 * UOR_PER_MilliMeter) - lastDiameter - m_curreDiameter / 2 - startbendRadius;

		endtype.SetendNormal(vetor);
		endtype.SetbendLen(startbendLen);
		endtype.SetbendRadius(startbendRadius);
	}
	else// 内侧面
	{
		CVector3D vetor;
		if (isUpRebar)// 钢筋层在配筋面上面
		{
			vetor = CVector3D::From(0, 0, 1);
		}
		else// 钢筋层在配筋面下面
		{
			vetor = CVector3D::From(0, 0, -1);
		}
		endtype.SetType(PITRebarEndType::kBend);
		double startbendRadius = RebarCode::GetPinRadius(m_holeRebarInfo.brstring, ACTIVEMODEL, false);//value 1
		double diameter = RebarCode::GetBarDiameter(m_holeRebarInfo.brstring, ACTIVEMODEL);//value 3
		BrString strRebarSize(GetMainRebars().at(0).rebarSize);
		double lastDiameter = 0.0;//上一层的钢筋直径
		if (m_curLevel == 1)
			lastDiameter = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL);

		double startbendLen = m_slabThickness - (GetConcrete().postiveCover * 2 * UOR_PER_MilliMeter) - lastDiameter - m_curreDiameter / 2 - startbendRadius;

		endtype.SetendNormal(vetor);
		endtype.SetbendLen(startbendLen);
		endtype.SetbendRadius(startbendRadius);
	}
	//1、遇到孔洞需要往孔洞内互相锚入


	//2、有孔洞且上方有墙需要向上锚入
	CVector3D ORIPT = GetPlacement().GetTranslation();
	CMatrix3D tmpmat = GetPlacement();
	Transform trans;
	tmpmat.AssignTo(trans);
	trans.InverseOf(trans);
	TransformInfo transinfo(trans);

	DPoint3d ptcenter = DPoint3d::From(0, 0, 0);
	DPoint3d facenormal = DPoint3d::From(0, 1, 0);
	Transform tran;
	mdlTMatrix_computeFlattenTransform(&tran, &ptcenter, &facenormal);
	TransformInfo tinfo(tran);

	vector<MSElementDescrP> tmpdescrs;
	for (int i = 0; i < m_ldfoordata.upnum; i++)
	{
		MSElementDescrP tmpupfaces = nullptr;
		m_ldfoordata.upfaces[i]->Duplicate(&tmpupfaces);
		//mdlElmdscr_add(tmpupfaces);
		EditElementHandle tmpeeh(tmpupfaces, true, false, ACTIVEMODEL);
		tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, transinfo);
		tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, tinfo);
		tmpupfaces = tmpeeh.ExtractElementDescr();
		tmpdescrs.push_back(tmpupfaces);
		//mdlElmdscr_add(tmpupfaces);
	}
	if (tmpdescrs.size() == 0)
	{
		return false;
	}
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstart, ptend), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, tinfo);
	//eeh.AddToModel();
	DPoint3d ptstart1, ptend1;
	mdlElmdscr_extractEndPoints(&ptstart1, nullptr, &ptend1, nullptr, eeh.ExtractElementDescr(), ACTIVEMODEL);
	int verRe = IsHaveVerticalWall(ptstart1, ptend1, &tmpdescrs.at(0), tmpdescrs.size());
	if (verRe == 1 || verRe == 2)//有钢筋点在墙的范围内，需要往墙上锚入
	{
		endtype.SetType(PITRebarEndType::kBend);
		double startbendRadius = RebarCode::GetPinRadius(m_holeRebarInfo.brstring, ACTIVEMODEL, false);//value 1
		double diameter = RebarCode::GetBarDiameter(m_holeRebarInfo.brstring, ACTIVEMODEL);//value 3
		double startbendLen = 15 * stod(m_holeRebarInfo.brstring.Get()) * UOR_PER_MilliMeter;
		double LaE = PlaneRebarAssembly::GetLae();
		if (LaE > 0)
		{
			LaE *= stod(m_holeRebarInfo.brstring.Get());
		}
		else
		{
			LaE = 15 * stod(m_holeRebarInfo.brstring.Get()) * UOR_PER_MilliMeter;
		}
		CVector3D vetor;
		if (!m_holeRebarInfo.bIsUpFace && isUpRebar)
		{
			startbendLen = LaE;

		}
		vetor = CVector3D::From(0, 0, 1);
		endtype.SetendNormal(vetor);
		endtype.SetbendLen(startbendLen);
		endtype.SetbendRadius(startbendRadius);
	}
	//else
	//{
	//	CVector3D vetor;
	//	if (m_holeRebarInfo.bIsUpFace)
	//	{
	//		vetor = CVector3D::From(0, 0, -1);
	//	}
	//	else
	//	{
	//		vetor = CVector3D::From(0, 0, 1);
	//	}
	//	endtype.SetType(PITRebarEndType::kBend);

	//	double startbendRadius = RebarCode::GetPinRadius(m_holeRebarInfo.brstring, ACTIVEMODEL, false);//value 1
	//	double diameter = RebarCode::GetBarDiameter(m_holeRebarInfo.brstring, ACTIVEMODEL);//value 3
	//	double startbendLen = 15 * stod(m_holeRebarInfo.brstring.Get()) * UOR_PER_MilliMeter;
	//	double LaE = PlaneRebarAssembly::GetLae();
	//	endtype.SetendNormal(vetor);
	//	endtype.SetbendLen(startbendLen);
	//	endtype.SetbendRadius(startbendRadius);
	//	return false;
	//}

}

/*
* @desc:	根据孔洞截断钢筋锚固长度得到新的锚固长度
* @param[in] ptStr 钢筋锚固部分的起始点
* @param[in] vecAnchor 锚固方向，用来计算锚固结束点
* @param[in] rebarLevel 当前钢筋层，如果ptStr是当前层起始点，则填0，如果ptStr是第一层起始点，则填当前层
* @param[in\out] dAnchorleng 锚固长度，用来计算锚固结束点，并在截断后更新为新的长度
* @author	Hong ZhuoHui
* @Date:	2024/3/1
*/
void PlaneRebarAssembly::CutRebarAnchorLeng(Dpoint3d ptStr, CVector3D vecAnchor, int rebarLevel, double & dAnchorleng)
{
	double dSideCover = GetConcrete().sideCover * UOR_PER_MilliMeter;
	Dpoint3d ptEnd = ptStr;
	vecAnchor.ScaleToLength(dAnchorleng);
	ptEnd.Add(vecAnchor);
	vector<Dpoint3d> vecPts;
	GetIntersectPointsWithHoles(vecPts, m_useHoleehs, ptStr, ptEnd, dSideCover);

	if (vecPts.size() > 0)
	{
		double minDis = dAnchorleng;
		for (auto it : vecPts)
		{
			double dis = ptStr.Distance(it);
			if (dis < dAnchorleng)
			{
				dAnchorleng = dis;
			}
		}
	}
	else
	{
		dAnchorleng = ptEnd.Distance(ptStr);
	}
	while (rebarLevel > 0)//前层
	{
		BrString strRebarSize(GetMainRebars().at(rebarLevel - 1).rebarSize);
		double diameter = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL);
		dAnchorleng - diameter;
		rebarLevel--;
	}
	dAnchorleng -= dSideCover;
}


//集水坑
void PlaneRebarAssembly::CreateCatchpitBySelf(vector<MSElementDescrP> tmpAnchordescrs, PIT::LineSegment Lineseg, double bendradius, double la0, double lae, double diameter, int irebarlevel, bool isInface, bool bisSumps, bool isYdir)
{
	DRange3d range_eeh;
	mdlElmdscr_computeRange(&range_eeh.low, &range_eeh.high, m_pOldElm->GetElementDescrCP(), nullptr);

	DRange3d range_CurFace;
	mdlElmdscr_computeRange(&range_CurFace.low, &range_CurFace.high, m_CurentFace->GetElementDescrCP(), nullptr);

	m_verSlabFaceInfo.ClearData();
	double dSideCoverConcrete = GetConcrete().sideCover * UOR_PER_MilliMeter;
	DVec3d vec2;
	if (Lineseg.IsEqual(m_LineSeg1))
	{
		vec2 = m_LineSeg2.GetLineVec();
	}
	else
	{
		vec2 = m_LineSeg1.GetLineVec();
	}
	DVec3d vec1 = Lineseg.GetLineVec();
	Dpoint3d ptStart = Lineseg.GetLineStartPoint();
	Dpoint3d ptEnd = Lineseg.GetLineEndPoint();
	bool flag = false;
	if (isYdir && (ptStart.z > ptEnd.z))
	{
		flag = true;
	}
	for (size_t i = 0; i < tmpAnchordescrs.size(); i++)
	{
		DVec3d vecfacenor;
		DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
		mdlElmdscr_extractNormal(&vecfacenor, nullptr, tmpAnchordescrs[i], &ptDefault);

		if (bisSumps) //集水坑锚固处理
		{
			if (abs(ptDefault.DotProduct(vecfacenor)) < 0.1)//垂直表示竖直面
			{
				//一个平行一个垂直 表示配筋面是水平面
				if (abs(vec1.DotProduct(vecfacenor)) > 0.9 &&  abs(ptDefault.DotProduct(vec2)) < 0.1)
				{
					Dpoint3d ptmin, ptmax;
					mdlElmdscr_computeRange(&ptmin, &ptmax, tmpAnchordescrs[i], nullptr);
					Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);

					EditElementHandle eehtest(m_Solid->GetElementRef(), m_Solid->GetModelRef());
					EditElementHandle Eleeh;
					std::vector<EditElementHandle*> Holeehs;
					EFT::GetSolidElementAndSolidHoles(eehtest, Eleeh, Holeehs);

					EditElementHandle copyEleeh;
					copyEleeh.Duplicate(Eleeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(Eleeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(copyEleeh);

					EditElementHandle copyEleeh2;
					copyEleeh2.Duplicate(eehtest);
					ElementCopyContext copier1(ACTIVEMODEL);
					copier1.SetSourceModelRef(Eleeh.GetModelRef());
					copier1.SetTransformToDestination(true);
					copier1.SetWriteElements(false);
					copier1.DoCopy(copyEleeh2);


					if (!Eleeh.IsValid())
					{
						mdlDialog_dmsgsPrint(L"非法的板实体!");

					}

					DPoint3d minPos;
					EditElementHandle downface;
					if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, true))
					{
						return;
					}

					EditElementHandle upface;
					if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, upface, minPos, false))
					{
						return;
					}

					Dpoint3d ptfaceStart, ptfaceEnd;
					mdlElmdscr_computeRange(&ptfaceStart, &ptfaceEnd, m_face.GetElementDescrCP(), nullptr);
					Dpoint3d mid = DPoint3d::From((ptfaceStart.x + ptfaceEnd.x) / 2, (ptfaceStart.y + ptfaceEnd.y) / 2, (ptfaceStart.z + ptfaceEnd.z) / 2);

					Dpoint3d ptfaceStartDown, ptfaceEndDown;
					mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
					Dpoint3d middown = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);

					Dpoint3d ptfaceStartUp, ptfaceEndUp;
					mdlElmdscr_computeRange(&ptfaceStartUp, &ptfaceEndUp, upface.GetElementDescrCP(), nullptr);
					Dpoint3d midup = DPoint3d::From((ptfaceStartUp.x + ptfaceEndUp.x) / 2, (ptfaceStartUp.y + ptfaceEndUp.y) / 2, (ptfaceStartUp.z + ptfaceEndUp.z) / 2);

					bool bIsin = false;//内外面，确定弯钩
					double angel = 0;

					DVec3d VecAngle = DVec3d::FromCrossProduct(vec2, vecfacenor);
					if (ptmid.z > ptStart.z) //上面
					{
						double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, 1));
						angel = thod / PI * 180;
						if (mid.z - middown.z > 10)
						{
							bIsin = true;
						}
						else
						{
							bIsin = false;
						}
						if (VecAngle.z < 0)
						{
							VecAngle.Negate();
						}
					}
					else //下面
					{
						double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, -1));
						angel = thod / PI * 180;
						if ((mid.z - midup.z) < 10)
						{
							bIsin = false;
						}
						else
						{
							bIsin = true;
							/*if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, false, const_cast<MSElementDescrP>(m_face.GetElementDescrCP())))
							{
								return;
							}*/
						}
						if (VecAngle.z > 0)
						{
							VecAngle.Negate();
						}
					}

					std::vector<EditElementHandle*> allFaces;
					std::vector<EditElementHandle*> allParalFaces;
					ExtractFacesTool::GetFaces(copyEleeh2, allFaces);
					for (size_t index = 0; index < allFaces.size(); index++)
					{
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allFaces[index]->GetElementDescrP(), &ptDefault);
						if (abs(vecface.DotProduct(vecfacenor)) > 0.9)
						{
							allParalFaces.push_back(allFaces[index]);
						}
					}

					double distance = 0.0;
					for (size_t index = 0; index < allParalFaces.size(); index++)
					{

						Dpoint3d ptfacemin, ptfacemax;
						mdlElmdscr_computeRange(&ptfacemin, &ptfacemax, allParalFaces[index]->GetElementDescrCP(), nullptr);
						Dpoint3d ptfaceMid = Dpoint3d::From((ptfacemin.x + ptfacemax.x) / 2, (ptfacemin.y + ptfacemax.y) / 2, (ptfacemin.z + ptfacemax.z) / 2);
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allParalFaces[index]->GetElementDescrP(), &ptDefault);
						Dpoint3d ptproject;
						mdlVec_projectPointToPlane(&ptproject, &ptmid, &ptfaceMid, &vecface);
						ISolidKernelEntityPtr entityPtr;
						if (SolidUtil::Convert::ElementToBody(entityPtr, *allParalFaces[index]) == SUCCESS)
						{
							if (!SolidUtil::IsPointInsideBody(*entityPtr, ptproject))
							{
								Dpoint3d ptOut = Dpoint3d::FromZero();
								StatusInt status = mdlMeasure_closestPointOnElement(&ptOut, allParalFaces[index], NULL, &ptproject);
								if (status == SUCCESS)
								{
									double dDistanceXY = ptOut.DistanceXY(ptproject);
									if (dDistanceXY > 10 && (ptproject.z < ptfacemin.z || ptproject.z > ptfacemin.z))//有误差，所以不能完全判断是否是面的点
									{
										continue;
									}
								}
								else
								{
									continue;
								}
							}
						}

						double ditanceBetweenFace = ptmid.Distance(ptproject);
						if (ditanceBetweenFace < 1)
						{
							continue;
						}
						else if (distance < 1 || ditanceBetweenFace < distance)
						{
							distance = ditanceBetweenFace;
						}

					}
					double dAnchorLeng = la0;
					if (!bIsin)
					{
						dAnchorLeng = lae;
					}

					if (ptmid.Distance(ptStart) < ptmid.Distance(ptEnd))
					{
						m_vecEndNormalStart = VecAngle;
						m_bStartAnhorselslantedFace = true;
						m_verSlabFaceInfo.strtype.endType = 8;
						m_verSlabFaceInfo.strtype.rotateAngle = angel;
						m_verSlabFaceInfo.strtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.strtype.endPtInfo.value3 = dAnchorLeng;
						DVec3d yDir = DVec3d::From(0, 1, 0);
						if (fabs(yDir.DotProduct(GetfaceNormal())) > 0.9 && isYdir && (fabs(range_eeh.ZLength() - range_CurFace.ZLength()) < 300) && flag && (m_CatchpitType == 1))
						{
							m_vecEndNormalStart.Negate();
							m_verSlabFaceInfo.strtype.endPtInfo.value3 = la0;
						}
						if (bIsin)
						{
							m_twoFacesDistance = distance;
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;
						}
						else
						{
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = m_verSlabFaceInfo.dStartanchoroffset - dSideCoverConcrete - bendradius / 2;
						}
					}
					else
					{
						m_vecEndNormalEnd = VecAngle;
						m_bEndAnhorselslantedFace = true;
						m_verSlabFaceInfo.endtype.endType = 8;
						m_verSlabFaceInfo.endtype.rotateAngle = angel;
						m_verSlabFaceInfo.endtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.endtype.endPtInfo.value3 = dAnchorLeng;
						DVec3d yDir = DVec3d::From(0, 1, 0);
						if (fabs(yDir.DotProduct(GetfaceNormal())) > 0.9 && isYdir && (fabs(range_eeh.ZLength() - range_CurFace.ZLength()) < 300) && !flag && (m_CatchpitType == 1))
						{
							m_vecEndNormalEnd.Negate();
							m_verSlabFaceInfo.endtype.endPtInfo.value3 = la0;
						}
						if (bIsin)
						{
							m_twoFacesDistance = distance;
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;

						}
						else
						{
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = m_verSlabFaceInfo.dEndanchoroffset - dSideCoverConcrete - bendradius / 2;
						}
					}


				}
				else if (abs(vec1.DotProduct(vecfacenor)) > 0.9 && abs(ptDefault.DotProduct(vec2)) > 0.9)//配筋方向平行，另一方向平行Z变时配筋面为竖直面
				{
					Dpoint3d ptmin, ptmax;
					mdlElmdscr_computeRange(&ptmin, &ptmax, tmpAnchordescrs[i], nullptr);
					Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);//锚入面中心点

					EditElementHandle eehtest(m_Solid->GetElementRef(), m_Solid->GetModelRef());
					EditElementHandle Eleeh;
					std::vector<EditElementHandle*> Holeehs;
					EFT::GetSolidElementAndSolidHoles(eehtest, Eleeh, Holeehs);

					EditElementHandle copyEleeh;
					copyEleeh.Duplicate(Eleeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(Eleeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(copyEleeh);

					EditElementHandle copyEleeh2;
					copyEleeh2.Duplicate(eehtest);
					ElementCopyContext copier1(ACTIVEMODEL);
					copier1.SetSourceModelRef(Eleeh.GetModelRef());
					copier1.SetTransformToDestination(true);
					copier1.SetWriteElements(false);
					copier1.DoCopy(copyEleeh2);

					std::vector<EditElementHandle*> allFaces;//未填充
					std::vector<EditElementHandle*> allParalFaces;
					ExtractFacesTool::GetFaces(copyEleeh2, allFaces);

					std::vector<EditElementHandle*> allFaces1;//填充
					std::vector<EditElementHandle*> allParalFaces1;
					ExtractFacesTool::GetFaces(copyEleeh, allFaces1);



					Dpoint3d ptMin, ptMax;
					mdlElmdscr_computeRange(&ptMin, &ptMax, m_face.GetElementDescrCP(), nullptr);
					Dpoint3d ptrebarFaceMid = Dpoint3d::From((ptMin.x + ptMax.x) / 2, (ptMin.y + ptMax.y) / 2, (ptMin.z + ptMax.z) / 2);//配筋面中心点

					//所有与配筋面平行的面
					DVec3d facenormal = GetfaceNormal();

					bool bIsin = true;
					for (size_t index = 0; index < allFaces1.size(); index++)
					{
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allFaces1[index]->GetElementDescrP(), &ptDefault);
						if (abs(vecface.DotProduct(facenormal)) > 0.9)
						{
							//allParalFaces1.push_back(allFaces1[index]);
							Dpoint3d faceptMin, faceptMax;
							mdlElmdscr_computeRange(&faceptMin, &faceptMax, allFaces1[index]->GetElementDescrCP(), nullptr);
							Dpoint3d ptFaceMid = Dpoint3d::From((faceptMin.x + faceptMax.x) / 2, (faceptMin.y + faceptMax.y) / 2, (faceptMin.z + faceptMax.z) / 2);
							double distance = ptFaceMid.Distance(ptrebarFaceMid);
							if (COMPARE_VALUES_EPS(distance, 0.0, 10) == 0)
							{
								bIsin = false;
							}
							else if (COMPARE_VALUES_EPS(ptFaceMid.x, ptrebarFaceMid.x, 10) == 0 && COMPARE_VALUES_EPS(ptFaceMid.y, ptrebarFaceMid.y, 10) == 0)
							{
								bIsin = false;
							}
						}
					}
					if (GetMainRebars().at(m_curLevel).rebarDir == 0)
						ptmid.z = ptrebarFaceMid.z;
					Dpoint3d ptproject;
					mdlVec_projectPointToPlane(&ptproject, &ptrebarFaceMid, &ptmid, &vecfacenor);
					DVec3d VecAngle = ptmid - ptproject;
					VecAngle.Normalize();

					double dExtendDistance = 0;
					if (abs(VecAngle.DotProduct(facenormal)) < 0.9 && m_CatchpitType != 2)
					{
						dExtendDistance = 2 * dSideCoverConcrete + diameter;
					}
					else if (VecAngle.DotProduct(facenormal) < 0.9 && m_CatchpitType == 2)
					{
						dExtendDistance = 2 * dSideCoverConcrete + diameter;
					}
					/*if (!bIsin)
					{
						VecAngle.Negate();
					}*/

					double thod = GetfaceNormal().AngleTo(VecAngle);
					double angel = thod / PI * 180;

					for (size_t index = 0; index < allFaces.size(); index++)
					{
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allFaces[index]->GetElementDescrP(), &ptDefault);
						if (abs(vecface.DotProduct(vecfacenor)) > 0.9)
						{
							allParalFaces.push_back(allFaces[index]);
						}
					}


					double distance = 0.0;
					for (size_t index = 0; index < allParalFaces.size(); index++)
					{

						Dpoint3d ptfacemin, ptfacemax;
						mdlElmdscr_computeRange(&ptfacemin, &ptfacemax, allParalFaces[index]->GetElementDescrCP(), nullptr);
						Dpoint3d ptfaceMid = Dpoint3d::From((ptfacemin.x + ptfacemax.x) / 2, (ptfacemin.y + ptfacemax.y) / 2, (ptfacemin.z + ptfacemax.z) / 2);
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allParalFaces[index]->GetElementDescrP(), &ptDefault);
						Dpoint3d ptproject;
						mdlVec_projectPointToPlane(&ptproject, &ptmid, &ptfaceMid, &vecface);
						ISolidKernelEntityPtr entityPtr;
						if (SolidUtil::Convert::ElementToBody(entityPtr, *allParalFaces[index]) == SUCCESS)
						{
							if (!SolidUtil::IsPointInsideBody(*entityPtr, ptproject))
							{
								Dpoint3d ptOut = Dpoint3d::FromZero();
								StatusInt status = mdlMeasure_closestPointOnElement(&ptOut, allParalFaces[index], NULL, &ptproject);
								if (status == SUCCESS)
								{
									double dDistanceXY = ptOut.DistanceXY(ptproject);
									if (dDistanceXY > 10 && (ptproject.z < ptfacemin.z || ptproject.z > ptfacemin.z))//有误差，所以不能完全判断是否是面的点
									{
										continue;
									}
								}
								else
								{
									continue;
								}
							}
						}

						double ditanceBetweenFace = ptmid.Distance(ptproject);
						if (ditanceBetweenFace < 1)
						{
							continue;
						}
						else if (distance < 1 || ditanceBetweenFace < distance)
						{
							distance = ditanceBetweenFace;
						}

					}
					double dAnchorLeng = la0;
					if (!bIsin)
					{
						dAnchorLeng = lae;
					}

					if (ptmid.Distance(ptStart) < ptmid.Distance(ptEnd))
					{
						m_vecEndNormalStart = VecAngle;
						m_bStartAnhorselslantedFace = true;
						m_verSlabFaceInfo.strtype.endType = 8;
						m_verSlabFaceInfo.strtype.rotateAngle = angel;
						m_verSlabFaceInfo.strtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.strtype.endPtInfo.value3 = dAnchorLeng;
						DVec3d yDir = DVec3d::From(0, 1, 0);
						if (fabs(yDir.DotProduct(GetfaceNormal())) > 0.9 && isYdir && (fabs(range_eeh.ZLength() - range_CurFace.ZLength()) < 300) && flag && (m_CatchpitType == 1))
						{
							m_vecEndNormalStart.Negate();
							m_verSlabFaceInfo.strtype.endPtInfo.value3 = la0;
						}
						if (bIsin)
						{
							m_twoFacesDistance = distance;
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;
						}
						else
						{
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = m_verSlabFaceInfo.dStartanchoroffset - dSideCoverConcrete - bendradius / 2 + dExtendDistance;
						}
					}
					else
					{
						m_vecEndNormalEnd = VecAngle;
						m_bEndAnhorselslantedFace = true;
						m_verSlabFaceInfo.endtype.endType = 8;
						m_verSlabFaceInfo.endtype.rotateAngle = angel;
						m_verSlabFaceInfo.endtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.endtype.endPtInfo.value3 = dAnchorLeng;
						DVec3d yDir = DVec3d::From(0, 1, 0);
						if (fabs(yDir.DotProduct(GetfaceNormal())) > 0.9 && isYdir && (fabs(range_eeh.ZLength() - range_CurFace.ZLength()) < 300) && !flag && (m_CatchpitType == 1))
						{
							m_vecEndNormalEnd.Negate();
							m_verSlabFaceInfo.endtype.endPtInfo.value3 = la0;
						}
						if (bIsin)
						{
							m_twoFacesDistance = distance;
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;

						}
						else
						{
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = m_verSlabFaceInfo.dEndanchoroffset - dSideCoverConcrete - bendradius / 2 + dExtendDistance;
						}
					}




				}
			}
			else  if (abs(ptDefault.DotProduct(vecfacenor)) > 0.9)//锚固面平行面
			{
				if (abs(vec1.DotProduct(vecfacenor)) > 0.9 &&  abs(ptDefault.DotProduct(vec2)) < 0.1)//配筋面为竖直面
				{
					Dpoint3d ptmin, ptmax;
					mdlElmdscr_computeRange(&ptmin, &ptmax, tmpAnchordescrs[i], nullptr);
					Dpoint3d ptmid = Dpoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);

					EditElementHandle eehtest(m_Solid->GetElementRef(), m_Solid->GetModelRef());
					EditElementHandle Eleeh;
					std::vector<EditElementHandle*> Holeehs;
					EFT::GetSolidElementAndSolidHoles(eehtest, Eleeh, Holeehs);

					EditElementHandle copyEleeh;
					copyEleeh.Duplicate(Eleeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(Eleeh.GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(copyEleeh);

					EditElementHandle copyEleeh2;
					copyEleeh2.Duplicate(eehtest);
					ElementCopyContext copier1(ACTIVEMODEL);
					copier1.SetSourceModelRef(Eleeh.GetModelRef());
					copier1.SetTransformToDestination(true);
					copier1.SetWriteElements(false);
					copier1.DoCopy(copyEleeh2);


					if (!Eleeh.IsValid())
					{
						mdlDialog_dmsgsPrint(L"非法的板实体!");

					}

					DPoint3d minPos;
					EditElementHandle downface;
					if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, true))
					{
						return;
					}

					EditElementHandle upface;
					if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, upface, minPos, false))
					{
						return;
					}

					Dpoint3d ptfaceStart, ptfaceEnd;
					mdlElmdscr_computeRange(&ptfaceStart, &ptfaceEnd, m_face.GetElementDescrCP(), nullptr);
					Dpoint3d mid = DPoint3d::From((ptfaceStart.x + ptfaceEnd.x) / 2, (ptfaceStart.y + ptfaceEnd.y) / 2, (ptfaceStart.z + ptfaceEnd.z) / 2);

					Dpoint3d ptfaceStartDown, ptfaceEndDown;
					mdlElmdscr_computeRange(&ptfaceStartDown, &ptfaceEndDown, downface.GetElementDescrCP(), nullptr);
					Dpoint3d middown = DPoint3d::From((ptfaceStartDown.x + ptfaceEndDown.x) / 2, (ptfaceStartDown.y + ptfaceEndDown.y) / 2, (ptfaceStartDown.z + ptfaceEndDown.z) / 2);

					Dpoint3d ptfaceStartUp, ptfaceEndUp;
					mdlElmdscr_computeRange(&ptfaceStartUp, &ptfaceEndUp, upface.GetElementDescrCP(), nullptr);
					Dpoint3d midup = DPoint3d::From((ptfaceStartUp.x + ptfaceEndUp.x) / 2, (ptfaceStartUp.y + ptfaceEndUp.y) / 2, (ptfaceStartUp.z + ptfaceEndUp.z) / 2);

					bool bIsin = false;//内外面，确定弯钩
					double angel = 0;

					DVec3d VecAngle = GetfaceNormal();//DVec3d::FromCrossProduct(vec2, vecfacenor);
					if (ptmid.z > mid.z) //上面
					{
						double thod = GetfaceNormal().AngleTo(VecAngle);
						angel = thod / PI * 180;
						/*if (mid.z - middown.z > 10)
						{
							bIsin = true;
						}
						else
						{
							bIsin = false;
						}
						if (VecAngle.z < 0)
						{
							VecAngle.Negate();
						}*/
					}
					else //下面
					{
						//VecAngle.Negate();

						if (ptmid.z - middown.z > 10)
						{
							bIsin = true;
							VecAngle.Negate();
						}
						else
						{
							bIsin = false;
							/*if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, false, const_cast<MSElementDescrP>(m_face.GetElementDescrCP())))
							{
								return;
							}*/
						}
						double thod = GetfaceNormal().AngleTo(VecAngle);
						angel = thod / PI * 180;
						//if (VecAngle.z > 0)
						//{
						//	VecAngle.Negate();
						//}
					}

					std::vector<EditElementHandle*> allFaces;
					std::vector<EditElementHandle*> allParalFaces;
					ExtractFacesTool::GetFaces(copyEleeh2, allFaces);
					for (size_t index = 0; index < allFaces.size(); index++)
					{
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allFaces[index]->GetElementDescrP(), &ptDefault);
						if (abs(vecface.DotProduct(vecfacenor)) > 0.9)
						{
							allParalFaces.push_back(allFaces[index]);
						}
					}

					double distance = 0.0;
					for (size_t index = 0; index < allParalFaces.size(); index++)
					{

						Dpoint3d ptfacemin, ptfacemax;
						mdlElmdscr_computeRange(&ptfacemin, &ptfacemax, allParalFaces[index]->GetElementDescrCP(), nullptr);
						Dpoint3d ptfaceMid = Dpoint3d::From((ptfacemin.x + ptfacemax.x) / 2, (ptfacemin.y + ptfacemax.y) / 2, (ptfacemin.z + ptfacemax.z) / 2);
						DVec3d vecface;
						mdlElmdscr_extractNormal(&vecface, nullptr, allParalFaces[index]->GetElementDescrP(), &ptDefault);
						Dpoint3d ptproject;
						mdlVec_projectPointToPlane(&ptproject, &ptmid, &ptfaceMid, &vecface);

						/*ISolidKernelEntityPtr entityPtr;
						if (SolidUtil::Convert::ElementToBody(entityPtr, *allParalFaces[index]) == SUCCESS)
						{
							if (!SolidUtil::IsPointInsideBody(*entityPtr, ptproject))
							{
								continue;
							}
						}
						double ditanceBetweenFace = ptmid.Distance(ptproject);
						if (ditanceBetweenFace < 1)
						{
							continue;
						}
						else if (distance < 1 || ditanceBetweenFace < distance)
						{
							distance = ditanceBetweenFace;
						}*/
						//斜面求得ptmid不在面上，导致后续求得点都不在面上，修改判断方式，直接以Z小于ptmid.z且distance最大得面为钢筋贴近面					
						if (ptmid.z - ptproject.z < 10)
						{
							continue;
						}
						double ditanceBetweenFace = ptmid.Distance(ptproject);
						if (ditanceBetweenFace < 1)
						{
							continue;
						}
						else if (distance < 1 || ditanceBetweenFace > distance)
						{
							distance = ditanceBetweenFace;
						}
					}
					double dAnchorLeng = la0;
					if (!bIsin)
					{
						dAnchorLeng = lae;
					}

					if (ptmid.Distance(ptStart) < ptmid.Distance(ptEnd))
					{
						m_vecEndNormalStart = VecAngle;
						m_bStartAnhorselslantedFace = true;
						m_verSlabFaceInfo.strtype.endType = 8;
						m_verSlabFaceInfo.strtype.rotateAngle = angel;
						m_verSlabFaceInfo.strtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.strtype.endPtInfo.value3 = dAnchorLeng;
						//外侧面y方向的面向板的方向锚入
						DVec3d yDir = DVec3d::From(0, 1, 0);
						if (fabs(yDir.DotProduct(GetfaceNormal())) > 0.9 && isYdir && (fabs(range_eeh.ZLength() - range_CurFace.ZLength()) < 300) && flag && (m_CatchpitType == 1))
						{
							m_vecEndNormalStart.Negate();
							m_verSlabFaceInfo.strtype.endPtInfo.value3 = la0;
						}
						//特殊集水坑左边的墙体外侧面往左边板锚入
						DVec3d xDir = DVec3d::From(1, 0, 0);
						if (fabs(xDir.DotProduct(GetfaceNormal())) > 0.9 && isYdir && (COMPARE_VALUES_EPS(range_eeh.low.x, range_CurFace.low.x, 300) == 0) && flag && m_CatchpitType == 1)
						{
							m_vecEndNormalStart.Negate();
							m_verSlabFaceInfo.strtype.endPtInfo.value3 = la0;
						}

						if (bIsin)
						{
							m_twoFacesDistance = distance;
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;
						}
						else
						{
							m_verSlabFaceInfo.bStartAnhorsel = true;
							m_verSlabFaceInfo.dStartanchoroffset = m_verSlabFaceInfo.dStartanchoroffset - dSideCoverConcrete - bendradius / 2;
						}
					}
					else
					{
						m_vecEndNormalEnd = VecAngle;
						m_bEndAnhorselslantedFace = true;
						m_verSlabFaceInfo.endtype.endType = 8;
						m_verSlabFaceInfo.endtype.rotateAngle = angel;
						m_verSlabFaceInfo.endtype.endPtInfo.value1 = bendradius;
						m_verSlabFaceInfo.endtype.endPtInfo.value3 = dAnchorLeng;
						//外侧面y方向的面向板的方向锚入
						DVec3d yDir = DVec3d::From(0, 1, 0);
						if (fabs(yDir.DotProduct(GetfaceNormal())) > 0.9 && isYdir && (fabs(range_eeh.ZLength() - range_CurFace.ZLength()) < 300) && !flag && (m_CatchpitType == 1))
						{
							m_vecEndNormalEnd.Negate();
							m_verSlabFaceInfo.endtype.endPtInfo.value3 = la0;
						}
						//特殊集水坑左边的墙体外侧面往左边板锚入
						DVec3d xDir = DVec3d::From(1, 0, 0);
						if (fabs(xDir.DotProduct(GetfaceNormal())) > 0.9 && isYdir && (COMPARE_VALUES_EPS(range_eeh.low.x, range_CurFace.low.x, 300) == 0) && !flag && m_CatchpitType == 1)
						{
							m_vecEndNormalEnd.Negate();
							m_verSlabFaceInfo.endtype.endPtInfo.value3 = la0;
						}
						if (bIsin)
						{
							m_twoFacesDistance = distance;
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = distance - dSideCoverConcrete - bendradius / 2 - diameter * 2;

						}
						else
						{
							m_verSlabFaceInfo.bEndAnhorsel = true;
							m_verSlabFaceInfo.dEndanchoroffset = m_verSlabFaceInfo.dEndanchoroffset - dSideCoverConcrete - bendradius / 2;
						}
					}

				}
			}

		}

	}


}

// 处理钢筋对于Z型板的钢筋分区。如果本面更长则进行分区，无论长短都需要在拐角增加平面
bool PlaneRebarAssembly::CalculateZCorner(map<int, int>& sectionPairs, vector<MSElementDescrP>& parafaces, DPoint3d& minP, DPoint3d& maxP, bool isXDir)
{
	DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
	DVec3d vecfacenor;
	mdlElmdscr_extractNormal(&vecfacenor, nullptr, m_Solid->GetElementDescrP(), &ptDefault);
	std::vector<EditElementHandle*> allFaces;//该元素所有的面
	std::vector<EditElementHandle*> allParalFaces;//与所选面平行的所有面
	ExtractFacesTool::GetFaces(*m_Solid, allFaces);
	for (size_t index = 0; index < allFaces.size(); index++)
	{
		DVec3d vecface;
		mdlElmdscr_extractNormal(&vecface, nullptr, allFaces[index]->GetElementDescrP(), &ptDefault);
		if (abs(vecface.DotProduct(vecfacenor)) > 0.9)
		{
			allParalFaces.push_back(allFaces[index]);
		}
	}

	double eps = 100 * UOR_PER_MilliMeter;//同一面左右高度可能有较大偏差
	DPoint3d ptStart = isXDir ? m_LineSeg1.GetLineStartPoint() : m_LineSeg2.GetLineStartPoint();
	DPoint3d ptEnd = isXDir ? m_LineSeg1.GetLineEndPoint() : m_LineSeg2.GetLineEndPoint();
	double start = isXDir ? ptStart.x : ptStart.y;
	double end = isXDir ? ptEnd.x : ptEnd.y;
	for (size_t index = 0; index < allParalFaces.size(); index++)
	{
		Dpoint3d ptFaceMin, ptFaceMax;
		mdlElmdscr_computeRange(&ptFaceMin, &ptFaceMax, allParalFaces[index]->GetElementDescrCP(), nullptr);
		double faceStart = isXDir ? ptFaceMin.x : ptFaceMin.y;
		double faceEnd = isXDir ? ptFaceMax.x : ptFaceMax.y;

		if (COMPARE_VALUES_EPS(abs(ptStart.z - ptFaceMin.z), m_slabThickness, eps) != 0)
			continue;
		if (COMPARE_VALUES_EPS(m_LineSeg1.GetLength(), abs(faceEnd - faceStart), eps) == 0)
			continue;
		DPoint3d pts[4] = { 0 };
		if (COMPARE_VALUES_EPS(faceStart - start, m_slabThickness, eps) == 0 && COMPARE_VALUES_EPS(faceEnd, end, eps) == 0) {
			// 左长，从对面构建平面，填充分区
			pts[0] = DPoint3d::From(ptStart.x, ptStart.y, ptFaceMin.z);
			pts[1] = DPoint3d::From(ptFaceMin.x, ptStart.y, ptFaceMin.z);
			pts[2] = DPoint3d::From(ptFaceMin.x, ptFaceMax.y, ptFaceMin.z);
			pts[3] = DPoint3d::From(ptStart.x, ptFaceMax.y, ptFaceMin.z);

			sectionPairs[0] = (int)(faceStart - start);
			sectionPairs[(int)(faceStart - start)] = 0;
		}
		else if (COMPARE_VALUES_EPS(start - faceStart, m_slabThickness, eps) == 0 && COMPARE_VALUES_EPS(faceEnd, end, eps) == 0) {
			// 左短，从自身构建平面，扩大分区范围
			pts[0] = DPoint3d::From(ptStart.x, ptStart.y, ptStart.z);
			pts[1] = DPoint3d::From(ptFaceMin.x, ptStart.y, ptStart.z);
			pts[2] = DPoint3d::From(ptFaceMin.x, ptFaceMax.y, ptStart.z);
			pts[3] = DPoint3d::From(ptStart.x, ptFaceMax.y, ptStart.z);

			if (isXDir)//XOZ平面需要使用x和z计算
				minP.x -= m_slabThickness;
			else
				minP.z -= m_slabThickness;
		}
		else if (COMPARE_VALUES_EPS(faceStart, start, eps) == 0 && COMPARE_VALUES_EPS(faceEnd - end, m_slabThickness, eps) == 0) {
			// 右短，从自身构建平面，扩大分区范围
			pts[0] = DPoint3d::From(ptEnd.x, ptEnd.y, ptEnd.z);
			pts[1] = DPoint3d::From(ptFaceMax.x, ptEnd.y, ptEnd.z);
			pts[2] = DPoint3d::From(ptFaceMax.x, ptFaceMax.y, ptEnd.z);
			pts[3] = DPoint3d::From(ptEnd.x, ptFaceMax.y, ptEnd.z);

			if (isXDir)
				maxP.x += m_slabThickness;
			else
				maxP.z += m_slabThickness;
		}
		else if (COMPARE_VALUES_EPS(faceStart, start, eps) == 0 && COMPARE_VALUES_EPS(end - faceEnd, m_slabThickness, eps) == 0) {
			// 右长，从对面构建平面，填充分区
			pts[0] = DPoint3d::From(ptFaceMax.x, ptEnd.y, ptFaceMax.z);
			pts[1] = DPoint3d::From(ptEnd.x, ptEnd.y, ptFaceMax.z);
			pts[2] = DPoint3d::From(ptEnd.x, ptFaceMax.y, ptFaceMax.z);
			pts[3] = DPoint3d::From(ptFaceMax.x, ptFaceMax.y, ptFaceMax.z);

			sectionPairs[(int)(faceStart - start)] = (int)(faceEnd - start);
			sectionPairs[(int)(faceEnd - start)] = 0;
		}
		if (pts[0].IsEqual(DPoint3d::From(0, 0, 0)))
			continue;

		EditElementHandle eehZCorner;
		ShapeHandler::CreateShapeElement(eehZCorner, NULL, pts, 4, true, *ACTIVEMODEL);
		if (!m_zCorner)
			m_zCorner = new EditElementHandle();
		m_zCorner->Duplicate(eehZCorner);
		parafaces.push_back(eehZCorner.GetElementDescrP());
		break;
	}
	return true;
}

void PlaneRebarAssembly::CalculateInSideData(MSElementDescrP face/*当前配筋面*/, MSElementDescrP tmpupfaces[40], MSElementDescrP tmpdownfaces[40], int i, DVec3d rebarVec)
{
	m_insidef.ClearData();
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	/*WString strSize1 = GetvecDirSize().at(i);
	if (strSize1.find(L"mm") != WString::npos)
	{
		strSize1.ReplaceAll(L"mm", L"");
	}*/
	BrString strRebarSize(GetMainRebars().at(i).rebarSize);
	//double	diameter = RebarCode::GetBarDiameter(strSize1, ACTIVEMODEL);		//乘以了10
	double diameter = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL);
	//double	bendradius = RebarCode::GetPinRadius(strSize1, ACTIVEMODEL, false);		//乘以了10
	double	bendradius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);		//乘以了10
	double   diameterPre = 0;
	double la_d = stod(GetMainRebars().at(i).rebarSize) * uor_per_mm;
	//double La0 = diameter * 15;//先为15d,如果判断外侧则修改
	double La0 = la_d * 15;//先为15d,如果判断外侧则修改s

	if (i - 1 >= 0)
	{
		BrString strRebarSize1(GetMainRebars().at(i - 1).rebarSize);
		diameterPre = RebarCode::GetBarDiameter(strRebarSize1, ACTIVEMODEL);
	}
	else if (i == 0)
	{
		BrString strRebarSize1(GetMainRebars().at(i + 1).rebarSize);
		diameterPre = RebarCode::GetBarDiameter(strRebarSize1, ACTIVEMODEL);
	}

	//将所有面转换到XOZ平面
	CVector3D ORIPT = GetPlacement().GetTranslation();
	// PITCommonTool::CPointTool::DrowOnePoint(ORIPT, 1, 2);
	CMatrix3D tmpmat = GetPlacement();
	Transform trans;
	tmpmat.AssignTo(trans);
	trans.InverseOf(trans);
	TransformInfo transinfo(trans);

	DPoint3d ptcenter = DPoint3d::From(0, 0, 0);
	DPoint3d facenormal = DPoint3d::From(0, 1, 0);
	Transform tran;
	mdlTMatrix_computeFlattenTransform(&tran, &ptcenter, &facenormal);
	TransformInfo tinfo(tran);
	//mdlElmdscr_add(face);
	EditElementHandle faceeeh(face, false, false, ACTIVEMODEL);

	faceeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(faceeeh, transinfo);
	faceeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(faceeeh, tinfo);

	//mdlElmdscr_add(faceeeh.GetElementDescrP());
	DPoint3d minP, maxP;
	//计算指定元素描述符中元素的范围。
	mdlElmdscr_computeRange(&minP, &maxP, faceeeh.GetElementDescrP(), NULL);
	//faceeeh.AddToModel();
	DPoint3d midpos = minP;
	midpos.Add(maxP);
	midpos.Scale(0.5);
	//mdlElmdscr_add(faceeeh.GetElementDescrP());
	vector<MSElementDescrP> tmpdescrs;
	for (int i = 0; i < m_ldfoordata.upnum; i++)
	{
		EditElementHandle tmpeeh(tmpupfaces[i], true, false, ACTIVEMODEL);
		tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, transinfo);
		tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, tinfo);
		tmpupfaces[i] = tmpeeh.ExtractElementDescr();
		tmpdescrs.push_back(tmpupfaces[i]);
		//mdlElmdscr_add(tmpupfaces[i]);
	}
	for (int i = 0; i < m_ldfoordata.downnum; i++)
	{
		EditElementHandle tmpeeh(tmpdownfaces[i], true, false, ACTIVEMODEL);
		tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, transinfo);
		tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, tinfo);
		tmpdownfaces[i] = tmpeeh.ExtractElementDescr();
		tmpdescrs.push_back(tmpdownfaces[i]);
		//mdlElmdscr_add(tmpdownfaces[i]);
	}

	vector<MSElementDescrP> tmpAnchordescrs;
	for (ISubEntityPtr face1 : m_anchorFaces)
	{
		EditElementHandle eehFace;
		if (!PIT::ConvertToElement::SubEntityToElement(eehFace, face1, ACTIVEMODEL))
		{

			continue;
		}
		eehFace.GetElementDescrP();
		MSElementDescrP TempEdp = eehFace.ExtractElementDescr();
		//mdlElmdscr_add(TempEdp);
		tmpAnchordescrs.push_back(TempEdp);

	}

	if (tmpdescrs.size() == 0)
	{
		return;
	}

	DVec3d tmpVec = DVec3d::From(1, 0, 0);
	//m_outsidef.calLen = diameterPre;
	EditElementHandle eehline1;
	EditElementHandle eehline2;
	LineHandler::CreateLineElement(eehline1, NULL, m_LineSeg1.GetLineSeg(), ACTIVEMODEL->Is3d(), *ACTIVEMODEL);
	LineHandler::CreateLineElement(eehline2, NULL, m_LineSeg2.GetLineSeg(), ACTIVEMODEL->Is3d(), *ACTIVEMODEL);

	eehline1.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline1, transinfo);
	eehline1.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline1, tinfo);
	//eehline1.AddToModel();
	DPoint3d pt1, pt2;
	mdlElmdscr_extractEndPoints(&pt1, nullptr, &pt2, nullptr, eehline1.GetElementDescrP(), ACTIVEMODEL);
	DVec3d vec1 = pt2 - pt1;

	eehline2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline2, transinfo);
	eehline2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline2, tinfo);
	//eehline2.AddToModel();
	DPoint3d pt3, pt4;
	mdlElmdscr_extractEndPoints(&pt3, nullptr, &pt4, nullptr, eehline2.GetElementDescrP(), ACTIVEMODEL);
	DVec3d vec2 = pt4 - pt3;

	if (GetMainRebars().at(i).rebarDir == 1)	//纵向钢筋,局部坐标系Z方向
	{
		tmpVec = DVec3d::From(0, 0, 1);
		//中间位置钢筋线,计算两端是否有垂直钢筋
		DPoint3d midstr = midpos;
		DPoint3d midend = midstr;
		if (vec2.DotProduct(tmpVec) > 0.9)
		{
			midstr.z = midpos.z - m_ldfoordata.Ylenth / 2 + GetConcrete().sideCover*uor_per_mm;
			midend.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetConcrete().sideCover*uor_per_mm;
		}
		else
		{
			midstr.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetConcrete().sideCover*uor_per_mm;
			midend.z = midpos.z - m_ldfoordata.Ylenth / 2 + GetConcrete().sideCover*uor_per_mm;
		}

		int verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size());
		switch (verRe)
		{
		case 0:
		{
			m_insidef.Verstr = false;
			m_insidef.Verend = false;
			break;
		}
		case 1:
		{
			m_insidef.Verstr = true;
			m_insidef.Verend = false;
			break;
		}
		case 2:
		{
			m_insidef.Verstr = false;
			m_insidef.Verend = true;
			break;
		}
		case 3:
		{
			m_insidef.Verstr = true;
			m_insidef.Verend = true;
			break;
		}
		}
		if (tmpdescrs.size() == 0)//上下都没有墙时
		{
			m_sidetype = SideType::Nor;
		}
		if (m_ldfoordata.downnum > 0)//顶部内侧面
		{
			if (m_insidef.Verstr || m_insidef.Verend)
			{
				//if (i == 1)//内侧钢筋
				//{
				//	m_insidef.calLen = diameter;
				//}
				//else if (i == 0)//外侧钢筋
				//{
				//	m_insidef.calLen = diameter * 2;
				//}

				double angel = 0;
				double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, -1));
				angel = thod / PI * 180;
				m_insidef.calLen = diameter * (i + 1);
				//m_insidef.calLen = WallRebars_OffsetLength()
				if (m_insidef.Verstr)
				{
					m_insidef.strtype.endType = 4;
					m_insidef.strtype.rotateAngle = angel;//-90;
					m_insidef.strtype.endPtInfo.value1 = bendradius;
					m_insidef.strtype.endPtInfo.value3 = La0;
				}
				if (m_insidef.Verend)
				{
					m_insidef.endtype.endType = 4;
					m_insidef.endtype.rotateAngle = angel;//-90;
					m_insidef.endtype.endPtInfo.value1 = bendradius;
					m_insidef.endtype.endPtInfo.value3 = La0;
				}

			}

		}
		else if (m_ldfoordata.upnum > 0)//底部内侧面
		{
			if (m_insidef.Verstr || m_insidef.Verend)
			{
				//if (i == 2)//内侧钢筋
				//{
				//	m_insidef.calLen = diameter;
				//}
				//else if (i == 3)//外侧钢筋
				//{
				//	m_insidef.calLen = diameter * 2;
				//}
				m_insidef.calLen = diameter * (i + 1);

				double angel = 0;
				double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, 1));
				angel = thod / PI * 180;
				if (m_insidef.Verstr)
				{
					m_insidef.strtype.endType = 4;
					m_insidef.strtype.rotateAngle = angel;//90;
					m_insidef.strtype.endPtInfo.value1 = bendradius;
					m_insidef.strtype.endPtInfo.value3 = La0;
				}
				if (m_insidef.Verend)
				{
					m_insidef.endtype.endType = 4;
					m_insidef.endtype.rotateAngle = angel;//90;
					m_insidef.endtype.endPtInfo.value1 = bendradius;
					m_insidef.endtype.endPtInfo.value3 = La0;
				}


			}
		}
		else//上下都没有墙，正常面处理
		{

			m_sidetype = SideType::Nor;
		}

		//看是否需要锚入自身面
		double LaE = PlaneRebarAssembly::GetLae();
		if (LaE > 0)
		{
			LaE *= stod(GetMainRebars().at(i).rebarSize);
		}
		else
		{
			LaE = 15 * diameter;
		}
		CreateAnchorBySelf(tmpAnchordescrs, m_LineSeg2, bendradius, La0, LaE, diameter, diameterPre, i);


		//计算配筋区间值,平行墙处理
		map<int, int>  sectionPairs;
		double offset = 0;
		if (COMPARE_VALUES_EPS(minP.x, 0, 50) == 0)
		{
			offset += minP.x;
			maxP.x -= offset;
			minP.x -= offset;
		}
		sectionPairs[(int)minP.x] = (int)maxP.x;//大面区间
		sectionPairs[(int)maxP.x] = 0;//大面区间

		vector<MSElementDescrP> parafaces;//平行墙
		for (int i = 0; i < tmpdescrs.size(); i++)
		{
			/*过滤一些墙*/
			DRange3d faceRange;
			mdlElmdscr_computeRange(&faceRange.low, &faceRange.high, tmpdescrs[i], NULL);
			if (faceRange.XLength() < 0.8 * m_ldfoordata.Xlenth && faceRange.ZLength() < 0.8 * m_ldfoordata.Ylenth)
			{
				if (faceRange.XLength() > faceRange.ZLength())//横墙
				{
					if (faceRange.low.z < 2 * uor_per_mm || abs(faceRange.high.z - m_ldfoordata.Ylenth) < 2 * uor_per_mm)
						;
					else
						continue;
				}
				else//竖墙
				{
					if (faceRange.low.x < 2 * uor_per_mm || abs(faceRange.high.x - m_ldfoordata.Xlenth) < 2 * uor_per_mm)
						;
					else
						continue;
				}
			}
			/*过滤一些墙*/
			DPoint3d tmpstr, tmpend;
			tmpstr = tmpend = DPoint3d::From(0, 0, 0);
			PITCommonTool::CElementTool::GetLongestLineMidPt(tmpdescrs[i], tmpstr, tmpend);
			DVec3d vectmp = tmpend - tmpstr;
			vectmp.Normalize();
			if (abs(vectmp.DotProduct(tmpVec)) > 0.9)
			{
				DPoint3d tminP, tmaxP;
				//计算指定元素描述符中元素的范围。
				mdlElmdscr_computeRange(&tminP, &tmaxP, tmpdescrs[i], NULL);
				if (COMPARE_VALUES_EPS(tminP.x, 0, 50) == 0)
				{
					offset += tminP.x;
				}
				tmaxP.x -= offset;
				tminP.x -= offset;
				if (COMPARE_VALUES_EPS(tminP.x, 0, 50) < 0)
				{
					tminP.x = 0;
				}
				if (COMPARE_VALUES_EPS(tmaxP.x, maxP.x, 50) > 0)
				{
					tmaxP.x = maxP.x;
				}
				sectionPairs[(int)tminP.x] = (int)tmaxP.x;
				sectionPairs[(int)tmaxP.x] = 0;
				parafaces.push_back(tmpdescrs[i]);
			}
		}

		CalculateZCorner(sectionPairs, parafaces, minP, maxP);

		map<int, int>::iterator itr = sectionPairs.begin();
		for (; itr != sectionPairs.end(); itr++)
		{
			map<int, int>::iterator itrnex = itr;
			itrnex++;
			if (itrnex == sectionPairs.end())
			{
				break;
			}
			if (itrnex->first - itr->first < 3)
			{
				continue;
			}
			int midpos = (itr->first + itrnex->first) / 2;
			//判断中心点坐标是否在平行墙中，如果在跳过（不用配钢筋），不在的话，判断左右两端是否为大面的边坐标
			//如果是，不用配置边上钢筋，不是需要配置边钢筋
			bool isInWall = false;
			for (int j = 0; j < parafaces.size(); j++)
			{
				DPoint3d tminP, tmaxP;
				//计算指定元素描述符中元素的范围。
				mdlElmdscr_computeRange(&tminP, &tmaxP, parafaces[j], NULL);
				if (midpos > (tminP.x - 1.0) && midpos < (tmaxP.x + 1.0))
				{
					isInWall = true;
					break;
				}
			}
			if (!isInWall)//如果不在墙内
			{
				m_insidef.pos[m_insidef.posnum].str = itr->first;
				m_insidef.pos[m_insidef.posnum].end = itrnex->first;
				if (abs(itr->first - minP.x) > 100 && parafaces.size() > 0)//不是起始区间值
				{
					m_insidef.pos[m_insidef.posnum].addstr = true;
					if (i == 1 || i == 2)//非外侧钢筋
					{
						m_insidef.pos[m_insidef.posnum].strval = GetConcrete().sideCover*uor_per_mm * 2 + 2 * diameter;
					}
					else
					{
						m_insidef.pos[m_insidef.posnum].strval = GetConcrete().sideCover*uor_per_mm * 2 + diameter;
					}
				}
				if (abs(itrnex->first - maxP.x) > 100 && parafaces.size() > 0)//不是终止区间值
				{
					m_insidef.pos[m_insidef.posnum].addend = true;
					if (i == 1 || i == 2)//非外侧钢筋
					{
						m_insidef.pos[m_insidef.posnum].endval = GetConcrete().sideCover*uor_per_mm * 2 + 2 * diameter;
					}
					else
					{
						m_insidef.pos[m_insidef.posnum].endval = GetConcrete().sideCover*uor_per_mm * 2 + diameter;
					}
				}
				m_insidef.posnum++;
			}
		}


	}
	else//横向钢筋X方向,横向钢筋起点和终点是相反的
	{
		tmpVec = DVec3d::From(1, 0, 0);
		//中间位置钢筋线,计算两端是否有垂直钢筋
		DPoint3d midstr = midpos;
		DPoint3d midend = midstr;
		if (vec1.DotProduct(tmpVec) > 0.9)
		{
			midstr.x = midpos.x - m_ldfoordata.Xlenth / 2 + GetConcrete().sideCover*uor_per_mm;

			midend.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetConcrete().sideCover*uor_per_mm;
		}
		else
		{
			midstr.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetConcrete().sideCover*uor_per_mm;
			midend.x = midpos.x - m_ldfoordata.Xlenth / 2 + GetConcrete().sideCover*uor_per_mm;
		}

		int verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size());
		switch (verRe)
		{
		case 0:
		{
			m_insidef.Verstr = false;
			m_insidef.Verend = false;
			break;
		}
		case 1:
		{
			m_insidef.Verstr = true;
			m_insidef.Verend = false;
			break;
		}
		case 2:
		{
			m_insidef.Verstr = false;
			m_insidef.Verend = true;
			break;
		}
		case 3:
		{
			m_insidef.Verstr = true;
			m_insidef.Verend = true;
			break;
		}
		}
		if (tmpdescrs.size() == 0)//上下都没有墙时
		{
			m_sidetype = SideType::Nor;
		}
		if (m_ldfoordata.downnum > 0)//顶部内侧面
		{
			//if (i == 1)//内侧钢筋
			//{
			//	m_insidef.calLen = diameter;
			//}
			//else if (i == 0)//外侧钢筋
			//{
			//	m_insidef.calLen = diameter * 2;
			//}
			m_insidef.calLen = diameter * 2;
			double angel = 0;
			double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, -1));
			angel = thod / PI * 180;
			if (m_insidef.Verstr)
			{
				m_insidef.strtype.endType = 4;
				m_insidef.strtype.rotateAngle = angel;//0;
				m_insidef.strtype.endPtInfo.value1 = bendradius;
				m_insidef.strtype.endPtInfo.value3 = La0;
			}
			if (m_insidef.Verend)
			{
				m_insidef.endtype.endType = 4;
				m_insidef.endtype.rotateAngle = angel;//0;
				m_insidef.endtype.endPtInfo.value1 = bendradius;
				m_insidef.endtype.endPtInfo.value3 = La0;
			}

		}
		else if (m_ldfoordata.upnum > 0)//底部内侧面
		{
			if (m_insidef.Verstr || m_insidef.Verend)
			{
				//if (i == 2)//内侧钢筋
				//{
				//	m_insidef.calLen = diameter;
				//}
				//else if (i == 3)//外侧钢筋
				//{
				//	m_insidef.calLen = diameter * 2;
				//}
				m_insidef.calLen = diameter * 2;
				double angel = 0;
				double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, 1));
				angel = thod / PI * 180;
				if (m_insidef.Verstr)
				{
					m_insidef.strtype.endType = 4;
					m_insidef.strtype.rotateAngle = angel;//180;
					m_insidef.strtype.endPtInfo.value1 = bendradius;
					m_insidef.strtype.endPtInfo.value3 = La0;
				}
				if (m_insidef.Verend)
				{
					m_insidef.endtype.endType = 4;
					m_insidef.endtype.rotateAngle = angel;// 180;
					m_insidef.endtype.endPtInfo.value1 = bendradius;
					m_insidef.endtype.endPtInfo.value3 = La0;
				}
			}
		}
		else//上下都没有墙，正常面处理
		{
			m_sidetype = SideType::Nor;
		}
		double LaE = PlaneRebarAssembly::GetLae();
		if (LaE > 0)
		{
			LaE *= stod(GetMainRebars().at(i).rebarSize);
		}
		else
		{
			LaE = 15 * diameter;
		}
		CreateAnchorBySelf(tmpAnchordescrs, m_LineSeg1, bendradius, La0, LaE, diameter, diameterPre, i);


		//计算配筋区间值,平行钢筋处理
		map<int, int>  sectionPairs;
		double offset = 0;
		if (COMPARE_VALUES_EPS(minP.z, 0, 50) == 0)
		{
			offset += minP.z;
			maxP.z -= offset;
			minP.z -= offset;
		}
		sectionPairs[(int)minP.z] = (int)maxP.z;//大面区间
		sectionPairs[(int)maxP.z] = 0;//大面区间


		vector<MSElementDescrP> parafaces;//平行墙
		for (int i = 0; i < tmpdescrs.size(); i++)
		{
			/*过滤一些墙*/
			DRange3d faceRange;
			mdlElmdscr_computeRange(&faceRange.low, &faceRange.high, tmpdescrs[i], NULL);
			if (faceRange.XLength() < 0.8 * m_ldfoordata.Xlenth && faceRange.ZLength() < 0.8 * m_ldfoordata.Ylenth)
			{
				if (faceRange.XLength() > faceRange.ZLength())//横墙
				{
					if (faceRange.low.z < 2 * uor_per_mm || abs(faceRange.high.z - m_ldfoordata.Ylenth) < 2 * uor_per_mm)
						;
					else
						continue;
				}
				else//竖墙
				{
					if (faceRange.low.x < 2 * uor_per_mm || abs(faceRange.high.x - m_ldfoordata.Xlenth) < 2 * uor_per_mm)
						;
					else
						continue;
				}

			}
			/*过滤一些墙*/
			DPoint3d tmpstr, tmpend;
			tmpstr = tmpend = DPoint3d::From(0, 0, 0);
			PITCommonTool::CElementTool::GetLongestLineMidPt(tmpdescrs[i], tmpstr, tmpend);
			DVec3d vectmp = tmpend - tmpstr;
			vectmp.Normalize();
			if (abs(vectmp.DotProduct(tmpVec)) > 0.9)
			{
				DPoint3d tminP, tmaxP;
				//计算指定元素描述符中元素的范围。
				mdlElmdscr_computeRange(&tminP, &tmaxP, tmpdescrs[i], NULL);
				//tmpqj[m_ldfoordata.Ylenth - (int)tminP.z] = m_ldfoordata.Ylenth - (int)tmaxP.z;
				//tmpqj[m_ldfoordata.Ylenth - (int)tmaxP.z] = 0;
				if (COMPARE_VALUES_EPS(tminP.z, 0, 50) == 0)
				{
					offset += tminP.z;
				}
				tmaxP.z -= offset;
				tminP.z -= offset;
				if (COMPARE_VALUES_EPS(tminP.z, 0, 50) < 0)
				{
					tminP.z = 0;
				}
				if (COMPARE_VALUES_EPS(tmaxP.z, maxP.z, 50) > 0)
				{
					tmaxP.z = maxP.z;
				}
				sectionPairs[(int)tminP.z] = (int)tmaxP.z;
				sectionPairs[(int)tmaxP.z] = 0;
				parafaces.push_back(tmpdescrs[i]);
			}
		}

		CalculateZCorner(sectionPairs, parafaces, minP, maxP, false);

		map<int, int>::iterator itr = sectionPairs.begin();
		for (; itr != sectionPairs.end(); itr++)
		{
			map<int, int>::iterator itrnex = itr;
			itrnex++;
			if (itrnex == sectionPairs.end())
			{
				break;
			}
			if (itrnex->first - itr->first < 3)
			{
				continue;
			}
			int midpos = (itr->first + itrnex->first) / 2;
			//判断中心点坐标是否在平行墙中，如果在跳过（不用配钢筋），不在的话，判断左右两端是否为大面的边坐标
			//如果是，不用配置边上钢筋，不是需要配置边钢筋
			bool isInWall = false;
			for (int j = 0; j < parafaces.size(); j++)
			{
				DPoint3d tminP, tmaxP;
				//计算指定元素描述符中元素的范围。
				mdlElmdscr_computeRange(&tminP, &tmaxP, parafaces[j], NULL);
				//if (midpos > (m_ldfoordata.Ylenth - tmaxP.z - 1.0) && midpos < (m_ldfoordata.Ylenth - tminP.z + 1.0))
				if (midpos < (tmaxP.z - 1.0) && midpos >(tminP.z + 1.0))
				{
					isInWall = true;
					break;
				}
			}
			if (!isInWall)//如果不在墙内
			{
				m_insidef.pos[m_insidef.posnum].str = itr->first;
				m_insidef.pos[m_insidef.posnum].end = itrnex->first;
				if (abs(itr->first - minP.z) > 100 && parafaces.size() > 0)//不是起始区间值
				{
					if (i == 1 || i == 2)//非外侧钢筋
					{
						m_insidef.pos[m_insidef.posnum].strval = GetConcrete().sideCover*uor_per_mm * 2 + 2 * diameter;
					}
					else
					{
						m_insidef.pos[m_insidef.posnum].strval = GetConcrete().sideCover*uor_per_mm * 2 + diameter;
					}
				}
				if (abs(itrnex->first - maxP.z) > 100 && parafaces.size() > 0)//不是终止区间值
				{
					if (i == 1 || i == 2)//非外侧钢筋
					{
						m_insidef.pos[m_insidef.posnum].endval = GetConcrete().sideCover*uor_per_mm * 2 + 2 * diameter;
					}
					else
					{
						m_insidef.pos[m_insidef.posnum].endval = GetConcrete().sideCover*uor_per_mm * 2 + diameter;
					}
				}
				m_insidef.posnum++;
			}
		}
	}
}

void PlaneRebarAssembly::CalculateOutSideData(MSElementDescrP face/*当前配筋面*/, MSElementDescrP tmpupfaces[40], MSElementDescrP tmpdownfaces[40], int i, DVec3d rebarVec)
{
	m_outsidef.ClearData();
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	//WString strSize1 = GetvecDirSize().at(i);
	//if (strSize1.find(L"mm") != WString::npos)
	//{
	//	strSize1.ReplaceAll(L"mm", L"");
	//}
	//double	diameter = RebarCode::GetBarDiameter(strSize1, ACTIVEMODEL);		//乘以了10
	//double	bendradius = RebarCode::GetPinRadius(strSize1, ACTIVEMODEL, false);		//乘以了10


	BrString strRebarSize(GetMainRebars().at(i).rebarSize);

	double diameter = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL);

	double	bendradius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);


	double   diameterPre = 0;
	//double LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
	double LaE = PlaneRebarAssembly::GetLae();
	if (LaE > 0)
	{
		LaE *= stod(GetMainRebars().at(i).rebarSize);
	}
	else
	{
		LaE = 15 * diameter;
	}
	if (i - 1 >= 0)
	{
		BrString strRebarSize1(GetMainRebars().at(i - 1).rebarSize);
		diameterPre = RebarCode::GetBarDiameter(strRebarSize1, ACTIVEMODEL);
	}
	else if (i == 0)
	{
		BrString strRebarSize1(GetMainRebars().at(i + 1).rebarSize);
		diameterPre = RebarCode::GetBarDiameter(strRebarSize1, ACTIVEMODEL);
	}

	//将所有面转换到XOZ平面
	CVector3D ORIPT = GetPlacement().GetTranslation();
	// PITCommonTool::CPointTool::DrowOnePoint(ORIPT, 1, 5);
	CMatrix3D tmpmat = GetPlacement();
	Transform trans;
	tmpmat.AssignTo(trans);
	trans.InverseOf(trans);
	TransformInfo transinfo(trans);

	DPoint3d ptcenter = DPoint3d::From(0, 0, 0);
	DPoint3d facenormal = DPoint3d::From(0, 1, 0);
	Transform tran;
	mdlTMatrix_computeFlattenTransform(&tran, &ptcenter, &facenormal);
	TransformInfo tinfo(tran);

	EditElementHandle faceeeh(face, false, false, ACTIVEMODEL);
	faceeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(faceeeh, transinfo);
	faceeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(faceeeh, tinfo);
	DPoint3d minP, maxP;
	//计算指定元素描述符中元素的范围。
	mdlElmdscr_computeRange(&minP, &maxP, faceeeh.GetElementDescrP(), NULL);
	DPoint3d midpos = minP;
	midpos.Add(maxP);
	midpos.Scale(0.5);
	//mdlElmdscr_add(faceeeh.GetElementDescrP());


	vector<MSElementDescrP> tmpdescrs;
	for (int i = 0; i < m_ldfoordata.upnum; i++)
	{
		EditElementHandle tmpeeh(tmpupfaces[i], true, false, ACTIVEMODEL);
		tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, transinfo);
		tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, tinfo);
		tmpupfaces[i] = tmpeeh.ExtractElementDescr();
		tmpdescrs.push_back(tmpupfaces[i]);
		//mdlElmdscr_add(tmpupfaces[i]);
	}
	for (int i = 0; i < m_ldfoordata.downnum; i++)
	{
		EditElementHandle tmpeeh(tmpdownfaces[i], true, false, ACTIVEMODEL);
		tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, transinfo);
		tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, tinfo);
		tmpdownfaces[i] = tmpeeh.ExtractElementDescr();
		tmpdescrs.push_back(tmpdownfaces[i]);
		//mdlElmdscr_add(tmpdownfaces[i]);
	}

	vector<MSElementDescrP> tmpAnchordescrs;
	for (ISubEntityPtr face1 : m_anchorFaces)
	{
		EditElementHandle eehFace;
		if (!PIT::ConvertToElement::SubEntityToElement(eehFace, face1, ACTIVEMODEL))
		{

			continue;
		}
		eehFace.GetElementDescrP();
		MSElementDescrP TempEdp = eehFace.ExtractElementDescr();
		//mdlElmdscr_add(TempEdp);
		tmpAnchordescrs.push_back(TempEdp);

	}

	if (tmpdescrs.size() == 0)
	{
		return;
	}

	EditElementHandle eehline1;
	EditElementHandle eehline2;
	LineHandler::CreateLineElement(eehline1, NULL, m_LineSeg1.GetLineSeg(), ACTIVEMODEL->Is3d(), *ACTIVEMODEL);
	LineHandler::CreateLineElement(eehline2, NULL, m_LineSeg2.GetLineSeg(), ACTIVEMODEL->Is3d(), *ACTIVEMODEL);

	eehline1.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline1, transinfo);
	eehline1.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline1, tinfo);

	DPoint3d pt1, pt2;
	mdlElmdscr_extractEndPoints(&pt1, nullptr, &pt2, nullptr, eehline1.GetElementDescrP(), ACTIVEMODEL);
	DVec3d vec1 = pt2 - pt1;
	vec1.Normalize();
	eehline2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline2, transinfo);
	eehline2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehline2, tinfo);
	DPoint3d pt3, pt4;
	mdlElmdscr_extractEndPoints(&pt3, nullptr, &pt4, nullptr, eehline2.GetElementDescrP(), ACTIVEMODEL);
	DVec3d vec2 = pt4 - pt3;
	vec2.Normalize();
	//m_outsidef.calLen = diameterPre;
	if (GetMainRebars().at(i).rebarDir == 1)	//纵向钢筋,局部坐标系Z方向
	{
		//中间位置钢筋线,计算两端是否有垂直钢筋
		DVec3d vecTemp = DVec3d::From(0, 0, 1);
		DPoint3d midstr = midpos;
		DPoint3d midend = midstr;
		if (vec2.DotProduct(vecTemp) > 0.9)
		{
			midstr.z = midpos.z - m_ldfoordata.Ylenth / 2 + GetConcrete().sideCover*uor_per_mm;
			midend.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetConcrete().sideCover*uor_per_mm;
		}
		else
		{
			midstr.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetConcrete().sideCover*uor_per_mm;
			midend.z = midpos.z - m_ldfoordata.Ylenth / 2 + GetConcrete().sideCover*uor_per_mm;
		}

		int verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size());
		switch (verRe)
		{
		case 0:
		{
			m_outsidef.Verstr = false;
			m_outsidef.Verend = false;
			break;
		}
		case 1:
		{
			m_outsidef.Verstr = true;
			m_outsidef.Verend = false;
			break;
		}
		case 2:
		{
			m_outsidef.Verstr = false;
			m_outsidef.Verend = true;
			break;
		}
		case 3:
		{
			m_outsidef.Verstr = true;
			m_outsidef.Verend = true;
			break;
		}
		}

		//起始位置钢筋线，计算起始位置是否有平行墙
		DPoint3d firstr = midpos;
		if (vec1.DotProduct(DVec3d::From(1, 0, 0)) > 0.9)
		{
			firstr.x = firstr.x - m_ldfoordata.Xlenth / 2 + GetConcrete().sideCover*uor_per_mm;
		}
		else
		{
			firstr.x = firstr.x + m_ldfoordata.Xlenth / 2 - GetConcrete().sideCover*uor_per_mm;
		}

		firstr.z = midstr.z;
		DPoint3d firend = firstr;
		firend.z = midend.z;
		if (IsHaveParaWall(firstr, firend, &tmpdescrs.at(0), tmpdescrs.size()))
		{
			m_outsidef.isdelstr = true;
		}
		EditElementHandle eehstr;
		LineHandler::CreateLineElement(eehstr, nullptr, DSegment3d::From(firstr, firend), true, *ACTIVEMODEL);
		//eehstr.AddToModel();
		//终止位置钢筋线，计算终止位置是否有平行钢筋
		DPoint3d secstr = midpos;
		if (vec1.DotProduct(DVec3d::From(1, 0, 0)) > 0.9)
		{
			secstr.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetConcrete().sideCover*uor_per_mm;
		}
		else
		{
			secstr.x = midpos.x - m_ldfoordata.Xlenth / 2 + GetConcrete().sideCover*uor_per_mm;
		}
		secstr.z = midstr.z;
		DPoint3d secend = secstr;
		secend.z = midend.z;
		if (IsHaveParaWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size()))
		{
			m_outsidef.isdelend = true;
		}
		if (m_ldfoordata.downnum > 0)//顶部外侧面
		{
			// if (i > 0 && (m_outsidef.Verstr || m_outsidef.Verend))//如果前面还有钢筋层，且有和钢筋线垂直的墙，钢筋长度左右都减少一个钢筋直径
			// {
			// 	m_outsidef.calLen = diameterPre;
			// }
			double angel = 0;
			double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, -1));
			angel = thod / PI * 180;
			if (m_outsidef.Verstr)
			{
				m_outsidef.strtype.endType = 4;
				m_outsidef.strtype.rotateAngle = angel;//-90;
				m_outsidef.strtype.endPtInfo.value1 = bendradius;
				m_outsidef.strtype.endPtInfo.value3 = LaE;
			}
			if (m_outsidef.Verend)
			{
				m_outsidef.endtype.endType = 4;
				m_outsidef.endtype.rotateAngle = angel;//-90;
				m_outsidef.endtype.endPtInfo.value1 = bendradius;
				m_outsidef.endtype.endPtInfo.value3 = LaE;
			}
		}
		else if (m_ldfoordata.upnum > 0)//底部外侧面
		{
			// if (i > 0 && (m_outsidef.Verstr || m_outsidef.Verend))//如果前面还有钢筋层，且有和钢筋线垂直的墙，钢筋长度左右都减少一个钢筋直径
			// {
			// 	m_outsidef.calLen = diameterPre;
			// }
			double angel = 0;
			double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, 1));
			angel = thod / PI * 180;
			if (m_outsidef.Verstr)
			{
				m_outsidef.strtype.endType = 4;
				m_outsidef.strtype.rotateAngle = angel;//90;
				m_outsidef.strtype.endPtInfo.value1 = bendradius;
				m_outsidef.strtype.endPtInfo.value3 = LaE;
			}
			if (m_outsidef.Verend)
			{
				m_outsidef.endtype.endType = 4;
				m_outsidef.endtype.rotateAngle = angel;//90;
				m_outsidef.endtype.endPtInfo.value1 = bendradius;
				m_outsidef.endtype.endPtInfo.value3 = LaE;
			}
		}
		else//上下都没有墙，正常面处理
		{
			m_sidetype = SideType::Nor;
		}
		double la0 = 15 * stod(GetMainRebars().at(i).rebarSize) * uor_per_mm;
		CreateAnchorBySelf(tmpAnchordescrs, m_LineSeg2, bendradius, la0/*15 * diameter*/, LaE, diameter, diameterPre, i, false);

		//计算配筋区间值,平行墙处理
		map<int, int>  sectionPairs;
		double offset = 0;
		if (COMPARE_VALUES_EPS(minP.x, 0, 50) == 0)
		{
			offset += minP.x;
			maxP.x -= offset;
			minP.x -= offset;
		}
		sectionPairs[(int)minP.x] = (int)maxP.x;//大面区间
		sectionPairs[(int)maxP.x] = 0;//大面区间
		vector<MSElementDescrP> parafaces;//平行墙
		for (int i = 0; i < tmpdescrs.size(); i++)
		{
			/*过滤一些墙*/
			DRange3d faceRange;
			mdlElmdscr_computeRange(&faceRange.low, &faceRange.high, tmpdescrs[i], NULL);
			if (faceRange.XLength() < 0.8 * m_ldfoordata.Xlenth && faceRange.ZLength() < 0.8 * m_ldfoordata.Ylenth)
			{
				if (faceRange.XLength() > faceRange.ZLength())//横墙
				{
					if (faceRange.low.z < 2 * uor_per_mm || abs(faceRange.high.z - m_ldfoordata.Ylenth) < 2 * uor_per_mm)
						;
					else
						continue;
				}
				else//竖墙
				{
					if (faceRange.low.x < 2 * uor_per_mm || abs(faceRange.high.x - m_ldfoordata.Xlenth) < 2 * uor_per_mm)
						;
					else
						continue;
				}
			}
			/*过滤一些墙*/
			DPoint3d tmpstr, tmpend;
			tmpstr = tmpend = DPoint3d::From(0, 0, 0);
			PITCommonTool::CElementTool::GetLongestLineMidPt(tmpdescrs[i], tmpstr, tmpend);
			DVec3d vectmp = tmpend - tmpstr;
			vectmp.Normalize();
			if (abs(vectmp.DotProduct(vecTemp)) > 0.9)
			{
				DPoint3d tminP, tmaxP;
				//计算指定元素描述符中元素的范围。
				mdlElmdscr_computeRange(&tminP, &tmaxP, tmpdescrs[i], NULL);
				if (COMPARE_VALUES_EPS(tminP.x, 0, 50) == 0)
				{
					offset += tminP.x;
				}
				tmaxP.x -= offset;
				tminP.x -= offset;
				if (COMPARE_VALUES_EPS(tminP.x, 0, 50) < 0)
				{
					tminP.x = 0;
				}
				if (COMPARE_VALUES_EPS(tmaxP.x, maxP.x, 50) > 0)
				{
					tmaxP.x = maxP.x;
				}
				sectionPairs[(int)tminP.x] = (int)tmaxP.x;
				sectionPairs[(int)tmaxP.x] = 0;
				parafaces.push_back(tmpdescrs[i]);
			}
		}

		CalculateZCorner(sectionPairs, parafaces, minP, maxP);

		map<int, int>::iterator itr = sectionPairs.begin();
		for (; itr != sectionPairs.end(); itr++)
		{
			map<int, int>::iterator itrnex = itr;
			itrnex++;
			if (itrnex == sectionPairs.end())
			{
				break;
			}
			if (itrnex->first - itr->first < 3)
			{
				continue;
			}
			int midpos = (itr->first + itrnex->first) / 2;
			//判断中心点坐标是否在平行墙中，如果在跳过（不用配钢筋），不在的话，判断左右两端是否为大面的边坐标
			//如果是，不用配置边上钢筋，不是需要配置边钢筋
			bool isInWall = false;
			if (!isInWall)//如果不在墙内
			{
				m_outsidef.pos[m_outsidef.posnum].str = itr->first;
				m_outsidef.pos[m_outsidef.posnum].end = itrnex->first;
				if (abs(itr->first - minP.x) > 100 && parafaces.size() > 0)//不是起始区间值
				{
					m_outsidef.pos[m_outsidef.posnum].addstr = true;
					if (i == 1 || i == 2)//非外侧钢筋
					{
						m_outsidef.pos[m_outsidef.posnum].strval = GetConcrete().sideCover*uor_per_mm/* * 2*/ + 2 * diameter;
					}
					else
					{
						m_outsidef.pos[m_outsidef.posnum].strval = GetConcrete().sideCover*uor_per_mm/* * 2*/ + diameter;
					}
				}
				if (abs(itrnex->first - maxP.x) > 100 && parafaces.size() > 0)//不是终止区间值
				{
					m_outsidef.pos[m_outsidef.posnum].addend = true;
					if (i == 1 || i == 2)//非外侧钢筋
					{
						m_outsidef.pos[m_outsidef.posnum].endval = GetConcrete().sideCover*uor_per_mm/* * 2*/ + 2 * diameter;
					}
					else
					{
						m_outsidef.pos[m_outsidef.posnum].endval = GetConcrete().sideCover*uor_per_mm/* * 2*/ + diameter;
					}
				}
				m_outsidef.posnum++;
			}
		}
	}
	else//横向钢筋X方向
	{
		//中间位置钢筋线,计算两端是否有垂直钢筋
		DVec3d tempvec = DVec3d::From(1, 0, 0);
		DPoint3d midstr = midpos;
		DPoint3d midend = midstr;
		if (vec1.DotProduct(tempvec) > 0.9)
		{
			midstr.x = midpos.x - m_ldfoordata.Xlenth / 2 + GetConcrete().sideCover*uor_per_mm;
			midend.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetConcrete().sideCover*uor_per_mm;
		}
		else
		{
			midstr.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetConcrete().sideCover*uor_per_mm;
			midend.x = midpos.x - m_ldfoordata.Xlenth / 2 + GetConcrete().sideCover*uor_per_mm;
		}
		int verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size());
		switch (verRe)
		{
		case 0:
		{
			m_outsidef.Verstr = false;
			m_outsidef.Verend = false;
			break;
		}
		case 1:
		{
			m_outsidef.Verstr = true;
			m_outsidef.Verend = false;
			break;
		}
		case 2:
		{
			m_outsidef.Verstr = false;
			m_outsidef.Verend = true;
			break;
		}
		case 3:
		{
			m_outsidef.Verstr = true;
			m_outsidef.Verend = true;
			break;
		}
		}

		//起始位置钢筋线，计算起始位置是否有平行墙
		DPoint3d firstr = midpos;
		if (vec2.DotProduct(DVec3d::From(0, 0, 1)) > 0.9)
		{
			firstr.z = firstr.z - m_ldfoordata.Ylenth / 2 + GetConcrete().sideCover*uor_per_mm;
		}
		else
		{
			firstr.z = firstr.z + m_ldfoordata.Ylenth / 2 - GetConcrete().sideCover*uor_per_mm;
		}
		firstr.x = midstr.x;
		DPoint3d firend = firstr;
		firend.x = midend.x;
		if (IsHaveParaWall(firstr, firend, &tmpdescrs.at(0), tmpdescrs.size()))
		{
			m_outsidef.isdelstr = true;

		}
		EditElementHandle eehstr;
		LineHandler::CreateLineElement(eehstr, nullptr, DSegment3d::From(firstr, firend), true, *ACTIVEMODEL);
		//eehstr.AddToModel();


		//终止位置钢筋线，计算终止位置是否有平行钢筋
		DPoint3d secstr = midpos;
		if (vec2.DotProduct(DVec3d::From(0, 0, 1)) > 0.9)
		{
			secstr.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetConcrete().sideCover*uor_per_mm;
		}
		else
		{
			secstr.z = midpos.z - m_ldfoordata.Ylenth / 2 + GetConcrete().sideCover*uor_per_mm;

		}
		secstr.x = midstr.x;
		DPoint3d secend = secstr;
		secend.x = midend.x;
		if (IsHaveParaWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size()))
		{
			m_outsidef.isdelend = true;
		}
		if (m_ldfoordata.downnum > 0)//顶部外侧面
		{
			// if (i > 0 && (m_outsidef.Verstr || m_outsidef.Verend))//如果前面还有钢筋层，且有和钢筋线垂直的墙，钢筋长度左右都减少一个钢筋直径
			// {
			// 	m_outsidef.calLen = diameterPre;
			// }
			double angel = 0;
			double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, -1));
			angel = thod / PI * 180;
			if (m_outsidef.Verstr)
			{
				m_outsidef.strtype.endType = 4;
				m_outsidef.strtype.rotateAngle = angel;//0;
				m_outsidef.strtype.endPtInfo.value1 = bendradius;
				m_outsidef.strtype.endPtInfo.value3 = LaE;
			}
			if (m_outsidef.Verend)
			{
				m_outsidef.endtype.endType = 4;
				m_outsidef.endtype.rotateAngle = angel;//0;
				m_outsidef.endtype.endPtInfo.value1 = bendradius;
				m_outsidef.endtype.endPtInfo.value3 = LaE;
			}
		}
		else if (m_ldfoordata.upnum > 0)//底部外侧面
		{
			// if (i > 0 && (m_outsidef.Verstr || m_outsidef.Verend))//如果前面还有钢筋层，且有和钢筋线垂直的墙，钢筋长度左右都减少一个钢筋直径
			// {
			// 	m_outsidef.calLen = diameterPre;
			// }
			double angel = 0;
			double thod = GetfaceNormal().AngleTo(DVec3d::From(0, 0, 1));
			angel = thod / PI * 180;
			if (m_outsidef.Verstr)
			{
				m_outsidef.strtype.endType = 4;
				m_outsidef.strtype.rotateAngle = angel;//180;
				m_outsidef.strtype.endPtInfo.value1 = bendradius;
				m_outsidef.strtype.endPtInfo.value3 = LaE;
			}
			if (m_outsidef.Verend)
			{
				m_outsidef.endtype.endType = 4;
				m_outsidef.endtype.rotateAngle = angel;//180;
				m_outsidef.endtype.endPtInfo.value1 = bendradius;
				m_outsidef.endtype.endPtInfo.value3 = LaE;
			}
		}
		else//上下都没有墙，正常面处理
		{
			m_sidetype = SideType::Nor;
		}
		double la0 = 15 * stod(GetMainRebars().at(i).rebarSize) * uor_per_mm;
		CreateAnchorBySelf(tmpAnchordescrs, m_LineSeg1, bendradius, la0/*15 * diameter*/, LaE, diameter, diameterPre, i, false);

		//计算配筋区间值,平行钢筋处理
		map<int, int>  sectionPairs;
		double offset = 0;
		if (COMPARE_VALUES_EPS(minP.z, 0, 50) == 0)
		{
			offset += minP.z;
			maxP.z -= offset;
			minP.z -= offset;
		}
		sectionPairs[(int)minP.z] = (int)maxP.z;//大面区间
		sectionPairs[(int)maxP.z] = 0;//大面区间
		vector<MSElementDescrP> parafaces;//平行墙
		for (int i = 0; i < tmpdescrs.size(); i++)
		{
			/*过滤一些墙*/
			DRange3d faceRange;
			mdlElmdscr_computeRange(&faceRange.low, &faceRange.high, tmpdescrs[i], NULL);
			if (faceRange.XLength() < 0.8 * m_ldfoordata.Xlenth && faceRange.ZLength() < 0.8 * m_ldfoordata.Ylenth)
			{
				/*if (faceRange.low.x < 2 * uor_per_mm || faceRange.low.z < 2 * uor_per_mm || abs(faceRange.high.z - m_ldfoordata.Ylenth) < 2 * uor_per_mm || abs(faceRange.high.x - m_ldfoordata.Xlenth) < 2 * uor_per_mm)
				{
					;
				}*/
				if (faceRange.XLength() > faceRange.ZLength())//横墙
				{
					if (faceRange.low.z < 2 * uor_per_mm || abs(faceRange.high.z - m_ldfoordata.Ylenth) < 2 * uor_per_mm)
						;
					else
						continue;
				}
				else//竖墙
				{
					if (faceRange.low.x < 2 * uor_per_mm || abs(faceRange.high.x - m_ldfoordata.Xlenth) < 2 * uor_per_mm)
						;
					else
						continue;
				}

			}
			/*过滤一些墙*/
			DPoint3d tmpstr, tmpend;
			tmpstr = tmpend = DPoint3d::From(0, 0, 0);
			PITCommonTool::CElementTool::GetLongestLineMidPt(tmpdescrs[i], tmpstr, tmpend);
			DVec3d vectmp = tmpend - tmpstr;
			vectmp.Normalize();
			if (abs(vectmp.DotProduct(tempvec)) > 0.9)
			{
				DPoint3d tminP, tmaxP;
				//计算指定元素描述符中元素的范围。
				mdlElmdscr_computeRange(&tminP, &tmaxP, tmpdescrs[i], NULL);
				// tmpqj[m_ldfoordata.Ylenth - (int)tminP.z] = m_ldfoordata.Ylenth - (int)tmaxP.z;
				// tmpqj[m_ldfoordata.Ylenth - (int)tmaxP.z] = 0;
				if (COMPARE_VALUES_EPS(tminP.z, 0, 50) == 0)
				{
					offset += tminP.z;
				}
				tmaxP.z -= offset;
				tminP.z -= offset;
				if (COMPARE_VALUES_EPS(tminP.z, 0, 50) < 0)
				{
					tminP.z = 0;
				}
				if (COMPARE_VALUES_EPS(tmaxP.z, maxP.z, 50) > 0)
				{
					tmaxP.z = maxP.z;
				}
				sectionPairs[(int)tminP.z] = (int)tmaxP.z;
				sectionPairs[(int)tmaxP.z] = 0;
				parafaces.push_back(tmpdescrs[i]);
			}
		}

		CalculateZCorner(sectionPairs, parafaces, minP, maxP, false);

		map<int, int>::iterator itr = sectionPairs.begin();
		for (; itr != sectionPairs.end(); itr++)
		{
			map<int, int>::iterator itrnex = itr;
			itrnex++;
			if (itrnex == sectionPairs.end())
			{
				break;
			}
			if (itrnex->first - itr->first < 3)
			{
				continue;
			}
			int midpos = (itr->first + itrnex->first) / 2;
			//判断中心点坐标是否在平行墙中，如果在跳过（不用配钢筋），不在的话，判断左右两端是否为大面的边坐标
			//如果是，不用配置边上钢筋，不是需要配置边钢筋
			bool isInWall = false;
			if (!isInWall)//如果不在墙内
			{
				m_outsidef.pos[m_outsidef.posnum].str = itr->first;
				m_outsidef.pos[m_outsidef.posnum].end = itrnex->first;
				if (abs(itr->first - minP.z) > 100 && parafaces.size() > 0)//不是起始区间值
				{
					if (i == 1 || i == 2)//非外侧钢筋
					{
						m_outsidef.pos[m_outsidef.posnum].strval = GetConcrete().sideCover*uor_per_mm/* * 2*/ + 2 * diameter;
					}
					else
					{
						m_outsidef.pos[m_outsidef.posnum].strval = GetConcrete().sideCover*uor_per_mm/* * 2*/ + diameter;
					}
				}
				if (abs(itrnex->first - maxP.z) > 100 && parafaces.size() > 0)//不是终止区间值
				{
					if (i == 1 || i == 2)//非外侧钢筋
					{
						m_outsidef.pos[m_outsidef.posnum].endval = GetConcrete().sideCover*uor_per_mm/* * 2*/ + 2 * diameter;
					}
					else
					{
						m_outsidef.pos[m_outsidef.posnum].endval = GetConcrete().sideCover*uor_per_mm/* * 2*/ + diameter;
					}
				}
				m_outsidef.posnum++;
			}
		}
	}
}


//平面配筋---end
long PlaneRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		map << _d;
		return 0;
	}
	default:
		break;
	}
	return -1;
}


/*
	* @desc:		根据周围元素和钢筋线的位置计算钢筋是否合法
	* @param[in]	originPt  钢筋起始点
	* @param[in]	dSideCover 侧向保护距离
	* @param[in]	alleehs  所有实体集合
	* @param[in/out]    tmpEndType  钢筋的末端类型
	* @author	LiuSilei
	* @Date:	2024/11/15
	*/
	// 钢筋锚固检查与处理函数
void PlaneRebarAssembly::CheckRebarAnchorage(
	DPoint3d& originPt,    // 钢筋起始点
	double dSideCover,  // 侧向保护距离
	vector<EditElementHandle*> alleehs, // 所有实体集合
	PIT::PITRebarEndType& tmpEndType // 钢筋的末端类型
)
{
	CVector3D vec = tmpEndType.GetendNormal();
	double length = tmpEndType.GetbendLen() + tmpEndType.GetbendRadius();
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	DPoint3d endTempPt = originPt;
	DPoint3d endTempPtEx = originPt;

	BrString strRebarSize(GetMainRebars().at(0).rebarSize);
	double lastDiameter = 0.0;//上一层的钢筋直径
	if (m_curLevel == 1)
		lastDiameter = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL);
	double moveDis = (GetConcrete().postiveCover * 2 * UOR_PER_MilliMeter) + lastDiameter + m_curreDiameter / 2;


	// Step 1: 移动钢筋到新的位置，检查是否在实体内
	movePoint(vec, endTempPt, length);
	if (!ISPointInHoles(alleehs, endTempPt))
	{
		vec.Negate();  // 如果不在实体内，反转钢筋方向
	}
	movePoint(vec, endTempPtEx, length);

	// Step 2: 如果锚固点依旧不在实体内，进行进一步检查
	if (!ISPointInHoles(alleehs, endTempPtEx)) // 判断反转后是否没有锚入实体
	{
		vec.Negate();  // 反转方向

		// Step 3: 如果锚固点与孔洞有交点，进行反转和截短处理
		if (m_Holeehs.size() != 0)
		{
			GetIntersectPointsWithHoles(tmppts, m_Holeehs, originPt, endTempPt, dSideCover, matrix);
			if (tmppts.size() <= 0)
				GetIntersectPointsWithHolesByInsert(tmppts, m_Holeehs, originPt, endTempPt, dSideCover, matrix);
		}

		// Step 4: 如果交点存在，计算新的锚固点位置
		if (tmppts.size() > 0)
		{
			double dis = (double)originPt.Distance(endTempPt);
			if (dis != length)
			{
				CVector3D vectortepm = originPt - endTempPt;
				vectortepm.Normalize();
				vectortepm.ScaleToLength(moveDis);
				endTempPt.Add(vectortepm);

				// 重新计算距离，减去弯曲半径
				dis = (double)originPt.Distance(endTempPt) - tmpEndType.GetbendRadius();

				if (dis < 0) // 如果弯曲半径已超出范围，进行反转并截短
				{
					vec.Negate();
					endTempPt = originPt;
					endTempPtEx = originPt;
					movePoint(vec, endTempPt, length);
					GetIntersectPointsWithHoles(tmppts, alleehs, endTempPtEx, endTempPt, dSideCover, matrix);
					GetIntersectPointsWithHolesByInsert(tmppts, alleehs, endTempPtEx, endTempPt, dSideCover, matrix);

					if (tmppts.size() > 0)
					{
						vectortepm = originPt - endTempPtEx;
						vectortepm.Normalize();
						vectortepm.ScaleToLength(moveDis);
						endTempPtEx.Add(vectortepm);
						dis = (double)originPt.Distance(endTempPtEx) - tmpEndType.GetbendRadius();
						tmpEndType.SetbendLen(dis);
					}
				}
				else
				{
					tmpEndType.SetbendLen(dis);
				}
			}
		}
		// Step 5: 如果没有交点且未锚固，继续检查并截短
		else if (!ISPointInHoles(alleehs, endTempPt))
		{
			endTempPt = originPt;
			endTempPtEx = originPt;
			movePoint(vec, endTempPt, length);
			GetIntersectPointsWithHoles(tmppts, alleehs, endTempPtEx, endTempPt, dSideCover, matrix);
			GetIntersectPointsWithHolesByInsert(tmppts, alleehs, endTempPtEx, endTempPt, dSideCover, matrix);

			if (tmppts.size() > 0)
			{
				CVector3D vectortepm = originPt - endTempPtEx;
				vectortepm.Normalize();
				vectortepm.ScaleToLength(moveDis);
				endTempPtEx.Add(vectortepm);
				length = (double)originPt.Distance(endTempPtEx) - tmpEndType.GetbendRadius();
				tmpEndType.SetbendLen(length);
			}
		}
		else
		{
			vec.Negate();  // 反转方向
		}
	}
	tmpEndType.SetendNormal(vec);
}