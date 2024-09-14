/*--------------------------------------------------------------------------------------+
|  $Copyright: (c) 2019 Bentley Systems, Incorporated. All rights reserved. $
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include "RebarModel.h"
#include "GalleryIntelligentRebar.h"
#include "rebarSDK_2d.h"
#include "PITMSCECommon.h"

//bool SDK_RebarDetailer::IsUserDefined(BarImplementor *theBar, TDrawCommand drawCommand, int calcMode) const
//{
//    if (theBar != nullptr)
//    {
//		 ElementId elemId = GetRebarDetailSetId(theBar);
//		 DgnModelRefP modelRef = GetModelRef(theBar);
//		 ElementHandle eh(elemId, modelRef);
//		 RebarDetailSet *rds = RebarDetailSet::Fetch(eh);
//		 if (rds != nullptr)
//		 {
//			 RebarShapeCP shape = nullptr;
//			 RebarElementCP rep = nullptr;
//			 RebarSetCP rset = nullptr;
//			 rds->GetAssociatedRebarElement(rset, rep, shape, modelRef);
//			 for (RebarDetailElement *rde : *rds)
//			 {
//				 RebarCurveVectorCR rebar_curve = rde->GetCurveSegment();
//
//			 }
//		 }
//    }
//
//    return false;
//}
//
//bool SDK_RebarDetailer::DrawUserBar(BarImplementor *theBar, TDrawCommand drawCommand, int calcMode) const
//{
//    if (theBar != nullptr)
//    {
//        ElementId elemId = GetRebarDetailSetId(theBar);
//        DgnModelRefP modelRef = GetModelRef(theBar);
//        ElementHandle eh(elemId, modelRef);
//        RebarDetailSet *rds = RebarDetailSet::Fetch(eh);
//        if (rds != nullptr)
//        {
//			RebarShapeCP shape = nullptr;
//			RebarElementCP rep = nullptr;
//			RebarSetCP rset = nullptr;
//			rds->GetAssociatedRebarElement(rset, rep, shape, modelRef);
//			intptr_t num = rds->GetChildElementCount();
//			if (num > 1)
//			{
//				RebarDetailElement *rde = rds->GetChildElement(0);
//				ElementId rdeid = rde->GetElementId();
//				EditElementHandle rdeeeh(rdeid, modelRef);
//				ElementRefP oldref = rdeeeh.GetElementRef();
//				const RebarElement *rebarele = rde->GetRebarElement(modelRef);
//				if (rebarele != nullptr)
//				{
//					DgnModelRefP  modelref = rebarele->GetModelRef(modelRef);
//					ElementId eleid = rebarele->GetElementId();
//					EditElementHandle testeeh(eleid, modelref);
//					string level, grade;
//					GetRebarLevelItemTypeValue(testeeh, level, grade);
//					SetRebarLevelItemTypeValue(rdeeeh, level, 1, modelref);
//					rdeeeh.ReplaceInModel(oldref);
//				}
//				RebarCurveVectorCR rebar_curve = rde->GetCurveSegment();
//				CPoint3D startPt = rebar_curve.GetCentroid();
//				RebarDetailElement *rde2 = rds->GetChildElement(num - 1);
//				RebarCurveVectorCR rebar_curve2 = rde2->GetCurveSegment();
//				CPoint3D endPt = rebar_curve2.GetCentroid();
//
//                unsigned long groupHandle = 0;
//                //if (drawCommand & TDrawCommand::TRANSIENT_BAR)
//                //{
//                //    NewComplexSegment(theBar, TSegmentType::CREATE_TRANSIENT_ELEM, groupHandle, TLocationType::DELIMITER_SEG, 20, TSegmentType::DELIMITER_PROPERTIES, drawCommand);
//                //}
//
//                //// add your 2d segments
//                //NewLineSegment(theBar, startPt, endPt, TLocationType::DELIMITER_SEG, 20, 0, groupHandle, drawCommand);
//
//                //if (drawCommand & TDrawCommand::TRANSIENT_BAR)
//                    NewComplexSegment(theBar, TSegmentType::SAVE_TRANSIENT_ELEM, groupHandle, TLocationType::DELIMITER_SEG, 20, TSegmentType::DELIMITER_PROPERTIES, drawCommand);
//            }
//        }
//    }
//
//    return true;
//}
bool SDK_RebarDetailer::IsUserDefined(BarImplementor *theBar, TDrawCommand drawCommand, int calcMode) const
{
	if (theBar != nullptr)
	{
		ElementId elemId = GetRebarDetailSetId(theBar);
		DgnModelRefP modelRef = GetModelRef(theBar);
		ElementHandle eh(elemId, modelRef);
		RebarDetailSet *rds = RebarDetailSet::Fetch(eh);
		if (rds != nullptr)
		{
			RebarShapeCP shape = nullptr;
			RebarElementCP rep = nullptr;
			RebarSetCP rset = nullptr;
			rds->GetAssociatedRebarElement(rset, rep, shape, modelRef);
			for (RebarDetailElement *rde : *rds)
			{
				RebarCurveVectorCR rebar_curve = rde->GetCurveSegment();

			}
		}
	}

	return false;
}

bool SDK_RebarDetailer::DrawUserBar(BarImplementor *theBar, TDrawCommand drawCommand, int calcMode) const
{
	if (theBar != nullptr)
	{
		ElementId elemId = GetRebarDetailSetId(theBar);
		DgnModelRefP modelRef = GetModelRef(theBar);
		ElementHandle eh(elemId, modelRef);
		RebarDetailSet *rds = RebarDetailSet::Fetch(eh);
		if (rds != nullptr)
		{
			RebarShapeCP shape = nullptr;
			RebarElementCP rep = nullptr;
			RebarSetCP rset = nullptr;
			rds->GetAssociatedRebarElement(rset, rep, shape, modelRef);
			intptr_t num = rds->GetChildElementCount();
			if (num > 1)
			{
				RebarDetailElement *rde = rds->GetChildElement(0);
				RebarCurveVectorCR rebar_curve = rde->GetCurveSegment();
				CPoint3D startPt = rebar_curve.GetCentroid();
				RebarDetailElement *rde2 = rds->GetChildElement(num - 1);
				RebarCurveVectorCR rebar_curve2 = rde2->GetCurveSegment();
				CPoint3D endPt = rebar_curve2.GetCentroid();

				unsigned long groupHandle = 0;
				if (drawCommand & TDrawCommand::TRANSIENT_BAR)
				{
					NewComplexSegment(theBar, TSegmentType::CREATE_TRANSIENT_ELEM, groupHandle, TLocationType::DELIMITER_SEG, 20, TSegmentType::DELIMITER_PROPERTIES, drawCommand);
				}

				// add your 2d segments
				//NewLineSegment(theBar, startPt, endPt, TLocationType::DELIMITER_SEG, 20, 0, groupHandle, drawCommand);

				if (drawCommand & TDrawCommand::TRANSIENT_BAR)
					NewComplexSegment(theBar, TSegmentType::SAVE_TRANSIENT_ELEM, groupHandle, TLocationType::DELIMITER_SEG, 20, TSegmentType::DELIMITER_PROPERTIES, drawCommand);
			}
		}
	}

	return true;
}