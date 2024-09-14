#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	ǽ���
*	Project:		��ά����ͼ��Ŀ
*	Author:			LiuXiang
*	Date:			2021/02/19
	Version:		V1.0
*	Description:	WallRebarAssembly
*	History:
*	1. Date:		2021/02/19
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/

#include <RebarElements.h>
#include "CommonFile.h"
#include "PITRebarEndType.h"
#include "PITRebarCurve.h"
#include "PITArcSegment.h"
#include "PITRebarAssembly.h"

#define  MaxWallThickness     600          //����ǽ�����ǽ����

class CWallRebarDlg;
class WallRebarAssembly : public PIT::PITRebarAssembly
{
public:
	enum WallType
	{
		STWALL,//ֱǽ
		GWALL, //����ǽ
		ARCWALL,//����ǽ
		ELLIPSEWall,//Բ��ǽ
		STGWALL, //���Ⱥ�ֱǽ
		Other
	};

	enum  ElementType
	{
		WALL,
		FLOOR,
		EOther
	};

	//ǽ����
	struct WallData
	{
		MSElementDescrP downFace = nullptr;//ǽ����
		vector<DSegment3d> vecFontLine;//�����
		vector<DSegment3d> vecBackLine;//�ڲ���
		DVec3d vecdown = DVec3d::From(0,0,1);//�ײ��淨��
		double height = 0;//�߶�
		double thickness = 0;//���
		vector<MSElementDescrP> upfloorfaces;//ǽ�϶������
		vector<MSElementDescrP> downfloorfaces;//ǽ�¶������
		vector<IDandModelref> upfloorID;//ǽ�϶���ID��ref
		vector<IDandModelref> downfloorID;//ǽ�¶���ID��ref
		vector<IDandModelref> wallID;//�ܱ�ê�̵�ǽID��ref
		vector<IDandModelref> floorID;//�ܱ�ê�̵�ǽID��ref
		double upfloorth = 0;//������
		double downfloorth = 0;//�װ���
		vector<MSElementDescrP> cutWallfaces;//�뵱ǰǽ��ͬ���й�����ǽ�����
		DPoint3d vecToWall = DVec3d::From(0, 0, 0);//���泯ǽ�ڵķ���
		void ClearData()
		{
			for(int i = 0;i<upfloorfaces.size();i++)
			{
				mdlElmdscr_freeAll(&upfloorfaces.at(i));
			}
			upfloorfaces.clear();
			for (int i = 0; i < downfloorfaces.size(); i++)
			{
				mdlElmdscr_freeAll(&downfloorfaces.at(i));
			}
			downfloorfaces.clear();
			for (int i = 0; i < cutWallfaces.size(); i++)
			{
				mdlElmdscr_freeAll(&cutWallfaces.at(i));
			}
			cutWallfaces.clear();
			mdlElmdscr_freeAll(&downFace);
		}
	}m_walldata;

	//��������ݣ�ÿһ��ֽ��Ӧһ�����������ݣ�,������������Ϊ�Ѿ��˹�uor_per_mm��ֵ
	struct BarLinesdata
	{
		MSElementDescrP path = nullptr;//·���ߴ�
		MSElementDescrP barline = nullptr;//�����ֽ��ߴ�
		double spacing = 0;//�ֽ���
		double strDis = 0;//��ʼ�����㣬�����ò��汣����+��ʼƫ����+�ֽ�ֱ��/2��
		double endDis = 0;//��ֹ�����㣬�����ò��汣����+��ֹƫ����+�ֽ�ֱ��/2��
		double diameter = 0;//�ֽ�ֱ��
		double extendstrDis = 0;//����ʼ���ӳ�ֵ�����������ֽΪ�²���ȣ�����Ǻ���ֽ�Ϊ��ʼ��ǽ��ȣ�
		double extendendDis = 0;//����ֹ���ӳ�ֵ�����������ֽΪ�ϲ���ȣ�����Ǻ���ֽ�Ϊ��ֹ��ǽ��ȣ�
		double extenddiameter = 0;//����1L�ֽ�ֱ��
		double extstrdiameter = 0;//�װ�1L�ֽ�ֱ��
		bool isInSide = false;//�Ƿ��ڲ���
		DPoint3d vecstr = DPoint3d::From(0, 0, 0);//��ʼ�乳����û���乳�ľ���0��0��0
		DPoint3d vecend = DPoint3d::From(0, 0, 0);//��ֹ�乳����û���乳�ľ���0��0��0
		DPoint3d vecHoleehs = DPoint3d::From(0, 0, 0);//������ֿ׶���׶�ê��ķ���
		double strMG = 0;//���ê�̳���
		double endMG = 0;//�յ�ê�̳���
		double holMG = 0;//�׶�ê�̳���
	};


	    BE_DATA_VALUE(string, wallName)				//ǽ����
		BE_DATA_REFER(BeMatrix, Placement)
		BE_DATA_VALUE(bool, bACCRebar)				//�Ƿ�����������
		BE_DATA_VALUE(UINT, ACCRebarMethod)			//����������ʽ
		BE_DATA_VALUE(double, PositiveCover)			//���汣����
		BE_DATA_VALUE(double, ReverseCover)			//���汣����
		BE_DATA_VALUE(double, SideCover)				//���汣����
		BE_DATA_VALUE(int, RebarLevelNum)			//�ֽ����
	//	BE_DATA_VALUE(int,					IsStaggered)			//�Ƿ񽻴�
		BE_DATA_VALUE(vector<int>, vecDir)					//����,0��ʾx�ᣬ1��ʾz��
		BE_DATA_VALUE(vector<BrString>, vecDirSize)				//�ߴ�
		BE_DATA_VALUE(vector<int>, vecRebarType)			//�ֽ�����
		BE_DATA_VALUE(vector<double>, vecDirSpacing)			//���
		BE_DATA_VALUE(vector<double>, vecStartOffset)			//���ƫ��
		BE_DATA_VALUE(vector<double>, vecEndOffset)			//�յ�ƫ��
		BE_DATA_VALUE(vector<double>, vecLevelSpace)			//��ǰ����
		BE_DATA_VALUE(vector<int>, vecDataExchange)			//���ݽ���
		BE_DATA_VALUE(vector<int>, vecRebarLevel)			// �ֽ��ţ���ǰ�����м��
		BE_DATA_VALUE(vector<double>, vecAngle)			// �Ƕ�
		BE_DATA_VALUE(vector<int>, vecRebarLineStyle)		//�ֽ�����
		BE_DATA_VALUE(vector<int>, vecRebarWeight)			//�ֽ��߿�

		BE_DATA_VALUE(vector<PIT::LapOptions>, vecLapOptions)			//���ѡ��
		BE_DATA_VALUE(vector<vector<PIT::EndType> >, vvecEndType)		//�˲���ʽ
		BE_DATA_VALUE(vector<TwinBarSet::TwinBarLevelInfo >, vecTwinRebarLevel)	//�����
		BE_DATA_VALUE(vector<ElementId>, vecSetId)								//SetId
		BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)     //ǰ���ߵ����е�

public:
	WallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~WallRebarAssembly() {};
	double m_width;
public:
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//ɸѡ��Ŀ׶�
	std::map<EditElementHandle*, EditElementHandle*> m_doorsholes;//�����Ŷ������Ŷ��ϵĸ�ʵ��
	std::vector<DPoint3d>     m_vecRebarPtsLayer;//���׶���ȡǰ�����иֽ������
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//���׶���ȡǰ�����в��������
	std::vector<DPoint3d>     m_vecTieRebarPtsLayer; //������������Ҫ�ĸֽ������

public:
	vector<vector<vector<DPoint3d>> > m_vecRebarStartEnd;	//����˿׶������е�

	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;//δ��ܿ׶������е�
	PIT::PITRebarEndType Hol;
	vector<ElementRefP> m_allLines;//Ԥ����ť���º�����иֽ���
	double lae = 0;
private:
	BE_DATA_VALUE(WallType,				wallType)				//ǽ����
protected:
	void			Init();

protected:
	virtual int		GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Wall; }

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh) { return true; }


public:
	virtual bool	InsertMakeRebars(DgnModelRefP modelRef) { return true; }
	virtual bool	MakeRebars(DgnModelRefP modelRef) { return true; }
	virtual bool	SetWallData(ElementHandleCR eh) { return true; }
	virtual void	InitUcsMatrix() {}

// 	//ͨ�������������
 	virtual bool	MakeACCRebars(DgnModelRefP modelRef) { return true; }
// 	//end

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef) {}

	static WallType JudgeWallType(ElementHandleCR eh);
	static ElementType JudgeElementType(ElementHandleCR eh);
	static bool IsWallSolid(ElementHandleCR eh);
	void SetConcreteData(PIT::Concrete const & concreteData);
	void SetRebarData(vector<PIT::ConcreteRebar> const& vecRebarData);
	void SetRebarEndTypes(vector<PIT::EndType> const& vvecEndTypes);
//	void SetTwinbarInfo(TwinBarSet::TwinBarInfo const& twInfo);
//	void GetTwinbarInfo(TwinBarSet::TwinBarInfo& twInfo);
	void InitRebarSetId();
	void GetConcreteData(PIT::Concrete& concreteData);
	void GetRebarData(vector<PIT::ConcreteRebar>& vecData) const;
	static bool IsSmartSmartFeature(EditElementHandle& eeh);
	void SetTieRebarInfo(TieReBarInfo const& tieRebarInfo);
	const TieReBarInfo GetTieRebarInfo() const;

	void ClearLines();
	//��ǰ�ֽ��0��ʾx�ᣬ1��ʾz��
	int m_nowvecDir;

	//��ǰ��
	int m_nowlevel;

private:
	TieReBarInfo m_tieRebarInfo;
};


class STWallRebarAssembly : public WallRebarAssembly
{
	BE_DATA_VALUE(STWallGeometryInfo, STwallData)			//STWall��������
//	BE_DATA_VALUE(bool, IsTwinrebar)			//�ڻ�rebarcuveʱ���ж��Ƿ�Ϊ�����
public :

	CVector3D m_VecX = CVector3D::From(1, 0, 0);//�ֲ�����ϵ�µ�X����
	CVector3D m_VecZ = CVector3D::From(0, 0, 1);//�ֲ�����ϵ�µ�Z����

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
	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebar, double xPos, double height, double startOffset,
		double endOffset, PIT::PITRebarEndTypes &endType, CMatrix3D const& mat,bool istwin = false);
	bool makeRebarCurve_Laption(RebarCurve& rebar, double xPos, double height, double bendRadius,
		double bendLen, RebarEndTypes const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool istwin = false);
	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag=true);
	bool makaRebarCurve(const vector<DPoint3d>& linePts, double extendStrDis, double extendEndDis, double diameter, double strMoveDis, double endMoveDis, bool isInSide,
	                    const PIT::PITRebarEndTypes& endTypes, vector<PIT::PITRebarCurve>& rebars,
	                    std::vector<EditElementHandle*> upflooreehs, std::vector<EditElementHandle*> downflooreehs, std::vector<EditElementHandle*> Walleehs,
						std::vector<EditElementHandle*>alleehs);


	bool		m_isPushTieRebar; // �Ƿ�push��������ĸֽ����

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::STWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"STWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"STWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	
protected:
	/*

	*/
	RebarSetTag* MakeRebars(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, 
		double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, 
		CMatrix3D const& mat, const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel,int level, int grade, int DataExchange, DgnModelRefP modelRef, int rebarLineStyle,
		int rebarWeight);
//	RebarSetTag* MakeRebars_ACC(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat, const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel, DgnModelRefP modelRef);
	RebarSetTag* MakeRebars_Laption(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, PIT::LapOptions const& lapOptions, vector<PIT::EndType> const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

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
		,pWall(NULL)
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
	* @desc:		���ݱ�������ƶ��������¼�������ֽ��ߴ�
	* @param[in/out]	data ���������
	* @return	MSElementDescrP �µĸֽ��ߴ�
	* @author	Hong ZhuoHui
	* @Date:	2023/09/13
	*/
	void ReCalBarLineByCoverAndDis(BarLinesdata& data);

	/*
	* @desc:		����ʵ�������׶���Z��ǽ��
	* @param[in]	wallEeh ǽ
	* @param[out]	holes �׶�
	* @author	Hong ZhuoHui
	* @Date:	2023/09/13
	*/
	void CalHolesBySubtract(EditElementHandleCR wallEeh, std::vector<EditElementHandle*>& holes);

	/*
	* @desc:		����endType
	* @param[in]	data �ֽ�������
	* @param[in]	sizeKey
	* @param[out]	pitRebarEndTypes �˲���ʽ
	* @param[in]	modelRef
	* @author	Hong ZhuoHui
	* @Date:	2023/09/19
	*/
	void CalRebarEndTypes(const BarLinesdata& data, BrStringCR sizeKey,
		PIT::PITRebarEndTypes& pitRebarEndTypes, DgnModelRefP modelRef);

	/*
	* @desc:		���ݶ��װ����¼�����������
	* @param[in]	strPt �ֽ������
	* @param[in]	endPt �ֽ����ص�
	* @param[in]	strMoveLen �����ƶ����룬�����乳�Ǳ����㣬���乳�Ǳ�����+�ֽ�뾶
	* @param[in]	endMoveLen �յ���ƶ����룬�����乳�Ǳ����㣬���乳�Ǳ�����+�ֽ�뾶
	* @param[out]	extendStrDis �����������
	* @param[out]	extendEndDis �յ���������
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


class GWallRebarAssembly : public WallRebarAssembly
{
	BE_DATA_VALUE(bool, IsTwinrebar)			//�ڻ�rebarcuveʱ���ж��Ƿ�Ϊ�����
	enum GWallType
	{
		LineWall,			//����ǽ
		ArcWall,			//����ǽ
		LineAndArcWALL,		//���߻���ǽ
		Custom,				//�Զ�����������ǽ
	}m_GWallType;
public:
	DPoint3d m_LineNormal;
	STWallGeometryInfo m_STwallData;
	double m_angle_left;
	double m_angle_right;
	double m_sidecover;
	~GWallRebarAssembly()
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
	bool makeLineWallRebarCurve(RebarCurve& rebar, int dir, vector<CPoint3D> const& vecRebarVertex, double bendRadius, double bendLen, RebarEndTypes const& endTypes, CVector3D const& endNormal, CMatrix3D const& mat);

	bool makeArcWallRebarCurve(RebarCurve& rebar, double xPos, double height, double startOffset, double endOffset, double bendRadius, double bendLen, RebarEndTypes const& endTypes, CVector3D const& endNormal, CMatrix3D const& mat) {};

	bool makeLineAndArcWallRebarCurve(RebarCurve& rebar, double xPos, double height, double startOffset, double endOffset, double bendRadius, double bendLen, RebarEndTypes const& endTypes, CVector3D const& endNormal, CMatrix3D const& mat) {};

	void  CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::GWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"GWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"GWall Rebar"; }
//	virtual bool        OnDoubleClick() override;
//	virtual bool        Rebuild() override;

protected:
	/*

	*/
	RebarSetTag* MakeRebars_Transverse(ElementId& rebarSetId, BrStringCR sizeKey, vector<CPoint3D> vecPt,double spacing, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

	RebarSetTag* MakeRebars_Longitudinal(ElementId& rebarSetId, BrStringCR sizeKey, double &xDir, const vector<double> height, double spacing, double startOffset, double endOffset, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

	
	void JudgeGWallType(ElementHandleCR eh);

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef);

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);

	virtual void CalculateUseHoles(DgnModelRefP modelRef);
	bool GetUcsAndStartEndData(int index, double thickness,DgnModelRefP modelRef,bool isSTGWALL = false);
	void GetMaxThickness(DgnModelRefP modelRef, double& thickness);
	RebarSetTag* MakeRebars
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		double              xLen,
		double              height,
		double              spacing,
		double              startOffset,
		double              endOffset,
		vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
		vector<CVector3D> const& vecEndNormal,
		CMatrix3D const&    mat,
		TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
		int level,
		int grade,
		int DataExchange,
		bool				bTwinbarLevel,
		DgnModelRefP        modelRef,
		bool  drawlast = true
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
		bool isTwin = false
	);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(GWallRebarAssembly, RebarAssembly)

public:
public:
	GWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) 
		:WallRebarAssembly(id, modelRef)
#ifdef PDMSIMPORT
		, pWall(NULL)
#endif
	{};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);

	void SetLinePts(const map<int, vector<DPoint3d>> &pts) { m_vecLinePts = pts; }
	map<int, vector<DPoint3d>> GetLinePts() const { return m_vecLinePts; }
	void SetIsPushTieRebar(bool bl) { m_isPushTieRebar = bl; }
	bool GetIsPushTieRebar() const { return m_isPushTieRebar; }
	void SetGWallHeight(double height) { m_GWallData.height = height; }
	double GetGWallHeight() const { return m_GWallData.height; }

private:

	struct GWallGeometryInfo
	{
		vector<DPoint3d> vecPositivePt;		//����ǽ:����ױ߶�������;����ǽ:����ױ�����յ��Բ��ȷ��һ��������һ��ΪԲ�ļ�������յ㣬�ڶ���ΪԲ�ļ�����һ�������յ�͵�ǰ�����յ㣬�Դ�����
		vector<DPoint3d> vecReversePt;		//����ǽ:����ױ߶�������;����ǽ:����ױ�����յ��Բ��ȷ��һ��������һ��ΪԲ�ļ�������յ㣬�ڶ���ΪԲ�ļ�����һ�������յ�͵�ǰ�����յ㣬�Դ�����
		vector<double> vecLength;			//����ǽ:����ױ߱߳�;����ǽ:����ÿһ�λ��İ뾶
		double height;						//ǽ��
		double thickness;					//ǽ��
	}m_GWallData;


	map<int, vector<DPoint3d>> m_vecLinePts;//����ǰ���߶εĵ㣬��һ����ʶΪ�ڼ���

	bool m_isPushTieRebar;

#ifdef PDMSIMPORT
	GWallComponent *pWall;
#endif
};

class ArcWallRebarAssembly : public WallRebarAssembly
{
public:
	CWallRebarDlg *pArcWallDoubleRebarDlg;
private:

	bool CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);

//	bool makeArcWallRebarCurve(RebarCurve& rebar, double xPos, double height, double startOffset, double endOffset, double bendRadius, double bendLen, RebarEndTypes const& endTypes, CVector3D const& endNormal, CMatrix3D const& mat);

	bool makeLineRebarCurve
	(
		vector<PIT::PITRebarCurve>& 	rebar,
		PIT::ArcSegment				arcSeg,
		double					dLen,
		double                  space,
		double					startOffset,
		double					endOffset,
		PIT::PITRebarEndTypes&		endTypes
	);

	void CalculateUseHoles(DgnModelRefP modelRef);

	bool makeArcWallRebarCurve
	(
		vector<PIT::PITRebarCurve>& 	rebar,
		PIT::ArcSegment				arcSeg,
		double                  space,
		double					startOffset,
		double					endOffset,
		PIT::PITRebarEndTypes&		endTypes
	);

	RebarSetTag* MakeRebars_Line
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		PIT::ArcSegment			arcSeg,
		double              dLen,
		double              spacing,
		double              startOffset,
		double              endOffset,
		vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
		vector<CVector3D> const& vecEndNormal,
		TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
		int level,
		int grade,
		int DataExchange,
		bool				bTwinbarLevel,
		DgnModelRefP        modelRef
	);

	RebarSetTag* MakeRebars_Arc
	(
		ElementId&          rebarSetId,
		BrStringCR          sizeKey,
		PIT::ArcSegment			arcSeg,
		double              spacing,
		double              startOffset,
		double              endOffset,
		vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
		vector<CVector3D> const& vecEndNormal,
		TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
		int level,
		int grade,
		int DataExchange,
		bool				bTwinbarLevel,
		DgnModelRefP        modelRef
	);

	//��ȡ�µĻ��ߺ͸ֽ���
	void GetNewArcAndSpacing(PIT::ArcSegment oldArc, PIT::ArcSegment& newArc, double angle, double& newSpacing);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::ArcWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);
	


public:
	virtual bool	AnalyzingWallGeometricDataARC(ElementHandleCR eh ,PIT::ArcSegment &arcFront, PIT::ArcSegment &arcBack);
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(ArcWallRebarAssembly, RebarAssembly)

public:
	ArcWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL)
		:WallRebarAssembly(id, modelRef)
	{
		pArcWallDoubleRebarDlg = nullptr;
		memset(&m_ArcWallData, 0, sizeof(ArcWallGeometryInfo));
	};

	~ArcWallRebarAssembly()
	{

	};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);

private:
	struct ArcWallGeometryInfo
	{
		PIT::ArcSegment	OuterArc;		//
		PIT::ArcSegment	InnerArc;		//
		double height;				//ǽ��
		double thickness;			//ǽ��
	}m_ArcWallData;

	bool		m_isPushTieRebar; // �Ƿ�push��������ĸֽ����
	PIT::ArcSegment m_outMaxArc;	//�⻡�����
	double m_sideCoverAngle = 0;	//������Ƕ�
};

class ELLWallRebarAssembly : public WallRebarAssembly
{
public:
	CWallRebarDlg *pEllWallDoubleRebarDlg;
	virtual ~ELLWallRebarAssembly()
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
	// ���ɵ��
	RebarSetTag* MakeRebar_Vertical
	(
		ElementId& rebarSetId,
		BrString sizeKey,
		DgnModelRefP modelRef,
		double startOffset,  // ��ʼƫ��
		double endOffset,    // �յ�ƫ��
		double spacing,		 // ���
		double dRoundRadius, // Բ�İ뾶
		double rebarLen,		 // �ֽ��
		int level,
		int grade,
		int DataExchange,
		bool isTwinRebar = false // �Ƿ��ǲ���
	);

	RebarSetTag* MakeRebar_Round
	(
		ElementId& rebarSetId,
		BrString sizeKey,
		DgnModelRefP modelRef,
		double startOffset, // ��ʼƫ��
		double endOffset,   // �յ�ƫ��
		double spacing,		// ���
		double dRoundRadius,// Բ�İ뾶
		int level,
		int grade,
		int DataExchange,
		bool isTwinRebar = false // �Ƿ��ǲ���
	);

	bool CalculateRound(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt, int nStep = 0);

	bool CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);

	bool makeArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP mscArc, PIT::PITRebarEndTypes& endTypes, const vector<DPoint3d>& pts);

	bool makeBreakArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP mscArc, PIT::PITRebarEndTypes& endTypes, const vector<DPoint3d>& pts);

	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d ptStr, double dRebarLength);

	bool makeRoundRebarCurve(vector<PIT::PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d	centerPoint, double dRoundRadius);

	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

	vector<CurveVectorPtr> CreateBreakArcRange(const Transform& tran);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::ELLWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ELLWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ELLWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(ELLWallRebarAssembly, RebarAssembly)


public:
	ELLWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		WallRebarAssembly(id, modelRef)
#ifdef PDMSIMPORT
		, pWall(NULL)
#endif
	{
		pEllWallDoubleRebarDlg = NULL;
		m_vecRebarStartEnd.clear();

		m_ELLWallData.dRadiusOut = 0.0;
		m_ELLWallData.dRadiusInn = 0.0;
		m_ELLWallData.dHeight = 0.0;
		m_ELLWallData.centerpt = DPoint3d::From(0, 0, 0);
		m_ELLWallData.ArcDPs[0] = DPoint3d::From(0, 0, 0);
		m_ELLWallData.ArcDPs[1] = DPoint3d::From(0, 0, 0);
	};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);

	void CalculateUseHoles(DgnModelRefP modelRef);

	void SetBreakAngelData(vector<PIT::BreakAngleData> data) { m_vecBreakData = data; }

private:
	struct ELLWallGeometryInfo
	{
		double   dRadiusOut;	  // ��Բ�뾶
		double   dRadiusInn;	  // ��Բ�뾶
		double	 dHeight;		  // �߶�
		DPoint3d centerpt;		  // �������ĵ�
		DPoint3d ArcDPs[2];		  // �������յ�
		UInt16       type;         //Ԫ������

	}m_ELLWallData;

	struct LevelInfo
	{
		int rebarLevel;
		double LevelSpacing;
	};

	struct TwinRebarDataTmp
	{
		int rebarNum;
		double spacing;
		double diameter;
	}m_reabrTwinData;

	vector<PIT::ArcSegment> m_vecArcSeg; // Ԥ�����θֽ��¼
	vector<PIT::BreakAngleData> m_vecBreakData;	//�Ͽ�����
	map<int, vector<EditElementHandle*>> m_vecUseHoles; //���ǶȵĿ׶�

#ifdef PDMSIMPORT
private:
	ELLWallComponent *pWall;
#endif
};

/*
* ClassName:	ֱ������ǽ���ϲ�ֱǽ��
* Description:	
* Author:		hzh
* Date:			2022/11/08*/
class STGWallRebarAssembly : public GWallRebarAssembly
{
public:
	//��ǰǽ��������ǽ���λ�ù�ϵ
	enum WallPos
	{
		Horizontal = 0, //ƽ��
		IN_WALL,		//�ڰ�
		OUT_WALL		//��͹
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
	* @desc:	��ȡǽ��ǰ������߶�
	* @param[in]	eeh	ǽ
	* @param[out]	frontLines ǰ�����߶�
	* @param[out]	backLines ������߶�
	* @return	�ɹ�true��ʧ��false
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
		vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
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
	* @desc:	��ʼ��ƽ�к����Ϣ	
	* @author	hzh
	* @Date:	2022/11/08
	*/
	void InitLevelHoriInfos();

	/*
	* @desc:	����ƽ�к����Ϣ
	* @param[in]	level ������ڲ�	
	* @param[in]	tag ����rebarsettag
	* @param[in]	rightWall �ò�����һ��ǽ�Ĺ�ϵ
	* @param[out]	levelHoriTags ��ƽ�й�ϵ����ĸֽ��
	* @remark	�����Ƿ�ƽ�з��飬��ÿһ���������飬ÿһ��������ƽ�еĺ����ǿ��Ժϲ�
	* @author	hzh
	* @Date:	2022/11/08
	*/
	void CalcLevelHoriInfos(int level, RebarSetTag* tag, WallPos rightWall,
		map<int, vector<vector<RebarSetTag*>>>& levelHoriTags);

	/*
	* @desc:	�޸ĺ��
	* @param[in]	levelHoriTags 	��ƽ�й�ϵ����ĸֽ����Ϣ
	* @param[in]	levelName ������������߲���	
	* @remark	���ɺϲ��ĺ����кϲ�����
	* @author	hzh
	* @Date:	2022/11/08
	*/
	void UpdateHoriRebars(const map<int, vector<vector<RebarSetTag*>>>& levelHoriTags, const CString& levelName);
	
	/*
	* @desc:	�޸ĸֽ�
	* @param[in]	zRebars ����z����ĺ����ɺϲ��ĸֽ�	
	* @param[in]	levelName ������������߲���	
	* @author	hzh
	* @Date:	2022/11/08
	*/
	void UpdateRebars(const map<int, vector<ElementRefP>>& zRebars, const CString& levelName);
	
	/*
	* @desc:	����ֽ��ܿ׶���Ķ˵�
	* @param[in]	strPt �ֽ�ԭ��ʼ��
	* @param[in]	endPt �ֽ�ԭ�յ�
	* @return	map<int, DPoint3d> ��ܿ׶���ĵ꣬����Ϊ�˵�
	* @author	hzh
	* @Date:	2022/11/08
	*/
	map<int, DPoint3d> CalcRebarPts(DPoint3d& strPt, DPoint3d& endPt);

	/*
	* @desc:	����ǽ��ֶ�֮��Ĺ�ϵ������ÿһ����ǽ������
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
	vector<LinePt> m_frontLinePts;//����ǰ���߶εĵ�
	vector<LinePt> m_backLinePts;//��������߶εĵ�
	map<int, bool> m_levelIsHori;	//�ֽ���Ƿ�ƽ��
	map<int, vector<vector<RebarSetTag*>>> m_levelHoriTags; //�ֽ�㰴ƽ�з���
	map<int, vector<vector<RebarSetTag*>>> m_twinLevelHoriTags; //����ֽ�㰴ƽ�з���
	map<int, vector<int>> m_rangeIdxWalls;	//�����ǽ
};