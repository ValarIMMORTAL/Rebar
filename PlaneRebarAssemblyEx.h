#pragma once
class FacesRebarAssemblyEx;
#include "FacesRebarAssemblyEx.h"

class PlaneRebarAssemblyEx : public FacesRebarAssemblyEx
{
private:
	BE_DATA_VALUE(PIT::LineSegment, LineSeg1) //基准线段，若平面垂直与XOY平面，则为底边，若平面平行与XOY平面，则为左边
	BE_DATA_VALUE(PIT::LineSegment, LineSeg2) //基准线段，平面内过LineSeg1的起点垂直与LineSeg的方向的线段

	//	bool				_bAnchor;
	unsigned short _anchorPos = 0; //直锚位置
	ElementHandle _ehCrossPlanePre;
	ElementHandle _ehCrossPlaneNext;
	unsigned short _bendPos = 0; //90度弯曲位置

public:
	PlaneRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL);
	~PlaneRebarAssemblyEx();

private:
	//根据钢筋点信息构建ebarCurve
	//vector<PIT::PITRebarCurve>& rebar 得到的RebarCurve
	//PIT::PITRebarEndTypes& endTypes 钢筋点信息和端部信息
	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebar, const PIT::PITRebarEndTypes& endTypes,
	                    double endTypeStartOffset, double endTypeEndOffset);

protected:
	virtual int GetPolymorphic() const override
	{
		return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Plane;
	}

	virtual WString GetDescriptionEx(ElementHandleCR eh) const override { return L"Plane Rebar"; }
	virtual WString GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Plane Rebar"; }
	virtual bool OnDoubleClick() override;
	virtual bool Rebuild() override;

protected:
	RebarSetTag* MakeRebars(ElementId& rebarSetId, PIT::LineSegment rebarLine, PIT::LineSegment vec, int dir,
	                        BrStringCR sizeKey, double spacing, double startOffset, double endOffset, int level,
	                        int grade, int DataExchange, vector<PIT::EndType> const& vecEndType,
	                        vector<CVector3D> const& vecEndNormal, DgnModelRefP modelRef);

	//计算孔洞
	void CalculateUseHoles(DgnModelRefP modelRef);

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(PlaneRebarAssemblyEx, RebarAssembly)

public:
	//分析面的信息，主要分析出m_lineseg1和m_lineseg2，横向和纵向钢筋的先段
	virtual bool AnalyzingFaceGeometricData(EditElementHandleR eeh);
	//创建钢筋的主要入口
	virtual bool MakeRebars(DgnModelRefP modelRef);
	//	void SetAnchor(bool anchor) { _bAnchor = anchor; }
	void SetCrossPlanePre(ElementHandleCR eh) { _ehCrossPlanePre = eh; }
	void SetCrossPlaneNext(ElementHandleCR eh) { _ehCrossPlaneNext = eh; }
};
