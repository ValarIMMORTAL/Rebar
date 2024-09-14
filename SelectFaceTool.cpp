/*--------------------------------------------------------------------------------------+
|
|     $Source: MstnExamples/Elements/exampleSolids/exampleModifyFaceTool.cpp $
|
|  $Copyright: (c) 2015 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "exampleSolids.h"

USING_NAMESPACE_BENTLEY_DGNPLATFORM;
USING_NAMESPACE_BENTLEY_MSTNPLATFORM;
USING_NAMESPACE_BENTLEY_MSTNPLATFORM_ELEMENT;

/*=================================================================================**//**
* Example showing how to use LocateSubEntityTool to write a tool for applying
* local operations to faces using the solids kernel api.
* 
* @bsiclass                                                               Bentley Systems
+===============+===============+===============+===============+===============+======*/
struct          TestModifyFaceTool : LocateSubEntityTool
{
enum ToolOperation
    {
    OP_FaceOffset       = 0,
    OP_FaceTranslate    = 1,
    OP_FaceRotate       = 2,
    OP_FaceDelete       = 3,
    OP_FaceHollow       = 4,
    };

protected:

ToolOperation   m_operation;

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
TestModifyFaceTool (int cmdName, ToolOperation operation)
    {
    SetCmdName (cmdName, 0);
    m_operation = operation;
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
virtual bool _CollectCurves () override {return false;} // Tool does not support wire bodies...wire bodies won't be collected.
virtual bool _CollectSurfaces () override {return false;} // Tool does not support sheet bodies...sheet bodies won't be collected.

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
virtual BentleyStatus _OnProcessSolidPrimitive (ISolidPrimitivePtr& geomPtr, DisplayPathCR path) override {return ERROR;} // Promote capped surface to solid body...
virtual BentleyStatus _OnProcessPolyface (PolyfaceHeaderPtr& geomPtr, DisplayPathCR path) override {return SUCCESS;} // Don't convert a closed mesh to a BRep (and don't collect), can be expensive for large meshes...

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
virtual ISubEntity::SubEntityType _GetSubEntityTypeMask () override {return ISubEntity::SubEntityType_Face;}
virtual bool _RequireSubEntitySupport () override {return true;} // Require solid w/at least 1 face...
virtual bool _AcceptIdentifiesSubEntity () {return OP_FaceHollow != m_operation;} // Solid accept point may also accept first face (except hollow which can apply to entire body)...
virtual bool _AllowMissToAccept (DgnButtonEventCR ev) {return (OP_FaceHollow == m_operation ? true : __super::_AllowMissToAccept (ev));} // Don't require face for hollow...

/*---------------------------------------------------------------------------------**//**
* Return true if this element should be accepted for the modify operation.
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
virtual bool _IsElementValidForOperation (ElementHandleCR eh, HitPathCP path, WStringR cantAcceptReason) override
    {
    // Base class implementation returns true if geometry cache isn't empty, which in this case means the cache contains at least 1 BRep solid.
    // To be valid for modification element should be fully represented by a single solid; reject if there are multiple solid bodies or missing geometry.
    // NOTE: Simple count test is sufficient (w/o also checking TryGetAsBRep) as override of _Collect and _OnProcess methods have tool only collecting BRep solids.
    return (__super::_IsElementValidForOperation (eh, path, cantAcceptReason) && 1 == GetElementGraphicsCacheCount (eh) && !IsGeometryMissing (eh));
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
double GetDistance ()
    {
    // For testing purposes just use some % of element's range. This value would normally be supplied via tool settings.
    ScanRangeCP elRange = &GetElementAgenda ().GetFirstP ()->GetElementCP ()->hdr.dhdr.range;
    DPoint3d    range[2];

    range[0].x = (double) elRange->xlowlim;
    range[0].y = (double) elRange->ylowlim;
    range[0].z = (double) elRange->zlowlim;
    range[1].x = (double) elRange->xhighlim;
    range[1].y = (double) elRange->yhighlim;
    range[1].z = (double) elRange->zhighlim;

    return range[0].Distance (range[1]) * 0.1;
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
double GetAngle ()
    {
    // For testing purposes just use 10 degrees. This value would normally be supplied via tool settings.
    return Angle::DegreesToRadians (10.0);
    }

/*---------------------------------------------------------------------------------**//**
* Perform the solid operation using the accepted faces. 
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
StatusInt DoOperation (ISolidKernelEntityPtr& entityPtr, bvector<ISubEntityPtr>& faces)
    {
    switch (m_operation)
        {
        case OP_FaceTranslate:
        case OP_FaceRotate:
            {
            bvector<Transform>  translations;

            for each (ISubEntityPtr subEntity in faces)
                {
                DRange1d    uRange, vRange;
                Transform   translate = Transform::FromIdentity ();

                if (SUCCESS == SolidUtil::GetFaceParameterRange (*subEntity, uRange, vRange))
                    {
                    DPoint3d    point;
                    DVec3d      normal, uDir, vDir;
                    DPoint2d    uvParam;

                    uvParam.x = (uRange.low + uRange.high) * 0.5;
                    uvParam.y = (vRange.low + vRange.high) * 0.5;

                    if (SUCCESS == SolidUtil::EvaluateFace (*subEntity, point, normal, uDir, vDir, uvParam))
                        {
                        if (OP_FaceRotate == m_operation)
                            {
                            RotMatrix   rMatrix;

                            rMatrix.InitFromVectorAndRotationAngle (uDir, GetAngle ());
                            translate.InitFromMatrixAndFixedPoint (rMatrix, point);
                            }
                        else
                            {
                            normal.ScaleToLength (GetDistance ());
                            translate.SetTranslation (normal);
                            }
                        }
                    }

                translations.push_back (translate);
                }

            return SolidUtil::Modify::TransformFaces (entityPtr, &faces.front (), &translations.front (), faces.size ());
            }

        case OP_FaceDelete:
            {
            return SolidUtil::Modify::DeleteFaces (entityPtr, &faces.front (), faces.size ());
            }

        case OP_FaceHollow:
            {
            bvector<double> offsets;

            offsets.insert (offsets.begin (), faces.size (), 0.0); // Set accepted face offset to 0.0, distance used as offset for un-selected faces.

            return SolidUtil::Modify::HollowFaces (entityPtr, GetDistance () * 0.25, &faces.front (), &offsets.front (), faces.size ());
            }

        default:
            {
            bvector<double> offsets;

            offsets.insert (offsets.begin (), faces.size (), GetDistance ());

            return SolidUtil::Modify::OffsetFaces (entityPtr, &faces.front (), &offsets.front (), faces.size ());
            }
        }
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
virtual StatusInt _OnElementModify (EditElementHandleR eeh) override
    {
    bvector<IElementGraphicsPtr>  geomCache;

    // NOTE: Since we setup tool to only collect a single brep, we can just grab first cache entry...
    ISolidKernelEntityPtr   entityPtr = (SUCCESS == GetElementGraphicsCache (eeh, geomCache) ? TryGetAsBRep (geomCache.front ()) : NULL);

    if (!entityPtr.IsValid ())
        return ERROR;

    if (SUCCESS != DoOperation (entityPtr, GetAcceptedSubEntities ()))
        return ERROR;

    return SolidUtil::Convert::BodyToElement (eeh, *entityPtr, &eeh, *eeh.GetModelRef ());
    }

/*---------------------------------------------------------------------------------**//**
* Install a new instance of the tool. Will be called in response to external events
* such as undo or by the base class from _OnReinitialize when the tool needs to be
* reset to it's initial state.
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
virtual void _OnRestartTool () override
    {
    InstallNewInstance (GetToolId (), m_operation);
    }

public:

/*---------------------------------------------------------------------------------**//**
* Method to create and install a new instance of the tool. If InstallTool returns ERROR,
* the new tool instance will be freed/invalid. Never call delete on RefCounted classes.
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
static void InstallNewInstance (int toolId, ToolOperation operation)
    {
    TestModifyFaceTool* tool = new TestModifyFaceTool (toolId, operation);

    tool->InstallTool ();
    }

}; // TestModifyFaceTool

/*=================================================================================**//**
* Functions associated with command numbers for starting tools.
* @param[in] unparsed Additional input supplied after command string.
+===============+===============+===============+===============+===============+======*/
/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
Public void startExampleFaceOffsetTool (WCharCP unparsed)
    {
    // NOTE: Call the method to create/install the tool, RefCounted classes don't have public constructors...
    TestModifyFaceTool::InstallNewInstance (CMDNAME_ExampleFaceOffsetTool, TestModifyFaceTool::OP_FaceOffset);
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
Public void startExampleFaceTranslateTool (WCharCP unparsed)
    {
    // NOTE: Call the method to create/install the tool, RefCounted classes don't have public constructors...
    TestModifyFaceTool::InstallNewInstance (CMDNAME_ExampleFaceTranslateTool, TestModifyFaceTool::OP_FaceTranslate);
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
Public void startExampleFaceRotateTool (WCharCP unparsed)
    {
    // NOTE: Call the method to create/install the tool, RefCounted classes don't have public constructors...
    TestModifyFaceTool::InstallNewInstance (CMDNAME_ExampleFaceRotateTool, TestModifyFaceTool::OP_FaceRotate);
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
Public void startExampleFaceDeleteTool (WCharCP unparsed)
    {
    // NOTE: Call the method to create/install the tool, RefCounted classes don't have public constructors...
    TestModifyFaceTool::InstallNewInstance (CMDNAME_ExampleFaceDeleteTool, TestModifyFaceTool::OP_FaceDelete);
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
Public void startExampleFaceHollowTool (WCharCP unparsed)
    {
    // NOTE: Call the method to create/install the tool, RefCounted classes don't have public constructors...
    TestModifyFaceTool::InstallNewInstance (CMDNAME_ExampleFaceHollowTool, TestModifyFaceTool::OP_FaceHollow);
    }
