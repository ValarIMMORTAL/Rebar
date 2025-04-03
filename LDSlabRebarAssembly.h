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
		BE_DATA_VALUE(STWallGeometryInfo, STwallData)			//STWall��������
	    BE_DATA_VALUE(bool, IsTwinrebar)			//�ڻ�rebarcuveʱ���ж��Ƿ�Ϊ�����
	public:
		DgnModelRefP m_model;
		std::vector<EditElementHandle*> m_Noweeh;
		std::vector<EditElementHandle*> m_Holeehs;
		std::vector<EditElementHandle*> m_Negs;
		std::vector<EditElementHandle*> m_useHoleehs;//ɸѡ��Ŀ׶�

		EditElementHandle* m_pOldElm;  // ����׶��İ�
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
		std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//���׶���ȡǰ�����в��������
		std::vector<DPoint3d>     m_vecTieRebarPtsLayer; //������������Ҫ�ĸֽ������
		RebarSetTagArray rsetTags;
		CSlabRebarDlg * pSlabDoubleRebarDlg;

		//�ȵ����Ӧ����
		struct LDFloorData
		{
			DPoint3d oriPt = {0};//�ȵ�����С��
			double Xlenth = 0;//ת����XOYƽ������X����
			double Ylenth = 0;
			double Zlenth = 0;//���
			DVec3d Vec = DVec3d::From(0, 0, 1);//Ĭ��ȡZ�᷽��
			MSElementDescrP facedes = nullptr;//����
			MSElementDescrP upfaces[40];//��¼�嶥ǽ����ཻ����
			int upnum = 0;//��¼����ǽ�����
			MSElementDescrP downfaces[40];//��¼���ǽ����ཻ����
			int downnum = 0;//��¼����ǽ�����
		}m_ldfoordata;

		struct LDSlabGeometryInfo//������Ϣ
		{
			DPoint3d ptStart;
			DPoint3d ptEnd;
			double length;
			double width;    //���
			double height;
			DVec3d vecZ;

		}m_STslabData;

		enum  SideType//���������
		{
			Nor = 0,//������
			Out,//�����
			In  //�ڲ���
		}m_sidetype;

		struct OutSideQuJianInfo
		{
			int str = 0;
			int end = 0;
			bool addstr = false;//��ʼλ���Ƿ���Ҫ�Ӹֽ�
			bool addend = false;//��ֹλ���Ƿ���Ҫ�Ӹֽ�
			int strval = 0;
			int endval = 0;
		};

		struct OutSideFaceInfo//��������õ��������Ϣ��ÿһ�������
		{
			OutSideQuJianInfo  pos[10] = { 0 };//��ֽ���ƽ�еı��������䣨����ֽ�ʱΪZ����ֵ������ֽ�ʱΪX����ֵ��
			int  posnum = 0;//��ʶ��������;
			bool isdelstr = false;//�Ƿ�ɾ����ʼ�ֽΪ�����ǽ�ֽ�����
			bool isdelend = false;//�Ƿ�ɾ��β���ֽΪ�����ǽ�ֽ�����
			bool Verstr = false;//��ʼ�д�ֱǽ
			bool Verend = false;//β���д�ֱǽ
			double calLen = 0;//�жϵ�ǰ�ֽ�㣬�ֽ�ǰ�滹�в㣬��㲿�ֳ�����С1���ֽ�ֱ����û�в�������С���ֽ�ê�봦��
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
			bool addstr = false;//��ʼλ���Ƿ���Ҫ�Ӹֽ�
			bool addend = false;//��ֹλ���Ƿ���Ҫ�Ӹֽ�
			int strval = 0;
			int endval = 0;
		};


		struct InSideFaceInfo//�ڲ������õ��������Ϣ��ÿһ�������
		{
			InSideQuJianInfo  pos[10] = {0};//��ֽ���ƽ�еı��������䣨����ֽ�ʱΪZ����ֵ������ֽ�ʱΪX����ֵ��
			int  posnum = 0;//��ʶ��������
			bool Verstr = false;//��ʼ�д�ֱǽ
			bool Verend = false;//β���д�ֱǽ
			double calLen = 0;//�д�ֱǽʱ����ȥ�ĸ��ֽ�ֱ��
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
		void CalculateOutSideData(MSElementDescrP face/*��ǰ�����*/,
			MSElementDescrP tmpupfaces[40],
			MSElementDescrP tmpdownfaces[40],
			int i,
			DVec3d rebarVec, double& dis_x, double& dis_y);
		void CalculateInSideData(MSElementDescrP face/*��ǰ�����*/,
			MSElementDescrP tmpupfaces[40],
			MSElementDescrP tmpdownfaces[40],
			int i,
			DVec3d rebarVec);

	protected:
		//	void  CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef);
		virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 3; }//�ж����ĸ���
		virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"LDSlab Rebar"; }//����
		virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"LDSlab Rebar"; }
		virtual bool        OnDoubleClick() override;//˫�����ٴγ��ֶԻ���
		virtual bool        Rebuild() override;//�����޸ĺ󣬵������µ����ֽ�

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

			vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
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
		virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }//��������
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

		/// @brief ��õ�ǰ�ֽ���Lae����(ê�̳��ȣ�
			/// @return ����UORΪ��λ��Lae���ȣ������ȡʧ�ܣ�����ֵС��0
		double get_lae()const;

		/// @brief ��õ�ǰ�ֽ���L0��������ӳ��ȣ�
		/// @return ����UORΪ��λ��L0���ȣ������ȡʧ�ܣ�����ֵС��0
		double get_la0()const;

		/// @brief ɨ��range�ڵ�����Ԫ��
		/// @param range 
		/// @param filter ���˺���
		/// @return 
		std::vector<ElementHandle> scan_elements_in_range(const DRange3d &range, std::function<bool(const ElementHandle &)> filter);
		bool is_Floor(const ElementHandle& element);

		bool is_Wall(const ElementHandle &element);
		

		bool m_isdown = false; // �Ƿ��������һ��ֽ�ǵĻ������׶���Ҫ����ê��

		bool m_isoutside = false;// �Ƿ�������棻�����ʱ�ֽ������׶�����Ҫ����

		PITRebarEndType m_Nor_EndType;//���ò���Ҫê��ֽ�Ķ˲���ʽ

		int m_xflag = 0;//��־�������һ�����ɺ���ֽ��ʱ������

		int m_yflag = 0;//��־�������һ����������ֽ��ʱ������

		double m_RebarRadius;//�ֽ�뾶�����������׶��ĸֽ�����ê�볤��

		map<int, double> m_mapLevelRadius;//�洢ÿһ��ֽ�İ뾶

		int m_rebarlevel;//��¼�ֽ��ǵڼ�����������׶�ʱ�ж���x����y����

		int m_endtypes = 0;//��¼ x��y ����ĸֽ��Ƿ��д�ֱǽ�ж��Ƿ���Ҫ�˲���ʽ

		vector<int> m_XvecbegAndEnd;//��¼ X ��������û�������׶��ֽ����ʼ�㣬�����ж���Щ�׶��ڱ�Ե�ĸֽ�

		vector<int> m_YvecbegAndEnd;//��¼ Y ��������û�������׶��ֽ����ʼ�㣬�����ж���Щ�׶��ڱ�Ե�ĸֽ�

		int m_xdistance;//��¼�ֽ���ʼ�յ�����ľ��룬�׶��ڱ�Ե�ĸֽ���ʼ�յ�ľ���С�������ĸֽ����

		int m_ydistance;//��¼�ֽ���ʼ�յ�����ľ��룬�׶��ڱ�Ե�ĸֽ���ʼ�յ�ľ���С�������ĸֽ����

		vector<ElementHandle> m_Allwalls;//������߸������е�ǽ
		
		//���øֽ��curve���Ķ˲���ʽ
		PITRebarEndType Set_begCurvePoint(PITRebarEndTypes &tmpendTypes, PITRebarEndTypes &endTypes, map<int, DPoint3d> &map_pts, int &num, int &begflag, DRange3d &hole_range, double &xStrdistance, double &xEnddistance, double &yStrdistance, double &yEnddistance, map<int, DPoint3d>::iterator &itr, map<int, DPoint3d>::iterator &tmpitrnext);
		
		//���øֽ��curve�յ�Ķ˲���ʽ
		PITRebarEndType Set_endCurvePoint(PITRebarEndTypes &tmpendTypes, PITRebarEndTypes &endTypes, map<int, DPoint3d> &map_pts, int &num, int &endflag, DRange3d &hole_range, double &xStrdistance, double &xEnddistance, double &yStrdistance, double &yEnddistance, map<int, DPoint3d>::iterator &itrplus);
		
		//���ö˲���ʽ
		PITRebarEndType Set_endType();

		//�����ڲ����ȥ����ǽ��֮���ƫ�ƾ���
		double InsideFace_OffsetLength(DPoint3dCR RebarlineMidPt);

		//���øֽ�����յ�����ƫ�� ǽ�������һ��������ֽ��ֱ��
		double WallRebars_OffsetLength(DPoint3dCR RebarPt);

		bool m_isMidFloor = false;//�Ƿ���ԭZ�Ͱ� ����ǽ�м�İ�

		bool m_isIndownWall = false;//�ж�Z�Ͱ�𿪵�ǽ�����ڰ���Ϸ������·�

		bool m_isXdir = false;//�ж�Z�Ͱ�𿪵�ǽ��X������Y����

		//��������ɨ�赽��ǽ�Ͱ�����
		vector <EditElementHandle*> m_ScanedAllWallsandFloor;

		bool m_strDelete = false;//�����ֽ��ǽ�����ʼ�ֽ��Ƿ�ɾ�������ǽ�ֽ�����

		bool m_endDelete = false;//�����ֽ���ʼ�ֽ��Ƿ�ɾ�������ǽ�ֽ�����

		bool m_isCombineFloor = false;//�Ƿ��Ǻϲ������

		virtual void SetisCombineFloor(bool isCombine) { m_isCombineFloor = isCombine; };

		//bool m_isAddset = true;

		//int m_rebarSetNum = 0;

		//double m_adjustedSpacing;

		//RebarSetP   m_rebarSet = nullptr;
	};
}

