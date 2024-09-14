#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include <RebarDetailElement.h>

// CAddVerticalRebarDlg 对话框

class CAddVerticalRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAddVerticalRebarDlg)
	struct OriginData
	{
		map<int, vector<RebarPoint>> mapselectrebars;//选中的所有横筋的几何信息集合
		std::vector<ElementRefP> selectrebars;//选中的所有横筋的集合
		EditElementHandle ehSel;
	}m_orgdata;

	struct CalculateData
	{
		vector<RebarVertices> rebarPts;//所有点筋的几何数据集合
		vector<BrString> vecDir;//每个点筋对应的直径值
		vector<ElementRefP> allLines;//参考线段集合
		vector<RebarPoint> vecrefpt;//参考点筋点数据（用于计算起始位置和终止位置）
	}m_caldata;

	struct SetData
	{
		bool genuprebar;//是否要生成上钢筋
		bool gendownrebar;//是否要生成下钢筋
		bool genleftrebar;//是否要生成左钢筋
		bool genrightrebar;//是否要生成右钢筋

		double upDis;//上偏移值
		double downdis;//下偏移值
		double leftdis;//左偏移值
		double rightdis;//右偏移值

		bool isuseavespacing;//是否使用平均间距
		double spacing;//间距
		double diameter;//点筋直径

		ElementRefP refline;//选择的参考直线
		vector<ElementRefP> refdotrebar;//选择的参考点筋

	}m_setdata;


public:
	CAddVerticalRebarDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CAddVerticalRebarDlg();
	void DrawVerticalReabr();
	void GetInterSectPointsByRebarmap(map<int, vector<RebarPoint>>& mapselectrebars, vector<DPoint3d>& interpts);
	void GetIntersetPointsRebarWithRebar(vector<RebarPoint>& rebarpts, vector<DPoint3d>& interpts);
	void ExtendLine(DPoint3d& ptstr, DPoint3d& ptend, double dis);
	void CalculatecalData();
	void ClearLines();
	void CalculateVertexAndDrawLines();
	void CalculateVertexAndDrawLinesWhenOneRebar();
	void CalculateVertexAndDrawLinesWhenTwoRebar();
	void CalculateVertexAndDrawLinesWhenFourRebar();


	void GetSetDataFromWindow();//从界面取得设置数据
	void CalculateRefRebarPoints();
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AddVerticalRebar_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	int Typenum;
	CEdit m_edit_upmove;
	CEdit m_edit_leftmove;
	CEdit m_rightmove;
	CEdit m_edit_downmove;
	CButton m_check_isavespacing;
	CEdit m_edit_spacingdis;
	CButton m_button_selectRebar;
	CButton m_button_selcetLine;
	afx_msg void OnBnClickedOk();
	CEdit m_edit_rebardiameter;
	afx_msg void OnEnKillfocusAtEditRebardiameter();
	CButton m_check_uprebar;
	CButton m_check_downrebar;
	CButton m_check_leftrebar;
	CButton m_check_rightrebar;
	afx_msg void OnEnKillfocusAtEditUpmove();
	afx_msg void OnEnKillfocusAtEditLeftmove();
	afx_msg void OnEnKillfocusAtEditDownmove();
	afx_msg void OnEnKillfocusAtEditRightmove();
	afx_msg void OnBnClickedAdCheckUprebar();
	afx_msg void OnBnClickedAdCheckDownrebar();
	afx_msg void OnBnClickedAdCheckLeftrebar();
//	afx_msg void OnBnKillfocusAdCheckRightrebar();
	afx_msg void OnBnClickedAdCheckRightrebar();
	afx_msg void OnBnClickedAtCheckIsavespacing();
	afx_msg void OnEnKillfocusAtEditSpacingdis();
	afx_msg void OnBnClickedAtButtonSelectrefrebar();
	afx_msg void OnBnClickedAtButtonSelectrefline();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClose();
	CComboBox m_type;
	afx_msg void OnCbnSelchangeCombo1();
};
