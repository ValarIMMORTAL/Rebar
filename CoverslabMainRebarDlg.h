#pragma once
#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"

// CoverslabMainRebarDlg 对话框

class CoverslabMainRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CoverslabMainRebarDlg)

public:
	CoverslabMainRebarDlg(ElementHandleCR eh, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CoverslabMainRebarDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CoverslabRebar_MainRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	void InitUIData();

	PIT::Concrete m_concrete;
	ElementHandle m_ehSel;
	std::vector<PIT::ConcreteRebar> m_vecRebarData;//m_vecRebarData为接收全局的g_vecRebarData来进行运算
public:
	void SetListDefaultData();
	void UpdateRebarList();
	virtual BOOL OnInitDialog();

public:
	void GetConcreteData(PIT::Concrete& concreteData);
	void SetConcreteData(PIT::Concrete& concreteData);
	void SetListRowData(const std::vector<PIT::ConcreteRebar>& vecListData) {
		m_vecRebarData = vecListData;
	}
	void GetListRowData(std::vector<PIT::ConcreteRebar> &vecListData) {
		vecListData = m_vecRebarData;
	};
	CComboBox m_ComboIsTwinBars;
	CMainRebarListCtrl m_listMainRebar;
	CEdit m_EditPositive;
	CEdit m_EditSide;
	CEdit m_EditReverse;
	CEdit m_EditLevel;

	int	m_CoverslabType;

	afx_msg void OnEnChangeCEdit1();
	afx_msg void OnEnChangeCEdit2();
	afx_msg void OnEnChangeCEdit3();
	afx_msg void OnEnChangeCEdit4();
};
