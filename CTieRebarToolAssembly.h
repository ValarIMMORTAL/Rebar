#pragma once
#include "CommonFile.h"
#include <RebarElements.h>
#include "RebarDetailElement.h"
#include "PITRebarCurve.h"

class CTieRebarToolAssembly : public RebarAssembly
{
	BE_DATA_VALUE(double,					Cover)			// 保护层
	BE_DATA_VALUE(bool,						isContinRebar)  // 是否连续配筋
	BE_DATA_VALUE(vector<ElementId>,		vecSetId)		// SetId
	BE_DATA_VALUE(int,						modelType);		// 0 : 墙 1: 板
	BE_DATA_VALUE(double,				posSpacing1);   // 正面间距1
	BE_DATA_VALUE(double,				posSpacing2);   // 正面间距2
	BE_DATA_VALUE(double,				revSpacing1);   // 反面间距1
	BE_DATA_VALUE(double, 				revSpacing2);   // 反面间距2

public:
	CTieRebarToolAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~CTieRebarToolAssembly();

	void GetHoleWithElement(ElementHandleCR eh);

	bool MakeRebars(DgnModelRefP modelRef);

	void InitReabrData();

	void GetEleNameAndType(ElementHandleR eeh);

	bool push_vecPoint(vector<DSegment3d>& vecLevRebar, DPoint3d& ptstr, DPoint3d& ptend, CVector3D& vec, CVector3D& vecAnthor, DgnModelRefP modelRef);

	void SortVecRebar(vector<DSegment3d>& vecSeg, const CVector3D& sortVec);

	void CalculateSelectRebarInfo(vector<ElementRefP>& vecSelectrebars, DgnModelRefP modelRef);

	void SortVecRebar(vector<DSegment3d>& vecSeg, const CVector3D& perpendicularSortVec, const CVector3D& parallelSortVec);

	static bool IsSmartSmartFeature(EditElementHandle& eeh);

	void SortVecRebar(vector<DSegment3d>& vecFirVec, vector<DSegment3d>& vecSecVec);

	void SortAllVecRebar();

	void TraveAllRebar(RebarAssembly* pRebarAssembly, DgnModelRefP modelRef);

	bool AnalyzingWallGeometricData(ElementHandleCR eh);

	void CalaRebarStartEnd(RebarElementP rep, DPoint3d& PtStar, DPoint3d& PtEnd, double& diameter, DgnModelRefP modelRef);

	void GetStartEndPoint(vector<vector<DSegment3d> >& vecStartEnd);

	void SetTieRebarInfo(TieReBarInfo& tieRebarInfo)
	{
		m_tieRebarInfo = tieRebarInfo;
	}

	void Settrans(Transform& trans)
	{
		m_trans = trans;
	}

private:
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;

	BrString						m_MainFirLevSizekey;   // 主方向第一层的尺寸
	BrString						m_MainSecLevSizekey;   // 主方向第二次的尺寸
	BrString						m_AnthorFirLevSizekey;   // 另一方向第一层的尺寸
	BrString						m_AnthorSecLevSizekey;   // 另一方向第二层的尺寸

	CVector3D						m_MainLevVec;	// 选择一个为主方向
	CVector3D						m_AnthorLevVec; // 另一个垂直方向

	vector<DSegment3d>				m_vecMainFirLevRebar; // 选择一个为主方向的第一层钢筋
	vector<DSegment3d>				m_vecMainSecLevRebar; // 另一个为主方向的第二层钢筋

	vector<DSegment3d>				m_vecAnthorFirLevRebar; // 选择一个为另一方向的第一层钢筋
	vector<DSegment3d>				m_vecAnthorSecLevRebar; // 另一个为另一方向的第二1层钢筋

	TieReBarInfo					m_tieRebarInfo;

	Transform						m_trans;

	DPoint3d						m_ptStart;
	DPoint3d						m_ptEnd;

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 11; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Beam Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Beam Rebar"; }
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CTieRebarToolAssembly, RebarAssembly)

};
