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
	PointStringHandler::CreatePointStringElement(eehPoint, nullptr, pointArray.data(), nullptr, pointArray.size(), false, true, *modelRef);

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

//多平面联合配筋--begin
MultiPlaneRebarAssemblyEx::MultiPlaneRebarAssemblyEx(ElementId id, DgnModelRefP modelRef) :FacesRebarAssemblyEx(id, modelRef)
{
	m_vecRebarStartEnd.clear();
	m_vecLineSeg.clear();
	m_vecFace.clear();
	m_vecFaceNormal.clear();
}

MultiPlaneRebarAssemblyEx::~MultiPlaneRebarAssemblyEx()
{
}

bool MultiPlaneRebarAssemblyEx::AnalyzingFaceGeometricData(EditElementHandleR eeh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eeh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	if (m_Solid != NULL && m_vecElm.empty())
	{
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(*m_Solid, Eleeh, Holeehs);
		m_Holeehs.insert(m_Holeehs.end(), Holeehs.begin(), Holeehs.end());
	}

	for (size_t i = 0; i < m_vecElm.size(); ++i)
	{
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(m_vecElm[i], Eleeh, Holeehs);
		m_Holeehs.insert(m_Holeehs.end(), Holeehs.begin(), Holeehs.end());
	}
	//先转到xoy平面
// 	
// 	RotMatrix rMatrix;
// 	Transform trans;
// 	DPoint3d vecz = DPoint3d::From(0, 0, 1);
// 	mdlRMatrix_getIdentity(&rMatrix);
// 	mdlRMatrix_fromVectorToVector(&rMatrix, &GetfaceNormal(), &vecz);//旋转到xoy平面
// 	mdlTMatrix_fromRMatrix(&trans, &rMatrix);
// 	mdlTMatrix_setOrigin(&trans, &DPoint3d::From(0,0,0));
// 	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, TransformInfo(trans));
	EditElementHandle tmpeeh;
	if (m_UseXOYDir)//如果是板平面，以面的范围面作为基准面
	{
		DPoint3d minP, maxP;
		mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
		DPoint3d PTS[4];
		PTS[0] = PTS[1] = minP;
		PTS[1].x = maxP.x;
		PTS[2] = PTS[1]; PTS[2].y = maxP.y;
		PTS[3] = PTS[2]; PTS[3].x = minP.x;
		ShapeHandler::CreateShapeElement(tmpeeh, NULL, PTS, 4, true, *ACTIVEMODEL);
	}
	else
	{
		tmpeeh.Duplicate(eeh);
	}

	ElementAgenda agenda;//存放打散之后的元素
	DropGeometryPtr pDropGeometry = DropGeometry::Create();//创建一个DropGeometry实例来设置几何选项
	if (tmpeeh.GetElementType() == CMPLX_SHAPE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Complex);
	}
	else if (tmpeeh.GetElementType() == SURFACE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Solids);
	}
	else
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_LinearSegments);
	}
	if (SUCCESS != tmpeeh.GetDisplayHandler()->Drop(tmpeeh, agenda, *pDropGeometry))
	{
		agenda.Clear();
		mdlOutput_printf(MSG_STATUS, L"打散面失败！");
		return false;
	}

	class lineSort {
	public:
		bool operator() (const LineSegment &line1, const LineSegment &line2) const {
			return line1.GetLength() > line2.GetLength();
		}
	};
	std::set < LineSegment, lineSort > lineSegs;
	vector<ArcSegment> vecArcSeg;
	for (EditElementHandleR LineEeh : agenda)
	{
		if (LineEeh.GetElementType() == LINE_ELM)
		{
			DPoint3d pt[2];
			mdlLinear_extract(pt, NULL, LineEeh.GetElementP(), tmpeeh.GetModelRef());
			LineSegment lineSeg(pt[0], pt[1]);
			lineSegs.insert(lineSeg);
		}
	}

	// 取长度最长的边
	// mdlElmdscr_extractNormal(&PopfaceNormal(), NULL, tmpeeh.GetElementDescrCP(), NULL);
	//	PopfaceNormal().Negate();
	LineSegment maxLine = *lineSegs.begin();
	CVector3D  xVec(maxLine.GetLineVec());
	CVector3D  yVec = CVector3D(PopfaceNormal()).CrossProduct(xVec);
	BeMatrix   matrix = CMatrix3D::Ucs(maxLine.GetLineStartPoint(), xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	Transform trans;
	matrix.AssignTo(trans);
	trans.InverseOf(trans);
	tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, TransformInfo(trans));
	//	eeh.AddToModel();
	// 
	if (m_vecElm.size())
	{
		if (m_slabUpFace)
		{
			SetfaceNormal(DVec3d::From(0, 0, -1));
		}
		else
		{
			SetfaceNormal(DVec3d::From(0, 0, 1));
		}
	}
	//计算平面的最大范围
	DPoint3d ptMin, ptMax;
	mdlElmdscr_computeRange(&ptMin, &ptMax, tmpeeh.GetElementDescrCP(), NULL);

	DPoint3d ptLineSeg1End = ptMax;
	DPoint3d ptLineSeg2End = ptMin;
	ptLineSeg1End.y = ptMin.y;
	ptLineSeg2End.y = ptMax.y;

	DSegment3d lineSeg1 = DSegment3d::From(ptMin, ptLineSeg1End);
	DSegment3d lineSeg2 = DSegment3d::From(ptMin, ptLineSeg2End);

	EditElementHandle ehLine1, ehLine2;
	LineHandler::CreateLineElement(ehLine1, NULL, lineSeg1, true, *ACTIVEMODEL);
	LineHandler::CreateLineElement(ehLine2, NULL, lineSeg2, true, *ACTIVEMODEL);
	// 	ehLine1.AddToModel();
	// 	ehLine2.AddToModel();

	trans.InverseOf(trans);
	ehLine1.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(ehLine1, TransformInfo(trans));
	ehLine2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(ehLine2, TransformInfo(trans));

	DPoint3d ptLine1[2];
	DPoint3d ptLine2[2];
	mdlLinear_extract(ptLine1, NULL, ehLine1.GetElementCP(), ACTIVEMODEL);
	mdlLinear_extract(ptLine2, NULL, ehLine2.GetElementCP(), ACTIVEMODEL);

	stLineSeg.LineSeg1.SetLineSeg(DSegment3d::From(ptLine1[0], ptLine1[1]));
	stLineSeg.LineSeg2.SetLineSeg(DSegment3d::From(ptLine2[0], ptLine2[1]));

	if (stLineSeg.LineSeg1.GetLineVec().IsParallelTo(DVec3d::From(0, 0, 1)))
	{
		LineSegment segTmp = stLineSeg.LineSeg2;
		stLineSeg.LineSeg2 = stLineSeg.LineSeg1;
		stLineSeg.LineSeg1 = segTmp;
	}

	if (m_vecLineSeg.size() > 0)
	{
		PIT::LineSegment  lineSeg;
		lineSeg = m_vecLineSeg.at(m_vecLineSeg.size() - 1).LineSeg1;

		DPoint3d pt1[2];
		pt1[0] = lineSeg.GetLineStartPoint();
		pt1[1] = lineSeg.GetLineEndPoint();

		DPoint3d pt2[2];
		pt2[0] = stLineSeg.LineSeg1.GetLineStartPoint();
		pt2[1] = stLineSeg.LineSeg1.GetLineEndPoint();

		CVector3D vec = pt1[1] - pt1[0];
		vec.Normalize();

		if (abs(vec.x - 0.00) < EPS)
		{
			pt2[0].x = pt1[0].x;
			pt2[1].x = pt1[1].x;
		}

		if (abs(vec.y - 0.00) < EPS)
		{
			pt2[0].y = pt1[0].y;
			pt2[1].y = pt1[1].y;
		}

		if (abs(vec.z - 0.00) < EPS)
		{
			pt2[0].z = pt1[0].z;
			pt2[1].z = pt1[1].z;
		}

		stLineSeg.LineSeg1.SetLineStartPoint(pt2[0]);
		stLineSeg.LineSeg1.SetLineEndPoint(pt2[1]);
	}

	stLineSeg.iIndex = (int)m_vecLineSeg.size();

	if (GetfaceType() == FaceType::other && m_vecLineSeg.size() > 0)
	{
		DSegment3d segLin1 = stLineSeg.LineSeg1.GetLineSeg();
		DSegment3d segLin2 = m_vecLineSeg.at(0).LineSeg1.GetLineSeg();

		DVec3d vec1 = stLineSeg.LineSeg1.GetLineVec();
		DVec3d vec2 = m_vecLineSeg.at(0).LineSeg1.GetLineVec();

		vec1.x = fabs(vec1.x - 0.0) < uor_ref ? 0 : vec1.x;
		vec1.y = fabs(vec1.y - 0.0) < uor_ref ? 0 : vec1.y;
		vec1.z = fabs(vec1.z - 0.0) < uor_ref ? 0 : vec1.z;

		vec2.x = fabs(vec2.x - 0.0) < uor_ref ? 0 : vec2.x;
		vec2.y = fabs(vec2.y - 0.0) < uor_ref ? 0 : vec2.y;
		vec2.z = fabs(vec2.z - 0.0) < uor_ref ? 0 : vec2.z;

		vec1.Normalize();
		vec2.Normalize();

		if (vec1.IsPerpendicularTo(vec2))
		{
			stLineSeg.LineSeg1.SetLineSeg(stLineSeg.LineSeg2.GetLineSeg());
			stLineSeg.LineSeg2.SetLineSeg(segLin1);
		}
	}

	m_vecLineSeg.push_back(stLineSeg);
	//m_vecFace.push_back(eeh);
	m_vecFaceNormal.push_back(PopfaceNormal());
	PopvecFrontPts().push_back(stLineSeg.LineSeg1.GetLineStartPoint());
	PopvecFrontPts().push_back(stLineSeg.LineSeg1.GetLineEndPoint());
	return true;
}

void MultiPlaneRebarAssemblyEx::CalculateUseHoles(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetConcrete().sideCover*uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

	if (GetConcrete().isHandleHole)//计算需要处理的孔洞
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

void MultiPlaneRebarAssemblyEx::CmdSort(vector<DPoint3d>& vecPoint)
{
	m_ComVec.Normalize();

	for (int i = 0; i < vecPoint.size() - 1; i++)
	{
		for (int j = 0; j < vecPoint.size() - i - 1; j++)
		{
			DPoint3d ptNex = vecPoint[j];
			DPoint3d ptPre = vecPoint[j + 1];
			DPoint3d vec = ptNex - ptPre;
			vec.Normalize();
			if (COMPARE_VALUES_EPS(fabs(vec.x + m_ComVec.x), fabs(vec.x) + fabs(m_ComVec.x), EPS) != 0 ||
				COMPARE_VALUES_EPS(fabs(vec.y + m_ComVec.y), fabs(vec.y) + fabs(m_ComVec.y), EPS) != 0 ||
				COMPARE_VALUES_EPS(fabs(vec.z + m_ComVec.z), fabs(vec.z) + fabs(m_ComVec.z), EPS) != 0)
			{
				DPoint3d pt = vecPoint[j + 1];
				vecPoint[j + 1] = vecPoint[j];
				vecPoint[j] = pt;
			}
		}
	}
	return;
}

// 每条线段首尾连接
void MultiPlaneRebarAssemblyEx::SortLineSegVec(vector<LineSeg>& m_vecLineSeg, double uor_per_mm)
{
	if (m_vecLineSeg.size() < 2)
	{
		return;
	}

	// 第一段的两个点
	DPoint3d ptFir[2] = { DPoint3d::FromZero(), DPoint3d::FromZero() };
	ptFir[0] = m_vecLineSeg.at(0).LineSeg1.GetLineStartPoint();
	ptFir[1] = m_vecLineSeg.at(0).LineSeg1.GetLineEndPoint();

	for (int nIndex = 0; nIndex < 2; nIndex++)
	{
		bool bFlag = true;
		DPoint3d ptPreEnd = ptFir[nIndex];   // 前一段的终点
		for (int i = 1; i < m_vecLineSeg.size(); i++)
		{
			// 上一段终点要等于下一段的起点
			DPoint3d ptCurStr = m_vecLineSeg.at(i).LineSeg1.GetLineStartPoint();
			if (COMPARE_VALUES_EPS(ptCurStr.x, ptPreEnd.x, uor_per_mm) != 0 ||
				COMPARE_VALUES_EPS(ptCurStr.y, ptPreEnd.y, uor_per_mm) != 0 ||
				COMPARE_VALUES_EPS(ptCurStr.z, ptPreEnd.z, uor_per_mm) != 0)
			{
				DPoint3d ptCurEnd = m_vecLineSeg.at(i).LineSeg1.GetLineEndPoint();
				if (COMPARE_VALUES_EPS(ptCurEnd.x, ptPreEnd.x, uor_per_mm) == 0 &&
					COMPARE_VALUES_EPS(ptCurEnd.y, ptPreEnd.y, uor_per_mm) == 0 &&
					COMPARE_VALUES_EPS(ptCurEnd.z, ptPreEnd.z, uor_per_mm) == 0)
				{
					DPoint3d pt[2];
					pt[0] = m_vecLineSeg.at(i).LineSeg1.GetLineStartPoint();
					pt[1] = m_vecLineSeg.at(i).LineSeg1.GetLineEndPoint();
					m_vecLineSeg.at(i).LineSeg1.SetLineStartPoint(pt[1]);
					m_vecLineSeg.at(i).LineSeg1.SetLineEndPoint(pt[0]);
				}
				else
				{
					bFlag = false;
				}

			}

			ptPreEnd = m_vecLineSeg.at(i).LineSeg1.GetLineEndPoint();
		}
		if (nIndex == 0 && bFlag)
		{
			m_vecLineSeg.at(0).LineSeg1.SetLineStartPoint(ptFir[1]);
			m_vecLineSeg.at(0).LineSeg1.SetLineEndPoint(ptFir[0]);
		}
	}
}


// 将各个面的配筋方向排序
void MultiPlaneRebarAssemblyEx::SortLineSeg(vector<LineSeg>&	m_vecLineSeg, double uor_per_mm)
{
	if (m_vecLineSeg.size() < 2)
	{
		return;
	}

	//计算指定元素描述符中元素的范围。
	DPoint3d minP, minP_nex;
	EditElementHandle eehFace(m_vecFace.at(0), false);
	mdlElmdscr_computeRange(&minP, NULL, eehFace.GetElementDescrP(), NULL);

	EditElementHandle eehFaceNex(m_vecFace.at(1), false);
	mdlElmdscr_computeRange(&minP_nex, NULL, eehFaceNex.GetElementDescrP(), NULL);

	m_ComVec = minP_nex - minP;

	DVec3d vecPre = m_vecLineSeg.at(0).LineSeg1.GetLineVec();
	DVec3d vecNex = m_vecLineSeg.at(0).LineSeg2.GetLineVec();

	vector<DPoint3d> vecPoint;
	for (int i = 0; i < m_vecLineSeg.size(); i++)
	{
		if (i > 0)
		{
			EditElementHandle eehFace1(m_vecFace.at(i - 1), false);
			mdlElmdscr_computeRange(&minP, NULL, eehFace1.GetElementDescrP(), NULL);

			EditElementHandle eehFaceNex1(m_vecFace.at(i), false);
			mdlElmdscr_computeRange(&minP_nex, NULL, eehFaceNex1.GetElementDescrP(), NULL);

			m_ComVec = minP_nex - minP;
		}

		DPoint3d ptEnd, ptStr;
		ptStr = m_vecLineSeg.at(i).LineSeg1.GetLineStartPoint();
		ptEnd = m_vecLineSeg.at(i).LineSeg1.GetLineEndPoint();

		DPoint3d vec = ptEnd - ptStr;

		if (COMPARE_VALUES_EPS(fabs(vec.x + m_ComVec.x), fabs(vec.x) + fabs(m_ComVec.x), uor_per_mm) != 0 ||
			COMPARE_VALUES_EPS(fabs(vec.y + m_ComVec.y), fabs(vec.y) + fabs(m_ComVec.y), uor_per_mm) != 0 ||
			COMPARE_VALUES_EPS(fabs(vec.z + m_ComVec.z), fabs(vec.z) + fabs(m_ComVec.z), uor_per_mm) != 0)
		{
			m_vecLineSeg.at(i).LineSeg1.SetLineStartPoint(ptEnd);
			m_vecLineSeg.at(i).LineSeg1.SetLineEndPoint(ptStr);
		}
	}

	// 确保线段首尾相连
	SortLineSegVec(m_vecLineSeg, uor_per_mm);

	DPoint3d ptEnd, ptStr;

	double dLength = m_vecLineSeg.at(0).LineSeg2.GetLength();
	ptStr = m_vecLineSeg.at(0).LineSeg1.GetLineStartPoint();

	CVector3D vecTemp = m_vecLineSeg.at(0).LineSeg2.GetLineVec();
	vecTemp.Normalize();
	vecTemp.ScaleToLength(dLength);
	ptEnd = ptStr;
	ptEnd.Add(vecTemp);

	m_vecLineSeg.at(0).LineSeg2.SetLineStartPoint(ptStr);
	m_vecLineSeg.at(0).LineSeg2.SetLineEndPoint(ptEnd);

	int iIndex = 0;
	for (LineSeg linSeg : m_vecLineSeg)
	{
		if (iIndex & 0x1)
		{
			iIndex++;
			continue;
		}
		EditElementHandle eehTmp;
		if (SUCCESS == LineHandler::CreateLineElement(eehTmp, NULL, linSeg.LineSeg1.GetLineSeg(), true, *ACTIVEMODEL))
		{
			eehTmp.AddToModel();
		}

		//EditElementHandle eehTTmp;
		//if (SUCCESS == LineHandler::CreateLineElement(eehTTmp, NULL, linSeg.LineSeg2.GetLineSeg(), true, *ACTIVEMODEL))
		//{
		//	eehTTmp.AddToModel();
		//}
		iIndex++;
	}

	return;
}

bool MultiPlaneRebarAssemblyEx::JudgeMergeType(PIT::LineSegment& LineSeg1, PIT::LineSegment& LineSeg2, double uor_per_mm)
{
	DPoint3d ptStart = LineSeg1.GetLineEndPoint();
	DPoint3d ptEnd = LineSeg2.GetLineStartPoint();

	if (fabs(ptStart.x - ptEnd.x) < uor_per_mm && fabs(ptStart.y - ptEnd.y) < uor_per_mm && fabs(ptStart.z - ptEnd.z) < uor_per_mm)
	{
		return true;
	}
	return false;
}

bool MultiPlaneRebarAssemblyEx::makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, vector<DPoint3d>& rebarPoint, int iIndex, const PIT::PITRebarEndTypes& endTypes, double bendRadius)
{
	int nCnt = (int)rebarPoint.size();
	DPoint3d startPt = rebarPoint.at(0);
	DPoint3d endPt = rebarPoint.at(1);
	if (GetIntersectPointsWithNegs(m_Negs, startPt, endPt))
	{
		return false;
	}

	m_vecRebarPtsLayer.push_back(startPt);
	m_vecRebarPtsLayer.push_back(endPt);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetConcrete().sideCover * uor_per_mm;
	vector<DPoint3d> tmppts;
	vector<DSegment3d> vecPtRebars;

	vector<DPoint3d> tmpptsTmp;
	if (m_vecFace.size() > 0) // 与原实体相交(无孔洞)
	{
		//将原平面往法向方向拉伸为一个实体
		EditElementHandle eehSolid;
		// 		DVec3d vec = GetfaceNormal();
		// 		vec.ScaleToLength(100000);
		// 		SolidHandler::CreateProjectionElement(eehSolid, NULL, m_face, m_LineSeg1.GetLineStartPoint(), vec, NULL, true, *ACTIVEMODEL);
		for (int i = 0; i < rebarPoint.size() - 1; i++)
		{
			startPt = rebarPoint.at(i);
			endPt = rebarPoint.at(i + 1);
			if (m_vecFace.at(iIndex).IsValid())
			{
				ISolidKernelEntityPtr ptarget;
				vector<vector<DPoint3d>> vecTmp;

				SolidUtil::Convert::ElementToBody(ptarget, m_vecFace.at(iIndex), true, true, true);
				if (SUCCESS == SolidUtil::Modify::ThickenSheet(ptarget, 5000 * uor_per_mm, 5000 * uor_per_mm))
				{
					if (SUCCESS == SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL))
					{
						DPoint3d startPtBack = startPt;
						DPoint3d endPtBack = endPt;
						GetIntersectPointsWithOldElm(tmpptsTmp, &eehSolid, startPt, endPt, 0.00);
						if (tmpptsTmp.size() > 1)
						{
							// 存在交点为两个以上的情况
							GetIntersectPointsWithSlabRebar(vecTmp, tmpptsTmp, startPt, endPt, &eehSolid, 0.00);

							int iTemp = 0;
							for (vector<DPoint3d> vec : vecTmp)
							{
								vecPtRebars.push_back({ vec.at(0), vec.at(1) });
								iTemp++;
							}
						}
						else // 没有交点
						{
							vecPtRebars.push_back({ startPtBack, endPtBack });
						}
					}
				}
			}
			else // 中间的面不交
			{
				vecPtRebars.push_back({ startPt, endPt });
			}
		}
	}

	bvector<DPoint3d> allpts;
	if (vecPtRebars.size() == 1)
	{
		DPoint3d pt[2];
		vecPtRebars.at(0).GetStartPoint(pt[0]);
		vecPtRebars.at(0).GetEndPoint(pt[1]);

		allpts.push_back(pt[0]);
		allpts.push_back(pt[1]);

		RebarVertices  vers;
		GetRebarVerticesFromPoints(vers, allpts, bendRadius);

		PIT::PITRebarCurve rebar;
		rebar.SetVertices(vers);

		rebar.EvaluateEndTypes(endTypes);
		rebars.push_back(rebar);
		allpts.clear();
	}
	m_vecRebarStartEnd.push_back(vecPtRebars);
	return true;
}

bool MultiPlaneRebarAssemblyEx::makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, vector<DPoint3d>& rebarPoint, vector<int>& vecIndex, const PIT::PITRebarEndTypes& endTypes, double bendRadius)
{
	int nCnt = (int)rebarPoint.size();
	DPoint3d startPt = rebarPoint.at(0);
	DPoint3d endPt = rebarPoint.at(1);
	if (GetIntersectPointsWithNegs(m_Negs, startPt, endPt))
	{
		return false;
	}

	m_vecRebarPtsLayer.push_back(startPt);
	m_vecRebarPtsLayer.push_back(endPt);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetConcrete().sideCover * uor_per_mm;
	vector<DPoint3d> tmppts;
	vector<DSegment3d> vecPtRebars;

	vector<DPoint3d> tmpptsTmp;
	if (m_vecFace.size() > 0) // 与原实体相交(无孔洞)
	{
		//将原平面往法向方向拉伸为一个实体
		EditElementHandle eehSolid;
		// 		DVec3d vec = GetfaceNormal();
		// 		vec.ScaleToLength(100000);
		// 		SolidHandler::CreateProjectionElement(eehSolid, NULL, m_face, m_LineSeg1.GetLineStartPoint(), vec, NULL, true, *ACTIVEMODEL);
		for (int i = 0; i < rebarPoint.size() - 1; i++)
		{
			int iIndex = 0;
			for (int j = 0; j < (int)vecIndex.size(); j++)
			{
				if (i + 1 < vecIndex.at(j))
				{
					iIndex = j;
					break;
				}
			}

			startPt = rebarPoint.at(i);
			endPt = rebarPoint.at(i + 1);
			if (m_vecFace.at(iIndex).IsValid() && (i == 0 || i == rebarPoint.size() - 2)) // 只交首位两端的面
			{
				ISolidKernelEntityPtr ptarget;
				vector<vector<DPoint3d>> vecTmp;

				SolidUtil::Convert::ElementToBody(ptarget, m_vecFace.at(iIndex), true, true, true);
				if (SUCCESS == SolidUtil::Modify::ThickenSheet(ptarget, 5000 * uor_per_mm, 5000 * uor_per_mm))
				{
					if (SUCCESS == SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL))
					{
						DPoint3d startPtBack = startPt;
						DPoint3d endPtBack = endPt;
						GetIntersectPointsWithOldElm(tmpptsTmp, &eehSolid, startPt, endPt, 0.00);
						if (tmpptsTmp.size() > 1)
						{
							// 存在交点为两个以上的情况
							GetIntersectPointsWithSlabRebar(vecTmp, tmpptsTmp, startPt, endPt, &eehSolid, 0.00);

							int iTemp = 0;
							for (vector<DPoint3d> vec : vecTmp)
							{
								if (i == 0 && iTemp == vecTmp.size() - 1)
								{
									vecPtRebars.push_back({ vec.at(0), endPtBack });
								}
								else if (i == rebarPoint.size() - 2 && iTemp == 0)
								{
									vecPtRebars.push_back({ startPtBack, vec.at(1) });
								}
								else
								{
									vecPtRebars.push_back({ vec.at(0), vec.at(1) });
								}
								iTemp++;
							}
						}
						else // 没有交点
						{
							vecPtRebars.push_back({ startPtBack, endPtBack });
						}
					}
				}
			}
			else // 中间的面不交
			{
				vecPtRebars.push_back({ startPt, endPt });
			}
		}
	}

	vector<DSegment3d> vecPtRebarsNew;
	for (int i = 0; i < (int)vecPtRebars.size(); i++)
	{
		DPoint3d pt[2];
		vecPtRebars[i].GetStartPoint(pt[0]);
		vecPtRebars[i].GetEndPoint(pt[1]);

		GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt[0], pt[1], dSideCover);

		if (tmppts.size() == 2)
		{
			vecPtRebarsNew.push_back({ pt[0], tmppts[0] });
			vecPtRebarsNew.push_back({ tmppts[1], pt[1] });
		}
		else
		{
			vecPtRebarsNew.push_back({ pt[0] , pt[1] });
		}
	}
	m_vecRebarStartEnd.push_back(vecPtRebarsNew);
	bvector<DPoint3d> allpts;
	for (int i = 0; i < (int)vecPtRebarsNew.size() - 1; i++)
	{

		DSegment3d seg_cur, seg_nex;
		DPoint3d ptStart_cur, ptEnd_cur;
		DPoint3d ptStart_nex, ptEnd_nex;

		seg_cur = vecPtRebarsNew.at(i);
		seg_nex = vecPtRebarsNew.at(i + 1);

		seg_cur.GetStartPoint(ptStart_cur);
		seg_cur.GetEndPoint(ptEnd_cur);

		seg_nex.GetStartPoint(ptStart_nex);
		seg_nex.GetEndPoint(ptEnd_nex);

		if (i == 0)
		{
			allpts.push_back(ptStart_cur);
		}
		CVector3D vec1 = ptEnd_cur - ptStart_cur;
		CVector3D vec2 = ptEnd_nex - ptStart_nex;

		vec1.Normalize();
		vec2.Normalize();

		if (!vec2.IsEqual(vec1, 0.001))
		{
			allpts.push_back(ptEnd_cur);
		}

		if (!ptEnd_cur.IsEqual(ptStart_nex, EPS))
		{
			RebarVertices  vers;
			GetRebarVerticesFromPoints(vers, allpts, bendRadius);

			PIT::PITRebarCurve rebar;
			rebar.SetVertices(vers);

			rebar.EvaluateEndTypes(endTypes);
			rebars.push_back(rebar);
			allpts.clear();

			if (i == (int)vecPtRebarsNew.size() - 2)
			{
				allpts.push_back(ptStart_nex);
				allpts.push_back(ptEnd_nex);

				RebarVertices  vers;
				GetRebarVerticesFromPoints(vers, allpts, bendRadius);
				PIT::PITRebarCurve rebar;
				rebar.SetVertices(vers);

				rebar.EvaluateEndTypes(endTypes);
				rebars.push_back(rebar);
				allpts.clear();
			}
			continue;
		}

		if (i == (int)vecPtRebarsNew.size() - 2)
		{
			allpts.push_back(ptEnd_nex);

			RebarVertices  vers;
			GetRebarVerticesFromPoints(vers, allpts, bendRadius);
			PIT::PITRebarCurve rebar;
			rebar.SetVertices(vers);

			rebar.EvaluateEndTypes(endTypes);
			rebars.push_back(rebar);
			allpts.clear();
		}
	}

	/*for (size_t i = 0; i < vecPtRebars.size(); i++)
	{
		vecPtRebars.at(i).GetStartPoint(startPt);
		vecPtRebars.at(i).GetEndPoint(endPt);
		GetIntersectPointsWithHoles(tmppts, m_useHoleehs, startPt, endPt, dSideCover);
		map<int, DPoint3d> map_pts;
		bool isStr = false;
		for (DPoint3d pt : tmppts)
		{
			if (ExtractFacesTool::IsPointInLine(pt, startPt, endPt, ACTIVEMODEL, isStr))
			{
				int dis = (int)startPt.Distance(pt);
				if (map_pts.find(dis) != map_pts.end())
				{
					dis = dis + 1;
				}
				map_pts[dis] = pt;
			}
		}
		if (map_pts.find(0) != map_pts.end())
		{
			map_pts[1] = startPt;
		}
		else
		{
			map_pts[0] = startPt;
		}
		int dis = (int)startPt.Distance(endPt);
		if (map_pts.find(dis) == map_pts.end())
		{
			map_pts[dis] = endPt;
		}
		else
		{
			dis = dis + 1;
			map_pts[dis] = endPt;
		}



		for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
		{
			PITRebarEndTypes endTypesTmp = endTypes;
			PITRebarCurve rebar;
			RebarVertexP vex;
			DPoint3d ptStart(itr->second);
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(itr->second);
			vex->SetType(RebarVertex::kStart);
			endTypesTmp.beg.SetptOrgin(itr->second);

			map<int, DPoint3d>::iterator itrplus = ++itr;
			if (itrplus == map_pts.end())
			{
				break;
			}

			endTypesTmp.end.SetptOrgin(itrplus->second);

			DPoint3d ptEnd(itrplus->second);
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(itrplus->second);
			vex->SetType(RebarVertex::kEnd);

			double dis1 = ptStart.Distance(ptStartBack);
			if (COMPARE_VALUES_EPS(dis1, 0, 1) != 0)
			{
				endTypesTmp.beg.SetType(PITRebarEndType::kNone);
			}
			double dis2 = ptEnd.Distance(ptEndBack);

			if (COMPARE_VALUES_EPS(dis2, 0, 1) != 0)
			{
				endTypesTmp.end.SetType(PITRebarEndType::kNone);
			}
			// 			EditElementHandle eeh;
			// 			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptStart, ptEnd), true, *ACTIVEMODEL);
			// 			eeh.AddToModel();
			rebar.EvaluateEndTypes(endTypesTmp);
			//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
			rebars.push_back(rebar);*/
	return true;
}

RebarSetTag* MultiPlaneRebarAssemblyEx::MakeRebars
(
	ElementId&					rebarSetId,
	vector<PIT::LineSegment>&	vecRebarLine,
	vector<PIT::LineSegment>&	vec,
	int							dir,
	BrStringCR					sizeKey,
	double						spacing,
	double						startOffset,
	double						endOffset,
	int							level,
	int							grade,
	int							DataExchange,
	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const&	vecEndNormal,
	DgnModelRefP				modelRef
)
{
	bool const isStirrup = false;

	if (vecRebarLine.size() < 1)
	{
		return NULL;
	}

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}

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
	double startbendLen, endbendLen;
	double begStraightAnchorLen = 0, endStraightAnchorLen = 0;
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
	}
	break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	bool isFlag = false;
	if (vec.size() == 1)
	{
		isFlag = true;
	}

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double adjustedXLen, adjustedSpacing;
	double sideCov = GetConcrete().sideCover * uor_per_mm;

	int nCnt = (int)vecRebarLine.size();

	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);

	for (int i = 0; i < (int)vecRebarLine.size() - 1; i++)
	{
		if (!JudgeMergeType(vecRebarLine.at(i), vecRebarLine.at(i + 1), uor_per_mm))
		{
			DPoint3d ptIntersec;
			if (SUCCESS == mdlVec_intersect(&ptIntersec, &vecRebarLine.at(i).GetLineSeg(), &vecRebarLine.at(i + 1).GetLineSeg()))
			{
				vecRebarLine.at(i).SetLineEndPoint(ptIntersec);
				vecRebarLine.at(i + 1).SetLineStartPoint(ptIntersec);
			}
		}
	}

	for (int i = 0; i < (int)vec.size() - 1; i++)
	{
		if (!JudgeMergeType(vec.at(i), vec.at(i + 1), uor_per_mm))
		{
			DPoint3d ptIntersec;
			if (SUCCESS == mdlVec_intersect(&ptIntersec, &vec.at(i).GetLineSeg(), &vec.at(i + 1).GetLineSeg()))
			{
				vec.at(i).SetLineEndPoint(ptIntersec);
				vec.at(i + 1).SetLineStartPoint(ptIntersec);
			}
		}
	}
	for (int i = 0; i < (int)vecRebarLine.size(); i++)
	{
		vecRebarLine.at(i).PerpendicularOffset(startOffset, vec.at(0).GetLineVec());
	}
	double updownSideCover = 50 * uor_per_mm;
	if (isFlag)
	{
		adjustedXLen = vec.at(0).GetLength() - updownSideCover * 2 - startOffset - endOffset;
	}
	else
	{
		//vec.at(0).Shorten(sideCov * 2 + diameter + spacing, true);
		//vec.at(vec.size() - 1).Shorten(sideCov * 2 + diameter + spacing, false);

		double dLength = 0.00;
		for (auto seg : vec)
		{
			dLength += seg.GetLength();
		}
		adjustedXLen = dLength - updownSideCover * 2 - startOffset - endOffset;
	}

	int numRebar = 0;
	numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
	{
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	}
	adjustedSpacing = spacing;
	//	rebarLine.PerpendicularOffset(startOffset, vec.GetLineVec());
	//	rebarLine.PerpendicularOffset(sideCov, vec.GetLineVec());
	vector<PITRebarCurve>     rebarCurvesNum;

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

	if (isFlag)
	{
		vecRebarLine.at(0).Shorten(sideCov + endTypeStartOffset, true);
		vecRebarLine.at(nCnt - 1).Shorten(sideCov + endTypeStartOffset, false);
	}
	else
	{
		vecRebarLine.at(0).Shorten(sideCov + endTypeStartOffset, true);
		vecRebarLine.at(0).Shorten(sideCov + endTypeStartOffset, false);
	}

	start.SetptOrgin(vecRebarLine.at(0).GetLineStartPoint());

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);
	end.SetptOrgin(vecRebarLine.at(nCnt - 1).GetLineEndPoint());

	PITRebarEndTypes   endTypes = { start, end };
	//	rebarLine.PerpendicularOffset(sideCov+diameter*0.5, vec.GetLineVec());

	double totalAdjust = 0.00;

	DVec3d offsetVec_pre = vec.at(0).GetLineVec();// 上一次调整方向
	int iIndexFace = 0;
	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurves;

		vector<DPoint3d> vecPoint;
		vector<int> vecIndex;

		for (int j = 0; j < vecRebarLine.size(); j++)
		{
			PIT::LineSegment seg = vecRebarLine.at(j);
			DPoint3d pt[2];
			pt[0] = seg.GetLineStartPoint();
			pt[1] = seg.GetLineEndPoint();

			if (j == 0)
			{
				vecPoint.push_back(pt[0]);
				vecPoint.push_back(pt[1]);
			}
			else
			{
				vecPoint.push_back(pt[1]);
			}
			vecIndex.push_back((int)vecPoint.size());
		}

		if (isFlag)
		{
			makeRebarCurve(rebarCurves, vecPoint, vecIndex, endTypes, bendRadius);
		}
		else
		{
			makeRebarCurve(rebarCurves, vecPoint, iIndexFace, endTypes, bendRadius);
		}

		//		xPos += adjustedSpacing
		if (isFlag)
		{
			DVec3d offsetVec = vec.at(0).GetLineVec();
			for (int j = 0; j < (int)vecRebarLine.size(); j++)
			{
				vecRebarLine[j].PerpendicularOffset(adjustedSpacing, offsetVec);
			}

			rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
		}
		else
		{
			DVec3d offsetVec;
			double totalTmp = 0.00;

			double adjustedSpacingTemp = adjustedSpacing;
			for (int j = 0; j < (int)vec.size(); j++)
			{
				totalTmp += vec.at(j).GetLength();
				if (COMPARE_VALUES_EPS(totalTmp - updownSideCover - diameter * 0.5 - startOffset, totalAdjust + adjustedSpacing, EPS) > 0)
				{
					offsetVec = vec.at(j).GetLineVec();
					iIndexFace = j;
					if (!offsetVec_pre.IsEqual(offsetVec, EPS))
					{
						double dTemp = totalTmp - updownSideCover - diameter * 0.5 - startOffset - vec.at(j).GetLength() - totalAdjust;
						vecRebarLine[0].PerpendicularOffset(dTemp, offsetVec_pre);
						offsetVec_pre = offsetVec;

						adjustedSpacingTemp -= dTemp;
					}
					break;
				}
			}
			vecRebarLine[0].PerpendicularOffset(adjustedSpacingTemp, offsetVec);

			totalAdjust += adjustedSpacing;

			rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
		}
	}

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);
	vector<DSegment3d> vecStartEnd;
	int j = 0;
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
		// 		EditElementHandle eeh;
		// 		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		// 		eeh.AddToModel();

		RebarElementP rebarElement = NULL;
		if (!g_FacePreviewButtonsDown)//预览状态下不生成钢筋
		{
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
				Stype = "front";
			}
			else if (DataExchange == 1)
			{
				Stype = "midden";
			}
			else
			{
				Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}

	//	
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

bool MultiPlaneRebarAssemblyEx::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	CalculateUseHoles(modelRef);
	m_vecRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
	double dLevelSpace = 0;
	double dSideCover = GetConcrete().sideCover * uor_per_mm;
	double dPositiveCover = GetConcrete().postiveCover * uor_per_mm;
	double offset = dPositiveCover;

	// 将各个面的配筋方向排序
	SortLineSeg(m_vecLineSeg, uor_per_mm);

	vector<ElementHandle>	vecFace;
	for (int i = 0; i < (int)m_vecLineSeg.size(); i++)
	{
		vecFace.push_back(m_vecFace.at(m_vecLineSeg.at(i).iIndex));
	}
	m_vecFace.clear();
	m_vecFace.insert(m_vecFace.begin(), vecFace.begin(), vecFace.end());

	vector<DVec3d>	vecFaceNormal;
	for (int i = 0; i < (int)m_vecFaceNormal.size(); i++)
	{
		vecFaceNormal.push_back(m_vecFaceNormal.at(m_vecLineSeg.at(i).iIndex));
	}
	m_vecFaceNormal = vecFaceNormal;

	vector<PIT::LineSegment> vecMergeSeg;  // 需要合并的线方向
	vector<PIT::LineSegment> vecSeg;	   // 不需要合并的线方向
	bool bFlag = true;    // true : 合并方向为横向钢筋  false ： 合并方向为竖向钢筋
	if (m_vecLineSeg.size() > 1)
	{
		if (JudgeMergeType(m_vecLineSeg[0].LineSeg1, m_vecLineSeg[1].LineSeg1, uor_per_mm))
		{
			bFlag = true;
			for (LineSeg lineSge : m_vecLineSeg)
			{
				vecMergeSeg.push_back(lineSge.LineSeg1);
				vecSeg.push_back(lineSge.LineSeg2);
			}
		}
		else
		{
			bFlag = false;
			for (LineSeg lineSge : m_vecLineSeg)
			{
				vecMergeSeg.push_back(lineSge.LineSeg2);
				vecSeg.push_back(lineSge.LineSeg1);
			}
		}
	}

	if (vecMergeSeg.size() == 0 || vecSeg.size() == 0)
	{
		return false;
	}

	int iRebarSetTag = 0;
	int iRebarLevelNum = GetConcrete().rebarLevelNum;
	vector<PIT::EndType> vecEndType;
	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		PopSetIds().push_back(0);
		RebarSetTag* tag = NULL;
		vector<PIT::EndType> vecEndType;
		if (GetvecEndTypes().empty())		//没有设置端部样式，设置默认值
		{
			EndType endType;
			memset(&endType, 0, sizeof(endType));
			vecEndType = { { 0,0,0 },{0,0,0} ,endType };
		}
		else
		{
			vecEndType = GetvecEndTypes().at(i);
		}

		BrString strRebarSize(GetMainRebars().at(i).rebarSize);
		double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
		double spacing = GetMainRebars().at(i).spacing * uor_per_mm;
		double startOffset = GetMainRebars().at(i).startOffset * uor_per_mm;
		double endOffset = GetMainRebars().at(i).endOffset * uor_per_mm;
		int	rebarDir = GetMainRebars().at(i).rebarDir;
		int	DataExchange = GetMainRebars().at(i).datachange;
		double levelspacing = GetMainRebars().at(i).levelSpace * uor_per_mm;
		m_vecRebarPtsLayer.clear();
		m_vecTwinRebarPtsLayer.clear();
		vector<CVector3D> vecEndNormal(2);
		CVector3D	endNormal;	//端部弯钩方向

		int nCnt = (int)vecMergeSeg.size();

		int comDir = 0;
		if (!bFlag)
		{
			comDir = 1;
		}

		if (rebarDir == comDir)
		{
			if (GetvecEndTypes().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = GetfaceNormal();
					CVector3D rebarVec = vecMergeSeg.at(0).GetLineVec();
					if (k == 1)
					{
						rebarVec = vecMergeSeg.at(nCnt - 1).GetLineVec();
					}
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}

			offset += levelspacing;
			if (0 == i)	//首层偏移当前钢筋直径
			{
				offset += diameter * 0.5;
			}
			else
			{
				double diameterPre = RebarCode::GetBarDiameter(GetMainRebars().at(i - 1).rebarSize, modelRef);	//上一层钢筋直径
				offset += diameterPre * 0.5;	//偏移上一层钢筋的半径
				offset += diameter * 0.5;		//偏移当前层钢筋的半径
			}


			vector<PIT::LineSegment> vecSeg1;
			vector<PIT::LineSegment> vecSeg2;

			vecSeg1.insert(vecSeg1.begin(), vecMergeSeg.begin(), vecMergeSeg.end());
			vecSeg2.push_back(vecSeg.at(0));

			for (int j = 0; j < (int)vecMergeSeg.size(); j++)
			{
				// vecMergeSeg.at(i).PerpendicularOffset(dSideCover + diameter * 0.5, vecSegTmp.at(0).GetLineVec());
				DVec3d  vec = m_vecFaceNormal.at(j);
				if (GetisReverse())
				{
					vec.Negate();
				}
				vecSeg1.at(j).PerpendicularOffset(offset, vec);
				vecSeg1.at(j).PerpendicularOffset(dSideCover + diameter * 0.5, vecSeg2.at(0).GetLineVec());
			}

			tag = MakeRebars(PopSetIds().at(i), vecSeg1, vecSeg2, rebarDir, strRebarSize, spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();
		}
		else
		{
			if (GetvecEndTypes().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = GetfaceNormal();
					CVector3D rebarVec = vecSeg.at(0).GetLineVec();
					if (k == 1)
					{
						rebarVec = vecSeg.at(nCnt - 1).GetLineVec();
					}
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}

			offset += levelspacing;
			if (0 == i)	//首层偏移当前钢筋直径
			{
				offset += diameter * 0.5;
			}
			else
			{
				double diameterPre = RebarCode::GetBarDiameter(GetMainRebars().at(i - 1).rebarSize, modelRef);	//上一层钢筋直径
				offset += diameterPre * 0.5;	//偏移上一层钢筋的半径
				offset += diameter * 0.5;		//偏移当前层钢筋的半径
			}

			vector<PIT::LineSegment> vecSeg1;
			vector<PIT::LineSegment> vecSeg2;

			vecSeg1.insert(vecSeg1.begin(), vecMergeSeg.begin(), vecMergeSeg.end());
			vecSeg2.push_back(vecSeg.at(0));

			DVec3d  vec = m_vecFaceNormal.at(0);
			if (GetisReverse())
			{
				vec.Negate();
			}

			vecSeg2.at(0).PerpendicularOffset(offset, vec);
			vecSeg2.at(0).PerpendicularOffset(dSideCover + diameter * 0.5, vecSeg1.at(0).GetLineVec());

			for (int j = 0; j < (int)vecMergeSeg.size(); j++)
			{
				// vecMergeSeg.at(i).PerpendicularOffset(dSideCover + diameter * 0.5, vecSegTmp.at(0).GetLineVec());
				DVec3d  vec = m_vecFaceNormal.at(j);
				if (GetisReverse())
				{
					vec.Negate();
				}
				vecSeg1.at(j).PerpendicularOffset(offset, vec);
			}

			tag = MakeRebars(PopSetIds().at(i), vecSeg2, vecSeg1, rebarDir, strRebarSize, spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();
		}
		if (m_vecRebarPtsLayer.size() > 1)
		{
			for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
			{
				int n = m + 1;
				RebarPoint rbPt;
				rbPt.Layer = GetMainRebars().at(i).rebarLevel;
				rbPt.vecDir = rebarDir;
				rbPt.ptstr = m_vecRebarPtsLayer.at(m);
				rbPt.ptend = m_vecRebarPtsLayer.at(n);
				rbPt.DataExchange = DataExchange;
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
				rbPt.Layer = i;
				rbPt.vecDir = rebarDir;
				rbPt.ptstr = m_vecTwinRebarPtsLayer.at(m);
				rbPt.ptend = m_vecTwinRebarPtsLayer.at(n);
				rbPt.DataExchange = DataExchange;
				g_vecTwinRebarPtsNoHole.push_back(rbPt);
				m++;
			}
		}
	}

	DrawPreviewLines();

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

long MultiPlaneRebarAssemblyEx::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool MultiPlaneRebarAssemblyEx::OnDoubleClick()
{
	ElementId tmpid = GetSelectedElement();
	if (tmpid == 0)
	{
		return false;
	}
	DgnModelRefP modelp = GetSelectedModel();
	EditElementHandle ehSel;
	if (modelp == nullptr)
	{
		if (ehSel.FindByID(tmpid, ACTIVEMODEL) != SUCCESS)
		{
			ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
			for (DgnModelRefP modelRef : modelRefCol)
			{
				if (ehSel.FindByID(tmpid, modelRef) == SUCCESS)
				{
					modelp = modelRef;
					break;
				}

			}
		}
	}
	else
	{
		ehSel.FindByID(tmpid, modelp);
	}
	// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// 	CFacesRebarDlg dlg(ehSel,CWnd::FromHandle(MSWIND));
	// 	
	// 	dlg.SetConcreteId(GetConcreteOwner());
	// 	if (IDCANCEL == dlg.DoModal())
	// 		return false;

	return true;
}

bool MultiPlaneRebarAssemblyEx::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();

	//	SetWallData(ehWall);
	MakeRebars(modelRef);
	Save(modelRef);
	return true;
}


//平面配筋--begin
PlaneRebarAssemblyEx::PlaneRebarAssemblyEx(ElementId id, DgnModelRefP modelRef) :FacesRebarAssemblyEx(id, modelRef)
{
	m_vecRebarStartEnd.clear();
}

PlaneRebarAssemblyEx::~PlaneRebarAssemblyEx()
{
}

bool PlaneRebarAssemblyEx::makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, const PIT::PITRebarEndTypes& endTypes)
{
	DPoint3d  startPt = endTypes.beg.GetptOrgin();
	DPoint3d  endPt = endTypes.end.GetptOrgin();
	DPoint3d  ptStartBack = endTypes.beg.GetptOrgin();
	DPoint3d  ptEndBack = endTypes.end.GetptOrgin();
	//确保起点终点是从小到大---begin
	DVec3d vec = endPt - startPt;
	DVec3d vecX = DVec3d::From(1, 0, 0);
	vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
	vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
	vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
	vec.Normalize();

	if (GetIntersectPointsWithNegs(m_Negs, startPt, endPt))
	{
		return false;
	}

	m_vecRebarPtsLayer.push_back(startPt);
	m_vecRebarPtsLayer.push_back(endPt);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = GetConcrete().sideCover * uor_per_mm;
	vector<DPoint3d> tmppts;
	vector<vector<DPoint3d>> vecPtRebars;
	vector<DPoint3d> tmpptsTmp;
	if (m_face.IsValid()) // 与原实体相交(无孔洞)
	{
		//将原平面往法向方向拉伸为一个实体
		EditElementHandle eehSolid;
		ISolidKernelEntityPtr ptarget;
		SolidUtil::Convert::ElementToBody(ptarget, m_face, true, true, true);
		if (SUCCESS == SolidUtil::Modify::ThickenSheet(ptarget, 5000.0 * uor_per_mm, 5000 * uor_per_mm))
		{
			if (SUCCESS == SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL))
			{
				DRange3d range;
				mdlElmdscr_computeRange(&range.low, &range.high, eehSolid.GetElementDescrP(), NULL);
				DVec3d tmpVec = endPt - startPt;
				tmpVec.ScaleToLength(5000 * UOR_PER_MilliMeter);
				endPt.Add(tmpVec);
				tmpVec.Negate();
				startPt.Add(tmpVec);
				GetIntersectPointsWithOldElm(tmpptsTmp, &eehSolid, startPt, endPt, dSideCover);
				if (tmpptsTmp.size() > 1)
				{
					// 存在交点为两个以上的情况
					GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, startPt, endPt, &eehSolid, dSideCover);
				}
			}
		}
	}

	if (tmpptsTmp.size() < 2)
	{
		vector<DPoint3d> vecPt;
		vecPt.push_back(startPt);
		vecPt.push_back(endPt);

		vecPtRebars.push_back(vecPt);
	}


	/*EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
	eehline.AddToModel();*/

	for (size_t i = 0; i < vecPtRebars.size(); i++)
	{
		startPt = vecPtRebars.at(i).at(0);
		endPt = vecPtRebars.at(i).at(1);
		GetIntersectPointsWithHoles(tmppts, m_useHoleehs, startPt, endPt, dSideCover);
		map<int, DPoint3d> map_pts;
		bool isStr = false;
		for (DPoint3d pt : tmppts)
		{
			if (ExtractFacesTool::IsPointInLine(pt, startPt, endPt, ACTIVEMODEL, isStr))
			{
				int dis = (int)startPt.Distance(pt);
				if (map_pts.find(dis) != map_pts.end())
				{
					dis = dis + 1;
				}
				map_pts[dis] = pt;
			}
		}
		if (map_pts.find(0) != map_pts.end())
		{
			map_pts[1] = startPt;
		}
		else
		{
			map_pts[0] = startPt;
		}
		int dis = (int)startPt.Distance(endPt);
		if (map_pts.find(dis) == map_pts.end())
		{
			map_pts[dis] = endPt;
		}
		else
		{
			dis = dis + 1;
			map_pts[dis] = endPt;
		}



		for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
		{
			PITRebarEndTypes endTypesTmp = endTypes;
			PITRebarCurve rebar;
			RebarVertexP vex;
			DPoint3d ptStart(itr->second);
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(itr->second);
			vex->SetType(RebarVertex::kStart);
			endTypesTmp.beg.SetptOrgin(itr->second);

			map<int, DPoint3d>::iterator itrplus = ++itr;
			if (itrplus == map_pts.end())
			{
				break;
			}

			endTypesTmp.end.SetptOrgin(itrplus->second);

			DPoint3d ptEnd(itrplus->second);
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(itrplus->second);
			vex->SetType(RebarVertex::kEnd);

			/*double dis1 = ptStart.Distance(ptStartBack);
			if (COMPARE_VALUES_EPS(dis1, 0, 1) != 0)
			{
				endTypesTmp.beg.SetType(PITRebarEndType::kNone);
			}
			double dis2 = ptEnd.Distance(ptEndBack);

			if (COMPARE_VALUES_EPS(dis2, 0, 1) != 0)
			{
				endTypesTmp.end.SetType(PITRebarEndType::kNone);
			}*/
			// 			EditElementHandle eeh;
			// 			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptStart, ptEnd), true, *ACTIVEMODEL);
			// 			eeh.AddToModel();
			rebar.EvaluateEndTypes(endTypesTmp);
			//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
			rebars.push_back(rebar);
		}
	}
	return true;
}

RebarSetTag* PlaneRebarAssemblyEx::MakeRebars
(
	ElementId&          rebarSetId,
	LineSegment			rebarLine,
	LineSegment			vec,
	int					dir,
	BrStringCR          sizeKey,
	double              spacing,
	double              startOffset,
	double              endOffset,
	int					level,
	int					grade,
	int					DataExchange,
	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
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
	double startbendLen, endbendLen;
	double begStraightAnchorLen = 0, endStraightAnchorLen = 0;
	auto ProcessEndType = [&](RebarEndType& endType, PIT::EndType const& srcEndType, BrStringCR sizeKey, DgnModelRefP modelRef, double& bendRadius, double& bendLen, double& straightAnchorLen)
	{
		switch (srcEndType.endType)
		{
		case 0: //无
		case 1: //弯曲
		case 2: //吊钩
		case 3: //折线
			endType.SetType(RebarEndType::kNone);
			break;
		case 7: //直锚
			endType.SetType(RebarEndType::kLap);
			straightAnchorLen = srcEndType.endPtInfo.value1; //锚入长度
			break;
		case 4: //90度弯钩
		case 5: //135度弯钩
		case 6: //180度弯钩
		{
			if (srcEndType.endType == 4) endType.SetType(RebarEndType::kBend);
			else if (srcEndType.endType == 5) endType.SetType(RebarEndType::kCog);
			else if (srcEndType.endType == 6) endType.SetType(RebarEndType::kHook);
			bendRadius = srcEndType.endPtInfo.value1;
			if (COMPARE_VALUES(bendRadius, 0) == 0)
			{
				bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false); //乘以了30
			}
			bendLen = srcEndType.endPtInfo.value3; //预留长度
			if (COMPARE_VALUES(bendLen, 0) == 0)
			{
				bendLen = RebarCode::GetBendLength(sizeKey, endType, modelRef); //乘以了100
			}
		}
		break;
		case 8: //用户
			endType.SetType(RebarEndType::kCustom);
			break;
		default:
			break;
		}
	};
	// 使用新函数处理起始端和结束端
	ProcessEndType(endTypeStart, endType[0], sizeKey, modelRef, startbendRadius, startbendLen, begStraightAnchorLen);
	ProcessEndType(endTypeEnd, endType[1], sizeKey, modelRef, endbendRadius, endbendLen, endStraightAnchorLen);

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double adjustedXLen, adjustedSpacing;
	double sideCov = GetConcrete().sideCover*uor_per_mm;
	double updownSideCover = 50 * uor_per_mm;
	rebarLine.PerpendicularOffset(startOffset, vec.GetLineVec());
	if (GetConcrete().isFaceUnionRebar && !_ehCrossPlanePre.IsValid() && !_ehCrossPlaneNext.IsValid())
	{
		double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);

		rebarLine.PerpendicularOffset(diameter + spacing, vec.GetLineVec());
		rebarLine.PerpendicularOffset(diameter, PopfaceNormal());
		vec.Shorten(sideCov * 2 + diameter + spacing, true);
		vec.Shorten(-sideCov * 2 - diameter * 2, false);
		adjustedXLen = vec.GetLength() - diameter - startOffset - endOffset;
	}
	else
	{
		adjustedXLen = vec.GetLength() - updownSideCover * 2 - startOffset - endOffset;
	}
	int numRebar = 0;
	numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	adjustedSpacing = spacing;
	//	rebarLine.PerpendicularOffset(startOffset, vec.GetLineVec());
	//	rebarLine.PerpendicularOffset(sideCov, vec.GetLineVec());
	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0 && endType[0].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0 && endType[1].endType != 7)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

	// 	EditElementHandle eeh;
	// 	LineHandler::CreateLineElement(eeh, NULL, rebarLine.GetLineSeg(), true, *ACTIVEMODEL);
	// 	eeh.AddToModel();


	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);
	//rebarLine.Shorten(sideCov + endTypeStartOffset,true);
	rebarLine.Shorten(/*sideCov + */endTypeStartOffset, true);
	start.SetptOrgin(rebarLine.GetLineStartPoint());
	//rebarLine.Shorten(sideCov + endTypEendOffset, false);
	rebarLine.Shorten(/*sideCov + */endTypEendOffset, false);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);
	end.SetptOrgin(rebarLine.GetLineEndPoint());

	PITRebarEndTypes   endTypes = { start, end };
	//	rebarLine.PerpendicularOffset(sideCov+diameter*0.5, vec.GetLineVec());
	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurves;
		if (i == numRebar - 1)//如果是最后一根，要判断当前还有多少距离,符合距离要求就要再布置一根
		{
			double sDis = adjustedXLen - (numRebar - 2)*adjustedSpacing;
			if (sDis > spacing)
			{
				DVec3d dvec = vec.GetLineVec();
				rebarLine.PerpendicularOffset((sDis - spacing), dvec);
			}
			else
			{
				DVec3d dvec = vec.GetLineVec();
				dvec.Negate();
				rebarLine.PerpendicularOffset((spacing - sDis), dvec);
			}
		}
		endTypes.beg.SetptOrgin(rebarLine.GetLineStartPoint());
		endTypes.end.SetptOrgin(rebarLine.GetLineEndPoint());

		makeRebarCurve(rebarCurves, endTypes);

		//		xPos += adjustedSpacing
		DVec3d offsetVec = vec.GetLineVec();
		rebarLine.PerpendicularOffset(adjustedSpacing, offsetVec);

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);
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
		// 		EditElementHandle eeh;
		// 		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		// 		eeh.AddToModel();

		RebarElementP rebarElement = NULL;
		if (!g_FacePreviewButtonsDown)//预览标志，预览状态下不要生成钢筋
		{
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
				Stype = "front";
			}
			else if (DataExchange == 1)
			{
				Stype = "midden";
			}
			else
			{
				Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}

		j++;
	}

	m_vecRebarCurvePt.push_back(rebarCurvesNum);
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

bool PlaneRebarAssemblyEx::MakeRebars(DgnModelRefP modelRef)
{
	_d = 1.0;

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

	RebarSetTagArray rsetTags;
	m_vecRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
	double dLevelSpace = 0;
	double updownSideCover = 50 * uor_per_mm;
	double dSideCover = GetConcrete().sideCover * uor_per_mm;
	double dPositiveCover = GetConcrete().postiveCover * uor_per_mm;
	double offset = dPositiveCover;

	int iRebarSetTag = 0;
	int iRebarLevelNum = GetConcrete().rebarLevelNum;
	vector<PIT::EndType> vecEndType;

	// 孔洞规避前 面配筋钢筋点 按层存储，之前不同层的不能删掉
	std::vector<RebarPoint> vecRebarPoint;
	ElementId contid = this->FetchConcrete();
	GetElementXAttribute(contid, vecRebarPoint, vecRebarPointsXAttribute, ACTIVEMODEL);
	std::map<int, vector<RebarPoint>> mapRebarPoint;
	for (RebarPoint stRebarPt : vecRebarPoint)
	{
		auto iter = mapRebarPoint.find(stRebarPt.DataExchange);
		if (iter != mapRebarPoint.end())
		{
			iter->second.push_back(stRebarPt);
		}
		else
		{
			vector<RebarPoint> vecTemp;
			vecTemp.push_back(stRebarPt);
			mapRebarPoint[stRebarPt.DataExchange] = vecTemp;
		}
	}
	// end

	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		PopSetIds().push_back(0);
		RebarSetTag* tag = NULL;
		vector<PIT::EndType> vecEndType;
		if (GetvecEndTypes().empty())		//没有设置端部样式，设置默认值
		{
			EndType endType;
			memset(&endType, 0, sizeof(endType));
			vecEndType = { { 0,0,0 } ,endType };
		}
		else
		{
			vecEndType = GetvecEndTypes().at(i);
		}

		BrString strRebarSize(GetMainRebars().at(i).rebarSize);
		double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
		double spacing = GetMainRebars().at(i).spacing * uor_per_mm;
		double startOffset = GetMainRebars().at(i).startOffset * uor_per_mm;
		double endOffset = GetMainRebars().at(i).endOffset * uor_per_mm;
		int	rebarDir = GetMainRebars().at(i).rebarDir;
		double levelspacing = GetMainRebars().at(i).levelSpace * uor_per_mm;
		int DataExchange = GetMainRebars().at(i).datachange;
		m_vecRebarPtsLayer.clear();
		m_vecTwinRebarPtsLayer.clear();
		LineSegment lineSeg1 = m_LineSeg1;
		LineSegment lineSeg2 = m_LineSeg2;
		vector<CVector3D> vecEndNormal(2);
		CVector3D	endNormal;	//端部弯钩方向
		if (rebarDir == 1)	//纵向钢筋
		{
			if (GetConcrete().isFaceUnionRebar && !_ehCrossPlanePre.IsValid() && !_ehCrossPlaneNext.IsValid())
			{
				continue;
			}
			if (GetvecEndTypes().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = GetfaceNormal();
					CVector3D rebarVec = m_LineSeg2.GetLineVec();
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}

			DVec3d vec = m_LineSeg1.GetLineVec();
			offset += levelspacing;
			if (0 == i)	//首层偏移当前钢筋直径
			{
				offset += diameter * 0.5;
			}
			else
			{
				double diameterPre = RebarCode::GetBarDiameter(GetMainRebars().at(i - 1).rebarSize, modelRef);	//上一层钢筋直径
				offset += diameterPre * 0.5;	//偏移上一层钢筋的半径
				offset += diameter * 0.5;		//偏移当前层钢筋的半径
			}
			lineSeg2.PerpendicularOffset(offset, GetfaceNormal());
			lineSeg2.PerpendicularOffset(updownSideCover, vec);
			tag = MakeRebars(PopSetIds().at(i), lineSeg2, m_LineSeg1, rebarDir, strRebarSize, spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();
		}
		else
		{
			if (GetvecEndTypes().size() > 0)
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double dRotateAngle = vecEndType.at(k).rotateAngle;
					CVector3D rebarVec = m_LineSeg1.GetLineVec();
					endNormal = GetfaceNormal();
					endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
					vecEndNormal[k] = endNormal;
				}
			}

			DVec3d vec = m_LineSeg2.GetLineVec();
			offset += levelspacing;
			if (0 == i)	//首层偏移当前钢筋直径
			{
				offset += diameter * 0.5;
			}
			else
			{
				double diameterPre = RebarCode::GetBarDiameter(GetMainRebars().at(i - 1).rebarSize, modelRef);	//上一层钢筋直径
				offset += diameterPre * 0.5;	//偏移上一层钢筋的半径
				offset += diameter * 0.5;		//偏移当前层钢筋的半径
			}
			lineSeg1.PerpendicularOffset(offset, GetfaceNormal());
			lineSeg1.PerpendicularOffset(updownSideCover, vec);
			if (_ehCrossPlanePre.IsValid())
			{
				vecEndType[_anchorPos].endType = 7;
				vecEndType[_anchorPos].endPtInfo.value1 = _d;
			}
			if (_ehCrossPlaneNext.IsValid())
			{
				vecEndType[_bendPos].endType = 7;
				vecEndType[_bendPos].endPtInfo.value1 = _d;
				//vecEndType[_bendPos].endType = 4;
				//vecEndNormal[_bendPos] = PopfaceNormal();
				//vecEndType[_bendPos].endPtInfo.value3 = _d*2;
			}
			tag = MakeRebars(PopSetIds().at(i), lineSeg1, m_LineSeg2, rebarDir, strRebarSize, spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}

			vecEndType.clear();
		}

		// 删除属于当前成的钢筋
		auto itrTemp = mapRebarPoint.find(DataExchange);
		if (itrTemp != mapRebarPoint.end())
		{
			mapRebarPoint.erase(itrTemp);
		}
		// end

		if (m_vecRebarPtsLayer.size() > 1)
		{
			for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
			{
				int n = m + 1;
				RebarPoint rbPt;
				rbPt.Layer = GetMainRebars().at(i).rebarLevel;
				rbPt.vecDir = rebarDir;
				rbPt.ptstr = m_vecRebarPtsLayer.at(m);
				rbPt.ptend = m_vecRebarPtsLayer.at(n);
				rbPt.DataExchange = DataExchange;
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
				rbPt.Layer = GetMainRebars().at(i).rebarLevel;
				rbPt.vecDir = rebarDir;
				rbPt.ptstr = m_vecTwinRebarPtsLayer.at(m);
				rbPt.ptend = m_vecTwinRebarPtsLayer.at(n);
				rbPt.DataExchange = DataExchange;
				g_vecTwinRebarPtsNoHole.push_back(rbPt);
				m++;
			}
		}
	}

	for (auto itr = mapRebarPoint.begin(); itr != mapRebarPoint.end(); itr++)
	{
		g_vecRebarPtsNoHole.insert(g_vecRebarPtsNoHole.end(), itr->second.begin(), itr->second.end());
	}

	DrawPreviewLines();

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}


bool PlaneRebarAssemblyEx::OnDoubleClick()
{
	ElementId testid = GetElementId();
	PlaneRebarAssemblyEx* faceRebar = REA::Load<PlaneRebarAssemblyEx>(testid, ACTIVEMODEL);

	ElementId tmpid = GetSelectedElement();
	if (tmpid == 0)
	{
		return false;
	}
	DgnModelRefP modelp = GetSelectedModel();
	EditElementHandle ehSel;
	if (modelp == nullptr)
	{
		if (ehSel.FindByID(tmpid, ACTIVEMODEL) != SUCCESS)
		{
			ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
			for (DgnModelRefP modelRef : modelRefCol)
			{
				if (ehSel.FindByID(tmpid, modelRef) == SUCCESS)
				{
					modelp = modelRef;
					break;
				}

			}
		}
	}
	else
	{
		ehSel.FindByID(tmpid, modelp);
	}
	// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// 	CFacesRebarDlg dlg(ehSel,CWnd::FromHandle(MSWIND));
	// 	
	// 	dlg.SetConcreteId(GetConcreteOwner());
	// 	if (IDCANCEL == dlg.DoModal())
	// 		return false;

	return true;
}

bool PlaneRebarAssemblyEx::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();

	//	SetWallData(ehWall);
	MakeRebars(modelRef);
	Save(modelRef);
	return true;
}

bool PlaneRebarAssemblyEx::AnalyzingFaceGeometricData(EditElementHandleR eeh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eeh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	if (GetfaceType() == FaceType::other)
	{
		MSElementDescrP msDscp = eeh.GetElementDescrP();
		if (msDscp == NULL)
		{
			return false;
		}

		// 取一个元素的范围
		DPoint3d minP = { 0 }, maxP = { 0 };
		mdlElmdscr_computeRange(&minP, &maxP, msDscp, NULL); // 计算指定元素描述符中元素的范围

		DVec3d ptVec = maxP - minP;
		DPoint3d ptEnd1 = DPoint3d::FromZero();
		DPoint3d ptEnd2 = DPoint3d::FromZero();
		if (fabs(ptVec.z - 0.0) < uor_ref) // XOY 平面
		{
			if (COMPARE_VALUES_EPS(maxP.x - minP.x, maxP.y - minP.y < 0, EPS) >= 0)
			{
				ptEnd1 = maxP;
				ptEnd1.y = minP.y;

				ptEnd2 = maxP;
				ptEnd2.x = minP.x;
			}
			else
			{
				ptEnd1 = maxP;
				ptEnd1.x = minP.x;

				ptEnd2 = maxP;
				ptEnd2.y = minP.y;
			}

		}
		else if (fabs(ptVec.x - 0.0) < uor_ref) // YOZ平面
		{
			if (COMPARE_VALUES_EPS(maxP.y - minP.y, maxP.z - minP.z < 0, EPS) >= 0)
			{
				ptEnd1 = maxP;
				ptEnd1.z = minP.z;

				ptEnd2 = maxP;
				ptEnd2.y = minP.y;
			}
			else
			{
				ptEnd1 = maxP;
				ptEnd1.y = minP.y;

				ptEnd2 = maxP;
				ptEnd2.z = minP.z;
			}
		}
		else if (fabs(ptVec.y - 0.0) < uor_ref) // XOZ平面
		{
			if (COMPARE_VALUES_EPS(maxP.x - minP.x, maxP.z - minP.z < 0, EPS) >= 0)
			{
				ptEnd1 = maxP;
				ptEnd1.z = minP.z;

				ptEnd2 = maxP;
				ptEnd2.x = minP.x;
			}
			else
			{
				ptEnd1 = maxP;
				ptEnd1.x = minP.x;

				ptEnd2 = maxP;
				ptEnd2.z = minP.z;
			}
		}
		else
		{
			return false;
		}

		m_LineSeg1.SetLineSeg(DSegment3d::From(minP, ptEnd1));
		m_LineSeg2.SetLineSeg(DSegment3d::From(minP, ptEnd2));

		return true;
	}

	if (m_Solid != NULL && m_vecElm.empty())
	{
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(*m_Solid, Eleeh, Holeehs);
		m_Holeehs = Holeehs;
	}

	for (size_t i = 0; i < m_vecElm.size(); ++i)
	{
		//将孔洞上有板的孔洞过滤
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(m_vecElm[i], Eleeh, Holeehs);
		for (auto it : Holeehs)
		{
			double areap = 0;
			mdlMeasure_areaProperties(nullptr, &areap, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, it->GetElementDescrP(), 0);
			if (areap > BIGHOLEAREA)
			{
				m_Holeehs.push_back(it);
				continue;
			}
			//mdlElmdscr_addByModelRef(it->GetElementDescrP(), ACTIVEMODEL);
			DPoint3d holeMinPt = { 0,0,0 }, holeMaxPt = { 0,0,0 };
			mdlElmdscr_computeRange(&holeMinPt, &holeMaxPt, it->GetElementDescrP(), nullptr);
			DRange3d holeRange = DRange3d::From(holeMinPt, holeMaxPt);
			bool isValid = true;
			vector<MSElementDescrP> elmDecrP;
			for (size_t j = 0; j < m_vecElm.size(); ++j)
			{
				if (j == i)
				{
					continue;
				}
				EditElementHandle elmEeh(m_vecElm[j], m_vecElm[j].GetDgnModelP());
				DPoint3d elmMinPt = { 0,0,0 }, elmMaxPt = { 0,0,0 };
				mdlElmdscr_computeRange(&elmMinPt, &elmMaxPt, elmEeh.GetElementDescrP(), nullptr);
				DPoint3d midPt = elmMinPt; midPt.Add(elmMaxPt); midPt.Scale(0.5);
				if (holeRange.IsContained(midPt))
				{
					isValid = false;
					break;
				}
			}
			if (isValid)
			{
				m_Holeehs.push_back(it);
			}
		}
		//m_Holeehs.insert(m_Holeehs.end(), Holeehs.begin(), Holeehs.end());
	}
	//先转到xoy平面
// 	
// 	RotMatrix rMatrix;
// 	Transform trans;
// 	DPoint3d vecz = DPoint3d::From(0, 0, 1);
// 	mdlRMatrix_getIdentity(&rMatrix);
// 	mdlRMatrix_fromVectorToVector(&rMatrix, &GetfaceNormal(), &vecz);//旋转到xoy平面
// 	mdlTMatrix_fromRMatrix(&trans, &rMatrix);
// 	mdlTMatrix_setOrigin(&trans, &DPoint3d::From(0,0,0));
// 	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, TransformInfo(trans));
	EditElementHandle tmpeeh;
	if (m_UseXOYDir)//如果是板平面，以面的范围面作为基准面
	{
		DPoint3d minP, maxP;
		mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
		DPoint3d PTS[4];
		PTS[0] = PTS[1] = minP;
		PTS[1].x = maxP.x;
		PTS[2] = PTS[1]; PTS[2].y = maxP.y;
		PTS[3] = PTS[2]; PTS[3].x = minP.x;
		ShapeHandler::CreateShapeElement(tmpeeh, NULL, PTS, 4, true, *ACTIVEMODEL);
	}
	else
	{
		tmpeeh.Duplicate(eeh);
	}

	ElementAgenda agenda;//存放打散之后的元素
	DropGeometryPtr pDropGeometry = DropGeometry::Create();//创建一个DropGeometry实例来设置几何选项
	if (tmpeeh.GetElementType() == CMPLX_SHAPE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Complex);
	}
	else if (tmpeeh.GetElementType() == SURFACE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Solids);
	}
	else
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_LinearSegments);
	}
	if (SUCCESS != tmpeeh.GetDisplayHandler()->Drop(tmpeeh, agenda, *pDropGeometry))
	{
		agenda.Clear();
		mdlOutput_printf(MSG_STATUS, L"打散面失败！");
		return false;
	}

	class lineSort {
	public:
		bool operator()(const LineSegment &line1, const LineSegment &line2) const {
			return line1.GetLength() > line2.GetLength();
		}
	};
	std::set < LineSegment, lineSort > lineSegs;
	vector<ArcSegment> vecArcSeg;
	for (EditElementHandleR LineEeh : agenda)
	{
		if (LineEeh.GetElementType() == LINE_ELM)
		{
			DPoint3d pt[2];
			mdlLinear_extract(pt, NULL, LineEeh.GetElementP(), tmpeeh.GetModelRef());
			LineSegment lineSeg(pt[0], pt[1]);
			lineSegs.insert(lineSeg);
		}
	}

	// 取长度最长的边
	// mdlElmdscr_extractNormal(&PopfaceNormal(), NULL, tmpeeh.GetElementDescrCP(), NULL);
	//	PopfaceNormal().Negate();
	LineSegment maxLine = *lineSegs.begin();

	//针对斜切墙体，最长线可能是斜边，需要重新获取最长先段
	DVec3d vecmax = maxLine.GetLineVec();
	if (!m_UseXOYDir)
	{
		vecmax.Normalize();
		if (COMPARE_VALUES_EPS(vecmax.z, 0.00, 0.1) != 0)
		{
			maxLine = *(++lineSegs.begin());
		}
	}

	CVector3D  xVec(maxLine.GetLineVec());
	CVector3D  yVec = CVector3D(PopfaceNormal()).CrossProduct(xVec);
	BeMatrix   matrix = CMatrix3D::Ucs(maxLine.GetLineStartPoint(), xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	Transform trans;
	matrix.AssignTo(trans);
	trans.InverseOf(trans);
	tmpeeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(tmpeeh, TransformInfo(trans));
	//eeh.AddToModel();
	//tmpeeh.AddToModel();
// 
	if (m_vecElm.size())
	{
		if (m_slabUpFace)
		{
			SetfaceNormal(DVec3d::From(0, 0, -1));
		}
		else
		{
			SetfaceNormal(DVec3d::From(0, 0, 1));
		}
	}
	//计算平面的最大范围
	DPoint3d ptMin, ptMax;
	mdlElmdscr_computeRange(&ptMin, &ptMax, tmpeeh.GetElementDescrCP(), NULL);

	DPoint3d ptLineSeg1End = ptMax;
	DPoint3d ptLineSeg2End = ptMin;
	ptLineSeg1End.y = ptMin.y;
	ptLineSeg2End.y = ptMax.y;

	DSegment3d lineSeg1 = DSegment3d::From(ptMin, ptLineSeg1End);
	DSegment3d lineSeg2 = DSegment3d::From(ptMin, ptLineSeg2End);

	EditElementHandle ehLine1, ehLine2;
	LineHandler::CreateLineElement(ehLine1, NULL, lineSeg1, true, *ACTIVEMODEL);
	LineHandler::CreateLineElement(ehLine2, NULL, lineSeg2, true, *ACTIVEMODEL);
	// 	ehLine1.AddToModel();
	// 	ehLine2.AddToModel();

	trans.InverseOf(trans);
	ehLine1.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(ehLine1, TransformInfo(trans));
	ehLine2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(ehLine2, TransformInfo(trans));

	DPoint3d ptLine1[2];
	DPoint3d ptLine2[2];
	mdlLinear_extract(ptLine1, NULL, ehLine1.GetElementCP(), ACTIVEMODEL);
	mdlLinear_extract(ptLine2, NULL, ehLine2.GetElementCP(), ACTIVEMODEL);

	m_LineSeg1.SetLineSeg(DSegment3d::From(ptLine1[0], ptLine1[1]));
	m_LineSeg2.SetLineSeg(DSegment3d::From(ptLine2[0], ptLine2[1]));

	if (/*m_LineSeg1.GetLineVec().IsParallelTo(DVec3d::From(0,0,1))*/ abs(m_LineSeg1.GetLineVec().DotProduct(DVec3d::From(0, 0, 1))) > 0.9
		|| (abs(m_LineSeg1.GetLineVec().DotProduct(DVec3d::From(0, 1, 0))) > 0.9 && abs(m_LineSeg2.GetLineVec().DotProduct(DVec3d::From(1, 0, 0))) > 0.9))
	{
		LineSegment segTmp = m_LineSeg2;
		m_LineSeg2 = m_LineSeg1;
		m_LineSeg1 = segTmp;
	}
	else if (COMPARE_VALUES_EPS(m_LineSeg1.GetLineStartPoint().x, m_LineSeg1.GetLineEndPoint().x, EPS) == 0
		&& COMPARE_VALUES_EPS(m_LineSeg1.GetLineStartPoint().y, m_LineSeg1.GetLineEndPoint().y, EPS) != 0
		&& m_LineSeg2.GetLineVec().IsParallelTo(DVec3d::From(1, 0, 0)))
	{
		//Added by chenxuan 面配筋横向竖向方向不对
		LineSegment segTmp = m_LineSeg2;
		m_LineSeg2 = m_LineSeg1;
		m_LineSeg1 = segTmp;
	}

	if (_ehCrossPlanePre.IsValid())
	{
		DPoint3d ptNormal;
		DPoint3d ptInFace;
		mdlElmdscr_extractNormal(&ptNormal, &ptInFace, _ehCrossPlanePre.GetElementDescrCP(), NULL);
		if (ptNormal.IsParallelTo(m_LineSeg2.GetLineVec()))
		{
			LineSegment seg = m_LineSeg1;
			m_LineSeg1 = m_LineSeg2;
			m_LineSeg2 = seg;
		}

		//判断垂直面在起点处还是终点处
		double dis1 = ptInFace.Distance(m_LineSeg1.GetLineStartPoint());
		double dis2 = ptInFace.Distance(m_LineSeg1.GetLineEndPoint());
		_anchorPos = dis1 > dis2 ? 1 : 0;
	}
	if (_ehCrossPlaneNext.IsValid())
	{
		DPoint3d ptNormal;
		DPoint3d ptInFace;
		mdlElmdscr_extractNormal(&ptNormal, &ptInFace, _ehCrossPlaneNext.GetElementDescrCP(), NULL);
		if (ptNormal.IsParallelTo(m_LineSeg2.GetLineVec()))
		{
			LineSegment seg = m_LineSeg1;
			m_LineSeg1 = m_LineSeg2;
			m_LineSeg2 = seg;
		}

		//判断垂直面在起点处还是终点处
		double dis1 = ptInFace.Distance(m_LineSeg1.GetLineStartPoint());
		double dis2 = ptInFace.Distance(m_LineSeg1.GetLineEndPoint());
		_bendPos = dis1 > dis2 ? 1 : 0;
	}
	// start确保向量是从绝对坐标系最低往最高
	auto SwapLineSeg = [&](PIT::LineSegment& lineSeg1, PIT::LineSegment& lineSeg2,
		Dpoint3d& ptSeg1Start, Dpoint3d& ptSeg1End, Dpoint3d& ptSeg2Start, Dpoint3d& ptSeg2End)
	{
		swap(ptSeg1Start, ptSeg1End);
		lineSeg1.SetLineStartPoint(ptSeg1Start);
		lineSeg1.SetLineEndPoint(ptSeg1End);
		lineSeg2.SetLineStartPoint(ptSeg2Start);
		lineSeg2.SetLineEndPoint(ptSeg2End);
	};
	auto StandardStartAndEnd = [&](PIT::LineSegment& lineSeg1, PIT::LineSegment& lineSeg2)
	{
		DVec3d vec1 = lineSeg1.GetLineVec();
		DVec3d vec2 = lineSeg2.GetLineVec();
		double length1 = lineSeg1.GetLength();
		double length2 = lineSeg2.GetLength();
		Dpoint3d ptSeg1Start = lineSeg1.GetLineStartPoint();
		Dpoint3d ptSeg1End = lineSeg1.GetLineEndPoint();
		Dpoint3d ptSeg2Start = lineSeg2.GetLineStartPoint();
		Dpoint3d ptSeg2End = lineSeg2.GetLineEndPoint();
		if (vec1.DotProduct(DVec3d::From(-1, 0, 0)) > 0.9)
		{
			ptSeg2Start = ptSeg1End;
			movePoint(vec1, ptSeg2End, length1);
			SwapLineSeg(lineSeg1, lineSeg2, ptSeg1Start, ptSeg1End, ptSeg2Start, ptSeg2End);
		}
		else if (vec1.DotProduct(DVec3d::From(0, -1, 0)) > 0.9)
		{
			ptSeg2Start = ptSeg1End;
			movePoint(vec1, ptSeg2End, length1);
			SwapLineSeg(lineSeg1, lineSeg2, ptSeg1Start, ptSeg1End, ptSeg2Start, ptSeg2End);
		}
		else if (vec1.DotProduct(DVec3d::From(0, 0, -1)) > 0.9)
		{
			ptSeg2Start = ptSeg1End;
			movePoint(vec1, ptSeg2End, length1);
			SwapLineSeg(lineSeg1, lineSeg2, ptSeg1Start, ptSeg1End, ptSeg2Start, ptSeg2End);
		}
	};
	StandardStartAndEnd(m_LineSeg1, m_LineSeg2);
	StandardStartAndEnd(m_LineSeg2, m_LineSeg1);
	// end确保向量是从绝对坐标系最低往最高
	PopvecFrontPts().push_back(m_LineSeg1.GetLineStartPoint());
	PopvecFrontPts().push_back(m_LineSeg1.GetLineEndPoint());
	return true;
}

void PlaneRebarAssemblyEx::CalculateUseHoles(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetConcrete().sideCover*uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

	if (GetConcrete().isHandleHole)//计算需要处理的孔洞
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
//平面配筋---end
long PlaneRebarAssemblyEx::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		map << _d;
		return 0;
	}
	default:
		break;
	}
	return -1;
}

//弧面配筋---begin
long CamberedSurfaceRebarAssemblyEx::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool CamberedSurfaceRebarAssemblyEx::CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
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

bool CamberedSurfaceRebarAssemblyEx::makeLineRebarCurve(vector<PITRebarCurve>& rebar, ArcSegment arcSeg, double dLen, double space, double startOffset, double endOffset, PITRebarEndTypes & endTypes)
{
	// 	DEllipse3d seg = DEllipse3d::FromPointsOnArc(arcSeg.ptStart, arcSeg.ptMid, arcSeg.ptEnd);
	// 
	// 	ICurvePrimitivePtr curve = ICurvePrimitive::CreateArc(seg);
	// 
	// 	CurveLocationDetail arcDetail;
	// 	curve->PointAtSignedDistanceFromFraction(0, space, false, arcDetail);
	// 	DPoint3d ptStart = arcDetail.point;
	// 	DPoint3d vec = { 0,0,1 };
	// 	vec.Scale(startOffset);
	// 	ptStart.Add(vec);
	// 
	// 	RebarVertexP vex;
	// 	vex = &rebar.PopVertices().NewElement();
	// 	vex->SetIP(ptStart);
	// 	vex->SetType(RebarVertex::kStart);
	// 	endTypes.beg.SetptOrgin(ptStart);
	// 
	// 	DPoint3d ptEnd = arcDetail.point;
	// 	ptEnd.z += dLen;
	// 	vec.Normalize();
	// 	vec.Negate();
	// 	vec.Scale(endOffset);
	// 	ptEnd.Add(vec);
	// 	endTypes.end.SetptOrgin(ptEnd);
	// 
	// 	vex = &rebar.PopVertices().NewElement();
	// 	vex->SetIP(ptEnd);
	// 	vex->SetType(RebarVertex::kEnd);
	// 
	// 	rebar.EvaluateEndTypes(endTypes);
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
	double dSideCover = GetConcrete().sideCover*uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, ptStart, ptEnd, dSideCover, matrix);

	/*EditElementHandle eehline;
	LineHandler::CreateLineElement(eehline, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
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

bool CamberedSurfaceRebarAssemblyEx::makeArcWallRebarCurve(vector<PIT::PITRebarCurve>& rebar, ArcSegment arcSeg, double space, double startOffset, double endOffset, PITRebarEndTypes& endTypes)
{
	// 	DPoint3d ptArcStart = arcSeg.ptStart;
	// 	ptArcStart.z += space;
	// 	DPoint3d ptArcEnd = arcSeg.ptEnd;
	// 	ptArcEnd.z += space;
	// 	DPoint3d ptArcCenter = arcSeg.ptCenter;
	// 	ptArcCenter.z += space;
	// 	DPoint3d ptArcMid = arcSeg.ptMid;
	// 	ptArcMid.z += space;
	// 
	// 	return CalculateArc(rebar, ptArcStart, ptArcMid, ptArcEnd);
	DPoint3d ptArcStart = arcSeg.ptStart;
	ptArcStart.z += space;
	DPoint3d ptArcEnd = arcSeg.ptEnd;
	ptArcEnd.z += space;
	DPoint3d ptArcCenter = arcSeg.ptCenter;
	ptArcCenter.z += space;
	DPoint3d ptArcMid = arcSeg.ptMid;
	ptArcMid.z += space;


	EditElementHandle arceeh;
	ArcHandler::CreateArcElement(arceeh, nullptr, DEllipse3d::FromPointsOnArc(ptArcStart, ptArcMid, ptArcEnd), true, *ACTIVEMODEL);
	//arceeh.AddToModel();
	vector<DPoint3d> pts;
	GetARCIntersectPointsWithHoles(pts, m_useHoleehs, ptArcStart, ptArcEnd, ptArcMid);


	if (pts.size() > 0)
	{
		/*EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(pts[0], pts[1]), true, *ACTIVEMODEL);
		eeh.AddToModel();*/
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

		if (ISPointInHoles(m_Holeehs, tmpMid))
		{
			if (ISPointInHoles(m_Holeehs, tmpstr) && ISPointInHoles(m_Holeehs, tmpend))
			{
				continue;
			}
		}
		if (CalculateArc(trebar, tmpstr, tmpMid, tmpend))
		{
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(tmpstr, tmpMid, tmpend), true, *ACTIVEMODEL);
			if (g_FacePreviewButtonsDown)
			{//预览状态下画线,并存储线
				arceeh1.AddToModel();
				m_allPreViewEehs.push_back(arceeh1.GetElementRef());
			}
			rebar.push_back(trebar);
		}
	}
	return true;
}


bool CamberedSurfaceRebarAssemblyEx::OnDoubleClick()
{
	ElementId tmpid = GetSelectedElement();
	if (tmpid == 0)
	{
		return false;
	}
	DgnModelRefP modelp = GetSelectedModel();
	EditElementHandle ehSel;
	if (modelp == nullptr)
	{
		if (ehSel.FindByID(tmpid, ACTIVEMODEL) != SUCCESS)
		{
			ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
			for (DgnModelRefP modelRef : modelRefCol)
			{
				if (ehSel.FindByID(tmpid, modelRef) == SUCCESS)
				{
					modelp = modelRef;
					break;
				}

			}
		}
	}
	else
	{
		ehSel.FindByID(tmpid, modelp);
	}
	// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// 	CFacesRebarDlg dlg(ehSel,faces CWnd::FromHandle(MSWIND));
	// 	dlg.SetConcreteId(GetConcreteOwner());
	// 	if (IDCANCEL == dlg.DoModal())
	// 		return false;

	return true;
}

bool CamberedSurfaceRebarAssemblyEx::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();

	//	SetWallData(ehWall);
	MakeRebars(modelRef);
	Save(modelRef);
	return true;
}

bool CamberedSurfaceRebarAssemblyEx::AnalyzingFaceGeometricData(EditElementHandleR eeh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eeh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	if (m_Solid != NULL)
	{
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(*m_Solid, Eleeh, Holeehs);
		m_Holeehs = Holeehs;

		vector<MSElementDescrP> vecDownFaceLine;
		vector<MSElementDescrP> vecDownFontLine;
		vector<MSElementDescrP> vecDownBackLine;
		ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine, vecDownBackLine, NULL);

		for (size_t i = 0; i < vecDownFaceLine.size(); i++)
		{
			if (vecDownFaceLine[i] != nullptr)
			{
				if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
				{
					DEllipse3d ellipsePro;
					mdlArc_extractDEllipse3d(&ellipsePro, &vecDownFaceLine[i]->el);
					DPoint3d ptStart, ptEnd;
					ellipsePro.EvaluateEndPoints(ptStart, ptEnd);
					double dRadius = ptStart.Distance(ellipsePro.center);
					if (dRadius > _dOuterArcRadius)
					{
						_dOuterArcRadius = dRadius;
					}
				}
			}
		}
	}

	ElementAgenda agenda;//存放打散之后的元素
	DropGeometryPtr pDropGeometry = DropGeometry::Create();//创建一个DropGeometry实例来设置几何选项
	if (eeh.GetElementType() == CMPLX_SHAPE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Complex);
	}
	else if (eeh.GetElementType() == SURFACE_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Solids);
	}
	else
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_LinearSegments);
	}
	if (SUCCESS != eeh.GetDisplayHandler()->Drop(eeh, agenda, *pDropGeometry))
	{
		agenda.Clear();
		mdlOutput_printf(MSG_STATUS, L"打散面失败！");
		return false;
	}

	DEllipse3d ellipsePro;
	ArcSegment arcSeg;
	vector<LineSegment> vecLineSeg;
	vector<ArcSegment> vecArcSeg;
	for (EditElementHandleR LineEeh : agenda)
	{
		if (LineEeh.GetElementType() == ARC_ELM)
		{
			mdlArc_extractDEllipse3d(&ellipsePro, LineEeh.GetElementP());
			ellipsePro.EvaluateEndPoints(arcSeg.ptStart, arcSeg.ptEnd);
			arcSeg.dRadius = arcSeg.ptStart.Distance(ellipsePro.center);
			arcSeg.ptCenter = ellipsePro.center;
			arcSeg.dLen = ellipsePro.ArcLength();
			mdlElmdscr_pointAtDistance(&arcSeg.ptMid, NULL, arcSeg.dLen / 2, LineEeh.GetElementDescrP(), 1e-6);
			vecArcSeg.push_back(arcSeg);
		}

		if (LineEeh.GetElementType() == LINE_ELM)
		{
			DPoint3d pt[2];
			mdlLinear_extract(pt, NULL, LineEeh.GetElementP(), eeh.GetModelRef());
			LineSegment lineSeg(pt[0], pt[1]);
			vecLineSeg.push_back(lineSeg);
		}
	}

	if (vecArcSeg.size() != 2 && vecLineSeg.size() != 2)
	{
		return false;
	}

	if (COMPARE_VALUES_EPS(vecArcSeg[0].dRadius, vecArcSeg[1].dRadius, 1e-6) != 0)//圆环面
	{
		m_height = fabs(vecArcSeg[0].dRadius - vecArcSeg[1].dRadius);
		//弧线取圆环内弧
		m_ArcSeg = vecArcSeg[0].dRadius < vecArcSeg[1].dRadius ? vecArcSeg[0] : vecArcSeg[1];
		//		m_arcType = ArcType::Torus;
	}
	else	//侧面
	{
		m_height = vecLineSeg[0].GetLength();	//任意取一条直角边即为
		//弧线取z值最小的弧线
		m_ArcSeg = vecArcSeg[0].ptCenter.z < vecArcSeg[1].ptCenter.z ? vecArcSeg[0] : vecArcSeg[1];
		//		m_arcType = ArcType::CurvedSurface;
	}
	// mdlElmdscr_extractNormal(&PopfaceNormal(), NULL, eeh.GetElementDescrCP(), NULL);

	return true;
}

bool CamberedSurfaceRebarAssemblyEx::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	CalculateUseHoles(modelRef);

	RebarSetTagArray rsetTags;
	m_vecRebarStartEnd.clear();
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
	double dLevelSpace = 0;
	double dSideCover = GetConcrete().sideCover * uor_per_mm;


	int iRebarSetTag = 0;
	int iRebarLevelNum = GetConcrete().rebarLevelNum;
	int iTwinbarSetIdIndex = 0;
	vector<PIT::EndType> vecEndType;
	double newRadius = m_ArcSeg.dRadius;
	double diameterPre = 0.0;

	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		RebarSetTag* tag = NULL;
		vector<PIT::EndType> vecEndType;
		if (GetvecEndTypes().empty())		//没有设置端部样式，设置默认值
		{
			EndType endType;
			memset(&endType, 0, sizeof(endType));
			vecEndType = { { 0,0,0 },{0,0,0} ,endType };
		}
		else
		{
			vecEndType = GetvecEndTypes().at(i);
		}

		BrString strRebarSize(GetMainRebars().at(i).rebarSize);
		double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
		double spacing = GetMainRebars().at(i).spacing * uor_per_mm;
		double startOffset = GetMainRebars().at(i).startOffset * uor_per_mm;
		double endOffset = GetMainRebars().at(i).endOffset * uor_per_mm;
		int	rebarDir = GetMainRebars().at(i).rebarDir;
		int DataExchange = GetMainRebars().at(i).datachange;
		double levelspacing = GetMainRebars().at(i).levelSpace * uor_per_mm;

		double radiusOffset = 0;
		if (0 == i)	//首层偏移当前钢筋半径
		{
			radiusOffset += GetConcrete().postiveCover * uor_per_mm;
			radiusOffset += diameter * 0.5;
		}
		else
		{
			diameterPre = RebarCode::GetBarDiameter(GetMainRebars().at(i - 1).rebarSize, modelRef);	//上一层钢筋直径
			radiusOffset += diameterPre * 0.5;	//偏移上一层钢筋的半径
			radiusOffset += diameter * 0.5;	//偏移当前层钢筋的半径
		}

		radiusOffset += GetMainRebars().at(i).levelSpace * uor_per_mm;	//偏移层间距

		ArcSegment newArc = m_ArcSeg;
		// 		DPoint3d vecArc = newArc.ptCenter - newArc.ptStart;
		// 		vecArc.Normalize();
		// 		vecArc.z = 0;
		// 		DPoint3d ptLine1 = newArc.ptStart;
		// 		DPoint3d ptLine2 = newArc.ptStart;
		// 		DPoint3d ptNormal = GetfaceNormal();
		// 		ptNormal.z = 0;
		// 		double dot = vecArc.DotProduct(ptNormal);
		if (COMPARE_VALUES_EPS(m_ArcSeg.dRadius, _dOuterArcRadius, 10) == 0/*COMPARE_VALUES(dot, 0) > 0*/)  //外弧
		{
			newRadius -= radiusOffset;
		}
		else	//内弧
		{
			newRadius += radiusOffset;
		}

		newArc.ScaleToRadius(newRadius);		//整体缩放
// 		newArc.CutArc(m_ArcWallData.OuterArc.ptStart, m_ArcWallData.InnerArc.ptEnd);
// 		newArc.CutArc(m_ArcWallData.OuterArc.ptEnd, m_ArcWallData.InnerArc.ptStart);

		vector<CVector3D> vecEndNormal(2);
		if (0 == GetMainRebars().at(i).rebarDir)	//水平弧形钢筋
		{
			newArc.Shorten(dSideCover, false);		//起点缩短
			newArc.Shorten(dSideCover, true);		//终点缩短
			newArc.OffsetByAxisZ(diameter * 0.5 + dSideCover/* + diameterTie*/);
			PopSetIds().push_back(0);
			tag = MakeRebars_Arc(PopSetIds().back(), strRebarSize, newArc, spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();
		}
		else
		{
			newArc.Shorten(dSideCover + diameter * 0.5, false);		//起点缩短
			newArc.Shorten(dSideCover + diameter * 0.5, true);		//终点缩短
			newArc.OffsetByAxisZ(dSideCover/* + diameterTie*/);
			double dLen = m_height - (dSideCover/* + diameterTie*/) * 2;
			PopSetIds().push_back(0);
			tag = MakeRebars_Line(PopSetIds().back(), strRebarSize, newArc, dLen, spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType, DataExchange, vecEndType, vecEndNormal, modelRef);
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();
		}
	}

	DrawPreviewLines();

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

RebarSetTag* CamberedSurfaceRebarAssemblyEx::MakeRebars_Line
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	ArcSegment			arcSeg,
	double              dLen,
	double              spacing,
	double              startOffset,
	double              endOffset,
	int					level,
	int					grade,
	int					DataExchange,
	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
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
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double adjustedXLen, adjustedSpacing;

	double sideCov = GetConcrete().sideCover * uor_per_mm;
	int numRebar = 0;
	adjustedXLen = arcSeg.dLen - sideCov - diameter - startOffset - endOffset;

	numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);

	double xPos = startOffset;
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

	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);

	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };
	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);

	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve>     rebarCurve;
		//		RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
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

		RebarElementP rebarElement = NULL;
		if (!g_FacePreviewButtonsDown)
		{
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
				Stype = "front";
			}
			else if (DataExchange == 1)
			{
				Stype = "midden";
			}
			else
			{
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


RebarSetTag * CamberedSurfaceRebarAssemblyEx::MakeRebars_Arc(ElementId & rebarSetId, BrStringCR sizeKey, ArcSegment arcSeg, double spacing, double startOffset, double endOffset, int level, int grade, int DataExchange, vector<PIT::EndType> const & endType, vector<CVector3D> const & vecEndNormal, DgnModelRefP modelRef)
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
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double adjustedXLen, adjustedSpacing;

	// 	BrString strTieRebarSize = GetTieRebarInfo().rebarSize;
	// 	if (strTieRebarSize.Find(L"mm") != string::npos)
	// 		strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm
	// 	double diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//拉筋直径

	double sideCov = GetConcrete().sideCover * uor_per_mm;
	int numRebar = 0;
	adjustedXLen = m_height - sideCov * 2 - diameter /*- diameterTie*2*/ - startOffset - endOffset;
	numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);

	double xPos = startOffset;
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
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };
	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);

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

	//	vector<DSegment3d> vecStartEnd;
	for (int j = 0; j < (int)rebarCurvesNum.size(); ++j)
	{
		PITRebarCurve rebarCurve = rebarCurvesNum[j];

		RebarElementP rebarElement = NULL;
		if (!g_FacePreviewButtonsDown)//预览标志
		{
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
				Stype = "front";
			}
			else if (DataExchange == 1)
			{
				Stype = "midden";
			}
			else
			{
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

void CamberedSurfaceRebarAssemblyEx::CalculateUseHoles(DgnModelRefP modelRef)
{
	m_useHoleehs.clear();

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetConcrete().sideCover*uor_per_mm;

	if (GetConcrete().isHandleHole)//计算需要处理的孔洞
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
			DPoint3d ptcenter = m_ArcSeg.ptCenter;
			DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());

			ptcenter.z = ptele.z;
			CVector3D yVec = ptcenter - ptele;
			yVec.Normalize();

			CVector3D  xVec = CVector3D::kZaxis.CrossProduct(yVec);

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

			if (isNeed)
			{
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*m_Holeehs.at(j));
				PlusSideCover(*m_Holeehs.at(j), dSideCover, trans);
				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}
}


