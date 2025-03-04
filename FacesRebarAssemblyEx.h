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
	vector<vector<DSegment3d> > m_vecRebarStartEnd;	//规避了孔洞的所有点
	vector<vector<PIT::PITRebarCurve>> m_vecRebarCurvePt;	//钢筋预览线的所有点
	vector<ElementRefP> m_allPreViewEehs;//预览按钮按下后的所有钢筋线点元素
	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;//未规避孔洞的所有点
	void ClearLines();
	bool m_isClearLine = true;

public:
	FacesRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~FacesRebarAssemblyEx();

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