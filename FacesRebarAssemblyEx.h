#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	ǽ���
*	Project:		��ά����ͼ��Ŀ
*	Author:			LiuXiang
*	Date:			2021/06/08
	Version:		V1.0
*	Description:	FacesRebarAssemblyEx
*	History:
*	1. Date:		2021/06/08
*	Author:			LiuXiang
*	Modification:	create file
*	Edit�� Liu Silei 2025/2/24
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
	BE_DATA_REFER(BeMatrix, Placement)              //��ǰ�ֲ�����ԭ��
		BE_DATA_VALUE(PIT::Concrete, Concrete)				//������
		BE_DATA_VALUE(vector<PIT::ConcreteRebar>, MainRebars)				//����
		BE_DATA_VALUE(vector<vector<PIT::EndType> >, vecEndTypes)			//�˲���ʽ
		BE_DATA_VALUE(vector<ElementId>, SetIds)					//SetId
		BE_DATA_VALUE(DVec3d, faceNormal)				//ƽ�淨��
		BE_DATA_VALUE(bool, isReverse)
		BE_DATA_VALUE(FaceType, faceType)
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
public:
	FacesRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~FacesRebarAssemblyEx();

public:
	vector<vector<DSegment3d> > m_vecRebarStartEnd;	//����˿׶������е�
	vector<vector<PIT::PITRebarCurve>> m_vecRebarCurvePt;	//�ֽ�Ԥ���ߵ����е�
	vector<ElementRefP> m_allPreViewEehs;//Ԥ����ť���º�����иֽ��ߵ�Ԫ��
	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;//δ��ܿ׶������е�
	void ClearLines();
	bool m_isClearLine = true;

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

	bool m_UseXOYDir;//�Ƿ�Ӧ��XOY������Ϊ��ֽ����ɷ���
	void Setd(double d) { _d = d; }
};

class MultiPlaneRebarAssemblyEx : public FacesRebarAssemblyEx
{
public:
	MultiPlaneRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL);
	~MultiPlaneRebarAssemblyEx();

	struct LinePoint
	{
		DPoint3d ptPoint;
		int		 iIndex;
	}stLinePoint;

	struct LineSeg
	{
		int				  iIndex;
		PIT::LineSegment  LineSeg1; //��׼�߶Σ���ƽ�洹ֱ��XOYƽ�棬��Ϊ�ױߣ���ƽ��ƽ����XOYƽ�棬��Ϊ���
		PIT::LineSegment  LineSeg2;	//��׼�߶Σ�ƽ���ڹ�LineSeg1����㴹ֱ��LineSeg�ķ�����߶�
	}stLineSeg;
public:
	virtual bool AnalyzingFaceGeometricData(EditElementHandleR eeh);

	virtual bool MakeRebars(DgnModelRefP modelRef);

	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, vector<DPoint3d>& rebarPoint, int iIndex, const PIT::PITRebarEndTypes& endTypes, double bendRadius);

	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, vector<DPoint3d>& rebarPoint, vector<int>& vecIndex, const PIT::PITRebarEndTypes& endTypes, double bendRadius);

	void SortLineSegVec(vector<LineSeg>& m_vecLineSeg, double uor_per_mm);

	void SortLineSeg(vector<LineSeg>&	m_vecLineSeg, double uor_per_mm);

	void CmdSort(vector<DPoint3d>& vecPoint);

	void AddFace(EditElementHandleR faceEeh) { m_vecFace.push_back(faceEeh); }

	RebarSetTag* MakeRebars
	(
		ElementId&					rebarSetId,
		vector<PIT::LineSegment>&	vecRebarLine,
		vector<PIT::LineSegment>&	vec,
		int							dir,
		BrStringCR					sizeKey,
		double						spacing,
		double						startOffset,
		double						endOffset,
		int							level,
		int							grade,
		int							DataExchange,
		vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
		vector<CVector3D> const&	vecEndNormal,
		DgnModelRefP				modelRef
	);

	void CalculateUseHoles(DgnModelRefP modelRef);

	bool JudgeMergeType(PIT::LineSegment& LineSeg1, PIT::LineSegment& LineSeg2, double uor_per_mm);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Plane; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"MultiPlane Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"MultiPlane Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(MultiPlaneRebarAssemblyEx, RebarAssembly)

private:
	vector<LineSeg>			m_vecLineSeg;
	vector<ElementHandle>	m_vecFace;
	vector<DVec3d>			m_vecFaceNormal;

	DPoint3d				m_ComVec;
	double					m_offset;

};


class PlaneRebarAssemblyEx : public FacesRebarAssemblyEx
{
	BE_DATA_VALUE(PIT::LineSegment, LineSeg1)	//��׼�߶Σ���ƽ�洹ֱ��XOYƽ�棬��Ϊ�ױߣ���ƽ��ƽ����XOYƽ�棬��Ϊ���
		BE_DATA_VALUE(PIT::LineSegment, LineSeg2)	//��׼�߶Σ�ƽ���ڹ�LineSeg1����㴹ֱ��LineSeg�ķ�����߶�

	//	bool				_bAnchor;
		unsigned short		_anchorPos = 0;			//ֱêλ��
	ElementHandle		_ehCrossPlanePre;
	ElementHandle		_ehCrossPlaneNext;
	unsigned short		_bendPos = 0;			//90������λ��

public:
	PlaneRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL);
	~PlaneRebarAssemblyEx();

private:
	//���ݸֽ����Ϣ����ebarCurve
	//vector<PIT::PITRebarCurve>& rebar �õ���RebarCurve
	//PIT::PITRebarEndTypes& endTypes �ֽ����Ϣ�Ͷ˲���Ϣ
	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebar, const PIT::PITRebarEndTypes& endTypes);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Plane; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Plane Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Plane Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:
	RebarSetTag* MakeRebars(ElementId& rebarSetId, PIT::LineSegment rebarLine, PIT::LineSegment vec, int dir, BrStringCR sizeKey, double spacing, double startOffset, double endOffset, int	level, int grade, int DataExchange, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, DgnModelRefP modelRef);

	//����׶�
	void CalculateUseHoles(DgnModelRefP modelRef);

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(PlaneRebarAssemblyEx, RebarAssembly)

public:
	//���������Ϣ����Ҫ������m_lineseg1��m_lineseg2�����������ֽ���ȶ�
	virtual bool	AnalyzingFaceGeometricData(EditElementHandleR eeh);
	//�����ֽ����Ҫ���
	virtual bool	MakeRebars(DgnModelRefP modelRef);
	//	void SetAnchor(bool anchor) { _bAnchor = anchor; }
	void SetCrossPlanePre(ElementHandleCR eh) { _ehCrossPlanePre = eh; }
	void SetCrossPlaneNext(ElementHandleCR eh) { _ehCrossPlaneNext = eh; }


};


class CamberedSurfaceRebarAssemblyEx : public FacesRebarAssemblyEx
{
public:
	// 	enum ArcType
	// 	{
	// 		CurvedSurface,	//����
	// 		Torus			//Բ����
	// 	};
	BE_DATA_VALUE(PIT::ArcSegment, ArcSeg)			//������
		BE_DATA_VALUE(double, height)					//����߶�
		//	BE_DATA_VALUE(ArcType,	arcType)			//��������
		double		_dOuterArcRadius;
private:

	bool CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);

	bool makeLineRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::ArcSegment arcSeg, double dLen, double space, double startOffset, double endOffset, PIT::PITRebarEndTypes& endTypes);

	bool makeArcWallRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::ArcSegment arcSeg, double space, double startOffset, double endOffset, PIT::PITRebarEndTypes& endTypes);

	RebarSetTag* MakeRebars_Line
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		PIT::ArcSegment		arcSeg,
		double              dLen,
		double              spacing,
		double              startOffset,
		double              endOffset,
		int					level,
		int					grade,
		int					DataExchange,
		vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
		vector<CVector3D> const& vecEndNormal,
		DgnModelRefP        modelRef
	);

	RebarSetTag* MakeRebars_Arc
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		PIT::ArcSegment		arcSeg,
		double              spacing,
		double              startOffset,
		double              endOffset,
		int					level,
		int					grade,
		int					DataExchange,
		vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
		vector<CVector3D> const& vecEndNormal,
		DgnModelRefP        modelRef
	);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::CamberedSurface; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

	void CalculateUseHoles(DgnModelRefP modelRef);

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CamberedSurfaceRebarAssemblyEx, RebarAssembly)

public:
	CamberedSurfaceRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL)
		:FacesRebarAssemblyEx(id, modelRef), _dOuterArcRadius(0.0)
	{
	};

	~CamberedSurfaceRebarAssemblyEx()
	{
	};

	virtual bool	AnalyzingFaceGeometricData(EditElementHandleR eeh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);
};