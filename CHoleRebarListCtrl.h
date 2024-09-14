#pragma once
#include "ListCtrlEx.h"
#include "CommonFile.h"

class CHoleRebar_ReinforcingDlg;
class CHoleRebar_StructualDlg;
class CDoorHoleDlg;
class CHoleRebarReinForcingCtrl :public ListCtrlEx::CListCtrlEx
{
public:
	CHoleRebar_ReinforcingDlg* m_Dlg;
	EditElementHandle  m_affectedElement;//树形对话框中高亮选择的元素
	vector<ElementId> m_vecunionID;
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	void GetAllRebarData(std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData);
	void DeleteElements();
	DECLARE_MESSAGE_MAP()
	// message handlers
	afx_msg void		OnLButtonDown(UINT nFlags, CPoint point);
};


class CHoleRebarStructualCtrl :public ListCtrlEx::CListCtrlEx
{
public:
	CHoleRebar_StructualDlg* m_Dlg;
	EditElementHandle  m_affectedElement;//树形对话框中高亮选择的元素
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	vector<ElementId> m_vecunionID;
	void DeleteElements();
	void GetAllRebarData(std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData);
	DECLARE_MESSAGE_MAP()
	// message handlers
	afx_msg void		OnLButtonDown(UINT nFlags, CPoint point);
};

class CHoleRebarAddUnionCtrl :public ListCtrlEx::CListCtrlEx
{
public:
	string m_unionName;
	EditElementHandle  m_affectedElement;//树形对话框中高亮选择的元素
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	void GetAllRebarData(std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData);
	void GetAllRebarDataStructual(std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData);
	DECLARE_MESSAGE_MAP()
	// message handlers
	afx_msg void		OnLButtonDown(UINT nFlags, CPoint point);
};

class CDoorHoleRebarCtrl :public ListCtrlEx::CListCtrlEx
{
public:
	CDoorHoleDlg* m_Dlg;
	EditElementHandle  m_affectedElement;//树形对话框中高亮选择的元素
	vector<ElementId> m_vecunionID;
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	void GetAllRebarData(std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData);
	void DeleteElements();
	DECLARE_MESSAGE_MAP()
	// message handlers
	afx_msg void		OnLButtonDown(UINT nFlags, CPoint point);
};





//点选孔洞
struct SelectHoleTools : DgnElementSetTool
{

private:
	bool            IsRebarDetailSet(ElementHandleCR eh) { return NULL != RebarDetailSet::Fetch(eh); }

protected:

	SelectHoleTools(int toolId) :DgnElementSetTool(toolId)
	{
		m_SHoledlg = nullptr;
		m_RHoledlg = nullptr;
	}

	bool            _OnResetButton(DgnButtonEventCR ev) override { _OnRestartTool(); return true; }
	bool            _DoGroups() override { return false; }
	bool            _NeedAcceptPoint() override { return true; }
	bool            _NeedPointForSelection() override { return false; }
	StatusInt       _OnElementModify(EditElementHandleR  el) override { return ERROR; }

	virtual size_t  _GetAdditionalLocateNumRequired() override { return 1; }
	virtual bool    _OnModifierKeyTransition(bool wentDown, int key) override { return OnModifierKeyTransitionHelper(wentDown, key); }
	virtual bool    _WantAdditionalLocate(DgnButtonEventCP ev) override;

	virtual bool            _IsModifyOriginal() override { return false; }

	void _OnRestartTool() override { _ExitTool(); }

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
	{
		return __super::_BuildLocateAgenda(path, ev);
	}
	virtual UsesDragSelect  _AllowDragSelect() override { return USES_DRAGSELECT_Box; }

public:
	CHoleRebar_StructualDlg* m_SHoledlg;
	CHoleRebar_ReinforcingDlg* m_RHoledlg;
	static void             InstallNewInstance(int toolId, CDialogEx* editdlg);
};