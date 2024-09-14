#pragma once
#include "GalleryIntelligentRebarids.h"
#include "CWallMainRebarDlg.h"
// CParapetDlg 对话框

class CParapetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CParapetDlg)

public:
	CParapetDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CParapetDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_Parapet };
#endif
	void SetSelectElement(ElementHandleCR eh) { ehSel = eh; }
	ElementHandle GetSelectElement() { return ehSel; }
	void SetConcreteId(ElementId id) { m_ConcreteId = id; }
	void SetDefaultData();
private:
	ElementHandle ehSel;
	ElementId m_ConcreteId;
protected:
	virtual void PostNcDestroy();
public:
	WallSetInfo m_WallSetInfo;
	CWallMainRebarDlg					m_PageMainRebar;			//主要配筋
	virtual BOOL OnInitDialog();
	std::vector<PIT::ConcreteRebar>					m_vecRebarData;
	std::vector<PIT::LapOptions>					m_vecLapOptionData;
	std::vector<PIT::EndType>						m_vecEndTypeData;
	std::vector<PIT::AssociatedComponent>			m_vecACData;
	std::vector<TwinBarSet::TwinBarLevelInfo>	m_vecTwinBarData;
	TwinBarSet::TwinBarInfo						m_twinBarInfo;
	TieReBarInfo								m_tieRebarInfo;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CStatic m_static_maindlg;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CEdit m_edit_ulenth;
};
