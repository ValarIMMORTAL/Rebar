#pragma once
#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"
#include "CommonFile.h"

// CFacesRebarEndTypeDlg 对话框

class CFacesRebarEndTypeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFacesRebarEndTypeDlg)

public:
	CFacesRebarEndTypeDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CFacesRebarEndTypeDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_FacesRebar_EndType };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	void InitUIData();

	std::vector<PIT::EndType> m_vecEndType;

	ElementHandle m_ehSel;
	
	int m_rebarLevelNum;
	int m_nCurrentRow;  // 当前编辑的行索引
public:
	int GetCurrentRow() const
	{
		return m_nCurrentRow;
	}

	void SetListDefaultData();
	void UpdateEndTypeList();

	void SetListRowData(const std::vector<PIT::EndType>& vecListData) {
		m_vecEndType = vecListData;
	}
	void GetListRowData(std::vector<PIT::EndType> &vecListData) {
		vecListData = m_vecEndType;
	};

	std::vector<PIT::ConcreteRebar> m_vecRebarData;
	// 
	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }
	// 
	void SetRearLevelNum(int rebarLevelNum) { m_rebarLevelNum = rebarLevelNum; }

public:
	CRebarEndTypeListCtrl m_ListEndType;
	virtual BOOL OnInitDialog();
	void SetOffsetValue(int row, double length);
	afx_msg LRESULT OnEndTypeButtonDown(WPARAM wParam, LPARAM lParam);
};
