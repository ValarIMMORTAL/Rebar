﻿#pragma once


// CStretchStirrupRebarToolDlg 对话框

class CStretchStirrupRebarToolDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CStretchStirrupRebarToolDlg)

public:
	CStretchStirrupRebarToolDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CStretchStirrupRebarToolDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_STRETCHSTIRRUPREBAR };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
