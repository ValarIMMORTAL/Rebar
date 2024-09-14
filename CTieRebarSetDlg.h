#pragma once
#include "CommonFile.h"

// CTieRebarSetDlg 对话框

class CTieRebarSetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CTieRebarSetDlg)

public:
	CTieRebarSetDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CTieRebarSetDlg();

	void SetDisplayCheck(bool isDisplay)
	{
		m_isDisplayCheck = isDisplay;
	}

	bool GetContin()
	{
		return m_isContin;
	}

	void SetContin(bool isContin)
	{
		m_isContin = isContin;
	}

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_WallRebar_TieRebarSet };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	void InitUIData();

	TieReBarInfo m_tieRebarInfo;

	bool					   m_isContin;
	bool					   m_isDisplayCheck;

	CButton					   m_IsContinCheck;

public:
	void GetTieRebarData(TieReBarInfo&	tieRebarInfo);
	void SetTieRebarData(const TieReBarInfo&	tieRebarInfo);

	std::vector<PIT::ConcreteRebar> m_vecRebarData;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedRadio1();
	afx_msg void OnBnClickedRadio3();
	afx_msg void OnBnClickedRadio2();
	afx_msg void OnBnClickedRadio4();
	afx_msg void OnBnClickedCheck1();
	CComboBox m_ComboRebarType;
	CComboBox m_ComboRebarSize;
	CButton m_CheckPatch;
	CComboBox m_ComboTieRebarMethod;
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnCbnSelchangeCombo2();
	CStatic m_staticSpace1;
	CStatic m_staticSpace2;
	afx_msg void OnEnKillfocusEdit1();
	afx_msg void OnEnKillfocusEdit2();
	afx_msg void OnBnClickedRadio5();
	afx_msg void OnCbnSelchangeComboRebarstyle();
	afx_msg void OnBnClickedCheck2();
};
