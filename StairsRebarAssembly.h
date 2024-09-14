#pragma once
#include "CommonFile.h"
#include <RebarElements.h>
#include "RebarDetailElement.h"
#include "PITRebarCurve.h"

class CStarisRebarDlog;
class CStairsRebarAssembly : public RebarAssembly
{
	BE_DATA_REFER(BeMatrix, Placement)
	BE_DATA_VALUE(vector<ElementId>, vecSetId)			// SetId
	BE_DATA_VALUE(double, Cover) // ������

	struct AttachRebarInfo;
public:
	CStairsRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~CStairsRebarAssembly();

	void SetUcsMatrix(DPoint3d ptStart, DPoint3d ptEnd);

	void GetStairsFeatureParam(ElementHandleCR eh);

	void CalculateTransform(CVector3D& transform, BrStringCR sizeKey, DgnModelRefP modelRef);

	static bool IsSmartSmartFeature(EditElementHandle& eeh);

	bool MakeRebars(DgnModelRefP modelRef);

	bool makeRebarCurve
	(
		PIT::PITRebarCurve&			rebar,
		PIT::PITRebarEndTypes&		endTypes,
		CPoint3D const&         ptstr,
		CPoint3D const&         ptend
	);

	bool makeRebarCurve
	(
		PIT::PITRebarCurve&			rebar,
		PIT::PITRebarEndTypes&		endTypes,
		vector<PIT::EndType>&			vecEndtype,
		vector<CPoint3D>&			vecPoint,
		double						bendRadius,
		bool						bFlag = true
	);

	RebarSetTag* MakeRebars
	(
		vector<PIT::EndType> vecEndtype,
		CMatrix3D const&    mat,
		ElementId& rebarSetId,
		BrStringCR sizeKey,
		DgnModelRefP modelRef
	);

	RebarSetTag* MakeStepRebars
	(
		vector<PIT::EndType> vecEndtype,
		ElementId& rebarSetId,
		BrStringCR sizeKey,
		DgnModelRefP modelRef
	);

	RebarSetTag* MakeAttachRebars
	(
		vector<AttachRebarInfo>& vvecRebarInfo,
		vector<PIT::EndType>& vecEndtype,
		ElementId& rebarSetId,
		BrStringCR sizeKey,
		DgnModelRefP modelRef
	);

	RebarSetTag* MakeTieRebars
	(
		vector<PIT::EndType> vecEndtype,
		vector<vector<CPoint3D>> vvecTieRebarPts,
		ElementId& rebarSetId,
		BrStringCR sizeKey,
		DgnModelRefP modelRef
	);

	void GetRebarInfo(StairRebarInfo const & StairsRebarInfo)
	{
		m_StairsRebarInfo = StairsRebarInfo;
	}

private:
	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

	void PushAttachRebarInfo(DgnModelRefP modelRef);

private:
	// ̨�׵�����Ϣ
	struct StairsDownInfo
	{
		DPoint3d ptStart; // ������߷���
		DPoint3d ptEnd;
		double height; // ̨�׵Ĵ�ֱ�߶�
		double length; // ̨�׵���ĳ���
	}m_StairsDownInfo;

	// ̨�ײ�����Ϣ
	struct StairsSideInfo
	{
		double StepHeight; // ÿ��̨�׵ĸ߶�
		double StepWidth; // ÿ��̨�׵Ŀ��
		double StepLength; // ÿ��̨�׵ĳ���

		double minDis_Down; // ̨����͵ĵ㵽����ľ���
		double maxDis_Down; // ̨����ߵĵ㵽����ľ���

		DPoint3d ptStart;
		DPoint3d ptEnd; // �����ϱߵ��±ߵķ���

		DPoint3d ptFrontStart;	// ¥�����﷽��
		DPoint3d ptBackStart;

	}m_StairsSideInfo;

	// ��β���Ӹֽ� ��Ͷ˲���Ϣ
	struct AttachRebarInfo
	{
		RebarEndType::Type	strEndType;		 // ����Ƿ����乳
		RebarEndType::Type	endEndType;		 // �յ��Ƿ����乳
		CVector3D			vecEndNormal;	 // ����乳����
		CVector3D			vecEndNormalTmp; // �յ��乳����
		vector<DPoint3d>	vecPoint;		 // ����Ϣ
	};

	StairRebarInfo m_StairsRebarInfo;//�ֽ���Ϣ
	double m_stepDiameter;

	DPoint3d m_ptMainRebarUp[2];
	DPoint3d m_ptMainRebarDown[2];

	DSegment3d m_SideMaxLine; // ¥��̤����������µ���
	vector<DSegment3d> m_vecSideXYLine; // ¥��̤������ˮƽ����
	vector<DSegment3d> m_vecSideZLine;  // ¥��̤������Z�����

	vector<DPoint3d> m_vecTailPoint; // ¥��β���ĵ����㣬��������

	vector<vector<CPoint3D>> m_vvecRebarPts;

	// ��β���Ӹֽ� 
	vector<AttachRebarInfo> m_vvecAttach_8mm;  // ���Ӹֽ� 8mm
	vector<AttachRebarInfo> m_vvecAttach_10mm; // ���Ӹֽ� 10mm
	vector<AttachRebarInfo> m_vvecAttach_12mm; // β�����Ӹֽ� 12mm

	CStarisRebarDlog*  m_pStarisRebarDlg;

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 9; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Beam Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Beam Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CStairsRebarAssembly, RebarAssembly)

};
