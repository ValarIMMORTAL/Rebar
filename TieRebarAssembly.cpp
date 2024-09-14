#include "_ustation.h"
#include <SelectionRebar.h>
#include "TieRebarAssembly.h"
#include "BentlyCommonfile.h"

bool TieRebarAssembly::SetWallData(ElementHandleCR eh)
{
// 	bool bRet = AnalyzingWallGeometricData(eh);
// 	if (!bRet)
// 		return false;
// 	DPoint3d ptStart = m_STwallData.ptStart;
// 	DPoint3d ptEnd = m_STwallData.ptEnd;
// 
// 	CVector3D  xVec(ptStart, ptEnd);
// 
// 	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
// 	CVector3D  yVecNegate = yVec;
// 	yVecNegate.Negate();
// 	yVecNegate.Normalize();
// 	yVecNegate.ScaleToLength(m_STwallData.width);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
// 	ptStart.Add(yVecNegate);
// 	ptEnd.Add(yVecNegate);
// 
// 	CVector3D  xVecNew(ptStart, ptEnd);
// 	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
// 	SetPlacement(placement);
// 
// 	SetSelectedElement(eh.GetElementId());
// 	SetSelectedModel(eh.GetModelRef());
 	return true;
}

void TieRebarAssembly::CalculateTransform(CVector3D& transform, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = m_componentData.sideCover *uor_per_mm;

	//假设所有都是X*X的方式变换，其余的再做相应偏移
	double diameter = RebarCode::GetBarDiameter(m_rebarSize, modelRef);		//乘以了10
	if (diameter > BE_TOLERANCE)
	{
		transform = CVector3D::From(0.0, 0.0, 0.0);
		//防止侧面保护层设置太大，大于混凝土的尺寸
		if (COMPARE_VALUES(dSideCover, m_componentData.length - m_componentData.posRebarData.HRebarData.rebarSpacing) >= 0 || COMPARE_VALUES(dSideCover, m_componentData.width - m_componentData.posRebarData.VRebarData.rebarSpacing) >= 0)
			return;

		transform.z = dSideCover + diameter * 0.5;
		transform.x = dSideCover + diameter * 0.5;
	}
}

bool TieRebarAssembly::OnDoubleClick()
{
	return false;
}

bool TieRebarAssembly::Rebuild()
{
	return false;
}

bool TieRebarAssembly::makeRebarCurve(RebarCurve & rebar, double xPos, double length, double bendRadius, double bendLen, RebarEndTypes const & endType, CVector3D const & endNormal, CMatrix3D const & mat)
{
	return false;
}

RebarSetTag * TieRebarAssembly::MakeRebars(ElementId & rebarSetId, double length, CVector3D const & endNormal, CVector3D const & vecTrans, DgnModelRefP modelRef)
{
	return nullptr;
}

bool TieRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = m_componentData.sideCover*uor_per_mm;
	CVector3D trans;
	//计算每一根钢筋的偏移量
	CalculateTransform(trans, modelRef);

	double dLength = m_componentData.length;
	double dWidth = m_componentData.height;

	int iRebarSetTag = 0;

	RebarSetTag* tag = NULL;
	CMatrix3D   mat;

	
	CVector3D	endNormal(1.0, 0.0, 0.0);	//端部弯钩方向
// 	double dRotateAngle = 45;
// 	CVector3D vec = m_STwallData.ptEnd - m_STwallData.ptStart;
// 	endNormal.Rotate(dRotateAngle / PI * 0.5, vec);	//以钢筋方向为轴旋转
	tag = MakeRebars(m_setId, dLength, endNormal, trans, modelRef);
	if (NULL != tag)
	{
		iRebarSetTag++;
		tag->SetBarSetTag(iRebarSetTag);
		rsetTags.Add(tag);
	}
	
	return (SUCCESS == AddRebarSets(rsetTags));
}


long TieRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}