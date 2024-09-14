#pragma once
#include <RebarElements.h>

enum TieRebarStyle
{
	XX,			//X*X
	XX2,		//X*2X
	X2X,		//2X*X
	X2X2,		//2X*2X
	Custom,		//自定义样式
};

struct RebarData
{
	BrString rebarSize;				//钢筋尺寸
	double rebarSpacing;			//钢筋间距
};

struct FaceRebarData
{
	RebarData HRebarData;			//水平钢筋数据
	RebarData VRebarData;			//竖向钢筋数据
};

struct ComponentData				//待配拉筋构件数据
{
	double positiveCover;			//正面保护层
	double reverseCover;			//反面保护层
	double sideCover;				//侧面保护层
	double length;					//长
	double width;					//宽
	double height;					//高
	FaceRebarData	posRebarData;	//正面钢筋数据
	FaceRebarData	revRebarData;	//反面钢筋数据
};

//拉筋
class TieRebarAssembly : public RebarAssembly
{
	BE_DATA_REFER(BeMatrix, Placement)				//变换矩阵
	BE_DATA_VALUE(int, TieRebar)					//是否使用拉筋
	BE_DATA_VALUE(TieRebarStyle, style)				//拉筋布置样式
	BE_DATA_VALUE(BrString, rebarSize)				//钢筋尺寸
	BE_DATA_VALUE(int, rebarType)					//钢筋类型
	BE_DATA_VALUE(ComponentData, componentData)		//待配筋构建数据

	BE_DATA_VALUE(ElementId, setId)					//SetId

public:
	TieRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) {}
	virtual ~TieRebarAssembly() {}

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 2; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"TieRebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"TieRebar"; }
	virtual void		CalculateTransform(CVector3D& transform, DgnModelRefP modelRef);	//整体钢筋的偏移
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

