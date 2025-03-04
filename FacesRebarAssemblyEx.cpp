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
#include "CFacesRebarDlgEx.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "FacesRebarAssemblyEx.h"
#include <CPointTool.h>
#include "PITMSCECommon.h"
#include "XmlHelper.h"
#include "SelectRebarTool.h"

using namespace PIT;
extern bool g_FacePreviewButtonsDown;

FacesRebarAssemblyEx::FacesRebarAssemblyEx(ElementId id, DgnModelRefP modelRef) :
	PITRebarAssembly(id, modelRef)
{
	m_UseXOYDir = false;
	Init();
}

FacesRebarAssemblyEx::~FacesRebarAssemblyEx()
{
	std::for_each(m_useHoleehs.begin(), m_useHoleehs.end(), [](EditElementHandle* &eh) {delete eh, eh = NULL; });
	std::for_each(m_Negs.begin(), m_Negs.end(), [](EditElementHandle* &eh) {delete eh, eh = NULL; });
}

void FacesRebarAssemblyEx::Init()
{
	memset(&m_Concrete, 0, sizeof(Concrete));
}

void FacesRebarAssemblyEx::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

void FacesRebarAssemblyEx::DrawPoint(const DPoint3d& point, int color, EditElementHandle& eehPoint, DgnModelRefP modelRef)
{
	// �����㴮������һ���㣩
	bvector<DPoint3d> pointArray = { point };
	PointStringHandler::CreatePointStringElement(eehPoint, nullptr, pointArray.data(), nullptr, pointArray.size(), false, modelRef->Is3d(), *modelRef);

	// ������ɫ & ͼ��
	ElementPropertiesSetterPtr prop = ElementPropertiesSetter::Create();
	prop->SetWeight(10);
	prop->SetColor(color);
	prop->Apply(eehPoint);

	// ��ӵ�ģ��
	eehPoint.AddToModel();
}

/**
 * @brief ����Ԥ����
 *
 * �ú�����ȫ�ֱ���g_FacePreviewButtonsDownΪtrueʱִ�У����Ƹֽ��Ԥ���ߡ�
 * �����洢�˸ֽ���ʼ��ͽ������m_vecRebarStartEnd������
 * ����ÿ��Ԫ�أ�����������߶ε�ģ���У�������Щ�߶δ洢��m_allLines�������Ա����ʹ�á�
 */
void FacesRebarAssemblyEx::DrawPreviewLines()
{
	if (g_FacePreviewButtonsDown)//��Ԥ����
	{
		// ԭ����Ԥ����ֻ������ֹ�㣬������ê�ֽ�ʱ��������⣬�������û�л���Ԥ���ߣ��ο���������Ԥ���ߵ�ɼ���ʽ
		for (auto rebarCurves : m_vecRebarCurvePt)
		{
			for (auto rebarCurve : rebarCurves)
			{
				bvector<DPoint3d> point3ds;
				rebarCurve.GetIps(point3ds);
				EditElementHandle eehStrPoint, eehEndPoint;
				DrawPoint(point3ds[0], 1, eehStrPoint, ACTIVEMODEL);// ��ɫ���
				DrawPoint(point3ds[point3ds.size() - 1], 4, eehEndPoint, ACTIVEMODEL);// ��ɫ�յ�
				m_allPreViewEehs.push_back(eehStrPoint.GetElementRef());// �洢���
				m_allPreViewEehs.push_back(eehEndPoint.GetElementRef());// �洢�յ�
				EditElementHandle eeh;
				// LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				LineStringHandler::CreateLineStringElement(eeh, nullptr, point3ds.data(), point3ds.size(), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allPreViewEehs.push_back(eeh.GetElementRef());//�洢���л���
			}
		}
	}
}

void FacesRebarAssemblyEx::ClearLines()
{
	if (m_isClearLine)
	{
		for (ElementRefP tmpeeh : m_allPreViewEehs)
		{
			if (tmpeeh != nullptr)
			{
				EditElementHandle eeh(tmpeeh, tmpeeh->GetDgnModelP());
				eeh.DeleteFromModel();
			}
		}
		m_allPreViewEehs.clear();
	}
}

FacesRebarAssemblyEx::FaceType FacesRebarAssemblyEx::JudgeFaceType(EditElementHandleR eehFace, DgnModelRefP modelRef)
{
	FaceType faceType = FaceType::other;

	if (!eehFace.IsValid())
	{
		return faceType;
	}
	DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
	DPoint3d ptNormal;
	mdlElmdscr_extractNormal(&ptNormal, nullptr, eehFace.GetElementDescrCP(), &ptDefault);
	if (ptDefault.IsParallelTo(ptNormal))
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

void FacesRebarAssemblyEx::SetRebarEndTypes(vector<PIT::EndType> const & vecEndTypes)
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