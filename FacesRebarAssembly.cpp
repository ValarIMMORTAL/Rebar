/*--------------------------------------------------------------------------------------+
|
|     $Source: WallRebarAssembly.cpp $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "CFacesRebarDlg.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "FacesRebarAssembly.h"

#include <CModelTool.h>
#include <CPointTool.h>

#include "PITMSCECommon.h"
#include "XmlHelper.h"
#include "SelectRebarTool.h"
#include "RebarHelper.h"
#include "CFaceTool.h"
#include "CElementTool.h"
#include "WallHelper.h"
#include "SlabHelper.h"
#include "CSolidTool.h"

#define  Waterstops_Width (60*UOR_PER_MilliMeter)

using namespace Gallery;

using namespace PIT;
extern bool FacePreviewButtonsDown;
bool FacesRebarAssembly::GetIntersectPointsWithOldElmOwner(vector<DPoint3d>& interPoints, EditElementHandleP oldElm, DPoint3d& ptstr, DPoint3d& ptend, double dSideCover)
{
	EditElementHandle lineeeh;
	LineHandler::CreateLineElement(lineeeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
	ISolidKernelEntityPtr LineentityPtr;
	bool isInside = false;

	//EditElementHandle arceeh;
	//ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(ptstr, ptmid, ptend), true, *ACTIVEMODEL);
	//ISolidKernelEntityPtr LineentityPtr;
	if (SolidUtil::Convert::ElementToBody(LineentityPtr, lineeeh) == SUCCESS)
	{
		if (oldElm->IsValid())
		{
			EditElementHandle tmpeeh;
			tmpeeh.Duplicate(*oldElm);

			vector<DPoint3d>	vecresult;
			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, tmpeeh) == SUCCESS)
			{
				if (BodyIntersect(vecresult, LineentityPtr, entityPtr)) // �ཻ�ĵ�
				{
					if (vecresult.size() > 0)
					{
						double minStartDistance = ptstr.Distance(ptend);
						double minEndDistance = ptstr.Distance(ptend);
						Dpoint3d pt1 = ptstr;
						Dpoint3d pt2 = ptend;
						for (size_t i = 0; i < vecresult.size(); i++)
						{
							double startDistance = pt1.Distance(vecresult[i]);
							double endDistance = pt2.Distance(vecresult[i]);
							if (startDistance < minStartDistance)
							{
								minStartDistance = startDistance;
								ptstr = vecresult[i];
							}
							if (endDistance < minEndDistance)
							{
								minEndDistance = endDistance;
								ptend = vecresult[i];
							}
						}
					}
					isInside = true;
				}
				else
				{
					//if (!vecresult.empty() && !vecresult[0].IsEqual(vecresult[1]))
					//{
					//	// ����һ�����Ƿ��ڸ���������ڲ���߽��� 
					//	double leftSideCov = dSideCover;
					//	double rightSideCov = dSideCover;
					//	double allSideCov = leftSideCov + rightSideCov;
					//	DPoint3d vec = ptend - ptstr;
					//	if (SolidUtil::IsPointInsideBody(*entityPtr, ptstr) && ptend.Distance(vecresult[0]) < ptstr.Distance(vecresult[0]))
					//	{
					//		ptend = vecresult[0];
					//		vec.Negate();
					//		vec.ScaleToLength(rightSideCov);
					//		ptend.Add(vec);
					//		isInside = true;
					//	}
					//	else if (SolidUtil::IsPointInsideBody(*entityPtr, ptend) && ptend.Distance(vecresult[0]) > ptstr.Distance(vecresult[0]))
					//	{
					//		ptstr = vecresult[0];
					//		vec.Normalize();
					//		vec.ScaleToLength(leftSideCov);
					//		ptstr.Add(vec);
					//		isInside = true;
					//	}
					//}
					// ����һ�����Ƿ��ڸ���������ڲ���߽��� 
					if (SolidUtil::IsPointInsideBody(*entityPtr, ptstr))
					{
						isInside = true;
					}
					if (SolidUtil::IsPointInsideBody(*entityPtr, ptend))
					{
						isInside = true;
					}
				}
			}
		}
	}

	return isInside; // ����յ㶼û����ʵ������
}

int FacesRebarAssembly::IsHaveVerticalWall(DPoint3d ptstr, DPoint3d ptend, MSElementDescrP tmpfaces[20], int facenum, bool isGetFaceType)
{
	int revalue = 0;
	DVec3d vecRebar = ptend - ptstr;
	vecRebar.Normalize();

	vector<MSElementDescrP> verfacesstr;//������ʼ�㣬�����ߴ�ֱ����
	vector<MSElementDescrP> verfaceend;
	for (int i = 0; i < facenum; i++)
	{
		DPoint3d tmpstr, tmpend;
		tmpstr = tmpend = DPoint3d::From(0, 0, 0);
		PITCommonTool::CElementTool::GetLongestLineMidPt(tmpfaces[i], tmpstr, tmpend);
		DVec3d vectmp = tmpend - tmpstr;
		vectmp.Normalize();
		//��Ϊ���Ż��׶���Χ��ǽ��ֽ��ƽ��ʱ���ֽ�Ҳ��Ҫ��ǽ��ê��ע�͵�����ʱ���ע�Ͳ������ж�������
		if (!isGetFaceType || abs(vectmp.DotProduct(vecRebar)) < 0.01)
		{
			EditElementHandle tmpeeh(tmpfaces[i], false, false, ACTIVEMODEL);
			if (ISPointInElement(&tmpeeh, ptstr))
			{
				verfacesstr.push_back(tmpfaces[i]);
			}
			else if (ISPointInElement(&tmpeeh, ptend))
			{
				verfaceend.push_back(tmpfaces[i]);
			}
		}
	}

	if (verfacesstr.size() == 0 && verfaceend.size() == 0)
	{
		revalue = 0;//�����û�д�ֱǽ
	}
	else if (verfacesstr.size() > 0 && verfaceend.size() > 0)
	{
		revalue = 3;//������д�ֱǽ
	}
	else if (verfacesstr.size() > 0)
	{
		revalue = 1;//��ʼ���д�ֱǽ
	}
	else
	{
		revalue = 2;//��ֹ���д�ֱǽ
	}
	return revalue;
}

//�Ƿ���ƽ��ǽ
bool FacesRebarAssembly::IsHaveParaWall(DPoint3d ptstr, DPoint3d ptend, MSElementDescrP tmpfaces[20], int facenum, bool isGetFaceType)
{
	int revalue = 0;
	double totalLength = ptstr.Distance(ptend);
	DVec3d vecRebar = ptend - ptstr;
	vecRebar.Normalize();
	DPoint3d midpos = ptstr;
	midpos.Add(ptend);
	midpos.Scale(0.5);
	vector<MSElementDescrP> parafaces;//������ʼ�㣬������ƽ�е���
	for (int i = 0; i < facenum; i++)
	{
		DPoint3d tmpstr, tmpend;
		tmpstr = tmpend = DPoint3d::From(0, 0, 0);
		PITCommonTool::CElementTool::GetLongestLineMidPt(tmpfaces[i], tmpstr, tmpend);
		DVec3d vectmp = tmpend - tmpstr;
		vectmp.Normalize();
		double length = tmpstr.Distance(tmpend);
		if (isGetFaceType && length < totalLength * 0.8)// ��ʱ���ݳ��ȹ��˼�ˮ�ӵĶ�ǽ
			continue;
		if (abs(vectmp.DotProduct(vecRebar)) > 0.9)
		{
			EditElementHandle tmpeeh(tmpfaces[i], false, false, ACTIVEMODEL);
			if (ISPointInElement(&tmpeeh, ptstr) || ISPointInElement(&tmpeeh, ptend) || ISPointInElement(&tmpeeh, midpos))
			{
				return true;
			}
		}
	}
	return false;
}

void FacesRebarAssembly::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

// @brief �����������ֱ����ĸֽ��Ƿ���Ҫ�任ê�̷����ê�̳��ȣ�;
// @param range_eeh��		��ˮ��ʵ���range
// @param range_CurFace��	��ǰ������range
// @return �����Ҫ����true
// @Add by tanjie, 2024.1.9
bool FacesRebarAssembly::AnalyseOutsideFace(DRange3d &range_eeh, DRange3d &range_CurFace)
{
	//DRange3d range_CurFace;
	//mdlElmdscr_computeRange(&range_CurFace.low, &range_CurFace.high, m_CurentFace->GetElementDescrCP(), nullptr);
	//��ǰ������ĵ�
	DPoint3d midPt = DPoint3d::From((range_CurFace.low.x + range_CurFace.high.x) / 2, (range_CurFace.low.y + range_CurFace.high.y) / 2, (range_CurFace.low.z + range_CurFace.high.z) / 2);
	//��ǰ��ķ�����
	DPoint3d faceNormal = GetfaceNormal();
	faceNormal.Normalize();
	faceNormal.ScaleToLength(30);
	/*DPoint3d maxpt;
	mdlElmdscr_extractNormal(&facenormal, &maxpt, m_CurentFace->GetElementDescrCP(), NULL);*/
	DPoint3d midPtFront = midPt;
	midPtFront.Add(faceNormal);
	DPoint3d midPtBack = midPt;
	faceNormal.Negate();
	midPtBack.Add(faceNormal);
	bool isChange = false;
	for (auto it : m_AllFloors)
	{
		DRange3d range_FloorFace;
		mdlElmdscr_computeRange(&range_FloorFace.low, &range_FloorFace.high, it.GetElementDescrCP(), nullptr);
		if (range_FloorFace.IsContainedXY(midPt) && range_FloorFace.IsContainedXY(midPtFront) && range_FloorFace.IsContainedXY(midPtBack))
			isChange = true;

	}
	if (m_AllFloors.size() == 2 && faceNormal.IsParallelTo(DVec3d::UnitY()) && m_CatchpitType == 0)
	{//��׼��ˮ����Щ��ģ����ˮ�������ߵ�һ�µ��ǽ���ֿ��������鵥���İ�
		DRange3d range1, range2;
		mdlElmdscr_computeRange(&range1.low, &range1.high, m_AllFloors.front().GetElementDescrCP(), nullptr);
		mdlElmdscr_computeRange(&range2.low, &range2.high, m_AllFloors.back().GetElementDescrCP(), nullptr);
		if (COMPARE_VALUES_EPS(range1.high.z, range_eeh.high.z, 40 * UOR_PER_MilliMeter) == 0 &&
			COMPARE_VALUES_EPS(range2.high.z, range_eeh.high.z, 40 * UOR_PER_MilliMeter) == 0)
			isChange = true;
	}
	
	return isChange;
}

FacesRebarAssembly::FacesRebarAssembly(ElementId id, DgnModelRefP modelRef) :
	PITRebarAssembly(id, modelRef)
{
	m_UseXOYDir = false;
	m_VeticalPlanes.clear();
	//m_AllFloors.clear();
	Init();
}

FacesRebarAssembly::~FacesRebarAssembly()
{
	std::for_each(m_useHoleehs.begin(), m_useHoleehs.end(), [](EditElementHandle* &eh) {delete eh, eh = NULL; });
	std::for_each(m_Negs.begin(), m_Negs.end(), [](EditElementHandle* &eh) {delete eh, eh = NULL; });
}

void FacesRebarAssembly::Init()
{
	memset(&m_Concrete, 0, sizeof(Concrete));
}


void FacesRebarAssembly::ClearLines()
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

FacesRebarAssembly::FaceType FacesRebarAssembly::JudgeFaceType(EditElementHandleR eehFace, DgnModelRefP modelRef)
{
	FaceType faceType = FaceType::other;

	if (!eehFace.IsValid())
	{
		return faceType;
	}
	DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
	DPoint3d ptNormal;
	mdlElmdscr_extractNormal(&ptNormal, nullptr, eehFace.GetElementDescrCP(), &ptDefault);
	if (/*ptDefault.IsParallelTo(ptNormal)*/abs(ptNormal.DotProduct(ptDefault)) > 0.9)
	{
		return FaceType::Plane;
	}

	ElementAgenda agenda;//��Ŵ�ɢ֮���Ԫ��
	DropGeometryPtr pDropGeometry = DropGeometry::Create();//����һ��DropGeometryʵ�������ü���ѡ��
	if (eehFace.GetElementType() == CMPLX_SHAPE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Complex);
	}
	else if (eehFace.GetElementType() == SURFACE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Solids);
	}
	else
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_LinearSegments);
	}
	if (SUCCESS != eehFace.GetDisplayHandler()->Drop(eehFace, agenda, *pDropGeometry))
	{
		agenda.Clear();
		mdlOutput_printf(MSG_STATUS, L"ͬ���ͼʱ��ɢǽ�ĵ���ʧ�ܣ�");
		return faceType;
	}

	int flag = 0;
	double zPre = 0;
	for (EditElementHandleR LineEeh : agenda)
	{
		if (LineEeh.GetElementType() == ARC_ELM)
		{
			faceType = FaceType::CamberedSurface;
			DEllipse3d ellipsePro;
			DPoint3d ptStart, ptEnd;
			mdlArc_extractDEllipse3d(&ellipsePro, LineEeh.GetElementP());
			ellipsePro.EvaluateEndPoints(ptStart, ptEnd);
			if (0 == flag)
			{
				zPre = ptStart.z;
			}
			if (++flag >= 2)
			{
				if (COMPARE_VALUES(zPre, ptStart.z) != 0)
				{
					return faceType;
				}
			}
		}
		else
		{
			DPoint3d pt[2];
			mdlLinear_extract(pt, NULL, LineEeh.GetElementP(), LineEeh.GetModelRef());
			DVec3d vec = pt[1] - pt[0];
			if (vec.IsPerpendicularTo(DVec3d::From(1, 0, 0)) || vec.IsPerpendicularTo(DVec3d::From(0, 1, 0)))
			{
				faceType = FaceType::Plane;
			}
		}
	}

	return faceType;
}

void FacesRebarAssembly::SetRebarEndTypes(vector<PIT::EndType> const & vecEndTypes)
{
	if (vecEndTypes.size())
		m_vecEndTypes.clear();

	vector<PIT::EndType> vec;
	vec.reserve(2);
	for (size_t i = 0; i < vecEndTypes.size(); i++)
	{
		if (i & 0x01)
		{
			vec.push_back(vecEndTypes[i]);
			m_vecEndTypes.push_back(vec);
			vec.clear();
		}
		else
		{
			vec.push_back(vecEndTypes[i]);
		}
	}
}
