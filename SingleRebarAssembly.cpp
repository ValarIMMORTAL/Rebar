/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/SingleRebarAssembly.cpp $
|
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include <SelectionRebar.h>
#include "SingleRebarAssembly.h"
#include "XmlHelper.h"
#include "CommonFile.h"
#include "ExtractFacesTool.h"

SingleRebarAssembly::SingleRebarAssembly (ElementId id, DgnModelRefP modelRef): 
    RebarAssembly (id, modelRef)
    {
		mCAddVerticalRebarflg = false;
		mMarkname = "";
		mreMark = "";
    }

bool makeRebarCurve
(
RebarCurve&             rebar,
RebarVertices&       pts,
double                  bendRadius,
double                  bendLen,
RebarEndTypes const&    endTypes,
CVector3D const&        endNormal
)
    {
	for (int i=0;i<pts.GetSize();i++)
	{
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(pts.At(i).GetIP());
		vex->SetArcPt(0, pts.At(i).GetArcPt(0));
		vex->SetArcPt(1, pts.At(i).GetArcPt(1));
		vex->SetArcPt(2, pts.At(i).GetArcPt(2));
		vex->SetCenter(pts.At(i).GetCenter());
		vex->SetRadius(pts.At(i).GetRadius());
		vex->SetTanPt(0, pts.At(i).GetTanPt(0));
		vex->SetTanPt(1, pts.At(i).GetTanPt(1));
		vex->SetType(pts.At(i).GetType());
	}
    //rebar.EvaluateEndTypes (endTypes, bendRadius, bendLen + bendRadius, &endNormal);
	//rebar.EvaluateBend(bendRadius);
    return true;
    }


string TCHAR2char(TCHAR* tchStr)
{
	char chRtn[100] = { 0 };
	BeStringUtilities::WCharToCurrentLocaleChar(chRtn, tchStr, 100);
	string str(chRtn);
	return str;
}


RebarSetTag* SingleRebarAssembly::MakeRebars
(
ElementId&          rebarSetId,
double              spacing,
vector<RebarVertices>&   pts,
CVector3D const&    endNormal,
DgnModelRefP        modelRef
)
{
    bool const isStirrup = false;

    RebarSetP   rebarSet = RebarSet::Fetch (rebarSetId, modelRef);
    if (NULL == rebarSet)
        return NULL;

    rebarSet->SetRebarDisplayMode (RebarDisplayMode::kRebarCylinderMode);
    rebarSet->SetCallerId (GetCallerId());
    rebarSet->StartUpdate (modelRef);

    RebarEndType endType;
    endType.SetType (RebarEndType::kNone);

    double uor_per_mm = modelRef->GetModelInfoCP ()->GetUorPerMeter () / 1000.0;
	WString    LevelName;

	if (mrebareeh.IsValid())
	{
		DgnFileP  pDgnFile = ISessionMgr::GetActiveDgnFile();
		ElementPropertiesGetterPtr propEleGet = ElementPropertiesGetter::Create(mrebareeh);
		std::vector<LevelId>vctlevelid;
		for (LevelHandle lh : pDgnFile->GetLevelCacheR())
		{
			LevelId   Levelid = lh.GetLevelId();
			LevelId   Level = propEleGet->GetLevel();
			if (Levelid == (propEleGet->GetLevel()))
			{
				lh.GetDisplayName(LevelName);
			}
		}
	}
	
	int numRebar = (int)pts.size();
	int num = Gettypenum();
	if (mSelectedRebarGrade!="")
	{
		if (mSelectedRebarGrade == "A")
			num = 0;
		else if (mSelectedRebarGrade == "B")
			num = 1;
		else if (mSelectedRebarGrade == "C")
			num = 2;
		else
			num = 3;
	}
    for (int i = 0; i < pts.size(); i++)
    {
		BrString   sizeKey = m_vecDirSize.at(i);
		double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
		double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, isStirrup);
		double bendLen = RebarCode::GetBendLength(sizeKey, endType, modelRef);
		
        RebarCurve      rebarCurve;
        RebarEndTypes   endTypes = {endType, endType};
        makeRebarCurve (rebarCurve,pts.at(i) , bendRadius, bendLen, endTypes, endNormal);

        RebarSymbology symb;
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);

		CString cstr(LevelName.c_str());
		if (mCAddVerticalRebarflg)
		{
			symb.SetRebarLevel(TEXT_MAIN_REBAR);//画的是点筋则设置为主筋图层
		}
		else
			symb.SetRebarLevel (cstr);//钢筋锚入或钢筋合并设置为所选钢筋的图层

        RebarElementP rebarElement = rebarSet->AssignRebarElement (i, numRebar, symb, modelRef);
			
        if (nullptr != rebarElement)
        {
            RebarShapeData shape;
            shape.SetSizeKey ((LPCTSTR)sizeKey);
            shape.SetIsStirrup (isStirrup);
            shape.SetLength (rebarCurve.GetLength () / uor_per_mm);
            rebarElement->Update (rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
 			string Stype = "SingleRebar";
			if (mSelectedRebarType != "")
				Stype = mSelectedRebarType;
// 			if (mCAddVerticalRebarflg)
// 				Stype = "PointRebar";
// 			else
// 				Stype = "AnchorinRebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, mSelectedRebarLevel, num, Stype, modelRef);
			SetRebarHideData(tmprebar, spacing, ACTIVEMODEL);
			if (mreMark!=""&&mMarkname!="")
			{
				SetRebarCodeItemTypeValue(tmprebar, mreMark, mMarkname, L"DgnLib", L"UserMark", ACTIVEMODEL);
			}
			tmprebar.ReplaceInModel(oldref);
        }
    }

    RebarSetData setdata;
    setdata.SetNumber (numRebar);
    setdata.SetNominalSpacing (spacing);
    setdata.SetAverageSpacing(spacing);

    rebarSet->FinishUpdate (setdata, modelRef);
	rebarSet->SetSetData(setdata);
    RebarSetTag* tag = new RebarSetTag ();
    tag->SetRset (rebarSet);
    tag->SetIsStirrup (isStirrup);

    return tag;
}

bool SingleRebarAssembly::MakeRebars (DgnModelRefP modelRef)
{
    NewRebarAssembly (modelRef);
    RebarSetTagArray rsetTags;
     CVector3D   endNormal (1.0, 0.0, 0.0);
       
	RebarSetTag* tag = MakeRebars(m_SetId, m_spacing, m_rebarPts,endNormal, modelRef);
	if (NULL != tag)
	{
		tag->SetBarSetTag(1);
		rsetTags.Add(tag);
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

long SingleRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}

bool SingleRebarAssembly::OnDoubleClick ()
    {
	/* GetRebarData (g_slabRebarInfo);

	 int lastAction = ACTIONBUTTON_CANCEL;
	 if (SUCCESS != mdlDialog_openModal (&lastAction, GetResourceHandle (), DIALOGID_RebarSDKEditSlabRebar) || lastAction != ACTIONBUTTON_OK)
		 return false;

	 DgnModelRefP modelRef = ACTIVEMODEL;

	 SetRebarData (g_slabRebarInfo);
	 MakeRebars (modelRef);
	 Save (modelRef);*/

    return true;
    }

bool SingleRebarAssembly::Rebuild ()
    {
	/*ElementHandle slab (GetSelectedElement (), GetSelectedModel ());
	if (!slab.IsValid ())
		return false;

	DgnModelRefP modelRef = ACTIVEMODEL;

	SetSlabData (slab);
	MakeRebars (modelRef);
	Save (modelRef);*/

    return true;
    }

void SingleRebarAssembly::SetRebarData ()
    {

    }

void SingleRebarAssembly::GetRebarData () const
    {
    
    }


void SingleRebarAssembly::SetSelectedRebar(ElementHandleCR slectReabr, bool CAddVerticalRebarflg)
{
	if (CAddVerticalRebarflg)
	{
		mCAddVerticalRebarflg = CAddVerticalRebarflg;//画点筋标志
	}
	EditElementHandle eeh(slectReabr, false);
	string Level = "1";
	string Grade = "A";
	if (RebarElement::IsRebarElement(eeh))
	{
		GetRebarLevelItemTypeValue(eeh, mSelectedRebarLevel, mSelectedRebarType, mSelectedRebarGrade);//获取选中钢筋的属性，写入U形筋中		
	}
	mrebareeh = slectReabr;
}

bool SingleRebarAssembly::SetSlabData (ElementHandleCR slab)
    {
	ElementHandle eeh(slab);
	if (!eeh.IsValid())
	{
		ElementId conid = GetConcreteOwner();
		eeh = ElementHandle(conid, ACTIVEMODEL);
	}
    if (eeh.GetElementId () <= 0)
        return false;
    SetSelectedElement (eeh.GetElementId ());
    SetSelectedModel (eeh.GetModelRef ());
    return true;
    }

bool SingleRebarAssembly::IsSlabSolid (ElementHandleCR eh)
    {
    DgnBoxDetail        detail;
    ISolidPrimitivePtr  solid = ISolidPrimitiveQuery::ElementToSolidPrimitive (eh);
    if (!solid.IsValid () || !solid->TryGetDgnBoxDetail (detail))
        return false;

    DPoint3d    center;
    RotMatrix   rMatrix;
    DVec3d      size;
    return detail.IsBlock (center, rMatrix, size, 0.5, 0.5, 0.5);
    }
