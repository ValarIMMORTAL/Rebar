#pragma once
class FacesRebarAssembly;
#include "FacesRebarAssembly.h"

class CamberedSurfaceRebarAssembly : public FacesRebarAssembly
{
public:
	// 	enum ArcType
	// 	{
	// 		CurvedSurface,	//曲面
	// 		Torus			//圆环面
	// 	};
	BE_DATA_VALUE(PIT::ArcSegment, ArcSeg)			//弧数据
		BE_DATA_VALUE(double, height)					//弧面高度
		//	BE_DATA_VALUE(ArcType,	arcType)			//弧面类型
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
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
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
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
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
	BE_DECLARE_VMS(CamberedSurfaceRebarAssembly, RebarAssembly)

public:
	CamberedSurfaceRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL)
		:FacesRebarAssembly(id, modelRef), _dOuterArcRadius(0.0)
	{
	};

	~CamberedSurfaceRebarAssembly()
	{
	};

	virtual bool	AnalyzingFaceGeometricData(EditElementHandleR eeh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);
};

