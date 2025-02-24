#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	墙配筋
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/02/19
	Version:		V1.0
*	Description:	WallRebarAssembly
*	History:
*	1. Date:		2021/02/19
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/

#include <RebarElements.h>
#include "../CommonFile.h"
#include "../PITRebarEndType.h"
#include "../PITRebarCurve.h"
#include "../PITArcSegment.h"
#include "../PITRebarAssembly.h"
#define  MaxWallThickness     600          //折线墙最大处理墙厚处理

class CWallRebarDlgNew;
class WallRebarAssemblyNew : public PIT::PITRebarAssembly
{
public:
	enum WallType
	{
		STWALL,
		GWALL,
		ARCWALL,
		ELLIPSEWall,
		Other
	};

	enum  ElementType
	{
		WALL,
		FLOOR,
		EOther
	};

	BE_DATA_VALUE(string, wallName)				//墙名称
		BE_DATA_REFER(BeMatrix, Placement)
		BE_DATA_VALUE(bool, bACCRebar)				//是否关联构件配筋
		BE_DATA_VALUE(UINT, ACCRebarMethod)			//关联构件配筋方式
		BE_DATA_VALUE(double, PositiveCover)			//正面保护层
		BE_DATA_VALUE(double, ReverseCover)			//反面保护层
		BE_DATA_VALUE(double, SideCover)				//侧面保护层
		BE_DATA_VALUE(int, RebarLevelNum)			//钢筋层数
	//	BE_DATA_VALUE(int,					IsStaggered)			//是否交错
		BE_DATA_VALUE(vector<int>, vecDir)					//方向,0表示x轴，1表示z轴
		BE_DATA_VALUE(vector<BrString>, vecDirSize)				//尺寸
		BE_DATA_VALUE(vector<int>, vecRebarType)			//钢筋类型
		BE_DATA_VALUE(vector<double>, vecDirSpacing)			//间隔
		BE_DATA_VALUE(vector<double>, vecStartOffset)			//起点偏移
		BE_DATA_VALUE(vector<double>, vecEndOffset)			//终点偏移
		BE_DATA_VALUE(vector<double>, vecLevelSpace)			//与前层间距
		BE_DATA_VALUE(vector<int>, vecDataExchange)			//数据交换
		BE_DATA_VALUE(vector<int>, vecRebarLevel)			// 钢筋层号，分前、后、中间层

		BE_DATA_VALUE(vector<PIT::LapOptions>, vecLapOptions)			//搭接选项
		BE_DATA_VALUE(vector<vector<PIT::EndType> >, vvecEndType)		//端部样式
		BE_DATA_VALUE(vector<TwinBarSet::TwinBarLevelInfo >, vecTwinRebarLevel)	//并筋层
		BE_DATA_VALUE(vector<ElementId>, vecSetId)								//SetId
		BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)     //前面线的所有点
		BE_DATA_VALUE(CutRebarInfo, stCutRebarInfo)		 // 切割钢筋信息
		BE_DATA_VALUE(bool, isReserveCut)				 // 是否反向切割钢筋

		BE_DATA_VALUE(std::vector<CNCutRebarInfo>, vecCutInfo)		// 截断信息

public:
	WallRebarAssemblyNew(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~WallRebarAssemblyNew() {};
	double m_width;
	std::vector<ElementRefP> m_selectrebars;

public:
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//筛选后的孔洞
	std::map<EditElementHandle*, EditElementHandle*> m_doorsholes;//所有门洞，及门洞上的负实体
	std::vector<DPoint3d>     m_vecRebarPtsLayer;//被孔洞截取前的所有钢筋点数据
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//被孔洞截取前的所有并筋点数据
	std::vector<DPoint3d>     m_vecTieRebarPtsLayer; //拉筋生成所需要的钢筋点数据

public:
	vector<vector<DSegment3d> > m_vecRebarStartEnd;	//规避了孔洞的所有点

	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;//未规避孔洞的所有点

	vector<ElementRefP> m_allLines;//预览按钮按下后的所有钢筋线

	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

	RebarAssembly* m_pOldRebaras;

private:
	BE_DATA_VALUE(WallType,				wallType)				//墙类型
protected:
	void			Init();

protected:
	virtual int		GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Wall; }

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh) { return true; }


public:
	virtual bool	InsertMakeRebars(DgnModelRefP modelRef) { return true; }
	virtual bool	MakeRebars(DgnModelRefP modelRef) { return true; }
	virtual bool	SetWallData(ElementHandleCR eh) { return true; }

	double floadToInt(double dSrc);

	/***********************************************************************************************
	*****					切割中线钢筋
	*****	ptStr		:	直线钢筋起点
	*****	ptEnd		：	直线钢筋终点
	*****	vecSplit	:	分割距离 按 离起点距离
	************************************************************************************************/
	virtual bool CutLineRebarCurve(std::vector<PIT::PITRebarCurve>& rebars, PIT::PITRebarEndTypes& endTypes, DPoint3d& ptStr, DPoint3d& ptEnd, vector<double>& vecSplit);

	/***********************************************************************************************
	*****	ptStr		:	直线钢筋起点
	*****	ptEnd		：	直线钢筋终点
	*****	vecSplit	:	分割距离 按 离起点距离
	************************************************************************************************/
	bool CalaLineRebarCutPoint(DPoint3dCR ptStr, DPoint3dCR ptEnd, vector<double>& vecSplit, double diameterTol, double preLength, bool bFlag);

	void SetCutLenIndex()
	{
		double uor_per_m = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter();
		std::vector<double> m_vecDouble;
		m_vecDouble.push_back(m_stCutRebarInfo.dCutLength1);
		m_vecDouble.push_back(m_stCutRebarInfo.dCutLength2);
		m_vecDouble.push_back(m_stCutRebarInfo.dCutLength3);
		
		sort(m_vecDouble.begin(), m_vecDouble.end());

		m_CutLenIndex[0] = m_vecDouble.at(0) * uor_per_m;
		m_CutLenIndex[1] = m_vecDouble.at(1) * uor_per_m;
		m_CutLenIndex[2] = m_vecDouble.at(2) * uor_per_m;
	}

	virtual void	InitUcsMatrix() {}

// 	//通过关联构件配筋
 	virtual bool	MakeACCRebars(DgnModelRefP modelRef) { return true; }
// 	//end

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef) {}

	static WallType JudgeWallType(ElementHandleCR eh);
	static ElementType JudgeElementType(ElementHandleCR eh);
	static bool IsWallSolid(ElementHandleCR eh);
	void SetConcreteData(PIT::Concrete const & concreteData);
	void SetRebarData(vector<PIT::ConcreteRebar> const& vecRebarData);
	void SetRebarEndTypes(vector<PIT::EndType> const& vvecEndTypes);
//	void SetTwinbarInfo(TwinBarSet::TwinBarInfo const& twInfo);
//	void GetTwinbarInfo(TwinBarSet::TwinBarInfo& twInfo);
	void InitRebarSetId();
	void GetConcreteData(PIT::Concrete& concreteData);
	void GetRebarData(vector<PIT::ConcreteRebar>& vecData) const;
	static bool IsSmartSmartFeature(EditElementHandle& eeh);
	void SetTieRebarInfo(TieReBarInfo const& tieRebarInfo);
	const TieReBarInfo GetTieRebarInfo() const;

	void ClearLines();

private:
	TieReBarInfo m_tieRebarInfo;

	double m_CutLenIndex[3];
};


class STWallRebarAssemblyNew : public WallRebarAssemblyNew
{
	BE_DATA_VALUE(STWallGeometryInfo, STwallData)			//STWall几何数据
//	BE_DATA_VALUE(bool, IsTwinrebar)			//在画rebarcuve时，判断是否为并筋层
public :
	DPoint3d m_LineNormal;
	CWallRebarDlgNew *pWallDoubleRebarDlg;
	double m_angle_left;
	double m_angle_right;
	virtual ~STWallRebarAssemblyNew()
	{
		for (int j = 0; j < m_Negs.size(); j++)
		{
			if (m_Negs.at(j) != nullptr)
			{
				delete m_Negs.at(j);
				m_Negs.at(j) = nullptr;
			}
		}
		for (int j = 0; j < m_Holeehs.size(); j++)
		{

			if (m_Holeehs.at(j) != nullptr)
			{
				delete m_Holeehs.at(j);
				m_Holeehs.at(j) = nullptr;
			}
		}


	};
	
private:
	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebar, double xPos, double height, double startOffset,
		double endOffset, PIT::PITRebarEndTypes &endType, CMatrix3D const& mat, double diameter, bool isStrLineCut, bool istwin = false);
	bool makeRebarCurve_Laption(RebarCurve& rebar, double xPos, double height, double bendRadius,
		double bendLen, RebarEndTypes const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool istwin = false);
	
	bool		m_isPushTieRebar; // 是否push进入拉筋的钢筋点中

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::STWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"STWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"STWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	
protected:
	/*

	*/
	RebarSetTag* MakeRebars(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, 
		double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, 
		CMatrix3D const& mat, const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel,int level, int grade, int DataExchange, DgnModelRefP modelRef);
//	RebarSetTag* MakeRebars_ACC(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat, const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel, DgnModelRefP modelRef);
	RebarSetTag* MakeRebars_Laption(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, PIT::LapOptions const& lapOptions, vector<PIT::EndType> const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(STWallRebarAssemblyNew, RebarAssembly)


public:
	STWallRebarAssemblyNew(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		WallRebarAssemblyNew(id, modelRef)
#ifdef PDMSIMPORT
		,pWall(NULL)
#endif
	{
		pWallDoubleRebarDlg = NULL;
		m_vecRebarStartEnd.clear();
	};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);

	virtual void	InitUcsMatrix();

//	virtual bool	MakeACCRebars(DgnModelRefP modelRef);

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef);
	void  CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef);
	void CalculateUseHoles(DgnModelRefP modelRef);

#ifdef PDMSIMPORT
private:	
	STWallComponent *pWall;
#endif
};


class GWallRebarAssemblyNew : public WallRebarAssemblyNew
{
	BE_DATA_VALUE(bool, IsTwinrebar)			//在画rebarcuve时，判断是否为并筋层
	enum GWallType
	{
		LineWall,			//折线墙
		ArcWall,			//弧形墙
		LineAndArcWALL,		//折线弧形墙
		Custom,				//自定义任意类型墙
	}m_GWallType;
public:
	DPoint3d m_LineNormal;
	STWallGeometryInfo m_STwallData;
	double m_angle_left;
	double m_angle_right;
	double m_sidecover;
	~GWallRebarAssemblyNew()
	{
		for (int j = 0; j < m_Negs.size(); j++)
		{
			if (m_Negs.at(j) != nullptr)
			{
				delete m_Negs.at(j);
				m_Negs.at(j) = nullptr;
			}
		}
		for (int j = 0; j < m_Holeehs.size(); j++)
		{

			if (m_Holeehs.at(j) != nullptr)
			{
				delete m_Holeehs.at(j);
				m_Holeehs.at(j) = nullptr;
			}
		}
	};

private:
	bool makeLineWallRebarCurve(RebarCurve& rebar, int dir, vector<CPoint3D> const& vecRebarVertex, double bendRadius, double bendLen, RebarEndTypes const& endTypes, CVector3D const& endNormal, CMatrix3D const& mat);

	bool makeArcWallRebarCurve(RebarCurve& rebar, double xPos, double height, double startOffset, double endOffset, double bendRadius, double bendLen, RebarEndTypes const& endTypes, CVector3D const& endNormal, CMatrix3D const& mat) {};

	bool makeLineAndArcWallRebarCurve(RebarCurve& rebar, double xPos, double height, double startOffset, double endOffset, double bendRadius, double bendLen, RebarEndTypes const& endTypes, CVector3D const& endNormal, CMatrix3D const& mat) {};

	void  CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::GWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"GWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"GWall Rebar"; }
//	virtual bool        OnDoubleClick() override;
//	virtual bool        Rebuild() override;

protected:
	/*

	*/
	RebarSetTag* MakeRebars_Transverse(ElementId& rebarSetId, BrStringCR sizeKey, vector<CPoint3D> vecPt,double spacing, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

	RebarSetTag* MakeRebars_Longitudinal(ElementId& rebarSetId, BrStringCR sizeKey, double &xDir, const vector<double> height, double spacing, double startOffset, double endOffset, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

	
	void JudgeGWallType(ElementHandleCR eh);

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef);

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);

	void CalculateUseHoles(DgnModelRefP modelRef);
	bool GetUcsAndStartEndData(int index, double thickness,DgnModelRefP modelRef);
	void GetMaxThickness(DgnModelRefP modelRef, double& thickness);
	RebarSetTag* MakeRebars
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		double              xLen,
		double              height,
		double              spacing,
		double              startOffset,
		double              endOffset,
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
		vector<CVector3D> const& vecEndNormal,
		CMatrix3D const&    mat,
		TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
		int level,
		int grade,
		int DataExchange,
		bool				bTwinbarLevel,
		DgnModelRefP        modelRef,
		bool  drawlast = true
	);
	bool makeRebarCurve
	(
		vector<PIT::PITRebarCurve>&     rebars,
		double                  xPos,
		double                  yLen,
		double					startOffset,
		double					endOffset,
		PIT::PITRebarEndTypes&		endTypes,
		CMatrix3D const&        mat,
		bool isTwin = false
	);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(GWallRebarAssemblyNew, RebarAssembly)

public:
public:
	GWallRebarAssemblyNew(ElementId id = 0, DgnModelRefP modelRef = NULL)
		:WallRebarAssemblyNew(id, modelRef)
#ifdef PDMSIMPORT
		, pWall(NULL)
#endif
	{};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);
private:

	struct GWallGeometryInfo
	{
		vector<DPoint3d> vecPositivePt;		//折线墙:正面底边顶点数组;弧形墙:正面底边起点终点加圆心确定一条弧，第一组为圆心加上起点终点，第二组为圆心加上上一条弧的终点和当前弧的终点，以此类推
		vector<DPoint3d> vecReversePt;		//折线墙:反面底边顶点数组;弧形墙:反面底边起点终点加圆心确定一条弧，第一组为圆心加上起点终点，第二组为圆心加上上一条弧的终点和当前弧的终点，以此类推
		vector<double> vecLength;			//折线墙:正面底边边长;弧形墙:正面每一段弧的半径
		double height;						//墙高
		double thickness;					//墙厚
	}m_GWallData;


	map<int, vector<DPoint3d>> m_vecLinePts;//底面前后线段的点，第一个标识为第几段

	bool m_isPushTieRebar;

#ifdef PDMSIMPORT
	GWallComponent *pWall;
#endif
};

class ArcWallRebarAssemblyNew : public WallRebarAssemblyNew
{
public:
	CWallRebarDlgNew *pArcWallDoubleRebarDlg;
private:

	bool CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);

//	bool makeArcWallRebarCurve(RebarCurve& rebar, double xPos, double height, double startOffset, double endOffset, double bendRadius, double bendLen, RebarEndTypes const& endTypes, CVector3D const& endNormal, CMatrix3D const& mat);

	bool makeLineRebarCurve
	(
		vector<PIT::PITRebarCurve>& 	rebar,
		PIT::ArcSegment				arcSeg,
		double					dLen,
		double                  space,
		double					startOffset,
		double					endOffset,
		PIT::PITRebarEndTypes&		endTypes
	);

	void CalculateUseHoles(DgnModelRefP modelRef);

	bool makeArcWallRebarCurve
	(
		vector<PIT::PITRebarCurve>& 	rebar,
		PIT::ArcSegment				arcSeg,
		double                  space,
		double					startOffset,
		double					endOffset,
		PIT::PITRebarEndTypes&		endTypes
	);

	RebarSetTag* MakeRebars_Line
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		PIT::ArcSegment			arcSeg,
		double              dLen,
		double              spacing,
		double              startOffset,
		double              endOffset,
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
		vector<CVector3D> const& vecEndNormal,
		TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
		int level,
		int grade,
		int DataExchange,
		bool				bTwinbarLevel,
		DgnModelRefP        modelRef
	);

	RebarSetTag* MakeRebars_Arc
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		PIT::ArcSegment			arcSeg,
		double              spacing,
		double              startOffset,
		double              endOffset,
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
		vector<CVector3D> const& vecEndNormal,
		TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
		int level,
		int grade,
		int DataExchange,
		bool				bTwinbarLevel,
		DgnModelRefP        modelRef
	);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::ArcWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(ArcWallRebarAssemblyNew, RebarAssembly)

public:
	ArcWallRebarAssemblyNew(ElementId id = 0, DgnModelRefP modelRef = NULL)
		:WallRebarAssemblyNew(id, modelRef)
	{
		pArcWallDoubleRebarDlg = nullptr;
		memset(&m_ArcWallData, 0, sizeof(ArcWallGeometryInfo));
	};

	~ArcWallRebarAssemblyNew()
	{

	};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);
private:
	struct ArcWallGeometryInfo
	{
		PIT::ArcSegment	OuterArc;		//
		PIT::ArcSegment	InnerArc;		//
		double height;				//墙高
		double thickness;			//墙厚
	}m_ArcWallData;

	bool		m_isPushTieRebar; // 是否push进入拉筋的钢筋点中
};

class ELLWallRebarAssemblyNew : public WallRebarAssemblyNew
{
public:
	CWallRebarDlgNew *pEllWallDoubleRebarDlg;
	virtual ~ELLWallRebarAssemblyNew()
	{
		for (int j = 0; j < m_Negs.size(); j++)
		{
			if (m_Negs.at(j) != nullptr)
			{
				delete m_Negs.at(j);
				m_Negs.at(j) = nullptr;
			}
		}
		for (int j = 0; j < m_Holeehs.size(); j++)
		{

			if (m_Holeehs.at(j) != nullptr)
			{
				delete m_Holeehs.at(j);
				m_Holeehs.at(j) = nullptr;
			}
		}
	};

private:
	// 生成点筋
	RebarSetTag* MakeRebar_Vertical
	(
		ElementId& rebarSetId,
		BrString sizeKey,
		DgnModelRefP modelRef,
		double startOffset,  // 起始偏移
		double endOffset,    // 终点偏移
		double spacing,		 // 间距
		double dRoundRadius, // 圆的半径
		double rebarLen,		 // 钢筋长度
		int level,
		int grade,
		int DataExchange,
		bool isTwinRebar = false // 是否是并筋
	);

	RebarSetTag* MakeRebar_Round
	(
		ElementId& rebarSetId,
		BrString sizeKey,
		DgnModelRefP modelRef,
		double startOffset, // 起始偏移
		double endOffset,   // 终点偏移
		double spacing,		// 间距
		double dRoundRadius,// 圆的半径
		int level,
		int grade,
		int DataExchange,
		bool isTwinRebar = false // 是否是并筋
	);

	bool CalculateRound(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt, int nStep = 0);

	bool CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);

	bool makeArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP mscArc, PIT::PITRebarEndTypes& endTypes, const vector<DPoint3d>& pts);

	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d ptStr, double dRebarLength);

	bool makeRoundRebarCurve(vector<PIT::PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d	centerPoint, double dRoundRadius);

	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);
protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::ELLWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ELLWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ELLWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(ELLWallRebarAssemblyNew, RebarAssembly)


public:
	ELLWallRebarAssemblyNew(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		WallRebarAssemblyNew(id, modelRef)
#ifdef PDMSIMPORT
		, pWall(NULL)
#endif
	{
		pEllWallDoubleRebarDlg = NULL;
		m_vecRebarStartEnd.clear();

		m_ELLWallData.dRadiusOut = 0.0;
		m_ELLWallData.dRadiusInn = 0.0;
		m_ELLWallData.dHeight = 0.0;
		m_ELLWallData.centerpt = DPoint3d::From(0, 0, 0);
		m_ELLWallData.ArcDPs[0] = DPoint3d::From(0, 0, 0);
		m_ELLWallData.ArcDPs[1] = DPoint3d::From(0, 0, 0);
	};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);

	void CalculateUseHoles(DgnModelRefP modelRef);

private:
	struct ELLWallGeometryInfo
	{
		double   dRadiusOut;	  // 外圆半径
		double   dRadiusInn;	  // 内圆半径
		double	 dHeight;		  // 高度
		DPoint3d centerpt;		  // 弧的中心点
		DPoint3d ArcDPs[2];		  // 弧起点和终点
	}m_ELLWallData;

	struct LevelInfo
	{
		int rebarLevel;
		double LevelSpacing;
	};

	struct TwinRebarDataTmp
	{
		int rebarNum;
		double spacing;
		double diameter;
	}m_reabrTwinData;

	vector<PIT::ArcSegment> m_vecArcSeg; // 预览弧形钢筋记录

#ifdef PDMSIMPORT
private:
	ELLWallComponent *pWall;
#endif
};
