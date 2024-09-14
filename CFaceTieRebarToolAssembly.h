#pragma once
#include "CommonFile.h"
#include <RebarElements.h>
#include "RebarDetailElement.h"
#include "PITRebarCurve.h"


class CFaceTieRebarToolAssembly : public RebarAssembly
{
	BE_DATA_VALUE(double, Cover)			// ������
	BE_DATA_VALUE(bool, isContinRebar)  // �Ƿ��������
	BE_DATA_VALUE(vector<ElementId>, vecSetId)		// SetId
	BE_DATA_VALUE(int, modelType);		// 0 : ǽ 1: ��
	BE_DATA_VALUE(double, posSpacing1);   // ������1
	BE_DATA_VALUE(double, posSpacing2);   // ������2
	BE_DATA_VALUE(double, revSpacing1);   // ������1
	BE_DATA_VALUE(double, revSpacing2);   // ������2

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

	BrString						m_MainFirLevSizekey;   // �������һ��ĳߴ�
	BrString						m_MainSecLevSizekey;   // ������ڶ��εĳߴ�
	BrString						m_AnthorFirLevSizekey;   // ��һ�����һ��ĳߴ�
	BrString						m_AnthorSecLevSizekey;   // ��һ����ڶ���ĳߴ�

	CVector3D						m_MainLevVec;	// ѡ��һ��Ϊ������
	CVector3D						m_AnthorLevVec; // ��һ����ֱ����

	vector<DSegment3d>				m_vecMainFirLevRebar; // ѡ��һ��Ϊ������ĵ�һ��ֽ�
	vector<DSegment3d>				m_vecMainSecLevRebar; // ��һ��Ϊ������ĵڶ���ֽ�

	vector<DSegment3d>				m_vecAnthorFirLevRebar; // ѡ��һ��Ϊ��һ����ĵ�һ��ֽ�
	vector<DSegment3d>				m_vecAnthorSecLevRebar; // ��һ��Ϊ��һ����ĵڶ�1��ֽ�

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