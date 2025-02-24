﻿#pragma once
#include "CWallRebarListCtrlNew.h"

// CTwinBarSet 对话框

class CTwinBarSetDlgNew : public CDialogEx
{
	DECLARE_DYNAMIC(CTwinBarSetDlgNew)

public:
	CTwinBarSetDlgNew(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CTwinBarSetDlgNew();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CNWallRebar_TwinBarSet };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	void InitUIData();

	std::vector<TwinBarSet::TwinBarLevelInfo> m_vecTwinBar;

public:
	void SetListDefaultData();
	void UpdateTwinBarsList();
	CString GetTwinBarLevelName(int rebarLevel);

	std::vector<PIT::ConcreteRebar> m_vecRebarData;
//	void GetTwinBarInfo(TwinBarSet::TwinBarInfo& twInfo);
public:

	void SetListRowData(const std::vector<TwinBarSet::TwinBarLevelInfo>& vecListData) {
		m_vecTwinBar = vecListData;
	}
	void GetListRowData(std::vector<TwinBarSet::TwinBarLevelInfo> &vecListData) {
		vecListData = m_vecTwinBar;
	};
public:
	CTwinBarListCtrlNew m_ListTwinBars;
	virtual BOOL OnInitDialog();

//	CButton m_CheckIsStaggered;
//	CButton m_CheckIsTwinBars;
//	afx_msg void OnBnKillfocusCheck1();
//	afx_msg void OnBnKillfocusCheck2();
//	afx_msg void OnBnClickedCheck1();
//	afx_msg void OnBnClickedCheck2();
};
