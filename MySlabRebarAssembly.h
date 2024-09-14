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
class MySlabRebarAssembly : public RebarAssembly                   //ǽ���ĸֽ���Ϣ
{
public:
		BE_DATA_REFER(BeMatrix, Placement)              //��ǰ�ֲ�����ԭ��
		BE_DATA_VALUE(bool, bACCRebar)				//�Ƿ�����������
		BE_DATA_VALUE(UINT, ACCRebarMethod)			//����������ʽ
		BE_DATA_VALUE(double, PositiveCover)			//���汣����
		BE_DATA_VALUE(double, ReverseCover)			//���汣����
		BE_DATA_VALUE(double, SideCover)				//���汣����
		BE_DATA_VALUE(int, RebarLevelNum)			//�ֽ����
//		BE_DATA_VALUE(char*, SlabRebarMethod)			//��ʽ
//		BE_DATA_VALUE(int, Twinbars)				//�Ƿ�ʹ�ò���
//		BE_DATA_VALUE(int, IsStaggered)			//�Ƿ񽻴�
		BE_DATA_VALUE(vector<int>, vecDir)					//����,0��ʾx�ᣬ1��ʾz��
		BE_DATA_VALUE(vector<BrString>, vecDirSize)				//�ߴ�
		BE_DATA_VALUE(vector<int>, vecRebarType)			//�ֽ�����
		BE_DATA_VALUE(vector<double>, vecDirSpacing)			//���
		BE_DATA_VALUE(vector<double>, vecStartOffset)			//���ƫ��
		BE_DATA_VALUE(vector<double>, vecEndOffset)			//�յ�ƫ��
		BE_DATA_VALUE(vector<double>, vecLevelSpace)			//��ǰ����
		BE_DATA_VALUE(vector<int>, vecRebarLevel)				// �ֽ������ǰ���м��й�
		BE_DATA_VALUE(vector<int>, vecDataExchange)			//���ݽ���
		BE_DATA_REFER(int, staggeredStyle)					// ��������
		//BE_DATA_VALUE(vector<int>, vecRebarColor)			//�ֽ���ɫ
		BE_DATA_VALUE(vector<int>, vecRebarLineStyle)		//�ֽ�����
		BE_DATA_VALUE(vector<int>, vecRebarWeight)			//�ֽ��߿�

		BE_DATA_VALUE(vector<PIT::LapOptions>, vecLapOptions)			//���ѡ��
		BE_DATA_VALUE(vector<TwinBarSet::TwinBarLevelInfo >, vecTwinRebarLevel)	//�����
		BE_DATA_VALUE(vector<vector<PIT::EndType> >, vvecEndType)		//�˲���ʽ
		BE_DATA_VALUE(vector<ElementId>, vecSetId)								//rebarelementSet��ÿ��Id
		BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)     //ǰ���ߵ����е�
		BE_DATA_VALUE(Abanurus_PTRebarData, Abanurus_PTRebarData) //֧�յ������

public:
	MySlabRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~MySlabRebarAssembly() {};
	
public:
	vector<vector<vector<DPoint3d>> > m_vecAllRebarStartEnd;
	map<int, vector<vector<DPoint3d>>> m_vecAllRebarStartEndMap;
	vector<ElementRefP> m_allLines;//Ԥ����ť���º�����иֽ���
	void ClearLines();
	bool CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);
public:
	std::string m_strElmName;
	double m_height;
	int m_strSlabRebarMethod;//��ʽ    0:Ĭ�ϣ�1��������2������
	int m_nowvecDir;//��ǰ�ֽ��0��ʾx�ᣬ1��ʾz��
private:
	BE_DATA_VALUE(SlabType, slabType)				//ǽ����
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
	BE_DATA_VALUE(STWallGeometryInfo, STwallData)			//STWall��������
		BE_DATA_VALUE(bool, IsTwinrebar)			//�ڻ�rebarcuveʱ���ж��Ƿ�Ϊ�����
public:
	DgnModelRefP m_model;
	std::vector<EditElementHandle*> m_Noweeh;
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//ɸѡ��Ŀ׶�

	EditElementHandle* m_pOldElm;  // ����׶��İ�
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
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//���׶���ȡǰ�����в��������
	std::vector<DPoint3d>     m_vecTieRebarPtsLayer; //������������Ҫ�ĸֽ������
	RebarSetTagArray rsetTags;
	CSlabRebarDlg * pSlabDoubleRebarDlg;

	bool m_isAbanurus = false;//�Ƿ���֧�����

	//�ȵ����Ӧ����
	struct LDFloorData
	{
		DPoint3d oriPt = { 0 };//�ȵ�����С��
		double Xlenth = 0;//ת����XOYƽ������X����
		double Ylenth = 0;
		double Zlenth = 0;//���
		DVec3d Vec = DVec3d::From(0, 0, 1);//Ĭ��ȡZ�᷽��
		MSElementDescrP facedes = nullptr;//����
		MSElementDescrP upfaces[40];//��¼�嶥ǽ����ཻ����
		int upnum = 0;//��¼����ǽ�����
		MSElementDescrP downfaces[10];//��¼���ǽ����ཻ����
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

protected:
	//	void  CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef);
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 3; }//�ж����ĸ���
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"LDSlab Rebar"; }//����
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"LDSlab Rebar"; }
	virtual bool        OnDoubleClick() override;//˫�����ٴγ��ֶԻ���
	virtual bool        Rebuild() override;//�����޸ĺ󣬵������µ����ֽ�

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
		vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
		vector<CVector3D> const& vecEndNormal,
		CMatrix3D const&    mat,
		TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
		bool				bTwinbarLevel,
		int level,
		int grade,
		int DataExchange,
		DgnModelRefP        modelRef
	);
	RebarSetTag* STSlabRebarAssembly::MakeRebars_Abanurus//֧�����
	(
		ElementId&          rebarSetId,
		int level,//���㣬���������Դ�Ϊ0��1��2��3
		vector<vector<DPoint3d>> const& rebarpts,
		PITRebarEndTypes   &PITendTypes,	//�洢���˲����յ�˲�����
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
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }//��������
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

	/*֧�����*/
	bool m_isBend = false;//�Ƿ���Ҫ��ê��ê���ܳ���ΪLae

	// �ֽ����λ��֧����ֽ��յ�λ�ڰ���
	map <int, vector<DPoint3d>> m_mapRroundPts;//����֧����Χ���ĸ��ֽ����ʼ��,���£����£����ϣ����ϣ�int����Ϊ0��1��2��3 

	map <int, vector<vector<DPoint3d>>> m_mapAllRebarpts;//�������еĸֽ���ʼ��,�£��ң��ϣ���һ�Ÿֽint����Ϊ0��1��2��3
														 //���ܵĵ��������������ţ����Ҳ�����
	
	map <int, vector<vector<DPoint3d>>> m_mapActualAllRebarpts;//ʵ�ʵĸֽ�ĵ��λ�ã��������빿���غ�
	/// @brief ɨ��range�ڵ�����Ԫ��
	/// @param range 
	/// @param filter ���˺���
	/// @return 
	std::vector<ElementHandle> scan_elements_in_range(const DRange3d &range, std::function<bool(const ElementHandle &)> filter);
	
	bool is_Floor(const ElementHandle &element);//�ж��Ƿ��ǰ�

	vector<ElementHandle> m_downFloor;//����֧������İ�
	
	double get_lae()const;//��ȡLae����

	double m_lae = 0.0;//ê�̳���

	double m_floor_Thickness = 0.0;//֧������İ�ĺ��

	double m_HSpace = 0.0;//��������

	double m_VSpace = 0.0;//���������

	//ͨ��֧��������İ��Range����ֽ������յ�
	void CalculateRebarPts();

	//�ƶ�����λ�ã�ʹ�ò��빿��������غ�
	void MoveRebarPts();
};
