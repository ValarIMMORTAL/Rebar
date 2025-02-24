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

#include "../RebarMaker.h"
#include "PITACCRebarAssemblyNew.h"
#include "AssociatedComponentNew.h"
#include "../PITArcSegment.h"

#define BE_ACCCLASS_MAKERNEW(var)		ACCRebarMethod##var##_MakerNew

using namespace PIT;

class ACCRebarMakerNew : public RebarMakerFactory
{
	BE_DATA_VALUE(vector<PIT::ConcreteRebar>, vecRebarData)

public:
	ACCRebarMakerNew(ElementHandle currEh, const IntersectEle& ACCEh, ACCRebarAssemblyNew *rebarAssembly);

	virtual ~ACCRebarMakerNew()
	{
		for (int j = 0; j < m_Holeehs.size(); j++)
		{

			if (m_Holeehs.at(j) != nullptr)
			{
				delete m_Holeehs.at(j);
				m_Holeehs.at(j) = nullptr;
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
	static	bool CreateACCRebar(ACCRebarAssemblyNew*  rebar, ElementHandleCR currEh, std::vector<PIT::ConcreteRebar> vecRebarData, DgnModelRefP modelRef);

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

protected:
	ElementHandle			m_eh;					//��ǰ������Ԫ��
//	std::shared_ptr<CACC>	m_pACC;					//��������������ָ��

	IntersectEle			m_ACCEh;				//�˲���������
	ACCRebarAssemblyNew*		m_RebarAssembly;		//��������rebarAssemblyָ��

	STWallGeometryInfo		m_CurSTwallData;		//��ǰSTWALL��������
	STWallGeometryInfo		m_InSecSTwallData;		//�˲�����STWALL��������

	vector<DPoint3d>		m_vecInSecPartPt;		//��ǰ����������������������潻��

	double					m_slabHeight;			//����
public:
	bool m_isFrCoverside;//���������뵱ǰ���������ཻ���Ƿ��Ӧ�Ĺ�������ǰ��������
};

//ê����ʽ1
class ACCRebarMethod1_MakerNew : public ACCRebarMakerNew
{
public:
	ACCRebarMethod1_MakerNew(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssemblyNew *rebarAssembly) :ACCRebarMakerNew(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod1_MakerNew() {}

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
class ACCRebarMethod2_MakerNew : public ACCRebarMakerNew
{
public:
	ACCRebarMethod2_MakerNew(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssemblyNew *rebarAssembly) :ACCRebarMakerNew(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod2_MakerNew() {}

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
class ACCRebarMethod3_MakerNew : public ACCRebarMakerNew
{
public:
	ACCRebarMethod3_MakerNew(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssemblyNew *rebarAssembly) :ACCRebarMakerNew(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod3_MakerNew() {}

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
	RebarSetP		MakeURebarCurve(ElementId rebarSetId, const vector<vector<DSegment3d> >& vvecStartEnd, double width, DgnModelRefP modelRef);
	/*
	* @description: ����ê����ʽ3�������ݽ�
	* @param	vecRebarCurve	IN		�ݽ�ֽ�������
	* @param	modelRef		IN		��������������model,�ֽ�����ڸ�model��
	*
	* @return	��
	* @author	LiuXiang
	* @Time		2021/04/28
	*/
	bool			MakeIntersectionRebarCurve(std::vector<PITRebarCurve>& vecRebarCurve, DgnModelRefP modelRef);

private:
	DPoint3d		m_RebarExtendvec;	//�ֽ����췽��
	double			m_L0;
	UInt32			m_Relation;			//����������ϵ
};

//ê����ʽ4
class ACCRebarMethod4_MakerNew : public ACCRebarMakerNew
{
public:
	ACCRebarMethod4_MakerNew(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssemblyNew *rebarAssembly) :ACCRebarMakerNew(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod4_MakerNew() {}

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
class ACCRebarMethod7_MakerNew : public ACCRebarMakerNew
{
public:
	ACCRebarMethod7_MakerNew(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssemblyNew *rebarAssembly) :ACCRebarMakerNew(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod7_MakerNew() {}

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
class ACCRebarMethod9_MakerNew : public ACCRebarMakerNew
{
public:
	ACCRebarMethod9_MakerNew(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssemblyNew *rebarAssembly) :ACCRebarMakerNew(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod9_MakerNew() {}

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
class ACCRebarMethod10_MakerNew : public ACCRebarMakerNew
{
public:
	ACCRebarMethod10_MakerNew(ElementHandleCR currEh, const IntersectEle& ACCEh, ACCRebarAssemblyNew *rebarAssembly) :ACCRebarMakerNew(currEh, ACCEh, rebarAssembly)
	{}

	virtual ~ACCRebarMethod10_MakerNew() {}

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
class LineStringRebarMakerNew : public RebarMakerFactory
{
	    BE_DATA_VALUE(ElementId, callerId)
		BE_DATA_VALUE(LineSegVct, segs)
		BE_DATA_VALUE(BrString, rebarSize)
		BE_DATA_VALUE(double, spacing)
		BE_DATA_VALUE(double, height)
		BE_DATA_VALUE(double, cover)

public:
	LineStringRebarMakerNew(ElementId callerId, const LineSegVct &lines, BrStringCR rebarSize, double spacing, double height, double cover = 0)
		:m_callerId(callerId), m_segs(lines), m_rebarSize(rebarSize), m_spacing(spacing), m_height(height), m_cover(cover)
	{}
	virtual ~LineStringRebarMakerNew() {}

	LineStringRebarMakerNew() = default;
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