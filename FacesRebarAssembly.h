#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	墙配筋
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/06/08
	Version:		V1.0
*	Description:	FacesRebarAssembly
*	History:
*	1. Date:		2021/06/08
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/

#include <RebarElements.h>
#include "CommonFile.h"
#include "PITRebarEndType.h"
#include "PITRebarCurve.h"
#include "PITArcSegment.h"
#include "PITRebarAssembly.h"
#include "ScanIntersectTool.h"
#include "PITRebarCurve.h"
#include "PITRebarEndType.h"


class FacesRebarAssembly : public PIT::PITRebarAssembly
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
	BE_DATA_VALUE(vector<PIT::ConcreteRebar>,	MainRebars)				//主筋
	BE_DATA_VALUE(vector<vector<PIT::EndType> >, vecEndTypes)			//端部样式
	BE_DATA_VALUE(vector<ElementId>,		SetIds)					//SetId
	BE_DATA_VALUE(vector<ElementId>, vecSetId)								//rebarelementSet的每个Id
	BE_DATA_VALUE(DVec3d,					faceNormal)				//平面法向
	BE_DATA_VALUE(bool,						isReverse)
	BE_DATA_VALUE(FaceType,					faceType)
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

	bool m_bisSump;

	bool m_isCatchpit = false;//是否是集水坑配筋

	int m_CatchpitType = -1;//0是标准集水坑，1是特殊集水坑，2是双集水坑

	vector<ElementHandle> m_AllFloors;//保留集水坑上的板

	//double m_UpFloor_Height = 0.0;//集水坑上方板的高度

	vector <EditElementHandle*> m_ScanedFloors;//板的实体

	map<ElementHandle*, double> m_mapFloorAndHeight;//集水坑上面的板对应的高度

	RebarSetTagArray m_rsetTags;

	int m_solidType = -1; //0：板，1为墙
	bvector<ISubEntityPtr> m_anchorFaces;//Z形板选择的锚入的面
	vector<EditElementHandle*> m_VeticalPlanes;//需要锚入的面
	EditElementHandle* m_CurentFace = nullptr;//当前的配筋面

	int m_facePlace = -1;//0：内侧面，1：外侧面

public:
	FacesRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~FacesRebarAssembly();

public:
	vector<vector<DSegment3d> > m_vecRebarStartEnd;	//规避了孔洞的所有点
	vector<ElementRefP> m_allLines;//预览按钮按下后的所有钢筋线
	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;//未规避孔洞的所有点
	void ClearLines();

	// @brief 分析外侧面竖直方向的钢筋是否需要变换锚固方向和锚固长度，;
	// @param range_eeh：		集水坑实体的range
	// @param range_CurFace：	当前配筋面的range
	// @return 如果需要返回true
	// @Add by tanjie, 2024.1.9
	bool AnalyseOutsideFace(DRange3d &range_eeh,DRange3d &range_CurFace);

protected:
	void			Init();
	void			movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);
	bool GetIntersectPointsWithOldElmOwner(vector<DPoint3d>& interPoints, EditElementHandleP oldElm, DPoint3d& ptstr, DPoint3d& ptend, double dSideCover);
	int IsHaveVerticalWall(DPoint3d ptstr, DPoint3d ptend, MSElementDescrP tmpfaces[20], int facenum, bool isGetFaceType = false);
	//是否有平行墙
	bool IsHaveParaWall(DPoint3d ptstr, DPoint3d ptend, MSElementDescrP tmpfaces[20], int facenum, bool isGetFaceType = false);

protected:
	virtual int		GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Face; }

public:
	static FaceType JudgeFaceType(EditElementHandleR eehFace, DgnModelRefP modelRef);

	virtual bool	AnalyzingFaceGeometricData(EditElementHandleR eeh) { return true; }
	virtual bool	AnalyzingFloorData(ElementHandleCR eh) { return true; }
	virtual bool	MakeRebars(DgnModelRefP modelRef) { return true; }
	virtual void	SetSelectedAnchorFace(bvector<ISubEntityPtr> m_faces) 
	{
		m_anchorFaces = m_faces;
	}

	virtual void    SetVerticalPlanes(vector<EditElementHandle*> faces)
	{
		m_VeticalPlanes = faces;
	}

	virtual void	SetCurentFace(EditElementHandle* face)
	{
		m_CurentFace = face;
	}

	void SetRebarEndTypes(vector<PIT::EndType> const& vvecEndTypes);

	bool m_UseXOYDir;//是否应用XOY方向作为面钢筋生成方向
	void Setd(double d) { _d = d; }
};
