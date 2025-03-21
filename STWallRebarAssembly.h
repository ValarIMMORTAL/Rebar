#pragma once
class WallRebarAssembly;
#include "WallRebarAssembly.h"

class STWallRebarAssembly : public WallRebarAssembly
{
	BE_DATA_VALUE(STWallGeometryInfo, STwallData)			//STWall��������
//	BE_DATA_VALUE(bool, IsTwinrebar)			//�ڻ�rebarcuveʱ���ж��Ƿ�Ϊ�����
public:

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
		double endOffset, PIT::PITRebarEndTypes &endType, CMatrix3D const& mat, bool istwin = false);
	bool makeRebarCurve_Laption(RebarCurve& rebar, double xPos, double height, double bendRadius,
		double bendLen, RebarEndTypes const& endType, CVector3D const& endNormal, CMatrix3D const& mat, bool istwin = false);
	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);
	bool Is_Wall(const ElementHandle & element);
	void AdjustAnchorageLengthInWall(DPoint3d originPt, PIT::PITRebarEndType& rebarEndType, const vector<EditElementHandle*>& alleehs);
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
		CMatrix3D const& mat, const TwinBarSet::TwinBarLevelInfo &twinBarInfo, bool bTwinbarLevel, int level, int grade, int DataExchange, DgnModelRefP modelRef, int rebarLineStyle,
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
		, pWall(NULL)
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
	* @desc:		������ΧԪ�غ͸ֽ��ߵ�λ�ü���ֽ��Ƿ�Ϸ�
	* @param[in]	nowVec  �ж�����ֱ������ˮƽ����
	* @param[in]	alleehs ��ΧԪ��
	* @param[in]	tmpendEndTypes �˲���ʽ
	* @param[in]	lineEeh �ֽ���
	* @param[in]    Eleeh  ����֮ǰ��ʵ�壨ǽ���壩
	* @param[in]	anchored_vec  ê��Ƕȣ�����
	* @param[in]	matrix  ͶӰ����
	* @param[in]	moveDis ���������
	* @param[in]	lenth   ê�볤��
	* @param[in]	ori_Point   ԭ�˵�λ��
	* @param[in]	mov_Point   �޸Ķ˵�λ��
	* @param[in]	type_flag   �Ƿ���Ҫ�˲���ʽ�����FLAGE=1��ʾ����Ҫ�˲���ʽ�����FLAGE=2��ʾβ�˵�λ���޸�Ϊmov_Pointλ��
	* @param[in/out]	data ���������
	* @author	ChenDong
	* @Date:	2024/10/17
	*/
	void JudgeBarLinesLegitimate(const CVector3D  nowVec, vector<EditElementHandle*>alleehs, PIT::PITRebarEndTypes& tmpendEndTypes,
		EditElementHandle &lineEeh, EditElementHandle *Eleeh, CVector3D anchored_vec
		, const Transform matrix, double MoveDis, double lenth, DPoint3d &Point, DPoint3d &Point2, int &type_Flag);

	/*

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
