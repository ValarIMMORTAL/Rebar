/*--------------------------------------------------------------------------------------+
|
|     $Source: MySlabRebarAssembly.cpp $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include "resource.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "LDSlabRebarAssembly.h"

#include <CPointTool.h>

#include "WallRebarDlg.h"
#include "ExtractFacesTool.h"
#include "CSlabRebarDlg.h"
#include "TieRebar.h"
#include "ElementAttribute.h"
#include "PITRebarEndType.h"
#include "PITMSCECommon.h"
#include "BentlyCommonfile.h"
#include "XmlHelper.h"
#include "CFaceTool.h"
#include "CElementTool.h"
#include "WallHelper.h"
#include "CModelTool.h"
#include "CSolidTool.h"

extern bool SlabPreviewButtonDown;//板主要配筋界面的预览按钮
extern int direction;
extern EditElementHandle m_pDownFace;
extern GlobalParameters g_globalpara;
namespace Gallery
{
	void LDSlabRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
	{
		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
		m_useHoleehs.clear();
		double dSideCover = GetSideCover()*uor_per_mm;
		Transform matrix;
		GetPlacement().AssignTo(matrix);

		Transform trans;
		GetPlacement().AssignTo(trans);
		trans.InverseOf(trans);
		if (g_wallRebarInfo.concrete.isHandleHole)//计算需要处理的孔洞
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

	double LDSlabRebarAssembly::get_lae() const
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

	double LDSlabRebarAssembly::get_la0() const
	{
		if (g_globalpara.m_laplenth.find("A") != g_globalpara.m_laplenth.end())
		{
			auto iter = g_globalpara.m_laplenth.find("A");
			return iter->second * UOR_PER_MilliMeter;
		}
		else if (g_globalpara.m_laplenth.find("C") != g_globalpara.m_laplenth.end())
		{
			auto iter = g_globalpara.m_laplenth.find("C");
			return iter->second * UOR_PER_MilliMeter;
		}
		else if (g_globalpara.m_laplenth.find("D") != g_globalpara.m_laplenth.end())
		{
			auto iter = g_globalpara.m_laplenth.find("D");
			return iter->second * UOR_PER_MilliMeter;
		}
		else
		{
			return -1.0;
		}
	}

	std::vector<ElementHandle> LDSlabRebarAssembly::scan_elements_in_range(const DRange3d & range, std::function<bool(const ElementHandle&)> filter)
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
	bool Gallery::LDSlabRebarAssembly::is_Floor(const ElementHandle & element)
	{
		std::string _name, type;
		if (!GetEleNameAndType(const_cast<ElementHandleR>(element), _name, type))
		{
			return false;
		}
		if (type.find("FLOOR") != std::string::npos)//墙
		{
			return true;
		}
		else 
			return false;
	}
	bool Gallery::LDSlabRebarAssembly::is_Wall(const ElementHandle & element)
	{
		std::string _name, type;
		if (!GetEleNameAndType(const_cast<ElementHandleR>(element), _name, type))
		{
			return false;
		}
		if(type.find("WALL")!= std::string::npos)//墙
		{
			return true;
		}
		else if (type.find("FLOOR") != std::string::npos)//板
		{
			DRange3d slab_range;
			mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, element.GetElementDescrCP(), NULL);
			slab_range.low.z -= 5 * UOR_PER_MilliMeter;

			slab_range.high.z += 5 * UOR_PER_MilliMeter;
			vector<ElementHandle> temp;
			temp = // 扫描附近的板
				scan_elements_in_range(
					slab_range,
					[&](const ElementHandle &eh) -> bool {
						if (eh.GetElementId() == m_pOldElm->GetElementId())
						{
							// 过滤掉自己
							return false;
						}
						// 只需要墙
						return is_Floor(eh);
					});
			bool up = false, down = false;
			for (auto itr : temp)
			{
				DPoint3d minP = { 0 }, maxP = { 0 };
				mdlElmdscr_computeRange(&minP, &maxP, itr.GetElementDescrCP(), NULL);
				DRange3d range;
				range.low = minP;
				range.high = maxP;

				if (range.IsContained(slab_range.high))
					up = true;
				if (range.IsContained(slab_range.low))
					down = true;
			}
			if (down&&up)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		return false;
	}


	//设置钢筋的curve起点的端部样式
	PITRebarEndType LDSlabRebarAssembly::Set_begCurvePoint(PITRebarEndTypes & tmpendTypes, PITRebarEndTypes & endTypes, map<int, DPoint3d>& map_pts, int & num, int & begflag, DRange3d & hole_range, double & xStrdistance, double & xEnddistance, double & yStrdistance, double & yEnddistance, map<int, DPoint3d>::iterator & itr, map<int, DPoint3d>::iterator & tmpitrnext)
	{
		double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		if ((map_pts.size() == 4) && (num % 2 == 1) || ((map_pts.size() > 4) && ((num > 0) && (num < map_pts.size() / 2))))
		{// 有孔洞的情况，包括一条钢筋穿过一个孔洞和穿过多个孔洞的情况
			PITRebarEndType Hole_beg = Set_endType();
			if ((m_isdown && !m_isoutside) || (!m_isdown && !m_isoutside))//下面一层并且是内侧面或者是上面一层的内侧面
			{
				CVector3D endNormal = Hole_beg.GetendNormal();
				endNormal.Negate();
				Hole_beg.SetendNormal(endNormal);
			}
			double Zhight = m_STwallData.width;
			if (/*(num % 2 == 1) && */(GetvecDir().at(m_rebarlevel) == 1))
			{
				double lastRadius = 0.0;
				if (GetvecDir().at(0) == 0)//板配筋是横纵纵横
				{
					if (m_rebarlevel == 1)
						lastRadius = m_mapLevelRadius[0];
					else if (m_rebarlevel == 2)
						lastRadius = m_mapLevelRadius[3];
					Hole_beg.SetbendLen(Zhight - Hole_beg.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
				}
				else//纵横横纵
				{
					Hole_beg.SetbendLen(Zhight - Hole_beg.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
				}
				itr->second.y += m_RebarRadius;
			}
			else if (/*(num % 2 == 1) && */(GetvecDir().at(m_rebarlevel) == 0))//x
			{
				double lastRadius = 0.0;
				if (GetvecDir().at(0) == 1)//板配筋是纵横横纵
				{
					if (m_rebarlevel == 1)
						lastRadius = m_mapLevelRadius[0];
					else if (m_rebarlevel == 2)
						lastRadius = m_mapLevelRadius[3];
				}
				Hole_beg.SetbendLen(Zhight - Hole_beg.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
				itr->second.x += m_RebarRadius;
			}
			tmpendTypes.beg = Hole_beg;
			begflag = 1;

		}
		else if ((m_useHoleehs.size() > 0) && (map_pts.size() == 2) && (abs(tmpitrnext->first - m_STwallData.length) > 4 * GetSideCover() * uor_per_mm)/*(abs(tmpitrnext->first - m_xdistance) > 0.01) && (xStrdistance > xEnddistance)*/)
		{// 有孔洞且孔洞在板边缘的情况
			if ((GetvecDir().at(m_rebarlevel) == 0) && ((abs(itr->second.x - hole_range.low.x) < 1.5* GetSideCover() * uor_per_mm) || ((abs(itr->second.x - hole_range.high.x) < 1.5* GetSideCover() * uor_per_mm))))
			{
				PITRebarEndType Hole_beg = Set_endType();
				if ((m_isdown && !m_isoutside) || (!m_isdown && !m_isoutside))//下面一层并且是内侧面或者是上面一层的内侧面
				{
					CVector3D endNormal = Hole_beg.GetendNormal();
					endNormal.Negate();
					Hole_beg.SetendNormal(endNormal);
				}
				double Zhight = m_STwallData.width;
				if (/*(num % 2 == 1) && */(GetvecDir().at(m_rebarlevel) == 1))
				{
					double lastRadius = 0.0;
					if (GetvecDir().at(0) == 0)//板配筋是横纵纵横
					{
						if (m_rebarlevel == 1)
							lastRadius = m_mapLevelRadius[0];
						else if (m_rebarlevel == 2)
							lastRadius = m_mapLevelRadius[3];
					}
					Hole_beg.SetbendLen(Zhight - Hole_beg.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
					itr->second.y += m_RebarRadius;
				}
				else if (GetvecDir().at(m_rebarlevel) == 0)//x
				{
					double lastRadius = 0.0;
					if (GetvecDir().at(0) == 1)//板配筋是纵横横纵
					{
						if (m_rebarlevel == 1)
							lastRadius = m_mapLevelRadius[0];
						else if (m_rebarlevel == 2)
							lastRadius = m_mapLevelRadius[3];
					}
					Hole_beg.SetbendLen(Zhight - Hole_beg.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
					itr->second.x += m_RebarRadius;
				}
				
				tmpendTypes.beg = Hole_beg;
				begflag = 1;
			}
			else if ((GetvecDir().at(m_rebarlevel) == 1) && ((abs(itr->second.y - hole_range.low.y) < 1.5* GetSideCover() * uor_per_mm) || ((abs(itr->second.y - hole_range.high.y) < 1.5* GetSideCover() * uor_per_mm))))
			{
				PITRebarEndType Hole_beg = Set_endType();
				if ((m_isdown && !m_isoutside) || (!m_isdown && !m_isoutside))//下面一层并且是内侧面或者是上面一层的内侧面
				{
					CVector3D endNormal = Hole_beg.GetendNormal();
					endNormal.Negate();
					Hole_beg.SetendNormal(endNormal);
				}
				double Zhight = m_STwallData.width;
				if (/*(num % 2 == 1) && */(GetvecDir().at(m_rebarlevel) == 1))
				{
					double lastRadius = 0.0;
					if (GetvecDir().at(0) == 0)//板配筋是横纵纵横
					{
						if (m_rebarlevel == 1)
							lastRadius = m_mapLevelRadius[0];
						else if (m_rebarlevel == 2)
							lastRadius = m_mapLevelRadius[3];
					}
					Hole_beg.SetbendLen(Zhight - Hole_beg.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
					itr->second.y += m_RebarRadius;
				}
				else if (GetvecDir().at(m_rebarlevel) == 0)//x
				{
					double lastRadius = 0.0;
					if (GetvecDir().at(0) == 0)//板配筋是横纵纵横
					{
						if (m_rebarlevel == 1)
							lastRadius = m_mapLevelRadius[0];
						else if (m_rebarlevel == 2)
							lastRadius = m_mapLevelRadius[3];
					}
					Hole_beg.SetbendLen(Zhight - Hole_beg.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
					itr->second.x += m_RebarRadius;
				}
				tmpendTypes.beg = Hole_beg;
				begflag = 1;
			}

		}
		//else if ((map_pts.size() == 2) && (abs(tmpitrnext->first - m_xdistance) > 0.01) && (xStrdistance > xEnddistance) && (GetvecDir().at(m_rebarlevel) == 0))
		//{// 没有孔洞但是x方向的钢筋比正常的钢筋短的情况，用于异形板遇到斜墙的锚入
		//	for (auto temwall_eeh : m_Allwalls)
		//	{
		//		DRange3d tmpWall_range;
		//		mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, temwall_eeh.GetElementDescrCP(), nullptr);
		//		if (tmpWall_range.IsContainedXY(itr->second))
		//		{
		//			if (m_endtypes == 2)
		//			{
		//				tmpendTypes.beg = endTypes.end;
		//				begflag = 2;
		//				break;
		//			}
		//			else if (m_endtypes == 0)
		//			{
		//				//自己设置端部样式
		//				tmpendTypes.beg = Set_endType();
		//				begflag = 2;
		//				break;

		//			}

		//		}
		//	}
		//}
		//else if ((map_pts.size() == 2) && (abs(tmpitrnext->first - m_ydistance) > 0.01) && (yStrdistance > yEnddistance) && (GetvecDir().at(m_rebarlevel) == 1))
		//{// 没有孔洞但是y方向的钢筋比正常的钢筋短的情况，用于异形板遇到斜墙的锚入
		//	for (auto temwall_eeh : m_Allwalls)
		//	{
		//		DRange3d tmpWall_range;
		//		mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, temwall_eeh.GetElementDescrCP(), nullptr);
		//		if (tmpWall_range.IsContainedXY(itr->second))
		//		{
		//			if (m_endtypes == 2)
		//			{
		//				tmpendTypes.beg = endTypes.end;
		//				begflag = 2;
		//				break;
		//			}
		//			else if (m_endtypes == 0)
		//			{
		//				//自己设置端部样式
		//				tmpendTypes.beg = Set_endType();
		//				begflag = 2;
		//				break;
		//			}

		//		}
		//	}
		//}
		if ((begflag == 1) && (m_useHoleehs.size() > 0))//有孔洞的情况下遇到墙需要往墙上锚入
		{
			for (auto walleh : m_Allwalls)
			{
				if (m_ldfoordata.downnum == 0 && m_ldfoordata.upnum == 0)//7BGZ7Y01DB 板特殊处理
					break;
				DRange3d tmpWall_range;
				mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, walleh.GetElementDescrCP(), nullptr);
				if (tmpWall_range.IsContainedXY(itr->second)/* && (begflag == 1) && (m_useHoleehs.size() > 0)*/)
				{	// 判断钢筋断点在墙的范围内并且根据孔洞做了锚固
					vector<PIT::ConcreteRebar>	my_vecRebarData;
					WallRebarInfo my_wallRebarInfo;
					my_wallRebarInfo.concrete.postiveCover = 50;
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
					if (tmpWall_range.high.z > map_pts.begin()->second.z)//墙在孔洞上方
					{
						if (GetvecDataExchange().at(m_rebarlevel) == 0)//板底
						{
							if (GetvecDir().at(m_rebarlevel) == 0)
							{
								double xpoint = tmpWall_range.low.x + my_wallRebarInfo.concrete.postiveCover * uor_per_mm + m_RebarRadius;
								if(abs(xpoint - itr->second.x) < 4 * GetSideCover() * uor_per_mm)
									itr->second.x = tmpWall_range.low.x + my_wallRebarInfo.concrete.postiveCover * uor_per_mm + m_RebarRadius;
							}
							else
							{
								double ypoint = tmpWall_range.low.y + my_wallRebarInfo.concrete.postiveCover * uor_per_mm + m_RebarRadius;
								if(abs(ypoint - itr->second.y) < 4 * GetSideCover() * uor_per_mm)
									itr->second.y = tmpWall_range.low.y + my_wallRebarInfo.concrete.postiveCover * uor_per_mm + m_RebarRadius;
							}
							tmpendTypes.beg.SetbendLen(get_lae() * 2 * m_RebarRadius / uor_per_mm);
							if (tmpWall_range.ZLength() < 15 * 2 * m_RebarRadius)//顶部墙高度太低，需要截断
							{
								double bendradius = RebarCode::GetPinRadius(GetvecDirSize().at(m_rebarlevel), ACTIVEMODEL, false);
								if (m_rebarlevel == 0)
								{
									tmpendTypes.beg.SetbendLen(m_ldfoordata.Zlenth + tmpWall_range.ZLength() - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - GetPositiveCover() * uor_per_mm - bendradius - m_RebarRadius);
								}
								else if (m_rebarlevel == 1)
								{
									tmpendTypes.beg.SetbendLen(m_ldfoordata.Zlenth + tmpWall_range.ZLength() - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - GetPositiveCover() * uor_per_mm - bendradius - m_RebarRadius - 2 * m_mapLevelRadius[0]);
								}
							}	
							break;
						}
						else if (GetvecDataExchange().at(m_rebarlevel) == 2)//板顶
						{
							double diameterV = 0.0;//竖筋
							double diameterH = 0.0;//横筋
							char rebarSize[512] = "12C";
							diameterV = RebarCode::GetBarDiameter(rebarSize, ACTIVEMODEL);
							diameterH = RebarCode::GetBarDiameter(rebarSize, ACTIVEMODEL);
							if ((contid != 0) && (my_vecRebarData.size() != 0))
							{
								for (int i = 0; i < my_vecRebarData.size(); ++i)
								{
									if (my_vecRebarData[i].rebarDir == 1 && my_vecRebarData[i].datachange == 0)
									{
										diameterV = RebarCode::GetBarDiameter(my_vecRebarData[i].rebarSize, ACTIVEMODEL);
									}
									if (my_vecRebarData[i].rebarDir == 0 && my_vecRebarData[i].datachange == 0)
									{
										diameterH = RebarCode::GetBarDiameter(my_vecRebarData[i].rebarSize, ACTIVEMODEL);
									}

								}
							}
							else
							{
								char rebarSize[512] = "12C";
								diameterV = RebarCode::GetBarDiameter(rebarSize, ACTIVEMODEL);
								diameterH = RebarCode::GetBarDiameter(rebarSize, ACTIVEMODEL);
							}
							if (GetvecDir().at(m_rebarlevel) == 0)
							{
								double xpoint = tmpWall_range.low.x + my_wallRebarInfo.concrete.postiveCover * uor_per_mm + diameterH + diameterV + m_RebarRadius;
								if(abs(xpoint - itr->second.x) < 4 * GetSideCover() * uor_per_mm)
									itr->second.x = tmpWall_range.low.x + my_wallRebarInfo.concrete.postiveCover * uor_per_mm + diameterH + diameterV + m_RebarRadius;
							}
							else
							{
								double ypoint = tmpWall_range.low.y + my_wallRebarInfo.concrete.postiveCover * uor_per_mm + diameterH + diameterV + m_RebarRadius;
								if (abs(ypoint - itr->second.y) < 4 * GetSideCover() * uor_per_mm)
									itr->second.y = tmpWall_range.low.y + my_wallRebarInfo.concrete.postiveCover * uor_per_mm + diameterH + diameterV + m_RebarRadius;
							}
							CVector3D endNormal = tmpendTypes.beg.GetendNormal();
							endNormal.Negate();
							tmpendTypes.beg.SetendNormal(endNormal);
							tmpendTypes.beg.SetbendLen(15 * 2 * m_RebarRadius);
							if (tmpWall_range.ZLength() < 15 * 2 * m_RebarRadius)//顶部墙高度太低，需要截断
							{
								double bendradius = RebarCode::GetPinRadius(GetvecDirSize().at(m_rebarlevel), ACTIVEMODEL, false);
								if(m_rebarlevel == 2)
									tmpendTypes.beg.SetbendLen(tmpWall_range.ZLength() + GetReverseCover() * uor_per_mm  + 2 * m_mapLevelRadius[3] + m_RebarRadius - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - bendradius);
								else if(m_rebarlevel == 3)
									tmpendTypes.beg.SetbendLen(tmpWall_range.ZLength() + GetReverseCover() * uor_per_mm  + m_RebarRadius - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - bendradius);
							}
							break;
						}
					}
				}
			}
		}

		return tmpendTypes.beg;
	}

	PITRebarEndType LDSlabRebarAssembly::Set_endCurvePoint(PITRebarEndTypes & tmpendTypes, PITRebarEndTypes & endTypes, map<int, DPoint3d>& map_pts, int & num, int & endflag, DRange3d & hole_range, double & xStrdistance, double & xEnddistance, double & yStrdistance, double & yEnddistance, map<int, DPoint3d>::iterator & itrplus)
	{
		double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		if (((map_pts.size() == 4) && (num % 2 == 0)) || ((map_pts.size() > 4) && ((num >= 0) && (num < map_pts.size()/2 - 1))))
		{// 有孔洞的情况，包括一条钢筋穿过一个孔洞和穿过多个孔洞的情况
			PITRebarEndType Hole_end = Set_endType();
			if ((m_isdown && !m_isoutside) || (!m_isdown && !m_isoutside))//下面一层并且是内侧面或者是上面一层的内侧面
			{
				CVector3D endNormal = Hole_end.GetendNormal();
				endNormal.Negate();
				Hole_end.SetendNormal(endNormal);
			}
			double Zhight = m_STwallData.width;
			if (/*(num % 2 == 0) && */(GetvecDir().at(m_rebarlevel) == 1))//y方向
			{
				double lastRadius = 0.0;
				if (GetvecDir().at(0) == 0)//板配筋是横纵纵横
				{
					if (m_rebarlevel == 1)
						lastRadius = m_mapLevelRadius[0];
					else if (m_rebarlevel == 2)
						lastRadius = m_mapLevelRadius[3];
				}
				Hole_end.SetbendLen(Zhight - Hole_end.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
				itrplus->second.y -= m_RebarRadius;
			}
			else if (/*(num % 2 == 0) && */(GetvecDir().at(m_rebarlevel) == 0))//x
			{
				double lastRadius = 0.0;
				if (GetvecDir().at(0) == 1)//板配筋是纵横横纵
				{
					if (m_rebarlevel == 1)
						lastRadius = m_mapLevelRadius[0];
					else if (m_rebarlevel == 2)
						lastRadius = m_mapLevelRadius[3];
				}
				Hole_end.SetbendLen(Zhight - Hole_end.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
				itrplus->second.x -= m_RebarRadius;
			}
			tmpendTypes.end = Hole_end;
			endflag = 1;

		}
		else if ((m_useHoleehs.size() > 0) && (map_pts.size() == 2) && (abs(itrplus->first - m_xdistance) > 0.01) && (xStrdistance < xEnddistance))
		{// 有孔洞且孔洞在板边缘的情况
			if ((GetvecDir().at(m_rebarlevel) == 0) && ((abs(itrplus->second.x - hole_range.low.x) < 1.5* GetSideCover()) || ((abs(itrplus->second.x - hole_range.high.x) < 1.5* GetSideCover()))))
			{
				PITRebarEndType Hole_end = Set_endType();
				if ((m_isdown && !m_isoutside) || (!m_isdown && !m_isoutside))//下面一层并且是内侧面或者是上面一层的内侧面
				{
					CVector3D endNormal = Hole_end.GetendNormal();
					endNormal.Negate();
					Hole_end.SetendNormal(endNormal);
				}
				double Zhight = m_STwallData.width;
				if (/*(num % 2 == 0) && */(GetvecDir().at(m_rebarlevel) == 1))//y方向
				{
					double lastRadius = 0.0;
					if (GetvecDir().at(0) == 0)//板配筋是横纵纵横
					{
						if (m_rebarlevel == 1)
							lastRadius = m_mapLevelRadius[0];
						else if (m_rebarlevel == 2)
							lastRadius = m_mapLevelRadius[3];
					}		
					Hole_end.SetbendLen(Zhight - Hole_end.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
					itrplus->second.y -= m_RebarRadius;
				}
				else if (GetvecDir().at(m_rebarlevel) == 0)//x
				{
					double lastRadius = 0.0;
					if (GetvecDir().at(0) == 1)//板配筋是纵横横纵
					{
						if (m_rebarlevel == 1)
							lastRadius = m_mapLevelRadius[0];
						else if (m_rebarlevel == 2)
							lastRadius = m_mapLevelRadius[3];
					}
					Hole_end.SetbendLen(Zhight - Hole_end.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
					itrplus->second.x -= m_RebarRadius;
				}
				tmpendTypes.end = Hole_end;
				endflag = 1;
			}
			else if ((GetvecDir().at(m_rebarlevel) == 1) && ((abs(itrplus->second.y - hole_range.low.y) < 1.5* GetSideCover()) || ((abs(itrplus->second.y - hole_range.high.y) < 1.5* GetSideCover()))))
			{
				PITRebarEndType Hole_end = Set_endType();
				if ((m_isdown && !m_isoutside) || (!m_isdown && !m_isoutside))//下面一层并且是内侧面或者是上面一层的内侧面
				{
					CVector3D endNormal = Hole_end.GetendNormal();
					endNormal.Negate();
					Hole_end.SetendNormal(endNormal);
				}
				double Zhight = m_STwallData.width;
				if (/*(num % 2 == 0) && */(GetvecDir().at(m_rebarlevel) == 1))//y方向
				{
					double lastRadius = 0.0;
					if (GetvecDir().at(0) == 0)//板配筋是横纵纵横
					{
						if (m_rebarlevel == 1)
							lastRadius = m_mapLevelRadius[0];
						else if (m_rebarlevel == 2)
							lastRadius = m_mapLevelRadius[3];
					}
					Hole_end.SetbendLen(Zhight - Hole_end.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
				}
				else if (GetvecDir().at(m_rebarlevel) == 0)//x
				{
					double lastRadius = 0.0;
					if (GetvecDir().at(0) == 1)//板配筋是横纵纵横
					{
						if (m_rebarlevel == 1)
							lastRadius = m_mapLevelRadius[0];
						else if (m_rebarlevel == 2)
							lastRadius = m_mapLevelRadius[3];
					}
					Hole_end.SetbendLen(Zhight - Hole_end.GetbendRadius() - (this->GetReverseCover() + GetPositiveCover()) * uor_per_mm - m_RebarRadius - 2 * lastRadius);
				}
				tmpendTypes.end = Hole_end;
				endflag = 1;
			}

		}
		//else if ((map_pts.size() == 2) && (abs(itrplus->first - m_xdistance) > 0.01) && (xStrdistance < xEnddistance) && (GetvecDir().at(m_rebarlevel) == 0))
		//{// 没有孔洞但是x方向的钢筋比正常的钢筋短的情况，用于异形板遇到斜墙的锚入
		//	for (auto temwall_eeh : m_Allwalls)
		//	{
		//		DRange3d tmpWall_range;
		//		mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, temwall_eeh.GetElementDescrCP(), nullptr);
		//		if (tmpWall_range.IsContainedXY(itrplus->second)/*(itrplus->second.x > tmpWall_range.low.x) && (itrplus->second.x < tmpWall_range.high.x) && (itrplus->second.y > tmpWall_range.low.y) && (itrplus->second.y < tmpWall_range.high.y)*/)
		//		{
		//			if (m_endtypes == 1)
		//			{
		//				tmpendTypes.end = endTypes.beg;
		//				endflag = 2;
		//				break;
		//			}
		//			else if (m_endtypes == 0)
		//			{
		//				//自己设置端部样式
		//				tmpendTypes.end = Set_endType();
		//				endflag = 2;
		//				break;
		//			}

		//		}
		//	}
		//}
		//else if ((map_pts.size() == 2) && (abs(itrplus->first - m_ydistance) > 0.01) && (yStrdistance < yEnddistance) && (GetvecDir().at(m_rebarlevel) == 1))
		//{// 没有孔洞但是y方向的钢筋比正常的钢筋短的情况，用于异形板遇到斜墙的锚入
		//	for (auto temwall_eeh : m_Allwalls)
		//	{
		//		DRange3d tmpWall_range;
		//		mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, temwall_eeh.GetElementDescrCP(), nullptr);
		//		if (tmpWall_range.IsContainedXY(itrplus->second))
		//		{
		//			if (m_endtypes == 1)
		//			{
		//				tmpendTypes.end = endTypes.beg;
		//				endflag = 2;
		//				break;
		//			}
		//			else if (m_endtypes == 0)
		//			{
		//				//自己设置端部样式
		//				tmpendTypes.end = Set_endType();
		//				endflag = 2;
		//				break;
		//			}

		//		}
		//	}
		//}
		if ((endflag == 1) && (m_useHoleehs.size() > 0))//有孔洞的情况下遇到墙需要往墙上锚入
		{
			for (auto walleh : m_Allwalls)
			{
				if (m_ldfoordata.downnum == 0 && m_ldfoordata.upnum == 0)//7BGZ7Y01DB 板特殊处理
					break;
				DRange3d tmpWall_range;
				mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, walleh.GetElementDescrCP(), nullptr);
				if (tmpWall_range.IsContainedXY(itrplus->second)/* && (endflag == 1) && (m_useHoleehs.size() > 0)*/)
				{
					// 获取在钢筋断点上面的墙的信息，得到钢筋信息
					vector<PIT::ConcreteRebar>	my_vecRebarData;
					WallRebarInfo my_wallRebarInfo;
					my_wallRebarInfo.concrete.postiveCover = 50;
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
					if (tmpWall_range.high.z > map_pts.begin()->second.z)//墙在孔洞上方
					{
						if (GetvecDataExchange().at(m_rebarlevel) == 0)//板底
						{
							if (GetvecDir().at(m_rebarlevel) == 0)
							{
								double xpoint = tmpWall_range.high.x - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - m_RebarRadius;
								if(abs(xpoint - itrplus->second.x) < 4 * GetSideCover() * uor_per_mm)
									itrplus->second.x = tmpWall_range.high.x - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - m_RebarRadius;
							}	
							else
							{
								double ypoint = tmpWall_range.high.y - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - m_RebarRadius;
								if(abs(ypoint - itrplus->second.y) < 4 * GetSideCover() * uor_per_mm)
									itrplus->second.y = tmpWall_range.high.y - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - m_RebarRadius;
							}
							tmpendTypes.end.SetbendLen(get_lae() * 2 * m_RebarRadius / uor_per_mm);
							if (tmpWall_range.ZLength() < 15 * 2 * m_RebarRadius)//顶部墙高度太低，需要截断
							{
								double bendradius = RebarCode::GetPinRadius(GetvecDirSize().at(m_rebarlevel), ACTIVEMODEL, false);
								if (m_rebarlevel == 0)
								{
									tmpendTypes.end.SetbendLen(m_ldfoordata.Zlenth + tmpWall_range.ZLength() - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - GetPositiveCover() * uor_per_mm - bendradius - m_RebarRadius);
								}
								else if (m_rebarlevel == 1)
								{
									tmpendTypes.end.SetbendLen(m_ldfoordata.Zlenth + tmpWall_range.ZLength() - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - GetPositiveCover() * uor_per_mm - bendradius - m_RebarRadius - 2 * m_mapLevelRadius[0]);
								}
							}
							break;
						}
						else if (GetvecDataExchange().at(m_rebarlevel) == 2)//板顶
						{
							double diameterV = 0.0;//竖筋
							double diameterH = 0.0;//横筋
							char rebarSize[512] = "12C";
							diameterV = RebarCode::GetBarDiameter(rebarSize, ACTIVEMODEL);
							diameterH = RebarCode::GetBarDiameter(rebarSize, ACTIVEMODEL);
							if ((contid != 0) && (my_vecRebarData.size() != 0))
							{
								for (int i = 0; i < my_vecRebarData.size(); ++i)
								{
									if (my_vecRebarData[i].rebarDir == 1 && my_vecRebarData[i].datachange == 2)
									{
										diameterV = RebarCode::GetBarDiameter(my_vecRebarData[i].rebarSize, ACTIVEMODEL);
									}
									if (my_vecRebarData[i].rebarDir == 0 && my_vecRebarData[i].datachange == 2)
									{
										diameterH = RebarCode::GetBarDiameter(my_vecRebarData[i].rebarSize, ACTIVEMODEL);
									}

								}
							}
							else
							{
								char rebarSize[512] = "12C";
								diameterV = RebarCode::GetBarDiameter(rebarSize, ACTIVEMODEL);
								diameterH = RebarCode::GetBarDiameter(rebarSize, ACTIVEMODEL);
							}
							if (GetvecDir().at(m_rebarlevel) == 0)
							{
								double xpoint = tmpWall_range.high.x - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - diameterH - diameterV - m_RebarRadius;
								if(abs(xpoint - itrplus->second.x) < 4 * GetSideCover() * uor_per_mm)
									itrplus->second.x = tmpWall_range.high.x - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - diameterH - diameterV - m_RebarRadius;
							}	
							else
							{
								double ypoint = tmpWall_range.high.y - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - diameterH - diameterV - m_RebarRadius;
								if (abs(ypoint - itrplus->second.y) < 4 * GetSideCover() * uor_per_mm)
									itrplus->second.y = tmpWall_range.high.y - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - diameterH - diameterV - m_RebarRadius;

							}

							CVector3D endNormal = tmpendTypes.end.GetendNormal();
							endNormal.Negate();
							tmpendTypes.end.SetendNormal(endNormal);
							tmpendTypes.end.SetbendLen(15 * 2 * m_RebarRadius);
							if (tmpWall_range.ZLength() < 15 * 2 * m_RebarRadius)//顶部墙高度太低，需要截断
							{
								double bendradius = RebarCode::GetPinRadius(GetvecDirSize().at(m_rebarlevel), ACTIVEMODEL, false);
								if (m_rebarlevel == 2)
									tmpendTypes.end.SetbendLen(tmpWall_range.ZLength() + GetReverseCover() * uor_per_mm + 2 * m_mapLevelRadius[3] + m_RebarRadius - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - bendradius);
								else if (m_rebarlevel == 3)
									tmpendTypes.end.SetbendLen(tmpWall_range.ZLength() + GetReverseCover() * uor_per_mm + m_RebarRadius - my_wallRebarInfo.concrete.postiveCover * uor_per_mm - bendradius);
							}
							break;
						}
					}
				}

			}
		}

		return tmpendTypes.end;
	}

	PITRebarEndType LDSlabRebarAssembly::Set_endType()
	{
		double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		PITRebarEndType tmpEndType;
		tmpEndType.SetType(PIT::PITRebarEndType::kBend);
		WString sizekey = GetvecDirSize().at(m_rebarlevel);
		double lae_d = stod(sizekey.GetWCharCP()) * uor_per_mm;
		//double diameter = 2 * m_RebarRadius;
		double bendradius = RebarCode::GetPinRadius(GetvecDirSize().at(m_rebarlevel), ACTIVEMODEL, false);
		CVector3D endnormal = CVector3D::From(0, 0, 0);
		double outLae = get_lae() * lae_d / uor_per_mm;
		double insideLae = 15 * lae_d;
		if (GetvecDataExchange().at(m_rebarlevel) == 0)//下面一层钢筋处理，板底
		{
			if (m_isoutside)//外侧面
			{
				endnormal = CVector3D::From(0, 0, 1);//往上锚入
				tmpEndType.SetbendLen(outLae);
				tmpEndType.SetbendRadius(bendradius);
				tmpEndType.SetendNormal(endnormal);
			}
			else//内侧面
			{
				endnormal = CVector3D::From(0, 0, -1);//往下锚入
				tmpEndType.SetbendLen(insideLae);
				tmpEndType.SetbendRadius(bendradius);
				tmpEndType.SetendNormal(endnormal);
			}
		}
		else if (GetvecDataExchange().at(m_rebarlevel) == 2)//上面一层钢筋处理，板顶
		{
			if (m_isoutside)
			{
				endnormal = CVector3D::From(0, 0, -1);//往下锚入
				tmpEndType.SetbendLen(outLae);
				tmpEndType.SetbendRadius(bendradius);
				tmpEndType.SetendNormal(endnormal);
			}
			else
			{
				endnormal = CVector3D::From(0, 0, 1);//往上锚入
				tmpEndType.SetbendLen(insideLae);
				tmpEndType.SetbendRadius(bendradius);
				tmpEndType.SetendNormal(endnormal);
			}
		}
		return tmpEndType;
	}

	double LDSlabRebarAssembly::InsideFace_OffsetLength(DPoint3dCR RebarlineMidPt)
	{
		for(auto walleh : m_Allwalls)
		{
			DRange3d tmpWall_Range;
			mdlElmdscr_computeRange(&tmpWall_Range.low, &tmpWall_Range.high, walleh.GetElementDescrCP(), nullptr);
			if(tmpWall_Range.IsContainedXY(RebarlineMidPt))
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
				if((contid != 0) && (my_vecRebarData.size() != 0))
					diameterV = RebarCode::GetBarDiameter(my_vecRebarData[my_vecRebarData.size() - 1].rebarSize, ACTIVEMODEL);

				return ((GetSideCover() + my_wallRebarInfo.concrete.reverseCover) * Get_uor_per_mm + diameterV);
			}

		}
		return 0;
	}

	double LDSlabRebarAssembly::WallRebars_OffsetLength(DPoint3dCR RebarPt)
	{
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
			}

		}
		return 0;
	}


	bool LDSlabRebarAssembly::makeRebarCurve_G
	(
		vector<PITRebarCurve>&     rebars,
		double                  xPos,
		double                  yLen,
		double					startOffset,
		double					endOffset,
		PITRebarEndTypes&		endTypes,
		CMatrix3D const&        mat,
		bool&                tag,
		bool isTwin
	)
	{
		double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		CPoint3D  startPt;
		CPoint3D  endPt;

		//不允许为负值
	// 	if (startPt < 0)
	// 		startPt = 0;
	// 	if (endOffset < 0)
	// 		endOffset = 0;
		startPt = CPoint3D::From(xPos, 0.0, -yLen / 2.0);
		endPt = CPoint3D::From(xPos, 0.0, yLen / 2.0);


		Transform trans;
		mat.AssignTo(trans);
		TransformInfo transinfo(trans);
		EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
		eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
		//eeh.AddToModel();

		DPoint3d pt1[2];
		mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改

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
		//---end
		/*if (GetslabType() == STSLAB&& !(m_Negs.size() > 0))
		{
			if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
			{
				return false;
			}
		}
		else
		{
			if (GetPoints_G( pt1[0], pt1[1], uor_per_mm, startOffset, tag))
			{
				return false;
			}
		}*/
		//	CVector3D  Vec(pt1[0], pt1[1]);
		//	CVector3D  nowVec = Vec;
		//
		////	CVector3D  nowVec = CVector3D::kZaxis.CrossProduct(VecNegate);
		//	nowVec.Normalize();
		//
		//	nowVec.ScaleToLength(startOffset);
		//	pt1[0].Add(nowVec);
		//	nowVec.Negate();
		//	nowVec.ScaleToLength(endOffset);
		//	pt1[1].Add(nowVec);
		//	
		DVec3d vec1 = pt1[1] - pt1[0];
		DVec3d vecX1 = DVec3d::From(1, 0, 0);
		vec1.x = COMPARE_VALUES_EPS(abs(vec1.x), 0, 10) == 0 ? 0 : vec1.x;
		vec1.y = COMPARE_VALUES_EPS(abs(vec1.y), 0, 10) == 0 ? 0 : vec1.y;
		vec1.z = COMPARE_VALUES_EPS(abs(vec1.z), 0, 10) == 0 ? 0 : vec1.z;
		vec1.Normalize();
		if (vec.IsPerpendicularTo(vecX1))
		{
			if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) > 0)
			{
				tag = false;
			}
		}
		else
		{
			if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) > 0)
			{
				tag = false;
			}
		}

		// EditElementHandle eeh2;
		// GetContractibleeeh(eeh2);//获取减去保护层的端部缩小的实体

		double dSideCover = GetSideCover() * uor_per_mm;
		vector<DPoint3d> tmppts;
		Transform matrix;
		GetPlacement().AssignTo(matrix);
		vector<vector<DPoint3d>> vecPtRebars;
		vector<DPoint3d> tmpptsTmp;
		vecPtRebars.clear();
		tmpptsTmp.clear();
		if (m_pOldElm != NULL) // 与原实体相交(无孔洞)
		{
			GetIntersectPointsWithOldElm(tmpptsTmp, m_pOldElm, pt1[0], pt1[1], dSideCover, matrix);

			if (tmpptsTmp.size() > 1)
			{
				// 存在交点为两个以上的情况
				GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, pt1[0], pt1[1], m_pOldElm, dSideCover);
			}
		}

		if (tmpptsTmp.size() < 2 && vecPtRebars.size() == 0)
		{
			vector<DPoint3d> vecPt;
			vecPt.push_back(pt1[0]);
			vecPt.push_back(pt1[1]);

			vecPtRebars.push_back(vecPt);
		}

		for (size_t i = 0; i < vecPtRebars.size(); i++)
		{
			pt1[0] = vecPtRebars.at(i).at(0);
			pt1[1] = vecPtRebars.at(i).at(1);
			CVector3D  Vec(pt1[0], pt1[1]);
			CVector3D  nowVec = Vec;
			CVector3D  nowVec1 = Vec;
			nowVec1.Normalize();

			//	CVector3D  nowVec = CVector3D::kZaxis.CrossProduct(VecNegate);
			nowVec.Normalize();
			nowVec.ScaleToLength(startOffset);
			pt1[0].Add(nowVec);
			if(nowVec.x == 0 && nowVec.y == 0)
			{
				nowVec1.ScaleToLength(endOffset);
				nowVec1.Negate();
				pt1[1].Add(nowVec1);
			}
			else
			{
				nowVec.ScaleToLength(endOffset);
				nowVec.Negate();
				pt1[1].Add(nowVec);
			}
			

			if (!isTwin)//非并筋时才需要存储点信息
			{
				m_vecRebarPtsLayer.push_back(pt1[0]);
				m_vecRebarPtsLayer.push_back(pt1[1]);
			}
			GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);

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

			int num = 0;
			for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
			{
				//PITCommonTool::CPointTool::DrowOnePoint(itr->second,1,0);
				DRange3d hole_range;
				if (m_useHoleehs.size() > 0)
				{
					mdlElmdscr_computeRange(&hole_range.low, &hole_range.high, m_useHoleehs.front()->GetElementDescrCP(), nullptr);
				}
				// 取得迭代器与下一个迭代器的值
				auto tmpitr = itr;
				auto tmpitr1 = itr;
				auto tmpitrnext = ++tmpitr1;
				////////当钢筋线段太短时放弃生成curve,主要是异形板的斜墙周围出现极短的钢筋
				if (tmpitrnext->first <  1 * GetPositiveCover() * uor_per_mm)
					break;

				if ((GetvecDir().at(m_rebarlevel) == 0) && (m_xflag == 0))
				{//保存横向的信息
					// 存储正常钢筋的起始终点的x坐标
					m_XvecbegAndEnd.clear();
					m_XvecbegAndEnd.emplace_back(tmpitr->second.x);
					m_XvecbegAndEnd.emplace_back(tmpitrnext->second.x);
					// 存储正常钢筋 x方向起始终点两点的距离
					m_xdistance = tmpitrnext->first;

					//m_Nor_EndType = endTypes.end;//有孔洞时末端被设置成无端部样式，保存到m_Nor_EndType
					m_xflag = 1;
				}
				if ((m_yflag == 0) && (GetvecDir().at(m_rebarlevel) == 1))
				{//保存纵向的信息
					m_YvecbegAndEnd.clear();
					//m_vecbegAndEnd.resize(2);
					m_YvecbegAndEnd.emplace_back(tmpitr->second.y);
					m_YvecbegAndEnd.emplace_back(tmpitrnext->second.y);
					// 存储正常钢筋 y方向起始终点两点的距离
					m_ydistance = tmpitrnext->first;

					m_yflag = 1;
				}
				double xStrdistance = 0.0;
				double xEnddistance = 0.0;
				if(m_XvecbegAndEnd.size()>0)
				{
					xStrdistance = abs(tmpitr->second.x - m_XvecbegAndEnd[0]);
					xEnddistance = abs(tmpitrnext->second.x - m_XvecbegAndEnd[1]);
				}
				double yStrdistance = 0.0;
				double yEnddistance = 0.0;
				if (m_YvecbegAndEnd.size() > 0)
				{
					yStrdistance = abs(tmpitr->second.y - m_YvecbegAndEnd[0]);
					yEnddistance = abs(tmpitrnext->second.y - m_YvecbegAndEnd[1]);
				}

				//if (m_useHoleehs.size() > 0)// 由于有孔洞时将beg设置为有锚入， end为无； 便于将无锚入的样式保存到m_Nor_EndType
				//{
				//	endTypes.end = endTypes.beg;
				//}
				PITRebarEndTypes		tmpendTypes;

				PITRebarCurve rebar;
				
				/*////////begin  起点：有孔洞的时候中间断开的钢筋也要添加锚入////////*/
				int begflag = 0;// 没有设置过端部样式为0，孔洞的情况设置样式为1，其他情况设置样式为2
				// 根据不同情况设置钢筋起点的端部样式
				tmpendTypes.beg = Set_begCurvePoint(tmpendTypes, endTypes, map_pts, num, begflag, hole_range, xStrdistance, xEnddistance, yStrdistance, yEnddistance, itr, tmpitrnext);

				{
					int scanbeg_flag = 0;// 标志，如果钢筋起点在墙内，则判断是否需要设置端部样式
					int flag = 0;// 标志，特殊情况
					DPoint3d scanbeg_Ptr ;// 标志，当钢筋于墙是垂直时候,墙在钢筋上面或者下面最近的点；
					for (auto walleh : m_Allwalls)
					{//遍历所有墙，如果钢筋的起点在墙的范围内，就设置端部样式
						/*EditElementHandle downFace;
						vector<MSElementDescrP> vec_line;
						vector<MSElementDescrP> vec_linefront;
						vector<MSElementDescrP> vec_lineback;
						double tHeight = 0.0;
						ExtractFacesTool::GetFrontBackLineAndDownFace(walleh,&downFace,vec_line,vec_linefront,vec_lineback,&tHeight);*/
						
						DRange3d tmpWall_range;
						mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, walleh.GetElementDescrCP(), nullptr);
						if (itr->second.z > tmpWall_range.high.z)//墙在板上面
							scanbeg_Ptr = tmpWall_range.high;
						else if(itr->second.z < tmpWall_range.low.z)//墙在板下面
							scanbeg_Ptr = tmpWall_range.low;
						if (GetvecDir().at(m_rebarlevel) == 0)
						{
							if ((tmpWall_range.XLength() - tmpWall_range.YLength() > 10 * uor_per_mm) && tmpWall_range.IsContainedXY(itr->second))
							{//平行墙时，判断钢筋起点往上下偏移墙的距离，是否还有其他墙与其相交，（排除斜墙）
							 //如果相交说明该板在转角处，即使墙与钢筋的方向一样，也需要锚入
								int xflag = 0;
								for (auto walleh : m_Allwalls)
								{
									Dpoint3d tmpPt1 = itr->second;
									Dpoint3d tmpPt2 = itr->second;
									if (abs(tmpWall_range.XLength() - tmpWall_range.YLength()) > 10 * uor_per_mm)
									{//除斜墙外都偏移距离
										tmpPt1.y += tmpWall_range.YLength() + uor_per_mm;
										tmpPt2.y -= (tmpWall_range.YLength() + uor_per_mm);
									}
									DRange3d tmpWall_range1;
									mdlElmdscr_computeRange(&tmpWall_range1.low, &tmpWall_range1.high, walleh.GetElementDescrCP(), nullptr);
									if (abs(tmpWall_range1.XLength() - tmpWall_range1.YLength()) < 10 * uor_per_mm)
										continue;
									if (tmpWall_range1.IsContainedXY(tmpPt1) || tmpWall_range1.IsContainedXY(tmpPt2))
									{
										xflag = 1;
										flag = is_Floor(walleh);
										if (itr->second.z > tmpWall_range1.high.z)//墙在板下面
											scanbeg_Ptr = tmpWall_range1.high;
										else if (itr->second.z < tmpWall_range1.low.z)//墙在板上面
											scanbeg_Ptr = tmpWall_range1.low;
										break;
									}
								}
								if (xflag == 1)
								{
									scanbeg_flag = 1;
									break;
								}
								else
									continue;
							}
							else/* if (abs(tmpWall_range.XLength() - tmpWall_range.YLength()) < 10 * uor_per_mm)*///非平行墙处理
							{
								if (tmpWall_range.IsContainedXY(itr->second))
								{
									scanbeg_flag = 1;
									flag = 1;
									break;
								}
							}
								
						}
						else if (GetvecDir().at(m_rebarlevel) == 1)
						{
							if ((tmpWall_range.YLength() - tmpWall_range.XLength() > 10 * uor_per_mm) && tmpWall_range.IsContainedXY(itr->second))
							{
								
								int yflag = 0;
								for (auto walleh : m_Allwalls)
								{
									Dpoint3d tmpPt1 = itr->second;
									Dpoint3d tmpPt2 = itr->second;
									if (abs(tmpWall_range.XLength() - tmpWall_range.YLength()) > 10 * uor_per_mm)
									{
										tmpPt1.x += tmpWall_range.XLength() + uor_per_mm;
										tmpPt2.x -= (tmpWall_range.XLength() + uor_per_mm);
									}
									DRange3d tmpWall_range1;
									mdlElmdscr_computeRange(&tmpWall_range1.low, &tmpWall_range1.high, walleh.GetElementDescrCP(), nullptr);
									if (abs(tmpWall_range1.XLength() - tmpWall_range1.YLength()) < 10 * uor_per_mm)
										continue;
									if (tmpWall_range1.IsContainedXY(tmpPt1) || tmpWall_range1.IsContainedXY(tmpPt2))
									{
										yflag = 1;
										flag = is_Floor(walleh);
										if (itr->second.z > tmpWall_range1.high.z)//墙在板下面
											scanbeg_Ptr = tmpWall_range1.high;
										else if (itr->second.z < tmpWall_range1.low.z)//墙在板上面
											scanbeg_Ptr = tmpWall_range1.low;
										break;
									}
								}
								if (yflag == 1)
								{
									scanbeg_flag = 1;
									break;
								}
								else
									continue;
							}
							else/* if (abs(tmpWall_range.XLength() - tmpWall_range.YLength()) < 10 * uor_per_mm)*///非平行墙处理
							{
								if (tmpWall_range.IsContainedXY(itr->second))
								{
									scanbeg_flag = 1;
									flag = 1;
									break;
								}
							}
								
						}
						/*if (tmpWall_range.IsContainedXY(itr->second))
						{
							scanbeg_flag = 1;
							break;
						}*/
					}
					/*if ((scanbeg_flag == 0) && (m_useHoleehs.size() > 0) && (begflag != 1))
					{
						tmpendTypes.beg = m_Nor_EndType;
						begflag = 1;
					}*/
					if ((scanbeg_flag == 1) && (begflag != 1))
					{
						if (m_endtypes == 2)
						{
							tmpendTypes.beg = endTypes.end;
							begflag = 2;
						}
						else if (m_endtypes == 0)
						{
							//自己设置端部样式
							tmpendTypes.beg = Set_endType();
							if (flag)
							{
								CVector3D vec = CVector3D::From(0, 0, scanbeg_Ptr.z - itr->second.z);
								vec.Normalize();
								tmpendTypes.beg.SetendNormal(vec);
								WString sizekey = GetvecDirSize().at(m_rebarlevel);
								double lae_d = stod(sizekey.GetWCharCP()) * uor_per_mm;
								//double diameter = 2 * m_RebarRadius;
								double outLae = get_lae() * lae_d / uor_per_mm;
								double insideLae = 15 * lae_d;
								if (COMPARE_VALUES_EPS(vec.z, -1, 0.1) == 0)//锚入方向朝下
								{
									if (GetvecDataExchange().at(m_rebarlevel) == 0)//下面一层钢筋处理，板底
									{
										tmpendTypes.beg.SetbendLen(insideLae);
									}
									else if (GetvecDataExchange().at(m_rebarlevel) == 2)//上
									{
										tmpendTypes.beg.SetbendLen(outLae);
									}
								}
								else if (COMPARE_VALUES_EPS(vec.z, 1, 0.1) == 0)//锚入方向朝上
								{
									if (GetvecDataExchange().at(m_rebarlevel) == 0)//下面一层钢筋处理，板底
									{
										tmpendTypes.beg.SetbendLen(outLae);
									}
									else if (GetvecDataExchange().at(m_rebarlevel) == 2)//上
									{
										tmpendTypes.beg.SetbendLen(insideLae);
									}
								}
							}
							begflag = 2;
						}
					}
				}
				//for (auto walleh : m_Allwalls)
				//{//遍历所有墙，如果墙的方向与钢筋方向相同，将端部样式设为无
				//	DRange3d tmpWall_range;
				//	mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, walleh.GetElementDescrCP(), nullptr);
				//	if (tmpWall_range.IsContainedXY(itr->second))
				//	{
				//		if (GetvecDir().at(m_rebarlevel) == 0)
				//		{
				//			if (tmpWall_range.XLength() > tmpWall_range.YLength())
				//			{
				//				tmpendTypes.beg.SetType(PIT::PITRebarEndType::kNone);
				//				begflag = 1;
				//				break;
				//			}
				//		}
				//		else if (GetvecDir().at(m_rebarlevel) == 1)
				//		{
				//			if (tmpWall_range.YLength() > tmpWall_range.XLength())
				//			{
				//				tmpendTypes.beg.SetType(PIT::PITRebarEndType::kNone);
				//				begflag = 1;
				//				break;
				//			}
				//		}
				//		break;
				//	}
				//}
				/*////////end  起点：有孔洞的时候中间断开的钢筋也要添加锚入////////*/
				if ((pt1[0].Distance(itr->second) < 10) && (begflag == 0))
				{
					tmpendTypes.beg = endTypes.beg;
				}
				/*if ((m_endtypes == 0) && (begflag == 0) && (m_useHoleehs.size() > 0) )
				{
					tmpendTypes.beg = m_Nor_EndType;
				}*/
				if((endTypes.beg.GetType() == PIT::PITRebarEndType::kNone) && (tmpendTypes.beg.GetType() == PIT::PITRebarEndType::kBend) && (begflag == 2))
				{

					//if(GetvecDataExchange().at(m_rebarlevel) == 0)//板底
					//{
						if(m_isoutside)//外侧面
						{
							if (GetvecDir().at(m_rebarlevel) == 1)//y
							{
								itr->second.y += m_RebarRadius;
							}
							else
							{
								itr->second.x += m_RebarRadius;
							}
						}
						else//内侧面
						{
							if (GetvecDir().at(m_rebarlevel) == 1)//y
							{
								itr->second.y += (4 * m_mapLevelRadius[2] + m_RebarRadius);
							}
							else
							{
								itr->second.x += (4 * m_mapLevelRadius[2] + m_RebarRadius);
							}
						}
						//if(GetvecDir().at(m_rebarlevel) == 1)//y
						//{
						//	itr->second.y += (4 * m_mapLevelRadius[2] + m_RebarRadius);
						//}
						//else
						//{
						//	itr->second.x += (4 * m_mapLevelRadius[2] + m_RebarRadius);
						//}
					//}
					//else//板顶
					//{
					//	if (m_isoutside)//外侧面
					//	{
					//		if (GetvecDir().at(m_rebarlevel) == 1)//y
					//		{
					//			itr->second.y += m_RebarRadius;
					//		}
					//		else
					//		{
					//			itr->second.x += m_RebarRadius;
					//		}
					//	}
					//	else//内侧面
					//	{
					//		if (GetvecDir().at(m_rebarlevel) == 1)//y
					//		{
					//			itr->second.y += (4 * m_mapLevelRadius[2] + m_RebarRadius);
					//		}
					//		else
					//		{
					//			itr->second.x += (4 * m_mapLevelRadius[2] + m_RebarRadius);
					//		}
					//	}
						//if (GetvecDir().at(m_rebarlevel) == 1)//y
						//{
						//	itr->second.y += m_RebarRadius;
						//}
						//else
						//{
						//	itr->second.x += m_RebarRadius;
						//}
					
				}
				if (tmpendTypes.beg.GetType() != PIT::PITRebarEndType::kNone && (begflag != 1))
				{//有些特殊版没有遇到墙也会锚固
					int tmpflagbeg = 0;
					for (auto walleh : m_Allwalls)
					{//遍历所有墙，钢筋起点没有在墙范围内，将端部样式设为无
						DRange3d tmpWall_range;
						mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, walleh.GetElementDescrCP(), nullptr);
						if (tmpWall_range.IsContainedXY(itr->second))
						{
							tmpflagbeg = 1;
							break;
						}
					}
					if (!tmpflagbeg)
					{
						tmpendTypes.beg.SetType(PIT::PITRebarEndType::kNone);
					}
				}
				/*判断钢筋锚入出去之后是否在构件中，如果不在则反向*/
				Dpoint3d ExtendPt = itr->second;
				CVector3D NormalVec = tmpendTypes.beg.GetendNormal();
				NormalVec.Normalize();
				auto length = tmpendTypes.beg.GetbendLen();
				CVector3D vec_Up = CVector3D::From(0, 0, 1);
				CVector3D vec_Down = CVector3D::From(0, 0, -1);
				if (COMPARE_VALUES_EPS(NormalVec, vec_Up, 0.01) == 0)
					ExtendPt.z += tmpendTypes.beg.GetbendLen();
				else if(COMPARE_VALUES_EPS(NormalVec, vec_Down, 0.01) == 0)
					ExtendPt.z -= tmpendTypes.beg.GetbendLen();
				auto move_point([&](CVector3D vec, Dpoint3d &point, double length) -> void {
					vec.Normalize();          // 归一化向量
					vec.ScaleToLength(length); // 调整向量长度
					point.Add(vec);           // 将向量叠加到点上
				});
				if (!ISPointInHoles(m_ScanedAllWallsandFloor, ExtendPt))
				{
					Dpoint3d  temp_pt = itr->second;
					NormalVec.Negate();
					double bendLen = tmpendTypes.beg.GetbendLen() - tmpendTypes.beg.GetbendRadius();
					move_point(NormalVec, temp_pt, length);
					if (!ISPointInHoles(m_ScanedAllWallsandFloor, temp_pt))
					{
						NormalVec.Negate();
						temp_pt = itr->second;
						move_point(NormalVec, temp_pt, bendLen);
						if (!ISPointInHoles(m_ScanedAllWallsandFloor, temp_pt))
						{
							temp_pt = itr->second;
							NormalVec.Negate();
							move_point(NormalVec, temp_pt, length);
							
							if (!ISPointInHoles(m_ScanedAllWallsandFloor, temp_pt))
							{
								NormalVec.Negate();
								bendLen-= tmpendTypes.beg.GetbendRadius();
								tmpendTypes.beg.SetbendLen(bendLen);
							}
							else
								tmpendTypes.beg.SetbendLen(bendLen);
						}
					}
					else
					tmpendTypes.beg.SetbendLen(bendLen);
				}
				else 
				{
					EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());
					EditElementHandle * ptr = &testeeh;
					if (ISPointInElement(ptr, ExtendPt))
					{
						Dpoint3d  temp_pt = itr->second;
						NormalVec.Negate();
						move_point(NormalVec, temp_pt, length);

						if (!ISPointInHoles(m_ScanedAllWallsandFloor, temp_pt))
						{
							NormalVec.Negate();
						}
					}
				}
				tmpendTypes.beg.SetendNormal(NormalVec);
				tmpendTypes.beg.SetptOrgin(itr->second);
				RebarVertexP vex;
				vex = &rebar.PopVertices().NewElement();
				vex->SetIP(itr->second);
				vex->SetType(RebarVertex::kStart);
				map<int, DPoint3d>::iterator itrplus = ++itr;
				if (itrplus == map_pts.end())
				{
					break;
				}
				/*////////begin  终点：有孔洞的时候中间断开的钢筋也要添加锚入////////*/
				int endflag = 0;// 没有设置过端部样式为0，孔洞的情况设置样式为1，其他情况设置样式为2
				tmpendTypes.end = Set_endCurvePoint(tmpendTypes, endTypes, map_pts, num, endflag, hole_range, xStrdistance, xEnddistance, yStrdistance, yEnddistance, itrplus);
				
				{//遍历断点上下是否有墙，没有墙的端部设置为空，有墙的设置端部样式
					int scanflag = 0;
					int flag = 0;// 标志，特殊情况
					DPoint3d scanbeg_Ptr;// 标志，当钢筋于墙是垂直时候,墙在钢筋上面或者下面最近的点；
					for (auto walleh : m_Allwalls)
					{
						DRange3d tmpWall_range;
						mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, walleh.GetElementDescrCP(), nullptr);
						if (itr->second.z > tmpWall_range.high.z)//墙在板下面
							scanbeg_Ptr = tmpWall_range.high;
						else if (itr->second.z < tmpWall_range.low.z)//墙在板上面
							scanbeg_Ptr = tmpWall_range.low;
						if (GetvecDir().at(m_rebarlevel) == 0)
						{
							if ((tmpWall_range.XLength() > tmpWall_range.YLength()) && tmpWall_range.IsContainedXY(itrplus->second))
							{//平行墙时，判断钢筋起点往上下偏移墙的距离，是否还有其他墙与其相交，（排除斜墙）
							 //如果相交说明该板在转角处，即使墙与钢筋的方向一样，也需要锚入
								int xflag = 0;
								for (auto walleh : m_Allwalls)
								{
									Dpoint3d tmpPt1 = itr->second;
									Dpoint3d tmpPt2 = itr->second;
									if (abs(tmpWall_range.XLength() - tmpWall_range.YLength()) > 10 * uor_per_mm)
									{
										tmpPt1.y += tmpWall_range.YLength() + uor_per_mm;
										tmpPt2.y -= (tmpWall_range.YLength() + uor_per_mm);
									}
									DRange3d tmpWall_range1;
									mdlElmdscr_computeRange(&tmpWall_range1.low, &tmpWall_range1.high, walleh.GetElementDescrCP(), nullptr);
									if(abs(tmpWall_range1.XLength() - tmpWall_range1.YLength()) < 10 * uor_per_mm)
										continue;
									if (tmpWall_range1.IsContainedXY(tmpPt1) || tmpWall_range1.IsContainedXY(tmpPt2))
									{
										xflag = 1;
										flag = is_Floor(walleh);
										if (itr->second.z > tmpWall_range1.high.z)//墙在板下面
											scanbeg_Ptr = tmpWall_range1.high;
										else if (itr->second.z < tmpWall_range1.low.z)//墙在板上面
											scanbeg_Ptr = tmpWall_range1.low;
										break;
									}
								}
								if (xflag == 1)
								{
									scanflag = 1;
									break;
								}
								else
									continue;
							}
							else/* if (abs(tmpWall_range.XLength() - tmpWall_range.YLength()) < 10 * uor_per_mm)*///斜墙处理
							{
								if (tmpWall_range.IsContainedXY(itrplus->second))
								{
									scanflag = 1;
									flag = 1;
									break;
								}
							}
								
						}
						else if (GetvecDir().at(m_rebarlevel) == 1)
						{
							if ((tmpWall_range.YLength() > tmpWall_range.XLength()) && tmpWall_range.IsContainedXY(itrplus->second))
							{
								
								int yflag = 0;
								for (auto walleh : m_Allwalls)
								{
									Dpoint3d tmpPt1 = itr->second;
									Dpoint3d tmpPt2 = itr->second;
									if (abs(tmpWall_range.XLength() - tmpWall_range.YLength()) > 10 * uor_per_mm)
									{
										tmpPt1.x += tmpWall_range.XLength() + uor_per_mm;
										tmpPt2.x -= (tmpWall_range.XLength() + uor_per_mm);
									}
									DRange3d tmpWall_range1;
									mdlElmdscr_computeRange(&tmpWall_range1.low, &tmpWall_range1.high, walleh.GetElementDescrCP(), nullptr);
									if (abs(tmpWall_range1.XLength() - tmpWall_range1.YLength() < 10 * uor_per_mm))
										continue;
									if (tmpWall_range1.IsContainedXY(tmpPt1) || tmpWall_range1.IsContainedXY(tmpPt2))
									{
										yflag = 1;
										flag = is_Floor(walleh);
										if (itr->second.z > tmpWall_range1.high.z)//墙在板下面
											scanbeg_Ptr = tmpWall_range1.high;
										else if (itr->second.z < tmpWall_range1.low.z)//墙在板上面
											scanbeg_Ptr = tmpWall_range1.low;
										break;
									}
								}
								if (yflag == 1)
								{
									scanflag = 1;
									break;
								}
								else
									continue;
							}
							else/* if (abs(tmpWall_range.XLength() - tmpWall_range.YLength()) < 10 * uor_per_mm)*///斜墙处理
							{
								if (tmpWall_range.IsContainedXY(itrplus->second))
								{
									scanflag = 1;
									flag = 1;
									break;
								}
							}
						}
						/*if (tmpWall_range.IsContainedXY(itrplus->second))
						{
							scanflag = 1;
							break;
						}*/
					}
					/*if ((scanflag == 0) && (m_useHoleehs.size() > 0) && (endflag != 1))
					{
						tmpendTypes.end = m_Nor_EndType;
						endflag = 1;
					}
					else */if ((scanflag == 1) && (endflag != 1))
					{
						if (m_endtypes == 1)
						{
							tmpendTypes.end = endTypes.beg;
							endflag = 2;
						}
						else if (m_endtypes == 0)
						{
							//自己设置端部样式
							tmpendTypes.end = Set_endType();
							if(flag)
							{
								CVector3D vec = CVector3D::From(0, 0, scanbeg_Ptr.z - itr->second.z);
								vec.Normalize();
								tmpendTypes.end.SetendNormal(vec);
								WString sizekey = GetvecDirSize().at(m_rebarlevel);
								double lae_d = stod(sizekey.GetWCharCP()) * uor_per_mm;
								//double diameter = 2 * m_RebarRadius;
								double outLae = get_lae() * lae_d / uor_per_mm;
								double insideLae = 15 * lae_d;
								if (COMPARE_VALUES_EPS(vec.z, -1, 0.1) == 0)//锚入方向朝下
								{
									if (GetvecDataExchange().at(m_rebarlevel) == 0)//下面一层钢筋处理，板底
									{
										tmpendTypes.end.SetbendLen(insideLae);
									}
									else if (GetvecDataExchange().at(m_rebarlevel) == 2)//上
									{
										tmpendTypes.end.SetbendLen(outLae);
									}
								}
								else if (COMPARE_VALUES_EPS(vec.z, 1, 0.1) == 0)//锚入方向朝上
								{
									if (GetvecDataExchange().at(m_rebarlevel) == 0)//下面一层钢筋处理，板底
									{
										tmpendTypes.end.SetbendLen(outLae);
									}
									else if (GetvecDataExchange().at(m_rebarlevel) == 2)//上
									{
										tmpendTypes.end.SetbendLen(insideLae);
									}
								}
							}
							endflag = 2;
						} 
						
					}
				}
				//for (auto walleh : m_Allwalls)
				//{//遍历所有墙，如果墙的方向与钢筋方向相同，将端部样式设为无
				//	DRange3d tmpWall_range;
				//	mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, walleh.GetElementDescrCP(), nullptr);
				//	if (tmpWall_range.IsContainedXY(itr->second))
				//	{
				//		if (GetvecDir().at(m_rebarlevel) == 0)
				//		{
				//			if (tmpWall_range.XLength() > tmpWall_range.YLength())
				//			{
				//				tmpendTypes.end.SetType(PIT::PITRebarEndType::kNone);
				//				endflag = 1;
				//				break;
				//			}
				//		}
				//		else if (GetvecDir().at(m_rebarlevel) == 1)
				//		{
				//			if (tmpWall_range.YLength() > tmpWall_range.XLength())
				//			{
				//				tmpendTypes.end.SetType(PIT::PITRebarEndType::kNone);
				//				endflag = 1;
				//				break;
				//			}
				//		}
				//		break;
				//	}
				//}
				/*////////end  终点：有孔洞的时候中间断开的钢筋也要添加锚入////////*/
				if ((pt1[1].Distance(itrplus->second) < 10) && (endflag == 0))
				{
					tmpendTypes.end = endTypes.end;
				}
				/*if ((m_endtypes == 0) && (endflag == 0) && (m_useHoleehs.size() > 0))
				{
					tmpendTypes.end = m_Nor_EndType;
				}*/
				
				if ((endTypes.end.GetType() == PIT::PITRebarEndType::kNone) && (tmpendTypes.end.GetType() == PIT::PITRebarEndType::kBend) && (endflag == 2))
				{

					if (!m_isoutside)//内侧面
					{
						if (GetvecDir().at(m_rebarlevel) == 1)//y
						{
							itrplus->second.y -= (4 * m_mapLevelRadius[2] + m_RebarRadius);
						}
						else
						{
							itrplus->second.x -= (4 * m_mapLevelRadius[2] + m_RebarRadius);
						}
					}
					else//外侧面
					{
						if (GetvecDir().at(m_rebarlevel) == 1)//y
						{
							itrplus->second.y -= m_RebarRadius;
						}
						else
						{
							itrplus->second.x -= m_RebarRadius;
						}
					}
				}
				if (tmpendTypes.end.GetType() != PIT::PITRebarEndType::kNone && (endflag != 1))
				{//有些特殊版没有遇到墙也会锚固
					int tmpflagend = 0;
					for (auto walleh : m_Allwalls)
					{//遍历所有墙，如果钢筋终点不在墙范围内，将端部样式设为无
						DRange3d tmpWall_range;
						mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, walleh.GetElementDescrCP(), nullptr);
						if (tmpWall_range.IsContainedXY(itrplus->second))
						{
							tmpflagend = 1;
							break;
						}
					}
					if (!tmpflagend)
					{
						tmpendTypes.end.SetType(PIT::PITRebarEndType::kNone);
					}
				}
				/*判断钢筋锚入出去之后是否在构件中，如果不在则反向*/
				Dpoint3d ExtendPt_end = itrplus->second; 
				CVector3D NormalVec_end = tmpendTypes.end.GetendNormal();
				NormalVec_end.Normalize();
				auto length_end = tmpendTypes.end.GetbendLen();
				CVector3D vec_str_Up = CVector3D::From(0, 0, 1);
				CVector3D vec_str_Down = CVector3D::From(0, 0, -1);

				//PITCommonTool::CPointTool::DrowOnePoint(ExtendPt_end, 1, 1);//绿
	
				if (COMPARE_VALUES_EPS(NormalVec_end, vec_str_Up, 0.01) == 0)
					ExtendPt_end.z += tmpendTypes.end.GetbendLen();
				else if (COMPARE_VALUES_EPS(NormalVec_end, vec_str_Down, 0.01) == 0)
					ExtendPt_end.z -= tmpendTypes.end.GetbendLen();

				if (!ISPointInHoles(m_ScanedAllWallsandFloor, ExtendPt_end))//是否端点锚出
				{
					Dpoint3d  temp_pt = itrplus->second;
					double bendLen = tmpendTypes.end.GetbendLen() - tmpendTypes.end.GetbendRadius();//回缩一个保护层距离
					move_point(NormalVec_end, temp_pt, length_end);
					if (!ISPointInHoles(m_ScanedAllWallsandFloor, temp_pt))
					{
						NormalVec_end.Negate();
						temp_pt = itrplus->second;
						move_point(NormalVec_end, temp_pt, length_end);
						if (!ISPointInHoles(m_ScanedAllWallsandFloor, temp_pt))//方向取反再判断一次
						{
							temp_pt = itrplus->second;
							NormalVec_end.Negate();
							move_point(NormalVec_end, temp_pt, bendLen);

							if (!ISPointInHoles(m_ScanedAllWallsandFloor, temp_pt))
							{
								NormalVec_end.Negate();
								bendLen -= tmpendTypes.end.GetbendRadius();
								tmpendTypes.end.SetbendLen(bendLen);
							}
							else
								tmpendTypes.end.SetbendLen(bendLen);
						}
					}
					else
						tmpendTypes.end.SetbendLen(bendLen);
				}
				else
				{
					EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());
					EditElementHandle * ptr = &testeeh;
					if (ISPointInElement(ptr, ExtendPt_end))
					{
						Dpoint3d  temp_pt = itrplus->second;
						NormalVec_end.Negate();
						move_point(NormalVec_end, temp_pt, length_end);

						if (!ISPointInHoles(m_ScanedAllWallsandFloor, temp_pt))
						{
							NormalVec_end.Negate();
						}
					}
				}

				tmpendTypes.end.SetendNormal(NormalVec_end);
				tmpendTypes.end.SetptOrgin(itrplus->second);

				vex = &rebar.PopVertices().NewElement();
				vex->SetIP(itrplus->second);
				vex->SetType(RebarVertex::kEnd);

				rebar.EvaluateEndTypes(tmpendTypes);
				//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
				rebars.push_back(rebar);
				num++;
			}

		}
		//rebar.DoMatrix(mat);
		return true;
	}

	RebarSetTag* LDSlabRebarAssembly::MakeRebars
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		double              xLen,
		double              width,
		double              spacing,
		double              startOffset,
		double              endOffset,
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
		vector<CVector3D> const& vecEndNormal,
		CMatrix3D const&    mat,
		TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
		bool				bTwinbarLevel,
		int level,
		int grade,
		int DataExchange,
		DgnModelRefP        modelRef,
		/*int rebarColor,*/
		int rebarLineStyle,
		int rebarWeight,
		int index
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
		double startbendRadius, endbendRadius;
		double startbendLen, startbendLenTb, endbendLen, endbendLenTb;
		double begStraightAnchorLen, endStraightAnchorLen;
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
			startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//乘以了100
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
			startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//乘以了100
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
			startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//乘以了100
		}
		break;
		case 8:	//用户
			endTypeStart.SetType(RebarEndType::kCustom);
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
			endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//乘以了100
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
			endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//乘以了100
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
			endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//乘以了100
		}

		break;
		case 8:	//用户
			endTypeEnd.SetType(RebarEndType::kCustom);
			break;
		default:
			break;
		}

		double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
		m_RebarRadius = diameter / 2;
		double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
		double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);	//乘以了30
		double adjustedXLen, adjustedSpacing;
		int numRebar = 0;

		double leftSideCov, rightSideCov, allSideCov;
		leftSideCov = GetSideCover()*uor_per_mm;
		rightSideCov = GetSideCover()*uor_per_mm;
		allSideCov = leftSideCov + rightSideCov;


		if (twinBarInfo.hasTwinbars)	//并筋
			adjustedXLen = xLen - allSideCov - diameter - diameterTb /*- startOffset - endOffset*/;
		else
			adjustedXLen = xLen - allSideCov - diameter /*- startOffset - endOffset*/;
		//	double adjustedXLen = xLen - 2.0 * GetSideCover()*uor_per_mm - diameter - startOffset - endOffset;

		if (bTwinbarLevel)				//并筋层钢筋条数
		{
			numRebar = (int)floor(adjustedXLen / (spacing * (twinBarInfo.interval + 1)) + 0.5) + 1;
			int numRebar1 = (int)floor(adjustedXLen / spacing + 0.5) + 1;
			adjustedSpacing = spacing;
			if (numRebar1 > 1)
			{
				adjustedSpacing = adjustedXLen / (numRebar1 - 1);	//非并筋层平均间距
				adjustedSpacing *= (twinBarInfo.interval + 1);		//并筋层实际间距需乘以钢筋间隔
			}
		}
		else
		{
			numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;
			//if(m_outsidef.posnum > 1)
			//	m_rebarSetNum += numRebar;
			adjustedSpacing = spacing;
			if (numRebar > 1)
				adjustedSpacing = adjustedXLen / (numRebar - 1);
			//if(m_endDelete && m_strDelete)
			//	m_adjustedSpacing = adjustedSpacing;
		}
		double xPos = startOffset;
		if (bTwinbarLevel)				//并筋层需偏移一个钢筋的距离
		{
			xPos += diameter * 0.5;
			if (diameterTb > diameter)
			{
				double radius = diameter * 0.5;
				double radiusTb = diameterTb * 0.5;
				double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radiusTb - radius)*(radiusTb - radius), 0.5) - radius;
				xPos += dOffset;
			}
			else if (diameterTb < diameter)
			{
				double radius = diameter * 0.5;
				double radiusTb = diameterTb * 0.5;
				double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radius - radiusTb)*(radius - radiusTb), 0.5) - radius;
				xPos += dOffset;
			}
			else
			{
				xPos += diameter * 0.5;
			}
		}
		vector<PITRebarCurve>     rebarCurvesNum;
		int j = 0;
		double endTypeStartOffset = endType[0].offset * uor_per_mm;
		double endTypEendOffset = endType[1].offset * uor_per_mm;
		if (endType[0].endType != 0 && endType[0].endType != 7)	//端部弯曲时额外偏移钢筋半径
			endTypeStartOffset += diameter * 0.5;
		if (endType[1].endType != 0 && endType[1].endType != 7)	//端部弯曲时额外偏移钢筋半径
			endTypEendOffset += diameter * 0.5;

		//Z形板拆分开的板的钢筋往内缩进长度
		if (m_isMidFloor  && m_Allwalls.size() == 3)
		{
			{
				if (m_sidetype == SideType::Out)//外侧面钢筋处理
				{
					DRange3d floor_Range;
					mdlElmdscr_computeRange(&floor_Range.low, &floor_Range.high, m_pOldElm->GetElementDescrCP(), nullptr);
					DPoint3d StrmidPt = floor_Range.low;
					if (GetvecDir().at(m_rebarlevel) == 0)
					{
						StrmidPt.x += 100 * uor_per_mm;
						StrmidPt.y = (floor_Range.low.y + floor_Range.high.y) / 2;
					}
					else
					{
						StrmidPt.y += 100 * uor_per_mm;
						StrmidPt.x = (floor_Range.low.x + floor_Range.high.x) / 2;
					}

					double Stroffset = WallRebars_OffsetLength(StrmidPt);

					DPoint3d EndmidPt = floor_Range.high;
					if (GetvecDir().at(m_rebarlevel) == 0)
					{
						EndmidPt.x -= 100 * uor_per_mm;
						EndmidPt.y = (floor_Range.low.y + floor_Range.high.y) / 2;
					}
					else
					{
						EndmidPt.y -= 100 * uor_per_mm;
						EndmidPt.x = (floor_Range.low.x + floor_Range.high.x) / 2;
					}
					
					double Endoffset = WallRebars_OffsetLength(EndmidPt);

					if (endTypeStartOffset != 0)
					{
						if (Stroffset == 0)
							endTypeStartOffset = endTypeStartOffset + 4 * m_RebarRadius;
						else
							endTypeStartOffset = endTypeStartOffset + Stroffset;
					}
					if (endTypEendOffset != 0)
					{
						if (Endoffset == 0)
							endTypEendOffset = endTypEendOffset + 4 * m_RebarRadius;
						else
							endTypEendOffset = endTypEendOffset + Endoffset;
					}
				}
				else if (m_sidetype == SideType::In)//内侧面钢筋处理
				{
					endTypeStartOffset = endTypeStartOffset + m_outsidef.calLen;
					endTypEendOffset = endTypEendOffset + m_outsidef.calLen;
				}
			}
		}
		else
		{
			if (m_sidetype == SideType::Out)//外侧面钢筋处理
			{
				endTypeStartOffset = endTypeStartOffset + m_outsidef.calLen;
				endTypEendOffset = endTypEendOffset + m_outsidef.calLen;
			}
			if (m_sidetype == SideType::In)//内侧面钢筋处理
			{
				DRange3d floor_Range;
				mdlElmdscr_computeRange(&floor_Range.low, &floor_Range.high, m_pOldElm->GetElementDescrCP(), nullptr);
				
				DPoint3d StrmidPt = floor_Range.low;
				if (GetvecDir().at(m_rebarlevel) == 0)
				{
					StrmidPt.x += 100 * uor_per_mm;
					StrmidPt.y = (floor_Range.low.y + floor_Range.high.y) / 2;
				}
				else
				{
					StrmidPt.y += 100 * uor_per_mm;
					StrmidPt.x = (floor_Range.low.x + floor_Range.high.x) / 2;
				}
				
				double Stroffset = WallRebars_OffsetLength(StrmidPt);

				DPoint3d EndmidPt = floor_Range.high;
				if (GetvecDir().at(m_rebarlevel) == 0)
				{
					EndmidPt.x -= 100 * uor_per_mm;
					EndmidPt.y = (floor_Range.low.y + floor_Range.high.y) / 2;
				}
				else
				{
					EndmidPt.y -= 100 * uor_per_mm;
					EndmidPt.x = (floor_Range.low.x + floor_Range.high.x) / 2;
				}

				double Endoffset = WallRebars_OffsetLength(EndmidPt);

				if (endTypeStartOffset != 0)
				{
					if (Stroffset == 0)
						endTypeStartOffset = endTypeStartOffset + m_insidef.calLen;
					else
						endTypeStartOffset = endTypeStartOffset + Stroffset;
				}
				if (endTypEendOffset != 0)
				{
					if (Endoffset == 0)
						endTypEendOffset = endTypEendOffset + m_insidef.calLen;
					else
						endTypEendOffset = endTypEendOffset + Endoffset;
				}
				/*else
				{
					endTypeStartOffset = endTypeStartOffset + m_insidef.calLen;
					endTypEendOffset = endTypEendOffset + m_insidef.calLen;
				}*/

			}
		}
		


		PITRebarEndType start, end;
		start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
		start.Setangle(endType[0].rotateAngle);
		start.SetstraightAnchorLen(begStraightAnchorLen);
		if (bTwinbarLevel)				//并筋层
		{
			start.SetbendLen(startbendLenTb);
			start.SetbendRadius(bendRadiusTb);
		}
		else
		{
			start.SetbendLen(startbendLen - startbendRadius);
			start.SetbendRadius(startbendRadius);
		}
		start.SetendNormal(vecEndNormal[0]);

		end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
		end.Setangle(endType[1].rotateAngle);
		end.SetstraightAnchorLen(endStraightAnchorLen);
		if (bTwinbarLevel)				//并筋层
		{
			end.SetbendLen(endbendLenTb);
			end.SetbendRadius(bendRadiusTb);
		}
		else
		{
			end.SetbendLen(endbendLen - endbendRadius);
			end.SetbendRadius(endbendRadius);
		}
		end.SetendNormal(vecEndNormal[1]);

		/*if (!m_flag)
		{
			if ((endType[0].endType != 0) || (endType[1].endType != 0))
			{
				if (start.GetType() != 0)
					m_Vertical_EndType = start;
				else if (end.GetType() != 0)
					m_Vertical_EndType = end;
				m_flag++;
			}
			
		}*/
		PITRebarEndTypes   endTypes = { start, end };
		if (m_isMidFloor && m_Allwalls.size() == 3)
		{
			if (m_isIndownWall)
			{
				if (m_isXdir)
				{
					if (GetvecDir().at(m_rebarlevel) == 0)
					{
						CVector3D endNormal(0, 0, 1);
						endTypes.beg.SetendNormal(endNormal);
						endTypes.end.SetendNormal(endNormal);
					}
				}
				else
				{
					if (GetvecDir().at(m_rebarlevel) == 1)
					{
						CVector3D endNormal(0, 0, 1);
						endTypes.beg.SetendNormal(endNormal);
						endTypes.end.SetendNormal(endNormal);
					}
				}
			}
			else
			{
				if (m_isXdir)
				{
					if (GetvecDir().at(m_rebarlevel) == 1)
					{
						CVector3D endNormal(0, 0, 1);
						endTypes.beg.SetendNormal(endNormal);
						endTypes.end.SetendNormal(endNormal);
					}
				}
				else
				{
					if (GetvecDir().at(m_rebarlevel) == 0)
					{
						CVector3D endNormal(0, 0, 1);
						endTypes.beg.SetendNormal(endNormal);
						endTypes.end.SetendNormal(endNormal);
					}
				}
			}
		}
		for (int i = 0; i < numRebar; i++)//钢筋属性
		{
			
			bool tag;
			vector<PITRebarCurve>     rebarCurves;
			tag = true;
			makeRebarCurve_G(rebarCurves, xPos, width, endTypeStartOffset, endTypEendOffset, endTypes, mat, tag, bTwinbarLevel);
			xPos += adjustedSpacing;
			isTemp = false;
			if (!tag)
				continue;
			if (m_sidetype == SideType::Out)//外侧面钢筋处理
			{
				if (i == 0 && m_strDelete)
					continue;
				if (i == numRebar - 1 && m_endDelete)
					continue;
			}
			std::vector<double> lengths;
			for (const auto& it : m_insidef.pos)
			{
				double length = it.end - it.str;
				lengths.push_back(length); // 收集所有长度
			}
			// 排序并去重
			std::sort(lengths.begin(), lengths.end(), std::greater<double>());
			lengths.erase(std::unique(lengths.begin(), lengths.end()), lengths.end()); // 去重
			double secondMaxLength = (lengths.size() > 1) ? lengths[1] : 0.0; // 获取第二大的长度

			rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
			if (i != 0 && (i != (numRebar - 1)))
				rebarCurves.clear();
			if ((m_sidetype == SideType::In && rebarCurves.size() != 0) || (m_sidetype == SideType::Out && !m_strDelete && !m_endDelete && rebarCurves.size() != 0))//内侧面钢筋处理
			{
				if ((i == 0 && m_insidef.strval && m_sidetype == SideType::In ) || ( i == 0 && m_outsidef.strval && m_sidetype == SideType::Out ))
				{
					//在反方向添加一根钢筋
					auto  PtStr = rebarCurves.front().GetVertices().At(0).GetIP();
					auto  PtEnd = rebarCurves.front().GetVertices().At(1).GetIP();
					rebarCurves.clear();
					DPoint3d midPt = PtStr;
					midPt.Add(PtEnd);
					midPt.Scale(0.5);
					if (GetvecDir().at(m_rebarlevel) == 0)//x
						midPt.y += 2 * GetSideCover() * uor_per_mm;
					else//y
						midPt.x -= 2 * GetSideCover() * uor_per_mm;
					double inside_Offset = InsideFace_OffsetLength(midPt) + 2 * m_RebarRadius;//墙和板的保护层距离 + 墙的最边上的钢筋直径加上自身直径
					double tmppos = xPos;
					tmppos = tmppos - inside_Offset - adjustedSpacing;
					/*double tmppos = xPos;
					tmppos = tmppos - m_insidef.strval - adjustedSpacing;*/
					makeRebarCurve_G(rebarCurves, tmppos, width, endTypeStartOffset, endTypEendOffset, endTypes, mat, tag, bTwinbarLevel);
					rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
				}
				else if (( i == numRebar - 1 && m_insidef.endval && m_sidetype == SideType::In ) || (i == numRebar - 1 && m_outsidef.endval && m_sidetype == SideType::Out))
				{
					//在反方向添加一根钢筋
					auto  PtStr = rebarCurves.front().GetVertices().At(0).GetIP();
					auto  PtEnd = rebarCurves.front().GetVertices().At(1).GetIP();
					rebarCurves.clear();
					DPoint3d midPt = PtStr;
					midPt.Add(PtEnd);
					midPt.Scale(0.5);
					if (GetvecDir().at(m_rebarlevel) == 0)//x
						midPt.y -= 2 * GetSideCover() * uor_per_mm;
					else//y
						midPt.x += 2 * GetSideCover() * uor_per_mm;
					double inside_Offset = InsideFace_OffsetLength(midPt) + 2 * m_RebarRadius;//墙和板的保护层距离 + 墙的最边上的钢筋直径加上自身直径
					double tmppos = xPos;
					tmppos = tmppos + inside_Offset - adjustedSpacing;
					/*double tmppos = xPos;
					tmppos = tmppos + m_insidef.endval - adjustedSpacing;*/
					makeRebarCurve_G(rebarCurves, tmppos, width, endTypeStartOffset, endTypEendOffset, endTypes, mat, tag, bTwinbarLevel);
					rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
				}
			}


		}//rebarset里面rebarelement初步建立完成
		//钢筋组
		numRebar = (int)rebarCurvesNum.size();

		RebarSymbology symb;
		if (twinBarInfo.hasTwinbars && bTwinbarLevel)
		{
			SetRebarColorBySize(twinBarInfo.rebarSize, symb);
			symb.SetRebarLevel(TEXT_TWIN_REBAR);
		}
		else
		{
			string str(sizeKey);
			char ccolar[20] = { 0 };
			strcpy(ccolar, str.c_str());
			SetRebarColorBySize(ccolar, symb);
			symb.SetRebarLevel(TEXT_MAIN_REBAR);
		}

		vector<vector<DPoint3d>> vecStartEnd;
		for (PITRebarCurve rebarCurve : rebarCurvesNum)
		{
			CPoint3D ptstr, ptend;
			rebarCurve.GetEndPoints(ptstr, ptend);
			DPoint3d midPos = ptstr;
			midPos.Add(ptend);
			midPos.Scale(0.5);
			if (m_strSlabRebarMethod != 2) // 起点、终点一致  画圆形钢筋
			{
				if (ISPointInHoles(m_Holeehs, midPos))
				{
					if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
					{
						continue;
					}
				}
			}
			vector<DPoint3d> linePts;
			RebarVertices vertices = rebarCurve.GetVertices();
			for (size_t i = 0; i < (size_t)vertices.GetSize(); ++i)
			{
				RebarVertex *tmpVertex = &vertices.At(i);
				linePts.push_back(tmpVertex->GetIP());
			}

			RebarElementP rebarElement = nullptr;
			if (!SlabPreviewButtonDown)//预览标志,预览状态下不生成钢筋
			{
				rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
			}

			if (nullptr != rebarElement)
			{
				//EditElementHandle eeh;
				//LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
				//eeh.AddToModel();

				RebarShapeData shape;
				if (bTwinbarLevel)
				{
					shape.SetSizeKey(CString(twinBarInfo.rebarSize));
					shape.SetIsStirrup(isStirrup);
					shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
					RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
					rebarElement->Update(rebarCurve, diameterTb, endTypes, shape, modelRef, false);
				}
				else
				{
					shape.SetSizeKey((LPCTSTR)sizeKey);
					shape.SetIsStirrup(isStirrup);
					shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
					RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
					rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
				}

				ElementId eleid = rebarElement->GetElementId();
				EditElementHandle tmprebar(eleid, ACTIVEMODEL);
				char tlevel[256];
				sprintf(tlevel, "%d", level);
				string slevel(tlevel);
				string Stype;
				if (DataExchange == 0)
				{
					if (twinBarInfo.hasTwinbars && bTwinbarLevel)
						Stype = "Twinback";
					else
						Stype = "back";
				}
				else if (DataExchange == 1)
				{
					if (twinBarInfo.hasTwinbars && bTwinbarLevel)
						Stype = "Twinmidden";
					else
						Stype = "midden";
				}
				else
				{
					if (twinBarInfo.hasTwinbars && bTwinbarLevel)
						Stype = "Twinfront";
					else
						Stype = "front";
				}
				ElementRefP oldref = tmprebar.GetElementRef();
				SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
				SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);

				ElementPropertiesSetterPtr propEle = ElementPropertiesSetter::Create();
				/*if (rebarColor > -1)
				{
					propEle->SetColor(rebarColor);
				}*/
				propEle->SetLinestyle(rebarLineStyle, NULL);
				propEle->SetWeight(rebarWeight);
				propEle->Apply(tmprebar);

				tmprebar.ReplaceInModel(oldref);
			}
			j++;
			vecStartEnd.push_back(linePts);
			m_vecAllRebarStartEndMap[index].push_back(linePts);
		}
		m_vecAllRebarStartEnd.push_back(vecStartEnd);//存储所有线段
	
		RebarSetData setdata;
		setdata.SetNumber(numRebar);
		CString spacingstring;
		spacingstring.Format(_T("%lf"), spacing / uor_per_mm);
		setdata.SetSpacingString(spacingstring);
		setdata.SetNominalSpacing(spacing / uor_per_mm);
		setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

		//if (m_outsidef.posnum > 1 && !m_strDelete)
		//	m_rebarSet = rebarSet;

		int ret = -1;
		ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

		RebarSetTag* tag = new RebarSetTag();
		tag->SetRset(rebarSet);
		tag->SetIsStirrup(isStirrup);

		return tag;
	}

	void LDSlabRebarAssembly::CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef)
	{
		if (modelRef == NULL)
			return;

		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		vTransform.clear();
		vTransformTb.clear();
		double dSideCover = GetSideCover()*uor_per_mm;
		double dPositiveCover = GetPositiveCover()*uor_per_mm;
		double dReverseCover = GetReverseCover()*uor_per_mm;
		double dLevelSpace = 0;
		double diameterTie = 0.0;
		BrString strTieRebarSize(GetTieRebarInfo().rebarSize);
		if (strTieRebarSize != L"" && 0 != GetTieRebarInfo().tieRebarMethod)
		{
			if (strTieRebarSize.Find(L"mm") != -1)
			{
				strTieRebarSize.Replace(L"mm", L"");
			}
			diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//拉筋直径
		}

		map<int, double> map_RebarSpace;//存储每一层钢筋的偏移距离
		map_RebarSpace.clear();

		double dOffset = dPositiveCover + diameterTie;
		double dOffsetTb = dPositiveCover + diameterTie;
		for (size_t i = 0; i < GetRebarLevelNum(); i++)
		{
			WString strSize = GetvecDirSize().at(i);
			if (strSize.find(L"mm") != WString::npos)
			{
				strSize.ReplaceAll(L"mm", L"");
			}

			double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10
			double diameterTb = 0.0;
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
			{
				diameterTb = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(i).rebarSize, modelRef);		//乘以了10
			}

			if (diameter > BE_TOLERANCE)
			{
				CVector3D	zTrans(0.0, 0.0, 0.0);
				CVector3D	zTransTb;
				if (GetvecDir().at(i) == 0) //水平
				{
					zTrans.z = m_STwallData.height - dSideCover - diameter * 0.5;
					zTrans.x = m_STwallData.length * 0.5;
				}
				else
				{
					zTrans.z = m_STwallData.height * 0.5;
					zTrans.x = dSideCover + diameter * 0.5;
				}
				zTransTb = zTrans;
				{
					WString strSizePre;
					if (i != 0)
					{
						strSizePre = WString(GetvecDirSize().at(i - 1).Get());
						if (strSizePre.find(L"mm") != WString::npos)
						{
							strSizePre.ReplaceAll(L"mm", L"");
						}
					}

					double diameterPre = RebarCode::GetBarDiameter(strSizePre, modelRef);		//乘以了10
					//if (0 == i)
					//{
					//	dOffset += diameter / 2.0;	//偏移首层钢筋半径
					//	dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm;
					//}
					//else
					//{
					//	dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm + diameter * 0.5 + diameterPre * 0.5;//层间距加上当前钢筋直径
					//}

					//dOffset += dLevelSpace;
					//dOffsetTb = dOffset;
					/* 修改偏移距离 */
					if (0 == i)
					{
						double diameter1 = 0.0;
						double diameter2 = 0.0;//在下面一层的钢筋的两个直径
						double diameter3 = 0.0;//可能底面有三层钢筋
						for (int n = 0; n < GetRebarLevelNum(); ++n)
						{
							if (GetvecDataExchange().at(n) == 0)
							{
								WString strSizePre;
								strSizePre = WString(GetvecDirSize().at(n).Get());
								if (strSizePre.find(L"mm") != WString::npos)
								{
									strSizePre.ReplaceAll(L"mm", L"");
								}
								if (diameter1 == 0.0)
									diameter1 = RebarCode::GetBarDiameter(strSizePre, modelRef);
								else if(diameter2 == 0.0)
									diameter2 = RebarCode::GetBarDiameter(strSizePre, modelRef);
								else
									diameter3 = RebarCode::GetBarDiameter(strSizePre, modelRef);
							}

						}
						if (GetvecDataExchange().at(i) == 2)//设置板顶钢筋的初始偏移面
							dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm + diameter * 0.5  + diameter1 + diameter2 + diameter3;
						else//板底钢筋
							dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm + diameter / 2.0;
						map_RebarSpace[i] = dLevelSpace;
					}
					else
					{
						double mydiameterPre = 0.0;
						double levelSpace = 0.0;
						for (int j = 0; j < i; ++j)//获取上一个在同一底面或顶面的钢筋直径和偏移距离
						{
							auto search = map_RebarSpace.find(j);
							if (search != map_RebarSpace.end())
							{
								if (GetvecDataExchange().at(j) == GetvecDataExchange().at(i))
								{
									WString strSizePre;
									strSizePre = WString(GetvecDirSize().at(j).Get());
									if (strSizePre.find(L"mm") != WString::npos)
									{
										strSizePre.ReplaceAll(L"mm", L"");
									}
									mydiameterPre = RebarCode::GetBarDiameter(strSizePre, modelRef);
									levelSpace = map_RebarSpace[j];
								}
							}
						}
						if (mydiameterPre == 0.0)//说明之前的钢筋没有与现在要设置的钢筋是同一面的
						{
							double diameter1 = 0.0;
							double diameter2 = 0.0;//在下面一层的钢筋的两个直径
							double diameter3 = 0.0;//可能底面有三层钢筋
							for (int n = 0; n < GetRebarLevelNum(); ++n)
							{
								if (GetvecDataExchange().at(n) == 0)
								{
									WString strSizePre;
									strSizePre = WString(GetvecDirSize().at(n).Get());
									if (strSizePre.find(L"mm") != WString::npos)
									{
										strSizePre.ReplaceAll(L"mm", L"");
									}
									if (diameter1 == 0.0)
										diameter1 = RebarCode::GetBarDiameter(strSizePre, modelRef);
									else if (diameter2 == 0.0)
										diameter2 = RebarCode::GetBarDiameter(strSizePre, modelRef);
									else
										diameter3 = RebarCode::GetBarDiameter(strSizePre, modelRef);
								}
								
							}
							if (GetvecDataExchange().at(i) == 2)
								dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm + diameter * 0.5  + diameter1 + diameter2 + diameter3;
							else
								dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm + diameter * 0.5;

						}
						else //说明有同样一个面的钢筋，只要在其基础上偏移
							dLevelSpace = levelSpace + diameter * 0.5 + mydiameterPre * 0.5;
						map_RebarSpace[i] = dLevelSpace;
					}

					dOffset = dPositiveCover + dLevelSpace;//每一个钢筋层的偏移距离
					dOffsetTb = dOffset;
					/* 修改偏移距离 */
					if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
					{
						if (diameterTb > diameter)//并筋层的钢筋比主筋直径大
							dOffsetTb += (diameterTb / 2.0 - diameter / 2.0);
						else
							dOffsetTb -= (diameter / 2.0 - diameterTb / 2.0);
					}
					if (COMPARE_VALUES(m_STwallData.width - dOffset, dReverseCover + diameterTie) < 0)		//当前钢筋层已嵌入到了反面保护层中时，实际布置的钢筋层间距就不再使用设置的与上层间距，而是使用保护层进行限制
					{
						zTrans.y = m_STwallData.width - dReverseCover - diameter / 2.0 - diameterTie;
						zTransTb.y = zTrans.y;
						if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
						{
							if (diameterTb > diameter)//并筋层的钢筋比主筋直径大
								zTransTb.y += (diameterTb / 2.0 - diameter / 2.0);
							else
								zTransTb.y -= (diameter / 2.0 - diameterTb / 2.0);
						}
						//判断：如果上一层的zTrans.y与当前层的zTrans.y相同，则上一层减去当前层的钢筋直径。（防止钢筋碰撞）
						double compare = zTrans.y;
						if (vTransform.size() > 0)
						{
							double reverseOffset = diameter;
							for (int j = (int)vTransform.size() - 1; j >= 0; j--)
							{
								WString strSize1 = GetvecDirSize().at(j);
								if (strSize1.find(L"mm") != WString::npos)
								{
									strSize1.ReplaceAll(L"mm", L"");
								}
								diameterPre = RebarCode::GetBarDiameter(strSize1, modelRef);		//乘以了10
								if (COMPARE_VALUES(vTransform[j].y + diameterPre * 0.5, compare - diameter * 0.5) > 0)	//嵌入了下一根钢筋终
								{
									vTransform[j].y -= reverseOffset;
									vTransformTb[j].y = vTransform[j].y;
									if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(j).hasTwinbars)
									{
										double diameterTbPre = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(j).rebarSize, modelRef);		//乘以了10

										if (diameterTbPre > diameterPre)//并筋层的钢筋比主筋直径大
											vTransformTb[j].y -= (diameterTbPre / 2.0 - diameterPre / 2.0);
										else
											vTransformTb[j].y += (diameterPre / 2.0 - diameterTbPre / 2.0);
									}
									compare = vTransform[j].y;
									diameter = diameterPre;
								}
							}
						}
					}
					else
					{
						zTrans.y = dOffset;
						zTransTb.y = dOffsetTb;
					}
				}
				vTransform.push_back(zTrans);
				vTransformTb.push_back(zTransTb);
			}
		}
	}
	void MoveCenterFaceByPt(MSElementDescrP &pFacet, DVec3d vec,DPoint3d oript)
	{
		mdlCurrTrans_begin();
		Transform tMatrix;
		mdlTMatrix_getIdentity(&tMatrix);
		mdlTMatrix_setOrigin(&tMatrix, &oript);
		mdlTMatrix_setTranslation(&tMatrix, &vec);
		mdlElmdscr_transform(&pFacet, &tMatrix);
		mdlCurrTrans_end();
	}
	//按当前层信息移动配筋面和墙面，并重新计算转换矩阵
	void LDSlabRebarAssembly::ReTransFaces(vector<CVector3D>& vTrans, vector<CVector3D>& vTransTb,int i
		,MSElementDescrP& upface, MSElementDescrP tmpupfaces[40], MSElementDescrP tmpdownfaces[40])
	{
		DVec3d vecZ = DVec3d::From(0, 0, 1);
		vecZ.Normalize();
		//获取每一个面的位置
		MSElementDescrP tmpdescr = nullptr;
		mdlElmdscr_duplicate(&tmpdescr, m_ldfoordata.facedes);
		double FHight = vTrans[i].y;
		vecZ.Scale(FHight);

		DVec3d negVecZ = DVec3d::From(0, 0, -1);
		double negmove = m_ldfoordata.Zlenth - FHight;
		negVecZ.Scale(negmove);
		mdlElmdscr_duplicate(&upface, m_ldfoordata.facedes);
		MoveCenterFaceByPt(upface, vecZ, m_ldfoordata.oriPt);
		for (int k = 0;k<m_ldfoordata.downnum;k++)
		{
			if (m_ldfoordata.downfaces[k]!=nullptr)
			{
				mdlElmdscr_duplicate(&tmpdownfaces[k], m_ldfoordata.downfaces[k]);
				MoveCenterFaceByPt(tmpdownfaces[k], vecZ, m_ldfoordata.oriPt);
			}
			
		}
		for (int k = 0; k < m_ldfoordata.upnum; k++)
		{
			if (m_ldfoordata.upfaces[k] != nullptr)
			{
				mdlElmdscr_duplicate(&tmpupfaces[k], m_ldfoordata.upfaces[k]);
				MoveCenterFaceByPt(tmpupfaces[k], negVecZ, m_ldfoordata.oriPt);
			}

		}
		CVector3D ORIPT = m_ldfoordata.oriPt;
		ORIPT.Add(vecZ);
		BeMatrix   placement = GetPlacement();
		placement.SetTranslation(ORIPT);
		SetPlacement(placement);
		vTrans[i].y = 0;//y方向不用再偏移
		vTransTb[i].y = 0;
	}

	//是否有垂直墙,0,没有；1，起始有，终止没有；2，终止有，起始没有；3，都有；4,特殊情况Z字墙锚入
	int IsHaveVerticalWall(DPoint3d ptstr,DPoint3d ptend, MSElementDescrP tmpfaces[10],int facenum)
	{
		int revalue = 0;
		DVec3d vecRebar = ptend - ptstr;
		vecRebar.Normalize();

		vector<MSElementDescrP> verfacesstr;//包含起始点，且与线垂直的面
		vector<MSElementDescrP> verfaceend;
		for (int i = 0;i<facenum;i++)
		{
			DPoint3d tmpstr, tmpend;
			tmpstr = tmpend = DPoint3d::From(0, 0, 0);
			PITCommonTool::CElementTool::GetLongestLineMidPt(tmpfaces[i], tmpstr, tmpend);
			DVec3d vectmp = tmpend - tmpstr;
			vectmp.Normalize();
			if (abs(vectmp.DotProduct(vecRebar))<0.01)
			{
				EditElementHandle tmpeeh(tmpfaces[i], false, false, ACTIVEMODEL);
				if (ISPointInElement(&tmpeeh,ptstr))
				{
					verfacesstr.push_back(tmpfaces[i]);
				}
				else if (ISPointInElement(&tmpeeh, ptend))
				{
					verfaceend.push_back(tmpfaces[i]);
				}
			}
		}

		if (verfacesstr.size()==0&&verfaceend.size()==0)
		{
			revalue = 0;
		}
		else if (verfacesstr.size()>0&&verfaceend.size()>0)
		{
			revalue = 3;
		}
		else if (verfacesstr.size()>0)
		{
			revalue = 1;
		}
		else
		{
			revalue = 2;
		}
		return revalue;
	}

	//是否有平行墙
	bool IsHaveParaWall(DPoint3d ptstr, DPoint3d ptend, MSElementDescrP tmpfaces[10], int facenum)
	{
		int revalue = 0;
		DVec3d vecRebar = ptend - ptstr;
		vecRebar.Normalize();
		DPoint3d midpos = ptstr;
		midpos.Add(ptend);
		midpos.Scale(0.5);
		vector<MSElementDescrP> parafaces;//包含起始点，且与线平行的面
		for (int i = 0; i < facenum; i++)
		{
			DPoint3d tmpstr, tmpend;
			tmpstr = tmpend = DPoint3d::From(0, 0, 0);
			PITCommonTool::CElementTool::GetLongestLineMidPt(tmpfaces[i], tmpstr, tmpend);
			DVec3d vectmp = tmpend - tmpstr;
			vectmp.Normalize();
			if (abs(vectmp.DotProduct(vecRebar)) > 0.9)
			{
				EditElementHandle tmpeeh(tmpfaces[i], false, false, ACTIVEMODEL);
				if (ISPointInElement(&tmpeeh, ptstr)||ISPointInElement(&tmpeeh, ptend)|| ISPointInElement(&tmpeeh, midpos))
				{
					return true;
				}
			}
		}
		return false;
	}
	void LDSlabRebarAssembly::CalculateInSideData(MSElementDescrP face/*当前配筋面*/,
		MSElementDescrP tmpupfaces[40],
		MSElementDescrP tmpdownfaces[40],
		int i,
		DVec3d rebarVec)
	{
		m_insidef.ClearData();
		double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		WString strSize1 = GetvecDirSize().at(i);
		if (strSize1.find(L"mm") != WString::npos)
		{
			strSize1.ReplaceAll(L"mm", L"");
		}

		int len = strSize1.size() * 4;
		setlocale(LC_CTYPE, "");
		char *p = new char[len];
		wcstombs(p, strSize1.c_str(), len);
		string tmpstr(p);
		string str_d = tmpstr.substr(0, 2);
		double Lae_d = stod(str_d);//得到sizekey中的直径
		delete[] p;

		double	diameter = RebarCode::GetBarDiameter(strSize1, ACTIVEMODEL);		//乘以了10
		double	bendradius = RebarCode::GetPinRadius(strSize1, ACTIVEMODEL, false);		//乘以了10
		double   diameterPre = 0;
		//double LaE = diameter*15;
		double LaE = 15 * Lae_d *uor_per_mm;
		if (m_isMidFloor  && m_Allwalls.size() == 3)
		{
			LaE = get_lae() * Lae_d;
		}
		//double LaE = get_lae();
		//if (LaE > 0)
		//	//LaE = LaE * diameter / uor_per_mm;
		//	//LaE = LaE * Lae_d;
		//	LaE = 15 * Lae_d * uor_per_mm;
		//else
		//	LaE = diameter * 15;
		if (i - 1 >= 0)
		{
			WString  strSize = GetvecDirSize().at(i - 1);
			if (strSize.find(L"mm") != WString::npos)
			{
				strSize.ReplaceAll(L"mm", L"");
			}
			diameterPre = RebarCode::GetBarDiameter(strSize, ACTIVEMODEL);
		}

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
		if (tmpdescrs.size() == 0)
		{
			return;
		}

		DVec3d tmpVec = DVec3d::From(1, 0, 0);
		//m_outsidef.calLen = diameterPre;
		if (GetvecDir().at(i) == 1)	//纵向钢筋,局部坐标系Z方向
		{
			tmpVec = DVec3d::From(0, 0, 1);
			//中间位置钢筋线,计算两端是否有垂直钢筋
			DPoint3d midstr = midpos;
			midstr.z = midpos.z - m_ldfoordata.Ylenth / 2 + GetSideCover()*uor_per_mm;
			DPoint3d midend = midstr;
			midend.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetSideCover()*uor_per_mm;
			int verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size());
			m_endtypes = verRe;
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
			if (tmpdescrs.size()==0)//上下都没有墙时
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
					m_insidef.calLen = diameter * 2;
					if (m_insidef.Verstr)
					{
						m_insidef.strtype.endType = 4;
						m_insidef.strtype.rotateAngle = -90;
						m_insidef.strtype.endPtInfo.value1 = bendradius;
						m_insidef.strtype.endPtInfo.value3 = LaE;
					}
					if (m_insidef.Verend)
					{
						m_insidef.endtype.endType = 4;
						m_insidef.endtype.rotateAngle = -90;
						m_insidef.endtype.endPtInfo.value1 = bendradius;
						m_insidef.endtype.endPtInfo.value3 = LaE;
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
					m_insidef.calLen = diameter * 2;
					if (m_insidef.Verstr)
					{
						m_insidef.strtype.endType = 4;
						m_insidef.strtype.rotateAngle = 90;
						m_insidef.strtype.endPtInfo.value1 = bendradius;
						m_insidef.strtype.endPtInfo.value3 = LaE;
					}
					if (m_insidef.Verend)
					{
						m_insidef.endtype.endType = 4;
						m_insidef.endtype.rotateAngle = 90;
						m_insidef.endtype.endPtInfo.value1 = bendradius;
						m_insidef.endtype.endPtInfo.value3 = LaE;
					}

					
				}
			}
			else//上下都没有墙，正常面处理
			{
				m_sidetype = SideType::Nor;
			}

			//计算配筋区间值,平行墙处理
			map<int, int>  tmpqj;
			tmpqj[(int)minP.x] = (int)maxP.x;//大面区间
			tmpqj[(int)maxP.x] = 0;//大面区间
			vector<MSElementDescrP> parafaces;//平行墙
			for (int i = 0; i < tmpdescrs.size(); i++)
			{
				/*过滤一些墙*/
				DRange3d faceRange;
				mdlElmdscr_computeRange(&faceRange.low, &faceRange.high, tmpdescrs[i], NULL);
				if (faceRange.XLength() < 0.6 * m_ldfoordata.Xlenth && faceRange.ZLength() < 0.6 * m_ldfoordata.Ylenth)
				{
					/*if (faceRange.low.x < 2 * uor_per_mm || faceRange.low.z < 2 * uor_per_mm || abs(faceRange.high.z - m_ldfoordata.Ylenth) < 2 * uor_per_mm || abs(faceRange.high.x - m_ldfoordata.Xlenth) < 2 * uor_per_mm)
						;*/
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
					if (tminP.z < 50 * UOR_PER_MilliMeter)
						tminP.z = 0;
					if (tmaxP.z < 50 * UOR_PER_MilliMeter)
						tmaxP.z = 0;
					tmpqj[(int)tminP.x] = (int)tmaxP.x;
					tmpqj[(int)tmaxP.x] = 0;
					parafaces.push_back(tmpdescrs[i]);
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
					if (abs(itr->first - minP.x) > 100&& parafaces.size()>0)//不是起始区间值
					{
						m_insidef.pos[m_insidef.posnum].addstr = true;
						if (i==1||i==2)//非外侧钢筋
						{
							m_insidef.pos[m_insidef.posnum].strval = GetSideCover()*uor_per_mm * 2 + 2 * diameter;
						}
						else
						{
							m_insidef.pos[m_insidef.posnum].strval = GetSideCover()*uor_per_mm * 2 + diameter;
						}
					}
					if (abs(itrnex->first - maxP.x) > 100 && parafaces.size() > 0)//不是终止区间值
					{
						m_insidef.pos[m_insidef.posnum].addend = true;
						if (i == 1 || i == 2)//非外侧钢筋
						{
							m_insidef.pos[m_insidef.posnum].endval = GetSideCover()*uor_per_mm * 2 + 2 * diameter;
						}
						else
						{
							m_insidef.pos[m_insidef.posnum].endval = GetSideCover()*uor_per_mm * 2+ diameter;
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
			midstr.x = midpos.x - m_ldfoordata.Xlenth / 2 + GetSideCover()*uor_per_mm;
			DPoint3d midend = midstr;
			midend.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetSideCover()*uor_per_mm;
			int verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size());
			m_endtypes = verRe;
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
				if (m_insidef.Verstr)
				{
					m_insidef.strtype.endType = 4;
					m_insidef.strtype.rotateAngle = 0;
					m_insidef.strtype.endPtInfo.value1 = bendradius;
					m_insidef.strtype.endPtInfo.value3 = LaE;
				}
				if (m_insidef.Verend)
				{
					m_insidef.endtype.endType = 4;
					m_insidef.endtype.rotateAngle = 0;
					m_insidef.endtype.endPtInfo.value1 = bendradius;
					m_insidef.endtype.endPtInfo.value3 = LaE;
				}
				
			}
			else if (m_ldfoordata.upnum > 0)//底部内侧面
			{
				if (m_insidef.Verstr || m_insidef.Verend)
				{
					//if (i==2)//内侧钢筋
					//{
					//	m_insidef.calLen = diameter;
					//}
					//else if (i==3)//外侧钢筋
					//{
					//	m_insidef.calLen = diameter*2;
					//}
					m_insidef.calLen = diameter * 2;
					if (m_insidef.Verstr)
					{
						m_insidef.strtype.endType = 4;
						m_insidef.strtype.rotateAngle = 180;
						m_insidef.strtype.endPtInfo.value1 = bendradius;
						m_insidef.strtype.endPtInfo.value3 = LaE;
					}
					if (m_insidef.Verend)
					{
						m_insidef.endtype.endType = 4;
						m_insidef.endtype.rotateAngle = 180;
						m_insidef.endtype.endPtInfo.value1 = bendradius;
						m_insidef.endtype.endPtInfo.value3 = LaE;
					}
				}
			}
			else//上下都没有墙，正常面处理
			{
				m_sidetype = SideType::Nor;
			}

			//计算配筋区间值,平行钢筋处理
			map<int, int>  tmpqj;
			tmpqj[(int)minP.z] = (int)maxP.z;//大面区间
			tmpqj[(int)maxP.z] = 0;//大面区间
			vector<MSElementDescrP> parafaces;//平行墙
			for (int i = 0; i < tmpdescrs.size(); i++)
			{
				/*过滤一些墙*/
				DRange3d faceRange;
				mdlElmdscr_computeRange(&faceRange.low, &faceRange.high, tmpdescrs[i], NULL);
				if (faceRange.XLength() < 0.6 * m_ldfoordata.Xlenth && faceRange.ZLength() < 0.6 * m_ldfoordata.Ylenth)
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
				if (abs(vectmp.DotProduct(tmpVec)) > 0.9)
				{
					DPoint3d tminP, tmaxP;
					//计算指定元素描述符中元素的范围。
					mdlElmdscr_computeRange(&tminP, &tmaxP, tmpdescrs[i], NULL);
					if (tminP.z < 50 * UOR_PER_MilliMeter)
						tminP.z = 0;
					if (tmaxP.z < 50 * UOR_PER_MilliMeter)
						tmaxP.z = 0;
					tmpqj[m_ldfoordata.Ylenth - (int)tminP.z] = m_ldfoordata.Ylenth - (int)tmaxP.z;
					tmpqj[m_ldfoordata.Ylenth - (int)tmaxP.z] = 0;
					parafaces.push_back(tmpdescrs[i]);
				}
			}
			map<int, int>::iterator itr = tmpqj.begin();
			for (;itr!=tmpqj.end();itr++)
			{
				map<int, int>::iterator itrnex = itr;
				itrnex++;
				if (itrnex==tmpqj.end())
				{
					break;
				}
				if (itrnex->first-itr->first<3)
				{
					continue;
				}
				int midpos = (itr->first + itrnex->first)/2;
				//判断中心点坐标是否在平行墙中，如果在跳过（不用配钢筋），不在的话，判断左右两端是否为大面的边坐标
				//如果是，不用配置边上钢筋，不是需要配置边钢筋
				bool isInWall = false;
				for (int j = 0;j<parafaces.size();j++)
				{
					DPoint3d tminP, tmaxP;
					//计算指定元素描述符中元素的范围。
					mdlElmdscr_computeRange(&tminP, &tmaxP, parafaces[j], NULL);
					if (midpos>(m_ldfoordata.Ylenth - tmaxP.z-1.0)&&midpos<(m_ldfoordata.Ylenth - tminP.z+1.0))
					{
						isInWall = true;
						break;
					}
				}
				if (!isInWall)//如果不在墙内
				{
					m_insidef.pos[m_insidef.posnum].str = itr->first;
					m_insidef.pos[m_insidef.posnum].end = itrnex->first;
					if (abs(itr->first - minP.z)>100 && parafaces.size() > 0)//不是起始区间值
					{
						if (i == 1 || i == 2)//非外侧钢筋
						{
							m_insidef.pos[m_insidef.posnum].strval = GetSideCover()*uor_per_mm * 2 + 2 * diameter;
						}
						else
						{
							m_insidef.pos[m_insidef.posnum].strval = GetSideCover()*uor_per_mm * 2 + diameter;
						}
					}
					if (abs(itrnex->first - maxP.z) > 100 && parafaces.size() > 0)//不是终止区间值
					{
						if (i == 1 || i == 2)//非外侧钢筋
						{
							m_insidef.pos[m_insidef.posnum].endval = GetSideCover()*uor_per_mm * 2 + 2 * diameter;
						}
						else
						{
							m_insidef.pos[m_insidef.posnum].endval = GetSideCover()*uor_per_mm * 2 + diameter;
						}
					}
					m_insidef.posnum++;
				}
			}

		}

	}

	void LDSlabRebarAssembly::CalculateOutSideData(MSElementDescrP face/*当前配筋面*/,
		MSElementDescrP tmpupfaces[40],
		MSElementDescrP tmpdownfaces[40],
		int i,
		DVec3d rebarVec,double& dis_x,double& dis_y)
	{
		m_outsidef.ClearData();
		double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		WString strSize1 = GetvecDirSize().at(i);
		if (strSize1.find(L"mm") != WString::npos)
		{
			strSize1.ReplaceAll(L"mm", L"");
		}
		int len = strSize1.size() * 4;
		setlocale(LC_CTYPE, "");
		char *p = new char[len];
		wcstombs(p, strSize1.c_str(), len);
		string tmpstr(p);
		string str_d = tmpstr.substr(0, 2);
		double La0_d = stod(str_d);
		delete[] p;

	   double	diameter = RebarCode::GetBarDiameter(strSize1, ACTIVEMODEL);		//乘以了10
	   double	bendradius = RebarCode::GetPinRadius(strSize1, ACTIVEMODEL,false);		//乘以了10
	   double   diameterPre = 0;
	   //double LaE = g_globalpara.m_alength[StringOperator::Convert::WStringToString(strSize1.GetWCharCP())]*uor_per_mm - bendradius - diameter / 2;
	   //double LaE = get_la0();
	   WString sizekey = GetvecDirSize().at(m_rebarlevel);
	   double lae_d = stod(sizekey.GetWCharCP()) * uor_per_mm;
	   double LaE = get_lae();
	   if (LaE > 0)
	   {
		   //LaE = LaE * diameter / uor_per_mm - bendradius - diameter / 2;
		   LaE = LaE * La0_d;
		   //LaE = LaE * diameter / uor_per_mm;
	   }
	   else
	   {
		   LaE = g_globalpara.m_alength[StringOperator::Convert::WStringToString(strSize1.GetWCharCP())] * uor_per_mm - bendradius - diameter / 2;
	   }
	   if (m_isMidFloor  && m_Allwalls.size() == 3)
	   {
		   LaE = 15 * lae_d;
	   }
	   if (i-1>=0)
	   {
		   WString  strSize = GetvecDirSize().at(i-1);
		   if (strSize.find(L"mm") != WString::npos)
		   {
			   strSize.ReplaceAll(L"mm", L"");
		   }
		   diameterPre = RebarCode::GetBarDiameter(strSize, ACTIVEMODEL);
	   }

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
		for (int i = 0;i<m_ldfoordata.upnum;i++)
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
		if (tmpdescrs.size()==0)
		{
			return;
		}
		//m_outsidef.calLen = diameterPre;
		DVec3d tmpVec = DVec3d::From(1, 0, 0);
		if (GetvecDir().at(i) == 1)	//纵向钢筋,局部坐标系Z方向
		{
			tmpVec = DVec3d::From(0, 0, 1);
			//中间位置钢筋线,计算两端是否有垂直钢筋
			DPoint3d midstr = midpos;
			midstr.z = midpos.z - m_ldfoordata.Ylenth / 2 + GetSideCover()*uor_per_mm;
			DPoint3d midend = midstr;
			midend.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetSideCover()*uor_per_mm;
			int verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size());
			m_endtypes = verRe;
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
			firstr.x = firstr.x - m_ldfoordata.Xlenth / 2 + GetSideCover()*uor_per_mm;
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
			secstr.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetSideCover()*uor_per_mm;
			secstr.z = midstr.z;
			DPoint3d secend = secstr;
			secend.z = midend.z;
			if (IsHaveParaWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size()))
			{
				m_outsidef.isdelend = true;
			}
			if (m_ldfoordata.downnum > 0)//顶部外侧面
			{
				if (i == 2 && (m_outsidef.Verstr || m_outsidef.Verend))//如果前面还有钢筋层，且有和钢筋线垂直的墙，钢筋长度左右都减少一个钢筋直径
				{
					//if (GetvecDir().at(0) == 1)
						diameterPre = 0;
					m_outsidef.calLen = diameterPre;
				}
				if (m_outsidef.Verstr)
				{
					m_outsidef.strtype.endType = 4;
					m_outsidef.strtype.rotateAngle = -90;
					m_outsidef.strtype.endPtInfo.value1 = bendradius;
					m_outsidef.strtype.endPtInfo.value3 = LaE;
				}
				if (m_outsidef.Verend)
				{
					m_outsidef.endtype.endType = 4;
					m_outsidef.endtype.rotateAngle = -90;
					m_outsidef.endtype.endPtInfo.value1 = bendradius;
					m_outsidef.endtype.endPtInfo.value3 = LaE;
				}
			}
			else if (m_ldfoordata.upnum > 0)//底部外侧面
			{
				if (i == 1 && (m_outsidef.Verstr || m_outsidef.Verend))//如果前面还有钢筋层，且有和钢筋线垂直的墙，钢筋长度左右都减少一个钢筋直径
				{
					//if (GetvecDir().at(0) == 1)
						diameterPre = 0;
					m_outsidef.calLen = diameterPre;
				}
				if (m_outsidef.Verstr)
				{
					m_outsidef.strtype.endType = 4;
					m_outsidef.strtype.rotateAngle = 90;
					m_outsidef.strtype.endPtInfo.value1 = bendradius;
					m_outsidef.strtype.endPtInfo.value3 = LaE;
				}
				if (m_outsidef.Verend)
				{
					m_outsidef.endtype.endType = 4;
					m_outsidef.endtype.rotateAngle = 90;
					m_outsidef.endtype.endPtInfo.value1 = bendradius;
					m_outsidef.endtype.endPtInfo.value3 = LaE;
				}
			}
			else//上下都没有墙，正常面处理
			{
				m_sidetype = SideType::Nor;
			}
			//计算配筋区间值,平行墙处理
			map<int, int>  tmpqj;
			tmpqj[(int)minP.x] = (int)maxP.x;//大面区间
			tmpqj[(int)maxP.x] = 0;//大面区间
			vector<MSElementDescrP> parafaces;//平行墙
			for (int i = 0; i < tmpdescrs.size(); i++)
			{
				/*过滤一些墙*/
				DRange3d faceRange;
				mdlElmdscr_computeRange(&faceRange.low, &faceRange.high, tmpdescrs[i], NULL);
				if (faceRange.XLength() < 0.8 * m_ldfoordata.Xlenth && faceRange.ZLength() < 0.8 * m_ldfoordata.Ylenth)
				{
					/*if (faceRange.low.x < 2 * uor_per_mm || faceRange.low.z < 2 * uor_per_mm || abs(faceRange.high.z - m_ldfoordata.Ylenth) < 2 * uor_per_mm || abs(faceRange.high.x - m_ldfoordata.Xlenth) < 2 * uor_per_mm)
						;*/
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
					/*如果板的最长位置大于墙的最长位置，板的最短位置小于墙的最短位置误差100*/
					if (((int)tmaxP.x <= (int)maxP.x + 100) && ((int)tminP.x + 100) >= (int)minP.x)
					{
						tmpqj[(int)tminP.x] = (int)tmaxP.x;
						tmpqj[(int)tmaxP.x] = 0;
						parafaces.push_back(tmpdescrs[i]);
					}
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
				int midpos = (itr->first + itrnex->first) / 2;
				//判断中心点坐标是否在平行墙中，如果在跳过（不用配钢筋），不在的话，判断左右两端是否为大面的边坐标
				//如果是，不用配置边上钢筋，不是需要配置边钢筋
				bool isInWall = false;
				//for (int j = 0; j < parafaces.size(); j++)
				//{
				//	DPoint3d tminP, tmaxP;
				//	//计算指定元素描述符中元素的范围。
				//	mdlElmdscr_computeRange(&tminP, &tmaxP, parafaces[j], NULL);
				//	if (midpos > (tminP.x - 1.0) && midpos < (tmaxP.x + 1.0))
				//	{
				//		isInWall = true;
				//		break;
				//	}
				//}
				if (!isInWall)//如果不在墙内
				{
					m_outsidef.pos[m_outsidef.posnum].str = itr->first;
					m_outsidef.pos[m_outsidef.posnum].end = itrnex->first;
					if (abs(itr->first - minP.x) > 100 && parafaces.size() > 0)//不是起始区间值
					{
						m_outsidef.pos[m_outsidef.posnum].addstr = true;
						if (i == 1 || i == 2)//非外侧钢筋
						{
							m_outsidef.pos[m_outsidef.posnum].strval = GetSideCover()*uor_per_mm/* * 2*/ + 2 * diameter;
						}
						else
						{
							m_outsidef.pos[m_outsidef.posnum].strval = GetSideCover()*uor_per_mm/* * 2*/ + diameter;
						}
					}
					if (abs(itrnex->first - maxP.x) > 100 && parafaces.size() > 0)//不是终止区间值
					{
						m_outsidef.pos[m_outsidef.posnum].addend = true;
						if (i == 1 || i == 2)//非外侧钢筋
						{
							m_outsidef.pos[m_outsidef.posnum].endval = GetSideCover()*uor_per_mm/* * 2*/ + 2 * diameter;
						}
						else
						{
							m_outsidef.pos[m_outsidef.posnum].endval = GetSideCover()*uor_per_mm/* * 2*/ + diameter;
						}
					}
					m_outsidef.posnum++;
				}
			}
		}
		else//横向钢筋X方向
		{
			tmpVec = DVec3d::From(1, 0, 0);
			//中间位置钢筋线,计算两端是否有垂直钢筋
			DPoint3d midstr = midpos;
			midstr.x = midpos.x - m_ldfoordata.Xlenth / 2 + GetSideCover()*uor_per_mm;
			DPoint3d midend = midstr;
			midend.x = midpos.x + m_ldfoordata.Xlenth / 2 - GetSideCover()*uor_per_mm;
			int verRe = IsHaveVerticalWall(midstr, midend, &tmpdescrs.at(0), tmpdescrs.size());
			m_endtypes = verRe;
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
			firstr.z = firstr.z - m_ldfoordata.Ylenth / 2 + GetSideCover()*uor_per_mm;
			firstr.x = midstr.x;
			DPoint3d firend = firstr;
			firend.x = midend.x;
			if (IsHaveParaWall(firstr, firend, &tmpdescrs.at(0), tmpdescrs.size()))
			{
				m_outsidef.isdelend = true;
			}
			EditElementHandle eehstr;
			LineHandler::CreateLineElement(eehstr, nullptr, DSegment3d::From(firstr, firend), true, *ACTIVEMODEL);
			//eehstr.AddToModel();


			//终止位置钢筋线，计算终止位置是否有平行钢筋
			DPoint3d secstr = midpos;
			secstr.z = midpos.z + m_ldfoordata.Ylenth / 2 - GetSideCover()*uor_per_mm;
			secstr.x = midstr.x;
			DPoint3d secend = secstr;
			secend.x = midend.x;
			if (IsHaveParaWall(secstr, secend, &tmpdescrs.at(0), tmpdescrs.size()))
			{
				m_outsidef.isdelstr = true;
			}
			if (m_ldfoordata.downnum > 0)//顶部外侧面
			{
				if (i == 2 && (m_outsidef.Verstr || m_outsidef.Verend))//如果前面还有钢筋层，且有和钢筋线垂直的墙，钢筋长度左右都减少一个钢筋直径
				{
					//if (GetvecDir().at(0) == 1)
						diameterPre = 0;
					m_outsidef.calLen = diameterPre;
				}
				if (m_outsidef.Verstr)
				{
					m_outsidef.strtype.endType = 4;
					m_outsidef.strtype.rotateAngle = 0;
					m_outsidef.strtype.endPtInfo.value1 = bendradius;
					m_outsidef.strtype.endPtInfo.value3 = LaE;
				}
				if (m_outsidef.Verend)
				{
					m_outsidef.endtype.endType = 4;
					m_outsidef.endtype.rotateAngle = 0;
					m_outsidef.endtype.endPtInfo.value1 = bendradius;
					m_outsidef.endtype.endPtInfo.value3 = LaE;
				}
			}
			else if (m_ldfoordata.upnum > 0)//底部外侧面
			{
				if (i == 1 && (m_outsidef.Verstr || m_outsidef.Verend))//如果前面还有钢筋层，且有和钢筋线垂直的墙，钢筋长度左右都减少一个钢筋直径
				{
					//if (GetvecDir().at(0) == 1)
						diameterPre = 0;
					m_outsidef.calLen = diameterPre;
				}
				if (m_outsidef.Verstr)
				{
					m_outsidef.strtype.endType = 4;
					m_outsidef.strtype.rotateAngle = 180;
					m_outsidef.strtype.endPtInfo.value1 = bendradius;
					m_outsidef.strtype.endPtInfo.value3 = LaE;
				}
				if (m_outsidef.Verend)
				{
					m_outsidef.endtype.endType = 4;
					m_outsidef.endtype.rotateAngle = 180;
					m_outsidef.endtype.endPtInfo.value1 = bendradius;
					m_outsidef.endtype.endPtInfo.value3 = LaE;
				}
			}
			else//上下都没有墙，正常面处理
			{
				m_sidetype = SideType::Nor;
			}
			//计算配筋区间值,平行钢筋处理
			map<int, int>  tmpqj;
			tmpqj[(int)minP.z] = (int)maxP.z;//大面区间
			tmpqj[(int)maxP.z] = 0;//大面区间
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
				if (abs(vectmp.DotProduct(tmpVec)) > 0.9)
				{
					DPoint3d tminP, tmaxP;
					//计算指定元素描述符中元素的范围。
					mdlElmdscr_computeRange(&tminP, &tmaxP, tmpdescrs[i], NULL);
					tmpqj[m_ldfoordata.Ylenth - (int)tminP.z] = m_ldfoordata.Ylenth - (int)tmaxP.z;
					tmpqj[m_ldfoordata.Ylenth - (int)tmaxP.z] = 0;
					parafaces.push_back(tmpdescrs[i]);
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
				int midpos = (itr->first + itrnex->first) / 2;
				//判断中心点坐标是否在平行墙中，如果在跳过（不用配钢筋），不在的话，判断左右两端是否为大面的边坐标
				//如果是，不用配置边上钢筋，不是需要配置边钢筋
				bool isInWall = false;
				//for (int j = 0; j < parafaces.size(); j++)
				//{
				//	DPoint3d tminP, tmaxP;
				//	//计算指定元素描述符中元素的范围。
				//	mdlElmdscr_computeRange(&tminP, &tmaxP, parafaces[j], NULL);
				//	if (midpos > (m_ldfoordata.Ylenth - tmaxP.z - 1.0) && midpos < (m_ldfoordata.Ylenth - tminP.z + 1.0))
				//	{
				//		isInWall = true;
				//		break;
				//	}
				//}
				if (!isInWall)//如果不在墙内
				{
					m_outsidef.pos[m_outsidef.posnum].str = itr->first;
					m_outsidef.pos[m_outsidef.posnum].end = itrnex->first;
					if (abs(itr->first - minP.z) > 100 && parafaces.size() > 0)//不是起始区间值
					{
						if (i == 1 || i == 2)//非外侧钢筋
						{
							m_outsidef.pos[m_outsidef.posnum].strval = GetSideCover()*uor_per_mm/* * 2*/ + 2 * diameter;
						}
						else
						{
							m_outsidef.pos[m_outsidef.posnum].strval = GetSideCover()*uor_per_mm/* * 2*/ + diameter;
						}
					}
					if (abs(itrnex->first - maxP.z) > 100 && parafaces.size() > 0)//不是终止区间值
					{
						if (i == 1 || i == 2)//非外侧钢筋
						{
							m_outsidef.pos[m_outsidef.posnum].endval = GetSideCover()*uor_per_mm/* * 2*/ + 2 * diameter;
						}
						else
						{
							m_outsidef.pos[m_outsidef.posnum].endval = GetSideCover()*uor_per_mm/* * 2*/ + diameter;
						}
					}
					m_outsidef.posnum++;
				}
			}
		}

	}
	//a、如果下部有墙，下部面为内侧面
		//钢筋方向VEC处理
		//(1)、取出所有与VEC方向同向的面，将墙面减去后计算得到配筋面；
		//(2)、按不同的配筋面数据配筋，判断钢筋面的起点和终点是否为整个大配筋面的起点或终点，
				 //如果是，忽略；不是，在远离配筋面中心的方向,多配置一根钢筋，偏移距离2个保护层+（第几层 - 1）个钢筋直径
		 //(3)、钢筋长度计算，起点上部有没有与钢筋方向垂直的墙，终点沿用起点数据
				//有垂直墙，再判断当前钢筋层，钢筋前面还有层，长度缩小2个钢筋直径，没有层缩小1个钢筋直径（钢筋锚入处理）
				//没有垂直墙，钢筋长度不用缩减
	void LDSlabRebarAssembly::CreateInSideFaceAssembly(int& iTwinbarSetIdIndex, int& setCount, MSElementDescrP upface,
		MSElementDescrP tmpupfaces[40],
		MSElementDescrP tmpdownfaces[40],
		int i,
		DVec3d rebarVec,
		vector<CVector3D>& vTrans,
		vector<CVector3D>& vTransTb
	)
	{
		m_sidetype = SideType::In;
		//计算此钢筋层的外侧信息
		double dLength = m_STwallData.length;
		double dWidth = m_STwallData.height;
		double dis_x = 0;//X方向偏移
		double dis_y = 0;//Y方向偏移

		CalculateInSideData(upface, tmpupfaces, tmpdownfaces, i, rebarVec);

		CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
		CMatrix3D   mat, matTb;
		vector<PIT::EndType> vecEndType;
		if (GetvvecEndType().empty())		//没有设置端部样式，设置默认值
		{
			EndType endType;
			memset(&endType, 0, sizeof(endType));
			vecEndType = { { 0,0,0 },{0,0,0}};
		}
		else if (i < GetvvecEndType().size()) // 避免越界访问
		{
			vecEndType = GetvvecEndType()[i];
		}
		// 确保 vecEndType 至少有 2 个元素
		if (vecEndType.size() < 2) {
			vecEndType.resize(2);
		}
		//内侧面弯钩处理
		vecEndType[0] = m_insidef.strtype;
		vecEndType[1] = m_insidef.endtype;

		double  Misdisstr = 0; double Misdisend = 0;
		double tLenth = dLength;

		m_vecRebarPtsLayer.clear();
		m_vecTwinRebarPtsLayer.clear();
		m_vecTieRebarPtsLayer.clear();
		m_nowvecDir = GetvecDir().at(i);
		if (GetvecDir().at(i) == 1)	//纵向钢筋
		{
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向
			for (size_t k = 0; k < vecEndNormal.size(); ++k)
			{
				double dRotateAngle = vecEndType.at(k).rotateAngle;
				endNormal = m_STwallData.ptEnd - m_STwallData.ptStart;
				endNormal.Normalize();
				endNormal.Negate();
				CVector3D rebarVec = CVector3D::kYaxis;
				/*					endNormal = rebarVec.CrossProduct(vec);*/
				endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
				vecEndNormal[k] = endNormal;
			}			
			if (m_strSlabRebarMethod != 2)
			{
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;

				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
			}
			for (int j=0;j<m_insidef.posnum;j++)
			{
				double nowLen = dLength;
				double nowwidth = dWidth;
				nowLen = m_insidef.pos[j].end - m_insidef.pos[j].str;
				dis_x = m_insidef.pos[j].str;
				m_insidef.strval = m_insidef.pos[j].strval;
				m_insidef.endval = m_insidef.pos[j].endval;
				MakeFaceRebars(iTwinbarSetIdIndex, setCount, i,
					nowLen, nowwidth, vecEndType, vecEndNormal, dis_x, dis_y,
					mat, matTb);
			}
			vecEndType.clear();
			
		}
		else
		{
			double leftSideCov = 0;
			double rightSideCov = 0;
			double allSideCov = leftSideCov + rightSideCov;
			vTrans[i].x = (tLenth - allSideCov) / 2 + Misdisstr + leftSideCov;
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向
			for (size_t k = 0; k < vecEndNormal.size(); ++k)
			{
				double dRotateAngle = vecEndType.at(k).rotateAngle;
				CVector3D rebarVec = m_STwallData.ptEnd - m_STwallData.ptStart;
				rebarVec.Negate();   
				endNormal = CVector3D::From(0, 0, -1);
				endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
				vecEndNormal[k] = endNormal;	
			}
			mat = rot90;
			matTb = rot90;
			if (m_strSlabRebarMethod != 2)
			{
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;

				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
			}
			//奇数层为并筋层,偶数层为普通层
			for (int j = 0; j < m_insidef.posnum; j++)
			{
				double nowLen = dLength;
				double nowwidth = dWidth;
				nowwidth = m_insidef.pos[j].end - m_insidef.pos[j].str;
				dis_x = m_insidef.pos[j].str;
				m_insidef.strval = m_insidef.pos[j].strval;
				m_insidef.endval = m_insidef.pos[j].endval;
				if (nowwidth > 3 * dWidth)
					break;
				MakeFaceRebars(iTwinbarSetIdIndex, setCount, i,
					nowwidth, nowLen, vecEndType, vecEndNormal, dis_x, dis_y,
					mat, matTb);
			}
			//end
			vecEndType.clear();
		}
	}
	//b、如果下部没有墙，下部面为外侧面
	//钢筋方向VEC处理
	   //（1）钢筋长度计算，上部有没有墙，终点下部处理和起点类似
	         //起点或终点钢筋，判断有没有和钢筋方向平行且钢筋线在墙面内时，起点或终点钢筋不生成
			//有和钢筋线垂直的墙，再判断当前钢筋层，钢筋前面还有层，起点部分长度缩小1个钢筋直径，没有层缩不缩小（钢筋锚入处理）
		   //没有墙，钢筋长度不用缩减
	void LDSlabRebarAssembly::CreateOutSideFaceAssembly(int& iTwinbarSetIdIndex,int& setCount, MSElementDescrP upface,
		MSElementDescrP tmpupfaces[40],
		MSElementDescrP tmpdownfaces[40],
		int i,
		DVec3d rebarVec,
		vector<CVector3D>& vTrans,
	    vector<CVector3D>& vTransTb
		)
	{
		m_sidetype = SideType::Out;
		//计算此钢筋层的外侧信息
		double dLength = m_STwallData.length;
		double dWidth = m_STwallData.height;
		double dis_x = 0;//X方向偏移
		double dis_y = 0;//Y方向偏移

		CalculateOutSideData(upface, tmpupfaces, tmpdownfaces, i, rebarVec, dis_x, dis_y);

		CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);

		CMatrix3D   mat, matTb;
		vector<PIT::EndType> vecEndType;
		if (GetvvecEndType().empty())		//没有设置端部样式，设置默认值
		{
			EndType endType;
			memset(&endType, 0, sizeof(endType));
			vecEndType = { { 0,0,0 },{0,0,0}};
		}
		else if (i < GetvvecEndType().size()) // 避免越界访问
		{
			vecEndType = GetvvecEndType()[i];
		}
		// 确保 vecEndType 至少有 2 个元素
		if (vecEndType.size() < 2) {
			vecEndType.resize(2);
		}
		//外侧面弯钩处理
		vecEndType[0] = m_outsidef.strtype;
		vecEndType[1] = m_outsidef.endtype;
		double  Misdisstr = 0; double Misdisend = 0;
		double tLenth = dLength;

		m_vecRebarPtsLayer.clear();
		m_vecTwinRebarPtsLayer.clear();
		m_vecTieRebarPtsLayer.clear();
		m_nowvecDir = GetvecDir().at(i);
		if (GetvecDir().at(i) == 1)	//纵向钢筋
		{
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向
			for (size_t k = 0; k < vecEndNormal.size(); ++k)
			{
				double dRotateAngle = vecEndType.at(k).rotateAngle;
				endNormal = m_STwallData.ptEnd - m_STwallData.ptStart;
				endNormal.Normalize();
				endNormal.Negate();
				CVector3D rebarVec = CVector3D::kYaxis;
				/*					endNormal = rebarVec.CrossProduct(vec);*/
				endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
				vecEndNormal[k] = endNormal;
			}

			if (m_strSlabRebarMethod != 2)
			{
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;

				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
			}
			double maxLength = 0;
			for(auto it : m_outsidef.pos)
			{
				double length = it.end - it.str;
				if (maxLength < length)
					maxLength = length;
			}
			double minLength = maxLength;
			for (auto it : m_outsidef.pos)
			{
				double length = it.end - it.str;
				if (minLength > length && length != 0)
					minLength = length;
			}
			for (int j = 0; j < m_outsidef.posnum; j++)
			{
				double nowLen = dLength;
				double nowwidth = dWidth;
				nowLen = m_outsidef.pos[j].end - m_outsidef.pos[j].str;
				dis_x = m_outsidef.pos[j].str;
				m_outsidef.strval = m_outsidef.pos[j].strval;
				m_outsidef.endval = m_outsidef.pos[j].endval;

				if (nowLen < 1201 * UOR_PER_MilliMeter && nowLen != maxLength)//(minLength == nowLen && nowLen != maxLength)//最边缘的外侧面的起始和终端两条钢筋需要删除
				{
					m_strDelete = true;
					m_endDelete = true;
				}
				/*if (m_outsidef.posnum > 1 && j != m_outsidef.posnum - 1)
					m_isAddset = false;*/
				MakeFaceRebars(iTwinbarSetIdIndex, setCount, i,
					nowLen, nowwidth, vecEndType, vecEndNormal, dis_x, dis_y,
					mat, matTb);
				m_strDelete = false;
				m_endDelete = false;
				//m_isAddset = true;
			}
			/*MakeFaceRebars(iTwinbarSetIdIndex, setCount, i,
				dLength, dWidth, vecEndType, vecEndNormal, dis_x, dis_y,
				mat, matTb);*/
			vecEndType.clear();
		}
		else
		{
			double leftSideCov = 0;
			double rightSideCov = 0;
			double allSideCov = leftSideCov + rightSideCov;
			vTrans[i].x = (tLenth - allSideCov) / 2 + Misdisstr + leftSideCov;
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向
			for (size_t k = 0; k < vecEndNormal.size(); ++k)
			{
				double dRotateAngle = vecEndType.at(k).rotateAngle;
				CVector3D rebarVec = m_STwallData.ptEnd - m_STwallData.ptStart;
				rebarVec.Negate();
				endNormal = CVector3D::From(0, 0, -1);
				endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
				vecEndNormal[k] = endNormal;
			}
			mat = rot90;
			matTb = rot90;
			if (m_strSlabRebarMethod != 2)
			{
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;

				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
			}
			//奇数层为并筋层,偶数层为普通层
			double maxWidth = 0;
			for (auto it : m_outsidef.pos)
			{
				double width = it.end - it.str;
				if (maxWidth < width)
					maxWidth = width;
			}
			for (int j = 0; j < m_outsidef.posnum; j++)
			{
				double nowLen = dLength;
				double nowwidth = dWidth;
				nowwidth = m_outsidef.pos[j].end - m_outsidef.pos[j].str;
				dis_x = m_outsidef.pos[j].str;
				m_outsidef.strval = m_outsidef.pos[j].strval;
				m_outsidef.endval = m_outsidef.pos[j].endval;
				if (nowwidth > 3 * dWidth)
					break;
				if (nowwidth < 1201 * UOR_PER_MilliMeter && nowwidth != maxWidth)//最边缘的外侧面的起始和终端两条钢筋需要删除
				{
					m_strDelete = true;
					m_endDelete = true;
				}
				/*if (m_outsidef.posnum > 1 && j != m_outsidef.posnum - 1)
					m_isAddset = false;*/
				MakeFaceRebars(iTwinbarSetIdIndex, setCount, i,
					nowwidth, nowLen, vecEndType, vecEndNormal, dis_x, dis_y,
					mat, matTb);
				m_strDelete = false;
				m_endDelete = false;
				//m_isAddset = true;
			}
			/*MakeFaceRebars(iTwinbarSetIdIndex, setCount, i,
				dWidth, tLenth, vecEndType, vecEndNormal, dis_x, dis_y,
				mat, matTb);*/
			//end
			vecEndType.clear();
		}
	}

	//绘制单层钢筋
	void LDSlabRebarAssembly::MakeFaceRebars(int& iTwinbarSetIdIndex,int& setCount,int i,
		double dLength,double dWidth, vector<PIT::EndType>& vecEndType, 
		vector<CVector3D>& vecEndNormal,double dis_x,double dis_y,
		CMatrix3D&  mat, CMatrix3D&  matTb)
	{
		RebarSetTag* tag = NULL;
		double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		//绘制并筋--begin
		if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
		{
			//先绘制非并筋层
			PopvecSetId().push_back(0);
			setCount++;
			tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), 
				dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm,GetvecStartOffset().at(i)*uor_per_mm + dis_x,
				GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i),
				GetvecRebarType().at(i), GetvecDataExchange().at(i), ACTIVEMODEL,/*GetvecRebarColor().at(i),*/GetvecRebarLineStyle().at(i),GetvecRebarWeight().at(i),i);
			if (NULL != tag)
			{
				tag->SetBarSetTag(setCount);
				rsetTags.Add(tag);
			}

			//绘制并筋层
			PopvecSetId().push_back(0);
			setCount++;
			tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm,
				GetvecStartOffset().at(i)*uor_per_mm + dis_x,
				GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), true,
				GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), ACTIVEMODEL, /*GetvecRebarColor().at(i),*/ GetvecRebarLineStyle().at(i), GetvecRebarWeight().at(i),i);
			
			if (NULL != tag)
			{
				tag->SetBarSetTag(setCount);
				rsetTags.Add(tag);
			}
			iTwinbarSetIdIndex++;
		}
		else //当前层未设置并筋
		{
			//if (m_outsidef.posnum > 1)
			//{
			//	TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
			//	PopvecSetId().push_back(0);
			//	
			//	setCount++;
			//	tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm,
			//		GetvecStartOffset().at(i)*uor_per_mm + dis_x,
			//		GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, twinRebar, false, GetvecRebarLevel().at(i),
			//		GetvecRebarType().at(i), GetvecDataExchange().at(i), ACTIVEMODEL, /*GetvecRebarColor().at(i),*/ GetvecRebarLineStyle().at(i), GetvecRebarWeight().at(i), i);
			//	
			//	if (m_isAddset)
			//	{
			//		RebarSetData setdata;
			//		setdata.SetNumber(m_rebarSetNum);
			//		CString spacingstring;
			//		spacingstring.Format(_T("%lf"), GetvecDirSpacing().at(i));
			//		setdata.SetSpacingString(spacingstring);
			//		setdata.SetNominalSpacing(GetvecDirSpacing().at(i));
			//		setdata.SetAverageSpacing(m_adjustedSpacing / uor_per_mm);

			//		int ret = -1;
			//		ret = m_rebarSet->FinishUpdate(setdata, ACTIVEMODEL);	//返回的是钢筋条数

			//		RebarSetTag* mytag = new RebarSetTag();
			//		mytag->SetRset(m_rebarSet);
			//		mytag->SetIsStirrup(false);
			//		m_rebarSetNum = 0;
			//		if (NULL != mytag)
			//		{
			//			mytag->SetBarSetTag(setCount);
			//			rsetTags.Add(mytag);
			//		}
			//	}
			//	
			//}
			//else
			//{
				TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm,
					GetvecStartOffset().at(i)*uor_per_mm + dis_x,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, twinRebar, false, GetvecRebarLevel().at(i),
					GetvecRebarType().at(i), GetvecDataExchange().at(i), ACTIVEMODEL, /*GetvecRebarColor().at(i),*/ GetvecRebarLineStyle().at(i), GetvecRebarWeight().at(i), i);
				if (NULL != tag)
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}
			//}
			
		}
	}




	bool LDSlabRebarAssembly::MakeRebars(DgnModelRefP modelRef)      // 创建钢筋
	{
		m_sidetype = SideType::Nor;
		rsetTags.Clear(true);
		m_vecAllRebarStartEnd.clear();
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
		g_vecRebarPtsNoHole.clear();
		g_vecTieRebarPtsNoHole.clear();
		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		
		double dLevelSpace = 0;
		double dSideCover = GetSideCover()*uor_per_mm;
		if (COMPARE_VALUES(dSideCover, m_ldfoordata.Zlenth) >= 0)	//如果侧面保护层大于等于墙的长度
		{
			mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"侧面保护层大于等于墙的长度,无法创建钢筋层", MessageBoxIconType::Information);
			return false;
		}
		vector<CVector3D> vTrans;
		vector<CVector3D> vTransTb;
		//计算侧面整体偏移量
		CalculateTransform(vTrans, vTransTb, modelRef);
		if (vTrans.size() != GetRebarLevelNum())
		{
			return false;
		}

		double dLength = m_STwallData.length;
		double dWidth = m_STwallData.height;

		int iRebarSetTag = 0;
		int iRebarLevelNum = GetRebarLevelNum();
		int iTwinbarSetIdIndex = 0;
		int setCount = 0;
		vector<PIT::EndType> vecEndType;


		int frontlevel = 0;//前层层号
		int backlevel = 0;//后层层号
		int midlevel = 0;//中间层号
		int allbacklevel = 0;
		//统计下背后总过有多少层
		for (int i = 0; i < iRebarLevelNum; ++i)
		{
			if (GetvecDataExchange().at(i) == 2)
			{
				allbacklevel++;
			}
		}
		backlevel = allbacklevel;
		for (int i = 0; i < GetRebarLevelNum(); ++i)
		{
			double RebarRadius = RebarCode::GetBarDiameter(GetvecDirSize().at(i), ACTIVEMODEL)/2;
			m_mapLevelRadius[i] = RebarRadius;
		}

		/*/////扫描板附近的所有墙/////*/
		DRange3d slab_range;
		mdlElmdscr_computeRange(&slab_range.low, &slab_range.high, m_pOldElm->GetElementDescrCP(), NULL);
		slab_range.low.z -= 5*UOR_PER_MilliMeter;
		slab_range.low.x += 5*UOR_PER_MilliMeter;
		slab_range.low.y += 5*UOR_PER_MilliMeter;
		if (m_ldfoordata.downnum == 0 && m_ldfoordata.upnum == 0)// 7BGZ7Y01DB- 上下没有相连的墙
			slab_range.high.z += UOR_PER_MilliMeter * 1000;
		else
			slab_range.high.z += 5*UOR_PER_MilliMeter;
		slab_range.high.x -= 5*UOR_PER_MilliMeter;
		slab_range.high.y -= 5*UOR_PER_MilliMeter;
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

		m_ScanedAllWallsandFloor.clear();
		for (auto walleh : m_Allwalls)
		{
			EditElementHandle walleeh(walleh.GetElementId(), walleh.GetModelRef());
			if (walleeh.IsValid())
			{
				EditElementHandle *eeh = new EditElementHandle();
				eeh->Duplicate(walleeh);
				ISolidKernelEntityPtr entityPtr;
				if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
				{
					SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
					eeh->GetElementDescrP();
					m_ScanedAllWallsandFloor.push_back(eeh);
				}
			}
		}
		m_ScanedAllWallsandFloor.emplace_back(m_pOldElm);
		for (auto walleh : m_Allwalls)
		{//遍历所有墙，判断是不是Z型板拆开的两块板
			auto id = walleh.GetElementId();
			DRange3d tmpWall_range;
			mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, walleh.GetElementDescrCP(), nullptr);
			DRange3d floor_range;
			mdlElmdscr_computeRange(&floor_range.low, &floor_range.high, m_pOldElm->GetElementDescrCP(), nullptr);

			//取出墙方向和板方向
			DVec3d wall_direction = tmpWall_range.high - tmpWall_range.low;
			DVec3d fool_direction = floor_range.high - floor_range.low;
			bool  is_one_direction = false;
			if ((wall_direction.x > wall_direction.y && fool_direction.x > fool_direction.y)//如果墙板的方向相同
				|| (wall_direction.y > wall_direction.x &&fool_direction.y > fool_direction.x)
				)
			{
				is_one_direction = true;
			}

			if (tmpWall_range.low.z < floor_range.low.z && tmpWall_range.high.z > floor_range.high.z && is_one_direction)
			{
				m_isMidFloor = true;		
				auto id = walleh.GetElementId();

				for (auto walleh : m_Allwalls)
				{//遍历所有墙，确定是中间的板之后确定剩余那块墙是在板上还是板下
					DRange3d tmpWall_range;
					mdlElmdscr_computeRange(&tmpWall_range.low, &tmpWall_range.high, walleh.GetElementDescrCP(), nullptr);
					DRange3d floor_range;
					mdlElmdscr_computeRange(&floor_range.low, &floor_range.high, m_pOldElm->GetElementDescrCP(), nullptr);
					if (tmpWall_range.low.z < floor_range.low.z && tmpWall_range.high.z > floor_range.high.z)
					{
						continue;
					}
					if (tmpWall_range.low.z < floor_range.low.z)//判断墙的高度
						m_isIndownWall = true;
					else if (tmpWall_range.high.z > floor_range.high.z)
						m_isIndownWall = false;
					if (tmpWall_range.XLength() > tmpWall_range.YLength())//判断墙的方向
						m_isXdir = true;
					else
						m_isXdir = false;
				}
			}
		}
		for (int i = 0; i < iRebarLevelNum; ++i)
		{
			m_rebarlevel = i;
			if (GetvecDir().at(m_rebarlevel) == 0)
				m_xflag = 0;
			else if (GetvecDir().at(m_rebarlevel) == 1)
				m_yflag = 0;
			if (vTrans.size() != GetRebarLevelNum())
			{
				return false;
			}
			MSElementDescrP upface = nullptr;
			MSElementDescrP tmpupfaces[40] = {0,0,0,0,0,0,0,0,0,0};
			MSElementDescrP tmpdownfaces[40] = {0,0,0,0,0,0,0,0,0,0 };
			ReTransFaces(vTrans, vTransTb, i, upface, tmpupfaces, tmpdownfaces);
			double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;
			// 计算层关系
			int tmpLevel;
			DVec3d vecRebar = DVec3d::From(1, 0, 0);
			if (GetvecDir().at(i) == 1)	//纵向钢筋
			{
				vecRebar = DVec3d::From(0, 1, 0);
			}
			if (GetvecDataExchange().at(i) == 0)//前面层,下部钢筋处理
			{
				frontlevel++;
				tmpLevel = frontlevel;

				m_isMidFloor = false;

				if (m_isMidFloor && m_Allwalls.size() == 3)
				{
					if (m_isIndownWall)
					{
						if (m_isXdir)
						{
							if(GetvecDir().at(i) == 0)
								CreateInSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
							else
								CreateOutSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
						}
						else
						{
							if (GetvecDir().at(i) == 1)
								CreateInSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
							else
								CreateOutSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
						}
					}
					else
					{
						if (m_isXdir)
						{
							if (GetvecDir().at(i) == 1)
								CreateInSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
							else
								CreateOutSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
						}
						else
						{
							if (GetvecDir().at(i) == 0)
								CreateInSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
							else
								CreateOutSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
						}
					}
				}
				else
				{
					m_isdown = true;
					/*frontlevel++;
					tmpLevel = frontlevel;*/
					if (m_ldfoordata.downnum >=2)//下部有墙，下部面为内侧面
					{
						//a、如果下部有墙，下部面为内侧面
						//钢筋方向VEC处理
						   //(1)、取出所有与VEC方向同向的面，将墙面减去后计算得到配筋面；
						   //(2)、按不同的配筋面数据配筋，判断钢筋面的起点和终点是否为整个大配筋面的起点或终点，
								  //如果是，忽略；不是，在远离配筋面中心的方向,多配置一根钢筋，偏移距离2个保护层+（第几层 - 1）个钢筋直径
						   //(3)、钢筋长度计算，起点上部有没有与钢筋方向垂直的墙，终点沿用起点数据
								  //有垂直墙，再判断当前钢筋层，钢筋前面还有层，长度缩小2个钢筋直径，没有层缩小1个钢筋直径（钢筋锚入处理）
								  //没有垂直墙，钢筋长度不用缩减
						m_isoutside = false;
						CreateInSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
					}
					else
					{
						//b、如果下部没有墙，下部面为外侧面
						 //钢筋方向VEC处理
						 //（1）钢筋长度计算，上部有没有墙，终点下部处理和起点类似
										  //起点或终点钢筋，判断有没有和钢筋方向平行且钢筋线在墙面内时，起点或终点钢筋不生成
										  //有和钢筋线垂直的墙，再判断当前钢筋层，钢筋前面还有层，起点部分长度缩小1个钢筋直径，没有层缩不缩小（钢筋锚入处理）
								  //没有墙，钢筋长度不用缩减
						m_isoutside = true;
						CreateOutSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
					}
				}
			}
			else if (GetvecDataExchange().at(i) == 2)//后面层，上部钢筋处理
			{
				tmpLevel = backlevel;
				backlevel--;
				if (m_isMidFloor && m_Allwalls.size() == 3)
				{
					if (m_isIndownWall)
					{
						if (m_isXdir)
						{
							if (GetvecDir().at(i) == 1)
								CreateInSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
							else
								CreateOutSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
						}
						else
						{
							if (GetvecDir().at(i) == 0)
								CreateInSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
							else
								CreateOutSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
						}
					}
					else
					{
						if (m_isXdir)
						{
							if (GetvecDir().at(i) == 0)
								CreateInSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
							else
								CreateOutSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
						}
						else
						{
							if (GetvecDir().at(i) == 1)
								CreateInSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
							else
								CreateOutSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
						}
					}
				}
				else
				{
					m_isdown = false;
					/*tmpLevel = backlevel;
					backlevel--;*/
					if (m_ldfoordata.downnum == 0 && m_ldfoordata.upnum == 0)
					{
						m_isoutside = false;
						CreateOutSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
					}
					else
					{
						if (m_ldfoordata.downnum >= 2)//下部有墙，后面层为外侧面
						{
							//b、如果上部没有墙，上部面为外侧面
							 //钢筋方向VEC处理
							 //（1）钢筋长度计算，下部有没有与钢筋方向垂直的墙，终点下部处理和起点类似
									  //起点或终点钢筋，判断有没有和钢筋方向平行且钢筋线在墙面内时，起点或终点钢筋不生成
											  //有和钢筋线垂直的墙，再判断当前钢筋层，钢筋前面还有层，起点部分长度缩小1个钢筋直径，没有层缩不缩小（钢筋锚入处理）
									  //没有墙，钢筋长度不用缩减
							m_isoutside = true;
							CreateOutSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
						}
						else
						{
							//a、如果上部有墙，上部面为内侧面
							//钢筋方向VEC处理
							   //(1)、取出所有与VEC方向同向的面，将墙面减去后得到配筋面；
							   //(2)、按不同的配筋面数据配筋，判断钢筋面的起点和终点是否为整个大配筋面的起点或终点，
									  //如果是，忽略；不是，在远离配筋面中心的方向,多配置一根钢筋，偏移距离2个保护层+（第几层 - 1）个钢筋直径
							   //(3)、钢筋长度计算，起点上部有没有与钢筋方向垂直的墙，终点沿用起点数据
									  //有垂直墙，再判断当前钢筋层，钢筋前面还有层，长度缩小2个钢筋直径，没有层缩小1个钢筋直径（钢筋锚入处理）
									  //没有垂直墙，钢筋长度不用缩减
							m_isoutside = false;
							CreateInSideFaceAssembly(iTwinbarSetIdIndex, setCount, upface, tmpupfaces, tmpdownfaces, i, vecRebar, vTrans, vTransTb);
						}
					}
				}
				
				
			}
			else
			{
				midlevel++;
				tmpLevel = midlevel;
			}
			if (m_vecRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecRebarPtsLayer.at(m);
					rbPt.ptend = m_vecRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
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
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTwinRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTwinRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTwinRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
			if (m_vecTieRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecTieRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTieRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTieRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTieRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
		}

		if (SlabPreviewButtonDown)//预览按钮按下，则画主筋线
		{
			int index = 0;
			m_allLines.clear();
			//主筋及并筋线信息
			for (auto it = m_vecAllRebarStartEndMap.begin(); it != m_vecAllRebarStartEndMap.end(); it++, index++)
			{

				if (index >= GetvecDir().size()) continue;
				if (index >= GetvecDataExchange().size()) continue;
				UInt32 colors = 3;//绿色
				int style = 0;//实线
				UInt32	weight = 2;

				if (GetvecDir().at(index) == 0)//如果是水平方向
					colors = 1;//红色

				if (GetvecDataExchange().at(index) == 2)//如果是内侧
					style = 2;//虚线
				//修改后板不在区分内外侧，板下为板底，板上为板顶
				//if (Gallery::WallHelper::analysis_slab_isTop(ehSel))//如果是顶板需要切换
				//{
				//	if (style == 2)//虚线换成实线
				//		style = 0;
				//	else
				//		style = 2;
				//}


				vector<vector<DPoint3d>> faceLinePts = it->second;
				for (auto it : faceLinePts)
				{
					vector<DPoint3d> linePts = it;
					EditElementHandle eeh;
					LineStringHandler::CreateLineStringElement(eeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
					mdlElement_setSymbology(eeh.GetElementP(), &colors, &weight, &style);
					eeh.AddToModel();
					m_allLines.push_back(eeh.GetElementRef());
				}
			}
		}
		if (g_globalpara.Getrebarstyle() != 0)
		{
			return (SUCCESS == AddRebarSets(rsetTags));
		}
		return true;
	}

	long LDSlabRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

	bool LDSlabRebarAssembly::OnDoubleClick()
	{
		EditElementHandle ehSel;
		if (!PIT::GetAssemblySelectElement(ehSel, this))
		{
			return false;
		}

		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		pSlabDoubleRebarDlg = new CSlabRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
		pSlabDoubleRebarDlg->SetSelectElement(ehSel);
		pSlabDoubleRebarDlg->Create(IDD_DIALOG_SlabRebar);
		pSlabDoubleRebarDlg->SetConcreteId(FetchConcrete());
		pSlabDoubleRebarDlg->ShowWindow(SW_SHOW);

		// 	CSlabRebarDlg dlg(ehSel, CWnd::FromHandle(MSWIND));
		// 
		// 	//	dlg.SetSelectElement(ehSel);
		// 	dlg.SetConcreteId(FetchConcrete());
		// 	if (IDCANCEL == dlg.DoModal())
		return false;

		return true;
	}

	bool LDSlabRebarAssembly::Rebuild()
	{
		if (!GetSelectedModel())
			return false;

		ElementHandle ehslab(GetSelectedElement(), GetSelectedModel());
		if (!ehslab.IsValid())
			return false;

		DgnModelRefP modelRef = ehslab.GetModelRef();

		SetSlabData(ehslab);

		MakeRebars(modelRef);//调用创建钢筋
		Save(modelRef);

		ElementId contid = FetchConcrete();
		return true;
	}



	void LDSlabRebarAssembly::SetSlabRebarDir(DSegment3d& Seg, ArcRebar& mArcLine)//板配筋的方向
	{

	}

	//局部配筋平面在XOZ平面
	bool LDSlabRebarAssembly::SetSlabData(ElementHandleCR eh)
	{
		bool bRet = AnalyzingSlabGeometricData(eh);
		if (!bRet)
			return false;
		PopvecFrontPts().push_back(m_STslabData.ptStart);
		PopvecFrontPts().push_back(m_STslabData.ptEnd);
		DPoint3d ptStart, ptEnd;
		DPoint3d ptOrgin = m_STslabData.ptStart;
	
		ptStart = m_STslabData.ptStart;
		ptEnd = m_STslabData.ptEnd;

		DVec3d tmpz = m_STslabData.vecZ;
		tmpz.Normalize();

		CVector3D  yVec = tmpz;     //返回两个向量的（标量）叉积。y  	
		yVec.Scale(-1);
		CVector3D  xVecNew(ptStart, ptEnd);
		xVecNew.Normalize();
		bool isXtY = false;
		tmpz.Scale(m_STslabData.width);
		ptOrgin.Add(tmpz);
		BeMatrix   placement = CMatrix3D::Ucs(ptOrgin, xVecNew, yVec, isXtY);		//方向为X轴，水平垂直方向为Y轴
		//placement.SetScaleFactors(1, 1, -1);
		SetPlacement(placement);
		PopvecFrontPts().push_back(ptStart);
		PopvecFrontPts().push_back(ptEnd);
		SetSelectedElement(eh.GetElementId());
		SetSelectedModel(eh.GetModelRef());
		return true;
	}

	//获取与板顶与板底的墙面
	void GetUpDownWallFaces(LDSlabRebarAssembly::LDFloorData& m_ldfoordata,bool isCombineFloor = false)
	{
		MSElementDescrP downface = nullptr;
		mdlElmdscr_duplicate(&downface, m_ldfoordata.facedes);

		DVec3d vecZ = DVec3d::From(0, 0, 1);
		double FHight = m_ldfoordata.Zlenth + 10;
		vecZ.Scale(FHight);
		MSElementDescrP upface = nullptr;
		mdlElmdscr_duplicate(&upface, m_ldfoordata.facedes);
		PITCommonTool::CFaceTool::MoveCenterFaceByMidPt(upface, vecZ);
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
		mdlTMatrix_scale(&matrix, &matrix, 0.9999, 0.9999, 1.0);
		mdlTMatrix_setOrigin(&matrix, &ptCenter);
		TransformInfo tInfo(matrix);
		eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, tInfo);
		//mdlElmdscr_add(eeh.GetElementDescrP());
		m_ldfoordata.downnum = 0;
		m_ldfoordata.upnum = 0;
		std::vector<IDandModelref>  Same_Eles;
		GetNowScanElems(eeh, Same_Eles);
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
					MSElementDescrP resultEdp = nullptr;
					DVec3d m_moveDir = DVec3d::From(0, 0, -1000);
					Transform tMatrix;
					mdlTMatrix_getIdentity(&tMatrix);
					mdlTMatrix_setTranslation(&tMatrix, &m_moveDir);
					mdlElmdscr_transform(&downface, &tMatrix);

					//mdlElmdscr_add(downface);
					PITCommonTool::CSolidTool::SolidBoolWithFace(resultEdp, downface, Eleeh.GetElementDescrP(),
						BOOLOPERATION_INTERSECT);

					DRange3d faceRange, eehRange;
					mdlElmdscr_computeRange(&faceRange.low, &faceRange.high, downface, NULL);
					mdlElmdscr_computeRange(&eehRange.low, &eehRange.high, tmpeeh.GetElementDescrP(), NULL);
					//PITCommonTool::CPointTool::DrowOnePoint(eehRange.high, 1, 1);//红
					//PITCommonTool::CPointTool::DrowOnePoint(faceRange.high, 1, 2);//黄
					//如果墙方向不一致，或者没有交点面，或者位置错误退出
					if (resultEdp == nullptr )
					{
						continue;
					}

					if (!ExtractFacesTool::GetFloorDownFace(Eleeh, upface, maxpt, false,downface))
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
		mdlTMatrix_scale(&matrix, &matrix, 0.99999, 0.99999, 1.0);
		mdlTMatrix_setOrigin(&matrix, &ptCenter);
		TransformInfo tInfo2(matrix);
		eehup.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehup, tInfo2);

		std::vector<IDandModelref>  Same_Elesup;
		GetNowScanElems(eehup, Same_Elesup);
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
					MSElementDescrP resultEdp = nullptr;

					DVec3d m_moveDir = DVec3d::From(0, 0, 1000);
					Transform tMatrix;
					mdlTMatrix_getIdentity(&tMatrix);
					mdlTMatrix_setTranslation(&tMatrix, &m_moveDir);
					mdlElmdscr_transform(&upface, &tMatrix);

					PITCommonTool::CSolidTool::SolidBoolWithFace(resultEdp, upface, Eleeh.GetElementDescrP(),
						BOOLOPERATION_INTERSECT);

					DRange3d faceRange, eehRange;
					mdlElmdscr_computeRange(&faceRange.low, &faceRange.high, upface, NULL);
					mdlElmdscr_computeRange(&eehRange.low, &eehRange.high, tmpeeh.GetElementDescrP(), NULL);

					//如果墙方向不一致，或者没有交点面，或者位置错误退出
					//判断两个range是否有公共的相交的位置
					if (nullptr == resultEdp ) 
					{
						// 如果没有交集，跳过当前循环
						continue;
					}
					else
					{
						if (!ExtractFacesTool::GetFloorDownFace(Eleeh, dface, minpt, true, upface))
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

	}


	bool LDSlabRebarAssembly::AnalyzingSlabGeometricData(ElementHandleCR eh)
	{
		double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		std::vector<EditElementHandle*> Negs;

		DgnModelRefP model = eh.GetModelRef();
		m_model = model;

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
		DPoint3d minP2, maxP2;
		//计算指定元素描述符中元素的范围。
		mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
		if (!Eleeh.IsValid())
		{
			mdlDialog_dmsgsPrint(L"非法的板实体!");
			return false;
		}
		if (m_pOldElm == NULL)
		{
			m_pOldElm = new EditElementHandle();
		}
		m_pOldElm->Duplicate(Eleeh);
		DPoint3d minPos;
		EditElementHandle downface;
		if (!ExtractFacesTool::GetFloorDownFaceForSlab(copyEleeh, downface, minPos,true))
		{
			return false;
		}
		//downface.AddToModel();
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
		mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
		DPoint3d minP, maxP;
		//计算指定元素描述符中元素的范围。
		mdlElmdscr_computeRange(&minP, &maxP, downface.GetElementDescrP(), NULL);
		trans.InverseOf(trans);
		downface.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(downface, TransformInfo(trans));
		m_ldfoordata.Ylenth = (maxP.y - minP.y)*uor_now / uor_ref;
		m_ldfoordata.Xlenth = (maxP.x - minP.x)*uor_now / uor_ref;
		m_ldfoordata.Zlenth = (maxP2.z - minP2.z)*uor_now / uor_ref;
		m_ldfoordata.oriPt = minP;
		m_ldfoordata.Vec = DVec3d::From(facenormal.x, facenormal.y, facenormal.z);
		m_ldfoordata.facedes = downface.ExtractElementDescr();
		bool isCombine = false;
		if (m_isCombineFloor)
			isCombine = true;
		GetUpDownWallFaces(m_ldfoordata, isCombine);

		//计算整个板的参数
		m_STslabData.height = (maxP.y - minP.y)*uor_now / uor_ref;
		m_STslabData.length = (maxP.x - minP.x)*uor_now / uor_ref;
		m_STslabData.width = (maxP2.z - minP2.z)*uor_now / uor_ref;
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
		m_STslabData.vecZ = ptPreEnd - ptPreStr;
		m_STslabData.vecZ.Normalize();


		m_STwallData.height = m_STslabData.height;
		m_STwallData.length = m_STslabData.length;
		m_STwallData.ptStart = m_STslabData.ptStart;
		m_STwallData.ptEnd = m_STslabData.ptEnd;
		m_STwallData.ptPreStr = ptPreStr;
		m_STwallData.ptPreEnd = ptPreEnd;
		m_Holeehs = Holeehs;
		m_height = m_STslabData.width;
		m_STwallData.width = m_STslabData.width;

		return true;
	}
}




