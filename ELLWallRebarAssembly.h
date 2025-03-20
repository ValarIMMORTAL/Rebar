#pragma once
class WallRebarAssembly;
#include "WallRebarAssembly.h"

class ELLWallRebarAssembly : public WallRebarAssembly
{
public:
	CWallRebarDlg *pEllWallDoubleRebarDlg;
	virtual ~ELLWallRebarAssembly()
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
	// 生成点筋
	RebarSetTag* MakeRebar_Vertical
	(
		ElementId& rebarSetId,
		BrString sizeKey,
		DgnModelRefP modelRef,
		double startOffset,  // 起始偏移
		double endOffset,    // 终点偏移
		double spacing,		 // 间距
		double dRoundRadius, // 圆的半径
		double rebarLen,		 // 钢筋长度
		int level,
		int grade,
		int DataExchange,
		bool isTwinRebar = false // 是否是并筋
	);

	RebarSetTag* MakeRebar_Round
	(
		ElementId& rebarSetId,
		BrString sizeKey,
		DgnModelRefP modelRef,
		double startOffset, // 起始偏移
		double endOffset,   // 终点偏移
		double spacing,		// 间距
		double dRoundRadius,// 圆的半径
		int level,
		int grade,
		int DataExchange,
		bool isTwinRebar = false // 是否是并筋
	);

	bool CalculateRound(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt, int nStep = 0);

	bool CalculateArc(PIT::PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);

	bool makeArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP mscArc, PIT::PITRebarEndTypes& endTypes, const vector<DPoint3d>& pts);

	bool makeBreakArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP mscArc, PIT::PITRebarEndTypes& endTypes, const vector<DPoint3d>& pts);

	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d ptStr, double dRebarLength);

	bool makeRoundRebarCurve(vector<PIT::PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d	centerPoint, double dRoundRadius);

	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

	vector<CurveVectorPtr> CreateBreakArcRange(const Transform& tran);

protected:
	virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + PIT::PITRebarAssemblyEnum::ELLWall; }
	virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"ELLWall Rebar"; }
	virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"ELLWall Rebar"; }
	virtual bool        OnDoubleClick() override;
	virtual bool        Rebuild() override;

protected:

	virtual bool	AnalyzingWallGeometricData(ElementHandleCR eh);
public:
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(ELLWallRebarAssembly, RebarAssembly)


public:
	ELLWallRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL) :
		WallRebarAssembly(id, modelRef)
#ifdef PDMSIMPORT
		, pWall(NULL)
#endif
	{
		pEllWallDoubleRebarDlg = NULL;
		m_vecRebarStartEnd.clear();

		m_ELLWallData.dRadiusOut = 0.0;
		m_ELLWallData.dRadiusInn = 0.0;
		m_ELLWallData.dHeight = 0.0;
		m_ELLWallData.centerpt = DPoint3d::From(0, 0, 0);
		m_ELLWallData.ArcDPs[0] = DPoint3d::From(0, 0, 0);
		m_ELLWallData.ArcDPs[1] = DPoint3d::From(0, 0, 0);
	};

	virtual bool	SetWallData(ElementHandleCR eh);

	virtual bool	MakeRebars(DgnModelRefP modelRef);

	void CalculateUseHoles(DgnModelRefP modelRef);

	void SetBreakAngelData(vector<PIT::BreakAngleData> data) { m_vecBreakData = data; }

private:
	struct ELLWallGeometryInfo
	{
		double   dRadiusOut;	  // 外圆半径
		double   dRadiusInn;	  // 内圆半径
		double	 dHeight;		  // 高度
		DPoint3d centerpt;		  // 弧的中心点
		DPoint3d ArcDPs[2];		  // 弧起点和终点
		UInt16       type;         //元素类型

	}m_ELLWallData;

	struct LevelInfo
	{
		int rebarLevel;
		double LevelSpacing;
	};

	struct TwinRebarDataTmp
	{
		int rebarNum;
		double spacing;
		double diameter;
	}m_reabrTwinData;

	vector<PIT::ArcSegment> m_vecArcSeg; // 预览弧形钢筋记录
	vector<PIT::BreakAngleData> m_vecBreakData;	//断开数据
	map<int, vector<EditElementHandle*>> m_vecUseHoles; //按角度的孔洞

#ifdef PDMSIMPORT
private:
	ELLWallComponent *pWall;
#endif
};

