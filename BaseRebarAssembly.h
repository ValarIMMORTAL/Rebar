#include "Public.h"
#include <RebarElements.h>
#include "PITRebarCurve.h"
#include "PITPSPublic.h"
#include "ExtractFacesTool.h"
#include "MySlabRebarAssembly.h"
#include "StirrupRebar.h"
enum BaseType
{
	SquareBase,//方形基础
	RoundBase,//圆型基础
	OtherBase
};

class BaseRebarDlg;

class BaseStirrupRebar : public PIT::StirrupRebar
{

public:
	vector<CPoint3D> m_vecRebarPt;
	StirrupData m_StirrupData;	//箍筋对话框获取的数据

	BE_DATA_VALUE(PIT::StirrupRebarData, rebarData)

public:
	explicit BaseStirrupRebar(const vector<CPoint3D> &vecRebarPts, PIT::StirrupRebarDataCR rebarData);
private:
	PIT::PITRebarCurve  m_Curve;

};


class BaseStirrupRebarMaker : public PIT::StirrupRebarMaker
{

	BE_DATA_VALUE(bool, bUp)
public:
	bool IsArcBase;
	BaseStirrupRebarMaker(const vector<ElementId>& rebar_Vs, const vector<ElementId>  &rebarId_Hs, PIT::StirrupRebarDataCR rebarData, bool bUp = false, DgnModelRefP modelRef = ACTIVEMODEL);

public:
	vector<shared_ptr<BaseStirrupRebar> >	m_pStirrupRebars;
	vector<vector<CPoint3D>	>			m_vecStirrupRebarPts;
	virtual RebarSetTag* MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef = ACTIVEMODEL);
	bool CalculateRound(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt, int nStep);
	bool makeRoundRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::PITRebarEndTypes& endTypes, DgnModelRefP modelRef, double& levelSpacing,int& rebarNum, double diameter);
	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);
	bool	CalStirrupRebarPts(vector<CPoint3D> &pts, PIT::StirrupRebarDataCR rebarData, STWallGeometryInfo& baseData, double& SideCover, double& PositiveCover, double& StartOffset, bool up, ElementHandleR m_ehSel);

private:
	struct arcBasePoint
	{
		DPoint3d ptStr;
		DPoint3d ptMid;
		DPoint3d ptEnd;
		DPoint3d centerPtr;
		double ArcRadius;
	}stArcPoint;
	
};


class  AbanurusRebarAssembly : public STSlabRebarAssembly
 {
 public:
	BE_DATA_VALUE(StirrupData, StirrupData)
	BE_DATA_VALUE(double, Len)			//延申长度
public:
	void AbanurusRebarAssembly::InitRebarParam(double ulenth);
	AbanurusRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~AbanurusRebarAssembly()
	{
	};
	void Init();
	DgnModelRefP m_modelRef;
	BaseRebarDlg * m_pBaseRebarMainDlg;
	static bool IsSmartSmartFeature(EditElementHandle& eeh);

	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 100; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"BaseRebar Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"BaseRebar Rebar"; }	
	virtual bool        OnDoubleClick() override;
	void SetBaseType(BaseType basetype);
	void SetConcreteData(PIT::Concrete const& concreteData, StirrupData const& stirrpinfo,double& Len, Abanurus_PTRebarData const& ptRebarinfo);
	void SetRebarData(vector<PIT::ConcreteRebar> const& vecData);
	bool AnalyzingSlabGeometricData(ElementHandleCR eh);
	bool SetBaseData(ElementHandleCR eh);
	bool SetSlabData(ElementHandleCR eh);
	bool MakeRebars(DgnModelRefP modelRef);
	BE_DECLARE_VMS(AbanurusRebarAssembly, RebarAssembly)
	void SetStirrupPts(int &pt_Hnum, int pt_Vnum, int &pt_Vanum, int & strrrupHNum, int &strrrupVNum, double & pt_radius, double &strp_radius, double & highZ, double &dLevelSpace, PIT::StirrupRebarData &rebarData, RebarSetTagArray &tmprsetTags,DgnModelRefP modelRef);
	bool m_isHtieRebar = false;
	bool m_isVtieRebar = false;

	//生成拉筋
	RebarSetTag* MakeTieRebar(vector<vector<Dpoint3d>>  &rebarPts);

private:
	char m_StirrupRebaSize[512];		//箍筋尺寸
	DgnModelRefP m_model;
	BaseStirrupRebarMaker *m_PStirrupRebarMaker;
	ElementHandle m_ehSel;
	bool isarc;
};