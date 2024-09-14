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
class CoverslabRebarAssembly : public PIT::PITRebarAssembly              //�ǰ����ĸֽ���Ϣ
{
public:
	enum CoverSlabType
	{
		STCoverSlab,	   // ST�ǰ�11����
		STCoverSlab_Ten,   // ֻ��10����
		SZCoverSlab,
		SICoverSlab,
		OtherCoverSlab
	};
	BE_DATA_REFER(BeMatrix, Placement)              //��ǰ�ֲ�����ԭ��
	BE_DATA_VALUE(double, PositiveCover)			//���汣����
	BE_DATA_VALUE(double, ReverseCover)				//���汣����
	BE_DATA_VALUE(double, SideCover)				//���汣����
	BE_DATA_VALUE(int, RebarLevelNum)				//�ֽ����
	BE_DATA_VALUE(vector<int>, vecDir)				//����
	BE_DATA_VALUE(vector<BrString>, vecDirSize)		//�ߴ�
	BE_DATA_VALUE(vector<int>, vecRebarType)		//�ֽ�����
	BE_DATA_VALUE(vector<double>, vecDirSpacing)	//���
	BE_DATA_VALUE(vector<double>, vecStartOffset)	//���ƫ��
	BE_DATA_VALUE(vector<double>, vecEndOffset)		//�յ�ƫ��
	BE_DATA_VALUE(vector<double>, vecLevelSpace)	//��ǰ����
	BE_DATA_REFER(int, staggeredStyle)				// ��������
	BE_DATA_VALUE(vector<TwinBarSet::TwinBarLevelInfo >, vecTwinRebarLevel)	//�����
	BE_DATA_VALUE(vector<vector<PIT::EndType> >, vvecEndType)					//�˲���ʽ
	BE_DATA_VALUE(vector<ElementId>, vecSetId)								//rebarelementSet��ÿ��Id

public:
	CoverslabRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~CoverslabRebarAssembly() {};

public:
	vector<vector<DSegment3d> > m_vecRebarStartEnd;	//����˿׶������е�

	vector<vector<DSegment3d> > m_vecAllRebarStartEnd;//δ��ܿ׶������е�
private:
	BE_DATA_VALUE(CoverSlabType, CoverslabType)				//�ǰ�����
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
	BE_DATA_VALUE(STWallGeometryInfo, SICoverSlabData)			//STWall��������
		BE_DATA_VALUE(bool, IsTwinrebar)			//�ڻ�rebarcuveʱ���ж��Ƿ�Ϊ�����
public:
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//ɸѡ��Ŀ׶�
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

	std::vector<DPoint3d>     m_vecRebarPtsLayer;//���׶���ȡǰ�����иֽ������
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//���׶���ȡǰ�����в��������
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
	BE_DATA_VALUE(STWallGeometryInfo, STCoverSlabData)			//STWall��������
		BE_DATA_VALUE(bool, IsTwinrebar)			//�ڻ�rebarcuveʱ���ж��Ƿ�Ϊ�����
public:
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//ɸѡ��Ŀ׶�
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

	std::vector<DPoint3d>     m_vecRebarPtsLayer;//���׶���ȡǰ�����иֽ������
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//���׶���ȡǰ�����в��������
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
	static void GetHighorDownFace(ElementHandleCR eeh, EditElementHandleR DownFace, double* tHeight,bool chooseface);//trueΪ����falseΪ����
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
		double width;    //���
		double height;//�˲��ĸ�
	}CoverSlabHighDate;

	struct STCoverSlabLowDateInfo
	{
		double length;
		double width;    //���
		double height;//�˲��ĸ�
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
	BE_DATA_VALUE(STWallGeometryInfo, SZCoverSlabData)			//STWall��������
	BE_DATA_VALUE(bool, IsTwinrebar)							//�ڻ�rebarcuveʱ���ж��Ƿ�Ϊ�����
public:
	CoverslabRebarDlg * pSZCoverslabDoubleRebarDlg;
	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<EditElementHandle*> m_Negs;
	std::vector<EditElementHandle*> m_useHoleehs;//ɸѡ��Ŀ׶�
	EditElementHandle* m_pOldElm;
	DPoint3d m_LineNormal;
	double m_diameter1;
	double m_angle_left;
	double m_angle_right;
	bool   m_SZCoverFlag; // �����������СΪtrue , ����Ϊfalse
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

	std::vector<DPoint3d>     m_vecRebarPtsLayer;//���׶���ȡǰ�����иֽ������
	std::vector<DPoint3d>     m_vecTwinRebarPtsLayer;//���׶���ȡǰ�����в��������
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
	static void GetHighorDownFace(ElementHandleCR eeh, EditElementHandleR DownFace, double* tHeight, bool chooseface);//trueΪ����falseΪ����
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
	struct STCoverSlabHighDateInfo//�ǰ��������Ϣ
	{
		double morewidth;
		double length;
		double width;    //���
		double height;//�˲��ĸ�
	}CoverSlabHighDate;

	struct STCoverSlabLowDateInfo //�������Ϣ
	{
		double length;
		double width;    //���
		double height;//�˲��ĸ�
	}CoverSlabLowDate;

	double m_differ_width; // SZ�Ͱ���Ӳ��
	// bool pointineehType;//��־����ST�ǰ�

	double m_zLen;
#ifdef PDMSIMPORT
private:
	STWallComponent *pWall;
#endif
};