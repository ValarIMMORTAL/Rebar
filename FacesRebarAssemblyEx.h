#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	ǽ���
*	Project:		��ά����ͼ��Ŀ
*	Author:			LiuXiang
*	Date:			2021/06/08
	Version:		V1.0
*	Description:	FacesRebarAssemblyEx
*	History:
*	1. Date:		2021/06/08
*	Author:			LiuXiang
*	Modification:	create file
*	Edit�� Liu Silei 2025/2/24
*
**************************************************************/
#include <RebarElements.h>
#include "CommonFile.h"
#include "PITRebarEndType.h"
#include "PITRebarCurve.h"
#include "PITArcSegment.h"
#include "PITRebarAssembly.h"

class FacesRebarAssemblyEx : public PIT::PITRebarAssembly
{
public:
	enum FaceType
	{
		Plane,
		CamberedSurface,
		other,
	};
	// �洢ÿ�� MakeRebars ���ɵĸֽ���Ϣ
	struct RebarSetInfo
	{
		ElementHandle face; // �ֽ�������Ԫ�ؾ��
		DVec3d faceNormal; // �����淨��
		std::vector<PIT::PITRebarCurve> rebarCurves; // �ֽ������б�
		BrString sizeKey; // �ֽ�ߴ�
		RebarEndTypes endTypes; // �˲�����
		double diameter; // �ֽ�ֱ��
		double spacing; // �ֽ���
		double adjustedSpacing; // ������ļ��
		int numRebar; // �ֽ�����
		RebarSetP rebarSet; // �ֽ
		int level; // �ֽ�㼶
		int grade; // �ֽ�ȼ�
		int dataExchange; // ���ݽ�����ʶ
	};

	double scalingFactor = 1;
	BE_DATA_REFER(BeMatrix, Placement) //��ǰ�ֲ�����ԭ��
	BE_DATA_VALUE(PIT::Concrete, Concrete) //������
	BE_DATA_VALUE(vector<PIT::ConcreteRebar>, MainRebars) //����
	BE_DATA_VALUE(vector<vector<PIT::EndType> >, vecEndTypes) //�˲���ʽ
	BE_DATA_VALUE(vector<ElementId>, SetIds) //SetId
	BE_DATA_VALUE(DVec3d, faceNormal) //ƽ�淨��
	BE_DATA_VALUE(bool, isReverse)
	BE_DATA_VALUE(FaceType, faceType)
	BE_DATA_VALUE(vector<DPoint3d>, vecFrontPts) //ǰ���ߵ����е�

public:
	std::vector<EditElementHandle*> m_Holeehs; //�׶�
	std::vector<EditElementHandle*> m_Negs; //��ʵ��
	std::vector<EditElementHandle*> m_useHoleehs; //ɸѡ��Ŀ׶��͸�ʵ��
	std::vector<DPoint3d> m_vecRebarPtsLayer; //���׶���ȡǰ�����иֽ������
	std::vector<DPoint3d> m_vecTwinRebarPtsLayer; //���׶���ȡǰ�����в��������

	ElementHandle m_face;
	EditElementHandle* m_Solid;
	std::vector<ElementHandle> m_vecElm;
	bool m_slabUpFace;
	vector<vector<DSegment3d>> m_vecRebarStartEnd; //����˿׶������е�
	vector<vector<PIT::PITRebarCurve>> m_vecRebarCurvePt; //�ֽ�Ԥ���ߵ����е�
	vector<ElementRefP> m_allPreViewEehs; //Ԥ����ť���º�����иֽ��ߵ�Ԫ��
	vector<vector<DSegment3d>> m_vecAllRebarStartEnd; //δ��ܿ׶������е�
	void ClearLines();
	bool m_isClearLine = true;

public:
	FacesRebarAssemblyEx(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~FacesRebarAssemblyEx();

protected:
	std::vector<RebarSetInfo> m_rebarSetInfos; // �洢��� MakeRebars ���õ���Ϣ

	void Init();
	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);
	bool IsPointHigherXYZ(const DPoint3d& p1, const DPoint3d& p2);
	DPoint3d RangeMidPoint(const DRange3d &range);
	DPoint3d GetEhCenterPt(ElementHandle eh);
	void DrawPoint(const DPoint3d& point, int color, EditElementHandle& eehPoint, DgnModelRefP modelRef);
	void DrawPreviewLines();
	void DrawAllRebars(DgnModelRefP modelRef, RebarSetTagArray& rsetTags);
	void ReverseMainRebars(bool isParallelToY);

protected:
	virtual int GetPolymorphic() const override
	{
		return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::Face;
	}

public:
	static FaceType JudgeFaceType(EditElementHandleR eehFace, DgnModelRefP modelRef);

	virtual bool AnalyzingFaceGeometricData(EditElementHandleR eeh) { return true; }

	virtual bool MakeRebars(DgnModelRefP modelRef) { return true; }

	void SetRebarEndTypes(vector<PIT::EndType> const& vvecEndTypes);

	bool m_UseXOYDir; //�Ƿ�Ӧ��XOY������Ϊ��ֽ����ɷ���
};
