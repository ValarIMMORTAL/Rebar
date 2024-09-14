#pragma once
#include "CommonFile.h" 

// CoverslabTieRebarSetDlg 对话框

class CoverslabTieRebarSetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CoverslabTieRebarSetDlg)

public:
	CoverslabTieRebarSetDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CoverslabTieRebarSetDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CoverslabRebar_TieRebarSet };
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

	std::vector<PIT::ConcreteRebar> m_vecRebarData;

public:
	virtual BOOL OnInitDialog();

	CComboBox m_ComboRebarType;
	CComboBox m_ComboRebarSize;
	CButton m_CheckPatch;
	CComboBox m_ComboTieRebarMethod;

	CStatic m_staticSpace1;
	CStatic m_staticSpace2;
	afx_msg void OnCbnSelchangeCComboRebarstyle();
	afx_msg void OnCbnSelchangeCCombo1();
	afx_msg void OnCbnSelchangeCCombo2();
	afx_msg void OnBnClickedCRadio1();
	afx_msg void OnBnClickedCRadio2();
	afx_msg void OnBnClickedCRadio3();
	afx_msg void OnBnClickedCRadio4();
	afx_msg void OnBnClickedCRadio5();
	afx_msg void OnBnClickedCCheck1();
	afx_msg void OnEnChangeCEdit1();
	afx_msg void OnEnChangeCEdit2();
	afx_msg void OnBnClickedCButton1();
};
