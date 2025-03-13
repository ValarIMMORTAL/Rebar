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

bool FacesRebarAssemblyEx::IsPointHigherXYZ(const DPoint3d& p1, const DPoint3d& p2) {
	if (p1.x != p2.x)
		return p1.x > p2.x; // 先比 x
	if (p1.y != p2.y)
		return p1.y > p2.y; // 再比 y
	return p1.z > p2.z;     // 最后比 z
}

DPoint3d FacesRebarAssemblyEx::RangeMidPoint(const DRange3d &range)
{
	DPoint3d point;

	point.x = (range.low.x + range.high.x) / 2.0;
	point.y = (range.low.y + range.high.y) / 2.0;
	point.z = (range.low.z + range.high.z) / 2.0;

	return point;
}

DPoint3d FacesRebarAssemblyEx::GetEhCenterPt(ElementHandle eh)
{
	DRange3d range;
	mdlElmdscr_computeRange(&range.low, &range.high, eh.GetElementDescrCP(), nullptr);
	
	return RangeMidPoint(range);
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

// 统一处理并绘制所有存储的钢筋信息
void FacesRebarAssemblyEx::DrawAllRebars(DgnModelRefP modelRef, RebarSetTagArray& rsetTags)
{
	// 整体处理：过滤被孔洞完全覆盖的钢筋
	for (auto& info : m_rebarSetInfos)
	{
		std::vector<PITRebarCurve> filteredCurves;
		for (size_t i = 0; i < info.rebarCurves.size(); ++i)
		{
			const auto& rebarCurve = info.rebarCurves[i];
			CPoint3D ptstr, ptend;
			rebarCurve.GetEndPoints(ptstr, ptend);
			DPoint3d midPos = ptstr;
			midPos.Add(ptend);
			midPos.Scale(0.5);

			if (ISPointInHoles(m_Holeehs, midPos) &&
				ISPointInHoles(m_Holeehs, ptstr) &&
				ISPointInHoles(m_Holeehs, ptend))
			{
				continue; // 跳过被孔洞完全覆盖的钢筋
			}

			filteredCurves.push_back(rebarCurve);
		}
		info.rebarCurves = std::move(filteredCurves);
		info.numRebar = info.rebarCurves.size();
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	// 遍历所有存储的钢筋信息并绘制
	for (size_t idx = 0; idx < m_rebarSetInfos.size(); ++idx)
	{
		auto& info = m_rebarSetInfos[idx];
		if (info.rebarCurves.empty()) continue;

		// 更新钢筋集
		RebarSetData setData;
		setData.SetNumber(info.numRebar);
		setData.SetNominalSpacing(info.spacing / uor_per_mm);
		setData.SetAverageSpacing(info.adjustedSpacing / uor_per_mm);
		info.rebarSet->FinishUpdate(setData, modelRef);

		// 配置钢筋符号学属性
		RebarSymbology symb;
		string str(info.sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);

		// 存储预览线信息
		m_vecRebarCurvePt.push_back(info.rebarCurves);

		// 绘制钢筋
		int j = 0;
		for (const auto& rebarCurve : info.rebarCurves)
		{
			if (g_FacePreviewButtonsDown) break; // 预览状态下不生成钢筋

			RebarElementP rebarElement = info.rebarSet->AssignRebarElement(j, info.numRebar, symb, modelRef);
			if (!rebarElement) continue;

			// 设置钢筋形状数据
			RebarShapeData shape;
			shape.SetSizeKey(static_cast<LPCTSTR>(info.sizeKey)); // sizeKey 需要从外部传递或存储
			shape.SetIsStirrup(false);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);

			// 更新钢筋元素
			rebarElement->Update(rebarCurve, info.diameter, info.endTypes, shape, modelRef, false);

			// 配置钢筋属性
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			std::string slevel = std::to_string(info.level);
			std::string sType;
			if (info.dataExchange == 0) sType = "front";
			else if (info.dataExchange == 1) sType = "midden";
			else sType = "back";

			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, info.grade, sType, ACTIVEMODEL);
			SetRebarHideData(tmprebar, info.spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

			j++;
		}

		RebarSetTag* tag = new RebarSetTag();
		tag->SetRset(info.rebarSet);
		tag->SetIsStirrup(false);
		tag->SetBarSetTag(idx + 1);
		rsetTags.Add(tag);
	}

	// 清空临时存储
	m_rebarSetInfos.clear();
}


/**
 * @brief 反转主筋数据
 *
 * 该函数用于反转主筋数据，同时保留每个主筋的级别和方向（如果主筋平行于Y轴）。
 * 首先从当前对象中弹出主筋数据，然后保存每个主筋的级别和方向。
 * 接着反转主筋数据的顺序，最后恢复保存的级别和方向信息，并将反转后的数据设置回当前对象。
 *
 * @param isParallelToY 一个布尔值，表示主筋是否平行于Y轴。
 *                      如果为true，则在反转过程中保留主筋的方向信息。
 */
void FacesRebarAssemblyEx::ReverseMainRebars(bool isParallelToY)
{
    // 从当前对象中弹出主筋数据
    vector<PIT::ConcreteRebar> mainRebarsData = PopMainRebars();

    // 在反转之前，保存需要保持的数据
    // 存储每个主筋的级别
    vector<int> rebarLevels(mainRebarsData.size());
    // 存储每个主筋的方向
    vector<int> rebarDirs(mainRebarsData.size());
    for (size_t i = 0; i < mainRebarsData.size(); ++i) {
        // 保存每个主筋的级别
        rebarLevels[i] = mainRebarsData[i].rebarLevel;
        if (isParallelToY)
            // 如果主筋平行于Y轴，保存每个主筋的方向
            rebarDirs[i] = mainRebarsData[i].rebarDir;
    }

    // 反转整个vector
    reverse(mainRebarsData.begin(), mainRebarsData.end());

    // 恢复被保存的字段
    for (size_t i = 0; i < mainRebarsData.size(); ++i) {
        // 恢复每个主筋的级别
        mainRebarsData[i].rebarLevel = rebarLevels[i];
        if (isParallelToY)
            // 如果主筋平行于Y轴，恢复每个主筋的方向
            mainRebarsData[i].rebarDir = rebarDirs[i];
    }

    // 将反转后的数据设置回当前对象
    this->SetMainRebars(mainRebarsData);
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