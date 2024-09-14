/*--------------------------------------------------------------------------------------+
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
+--------------------------------------------------------------------------------------*/
#pragma once
#ifndef _INCLUDED_REBAR_V8_COMMON__USTATION_H_
#define _INCLUDED_REBAR_V8_COMMON__USTATION_H_

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <wincrypt.h>
#include <afxcontrolbars.h>
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#include <Bentley\bentley.h>
#include <DgnPlatform\DgnPlatform.h>

USING_NAMESPACE_BENTLEY
USING_NAMESPACE_BENTLEY_DGNPLATFORM

//either one of following 3 lines will disable DVec3d legacy mode that is being set by mdl.h and prevents dll import from ustation.dll that use DVec3d
//#include <msgeomstructs.hpp> //must be included before "mdl.h"
//#include <basetype.h>
//#define NO_LEGACY_DVEC3D
#include <Bentley\WString.h>
//#include <Geom\msgeomstructs.hpp>   not published!
#include <Mstn\MdlApi\mdl.h>
#include <Mstn\MdlApi\mdlerrs.r.h>
#include <Mstn\MdlApi\mselems.h>
#include <Mstn\MdlApi\mdlbspln.h>
#include <Mstn\MdlApi\msinputq.h>
#include <Mstn\MdlApi\cmdclass.r.h>
#include <Mstn\MdlApi\dlogbox.r.h>
#include <Mstn\MdlApi\dlogitem.h>
#include <Mstn\MdlApi\global.h>
#include <Mstn\MdlApi\cexpr.h>
#include <Mstn\MdlApi\rscdefs.r.h>
#include <Mstn\MdlApi\userfnc.h>
#include <DgnPlatform\Tcb\tcb.r.h>
#include <Mstn\MdlApi\scanner.h>
#include <Mstn\MdlApi\dlogids.r.h>
#include <DgnPlatform\image.h>
#include <Mstn\MdlApi\msdim.fdf>
#include <Mstn\MdlApi\mscell.fdf>
#include <Mstn\MdlApi\msselect.fdf>
#include <Mstn\MdlApi\msdialog.fdf>
#include <Mstn\MdlApi\dlogman.fdf>
#include <Mstn\MdlApi\msassoc.fdf>
#include <Mstn\MdlApi\msrsrc.fdf>
#include <Mstn\MdlApi\msparse.fdf>
#include <Mstn\MdlApi\msoutput.fdf>
#include <Mstn\MdlApi\mscexpr.fdf>
#include <Mstn\MdlApi\msstring.fdf>
#include <Mstn\MdlApi\mssystem.fdf>
#include <Mstn\MdlApi\mscnv.fdf>
#include <Mstn\MdlApi\msvec.fdf>
#include <Mstn\MdlApi\msrmatrx.fdf>
#include <Mstn\MdlApi\mstmatrx.fdf>
#include <Mstn\MdlApi\msstate.fdf>
#include <Mstn\MdlApi\mstrnsnt.fdf>
#include <Mstn\MdlApi\mselemen.fdf>
#include <Mstn\MdlApi\mselmdsc.fdf>
#include <Mstn\MdlApi\msscan.fdf>
#include <Mstn\MdlApi\msfile.fdf>
#include <Mstn\MdlApi\msimage.fdf>
#include <Mstn\MdlApi\msstrngl.fdf>

#include <Mstn\MdlApi\msinput.fdf>
#include <Mstn\MdlApi\mscurrtr.fdf>
#include <Mstn\MdlApi\msdgnmodelref.fdf>
#include <Mstn\MdlApi\mslinkge.fdf>

// since V8
#if defined (__cplusplus)
// will not be compiled with resource files from here on
#include <Mstn\MdlApi\msbsplin.fdf>
#include <Mstn\MdlApi\msmodel.fdf>
#include <Mstn\MdlApi\msstring.fdf>
#include <Mstn\MdlApi\msritem.fdf>
#include <Mstn\MdlApi\msreffil.fdf>
#include <Mstn\MdlApi\msbsplin.fdf>
#include <Mstn\MdlApi\msdgnmodelref.fdf>
#include <Mstn\MdlApi\leveltable.fdf>
#include <Mstn\MdlApi\msmisc.fdf>
#include <Mstn\MdlApi\msvar.fdf>
#include <Mstn\MdlApi\modelindex.fdf>
#include <Mstn\MdlApi\workmode.fdf>
#include <Mstn\MdlApi\listmodel.fdf>
#include <Mstn\MdlApi\mslstyle.fdf>
#include <Mstn\MdlApi\elementref.h>
#include <Mstn\MdlApi\msdisplaypath.h>
#include <Mstn\MdlApi\dlmsys.fdf>
#include <Mstn\MdlApi\mdllib.fdf>
#include <Mstn\MdlApi\msunits.fdf>

#include <Mstn\MdlApi\mslocate.fdf>
#include <Mstn\MdlApi\msview.fdf>
#include <Mstn\MdlApi\mswrkdgn.fdf>
#include <Mstn\MdlApi\msscancrit.fdf>
#include <Mstn\MdlApi\msbnrypo.h>
#include <DgnPlatform\DgnPlatformBaseType.r.h>
#include <Mstn\MdlApi\msdwgappdata.fdf>
#include <Mstn\MdlApi\mswindow.fdf>
#include <Mstn\MdlApi\miscilib.fdf>
#include <Mstn\MdlApi\treemodel.fdf>
#include <Mstn\PSolid\mssolid.h>
#include <Mstn\MdlApi\msbrepcommon.r.h>
#include <DgnPlatform\image.h>
#include <ImageLib\imageLibApi.h>
//#include <Mstn\MdlApi\ImageLib.fdf>  not delivered in SDK
#include <DgnPlatform\TextBlock\TextBlock.h>
// #include <LicenseClient\LicClient.h> not published

#include <DgnPlatform\Handler.h>
#include <ECObjects\ECSchema.h>
#include <ECObjects\ECObjects.h>
#include <ECObjects\ECValue.h>
#include <DgnPlatform\DisplayFilter.h>

#include <DgnPlatform\DgnPlatformAPI.h>
//#include <DgnPlatform\ECXDProvider.h>  not published
#include <DgnPlatform\ExtendedElementHandler.h>
// #include <DgnPlatform\DelegatedElementECEnabler.h>  not published
#include <DgnPlatform\DgnPlatform.r.h>

#include <Rebar/sdk_ustation.h>
#include "elementalgorithm.h"
typedef void *ScanBufPtr;

#ifdef fwrite
#undef fwrite
#endif

#undef boolean
#undef byte

#define _GPK_NO_BOOLEAN_DEF_
// < NEEDS_PS_VANCOUVER_REVIEW - This DgnDrawMode seems not to be supported in CE
#define     DRAW_MODE_ClearHilite   (DgnDrawMode)64
// />
#define     USTN_TYPEDEFS       1

#include "flag.h"
#include "BeDefiner.h"
#include "BeRelation.h"
//#include "RebarModel.h"
#endif // __cplusplus

typedef unsigned int        USTN_UINT;
typedef int                 USTN_INT;
typedef long                LONG_COUNT;

//global MS resource file handle
extern RscFileHandle g_rfHandle;

#define _USTN_CONVERTASSERTSTOPRINTF 1
//#define _WIN32_WINNT _WIN32_WINNT_MAXVER
#endif
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>
#include <afxcontrolbars.h>

