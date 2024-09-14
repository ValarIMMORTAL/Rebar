#pragma once
#include "CommonFile.h"
#include <RebarElements.h>
#include "RebarDetailElement.h"
#include "PITRebarCurve.h"

class CTieRebarToolAssembly : public RebarAssembly
{
	BE_DATA_VALUE(double,					Cover)			// ������
	BE_DATA_VALUE(bool,						isContinRebar)  // �Ƿ��������
	BE_DATA_VALUE(vector<ElementId>,		vecSetId)		// SetId
	BE_DATA_VALUE(int,						modelType);		// 0 : ǽ 1: ��
	BE_DATA_VALUE(double,				posSpacing1);   // ������1
	BE_DATA_VALUE(double,				posSpacing2);   // ������2
	BE_DATA_VALUE(double,				revSpacing1);   // ������1
	BE_DATA_VALUE(double, 				revSpacing2);   // ������2

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
