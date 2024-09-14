#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"


// CInsertRebarDlg 对话框
class CInsertRebarAssemblyColumn;
class CColInsertRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CColInsertRebarDlg)

public:

	CColInsertRebarDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CColInsertRebarDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_InsertRebarCol };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

public:
	virtual BOOL OnInitDialog();
	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }

	void SetBashElement(ElementHandleCR eh) { m_basis = eh; }

	void SetConcreteId(ElementId id) { m_ConcreteId = id; }

	void SetDiagParam(InsertRebarInfo& stInserRebarInfo);

	void CoverOnBnClickedOk();

	void SetDefaultInfo(InsertRebarInfo& stInsertRebarInfo);

	void SetColInsertRebar(CInsertRebarAssemblyColumn* pColInsertRebarAssembly)
	{
		m_pColInsertRebarAssembly = pColInsertRebarAssembly;
	}

	void SetRebarSizeAndType(CComboBox& CombRebarSize, CComboBox& CombRebarType);

private:
	ElementId				m_ConcreteId;
	ElementHandle			m_ehSel;
	ElementHandle			m_basis;

	CInsertRebarAssemblyColumn*	m_pColInsertRebarAssembly;

	InsertRebarInfo m_InsertRebarInfo;

	CComboBox	m_CombElementShape;
	CEdit		m_EditElementLength;
	CEdit		m_EditElementWidth;
	CEdit		m_EditProtectiveLayer;
	CComboBox	m_CombVerticalSize;
	CComboBox	m_CombVerticalType;
	CComboBox	m_CombHoopRebarSize;
	CComboBox	m_CombHoopRebarType;
	CEdit       m_EditLongSurfaceNum;
	CEdit       m_EditShortSurfaceNum;
	CComboBox	m_CombRebarSize;
	CComboBox   m_CombRebarType;
	CEdit		m_EditEmbed;
	CEdit		m_EditExpand;
	CComboBox	m_CombEndStyle;
	CComboBox	m_CombHookDirection;
	CEdit		m_EditRotationAngle;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnEnChangeEdit8();
	afx_msg void OnEnChangeEdit7();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnCbnSelchangeCombo10();
	afx_msg void OnEnChangeEdit5();
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnCbnSelchangeCombo4();
	afx_msg void OnCbnSelchangeCombo5();
	afx_msg void OnEnChangeEdit6();
	afx_msg void OnCbnSelchangeCombo7();
	afx_msg void OnCbnSelchangeCombo8();
	afx_msg void OnEnChangeEdit9();
	afx_msg void OnCbnSelchangeCombo9();
	afx_msg void OnEnChangeEdit10();
	afx_msg void OnEnChangeEdit3();
};
