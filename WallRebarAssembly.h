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
#include "CommonFile.h"
#include "PITRebarEndType.h"
#include "PITRebarCurve.h"
#include "PITArcSegment.h"
#include "PITRebarAssembly.h"

#define  MaxWallThickness     600          //折线墙最大处理墙厚处理

class CWallRebarDlg;
class WallRebarAssembly : public PIT::PITRebarAssembly
{
public:
	enum WallType
	{
		STWALL,//直墙
		GWALL, //折线墙
		ARCWALL,//弧线墙
		ELLIPSEWall,//圆形墙
		STGWALL, //不等厚直墙
		Other
	};

	enum  ElementType
	{
		WALL,
		FLOOR,
		EOther
	};

	//墙数据
	struct WallData
	{
		MSElementDescrP downFace = nullptr;//墙底面
		vector<DSegment3d> vecFontLine;//外侧线
		vector<DSegment3d> vecBackLine;//内侧线
		DVec3d vecdown = DVec3d::From(0,0,1);//底部面法相
		double height = 0;//高度
		double thickness = 0;//厚度
		vector<MSElementDescrP> upfloorfaces;//墙上顶板底面
		vector<MSElementDescrP> downfloorfaces;//墙下顶板底面
		vector<IDandModelref> upfloorID;//墙上顶板ID与ref
		vector<IDandModelref> downfloorID;//墙下顶板ID与ref
		vector<IDandModelref> wallID;//周边锚固的墙ID与ref
		vector<IDandModelref> floorID;//周边锚固的墙ID与ref
		double upfloorth = 0;//顶板厚度
		double downfloorth = 0;//底板厚度
		vector<MSElementDescrP> cutWallfaces;//与当前墙体同层有关联的墙体底面
		DPoint3d vecToWall = DVec3d::From(0, 0, 0);//正面朝墙内的方向
		void ClearData()
		{
			for(int i = 0;i<upfloorfaces.size();i++)
			{
				mdlElmdscr_freeAll(&upfloorfaces.at(i));
			}
			upfloorfaces.clear();
			for (int i = 0; i < downfloorfaces.size(); i++)
			{
				mdlElmdscr_freeAll(&downfloorfaces.at(i));
			}
			downfloorfaces.clear();
			for (int i = 0; i < cutWallfaces.size(); i++)
			{
				mdlElmdscr_freeAll(&cutWallfaces.at(i));
			}
			cutWallfaces.clear();
			mdlElmdscr_freeAll(&downFace);
		}
	}m_walldata;

	//配筋线数据（每一层钢筋对应一个这样的数据）,里面所有数据为已经乘过uor_per_mm的值
	struct BarLinesdata
	{
		MSElementDescrP path = nullptr;//路径线串
		MSElementDescrP barline = nullptr;//基础钢筋线串
		double spacing = 0;//钢筋间距
		double strDis = 0;//起始保护层，（设置侧面保护层+起始偏移量+钢筋直径/2）
		double endDis = 0;//终止保护层，（设置侧面保护层+终止偏移量+钢筋直径/2）
		double diameter = 0;//钢筋直径
		double extendstrDis = 0;//线起始端延长值（如果是竖向钢筋，为下层板厚度；如果是横向钢筋为起始点墙厚度）
		double extendendDis = 0;//线终止端延长值（如果是竖向钢筋，为上层板厚度；如果是横向钢筋为终止点墙厚度）
		double extenddiameter = 0;//顶板1L钢筋直径
		double extstrdiameter = 0;//底板1L钢筋直径
		bool isInSide = false;//是否内侧面
		DPoint3d vecstr = DPoint3d::From(0, 0, 0);//起始弯钩方向，没有弯钩的就是0，0，0
		DPoint3d vecend = DPoint3d::From(0, 0, 0);//终止弯钩方向，没有弯钩的就是0，0，0
		DPoint3d vecHoleehs = DPoint3d::From(0, 0, 0);//如果出现孔洞则孔洞锚入的方向
		double strMG = 0;//起点锚固长度
		double endMG = 0;//终点锚固长度
		double holMG = 0;//孔洞锚固长度
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
		BE_DATA_VALUE(vector<double>, vecAngle)			// 角度
		BE_DATA_VALUE(vector<int>, vecRebarLineStyle)		//钢筋线形
		BE_DATA_VALUE(vector<int>, vecRebarWeight)			//钢筋线宽

		BE_DATA_VALUE(vector<PIT::LapOptions>, vecLapOptions)			//搭接选项
		BE_DATA_VALUE(vector<vector<PIT::EndType> >, vvecEndType)		//端部样式
		BE_DATA_VALUE(vector<TwinBarSet::TwinBarLevelInfo >, vecTwinRebarLevel)	//并筋层
		BE_DATA_VALUE(vector<ElementId>, vecSetId)								//SetId
		BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)     //前面线的所有点

public:
	WallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~WallRebarAssembly() {};
	double m_width;
public:
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//筛选后的孔洞
	std::map<EditElementHandle*, EditElementHandle*> m_doorsholes;//所有门洞，及门洞上的负实体
	std::vector<DPoint3d>     m_vecRebarPtsLayer;//被孔洞截取前的所有钢筋点数据
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//被孔洞截取前的所有并筋点数据
	std::vector<DPoint3d>     m_vecTieRebarPtsLayer; //拉筋生成所需要的钢筋点数据

public:
	vector<vector<vector<DPoint3d>> > m_vecRebarStartEnd;	//规避了孔洞的所有点

	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;//未规避孔洞的所有点
	PIT::PITRebarEndType Hol;
	vector<ElementRefP> m_allLines;//预览按钮按下后的所有钢筋线
	double lae = 0;
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
	//当前钢筋方向，0表示x轴，1表示z轴
	int m_nowvecDir;

	//当前层
	int m_nowlevel;

private:
	TieReBarInfo m_tieRebarInfo;
};


class STWallRebarAssembly : public WallRebarAssembly
{
	BE_DATA_VALUE(STWallGeometryInfo, STwallData)			//STWall几何数据
//	BE_DATA_VALUE(bool, IsTwinrebar)			//在画rebarcuve时，判断是否为并筋层
public :

	CVector3D m_VecX = CVector3D::From(1, 0, 0);//局部坐标系下的X方向
	CVector3D m_VecZ = CVector3D::From(0, 0, 1);//局部坐标系下的Z方向

	DPoint3d m_LineNormal;
	CWallRebarDlg *pWallDoubleRebarDlg;
	double m_angle_left;
	double m_angle_right;
	virtual ~STWallRebarAssembly()
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
		double endOffset, PIT::PITRebarEndTypes &endType, CMatrix3D const& mat,bool istwin = false);
	bool makeRebarCurve_Laption(RebarCurve& rebar, double xPos, double height, double bendRadius,
		double bendLen, RebarEndTypes const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool istwin = false);
	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag=true);
	bool makaRebarCurve(const vector<DPoint3d>& linePts, double extendStrDis, double extendEndDis, double diameter, double strMoveDis, double endMoveDis, bool isInSide,
	                    const PIT::PITRebarEndTypes& endTypes, vector<PIT::PITRebarCurve>& rebars,
	                    std::vector<EditElementHandle*> upflooreehs, std::vector<EditElementHandle*> downflooreehs, std::vector<EditElementHandle*> Walleehs,
						std::vector<EditElementHandle*>alleehs);


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
		CMatrix3D const& mat, const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel,int level, int grade, int DataExchange, DgnModelRefP modelRef, int rebarLineStyle,
		int rebarWeight);
//	RebarSetTag* MakeRebars_ACC(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat, const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel, DgnModelRefP modelRef);
	RebarSetTag* MakeRebars_Laption(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, PIT::LapOptions const& lapOptions, vector<PIT::EndType> const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

	RebarSetTag* MakeRebars(ElementId& rebarSetId, BrStringCR sizeKey, const vector<BarLinesdata>& barLinesData, 
		double strOffset, double endOffset, int level, int grade, DgnModelRefP modelRef, int rebarLineStyle, int rebarWeight);

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(STWallRebarAssembly, RebarAssembly)


public:
	STWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		WallRebarAssembly(id, modelRef)
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

	void CalculateBarLinesData(map<int, vector<BarLinesdata>> &barlines, DgnModelRefP modelRef);

	/*
	* @desc:		根据保护层和移动距离重新计算基础钢筋线串
	* @param[in/out]	data 配筋线数据
	* @return	MSElementDescrP 新的钢筋线串
	* @author	Hong ZhuoHui
	* @Date:	2023/09/13
	*/
	void ReCalBarLineByCoverAndDis(BarLinesdata& data);

	/*
	* @desc:		根据实体求差算孔洞（Z型墙）
	* @param[in]	wallEeh 墙
	* @param[out]	holes 孔洞
	* @author	Hong ZhuoHui
	* @Date:	2023/09/13
	*/
	void CalHolesBySubtract(EditElementHandleCR wallEeh, std::vector<EditElementHandle*>& holes);

	/*
	* @desc:		计算endType
	* @param[in]	data 钢筋线数据
	* @param[in]	sizeKey
	* @param[out]	pitRebarEndTypes 端部样式
	* @param[in]	modelRef
	* @author	Hong ZhuoHui
	* @Date:	2023/09/19
	*/
	void CalRebarEndTypes(const BarLinesdata& data, BrStringCR sizeKey,
		PIT::PITRebarEndTypes& pitRebarEndTypes, DgnModelRefP modelRef);

	/*
	* @desc:		根据顶底板重新计算伸缩距离
	* @param[in]	strPt 钢筋线起点
	* @param[in]	endPt 钢筋线重点
	* @param[in]	strMoveLen 起点端移动距离，不带弯钩是保护层，带弯钩是保护层+钢筋半径
	* @param[in]	endMoveLen 终点端移动距离，不带弯钩是保护层，带弯钩是保护层+钢筋半径
	* @param[out]	extendStrDis 起点伸缩距离
	* @param[out]	extendEndDis 终点伸缩距离
	* @author	Hong ZhuoHui
	* @Date:	2023/09/20
	*/
	void ReCalExtendDisByTopDownFloor(const DPoint3d& strPt, const DPoint3d& endPt, double strMoveLen, double endMoveLen,
		double& extendStrDis, double& extendEndDis, bool isInSide);

	void CalculateLeftRightBarLines(vector<BarLinesdata>& barlines, double fdiam, double allfdiam, DPoint3dR vecHoleehs, MSElementDescrP& path, MSElementDescrP& barline,
		int side, int index);
	double get_lae() const;

	void CalculateUpDownBarLines(vector<BarLinesdata>& barlines, double fdiam, double allfdiam, DPoint3dR vecHoleehs, MSElementDescrP& path, MSElementDescrP& barline,
	                             int side, int index);

#ifdef PDMSIMPORT
private:	
	STWallComponent *pWall;
#endif
};


class GWallRebarAssembly : public WallRebarAssembly
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
	~GWallRebarAssembly()
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

public:
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

	virtual void CalculateUseHoles(DgnModelRefP modelRef);
	bool GetUcsAndStartEndData(int index, double thickness,DgnModelRefP modelRef,bool isSTGWALL = false);
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
	BE_DECLARE_VMS(GWallRebarAssembly, RebarAssembly)

public:
public:
	GWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) 
		:WallRebarAssembly(id, modelRef)
#ifdef PDMSIMPORT
		, pWall(NULL)
#endif
	{};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);

	void SetLinePts(const map<int, vector<DPoint3d>> &pts) { m_vecLinePts = pts; }
	map<int, vector<DPoint3d>> GetLinePts() const { return m_vecLinePts; }
	void SetIsPushTieRebar(bool bl) { m_isPushTieRebar = bl; }
	bool GetIsPushTieRebar() const { return m_isPushTieRebar; }
	void SetGWallHeight(double height) { m_GWallData.height = height; }
	double GetGWallHeight() const { return m_GWallData.height; }

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

class ArcWallRebarAssembly : public WallRebarAssembly
{
public:
	CWallRebarDlg *pArcWallDoubleRebarDlg;
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

	//获取新的弧线和钢筋间距
	void GetNewArcAndSpacing(PIT::ArcSegment oldArc, PIT::ArcSegment& newArc, double angle, double& newSpacing);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::ArcWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);
	


public:
	virtual bool	AnalyzingWallGeometricDataARC(ElementHandleCR eh ,PIT::ArcSegment &arcFront, PIT::ArcSegment &arcBack);
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(ArcWallRebarAssembly, RebarAssembly)

public:
	ArcWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL)
		:WallRebarAssembly(id, modelRef)
	{
		pArcWallDoubleRebarDlg = nullptr;
		memset(&m_ArcWallData, 0, sizeof(ArcWallGeometryInfo));
	};

	~ArcWallRebarAssembly()
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
	PIT::ArcSegment m_outMaxArc;	//外弧最大弧形
	double m_sideCoverAngle = 0;	//保护层角度
};

class ELLWallRebarAssembly : public WallRebarAssembly
{
public:
	CWallRebarDlg *pEllWallDoubleRebarDlg;
	virtual ~ELLWallRebarAssembly()
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

	bool makeBreakArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP mscArc, PIT::PITRebarEndTypes& endTypes, const vector<DPoint3d>& pts);

	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d ptStr, double dRebarLength);

	bool makeRoundRebarCurve(vector<PIT::PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d	centerPoint, double dRoundRadius);

	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

	vector<CurveVectorPtr> CreateBreakArcRange(const Transform& tran);

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
	BE_DECLARE_VMS(ELLWallRebarAssembly, RebarAssembly)


public:
	ELLWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		WallRebarAssembly(id, modelRef)
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

	void SetBreakAngelData(vector<PIT::BreakAngleData> data) { m_vecBreakData = data; }

private:
	struct ELLWallGeometryInfo
	{
		double   dRadiusOut;	  // 外圆半径
		double   dRadiusInn;	  // 内圆半径
		double	 dHeight;		  // 高度
		DPoint3d centerpt;		  // 弧的中心点
		DPoint3d ArcDPs[2];		  // 弧起点和终点
		UInt16       type;         //元素类型

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
	vector<PIT::BreakAngleData> m_vecBreakData;	//断开数据
	map<int, vector<EditElementHandle*>> m_vecUseHoles; //按角度的孔洞

#ifdef PDMSIMPORT
private:
	ELLWallComponent *pWall;
#endif
};

/*
* ClassName:	直的折现墙配筋（合并直墙）
* Description:	
* Author:		hzh
* Date:			2022/11/08*/
class STGWallRebarAssembly : public GWallRebarAssembly
{
public:
	//当前墙面与相邻墙面的位置关系
	enum WallPos
	{
		Horizontal = 0, //平行
		IN_WALL,		//内凹
		OUT_WALL		//外凸
	};
	STGWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL)
		:GWallRebarAssembly(id, modelRef) {
		pSTGWallDoubleRebarDlg = NULL;
		m_vecRebarStartEnd.clear();
	}

protected:
	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);

	virtual void CalculateUseHoles(DgnModelRefP modelRef);

public:
	virtual bool	SetWallData(ElementHandleCR eh);
	virtual bool	MakeRebars(DgnModelRefP modelRef);
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(STGWallRebarAssembly, GWallRebarAssembly)
private:
	/*
	* @desc:	获取墙的前后地面线段
	* @param[in]	eeh	墙
	* @param[out]	frontLines 前底面线段
	* @param[out]	backLines 后底面线段
	* @return	成功true，失败false
	* @author	hzh
	* @Date:	2022/11/08
	*/
	bool GetFrontBackLines(EditElementHandleCR eeh, vector<MSElementDescrP>& frontLines, vector<MSElementDescrP>& backLines);
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
		bool  drawlast = true,
		bool  isHoriRebar = true,
		WallPos leftWall = Horizontal,
		WallPos rightWall = Horizontal,
		double leftDis = 0,
		double rightDis = 0
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
		bool isTwin = false,
		WallPos leftWall = Horizontal,
		WallPos rightWall = Horizontal,
		double leftDis = 0,
		double rightDis = 0,
		double bendLen = 0,
		double  rebarDia = 0
	);

	/*
	* @desc:	初始化平行横筋信息	
	* @author	hzh
	* @Date:	2022/11/08
	*/
	void InitLevelHoriInfos();

	/*
	* @desc:	计算平行横筋信息
	* @param[in]	level 横筋所在层	
	* @param[in]	tag 横筋的rebarsettag
	* @param[in]	rightWall 该层与下一面墙的关系
	* @param[out]	levelHoriTags 按平行关系分组的钢筋层
	* @remark	将横筋按是否平行分组，即每一层有若干组，每一组有若干平行的横筋，它们可以合并
	* @author	hzh
	* @Date:	2022/11/08
	*/
	void CalcLevelHoriInfos(int level, RebarSetTag* tag, WallPos rightWall,
		map<int, vector<vector<RebarSetTag*>>>& levelHoriTags);

	/*
	* @desc:	修改横筋
	* @param[in]	levelHoriTags 	按平行关系分组的钢筋层信息
	* @param[in]	levelName 层名，主筋或者并筋	
	* @remark	将可合并的横筋进行合并处理
	* @author	hzh
	* @Date:	2022/11/08
	*/
	void UpdateHoriRebars(const map<int, vector<vector<RebarSetTag*>>>& levelHoriTags, const CString& levelName);
	
	/*
	* @desc:	修改钢筋
	* @param[in]	zRebars 根据z分类的横筋，即可合并的钢筋	
	* @param[in]	levelName 层名，主筋或者并筋	
	* @author	hzh
	* @Date:	2022/11/08
	*/
	void UpdateRebars(const map<int, vector<ElementRefP>>& zRebars, const CString& levelName);
	
	/*
	* @desc:	计算钢筋规避孔洞后的端点
	* @param[in]	strPt 钢筋原起始点
	* @param[in]	endPt 钢筋原终点
	* @return	map<int, DPoint3d> 规避孔洞后的店，两两为端点
	* @author	hzh
	* @Date:	2022/11/08
	*/
	map<int, DPoint3d> CalcRebarPts(DPoint3d& strPt, DPoint3d& endPt);

	/*
	* @desc:	计算墙与分段之间的关系，即将每一段与墙绑定起来
	* @author	hzh
	* @Date:	2022/11/08
	*/
	void CalcWallsInRange();

protected:
	virtual bool        OnDoubleClick() override;

public:
	CWallRebarDlg *pSTGWallDoubleRebarDlg;
private:
	struct LinePt
	{
		DPoint3d startPt;
		DPoint3d endPt;
	};
	vector<LinePt> m_frontLinePts;//底面前面线段的点
	vector<LinePt> m_backLinePts;//底面后面线段的点
	map<int, bool> m_levelIsHori;	//钢筋层是否平行
	map<int, vector<vector<RebarSetTag*>>> m_levelHoriTags; //钢筋层按平行分组
	map<int, vector<vector<RebarSetTag*>>> m_twinLevelHoriTags; //并筋钢筋层按平行分组
	map<int, vector<int>> m_rangeIdxWalls;	//区域和墙
};