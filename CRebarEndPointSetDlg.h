#pragma once
#include "CommonFile.h"

// CRebarEndPointSetDlg 对话框

class CRebarEndPointSetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRebarEndPointSetDlg)

public:
	CRebarEndPointSetDlg(int endType,CWnd* pParent = nullptr);
	virtual ~CRebarEndPointSetDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ENDTYPESET };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	PIT::EndType::RebarEndPointInfo GetRebarEndPointInfo() { return m_endPtInfo; }
	void SetRebarEndPointInfo(PIT::EndType::RebarEndPointInfo endPtInfo) {m_endPtInfo = endPtInfo; }
	void SetRebarSize(BrString rebarSize) { m_strRebarSize = rebarSize; }
private:
	int m_endType;	//
	bool m_bDrawPic;
	PIT::EndType::RebarEndPointInfo m_endPtInfo;
	BrString m_strRebarSize;
	double m_rebarDia; //直径
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
	CStatic m_stEndType1;
	CStatic m_stEndType2;
	CStatic m_stEndType3;
	CStatic m_stEndType4;
	CStatic m_stEndType5;
	CStatic m_stEndType6;
	afx_msg void OnBnClickedCheckEndtype1();
	afx_msg void OnBnClickedCheckEndtype3();
	afx_msg void OnBnClickedCheckEndtype4();
	afx_msg void OnBnClickedButtonLoad();
	CStatic m_stEndTypePic;
	CButton m_overrideTail;

	afx_msg void OnPaint();
	afx_msg void OnBnClickedCheckOverridetail();
	afx_msg void OnEnChangeEditEndtype2();
	afx_msg void OnEnChangeEditEndtype3();
};
