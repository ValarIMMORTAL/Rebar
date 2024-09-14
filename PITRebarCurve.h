#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	PITRebarCurve
*	Project:		��ά����ͼ��Ŀ
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
		* @description: ���ɸֽ��������ֵĻ���
		* @param	seg			OUT		�ֽ������Ļ��߲���
		* @param	ptOrgin		IN		���ǵ�
		* @param	ptVec		IN		�ֽ��
		* @param	endNormal	IN		��������
		* @param	angle		IN		�����Ƕ�
		* @param	bendRadius	IN		�����뾶
		*
		* @return	��
		* @author	LiuXiang
		* @Time		2021/4/21
		*/
		void GenerateArc(BeArcSegR seg, DPoint3dCR ptOrgin, DPoint3dCR ptVec, CVector3D endNormal, double angle, double bendRadius) const;

		//	void GetEndTypeVertex(std::vector<CPoint3D> &vex, BeArcSegCR seg, CVector3D endNormal, double len) const;

			/*
			* @description: �ֽ��������
			* @param	endType		IN		�ֽ�˲���ʽ
			* @param	rebarVec	IN		�ֽ��
			* @param	angle		IN		�ֽ������Ƕ�
			*
			* @return	��
			* @author	LiuXiang
			* @Time		2021/4/21
			*/
		void RebarEndBendBeg(const PITRebarEndTypes & endType, DPoint3d rebarVec, double angle, bool isStirrup = false);

		void RebarEndBendBeg_Arc(const PITRebarEndTypes & endType, DPoint3d rebarVec, double angle, bool isStirrup = false);

		/*
		* @description: �ֽ��յ�����
		* @param	endType		IN		�ֽ�˲���ʽ
		* @param	rebarVec	IN		�ֽ��
		* @param	angle		IN		�ֽ������Ƕ�
		*
		* @return	��
		* @author	LiuXiang
		* @Time		2021/4/21
		*/
		void RebarEndBendEnd(const PITRebarEndTypes & endType, DPoint3d rebarVec, double angle, bool isArcRebar = false);

		/*
		* @description: �ֽ�ֱê
		* @param	endType		IN		�ֽ�˲���ʽ
		* @param	bBegin		IN		�Ƿ������
		*
		* @return	��
		* @author	LiuXiang
		* @Time		2021/4/21
		*/
		void RebarEndStraightAnchor(const PITRebarEndTypes & endType, bool bBegin, bool isStirrup = false);

	public:
		/*
		* @description: ����Ļ���
		* @param	endType		IN		�ֽ�˲���ʽ
		*
		* @return	��
		* @author	djp
		* @Time		2021/9/2
		*/
		bool makeStirrupURebarWithNormal(vector<CPoint3D>& vecRebarVertex,double bendRadius,double bendLen,RebarEndTypes const&  endTypes,CVector3D   endNormal);
		/*
		* @description: ʵ�ָֽ�˲���ʽ
		* @param	endType		IN		�ֽ�˲���ʽ
		*
		* @return	��
		* @author	LiuXiang
		* @Time		2021/4/21
		*/
		void EvaluateEndTypes(const PITRebarEndTypes &endType);

		void EvaluateEndTypes(const PITRebarEndTypes &endType,double angel1,double angel2 );

		void EvaluateEndTypesStirrup(PITRebarEndTypes endType);

		void EvaluateEndTypesArc(PITRebarEndTypes endType);

		/*
		* @description: ���ݵ㴴��U�θֽ��ߣ��������ڵ���4
		* @param	vecRebarVertex	IN		U�θֽ�㼯��
		* @param	bendRadius		IN		�����뾶
		*
		* @return	ʧ�ܷ���false,�ɹ�����true
		* @author	LiuXiang
		* @Time		2021/4/21
		*/
		bool makeURebarCurve(vector<CPoint3D> const& vecRebarVertex, double bendRadius);


		/*
		* @description: ���ݵ㴴��������,�������ڵ���5
		* @param	vecRebarVertex	IN		����㼯��
		* @param	bendRadius		IN		�����뾶
		* @param	endTypes		IN		�˲���ʽ
		*
		* @return	ʧ�ܷ���false,�ɹ�����true
		* @author	LiuXiang
		* @Time		2021/7/26
		*/
		bool makeStirrupRebarCurve(vector<CPoint3D> const& vecRebarVertex, double bendRadius, PITRebarEndTypes endTypes);

		/*
		* @desc:	���ݵ�����ֱ�ߡ�L�͡�U�͸ֽ�
		* @param[in]	vecRebarVertex 	���㼯��
		* @param[in]	bendRadius �����뾶
		* @return	ʧ�ܷ���false,�ɹ�����true
		* @remark	
		* @author	Hong ZhuoHui
		* @Date:	2023/01/06
		*/
		bool makeILURebarCurve(vector<CPoint3D> const& vecRebarVertex, double bendRadius);

		/*
		* @desc:	���ݵ�����ֱ�ĸֽ�
		* @param[in]	vecRebarVertex �ֽ�㼯��	
		* @return	bool ʧ�ܷ���false,�ɹ�����true
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
	* @description: ȡ�ֽ���������߶�
	* @param	rebarCurve	IN		�ֽ���
	* @param	lineMax		OUT		��߶�
	*
	* @return	ʧ�ܷ���false,�ɹ�����true
	* @author	LiuXiang
	* @Time		2021/7/23
	*/
	static bool GetMaxLenLine(RebarCurveCR rebarCurve, LineSegment &lineMax);
};

}
