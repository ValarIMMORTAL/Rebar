#pragma once
#pragma once

/*--------------------------------------------------------------------------------------+
|
|     $Source: CInsertRebarAssemblyWall.h $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/

#include "RebarDetailElement.h"
#include <RebarElements.h>
#include "CommonFile.h"

class CInsertRebarDlgNew;
class CInsertRebarAssemblySTWallNew : public RebarAssembly
{
	BE_DATA_REFER(BeMatrix, Placement)
	BE_DATA_REFER(ACCConcrete, accConcrete)
	BE_DATA_VALUE(vector<ElementId>, vecSetId)			// SetId
	BE_DATA_VALUE(std::vector<InsertRebarInfo::WallInfo>, vecWallData)
	BE_DATA_VALUE(bool, isStaggered)			// 交错类型

	BE_DATA_VALUE(double, PositiveCover)		//正面保护层
	BE_DATA_VALUE(double, ReverseCover)		//反面保护层
	BE_DATA_VALUE(double, SideCover)			//侧面保护层
	BE_DATA_VALUE(InsertRebarInfo::WallInfo, stWallInfo)
	BE_DATA_VALUE(std::vector<InsertRebarPoint>, vecRebarPts)
	BE_DATA_VALUE(double,			NormalSpace)
	BE_DATA_VALUE(double,			AverageSpace)

public:
	CInsertRebarAssemblySTWallNew(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~CInsertRebarAssemblySTWallNew();

	void filterHoleehs(DgnModelRefP modelRef);

	void SetConcreteData(const PIT::Concrete& concreteData);

	virtual bool AnalyzingWallGeometricData(ElementHandleCR eh);

	static void SortVecRebar(vector<InsertRebarPoint>& vecPoint, CVector3D& sortVec);

	static double CalcSlabThickness(ElementHandleCR eh);
	static bool CalaWallNormalVec(ElementHandleCR eh, CVector3DR vecNormal);

	CInsertRebarDlgNew * pInsertDoubleRebarDlg;
	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

private:
	CInsertRebarDlgNew*			m_pInsertRebarDlgNew;

private:
	struct STWallGeometryInfo
	{
		DPoint3d ptStart;
		DPoint3d ptEnd;
		double length;
		double width;
		double height;
	}m_STwallData;

	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_useHoleehs; // 筛选后的孔洞

public:

	void Init();

	bool MakeRebars(DgnModelRefP modelRef);

	RebarSetTag* MakeRebars
	(
		vector<InsertRebarPoint>&		vecRebarPts,
		InsertRebarInfo::WallInfo&		wallData,
		ElementId&						rebarSetId,
		DgnModelRefP					modelRef
	);

	bool makeRebarCurve
	(
		RebarCurve&				rebar,
		double                  bendRadius,
		double                  bendLen,
		double					dRotateAngle,
		RebarEndTypes const&    endTypes,
		DPoint3d&			ptstr,
		DPoint3d&			ptend
	);

	static bool IsSmartSmartFeature(EditElementHandle& eeh);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 15; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Insert RebarNew"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Insert RebarNew"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CInsertRebarAssemblySTWallNew, RebarAssembly)

};
