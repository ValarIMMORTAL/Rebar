/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/dialogs.h $
|
|  $Copyright: (c) 2019 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#pragma once

//--------------------------------------------------------------------
// This file is included by both .cpp/h and .r files
//--------------------------------------------------------------------
enum WallRebarDialogIds
{
	DIALOGID_WallRebar = 1,
	LISTBOXID_Rebar,
	LISTBOXID_LapOptions,
	LISTBOXID_EndType,
	LISTBOXID_AssociatedComponent,
	PUSHBUTTONID_WallRebar_Apply,
	COMBOBOXID_IsTwinBars,
	TPLISTID_WallRebar,
	STRINGLISTID_IsTwinBars,
	STRINGLISTID_RebarDir,
	STRINGLISTID_RebarSize,
	STRINGLISTID_RebarType,
	STRINGLISTID_EndType,
	STRINGLISTID_ConnectMethod,
	STRINGLISTID_IsStaggered,
	TEXTID_PostiveCover,
	TEXTID_ReverseCover,
	TEXTID_ProfileCover,
	TEXTID_Levels,
	TEXTID_IsTwinBars,
	ListBoxComBoBoxID_RebarDir,
	ListBoxComBoBoxID_RebarSize,
	ListBoxComBoBoxID_RebarType,
	ListBoxTextID_RebarSpace,
	ListBoxTextID_RebarStartOffset,
	ListBoxTextID_RebarEndOffset,
	ListBoxTextID_RebarLevelSpace,
	ListBoxComBoBoxID_RebarDataChange,
	ListBoxComBoBoxID_EndType,
	ListBoxTextID_Offset,
	ListBoxTextID_RotateAngle,
	ListBoxComBoBoxID_ConnectMethod,
	ListBoxTextID_LapLength,
	ListBoxTextID_StockLength,
	ListBoxTextID_MillLength,
	ListBoxComBoBoxID_IsStaggered,
	ListBoxTextID_StaggeredLength,
};

enum RebarHookIds
    {
	HOOKDIALOGID_WallRebar = 1,
	HOOKLISTBOXID_MainRebar,
	HOOKLISTBOXID_LapOptions,
	HOOKLISTBOXID_EndType,
	HOOKLISTBOXID_AssociatedComponent,
	HOOKBUTTONID_WallRebar_Apply,
	HOOKTABPAGEID_MainRebar,
	HOOKLISTBOXCOMBOBOXID_MainRebar,
	HOOKLISTBOXTEXTID_MainRebar,
	HOOKLISTBOXCOMBOBOXID_MainRebar_RebarDataChange,
	HOOKDIALOGIDTEXT_Level,
	HOOKTABPAGEID_EndType,
	HOOKLISTBOXCOMBOBOXID_EndType,
	HOOKLISTBOXTEXTID_EndType,
	HOOKTABPAGEID_LapOptions,
	HOOKLISTBOXCOMBOBOXID_LapOptions,
	HOOKLISTBOXTEXTID_LapOptions,
};


#define	TABPAGEID_MainRebar					 1
#define	TABPAGEID_LapOptions				 2
#define	TABPAGEID_EndType					 3
#define	TABPAGEID_AssociatedComponent        4