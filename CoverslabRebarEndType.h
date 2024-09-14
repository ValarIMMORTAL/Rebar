#pragma once
#include "CEditListCtrl.h"
#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"

// CoverslabRebarEndType 对话框

class CoverslabRebarEndType : public CDialogEx
{
	DECLARE_DYNAMIC(CoverslabRebarEndType)

public:
	CoverslabRebarEndType(ElementHandleCR eh, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CoverslabRebarEndType();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CoverslabRebar_EndType };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
private:
	void InitUIData();

	std::vector<PIT::EndType> m_vecEndType;

	ElementHandle m_ehSel;
	int	m_CoverslabType;
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
};
