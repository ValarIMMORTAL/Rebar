

/*--------------------------------------------------------------------------------------+
|
|     $Source: RainshedRebarAssembly.h $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/

#include <RebarElements.h>
//#include "MySlabRebarComponent.h"
#include "CommonFile.h"
#include "WallRebarAssembly.h"

class RainshedRebarDlg;

class RainshedRebarAssembly : public RebarAssembly                   //墙配筋的钢筋信息
{
	BE_DATA_REFER(BeMatrix, Placement)              //当前局部坐标原点
	BE_DATA_VALUE(double, PositiveCover)			//下面保护层
	BE_DATA_VALUE(double, ReverseCover)			//上面保护层
	BE_DATA_VALUE(double, SideCover)				//侧面保护层
	BE_DATA_VALUE(int, RebarLevelNum)			//钢筋层数
	BE_DATA_VALUE(vector<int>, vecDir)					//方向,0表示x轴，1表示z轴
	BE_DATA_VALUE(vector<BrString>, vecDirSize)				//尺寸
	BE_DATA_VALUE(vector<int>, vecRebarType)			//钢筋类型
	BE_DATA_VALUE(vector<double>, vecDirSpacing)			//间隔
	BE_DATA_VALUE(vector<double>, vecStartOffset)			//起点偏移
	BE_DATA_VALUE(vector<double>, vecEndOffset)			//终点偏移
	BE_DATA_VALUE(vector<double>, vecLevelSpace)			//与前层间距

	BE_DATA_VALUE(vector<vector<PIT::EndType> >, vvecEndType)		//端部样式
	BE_DATA_VALUE(vector<ElementId>, vecSetId)								//rebarelementSet的每个Id

public:
	RainshedRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~RainshedRebarAssembly() {};

public:
	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;
public:
	std::string m_strElmName;
	

private:
//	BE_DATA_VALUE(SlabType, slabType)				//墙类型
protected:
	void			Init();

protected:
	virtual int		GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 2; }

	virtual bool	AnalyzingSlabGeometricData(ElementHandleCR eh) { return true; }


public:
	virtual bool	InsertMakeRebars(DgnModelRefP modelRef) { return true; }
	virtual bool	MakeRebars(DgnModelRefP modelRef) { return true; }
	virtual bool	SetSlabData(ElementHandleCR eh) { return true; }


	virtual bool	MakeACCRebars(DgnModelRefP modelRef) { return true; }

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef) {}

	//static SlabType JudgeSlabType(ElementHandleCR eh);

	static bool IsSmartSmartFeature(EditElementHandle& eeh);

	//static bool IsSlabSolid(ElementHandleCR eh);
	void SetConcreteData(PIT::Concrete const & concreteData);
	void SetRebarData(vector<PIT::ConcreteRebar> const& vecRebarData);
	//void SetRebarEndTypes(vector<PIT::EndType> const& vvecEndTypes);

	//void InitRebarSetId();
	//void GetConcreteData(Concrete& concreteData);
	//void GetRebarData(vector<PIT::ConcreteRebar>& vecData) const;
	//static bool IsSmartSmartFeature(EditElementHandle& eeh);
	//void SetTieRebarInfo(TieReBarInfo const& tieRebarInfo);
	//const TieReBarInfo GetTieRebarInfo() const;
private:
	TieReBarInfo m_tieRebarInfo;
};


class STRainshedRebarAssembly : public WallRebarAssembly
{
	BE_DATA_VALUE(STWallGeometryInfo, STRainshedData)			//STWall几何数据

public:
	//	ElementHandle m_eeh1;//当前模型eeh
	DgnModelRefP m_model;
	std::vector<EditElementHandle*> m_Noweeh;
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//筛选后的孔洞
	DPoint3d m_LineNormal;

	~STRainshedRebarAssembly()
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
	std::vector<DPoint3d>     m_vecRebarPtsLayer;
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//被孔洞截取前的所有并筋点数据
	RainshedRebarDlg * pSTRainshedDoubleRebarDlg;
private:
		
	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebar, double xPos, double height, double zhig, double startOffset, double endOffset, PIT::PITRebarEndTypes &endType, CMatrix3D const& mat, bool& tag);


protected:
		
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Rainshed; }//判断是哪个类
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"STRainshed Rebar"; }//描述
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"STRainshed Rebar"; }
	virtual bool        OnDoubleClick() override;//双击后再次出现对话框
	virtual bool        Rebuild() override;//构件修改后，调用重新调整钢筋

protected:
	/*

	*///创建钢筋函数
	RebarSetTag* MakeRebars(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat,DgnModelRefP modelRef);
	//	RebarSetTag* MakeRebars_Laption(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double width, double spacing, double startOffset, double endOffset, LapOptions const& lapOptions, vector<PIT::EndType> const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

	virtual void	CalculateTransformSPY1(vector<CVector3D> &vTransform, DgnModelRefP modelRef);

	virtual void	CalculateTransformSPY2(vector<CVector3D> &vTransform, DgnModelRefP modelRef);

	virtual bool	AnalyzingSlabGeometricData(ElementHandleCR eh);
public:

	virtual void SetRebarData(vector<PIT::ConcreteRebar> const& vecData); //用vector数组存每层钢筋的信息

	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

//	void GetRainshedFace(ElementHandleCR eeh, double* tHeight);
	void GetRainshedFace(ElementHandleCR eeh, EditElementHandleR DownFace, EditElementHandleR DownLowFace, double* tHeight);
	bool GetFrontBackLineAndHighFace(ElementHandleCR eeh, EditElementHandle* pHighFace, vector<MSElementDescrP>& vec_line, vector<MSElementDescrP>& vec_Lowline,
		vector<DSegment3d>& vec_linefront, vector<DSegment3d>& vec_lineback, vector<DSegment3d>& vec_linefront_Down, vector<DSegment3d>& vec_lineback_Down, double* tHeight);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }//创建几何
	BE_DECLARE_VMS(STRainshedRebarAssembly, RebarAssembly)

public:
	double m_bendRadius1;
	double m_diameter1;
	STRainshedRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		WallRebarAssembly(id, modelRef)
#ifdef PDMSIMPORT
		, pSlab(NULL)
#endif
	{
		pSTRainshedDoubleRebarDlg = nullptr;
		m_zLength = 0.00;
		m_vecAllRebarStartEnd.clear();
	};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);
//	void CalculateUseHoles(DgnModelRefP modelRef);
private:
	struct STSlabGeometryInfo//几何信息
	{
		DPoint3d ptStart_H;
		DPoint3d ptEnd_H;
		DPoint3d ptStart_L;
		DPoint3d ptStartBack_L;
		DPoint3d ptEnd_L;
		double length;
		double width;    //宽度
		double length_down; // 底部长度
		double width_down;  // 底部宽度
		double maxheight;//端部的高
		double moreheight=0;//自由端的高
		double minheight;//低的高
		DRange3d lowfacep;//底面range
		DRange3d hightfacep; //上面range
	}m_RainshedData;
	CVector3D  m_xVec;

	double	m_zLength;
public:
	//	vector<DPoint3d> m_allpoint;
	DPoint3d m_stP[2];
	DPoint3d m_edP[2];
	CVector3D m_widthDirection;// 宽度方向
#ifdef PDMSIMPORT
	STSlabComponent *pSlab;
#endif
};

