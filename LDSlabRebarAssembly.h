#pragma once
#pragma once

/*--------------------------------------------------------------------------------------+
|
|     $Source: SlabRebarAssembly.h $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/

#include "MySlabRebarAssembly.h"
#include <RebarElements.h>
#include "CommonFile.h"
#include "PITRebarCurve.h"
using namespace PIT;
class CSlabRebarDlg;
class CWallMainRebarDlg;
namespace Gallery
{
	class LDSlabRebarAssembly : public MySlabRebarAssembly
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
		~LDSlabRebarAssembly()
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
		};

	public:
		std::vector<DPoint3d>     m_vecRebarPtsLayer;
		std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//被孔洞截取前的所有并筋点数据
		std::vector<DPoint3d>     m_vecTieRebarPtsLayer; //拉筋生成所需要的钢筋点数据
		RebarSetTagArray rsetTags;
		CSlabRebarDlg * pSlabDoubleRebarDlg;

		//廊道板对应数据
		struct LDFloorData
		{
			DPoint3d oriPt = {0};//廊道板最小点
			double Xlenth = 0;//转换到XOY平面后最大X长度
			double Ylenth = 0;
			double Zlenth = 0;//板厚
			DVec3d Vec = DVec3d::From(0, 0, 1);//默认取Z轴方向
			MSElementDescrP facedes = nullptr;//底面
			MSElementDescrP upfaces[40];//记录板顶墙与板相交的面
			int upnum = 0;//记录板上墙面个数
			MSElementDescrP downfaces[40];//记录板底墙与板相交的面
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

		enum  SideType//配筋面类型
		{
			Nor = 0,//正常面
			Out,//外侧面
			In  //内侧面
		}m_sidetype;

		struct OutSideQuJianInfo
		{
			int str = 0;
			int end = 0;
			bool addstr = false;//起始位置是否需要加钢筋
			bool addend = false;//终止位置是否需要加钢筋
			int strval = 0;
			int endval = 0;
		};

		struct OutSideFaceInfo//外侧面计算得到的配筋信息，每一个外侧面
		{
			OutSideQuJianInfo  pos[10] = { 0 };//与钢筋线平行的边坐标区间（横向钢筋时为Z坐标值，竖向钢筋时为X坐标值）
			int  posnum = 0;//标识区间数量;
			bool isdelstr = false;//是否删除起始钢筋，为了配合墙钢筋生成
			bool isdelend = false;//是否删除尾部钢筋，为了配合墙钢筋生成
			bool Verstr = false;//起始有垂直墙
			bool Verend = false;//尾部有垂直墙
			double calLen = 0;//判断当前钢筋层，钢筋前面还有层，起点部分长度缩小1个钢筋直径，没有层缩不缩小（钢筋锚入处理）
			int strval = 0;
			int endval = 0;
			PIT::EndType endtype = { 0 };
			PIT::EndType strtype = { 0 };
			void ClearData()
			{
				for (int i = 0; i < 10; i++)
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
			}
		}m_outsidef;

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
			InSideQuJianInfo  pos[10] = {0};//与钢筋线平行的边坐标区间（横向钢筋时为Z坐标值，竖向钢筋时为X坐标值）
			int  posnum = 0;//标识区间数量
			bool Verstr = false;//起始有垂直墙
			bool Verend = false;//尾部有垂直墙
			double calLen = 0;//有垂直墙时，减去四个钢筋直径
			int strval = 0;
			int endval = 0;
			PIT::EndType endtype = { 0 };
			PIT::EndType strtype = { 0 };
			void ClearData()
			{
				for (int i=0;i<10;i++)
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
			}
		}m_insidef;
		void CalculateOutSideData(MSElementDescrP face/*当前配筋面*/,
			MSElementDescrP tmpupfaces[40],
			MSElementDescrP tmpdownfaces[40],
			int i,
			DVec3d rebarVec, double& dis_x, double& dis_y);
		void CalculateInSideData(MSElementDescrP face/*当前配筋面*/,
			MSElementDescrP tmpupfaces[40],
			MSElementDescrP tmpdownfaces[40],
			int i,
			DVec3d rebarVec);

	protected:
		//	void  CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef);
		virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 3; }//判断是哪个类
		virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"LDSlab Rebar"; }//描述
		virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"LDSlab Rebar"; }
		virtual bool        OnDoubleClick() override;//双击后再次出现对话框
		virtual bool        Rebuild() override;//构件修改后，调用重新调整钢筋

	protected:

		virtual bool	AnalyzingSlabGeometricData(ElementHandleCR eh);

		void MakeFaceRebars(int& iTwinbarSetIdIndex,int& setCount, int i, double dLength, double dWidth, vector<PIT::EndType>& vecEndType,
			vector<CVector3D>& vecEndNormal, double dis_x, double dis_y,
			CMatrix3D&  mat, CMatrix3D&  matTb);
		void CreateOutSideFaceAssembly(int& iTwinbarSetIdIndex,int& setCount, MSElementDescrP upface,
			MSElementDescrP tmpupfaces[40],
			MSElementDescrP tmpdownfaces[40],
			int i,
			DVec3d rebarVec,
			vector<CVector3D>& vTrans,
			vector<CVector3D>& vTransTb
		);
		void CreateInSideFaceAssembly(int& iTwinbarSetIdIndex, int& setCount, MSElementDescrP upface,
			MSElementDescrP tmpupfaces[40],
			MSElementDescrP tmpdownfaces[40],
			int i,
			DVec3d rebarVec,
			vector<CVector3D>& vTrans,
			vector<CVector3D>& vTransTb
		);
		RebarSetTag* LDSlabRebarAssembly::MakeRebars
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
			DgnModelRefP        modelRef,
			/*int rebarColor,*/
			int rebarLineStyle,
			int rebarWeight,
			int index
		);
		bool LDSlabRebarAssembly::makeRebarCurve_G
		(
			vector<PITRebarCurve>&     rebars,
			double                  xPos,
			double                  yLen,
			double					startOffset,
			double					endOffset,
			PITRebarEndTypes&		endTypes,
			CMatrix3D const&        mat,
			bool&                tag,
			bool isTwin,
			vector<DPoint3d> vec_ptBoundary
		);
		void CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef);
		void ReTransFaces(vector<CVector3D>& vTrans, vector<CVector3D>& vTransTb, int i
			, MSElementDescrP& upface, MSElementDescrP tmpupfaces[40], MSElementDescrP tmpdownfaces[40]);
	public:
		virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }//创建几何
		BE_DECLARE_VMS(LDSlabRebarAssembly, RebarAssembly)
		bool isTemp = false;
		ElementHandle ehSel;
	public:
		LDSlabRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
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

		/// @brief 获得当前钢筋层的Lae参数(锚固长度）
			/// @return 返回UOR为单位的Lae长度，如果获取失败，返回值小于0
		double get_lae()const;

		/// @brief 获得当前钢筋层的L0参数（搭接长度）
		/// @return 返回UOR为单位的L0长度，如果获取失败，返回值小于0
		double get_la0()const;

		/// @brief 扫描range内的所有元素
		/// @param range 
		/// @param filter 过滤函数
		/// @return 
		std::vector<ElementHandle> scan_elements_in_range(const DRange3d &range, std::function<bool(const ElementHandle &)> filter);
		bool is_Floor(const ElementHandle& element);

		bool is_Wall(const ElementHandle &element);
		

		bool m_isdown = false; // 是否是下面的一层钢筋，是的话遇到孔洞需要往上锚入

		bool m_isoutside = false;// 是否是外侧面；外侧面时钢筋遇到孔洞不需要反向

		PITRebarEndType m_Nor_EndType;//设置不需要锚入钢筋的端部样式

		int m_xflag = 0;//标志，允许第一次生成横向钢筋的时候设置

		int m_yflag = 0;//标志，允许第一次生成纵向钢筋的时候设置

		double m_RebarRadius;//钢筋半径，用于遇到孔洞的钢筋设置锚入长度

		map<int, double> m_mapLevelRadius;//存储每一层钢筋的半径

		int m_rebarlevel;//记录钢筋是第几层便于遇到孔洞时判断是x还是y方向

		int m_endtypes = 0;//记录 x或y 方向的钢筋是否有垂直墙判断是否需要端部样式

		vector<int> m_XvecbegAndEnd;//记录 X 方向正常没有遇到孔洞钢筋的起始点，用于判断有些孔洞在边缘的钢筋

		vector<int> m_YvecbegAndEnd;//记录 Y 方向正常没有遇到孔洞钢筋的起始点，用于判断有些孔洞在边缘的钢筋

		int m_xdistance;//记录钢筋起始终点两点的距离，孔洞在边缘的钢筋起始终点的距离小于正常的钢筋距离

		int m_ydistance;//记录钢筋起始终点两点的距离，孔洞在边缘的钢筋起始终点的距离小于正常的钢筋距离

		vector<ElementHandle> m_Allwalls;//保留板边附近所有的墙
		
		//设置钢筋的curve起点的端部样式
		PITRebarEndType Set_begCurvePoint(PITRebarEndTypes &tmpendTypes, PITRebarEndTypes &endTypes, map<int, DPoint3d> &map_pts, int &num, int &begflag, DRange3d &hole_range, double &xStrdistance, double &xEnddistance, double &yStrdistance, double &yEnddistance, map<int, DPoint3d>::iterator &itr, map<int, DPoint3d>::iterator &tmpitrnext);
		
		//设置钢筋的curve终点的端部样式
		PITRebarEndType Set_endCurvePoint(PITRebarEndTypes &tmpendTypes, PITRebarEndTypes &endTypes, map<int, DPoint3d> &map_pts, int &num, int &endflag, DRange3d &hole_range, double &xStrdistance, double &xEnddistance, double &yStrdistance, double &yEnddistance, map<int, DPoint3d>::iterator &itrplus);
		
		//设置端部样式
		PITRebarEndType Set_endType();

		//设置内侧面减去两个墙面之后的偏移距离
		double InsideFace_OffsetLength(DPoint3dCR RebarlineMidPt);

		//设置钢筋起点终点向内偏移 墙的外侧面一层的两根钢筋的直径
		double WallRebars_OffsetLength(DPoint3dCR RebarPt);

		bool m_isMidFloor = false;//是否是原Z型板 卡在墙中间的板

		bool m_isIndownWall = false;//判断Z型板拆开的墙是属于板的上方还是下方

		bool m_isXdir = false;//判断Z型板拆开的墙是X方向还是Y方向

		//保存所有扫描到的墙和板自身
		vector <EditElementHandle*> m_ScanedAllWallsandFloor;

		bool m_strDelete = false;//外侧面钢筋靠近墙面的起始钢筋是否删除，配合墙钢筋生成

		bool m_endDelete = false;//外侧面钢筋终始钢筋是否删除，配合墙钢筋生成

		bool m_isCombineFloor = false;//是否是合并板配筋

		virtual void SetisCombineFloor(bool isCombine) { m_isCombineFloor = isCombine; };

		//bool m_isAddset = true;

		//int m_rebarSetNum = 0;

		//double m_adjustedSpacing;

		//RebarSetP   m_rebarSet = nullptr;
	};
}

