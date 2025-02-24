#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	�����������
*	Project:		��ά����ͼ��Ŀ
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
			BE_DATA_VALUE(UINT, ACCRebarMethod)					//����������ʽ
			BE_DATA_VALUE(double, PositiveCover)				//���汣����
			BE_DATA_VALUE(double, ReverseCover)					//���汣����
			BE_DATA_VALUE(double, SideCover)					//���汣����
			BE_DATA_VALUE(int, RebarLevelNum)					//�ֽ����
			BE_DATA_VALUE(vector<int>, vecDir)					//����,0��ʾx�ᣬ1��ʾz��
			BE_DATA_VALUE(vector<BrString>, vecDirSize)			//�ߴ�
			BE_DATA_VALUE(vector<int>, vecRebarType)			//�ֽ�����
			BE_DATA_VALUE(vector<double>, vecDirSpacing)		//���
			BE_DATA_VALUE(vector<double>, vecStartOffset)		//���ƫ��
			BE_DATA_VALUE(vector<double>, vecEndOffset)			//�յ�ƫ��
			BE_DATA_VALUE(vector<double>, vecLevelSpace)		//��ǰ����
			BE_DATA_VALUE(vector<int>, vecDataExchange)			//���ݽ���
			BE_DATA_VALUE(vector<int>, vecRebarLevel)			// �ֽ��ţ���ǰ�����м��

			BE_DATA_VALUE(vector<vector<PIT::EndType> >, vvecEndType)		//�˲���ʽ
			BE_DATA_VALUE(vector<TwinBarSet::TwinBarLevelInfo >, vecTwinRebarLevel)	//�����
			BE_DATA_VALUE(vector<AssociatedComponent >, vecAC)	//��������
			BE_DATA_VALUE(vector<ElementId>, vecSetId)								//SetId
			BE_DATA_VALUE(ComponentType, cpType)				//��������
			BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)     //ǰ���ߵ����е�
			BE_DATA_VALUE(vector<DPoint3d>, vecCutPoints)		//�ض������ĵ�
	public:
		double m_width;
		vector<ElementRefP> m_allLines;//Ԥ����ť���º�����иֽ���

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
		* @description: ���ݳ�ʼ��
		*
		* @return	��
		* @author	LiuXiang
		* @Time		2021/04/21
		*/
		void			Init();
		
		/*
		* @description: ��ֹ�����������⣬ÿ����RebarAssembly���������еķ���ֵ���벻һ��
		*
		* @return	��
		* @author	LiuXiang
		* @Time		2021/04/21
		*/
		virtual int		GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 3; }

	public:
		/*
		* @description: �жϹ�������
		* @param	eh	IN		������Ԫ��
		*
		* @return	����������
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
		vector<vector<DSegment3d> >		m_vecRebarStartEnd;	//����˿׶������е�

		std::vector<vector<DSegment3d>> m_vecTieRebarStartEnd; // ����˿׶������е� -- ����ʹ��

		std::vector<EditElementHandle*> m_Holeehs;
		std::vector<EditElementHandle*> m_Negs;
		std::vector<EditElementHandle*> m_useHoleehs;//ɸѡ��Ŀ׶�
		std::map<EditElementHandle*, EditElementHandle*> m_doorsholes;//�����Ŷ������Ŷ��ϵĸ�ʵ��
		DPoint3d m_LineNormal;

		double m_angle_left;
		double m_angle_right;

		BE_DATA_VALUE(DSegment3d, segOrg)						//ԭģ������
		BE_DATA_VALUE(double, heightOrg)						//ԭģ�͸߶�
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
		BE_DATA_VALUE(STWallGeometryInfo, STwallData)			//STWall��������

	public:
		CWallRebarDlgNew * pACCSTWallDoubleRebarDlg;
		vector<EditElementHandle*> m_CutSolids;
		RebarSetTagArray m_rsetTags;
		~ACCCNSTWallRebarAssembly(){
			std::for_each(m_CutSolids.begin(), m_CutSolids.end(), [](EditElementHandle* &eh) {delete eh; eh = NULL; });
		};

		std::vector<DPoint3d>     m_vecRebarPtsLayer;//���׶���ȡǰ�����иֽ������
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


