#pragma once
#include "CommonFile.h"
#include "PITRebarAssembly.h"
#include "PITRebarCurve.h"
#include "PITArcSegment.h"
#include "PITRebarEndType.h"
#include <RebarElements.h>
//#include "ElementAlgorithm.h"

class CDomeRebarMainDlg;
class CDomeRebarAssembly : public PIT::PITRebarAssembly
{
	BE_DATA_VALUE(double,										Cover)						// 保护层
	BE_DATA_VALUE(double,										domeHight)					// 穹顶高度
	BE_DATA_VALUE(double,										domeRadius)					// 穹顶半径
	BE_DATA_VALUE(vector<ElementId>,							vecSetId)					// SetId
	BE_DATA_VALUE(vector<PIT::DomeLevelInfo>,					vecDomeLevelInfo)			// 穹顶配筋范围信息

private:
	struct arcDefinePoint;

public:
	CDomeRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) : PIT::PITRebarAssembly(id, modelRef)
	{
		m_pDomeRebarMainDlg = NULL;
		m_roundFlag = false;
	}

	virtual ~CDomeRebarAssembly();

	void SetCuteFacePoint(vector<st_VertexVec>& vecVertex)
	{
		m_vecVertex = vecVertex;
	}

	void SetmapDomeLevelDetailInfo(map<int, vector<PIT::DomeLevelDetailInfo>>& mapDomeLevelDetailInfo)
	{
		m_mapDomeLevelDetailInfo = mapDomeLevelDetailInfo;
	}

	bool CalculateRound(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt, int nStep = 0);

	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

	DPoint3d GetIntersectRound(DPoint3d roundCenterPt, DPoint3dR centerPtr, DPoint3dR arcStr, DPoint3dR arcMid, DPoint3dR arcEnd, DgnModelRefP modelRef);

	bool IntersectRound(DPoint3dR centerPtr, DPoint3dR arcStr, DPoint3dR arcMid, DPoint3dR arcEnd);

	void TranseArcSide(DPoint3dR centerPt, DPoint3dR begPt, DPoint3dR midPt, DPoint3dR endPt, double reduceRadius);

	void GetDomeFeatureParam(EditElementHandleR eh, DPoint3dCR maxDomePt);

	void GetDomeCutFace(EditElementHandleR domeCutFace);

	bool makeRoundRebarCurve(vector<PIT::PITRebarCurve>& rebar, vector<arcDefinePoint>& vecArcPoint, PIT::PITRebarEndTypes& endTypes, DgnModelRefP modelRef, double levelSpacing, double diameter);

	bool makeArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP arcMsDescrp, PIT::PITRebarEndTypes& endTypes, double reduceRadius, double diameter, double startOffset);

	bool CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);

	bool MakeRebars(DgnModelRefP modelRef);

	bool MakeRebarHoopOrRadial(map<int, vector<PIT::DomeLevelDetailInfo>>::iterator itr, RebarSetTagArray& rsetTags, RebarSetTag* tag, DgnModelRefP modelRef, int& nTagIndex);

	void SetCircleCenter(DPoint3dR pt)
	{
		m_circleCenter = pt;
	}

	bool MakeRebarXYIntersect(map<int, vector<PIT::DomeLevelDetailInfo>>::iterator itr, RebarSetTagArray& rsetTags, RebarSetTag* tag, DgnModelRefP modelRef, int& nTagIndex);

	bool makeXYRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::PITRebarEndTypes& endTypes, DgnModelRefP modelRef, double levelSpacing, double diameter, bool bFlag = true);

	RebarSetTag* MakeRebars_XY(ElementId & rebarSetId, BrString sizeKey, vector<MSElementDescrP>& vecArcLine, DgnModelRefP modelRef, double levelSpacing, bool bFlag);

	RebarSetTag* MakeRebars_Arc(ElementId & rebarSetId, BrString sizeKey, vector<MSElementDescrP>& vecArcLine, DgnModelRefP modelRef, double levelSpacing);

	RebarSetTag* MakeRebars_Round(ElementId & rebarSetId, BrString sizeKey, vector<MSElementDescrP>& vecArcLine, DgnModelRefP modelRef, double levelSpacing);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 12; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Dome Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Dome Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CDomeRebarAssembly, RebarAssembly)

private:
	struct arcDefinePoint
	{
		DPoint3d ptStr;
		DPoint3d ptMid;
		DPoint3d ptEnd;
		DPoint3d centerPtr;
		double	diameter;
	}stArcPoint;

	struct arcCutInfo
	{
		double     dRadius1;		// 起始配筋半径
		double	   dRadius2;		// 结束配筋半径
		double	   dAngleOrSpace;	// 角度 / 间距
		double     dStartOffset;    // 起始偏移(径向)
	}m_stArcCutInfo;

	CDomeRebarMainDlg*			m_pDomeRebarMainDlg;
	vector<arcDefinePoint>		m_vecArcPoint; // 上一次生成的弧形钢筋信息

	bool						m_roundFlag;

	vector<MSElementDescrP>		m_vecArcLine;	// first : 外弧   second : 内弧
	DPoint3d					m_arcCenter;    // 弧的中心点

	double						m_innerArcRadius; // 内弧半径
	double						m_outerArcRadius; // 外弧半径

	DPoint3d					m_circleCenter;  // 穹顶的中心点(顶面)
	vector<st_VertexVec>		m_vecVertex;     // 截面的各个点
	double						m_dZLevelStart;	 // 穹顶在Z轴上圆形钢筋起始布筋点
	double						m_dZLevelEnd;	 // 穹顶在Z轴上圆形钢筋结束布筋点

	map<int, vector<PIT::DomeLevelDetailInfo>>	m_mapDomeLevelDetailInfo;		// 穹顶每层钢筋具体信息map
};
