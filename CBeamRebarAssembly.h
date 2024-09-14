#pragma once
#include "RebarDetailElement.h"
#include <RebarElements.h>
#include "CommonFile.h"
#include "PITRebarCurve.h"

using namespace PIT;

struct RebarSDKPierVaseData
{
	enum TieType
	{
		kBend,
		kCog,
		kHook
	};
	//PierVase
	BE_DATA_VALUE(double, Height)
		BE_DATA_VALUE(double, Depth)
		BE_DATA_VALUE(double, CapHeight)
		BE_DATA_VALUE(double, CapWidth)
		BE_DATA_VALUE(double, FootHeight)
		BE_DATA_VALUE(double, FootWidth)
		BE_DATA_VALUE(double, Cover)
		BE_DATA_VALUE(double, ShearSpacing)
		BE_DATA_REFER(BrString, ShearBarSize)
		BE_DATA_VALUE(double, LongitudinalSpacing)
		BE_DATA_REFER(BrString, LongitudinalBarSize)
		BE_DATA_VALUE(TieType, TieType)
public:
	RebarSDKPierVaseData()
		: m_Height(0.0)
		, m_Depth(0.0)
		, m_CapHeight(0.0)
		, m_CapWidth(0.0)
		, m_FootHeight(0.0)
		, m_FootWidth(0.0)
		, m_Cover(0.0)
		, m_ShearSpacing(0.0)
		, m_LongitudinalSpacing(0.0)
		, m_TieType(kBend)
	{
	}
};

class CBeamRebarMainDlg;
class CBeamRebarAssembly : public RebarAssembly, private RebarSDKPierVaseData
{
	BE_DATA_REFER(BeMatrix,											Placement)
	BE_DATA_VALUE(vector<ElementId>,								vecSetId)			// SetId
	BE_DATA_VALUE(BeamRebarInfo::BeamDefaultData,					stDefaultInfo)
	BE_DATA_VALUE(BeamRebarInfo::BeamBaseData,						stBeamBaseData)
	BE_DATA_VALUE(vector<BeamRebarInfo::BeamAreaVertical>,			vecBeamAreaVertical)
	BE_DATA_VALUE(vector<BeamRebarInfo::BeamRebarVertical>,			vecBeamRebarVertical)
	BE_DATA_VALUE(STWallGeometryInfo,								BeamInfo)			// Beam的几何数据

	BE_DATA_VALUE(vector<BeamRebarInfo::BeamCommHoop>,				vecBeamCommHoop)
	BE_DATA_VALUE(vector<BeamRebarInfo::BeamRebarHoop>,				vecBeamRebarHoop)

public:
	CBeamRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~CBeamRebarAssembly();

	struct BeamDirection
	{
		DPoint3d ptStart;
		DPoint3d ptEnd;

	}m_stBeamDirection;

	void CBeamRebarAssembly::CalculateTransform(CVector3D& transform, BrStringCR sizeKey, BeamRebarInfo::BeamAreaVertical stBeamAreaVertical, DgnModelRefP modelRef);

	bool CalcBeamBaseInfo(ElementHandleCR eh);

	bool MakeRebars(DgnModelRefP modelRef);

	RebarSetTag* MakeRebars
	(
		BeamRebarInfo::BeamAreaVertical& stBeamAreaVertical,
		BeamRebarInfo::BeamRebarVertical& stBeamRebarVertical,
		vector<PIT::EndType> vecEndtype,
		CMatrix3D const&    mat,
		ElementId& rebarSetId,
		BrStringCR sizeKey,
		DgnModelRefP modelRef
	);

	RebarSetTag* MakeRebars
	(
		BeamRebarInfo::BeamCommHoop& stBeamCommHoop,
		BeamRebarInfo::BeamRebarHoop& stBeamRebarHoop,
		vector<PIT::EndType> vecEndtype,
		CMatrix3D const&    mat,
		ElementId& rebarSetId,
		BrStringCR sizeKey,
		DgnModelRefP modelRef
	);

	bool makeRebarCurve
	(
		PITRebarCurve&			  rebar,
		PITRebarEndTypes&		  endTypes,
		CPoint3D const&			  ptstr,
		CPoint3D const&			  ptend
	);

	bool makeRebarCurve
	(
		PITRebarCurve&				rebar,
		PITRebarEndTypes&			endTypes,
		vector<PIT::EndType>&			vecEndtype,
		vector<CPoint3D>&			vecPoint,
		double						bendRadius
	);

	static bool IsSmartSmartFeature(EditElementHandle& eeh);

	RebarSetTag* MakeStirrups(DgnModelRefP modelRef);

	int MakeRebarsTmp(DgnModelRefP modelRef);

	bool GetRectFromHeight(double& width, double& depth, double height, double cover);

	bool GetHeightFromWidth(double& height, double width, double diameter) const;

	int MakeStirrup(RebarCurve &rebar, double z, double width, double depth, double cover, double diameter, double bendRadius, double bendLen, const RebarEndTypes &endTypes) const;

	int MakeVerticalXDirBar(RebarCurve &rebar, double xPos, double height, double diameter, double bendRadius, double bendLen, RebarEndTypes const& endTypes, bool isFront) const;
	
	int MakeVerticalYDirBar(RebarCurve &rebar, double yPos, double barDiameter, double bendRadius, double bendLen, RebarEndTypes const& endTypes, bool isRight) const;

	bool GetVerticalYDirPoints(bvector<BePoint3D>& points, double& vaseRadius, double yPos, double barDiameter, double bendRadius) const;

	RebarSetTag* MakeVerticalYDirBars(ElementId& setId, bool isRight, DgnModelRefP modelRef);

	RebarSetTag* MakeVerticalXDirBars(ElementId& setId, bool isFront, DgnModelRefP modelRef);

private:
	struct HoopPointInfo
	{
		int						nPosition;
		CPoint3D				ptPoint;
		double					diameter; // 直径
	};

	vector<HoopPointInfo>		 m_vecPointHoop; // 箍筋中的点

	CBeamRebarMainDlg*			 m_pBeamRebarMainDlg;

private:
	bool		m_bSetup;
	// rebar
	ElementId   m_stirrup_set_id;
	ElementId   m_front_set_id;
	ElementId   m_back_set_id;
	ElementId   m_left_set_id;
	ElementId   m_right_set_id;

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 7; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Beam Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Beam Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CBeamRebarAssembly, RebarAssembly)
};
