/*--------------------------------------------------------------------------------------+
|
|     $Source: WallRebarAssembly.cpp $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include "resource.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "WallRebarAssembly.h"
#include "WallRebarDlg.h"
#include "ExtractFacesTool.h"
#include "CWallRebarDlg.h"
#include "TieRebar.h"
#include "ElementAttribute.h"
#include "PITMSCECommon.h"
#include "ExtractFacesTool.h"
#include "XmlHelper.h"
#include "CSolidTool.h"
#include "CFaceTool.h"
#include <unordered_set>
// #include "SelectRebarTool.h"
#include <RebarHelper.h>
// #include "HoleRebarAssembly.h"
#include <CPointTool.h>

#include "MakeRebarHelper.h"

extern bool PreviewButtonDown;//��Ҫ�������Ԥ����ť
extern map<int, vector<RebarPoint>> g_wallRebarPtsNoHole;

using namespace PIT;
WallRebarAssembly::WallRebarAssembly(ElementId id, DgnModelRefP modelRef) :
	PITRebarAssembly(id, modelRef),
	m_PositiveCover(0),
	m_ReverseCover(0),
	m_SideCover(0),
	m_RebarLevelNum(4),
	m_width(0),
	m_nowvecDir(0)
{
	Init();
}

void WallRebarAssembly::Init()
{
	m_vecDir.resize(m_RebarLevelNum);
	m_vecDirSize.resize(m_RebarLevelNum);
	m_vecDirSpacing.resize(m_RebarLevelNum);
	m_vecRebarType.resize(m_RebarLevelNum);
	m_vecStartOffset.resize(m_RebarLevelNum);
	m_vecEndOffset.resize(m_RebarLevelNum);
	m_vecLevelSpace.resize(m_RebarLevelNum);
	m_vecDataExchange.resize(m_RebarLevelNum);
	m_vecRebarLevel.resize(m_RebarLevelNum);
	m_vecAngle.resize(m_RebarLevelNum);
	// 	int twinRebarLevel = 0;
	// 
	// 	//�������󲢽���Ҫ���ò�һ���Ĳ�
	// 	for (size_t i = 0; i < GetvecTwinRebarLevel().size(); i++)
	// 	{
	// 		if (GetvecTwinRebarLevel().at(i).hasTwinbars)
	// 		{
	// 			twinRebarLevel++;
	// 		}
	// 	}
	// 	m_vecSetId.resize(m_RebarLevelNum+ twinRebarLevel);
	// 	for (size_t i = 0; i < m_vecSetId.size(); i++)
	// 	{
	// 		m_vecSetId[i] = 0;
	// 	}
// 	int twinRebarLevel = 0;
// 
// 	//�������󲢽���Ҫ���ò�һ���Ĳ�
// 	for (size_t i = 0; i < GetvecTwinRebarLevel().size(); i++)
// 	{
// 		if (GetvecTwinRebarLevel().at(i).hasTwinbars)
// 		{
// 			twinRebarLevel++;
// 		}
// 	}
// 	m_vecSetId.resize(m_RebarLevelNum+ twinRebarLevel);
// 	for (size_t i = 0; i < m_vecSetId.size(); i++)
// 	{
// 		m_vecSetId[i] = 0;
// 	}

	m_vecRebarLineStyle.resize(m_RebarLevelNum);	//�ֽ�����
	m_vecRebarWeight.resize(m_RebarLevelNum);		//�ֽ��߿�
// 	int twinRebarLevel = 0;
// 
// 	//�������󲢽���Ҫ���ò�һ���Ĳ�
// 	for (size_t i = 0; i < GetvecTwinRebarLevel().size(); i++)
// 	{
// 		if (GetvecTwinRebarLevel().at(i).hasTwinbars)
// 		{
// 			twinRebarLevel++;
// 		}
// 	}
// 	m_vecSetId.resize(m_RebarLevelNum+ twinRebarLevel);
// 	for (size_t i = 0; i < m_vecSetId.size(); i++)
// 	{
// 		m_vecSetId[i] = 0;
// 	}

}

void WallRebarAssembly::SetConcreteData(Concrete const& concreteData)
{
	m_PositiveCover = concreteData.postiveCover;
	m_ReverseCover = concreteData.reverseCover;
	m_SideCover = concreteData.sideCover;
	m_RebarLevelNum = concreteData.rebarLevelNum;
}
MSElementDescrP WallRebarAssembly::GetElementDownFace(ElementHandleCR eeh, EditElementHandleR DownFace, double* tHeight)
{
	ISolidKernelEntityPtr entityPtr;
	if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) != SUCCESS) // �ӿ��Ա�ʾ�����ߡ�Ƭ��ʵ���Ԫ�ش���ʵ��
	{
		return false;
	}
	DRange3d range;
	// Dpoint3d lowPt;
	// Dpoint3d highPt;
	if (SolidUtil::GetEntityRange(range, *entityPtr) != SUCCESS) // ��ȡ��������������߽��
	{
		return false;
	}

	if (tHeight != NULL)
		*tHeight = range.ZLength();
	// lowPt = range.low;
	// highPt = range.high;

	bvector<ISubEntityPtr> subEntities;
	if (SolidUtil::GetBodyFaces(&subEntities, *entityPtr) <= 0) // ��ѯ��������漯 
	{
		return false;
	}

	// ��С��Z
	float min_z = FLT_MAX;
	float max_z = -1000000000;
	// ��С����
	ISubEntityPtr min_face = nullptr;
	ISubEntityPtr max_face = nullptr;

	size_t iSize = subEntities.size();
	for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
	{
		ISubEntityPtr subEntity = subEntities[iIndex];

		DVec3d normal;
		DVec3d z_axis = DVec3d::UnitZ();
		if (SolidUtil::GetPlanarFaceData(nullptr, &normal, *subEntity) != SUCCESS)
		{
			continue;
		}

		/// ���㷨����Z��ĵ��
		auto dot_length = normal.DotProduct(z_axis);
		/// ���˵���Z�ᴹֱ����
		if (abs(dot_length) < 1e-6)
		{
			continue;
		}
		// ��������İ�Χ�е�z
		DRange3d range;
		if (SolidUtil::GetSubEntityRange(range, *subEntity) != SUCCESS) // ��ȡ�������ߵ������߽�� 
		{
			continue;
		}
		// ��ȡZ��С�ĵ���
		if (range.low.z < min_z)
		{
			min_z = range.low.z;
			min_face = subEntity;
		}
		if (range.high.z > max_z)
		{
			max_z = range.high.z;
			max_face = subEntity;
		}
	}

	if (min_face == nullptr)
	{
		return false;
	}
	MSElementDescrP combined_face = nullptr;
	vector<CurveVectorPtr> all_face;
	CurveVectorPtr  curves_min;
	CurveVectorPtr  curves_max;
	// ����������ʵ��ļ� CurveVector ��ʾ
	SolidUtil::Convert::SubEntityToCurveVector(curves_min, *min_face);
	SolidUtil::Convert::SubEntityToCurveVector(curves_max, *max_face);
	all_face.push_back(curves_min);
	all_face.push_back(curves_max);
	if (curves_min != NULL)
	{
		if (curves_max != NULL)
		{
			combined_face = PITCommonTool::CFaceTool::CombineFaces(all_face, eeh.GetModelRef());
		}
		else
		{
			// �ӱ�ʾ�㴮���������ߡ��պ����߻������ CurveVector ��������Ԫ�� 
			DraftingElementSchema::ToElement(DownFace, *curves_min, nullptr, eeh.GetModelRef()->Is3d(), *eeh.GetModelRef());
			combined_face = DownFace.ExtractElementDescr();
		}
		if (combined_face != nullptr)
		{
			return combined_face;
		}
	}
	else
	{
		IGeometryPtr geom_min;
		// ����������ʵ�壨�� BRep �����壩�ļ� IGeometryPtr ��ʾ 
		SolidUtil::Convert::SubEntityToGeometry(geom_min, *min_face, *eeh.GetModelRef());
		ISolidPrimitivePtr tmpPtr = geom_min->GetAsISolidPrimitive();
		if (tmpPtr != NULL)
		{

			if (SUCCESS == DraftingElementSchema::ToElement(DownFace, *tmpPtr, nullptr, *eeh.GetModelRef()))
			{
				return combined_face;
			}
		}

	}

	return false;
}

void WallRebarAssembly::SetRebarData(vector<PIT::ConcreteRebar> const& vecData)
{
	//�������桢�м䡢����ֽ������޸ĸֽ�����
	vector<PIT::ConcreteRebar> tmpvecdata;
	tmpvecdata.insert(tmpvecdata.begin(), vecData.begin(), vecData.end());
	vector<PIT::ConcreteRebar> tmpfront;
	vector<PIT::ConcreteRebar> tmpmid;
	vector<PIT::ConcreteRebar> tmpback;
	for (PIT::ConcreteRebar data : tmpvecdata)
	{
		if (data.datachange == 0)
		{
			tmpfront.push_back(data);
		}
		else if (data.datachange == 1)
		{
			tmpmid.push_back(data);
		}
		else
		{
			tmpback.push_back(data);
		}
	}
	double frontlenth = 0;
	double midlenth = 0;
	double backlenth = 0;
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	for (PIT::ConcreteRebar data : tmpfront)
	{
		double diameter = RebarCode::GetBarDiameter(data.rebarSize, ACTIVEMODEL) / uor_per_mm;
		if (data.rebarLevel == 1)
		{
			frontlenth = frontlenth + diameter;
		}
		else
		{
			frontlenth = frontlenth + diameter + data.levelSpace;
		}
	}

	for (PIT::ConcreteRebar data : tmpmid)
	{
		double diameter = RebarCode::GetBarDiameter(data.rebarSize, ACTIVEMODEL) / uor_per_mm;
		if (data.rebarLevel == 1)
		{
			midlenth = midlenth + diameter;
		}
		else
		{
			midlenth = midlenth + diameter + data.levelSpace;
		}
	}
	for (PIT::ConcreteRebar data : tmpback)
	{
		double diameter = RebarCode::GetBarDiameter(data.rebarSize, ACTIVEMODEL) / uor_per_mm;
		if (data.rebarLevel == tmpback.size())
		{
			backlenth = backlenth + diameter;
		}
		else
		{
			backlenth = backlenth + diameter + data.levelSpace;
		}
	}

	int midid = 0;
	int backid = 0;

	if (m_wallType != ELLIPSEWall)
	{
		for (PIT::ConcreteRebar& data : tmpvecdata)
		{
			if (data.datachange == 1 && midid == 0)
			{
				data.levelSpace = m_width / (2 * uor_per_mm) - midlenth / 2 - frontlenth - m_PositiveCover;
				midid = 1;
			}
			else if (data.datachange == 2 && backid == 0)
			{
				//data.levelSpace = 4000;
				if (midlenth != 0)
				{
					data.levelSpace = m_width / (2 * uor_per_mm) - midlenth / 2 - m_ReverseCover - backlenth;
				}
				else
				{
					data.levelSpace = m_width / uor_per_mm - backlenth - m_ReverseCover - frontlenth - m_PositiveCover;
				}

				backid = 1;
			}
		}
	}

	if (tmpvecdata.empty())
	{
		return;
	}
	if (tmpvecdata.size() != m_RebarLevelNum)
	{
		return;
	}

	for (size_t i = 0; i < tmpvecdata.size(); i++)
	{
		if (i < m_vecDir.size())
		{
			m_vecDir[i] = tmpvecdata[i].rebarDir;
			m_vecDirSize[i] = tmpvecdata[i].rebarSize;
			m_vecRebarType[i] = tmpvecdata[i].rebarType;
			m_vecDirSpacing[i] = tmpvecdata[i].spacing;
			m_vecStartOffset[i] = tmpvecdata[i].startOffset;
			m_vecEndOffset[i] = tmpvecdata[i].endOffset;
			m_vecLevelSpace[i] = tmpvecdata[i].levelSpace;
			m_vecDataExchange[i] = tmpvecdata[i].datachange;
			m_vecRebarLevel[i] = tmpvecdata[i].rebarLevel;
			//			GetDiameterAddType(m_vecDirSize[i], m_vecRebarType[i]);
			m_vecAngle[i] = tmpvecdata[i].angle;
			m_vecRebarLineStyle[i] = tmpvecdata[i].rebarLineStyle;
			m_vecRebarWeight[i] = tmpvecdata[i].rebarWeight;
		}
		else
		{
			m_vecDir.push_back(tmpvecdata[i].rebarDir);
			m_vecDirSize.push_back(tmpvecdata[i].rebarSize);
			m_vecRebarType.push_back(tmpvecdata[i].rebarType);
			m_vecDirSpacing.push_back(tmpvecdata[i].spacing);
			m_vecStartOffset.push_back(tmpvecdata[i].startOffset);
			m_vecEndOffset.push_back(tmpvecdata[i].endOffset);
			m_vecLevelSpace.push_back(tmpvecdata[i].levelSpace);
			m_vecDataExchange.push_back(tmpvecdata[i].datachange);
			m_vecRebarLevel.push_back(tmpvecdata[i].rebarLevel);
			m_vecAngle.push_back(tmpvecdata[i].angle);
			//			m_vecSetId.push_back(0);
			m_vecRebarLineStyle.push_back(tmpvecdata[i].rebarLineStyle);
			m_vecRebarWeight.push_back(tmpvecdata[i].rebarWeight);
		}
	}

}

void WallRebarAssembly::SetRebarEndTypes(vector<PIT::EndType> const & vecEndTypes)
{
	// 	if (vecEndTypes.empty())
	// 	{
	// 		m_vvecEndType = { { {0,0,0} }, { {0,0,0} } };
	// 		return;
	// 	}
	if (vecEndTypes.size())
		m_vvecEndType.clear();

	vector<PIT::EndType> vec;
	vec.reserve(2);
	for (size_t i = 0; i < vecEndTypes.size(); i++)
	{
		if (i & 0x01)
		{
			vec.push_back(vecEndTypes[i]);
			m_vvecEndType.push_back(vec);
			vec.clear();
		}
		else
		{
			vec.push_back(vecEndTypes[i]);
		}
	}
}

void WallRebarAssembly::InitRebarSetId()
{
	int twinRebarLevel = 0;
	//�������󲢽���Ҫ���ò�һ���Ĳ�
	for (size_t i = 0; i < m_vecTwinRebarLevel.size(); i++)
	{
		if (m_vecTwinRebarLevel[i].hasTwinbars)
		{
			twinRebarLevel++;
		}
	}

	if (m_vecSetId.size() != m_RebarLevelNum + twinRebarLevel)
	{
		m_vecSetId.resize(m_RebarLevelNum + twinRebarLevel);
		for (size_t i = 0; i < m_vecSetId.size(); ++i)
			m_vecSetId[i] = 0;
	}
}

void WallRebarAssembly::GetConcreteData(Concrete& concreteData)
{
	concreteData.postiveCover = m_PositiveCover;
	concreteData.reverseCover = m_ReverseCover;
	concreteData.sideCover = m_SideCover;
	concreteData.rebarLevelNum = m_RebarLevelNum;
	//	concreteData.isTwinbars = m_Twinbars;
}
void WallRebarAssembly::GetRebarData(vector<PIT::ConcreteRebar>& vecData) const
{
	if (0 == m_RebarLevelNum || m_vecDir.empty() || m_vecDirSize.empty() || m_vecDirSpacing.empty())
	{
		return;
	}
	for (size_t i = 0; i < m_RebarLevelNum; i++)
	{
		ConcreteRebar rebarData;
		// 		rebarData.postiveCover = m_PositiveCover;
		// 		rebarData.reverseCover = m_ReverseCover;
		// 		rebarData.reverseCover = m_SideCover;
		rebarData.rebarDir = m_vecDir[i];
		strcpy(rebarData.rebarSize, m_vecDirSize[i]);
		rebarData.rebarType = m_vecRebarType[i];
		rebarData.spacing = m_vecDirSpacing[i];
		rebarData.startOffset = m_vecStartOffset[i];
		rebarData.endOffset = m_vecEndOffset[i];
		rebarData.levelSpace = m_vecLevelSpace[i];
		rebarData.datachange = m_vecDataExchange[i];
		rebarData.angle = m_vecAngle[i];
		vecData.push_back(rebarData);
	}
}

void WallRebarAssembly::SetTieRebarInfo(TieReBarInfo const & tieRebarInfo)
{
	m_tieRebarInfo = tieRebarInfo;
}

const TieReBarInfo WallRebarAssembly::GetTieRebarInfo() const
{
	return m_tieRebarInfo;
}

void WallRebarAssembly::ClearLines()
{
	for (ElementRefP tmpeeh : m_allLines)
	{
		if (tmpeeh != nullptr)
		{
			EditElementHandle eeh(tmpeeh, tmpeeh->GetDgnModelP());
			eeh.DeleteFromModel();
		}
	}
	m_allLines.clear();
}

WallRebarAssembly::ElementType WallRebarAssembly::JudgeElementType(ElementHandleCR eh)
{
	EditElementHandle eeh(eh, eh.GetModelRef());
	string  htype;
	DgnECManagerR ecMgr = DgnECManager::GetManager();
	FindInstancesScopePtr scope = FindInstancesScope::CreateScope(eeh, FindInstancesScopeOption(DgnECHostType::Element));
	ECQueryPtr            ecQuery = ECQuery::CreateQuery(ECQUERY_PROCESS_SearchAllClasses);
	ecQuery->SetSelectProperties(true);
	for (DgnECInstancePtr instance : ecMgr.FindInstances(*scope, *ecQuery))
	{
		DgnElementECInstanceP elemInst = instance->GetAsElementInstance();
		if (elemInst->GetClass().GetDisplayLabel() == L"PDMS_Attributes")//�����PDMS_Attributes��EC���ԣ���ȡTYPEֵ
		{
			ECN::ECValue ecVal;
			elemInst->GetValue(ecVal, L"TYPE");
			char tType[1024];
			ecVal.ToString().ConvertToLocaleChars(tType);
			htype = tType;
			if (htype == "FLOOR")
			{
				return FLOOR;
			}
			else
			{
				return WALL;
			}
			break;
		}
	}
	return WALL;
}

WallRebarAssembly::WallType WallRebarAssembly::JudgeWallType(ElementHandleCR eh)
{
	ELLWallGeometryInfo m_ELLWallData;
	GetElementXAttribute(eh.GetElementId(), sizeof(m_ELLWallData), m_ELLWallData, RoundrebarGroup, ACTIVEMODEL);
	if (m_ELLWallData.type == ELLIPSEWall)
	{
		return ELLIPSEWall;
	}
	//	return STWALL;
		//ͨ���ֽ�Ԫ�����ж�Ԫ�ص�����
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;
	EditElementHandle testeeh(eh, false);
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	if (!EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs))
	{
		return Other;
	}
	for (int j = 0; j < Holeehs.size(); j++)
	{
		delete Holeehs.at(j);
		Holeehs.at(j) = nullptr;
	}

	// ��Բ����Բ�Σ�ǽ��������
	EditElementHandle eehDownFace;
	if (!ExtractFacesTool::GetDownFace(Eleeh, eehDownFace, nullptr))
	{
		return Other;
	}
	if (!eehDownFace.IsValid())
		return Other;
	if (eehDownFace.GetElementType() == ELLIPSE_ELM)
	{
		return ELLIPSEWall;
	}
	////////////////////////////////////////////////////////////

	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine, vecDownBackLine, NULL);

	if (vecDownFaceLine.empty())
		return Other;
	bool isarcwall = false;
	int arcnum = 0;
	for (size_t i = 0; i < vecDownFaceLine.size(); i++)
	{
		if (vecDownFaceLine[i] != nullptr)
		{
			if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
			{
				arcnum++;
			}
		}
	}
	if (arcnum >= 2)
	{
		isarcwall = true;
	}
	if (isarcwall == true)//ֻҪ�л��߾��ж�Ϊ����ǽ��
	{
		for (size_t i = 0; i < vecDownFaceLine.size(); i++)
		{
			mdlElmdscr_freeAll(&vecDownFaceLine[i]);
		}
		return ARCWALL;
	}
	if (vecDownFontLine.empty() || vecDownBackLine.empty())
	{
		for (size_t i = 0; i < vecDownFaceLine.size(); i++)
		{
			mdlElmdscr_freeAll(&vecDownFaceLine[i]);
		}
		return Other;
	}
	if (vecDownFaceLine.size() != 4 && vecDownFontLine.size() != 1 && vecDownBackLine.size() != 1)
	{
		for (size_t i = 0; i < vecDownFaceLine.size(); i++)
		{
			mdlElmdscr_freeAll(&vecDownFaceLine[i]);
		}
		return GWALL;
	}
	if (vecDownFaceLine.size() != 4 && vecDownFontLine.size() != vecDownBackLine.size())
	{
		for (size_t i = 0; i < vecDownFaceLine.size(); i++)
		{
			mdlElmdscr_freeAll(&vecDownFaceLine[i]);
		}
		return STGWALL;
	};


	/*int arcCount = 0;
	for (size_t i = 0; i < vecDownFaceLine.size(); i++)
	{
		if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
		{
			arcCount++;
		}
	}
	if (2 == arcCount && vecDownFaceLine.size() == 4)
	{
		return ARCWALL;
	}*/

	return STWALL;
}
bool WallRebarAssembly::IsSmartSmartFeature(EditElementHandle& eeh)
{
	SmartFeatureNodePtr pFeatureNode;
	if (SmartFeatureElement::IsSmartFeature(eeh))
	{
		return true;
	}
	else
	{
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			if (SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef()) == SUCCESS)
			{
				//eeh.AddToModel();
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}

		}
		return true;
	}
}



bool WallRebarAssembly::IsWallSolid(ElementHandleCR eh)
{
#ifdef PDMSIMPORT
	WString strName;
	WString strType;
	DgnECManagerR ecMgr = DgnECManager::GetManager();
	FindInstancesScopePtr scope = FindInstancesScope::CreateScope(eh, FindInstancesScopeOption(DgnECHostType::Element));
	ECQueryPtr            ecQuery = ECQuery::CreateQuery(ECQUERY_PROCESS_SearchAllClasses);
	//ECQUERY_PROCESS_SearchAllExtrinsic will only search ECXAttr
	ecQuery->SetSelectProperties(true);

	for (DgnECInstancePtr instance : ecMgr.FindInstances(*scope, *ecQuery))
	{
		DgnElementECInstanceP elemInst = instance->GetAsElementInstance();
		for (ECPropertyP ecProp : elemInst->GetClass().GetProperties())
		{
			WString strLabel = ecProp->GetDisplayLabel().GetWCharCP();
			if (strLabel != L"NAME" && strLabel != L"TYPE")
			{
				continue;
			}

			WString valStr;
			ECValue ecVal;
			elemInst->GetValue(ecVal, ecProp->GetName().GetWCharCP());
			IDgnECTypeAdapterR typeAdapter = IDgnECTypeAdapter::GetForProperty(*ecProp);
			IDgnECTypeAdapterContextPtr typeContext = IDgnECTypeAdapterContext::Create(*ecProp, *elemInst, ecProp->GetName().GetWCharCP());
			typeAdapter.ConvertToString(valStr, ecVal, *typeContext);

			if (strLabel == L"NAME")
				strName = valStr;
			else
				strType = valStr;
		}
	}
	if (strName.find(L"VB") != WString::npos)	//ǽ
	{
		return true;
	}
	return false;
#else
	return true;
#endif // PDMSIMPORT
}

double WallRebarAssembly::GetDownFaceVecAndThickness(MSElementDescrP downFace, DPoint3d& Vec)
{
	double thickness = 0;
	//��ȡǽ�����ǰ���ߴ��ͺ��
	vector<DSegment3d> vec_linefront;
	vector<DSegment3d> vec_lineback;
	ExtractFacesTool::GetFrontBackLinePoint(downFace, vec_linefront, vec_lineback, thickness);

	double linelenth = 0;//ȡ�������Ϊ�ֽ��߷���
	for (DSegment3d tmpseg : vec_linefront)
	{
		DPoint3d ptstr, ptend;
		tmpseg.GetStartPoint(ptstr);
		tmpseg.GetEndPoint(ptend);
		double tmpdis = ptstr.Distance(ptend);
		if (tmpdis > linelenth)
		{
			linelenth = tmpdis;
			Vec = ptend - ptstr;
			Vec.Normalize();
		}
	}
	return thickness;
}

//ͨ���㼯�ϴ����ߴ�
MSElementDescrP WallRebarAssembly::GetLines(vector<DSegment3d>& lines)
{
	vector<DPoint3d> pts;
	for (DSegment3d seg : lines)
	{
		DPoint3d str, end;
		seg.GetStartPoint(str);
		seg.GetEndPoint(end);
		if (pts.size() != 0)
		{
			if (pts[pts.size() - 1].Distance(str) < 10)
			{
				pts.push_back(end);
			}
			else
			{
				pts.push_back(str);
			}
		}
		else
		{
			pts.push_back(str);
			pts.push_back(end);
		}
	}
	EditElementHandle eeh;
	LineStringHandler::CreateLineStringElement(eeh, NULL, &pts[0], pts.size(), true, *ACTIVEMODEL);
	return eeh.ExtractElementDescr();
}

//�����������ӳ��ߴ�����ֹ�պõ������Ͻ������������
void WallRebarAssembly::ExtendLineString(MSElementDescrP& linedescr, double dis)
{
	vector<DPoint3d> dpts;
	if (linedescr == nullptr) return;
	ExtractLineStringPoint(linedescr, dpts);
	if (dpts.size() > 1)
	{
		Dpoint3d vectmp = dpts[1] - dpts[0];
		vectmp.Normalize();
		vectmp.Negate();
		vectmp.Scale(dis);
		dpts[0].Add(vectmp);
		int tsize = dpts.size();
		vectmp = dpts[tsize - 1] - dpts[tsize - 2];
		vectmp.Normalize();
		vectmp.Scale(dis);
		dpts[tsize - 1].Add(vectmp);

		//�ع��ߴ�
		EditElementHandle eeh;
		LineStringHandler::CreateLineStringElement(eeh, nullptr, &dpts[0], dpts.size(), true, *ACTIVEMODEL);
		mdlElmdscr_freeAll(&linedescr);
		linedescr = eeh.ExtractElementDescr();
	}



}


void WallRebarAssembly::GetMovePath(MSElementDescrP& pathline, double movedis, MSElementDescrP downface)
{

	MSElementDescrP tmpdescr = nullptr;
	mdlElmdscr_duplicate(&tmpdescr, downface);



	//1��������Ԫ��ת����XOYƽ��
	DVec3d vecZ = DVec3d::UnitZ();
	DPoint3d zeropt = DPoint3d::From(0, 0, 0);
	DPoint3d tmpvecZ = DPoint3d::From(0, 0, 1);
	Transform trans;			//����ͶӰ����
	mdlTMatrix_computeFlattenTransform(&trans, &zeropt, &tmpvecZ);
	mdlElmdscr_transform(&pathline, &trans);
	mdlElmdscr_transform(&tmpdescr, &trans);
	//2��ƫ���ߴ�
	vector<DPoint3d> dpts;
	ExtractLineStringPoint(pathline, dpts);
	DPoint3d movevec;
	if (dpts.size() > 1)
	{
		movevec = dpts[1] - dpts[0];
		movevec.Normalize();
		movevec.CrossProduct(movevec, vecZ);
	}
	else
	{
		return;
	}
	//3�������ߴ������Ľ��㡣
	//mdlElmdscr_add(pathline);
	MSElementDescrP pathlinecpy = nullptr;
	movedis = -movedis;
	mdlElmdscr_copyParallel(&pathlinecpy, pathline, &movevec, movedis, &vecZ);
	//mdlElmdscr_add(pathlinecpy);
	DPoint3d arcinter[2];
	//mdlElmdscr_add(tmpdescr);
	int isConti = mdlIntersect_allBetweenElms(arcinter, nullptr, 2, pathlinecpy, tmpdescr, nullptr, 0.1);
	// START���� #58154?�����������������������������������������е��������õ�һ������ֻ��һ�����㣬������һ������
	if (isConti != 2)
		isConti = mdlIntersect_allBetweenExtendedElms(arcinter, nullptr, 2, pathlinecpy, tmpdescr, nullptr, 0.1, nullptr, nullptr);
	if (isConti != 2)//���û�н��㣬�����ƶ���
	{
		movedis = -movedis;
		mdlElmdscr_copyParallel(&pathlinecpy, pathline, &movevec, movedis, &vecZ);
		//mdlElmdscr_add(pathlinecpy);
		int tmpnum = mdlIntersect_allBetweenElms(arcinter, nullptr, 2, pathlinecpy, tmpdescr, nullptr, 0.1);
		if (tmpnum != 2)
			mdlIntersect_allBetweenExtendedElms(arcinter, nullptr, 2, pathlinecpy, tmpdescr, nullptr, 0.1, nullptr, nullptr);
		// END���� #58154?�������������������������������������������е��������õ�һ������ֻ��һ�����㣬������һ������
		DVec3d vecch = arcinter[1] - arcinter[0];
		vecch.Normalize();
		DVec3d vecf = dpts[dpts.size() - 1] - dpts[0];
		vecf.Normalize();
		if (abs(vecch.DotProduct(vecf)) < 0.98)
		{
			movedis = -movedis;
			mdlElmdscr_copyParallel(&pathlinecpy, pathline, &movevec, movedis, &vecZ);
			ExtendLineString(pathlinecpy, movedis * 50);
			int num = mdlIntersect_allBetweenElms(arcinter, nullptr, 2, pathlinecpy, tmpdescr, nullptr, 0.01);
			if (num < 2)
				mdlIntersect_allBetweenExtendedElms(arcinter, nullptr, 2, pathlinecpy, tmpdescr, nullptr, 0.01, nullptr, nullptr);
		}
	}
	else
	{
		//��Ҫ�ж���б���Ƿ����󽻵�֮ǰ��һ��
		DVec3d vecch = arcinter[1] - arcinter[0];
		vecch.Normalize();
		DVec3d vecf = dpts[dpts.size() - 1] - dpts[0];
		vecf.Normalize();
		if (abs(vecch.DotProduct(vecf)) < 0.98)
		{
			movedis = -movedis;
			mdlElmdscr_copyParallel(&pathlinecpy, pathline, &movevec, movedis, &vecZ);
			ExtendLineString(pathlinecpy, movedis * 50);
			int num = mdlIntersect_allBetweenElms(arcinter, nullptr, 2, pathlinecpy, tmpdescr, nullptr, 0.01);
			if (num < 2)
				mdlIntersect_allBetweenExtendedElms(arcinter, nullptr, 2, pathlinecpy, tmpdescr, nullptr, 0.01, nullptr, nullptr);
		}
	}

	dpts.clear();
	ExtractLineStringPoint(pathlinecpy, dpts);
	//PITCommonTool::CPointTool::DrowOneLine(DSegment3d::From(dpts[0], dpts[1]), 2);//��
	//4����ȡ�ߴ������е㣬����ʼ�����ֹ���滻Ϊ����Ľ���
	if (dpts[0].Distance(arcinter[0]) < dpts[0].Distance(arcinter[1]) && arcinter[0].Distance(arcinter[1]) > 10)
	{
		dpts[0] = arcinter[0];
		dpts[dpts.size() - 1] = arcinter[1];
	}
	else if (arcinter[0].Distance(arcinter[1]) > 10)
	{
		dpts[0] = arcinter[1];
		dpts[dpts.size() - 1] = arcinter[0];
	}
	//PITCommonTool::CPointTool::DrowOneLine(DSegment3d::From(dpts[0], dpts[1]), 5);//��
	//4.1��XOYƽ���ת����ԭʼб����
	DPoint3d facenormal;
	DPoint3d maxpt;
	mdlElmdscr_extractNormal(&facenormal, &maxpt, downface, NULL);
	DPoint3d cpt = getCenterOfElmdescr(downface);
	facenormal.Normalize();
	DPlane3d plane = DPlane3d::FromOriginAndNormal(cpt, DVec3d::From(facenormal.x, facenormal.y, facenormal.z));

	for (int i = 0; i < dpts.size(); i++)
	{
		DPoint3d movept = dpts[i];
		movept.z = cpt.z - 1000;
		DRay3d  dray;
		DVec3d moveFace = DVec3d::From(0, 0, 2000);
		dray.InitFromOriginAndVector(movept, moveFace);
		double hvalue = 0;
		if (dray.Intersect(movept, hvalue, plane))
		{
			dpts[i] = movept;
		}
	}
	//PITCommonTool::CPointTool::DrowOneLine(DSegment3d::From(dpts[0], dpts[1]), 3);//��
	//5�������µ��ߴ������»����ߴ�
	EditElementHandle eeh;
	LineStringHandler::CreateLineStringElement(eeh, nullptr, &dpts[0], dpts.size(), true, *ACTIVEMODEL);
	mdlElmdscr_freeAll(&pathline);
	mdlElmdscr_freeAll(&pathlinecpy);
	pathline = eeh.ExtractElementDescr();
	//6������ת����ԭ����λ��
	//mdlElmdscr_add(pathline);
}

/*
 * @desc:       ����·�����ɾ��ȷֲ��ĵ�
 * @param[in]    ptStr ·�����
 * @param[in]    ptEnd ·���յ�
 * @param[in]    numPoints ���ɵĵ�����
 * @param[out]   points ���ɵ�·�����б�
 */
void WallRebarAssembly::GeneratePathPoints(const DPoint3d& ptStr, const DPoint3d& ptEnd, int numPoints, vector<DPoint3d>& points)
{
	points.clear();
	if (numPoints <= 0) return;

	// ����·������
	DVec3d pathVec = DVec3d::FromZero();
	pathVec.DifferenceOf(ptEnd, ptStr);
	pathVec.Scale(1.0 / (numPoints - 1));
	DPoint3d point = ptStr;

	// ���ȷֲ���
	for (int i = 0; i < numPoints; i++)
	{
		points.push_back(point);
		point.SumOf(point, pathVec);
	}
}

/*
 * @desc:       ͳ��·�����ڰ�Ԫ���ڵ�������ͶӰ�� XY ƽ����жϣ�
 * @param[in]    eeh ��Ԫ��
 * @param[in]    points ·�����б�
 * @return       �ڰ�Ԫ���ڵĵ�����
 */
int WallRebarAssembly::CountPointsInElement(EditElementHandleP eeh, const vector<DPoint3d>& points)
{
	if (!eeh->IsValid() || points.empty()) return 0;

	// ͳ����Ԫ���ڵĵ�����
	int pointsInElement = 0;
	for (const auto& point : points)
	{
		DPoint3d projPoint = point;
		projPoint.z = 0; // ͶӰ�� XY ƽ��
		if (ISPointInElement(eeh, projPoint))
		{
			pointsInElement++;
		}
	}

	return pointsInElement;
}

//�ӳ�����ͨ�����°�
void WallRebarAssembly::ExtendLineByFloor(vector<MSElementDescrP>& floorfaces, vector<IDandModelref>& floorRf, DPoint3d& ptstr, DPoint3d& ptend, DPoint3d& vecLine, double thick, double& Dimr, DPoint3d vecOutwall)
{
	DPoint3d zeropt = DPoint3d::From(0, 0, 0);
	DPoint3d tmpvecZ = DPoint3d::From(0, 0, 1);
	Transform tran;			//����ͶӰ����
	mdlTMatrix_computeFlattenTransform(&tran, &zeropt, &tmpvecZ);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double eps = uor_per_mm;
	bool ishaveupfloor = false;
	bool isMedial = false;//�Ƿ����ڲ�

	DPoint3d ptPathStr = ptstr;//������յ��жϸ�Ϊ�е��ж�
	ptPathStr.Add(ptend);
	ptPathStr.Scale(0.5);
	DPoint3d ptPathEnd = ptPathStr;
	ptPathEnd.Add(vecLine);
	ptPathEnd.Add(vecLine);
	ptPathEnd.Add(vecLine);
	// ����·����
	int numPoints = 20;
	int minPointsInRange = 10;
	vector<DPoint3d> pathPoints;
	GeneratePathPoints(ptPathStr, ptPathEnd, numPoints, pathPoints);

	for (int i = 0; i < floorfaces.size(); i++)
	{
		DRange3d range;
		//����ָ��Ԫ����������Ԫ�صķ�Χ��
		mdlElmdscr_computeRange(&range.low, &range.high, floorfaces.at(i), NULL);
		range.low.x = range.low.x - eps;
		range.low.y = range.low.y - eps;
		range.high.x = range.high.x + eps;
		range.high.y = range.high.y + eps;
		
		// ͳ���ڰ巶Χ�ڵĵ�����
		int pointsInRange = 0;
		for (const auto& point : pathPoints)
		{
			if (range.IsContainedXY(point))
			{
				pointsInRange++;
			}
		}

		// ����㹻��ĵ��ڷ�Χ�ڣ���Ϊ·����������
		if (pointsInRange > minPointsInRange)
		{
			ElementId elIDdata = 0;
			GetElementXAttribute(floorRf.at(i).ID, sizeof(ElementId), elIDdata, ConcreteIDXAttribute, floorRf.at(i).tModel);
			std::vector<PIT::ConcreteRebar>                        test_vecRebarData1;
			std::vector<PIT::ConcreteRebar>                        test_vecRebarData2;
			GetElementXAttribute(elIDdata, test_vecRebarData1, RebarInsideFace, ACTIVEMODEL);
			GetElementXAttribute(elIDdata, test_vecRebarData2, RebarOutsideFace, ACTIVEMODEL);

			ishaveupfloor = true;
			ElementId concreteid = 0;
			vector<PIT::ConcreteRebar> vecRebar;
			int ret = GetElementXAttribute(floorRf.at(i).ID, sizeof(ElementId), concreteid, ConcreteIDXAttribute, floorRf.at(i).tModel);
			if (ret == SUCCESS)
			{
				GetElementXAttribute(concreteid, vecRebar, vecRebarDataXAttribute, ACTIVEMODEL);
			}
			else
			{
				GetElementXAttribute(floorRf.at(i).ID, vecRebar, vecRebarDataXAttribute, ACTIVEMODEL);
			}
			if (test_vecRebarData1.size() != 0 && test_vecRebarData1.size() != 0 && vecRebar.size() == 2)
			{
				vecRebar.clear();
				vecRebar.push_back(test_vecRebarData1[0]);
				vecRebar.push_back(test_vecRebarData1[1]);
				vecRebar.push_back(test_vecRebarData2[0]);
				vecRebar.push_back(test_vecRebarData2[1]);
			}	
			CVector3D wallVec = vecOutwall;
			wallVec.Perpendicular(CVector3D::kZaxis);
			wallVec.Normalize();
			bool WallDir;
			if (!wallVec.IsEqual(Dpoint3d::FromZero()))
			{
				if (COMPARE_VALUES_EPS(abs(wallVec.x), 1, 0.1))
					WallDir = false;
				if (COMPARE_VALUES_EPS(abs(wallVec.y), 1, 0.1))
					WallDir = true;
			}
			if (!vecRebar.empty() && vecRebar.at(0).rebarDir == WallDir)//�����������øֽ����ݲ�Ϊ�ղ���L1���ȵ�������ͬ
			{
				auto it = vecRebar.begin();
				BrString strRebarSize = it->rebarSize;
				if (strRebarSize != L"")
				{
					if (strRebarSize.Find(L"mm") != -1)
					{
						strRebarSize.Replace(L"mm", L"");
					}
				}
				else
				{
					strRebarSize = XmlManager::s_alltypes[it->rebarType];
				}
				strcpy(it->rebarSize, CT2A(strRebarSize));
				GetDiameterAddType(it->rebarSize, it->rebarType);
				Dimr = RebarCode::GetBarDiameter(it->rebarSize, ACTIVEMODEL);		//������10
			}
			break;
		}
	}
	if (ishaveupfloor)//������϶�����п���Ҫ���������
	{
		// �ƶ�·���㣨��ǽ����෽���ƶ����ǽ��
		vector<DPoint3d> movedPoints;
		vecOutwall.Scale(thick * uor_per_mm / 2);
		for (const auto& point : pathPoints)
		{
			DPoint3d movedPoint = point;
			movedPoint.Add(vecOutwall);
			movedPoints.push_back(movedPoint);
		}

		// ͶӰԭʼ·���㵽 XY ƽ��
		vector<DPoint3d> projPoints;
		for (const auto& point : pathPoints)
		{
			DPoint3d projPoint = point;
			projPoint.z = 0;
			projPoints.push_back(projPoint);
		}

		//PITCommonTool::CPointTool::DrowOnePoint(movept, 1, 4);//��
		for (int i = 0; i < floorfaces.size(); i++)
		{
			DRange3d range;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			mdlElmdscr_computeRange(&range.low, &range.high, floorfaces.at(i), NULL);
			// EditElementHandle shapeEeh(floorfaces.at(i), false, false, ACTIVEMODEL); //��������Ƿ���棬����ʱ����isClosed������Ϊtrue���Ƿ����
			// CurveVectorPtr curvePtr = ICurvePathQuery::ElementToCurveVector(shapeEeh); //ת����
			//CurveVector::InOutClassification pos1 = curvePtr->PointInOnOutXY(movedPoints[0]); //�����Ĺ�ϵ
			range.low.x = range.low.x - eps;
			range.low.y = range.low.y - eps;
			range.high.x = range.high.x + eps;
			range.high.y = range.high.y + eps;

			// ͳ���ƶ���ĵ��ڷ�Χ�ڵ�����
			int pointsInRangeMoved = 0;
			for (const auto& point : movedPoints)
			{
				if (range.IsContainedXY(point))
				{
					pointsInRangeMoved++;
				}
			}
			// ����㹻����ƶ����ڷ�Χ�ڣ���һ���ж�Ԫ�ذ���
			if (pointsInRangeMoved > minPointsInRange)
			{
				//����жϵ���range�ڣ����жϴ��������Ƿ��н���(����ͶӰ��XOYƽ�棬���ж�)
				MSElementDescrP cdescr = nullptr;
				mdlElmdscr_duplicate(&cdescr, floorfaces.at(i));
				//����ͶӰ��XOYƽ��
				mdlElmdscr_transform(&cdescr, &tran);
				EditElementHandle teeh(cdescr, true, false, ACTIVEMODEL);

				// ͳ���ƶ���ĵ��ԭʼ����ͶӰԪ���ڵ�����
				int movedPointsInElement = CountPointsInElement(&teeh, movedPoints);
				int projPointsInElement = CountPointsInElement(&teeh, projPoints);

				// �������㶼��Ԫ���ڣ���Ϊ���ڲ�
				if (movedPointsInElement > minPointsInRange && projPointsInElement > minPointsInRange)
				{
					isMedial = true;
					break;
				}
			}
		}
		if (!isMedial)//�����
		{
			Dpoint3d vectmp = ptstr - ptend;
			vectmp.Normalize();
			vectmp.Scale(thick*uor_per_mm);
			ptstr.Add(vectmp);
		}

	}
}

//���㵱ǰ��ʼ���Ƿ��а壬����������ڲ໹�������
bool WallRebarAssembly::CalculateBarLineDataByFloor(vector<MSElementDescrP>& floorfaces, vector<IDandModelref>& floorRf, DPoint3d& ptstr, DPoint3d& ptend, DPoint3d& vecLine, double thick,
	DPoint3d vecOutwall, bool& isInside, double& diameter)
{
	DPoint3d zeropt = DPoint3d::From(0, 0, 0);
	DPoint3d tmpvecZ = DPoint3d::From(0, 0, 1);
	Transform tran;			//����ͶӰ����
	mdlTMatrix_computeFlattenTransform(&tran, &zeropt, &tmpvecZ);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double eps = uor_per_mm;
	bool ishaveupfloor = false;//�Ƿ��а�
	bool isMedial = false;
	vector<PIT::ConcreteRebar> vecRebar;
	std::vector<PIT::ConcreteRebar> rebarInsideFace;
	std::vector<PIT::ConcreteRebar> rebarOutsideFace;

	// ����·��������е��յ��жϣ���һ���ڰ巶Χ�ڶ������ڲ࣬vecLineֻ��·����1/3����
	DPoint3d ptPathStr = ptstr;
	ptPathStr.Add(ptend);
	ptPathStr.Scale(0.5);
	DPoint3d ptPathEnd = ptPathStr;
	ptPathEnd.Add(vecLine);
	ptPathEnd.Add(vecLine);
	ptPathEnd.Add(vecLine);
	// ����·����
	int numPoints = 20;
	int minPointsInRange = 10;
	vector<DPoint3d> pathPoints;
	GeneratePathPoints(ptPathStr, ptPathEnd, numPoints, pathPoints);

	for (int i = 0; i < floorfaces.size(); i++)
	{
		DRange3d range;
		//����ָ��Ԫ����������Ԫ�صķ�Χ��
		mdlElmdscr_computeRange(&range.low, &range.high, floorfaces.at(i), NULL);
		range.low.x = range.low.x - eps;
		range.low.y = range.low.y - eps;
		range.high.x = range.high.x + eps;
		range.high.y = range.high.y + eps;

		// ͳ���ڰ巶Χ�ڵĵ�����
		int pointsInRange = 0;
		for (const auto& point : pathPoints)
		{
			if (range.IsContainedXY(point))
			{
				pointsInRange++;
			}
		}

		// ����㹻��ĵ��ڷ�Χ�ڣ���Ϊ·����������
		if (pointsInRange > minPointsInRange)
		{
			ElementId elIDdata = 0;
			GetElementXAttribute(floorRf.at(i).ID, sizeof(ElementId), elIDdata, ConcreteIDXAttribute, floorRf.at(i).tModel);
			GetElementXAttribute(elIDdata, rebarInsideFace, RebarInsideFace, ACTIVEMODEL);
			GetElementXAttribute(elIDdata, rebarOutsideFace, RebarOutsideFace, ACTIVEMODEL);

			ishaveupfloor = true;
			ElementId concreteid = 0;
			int ret = GetElementXAttribute(floorRf.at(i).ID, sizeof(ElementId), concreteid, ConcreteIDXAttribute, floorRf.at(i).tModel);
			if (ret == SUCCESS)
			{
				GetElementXAttribute(concreteid, vecRebar, vecRebarDataXAttribute, ACTIVEMODEL);
			}
			else
			{
				GetElementXAttribute(floorRf.at(i).ID, vecRebar, vecRebarDataXAttribute, ACTIVEMODEL);
			}
			if (rebarInsideFace.size() != 0 && rebarOutsideFace.size() != 0 && vecRebar.size() == 2)
			{
				vecRebar.clear();
				vecRebar.push_back(rebarInsideFace[0]);
				vecRebar.push_back(rebarInsideFace[1]);
				vecRebar.push_back(rebarOutsideFace[0]);
				vecRebar.push_back(rebarOutsideFace[1]);
			}
			break;
		}
	}
	if (ishaveupfloor)//����а���п���Ҫ��������
	{
		// �ƶ�·���㣨��ǽ����෽���ƶ����ǽ��
		vector<DPoint3d> movedPoints;
		vecOutwall.Scale(thick * uor_per_mm / 2);
		for (const auto& point : pathPoints)
		{
			DPoint3d movedPoint = point;
			movedPoint.Add(vecOutwall);
			movedPoint.z = 0; // ͶӰ�� XY ƽ��
			movedPoints.push_back(movedPoint);
		}

		// ͶӰԭʼ·���㵽 XY ƽ��
		vector<DPoint3d> projPoints;
		for (const auto& point : pathPoints)
		{
			DPoint3d projPoint = point;
			projPoint.z = 0;
			projPoints.push_back(projPoint);
		}

		for (int i = 0; i < floorfaces.size(); i++)
		{
			DRange3d range;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			mdlElmdscr_computeRange(&range.low, &range.high, floorfaces.at(i), NULL);
			range.low.x = range.low.x - eps;
			range.low.y = range.low.y - eps;
			range.high.x = range.high.x + eps;
			range.high.y = range.high.y + eps;

			// ͳ���ƶ���ĵ��ڷ�Χ�ڵ�����
            int pointsInRangeMoved = 0;
            for (const auto& point : movedPoints)
            {
                if (range.IsContainedXY(point))
                {
                    pointsInRangeMoved++;
                }
            }

            // ����㹻����ƶ����ڷ�Χ�ڣ���һ���ж�Ԫ�ذ���
            if (pointsInRangeMoved > minPointsInRange)
			{
				//����жϵ���range�ڣ����жϴ��������Ƿ��н���(����ͶӰ��XOYƽ�棬���ж�)
				MSElementDescrP cdescr = nullptr;
				mdlElmdscr_duplicate(&cdescr, floorfaces.at(i));
				//����ͶӰ��XOYƽ��
				mdlElmdscr_transform(&cdescr, &tran);
				EditElementHandle teeh(cdescr, true, false, ACTIVEMODEL);

				// ͳ���ƶ���ĵ��ԭʼ����ͶӰԪ���ڵ�����
				int movedPointsInElement = CountPointsInElement(&teeh, movedPoints);
				int projPointsInElement = CountPointsInElement(&teeh, projPoints);

				// �������㶼��Ԫ���ڣ���Ϊ���ڲ�
				if (movedPointsInElement > minPointsInRange && projPointsInElement > minPointsInRange)
				{
					isMedial = true;
					break;
				}
			}
		}
		if (!isMedial)//�����
		{
			isInside = false;
			CVector3D wallVec = vecOutwall;
			wallVec.Perpendicular(CVector3D::kZaxis);
			wallVec.Normalize();
			bool WallDir;
			if (!wallVec.IsEqual(Dpoint3d::FromZero()))
			{
				if (COMPARE_VALUES_EPS(abs(wallVec.x), 1, 0.1))
					WallDir = false;
				if (COMPARE_VALUES_EPS(abs(wallVec.y), 1, 0.1))
					WallDir = true;
			}
			//�����������øֽ����ݲ�Ϊ�ղ��Ұ�L1�ֽ�����ȵ�������ͬ��Ҫƫ��һ��L1�ֽ�ֱ��
			if (!vecRebar.empty() && vecRebar.at(0).rebarDir == WallDir && rebarInsideFace.empty())
			{
			
				auto it = vecRebar.begin();
				BrString strRebarSize = it->rebarSize;
				if (strRebarSize != L"")
				{
					if (strRebarSize.Find(L"mm") != -1)
					{
						strRebarSize.Replace(L"mm", L"");
					}
				}
				else
				{
					strRebarSize = XmlManager::s_alltypes[it->rebarType];
				}
				strcpy(it->rebarSize, CT2A(strRebarSize));
				GetDiameterAddType(it->rebarSize, it->rebarType);
			
				diameter = RebarCode::GetBarDiameter(it->rebarSize, ACTIVEMODEL);		//������10
			}
		}
		else
		{
			isInside = true;
			if (!vecRebar.empty())//�ڲ���������ݴ�����Ѿ���ã��Ǿ���ƫ��ÿ��ֽ�ֱ��֮��
			{

				int i = 0;
				for (auto it : vecRebar)
				{
					BrString strRebarSize = it.rebarSize;
					if (strRebarSize != L"")
					{
						if (strRebarSize.Find(L"mm") != -1)
						{
							strRebarSize.Replace(L"mm", L"");
						}
					}
					else
					{
						strRebarSize = XmlManager::s_alltypes[it.rebarType];
					}
					strcpy(it.rebarSize, CT2A(strRebarSize));
					GetDiameterAddType(it.rebarSize, it.rebarType);
					if (i + 1 > vecRebar.size() / 2)
						continue;
					diameter += RebarCode::GetBarDiameter(it.rebarSize, ACTIVEMODEL);		//������10
					i++;
				}


			}
		}

	}
	return ishaveupfloor;
}

//���ݿ��ڵ�ǰǽ���ϵ�����ǽ�壬�����������·��,���ݶ��·���ٹ������BarLinesdata���������
void WallRebarAssembly::GetCutPathLines(vector<WallRebarAssembly::BarLinesdata>& barlines, double sidespacing, double diameter,
	MSElementDescrP& path, vector<MSElementDescrP>& cutWallfaces, MSElementDescrP downface, double height)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	DPoint3d zeropt = DPoint3d::From(0, 0, 0);
	DPoint3d tmpvecZ = DPoint3d::From(0, 0, 1);
	Transform tran;			//����ͶӰ����
	mdlTMatrix_computeFlattenTransform(&tran, &zeropt, &tmpvecZ);
	//�Ƚ���ͶӰ��XOYƽ��
	MSElementDescrP copydescr = nullptr;
	mdlElmdscr_duplicate(&copydescr, path);
	mdlElmdscr_transform(&copydescr, &tran);

	DPoint3d ptstrOld, ptendOld;
	mdlElmdscr_extractEndPoints(&ptstrOld, nullptr, &ptendOld, nullptr, copydescr, ACTIVEMODEL);
	DPoint3d vecline = ptendOld - ptstrOld;
	vecline.Normalize();

	DPoint3d downcpt = getCenterOfElmdescr(downface);
	downcpt.z = 0;
	DPoint3d ptProjectdown;	//ͶӰ��
	mdlVec_projectPointToLine(&ptProjectdown, NULL, &downcpt, &ptstrOld, &ptendOld);
	DPoint3d Vecprj = downcpt - ptProjectdown;
	Vecprj.Normalize();

	DPoint3d ptstr = ptstrOld;
	DPoint3d ptend = ptendOld;

	map<int, DPoint3d>  pathpts;
	pathpts[0] = ptstr;
	int tmpdis = ptstr.Distance(ptend);
	pathpts[tmpdis] = ptend;

	//���ߴ���������������������󽻵�
	DPoint3d vecL = ptstr - ptend;//��ʼ�ߴ�������
	vecL.Normalize();
	vecL.Scale(10);//�ӳ�
	ptstr.Add(vecL);
	vecL.Normalize();
	vecL.Scale(-1);//�����ӳ�
	ptend.Add(vecL);
	EditElementHandle nlineeeh;
	LineHandler::CreateLineElement(nlineeeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
	//nlineeeh.AddToModel();
	map<int, vector<DPoint3d>> cutwallpoints;//�����е���ǽ�����ߵ������յ㼯��
	int k = 0;
	for (int i = 0; i < cutWallfaces.size(); i++)
	{
		MSElementDescrP cdescr = nullptr;
		mdlElmdscr_duplicate(&cdescr, cutWallfaces.at(i));
		//����ͶӰ��XOYƽ��
 		mdlElmdscr_transform(&cdescr, &tran);
		DPoint3d vecwall = vecline;
		GetDownFaceVecAndThickness(cdescr, vecwall);
		double dotvalue = vecwall.DotProduct(vecline);
		if (abs(dotvalue) < 0.01)//����ֱʱ�Ž�;;ƽ��ʱ|| abs(dotvalue) > 0.98
		{
			//����ǽ�������ĵ㣬��ǽ�������ĵ�ͶӰ��XOY���ߴ��ϣ��õ�ͶӰ�㣬Ȼ��ǽ��������ĵ��ƶ���ͶӰ��λ�ã������ߴ������Ľ���			
			DPoint3d cpt = getCenterOfElmdescr(cdescr);
			DPoint3d ptProject1;	//ͶӰ��
			mdlVec_projectPointToLine(&ptProject1, NULL, &cpt, &ptstr, &ptend);
			DVec3d vecmove = ptProject1 - cpt;
			ExtractFacesTool::MoveFacets(cdescr, vecmove);
			//mdlElmdscr_add(cdescr);
			vecmove.Normalize();
			vecmove.Scale(-1);
			dotvalue = vecmove.DotProduct(Vecprj);
			if (dotvalue < -0.95)//��Ҫ���ĵ���ͶӰ��ķ����Լ�ǽ�����ĵ���ͶӰ��ķ�����ʱ���ж�����Ϊ��ǰ��Ĵ�ֱ��
			{
				//��������Ľ���
				DPoint3d arcinter[2];
				if (mdlIntersect_allBetweenElms(arcinter, nullptr, 2, nlineeeh.GetElementDescrP(), cdescr, nullptr, 0.1) == 2)//���û�н��㣬�����ƶ���
				{
					int tmpdis = arcinter[0].Distance(ptstrOld);
					if (tmpdis > 100)
						pathpts[tmpdis] = arcinter[0];

					tmpdis = arcinter[1].Distance(ptstrOld);
					if (tmpdis > 100)
						pathpts[tmpdis] = arcinter[1];
					cutwallpoints[k].push_back(arcinter[0]);
					cutwallpoints[k].push_back(arcinter[1]);
					k++;
				}
				//mdlElmdscr_add(cdescr);
			}
		}
	}
	DPoint3d facenormal;
	DPoint3d maxpt;
	mdlElmdscr_extractNormal(&facenormal, &maxpt, downface, NULL);
	DPoint3d cpt = getCenterOfElmdescr(downface);
	facenormal.Normalize();
	DPlane3d plane = DPlane3d::FromOriginAndNormal(cpt, DVec3d::From(facenormal.x, facenormal.y, facenormal.z));

	map<int, DPoint3d>  pathpts_data;
	pathpts_data[0] = pathpts.begin()->second;
	pathpts_data[1] = pathpts.rbegin()->second;
	map<int, DPoint3d>::iterator itr = pathpts_data.begin();
	map<int, DPoint3d>::iterator itrnext = pathpts_data.begin();

	itrnext++;
	for (; itrnext != pathpts_data.end();)
	{
		DPoint3d ptstr = itr->second;
		DPoint3d ptend = itrnext->second;
		DPoint3d midpt = ptstr;
		midpt.Add(ptend);
		midpt.Scale(0.5);
		bool isInWall = false;//�ж����ĵ��Ƿ����е�ǽ���ϣ���������������߶Σ�����ֱ�ӻ���
		for (int m = 0; m < k; m++)
		{
			bool istr = false;
			isInWall = EFT::IsPointInLine(midpt, cutwallpoints[m][0], cutwallpoints[m][1], ACTIVEMODEL, istr);
			if (isInWall) break;
		}
		if (!isInWall)
		{
			//���str����·���ߵ��������յ㣬��Ҫ�ӳ� = 2*���汣���� + �ֽ�ֱ��
			double exDis = 2 * sidespacing + diameter / 2;
			//���ߴ���������������������󽻵�
			DPoint3d vecL = ptstr - ptend;//��ʼ�ߴ�������
			vecL.Normalize();
			vecL.Scale(exDis);//�ӳ�
			if (ptstr.Distance(ptstrOld) > 100 && ptstr.Distance(ptendOld) > 100)
			{
				ptstr.Add(vecL);
			}
			if (ptend.Distance(ptstrOld) > 100 && ptend.Distance(ptendOld) > 100)
			{
				vecL.Scale(-1);
				ptend.Add(vecL);
			}
			//��XOYƽ���ת���������ϣ����»��Ƹֽ��ߴ�
			//4.1��XOYƽ���ת����ԭʼб����
			DPoint3d movept = ptstr;
			movept.z = cpt.z - 1000;
			DRay3d  dray;
			DVec3d moveFace = DVec3d::From(0, 0, 2000);
			dray.InitFromOriginAndVector(movept, moveFace);
			double hvalue = 0;
			if (dray.Intersect(movept, hvalue, plane))
			{
				ptstr = movept;
			}
			movept = ptend;
			movept.z = cpt.z - 1000;
			dray.InitFromOriginAndVector(movept, moveFace);
			if (dray.Intersect(movept, hvalue, plane))
			{
				ptend = movept;
			}

			//����·������ֽ���
			WallRebarAssembly::BarLinesdata tmpdata;
			EditElementHandle nlineeeh;
			//test
			LineHandler::CreateLineElement(nlineeeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			tmpdata.path = nlineeeh.ExtractElementDescr();
			//�����ֽ���
			ptend = ptstr;
			ptend.z = ptend.z + height * uor_per_mm;
			LineHandler::CreateLineElement(nlineeeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			tmpdata.barline = nlineeeh.ExtractElementDescr();
			barlines.push_back(tmpdata);
		}
		itr++;
		itrnext++;
        }

}



//�������ӳ��ߣ�������Χ���ڵ�ǰǽ��������ӳ��ߵĽ��㣬
//ͬʱ�������ǰ�ֽ��ߵ�ê�뷽���ê�̳���
double  WallRebarAssembly::GetExtendptByWalls(DPoint3d& str, DPoint3d str_1, double thick, MSElementDescrP& Wallfaces, vector<MSElementDescrP>& cutWallfaces,
	DPlane3d plane, DPoint3d cpt, double Lae, double L0, DPoint3d& MGvec)
{
	double mgDis = L0;
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	//ת��XOYƽ�洦��
	DPoint3d ptstr = str;
	DPoint3d ptstr_1 = str_1;

	ptstr.z = 0;
	ptstr_1.z = 0;
	DPoint3d zeropt = DPoint3d::From(0, 0, 0);
	DPoint3d tmpvecZ = DPoint3d::From(0, 0, 1);
	Transform tran;			//����ͶӰ����
	mdlTMatrix_computeFlattenTransform(&tran, &zeropt, &tmpvecZ);

	DPoint3d vecLine = ptstr - ptstr_1;//��ʼ�ߴ�������
	vecLine.Normalize();
	vecLine.Scale(thick*1.5*uor_per_mm);//�ӳ�
	ptstr.Add(vecLine);
	vecLine.Normalize();
	vecLine.Scale(10 * uor_per_mm);//�����յ�
	ptstr_1.Add(vecLine);
	EditElementHandle nlineeeh;
	LineHandler::CreateLineElement(nlineeeh, nullptr, DSegment3d::From(ptstr, ptstr_1), true, *ACTIVEMODEL);
	//nlineeeh.AddToModel();
	DPoint3d minP;
	DPoint3d maxP;
	//����ָ��Ԫ����������Ԫ�صķ�Χ��
	mdlElmdscr_computeRange(&minP, &maxP, Wallfaces, NULL);
	minP.x = minP.x + 50, minP.y = minP.y + 50;
	maxP.x = maxP.x - 50, maxP.y = maxP.y - 50;
	DRange3d Wallrange;
	Wallrange.low = minP;
	Wallrange.high = maxP;
	bool isInside = false;
	vecLine = ptstr - ptstr_1;
	vecLine.Normalize();
	vecLine.Scale(-50 * uor_per_mm);
	vector<DPoint3d> strpts;//�����������Ͻ��ĵ㼯��
	vector<double>   vecmg;//ê�̳��ȼ���
	vector<Dpoint3d> vecmgvec;//ê�̷��򼯺�
	for (int i = 0; i < cutWallfaces.size(); i++)
	{
		MSElementDescrP copydescr = nullptr;
		mdlElmdscr_duplicate(&copydescr, cutWallfaces.at(i));
		//����ͶӰ��XOYƽ��
		mdlElmdscr_transform(&copydescr, &tran);
		//mdlElmdscr_add(copydescr);
		//��������Ľ���
		DPoint3d arcinter[2];
		//��Χ�ж�
		DPoint3d lowPt = { 0,0,0 }, highPt = { 0,0,0 };
		if (mdlIntersect_allBetweenElms(arcinter, nullptr, 2, nlineeeh.GetElementDescrP(), copydescr, nullptr, 0.1) == 2)//���û�н��㣬�����ƶ���
		{
			DPoint3d LineVect;
			MSElementDescrP my_face = nullptr;
			if (Wallrange.IsContainedXY(arcinter[0]) || Wallrange.IsContainedXY(arcinter[1]))
			{
				continue;
			}
			if (str.Distance(arcinter[0]) > str.Distance(arcinter[1]))
			{
				strpts.push_back(arcinter[0]);
				LineVect = arcinter[0] - arcinter[1];
				LineVect.Normalize();
			}
			else
			{
				strpts.push_back(arcinter[1]);
				LineVect = arcinter[1] - arcinter[0];
				LineVect.Normalize();
			}
			//PITCommonTool::CPointTool::DrowOnePoint(arcinter[0], 1, 2);
			//PITCommonTool::CPointTool::DrowOnePoint(arcinter[1], 1, 2);
			//ȡ��ǽ����
			double mgdistmp = 0;
			DPoint3d mgVec = DPoint3d::From(0, 0, 0);
			DRange3d range;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			mdlElmdscr_computeRange(&range.low, &range.high, copydescr, NULL);
			range.low.x = range.low.x - 1;
			range.low.y = range.low.y - 1;
			range.high.x = range.high.x + 1;
			range.high.y = range.high.y + 1;
			GetDownFaceVecAndThickness(copydescr, mgVec);
			//mdlElmdscr_add(copydescr);
			DPoint3d vecpt = vecLine;
			vecpt.Normalize();
			if (abs(vecpt.DotProduct(mgVec)) < 0.9)//��ֱʱ���ӳ�
			{
				mgVec.Scale(100 * uor_per_mm);
				DPoint3d mgVecNeg = mgVec;
				mgVecNeg.Scale(-1);
				DPoint3d nowpt = strpts.at(strpts.size() - 1);
				//�ѽ������ڲ��ƶ�һ�����
				{
					nowpt.Add(vecLine);
					DPoint3d nowptNeg = nowpt;
					nowpt.Add(mgVec);
					nowptNeg.Add(mgVecNeg);
					//PITCommonTool::CPointTool::DrowOnePoint(nowpt, 1, 1);
					//PITCommonTool::CPointTool::DrowOnePoint(nowptNeg, 1, 1);
					bool isFront = false;//������ê��ķ���ǽ�ڲ�
					bool isNeg = false;//������ê��ķ�����ǽ�ڲ�
					if (range.IsContainedXY(nowpt))
					{
						isFront = true;
					}
					if (range.IsContainedXY(nowptNeg))
					{
						isNeg = true;
					}

					if (isFront&&isNeg)//�����������˵��Ϊ�ڲ��棬ê�̳���Ϊ15d
					{
						//������LAE���������ǽ�ڣ�˵��Ϊ��ǰ�����������˵��Ϊ������
						mgDis = L0;
						nowpt = strpts.at(strpts.size() - 1);
						mgVec.Normalize();

						//PITCommonTool::CPointTool::DrowOnePoint(nowptNeg, 1, 3);
						mgVec.Scale(thick);
						nowpt.Add(mgVec);
						//PITCommonTool::CPointTool::DrowOnePoint(nowpt, 1, 3);
						if (range.IsContainedXY(nowpt))
						{
							mgVec.Normalize();
						}
						else
						{
							mgVec.Negate();
						}

						double angle = LineVect.AngleToXY(mgVec);
						if (angle > PI / 2 || angle < -PI / 2)
							mgVec.Negate();

						vecmg.push_back(mgDis);
						vecmgvec.push_back(mgVec);

					}
					else if (isFront || isNeg)//ֻ��һ���У�˵��Ϊ�����ֽ����ȡLAE
					{
						mgDis = Lae / 2;
						if (isFront)
							mgVec.Normalize();
						else
						{
							mgVec = mgVecNeg;
							mgVec.Normalize();
						}
						vecmg.push_back(mgDis);
						vecmgvec.push_back(mgVec);
					}

					nowpt = strpts.at(strpts.size() - 1);
					nowpt.Add(vecLine);

					DPoint3d movept = nowpt;
					mgVec.Normalize();
					mgVec.Scale(mgDis);
					movept.Add(mgVec);
					/*
					EditElementHandle tmpeeh;
					LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(nowpt, movept), true, *ACTIVEMODEL);
					tmpeeh.AddToModel();*/

				}


			}

		}
	}

	if (strpts.size() > 0 && vecmg.size() == strpts.size())
	{
		DPoint3d movept = ptstr;
		double movedis = 0;
		for (int i = 0; i < strpts.size(); i++)
		{
			DPoint3d tmppt = strpts.at(i);
			double nowmove = tmppt.Distance(ptstr);
			if (nowmove > movedis)
			{
				movedis = nowmove;
				movept = tmppt;
				mgDis = vecmg[i];//ê�̳���
				MGvec = vecmgvec[i];//ê�̷���
			}
		}
		if (movept.Distance(ptstr) > 100)//�����ƶ������б��Ľ���
		{
			movept.z = cpt.z - 1000;
			DRay3d  dray;
			DVec3d moveFace = DVec3d::From(0, 0, 2000);
			dray.InitFromOriginAndVector(movept, moveFace);
			double hvalue = 0;
			if (dray.Intersect(moveFace, hvalue, plane))
			{
				ptstr = moveFace;
				str = ptstr;
			}


		}

	}
	else//����ӳ�����û��ǽ���ټ����Ƿ���ǽ������ǰǽ����
	{
		ptstr = str;
		ptstr_1 = str_1;

		ptstr.z = 0;
		ptstr_1.z = 0;

		//����������С10MM
		DPoint3d vecLine = ptstr_1 - ptstr;//��ʼ�ߴ�������
		vecLine.Normalize();
		vecLine.Scale(10 * uor_per_mm);//�ӳ�
		ptstr.Add(vecLine);
		vecLine.Normalize();
		vecLine.Scale(-1);//�����յ�
		ptstr_1.Add(vecLine);


		DPoint3d mgVec = DPoint3d::From(0, 0, 1);
		DPoint3d mgVecNeg = DPoint3d::From(0, 0, 0);

		vecLine.Normalize();
		mgVec.CrossProduct(mgVec, vecLine);
		mgVecNeg = mgVec;
		mgVecNeg.Scale(-1);

		DPoint3d nowpt = ptstr;
		DPoint3d nowptNeg = ptstr;
		mgVec.Scale(thick);
		mgVecNeg.Scale(thick);
		nowpt.Add(mgVec);
		nowptNeg.Add(mgVecNeg);
		//PITCommonTool::CPointTool::DrowOnePoint(nowpt,1,11);
		//PITCommonTool::CPointTool::DrowOnePoint(nowptNeg,1,11);
		vector <double>max_data;//���곤��
		vector <DPoint3d>mov_Direction;//���귽��
		for (int i = 0; i < cutWallfaces.size(); i++)
		{
			MSElementDescrP copydescr = nullptr;
			mdlElmdscr_duplicate(&copydescr, cutWallfaces.at(i));
			//����ͶӰ��XOYƽ��
			mdlElmdscr_transform(&copydescr, &tran);

			DRange3d range;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			mdlElmdscr_computeRange(&range.low, &range.high, copydescr, NULL);
			range.low.x = range.low.x - 10;
			range.low.y = range.low.y - 10;
			range.high.x = range.high.x + 10;
			range.high.y = range.high.y + 10;
			bool isFront = false;//������ê��ķ���ǽ�ڲ�
			bool isNeg = false;//������ê��ķ�����ǽ�ڲ�
			if (range.IsContainedXY(nowpt))
			{
				isFront = true;
			}
			if (range.IsContainedXY(nowptNeg))
			{
				isNeg = true;
			}
			if (isFront || isNeg)
			{
				mgVec.Normalize();
				mgVecNeg.Normalize();
				nowpt = ptstr;
				nowptNeg = ptstr;
				mgVec.Scale(100 * uor_per_mm);
				mgVecNeg.Scale(100 * uor_per_mm);
				nowpt.Add(mgVec);
				nowptNeg.Add(mgVecNeg);
				bool isFront2 = false;//������ê��ķ���ǽ�ڲ�
				bool isNeg2 = false;//������ê��ķ�����ǽ�ڲ�
				//PITCommonTool::CPointTool::DrowOnePoint(nowpt, 1,1);
				//PITCommonTool::CPointTool::DrowOnePoint(nowptNeg, 1,1);
				if (range.IsContainedXY(nowpt))
				{
					isFront2 = true;
				}
				if (range.IsContainedXY(nowptNeg))
				{
					isNeg2 = true;
				}
				if (isFront2 || isNeg2)//�����������˵��Ϊ�ڲ��棬ê�̳���Ϊ15d
				{
					//������LAE���������ǽ�ڣ�˵��Ϊ��ǰ�����������˵��Ϊ������
					mgDis = L0;
					nowpt = ptstr;
					mgVec.Normalize();

					//PITCommonTool::CPointTool::DrowOnePoint(nowptNeg, 1, 3);
					mgVec.Scale(thick);
					nowpt.Add(mgVec);
					//PITCommonTool::CPointTool::DrowOnePoint(nowpt, 1, 3);
					if (isFront)
					{
						mgVec.Normalize();
						Dpoint3d  ptr = ptstr;
						Dpoint3d  ptr1 = range.high;
						ptr1.Add(range.low);
						ptr1.Scale(0.5);
						ptr.Add(ptstr_1);
						ptr.Scale(0.5);
						DPoint3d vec = ptr1 - ptr;
						vec.Normalize();
						double angle = vec.AngleToXY(mgVec);
						if (angle > PI / 2 || angle < -PI / 2)
							mgVec.Negate();
						MGvec = mgVec;
					}
					else
					{
						mgVecNeg.Normalize();
						MGvec = mgVecNeg;
					}

					mgVec = MGvec;
					mgVec.Scale(mgDis);
					nowpt = ptstr;
					nowpt.Add(vecLine);
					DPoint3d movept = nowpt;
					movept.Add(mgVec);
					max_data.push_back(mgDis);
					mov_Direction.push_back(MGvec);
					/*EditElementHandle tmpeeh;
					LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(nowpt, movept), true, *ACTIVEMODEL);
					tmpeeh.AddToModel();*/
				}
				else //Ϊ�����ֽ����ȡLAE
				{
					mgDis = Lae / 2;
					if (isFront)
					{
						mgVec.Normalize();
						MGvec = mgVec;
					}
					else
					{
						mgVecNeg.Normalize();
						MGvec = mgVecNeg;
					}

					mgVec = MGvec;
					nowpt = ptstr;
					nowpt.Add(vecLine);
					DPoint3d movept = nowpt;
					mgVec.Scale(mgDis);
					movept.Add(mgVec);
					max_data.push_back(mgDis);
					mov_Direction.push_back(MGvec);
					/*EditElementHandle tmpeeh;
					LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(nowpt, movept), true, *ACTIVEMODEL);
					tmpeeh.AddToModel();*/
				}
			}
		}
	}

	return mgDis;
}

/*
* @desc:		����endType
* @param[in]	data �ֽ�������
* @param[in]	sizeKey
* @param[out]	pitRebarEndTypes �˲���ʽ
* @param[in]	modelRef
* @author	Hong ZhuoHui
* @Date:	2023/09/19
*/
void WallRebarAssembly::interPtsSort(vector<DPoint3d> &interPts, const DPoint3d &originPt)
{
	std::sort(interPts.begin(), interPts.end(), [&](const DPoint3d& pt1, const DPoint3d& pt2) {
		double dis1 = originPt.Distance(pt1);
		double dis2 = originPt.Distance(pt2);
		return COMPARE_VALUES_EPS(dis1, dis2, 1e-6) == 1;
	});
}


//��ȡ����
double WallRebarAssembly::GetFloorThickness(EditElementHandleR Eleeh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	DPoint3d minP2, maxP2;
	//����ָ��Ԫ����������Ԫ�صķ�Χ��
	mdlElmdscr_computeRange(&minP2, &maxP2, Eleeh.GetElementDescrP(), NULL);
	if (!Eleeh.IsValid())
	{
		mdlDialog_dmsgsPrint(L"�Ƿ��İ�ʵ��!");
		return 0.0;
	}
	DPoint3d minPos;
	EditElementHandle downface;
	if (!ExtractFacesTool::GetFloorDownFace(Eleeh, downface, minPos, true))
	{
		return false;
	}
	DPoint3d midPT = minPos;
	midPT.z = midPT.z - 10 * uor_now;
	DPoint3d uppt = midPT;
	uppt.z = maxP2.z + 10 * uor_now;

	vector<DPoint3d> tmppts;
	Transform matrix;
	matrix.InitIdentity();
	vector<DPoint3d> tmpptsTmp;
	tmpptsTmp.clear();
	GetIntersectPointsWithOldElm(tmpptsTmp, &Eleeh, midPT, uppt, 0, matrix);
	double thight = maxP2.z - minP2.z;
	double thight2 = maxP2.z - minP2.z;

	{
		//ͨ�����������
		double minz = maxP2.z;
		double maxz = minP2.z;
		for (int i = 0; i < tmpptsTmp.size(); i++)
		{
			if (tmpptsTmp[i].z > maxz)
				maxz = tmpptsTmp[i].z;
			if (tmpptsTmp[i].z < minz)
				minz = tmpptsTmp[i].z;

		}
		thight = abs(maxz - minz) / uor_now;

		//ͨ���߲������
		DVec3d vecZ = DVec3d::UnitZ();
		DPoint3d facenormal;
		minPos = minP2;
		minPos.Add(maxP2);
		minPos.Scale(0.5);
		mdlElmdscr_extractNormal(&facenormal, nullptr, downface.GetElementDescrP(), NULL);

		facenormal.Scale(-1);
		RotMatrix rMatrix;
		Transform trans;
		mdlRMatrix_getIdentity(&rMatrix);
		mdlRMatrix_fromVectorToVector(&rMatrix, &facenormal, &vecZ);//��ת��xoyƽ��
		mdlTMatrix_fromRMatrix(&trans, &rMatrix);
		mdlTMatrix_setOrigin(&trans, &minPos);
		Eleeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(Eleeh, TransformInfo(trans));
		//����ָ��Ԫ����������Ԫ�صķ�Χ��
		mdlElmdscr_computeRange(&minP2, &maxP2, Eleeh.GetElementDescrP(), NULL);
		thight2 = (maxP2.z - minP2.z) / uor_now;
	}
	thight = (thight < thight2) ? thight : thight2;
	return thight;
}

bool WallRebarAssembly::get_value1(vector<ElementId> vec_walls, EditElementHandle& tmpeeh)
{
	for (int i = 0; i < vec_walls.size(); i++)
	{
		if (tmpeeh.GetElementId() == vec_walls[i])
		{
			return true;
		}
	}
	return false;
}

//��ȡ���Ҳ�ǽ�ĵ���
void WallRebarAssembly::GetLeftRightWallFaces(WallRebarAssembly::WallData& walldata, EditElementHandleR eeh, string wallname)
{
	Transform matrix;
	DPoint3d minP = { 0 }, maxP = { 0 };
	mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
	DPoint3d ptCenter = minP;
	ptCenter.Add(maxP);
	ptCenter.Scale(0.5);
	double Xlenth = maxP.x - minP.x;
	double Ylenth = maxP.y - minP.y;
	mdlTMatrix_getIdentity(&matrix);
	mdlTMatrix_scale(&matrix, &matrix, 1.0 + 100.0 / Xlenth, 1.0 + 100.0 / Ylenth, 0.8);
	mdlTMatrix_setOrigin(&matrix, &ptCenter);
	TransformInfo tInfo(matrix);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, tInfo);
	vector<ElementId> vec_walls;
	GetElementXAttribute(eeh.GetElementId(), vec_walls, UnionVecWallIDXAttribute, eeh.GetModelRef());

	std::vector<IDandModelref>  Same_Eles;
	GetNowScanElems(eeh, Same_Eles);
	for (int i = 0; i < Same_Eles.size(); i++)
	{
		string Ename, Etype;
		EditElementHandle tmpeeh(Same_Eles.at(i).ID, Same_Eles.at(i).tModel);
		if (GetEleNameAndType(tmpeeh, Ename, Etype))
		{
			if (get_value1(vec_walls, tmpeeh))
				continue;
			if (Etype.find("WALL") != string::npos&&Ename.find(wallname) == string::npos)
			{
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(tmpeeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(tmpeeh);
				DPoint3d maxpt;
				MSElementDescrP downface = nullptr;

				downface = ExtractFacesTool::GetCombineDownFace(tmpeeh);

				if (downface != nullptr)
				{
					//mdlElmdscr_add(downface);
					walldata.cutWallfaces.push_back(downface);
					walldata.wallID.push_back(Same_Eles.at(i));
				}

			}
		}
	}
}
//��ȡ��嶥���׵ĵ���
void WallRebarAssembly::GetUpDownFloorFaces(WallRebarAssembly::WallData& walldata, EditElementHandleR eeh)
{

	Transform matrix;
	DPoint3d minP = { 0 }, maxP = { 0 };
	mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
	DPoint3d ptCenter = minP;
	ptCenter.Add(maxP);
	ptCenter.Scale(0.5);
	double Xlenth = maxP.x - minP.x;
	double Ylenth = maxP.y - minP.y;
	mdlTMatrix_getIdentity(&matrix);
	mdlTMatrix_scale(&matrix, &matrix, 1.0 + 100.0 / Xlenth, 1.0 + 100.0 / Ylenth, 1.01);
	mdlTMatrix_setOrigin(&matrix, &ptCenter);
	TransformInfo tInfo(matrix);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, tInfo);

	std::vector<IDandModelref>  Same_Eles;
	std::vector<IDandModelref>  Eles;
	GetNowScanElems(eeh, Same_Eles);
	walldata.upfloorID.clear();
	walldata.downfloorID.clear();
	walldata.floorID.clear();
	for (int i = 0; i < Same_Eles.size(); i++)
	{
		string Ename, Etype;
		EditElementHandle tmpeeh(Same_Eles.at(i).ID, Same_Eles.at(i).tModel);
		if (GetEleNameAndType(tmpeeh, Ename, Etype))
		{
			if (Etype == "FLOOR")
			{
				/*ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(tmpeeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(tmpeeh);*/
				EditElementHandle downeh;

				double tHeight;
				//ExtractFacesTool::GetDownFace(tmpeeh, downeh, &tHeight);
				//MSElementDescrP downface = downeh.ExtractElementDescr();
				MSElementDescrP downface = WallRebarAssembly::GetElementDownFace(tmpeeh, downeh, &tHeight);
				if (downface == nullptr)
				{
					continue;
				}
				{
					DPoint3d minP2 = { 0 }, maxP2 = { 0 };
					mdlElmdscr_computeRange(&minP2, &maxP2, tmpeeh.GetElementDescrP(), NULL);
					//mdlElmdscr_add(downface);   //������ʾ��ǰ��ĵ���
					//�������
					double thick = GetFloorThickness(tmpeeh);
					if ((int)maxP2.z >= (int)maxP.z && (int)minP2.z > (int)minP.z)//�ж��Ƿ�Ϊ����
					{
						walldata.upfloorfaces.push_back(downface);
						walldata.upfloorth = thick;
						walldata.upfloorID.push_back(Same_Eles.at(i));
					}
					else if (((int)maxP2.z < (int)maxP.z && (int)minP2.z <= (int)minP.z)|| ((int)maxP2.z < (int)maxP.z && (int)minP2.z >= (int)minP.z)||
						((int)maxP2.z >= (int)maxP.z && (int)minP2.z < (int)minP.z) || ((int)minP.z >= (int)maxP2.z))//�ж��Ƿ�Ϊ�װ�
					{
						walldata.downfloorfaces.push_back(downface);
						if (thick < walldata.downfloorth || walldata.downfloorth == 0)
							walldata.downfloorth = thick;
						walldata.downfloorID.push_back(Same_Eles.at(i));
					}
					else
					{
						walldata.floorID.push_back(Same_Eles.at(i));
					}

				}
				Eles.push_back(Same_Eles.at(i));
			}

		}
	}
	for (auto it : Eles)
	{
		bool up = false, down = false;
		EditElementHandle tmpeeh(it.ID, it.tModel);
		DPoint3d minP2 = { 0 }, maxP2 = { 0 };
		mdlElmdscr_computeRange(&minP2, &maxP2, tmpeeh.GetElementDescrP(), NULL);
		minP2.z -= 500; maxP2.z += 500;
		for (auto itr : Eles)
		{
			EditElementHandle tmpeeh2(itr.ID, itr.tModel);
			DPoint3d minP = { 0 }, maxP = { 0 };
			mdlElmdscr_computeRange(&minP, &maxP, tmpeeh2.GetElementDescrP(), NULL);
			DRange3d range;
			range.low = minP;
			range.high = maxP;

			if (range.IsContained(maxP2))
				up = true;
			if (range.IsContained(minP2))
				down = true;
		}
		if (down&&up)
		{
			ElementCopyContext copier2(ACTIVEMODEL);
			copier2.SetSourceModelRef(tmpeeh.GetModelRef());
			copier2.SetTransformToDestination(true);
			copier2.SetWriteElements(false);
			copier2.DoCopy(tmpeeh);
			DPoint3d maxpt;
			MSElementDescrP downface = nullptr;

			downface = ExtractFacesTool::GetCombineDownFace(tmpeeh);

			if (downface != nullptr)
			{
				//mdlElmdscr_add(downface);
				walldata.cutWallfaces.push_back(downface);
				walldata.wallID.push_back(it);
			}
		}
	}
}
