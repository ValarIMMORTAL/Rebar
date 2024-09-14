#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include <RebarDetailElement.h>

// CTwinBarToolDlg 对话框

class CTwinBarToolDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CTwinBarToolDlg)

public:
	CTwinBarToolDlg(vector<ElementId> vecElm, CWnd* pParent = nullptr);
	virtual ~CTwinBarToolDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_TwinBarTool };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	void SetTwinRebarVec(DVec3dCR vec) { m_vec = vec; }
	void SetDefaultRebarSize(BrStringCR strSize) { m_rebarSize = strSize; }
	void SetDefaultRebarType(BrStringCR strType) { m_rebarType = strType; }
	void DrawRefLine(bool negate = false);

private:
	vector<ElementId> m_vecElm;
	bool m_bMonitor;
	DVec3d m_vec;
	BrString m_rebarSize;
	BrString m_rebarType;
	vector<ElementId> m_refLineIds;

public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedCheck1();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnEnChangeEdit13();
	CComboBox m_CombRebarSize;
	CComboBox m_CombRebarType;
	CButton m_Check;
};
