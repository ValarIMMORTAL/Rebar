#pragma once

/*--------------------------------------------------------------------------------------+
|
|     $Source: WallRebarAssembly.h $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "RebarDetailElement.h"
#include <RebarElements.h>
#include "PITRebarCurve.h"
#include "CommonFile.h"
class CDoorHoleDlg;
class DoorHoleRebarAssembly : public RebarAssembly
{
	struct PosValue
	{
		double    minx;
		double	  maxx;
		double	  minz;
		double	  maxz;
	};
public:
	BE_DATA_REFER(ACCConcrete, accConcrete)
		BE_DATA_REFER(Transform, Trans)
		BE_DATA_VALUE(vector<BrString>, vecDirSize)				//尺寸
		BE_DATA_VALUE(std::vector<HoleRebarInfo::ReinForcingInfo>, vecReinF)			//孔洞加强筋设置数据
		BE_DATA_VALUE(std::vector<RebarPoint>, rebarPts)			//钢筋所有点坐标
		BE_DATA_VALUE(std::vector<RebarPoint>, twinrebarPts)			//并筋所有点坐标
		BE_DATA_VALUE(vector<ElementId>, vecSetId)								//SetId
		BE_DATA_VALUE(double, FrontdiameterSide)    //前一个侧向构造筋直径，主要用来判断横向和竖向的构造筋错开位置
		BE_DATA_VALUE(double, FrontMaindiameter)    //前一层主筋直径，主要用来判断横向和竖向的构造筋错开位置
		BE_DATA_VALUE(double, Maindiameter)         //当前层主筋直径，主要用来判断横向和竖向的构造筋错开位置
		BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)
public:
	enum Direction
	{
		Left,
		Right,
		Up,
		Down
	};

	DoorHoleRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~DoorHoleRebarAssembly() 
	{
		
	};
public:
	void ClearData();
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	std::map<std::string, IDandModelref> m_NEGholeidAndmodel;
	vector<EditElementHandle*> m_useHoleehs;
	std::string m_strElmName;
	std::map<int, vector<RebarPoint>> m_LayerRebars;
	std::map<int, vector<RebarPoint>> m_LayerTwinRebars;
	std::vector<TwinBarSet::TwinBarLevelInfo>	m_vecTwinBarData;
	CDoorHoleDlg* m_holedlg;
	TwinBarSet::TwinBarLevelInfo m_twinbarinfo;
protected:
	void			Init();
public:
	void SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas);
	bool	MakeRebars(DgnModelRefP modelRef);

	void  SetLayerRebars();
	void  TransFromRebarPts(vector<RebarPoint>&  rebarPts);
	void CreateURebars(DPoint3d pt, double diameter, double diameterU, double L0Lenth, double distance, double uor_per_mm,
		TransformInfo transinfo, DPoint3d minP, DPoint3d maxP, DPoint3d ptstr, Direction dir, PosValue Pvalue,
		double bendLen, double bendRadius, RebarEndTypes endTypes, vector<PITRebarCurve>&   rebarCurvesNum, bool ishaveTwinBar = false);
	void CreateSideRebars(vector<DPoint3d>& pts, int sideNum, double sideSpacing, double LaLenth,
		TransformInfo transinfo, DPoint3d minP, DPoint3d maxP, double diameter, double diameterSide, double uor_per_mm, Direction dir, PosValue Pvalue,
		double bendLen, double bendRadius, RebarEndTypes endTypes, vector<PITRebarCurve>&   rebarCurvesNum);
	void CalculateTransByFrontPts(HoleRebarInfo::ReinForcingInfo tmpinfo);
	void MakeUrebar(EditElementHandleR  eehHole, int& tagID, DgnModelRefP modelRef,
		RebarSetTagArray& rsetTags, double Wthickness, HoleRebarInfo::ReinForcingInfo tmpinfo, int theSec);
	void GetNowUseHole(EditElementHandleR eehHole);//获取当前使用的孔洞集合，排除当前的门洞
	void ApplyTransToHoles(Transform trans);
protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 16; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar DOOR"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar  DOOR"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(DoorHoleRebarAssembly, RebarAssembly)

		RebarSetTag* MakeMainURebars(
			double Wthickness,
			HoleRebarInfo::ReinForcingInfo tmpinfo,
			EditElementHandle& eehHole,
			EditElementHandle& eehHolePlus,
			vector<RebarPoint>&  rebarPtsF,
			vector<RebarPoint>&  rebarPtsB,
			ElementId&          rebarSetId,
			BrStringCR          sizeKey,
			DgnModelRefP        modelRef
		);
	void MakeLineStringsrebar(EditElementHandleR  eehHole, EditElementHandleR  NegeehHole, int& tagID, DgnModelRefP modelRef,
		RebarSetTagArray& rsetTags, double Wthickness, HoleRebarInfo::ReinForcingInfo tmpinfo, int theSec);

	RebarSetTag* MakeLineStringRebars
	(
		double Wthickness,
		HoleRebarInfo::ReinForcingInfo tmpinfo,
		EditElementHandle& eehHole,
		EditElementHandle& eehHolePlus,
		EditElementHandleR NegeehHole,
		EditElementHandleR NegHolePlus,
		vector<RebarPoint>&  rebarPtsF,
		vector<RebarPoint>&  rebarPtsB,
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		DgnModelRefP        modelRef,
		RebarSetTag* tagD
	);
	void CtreateLineStringRebarCuve(DPoint3d ptIntersect, DPoint3d ptIntersectNeg, vector<PITRebarCurve>& rebarCurvesNumS,
		double diameterSide, double diameter, double Wthickness, double bendRadiusS, double bendLenS,
		DgnModelRefP modelRef, PosValue Pvalue, DPoint3d maxP, DPoint3d minP,
		double L0Lenth, double LaLenth, Direction dir, bool isNegInFront);
	//添加附加U形筋和附加点筋
	void CtreateAdditionalRebarCuve
	(
		DPoint3d ptIntersect, DPoint3d ptIntersectNeg, vector<PITRebarCurve>& rebarCurvesNumAU,
		vector<PITRebarCurve>& rebarCurvesNumAD, double diameterU, double diameter,
	double Wthickness, double bendRadiusU, double bendLenU,
	DgnModelRefP modelRef, PosValue Pvalue, DPoint3d maxP, DPoint3d minP,
	double L0LenthU, Direction dir, bool isNegInFront, double disB
	);
	vector<EditElementHandle*> m_NowuseHoleehs;
};


