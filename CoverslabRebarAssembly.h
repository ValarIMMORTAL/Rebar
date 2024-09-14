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
#include "CommonFile.h"
#include "PITRebarCurve.h"
#include "PITRebarAssembly.h"

class CoverslabRebarDlg;
using namespace PIT;
class CoverslabRebarAssembly : public PIT::PITRebarAssembly              //盖板配筋的钢筋信息
{
public:
	enum CoverSlabType
	{
		STCoverSlab,	   // ST盖板11个面
		STCoverSlab_Ten,   // 只有10个面
		SZCoverSlab,
		SICoverSlab,
		OtherCoverSlab
	};
	BE_DATA_REFER(BeMatrix, Placement)              //当前局部坐标原点
	BE_DATA_VALUE(double, PositiveCover)			//正面保护层
	BE_DATA_VALUE(double, ReverseCover)				//反面保护层
	BE_DATA_VALUE(double, SideCover)				//侧面保护层
	BE_DATA_VALUE(int, RebarLevelNum)				//钢筋层数
	BE_DATA_VALUE(vector<int>, vecDir)				//方向
	BE_DATA_VALUE(vector<BrString>, vecDirSize)		//尺寸
	BE_DATA_VALUE(vector<int>, vecRebarType)		//钢筋类型
	BE_DATA_VALUE(vector<double>, vecDirSpacing)	//间隔
	BE_DATA_VALUE(vector<double>, vecStartOffset)	//起点偏移
	BE_DATA_VALUE(vector<double>, vecEndOffset)		//终点偏移
	BE_DATA_VALUE(vector<double>, vecLevelSpace)	//与前层间距
	BE_DATA_REFER(int, staggeredStyle)				// 交错类型
	BE_DATA_VALUE(vector<TwinBarSet::TwinBarLevelInfo >, vecTwinRebarLevel)	//并筋层
	BE_DATA_VALUE(vector<vector<PIT::EndType> >, vvecEndType)					//端部样式
	BE_DATA_VALUE(vector<ElementId>, vecSetId)								//rebarelementSet的每个Id

public:
	CoverslabRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~CoverslabRebarAssembly() {};

public:
	vector<vector<DSegment3d> > m_vecRebarStartEnd;	//规避了孔洞的所有点

	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;//未规避孔洞的所有点
private:
	BE_DATA_VALUE(CoverSlabType, CoverslabType)				//盖板类型
protected:
	void			Init();

protected:
	virtual int		GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 2; }

	virtual bool	AnalyzingSlabGeometricData(ElementHandleCR eh) { return true; }


public:
	static bool IsSmartSmartFeature(EditElementHandle& eeh);

	virtual bool	InsertMakeRebars(DgnModelRefP modelRef) { return true; }
	virtual bool	MakeRebars(DgnModelRefP modelRef) { return true; }
	virtual bool	SetCoverSlabData(ElementHandleCR eh) { return true; }
	//	virtual void	InitUcsMatrix() {}

	virtual bool	MakeACCRebars(DgnModelRefP modelRef) { return true; }

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef) {}

	static CoverSlabType JudgeSlabType(ElementHandleCR eh);

	//	static bool IsSlabSolid(ElementHandleCR eh);
	void SetConcreteData(Concrete const & concreteData);
	void SetRebarData(vector<PIT::ConcreteRebar> const& vecRebarData);
		void SetRebarEndTypes(vector<PIT::EndType> const& vvecEndTypes);
		//void SetTwinbarInfo(TwinBarSet::TwinBarInfo const& twInfo);
		//void GetTwinbarInfo(TwinBarSet::TwinBarInfo& twInfo);
	void InitRebarSetId();
	void GetConcreteData(Concrete& concreteData);
	void GetRebarData(vector<PIT::ConcreteRebar>& vecData) const;
	//	static bool IsSmartSmartFeature(EditElementHandle& eeh);
	void SetTieRebarInfo(TieReBarInfo const& tieRebarInfo);
	const TieReBarInfo GetTieRebarInfo() const;
private:
	TieReBarInfo m_tieRebarInfo;
};


class SICoverslabRebarAssembly : public CoverslabRebarAssembly
{
	BE_DATA_VALUE(STWallGeometryInfo, SICoverSlabData)			//STWall几何数据
		BE_DATA_VALUE(bool, IsTwinrebar)			//在画rebarcuve时，判断是否为并筋层
public:
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//筛选后的孔洞
	CoverslabRebarDlg * pCoverslabDoubleRebarDlg;
	DPoint3d m_LineNormal;
	double m_diameter1;
	double m_angle_left;
	double m_angle_right;
	~SICoverslabRebarAssembly()
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

	std::vector<DPoint3d>     m_vecRebarPtsLayer;//被孔洞截取前的所有钢筋点数据
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//被孔洞截取前的所有并筋点数据
private:
	bool makeRebarCurve(vector<PITRebarCurve>& rebar, double xPos, double height, double startOffset,
		double endOffset, double bendRadius, const vector<PIT::EndType>& vecEndtype, PITRebarEndTypes &endType, CMatrix3D const& mat,bool isTwin = false);
	//	bool makeRebarCurve_Laption(RebarCurve& rebar, double xPos, double height, double bendRadius, double bendLen, RebarEndTypes const& endType, CVector3D const& endNormal, CMatrix3D const& mat);

	bool makeRebarCurve
	(
		PITRebarCurve&				rebar,
		PITRebarEndTypes&			endTypes,
		const vector<PIT::EndType>&	vecEndtype,
		vector<DPoint3d>&			vecPoint,
		double						bendRadius
	);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + STWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"SICover Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"SICover Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:
	/*

	*/
	RebarSetTag* MakeRebars(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat, DgnModelRefP modelRef);
	//	RebarSetTag* MakeRebars_ACC(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat, const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel, DgnModelRefP modelRef);
//	RebarSetTag* MakeRebars_Laption(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, LapOptions const& lapOptions, vector<PIT::EndType> const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

	virtual bool	AnalyzingSICoverSlabData(ElementHandleCR eh);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(SICoverslabRebarAssembly, RebarAssembly)

public:
	SICoverslabRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		CoverslabRebarAssembly(id, modelRef)
#ifdef PDMSIMPORT
		, pWall(NULL)
#endif
	{
		pCoverslabDoubleRebarDlg = NULL;
		m_vecRebarStartEnd.clear();
	};

	virtual bool	SetCoverSlabData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);

	//	virtual void	InitUcsMatrix();

		//	virtual bool	MakeACCRebars(DgnModelRefP modelRef);

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef);
	//	void  CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef);
	void CalculateUseHoles(DgnModelRefP modelRef);

#ifdef PDMSIMPORT
private:
	STWallComponent *pWall;
#endif
};

class STCoverslabRebarAssembly : public CoverslabRebarAssembly
{
	BE_DATA_VALUE(STWallGeometryInfo, STCoverSlabData)			//STWall几何数据
		BE_DATA_VALUE(bool, IsTwinrebar)			//在画rebarcuve时，判断是否为并筋层
public:
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//筛选后的孔洞
	EditElementHandle* m_pOldElm;
	DPoint3d m_LineNormal;
	double m_diameter1;
	double m_angle_left;
	double m_angle_right;
	CoverslabRebarDlg * pSTCoverslabDoubleRebarDlg;
	~STCoverslabRebarAssembly()
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

	std::vector<DPoint3d>     m_vecRebarPtsLayer;//被孔洞截取前的所有钢筋点数据
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//被孔洞截取前的所有并筋点数据
private:
	bool makeRebarCurve(vector<PITRebarCurve>& rebar, double xPos, double height, 
		double startOffset, double endOffset, const vector<PIT::EndType>& vecEndtype, PITRebarEndTypes &endType, CMatrix3D const& mat,bool isTwin = false);
	//	bool makeRebarCurve_Laption(RebarCurve& rebar, double xPos, double height, double bendRadius, double bendLen, RebarEndTypes const& endType, CVector3D const& endNormal, CMatrix3D const& mat);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + STWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"STWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"STWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:
	RebarSetTag* MakeRebars(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat, DgnModelRefP modelRef);

	virtual bool AnalyzingSICoverSlabData(ElementHandleCR eh);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(STCoverslabRebarAssembly, RebarAssembly)

public:
	static void GetHighorDownFace(ElementHandleCR eeh, EditElementHandleR DownFace, double* tHeight,bool chooseface);//true为上面false为下面
	static bool GetHighorDownLine(ElementHandleCR eeh, vector<DSegment3d>& vec_linefront, vector<DSegment3d>& vec_lineback, double* tHeight, bool chooseface);
	void AnalyzingAllFace(ElementHandleCR eeh);
public:
	STCoverslabRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		CoverslabRebarAssembly(id, modelRef)
#ifdef PDMSIMPORT
		, pWall(NULL)
#endif
	{
		pSTCoverslabDoubleRebarDlg = nullptr;
		m_vecRebarStartEnd.clear();
	};

	virtual bool	SetCoverSlabData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);

	//	virtual void	InitUcsMatrix();

		//	virtual bool	MakeACCRebars(DgnModelRefP modelRef);

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef);
	//	void  CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef);
	void CalculateUseHoles(DgnModelRefP modelRef);
private:
	struct STCoverSlabHighDateInfo
	{
		double length;
		double width;    //宽度
		double height;//端部的高
	}CoverSlabHighDate;

	struct STCoverSlabLowDateInfo
	{
		double length;
		double width;    //宽度
		double height;//端部的高
	}CoverSlabLowDate;

	double m_zLen;

#ifdef PDMSIMPORT
private:
	STWallComponent *pWall;
#endif
};

class CoverslabRebarDlg;
class SZCoverslabRebarAssembly : public CoverslabRebarAssembly
{
	BE_DATA_VALUE(STWallGeometryInfo, SZCoverSlabData)			//STWall几何数据
	BE_DATA_VALUE(bool, IsTwinrebar)							//在画rebarcuve时，判断是否为并筋层
public:
	CoverslabRebarDlg * pSZCoverslabDoubleRebarDlg;
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//筛选后的孔洞
	EditElementHandle* m_pOldElm;
	DPoint3d m_LineNormal;
	double m_diameter1;
	double m_angle_left;
	double m_angle_right;
	bool   m_SZCoverFlag; // 上起点比下起点小为true , 否则为false
	~SZCoverslabRebarAssembly()
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

	std::vector<DPoint3d>     m_vecRebarPtsLayer;//被孔洞截取前的所有钢筋点数据
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//被孔洞截取前的所有并筋点数据
private:
	bool makeRebarCurve(vector<PITRebarCurve>& rebar, double xPos, double height, double startOffset, 
		double endOffset, const vector<PIT::EndType>&	vecEndtype, PITRebarEndTypes &endType, CMatrix3D const& mat);
	bool makeRebarCurve_SZ(vector<PIT::PITRebarCurve>& rebar, double xPos, double height, double startOffset, 
		double endOffset, PIT::PITRebarEndTypes &endType, CMatrix3D const& mat, bool isTwin = false);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + STWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"STWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"STWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
protected:
	/*

	*/
	RebarSetTag* MakeRebars(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat, DgnModelRefP modelRef);
	//	RebarSetTag* MakeRebars_ACC(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const& endType, vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat, const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel, DgnModelRefP modelRef);
//	RebarSetTag* MakeRebars_Laption(ElementId& rebarSetId, BrStringCR sizeKey, double xDir, double height, double spacing, double startOffset, double endOffset, LapOptions const& lapOptions, vector<PIT::EndType> const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

	virtual bool	AnalyzingSICoverSlabData(ElementHandleCR eh);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(SZCoverslabRebarAssembly, RebarAssembly)

public:
	static void GetHighorDownFace(ElementHandleCR eeh, EditElementHandleR DownFace, double* tHeight, bool chooseface);//true为上面false为下面
	static bool GetHighorDownLine(ElementHandleCR eeh, vector<DSegment3d>& vec_linefront, vector<DSegment3d>& vec_lineback, double* tHeight, bool chooseface);
	void AnalyzingAllFace(ElementHandleCR eeh);
public:
	SZCoverslabRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		CoverslabRebarAssembly(id, modelRef)
#ifdef PDMSIMPORT
		, pWall(NULL)
#endif
	{
		m_SZCoverFlag = true;
		m_vecRebarStartEnd.clear();
		pSZCoverslabDoubleRebarDlg = NULL;
	};

	virtual bool	SetCoverSlabData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);

	//	virtual void	InitUcsMatrix();

		//	virtual bool	MakeACCRebars(DgnModelRefP modelRef);

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, DgnModelRefP modelRef);
	//	void  CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef);
//	void CalculateUseHoles(DgnModelRefP modelRef);
private:
	struct STCoverSlabHighDateInfo//盖板上面板信息
	{
		double morewidth;
		double length;
		double width;    //宽度
		double height;//端部的高
	}CoverSlabHighDate;

	struct STCoverSlabLowDateInfo //下面板信息
	{
		double length;
		double width;    //宽度
		double height;//端部的高
	}CoverSlabLowDate;

	double m_differ_width; // SZ型板叠加差长度
	// bool pointineehType;//标志哪种ST盖板

	double m_zLen;
#ifdef PDMSIMPORT
private:
	STWallComponent *pWall;
#endif
};