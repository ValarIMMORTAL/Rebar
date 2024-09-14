#pragma once
/*--------------------------------------------------------------------------------------+
|
|  $Source: sdk/example/RebarSDKExampleids.h $
|
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "../CommonId/AllCommonXAttribute.h"

#define RTYPE_XML    'XML'
#define RSCID_XML    1

#define RTYPE_GALLERY_SETTINGS 'GS'
#define RSCID_GALLERY_SETTINGS 2


#define TWIN_REBAR_LEVEL	L"TWIN_REBAR"
#define U_REBAR_LEVEL		L"U_REBAR"
#define STIRRUP_REBAR_LEVEL		L"STIRRUP_REBAR"

enum CmdNameRebarIds
    {
    CMDNAME_RebarPlace = 1,
	CMDNAME_InsertRebarTool,
	CMDNAME_RebarSDKReadRebar,
	CMDNAME_TieRebarFace
   };

/*----------------------------------------------------------------------+
|                                                                       |
|   String List IDS                                                     |
|                                                                       |
+----------------------------------------------------------------------*/
enum StringListRebarIds
    {
    STRINGLISTID_RebarSDKExampleCommandNames = 0,
    STRINGLISTID_RebarSDKExampleTextMessages
    };

/*----------------------------------------------------------------------+
|                                                                       |
|   Prompts                                                             |
|                                                                       |
+----------------------------------------------------------------------*/
enum PromptRebarIds
    {
    PROMPT_SELECT_SMARTSOLID_ELEMENT = 1,
    PROMPT_ACCEPTREJECT,
    PROMPT_AcceptOrReject,
    PROMPT_SelectSlabSolid,
    PROMPT_SelectRebarElement,
    PROMPT_SelectPierVaseElement,
    PROMPT_AcceptSelectionSet,
    PROMPT_EnterFirstPoint,
    PROMPT_EnterSecondPoint,
	PROMPT_SELECT_BASIS,
	PROMPT_SELECT_EXPAND,
	PROMPT_SELECT_1,
	PROMPT_SELECT_2
    };

enum ItemListRebarIds
    {
    ItemList_PlaceWallRebar = 1,
    ItemList_PlaceSlabRebar,
    };

enum StringListInsertIds
	{
	STRINGLISTID_Commands = 1,
	STRINGLISTID_Prompts
	};


struct SlabRebarInfo
    {
    double  topCover;
    double  bottomCover;
    double  sideCover;
    char    topXDirSize[512];
    double  topXDirSpacing;
    char    topYDirSize[512];
    double  topYDirSpacing;
    char    bottomXDirSize[512];
    double  bottomXDirSpacing;
    char    bottomYDirSize[512];
    double  bottomYDirSpacing;
    };

//钢筋材料表相关信息
struct RebarListInfo
{
	char m_strRebarOrder[512];//钢筋编号
	char m_strDiameter[512];//钢筋直径
	char m_strRebarGroupSize[512];//组数
	char m_strRebarSetNum[512];//每组根数
	char m_strRebarAllNum[512];//总根数
	char m_strRebarLength[512];//长度
	char m_strRebarHooktype[512];//弯钩类型
	char m_strRebarSharpeNum[512];//形状编码
};

//钢筋重量表相关信息
struct Rebarinfo
{
	char RebarGrade[512];//钢筋等级 例HPB300
	char AvgDamieter[20];//平均直径
	char BndRebarLength[20];//弯曲钢筋长度
	char StrRebarLength[20];//直线钢筋长度

	char BndRebarWeight[20];//弯曲钢筋重量
	char StrRebarWeight[20];//直线钢筋重量

	char AllLen[20];//所有直径的钢筋总长度
	char AllWei[20];//所有直径的钢筋总重量
};

// 钢筋重量表相关信息
// struct RebarWeightListInfo
// {
// 	int RebarDiameter;//钢筋直径
// 	double RebarAllLength;//某个直径的总长度
// 
// };


struct TwinBarSet
{
	struct TwinBarInfo
	{
		int		isTwinbars;			//是否为并筋
		int		isStaggered;		//是否交错
	}twinBarInfo;

	struct TwinBarLevelInfo
	{
		char    levelName[512];		//并筋层名称
		int		hasTwinbars;		//是否有并筋
		char    rebarSize[512];		//并筋尺寸
		int		rebarType;			//并筋型号
		int		interval;			//并筋间隔
	}twinbarlevelinfo;

};

struct TieReBarInfo
{
	int		tieRebarMethod;		//拉筋样式		0：无拉筋，1：直拉，2：斜拉
	char    rebarSize[512];		//钢筋尺寸
	int		rebarType;			//钢筋型号
	int		tieRebarStyle;		//拉筋布置方式	0：X*X;		1:X*2X;		2:2X*X;		3:2X*2X;	4:自定义样式
	int		isPatch;			//是否边部补齐拉筋
	int		rowInterval;		//自定义样式时的水平间隔
	int		colInterval;		//自定义样式时的竖直间隔
};

struct StairRebarInfo
{

	int		StairsStyle;		//楼梯样式		0：预制，1：线交
	char    rebarSize[512];		//钢筋尺寸
	int		rebarType;			//钢筋型号
	double		StairsCover;		//保护层

};

struct CustomizeRebarInfo
{

	int		rebarType;			//钢筋型号		
	char    rebarSize[512];		//钢筋尺寸
	char	rebarArrayDir[50];	//阵列方向
	int		rebarArrayNum;		//阵列数量
	double  rebarSpacing;		//钢筋间距
	char    rebarbsType[50];        //钢筋识类型
	char    rebarLevel[50];
};
struct WallSetInfo
{
	char    rebarSize[512];		//钢筋尺寸
	int		rebarType;			//钢筋型号
	double  spacing;			//钢筋间距
	double  uLenth;             //U型筋延申长度
};

struct CustomRebarl
{
	int     number;                     //钢筋序号
	double  Rebarlength;             //钢筋长度
	int     lengthtype;                //长度类型
};

struct CenterlineLength
{
	double Centerline;
};

struct InsertRebarInfo			// 插筋信息
{

	struct ColumnInfo
	{
		int				shape;			// 0: 方形   1: 圆形
		double			length;			// 长度
		double			width;			// 宽度
		double			heigth;			// 高度
		double			columeCover;	// 柱保护层

		char			rebarVerticalSize[512];		// 纵筋尺寸
		int				rebarVerticalType;			// 纵筋型号
		char			rebarHoopSize[512];			// 箍筋尺寸
		int				rebarHoopType;				// 箍筋型号

	}colInfo;

	struct RebarInfo // 插筋的信息
	{
		int					longNum;		// 长面数量
		int					shortNum;		// 短面数量
		char				rebarSize[512];	// 钢筋尺寸
		int					rebarType;		// 钢筋型号
		double				embedLength;	// 埋置长度
		double				expandLength;	// 拓展长度
			
		int					endType;		// 端部样式类型
		int					cornerType;		// 弯钩方向
		double				rotateAngle;	// 旋转角
	}rebarInfo;

	enum ConnectStyle	//连接方式
	{
		StaggerdJoint = 0,	//错开搭接
		MechanicalJoint,	//机械连接
	};

	struct WallInfo	// 主筋信息
	{
		int					staggeredStyle; // 交错类型
		int					wallTopType;	// 上下墙类型
		double				postiveCover;   // 板底部保护层
		double				slabDistance;   // 钢筋直线长度(板中的)
		double				slabThickness;  // 板的厚度
		double				embedLength;	// 埋置长度
		double				expandLength;	// 拓展长度
		double				rotateAngle;	// 旋转角
		int					endType;		// 端部样式类型
		char				rebarSize[512];	// 钢筋尺寸
		bool				isStaggered;    // 是否交错
		double				mainDiameter;	// 主筋中的直径
		int                 HookDirection;  //弯钩方向
		double				NormalSpace;
		double				AverageSpace;
		bool				isBack;			//钢筋类型是否是背面钢筋
		int					connectStyle;	//连接方式
	}wallInfo;

};
enum ReinForcingType//加强筋配置方式
{
	V1_ADD_H3 = 0,//水平1L+竖直3L
	V1_ADD_H4,//水平1L+竖直4L
	V2_ADD_H3,//水平2L+竖直3L
	V2_ADD_H4,//水平2L+竖直4L
	ALL,//水平1L+水平2L+竖直3L+竖直4L
};


struct ACCConcrete
{
	double  postiveOrTopCover;			//顶部或上面保护层（板为上面）
	double  reverseOrBottomCover;		//底部或下面保护层（板为下面）
	double  sideCover;					//侧面保护层
//	double  offset;						//钢筋偏移
};

struct StirrupData
{
	int		rebarNum;			//数量
	char    rebarSize[512];		//钢筋尺寸
	int		rebarType;			//钢筋型号
	double		rebarStartOffset;			//起点偏移
	double		rebarEndOffset;			//终点偏移
	double      tranLenth;//延申长度
};

struct Abanurus_PTRebarData
{
	int ptHNum;//点筋横向数量
	int ptVNum;//点筋纵向数量
	char ptrebarSize[512];//点筋尺寸
	int ptrebarType;//点筋等级
	char stirrupRebarsize[512];//箍筋尺寸
	int stirrupRebarType;//箍筋等级
	//int tieNum;//拉筋数量
	//char tierebarSize[512];//拉筋尺寸
	//int tierebarType;//拉筋等级

};



struct BeamRebarInfo
{
	struct BeamBaseData
	{
		double			dWidth;
		double			dDepth;
		double			dLeftSide;
		double			dRightSide;
		double			dLeftXLen;
		double			dRightXLen;
		double			dNetSpan;	 // 净跨度
		double			dAxisToAxis; // 轴线到轴线
	}beamBaseData;

	struct BeamDefaultData
	{
		double			dTop;
		double			dLeft;
		double			dRight;
		double			dUnder;
		double			dFloor;
		double			dMargin;
		int				nRebarCal; // 钢筋计算

	}beamDefaultData; // 梁默认值

	struct BeamAreaVertical
	{
		char			label[512];			// 标签
		double			dSpace;				// 间隙
		double			dStartOffset;		// 起点偏移
		int				nTotNum;			// 总数量
		double			dEndOffset;			// 终点偏移
		int				nPosition;			// 位置
	}beamAreaVertical; // 区域数据

	struct BeamRebarVertical
	{
		char			label[512];			// 标签	
		double			dLeftOffset;		// 左端偏移
		double			dRightOffset;		// 右端偏移
		double			dLeftRotateAngle;	// 左端旋转角
		double			dRightRotateAngle;  // 右端旋转角
		int				nPosition;			// 位置
		int				nTotNum;			// 总数量
		int				nLeftEndStyle;		// 左端样式
		int				nRightEndStyle;		// 右端样式
		char			rebarSize[512];		// 钢筋尺寸
		int				rebarType;			// 钢筋类型
	}beamRebarVertical;

	struct BeamCommHoop
	{
		char					label[512];		// 标签
		double					dSpacing;		// 间距
		char					rebarSize[512]; // 钢筋尺寸
		int						rebarType;		// 钢筋类型
		double					dStartPos;		// 始端
		double					dEnd_N_Deep;	// 通过N*深度
		int						nPostion;		// 位置
		double					dEndPos;		// 终端
		double					dStart_N_Deep;  // 通过N*深度
	}beamCommHoop;

	struct BeamRebarHoop
	{
		char					label[512];	// 标签
		double					dOffset;
		double					dStartRotate;
		double					dEndRotate;
		int						nStartEndType; // 起点端部类型
		int						nFnishEndType; // 起点端部类型
	}beamRebarHoop;

};
struct FaceTieReBarInfo
{
	int		tieRebarMethod;		//拉筋样式		0：对扣拉筋，1：单根对扣
	char    rebarSize[512];		//钢筋尺寸
	int		rebarType;			//钢筋型号
	int		tieRebarStyle;		//拉筋布置方式	0：200*400;		1:400*400;	
	//int		isPatch;			//是否边部补齐拉筋
	int firstAngle;
	int secondAngle;
	int		rowInterval;
	int		colInterval;
};