#pragma once
class WallRebarAssembly;
#include "WallRebarAssembly.h"

class STWallRebarAssembly : public WallRebarAssembly
{
	BE_DATA_VALUE(STWallGeometryInfo, STwallData)			//STWall几何数据
//	BE_DATA_VALUE(bool, IsTwinrebar)			//在画rebarcuve时，判断是否为并筋层
public:

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
	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);
	void ExtractBoundaryPoints(MSElementDescrP faceDescr, vector<DPoint3d>& vec_ptBoundary);
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
		, pWall(NULL)
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
	* @desc:		根据周围元素和钢筋线的位置计算钢筋是否合法
	* @param[in]	nowVec  判断是竖直方向还是水平方向
	* @param[in]	alleehs 周围元素
	* @param[in]	tmpendEndTypes 端部样式
	* @param[in]	lineEeh 钢筋线
	* @param[in]    Eleeh  开孔之前的实体（墙、板）
	* @param[in]	anchored_vec  锚入角度，方向
	* @param[in]	matrix  投影矩阵
	* @param[in]	moveDis 保护层距离
	* @param[in]	lenth   锚入长度
	* @param[in]	ori_Point   原端点位置
	* @param[in]	mov_Point   修改端点位置
	* @param[in]	type_flag   是否不需要端部样式，如果FLAGE=1表示不需要端部样式，如果FLAGE=2表示尾端点位置修改为mov_Point位置
	* @param[in/out]	data 配筋线数据
	* @author	ChenDong
	* @Date:	2024/10/17
	*/
	void JudgeBarLinesLegitimate(const CVector3D  nowVec, vector<EditElementHandle*>alleehs, PIT::PITRebarEndTypes& tmpendEndTypes,
		EditElementHandle &lineEeh, EditElementHandle *Eleeh, CVector3D anchored_vec
		, const Transform matrix, double MoveDis, double lenth, DPoint3d &Point, DPoint3d &Point2, int &type_Flag);

	/*
	* @desc:       处理板的关键点投影和交点调整。提取板的所有关键点，将其投影到钢筋线上，根据投影点与关键点的位置关系调整交点位置
	* @param[in]  floorEeh 楼板的编辑元素句柄，用于获取楼板的相关信息
	* @param[in]  interPts 钢筋线与楼板的交点集合，可能会根据处理结果进行调整
	* @param[in]  dSideCover 侧面保护层厚度，用于判断投影点与关键点的距离是否满足条件
	* @param[in]  thickness 楼板厚度，用于调整交点位置
	* @param[in]  isUpBoard 是否为顶板
	* @author     LiuSilei
	* @Date:      2025/4/10
	*/
	void ProcessBoardKeyPoints(EditElementHandle& floorEeh, vector<DPoint3d>& interPts, double dSideCover, double thickness, bool isUpBoard);

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
