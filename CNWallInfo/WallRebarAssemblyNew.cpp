/*--------------------------------------------------------------------------------------+
|
|     $Source: WallRebarAssembly.cpp $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_USTATION.h"
#include "../resource.h"
#include <SelectionRebar.h>
#include "../GalleryIntelligentRebar.h"
#include "WallRebarAssemblyNew.h"
#include "CWallRebarDlgNew.h"
#include "ExtractFacesTool.h"
#include "CWallRebarDlgNew.h"
#include "../TieRebar.h"
#include "ElementAttribute.h"
#include "PITMSCECommon.h"
#include "ExtractFacesTool.h"
#include "../XmlHelper.h"
#include "../XmlManager.h"

extern bool PreviewButtonDownNew;

using namespace PIT;
WallRebarAssemblyNew::WallRebarAssemblyNew(ElementId id, DgnModelRefP modelRef) :
	PITRebarAssembly(id, modelRef),
	m_PositiveCover(0),
	m_ReverseCover(0),
	m_SideCover(0),
	m_RebarLevelNum(4),
	m_width(0)
{
	Init();
}

void WallRebarAssemblyNew::Init()
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
// 	int twinRebarLevel = 0;
// 
// 	//根据需求并筋需要设置不一样的层
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

void WallRebarAssemblyNew::SetConcreteData(Concrete const& concreteData)
{
	m_PositiveCover = concreteData.postiveCover;
	m_ReverseCover = concreteData.reverseCover;
	m_SideCover = concreteData.sideCover;
	m_RebarLevelNum = concreteData.rebarLevelNum;
}

void WallRebarAssemblyNew::SetRebarData(vector<PIT::ConcreteRebar> const& vecData)
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
		BrString keySize = XmlManager::s_alltypes[data.rebarType];

		double diameter = RebarCode::GetBarDiameter(keySize, ACTIVEMODEL) / uor_per_mm;
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
		BrString keySize = XmlManager::s_alltypes[data.rebarType];
		double diameter = RebarCode::GetBarDiameter(keySize, ACTIVEMODEL) / uor_per_mm;
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
		BrString keySize = XmlManager::s_alltypes[data.rebarType];
		double diameter = RebarCode::GetBarDiameter(keySize, ACTIVEMODEL) / uor_per_mm;
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
			m_vecRebarType[i] = tmpvecdata[i].rebarType;
			m_vecDirSpacing[i] = tmpvecdata[i].spacing;
			m_vecStartOffset[i] = tmpvecdata[i].startOffset;
			m_vecEndOffset[i] = tmpvecdata[i].endOffset;
			m_vecLevelSpace[i] = tmpvecdata[i].levelSpace;
			m_vecDataExchange[i] = tmpvecdata[i].datachange;
			m_vecRebarLevel[i] = tmpvecdata[i].rebarLevel;
			//			GetDiameterAddType(m_vecDirSize[i], m_vecRebarType[i]);
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
			//			m_vecSetId.push_back(0);
		}
	}

}

void WallRebarAssemblyNew::SetRebarEndTypes(vector<PIT::EndType> const & vecEndTypes)
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

void WallRebarAssemblyNew::InitRebarSetId()
{
	int twinRebarLevel = 0;
	//根据需求并筋需要设置不一样的层
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

void WallRebarAssemblyNew::GetConcreteData(Concrete& concreteData)
{
	concreteData.postiveCover = m_PositiveCover;
	concreteData.reverseCover = m_ReverseCover;
	concreteData.sideCover = m_SideCover;
	concreteData.rebarLevelNum = m_RebarLevelNum;
//	concreteData.isTwinbars = m_Twinbars;
}
void WallRebarAssemblyNew::GetRebarData(vector<PIT::ConcreteRebar>& vecData) const
{
	if (0 == m_RebarLevelNum || m_vecDir.empty() || m_vecDirSize.empty() || m_vecDirSpacing.empty())
	{
		return;
	}
	for (size_t i = 0; i < m_RebarLevelNum; i++)
	{
		ConcreteRebar rebarData;
		rebarData.rebarDir = m_vecDir[i];
		rebarData.rebarType = m_vecRebarType[i];
		rebarData.spacing = m_vecDirSpacing[i];
		rebarData.startOffset = m_vecStartOffset[i];
		rebarData.endOffset = m_vecEndOffset[i];
		rebarData.levelSpace = m_vecLevelSpace[i];
		rebarData.datachange = m_vecDataExchange[i];
		vecData.push_back(rebarData);
	}
}

void WallRebarAssemblyNew::SetTieRebarInfo(TieReBarInfo const & tieRebarInfo)
{
	m_tieRebarInfo = tieRebarInfo;
}

const TieReBarInfo WallRebarAssemblyNew::GetTieRebarInfo() const
{
	return m_tieRebarInfo;
}

void WallRebarAssemblyNew::ClearLines()
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

void WallRebarAssemblyNew::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

// 浮点数是四舍五入 不含小数位
double WallRebarAssemblyNew::floadToInt(double dSrc)
{
	double dIndex;
	int bite_1 = static_cast<unsigned int>(dSrc * 10) % 10; // 个位数
	if (bite_1 > 4)
	{
		dIndex = static_cast<unsigned int>(dSrc + 1);
	}
	else
	{

		dIndex = static_cast<unsigned int>(dSrc);
	}
	return dIndex;
}


/***********************************************************************************************
*****	ptStr		:	直线钢筋起点
*****	ptEnd		：	直线钢筋终点
*****	vecSplit	:	计算直钢筋切割点
*****   preLength	:   前面已截取长度
************************************************************************************************/
bool WallRebarAssemblyNew::CalaLineRebarCutPoint(DPoint3dCR ptStr, DPoint3dCR ptEnd, vector<double>& vecSplit, double diameterTol, double preLength, bool bFlag)
{
	if (!m_stCutRebarInfo.isCutRebar)
	{
		return true;
	}
	double dRebarLength = ptStr.Distance(ptEnd); // 钢筋原长度

	CVector3D vec = ptEnd - ptStr;
	// 3.95 5.95 8.95
	if (COMPARE_VALUES_EPS(dRebarLength, m_CutLenIndex[2], EPS) < 0)
	{
		if (COMPARE_VALUES_EPS(dRebarLength, m_CutLenIndex[1], EPS) > 0)
		{
			vecSplit.push_back(m_CutLenIndex[1] + preLength);
		}
		if (COMPARE_VALUES_EPS(dRebarLength, m_CutLenIndex[0], EPS) > 0)
		{
			vecSplit.push_back(m_CutLenIndex[0] + preLength);
		}
		else
		{
			;
		}
		return false;
	}

	double moveLength = 0.0;
	if (bFlag)
	{ 
		// 第一种方式全部用 第三种长度(最大的)截取  此处按照m_CutLenIndex[2] 的 标准布筋 后面也是按这一标准 
		// 以后可以做修改
		vecSplit.push_back(m_CutLenIndex[2] + preLength);

		moveLength = m_CutLenIndex[2];
	}
	else
	{	
		// 切割长度 除 第三种长度 的倍数
		double dTemp1 = (preLength + m_CutLenIndex[2]) / m_CutLenIndex[2];
		double dTemp2 = (preLength + m_CutLenIndex[1]) / m_CutLenIndex[2];
		double dTemp3 = (preLength + m_CutLenIndex[0]) / m_CutLenIndex[2];
		
		// 长度除 第三种长度 的倍数 减去 最近的 第三种长度的公倍数 就是 两者之间的距离 要小于 给定距离
		// 减去四舍五入后的整数倍m_CutLenIndex[2]
		// 如 ： 3.56 倍 m_CutLenIndex[2] 长度 ---》 (4 - 3.56) * m_CutLenIndex[2] < 给定距离 
		if (COMPARE_VALUES_EPS(fabs(dTemp1 - floadToInt(dTemp1)) * m_CutLenIndex[2], diameterTol, EPS) > 0)
		{
			vecSplit.push_back(m_CutLenIndex[2] + preLength);

			moveLength = m_CutLenIndex[2];
		}	
		else if (COMPARE_VALUES_EPS(fabs(dTemp2 - floadToInt(dTemp2)) * m_CutLenIndex[2], diameterTol, EPS) > 0)
		{
			vecSplit.push_back(m_CutLenIndex[1] + preLength);

			moveLength = m_CutLenIndex[1];
		}
		else if (COMPARE_VALUES_EPS(fabs(dTemp3 - floadToInt(dTemp3)) * m_CutLenIndex[2], diameterTol, EPS) > 0)
		{
			vecSplit.push_back(m_CutLenIndex[0] + preLength);

			moveLength = m_CutLenIndex[0];
		}
		else
		{
			if (COMPARE_VALUES_EPS(m_CutLenIndex[2] + diameterTol, dRebarLength, EPS) > 0)
			{
				vecSplit.push_back(m_CutLenIndex[2] + preLength + diameterTol);

				moveLength = m_CutLenIndex[2] + diameterTol;
			}
			else
			{
				vecSplit.push_back(m_CutLenIndex[2] + preLength + diameterTol);

				moveLength = m_CutLenIndex[2] + diameterTol;
			}
		}
	} 

	DPoint3d ptTmp = ptStr;
	movePoint(vec, ptTmp, moveLength);
	CalaLineRebarCutPoint(ptTmp, ptEnd, vecSplit, diameterTol, moveLength + preLength, bFlag);

	return true;
}


/***********************************************************************************************
*****	ptStr		:	直线钢筋起点
*****	ptEnd		：	直线钢筋终点
*****	vecSplit	:	分割距离 按 离起点距离 标准
************************************************************************************************/
bool WallRebarAssemblyNew::CutLineRebarCurve(vector<PITRebarCurve>& rebars, PITRebarEndTypes& endTypes, DPoint3d& ptStr, DPoint3d& ptEnd, vector<double>& vecSplit)
{
	CVector3D vec = ptEnd - ptStr;
	vec.Normalize();  // 直线钢筋的方向

	vecSplit.push_back(ptStr.Distance(ptEnd));

	DPoint3d ptCurr = ptStr; // 钢筋线段当前点

	PITRebarEndTypes		tmpendTypes;
	for (unsigned int i = 0; i < vecSplit.size(); i++)
	{
		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();

		// 当前段起点
		vex->SetIP(ptCurr);
		vex->SetType(RebarVertex::kStart);
		// end
		
		// 当前段终点
		DPoint3d ptTmpEnd = ptStr;
		movePoint(vec, ptTmpEnd, vecSplit.at(i));
		// end

		if (ptStr.Distance(ptCurr) < 10) // 起点才有端部设置
		{
			tmpendTypes.beg = endTypes.beg;
		}
		tmpendTypes.beg.SetptOrgin(ptCurr);

		if (ptEnd.Distance(ptTmpEnd) < 10) // 终点才有端部设置
		{
			tmpendTypes.end = endTypes.end;
		}
		tmpendTypes.end.SetptOrgin(ptTmpEnd);

		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(ptTmpEnd);
		vex->SetType(RebarVertex::kEnd);

		rebar.EvaluateEndTypes(tmpendTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebars.push_back(rebar);

		ptCurr = ptTmpEnd;
	}

	return true;
}


WallRebarAssemblyNew::ElementType WallRebarAssemblyNew::JudgeElementType(ElementHandleCR eh)
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
		if (elemInst->GetClass().GetDisplayLabel() == L"PDMS_Attributes")//如果有PDMS_Attributes的EC属性，读取TYPE值
		{
			ECN::ECValue ecVal;
			elemInst->GetValue(ecVal, L"TYPE");
			char tType[1024];
			ecVal.ToString().ConvertToLocaleChars(tType);
			htype = tType;
			if (htype=="FLOOR")
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

WallRebarAssemblyNew::WallType WallRebarAssemblyNew::JudgeWallType(ElementHandleCR eh)
{
//	return STWALL;
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

	// 椭圆（含圆形）墙类型类型
	double dHeight = 0.0;
	EditElementHandle eehDownFace;
	ExtractFacesTool::GetDownFace(Eleeh, eehDownFace, &dHeight);
	if (eehDownFace.GetElementType() == ELLIPSE_ELM)
	{
		return ELLIPSEWall;
	}
	////////////////////////////////////////////////////////////

	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine, vecDownBackLine,NULL);
	
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
	if (arcnum>=2)
	{
		isarcwall = true;
	}
	if (isarcwall == true)//只要有弧线就判断为弧形墙。
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

bool WallRebarAssemblyNew::IsSmartSmartFeature(EditElementHandle& eeh)
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
				eeh.AddToModel();
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}

		}
		return true;
	}
}

bool WallRebarAssemblyNew::IsWallSolid(ElementHandleCR eh)
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

bool STWallRebarAssemblyNew::makeRebarCurve
(
	vector<PITRebarCurve>&     rebars,
	double                  xPos,
	double                  yLen,
	double					startOffset,
	double					endOffset,
	PITRebarEndTypes&		endTypes,
	CMatrix3D const&        mat,
	double					diameter,
	bool isStrLineCut,
	bool istwin
)
{

	CPoint3D  startPt;
	CPoint3D  endPt;

	//不允许为负值
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
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改
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
	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}

	if (!istwin)//非并筋时才需要存储点信息
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
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1],dSideCover, matrix);

	/*EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
	eehline.AddToModel();*/

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt:tmppts)
	{
		if (ExtractFacesTool::IsPointInLine(pt, pt1[0], pt1[1], ACTIVEMODEL, isStr))
		{
			int dis = (int)pt1[0].Distance(pt);		
			if (map_pts.find(dis)!=map_pts.end())
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
		itrplus++;	// 下一个点
		if (itrplus == map_pts.end())
		{
			break;
		}

		vector<double> vecSplit;
		CalaLineRebarCutPoint(itr->second, itrplus->second, vecSplit, diameter * 35.0, 0.0, isStrLineCut);
		//// 从终点往后切割
		//else
		//{
		//	CalaLineRebarCutPoint(itrplus->second, itr->second, vecSplit, 0.0);

		//	reverse(vecSplit.begin(), vecSplit.end());

		//	double dRebarLength = itr->second.Distance(itrplus->second);
		//	for (unsigned int i = 0; i < vecSplit.size(); i++)
		//	{
		//		vecSplit.at(i) = dRebarLength - vecSplit.at(i);
		//	}
		//}
		// end
		CutLineRebarCurve(rebars, endTypes, itr->second, itrplus->second, vecSplit);
	}
	//rebar.DoMatrix(mat);
	return true;
}


bool STWallRebarAssemblyNew::makeRebarCurve_Laption
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
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改


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

RebarSetTag* STWallRebarAssemblyNew::MakeRebars
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
	BrString twinsizekey = XmlManager::s_alltypes[twinBarInfo.rebarType];
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
		startbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeStart, modelRef);	//乘以了100
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
		startbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeStart, modelRef);	//乘以了100
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
		startbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeStart, modelRef);	//乘以了100
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
		endbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeEnd, modelRef);	//乘以了100
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
		endbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeEnd, modelRef);	//乘以了100
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
		endbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeEnd, modelRef);	//乘以了100
	}

		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double diameterTb = RebarCode::GetBarDiameter(twinsizekey, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinsizekey, modelRef, false);	//乘以了30
	double adjustedXLen, adjustedSpacing;

	double leftSideCov, rightSideCov, allSideCov;
	if (startOffset >0||endOffset>0)
	{
		leftSideCov = GetSideCover()*uor_per_mm / sin(m_angle_left);
		rightSideCov = GetSideCover()*uor_per_mm / sin(m_angle_right);
		
	}
	else
	{
		leftSideCov = GetSideCover()*uor_per_mm ;
		rightSideCov = GetSideCover()*uor_per_mm;
	}
	allSideCov = leftSideCov + rightSideCov;
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

	SetCutLenIndex();
	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurves;
//		RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
		bool isStrLineCut = true;
		if (i & 0x1)
		{
			isStrLineCut = false;
		}
	
		makeRebarCurve(rebarCurves, xPos, height - allSideCov, endTypeStartOffset, endTypEendOffset, endTypes, mat, diameter, isStrLineCut, bTwinbarLevel);

		xPos += adjustedSpacing;

		rebarCurvesNum.insert(rebarCurvesNum.end(),rebarCurves.begin(), rebarCurves.end());
	}
	
	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinsizekey,symb);
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}
	else
	{
		string str(sizeKey);
		char ccolar[20] = {0};
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}

	vector<DSegment3d> vecStartEnd;

	
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

		vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));
		/*EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.AddToModel();
	*/
		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDownNew)
		{//若不是预览状态下则生成钢筋
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinsizekey.Get()));
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
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype,ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
		j++;
	}


	m_vecRebarStartEnd.push_back(vecStartEnd);
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

RebarSetTag* STWallRebarAssemblyNew::MakeRebars_Laption
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	double              xLen,
	double              height,
	double              spacing,
	double				startOffset,
	double				endOffset,
	LapOptions const& lapOptions,
	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
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
	if (0 == lapOptions.connectMethod)	//机械连接
	{
		endTypeStart.SetType(RebarEndType::kTerminator);
		endTypeEnd.SetType(RebarEndType::kTerminator);
	}
	else	//非机械连接，使用端部样式
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
		case 0:	//无
		case 1:	//弯曲
		case 2:	//吊钩
		case 3:	//折线
			endTypeStart.SetType(RebarEndType::kNone);
			break;
		case 7:	//直锚
			endTypeStart.SetType(RebarEndType::kLap);
			break;
		case 4:	//90度弯钩
			endTypeStart.SetType(RebarEndType::kBend);
			break;
		case 5:	//135度弯钩
			endTypeStart.SetType(RebarEndType::kCog);
			break;
		case 6:	//180度弯钩
			endTypeStart.SetType(RebarEndType::kHook);
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
			break;
		case 4:	//90度弯钩
			endTypeEnd.SetType(RebarEndType::kBend);
			break;
		case 5:	//135度弯钩
			endTypeEnd.SetType(RebarEndType::kCog);
			break;
		case 6:	//180度弯钩
			endTypeEnd.SetType(RebarEndType::kHook);
			break;
		case 8:	//用户
			endTypeEnd.SetType(RebarEndType::kCustom);
			break;
		default:
			break;
		}
	}
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
	double bendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
	double leftSideCov = GetSideCover()*uor_per_mm / sin(m_angle_left);
	double rightSideCov = GetSideCover()*uor_per_mm / sin(m_angle_right);
	double allSideCov = leftSideCov + rightSideCov;

	double adjustedXLen = xLen - allSideCov - diameter - startOffset - endOffset;
	int numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;

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
		if (!PreviewButtonDownNew)
		{//若不是预览状态下则生成钢筋
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
				xPos -= diameter;	//删除上次并筋偏移距离
		}
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

void STWallRebarAssemblyNew::CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef)
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
	BrString strTieRebarSize(XmlManager::s_alltypes[GetTieRebarInfo().rebarType]);
	//if (strTieRebarSize != L""&& 0 != GetTieRebarInfo().tieRebarMethod)
	//{
	//	if (strTieRebarSize.Find(L"mm") != -1)
	//	{
	//		strTieRebarSize.Replace(L"mm", L"");
	//	}
	//	diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//拉筋直径
	//}
	
	double dOffset = dPositiveCover + diameterTie;
	double dOffsetTb = dPositiveCover + diameterTie;
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize =XmlManager::s_alltypes[GetvecRebarType().at(i)];
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}

		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10
		double diameterTb = 0.0;
		if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
		{
			BrString sizekey = XmlManager::s_alltypes[GetvecTwinRebarLevel().at(i).rebarType];
			diameterTb = RebarCode::GetBarDiameter(sizekey, modelRef);		//乘以了10
		}

		if (diameter > BE_TOLERANCE)
		{
			CVector3D	zTrans(0.0, 0.0, 0.0);
			CVector3D	zTransTb;
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

// 			if (0 == i)
// 			{
// 				dOffset += diameter / 2.0;	//偏移首层钢筋半径
// 				zTrans.y = dOffset;
// 		    }
			//if (0 == i)
			//{
			//	dOffset += diameter / 2.0;	//偏移首层钢筋半径
			//	zTrans.y = dOffset;

			//	if (GetvecTwinRebarLevel().at(i).hasTwinbars)
			//	{
			//		if (diameterTb > diameter)//并筋层的钢筋比主筋直径大
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
				
				
				double diameterPre;		//乘以了10
				if (0 == i)
				{
					diameterPre = diameter;
					dOffset += diameter / 2.0;	//偏移首层钢筋半径
					dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm;
				}
				else
				{
					BrString strSizePre = XmlManager::s_alltypes[GetvecRebarType().at(i - 1)];
					diameterPre = RebarCode::GetBarDiameter(strSizePre, modelRef);
					dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm + diameter*0.5 + diameterPre * 0.5;//层间距加上当前钢筋直径
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
				if (COMPARE_VALUES(m_STwallData.width - dOffset, dReverseCover + diameterTie) < 0)		//当前钢筋层已嵌入到了反面保护层中时，实际布置的钢筋层间距就不再使用设置的与上层间距，而是使用保护层进行限制
				{
					zTrans.y = m_STwallData.width - dReverseCover - diameter / 2.0 - diameterTie;
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
							BrString strSize1 = XmlManager::s_alltypes[GetvecRebarType().at(j)];
							diameterPre = RebarCode::GetBarDiameter(strSize1, modelRef);		//乘以了10
							if (COMPARE_VALUES(vTransform[j].y+diameterPre*0.5, compare- diameter*0.5) > 0)	//嵌入了下一根钢筋终
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


void STWallRebarAssemblyNew::CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend,double& lenth, CMatrix3D   mat, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return;
	DPoint3d ptstar = m_STwallData.ptStart;
	DPoint3d ptend  = m_STwallData.ptEnd;

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstar, ptend), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	
	DPoint3d pt1[2];
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改
	if (pt1[0].Distance(ptstar)>pt1[0].Distance(ptend))//确保pt1[0]为起始点
	{
		DPoint3d tmpPt = pt1[0];
		pt1[0] = pt1[1];
		pt1[1] = tmpPt;
	}

	vector<DPoint3d> inpts;
	GetIntersectPointsWithHoles(inpts, m_Negs, ptstar, ptend);
	lenth = pt1[0].Distance(pt1[1]);
	//if (!GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	if (inpts.size()>0)
	{
		/*EditElementHandle eeh1;
		LineHandler::CreateLineElement(eeh1, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
		eeh1.AddToModel();*/
		DPoint3d ptProject1;	//投影点
		mdlVec_projectPointToLine(&ptProject1, NULL, &pt1[0], &m_STwallData.ptStart, &m_STwallData.ptEnd);

		DPoint3d ptProject2;	//投影点
		mdlVec_projectPointToLine(&ptProject2, NULL, &pt1[1], &m_STwallData.ptStart, &m_STwallData.ptEnd);

		misDisstr = ptProject1.Distance(ptstar);

		misDisend = ptProject2.Distance(ptend);
		
	}


	
}


void STWallRebarAssemblyNew::CalculateUseHoles(DgnModelRefP modelRef)
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
				if (m_doorsholes[m_Holeehs.at(j)]!=nullptr)//如果是门洞
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
					PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix,isdoorNeg,m_STwallData.width);
				}
				else
				{
					PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
				}
							
				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
		}
	}
	if (m_useHoleehs.size()>1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}



}


bool STWallRebarAssemblyNew::MakeRebars(DgnModelRefP modelRef)
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
	if (COMPARE_VALUES(dSideCover, m_STwallData.length) >= 0)	//如果侧面保护层大于等于墙的长度
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"侧面保护层大于等于墙的长度,无法创建钢筋层", MessageBoxIconType::Information);
		return false;
	}
	vector<CVector3D> vTrans;
	vector<CVector3D> vTransTb;
	//计算侧面整体偏移量
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
	int setCount = 0;
	vector<PIT::EndType> vecEndType;

	int frontlevel = 0;//前层层号
	int backlevel = 0;//后层层号
	int midlevel = 0;//中间层号
	int allbacklevel = 0;
	//统计下背后总过有多少层
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
		BrString sizekey = XmlManager::s_alltypes[GetvecRebarType().at(i)];
		int tmpLevel;
		if (GetvecDataExchange().at(i)==0)//前面层
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
		if (GetvvecEndType().empty())		//没有设置端部样式，设置默认值
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

		double diameter = RebarCode::GetBarDiameter(sizekey, modelRef) / 2;
		m_vecRebarPtsLayer.clear();
		m_vecTwinRebarPtsLayer.clear();
		m_vecTieRebarPtsLayer.clear();
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
					if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
					{
						endNormal.Negate();
					}
					CVector3D rebarVec = CVector3D::kZaxis;
/*					endNormal = rebarVec.CrossProduct(vec);*/
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}
			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;
			matTb.SetTranslation(vTransTb[i]);
			matTb = GetPlacement() * matTb;
			//绘制并筋--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
			{
				m_isPushTieRebar = false;
				//先绘制非并筋层
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), sizekey, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
					GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}

				m_isPushTieRebar = true;
				//绘制并筋层
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), sizekey, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
					GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), true, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
			else //当前层未设置并筋
			{
				m_isPushTieRebar = true;
				TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), sizekey, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
					GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat, twinRebar, false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
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
			CVector3D	endNormal;	//端部弯钩方向
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
				m_isPushTieRebar = true;
				//先绘制非并筋层
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), sizekey, dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}

				m_isPushTieRebar = false;
				//绘制并筋层
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), sizekey, dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), true, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;

			}
			else //当前层未设置并筋
			{
				m_isPushTieRebar = true;
				TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
				PopvecSetId().push_back(0);
				setCount++;
				tag = MakeRebars(PopvecSetId().back(), sizekey, dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, twinRebar, false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(setCount);
					rsetTags.Add(tag);
				}
			}
			//end
			vecEndType.clear();
		}
		if (m_vecRebarPtsLayer.size()>1)
		{
			for (int m = 0; m < m_vecRebarPtsLayer.size() - 1;m++)
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


	if (PreviewButtonDownNew)//预览按钮按下，则画主筋线
	{
		m_allLines.clear();
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
	}


	//添加拉筋--begin
	vector<vector<DSegment3d>> vctTieRebarLines;//存储所有的拉筋直线信息，用于预览
	if (0 != GetTieRebarInfo().tieRebarMethod/* && (m_vecAllRebarStartEnd.size() >= 4)*/)
	{
		FaceRebarDataArray faceDataArray;
		faceDataArray.posRebarData.HRebarData.rebarSize = GetvecDirSize().at(0);
		faceDataArray.posRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(0);
		faceDataArray.posRebarData.VRebarData.rebarSize = GetvecDirSize().at(1);
		faceDataArray.posRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(1);

		faceDataArray.revRebarData.HRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecDirSize().size() - 1);
		faceDataArray.revRebarData.VRebarData.rebarSize = GetvecDirSize().at(GetvecDirSize().size() - 2);
		faceDataArray.revRebarData.VRebarData.rebarSpacing = GetvecDirSpacing().at(GetvecDirSize().size() - 2);

		vector<vector<DSegment3d> > vecStartEnd;		//只存储1，2层和倒数第1，2层
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
		vecStartEnd.push_back(vvecSeg[vvecSeg.size()-2]);
		vecStartEnd.push_back(vvecSeg[vvecSeg.size()-1]);
		BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
		int	tieRebarStyle = GetTieRebarInfo().tieRebarStyle;
		if (strTieRebarSize.Find(L"mm") != string::npos)
			strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm

		PopvecSetId().push_back(0);
		TieRebarMaker tieRebarMaker(faceDataArray, vecStartEnd, (TieRebarStyle)tieRebarStyle, strTieRebarSize);
		tieRebarMaker.m_CallerId = GetCallerId();
		tieRebarMaker.SetCustomStyle(GetTieRebarInfo().rowInterval, GetTieRebarInfo().colInterval);
		tieRebarMaker.SetModeType(0);
		Transform trans;
		GetPlacement().AssignTo(trans);
		tieRebarMaker.SetTrans(trans);
		vector<EditElementHandle*> vecAllSolid;
		vecAllSolid.insert(vecAllSolid.begin(),m_Holeehs.begin(), m_Holeehs.end());
		vecAllSolid.insert(vecAllSolid.end(), m_Negs.begin(), m_Negs.end());
		tieRebarMaker.SetDownVec(m_STwallData.ptStart, m_STwallData.ptEnd);
		tieRebarMaker.SetHoles(vecAllSolid);
		tieRebarMaker.SetHoleCover(GetSideCover()*uor_per_mm);
		RebarSetTag* tag = tieRebarMaker.MakeRebar(PopvecSetId().at(GetvecSetId().size() - 1), modelRef);
		tieRebarMaker.GetRebarPts(vctTieRebarLines);//取出所有的拉筋直线信息
		if (NULL != tag && (!PreviewButtonDownNew))
		{
			tag->SetBarSetTag(iRebarLevelNum + 1);
			rsetTags.Add(tag);
		}
	}

	if (PreviewButtonDownNew)//预览按钮按下，则画拉筋线
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

long STWallRebarAssemblyNew::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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


bool STWallRebarAssemblyNew::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pWallDoubleRebarDlg = new CWallRebarDlgNew(ehSel, CWnd::FromHandle(MSWIND));
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

bool STWallRebarAssemblyNew::Rebuild()
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


bool STWallRebarAssemblyNew::SetWallData(ElementHandleCR eh)
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
	string name = StringOperator::Convert::WStringToString(strName.c_str());
	//读取数据
//	ComponentDataManage dataManage("C:\\Users\\Power-Itech-LX\\Desktop\\data\\1BFX15--VB-.bim"); // 构件数据管理
	ComponentDataManage dataManage("C:\\Users\\Power-Itech-LX\\Desktop\\1BFX-----CW\\1BFX-----B\\data\\1BFX2078VB-.bim"); // 构件数据管理
	dataManage.GetAllComponentData();

	vector<IComponent*> vecComponent = ComponentDataManage::GetAllComponent();
	for (size_t i = 0; i < vecComponent.size(); i++)
	{
		IComponent *Component = vecComponent[i];
		string strComponentName = Component->GetEleName();
		string strType = Component->GetEleType();
		if (strType == "STWALL")
		{
			STWallComponent* stWall = dynamic_cast<STWallComponent*>(Component);
			if (stWall == NULL)
			{
				continue;
			}
			string strComponentName = stWall->GetEleName();
			if (name == strComponentName)
			{
				pWall = stWall;
			}
		}
	}
	if (pWall == NULL || !eh.IsValid())
	{
		return false;
	}

	double uor_per_mm = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	m_STwallData.length = pWall->GetPtStart().Distance(pWall->GetPtEnd()) * uor_per_mm;
	m_STwallData.width = pWall->GetWidth() * uor_per_mm;
	m_STwallData.height = pWall->GetHeight() * uor_per_mm;

	DPoint3d ptStart = pWall->GetPtStart();
	ptStart.x *= uor_per_mm;
	ptStart.y *= uor_per_mm;
	ptStart.z *= uor_per_mm;
	DPoint3d ptEnd = pWall->GetPtEnd();
	ptEnd.x *= uor_per_mm;
	ptEnd.y *= uor_per_mm;
	ptEnd.z *= uor_per_mm;


	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
	//由于该线为底面中心线，需将原点进行偏移1/2的厚度
	//以yVec的反反向进行偏移1/2的墙厚
	CVector3D  yVecNegate = yVec;
	yVecNegate.Negate();
	yVecNegate.Normalize();
	yVecNegate.ScaleToLength(m_STwallData.width * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	ptStart.Add(yVecNegate);
	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(ptStart, ptEnd);

// 	CVector3D  xVecNegate = xVecNew;
// 	xVecNegate.Normalize();
// 	xVecNegate.ScaleToLength(m_STwallData.length * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
// 	ptStart.Add(xVecNegate);
// 	CVector3D  zVecNegate = CVector3D::kZaxis;
// 	zVecNegate.Normalize();
// 	zVecNegate.ScaleToLength(m_STwallData.height * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
// 	ptStart.Add(zVecNegate);
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
#else
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;
	InitUcsMatrix();
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
#endif
}

bool STWallRebarAssemblyNew::AnalyzingWallGeometricData(ElementHandleCR eh)
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
	EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &m_STwallData.height);
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

	if (vecDownBackLine.size()>1||vecDownFontLine.size()>1)
	{
		GetMaxDownFacePts(vecDownFontLine, vecDownBackLine, pt1, pt2);
	}
	
	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model,m_STwallData.height);
	
	m_STwallData.height = m_STwallData.height*uor_now / uor_ref;
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

	if (Negs.size()>0)//STWALL有斜边
	{
		
		DPoint3d vecBack = pt2[0] - pt2[1];
		DPoint3d vecLeft = pt1[0] - pt2[0];
		DPoint3d vecRight = pt1[1] - pt2[1];

		vecBack.Normalize();
		vecLeft.Normalize();
		vecRight.Normalize();

		m_angle_left = vecBack.AngleTo(vecLeft);
		if (m_angle_left>PI/2)
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
	
	return true;
}

void STWallRebarAssemblyNew::InitUcsMatrix()
{
	DPoint3d ptStart = m_STwallData.ptStart;
	DPoint3d ptEnd = m_STwallData.ptEnd;

	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
//	CVector3D  yVecNegate = yVec;
//	yVecNegate.Negate();
//	yVecNegate.Normalize();
//	yVecNegate.ScaleToLength(m_STwallData.width);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
//	ptStart.Add(yVecNegate);
//	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(ptStart, ptEnd);
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	/*DPoint3d vecY = yVec;
	vecY.Normalize();
	vecY.Scale(m_STwallData.width / 5);
	ptStart.Add(vecY);
	ptEnd.Add(vecY);*/
	PopvecFrontPts().push_back(ptStart);
	PopvecFrontPts().push_back(ptEnd);
}

bool GWallRebarAssemblyNew::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;
	//InitUcsMatrix();
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}

void GWallRebarAssemblyNew::JudgeGWallType(ElementHandleCR eh)
{
	//通过分解元素来判断元素的类型
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

	if (iArcFlag >= 2 && iLineFlag > 2)	//底面有至少2条弧且有2条以上线段
		m_GWallType = LineAndArcWALL;
	else if (!iArcFlag && iLineFlag >= 4)	//底面全部是线段
	{
		m_GWallType = LineWall;
		for (size_t i = 0; i < vecDownFrontLine.size(); i++)
		{
			DPoint3d pt1[2], pt2[2];
			mdlLinear_extract(pt1, NULL, &vecDownFrontLine[i]->el, eh.GetModelRef());
			mdlLinear_extract(pt2, NULL, &vecDownBackLine[i]->el, eh.GetModelRef());
			//判断是否平行
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
//		return false;	//暂时不处理自定义类型墙
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
//// 				mdlDialog_dmsgsPrint(L"GTWALL折线墙正反面底边长度不一样");
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
//		//将反面的点逆序，使之与正面的点一一对应。
////		std::reverse(m_GWallData.vecReversePt.begin(), m_GWallData.vecReversePt.end());
//
//		//将正面的底边的起点投影到反面线上，计算起点与投影点的距离即为墙厚度
//		DPoint3d ptProject1;	//投影点
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

bool GWallRebarAssemblyNew::AnalyzingWallGeometricData(ElementHandleCR eh)
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
		return false;	//暂时不处理自定义类型墙

	if (vecDownFrontLine.size()!=vecDownBackLine.size())//顶点数要求一致
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
		if (backlenth<frontlenth)
		{
			vector<vector<DPoint3d>>  tmppts;
			tmppts = allfrontpts;
			allfrontpts = allbackpts;
			allbackpts = tmppts;
			std::reverse(allbackpts.begin(), allbackpts.end());
			std::reverse(allfrontpts.begin(), allfrontpts.end());
		}

		for (int i=0;i<allfrontpts.size();i++)
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


void GWallRebarAssemblyNew::CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	vTransform.clear();
	vTransformTb.clear();
	double dSideCover = m_sidecover *uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double dLevelSpace = 0;
	double dOffset = dPositiveCover;
	double dOffsetTb = dPositiveCover;
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		BrString sizekey = XmlManager::s_alltypes[GetvecRebarType().at(i)];
		BrString sizekey2 = XmlManager::s_alltypes[GetvecTwinRebarLevel().at(i).rebarType];
		double diameter = RebarCode::GetBarDiameter(sizekey, modelRef);		//乘以了10
		double diameterTb = 0.0;
		if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
		{
			diameterTb = RebarCode::GetBarDiameter(sizekey2, modelRef);		//乘以了10
		}

		if (diameter > BE_TOLERANCE)
		{
			CVector3D	zTrans(0.0, 0.0, 0.0);
			CVector3D	zTransTb;
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
				BrString strSizePre;
				if (i != 0)
				{
					strSizePre = XmlManager::s_alltypes[GetvecTwinRebarLevel().at(i-1).rebarType];
					
				}

				double diameterPre = RebarCode::GetBarDiameter(strSizePre, modelRef);		//乘以了10
				if (0 == i)
				{
					dOffset += diameter / 2.0;	//偏移首层钢筋半径
					dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm;
				}
				else
				{
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
							WString strSize1 = GetvecDirSize().at(j);
							if (strSize1.find(L"mm") != WString::npos)
							{
								strSize1.ReplaceAll(L"mm", L"");
							}
							diameterPre = RebarCode::GetBarDiameter(strSize1, modelRef);		//乘以了10
							if (COMPARE_VALUES(vTransform[j].y + diameterPre * 0.5, compare - diameter * 0.5) > 0)	//嵌入了下一根钢筋终
							{
								vTransform[j].y -= reverseOffset;
								vTransformTb[j].y = vTransform[j].y;
								if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(j).hasTwinbars)
								{
									BrString strSizePre2 = XmlManager::s_alltypes[GetvecTwinRebarLevel().at(j).rebarType];
									double diameterTbPre = RebarCode::GetBarDiameter(strSizePre2, modelRef);		//乘以了10

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

bool GWallRebarAssemblyNew::makeLineWallRebarCurve(RebarCurve & rebar, int dir, vector<CPoint3D> const& vecRebarVertex, double bendRadius, double bendLen, RebarEndTypes const & endTypes, CVector3D const & endNormal, CMatrix3D const & mat)
{
	RebarVertexP vex;
	CPoint3D  startPt;
	CPoint3D  endPt;

	if (0 == dir)	//横向钢筋
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
	else			//纵向钢筋
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

RebarSetTag * GWallRebarAssemblyNew::MakeRebars_Transverse
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
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
	double bendLen = RebarCode::GetBendLength(sizeKey, endType, modelRef);	//乘以了100

	double adjustedXLen = m_GWallData.height;

	adjustedXLen -= (2.0 * m_sidecover*uor_per_mm + diameter);

	int numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;

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
		if (!PreviewButtonDownNew)
		{//若不是预览状态下则生成钢筋
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
				xPos -= diameter;	//删除上次并筋偏移距离
		}
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

RebarSetTag * GWallRebarAssemblyNew::MakeRebars_Longitudinal(ElementId& rebarSetId, BrStringCR sizeKey, double &xDir, const vector<double> height, double spacing, double startOffset, double endOffset, CVector3D const& endNormal, CMatrix3D const& mat, bool bTwinBars, DgnModelRefP modelRef)
{
	return nullptr;
}
void GWallRebarAssemblyNew::GetMaxThickness(DgnModelRefP modelRef,double& thickness)
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
		DPoint3d BackStr;	//投影点
		mdlVec_projectPointToLine(&BackStr, NULL, &FrontStr, &m_vecLinePts[i].at(2), &m_vecLinePts[i].at(3));
		DPoint3d BackEnd;	//投影点
		mdlVec_projectPointToLine(&BackEnd, NULL, &FrontEnd, &m_vecLinePts[i].at(2), &m_vecLinePts[i].at(3));
		double tmpth = FrontStr.Distance(BackStr)*uor_per_mm / uor_ref;
		if (i==0)
		{
			thickness = tmpth;
		}
		else if (thickness< tmpth)
		{
			thickness = tmpth;
		}
	}
}

bool GWallRebarAssemblyNew::GetUcsAndStartEndData(int index,double thickness, DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = GetSelectedModel()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	DPoint3d FrontStr = m_vecLinePts[index].at(0);
	DPoint3d FrontEnd = m_vecLinePts[index].at(1);

	if (thickness>= MaxWallThickness * uor_per_mm &&(index+1)<m_vecLinePts.size())//重新计算终点
	{
		if (m_vecLinePts[index+1].size()==4)
		{
			DPoint3d BackStr;	//投影点
			mdlVec_projectPointToLine(&BackStr, NULL, &FrontStr, &m_vecLinePts[index].at(2), &m_vecLinePts[index].at(3));
			DVec3d   vec1 = BackStr - FrontStr;
			vec1.Normalize();
 
			DPoint3d FrontStr2 = m_vecLinePts[index+1].at(0);
			DPoint3d FrontEnd2 = m_vecLinePts[index+1].at(1);
			DPoint3d BackStr2;	//第二条线的投影点
			mdlVec_projectPointToLine(&BackStr2, NULL, &FrontStr2, &m_vecLinePts[index+1].at(2), &m_vecLinePts[index+1].at(3));
			DVec3d   vec2 = BackStr2 - FrontStr2;
			vec2.Normalize();

			double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(0), modelRef);//取最前面一层的钢筋直径
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
			mdlVec_intersect(&intercept, &L1,&L2);


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
			IntersectionPointToArcData(data, intercept, tmpPt1, tmpPt2,diameter/2);

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
			mdlVec_projectPointToLine(&tmpPt2, NULL, &tmpPt2, &m_vecLinePts[index+1].at(0), &m_vecLinePts[index+1].at(1));

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
	 FrontStr = m_vecLinePts[index].at(0);
	 FrontEnd = m_vecLinePts[index].at(1);

	DPoint3d BackStr;	//投影点
	mdlVec_projectPointToLine(&BackStr, NULL, &FrontStr, &m_vecLinePts[index].at(2), &m_vecLinePts[index].at(3));
	DPoint3d BackEnd;	//投影点
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


	//if (Negs.size() > 0)//STWALL有斜边
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
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);
	if (index ==0)
	{
		PopvecFrontPts().push_back(ptStart);
		PopvecFrontPts().push_back(ptEnd);
	}
	else
	{
		PopvecFrontPts().push_back(ptEnd);
	}
	
	return true;
}

bool GWallRebarAssemblyNew::MakeRebars(DgnModelRefP modelRef)
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

	for (int k = 0;k<m_vecLinePts.size();k++)
	{
		
		if (m_vecLinePts[k].size()!=4)
		{
			continue;
		}
		double  sidecover = GetSideCover();//取得侧面保护层
		m_sidecover = GetSideCover();

		GetUcsAndStartEndData(k, thickness, modelRef);
		m_useHoleehs.clear();
		CalculateUseHoles(modelRef);
		
		if (thickness >= MaxWallThickness*uor_per_mm)
		{
			//SetSideCover(0);//先将侧面保护层设置为0			
		}
		CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
		double dLevelSpace = 0;
		double dSideCover = m_sidecover *uor_per_mm;
		if (COMPARE_VALUES(dSideCover, m_STwallData.width) >= 0)	//如果侧面保护层大于等于墙的长度
		{
			mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"侧面保护层大于等于墙的长度,无法创建钢筋层", MessageBoxIconType::Information);
			return false;
		}
		
		vector<CVector3D> vTrans;
		vector<CVector3D> vTransTb;
		//计算侧面整体偏移量
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
			BrString sizekey = XmlManager::s_alltypes[GetvecRebarType().at(i)];
			//		int iActualIndex = i;
			RebarSetTag* tag = NULL;
			CMatrix3D   mat, matTb;

			vector<PIT::EndType> vecEndType;
			if (GetvvecEndType().empty())		//没有设置端部样式，设置默认值
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
			double diameter = RebarCode::GetBarDiameter(sizekey, modelRef) / 2;
			m_vecRebarPtsLayer.clear();
			m_vecTwinRebarPtsLayer.clear();
			m_vecTieRebarPtsLayer.clear();
			if (GetvecDir().at(i) == 1)	//纵向钢筋
			{
				bool drawlast = true;
				if (i<=1&&thickness>= MaxWallThickness * uor_per_mm&&k!= m_vecLinePts.size()-1)//板厚大于600，并且是第一次画点筋，并且不是最后一段墙的配筋，最后一根点筋不绘制
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

				//if (k==0&&thickness>=10)//第一段
				//{
				//	misDisH_left = misDisH_left + sidecover*uor_per_mm;
				//}
				//else if (k== m_vecLinePts.size() - 1 && thickness >= 10)
				//{
				//	misDisH_right = misDisH_right + sidecover * uor_per_mm;
				//}


				vector<CVector3D> vecEndNormal(2);
				CVector3D	endNormal;	//端部弯钩方向
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
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
						vecEndNormal[k] = endNormal;
					}
				}
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;
				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
				//绘制并筋--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
				{
					m_isPushTieRebar = false;
					//先绘制非并筋层
					tag = MakeRebars(PopvecSetId().at(i), sizekey, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i),false, modelRef, drawlast);
					if (NULL != tag && (!PreviewButtonDownNew))
					{
						tag->SetBarSetTag(i + 1+ numtag);
						rsetTags.Add(tag);
					}

					m_isPushTieRebar = true;
					//绘制并筋层
					tag = MakeRebars(PopvecSetId().at(iTwinbarSetIdIndex + iRebarLevelNum), sizekey, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true, modelRef, drawlast);
					if (NULL != tag && (!PreviewButtonDownNew))
					{
						tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1 + numtag);
						rsetTags.Add(tag);
					}
					iTwinbarSetIdIndex++;
				}
				else //当前层未设置并筋
				{
					m_isPushTieRebar = true;
					TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
					tag = MakeRebars(PopvecSetId().at(i), sizekey, dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat, twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef,drawlast);
					if (NULL != tag && (!PreviewButtonDownNew))
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
				double leftSideCov = m_sidecover *uor_per_mm / sin(m_angle_left);
				double rightSideCov = m_sidecover *uor_per_mm / sin(m_angle_right);

				//if (k == 0 && thickness >= 10)//第一段
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
				CVector3D	endNormal;	//端部弯钩方向
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
					m_isPushTieRebar = true;
					//先绘制非并筋层
					tag = MakeRebars(PopvecSetId().at(i), sizekey, dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
					if (NULL != tag && (!PreviewButtonDownNew))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);
					}

					m_isPushTieRebar = false;
					//绘制并筋层
					tag = MakeRebars(PopvecSetId().at(iTwinbarSetIdIndex + iRebarLevelNum), sizekey, dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true, modelRef);
					if (NULL != tag && (!PreviewButtonDownNew))
					{
						tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1 + numtag);
						rsetTags.Add(tag);
					}
					iTwinbarSetIdIndex++;

				}
				else //当前层未设置并筋
				{
					m_isPushTieRebar = true;
					TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
					tag = MakeRebars(PopvecSetId().at(i), sizekey, dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
					if (NULL != tag && (!PreviewButtonDownNew))
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

		////添加拉筋--begin
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

		//	vector<vector<pair<CPoint3D, CPoint3D> > > vecStartEnd;		//只存储1，2层和倒数第1，2层
		//	vecStartEnd.push_back(m_vecAllRebarStartEnd[0]);
		//	vecStartEnd.push_back(m_vecAllRebarStartEnd[1]);
		//	vecStartEnd.push_back(m_vecAllRebarStartEnd[m_vecAllRebarStartEnd.size() - 1]);
		//	vecStartEnd.push_back(m_vecAllRebarStartEnd[m_vecAllRebarStartEnd.size() - 2]);
		//	BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
		//	int	tieRebarStyle = GetTieRebarInfo().tieRebarStyle;
		//	if (strTieRebarSize.Find(L"mm") != string::npos)
		//		strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm
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

	if (PreviewButtonDownNew)//预览按钮按下，则画主筋线
	{
		m_allLines.clear();
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

long GWallRebarAssemblyNew::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

void GWallRebarAssemblyNew::CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef)
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


RebarSetTag* GWallRebarAssemblyNew::MakeRebars
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
	int level,
	int grade,
	int DataExchange,
	bool				bTwinbarLevel,
	DgnModelRefP        modelRef,
	bool  drawlast
)
{
	BrString twinsizekey = XmlManager::s_alltypes[twinBarInfo.rebarType];
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
		startbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeEnd, modelRef);	//乘以了100
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
		startbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeEnd, modelRef);	//乘以了100
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
		startbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeEnd, modelRef);	//乘以了100
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
		endbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeEnd, modelRef);	//乘以了100
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
		endbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeEnd, modelRef);	//乘以了100
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
		endbendLenTb = RebarCode::GetBendLength(twinsizekey, endTypeEnd, modelRef);	//乘以了100
	}

	break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double diameterTb = RebarCode::GetBarDiameter(twinsizekey, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinsizekey, modelRef, false);	//乘以了30
	double adjustedXLen, adjustedSpacing;

	double leftSideCov = GetSideCover()*uor_per_mm / sin(m_angle_left);
	double rightSideCov = GetSideCover() *uor_per_mm / sin(m_angle_right);
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
	if (endType[0].endType != 0)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	if (bTwinbarLevel)				//并筋层
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

	for (int i = 0; i < numRebar; i++)
	{
		if (!drawlast&&i == numRebar - 1)
		{
			continue;
		}
		vector<PITRebarCurve>     rebarCurves;
		//		RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
		makeRebarCurve(rebarCurves, xPos, height - allSideCov, endTypeStartOffset, endTypEendOffset, endTypes, mat,m_IsTwinrebar);

		xPos += adjustedSpacing;

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinsizekey, symb);
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
	vector<DSegment3d> vecStartEnd;
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

		vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));
		/*	EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			eeh.AddToModel();*/

		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDownNew)
		{//若不是预览状态下则生成钢筋
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinsizekey.Get()));
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
	}

	m_vecAllRebarStartEnd.push_back(vecStartEnd);
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

bool GWallRebarAssemblyNew::makeRebarCurve
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

	//不允许为负值
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
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改

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
	double dSideCover = m_sidecover *uor_per_mm;
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

void GWallRebarAssemblyNew::CalculateUseHoles(DgnModelRefP modelRef)
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
			bool isdoorNeg = false;//判断是否为门洞NEG
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
			if (interpts.size()>0)
			{
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
void ArcWallRebarAssemblyNew::CalculateUseHoles(DgnModelRefP modelRef)
{
	m_useHoleehs.clear();
	//m_useHoleehs.insert(m_useHoleehs.begin(), m_Holeehs.begin(), m_Holeehs.end());

	
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetSideCover()*uor_per_mm;

	

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
			DPoint3d ptcenter = m_ArcWallData.InnerArc.ptCenter;
			DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());

			ptcenter.z = ptele.z;
			CVector3D yVec = ptcenter - ptele;
			yVec.Normalize();

			CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

			DPoint3d ptStart = ptcenter;
			BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴
			
			Transform trans;
			placement.AssignTo(trans);
			Transform intrans = trans;
			intrans.InverseOf(intrans);

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
long ArcWallRebarAssemblyNew::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool ArcWallRebarAssemblyNew::CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
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

bool ArcWallRebarAssemblyNew::makeLineRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::ArcSegment arcSeg, double dLen, double space, double startOffset, double endOffset, PIT::PITRebarEndTypes & endTypes)
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

bool ArcWallRebarAssemblyNew::makeArcWallRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::ArcSegment arcSeg,double space,double startOffset,double endOffset, PIT::PITRebarEndTypes& endTypes)
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
	
	
	if (pts.size()>0)
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
	dislenth =0;
	mdlElmdscr_distanceAtPoint(&dislenth, nullptr, nullptr, arceeh.GetElementDescrP(), &ptArcEnd, 0.1);
	for (DPoint3d pt : pts)
	{
		double dis1;
		dis1 = 0;
		mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, arceeh.GetElementDescrP(), &pt, 0.1);
		if (dis1>10 && dis1<= dislenth +10)
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
		DPoint3d tmpMid,tmpstr,tmpend;
		
		tmpstr = itr->second;
		map<int, DPoint3d>::iterator itrplus =++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		tmpend = itrplus->second;
		double dis2;
		dis2= 0;
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


bool ArcWallRebarAssemblyNew::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pArcWallDoubleRebarDlg = new CWallRebarDlgNew(ehSel, CWnd::FromHandle(MSWIND));
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

bool ArcWallRebarAssemblyNew::Rebuild()
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

bool ArcWallRebarAssemblyNew::AnalyzingWallGeometricData(ElementHandleCR eh)
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
				if (j==0)
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
	PIT::ArcSegment arcFront, arcBack;
	{//计算外弧改变之前的起点和终点
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
		//开始角度延扫描角度反方向移动，扫描角度加大；
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
		if (starangleF < starangleB)//外弧取扫掠角度最大时的弧，和内弧比较时
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
		if (starangleF > starangleB)//外弧取扫掠角度最大时的弧，和内弧比较时
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
	//开始角度延扫描角度反方向移动，扫描角度加大；
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
	//开始角度延扫描角度反方向移动，扫描角度加大；
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

	//外弧计算
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

bool ArcWallRebarAssemblyNew::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}
bool ArcWallRebarAssemblyNew::MakeRebars(DgnModelRefP modelRef)
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
		strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm
	double diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//拉筋直径
	double newRadius = m_ArcWallData.OuterArc.dRadius - GetPositiveCover() * uor_per_mm - diameterTie;
	double diameterPre = 0.0;

	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		BrString sizekey = XmlManager::s_alltypes[GetvecRebarType().at(i)];
		m_vecRebarPtsLayer.clear();
		m_vecTieRebarPtsLayer.clear();
		RebarSetTag* tag = NULL;
		vector<PIT::EndType> vecEndType;
		if (GetvvecEndType().empty())		//没有设置端部样式，设置默认值
		{
			EndType endType;
			memset(&endType, 0, sizeof(endType));
			vecEndType = { { 0,0,0 },{0,0,0} ,endType };
		}
		else
		{
			vecEndType = GetvvecEndType().at(i);
		}

		double diameter = RebarCode::GetBarDiameter(sizekey, modelRef);	//当前层钢筋直径

		PIT::ArcSegment newArc = m_ArcWallData.OuterArc;
		if (0 == i)	//首层偏移当前钢筋直径
		{
			newRadius -= diameter * 0.5;
		}
		else
		{
			diameterPre = RebarCode::GetBarDiameter(GetvecDirSize().at(i - 1), modelRef);	//上一层钢筋直径
			newRadius -= diameterPre * 0.5;	//偏移上一层钢筋的半径
			newRadius -= diameter * 0.5;	//偏移当前层钢筋的半径
		}

		newRadius -= GetvecLevelSpace().at(i) * uor_per_mm;	//偏移层间距
		double offset = m_ArcWallData.OuterArc.dRadius - newRadius;	//总偏移长度
		double reverseCover = GetReverseCover()*uor_per_mm + diameter * 0.5 + diameterTie;	//反面实际需预留宽度
		if (offset > m_ArcWallData.thickness - reverseCover)	//偏移出了墙
		{
			double reverseOffset = 0.0;
			for (int j = iRebarLevelNum - 1; j > i; --j)
			{
				double diameterLater = RebarCode::GetBarDiameter(GetvecDirSize().at(j), modelRef);	//后续层钢筋直径
				reverseOffset += diameter;
			}
			offset = m_ArcWallData.thickness - reverseCover - reverseOffset;
			newRadius = m_ArcWallData.OuterArc.dRadius - offset;
		}
		newArc.ScaleToRadius(newRadius);		//整体缩放
		newArc.CutArc(m_ArcWallData.OuterArc.FptStart, m_ArcWallData.InnerArc.ptEnd);
		newArc.CutArc(m_ArcWallData.OuterArc.FptEnd, m_ArcWallData.InnerArc.ptStart);

		//PopvecSetId().push_back(0);
		vector<CVector3D> vecEndNormal(2);
		if (0 == GetvecDir().at(i))	//水平弧形钢筋
		{
			CVector3D	endNormal;	//端部弯钩方向
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
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}
			newArc.Shorten(dSideCover, false);		//起点缩短
			newArc.Shorten(dSideCover, true);		//终点缩短
			newArc.OffsetByAxisZ(diameter * 0.5 + GetSideCover()*uor_per_mm + diameterTie);
			//绘制并筋--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
			{
				m_isPushTieRebar = true;
				//先绘制非并筋层
				PopvecSetId().push_back(0);
				tag = MakeRebars_Arc(PopvecSetId().back(), sizekey, newArc, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}

				m_isPushTieRebar = false;
				PopvecSetId().push_back(0);
				//绘制并筋层
				tag = MakeRebars_Arc(PopvecSetId().back(), sizekey, newArc, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true, modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
			else //当前层未设置并筋
			{
				m_isPushTieRebar = true;
				TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
				PopvecSetId().push_back(0);
				tag = MakeRebars_Arc(PopvecSetId().back(), sizekey, newArc, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i),false, modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}
			}
			vecEndType.clear();
 		}
 		else
		{
			CVector3D	endNormal;	//端部弯钩方向
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
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}

			newArc.Shorten(dSideCover + diameter * 0.5, false);		//起点缩短
			newArc.Shorten(dSideCover + diameter * 0.5, true);		//终点缩短
			newArc.OffsetByAxisZ(GetSideCover()*uor_per_mm + diameterTie);
			double dLen = m_ArcWallData.height - (GetSideCover()*uor_per_mm + diameterTie)*2;
			//绘制并筋--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
			{
				m_isPushTieRebar = false;
				//先绘制非并筋层
				PopvecSetId().push_back(0);
				tag = MakeRebars_Line(PopvecSetId().back(), sizekey, newArc, dLen, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i),false, modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}

				m_isPushTieRebar = true;
				//绘制并筋层
				PopvecSetId().push_back(0);
				tag = MakeRebars_Line(PopvecSetId().back(), sizekey, newArc, dLen, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true, modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
			else //当前层未设置并筋
			{
				m_isPushTieRebar = true;
				TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
				PopvecSetId().push_back(0);
				tag = MakeRebars_Line(PopvecSetId().back(), sizekey, newArc,dLen, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
					GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), false, modelRef);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(i + 1);
					rsetTags.Add(tag);
				}
			}
			vecEndType.clear();
		}
		if (m_vecRebarPtsLayer.size() > 1)
		{
			if (GetvecDir().at(i) == 0)//X轴时，存储了三个点
			{
				for (int m = 0; m < m_vecRebarPtsLayer.size() - 2; m++)
				{
					int n = m + 1;
					int k = n + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
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
			if (GetvecDir().at(i) == 0)//X轴时，存储了三个点
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



	//添加拉筋--begin
	vector<vector<DSegment3d>> vctTieRebarLines;//存储所有的拉筋直线信息，用于预览
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

		vector<vector<RebarPoint> > vecStartEnd;		//只存储1，2层和倒数第1，2层
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
			strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm

		PopvecSetId().push_back(0);

		vector<vector<DSegment3d> > vecStartEndTmp;		//只存储1，2层和倒数第1，2层
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
		tieRebarMaker.GetRebarPts(vctTieRebarLines);//取出所有的拉筋直线信息
		if (NULL != tag && (!PreviewButtonDownNew))
		{
			tag->SetBarSetTag(iRebarLevelNum + 1);
			rsetTags.Add(tag);
		}
	}

	if (PreviewButtonDownNew)//预览按钮按下，则画主筋线
	{
		m_allLines.clear();
		for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)
		{
			vector<DSegment3d> vcttemp(*it);
			for (int x = 0; x < vcttemp.size(); x++)
			{
				DPoint3d strPoint = DPoint3d::From(vcttemp[x].point[0].x, vcttemp[x].point[0].y, vcttemp[x].point[0].z);
				DPoint3d endPoint = DPoint3d::From(vcttemp[x].point[1].x, vcttemp[x].point[1].y, vcttemp[x].point[1].z);
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


RebarSetTag* ArcWallRebarAssemblyNew::MakeRebars_Line
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	PIT::ArcSegment			arcSeg,
	double              dLen,
	double              spacing,
	double              startOffset,
	double              endOffset,
	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
	TwinBarSet::TwinBarLevelInfo const& twinBarInfo,
	int level,
	int grade,
	int DataExchange,
	bool				bTwinbarLevel,
	DgnModelRefP        modelRef
)
{
	BrString twinsizekey = XmlManager::s_alltypes[twinBarInfo.rebarType];
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
	double diameterTb = RebarCode::GetBarDiameter(twinsizekey, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinsizekey, modelRef, false);	//乘以了30
	double adjustedXLen, adjustedSpacing;

	double sideCov = GetSideCover()*uor_per_mm;
	int numRebar = 0;
	if (twinBarInfo.hasTwinbars)	//并筋
		adjustedXLen = arcSeg.dLen - sideCov - diameter - diameterTb - startOffset - endOffset;
	else
		adjustedXLen = arcSeg.dLen - sideCov - diameter - startOffset - endOffset;
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
		begStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
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
		endStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

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
	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinsizekey, symb);
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

	vector<DSegment3d> vecStartEnd;
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

		vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));
		/*EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.AddToModel();
*/
		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDownNew)
		{//若不是预览状态下则生成钢筋
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinsizekey.Get()));
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
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);
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

RebarSetTag * ArcWallRebarAssemblyNew::MakeRebars_Arc(ElementId & rebarSetId, BrStringCR sizeKey, PIT::ArcSegment arcSeg,double spacing, double startOffset, double endOffset, vector<PIT::EndType> const & endType, vector<CVector3D> const & vecEndNormal, TwinBarSet::TwinBarLevelInfo const & twinBarInfo, int level, int grade, int DataExchange, bool bTwinbarLevel, DgnModelRefP modelRef)
{
	bool const isStirrup = false;
	BrString twinsizekey = XmlManager::s_alltypes[twinBarInfo.rebarType];
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
	double diameterTb = RebarCode::GetBarDiameter(twinsizekey, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinsizekey, modelRef, false);
	double adjustedXLen, adjustedSpacing;

	BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
	if (strTieRebarSize.Find(L"mm") != string::npos)
		strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm
	double diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//拉筋直径

	double sideCov = GetSideCover()*uor_per_mm;
	int numRebar = 0;
	if (twinBarInfo.hasTwinbars)	//并筋
		adjustedXLen = m_ArcWallData.height - sideCov*2 - diameter - diameterTie*2 - diameterTb - startOffset - endOffset;
	else
		adjustedXLen = m_ArcWallData.height - sideCov*2 - diameter - diameterTie*2 - startOffset - endOffset;
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
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

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
		begStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//乘以了100
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
		endStraightAnchorLen = 400.0 * uor_per_mm;
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//乘以了30
		endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//乘以了100
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

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
	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinsizekey, symb);
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

	vector<DSegment3d> vecStartEnd;
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

		vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));
		/*	EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			eeh.AddToModel();
*/
		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDownNew)
		{//若不是预览状态下则生成钢筋
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinsizekey.Get()));
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
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);
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


bool ELLWallRebarAssemblyNew::AnalyzingWallGeometricData(ElementHandleCR eh)
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
	GetDoorHoles(Holeehs, m_doorsholes);

	vector<MSElementDescrP> vecEllipse;
	EFT::GetEllipseDownFace(Eleeh, vecEllipse, Holeehs, &m_ELLWallData.dHeight);

	double minRadius = 0.0;
	for (MSElementDescrP ms : vecEllipse)
	{
		double dRadius = 0.0;
		DPoint3d centerpt;
		DPoint3d ArcDPs[2];

		mdlArc_extract(ArcDPs, NULL, NULL, &dRadius, NULL, NULL, &centerpt, &ms->el);
		mdlElmdscr_freeAll(&ms);

		if (COMPARE_VALUES_EPS(dRadius, m_ELLWallData.dRadiusOut, EPS) > 0) // 最大半径的圆
		{
			m_ELLWallData.dRadiusOut = dRadius;
			m_ELLWallData.ArcDPs[0] = ArcDPs[0];
			m_ELLWallData.ArcDPs[1] = ArcDPs[1];
			m_ELLWallData.centerpt = centerpt;
		}

		if (fabs(minRadius - 0.0) < 10 || COMPARE_VALUES_EPS(minRadius, dRadius, EPS) > 0) // 最小半径的圆
		{
			minRadius = dRadius;
		}
	}
	m_ELLWallData.dRadiusInn = minRadius;

	m_Holeehs = Holeehs;
	return true;
}


bool ELLWallRebarAssemblyNew::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}


void ELLWallRebarAssemblyNew::CalculateUseHoles(DgnModelRefP modelRef)
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
				//if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//如果是门洞
				//{
				//	continue;
				//}
				bool isdoorNeg = false;//判断是否为门洞NEG
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

				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}
}

long ELLWallRebarAssemblyNew::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

// 生成点筋
RebarSetTag* ELLWallRebarAssemblyNew::MakeRebar_Vertical
(
	ElementId& rebarSetId,
	BrString sizeKey,
	DgnModelRefP modelRef,
	double startOffset,  // 起始偏移
	double endOffset,    // 终点偏移
	double spacing,		 // 间距
	double dRoundRadius, // 圆的半径
	double rebarLen,		 // 钢筋长度
	int level,
	int grade,
	int DataExchange,
	bool isTwinRebar // 是否是并筋
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

	vector<PIT::EndType> endType; // 端部样式
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

	vector<CVector3D>  vecEndNormal; // 端部弯钩方向
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

	int numRebar = (int)floor(adjustedXLen / spacing + 0.5);
	adjustedSpacing = spacing;
	if (numRebar > 1)
	{
		adjustedSpacing = adjustedXLen / (numRebar);
	}

	if (numRebar > 1)
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
	if (endType[0].endType != 0 && endType[0].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//端部弯曲时额外偏移钢筋半径
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

	vector<DSegment3d> vecStartEnd;
	for (int j = 0; j < (int)rebarCurvesNum.size(); ++j)
	{
		PITRebarCurve rebarCurve = rebarCurvesNum[j];
		RebarElementP rebarElement = nullptr;
		if (!PreviewButtonDownNew)
		{//若不是预览状态下则生成钢筋
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}

		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);

		vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));

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

	m_vecRebarStartEnd.push_back(vecStartEnd);

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


// 生成圆形钢筋
RebarSetTag* ELLWallRebarAssemblyNew::MakeRebar_Round
(
	ElementId& rebarSetId,
	BrString sizeKey,
	DgnModelRefP modelRef,
	double startOffset, // 起始偏移
	double endOffset,   // 终点偏移
	double spacing,		// 间距
	double dRoundRadius,// 圆的半径
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

	vector<PIT::EndType> endType; // 端部样式
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

	vector<CVector3D>  vecEndNormal; // 端部弯钩方向
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

	double adjustedSpacing = 0.0;
	double adjustedXLen = m_ELLWallData.dHeight - diameter - startOffset - endOffset - GetSideCover() * uor_per_mm * 2;
	int numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;
	adjustedSpacing = spacing;

	if (numRebar > 1)
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
	if (endType[0].endType != 0 && endType[0].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//端部弯曲时额外偏移钢筋半径
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
		// 画圆面
		EditElementHandle eehRound;
		if (SUCCESS != EllipseHandler::CreateEllipseElement(eehRound, NULL, centerPt, dRoundRadius, dRoundRadius, 0, true, *modelRef))
		{
			return false;
		}

		vector<MSElementDescrP> vecTmp;
		vector<DPoint3d> vecArcIntersect;
		EFT::IntersectHoleEllWall(eehRound, centerPt, m_useHoleehs, vecTmp, vecArcIntersect);
		if (vecTmp.size() == 0) // 没有被孔洞切割 -- 圆形
		{
			makeRoundRebarCurve(rebarCurvesNum, endTypes, centerPt, dRoundRadius);
		}
		else // 被孔洞切割成弧形
		{
			makeArcRebarCurve(rebarCurvesNum, vecTmp.at(0), endTypes, vecArcIntersect);
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
		if (!PreviewButtonDownNew)
		{//若不是预览状态下则生成钢筋
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
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet); 
	tag->SetIsStirrup(isStirrup);

	return tag;
}

bool ELLWallRebarAssemblyNew::makeArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP mscArc, PIT::PITRebarEndTypes& endTypes, const vector<DPoint3d>& pts)
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


bool ELLWallRebarAssemblyNew::makeRebarCurve(vector<PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d ptStr, double dRebarLength)
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
		rebarCurvesNum.push_back(rebar);
	}
	//rebar.DoMatrix(mat);
	return true;
}


// 圆形状钢筋
bool ELLWallRebarAssemblyNew::makeRoundRebarCurve(vector<PITRebarCurve>& rebarCurvesNum, PIT::PITRebarEndTypes& endTypes, DPoint3d	centerPoint, double dRoundRadius)
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
	// 画第一段弧
	EditElementHandle eehArc;
	//以ptA为圆心,dRadius为半径，ptIn1为弧的起点，ptIn2为弧的终点画弧
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

	// 画后三段弧
	for (int i = 0; i < 3; i++)
	{
		arcStr = arcEnd;
		arcEnd = centerPoint;

		vec = vec.Perpendicular();
		movePoint(vec, arcEnd, dRoundRadius);

		//以ptA为圆心,dRadius为半径，ptIn1为弧的起点，ptIn2为弧的终点画弧
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


void ELLWallRebarAssemblyNew::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}


bool ELLWallRebarAssemblyNew::CalculateRound(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt, int nStep)
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
		if (nStep == 1) // 圆的首段弧
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

		if (nStep == 2) // 圆的最后一段弧
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


bool ELLWallRebarAssemblyNew::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	CalculateUseHoles(modelRef);

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	int iRebarLevelNum = GetRebarLevelNum();
	vector<PIT::EndType> vecEndType;

	double totalDiameter = 0.00;
	for (BrString sizeKey : GetvecDirSize())
	{
		totalDiameter += RebarCode::GetBarDiameter(sizeKey, modelRef);
	}

	vector<int> vecDataChange = GetvecDataExchange();
	vector<double> vecLevelSpace = GetvecLevelSpace();

	double levelSpacingTol = 0.0; // 已配置的层间距和

	// 计算层偏移距离
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

		// 大于两弧的半径差就不算，只能接受微调
		if (COMPARE_VALUES_EPS(vecLevelSpace.at(i) * uor_per_mm, m_ELLWallData.dRadiusOut - m_ELLWallData.dRadiusInn, EPS) < 0)
		{
			levelSpacingTol += vecLevelSpace.at(i) * uor_per_mm;
		}

	}
	// end

	// 计算正面、背面、中间 的层间距
	double levelSpacing = m_ELLWallData.dRadiusOut - m_ELLWallData.dRadiusInn - GetPositiveCover() * uor_per_mm - GetReverseCover() * uor_per_mm;
	levelSpacing -= totalDiameter - levelSpacingTol;

	for (auto itr = mapLevelSapce.begin(); itr != mapLevelSapce.end(); itr++)
	{
		if (itr->first == 1) // 中间
		{
			if (itr->second.size() > 0)
			{
				itr->second.at(0).LevelSpacing += levelSpacing * 0.5;
				levelSpacing *= 0.5;
			}
		}
		else if (itr->first == 2) // 背面
		{
			if (itr->second.size() > 0)
			{
				itr->second.at(0).LevelSpacing += levelSpacing;
			}
		}
	}

	m_vecArcSeg.clear();
	m_vecRebarStartEnd.clear();

	int iTwinbarSetIdIndex = 0;
	levelSpacing = m_ELLWallData.dRadiusOut - GetPositiveCover() * uor_per_mm;
	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		BrString sizekey = XmlManager::s_alltypes[GetvecRebarType().at(i)];
		BrString sizekeytwin = XmlManager::s_alltypes[GetvecTwinRebarLevel().at(i).rebarType];
		
		double diamter = RebarCode::GetBarDiameter(sizekey, modelRef);

		vector<LevelInfo> vecDis = mapLevelSapce[GetvecDataExchange().at(i)];
		for (LevelInfo info : vecDis)
		{
			if (info.rebarLevel == i)
			{
				levelSpacing -= info.LevelSpacing;
				break;
			}
		}

		levelSpacing -= diamter * 0.5;

		PopvecSetId().push_back(0);
		if (GetvecDir().at(i) == 0)
		{
			RebarSetTag* tag = MakeRebar_Round(GetvecSetId().back(), sizekey, modelRef, 
				GetvecStartOffset().at(i) * uor_per_mm, GetvecEndOffset().at(i) * uor_per_mm, 
				GetvecDirSpacing().at(i) * uor_per_mm, levelSpacing, GetvecRebarLevel().at(i), 
				GetvecRebarType().at(i), GetvecDataExchange().at(i));
			if (NULL != tag && (!PreviewButtonDownNew))
			{
				tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
				rsetTags.Add(tag);
			}

			//绘制并筋--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
			{
				PopvecSetId().push_back(0);

				double diamterTwin = RebarCode::GetBarDiameter(sizekeytwin, modelRef);
				//绘制并筋层
				tag = MakeRebar_Round(GetvecSetId().back(), sizekeytwin, modelRef, 
					GetvecStartOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5, 
					GetvecEndOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5, 
					GetvecDirSpacing().at(i) * uor_per_mm, levelSpacing, GetvecRebarLevel().at(i), 
					GetvecRebarType().at(i), GetvecDataExchange().at(i), true);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
		}
		else
		{
			RebarSetTag* tag = MakeRebar_Vertical(GetvecSetId().at(i), sizekey, modelRef, 
				GetvecStartOffset().at(i) * uor_per_mm, GetvecEndOffset().at(i) * uor_per_mm, 
				GetvecDirSpacing().at(i) * uor_per_mm, levelSpacing, m_ELLWallData.dHeight, GetvecRebarLevel().at(i), 
				GetvecRebarType().at(i), GetvecDataExchange().at(i));
			if (NULL != tag && (!PreviewButtonDownNew))
			{
				tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
				rsetTags.Add(tag);
			}
			//绘制并筋--begin
			if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
			{
				PopvecSetId().push_back(0);

				double diamterTwin = RebarCode::GetBarDiameter(sizekeytwin, modelRef);
				//绘制并筋层
				tag = MakeRebar_Vertical(GetvecSetId().back(), sizekeytwin, modelRef, 
					GetvecStartOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5, 
					GetvecEndOffset().at(i) * uor_per_mm + diamter * 0.5 + diamterTwin * 0.5, 
					GetvecDirSpacing().at(i) * uor_per_mm, levelSpacing, m_ELLWallData.dHeight,
					GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), true);
				if (NULL != tag && (!PreviewButtonDownNew))
				{
					tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1);
					rsetTags.Add(tag);
				}
				iTwinbarSetIdIndex++;
			}
		}

		levelSpacing -= diamter * 0.5;
	}

	if (PreviewButtonDownNew)//预览按钮按下，则画主筋线
	{
		m_allLines.clear();
		for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)
		{
			vector<DSegment3d> vcttemp(*it);
			for (int x = 0; x < vcttemp.size(); x++)
			{
				DPoint3d strPoint = DPoint3d::From(vcttemp[x].point[0].x, vcttemp[x].point[0].y, vcttemp[x].point[0].z);
				DPoint3d endPoint = DPoint3d::From(vcttemp[x].point[1].x, vcttemp[x].point[1].y, vcttemp[x].point[1].z);
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				m_allLines.push_back(eeh.GetElementRef());
			}
		}
		for (PIT::ArcSegment arcSeg : m_vecArcSeg)
		{
			if (COMPARE_VALUES_EPS(arcSeg.ptStart.Distance(arcSeg.ptEnd), uor_per_mm, EPS) < 0) // 圆形
			{
				EditElementHandle arceeh;
				if (SUCCESS == ArcHandler::CreateArcElement(arceeh, NULL, arcSeg.ptCenter, arcSeg.dRadius, arcSeg.dRadius, 0, 0, 2 * PI, true, *ACTIVEMODEL))
				{
					arceeh.AddToModel();
					m_allLines.push_back(arceeh.GetElementRef());
				}
			}
			else // 弧形
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

bool ELLWallRebarAssemblyNew::CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
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


bool ELLWallRebarAssemblyNew::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pEllWallDoubleRebarDlg = new CWallRebarDlgNew(ehSel, CWnd::FromHandle(MSWIND));
	pEllWallDoubleRebarDlg->SetSelectElement(ehSel);
	pEllWallDoubleRebarDlg->Create(IDD_DIALOG_WallRebar);
	pEllWallDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pEllWallDoubleRebarDlg->ShowWindow(SW_SHOW);
	return true;
}

bool ELLWallRebarAssemblyNew::Rebuild()
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