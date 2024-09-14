#pragma once
/*--------------------------------------------------------------------------------------+
|
|  $Source: sdk/example/RebarSDKExampleids.h $
|
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "../CommonId/AllCommonXAttribute.h"

#define RTYPE_XML    'XML'
#define RSCID_XML    1

#define RTYPE_GALLERY_SETTINGS 'GS'
#define RSCID_GALLERY_SETTINGS 2


#define TWIN_REBAR_LEVEL	L"TWIN_REBAR"
#define U_REBAR_LEVEL		L"U_REBAR"
#define STIRRUP_REBAR_LEVEL		L"STIRRUP_REBAR"

enum CmdNameRebarIds
    {
    CMDNAME_RebarPlace = 1,
	CMDNAME_InsertRebarTool,
	CMDNAME_RebarSDKReadRebar,
	CMDNAME_TieRebarFace
   };

/*----------------------------------------------------------------------+
|                                                                       |
|   String List IDS                                                     |
|                                                                       |
+----------------------------------------------------------------------*/
enum StringListRebarIds
    {
    STRINGLISTID_RebarSDKExampleCommandNames = 0,
    STRINGLISTID_RebarSDKExampleTextMessages
    };

/*----------------------------------------------------------------------+
|                                                                       |
|   Prompts                                                             |
|                                                                       |
+----------------------------------------------------------------------*/
enum PromptRebarIds
    {
    PROMPT_SELECT_SMARTSOLID_ELEMENT = 1,
    PROMPT_ACCEPTREJECT,
    PROMPT_AcceptOrReject,
    PROMPT_SelectSlabSolid,
    PROMPT_SelectRebarElement,
    PROMPT_SelectPierVaseElement,
    PROMPT_AcceptSelectionSet,
    PROMPT_EnterFirstPoint,
    PROMPT_EnterSecondPoint,
	PROMPT_SELECT_BASIS,
	PROMPT_SELECT_EXPAND,
	PROMPT_SELECT_1,
	PROMPT_SELECT_2
    };

enum ItemListRebarIds
    {
    ItemList_PlaceWallRebar = 1,
    ItemList_PlaceSlabRebar,
    };

enum StringListInsertIds
	{
	STRINGLISTID_Commands = 1,
	STRINGLISTID_Prompts
	};


struct SlabRebarInfo
    {
    double  topCover;
    double  bottomCover;
    double  sideCover;
    char    topXDirSize[512];
    double  topXDirSpacing;
    char    topYDirSize[512];
    double  topYDirSpacing;
    char    bottomXDirSize[512];
    double  bottomXDirSpacing;
    char    bottomYDirSize[512];
    double  bottomYDirSpacing;
    };

//�ֽ���ϱ������Ϣ
struct RebarListInfo
{
	char m_strRebarOrder[512];//�ֽ���
	char m_strDiameter[512];//�ֽ�ֱ��
	char m_strRebarGroupSize[512];//����
	char m_strRebarSetNum[512];//ÿ�����
	char m_strRebarAllNum[512];//�ܸ���
	char m_strRebarLength[512];//����
	char m_strRebarHooktype[512];//�乳����
	char m_strRebarSharpeNum[512];//��״����
};

//�ֽ������������Ϣ
struct Rebarinfo
{
	char RebarGrade[512];//�ֽ�ȼ� ��HPB300
	char AvgDamieter[20];//ƽ��ֱ��
	char BndRebarLength[20];//�����ֽ��
	char StrRebarLength[20];//ֱ�߸ֽ��

	char BndRebarWeight[20];//�����ֽ�����
	char StrRebarWeight[20];//ֱ�߸ֽ�����

	char AllLen[20];//����ֱ���ĸֽ��ܳ���
	char AllWei[20];//����ֱ���ĸֽ�������
};

// �ֽ������������Ϣ
// struct RebarWeightListInfo
// {
// 	int RebarDiameter;//�ֽ�ֱ��
// 	double RebarAllLength;//ĳ��ֱ�����ܳ���
// 
// };


struct TwinBarSet
{
	struct TwinBarInfo
	{
		int		isTwinbars;			//�Ƿ�Ϊ����
		int		isStaggered;		//�Ƿ񽻴�
	}twinBarInfo;

	struct TwinBarLevelInfo
	{
		char    levelName[512];		//���������
		int		hasTwinbars;		//�Ƿ��в���
		char    rebarSize[512];		//����ߴ�
		int		rebarType;			//�����ͺ�
		int		interval;			//������
	}twinbarlevelinfo;

};

struct TieReBarInfo
{
	int		tieRebarMethod;		//������ʽ		0�������1��ֱ����2��б��
	char    rebarSize[512];		//�ֽ�ߴ�
	int		rebarType;			//�ֽ��ͺ�
	int		tieRebarStyle;		//����÷�ʽ	0��X*X;		1:X*2X;		2:2X*X;		3:2X*2X;	4:�Զ�����ʽ
	int		isPatch;			//�Ƿ�߲���������
	int		rowInterval;		//�Զ�����ʽʱ��ˮƽ���
	int		colInterval;		//�Զ�����ʽʱ����ֱ���
};

struct StairRebarInfo
{

	int		StairsStyle;		//¥����ʽ		0��Ԥ�ƣ�1���߽�
	char    rebarSize[512];		//�ֽ�ߴ�
	int		rebarType;			//�ֽ��ͺ�
	double		StairsCover;		//������

};

struct CustomizeRebarInfo
{

	int		rebarType;			//�ֽ��ͺ�		
	char    rebarSize[512];		//�ֽ�ߴ�
	char	rebarArrayDir[50];	//���з���
	int		rebarArrayNum;		//��������
	double  rebarSpacing;		//�ֽ���
	char    rebarbsType[50];        //�ֽ�ʶ����
	char    rebarLevel[50];
};
struct WallSetInfo
{
	char    rebarSize[512];		//�ֽ�ߴ�
	int		rebarType;			//�ֽ��ͺ�
	double  spacing;			//�ֽ���
	double  uLenth;             //U�ͽ����곤��
};

struct CustomRebarl
{
	int     number;                     //�ֽ����
	double  Rebarlength;             //�ֽ��
	int     lengthtype;                //��������
};

struct CenterlineLength
{
	double Centerline;
};

struct InsertRebarInfo			// �����Ϣ
{

	struct ColumnInfo
	{
		int				shape;			// 0: ����   1: Բ��
		double			length;			// ����
		double			width;			// ���
		double			heigth;			// �߶�
		double			columeCover;	// ��������

		char			rebarVerticalSize[512];		// �ݽ�ߴ�
		int				rebarVerticalType;			// �ݽ��ͺ�
		char			rebarHoopSize[512];			// ����ߴ�
		int				rebarHoopType;				// �����ͺ�

	}colInfo;

	struct RebarInfo // ������Ϣ
	{
		int					longNum;		// ��������
		int					shortNum;		// ��������
		char				rebarSize[512];	// �ֽ�ߴ�
		int					rebarType;		// �ֽ��ͺ�
		double				embedLength;	// ���ó���
		double				expandLength;	// ��չ����
			
		int					endType;		// �˲���ʽ����
		int					cornerType;		// �乳����
		double				rotateAngle;	// ��ת��
	}rebarInfo;

	enum ConnectStyle	//���ӷ�ʽ
	{
		StaggerdJoint = 0,	//�����
		MechanicalJoint,	//��е����
	};

	struct WallInfo	// ������Ϣ
	{
		int					staggeredStyle; // ��������
		int					wallTopType;	// ����ǽ����
		double				postiveCover;   // ��ײ�������
		double				slabDistance;   // �ֽ�ֱ�߳���(���е�)
		double				slabThickness;  // ��ĺ��
		double				embedLength;	// ���ó���
		double				expandLength;	// ��չ����
		double				rotateAngle;	// ��ת��
		int					endType;		// �˲���ʽ����
		char				rebarSize[512];	// �ֽ�ߴ�
		bool				isStaggered;    // �Ƿ񽻴�
		double				mainDiameter;	// �����е�ֱ��
		int                 HookDirection;  //�乳����
		double				NormalSpace;
		double				AverageSpace;
		bool				isBack;			//�ֽ������Ƿ��Ǳ���ֽ�
		int					connectStyle;	//���ӷ�ʽ
	}wallInfo;

};
enum ReinForcingType//��ǿ�����÷�ʽ
{
	V1_ADD_H3 = 0,//ˮƽ1L+��ֱ3L
	V1_ADD_H4,//ˮƽ1L+��ֱ4L
	V2_ADD_H3,//ˮƽ2L+��ֱ3L
	V2_ADD_H4,//ˮƽ2L+��ֱ4L
	ALL,//ˮƽ1L+ˮƽ2L+��ֱ3L+��ֱ4L
};


struct ACCConcrete
{
	double  postiveOrTopCover;			//���������汣���㣨��Ϊ���棩
	double  reverseOrBottomCover;		//�ײ������汣���㣨��Ϊ���棩
	double  sideCover;					//���汣����
//	double  offset;						//�ֽ�ƫ��
};

struct StirrupData
{
	int		rebarNum;			//����
	char    rebarSize[512];		//�ֽ�ߴ�
	int		rebarType;			//�ֽ��ͺ�
	double		rebarStartOffset;			//���ƫ��
	double		rebarEndOffset;			//�յ�ƫ��
	double      tranLenth;//���곤��
};

struct Abanurus_PTRebarData
{
	int ptHNum;//����������
	int ptVNum;//�����������
	char ptrebarSize[512];//���ߴ�
	int ptrebarType;//���ȼ�
	char stirrupRebarsize[512];//����ߴ�
	int stirrupRebarType;//����ȼ�
	//int tieNum;//��������
	//char tierebarSize[512];//����ߴ�
	//int tierebarType;//����ȼ�

};



struct BeamRebarInfo
{
	struct BeamBaseData
	{
		double			dWidth;
		double			dDepth;
		double			dLeftSide;
		double			dRightSide;
		double			dLeftXLen;
		double			dRightXLen;
		double			dNetSpan;	 // �����
		double			dAxisToAxis; // ���ߵ�����
	}beamBaseData;

	struct BeamDefaultData
	{
		double			dTop;
		double			dLeft;
		double			dRight;
		double			dUnder;
		double			dFloor;
		double			dMargin;
		int				nRebarCal; // �ֽ����

	}beamDefaultData; // ��Ĭ��ֵ

	struct BeamAreaVertical
	{
		char			label[512];			// ��ǩ
		double			dSpace;				// ��϶
		double			dStartOffset;		// ���ƫ��
		int				nTotNum;			// ������
		double			dEndOffset;			// �յ�ƫ��
		int				nPosition;			// λ��
	}beamAreaVertical; // ��������

	struct BeamRebarVertical
	{
		char			label[512];			// ��ǩ	
		double			dLeftOffset;		// ���ƫ��
		double			dRightOffset;		// �Ҷ�ƫ��
		double			dLeftRotateAngle;	// �����ת��
		double			dRightRotateAngle;  // �Ҷ���ת��
		int				nPosition;			// λ��
		int				nTotNum;			// ������
		int				nLeftEndStyle;		// �����ʽ
		int				nRightEndStyle;		// �Ҷ���ʽ
		char			rebarSize[512];		// �ֽ�ߴ�
		int				rebarType;			// �ֽ�����
	}beamRebarVertical;

	struct BeamCommHoop
	{
		char					label[512];		// ��ǩ
		double					dSpacing;		// ���
		char					rebarSize[512]; // �ֽ�ߴ�
		int						rebarType;		// �ֽ�����
		double					dStartPos;		// ʼ��
		double					dEnd_N_Deep;	// ͨ��N*���
		int						nPostion;		// λ��
		double					dEndPos;		// �ն�
		double					dStart_N_Deep;  // ͨ��N*���
	}beamCommHoop;

	struct BeamRebarHoop
	{
		char					label[512];	// ��ǩ
		double					dOffset;
		double					dStartRotate;
		double					dEndRotate;
		int						nStartEndType; // ���˲�����
		int						nFnishEndType; // ���˲�����
	}beamRebarHoop;

};
struct FaceTieReBarInfo
{
	int		tieRebarMethod;		//������ʽ		0���Կ����1�������Կ�
	char    rebarSize[512];		//�ֽ�ߴ�
	int		rebarType;			//�ֽ��ͺ�
	int		tieRebarStyle;		//����÷�ʽ	0��200*400;		1:400*400;	
	//int		isPatch;			//�Ƿ�߲���������
	int firstAngle;
	int secondAngle;
	int		rowInterval;
	int		colInterval;
};