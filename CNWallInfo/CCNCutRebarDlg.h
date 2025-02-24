#pragma once
#include "../../GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrlNew.h"

// CCNCutRebarDlg 对话框

class CCNCutRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CCNCutRebarDlg)

public:
	CCNCutRebarDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CCNCutRebarDlg();

	virtual BOOL OnInitDialog();

	void UpdateCutList();

	void SetListDefaultData();

public: 
	std::vector<ElementRefP> m_selectrebars;

	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }

	void InitRebarSetsAndehSel();

	double CalaRebarLength(RebarElementP pRebar, DgnModelRefP modelRef);

	void SetvecCutInfo(std::vector<CNCutRebarInfo>& vecCutInfo)
	{
		m_vecCutInfo = vecCutInfo;
	}

	CCNCutRebarList m_ListCutRebar;

private:

	int				m_nIndex;
	CEdit			m_EditNumber;
	CButton			m_CheckReserve;

	ElementHandle	m_ehSel;

	std::vector<CNCutRebarInfo> m_vecCutInfo;

	RebarAssembly* m_pRebaras;

	double		   m_rebarLength;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CNCutRebarDlg };
#endif

public:
	afx_msg void OnEnKillfocusEdit1();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCheck1();
};
