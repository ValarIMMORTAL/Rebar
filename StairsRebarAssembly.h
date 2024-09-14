#pragma once
#include "CommonFile.h"
#include <RebarElements.h>
#include "RebarDetailElement.h"
#include "PITRebarCurve.h"

class CStarisRebarDlog;
class CStairsRebarAssembly : public RebarAssembly
{
	BE_DATA_REFER(BeMatrix, Placement)
	BE_DATA_VALUE(vector<ElementId>, vecSetId)			// SetId
	BE_DATA_VALUE(double, Cover) // 保护层

	struct AttachRebarInfo;
public:
	CStairsRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~CStairsRebarAssembly();

	void SetUcsMatrix(DPoint3d ptStart, DPoint3d ptEnd);

	void GetStairsFeatureParam(ElementHandleCR eh);

	void CalculateTransform(CVector3D& transform, BrStringCR sizeKey, DgnModelRefP modelRef);

	static bool IsSmartSmartFeature(EditElementHandle& eeh);

	bool MakeRebars(DgnModelRefP modelRef);

	bool makeRebarCurve
	(
		PIT::PITRebarCurve&			rebar,
		PIT::PITRebarEndTypes&		endTypes,
		CPoint3D const&         ptstr,
		CPoint3D const&         ptend
	);

	bool makeRebarCurve
	(
		PIT::PITRebarCurve&			rebar,
		PIT::PITRebarEndTypes&		endTypes,
		vector<PIT::EndType>&			vecEndtype,
		vector<CPoint3D>&			vecPoint,
		double						bendRadius,
		bool						bFlag = true
	);

	RebarSetTag* MakeRebars
	(
		vector<PIT::EndType> vecEndtype,
		CMatrix3D const&    mat,
		ElementId& rebarSetId,
		BrStringCR sizeKey,
		DgnModelRefP modelRef
	);

	RebarSetTag* MakeStepRebars
	(
		vector<PIT::EndType> vecEndtype,
		ElementId& rebarSetId,
		BrStringCR sizeKey,
		DgnModelRefP modelRef
	);

	RebarSetTag* MakeAttachRebars
	(
		vector<AttachRebarInfo>& vvecRebarInfo,
		vector<PIT::EndType>& vecEndtype,
		ElementId& rebarSetId,
		BrStringCR sizeKey,
		DgnModelRefP modelRef
	);

	RebarSetTag* MakeTieRebars
	(
		vector<PIT::EndType> vecEndtype,
		vector<vector<CPoint3D>> vvecTieRebarPts,
		ElementId& rebarSetId,
		BrStringCR sizeKey,
		DgnModelRefP modelRef
	);

	void GetRebarInfo(StairRebarInfo const & StairsRebarInfo)
	{
		m_StairsRebarInfo = StairsRebarInfo;
	}

private:
	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

	void PushAttachRebarInfo(DgnModelRefP modelRef);

private:
	// 台阶底面信息
	struct StairsDownInfo
	{
		DPoint3d ptStart; // 底面最长线方向
		DPoint3d ptEnd;
		double height; // 台阶的垂直高度
		double length; // 台阶底面的长度
	}m_StairsDownInfo;

	// 台阶侧面信息
	struct StairsSideInfo
	{
		double StepHeight; // 每级台阶的高度
		double StepWidth; // 每级台阶的宽度
		double StepLength; // 每级台阶的长度

		double minDis_Down; // 台阶最低的点到底面的距离
		double maxDis_Down; // 台阶最高的点到底面的距离

		DPoint3d ptStart;
		DPoint3d ptEnd; // 侧面上边到下边的方向

		DPoint3d ptFrontStart;	// 楼梯往里方向
		DPoint3d ptBackStart;

	}m_StairsSideInfo;

	// 首尾附加钢筋 点和端部信息
	struct AttachRebarInfo
	{
		RebarEndType::Type	strEndType;		 // 起点是否有弯钩
		RebarEndType::Type	endEndType;		 // 终点是否有弯钩
		CVector3D			vecEndNormal;	 // 起点弯钩方向
		CVector3D			vecEndNormalTmp; // 终点弯钩方向
		vector<DPoint3d>	vecPoint;		 // 点信息
	};

	StairRebarInfo m_StairsRebarInfo;//钢筋信息
	double m_stepDiameter;

	DPoint3d m_ptMainRebarUp[2];
	DPoint3d m_ptMainRebarDown[2];

	DSegment3d m_SideMaxLine; // 楼梯踏步侧面最底下的线
	vector<DSegment3d> m_vecSideXYLine; // 楼梯踏步侧面水平的线
	vector<DSegment3d> m_vecSideZLine;  // 楼梯踏步侧面Z轴的线

	vector<DPoint3d> m_vecTailPoint; // 楼梯尾部的点筋起点，从外至里

	vector<vector<CPoint3D>> m_vvecRebarPts;

	// 首尾附加钢筋 
	vector<AttachRebarInfo> m_vvecAttach_8mm;  // 附加钢筋 8mm
	vector<AttachRebarInfo> m_vvecAttach_10mm; // 附加钢筋 10mm
	vector<AttachRebarInfo> m_vvecAttach_12mm; // 尾部附加钢筋 12mm

	CStarisRebarDlog*  m_pStarisRebarDlg;

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 9; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"Beam Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"Beam Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(CStairsRebarAssembly, RebarAssembly)

};
