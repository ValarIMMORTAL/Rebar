#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include <RebarDetailElement.h>

// CCombineRebardlg 对话框

class CCombineRebardlg : public CDialogEx
{
	DECLARE_DYNAMIC(CCombineRebardlg)

public:
	vector<RebarVertices> m_rebarPts;
	vector<ElementRefP> m_Editrebars;//需要修改的钢筋
	vector<BrString> m_vecDir;

	map<int, vector<ElementRefP>> m_mapselectrebars;
	std::vector<ElementRefP> m_selectrebars;
	std::vector<ElementRefP> m_Verticalrebars;
	vector<ElementRefP> m_allLines;
	CCombineRebardlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CCombineRebardlg();
	void SorSelcetRebar();
	void UpdateDataAndWindow();
	void CalculateVertexAndDrawLines();
	void ClearLines();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_COMBINEREBAR_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	bool isuseinters;
	CButton m_check_genvrebar;
	CButton m_button_selectVrebar;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedCheckGenvrebar();
	afx_msg void OnBnClickedButtonSelectVerrebar();
	CButton m_check_useinters;
	afx_msg void OnBnClickedCheckUseintersect();
};
void GetPtsWithBendRadius(RebarCurve curve, vector<DPoint3d>& pts, vector<double>& bendradius);