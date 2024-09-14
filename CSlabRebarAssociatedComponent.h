#pragma once
#include "CommonFile.h"
#include "ListCtrlEx.h"


// CSlabRebarAssociatedComponent 对话框

class CSlabRebarAssociatedComponent : public CDialogEx
{
	DECLARE_DYNAMIC(CSlabRebarAssociatedComponent)

public:
	CSlabRebarAssociatedComponent(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSlabRebarAssociatedComponent();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_SlabRebar_AssociatedComponent };
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	void InitUIData();

	std::vector<PIT::AssociatedComponent> m_vecAC;
	ElementHandle m_ehSel;

public:
	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }
	ElementHandle GetSelectElement() { return m_ehSel; }

	void SetListDefaultData();
	void UpdateACList();

public:
	void SetListRowData(const std::vector<PIT::AssociatedComponent>& vecListData) {
		m_vecAC = vecListData;
	}
	void GetListRowData(std::vector<PIT::AssociatedComponent> &vecListData) {
		vecListData = m_vecAC;
	};

public:
	virtual BOOL OnInitDialog();
	ListCtrlEx::CListCtrlEx m_ListAC;
	//afx_msg void OnLvnItemchangedListAc(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedSListAc(NMHDR *pNMHDR, LRESULT *pResult);
};
