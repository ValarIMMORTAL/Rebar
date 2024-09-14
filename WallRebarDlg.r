/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/dialogs.r $
|
|  $Copyright: (c) 2019 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------+
|                                                                       |
|                                                                       |
+----------------------------------------------------------------------*/
/*----------------------------------------------------------------------+
|                                                                       |
|   Function -                                                          |
|                                                                       |
|       Basic Dialog Example Resources                                  |
|                                                                       |
+----------------------------------------------------------------------*/
/*----------------------------------------------------------------------+
|                                                                       |
|   Include Files                                                       |
|                                                                       |
+----------------------------------------------------------------------*/
#include <Mstn\MdlApi\dlogbox.r.h>    /* dlog box manager resource constants & structs */
#include <Mstn\MdlApi\dlogids.r.h>    /* MicroStation resource IDs */

#include "GalleryIntelligentRebarids.h"
#include "WallRebarDlg.h"      /* basic dialog box example constants & structs */
#include "WallRebarDlgtxt.h"   /* basic dialog box static text defines */

DialogBoxRsc DIALOGID_WallRebar =
{
    DIALOGATTR_DEFAULT | DIALOGATTR_SINKABLE,
    100*XC, 28*YC,
    NOHELP, MHELP, HOOKDIALOGID_WallRebar, NOPARENTID,
	TXT_WallRebar,
	{
		{{0.5*XC, GENY(1), 95*XC, 0},	TabPageList, TPLISTID_WallRebar, ON, 0,"",""},
		{{70*XC,  GENY(21), 8*XC,  0},	PushButton,  PUSHBUTTONID_OK, ON, 0, TXT_Apply, ""},	//”¶”√
	}
};

DItem_TabPageListRsc  TPLISTID_WallRebar =
{
    0, 0,
    NOSYNONYM, 
    NOHELP, MHELP, NOHOOK, NOARG,
    TABATTR_DEFAULT,
    TXT_NULL,
	{
	{{0,0,0,0}, TabPage, TABPAGEID_MainRebar,	ON, 0,"",""},
	{{0,0,0,0}, TabPage, TABPAGEID_LapOptions,	ON, 0,"",""},
	{{0,0,0,0}, TabPage, TABPAGEID_EndType,		ON, 0,"",""},
	{{0,0,0,0}, TabPage, TABPAGEID_AssociatedComponent,	ON, 0,"",""},
	}
};

DItem_TabPageRsc  TABPAGEID_MainRebar =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, 
    HOOKTABPAGEID_MainRebar, NOARG,
    TABATTR_DEFAULT,
    NOTYPE, NOICON,
    TXT_MainRebar,
	{
		{{35*XC,	GENY(6.5),	8 * XC,	1*YC},	ComboBox,	COMBOBOXID_IsTwinBars,	ON,	0,			TXT_NULL,		""},
		{{7*XC,		GENY(4),	8 * XC,	1*YC},	Text,		TEXTID_PostiveCover,	ON, 0,			TXT_NULL,		""},
		{{30*XC,	GENY(4),	8 * XC,	1*YC},	Text,		TEXTID_ReverseCover,	ON, 0,			TXT_NULL,		""},
		{{53*XC,	GENY(4),	8 * XC,	1*YC},	Text,		TEXTID_ProfileCover,	ON, 0,			TXT_NULL,		""},
		{{10*XC,	GENY(6.5),	8 * XC,	1*YC},	Text,		TEXTID_Levels,			ON, 0,			TXT_NULL,		""},
		{{25.5*XC,	GENY(6.5),	8 * XC,	1*YC},	Label,		0,						ON, ALIGN_LEFT,	TXT_TwinBars,	""},
		{{1.5*XC,	GENY(9),	90* XC,	14*YC},	ListBox,	LISTBOXID_Rebar,		ON, 0,			TXT_NULL,		""},
		{{1.5*XC,	GENY(3),	90* XC, 3.5*YC},GroupBox,	0,						ON, 0,			TXT_LevelView,	""},

	}
};

DItem_TabPageRsc  TABPAGEID_LapOptions =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, 
    HOOKTABPAGEID_LapOptions, NOARG,
    TABATTR_DEFAULT,
    NOTYPE, NOICON,
    TXT_LapOptions,
	{
		{{1.5*XC,	GENY(4),	94 * XC,	20*YC},ListBox ,	LISTBOXID_LapOptions,	ON, 0, "", ""},
	}
};

DItem_TabPageRsc  TABPAGEID_EndType =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, 
    HOOKTABPAGEID_EndType, NOARG,
    TABATTR_DEFAULT,
    NOTYPE, NOICON,
    TXT_EndStyle,
	{
		{{1.5*XC,	GENY(4),	94 * XC,	20*YC},ListBox ,	LISTBOXID_EndType,	ON, 0, "", ""},
	}
};

DItem_TabPageRsc  TABPAGEID_AssociatedComponent =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, 
    NOHOOK, NOARG,
    TABATTR_DEFAULT,
    NOTYPE, NOICON,
    TXT_AssociatedComponent,
	{
		{{1.5*XC,	GENY(4),	62 * XC,	20*YC},	ListBox,LISTBOXID_AssociatedComponent,	ON, 0,			"",					""},
		{{64*XC,	GENY(3),	20 * XC,	1*YC},	Label,	0,								ON, ALIGN_LEFT,	TXT_AnchorLinkPatternDiagram, ""},
	}
};
	
DItem_PushButtonRsc PUSHBUTTONID_WallRebar_Apply =
{
	NOT_DEFAULT_BUTTON, NOHELP, MHELP, HOOKBUTTONID_WallRebar_Apply, NOARG,
	NOCMD, LCMD, "",
	""
};

DItem_ListBoxRsc LISTBOXID_Rebar =
{
    NOHELP, LHELPCMD, HOOKLISTBOXID_MainRebar, NOARG,
    LISTATTR_DRAWPREFIXICON | LISTATTR_EDITABLE | LISTATTR_GRID | LISTATTR_INDEPENDENTCOLS,
	4, 1,
    "",
    {
        {10*XC, 45, ALIGN_LEFT, TXT_Level},
        {10*XC, 45, ALIGN_LEFT, TXT_Direction},
		{10*XC, 45, ALIGN_LEFT, TXT_Diameter},
        {10*XC, 45, ALIGN_LEFT, TXT_Type},
        {10*XC, 45, ALIGN_LEFT, TXT_Spacing},
        {10*XC, 45, ALIGN_LEFT, TXT_StartOffset},
        {10*XC, 45, ALIGN_LEFT, TXT_EndOffset},
		{10*XC, 45, ALIGN_LEFT, TXT_LevelSpacing},
		{10*XC, 45, ALIGN_LEFT, TXT_DataChange},
    }
};	

DItem_ListBoxRsc LISTBOXID_LapOptions =
{
    NOHELP, LHELPCMD, HOOKLISTBOXID_LapOptions, NOARG,
    LISTATTR_DRAWPREFIXICON | LISTATTR_EDITABLE | LISTATTR_GRID | LISTATTR_INDEPENDENTCOLS,
	4, 1,
    "",
    {
        {13.5*XC, 45, ALIGN_LEFT, TXT_Level},
        {13.5*XC, 45, ALIGN_LEFT, TXT_ConnectionMethod},
		{13.5*XC, 45, ALIGN_LEFT, TXT_LapLength},
        {13.5*XC, 45, ALIGN_LEFT, TXT_StockLength},
        {13.5*XC, 45, ALIGN_LEFT, TXT_MillLength},
        {13.5*XC, 45, ALIGN_LEFT, TXT_Staggered},
        {10  *XC, 45, ALIGN_LEFT, TXT_StaggeredLength},

    }
};	

DItem_ListBoxRsc LISTBOXID_EndType =
{
	NOHELP, LHELPCMD, HOOKLISTBOXID_EndType, NOARG,
    LISTATTR_DRAWPREFIXICON | LISTATTR_EDITABLE | LISTATTR_GRID | LISTATTR_INDEPENDENTCOLS,
	4, 1,
	"",
	{
		{15*XC, 45, ALIGN_LEFT, TXT_Position},
		{15*XC, 45, ALIGN_LEFT, TXT_Type},
		{15*XC, 45, ALIGN_LEFT, TXT_EndProperty},
		{15*XC, 45, ALIGN_LEFT, TXT_Offset},
		{15*XC, 45, ALIGN_LEFT, TXT_RotationAngle},
		{12*XC, 45, ALIGN_LEFT, TXT_Clear},
	}
};

DItem_ListBoxRsc LISTBOXID_AssociatedComponent =
{
    NOHELP, LHELPCMD, NOHOOK, NOARG,
    LISTATTR_DRAWPREFIXICON | LISTATTR_EDITABLE | LISTATTR_GRID | LISTATTR_INDEPENDENTCOLS,
	4, 1,
    "",
    {
        {10*XC, 45, ALIGN_LEFT, TXT_ReinforcedWallName},
        {10*XC, 45, ALIGN_LEFT, TXT_AssociatedComponent},
		{10*XC, 45, ALIGN_LEFT, TXT_ComponentRelationship},
        {10*XC, 45, ALIGN_LEFT, TXT_ReinforcedRebarLevel},
        {10*XC, 45, ALIGN_LEFT, TXT_Association},
        {7 *XC, 45, ALIGN_LEFT, TXT_AnchorLinkPattern},
    }
};


DItem_TextRsc TEXTID_PostiveCover =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    NOHOOK, NOARG,
    12, "%w", "%w", "", "", NOMASK, NOCONCAT,
    TXT_PostiveCover, "g_wallRebarInfo.concrete.postiveCover"
};

DItem_TextRsc TEXTID_ReverseCover =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    NOHOOK, NOARG,
    12, "%w", "%w", "", "", NOMASK, NOCONCAT,
    TXT_ReverseCover, "g_wallRebarInfo.concrete.reverseCover"
};

DItem_TextRsc TEXTID_ProfileCover =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    NOHOOK, NOARG,
    12, "%w", "%w", "", "", NOMASK, NOCONCAT,
    TXT_ProfileCover, "g_wallRebarInfo.concrete.sideCover"
};

DItem_TextRsc TEXTID_Levels =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    HOOKDIALOGIDTEXT_Level, NOARG,
    12, "%d", "%d", "0", "10", NOMASK, NOCONCAT,
    TXT_Levels, "g_wallRebarInfo.concrete.rebarLevelNum"
};

 DItem_ComboBoxRsc COMBOBOXID_IsTwinBars =
{
	NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, NOHOOK,
	NOARG, 10, "%s", "%s", "", "", NOMASK,STRINGLISTID_IsTwinBars, 10, 0, 0, 0,
	COMBOATTR_READONLY | COMBOATTR_INDEXISVALUE | COMBOATTR_DRAWPREFIXICON,
	"","g_wallRebarInfo.concrete.isTwinbars",
	{
		{ 20 * XC, 20, 0, "" },
	}
};

 DItem_ComboBoxRsc ListBoxComBoBoxID_RebarDir =
{
	NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, HOOKLISTBOXCOMBOBOXID_MainRebar,
	NOARG, 10, "%s", "%s", "", "", NOMASK,STRINGLISTID_RebarDir, 10, 0, 0, 0,
	COMBOATTR_READONLY | COMBOATTR_INDEXISVALUE | COMBOATTR_DRAWPREFIXICON,
	"","g_wallRebarInfo.rebar.rebarDir",
	{
		{ 20 * XC, 20, 0, "" },
	}
};

DItem_ComboBoxRsc ListBoxComBoBoxID_RebarSize =
{
	NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, HOOKLISTBOXCOMBOBOXID_MainRebar,
	NOARG, 10, "%s", "%s", "", "", NOMASK,STRINGLISTID_RebarSize, 10, 0, 0, 0,
	COMBOATTR_READONLY | COMBOATTR_DRAWPREFIXICON,
	"","g_wallRebarInfo.rebar.rebarSize",
	{
		{ 20 * XC, 20, 0, "" },
	}
};

DItem_ComboBoxRsc ListBoxComBoBoxID_RebarType =
{
	NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, HOOKLISTBOXCOMBOBOXID_MainRebar,
	NOARG, 10, "%s", "%s", "", "", NOMASK,STRINGLISTID_RebarType, 10, 0, 0, 0,
	COMBOATTR_READONLY | COMBOATTR_INDEXISVALUE | COMBOATTR_DRAWPREFIXICON,
	"","g_wallRebarInfo.rebar.rebarType",
	{
		{ 20 * XC, 20, 0, "" },
	}
};

DItem_TextRsc ListBoxTextID_RebarSpace =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    HOOKLISTBOXTEXTID_MainRebar, NOARG,
    12, "%f", "%f", "", "", NOMASK, NOCONCAT,
    "", "g_wallRebarInfo.rebar.spacing"
};

DItem_TextRsc ListBoxTextID_RebarStartOffset =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    HOOKLISTBOXTEXTID_MainRebar, NOARG,
    12, "%f", "%f", "", "", NOMASK, NOCONCAT,
    "", "g_wallRebarInfo.rebar.startOffset"
};

DItem_TextRsc ListBoxTextID_RebarEndOffset =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    HOOKLISTBOXTEXTID_MainRebar, NOARG,
    12, "%f", "%f", "", "", NOMASK, NOCONCAT,
    "", "g_wallRebarInfo.rebar.endOffset"
};

DItem_TextRsc ListBoxTextID_RebarLevelSpace =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    HOOKLISTBOXTEXTID_MainRebar, NOARG,
    12, "%f", "%f", "", "", NOMASK, NOCONCAT,
    "", "g_wallRebarInfo.rebar.levelSpace"
};

DItem_ComboBoxRsc ListBoxComBoBoxID_RebarDataChange =
{
	NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, HOOKLISTBOXCOMBOBOXID_MainRebar_RebarDataChange,
	NOARG, 10, "%s", "%s", "", "", NOMASK,0, 10, 0, 0, 0,
	COMBOATTR_READONLY | COMBOATTR_INDEXISVALUE | COMBOATTR_DRAWPREFIXICON,
	"","g_wallRebarInfo.rebar.datachange",
	{
		{ 20 * XC, 20, 0, "" },
	}
};

DItem_ComboBoxRsc ListBoxComBoBoxID_EndType =
{
	NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, HOOKLISTBOXCOMBOBOXID_EndType,
	NOARG, 10, "%s", "%s", "", "", NOMASK,STRINGLISTID_EndType, 10, 0, 0, 0,
	COMBOATTR_READONLY | COMBOATTR_INDEXISVALUE | COMBOATTR_DRAWPREFIXICON,
	"","g_wallRebarInfo.endtype.endType",
	{
		{ 20 * XC, 20, 0, "" },
	}
};

DItem_TextRsc ListBoxTextID_Offset =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    HOOKLISTBOXTEXTID_EndType, NOARG,
    12, "%f", "%f", "", "", NOMASK, NOCONCAT,
    "", "g_wallRebarInfo.endtype.offset"
};

DItem_TextRsc ListBoxTextID_RotateAngle =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    HOOKLISTBOXTEXTID_EndType, NOARG,
    12, "%f", "%f", "", "", NOMASK, NOCONCAT,
    "", "g_wallRebarInfo.endtype.rotateAngle"
};

DItem_ComboBoxRsc ListBoxComBoBoxID_ConnectMethod =
{
	NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, HOOKLISTBOXCOMBOBOXID_LapOptions,
	NOARG, 10, "%s", "%s", "", "", NOMASK,STRINGLISTID_ConnectMethod, 10, 0, 0, 0,
	COMBOATTR_READONLY | COMBOATTR_INDEXISVALUE | COMBOATTR_DRAWPREFIXICON,
	"","g_wallRebarInfo.lapoption.connectMethod",
	{
		{ 20 * XC, 20, 0, "" },
	}
};

DItem_TextRsc ListBoxTextID_LapLength =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    HOOKLISTBOXTEXTID_LapOptions, NOARG,
    12, "%f", "%f", "", "", NOMASK, NOCONCAT,
    "", "g_wallRebarInfo.lapoption.lapLength"
};

DItem_TextRsc ListBoxTextID_StockLength =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    HOOKLISTBOXTEXTID_LapOptions, NOARG,
    12, "%f", "%f", "", "", NOMASK, NOCONCAT,
    "", "g_wallRebarInfo.lapoption.stockLength"
};

DItem_TextRsc ListBoxTextID_MillLength =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    HOOKLISTBOXTEXTID_LapOptions, NOARG,
    12, "%f", "%f", "", "", NOMASK, NOCONCAT,
    "", "g_wallRebarInfo.lapoption.millLength"
};

DItem_ComboBoxRsc ListBoxComBoBoxID_IsStaggered =
{
	NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP, HOOKLISTBOXCOMBOBOXID_LapOptions,
	NOARG, 10, "%s", "%s", "", "", NOMASK,STRINGLISTID_IsStaggered, 10, 0, 0, 0,
	COMBOATTR_READONLY | COMBOATTR_INDEXISVALUE | COMBOATTR_DRAWPREFIXICON,
	"","g_wallRebarInfo.lapoption.isStaggered",
	{
		{ 20 * XC, 20, 0, "" },
	}
};

DItem_TextRsc ListBoxTextID_StaggeredLength =
{
    NOCMD, LCMD, NOSYNONYM, NOHELP, MHELP,
    HOOKLISTBOXTEXTID_LapOptions, NOARG,
    12, "%f", "%f", "", "", NOMASK, NOCONCAT,
    "", "g_wallRebarInfo.lapoption.staggeredLength"
};