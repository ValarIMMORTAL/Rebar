#pragma once
class FacesRebarAssemblyEx;
#include "FacesRebarAssemblyEx.h"

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
		PIT::LineSegment  LineSeg1; //基准线段，若平面垂直与XOY平面，则为底边，若平面平行与XOY平面，则为左边
		PIT::LineSegment  LineSeg2;	//基准线段，平面内过LineSeg1的起点垂直与LineSeg的方向的线段
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
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
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
