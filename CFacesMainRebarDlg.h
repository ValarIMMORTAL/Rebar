﻿#pragma once
#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"
#include "C_RebarTemplate.h"

// CFacesMainRebarDlg 对话框


class CFacesMainRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFacesMainRebarDlg)

public:
	CFacesMainRebarDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CFacesMainRebarDlg();

	void SetHide(bool isHide)
	{
		m_isHide = isHide;
	}

	void SetDlgType(int FaceDlgType)
	{
		m_FaceDlgType = FaceDlgType;
	}

	void ChangeRebarSizedata(char * rebarSize)
	{
		auto it = m_vecRebarData.begin();
		for (it; it != m_vecRebarData.end(); it++)
			strcpy(it->rebarSize, rebarSize);
}
	void ChangeRebarTypedata(int rebarType)
	{
		auto it = m_vecRebarData.begin();
		for (it; it != m_vecRebarData.end(); it++)
			it->rebarType = rebarType;
	}
	void ChangeRebarSpacedata(double rebarSpace)
	{
		auto it = m_vecRebarData.begin();
		for (it; it != m_vecRebarData.end(); it++)
			it->spacing = rebarSpace;
	}

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_FacesRebar_MainRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()


private:
	void InitUIData();
	CDialogEx *			m_FaceDlgPtr;
	PIT::Concrete	m_Concrete;
	std::vector<PIT::ConcreteRebar> m_vecRebarData;
public:
	void SetListDefaultData();
	void UpdateRebarList();

	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }

	void SetSelectfaces(bvector<ISubEntityPtr>& selectfaces);

	void DeleteFaceLine();

	void SavePrt(CDialogEx * Ptr);


	bool GetIsReverse()
	{
		return m_check_reverse.GetCheck() == 1;
	}

	bool GetIsMergeRebar()
	{
		return m_isMergeRebar;
	}
	bool GetIsMergeFace()
	{
		return m_isMergeFace;
	}
	bool GetIsAnchorFace()
	{
		return m_isAnchorFace;
	}

	bool GetIsSumps()
	{
		return m_isSumps;;
	}

	CVector3D GetvecFaceNormal(size_t iIndex);

private:
	ElementHandle m_ehSel;
	bool m_isHide;

	int  m_FaceDlgType; // 0: 正常面配筋 1:多板联合配筋
	bool m_isMergeRebar;
	bool m_isMergeFace; //同板面合并
	bool m_isAnchorFace; //同板多面互锚
	bool m_isSumps; //集水坑

public:
	PIT::Concrete GetConcreteData();
	void SetConcreteData(PIT::Concrete concrete) { m_Concrete = concrete; }
	void SetListRowData(const std::vector<PIT::ConcreteRebar>& vecListData) {
		m_vecRebarData = vecListData;
	}
	void GetListRowData(std::vector<PIT::ConcreteRebar> &vecListData) {
		vecListData = m_vecRebarData;
	};
	virtual BOOL OnInitDialog();
	CMainRebarListCtrl m_listMainRebar;
	CEdit m_EditPositive;
	CEdit m_EditSide;
	CEdit m_EditLevel;

	PIT::DlgData  m_dlgData;			//界面上的保护层等数据
	C_RebarTemplate	 m_RebarTemplate;	//钢筋模板界面
	CString m_templateName;				//上一次使用的模板名称
	void OnBnClickedUpdatadlg();		//刷新界面，参数模板按确认按钮之后调用
	void Save_templateName();			//保存上一次的模板名称

	afx_msg void OnEnKillfocusEdit1();
	afx_msg void OnEnKillfocusEdit2();
	afx_msg void OnEnKillfocusEdit4();

	afx_msg LRESULT OnComboBoxDataChange(WPARAM wParam, LPARAM lParam);
	// 是否规避孔洞设置
	CButton m_hole_check;

	// 是否反向配筋
	CButton m_check_reverse;

	CButton	m_check_merge;

	bvector<ISubEntityPtr> m_selectfaces;

	vector<CVector3D>			m_vecFaceNormal;
	vector<ElementId>			m_vecFaceLine;

	// 忽略孔洞大小设置
	CEdit m_mholesize_edit;
	afx_msg void OnBnClickedHoleCheck();
	afx_msg void OnEnKillfocusMholesizeEdit();
	afx_msg void OnBnClickedFaceunionCheck();
	CButton m_SlabUnionCheck;
	afx_msg void OnBnClickedSlabunionCheck();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedCheck2();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedRebartemplate();
	afx_msg void OnBnClickedCheck9();
	CButton m_checkMergeFace;
	CButton m_checkAnchorFace;
	afx_msg void OnBnClickedCheck3();
	afx_msg void OnBnClickedCheck10();
	CButton m_checkIsSumps;
};
