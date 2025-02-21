/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/RebarSDKExample.h $
|
|  $Copyright: (c) 2019 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#pragma once

#include <DgnPlatform\DgnPlatformAPI.h>

#include <Mstn/MdlApi/MdlApi.h>
#include <DgnView/AccuDraw.h>
#include <DgnView/DgnElementSetTool.h>

#include "RebarElements.h"

#include "GalleryIntelligentRebarIds.h"
#include "GalleryIntelligentRebarcmd.h"
#include <Mstn\ISessionMgr.h> 

USING_NAMESPACE_BENTLEY_DGNPLATFORM;
USING_NAMESPACE_BENTLEY_MSTNPLATFORM;
USING_NAMESPACE_BENTLEY_MSTNPLATFORM_ELEMENT;


#if defined (NOT_USED)
/*=================================================================================**//**
* @bsiclass                                                               Bentley Systems
+===============+===============+===============+===============+===============+======*/
struct          TagsExampleHelper
{

private:

    static WString s_textStyleName;
    static WString s_tagSetName;

public:

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
    static WString GetOrCreateTextStyleName();

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
    static DgnTextStylePtr GetOrCreateTextStyle();

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
    static bool TagSetExists();

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
    static WString GetTagSetName();

};

#endif

RscFileHandle GetResourceHandle();

// commands by number
void testExample_createTagSet (WCharCP);
void testExample_placeTagFromText (WCharCP);

void RebarSDK_PlaceRebar (WCharCP unparsed);
void RebarSDK_ReadRebar (WCharCP unparsed);

//test
void PlaceWallRebar(WCharCP unparsed);

void FacesRebar(ElementHandleCR eeh, ElementId eehnew, const bvector<ISubEntityPtr> &faces);

void FacesRebarEx(ElementHandleCR eeh, ElementId eehnew, const bvector<ISubEntityPtr> &faces);
//end
// commands by name
// void RebarSDK_Test(WCharCP);
// void RebarSDK_Metro(WCharCP);
// void RebarSDK_KRB(WCharCP);
// void RebarSDK_PierVase(WCharCP);
// void RebarSDK_ARMA(WCharCP);

extern SlabRebarInfo    g_slabRebarInfo;

extern WString g_taskID;