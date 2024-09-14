#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include <RebarDetailElement.h>
// CRebarEditDlg 对话框

class CRebarEditDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRebarEditDlg)

public:
	enum  AnchorInType//锚入方式
	{
		Straight,
		Curve
	};
	struct RebarEditData
	{
		double movelenth;
		double l0lenth;
		bool isend;
		double angle;
		AnchorInType type;
		bool isArc;
	}m_rebareditdata;
	vector<RebarVertices> m_rebarPts;
	vector<BrString> m_vecDir;

	std::vector<EditElementHandle*> m_Holeehs;
	std::vector<ElementRefP> m_selectrebars;
	std::vector<ElementRefP> m_Anchorinrebars;
	vector<ElementRefP> m_allLines;
	CRebarEditDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CRebarEditDlg();
	void UpdateWindow();
	void UpdateDataAndWindow();
	void CalculateVertexAndDrawLines();
	void CalculterArcVertexAndDrawLines();
	void ClearLines();
	void CalcWallHoles();
	void SetIsEnd(bool isEnd);
	void SetIsArc(bool isArc) { m_rebareditdata.isArc = isArc; }
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_REBAREDIT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	
	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edit_movelenth;
	CEdit m_edit_L0Lenth;
	CComboBox m_combo_type;
	afx_msg void OnEnKillfocusEditRmovelenth();
	afx_msg void OnEnKillfocusEditRl0lenth();
	afx_msg void OnCbnSelchangeComboRtype();
	afx_msg void OnBnClickedButtonRselectrebars();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CButton m_check_useend;
	afx_msg void OnBnClickedCheckRuseend();
	CEdit m_edit_rotateangle;
	afx_msg void OnBnClickedRbuttonRotate();
	afx_msg void OnEnKillfocusEditRrotate();
};
