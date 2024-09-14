#pragma once
#include "_USTATION.h"
#include "Public.h"
#include "LEVELFILE.h"
#include "GalleryIntelligentRebarids.h"
#include "RebarCommonData.h"
#include "XmlManager.h"
namespace PIT
{
	enum PITRebarAssemblyEnum
	{
		Wall = 2,
		STWall,
		GWall,
		ArcWall,
		ELLWall,
		PITSTWall,
		PITGWall,
		PTIArcWall,
		Face,
		Plane,
		CamberedSurface,
		Parapet,
		Rainshed,
	};
}

struct ArcRebar //����״�����Ļ�����Ϣ
{
	DPoint3d centerpt;
	DPoint3d ptStart;
	DPoint3d ptMid;	//�м�ĵ�
	DPoint3d ptEnd;
	double radius;
	double ArcLen;
	double slabHight;
	bool isCircle;	//�Ƿ���Բ
};

struct RebarPoint
{
	RebarPoint()
	{
		Layer = 0;
		vecDir = 0;
		sec = 0;
		ptstr = ptend = ptmid = DPoint3d::From(0, 0, 0);
	}
	int Layer; // �Ǹ���ڼ��� �磺 ���� ��һ�㣬����ڶ��� ����
	int vecDir;//0��ʾX�ᣬ1��ʾZ��
	int sec;//�ڼ��Σ���0��ʼ��Ĭ�϶��ǵ�һ��
	int DataExchange;

	int iIndex; // ���øֽ���Ϣʱ�����
	DPoint3d ptstr;
	DPoint3d ptend;
	DPoint3d ptmid;
};

struct CutRebarInfo
{
	bool isCutRebar;
	double dCutLength1;
	double dCutLength2;
	double dCutLength3;
};

struct InsertRebarPoint
{
	InsertRebarPoint()
	{
		ptstr = DPoint3d::From(0, 0, 0);
		ptmid = DPoint3d::From(0, 0, 0);
		ptend = DPoint3d::From(0, 0, 0);
		memset(sizeKey, 0, sizeof(sizeKey));
		isMid = false;
		memset(SelectedRebarType, 0, sizeof(SelectedRebarType));
		memset(level, 0, sizeof(level));
		memset(grade, 0, sizeof(grade));
		mid = 0;
	}
	ElementId mid;
	ElementId Rsid;//�����ֽ���
	DPoint3d ptstr;
	DPoint3d ptmid;
	DPoint3d ptend;
	char sizeKey[128];
	bool isMid;
	char SelectedRebarType[128];//�ֽ����������
	char level[128];//�ֽ���
	char grade[128];//�ֽ�ȼ�
};


struct RebarLine
{
	std::string cellName;
	std::string groupName;
	DPoint3d ptStr;
	DPoint3d ptEnd;
	double rebarDitr;
};

struct CNCutRebarInfo
{
	int nIndex;			// �±꣬����ǵڼ���

	double dLength;		// ����
	int nVecType;		// �ض���ʼ����  0 �� ���� �Ӹֽ���㿪ʼ 1 �� �Ӹֽ��յ㿪ʼ
};


struct STWallGeometryInfo
{
	DPoint3d ptPreStr;
	DPoint3d ptPreEnd;
	DPoint3d ptStart;
	DPoint3d ptEnd;
	DPoint3d ptBckStr;
	DPoint3d ptBckEnd;
	double length;
	double width;
	double height;

	STWallGeometryInfo()
	{
		ptStart = { 0,0,0 };
		ptEnd = { 0,0,0 };
		ptStart = { 0,0,0 };
		ptEnd = { 0,0,0 };
		ptBckStr = { 0,0,0 };
		ptBckEnd = { 0,0,0 };
		length = 0;
		width = 0;
		height = 0;
	}
};

namespace PIT
{
	struct Concrete
	{
		double  postiveCover;		//����������
		double  reverseCover;		//�ײ�������
		double  sideCover;			//���汣����
		int		rebarLevelNum;		//�ֽ����
		int     isHandleHole;       //�Ƿ��ܿ׶�
		double  MissHoleSize;
		int		isFaceUnionRebar;	//�Ƿ����������
		int		isSlabUpFaceUnionRebar;	//�Ƿ�嶥���������
		int		m_SlabRebarMethod;//�����ʽ
	};

	struct ConcreteRebar
	{
		int		rebarLevel;			//�ֽ��
		int		rebarDir;			//����			0������ֽ1������ֽ�
		char    rebarSize[512];		//�ֽ�ߴ�
		int		rebarType;			//�ֽ��ͺ�
		double  spacing;			//�ֽ���
		double  startOffset;		//���ƫ��
		double  endOffset;			//�յ�ƫ��
		double  levelSpace;			//�ֽ����
		int		datachange;			//���ݽ���
		double	angle;				//����Ƕ�
		//int		rebarColor;			//�ֽ���ɫ
		int		rebarLineStyle;		//�ֽ�����
		int		rebarWeight;		//�ֽ��߿�
	};

	struct LapOptions
	{
		int		rebarLevel;			//�ֽ��
		int		connectMethod;		//���ӷ�ʽ
		double  lapLength;			//��ӳ���
		double  stockLength;		//��泤��
		double  millLength;			//�ӹ�����
		int		isStaggered;		//�Ƿ񽻴�
		double  staggeredLength;	//������
		double  udLength;	        //Ů��ǽ��U�ν�����곤��	
	};
	struct EndType
	{
		int		endType;			//����:0,��;1,�乳��2��������3�����ߣ�
		                              //4��90���乳��5��135���乳��6��180���乳��7��ֱê
		double  offset;				//ƫ��
		double  rotateAngle;		//��ת��

		//��ͬ�˲��������ݲ�һ��
		struct RebarEndPointInfo
		{
			double value1 = 0.00;//�����뾶
			double value2 = 0.00;//��ת�Ƕ�
			double value3 = 0.00;//ê�볤��
			double value4 = 0.00;
			double value5 = 0.00;
			double value6 = 0.00;
			double value7 = 0.00;
			double value8 = 0.00;
			double value9 = 0.00;
		}endPtInfo;
	};

	struct AssociatedComponent
	{
		char    CurrentWallName[512];			//�����ǽ����
		char	associatedComponentName[512];	//��������
		int		mutualRelation;					//�໥��ϵ		0��ͬ��ǽ��1���ϲ�ǽ��2:�²�ǽ��3���ϲ�壬4���²��
	//	int		rebarLevel;						//����ֽ��
		int		associatedRelation;				//������ϵ		0�����ԣ�1��ê�룬2����ê��
		int		anchoringMethod;				//ê�����ӷ�ʽ	0-9��ê�����ӷ�ʽ
		int		isEndBothACC;					//�Ƿ�Ϊ�˲���������
		double	endOffset;						//�˲�ƫ��				//ê����ʽ1��ê����ʽ2��ʹ��
		int		acPositon;						//��������λ�� 0�����	1:�յ�	2���м�	//ê����ʽ2��ʹ��
		double	startL0;						//���˲�Ԥ������		//
		double	endL0;							//�յ�˲�Ԥ������		//
		double	La;								//��ǽ�����յ�˲�Ԥ������		//ê����ʽ9��ʹ��
		int		isReverse;						//�Ƿ�ת����ת������ֽ��һ�������һ��ֽ�ֱê���Ȼ���		//ê����ʽ9��ʹ��
		int		isCut;							//�Ƿ��ж���ǽ			//ê����ʽ4��ʹ��
	};

	struct WallRebarInfo			//ǽ��ÿһ��ĸֽ���Ϣ
	{
		Concrete concrete;

		ConcreteRebar rebar;

		LapOptions lapoption;

		EndType endtype;

		AssociatedComponent associatedcomponent;
	};

	struct DomeLevelInfo
	{
		int		   nNumber;			// ���
		double     dRadius1;		// ��ʼ���뾶
		double	   dRadius2;		// �������뾶
		int        nLayoutType;     // �ֽ�÷�ʽ 0 : XY����  1 : ��������
	};

	struct DomeCoverInfo
	{
		double		dCover;		// 񷶥������
		ElementId	eehId;		// 񷶥�������ʵ��id
	};

	struct DomeLevelDetailInfo // ÿ��ֽ����Ϣ
	{
		int		nLevel;			// ����
		int		nNumber;		// ��Ӧ  DomeLevelInfo �е����

		int		rebarShape;     // �ֽ���״   �������� --  0 : ����  1 : ���� 2    XY���� --  0 : X��  1 : Y��
		char	rebarSize[512]; // �ֽ�ֱ��
		int		rebarType;		// �ֽ�����
		double  dAngleOrSpace;	// �Ƕ� / ���
		double  dSpacing;		// ��ǰ����
		double  dStartOffset;   // ��ʼƫ��(����)
	};

	struct RebarData
	{
		BrString rebarSize;
		BrString rebarType;
		RebarSymbology rebarSymb;
		char SelectedRebarType[512];
		char SelectedRebarLevel[512];
		char SelectedRebarGrade[512];
		double SelectedSpacing;
	};
	BE_DEFINE_SUFFIX_TYPEDEFS(RebarData)

	//��ǽ�Ͽ��Ƕ�����
	struct BreakAngleData
	{
		double beginAngle;	//��ʼ�Ƕ�
		double endAngle;	//�����Ƕ�
	};

	struct DlgData//�����ϵ�����
	{
		double  postiveCover;			//���汣����
		double  sideCover;				//���汣���� 
		double  reverseCover;			//���汣����
		double  missHoleSize;			//���Գߴ�
		int		rebarLevelNum;			//�ֽ����
	};
};


extern std::map<std::string, IDandModelref> g_mapidAndmodel;
extern PIT::WallRebarInfo g_wallRebarInfo;
extern ElementHandle g_SelectedElm;
extern ElementId g_ConcreteId;
extern TwinBarSet::TwinBarInfo g_twinBarInfo;
extern TieReBarInfo	g_tieRebarInfo;
extern std::vector<PIT::ConcreteRebar>		g_vecRebarData;
extern std::vector<PIT::EndType>							g_vecEndTypeData;
extern std::vector<TwinBarSet::TwinBarLevelInfo>	g_vecTwinBarData;
extern std::vector<PIT::AssociatedComponent>	g_vecACData;
extern std::vector<PIT::LapOptions>	g_vecLapOptionData;
extern std::vector<RebarPoint>                   g_vecRebarPtsNoHole;//�׶��ж�֮ǰ�ĸֽ���ʼ��
extern std::vector<RebarPoint>                   g_vecTwinRebarPtsNoHole;//�׶��ж�֮ǰ�Ĳ�����ʼ��
extern std::vector<RebarPoint>					 g_vecTieRebarPtsNoHole; //�׶��ж�֮ǰ��������ʼ��

extern ElementHandle g_InsertElm;	// ������
extern ElementHandle g_ColumnElm;	// �����ʵ��

extern InsertRebarInfo 			g_InsertRebarInfo;	// ��������Ϣ
extern std::vector<InsertRebarInfo::WallInfo> g_vecWallInsertInfo; // ǽ��������Ϣ
extern int CoverType;
extern std::list<CString> g_listRebarType;
extern std::list<CString> g_listRebarType2;

bool GetConcreteXAttribute(ElementId elmID, DgnModelRefP pModel);

bool SetConcreteXAttribute(ElementId elmID, DgnModelRefP pModel);


/*
* @desc:	�������������Ϣ
* @param[in]	condition ��������
* @param[in]	outStr �����Ϣ
* @return	������������false�������Ϣ�������Ϸ���true
* @author	hzh
* @Date:	2022/11/22
*/
bool IsValidAndPromout(std::function<bool()> condition, WString outStr);

/*
* @desc:	��ȡѡ��Ԫ�ؼ���
* @param[out]	selectset ѡ��Ԫ�ؼ�
* @param[in]	outStr �����Ϣ
* @return	�ɹ�true��ʧ��false
* @author	hzh
* @Date:	2022/11/22
*/
bool GetSelectAgenda(ElementAgendaR selectset, WString outStr);