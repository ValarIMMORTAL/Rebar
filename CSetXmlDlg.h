#pragma once
#include "CommonFile.h"

// CSetXmlDlg 对话框

class CSetXmlDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSetXmlDlg)

public:
	CSetXmlDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSetXmlDlg();

	virtual BOOL OnInitDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_SetXml };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeComboXmlfilename();

private:
	const std::list<CString> m_listFileName = { _T("RebarCode_zhongguangheC40.xml"), _T("RebarCode_zhongguangheC50.xml"), _T("RebarCode_zhongguangheC60.xml") };

	CComboBox m_comboXmlName;

	WString m_xmlFileName; //文件名
public:
	afx_msg void OnBnClickedOk();
};
