#pragma once
#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"
// CGWallRebarDlg 对话框

class CGWallRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CGWallRebarDlg)

public:
	CGWallRebarDlg(ElementHandleCR eh,CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CGWallRebarDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_Gwall };
#endif
	void InitUIData();

	std::vector<PIT::ConcreteRebar> m_vecRebarData;
public:
	
	ElementHandle GetSelectElement() { return m_ehSel; }
	void SetConcreteId(ElementId id) { m_ConcreteId = id; }

	ElementId m_ConcreteId;
	void SetListDefaultData();
	void UpdateRebarList();

	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }

	void ChangeRebarSizedata(char * rebarSize)
	{
		auto it = m_vecRebarData.begin();
		for (it; it != m_vecRebarData.end(); it++)
			strcpy(it->rebarSize, rebarSize);
	}
	void ChangeRebarTypedata(int rebarType)
	{
		auto it = m_vecRebarData.begin();
		for (it; it != m_vecRebarData.end(); it++)
			it->rebarType = rebarType;
	}
	void ChangeRebarSpacedata(double rebarSpace)
	{
		auto it = m_vecRebarData.begin();
		for (it; it != m_vecRebarData.end(); it++)
			it->spacing = rebarSpace;
	}

	CComboBox	m_ComboSize;//尺寸
	CComboBox	m_ComboType;//型号
	CEdit		m_EditSpace;//间距
	CStatic		m_static_wallname;
	WallSetInfo m_WallSetInfo;

private:
	ElementHandle m_ehSel;
	string wall_name;
public:
	void GetConcreteData(PIT::Concrete &concreteData);
	void SetListRowData(const std::vector<PIT::ConcreteRebar>& vecListData) {
		m_vecRebarData = vecListData;
	}
	void GetListRowData(std::vector<PIT::ConcreteRebar> &vecListData) {
		vecListData = m_vecRebarData;
	};
	virtual BOOL OnInitDialog();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_editPositive;
	CEdit m_editSide;
	CEdit m_editReverse;
	CButton m_hole_check;
	CEdit m_holesize_edit;
	CEdit m_editLevel;
	CMainRebarListCtrl m_listMainRebar;
	afx_msg void OnBnOKClickedGwall();
	afx_msg void OnEnKillfocusEditGpositive();
	afx_msg void OnEnKillfocusEditGside();
	afx_msg void OnEnKillfocusEditGreverse();
	afx_msg void OnBnClickedHoleGcheck();
	afx_msg void OnEnKillfocusMholesizeGedit();
	afx_msg void OnEnKillfocusEditGlevel();
	afx_msg void OnBnClickedGwall();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnStnClickedStaticWallname();
	afx_msg void OnCbnSelchangeCombo2();
};
