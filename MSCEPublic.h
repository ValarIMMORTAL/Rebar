#pragma once
#include <Mstn/MdlApi/mselmdsc.fdf> 
#include <Mstn/MdlApi/dlogitem.h>    /* Dialog Box Manager structures & constants */
#include <Mstn/MdlApi/dlogman.fdf>   /* dialog box manager function prototypes */
#include <Mstn/MdlApi/mselems.h>
#include <Mstn/MdlApi/mselemen.fdf>
#include <Mstn/MdlApi/msdialog.fdf>
#include <Mstn/MdlApi/msritem.fdf>
#include <Mstn/MdlApi/msrmgr.h>
#include <Mstn/MdlApi/leveltable.fdf>
#include <Mstn/MdlApi/mscell.fdf>
#include <Mstn/MdlApi/msdgnobj.fdf>
#include <Mstn/MdlApi/mdlerrs.r.h>
#include <Mstn/MdlApi/msassoc.fdf>
#include <Mstn/MdlApi/msdgnmodelref.fdf>
#include <Mstn/MdlApi/mssystem.fdf>
#include <Mstn/MdlApi/msinput.fdf>
#include <Mstn/MdlApi/msmodel.fdf>
#include <Mstn/MdlApi/msparse.fdf>
#include <Mstn/MdlApi/msstate.fdf>
#include <Mstn/MdlApi/mscexpr.fdf>
#include <Mstn/MdlApi/ditemlib.fdf>
#include <Mstn/MdlApi/mscurrtr.fdf>
#include <Mstn/MdlApi/mspop.h>
#include <Mstn/MdlApi/msmdlmesh.fdf>
#include <Mstn/MdlApi/mstmatrx.fdf> 
#include <Mstn/MdlApi/msvec.fdf> 
#include <Mstn/MdlApi/msoutput.fdf> 
#include <Mstn/MdlApi/msbsplin.fdf> 
#include <Mstn/PSolid/mssolid.h>
#include <Mstn/PSolid/mssolid.fdf>
#include <Mstn/DocumentManager.h>
#include <Mstn/MdlApi/MdlApi.h>
#include <Mstn/basetype.h>
#include <Mstn/XmlTools/mdlxmltools.fdf>
#include <DgnPlatform/DgnDocumentManager.h>
#include <DgnPlatform/ElementHandle.h>
#include <DgnPlatform/CustomItemType.h>
#include <DgnPlatform/ChainHeaderHandlers.h>
#include <DgnPlatform/LinearHandlers.h>
#include <DgnPlatform/ArcHandlers.h>
#include <DgnPlatform/SurfaceAndSolidHandlers.h>
#include <DgnPlatform/DropGraphics.h>
#include <DgnPlatform/MeshHeaderHandler.h>
#include <DgnPlatform/ISettings.h>
#include <DgnPlatform/DgnECInstance.h>
#include <DgnPlatform/ExportMacros.h>
#include <DgnPlatform/DgnFileIO/DgnElements.h>
#include <ECObjects/ECObjectsAPI.h>
#include <PSolid/PSolidCoreAPI.h>
#include <Geom/GeomApi.h>


#define UOR_PER_MM(uor, mm, modelRef) *(uor) = mdlModelRef_getUorPerMeter(modelRef)/1000 * (mm)
#define UOR_PER_METER	(mdlModelRef_getUorPerMeter(ACTIVEMODEL))			//m
#define UOR_PER_MilliMeter (mdlModelRef_getUorPerMeter(ACTIVEMODEL) / 1000)