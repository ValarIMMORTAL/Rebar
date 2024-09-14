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
	BE_DATA_VALUE(double,										Cover)						// ������
	BE_DATA_VALUE(double,										domeHight)					// 񷶥�߶�
	BE_DATA_VALUE(double,										domeRadius)					// 񷶥�뾶
	BE_DATA_VALUE(vector<ElementId>,							vecSetId)					// SetId
	BE_DATA_VALUE(vector<PIT::DomeLevelInfo>,					vecDomeLevelInfo)			// 񷶥��Χ��Ϣ

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
		double     dRadius1;		// ��ʼ���뾶
		double	   dRadius2;		// �������뾶
		double	   dAngleOrSpace;	// �Ƕ� / ���
		double     dStartOffset;    // ��ʼƫ��(����)
	}m_stArcCutInfo;

	CDomeRebarMainDlg*			m_pDomeRebarMainDlg;
	vector<arcDefinePoint>		m_vecArcPoint; // ��һ�����ɵĻ��θֽ���Ϣ

	bool						m_roundFlag;

	vector<MSElementDescrP>		m_vecArcLine;	// first : �⻡   second : �ڻ�
	DPoint3d					m_arcCenter;    // �������ĵ�

	double						m_innerArcRadius; // �ڻ��뾶
	double						m_outerArcRadius; // �⻡�뾶

	DPoint3d					m_circleCenter;  // 񷶥�����ĵ�(����)
	vector<st_VertexVec>		m_vecVertex;     // ����ĸ�����
	double						m_dZLevelStart;	 // 񷶥��Z����Բ�θֽ���ʼ�����
	double						m_dZLevelEnd;	 // 񷶥��Z����Բ�θֽ���������

	map<int, vector<PIT::DomeLevelDetailInfo>>	m_mapDomeLevelDetailInfo;		// 񷶥ÿ��ֽ������Ϣmap
};
