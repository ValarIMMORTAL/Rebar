#pragma once
#include "CWallRebarListCtrl.h"

// CoverslabRebarTwinDlg 对话框

class CoverslabRebarTwinDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CoverslabRebarTwinDlg)

public:
	CoverslabRebarTwinDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CoverslabRebarTwinDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CoverslabRebar_TwinBarSet };

#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	void InitUIData();

	std::vector<TwinBarSet::TwinBarLevelInfo> m_vecTwinBar;//小开关

public:
	std::vector<PIT::ConcreteRebar> m_vecRebarData;

	void SetListDefaultData();
	void UpdateTwinBarsList();
	CString GetTwinBarLevelName(int rebarLevel);
	// void GetTwinBarInfo(TwinBarSet::TwinBarInfo& twInfo);

public:

	void SetDefaultRebarData();

	void SetListRowData(const std::vector<TwinBarSet::TwinBarLevelInfo>& vecListData) {
		m_vecTwinBar = vecListData;
	}
	void GetListRowData(std::vector<TwinBarSet::TwinBarLevelInfo> &vecListData) {
		vecListData = m_vecTwinBar;
	};
public:
	CTwinBarListCtrl m_ListTwinBars;
	virtual BOOL OnInitDialog();
	afx_msg void OnLvnItemchangedCListTwinbarset(NMHDR *pNMHDR, LRESULT *pResult);
};
