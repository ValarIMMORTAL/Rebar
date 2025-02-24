#pragma once
#include "WallRebarAssemblyNew.h"

class CCutRebarAssembly : public WallRebarAssemblyNew
{
public:
	CCutRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		WallRebarAssemblyNew(id, modelRef)
#ifdef PDMSIMPORT
		, pWall(NULL)
#endif
	{
	};
	virtual ~CCutRebarAssembly() {}

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::ELLWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ELLWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ELLWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CCutRebarAssembly, RebarAssembly)

public:
	virtual bool MakeRebars(DgnModelRefP modelRef);

	double CalaRebarLength(RebarElementP pRebar, DgnModelRefP modelRef);

	void MakeCutRebarCureOneLen(vector<PIT::PITRebarCurve>& rebarCurvesNum, RebarElementP pRebar, double startLen, double endLen);

	void MakeCutRebarCure(vector<CNCutRebarInfo>& vecCutInfo, vector<PIT::PITRebarCurve>& rebarCurvesNum, DPoint3d ptStr, DPoint3d ptEnd, double diameter, bool bFlag = false);

	void CalaRebarStartEnd(RebarElementP rep, DPoint3d& PtStar, DPoint3d& PtEnd, double& diameter, DgnModelRefP modelRef);

	void CalcRebarSet();

	// È¡rebarCurµÄGetIP()µã
	bool GetArcIpPoint(DPoint3dR ptIP, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);

	virtual bool SetWallData(ElementHandleCR eh)
	{
		SetSelectedElement(eh.GetElementId());
		SetSelectedModel(eh.GetModelRef());
		return true;
	}

	RebarSetTag* MakeCutRebar(RebarSetP rebarSetOld, std::vector<ElementRefP>& vecRebarRef, ElementId rebarSetId, DgnModelRefP modelRef);

	map<ElementId, std::vector<ElementRefP>> map_RebarSet;

private:
	struct CWriteRebarInfo
	{
		double diameter;
		BrString sizeKey;
		BrString rebarlevel; // ¸Ö½îÍ¼²ã

		string Level;
		string rebarType;
		string rebarGrade;
	};
};
