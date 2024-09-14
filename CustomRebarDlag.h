#pragma once
#include "Public.h"
#include "CustomizeTool.h"

// CustomRebarDlag 对话框

class CustomRebarDlag : public CDialogEx
{
	DECLARE_DYNAMIC(CustomRebarDlag)

public:
	CustomRebarDlag(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CustomRebarDlag();
	virtual BOOL OnInitDialog();

	void ParsingElementPro(ElementHandleCR eeh);
	vector<pointInfo> m_vctLineinfo;
	int m_Linetype;//1:全为弧线   0：全为直线  -1：有弧线和直线

	CEdit	  m_EditArrayNum;
	CComboBox m_ComboArrayDir;
	CComboBox m_ComboSize;
	CComboBox m_ComboType;//型号
	CEdit m_EditSpcing;	//间距

	CustomizeRebarInfo m_CustomizeRebarInfo;
	ElementHandle m_ehSel;//线

	ElementHandle m_WallehSel;//墙
	CustomRebarAssembly * m_PcustomAssembly;
	shared_ptr<CustomRebar> m_Pcustom;
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CustomizeRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnBnClickedOk();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnEnChangeEditSpacing();
	CEdit m_edit_level;
	CEdit m_edit_type;
};



struct SelectLineTools: DgnElementSetTool
{

private:
	bool            IsRebarDetailSet(ElementHandleCR eh) { return NULL != RebarDetailSet::Fetch(eh); }

protected:

	SelectLineTools(int toolId) :DgnElementSetTool(toolId) {}

	bool            _OnResetButton(DgnButtonEventCR ev) override { _OnRestartTool(); return true; }
	bool            _DoGroups() override { return false; }
	bool            _NeedAcceptPoint() override { return true; }
	bool            _NeedPointForSelection() override { return false; }
	StatusInt       _OnElementModify(EditElementHandleR  el) override { return ERROR; }

	virtual size_t  _GetAdditionalLocateNumRequired() override { return 1; }
	virtual bool    _OnModifierKeyTransition(bool wentDown, int key) override { return OnModifierKeyTransitionHelper(wentDown, key); }
	virtual bool    _WantAdditionalLocate(DgnButtonEventCP ev) override;

	virtual bool            _IsModifyOriginal() override { return false; }

	void _OnRestartTool() override {_ExitTool();}

	virtual void _SetupAndPromptForNextAction() override {}

	virtual bool _OnModifyComplete(DgnButtonEventCR ev) override;

	virtual UsesSelection _AllowSelection() override
	{
		return USES_SS_Check;
	}

	virtual bool _WantDynamics() override { return false; }
	virtual bool _OnDataButton(DgnButtonEventCR ev)override;
	virtual bool _OnPostLocate(HitPathCP path, WStringR cantAcceptReason) override;

	virtual EditElementHandleP _BuildLocateAgenda(HitPathCP path, DgnButtonEventCP ev) override 
	{ return __super::_BuildLocateAgenda(path, ev); }
	virtual UsesDragSelect  _AllowDragSelect() override { return USES_DRAGSELECT_Box; }

public:

	CustomRebarDlag* m_Linedlg;
	CustomRebarAssembly * P_ptr;
	static void             InstallNewInstance(int toolId, CustomRebarDlag* editdlg);
};