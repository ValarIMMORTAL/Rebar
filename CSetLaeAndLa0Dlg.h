#pragma once
#include "CSetLae.h"
#include "CSetLa0.h"
#include "XmlHelper.h"
#include <iostream>
// CSetLaeAndLa0Dlg 对话框
extern GlobalParameters g_globalpara;
extern int g_global_stander;

class CSetLaeAndLa0Dlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSetLaeAndLa0Dlg)

public:
	CSetLaeAndLa0Dlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSetLaeAndLa0Dlg();

	virtual BOOL OnInitDialog();
public:
	//static int    g_global_stander ;//现在选择的钢筋标准 0国标，1是核工业标
	int			  m_CurSelTab;//现在选择的界面下标 0是搭接，1是锚固
	CString       m_Str_Stander;		//钢筋标准
	CComboBox     m_Stander_ComboBox;		//钢筋标准复合框
	CSetLa0       m_La0Dlg;	//搭接界面
	CSetLae       m_LaeDlg;	//锚固界面
	CDialog*	  pDialog[2];	//用来保存对话框对象指针

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_SetLaeAndLa0 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnTcnSelchangeTabLa(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnStandard_Selection();//结构标准选择
	CTabCtrl m_tab;//界面切换

};
