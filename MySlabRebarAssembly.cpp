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

extern bool SlabPreviewButtonDown;//����Ҫ�������Ԥ����ť
CWallMainRebarDlg*  SelectLineDirTool::m_Ptr = NULL;

int direction;
EditElementHandle m_pDownFace;
MySlabRebarAssembly::MySlabRebarAssembly(ElementId id, DgnModelRefP modelRef) :     // ���캯����ʼ��һЩֵ
	RebarAssembly(id, modelRef),
	m_PositiveCover(0),                          //���汣����
	m_ReverseCover(0),                           //���汣����
	m_SideCover(0),                               //���汣����
	m_RebarLevelNum(4)                            //�ֽ����
{
	Init();
}
void MySlabRebarAssembly::Init()            //����ָ�������ĳ���Ϊm_RebarLevelNum
{
	m_vecDir.resize(m_RebarLevelNum);            //����,0��ʾx�ᣬ1��ʾz��
	m_vecDirSize.resize(m_RebarLevelNum);         //�ߴ�
	m_vecDirSpacing.resize(m_RebarLevelNum);       //���
	m_vecRebarType.resize(m_RebarLevelNum);        //�ֽ�����
	m_vecStartOffset.resize(m_RebarLevelNum);       //���ƫ��
	m_vecEndOffset.resize(m_RebarLevelNum);         //�յ�ƫ��
	m_vecLevelSpace.resize(m_RebarLevelNum);       //��ǰ����
	m_vecDataExchange.resize(m_RebarLevelNum);
	m_vecRebarLevel.resize(m_RebarLevelNum);
	m_vecSetId.resize(m_RebarLevelNum);            //SetId
	//m_vecRebarColor.resize(m_RebarLevelNum);		//�ֽ���ɫ
	m_vecRebarLineStyle.resize(m_RebarLevelNum);	//�ֽ�����
	m_vecRebarWeight.resize(m_RebarLevelNum);		//�ֽ��߿�
	int twinRebarLevel = 0;

	//�������󲢽���Ҫ���ò�һ���Ĳ�
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


void MySlabRebarAssembly::SetConcreteData(Concrete const& concreteData)    //����������������Ϣ�Ͳ���
{
	m_PositiveCover = concreteData.postiveCover;
	m_ReverseCover = concreteData.reverseCover;
	m_SideCover = concreteData.sideCover;
	m_RebarLevelNum = concreteData.rebarLevelNum;
	m_strSlabRebarMethod = concreteData.m_SlabRebarMethod;
}

void MySlabRebarAssembly::SetRebarData(vector<PIT::ConcreteRebar> const& vecData)     //��vector�����ÿ��ֽ����Ϣ
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

void MySlabRebarAssembly::SetRebarEndTypes(vector<PIT::EndType> const & vecEndTypes)       //����˲�
{
	// 	if (vecEndTypes.empty())
	// 	{
	// 		m_vvecEndType = { { {0,0,0} }, { {0,0,0} } };
	// 		return;
	// 	}
	if (vecEndTypes.size())                   //���ö˲�������ж������
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

void MySlabRebarAssembly::GetConcreteData(Concrete& concreteData)             //���������������Ϣ�Ͳ�����Ϣ
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


SlabType MySlabRebarAssembly::JudgeSlabType(ElementHandleCR eh)             //�ж�������ǽ
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


bool MySlabRebarAssembly::IsSlabSolid(ElementHandleCR eh)          //�ж��ǲ���ǽ
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

	/// ɨ���ڰ�Χ�з�Χ�ڵĹ���
	if (!PITCommonTool::CModelTool::AnalysisModelGetElements(range, components, descriptions))
	{
		// ���ؿյļ���
		return ehs;
	}

	const auto PLUS_ID = 1000000;

	ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
	for (const auto &kv : descriptions)
	{
		if (kv.second == NULL) continue;
		auto id = kv.first;
		// ���ﷵ�ص�id���ܼ���һ��PlusID�Է�ֹ�ظ�
		if (id >= PLUS_ID)
		{
			id -= PLUS_ID;
		}
		//const auto component = kv.second;
		// const auto desc = kv.second;

		// ɨ�����е�model_ref���ҵ���Ԫ�����ڵ�model_ref

		for (DgnModelRefP modelRef : modelRefCol)
		{
			EditElementHandle tmpeeh;
			if (tmpeeh.FindByID(id, modelRef) == SUCCESS)
			{
				auto eh = ElementHandle(id, modelRef);
				if (filter(eh))
				{
					// �������������뵽��� 
					ehs.push_back(std::move(eh));
				}
			}

		}
	}

	// ����+ȥ��
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

//����֧����������յ�
void STSlabRebarAssembly::CalculateRebarPts()
{
	m_mapRroundPts.clear();
	m_mapAllRebarpts.clear();
	EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());
	DRange3d eeh_range;//֧�յ�range
	mdlElmdscr_computeRange(&eeh_range.low, &eeh_range.high, testeeh.GetElementDescrCP(), nullptr);

	/*���ĺ�ȣ�ͨ����ȡ���бߣ���ʼ�������ó�Z������ߵĳ���*/
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
	double diameter = RebarCode::GetBarDiameter(strSize, ACTIVEMODEL);		//������10
	double radius = diameter / 2;//���İ뾶
	double endbendRadius = RebarCode::GetPinRadius(strSize, ACTIVEMODEL, false);	//������30,�����뾶
	double positiveCover = GetPositiveCover() * UOR_PER_MilliMeter; //���汣���㣬������֧������ľ���
	double reverseCover = GetReverseCover() * UOR_PER_MilliMeter;	//���汣���㣬�ֽ�֧������İ�
	double sideCover = GetSideCover() * UOR_PER_MilliMeter + stirrup_radius * 2;			//���汣���㣬�����˹���������ֱ��
	if (COMPARE_VALUES(positiveCover, eeh_range.ZLength()) >= 0)
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"���汣������ڵ���֧�յĺ��,�޷������ֽ��", MessageBoxIconType::Information);
		return;
	}
	if (COMPARE_VALUES(reverseCover, m_floor_Thickness) >= 0)
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"���汣������ڵ��ڰ�ĺ��,�޷������ֽ��", MessageBoxIconType::Information);
		return;
	}
	
	double lae_d = stod(strSize.GetWCharCP());
	double lae = lae_d * get_lae();//ê��ĳ���
	m_lae = lae;
	int PtHnum = GetAbanurus_PTRebarData().ptHNum;//����������������ܵ�����
	int PtVnum = GetAbanurus_PTRebarData().ptVNum;//�ݽ��������������ܵ�����

	//���Ϊÿ���������ĵ���
	double Hlength = eeh_range.XLength() - 2 * sideCover - diameter;//�����ܳ�
	double HSpace = Hlength / (PtHnum - 1);							//ÿ������ֽ�ļ��
	double Vlength = eeh_range.YLength() - 2 * sideCover - diameter;//�����ܳ�
	double VSpace = Vlength / (PtVnum - 1);							//ÿ������ֽ�ļ��

	m_HSpace = HSpace;
	m_VSpace = VSpace;

	// 1����������ܵ��ĸ�����������������ڶ����ټ����������
	DPoint3d leftDownStr, rightDownStr, rightUpStr, leftUpStr;
	DPoint3d leftDownEnd, rightDownEnd, rightUpEnd, leftUpEnd;
	vector<Dpoint3d> vec_leftDown, vec_rightDown, vec_rightUp, vec_leftUp;
	vec_leftDown.clear(); vec_rightDown.clear(); vec_rightUp.clear(); vec_leftUp.clear();
	vector<vector<DPoint3d>> vec_Downpts, vec_Rightpts, vec_Uppts, vec_Leftpts;
	vec_Downpts.clear(); vec_Rightpts.clear(); vec_Uppts.clear(); vec_Leftpts.clear();
	stirrup_radius = 0;
	if (lae > m_floor_Thickness - reverseCover)//��Ҫ��ê
	{
		m_isBend = true;
		//����
		leftDownStr.x = eeh_range.low.x + sideCover + radius + stirrup_radius;
		leftDownStr.y = eeh_range.low.y + sideCover + radius + stirrup_radius;
		leftDownStr.z = eeh_range.high.z - positiveCover;
		vec_leftDown.emplace_back(leftDownStr);
		leftDownEnd = leftDownStr;
		leftDownEnd.z = eeh_range.low.z - m_floor_Thickness + reverseCover + radius;
		vec_leftDown.emplace_back(leftDownEnd);
		m_mapRroundPts[0] = vec_leftDown;
		
		//����
		rightDownStr.x = eeh_range.high.x - sideCover - radius - stirrup_radius;
		rightDownStr.y = eeh_range.low.y + sideCover + radius + stirrup_radius;
		rightDownStr.z = eeh_range.high.z - positiveCover;
		vec_rightDown.emplace_back(rightDownStr);
		rightDownEnd = rightDownStr;
		rightDownEnd.z = eeh_range.low.z - m_floor_Thickness + reverseCover + radius;
		vec_rightDown.emplace_back(rightDownEnd);
		m_mapRroundPts[1] = vec_rightDown;

		//����
		rightUpStr.x = eeh_range.high.x - sideCover - radius - stirrup_radius;
		rightUpStr.y = eeh_range.high.y - sideCover - radius - stirrup_radius;
		rightUpStr.z = eeh_range.high.z - positiveCover;
		vec_rightUp.emplace_back(rightUpStr);
		rightUpEnd = rightUpStr;
		rightUpEnd.z = eeh_range.low.z - m_floor_Thickness + reverseCover + radius;
		vec_rightUp.emplace_back(rightUpEnd);
		m_mapRroundPts[2] = vec_rightUp;

		//����
		leftUpStr.x = eeh_range.low.x + sideCover + radius + stirrup_radius;
		leftUpStr.y = eeh_range.high.y - sideCover - radius - stirrup_radius;
		leftUpStr.z = eeh_range.high.z - positiveCover;
		vec_leftUp.emplace_back(leftUpStr);
		leftUpEnd = leftUpStr;
		leftUpEnd.z = eeh_range.low.z - m_floor_Thickness + reverseCover + radius;
		vec_leftUp.emplace_back(leftUpEnd);
		m_mapRroundPts[3] = vec_leftUp;
		
	}
	else//ֱê
	{
		m_isBend = false;
		//����
		leftDownStr.x = eeh_range.low.x + sideCover + radius + stirrup_radius;
		leftDownStr.y = eeh_range.low.y + sideCover + radius + stirrup_radius;
		leftDownStr.z = eeh_range.high.z - positiveCover;
		vec_leftDown.emplace_back(leftDownStr);
		leftDownEnd = leftDownStr;
		leftDownEnd.z = eeh_range.low.z - lae;
		vec_leftDown.emplace_back(leftDownEnd);
		m_mapRroundPts[0] = vec_leftDown;

		//����
		rightDownStr.x = eeh_range.high.x - sideCover - radius - stirrup_radius;
		rightDownStr.y = eeh_range.low.y + sideCover + radius + stirrup_radius;
		rightDownStr.z = eeh_range.high.z - positiveCover;
		vec_rightDown.emplace_back(rightDownStr);
		rightDownEnd = rightDownStr;
		rightDownEnd.z = eeh_range.low.z - lae;
		vec_rightDown.emplace_back(rightDownEnd);
		m_mapRroundPts[1] = vec_rightDown;

		//����
		rightUpStr.x = eeh_range.high.x - sideCover - radius - stirrup_radius;
		rightUpStr.y = eeh_range.high.y - sideCover - radius - stirrup_radius;
		rightUpStr.z = eeh_range.high.z - positiveCover;
		vec_rightUp.emplace_back(rightUpStr);
		rightUpEnd = rightUpStr;
		rightUpEnd.z = eeh_range.low.z - lae;
		vec_rightUp.emplace_back(rightUpEnd);
		m_mapRroundPts[2] = vec_rightUp;

		//����
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
	//�������Ÿֽ��
	if (PtHnum > 2)
	{//��������,�������ܵĵ�
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
	else if(PtHnum == 2)//������ֻ�������������ܵĵ�
	{
		vec_Downpts.emplace_back(vec_leftDown);
		vec_Downpts.emplace_back(vec_rightDown);
		m_mapAllRebarpts[0] = vec_Downpts;

		vec_Uppts.emplace_back(vec_rightUp);
		vec_Uppts.emplace_back(vec_leftUp);
		m_mapAllRebarpts[2] = vec_Uppts;
	}
	if (PtVnum > 2)//�������ֽ�ֻ�������Ͳ������������ŵĵ㣬�������žͰ��������ܵĵ�
	{//�������ţ����������ܵĵ�
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
	////���������뾶
	//double stirrup_bendRadius = RebarCode::GetPinRadius(stirrupstrSize, ACTIVEMODEL, true);
	//WString ptstrSize(GetAbanurus_PTRebarData().ptrebarSize);
	//if (ptstrSize.find(L"mm") != WString::npos)
	//{
	//	ptstrSize.ReplaceAll(L"mm", L"");
	//}
	////���뾶
	//double pt_radius = RebarCode::GetBarDiameter(ptstrSize, ACTIVEMODEL) / 2;

	//double move_sqrtDistance = sqrt(pt_radius);
	WString stirrupstrSize(GetAbanurus_PTRebarData().stirrupRebarsize);
	if (stirrupstrSize.find(L"mm") != WString::npos)
	{
		stirrupstrSize.ReplaceAll(L"mm", L"");
	}
	//����ֱ��
	double stirrup_diameter = RebarCode::GetBarDiameter(stirrupstrSize, ACTIVEMODEL);
	//double offset_distance = stirrup_diameter;
	int Hnum = GetAbanurus_PTRebarData().ptHNum;
	int Vnum = GetAbanurus_PTRebarData().ptVNum;

	m_mapActualAllRebarpts = m_mapAllRebarpts;

	if (Hnum % 2 == 1)//����
	{
		//����һ��
		for (int i = 0; i < m_mapActualAllRebarpts[0].size() - 1; ++i)
		{
			if (i % 2)//��������ƫ�ƣ�˫������ƫ��
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
		//����һ��
		for (int i = 1; i < m_mapActualAllRebarpts[2].size(); ++i)
		{
			if (i % 2)//��������ƫ�ƣ�˫������ƫ��
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
	else if (Hnum % 2 == 0)//˫��
	{
		//����һ��
		for (int i = 0; i < m_mapActualAllRebarpts[0].size(); ++i)
		{
			if (i % 2)//��������ƫ�ƣ�˫������ƫ��.���涼����ƫ��
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
		//����һ��
		for (int i = 0; i < m_mapActualAllRebarpts[2].size(); ++i)
		{
			if (i % 2)//��������ƫ�ƣ�˫������ƫ��.���涼����ƫ��
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

	if (Vnum > 2)//�ݽ���������2 �����漰���������ߴ洢������
	{
		//�ұ�һ��
		for (int i = 0; i < m_mapActualAllRebarpts[1].size(); ++i)
		{
			if (i % 2)//��������ƫ�ƣ�˫�����£�������ƫ��
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
		//���һ��
		if (Vnum % 2)//˫���£������ϣ�������
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
		else//˫���ϣ������£�������
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

	//������Ϊ��ֵ
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
	// GetContractibleeeh(eeh2);//��ȡ��ȥ������Ķ˲���С��ʵ��

	double dSideCover = GetSideCover() * uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	vector<vector<DPoint3d>> vecPtRebars;
	vector<DPoint3d> tmpptsTmp;
	vecPtRebars.clear();
	tmpptsTmp.clear();
	if (m_pOldElm != NULL) // ��ԭʵ���ཻ(�޿׶�)
	{
		GetIntersectPointsWithOldElm(tmpptsTmp, m_pOldElm, pt1[0], pt1[1], dSideCover, matrix);

		if (tmpptsTmp.size() > 1)
		{
			// ���ڽ���Ϊ�������ϵ����
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

		if (!isTwin)//�ǲ���ʱ����Ҫ�洢����Ϣ
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
	vector<PIT::EndType> const& endType,	//�洢���˲����յ�˲�����
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

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);	//������30
	double adjustedXLen, adjustedSpacing;
	int numRebar = 0;

	double leftSideCov, rightSideCov, allSideCov;
	leftSideCov = GetSideCover()*uor_per_mm;
	rightSideCov = GetSideCover()*uor_per_mm;
	allSideCov = leftSideCov + rightSideCov;


	if (twinBarInfo.hasTwinbars)	//����
		adjustedXLen = xLen - allSideCov - diameter - diameterTb /*- startOffset*/ - endOffset;
	else
		adjustedXLen = xLen - allSideCov - diameter /*- startOffset*/ - endOffset;
	//	double adjustedXLen = xLen - 2.0 * GetSideCover()*uor_per_mm - diameter - startOffset - endOffset;

	if (bTwinbarLevel)				//�����ֽ�����
	{
		numRebar = (int)floor(adjustedXLen / (spacing * (twinBarInfo.interval + 1)) + 0.5) + 1;
		int numRebar1 = (int)floor(adjustedXLen / spacing + 0.5) + 1;
		adjustedSpacing = spacing;
		if (numRebar1 > 1)
		{
			adjustedSpacing = adjustedXLen / (numRebar1 - 1);	//�ǲ����ƽ�����
			adjustedSpacing *= (twinBarInfo.interval + 1);		//�����ʵ�ʼ������Ըֽ���
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
	if (bTwinbarLevel)				//�������ƫ��һ���ֽ�ľ���
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
	for (int i = 0; i < numRebar; i++)//�ֽ�����
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
	}//rebarset����rebarelement�����������
	//�ֽ���
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
		if (m_strSlabRebarMethod != 2) // ��㡢�յ�һ��  ��Բ�θֽ�
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
		if (!SlabPreviewButtonDown)//Ԥ����־,Ԥ��״̬�²����ɸֽ�
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
	m_vecAllRebarStartEnd.push_back(vecStartEnd);//�洢�����߶�
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	CString spacingstring;
	spacingstring.Format(_T("%lf"), spacing / uor_per_mm);
	setdata.SetSpacingString(spacingstring);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

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
	ret = rebarSet->FinishUpdate(setdata, ACTIVEMODEL);	//���ص��Ǹֽ�����

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

bool STSlabRebarAssembly::MakeRebars(DgnModelRefP modelRef)      // �����ֽ�
{
	if (m_isAbanurus)
	{
		rsetTags.Clear(true);
		EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());

		DRange3d eeh_range;//֧�յ�range
		mdlElmdscr_computeRange(&eeh_range.low, &eeh_range.high, testeeh.GetElementDescrCP(), nullptr);

		//1. ɨ��֧������İ�
		eeh_range.low.z -= 2 * UOR_PER_MilliMeter;
		m_downFloor = 
			scan_elements_in_range(
				eeh_range,
				[&](const ElementHandle &eh) -> bool {
			if (eh.GetElementId() == testeeh.GetElementId())
			{
				// ���˵��Լ�
				return false;
			}
			// ֻ��Ҫ��
			return is_Floor(eh);
		});

		//2. ����ֽ������յ�
		CalculateRebarPts();

		//3. �ƶ��ֽ�ĵ�
		MoveRebarPts();
	
		double reverseCover = GetReverseCover() * UOR_PER_MilliMeter;
		BrString sizekey = GetAbanurus_PTRebarData().ptrebarSize;
		double bendRadius = RebarCode::GetPinRadius(sizekey, ACTIVEMODEL, false);
		double bendLength = 0;
		double diameter = RebarCode::GetBarDiameter(sizekey, ACTIVEMODEL);		//������10
		double radius = diameter / 2;//���İ뾶
		int setCount = 0;
		//3. ���ݴ洢ÿһ�ŵĸֽ��ȥ���ɸֽ�,�������������������������2���Ͳ������������Ÿֽ�
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
		if (COMPARE_VALUES(dSideCover, m_ldfoordata.Zlenth) >= 0)	//������汣������ڵ���ǽ�ĳ���
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
		int setCount = 0;
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
		DVec3d vecZ = DVec3d::From(0, 0, 1);
		for (int i = 0; i < iRebarLevelNum; ++i)
		{
			vecZ.Normalize();
			//��ȡÿһ�����λ��
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
			vTrans[i].y = 0;//y��������ƫ��
			double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;
			// ������ϵ
			double dis_x = 0;//X����ƫ��
			double dis_y = 0;//Y����ƫ��
			int tmpLevel;
			if (GetvecDataExchange().at(i) == 0)//ǰ���,�ϲ��ֽ��
			{
				frontlevel++;
				tmpLevel = frontlevel;
				//a������ϲ���ǽ���ϲ���Ϊ�ڲ���
					//�ֽ��VEC����
					   //(1)��ȡ��������VEC����ͬ����棬��ǽ���ȥ��õ�����棻
					   //(2)������ͬ��������������жϸֽ���������յ��Ƿ�Ϊ�����������������յ㣬
							  //����ǣ����ԣ����ǣ���Զ����������ĵķ���,������һ���ֽƫ�ƾ���2��������+���ڼ��� - 1�����ֽ�ֱ��
					   //(3)���ֽ�ȼ��㣬����ϲ���û����ֽ��ֱ��ǽ���յ������������
							  //�д�ֱǽ�����жϵ�ǰ�ֽ�㣬�ֽ�ǰ�滹�в㣬������С2���ֽ�ֱ����û�в���С1���ֽ�ֱ�����ֽ�ê�봦��
							  //û�д�ֱǽ���ֽ�Ȳ�������
				//b������ϲ�û��ǽ���ϲ���Ϊ�����
					 //�ֽ��VEC����
					 //��1���ֽ�ȼ��㣬�²���û����ֽ��ֱ��ǽ���յ��²�������������
							  //�д�ֱǽ�����жϵ�ǰ�ֽ�㣬�ֽ�ǰ�滹�в㣬��㲿�ֳ�����С1���ֽ�ֱ����û�в�������С���ֽ�ê�봦��
							  //û��ǽ���ֽ�Ȳ�������
			}
			else if (GetvecDataExchange().at(i) == 2)//����㣬�²��ֽ��
			{
				tmpLevel = backlevel;
				backlevel--;
				//a������²���ǽ���²���Ϊ�ڲ���
					//�ֽ��VEC����
					   //(1)��ȡ��������VEC����ͬ����棬��ǽ���ȥ�����õ�����棻
					   //(2)������ͬ��������������жϸֽ���������յ��Ƿ�Ϊ�����������������յ㣬
							  //����ǣ����ԣ����ǣ���Զ����������ĵķ���,������һ���ֽƫ�ƾ���2��������+���ڼ��� - 1�����ֽ�ֱ��
					   //(3)���ֽ�ȼ��㣬����ϲ���û����ֽ��ֱ��ǽ���յ������������
							  //�д�ֱǽ�����жϵ�ǰ�ֽ�㣬�ֽ�ǰ�滹�в㣬������С2���ֽ�ֱ����û�в���С1���ֽ�ֱ�����ֽ�ê�봦��
							  //û�д�ֱǽ���ֽ�Ȳ�������
				//b������²�û��ǽ���²���Ϊ�����
					 //�ֽ��VEC����
					 //��1���ֽ�ȼ��㣬�²���û����ֽ��ֱ��ǽ���յ��²�������������
							  //�д�ֱǽ�����жϵ�ǰ�ֽ�㣬�ֽ�ǰ�滹�в㣬��㲿�ֳ�����С1���ֽ�ֱ����û�в�������С���ֽ�ê�봦��
							  //û��ǽ���ֽ�Ȳ�������


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
			if (GetvecDir().at(i) == 1)	//����ֽ�
			{
				vector<CVector3D> vecEndNormal(2);
				CVector3D	endNormal;	//�˲��乳����
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
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
						vecEndNormal[k] = endNormal;
					}
				}

				if (m_strSlabRebarMethod != 2)
				{
					mat.SetTranslation(vTrans[i]);
					mat = GetPlacement() * mat;
				}
				else //��������ʽ����Ҫ *  GetPlacement() 
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
				else //��������ʽ����Ҫ *  GetPlacement() 
				{
					vTransTb[i].z = vTransTb[i].y;
					vTransTb[i].x = 0.0;
					vTransTb[i].y = 0.0;
					matTb.SetTranslation(vTransTb[i]);

				}

				//���Ʋ���--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
				{
					//�Ȼ��Ʒǲ����
					PopvecSetId().push_back(0);
					setCount++;
					tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + dis_x,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
					if (NULL != tag)
					{
						tag->SetBarSetTag(setCount);
						rsetTags.Add(tag);
					}

					//���Ʋ����
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
				else //��ǰ��δ���ò���
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
				CVector3D	endNormal;	//�˲��乳����
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
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
						vecEndNormal[k] = endNormal;
					}
				}
				mat = rot90;
				if (m_strSlabRebarMethod != 2)
				{
					mat.SetTranslation(vTrans[i]);
					mat = GetPlacement() * mat;
				}
				else //��������ʽ����Ҫ *  GetPlacement() 
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
				else //��������ʽ����Ҫ *  GetPlacement() 
				{
					vTransTb[i].z = vTransTb[i].y;
					vTransTb[i].x = 0.0;
					vTransTb[i].y = 0.0;
					matTb.SetTranslation(vTransTb[i]);
				}
				//������Ϊ�����,ż����Ϊ��ͨ��

				//���Ʋ���--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
				{
					//�Ȼ��Ʒǲ����
					PopvecSetId().push_back(0);
					setCount++;
					tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(i), dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + dis_y,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, GetvecTwinRebarLevel().at(i), false, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i), modelRef);
					if (NULL != tag)
					{
						tag->SetBarSetTag(setCount);
						rsetTags.Add(tag);
					}
					//���Ʋ����
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
				else //��ǰ��δ���ò���
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

		if (SlabPreviewButtonDown)//Ԥ����ť���£���������
		{
			int index = 0;
			m_allLines.clear();
			//�����������Ϣ
			for (auto it = m_vecAllRebarStartEnd.begin(); it != m_vecAllRebarStartEnd.end(); it++, index++)
			{

				UInt32 colors = 3;//��ɫ
				int style = 0;//ʵ��
				UInt32	weight = 2;

				if (GetvecDir().at(index) == 0)//�����ˮƽ����
					colors = 1;//��ɫ

				if (GetvecDataExchange().at(index) == 2)//������ڲ�
					style = 2;//����
				if (Gallery::WallHelper::analysis_slab_isTop(ehSel))//����Ƕ�����Ҫ�л�
				{
					if (style == 2)//���߻���ʵ��
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
				strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//ɾ��mm

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
			tieRebarMaker.GetRebarPts(vctTieRebarLines);//ȡ�����е�����ֱ����Ϣ
			if (NULL != tag && (!SlabPreviewButtonDown))
			{
				tag->SetBarSetTag(iRebarLevelNum + 1);
				rsetTags.Add(tag);
			}
		}

		if (SlabPreviewButtonDown)//Ԥ����ť���£���������
		{
			// 		m_allLines.clear();
			// 		//�����������Ϣ
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
					//����ֱ����Ϣ
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

	MakeRebars(modelRef);//���ô����ֽ�
	Save(modelRef);

	ElementId contid = FetchConcrete();
	return true;
}



void STSlabRebarAssembly::SetSlabRebarDir(DSegment3d& Seg, ArcRebar& mArcLine)//�����ķ���
{

}

//�ֲ����ƽ����XOZƽ��
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

	CVector3D  yVec = tmpz;     //�������������ģ������������y  	
	yVec.Scale(-1);
	CVector3D  xVecNew(ptStart, ptEnd);
	xVecNew.Normalize();
	bool isXtY = false;
	tmpz.Scale(m_STslabData.width);
	ptOrgin.Add(tmpz);
	BeMatrix   placement = CMatrix3D::Ucs(ptOrgin, xVecNew, yVec, isXtY);		//����ΪX�ᣬˮƽ��ֱ����ΪY��
	//placement.SetScaleFactors(1, 1, -1);
	SetPlacement(placement);
	PopvecFrontPts().push_back(ptStart);
	PopvecFrontPts().push_back(ptEnd);
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}

//��ȡ��嶥���׵�ǽ��
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
	//��ȡ����ǽ
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

	//��ȡ����ǽ
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
				EditElementHandle dface;//ǽ����
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
	//����ָ��Ԫ����������Ԫ�صķ�Χ��
	mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
	if (!Eleeh.IsValid())
	{
		mdlDialog_dmsgsPrint(L"�Ƿ��İ�ʵ��!");
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
	mdlRMatrix_fromVectorToVector(&rMatrix, &facenormal, &vecZ);//��ת��xoyƽ��
	mdlTMatrix_fromRMatrix(&trans, &rMatrix);
	mdlTMatrix_setOrigin(&trans, &minPos);
	copyEleeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(copyEleeh, TransformInfo(trans));
	downface.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(downface, TransformInfo(trans));

	double tmpangle = facenormal.AngleTo(vecZ);
	if (tmpangle > PI / 2)
	{
		tmpangle = PI - tmpangle;
	}
	//����ָ��Ԫ����������Ԫ�صķ�Χ��
	mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
	DPoint3d minP, maxP;
	//����ָ��Ԫ����������Ԫ�صķ�Χ��
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

	//����������Ĳ���
	m_STslabData.height = (maxP.y - minP.y)*uor_now / uor_ref;
	m_STslabData.length = (maxP.x - minP.x)*uor_now / uor_ref;
	m_STslabData.width = (maxP2.z - minP2.z)*uor_now / uor_ref;
	m_STslabData.ptStart = minP;
	m_STslabData.ptEnd = minP;
	m_STslabData.ptEnd.x = maxP.x;

	//���㵱ǰZ���귽��
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


//ѡ���߶Σ�ȷ��������
void SelectLineDirTool::InstallNewInstance(int toolId, CWallMainRebarDlg *Ptr)
{
	mdlSelect_freeAll();
	//�����ѡ�е�Ԫ��
	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	//��������
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
	EditElementHandle ehLine(theElementHandle, ACTIVEMODEL);//ѡ����߶�

	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	SelectionSetManager::GetManager().AddElement(ehLine.GetElementRef(), ACTIVEMODEL);//����ѡ�񼯸���
	if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 1)//������ʽ
	{
		m_Ptr->ParsingLineDir(ehLine);
	}
	else if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2)//���䷽ʽ
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




