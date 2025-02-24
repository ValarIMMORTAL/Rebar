#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	关联构建配筋
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/04/21
	Version:		V1.0
*	Description:	PITACCRebarAssembly
*	History:
*	1. Date:		2021/04/21
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/

#include <RebarElements.h>
#include "../CommonFile.h"
#include "../PITRebarEndType.h"
#include "../PITRebarCurve.h"
#include "../PITRebarAssembly.h"

class CWallRebarDlgNew;
namespace PIT
{
	class ACCCNRebarAssembly : public PITRebarAssembly
	{
	public:
		enum ComponentType
		{
			SLAB,
			STWALL,
			GWALL,
			Other
		};

		    BE_DATA_REFER(BeMatrix, Placement)
			BE_DATA_VALUE(UINT, ACCRebarMethod)					//关联构件配筋方式
			BE_DATA_VALUE(double, PositiveCover)				//正面保护层
			BE_DATA_VALUE(double, ReverseCover)					//反面保护层
			BE_DATA_VALUE(double, SideCover)					//侧面保护层
			BE_DATA_VALUE(int, RebarLevelNum)					//钢筋层数
			BE_DATA_VALUE(vector<int>, vecDir)					//方向,0表示x轴，1表示z轴
			BE_DATA_VALUE(vector<BrString>, vecDirSize)			//尺寸
			BE_DATA_VALUE(vector<int>, vecRebarType)			//钢筋类型
			BE_DATA_VALUE(vector<double>, vecDirSpacing)		//间隔
			BE_DATA_VALUE(vector<double>, vecStartOffset)		//起点偏移
			BE_DATA_VALUE(vector<double>, vecEndOffset)			//终点偏移
			BE_DATA_VALUE(vector<double>, vecLevelSpace)		//与前层间距
			BE_DATA_VALUE(vector<int>, vecDataExchange)			//数据交换
			BE_DATA_VALUE(vector<int>, vecRebarLevel)			// 钢筋层号，分前、后、中间层

			BE_DATA_VALUE(vector<vector<PIT::EndType> >, vvecEndType)		//端部样式
			BE_DATA_VALUE(vector<TwinBarSet::TwinBarLevelInfo >, vecTwinRebarLevel)	//并筋层
			BE_DATA_VALUE(vector<AssociatedComponent >, vecAC)	//关联构件
			BE_DATA_VALUE(vector<ElementId>, vecSetId)								//SetId
			BE_DATA_VALUE(ComponentType, cpType)				//构件类型
			BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)     //前面线的所有点
			BE_DATA_VALUE(vector<DPoint3d>, vecCutPoints)		//截断体中心点
	public:
		double m_width;
		vector<ElementRefP> m_allLines;//预览按钮按下后的所有钢筋线

		ACCCNRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL)
			:PITRebarAssembly(id, modelRef),
			m_PositiveCover(0),
			m_ReverseCover(0),
			m_SideCover(0),
			m_RebarLevelNum(4),
			m_width(0)
		{}
	protected:
		/*
		* @description: 数据初始化
		*
		* @return	无
		* @author	LiuXiang
		* @Time		2021/04/21
		*/
		void			Init();
		
		/*
		* @description: 防止出现意外问题，每个从RebarAssembly派生的类中的返回值必须不一样
		*
		* @return	无
		* @author	LiuXiang
		* @Time		2021/04/21
		*/
		virtual int		GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 3; }

	public:
		/*
		* @description: 判断构件类型
		* @param	eh	IN		待配筋构件元素
		*
		* @return	待配筋构件类型
		* @author	LiuXiang
		* @Time		2021/04/21
		*/
		static ComponentType JudgeWallType(ElementHandleCR eh);

		static bool IsSmartSmartFeature(EditElementHandle& eeh);

		void SetConcreteData(Concrete const & concreteData);
		void SetRebarEndTypes(vector<PIT::EndType> const& vvecEndTypes);
		void GetConcreteData(Concrete& concreteData);
		void SetTieRebarInfo(TieReBarInfo const& tieRebarInfo);
		const TieReBarInfo GetTieRebarInfo() const;
//		void InitRebarSetId();
		void SetRebarData(vector<PIT::ConcreteRebar> const& vecRebarData);
		void GetRebarData(vector<PIT::ConcreteRebar>& vecData) const;

		void ClearLines();

		virtual bool	SetComponentData(ElementHandleCR eh) { return true; }
		virtual bool	MakeRebars(DgnModelRefP modelRef) { return true; }
		
	private:
		TieReBarInfo m_tieRebarInfo;
	};

	class ACCCNWallRebarAssembly : public ACCCNRebarAssembly
	{
	public:
		ACCCNWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL)
		{

		}
		virtual ~ACCCNWallRebarAssembly()
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
		vector<vector<DSegment3d> >		m_vecRebarStartEnd;	//规避了孔洞的所有点

		std::vector<vector<DSegment3d>> m_vecTieRebarStartEnd; // 规避了孔洞的所有点 -- 拉筋使用

		std::vector<EditElementHandle*> m_Holeehs;
		std::vector<EditElementHandle*> m_Negs;
		std::vector<EditElementHandle*> m_useHoleehs;//筛选后的孔洞
		std::map<EditElementHandle*, EditElementHandle*> m_doorsholes;//所有门洞，及门洞上的负实体
		DPoint3d m_LineNormal;

		double m_angle_left;
		double m_angle_right;

		BE_DATA_VALUE(DSegment3d, segOrg)						//原模型正面
		BE_DATA_VALUE(double, heightOrg)						//原模型高度
	protected:
		virtual int		GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 3; }

		virtual bool	AnalyzingWallGeometricDataAndHoles(ElementHandleCR eh, STWallGeometryInfo &data);

		bool	AnalyzingWallGeometricData(ElementHandleCR eh, STWallGeometryInfo &data,bool bCut = false);


	public:
		virtual bool	MakeRebars(DgnModelRefP modelRef) { return true; }
		virtual void	InitUcsMatrix() {}
		virtual bool	SetComponentData(ElementHandleCR eh) { return true; }

//		virtual void	CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef) {}


		static bool IsWallSolid(ElementHandleCR eh);


	};


	class ACCCNSTWallRebarAssembly : public ACCCNWallRebarAssembly
	{
		BE_DATA_VALUE(STWallGeometryInfo, STwallData)			//STWall几何数据

	public:
		CWallRebarDlgNew * pACCSTWallDoubleRebarDlg;
		vector<EditElementHandle*> m_CutSolids;
		RebarSetTagArray m_rsetTags;
		~ACCCNSTWallRebarAssembly(){
			std::for_each(m_CutSolids.begin(), m_CutSolids.end(), [](EditElementHandle* &eh) {delete eh; eh = NULL; });
		};

		std::vector<DPoint3d>     m_vecRebarPtsLayer;//被孔洞截取前的所有钢筋点数据
	private:
		bool makeRebarCurve(vector<PITRebarCurve>& rebar, double xPos, double height, double startOffset,
			double endOffset, PITRebarEndTypes const& endType, CMatrix3D const& mat, bool isTwin = false);
		vector<DSegment3d> vecStartEnd;
	protected:
		virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 2; }
		virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"STWall Rebar"; }
		virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"STWall Rebar"; }
		virtual bool        OnDoubleClick() override;
		virtual bool        Rebuild() override;

	protected:
		/*

		*/
		RebarSetTag* MakeRebars(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset,
			double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat,
			const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel, int level, int grade,int DataExchange, DgnModelRefP modelRef);

	public:
		virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
		BE_DECLARE_VMS(ACCCNSTWallRebarAssembly, RebarAssembly)

	public:
		ACCCNSTWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
			ACCCNWallRebarAssembly(id, modelRef)
		{
			pACCSTWallDoubleRebarDlg = nullptr;
			m_vecRebarStartEnd.clear();
		};
		
		virtual bool	SetComponentData(ElementHandleCR eh);

		virtual bool	MakeRebars(DgnModelRefP modelRef);

		virtual void	InitUcsMatrix();

		virtual void	CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, const CVector3D& org = CVector3D(0,0,0), DgnModelRefP modelRef = ACTIVEMODEL);
		void  CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef);
		void CalculateUseHoles(DgnModelRefP modelRef);

		vector<STWallGeometryInfo> m_vecCutWallData;
		void			SetCutSoild(vector<EditElementHandle*> vecCutSolid,DSegment3d segLine,double height) 
		{
			m_CutSolids = vecCutSolid; 
			m_vecCutWallData.clear();
			m_vecCutWallData.shrink_to_fit();
			SetsegOrg(segLine);
			SetheightOrg(height);
			for (size_t i = 0; i < m_CutSolids.size(); ++i)
			{
				STWallGeometryInfo cutData;
				AnalyzingWallGeometricData(*m_CutSolids[i], cutData,true);
				m_vecCutWallData.push_back(cutData);
			}
		}
		vector<EditElementHandle*>&			PopCutSoild() { return m_CutSolids; }
		static UInt32	JudgeComponentRelation(const STWallGeometryInfo &CurrSTWall, const STWallGeometryInfo &ACCSTwall);
	};
}


