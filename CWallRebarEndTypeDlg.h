#pragma once
// #include "CEditListCtrl.h"
#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"
#include "CommonFile.h"

// CWallRebarEndType 对话框

class CWallRebarEndTypeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CWallRebarEndTypeDlg)

public:
	CWallRebarEndTypeDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWallRebarEndTypeDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_WallRebar_EndType };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	void InitUIData();

	std::vector<PIT::EndType> m_vecEndType;

	ElementHandle m_ehSel;

//	BrString m_strRebarSize;
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
	void GetAllEndTypeData(std::vector<PIT::EndType> &vecListData);
	
	std::vector<PIT::ConcreteRebar> m_vecRebarData;
// 
	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }
// 
// 	void SetCurRebarSize(BrString rebarSize) { m_strRebarSize = rebarSize; }

public:
	CRebarEndTypeListCtrl m_ListEndType;
	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnEndTypeButtonDown(WPARAM wParam, LPARAM lParam);
};
