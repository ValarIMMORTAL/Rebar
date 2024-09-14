/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/SelectRebarTool.cpp $
|
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#pragma once
#include "_ustation.h"
#include "GalleryIntelligentRebar.h"
#include "CRebarEditDlg.h"
#include "CCombineRebardlg.h"
#include "rebarSDK_2d.h"
#include "RebarCatalog.h"
#include "RebarElements.h"
#include <RebarDetailElement.h>
#include "Mstn/ISessionMgr.h"
#include "PITMSCECommon.h"
#include "CAddVerticalRebarDlg.h"
#include "ModifyRebarToolDlg.h"
//class CRebarEditDlg;
//class CCombineRebardlg;
//class CAddVerticalRebarDlg;
//struct SelectRebarTool : DgnElementSetTool
//    {
//	
//    private:
//                void            SetupForLocate ();
//                bool            IsRebarDetailSet (ElementHandleCR eh)                       { return NULL != RebarDetailSet::Fetch (eh); }
//
//    protected:
//        virtual bool            _DoGroups () override                                       { return false; }
//        virtual bool            _WantAccuSnap () override                                   { return false; }
//        virtual bool            _WantDynamics () override                                   { return false; }
//        virtual UsesSelection   _AllowSelection () override                                 { return USES_SS_Check; }
//        virtual UsesDragSelect  _AllowDragSelect () override                                { return USES_DRAGSELECT_Box; }
//        virtual bool            _NeedAcceptPoint () override                                { return SOURCE_Pick == _GetElemSource (); }
//        virtual bool            _NeedPointForSelection () override                          { return true; }
//		virtual size_t          _GetAdditionalLocateNumRequired() override;
//		virtual bool            _WantAdditionalLocate(DgnButtonEventCP ev) override			{ return WantAdditionalLocateHelper (ev); }
//        virtual bool            _OnModifierKeyTransition (bool wentDown, int key) override  { return OnModifierKeyTransitionHelper (wentDown, key); }
//
//        virtual StatusInt       _OnElementModify (EditElementHandleR eeh) override;
//        virtual bool            _OnModifyComplete (DgnButtonEventCR ev) override;
//        virtual bool            _FilterAgendaEntries () override;
//        virtual bool            _OnPostLocate (HitPathCP path, WStringR cantAcceptReason) override;
//        virtual void            _SetupAndPromptForNextAction () override;
//        virtual void            _OnRestartTool () override;
//		virtual EditElementHandleP _BuildLocateAgenda(HitPathCP  path,DgnButtonEventCP  ev) override;
//
//		
//		virtual bool _OnResetButton(DgnButtonEventCR ev) override
//		{
//			_ExitTool();
//			return true;
//		}
//		
//    public:	
//         SelectRebarTool (int toolId);
//		 CRebarEditDlg* m_editdlg;
//		 CCombineRebardlg* m_combinerebardlg;
//		 CAddVerticalRebarDlg* m_addverticaldlg;
//		 ModifyRebarToolDlg* m_modifyRebarToolDlg;
//        static void             InstallNewInstance (int toolId, CRebarEditDlg* editdlg);
//		static void             InstallNewInstance2(int toolId, CCombineRebardlg* editdlg);
//		static void             InstallNewInstanceVertical(int toolId, CAddVerticalRebarDlg* editdlg);
//		static void             InstallNewInstanceModifyDlg(int toolId, ModifyRebarToolDlg* editdlg);
//
//	private:
//		DPoint3d m_endRebarPt;   
//};
//
//
//
//class SelectLineTool :public DgnElementSetTool
//{
//public:
//	SelectLineTool();
//	CAddVerticalRebarDlg* m_addverticaldlg;
//	void static InstallNewInstance(CAddVerticalRebarDlg* editdlg);
//protected:
//	virtual bool    _SetupForModify(DgnButtonEventCR ev, bool isDynamics)override;
//
//	virtual bool _NeedAcceptPoint() override { return true; }
//
//	virtual StatusInt   _OnElementModify(EditElementHandleR el)override;
//
//	virtual void _OnRestartTool()override;
//
//	virtual bool    _WantAdditionalLocate(DgnButtonEventCP ev)override;
//
//	virtual UsesDragSelect  _AllowDragSelect()override { return USES_DRAGSELECT_None; }
//
//	virtual bool    _FilterAgendaEntries()override;
//
//	virtual bool    _OnPostLocate(HitPathCP path, WStringR cantAcceptReason)override;
//
//	virtual bool    _OnModifyComplete(DgnButtonEventCR ev)override;
//	/**
//	* @brief 鼠标右键消息函数
//	* @param[in]  ev   点击消息响应对象
//	* @return 工具完成操作后返回为true
//	*/
//	virtual bool _OnResetButton(DgnButtonEventCR ev) override { _ExitTool(); return true; }
//};
//
//
//void RebarSDK_ReadRebar(CRebarEditDlg* editdlg);


typedef struct RebarArcData
{
	double dArcRadius;
	DPoint3d ptArcCenter;
	DPoint3d ptArcBegin;
	DPoint3d ptArcEnd;
	DPoint3d ptArcMid;
}RebarArcData;

bool GetStartEndPointFromRebar(EditElementHandleP start, DPoint3d& PtStar, DPoint3d& PtEnd, double& diameter);


bool GetStartEndAndRadius(EditElementHandleP start, vector<EditElementHandleP> endEehs, double moveLenth,vector<DPoint3d>& NormalPts);



int IntersectionPointToArcDataRebar(RebarArcData &arcInfo, DPoint3d ptA, DPoint3d ptB, DPoint3d ptC, double dRadius);



bool GetRebarVertices(RebarVertices&  vers, RebarCurve& curve, vector<DPoint3d>& NormalPts,
	double bendRadius, double L0Lenth, double rotateangle,bool isEnd);

bool GetArcRebarVertices(RebarVertices&  vers, RebarCurve& curve, vector<DPoint3d>& NormalPts,
	double bendRadius, double L0Lenth, double rotateangle, bool isEnd);

bool CalculateArc(RebarVertices& vers, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt);

bool GetRebarVerticesWhenStraightLenth(RebarVertices&  vers, RebarCurve& curve, vector<DPoint3d>& NormalPts, double L0Lenth, bool isEnd);

void CalculateAddVerticalRebars(EditElementHandleP end1, EditElementHandleP end2, RebarVertex Rebarver,
	double diameter, bvector<DPoint3d>& allpts, vector<RebarVertices>& rebarPts, vector<BrString>& vecDir, double spacinglenth);

	int GetAllCombinePtsUseIntersect(bvector<DPoint3d>& allpts, bvector<DPoint3d>& pts1,
		bvector<DPoint3d>& pts2, DPoint3d intersectpt, EditElementHandleR linestring1, EditElementHandleR linestring2);

	int GetAllCombinePtsUseEndPoint(bvector<DPoint3d>& allpts, bvector<DPoint3d>& pts1,
		bvector<DPoint3d>& pts2, DPoint3d intersectpt, EditElementHandleR linestring1, EditElementHandleR linestring2);

	bool GetRebarVerticesFromPoints(RebarVertices&  vers, bvector<DPoint3d>& allPts, double bendRadius);

	bool GetRebarVerticesFromPointsAndBendRadius(RebarVertices&  vers, bvector<DPoint3d>& allPts, double bendRadius, vector<DPoint3d>& vecpts, vector<double>& vecbendradius);

	
	void MoveRebarPointByNormal(RebarPoint& tmppt, double movdis, DPoint3d vecNormal=DPoint3d::From(0,0,0));