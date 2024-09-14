#pragma once
#include "Public.h"
#include <RebarElements.h>
#include "RebarDetailElement.h"
#include "PITRebarCurve.h"
#include "RebarMaker.h"

class CustomRebarDlag;
//class CustomRebarAssembly : public RebarAssembly
//{
//public:
//	CustomizeRebarInfo m_CustomRebarInfo;
//	double m_Diameter;
//	bool MakeRebars(DgnModelRefP modelRef);
//protected:
//	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 17; }
//	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Customize Rebar"; }
//	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Customize Rebar"; }
//	virtual bool        OnDoubleClick() override;
//	virtual bool        Rebuild() override;
//	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
//	BE_DECLARE_VMS(CustomRebarAssembly, RebarAssembly)
//
//};

struct pointInfo
{
	DPoint3d ptStr;
	DPoint3d ptCenter;
	DPoint3d ptEnd;
	CVector3D vec;
	ICurvePrimitive::CurvePrimitiveType curType;
	DPoint3d ptMid; //弧行的中心点
};
class CustomRebar : public RebarElement
{
public:
	CustomizeRebarInfo m_CustomRebarInfo;
	vector<pointInfo> m_vecRebarPts;//一根线的信息
	int m_linestyle;//1:全为弧线   0：全为直线  -1：有弧线和直线

	CustomRebar(const vector<pointInfo> &vecRebarPts) {}
	~CustomRebar() {}
	void Create(RebarSetR rebarSet);
//	static RebarElement* CreateURebar(RebarSetR rebarSet, const vector<CPoint3D>& vecRebarPts);

	bool CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);
	bool CalculateArc(RebarVertices&  vers, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);
	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);
private:
	PIT::PITRebarCurve  m_Curve;
};





class CustomRebarAssembly : public RebarMakerFactory
{
public:
	ElementId m_contid;

	CustomRebarAssembly(ElementHandleR eeh) {}
	~CustomRebarAssembly() {}
	virtual RebarSetTag* MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);

	void Push_CustomRebar(shared_ptr<CustomRebar> pCustomRebar)
	{
		m_vecCustomRebar.push_back((shared_ptr<CustomRebar>)pCustomRebar);
	}
//	void ParsingElementPro(ElementHandleCR eeh);
private:
	vector<shared_ptr<CustomRebar> >		m_vecCustomRebar;

};