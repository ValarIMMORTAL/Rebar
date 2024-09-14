
#include "CoverslabTieRebarSetDlg.h"
#include "CoverslabMainRebarDlg.h"
#include "CoverslabRebarEndType.h"
#include "CoverslabRebarTwinDlg.h"
#include "CoverslabRebarAssembly.h"
// CoverslabRebarDlg 对话框

class CoverslabRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CoverslabRebarDlg)

public:
	CoverslabRebarDlg(ElementHandleCR eh, CWnd* pParent =nullptr);   // 标准构造函数
	virtual ~CoverslabRebarDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CoverslabRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void PostNcDestroy();
	DECLARE_MESSAGE_MAP()
public:

	CTabCtrl m_tab;
public:
	void SetSelectElement(ElementHandleCR eh) { ehSel = eh; }
	ElementHandle GetSelectElement() { return ehSel; }
	void SetConcreteId(ElementId id) { m_ConcreteId = id; }

	bool m_isDoubleClick;

private:
	ElementHandle ehSel;
	ElementId m_ConcreteId;

	std::vector<PIT::ConcreteRebar>					m_vecRebarData;
//	std::vector<PIT::LapOptions>						m_vecLapOptionData;
	std::vector<PIT::EndType>						m_vecEndTypeData;
//	std::vector<PIT::AssociatedComponent>			m_vecACData;
	std::vector<TwinBarSet::TwinBarLevelInfo>	m_vecTwinBarData;
	TwinBarSet::TwinBarInfo						m_twinBarInfo;

	WallRebarInfo								m_wallRebarInfo;

	TieReBarInfo								m_tieRebarInfo;

public:
	int m_CurSelTab;
	CoverslabMainRebarDlg						m_PageMainRebar;			//主要配筋
	CoverslabRebarEndType						m_PageEndType;				//端部样式
	CoverslabRebarTwinDlg						m_PageTwinBars;				//并筋设置
		//CWallRebarAssociatedComponentDlg	m_PageAssociatedComponent;
	CDialog*							pDialog[3];					//用来保存对话框对象指针
	virtual BOOL OnInitDialog();
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnTcnSelchangeTabCoverslabrebar(NMHDR *pNMHDR, LRESULT *pResult);
};
