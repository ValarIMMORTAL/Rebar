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
	std::vector<EditElementHandle*> m_around_ele_holeEehs;//��ȡ��ǰѡ��ʵ����ΧԪ�صĿ׶�
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

	void interPtsSort(vector<DPoint3d> &interPts, const DPoint3d &originPt);
	//��ȡ����
	double GetFloorThickness(EditElementHandleR Eleeh);
	bool get_value1(vector<ElementId> vec_walls, EditElementHandle& tmpeeh);
	void GetLeftRightWallFaces(WallRebarAssembly::WallData& walldata, EditElementHandleR eeh, string wallname);
	void GetUpDownFloorFaces(WallRebarAssembly::WallData& walldata, EditElementHandleR eeh);
	double GetDownFaceVecAndThickness(MSElementDescrP downFace, DPoint3d& Vec);
	void ExtendLineByFloor(vector<MSElementDescrP>& floorfaces, vector<IDandModelref>& floorRf, DPoint3d& ptstr, DPoint3d& ptend, DPoint3d& vecLine, double thick, double& Dimr, DPoint3d vecOutwall);
	double GetExtendptByWalls(DPoint3d& str, DPoint3d str_1, double thick, MSElementDescrP& Wallfaces, vector<MSElementDescrP>& cutWallfaces,
		DPlane3d plane, DPoint3d cpt, double Lae, double L0, DPoint3d& MGvec);
	bool CalculateBarLineDataByFloor(vector<MSElementDescrP>& floorfaces, vector<IDandModelref>& floorRf, DPoint3d& ptstr, DPoint3d& ptend, DPoint3d& vecLine, double thick,
		DPoint3d vecOutwall, bool& isInside, double& diameter);
	void GetCutPathLines(vector<WallRebarAssembly::BarLinesdata>& barlines, double sidespacing, double diameter,
		MSElementDescrP& path, vector<MSElementDescrP>& cutWallfaces, MSElementDescrP downface, double height);
	void GetMovePath(MSElementDescrP& pathline, double movedis, MSElementDescrP downface);
	void GeneratePathPoints(const DPoint3d& ptStr, const DPoint3d& ptEnd, int numPoints, vector<DPoint3d>& points);
	int CountPointsInElement(EditElementHandleP eeh, const vector<DPoint3d>& points);
	void ExtendLineString(MSElementDescrP& linedescr, double dis);
	MSElementDescrP GetLines(vector<DSegment3d>& lines);
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
	//��ȡ��ǰʵ��z���������z����С����
	static MSElementDescrP GetElementDownFace(ElementHandleCR eeh, EditElementHandleR DownFace, double* tHeight);
	
	void ClearLines();
	//��ǰ�ֽ��0��ʾx�ᣬ1��ʾz��
	int m_nowvecDir;

	//��ǰ��
	int m_nowlevel;

private:
	TieReBarInfo m_tieRebarInfo;
};