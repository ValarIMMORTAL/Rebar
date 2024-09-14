#pragma once
#include "_USTATION.h"
#include "Public.h"
#include "LEVELFILE.h"
#include "GalleryIntelligentRebarids.h"
#include "RebarCommonData.h"
#include "XmlManager.h"
namespace PIT
{
	enum PITRebarAssemblyEnum
	{
		Wall = 2,
		STWall,
		GWall,
		ArcWall,
		ELLWall,
		PITSTWall,
		PITGWall,
		PTIArcWall,
		Face,
		Plane,
		CamberedSurface,
		Parapet,
		Rainshed,
	};
}

struct ArcRebar //放射状板配筋的弧线信息
{
	DPoint3d centerpt;
	DPoint3d ptStart;
	DPoint3d ptMid;	//中间的点
	DPoint3d ptEnd;
	double radius;
	double ArcLen;
	double slabHight;
	bool isCircle;	//是否是圆
};

struct RebarPoint
{
	RebarPoint()
	{
		Layer = 0;
		vecDir = 0;
		sec = 0;
		ptstr = ptend = ptmid = DPoint3d::From(0, 0, 0);
	}
	int Layer; // 那个面第几层 如： 正面 第一层，正面第二层 这种
	int vecDir;//0表示X轴，1表示Z轴
	int sec;//第几段，从0开始，默认都是第一段
	int DataExchange;

	int iIndex; // 配置钢筋信息时的序号
	DPoint3d ptstr;
	DPoint3d ptend;
	DPoint3d ptmid;
};

struct CutRebarInfo
{
	bool isCutRebar;
	double dCutLength1;
	double dCutLength2;
	double dCutLength3;
};

struct InsertRebarPoint
{
	InsertRebarPoint()
	{
		ptstr = DPoint3d::From(0, 0, 0);
		ptmid = DPoint3d::From(0, 0, 0);
		ptend = DPoint3d::From(0, 0, 0);
		memset(sizeKey, 0, sizeof(sizeKey));
		isMid = false;
		memset(SelectedRebarType, 0, sizeof(SelectedRebarType));
		memset(level, 0, sizeof(level));
		memset(grade, 0, sizeof(grade));
		mid = 0;
	}
	ElementId mid;
	ElementId Rsid;//所属钢筋组
	DPoint3d ptstr;
	DPoint3d ptmid;
	DPoint3d ptend;
	char sizeKey[128];
	bool isMid;
	char SelectedRebarType[128];//钢筋的类型属性
	char level[128];//钢筋层号
	char grade[128];//钢筋等级
};


struct RebarLine
{
	std::string cellName;
	std::string groupName;
	DPoint3d ptStr;
	DPoint3d ptEnd;
	double rebarDitr;
};

struct CNCutRebarInfo
{
	int nIndex;			// 下标，标记是第几段

	double dLength;		// 长度
	int nVecType;		// 截断起始方向  0 ： 正向 从钢筋起点开始 1 ： 从钢筋终点开始
};


struct STWallGeometryInfo
{
	DPoint3d ptPreStr;
	DPoint3d ptPreEnd;
	DPoint3d ptStart;
	DPoint3d ptEnd;
	DPoint3d ptBckStr;
	DPoint3d ptBckEnd;
	double length;
	double width;
	double height;

	STWallGeometryInfo()
	{
		ptStart = { 0,0,0 };
		ptEnd = { 0,0,0 };
		ptStart = { 0,0,0 };
		ptEnd = { 0,0,0 };
		ptBckStr = { 0,0,0 };
		ptBckEnd = { 0,0,0 };
		length = 0;
		width = 0;
		height = 0;
	}
};

namespace PIT
{
	struct Concrete
	{
		double  postiveCover;		//顶部保护层
		double  reverseCover;		//底部保护层
		double  sideCover;			//侧面保护层
		int		rebarLevelNum;		//钢筋层数
		int     isHandleHole;       //是否规避孔洞
		double  MissHoleSize;
		int		isFaceUnionRebar;	//是否面联合配筋
		int		isSlabUpFaceUnionRebar;	//是否板顶面联合配筋
		int		m_SlabRebarMethod;//板的配筋方式
	};

	struct ConcreteRebar
	{
		int		rebarLevel;			//钢筋层
		int		rebarDir;			//方向			0：横向钢筋，1：竖向钢筋
		char    rebarSize[512];		//钢筋尺寸
		int		rebarType;			//钢筋型号
		double  spacing;			//钢筋间距
		double  startOffset;		//起点偏移
		double  endOffset;			//终点偏移
		double  levelSpace;			//钢筋层间隔
		int		datachange;			//数据交换
		double	angle;				//径向角度
		//int		rebarColor;			//钢筋颜色
		int		rebarLineStyle;		//钢筋线型
		int		rebarWeight;		//钢筋线宽
	};

	struct LapOptions
	{
		int		rebarLevel;			//钢筋层
		int		connectMethod;		//连接方式
		double  lapLength;			//搭接长度
		double  stockLength;		//库存长度
		double  millLength;			//加工长度
		int		isStaggered;		//是否交错
		double  staggeredLength;	//交错长度
		double  udLength;	        //女儿墙中U形筋的延申长度	
	};
	struct EndType
	{
		int		endType;			//类型:0,无;1,弯钩；2，吊钩；3，折线；
		                              //4，90度弯钩；5，135度弯钩；6，180度弯钩；7，直锚
		double  offset;				//偏移
		double  rotateAngle;		//旋转角

		//不同端部类型数据不一样
		struct RebarEndPointInfo
		{
			double value1 = 0.00;//弯曲半径
			double value2 = 0.00;//旋转角度
			double value3 = 0.00;//锚入长度
			double value4 = 0.00;
			double value5 = 0.00;
			double value6 = 0.00;
			double value7 = 0.00;
			double value8 = 0.00;
			double value9 = 0.00;
		}endPtInfo;
	};

	struct AssociatedComponent
	{
		char    CurrentWallName[512];			//待配筋墙名称
		char	associatedComponentName[512];	//关联构件
		int		mutualRelation;					//相互关系		0：同层墙，1：上层墙，2:下层墙，3：上层板，4：下层板
	//	int		rebarLevel;						//待配钢筋层
		int		associatedRelation;				//关联关系		0：忽略，1：锚入，2：被锚入
		int		anchoringMethod;				//锚固连接方式	0-9种锚固连接方式
		int		isEndBothACC;					//是否为端部关联构件
		double	endOffset;						//端部偏移				//锚固样式1与锚固样式2中使用
		int		acPositon;						//关联构件位置 0：起点	1:终点	2：中间	//锚固样式2中使用
		double	startL0;						//起点端部预留长度		//
		double	endL0;							//终点端部预留长度		//
		double	La;								//上墙变宽侧终点端部预留长度		//锚固样式9中使用
		int		isReverse;						//是否反转，反转后纵向钢筋第一层与最后一层钢筋直锚长度互换		//锚固样式9中使用
		int		isCut;							//是否切断主墙			//锚固样式4中使用
	};

	struct WallRebarInfo			//墙的每一层的钢筋信息
	{
		Concrete concrete;

		ConcreteRebar rebar;

		LapOptions lapoption;

		EndType endtype;

		AssociatedComponent associatedcomponent;
	};

	struct DomeLevelInfo
	{
		int		   nNumber;			// 序号
		double     dRadius1;		// 起始配筋半径
		double	   dRadius2;		// 结束配筋半径
		int        nLayoutType;     // 钢筋布置方式 0 : XY正交  1 : 环径正交
	};

	struct DomeCoverInfo
	{
		double		dCover;		// 穹顶保护层
		ElementId	eehId;		// 穹顶被代替的实体id
	};

	struct DomeLevelDetailInfo // 每层钢筋的信息
	{
		int		nLevel;			// 层数
		int		nNumber;		// 对应  DomeLevelInfo 中的序号

		int		rebarShape;     // 钢筋形状   环径正交 --  0 : 环向  1 : 径向 2    XY正交 --  0 : X向  1 : Y向
		char	rebarSize[512]; // 钢筋直径
		int		rebarType;		// 钢筋类型
		double  dAngleOrSpace;	// 角度 / 间距
		double  dSpacing;		// 与前层间距
		double  dStartOffset;   // 起始偏移(弧形)
	};

	struct RebarData
	{
		BrString rebarSize;
		BrString rebarType;
		RebarSymbology rebarSymb;
		char SelectedRebarType[512];
		char SelectedRebarLevel[512];
		char SelectedRebarGrade[512];
		double SelectedSpacing;
	};
	BE_DEFINE_SUFFIX_TYPEDEFS(RebarData)

	//环墙断开角度数据
	struct BreakAngleData
	{
		double beginAngle;	//开始角度
		double endAngle;	//结束角度
	};

	struct DlgData//界面上的数据
	{
		double  postiveCover;			//正面保护层
		double  sideCover;				//侧面保护层 
		double  reverseCover;			//背面保护层
		double  missHoleSize;			//忽略尺寸
		int		rebarLevelNum;			//钢筋层数
	};
};


extern std::map<std::string, IDandModelref> g_mapidAndmodel;
extern PIT::WallRebarInfo g_wallRebarInfo;
extern ElementHandle g_SelectedElm;
extern ElementId g_ConcreteId;
extern TwinBarSet::TwinBarInfo g_twinBarInfo;
extern TieReBarInfo	g_tieRebarInfo;
extern std::vector<PIT::ConcreteRebar>		g_vecRebarData;
extern std::vector<PIT::EndType>							g_vecEndTypeData;
extern std::vector<TwinBarSet::TwinBarLevelInfo>	g_vecTwinBarData;
extern std::vector<PIT::AssociatedComponent>	g_vecACData;
extern std::vector<PIT::LapOptions>	g_vecLapOptionData;
extern std::vector<RebarPoint>                   g_vecRebarPtsNoHole;//孔洞切断之前的钢筋起始点
extern std::vector<RebarPoint>                   g_vecTwinRebarPtsNoHole;//孔洞切断之前的并筋起始点
extern std::vector<RebarPoint>					 g_vecTieRebarPtsNoHole; //孔洞切断之前的拉筋起始点

extern ElementHandle g_InsertElm;	// 插筋基础
extern ElementHandle g_ColumnElm;	// 插筋柱实体

extern InsertRebarInfo 			g_InsertRebarInfo;	// 插筋相关信息
extern std::vector<InsertRebarInfo::WallInfo> g_vecWallInsertInfo; // 墙插筋相关信息
extern int CoverType;
extern std::list<CString> g_listRebarType;
extern std::list<CString> g_listRebarType2;

bool GetConcreteXAttribute(ElementId elmID, DgnModelRefP pModel);

bool SetConcreteXAttribute(ElementId elmID, DgnModelRefP pModel);


/*
* @desc:	条件满足输出信息
* @param[in]	condition 条件函数
* @param[in]	outStr 输出信息
* @return	符合条件返回false并输出信息，不符合返回true
* @author	hzh
* @Date:	2022/11/22
*/
bool IsValidAndPromout(std::function<bool()> condition, WString outStr);

/*
* @desc:	获取选择元素集合
* @param[out]	selectset 选择元素集
* @param[in]	outStr 输出信息
* @return	成功true，失败false
* @author	hzh
* @Date:	2022/11/22
*/
bool GetSelectAgenda(ElementAgendaR selectset, WString outStr);