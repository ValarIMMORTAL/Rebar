#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	ǽ���
*	Project:		��ά����ͼ��Ŀ
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
	BE_DATA_REFER(BeMatrix, Placement)              //��ǰ�ֲ�����ԭ��
	BE_DATA_VALUE(PIT::Concrete, Concrete)				//������
	BE_DATA_VALUE(vector<PIT::ConcreteRebar>,	MainRebars)				//����
	BE_DATA_VALUE(vector<vector<PIT::EndType> >, vecEndTypes)			//�˲���ʽ
	BE_DATA_VALUE(vector<ElementId>,		SetIds)					//SetId
	BE_DATA_VALUE(vector<ElementId>, vecSetId)								//rebarelementSet��ÿ��Id
	BE_DATA_VALUE(DVec3d,					faceNormal)				//ƽ�淨��
	BE_DATA_VALUE(bool,						isReverse)
	BE_DATA_VALUE(FaceType,					faceType)
    BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)     //ǰ���ߵ����е�

public:
	std::vector<EditElementHandle* > m_Holeehs;					//�׶�
	std::vector<EditElementHandle* > m_Negs;					//��ʵ��
	std::vector<EditElementHandle* > m_useHoleehs;				//ɸѡ��Ŀ׶��͸�ʵ��
	std::vector<DPoint3d>			 m_vecRebarPtsLayer;		//���׶���ȡǰ�����иֽ������
	std::vector<DPoint3d>			 m_vecTwinRebarPtsLayer;	//���׶���ȡǰ�����в��������

	ElementHandle					m_face;
	EditElementHandle				*m_Solid;
	std::vector<ElementHandle>		m_vecElm;
	bool							m_slabUpFace;

	bool m_bisSump;

	bool m_isCatchpit = false;//�Ƿ��Ǽ�ˮ�����

	int m_CatchpitType = -1;//0�Ǳ�׼��ˮ�ӣ�1�����⼯ˮ�ӣ�2��˫��ˮ��

	vector<ElementHandle> m_AllFloors;//������ˮ���ϵİ�

	//double m_UpFloor_Height = 0.0;//��ˮ���Ϸ���ĸ߶�

	vector <EditElementHandle*> m_ScanedFloors;//���ʵ��

	map<ElementHandle*, double> m_mapFloorAndHeight;//��ˮ������İ��Ӧ�ĸ߶�

	RebarSetTagArray m_rsetTags;

	int m_solidType = -1; //0���壬1Ϊǽ
	bvector<ISubEntityPtr> m_anchorFaces;//Z�ΰ�ѡ���ê�����
	vector<EditElementHandle*> m_VeticalPlanes;//��Ҫê�����
	EditElementHandle* m_CurentFace = nullptr;//��ǰ�������

	int m_facePlace = -1;//0���ڲ��棬1�������

public:
	FacesRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~FacesRebarAssembly();

public:
	vector<vector<DSegment3d> > m_vecRebarStartEnd;	//����˿׶������е�
	vector<ElementRefP> m_allLines;//Ԥ����ť���º�����иֽ���
	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;//δ��ܿ׶������е�
	void ClearLines();

	// @brief �����������ֱ����ĸֽ��Ƿ���Ҫ�任ê�̷����ê�̳��ȣ�;
	// @param range_eeh��		��ˮ��ʵ���range
	// @param range_CurFace��	��ǰ������range
	// @return �����Ҫ����true
	// @Add by tanjie, 2024.1.9
	bool AnalyseOutsideFace(DRange3d &range_eeh,DRange3d &range_CurFace);

protected:
	void			Init();
	void			movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);
	bool GetIntersectPointsWithOldElmOwner(vector<DPoint3d>& interPoints, EditElementHandleP oldElm, DPoint3d& ptstr, DPoint3d& ptend, double dSideCover);
	int IsHaveVerticalWall(DPoint3d ptstr, DPoint3d ptend, MSElementDescrP tmpfaces[20], int facenum, bool isGetFaceType = false);
	//�Ƿ���ƽ��ǽ
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

	bool m_UseXOYDir;//�Ƿ�Ӧ��XOY������Ϊ��ֽ����ɷ���
	void Setd(double d) { _d = d; }
};
