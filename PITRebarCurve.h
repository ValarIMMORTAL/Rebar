#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	PITRebarCurve
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/04/21
	Version:		V1.0
*	Description:	rebarCurve
*	History:
*	1. Date:		2021/04/21
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/

#include <RebarElements.h>
#include "RebarCatalog.h"
#include "PITRebarEndType.h"
#include "RebarElements.h"
#include "LineSeg3D.h"
#include "PITArcSegment.h"
#include "CommonFile.h"

namespace PIT
{
	void SetLevelidByRebarData(std::vector<PIT::ConcreteRebar>& vecRebarData);
	bool GetAssemblySelectElement(EditElementHandleR ehSel, RebarAssembly* assem);
	class PITRebarCurve :public RebarCurve
	{
	public:
		/*
		* @description: 生成钢筋弯曲部分的弧线
		* @param	seg			OUT		钢筋弯曲的弧线部分
		* @param	ptOrgin		IN		倒角点
		* @param	ptVec		IN		钢筋方向
		* @param	endNormal	IN		弯曲方向
		* @param	angle		IN		弯曲角度
		* @param	bendRadius	IN		弯曲半径
		*
		* @return	无
		* @author	LiuXiang
		* @Time		2021/4/21
		*/
		void GenerateArc(BeArcSegR seg, DPoint3dCR ptOrgin, DPoint3dCR ptVec, CVector3D endNormal, double angle, double bendRadius) const;

		//	void GetEndTypeVertex(std::vector<CPoint3D> &vex, BeArcSegCR seg, CVector3D endNormal, double len) const;

			/*
			* @description: 钢筋起点弯曲
			* @param	endType		IN		钢筋端部样式
			* @param	rebarVec	IN		钢筋方向
			* @param	angle		IN		钢筋弯曲角度
			*
			* @return	无
			* @author	LiuXiang
			* @Time		2021/4/21
			*/
		void RebarEndBendBeg(const PITRebarEndTypes & endType, DPoint3d rebarVec, double angle, bool isStirrup = false);

		void RebarEndBendBeg_Arc(const PITRebarEndTypes & endType, DPoint3d rebarVec, double angle, bool isStirrup = false);

		/*
		* @description: 钢筋终点弯曲
		* @param	endType		IN		钢筋端部样式
		* @param	rebarVec	IN		钢筋方向
		* @param	angle		IN		钢筋弯曲角度
		*
		* @return	无
		* @author	LiuXiang
		* @Time		2021/4/21
		*/
		void RebarEndBendEnd(const PITRebarEndTypes & endType, DPoint3d rebarVec, double angle, bool isArcRebar = false);

		/*
		* @description: 钢筋直锚
		* @param	endType		IN		钢筋端部样式
		* @param	bBegin		IN		是否是起点
		*
		* @return	无
		* @author	LiuXiang
		* @Time		2021/4/21
		*/
		void RebarEndStraightAnchor(const PITRebarEndTypes & endType, bool bBegin, bool isStirrup = false);

	public:
		/*
		* @description: 箍筋的绘制
		* @param	endType		IN		钢筋端部样式
		*
		* @return	无
		* @author	djp
		* @Time		2021/9/2
		*/
		bool makeStirrupURebarWithNormal(vector<CPoint3D>& vecRebarVertex,double bendRadius,double bendLen,RebarEndTypes const&  endTypes,CVector3D   endNormal);
		/*
		* @description: 实现钢筋端部样式
		* @param	endType		IN		钢筋端部样式
		*
		* @return	无
		* @author	LiuXiang
		* @Time		2021/4/21
		*/
		void EvaluateEndTypes(const PITRebarEndTypes &endType);

		void EvaluateEndTypes(const PITRebarEndTypes &endType,double angel1,double angel2 );

		void EvaluateEndTypesStirrup(PITRebarEndTypes endType);

		void EvaluateEndTypesArc(PITRebarEndTypes endType);

		/*
		* @description: 根据点创建U形钢筋线，点必须大于等于4
		* @param	vecRebarVertex	IN		U形钢筋顶点集合
		* @param	bendRadius		IN		弯曲半径
		*
		* @return	失败返回false,成功返回true
		* @author	LiuXiang
		* @Time		2021/4/21
		*/
		bool makeURebarCurve(vector<CPoint3D> const& vecRebarVertex, double bendRadius);


		/*
		* @description: 根据点创建箍筋线,点必须大于等于5
		* @param	vecRebarVertex	IN		箍筋顶点集合
		* @param	bendRadius		IN		弯曲半径
		* @param	endTypes		IN		端部样式
		*
		* @return	失败返回false,成功返回true
		* @author	LiuXiang
		* @Time		2021/7/26
		*/
		bool makeStirrupRebarCurve(vector<CPoint3D> const& vecRebarVertex, double bendRadius, PITRebarEndTypes endTypes);

		/*
		* @desc:	根据点生成直线、L型、U型钢筋
		* @param[in]	vecRebarVertex 	顶点集合
		* @param[in]	bendRadius 弯曲半径
		* @return	失败返回false,成功返回true
		* @remark	
		* @author	Hong ZhuoHui
		* @Date:	2023/01/06
		*/
		bool makeILURebarCurve(vector<CPoint3D> const& vecRebarVertex, double bendRadius);

		/*
		* @desc:	根据点生成直的钢筋
		* @param[in]	vecRebarVertex 钢筋顶点集合	
		* @return	bool 失败返回false,成功返回true
		* @author	Hong ZhuoHui
		* @Date:	2023/01/04
		*/
		bool makeStraightRebarCurve(vector<CPoint3D> const& vecRebarVertex);

	bool makeRebarCurve(double bendRadius,double bendLen,RebarEndTypes const& endTypes,CPoint3D const&ptstr,CPoint3D const&ptend);

	bool makeRebarCurveWithNormal
	(
		double                  bendRadius,
		double                  bendLen,
		RebarEndTypes const&    endTypes,
		CPoint3D const&        ptstr,
		CPoint3D const&        ptend,
		CVector3D   endNormal,
		CMatrix3D const&        mat
	);

	bool makeURebarWithNormal(vector<CPoint3D>& vecRebarVertex,
		double                  bendRadius,
		double                  bendLen,
		RebarEndTypes const&    endTypes,
		CVector3D   endNormal,
		CMatrix3D const&     mat);

	/*
	* @description: 取钢筋线中最长的线段
	* @param	rebarCurve	IN		钢筋线
	* @param	lineMax		OUT		最长线段
	*
	* @return	失败返回false,成功返回true
	* @author	LiuXiang
	* @Time		2021/7/23
	*/
	static bool GetMaxLenLine(RebarCurveCR rebarCurve, LineSegment &lineMax);
};

}
