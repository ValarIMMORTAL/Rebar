/*--------------------------------------------------------------------------------------+
|
|     $Source: MySlabRebarAssembly.cpp $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include "resource.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "MySlabRebarAssembly.h"

#include <CElementTool.h>
#include <CPointTool.h>

#include "WallRebarDlg.h"
#include "ExtractFacesTool.h"
#include "CSlabRebarDlg.h"
#include "TieRebar.h"
#include "ElementAttribute.h"
#include "PITRebarEndType.h"
#include "PITMSCECommon.h"
#include "BentlyCommonfile.h"
#include "XmlHelper.h"
#include "CFaceTool.h"
#include "WallHelper.h"
#include "CModelTool.h"
#include <math.h>

extern bool SlabPreviewButtonDown;//板主要配筋界面的预览按钮
CWallMainRebarDlg*  SelectLineDirTool::m_Ptr = NULL;

int direction;
EditElementHandle m_pDownFace;
MySlabRebarAssembly::MySlabRebarAssembly(ElementId id, DgnModelRefP modelRef) :     // 构造函数初始化一些值
	RebarAssembly(id, modelRef),
	m_PositiveCover(0),                          //正面保护层
	m_ReverseCover(0),                           //反面保护层
	m_SideCover(0),                               //反面保护层
	m_RebarLevelNum(4)                            //钢筋层数
{
	Init();
}
void MySlabRebarAssembly::Init()            //重新指定容器的长度为m_RebarLevelNum
{
	m_vecDir.resize(m_RebarLevelNum);            //方向,0表示x轴，1表示z轴
	m_vecDirSize.resize(m_RebarLevelNum);         //尺寸
	m_vecDirSpacing.resize(m_RebarLevelNum);       //间隔
	m_vecRebarType.resize(m_RebarLevelNum);        //钢筋类型
	m_vecStartOffset.resize(m_RebarLevelNum);       //起点偏移
	m_vecEndOffset.resize(m_RebarLevelNum);         //终点偏移
	m_vecLevelSpace.resize(m_RebarLevelNum);       //与前层间距
	m_vecDataExchange.resize(m_RebarLevelNum);
	m_vecRebarLevel.resize(m_RebarLevelNum);
	m_vecSetId.resize(m_RebarLevelNum);            //SetId
	//m_vecRebarColor.resize(m_RebarLevelNum);		//钢筋颜色
	m_vecRebarLineStyle.resize(m_RebarLevelNum);	//钢筋线形
	m_vecRebarWeight.resize(m_RebarLevelNum);		//钢筋线宽
	int twinRebarLevel = 0;

	//根据需求并筋需要设置不一样的层
	for (size_t i = 0; i < GetvecTwinRebarLevel().size(); i++)
	{
		if (GetvecTwinRebarLevel().at(i).hasTwinbars)
		{
			twinRebarLevel++;
		}
	}
	m_vecSetId.resize(m_RebarLevelNum + twinRebarLevel);
	for (size_t i = 0; i < m_vecSetId.size(); i++)
	{
		m_vecSetId[i] = 0;
	}
}


bool MySlabRebarAssembly::CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
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

		vex = &curve.PopVertices().NewElement();
		vex->SetIP(end);
		vex->SetType(RebarVertex::kEnd);      // last IP

		mat = mat.Inverse();
		curve.DoMatrix(mat);              // transform back

		ret = true;
	}

	return ret;
}


void MySlabRebarAssembly::SetConcreteData(Concrete const& concreteData)    //设置三个保护层信息和层数
{
	m_PositiveCover = concreteData.postiveCover;
	m_ReverseCover = concreteData.reverseCover;
	m_SideCover = concreteData.sideCover;
	m_RebarLevelNum = concreteData.rebarLevelNum;
	m_strSlabRebarMethod = concreteData.m_SlabRebarMethod;
}

void MySlabRebarAssembly::SetRebarData(vector<PIT::ConcreteRebar> const& vecData)     //用vector数组存每层钢筋的信息
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

    for (PIT::ConcreteRebar& data : tmpvecdata)
		{
			if (data.datachange == 1 && midid == 0)
			{
				data.levelSpace = m_height / (2 * uor_per_mm) - midlenth / 2 - frontlenth - m_PositiveCover;
				midid = 1;
			}
			else if (data.datachange == 2 && backid == 0)
			{
				//data.levelSpace = 4000;
				if (midlenth != 0)
				{
					data.levelSpace = m_height / (2 * uor_per_mm) - midlenth / 2 - m_ReverseCover - backlenth;
				}
				else
				{
					data.levelSpace = m_height / uor_per_mm - backlenth - m_ReverseCover - frontlenth - m_PositiveCover;
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
			m_vecDirSize[i] = tmpvecdata[i].rebarSize;
			m_vecRebarType[i] = tmpvecdata[i].rebarType;
			m_vecDirSpacing[i] = tmpvecdata[i].spacing;
			m_vecStartOffset[i] = tmpvecdata[i].startOffset;
			m_vecEndOffset[i] = tmpvecdata[i].endOffset;
			m_vecLevelSpace[i] = tmpvecdata[i].levelSpace;
			m_vecDataExchange[i] = tmpvecdata[i].datachange;
			m_vecRebarLevel[i] = tmpvecdata[i].rebarLevel;
			//			GetDiameterAddType(m_vecDirSize[i], m_vecRebarType[i]);
			//m_vecRebarColor[i] = tmpvecdata[i].rebarColor;
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
			m_vecSetId.push_back(0);
			//m_vecRebarColor.push_back(tmpvecdata[i].rebarColor);
			m_vecRebarLineStyle.push_back(tmpvecdata[i].rebarLineStyle);
			m_vecRebarWeight.push_back(tmpvecdata[i].rebarWeight);
		}
	}

}

void MySlabRebarAssembly::SetRebarEndTypes(vector<PIT::EndType> const & vecEndTypes)       //处理端部
{
	// 	if (vecEndTypes.empty())
	// 	{
	// 		m_vvecEndType = { { {0,0,0} }, { {0,0,0} } };
	// 		return;
	// 	}
	if (vecEndTypes.size())                   //设置端部，如果有东西清空
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

void MySlabRebarAssembly::InitRebarSetId()
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

void MySlabRebarAssembly::GetConcreteData(Concrete& concreteData)             //获得三个保护层信息和层数信息
{
	concreteData.postiveCover = m_PositiveCover;
	concreteData.reverseCover = m_ReverseCover;
	concreteData.sideCover = m_SideCover;
	concreteData.rebarLevelNum = m_RebarLevelNum;
	//	concreteData.isTwinbars = m_Twinbars;
}
void MySlabRebarAssembly::GetRebarData(vector<PIT::ConcreteRebar>& vecData) const
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
		vecData.push_back(rebarData);
	}
}
void MySlabRebarAssembly::SetTieRebarInfo(TieReBarInfo const & tieRebarInfo)
{
	m_tieRebarInfo = tieRebarInfo;
}


void MySlabRebarAssembly::ClearLines()
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


const TieReBarInfo MySlabRebarAssembly::GetTieRebarInfo() const
{
	return m_tieRebarInfo;
}


SlabType MySlabRebarAssembly::JudgeSlabType(ElementHandleCR eh)             //判断是哪种墙
{

	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;
	EditElementHandle testeeh(eh, false);
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	if (!EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs))
	{
		return SlabTypeOther;
	}
	for (int j = 0; j < Holeehs.size(); j++)
	{
		delete Holeehs.at(j);
		Holeehs.at(j) = nullptr;
	}
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine, vecDownBackLine, NULL);

	if (vecDownFaceLine.empty())
		return SlabTypeOther;
	if (vecDownFontLine.empty() || vecDownBackLine.empty())
	{
		for (size_t i = 0; i < vecDownFaceLine.size(); i++)
		{
			mdlElmdscr_freeAll(&vecDownFaceLine[i]);
		}
		return SlabTypeOther;
	}
	if (vecDownFaceLine.size() != 4 && vecDownFontLine.size() != 1 && vecDownBackLine.size() != 1)
	{
		for (size_t i = 0; i < vecDownFaceLine.size(); i++)
		{
			mdlElmdscr_freeAll(&vecDownFaceLine[i]);
		}
		return GSLAB;
	}

	for (size_t i = 0; i < vecDownFaceLine.size(); i++)
	{
		if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
		{
			return GSLAB;
		}
	}
	return STSLAB;
}

bool MySlabRebarAssembly::IsSmartSmartFeature(EditElementHandle& eeh)
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


bool MySlabRebarAssembly::IsSlabSolid(ElementHandleCR eh)          //判断是不是墙
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

void STSlabRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
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
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*m_Holeehs.at(j));
				PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}
}

std::vector<ElementHandle> STSlabRebarAssembly::scan_elements_in_range(const DRange3d & range, std::function<bool(const ElementHandle&)> filter)
{
	std::vector<ElementHandle> ehs;

	std::map<ElementId, IComponent *> components;
	std::map<ElementId, MSElementDescrP> descriptions;

	/// 扫描在包围盒范围内的构件
	if (!PITCommonTool::CModelTool::AnalysisModelGetElements(range, components, descriptions))
	{
		// 返回空的即可
		return ehs;
	}

	const auto PLUS_ID = 1000000;

	ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
	for (const auto &kv : descriptions)
	{
		if (kv.second == NULL) continue;
		auto id = kv.first;
		// 这里返回的id可能加了一个PlusID以防止重复
		if (id >= PLUS_ID)
		{
			id -= PLUS_ID;
		}
		//const auto component = kv.second;
		// const auto desc = kv.second;

		// 扫描所有的model_ref，找到该元素所在的model_ref

		for (DgnModelRefP modelRef : modelRefCol)
		{
			EditElementHandle tmpeeh;
			if (tmpeeh.FindByID(id, modelRef) == SUCCESS)
			{
				auto eh = ElementHandle(id, modelRef);
				if (filter(eh))
				{
					// 满足条件，加入到输出 
					ehs.push_back(std::move(eh));
				}
			}

		}
	}

	// 排序+去重
	std::sort(
		ehs.begin(), ehs.end(),
		[](const ElementHandle &eh_a, const ElementHandle &eh_b) -> bool
	{
		return eh_a.GetElementId() < eh_b.GetElementId();
	});

	auto new_end = std::unique(
		ehs.begin(), ehs.end(),
		[](const ElementHandle &eh_a, const ElementHandle &eh_b) -> bool
	{
		return eh_a.GetElementId() == eh_b.GetElementId();
	});

	return std::vector<ElementHandle>(ehs.begin(), new_end);
}

bool STSlabRebarAssembly::is_Floor(const ElementHandle & element)
{
	std::string _name, type;
	if (!GetEleNameAndType(const_cast<ElementHandleR>(element), _name, type))
	{
		return false;
	}
	auto result_pos = type.find("FLOOR");
	return result_pos != std::string::npos;
}

double STSlabRebarAssembly::get_lae() const
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

//计算支墩配筋的起点终点
void STSlabRebarAssembly::CalculateRebarPts()
{
	m_mapRroundPts.clear();
	m_mapAllRebarpts.clear();
	EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());
	DRange3d eeh_range;//支墩的range
	mdlElmdscr_computeRange(&eeh_range.low, &eeh_range.high, testeeh.GetElementDescrCP(), nullptr);

	/*求板的厚度，通过获取所有边，起始点向量得出Z方向的线的长度*/
	vector<MSElementDescrP> edges;
	edges.clear();
	EditElementHandle eeh_Floor(m_downFloor.front(), ACTIVEMODEL);
	PITCommonTool::CElementTool::GetALLEdges(eeh_Floor, edges);
	for (auto tmpedge : edges)
	{
		EditElementHandle tmpeeh(tmpedge, false, false, ACTIVEMODEL);
		DPoint3d pt[2];
		mdlLinear_extract(pt, NULL, tmpeeh.GetElementP(), tmpeeh.GetModelRef());
		CVector3D vec_line(pt[1], pt[0]);
		vec_line.Normalize();
		if (COMPARE_VALUES_EPS(abs(vec_line.DotProduct(CVector3D::kZaxis)), 1, 0.1) == 0)
		{
			m_floor_Thickness = mdlVec_distance(&pt[0], &pt[1]);
			break;
		}
	}

	WString strSize(GetAbanurus_PTRebarData().ptrebarSize);
	if (strSize.find(L"mm") != WString::npos)
	{
		strSize.ReplaceAll(L"mm", L"");
	}
	WString stirrupstrSize(GetAbanurus_PTRebarData().stirrupRebarsize);
	if (stirrupstrSize.find(L"mm") != WString::npos)
	{
		stirrupstrSize.ReplaceAll(L"mm", L"");
	}

	double stirrup_radius = RebarCode::GetBarDiameter(stirrupstrSize, ACTIVEMODEL) / 2;
	double diameter = RebarCode::GetBarDiameter(strSize, ACTIVEMODEL);		//乘以了10
	double radius = diameter / 2;//点筋的半径
	double endbendRadius = RebarCode::GetPinRadius(strSize, ACTIVEMODEL, false);	//乘以了30,弯曲半径
	double positiveCover = GetPositiveCover() * UOR_PER_MilliMeter; //正面保护层，点筋距离支墩上面的距离
	double reverseCover = GetReverseCover() * UOR_PER_MilliMeter;	//反面保护层，钢筋支墩下面的板
	double sideCover = GetSideCover() * UOR_PER_MilliMeter + stirrup_radius * 2;			//侧面保护层，包括了箍筋和拉筋的直径
	if (COMPARE_VALUES(positiveCover, eeh_range.ZLength()) >= 0)
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"正面保护层大于等于支墩的厚度,无法创建钢筋层", MessageBoxIconType::Information);
		return;
	}
	if (COMPARE_VALUES(reverseCover, m_floor_Thickness) >= 0)
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"反面保护层大于等于板的厚度,无法创建钢筋层", MessageBoxIconType::Information);
		return;
	}
	
	double lae_d = stod(strSize.GetWCharCP());
	double lae = lae_d * get_lae();//锚入的长度
	m_lae = lae;
	int PtHnum = GetAbanurus_PTRebarData().ptHNum;//横筋数量，包括四周的两根
	int PtVnum = GetAbanurus_PTRebarData().ptVNum;//纵筋数量，包括四周的两根

	//间距为每根点筋的中心点间距
	double Hlength = eeh_range.XLength() - 2 * sideCover - diameter;//横向总长
	double HSpace = Hlength / (PtHnum - 1);							//每根横向钢筋的间距
	double Vlength = eeh_range.YLength() - 2 * sideCover - diameter;//纵向总长
	double VSpace = Vlength / (PtVnum - 1);							//每根纵向钢筋的间距

	m_HSpace = HSpace;
	m_VSpace = VSpace;

	// 1、计算出四周的四个点筋。如果横纵数量大于二，再计算其他点筋
	DPoint3d leftDownStr, rightDownStr, rightUpStr, leftUpStr;
	DPoint3d leftDownEnd, rightDownEnd, rightUpEnd, leftUpEnd;
	vector<Dpoint3d> vec_leftDown, vec_rightDown, vec_rightUp, vec_leftUp;
	vec_leftDown.clear(); vec_rightDown.clear(); vec_rightUp.clear(); vec_leftUp.clear();
	vector<vector<DPoint3d>> vec_Downpts, vec_Rightpts, vec_Uppts, vec_Leftpts;
	vec_Downpts.clear(); vec_Rightpts.clear(); vec_Uppts.clear(); vec_Leftpts.clear();
	stirrup_radius = 0;
	if (lae > m_floor_Thickness - reverseCover)//需要弯锚
	{
		m_isBend = true;
		//左下
		leftDownStr.x = eeh_range.low.x + sideCover + radius + stirrup_radius;
		leftDownStr.y = eeh_range.low.y + sideCover + radius + stirrup_radius;
		leftDownStr.z = eeh_range.high.z - positiveCover;
		vec_leftDown.emplace_back(leftDownStr);
		leftDownEnd = leftDownStr;
		leftDownEnd.z = eeh_range.low.z - m_floor_Thickness + reverseCover + radius;
		vec_leftDown.emplace_back(leftDownEnd);
		m_mapRroundPts[0] = vec_leftDown;
		
		//右下
		rightDownStr.x = eeh_range.high.x - sideCover - radius - stirrup_radius;
		rightDownStr.y = eeh_range.low.y + sideCover + radius + stirrup_radius;
		rightDownStr.z = eeh_range.high.z - positiveCover;
		vec_rightDown.emplace_back(rightDownStr);
		rightDownEnd = rightDownStr;
		rightDownEnd.z = eeh_range.low.z - m_floor_Thickness + reverseCover + radius;
		vec_rightDown.emplace_back(rightDownEnd);
		m_mapRroundPts[1] = vec_rightDown;

		//右上
		rightUpStr.x = eeh_range.high.x - sideCover - radius - stirrup_radius;
		rightUpStr.y = eeh_range.high.y - sideCover - radius - stirrup_radius;
		rightUpStr.z = eeh_range.high.z - positiveCover;
		vec_rightUp.emplace_back(rightUpStr);
		rightUpEnd = rightUpStr;
		rightUpEnd.z = eeh_range.low.z - m_floor_Thickness + reverseCover + radius;
		vec_rightUp.emplace_back(rightUpEnd);
		m_mapRroundPts[2] = vec_rightUp;

		//左上
		leftUpStr.x = eeh_range.low.x + sideCover + radius + stirrup_radius;
		leftUpStr.y = eeh_range.high.y - sideCover - radius - stirrup_radius;
		leftUpStr.z = eeh_range.high.z - positiveCover;
		vec_leftUp.emplace_back(leftUpStr);
		leftUpEnd = leftUpStr;
		leftUpEnd.z = eeh_range.low.z - m_floor_Thickness + reverseCover + radius;
		vec_leftUp.emplace_back(leftUpEnd);
		m_mapRroundPts[3] = vec_leftUp;
		
	}
	else//直锚
	{
		m_isBend = false;
		//左下
		leftDownStr.x = eeh_range.low.x + sideCover + radius + stirrup_radius;
		leftDownStr.y = eeh_range.low.y + sideCover + radius + stirrup_radius;
		leftDownStr.z = eeh_range.high.z - positiveCover;
		vec_leftDown.emplace_back(leftDownStr);
		leftDownEnd = leftDownStr;
		leftDownEnd.z = eeh_range.low.z - lae;
		vec_leftDown.emplace_back(leftDownEnd);
		m_mapRroundPts[0] = vec_leftDown;

		//右下
		rightDownStr.x = eeh_range.high.x - sideCover - radius - stirrup_radius;
		rightDownStr.y = eeh_range.low.y + sideCover + radius + stirrup_radius;
		rightDownStr.z = eeh_range.high.z - positiveCover;
		vec_rightDown.emplace_back(rightDownStr);
		rightDownEnd = rightDownStr;
		rightDownEnd.z = eeh_range.low.z - lae;
		vec_rightDown.emplace_back(rightDownEnd);
		m_mapRroundPts[1] = vec_rightDown;

		//右上
		rightUpStr.x = eeh_range.high.x - sideCover - radius - stirrup_radius;
		rightUpStr.y = eeh_range.high.y - sideCover - radius - stirrup_radius;
		rightUpStr.z = eeh_range.high.z - positiveCover;
		vec_rightUp.emplace_back(rightUpStr);
		rightUpEnd = rightUpStr;
		rightUpEnd.z = eeh_range.low.z - lae;
		vec_rightUp.emplace_back(rightUpEnd);
		m_mapRroundPts[2] = vec_rightUp;

		//左上
		leftUpStr.x = eeh_range.low.x + sideCover + radius + stirrup_radius;
		leftUpStr.y = eeh_range.high.y - sideCover - radius - stirrup_radius;
		leftUpStr.z = eeh_range.high.z - positiveCover;
		vec_leftUp.emplace_back(leftUpStr);
		leftUpEnd = leftUpStr;
		leftUpEnd.z = eeh_range.low.z - lae;
		vec_leftUp.emplace_back(leftUpEnd);
		m_mapRroundPts[3] = vec_leftUp;

	}

	vector<Dpoint3d> vec_tmp;
	//上下两排钢筋点
	if (PtHnum > 2)
	{//上下两排,包括四周的点
		vec_Downpts.emplace_back(vec_leftDown);
		for (int i = 0; i < PtHnum - 2; ++i)
		{
			leftDownStr.x += HSpace;
			leftDownEnd.x += HSpace;
			vec_tmp.clear();
			vec_tmp.emplace_back(leftDownStr);
			vec_tmp.emplace_back(leftDownEnd);
			vec_Downpts.emplace_back(vec_tmp);
		}
		vec_Downpts.emplace_back(vec_rightDown);
		m_mapAllRebarpts[0] = vec_Downpts;

		vec_Uppts.emplace_back(vec_rightUp);
		for (int i = 0; i < PtHnum - 2; ++i)
		{
			rightUpStr.x -= HSpace;
			rightUpEnd.x -= HSpace;
			vec_tmp.clear();
			vec_tmp.emplace_back(rightUpStr);
			vec_tmp.emplace_back(rightUpEnd);
			vec_Uppts.emplace_back(vec_tmp);
		}
		vec_Uppts.emplace_back(vec_leftUp);
		m_mapAllRebarpts[2] = vec_Uppts;
	}
	else if(PtHnum == 2)//横向点筋只有两个，即四周的点
	{
		vec_Downpts.emplace_back(vec_leftDown);
		vec_Downpts.emplace_back(vec_rightDown);
		m_mapAllRebarpts[0] = vec_Downpts;

		vec_Uppts.emplace_back(vec_rightUp);
		vec_Uppts.emplace_back(vec_leftUp);
		m_mapAllRebarpts[2] = vec_Uppts;
	}
	if (PtVnum > 2)//如果纵向钢筋只有两个就不保存左右两排的点，上下两排就包括了四周的点
	{//左右两排，不包括四周的点
		for (int i = 0; i < PtVnum - 2; ++i)
		{
			rightDownStr.y += VSpace;
			rightDownEnd.y += VSpace;
			vec_tmp.clear();
			vec_tmp.emplace_back(rightDownStr);
			vec_tmp.emplace_back(rightDownEnd);
			vec_Rightpts.emplace_back(vec_tmp);
		}
		m_mapAllRebarpts[1] = vec_Rightpts;

		for (int i = 0; i < PtVnum - 2; ++i)
		{
			leftUpStr.y -= VSpace;
			leftUpEnd.y -= VSpace;
			vec_tmp.clear();
			vec_tmp.emplace_back(leftUpStr);
			vec_tmp.emplace_back(leftUpEnd);
			vec_Leftpts.emplace_back(vec_tmp);
		}
		m_mapAllRebarpts[3] = vec_Leftpts;

	}


	
}

void STSlabRebarAssembly::MoveRebarPts()
{
	////箍筋弯曲半径
	//double stirrup_bendRadius = RebarCode::GetPinRadius(stirrupstrSize, ACTIVEMODEL, true);
	//WString ptstrSize(GetAbanurus_PTRebarData().ptrebarSize);
	//if (ptstrSize.find(L"mm") != WString::npos)
	//{
	//	ptstrSize.ReplaceAll(L"mm", L"");
	//}
	////点筋半径
	//double pt_radius = RebarCode::GetBarDiameter(ptstrSize, ACTIVEMODEL) / 2;

	//double move_sqrtDistance = sqrt(pt_radius);
	WString stirrupstrSize(GetAbanurus_PTRebarData().stirrupRebarsize);
	if (stirrupstrSize.find(L"mm") != WString::npos)
	{
		stirrupstrSize.ReplaceAll(L"mm", L"");
	}
	//箍筋直径
	double stirrup_diameter = RebarCode::GetBarDiameter(stirrupstrSize, ACTIVEMODEL);
	//double offset_distance = stirrup_diameter;
	int Hnum = GetAbanurus_PTRebarData().ptHNum;
	int Vnum = GetAbanurus_PTRebarData().ptVNum;

	m_mapActualAllRebarpts = m_mapAllRebarpts;

	if (Hnum % 2 == 1)//单数
	{
		//下面一排
		for (int i = 0; i < m_mapActualAllRebarpts[0].size() - 1; ++i)
		{
			if (i % 2)//单数向左偏移，双数向右偏移
			{
				for (auto &it : m_mapActualAllRebarpts[0].at(i))
				{
					it.x -= stirrup_diameter;
					it.y += stirrup_diameter;
				}
				/*for (int j = 0; j < m_mapActualAllRebarpts[0].at(i).size(); ++j)
				{
					m_mapActualAllRebarpts[0].at(i).at(j).x -= stirrup_diameter;
					m_mapActualAllRebarpts[0].at(i).at(j).y += stirrup_diameter;
				}*/
			}
			else
			{
				for (auto &it : m_mapActualAllRebarpts[0].at(i))
				{
					it.x += stirrup_diameter;
					it.y += stirrup_diameter;
				}
				/*for (int j = 0; j < m_mapActualAllRebarpts[0].at(i).size(); ++j)
				{
					m_mapActualAllRebarpts[0].at(i).at(j).x += stirrup_diameter;
					m_mapActualAllRebarpts[0].at(i).at(j).y += stirrup_diameter;
				}*/
			}
		}
		for (auto &it : m_mapActualAllRebarpts[0].at(m_mapActualAllRebarpts[0].size() - 1))
		{
			it.x -= stirrup_diameter;
			it.y += stirrup_diameter;
		}
		/*for (int j = 0; j < m_mapActualAllRebarpts[0].at(m_mapActualAllRebarpts[0].size() - 1).size(); ++j)
		{
			m_mapActualAllRebarpts[0].at(m_mapActualAllRebarpts[0].size() - 1).at(j).x -= stirrup_diameter;
			m_mapActualAllRebarpts[0].at(m_mapActualAllRebarpts[0].size() - 1).at(j).y += stirrup_diameter;
		}*/
		//上面一排
		for (int i = 1; i < m_mapActualAllRebarpts[2].size(); ++i)
		{
			if (i % 2)//单数向左偏移，双数向右偏移
			{
				for (auto &it : m_mapActualAllRebarpts[2].at(i))
				{
					it.x -= stirrup_diameter;
					it.y -= stirrup_diameter;
				}
			}
			else
			{
				for (auto &it : m_mapActualAllRebarpts[2].at(i))
				{
					it.x += stirrup_diameter;
					it.y -= stirrup_diameter;
				}
			}
		}
		for (auto &it : m_mapActualAllRebarpts[2].at(0))
		{
			it.x -= stirrup_diameter;
			it.y -= stirrup_diameter;
		}
	}
	else if (Hnum % 2 == 0)//双数
	{
		//下面一排
		for (int i = 0; i < m_mapActualAllRebarpts[0].size(); ++i)
		{
			if (i % 2)//单数向左偏移，双数向右偏移.下面都往上偏移
			{
				for (auto &it : m_mapActualAllRebarpts[0].at(i))
				{
					it.x -= stirrup_diameter;
					it.y += stirrup_diameter;
				}
			}
			else
			{
				for (auto &it : m_mapActualAllRebarpts[0].at(i))
				{
					it.x += stirrup_diameter;
					it.y += stirrup_diameter;
				}
			}
		}
		//上面一排
		for (int i = 0; i < m_mapActualAllRebarpts[2].size(); ++i)
		{
			if (i % 2)//单数向右偏移，双数向左偏移.上面都往下偏移
			{
				for (auto &it : m_mapActualAllRebarpts[2].at(i))
				{
					it.x += stirrup_diameter;
					it.y -= stirrup_diameter;
				}
			}
			else
			{
				for (auto &it : m_mapActualAllRebarpts[2].at(i))
				{
					it.x -= stirrup_diameter;
					it.y -= stirrup_diameter;
				}
			}
		}
	}

	if (Vnum > 2)//纵筋数量大于2 ，才涉及到左右两边存储的数据
	{
		//右边一排
		for (int i = 0; i < m_mapActualAllRebarpts[1].size(); ++i)
		{
			if (i % 2)//单数往上偏移，双数往下，都往左偏移
			{
				for (auto &it : m_mapActualAllRebarpts[1].at(i))
				{
					it.x -= stirrup_diameter;
					it.y += stirrup_diameter;
				}
			}
			else
			{
				for (auto &it : m_mapActualAllRebarpts[1].at(i))
				{
					it.x -= stirrup_diameter;
					it.y -= stirrup_diameter;
				}
			}
		}
		//左边一排
		if (Vnum % 2)//双往下，单往上，都往右
		{
			for (int i = 0; i < m_mapActualAllRebarpts[3].size(); ++i)
			{
				if (i % 2)
				{
					for (auto &it : m_mapActualAllRebarpts[3].at(i))
					{
						it.x += stirrup_diameter;
						it.y += stirrup_diameter;
					}
				}
				else
				{
					for (auto &it : m_mapActualAllRebarpts[3].at(i))
					{
						it.x += stirrup_diameter;
						it.y -= stirrup_diameter;
					}
				}
			}
		}
		else//双往上，单往下，都往右
		{
			for (int i = 0; i < m_mapActualAllRebarpts[3].size(); ++i)
			{
				if (i % 2)
				{
					for (auto &it : m_mapActualAllRebarpts[3].at(i))
					{
						it.x += stirrup_diameter;
						it.y -= stirrup_diameter;
					}
				}
				else
				{
					for (auto &it : m_mapActualAllRebarpts[3].at(i))
					{
						it.x += stirrup_diameter;
						it.y += stirrup_diameter;
					}
				}
			}
		}

	}


}

bool STSlabRebarAssembly::makeRebarCurve_G
(
	vector<PITRebarCurve>&     rebars,
	double                  xPos,
	double                  yLen,
	double					startOffset,
	double					endOffset,
	PITRebarEndTypes&		endTypes,
	CMatrix3D const&        mat,
	bool&                tag,
	bool isTwin
)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	CPoint3D  startPt;
	CPoint3D  endPt;

	//不允许为负值
// 	if (startPt < 0)
// 		startPt = 0;
// 	if (endOffset < 0)
// 		endOffset = 0;
	startPt = CPoint3D::From(xPos, 0.0, -yLen / 2.0);
	endPt = CPoint3D::From(xPos, 0.0, yLen / 2.0);


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
	/*if (GetslabType() == STSLAB&& !(m_Negs.size() > 0))
	{
		if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
		{
			return false;
		}
	}
	else
	{
		if (GetPoints_G( pt1[0], pt1[1], uor_per_mm, startOffset, tag))
		{
			return false;
		}
	}*/
	//	CVector3D  Vec(pt1[0], pt1[1]);
	//	CVector3D  nowVec = Vec;
	//
	////	CVector3D  nowVec = CVector3D::kZaxis.CrossProduct(VecNegate);
	//	nowVec.Normalize();
	//
	//	nowVec.ScaleToLength(startOffset);
	//	pt1[0].Add(nowVec);
	//	nowVec.Negate();
	//	nowVec.ScaleToLength(endOffset);
	//	pt1[1].Add(nowVec);
	//	
	DVec3d vec1 = pt1[1] - pt1[0];
	DVec3d vecX1 = DVec3d::From(1, 0, 0);
	vec1.x = COMPARE_VALUES_EPS(abs(vec1.x), 0, 10) == 0 ? 0 : vec1.x;
	vec1.y = COMPARE_VALUES_EPS(abs(vec1.y), 0, 10) == 0 ? 0 : vec1.y;
	vec1.z = COMPARE_VALUES_EPS(abs(vec1.z), 0, 10) == 0 ? 0 : vec1.z;
	vec1.Normalize();
	if (vec.IsPerpendicularTo(vecX1))
	{
		if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) > 0)
		{
			tag = false;
		}
	}
	else
	{
		if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) > 0)
		{
			tag = false;
		}
	}

	// EditElementHandle eeh2;
	// GetContractibleeeh(eeh2);//获取减去保护层的端部缩小的实体

	double dSideCover = GetSideCover() * uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	vector<vector<DPoint3d>> vecPtRebars;
	vector<DPoint3d> tmpptsTmp;
	vecPtRebars.clear();
	tmpptsTmp.clear();
	if (m_pOldElm != NULL) // 与原实体相交(无孔洞)
	{
		GetIntersectPointsWithOldElm(tmpptsTmp, m_pOldElm, pt1[0], pt1[1], dSideCover, matrix);

		if (tmpptsTmp.size() > 1)
		{
			// 存在交点为两个以上的情况
			GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, pt1[0], pt1[1], m_pOldElm, dSideCover);
		}
	}

	if (tmpptsTmp.size() < 2 && vecPtRebars.size() == 0)
	{
		vector<DPoint3d> vecPt;
		vecPt.push_back(pt1[0]);
		vecPt.push_back(pt1[1]);

		vecPtRebars.push_back(vecPt);
	}

	for (size_t i = 0; i < vecPtRebars.size(); i++)
	{
		pt1[0] = vecPtRebars.at(i).at(0);
		pt1[1] = vecPtRebars.at(i).at(1);
		CVector3D  Vec(pt1[0], pt1[1]);
		CVector3D  nowVec = Vec;

		//	CVector3D  nowVec = CVector3D::kZaxis.CrossProduct(VecNegate);
		nowVec.Normalize();
		nowVec.ScaleToLength(startOffset);
		pt1[0].Add(nowVec);
		nowVec.ScaleToLength(endOffset);
		nowVec.Negate();
		pt1[1].Add(nowVec);

		if (!isTwin)//非并筋时才需要存储点信息
		{
			m_vecRebarPtsLayer.push_back(pt1[0]);
			m_vecRebarPtsLayer.push_back(pt1[1]);
		}
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
			if (pt1[0].Distance(itr->second) < 10)
			{
				tmpendTypes.beg = endTypes.beg;
			}
			tmpendTypes.beg.SetptOrgin(itr->second);
			map<int, DPoint3d>::iterator itrplus = ++itr;
			if (itrplus == map_pts.end())
			{
				break;
			}
			if (pt1[1].Distance(itrplus->second) < 10)
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

	}
	//rebar.DoMatrix(mat);
	return true;
}

RebarSetTag* STSlabRebarAssembly::MakeRebars
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	double              xLen,
	double              width,
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
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//乘以了100
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
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//乘以了100
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
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeStart, modelRef);	//乘以了100
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
		endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//乘以了100
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
		endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//乘以了100
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
		endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//乘以了100
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
	int numRebar = 0;

	double leftSideCov, rightSideCov, allSideCov;
	leftSideCov = GetSideCover()*uor_per_mm;
	rightSideCov = GetSideCover()*uor_per_mm;
	allSideCov = leftSideCov + rightSideCov;


	if (twinBarInfo.hasTwinbars)	//并筋
		adjustedXLen = xLen - allSideCov - diameter - diameterTb /*- startOffset*/ - endOffset;
	else
		adjustedXLen = xLen - allSideCov - diameter /*- startOffset*/ - endOffset;
	//	double adjustedXLen = xLen - 2.0 * GetSideCover()*uor_per_mm - diameter - startOffset - endOffset;

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
	if (bTwinbarLevel)				//并筋层需偏移一个钢筋的距离
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
	for (int i = 0; i < numRebar; i++)//钢筋属性
	{
		bool tag;
		vector<PITRebarCurve>     rebarCurves;
		tag = true;
		makeRebarCurve_G(rebarCurves, xPos, width, endTypeStartOffset, endTypEendOffset, endTypes, mat, tag, bTwinbarLevel);
		xPos += adjustedSpacing;
		isTemp = false;
		if (!tag)
			continue;
		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}//rebarset里面rebarelement初步建立完成
	//钢筋组
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
		if (m_strSlabRebarMethod != 2) // 起点、终点一致  画圆形钢筋
		{
			if (ISPointInHoles(m_Holeehs, midPos))
			{
				if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
				{
					continue;
				}
			}
		}
		vector<DPoint3d> linePts;
		RebarVertices vertices = rebarCurve.GetVertices();
		for (size_t i = 0; i < (size_t)vertices.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &vertices.At(i);
			linePts.push_back(tmpVertex->GetIP());
		}

		RebarElementP rebarElement = nullptr;
		if (!SlabPreviewButtonDown)//预览标志,预览状态下不生成钢筋
		{
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}

		if (nullptr != rebarElement)
		{
			//EditElementHandle eeh;
			//LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			//eeh.AddToModel();

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
					Stype = "Twinback";
				else
					Stype = "back";
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
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
		vecStartEnd.push_back(linePts);
	}
	m_vecAllRebarStartEnd.push_back(vecStartEnd);//存储所有线段
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	CString spacingstring;
	spacingstring.Format(_T("%lf"), spacing / uor_per_mm);
	setdata.SetSpacingString(spacingstring);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

RebarSetTag * STSlabRebarAssembly::MakeRebars_Abanurus(ElementId & rebarSetId, int level, vector<vector<DPoint3d>> const& rebarpts, PITRebarEndTypes   &PITendTypes, RebarEndTypes &endtypes)
{
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, ACTIVEMODEL);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(ACTIVEMODEL);

	vector<PITRebarCurve>     rebarCurvesNum;
	vector<PITRebarCurve>     rebarCurves;
	PITRebarEndTypes		tmpendTypes;

	for (auto it : rebarpts)
	{
		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(it.front());
		vex->SetType(RebarVertex::kStart);
		tmpendTypes.beg = PITendTypes.beg;
		tmpendTypes.beg.SetptOrgin(it.front());

		tmpendTypes.end = PITendTypes.end;
		tmpendTypes.end.SetptOrgin(it.back());
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(it.back());
		vex->SetType(RebarVertex::kEnd);

		rebar.EvaluateEndTypes(tmpendTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebarCurves.push_back(rebar);
		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}
	int numRebar = (int)rebarCurvesNum.size();
	BrString sizeKey = GetAbanurus_PTRebarData().ptrebarSize;
	RebarSymbology symb;
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_INSERTION_REBAR);
	}
	int j = 0;
	bool isStirrup = false;
	double spacing = 0.0;
	if (level == 0 || level == 2)
		spacing = m_HSpace;
	else
		spacing = m_VSpace;
	double diameter = RebarCode::GetBarDiameter(sizeKey, ACTIVEMODEL);
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		RebarElementP rebarElement = nullptr;
		rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, ACTIVEMODEL);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / UOR_PER_MilliMeter);
			rebarElement->Update(rebarCurve, diameter, endtypes, shape, ACTIVEMODEL, false);


			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string Stype = "InsertRebar";
			string Level = to_string(level);
			int grade = GetAbanurus_PTRebarData().ptrebarType;
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Level, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / UOR_PER_MilliMeter, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	
	CString spacingstring;
	spacingstring.Format(_T("%lf"), spacing / UOR_PER_MilliMeter);
	setdata.SetSpacingString(spacingstring);
	setdata.SetNominalSpacing(spacing / UOR_PER_MilliMeter);
	setdata.SetAverageSpacing(spacing / UOR_PER_MilliMeter);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, ACTIVEMODEL);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

void STSlabRebarAssembly::CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef)
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
	BrString strTieRebarSize(GetTieRebarInfo().rebarSize);
	if (strTieRebarSize != L"" && 0 != GetTieRebarInfo().tieRebarMethod)
	{
		if (strTieRebarSize.Find(L"mm") != -1)
		{
			strTieRebarSize.Replace(L"mm", L"");
		}
		diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//拉筋直径
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

		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10
		double diameterTb = 0.0;
		if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
		{
			diameterTb = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(i).rebarSize, modelRef);		//乘以了10
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
				WString strSizePre;
				if (i != 0)
				{
					strSizePre = WString(GetvecDirSize().at(i - 1).Get());
					if (strSizePre.find(L"mm") != WString::npos)
					{
						strSizePre.ReplaceAll(L"mm", L"");
					}
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
				}
			}
			vTransform.push_back(zTrans);
			vTransformTb.push_back(zTransTb);
		}
	}
}
void MoveCenterFaceByPt(MSElementDescrP &pFacet, DVec3d vec, DPoint3d oript)
{
	mdlCurrTrans_begin();
	Transform tMatrix;
	mdlTMatrix_getIdentity(&tMatrix);
	mdlTMatrix_setOrigin(&tMatrix, &oript);
	mdlTMatrix_setTranslation(&tMatrix, &vec);
	mdlElmdscr_transform(&pFacet, &tMatrix);
	mdlCurrTrans_end();
}

bool STSlabRebarAssembly::MakeRebars(DgnModelRefP modelRef)      // 创建钢筋
{
	if (m_isAbanurus)
	{
		rsetTags.Clear(true);
		EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());

		DRange3d eeh_range;//支墩的range
		mdlElmdscr_computeRange(&eeh_range.low, &eeh_range.high, testeeh.GetElementDescrCP(), nullptr);

		//1. 扫描支墩下面的板
		eeh_range.low.z -= 2 * UOR_PER_MilliMeter;
		m_downFloor = 
			scan_elements_in_range(
				eeh_range,
				[&](const ElementHandle &eh) -> bool {
			if (eh.GetElementId() == testeeh.GetElementId())
			{
				// 过滤掉自己
				return false;
			}
			// 只需要板
			return is_Floor(eh);
		});

		//2. 计算钢筋的起点终点
		CalculateRebarPts();

		//3. 移动钢筋的点
		MoveRebarPts();
	
		double reverseCover = GetReverseCover() * UOR_PER_MilliMeter;
		BrString sizekey = GetAbanurus_PTRebarData().ptrebarSize;
		double bendRadius = RebarCode::GetPinRadius(sizekey, ACTIVEMODEL, false);
		double bendLength = 0;
		double diameter = RebarCode::GetBarDiameter(sizekey, ACTIVEMODEL);		//乘以了10
		double radius = diameter / 2;//点筋的半径
		int setCount = 0;
		//3. 根据存储每一排的钢筋点去生成钢筋,下右上左，如果纵向点筋数量等于2，就不会有左右两排钢筋
		for (auto it : m_mapActualAllRebarpts/*m_mapAllRebarpts*/)
		{
			RebarEndType endTypeStart, endTypeEnd;
			endTypeStart.SetType(RebarEndType::kNone);
			endTypeEnd.SetType(RebarEndType::kNone);
			if (m_isBend)
			{
				endTypeEnd.SetType(RebarEndType::kBend);
				bendRadius = RebarCode::GetPinRadius(sizekey, ACTIVEMODEL, false);
				bendLength = m_lae - (m_floor_Thickness - reverseCover - radius) - bendRadius;
			}
			RebarEndTypes endtypes{ endTypeStart, endTypeEnd };
			PITRebarEndType start, end;
			start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
			start.Setangle(0);

			end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
			end.SetbendLen(bendLength);
			end.SetbendRadius(bendRadius);
			CVector3D endNormal = CVector3D::From(0, 0, 0);
			if (it.first == 0)
				endNormal = CVector3D::From(0, 1, 0);
			else if(it.first == 1)
				endNormal = CVector3D::From(-1, 0, 0);
			else if (it.first == 2)
				endNormal = CVector3D::From(0, -1, 0);
			else if (it.first == 3)
				endNormal = CVector3D::From(1, 0, 0);
			end.SetendNormal(endNormal);

			PITRebarEndTypes PITendTypes = { start,end };
			RebarSetTag* tag = NULL;
			PopvecSetId().push_back(0);
			setCount++;
			tag = MakeRebars_Abanurus(PopvecSetId().back(),it.first, it.second, PITendTypes,endtypes);
			if (NULL != tag)
			{
				tag->SetBarSetTag(setCount);
				rsetTags.Add(tag);
			}

		}

	}
	else
	{
		rsetTags.Clear(true);
		m_vecAllRebarStartEnd.clear();
		EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());
		//testeeh.AddToModel();

		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
		SetSelectedElement(Eleeh.GetElementId());

		if (m_Holeehs.size() == 0 && Holeehs.size() > 0)

		{
			m_Holeehs = Holeehs;
		}
		if (g_globalpara.Getrebarstyle() != 0)
		{
			NewRebarAssembly(modelRef);
		}
		SetSelectedElement(testeeh.GetElementId());
		CalculateUseHoles(modelRef);
		g_vecRebarPtsNoHole.clear();
		g_vecTieRebarPtsNoHole.clear();
		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
		double dLevelSpace = 0;
		double dSideCover = GetSideCover()*uor_per_mm;
		if (COMPARE_VALUES(dSideCover, m_ldfoordata.Zlenth) >= 0)	//如果侧面保护层大于等于墙的长度
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
		DVec3d vecZ = DVec3d::From(0, 0, 1);
		for (int i = 0; i < iRebarLevelNum; ++i)
		{
			vecZ.Normalize();
			//获取每一个面的位置
			MSElementDescrP tmpdescr = nullptr;
			mdlElmdscr_duplicate(&tmpdescr, m_ldfoordata.facedes);
			double FHight = vTrans[i].y;
			vecZ.Scale(FHight);
			MSElementDescrP upface = nullptr;
			mdlElmdscr_duplicate(&upface, m_ldfoordata.facedes);
			MoveCenterFaceByPt(upface, vecZ, m_ldfoordata.oriPt);

			CVector3D ORIPT = m_ldfoordata.oriPt;
			ORIPT.Add(vecZ);
			BeMatrix   placement = GetPlacement();
			placement.SetTranslation(ORIPT);
			SetPlacement(placement);
			vTrans[i].y = 0;//y方向不用再偏移
			double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;
			// 计算层关系
			double dis_x = 0;//X方向偏移
			double dis_y = 0;//Y方向偏移
			int tmpLevel;
			if (GetvecDataExchange().at(i) == 0)//前面层,上部钢筋处理
			{
				frontlevel++;
				tmpLevel = frontlevel;
				//a、如果上部有墙，上部面为内侧面
					//钢筋方向VEC处理
					   //(1)、取出所有与VEC方向同向的面，将墙面减去后得到配筋面；
					   //(2)、按不同的配筋面数据配筋，判断钢筋面的起点和终点是否为整个大配筋面的起点或终点，
							  //如果是，忽略；不是，在远离配筋面中心的方向,多配置一根钢筋，偏移距离2个保护层+（第几层 - 1）个钢筋直径
					   //(3)、钢筋长度计算，起点上部有没有与钢筋方向垂直的墙，终点沿用起点数据
							  //有垂直墙，再判断当前钢筋层，钢筋前面还有层，长度缩小2个钢筋直径，没有层缩小1个钢筋直径（钢筋锚入处理）
							  //没有垂直墙，钢筋长度不用缩减
				//b、如果上部没有墙，上部面为外侧面
					 //钢筋方向VEC处理
					 //（1）钢筋长度计算，下部有没有与钢筋方向垂直的墙，终点下部处理和起点类似
							  //有垂直墙，再判断当前钢筋层，钢筋前面还有层，起点部分长度缩小1个钢筋直径，没有层缩不缩小（钢筋锚入处理）
							  //没有墙，钢筋长度不用缩减
			}
			else if (GetvecDataExchange().at(i) == 2)//后面层，下部钢筋处理
			{
				tmpLevel = backlevel;
				backlevel--;
				//a、如果下部有墙，下部面为内侧面
					//钢筋方向VEC处理
					   //(1)、取出所有与VEC方向同向的面，将墙面减去后计算得到配筋面；
					   //(2)、按不同的配筋面数据配筋，判断钢筋面的起点和终点是否为整个大配筋面的起点或终点，
							  //如果是，忽略；不是，在远离配筋面中心的方向,多配置一根钢筋，偏移距离2个保护层+（第几层 - 1）个钢筋直径
					   //(3)、钢筋长度计算，起点上部有没有与钢筋方向垂直的墙，终点沿用起点数据
							  //有垂直墙，再判断当前钢筋层，钢筋前面还有层，长度缩小2个钢筋直径，没有层缩小1个钢筋直径（钢筋锚入处理）
							  //没有垂直墙，钢筋长度不用缩减
				//b、如果下部没有墙，下部面为外侧面
					 //钢筋方向VEC处理
					 //（1）钢筋长度计算，下部有没有与钢筋方向垂直的墙，终点下部处理和起点类似
							  //有垂直墙，再判断当前钢筋层，钢筋前面还有层，起点部分长度缩小1个钢筋直径，没有层缩不缩小（钢筋锚入处理）
							  //没有墙，钢筋长度不用缩减


			}
			else
			{
				midlevel++;
				tmpLevel = midlevel;
			}
			// end
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

			/*CVector3D tmpVector(m_LineNormal);
			tmpVector.Scale(vTrans[i].y);
			CMatrix3D   tmpmat;
			tmpmat.SetTranslation(tmpVector);*/
			double  Misdisstr = 0; double Misdisend = 0;
			double tLenth = dLength;

			if (vTrans.size() != GetRebarLevelNum())
			{
				return false;
			}
			m_vecRebarPtsLayer.clear();
			m_vecTwinRebarPtsLayer.clear();
			m_vecTieRebarPtsLayer.clear();
			m_nowvecDir = GetvecDir().at(i);
			if (GetvecDir().at(i) == 1)	//纵向钢筋
			{
				vector<CVector3D> vecEndNormal(2);
				CVector3D	endNormal;	//端部弯钩方向
				if (GetvvecEndType().size() > 0)
				{
					for (size_t k = 0; k < vecEndNormal.size(); ++k)
					{
						double dRotateAngle = vecEndType.at(k).rotateAngle;
						endNormal = m_STwallData.ptEnd - m_STwallData.ptStart;
						endNormal.Normalize();
						endNormal.Negate();
						CVector3D rebarVec = CVector3D::kYaxis;
						/*					endNormal = rebarVec.CrossProduct(vec);*/
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
						vecEndNormal[k] = endNormal;
					}
				}

				if (m_strSlabRebarMethod != 2)
				{
					mat.SetTranslation(vTrans[i]);
					mat = GetPlacement() * mat;
				}
				else //放射形配筋方式不需要 *  GetPlacement() 
				{
					vTrans[i].z = vTrans[i].y;
					vTrans[i].x = 0.0;
					vTrans[i].y = 0.0;
					mat.SetTranslation(vTrans[i]);

				}

				if (m_strSlabRebarMethod != 2)
				{
					matTb.SetTranslation(vTransTb[i]);
					matTb = GetPlacement() * matTb;
				}
				else //放射形配筋方式不需要 *  GetPlacement() 
				{
					vTransTb[i].z = vTransTb[i].y;
					vTransTb[i].x = 0.0;
					vTransTb[i].y = 0.0;
					matTb.SetTranslation(vTransTb[i]);

				}

				//绘制并筋--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
				{
					//先绘制非并筋层
					PopvecSetId().push_back(0);
					setCount++;
					tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + dis_x,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
					if (NULL != tag)
					{
						tag->SetBarSetTag(setCount);
						rsetTags.Add(tag);
					}

					//绘制并筋层
					PopvecSetId().push_back(0);
					setCount++;
					tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + dis_x,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), true, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
					if (NULL != tag)
					{
						tag->SetBarSetTag(setCount);
						rsetTags.Add(tag);
					}
					iTwinbarSetIdIndex++;
				}
				else //当前层未设置并筋
				{
					TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
					PopvecSetId().push_back(0);
					setCount++;
					tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + dis_x,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, twinRebar, false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
					if (NULL != tag)
					{
						tag->SetBarSetTag(setCount);
						rsetTags.Add(tag);
					}
				}
				vecEndType.clear();
			}
			else
			{
				double leftSideCov = 0;
				double rightSideCov = 0;
				double allSideCov = leftSideCov + rightSideCov;

				vTrans[i].x = (tLenth - allSideCov) / 2 + Misdisstr + leftSideCov;
				vector<CVector3D> vecEndNormal(2);
				CVector3D	endNormal;	//端部弯钩方向
				if (GetvvecEndType().size() > 0)
				{
					for (size_t k = 0; k < vecEndNormal.size(); ++k)
					{
						double dRotateAngle = vecEndType.at(k).rotateAngle;
						CVector3D rebarVec = m_STwallData.ptEnd - m_STwallData.ptStart;
						rebarVec.Negate();
						endNormal = CVector3D::From(0, 0, -1);
						/*if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
						{
							endNormal.Negate();
						}*/
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
						vecEndNormal[k] = endNormal;
					}
				}
				mat = rot90;
				if (m_strSlabRebarMethod != 2)
				{
					mat.SetTranslation(vTrans[i]);
					mat = GetPlacement() * mat;
				}
				else //放射形配筋方式不需要 *  GetPlacement() 
				{
					vTrans[i].z = vTrans[i].y;
					vTrans[i].x = 0.0;
					vTrans[i].y = 0.0;
					mat.SetTranslation(vTrans[i]);
				}

				matTb = rot90;
				if (m_strSlabRebarMethod != 2)
				{
					matTb.SetTranslation(vTransTb[i]);
					matTb = GetPlacement() * matTb;
				}
				else //放射形配筋方式不需要 *  GetPlacement() 
				{
					vTransTb[i].z = vTransTb[i].y;
					vTransTb[i].x = 0.0;
					vTransTb[i].y = 0.0;
					matTb.SetTranslation(vTransTb[i]);
				}
				//奇数层为并筋层,偶数层为普通层

				//绘制并筋--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//绘制并筋
				{
					//先绘制非并筋层
					PopvecSetId().push_back(0);
					setCount++;
					tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + dis_y,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
					if (NULL != tag)
					{
						tag->SetBarSetTag(setCount);
						rsetTags.Add(tag);
					}
					//绘制并筋层
					PopvecSetId().push_back(0);
					setCount++;
					tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + dis_y,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, matTb, GetvecTwinRebarLevel().at(i), true, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
					if (NULL != tag)
					{
						tag->SetBarSetTag(setCount);
						rsetTags.Add(tag);
					}
					iTwinbarSetIdIndex++;

				}
				else //当前层未设置并筋
				{
					TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
					PopvecSetId().push_back(0);
					setCount++;
					tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + dis_y,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, twinRebar, false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
					if (NULL != tag)
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

		if (SlabPreviewButtonDown)//预览按钮按下，则画主筋线
		{
			int index = 0;
			m_allLines.clear();
			//主筋及并筋线信息
			for (auto it = m_vecAllRebarStartEnd.begin(); it != m_vecAllRebarStartEnd.end(); it++, index++)
			{

				UInt32 colors = 3;//绿色
				int style = 0;//实线
				UInt32	weight = 2;

				if (GetvecDir().at(index) == 0)//如果是水平方向
					colors = 1;//红色

				if (GetvecDataExchange().at(index) == 2)//如果是内侧
					style = 2;//虚线
				if (Gallery::WallHelper::analysis_slab_isTop(ehSel))//如果是顶板需要切换
				{
					if (style == 2)//虚线换成实线
						style = 0;
					else
						style = 2;
				}


				vector<vector<DPoint3d>> faceLinePts = *it;
				for (auto it : faceLinePts)
				{
					vector<DPoint3d> linePts = it;
					EditElementHandle eeh;
					LineStringHandler::CreateLineStringElement(eeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
					mdlElement_setSymbology(eeh.GetElementP(), &colors, &weight, &style);
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

			faceDataArray.revRebarData.HRebarData.rebarSize = GetvecDirSize().at(GetvecDirSize().size() - 1);
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
				//return true;
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
			TieRebarMaker tieRebarMaker(faceDataArray, vecStartEnd, (TieRebarStyle)tieRebarStyle, strTieRebarSize);
			tieRebarMaker.m_CallerId = GetCallerId();
			tieRebarMaker.SetCustomStyle(GetTieRebarInfo().rowInterval, GetTieRebarInfo().colInterval);
			Transform trans;
			GetPlacement().AssignTo(trans);
			tieRebarMaker.SetTrans(trans);
			vector<EditElementHandle*> vecAllSolid;
			vecAllSolid.insert(vecAllSolid.begin(), m_Holeehs.begin(), m_Holeehs.end());
			vecAllSolid.insert(vecAllSolid.end(), m_Negs.begin(), m_Negs.end());
			tieRebarMaker.SetDownVec(m_STwallData.ptStart, m_STwallData.ptEnd);
			tieRebarMaker.SetHoles(vecAllSolid);
			tieRebarMaker.SetHoleCover(GetSideCover()*uor_per_mm);
			tieRebarMaker.SetModeType(1);
			RebarSetTag* tag = tieRebarMaker.MakeRebar(PopvecSetId().at(GetvecSetId().size() - 1), modelRef);
			tieRebarMaker.GetRebarPts(vctTieRebarLines);//取出所有的拉筋直线信息
			if (NULL != tag && (!SlabPreviewButtonDown))
			{
				tag->SetBarSetTag(iRebarLevelNum + 1);
				rsetTags.Add(tag);
			}
		}

		if (SlabPreviewButtonDown)//预览按钮按下，则画拉筋线
		{
			// 		m_allLines.clear();
			// 		//主筋及并筋线信息
			// 		for (auto it = m_vecAllRebarStartEnd.begin(); it != m_vecAllRebarStartEnd.end(); it++)
			// 		{
			// 			vector<DSegment3d> vcttemp(*it);
			// 			for (int x = 0; x < vcttemp.size(); x++)
			// 			{
			// 				DPoint3d strPoint = DPoint3d::From(vcttemp[x].point[0].x, vcttemp[x].point[0].y, vcttemp[x].point[0].z);
			// 				DPoint3d endPoint = DPoint3d::From(vcttemp[x].point[1].x, vcttemp[x].point[1].y, vcttemp[x].point[1].z);
			// 				EditElementHandle eeh;
			// 				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
			// 				eeh.AddToModel();
			// 				m_allLines.push_back(eeh.GetElementRef());
			// 			}
			// 		}
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
		/*else
		{
			RebarModel *rmv = RMV;
			if (rmv != nullptr)
			{
				for (int i = 0;i< rsetTags.GetSize();i++)
				{
					RebarSetTag* tag = rsetTags.GetAt(i);
					if (tag!=nullptr)
					{
						for (int j = 0;j<tag->GetRset()->GetChildElementCount();j++)
						{
							RebarElementP rebele = tag->GetRset()->GetChildElement(j);
							if (rebele==nullptr)
							{
								continue;
							}
							rmv->SaveRebar(*rebele, rebele->GetModelRef(), true);
						}
					}
				}

			}
		}*/
	}
	return true;
}

long STSlabRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool STSlabRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	pSlabDoubleRebarDlg = new CSlabRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pSlabDoubleRebarDlg->SetSelectElement(ehSel);
	pSlabDoubleRebarDlg->Create(IDD_DIALOG_SlabRebar);
	pSlabDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pSlabDoubleRebarDlg->ShowWindow(SW_SHOW);

	// 	CSlabRebarDlg dlg(ehSel, CWnd::FromHandle(MSWIND));
	// 
	// 	//	dlg.SetSelectElement(ehSel);
	// 	dlg.SetConcreteId(FetchConcrete());
	// 	if (IDCANCEL == dlg.DoModal())
	return false;

	return true;
}

bool STSlabRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehslab(GetSelectedElement(), GetSelectedModel());
	if (!ehslab.IsValid())
		return false;

	DgnModelRefP modelRef = ehslab.GetModelRef();

	SetSlabData(ehslab);

	MakeRebars(modelRef);//调用创建钢筋
	Save(modelRef);

	ElementId contid = FetchConcrete();
	return true;
}



void STSlabRebarAssembly::SetSlabRebarDir(DSegment3d& Seg, ArcRebar& mArcLine)//板配筋的方向
{

}

//局部配筋平面在XOZ平面
bool STSlabRebarAssembly::SetSlabData(ElementHandleCR eh)
{
	bool bRet = AnalyzingSlabGeometricData(eh);
	if (!bRet)
		return false;
	PopvecFrontPts().push_back(m_STslabData.ptStart);
	PopvecFrontPts().push_back(m_STslabData.ptEnd);
	DPoint3d ptStart, ptEnd;
	DPoint3d ptOrgin = m_STslabData.ptStart;
	ehSel = eh;
	ptStart = m_STslabData.ptStart;
	ptEnd = m_STslabData.ptEnd;

	DVec3d tmpz = m_STslabData.vecZ;
	tmpz.Normalize();

	CVector3D  yVec = tmpz;     //返回两个向量的（标量）叉积。y  	
	yVec.Scale(-1);
	CVector3D  xVecNew(ptStart, ptEnd);
	xVecNew.Normalize();
	bool isXtY = false;
	tmpz.Scale(m_STslabData.width);
	ptOrgin.Add(tmpz);
	BeMatrix   placement = CMatrix3D::Ucs(ptOrgin, xVecNew, yVec, isXtY);		//方向为X轴，水平垂直方向为Y轴
	//placement.SetScaleFactors(1, 1, -1);
	SetPlacement(placement);
	PopvecFrontPts().push_back(ptStart);
	PopvecFrontPts().push_back(ptEnd);
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}

//获取与板顶与板底的墙面
void GetUpDownWallFaces(STSlabRebarAssembly::LDFloorData& m_ldfoordata)
{
	MSElementDescrP downface = nullptr;
	mdlElmdscr_duplicate(&downface, m_ldfoordata.facedes);

	DVec3d vecZ = DVec3d::From(0, 0, 1);
	double FHight = m_ldfoordata.Zlenth + 10;
	vecZ.Scale(FHight);
	MSElementDescrP upface = nullptr;
	mdlElmdscr_duplicate(&upface, m_ldfoordata.facedes);
	PITCommonTool::CFaceTool::MoveCenterFaceByMidPt(upface, vecZ);
	//mdlElmdscr_add(upface);
	//获取底面墙
	EditElementHandle eeh(downface, true, false, ACTIVEMODEL);
	Transform matrix;
	DPoint3d minP = { 0 }, maxP = { 0 };
	mdlElmdscr_computeRange(&minP, &maxP, downface, NULL);
	DPoint3d ptCenter = minP;
	ptCenter.Add(maxP);
	ptCenter.Scale(0.5);
	mdlTMatrix_getIdentity(&matrix);
	mdlTMatrix_scale(&matrix, &matrix, 0.99, 0.99, 1.0);
	mdlTMatrix_setOrigin(&matrix, &ptCenter);
	TransformInfo tInfo(matrix);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, tInfo);

	std::vector<IDandModelref>  Same_Eles;
	GetNowIntersectDatas(eeh, Same_Eles);
	for (int i = 0; i < Same_Eles.size(); i++)
	{
		string Ename, Etype;
		EditElementHandle tmpeeh(Same_Eles.at(i).ID, Same_Eles.at(i).tModel);
		if (GetEleNameAndType(tmpeeh, Ename, Etype))
		{
			if (Etype != "FLOOR")
			{
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(tmpeeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(tmpeeh);
				EditElementHandle upface;
				DPoint3d maxpt;
				if (!ExtractFacesTool::GetFloorDownFace(tmpeeh, upface, maxpt, false, downface))
				{
					continue;
				}
				if (upface.IsValid())
				{
					//mdlElmdscr_add(upface.GetElementDescrP());
					m_ldfoordata.upfaces[m_ldfoordata.upnum++] = upface.ExtractElementDescr();
				}

			}
		}
	}

	//获取顶面墙
	EditElementHandle eehup(upface, true, false, ACTIVEMODEL);
	mdlElmdscr_computeRange(&minP, &maxP, upface, NULL);
	ptCenter = minP;
	ptCenter.Add(maxP);
	ptCenter.Scale(0.5);
	mdlTMatrix_getIdentity(&matrix);
	mdlTMatrix_scale(&matrix, &matrix, 0.99, 0.99, 1.0);
	mdlTMatrix_setOrigin(&matrix, &ptCenter);
	TransformInfo tInfo2(matrix);
	eehup.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehup, tInfo2);

	std::vector<IDandModelref>  Same_Elesup;
	GetNowIntersectDatas(eehup, Same_Elesup);
	for (int i = 0; i < Same_Elesup.size(); i++)
	{
		string Ename, Etype;
		EditElementHandle tmpeeh(Same_Elesup.at(i).ID, Same_Elesup.at(i).tModel);
		if (GetEleNameAndType(tmpeeh, Ename, Etype))
		{
			if (Etype != "FLOOR")
			{
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(tmpeeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(tmpeeh);
				EditElementHandle dface;//墙底面
				DPoint3d minpt;
				if (!ExtractFacesTool::GetFloorDownFace(tmpeeh, dface, minpt, true, upface))
				{
					continue;
				}
				if (dface.IsValid())
				{
					//mdlElmdscr_add(dface.GetElementDescrP());
					m_ldfoordata.downfaces[m_ldfoordata.downnum++] = dface.ExtractElementDescr();
				}
			}
		}
	}

}


bool STSlabRebarAssembly::AnalyzingSlabGeometricData(ElementHandleCR eh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	m_model = model;

	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);

	EditElementHandle copyEleeh;
	copyEleeh.Duplicate(eh);
	ElementCopyContext copier2(ACTIVEMODEL);
	copier2.SetSourceModelRef(eh.GetModelRef());
	copier2.SetTransformToDestination(true);
	copier2.SetWriteElements(false);
	copier2.DoCopy(copyEleeh);
	DPoint3d minP2, maxP2;
	//计算指定元素描述符中元素的范围。
	mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
	if (!Eleeh.IsValid())
	{
		mdlDialog_dmsgsPrint(L"非法的板实体!");
		return false;
	}
	if (m_pOldElm == NULL)
	{
		m_pOldElm = new EditElementHandle();
	}
	m_pOldElm->Duplicate(Eleeh);
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
	RotMatrix rMatrix;
	Transform trans;
	mdlRMatrix_getIdentity(&rMatrix);
	mdlRMatrix_fromVectorToVector(&rMatrix, &facenormal, &vecZ);//旋转到xoy平面
	mdlTMatrix_fromRMatrix(&trans, &rMatrix);
	mdlTMatrix_setOrigin(&trans, &minPos);
	copyEleeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(copyEleeh, TransformInfo(trans));
	downface.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(downface, TransformInfo(trans));

	double tmpangle = facenormal.AngleTo(vecZ);
	if (tmpangle > PI / 2)
	{
		tmpangle = PI - tmpangle;
	}
	//计算指定元素描述符中元素的范围。
	mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
	DPoint3d minP, maxP;
	//计算指定元素描述符中元素的范围。
	mdlElmdscr_computeRange(&minP, &maxP, downface.GetElementDescrP(), NULL);
	trans.InverseOf(trans);
	downface.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(downface, TransformInfo(trans));
	m_ldfoordata.Ylenth = (maxP.y - minP.y)*uor_now / uor_ref;
	m_ldfoordata.Xlenth = (maxP.x - minP.x)*uor_now / uor_ref;
	m_ldfoordata.Zlenth = (maxP2.z - minP2.z)*uor_now / uor_ref;
	m_ldfoordata.oriPt = minP;
	m_ldfoordata.Vec = DVec3d::From(facenormal.x, facenormal.y, facenormal.z);
	m_ldfoordata.facedes = downface.ExtractElementDescr();
	GetUpDownWallFaces(m_ldfoordata);

	//计算整个板的参数
	m_STslabData.height = (maxP.y - minP.y)*uor_now / uor_ref;
	m_STslabData.length = (maxP.x - minP.x)*uor_now / uor_ref;
	m_STslabData.width = (maxP2.z - minP2.z)*uor_now / uor_ref;
	m_STslabData.ptStart = minP;
	m_STslabData.ptEnd = minP;
	m_STslabData.ptEnd.x = maxP.x;

	//计算当前Z坐标方向
	DPoint3d ptPreStr = m_STslabData.ptStart;
	DPoint3d ptPreEnd = m_STslabData.ptStart;
	ptPreEnd.z = ptPreEnd.z + m_STslabData.width*uor_now;

	mdlTMatrix_transformPoint(&m_ldfoordata.oriPt, &trans);
	mdlTMatrix_transformPoint(&m_STslabData.ptStart, &trans);
	mdlTMatrix_transformPoint(&m_STslabData.ptEnd, &trans);
	mdlTMatrix_transformPoint(&ptPreStr, &trans);
	mdlTMatrix_transformPoint(&ptPreEnd, &trans);
	m_STslabData.vecZ = ptPreEnd - ptPreStr;
	m_STslabData.vecZ.Normalize();


	m_STwallData.height = m_STslabData.height;
	m_STwallData.length = m_STslabData.length;
	m_STwallData.ptStart = m_STslabData.ptStart;
	m_STwallData.ptEnd = m_STslabData.ptEnd;
	m_STwallData.ptPreStr = ptPreStr;
	m_STwallData.ptPreEnd = ptPreEnd;
	m_Holeehs = Holeehs;
	m_height = m_STslabData.width;
	m_STwallData.width = m_STslabData.width;

	return true;
}


//选择线段，确定板配筋方向
void SelectLineDirTool::InstallNewInstance(int toolId, CWallMainRebarDlg *Ptr)
{
	mdlSelect_freeAll();
	//清除已选中的元素
	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	//启动工具
	SelectLineDirTool* tool = new SelectLineDirTool(toolId);
	tool->InstallTool();
	m_Ptr = Ptr;
}

StatusInt SelectLineDirTool::_OnElementModify(EditElementHandleR el)
{
	return ERROR;
}

void SelectLineDirTool::_SetupAndPromptForNextAction()
{

}

bool SelectLineDirTool::_OnModifyComplete(DgnButtonEventCR ev)
{
	return true;
}


bool SelectLineDirTool::_OnModifierKeyTransition(bool wentDown, int key)
{
	if (CTRLKEY != key)
		return false;
	if (GetElementAgenda().GetCount() < 2)
		return false;
	if (wentDown)
	{
		__super::_SetLocateCursor(true);
	}
	else {
		__super::_SetLocateCursor(false);
	}
	return true;
}

void SelectLineDirTool::_OnRestartTool()
{
	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	_ExitTool();
}

bool SelectLineDirTool::_DoGroups()
{
	return false;
}

bool SelectLineDirTool::_WantDynamics()
{
	return false;
}

bool SelectLineDirTool::_WantAdditionalLocate(DgnButtonEventCP ev)
{
	if (NULL == ev)
		return true;
	return (GetElementAgenda().GetCount() < 1 || ev->IsControlKey());
}


bool SelectLineDirTool::_OnPostLocate(HitPathCP path, WStringR cantAcceptReason)
{
	if (!DgnElementSetTool::_OnPostLocate(path, cantAcceptReason))
		return false;

	ElementHandle eh(path->GetHeadElem(), path->GetRoot());
	
	if (eh.IsValid() && (eh.GetElementType() == ARC_ELM  || eh.GetElementType() == LINE_ELM))
	{
		return true;
	}
	return false;
}

EditElementHandleP SelectLineDirTool::_BuildLocateAgenda(HitPathCP path, DgnButtonEventCP ev)
{
	return __super::_BuildLocateAgenda(path, ev);
}

bool SelectLineDirTool::_OnDataButton(DgnButtonEventCR ev)
{
	HitPathCP   path = _DoLocate(ev, true, ComponentMode::Innermost);
	if (path == NULL)
		return false;

	ElementHandle theElementHandle(mdlDisplayPath_getElem((DisplayPathP)path, 0), mdlDisplayPath_getPathRoot((DisplayPathP)path));
	EditElementHandle ehLine(theElementHandle, ACTIVEMODEL);//选择的线段

	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	SelectionSetManager::GetManager().AddElement(ehLine.GetElementRef(), ACTIVEMODEL);//加入选择集高亮
	if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 1)//正交方式
	{
		m_Ptr->ParsingLineDir(ehLine);
	}
	else if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2)//放射方式
	{
		m_Ptr->ParsingArcLineDir(ehLine);
	}
	else
	{}

	return true;
}


bool SelectLineDirTool::_OnResetButton(DgnButtonEventCR ev)
{
	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	_ExitTool();
	return true;
}




