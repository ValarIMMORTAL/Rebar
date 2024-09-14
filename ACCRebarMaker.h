#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	ACCRebarMaker
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/04/12
	Version:		V1.0
*	Description:	ACCRebarMaker
*	History:
*	1. Date:		2021/04/12
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/

#include "RebarMaker.h"
#include "PITACCRebarAssembly.h"
#include "AssociatedComponent.h"
#include "PITArcSegment.h"

#define BE_ACCCLASS_MAKER(var)		ACCRebarMethod##var##_Maker

// template<typename T = WallRebarAssembly>
// class ACCRebar : RebarMakerFactory
// {
// public:
// 	ACCRebar(T* rebarassembly,IntersectEle accEh):assembly(rebarassembly),m_accEh(accEh){}
// 	~ACCRebar() {}
// 
// 	virtual RebarSetTag* MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL) { return NULL; }
// 
// 	void MakeRebars(DgnModelRefP modelRef);
// 	
// private:
// 	T *assembly;
// 	IntersectEle m_accEh;
// };
// 
// template<typename T>
// inline void ACCRebar<T>::MakeRebars(DgnModelRefP modelRef)
// {
// 	//dosomthing
// 	if (assembly != NULL)
// 	{
// //		assembly->SetACComponenet(accEh);
// 		assembly->MakeACCRebars();
// 	}
// }

using namespace PIT;

class ACCRebarMaker : public RebarMakerFactory
{
	BE_DATA_VALUE(vector<PIT::ConcreteRebar>, vecRebarData)

public:
	ACCRebarMaker(ElementHandle currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly);

	virtual ~ACCRebarMaker() 
	{
		for (int j = 0; j < m_Holeehs.size(); j++)
		{

			if (m_Holeehs.at(j) != nullptr)
			{
				if (m_Holeehs.at(j)->IsValid())
				{
					delete m_Holeehs.at(j);
					m_Holeehs.at(j) = nullptr;
				}
				
			}
		}
	}
	bool makeRebarCurve
	(
		DPoint3d ptstr,
		DPoint3d ptend,
		vector<PITRebarCurve>&     rebars,
		PITRebarEndTypes&		endTypes,
		double dSideCover
	);
	void PlusHoles(double dSideCover);
	/*
	* @description: 关联构件配筋函数，创建关联构件钢筋
	* @param	rebar		IN		指向具体的待配筋墙的rebarassembly
	* @param	currEh		IN		当前待配筋构件
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	static	bool CreateACCRebar(ACCRebarAssembly*  rebar, ElementHandleCR currEh, std::vector<PIT::ConcreteRebar> vecRebarData, DgnModelRefP modelRef);

protected:
	/*
	* @description: 关联构件配筋附加钢筋配筋函数，根据派生类来确定调用哪个函数
	* @param	rebarSetId	IN		附加钢筋的RebarSetId
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	virtual RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);

	void AddRebarLevelAndType(RebarElementP rebarElement, BrString sizeKey, DPoint3d ptRebar, DPoint3d ptStr, DPoint3d ptEnd, double dDistance, bool bFlag = false);


	/*
	* @description: 分析STWALL的几何数据，用于后续配筋
	* @param	eh			IN		STWALL直墙元素
	* @param	stwallInfo	OUT		STWALL直墙几何数据结构体
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	virtual bool			AnalyzingSTWallGeometricData(const ElementHandle& eh, ElementHandle& cureeh, STWallGeometryInfo &stwallInfo);

	/*
	* @description: 分析板的数据，用于后续配筋
	* @param	eh			IN		板元素
	* @param	slabHeight	OUT		板厚度
	* @return	无
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	virtual bool			AnalyzingSlabGeometricData(ElementHandleCR eh, ElementHandle& cureeh, double& slabHeight);

	/*
	* @description: 计算STWALL交集处部分的顶点数组，该顶点数组为交集部分的底面交点
	* @param	vecInSecPartPt	OUT		交集部分的底面交点数组
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	virtual bool			CaluateSTWallIntersectPart(vector<DPoint3d> &vecInSecPartPt);

public:
	vector<vector<DSegment3d>> m_RebarLinePts; //存储关联处钢筋的点，用于预览按钮画线
	/*
	* @description: 关联构件配筋主要钢筋配筋函数，根据派生类来确定调用哪个函数
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	virtual bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: 获取当前待配筋构件元素
	* @param	无
	*
	* @return	当前待配筋构件元素
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	ElementHandle			GetCurElementHandle() { return m_eh; }

	bool GetCoverSideinFrontOrBack();
	//在计算关联构件和当前模型的相交区域前，扣除两个构件的前后保护层，从新计算中心点和宽度
	bool ReCalculateWidthAndPoint();

	bool ReCalculateWidthAndPointForL4();

	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_UseHoleehs;
	int                     m_associatedRelation;    //关联关系，默认为0忽略，1锚入，2被锚入
protected:
	
	ElementHandle			m_eh;					//当前待配筋构件元素
//	std::shared_ptr<CACC>	m_pACC;					//关联构件分析类指针

	IntersectEle			m_ACCEh;				//端部关联构件
	ACCRebarAssembly*		m_RebarAssembly;		//待配筋构件的rebarAssembly指针

	STWallGeometryInfo		m_CurSTwallData;		//当前STWALL几何数据
	STWallGeometryInfo		m_InSecSTwallData;		//端部关联STWALL几何数据

	vector<DPoint3d>		m_vecInSecPartPt;		//当前构件与关联构件交集处底面交点

	double					m_slabHeight;			//板厚度
public: 
	bool m_isFrCoverside;//关联构件与当前配筋体最外侧交点是否对应的关联构件前保护层厚度
};

//锚固样式1
class ACCRebarMethod1_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod1_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod1_Maker() {}

public:
	/*
	* @description: 锚固样式1主要钢筋配筋
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: 配置锚固样式1交集处纵筋
	* @param	rebarSetId	IN		附加钢筋的RebarSetId
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};

//锚固样式2
class ACCRebarMethod2_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod2_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod2_Maker() {}

public:
	/*
	* @description: 锚固样式2主要钢筋配筋
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: 配置锚固样式2交集处纵筋
	* @param	rebarSetId	IN		附加钢筋的RebarSetId
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};


//锚固样式3
class ACCRebarMethod3_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod3_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod3_Maker() {}

public:
	/*
	* @description: 锚固样式3主要钢筋配筋
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: 配置锚固样式3交集处U形钢筋与纵筋
	* @param	rebarSetId	IN		附加钢筋的RebarSetId
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);

private:
	/*
	* @description: 配置锚固样式3交集处U形钢筋
	* @param	rebarSetId		IN		附加钢筋的RebarSetId
	* @param	vvecStartEnd	IN		所有主筋的起点与终点
	* @param	modelRef		IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	RebarSetP		MakeURebarCurve(ElementId rebarSetId, const vector<vector<DSegment3d> >& vvecStartEnd , double width, DgnModelRefP modelRef);
	/*
	* @description: 配置锚固样式3交集处纵筋
	* @param	vecRebarCurve	IN		纵筋钢筋线数组
	* @param	modelRef		IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	bool			MakeIntersectionRebarCurve(std::vector<PITRebarCurve>& vecRebarCurve,DgnModelRefP modelRef);

private:
	DPoint3d		m_RebarExtendvec;	//钢筋延伸方向
	double			m_L0;
	UInt32			m_Relation;			//构件关联关系
};

//锚固样式4
class ACCRebarMethod4_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod4_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod4_Maker() {}

public:
	/*
	* @description: 锚固样式4主要钢筋配筋
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: 配置锚固样式4交集处纵筋
	* @param	rebarSetId	IN		附加钢筋的RebarSetId
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};


//锚固样式7
class ACCRebarMethod7_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod7_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod7_Maker() {}

public:
	/*
	* @description: 锚固样式7主要钢筋配筋
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: 锚固样式7无需额外配筋，直接返回NULL
	* @param	rebarSetId	IN		附加钢筋的RebarSetId
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};

//锚固样式9
class ACCRebarMethod9_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod9_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod9_Maker() {}

public:
	/*
	* @description: 锚固样式9主要钢筋配筋
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: 锚固样式9无需额外配筋，直接返回NULL
	* @param	rebarSetId	IN		附加钢筋的RebarSetId
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};

//锚固样式10
class ACCRebarMethod10_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod10_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod10_Maker() {}

public:
	/*
	* @description: 锚固样式10主要钢筋配筋
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: 锚固样式10无需额外配筋，直接返回NULL
	* @param	rebarSetId	IN		附加钢筋的RebarSetId
	* @param	modelRef	IN		混凝土假体所在model,钢筋将生成在该model下
	*
	* @return	无
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};


/*
围绕线串绘制纵向钢筋
*/
class LineStringRebarMaker : public RebarMakerFactory
{
	BE_DATA_VALUE(ElementId, callerId)
	BE_DATA_VALUE(LineSegVct, segs)
	BE_DATA_VALUE(BrString, rebarSize)
	BE_DATA_VALUE(double, spacing)
	BE_DATA_VALUE(double, height)
	BE_DATA_VALUE(double, cover)
	
public:
	LineStringRebarMaker(ElementId callerId,const LineSegVct &lines,BrStringCR rebarSize,double spacing, double height,double cover = 0)
		:m_callerId(callerId),m_segs(lines),m_rebarSize(rebarSize),m_spacing(spacing),m_height(height),m_cover(cover)
	{}
	virtual ~LineStringRebarMaker() {}
	
	LineStringRebarMaker() = default;
	std::vector<EditElementHandle*> m_useHoleehs;//筛选后的孔洞
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);

	RebarSetP MakeLineStringRebar(ElementId rebarSetId, ElementId callerId, const vector<LineSegment>& segs, double offset, DgnModelRefP modelRef);
	bool makeRebarCurve
	(
		DPoint3d ptstr,
		DPoint3d ptend,
		vector<shared_ptr<PITRebarCurve>>&     rebars,
		PITRebarEndTypes&		endTypes,
		double dSideCover
	);
};