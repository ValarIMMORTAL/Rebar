#pragma once
#include "CommonFile.h"
#include "CColInsertRebarDlg.h"
#include "CWallInsertRebarDlg.h"
#include "GalleryIntelligentRebarids.h"


// CInsertRebarMain 对话框
class CInsertRebarMainDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CInsertRebarMainDlg)

public:
	virtual BOOL OnInitDialog();
protected:
	virtual void PostNcDestroy();
public:
	CInsertRebarMainDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CInsertRebarMainDlg();

	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }

	void SetBashElement(ElementHandleCR eh) { m_basis = eh; }

	void SetFirstItem(int FirstItem) { m_FirstItem = FirstItem; }

	void SetConcreteId(ElementId id) { m_ConcreteId = id; }

	CColInsertRebarDlg		m_PageColInserRebar;	 // 插筋--柱
	CWallInsertRebarDlg		m_PageWallInsertRebar;   // 插筋--墙

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_InsertRebarMain };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	CTabCtrl				m_tab;

	int						m_CurSelTab;
	int				        m_FirstItem;			 // 第一页标题 0: 柱插筋 在前 1: 墙插筋在前
	CDialog*				p_Dialog[2];			 // 用来保存对话框对象指

	ElementId				m_ConcreteId;

	bool					bIsFirst;
	CString					m_arrItem[2];
public:
	afx_msg void OnTcnSelchangeTab2(NMHDR *pNMHDR, LRESULT *pResult);

	ElementHandle			m_ehSel;
	ElementHandle			m_basis;

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};


/*=================================================================================**//**
* @bsiclass                                                               Bentley Systems
+===============+===============+===============+===============+===============+======*/
struct DgnInsertRebarTool : public DgnElementSetTool
{
private:

public:

	DgnInsertRebarTool(int toolID) : DgnElementSetTool(toolID){}

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
	void            CreateRebarElement(EditElementHandleR tagEeh, bool isDynamics);
	StatusInt       _ProcessAgenda(DgnButtonEventCR  ev) override;
	bool            _OnInstall() override;
	void            _OnRestartTool() override;

public:

	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	static void InstallNewInstance(int toolId);

};

