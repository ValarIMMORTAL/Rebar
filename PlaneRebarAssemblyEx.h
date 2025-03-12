#pragma once
class FacesRebarAssemblyEx;
#include "FacesRebarAssemblyEx.h"

class PlaneRebarAssemblyEx : public FacesRebarAssemblyEx
{
private:
	BE_DATA_VALUE(PIT::LineSegment, LineSeg1) //��׼�߶Σ���ƽ�洹ֱ��XOYƽ�棬��Ϊ�ױߣ���ƽ��ƽ����XOYƽ�棬��Ϊ���
	BE_DATA_VALUE(PIT::LineSegment, LineSeg2) //��׼�߶Σ�ƽ���ڹ�LineSeg1����㴹ֱ��LineSeg�ķ�����߶�

	//	bool				_bAnchor;
	unsigned short _anchorPos = 0; //ֱêλ��
	ElementHandle _ehCrossPlanePre;
	ElementHandle _ehCrossPlaneNext;
	unsigned short _bendPos = 0; //90������λ��

public:
	PlaneRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL);
	~PlaneRebarAssemblyEx();

private:
	//���ݸֽ����Ϣ����ebarCurve
	//vector<PIT::PITRebarCurve>& rebar �õ���RebarCurve
	//PIT::PITRebarEndTypes& endTypes �ֽ����Ϣ�Ͷ˲���Ϣ
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

	//����׶�
	void CalculateUseHoles(DgnModelRefP modelRef);

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(PlaneRebarAssemblyEx, RebarAssembly)

public:
	//���������Ϣ����Ҫ������m_lineseg1��m_lineseg2�����������ֽ���ȶ�
	virtual bool AnalyzingFaceGeometricData(EditElementHandleR eeh);
	//�����ֽ����Ҫ���
	virtual bool MakeRebars(DgnModelRefP modelRef);
	//	void SetAnchor(bool anchor) { _bAnchor = anchor; }
	void SetCrossPlanePre(ElementHandleCR eh) { _ehCrossPlanePre = eh; }
	void SetCrossPlaneNext(ElementHandleCR eh) { _ehCrossPlaneNext = eh; }
};
