#include "_ustation.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "CFacesRebarDlgEx.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "MultiPlaneRebarAssemblyEx.h"
#include <CPointTool.h>
#include "PITMSCECommon.h"
#include "XmlHelper.h"
#include "SelectRebarTool.h"

using namespace PIT;
extern bool g_FacePreviewButtonsDown;

//多平面联合配筋--begin
MultiPlaneRebarAssemblyEx::MultiPlaneRebarAssemblyEx(ElementId id, DgnModelRefP modelRef) : FacesRebarAssemblyEx(
	id, modelRef)
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
	if (m_UseXOYDir) //如果是板平面，以面的范围面作为基准面
	{
		DPoint3d minP, maxP;
		mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
		DPoint3d PTS[4];
		PTS[0] = PTS[1] = minP;
		PTS[1].x = maxP.x;
		PTS[2] = PTS[1];
		PTS[2].y = maxP.y;
		PTS[3] = PTS[2];
		PTS[3].x = minP.x;
		ShapeHandler::CreateShapeElement(tmpeeh, NULL, PTS, 4, true, *ACTIVEMODEL);
	}
	else
	{
		tmpeeh.Duplicate(eeh);
	}

	ElementAgenda agenda; //存放打散之后的元素
	DropGeometryPtr pDropGeometry = DropGeometry::Create(); //创建一个DropGeometry实例来设置几何选项
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

	class lineSort
	{
	public:
		bool operator()(const LineSegment& line1, const LineSegment& line2) const
		{
			return line1.GetLength() > line2.GetLength();
		}
	};
	std::set<LineSegment, lineSort> lineSegs;
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
	CVector3D xVec(maxLine.GetLineVec());
	CVector3D yVec = CVector3D(PopfaceNormal()).CrossProduct(xVec);
	BeMatrix matrix = CMatrix3D::Ucs(maxLine.GetLineStartPoint(), xVec, yVec, false); //方向为X轴，水平垂直方向为Y轴
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

	if (/*stLineSeg.LineSeg1.GetLineVec().IsParallelTo(DVec3d::From(0,0,1))*/ abs(
			stLineSeg.LineSeg1.GetLineVec().DotProduct(DVec3d::From(0, 0, 1))) > 0.9
		|| (abs(stLineSeg.LineSeg1.GetLineVec().DotProduct(DVec3d::From(0, 1, 0))) > 0.9 && abs(
			stLineSeg.LineSeg2.GetLineVec().DotProduct(DVec3d::From(1, 0, 0))) > 0.9))
	{
		swap(stLineSeg.LineSeg1, stLineSeg.LineSeg2);
	}
	else if (COMPARE_VALUES_EPS(stLineSeg.LineSeg1.GetLineStartPoint().x, stLineSeg.LineSeg1.GetLineEndPoint().x,
	                            EPS) == 0
		&& COMPARE_VALUES_EPS(stLineSeg.LineSeg1.GetLineStartPoint().y, stLineSeg.LineSeg1.GetLineEndPoint().y,
		                      EPS) != 0
		&& stLineSeg.LineSeg2.GetLineVec().IsParallelTo(DVec3d::From(1, 0, 0)))
	{
		//Added by chenxuan 面配筋横向竖向方向不对
		swap(stLineSeg.LineSeg1, stLineSeg.LineSeg2);
	}

	/*if (m_vecLineSeg.size() > 0)
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
	}*/

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
	StandardStartAndEnd(stLineSeg.LineSeg1, stLineSeg.LineSeg2);
	StandardStartAndEnd(stLineSeg.LineSeg2, stLineSeg.LineSeg1);
	// end确保向量是从绝对坐标系最低往最高
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
	double dSideCover = GetConcrete().sideCover * uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

	if (GetConcrete().isHandleHole) //计算需要处理的孔洞
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
	DPoint3d ptFir[2] = {DPoint3d::FromZero(), DPoint3d::FromZero()};
	ptFir[0] = m_vecLineSeg.at(0).LineSeg1.GetLineStartPoint();
	ptFir[1] = m_vecLineSeg.at(0).LineSeg1.GetLineEndPoint();

	for (int nIndex = 0; nIndex < 2; nIndex++)
	{
		bool bFlag = true;
		DPoint3d ptPreEnd = ptFir[nIndex]; // 前一段的终点
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
void MultiPlaneRebarAssemblyEx::SortLineSeg(vector<LineSeg>& m_vecLineSeg, double uor_per_mm)
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

bool MultiPlaneRebarAssemblyEx::JudgeMergeType(PIT::LineSegment& LineSeg1, PIT::LineSegment& LineSeg2,
                                               double uor_per_mm)
{
	DPoint3d ptStart = LineSeg1.GetLineEndPoint();
	DPoint3d ptEnd = LineSeg2.GetLineStartPoint();

	if (fabs(ptStart.x - ptEnd.x) < uor_per_mm && fabs(ptStart.y - ptEnd.y) < uor_per_mm && fabs(ptStart.z - ptEnd.z) <
		uor_per_mm)
	{
		return true;
	}
	return false;
}

bool MultiPlaneRebarAssemblyEx::makeRebarCurve(vector<PIT::PITRebarCurve>& rebars,
                                               const PIT::PITRebarEndTypes& endTypes, double endTypeStartOffset,
                                               double endTypeEndOffset)
{
	DPoint3d startPt = endTypes.beg.GetptOrgin();
	DPoint3d endPt = endTypes.end.GetptOrgin();
	DPoint3d ptStartBack = endTypes.beg.GetptOrgin();
	DPoint3d ptEndBack = endTypes.end.GetptOrgin();
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
		if (SolidUtil::Convert::ElementToBody(ptarget, m_face, true, true, true) == SUCCESS &&
			SolidUtil::Modify::ThickenSheet(ptarget, 5000.0 * uor_per_mm, 5000 * uor_per_mm) == SUCCESS &&
			SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL) == SUCCESS)
		{
			DRange3d range;
			mdlElmdscr_computeRange(&range.low, &range.high, eehSolid.GetElementDescrP(), NULL);
			DVec3d tmpVec = endPt - startPt;
			tmpVec.ScaleToLength(5000 * UOR_PER_MilliMeter);
			endPt += tmpVec;
			tmpVec.Negate();
			startPt += tmpVec;
			GetIntersectPointsWithOldElm(tmpptsTmp, &eehSolid, startPt, endPt, dSideCover);
			if (tmpptsTmp.size() > 1)
			{
				// 存在交点为两个以上的情况
				GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, startPt, endPt, &eehSolid, dSideCover);
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

		DVec3d vecTmp;
		vecTmp.DifferenceOf(startPt, endPt);
		vecTmp.Normalize();
		// 遍历 map_pts 生成钢筋段
		for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
		{
			PITRebarEndTypes endTypesTmp = endTypes;
			PITRebarCurve rebar;
			RebarVertexP vex;
			DPoint3d ptStart(itr->second);
			// 钢筋起点端部偏移
			if (ptStart.IsEqual(map_pts.begin()->second) && i == 0)
				ptStart.SumOf(ptStart, vecTmp, endTypeStartOffset);
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(ptStart);
			vex->SetType(RebarVertex::kStart);
			endTypesTmp.beg.SetptOrgin(ptStart);

			auto itrplus = ++itr;
			if (itrplus == map_pts.end())
			{
				break;
			}

			vecTmp.Negate();
			DPoint3d ptEnd(itrplus->second);
			// 钢筋终点端部偏移
			if (ptEnd.IsEqual((--map_pts.end())->second) && i == vecPtRebars.size() - 1)
				ptEnd.SumOf(ptEnd, vecTmp, endTypeEndOffset);
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(ptEnd);
			vex->SetType(RebarVertex::kEnd);
			endTypesTmp.end.SetptOrgin(ptEnd);

			// EditElementHandle eeh;
			// LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptStart, ptEnd), true, *ACTIVEMODEL);
			// eeh.AddToModel();
			// 生成钢筋段并添加到列表
			rebar.EvaluateEndTypes(endTypesTmp);
			rebars.push_back(rebar);
		}
	}
	return true;
}

RebarSetTag* MultiPlaneRebarAssemblyEx::MakeRebars
(
	ElementId& rebarSetId,
	LineSegment rebarLine,
	LineSegment vec,
	int dir,
	BrStringCR sizeKey,
	double spacing,
	double startOffset,
	double endOffset,
	int level,
	int grade,
	int DataExchange,
	vector<PIT::EndType> const& vecEndType, //存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
	DgnModelRefP modelRef
)
{
	// 检查端部类型和法线向量是否匹配
	if (vecEndType.size() != vecEndNormal.size() || vecEndType.size() == 0)
	{
		return nullptr;
	}

	// 获取或创建钢筋集
	RebarSetP rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (!rebarSet)
	{
		return nullptr;
	}

	// 设置钢筋集属性
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	// 处理端部类型
	RebarEndType endTypeStart, endTypeEnd;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startBendRadius = 0, endBendRadius = 0;
	double startBendLen = 0, endBendLen = 0;
	double begStraightAnchorLen = 0, endStraightAnchorLen = 0;
	auto ProcessEndType = [](RebarEndType& endType, PIT::EndType const& srcEndType, BrStringCR sizeKey,
	                          DgnModelRefP modelRef, double& bendRadius, double& bendLen, double& straightAnchorLen)
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
		case 8: //用户自定义
			endType.SetType(RebarEndType::kCustom);
			break;
		default:
			break;
		}
	};
	ProcessEndType(endTypeStart, vecEndType[0], sizeKey, modelRef, startBendRadius, startBendLen, begStraightAnchorLen);
	ProcessEndType(endTypeEnd, vecEndType[1], sizeKey, modelRef, endBendRadius, endBendLen, endStraightAnchorLen);

	// 计算钢筋排列参数
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double adjustedXLen, adjustedSpacing;
	double sideCov = GetConcrete().sideCover * uor_per_mm;
	double updownSideCover = 50 * uor_per_mm;
	rebarLine.PerpendicularOffset(startOffset, vec.GetLineVec());
	adjustedXLen = vec.GetLength() - updownSideCover * 2 - startOffset - endOffset;
	int numRebar = static_cast<int>(floor(adjustedXLen / spacing + 0.85)) + 1;
	adjustedSpacing = spacing;
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	adjustedSpacing = spacing;
	//	rebarLine.PerpendicularOffset(startOffset, vec.GetLineVec());
	//	rebarLine.PerpendicularOffset(sideCov, vec.GetLineVec());
	int j = 0;
	double endTypeStartOffset = vecEndType[0].offset * uor_per_mm;
	double endTypeEndOffset = vecEndType[1].offset * uor_per_mm;
	if (vecEndType[0].endType != 0 && vecEndType[0].endType != 7) //端部弯曲时额外偏移钢筋半径
		endTypeStartOffset -= diameter * 0.5;
	if (vecEndType[1].endType != 0 && vecEndType[1].endType != 7) //端部弯曲时额外偏移钢筋半径
		endTypeEndOffset -= diameter * 0.5;

	// 	EditElementHandle eeh;
	// 	LineHandler::CreateLineElement(eeh, NULL, rebarLine.GetLineSeg(), true, *ACTIVEMODEL);
	// 	eeh.AddToModel();

	// 设置端部类型参数
	PITRebarEndType start, end;
	start.SetType(static_cast<PITRebarEndType::Type>(endTypeStart.GetType()));
	start.Setangle(vecEndType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	start.SetbendLen(startBendLen);
	start.SetbendRadius(startBendRadius);
	start.SetendNormal(vecEndNormal[0]);
	//rebarLine.Shorten(sideCov + endTypeStartOffset,true);
	//rebarLine.Shorten(/*sideCov + */endTypeStartOffset, true);
	start.SetptOrgin(rebarLine.GetLineStartPoint());
	//rebarLine.Shorten(sideCov + endTypEendOffset, false);
	//rebarLine.Shorten(/*sideCov + */endTypeEndOffset, false);

	end.SetType(static_cast<PITRebarEndType::Type>(endTypeEnd.GetType()));
	end.Setangle(vecEndType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	end.SetbendLen(endBendLen);
	end.SetbendRadius(endBendRadius);
	end.SetendNormal(vecEndNormal[1]);
	end.SetptOrgin(rebarLine.GetLineEndPoint());

	PITRebarEndTypes endTypes = {start, end};
	//	rebarLine.PerpendicularOffset(sideCov+diameter*0.5, vec.GetLineVec());
	vector<PITRebarCurve> rebarCurvesNum;
	for (int i = 0; i < numRebar; i++)
	{
		vector<PITRebarCurve> rebarCurves;
		if (i == numRebar - 1) //如果是最后一根，要判断当前还有多少距离,符合距离要求就要再布置一根
		{
			double sDis = adjustedXLen - (numRebar - 2) * adjustedSpacing;
			DVec3d dvec = vec.GetLineVec();
			if (sDis > spacing)
			{
				rebarLine.PerpendicularOffset((sDis - spacing), dvec);
			}
			else
			{
				dvec.Negate();
				rebarLine.PerpendicularOffset((spacing - sDis), dvec);
			}
		}
		endTypes.beg.SetptOrgin(rebarLine.GetLineStartPoint());
		endTypes.end.SetptOrgin(rebarLine.GetLineEndPoint());

		makeRebarCurve(rebarCurves, endTypes, endTypeStartOffset, endTypeEndOffset);

		//		xPos += adjustedSpacing
		DVec3d offsetVec = vec.GetLineVec();
		rebarLine.PerpendicularOffset(adjustedSpacing, offsetVec);

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	string str(sizeKey);
	char ccolar[20] = {0};
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
		if (!g_FacePreviewButtonsDown) //预览标志，预览状态下不要生成钢筋
		{
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(false);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes endTypes = {endTypeStart, endTypeEnd};
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
	ret = rebarSet->FinishUpdate(setdata, modelRef); //返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);

	return tag;
}

bool MultiPlaneRebarAssemblyEx::MakeRebars(DgnModelRefP modelRef)
{
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

	for (auto lineSeg : m_vecLineSeg)
	{
		// 分配每一面的数据
		m_LineSeg1 = lineSeg.LineSeg1;
		m_LineSeg2 = lineSeg.LineSeg2;
		m_face = m_vecFace[lineSeg.iIndex];
		this->SetfaceNormal(m_vecFaceNormal[lineSeg.iIndex]);
		offset = dPositiveCover;

		// 面配筋竖直面配置方式优化，调换层次数据
		if (abs(m_LineSeg2.GetLineVec().DotProduct(DVec3d::From(0, 0, -1))) > 0.9)
		{
			bool isParallelToY = m_LineSeg1.GetLineVec().IsParallelTo(DVec3d::UnitY());
			ReverseMainRebars(isParallelToY);
		}

		// 遍历钢筋层级，生成每层钢筋并保存相关数据
		for (int i = 0; i < iRebarLevelNum; ++i)
		{
			// 初始化当前层的数据
			PopSetIds().push_back(0);
			std::vector<PIT::EndType> vecEndType;
			if (GetvecEndTypes().empty()) // 没有设置端部样式时，使用默认值
			{
				EndType endType;
				memset(&endType, 0, sizeof(endType));
				vecEndType = { {0, 0, 0}, endType };
			}
			else
			{
				vecEndType = GetvecEndTypes().at(i);
			}

			// 获取当前层钢筋的基本参数
			BrString strRebarSize(GetMainRebars().at(i).rebarSize);
			double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
			double spacing = GetMainRebars().at(i).spacing * uor_per_mm;
			double startOffset = GetMainRebars().at(i).startOffset * uor_per_mm;
			double endOffset = GetMainRebars().at(i).endOffset * uor_per_mm;
			int rebarDir = GetMainRebars().at(i).rebarDir;
			double levelSpacing = GetMainRebars().at(i).levelSpace * uor_per_mm;
			int dataExchange = GetMainRebars().at(i).datachange;

			// 清空当前层的点集
			m_vecRebarPtsLayer.clear();
			m_vecTwinRebarPtsLayer.clear();
			LineSegment lineSeg1 = m_LineSeg1;
			LineSegment lineSeg2 = m_LineSeg2;
			std::vector<CVector3D> vecEndNormal(2);
			CVector3D endNormal;

			// 计算端部弯钩方向
			if (!GetvecEndTypes().empty())
			{
				for (size_t k = 0; k < vecEndNormal.size(); ++k)
				{
					double rotateAngle = vecEndType.at(k).rotateAngle;
					endNormal = GetfaceNormal();
					CVector3D rebarVec = (rebarDir == 1) ? m_LineSeg2.GetLineVec() : m_LineSeg1.GetLineVec();
					endNormal.Rotate(rotateAngle * PI / 180, rebarVec);
					vecEndNormal[k] = endNormal;
				}
			}

			// 计算当前层钢筋的偏移量
			offset += levelSpacing;
			if (i == 0) // 首层偏移当前钢筋半径
			{
				offset += diameter * 0.5;
			}
			else // 其他层需要考虑上下层钢筋的直径
			{
				double diameterPre = RebarCode::GetBarDiameter(GetMainRebars().at(i - 1).rebarSize, modelRef);
				offset += diameterPre * 0.5; // 偏移上一层钢筋的半径
				offset += diameter * 0.5;    // 偏移当前层钢筋的半径
			}

			// 根据钢筋方向调整线段位置并生成钢筋
			RebarSetTag* tag = nullptr;
			if (rebarDir == 1) // 纵向钢筋
			{
				DVec3d vec = m_LineSeg1.GetLineVec();
				lineSeg2.PerpendicularOffset(offset, GetfaceNormal());
				lineSeg2.PerpendicularOffset(updownSideCover, vec);
				tag = MakeRebars(PopSetIds().at(i + iRebarLevelNum * lineSeg.iIndex), lineSeg2, m_LineSeg1, rebarDir, strRebarSize, 
					spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType,
					dataExchange, vecEndType, vecEndNormal, modelRef);
			}
			else // 横向钢筋
			{
				DVec3d vec = m_LineSeg2.GetLineVec();
				lineSeg1.PerpendicularOffset(offset, GetfaceNormal());
				lineSeg1.PerpendicularOffset(updownSideCover, vec);
				tag = MakeRebars(PopSetIds().at(i + iRebarLevelNum * lineSeg.iIndex), lineSeg1, m_LineSeg2, rebarDir, strRebarSize,
					spacing, startOffset, endOffset, GetMainRebars().at(i).rebarLevel, GetMainRebars().at(i).rebarType,
					dataExchange, vecEndType, vecEndNormal, modelRef);
			}

			// 保存生成的钢筋标签
			if (tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}
			vecEndType.clear();

			// 删除当前层的临时钢筋点数据
			auto itrTemp = mapRebarPoint.find(dataExchange);
			if (itrTemp != mapRebarPoint.end())
			{
				mapRebarPoint.erase(itrTemp);
			}

			// 保存钢筋点数据到全局容器
			if (m_vecRebarPtsLayer.size() > 1)
			{
				for (size_t m = 0; m < m_vecRebarPtsLayer.size() - 1; m += 2)
				{
					RebarPoint rbPt;
					rbPt.Layer = GetMainRebars().at(i).rebarLevel;
					rbPt.vecDir = rebarDir;
					rbPt.ptstr = m_vecRebarPtsLayer.at(m);
					rbPt.ptend = m_vecRebarPtsLayer.at(m + 1);
					rbPt.DataExchange = dataExchange;
					g_vecRebarPtsNoHole.push_back(rbPt);
				}
			}

			// 保存双排钢筋点数据到全局容器
			if (m_vecTwinRebarPtsLayer.size() > 1)
			{
				for (size_t m = 0; m < m_vecTwinRebarPtsLayer.size() - 1; m += 2)
				{
					RebarPoint rbPt;
					rbPt.Layer = GetMainRebars().at(i).rebarLevel;
					rbPt.vecDir = rebarDir;
					rbPt.ptstr = m_vecTwinRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTwinRebarPtsLayer.at(m + 1);
					rbPt.DataExchange = dataExchange;
					g_vecTwinRebarPtsNoHole.push_back(rbPt);
				}
			}
		}

		// 恢复层次数据，不影响其它面层次数据
		if (abs(m_LineSeg2.GetLineVec().DotProduct(DVec3d::From(0, 0, -1))) > 0.9)
		{
			bool isParallelToY = m_LineSeg1.GetLineVec().IsParallelTo(DVec3d::UnitY());
			ReverseMainRebars(isParallelToY);
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

long MultiPlaneRebarAssemblyEx::GetStreamMap(BeStreamMap& map, int typeof /* = 0 */, int versionof /* = -1 */)
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
