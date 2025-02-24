#pragma once
#include "../../GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrlNew.h"
#include "CWallRebarAssociatedComponentDlgNew.h"
#include "WallRebarAssemblyNew.h"
// CWallMainRebar 对话框

class CWallRebarDlgNew;
class CWallMainRebarDlgNew : public CDialogEx
{
	DECLARE_DYNAMIC(CWallMainRebarDlgNew)

public:
	CWallMainRebarDlgNew(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWallMainRebarDlgNew();
	CWallRebarAssociatedComponentDlgNew *m_assodlg;
	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CNWallRebar_MainRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	void InitUIData();


public:
	DSegment3d m_seg;//板配筋的方向线段
	ArcRebar  mArcLine;

	double m_height;//板的Z坐标
	void SetListDefaultData();
	void UpdateRebarList();
	void getWallSetInfo(WallSetInfo& wallsetinfo);
	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }
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

private:
	ElementHandle m_ehSel;
	WallSetInfo m_WallsetInfo;
	//char m_SlabRebarMethod[50] ;
	std::vector<PIT::ConcreteRebar> m_vecRebarData;

	CDialogEx *						pm_MainPageRebar;

	CButton			m_CheckIsCutRebar;

	CutRebarInfo	m_stCutRebarInfo;
public:

	void SavePtr(CDialogEx* Pthis)
	{
		pm_MainPageRebar = Pthis;
	}
	void getTieRebarData(const TieReBarInfo & tieRebarInfo);
	void getTwinRowData(const TwinBarSet::TwinBarInfo & twinRebarInfo);

//	void PreviewRebarLines();
	void GetConcreteData(PIT::Concrete &concreteData);
	void SetListRowData(const std::vector<PIT::ConcreteRebar>& vecListData) {
		m_vecRebarData = vecListData;
	}
	void GetListRowData(std::vector<PIT::ConcreteRebar> &vecListData) {
		vecListData = m_vecRebarData;
	};
	bool ParsingLineDir(EditElementHandleR ehLine);
	bool ParsingArcLineDir(EditElementHandleR ehLine);
	virtual BOOL OnInitDialog();
//	CComboBox m_ComboIsTwinBars;
	CMainRebarListCtrlNew m_listMainRebar;
	CEdit m_EditPositive;
	CEdit m_EditSide;
	CEdit m_EditReverse;
	CEdit m_EditLevel;
	CEdit m_EditCutLength1;
	CEdit m_EditCutLength2;
	CEdit m_EditCutLength3;
	CComboBox m_ComRebarMethod;//板的配筋方式

	void GetCutRebarInfo(CutRebarInfo& stCutRebarInfo)
	{
		stCutRebarInfo = m_stCutRebarInfo;
	}

	void SetCutRebarInfo(CutRebarInfo& stCutRebarInfo)
	{
		m_stCutRebarInfo = stCutRebarInfo;
	}

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
	//afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnEnChangeEdit7();
	afx_msg void OnEnChangeEdit6();
	afx_msg void OnEnChangeEdit5();
};
