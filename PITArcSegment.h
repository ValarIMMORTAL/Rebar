#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	PITArcSegment
*	Project:		��ά����ͼ��Ŀ
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
		double   dRadiusOut;	  // ��Բ�뾶
		double   dRadiusInn;	  // ��Բ�뾶
		double	 dHeight;		  // �߶�
		DPoint3d centerpt;		  // �������ĵ�
		DPoint3d ArcDPs[2];		  // �������յ�
		UInt16       type;         //Ԫ������

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
		DPoint3d	ptStart;	//�����
		DPoint3d	ptMid;		//���е�
		DPoint3d	ptEnd;		//���յ�
		DPoint3d	ptCenter;	//Բ��
		DPoint3d    FptStart;	//�ı仡֮ǰ���
		DPoint3d    FptEnd;		//�ı仡֮ǰ�յ�
		double		dRadius;	//�뾶
		double		dLen;		//����

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

		const DPoint3d GetCenter() const; //�߶��е�

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
		DSegment3d	seg;		//�߶�
	};

	typedef vector<LineSegment> LineSegVct;
}
