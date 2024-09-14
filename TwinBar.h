#pragma once
#include "RebarMaker.h"
#include "CommonFile.h"
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	TwinBar
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/07/14
	Version:		V1.0
*	Description:	TwinBar
*	History:
*	1. Date:		2021/07/14
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/
namespace PIT
{
	class TwinRebar : public RebarElement
	{
		BE_DATA_VALUE(PIT::RebarData, rebarData)
		BE_DATA_VALUE(DVec3d,	vec)

	public:
		static RebarElement* CreateTwinRebar(RebarSetR rebarSet, RebarElementCP rebar, PIT::RebarDataCR rebarData, DVec3dCR vec);

	public:
		explicit TwinRebar(RebarElementCP pOrgRebar,RebarDataCR rebarData,DVec3dCR vec);
		~TwinRebar();
		ElementId DrawRefLine();
		RebarElement*  Create(RebarSetR rebarSet);
	private:
		RebarElementCP	m_pOrgRebar;
		bool			m_bShowRefLine;
	};
	BE_DEFINE_SUFFIX_TYPEDEFS(TwinRebar)

	class TwinBarMaker : public RebarMakerFactory
	{
		BE_DATA_VALUE(bool, bNegate)
		BE_DATA_VALUE(int, interval)
	public:
		explicit TwinBarMaker(ElementId OrgRebarId, PIT::RebarDataCR twinRebarData, DVec3dCR vec, DgnModelRefP modelRef = ACTIVEMODEL);
		TwinBarMaker(const std::vector<std::pair<ElementId, PIT::RebarData> > &twinRebars, DVec3dCR vec, DgnModelRefP modelRef = ACTIVEMODEL);
		virtual ~TwinBarMaker();
	public:
		virtual RebarSetTag* MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);

//		void AddRebar(TwinRebarP rebar);

	private:
		RebarSetP						m_pRebarSet;
		std::vector<std::shared_ptr<TwinRebar> > m_vecTwinRebar;
// 		vector<ElementId> m_vecElm;
// 		vector<PIT::RebarData> m_vecTwinRebarData;
	};

}