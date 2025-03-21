#pragma once
class FacesRebarAssembly;
#include "FacesRebarAssembly.h"

class PlaneRebarAssembly : public FacesRebarAssembly
{
	BE_DATA_VALUE(PIT::LineSegment, LineSeg1)	//基准线段，若平面垂直与XOY平面，则为底边，若平面平行与XOY平面，则为左边
		BE_DATA_VALUE(PIT::LineSegment, LineSeg2)	//基准线段，平面内过LineSeg1的起点垂直与LineSeg的方向的线段
		BE_DATA_VALUE(STWallGeometryInfo, STwallData)
		//	bool				_bAnchor;
		unsigned short		_anchorPos = 0;			//直锚位置
	ElementHandle		_ehCrossPlanePre;
	ElementHandle		_ehCrossPlaneNext;
	unsigned short		_bendPos = 0;			//90度弯曲位置

public:
	PlaneRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	~PlaneRebarAssembly();

private:
	//设置内侧面减去两个墙面之后的偏移距离
	double InsideFace_OffsetLength(DPoint3dCR RebarlineMidPt);
	//获取rebarcurve,主要是钢筋线与面拉伸的实体相交，然后在钢筋两端设置事先求出的端部样式
	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebar, const PIT::PITRebarEndTypes& endTypes);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Plane; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Plane Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Plane Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:
	RebarSetTag* MakeRebars(ElementId& rebarSetId, PIT::LineSegment rebarLine, PIT::LineSegment vec, int dir, BrStringCR sizeKey, double xLen, double spacing, double startOffset, double endOffset, int	level, int grade, int DataExchange, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, DgnModelRefP modelRef);

	void CalculateUseHoles(DgnModelRefP modelRef);


public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(PlaneRebarAssembly, RebarAssembly)

public:
	//主要求出面配筋横竖两根钢筋的向量
	virtual bool	AnalyzingFaceGeometricData(EditElementHandleR eeh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);
	//	void SetAnchor(bool anchor) { _bAnchor = anchor; }
	void SetCrossPlanePre(ElementHandleCR eh) { _ehCrossPlanePre = eh; }
	void SetCrossPlaneNext(ElementHandleCR eh) { _ehCrossPlaneNext = eh; }
	//分析是板还是墙，如果是板则分析出板厚，底面，顶面
	virtual bool	AnalyzingFloorData(ElementHandleCR eh);
	//分析墙相关信息，并分析出墙钢筋锚固信息
	void AnalyzeFloorAndWall(EditElementHandleCR eeh, int i, MSElementDescrP EdpFace);
	void DealWintHoriRebar(PIT::LineSegment& lineSeg1, PIT::LineSegment& linesegment2, int rebarnum);
	void GetHoriEndType(PIT::LineSegment lineSeg1, int rebarnum);
	//分析面的类型
	int GetFaceType(MSElementDescrP face, MSElementDescrP upface[40], int upfacenum, MSElementDescrP downface[40], int downfacenum, int i, DVec3d vecRebar);
	void ChangeRebarLine(PIT::LineSegment& lineSeg);
	//bisSumps是否是集水坑
	void CreateAnchorBySelf(vector<MSElementDescrP> tmpAnchordescrs, PIT::LineSegment Lineseg, double benrandis, double la0, double lae, double diameter, double diameterPre, int irebarlevel, bool isInface = true, bool bisSumps = false);
	//钢筋遇到孔洞时，求出端部锚固信息，包括长度，方向。
	bool GetHoleRebarAnchor(Dpoint3d ptstart, Dpoint3d ptend, Dpoint3d curPt, PIT::PITRebarEndType& endtype);

	//设置当前配筋面
	virtual void SetCurentFace(EditElementHandle* face)
	{
		m_CurentFace = face;
	}
	/*
	* @desc:	根据孔洞截断钢筋锚固长度得到新的锚固长度
	* @param[in] ptStr 钢筋锚固部分的起始点
	* @param[in] vecAnchor 锚固方向，用来计算锚固结束点
	* @param[in] rebarLevel 当前钢筋层，如果ptStr是当前层起始点，则填0，如果ptStr是第一层起始点，则填当前层
	* @param[in\out] dAnchorleng 锚固长度，用来计算锚固结束点，并在截断后更新为新的长度
	* @author	Hong ZhuoHui
	* @Date:	2024/3/1
	*/
	void CutRebarAnchorLeng(Dpoint3d ptStr, CVector3D vecAnchor, int rebarLevel, double& dAnchorleng);
	/*
	* @desc:		根据周围元素和钢筋线的位置计算钢筋是否合法
	* @param[in]	originPt  钢筋起始点
	* @param[in]	dSideCover 侧向保护距离
	* @param[in]	alleehs  所有实体集合
	* @param[in\out]    tmpEndType  钢筋的末端类型
	* @author	LiuSilei
	* @Date:	2024/11/15
	*/
	// 钢筋锚固检查与处理函数
	void PlaneRebarAssembly::CheckRebarAnchorage(
		DPoint3d& originPt,    // 钢筋起始点
		double dSideCover,  // 侧向保护距离
		vector<EditElementHandle*> alleehs, // 所有实体集合
		PIT::PITRebarEndType& tmpEndType // 钢筋的末端类型
	);
	//集水坑
	void CreateCatchpitBySelf(vector<MSElementDescrP> tmpAnchordescrs, PIT::LineSegment Lineseg, double benrandis, double la0, double lae, double diameter, int irebarlevel, bool isInface = true, bool bisSumps = false, bool isYdir = false);
	/*
	* @desc:		处理钢筋对于Z型板的钢筋分区。如果本面更长则进行分区，无论长短都需要在拐角增加平面
	* @param[in]	sectionPairs  钢筋分区
	* @param[in]	parafaces 平行墙
	* @param[in]	minP  最小点
	* @param[in]	maxP  最大点
	* @param[in]    isXDir  钢筋分布方向
	* @author	LiuSilei
	* @Date:	2024/10/30
	*/
	bool CalculateZCorner(map<int, int>& sectionPairs, vector<MSElementDescrP>& parafaces, DPoint3d& minP, DPoint3d& maxP, bool isXDir = true);
	//分析内面钢筋信息，包括端部样式等
	void CalculateInSideData(MSElementDescrP face/*当前配筋面*/,
		MSElementDescrP tmpupfaces[40],
		MSElementDescrP tmpdownfaces[40],
		int i,
		DVec3d rebarVec);
	//分析外面钢筋信息，包括端部样式等
	void CalculateOutSideData(MSElementDescrP face/*当前配筋面*/,
		MSElementDescrP tmpupfaces[40],
		MSElementDescrP tmpdownfaces[40],
		int i,
		DVec3d rebarVec);

	// @brief 扫描range内的所有元素
	// @param range 
	// @param filter 过滤函数
	// @return 
	std::vector<ElementHandle> scan_elements_in_range(const DRange3d &range, std::function<bool(const ElementHandle &)> filter);

	/*
	* @desc:	判断该元素是否是墙元素
	* @param[in] element 元素
	*/
	bool is_Wall(const ElementHandle &element);

	/*
	* @desc:	设置钢筋起点终点向内偏移 墙的外侧面一层的两根钢筋的直径
	* @param[in] RebarPt 通过该点所在某个墙的范围中获取该墙的数据
	*/
	double WallRebars_OffsetLength(DPoint3dCR RebarPt);

	//去除重复点
	static void RemoveRepeatPoint(vector<DPoint3d>& vecPoint);
	static double GetLae();

	//廊道板对应数据
	struct LDFloorData
	{
		DPoint3d oriPt = { 0 };//廊道板最小点
		double Xlenth = 0;//转换到XOY平面后最大X长度
		double Ylenth = 0;
		double Zlenth = 0;//板厚
		DVec3d Vec = DVec3d::From(0, 0, 1);//默认取Z轴方向
		MSElementDescrP facedes = nullptr;//底面
		MSElementDescrP facedesUp = nullptr;//顶面
		MSElementDescrP upfaces[40];//记录板顶墙与板相交的面
		int upnum = 0;//记录板上墙面个数
		MSElementDescrP downfaces[40];//记录板底墙与板相交的面
		int downnum = 0;//记录板上墙面个数
	}m_ldfoordata;
	enum  SideType//配筋面类型
	{
		Nor = 0,//正常面
		Out,//外侧面
		In  //内侧面
	}m_sidetype;

	struct InSideQuJianInfo
	{
		int str = 0;
		int end = 0;
		bool addstr = false;//起始位置是否需要加钢筋
		bool addend = false;//终止位置是否需要加钢筋
		int strval = 0;
		int endval = 0;
	};

	struct InSideFaceInfo//内侧面计算得到的配筋信息，每一个外侧面
	{
		InSideQuJianInfo  pos[20] = { 0 };//与钢筋线平行的边坐标区间（横向钢筋时为Z坐标值，竖向钢筋时为X坐标值）
		int  posnum = 0;//标识区间数量
		bool Verstr = false;//起始有垂直墙
		bool Verend = false;//尾部有垂直墙
		double calLen = 0;//有垂直墙时，减去四个钢筋直径
		int strval = 0;
		int endval = 0;
		PIT::EndType endtype = { 0 };
		PIT::EndType strtype = { 0 };
		bool bStartAnhorsel = false;
		bool bEndAnhorsel = false;
		double dStartanchoroffset = 0;
		double dEndanchoroffset = 0;
		bool m_bStartIsSlefAhol = false;//当前弯钩是自选面锚固
		bool m_bEndIsSlefAhol = false;//当前弯钩是自选面锚固
		void ClearData()
		{
			for (int i = 0; i < 20; i++)
			{
				pos[i].str = 0;
				pos[i].end = 0;
				pos[i].addstr = false;
				pos[i].addend = false;
				pos[i].strval = 0;
				pos[i].endval = 0;
			}
			posnum = 0;
			Verstr = false;
			Verend = false;
			calLen = 0;
			strval = 0;
			endval = 0;
			endtype = { 0 };
			strtype = { 0 };
			bStartAnhorsel = false;
			bEndAnhorsel = false;
			dStartanchoroffset = 0;
			dEndanchoroffset = 0;
			m_bStartIsSlefAhol = false;
			m_bEndIsSlefAhol = false;

		}
	}m_insidef;
	bool m_bStartAnhorselslantedFace = false; //锚入斜面
	bool m_bEndAnhorselslantedFace = false;

	struct OutSideQuJianInfo
	{
		int str = 0;//区间起点
		int end = 0;//区间终点
		bool addstr = false;//起始位置是否需要加钢筋
		bool addend = false;//终止位置是否需要加钢筋
		int strval = 0;//起点可延伸的距离
		int endval = 0;//终点可延伸的距离
	};
	struct OutSideFaceInfo//外侧面计算得到的配筋信息，每一个外侧面
	{
		OutSideQuJianInfo  pos[20] = { 0 };//与钢筋线平行的边坐标区间（横向钢筋时为Z坐标值，竖向钢筋时为X坐标值）
		int  posnum = 0;//标识区间数量
		bool isdelstr = false;//是否删除起始钢筋，为了配合墙钢筋生成
		bool isdelend = false;//是否删除尾部钢筋，为了配合墙钢筋生成
		bool Verstr = false;//起始有垂直墙
		bool Verend = false;//尾部有垂直墙
		double calLen = 0;//判断当前钢筋层，钢筋前面还有层，起点部分长度缩小1个钢筋直径，没有层缩不缩小（钢筋锚入处理）
		int strval = 0;
		int endval = 0;
		PIT::EndType endtype = { 0 };
		PIT::EndType strtype = { 0 };
		bool bStartAnhorsel = false;
		bool bEndAnhorsel = false;
		double dStartanchoroffset = 0;
		double dEndanchoroffset = 0;
		bool m_bStartIsSlefAhol = false;
		bool m_bEndIsSlefAhol = false;
		void ClearData()
		{
			for (int i = 0; i < 20; i++)
			{
				pos[i].str = 0;
				pos[i].end = 0;
				pos[i].addstr = false;
				pos[i].addend = false;
				pos[i].strval = 0;
				pos[i].endval = 0;
			}
			posnum = 0;
			isdelstr = false;
			isdelend = false;
			Verstr = false;
			Verend = false;
			calLen = 0;
			strval = 0;
			endval = 0;
			endtype = { 0 };
			strtype = { 0 };
			bStartAnhorsel = false;
			bEndAnhorsel = false;
			dStartanchoroffset = 0;
			dEndanchoroffset = 0;
			m_bStartIsSlefAhol = false;
			m_bEndIsSlefAhol = false;
		}
	}m_outsidef;

	struct NorSideQuJianInfo
	{
		int str = 0;
		int end = 0;
		bool addstr = false;//起始位置是否需要加钢筋
		bool addend = false;//终止位置是否需要加钢筋
		int strval = 0;
		int endval = 0;
	};
	struct NorSideFaceInfo//竖直面计算得到的配筋信息，每一个竖直面
	{
		NorSideQuJianInfo  pos[20] = { 0 };//与钢筋线平行的边坐标区间（横向钢筋时为Z坐标值，竖向钢筋时为X坐标值）
		int  posnum = 0;//标识区间数量
		double calLen = 0;//判断当前钢筋层，钢筋前面还有层，起点部分长度缩小1个钢筋直径，没有层缩不缩小（钢筋锚入处理）
		int strval = 0;
		int endval = 0;
		PIT::EndType endtype = { 0 };
		PIT::EndType strtype = { 0 };
		bool bStartAnhorsel = false;
		bool bEndAnhorsel = false;
		double dStartanchoroffset = 0;
		double dEndanchoroffset = 0;
		bool m_bStartIsSlefAhol = false;
		bool m_bEndIsSlefAhol = false;
		void ClearData()
		{
			for (int i = 0; i < 20; i++)
			{
				pos[i].str = 0;
				pos[i].end = 0;
				pos[i].addstr = false;
				pos[i].addend = false;
				pos[i].strval = 0;
				pos[i].endval = 0;
			}
			posnum = 0;
			calLen = 0;
			strval = 0;
			endval = 0;
			endtype = { 0 };
			strtype = { 0 };
			bStartAnhorsel = false;
			bEndAnhorsel = false;
			dStartanchoroffset = 0;
			dEndanchoroffset = 0;
			m_bStartIsSlefAhol = false;
			m_bEndIsSlefAhol = false;
		}
	}m_norsidef;

	struct LDSlabGeometryInfo//几何信息
	{
		DPoint3d ptStart;
		DPoint3d ptEnd;
		double length;
		double width;    //宽度
		double height;
		DVec3d vecZ;

	}m_STslabData;

	struct verSlabFaceInfo
	{
		PIT::EndType endtype = { 0 };
		PIT::EndType strtype = { 0 };
		bool bStartAnhorsel = false;
		bool bEndAnhorsel = false;
		double dStartanchoroffset = 0;
		double dEndanchoroffset = 0;
		void ClearData()
		{
			endtype = { 0 };
			strtype = { 0 };
			bStartAnhorsel = false;
			bEndAnhorsel = false;
			dStartanchoroffset = 0;
			dEndanchoroffset = 0;
		}
	}m_verSlabFaceInfo;

	struct HoleRebarInfo
	{
		bool bIsUpFace = false;
		BrString brstring = "";

		void ClearData()
		{
			bIsUpFace = false;
			brstring = "";
		}

	}m_holeRebarInfo;

	CVector3D m_vecEndNormalStart;//锚入斜面时用，直接算出endnormal
	CVector3D m_vecEndNormalEnd;



	PIT::EndType m_topEndinfo = { 0 };
	double m_dUpSlabThickness = 0;
	bool m_bUpIsStatr = false;
	PIT::EndType m_bottomEndinfo = { 0 };
	double m_dbottomSlabThickness = 0;

	int m_wallTopFaceType = -1; //0:内面，1：外面
	int m_wallBottomFaceType = -1; //0:内面，1：外面
	double m_dTopOffset = 0;
	double m_dBottomOffset = 0;

	int m_iCurRebarDir = -1;
	std::vector<ElementHandle> m_associatedWall;

	PIT::EndType m_HorEndType = { 0 };
	PIT::EndType m_HorStrType = { 0 };
	int m_bStartType = -1;//0:内面，1：外面
	int m_bEndInType = -1;//0:内面，1：外面
	double m_dStartOffset = 0;
	double m_dEndOffset = 0;
	MSElementDescrP m_TopSlabEdp = nullptr;
	MSElementDescrP m_BottomSlabEdp = nullptr;
	vector<MSElementDescrP> m_vecWallEdp;

	int m_nowvecDir;//当前钢筋方向，0表示x轴，1表示z轴
	EditElementHandle* m_pOldElm = NULL;  // 补完孔洞的板

	double m_curreDiameter;//当前直径
	double m_twoFacesDistance = 0.0;//集水坑内侧面与外侧面之间的距离
	int m_curLevel;//当前配筋层
	EditElementHandle* m_CurentFace = nullptr;//当前的配筋面
	double m_slabThickness = 0.0;//面配筋所选择的板的厚度
	EditElementHandle* m_zCorner = nullptr;//Z型板拐角需要配置额外的钢筋

	vector<ElementHandle> m_Allwalls;//保留板边附近所有的墙
	vector<EditElementHandle*> m_allEehs;//自身以及附近所有实体
	bool m_strDelete = false;//外侧面钢筋靠近墙面的起始钢筋是否删除，配合墙钢筋生成

	bool m_endDelete = false;//外侧面钢筋终始钢筋是否删除，配合墙钢筋生成
};


