/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/SlabRebarAssembly.h $
|
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#pragma once
#include <RebarElements.h>
#include "RebarDetailElement.h"
#include <map>
class SingleRebarAssembly : public RebarAssembly
{
public:
	    BE_DATA_VALUE(vector<RebarVertices>, rebarPts)			             //钢筋所有点坐标
		BE_DATA_VALUE(ElementId, SetId)								//SetId
		BE_DATA_VALUE(double, spacing)								//
		BE_DATA_VALUE(vector<BrString>, vecDirSize)	
		BE_DATA_VALUE(int, typenum)//尺寸
		bool mCAddVerticalRebarflg;//点筋标志，用于设置点筋图层为主筋图层，不随钢筋设置图层
		string mSelectedRebarType;
		string mSelectedRebarLevel;
		string mSelectedRebarGrade;
		string mMarkname;
		string mreMark;


protected:
    virtual int         GetPolymorphic () const override                            { return REX_Type::kLastRebarElement + kRebarAssembly + 10; }
    virtual WString     GetDescriptionEx (ElementHandleCR eh) const override        { return L"Single Rebar"; }
    virtual WString     GetPathDescriptionEx (ElementHandleCR eh) const override    { return L"Single Rebar By SDK"; }
    virtual bool        OnDoubleClick () override;
    virtual bool        Rebuild () override;

protected:
	RebarSetTag* MakeRebars
	(
		ElementId&          rebarSetId,
		double              spacing,
		vector<RebarVertices>&   pts,
		CVector3D const&    endNormal,
		DgnModelRefP        modelRef
	);
public:
public:
	SingleRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~SingleRebarAssembly() {};
    virtual BentleyStatus GetPresentation (EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
    BE_DECLARE_VMS (SingleRebarAssembly, RebarAssembly)

public:
    bool SetSlabData (ElementHandleCR slab);
	void SetSelectedRebar(ElementHandleCR slectReabr,bool CAddVerticalRebarflg = false);
    void SetRebarData ();
    void GetRebarData () const;
    bool MakeRebars (DgnModelRefP modelRef);
	ElementHandle mrebareeh;
    static bool IsSlabSolid (ElementHandleCR eh);
	void SetEcDatas(string RebarType, string RebarLevel, string RebarGrade, string markname = "", string remark = "")
	{
		mSelectedRebarType = RebarType;
		mSelectedRebarLevel = RebarLevel;
		mSelectedRebarGrade = RebarGrade;
		mMarkname = markname;
		mreMark = remark;
	}
 };
