/*--------------------------------------------------------------------------------------+
|
|     $Source: sdk/example/SlabRebarAssembly.h $
|
|  $Copyright: (c) 2018 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#pragma once
#include <RebarElements.h>
#include "PITRebarEndType.h"
#include "PITRebarCurve.h"
#include "PITArcSegment.h"
#include "PITRebarAssembly.h"

class EdgeRebarAssembly : public RebarAssembly
{
	BE_DATA_VALUE(ElementId, SetId)				//SetId
	BE_DATA_VALUE(BrString, sizeKey)			//�ֽ���
	BE_DATA_VALUE(double, spacing)								//���
public:
	
protected:
	
public:
	EdgeRebarAssembly(ElementId id = 0, DgnModelRefP modelRef = NULL);
	virtual ~EdgeRebarAssembly() {};
	virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
	BE_DECLARE_VMS(EdgeRebarAssembly, RebarAssembly)

public:
	void SetVerticesData(vector<RebarVertices> RerbarlData) { m_rebarPts.assign(RerbarlData.begin(), RerbarlData.end());}//���øֽ��߶�����
	bool SetSlabData(ElementHandleCR slab);
	bool MakeRebars(DgnModelRefP modelRef);
	bool makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, vector<RebarVertices>& pts);
private:
	vector<RebarVertices> m_rebarPts;  //�ֽ�ĵ�����
};
