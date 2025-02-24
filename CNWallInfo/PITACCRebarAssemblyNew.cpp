#include "_ustation.h"
#include "../resource.h"
#include <SelectionRebar.h>
#include "../GalleryIntelligentRebar.h"
#include "PITACCRebarAssemblyNew.h"
#include "CWallRebarDlgNew.h"
#include "ExtractFacesTool.h"
#include "../TieRebar.h"
#include "ElementAttribute.h"
#include "ACCRebarMakerNew.h"
#include "PITMSCECommon.h"
#include "../XmlManager.h"

using namespace PIT;

extern bool PreviewButtonDown;//预览按钮标志

void ACCRebarAssemblyNew::Init()
{
	m_vecDir.resize(m_RebarLevelNum);
	m_vecDirSpacing.resize(m_RebarLevelNum);
	m_vecRebarType.resize(m_RebarLevelNum);
	m_vecStartOffset.resize(m_RebarLevelNum);
	m_vecEndOffset.resize(m_RebarLevelNum);
	m_vecLevelSpace.resize(m_RebarLevelNum);
	m_vecRebarLevel.resize(m_RebarLevelNum);

//	int twinRebarLevel = 0;

	//根据需求并筋需要设置不一样的层
// 	for (size_t i = 0; i < GetvecTwinRebarLevel().size(); i++)
// 	{
// 		if (GetvecTwinRebarLevel().at(i).hasTwinbars)
// 		{
// 			twinRebarLevel++;
// 		}
// 	}
// 	m_vecSetId.resize(m_RebarLevelNum + twinRebarLevel);
// 	for (size_t i = 0; i < m_vecSetId.size(); i++)
// 	{
// 		m_vecSetId[i] = 0;
// 	}
}

void ACCRebarAssemblyNew::SetConcreteData(Concrete const& concreteData)
{
	m_PositiveCover = concreteData.postiveCover;
	m_ReverseCover = concreteData.reverseCover;
	m_SideCover = concreteData.sideCover;
	m_RebarLevelNum = concreteData.rebarLevelNum;
}


void ACCRebarAssemblyNew::SetRebarEndTypes(vector<PIT::EndType> const & vecEndTypes)
{
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

// void ACCRebarAssemblyNew::InitRebarSetId()
// {
// 	int twinRebarLevel = 0;
// 	//根据需求并筋需要设置不一样的层
// 	for (size_t i = 0; i < m_vecTwinRebarLevel.size(); i++)
// 	{
// 		if (m_vecTwinRebarLevel[i].hasTwinbars)
// 		{
// 			twinRebarLevel++;
// 		}
// 	}
// 
// 	if (m_vecSetId.size() != m_RebarLevelNum + twinRebarLevel)
// 	{
// 		m_vecSetId.resize(m_RebarLevelNum + twinRebarLevel);
// 		for (size_t i = 0; i < m_vecSetId.size(); ++i)
// 			m_vecSetId[i] = 0;
// 	}
// }

void ACCRebarAssemblyNew::GetConcreteData(Concrete& concreteData)
{
	concreteData.postiveCover = m_PositiveCover;
	concreteData.reverseCover = m_ReverseCover;
	concreteData.sideCover = m_SideCover;
	concreteData.rebarLevelNum = m_RebarLevelNum;
}

void ACCRebarAssemblyNew::SetTieRebarInfo(TieReBarInfo const & tieRebarInfo)
{
	m_tieRebarInfo = tieRebarInfo;
}

const TieReBarInfo ACCRebarAssemblyNew::GetTieRebarInfo() const
{
	return m_tieRebarInfo;
}

ACCRebarAssemblyNew::ComponentType ACCRebarAssemblyNew::JudgeWallType(ElementHandleCR eh)
{
	return STWALL;
	//通过分解元素来判断元素的类型
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
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine, vecDownBackLine, NULL);

	if (vecDownFaceLine.empty())
		return Other;
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

	for (size_t i = 0; i < vecDownFaceLine.size(); i++)
	{
		if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
		{
			return GWALL;
		}
	}
	return STWALL;
}

bool ACCRebarAssemblyNew::IsSmartSmartFeature(EditElementHandle& eeh)
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
//				eeh.AddToModel();
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}
		}
		return true;
	}
}


void ACCRebarAssemblyNew::SetRebarData(vector<PIT::ConcreteRebar> const& vecData)
{
	//加了正面、中间、反面钢筋概念后，修改钢筋层距离
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
		BrString sizekey = XmlManager::s_alltypes[data.rebarType];
		double diameter = RebarCode::GetBarDiameter(sizekey, ACTIVEMODEL) / uor_per_mm;
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
		BrString sizekey = XmlManager::s_alltypes[data.rebarType];
		double diameter = RebarCode::GetBarDiameter(sizekey, ACTIVEMODEL) / uor_per_mm;
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
		BrString sizekey = XmlManager::s_alltypes[data.rebarType];
		double diameter = RebarCode::GetBarDiameter(sizekey, ACTIVEMODEL) / uor_per_mm;
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
			m_vecRebarType[i] = tmpvecdata[i].rebarType;
			m_vecDirSpacing[i] = tmpvecdata[i].spacing;
			m_vecStartOffset[i] = tmpvecdata[i].startOffset;
			m_vecEndOffset[i] = tmpvecdata[i].endOffset;
			m_vecLevelSpace[i] = tmpvecdata[i].levelSpace;
			m_vecDataExchange[i] = tmpvecdata[i].datachange;
			m_vecRebarLevel[i] = tmpvecdata[i].rebarLevel;

		}
		else
		{
			m_vecDir.push_back(tmpvecdata[i].rebarDir);
			m_vecRebarType.push_back(tmpvecdata[i].rebarType);
			m_vecDirSpacing.push_back(tmpvecdata[i].spacing);
			m_vecStartOffset.push_back(tmpvecdata[i].startOffset);
			m_vecEndOffset.push_back(tmpvecdata[i].endOffset);
			m_vecLevelSpace.push_back(tmpvecdata[i].levelSpace);
			m_vecDataExchange.push_back(tmpvecdata[i].datachange);
			m_vecRebarLevel.push_back(tmpvecdata[i].rebarLevel);
		}
	}
}


void ACCRebarAssemblyNew::ClearLines()
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


void ACCRebarAssemblyNew::GetRebarData(vector<PIT::ConcreteRebar>& vecData) const
{
	if (0 == GetRebarLevelNum() || GetvecDir().empty()  || GetvecDirSpacing().empty())
	{
		return;
	}
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		ConcreteRebar rebarData;
		// 		rebarData.postiveCover = m_PositiveCover;
		// 		rebarData.reverseCover = m_ReverseCover;
		// 		rebarData.reverseCover = m_SideCover;
		rebarData.rebarDir = GetvecDir()[i];
		rebarData.rebarType = GetvecRebarType()[i];
		rebarData.spacing = GetvecDirSpacing()[i];
		rebarData.startOffset = GetvecStartOffset()[i];
		rebarData.endOffset = GetvecEndOffset()[i];
		rebarData.levelSpace = GetvecLevelSpace()[i];
		vecData.push_back(rebarData);
	}
}

ACCWallRebarAssemblyNew::ACCWallRebarAssemblyNew(ElementId id, DgnModelRefP modelRef) :ACCRebarAssemblyNew(id, modelRef)
{

}

bool ACCWallRebarAssemblyNew::IsWallSolid(ElementHandleCR eh)
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
	if (strName.find(L"VB") != WString::npos)	//墙
	{
		return true;
	}
	return false;
#else
	return true;
#endif // PDMSIMPORT
}

bool ACCWallRebarAssemblyNew::AnalyzingWallGeometricDataAndHoles(ElementHandleCR eh, STWallGeometryInfo &data)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	DPoint3d FrontStr, FrontEnd;
	DPoint3d BackStr, BackEnd;
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
	EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &data.height);
	GetDoorHoles(Holeehs, m_doorsholes);

	if (vecDownFontLine.empty() || vecDownBackLine.empty())
	{
		return false;
	}

	DPoint3d pt1[2];
	vecDownFontLine[0].GetStartPoint(pt1[0]);
	vecDownFontLine[0].GetEndPoint(pt1[1]);

	DPoint3d pt2[2];
	vecDownBackLine[0].GetStartPoint(pt2[0]);
	vecDownBackLine[0].GetEndPoint(pt2[1]);


	if (vecDownBackLine.size() > 1 || vecDownFontLine.size() > 1)
	{
		GetMaxDownFacePts(vecDownFontLine, vecDownBackLine, pt1, pt2);
	}

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, data.height);

	data.height = data.height*uor_now / uor_ref;
	data.width = FrontStr.Distance(BackStr)*uor_now / uor_ref;
	data.length = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;
	data.ptStart = FrontStr;
	data.ptEnd = FrontEnd;

	PopvecFrontPts().push_back(FrontStr);
	PopvecFrontPts().push_back(FrontEnd);

	FrontStr.Add(FrontEnd);
	FrontStr.Scale(0.5);
	BackStr.Add(BackEnd);
	BackStr.Scale(0.5);
	m_LineNormal = BackStr - FrontStr;
	m_LineNormal.Normalize();

	

	m_Negs = Negs;

	if (Negs.size() > 0)//STWALL有斜边
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
	m_width = data.width;
	return true;
}

bool ACCWallRebarAssemblyNew::AnalyzingWallGeometricData(ElementHandleCR eh, STWallGeometryInfo &data,bool bCut)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	DPoint3d FrontStr, FrontEnd;
	DPoint3d BackStr, BackEnd;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
 	vector<DSegment3d> vecDownFontLine;
 	vector<DSegment3d> vecDownBackLine;
	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	std::for_each(Holeehs.begin(), Holeehs.end(), [](EditElementHandle* &eh) {delete eh; eh = NULL; });
	DPoint3d pt1[2];
	DPoint3d pt2[2];

	
	if (bCut)
	{
		//两个点都在原始直线上
		bool bNearStart = false;
		vector<MSElementDescrP> vec_line;
		vector<MSElementDescrP> vec_linefront;
		vector<MSElementDescrP> vec_lineback;
		EFT::GetFrontBackLineAndDownFace(Eleeh, NULL, vec_line, vec_linefront, vec_lineback, NULL);

		for (MSElementDescrP ms : vec_line)
		{
			DPoint3d pt[2];
			mdlLinear_extract(pt, NULL, &ms->el, eh.GetModelRef());

			if (ExtractFacesTool::IsPointInLine(pt[0], GetsegOrg().point[0], GetsegOrg().point[1], ACTIVEMODEL, bNearStart)
				&& ExtractFacesTool::IsPointInLine(pt[1], GetsegOrg().point[0], GetsegOrg().point[1], ACTIVEMODEL, bNearStart))
			{
				pt1[0] = pt[0];
				pt1[1] = pt[1];
			}
			else if(CVector3D(pt[0],pt[1]).IsParallelTo(CVector3D(GetsegOrg().point[0], GetsegOrg().point[1])))
			{
				pt2[0] = pt[0];
				pt2[1] = pt[1];
			}
			mdlElmdscr_freeAll(&ms);
		}

		data.height = GetheightOrg();
	}
	else
	{
		EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &data.height);
		EditElementHandle ehh;
		LineHandler::CreateLineElement(ehh, NULL, vecDownFontLine[0], true, *ACTIVEMODEL);
		ehh.AddToModel();
		if (vecDownFontLine.empty() || vecDownBackLine.empty())
		{
			return false;
		}

		vecDownFontLine[0].GetStartPoint(pt1[0]);
		vecDownFontLine[0].GetEndPoint(pt1[1]);

		vecDownBackLine[0].GetStartPoint(pt2[0]);
		vecDownBackLine[0].GetEndPoint(pt2[1]);
	}

	if (vecDownBackLine.size() > 1 || vecDownFontLine.size() > 1)
	{
		GetMaxDownFacePts(vecDownFontLine, vecDownBackLine, pt1, pt2);
	}

// 	EditElementHandle eeh1;
// 	LineHandler::CreateLineElement(eeh1, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
// 	eeh1.AddToModel();
// 	EditElementHandle eeh2;
// 	LineHandler::CreateLineElement(eeh2, nullptr, DSegment3d::From(pt2[0], pt2[1]), true, *ACTIVEMODEL);
// 	eeh2.AddToModel();

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, data.height);
	std::for_each(Negs.begin(), Negs.end(), [](EditElementHandle* &eh) {delete eh; eh = NULL; });
	data.height = data.height*uor_now / uor_ref;
	data.width = FrontStr.Distance(BackStr)*uor_now / uor_ref;
	data.length = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;
	data.ptStart = FrontStr;
	data.ptEnd = FrontEnd;
	m_width = data.width;

	return true;
}

bool ACCSTWallRebarAssemblyNew::makeRebarCurve
(
	vector<PITRebarCurve>&     rebars,
	double                  xPos,
	double                  yLen,
	double					startOffset,
	double					endOffset,
	PITRebarEndTypes const&		endTypes,
	CMatrix3D const&        mat,
	bool isTwin
)
{

	CPoint3D  startPt;
	CPoint3D  endPt;

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
	mdlElmdscr_extractEndPoints(&pt1[0], nullptr, &pt1[1], nullptr, eeh.GetElementDescrP(), eeh.GetModelRef());
//	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改

	//确保起点终点是从小到大---begin
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
	vecStartEnd.push_back(DSegment3d::From(tmpstr, tmpend));
	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}
	if (!isTwin)
	{
		m_vecRebarPtsLayer.push_back(pt1[0]);
		m_vecRebarPtsLayer.push_back(pt1[1]);
	}
	

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetSideCover()*uor_per_mm;
	vector<DPoint3d> tmppts;
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover);

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

//	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
//	{
//		PITRebarEndTypes endTypesTmp = endTypes;
//		DPoint3d ptStart(itr->second);
//		PITRebarCurve rebar;
//		RebarVertexP vex;
//		vex = &rebar.PopVertices().NewElement();
//		vex->SetIP(ptStart);
//		vex->SetType(RebarVertex::kStart);
//		endTypesTmp.beg.SetptOrgin(itr->second);
//
//
//		map<int, DPoint3d>::iterator itrplus = ++itr;
//
//		if (itrplus == map_pts.end())
//		{
//			break;
//		}
//
//		endTypesTmp.end.SetptOrgin(itrplus->second);
//
//		DPoint3d ptEnd(itrplus->second);
//		vex = &rebar.PopVertices().NewElement();
//		vex->SetIP(ptEnd);
//		vex->SetType(RebarVertex::kEnd);
//
//		double dis1 = ptStart.Distance(pt1[0]);
//		if (COMPARE_VALUES_EPS(dis1,0, 1) != 0)
//		{
//			endTypesTmp.beg.SetType(PITRebarEndType::kNone);
//		}
//		double dis2 = ptEnd.Distance(pt1[1]);
//
//		if (COMPARE_VALUES_EPS(dis2, 0, 1) != 0)
//		{
//			endTypesTmp.end.SetType(PITRebarEndType::kNone);
//		}
//		rebar.EvaluateEndTypes(endTypesTmp);
//		rebars.push_back(rebar);
//
//
//
//// 		for (int x = 0; x < rebar.PopVertices().GetSize(); x++)
//// 		{
//// 			RebarVertexP vex2 = rebar.PopVertices().GetAt(x);
//// 			vecPointStartEnd.push_back(vex2->GetIP());
//// 		}
//	}

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

RebarSetTag* ACCSTWallRebarAssemblyNew::MakeRebars
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	double              xLen,
	double              height,
	double              spacing,
	double              startOffset,
	double              endOffset,
	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
	CMatrix3D const&    mat,
	TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
	bool				bTwinbarLevel,
	int level, 
	int grade,
	int DataExchange,
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
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeStart.SetType(RebarEndType::kLap);
		begStraightAnchorLen = endType[0].endPtInfo.value1;	//锚入长度
		break;
	case 4:	//90度弯钩
	{
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		startbendLen = endType[0].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		}
		startbendLenTb = endType[0].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(startbendLenTb, 0) == 0)
		{
			startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//乘以了100
		}
	}
		break;
	case 5:	//135度弯钩
	{
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		startbendLen = endType[0].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		}
		startbendLenTb = endType[0].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(startbendLenTb, 0) == 0)
		{
			startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//乘以了100
		}
	}
		break;
	case 6:	//180度弯钩
	{
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		startbendLen = endType[0].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		}
		startbendLenTb = endType[0].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(startbendLenTb, 0) == 0)
		{
			startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//乘以了100
		}
	}
		break;
	case 8:	//用户
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (endType[1].endType)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeEnd.SetType(RebarEndType::kLap);
		endStraightAnchorLen = endType[1].endPtInfo.value1;	//锚入长度
		break;
	case 4:	//90度弯钩
	{
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		endbendLen = endType[1].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		}
		endbendLenTb = endType[1].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(endbendLenTb, 0) == 0)
		{
			endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//乘以了100
		}
	}
		break;
	case 5:	//135度弯钩
	{
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		endbendLen = endType[1].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		}
		endbendLenTb = endType[1].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(endbendLenTb, 0) == 0)
		{
			endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//乘以了100
		}
	}
		break;
	case 6:	//180度弯钩
	{
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		}
		endbendLen = endType[1].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		}
		endbendLenTb = endType[1].endPtInfo.value3;//预留长度
		if (COMPARE_VALUES(endbendLenTb, 0) == 0)
		{
			endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//乘以了100
		}
		
	}

		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);	//乘以了30
	double adjustedXLen, adjustedSpacing;

	double leftSideCov = GetSideCover()*uor_per_mm / sin(m_angle_left);
	double rightSideCov = GetSideCover()*uor_per_mm / sin(m_angle_right);
	double allSideCov = leftSideCov + rightSideCov;
	int numRebar = 0;
	if (twinBarInfo.hasTwinbars)	//并筋
		adjustedXLen = xLen - allSideCov - diameter - diameterTb - startOffset - endOffset;
	else
		adjustedXLen = xLen - allSideCov - diameter - startOffset - endOffset;
	if (bTwinbarLevel)				//并筋层钢筋条数
	{
		numRebar = (int)floor(adjustedXLen / (spacing * (twinBarInfo.interval + 1)) + 0.5) + 1;
		int numRebar1 = (int)floor(adjustedXLen / spacing + 0.5) + 1;
		adjustedSpacing = spacing;
		if (numRebar1 > 1)
		{
			adjustedSpacing = adjustedXLen / (numRebar1 - 1);	//非并筋层平均间距
			adjustedSpacing *= (twinBarInfo.interval + 1);		//并筋层实际间距需乘以钢筋间隔
		}
	}
	else
	{
		numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;
		adjustedSpacing = spacing;
		if (numRebar > 1)
			adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	double xPos = startOffset;
	if (bTwinbarLevel)				//并筋层需偏移一段距离
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
	if (endType[0].endType != 0 && endType[0].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	if (bTwinbarLevel)				//并筋层
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
	if (bTwinbarLevel)				//并筋层
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
	vecStartEnd.clear();
	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurves;
		makeRebarCurve(rebarCurves, xPos, height - allSideCov, endTypeStartOffset, endTypEendOffset, endTypes, mat,bTwinbarLevel);

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
//	vector<DSegment3d> vecAllStartEnd;
	EditElementHandle tmpeeh(GetSelectedElement(), GetSelectedModel());
	vector<EditElementHandle*> veceehs;
	veceehs.push_back(&tmpeeh);
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		/*DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);*/
//		vecAllStartEnd.push_back(DSegment3d::From(ptstr, ptend));
		Transform matrix;
		
		vector<DPoint3d> tmpptsTmp;
		GetIntersectPointsWithOldElm(tmpptsTmp,&tmpeeh, ptstr, ptend, 0.1, matrix);
		if (tmpptsTmp.size()==0)
		{
			bvector<DPoint3d> ips;
			rebarCurve.GetIps(ips);

			for (size_t i = 0;i<ips.size()-1;i++)
			{
				DPoint3d tmpstr = ips[i];
				DPoint3d tmpend = ips[i + 1];
				GetIntersectPointsWithOldElm(tmpptsTmp, &tmpeeh, tmpstr, tmpend, 0.1, matrix);
			}
			if (tmpptsTmp.size() == 0)//如果都没有交点，判断点是否在体内部
		   {
				bool isInSide = false;
				for (size_t j = 0; j < ips.size(); j++)
				{
					if (ISPointInHoles(veceehs,ips[j]))
					{
						isInSide = true;
						break;
					}
				}
				if (!isInSide)
				{
					/*EditElementHandle tmpline;
					LineHandler::CreateLineElement(tmpline, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);*/
					//tmpline.AddToModel();
					continue;
				}
		   }
		}
		
		//if (rebarCurve.PopVertices().GetSize() > 2)//有端部偏移
		//{
		//	for (int x = 0; x < rebarCurve.PopVertices().GetSize()-1; x++)
		//	{
		//		RebarVertexP vex1 = rebarCurve.PopVertices().GetAt(x);
		//		RebarVertexP vex2 = rebarCurve.PopVertices().GetAt(x+1);
		//		vecStartEnd.push_back(DSegment3d::From(vex1->GetIP(), vex2->GetIP()));
		//	}
		//}
		//else
		//{
		//	vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));
		//}
// 		EditElementHandle eeh;
// 		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
// 		eeh.AddToModel();

		RebarElementP rebarElement = NULL;
		if (!PreviewButtonDown)//预览按钮标志
		{
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
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);

	if (bTwinbarLevel)
	{
		m_vecTieRebarStartEnd.push_back(vecStartEnd);
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

void ACCSTWallRebarAssemblyNew::CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb,const CVector3D& org,DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	vTransform.clear();
	vTransformTb.clear();
	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double dLevelSpace = 0;
	double diameterTie = 0.0;
	BrString strTieRebarSize = XmlManager::s_alltypes[GetTieRebarInfo().rebarType];
	if (strTieRebarSize != L"" /*&& 0 != GetTieRebarInfo().tieRebarMethod*/)
	{
		diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//拉筋直径
	}

	double dOffset = dPositiveCover + diameterTie;
	double dOffsetTb = dPositiveCover + diameterTie;
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		BrString  sizekey = XmlManager::s_alltypes[GetvecRebarType().at(i)];
		double diameter = RebarCode::GetBarDiameter(sizekey, modelRef);		//乘以了10
		double diameterTb = 0.0;
		if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
		{
			diameterTb = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(i).rebarSize, modelRef);		//乘以了10
		}

		if (diameter > BE_TOLERANCE)
		{
			CVector3D	zTrans(org);
			zTrans.Normalize();
			CVector3D	zTransTb;
			zTrans.Normalize();
			if (GetvecDir().at(i) == 0) //水平
			{
				zTrans.z = m_STwallData.height - dSideCover - diameter * 0.5;
				zTrans.x = m_STwallData.length * 0.5;
			}
			else
			{
				zTrans.z = m_STwallData.height * 0.5;
				zTrans.x = dSideCover + diameter * 0.5;
			}
			zTransTb = zTrans;
			{
				double diameterPre;
				if (0 == i)
				{
					dOffset += diameter / 2.0;	//偏移首层钢筋半径
					dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm;
					diameterPre = diameter;
				}
				else
				{
					BrString  sizekey2 = XmlManager::s_alltypes[GetvecRebarType().at(i - 1)];
					diameterPre = RebarCode::GetBarDiameter(sizekey2, modelRef);		//乘以了10
					dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm + diameter * 0.5 + diameterPre * 0.5;//层间距加上当前钢筋直径
				}
				dOffset += dLevelSpace;
				dOffsetTb = dOffset;
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
				{
					if (diameterTb > diameter)//并筋层的钢筋比主筋直径大
						dOffsetTb += (diameterTb / 2.0 - diameter / 2.0);
					else
						dOffsetTb -= (diameter / 2.0 - diameterTb / 2.0);
				}
				if (COMPARE_VALUES(m_STwallData.width - dOffset, dReverseCover) < 0)		//当前钢筋层已嵌入到了反面保护层中时，实际布置的钢筋层间距就不再使用设置的与上层间距，而是使用保护层进行限制
				{
					zTrans.y = m_STwallData.width - dReverseCover - diameter / 2.0;
					zTransTb.y = zTrans.y;
					if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
					{
						if (diameterTb > diameter)//并筋层的钢筋比主筋直径大
							zTransTb.y += (diameterTb / 2.0 - diameter / 2.0);
						else
							zTransTb.y -= (diameter / 2.0 - diameterTb / 2.0);
					}
					//判断：如果上一层的zTrans.y与当前层的zTrans.y相同，则上一层减去当前层的钢筋直径。（防止钢筋碰撞）
					double compare = zTrans.y;
					if (vTransform.size() > 0)
					{
						double reverseOffset = diameter;
						for (int j = (int)vTransform.size() - 1; j >= 0; j--)
						{
							BrString  sizekey3 = XmlManager::s_alltypes[GetvecRebarType().at(j)];
							diameterPre = RebarCode::GetBarDiameter(sizekey3, modelRef);		//乘以了10
							if (COMPARE_VALUES(vTransform[j].y + diameterPre * 0.5, compare - diameter * 0.5) > 0)	//嵌入了下一根钢筋终
							{
								vTransform[j].y -= reverseOffset;
								vTransformTb[j].y = vTransform[j].y;
								if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(j).hasTwinbars)
								{
									double diameterTbPre = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(j).rebarSize, modelRef);		//乘以了10

									if (diameterTbPre > diameterPre)//并筋层的钢筋比主筋直径大
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
					// 					if (GetvecTwinRebarLevel().at(i).hasTwinbars && diameterTb > diameter)	//并筋层的钢筋比主筋直径大
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

void ACCSTWallRebarAssemblyNew::CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef)
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
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改

	if (!GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		/*EditElementHandle eeh1;
		LineHandler::CreateLineElement(eeh1, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
		eeh1.AddToModel();*/
		DPoint3d ptProject1;	//投影点
		mdlVec_projectPointToLine(&ptProject1, NULL, &pt1[0], &m_STwallData.ptStart, &m_STwallData.ptEnd);

		DPoint3d ptProject2;	//投影点
		mdlVec_projectPointToLine(&ptProject2, NULL, &pt1[1], &m_STwallData.ptStart, &m_STwallData.ptEnd);

		lenth = pt1[0].Distance(pt1[1]);
		misDisstr = ptProject1.Distance(ptstar);

		misDisend = ptProject2.Distance(ptend);

	}
}
void ACCSTWallRebarAssemblyNew::CalculateUseHoles(DgnModelRefP modelRef)
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

	if (g_wallRebarInfo.concrete.isHandleHole)//计算需要处理的孔洞
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
			//计算指定元素描述符中元素的范围。
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
				if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//如果是门洞
				{
					continue;
				}
				bool isdoorNeg = false;//判断是否为门洞NEG
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

bool ACCSTWallRebarAssemblyNew::MakeRebars(DgnModelRefP modelRef)
{
	m_rsetTags.Clear(true);
	NewRebarAssembly(modelRef);
	//RebarSetTagArray rsetTags;
	CalculateUseHoles(modelRef);
	m_vecRebarStartEnd.clear();
	m_vecTieRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
	double dLevelSpace = 0;
	double dSideCover = GetSideCover()*uor_per_mm;
	if (COMPARE_VALUES(dSideCover, m_STwallData.length) >= 0)	//如果侧面保护层大于等于墙的长度
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"侧面保护层大于等于墙的长度,无法创建钢筋层", MessageBoxIconType::Information);
		return false;
	}
	vector<CVector3D> vTrans;
	vector<CVector3D> vTransTb;
	//计算侧面整体偏移量
	CalculateTransform(vTrans, vTransTb, CVector3D(0,0,0), modelRef);
	if (vTrans.size() != GetRebarLevelNum())
	{
		return false;
	}

	double dLength = m_STwallData.length;
	double dWidth = m_STwallData.height;

	int iRebarSetTag = 0;
	int iRebarLevelNum = GetRebarLevelNum();
//	int cutSolidNum = (int)m_CutSolids.size();
//	int setNum = cutSolidNum / 2;
//	for (int j = 0; j < setNum*(iRebarLevelNum-2); ++j)	//新增RebarSetId
	int setCount = 0;
	vector<PIT::EndType> vecEndType;
	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		BrString  sizekeyi = XmlManager::s_alltypes[GetvecRebarType().at(i)];
		//		int iActualIndex = i;
		RebarSetTag* tag = NULL;
		CMatrix3D   mat, matTb;

		vector<PIT::EndType> vecEndType;
		if (GetvvecEndType().empty())		//没有设置端部样式，设置默认值
			vecEndType = { { 0,0,0,0,0,0 },{0,0,0,0,0,0} };
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
		
		double diameter = RebarCode::GetBarDiameter(sizekeyi, modelRef);
		if (COMPARE_VALUES(vecEndType[0].endPtInfo.value1,0) == 0)
		{
			vecEndType[0].endPtInfo.value1 = RebarCode::GetPinRadius(sizekeyi, modelRef, false);
		}
		m_vecRebarPtsLayer.clear();
		if (COMPARE_VALUES(vecEndType[1].endPtInfo.value1, 0) == 0)
		{
			vecEndType[1].endPtInfo.value1 = RebarCode::GetPinRadius(sizekeyi, modelRef, false);
		}

		if (GetvecDir().at(i) == 1)	//纵向钢筋
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
			CVector3D	endNormal;	//端部弯钩方向
			if (GetvvecEndType().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = m_STwallData.ptEnd - m_STwallData.ptStart;
					endNormal.Normalize();
					CVector3D rebarVec = CVector3D::kZaxis;
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}

			if (m_CutSolids.size() == 0)	//当前墙未被切断
			{
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;
				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
				//绘制并筋--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
				{
					//先绘制非并筋层
					PopvecSetId().push_back(0);
					setCount++;
					tag = MakeRebars(PopvecSetId().back(), sizekeyi, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i), GetvecRebarType().at(i) , GetvecDataExchange().at(i), modelRef);
					if (NULL != tag)
					{
						tag->SetBarSetTag(setCount);
						m_rsetTags.Add(tag);
					}

					//绘制并筋层
					PopvecSetId().push_back(0);
					setCount++;
					tag = MakeRebars(PopvecSetId().back(), sizekeyi, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), true, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
					if (NULL != tag)
					{
						tag->SetBarSetTag(setCount);
						m_rsetTags.Add(tag);
					}
				}
				else //当前层未设置并筋
				{
					TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
					PopvecSetId().push_back(0);
					setCount++;
					tag = MakeRebars(PopvecSetId().back(), sizekeyi, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat, twinRebar, false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
					if (NULL != tag)
					{
						tag->SetBarSetTag(setCount);
						m_rsetTags.Add(tag);
					}
				}
			}
			else
			{

				//当前墙被关联关系为被锚入的墙所切断
				for (int j = 0; j < m_vecCutWallData.size(); j++)
				{
					vector<EditElementHandle*> tmpwalls;
					tmpwalls.push_back(m_CutSolids[j]);
					bool isInCutEle = false;
					for (int tmpi = 0;tmpi<GetvecCutPoints().size();tmpi++)
					{
						if (ISPointInHoles(tmpwalls,GetvecCutPoints().at(tmpi)))
						{
							isInCutEle = true;
							break;
						}
					}
					//if (j & 1)
					if(isInCutEle)
					{
						continue;
						bool bNearStart = false;
						vector<MSElementDescrP> vec_line;
						vector<MSElementDescrP> vec_linefront;
						vector<MSElementDescrP> vec_lineback;
						EFT::GetFrontBackLineAndDownFace(*m_CutSolids[j], NULL, vec_line, vec_linefront, vec_lineback, NULL);
						vector<LineSegment> lineSegs;
						for (MSElementDescrP ms : vec_line)
						{
							DPoint3d pt[2];
							mdlLinear_extract(pt, NULL, &ms->el, m_CutSolids[j]->GetModelRef());
// 							if (CVector3D(pt[0], pt[1]).IsParallelTo(CVector3D(GetsegOrg().point[0], GetsegOrg().point[1])))
// 							{
							    pt[0].z = pt[1].z = pt[0].z + dSideCover;
								LineSegment seg(pt[0], pt[1]);
								
								DVec3d vec;
								vec.CrossProduct(CVector3D::kZaxis,seg.GetLineVec());
								seg.PerpendicularOffset(diameter*0.5, vec);
								lineSegs.push_back(seg);
// 							}
// 							else
// 							{
// 								lineSegs.push_back(LineSegment(pt[0], pt[1]));
// 							}
							mdlElmdscr_freeAll(&ms);
						}
						PopvecSetId().push_back(0);
						setCount++;
						LineStringRebarMakerNew lineStrRebar(GetCallerId(),lineSegs, sizekeyi, GetvecDirSpacing().at(i)*uor_per_mm,dWidth-dSideCover*2,dSideCover);
						lineStrRebar.m_useHoleehs = m_useHoleehs;
						tag = lineStrRebar.MakeRebar(PopvecSetId().back(), modelRef);
						if (NULL != tag)
						{
							tag->SetBarSetTag(setCount);
							m_rsetTags.Add(tag);
						}
					}
					else
					{
						dLength = m_vecCutWallData[j].length;
						DPoint3d ptStart = m_vecCutWallData[j].ptStart;
						DPoint3d ptEnd = m_vecCutWallData[j].ptEnd;
						CVector3D  xVec(ptStart, ptEnd);
						CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
						CMatrix3D   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴
						CMatrix3D   matCut, matCutTb;
						matCut.SetTranslation(vTrans[i]);
						matCutTb.SetTranslation(vTransTb[i]);
						matCut = placement * matCut;
						matCutTb = placement * matCutTb;

						//绘制并筋--begin
						if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
						{
							//先绘制非并筋层
							PopvecSetId().push_back(0);
							setCount++;
							tag = MakeRebars(PopvecSetId().back(), sizekeyi, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
								GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, matCut, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
							if (NULL != tag)
							{
								tag->SetBarSetTag(setCount);
								m_rsetTags.Add(tag);
							}

							//绘制并筋层
							PopvecSetId().push_back(0);
							setCount++;
							tag = MakeRebars(PopvecSetId().back(), sizekeyi, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
								GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, matCutTb, GetvecTwinRebarLevel().at(i), true, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
							if (NULL != tag)
							{
								tag->SetBarSetTag(setCount);
								m_rsetTags.Add(tag);
							}
						}
						else //当前层未设置并筋
						{
							TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
							PopvecSetId().push_back(0);
							setCount++;
							tag = MakeRebars(PopvecSetId().back(), sizekeyi, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_right,
								GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, matCut, twinRebar, false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
							if (NULL != tag)
							{
								tag->SetBarSetTag(setCount);
								m_rsetTags.Add(tag);
							}
						}
					}
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
			CVector3D	endNormal;	//端部弯钩方向
			if (GetvvecEndType().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					CVector3D rebarVec = m_STwallData.ptEnd - m_STwallData.ptStart;
					endNormal = CVector3D::From(0, 0, -1);
// 					if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
// 					{
// 						endNormal.Negate();
// 					}
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}
			mat = rot90;
			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;
			matTb = rot90;
			matTb.SetTranslation(vTransTb[i]);
			matTb = GetPlacement() * matTb;
			//奇数层为并筋层,偶数层为普通层

			//绘制并筋--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
			{
				//先绘制非并筋层
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), sizekeyi, dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
				if (NULL != tag)
				{
					tag->SetBarSetTag(setCount);
					m_rsetTags.Add(tag);
				}

				//绘制并筋层
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), sizekeyi, dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), true, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
				if (NULL != tag)
				{
					tag->SetBarSetTag(setCount);
					m_rsetTags.Add(tag);
				}
			}
			else //当前层未设置并筋
			{
				TwinBarSet::TwinBarLevelInfo twinRebar = {"",0,"",0,0};
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), sizekeyi, dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, twinRebar, false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
				if (NULL != tag)
				{
					tag->SetBarSetTag(setCount);
					m_rsetTags.Add(tag);
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
				rbPt.vecDir = GetvecDir().at(i);
				rbPt.ptstr = m_vecRebarPtsLayer.at(m);
				rbPt.ptend = m_vecRebarPtsLayer.at(n);
				rbPt.DataExchange = GetvecDataExchange().at(i);
				g_vecRebarPtsNoHole.push_back(rbPt);
				m++;
			}
		}
		
	}

	//添加拉筋--begin
	vector<vector<DSegment3d>> vctTieRebarLines;//存储所有的拉筋直线信息，用于预览
	if (0 != GetTieRebarInfo().tieRebarMethod/* && (m_vecAllRebarStartEnd.size() >= 4)*/)
	{
		PopvecSetId().push_back(0);
		setCount++;
		BrString  sizekey0 = XmlManager::s_alltypes[GetvecRebarType().at(0)];
		FaceRebarDataArray faceDataArray;
		faceDataArray.posRebarData.HRebarData.rebarSize = sizekey0;
		faceDataArray.posRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(0);
		faceDataArray.posRebarData.VRebarData.rebarSize = XmlManager::s_alltypes[GetvecRebarType().at(1)];
		faceDataArray.posRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(1);

		faceDataArray.revRebarData.HRebarData.rebarSize = XmlManager::s_alltypes[GetvecRebarType().at(GetvecRebarLevel().size() - 1)];
		faceDataArray.revRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecRebarLevel().size() - 1);
		faceDataArray.revRebarData.VRebarData.rebarSize = XmlManager::s_alltypes[GetvecRebarType().at(GetvecRebarLevel().size() - 2)];;
		faceDataArray.revRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecRebarLevel().size() - 2);

		vector<vector<DSegment3d> > vecStartEnd;		//只存储1，2层和倒数第1，2层
		vecStartEnd.push_back(m_vecTieRebarStartEnd[0]);
		vecStartEnd.push_back(m_vecTieRebarStartEnd[1]);
		vecStartEnd.push_back(m_vecTieRebarStartEnd[m_vecTieRebarStartEnd.size() - 2]);
		vecStartEnd.push_back(m_vecTieRebarStartEnd[m_vecTieRebarStartEnd.size() - 1]);
		BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
		int	tieRebarStyle = GetTieRebarInfo().tieRebarStyle;
		if (strTieRebarSize.Find(L"mm") != string::npos)
			strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm
		TieRebarMaker tieRebarMaker(faceDataArray, vecStartEnd, (TieRebarStyle)tieRebarStyle, strTieRebarSize);
		tieRebarMaker.m_CallerId = GetCallerId();
		tieRebarMaker.SetDownVec(m_STwallData.ptStart, m_STwallData.ptEnd);
		tieRebarMaker.SetCustomStyle(GetTieRebarInfo().rowInterval, GetTieRebarInfo().colInterval);
		tieRebarMaker.SetModeType(0);
		RebarSetTag* tag = tieRebarMaker.MakeRebar(PopvecSetId().at(GetvecSetId().size() - 1), modelRef);
		tieRebarMaker.GetRebarPts(vctTieRebarLines);//取出所有的拉筋直线信息
		if (NULL != tag &&(!PreviewButtonDown))
		{
			tag->SetBarSetTag(setCount);
			m_rsetTags.Add(tag);
		}
	}

	//预览按钮按下，则画线
	if (PreviewButtonDown)
	{
		m_allLines.clear();
		//主筋及并筋线信息
		for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)
		{
			vector<DSegment3d> vcttemp(*it);
			for (int x = 0; x < vcttemp.size(); x++)
			{
				DPoint3d strPoint = DPoint3d::From(vcttemp[x].point[0].x, vcttemp[x].point[0].y, vcttemp[x].point[0].z);
				DPoint3d endPoint = DPoint3d::From(vcttemp[x].point[1].x, vcttemp[x].point[1].y, vcttemp[x].point[1].z);
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allLines.push_back(eeh.GetElementRef());
			}
		}
		//拉筋直线信息
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
	}

	return true;
	//return (SUCCESS == AddRebarSets(m_rsetTags));
}

long ACCSTWallRebarAssemblyNew::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool ACCSTWallRebarAssemblyNew::OnDoubleClick()
{
// 	vector<PIT::ConcreteRebar> vecRebarData;
// 	vector<PIT::LapOptions> vecLaptionData;
// 	vector<PIT::EndType> vecEndTypeData;
// 	vector<TwinBarSet::TwinBarLevelInfo> vecTwinBarData;
// 	Concrete concreteData;
// 	//	TwinBarSet::TwinBarInfo twInfo;
// 	TieReBarInfo tieRebarInfo;
// 	// 	GetRebarData(vecData);
// // 	GetConcreteData(concreteData);
// 	ComponentType wallType = GetcpType();
// 	// 	int lastAction = ACTIONBUTTON_CANCEL;
// 	// 	if (SUCCESS != mdlDialog_openModal(&lastAction, GetResourceHandle(), DIALOGID_WallRebar) || lastAction != ACTIONBUTTON_OK)
// 	// 		return false;
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}
// 
// 	SetSelectedModel(modelp);
// 	GetConcreteXAttribute(testid, ACTIVEMODEL);
// 
// 	DgnModelRefP modelRef = ACTIVEMODEL;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	pACCSTWallDoubleRebarDlg = new CWallRebarDlgNew(ehSel, CWnd::FromHandle(MSWIND));
	pACCSTWallDoubleRebarDlg->Create(IDD_DIALOG_WallRebar);
	pACCSTWallDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pACCSTWallDoubleRebarDlg->ShowWindow(SW_SHOW);

// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
// 	CWallRebarDlg dlg(ehSel,CWnd::FromHandle(MSWIND));
// //	dlg.SetSelectElement(ehSel);
// 	dlg.SetConcreteId(FetchConcrete());
// 	if (IDCANCEL == dlg.DoModal())
// 		return false;
// 	dlg.m_PageMainRebar.GetConcreteData(concreteData);
// 	dlg.m_PageMainRebar.GetListRowData(vecRebarData);
// 	//	dlg.m_PageLapOption.GetListRowData(vecLaptionData);
// 	dlg.m_PageEndType.GetListRowData(vecEndTypeData);
// 	//	dlg.m_PageTwinBars.GetTwinBarInfo(twInfo);
// 	dlg.m_PageTwinBars.GetListRowData(vecTwinBarData);
// 	dlg.m_PageTieRebar.GetTieRebarData(tieRebarInfo);
// 	if (dlg.m_PageMainRebar.m_assodlg)
// 	{
// 		dlg.m_PageMainRebar.m_assodlg->GetListRowData(g_vecACData);
// 	}
// 	SetcpType(wallType);
// 	SetComponentData(ehSel);
// 	SetConcreteData(concreteData);
// 	SetRebarData(vecRebarData);
// //	SetvecLapOptions(vecLaptionData);
// 	SetRebarEndTypes(vecEndTypeData);
// 	SetvecTwinRebarLevel(vecTwinBarData);
// 	InitRebarSetId();
// 	SetTieRebarInfo(tieRebarInfo);
// 	SetACCRebarMethod(1);
// 	SetvecAC(g_vecACData);
// 	ACCRebarMaker::CreateACCRebar(this, ehSel, modelRef);
// 	Save(modelRef);
// 	ElementId conid = GetConcreteOwner();
// 	SetConcreteXAttribute(conid, ACTIVEMODEL);
// 	SetElementXAttribute(GetSelectedElement(), sizeof(ElementId), &conid, ConcreteIDXAttribute, GetSelectedModel());
// 	ACCConcrete concrete;
// 	//先取出之前存储的数据
// 	GetElementXAttribute(ehSel.GetElementId(), sizeof(ACCConcrete), concrete, ConcreteCoverXAttribute, ehSel.GetModelRef());
// 	concrete.postiveOrTopCover = g_wallRebarInfo.concrete.postiveCover;
// 	concrete.reverseOrBottomCover = g_wallRebarInfo.concrete.reverseCover;
// 	concrete.sideCover = g_wallRebarInfo.concrete.sideCover;
// 
// 	SetElementXAttribute(ehSel.GetElementId(), sizeof(ACCConcrete), &concrete, ConcreteCoverXAttribute, ehSel.GetModelRef());
	//	ElementId contid = GetConcreteOwner();
	return true;
}

bool ACCSTWallRebarAssemblyNew::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();

	SetComponentData(ehWall);
	MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();

	//eeh2.AddToModel();
	return true;
}


bool ACCSTWallRebarAssemblyNew::SetComponentData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricDataAndHoles(eh,m_STwallData);
	if (!bRet)
		return false;
	InitUcsMatrix();
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}

void ACCSTWallRebarAssemblyNew::InitUcsMatrix()
{
	DPoint3d ptStart = m_STwallData.ptStart;
	DPoint3d ptEnd = m_STwallData.ptEnd;

	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
// 	CVector3D  yVecNegate = yVec;
// 	yVecNegate.Negate();
// 	yVecNegate.Normalize();
// 	yVecNegate.ScaleToLength(m_STwallData.width);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
// 	ptStart.Add(yVecNegate);
// 	ptEnd.Add(yVecNegate);

	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);
	
}

//enum STWALLACCRelation
//{
//	StartPt,
//	EndPt,
// 	CurStartACCStart = 1,		//起点与起点相接
// 	CurStartACCEnd,			//起点与终点相接
// 	CurEndACCStart,			//终点与起点相接
// 	CurEndACCEnd,			//终点与终点相接
//};

UInt32	ACCSTWallRebarAssemblyNew::JudgeComponentRelation(const STWallGeometryInfo &CurrSTWall, const STWallGeometryInfo &ACCSTwall)
{
	//不与X轴垂直时，左边为起点，右边为终点
	//与X轴垂直时，下边为起点，上边为终点
	//0x00  不与X轴垂直，左上角
	//0x01  不与X轴垂直，右上角
	//0x11  不与X轴垂直，右下角
	//0x10  不与X轴垂直，左上角

	//0x100   与X轴垂直，右下角
	//0x101   与X轴垂直，右上角
	//0x111   与X轴垂直，左上角
	//0x110   与X轴垂直，左下角

	//判断关联墙在配筋墙的起点还是终点
	//当前构件中心线起点到关联构件起点线段的投影点
	DPoint3d ptPro1, ptPro2;
	mdlVec_projectPointToLine(&ptPro1, NULL, &CurrSTWall.ptStart, &ACCSTwall.ptStart, &ACCSTwall.ptEnd);
	double dis1 = ptPro1.Distance(CurrSTWall.ptStart);		//当前构件起点到关联构件的距离
	//当前构件中心线起点到关联构件终点线段的投影点
	mdlVec_projectPointToLine(&ptPro2, NULL, &CurrSTWall.ptEnd, &ACCSTwall.ptStart, &ACCSTwall.ptEnd);
	double dis2 = ptPro2.Distance(CurrSTWall.ptEnd);		//当前构件终点到关联构件的距离

	UInt32 ret = 0;
	DVec3d vec = CurrSTWall.ptEnd - CurrSTWall.ptStart;
	vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
	vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
	vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
	vec.Normalize();
	if (vec.IsPerpendicularTo(DVec3d::From(1, 0, 0)))	//当前构件垂直X轴
	{
		ret |= 0x100;
	}
	DPoint3d vecY = DPoint3d::From(0, 1, 0);
	if (COMPARE_VALUES_EPS(dis1, dis2, 10) > 0)		//关联构件在当前构件的终点
	{
		ret |= 0x01;
		//判断当前构件在关联构件的左边还是右边,是 “1_”还是“_1”
		DPoint3d midPt = ACCSTwall.ptStart;
		midPt.Add(ACCSTwall.ptEnd);
		midPt.Scale(0.5);
		midPt.z = CurrSTWall.ptEnd.z;

		DPoint3d VecZ = DPoint3d::From(0, 0, 1);
		DPoint3d vecNormal;
		DPoint3d vecLine = CurrSTWall.ptEnd - CurrSTWall.ptStart;
		vecLine.Normalize();
		vecNormal = VecZ;
		vecNormal.CrossProduct(VecZ, vecLine);

		DPoint3d midPtproject;
		mdlVec_projectPointToLine(&midPtproject, NULL, &midPt, &CurrSTWall.ptStart, &CurrSTWall.ptEnd);
		DPoint3d vecaccLine = midPt - midPtproject;
		vecaccLine.Normalize();
		if (ret& 0x100)//与X轴垂直
		{
			if (vecaccLine.DotProduct(vecNormal) > 0)//在同一边
			{
				ret |= 0x10;
			}
		}
		else
		{
			if (vecaccLine.DotProduct(vecNormal) < 0)//在同一边
			{
				ret |= 0x10;
			}
		}
		
		
	}
	else if (COMPARE_VALUES_EPS(dis1, dis2, 10) < 0)	//起点
	{
		DPoint3d midPt = ACCSTwall.ptStart;
		midPt.Add(ACCSTwall.ptEnd);
		midPt.Scale(0.5);
		midPt.z = CurrSTWall.ptStart.z;

		DPoint3d VecZ = DPoint3d::From(0, 0, 1);
		DPoint3d vecNormal;
		DPoint3d vecLine = CurrSTWall.ptStart - CurrSTWall.ptEnd;
		vecLine.Normalize();
		vecNormal = VecZ;
		vecNormal.CrossProduct(VecZ, vecLine);

		DPoint3d midPtproject;
		mdlVec_projectPointToLine(&midPtproject, NULL, &midPt, &CurrSTWall.ptStart, &CurrSTWall.ptEnd);
		DPoint3d vecaccLine = midPt - midPtproject;
		if (ret & 0x100)//与X轴垂直
		{
			if (vecaccLine.DotProduct(vecNormal) < 0)//在同一边
			{
				ret |= 0x10;
			}
		}
		else
		{
			if (vecaccLine.DotProduct(vecNormal) > 0)//在同一边
			{
				ret |= 0x10;
			}
		}
	
	}
	else
	{
		return 0xFFFF;
	}

	return ret;
}