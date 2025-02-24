#pragma once
#include "CWallMainRebarDlgNew.h"
//#include "CWallRebarLapOptionDlg.h"
#include "CWallRebarEndTypeDlgNew.h"
//#include "CWallRebarAssociatedComponentDlg.h"
#include "CTwinBarSetDlgNew.h"
#include "CTieRebarSetDlgNew.h"
#include "PITACCRebarAssemblyNew.h"
//#include "Mstn\MdlApi\mfcDialog\CBModalDialog.h"
//#include "Mstn\MdlApi\mfcDialog\CBModelessDialog.h"
// CWallRebar 对话框

class CWallRebarDlgNew : public CDialogEx
{
	DECLARE_DYNAMIC(CWallRebarDlgNew)

public:
	CWallRebarDlgNew(ElementHandleCR eh,CWnd* pParent = nullptr);   // 标准构造函数
//	CWallRebarDlg(UINT Id, CWnd* pParent = nullptr);
	virtual ~CWallRebarDlgNew();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CNWallRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void PostNcDestroy();
	DECLARE_MESSAGE_MAP()
public:
	CTabCtrl m_tab;

//	virtual StatusInt   Create();

 public:
	 static void RefreshWallRebars(ElementId testid, EditElementHandleR eeh);
	 void SetSelectElement(ElementHandleCR eh) { ehSel = eh; }
	 ElementHandle GetSelectElement() { return ehSel; }
	 void PreviewRebarLines();

// 	void SetRebarInfo(const vector<PIT::ConcreteRebar>& vecWallRebar);
// 	void SetLapOptionInfo(const vector<PIT::LapOptions>& vecLapOption);
// 	void SetEndTypeInfo(const vector<PIT::EndType>& vecEndType);
//	 void SetACInfo(const vector<PIT::AssociatedComponent>& vecACData);
// private:
// 	WallRebarInfo wallRebarInfo;
	 void SetConcreteId(ElementId id) { m_ConcreteId = id; }

private:
	WallRebarAssemblyNew * m_WallRebarLinesPtr;//预览按钮使用
	PIT::ACCRebarAssemblyNew*  m_WallACCRebarLinesPtr;//ACC预览按钮使用

	ElementId		m_ConcreteId;
	CutRebarInfo	m_stCutRebarInfo;
public:
	ElementHandle ehSel;
	int m_CurSelTab;
	CWallMainRebarDlgNew					m_PageMainRebar;			//主要配筋
	CWallRebarEndTypeDlgNew					m_PageEndType;				//端部样式
	CTwinBarSetDlgNew						m_PageTwinBars;				//并筋设置
	CTieRebarSetDlgNew						m_PageTieRebar;				//拉筋对话框
	//	CWallRebarAssociatedComponentDlg	m_PageAssociatedComponent;
	CDialog*							pDialog[4];					//用来保存对话框对象指针
	virtual BOOL OnInitDialog();
	afx_msg void OnTcnSelchangeTabWallrebar(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedOk();
	string wall_name;
public:
	CComboBox m_ComboSize;//尺寸
	CComboBox m_ComboType;//型号
	CEdit	  m_EditSpace;//间距

	WallSetInfo m_WallSetInfo;
// 	char    m_rebarSize[512];		//钢筋尺寸
// 	int		m_rebarType;			//钢筋型号
// 	double  m_spacing;			//钢筋间距

	std::vector<PIT::ConcreteRebar>					m_vecRebarData;
	std::vector<PIT::LapOptions>					m_vecLapOptionData;
	std::vector<PIT::EndType>						m_vecEndTypeData;
	std::vector<PIT::AssociatedComponent>			m_vecACData;
	std::vector<TwinBarSet::TwinBarLevelInfo>	m_vecTwinBarData;
	TwinBarSet::TwinBarInfo						m_twinBarInfo;
	TieReBarInfo								m_tieRebarInfo;
	afx_msg void OnBnClickedCancel();
	CStatic m_static_wallname;
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnStnClickedStaticWallname();
};
