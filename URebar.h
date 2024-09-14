#pragma once
#include "RebarMaker.h"
#include "PITRebarCurve.h"
#include "CommonFile.h"

/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	URebar
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/07/15
	Version:		V1.0
*	Description:	URebar
*	History:
*	1. Date:		2021/07/15
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/
namespace PIT
{
	
	struct URebarData: public PIT::RebarData
	{
		double legLength1{0.0};
		double legLength2{0.0};
		bool isArc{false};
		DPoint3d arcCenter{0.0,0.0,0.0};
	};
	BE_DEFINE_SUFFIX_TYPEDEFS(URebarData)

	class URebar : public RebarElement
	{
		BE_DATA_VALUE(PIT::URebarData, rebarData)
		BE_DATA_VALUE(vector<CPoint3D>, vecRebarPts)
	public:
		static RebarElement* CreateURebar(RebarSetR rebarSet, const vector<CPoint3D>& vecRebarPts, PIT::URebarDataCR rebarData);

	public:
		DVec3d							m_rebarVec_V;
		explicit URebar(const vector<CPoint3D> &vecRebarPts, URebarDataCR rebarData);
		~URebar();
		ElementId DrawRefLine(DgnModelRefP modelRef);
		ElementId DrawRefArcStrLine(DgnModelRefP modelRef);
		ElementId DrawRefArcEndLine(DgnModelRefP modelRef);
		
		RebarElement*  Create(RebarSetR rebarSet);

	private:
		PIT::PITRebarCurve  m_Curve;
	};
	BE_DEFINE_SUFFIX_TYPEDEFS(URebar)

	class URebarMaker : public RebarMakerFactory
	{
		BE_DATA_VALUE(bool,		bNegate)
		BE_DATA_VALUE(bool,		bUp)
		BE_DATA_VALUE(bool,		bInner)
	public:
		/*
		纵向两根钢筋确定U型钢筋的位置，横向钢筋数组确定U型钢筋的根数
		*/
		URebarMaker(ElementId  rebar_V1, ElementId  rebar_V2, const vector<ElementId>  &rebarId_H,
			PIT::URebarDataR rebarData, vector<EditElementHandle*> holeEehs,
			bool bNegate = false,bool bUp = false, bool bInner = false, DgnModelRefP modelRef = ACTIVEMODEL);
		URebarMaker(const vector<ElementId>  &rebarId_H, PIT::URebarDataR rebarData,
			vector<EditElementHandle*> holeEehs, const DVec3d& rebarVec, bool bNegate = false, bool bUp = false,
			bool isEnd = false, bool bInner = false, DgnModelRefP modelRef = ACTIVEMODEL);
		virtual ~URebarMaker();

	private:
		bool	CalURebarLeftRightDownPts(ElementId  rebar_V1, ElementId  rebar_V2, ElementId rebar_H, PIT::URebarDataR rebarData, DgnModelRefP modelRef);
		bool	CalURebarPts(vector<CPoint3D> &pts, DPoint3d ptLeftDown, DPoint3d ptRightDown, PIT::URebarDataCR rebarData, bool negate, bool up, DgnModelRefP modelRef);
	public:
		virtual RebarSetTag* MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);

		bool	UpdateURebar(vector<CPoint3D>& pts, DPoint3d ptLeftDown, DPoint3d ptRightDown, PIT::URebarDataCR rebarData,bool negate, bool up, int rebarIndex, DgnModelRefP modelRef);

		bool	UpdateURebars(PIT::URebarDataCR rebarData, DgnModelRefP modelRef);


		bool DrawRefLine(ElementId &id, int index, DgnModelRefP modelRef);

		bool DrawRefLine(vector<ElementId> &ids, DgnModelRefP modelRef);

//		void AddRebar(TwinRebarP rebar);
		
		void GetArcCenter(PIT::URebarDataR rebarData, RebarCurve curve_H, double BendRadius);

		void SetHoles(const vector<EditElementHandle*>& holes) { m_holes = holes; }

	private:
		vector<shared_ptr<URebar> >		m_vecURebar;

		BrString						m_RebarSize_V1;
		BrString						m_RebarSize_V2;
		BrString						m_RebarSize_H;
		DVec3d							m_rebarVec_H;
		DVec3d							m_rebarVec_V;
		vector<DPoint3d>				m_ptLeftDowns;
		vector<DPoint3d>				m_ptRightDowns;

		vector<vector<CPoint3D>	>		m_vecURebarPts;
		vector<EditElementHandle*>		m_holes;
	};

}

void CalculateArcCuve(PIT::PITRebarCurve& tmpCurve, DPoint3d PtStr, DPoint3d PtEnd, double Lenth, DPoint3d CenterPt, DVec3d rebarVec_V, bool isStr);

