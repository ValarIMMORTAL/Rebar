#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	拉筋
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/04/02
	Version:		V1.0
*	Description:	TieRebarMaker
*	History:
*	1. Date:		2021/04/02
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/
#include "RebarMaker.h"
#include "CommonFile.h"

enum TieRebarStyle
{
	XX,			//X*X
	XX2,		//X*2X
	X2X,		//2X*X
	X2X2,		//2X*2X
	Custom,		//自定义样式
};

enum TieFaceRebarStyle
{
	TWOFOUR,	//200*400
	FOURFOUR,	//400*400
};


struct TieRebarData
{
	BrString rebarSize;				//钢筋尺寸
	double rebarSpacing;			//钢筋间距
};

struct FaceRebarData
{
	TieRebarData HRebarData;			//主方向钢筋数据(外层钢筋)
	TieRebarData VRebarData;			//另一方向刚筋数据(内层钢筋)
};

struct FaceRebarDataArray			//待配拉筋构件数据
{
// 	double positiveCover;			//正面保护层
// 	double reverseCover;			//反面保护层
// 	double sideCover;				//侧面保护层
// 	double length;					//长
// 	double width;					//宽
// 	double height;					//高
	FaceRebarData	posRebarData;	//正面钢筋数据
	FaceRebarData	revRebarData;	//反面钢筋数据
};

struct TieRebarPt
{
	DPoint3d	pt;
	bool		isInHole;
};

struct RebarInsectionPt
{
	vector<vector<TieRebarPt> > vecInsecPtPositive;
	vector<vector<TieRebarPt> > vecInsecPtReverse;
};

//拉筋
class TieRebarMaker :RebarMakerFactory
{
public:
	ElementId m_CallerId;
public:
	//钢筋尺寸数据，钢筋实际起点与终点
	TieRebarMaker(const FaceRebarDataArray& faceRebarDataArray, const vector<vector<DSegment3d> >& vvecStartEnd, TieRebarStyle style, BrString rebarSize)
		:m_faceRebarDataArray(faceRebarDataArray), m_vecStartEnd(vvecStartEnd), m_style(style), m_tieRebarSize(rebarSize), m_rowInterval(0), m_colInterval(0)
	{
		m_modelType = 0;
	}

	TieRebarMaker(const FaceRebarDataArray& faceRebarDataArray, const vector<vector<DSegment3d> >& vvecStartEnd, TieFaceRebarStyle style, BrString rebarSize)
		:m_faceRebarDataArray(faceRebarDataArray), m_vecStartEnd(vvecStartEnd), m_TieFaceStyle(style), m_tieRebarSize(rebarSize), m_rowInterval(0), m_colInterval(0)
	{
		m_modelType = 0;
	}

	~TieRebarMaker() {}
public:
	virtual RebarSetTag* MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
	virtual RebarSetTag* MakeRebarforFace(ElementId rebarSetId, ElementId faceId, double angle1,double angle2,int TieRebarStyle, DgnModelRefP modelRef = ACTIVEMODEL);
	bool	SetCustomStyle(int rowInterval, int colInterval);
	void	SetTrans(Transform trans);
	void	SetHoles(vector<EditElementHandle*> vecHoles);//包括负实体
	void	SetHoleCover(double cover);

	void    CalaMainAnthorVec(const vector <DSegment3d> &vecPositiveStartEnd_M, const vector <DSegment3d> &vecPositiveStartEnd_A);

	void	SetDownVec(DPoint3d& ptStr, DPoint3d& ptEnd) { m_DownVec = ptEnd - ptStr; }

	void	SetArcPoint(DPoint3d& ptStr, DPoint3d& ptEnd, DPoint3d ptCenter)
	{
		m_arcCenter = ptCenter;
		m_arcStr = ptStr;
		m_arcEnd = ptEnd;
	}

	void	movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

	void	SetModeType(int modetype) { m_modelType = modetype; }

	void	MoveRebarLine(EditElementHandle& eehLine, DVec3d vec);

	void	SetArcStartEnd(vector<vector<RebarPoint> >&	arcVecStartEnd)
	{
		m_arcVecStartEnd = arcVecStartEnd;
	}
	void	GetRebarPts(vector<vector<DSegment3d>>& vctTieRebarLines)
	{
		vctTieRebarLines = m_vecRebarStartEnd;
	}

	void    SetRebarLevel(CString levelName)
	{
		LevelName = levelName;
	}

protected:
	virtual void	CalRebarIntersectionPointArc(RebarInsectionPt & rebarInsec, DgnModelRefP modelRef = ACTIVEMODEL);

	virtual void	CalRebarIntersectionPoint(RebarInsectionPt &rebarInsec, DgnModelRefP modelRef = ACTIVEMODEL);
	virtual void CalRebarIntersectionPoint(RebarInsectionPt &rebarInsec, RebarInsectionPt &rebarInsec2,int tieRebarMethod = 0, DgnModelRefP modelRef = ACTIVEMODEL);
protected:
	RebarCurve MakeOneTieRebar(DPoint3d ptStart, DPoint3d ptEnd, RebarEndTypes const& endTypes, const CVector3D& endNormal, DgnModelRefP modelRef = ACTIVEMODEL);
	RebarCurve MakeOneTieRebar(DPoint3d ptStart, DPoint3d ptEnd, double angle1, double angle2, const CVector3D& endNormal,int tieRebar, bool bdouble, DgnModelRefP modelRef = ACTIVEMODEL);;//按面拉筋使用。

	bool MakeTieRebar(std::vector<RebarCurve>& vecRebarCurve, const RebarInsectionPt& rebarInsec, RebarEndTypes const& endTypes, DgnModelRefP modelRef);

	bool MakeTieRebar(std::vector<RebarCurve>& vecRebarCurve, const RebarInsectionPt& rebarInsec, double angle1, double angle2, DgnModelRefP modelRef,int TieReabarMethod, bool bdoubleTie = false);//按面拉筋使用。角度可变

//	RebarSetTag* MakeTieRebars(ElementId rebarSetId, DgnModelRefP modelRef);

private:
	FaceRebarDataArray							m_faceRebarDataArray;
	vector<vector<DSegment3d> >					m_vecStartEnd;
	TieRebarStyle								m_style;
	TieFaceRebarStyle							m_TieFaceStyle;//面配筋使用
	BrString									m_tieRebarSize;
	int											m_rowInterval;
	int											m_colInterval;

	Transform									m_trans;
	vector<EditElementHandle*>					m_vecHoles;
	double										m_sideCover;

	CVector3D									m_MainVec;
	CVector3D									m_AnthorVec;
	CVector3D									m_DownVec;//底面最长边

	int											m_modelType;		// 0 : 墙 1: 板

	DPoint3d									m_arcCenter;
	DPoint3d									m_arcStr;
	DPoint3d									m_arcEnd;

	vector<vector<RebarPoint> >					m_arcVecStartEnd;
	vector<vector<DSegment3d> >					m_vecRebarStartEnd;	//所有直线点
	CString LevelName;
};

