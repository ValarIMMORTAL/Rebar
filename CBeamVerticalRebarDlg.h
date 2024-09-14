#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"
// CBeamVerticalRebarDlg 对话框

class CBeamVerticalRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBeamVerticalRebarDlg)

public:
	CBeamVerticalRebarDlg(ElementHandle ehSel, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CBeamVerticalRebarDlg();

	void SetRebarSizeAndType(CComboBox& CombRebarSize, CComboBox& CombRebarType, char* pRebarSize, int nRebarType);

	BOOL OnInitDialog();

	void SetDefaultData(vector<BeamRebarInfo::BeamAreaVertical>& vecBeamAreaData, vector<BeamRebarInfo::BeamRebarVertical>& vecBeamRebarData);

	void SetButtonFontSize();

	void UpdateListBeamInfo();

	void SetCommDataEnable(bool bFlag);

	void SetAreaDataEnable(BOOL FALSE);

	void UpdateAreaData(BeamRebarInfo::BeamAreaVertical& stBeamAreaData);

	void UpdateRebarData(BeamRebarInfo::BeamRebarVertical& stBeamRebarVertical);

	void SetBeamVerticalData(ElementId condit);

	CBeamListCtrl								m_ListBeamAreaInfo;
	vector<BeamRebarInfo::BeamAreaVertical>		m_vecBeamAreaData;
	vector<BeamRebarInfo::BeamRebarVertical>	m_vecBeamRebarData;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_BeamVerticalRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

private:
	ElementHandle							m_ehSel;
	
	CString									m_EidtLabel;
	double									m_EidtSpace;
	double									m_EidtStartOffset;
	int										m_EidtTotNum;
	double									m_EidtEndOffset;
	CString									m_CombPosition;


	CString									m_EidtLabel2;
	double									m_EidtLeftOffset;
	double									m_EidtRightOffset;
	CString									m_EditLeftRotateAngle;
	CString									m_EditRightRotateAngle;
	int										m_EidtTotNum2;
	CString									m_CombRebarSize;
	CString									m_CombRebarType;
	CString									m_CombPosition2;
	CString									m_CombLeftEndStyle;
	CString									m_CombRightEndStyle;

	int										m_nCurAreaLine;		// 当前选中区域列表数据的行

	bool									m_bChange;

	CEdit									m_EidtLabelCtl;
	CEdit									m_EidtSpaceCtl;
	CEdit									m_EidtStartOffsetCtl;
	CEdit									m_EidtTotNumCtl;
	CEdit									m_EidtEndOffsetCtl;
	CComboBox								m_CombPositionCtl;


	CEdit									m_EidtLabelCtl2;
	CEdit									m_EidtLeftOffsetCtl;
	CEdit									m_EidtRightOffsetCtl;
	CEdit									m_EditLeftRotateAngleCtl;
	CEdit									m_EditRightRotateAngleCtl;
	CEdit									m_EidtTotNumCtl2;
	CComboBox								m_CombRebarSizeCtl;
	CComboBox								m_CombRebarTypeCtl;
	CComboBox								m_CombPositionCtl2;
	CComboBox								m_CombLeftEndStyleCtl;
	CComboBox								m_CombRightEndStyleCtl;

	CButton									m_ButAreaAdd;
	CButton									m_ButAreaDel;
	CButton									m_ButRebarAdd;
	CButton									m_ButRebarDel;

	CString									m_arrPosition[3];

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnEnChangeEdit7();
	afx_msg void OnEnChangeEdit3();
	afx_msg void OnEnChangeEdit8();
	afx_msg void OnEnChangeEdit4();
	afx_msg void OnEnChangeEdit9();
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnEnChangeEdit10();
	afx_msg void OnEnChangeEdit5();
	afx_msg void OnCbnSelchangeCombo11();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnCbnSelchangeCombo4();
	afx_msg void OnEnChangeEdit11();
	afx_msg void OnCbnSelchangeCombo6();
	afx_msg void OnEnChangeEdit12();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton1();
};
