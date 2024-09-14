#pragma once
#include "CEditListCtrl.h"
#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"

// CSlabRebarEndType 对话框

class CSlabRebarEndType : public CDialogEx
{
	DECLARE_DYNAMIC(CSlabRebarEndType)

public:
	CSlabRebarEndType(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSlabRebarEndType();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_SlabRebar_EndType };
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	void InitUIData();

	std::vector<PIT::EndType> m_vecEndType;

public:
	void SetListDefaultData();
	void UpdateEndTypeList();

public:
	void SetListRowData(const std::vector<PIT::EndType>& vecListData) {
		m_vecEndType = vecListData;
	}
	void GetListRowData(std::vector<PIT::EndType> &vecListData) {
		vecListData = m_vecEndType;
	};
public:
	CRebarEndTypeListCtrl m_ListEndType;
	virtual BOOL OnInitDialog();
	afx_msg void OnLvnItemchangedListEndtype(NMHDR *pNMHDR, LRESULT *pResult);
};
