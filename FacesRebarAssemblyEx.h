#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	墙配筋
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/06/08
	Version:		V1.0
*	Description:	FacesRebarAssemblyEx
*	History:
*	1. Date:		2021/06/08
*	Author:			LiuXiang
*	Modification:	create file
*	Edit： Liu Silei 2025/2/24
*
**************************************************************/

#include <RebarElements.h>
#include "CommonFile.h"
#include "PITRebarEndType.h"
#include "PITRebarCurve.h"
#include "PITArcSegment.h"
#include "PITRebarAssembly.h"


class FacesRebarAssemblyEx : public PIT::PITRebarAssembly
{
public:
	enum FaceType
	{
		Plane,
		CamberedSurface,
		other,
	};
	double				_d;
	BE_DATA_REFER(BeMatrix, Placement)              //当前局部坐标原点
		BE_DATA_VALUE(PIT::Concrete, Concrete)				//保护层
		BE_DATA_VALUE(vector<PIT::ConcreteRebar>, MainRebars)				//主筋
		BE_DATA_VALUE(vector<vector<PIT::EndType> >, vecEndTypes)			//端部样式
		BE_DATA_VALUE(vector<ElementId>, SetIds)					//SetId
		BE_DATA_VALUE(DVec3d, faceNormal)				//平面法向
		BE_DATA_VALUE(bool, isReverse)
		BE_DATA_VALUE(FaceType, faceType)
		BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)     //前面线的所有点

public:
	std::vector<EditElementHandle* > m_Holeehs;					//孔洞
	std::vector<EditElementHandle* > m_Negs;					//负实体
	std::vector<EditElementHandle* > m_useHoleehs;				//筛选后的孔洞和负实体
	std::vector<DPoint3d>			 m_vecRebarPtsLayer;		//被孔洞截取前的所有钢筋点数据
	std::vector<DPoint3d>			 m_vecTwinRebarPtsLayer;	//被孔洞截取前的所有并筋点数据

	ElementHandle					m_face;
	EditElementHandle				*m_Solid;
	std::vector<ElementHandle>		m_vecElm;
	bool							m_slabUpFace;
public:
	FacesRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~FacesRebarAssemblyEx();

public:
	vector<vector<DSegment3d> > m_vecRebarStartEnd;	//规避了孔洞的所有点
	vector<vector<PIT::PITRebarCurve>> m_vecRebarCurvePt;	//钢筋预览线的所有点
	vector<ElementRefP> m_allPreViewEehs;//预览按钮按下后的所有钢筋线点元素
	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;//未规避孔洞的所有点
	void ClearLines();
	bool m_isClearLine = true;

protected:
	void			Init();
	void			movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);
	void			DrawPoint(const DPoint3d& point, int color, EditElementHandle& eehPoint, DgnModelRefP modelRef);
	void			DrawPreviewLines();

protected:
	virtual int		GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Face; }

public:
	static FaceType JudgeFaceType(EditElementHandleR eehFace, DgnModelRefP modelRef);

	virtual bool	AnalyzingFaceGeometricData(EditElementHandleR eeh) { return true; }

	virtual bool	MakeRebars(DgnModelRefP modelRef) { return true; }

	void SetRebarEndTypes(vector<PIT::EndType> const& vvecEndTypes);

	bool m_UseXOYDir;//是否应用XOY方向作为面钢筋生成方向
	void Setd(double d) { _d = d; }
};

class MultiPlaneRebarAssemblyEx : public FacesRebarAssemblyEx
{
public:
	MultiPlaneRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL);
	~MultiPlaneRebarAssemblyEx();

	struct LinePoint
	{
		DPoint3d ptPoint;
		int		 iIndex;
	}stLinePoint;

	struct LineSeg
	{
		int				  iIndex;
		PIT::LineSegment  LineSeg1; //基准线段，若平面垂直与XOY平面，则为底边，若平面平行与XOY平面，则为左边
		PIT::LineSegment  LineSeg2;	//基准线段，平面内过LineSeg1的起点垂直与LineSeg的方向的线段
	}stLineSeg;
public:
	virtual bool AnalyzingFaceGeometricData(EditElementHandleR eeh);

	virtual bool MakeRebars(DgnModelRefP modelRef);

	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, vector<DPoint3d>& rebarPoint, int iIndex, const PIT::PITRebarEndTypes& endTypes, double bendRadius);

	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, vector<DPoint3d>& rebarPoint, vector<int>& vecIndex, const PIT::PITRebarEndTypes& endTypes, double bendRadius);

	void SortLineSegVec(vector<LineSeg>& m_vecLineSeg, double uor_per_mm);

	void SortLineSeg(vector<LineSeg>&	m_vecLineSeg, double uor_per_mm);

	void CmdSort(vector<DPoint3d>& vecPoint);

	void AddFace(EditElementHandleR faceEeh) { m_vecFace.push_back(faceEeh); }

	RebarSetTag* MakeRebars
	(
		ElementId&					rebarSetId,
		vector<PIT::LineSegment>&	vecRebarLine,
		vector<PIT::LineSegment>&	vec,
		int							dir,
		BrStringCR					sizeKey,
		double						spacing,
		double						startOffset,
		double						endOffset,
		int							level,
		int							grade,
		int							DataExchange,
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
		vector<CVector3D> const&	vecEndNormal,
		DgnModelRefP				modelRef
	);

	void CalculateUseHoles(DgnModelRefP modelRef);

	bool JudgeMergeType(PIT::LineSegment& LineSeg1, PIT::LineSegment& LineSeg2, double uor_per_mm);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Plane; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"MultiPlane Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"MultiPlane Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(MultiPlaneRebarAssemblyEx, RebarAssembly)

private:
	vector<LineSeg>			m_vecLineSeg;
	vector<ElementHandle>	m_vecFace;
	vector<DVec3d>			m_vecFaceNormal;

	DPoint3d				m_ComVec;
	double					m_offset;

};


class PlaneRebarAssemblyEx : public FacesRebarAssemblyEx
{
	BE_DATA_VALUE(PIT::LineSegment, LineSeg1)	//基准线段，若平面垂直与XOY平面，则为底边，若平面平行与XOY平面，则为左边
		BE_DATA_VALUE(PIT::LineSegment, LineSeg2)	//基准线段，平面内过LineSeg1的起点垂直与LineSeg的方向的线段

	//	bool				_bAnchor;
		unsigned short		_anchorPos = 0;			//直锚位置
	ElementHandle		_ehCrossPlanePre;
	ElementHandle		_ehCrossPlaneNext;
	unsigned short		_bendPos = 0;			//90度弯曲位置

public:
	PlaneRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL);
	~PlaneRebarAssemblyEx();

private:
	//根据钢筋点信息构建ebarCurve
	//vector<PIT::PITRebarCurve>& rebar 得到的RebarCurve
	//PIT::PITRebarEndTypes& endTypes 钢筋点信息和端部信息
	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebar, const PIT::PITRebarEndTypes& endTypes);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Plane; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Plane Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Plane Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:
	RebarSetTag* MakeRebars(ElementId& rebarSetId, PIT::LineSegment rebarLine, PIT::LineSegment vec, int dir, BrStringCR sizeKey, double spacing, double startOffset, double endOffset, int	level, int grade, int DataExchange, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, DgnModelRefP modelRef);

	//计算孔洞
	void CalculateUseHoles(DgnModelRefP modelRef);

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(PlaneRebarAssemblyEx, RebarAssembly)

public:
	//分析面的信息，主要分析出m_lineseg1和m_lineseg2，横向和纵向钢筋的先段
	virtual bool	AnalyzingFaceGeometricData(EditElementHandleR eeh);
	//创建钢筋的主要入口
	virtual bool	MakeRebars(DgnModelRefP modelRef);
	//	void SetAnchor(bool anchor) { _bAnchor = anchor; }
	void SetCrossPlanePre(ElementHandleCR eh) { _ehCrossPlanePre = eh; }
	void SetCrossPlaneNext(ElementHandleCR eh) { _ehCrossPlaneNext = eh; }


};


class CamberedSurfaceRebarAssemblyEx : public FacesRebarAssemblyEx
{
public:
	// 	enum ArcType
	// 	{
	// 		CurvedSurface,	//曲面
	// 		Torus			//圆环面
	// 	};
	BE_DATA_VALUE(PIT::ArcSegment, ArcSeg)			//弧数据
		BE_DATA_VALUE(double, height)					//弧面高度
		//	BE_DATA_VALUE(ArcType,	arcType)			//弧面类型
		double		_dOuterArcRadius;
private:

	bool CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);

	bool makeLineRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::ArcSegment arcSeg, double dLen, double space, double startOffset, double endOffset, PIT::PITRebarEndTypes& endTypes);

	bool makeArcWallRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::ArcSegment arcSeg, double space, double startOffset, double endOffset, PIT::PITRebarEndTypes& endTypes);

	RebarSetTag* MakeRebars_Line
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		PIT::ArcSegment		arcSeg,
		double              dLen,
		double              spacing,
		double              startOffset,
		double              endOffset,
		int					level,
		int					grade,
		int					DataExchange,
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
		vector<CVector3D> const& vecEndNormal,
		DgnModelRefP        modelRef
	);

	RebarSetTag* MakeRebars_Arc
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		PIT::ArcSegment		arcSeg,
		double              spacing,
		double              startOffset,
		double              endOffset,
		int					level,
		int					grade,
		int					DataExchange,
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
		vector<CVector3D> const& vecEndNormal,
		DgnModelRefP        modelRef
	);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::CamberedSurface; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

	void CalculateUseHoles(DgnModelRefP modelRef);

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CamberedSurfaceRebarAssemblyEx, RebarAssembly)

public:
	CamberedSurfaceRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL)
		:FacesRebarAssemblyEx(id, modelRef), _dOuterArcRadius(0.0)
	{
	};

	~CamberedSurfaceRebarAssemblyEx()
	{
	};

	virtual bool	AnalyzingFaceGeometricData(EditElementHandleR eeh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);
};