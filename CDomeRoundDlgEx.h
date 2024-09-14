#pragma once
#include "CommonFile.h"
#include "CWallRebarListCtrl.h"
//#include "ElementAlgorithm.h"

// CDomeRoundDlgEx 对话框

class CDomeRoundDlgEx : public CDialogEx
{
	DECLARE_DYNAMIC(CDomeRoundDlgEx)

public:
	CDomeRoundDlgEx(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDomeRoundDlgEx();

	virtual BOOL OnInitDialog();

	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }

	void SetListDefaultData();

	void UpdateRebarList();

private:
	ElementHandle					m_ehSel;

	CRebarEndTypeListCtrl			m_listCtl;
	CEdit							m_EditCover;
	CEdit							m_EditZSpace;
	CEdit							m_EditRebarLevel;

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_DomeRoundDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnEnChangeEdit4();
};
