#pragma once
#include "RebarMaker.h"
#include "CommonFile.h"
#include "PITRebarCurve.h"
#include "RebarHelper.h"
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	StirrupRebar
*	Project:		��ά����ͼ��Ŀ
*	Author:			LiuXiang
*	Date:			2021/07/15
	Version:		V1.0
*	Description:	StirrupRebar
*	History:
*	1. Date:		2021/07/15
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/
namespace PIT
{
	struct StirrupRebarData : public PIT::RebarData
	{
		EndType beg;
		EndType end;
	};
	BE_DEFINE_SUFFIX_TYPEDEFS(StirrupRebarData)

	class StirrupRebar : public PIT::PITRebar
	{
		BE_DATA_VALUE(PIT::StirrupRebarData, rebarData)
		BE_DATA_VALUE(vector<CPoint3D>, vecRebarPts)
	public:
		static RebarElement* CreateStirrupRebar(RebarSetR rebarSet, const vector<CPoint3D>& vecRebarPts, PIT::StirrupRebarDataCR rebarData);

	public:
		explicit StirrupRebar(const vector<CPoint3D> &vecRebarPts, PIT::StirrupRebarDataCR rebarData);
		~StirrupRebar();
		ElementId DrawRefLine(DgnModelRefP modelRef);
		RebarElement*  Create(RebarSetR rebarSet);
	private:
		PIT::PITRebarCurve  m_Curve;
	};
	BE_DEFINE_SUFFIX_TYPEDEFS(StirrupRebar)

	class StirrupRebarMaker : public RebarMakerFactory
	{
		BE_DATA_VALUE(bool, bUp)
	public:
		/*
		���������ֽ�ȷ��U�͸ֽ��λ�ã�����ֽ�����ȷ��U�͸ֽ�ĸ���
		*/
		StirrupRebarMaker(const vector<ElementId>& rebar_Vs, const vector<ElementId>  &rebarId_Hs, PIT::StirrupRebarDataCR rebarData, bool bUp = false, DgnModelRefP modelRef = ACTIVEMODEL);
		virtual ~StirrupRebarMaker();
		std::string mSelectedRebarType;//�������ĸֽ���������
	private:
		bool	CalStirrupRebarPts(vector<CPoint3D> &pts, vector<ElementId> rebar_Vs, ElementId rebar_H, PIT::StirrupRebarDataCR rebarData, bool up, DgnModelRefP modeRef);
	public:
		virtual RebarSetTag* MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);

		bool	UpdateStirrupRebar(vector<CPoint3D>& pts, const vector<ElementId>& rebar_Vs, ElementId rebar_H, PIT::StirrupRebarDataCR rebarData, bool up, int rebarIndex, DgnModelRefP modeRef);

		bool	UpdateStirrupRebars(const vector<ElementId>& rebar_Vs, const vector<ElementId>  &rebarId_Hs, PIT::StirrupRebarDataCR rebarData, bool up, DgnModelRefP modeRef);


		bool DrawRefLine(ElementId &id, int index, DgnModelRefP modelRef);

		bool DrawRefLine(vector<ElementId> &ids, DgnModelRefP modelRef);

		vector<RebarHelper::RebarArcData> GetRebarArcDatas() { return m_rebarArcDatas; }

	private:
		vector<shared_ptr<StirrupRebar> >	m_pStirrupRebars;
		vector<vector<CPoint3D>	>			m_vecStirrupRebarPts;
		vector<RebarHelper::RebarArcData>				m_rebarArcDatas; //�ֽ�������������
	};
}