#pragma once
class FacesRebarAssembly;
#include "FacesRebarAssembly.h"

class MultiPlaneRebarAssembly : public FacesRebarAssembly
{
public:
	MultiPlaneRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	~MultiPlaneRebarAssembly();

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
	void AnalyzeRebarDir();
	void AnalyzeAssociateWallAndSlab(EditElementHandleCR eeh, int i, MSElementDescrP EdpFace);
	void AnalyzeHorizeEndType(vector<PIT::LineSegment> vecMergeSeg, int rebarnum);

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
	BE_DECLARE_VMS(MultiPlaneRebarAssembly, RebarAssembly)

private:
	vector<LineSeg>			m_vecLineSeg;
	vector<ElementHandle>	m_vecFace;
	vector<DVec3d>			m_vecFaceNormal;

	DPoint3d				m_ComVec;
	double					m_offset;

	struct
	{
		PIT::EndType m_topEndinfo;
		double m_dUpSlabThickness;
		bool m_bUpIsStatr;
		PIT::EndType m_bottomEndinfo;
		double m_dbottomSlabThickness;
		int m_wallTopFaceType; //0:内面，1：外面
		int m_wallBottomFaceType; //0:内面，1：外面
		double m_dTopOffset;
		double m_dBottomOffset;
		int m_iCurRebarDir;
		std::vector<ElementHandle> m_associatedWall;
		PIT::EndType m_HorEndType;
		PIT::EndType m_HorStrType;
		int m_bStartType;//0:内面，1：外面
		int m_bEndInType;//0:内面，1：外面
		double m_dStartOffset;
		double m_dEndOffset;
		vector<MSElementDescrP> m_vecWallEdp;
		void clear()
		{
			m_topEndinfo = { 0 };
			m_dUpSlabThickness = 0;
			m_bUpIsStatr = false;
			m_bottomEndinfo = { 0 };
			m_dbottomSlabThickness = 0;
			m_wallTopFaceType = -1; //0:内面，1：外面
			m_wallBottomFaceType = -1; //0:内面，1：外面
			m_dTopOffset = 0;
			m_dBottomOffset = 0;
			m_iCurRebarDir = -1;
			m_associatedWall.clear();
			m_HorEndType = { 0 };
			m_HorStrType = { 0 };
			m_bStartType = -1;//0:内面，1：外面
			m_bEndInType = -1;//0:内面，1：外面
			m_dStartOffset = 0;
			m_dEndOffset = 0;
			m_vecWallEdp.clear();
		}

	}m_wallfaceAssociateInfo;
	int m_iMergeType = -1; //0:墙-墙，1:板-板,2:墙板
};


