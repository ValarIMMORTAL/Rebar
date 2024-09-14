#pragma once

#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"

// CSlabMainRebarDlg 对话框

class CSlabRebarDlg;
class CSlabMainRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSlabMainRebarDlg)

public:
	CSlabMainRebarDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSlabMainRebarDlg();
	// CSlabRebarAssociatedComponent *m_assodlg;
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_SlabRebar_MainRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	void InitUIData();
	WallSetInfo m_SlabsetInfo;
	std::vector<PIT::ConcreteRebar> m_vecRebarData;//m_vecRebarData为接收全局的g_vecRebarData来进行运算

	CSlabRebarDlg *						pm_MainPageRebar;//保存CSlabRebarDlg对话框的指针

public:
	void SetListDefaultData();
	void UpdateRebarList();
	void getSlabSetInfo(WallSetInfo& Slabsetinfo);

public:
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

	void GetConcreteData(PIT::Concrete &concreteData);
	void SetListRowData(const std::vector<PIT::ConcreteRebar>& vecListData) {
		m_vecRebarData = vecListData;
	}
	void GetListRowData(std::vector<PIT::ConcreteRebar> &vecListData) {
		vecListData = m_vecRebarData;
	};
	virtual BOOL OnInitDialog();
	CComboBox m_ComboIsTwinBars;
	CMainRebarListCtrl m_listMainRebar;
	CEdit m_EditPositive;
	CEdit m_EditSide;
	CEdit m_EditReverse;
	CEdit m_EditLevel;
	afx_msg void OnEnKillfocusEdit1();
	afx_msg void OnEnKillfocusEdit2();
	afx_msg void OnEnKillfocusEdit3();
	afx_msg void OnEnKillfocusEdit4();

	afx_msg LRESULT OnComboBoxDataChange(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedButton1();

	// 是否规避孔洞设置
	CButton m_hole_check;
	// 忽略孔洞大小设置
	CEdit m_mholesize_edit;
	afx_msg void OnBnClickedHoleCheck();
	afx_msg void OnEnKillfocusMholesizeEdit();
	afx_msg void OnBnClickedButton2();
};
