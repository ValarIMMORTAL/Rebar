#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"
#include "CInsertRebarAssemblyWall.h"

// CWallInsertRebarDlg 对话框

class CWallInsertRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CWallInsertRebarDlg)

public:
	CWallInsertRebarDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWallInsertRebarDlg();

	virtual BOOL OnInitDialog();

	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }

	void SetBashElement(ElementHandleCR eh) { m_basis = eh; }

	void CoverOnBnClickedOk();

	void SetListRowData(const std::vector<PIT::ConcreteRebar>& vecListData)
	{
		m_vecRebarData = vecListData;
	}

	void SetListRowDataEndType(std::vector<PIT::EndType>& vecEndType)
	{
		m_vecEndType = vecEndType;
	}

	void SetConcreteId(ElementId id) { m_ConcreteId = id; }

	void UpdateRebarList();

	void SetListDefaultData();

	void UpdateEndTypeList();

	void SetWallAssemblyPtr(CInsertRebarAssemblySTWall*	ptr) { m_pInsertWallAssembly = ptr; }

	void SetListDefaultDataEndType();

	void SetstaggeredStyle(int staggeredStyle) { m_staggeredStyle = staggeredStyle; }

	int GetRebarPtsSize()
	{
		return (int)m_RebarPts.size();
	}

	int GetStaggeredStyle() { return m_staggeredStyle; }

private:
	void InitUIData();

private:
	CButton								    m_staggered_check1;
	CButton								    m_staggered_check2;
	CButton									m_staggered_check3;
	CButton									m_staggered_check4;

	std::vector<PIT::EndType>					m_vecEndType;
	std::vector<PIT::ConcreteRebar>				m_vecRebarData;
	std::vector<InsertRebarInfo::WallInfo>  m_vecWallInsertInfo;
	std::vector< InsertRebarInfo::WallInfo> m_vecWallInsertInfoBak;
	std::vector<PIT::EndType>					m_vecEndTypeBak;

	CInsertRebarListCtrl					m_listMainRebar;		// 配筋数据
	CRebarEndTypeListCtrl					m_ListEndType;			// 端部样式
	std::vector<RebarPoint>					m_RebarPts;
	Transform								m_trans;
	ElementHandle							m_ehSel;
	ElementHandle							m_basis;
	int										m_staggeredStyle;		// 交错类型 1 ：钢筋层交错 2：间隔交错
	int										m_wallTopType;          // 上下墙类型: 1 -- 上墙变宽 2 -- 上墙变窄
	CInsertRebarAssemblySTWall*				m_pInsertWallAssembly;
	ACCConcrete								m_acconcrete;

	ElementId								m_ConcreteId;

	vector<int>								m_vecFilterLevel;		// 过滤后的纵向钢筋层

	PIT::WallRebarInfo							m_slabRebarInfo; // 板相关信息

	PIT::WallRebarInfo							m_wallRebarInfo;

	double									m_diameter;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_InsertRebarWall };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLvnItemchangedList2(NMHDR *pNMHDR, LRESULT *pResult);
public:
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedCheck2();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCheck3();
	afx_msg void OnBnClickedCheck4();
};
