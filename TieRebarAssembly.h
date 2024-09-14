#pragma once
#include <RebarElements.h>

enum TieRebarStyle
{
	XX,			//X*X
	XX2,		//X*2X
	X2X,		//2X*X
	X2X2,		//2X*2X
	Custom,		//�Զ�����ʽ
};

struct RebarData
{
	BrString rebarSize;				//�ֽ�ߴ�
	double rebarSpacing;			//�ֽ���
};

struct FaceRebarData
{
	RebarData HRebarData;			//ˮƽ�ֽ�����
	RebarData VRebarData;			//����ֽ�����
};

struct ComponentData				//�������������
{
	double positiveCover;			//���汣����
	double reverseCover;			//���汣����
	double sideCover;				//���汣����
	double length;					//��
	double width;					//��
	double height;					//��
	FaceRebarData	posRebarData;	//����ֽ�����
	FaceRebarData	revRebarData;	//����ֽ�����
};

//����
class TieRebarAssembly : public RebarAssembly
{
	BE_DATA_REFER(BeMatrix, Placement)				//�任����
	BE_DATA_VALUE(int, TieRebar)					//�Ƿ�ʹ������
	BE_DATA_VALUE(TieRebarStyle, style)				//�������ʽ
	BE_DATA_VALUE(BrString, rebarSize)				//�ֽ�ߴ�
	BE_DATA_VALUE(int, rebarType)					//�ֽ�����
	BE_DATA_VALUE(ComponentData, componentData)		//����������

	BE_DATA_VALUE(ElementId, setId)					//SetId

public:
	TieRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) {}
	virtual ~TieRebarAssembly() {}

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 2; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"TieRebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"TieRebar"; }
	virtual void		CalculateTransform(CVector3D& transform, DgnModelRefP modelRef);	//����ֽ��ƫ��
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(TieRebarAssembly, RebarAssembly)

private:

	RebarSetTag* MakeRebars(ElementId& rebarSetId,double length, CVector3D const& endNormal, CVector3D const & vecTrans, DgnModelRefP modelRef);
	bool makeRebarCurve(RebarCurve& rebar, double xPos, double length, double bendRadius, double bendLen, RebarEndTypes const& endType, CVector3D const& endNormal, CMatrix3D const& mat);

public:
	virtual bool	SetWallData(ElementHandleCR eh);

public:
	virtual bool	MakeRebars(DgnModelRefP modelRef);
};

