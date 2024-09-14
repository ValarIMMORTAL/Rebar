#pragma once
#include "CommonFile.h"
#include "CInsertRebarAssemblyWallNew.h"


// CInsertRebarDlgNew 对话框

class CInsertRebarDlgNew : public CDialogEx
{
	DECLARE_DYNAMIC(CInsertRebarDlgNew)

public:
	CInsertRebarDlgNew(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CInsertRebarDlgNew();

	BOOL OnInitDialog();

	void SetSlabParam(EditElementHandle& basis);

	void CalaRebarPoint(std::vector<InsertRebarPoint>& RebarPts);

	void FilterReberPoint();

	void DrawRebarLine(DgnModelRefP modelRef);

	void SetSelectElement(EditElementHandle& eh) { m_ehSel = eh; }

	void ClearLine();

	CInsertRebarAssemblySTWallNew*	m_pInsertWallAssemblyNew;
	ElementAgenda					m_selectRebars;
	ElementAgenda					m_filterRebars;
protected:
	virtual void PostNcDestroy(); 
private:
	CEdit							m_EditExtended;		// 拓展
	CEdit							m_EditEmbed;		// 埋置
	CComboBox						m_CombEndType;
	CComboBox						m_CombEndNormal;
	CComboBox						m_CombConnectStyle;	//连接方式

	CButton							m_staggered_check;   // 是否交错
	CButton							m_InsertType1;		 // 上墙变窄
	CButton							m_InsertType2;		 // 上墙变宽
	CButton							m_IsReverse_check;   // 是否反向配筋

	InsertRebarInfo::WallInfo		m_stWallInfo;
	std::vector<InsertRebarPoint>	m_RebarPts;

	ElementId						m_conid;

	ElementHandle					m_ehSel;

	PIT::WallRebarInfo				m_wallRebarInfo;

	TieReBarInfo				    m_tieRebarInfo;

	vector<ElementId>				m_vecLineId;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_Insert };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnBnClickedCheck3();
	afx_msg void OnEnKillfocusEdit3();
	afx_msg void OnEnKillfocusEdit18();
private:
	// 弯钩方向
	CEdit m_EditDirection;
public:
	afx_msg void OnEnKillfocusEditJc1();
	afx_msg void OnEnChangeEditJc1();
	afx_msg void OnCbnSelchangeComboConnectstyle();

private:
	/*
	* @desc:	更新修改选中的点筋
	* @param[in]	rebarPts 插筋的端点信息	
	* @param[in]	modelRef 模型
	* @author	hzh
	* @Date:	2022/09/08
	*/
	void UpdateSelectRebars(const vector<InsertRebarPoint>& rebarPts, DgnModelRefP modelRef);
public:
	afx_msg void OnEnKillfocusInsertTzcdedit();
};



/*=================================================================================**//**
* @bsiclass                                                               Bentley Systems
+===============+===============+===============+===============+===============+======*/
struct DgnInsertRebarToolNew : public DgnElementSetTool
{
private:
	CInsertRebarDlgNew* m_pInsertRebarDlgNew;
public:

	DgnInsertRebarToolNew(int toolID, CInsertRebarDlgNew* pInsertRebarDlgNew) : DgnElementSetTool(toolID) 
	{
		m_pInsertRebarDlgNew = pInsertRebarDlgNew;
	}

	bool            _OnResetButton(DgnButtonEventCR ev) override { _OnRestartTool(); return true; }
	bool            _DoGroups() override { return false; }
	bool            _NeedAcceptPoint() override { return true; }
	bool            _NeedPointForSelection() override { return false; }
	StatusInt       _OnElementModify(EditElementHandleR  el) override { return ERROR; }

	bool            IsRebarDetailSet(ElementHandleCR eh) { return NULL != RebarDetailSet::Fetch(eh); }

	virtual size_t  _GetAdditionalLocateNumRequired() override { return 1; }
	virtual bool    _OnModifierKeyTransition(bool wentDown, int key) override { return OnModifierKeyTransitionHelper(wentDown, key); }
	virtual bool    _WantAdditionalLocate(DgnButtonEventCP ev) override;

	virtual bool    _IsModifyOriginal() { return false; }

	void            SetupForLocate();
	void            _SetupAndPromptForNextAction() override;
	bool            _SetupForModify(DgnButtonEventCR ev, bool isDynamics) override;
	EditElementHandleP  _BuildLocateAgenda(HitPathCP path, DgnButtonEventCP ev) override;
	bool            _OnDataButton(DgnButtonEventCR ev) override;
	bool            _OnPostLocate(HitPathCP path, WStringR cantAcceptReason) override;


	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	StatusInt       _ProcessAgenda(DgnButtonEventCR  ev) override;
	bool            _OnInstall() override;
	void            _OnRestartTool() override;

public:

	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	static void InstallNewInstance(int toolId, CInsertRebarDlgNew* pInsertRebarDlgNew);

};