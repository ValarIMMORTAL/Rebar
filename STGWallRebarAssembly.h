#pragma once

class WallRebarAssembly;
class GWallRebarAssembly;
#include "WallRebarAssembly.h"
#include "GWallRebarAssembly.h"

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