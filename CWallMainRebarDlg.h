#pragma once
#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"
#include "CWallRebarAssociatedComponentDlg.h"
#include "WallRebarAssembly.h"
#include "C_RebarTemplate.h"

// CWallMainRebar 对话框

class CWallRebarDlg;
class CWallMainRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CWallMainRebarDlg)

public:
	CWallMainRebarDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWallMainRebarDlg();
	CWallRebarAssociatedComponentDlg *m_assodlg;

	C_RebarTemplate							m_RebarTemplate;			//钢筋模板界面
	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_WallRebar_MainRebar };
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
	
	vector<PIT::BreakAngleData> GetBreakAngleData() const { return m_vecListData; }

	void SetBreakAngleData(const vector<PIT::BreakAngleData>& data) { m_vecListData = data; }

	void Set_m_DlgData();//保存界面上保护层等数据

	void Save_templateName();

	bool m_isWall;

	bool m_isFloor;

	CString m_templateName;
	
private:
	ElementHandle m_ehSel;
	WallSetInfo m_WallsetInfo;
	//char m_SlabRebarMethod[50] ;
	std::vector<PIT::ConcreteRebar> m_vecRebarData;
	vector<PIT::BreakAngleData> m_vecListData;
	CDialogEx *						pm_MainPageRebar;
	
	
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
	void GetAllRebarData(std::vector<PIT::ConcreteRebar> &vecListData);
	void GetAllWallRebarData(std::vector<PIT::ConcreteRebar> &vecListData);
	bool ParsingLineDir(EditElementHandleR ehLine);
	bool ParsingArcLineDir(EditElementHandleR ehLine);
	virtual BOOL OnInitDialog();
//	CComboBox m_ComboIsTwinBars;
	CMainRebarListCtrl m_listMainRebar;
	CEdit m_EditPositive;
	CEdit m_EditSide;
	CEdit m_EditReverse;
	CEdit m_EditLevel;
	CComboBox m_ComRebarMethod;//板的配筋方式

	PIT::DlgData  m_dlgData;//界面上的保护层等数据
	bool isUpdata = false;//是否按了刷新数据

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
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButtonBreakellipsewall();
	//afx_msg void OnBnClickedSavedata();
	afx_msg void OnBnClickedRebartemplate();
	afx_msg void OnBnClickedUpdatadlg();
	CStatic m_staticTxt1;
	CStatic m_staticTxt2;
	CStatic m_staticTxt3;
};

