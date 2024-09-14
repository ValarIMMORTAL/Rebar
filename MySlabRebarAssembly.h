#pragma once
#pragma once

/*--------------------------------------------------------------------------------------+
|
|     $Source: SlabRebarAssembly.h $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/

#include <RebarElements.h>
//#include "MySlabRebarComponent.h"
#include "CommonFile.h"
#include "PITRebarCurve.h"

enum SlabType
{
	STSLAB,
	GSLAB,
	SlabTypeOther
};


class CWallMainRebarDlg;
class	SelectLineDirTool : public DgnElementSetTool
{
public:
	~SelectLineDirTool()
	{
		auto& ssm = SelectionSetManager::GetManager();
		ssm.EmptyAll();
		mdlSelect_freeAll();
	}

	static void InstallNewInstance(int toolId,	CWallMainRebarDlg* Ptr);

protected:

	SelectLineDirTool(int toolId) :DgnElementSetTool(toolId) { }
	virtual bool            _IsModifyOriginal() override { return false; }

	StatusInt _OnElementModify(EditElementHandleR el) override;
	void _OnRestartTool() override;

	virtual void _SetupAndPromptForNextAction() override;

	virtual bool _WantAdditionalLocate(DgnButtonEventCP ev) override;
	virtual bool _OnModifyComplete(DgnButtonEventCR ev) override;

	virtual UsesSelection _AllowSelection() override
	{
		return USES_SS_Check;
	}

	virtual bool _DoGroups() override;

	virtual bool _WantDynamics() override;
	virtual bool _OnDataButton(DgnButtonEventCR ev)override;

	virtual bool _OnModifierKeyTransition(bool wentDown, int key) override;

	virtual bool _OnPostLocate(HitPathCP path, WStringR cantAcceptReason) override;

	bool _OnResetButton(DgnButtonEventCR ev) override;

	virtual EditElementHandleP _BuildLocateAgenda(HitPathCP path, DgnButtonEventCP ev) override;

	virtual UsesDragSelect  _AllowDragSelect() override { return USES_DRAGSELECT_Box; }
	virtual bool            _NeedAcceptPoint() override { return SOURCE_Pick == _GetElemSource(); }
	virtual size_t          _GetAdditionalLocateNumRequired() override { return 0; }

private:
	static CWallMainRebarDlg * m_Ptr;
};

using namespace PIT;
class MySlabRebarAssembly : public RebarAssembly                   //墙配筋的钢筋信息
{
public:
		BE_DATA_REFER(BeMatrix, Placement)              //当前局部坐标原点
		BE_DATA_VALUE(bool, bACCRebar)				//是否关联构件配筋
		BE_DATA_VALUE(UINT, ACCRebarMethod)			//关联构件配筋方式
		BE_DATA_VALUE(double, PositiveCover)			//正面保护层
		BE_DATA_VALUE(double, ReverseCover)			//反面保护层
		BE_DATA_VALUE(double, SideCover)				//侧面保护层
		BE_DATA_VALUE(int, RebarLevelNum)			//钢筋层数
//		BE_DATA_VALUE(char*, SlabRebarMethod)			//配筋方式
//		BE_DATA_VALUE(int, Twinbars)				//是否使用并筋
//		BE_DATA_VALUE(int, IsStaggered)			//是否交错
		BE_DATA_VALUE(vector<int>, vecDir)					//方向,0表示x轴，1表示z轴
		BE_DATA_VALUE(vector<BrString>, vecDirSize)				//尺寸
		BE_DATA_VALUE(vector<int>, vecRebarType)			//钢筋类型
		BE_DATA_VALUE(vector<double>, vecDirSpacing)			//间隔
		BE_DATA_VALUE(vector<double>, vecStartOffset)			//起点偏移
		BE_DATA_VALUE(vector<double>, vecEndOffset)			//终点偏移
		BE_DATA_VALUE(vector<double>, vecLevelSpace)			//与前层间距
		BE_DATA_VALUE(vector<int>, vecRebarLevel)				// 钢筋层数与前后中间有关
		BE_DATA_VALUE(vector<int>, vecDataExchange)			//数据交换
		BE_DATA_REFER(int, staggeredStyle)					// 交错类型
		//BE_DATA_VALUE(vector<int>, vecRebarColor)			//钢筋颜色
		BE_DATA_VALUE(vector<int>, vecRebarLineStyle)		//钢筋线形
		BE_DATA_VALUE(vector<int>, vecRebarWeight)			//钢筋线宽

		BE_DATA_VALUE(vector<PIT::LapOptions>, vecLapOptions)			//搭接选项
		BE_DATA_VALUE(vector<TwinBarSet::TwinBarLevelInfo >, vecTwinRebarLevel)	//并筋层
		BE_DATA_VALUE(vector<vector<PIT::EndType> >, vvecEndType)		//端部样式
		BE_DATA_VALUE(vector<ElementId>, vecSetId)								//rebarelementSet的每个Id
		BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)     //前面线的所有点
		BE_DATA_VALUE(Abanurus_PTRebarData, Abanurus_PTRebarData) //支墩点筋数据

public:
	MySlabRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~MySlabRebarAssembly() {};
	
public:
	vector<vector<vector<DPoint3d>> > m_vecAllRebarStartEnd;
	map<int, vector<vector<DPoint3d>>> m_vecAllRebarStartEndMap;
	vector<ElementRefP> m_allLines;//预览按钮按下后的所有钢筋线
	void ClearLines();
	bool CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);
public:
	std::string m_strElmName;
	double m_height;
	int m_strSlabRebarMethod;//配筋方式    0:默认，1：正交，2：放射
	int m_nowvecDir;//当前钢筋方向，0表示x轴，1表示z轴
private:
	BE_DATA_VALUE(SlabType, slabType)				//墙类型
protected:
	void			Init();

protected:
	virtual int		GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 2; }

	virtual bool	AnalyzingSlabGeometricData(ElementHandleCR eh) { return true; }


public:
	virtual bool	InsertMakeRebars(DgnModelRefP modelRef) { return true; }
	virtual bool	MakeRebars(DgnModelRefP modelRef) { return true; }
	virtual bool	SetSlabData(ElementHandleCR eh) { return true; }
	virtual void	SetisCombineFloor(bool isCombine) { return; };
	virtual void	SetSlabRebarDir(DSegment3d& Seg, ArcRebar&  mArcLine) { return; }
//	virtual void	InitUcsMatrix() {}

	virtual bool	MakeACCRebars(DgnModelRefP modelRef) { return true; }

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef) {}

	static SlabType JudgeSlabType(ElementHandleCR eh);

	static bool IsSlabSolid(ElementHandleCR eh);
	void SetConcreteData(Concrete const & concreteData);
	void SetRebarData(vector<PIT::ConcreteRebar> const& vecRebarData);
	void SetRebarEndTypes(vector<PIT::EndType> const& vvecEndTypes);
	//void SetTwinbarInfo(TwinBarSet::TwinBarInfo const& twInfo);
	//void GetTwinbarInfo(TwinBarSet::TwinBarInfo& twInfo);
	void InitRebarSetId();
	void GetConcreteData(Concrete& concreteData);
	void GetRebarData(vector<PIT::ConcreteRebar>& vecData) const;
	static bool IsSmartSmartFeature(EditElementHandle& eeh);
	void SetTieRebarInfo(TieReBarInfo const& tieRebarInfo);
	const TieReBarInfo GetTieRebarInfo() const;
private:
	TieReBarInfo m_tieRebarInfo;
};

class CSlabRebarDlg;
class STSlabRebarAssembly : public MySlabRebarAssembly
{
public:
	BE_DATA_VALUE(STWallGeometryInfo, STwallData)			//STWall几何数据
		BE_DATA_VALUE(bool, IsTwinrebar)			//在画rebarcuve时，判断是否为并筋层
public:
	DgnModelRefP m_model;
	std::vector<EditElementHandle*> m_Noweeh;
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//筛选后的孔洞

	EditElementHandle* m_pOldElm;  // 补完孔洞的板
	~STSlabRebarAssembly()
	{
		if (m_pOldElm != NULL)
		{
			delete m_pOldElm;
			m_pOldElm = NULL;
		}

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
		m_isBend = false;
		m_mapRroundPts.clear();
		m_mapAllRebarpts.clear();
		m_mapActualAllRebarpts.clear();
		m_lae = 0.0;
		m_floor_Thickness = 0.0;
		m_HSpace = 0.0;
		m_VSpace = 0.0;
	};

public:
	std::vector<DPoint3d>     m_vecRebarPtsLayer;
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//被孔洞截取前的所有并筋点数据
	std::vector<DPoint3d>     m_vecTieRebarPtsLayer; //拉筋生成所需要的钢筋点数据
	RebarSetTagArray rsetTags;
	CSlabRebarDlg * pSlabDoubleRebarDlg;

	bool m_isAbanurus = false;//是否是支墩配筋

	//廊道板对应数据
	struct LDFloorData
	{
		DPoint3d oriPt = { 0 };//廊道板最小点
		double Xlenth = 0;//转换到XOY平面后最大X长度
		double Ylenth = 0;
		double Zlenth = 0;//板厚
		DVec3d Vec = DVec3d::From(0, 0, 1);//默认取Z轴方向
		MSElementDescrP facedes = nullptr;//底面
		MSElementDescrP upfaces[40];//记录板顶墙与板相交的面
		int upnum = 0;//记录板上墙面个数
		MSElementDescrP downfaces[10];//记录板底墙与板相交的面
		int downnum = 0;//记录板上墙面个数
	}m_ldfoordata;

	struct LDSlabGeometryInfo//几何信息
	{
		DPoint3d ptStart;
		DPoint3d ptEnd;
		double length;
		double width;    //宽度
		double height;
		DVec3d vecZ;

	}m_STslabData;

protected:
	//	void  CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef);
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 3; }//判断是哪个类
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"LDSlab Rebar"; }//描述
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"LDSlab Rebar"; }
	virtual bool        OnDoubleClick() override;//双击后再次出现对话框
	virtual bool        Rebuild() override;//构件修改后，调用重新调整钢筋

protected:

	virtual bool	AnalyzingSlabGeometricData(ElementHandleCR eh);

	void MakeFaceRebars(int& iTwinbarSetIdIndex, int& setCount, int i, double dLength, double dWidth, vector<PIT::EndType>& vecEndType,
		vector<CVector3D>& vecEndNormal, double dis_x, double dis_y,
		CMatrix3D&  mat, CMatrix3D&  matTb);
	RebarSetTag* STSlabRebarAssembly::MakeRebars
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		double              xLen,
		double              width,
		double              spacing,
		double              startOffset,
		double              endOffset,
		vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
		vector<CVector3D> const& vecEndNormal,
		CMatrix3D const&    mat,
		TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
		bool				bTwinbarLevel,
		int level,
		int grade,
		int DataExchange,
		DgnModelRefP        modelRef
	);
	RebarSetTag* STSlabRebarAssembly::MakeRebars_Abanurus//支墩配筋
	(
		ElementId&          rebarSetId,
		int level,//插筋层，下右上左以此为0，1，2，3
		vector<vector<DPoint3d>> const& rebarpts,
		PITRebarEndTypes   &PITendTypes,	//存储起点端部与终点端部数据
		RebarEndTypes &endtypes
	);
	bool STSlabRebarAssembly::makeRebarCurve_G
	(
		vector<PITRebarCurve>&     rebars,
		double                  xPos,
		double                  yLen,
		double					startOffset,
		double					endOffset,
		PITRebarEndTypes&		endTypes,
		CMatrix3D const&        mat,
		bool&                tag,
		bool isTwin
	);
	void CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }//创建几何
	BE_DECLARE_VMS(STSlabRebarAssembly, RebarAssembly)
		bool isTemp = false;
	ElementHandle ehSel;
public:
	STSlabRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		MySlabRebarAssembly(id, modelRef)
#ifdef PDMSIMPORT
		, pSlab(NULL)
#endif
	{
		m_pOldElm = NULL;
		pSlabDoubleRebarDlg = NULL;
		m_vecAllRebarStartEnd.clear();
	};

	virtual bool	SetSlabData(ElementHandleCR eh);
	virtual void	SetSlabRebarDir(DSegment3d& Seg, ArcRebar&  mArcLine);
	virtual bool	MakeRebars(DgnModelRefP modelRef);
	virtual void CalculateUseHoles(DgnModelRefP modelRef);

	/*支墩配筋*/
	bool m_isBend = false;//是否需要弯锚，锚入总长度为Lae

	// 钢筋起点位于支墩里，钢筋终点位于板里
	map <int, vector<DPoint3d>> m_mapRroundPts;//保存支墩周围的四个钢筋的起始点,左下，右下，右上，左上，int依次为0，1，2，3 

	map <int, vector<vector<DPoint3d>>> m_mapAllRebarpts;//保存所有的钢筋起始点,下，右，上，左一排钢筋，int依次为0，1，2，3
														 //四周的点筋包括在上下两排，左右不包括
	
	map <int, vector<vector<DPoint3d>>> m_mapActualAllRebarpts;//实际的钢筋的点的位置，避免点筋与箍筋重合
	/// @brief 扫描range内的所有元素
	/// @param range 
	/// @param filter 过滤函数
	/// @return 
	std::vector<ElementHandle> scan_elements_in_range(const DRange3d &range, std::function<bool(const ElementHandle &)> filter);
	
	bool is_Floor(const ElementHandle &element);//判断是否是板

	vector<ElementHandle> m_downFloor;//保留支墩下面的板
	
	double get_lae()const;//获取Lae长度

	double m_lae = 0.0;//锚固长度

	double m_floor_Thickness = 0.0;//支墩下面的板的厚度

	double m_HSpace = 0.0;//点筋横向间距

	double m_VSpace = 0.0;//点筋纵向间距

	//通过支墩与下面的板的Range计算钢筋的起点终点
	void CalculateRebarPts();

	//移动点筋的位置，使得不与箍筋和拉筋重合
	void MoveRebarPts();
};
