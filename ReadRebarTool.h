/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/ReadRebarTool.cpp $
|
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include "GalleryIntelligentRebar.h"
#include "CTwinBarToolDlg.h"
#include "rebarSDK_2d.h"
#include "RebarCatalog.h"
#include "RebarElements.h"
#include <RebarDetailElement.h>
#include "Mstn/ISessionMgr.h"
#include "PITMSCECommon.h"
#include "CURebarToolDlg.h"
#include "CStirrupToolDlg.h"
#include "CStretchStirrupRebarToolDlg.h"

struct PITSelectRebarTool : DgnElementSetTool
{

private:
	bool					IsRebarDetailSet(ElementHandleCR eh) { return NULL != RebarDetailSet::Fetch(eh); }

protected:
	virtual bool            _DoGroups() override { return false; }
	virtual bool            _WantAccuSnap() override { return false; }
	virtual bool            _WantDynamics() override { return false; }
	virtual UsesSelection   _AllowSelection() override { return USES_SS_Check; }
	virtual UsesDragSelect  _AllowDragSelect() override { return USES_DRAGSELECT_Box; }
	virtual bool            _NeedAcceptPoint() override { return SOURCE_Pick == _GetElemSource(); }
	virtual bool            _NeedPointForSelection() override { return true; }
	virtual size_t          _GetAdditionalLocateNumRequired() override { return 1; }
	virtual bool            _WantAdditionalLocate(DgnButtonEventCP ev) override { return WantAdditionalLocateHelper(ev); }
	virtual bool            _OnModifierKeyTransition(bool wentDown, int key) override { return OnModifierKeyTransitionHelper(wentDown, key); }

	virtual StatusInt       _OnElementModify(EditElementHandleR eeh) override { return ERROR; }
	virtual bool            _OnModifyComplete(DgnButtonEventCR ev) override { return true; };
	virtual bool            _FilterAgendaEntries() override;
	virtual bool            _OnPostLocate(HitPathCP path, WStringR cantAcceptReason) override;
	virtual void            _SetupAndPromptForNextAction() override {}
	virtual void			_OnRestartTool()	override { _ExitTool(); }
	virtual bool			_OnResetButton(DgnButtonEventCR ev) override { _ExitTool(); return true; }

public:
	PITSelectRebarTool(int toolId) :DgnElementSetTool(toolId) {};
};

struct PITSelectTwinRebarTool : PITSelectRebarTool
    {
    protected:
        virtual bool            _OnModifyComplete (DgnButtonEventCR ev) override;

    public:	
		PITSelectTwinRebarTool(int toolId) :PITSelectRebarTool(toolId) {};
		CTwinBarToolDlg* m_TwinRebardlg;
		static void				InstallTwinRebarInstance(int toolId, CTwinBarToolDlg* editdlg);
};


struct PITSelectURebarTool : PITSelectRebarTool
{
protected:
	virtual bool            _OnModifyComplete(DgnButtonEventCR ev) override;
	virtual void            _SetupAndPromptForNextAction() override {}
	//virtual size_t          _GetAdditionalLocateNumRequired() override { return 1; }
	virtual EditElementHandleP _BuildLocateAgenda(HitPathCP  path, DgnButtonEventCP  ev) override;

public:
	PITSelectURebarTool(int toolId) :PITSelectRebarTool(toolId) {};
	CURebarToolDlg* m_URebardlg;
	static void				InstallURebarInstance(int toolId, CURebarToolDlg* editdlg);

private:
	DPoint3d m_endRebarPt;
};

struct PITSelectStirrupRebarTool : PITSelectRebarTool
{
protected:
	virtual bool            _OnModifyComplete(DgnButtonEventCR ev) override;
	virtual void            _SetupAndPromptForNextAction() override {}
	virtual size_t          _GetAdditionalLocateNumRequired() override { return 4; }

public:
	PITSelectStirrupRebarTool(int toolId) :PITSelectRebarTool(toolId) {};
	CStirrupToolDlg* m_StirrupRebardlg;
	static void				InstallStirrupRebarInstance(int toolId, CStirrupToolDlg* editdlg);
};


/*
* ClassName:	PITStretchStirrupRebarTool
* Description:	πøΩÓ¿≠…Ïπ§æﬂ
* Author:		hzh
* Date:			2022/05/27
*/
struct PITStretchStirrupRebarTool : PITSelectRebarTool
{
protected:
	virtual void            _SetupAndPromptForNextAction() override {}
	//virtual size_t          _GetAdditionalLocateNumRequired() override { return 4; }
	virtual bool			_OnDataButton(DgnButtonEventCR ev)override;
	virtual bool			_OnPostLocate(HitPathCP path, WStringR cantAcceptReason) override;

public:
	PITStretchStirrupRebarTool(int toolId) :PITSelectRebarTool(toolId) {};
	CStretchStirrupRebarToolDlg* m_StretchStirrupRebardlg;
	static void				InstallStretchStirrupRebarInstance(int toolId, CStretchStirrupRebarToolDlg* editdlg);
};
