#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	����
*	Project:		��ά����ͼ��Ŀ
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
	Custom,		//�Զ�����ʽ
};

enum TieFaceRebarStyle
{
	TWOFOUR,	//200*400
	FOURFOUR,	//400*400
};


struct TieRebarData
{
	BrString rebarSize;				//�ֽ�ߴ�
	double rebarSpacing;			//�ֽ���
};

struct FaceRebarData
{
	TieRebarData HRebarData;			//������ֽ�����(���ֽ�)
	TieRebarData VRebarData;			//��һ����ս�����(�ڲ�ֽ�)
};

struct FaceRebarDataArray			//�������������
{
// 	double positiveCover;			//���汣����
// 	double reverseCover;			//���汣����
// 	double sideCover;				//���汣����
// 	double length;					//��
// 	double width;					//��
// 	double height;					//��
	FaceRebarData	posRebarData;	//����ֽ�����
	FaceRebarData	revRebarData;	//����ֽ�����
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

//����
class TieRebarMaker :RebarMakerFactory
{
public:
	ElementId m_CallerId;
public:
	//�ֽ�ߴ����ݣ��ֽ�ʵ��������յ�
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
	void	SetHoles(vector<EditElementHandle*> vecHoles);//������ʵ��
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
	RebarCurve MakeOneTieRebar(DPoint3d ptStart, DPoint3d ptEnd, double angle1, double angle2, const CVector3D& endNormal,int tieRebar, bool bdouble, DgnModelRefP modelRef = ACTIVEMODEL);;//��������ʹ�á�

	bool MakeTieRebar(std::vector<RebarCurve>& vecRebarCurve, const RebarInsectionPt& rebarInsec, RebarEndTypes const& endTypes, DgnModelRefP modelRef);

	bool MakeTieRebar(std::vector<RebarCurve>& vecRebarCurve, const RebarInsectionPt& rebarInsec, double angle1, double angle2, DgnModelRefP modelRef,int TieReabarMethod, bool bdoubleTie = false);//��������ʹ�á��Ƕȿɱ�

//	RebarSetTag* MakeTieRebars(ElementId rebarSetId, DgnModelRefP modelRef);

private:
	FaceRebarDataArray							m_faceRebarDataArray;
	vector<vector<DSegment3d> >					m_vecStartEnd;
	TieRebarStyle								m_style;
	TieFaceRebarStyle							m_TieFaceStyle;//�����ʹ��
	BrString									m_tieRebarSize;
	int											m_rowInterval;
	int											m_colInterval;

	Transform									m_trans;
	vector<EditElementHandle*>					m_vecHoles;
	double										m_sideCover;

	CVector3D									m_MainVec;
	CVector3D									m_AnthorVec;
	CVector3D									m_DownVec;//�������

	int											m_modelType;		// 0 : ǽ 1: ��

	DPoint3d									m_arcCenter;
	DPoint3d									m_arcStr;
	DPoint3d									m_arcEnd;

	vector<vector<RebarPoint> >					m_arcVecStartEnd;
	vector<vector<DSegment3d> >					m_vecRebarStartEnd;	//����ֱ�ߵ�
	CString LevelName;
};

