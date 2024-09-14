#pragma once

#include "CommonFile.h" 

// CSlabTieRebarSetDlg 对话框

class CSlabTieRebarSetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSlabTieRebarSetDlg)

public:
	CSlabTieRebarSetDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSlabTieRebarSetDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_SlabRebar_TieRebarSet };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	void InitUIData();

	TieReBarInfo m_tieRebarInfo;
public:
	void GetTieRebarData(TieReBarInfo&	tieRebarInfo);
	void SetTieRebarData(const TieReBarInfo&	tieRebarInfo);

	void SetDefaultRebarData();

	std::vector<PIT::ConcreteRebar> m_vecRebarData;

public:
	virtual BOOL OnInitDialog();

	CComboBox m_ComboRebarType;
	CComboBox m_ComboRebarSize;
	CButton m_CheckPatch;
	CComboBox m_ComboTieRebarMethod;

	CStatic m_staticSpace1;
	CStatic m_staticSpace2;
	afx_msg void OnLvnItemchangedListTierebar(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedSButton1();
	afx_msg void OnEnChangeSEdit1();
	afx_msg void OnCbnSelchangeSComboRebarstyle();
	afx_msg void OnCbnSelchangeSCombo1();
	afx_msg void OnCbnSelchangeSCombo2();
	afx_msg void OnBnClickedSCheck1();
	afx_msg void OnBnClickedSRadio1();
	afx_msg void OnBnClickedSRadio2();
	afx_msg void OnBnClickedSRadio3();
	afx_msg void OnBnClickedSRadio4();
	afx_msg void OnBnClickedSRadio5();
	afx_msg void OnEnChangeSEdit2();
};
