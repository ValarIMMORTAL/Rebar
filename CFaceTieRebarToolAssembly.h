#pragma once
#include "CommonFile.h"
#include <RebarElements.h>
#include "RebarDetailElement.h"
#include "PITRebarCurve.h"


class CFaceTieRebarToolAssembly : public RebarAssembly
{
	BE_DATA_VALUE(double, Cover)			// 保护层
	BE_DATA_VALUE(bool, isContinRebar)  // 是否连续配筋
	BE_DATA_VALUE(vector<ElementId>, vecSetId)		// SetId
	BE_DATA_VALUE(int, modelType);		// 0 : 墙 1: 板
	BE_DATA_VALUE(double, posSpacing1);   // 正面间距1
	BE_DATA_VALUE(double, posSpacing2);   // 正面间距2
	BE_DATA_VALUE(double, revSpacing1);   // 反面间距1
	BE_DATA_VALUE(double, revSpacing2);   // 反面间距2

public:
	CFaceTieRebarToolAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~CFaceTieRebarToolAssembly();

	void InitReabrData();
	void Settrans(Transform& trans)
	{
		m_trans = trans;
	}
	bool AnalyzingWallGeometricData(ElementHandleCR eh);
	void GetEleNameAndType(ElementHandleR eeh);
	void SetTieRebarInfo(FaceTieReBarInfo& tieRebarInfo)
	{
		m_tieRebarInfo = tieRebarInfo;
	}
	void CalculateSelectRebarInfo(vector<ElementRefP>& vecSelectrebars, DgnModelRefP modelRef);
	void SortAllVecRebar();
	void TraveAllRebar(RebarAssembly* pRebarAssembly, DgnModelRefP modelRef);
	bool MakeRebars(DgnModelRefP modelRef);
	void SortVecRebar(vector<DSegment3d>& vecSeg, const CVector3D& perpendicularSortVec, const CVector3D& parallelSortVec);
	bool push_vecPoint(vector<DSegment3d>& vecLevRebar, DPoint3d& ptstr, DPoint3d& ptend, CVector3D& vec, CVector3D& vecAnthor, DgnModelRefP modelRef);
	void CalaRebarStartEnd(RebarElementP rep, DPoint3d& PtStar, DPoint3d& PtEnd, double& diameter, DgnModelRefP modelRef);
	void GetStartEndPoint(vector<vector<DSegment3d> >& vecStartEnd);
	void SortVecRebar(vector<DSegment3d>& vecSeg, const CVector3D& sortVec);
	void SetFaceId(ElementId faceId)
	{
		m_uFaceId = faceId;
	}
	void SetEndAngle(double Angle1, double Angle2);
// 	virtual bool OnDoubleClick()
// 	{
// 		MessageBox(NULL, L"111",L"22",MB_OK);
// 		return false;
// 	}
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

	FaceTieReBarInfo					m_tieRebarInfo;

	Transform						m_trans;

	DPoint3d						m_ptStart;
	DPoint3d						m_ptEnd;
	ElementId						m_uFaceId;
	double							m_angle1;
	double							m_angle2;
protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 11; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Beam Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Beam Rebar"; }
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CFaceTieRebarToolAssembly, RebarAssembly)
};