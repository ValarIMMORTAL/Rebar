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
		else// ��ȡZ���ĵ���
		{
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

bool ArcWallRebarAssembly::AnalyzingWallGeometricDataARC(ElementHandleCR eh, PIT::ArcSegment &arcFront, PIT::ArcSegment &arcBack)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle testeeh(eh, false);

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &m_ArcWallData.height);

	double frontRadius, backRadius;
	frontRadius = backRadius = 0;
	int j = 0;
	for (int i = 0; i < vecDownFaceLine.size(); i++)
	{
		if (vecDownFaceLine[i] != nullptr)
		{
			if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
			{
				double starR, sweepR;
				double radius;
				DPoint3d ArcDPs[2];
				RotMatrix rotM;
				DPoint3d centerpt;
				mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &vecDownFaceLine[i]->el);
				if (j == 0)
				{
					frontRadius = backRadius = radius;
					j++;
				}
				else
				{
					if (frontRadius > radius)
					{
						frontRadius = radius;
					}
					if (backRadius < radius)
					{
						backRadius = radius;
					}
				}
			}
		}
	}
	double starangleF, endangleF;
	double starangleB, endangleB;
	DPoint3d centerpt;
	CalcuteStrEndAngleAndRadius(starangleF, endangleF, frontRadius, centerpt, vecDownFaceLine);
	CalcuteStrEndAngleAndRadius(starangleB, endangleB, backRadius, centerpt, vecDownFaceLine);

	bool scalef = false;
	if (starangleF > endangleF)
	{
		double tmpangle;
		tmpangle = endangleF;
		endangleF = starangleF;
		starangleF = tmpangle;
		scalef = true;
	}
	if (starangleB > endangleB)
	{
		double tmpangle;
		tmpangle = endangleB;
		endangleB = starangleB;
		starangleB = tmpangle;
	}
	//PIT::ArcSegment arcFront, arcBack;
	{//�����⻡�ı�֮ǰ�������յ�
		double sweepAngle = endangleB - starangleB;
		double sweepR;
		if (scalef)
		{
			sweepAngle = 360 - sweepAngle;
			sweepR = -(sweepAngle) / 180 * fc_pi;
		}
		else
		{
			sweepR = (sweepAngle) / 180 * fc_pi;
		}
		//��ʼ�Ƕ���ɨ��Ƕȷ������ƶ���ɨ��ǶȼӴ�
		double starR = (360 - starangleB) / 180 * fc_pi;


		MSElement  newarcOut;
		mdlArc_create(&newarcOut, NULL, &centerpt, backRadius, backRadius, NULL, starR, -sweepR);
		DPoint3d ArcDPs[2];
		RotMatrix rotM;
		double radius;
		mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcOut);
		arcBack.FptStart = ArcDPs[0];
		arcBack.FptEnd = ArcDPs[1];
	}



	double sweepAngle = endangleF - starangleF;
	if (sweepAngle < 180)
	{
		if (starangleF < starangleB)//�⻡ȡɨ�ӽǶ����ʱ�Ļ������ڻ��Ƚ�ʱ
		{
			starangleB = starangleF;
		}
		if (endangleF > endangleB)
		{
			endangleB = endangleF;
		}
	}
	else
	{
		if (starangleF > starangleB)//�⻡ȡɨ�ӽǶ����ʱ�Ļ������ڻ��Ƚ�ʱ
		{
			starangleB = starangleF;
		}
		if (endangleF < endangleB)
		{
			endangleB = endangleF;
		}
	}
	double sweepR;
	if (scalef)
	{
		sweepAngle = 360 - sweepAngle;
		sweepR = -(sweepAngle) / 180 * fc_pi;
	}
	else
	{
		sweepR = (sweepAngle) / 180 * fc_pi;
	}
	//��ʼ�Ƕ���ɨ��Ƕȷ������ƶ���ɨ��ǶȼӴ�
	double starR = (360 - starangleF) / 180 * fc_pi;


	MSElement newarcIn, newarcOut;
	mdlArc_create(&newarcIn, NULL, &centerpt, frontRadius, frontRadius, NULL, starR, -sweepR);



	sweepAngle = endangleB - starangleB;
	if (scalef)
	{
		sweepAngle = 360 - sweepAngle;
		sweepR = -(sweepAngle) / 180 * fc_pi;
	}
	else
	{
		sweepR = (sweepAngle) / 180 * fc_pi;
	}
	//��ʼ�Ƕ���ɨ��Ƕȷ������ƶ���ɨ��ǶȼӴ�
	starR = (360 - starangleB) / 180 * fc_pi;

	mdlArc_create(&newarcOut, NULL, &centerpt, backRadius, backRadius, NULL, starR, -sweepR);


	MSElementDescrP msedIn, exmseOut;
	mdlElmdscr_new(&msedIn, NULL, &newarcIn);
	mdlElmdscr_new(&exmseOut, NULL, &newarcOut);

	//mdlElmdscr_add(msedIn);
	//mdlElmdscr_add(exmseOut);


	DEllipse3d ellipsePro;
	DPoint3d ArcDPs[2];
	RotMatrix rotM;
	double radius;
	mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcIn);
	mdlArc_extractDEllipse3d(&ellipsePro, &newarcIn);
	arcFront.ptStart = ArcDPs[0];
	arcFront.ptEnd = ArcDPs[1];
	arcFront.dRadius = frontRadius;
	arcFront.ptCenter = centerpt;
	arcFront.dLen = ellipsePro.ArcLength();
	mdlElmdscr_pointAtDistance(&arcFront.ptMid, NULL, arcFront.dLen / 2, msedIn, 1e-6);

	//�⻡����
	DEllipse3d ellipseRev;
	DPoint3d ArcoDPs[2];
	mdlArc_extract(ArcoDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcOut);
	mdlArc_extractDEllipse3d(&ellipseRev, &newarcOut);
	arcBack.ptStart = ArcoDPs[0];
	arcBack.ptEnd = ArcoDPs[1];
	arcBack.dRadius = backRadius;
	arcBack.ptCenter = centerpt;
	arcBack.dLen = ellipseRev.ArcLength();
	mdlElmdscr_pointAtDistance(&arcBack.ptMid, NULL, arcBack.dLen / 2, exmseOut, 1e-6);

	m_ArcWallData.thickness = fabs(arcBack.dRadius - arcFront.dRadius);
	m_width = m_ArcWallData.thickness;
	m_ArcWallData.OuterArc = arcBack;
	m_ArcWallData.InnerArc = arcFront;

	DPoint3d tmppt = m_ArcWallData.InnerArc.ptStart;
	m_ArcWallData.InnerArc.ptStart = m_ArcWallData.InnerArc.ptEnd;
	m_ArcWallData.InnerArc.ptEnd = tmppt;

	m_Holeehs = Holeehs;
	mdlElmdscr_freeAll(&exmseOut);
	mdlElmdscr_freeAll(&msedIn);
	return true;
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

bool STWallRebarAssembly::makeRebarCurve
(
	vector<PITRebarCurve>&     rebars,
	double                  xPos,
	double                  yLen,
	double					startOffset,
	double					endOffset,
	PITRebarEndTypes&		endTypes,
	CMatrix3D const&        mat,
	bool istwin
)
{

	CPoint3D  startPt;
	CPoint3D  endPt;

	DPoint3d strmove = m_VecZ;
	strmove.Scale(-yLen / 2.0 + startOffset);
	DPoint3d endmove = m_VecZ;
	endmove.Scale(yLen / 2.0 - endOffset);

	startPt = CPoint3D::From(xPos, 0.0, 0.0);
	endPt = CPoint3D::From(xPos, 0.0, 0.0);

	startPt.Add(strmove);
	endPt.Add(endmove);

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
	//mdlElmdscr_add(eeh.GetElementDescrP());
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	//eeh.AddToModel();

	DPoint3d pt1[2];
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//��ʱʹ�õ�ǰ����MODEL�������������޸�
	//ȷ������յ��Ǵ�С����---begin
	DVec3d vec = pt1[1] - pt1[0];
	DVec3d vecX = DVec3d::From(1, 0, 0);
	vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
	vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
	vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
	vec.Normalize();
	if (vec.IsPerpendicularTo(vecX))
	{
		if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
	}
	else
	{
		if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
	}
	//---end
	DPoint3d tmpstr, tmpend;
	tmpstr = pt1[0];
	tmpend = pt1[1];
	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}

	if (!istwin)//�ǲ���ʱ����Ҫ�洢����Ϣ
	{
		m_vecRebarPtsLayer.push_back(pt1[0]);
		m_vecRebarPtsLayer.push_back(pt1[1]);
	}
	if (m_isPushTieRebar)
	{
		m_vecTieRebarPtsLayer.push_back(pt1[0]);
		m_vecTieRebarPtsLayer.push_back(pt1[1]);
	}
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetSideCover()*uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);

	/*EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
	eehline.AddToModel();*/

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt : tmppts)
	{
		if (ExtractFacesTool::IsPointInLine(pt, pt1[0], pt1[1], ACTIVEMODEL, isStr))
		{
			int dis = (int)pt1[0].Distance(pt);
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = pt1[0];
	}
	else
	{
		map_pts[0] = pt1[0];
	}
	int dis = (int)pt1[0].Distance(pt1[1]);
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = pt1[1];
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = pt1[1];
	}



	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{
		PITRebarEndTypes		tmpendTypes;

		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itr->second);
		vex->SetType(RebarVertex::kStart);
		if (tmpstr.Distance(itr->second) < 10)
		{
			tmpendTypes.beg = endTypes.beg;
		}
		tmpendTypes.beg.SetptOrgin(itr->second);
		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		if (tmpend.Distance(itrplus->second) < 10)
		{
			tmpendTypes.end = endTypes.end;
		}

		tmpendTypes.end.SetptOrgin(itrplus->second);

		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itrplus->second);
		vex->SetType(RebarVertex::kEnd);

		rebar.EvaluateEndTypes(tmpendTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebars.push_back(rebar);
	}


	//rebar.DoMatrix(mat);
	return true;
}

double GetDownFaceVecAndThickness(MSElementDescrP downFace, DPoint3d& Vec)
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


bool STWallRebarAssembly::makeRebarCurve_Laption
(
	RebarCurve&             rebar,
	double                  xPos,
	double                  yLen,
	double                  bendRadius,
	double                  bendLen,
	RebarEndTypes const&    endTypes,
	CVector3D const&        endNormal,
	CMatrix3D const&        mat,
	bool istwin
)
{
	RebarVertexP vex;
	CPoint3D  startPt;
	CPoint3D  endPt;

	startPt = CPoint3D::From(xPos, 0.0, yLen / 2.0);
	endPt = CPoint3D::From(xPos, 0.0, -yLen / 2.0);

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
	//eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);

	DPoint3d pt1[2];
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//��ʱʹ�õ�ǰ����MODEL�������������޸�


	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(pt1[0]);
	vex->SetType(RebarVertex::kStart);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(pt1[1]);
	vex->SetType(RebarVertex::kEnd);

	rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
	//rebar.DoMatrix(mat);
	return true;
}

void STWallRebarAssembly::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

bool STWallRebarAssembly::makaRebarCurve(const vector<DPoint3d>& linePts, double extendStrDis, double extendEndDis,
	double diameter, double strMoveDis, double endMoveDis, bool isInSide,
	const PIT::PITRebarEndTypes& endTypes, vector<PIT::PITRebarCurve>& rebars, std::vector<EditElementHandle*> upflooreehs,
	std::vector<EditElementHandle*> flooreehs, std::vector<EditElementHandle*> Walleehs, std::vector<EditElementHandle*>alleehs)
{
	//�����ṩ�ĵ���������
	EditElementHandle lineEeh;
	LineStringHandler::CreateLineStringElement(lineEeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
	//�����ߵ���β��
	DPoint3d strPt = { 0,0,0 }, endPt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&strPt, nullptr, &endPt, nullptr, lineEeh.GetElementDescrP(), ACTIVEMODEL);
	//mdlElmdscr_add(lineEeh.GetElementDescrP());
	if (GetIntersectPointsWithNegs(m_Negs, strPt, endPt))
	{
		return false;
	}
	m_vecRebarPtsLayer.push_back(strPt);
	m_vecRebarPtsLayer.push_back(endPt);

	//�����ԭʼǽ���ཻ��ĵ�
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	//double dSideCover = GetSideCover()*uor_per_mm;
	//vector<DPoint3d> tmppts;
	//Transform matrix;
	//GetPlacement().AssignTo(matrix);
	//GetIntersectPointsWithHoles(tmppts, m_useHoleehs, strPt, endPt, dSideCover, matrix);
	double dSideCover = 0/*GetSideCover() * uor_per_mm*/;
	vector<DPoint3d> tmppts;
	vector<DPoint3d> tmppts2;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	vector<vector<DPoint3d>> vecPtRebars;
	vector<DPoint3d> tmpptsTmp;
	vector<DPoint3d> tmp_pts_Tmp;
	vecPtRebars.clear();
	PITRebarEndTypes tmpendEndTypes = endTypes;
	PITRebarEndTypes tmpendEndTypes_2;
	int flag_str = 0;
	int flag_end = 1;
	int flag_mid = -1;
	DPoint3d END_PTR;
	DPoint3d STR_PTR;
	tmpptsTmp.clear();
	EditElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	if (!ehWall.IsValid())
	{
		return false;
	}
	EFT::GetSolidElementAndSolidHoles(ehWall, Eleeh, Holeehs);//��ȡ����֮ǰ��ǽ��
	if (Eleeh.IsValid()) // ��ԭʵ���ཻ(�޿׶�)
	{
		//Eleeh.AddToModel();
		GetIntersectPointsWithOldElm(tmpptsTmp, &Eleeh, strPt, endPt, dSideCover, matrix);
		if (tmpptsTmp.size() > 1)
		{
			// ���ڽ���Ϊ�������ϵ����
			GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, strPt, endPt, &Eleeh, dSideCover);
		}
		//  �ų�����ֽ�����
		// ��ȡ is_inside �ľ���ֵ
		double is_transverse = std::abs(strPt.z - endPt.z);
		// �뿪��֮���ģ�Ͳ����ڽ��� ���� ��Ϊ����ֽ� �����
		GetIntersectPointsWithOldElm(tmp_pts_Tmp, &ehWall, strPt, endPt, dSideCover, matrix);
		if (tmp_pts_Tmp.size() < 1  && is_transverse >= 1000 )
		{
			return false;
		}
	}

	
	if (tmpptsTmp.size() < 2 && vecPtRebars.size() == 0)
	{
		vector<DPoint3d> vecPt;
		vecPt.push_back(strPt);
		vecPt.push_back(endPt);

		vecPtRebars.push_back(vecPt);
	}
	//�����ܰ�Ϳ׶���ĵ�	
	for (size_t i = 0; i < vecPtRebars.size(); i++)
	{
		strPt = vecPtRebars.at(i).at(0);
		endPt = vecPtRebars.at(i).at(1);

		CVector3D  Vec(strPt, endPt);
		CVector3D  nowVec = Vec;
		double extendStrDistemp = extendStrDis, extendEndDistemp = extendEndDis;
		//���ݶ��װ����¼���ƫ�ƾ���
		//PITCommonTool::CPointTool::DrowOneLine(strPt, endPt, 2);
		Vec.Normalize();
		if (COMPARE_VALUES_EPS(abs(Vec.x), 1, 0.1) == 0 || COMPARE_VALUES_EPS(abs(Vec.y), 1, 0.1) == 0 || COMPARE_VALUES_EPS(abs(Vec.z), 1, 1e-6) == 0)
		{
			ReCalExtendDisByTopDownFloor(strPt, endPt, strMoveDis, endMoveDis, extendStrDis, extendEndDis, isInSide);
		}
		//������β�ƶ�����
		DVec3d strVec = strPt - endPt;
		strVec.Normalize();
		strVec.ScaleToLength(extendStrDis);
		DVec3d endVec = endPt - strPt;
		endVec.Normalize();
		endVec.ScaleToLength(extendEndDis);
		//�ƶ���β��
		strPt.Add(strVec);
		endPt.Add(endVec);
		//PITCommonTool::CPointTool::DrowOneLine(strPt, endPt, 1);
		EditElementHandle* eeh = &Eleeh;
		//���ƫ�Ƶĵ���������׶�����˵�����洦�����Ƕ���ཻ��
		nowVec.Normalize();
		if (COMPARE_VALUES_EPS(abs(nowVec.z), 1, EPS) == 0)
		{
			//�װ�ת����1.���ê��δ�ڵװ�����ê��ת��2.���ת���δê�뵽�κ�ʵ���У���ԭ����,���п׶��ضϲ���
			CVector3D vector = endTypes.beg.GetendNormal();
			CVector3D vecZ = CVector3D::From(0, 0, -1);
			CVector3D vecO = CVector3D::From(0, 0, 0);
			DPoint3d  endtemp_point3d = strPt;
			DPoint3d nonstrtemp_point3d = strPt;
			double lenth = endTypes.beg.GetbendLen() + endTypes.beg.GetbendRadius();
			FreeAll(Holeehs);
			Holeehs.clear();
			tmppts.clear();
			movePoint(vecZ, endtemp_point3d, lenth);
			DPoint3d zeropt = DPoint3d::From(0, 0, 0);
			DPoint3d tmpvecZ = DPoint3d::From(0, 0, 1);
			Transform tran;			//����ͶӰ����
			mdlTMatrix_computeFlattenTransform(&tran, &zeropt, &tmpvecZ);
			MSElementDescrP downface = nullptr;
			downface = ExtractFacesTool::GetCombineDownFace(ehWall);
			DPoint3d vecwall1 = DPoint3d::From(0, 0, 0);
			GetDownFaceVecAndThickness(downface, vecwall1);

			bool isOpen = false;
			for (int i = 0; i < Walleehs.size(); i++)
			{
				MSElementDescrP downface = nullptr;
				downface = ExtractFacesTool::GetCombineDownFace(*Walleehs[i]);
				//����ͶӰ��XOYƽ��
				mdlElmdscr_transform(&downface, &tran);
				DPoint3d vecwall = DPoint3d::From(0, 0, 0);
				GetDownFaceVecAndThickness(downface, vecwall);
				if (ISPointInElement(Walleehs[i], endtemp_point3d) && abs(vecwall1.DotProduct(vecwall) < 0.9))//��ֱʱ���ӳ�)
				{
					break;
				}
			}
			if (isOpen&&isInSide)//�ж�Z���Ϸ������·��Ƿ���ǽ�������ڲ�ֽ�
			{
				tmpendEndTypes.beg.SetendNormal(vecO);
				tmpendEndTypes.beg.SetType(PIT::PITRebarEndType::Type::kNone);
				strPt = endtemp_point3d;
			}
			else
			{
				endtemp_point3d = strPt;
				JudgeBarLinesLegitimate(nowVec, alleehs, tmpendEndTypes, lineEeh, eeh, vector, matrix, strMoveDis, lenth, strPt, STR_PTR, flag_str);
			}
			
			//����ת����1.���ê��δ�ڶ�������ê��ת��2.���ת���δê�뵽�κ�ʵ���У���ԭ����,���п׶��ضϲ���
			vector = endTypes.end.GetendNormal();
			endtemp_point3d = endPt;
			DPoint3d nonendtemp_point3d = endPt;
			lenth = endTypes.end.GetbendLen() + endTypes.end.GetbendRadius();
			FreeAll(Holeehs);
			Holeehs.clear();
			tmppts.clear();
			vecZ.Negate();
			isOpen = false;
			movePoint(vecZ, endtemp_point3d, lenth);
			for (int i = 0; i < Walleehs.size(); i++)
			{
				MSElementDescrP downface = nullptr;
				downface = ExtractFacesTool::GetCombineDownFace(*Walleehs[i]);
				//����ͶӰ��XOYƽ��
				mdlElmdscr_transform(&downface, &tran);
				DPoint3d vecwall = DPoint3d::From(0, 0, 0);
				GetDownFaceVecAndThickness(downface, vecwall);
				if (ISPointInElement(Walleehs[i], endtemp_point3d) && abs(vecwall1.DotProduct(vecwall) < 0.9))//��ֱʱ���ӳ�)
				{
					//isOpen = true;
					break; 
				}
			}
			if (isOpen&&isInSide)//�ж�Z���Ϸ������·��Ƿ���ǽ�������ڲ�ֽ�
			{
				tmpendEndTypes.end.SetendNormal(vecO);
				tmpendEndTypes.end.SetType(PIT::PITRebarEndType::Type::kNone);
				endPt = endtemp_point3d;
			}
			else
			{
				endtemp_point3d = endPt;
                JudgeBarLinesLegitimate(nowVec,alleehs,tmpendEndTypes, lineEeh,eeh, vector, matrix, endMoveDis,lenth, endPt, END_PTR, flag_end);
			}
		}
		else if (COMPARE_VALUES_EPS(abs(nowVec.x), 1, 0.1) == 0 || COMPARE_VALUES_EPS(abs(nowVec.y), 1, 0.1) == 0)
		{
			DPoint3d  strtemp_point3d = strPt, endtemp_point3d = endPt;
			CVector3D vector1 = endTypes.beg.GetendNormal();
			double lenth1 = endTypes.beg.GetbendLen() + endTypes.beg.GetbendRadius();
			CVector3D vector2 = endTypes.end.GetendNormal();
			double lenth2 = endTypes.end.GetbendLen() + endTypes.end.GetbendRadius();
			CVector3D vectorZ = CVector3D::From(0, 0, 0);
			double lae = get_lae() * diameter / uor_per_mm;
			double der = diameter * 15;
			//�ж����̻����ӳ���ĵ��Ƿ���ǽ��
			strVec.Normalize();
			if (ISPointInHoles(Walleehs, strtemp_point3d) && vector1.IsEqual(vectorZ))
			{
				tmpendEndTypes.beg = Hol;
			    vector1 = tmpendEndTypes.beg.GetendNormal();
				lenth1 = tmpendEndTypes.beg.GetbendLen() + tmpendEndTypes.beg.GetbendRadius();
				CVector3D vec = (m_walldata.vecToWall);
				if (isInSide)
				{
					tmpendEndTypes.beg.SetbendLen(der);
					DPoint3d temp = strtemp_point3d;
					movePoint(vec, temp, lae);
					if (ISPointInHoles(Walleehs, temp))
						tmpendEndTypes.beg.SetendNormal(vec);
					else
						tmpendEndTypes.beg.SetendNormal(vec.Negate());
				}

				//tmpendEndTypes.beg.SetendNormal(CVector3D::kZaxis.Perpendicular(nowVec));
			}
			else
			{
				nowVec.Negate();
				DPoint3d temp = strtemp_point3d;
				movePoint(nowVec, strtemp_point3d, lae);
				movePoint(nowVec, temp, der);
				//PITCommonTool::CPointTool::DrowOnePoint(strtemp_point3d, 1,11);
				if (ISPointInHoles(flooreehs, strtemp_point3d))
				{
					tmpendEndTypes.beg.SetendNormal(vectorZ);
					tmpendEndTypes.beg.SetType(PIT::PITRebarEndType::Type::kNone);
					strPt = strtemp_point3d;
				}
				else if (ISPointInHoles(flooreehs, temp))
				{
					tmpendEndTypes.beg.SetendNormal(vectorZ);
					tmpendEndTypes.beg.SetType(PIT::PITRebarEndType::Type::kNone);
					strPt = temp;
				}
				else
				{
					strtemp_point3d = strPt;
				}
			}

			if (ISPointInHoles(Walleehs, endtemp_point3d) && vector2.IsEqual(vectorZ))
			{
				tmpendEndTypes.end = Hol;
			    vector2 = tmpendEndTypes.end.GetendNormal();
			    lenth2 = tmpendEndTypes.end.GetbendLen() + tmpendEndTypes.end.GetbendRadius();
				CVector3D vec = (m_walldata.vecToWall);
				if (isInSide)
				{
					tmpendEndTypes.end.SetbendLen(der);
					DPoint3d temp = endtemp_point3d;
					movePoint(vec, temp, lae);
					if (ISPointInHoles(Walleehs, temp))
						tmpendEndTypes.end.SetendNormal(vec);
					else
						tmpendEndTypes.end.SetendNormal(vec.Negate());
				}
				//tmpendEndTypes.end.SetendNormal(CVector3D::kZaxis.Perpendicular(nowVec));
			}
			else
			{
				nowVec.Negate();
				DPoint3d temp = endtemp_point3d;
				movePoint(nowVec, endtemp_point3d, lae);
				movePoint(nowVec, temp, der);
				//PITCommonTool::CPointTool::DrowOnePoint(endtemp_point3d, 1, 12);
				if (ISPointInHoles(flooreehs, endtemp_point3d))
				{

					tmpendEndTypes.end.SetendNormal(vectorZ);
					tmpendEndTypes.end.SetType(PIT::PITRebarEndType::Type::kNone);
					endPt = endtemp_point3d;
				}
				else if (ISPointInHoles(flooreehs, temp))
				{
					tmpendEndTypes.end.SetendNormal(vectorZ);
					tmpendEndTypes.end.SetType(PIT::PITRebarEndType::Type::kNone);
					endPt = temp;
				}
				else
				{
					endtemp_point3d = endPt;
				}
			}

		    //�ж��Ƿ�ê��
		    EditElementHandle* eeh = &Eleeh; 
		    //�ж���ʼ�����ʼ��ê��λ���Ƿ�ê��
		    JudgeBarLinesLegitimate(nowVec, alleehs, tmpendEndTypes, lineEeh, eeh, vector1, matrix, strMoveDis, lenth1, strtemp_point3d, STR_PTR, flag_str);
		    //β�˵��β�˵�λ���Ƿ�ê��
		    JudgeBarLinesLegitimate(nowVec, alleehs, tmpendEndTypes, lineEeh, eeh, vector2, matrix, endMoveDis, lenth2, endtemp_point3d, END_PTR, flag_end);
		}

		DPoint3d strPttepm = strPt, endPttepm = endPt;

		GetIntersectPointsWithHoles(tmppts, m_useHoleehs, strPt, endPt, dSideCover, matrix);

		vector<DPoint3d> aro_tmppts;//��Χʵ��׶�����
		vector<DPoint3d> uniquePoints;
		GetIntersectPointsWithHoles(aro_tmppts, m_Around_Ele_Holeehs, strPt, endPt, dSideCover, matrix);


		for (size_t j = 0; j < aro_tmppts.size(); j++) {
			bool isUnique = true;

			for (size_t i = 0; i < uniquePoints.size(); i++) {
				// �����Ψһ��
				if (uniquePoints[i].x + 1 > aro_tmppts[j].x &&
					uniquePoints[i].x - 1 < aro_tmppts[j].x &&
					uniquePoints[i].y + 1 > aro_tmppts[j].y &&
					uniquePoints[i].y - 1 < aro_tmppts[j].y &&
					uniquePoints[i].z + 1 > aro_tmppts[j].z &&
					uniquePoints[i].z - 1 < aro_tmppts[j].z) {
					isUnique = false; // �ҵ������Ƶĵ�
					break; // ������ǰ�˳�ѭ��
				}
			}

			if (isUnique) {
				uniquePoints.push_back(aro_tmppts[j]); // ���Ψһ�ĵ�
			}
		}

		//�жϵװ�ֽ�ê�����Ƿ������׶���������˰�,��������������������Ҫ��ת��ǰ�˲�����ֻ������ֱ����
		if (tmppts.empty() && !aro_tmppts.empty()) {
			for ( auto point : uniquePoints) {
				//PITCommonTool::CPointTool::DrowOnePoint(point, 1, 3); // ��
				tmppts.push_back(point);
			}
		}
		
		//���ݹ�ܿ׶���ĵ㽫�㰴������࣬����һ�飬�ȵ��ֽ��ߵ���β��
		map<int, DPoint3d> map_pts;
		bool isStr = false;
		for (DPoint3d pt : tmppts)
		{
			if (ExtractFacesTool::IsPointInLine(pt, strPt, endPt, ACTIVEMODEL, isStr))
			{
				int dis = (int)strPt.Distance(pt);
				if (map_pts.find(dis) != map_pts.end())
				{
					dis = dis + 1;
				}
				map_pts[dis] = pt;
			}
		}
		if (map_pts.find(0) != map_pts.end())
		{
			if (flag_str == 2)
			{
				map_pts[1] = STR_PTR;
			}
			else
			map_pts[1] = strPt;
		}
		else
		{
			if (flag_str == 2)
			{
				map_pts[0] = STR_PTR;
			}
			else
			map_pts[0] = strPt;
		}
		int dis = (int)strPt.Distance(endPt);
		if (map_pts.find(dis) == map_pts.end())
		{
			if (flag_end == 2)
			{
				map_pts[dis] = END_PTR;
			}
			else
			map_pts[dis] = endPt;
		}
		else
		{
			dis = dis + 1;
			if (flag_end == 2)
			{
				map_pts[dis] = END_PTR;
			}
			else
			map_pts[dis] = endPt;
		}
		for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
		{
			PITRebarEndTypes		tmpendTypes;
			CVector3D  nowVectmp;
			PITRebarCurve rebar;	
			RebarVertexP vex;
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(itr->second);
			vex->SetType(RebarVertex::kStart);
			if (strPttepm.Distance(itr->second) < 10)
			{
				if (flag_str == 3)
				{
					tmpendTypes.beg = tmpendEndTypes_2.beg;
				}
				else
				tmpendTypes.beg = tmpendEndTypes.beg;
			}
			else
			{
				//nowVectmp = tmpendEndTypes.beg.GetendNormal();
				tmpendTypes.beg = Hol;
				//tmpendTypes.beg.SetendNormal(nowVectmp.Negate());
			}
			tmpendTypes.beg.SetptOrgin(itr->second);
			map<int, DPoint3d>::iterator itrplus = ++itr;
			if (itrplus == map_pts.end())
			{
				break;
			}
			if (endPttepm.Distance(itrplus->second) < 10)
			{
				if (flag_end == 3)
				{
					tmpendTypes.end= tmpendEndTypes_2.end;
				}
				else
				tmpendTypes.end = tmpendEndTypes.end;
			}
			else
			{
				//nowVectmp = tmpendEndTypes.end.GetendNormal();
				tmpendTypes.end = Hol;
				//tmpendTypes.end.SetendNormal(nowVectmp.Negate());
			}

			tmpendTypes.end.SetptOrgin(itrplus->second);

			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(itrplus->second);
			vex->SetType(RebarVertex::kEnd);

			rebar.EvaluateEndTypes(tmpendTypes);
			//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
			rebars.push_back(rebar);
		}

	}

	return true;
}

RebarSetTag* STWallRebarAssembly::MakeRebars
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	double              xLen,
	double              height,
	double              spacing,
	double              startOffset,
	double              endOffset,
	vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
	vector<CVector3D> const& vecEndNormal,
	CMatrix3D const&    mat,
	TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
	bool				bTwinbarLevel,
	int level,
	int grade,
	int DataExchange,
	DgnModelRefP        modelRef,
	int rebarLineStyle,
	int rebarWeight
)
{
	//	m_IsTwinrebar = bTwinbarLevel;
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	if (endType.size() != vecEndNormal.size() || endType.size() == 0)
	{
		return NULL;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, startbendLenTb, endbendLen, endbendLenTb;
	double begStraightAnchorLen, endStraightAnchorLen;
	switch (endType[0].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeStart.SetType(RebarEndType::kLap);
		begStraightAnchorLen = endType[0].endPtInfo.value1;	//ê�볤��
		break;
	case 4:	//90���乳
	{
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		}
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//������100
	}
	break;
	case 5:	//135���乳
	{
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		}
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//������100
	}
	break;
	case 6:	//180���乳
	{
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		}
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//������100
	}
	break;
	case 8:	//�û�
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (endType[1].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeEnd.SetType(RebarEndType::kLap);
		endStraightAnchorLen = endType[1].endPtInfo.value1;	//ê�볤��
		break;
	case 4:	//90���乳
	{
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = endType[1].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		}
		endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}
	break;
	case 5:	//135���乳
	{
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = endType[1].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		}
		endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}
	break;
	case 6:	//180���乳
	{
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = endType[1].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		}
		endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}

	break;
	case 8:	//�û�
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}
	double updownSideCover = 50 * uor_per_mm;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);	//������30
	double adjustedXLen, adjustedSpacing;

	double leftSideCov, rightSideCov, allSideCov;
	if (startOffset > 0 || endOffset > 0)
	{
		leftSideCov = GetSideCover()*uor_per_mm / sin(m_angle_left);
		rightSideCov = GetSideCover()*uor_per_mm / sin(m_angle_right);

	}
	else
	{
		leftSideCov = GetSideCover()*uor_per_mm;
		rightSideCov = GetSideCover()*uor_per_mm;
	}
	allSideCov = leftSideCov + rightSideCov;
	double allUDSideCov = updownSideCover * 2;//�������ҵĺ������̶�Ϊ50���ֽ������ߵ�ǽ�ߵľ���
	int numRebar = 0;
	if (twinBarInfo.hasTwinbars)	//����
		adjustedXLen = xLen - allUDSideCov /*-diameter- diameterTb */ - startOffset - endOffset;
	else
		adjustedXLen = xLen - allUDSideCov - startOffset - endOffset;
	if (bTwinbarLevel)				//�����ֽ�����
	{
		numRebar = (int)floor(adjustedXLen / (spacing * (twinBarInfo.interval + 1)) + 0.5) + 1;
		int numRebar1 = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		//if (numRebar1 > 1)
		//{
		//	adjustedSpacing = adjustedXLen / (numRebar1 - 1);	//�ǲ����ƽ�����
		//	adjustedSpacing *= (twinBarInfo.interval + 1);		//�����ʵ�ʼ������Ըֽ���
		//}
	}
	else
	{
		numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		/*	if (numRebar > 1)
				adjustedSpacing = adjustedXLen / (numRebar - 1);*/
	}

	double xPos = startOffset;
	if (m_nowvecDir == 0)
		xPos = -xPos;
	xPos = xPos - diameter / 2;
	if (bTwinbarLevel)				//�������ƫ��һ�ξ���
	{
		xPos += diameter * 0.5;
		if (diameterTb > diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radiusTb - radius)*(radiusTb - radius), 0.5) - radius;
			xPos += dOffset;
		}
		else if (diameterTb < diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radius - radiusTb)*(radius - radiusTb), 0.5) - radius;
			xPos += dOffset;
		}
		else
		{
			xPos += diameter * 0.5;
		}
	}
	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		start.SetbendLen(startbendLenTb);
		start.SetbendRadius(bendRadiusTb);
	}
	else
	{
		start.SetbendLen(startbendLen);
		start.SetbendRadius(startbendRadius);
	}
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		end.SetbendLen(endbendLenTb);
		end.SetbendRadius(bendRadiusTb);
	}
	else
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };

	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurves;
		if (i == numRebar - 1)//��������һ����Ҫ�жϵ�ǰ���ж��پ���,���Ͼ���Ҫ���Ҫ�ٲ���һ��
		{
			double sDis = adjustedXLen - (numRebar - 2)*adjustedSpacing;
			if (sDis > 30 * uor_per_mm)
			{
				if (m_nowvecDir == 0)
				{
					xPos -= sDis;
					if (bTwinbarLevel)
					{
						xPos += (diameter + diameterTb);
					}
				}
				else
				{
					xPos += sDis;
					if (bTwinbarLevel)
					{
						xPos -= (diameter + diameterTb);
					}
				}
			}
			else
			{
				continue;
			}
		}
		else if (i != 0)//�������һ����Ҳ���ǵ�һ�������ϼ��
		{
			if (m_nowvecDir == 0)
			{
				xPos -= adjustedSpacing;
			}
			else
			{
				xPos += adjustedSpacing;
			}
		}
		makeRebarCurve(rebarCurves, xPos, height - allSideCov, endTypeStartOffset, endTypEendOffset, endTypes, mat, bTwinbarLevel);
		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinBarInfo.rebarSize, symb);
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}
	else
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}

	vector<vector<DPoint3d>> vecStartEnd;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (ISPointInHoles(m_useHoleehs, midPos))
		{
			if (ISPointInHoles(m_useHoleehs, ptstr) && ISPointInHoles(m_useHoleehs, ptend))
			{
				continue;
			}
		}
		if (ISPointInHoles(m_Negs, midPos))
		{
			if (ISPointInHoles(m_Negs, ptstr) && ISPointInHoles(m_Negs, ptend))
			{
				continue;
			}
		}

		vector<DPoint3d> linePts;
		RebarVertices vertices = rebarCurve.GetVertices();
		for (size_t i = 0; i < (size_t)vertices.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &vertices.At(i);
			linePts.push_back(tmpVertex->GetIP());
		}
		/*EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.AddToModel();
	*/
		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinBarInfo.rebarSize));
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameterTb, endTypes, shape, modelRef, false);

			}
			else
			{
				shape.SetSizeKey((LPCTSTR)sizeKey);
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
			}
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (DataExchange == 0)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinback";
				else
					Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);

			ElementPropertiesSetterPtr propEle = ElementPropertiesSetter::Create();
			propEle->SetLinestyle(rebarLineStyle, NULL);
			propEle->SetWeight(rebarWeight);
			propEle->Apply(tmprebar);
			tmprebar.ReplaceInModel(oldref);

		}
		j++;
		vecStartEnd.push_back(linePts);
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	CString spacingstring;
	spacingstring.Format(_T("%lf"), spacing / uor_per_mm);
	setdata.SetSpacingString(spacingstring);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����
	rebarSet->SetSetData(setdata);
	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

RebarSetTag* STWallRebarAssembly::MakeRebars_Laption
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	double              xLen,
	double              height,
	double              spacing,
	double				startOffset,
	double				endOffset,
	LapOptions const& lapOptions,
	vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
	CVector3D const&    endNormal,
	CMatrix3D const&    mat,
	bool				bTwinBars,
	DgnModelRefP        modelRef
)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;
	if (0 == lapOptions.connectMethod)	//��е����
	{
		endTypeStart.SetType(RebarEndType::kTerminator);
		endTypeEnd.SetType(RebarEndType::kTerminator);
	}
	else	//�ǻ�е���ӣ�ʹ�ö˲���ʽ
	{
		/*
		enum Type
		{
			kNone = 0,
			kBend,          // 90
			kCog,           // 135
			kHook,          // 180
			kLap,
			kCustom,
			kSplice,
			kTerminator         // Mechanical Device
		};
		*/
		switch (endType[0].endType)
		{
		case 0:	//��
		case 1:	//����
		case 2:	//����
		case 3:	//����
			endTypeStart.SetType(RebarEndType::kNone);
			break;
		case 7:	//ֱê
			endTypeStart.SetType(RebarEndType::kLap);
			break;
		case 4:	//90���乳
			endTypeStart.SetType(RebarEndType::kBend);
			break;
		case 5:	//135���乳
			endTypeStart.SetType(RebarEndType::kCog);
			break;
		case 6:	//180���乳
			endTypeStart.SetType(RebarEndType::kHook);
			break;
		case 8:	//�û�
			endTypeStart.SetType(RebarEndType::kCustom);
			break;
		default:
			break;
		}

		switch (endType[1].endType)
		{
		case 0:	//��
		case 1:	//����
		case 2:	//����
		case 3:	//����
			endTypeEnd.SetType(RebarEndType::kNone);
			break;
		case 7:	//ֱê
			endTypeEnd.SetType(RebarEndType::kLap);
			break;
		case 4:	//90���乳
			endTypeEnd.SetType(RebarEndType::kBend);
			break;
		case 5:	//135���乳
			endTypeEnd.SetType(RebarEndType::kCog);
			break;
		case 6:	//180���乳
			endTypeEnd.SetType(RebarEndType::kHook);
			break;
		case 8:	//�û�
			endTypeEnd.SetType(RebarEndType::kCustom);
			break;
		default:
			break;
		}
	}
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
	double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
	double leftSideCov = GetSideCover()*uor_per_mm / sin(m_angle_left);
	double rightSideCov = GetSideCover()*uor_per_mm / sin(m_angle_right);
	double allSideCov = leftSideCov + rightSideCov;

	double adjustedXLen = xLen - allSideCov - diameter - startOffset - endOffset;
	int numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;

	double adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);

	// 	double xPos = -adjustedXLen / 2.0;
	double xPos = startOffset;
	if (bTwinBars)
		numRebar *= 2;
	for (int i = 0; i < numRebar; i++)
	{
		RebarCurve      rebarCurve;
		RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
		makeRebarCurve_Laption(rebarCurve, xPos, height - allSideCov, bendRadius, bendLen, endTypes, endNormal, mat);

		RebarSymbology symb;
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);

		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(i, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);

			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
		}
		if (bTwinBars && !(i & 0x01))
			xPos += diameter;
		else
		{
			xPos += adjustedSpacing;
			if (bTwinBars)
				xPos -= diameter;	//ɾ���ϴβ���ƫ�ƾ���
		}
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

RebarSetTag * STWallRebarAssembly::MakeRebars(ElementId & rebarSetId, BrStringCR sizeKey,
	const vector<BarLinesdata>& barLinesData,
	double strOffset, double endOffset, int level, int grade, DgnModelRefP modelRef,
	int rebarLineStyle, int rebarWeight)
{
	for (auto it : barLinesData)
	{
		//���ݱ����㡢�ӳ�ֵ���»��ƻ����ֽ��ߴ�
		//ReCalBarLineByCoverAndDis(it);
		MSElementDescrP barline = it.barline;
		MSElementDescrP path = it.path;
		//mdlElmdscr_add(barline);
		//mdlElmdscr_add(path);
	}

	if (barLinesData.size() == 0)
		return nullptr;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	double diameter = barLinesData.at(0).diameter / uor_per_mm;
	double spacing = barLinesData.at(0).spacing / uor_per_mm;
	bool isInSide = barLinesData.at(0).isInSide;
	double adjustedSpacing = spacing;

	EditElementHandle ehWall(GetSelectedElement(), GetSelectedModel());//����

	std::vector<EditElementHandle*> upflooreehs;
	std::vector<EditElementHandle*> flooreehs;
	std::vector<EditElementHandle*>Walleehs;
	std::vector<EditElementHandle*>alleehs;
	m_Around_Ele_Holeehs.clear();
	for (IDandModelref IdMode : m_walldata.floorID)//�м��
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//������ҪԪ����Χ�Ŀ׶�
			EditElementHandle aroEleeh;
			EFT::GetSolidElementAndSolidHoles(*eeh, aroEleeh, m_Around_Ele_Holeehs);

			if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
				eeh->GetElementDescrP();
				flooreehs.push_back(eeh);
				alleehs.push_back(eeh);
			}
		}
	}
	for (IDandModelref IdMode : m_walldata.downfloorID)//�װ�
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//������ҪԪ����Χ�Ŀ׶�
			EditElementHandle aroEleeh;
			EFT::GetSolidElementAndSolidHoles(*eeh, aroEleeh, m_Around_Ele_Holeehs);

			if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
				eeh->GetElementDescrP();
				//downflooreehs.push_back(eeh);
				//Walleehs.push_back(eeh);
				alleehs.push_back(eeh);
			}
		}
	}
	for (IDandModelref IdMode : m_walldata.upfloorID)//����
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//������ҪԪ����Χ�Ŀ׶�
			EditElementHandle aroEleeh;
			EFT::GetSolidElementAndSolidHoles(*eeh, aroEleeh, m_Around_Ele_Holeehs);

			if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
				eeh->GetElementDescrP();
				upflooreehs.push_back(eeh);
				//Walleehs.push_back(eeh);
				alleehs.push_back(eeh);
			}
		}
	}
	for (IDandModelref IdMode : m_walldata.wallID)//����ǽ(ֻ�ж���ê����Ҫ����ǽ)
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//������ҪԪ����Χ�Ŀ׶�
			EditElementHandle aroEleeh;
			EFT::GetSolidElementAndSolidHoles(*eeh, aroEleeh, m_Around_Ele_Holeehs);

			if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
				eeh->GetElementDescrP();
				upflooreehs.push_back(eeh);
				Walleehs.push_back(eeh);
				alleehs.push_back(eeh);
			}
		}
	}

	EditElementHandle *eeh = new EditElementHandle();//���廹��Ҫ��������
	eeh->Duplicate(ehWall);
	ISolidKernelEntityPtr entityPtr; 
	if (ehWall.IsValid() && eeh->IsValid())
	{
		if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
		{
			SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
			eeh->GetElementDescrP();
			upflooreehs.push_back(eeh);
			//Walleehs.push_back(eeh);
			//downflooreehs.push_back(eeh);
			alleehs.push_back(eeh);
		}
	}

	vector<PIT::PITRebarCurve> rebarCurves;
	for (auto it : barLinesData)
	{
		//����˲���ʽ
		double strMoveDis = GetSideCover() * uor_per_mm + diameter / 2 * uor_per_mm + it.extstrdiameter;
		double endMoveDis = GetSideCover() * uor_per_mm + diameter / 2 * uor_per_mm + it.extenddiameter;
		PIT::PITRebarEndTypes pitRebarEndTypes;
		CalRebarEndTypes(it, sizeKey, pitRebarEndTypes, modelRef);
		double extendstrDis = it.extendstrDis;
		double extendendDis = it.extendendDis;
		if (pitRebarEndTypes.beg.GetType() != PITRebarEndType::Type::kNone)
		{
			extendstrDis -= diameter / 2 * uor_per_mm;
		}
		if (pitRebarEndTypes.end.GetType() != PITRebarEndType::Type::kNone)
		{
			extendendDis -= diameter / 2 * uor_per_mm;
		}

		//����ֽ���
		vector<vector<DPoint3d>> allLines;
		//mdlElmdscr_add(it.barline);
		//mdlElmdscr_add(it.path);
		ExtractFacesTool::GetVecToCurve(allLines, it.barline, it.path, spacing,
			it.strDis / uor_per_mm, it.endDis / uor_per_mm);
		if (allLines.size() == 0)
			continue;	
		for (auto ptsIt : allLines)
		{
			makaRebarCurve(ptsIt, extendstrDis, extendendDis, diameter * uor_per_mm, strMoveDis, endMoveDis, isInSide, pitRebarEndTypes, rebarCurves, upflooreehs, flooreehs, Walleehs, alleehs);
		}
	}
	if (m_vecRebarPtsLayer.size() > 1)
	{
		for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
		{
			int n = m + 1;
			RebarPoint rbPt;
			rbPt.Layer = GetvecRebarLevel().at(m_nowlevel);
			rbPt.iIndex = m_nowlevel;
			rbPt.sec = 0;
			rbPt.vecDir = GetvecDir().at(m_nowlevel);
			rbPt.ptstr = m_vecRebarPtsLayer.at(m);
			rbPt.ptend = m_vecRebarPtsLayer.at(n);
			rbPt.DataExchange = GetvecDataExchange().at(m_nowlevel);
			g_vecRebarPtsNoHole.push_back(rbPt);
			m++;
		}
	}
	FreeAll(alleehs);
	upflooreehs.clear(); flooreehs.clear(); Walleehs.clear();
	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kBend);
	endTypeEnd.SetType(RebarEndType::kBend);

	int numRebar = rebarCurves.size();
	RebarSymbology symb;

	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);

	vector<vector<DPoint3d>> vecStartEnd;
	int index = 0;
	for (PITRebarCurve rebarCurve : rebarCurves)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (ISPointInHoles(m_useHoleehs, midPos))
		{
			if (ISPointInHoles(m_useHoleehs, ptstr) && ISPointInHoles(m_useHoleehs, ptend))
			{
				continue;
			}
		}
		if (ISPointInHoles(m_Negs, midPos))
		{
			if (ISPointInHoles(m_Negs, ptstr) && ISPointInHoles(m_Negs, ptend))
			{
				continue;
			}
		}

		vector<DPoint3d> linePts;
		RebarVertices vertices = rebarCurve.GetVertices();
		for (size_t i = 0; i < (size_t)vertices.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &vertices.At(i);
			linePts.push_back(tmpVertex->GetIP());
		}
		/*EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.AddToModel();
	*/
		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(index, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(false);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (isInSide)
			{
				Stype = "inner";
			}
			else
			{
				Stype = "out";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing, ACTIVEMODEL);

			ElementPropertiesSetterPtr propEle = ElementPropertiesSetter::Create();
			propEle->SetLinestyle(rebarLineStyle, NULL);
			propEle->SetWeight(rebarWeight);
			propEle->Apply(tmprebar);
			tmprebar.ReplaceInModel(oldref);

		}
		index++;
		vecStartEnd.push_back(linePts);
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing);
	CString spacingstring;
	spacingstring.Format(_T("%lf"), spacing);
	setdata.SetSpacingString(spacingstring);
	setdata.SetAverageSpacing(adjustedSpacing);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����
	rebarSet->SetSetData(setdata);
	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);

	return tag;
}

//ͨ���㼯�ϴ����ߴ�
MSElementDescrP GetLines(vector<DSegment3d>& lines)
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
void ExtendLineString(MSElementDescrP& linedescr, double dis)
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


void GetMovePath(MSElementDescrP& pathline, double movedis, MSElementDescrP downface)
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
//�ӳ�����ͨ�����°�
void ExtendLineByFloor(vector<MSElementDescrP>& floorfaces, vector<IDandModelref>& floorRf, DPoint3d& ptstr, DPoint3d& ptend, DPoint3d& vecLine, double thick, double& Dimr, DPoint3d vecOutwall)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	bool ishaveupfloor = false;
	bool ishavetwoside = false;

	for (int i = 0; i < floorfaces.size(); i++)
	{
		DRange3d range;
		//����ָ��Ԫ����������Ԫ�صķ�Χ��
		mdlElmdscr_computeRange(&range.low, &range.high, floorfaces.at(i), NULL);
		range.low.x = range.low.x - 1;
		range.low.y = range.low.y - 1;
		range.high.x = range.high.x + 1;
		range.high.y = range.high.y + 1;
		//PITCommonTool::CPointTool::DrowOnePoint(ptstr,1,3);//��
		//PITCommonTool::CPointTool::DrowOnePoint(ptend, 1, 3);//��
		DPoint3d temp = ptstr;//������յ��жϸ�Ϊ�е��ж�
		temp.Add(ptend);
		temp.Scale(0.5);
		temp.Add(vecLine);
		//PITCommonTool::CPointTool::DrowOnePoint(temp, 1, 3);//��
		if (range.IsContainedXY(temp) || range.IsContainedXY(ptstr) || range.IsContainedXY(ptend))
		{
			ElementId testid = 0;
			GetElementXAttribute(floorRf.at(i).ID, sizeof(ElementId), testid, ConcreteIDXAttribute, floorRf.at(i).tModel);
			std::vector<PIT::ConcreteRebar>                        test_vecRebarData1;
			std::vector<PIT::ConcreteRebar>                        test_vecRebarData2;
			GetElementXAttribute(testid, test_vecRebarData1, RebarInsideFace, ACTIVEMODEL);
			GetElementXAttribute(testid, test_vecRebarData2, RebarOutsideFace, ACTIVEMODEL);

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
		DPoint3d movept = ptstr;
		vecOutwall.Scale(thick*uor_per_mm / 2);
		movept.Add(vecOutwall);
		vecLine.Scale(0.1);
		movept.Add(vecLine);
		//PITCommonTool::CPointTool::DrowOnePoint(movept, 1, 4);//��
		for (int i = 0; i < floorfaces.size(); i++)
		{
			DRange3d range;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			mdlElmdscr_computeRange(&range.low, &range.high, floorfaces.at(i), NULL);
			EditElementHandle shapeEeh(floorfaces.at(i), false, false, ACTIVEMODEL); //��������Ƿ���棬����ʱ����isClosed������Ϊtrue���Ƿ����
			CurveVectorPtr curvePtr = ICurvePathQuery::ElementToCurveVector(shapeEeh); //ת����
			CurveVector::InOutClassification pos1 = curvePtr->PointInOnOutXY(movept); //�����Ĺ�ϵ
			range.low.x = range.low.x - 1;
			range.low.y = range.low.y - 1;
			range.high.x = range.high.x + 1;
			range.high.y = range.high.y + 1;

			//���Դ�����ʾ��ǰ���жϵ��λ��
            PITCommonTool::CPointTool::DrowOnePoint(movept, 1, 1);//��
            PITCommonTool::CPointTool::DrowOnePoint(ptstr, 1, 2);//��
			if (range.IsContainedXY(movept) && range.IsContainedXY(ptstr))//CurveVector::INOUT_On == pos1 || CurveVector::INOUT_In == pos1)//range.IsContainedXY(movept)
			{
				ishavetwoside = true;//�ڲ���
				break;
			}
		}
		if (!ishavetwoside)//�����
		{
			Dpoint3d vectmp = ptstr - ptend;
			vectmp.Normalize();
			vectmp.Scale(thick*uor_per_mm);
			ptstr.Add(vectmp);
			//PITCommonTool::CPointTool::DrowOnePoint(ptstr, 1, 3);//��
		}

	}
}
//���㵱ǰ��ʼ���Ƿ��а壬����������ڲ໹�������
bool CalculateBarLineDataByFloor(vector<MSElementDescrP>& floorfaces, vector<IDandModelref>& floorRf, DPoint3d& ptstr, DPoint3d& ptend, DPoint3d& vecLine, double thick,
	DPoint3d vecOutwall, bool& isInside, double& diameter)
{
	DPoint3d zeropt = DPoint3d::From(0, 0, 0);
	DPoint3d tmpvecZ = DPoint3d::From(0, 0, 1);
	Transform tran;			//����ͶӰ����
	mdlTMatrix_computeFlattenTransform(&tran, &zeropt, &tmpvecZ);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	bool ishaveupfloor = false;//�Ƿ��а�
	bool ishavetwoside = false;
	vector<PIT::ConcreteRebar> vecRebar;
	std::vector<PIT::ConcreteRebar> test_vecRebarData1;
	std::vector<PIT::ConcreteRebar> test_vecRebarData2;
	for (int i = 0; i < floorfaces.size(); i++)
	{
		DRange3d range;
		//����ָ��Ԫ����������Ԫ�صķ�Χ��
		mdlElmdscr_computeRange(&range.low, &range.high, floorfaces.at(i), NULL);
		range.low.x = range.low.x - 1;
		range.low.y = range.low.y - 1;
		range.high.x = range.high.x + 1;
		range.high.y = range.high.y + 1;
		DPoint3d pointStr = ptstr;//��Ϊ�е��ж�
		pointStr.Add(ptend);
		pointStr.Scale(0.5);
		pointStr.Add(vecLine);
		//���Դ�����ʾ��ǰ���жϵ��λ��
		//PITCommonTool::CPointTool::DrowOnePoint(pointStr, 1, 1);
		//PITCommonTool::CPointTool::DrowOnePoint(range.high, 1, 1);
		//PITCommonTool::CPointTool::DrowOnePoint(range.low, 1, 1);

		if (range.IsContainedXY(pointStr))// && range.IsContainedXY(ptend)
		{
			ElementId testid = 0;
			GetElementXAttribute(floorRf.at(i).ID, sizeof(ElementId), testid, ConcreteIDXAttribute, floorRf.at(i).tModel);
			GetElementXAttribute(testid, test_vecRebarData1, RebarInsideFace, ACTIVEMODEL);
			GetElementXAttribute(testid, test_vecRebarData2, RebarOutsideFace, ACTIVEMODEL);

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
			if (test_vecRebarData1.size() != 0 && test_vecRebarData2.size() != 0 && vecRebar.size() == 2)
			{
				vecRebar.clear();
				vecRebar.push_back(test_vecRebarData1[0]);
				vecRebar.push_back(test_vecRebarData1[1]);
				vecRebar.push_back(test_vecRebarData2[0]);
				vecRebar.push_back(test_vecRebarData2[1]);
			}
			break;
		}
	}
	if (ishaveupfloor)//����а���п���Ҫ��������
	{
		DPoint3d movept = ptstr;
		vecOutwall.Scale(thick*uor_per_mm / 2);
		movept.Add(vecOutwall);
		movept.z = 0;
		vecLine.z = 0;
		movept.Add(vecLine);

		DPoint3d jude_pt = ptstr;//ȡ2/3
		jude_pt.Add(vecLine);
		jude_pt.z = 0;

		for (int i = 0; i < floorfaces.size(); i++)
		{
			//mdlElmdscr_add(floorfaces.at(i));
			DRange3d range;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			mdlElmdscr_computeRange(&range.low, &range.high, floorfaces.at(i), NULL);
			range.low.x = range.low.x - 10;
			range.low.y = range.low.y - 10;
			range.high.x = range.high.x + 10;
			range.high.y = range.high.y + 10;
			if (range.IsContainedXY(movept))
			{
				//����жϵ���range�ڣ����жϴ��������Ƿ��н���(����ͶӰ��XOYƽ�棬���ж�)
				MSElementDescrP cdescr = nullptr;
				mdlElmdscr_duplicate(&cdescr, floorfaces.at(i));
				//����ͶӰ��XOYƽ��
				mdlElmdscr_transform(&cdescr, &tran);
				EditElementHandle teeh(cdescr, true, false, ACTIVEMODEL);

				auto is_InElement = ISPointInElement(&teeh, movept);
				auto is_InElement2 = ISPointInElement(&teeh, jude_pt);

				if (is_InElement && is_InElement2)
				{
					ishavetwoside = true;
				}
			}
		}
		if (!ishavetwoside)//�����
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
			if (!vecRebar.empty() && vecRebar.at(0).rebarDir == WallDir && test_vecRebarData1.empty())
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
void GetCutPathLines(vector<WallRebarAssembly::BarLinesdata>& barlines, double sidespacing, double diameter,
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
double  GetExtendptByWalls(DPoint3d& str, DPoint3d str_1, double thick, MSElementDescrP& Wallfaces, vector<MSElementDescrP>& cutWallfaces,
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
	int FALG = 1;
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
		vector <DPoint3d>Mvec;//���귽��
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
					Mvec.push_back(MGvec);
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
					Mvec.push_back(MGvec);
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
void STWallRebarAssembly::CalRebarEndTypes(const BarLinesdata & data, BrStringCR sizeKey, PIT::PITRebarEndTypes & pitRebarEndTypes, DgnModelRefP modelRef)
{
	//����ê�뷽����ֽ��ļн���˲�����
	auto GetEndType = [&](const CVector3D& vec1, const CVector3D& vec2, PIT::PITRebarEndType& endType)->int {
		double angle = vec1.AngleTo(vec2);
		endType.Setangle(angle);
		if (COMPARE_VALUES_EPS(angle, PI / 2, 0.1) == 0) //90��
			return PIT::PITRebarEndType::Type::kBend;
		if (COMPARE_VALUES_EPS(angle, 3 / 4 * PI, 0.1) == 0) //135��
			return PIT::PITRebarEndType::Type::kCog;
		if (COMPARE_VALUES_EPS(angle, 0, 0.1) == 0) //180����ê
			return PIT::PITRebarEndType::Type::kHook;
		if (COMPARE_VALUES_EPS(angle, PI, 0.1) == 0) //ֱê
			return PIT::PITRebarEndType::Type::kLap;
		else
			return PIT::PITRebarEndType::Type::kCustom;
	};

	CVector3D strVec = data.vecstr;
	CVector3D endVec = data.vecend;
	CVector3D HolVec = data.vecHoleehs;
	strVec.Normalize();
	endVec.Normalize();
	//��ȡê�����
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	//Ĭ��û�ж˲���ʽ
	PIT::PITRebarEndType strEndType, endEndType, HolEndType;
	strEndType.SetType(PIT::PITRebarEndType::Type::kNone);
	//����ֽ�����β
	DPoint3d lineStrPt = { 0,0,0 }, lineEndPt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&lineStrPt, nullptr, &lineEndPt, nullptr, data.barline, modelRef);
	//������β�˲���ʽ
	if (!strVec.IsEqual(Dpoint3d::FromZero()))
	{
		CVector3D lineVec = lineEndPt - lineStrPt;
		lineVec.Normalize();
		int endType = GetEndType(strVec, lineVec, strEndType);
		strEndType.SetType(PIT::PITRebarEndType::Type(endType));
		strEndType.SetbendLen(data.strMG);// -bendRadius - diameter / 2
		strEndType.SetbendRadius(bendRadius);
		strEndType.SetendNormal(strVec);
	}
	if (!endVec.IsEqual(Dpoint3d::FromZero()))
	{
		CVector3D lineVec = lineStrPt - lineEndPt;
		lineVec.Normalize();
		int endType = GetEndType(endVec, lineVec, endEndType);
		endEndType.SetType(PIT::PITRebarEndType::Type(endType));
		endEndType.SetbendLen(data.endMG); //-bendRadius - diameter / 2
		endEndType.SetbendRadius(bendRadius);
		endEndType.SetendNormal(endVec);
	}
	if (!HolVec.IsEqual(Dpoint3d::FromZero()))
	{
		CVector3D lineVec = lineEndPt - lineStrPt;
		lineVec.Normalize();
		int endType = GetEndType(HolVec, lineVec, HolEndType);
		HolEndType.SetType(PIT::PITRebarEndType::Type(endType));
		HolEndType.SetbendLen(data.holMG - bendRadius - diameter / 2);
		HolEndType.SetbendRadius(bendRadius);
		HolEndType.SetendNormal(HolVec);
	}
	Hol = HolEndType;
	pitRebarEndTypes = { strEndType,endEndType };
}

void interPtsSort(vector<DPoint3d> &interPts, const DPoint3d &originPt)
{
	std::sort(interPts.begin(), interPts.end(), [&](const DPoint3d& pt1, const DPoint3d& pt2) {
		double dis1 = originPt.Distance(pt1);
		double dis2 = originPt.Distance(pt2);
		return COMPARE_VALUES_EPS(dis1, dis2, 1e-6) == 1;
	});
}

/*
* @desc:		���ݶ��װ����¼�����������
* @param[in]	strPt �ֽ������
* @param[in]	endPt �ֽ����ص�
* @param[in]	strMoveLen �����ƶ����룬�����乳�Ǳ����㣬���乳�Ǳ�����+�ֽ�뾶
* @param[in]	endMoveLen �յ���ƶ����룬�����乳�Ǳ����㣬���乳�Ǳ�����+�ֽ�뾶
* @param[out]	extendStrDis �����������
* @param[out]	extendEndDis �յ���������
* @author	Hong ZhuoHui
* @Date:	2023/09/20
*/
void STWallRebarAssembly::ReCalExtendDisByTopDownFloor(const DPoint3d & strPt, const DPoint3d & endPt, double strMoveLen,
	double endMoveLen, double & extendStrDis, double & extendEndDis, bool isInSide)
{
	//��ȡǽ��
	//extendStrDis = 0; extendEndDis = 0;
	EditElementHandle wallEeh(GetSelectedElement(), GetSelectedModel());
	if (!wallEeh.IsValid())
		return;
	DPoint3d wallLowPt = { 0,0,0 }, wallHighPt = { 0,0,0 };
	mdlElmdscr_computeRange(&wallLowPt, &wallHighPt, wallEeh.GetElementDescrP(), nullptr);
	vector<DPoint3d> Pts;
	DVec3d strVec1 = strPt - endPt;
	strVec1.Normalize();
	strVec1.ScaleToLength(40 * UOR_PER_MilliMeter);
	DVec3d endVec1 = endPt - strPt;
	endVec1.Normalize();
	endVec1.ScaleToLength(40 * UOR_PER_MilliMeter);
	DPoint3d Pt1 = strPt, Pt2 = endPt;
	Pt1.Add(strVec1);
	Pt2.Add(endVec1);
	//PITCommonTool::CPointTool::DrowOneLine(strPt, endPt, 2);
	GetIntersectPointsWithOldElm(Pts, &wallEeh, Pt1, Pt2);
	//PITCommonTool::CPointTool::DrowOnePoint(Pts[0], 1, 6);
	//PITCommonTool::CPointTool::DrowOnePoint(Pts[1], 1, 1);
	//1.������С�ĵ�����ĵ�
	Dpoint3d minPt = { INT_MAX, INT_MAX, INT_MAX }, maxPt = { INT_MIN, INT_MIN, INT_MIN };
	auto calMinMaxPt = [&](IDandModelref floor) {
		EditElementHandle floorEeh(floor.ID, floor.tModel);
		if (!floorEeh.IsValid())
			return;
		DPoint3d lowPt = { 0,0,0 }, highPt = { 0,0,0 };
		mdlElmdscr_computeRange(&lowPt, &highPt, floorEeh.GetElementDescrP(), nullptr);
		if (COMPARE_VALUES_EPS(lowPt.x, minPt.x, 1e-6) == -1)
			minPt.x = lowPt.x;
		if (COMPARE_VALUES_EPS(lowPt.y, minPt.y, 1e-6) == -1)
			minPt.y = lowPt.y;
		if (COMPARE_VALUES_EPS(lowPt.z, minPt.z, 1e-6) == -1)
			minPt.z = lowPt.z;
		if (COMPARE_VALUES_EPS(highPt.x, maxPt.x, 1e-6) == 1)
			maxPt.x = highPt.x;
		if (COMPARE_VALUES_EPS(highPt.y, maxPt.y, 1e-6) == 1)
			maxPt.y = highPt.y;
		if (COMPARE_VALUES_EPS(highPt.z, maxPt.z, 1e-6) == 1)
			maxPt.z = highPt.z;
	};
	for (auto it : m_walldata.upfloorID)
	{
		calMinMaxPt(it);
	}
	for (auto it : m_walldata.downfloorID)
	{
		calMinMaxPt(it);
	}

	//2.����С������ͶӰ���ֽ�����
	DPoint3d minProPt = minPt, maxProPt = maxPt;
	mdlVec_projectPointToLine(&minProPt, nullptr, &minPt, &strPt, &endPt);
	mdlVec_projectPointToLine(&maxProPt, nullptr, &maxPt, &strPt, &endPt);

	//3.���ݵ�ͶӰ��ľ������ֽ����ӳ�����
	double strMinDis = strPt.Distance(minProPt);
	double endMinDis = endPt.Distance(minProPt);
	double strMaxDis = strPt.Distance(maxProPt);
	double endMaxDis = endPt.Distance(maxProPt);
	double strMoveDis = 0, endMoveDis = 0;
	if (COMPARE_VALUES_EPS(strMinDis, endMinDis, 1e-6) == -1)
	{
		strMoveDis = strMinDis;
		endMoveDis = endMaxDis;
	}
	else
	{
		strMoveDis = strMaxDis;
		endMoveDis = endMinDis;
	}

	//4.�ӳ��ֽ���
	DPoint3d newStrPt = strPt, newEndPt = endPt;
	//4.1������β�ƶ�����
	DVec3d strVec = strPt - endPt;
	strVec.Normalize();
	strVec.ScaleToLength(strMoveDis + strMoveLen); //movelen�������������ģ���Ϊ�ֽ�����б�ģ�ͶӰ���е����
	DVec3d endVec = endPt - strPt;
	endVec.Normalize();
	endVec.ScaleToLength(endMoveDis + endMoveLen);
	//4.2�ƶ���β��
	newStrPt.Add(strVec*1.5);
	newEndPt.Add(endVec*1.5);
	//PITCommonTool::CPointTool::DrowOneLine(newStrPt, newEndPt, 11);
	//5.�����а��󽻣��õ���ʼ��˵Ľ�����յ�˵Ľ��㼯��
	vector<DPoint3d> interStrPts, interEndPts, allPts; //����
	int  FLAG = 0;
	bool isStrRecorded = false; // ��־����Ƿ�������
	bool isEndRecorded = false; // ��־�յ��Ƿ�������
	auto calInterPts = [&](IDandModelref floor, bool islsay = true) {
		EditElementHandle floorEeh(floor.ID, floor.tModel);
		if (!floorEeh.IsValid())
			return;

		//5.1.1 ���˲���ǽ��Χ�ڵİ�		
		DPoint3d lowPt = { 0,0,0 }, highPt = { 0,0,0 };
		mdlElmdscr_computeRange(&lowPt, &highPt, floorEeh.GetElementDescrP(), nullptr);
		DRange3d  vecRange;
		vecRange.low = wallLowPt;
		vecRange.high = wallHighPt;
		strVec.Normalize();
		int strRecord = 2; // ������Ľ������
		int endRecord = 2; // ������Ľ������
		if (islsay)
		{
			if ((COMPARE_VALUES_EPS(lowPt.z, wallLowPt.z - 50 * UOR_PER_MilliMeter, 1e-6) == 1 && //����͵���ǽ֮��
				COMPARE_VALUES_EPS(highPt.z, wallHighPt.z + 50 * UOR_PER_MilliMeter, 1e-6) == -1) &&
				COMPARE_VALUES_EPS(wallHighPt.z, highPt.z, 1e-6) == 0) //����ߵ���ǽ֮��
			{
				if (vecRange.IsContainedXY(lowPt) && wallLowPt.DistanceXY(lowPt) > 10 && COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0) //����͵㲻��ǽ��Χ,����ߵ㲻��ǽ��Χ
				{
					return;
				}

				if (vecRange.IsContainedXY(highPt) && wallHighPt.DistanceXY(highPt) > 10 && COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0) //����͵㲻��ǽ��Χ,����ߵ㲻��ǽ��Χ
				{
					return;
				}
			}
			MSElementDescrP is_lsay = nullptr;
			if (!PITCommonTool::CSolidTool::SolidBoolWithFace(is_lsay, wallEeh.GetElementDescrP(), floorEeh.GetElementDescrP(), BOOLOPERATION::BOOLOPERATION_INTERSECT)
				&& COMPARE_VALUES_EPS(lowPt.z, wallHighPt.z - 50 * UOR_PER_MilliMeter, 1e-6) == -1
				&& COMPARE_VALUES_EPS(highPt.z, wallHighPt.z, 1e-6) == 1
				&& highPt.z - wallHighPt.z < 55 * UOR_PER_MilliMeter)//��������͵���ǽ֮�䵫�����ߵ㲻��ǽ֮���Ұ��ǽ�����
			{
				return;
			}
		}
		/*PITCommonTool::CPointTool::DrowOnePoint(wallLowPt, 1, 5);
		PITCommonTool::CPointTool::DrowOnePoint(wallHighPt, 1, 6);
		PITCommonTool::CPointTool::DrowOnePoint(lowPt, 1, 1);
		PITCommonTool::CPointTool::DrowOnePoint(highPt, 1, 2);*/


		if ((COMPARE_VALUES_EPS(lowPt.z, wallLowPt.z + 80 * UOR_PER_MilliMeter, 1e-6) == -1 ||
			COMPARE_VALUES_EPS(lowPt.z, wallHighPt.z - 80 * UOR_PER_MilliMeter, 1e-6) == 1) && //����͵㲻��ǽ֮��
			(COMPARE_VALUES_EPS(highPt.z, wallLowPt.z + 80 * UOR_PER_MilliMeter, 1e-6) == -1 ||
				COMPARE_VALUES_EPS(highPt.z, wallHighPt.z - 80 * UOR_PER_MilliMeter, 1e-6) == 1) &&//����ߵ㲻��ǽ֮��
			COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0)//�ֽ��߷���Z��Ϊ1
		{

			//5.1.1 �õ������İ�Χ��ͶӰ���ֽ����ϵĵ�
			DPoint3d floorMinProPt = lowPt, floorMaxProPt = highPt;
			mdlVec_projectPointToLine(&floorMinProPt, nullptr, &lowPt, &strPt, &endPt);
			mdlVec_projectPointToLine(&floorMaxProPt, nullptr, &highPt, &strPt, &endPt);
			//5.1.2 ������ߵķ���͸ֽ��߷����ϵ���õ������ڸֽ��߷����ϵ���ʼ���յ�
			DPoint3d floorStrPt = floorMinProPt, floorEndPt = floorMaxProPt;
			DVec3d vec1 = strPt - endPt; vec1.Normalize();
			DVec3d vec2 = floorMinProPt - floorMaxProPt; vec2.Normalize();
			if (vec1.DotProduct(vec2) < 0) //��ͬ����
			{
				floorStrPt = floorMaxProPt;
				floorEndPt = floorMinProPt;
			}
			//5.1.3 ������ߵ���ʼ��͸ֽ��ߵ��յ�ľ��� ���� ���ߵ��յ�͸ֽ��ߵ����ľ��� С�ڸֽ��ߵ�1/5 ���˰�
			double dis = strPt.Distance(endPt) / 3;
			if (COMPARE_VALUES_EPS(floorStrPt.Distance(endPt), dis, 1e-6) == -1 ||
				COMPARE_VALUES_EPS(floorEndPt.Distance(strPt), dis, 1e-6) == -1)
			{
				return;
			}
		}
		//5.2 ������彻��
		vector<DPoint3d> interPts;
		//PITCommonTool::CPointTool::DrowOneLine(newStrPt, newEndPt, 1);
		//GetIntersectPointsWithOldElm(interPts, &floorEeh, newStrPt, newEndPt);
		//5.1.2 ���˲���ǽ��Χ�ڵ�ǽ
	    //����ָ��Ԫ����������Ԫ�صķ�Χ��
		DPoint3d range_data_low = { 0,0,0 }, range_data_high = { 0,0,0 };
		EditElementHandle wallEeh(GetSelectedElement(), GetSelectedModel());//��ȡѡ��model�Ĳ�����Χ
		DPoint3d wallLowPt = { 0,0,0 }, wallHighPt = { 0,0,0 };
		mdlElmdscr_computeRange(&wallLowPt, &wallHighPt, wallEeh.GetElementDescrP(), nullptr);
		for (int i = 0; i < m_walldata.cutWallfaces.size(); i++)//����Χ��ǽ�ĵ������ɸѡ
		{
			DPoint3d wallLow_Pt = { 0,0,0 }, wallHigh_Pt = { 0,0,0 };
			EditElementHandle wall_Eeh(m_walldata.wallID[i].ID, m_walldata.wallID[i].tModel);
			mdlElmdscr_computeRange(&wallLow_Pt, &wallHigh_Pt, wall_Eeh.GetElementDescrP(), nullptr);//��ȡѡ����Χǽ�Ĳ�����Χ

			if (wallLow_Pt.z > wallLowPt.z && wallHigh_Pt.z < wallHighPt.z)//�����Χǽ��ѡ��ǽ��range��Χ��
			{
				range_data_low = wallLow_Pt;
				range_data_high= wallHigh_Pt;
			}
			//�ж��Ƿ���Ҫ�����жϣ������Χǽ��ѡ��ǽ��range��Χ�ڣ�����Χ�İ�ֻ��Ҫê��һ�Σ������Զ��ê��
			if ( (FLAG != 2 && (strPt.z > range_data_low.z && endPt.z > range_data_low.z) && (strPt.z < range_data_high.z && endPt.z < range_data_high.z)) ||
				 (FLAG != 2 && (strPt.x > range_data_low.x && endPt.x > range_data_low.x) && (strPt.x < range_data_high.x && endPt.x < range_data_high.x)) )
			{
				FLAG = 1;
			}
		}
		if (FLAG == 1)//����ê��
		{
			GetIntersectPointsWithHole(interPts, &floorEeh, newStrPt, newEndPt);
			FLAG = 2;
		}
		if(FLAG == 0)//���Զ��ê��
		{
			GetIntersectPointsWithHole(interPts, &floorEeh, newStrPt, newEndPt);
			//PITCommonTool::CPointTool::DrowOnePoint(newStrPt, 1, 3);//��
			//PITCommonTool::CPointTool::DrowOnePoint(newEndPt, 1, 4);//��
		} 
		//5.3 �������н��㣬���������һ����ֽ��߶˵㼫���ĵ㣬���������壨ǽ��֮��û�н���
		bool isValid = false;
		if (Pts.size() == 0)//�����ǽû�н���˵�������겿�֣�����Ҫ���й���
		{
			isValid = true;
		}
		else
		{
			for (auto it : interPts)
			{
				//PITCommonTool::CPointTool::DrowOnePoint(it, 1, 3);
				double strDis = strPt.Distance(it);
				double endDis = endPt.Distance(it);
				double tatalDis = strPt.Distance(endPt);
				if (COMPARE_VALUES_EPS(strDis, 50 * UOR_PER_MilliMeter, 1e-6) <= 0 || //��ʼ����˵㼫��
					COMPARE_VALUES_EPS(endDis, 50 * UOR_PER_MilliMeter, 1e-6) <= 0 ||	//�յ���˵㼫��
					COMPARE_VALUES_EPS(tatalDis, strDis + endDis, 1) == 0 ||//�����ڸֽ����м�
					vecRange.IsContainedXY(it))//����ǽXYƽ��ķ�Χ��
				{
					isValid = true;
					break;
				}
				// ������������齻���Ƿ�ӽ� interStrPts �� interEndPts����Ϊ�ж�ê����Ƿ���Կ�ʵ��
				// ��齻���Ƿ�ӽ����˵Ľ��㼯��
				for (auto strPtInters : interStrPts) {
					if (islsay && COMPARE_VALUES_EPS(strPtInters.Distance(it), 50 * UOR_PER_MilliMeter, 1e-6) <= 0) {
						isValid = true;
						break;
					}
				}

				if (isValid) {
					break;  // �����ȷ���߿������죬����ѭ��
				}

				// ��齻���Ƿ�ӽ��յ�˵Ľ��㼯��
				for (auto endPtInters : interEndPts) {
					if (islsay && COMPARE_VALUES_EPS(endPtInters.Distance(it), 50 * UOR_PER_MilliMeter, 1e-6) <= 0) {
						isValid = true;
						break;
					}
				}

				if (isValid) {
					break;  // �����ȷ���߿������죬����ѭ��
				}
			}
		}
		if (!isValid)
			return;
		endVec.Normalize();
		//5.4 ��¼��
		if (Pts.size() == 0 && COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0)
		{
			for (int i = 0; i <= (int)interPts.size() - 1; i++)
			{
				for (int j = 0; j < (int)interPts.size() - 1 - i; j++)
				{
					CVector3D vec = CVector3D::From(interPts[j + 1].x - interPts[j].x, interPts[j + 1].y - interPts[j].y, interPts[j + 1].z - interPts[j].z);
					vec.Normalize();
					if (!(COMPARE_VALUES_EPS(endVec.x, vec.x, 0.1) == 0 &&
						COMPARE_VALUES_EPS(endVec.y, vec.y, 0.1) == 0 &&
						COMPARE_VALUES_EPS(endVec.z, vec.z, 0.1) == 0))
					{
						DPoint3d Tmp = interPts[j];
						interPts[j] = interPts[j + 1];
						interPts[j + 1] = Tmp;
					}
				}
			}
			for (int i = 0; i <= (int)interPts.size() - 1; i++)
			{
				if (((i + 1) & 0x01))
					interStrPts.push_back(interPts[i]);
				else
					interEndPts.push_back(interPts[i]);
			}
		}
		else
		{
			for (auto it : interPts)
			{
				double strDis = strPt.Distance(it);
				double endDis = endPt.Distance(it);
				allPts.push_back(it);
				if (COMPARE_VALUES_EPS(strDis, endDis, 1e-6) == -1)
				{
					if (!islsay)
					{
						if (!isStrRecorded)//ǽʵ����δ�������壬��������
							interStrPts.push_back(it);
						continue;
					}
					if (interStrPts.size() < 2 || strRecord == 1)//���������壬���°����һ�����㣬�������죬���¼Ϊ��
					{
						interStrPts.push_back(it);
						isStrRecorded = true;
						continue;
					}
					if (!isStrRecorded)//δ�������壬��������
					{
						if (strRecord == 2)//ǽ��ǽ˳����Ҫ��������������㣬�����¼Ϊ��
							strRecord--;
						interStrPts.push_back(it);
						isStrRecorded = true;
						continue;
					}
					// ��������壬ѡ����İ�
					interPtsSort(interStrPts, endPt);
					double tempStrDis = strPt.Distance(*interStrPts.end());
					if (COMPARE_VALUES_EPS(strDis, tempStrDis, 1e-6) == -1)//�µĽ����ڸ����İ�
					{
						if(strRecord == 2)//����������ʱ��գ���Ҫ�����°����������
							interStrPts.clear();
						interStrPts.push_back(it);
						strRecord--;
					}
				}
				else
				{
					if (!islsay)
					{
						if (!isEndRecorded)//ǽʵ����δ�������壬��������
							interEndPts.push_back(it);
						continue;
					}
					if (interEndPts.size() < 2 || endRecord == 1)//���������壬���°����һ�����㣬�������죬���¼Ϊ��
					{
						interEndPts.push_back(it);
						isEndRecorded = true;
						continue;
					}
					if (!isEndRecorded)//δ�������壬��������
					{
						if (endRecord == 2)//ǽ��ǽ˳����Ҫ��������������㣬�����¼Ϊ��
							endRecord--;
						interEndPts.push_back(it);
						isEndRecorded = true;
						continue;
					}
					// ��������壬ѡ����İ�
					interPtsSort(interEndPts, strPt);
					double tempEndDis = endPt.Distance(*interEndPts.end());
					if (COMPARE_VALUES_EPS(endDis, tempEndDis, 1e-6) == 1)//�µĽ����ڸ����İ�
					{
						if (endRecord == 2)//����������ʱ��գ���Ҫ�����°����������
							interEndPts.clear();
						interEndPts.push_back(it);
						endRecord--;
					}
				}
					
			}

		}

	};
	strVec.Normalize();
	for (auto it : m_walldata.upfloorID)
	{
		calInterPts(it);
	}
	for (auto it : m_walldata.downfloorID)
	{
		calInterPts(it);
	}
	if (!((COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) == 0) && isInSide))//���˵������ڲ�ֽ�
	{
		for (auto it : m_walldata.wallID)
		{
			calInterPts(it, false);
		}
	}
	if (!((COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0) && isInSide))//����ˮƽ���ڲ�ֽ�
	{
		for (auto it : m_walldata.floorID)
		{
			calInterPts(it, false);
		}
	}

	//ǽ����ܻ���Ҫ��������
	for (auto it : m_walldata.upfloorID)
	{
		calInterPts(it);
	}
	for (auto it : m_walldata.downfloorID)
	{
		calInterPts(it);
	}

	interPtsSort(interStrPts, endPt);
	interPtsSort(interEndPts, strPt);
	
	//7.��������ֵ
	if (interStrPts.size() > 0)
	{
		//PITCommonTool::CPointTool::DrowOnePoint(*interStrPts.begin(), 1, 3);
		extendStrDis = strPt.Distance(*interStrPts.begin());
		//���ݷ�������������
		strVec.Normalize();
		DVec3d extendStrVec = *interStrPts.begin() - strPt;
		extendStrVec.Normalize();
		if (extendStrVec.DotProduct(strVec) < 0) //������
			extendStrDis = extendStrDis * -1;
		//����������
		extendStrDis -= strMoveLen;
	}


	if (interEndPts.size() > 0)
	{
		//PITCommonTool::CPointTool::DrowOnePoint(*interEndPts.begin(), 1, 4);
		extendEndDis = endPt.Distance(*interEndPts.begin());
		//���ݷ�������������
		endVec.Normalize();
		DVec3d extendEndVec = *interEndPts.begin() - endPt;
		extendEndVec.Normalize();
		if (extendEndVec.DotProduct(endVec) < 0) //������
			extendEndDis = extendEndDis * -1;
		//����������
		extendEndDis -= endMoveLen;
	}


}

//��ȡê�̳���
double STWallRebarAssembly::get_lae() const
{
	if (g_globalpara.m_alength.find("A") != g_globalpara.m_alength.end())
	{
		auto iter = g_globalpara.m_alength.find("A");
		return iter->second * UOR_PER_MilliMeter;
	}
	else if (g_globalpara.m_alength.find("B") != g_globalpara.m_alength.end())
	{
		auto iter = g_globalpara.m_alength.find("B");
		return iter->second * UOR_PER_MilliMeter;
	}
	else if (g_globalpara.m_alength.find("C") != g_globalpara.m_alength.end())
	{
		auto iter = g_globalpara.m_alength.find("C");
		return iter->second * UOR_PER_MilliMeter;
	}
	else if (g_globalpara.m_alength.find("D") != g_globalpara.m_alength.end())
	{
		auto iter = g_globalpara.m_alength.find("D");
		return iter->second * UOR_PER_MilliMeter;
	}
	else
	{
		return -1.0;
	}
}


void STWallRebarAssembly::CalculateLeftRightBarLines(vector<BarLinesdata>& barlines, double fdiam, double allfdiam, DPoint3dR vecHoleehs, MSElementDescrP& path, MSElementDescrP& barline,
	int side, int index)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	WString strSize = GetvecDirSize().at(index);
	if (strSize.find(L"mm") != WString::npos)
	{
		strSize.ReplaceAll(L"mm", L"");
	}
	vector<MSElementDescrP>allpaths;
	double diameter1 = stoi(strSize.GetWCharCP())*uor_per_mm;
	double diameter = RebarCode::GetBarDiameter(strSize, ACTIVEMODEL);		//������10
	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;

	DPoint3d vecOutwall = m_walldata.vecToWall;//Զ��ǽ�ڵ�ƫ�Ʒ���
	vecOutwall.Negate();
	if (side != 0) vecOutwall.Negate();
	DPoint3d pt11, pt22;
	mdlElmdscr_extractEndPoints(&pt11, nullptr, &pt22, nullptr, barline, ACTIVEMODEL);
	double lengh = pt11.Distance(pt22) / 3;//�ƶ���1/3�ĵ�
	DPoint3d vecLine = pt22 - pt11;//��ʼ�ߴ�������
	vecLine.Normalize();
	vecLine.Scale(lengh);//�ӳ�
	//1������·���߳���
	//�����ж��ϲ���û�а壬�а�Ļ��Ƿ����Ҳ඼�ڰ�ķ�Χ�ڣ��������˵��Ϊ�ڲ��棨���Բ����ӳ�·���ߣ�
	//���ֻ��һ��˵��Ϊ����棬��Ҫ�ӳ�
	DPoint3d pt1, pt2;
	mdlElmdscr_extractEndPoints(&pt1, nullptr, &pt2, nullptr, path, ACTIVEMODEL);
	DPoint3d Tmept1 = pt1, Tmeppt2 = pt2;
	double D1 = 0, D2 = 0;
	ExtendLineByFloor(m_walldata.upfloorfaces, m_walldata.upfloorID, pt2, pt1, vecLine, m_walldata.upfloorth - diameter / uor_per_mm, D1, vecOutwall);
	ExtendLineByFloor(m_walldata.downfloorfaces, m_walldata.downfloorID, pt1, pt2, vecLine, m_walldata.downfloorth - diameter / uor_per_mm, D2, vecOutwall);


	if (Tmept1.Distance(pt1) > 1)//�°�
	{
		DPoint3d temp = Tmept1;
		pt1.z -= D2;
		temp.z = temp.z - diameter + D2;
		EditElementHandle tmpeeh;
		LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(pt1, temp), true, *ACTIVEMODEL);
		MSElementDescrP tmppath = tmpeeh.ExtractElementDescr();
		allpaths.push_back(tmppath);
	}
	//�м�
	EditElementHandle tmpeeh;
	LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(Tmept1, Tmeppt2), true, *ACTIVEMODEL);
	MSElementDescrP tmppath = tmpeeh.ExtractElementDescr();
	allpaths.push_back(tmppath);

	if (Tmeppt2.Distance(pt2) > 1)//�ϰ�
	{
		DPoint3d temp = Tmeppt2;
		pt2.z += D1;
		temp.z = temp.z + diameter - D1;
		EditElementHandle tmpeeh;
		LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(temp, pt2), true, *ACTIVEMODEL);
		MSElementDescrP tmppath = tmpeeh.ExtractElementDescr();
		allpaths.push_back(tmppath);
	}
	//tmpeeh.AddToModel();
	//2������ֽ��߳��Ⱥ�ê�̳��ȡ�ê�̷���
	//��ֽ�����ǽ�潻�㣬
	//(1)����н��㣬ֱ���ӳ��ֽ��ߵ����㴦
	DPoint3d facenormal;
	DPoint3d maxpt;
	mdlElmdscr_extractNormal(&facenormal, &maxpt, m_walldata.downFace, NULL);
	DPoint3d cpt = getCenterOfElmdescr(m_walldata.downFace);
	facenormal.Normalize();
	DPlane3d plane = DPlane3d::FromOriginAndNormal(cpt, DVec3d::From(facenormal.x, facenormal.y, facenormal.z));
	vector<DPoint3d> barpts;//ȡ�����иֽ��ߵ㼯��
	ExtractLineStringPoint(barline, barpts);

	//double L0 = 24 * uor_per_mm * 15;//15d
	//double Lae = 24 * uor_per_mm * 48 * 0.8;//48d
	double L0 = diameter1 * 15;//15d
	double Lae = diameter1 * get_lae() * 0.8 / uor_per_mm * 2;//LAE
	double mgDisHol = 0;
	//if (GetvecDataExchange().at(index) == 0)//�����
	//{
	//	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - 2 * diameter;
	//}
	//else if (GetvecDataExchange().at(index) == 2)//�ڲ���
	//{
	//	
	//}

	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - diameter;
	for (auto it : allpaths)
	{
		if (barpts.size() > 1)
		{//��ʼ���¼���ֽ�����ʼ��
			DPoint3d ptstrtmp = barpts[0];
			DPoint3d ptendtmp = barpts[barpts.size() - 1];
			double mgDisstr = 0;
			DPoint3d mgVecstr;
			mgDisstr = GetExtendptByWalls(ptstrtmp, barpts[1], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecstr);
			double mgDisend = 0;
			DPoint3d mgVecend;
			mgDisend = GetExtendptByWalls(ptendtmp, barpts[barpts.size() - 2], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecend);
			//linestringeeh.AddToModel();
		   //��2���ֽ�����ʼ�˼����������Ƿ�����ǽ�ڵĵ㣨�������Ȱ�����ǽ��ĳ��ȼ��㣩�����û��˵����ǽ���Ҷ�û������ǽ��ֱ�ӶϿ�����
		   //����У�����1/2�ĺ�ȳ��ȼ�����һ����Ƿ���ǽ�ڣ��ڵĻ���˵������Ϊ�ڲ�ǽ�棬���Ȱ�15d,����ΪԶ��ǽ�ڵķ��򣬸ֽ��߼���ֵ = ���汣���� + �Բ�ֽ�ֱ���� 
		   //���ֻ��һ���н���,˵��Ϊ�����ê�룬���Ȱ�0.8LAE,����Ϊ��ǽ�ڷ��򣬸ֽ��߼���ֵ =  ���汣���㡣
			//(3)�ֽ�����ֹ�˼��㣬�����裨2��
			BarLinesdata tmpbarline;
			tmpbarline.path = it;
			mdlElmdscr_duplicate(&tmpbarline.barline, barline);
			tmpbarline.spacing = GetvecDirSpacing().at(index)*uor_per_mm;
			tmpbarline.strDis = dSideCover + GetvecStartOffset().at(index)*uor_per_mm + diameter / 2;
			tmpbarline.endDis = dSideCover + GetvecEndOffset().at(index)*uor_per_mm + diameter / 2;
			tmpbarline.diameter = diameter;

			tmpbarline.extendstrDis = ptstrtmp.Distance(barpts[0]) - dSideCover;
			//����ê�볤���ж��ӳ�����
			if (tmpbarline.extendstrDis != 0 && abs(mgDisstr - L0) < 2)
			{
				tmpbarline.extendstrDis = tmpbarline.extendstrDis - allfdiam;
			}
			tmpbarline.extendendDis = ptendtmp.Distance(barpts[barpts.size() - 1]) - dSideCover;
			if (tmpbarline.extendendDis != 0 && abs(mgDisend - L0) < 2)
			{
				tmpbarline.extendendDis = tmpbarline.extendendDis - allfdiam;
			}
			tmpbarline.extstrdiameter = allfdiam - diameter / 2;
			tmpbarline.extenddiameter = allfdiam - diameter / 2;
			tmpbarline.isInSide = false;
			tmpbarline.vecstr = mgVecstr;
			tmpbarline.vecend = mgVecend;
			tmpbarline.vecHoleehs = vecHoleehs;
			tmpbarline.strMG = mgDisstr;
			tmpbarline.endMG = mgDisend;
			tmpbarline.holMG = mgDisHol;
			

			barlines.push_back(tmpbarline);

			////���Դ���
			//barpts[0] = ptstrtmp;
			//barpts[barpts.size() - 1] = ptendtmp;
			////���»��Ƹֽ���
			//EditElementHandle linestringeeh;
			//LineStringHandler::CreateLineStringElement(linestringeeh, nullptr, &barpts[0], barpts.size(), true, *ACTIVEMODEL);
			//MSElementDescrP nowlinedescr = linestringeeh.GetElementDescrP();

		}
	}

}

void STWallRebarAssembly::CalculateUpDownBarLines(vector<BarLinesdata>& barlines, double fdiam, double allfdiam, DPoint3dR vecHoleehs, MSElementDescrP& path, MSElementDescrP& barline,
	int side, int index)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	WString strSize = GetvecDirSize().at(index);
	if (strSize.find(L"mm") != WString::npos)
	{
		strSize.ReplaceAll(L"mm", L"");
	}
	double bendRadius = RebarCode::GetPinRadius(strSize, ACTIVEMODEL, false);
	double diameter = RebarCode::GetBarDiameter(strSize, ACTIVEMODEL);		//������10
	double diameter1 = stoi(strSize.GetWCharCP())*uor_per_mm;
	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double diameterStr = 0, diameterEnd = 0;
	DPoint3d vecOutwall = m_walldata.vecToWall;//Զ��ǽ�ڵ�ƫ�Ʒ���
	vecOutwall.Negate();
	if (side != 0) vecOutwall.Negate();
	DPoint3d pt11, pt22;
	mdlElmdscr_extractEndPoints(&pt11, nullptr, &pt22, nullptr, path, ACTIVEMODEL);
	double lengh = pt11.Distance(pt22) / 3;//�ƶ���1/3��
	DPoint3d vecLine = pt22 - pt11;//��ʼ�ߴ�������
	vecLine.Normalize();
	vecLine.Scale(lengh);//�ӳ�
	//1������·�ֽ��߳��ȡ��ֽ��ê�볤�Ⱥ�ê�뷽��
	//�����ж��ϲ���û�а壬�а�Ļ��Ƿ����Ҳ඼�ڰ�ķ�Χ�ڣ��������˵��Ϊ�ڲ��棨�ֽ����ӳ� = ���� - ������ - �Բ�ֽ�ֱ���ͣ��ֽ�ê�볤��Ϊ15d,ê�뷽��ΪԶ��ǽ����һ�ࣩ
	//���ֻ��һ��˵��Ϊ����棨�ֽ����ӳ� = ���� - ������,�ֽ�ê�볤��ΪLae,ê�뷽��Ϊ�а����һ�ࣩ
	DPoint3d pt1, pt2;
	mdlElmdscr_extractEndPoints(&pt1, nullptr, &pt2, nullptr, barline, ACTIVEMODEL);
	bool isInsidestr = false;
	bool isHavefloorstr = false;
	isHavefloorstr = CalculateBarLineDataByFloor(m_walldata.downfloorfaces, m_walldata.downfloorID, pt1, pt2, vecLine, m_walldata.downfloorth, vecOutwall, isInsidestr, diameterStr);
	//���Դ�����ʾ��ǰ���жϵ��λ��
	double mgDisstr = 0;//ê�̳���
	double extendstr = 0;//��������
	DPoint3d mgVecstr;//ê�̷���
	DPoint3d ptstr = pt1;
	DPoint3d ptend = pt2;
	if (isHavefloorstr)//����а�
	{
		if (isInsidestr)//���ڲ�
		{
			//PITCommonTool::CPointTool::DrowOneLine(pt1, pt2, 1);
			mgDisstr = 15 * diameter1;
			extendstr = extendstr + m_walldata.downfloorth*uor_per_mm - allfdiam - dSideCover;
			mgVecstr = vecOutwall;
			if (diameterStr == 0)
			{
				diameterStr = diameter;
				diameterStr = diameterStr * 2;
			}
		}
		else
		{
			//PITCommonTool::CPointTool::DrowOneLine(pt1, pt2, 2);
			mgDisstr = get_lae() * diameter1 / uor_per_mm;//lae
			extendstr = extendstr + m_walldata.downfloorth*uor_per_mm - dSideCover - diameterStr;
			mgVecstr = vecOutwall;
			mgVecstr.Negate();
		}
		Dpoint3d vectmp = ptstr - ptend;
		vectmp.Normalize();
		vectmp.Scale(extendstr);
		ptstr.Add(vectmp);

		//����ê�̷��򳤶���
		Dpoint3d mgpt = ptstr;
		mgVecstr.Scale(mgDisstr);
		mgpt.Add(mgVecstr);
		mgVecstr.Normalize();

		EditElementHandle theeh;
		LineHandler::CreateLineElement(theeh, nullptr, DSegment3d::From(ptstr, mgpt), true, *ACTIVEMODEL);
		//theeh.AddToModel();
	}
	else//����������Ϸ�û�а壬�ֽ���Ҫê�뵽ǽ���ڲ���ê�볤��Ϊǽ�� - ��ǰ������+�󱣻��㣩
	{
		mgDisstr = m_walldata.thickness - dPositiveCover - dReverseCover - bendRadius - 1.5*diameter1;
		if (isInsidestr)//���ڲ�
		{
			extendstr = -1 * dPositiveCover - allfdiam;
		}
		else
		{
			extendstr = -1 * dPositiveCover;
		}

		mgVecstr = vecOutwall;
		//mgVecstr.Negate();
		//����ê�̷��򳤶���
		Dpoint3d mgpt = ptend;
		mgVecstr.Scale(mgDisstr);
		mgpt.Add(mgVecstr);
		mgVecstr.Normalize();

		EditElementHandle theeh;
		LineHandler::CreateLineElement(theeh, nullptr, DSegment3d::From(ptend, mgpt), true, *ACTIVEMODEL);
		//theeh.AddToModel();
	}
	bool isInsideend = false;
	bool isHavefloorend = false;
	isHavefloorend = CalculateBarLineDataByFloor(m_walldata.upfloorfaces, m_walldata.upfloorID, pt2, pt1, vecLine, m_walldata.upfloorth, vecOutwall, isInsideend, diameterEnd);
	double mgDisend = 0;//ê�̳���
	double mgDisHol = 0;//ê�̳���
	double extendend = 0;//��������
	DPoint3d mgVecend;//ê�̷���
	if (isHavefloorend)//����а�
	{
		if (isInsideend)//���ڲ�
		{
			mgDisend = 15 * diameter1;
			extendend = extendend + m_walldata.upfloorth*uor_per_mm - dSideCover - allfdiam;
			mgVecend = vecOutwall;
			if (diameterEnd == 0)
			{
				diameterEnd = diameter;
				diameterEnd = diameterEnd * 2;
			}
		}
		else
		{
			mgDisend = get_lae() * diameter1 / uor_per_mm;//lae
			extendend = extendend + m_walldata.upfloorth*uor_per_mm - dSideCover - diameterEnd;
			mgVecend = vecOutwall;
			mgVecend.Negate();
		}
		Dpoint3d vectmp = ptend - ptstr;
		vectmp.Normalize();
		vectmp.Scale(extendend);
		ptend.Add(vectmp);

		//����ê�̷��򳤶���
		Dpoint3d mgpt = ptend;
		mgVecend.Scale(mgDisstr);
		mgpt.Add(mgVecend);
		mgVecend.Normalize();

		EditElementHandle theeh;
		LineHandler::CreateLineElement(theeh, nullptr, DSegment3d::From(ptend, mgpt), true, *ACTIVEMODEL);
		//theeh.AddToModel();
	}
	else//����������Ϸ�û�а壬�ֽ���Ҫê�뵽ǽ���ڲ���ê�볤��Ϊǽ�� - ��ǰ������+�󱣻��㣩
	{
		mgDisend = m_walldata.thickness - dPositiveCover - dReverseCover - bendRadius - 1.5*diameter1;
		extendend = 0;
		if (isInsidestr)//���ڲ�
		{
			extendend = -1 * dPositiveCover - allfdiam;
		}
		else
		{
			extendend = -1 * dPositiveCover;
		}
		mgVecend = vecOutwall;
		//mgVecend.Negate();
		//����ê�̷��򳤶���
		Dpoint3d mgpt = ptend;
		mgVecend.Scale(mgDisend);
		mgpt.Add(mgVecend);
		mgVecend.Normalize();

		EditElementHandle theeh;
		LineHandler::CreateLineElement(theeh, nullptr, DSegment3d::From(ptend, mgpt), true, *ACTIVEMODEL);
		//theeh.AddToModel();
	}
	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - 2 * diameter;
	//if (GetvecDataExchange().at(index) == 0)//�����
	//{
	//	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - 2 * diameter;
	//}
	//else if (GetvecDataExchange().at(index) == 2)//�ڲ���
	//{
	//	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - diameter;
	//}
	lae = get_lae() * diameter1 / uor_per_mm;//lae
	EditElementHandle teeh;
	LineHandler::CreateLineElement(teeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
	//teeh.AddToModel();
	DPoint3d facenormal;
	DPoint3d maxpt;
	mdlElmdscr_extractNormal(&facenormal, &maxpt, m_walldata.downFace, NULL);
	DPoint3d cpt = getCenterOfElmdescr(m_walldata.downFace);
	facenormal.Normalize();
	DPlane3d plane = DPlane3d::FromOriginAndNormal(cpt, DVec3d::From(facenormal.x, facenormal.y, facenormal.z));
	double L0 = 24 * uor_per_mm * 15;//15d
	double Lae = 24 * uor_per_mm * 48 * 0.8;//48d
	//2������·���ߣ�����ȷ����û�п��ڵ�ǰǽ���������ǽ�壬����м��������ǽ�巽��Ҫ��·���ߴ�ֱ������ֱ�Ĺ��˵���
	//�ӳ�·���ߣ���·�����ƶ�������ǽ������λ�ô������·����������ǽ���߽��㣬������ͶӰ��·�����ϣ�����¼map<int,Dpoint3d>,��һ��ֵΪ��·���������룬�ڶ���ΪͶӰ�㣻
	//����ͶӰ�㣬�����µ�·���ߣ������·����ǰ���ΪͶӰ�㣬�Ҳ�Ϊԭʼ·���ߵ������յ㣬��Ҫ�ӳ����ӳ�����Ϊ�����汣����*2 + �ֽ�ֱ��
	//�����·�����յ�ΪͶӰ�㣬�Ҳ�Ϊԭʼ·���ߵ������յ㣬��Ҫ�ӳ����ӳ�����Ϊ�����汣����*2 + �ֽ�ֱ��
	GetCutPathLines(barlines, dSideCover, diameter1, path, m_walldata.cutWallfaces, m_walldata.downFace, m_walldata.height);//�����ڵ�ǰǽ����ǽ
	//3��������·�������ĸֽ��ߺ�·���ߣ�����������
	for (int i = 0; i < barlines.size(); i++)
	{
		barlines.at(i).spacing = GetvecDirSpacing().at(index)*uor_per_mm;
		barlines.at(i).strDis = dSideCover + GetvecStartOffset().at(i)*uor_per_mm + diameter * 1.5;
		barlines.at(i).endDis = dSideCover + GetvecEndOffset().at(i)*uor_per_mm + diameter * 1.5;
		barlines.at(i).diameter = diameter;
		barlines.at(i).extstrdiameter = diameterStr;
		barlines.at(i).extenddiameter = diameterEnd;
		barlines.at(i).extendstrDis = extendstr;
		barlines.at(i).extendendDis = extendend;
		barlines.at(i).vecstr = mgVecstr;
		barlines.at(i).vecend = mgVecend;
		barlines.at(i).vecHoleehs = vecHoleehs;
		barlines.at(i).strMG = mgDisstr;
		barlines.at(i).endMG = mgDisend;
		barlines.at(i).holMG = mgDisHol;
		vector<DPoint3d> barpts;//ȡ�����иֽ��ߵ㼯��
		ExtractLineStringPoint(barlines.at(i).path, barpts);
		if (barpts.size() > 1)
		{//��ʼ���¼���ֽ�����ʼ��
			DPoint3d ptstrtmp = barpts[0];
			DPoint3d ptendtmp = barpts[barpts.size() - 1];
			double mgDisstr = 0;
			DPoint3d mgVecstr;
			mgDisstr = GetExtendptByWalls(ptstrtmp, barpts[1], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecstr);
			double mgDisend = 0;
			DPoint3d mgVecend;
			mgDisend = GetExtendptByWalls(ptendtmp, barpts[barpts.size() - 2], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecend);
			//����ê�볤���ж��ӳ�����
			if (ptstrtmp.Distance(barpts[0]) > 10 && abs(mgDisstr - L0) > 100)//���ֽ���Ҫ�ӳ�
			{
				//������һ���ֽ��࣬��֤���һ���ֽ���ص�
				DPoint3d tmpptvec = barpts[0] - ptstrtmp;
				tmpptvec.Normalize();
				tmpptvec.Scale(barlines.at(i).spacing);
				ptstrtmp.Add(tmpptvec);
				barpts[0] = ptstrtmp;
			}
			if (ptendtmp.Distance(barpts[barpts.size() - 1]) > 10 && abs(mgDisend - L0) > 100)
			{
				//������һ���ֽ��࣬��֤���һ���ֽ���ص�
				DPoint3d tmpptvec = barpts[barpts.size() - 1] - ptendtmp;
				tmpptvec.Normalize();
				tmpptvec.Scale(barlines.at(i).spacing);
				ptendtmp.Add(tmpptvec);
				barpts[barpts.size() - 1] = ptendtmp;
			}
			//�ؽ�·����
			EditElementHandle linestringeeh;
			LineStringHandler::CreateLineStringElement(linestringeeh, nullptr, &barpts[0], barpts.size(), true, *ACTIVEMODEL);
			mdlElmdscr_freeAll(&barlines.at(i).path);
			barlines.at(i).path = linestringeeh.ExtractElementDescr();
			//mdlElmdscr_add(barlines.at(i).path);
		}
	}
}
void STWallRebarAssembly::CalculateBarLinesData(map<int, vector<BarLinesdata>> &barlines, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double dLevelSpace = 0;

	double dOffset = dPositiveCover;
	double dOffsetTb = dPositiveCover;

	DSegment3d temp = m_walldata.vecFontLine[0];
	DPoint3d ptr1, ptr2;
	m_walldata.vecFontLine[0].GetStartPoint(ptr1);
	m_walldata.vecFontLine[0].GetEndPoint(ptr2);
	ptr1.Add(ptr2);
	ptr1.Scale(0.5);
	DRange3d range;
	//����ָ��Ԫ����������Ԫ�صķ�Χ��
	bool is_cond = false;
	//PITCommonTool::CPointTool::DrowOnePoint(ptr1, 1);//��
	for (auto it : m_walldata.downfloorfaces)//�ж�vecFontLine�Ƿ�������� �����������ڲ��߻���
	{
		if (abs(ptr1.x - ptr2.x) > 10 * uor_per_mm && abs(ptr1.y - ptr2.y) > 10 * uor_per_mm)
			break;
		mdlElmdscr_computeRange(&range.low, &range.high, it, NULL);
		//mdlElmdscr_add(it);
		range.low.x = range.low.x + 50 * uor_per_mm;
		range.low.y = range.low.y + 50 * uor_per_mm;
		range.high.x = range.high.x - 50 * uor_per_mm;
		range.high.y = range.high.y - 50 * uor_per_mm;
		if (range.IsContainedXY(ptr1))
		{
			m_walldata.vecFontLine[0] = m_walldata.vecBackLine[0];
			m_walldata.vecBackLine[0] = temp;
			m_walldata.vecToWall.Negate();
			is_cond = true;//�Ѿ��жϳɹ�
			break;
		}
	}
	if (!is_cond)
	{
		for (auto it : m_walldata.upfloorfaces)//�ж�vecFontLine�Ƿ�������� �����������ڲ��߻���
		{
			if (abs(ptr1.x - ptr2.x) > 10 * uor_per_mm && abs(ptr1.y - ptr2.y) > 10 * uor_per_mm)
				break;
			mdlElmdscr_computeRange(&range.low, &range.high, it, NULL);
			//mdlElmdscr_add(it);
			range.low.x = range.low.x + 50 * uor_per_mm;
			range.low.y = range.low.y + 50 * uor_per_mm;
			range.high.x = range.high.x - 50 * uor_per_mm;
			range.high.y = range.high.y - 50 * uor_per_mm;
			if (range.IsContainedXY(ptr1))
			{
				m_walldata.vecFontLine[0] = m_walldata.vecBackLine[0];
				m_walldata.vecBackLine[0] = temp;
				m_walldata.vecToWall.Negate();
				is_cond = true;//�Ѿ��жϳɹ�
				break;
			}
		}
	}

	//����ƫ�Ʒ���
	if (m_walldata.vecFontLine.size() == 0) return;
	DVec3d moveLine = m_walldata.vecdown;
	DPoint3d pt1[2];
	m_walldata.vecFontLine[0].GetStartPoint(pt1[0]);
	m_walldata.vecFontLine[0].GetEndPoint(pt1[1]);
	DPoint3d linevec = pt1[1] - pt1[0];
	linevec.Normalize();
	moveLine.CrossProduct(moveLine, linevec);
	DPoint3d pt2[2];
	m_walldata.vecBackLine[0].GetStartPoint(pt2[0]);

	//1�������桢�м䡢����ɸѡ����Ӧ�ĸֽ�������Ӧ�ĸֽ�ֱ��
	map<int, double> FDiameters;
	map<int, double> MDiameters;
	map<int, double> BDiameters;

	double allfdiam = 0;//ͳ�������ֽ�ֱ��
	double allmdiam = 0;//ͳ���м��ֽ�ֱ��
	double allbdiam = 0;//ͳ���ڲ���ֽ�ֱ��
	double alldiam = 0;//�Բ�ֽ�ֱ����
	DPoint3d vecHoleehs;//������ֿ׶���׶�ê��ķ���
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}
		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//������10
		if (GetvecDataExchange().at(i) == 0)//�����
		{
			FDiameters[GetvecRebarLevel().at(i)] = diameter;
			allfdiam = allfdiam + diameter;
		}
		else if (GetvecDataExchange().at(i) == 2)//�ڲ���
		{
			BDiameters[GetvecRebarLevel().at(i)] = diameter;
			allbdiam = allbdiam + diameter;
		}
		else
		{
			MDiameters[GetvecRebarLevel().at(i)] = diameter;
			allmdiam = allmdiam + diameter;
		}
	}

	//2�������ÿһ���ƫ��������������Ϊ��׼
	//�����棬���棬�м����������㣬
	//����ʱ��ƫ���� = ���汣���� + ��ǰ��ֽ�ֱ��/2 + ǰ��������ֽ�ֱ���ĺͣ�
	//�м�ʱ��ƫ���� = ǽ��/2 + ǰ��������ֽ�ֱ���ĺͣ�
	//����ʱ��ƫ���� = ǽ�� - ���汣���� - ����ǰ��ֽ�ֱ��/2 + ǰ��������ֽ�ֱ���ĺͣ���
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}

		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//������10
		double movedis = dPositiveCover + diameter / 2;
		double fdiam = 0;//ǰ�漸��ֽ�ֱ��

		if (GetvecDataExchange().at(i) == 0)//ǰ���
		{
			int tindex = GetvecRebarLevel().at(i);
			for (int k = 1; k < tindex; k++)//����֮ǰ�ĸֽ�ֱ��
			{
				movedis = FDiameters[k] + movedis;
				fdiam = fdiam + FDiameters[k];
			}
		}
		else if (GetvecDataExchange().at(i) == 2)//�����
		{
			movedis = m_walldata.thickness - dReverseCover - diameter / 2;
			int tindex = GetvecRebarLevel().at(i);
			for (int k = 1; k < tindex; k++)//����֮ǰ�ĸֽ�ֱ��
			{
				movedis = movedis - BDiameters[k];
				fdiam = fdiam + BDiameters[k];
			}
		}
		else
		{
			movedis = m_walldata.thickness / 2;
			int tindex = GetvecRebarLevel().at(i);
			for (int k = 1; k < tindex; k++)//����֮ǰ�ĸֽ�ֱ��
			{
				movedis = MDiameters[k] + movedis;
				fdiam = fdiam + MDiameters[k];
			}
		}
		if (GetvecDataExchange().at(i) == 0)//ǰ���,�Բ��Ϊ����
		{
			alldiam = allbdiam;
			vecHoleehs = pt2[0] - pt1[0];

		}
		else if (GetvecDataExchange().at(i) == 2)//����㣬�Բ��Ϊǰ��
		{
			alldiam = allfdiam;
			vecHoleehs = pt1[0] - pt2[0];
		}
		else
		{
			alldiam = 0;
		}

		vecHoleehs.Normalize();

		//����path�ߴ�������Ԫ�ء�ƫ��������ƫ�ƺ���ߴ�
		MSElementDescrP tmppath = GetLines(m_walldata.vecFontLine);
		GetMovePath(tmppath, movedis, m_walldata.downFace);
		//�����������
		DPoint3d pt1, pt2;
		mdlElmdscr_extractEndPoints(&pt1, nullptr, &pt2, nullptr, tmppath, ACTIVEMODEL);
		//PITCommonTool::CPointTool::DrowOneLine(DSegment3d::From(pt1, pt2), 4);//��
		pt2 = pt1;
		pt2.z = pt2.z + m_walldata.height*uor_per_mm;
		EditElementHandle tlineeeh;
		LineHandler::CreateLineElement(tlineeeh, nullptr, DSegment3d::From(pt1, pt2), true, *ACTIVEMODEL);
		//PITCommonTool::CPointTool::DrowOneLine(DSegment3d::From(pt1, pt2), 3);//��

		//PITCommonTool::CPointTool::DrowOneLine(m_walldata.vecFontLine[0],1);//��
		//PITCommonTool::CPointTool::DrowOneLine(m_walldata.vecBackLine[0], 2);//��
		//3��������Χ���塢�װ��ǽ��������㵱ǰʵ����Χ�͸ֽ��������Լ��乳����
		//�ֽ��ͬ����ʽ��һ��

		if (GetvecDir().at(i) == 0)//ˮƽ����ֽ�
		{
			MSElementDescrP path = tlineeeh.GetElementDescrP();
			MSElementDescrP barline = tmppath;
			CalculateLeftRightBarLines(barlines[i], fdiam, alldiam, vecHoleehs, path, barline, GetvecDataExchange().at(i), i);
			mdlElmdscr_freeAll(&tmppath);
		}
		else//��ֱ����ֽ�
		{
			MSElementDescrP barline = tlineeeh.GetElementDescrP();
			MSElementDescrP  path = tmppath;
			CalculateUpDownBarLines(barlines[i], fdiam, alldiam, vecHoleehs, path, barline, GetvecDataExchange().at(i), i);
			mdlElmdscr_freeAll(&tmppath);
		}
		/*BarLinesdata tmpdata;
		mdlElmdscr_add(tmpdata.barline);
		mdlElmdscr_add(tmpdata.path);
		tmpdata.spacing = GetvecDirSpacing().at(i)*uor_per_mm;
		tmpdata.strDis = dSideCover + GetvecStartOffset().at(i)*uor_per_mm;
		tmpdata.endDis = dSideCover + GetvecEndOffset().at(i)*uor_per_mm;
		barlines[i].push_back(tmpdata);*/
	}

}
/*
* @desc:		���ݱ�������ƶ��������¼�������ֽ��ߴ�
* @param[out]	data ���������
* @return	MSElementDescrP �µĸֽ��ߴ�
* @author	Hong ZhuoHui
* @Date:	2023/09/13
*/
void STWallRebarAssembly::ReCalBarLineByCoverAndDis(BarLinesdata & data)
{
	//������β�ƶ�����
	double strMoveLen = 0 - data.strDis;
	double endMoveLen = 0 - data.endDis;
	//������β��
	DPoint3d strPt = { 0,0,0 }, endPt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&strPt, nullptr, &endPt, nullptr, data.barline, ACTIVEMODEL);
	//������β�ƶ�����
	DVec3d strVec = strPt - endPt;
	strVec.Normalize();
	strVec.ScaleToLength(strMoveLen);
	DVec3d endVec = endPt - strPt;
	endVec.Normalize();
	endVec.ScaleToLength(endMoveLen);
	//�ƶ���β��
	strPt.Add(strVec);
	endPt.Add(endVec);
	//���»���
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPt, endPt), true, *ACTIVEMODEL);
	data.barline = eeh.GetElementDescrP();
	//ƽ��·����
	mdlCurrTrans_begin();
	Transform tMatrix;
	mdlTMatrix_getIdentity(&tMatrix);
	mdlTMatrix_setTranslation(&tMatrix, &strVec);
	mdlElmdscr_transform(&data.path, &tMatrix);
	mdlCurrTrans_end();
}

/*
* @desc:		����ʵ�������׶���Z��ǽ��
* @param[in]	wallEeh ǽ
* @param[out]	holes �׶�
* @author	Hong ZhuoHui
* @Date:	2023/09/13
*/
void STWallRebarAssembly::CalHolesBySubtract(EditElementHandleCR wallEeh, std::vector<EditElementHandle*>& holes)
{
	if (m_walldata.downFace == nullptr)
		return;

	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	MSElementDescrP tmpDowmFace = nullptr;
	m_walldata.downFace->Duplicate(&tmpDowmFace);
	EditElementHandle eehdown;
	eehdown.SetElementDescr(tmpDowmFace, true, true, ACTIVEMODEL);
	ISolidKernelEntityPtr ptarget;
	SolidUtil::Convert::ElementToBody(ptarget, eehdown);
	SolidUtil::Modify::ThickenSheet(ptarget, m_walldata.height * uor_now, 0.0);
	if (SUCCESS == SolidUtil::Convert::BodyToElement(eehdown, *ptarget, NULL, *ACTIVEMODEL))
	{
		EditElementHandle eehhole;
		if (SolidBoolWithFace(eehhole, eehdown, wallEeh, BOOLOPERATION_SUBTRACT, false))
		{
			//ȡ���е���
			std::vector<EditElementHandle*> allfaces;
			//ExtractFacesTool::GetFaces(holeeeh, allfaces);
			GetHoleElementAllFaces(eehhole, allfaces);
			vector<ISolidKernelEntityPtr> allfoceentitys;
			for (int i = 0; i < allfaces.size(); i++)
			{
				ISolidKernelEntityPtr tmpptr;
				if (SolidUtil::Convert::ElementToBody(tmpptr, *allfaces.at(i)) == SUCCESS)
				{
					allfoceentitys.push_back(tmpptr);
				}
			}
			bvector<ISolidKernelEntityPtr> sewn;
			bvector<ISolidKernelEntityPtr> unsewn;
			SolidUtil::Modify::SewBodies(sewn, unsewn, &allfoceentitys.front(), allfoceentitys.size(), true);
			for (int i = 0; i < sewn.size(); i++)
			{
				ISolidKernelEntityPtr tmpptr = sewn.at(i);
				EditElementHandle* tmpeeh = new EditElementHandle();
				if (SolidUtil::Convert::BodyToElement(*tmpeeh, *tmpptr, nullptr, *ACTIVEMODEL) == SUCCESS)
				{
					char tmpc[256];
					itoa(i, tmpc, 10);
					string holename;
					holename = holename + "_Hole_" + tmpc;
					SetElemGraphItemTypeValue(*tmpeeh, holename, "NEG", L"PARADATA", L"PARADATA", ACTIVEMODEL);
					holes.push_back(tmpeeh);
				}
			}
		}
	}
}
void STWallRebarAssembly::CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	vTransform.clear();
	vTransformTb.clear();
	double updownSideCover = 50 * uor_per_mm;
	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double dLevelSpace = 0;
	double diameterTie = 0.0;
	BrString strTieRebarSize(GetTieRebarInfo().rebarSize);
	if (strTieRebarSize != L""/*&& 0 != GetTieRebarInfo().tieRebarMethod*/)
	{
		diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//����ֱ��
	}

	double dOffset = dPositiveCover + diameterTie;
	double dOffsetTb = dPositiveCover + diameterTie;
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}

		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//������10
		double diameterTb = 0.0;
		if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
		{
			diameterTb = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(i).rebarSize, modelRef);		//������10
		}

		if (diameter > BE_TOLERANCE)
		{
			CVector3D	zTrans(0.0, 0.0, 0.0);
			CVector3D	zTransTb;
			if (GetvecDir().at(i) == 0) //ˮƽ
			{
				zTrans.z = updownSideCover - diameter * 0.5;
				zTrans.x = m_STwallData.length * 0.5;
				zTransTb = zTrans;
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
				{
					if (GetvecTwinRebarLevel().at(i).hasTwinbars)//��ǰ�ֽ�Ϊ�����
					{
						zTransTb.z = zTransTb.z + diameter + diameterTb;
					}
				}
			}
			else
			{
				zTrans.z = m_STwallData.height * 0.5;
				zTrans.x = updownSideCover + diameter * 0.5;
				zTransTb = zTrans;
			}


			// 			if (0 == i)
			// 			{
			// 				dOffset += diameter / 2.0;	//ƫ���ײ�ֽ�뾶
			// 				zTrans.y = dOffset;
			// 		    }
						//if (0 == i)
						//{
						//	dOffset += diameter / 2.0;	//ƫ���ײ�ֽ�뾶
						//	zTrans.y = dOffset;

						//	if (GetvecTwinRebarLevel().at(i).hasTwinbars)
						//	{
						//		if (diameterTb > diameter)//�����ĸֽ������ֱ����
						//		{
						//			dOffsetTb = dOffset;
						//			dOffsetTb -= (diameterTb / 2.0 - diameter / 2.0);
						//			zTransTb.y = dOffsetTb;
						//		}
						//		else
						//		{
						//			dOffsetTb = dOffset;
						//			dOffsetTb += (diameter / 2.0 - diameterTb / 2.0);
						//			zTransTb.y = dOffsetTb;
						//		}
						//	}
						//}
						//else
			{
				WString strSizePre;
				if (i != 0)
				{
					strSizePre = WString(GetvecDirSize().at(i - 1).Get());
					if (strSizePre.find(L"mm") != WString::npos)
					{
						strSizePre.ReplaceAll(L"mm", L"");
					}
				}

				double diameterPre = RebarCode::GetBarDiameter(strSizePre, modelRef);		//������10
				if (0 == i)
				{
					dOffset += diameter / 2.0;	//ƫ���ײ�ֽ�뾶
					dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm;
				}
				else
				{
					dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm + diameter * 0.5 + diameterPre * 0.5;//������ϵ�ǰ�ֽ�ֱ��
				}

				dOffset += dLevelSpace;
				dOffsetTb = dOffset;
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
				{
					if (diameterTb > diameter)//�����ĸֽ������ֱ����
						dOffsetTb += (diameterTb / 2.0 - diameter / 2.0);
					else
						dOffsetTb -= (diameter / 2.0 - diameterTb / 2.0);
				}
				if (COMPARE_VALUES(m_STwallData.width - dOffset, dReverseCover + diameterTie) < 0)		//��ǰ�ֽ����Ƕ�뵽�˷��汣������ʱ��ʵ�ʲ��õĸֽ����Ͳ���ʹ�����õ����ϲ��࣬����ʹ�ñ������������
				{
					zTrans.y = m_STwallData.width - dReverseCover - diameter / 2.0 - diameterTie;
					zTransTb.y = zTrans.y;
					if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
					{
						if (diameterTb > diameter)//�����ĸֽ������ֱ����
							zTransTb.y += (diameterTb / 2.0 - diameter / 2.0);
						else
							zTransTb.y -= (diameter / 2.0 - diameterTb / 2.0);
					}
					//�жϣ������һ���zTrans.y�뵱ǰ���zTrans.y��ͬ������һ���ȥ��ǰ��ĸֽ�ֱ��������ֹ�ֽ���ײ��
					double compare = zTrans.y;
					if (vTransform.size() > 0)
					{
						double reverseOffset = diameter;
						for (int j = (int)vTransform.size() - 1; j >= 0; j--)
						{
							WString strSize1 = GetvecDirSize().at(j);
							if (strSize1.find(L"mm") != WString::npos)
							{
								strSize1.ReplaceAll(L"mm", L"");
							}
							diameterPre = RebarCode::GetBarDiameter(strSize1, modelRef);		//������10
							if (COMPARE_VALUES(vTransform[j].y + diameterPre * 0.5, compare - diameter * 0.5) > 0)	//Ƕ������һ���ֽ���
							{
								if (j == vTransform.size() - 1)//Ϊ��ǰ�����ĵ�һԪ��
								{
									vTransform[j].y = zTrans.y;
									vTransformTb[j].y = vTransform[j].y;
								}
								else
								{
									vTransform[j].y = vTransform[j + 1].y;
									vTransformTb[j].y = vTransform[j].y;
								}

								vTransform[j].y -= reverseOffset;
								vTransformTb[j].y = vTransform[j].y;
								if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(j).hasTwinbars)
								{
									double diameterTbPre = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(j).rebarSize, modelRef);		//������10

									if (diameterTbPre > diameterPre)//�����ĸֽ������ֱ����
										vTransformTb[j].y -= (diameterTbPre / 2.0 - diameterPre / 2.0);
									else
										vTransformTb[j].y += (diameterPre / 2.0 - diameterTbPre / 2.0);
								}
								compare = vTransform[j].y;
								diameter = diameterPre;
							}
						}
					}
				}
				else
				{
					zTrans.y = dOffset;
					zTransTb.y = dOffsetTb;
					// 					if (GetvecTwinRebarLevel().at(i).hasTwinbars && diameterTb > diameter)	//�����ĸֽ������ֱ����
					// 					{
					// 						zTrans.y -= (diameterTb / 2.0 - diameter / 2.0) * 2;
					// 						zTransTb.y -= (diameterTb / 2.0 - diameter / 2.0);
					// 					}
				}
			}
			//			zTransTb = zTrans;
			vTransform.push_back(zTrans);
			vTransformTb.push_back(zTransTb);
		}
	}
}


void STWallRebarAssembly::CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return;
	DPoint3d ptstar = m_STwallData.ptStart;
	DPoint3d ptend = m_STwallData.ptEnd;

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstar, ptend), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);

	DPoint3d pt1[2];
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//��ʱʹ�õ�ǰ����MODEL�������������޸�
	if (pt1[0].Distance(ptstar) > pt1[0].Distance(ptend))//ȷ��pt1[0]Ϊ��ʼ��
	{
		DPoint3d tmpPt = pt1[0];
		pt1[0] = pt1[1];
		pt1[1] = tmpPt;
	}

	vector<DPoint3d> inpts;
	GetIntersectPointsWithHoles(inpts, m_Negs, ptstar, ptend);
	lenth = pt1[0].Distance(pt1[1]);
	//if (!GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	if (inpts.size() > 0)
	{
		/*EditElementHandle eeh1;
		LineHandler::CreateLineElement(eeh1, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
		eeh1.AddToModel();*/
		DPoint3d ptProject1;	//ͶӰ��
		mdlVec_projectPointToLine(&ptProject1, NULL, &pt1[0], &m_STwallData.ptStart, &m_STwallData.ptEnd);

		DPoint3d ptProject2;	//ͶӰ��
		mdlVec_projectPointToLine(&ptProject2, NULL, &pt1[1], &m_STwallData.ptStart, &m_STwallData.ptEnd);

		misDisstr = ptProject1.Distance(ptstar);

		misDisend = ptProject2.Distance(ptend);

	}



}


void STWallRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetSideCover()*uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

	if (g_wallRebarInfo.concrete.isHandleHole)//������Ҫ����Ŀ׶�
	{
		for (int j = 0; j < m_Holeehs.size(); j++)
		{
			EditElementHandle eeh;
			eeh.Duplicate(*m_Holeehs.at(j));

			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
			}

			TransformInfo transinfo(trans);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			DPoint3d minP;
			DPoint3d maxP;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
			DRange3d range;
			range.low = minP;
			range.high = maxP;
			bool isNeed = false;
			if (range.XLength() > misssize || range.ZLength() > misssize)
			{
				isNeed = true;
			}

			if (isNeed)
			{
				if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//������Ŷ�
				{
					continue;
				}
				bool isdoorNeg = false;//�ж��Ƿ�Ϊ�Ŷ�NEG
				isdoorNeg = IsDoorHoleNeg(m_Holeehs.at(j), m_doorsholes);
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*m_Holeehs.at(j));

				if (isdoorNeg)
				{
					PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix, isdoorNeg, m_STwallData.width);
				}
				else
				{
					PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
				}

				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}



}


bool STWallRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	CalculateUseHoles(modelRef);
	m_vecRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
	g_vecTieRebarPtsNoHole.clear();
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
	double dLevelSpace = 0;
	double dSideCover = GetSideCover()*uor_per_mm;
	//if (COMPARE_VALUES(dSideCover, m_STwallData.length) >= 0)	//������汣������ڵ���ǽ�ĳ���
	//{
	//	mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"���汣������ڵ���ǽ�ĳ���,�޷������ֽ��", MessageBoxIconType::Information);
	//	return false;
	//}

	if (m_walldata.downFace == nullptr) return false;
	map<int, vector<BarLinesdata>> barlines;
	CalculateBarLinesData(barlines, ACTIVEMODEL);
	int setCount = 0;
	m_nowlevel = -1;
	for (auto it : barlines)
	{
		m_vecRebarPtsLayer.clear();
		m_nowlevel++;
		if (it.second.size() == 0)
			continue;
		setCount++;
		RebarSetTag* tag = NULL;
		PopvecSetId().push_back(0);
		if (GetvecDataExchange().at(it.first) == 0)//�����
		{
			it.second[0].isInSide = false;
		}
		else if (GetvecDataExchange().at(it.first) == 2)//�ڲ���
		{
			it.second[0].isInSide = true;
		}
		//�ж��Ƿ�ǽȫ�������Χ
		DSegment3d temp = m_walldata.vecFontLine[0];
		DPoint3d ptr1, ptr2;
		m_walldata.vecFontLine[0].GetStartPoint(ptr1);
		m_walldata.vecFontLine[0].GetEndPoint(ptr2);
		ptr1.Add(ptr2);
		ptr1.Scale(0.5);//�е��ж�
		DRange3d range;
		//����ָ��Ԫ����������Ԫ�صķ�Χ��
		for (auto data : m_walldata.downfloorfaces)//�ж�vecFontLine�Ƿ������ �����������ڲ�
		{
			if (abs(ptr1.x - ptr2.x) > 10 * uor_per_mm && abs(ptr1.y - ptr2.y) > 10 * uor_per_mm)
				break;
			mdlElmdscr_computeRange(&range.low, &range.high, data, NULL);
			//mdlElmdscr_add(it);
			range.low.x = range.low.x + 50 * uor_per_mm;
			range.low.y = range.low.y + 50 * uor_per_mm;
			range.high.x = range.high.x - 50 * uor_per_mm;
			range.high.y = range.high.y - 50 * uor_per_mm;
			if (range.IsContainedXY(ptr1))
			{
				it.second[0].isInSide = false;
				break;
			}
		}
		tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(it.first), it.second,
			GetvecStartOffset().at(it.first)*uor_per_mm, GetvecEndOffset().at(it.first)*uor_per_mm,
			GetvecRebarLevel().at(it.first), GetvecRebarType().at(it.first), ACTIVEMODEL, GetvecRebarLineStyle().at(it.first), GetvecRebarWeight().at(it.first));
		if (NULL != tag && (!PreviewButtonDown))
		{
			tag->SetBarSetTag(setCount);
			rsetTags.Add(tag);
		}
	}
	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
	vector<CVector3D> vTrans;
	vector<CVector3D> vTransTb;
	//�����������ƫ����
	CalculateTransform(vTrans, vTransTb, modelRef);
	if (vTrans.size() != GetRebarLevelNum())
	{
		return false;
	}

	double dLength = m_STwallData.length;
	double dWidth = m_STwallData.height;

	int iRebarSetTag = 0;
	int iRebarLevelNum = GetRebarLevelNum();
	int iTwinbarSetIdIndex = 0;

	vector<PIT::EndType> vecEndType;

	int frontlevel = 0;//ǰ����
	int backlevel = 0;//�����
	int midlevel = 0;//�м���
	int allbacklevel = 0;
	//ͳ���±����ܹ��ж��ٲ�
	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		if (GetvecDataExchange().at(i) == 2)
		{
			allbacklevel++;
		}
	}
	backlevel = allbacklevel;
	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		int tmpLevel;
		if (GetvecDataExchange().at(i) == 0)//ǰ���
		{
			frontlevel++;
			tmpLevel = frontlevel;
		}
		else if (GetvecDataExchange().at(i) == 2)
		{
			tmpLevel = backlevel;
			backlevel--;
		}
		else
		{
			midlevel++;
			tmpLevel = midlevel;
		}
		//		int iActualIndex = i;
		RebarSetTag* tag = NULL;
		CMatrix3D   mat, matTb;

		vector<PIT::EndType> vecEndType;
		if (GetvvecEndType().empty())		//û�����ö˲���ʽ������Ĭ��ֵ
		{
			EndType endType;
			memset(&endType, 0, sizeof(endType));
			vecEndType = { { 0,0,0 },{0,0,0} ,endType };
		}
		else
		{
			vecEndType = GetvvecEndType().at(i);
		}

		CVector3D tmpVector(m_LineNormal);
		tmpVector.Scale(vTrans[i].y);
		CMatrix3D   tmpmat;
		tmpmat.SetTranslation(tmpVector);
		double  Misdisstr = 0; double Misdisend = 0;
		double tLenth = 0;
		CalculateNowPlacementAndLenth(Misdisstr, Misdisend, tLenth, tmpmat, modelRef);

		if (vTrans.size() != GetRebarLevelNum())
		{
			return false;
		}

		double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;
		m_vecRebarPtsLayer.clear();
		m_vecTwinRebarPtsLayer.clear();
		m_vecTieRebarPtsLayer.clear();
		m_nowvecDir = GetvecDir().at(i);
		if (GetvecDir().at(i) == 1)	//����ֽ�
		{
			double misDisH_left, misDisH_right;

			if (COMPARE_VALUES_EPS(m_angle_left, PI / 2, 0.001))
			{
				misDisH_left = (1 / sin(m_angle_left) - 1)*diameter + Misdisstr;
			}
			else
			{
				misDisH_left = 0;
			}
			if (COMPARE_VALUES_EPS(m_angle_right, PI / 2, 0.001))
			{
				misDisH_right = (1 / sin(m_angle_right) - 1)*diameter + Misdisend;
			}
			else
			{
				misDisH_right = 0;
			}

			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//�˲��乳����
			if (GetvvecEndType().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = m_STwallData.ptEnd - m_STwallData.ptStart;
					endNormal.Normalize();
					if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
					{
						endNormal.Negate();
					}
					CVector3D rebarVec = CVector3D::kZaxis;
					/*					endNormal = rebarVec.CrossProduct(vec);*/
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
					vecEndNormal[k] = endNormal;
				}
			}
			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;
			matTb.SetTranslation(vTransTb[i]);
			matTb = GetPlacement() * matTb;
			//���Ʋ���--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
			{
				m_isPushTieRebar = false;
				//�Ȼ��Ʒǲ����
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
					GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef, GetvecRebarLineStyle().at(i), GetvecRebarWeight().at(i));
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}

				m_isPushTieRebar = true;
				//���Ʋ����
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
					GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), true, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef, GetvecRebarLineStyle().at(i), GetvecRebarWeight().at(i));
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
			else //��ǰ��δ���ò���
			{
				m_isPushTieRebar = true;
				TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
					GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat, twinRebar, false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef, GetvecRebarLineStyle().at(i), GetvecRebarWeight().at(i));
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}
			}
			vecEndType.clear();
		}
		else
		{
			double misDisV_left, misDisV_right;
			if (COMPARE_VALUES_EPS(m_angle_left, PI / 2, 0.001))
			{
				misDisV_left = diameter / tan(m_angle_left);
			}
			else
			{
				misDisV_left = 0;
			}
			if (COMPARE_VALUES_EPS(m_angle_right, PI / 2, 0.001))
			{
				misDisV_right = diameter / tan(m_angle_right);
			}
			else
			{
				misDisV_right = 0;
			}
			double leftSideCov = GetSideCover()*uor_per_mm / sin(m_angle_left);
			double rightSideCov = GetSideCover()*uor_per_mm / sin(m_angle_right);
			double allSideCov = leftSideCov + rightSideCov;

			tLenth = tLenth - (misDisV_left + misDisV_right);
			vTrans[i].x = (tLenth - allSideCov) / 2 + Misdisstr + leftSideCov;
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//�˲��乳����
			if (GetvvecEndType().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					CVector3D rebarVec = m_STwallData.ptEnd - m_STwallData.ptStart;
					endNormal = CVector3D::From(0, 0, -1);
					if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
					{
						endNormal.Negate();
					}
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
					vecEndNormal[k] = endNormal;
				}
			}
			mat = rot90;
			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;
			matTb = rot90;
			matTb.SetTranslation(vTransTb[i]);
			matTb = GetPlacement() * matTb;
			//������Ϊ�����,ż����Ϊ��ͨ��

			//���Ʋ���--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
			{
				m_isPushTieRebar = true;
				//�Ȼ��Ʒǲ����
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef, GetvecRebarLineStyle().at(i), GetvecRebarWeight().at(i));
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}

				m_isPushTieRebar = false;
				//���Ʋ����
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), true, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef, GetvecRebarLineStyle().at(i), GetvecRebarWeight().at(i));
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;

			}
			else //��ǰ��δ���ò���
			{
				m_isPushTieRebar = true;
				TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, twinRebar, false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef, GetvecRebarLineStyle().at(i), GetvecRebarWeight().at(i));
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}
			}
			//end
			vecEndType.clear();
		}
		if (m_vecRebarPtsLayer.size() > 1)
		{
			for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
			{
				int n = m + 1;
				RebarPoint rbPt;
				rbPt.Layer = GetvecRebarLevel().at(i);
				rbPt.iIndex = i;
				rbPt.sec = 0;
				rbPt.vecDir = GetvecDir().at(i);
				rbPt.ptstr = m_vecRebarPtsLayer.at(m);
				rbPt.ptend = m_vecRebarPtsLayer.at(n);
				rbPt.DataExchange = GetvecDataExchange().at(i);
				g_vecRebarPtsNoHole.push_back(rbPt);
				m++;
			}
		}
		if (m_vecTwinRebarPtsLayer.size() > 1)
		{
			for (int m = 0; m < m_vecTwinRebarPtsLayer.size() - 1; m++)
			{
				int n = m + 1;
				RebarPoint rbPt;
				rbPt.Layer = GetvecRebarLevel().at(i);
				rbPt.iIndex = i;
				rbPt.sec = 0;
				rbPt.vecDir = GetvecDir().at(i);
				rbPt.ptstr = m_vecTwinRebarPtsLayer.at(m);
				rbPt.ptend = m_vecTwinRebarPtsLayer.at(n);
				rbPt.DataExchange = GetvecDataExchange().at(i);
				g_vecTwinRebarPtsNoHole.push_back(rbPt);
				m++;
			}
		}
		if (m_vecTieRebarPtsLayer.size() > 1)
		{
			for (int m = 0; m < m_vecTieRebarPtsLayer.size() - 1; m++)
			{
				int n = m + 1;
				RebarPoint rbPt;
				rbPt.Layer = GetvecRebarLevel().at(i);
				rbPt.iIndex = i;
				rbPt.sec = 0;
				rbPt.vecDir = GetvecDir().at(i);
				rbPt.ptstr = m_vecTieRebarPtsLayer.at(m);
				rbPt.ptend = m_vecTieRebarPtsLayer.at(n);
				rbPt.DataExchange = GetvecDataExchange().at(i);
				g_vecTieRebarPtsNoHole.push_back(rbPt);
				m++;
			}
		}
	}


	if (PreviewButtonDown)//Ԥ����ť���£���������
	{
		m_allLines.clear();
		for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)
		{
			vector<vector<DPoint3d>> faceLinePts = *it;
			for (auto it : faceLinePts)
			{
				vector<DPoint3d> linePts = it;
				EditElementHandle eeh;
				LineStringHandler::CreateLineStringElement(eeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allLines.push_back(eeh.GetElementRef());
			}
		}
	}


	//�������--begin
	vector<vector<DSegment3d>> vctTieRebarLines;//�洢���е�����ֱ����Ϣ������Ԥ��
	if (0 != GetTieRebarInfo().tieRebarMethod/* && (m_vecAllRebarStartEnd.size() >= 4)*/)
	{
		FaceRebarDataArray faceDataArray;
		faceDataArray.posRebarData.HRebarData.rebarSize = GetvecDirSize().at(0);
		faceDataArray.posRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(0);
		faceDataArray.posRebarData.VRebarData.rebarSize = GetvecDirSize().at(1);
		faceDataArray.posRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(1);

		faceDataArray.revRebarData.HRebarData.rebarSize = GetvecDirSize().at(GetvecDirSize().size() - 1);
		faceDataArray.revRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecDirSize().size() - 1);
		faceDataArray.revRebarData.VRebarData.rebarSize = GetvecDirSize().at(GetvecDirSize().size() - 2);
		faceDataArray.revRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecDirSize().size() - 2);

		vector<vector<DSegment3d> > vecStartEnd;		//ֻ�洢1��2��͵�����1��2��
		vector<vector<DSegment3d> > vvecSeg;
		int index = 0;
		vector<DSegment3d> vecSeg;
		for (size_t i = 0; i < g_vecTieRebarPtsNoHole.size(); ++i)
		{
			if (index != g_vecTieRebarPtsNoHole[i].iIndex)
			{
				vvecSeg.push_back(vecSeg);
				vecSeg.clear();
			}
			DSegment3d seg = DSegment3d::From(g_vecTieRebarPtsNoHole[i].ptstr, g_vecTieRebarPtsNoHole[i].ptend);
			vecSeg.push_back(seg);
			index = g_vecTieRebarPtsNoHole[i].iIndex;
			if (i == g_vecTieRebarPtsNoHole.size() - 1)
			{
				vvecSeg.push_back(vecSeg);
			}
		}

		if (vvecSeg.size() < 4)
		{
			if (g_globalpara.Getrebarstyle() != 0)
			{
				return (SUCCESS == AddRebarSets(rsetTags));
			}
			return true;
		}

		vecStartEnd.push_back(vvecSeg[0]);
		vecStartEnd.push_back(vvecSeg[1]);
		vecStartEnd.push_back(vvecSeg[vvecSeg.size() - 2]);
		vecStartEnd.push_back(vvecSeg[vvecSeg.size() - 1]);
		BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
		int	tieRebarStyle = GetTieRebarInfo().tieRebarStyle;
		if (strTieRebarSize.Find(L"mm") != string::npos)
			strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//ɾ��mm

		PopvecSetId().push_back(0);
		TieRebarMaker tieRebarMaker(faceDataArray, vecStartEnd, (TieRebarStyle)tieRebarStyle, strTieRebarSize);
		tieRebarMaker.m_CallerId = GetCallerId();
		tieRebarMaker.SetCustomStyle(GetTieRebarInfo().rowInterval, GetTieRebarInfo().colInterval);
		tieRebarMaker.SetModeType(0);
		Transform trans;
		GetPlacement().AssignTo(trans);
		tieRebarMaker.SetTrans(trans);
		vector<EditElementHandle*> vecAllSolid;
		vecAllSolid.insert(vecAllSolid.begin(), m_Holeehs.begin(), m_Holeehs.end());
		vecAllSolid.insert(vecAllSolid.end(), m_Negs.begin(), m_Negs.end());
		tieRebarMaker.SetDownVec(m_STwallData.ptStart, m_STwallData.ptEnd);
		tieRebarMaker.SetHoles(vecAllSolid);
		tieRebarMaker.SetHoleCover(GetSideCover()*uor_per_mm);
		RebarSetTag* tag = tieRebarMaker.MakeRebar(PopvecSetId().at(GetvecSetId().size() - 1), modelRef);
		tieRebarMaker.GetRebarPts(vctTieRebarLines);//ȡ�����е�����ֱ����Ϣ
		if (NULL != tag && (!PreviewButtonDown))
		{
			tag->SetBarSetTag(iRebarLevelNum + 1);
			rsetTags.Add(tag);
		}
	}

	if (PreviewButtonDown)//Ԥ����ť���£���������
	{
		for (auto itr = vctTieRebarLines.begin(); itr != vctTieRebarLines.end(); itr++)
		{
			vector<DSegment3d> vcttemp(*itr);
			for (int y = 0; y < vcttemp.size(); y++)
			{
				DPoint3d strPoint = DPoint3d::From(vcttemp[y].point[0].x, vcttemp[y].point[0].y, vcttemp[y].point[0].z);
				DPoint3d endPoint = DPoint3d::From(vcttemp[y].point[1].x, vcttemp[y].point[1].y, vcttemp[y].point[1].z);
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allLines.push_back(eeh.GetElementRef());
			}
		}
		return true;
	}

	//end
	/*AddRebarSets(rsetTags);
	RebarSets rebar_sets;
	GetRebarSets(rebar_sets, ACTIVEMODEL);
	return true;*/
	//return true;
	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

long STWallRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}


bool STWallRebarAssembly::OnDoubleClick()
{
	//	vector<PIT::ConcreteRebar> vecRebarData;
	//	vector<PIT::LapOptions> vecLaptionData;
	//	vector<PIT::EndType> vecEndTypeData;
	//	vector<TwinBarSet::TwinBarLevelInfo> vecTwinBarData;
	//	Concrete concreteData;
	//	TwinBarSet::TwinBarInfo twInfo;
	//	TieReBarInfo tieRebarInfo;
		// 	GetRebarData(vecData);
	// 	GetConcreteData(concreteData);
	//	WallType wallType = GetwallType();
	// 	int lastAction = ACTIONBUTTON_CANCEL;
	// 	if (SUCCESS != mdlDialog_openModal(&lastAction, GetResourceHandle(), DIALOGID_WallRebar) || lastAction != ACTIONBUTTON_OK)
	// 		return false;
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pWallDoubleRebarDlg = new CWallRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pWallDoubleRebarDlg->SetSelectElement(ehSel);
	pWallDoubleRebarDlg->Create(IDD_DIALOG_WallRebar);
	pWallDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pWallDoubleRebarDlg->ShowWindow(SW_SHOW);

	//	CWallRebarDlg dlg(ehSel,CWnd::FromHandle(MSWIND));
	//	dlg.SetSelectElement(ehSel);
	// 	dlg.SetConcreteId(FetchConcrete());
	// 	if (IDCANCEL == dlg.DoModal())
	// 		return false;
	//	dlg.m_PageMainRebar.GetConcreteData(concreteData);
	//	dlg.m_PageMainRebar.GetListRowData(vecRebarData);
	//	dlg.m_PageLapOption.GetListRowData(vecLaptionData);
	//	dlg.m_PageEndType.GetListRowData(vecEndTypeData);
	//	dlg.m_PageTwinBars.GetTwinBarInfo(twInfo);
	//	dlg.m_PageTwinBars.GetListRowData(vecTwinBarData);
	//	dlg.m_PageTieRebar.GetTieRebarData(tieRebarInfo);
	//	SetwallType(wallType);
	//	SetWallData(ehSel);
	//	SetConcreteData(concreteData);
	//	SetRebarData(vecRebarData);
	//	SetvecLapOptions(vecLaptionData);
	//	SetRebarEndTypes(vecEndTypeData);
	//	SetvecTwinRebarLevel(vecTwinBarData);
	//	InitRebarSetId();
	//	SetTieRebarInfo(tieRebarInfo);
	//	MakeRebars(modelRef);
	//	Save(modelRef);
	//	ElementId conid = GetConcreteOwner();
	//	SetConcreteXAttribute(conid, ACTIVEMODEL);
	//	SetElementXAttribute(GetSelectedElement(), sizeof(ElementId), &conid, ConcreteIDXAttribute, GetSelectedModel());
	//	ACCConcrete concrete;
	//	concrete.postiveOrTopCover = g_wallRebarInfo.concrete.postiveCover;
	//	concrete.reverseOrBottomCover = g_wallRebarInfo.concrete.reverseCover;
	//	concrete.sideCover = g_wallRebarInfo.concrete.sideCover;
	//	SetElementXAttribute(ehSel.GetElementId(), sizeof(ACCConcrete), &concrete, ConcreteCoverXAttribute, ehSel.GetModelRef());
	//	Transform trans;
	//	GetPlacement().AssignTo(trans);
	//	SetElementXAttribute(conid, sizeof(Transform), &trans, UcsMatrixXAttribute, ACTIVEMODEL);
	//	SetElementXAttribute(conid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
	//	SetElementXAttribute(conid, g_vecTwinRebarPtsNoHole, vecTwinRebarPointsXAttribute, ACTIVEMODEL);
	//	ElementId contid = GetConcreteOwner();
	return true;
}

bool STWallRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();

	SetWallData(ehWall);
	MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();
	//SetElementXAttribute(ehSel.GetElementId(), g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ehSel.GetModelRef());
	//eeh2.AddToModel();
	return true;
}


bool STWallRebarAssembly::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;
	InitUcsMatrix();
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}

//��ȡ����
double GetFloorThickness(EditElementHandleR Eleeh)
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

bool get_value1(vector<ElementId> vec_walls, EditElementHandle& tmpeeh)
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
void GetLeftRightWallFaces(WallRebarAssembly::WallData& walldata, EditElementHandleR eeh, string wallname)
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
void GetUpDownFloorFaces(WallRebarAssembly::WallData& walldata, EditElementHandleR eeh)
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
					if ((maxP2.z >= maxP.z && minP2.z > minP.z))//�ж��Ƿ�Ϊ����
					{
						walldata.upfloorfaces.push_back(downface);
						walldata.upfloorth = thick;
						walldata.upfloorID.push_back(Same_Eles.at(i));
					}
					else if ((maxP2.z < maxP.z && minP2.z <= minP.z)|| (maxP2.z < maxP.z && minP2.z >= minP.z)||
						(maxP2.z >= maxP.z && minP2.z < minP.z) || (minP.z >= maxP2.z))//�ж��Ƿ�Ϊ�װ�
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
bool STWallRebarAssembly::AnalyzingWallGeometricData(ElementHandleCR eh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	auto type = JudgeElementType(eh);
	DPoint3d FrontStr, FrontEnd;
	DPoint3d BackStr, BackEnd;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();
	m_doorsholes.clear();
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);//��ȡ����֮ǰ��ǽ��

	//1����ǽ��Ӳο�ģ�Ϳ�������ǰģ��
	EditElementHandle copyEleeh;
	copyEleeh.Duplicate(Eleeh);
	ElementCopyContext copier2(ACTIVEMODEL);
	copier2.SetSourceModelRef(Eleeh.GetModelRef());
	copier2.SetTransformToDestination(true);
	copier2.SetWriteElements(false);
	copier2.DoCopy(copyEleeh);
	m_walldata.ClearData();
	//2����ȡ�ϲ����ǽ�����������ͶӰ�����е���ĺϲ�
	m_walldata.downFace = ExtractFacesTool::GetCombineDownFace(copyEleeh);

	if (m_walldata.downFace != nullptr)
	{
		//mdlElmdscr_add(m_walldata.downFace);
	}
	//3����ȡǽ�����ǰ���ߴ��ͺ��
	m_walldata.vecFontLine.clear();
	m_walldata.vecBackLine.clear();
	ExtractFacesTool::GetFrontBackLinePoint(m_walldata.downFace, m_walldata.vecFontLine, m_walldata.vecBackLine, m_walldata.thickness);

	DPoint3d minP2, maxP2;
	//����ָ��Ԫ����������Ԫ�صķ�Χ��
	mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
	if (!Eleeh.IsValid())
	{
		//mdlDialog_dmsgsPrint(L"�Ƿ���ǽʵ��!");
		return false;
	}
	DPoint3d minPos;
	EditElementHandle downface;
	if (!ExtractFacesTool::GetFloorDownFace(copyEleeh, downface, minPos, true))
	{
		return false;
	}
	DVec3d vecZ = DVec3d::UnitZ();
	DPoint3d facenormal;
	minPos = minP2;
	minPos.Add(maxP2);
	minPos.Scale(0.5);
	mdlElmdscr_extractNormal(&facenormal, nullptr, downface.GetElementDescrP(), NULL);
	facenormal.Scale(-1);
	m_walldata.vecdown = DVec3d::From(facenormal.x, facenormal.y, facenormal.z);
	RotMatrix rMatrix;
	Transform trans;
	mdlRMatrix_getIdentity(&rMatrix);
	mdlRMatrix_fromVectorToVector(&rMatrix, &facenormal, &vecZ);//��ת��xoyƽ��
	mdlTMatrix_fromRMatrix(&trans, &rMatrix);
	mdlTMatrix_setOrigin(&trans, &minPos);
	copyEleeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(copyEleeh, TransformInfo(trans));
	downface.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(downface, TransformInfo(trans));

	GetDoorHoles(Holeehs, m_doorsholes);

	if (m_walldata.vecFontLine.empty() || m_walldata.vecBackLine.empty())
	{
		return false;
	}



	DPoint3d pt1[2];
	m_walldata.vecFontLine[0].GetStartPoint(pt1[0]);
	m_walldata.vecFontLine[0].GetEndPoint(pt1[1]);

	DPoint3d pt2[2];
	m_walldata.vecBackLine[0].GetStartPoint(pt2[0]);
	m_walldata.vecBackLine[0].GetEndPoint(pt2[1]);

	DPoint3d vecLine = pt1[1] - pt1[0];
	vecLine.Normalize();
	m_walldata.vecToWall = facenormal;
	m_walldata.vecToWall.CrossProduct(m_walldata.vecToWall, vecLine);


	if (m_walldata.vecBackLine.size() > 1 || m_walldata.vecFontLine.size() > 1)
	{
		GetMaxDownFacePts(m_walldata.vecFontLine, m_walldata.vecBackLine, pt1, pt2);
	}

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_STwallData.height);
	//����ָ��Ԫ����������Ԫ�صķ�Χ��
	mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
	m_walldata.height = (maxP2.z - minP2.z) / uor_now;
	m_STwallData.height = (maxP2.z - minP2.z)*uor_now / uor_ref;
	m_STwallData.width = FrontStr.Distance(BackStr)*uor_now / uor_ref;
	m_STwallData.length = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;
	m_STwallData.ptStart = FrontStr;
	m_STwallData.ptEnd = FrontEnd;
	m_width = m_STwallData.width;




	FrontStr.Add(FrontEnd);
	FrontStr.Scale(0.5);
	BackStr.Add(BackEnd);
	BackStr.Scale(0.5);
	m_LineNormal = BackStr - FrontStr;
	m_LineNormal.Normalize();

	m_Negs = Negs;
	if (Negs.size() > 0)//STWALL��б��
	{

		DPoint3d vecBack = pt2[0] - pt2[1];
		DPoint3d vecLeft = pt1[0] - pt2[0];
		DPoint3d vecRight = pt1[1] - pt2[1];

		vecBack.Normalize();
		vecLeft.Normalize();
		vecRight.Normalize();

		m_angle_left = vecBack.AngleTo(vecLeft);
		if (m_angle_left > PI / 2)
		{
			m_angle_left = PI - m_angle_left;
		}
		m_angle_right = vecBack.AngleTo(vecRight);
		if (m_angle_right > PI / 2)
		{
			m_angle_right = PI - m_angle_right;
		}

	}
	else
	{
		m_angle_left = PI / 2;
		m_angle_right = PI / 2;
	}
	m_Holeehs = Holeehs;

	string Ename, Etype;
	string wallname = GetwallName();
	if (GetEleNameAndType(testeeh, Ename, Etype))
	{
		wallname = Ename;
	}

	GetUpDownFloorFaces(m_walldata, testeeh);
	GetLeftRightWallFaces(m_walldata, testeeh, wallname);
	return true;
}

void STWallRebarAssembly::InitUcsMatrix()
{
	DPoint3d ptStart = m_STwallData.ptStart;
	DPoint3d ptEnd = m_STwallData.ptEnd;

	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
	yVec.Normalize();
	//	CVector3D  yVecNegate = yVec;
	//	yVecNegate.Negate();
	//	yVecNegate.Normalize();
	//	yVecNegate.ScaleToLength(m_STwallData.width);//������յ�����Ϊǽ�������ߣ����Դ˴�ֻ��Ҫƫ��1/2���
	//	ptStart.Add(yVecNegate);
	//	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(ptStart, ptEnd);
	xVecNew.Normalize();
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//����ΪX�ᣬˮƽ��ֱ����ΪY��
	SetPlacement(placement);
	CMatrix3D   mat = placement;
	mat = mat.Inverse();
	/*mat.m[2][0] = CVector3D::kZaxis.x;
	mat.m[2][1] = CVector3D::kZaxis.y;
	mat.m[2][2] = CVector3D::kZaxis.z;
	mat = mat.Inverse();
	placement = mat;
	SetPlacement(mat);

	mat = mat.Inverse();*/
	Transform trans;
	mat.AssignTo(trans);
	DPoint3d pt1 = ptStart;
	DPoint3d pt2 = ptEnd;
	EditElementHandle lineeeh;
	LineHandler::CreateLineElement(lineeeh, nullptr, DSegment3d::From(pt1, pt2), true, *ACTIVEMODEL);
	lineeeh.GetHandler().ApplyTransform(lineeeh, TransformInfo(trans));

	DPoint3d Vpt1[2];
	mdlLinear_extract(Vpt1, NULL, lineeeh.GetElementP(), ACTIVEMODEL);
	m_VecX = Vpt1[1] - Vpt1[0];
	m_VecX.Normalize();
	//lineeeh.AddToModel();

	pt1 = ptStart;
	pt2 = ptStart;
	pt2.z = pt2.z + 40000;
	EditElementHandle lineeeh2;
	LineHandler::CreateLineElement(lineeeh2, nullptr, DSegment3d::From(pt1, pt2), true, *ACTIVEMODEL);
	lineeeh2.GetHandler().ApplyTransform(lineeeh2, TransformInfo(trans));
	DPoint3d Vpt2[2];
	mdlLinear_extract(Vpt2, NULL, lineeeh2.GetElementP(), ACTIVEMODEL);
	m_VecZ = Vpt2[1] - Vpt2[0];
	m_VecZ.Normalize();
	//lineeeh2.AddToModel();

	DPoint3d tmppt = ptStart;
	yVec.Scale(m_STwallData.height);
	tmppt.Add(yVec);



	/*DPoint3d vecY = yVec;
	vecY.Normalize();
	vecY.Scale(m_STwallData.width / 5);
	ptStart.Add(vecY);
	ptEnd.Add(vecY);*/
	PopvecFrontPts().push_back(ptStart);
	PopvecFrontPts().push_back(ptEnd);
}
bool GWallRebarAssembly::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;
	//InitUcsMatrix();
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}

void GWallRebarAssembly::JudgeGWallType(ElementHandleCR eh)
{
	//ͨ���ֽ�Ԫ�����ж�Ԫ�ص�����
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFrontLine;
	vector<MSElementDescrP> vecDownBackLine;
	EditElementHandle testeeh(eh, false);
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);


	for (int j = 0; j < Holeehs.size(); j++)
	{
		delete Holeehs.at(j);
		Holeehs.at(j) = nullptr;
	}
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFrontLine, vecDownBackLine, NULL);
	if (vecDownFaceLine.empty() || vecDownFrontLine.empty() || vecDownBackLine.empty() || vecDownFrontLine.size() != vecDownBackLine.size())
	{
		m_GWallType = Custom;
		return;
	}

	int iLineFlag = 0;
	int iArcFlag = 0;
	for (size_t i = 0; i < vecDownFaceLine.size(); i++)
	{
		if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
			iArcFlag++;
		else
			iLineFlag++;

		mdlElmdscr_freeAll(&vecDownFaceLine[i]);
	}

	if (iArcFlag >= 2 && iLineFlag > 2)	//����������2��������2�������߶�
		m_GWallType = LineAndArcWALL;
	else if (!iArcFlag && iLineFlag >= 4)	//����ȫ�����߶�
	{
		m_GWallType = LineWall;
		for (size_t i = 0; i < vecDownFrontLine.size(); i++)
		{
			DPoint3d pt1[2], pt2[2];
			mdlLinear_extract(pt1, NULL, &vecDownFrontLine[i]->el, eh.GetModelRef());
			mdlLinear_extract(pt2, NULL, &vecDownBackLine[i]->el, eh.GetModelRef());
			//�ж��Ƿ�ƽ��
			DVec3d vec = DVec3d::From(pt1[1] - pt1[0]);
			DVec3d vec1 = DVec3d::From(pt2[1] - pt2[0]);
			vec.Normalize();
			vec1.Normalize();
			double dot = mdlVec_dotProduct(&vec, &vec1);
			if (COMPARE_VALUES(fabs(dot), 1) != 0)
			{
				m_GWallType = Custom;
				return;
			}
		}
	}
	else
		m_GWallType = ArcWall;
}
//
//bool GWallRebarAssembly::AnalyzingWallGeometricData(ElementHandleCR eh)
//{
//	DgnModelRefP model = eh.GetModelRef();
//	double uor_per_mm = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
//
//	JudgeGWallType(eh);
//	m_GWallData.vecPositivePt.clear();
//	m_GWallData.vecLength.clear();
//	vector<MSElementDescrP> vecDownFaceLine;
//	vector<MSElementDescrP> vecDownFrontLine;
//	vector<MSElementDescrP> vecDownBackLine;
//	EditElementHandle testeeh(eh, false);
//	EditElementHandle Eleeh;
//	std::vector<EditElementHandle*> Holeehs;
//	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
//	for (int j = 0; j < Holeehs.size(); j++)
//	{
//		delete Holeehs.at(j);
//		Holeehs.at(j) = nullptr;
//	}
//	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFrontLine, vecDownBackLine, &m_GWallData.height);
//	if (vecDownFaceLine.empty() || vecDownFrontLine.empty() || vecDownBackLine.empty() || vecDownFrontLine.size() != vecDownBackLine.size())	
//		return false;	//��ʱ�������Զ�������ǽ
//
//	switch (m_GWallType)
//	{
//	case GWallRebarAssembly::LineWall:
//	{
//		DPoint3d ptFront1, ptFront2, ptBack1, ptBack2;
//		for (size_t i = 0; i < vecDownFrontLine.size(); i++)
//		{
//			DPoint3d pt1[2], pt2[2];
//			mdlLinear_extract(pt1, NULL, &vecDownFrontLine[i]->el, model);
//			mdlLinear_extract(pt2, NULL, &vecDownBackLine[i]->el, model);
//			double dLength1 = mdlVec_distance(&pt1[0], &pt1[1]);
//			double dLength2 = mdlVec_distance(&pt2[0], &pt2[1]);
//// 			if (COMPARE_VALUES(dLength1, dLength2) != 0)
//// 			{
//// 				mdlDialog_dmsgsPrint(L"GTWALL����ǽ������ױ߳��Ȳ�һ��");
//// 				return false;
//// 			}
//			if (0 == i)
//			{
//				ptFront1 = pt1[0];
//				ptFront2 = pt1[1];
//				ptBack1 = pt2[1];
//				ptBack2 = pt2[0];
//				m_GWallData.vecPositivePt.push_back(pt1[0]);
//				m_GWallData.vecPositivePt.push_back(pt1[1]);
//				m_GWallData.vecReversePt.push_back(pt2[1]);
//				m_GWallData.vecReversePt.push_back(pt2[0]);
//			}
//			else
//			{
//				m_GWallData.vecPositivePt.push_back(pt1[1]);
//				m_GWallData.vecReversePt.push_back(pt2[0]);
//			}
//			m_GWallData.vecLength.push_back(dLength1);
//		}
//		//������ĵ�����ʹ֮������ĵ�һһ��Ӧ��
////		std::reverse(m_GWallData.vecReversePt.begin(), m_GWallData.vecReversePt.end());
//
//		//������ĵױߵ����ͶӰ���������ϣ����������ͶӰ��ľ��뼴Ϊǽ���
//		DPoint3d ptProject1;	//ͶӰ��
//		mdlVec_projectPointToLine(&ptProject1, NULL, &ptBack1, &ptBack2, &ptFront1);
//		m_GWallData.thickness = ptFront1.Distance(ptProject1);
//
//		vector<CPoint3D> vecRebarVertex;
//		vecRebarVertex.resize(m_GWallData.vecPositivePt.size());
//		for (size_t i = 0; i < GetRebarLevelNum(); i++)
//			m_vvRebarVertex.push_back(vecRebarVertex);
//
//
//		for (size_t i = 0; i < m_GWallData.vecPositivePt.size(); i++)
//		{
//			double dOffsetY = 0.0;
//			double dOffsetX = GetSideCover()*10;
//			for (size_t j = 0; j < GetRebarLevelNum(); j++)
//			{
//				m_vvRebarVertex[j][i] = m_GWallData.vecPositivePt[i];
//				CVector3D vec = m_GWallData.vecReversePt[i] - m_vvRebarVertex[j][i];
//				vec.Normalize();
//				double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(j), model);
//				dOffsetY = GetPositiveCover() * 10 + GetvecLevelSpace().at(j)*10 + diameter*0.5;
//				if (COMPARE_VALUES(dOffsetY,m_GWallData.thickness-GetReverseCover()*uor_per_mm) >= 0)
//					dOffsetY = m_GWallData.thickness - GetReverseCover()*10 - diameter * 0.5;
//				vec.ScaleToLength(dOffsetY);
//				m_vvRebarVertex[j][i].Add(vec);
//				CVector3D vec1 = m_GWallData.vecReversePt[i + 1] - m_GWallData.vecReversePt[i];
//				vec1.Normalize();
//				if (0 == i)
//				{
//					dOffsetX += GetvecStartOffset().at(j)*10;
//				}
//				if (i == m_GWallData.vecPositivePt.size() - 1)
//				{
//					vec1.Negate();
//					dOffsetX += GetvecEndOffset().at(j) * 10;
//
//				}
//				vec1.ScaleToLength(dOffsetX);
//
//				m_vvRebarVertex[j][i].Add(vec1);
//
//			}
//		}
//	}
//		break;
//	case GWallRebarAssembly::ArcWall:
//	{
//		//mdlArc_extract(DPoint3dP startEndPts,double *start, double *sweep, double *axis1, double *axis2, RotMatrixP  rotMatrix, DPoint3dP  center, MSElementCP  in)
//		double dRadius1, dRadius2;
//		for (size_t i = 0; i < vecDownFrontLine.size(); i++)
//		{
//			DPoint3d pt1[2], pt2[2];
//			DPoint3d ptCenter1, ptCenter2;
//			mdlArc_extract(pt1, NULL, NULL, &dRadius1, NULL, NULL, &ptCenter1,&vecDownFrontLine[i]->el);
//			mdlArc_extract(pt2, NULL, NULL, &dRadius2, NULL, NULL, &ptCenter2,&vecDownBackLine[i]->el);
//			m_GWallData.vecPositivePt.push_back(ptCenter1);
//			if (0 == i)
//			{
//				m_GWallData.vecPositivePt.push_back(pt1[0]);
//				m_GWallData.vecReversePt.push_back(pt2[0]);
//			}
//			m_GWallData.vecPositivePt.push_back(pt1[1]);
//			m_GWallData.vecReversePt.push_back(pt2[1]);
//			m_GWallData.vecLength.push_back(dRadius1);
//		}
//
//		m_GWallData.thickness = dRadius2 - dRadius1;
//	}
//		break;
//	case GWallRebarAssembly::LineAndArcWALL:
//		break;
//	case GWallRebarAssembly::Custom:
//		break;
//	default:
//		break;
//	}
//
//	return true;
//}

bool GWallRebarAssembly::AnalyzingWallGeometricData(ElementHandleCR eh)
{
	DgnModelRefP model = eh.GetModelRef();
	double uor_per_mm = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	m_vecLinePts.clear();
	m_doorsholes.clear();
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFrontLine;
	vector<MSElementDescrP> vecDownBackLine;
	EditElementHandle testeeh(eh, false);
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	GetDoorHoles(Holeehs, m_doorsholes);
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFrontLine, vecDownBackLine, &m_GWallData.height);
	if (vecDownFaceLine.empty() || vecDownFrontLine.empty() || vecDownBackLine.empty() || vecDownFrontLine.size() != vecDownBackLine.size())
		return false;	//��ʱ�������Զ�������ǽ

	if (vecDownFrontLine.size() != vecDownBackLine.size())//������Ҫ��һ��
	{
		return false;
	}

	vector<vector<DPoint3d>>  allfrontpts;
	vector<vector<DPoint3d>>  allbackpts;
	double frontlenth = 0;
	double backlenth = 0;
	for (size_t i = 0; i < vecDownFrontLine.size(); i++)
	{
		vector<DPoint3d> pts;
		DPoint3d pt1[2];
		mdlLinear_extract(pt1, NULL, &vecDownFrontLine[i]->el, model);
		double dLength1 = mdlVec_distance(&pt1[0], &pt1[1]);
		frontlenth = frontlenth + dLength1;
		pts.push_back(pt1[0]);
		pts.push_back(pt1[1]);
		allfrontpts.push_back(pts);

		vector<DPoint3d> pts2;
		DPoint3d pt2[2];
		mdlLinear_extract(pt2, NULL, &vecDownBackLine[i]->el, model);
		double dLength2 = mdlVec_distance(&pt2[0], &pt2[1]);
		backlenth = backlenth + dLength2;
		pts2.push_back(pt2[0]);
		pts2.push_back(pt2[1]);
		allbackpts.push_back(pts2);
	}
	if (backlenth < frontlenth)
	{
		vector<vector<DPoint3d>>  tmppts;
		tmppts = allfrontpts;
		allfrontpts = allbackpts;
		allbackpts = tmppts;
		std::reverse(allbackpts.begin(), allbackpts.end());
		std::reverse(allfrontpts.begin(), allfrontpts.end());
	}

	for (int i = 0; i < allfrontpts.size(); i++)
	{
		vector<DPoint3d> pts = allfrontpts.at(i);
		vector<DPoint3d> pts2 = allbackpts.at(i);
		m_vecLinePts[i].insert(m_vecLinePts[i].begin(), pts.begin(), pts.end());
		m_vecLinePts[i].insert(m_vecLinePts[i].end(), pts2.begin(), pts2.end());
		/*if (i==0)
		{
			EditElementHandle eehlinefront;
			LineHandler::CreateLineElement(eehlinefront, nullptr, DSegment3d::From(pts[0], pts[1]), true, *ACTIVEMODEL);
			eehlinefront.AddToModel();
		}
			EditElementHandle eehlineback;
			LineHandler::CreateLineElement(eehlineback, nullptr, DSegment3d::From(pts2[0], pts2[1]), true, *ACTIVEMODEL);
			eehlineback.AddToModel();*/

	}
	m_Holeehs = Holeehs;
	return true;
}


void GWallRebarAssembly::CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	vTransform.clear();
	vTransformTb.clear();
	double updownSideCover = 50 * uor_per_mm;
	double dSideCover = m_sidecover * uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double dLevelSpace = 0;
	double dOffset = dPositiveCover;
	double dOffsetTb = dPositiveCover;
	double diameterTie = 0.0;
	BrString strTieRebarSize(GetTieRebarInfo().rebarSize);
	if (strTieRebarSize != L""/*&& 0 != GetTieRebarInfo().tieRebarMethod*/)
	{
		diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//����ֱ��
	}

	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}
		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//������10
		double diameterTb = 0.0;
		if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
		{
			diameterTb = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(i).rebarSize, modelRef);		//������10
		}

		if (diameter > BE_TOLERANCE)
		{
			CVector3D	zTrans(0.0, 0.0, 0.0);
			CVector3D	zTransTb;
			if (GetvecDir().at(i) == 0) //ˮƽ
			{
				zTrans.z = updownSideCover - diameter * 0.5;
				zTrans.x = m_STwallData.length * 0.5;
				zTransTb = zTrans;
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
				{
					if (GetvecTwinRebarLevel().at(i).hasTwinbars)//��ǰ�ֽ�Ϊ�����
					{
						zTransTb.z = zTransTb.z + diameter + diameterTb;
					}
				}
			}
			else
			{
				zTrans.z = m_STwallData.height * 0.5;
				zTrans.x = updownSideCover + diameter * 0.5;
				zTransTb = zTrans;
			}
			{
				WString strSizePre;
				if (i != 0)
				{
					strSizePre = WString(GetvecDirSize().at(i - 1).Get());
					if (strSizePre.find(L"mm") != WString::npos)
					{
						strSizePre.ReplaceAll(L"mm", L"");
					}
				}

				double diameterPre = RebarCode::GetBarDiameter(strSizePre, modelRef);		//������10
				if (0 == i)
				{
					dOffset += diameter / 2.0;	//ƫ���ײ�ֽ�뾶
					dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm;
				}
				else
				{
					dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm + diameter * 0.5 + diameterPre * 0.5;//������ϵ�ǰ�ֽ�ֱ��
				}

				dOffset += dLevelSpace;
				dOffsetTb = dOffset;
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
				{
					if (diameterTb > diameter)//�����ĸֽ������ֱ����
						dOffsetTb += (diameterTb / 2.0 - diameter / 2.0);
					else
						dOffsetTb -= (diameter / 2.0 - diameterTb / 2.0);
				}
				if (COMPARE_VALUES(m_STwallData.width - dOffset, dReverseCover) < 0)		//��ǰ�ֽ����Ƕ�뵽�˷��汣������ʱ��ʵ�ʲ��õĸֽ����Ͳ���ʹ�����õ����ϲ��࣬����ʹ�ñ������������
				{
					zTrans.y = m_STwallData.width - dReverseCover - diameter / 2.0 - diameterTie;
					zTransTb.y = zTrans.y;
					if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
					{
						if (diameterTb > diameter)//�����ĸֽ������ֱ����
							zTransTb.y += (diameterTb / 2.0 - diameter / 2.0);
						else
							zTransTb.y -= (diameter / 2.0 - diameterTb / 2.0);
					}
					//�жϣ������һ���zTrans.y�뵱ǰ���zTrans.y��ͬ������һ���ȥ��ǰ��ĸֽ�ֱ��������ֹ�ֽ���ײ��
					double compare = zTrans.y;
					if (vTransform.size() > 0)
					{
						double reverseOffset = diameter;
						for (int j = (int)vTransform.size() - 1; j >= 0; j--)
						{
							WString strSize1 = GetvecDirSize().at(j);
							if (strSize1.find(L"mm") != WString::npos)
							{
								strSize1.ReplaceAll(L"mm", L"");
							}
							diameterPre = RebarCode::GetBarDiameter(strSize1, modelRef);		//������10
							if (COMPARE_VALUES(vTransform[j].y + diameterPre * 0.5, compare - diameter * 0.5) > 0)	//Ƕ������һ���ֽ���
							{
								if (j == vTransform.size() - 1)//Ϊ��ǰ�����ĵ�һԪ��
								{
									vTransform[j].y = zTrans.y;
									vTransformTb[j].y = vTransform[j].y;
								}
								else
								{
									vTransform[j].y = vTransform[j + 1].y;
									vTransformTb[j].y = vTransform[j].y;
								}

								vTransform[j].y -= reverseOffset;
								vTransformTb[j].y = vTransform[j].y;
								if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(j).hasTwinbars)
								{
									double diameterTbPre = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(j).rebarSize, modelRef);		//������10

									if (diameterTbPre > diameterPre)//�����ĸֽ������ֱ����
										vTransformTb[j].y -= (diameterTbPre / 2.0 - diameterPre / 2.0);
									else
										vTransformTb[j].y += (diameterPre / 2.0 - diameterTbPre / 2.0);
								}
								compare = vTransform[j].y;
								diameter = diameterPre;
							}
						}
					}
				}
				else
				{
					zTrans.y = dOffset;
					zTransTb.y = dOffsetTb;
					// 					if (GetvecTwinRebarLevel().at(i).hasTwinbars && diameterTb > diameter)	//�����ĸֽ������ֱ����
					// 					{
					// 						zTrans.y -= (diameterTb / 2.0 - diameter / 2.0) * 2;
					// 						zTransTb.y -= (diameterTb / 2.0 - diameter / 2.0);
					// 					}
				}
			}
			//			zTransTb = zTrans;
			vTransform.push_back(zTrans);
			vTransformTb.push_back(zTransTb);
		}
	}
}

bool GWallRebarAssembly::makeLineWallRebarCurve(RebarCurve & rebar, int dir, vector<CPoint3D> const& vecRebarVertex, double bendRadius, double bendLen, RebarEndTypes const & endTypes, CVector3D const & endNormal, CMatrix3D const & mat)
{
	RebarVertexP vex;
	CPoint3D  startPt;
	CPoint3D  endPt;

	if (0 == dir)	//����ֽ�
	{
		for (size_t i = 0; i < vecRebarVertex.size(); i++)
		{
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(vecRebarVertex[i]);
			if (0 == i)
				vex->SetType(RebarVertex::kStart);
			else if (vecRebarVertex.size() - 1 == i)
				vex->SetType(RebarVertex::kEnd);
			else
				vex->SetType(RebarVertex::kIP);
		}
		rebar.EvaluateBend(bendRadius);
		// 		EditElementHandle eeh;
		// 		LineStringHandler::CreateLineStringElement(eeh, NULL, &vecRebarVertex[0], 3, true, *ACTIVEMODEL);
		// 		eeh.AddToModel();
	}
	else			//����ֽ�
	{
		// 		startPt = CPoint3D::From(xPos, 0.0, yLen / 2.0 - startOffset);
		// 		endPt = CPoint3D::From(xPos, 0.0, -yLen / 2.0 + endOffset);
		// 
		// 		vex = &rebar.PopVertices().NewElement();
		// 		vex->SetIP(startPt);
		// 		vex->SetType(RebarVertex::kStart);
		// 
		// 		vex = &rebar.PopVertices().NewElement();
		// 		vex->SetIP(endPt);
		// 		vex->SetType(RebarVertex::kEnd);
	}


	//	rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
	//rebar.DoMatrix(mat);
	return true;
}

RebarSetTag * GWallRebarAssembly::MakeRebars_Transverse
(
	ElementId & rebarSetId,
	BrStringCR sizeKey,
	vector<CPoint3D> vecPt,
	double spacing,
	CVector3D const & endNormal,
	CMatrix3D const & mat,
	bool bTwinBars,
	DgnModelRefP modelRef
)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endType;
	endType.SetType(RebarEndType::kCog);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
	double bendLen = RebarCode::GetBendLength(sizeKey, endType, modelRef);	//������100

	double adjustedXLen = m_GWallData.height;

	adjustedXLen -= (2.0 * m_sidecover*uor_per_mm + diameter);

	int numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;

	double adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);

	// 	double xPos = -adjustedXLen / 2.0;
	double xPos = 0.0;
	if (bTwinBars)
		numRebar *= 2;
	for (int i = 0; i < numRebar; i++)
	{
		RebarCurve      rebarCurve;
		RebarEndTypes   endTypes = { endType, endType };
		CPoint3D ptStart;
		for (size_t i = 0; i < vecPt.size(); i++)
		{
			vecPt[i].z += xPos;
		}
		makeLineWallRebarCurve(rebarCurve, 0, vecPt, bendRadius, bendLen, endTypes, endNormal, mat);

		RebarSymbology symb;
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);

		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(i, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);

			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
		}
		if (bTwinBars && !(i % 2))
			xPos += diameter;
		else
		{
			xPos = adjustedSpacing;
			if (bTwinBars)
				xPos -= diameter;	//ɾ���ϴβ���ƫ�ƾ���
		}
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

RebarSetTag * GWallRebarAssembly::MakeRebars_Longitudinal(ElementId& rebarSetId, BrStringCR sizeKey, double &xDir, const vector<double> height, double spacing, double startOffset, double endOffset, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef)
{
	return nullptr;
}
void GWallRebarAssembly::GetMaxThickness(DgnModelRefP modelRef, double& thickness)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = GetSelectedModel()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	for (int i = 0; i < m_vecLinePts.size(); i++)
	{
		if (m_vecLinePts[i].size() != 4)
		{
			continue;
		}
		DPoint3d FrontStr = m_vecLinePts[i].at(0);
		DPoint3d FrontEnd = m_vecLinePts[i].at(1);
		DPoint3d BackStr;	//ͶӰ��
		mdlVec_projectPointToLine(&BackStr, NULL, &FrontStr, &m_vecLinePts[i].at(2), &m_vecLinePts[i].at(3));
		DPoint3d BackEnd;	//ͶӰ��
		mdlVec_projectPointToLine(&BackEnd, NULL, &FrontEnd, &m_vecLinePts[i].at(2), &m_vecLinePts[i].at(3));
		double tmpth = FrontStr.Distance(BackStr)*uor_per_mm / uor_ref;
		if (i == 0)
		{
			thickness = tmpth;
		}
		else if (thickness < tmpth)
		{
			thickness = tmpth;
		}
	}
}
bool GWallRebarAssembly::GetUcsAndStartEndData(int index, double thickness, DgnModelRefP modelRef, bool isSTGWALL)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = GetSelectedModel()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	DPoint3d FrontStr = m_vecLinePts[index].at(0);
	DPoint3d FrontEnd = m_vecLinePts[index].at(1);

	while (0)
	{
		if (thickness >= MaxWallThickness * uor_per_mm && (index + 1) < m_vecLinePts.size())//���¼����յ�
		{
			if (m_vecLinePts[index + 1].size() == 4)
			{
				DPoint3d BackStr;	//ͶӰ��
				mdlVec_projectPointToLine(&BackStr, NULL, &FrontStr, &m_vecLinePts[index].at(2), &m_vecLinePts[index].at(3));
				DVec3d   vec1 = BackStr - FrontStr;
				vec1.Normalize();

				DPoint3d FrontStr2 = m_vecLinePts[index + 1].at(0);
				DPoint3d FrontEnd2 = m_vecLinePts[index + 1].at(1);
				DPoint3d BackStr2;	//�ڶ����ߵ�ͶӰ��
				mdlVec_projectPointToLine(&BackStr2, NULL, &FrontStr2, &m_vecLinePts[index + 1].at(2), &m_vecLinePts[index + 1].at(3));
				DVec3d   vec2 = BackStr2 - FrontStr2;
				vec2.Normalize();

				double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(0), modelRef);//ȡ��ǰ��һ��ĸֽ�ֱ��
				double spacing = GetPositiveCover()*uor_per_mm;

				vec1.Scale(diameter + spacing);
				vec2.Scale(diameter + spacing);
				DPoint3d addStr1, addEnd1, addStr2, addEnd2;
				mdlVec_addPoint(&addStr1, &FrontStr, &vec1);
				mdlVec_addPoint(&addEnd1, &FrontEnd, &vec1);

				mdlVec_addPoint(&addStr2, &FrontStr2, &vec2);
				mdlVec_addPoint(&addEnd2, &FrontEnd2, &vec2);

				DPoint3d  intercept;
				DSegment3d L1 = DSegment3d::From(addStr1, addEnd1);
				DSegment3d L2 = DSegment3d::From(addStr2, addEnd2);
				int ret = mdlVec_intersect(&intercept, &L1, &L2);
				if (ret != SUCCESS)
				{
					break;
				}

				//EditElementHandle eeh1, eeh2;
				//LineHandler::CreateLineElement(eeh1, nullptr, DSegment3d::From(addStr1, intercept), true, *ACTIVEMODEL);
				//eeh1.AddToModel();

				//LineHandler::CreateLineElement(eeh2, nullptr, DSegment3d::From(intercept, addEnd2), true, *ACTIVEMODEL);
				//eeh2.AddToModel();

				DVec3d vecendstr1, vecendstr2;
				vecendstr1 = addEnd1 - addStr1;
				vecendstr2 = addStr2 - addEnd2;
				vecendstr1.Normalize();
				vecendstr2.Normalize();
				vecendstr1.Scale(diameter / 2);
				vecendstr2.Scale(diameter / 2);

				DPoint3d tmpPt1, tmpPt2;
				mdlVec_addPoint(&tmpPt1, &intercept, &vecendstr1);
				mdlVec_addPoint(&tmpPt2, &intercept, &vecendstr2);

				ArcData data;
				IntersectionPointToArcData(data, intercept, tmpPt1, tmpPt2, diameter / 2);

				mdlVec_addPoint(&tmpPt1, &data.ptArcBegin, &vecendstr1);
				mdlVec_addPoint(&tmpPt2, &data.ptArcEnd, &vecendstr2);

				/*	EditElementHandle tmpeeh;
					ArcHandler::CreateArcElement(tmpeeh, nullptr, DEllipse3d::FromCenterNormalRadius(data.ptArcCenter, DVec3d::From(0, 0, 1), diameter/2), true, *ACTIVEMODEL);
					tmpeeh.AddToModel();
					*/

				vecendstr1.Normalize();
				vecendstr2.Normalize();
				vecendstr1.Scale(m_sidecover*uor_per_mm);
				vecendstr2.Scale(m_sidecover*uor_per_mm);
				mdlVec_addPoint(&tmpPt1, &tmpPt1, &vecendstr1);
				mdlVec_addPoint(&tmpPt2, &tmpPt2, &vecendstr2);
				/*EditElementHandle tmpeeh2;
				LineHandler::CreateLineElement(tmpeeh2, nullptr, DSegment3d::From(tmpPt1, tmpPt2), true, *ACTIVEMODEL);
				tmpeeh2.AddToModel();*/

				mdlVec_projectPointToLine(&tmpPt1, NULL, &tmpPt1, &m_vecLinePts[index].at(0), &m_vecLinePts[index].at(1));
				mdlVec_projectPointToLine(&tmpPt2, NULL, &tmpPt2, &m_vecLinePts[index + 1].at(0), &m_vecLinePts[index + 1].at(1));

				m_vecLinePts[index].at(1) = tmpPt1;
				m_vecLinePts[index + 1].at(0) = tmpPt2;
				FrontEnd = tmpPt1;
				/*if (index == 0)
				{
					DVec3d vec = m_vecLinePts[index].at(1) - m_vecLinePts[index].at(0);
					vec.Normalize();
					vec.Scale(m_sidecover*uor_per_mm);
					mdlVec_addPoint(&m_vecLinePts[index].at(0), &m_vecLinePts[index].at(0), &vec);
				}
				if (index + 1 == m_vecLinePts.size() - 1)
				{
					DVec3d vec = m_vecLinePts[index+1].at(0) - m_vecLinePts[index+1].at(1);
					vec.Normalize();
					vec.Scale(m_sidecover*uor_per_mm);
					mdlVec_addPoint(&m_vecLinePts[index+1].at(1), &m_vecLinePts[index+1].at(1), &vec);
				}*/

			}
		}
	}

	FrontStr = m_vecLinePts[index].at(0);
	FrontEnd = m_vecLinePts[index].at(1);

	DPoint3d BackStr;	//ͶӰ��
	mdlVec_projectPointToLine(&BackStr, NULL, &FrontStr, &m_vecLinePts[index].at(2), &m_vecLinePts[index].at(3));
	DPoint3d BackEnd;	//ͶӰ��
	mdlVec_projectPointToLine(&BackEnd, NULL, &FrontEnd, &m_vecLinePts[index].at(2), &m_vecLinePts[index].at(3));
	m_STwallData.height = m_GWallData.height*uor_per_mm / uor_ref;
	m_STwallData.width = FrontStr.Distance(BackStr)*uor_per_mm / uor_ref;
	m_STwallData.length = FrontStr.Distance(FrontEnd)*uor_per_mm / uor_ref;
	m_STwallData.ptStart = FrontStr;
	m_STwallData.ptEnd = FrontEnd;
	FrontStr.Add(FrontEnd);
	FrontStr.Scale(0.5);
	BackStr.Add(BackEnd);
	BackStr.Scale(0.5);
	m_LineNormal = BackStr - FrontStr;
	m_LineNormal.Normalize();


	//if (Negs.size() > 0)//STWALL��б��
	//{
	//	DPoint3d vecBack = pt2[0] - pt2[1];
	//	DPoint3d vecLeft = pt1[0] - pt2[0];
	//	DPoint3d vecRight = pt1[1] - pt2[1];
	//	vecBack.Normalize();
	//	vecLeft.Normalize();
	//	vecRight.Normalize();
	//	m_angle_left = vecBack.AngleTo(vecLeft);
	//	if (m_angle_left > PI / 2)
	//	{
	//		m_angle_left = PI - m_angle_left;
	//	}
	//	m_angle_right = vecBack.AngleTo(vecRight);
	//	if (m_angle_right > PI / 2)
	//	{
	//		m_angle_right = PI - m_angle_right;
	//	}
	//}
	//else
	{
		m_angle_left = PI / 2;
		m_angle_right = PI / 2;
	}
	DPoint3d ptStart = m_STwallData.ptStart;
	DPoint3d ptEnd = m_STwallData.ptEnd;

	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
	CVector3D  xVecNew(ptStart, ptEnd);
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//����ΪX�ᣬˮƽ��ֱ����ΪY��
	SetPlacement(placement);
	if (index == 0)
	{
		PopvecFrontPts().push_back(ptStart);
		PopvecFrontPts().push_back(ptEnd);
	}
	else
	{
		if (isSTGWALL)
		{
			PopvecFrontPts().push_back(ptStart);
			PopvecFrontPts().push_back(ptEnd);
		}
		else
			PopvecFrontPts().push_back(ptEnd);
	}

	return true;
}
bool GWallRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	if (m_vecLinePts.empty())
		return false;


	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	double uor_ref = GetSelectedModel()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	int numtag = 0;

	m_vecAllRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
	g_vecTieRebarPtsNoHole.clear();

	double thickness = 0;
	GetMaxThickness(modelRef, thickness);

	for (int k = 0; k < m_vecLinePts.size(); k++)
	{

		if (m_vecLinePts[k].size() != 4)
		{
			continue;
		}
		double  sidecover = GetSideCover();//ȡ�ò��汣����
		m_sidecover = GetSideCover();

		GetUcsAndStartEndData(k, thickness, modelRef);
		m_useHoleehs.clear();
		CalculateUseHoles(modelRef);

		if (thickness >= MaxWallThickness * uor_per_mm)
		{
			//SetSideCover(0);//�Ƚ����汣��������Ϊ0			
		}
		CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
		double dLevelSpace = 0;
		double dSideCover = m_sidecover * uor_per_mm;
		if (COMPARE_VALUES(dSideCover, m_STwallData.width) >= 0)	//������汣������ڵ���ǽ�ĳ���
		{
			mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"���汣������ڵ���ǽ�ĳ���,�޷������ֽ��", MessageBoxIconType::Information);
			return false;
		}

		vector<CVector3D> vTrans;
		vector<CVector3D> vTransTb;
		//�����������ƫ����
		CalculateTransform(vTrans, vTransTb, modelRef);
		if (vTrans.size() != GetRebarLevelNum())
		{
			return false;
		}

		double dLength = m_STwallData.length;
		double dWidth = m_STwallData.height;

		int iRebarSetTag = 0;
		int iRebarLevelNum = GetRebarLevelNum();
		int iTwinbarSetIdIndex = 0;
		vector<PIT::EndType> vecEndType;
		for (int i = 0; i < iRebarLevelNum; ++i)
		{
			//		int iActualIndex = i;
			RebarSetTag* tag = NULL;
			CMatrix3D   mat, matTb;

			vector<PIT::EndType> vecEndType;
			if (GetvvecEndType().empty())		//û�����ö˲���ʽ������Ĭ��ֵ
			{
				EndType endType;
				memset(&endType, 0, sizeof(endType));
				vecEndType = { endType,endType };
			}
			else
			{
				vecEndType = GetvvecEndType().at(i);
			}

			CVector3D tmpVector(m_LineNormal);
			tmpVector.Scale(vTrans[i].y);
			CMatrix3D   tmpmat;
			tmpmat.SetTranslation(tmpVector);
			double  Misdisstr, Misdisend = 0;
			double tLenth;
			CalculateNowPlacementAndLenth(Misdisstr, Misdisend, tLenth, tmpmat, modelRef);


			if (vTrans.size() != GetRebarLevelNum())
			{
				return false;
			}
			double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;
			m_vecRebarPtsLayer.clear();
			m_vecTwinRebarPtsLayer.clear();
			m_vecTieRebarPtsLayer.clear();
			m_nowvecDir = GetvecDir().at(i);
			if (GetvecDir().at(i) == 1)	//����ֽ�
			{
				bool drawlast = true;
				if (i <= 1 && thickness >= MaxWallThickness * uor_per_mm&&k != m_vecLinePts.size() - 1)//������600�������ǵ�һ�λ������Ҳ������һ��ǽ�������һ��������
				{
					drawlast = false;
				}

				double misDisH_left, misDisH_right;

				if (COMPARE_VALUES_EPS(m_angle_left, PI / 2, 0.001))
				{
					misDisH_left = (1 / sin(m_angle_left) - 1)*diameter + Misdisstr;
				}
				else
				{
					misDisH_left = 0;
				}
				if (COMPARE_VALUES_EPS(m_angle_right, PI / 2, 0.001))
				{
					misDisH_right = (1 / sin(m_angle_right) - 1)*diameter + Misdisend;
				}
				else
				{
					misDisH_right = 0;
				}

				//if (k==0&&thickness>=10)//��һ��
				//{
				//	misDisH_left = misDisH_left + sidecover*uor_per_mm;
				//}
				//else if (k== m_vecLinePts.size() - 1 && thickness >= 10)
				//{
				//	misDisH_right = misDisH_right + sidecover * uor_per_mm;
				//}


				vector<CVector3D> vecEndNormal(2);
				CVector3D	endNormal;	//�˲��乳����
				if (GetvvecEndType().size() > 0)
				{
					for (size_t k = 0; k < vecEndNormal.size(); ++k)
					{
						double dRotateAngle = vecEndType.at(k).rotateAngle;
						endNormal = m_STwallData.ptEnd - m_STwallData.ptStart;
						endNormal.Normalize();
						if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
						{
							endNormal.Negate();
						}
						CVector3D rebarVec = CVector3D::kZaxis;
						/*					endNormal = rebarVec.CrossProduct(vec);*/
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
						vecEndNormal[k] = endNormal;
					}
				}
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;
				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
				//���Ʋ���--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
				{
					m_isPushTieRebar = false;
					//�Ȼ��Ʒǲ����
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef, drawlast);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);
					}

					m_isPushTieRebar = true;
					//���Ʋ����
					tag = MakeRebars(PopvecSetId().at(iTwinbarSetIdIndex + iRebarLevelNum), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true, modelRef, drawlast);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1 + numtag);
						rsetTags.Add(tag);
					}
					iTwinbarSetIdIndex++;
				}
				else //��ǰ��δ���ò���
				{
					m_isPushTieRebar = true;
					TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat, twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef, drawlast);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);
					}
				}
				vecEndType.clear();
			}
			else
			{
				double misDisV_left, misDisV_right;
				if (COMPARE_VALUES_EPS(m_angle_left, PI / 2, 0.001))
				{
					misDisV_left = diameter / tan(m_angle_left);
				}
				else
				{
					misDisV_left = 0;
				}
				if (COMPARE_VALUES_EPS(m_angle_right, PI / 2, 0.001))
				{
					misDisV_right = diameter / tan(m_angle_right);
				}
				else
				{
					misDisV_right = 0;
				}
				double leftSideCov = m_sidecover * uor_per_mm / sin(m_angle_left);
				double rightSideCov = m_sidecover * uor_per_mm / sin(m_angle_right);

				//if (k == 0 && thickness >= 10)//��һ��
				//{
				//	leftSideCov = leftSideCov + sidecover * uor_per_mm;
				//}
				//else if (k == m_vecLinePts.size() - 1 && thickness >= 10)
				//{
				//	rightSideCov = rightSideCov + sidecover * uor_per_mm;
				//}

				double allSideCov = leftSideCov + rightSideCov;

				tLenth = tLenth - (misDisV_left + misDisV_right);
				vTrans[i].x = (tLenth - allSideCov) / 2 + Misdisstr + leftSideCov;
				vector<CVector3D> vecEndNormal(2);
				CVector3D	endNormal;	//�˲��乳����
				if (GetvvecEndType().size() > 0)
				{
					for (size_t k = 0; k < vecEndNormal.size(); ++k)
					{
						double dRotateAngle = vecEndType.at(k).rotateAngle;
						CVector3D rebarVec = m_STwallData.ptEnd - m_STwallData.ptStart;
						endNormal = CVector3D::From(0, 0, -1);
						if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
						{
							endNormal.Negate();
						}
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
						vecEndNormal[k] = endNormal;
					}
				}
				mat = rot90;
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;
				matTb = rot90;
				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
				//������Ϊ�����,ż����Ϊ��ͨ��

				//���Ʋ���--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
				{
					m_isPushTieRebar = true;
					//�Ȼ��Ʒǲ����
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);
					}

					m_isPushTieRebar = false;
					//���Ʋ����
					tag = MakeRebars(PopvecSetId().at(iTwinbarSetIdIndex + iRebarLevelNum), GetvecDirSize().at(i), dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true, modelRef);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1 + numtag);
						rsetTags.Add(tag);
					}
					iTwinbarSetIdIndex++;

				}
				else //��ǰ��δ���ò���
				{
					m_isPushTieRebar = true;
					TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);
					}
				}
				//end
				vecEndType.clear();
			}
			if (m_vecRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = k;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecRebarPtsLayer.at(m);
					rbPt.ptend = m_vecRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
			if (m_vecTwinRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecTwinRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = k;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTwinRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTwinRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTwinRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
			if (m_vecTieRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecTieRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTieRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTieRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTieRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
			SetSideCover(sidecover);
		}

		////�������--begin
		//if (GetTieRebarInfo().tieRebarMethod && (m_vecAllRebarStartEnd.size() >= 4))
		//{
		//	PopvecSetId().push_back(0);
		//	FaceRebarDataArray faceDataArray;
		//	faceDataArray.posRebarData.HRebarData.rebarSize = GetvecDirSize().at(0);
		//	faceDataArray.posRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(0);
		//	faceDataArray.posRebarData.VRebarData.rebarSize = GetvecDirSize().at(1);
		//	faceDataArray.posRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(1);

		//	faceDataArray.revRebarData.HRebarData.rebarSize = GetvecDirSize().at(GetvecDirSize().size() - 1);
		//	faceDataArray.revRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecDirSize().size() - 1);
		//	faceDataArray.revRebarData.VRebarData.rebarSize = GetvecDirSize().at(GetvecDirSize().size() - 2);
		//	faceDataArray.revRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecDirSize().size() - 2);

		//	vector<vector<pair<CPoint3D, CPoint3D> > > vecStartEnd;		//ֻ�洢1��2��͵�����1��2��
		//	vecStartEnd.push_back(m_vecAllRebarStartEnd[0]);
		//	vecStartEnd.push_back(m_vecAllRebarStartEnd[1]);
		//	vecStartEnd.push_back(m_vecAllRebarStartEnd[m_vecAllRebarStartEnd.size() - 1]);
		//	vecStartEnd.push_back(m_vecAllRebarStartEnd[m_vecAllRebarStartEnd.size() - 2]);
		//	BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
		//	int	tieRebarStyle = GetTieRebarInfo().tieRebarStyle;
		//	if (strTieRebarSize.Find(L"mm") != string::npos)
		//		strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//ɾ��mm
		//	TieRebarMaker tieRebarMaker(faceDataArray, m_vecAllRebarStartEnd, (TieRebarStyle)tieRebarStyle, strTieRebarSize);
		//	tieRebarMaker.m_CallerId = GetCallerId();
		//	tieRebarMaker.SetCustomStyle(GetTieRebarInfo().rowInterval, GetTieRebarInfo().colInterval);
		//	RebarSetTag* tag = tieRebarMaker.MakeRebar(PopvecSetId().at(GetvecSetId().size() - 1), modelRef);
		//	if (NULL != tag)
		//	{
		//		tag->SetBarSetTag(iRebarLevelNum + 1 + numtag);
		//		rsetTags.Add(tag);
		//	}
		//}

		numtag = (int)rsetTags.GetSize();
	}

	if (PreviewButtonDown)//Ԥ����ť���£���������
	{
		m_allLines.clear();
		for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)
		{
			vector<vector<DPoint3d>> faceLinePts = *it;
			for (auto it : faceLinePts)
			{
				vector<DPoint3d> linePts = it;
				EditElementHandle eeh;
				LineStringHandler::CreateLineStringElement(eeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allLines.push_back(eeh.GetElementRef());
			}
		}
		return true;
	}



	//end
	/*AddRebarSets(rsetTags);
	RebarSets rebar_sets;
	GetRebarSets(rebar_sets, ACTIVEMODEL);
	return true;*/
	//return true;
	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

long GWallRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}

void GWallRebarAssembly::CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return;
	DPoint3d ptstar = m_STwallData.ptStart;
	DPoint3d ptend = m_STwallData.ptEnd;

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstar, ptend), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);

	DPoint3d pt1[2];
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//��ʱʹ�õ�ǰ����MODEL�������������޸�

	if (!GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		/*EditElementHandle eeh1;
		LineHandler::CreateLineElement(eeh1, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
		eeh1.AddToModel();*/
		DPoint3d ptProject1;	//ͶӰ��
		mdlVec_projectPointToLine(&ptProject1, NULL, &pt1[0], &m_STwallData.ptStart, &m_STwallData.ptEnd);

		DPoint3d ptProject2;	//ͶӰ��
		mdlVec_projectPointToLine(&ptProject2, NULL, &pt1[1], &m_STwallData.ptStart, &m_STwallData.ptEnd);

		lenth = pt1[0].Distance(pt1[1]);
		misDisstr = ptProject1.Distance(ptstar);

		misDisend = ptProject2.Distance(ptend);

	}



}


RebarSetTag* GWallRebarAssembly::MakeRebars
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	double              xLen,
	double              height,
	double              spacing,
	double              startOffset,
	double              endOffset,
	vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
	vector<CVector3D> const& vecEndNormal,
	CMatrix3D const&    mat,
	TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
	int level,
	int grade,
	int DataExchange,
	bool				bTwinbarLevel,
	DgnModelRefP        modelRef,
	bool  drawlast
)
{
	rebarSetId = 0;
	//	m_IsTwinrebar = bTwinbarLevel;
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	if (endType.size() != vecEndNormal.size() || endType.size() == 0)
	{
		return NULL;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, startbendLenTb, endbendLen, endbendLenTb;
	double begStraightAnchorLen, endStraightAnchorLen;
	switch (endType[0].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeStart.SetType(RebarEndType::kLap);
		begStraightAnchorLen = endType[0].endPtInfo.value1;	//ê�볤��
		break;
	case 4:	//90���乳
	{
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		}
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}
	break;
	case 5:	//135���乳
	{
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		}
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}
	break;
	case 6:	//180���乳
	{
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		}
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}
	break;
	case 8:	//�û�
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (endType[1].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeEnd.SetType(RebarEndType::kLap);
		endStraightAnchorLen = endType[1].endPtInfo.value1;	//ê�볤��
		break;
	case 4:	//90���乳
	{
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = endType[1].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		}
		endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}
	break;
	case 5:	//135���乳
	{
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = endType[1].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		}
		endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}
	break;
	case 6:	//180���乳
	{
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = endType[1].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		}
		endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}

	break;
	case 8:	//�û�
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}
	double updownSideCover = 50 * uor_per_mm;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);	//������30
	double adjustedXLen, adjustedSpacing;

	double leftSideCov = GetSideCover()*uor_per_mm / sin(m_angle_left);
	double rightSideCov = GetSideCover() *uor_per_mm / sin(m_angle_right);
	double allSideCov = leftSideCov + rightSideCov;
	double allUDSideCov = updownSideCover * 2;//�������ҵĺ������̶�Ϊ50���ֽ������ߵ�ǽ�ߵľ���
	int numRebar = 0;
	if (twinBarInfo.hasTwinbars)	//����
		adjustedXLen = xLen - allUDSideCov /*-diameter- diameterTb */ - startOffset - endOffset;
	else
		adjustedXLen = xLen - allUDSideCov - startOffset - endOffset;
	if (bTwinbarLevel)				//�����ֽ�����
	{
		numRebar = (int)floor(adjustedXLen / (spacing * (twinBarInfo.interval + 1)) + 0.85) + 1;
		int numRebar1 = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		//if (numRebar1 > 1)
		//{
		//	adjustedSpacing = adjustedXLen / (numRebar1 - 1);	//�ǲ����ƽ�����
		//	adjustedSpacing *= (twinBarInfo.interval + 1);		//�����ʵ�ʼ������Ըֽ���
		//}
	}
	else
	{
		numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		/*	if (numRebar > 1)
				adjustedSpacing = adjustedXLen / (numRebar - 1);*/
	}

	double xPos = startOffset;
	if (m_nowvecDir == 0)
		xPos = -xPos;
	xPos = xPos - diameter / 2;
	if (bTwinbarLevel)				//�������ƫ��һ�ξ���
	{
		xPos += diameter * 0.5;
		if (diameterTb > diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radiusTb - radius)*(radiusTb - radius), 0.5) - radius;
			xPos += dOffset;
		}
		else if (diameterTb < diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radius - radiusTb)*(radius - radiusTb), 0.5) - radius;
			xPos += dOffset;
		}
		else
		{
			xPos += diameter * 0.5;
		}
	}
	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		start.SetbendLen(startbendLenTb);
		start.SetbendRadius(startbendRadius);
	}
	else
	{
		start.SetbendLen(startbendLen);
		start.SetbendRadius(startbendRadius);
	}
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		end.SetbendLen(endbendLenTb);
		end.SetbendRadius(bendRadiusTb);
	}
	else
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };

	for (int i = 0; i < numRebar; i++)
	{
		if (!drawlast/*&&i == numRebar - 1*/)
		{
			continue;
		}
		vector<PITRebarCurve>     rebarCurves;
		if (i == numRebar - 1)//��������һ����Ҫ�жϵ�ǰ���ж��پ���,���Ͼ���Ҫ���Ҫ�ٲ���һ��
		{
			double sDis = adjustedXLen - (numRebar - 2)*adjustedSpacing;
			if (sDis > 30 * uor_per_mm)
			{
				if (m_nowvecDir == 0)
				{
					xPos -= sDis;
					if (bTwinbarLevel)
					{
						xPos += (diameter + diameterTb);
					}
				}
				else
				{
					xPos += sDis;
					if (bTwinbarLevel)
					{
						xPos -= (diameter + diameterTb);
					}
				}
			}
			else
			{
				continue;
			}
		}
		else if (i != 0)//�������һ����Ҳ���ǵ�һ�������ϼ��
		{
			if (m_nowvecDir == 0)
			{
				xPos -= adjustedSpacing;
			}
			else
			{
				xPos += adjustedSpacing;
			}
		}
		makeRebarCurve(rebarCurves, xPos, height - allSideCov, endTypeStartOffset, endTypEendOffset, endTypes, mat, bTwinbarLevel);
		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinBarInfo.rebarSize, symb);
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}
	else
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	vector<vector<DPoint3d>> vecStartEnd;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (ISPointInHoles(m_Holeehs, midPos))
		{
			if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
			{
				continue;
			}
		}

		vector<DPoint3d> linePts;
		RebarVertices vertices = rebarCurve.GetVertices();
		for (size_t i = 0; i < (size_t)vertices.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &vertices.At(i);
			linePts.push_back(tmpVertex->GetIP());
		}
		/*	EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			eeh.AddToModel();*/

		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinBarInfo.rebarSize));
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameterTb, endTypes, shape, modelRef, false);
			}
			else
			{
				shape.SetSizeKey((LPCTSTR)sizeKey);
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
			}
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (DataExchange == 0)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinback";
				else
					Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
		j++;
		vecStartEnd.push_back(linePts);
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

bool GWallRebarAssembly::makeRebarCurve
(
	vector<PITRebarCurve>&     rebars,
	double                  xPos,
	double                  yLen,
	double					startOffset,
	double					endOffset,
	PITRebarEndTypes&		endTypes,
	CMatrix3D const&        mat,
	bool isTwin
)
{

	CPoint3D  startPt;
	CPoint3D  endPt;

	//������Ϊ��ֵ
// 	if (startPt < 0)
// 		startPt = 0;
// 	if (endOffset < 0)
// 		endOffset = 0;

	startPt = CPoint3D::From(xPos, 0.0, -yLen / 2.0 + startOffset);
	endPt = CPoint3D::From(xPos, 0.0, yLen / 2.0 - endOffset);

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	//eeh.AddToModel();



	DPoint3d pt1[2];
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//��ʱʹ�õ�ǰ����MODEL�������������޸�

	//ȷ������յ��Ǵ�С����---begin
	DVec3d vec = pt1[1] - pt1[0];
	DVec3d vecX = DVec3d::From(1, 0, 0);
	vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
	vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
	vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
	vec.Normalize();
	if (vec.IsPerpendicularTo(vecX))
	{
		if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
	}
	else
	{
		if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
	}
	//---end




	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}
	if (isTwin)
	{
		m_vecRebarPtsLayer.push_back(pt1[0]);
		m_vecRebarPtsLayer.push_back(pt1[1]);
	}

	if (m_isPushTieRebar)
	{
		m_vecTieRebarPtsLayer.push_back(pt1[0]);
		m_vecTieRebarPtsLayer.push_back(pt1[1]);
	}
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = m_sidecover * uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);

	//EditElementHandle eehline;
	//LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
	//eehline.AddToModel();

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt : tmppts)
	{
		if (ExtractFacesTool::IsPointInLine(pt, pt1[0], pt1[1], ACTIVEMODEL, isStr))
		{
			int dis = (int)pt1[0].Distance(pt);
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = pt1[0];
	}
	else
	{
		map_pts[0] = pt1[0];
	}
	int dis = (int)pt1[0].Distance(pt1[1]);
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = pt1[1];
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = pt1[1];
	}



	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{
		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itr->second);
		vex->SetType(RebarVertex::kStart);
		endTypes.beg.SetptOrgin(itr->second);

		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}

		endTypes.end.SetptOrgin(itrplus->second);

		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itrplus->second);
		vex->SetType(RebarVertex::kEnd);

		rebar.EvaluateEndTypes(endTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebars.push_back(rebar);
	}


	//rebar.DoMatrix(mat);
	return true;
}

void GWallRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetSideCover()*uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

	if (g_wallRebarInfo.concrete.isHandleHole)//������Ҫ����Ŀ׶�
	{
		for (int j = 0; j < m_Holeehs.size(); j++)
		{
			EditElementHandle eeh;
			eeh.Duplicate(*m_Holeehs.at(j));
			bool isdoorNeg = false;//�ж��Ƿ�Ϊ�Ŷ�NEG
			isdoorNeg = IsDoorHoleNeg(m_Holeehs.at(j), m_doorsholes);
			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
			}
			if (isdoorNeg)
			{
				PlusSideCover(eeh, dSideCover, matrix, isdoorNeg, m_STwallData.width);
			}
			DPoint3d ptcenter = getCenterOfElmdescr(eeh.GetElementDescrP());
			vector<DPoint3d> interpts;
			DPoint3d tmpStr, tmpEnd;
			tmpStr = m_STwallData.ptStart;
			tmpEnd = m_STwallData.ptEnd;
			tmpStr.z = tmpEnd.z = ptcenter.z;
			GetIntersectPointsWithHole(interpts, &eeh, tmpStr, tmpEnd);
			if (interpts.size() > 0)
			{
				TransformInfo transinfo(trans);
				eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
				DPoint3d minP;
				DPoint3d maxP;
				//����ָ��Ԫ����������Ԫ�صķ�Χ��
				mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
				DRange3d range;
				range.low = minP;
				range.high = maxP;
				bool isNeed = false;
				if (range.XLength() > misssize || range.ZLength() > misssize)
				{
					isNeed = true;
				}

				if (isNeed)
				{
					if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//������Ŷ�
					{
						continue;
					}
					ElementCopyContext copier(ACTIVEMODEL);
					copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
					copier.SetTransformToDestination(true);
					copier.SetWriteElements(false);
					copier.DoCopy(*m_Holeehs.at(j));
					if (isdoorNeg)
					{
						PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix, isdoorNeg, m_STwallData.width);
					}
					else
					{
						PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
					}

					m_useHoleehs.push_back(m_Holeehs.at(j));
				}
			}

		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}



}
void ArcWallRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
{
	m_useHoleehs.clear();
	//m_useHoleehs.insert(m_useHoleehs.begin(), m_Holeehs.begin(), m_Holeehs.end());


	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetSideCover()*uor_per_mm;



	if (g_wallRebarInfo.concrete.isHandleHole)//������Ҫ����Ŀ׶�
	{
		for (int j = 0; j < m_Holeehs.size(); j++)
		{
			EditElementHandle eeh;
			eeh.Duplicate(*m_Holeehs.at(j));

			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
			}
			DPoint3d ptcenter = m_ArcWallData.InnerArc.ptCenter;
			DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());

			ptcenter.z = ptele.z;
			CVector3D yVec = ptcenter - ptele;
			yVec.Normalize();

			CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

			DPoint3d ptStart = ptcenter;
			BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//����ΪX�ᣬˮƽ��ֱ����ΪY��

			Transform trans;
			placement.AssignTo(trans);
			Transform intrans = trans;
			intrans.InverseOf(intrans);

			TransformInfo transinfo(trans);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			DPoint3d minP;
			DPoint3d maxP;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
			DRange3d range;
			range.low = minP;
			range.high = maxP;
			bool isNeed = false;
			if (range.XLength() > misssize || range.ZLength() > misssize)
			{
				isNeed = true;
			}

			/*if (isNeed)
			{
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*m_Holeehs.at(j));
				PlusSideCover(*m_Holeehs.at(j), dSideCover, trans);
				m_useHoleehs.push_back(m_Holeehs.at(j));
			}*/
			if (isNeed)
			{
				if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//������Ŷ�
				{
					continue;
				}
				bool isdoorNeg = false;//�ж��Ƿ�Ϊ�Ŷ�NEG
				isdoorNeg = IsDoorHoleNeg(m_Holeehs.at(j), m_doorsholes);
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*m_Holeehs.at(j));
				if (isdoorNeg)
				{
					PlusSideCover(*m_Holeehs.at(j), dSideCover, trans, isdoorNeg, m_ArcWallData.thickness);
				}
				else
				{
					PlusSideCover(*m_Holeehs.at(j), dSideCover, trans);
				}

				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}



}
long ArcWallRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}

bool ArcWallRebarAssembly::CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
{
	bool ret = false;

	BeArcSeg arc(begPt, midPt, endPt);

	CMatrix3D mat;
	CPoint3D cen;
	arc.GetCenter(cen);

	if (arc.GetPlanarMatrix(mat) && arc.GetCenter(cen))
	{
		CPoint3D beg = begPt;
		CPoint3D med = midPt;
		CPoint3D end = endPt;

		beg.Transform(mat);
		med.Transform(mat);
		end.Transform(mat);
		cen.Transform(mat);
		arc.Transform(mat);

		CVector3D tan1 = arc.GetTangentVector(beg);
		CVector3D tan2 = arc.GetTangentVector(end);

		CPointVect pv1(beg, tan1);
		CPointVect pv2(end, tan2);

		CPoint3D ip;
		bool isIntersect = pv1.Intersect(ip, pv2);

		double radius = arc.GetRadius();

		RebarVertexP vex;
		vex = &(curve.PopVertices()).NewElement();
		vex->SetIP(beg);
		vex->SetType(RebarVertex::kStart);      // first IP

		CPoint3D mid = (beg + end) / 2.0;
		CVector3D midVec(cen, mid);
		midVec.Normalize();

		if (isIntersect)
		{
			mid = cen + midVec * radius;

			// it can be on the other size
			CPoint3D mid1 = cen - midVec * radius;

			double d1 = med.Distance(mid1);
			double d2 = med.Distance(mid);

			if (d1 < d2)
			{
				mid = mid1;
				midVec = -midVec;
				// this is big arc we need 4 ips

				CVector3D midTan = midVec.Perpendicular();
				CPointVect pvm(mid, midTan);

				pv1.Intersect(ip, pvm);
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(ip);
				vex->SetType(RebarVertex::kIP);      // 2nd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				mid1 = (beg + mid) / 2.0;
				midVec = mid1 - cen;
				midVec.Normalize();
				mid1 = cen + midVec * radius;

				vex->SetArcPt(0, beg);
				vex->SetArcPt(1, mid1);
				vex->SetArcPt(2, mid);



				pv1.Intersect(ip, pvm);
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(beg);
				vex->SetType(RebarVertex::kIP);      // 3rd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				mid1 = (end + mid) / 2.0;
				midVec = mid1 - cen;
				midVec.Normalize();
				mid1 = cen + midVec * radius;

				vex->SetArcPt(0, mid);
				vex->SetArcPt(1, mid1);
				vex->SetArcPt(2, end);

			}
			else
			{
				// this is less than 90 or equal we need 3 ips
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(ip);
				vex->SetType(RebarVertex::kIP);      // 2nd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				vex->SetArcPt(0, beg);
				vex->SetArcPt(1, mid);
				vex->SetArcPt(2, end);
			}
		}
		else
		{
			// this is half circle - we need 4 ips
			midVec = arc.GetTangentVector(med);
			midVec.Normalize();
			DPoint3d ptMedTan = midVec;
			ptMedTan.Scale(radius);
			ptMedTan.Add(med);
			DPoint3d ptBegTan = tan1;
			ptBegTan.Scale(radius);
			ptBegTan.Add(beg);
			mdlVec_intersect(ip, &DSegment3d::From(beg, ptBegTan), &DSegment3d::From(med, ptMedTan));
			// 			EditElementHandle eeh;
			// 			LineHandler::CreateLineElement(eeh, NULL, DSegment3d::From(beg, ptBegTan), true, *ACTIVEMODEL);
			// 			eeh.AddToModel();
			// 			EditElementHandle eeh1;
			// 			LineHandler::CreateLineElement(eeh1, NULL, DSegment3d::From(med, ptMedTan), true, *ACTIVEMODEL);
			// 			eeh1.AddToModel();
			// 			CPointVect pvm(med, midVec);
			// 			pvm.Intersect(ip, tan1);
			// 			tan1 = ip - beg;
			// 			tan1.Normalize();
			// 
			// 			ip = beg + tan1 * radius;
			// 			midVec = ip - cen;
			// 			midVec.Normalize();
			//			midVec = ip - cen;
			mid = cen + tan1 * radius;
			DEllipse3d circle = DEllipse3d::FromCenterRadiusXY(cen, radius);

			double angle_start = circle.PointToAngle(beg);
			double angle_mid = circle.PointToAngle(mid);

			double angle = (angle_start + angle_mid) / 2;
			// 			if (angle < angle_mid)
			// 				angle += _PI;
			CPoint3D mid1;
			circle.Evaluate(&mid1, 0, angle);

			//			CPoint3D mid1 = cen + midVec;

			vex = &curve.PopVertices().NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 2nd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, beg);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, mid);

			// 			ip = end + tan1 * radius; // tan1 and tan2 parallel but tan1 has now the correct direction, do not change to tan2
			// 			midVec = ip - cen;
			// 			midVec.Normalize();
			// 			mid1 = cen + midVec * radius;

			//			midVec.Negate();
			DPoint3d ptEndTan = tan2;
			ptEndTan.Scale(radius);
			ptEndTan.Add(end);
			mdlVec_intersect(ip, &DSegment3d::From(end, ptEndTan), &DSegment3d::From(med, ptMedTan));
			// 			midVec = ip - cen;
			// 			mid1 = cen + midVec;

			// 			EditElementHandle eeh2;
			// 			LineHandler::CreateLineElement(eeh2, NULL, DSegment3d::From(end, ptEndTan), true, *ACTIVEMODEL);
			// 			eeh2.AddToModel();
			double angle_end = circle.PointToAngle(end);

			angle = (angle_end + angle_mid) / 2;
			// 			if (angle < angle_end)
			// 				angle += _PI;

			circle.Evaluate(&mid1, 0, angle);

			vex = &curve.PopVertices().NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 3rd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, mid);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, end);
		}

		vex = &curve.PopVertices().NewElement();
		vex->SetIP(end);
		vex->SetType(RebarVertex::kEnd);      // last IP

		mat = mat.Inverse();
		curve.DoMatrix(mat);              // transform back

		ret = true;
	}

	return ret;
}

bool ArcWallRebarAssembly::makeLineRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::ArcSegment arcSeg, double dLen, double space, double startOffset, double endOffset, PIT::PITRebarEndTypes & endTypes)
{
	DEllipse3d seg = DEllipse3d::FromPointsOnArc(arcSeg.ptStart, arcSeg.ptMid, arcSeg.ptEnd);

	ICurvePrimitivePtr curve = ICurvePrimitive::CreateArc(seg);

	CurveLocationDetail arcDetail;
	curve->PointAtSignedDistanceFromFraction(0, space, false, arcDetail);
	DPoint3d ptStart = arcDetail.point;
	DPoint3d vec = { 0,0,1 };
	vec.Scale(startOffset);
	ptStart.Add(vec);

	DPoint3d ptEnd = arcDetail.point;
	ptEnd.z += dLen;
	vec.Normalize();
	vec.Negate();
	vec.Scale(endOffset);
	ptEnd.Add(vec);

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetSideCover()*uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	m_vecRebarPtsLayer.push_back(ptStart);
	m_vecRebarPtsLayer.push_back(ptEnd);
	if (m_isPushTieRebar)
	{
		m_vecTieRebarPtsLayer.push_back(ptStart);
		m_vecTieRebarPtsLayer.push_back(ptEnd);
	}
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, ptStart, ptEnd, dSideCover, matrix);

	/*EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(ptStart, ptEnd), true, *ACTIVEMODEL);
	eehline.AddToModel();*/

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt : tmppts)
	{
		if (ExtractFacesTool::IsPointInLine(pt, ptStart, ptEnd, ACTIVEMODEL, isStr))
		{
			int dis = (int)ptStart.Distance(pt);
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = ptStart;
	}
	else
	{
		map_pts[0] = ptStart;
	}
	int dis = (int)ptStart.Distance(ptEnd);
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = ptEnd;
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = ptEnd;
	}



	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{
		PITRebarCurve tmprebar;
		RebarVertexP vex;
		vex = &tmprebar.PopVertices().NewElement();
		vex->SetIP(itr->second);
		vex->SetType(RebarVertex::kStart);
		endTypes.beg.SetptOrgin(itr->second);

		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}

		endTypes.end.SetptOrgin(itrplus->second);

		vex = &tmprebar.PopVertices().NewElement();
		vex->SetIP(itrplus->second);
		vex->SetType(RebarVertex::kEnd);

		tmprebar.EvaluateEndTypes(endTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebar.push_back(tmprebar);
	}

	/*RebarVertexP vex;
	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptStart);
	vex->SetType(RebarVertex::kStart);
	endTypes.beg.SetptOrgin(ptStart);

	DPoint3d ptEnd = arcDetail.point;
	ptEnd.z += dLen;
	vec.Normalize();
	vec.Negate();
	vec.Scale(endOffset);
	ptEnd.Add(vec);
	endTypes.end.SetptOrgin(ptEnd);

	vex = &rebar.PopVertices().NewElement();
	vex->SetIP(ptEnd);
	vex->SetType(RebarVertex::kEnd);

	rebar.EvaluateEndTypes(endTypes);*/

	return false;
}

bool ArcWallRebarAssembly::makeArcWallRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::ArcSegment arcSeg, double space, double startOffset, double endOffset, PIT::PITRebarEndTypes& endTypes)
{

	DPoint3d ptArcStart = arcSeg.ptStart;
	ptArcStart.z += space;
	DPoint3d ptArcEnd = arcSeg.ptEnd;
	ptArcEnd.z += space;
	DPoint3d ptArcCenter = arcSeg.ptCenter;
	ptArcCenter.z += space;
	DPoint3d ptArcMid = arcSeg.ptMid;
	ptArcMid.z += space;

	m_vecRebarPtsLayer.push_back(ptArcStart);
	m_vecRebarPtsLayer.push_back(ptArcMid);
	m_vecRebarPtsLayer.push_back(ptArcEnd);
	if (m_isPushTieRebar)
	{
		m_vecTieRebarPtsLayer.push_back(ptArcStart);
		m_vecTieRebarPtsLayer.push_back(ptArcMid);
		m_vecTieRebarPtsLayer.push_back(ptArcEnd);
	}

	EditElementHandle arceeh;
	ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(ptArcStart, ptArcMid, ptArcEnd), true, *ACTIVEMODEL);
	//arceeh.AddToModel();

	vector<DPoint3d> pts;
	GetARCIntersectPointsWithHoles(pts, m_Holeehs, ptArcStart, ptArcEnd, ptArcMid);


	if (pts.size() > 0)
	{
		/*for (int i =0;i<pts.size()-1;i++)
		{
		EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(pts[i], pts[i+1]), true, *ACTIVEMODEL);
		eeh.AddToModel();
		}*/

	}
	map<int, DPoint3d> map_pts;
	bool isStr = false;
	double dislenth;
	dislenth = 0;
	mdlElmdscr_distanceAtPoint(&dislenth, nullptr, nullptr, arceeh.GetElementDescrP(), &ptArcEnd, 0.1);
	for (DPoint3d pt : pts)
	{
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &pt, 0.1);
		if (dis1 > 10 && dis1 <= dislenth + 10)
		{
			int dis = (int)dis1;
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = ptArcStart;
	}
	else
	{
		map_pts[0] = ptArcStart;
	}
	int dis = (int)dislenth;
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = ptArcEnd;
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = ptArcEnd;
	}



	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{

		PITRebarCurve trebar;
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &itr->second, 0.1);
		DPoint3d tmpMid, tmpstr, tmpend;

		tmpstr = itr->second;
		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		tmpend = itrplus->second;
		double dis2;
		dis2 = 0;
		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, arceeh.GetElementDescrP(), &tmpend, 0.1);

		dis1 = dis1 + abs(dis2 - dis1) / 2;

		mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis1, arceeh.GetElementDescrP(), 0.1);

		if (ISPointInHoles(m_useHoleehs, tmpMid))
		{
			if (ISPointInHoles(m_useHoleehs, tmpstr) && ISPointInHoles(m_useHoleehs, tmpend))
			{
				itr--;
				continue;
			}
		}
		if (CalculateArc(trebar, tmpstr, tmpMid, tmpend))
		{
			trebar.EvaluateEndTypesArc(endTypes);
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(tmpstr, tmpMid, tmpend), true, *ACTIVEMODEL);
			//arceeh1.AddToModel();
			rebar.push_back(trebar);
		}
	}


	return true;
}


bool ArcWallRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pArcWallDoubleRebarDlg = new CWallRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pArcWallDoubleRebarDlg->SetSelectElement(ehSel);
	pArcWallDoubleRebarDlg->Create(IDD_DIALOG_WallRebar);
	pArcWallDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pArcWallDoubleRebarDlg->ShowWindow(SW_SHOW);



	// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// 	CWallRebarDlg dlg(ehSel, CWnd::FromHandle(MSWIND));
	// 	dlg.SetConcreteId(FetchConcrete());
	// 	if (IDCANCEL == dlg.DoModal())
	// 		return false;

	return true;
}

bool ArcWallRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();

	SetWallData(ehWall);
	MakeRebars(modelRef);
	Save(modelRef);
	return true;
}



bool ArcWallRebarAssembly::AnalyzingWallGeometricData(ElementHandleCR eh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle testeeh(eh, false);

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);

	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &m_ArcWallData.height);

	double frontRadius, backRadius;
	frontRadius = backRadius = 0;
	int j = 0;
	for (int i = 0; i < vecDownFaceLine.size(); i++)
	{
		if (vecDownFaceLine[i] != nullptr)
		{
			if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
			{
				double starR, sweepR;
				double radius;
				DPoint3d ArcDPs[2];
				RotMatrix rotM;
				DPoint3d centerpt;
				mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &vecDownFaceLine[i]->el);
				if (j == 0)
				{
					frontRadius = backRadius = radius;
					j++;
				}
				else
				{
					if (frontRadius > radius)
					{
						frontRadius = radius;
					}
					if (backRadius < radius)
					{
						backRadius = radius;
					}
				}
			}
		}
	}
	double starangleF, endangleF;
	double starangleB, endangleB;
	DPoint3d centerpt;
	CalcuteStrEndAngleAndRadius(starangleF, endangleF, frontRadius, centerpt, vecDownFaceLine);
	CalcuteStrEndAngleAndRadius(starangleB, endangleB, backRadius, centerpt, vecDownFaceLine);

	bool scalef = false;
	if (starangleF > endangleF)
	{
		double tmpangle;
		tmpangle = endangleF;
		endangleF = starangleF;
		starangleF = tmpangle;
		scalef = true;
	}
	if (starangleB > endangleB)
	{
		double tmpangle;
		tmpangle = endangleB;
		endangleB = starangleB;
		starangleB = tmpangle;
	}

	double strAngel = starangleF, endAngel = endangleF;
	if (strAngel > starangleB)
	{
		strAngel = starangleB;
	}
	if (endAngel < endangleB)
	{
		endangleB = endangleB;
	}
	//if (starangleF < starangleB)
	//{
	//	starangleB = starangleF;
	//}
	//else
	//{
	//	starangleF = starangleB;
	//}
	//if (endangleF > endangleB)
	//{
	//	endangleB = endangleF;
	//}
	//else
	//{
	//	endangleF = endangleB;
	//}
	PIT::ArcSegment arcFront, arcBack;
	{//�����⻡�ı�֮ǰ�������յ�
		double sweepAngle = endangleB - starangleB;
		double sweepR;
		if (scalef)
		{
			sweepAngle = 360 - sweepAngle;
			sweepR = -(sweepAngle) / 180 * fc_pi;
		}
		else
		{
			sweepR = (sweepAngle) / 180 * fc_pi;
		}
		//��ʼ�Ƕ���ɨ��Ƕȷ������ƶ���ɨ��ǶȼӴ�
		double starR = (360 - starangleB) / 180 * fc_pi;


		MSElement  newarcOut;
		mdlArc_create(&newarcOut, NULL, &centerpt, backRadius, backRadius, NULL, starR, -sweepR);
		DPoint3d ArcDPs[2];
		RotMatrix rotM;
		double radius;
		mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcOut);
		arcBack.FptStart = ArcDPs[0];
		arcBack.FptEnd = ArcDPs[1];
	}



	double sweepAngle = endangleF - starangleF;
	//if (sweepAngle < 180)
	//{
	//	if (starangleF < starangleB)//�⻡ȡɨ�ӽǶ����ʱ�Ļ������ڻ��Ƚ�ʱ
	//	{
	//		starangleB = starangleF;
	//	}
	//	if (endangleF > endangleB)
	//	{
	//		endangleB = endangleF;
	//	}
	//}
	//else
	//{
	//	if (starangleF > starangleB)//�⻡ȡɨ�ӽǶ����ʱ�Ļ������ڻ��Ƚ�ʱ
	//	{
	//		starangleB = starangleF;
	//	}
	//	if (endangleF < endangleB)
	//	{
	//		endangleB = endangleF;
	//	}
	//}
	double sweepR;
	if (scalef)
	{
		sweepAngle = 360 - sweepAngle;
		sweepR = -(sweepAngle) / 180 * fc_pi;
	}
	else
	{
		sweepR = (sweepAngle) / 180 * fc_pi;
	}
	//��ʼ�Ƕ���ɨ��Ƕȷ������ƶ���ɨ��ǶȼӴ�
	double starR = (360 - starangleF) / 180 * fc_pi;



	MSElement newarcIn, newarcOut;
	mdlArc_create(&newarcIn, NULL, &centerpt, frontRadius, frontRadius, NULL, starR, -sweepR);
	MSElementDescrP msedIn;
	mdlElmdscr_new(&msedIn, NULL, &newarcIn);
	DEllipse3d ellipsePro;
	DPoint3d ArcDPs[2];
	RotMatrix rotM;
	double radius;
	mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcIn);
	mdlArc_extractDEllipse3d(&ellipsePro, &newarcIn);
	arcFront.ptStart = ArcDPs[0];
	arcFront.ptEnd = ArcDPs[1];
	arcFront.dRadius = frontRadius;
	arcFront.ptCenter = centerpt;
	arcFront.dLen = ellipsePro.ArcLength();
	mdlElmdscr_pointAtDistance(&arcFront.ptMid, NULL, arcFront.dLen / 2, msedIn, 1e-6);


	sweepAngle = endangleB - starangleB;
	if (scalef)
	{
		sweepAngle = 360 - sweepAngle;
		sweepR = -(sweepAngle) / 180 * fc_pi;
	}
	else
	{
		sweepR = (sweepAngle) / 180 * fc_pi;
	}
	//��ʼ�Ƕ���ɨ��Ƕȷ������ƶ���ɨ��ǶȼӴ�
	starR = (360 - starangleB) / 180 * fc_pi;

	mdlArc_create(&newarcOut, NULL, &centerpt, backRadius, backRadius, NULL, starR, -sweepR);


	MSElementDescrP  exmseOut;
	mdlElmdscr_new(&exmseOut, NULL, &newarcOut);

	//mdlElmdscr_add(msedIn);
	//mdlElmdscr_add(exmseOut);

	//�⻡����
	DEllipse3d ellipseRev;
	DPoint3d ArcoDPs[2];
	mdlArc_extract(ArcoDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &newarcOut);
	mdlArc_extractDEllipse3d(&ellipseRev, &newarcOut);
	arcBack.ptStart = ArcoDPs[0];
	arcBack.ptEnd = ArcoDPs[1];
	arcBack.dRadius = backRadius;
	arcBack.ptCenter = centerpt;
	arcBack.dLen = ellipseRev.ArcLength();
	mdlElmdscr_pointAtDistance(&arcBack.ptMid, NULL, arcBack.dLen / 2, exmseOut, 1e-6);

	m_ArcWallData.thickness = fabs(arcBack.dRadius - arcFront.dRadius);
	m_width = m_ArcWallData.thickness;
	m_ArcWallData.OuterArc = arcBack;
	m_ArcWallData.InnerArc = arcFront;

	DPoint3d tmppt = m_ArcWallData.InnerArc.ptStart;
	m_ArcWallData.InnerArc.ptStart = m_ArcWallData.InnerArc.ptEnd;
	m_ArcWallData.InnerArc.ptEnd = tmppt;

	//����⻡����
	m_outMaxArc = arcBack;
	sweepAngle = endangleF - starangleF;
	if (scalef)
	{
		sweepAngle = 360 - sweepAngle;
		sweepR = -(sweepAngle) / 180 * fc_pi;
	}
	else
	{
		sweepR = (sweepAngle) / 180 * fc_pi;
	}
	//��ʼ�Ƕ���ɨ��Ƕȷ������ƶ���ɨ��ǶȼӴ�
	starR = (360 - starangleF) / 180 * fc_pi;
	MSElement newarcMax;
	mdlArc_create(&newarcMax, NULL, &centerpt, backRadius, backRadius, NULL, starR, -sweepR);
	MSElementDescrP msedMax;
	mdlElmdscr_new(&msedMax, NULL, &newarcMax);
	DEllipse3d ellipseMax;
	DPoint3d maxArcDPs[2];
	RotMatrix maxRotM;
	mdlArc_extract(maxArcDPs, &starR, &sweepR, &radius, NULL, &maxRotM, &centerpt, &newarcMax);
	mdlArc_extractDEllipse3d(&ellipseMax, &newarcMax);
	m_outMaxArc.ptStart = maxArcDPs[0];
	m_outMaxArc.ptEnd = maxArcDPs[1];
	m_outMaxArc.dRadius = backRadius;
	m_outMaxArc.ptCenter = centerpt;
	m_outMaxArc.dLen = ellipseMax.ArcLength();
	mdlElmdscr_pointAtDistance(&m_outMaxArc.ptMid, NULL, m_outMaxArc.dLen / 2, msedMax, 1e-6);

	m_sideCoverAngle = 0;

	m_Holeehs = Holeehs;
	mdlElmdscr_freeAll(&exmseOut);
	mdlElmdscr_freeAll(&msedIn);


	/*if (vecDownFontLine.size() > 1 || vecDownBackLine.size() > 1)
	{
		std::for_each(vecDownFaceLine.begin(), vecDownFaceLine.end(), [](MSElementDescrP &ms) { mdlElmdscr_freeAll(&ms); });
		return false;
	}
	DEllipse3d ellipsePro;
	ArcSegment arcFront, arcBack;
	if (vecDownFontLine[0]->el.ehdr.type == ARC_ELM)
	{
		mdlArc_extractDEllipse3d(&ellipsePro, &vecDownFontLine[0]->el);
		ellipsePro.EvaluateEndPoints(arcFront.ptStart, arcFront.ptEnd);
		arcFront.dRadius = arcFront.ptStart.Distance(ellipsePro.center);
		arcFront.ptCenter = ellipsePro.center;
		arcFront.dLen = ellipsePro.ArcLength();
		mdlElmdscr_pointAtDistance(&arcFront.ptMid, NULL, arcFront.dLen / 2, vecDownFontLine[0], 1e-6);
	}
	DEllipse3d ellipseRev;
	for (size_t i = 0; i < vecDownBackLine.size(); i++)
	{
		if (vecDownBackLine[i]->el.ehdr.type == ARC_ELM)
		{
			mdlArc_extractDEllipse3d(&ellipseRev, &vecDownBackLine[i]->el);

			ellipseRev.EvaluateEndPoints(arcBack.ptStart, arcBack.ptEnd);
			arcBack.dRadius = arcBack.ptStart.Distance(ellipseRev.center);
			arcBack.ptCenter = ellipseRev.center;
			arcBack.dLen = ellipseRev.ArcLength();
			mdlElmdscr_pointAtDistance(&arcBack.ptMid, NULL, arcBack.dLen / 2, vecDownBackLine[i], 1e-6);
		}
	}
	m_ArcWallData.thickness = fabs(arcFront.dRadius - arcBack.dRadius);
	m_ArcWallData.OuterArc = arcFront.dRadius > arcBack.dRadius ? arcFront : arcBack;
	m_ArcWallData.InnerArc = arcFront.dRadius > arcBack.dRadius ? arcBack : arcFront;
	m_Holeehs = Holeehs;*/

	return true;
}

bool ArcWallRebarAssembly::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}
bool ArcWallRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	vector<int> vctemp;
	vctemp = GetvecDataExchange();
	NewRebarAssembly(modelRef);
	CalculateUseHoles(modelRef);
	RebarSetTagArray rsetTags;
	m_vecRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
	g_vecTieRebarPtsNoHole.clear();
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
	double dLevelSpace = 0;
	double dSideCover = GetSideCover()*uor_per_mm;


	double dArcLen = m_ArcWallData.OuterArc.dLen;
	double dWidth = m_ArcWallData.thickness;

	int iRebarSetTag = 0;
	int iRebarLevelNum = GetRebarLevelNum();
	int iTwinbarSetIdIndex = 0;
	vector<PIT::EndType> vecEndType;
	BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
	if (strTieRebarSize.Find(L"mm") != string::npos)
		strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//ɾ��mm
	double diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//����ֱ��
	double newRadius = m_ArcWallData.OuterArc.dRadius - GetPositiveCover() * uor_per_mm - diameterTie;
	double diameterPre = 0.0;

	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		m_vecRebarPtsLayer.clear();
		m_vecTieRebarPtsLayer.clear();
		RebarSetTag* tag = NULL;
		vector<PIT::EndType> vecEndType;
		if (GetvvecEndType().empty())		//û�����ö˲���ʽ������Ĭ��ֵ
		{
			EndType endType;
			memset(&endType, 0, sizeof(endType));
			vecEndType = { { 0,0,0 },{0,0,0} ,endType };
		}
		else
		{
			vecEndType = GetvvecEndType().at(i);
		}

		double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef);	//��ǰ��ֽ�ֱ��

		PIT::ArcSegment newArc = m_ArcWallData.OuterArc;
		if (0 == i)	//�ײ�ƫ�Ƶ�ǰ�ֽ�ֱ��
		{
			newRadius -= diameter * 0.5;
		}
		else
		{
			diameterPre = RebarCode::GetBarDiameter(GetvecDirSize().at(i - 1), modelRef);	//��һ��ֽ�ֱ��
			newRadius -= diameterPre * 0.5;	//ƫ����һ��ֽ�İ뾶
			newRadius -= diameter * 0.5;	//ƫ�Ƶ�ǰ��ֽ�İ뾶
		}

		newRadius -= GetvecLevelSpace().at(i) * uor_per_mm;	//ƫ�Ʋ���
		double offset = m_ArcWallData.OuterArc.dRadius - newRadius;	//��ƫ�Ƴ���
		double reverseCover = GetReverseCover()*uor_per_mm + diameter * 0.5 + diameterTie;	//����ʵ����Ԥ�����
		if (offset > m_ArcWallData.thickness - reverseCover)	//ƫ�Ƴ���ǽ
		{
			double reverseOffset = 0.0;
			for (int j = iRebarLevelNum - 1; j > i; --j)
			{
				double diameterLater = RebarCode::GetBarDiameter(GetvecDirSize().at(j), modelRef);	//������ֽ�ֱ��
				reverseOffset += diameter;
			}
			offset = m_ArcWallData.thickness - reverseCover - reverseOffset;
			newRadius = m_ArcWallData.OuterArc.dRadius - offset;
		}
		newArc.ScaleToRadius(newRadius);		//��������
		newArc.CutArc(m_ArcWallData.OuterArc.FptStart, m_ArcWallData.InnerArc.ptEnd);
		newArc.CutArc(m_ArcWallData.OuterArc.FptEnd, m_ArcWallData.InnerArc.ptStart);

		//PopvecSetId().push_back(0);
		vector<CVector3D> vecEndNormal(2);
		if (0 == GetvecDir().at(i))	//ˮƽ���θֽ�
		{
			CVector3D	endNormal;	//�˲��乳����
			if (GetvvecEndType().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					CVector3D rebarVec = m_ArcWallData.OuterArc.FptEnd - m_ArcWallData.OuterArc.FptStart;
					endNormal = CVector3D::From(0, 0, -1);
					if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
					{
						endNormal.Negate();
					}
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
					vecEndNormal[k] = endNormal;
				}
			}
			newArc.Shorten(dSideCover, false);		//�������
			newArc.Shorten(dSideCover, true);		//�յ�����
			newArc.OffsetByAxisZ(diameter * 0.5 + GetSideCover()*uor_per_mm + diameterTie);
			//���Ʋ���--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
			{
				m_isPushTieRebar = true;
				//�Ȼ��Ʒǲ����
				PopvecSetId().push_back(0);
				tag = MakeRebars_Arc(PopvecSetId().back(), GetvecDirSize().at(i), newArc, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}

				m_isPushTieRebar = false;
				PopvecSetId().push_back(0);
				//���Ʋ����
				tag = MakeRebars_Arc(PopvecSetId().back(), GetvecDirSize().at(i), newArc, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
			else //��ǰ��δ���ò���
			{
				m_isPushTieRebar = true;
				TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
				PopvecSetId().push_back(0);
				tag = MakeRebars_Arc(PopvecSetId().back(), GetvecDirSize().at(i), newArc, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}
			}
			vecEndType.clear();
		}
		else
		{
			CVector3D	endNormal;	//�˲��乳����
			if (GetvvecEndType().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = m_ArcWallData.OuterArc.FptEnd - m_ArcWallData.OuterArc.FptStart;
					endNormal.Normalize();
					if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
					{
						endNormal.Negate();
					}
					CVector3D rebarVec = CVector3D::kZaxis;
					/*					endNormal = rebarVec.CrossProduct(vec);*/
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
					vecEndNormal[k] = endNormal;
				}
			}

			newArc.Shorten(dSideCover + diameter * 0.5, false);		//�������
			newArc.Shorten(dSideCover + diameter * 0.5, true);		//�յ�����
			newArc.OffsetByAxisZ(GetSideCover()*uor_per_mm + diameterTie);
			ArcSegment newArcSeg = newArc;
			double newSpacing = GetvecDirSpacing().at(i);
			if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2) //�������
			{
				GetNewArcAndSpacing(newArc, newArcSeg, GetvecAngle().at(i), newSpacing);
				if (m_sideCoverAngle == 0)
				{
					m_sideCoverAngle = (dSideCover + diameter * 0.5) / (newArcSeg.dRadius / 10 * PI / 180);
				}
				double shortenLen = newArcSeg.dRadius / 10 * PI / 180 * m_sideCoverAngle;
				newArcSeg.Shorten(shortenLen, false);		//�������
				newArcSeg.Shorten(shortenLen, true);		//�յ�����
				vector<double> vecSpacing = GetvecDirSpacing();
				vecSpacing[i] = newSpacing;
				SetvecDirSpacing(vecSpacing);
			}
			double dLen = m_ArcWallData.height - (GetSideCover()*uor_per_mm + diameterTie) * 2;
			//���Ʋ���--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
			{
				m_isPushTieRebar = false;
				//�Ȼ��Ʒǲ����
				PopvecSetId().push_back(0);
				tag = MakeRebars_Line(PopvecSetId().back(), GetvecDirSize().at(i), newArcSeg, dLen, newSpacing*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}

				m_isPushTieRebar = true;
				//���Ʋ����
				PopvecSetId().push_back(0);
				tag = MakeRebars_Line(PopvecSetId().back(), GetvecDirSize().at(i), newArcSeg, dLen, newSpacing*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
			else //��ǰ��δ���ò���
			{
				m_isPushTieRebar = true;
				TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
				PopvecSetId().push_back(0);
				tag = MakeRebars_Line(PopvecSetId().back(), GetvecDirSize().at(i), newArcSeg, dLen, newSpacing*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}
			}
			vecEndType.clear();
		}
		if (m_vecRebarPtsLayer.size() > 1)
		{
			if (GetvecDir().at(i) == 0)//X��ʱ���洢��������
			{
				for (int m = 0; m < m_vecRebarPtsLayer.size() - 2; m++)
				{
					int n = m + 1;
					int k = n + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecRebarPtsLayer.at(m);
					rbPt.ptmid = m_vecRebarPtsLayer.at(n);
					rbPt.ptend = m_vecRebarPtsLayer.at(k);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecRebarPtsNoHole.push_back(rbPt);
					m = m + 2;
				}
			}
			else
			{
				for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecRebarPtsLayer.at(m);
					rbPt.ptend = m_vecRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
		}
		if (m_vecTieRebarPtsLayer.size() > 1)
		{
			if (GetvecDir().at(i) == 0)//X��ʱ���洢��������
			{
				for (int m = 0; m < m_vecTieRebarPtsLayer.size() - 2; m++)
				{
					int n = m + 1;
					int k = n + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTieRebarPtsLayer.at(m);
					rbPt.ptmid = m_vecTieRebarPtsLayer.at(n);
					rbPt.ptend = m_vecTieRebarPtsLayer.at(k);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTieRebarPtsNoHole.push_back(rbPt);
					m = m + 2;
				}
			}
			else
			{
				for (int m = 0; m < m_vecTieRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTieRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTieRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTieRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
		}
	}
	/*for (RebarPoint pt:g_vecRebarPtsNoHole)
	{
		if (pt.vecDir==0)
		{
			EditElementHandle arceeh;
			ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(pt.ptstr, pt.ptmid, pt.ptend), true, *ACTIVEMODEL);
			arceeh.AddToModel();
		}
		else
		{
			EditElementHandle tmpeeh;
			LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(pt.ptstr, pt.ptend), true, *ACTIVEMODEL);
			tmpeeh.AddToModel();
		}
	}*/



	//�������--begin
	vector<vector<DSegment3d>> vctTieRebarLines;//�洢���е�����ֱ����Ϣ������Ԥ��
	if (0 != GetTieRebarInfo().tieRebarMethod/* && (m_vecAllRebarStartEnd.size() >= 4)*/)
	{
		FaceRebarDataArray faceDataArray;
		faceDataArray.posRebarData.HRebarData.rebarSize = GetvecDirSize().at(0);
		faceDataArray.posRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(0);
		faceDataArray.posRebarData.VRebarData.rebarSize = GetvecDirSize().at(1);
		faceDataArray.posRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(1);

		faceDataArray.revRebarData.HRebarData.rebarSize = GetvecDirSize().at(GetvecDirSize().size() - 1);
		faceDataArray.revRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecDirSize().size() - 1);
		faceDataArray.revRebarData.VRebarData.rebarSize = GetvecDirSize().at(GetvecDirSize().size() - 2);
		faceDataArray.revRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecDirSize().size() - 2);

		vector<vector<RebarPoint> > vecStartEnd;		//ֻ�洢1��2��͵�����1��2��
		vector<vector<RebarPoint> > vvecSeg;
		int index = 0;
		vector<RebarPoint> vecSeg;
		for (size_t i = 0; i < g_vecTieRebarPtsNoHole.size(); ++i)
		{
			if (index != g_vecTieRebarPtsNoHole[i].iIndex)
			{
				vvecSeg.push_back(vecSeg);
				vecSeg.clear();
			}
			DSegment3d seg = DSegment3d::From(g_vecTieRebarPtsNoHole[i].ptstr, g_vecTieRebarPtsNoHole[i].ptend);
			vecSeg.push_back(g_vecTieRebarPtsNoHole[i]);
			index = g_vecTieRebarPtsNoHole[i].iIndex;
			if (i == g_vecTieRebarPtsNoHole.size() - 1)
			{
				vvecSeg.push_back(vecSeg);
			}
		}

		if (vvecSeg.size() < 4)
		{
			if (g_globalpara.Getrebarstyle() != 0)
			{
				return (SUCCESS == AddRebarSets(rsetTags));
			}
			return true;
		}

		vecStartEnd.push_back(vvecSeg[0]);
		vecStartEnd.push_back(vvecSeg[1]);
		vecStartEnd.push_back(vvecSeg[vvecSeg.size() - 2]);
		vecStartEnd.push_back(vvecSeg[vvecSeg.size() - 1]);
		BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
		int	tieRebarStyle = GetTieRebarInfo().tieRebarStyle;
		if (strTieRebarSize.Find(L"mm") != string::npos)
			strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//ɾ��mm

		PopvecSetId().push_back(0);

		vector<vector<DSegment3d> > vecStartEndTmp;		//ֻ�洢1��2��͵�����1��2��
		TieRebarMaker tieRebarMaker(faceDataArray, vecStartEndTmp, (TieRebarStyle)tieRebarStyle, strTieRebarSize);
		tieRebarMaker.m_CallerId = GetCallerId();
		tieRebarMaker.SetArcStartEnd(vecStartEnd);
		tieRebarMaker.SetCustomStyle(GetTieRebarInfo().rowInterval, GetTieRebarInfo().colInterval);
		tieRebarMaker.SetModeType(0);
		Transform trans;
		GetPlacement().AssignTo(trans);
		tieRebarMaker.SetTrans(trans);
		vector<EditElementHandle*> vecAllSolid;
		vecAllSolid.insert(vecAllSolid.begin(), m_Holeehs.begin(), m_Holeehs.end());
		tieRebarMaker.SetDownVec(m_ArcWallData.OuterArc.ptStart, m_ArcWallData.OuterArc.ptEnd);
		tieRebarMaker.SetArcPoint(m_ArcWallData.OuterArc.ptStart, m_ArcWallData.OuterArc.ptEnd, m_ArcWallData.OuterArc.ptCenter);
		tieRebarMaker.SetHoles(vecAllSolid);
		tieRebarMaker.SetHoleCover(GetSideCover()*uor_per_mm);
		RebarSetTag* tag = tieRebarMaker.MakeRebar(PopvecSetId().back(), modelRef);
		tieRebarMaker.GetRebarPts(vctTieRebarLines);//ȡ�����е�����ֱ����Ϣ
		if (NULL != tag && (!PreviewButtonDown))
		{
			tag->SetBarSetTag(iRebarLevelNum + 1);
			rsetTags.Add(tag);
		}
	}

	if (PreviewButtonDown)//Ԥ����ť���£���������
	{
		m_allLines.clear();
		for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)
		{

			vector<vector<DPoint3d>> faceLinePts = *it;
			for (auto it : faceLinePts)
			{
				vector<DPoint3d> linePts = it;
				if (linePts.size() == 2)
				{
					DPoint3d strPoint = *linePts.begin();
					DPoint3d endPoint = *linePts.rbegin();
					CVector3D vecIndex = endPoint - strPoint;
					if (COMPARE_VALUES_EPS(vecIndex.z, 0.0, uor_per_mm) == 0)
					{
						DPoint3d arcCenter = m_ArcWallData.OuterArc.ptCenter;
						arcCenter.z = strPoint.z;
						EditElementHandle arceeh;
						ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromArcCenterStartEnd(arcCenter, strPoint, endPoint), true, *ACTIVEMODEL);
						arceeh.AddToModel();
						m_allLines.push_back(arceeh.GetElementRef());
					}
					else
					{
						EditElementHandle eeh;
						LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
						eeh.AddToModel();
						m_allLines.push_back(eeh.GetElementRef());
					}
				}
				else
				{
					EditElementHandle eeh;
					LineStringHandler::CreateLineStringElement(eeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
					eeh.AddToModel();
					m_allLines.push_back(eeh.GetElementRef());
				}

			}
		}

		for (auto itr = vctTieRebarLines.begin(); itr != vctTieRebarLines.end(); itr++)
		{
			vector<DSegment3d> vcttemp(*itr);
			for (int y = 0; y < vcttemp.size(); y++)
			{
				DPoint3d strPoint = DPoint3d::From(vcttemp[y].point[0].x, vcttemp[y].point[0].y, vcttemp[y].point[0].z);
				DPoint3d endPoint = DPoint3d::From(vcttemp[y].point[1].x, vcttemp[y].point[1].y, vcttemp[y].point[1].z);
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allLines.push_back(eeh.GetElementRef());
			}
		}
		return true;
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}


RebarSetTag* ArcWallRebarAssembly::MakeRebars_Line
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	PIT::ArcSegment			arcSeg,
	double              dLen,
	double              spacing,
	double              startOffset,
	double              endOffset,
	vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
	vector<CVector3D> const& vecEndNormal,
	TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
	int level,
	int grade,
	int DataExchange,
	bool				bTwinbarLevel,
	DgnModelRefP        modelRef
)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, startbendLenTb, endbendLen, endbendLenTb;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);	//������30
	double adjustedXLen, adjustedSpacing;

	double sideCov = GetSideCover()*uor_per_mm;
	int numRebar = 0;
	if (twinBarInfo.hasTwinbars)	//����
		adjustedXLen = arcSeg.dLen - sideCov - diameter - diameterTb - startOffset - endOffset;
	else
		adjustedXLen = arcSeg.dLen - sideCov - diameter - startOffset - endOffset;
	if (bTwinbarLevel)				//�����ֽ�����
	{
		numRebar = (int)floor(adjustedXLen / (spacing * (twinBarInfo.interval + 1)) + 0.85) + 1;
		int numRebar1 = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar1 > 1 && g_wallRebarInfo.concrete.m_SlabRebarMethod != 2) //�Ƿ������
		{
			adjustedSpacing = adjustedXLen / (numRebar1 - 1);	//�ǲ����ƽ�����
			adjustedSpacing *= (twinBarInfo.interval + 1);		//�����ʵ�ʼ������Ըֽ���
		}
	}
	else
	{
		numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar > 1 && g_wallRebarInfo.concrete.m_SlabRebarMethod != 2)
			adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	double xPos = startOffset;
	if (bTwinbarLevel)				//�������ƫ��һ�ξ���
	{
		xPos += diameter * 0.5;
		if (diameterTb > diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radiusTb - radius)*(radiusTb - radius), 0.5) - radius;
			xPos += dOffset;
		}
		else if (diameterTb < diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radius - radiusTb)*(radius - radiusTb), 0.5) - radius;
			xPos += dOffset;
		}
		else
		{
			xPos += diameter * 0.5;
		}
	}
	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypEendOffset += diameter * 0.5;

	switch (endType[0].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeStart.SetType(RebarEndType::kLap);
		begStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90���乳
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		break;
	case 5:	//135���乳
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		break;
	case 6:	//180���乳
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		break;
	case 8:	//�û�
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (endType[1].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeEnd.SetType(RebarEndType::kLap);
		endStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90���乳
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		break;
	case 5:	//135���乳
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		break;
	case 6:	//180���乳
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		break;
	case 8:	//�û�
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		start.SetbendLen(startbendLenTb);
		start.SetbendRadius(bendRadiusTb);
	}
	else
	{
		start.SetbendLen(startbendLen);
		start.SetbendRadius(startbendRadius);
	}
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		end.SetbendLen(endbendLenTb);
		end.SetbendRadius(bendRadiusTb);
	}
	else
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };
	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinBarInfo.rebarSize, symb);
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}
	else
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}

	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurve;
		//RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
		makeLineRebarCurve(rebarCurve, arcSeg, dLen, xPos, endTypeStartOffset, endTypEendOffset, endTypes);

		xPos += adjustedSpacing;

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurve.begin(), rebarCurve.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	vector<vector<DPoint3d>> vecStartEnd;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (ISPointInHoles(m_Holeehs, midPos))
		{
			if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
			{
				continue;
			}
		}

		vector<DPoint3d> linePts;
		RebarVertices vertices = rebarCurve.GetVertices();
		for (size_t i = 0; i < (size_t)vertices.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &vertices.At(i);
			linePts.push_back(tmpVertex->GetIP());
		}
		/*EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.AddToModel();
*/
		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinBarInfo.rebarSize));
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameterTb, endTypes, shape, modelRef, false);
			}
			else
			{
				shape.SetSizeKey((LPCTSTR)sizeKey);
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
			}
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (DataExchange == 0)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinback";
				else
					Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
		j++;
		vecStartEnd.push_back(linePts);
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

RebarSetTag * ArcWallRebarAssembly::MakeRebars_Arc(ElementId & rebarSetId, BrStringCR sizeKey, PIT::ArcSegment arcSeg, double spacing, double startOffset, double endOffset, vector<PIT::EndType> const & endType, vector<CVector3D> const & vecEndNormal, TwinBarSet::TwinBarLevelInfo const & twinBarInfo, int level, int grade, int DataExchange, bool bTwinbarLevel, DgnModelRefP modelRef)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, startbendLenTb, endbendLen, endbendLenTb;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);
	double adjustedXLen, adjustedSpacing;

	BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
	if (strTieRebarSize.Find(L"mm") != string::npos)
		strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//ɾ��mm
	double diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//����ֱ��

	double sideCov = GetSideCover()*uor_per_mm;
	int numRebar = 0;
	if (twinBarInfo.hasTwinbars)	//����
		adjustedXLen = m_ArcWallData.height - sideCov * 2 - diameter - diameterTie * 2 - diameterTb - startOffset - endOffset;
	else
		adjustedXLen = m_ArcWallData.height - sideCov * 2 - diameter - diameterTie * 2 - startOffset - endOffset;
	if (bTwinbarLevel)				//�����ֽ�����
	{
		numRebar = (int)floor(adjustedXLen / (spacing * (twinBarInfo.interval + 1)) + 0.85) + 1;
		int numRebar1 = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar1 > 1)
		{
			adjustedSpacing = adjustedXLen / (numRebar1 - 1);	//�ǲ����ƽ�����
			adjustedSpacing *= (twinBarInfo.interval + 1);		//�����ʵ�ʼ������Ըֽ���
		}
	}
	else
	{
		numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar > 1)
			adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	double xPos = startOffset;
	if (bTwinbarLevel)				//�������ƫ��һ�ξ���
	{
		xPos += diameter * 0.5;
		if (diameterTb > diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radiusTb - radius)*(radiusTb - radius), 0.5) - radius;
			xPos += dOffset;
		}
		else if (diameterTb < diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radius - radiusTb)*(radius - radiusTb), 0.5) - radius;
			xPos += dOffset;
		}
		else
		{
			xPos += diameter * 0.5;
		}
	}
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypEendOffset += diameter * 0.5;

	switch (endType[0].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeStart.SetType(RebarEndType::kLap);
		begStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90���乳
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		break;
	case 5:	//135���乳
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		break;
	case 6:	//180���乳
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		break;
	case 8:	//�û�
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (endType[1].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeEnd.SetType(RebarEndType::kLap);
		endStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90���乳
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		break;
	case 5:	//135���乳
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		break;
	case 6:	//180���乳
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		break;
	case 8:	//�û�
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		start.SetbendLen(startbendLenTb);
		start.SetbendRadius(bendRadiusTb);
	}
	else
	{
		start.SetbendLen(startbendLen);
		start.SetbendRadius(startbendRadius);
	}
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		end.SetbendLen(endbendLenTb);
		end.SetbendRadius(bendRadiusTb);
	}
	else
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };
	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinBarInfo.rebarSize, symb);
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}
	else
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	vector<PITRebarCurve>     rebarCurvesNum;
	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurve;
		//		RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
		makeArcWallRebarCurve(rebarCurve, arcSeg, xPos, endTypeStartOffset, endTypEendOffset, endTypes);

		xPos += adjustedSpacing;

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurve.begin(), rebarCurve.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	vector<vector<DPoint3d>> vecStartEnd;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (ISPointInHoles(m_Holeehs, midPos))
		{
			if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
			{
				continue;
			}
		}

		vector<DPoint3d> linePts;
		linePts.push_back(ptstr);
		linePts.push_back(ptend);
		//vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));
		/*	EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			eeh.AddToModel();
*/
		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinBarInfo.rebarSize));
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameterTb, endTypes, shape, modelRef, false);
			}
			else
			{
				shape.SetSizeKey((LPCTSTR)sizeKey);
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
			}
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);

			string Stype;
			if (DataExchange == 0)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinback";
				else
					Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
		j++;
		vecStartEnd.push_back(linePts);
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}


void ArcWallRebarAssembly::GetNewArcAndSpacing(PIT::ArcSegment oldArc, PIT::ArcSegment& newArc, double angle, double& newSpacing)
{
	//�����ɵĻ���
	EditElementHandle oldArcEeh;
	ArcHandler::CreateArcElement(oldArcEeh, nullptr, DEllipse3d::FromCenterRadiusXY(oldArc.ptCenter, oldArc.dRadius), true, *ACTIVEMODEL);

	//�⻡��ʼ�㵽���ĵ��ֱ��
	EditElementHandle strLineEeh;
	LineHandler::CreateLineElement(strLineEeh, nullptr, DSegment3d::From(m_outMaxArc.ptStart, m_outMaxArc.ptCenter), true, *ACTIVEMODEL);

	//�⻡�����㵽���ĵ��ֱ��
	EditElementHandle endLineEeh;
	LineHandler::CreateLineElement(endLineEeh, nullptr, DSegment3d::From(m_outMaxArc.ptEnd, m_outMaxArc.ptCenter), true, *ACTIVEMODEL);

	//�⻡�м�㵽���ĵ��ֱ��
	EditElementHandle midLineEeh;
	LineHandler::CreateLineElement(midLineEeh, nullptr, DSegment3d::From(m_outMaxArc.ptMid, m_outMaxArc.ptCenter), true, *ACTIVEMODEL);

	//�ɵĻ��ߺ�����ֱ�ߵĽ���Ϊ�»��ߵ���ʼ����յ�
	mdlIntersect_allBetweenElms(&newArc.ptStart, nullptr, 1, oldArcEeh.GetElementDescrP(), strLineEeh.GetElementDescrP(), nullptr, 1/*, nullptr, nullptr*/);
	mdlIntersect_allBetweenElms(&newArc.ptEnd, nullptr, 1, oldArcEeh.GetElementDescrP(), endLineEeh.GetElementDescrP(), nullptr, 1/*, nullptr, nullptr*/);
	mdlIntersect_allBetweenElms(&newArc.ptMid, nullptr, 1, oldArcEeh.GetElementDescrP(), midLineEeh.GetElementDescrP(), nullptr, 1/*, nullptr, nullptr*/);
	newArc.ptCenter = oldArc.ptCenter;

	//�µĻ���
	DEllipse3d newEllipse = DEllipse3d::FromPointsOnArc(newArc.ptStart, newArc.ptMid, newArc.ptEnd);
	EditElementHandle newArcEeh;
	ArcHandler::CreateArcElement(newArcEeh, nullptr, newEllipse, true, *ACTIVEMODEL);
	RotMatrix rotM;
	// double radius;
	mdlArc_extract(nullptr, nullptr, nullptr, &newArc.dRadius, NULL, &rotM, nullptr, newArcEeh.GetElementCP());
	newArc.dLen = newEllipse.ArcLength();

	newSpacing = newArc.dRadius / 10 * PI / 180 * angle;
}

bool ELLWallRebarAssembly::AnalyzingWallGeometricData(ElementHandleCR eh)//������������
{
	ELLWallGeometryInfo m_ELLWallDatarc;
	GetElementXAttribute(eh.GetElementId(), sizeof(m_ELLWallDatarc), m_ELLWallDatarc, RoundrebarGroup, ACTIVEMODEL);
	if (m_ELLWallDatarc.type == ELLIPSEWall)
	{
		m_ELLWallData = m_ELLWallDatarc;
		return  true;
	}

	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	// DPoint3d FrontStr, FrontEnd;
	// DPoint3d BackStr, BackEnd;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	vector<DSegment3d> vecDownFontLine;
	vector<DSegment3d> vecDownBackLine;

	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();
	m_doorsholes.clear();
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	GetDoorHoles(Holeehs, m_doorsholes);

	// ȡ���� ���ε���
	vector<MSElementDescrP> vecEllipse;
	EFT::GetEllipseDownFace(Eleeh, vecEllipse, Holeehs, &m_ELLWallData.dHeight);

	double minRadius = 0.0;
	for (MSElementDescrP& ms : vecEllipse)
	{
		double dRadius = 0.0;
		DPoint3d centerpt;
		DPoint3d ArcDPs[2];

		mdlArc_extract(ArcDPs, NULL, NULL, &dRadius, NULL, NULL, &centerpt, &ms->el);
		//mdlElmdscr_freeAll(&ms);

		if (COMPARE_VALUES_EPS(dRadius, m_ELLWallData.dRadiusOut, EPS) > 0) // ���뾶��Բ
		{
			m_ELLWallData.dRadiusOut = dRadius;
			m_ELLWallData.ArcDPs[0] = ArcDPs[0];
			m_ELLWallData.ArcDPs[1] = ArcDPs[1];
			m_ELLWallData.centerpt = centerpt;
		}

		//if (fabs(minRadius - 0.0) < 10 || COMPARE_VALUES_EPS(minRadius, dRadius, EPS) > 0) // ��С�뾶��Բ
		//{
		//	minRadius = dRadius;
		//}
	}
	for (MSElementDescrP& ms : vecEllipse)
	{
		double dRadius = 0.0;
		DPoint3d centerpt;
		DPoint3d ArcDPs[2];

		mdlArc_extract(ArcDPs, NULL, NULL, &dRadius, NULL, NULL, &centerpt, &ms->el);
		mdlElmdscr_freeAll(&ms);
		if (dRadius == 0 || !centerpt.AlmostEqual(m_ELLWallData.centerpt))
		{
			continue;
		}

		if (fabs(minRadius - 0.0) < 10 || COMPARE_VALUES_EPS(minRadius, dRadius, EPS) > 0) // ��С�뾶��Բ
		{
			minRadius = dRadius;
		}
	}
	m_ELLWallData.dRadiusInn = minRadius;

	m_Holeehs = Holeehs;
	return true;
}


bool ELLWallRebarAssembly::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}


void ELLWallRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetSideCover()*uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

	DPoint3d zeropt = DPoint3d::From(0, 0, 0);
	DPoint3d tmpvecZ = DPoint3d::From(0, 0, 1);
	Transform tran;			//����ͶӰ����
	mdlTMatrix_computeFlattenTransform(&tran, &zeropt, &tmpvecZ);
	vector<CurveVectorPtr> curveVectors = CreateBreakArcRange(tran);

	if (g_wallRebarInfo.concrete.isHandleHole)//������Ҫ����Ŀ׶�
	{
		for (int j = 0; j < m_Holeehs.size(); j++)
		{
			EditElementHandle eeh;
			eeh.Duplicate(*m_Holeehs.at(j));

			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
			}

			TransformInfo transinfo(trans);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			DPoint3d minP;
			DPoint3d maxP;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			MSElementDescrP pHole = eeh.GetElementDescrP();
			mdlElmdscr_computeRange(&minP, &maxP, pHole, NULL);
			DRange3d range;
			range.low = minP;
			range.high = maxP;
			if (COMPARE_VALUES_EPS(range.XLength(), misssize, 1e-6) == -1 &&
				COMPARE_VALUES_EPS(range.ZLength(), misssize, 1e-6) == -1)
			{
				continue;
			}
			//bool isNeed = false;
			//if (range.XLength() > misssize || range.ZLength() > misssize)
			//{
			//	isNeed = true;
			//}
			mdlElmdscr_transform(&pHole, &tran);
			mdlElmdscr_computeRange(&minP, &maxP, pHole, NULL);
			DPoint3d midPt = minP; midPt.Add(maxP); midPt.Scale(0.5);
			double dis = midPt.Distance(m_ELLWallData.centerpt);
			if (COMPARE_VALUES_EPS(dis, m_ELLWallData.dRadiusInn, 1) == -1)
			{
				continue;
			}





			//if (isNeed)
			//{
				//if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//������Ŷ�
				//{
				//	continue;
				//}
			bool isdoorNeg = false;//�ж��Ƿ�Ϊ�Ŷ�NEG
			isdoorNeg = IsDoorHoleNeg(m_Holeehs.at(j), m_doorsholes);
			ElementCopyContext copier(ACTIVEMODEL);
			copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
			copier.SetTransformToDestination(true);
			copier.SetWriteElements(false);
			copier.DoCopy(*m_Holeehs.at(j));

			if (isdoorNeg)
			{
				PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix, isdoorNeg);
			}
			else
			{
				PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
			}
			int index = 0;
			for (auto it : curveVectors)
			{
				CurveVector::InOutClassification pos1 = it->PointInOnOutXY(minP);
				CurveVector::InOutClassification pos2 = it->PointInOnOutXY(maxP);
				CurveVector::InOutClassification pos3 = it->PointInOnOutXY(midPt);
				if (CurveVector::INOUT_In == pos1 ||
					CurveVector::INOUT_In == pos2 ||
					CurveVector::INOUT_In == pos3)
				{
					m_vecUseHoles[index].push_back(m_Holeehs.at(j));
				}
				++index;
			}
			m_useHoleehs.push_back(m_Holeehs.at(j));
			//}
		}
	}
	for (auto& it : m_vecUseHoles)
	{
		if (it.second.size() > 1)
		{
			UnionIntersectHoles(it.second, m_Holeehs);
		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}

}

long ELLWallRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}

// ���ɵ��
RebarSetTag* ELLWallRebarAssembly::MakeRebar_Vertical
(
	ElementId& rebarSetId,
	BrString sizeKey,
	DgnModelRefP modelRef,
	double startOffset,  // ��ʼƫ��
	double endOffset,    // �յ�ƫ��
	double spacing,		 // ���
	double dRoundRadius, // Բ�İ뾶
	double rebarLen,		 // �ֽ��
	int level,
	int grade,
	int DataExchange,
	bool isTwinRebar // �Ƿ��ǲ���
)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	vector<PIT::EndType> endType; // �˲���ʽ
	PIT::EndType endFir;
	PIT::EndType endSec;
	endFir.rotateAngle = 0.00;
	endFir.endType = 0;
	endFir.offset = 0.00;

	endSec.rotateAngle = 0.00;
	endSec.endType = 0;
	endSec.offset = 0.00;

	endType.push_back(endFir);
	endType.push_back(endSec);

	vector<CVector3D>  vecEndNormal; // �˲��乳����
	CVector3D vecFir = CVector3D::From(0, 0, 0);
	CVector3D vecSec = CVector3D::From(0, 0, 0);
	vecEndNormal.push_back(vecFir);
	vecEndNormal.push_back(vecSec);

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);

	EditElementHandle eehRound;
	if (SUCCESS != EllipseHandler::CreateEllipseElement(eehRound, NULL, m_ELLWallData.centerpt, dRoundRadius, dRoundRadius, 0, true, *modelRef))
	{
		return NULL;
	}

	DPoint3d ArcDPs[2];
	mdlArc_extract(ArcDPs, NULL, NULL, NULL, NULL, NULL, NULL, &eehRound.GetElementDescrP()->el);

	double adjustedSpacing = 0.00;
	double adjustedXLen = 0.00;

	// mdlElmdscr_distanceAtPoint(&adjustedXLen, nullptr, nullptr, eehRound.GetElementDescrP(), &ArcDPs[1], 0.1);
	adjustedXLen = 2 * dRoundRadius * PI - diameter - startOffset - endOffset;

	int numRebar = (int)floor(adjustedXLen / spacing + 0.85);
	adjustedSpacing = spacing;
	if (numRebar > 1 && g_wallRebarInfo.concrete.m_SlabRebarMethod != 2)
	{
		adjustedSpacing = adjustedXLen / (numRebar);
	}

	if (numRebar > 1 && g_wallRebarInfo.concrete.m_SlabRebarMethod != 2)
	{
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	if (isTwinRebar)
	{
		numRebar = m_reabrTwinData.rebarNum;
		adjustedSpacing = m_reabrTwinData.spacing;
	}
	else
	{
		m_reabrTwinData.rebarNum = numRebar;
		m_reabrTwinData.spacing = adjustedSpacing;
		m_reabrTwinData.diameter = diameter;
	}

	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);
	PITRebarEndTypes   endTypes = { start, end };

	vector<PITRebarCurve>     rebarCurvesNum;

	double xPos = startOffset + diameter * 0.5;
	for (int i = 0; i < numRebar; i++)
	{
		DPoint3d ptStr = DPoint3d::FromZero();
		// �ƶ�ָ����������
		mdlElmdscr_pointAtDistance(&ptStr, nullptr, xPos, eehRound.GetElementDescrP(), 0.1);
		movePoint(CVector3D::kZaxis, ptStr, GetSideCover() * uor_per_mm);

		makeRebarCurve(rebarCurvesNum, endTypes, ptStr, rebarLen - GetSideCover() * uor_per_mm * 2);

		if (isTwinRebar && i == numRebar - 2)
		{
			xPos += adjustedSpacing - m_reabrTwinData.diameter - diameter;
			continue;
		}

		xPos += adjustedSpacing;

	}

	eehRound.DeleteFromModel();

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);

	if (!isTwinRebar)
	{
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	else
	{
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}

	vector<vector<DPoint3d>> vecStartEnd;
	for (int j = 0; j < (int)rebarCurvesNum.size(); ++j)
	{
		PITRebarCurve rebarCurve = rebarCurvesNum[j];
		RebarElementP rebarElement = nullptr;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}

		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);

		vector<DPoint3d> linePts;
		RebarVertices vertices = rebarCurve.GetVertices();
		for (size_t i = 0; i < (size_t)vertices.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &vertices.At(i);
			linePts.push_back(tmpVertex->GetIP());
		}

		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (DataExchange == 0)
			{
				if (isTwinRebar)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (isTwinRebar)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (isTwinRebar)
					Stype = "Twinback";
				else
					Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		vecStartEnd.push_back(linePts);
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}


// ����Բ�θֽ�
RebarSetTag* ELLWallRebarAssembly::MakeRebar_Round
(
	ElementId& rebarSetId,
	BrString sizeKey,
	DgnModelRefP modelRef,
	double startOffset, // ��ʼƫ��
	double endOffset,   // �յ�ƫ��
	double spacing,		// ���
	double dRoundRadius,// Բ�İ뾶
	int level,
	int grade,
	int DataExchange,
	bool isTwinRebar
)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	vector<PIT::EndType> endType; // �˲���ʽ
	PIT::EndType endFir;
	PIT::EndType endSec;
	endFir.rotateAngle = 0.00;
	endFir.endType = 0;
	endFir.offset = 0.00;

	endSec.rotateAngle = 0.00;
	endSec.endType = 0;
	endSec.offset = 0.00;

	endType.push_back(endFir);
	endType.push_back(endSec);

	vector<CVector3D>  vecEndNormal; // �˲��乳����
	CVector3D vecFir = CVector3D::From(0, 0, 0);
	CVector3D vecSec = CVector3D::From(0, 0, 0);
	vecEndNormal.push_back(vecFir);
	vecEndNormal.push_back(vecSec);

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);

	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	if (!isTwinRebar)
	{
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	else
	{
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}

	// ƫ�ƾ��룬Բ�ΰ���ֱ���ƫ��
	double adjustedSpacing = 0.0;
	double adjustedXLen = m_ELLWallData.dHeight - diameter - startOffset - endOffset - GetSideCover() * uor_per_mm * 2;
	int numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
	adjustedSpacing = spacing;
	// end

	if (numRebar > 1)
	{
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	if (isTwinRebar) // �����
	{
		numRebar = m_reabrTwinData.rebarNum;
		adjustedSpacing = m_reabrTwinData.spacing;
	}
	else
	{
		// Բ��ǽ���Χ̫�󣬰�֮ǰ���㷨�������е����⣬�˴���¼�����ƫ�����ͼ�࣬����
		// ������ʱ ��������Ļ�������ƫ�� ����뾶 + ����뾶
		m_reabrTwinData.rebarNum = numRebar;
		m_reabrTwinData.spacing = adjustedSpacing;
		m_reabrTwinData.diameter = diameter;
	}

	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };

	double xPos = startOffset + diameter * 0.5 + GetSideCover() * uor_per_mm;

	vector<PITRebarCurve>   rebarCurvesNum;
	DPoint3d centerPt = m_ELLWallData.centerpt;
	movePoint(CVector3D::kZaxis, centerPt, xPos);

	for (int i = 0; i < numRebar; i++)
	{
		if (m_vecBreakData.size() <= 1)
		{
			EditElementHandle eehRound;
			if (SUCCESS != EllipseHandler::CreateEllipseElement(eehRound, NULL, centerPt, dRoundRadius, dRoundRadius, 0, true, *modelRef))
			{
				return false;
			}
			vector<MSElementDescrP> vecTmp;
			vector<DPoint3d> vecArcIntersect;
			// Բ�θֽ���׶������õ����θֽֻ��һ��ȱ�ڵĻ����� �ཻ�� vecArcIntersect
			EFT::IntersectHoleEllWall(eehRound, centerPt, m_useHoleehs, vecTmp, vecArcIntersect);
			if (vecTmp.size() == 0) // û�б��׶��и� -- Բ��
			{
				makeRoundRebarCurve(rebarCurvesNum, endTypes, centerPt, dRoundRadius);
			}
			else // ���׶��и�ɻ���
			{
				// ��ÿһ�λ�
				makeArcRebarCurve(rebarCurvesNum, vecTmp.at(0), endTypes, vecArcIntersect);
			}

		}
		else //����
		{
			int index = 0;
			for (auto it : m_vecBreakData)
			{
				//MSElement arcElem;
				//double strRadian = it.beginAngle / 180 * PI;
				double sweepR = (it.endAngle - it.beginAngle)/* / 180 * PI*/;
				//mdlArc_create(&arcElem, NULL, &centerPt, dRoundRadius, dRoundRadius, NULL, strRadian, sweepR);
				MSElementDescrP arcElem = nullptr;
				CreateArc(&arcElem, centerPt, sweepR, it.beginAngle, it.endAngle, dRoundRadius / UOR_PER_MilliMeter);
				DPoint3d startDP, endDP, halfDP;
				mdlElmdscr_extractEndPoints(&startDP, NULL, &endDP, NULL, arcElem, ACTIVEMODEL);
				double Len;
				mdlElmdscr_distanceAtPoint(&Len, NULL, NULL, arcElem, &endDP, 1e-10);
				mdlElmdscr_pointAtDistance(&halfDP, NULL, Len / 2, arcElem, 1e-10);

				//EditElementHandle eehRound(&arcElem, modelRef);
				EditElementHandle eehRound(arcElem, false, false, modelRef);
				vector<MSElementDescrP> vecTmp;
				vector<DPoint3d> vecArcIntersect;
				// Բ�θֽ���׶������õ����θֽֻ��һ��ȱ�ڵĻ����� �ཻ�� vecArcIntersect
				//EFT::IntersectHoleEllWall(eehRound, centerPt, m_useHoleehs, vecTmp, vecArcIntersect);
				GetARCIntersectPointsWithHoles(vecArcIntersect, m_vecUseHoles[index], startDP, endDP, halfDP);
				makeBreakArcRebarCurve(rebarCurvesNum, eehRound.GetElementDescrP(), endTypes, vecArcIntersect);
				++index;
				//if (vecArcIntersect.size() == 0) // û�б��׶��и� -- Բ��
				//{
				//	makeArcRebarCurve(rebarCurvesNum, eehRound.GetElementDescrP(), endTypes, vecArcIntersect);							
				//}
				//else // ���׶��и�ɻ���
				//{
				//	// ��ÿһ�λ�
				//	makeArcRebarCurve(rebarCurvesNum, eehRound.GetElementDescrP(), endTypes, vecArcIntersect);
				//}
			}
		}
		if (isTwinRebar && i == numRebar - 2)
		{
			movePoint(CVector3D::kZaxis, centerPt, adjustedSpacing - diameter - m_reabrTwinData.diameter);
			continue;
		}

		movePoint(CVector3D::kZaxis, centerPt, adjustedSpacing);
	}

	numRebar = (int)rebarCurvesNum.size();

	vector<DSegment3d> vecStartEnd;
	for (int j = 0; j < (int)rebarCurvesNum.size(); ++j)
	{
		PITRebarCurve rebarCurve = rebarCurvesNum[j];
		RebarElementP rebarElement = nullptr;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (DataExchange == 0)
			{
				if (isTwinRebar)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (isTwinRebar)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (isTwinRebar)
					Stype = "Twinback";
				else
					Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

bool ELLWallRebarAssembly::makeArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP mscArc, PIT::PITRebarEndTypes& endTypes, const vector<DPoint3d>& pts)
{
	DPoint3d ArcDPs[2];
	DPoint3d centerpt;
	mdlArc_extract(ArcDPs, NULL, NULL, NULL, NULL, NULL, &centerpt, &mscArc->el);

	DPoint3d ptArcStart = ArcDPs[0];
	DPoint3d ptArcEnd = ArcDPs[1];

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	double dislenth;
	dislenth = 0;
	mdlElmdscr_distanceAtPoint(&dislenth, nullptr, nullptr, mscArc, &ptArcEnd, 0.1);
	for (DPoint3d pt : pts)
	{
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, mscArc, &pt, 0.1);
		if (dis1 > 10 && dis1 <= dislenth + 10)
		{
			int dis = (int)dis1;
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = ptArcStart;
	}
	else
	{
		map_pts[0] = ptArcStart;
	}
	int dis = (int)dislenth;
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = ptArcEnd;
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = ptArcEnd;
	}

	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{

		PITRebarCurve trebar;
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, mscArc, &itr->second, 0.1);
		DPoint3d tmpMid, tmpstr, tmpend;

		tmpstr = itr->second;
		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		tmpend = itrplus->second;
		double dis2;
		dis2 = 0;
		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, mscArc, &tmpend, 0.1);

		dis1 = dis1 + abs(dis2 - dis1) / 2;

		mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis1, mscArc, 0.1);

		if (ISPointInHoles(m_useHoleehs, tmpMid))
		{
			if (ISPointInHoles(m_useHoleehs, tmpstr) && ISPointInHoles(m_useHoleehs, tmpend))
			{
				itr--;
				continue;
			}
		}
		if (CalculateArc(trebar, tmpstr, tmpMid, tmpend))
		{
			ArcSegment arcSeg;
			arcSeg.ptStart = tmpstr;
			arcSeg.ptMid = tmpMid;
			arcSeg.ptEnd = tmpend;

			m_vecArcSeg.push_back(arcSeg);

			trebar.EvaluateEndTypesArc(endTypes);
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(tmpstr, tmpMid, tmpend), true, *ACTIVEMODEL);
			//arceeh1.AddToModel();
			rebar.push_back(trebar);
		}
	}
	return true;
}


bool ELLWallRebarAssembly::makeBreakArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP mscArc, PIT::PITRebarEndTypes& endTypes, const vector<DPoint3d>& pts)
{
	DPoint3d startDP, endDP;
	mdlElmdscr_extractEndPoints(&startDP, NULL, &endDP, NULL, mscArc, ACTIVEMODEL);
	double Len;
	mdlElmdscr_distanceAtPoint(&Len, NULL, NULL, mscArc, &endDP, 1e-10);

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt : pts)
	{
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, mscArc, &pt, 0.1);
		if (dis1 > 10 && dis1 <= Len + 10)
		{
			int dis = (int)dis1;
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = startDP;
	}
	else
	{
		map_pts[0] = startDP;
	}
	int dis = (int)Len;
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = endDP;
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = endDP;
	}

	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{

		PITRebarCurve trebar;
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, mscArc, &itr->second, 0.1);
		DPoint3d tmpMid, tmpstr, tmpend;

		tmpstr = itr->second;
		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		tmpend = itrplus->second;
		double dis2;
		dis2 = 0;
		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, mscArc, &tmpend, 0.1);

		dis1 = dis1 + abs(dis2 - dis1) / 2;

		mdlElmdscr_pointAtDistance(&tmpMid, nullptr, dis1, mscArc, 0.1);

		if (ISPointInHoles(m_useHoleehs, tmpMid))
		{
			if (ISPointInHoles(m_useHoleehs, tmpstr) && ISPointInHoles(m_useHoleehs, tmpend))
			{
				itr--;
				continue;
			}
		}
		if (CalculateArc(trebar, tmpstr, tmpMid, tmpend))
		{
			trebar.EvaluateEndTypesArc(endTypes);
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(tmpstr, tmpMid, tmpend), true, *ACTIVEMODEL);
			//arceeh1.AddToModel();
			rebar.push_back(trebar);
		}
	}
	return true;
}

bool ELLWallRebarAssembly::makeRebarCurve(vector<PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d ptStr, double dRebarLength)
{
	DPoint3d ptEnd = ptStr;
	movePoint(CVector3D::kZaxis, ptEnd, dRebarLength);

	DPoint3d pt1[2] = { ptStr, ptEnd };
	//---end
	DPoint3d tmpstr, tmpend;
	tmpstr = ptStr;
	tmpend = ptEnd;
	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetSideCover() * uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt : tmppts)
	{
		if (ExtractFacesTool::IsPointInLine(pt, pt1[0], pt1[1], ACTIVEMODEL, isStr))
		{
			int dis = (int)pt1[0].Distance(pt);
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = pt1[0];
	}
	else
	{
		map_pts[0] = pt1[0];
	}
	int dis = (int)pt1[0].Distance(pt1[1]);
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = pt1[1];
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = pt1[1];
	}

	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{
		map<int, DPoint3d>::iterator itrplus = itr;
		itrplus++;
		if (itrplus == map_pts.end())
		{
			break;
		}

		DPoint3d tmpMid = itr->second; tmpMid.Add(itrplus->second); tmpMid.Scale(0.5);
		if (ISPointInHoles(m_useHoleehs, tmpMid))
		{
			if (ISPointInHoles(m_useHoleehs, itr->second) && ISPointInHoles(m_useHoleehs, itrplus->second))
			{
				continue;
			}
		}

		PITRebarEndTypes		tmpendTypes;

		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itr->second);
		vex->SetType(RebarVertex::kStart);
		if (tmpstr.Distance(itr->second) < 10)
		{
			tmpendTypes.beg = endTypes.beg;
		}
		tmpendTypes.beg.SetptOrgin(itr->second);


		if (tmpend.Distance(itrplus->second) < 10)
		{
			tmpendTypes.end = endTypes.end;
		}

		tmpendTypes.end.SetptOrgin(itrplus->second);

		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itrplus->second);
		vex->SetType(RebarVertex::kEnd);

		rebar.EvaluateEndTypes(tmpendTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebarCurvesNum.push_back(rebar);
	}
	//rebar.DoMatrix(mat);
	return true;
}


// Բ��״�ֽ�
bool ELLWallRebarAssembly::makeRoundRebarCurve(vector<PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d	centerPoint, double dRoundRadius)
{
	DPoint3d arcStr;
	DPoint3d arcEnd;
	DPoint3d arcMid;
	CVector3D vec = CVector3D::kXaxis;
	arcStr = centerPoint;
	arcEnd = centerPoint;

	ArcSegment arcSeg;

	movePoint(vec, arcStr, dRoundRadius);
	vec = vec.Perpendicular();
	movePoint(vec, arcEnd, dRoundRadius);
	// ����һ�λ�
	EditElementHandle eehArc;
	//��ptAΪԲ��,dRadiusΪ�뾶��ptIn1Ϊ������㣬ptIn2Ϊ�����յ㻭��
	ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(centerPoint, arcStr, arcEnd), true, *ACTIVEMODEL);
	double dis2 = 0.00;

	mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, eehArc.GetElementDescrP(), &arcEnd, 0.1);

	dis2 /= 2;

	mdlElmdscr_pointAtDistance(&arcMid, nullptr, dis2, eehArc.GetElementDescrP(), 0.1);

	PITRebarCurve rebarCurve;
	if (CalculateRound(rebarCurve, arcStr, arcMid, arcEnd, 1))
	{
		arcSeg.ptCenter = centerPoint;
		arcSeg.ptStart = arcStr;
		arcSeg.dRadius = dRoundRadius;

		EditElementHandle arceeh1;
		ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(arcStr, arcMid, arcEnd), true, *ACTIVEMODEL);
	}

	// �������λ�
	for (int i = 0; i < 3; i++)
	{
		arcStr = arcEnd;
		arcEnd = centerPoint;

		vec = vec.Perpendicular();
		movePoint(vec, arcEnd, dRoundRadius);

		//��ptAΪԲ��,dRadiusΪ�뾶��ptIn1Ϊ������㣬ptIn2Ϊ�����յ㻭��
		ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(centerPoint, arcStr, arcEnd), true, *ACTIVEMODEL);

		dis2 = 0.00;

		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, eehArc.GetElementDescrP(), &arcEnd, 0.1);

		dis2 /= 2;

		mdlElmdscr_pointAtDistance(&arcMid, nullptr, dis2, eehArc.GetElementDescrP(), 0.1);
		int nStep = 0;
		if (i == 2)
		{
			nStep = 2;
		}
		if (CalculateRound(rebarCurve, arcStr, arcMid, arcEnd, nStep))
		{
			if (i == 0)
			{
				arcSeg.ptMid = arcEnd;
			}
			else if (i == 2)
			{
				arcSeg.ptEnd = arcEnd;
			}
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(arcStr, arcMid, arcEnd), true, *ACTIVEMODEL);
		}
	}
	m_vecArcSeg.push_back(arcSeg);
	rebarCurvesNum.push_back(rebarCurve);

	return true;
}


void ELLWallRebarAssembly::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}


vector<CurveVectorPtr> ELLWallRebarAssembly::CreateBreakArcRange(const Transform& tran)
{
	vector<CurveVectorPtr> curveVectors;
	for (auto it : m_vecBreakData)
	{
		//�ڻ�
		MSElementDescrP inArc = nullptr;
		CreateArc(&inArc, m_ELLWallData.centerpt, it.endAngle - it.beginAngle, it.beginAngle, it.endAngle, m_ELLWallData.dRadiusInn / UOR_PER_MilliMeter);
		EditElementHandle arcEehIn(inArc, true, false, ACTIVEMODEL);
		DPoint3d startDP, endDP;
		mdlElmdscr_extractEndPoints(&startDP, NULL, &endDP, NULL, arcEehIn.GetElementDescrP(), ACTIVEMODEL);
		//arcEehIn.AddToModel();
		//�⻡
		MSElementDescrP outArc = nullptr;
		CreateArc(&outArc, m_ELLWallData.centerpt, it.beginAngle - it.endAngle, it.endAngle, it.beginAngle, m_ELLWallData.dRadiusOut / UOR_PER_MilliMeter);
		EditElementHandle arcEehOut(outArc, true, false, ACTIVEMODEL);
		DPoint3d startDPOut, endDPOut;
		mdlElmdscr_extractEndPoints(&startDPOut, NULL, &endDPOut, NULL, arcEehOut.GetElementDescrP(), ACTIVEMODEL);
		//arcEehOut.AddToModel();

		//startDP.z = endDP.z = startDPOut.z = endDPOut.z = 0;
		//���ߵ���
		EditElementHandle lineEeh1;
		LineHandler::CreateLineElement(lineEeh1, nullptr, DSegment3d::From(endDPOut, startDP), true, *ACTIVEMODEL);
		//lineEeh1.AddToModel();
		EditElementHandle lineEeh2;
		LineHandler::CreateLineElement(lineEeh2, nullptr, DSegment3d::From(endDP, startDPOut), true, *ACTIVEMODEL);
		//lineEeh2.AddToModel();
		//���
		EditElementHandle shapeEeh;
		ChainHeaderHandler::CreateChainHeaderElement(shapeEeh, nullptr, true, true, *ACTIVEMODEL);
		ComplexShapeHandler::AddComponentElement(shapeEeh, arcEehIn);
		ComplexShapeHandler::AddComponentElement(shapeEeh, lineEeh2);
		ComplexShapeHandler::AddComponentElement(shapeEeh, arcEehOut);
		ComplexShapeHandler::AddComponentElement(shapeEeh, lineEeh1);
		MSElementDescrP pFace = shapeEeh.GetElementDescrP();
		mdlElmdscr_transform(&pFace, &tran);
		//mdlElmdscr_add(pFace);
		CurveVectorPtr curvePtr = ICurvePathQuery::ElementToCurveVector(shapeEeh);
		curveVectors.push_back(curvePtr);
	}
	return curveVectors;
}

bool ELLWallRebarAssembly::CalculateRound(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt, int nStep)
{
	bool ret = false;

	BeArcSeg arc(begPt, midPt, endPt);

	CPoint3D cen;
	arc.GetCenter(cen);

	if (arc.GetCenter(cen))
	{
		CPoint3D beg = begPt;
		CPoint3D med = midPt;
		CPoint3D end = endPt;

		CVector3D tan1 = arc.GetTangentVector(beg);
		CVector3D tan2 = arc.GetTangentVector(end);

		CPointVect pv1(beg, tan1);
		CPointVect pv2(end, tan2);

		CPoint3D ip;
		bool isIntersect = pv1.Intersect(ip, pv2);

		double radius = arc.GetRadius();

		RebarVertexP vex;
		if (nStep == 1) // Բ���׶λ�
		{
			vex = &(curve.PopVertices()).NewElement();
			vex->SetIP(beg);
			vex->SetType(RebarVertex::kStart);      // first IP
		}

		CPoint3D mid = (beg + end) / 2.0;
		CVector3D midVec(cen, mid);
		midVec.Normalize();

		if (isIntersect)
		{
			mid = cen + midVec * radius;

			// it can be on the other size
			CPoint3D mid1 = cen - midVec * radius;

			double d1 = med.Distance(mid1);
			double d2 = med.Distance(mid);

			if (COMPARE_VALUES_EPS(d1, d2, EPS) < 0)
			{
				mid = mid1;
				midVec = -midVec;
				// this is big arc we need 4 ips

				CVector3D midTan = midVec.Perpendicular();
				CPointVect pvm(mid, midTan);

				pv1.Intersect(ip, pvm);
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(ip);
				vex->SetType(RebarVertex::kIP);      // 2nd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				mid1 = (beg + mid) / 2.0;
				midVec = mid1 - cen;
				midVec.Normalize();
				mid1 = cen + midVec * radius;

				vex->SetArcPt(0, beg);
				vex->SetArcPt(1, mid1);
				vex->SetArcPt(2, mid);

				pv1.Intersect(ip, pvm);
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(beg);
				vex->SetType(RebarVertex::kIP);      // 3rd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				mid1 = (end + mid) / 2.0;
				midVec = mid1 - cen;
				midVec.Normalize();
				mid1 = cen + midVec * radius;

				vex->SetArcPt(0, mid);
				vex->SetArcPt(1, mid1);
				vex->SetArcPt(2, end);
			}
			else
			{
				// this is less than 90 or equal we need 3 ips
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(ip);
				vex->SetType(RebarVertex::kIP);      // 2nd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				vex->SetArcPt(0, beg);
				vex->SetArcPt(1, mid);
				vex->SetArcPt(2, end);
			}
		}
		else
		{
			// this is half circle - we need 4 ips
			midVec = arc.GetTangentVector(med);
			midVec.Normalize();
			DPoint3d ptMedTan = midVec;
			ptMedTan.Scale(radius);
			ptMedTan.Add(med);
			DPoint3d ptBegTan = tan1;
			ptBegTan.Scale(radius);
			ptBegTan.Add(beg);
			mdlVec_intersect(ip, &DSegment3d::From(beg, ptBegTan), &DSegment3d::From(med, ptMedTan));
			mid = cen + tan1 * radius;
			DEllipse3d circle = DEllipse3d::FromCenterRadiusXY(cen, radius);

			double angle_start = circle.PointToAngle(beg);
			double angle_mid = circle.PointToAngle(mid);

			double angle = (angle_start + angle_mid) / 2;
			CPoint3D mid1;
			circle.Evaluate(&mid1, 0, angle);

			vex = &curve.PopVertices().NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 2nd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, beg);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, mid);

			DPoint3d ptEndTan = tan2;
			ptEndTan.Scale(radius);
			ptEndTan.Add(end);
			mdlVec_intersect(ip, &DSegment3d::From(end, ptEndTan), &DSegment3d::From(med, ptMedTan));

			double angle_end = circle.PointToAngle(end);

			angle = (angle_end + angle_mid) / 2;

			circle.Evaluate(&mid1, 0, angle);

			vex = &curve.PopVertices().NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 3rd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, mid);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, end);
		}

		if (nStep == 2) // Բ�����һ�λ�
		{
			vex = &curve.PopVertices().NewElement();
			vex->SetIP(end);
			vex->SetType(RebarVertex::kEnd);      // last IP
		}
		else
		{
			//vex = &curve.PopVertices().NewElement();
			//vex->SetIP(end);
			//vex->SetType(RebarVertex::kIP);      // last IP
		}

		ret = true;
	}

	return ret;
}


bool ELLWallRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	CalculateUseHoles(modelRef);

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	int iRebarLevelNum = GetRebarLevelNum();
	vector<PIT::EndType> vecEndType;

	// ���иֽ���ֱ����
	double totalDiameter = 0.00;
	for (BrString sizeKey : GetvecDirSize())
	{
		totalDiameter += RebarCode::GetBarDiameter(sizeKey, modelRef);
	}
	// end

	vector<int> vecDataChange = GetvecDataExchange();
	vector<double> vecLevelSpace = GetvecLevelSpace();

	double levelSpacingTol = 0.0; // �����õĲ����

	// �����ƫ�ƾ���
	map<int, vector<LevelInfo>> mapLevelSapce;
	for (int i = 0; i < vecDataChange.size(); i++)
	{
		if (mapLevelSapce.find(vecDataChange.at(i)) != mapLevelSapce.end())
		{
			LevelInfo levelInfo;
			levelInfo.rebarLevel = i;
			levelInfo.LevelSpacing = vecLevelSpace.at(i) * uor_per_mm;
			mapLevelSapce[vecDataChange.at(i)].push_back(levelInfo);
		}
		else
		{
			LevelInfo levelInfo;
			levelInfo.rebarLevel = i;
			levelInfo.LevelSpacing = vecLevelSpace.at(i) * uor_per_mm;

			vector<LevelInfo> vecBack;
			vecBack.push_back(levelInfo);
			mapLevelSapce.insert(make_pair(vecDataChange.at(i), vecBack));
		}

		// ���������İ뾶��Ͳ��㣬ֻ�ܽ���΢��
		if (COMPARE_VALUES_EPS(vecLevelSpace.at(i) * uor_per_mm, m_ELLWallData.dRadiusOut - m_ELLWallData.dRadiusInn, EPS) < 0)
		{
			levelSpacingTol += vecLevelSpace.at(i) * uor_per_mm;
		}

	}
	// end

	// �������桢���桢�м� �Ĳ���
	double levelSpacing = m_ELLWallData.dRadiusOut - m_ELLWallData.dRadiusInn - GetPositiveCover() * uor_per_mm - GetReverseCover() * uor_per_mm;
	levelSpacing -= totalDiameter - levelSpacingTol;

	for (auto itr = mapLevelSapce.begin(); itr != mapLevelSapce.end(); itr++)
	{
		if (itr->first == 1) // �м�
		{
			if (itr->second.size() > 0)
			{
				itr->second.at(0).LevelSpacing += levelSpacing * 0.5;
				levelSpacing *= 0.5;
			}
		}
		else if (itr->first == 2) // ����
		{
			if (itr->second.size() > 0)
			{
				itr->second.at(0).LevelSpacing += levelSpacing;
			}
		}
	}
	// end

	m_vecArcSeg.clear();
	m_vecRebarStartEnd.clear();

	int iTwinbarSetIdIndex = 0;
	//levelSpacing = m_ELLWallData.dRadiusOut - GetPositiveCover() * uor_per_mm;
	levelSpacing = m_ELLWallData.dRadiusInn + GetPositiveCover() * uor_per_mm;
	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		BrString sizeKey = GetvecDirSize().at(i);
		double diamter = RebarCode::GetBarDiameter(sizeKey, modelRef);

		// ��¼�ֽ�����
		vector<LevelInfo> vecDis = mapLevelSapce[GetvecDataExchange().at(i)];
		for (LevelInfo info : vecDis)
		{
			if (info.rebarLevel == i)
			{
				//levelSpacing -= info.LevelSpacing;
				levelSpacing += info.LevelSpacing;
				break;
			}
		}

		//levelSpacing -= diamter * 0.5;
		levelSpacing += diamter * 0.5;

		PopvecSetId().push_back(0);
		if (GetvecDir().at(i) == 0)
		{
			RebarSetTag* tag = MakeRebar_Round(GetvecSetId().back(), GetvecDirSize().at(i), modelRef,
				GetvecStartOffset().at(i) * uor_per_mm, GetvecEndOffset().at(i) * uor_per_mm,
				GetvecDirSpacing().at(i) * uor_per_mm, levelSpacing, GetvecRebarLevel().at(i),
				GetvecRebarType().at(i), GetvecDataExchange().at(i));
			if (NULL != tag && (!PreviewButtonDown))
			{
				tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
				rsetTags.Add(tag);
			}

			//���Ʋ���--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
			{
				PopvecSetId().push_back(0);

				double diamterTwin = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(i).rebarSize, modelRef);
				//���Ʋ����
				tag = MakeRebar_Round(GetvecSetId().back(), GetvecTwinRebarLevel().at(i).rebarSize, modelRef,
					GetvecStartOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5,
					GetvecEndOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5,
					GetvecDirSpacing().at(i) * uor_per_mm, levelSpacing, GetvecRebarLevel().at(i),
					GetvecRebarType().at(i), GetvecDataExchange().at(i), true);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
		}
		else
		{
			double spacing = GetvecDirSpacing().at(i);
			if (g_wallRebarInfo.concrete.m_SlabRebarMethod = 2) //�������
			{
				spacing = levelSpacing / 10 * PI / 180 * GetvecAngle().at(i);
			}
			// ���
			RebarSetTag* tag = MakeRebar_Vertical(GetvecSetId().at(i), GetvecDirSize().at(i), modelRef,
				GetvecStartOffset().at(i) * uor_per_mm, GetvecEndOffset().at(i) * uor_per_mm,
				spacing* uor_per_mm, levelSpacing, m_ELLWallData.dHeight, GetvecRebarLevel().at(i),
				GetvecRebarType().at(i), GetvecDataExchange().at(i));
			if (NULL != tag && (!PreviewButtonDown))
			{
				tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
				rsetTags.Add(tag);
			}
			//���Ʋ���--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
			{
				PopvecSetId().push_back(0);

				double diamterTwin = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(i).rebarSize, modelRef);
				//���Ʋ����
				tag = MakeRebar_Vertical(GetvecSetId().back(), GetvecTwinRebarLevel().at(i).rebarSize, modelRef,
					GetvecStartOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5,
					GetvecEndOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5,
					spacing * uor_per_mm, levelSpacing, m_ELLWallData.dHeight,
					GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true);
				if (NULL != tag && (!PreviewButtonDown))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
		}

		levelSpacing += diamter * 0.5;
	}

	if (PreviewButtonDown)//Ԥ����ť���£���������
	{
		m_allLines.clear();
		for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)
		{
			vector<vector<DPoint3d>> faceLinePts = *it;
			for (auto it : faceLinePts)
			{
				vector<DPoint3d> linePts = it;
				EditElementHandle eeh;
				LineStringHandler::CreateLineStringElement(eeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allLines.push_back(eeh.GetElementRef());
			}
		}
		for (ArcSegment arcSeg : m_vecArcSeg)
		{
			if (arcSeg.ptStart.IsEqual(arcSeg.ptEnd, EPS)) // Բ��
			{
				EditElementHandle arceeh;
				if (SUCCESS == ArcHandler::CreateArcElement(arceeh, NULL, arcSeg.ptCenter, arcSeg.dRadius, arcSeg.dRadius, 0, 0, 2 * PI, true, *ACTIVEMODEL))
				{
					arceeh.AddToModel();
					m_allLines.push_back(arceeh.GetElementRef());
				}
			}
			else // ����
			{
				EditElementHandle arceeh;
				if (SUCCESS == ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(arcSeg.ptStart, arcSeg.ptMid, arcSeg.ptEnd), true, *ACTIVEMODEL))
				{
					arceeh.AddToModel();
					m_allLines.push_back(arceeh.GetElementRef());
				}
			}
		}
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

bool ELLWallRebarAssembly::CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
{
	bool ret = false;

	BeArcSeg arc(begPt, midPt, endPt);

	CMatrix3D mat;
	CPoint3D cen;
	arc.GetCenter(cen);

	if (arc.GetPlanarMatrix(mat) && arc.GetCenter(cen))
	{
		CPoint3D beg = begPt;
		CPoint3D med = midPt;
		CPoint3D end = endPt;

		beg.Transform(mat);
		med.Transform(mat);
		end.Transform(mat);
		cen.Transform(mat);
		arc.Transform(mat);

		CVector3D tan1 = arc.GetTangentVector(beg);
		CVector3D tan2 = arc.GetTangentVector(end);

		CPointVect pv1(beg, tan1);
		CPointVect pv2(end, tan2);

		CPoint3D ip;
		bool isIntersect = pv1.Intersect(ip, pv2);

		double radius = arc.GetRadius();

		RebarVertexP vex;
		vex = &(curve.PopVertices()).NewElement();
		vex->SetIP(beg);
		vex->SetType(RebarVertex::kStart);      // first IP

		CPoint3D mid = (beg + end) / 2.0;
		CVector3D midVec(cen, mid);
		midVec.Normalize();

		if (isIntersect)
		{
			mid = cen + midVec * radius;

			// it can be on the other size
			CPoint3D mid1 = cen - midVec * radius;

			double d1 = med.Distance(mid1);
			double d2 = med.Distance(mid);

			if (d1 < d2)
			{
				mid = mid1;
				midVec = -midVec;
				// this is big arc we need 4 ips

				CVector3D midTan = midVec.Perpendicular();
				CPointVect pvm(mid, midTan);

				pv1.Intersect(ip, pvm);
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(ip);
				vex->SetType(RebarVertex::kIP);      // 2nd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				mid1 = (beg + mid) / 2.0;
				midVec = mid1 - cen;
				midVec.Normalize();
				mid1 = cen + midVec * radius;

				vex->SetArcPt(0, beg);
				vex->SetArcPt(1, mid1);
				vex->SetArcPt(2, mid);



				pv1.Intersect(ip, pvm);
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(beg);
				vex->SetType(RebarVertex::kIP);      // 3rd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				mid1 = (end + mid) / 2.0;
				midVec = mid1 - cen;
				midVec.Normalize();
				mid1 = cen + midVec * radius;

				vex->SetArcPt(0, mid);
				vex->SetArcPt(1, mid1);
				vex->SetArcPt(2, end);

			}
			else
			{
				// this is less than 90 or equal we need 3 ips
				vex = &curve.PopVertices().NewElement();
				vex->SetIP(ip);
				vex->SetType(RebarVertex::kIP);      // 2nd IP
				vex->SetRadius(radius);
				vex->SetCenter(cen);

				vex->SetArcPt(0, beg);
				vex->SetArcPt(1, mid);
				vex->SetArcPt(2, end);
			}
		}
		else
		{
			// this is half circle - we need 4 ips
			midVec = arc.GetTangentVector(med);
			midVec.Normalize();
			DPoint3d ptMedTan = midVec;
			ptMedTan.Scale(radius);
			ptMedTan.Add(med);
			DPoint3d ptBegTan = tan1;
			ptBegTan.Scale(radius);
			ptBegTan.Add(beg);
			mdlVec_intersect(ip, &DSegment3d::From(beg, ptBegTan), &DSegment3d::From(med, ptMedTan));
			// 			EditElementHandle eeh;
			// 			LineHandler::CreateLineElement(eeh, NULL, DSegment3d::From(beg, ptBegTan), true, *ACTIVEMODEL);
			// 			eeh.AddToModel();
			// 			EditElementHandle eeh1;
			// 			LineHandler::CreateLineElement(eeh1, NULL, DSegment3d::From(med, ptMedTan), true, *ACTIVEMODEL);
			// 			eeh1.AddToModel();
			// 			CPointVect pvm(med, midVec);
			// 			pvm.Intersect(ip, tan1);
			// 			tan1 = ip - beg;
			// 			tan1.Normalize();
			// 
			// 			ip = beg + tan1 * radius;
			// 			midVec = ip - cen;
			// 			midVec.Normalize();
			//			midVec = ip - cen;
			mid = cen + tan1 * radius;
			DEllipse3d circle = DEllipse3d::FromCenterRadiusXY(cen, radius);

			double angle_start = circle.PointToAngle(beg);
			double angle_mid = circle.PointToAngle(mid);

			double angle = (angle_start + angle_mid) / 2;
			// 			if (angle < angle_mid)
			// 				angle += _PI;
			CPoint3D mid1;
			circle.Evaluate(&mid1, 0, angle);

			//			CPoint3D mid1 = cen + midVec;

			vex = &curve.PopVertices().NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 2nd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, beg);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, mid);

			// 			ip = end + tan1 * radius; // tan1 and tan2 parallel but tan1 has now the correct direction, do not change to tan2
			// 			midVec = ip - cen;
			// 			midVec.Normalize();
			// 			mid1 = cen + midVec * radius;

			//			midVec.Negate();
			DPoint3d ptEndTan = tan2;
			ptEndTan.Scale(radius);
			ptEndTan.Add(end);
			mdlVec_intersect(ip, &DSegment3d::From(end, ptEndTan), &DSegment3d::From(med, ptMedTan));
			// 			midVec = ip - cen;
			// 			mid1 = cen + midVec;

			// 			EditElementHandle eeh2;
			// 			LineHandler::CreateLineElement(eeh2, NULL, DSegment3d::From(end, ptEndTan), true, *ACTIVEMODEL);
			// 			eeh2.AddToModel();
			double angle_end = circle.PointToAngle(end);

			angle = (angle_end + angle_mid) / 2;
			// 			if (angle < angle_end)
			// 				angle += _PI;

			circle.Evaluate(&mid1, 0, angle);

			vex = &curve.PopVertices().NewElement();
			vex->SetIP(ip);
			vex->SetType(RebarVertex::kIP);      // 3rd IP
			vex->SetRadius(radius);
			vex->SetCenter(cen);

			vex->SetArcPt(0, mid);
			vex->SetArcPt(1, mid1);
			vex->SetArcPt(2, end);
		}

		vex = &curve.PopVertices().NewElement();
		vex->SetIP(end);
		vex->SetType(RebarVertex::kEnd);      // last IP

		mat = mat.Inverse();
		curve.DoMatrix(mat);              // transform back

		ret = true;
	}

	return ret;
}


bool ELLWallRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pEllWallDoubleRebarDlg = new CWallRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pEllWallDoubleRebarDlg->SetSelectElement(ehSel);
	pEllWallDoubleRebarDlg->Create(IDD_DIALOG_WallRebar);
	pEllWallDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pEllWallDoubleRebarDlg->ShowWindow(SW_SHOW);
	return true;
}

bool ELLWallRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();

	SetWallData(ehWall);
	MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();
	return true;
}

bool STGWallRebarAssembly::AnalyzingWallGeometricData(ElementHandleCR eh)
{
	DgnModelRefP model = eh.GetModelRef();
	double uor_per_mm = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	m_frontLinePts.clear();
	m_backLinePts.clear();
	m_doorsholes.clear();
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFrontLine;
	vector<MSElementDescrP> vecDownBackLine;
	EditElementHandle testeeh(eh, false);
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	GetDoorHoles(Holeehs, m_doorsholes);
	if (!GetFrontBackLines(Eleeh, vecDownFrontLine, vecDownBackLine))
	{
		return false;
	}

	auto GetPt = [](const MSElementDescrP& lineDecrP, LinePt& pt) {
		mdlElmdscr_extractEndPoints(&pt.startPt, nullptr, &pt.endPt, nullptr, lineDecrP, ACTIVEMODEL);
		if (COMPARE_VALUES_EPS(pt.startPt.x, pt.endPt.x, 1) == 1)
		{
			DPoint3d tmppt = pt.startPt;
			pt.startPt = pt.endPt;
			pt.endPt = tmppt;
		}
		else if (COMPARE_VALUES_EPS(pt.startPt.y, pt.endPt.y, 1) == 1)
		{
			DPoint3d tmppt = pt.startPt;
			pt.startPt = pt.endPt;
			pt.endPt = tmppt;
		}
	};
	for (auto it : vecDownFrontLine)
	{
		//mdlElmdscr_add(it);
		LinePt pt;
		GetPt(it, pt);
		m_frontLinePts.push_back(pt);
	}
	for (auto it : vecDownBackLine)
	{
		//mdlElmdscr_add(it);
		LinePt pt;
		GetPt(it, pt);
		m_backLinePts.push_back(pt);
	}

	bool isreverse = false;
	if (m_backLinePts.size() > m_frontLinePts.size())
	{
		vector<LinePt> tmpPts = m_backLinePts;
		m_backLinePts = m_frontLinePts;
		m_frontLinePts = tmpPts;
		isreverse = true;
	}
	map<int, vector<DPoint3d>> linePts;
	int index = 0;
	for (auto it : m_backLinePts)
	{
		for (auto frontIt : m_frontLinePts)
		{
			DPoint3d startPt;
			StatusInt startRet = mdlVec_projectPointToLine(&startPt, nullptr, &frontIt.startPt, &it.startPt, &it.endPt);
			bool flag = false;
			startRet = ExtractFacesTool::IsPointInLine(startPt, it.startPt, it.endPt, ACTIVEMODEL, flag);
			DPoint3d endPt;
			StatusInt endRet = mdlVec_projectPointToLine(&endPt, nullptr, &frontIt.endPt, &it.startPt, &it.endPt);
			endRet = ExtractFacesTool::IsPointInLine(endPt, it.startPt, it.endPt, ACTIVEMODEL, flag);
			//��ʼ�㶼����ͶӰ����������ʼ����յ���һ������ͶӰ����back����ʼ����յ���Ϊ�ֶε���ʼ����յ�
			DPoint3d frontStrPt = frontIt.startPt, frontEndPt = frontIt.endPt;
			if (!startRet && !endRet)
			{
				continue;
			}
			if (!endRet)
			{
				endPt = it.endPt;
				mdlVec_projectPointToLine(&frontEndPt, nullptr, &endPt, &frontIt.startPt, &frontIt.endPt);
			}
			if (!startRet)
			{
				startPt = it.startPt;
				mdlVec_projectPointToLine(&frontStrPt, nullptr, &startPt, &frontIt.startPt, &frontIt.endPt);
			}
			//ȥ��̫�̵��߶�
			double dis = startPt.Distance(endPt);
			if (COMPARE_VALUES_EPS(dis, 10, 0) == -1)
			{
				continue;
			}

			vector<DPoint3d> pts;
			if (isreverse)
			{
				pts.push_back(startPt);
				pts.push_back(endPt);
				pts.push_back(frontStrPt);
				pts.push_back(frontEndPt);
			}
			else
			{
				pts.push_back(frontStrPt);
				pts.push_back(frontEndPt);
				pts.push_back(startPt);
				pts.push_back(endPt);
			}
			linePts[index] = pts;
			index++;
		}
	}

	SetLinePts(linePts);
	m_width = 0;
	for (auto it : linePts)
	{
		double width = it.second.at(0).Distance(it.second.at(2));
		if (COMPARE_VALUES_EPS(m_width, width, 1) == -1)
		{
			m_width = width;
		}
	}
	CalcWallsInRange();
	m_Holeehs = Holeehs;
	return true;
}

void STGWallRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetSideCover()*uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

	if (g_wallRebarInfo.concrete.isHandleHole)//������Ҫ����Ŀ׶�
	{
		for (int j = 0; j < m_Holeehs.size(); j++)
		{
			EditElementHandle eeh;
			eeh.Duplicate(*m_Holeehs.at(j));
			bool isdoorNeg = false;//�ж��Ƿ�Ϊ�Ŷ�NEG
			isdoorNeg = IsDoorHoleNeg(m_Holeehs.at(j), m_doorsholes);
			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
			}
			if (isdoorNeg)
			{
				PlusSideCover(eeh, dSideCover, matrix, isdoorNeg, m_width);
			}
			//DPoint3d ptcenter = getCenterOfElmdescr(eeh.GetElementDescrP());
			//vector<DPoint3d> interpts;
			//DPoint3d tmpStr, tmpEnd;
			//tmpStr = m_STwallData.ptStart;
			//tmpEnd = m_STwallData.ptEnd;
			//tmpStr.z = tmpEnd.z = ptcenter.z;
			//GetIntersectPointsWithHole(interpts, &eeh, tmpStr, tmpEnd);
			//if (interpts.size() > 0)
			//{
			TransformInfo transinfo(trans);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			DPoint3d minP;
			DPoint3d maxP;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
			DRange3d range;
			range.low = minP;
			range.high = maxP;
			bool isNeed = false;
			if (range.XLength() > misssize || range.ZLength() > misssize)
			{
				isNeed = true;
			}

			if (isNeed)
			{
				if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//������Ŷ�
				{
					continue;
				}
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*m_Holeehs.at(j));
				if (isdoorNeg)
				{
					PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix, isdoorNeg, m_width);
				}
				else
				{
					PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
				}

				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
			//}

		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}

}

bool STGWallRebarAssembly::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;
	//InitUcsMatrix();
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}

bool STGWallRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	map<int, vector<DPoint3d>> linePts = GetLinePts();
	if (linePts.empty())
	{
		return false;
	}
	if (m_frontLinePts.empty() && m_backLinePts.empty())
		return false;

	InitLevelHoriInfos();

	EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	SetSelectedElement(Eleeh.GetElementId());
	if (g_globalpara.Getrebarstyle() != 0)
	{
		NewRebarAssembly(modelRef);
	}
	SetSelectedElement(testeeh.GetElementId());
	for (int j = 0; j < Holeehs.size(); j++)
	{
		if (Holeehs.at(j) != nullptr)
		{
			delete Holeehs.at(j);
			Holeehs.at(j) = nullptr;
		}
	}
	RebarSetTagArray rsetTags;
	m_useHoleehs.clear();
	CalculateUseHoles(modelRef);

	double uor_ref = GetSelectedModel()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	//for (auto it : m_useHoleehs)
	//{
	//	it->AddToModel();
	//}

	int numtag = 0;

	m_vecAllRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
	g_vecTieRebarPtsNoHole.clear();

	double thickness = 0;
	GetMaxThickness(modelRef, thickness);

	//�жϵ�ǰǽ������ǽ�Ĺ�ϵ
	auto UpdateWallPos = [](DVec3d& refVec, const DPoint3d& tmpStrPt, const DPoint3d& tmpEndPt)->WallPos {
		WallPos wallPos = Horizontal;
		if (COMPARE_VALUES_EPS(tmpStrPt.Distance(tmpEndPt), 10, 1) == -1)
		{
			return wallPos;
		}
		DVec3d tmpVec = tmpStrPt - tmpEndPt;
		refVec.Normalize();
		tmpVec.Normalize();
		if (tmpVec.DotProduct(refVec) > 0)
		{
			wallPos = IN_WALL;
		}
		else
		{
			wallPos = OUT_WALL;
		}

		return wallPos;
	};

	for (int k = 0; k < linePts.size(); k++)
	{

		if (linePts[k].size() != 4)
		{
			continue;
		}
		double  sidecover = GetSideCover();//ȡ�ò��汣����
		m_sidecover = GetSideCover();

		GetUcsAndStartEndData(k, thickness, modelRef, true);

		if (thickness >= MaxWallThickness * uor_per_mm)
		{
			//SetSideCover(0);//�Ƚ����汣��������Ϊ0			
		}
		CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
		double dLevelSpace = 0;
		double dSideCover = m_sidecover * uor_per_mm;
		if (COMPARE_VALUES(dSideCover, m_STwallData.width) >= 0)	//������汣������ڵ���ǽ�ĳ���
		{
			mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"���汣������ڵ���ǽ�ĳ���,�޷������ֽ��", MessageBoxIconType::Information);
			return false;
		}

		vector<CVector3D> vTrans;
		vector<CVector3D> vTransTb;
		//�����������ƫ����
		CalculateTransform(vTrans, vTransTb, modelRef);
		if (vTrans.size() != GetRebarLevelNum())
		{
			return false;
		}

		double dLength = m_STwallData.length;
		double dWidth = m_STwallData.height;

		int iRebarSetTag = 0;
		int iRebarLevelNum = GetRebarLevelNum();
		int iTwinbarSetIdIndex = 0;
		vector<PIT::EndType> vecEndType;
		for (int i = 0; i < iRebarLevelNum; ++i)
		{
			RebarSetTag* tag = NULL;
			CMatrix3D   mat, matTb;

			vector<PIT::EndType> vecEndType;
			if (GetvvecEndType().empty())		//û�����ö˲���ʽ������Ĭ��ֵ
			{
				EndType endType;
				memset(&endType, 0, sizeof(endType));
				vecEndType = { endType,endType };
			}
			else
			{
				vecEndType = GetvvecEndType().at(i);
			}

			CVector3D tmpVector(m_LineNormal);
			tmpVector.Scale(vTrans[i].y);
			CMatrix3D   tmpmat;
			tmpmat.SetTranslation(tmpVector);
			double  Misdisstr, Misdisend = 0;
			double tLenth;
			CalculateNowPlacementAndLenth(Misdisstr, Misdisend, tLenth, tmpmat, modelRef);

			vector<CVector3D> tmpEndNormal(2);
			WallPos leftWall = Horizontal, rightWall = Horizontal;
			double leftDis = 0, rightDis = 0; //����ǽ�뵱ǰǽ��ƫ�����
			if (GetvecDataExchange().at(i) == 0) //front
			{
				if (k != 0) //����ǽ
				{
					DPoint3d tmpStrPt = linePts.at(k - 1).at(1);
					DPoint3d tmpEndPt = linePts.at(k).at(0);
					leftDis = tmpStrPt.Distance(tmpEndPt);
					DVec3d refVec = linePts.at(k).at(2) - linePts.at(k).at(0);
					leftWall = UpdateWallPos(refVec, tmpStrPt, tmpEndPt);
					DVec3d tmpVec = tmpStrPt - tmpEndPt;
					tmpVec.Normalize();
					if (leftWall == IN_WALL)
					{
						tmpEndNormal.at(0) = tmpVec;
					}
				}
				if (k != linePts.size() - 1) //����ǽ
				{
					DPoint3d tmpStrPt = linePts.at(k + 1).at(0);
					DPoint3d tmpEndPt = linePts.at(k).at(1);
					rightDis = tmpStrPt.Distance(tmpEndPt);
					DVec3d refVec = linePts.at(k).at(3) - linePts.at(k).at(1);
					rightWall = UpdateWallPos(refVec, tmpStrPt, tmpEndPt);
					DVec3d tmpVec = tmpStrPt - tmpEndPt;
					tmpVec.Normalize();
					if (rightWall == IN_WALL)
					{
						tmpEndNormal.at(1) = tmpVec;
					}
				}
			}
			else //back
			{
				if (k != 0) //����ǽ
				{
					DPoint3d tmpStrPt = linePts.at(k - 1).at(3);
					DPoint3d tmpEndPt = linePts.at(k).at(2);
					leftDis = tmpStrPt.Distance(tmpEndPt);
					DVec3d refVec = linePts.at(k).at(0) - linePts.at(k).at(2);
					leftWall = UpdateWallPos(refVec, tmpStrPt, tmpEndPt);
					DVec3d tmpVec = tmpStrPt - tmpEndPt;
					tmpVec.Normalize();
					if (leftWall == IN_WALL)
					{
						tmpEndNormal.at(0) = tmpVec;
					}
				}
				if (k != linePts.size() - 1) //����ǽ
				{
					DPoint3d tmpStrPt = linePts.at(k + 1).at(2);
					DPoint3d tmpEndPt = linePts.at(k).at(3);
					rightDis = tmpStrPt.Distance(tmpEndPt);
					DVec3d refVec = linePts.at(k).at(1) - linePts.at(k).at(3);
					rightWall = UpdateWallPos(refVec, tmpStrPt, tmpEndPt);
					DVec3d tmpVec = tmpStrPt - tmpEndPt;
					tmpVec.Normalize();
					if (rightWall == IN_WALL)
					{
						tmpEndNormal.at(1) = tmpVec;
					}
				}
			}

			if (vTrans.size() != GetRebarLevelNum())
			{
				return false;
			}
			double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;
			m_vecRebarPtsLayer.clear();
			m_vecTwinRebarPtsLayer.clear();
			m_vecTieRebarPtsLayer.clear();
			if (GetvecDir().at(i) == 1)	//����ֽ�
			{
				bool drawlast = true;
				//if (i <= 1 && thickness >= MaxWallThickness * uor_per_mm&&k != linePts.size() - 1)//������600�������ǵ�һ�λ������Ҳ������һ��ǽ�������һ��������
				//{
				//	drawlast = false;
				//}

				double misDisH_left, misDisH_right;

				if (COMPARE_VALUES_EPS(m_angle_left, PI / 2, 0.001))
				{
					misDisH_left = (1 / sin(m_angle_left) - 1)*diameter + Misdisstr;
				}
				else
				{
					misDisH_left = 0;
				}
				if (COMPARE_VALUES_EPS(m_angle_right, PI / 2, 0.001))
				{
					misDisH_right = (1 / sin(m_angle_right) - 1)*diameter + Misdisend;
				}
				else
				{
					misDisH_right = 0;
				}

				vector<CVector3D> vecEndNormal(2);
				CVector3D	endNormal;	//�˲��乳����
				if (GetvvecEndType().size() > 0)
				{
					for (size_t k = 0; k < vecEndNormal.size(); ++k)
					{
						double dRotateAngle = vecEndType.at(k).rotateAngle;
						endNormal = m_STwallData.ptEnd - m_STwallData.ptStart;
						endNormal.Normalize();
						if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
						{
							endNormal.Negate();
						}
						CVector3D rebarVec = CVector3D::kZaxis;
						/*					endNormal = rebarVec.CrossProduct(vec);*/
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
						vecEndNormal[k] = endNormal;
					}
				}
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;
				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
				//���Ʋ���--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
				{
					SetIsPushTieRebar(false);
					//�Ȼ��Ʒǲ����
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dLength, dWidth,
						GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat,
						GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i),
						GetvecDataExchange().at(i), false, modelRef, drawlast, false);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);
					}

					SetIsPushTieRebar(true);
					//���Ʋ����
					tag = MakeRebars(PopvecSetId().at(iTwinbarSetIdIndex + iRebarLevelNum), GetvecDirSize().at(i),
						dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm,
						GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal,
						matTb, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i),
						GetvecDataExchange().at(i), true, modelRef, drawlast, false);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1 + numtag);
						rsetTags.Add(tag);
					}
					iTwinbarSetIdIndex++;
				}
				else //��ǰ��δ���ò���
				{
					SetIsPushTieRebar(true);
					TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dLength, dWidth,
						GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat,
						twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i),
						false, modelRef, drawlast, false);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);
					}
				}
				vecEndType.clear();
			}
			else
			{
				double misDisV_left, misDisV_right;
				if (COMPARE_VALUES_EPS(m_angle_left, PI / 2, 0.001))
				{
					misDisV_left = diameter / tan(m_angle_left);
				}
				else
				{
					misDisV_left = 0;
				}
				if (COMPARE_VALUES_EPS(m_angle_right, PI / 2, 0.001))
				{
					misDisV_right = diameter / tan(m_angle_right);
				}
				else
				{
					misDisV_right = 0;
				}
				double leftSideCov = m_sidecover * uor_per_mm / sin(m_angle_left);
				double rightSideCov = m_sidecover * uor_per_mm / sin(m_angle_right);
				double allSideCov = leftSideCov + rightSideCov;

				tLenth = tLenth - (misDisV_left + misDisV_right);
				vTrans[i].x = (tLenth - allSideCov) / 2 + Misdisstr + leftSideCov;
				vector<CVector3D> vecEndNormal(2);
				CVector3D	endNormal;	//�˲��乳����
				if (GetvvecEndType().size() > 0)
				{
					for (size_t k = 0; k < vecEndNormal.size(); ++k)
					{
						double dRotateAngle = vecEndType.at(k).rotateAngle;
						CVector3D rebarVec = m_STwallData.ptEnd - m_STwallData.ptStart;
						endNormal = CVector3D::From(0, 0, -1);
						if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
						{
							endNormal.Negate();
						}
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
						vecEndNormal[k] = endNormal;
					}
				}
				if (leftWall != Horizontal)
				{
					vecEndNormal.at(0) = tmpEndNormal.at(0);
				}
				if (rightWall != Horizontal)
				{
					vecEndNormal.at(1) = tmpEndNormal.at(1);
				}
				//if (leftWall == IN_WALL)
				//{
				//	vecEndType[0].endType = 4;
				//}
				//if (rightWall == IN_WALL)
				//{
				//	vecEndType[1].endType = 4;
				//}
				mat = rot90;
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;
				matTb = rot90;
				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
				//������Ϊ�����,ż����Ϊ��ͨ��

				//���Ʋ���--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
				{
					SetIsPushTieRebar(true);
					//�Ȼ��Ʒǲ����
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dWidth, tLenth,
						GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat,
						GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i),
						GetvecDataExchange().at(i), false, modelRef, true, true, leftWall, rightWall,
						leftDis, rightDis);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);

						CalcLevelHoriInfos(i, tag, rightWall, m_levelHoriTags);
					}

					SetIsPushTieRebar(false);
					//���Ʋ����
					tag = MakeRebars(PopvecSetId().at(iTwinbarSetIdIndex + iRebarLevelNum), GetvecDirSize().at(i),
						dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, matTb,
						GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i),
						GetvecDataExchange().at(i), true, modelRef, true, true, leftWall, rightWall,
						leftDis, rightDis);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1 + numtag);
						rsetTags.Add(tag);
						CalcLevelHoriInfos(i, tag, rightWall, m_twinLevelHoriTags);
					}
					iTwinbarSetIdIndex++;

				}
				else //��ǰ��δ���ò���
				{
					SetIsPushTieRebar(true);
					TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dWidth, tLenth,
						GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, twinRebar,
						GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i),
						false, modelRef, true, true, leftWall, rightWall, leftDis, rightDis);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);
						CalcLevelHoriInfos(i, tag, rightWall, m_levelHoriTags);
					}
				}
				//end
				vecEndType.clear();
				if (rightWall == Horizontal)
				{
					m_levelIsHori[i] = true;
				}
				else
				{
					m_levelIsHori[i] = false;
				}
			}
			if (m_vecRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = k;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecRebarPtsLayer.at(m);
					rbPt.ptend = m_vecRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecRebarPtsNoHole.push_back(rbPt);
					vector<int> walls = m_rangeIdxWalls[k];
					for (auto wallId : walls)
					{
						g_wallRebarPtsNoHole[wallId].push_back(rbPt);
					}
					m++;
				}
			}
			if (m_vecTwinRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecTwinRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = k;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTwinRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTwinRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTwinRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
			if (m_vecTieRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecTieRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTieRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTieRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTieRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
			SetSideCover(sidecover);
		}



		numtag = (int)rsetTags.GetSize();
	}

	UpdateHoriRebars(m_levelHoriTags, TEXT_MAIN_REBAR);
	UpdateHoriRebars(m_twinLevelHoriTags, TEXT_TWIN_REBAR);

	if (PreviewButtonDown)//Ԥ����ť���£���������
	{
		m_allLines.clear();
		for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)
		{
			vector<vector<DPoint3d>> faceLinePts = *it;
			for (auto it : faceLinePts)
			{
				vector<DPoint3d> linePts = it;
				EditElementHandle eeh;
				LineStringHandler::CreateLineStringElement(eeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allLines.push_back(eeh.GetElementRef());
			}
		}
		return true;
	}

	bool ret = true;
	if (g_globalpara.Getrebarstyle() != 0)
	{
		ret = AddRebarSets(rsetTags);
	}
	return ret;
}

RebarSetTag* STGWallRebarAssembly::MakeRebars(ElementId& rebarSetId, BrStringCR sizeKey,
	double xLen, double height, double spacing, double startOffset, double endOffset,
	vector<PIT::EndType> const& endType, /*�洢���˲����յ�˲����� */
	vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat,
	TwinBarSet::TwinBarLevelInfo const& twinBarInfo, int level, int grade,
	int DataExchange, bool bTwinbarLevel, DgnModelRefP modelRef,
	bool drawlast /*= true */,
	bool  isHoriRebar /*= true*/,
	WallPos leftWall/* = Horizontal*/,
	WallPos rightWall/* = Horizontal*/,
	double leftDis /*= 0*/,
	double rightDis /*= 0*/)
{
	rebarSetId = 0;
	//	m_IsTwinrebar = bTwinbarLevel;
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	if (endType.size() != vecEndNormal.size() || endType.size() == 0)
	{
		return NULL;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, startbendLenTb, endbendLen, endbendLenTb;
	double begStraightAnchorLen, endStraightAnchorLen;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double LaLenth = g_globalpara.m_alength[(string)sizeKey] * uor_per_mm;
	double L0Lenth = g_globalpara.m_laplenth[(string)sizeKey] * uor_per_mm;
	switch (endType[0].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeStart.SetType(RebarEndType::kLap);
		begStraightAnchorLen = endType[0].endPtInfo.value1;	//ê�볤��
		break;
	case 4:	//90���乳
	{
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = 12 * diameter;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef) * uor_per_mm;	//������100
		}
		startbendLenTb = startbendLen;
	}
	break;
	case 5:	//135���乳
	{
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		}
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}
	break;
	case 6:	//180���乳
	{
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		}
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}
	break;
	case 8:	//�û�
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (endType[1].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeEnd.SetType(RebarEndType::kLap);
		endStraightAnchorLen = endType[1].endPtInfo.value1;	//ê�볤��
		break;
	case 4:	//90���乳
	{
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = 12 * diameter;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef) * uor_per_mm;	//������100
		}
		endbendLenTb = endbendLen;
	}
	break;
	case 5:	//135���乳
	{
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = rightDis;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		}
		endbendLenTb = endbendLen;
	}
	break;
	case 6:	//180���乳
	{
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = endType[1].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		}
		endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}

	break;
	case 8:	//�û�
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);	//������30
	double adjustedXLen, adjustedSpacing;

	double leftSideCov = GetSideCover()*uor_per_mm / sin(m_angle_left);
	double rightSideCov = GetSideCover() *uor_per_mm / sin(m_angle_right);
	double allSideCov = leftSideCov + rightSideCov;
	int numRebar = 0;
	if (twinBarInfo.hasTwinbars)	//����
		adjustedXLen = xLen - allSideCov - diameter - diameterTb - startOffset - endOffset;
	else
		adjustedXLen = xLen - allSideCov - diameter - startOffset - endOffset;
	if (bTwinbarLevel)				//�����ֽ�����
	{
		numRebar = (int)floor(adjustedXLen / (spacing * (twinBarInfo.interval + 1)) + 0.5) + 1;
		int numRebar1 = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar1 > 1)
		{
			adjustedSpacing = adjustedXLen / (numRebar1 - 1);	//�ǲ����ƽ�����
			adjustedSpacing *= (twinBarInfo.interval + 1);		//�����ʵ�ʼ������Ըֽ���
		}
	}
	else
	{
		numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar > 1)
			adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	double xPos = startOffset;
	if (bTwinbarLevel)				//�������ƫ��һ�ξ���
	{
		xPos += diameter * 0.5;
		if (diameterTb > diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radiusTb - radius)*(radiusTb - radius), 0.5) - radius;
			xPos += dOffset;
		}
		else if (diameterTb < diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radius - radiusTb)*(radius - radiusTb), 0.5) - radius;
			xPos += dOffset;
		}
		else
		{
			xPos += diameter * 0.5;
		}
	}
	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		start.SetbendLen(startbendLenTb);
		start.SetbendRadius(startbendRadius);
	}
	else
	{
		start.SetbendLen(startbendLen);
		start.SetbendRadius(startbendRadius);
	}
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		end.SetbendLen(endbendLenTb);
		end.SetbendRadius(bendRadiusTb);
	}
	else
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };
	RebarEndType tmpType;
	tmpType.SetType(RebarEndType::kBend);
	//double bendLen = RebarCode::GetBendLength(sizeKey, tmpType, modelRef);

	for (int i = 0; i < numRebar; i++)
	{
		if (!drawlast&&i == numRebar - 1)
		{
			continue;
		}
		vector<PITRebarCurve>     rebarCurves;
		//		RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
		makeRebarCurve(rebarCurves, xPos, height - allSideCov, endTypeStartOffset, endTypEendOffset,
			endTypes, mat, GWallRebarAssembly::GetIsTwinrebar(), leftWall, rightWall, leftDis, rightDis,
			L0Lenth, diameter);

		xPos += adjustedSpacing;

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinBarInfo.rebarSize, symb);
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}
	else
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	vector<vector<DPoint3d>> vecStartEnd;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (ISPointInHoles(m_Holeehs, midPos))
		{
			if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
			{
				continue;
			}
		}

		vector<DPoint3d> linePts;
		RebarVertices vertices = rebarCurve.GetVertices();
		for (size_t i = 0; i < (size_t)vertices.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &vertices.At(i);
			linePts.push_back(tmpVertex->GetIP());
		}
		/*	EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			eeh.AddToModel();*/

		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinBarInfo.rebarSize));
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameterTb, endTypes, shape, modelRef, false);
			}
			else
			{
				shape.SetSizeKey((LPCTSTR)sizeKey);
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
			}
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (DataExchange == 0)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinback";
				else
					Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
		j++;
		vecStartEnd.push_back(linePts);
	}
	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

bool STGWallRebarAssembly::makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, double xPos, double yLen,
	double startOffset, double endOffset, PIT::PITRebarEndTypes& endTypes, CMatrix3D const& mat,
	bool isTwin /*= false */, WallPos leftWalls/* = Horizontal*/, WallPos rightWall/* = Horizontal*/,
	double leftDis /*= 0*/, double rightDis /*= 0*/, double bendLen/* = 0*/, double  rebarDia/* = 0*/)
{
	CPoint3D  startPt;
	CPoint3D  endPt;

	//������Ϊ��ֵ
// 	if (startPt < 0)
// 		startPt = 0;
// 	if (endOffset < 0)
// 		endOffset = 0;

	startPt = CPoint3D::From(xPos, 0.0, -yLen / 2.0 + startOffset);
	endPt = CPoint3D::From(xPos, 0.0, yLen / 2.0 - endOffset);

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	//eeh.AddToModel();

	DPoint3d pt1[2];
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//��ʱʹ�õ�ǰ����MODEL�������������޸�
	DPoint3d lineStrPt = pt1[0], lineEndPt = pt1[1];

	//ȷ������յ��Ǵ�С����---begin
	DVec3d vec = pt1[1] - pt1[0];
	DVec3d vecX = DVec3d::From(1, 0, 0);
	vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
	vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
	vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
	vec.Normalize();
	if (vec.IsPerpendicularTo(vecX))
	{
		if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
		if (leftWalls == OUT_WALL)
		{
			pt1[0].y -= bendLen + leftDis;
		}
		if (rightWall == OUT_WALL)
		{
			pt1[1].y += (bendLen + rightDis);
		}
	}
	else
	{
		if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
		if (leftWalls == OUT_WALL)
		{
			pt1[0].x -= bendLen + leftDis;
		}
		if (rightWall == OUT_WALL)
		{
			pt1[1].x += bendLen + rightDis;
		}
	}
	//---end

	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}
	m_vecRebarPtsLayer.push_back(pt1[0]);
	m_vecRebarPtsLayer.push_back(pt1[1]);
	if (isTwin)
	{
		m_vecTwinRebarPtsLayer.push_back(pt1[0]);
		m_vecTwinRebarPtsLayer.push_back(pt1[1]);
	}

	//if (m_isPushTieRebar)
	//{
	//	m_vecTieRebarPtsLayer.push_back(pt1[0]);
	//	m_vecTieRebarPtsLayer.push_back(pt1[1]);
	//}
	map<int, DPoint3d> map_pts = CalcRebarPts(pt1[0], pt1[1]);

	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{
		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		DPoint3d startPt = itr->second;
		vex->SetIP(startPt);
		vex->SetType(RebarVertex::kStart);
		endTypes.beg.SetptOrgin(startPt);

		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		DPoint3d endPt = itrplus->second;
		endTypes.end.SetptOrgin(endPt);

		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(endPt);
		vex->SetType(RebarVertex::kEnd);

		PIT::PITRebarEndTypes tmpEndTypes = endTypes;
		if (!(startPt == lineStrPt) && leftWalls == IN_WALL)
		{
			tmpEndTypes.beg.SetbendLen(0);
			tmpEndTypes.beg.SetType(PITRebarEndType::kNone);
			tmpEndTypes.beg.SetbendRadius(0);
		}
		if (!(endPt == lineEndPt) && rightWall == IN_WALL)
		{
			tmpEndTypes.end.SetbendLen(0);
			tmpEndTypes.end.SetType(PITRebarEndType::kNone);
			tmpEndTypes.end.SetbendRadius(0);
		}
		rebar.EvaluateEndTypes(tmpEndTypes);
		rebars.push_back(rebar);
	}
	//rebar.DoMatrix(mat);
	return true;
}

void STGWallRebarAssembly::InitLevelHoriInfos()
{
	int levelNum = GetRebarLevelNum();
	m_levelIsHori.clear();
	for (int i = 0; i < levelNum; ++i)
	{
		m_levelIsHori[i] = false;
	}
	m_levelHoriTags.clear();
	m_twinLevelHoriTags.clear();
}

void STGWallRebarAssembly::CalcLevelHoriInfos(int level, RebarSetTag* tag, WallPos rightWall,
	map<int, vector<vector<RebarSetTag*>>>& levelHoriTags)
{
	vector<vector<RebarSetTag*>> levelTags = levelHoriTags[level];;
	if (!m_levelIsHori[level])
	{
		vector<RebarSetTag*> tags;
		tags.push_back(tag);
		levelTags.push_back(tags);
	}
	else
	{
		size_t num = levelTags.size() - 1;
		levelTags[num].push_back(tag);
	}
	levelHoriTags[level] = levelTags;
}

void STGWallRebarAssembly::UpdateHoriRebars(const map<int, vector<vector<RebarSetTag*>>>& levelHoriTags,
	const CString& levelName)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	for (auto it : levelHoriTags)
	{
		vector<vector<RebarSetTag*>> levelTags = it.second;
		for (auto itr : levelTags)
		{
			if (itr.size() < 2)
			{
				continue;
			}
			map<int, vector<ElementRefP>> zRebars;
			for (auto tagIt : itr)
			{
				RebarSetP rebarSet = tagIt->GetRset();
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
					DPoint3d center = getCenterOfElmdescr(rebarEle.GetElementDescrP());
					int posz = (int)(center.z / (uor_per_mm * 10));
					zRebars[posz].push_back(rebarEle.GetElementRef());
				}
			}
			UpdateRebars(zRebars, levelName);
		}

	}

}

void STGWallRebarAssembly::UpdateRebars(const map<int, vector<ElementRefP>>& zRebars, const CString& levelName)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	auto GetBendPt = [&](EditElementHandleP rebarEeh, DPoint3d& bendStrPt, DPoint3d& bendEndPt, DPoint3d& bendPt) {
		RebarElementP rep = RebarElement::Fetch(*rebarEeh);
		RebarShape * rebarshape = rep->GetRebarShape(rebarEeh->GetModelRef());
		RebarCurve strcurve;//��ȡ�ֽ�ģ���е�������״
		rebarshape->GetRebarCurve(strcurve);
		CMatrix3D tmp3d(rep->GetLocation());
		strcurve.MatchRebarCurve(tmp3d, strcurve, uor_per_mm);
		strcurve.DoMatrix(rep->GetLocation());
		RebarVertices  vers;
		RebarVertices oldVers = strcurve.GetVertices();
		for (size_t i = 0; i < (size_t)oldVers.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &oldVers.At(i);
			if (tmpVertex->GetType() == RebarVertex::kStart)
			{
				bendStrPt = tmpVertex->GetIP();
			}
			if (tmpVertex->GetType() == RebarVertex::kEnd)
			{
				bendEndPt = tmpVertex->GetIP();
			}
			if (tmpVertex->GetType() == RebarVertex::kIP)
			{
				bendPt = tmpVertex->GetIP();
			}
		}
	};

	//if (levelName == TEXT_MAIN_REBAR)
	//{
	//	m_vecRebarStartEnd.clear();
	//}

	for (auto it : zRebars)
	{
		vector<ElementRefP> rebars = it.second;
		size_t num = rebars.size();
		EditElementHandle strRebarEeh(rebars[0], rebars[0]->GetDgnModelP());
		EditElementHandle endRebarEeh(rebars[num - 1], rebars[num - 1]->GetDgnModelP());
		if (RebarElement::IsRebarElement(strRebarEeh) && RebarElement::IsRebarElement(endRebarEeh))
		{
			//��β�ֽ��Ƿ����乳�������乳ʱ��λ��
			bool strIsBend = false, endIsBend = false;
			DPoint3d bendStrPt = { 0,0,0 }, bendEndPt = { 0,0,0 }; //��β��������µ���ʼ����յ�
			RebarElementP strRep = RebarElement::Fetch(strRebarEeh);
			DPoint3d firstBendStrPt = { 0,0,0 }, firstBendEndPt = { 0,0,0 }, firstBendPt = { 0,0,0 }; //�׸��ֽ����ʼ�㣬��ֹ���������
			GetBendPt(&strRebarEeh, firstBendStrPt, firstBendEndPt, firstBendPt);
			DPoint3d strPt = firstBendStrPt; //�߽�����
			RebarShape * strShape = strRep->GetRebarShape(strRebarEeh.GetModelRef());
			if ((!firstBendPt.IsEqual(DPoint3d::FromZero())) &&
				(strShape->GetEndType(0).GetType() != RebarEndType::kNone
					|| strShape->GetEndType(1).GetType() != RebarEndType::kNone))
			{
				strIsBend = true;
				strPt = firstBendPt;
				bendStrPt = firstBendStrPt;
			}
			RebarElementP endRep = RebarElement::Fetch(endRebarEeh);
			DPoint3d endBendStrPt = { 0,0,0 }, endBendEndPt = { 0,0,0 }, endBendPt = { 0,0,0 }; //ĩβ���ֽ����ʼ�㣬��ֹ���������
			GetBendPt(&endRebarEeh, endBendStrPt, endBendEndPt, endBendPt);
			DPoint3d endPt = endBendEndPt; //�߽���յ�
			RebarShape * endShape = endRep->GetRebarShape(endRebarEeh.GetModelRef());
			if ((!endBendPt.IsEqual(DPoint3d::FromZero())) &&
				(endShape->GetEndType(0).GetType() != RebarEndType::kNone ||
					endShape->GetEndType(1).GetType() != RebarEndType::kNone))
			{
				endIsBend = true;
				endPt = endBendPt;
				bendEndPt = endBendEndPt;
			}

			//�߽�Ķ˵�
			DVec3d vec = strPt - endPt;
			if (vec.IsPerpendicularTo(DVec3d::UnitX())) //��x����ֱ
			{
				if (COMPARE_VALUES_EPS(strPt.y, endPt.y, 10) == 1)
				{
					DPoint3d tmpPt = strPt;
					strPt = endPt;
					endPt = tmpPt;

					tmpPt = bendStrPt;
					bendStrPt = bendEndPt;
					bendEndPt = tmpPt;
				}
			}
			else
			{
				if (COMPARE_VALUES_EPS(strPt.x, endPt.x, 10) == 1)
				{
					DPoint3d tmpPt = strPt;
					strPt = endPt;
					endPt = tmpPt;

					tmpPt = bendStrPt;
					bendStrPt = bendEndPt;
					bendEndPt = tmpPt;
				}
			}

			//��ܿ׶������		
			map<int, DPoint3d> map_pts = CalcRebarPts(strPt, endPt); //��ܺ�ĵ�

			//vector<vector<DPoint3d>> vecStartEnd;
			int index = 0;
			auto endIt = map_pts.end(); --endIt;
			for (auto itr = map_pts.begin(); itr != map_pts.end(); itr++)
			{
				if (index >= rebars.size())
				{
					break;
				}
				//��Ҫ�޸ĵĸֽ��Ϊ�ϲ���ĸֽ�����<=�ϲ�ǰ�ģ����޸ĵķ�ʽ�����øֽ����Բ���
				EditElementHandle rebarEeh(rebars[index], ACTIVEMODEL);
				RebarElementP rep = RebarElement::Fetch(rebarEeh);
				RebarShape * rebarshape = rep->GetRebarShape(rebarEeh.GetModelRef());
				if (rebarshape == nullptr)
				{
					return;
				}
				BrString sizeKey(rebarshape->GetShapeData().GetSizeKey());
				double diameter = RebarCode::GetBarDiameter(sizeKey, ACTIVEMODEL);

				RebarEndType endType1, endType2;
				endType1.SetType(RebarEndType::kNone);
				endType2.SetType(RebarEndType::kNone);
				bvector<DPoint3d> allpts;
				if (strIsBend && itr == map_pts.begin())
				{
					allpts.push_back(bendStrPt);
					endType1.SetType(RebarEndType::kBend);
				}
				allpts.push_back(itr->second);
				auto nextItr = ++itr;
				allpts.push_back(nextItr->second);
				if (endIsBend && itr == endIt)
				{
					allpts.push_back(bendEndPt);
					endType2.SetType(RebarEndType::kBend);
				}
				RebarVertices  vers;
				// GetRebarVerticesFromPoints(vers, allpts, diameter);
				RebarHelper::GetRebarVerticesFromPoints(vers, allpts, diameter);

				//��ȡ�ֽ�ģ���е�������״
				RebarCurve curve;
				rebarshape->GetRebarCurve(curve);
				curve.SetVertices(vers);

				RebarEndTypes endTypes = { endType1, endType2 };

				//���õ�����ɫ��ͼ��
				RebarSymbology symb;
				string str(sizeKey);
				char ccolar[20] = { 0 };
				strcpy(ccolar, str.c_str());
				SetRebarColorBySize(ccolar, symb);
				symb.SetRebarLevel(levelName);//�����ǵ��������Ϊ����ͼ��
				rep->SetRebarElementSymbology(symb);

				RebarShapeData shape = rebarshape->GetShapeData();
				rep->Update(curve, diameter, endTypes, shape, rep->GetModelRef(), false);
				RebarModel *rmv = RMV;
				if (rmv != nullptr)
				{
					rmv->SaveRebar(*rep, rep->GetModelRef(), true);
				}

				//if (levelName == TEXT_MAIN_REBAR)
				//{
				//	
				//	vector<DPoint3d> linePts;
				//	RebarVertices vertices = curve.GetVertices();
				//	for (size_t i = 0; i < vertices.GetSize(); ++i)
				//	{
				//		RebarVertex *tmpVertex = &vertices.At(i);
				//		linePts.push_back(tmpVertex->GetIP());
				//	}
				//	vecStartEnd.push_back(linePts);
				//}
				index++;
			}
			//ɾ������ֽ�
			for (size_t i = index; i < rebars.size(); ++i)
			{
				ElementHandle rebarEeh(rebars[i], rebars[i]->GetDgnModelP());
				RebarElementP rep = RebarElement::Fetch(rebarEeh);
				if (rep == nullptr)
				{
					return;
				}
				RebarModel *rmv = RMV;
				if (rmv != nullptr)
				{
					rmv->Delete(*rep, ACTIVEMODEL);
				}
			}
		}
	}
}

map<int, DPoint3d> STGWallRebarAssembly::CalcRebarPts(DPoint3d& strPt, DPoint3d& endPt)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = m_sidecover * uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	if (g_wallRebarInfo.concrete.isHandleHole)
	{
		GetIntersectPointsWithHoles(tmppts, m_useHoleehs, strPt, endPt, dSideCover, matrix);
	}

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt : tmppts)
	{
		if (ExtractFacesTool::IsPointInLine(pt, strPt, endPt, ACTIVEMODEL, isStr))
		{
			int dis = (int)strPt.Distance(pt);
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = strPt;
	}
	else
	{
		map_pts[0] = strPt;
	}
	int dis = (int)strPt.Distance(endPt);
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = endPt;
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = endPt;
	}
	return map_pts;
}

bool __IsSmartSmartFeature__(EditElementHandle& eeh)
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
void STGWallRebarAssembly::CalcWallsInRange()
{
	g_wallRebarPtsNoHole.clear();
	ElementAgenda selectedElement;
	SelectionSetManager::GetManager().BuildAgenda(selectedElement);
	if (selectedElement.GetCount() < 2)
	{
		return;
	}
	vector<EditElementHandle*> allWalls;
	map<ElementId, EditElementHandle*> tmpselcets;
	for (EditElementHandleR eeh : selectedElement)
	{
		if (eeh.IsValid())
		{
			ElementId tmpid = eeh.GetElementId();
			if (tmpid != 0)
			{
				tmpselcets[tmpid] = &eeh;
			}
		}
	}
	for (map<ElementId, EditElementHandle*>::iterator itr = tmpselcets.begin(); itr != tmpselcets.end(); itr++)
	{
		if (itr->second == nullptr)
		{
			continue;
		}
		// if (HoleRFRebarAssembly::IsSmartSmartFeature(*itr->second))
		if (__IsSmartSmartFeature__(*itr->second))
		{
			allWalls.push_back(itr->second);
		}
	}

	map<int, vector<DPoint3d>> linePts = GetLinePts();
	if (allWalls.size() == 0 || linePts.size() == 0)
	{
		return;
	}
	map<int, MSElementDescrP> wallDownfaces; //ǽ����
	for (auto it : allWalls)
	{
		if (!it->IsValid())
		{
			continue;
		}
		int id = (int)(it->GetElementId());
		EditElementHandle downFace;
		double height = 0;
		ExtractFacesTool::GetDownFace(*it, downFace, &height);
		wallDownfaces[id] = downFace.ExtractElementDescr();
	}
	for (auto it : linePts)
	{
		vector<DPoint3d> pts = it.second;
		if (pts.size() != 4)
		{
			continue;
		}
		//��С��Χ
		DVec3d vec1 = pts[0] - pts[1];
		DVec3d vec2 = pts[2] - pts[3];
		vec1.Normalize(); vec1.Scale(10);
		pts[1].Add(vec1);
		vec1.Negate();
		pts[0].Add(vec1);
		vec2.Normalize(); vec2.Scale(10);
		pts[3].Add(vec2);
		vec2.Negate();
		pts[2].Add(vec2);
		DPoint3d shapePts[4] = { pts[0], pts[1], pts[2], pts[3] };
		EditElementHandle shapeEeh;
		ShapeHandler::CreateShapeElement(shapeEeh, nullptr, shapePts, 4, true, *ACTIVEMODEL);
		MSElementDescrP shapeDescr = shapeEeh.GetElementDescrP();
		for (auto wallIt : wallDownfaces)
		{
			DPoint3d interPt;
			int num = mdlIntersect_allBetweenElms(&interPt, nullptr, 1, wallIt.second, shapeDescr, nullptr, 1);
			if (num > 0)
			{
				m_rangeIdxWalls[it.first].push_back(wallIt.first);
			}
		}
		mdlElmdscr_freeAll(&shapeDescr);
	}

}


bool STGWallRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pSTGWallDoubleRebarDlg = new CWallRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pSTGWallDoubleRebarDlg->SetSelectElement(ehSel);
	pSTGWallDoubleRebarDlg->Create(IDD_DIALOG_WallRebar);
	pSTGWallDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pSTGWallDoubleRebarDlg->ShowWindow(SW_SHOW);
	return true;
}

bool STGWallRebarAssembly::GetFrontBackLines(EditElementHandleCR eeh, vector<MSElementDescrP>& frontLines, vector<MSElementDescrP>& backLines)
{
	vector<MSElementDescrP> vecDownFaceLine;
	double height;
	ExtractFacesTool::GetFrontBackLineAndDownFace(eeh, NULL, vecDownFaceLine, frontLines, backLines, &height);
	SetGWallHeight(height);
	if (vecDownFaceLine.empty() || frontLines.empty() || backLines.empty())
		return false;

	auto CalculateLines = [](vector<MSElementDescrP>& downLines) {
		int index = 0;
		for (auto it = downLines.begin() + 1; it != downLines.end();)
		{
			auto lastIt = it; lastIt--;
			DPoint3d lastStartPt, lastEndPt;
			mdlElmdscr_extractEndPoints(&lastStartPt, nullptr, &lastEndPt, nullptr, *lastIt, ACTIVEMODEL);
			DVec3d lastVec = lastEndPt - lastStartPt;
			lastVec.Normalize();
			DPoint3d startPt, endPt;
			mdlElmdscr_extractEndPoints(&startPt, nullptr, &endPt, nullptr, *it, ACTIVEMODEL);
			DVec3d curVec = endPt - startPt;
			curVec.Normalize();
			if (fabs(fabs(lastVec.DotProduct(curVec)) - 1) < 1e-6) //ƽ��
			{
				DPoint3d intersectPt;
				if (mdlIntersect_allBetweenElms(&intersectPt, nullptr, 1, *lastIt, *it, nullptr, 1) > 0)
				{
					downLines.erase(lastIt);
					DSegment3d seg = DSegment3d::From(lastStartPt, endPt);
					EditElementHandle eeh;
					LineHandler::CreateLineElement(eeh, nullptr, seg, true, *ACTIVEMODEL);
					downLines[index] = eeh.GetElementDescrP();
					continue;
				}
			}
			index++;
			it++;
		}
	};

	//CalculateLines(frontLines);
	//CalculateLines(backLines);

	auto FilterLines = [](vector<MSElementDescrP>& lines) {
		DPoint3d firstStrPt, firstEndPt;
		mdlElmdscr_extractEndPoints(&firstStrPt, nullptr, &firstEndPt, nullptr, *lines.begin(), ACTIVEMODEL);
		DVec3d firstVec = firstEndPt - firstStrPt;
		firstVec.Normalize();
		for (auto it = lines.begin(); it != lines.end();)
		{
			DPoint3d strPt, endPt;
			mdlElmdscr_extractEndPoints(&strPt, nullptr, &endPt, nullptr, *it, ACTIVEMODEL);
			DVec3d vec = endPt - strPt;
			vec.Normalize();
			if (fabs(fabs(vec.DotProduct(firstVec)) - 1) < 1e-6) //ƽ��
			{
				it++;
			}
			else
			{
				lines.erase(it);
			}
		}
	};

	FilterLines(frontLines);
	FilterLines(backLines);
	return true;
}

long STGWallRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}

/*
* @desc:		������ΧԪ�غ͸ֽ��ߵ�λ�ü���ֽ��Ƿ�Ϸ�
* @param[in]	nowVec  �ж�����ֱ������ˮƽ����
* @param[in]	alleehs ��ΧԪ��
* @param[in]	tmpendEndTypes �˲���ʽ
* @param[in]	lineEeh �ֽ���
* @param[in]    Eleeh  ����֮ǰ��ʵ�壨ǽ���壩
* @param[in]	direction  ê��Ƕȣ�����
* @param[in]	matrix  ͶӰ����
* @param[in]	MoveDis ���������
* @param[in]	lenth   ê�볤��
* @param[in]	Point   �˵�λ��
* @param[in]	Point2   �޸Ķ˵�λ��
* @param[in]	FLAGE   �Ƿ���Ҫ�˲���ʽ�����FLAGE=1��ʾ����Ҫ�˲���ʽ�����FLAGE=2��ʾβ�˵�λ���޸�ΪPoint2λ��
* @param[in/out]	data ���������
* @author	ChenDong
* @Date:	2024/10/17
*/
void STWallRebarAssembly::JudgeBarLinesLegitimate(CVector3D  nowVec,vector<EditElementHandle*>alleehs, PIT::PITRebarEndTypes&  tmpendEndTypes,
	EditElementHandle &lineEeh,EditElementHandle* Eleeh, CVector3D direction,
	Transform matrix, double MoveDis, double lenth, DPoint3d &Point, DPoint3d &Point2, int &FLAGE)
	{
	bool is_str = false;//�Ƿ�Ϊ��ʼ��
	DPoint3d using_pt;
	DPoint3d mid_end_pt;//�ֽ�˲��е��ж�
	vector<DPoint3d> tmppts;//ʵ�彻��
	DPoint3d str_Pt = { 0,0,0 }, end_Pt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&str_Pt, nullptr, &end_Pt, nullptr, lineEeh.GetElementDescrP(), ACTIVEMODEL);
	double dSideCover = 0;

	using_pt = Point;
	movePoint(direction, using_pt, lenth);

	//�ֽ��߶˵�͸ֽ�˵�λ���Ƿ�ê��
	if (!ISPointInHoles(alleehs, Point) || !ISPointInHoles(alleehs, using_pt ))
	{
		if (FLAGE == 0)//˵������ʼ��
			is_str = true;
		int flag = 0;
		//�˲���ʽ��ֱ��ê������бê��
		if (direction.x == floor(direction.x) &&  direction.y == floor(direction.y) && direction.z == floor(direction.z))
		{
			direction.Negate();
			flag = 1;//�˲��仡Ϊֱ��
		}
		else
		{
			direction.x = direction.x;
			direction.y = -direction.y;
			direction.z = direction.z;
		}
		using_pt = Point;
		mid_end_pt = Point;
		//����ȡ������һ��λ��
		STWallRebarAssembly::movePoint(direction, using_pt, lenth);
		STWallRebarAssembly::movePoint(direction, mid_end_pt, lenth/2);
		//����ֽ���ĩ�˵�û��ê���κ�ʵ��    
		if (!ISPointInHoles(alleehs, Point))
		{
			FLAGE = 2;
			//PITCommonTool::CPointTool::DrowOnePoint(Point, 1, 3);//��
			mdlElmdscr_extractEndPoints(&str_Pt, nullptr, &end_Pt, nullptr, lineEeh.GetElementDescrP(), ACTIVEMODEL);
			//ִ�иֽ���ԭʵ������
			GetIntersectPointsWithOldElm(tmppts, Eleeh, str_Pt, end_Pt, dSideCover, matrix);
			//��ԭʵ���н���
			if (tmppts.size() > 1)
			{
				//����һ��������ľ���
				CVector3D vectortepm = end_Pt - using_pt;
				vectortepm.Normalize();
				vectortepm.ScaleToLength(MoveDis);
				using_pt.Add(vectortepm);
				lenth = MoveDis + tmpendEndTypes.end.GetbendRadius();

				if (is_str)//˵������ʼ��
				Point2 = tmppts[tmppts.size() - 1];
				else
				Point2 = tmppts[tmppts.size() - 2];

				if (COMPARE_VALUES_EPS(abs(nowVec.z), 1, EPS) == 0)//��ֱ������
				{
					if (is_str)
						Point2.z = Point2.z + lenth;
					else
						Point2.z = Point2.z - lenth;
				}

				else if (COMPARE_VALUES_EPS(abs(nowVec.x), 1, 0.1) == 0 || COMPARE_VALUES_EPS(abs(nowVec.y), 1, 0.1) == 0)//ˮƽ������
				{
					if (is_str)
						Point2.x = Point2.x + lenth;
					else
						Point2.x = Point2.x - lenth;
				}
				if (!ISPointInHoles(alleehs, Point2))//�ж��Ƿ���ʵ���ڣ���������
				{
					direction.Negate();
				}
			}
		}
		//�ֽ�ĩ�˵���Ȼû��ê���κ�ʵ�壬��ʵ���ڽض�
		else if (!ISPointInHoles(alleehs, using_pt) || !ISPointInHoles(alleehs, mid_end_pt))
		{
			if (flag == 1)//�˲��仡Ϊֱ��
			{
				FLAGE = 2;
				using_pt = Point;
				DPoint3d end_pt = Point;
				direction.Negate();//����ԭ
				
				Point2 = Point;
				auto temp = Point;
				movePoint(direction, using_pt, lenth);
				GetIntersectPointsWithHoles(tmppts, alleehs, using_pt, temp, dSideCover, matrix);
				GetIntersectPointsWithHolesByInsert(tmppts, alleehs, using_pt, Point, dSideCover, matrix);

				//�����ԭģ���н���
				if (tmppts.size() > 0)
				{
					CVector3D vectortepm = end_pt - using_pt;
					vectortepm.Normalize();
					vectortepm.ScaleToLength(MoveDis);
					using_pt.Add(vectortepm);

					lenth = MoveDis + tmpendEndTypes.end.GetbendRadius();
					using_pt = Point;
					movePoint(direction, using_pt, lenth);
					//PITCommonTool::CPointTool::DrowOnePoint(using_pt, 1, 1);//��
					if (!ISPointInHoles(alleehs, using_pt))
					{
						direction.Negate();//����ԭ

						lenth -= tmpendEndTypes.end.GetbendRadius();
					}

					if (is_str)//˵������ʼ��
						tmpendEndTypes.beg.SetbendLen(lenth);
					else
					    tmpendEndTypes.end.SetbendLen(lenth);
				}
			}
			else//�˲��仡��Ϊֱ���п��ܻ������ģ��
			{
				FLAGE = 3;
			}
		}

		if (is_str)//˵������ʼ��
			tmpendEndTypes.beg.SetendNormal(direction);
		else//˵����ĩ�˵�
		    tmpendEndTypes.end.SetendNormal(direction);

	}
}