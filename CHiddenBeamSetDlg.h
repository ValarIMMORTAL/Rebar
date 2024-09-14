#pragma once
#include "CommonFile.h"
#include <RebarDetailElement.h>

// CHiddenBeamSetDlg 对话框

class CHiddenBeamSetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CHiddenBeamSetDlg)

public:
	CHiddenBeamSetDlg(ElementHandleCR eh,bool bEmbededCloumn, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CHiddenBeamSetDlg();

	void InitCheck();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_HiddenBeam };
#endif
	
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	ElementHandle m_ehSel;
	double		  m_BeamLen;
	bool m_bEmbededCloumn;
	ElementRefP m_pElmDownFace{nullptr};
	bool bDownFaceDrawn {false};
	DVec3d m_vecNormal;
	ElementRefP m_elmRef{nullptr};	//梁元素

public:
	bool SetDownFace(ElementRefP pElmDownFace);

	DVec3d GetFaceNormal();
	bool DrawBeam(bool negate);
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnBnClickedCheck1();
	CComboBox m_CombDownFace;
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton2();
	CButton m_Check;
	afx_msg void OnBnClickedCheck5();
	afx_msg void OnClose();
	afx_msg void OnBnClickedButton12();
};
