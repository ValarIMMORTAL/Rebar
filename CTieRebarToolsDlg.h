#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include "CTieRebarSetDlg.h"
#include "CTieRebarToolAssembly.h"


// CTieRebarToolsDlg 对话框

class CTieRebarToolsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CTieRebarToolsDlg)

public:
	CTieRebarToolsDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CTieRebarToolsDlg();

	virtual BOOL OnInitDialog();

	void SorSelcetRebar();

	void InitRebarSetsAndehSel();

	std::vector<ElementRefP> m_selectrebars;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_TieRebarTools };
#endif

private:

	int								m_CurSelTab;
	CTabCtrl						m_tab;
	CDialog*						p_Dialog[1];			 // 用来保存对话框对象指

	CTieRebarSetDlg					m_TableTieDlg;

	ElementHandle					m_ehSel;
	TieReBarInfo					m_tieRebarInfo;
	RebarAssembly*					m_pRebarAssembly;
	ElementId						m_contid;

	std::vector<PIT::ConcreteRebar>		m_vecRebarData;

	CTieRebarToolAssembly*			m_pTieRebarAssembly;

	map<int, vector<ElementRefP>>	m_mapselectrebars; // 选择的钢筋实体集合

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedOk();
};
