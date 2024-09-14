﻿#pragma once
#include "CommonFile.h"
#include "ListCtrlEx.h"

// CSlabRebarLapOptionDlg 对话框

class CSlabRebarLapOptionDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSlabRebarLapOptionDlg)

public:
	CSlabRebarLapOptionDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSlabRebarLapOptionDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_SlabRebar_LapOption };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	void InitUIData();

	std::vector<PIT::LapOptions> m_vecLapOption;

public:
	void SetListDefaultData();
	void UpdateLapOptionList();

public:
	void SetListRowData(const std::vector<PIT::LapOptions>& vecListData) {
		m_vecLapOption = vecListData;
	}
	void GetListRowData(std::vector<PIT::LapOptions> &vecListData) {
		vecListData = m_vecLapOption;
	};

public:
	virtual BOOL OnInitDialog();
	ListCtrlEx::CListCtrlEx m_ListLapOption;
	afx_msg void OnLvnItemchangedListLapoption(NMHDR *pNMHDR, LRESULT *pResult);
};
