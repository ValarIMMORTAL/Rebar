#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	ACCRebarMaker
*	Project:		��ά����ͼ��Ŀ
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
	* @description: ���������������������������ֽ�
	* @param	rebar		IN		ָ�����Ĵ����ǽ��rebarassembly
	* @param	currEh		IN		��ǰ������
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	static	bool CreateACCRebar(ACCRebarAssembly*  rebar, ElementHandleCR currEh, std::vector<PIT::ConcreteRebar> vecRebarData, DgnModelRefP modelRef);

protected:
	/*
	* @description: �����������Ӹֽ�������������������ȷ�������ĸ�����
	* @param	rebarSetId	IN		���Ӹֽ��RebarSetId
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	virtual RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);

	void AddRebarLevelAndType(RebarElementP rebarElement, BrString sizeKey, DPoint3d ptRebar, DPoint3d ptStr, DPoint3d ptEnd, double dDistance, bool bFlag = false);


	/*
	* @description: ����STWALL�ļ������ݣ����ں������
	* @param	eh			IN		STWALLֱǽԪ��
	* @param	stwallInfo	OUT		STWALLֱǽ�������ݽṹ��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	virtual bool			AnalyzingSTWallGeometricData(const ElementHandle& eh, ElementHandle& cureeh, STWallGeometryInfo &stwallInfo);

	/*
	* @description: ����������ݣ����ں������
	* @param	eh			IN		��Ԫ��
	* @param	slabHeight	OUT		����
	* @return	��
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	virtual bool			AnalyzingSlabGeometricData(ElementHandleCR eh, ElementHandle& cureeh, double& slabHeight);

	/*
	* @description: ����STWALL���������ֵĶ������飬�ö�������Ϊ�������ֵĵ��潻��
	* @param	vecInSecPartPt	OUT		�������ֵĵ��潻������
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	virtual bool			CaluateSTWallIntersectPart(vector<DPoint3d> &vecInSecPartPt);

public:
	vector<vector<DSegment3d>> m_RebarLinePts; //�洢�������ֽ�ĵ㣬����Ԥ����ť����
	/*
	* @description: �������������Ҫ�ֽ�������������������ȷ�������ĸ�����
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	virtual bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: ��ȡ��ǰ������Ԫ��
	* @param	��
	*
	* @return	��ǰ������Ԫ��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	ElementHandle			GetCurElementHandle() { return m_eh; }

	bool GetCoverSideinFrontOrBack();
	//�ڼ�����������͵�ǰģ�͵��ཻ����ǰ���۳�����������ǰ�󱣻��㣬���¼������ĵ�Ϳ��
	bool ReCalculateWidthAndPoint();

	bool ReCalculateWidthAndPointForL4();

	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_UseHoleehs;
	int                     m_associatedRelation;    //������ϵ��Ĭ��Ϊ0���ԣ�1ê�룬2��ê��
protected:
	
	ElementHandle			m_eh;					//��ǰ������Ԫ��
//	std::shared_ptr<CACC>	m_pACC;					//��������������ָ��

	IntersectEle			m_ACCEh;				//�˲���������
	ACCRebarAssembly*		m_RebarAssembly;		//��������rebarAssemblyָ��

	STWallGeometryInfo		m_CurSTwallData;		//��ǰSTWALL��������
	STWallGeometryInfo		m_InSecSTwallData;		//�˲�����STWALL��������

	vector<DPoint3d>		m_vecInSecPartPt;		//��ǰ����������������������潻��

	double					m_slabHeight;			//����
public: 
	bool m_isFrCoverside;//���������뵱ǰ���������ཻ���Ƿ��Ӧ�Ĺ�������ǰ��������
};

//ê����ʽ1
class ACCRebarMethod1_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod1_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod1_Maker() {}

public:
	/*
	* @description: ê����ʽ1��Ҫ�ֽ����
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: ����ê����ʽ1�������ݽ�
	* @param	rebarSetId	IN		���Ӹֽ��RebarSetId
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};

//ê����ʽ2
class ACCRebarMethod2_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod2_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod2_Maker() {}

public:
	/*
	* @description: ê����ʽ2��Ҫ�ֽ����
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: ����ê����ʽ2�������ݽ�
	* @param	rebarSetId	IN		���Ӹֽ��RebarSetId
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};


//ê����ʽ3
class ACCRebarMethod3_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod3_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod3_Maker() {}

public:
	/*
	* @description: ê����ʽ3��Ҫ�ֽ����
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: ����ê����ʽ3������U�θֽ����ݽ�
	* @param	rebarSetId	IN		���Ӹֽ��RebarSetId
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);

private:
	/*
	* @description: ����ê����ʽ3������U�θֽ�
	* @param	rebarSetId		IN		���Ӹֽ��RebarSetId
	* @param	vvecStartEnd	IN		���������������յ�
	* @param	modelRef		IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	RebarSetP		MakeURebarCurve(ElementId rebarSetId, const vector<vector<DSegment3d> >& vvecStartEnd , double width, DgnModelRefP modelRef);
	/*
	* @description: ����ê����ʽ3�������ݽ�
	* @param	vecRebarCurve	IN		�ݽ�ֽ�������
	* @param	modelRef		IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	bool			MakeIntersectionRebarCurve(std::vector<PITRebarCurve>& vecRebarCurve,DgnModelRefP modelRef);

private:
	DPoint3d		m_RebarExtendvec;	//�ֽ����췽��
	double			m_L0;
	UInt32			m_Relation;			//����������ϵ
};

//ê����ʽ4
class ACCRebarMethod4_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod4_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod4_Maker() {}

public:
	/*
	* @description: ê����ʽ4��Ҫ�ֽ����
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: ����ê����ʽ4�������ݽ�
	* @param	rebarSetId	IN		���Ӹֽ��RebarSetId
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};


//ê����ʽ7
class ACCRebarMethod7_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod7_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod7_Maker() {}

public:
	/*
	* @description: ê����ʽ7��Ҫ�ֽ����
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: ê����ʽ7���������ֱ�ӷ���NULL
	* @param	rebarSetId	IN		���Ӹֽ��RebarSetId
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};

//ê����ʽ9
class ACCRebarMethod9_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod9_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod9_Maker() {}

public:
	/*
	* @description: ê����ʽ9��Ҫ�ֽ����
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: ê����ʽ9���������ֱ�ӷ���NULL
	* @param	rebarSetId	IN		���Ӹֽ��RebarSetId
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};

//ê����ʽ10
class ACCRebarMethod10_Maker : public ACCRebarMaker
{
public:
	ACCRebarMethod10_Maker(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssembly *rebarAssembly) :ACCRebarMaker(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod10_Maker() {}

public:
	/*
	* @description: ê����ʽ10��Ҫ�ֽ����
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	bool			MakeRebars(DgnModelRefP modelRef);

	/*
	* @description: ê����ʽ10���������ֱ�ӷ���NULL
	* @param	rebarSetId	IN		���Ӹֽ��RebarSetId
	* @param	modelRef	IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/05/18
	*/
	RebarSetTag*	MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
};


/*
Χ���ߴ���������ֽ�
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
	std::vector<EditElementHandle*> m_useHoleehs;//ɸѡ��Ŀ׶�
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