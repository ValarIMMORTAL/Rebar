#pragma once

class WallRebarAssembly;
class GWallRebarAssembly;
#include "WallRebarAssembly.h"
#include "GWallRebarAssembly.h"

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