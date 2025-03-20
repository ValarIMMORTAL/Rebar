#pragma once
class WallRebarAssembly;
#include "WallRebarAssembly.h"


class ArcWallRebarAssembly : public WallRebarAssembly
{
public:
	CWallRebarDlg *pArcWallDoubleRebarDlg;
private:

	bool CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);

	//	bool makeArcWallRebarCurve(RebarCurve& rebar, double xPos, double height, double startOffset, double endOffset, double bendRadius, double bendLen, RebarEndTypes const& endTypes, CVector3D const& endNormal, CMatrix3D const& mat);

	bool makeLineRebarCurve
	(
		vector<PIT::PITRebarCurve>& 	rebar,
		PIT::ArcSegment				arcSeg,
		double					dLen,
		double                  space,
		double					startOffset,
		double					endOffset,
		PIT::PITRebarEndTypes&		endTypes
	);

	void CalculateUseHoles(DgnModelRefP modelRef);

	bool makeArcWallRebarCurve
	(
		vector<PIT::PITRebarCurve>& 	rebar,
		PIT::ArcSegment				arcSeg,
		double                  space,
		double					startOffset,
		double					endOffset,
		PIT::PITRebarEndTypes&		endTypes
	);

	RebarSetTag* MakeRebars_Line
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		PIT::ArcSegment			arcSeg,
		double              dLen,
		double              spacing,
		double              startOffset,
		double              endOffset,
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
		vector<CVector3D> const& vecEndNormal,
		TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
		int level,
		int grade,
		int DataExchange,
		bool				bTwinbarLevel,
		DgnModelRefP        modelRef
	);

	RebarSetTag* MakeRebars_Arc
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		PIT::ArcSegment			arcSeg,
		double              spacing,
		double              startOffset,
		double              endOffset,
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
		vector<CVector3D> const& vecEndNormal,
		TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
		int level,
		int grade,
		int DataExchange,
		bool				bTwinbarLevel,
		DgnModelRefP        modelRef
	);

	//获取新的弧线和钢筋间距
	void GetNewArcAndSpacing(PIT::ArcSegment oldArc, PIT::ArcSegment& newArc, double angle, double& newSpacing);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::ArcWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);



public:
	virtual bool	AnalyzingWallGeometricDataARC(ElementHandleCR eh, PIT::ArcSegment &arcFront, PIT::ArcSegment &arcBack);
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(ArcWallRebarAssembly, RebarAssembly)

public:
	ArcWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL)
		:WallRebarAssembly(id, modelRef)
	{
		pArcWallDoubleRebarDlg = nullptr;
		memset(&m_ArcWallData, 0, sizeof(ArcWallGeometryInfo));
	};

	~ArcWallRebarAssembly()
	{

	};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);

private:
	struct ArcWallGeometryInfo
	{
		PIT::ArcSegment	OuterArc;		//
		PIT::ArcSegment	InnerArc;		//
		double height;				//墙高
		double thickness;			//墙厚
	}m_ArcWallData;

	bool		m_isPushTieRebar; // 是否push进入拉筋的钢筋点中
	PIT::ArcSegment m_outMaxArc;	//外弧最大弧形
	double m_sideCoverAngle = 0;	//保护层角度
};
