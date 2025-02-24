#pragma once
#include "../../CommonFile.h"
#include "CWallRebarListCtrlNew.h"
#include "ExtractFacesTool.h"
#include "../../AssociatedComponent.h"
#include "CACCDataSetDlgNew.h"


// CWallRebarAssociatedComponentDlg 对话框

class CWallRebarAssociatedComponentDlgNew : public CDialogEx
{
	DECLARE_DYNAMIC(CWallRebarAssociatedComponentDlgNew)

public:
	CWallRebarAssociatedComponentDlgNew(ElementHandleCR eeh,CWnd* pParent = nullptr);
	virtual ~CWallRebarAssociatedComponentDlgNew();

public:
//	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }
	ElementHandle GetSelectElement() { return m_ehSel; }

	void SetListDefaultData();
	void UpdateACList();

public:
	void SetListRowData(const std::vector<PIT::AssociatedComponent>& vecListData) {
		m_vecAC = vecListData;
	}
	void GetListRowData(std::vector<PIT::AssociatedComponent> &vecListData) 
	{
		vecListData = m_vecAC;
	};

	void InitACCData();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CNWallRebar_AssociatedComponent };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	void InitUIData();

	std::vector<PIT::AssociatedComponent> m_vecAC;
	ElementHandle m_ehSel;
	vector<IntersectEle> m_ACCData;	//关联构件的数据
	std::shared_ptr<CACC> m_pACC;
public:
	virtual BOOL OnInitDialog();
	CWallRebarAssociatedCtrlNew m_ListAC;
	afx_msg LRESULT OnACCButtonDown(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnACCCtrlListLButtonDown(WPARAM wParam, LPARAM lParam);
	afx_msg void	OnDestroy();
	afx_msg void	OnClose();

//	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	CStatic m_ctrlAnchoring;
};
