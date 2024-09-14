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

class  CInsertRebarMainDlg;
class CInsertRebarAssemblySTWall : public RebarAssembly
{
	BE_DATA_REFER(BeMatrix,									Placement)
	BE_DATA_REFER(Transform,								Trans)
	BE_DATA_REFER(ACCConcrete,								accConcrete)
	BE_DATA_VALUE(vector<BrString>,							vecDirSize)			// �ߴ�
	BE_DATA_VALUE(std::vector<RebarPoint>,					rebarPts)			// �ֽ����е�����
	BE_DATA_VALUE(vector<ElementId>,						vecSetId)			// SetId
	BE_DATA_VALUE(std::vector<InsertRebarInfo::WallInfo>,	vecWallData)
	BE_DATA_VALUE(int,										RebarLevelNum)		// �ֽ����
	BE_DATA_VALUE(vector<vector<PIT::EndType> >,					vvecEndType)		// �˲���ʽ
	BE_DATA_VALUE(int,										staggered)			// ��������

	BE_DATA_VALUE(double,									PositiveCover)		//���汣����
	BE_DATA_VALUE(double,									ReverseCover)		//���汣����
	BE_DATA_VALUE(double,									SideCover)			//���汣����

public:
	CInsertRebarAssemblySTWall(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~CInsertRebarAssemblySTWall();

	void TransFromRebarPts(vector<RebarPoint>&  rebarPts);

	void SetConcreteData(const PIT::Concrete& concreteData);

	void SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas);

	void SetRebarEndTypes(vector<PIT::EndType> const & vecEndTypes);

	virtual bool AnalyzingWallGeometricData(ElementHandleCR eh);

	void CalculateUseHoles(DgnModelRefP modelRef);

	static double CalcSlabThickness(ElementHandleCR eh);

	void SetLayerRebars();


private:
	EditElementHandle*			m_pOldElm;
	CInsertRebarMainDlg*		m_pInsertRebarMainDlg;
	bool						m_bFirstLong; // �������: ÿ���һ���ֽ��Ƿ��ǳ���

	std::map<int, std::vector<RebarPoint>> 	m_mpLayerRebars;

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
	std::vector<EditElementHandle*> m_useHoleehs; // ɸѡ��Ŀ׶�

public:
	void Init();

	bool MakeRebars(DgnModelRefP modelRef);

	RebarSetTag* MakeRebars(
		vector<RebarPoint>&				rebarPts,
		vector<PIT::EndType>					vecEndtype,
		InsertRebarInfo::WallInfo		wallData,
		ElementId&						rebarSetId,
		BrStringCR						sizeKey,
		DgnModelRefP					modelRef,
		int&							rebarLevel
	);

	bool makeRebarCurve
	(
		RebarCurve&				rebar,
		double                  bendRadius,
		double                  bendLen,
		double					dRotateAngle,
		RebarEndTypes const&    endTypes,
		CPoint3D const&			ptstr,
		CPoint3D const&			ptend
	);

	static bool IsSmartSmartFeature(EditElementHandle& eeh);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 4; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CInsertRebarAssemblySTWall, RebarAssembly)

};
