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
	std::vector<EditElementHandle*> m_around_ele_holeEehs;//获取当前选择实体周围元素的孔洞
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

	void interPtsSort(vector<DPoint3d> &interPts, const DPoint3d &originPt);
	//获取板厚度
	double GetFloorThickness(EditElementHandleR Eleeh);
	bool get_value1(vector<ElementId> vec_walls, EditElementHandle& tmpeeh);
	void GetLeftRightWallFaces(WallRebarAssembly::WallData& walldata, EditElementHandleR eeh, string wallname);
	void GetUpDownFloorFaces(WallRebarAssembly::WallData& walldata, EditElementHandleR eeh);
	double GetDownFaceVecAndThickness(MSElementDescrP downFace, DPoint3d& Vec);
	void ExtendLineByFloor(vector<MSElementDescrP>& floorfaces, vector<IDandModelref>& floorRf, DPoint3d& ptstr, DPoint3d& ptend, DPoint3d& vecLine, double thick, double& Dimr, DPoint3d vecOutwall);
	double GetExtendptByWalls(DPoint3d& str, DPoint3d str_1, double thick, MSElementDescrP& Wallfaces, vector<MSElementDescrP>& cutWallfaces,
		DPlane3d plane, DPoint3d cpt, double Lae, double L0, DPoint3d& MGvec);
	bool CalculateBarLineDataByFloor(vector<MSElementDescrP>& floorfaces, vector<IDandModelref>& floorRf, DPoint3d& ptstr, DPoint3d& ptend, DPoint3d& vecLine, double thick,
		DPoint3d vecOutwall, bool& isInside, double& diameter);
	void GetCutPathLines(vector<WallRebarAssembly::BarLinesdata>& barlines, double sidespacing, double diameter,
		MSElementDescrP& path, vector<MSElementDescrP>& cutWallfaces, MSElementDescrP downface, double height);
	void GetMovePath(MSElementDescrP& pathline, double movedis, MSElementDescrP downface);
	void GeneratePathPoints(const DPoint3d& ptStr, const DPoint3d& ptEnd, int numPoints, vector<DPoint3d>& points);
	int CountPointsInElement(EditElementHandleP eeh, const vector<DPoint3d>& points);
	void ExtendLineString(MSElementDescrP& linedescr, double dis);
	MSElementDescrP GetLines(vector<DSegment3d>& lines);
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
	//获取当前实体z轴最大底面和z轴最小底面
	static MSElementDescrP GetElementDownFace(ElementHandleCR eeh, EditElementHandleR DownFace, double* tHeight);
	
	void ClearLines();
	//当前钢筋方向，0表示x轴，1表示z轴
	int m_nowvecDir;

	//当前层
	int m_nowlevel;

private:
	TieReBarInfo m_tieRebarInfo;
};