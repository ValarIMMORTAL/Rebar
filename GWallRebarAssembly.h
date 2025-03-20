#pragma once
class WallRebarAssembly;
#include "WallRebarAssembly.h"

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
	RebarSetTag* MakeRebars_Transverse(ElementId& rebarSetId, BrStringCR sizeKey, vector<CPoint3D> vecPt, double spacing, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);

	RebarSetTag* MakeRebars_Longitudinal(ElementId& rebarSetId, BrStringCR sizeKey, double &xDir, const vector<double> height, double spacing, double startOffset, double endOffset, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef);


	void JudgeGWallType(ElementHandleCR eh);

	virtual void	CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef);

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);

	virtual void CalculateUseHoles(DgnModelRefP modelRef);
	bool GetUcsAndStartEndData(int index, double thickness, DgnModelRefP modelRef, bool isSTGWALL = false);
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

