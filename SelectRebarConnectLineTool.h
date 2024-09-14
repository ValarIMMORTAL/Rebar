#pragma once
#include <RebarElements.h>
#include "CommonFile.h"
#include "PITRebarCurve.h"

class	SelectRebarConnectLineTool : public DgnElementSetTool
{
public:
	~SelectRebarConnectLineTool()
	{
		auto& ssm = SelectionSetManager::GetManager();
		ssm.EmptyAll();
		mdlSelect_freeAll();
	}

	//static void InstallNewInstance(int toolId, CWallMainRebarDlg* Ptr);
	static void InstallNewInstance(int toolId);
	SelectRebarConnectLineTool(int toolId) :DgnElementSetTool(toolId) { }
protected:

	
	virtual bool            _IsModifyOriginal() override { return false; }

	StatusInt _OnElementModify(EditElementHandleR el) override;
	void _OnRestartTool() override;

	virtual void _SetupAndPromptForNextAction() override;

	virtual bool _WantAdditionalLocate(DgnButtonEventCP ev) override;
	virtual bool _OnModifyComplete(DgnButtonEventCR ev) override;

	virtual UsesSelection _AllowSelection() override
	{
		return USES_SS_Check;
	}

	virtual bool _DoGroups() override;

	virtual bool _WantDynamics() override;
	virtual bool _OnDataButton(DgnButtonEventCR ev)override;

	virtual bool _OnModifierKeyTransition(bool wentDown, int key) override;

	virtual bool _OnPostLocate(HitPathCP path, WStringR cantAcceptReason) override;

	bool _OnResetButton(DgnButtonEventCR ev) override;

	virtual EditElementHandleP _BuildLocateAgenda(HitPathCP path, DgnButtonEventCP ev) override;

	virtual UsesDragSelect  _AllowDragSelect() override { return USES_DRAGSELECT_Box; }
	virtual bool            _NeedAcceptPoint() override { return SOURCE_Pick == _GetElemSource(); }
	virtual size_t          _GetAdditionalLocateNumRequired() override { return 0; }

public:
	//static CWallMainRebarDlg * m_Ptr;
	static double m_dLineHigh;
};