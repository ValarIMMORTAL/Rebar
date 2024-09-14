#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"

// CBeamHoopRebarDlg 对话框

class CBeamHoopRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBeamHoopRebarDlg)

public:
	CBeamHoopRebarDlg(ElementHandle ehSel, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CBeamHoopRebarDlg();

	void SetDefaultData(BeamRebarInfo::BeamCommHoop& m_stBeamCommHoop, BeamRebarInfo::BeamRebarHoop& m_stBeamRebarHoop);

	void SetButtonFontSize();

	BOOL OnInitDialog();

	void SetDefaultData();

	void UpdateCommList();

	void SetCommDataEnable(bool bFlag);

	void SetBeamHoopData(ElementId condit);

	void UpdateCommData(const BeamRebarInfo::BeamCommHoop& stBeamCommHoop);

	void UpdateRebarData(const BeamRebarInfo::BeamRebarHoop& stBeamRebarHoop);

	void SetRebarSizeAndType(CComboBox& CombRebarSize, CComboBox& CombRebarType, char* pRebarSize, int nRebarType);

	CBeamListCtrl								m_ListBeamCommHoop;
	CBeamListCtrl								m_ListBeamRebarHoop;
	vector<BeamRebarInfo::BeamCommHoop>			m_vecBeamCommHoop;
	vector<BeamRebarInfo::BeamRebarHoop>		m_vecBeamRebarHoop;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_BeamHoopRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

private:
	ElementHandle						m_ehSel;
	
	CEdit								m_EditLabelCtl;
	CEdit								m_EditSpacingCtl;
	CEdit								m_EditStartPosCtl;
	CEdit								m_EditStart_N_DeepCtl;
	CEdit								m_EditEndPosCtl;
	CEdit								m_EditEnd_N_DeepCtl;
	CEdit								m_EditDescript;
	CComboBox							m_CombRebarSizeCtl;
	CComboBox							m_CombRebarTypeCtl;
	CComboBox							m_CombPostionCtl;
	
	CEdit								m_EditLabelCtl2;
	CEdit								m_EditOffsetCtl;
	CEdit								m_EditStartRotateCtl;
	CEdit								m_EditEndRotateCtl;
	CComboBox							m_CombStartEndTypeCtl;
	CComboBox							m_CombFnishEndTypeCtl;

	CString								m_EditLabel;
	double								m_EditSpacing;
	double								m_EditStartPos;
	double								m_EditStart_N_Deep;
	double								m_EditEndPos;
	double								m_EditEnd_N_Deep;
	CString								m_CombRebarSize;
	CString								m_CombRebarType;
	CString								m_CombPostion;

	CString								m_EditLabel2;
	double								m_EditOffset;
	CString								m_EditStartRotate;
	CString								m_EditEndRotate;
	CString								m_CombStartEndType;
	CString								m_CombFnishEndType;

	int									m_nCurCommLine;

	CButton								m_BtnCommAdd;
	CButton								m_BtnCommDel;
	CButton								m_BtnRebarAdd;
	CButton								m_BtnRebarDel;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangeEdit2();
	afx_msg	void OnCbnSelchangeCombo1();
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnEnChangeEdit3();
	afx_msg	void OnEnChangeEdit6();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnEnChangeEdit4();
	afx_msg void OnEnChangeEdit7();
	afx_msg void OnEnChangeEdit8();
	afx_msg void OnEnChangeEdit9();
	afx_msg void OnCbnSelchangeCombo4();
	afx_msg void OnCbnSelchangeCombo5();
	afx_msg void OnEnChangeEdit11();
	afx_msg void OnEnChangeEdit12();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton6();
};