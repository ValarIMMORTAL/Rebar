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

using namespace PIT;
class  CHoleRebar_ReinforcingDlg;
class CHoleRebar_StructualDlg;
void GetRebarIDSIntesectWithHole(int& minid, int& maxid, EditElementHandleR eehHole, vector<RebarPoint>& rebarPts);
class HoleRFRebarAssembly : public RebarAssembly
{

public:
	BE_DATA_REFER(ACCConcrete, accConcrete)
	BE_DATA_REFER(Transform, Trans)
	BE_DATA_VALUE(vector<BrString>,		vecDirSize)				//尺寸
	BE_DATA_VALUE(vector<int>, vecRebarLevel)				
	BE_DATA_VALUE(std::vector<HoleRebarInfo::ReinForcingInfo>, vecReinF)			//孔洞加强筋设置数据
	BE_DATA_VALUE(std::vector<RebarPoint>, rebarPts)			//钢筋所有点坐标
	BE_DATA_VALUE(vector<ElementId>, vecSetId)								//SetId
	BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)
	BE_DATA_VALUE(int, typenum)
public:
	HoleRFRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~HoleRFRebarAssembly()
	{
	};
public:
	void ClearData();
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	vector<EditElementHandle*> m_useHoleehs;
	std::string m_strElmName;
	std::map<int, vector<RebarPoint>> m_LayerRebars;
	std::map<int, vector<RebarPoint>> m_LayerTwinRebars;
	CHoleRebar_ReinforcingDlg* m_holedlg;
	bool isfloor;

protected:
	void			Init();
public:
	void SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas);
	 bool	MakeRebars(DgnModelRefP modelRef);
	 static bool   IsSmartSmartFeature(EditElementHandle& eeh);
	 static RebarAssembly* GetRebarAssembly(ElementId concreteid, string assemblyname);
	 static void GetUnionHoleeehAndSize(EditElementHandle* eehhole, double& holesize, vector<HoleRebarInfo::ReinForcingInfo>& vecReininfo,
		 Transform trans, string UnionName, std::map<std::string, IDandModelref>& m_holeidAndmodel);
	 static void GetUnionHoleeeh(EditElementHandle* eehhole, vector<string>& vecUnionchildname, double& holesize, Transform trans,
		 std::map<std::string, IDandModelref>& m_holeidAndmodel);
	void  SetLayerRebars();
	void  TransFromRebarPts(vector<RebarPoint>&  rebarPts);
	void  CalculateTransByFrontPts(HoleRebarInfo::ReinForcingInfo tmpinfo);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 5; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(HoleRFRebarAssembly, RebarAssembly)

		RebarSetTag* MakeRebars(
			HoleRebarInfo::ReinForcingInfo tmpinfo,
			EditElementHandle& eehHole,
			EditElementHandle& eehHolePlus,
			vector<RebarPoint>&  rebarPts,
			ElementId&          rebarSetId,
			BrStringCR          sizeKey,
			DgnModelRefP        modelRef,
			int DataExchange
		);
	bool makeRebarCurve
	(
		RebarCurve&     rebar,
		double                  bendRadius,
		double                  bendLen,
		RebarEndTypes const&    endTypes,
		CPoint3D const&        ptstr,
		CPoint3D const&        ptend
	);
};

class HoleSTRebarAssembly : public RebarAssembly
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
		BE_DATA_VALUE(vector<int>, vecRebarLevel)		
		BE_DATA_VALUE(std::vector<HoleRebarInfo::ReinForcingInfo>, vecReinF)			//孔洞加强筋设置数据
		BE_DATA_VALUE(std::vector<RebarPoint>, rebarPts)			//钢筋所有点坐标
		BE_DATA_VALUE(std::vector<RebarPoint>, twinrebarPts)			//并筋所有点坐标
		BE_DATA_VALUE(vector<ElementId>, vecSetId)								//SetId
		BE_DATA_VALUE(double ,FrontdiameterSide)    //前一个侧向构造筋直径，主要用来判断横向和竖向的构造筋错开位置
		BE_DATA_VALUE(double, FrontMaindiameter)    //前一层主筋直径，主要用来判断横向和竖向的构造筋错开位置
		BE_DATA_VALUE(double, Maindiameter)         //当前层主筋直径，主要用来判断横向和竖向的构造筋错开位置
		BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)
		BE_DATA_VALUE(int, typenum)
		BE_DATA_VALUE(vector<int> , produce)//第一个值表示是否生成横筋，第二个表示是否生成u型钢筋
    struct HoleSTData
	{
		double sideSpacing;
		int sideNum;
		double diameter;
		double diameterU; 
		double L0Lenth; 
		double bendRadiusU;
		double bendLenU;
		double LaLenth;
		double diameterSide;
		double bendRadiusS;
		double bendLenS;
		BrString sizeKeyU;//计算U形钢筋直径和LO长度
		BrString sizeKeyS;//计算侧向构造筋直径和LA长度
	};
	HoleSTData m_stdata;

public:
	enum Direction
	{
		Left,
		Right,
		Up,
		Down
	};

	HoleSTRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~HoleSTRebarAssembly() 
	{
		
	};
public:
	void ClearData();
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	vector<EditElementHandle*> m_useHoleehs;
	std::string m_strElmName;//当前选择的孔洞名称
	std::map<int, vector<RebarPoint>> m_LayerRebars;
	std::map<int, vector<RebarPoint>> m_LayerTwinRebars;
	std::vector<TwinBarSet::TwinBarLevelInfo>	m_vecTwinBarData;
	CHoleRebar_StructualDlg* m_holedlg;
	TwinBarSet::TwinBarLevelInfo m_twinbarinfo;
	bool isfloor;
protected:
	void			Init();
public:
	void SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas);
	bool	MakeRebars(DgnModelRefP modelRef);
	
	void  SetLayerRebars();
	void  TransFromRebarPts(vector<RebarPoint>&  rebarPts);
	void CreateURebars(DPoint3d pt, double diameter, double diameterU, double L0Lenth, double distance, double uor_per_mm,
		TransformInfo transinfo, DPoint3d minP, DPoint3d maxP, DPoint3d ptstr, Direction dir, PosValue Pvalue,
		double bendLen, double bendRadius, RebarEndTypes endTypes, vector<PITRebarCurve>&   rebarCurvesNum,bool ishaveTwinBar = false);
	void CreateSideRebars(vector<DPoint3d>& pts, int sideNum, double sideSpacing, double LaLenth,
		TransformInfo transinfo, DPoint3d minP, DPoint3d maxP, double diameter, double diameterSide, double uor_per_mm, Direction dir, PosValue Pvalue,
		double bendLen, double bendRadius, RebarEndTypes endTypes, vector<PITRebarCurve>&   rebarCurvesNum);
	void CalculateTransByFrontPts(HoleRebarInfo::ReinForcingInfo tmpinfo);
	void CalculateUSizeAndSpacing(DgnModelRefP modelRef, HoleRebarInfo::ReinForcingInfo tmpinfo, 
		BrStringCR  sizeKey,double distance, double Wthickness,bool flag);
	void CalculateMinMaxPos(PosValue& Pvalue, vector<RebarPoint>& tmprebarPtsF, vector<RebarPoint> tmprebarPtsB, double diameter, int dir);
protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 6; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar STRUCTUAL"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar  STRUCTUAL"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(HoleSTRebarAssembly, RebarAssembly)

		RebarSetTag* MakeRebars(
			double Wthickness,
			HoleRebarInfo::ReinForcingInfo tmpinfo,
			EditElementHandle& eehHole,
			EditElementHandle& eehHolePlus,
			vector<RebarPoint>&  rebarPtsF,
			vector<RebarPoint>&  rebarPtsB,
			ElementId&          rebarSetId,
			BrStringCR          sizeKey,
			DgnModelRefP        modelRef,
			RebarSetTag*		tagS,
			bool                Holestrucal = false
		);
	RebarSetTag* HoleSTRebarAssembly::MakeRebars
	(
		double Wthickness,
		HoleRebarInfo::ReinForcingInfo tmpinfo,
		vector<string>& holenames,
		vector<RebarPoint>&  rebarPtsF,
		vector<RebarPoint>&  rebarPtsB,
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		DgnModelRefP        modelRef,
		RebarSetTag*		tagS,
		bool                Holestrucal = false
	);
	
};
class HoleArcRFRebarAssembly : public RebarAssembly
{

public:
	BE_DATA_REFER(ACCConcrete, accConcrete)
		BE_DATA_REFER(Transform, Trans)
		BE_DATA_VALUE(vector<BrString>, vecDirSize)				//尺寸
		BE_DATA_VALUE(vector<int>,		vecRebarLevel)			//钢筋层
		BE_DATA_VALUE(std::vector<HoleRebarInfo::ReinForcingInfo>, vecReinF)			//孔洞加强筋设置数据
		BE_DATA_VALUE(std::vector<RebarPoint>, rebarPts)			//钢筋所有点坐标
		BE_DATA_VALUE(vector<ElementId>, vecSetId)								//SetId
		BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)
		BE_DATA_VALUE(int, typenum)
public:
	HoleArcRFRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~HoleArcRFRebarAssembly() 
	{
	};
public:
	void ClearData();
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	vector<EditElementHandle*> m_useHoleehs;
	std::string m_strElmName;
	std::map<int, vector<RebarPoint>> m_LayerRebars;
	std::map<int, vector<RebarPoint>> m_LayerTwinRebars;
	CHoleRebar_ReinforcingDlg* m_holedlg;
protected:
	void			Init();
public:
	void SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas);
	bool	MakeRebars(DgnModelRefP modelRef);
	static bool   IsSmartSmartFeature(EditElementHandle& eeh);
	static RebarAssembly* GetRebarAssembly(ElementId concreteid, string assemblyname);
	static void GetUnionHoleeehAndSize(EditElementHandle* eehhole, double& holesize, vector<HoleRebarInfo::ReinForcingInfo>& vecReininfo,
		Transform trans, string UnionName, std::map<std::string, IDandModelref>& m_holeidAndmodel);
	static void GetUnionHoleeeh(EditElementHandle* eehhole, vector<string>& vecUnionchildname, double& holesize, Transform trans,
		std::map<std::string, IDandModelref>& m_holeidAndmodel);
	void  SetLayerRebars();
	void  TransFromRebarPts(vector<RebarPoint>&  rebarPts);
	void  CalculateTransByFrontPts(HoleRebarInfo::ReinForcingInfo tmpinfo);
	static void GetIDSIntesectWithHole(int& minid, int& maxid, EditElementHandleR eehHole, vector<RebarPoint>& rebarPts);
	static void CalculateIntersetPtWithHolesWithARCRebarCuve(DPoint3d& ptStr, DPoint3d& PtEnd, DPoint3d MidPt, vector<EditElementHandleP>& allholes);
	bool CalculateUpAndDownRebarcurve(EditElementHandle* eehHole, DPoint3d minP, DPoint3d maxP,
		RebarPoint rebrptmin, double Posz, HoleRebarInfo::ReinForcingInfo tmpinfo, vector<RebarCurve>& rebarCurvesNum);
protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 5; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(HoleArcRFRebarAssembly, RebarAssembly)

		RebarSetTag* MakeRebars(
			HoleRebarInfo::ReinForcingInfo tmpinfo,
			EditElementHandle& eehHole,
			EditElementHandle& eehHolePlus,
			vector<RebarPoint>&  rebarPts,
			ElementId&          rebarSetId,
			BrStringCR          sizeKey,
			DgnModelRefP        modelRef,
			int DataExchange
		);
	bool makeRebarCurve
	(
		RebarCurve&     rebar,
		double                  bendRadius,
		double                  bendLen,
		RebarEndTypes const&    endTypes,
		CPoint3D const&        ptstr,
		CPoint3D const&        ptend
	);
};

class HoleArcSTRebarAssembly : public RebarAssembly
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
		BE_DATA_VALUE(vector<int>, vecRebarLevel)				
		
		BE_DATA_VALUE(std::vector<HoleRebarInfo::ReinForcingInfo>, vecReinF)			//孔洞加强筋设置数据
		BE_DATA_VALUE(std::vector<RebarPoint>, rebarPts)			//钢筋所有点坐标
		BE_DATA_VALUE(std::vector<RebarPoint>, twinrebarPts)			//并筋所有点坐标
		BE_DATA_VALUE(vector<ElementId>, vecSetId)								//SetId
		BE_DATA_VALUE(double, FrontdiameterSide)    //前一个侧向构造筋直径，主要用来判断横向和竖向的构造筋错开位置
		BE_DATA_VALUE(double, FrontMaindiameter)    //前一层主筋直径，主要用来判断横向和竖向的构造筋错开位置
		BE_DATA_VALUE(double, Maindiameter)         //当前层主筋直径，主要用来判断横向和竖向的构造筋错开位置
		BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)
		BE_DATA_VALUE(int, typenum)
		BE_DATA_VALUE(DPoint3d, CenterPt)
		BE_DATA_VALUE(double, OutRadius)
		BE_DATA_VALUE(double, InRadius)
		BE_DATA_VALUE(vector<int>,producerebar)
public:
	enum Direction
	{
		Left,
		Right,
		Up,
		Down
	};

	HoleArcSTRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~HoleArcSTRebarAssembly() 
	{

	};
public:
	void ClearData();
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	vector<EditElementHandle*> m_useHoleehs;
	std::string m_strElmName;
	std::map<int, vector<RebarPoint>> m_LayerRebars;
	std::map<int, vector<RebarPoint>> m_LayerTwinRebars;
	std::vector<TwinBarSet::TwinBarLevelInfo>	m_vecTwinBarData;
	CHoleRebar_StructualDlg* m_holedlg;
	TwinBarSet::TwinBarLevelInfo m_twinbarinfo;
protected:
	void			Init();
public:
	void SetVecDirSizeData(std::vector<PIT::ConcreteRebar>& wallRebarDatas);
	bool	MakeRebars(DgnModelRefP modelRef);

	void  SetLayerRebars();
	void  TransFromRebarPts(vector<RebarPoint>&  rebarPts);
	void CreateURebars(DPoint3d pt, double diameter, double diameterU, double L0Lenth, double distance, double uor_per_mm,
		TransformInfo transinfo, DPoint3d minP, DPoint3d maxP, DPoint3d ptstr, Direction dir, PosValue PvalueF, PosValue PvalueB,
		double bendLen, double bendRadius, RebarEndTypes endTypes, vector<PITRebarCurve>&   rebarCurvesNum, bool ishaveTwinBar = false);
	void CreateSideRebars(vector<DPoint3d>& pts, int sideNum, double sideSpacing, double LaLenth,
		TransformInfo transinfo, DPoint3d minP, DPoint3d maxP, double diameter, double diameterSide, double uor_per_mm, Direction dir, 
		PosValue PvalueF, PosValue PvalueB,double bendLen, double bendRadius, RebarEndTypes endTypes, vector<PITRebarCurve>&   rebarCurvesNum);
	void CalculateTransByFrontPts(HoleRebarInfo::ReinForcingInfo tmpinfo);
	void CalculateArcLenth(DPoint3d& ptstr, DPoint3d& ptend, double L0Lenth,
		Direction dir, PosValue Pvalue, double uor_per_mm, TransformInfo transinfo);
protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 6; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar STRUCTUAL"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Hole Rebar  STRUCTUAL"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(HoleArcSTRebarAssembly, RebarAssembly)

		RebarSetTag* MakeRebars(
			double Wthickness,
			HoleRebarInfo::ReinForcingInfo tmpinfo,
			EditElementHandle& eehHole,
			EditElementHandle& eehHolePlus,
			vector<RebarPoint>&  rebarPtsF,
			vector<RebarPoint>&  rebarPtsB,
			ElementId&          rebarSetId,
			BrStringCR          sizeKey,
			DgnModelRefP        modelRef,
			bool                flag = false
		);
	RebarSetTag* HoleArcSTRebarAssembly::MakeRebars
	(
		double Wthickness,
		HoleRebarInfo::ReinForcingInfo tmpinfo,
		vector<string>& holenames,
		vector<RebarPoint>&  rebarPtsF,
		vector<RebarPoint>&  rebarPtsB,
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		DgnModelRefP        modelRef,
		bool                flag=false
	);

};

