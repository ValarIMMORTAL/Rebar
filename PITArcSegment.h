#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	PITArcSegment
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/06/08
	Version:		V1.0
*	Description:	ArcSegment
*	History:
*	1. Date:		2021/06/08
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/

#include <RebarElements.h>
#include "RebarCatalog.h"
#include "PITRebarEndType.h"
#include "RebarElements.h"
#include "LineSeg3D.h"
#include "RebarModel.h"
namespace PIT
{

	struct ELLWallGeometryInfo
	{
		double   dRadiusOut;	  // 外圆半径
		double   dRadiusInn;	  // 内圆半径
		double	 dHeight;		  // 高度
		DPoint3d centerpt;		  // 弧的中心点
		DPoint3d ArcDPs[2];		  // 弧起点和终点
		UInt16       type;         //元素类型

	};
	enum WallType
	{
		STWALL,
		GWALL,
		ARCWALL,
		ELLIPSEWall,
		Other
	};
	class ArcSegment
	{
	public:
		DPoint3d	ptStart;	//弧起点
		DPoint3d	ptMid;		//弧中点
		DPoint3d	ptEnd;		//弧终点
		DPoint3d	ptCenter;	//圆心
		DPoint3d    FptStart;	//改变弧之前起点
		DPoint3d    FptEnd;		//改变弧之前终点
		double		dRadius;	//半径
		double		dLen;		//弧长

		void ScaleToRadius(double dRadius);

		void Shorten(double dLength, bool start);

		void OffsetByAxisZ(double dLength);

		void CutArc(DPoint3d ptLineStart, DPoint3d ptLineEnd, ArcSegment *minArc = NULL);
	};

	class LineSegment
	{
	public:
		LineSegment() {};
		LineSegment(DPoint3dCR ptStart, DPoint3dCR ptEnd);
		LineSegment(DSegment3dCR segIn);

		void Shorten(double dLength, bool start);

		void PerpendicularOffset(double dLength, DVec3d vec = DVec3d::From(0,0,1));

		void CutLine(DSegment3dCR segCut, LineSegment *minArc = NULL);

		const DVec3d GetLineVec () const;

		const DPoint3d GetCenter() const; //线段中点

		const DSegment3d GetLineSeg() const;

		void SetLineSeg(const DSegment3d &segNew);

		const double GetLength() const;

		const DPoint3d GetLineStartPoint() const;
		void SetLineStartPoint(DPoint3dCR pt);
		const DPoint3d GetLineEndPoint() const;
		void  SetLineEndPoint(DPoint3dCR pt);

		DPoint3d Intersect(const LineSegment& segOther);
// 	public:
// 		LineSegmentArray operator+(const LineSegmentArray& lineSeg);
		bool hasSamePoint(const PIT::LineSegment& segCmp);
		bool IsEqual(const PIT::LineSegment& segCmp);

		int compareLength(const PIT::LineSegment& segCmp);

		bool compare(const PIT::LineSegment& seg1, const PIT::LineSegment& seg2);

	private:
		DSegment3d	seg;		//线段
	};

	typedef vector<LineSegment> LineSegVct;
}
