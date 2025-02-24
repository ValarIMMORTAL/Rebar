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
#include "../CommonFile.h"
#include "../PITRebarEndType.h"
#include "../PITRebarCurve.h"
#include "../PITArcSegment.h"
#include "../PITRebarAssembly.h"
#define  MaxWallThickness     600          //����ǽ�����ǽ����

class CWallRebarDlgNew;
class WallRebarAssemblyNew : public PIT::PITRebarAssembly
{
public:
	enum WallType
	{
		STWALL,
		GWALL,
		ARCWALL,
		ELLIPSEWall,
		Other
	};

	enum  ElementType
	{
		WALL,
		FLOOR,
		EOther
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

		BE_DATA_VALUE(vector<PIT::LapOptions>, vecLapOptions)			//���ѡ��
		BE_DATA_VALUE(vector<vector<PIT::EndType> >, vvecEndType)		//�˲���ʽ
		BE_DATA_VALUE(vector<TwinBarSet::TwinBarLevelInfo >, vecTwinRebarLevel)	//�����
		BE_DATA_VALUE(vector<ElementId>, vecSetId)								//SetId
		BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts)     //ǰ���ߵ����е�
		BE_DATA_VALUE(CutRebarInfo, stCutRebarInfo)		 // �и�ֽ���Ϣ
		BE_DATA_VALUE(bool, isReserveCut)				 // �Ƿ����и�ֽ�

		BE_DATA_VALUE(std::vector<CNCutRebarInfo>, vecCutInfo)		// �ض���Ϣ

public:
	WallRebarAssemblyNew(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~WallRebarAssemblyNew() {};
	double m_width;
	std::vector<ElementRefP> m_selectrebars;

public:
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//ɸѡ��Ŀ׶�
	std::map<EditElementHandle*, EditElementHandle*> m_doorsholes;//�����Ŷ������Ŷ��ϵĸ�ʵ��
	std::vector<DPoint3d>     m_vecRebarPtsLayer;//���׶���ȡǰ�����иֽ������
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//���׶���ȡǰ�����в��������
	std::vector<DPoint3d>     m_vecTieRebarPtsLayer; //������������Ҫ�ĸֽ������

public:
	vector<vector<DSegment3d> > m_vecRebarStartEnd;	//����˿׶������е�

	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;//δ��ܿ׶������е�

	vector<ElementRefP> m_allLines;//Ԥ����ť���º�����иֽ���

	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

	RebarAssembly* m_pOldRebaras;

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

	double floadToInt(double dSrc);

	/***********************************************************************************************
	*****					�и����߸ֽ�
	*****	ptStr		:	ֱ�߸ֽ����
	*****	ptEnd		��	ֱ�߸ֽ��յ�
	*****	vecSplit	:	�ָ���� �� ��������
	************************************************************************************************/
	virtual bool CutLineRebarCurve(std::vector<PIT::PITRebarCurve>& rebars, PIT::PITRebarEndTypes& endTypes, DPoint3d& ptStr, DPoint3d& ptEnd, vector<double>& vecSplit);

	/***********************************************************************************************
	*****	ptStr		:	ֱ�߸ֽ����
	*****	ptEnd		��	ֱ�߸ֽ��յ�
	*****	vecSplit	:	�ָ���� �� ��������
	************************************************************************************************/
	bool CalaLineRebarCutPoint(DPoint3dCR ptStr, DPoint3dCR ptEnd, vector<double>& vecSplit, double diameterTol, double preLength, bool bFlag);

	void SetCutLenIndex()
	{
		double uor_per_m = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter();
		std::vector<double> m_vecDouble;
		m_vecDouble.push_back(m_stCutRebarInfo.dCutLength1);
		m_vecDouble.push_back(m_stCutRebarInfo.dCutLength2);
		m_vecDouble.push_back(m_stCutRebarInfo.dCutLength3);
		
		sort(m_vecDouble.begin(), m_vecDouble.end());

		m_CutLenIndex[0] = m_vecDouble.at(0) * uor_per_m;
		m_CutLenIndex[1] = m_vecDouble.at(1) * uor_per_m;
		m_CutLenIndex[2] = m_vecDouble.at(2) * uor_per_m;
	}

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

private:
	TieReBarInfo m_tieRebarInfo;

	double m_CutLenIndex[3];
};


class STWallRebarAssemblyNew : public WallRebarAssemblyNew
{
	BE_DATA_VALUE(STWallGeometryInfo, STwallData)			//STWall��������
//	BE_DATA_VALUE(bool, IsTwinrebar)			//�ڻ�rebarcuveʱ���ж��Ƿ�Ϊ�����
public :
	DPoint3d m_LineNormal;
	CWallRebarDlgNew *pWallDoubleRebarDlg;
	double m_angle_left;
	double m_angle_right;
	virtual ~STWallRebarAssemblyNew()
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
		double endOffset, PIT::PITRebarEndTypes &endType, CMatrix3D const& mat, double diameter, bool isStrLineCut, bool istwin = false);
	bool makeRebarCurve_Laption(RebarCurve& rebar, double xPos, double height, double bendRadius,
		double bendLen, RebarEndTypes const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool istwin = false);
	
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
		CMatrix3D const& mat, const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel,int level, int grade, int DataExchange, DgnModelRefP modelRef);
//	RebarSetTag* MakeRebars_ACC(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat, const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel, DgnModelRefP modelRef);
	RebarSetTag* MakeRebars_Laption(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, PIT::LapOptions const& lapOptions, vector<PIT::EndType> const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(STWallRebarAssemblyNew, RebarAssembly)


public:
	STWallRebarAssemblyNew(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		WallRebarAssemblyNew(id, modelRef)
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

#ifdef PDMSIMPORT
private:	
	STWallComponent *pWall;
#endif
};


class GWallRebarAssemblyNew : public WallRebarAssemblyNew
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
	~GWallRebarAssemblyNew()
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

	void CalculateUseHoles(DgnModelRefP modelRef);
	bool GetUcsAndStartEndData(int index, double thickness,DgnModelRefP modelRef);
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
	BE_DECLARE_VMS(GWallRebarAssemblyNew, RebarAssembly)

public:
public:
	GWallRebarAssemblyNew(ElementId id = 0, DgnModelRefP modelRef = NULL)
		:WallRebarAssemblyNew(id, modelRef)
#ifdef PDMSIMPORT
		, pWall(NULL)
#endif
	{};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);
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

class ArcWallRebarAssemblyNew : public WallRebarAssemblyNew
{
public:
	CWallRebarDlgNew *pArcWallDoubleRebarDlg;
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

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::ArcWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ArcWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);

public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(ArcWallRebarAssemblyNew, RebarAssembly)

public:
	ArcWallRebarAssemblyNew(ElementId id = 0, DgnModelRefP modelRef = NULL)
		:WallRebarAssemblyNew(id, modelRef)
	{
		pArcWallDoubleRebarDlg = nullptr;
		memset(&m_ArcWallData, 0, sizeof(ArcWallGeometryInfo));
	};

	~ArcWallRebarAssemblyNew()
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
};

class ELLWallRebarAssemblyNew : public WallRebarAssemblyNew
{
public:
	CWallRebarDlgNew *pEllWallDoubleRebarDlg;
	virtual ~ELLWallRebarAssemblyNew()
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

	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d ptStr, double dRebarLength);

	bool makeRoundRebarCurve(vector<PIT::PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d	centerPoint, double dRoundRadius);

	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);
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
	BE_DECLARE_VMS(ELLWallRebarAssemblyNew, RebarAssembly)


public:
	ELLWallRebarAssemblyNew(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		WallRebarAssemblyNew(id, modelRef)
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

private:
	struct ELLWallGeometryInfo
	{
		double   dRadiusOut;	  // ��Բ�뾶
		double   dRadiusInn;	  // ��Բ�뾶
		double	 dHeight;		  // �߶�
		DPoint3d centerpt;		  // �������ĵ�
		DPoint3d ArcDPs[2];		  // �������յ�
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

#ifdef PDMSIMPORT
private:
	ELLWallComponent *pWall;
#endif
};
