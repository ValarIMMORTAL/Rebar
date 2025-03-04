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
	// 创建点串（包含一个点）
	bvector<DPoint3d> pointArray = { point };
	PointStringHandler::CreatePointStringElement(eehPoint, nullptr, pointArray.data(), nullptr, pointArray.size(), false, modelRef->Is3d(), *modelRef);

	// 设置颜色 & 图层
	ElementPropertiesSetterPtr prop = ElementPropertiesSetter::Create();
	prop->SetWeight(10);
	prop->SetColor(color);
	prop->Apply(eehPoint);

	// 添加到模型
	eehPoint.AddToModel();
}

/**
 * @brief 绘制预览线
 *
 * 该函数在全局变量g_FacePreviewButtonsDown为true时执行，绘制钢筋的预览线。
 * 遍历存储了钢筋起始点和结束点的m_vecRebarStartEnd向量，
 * 对于每个元素，创建并添加线段到模型中，并将这些线段存储在m_allLines向量里以便后续使用。
 */
void FacesRebarAssemblyEx::DrawPreviewLines()
{
	if (g_FacePreviewButtonsDown)//画预览线
	{
		// 原来的预览线只绘制起止点，遇到弯锚钢筋时会出现问题，如果发现没有绘制预览线，参考单面配筋的预览线点采集方式
		for (auto rebarCurves : m_vecRebarCurvePt)
		{
			for (auto rebarCurve : rebarCurves)
			{
				bvector<DPoint3d> point3ds;
				rebarCurve.GetIps(point3ds);
				EditElementHandle eehStrPoint, eehEndPoint;
				DrawPoint(point3ds[0], 1, eehStrPoint, ACTIVEMODEL);// 红色起点
				DrawPoint(point3ds[point3ds.size() - 1], 4, eehEndPoint, ACTIVEMODEL);// 蓝色终点
				m_allPreViewEehs.push_back(eehStrPoint.GetElementRef());// 存储起点
				m_allPreViewEehs.push_back(eehEndPoint.GetElementRef());// 存储终点
				EditElementHandle eeh;
				// LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				LineStringHandler::CreateLineStringElement(eeh, nullptr, point3ds.data(), point3ds.size(), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allPreViewEehs.push_back(eeh.GetElementRef());//存储所有画线
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

	ElementAgenda agenda;//存放打散之后的元素
	DropGeometryPtr pDropGeometry = DropGeometry::Create();//创建一个DropGeometry实例来设置几何选项
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
		mdlOutput_printf(MSG_STATUS, L"同组出图时打散墙的底面失败！");
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