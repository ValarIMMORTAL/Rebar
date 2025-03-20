#include "_ustation.h"
#include "resource.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "GWallRebarAssembly.h"
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