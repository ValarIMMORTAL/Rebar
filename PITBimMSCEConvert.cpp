#include "_ustation.h"
#include "Public.h"
#include "PITBimMSCEConvert.h"
#include <PSolid/PSolidCoreAPI.h>
#include "GalleryIntelligentRebarids.h"
#include "ExtractFacesTool.h"


using namespace std;
bool PIT::ConvertToElement::SubEntityToElement(EditElementHandleR eeh, ISubEntityPtr entity, DgnModelRefP modelRef)
{
	if (entity == NULL || modelRef == NULL)
	{
		return false;
	}
	CurveVectorPtr  curves;
	SolidUtil::Convert::SubEntityToCurveVector(curves, *entity);
	if (curves != NULL)
	{
		DraftingElementSchema::ToElement(eeh, *curves, nullptr, modelRef->Is3d(), *modelRef);
		if (!eeh.IsValid())
		{
			return false;
		}
	}
	else
	{
		IGeometryPtr geom = nullptr;
		SolidUtil::Convert::SubEntityToGeometry(geom, *entity, *modelRef);
		if (geom == nullptr)
		{
			return false;
		}
		IGeometry::GeometryType type = geom->GetGeometryType();
		ISolidPrimitivePtr tmpPtr = geom->GetAsISolidPrimitive();
		if (tmpPtr != NULL)
		{
			if (SUCCESS != DraftingElementSchema::ToElement(eeh, *tmpPtr, nullptr, *modelRef))
			{
				return false;
			}
		}
		else
		{
			MSBsplineSurfacePtr tmpCurve = geom->GetAsMSBsplineSurface();
			if (tmpCurve != NULL)
			{
				DraftingElementSchema::ToElement(eeh, *tmpCurve, nullptr, *modelRef);
				if (!eeh.IsValid())
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

//�Զ�����ǽ�ĸֽ���
void PIT::RebarFuction::AutoSetWallRebarCodes(vector<string>& teststring, std::map<std::string, IDandModelref>& mapidAndmodel)
{
	//�Ըֽ�����Զ����
	for (int i = 0; i < teststring.size(); i++)
	{
		string tmpname = teststring.at(i);
		ElementId eid = mapidAndmodel[tmpname].ID;
		DgnModelRefP model = mapidAndmodel[tmpname].tModel;
		if (eid != 0 && model != nullptr)
		{
			EditElementHandle eeh(eid, model);
			ElementId testid = 0;
			GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
			if (testid != 0)
			{
				vector<ElementId> vecRebars;
				//vector<ElementId> rebarsetsid;
				//GetElementXAttribute(testid, rebarsetsid, RebarSetIdXAttribute, ACTIVEMODEL);//���������д�õ�RebarSetIdȡ����
				GetElementXAttribute(testid, vecRebars, RebarIdXAttribute, ACTIVEMODEL);
				/*
				RebarAssemblies area;
				REA::GetRebarAssemblies(testid, area);
				GetVectorRebarSetsFromAssemblies(rebarsetsid, area);
				std::sort(rebarsetsid.begin(), rebarsetsid.end());
				rebarsetsid.erase(std::unique(rebarsetsid.begin(), rebarsetsid.end()), rebarsetsid.end());
				mdlSelect_freeAll();
				for (int x = 0; x < rebarsetsid.size(); x++)
				{
					RebarSetP rebarSet = RebarSet::Fetch(rebarsetsid[x], ACTIVEMODEL);
					int nNum = (int)rebarSet->GetChildElementCount(); // �ֽ����иֽ�����
					if (nNum == 0)
					{
						continue;
					}
					for (int j = 0; j < nNum; j++)
					{
						RebarElementP pRebar = rebarSet->GetChildElement(j);
						ElementId rebarElementId = pRebar->GetRebarElementId();

						EditElementHandle rebarEle(rebarElementId, ACTIVEMODEL);
						SelectionSetManager::GetManager().AddElement(rebarEle.GetElementRef(), ACTIVEMODEL);
					}
				}
				*/
				for (auto it : vecRebars)
				{
					EditElementHandle rebarEle(it, ACTIVEMODEL);
					SelectionSetManager::GetManager().AddElement(rebarEle.GetElementRef(), ACTIVEMODEL);
				}
				if (/*rebarsetsid.size() > 0 || */vecRebars.size() > 0)
				{
					AutoSetMarks();
					mdlSelect_freeAll();
					SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), model);
					mdlInput_sendSynchronizedKeyin(L"genrebarchart add rebarcode", 0, INPUTQ_HEAD, NULL);
				}
				mdlSelect_freeAll();

			}

		}
	}
}