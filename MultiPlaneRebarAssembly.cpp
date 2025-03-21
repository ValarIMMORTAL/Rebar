#include "_ustation.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "CFacesRebarDlg.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "MultiPlaneRebarAssembly.h"
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
#include "PlaneRebarAssembly.h"

#define  Waterstops_Width (60*UOR_PER_MilliMeter)

using namespace Gallery;

using namespace PIT;
extern bool FacePreviewButtonsDown;

//多平面联合配筋--begin
MultiPlaneRebarAssembly::MultiPlaneRebarAssembly(ElementId id, DgnModelRefP modelRef) :FacesRebarAssembly(id, modelRef)
{
	m_vecRebarStartEnd.clear();
	m_vecLineSeg.clear();
	m_vecFace.clear();
	m_vecFaceNormal.clear();
}

MultiPlaneRebarAssembly::~MultiPlaneRebarAssembly()
{
}

bool MultiPlaneRebarAssembly::AnalyzingFaceGeometricData(EditElementHandleR eeh)
{
	DVec3d faceNormal = PopfaceNormal();
	if (abs(faceNormal.DotProduct(DVec3d::From(0, 0, 1))) > 0.9)
	{
		m_UseXOYDir = true;
	}
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
		bool operator()(const LineSegment &line1, const LineSegment &line2) {
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
	tmpeeh.AddToModel();
	//计算平面的最大范围
	DPoint3d ptMin, ptMax;
	mdlElmdscr_computeRange(&ptMin, &ptMax, tmpeeh.GetElementDescrCP(), NULL);

	Dpoint3d pt[4];
	int num = 0;
	mdlLinear_extract(pt, &num, tmpeeh.GetElementP(), ACTIVEMODEL);

	vector<Dpoint3d> vecPtDown;
	vector<Dpoint3d> vecPtUp;
	for (int i = 0; i < 4; i++)
	{
		if (COMPARE_VALUES_EPS(pt[i].y, ptMax.y, 10) == 0)
		{
			vecPtUp.push_back(pt[i]);
		}
		else if (COMPARE_VALUES_EPS(pt[i].y, ptMin.y, 10) == 0)
		{
			vecPtDown.push_back(pt[i]);
		}
	}
	DSegment3d lineSeg1;
	DSegment3d lineSeg2;
	if (vecPtDown.size() == 2 && vecPtUp.size() == 2)
	{
		lineSeg1 = vecPtDown[0].x < vecPtDown[1].x ? DSegment3d::From(vecPtDown[0], vecPtDown[1]) : DSegment3d::From(vecPtDown[1], vecPtDown[0]);
		Dpoint3d downpt = vecPtDown[0].x < vecPtDown[1].x ? vecPtDown[0] : vecPtDown[1];
		Dpoint3d uppt = vecPtUp[0].x < vecPtUp[1].x ? vecPtUp[0] : vecPtUp[1];
		lineSeg2 = DSegment3d::From(downpt, uppt);
	}
	else
	{
		DPoint3d ptLineSeg1End = ptMax;
		DPoint3d ptLineSeg2End = ptMin;

		ptLineSeg1End.y = ptMin.y;
		ptLineSeg2End.y = ptMax.y;

		lineSeg1 = DSegment3d::From(ptMin, ptLineSeg1End);
		lineSeg2 = DSegment3d::From(ptMin, ptLineSeg2End);
	}



	EditElementHandle ehLine1, ehLine2;
	LineHandler::CreateLineElement(ehLine1, NULL, lineSeg1, true, *ACTIVEMODEL);
	LineHandler::CreateLineElement(ehLine2, NULL, lineSeg2, true, *ACTIVEMODEL);

	trans.InverseOf(trans);
	ehLine1.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(ehLine1, TransformInfo(trans));
	ehLine2.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(ehLine2, TransformInfo(trans));

	DPoint3d ptLine1[2];
	DPoint3d ptLine2[2];
	mdlLinear_extract(ptLine1, NULL, ehLine1.GetElementCP(), ACTIVEMODEL);
	mdlLinear_extract(ptLine2, NULL, ehLine2.GetElementCP(), ACTIVEMODEL);

	stLineSeg.LineSeg1.SetLineSeg(DSegment3d::From(ptLine1[0], ptLine1[1]));
	stLineSeg.LineSeg2.SetLineSeg(DSegment3d::From(ptLine2[0], ptLine2[1]));


	if (abs(stLineSeg.LineSeg1.GetLineVec().DotProduct(DVec3d::From(0, 0, 1))) > 0.9
		|| (abs(stLineSeg.LineSeg1.GetLineVec().DotProduct(DVec3d::From(0, 1, 0))) > 0.9 && abs(stLineSeg.LineSeg2.GetLineVec().DotProduct(DVec3d::From(0, 0, 1))) < 0.9))
	{
		LineSegment segTmp = stLineSeg.LineSeg2;
		stLineSeg.LineSeg2 = stLineSeg.LineSeg1;
		stLineSeg.LineSeg1 = segTmp;
	}

	//if (m_vecLineSeg.size() > 0)
	//{
	//	PIT::LineSegment  lineSeg;
	//	lineSeg = m_vecLineSeg.at(m_vecLineSeg.size() - 1).LineSeg1;

	//	DPoint3d pt1[2];
	//	pt1[0] = lineSeg.GetLineStartPoint();
	//	pt1[1] = lineSeg.GetLineEndPoint();
	//	DPoint3d pt1Other[2];
	//	pt1Other[0] = m_vecLineSeg.at(m_vecLineSeg.size() - 1).LineSeg2.GetLineStartPoint();
	//	pt1Other[1] = m_vecLineSeg.at(m_vecLineSeg.size() - 1).LineSeg2.GetLineEndPoint();

	//	DPoint3d pt2[2];
	//	pt2[0] = stLineSeg.LineSeg1.GetLineStartPoint();
	//	pt2[1] = stLineSeg.LineSeg1.GetLineEndPoint();
	//	DPoint3d pt2Other[2];
	//	pt2Other[0] = stLineSeg.LineSeg2.GetLineStartPoint();
	//	pt2Other[1] = stLineSeg.LineSeg2.GetLineEndPoint();


	//	DVec3d vec = pt1[1] - pt1[0];
	//	vec.Normalize();
	//	DVec3d vecother = m_vecLineSeg.at(m_vecLineSeg.size() - 1).LineSeg2.GetLineVec();

	//	DVec3d vec2 = pt2[1] - pt2[0];
	//	vec2.Normalize();

	//
	//	if (abs(vecother.DotProduct(DVec3d::From(0,0,1))) < 0.9)//是板
	//	{
	//		Dpoint3d ptStart = pt2[0], ptEnd = pt2[1];

	//		if (abs(vec.x - 0.00) < EPS)
	//		{
	//			ptStart.x = pt1[0].x;
	//			ptEnd.x = pt1[1].x;
	//		}
	//		if (abs(vec.y - 0.00) < EPS)
	//		{
	//			ptStart.y = pt1[0].y;
	//			ptEnd.y = pt1[1].y;
	//		}
	//		if (abs(vec.z - 0.00) < EPS)
	//		{
	//			ptStart.z = pt1[0].z;
	//			ptEnd.z = pt1[1].z;
	//		}
	//		double distance00 = ptStart.Distance(pt1[0]);
	//		double distance01 = ptStart.Distance(pt1[1]);
	//		double distance10 = ptEnd.Distance(pt1[0]);
	//		double distance11 = ptEnd.Distance(pt1[1]);
	//		
	//		if ((distance00 < distance01 && distance10 < distance11)
	//			|| (distance01  < distance00 &&distance11  < distance10))
	//		{
	//			if (abs(vec.x - 0.00) < EPS)
	//			{
	//				pt2[0].x = pt1[0].x;
	//				pt2[1].x = pt1[1].x;
	//			}

	//			if (abs(vec.y - 0.00) < EPS)
	//			{
	//				pt2[0].y = pt1[0].y;
	//				pt2[1].y = pt1[1].y;
	//			}

	//			if (abs(vec.z - 0.00) < EPS)
	//			{
	//				pt2[0].z = pt1[0].z;
	//				pt2[1].z = pt1[1].z;
	//			}
	//			stLineSeg.LineSeg1.SetLineStartPoint(pt2[0]);
	//			stLineSeg.LineSeg1.SetLineEndPoint(pt2[1]);
	//		}
	//		else 
	//		{
	//			if (abs(vecother.x - 0.00) < EPS)
	//			{
	//				pt2Other[0].x = pt1Other[0].x;
	//				pt2Other[1].x = pt1Other[1].x;
	//			}

	//			if (abs(vecother.y - 0.00) < EPS)
	//			{
	//				pt2Other[0].y = pt1Other[0].y;
	//				pt2Other[1].y = pt1Other[1].y;
	//			}

	//			if (abs(vecother.z - 0.00) < EPS)
	//			{
	//				pt2Other[0].z = pt1Other[0].z;
	//				pt2Other[1].z = pt1Other[1].z;
	//			}
	//			stLineSeg.LineSeg2.SetLineStartPoint(pt2Other[0]);
	//			stLineSeg.LineSeg2.SetLineEndPoint(pt2Other[1]);
	//		}
	//		
	//
	//		/*if (abs(m_vecFaceNormal[m_vecFaceNormal.size() - 1].DotProduct(DVec3d::From(0, 0, 1))) > 0.9 || 
	//			abs(faceNormal.DotProduct(DVec3d::From(0, 0, 1))) > 0.9)
	//		{
	//			if (abs(faceNormal.DotProduct(DVec3d::From(0, 0, 1))) > 0.9)
	//			{
	//				if (abs(vec2.DotProduct(vec)) > 0.9)
	//				{
	//					PIT::LineSegment line1 = stLineSeg.LineSeg1;
	//					stLineSeg.LineSeg1 = stLineSeg.LineSeg2;
	//					stLineSeg.LineSeg2 = line1;
	//				}
	//			}
	//			else 
	//			{
	//				if (abs(vec2.DotProduct(vec)) > 0.9)
	//				{
	//					PIT::LineSegment  lineSeg1 = m_vecLineSeg.at(m_vecLineSeg.size() - 1).LineSeg1;
	//					m_vecLineSeg.at(m_vecLineSeg.size() - 1).LineSeg1 = m_vecLineSeg.at(m_vecLineSeg.size() - 1).LineSeg2;
	//					m_vecLineSeg.at(m_vecLineSeg.size() - 1).LineSeg2 = lineSeg1;
	//				}
	//			}
	//		}
	//		else 
	//		{
	//			pt2[0].z = pt1[0].z;
	//			pt2[1].z = pt1[1].z;
	//		}*/
	//		
	//	}
	//	else 
	//	{
	//		if (abs(vec.x - 0.00) < EPS)
	//		{
	//			pt2[0].x = pt1[0].x;
	//			pt2[1].x = pt1[1].x;
	//		}

	//		if (abs(vec.y - 0.00) < EPS)
	//		{
	//			pt2[0].y = pt1[0].y;
	//			pt2[1].y = pt1[1].y;
	//		}

	//		if (abs(vec.z - 0.00) < EPS)
	//		{
	//			pt2[0].z = pt1[0].z;
	//			pt2[1].z = pt1[1].z;
	//		}
	//		stLineSeg.LineSeg1.SetLineStartPoint(pt2[0]);
	//		stLineSeg.LineSeg1.SetLineEndPoint(pt2[1]);
	//	}

	//}

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
	m_vecFaceNormal.push_back(faceNormal);
	PopvecFrontPts().push_back(stLineSeg.LineSeg1.GetLineStartPoint());
	PopvecFrontPts().push_back(stLineSeg.LineSeg1.GetLineEndPoint());
	return true;
}

void MultiPlaneRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
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

void MultiPlaneRebarAssembly::CmdSort(vector<DPoint3d>& vecPoint)
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
void MultiPlaneRebarAssembly::SortLineSegVec(vector<LineSeg>& m_vecLineSeg, double uor_per_mm)
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
void MultiPlaneRebarAssembly::SortLineSeg(vector<LineSeg>&	m_vecLineSeg, double uor_per_mm)
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
			//eehTmp.AddToModel();
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

bool MultiPlaneRebarAssembly::JudgeMergeType(PIT::LineSegment& LineSeg1, PIT::LineSegment& LineSeg2, double uor_per_mm)
{
	DPoint3d ptStart = LineSeg1.GetLineEndPoint();
	DPoint3d ptEnd = LineSeg2.GetLineStartPoint();

	if (fabs(ptStart.x - ptEnd.x) < Waterstops_Width/*uor_per_mm*/ && fabs(ptStart.y - ptEnd.y) < Waterstops_Width/*uor_per_mm*/ && fabs(ptStart.z - ptEnd.z) < Waterstops_Width/*uor_per_mm*/)
	{
		return true;
	}
	return false;
}

bool MultiPlaneRebarAssembly::makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, vector<DPoint3d>& rebarPoint, int iIndex, const PIT::PITRebarEndTypes& endTypes, double bendRadius)
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
		//pt[0].z += 800 * uor_per_mm;
		//pt[1].z -= 800 * uor_per_mm;

		allpts.push_back(pt[0]);
		allpts.push_back(pt[1]);

		RebarVertices  vers;
		//GetRebarVerticesFromPoints(vers, allpts, bendRadius);
		RebarHelper::GetRebarVerticesFromPoints(vers, allpts, bendRadius);


		PIT::PITRebarCurve rebar;
		rebar.SetVertices(vers);

		rebar.EvaluateEndTypes(endTypes);
		rebars.push_back(rebar);
		allpts.clear();
	}
	m_vecRebarStartEnd.push_back(vecPtRebars);
	return true;
}

bool MultiPlaneRebarAssembly::makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, vector<DPoint3d>& rebarPoint, vector<int>& vecIndex, const PIT::PITRebarEndTypes& endTypes, double bendRadius)
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
			//GetRebarVerticesFromPoints(vers, allpts, bendRadius);
			RebarHelper::GetRebarVerticesFromPoints(vers, allpts, bendRadius);


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
				//GetRebarVerticesFromPoints(vers, allpts, bendRadius);
				RebarHelper::GetRebarVerticesFromPoints(vers, allpts, bendRadius);
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
			//GetRebarVerticesFromPoints(vers, allpts, bendRadius);
			RebarHelper::GetRebarVerticesFromPoints(vers, allpts, bendRadius);
			PIT::PITRebarCurve rebar;
			rebar.SetVertices(vers);

			rebar.EvaluateEndTypes(endTypes);
			rebars.push_back(rebar);
			allpts.clear();
		}
	}

	//for (size_t i = 0; i < vecPtRebars.size(); i++)
	//{
	//	vecPtRebars.at(i).GetStartPoint(startPt);
	//	vecPtRebars.at(i).GetEndPoint(endPt);
	//	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, startPt, endPt, dSideCover);
	//	map<int, DPoint3d> map_pts;
	//	bool isStr = false;
	//	for (DPoint3d pt : tmppts)
	//	{
	//		if (ExtractFacesTool::IsPointInLine(pt, startPt, endPt, ACTIVEMODEL, isStr))
	//		{
	//			int dis = (int)startPt.Distance(pt);
	//			if (map_pts.find(dis) != map_pts.end())
	//			{
	//				dis = dis + 1;
	//			}
	//			map_pts[dis] = pt;
	//		}
	//	}
	//	if (map_pts.find(0) != map_pts.end())
	//	{
	//		map_pts[1] = startPt;
	//	}
	//	else
	//	{
	//		map_pts[0] = startPt;
	//	}
	//	int dis = (int)startPt.Distance(endPt);
	//	if (map_pts.find(dis) == map_pts.end())
	//	{
	//		map_pts[dis] = endPt;
	//	}
	//	else
	//	{
	//		dis = dis + 1;
	//		map_pts[dis] = endPt;
	//	}



	//	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	//	{
	//		PITRebarEndTypes endTypesTmp = endTypes;
	//		PITRebarCurve rebar;
	//		RebarVertexP vex;
	//		DPoint3d ptStart(itr->second);
	//		vex = &rebar.PopVertices().NewElement();
	//		vex->SetIP(itr->second);
	//		vex->SetType(RebarVertex::kStart);
	//		endTypesTmp.beg.SetptOrgin(itr->second);

	//		map<int, DPoint3d>::iterator itrplus = ++itr;
	//		if (itrplus == map_pts.end())
	//		{
	//			break;
	//		}

	//		endTypesTmp.end.SetptOrgin(itrplus->second);

	//		DPoint3d ptEnd(itrplus->second);
	//		vex = &rebar.PopVertices().NewElement();
	//		vex->SetIP(itrplus->second);
	//		vex->SetType(RebarVertex::kEnd);

	//		double dis1 = ptStart.Distance(ptStartBack);
	//		if (COMPARE_VALUES_EPS(dis1, 0, 1) != 0)
	//		{
	//			endTypesTmp.beg.SetType(PITRebarEndType::kNone);
	//		}
	//		double dis2 = ptEnd.Distance(ptEndBack);

	//		if (COMPARE_VALUES_EPS(dis2, 0, 1) != 0)
	//		{
	//			endTypesTmp.end.SetType(PITRebarEndType::kNone);
	//		}
	//		// 			EditElementHandle eeh;
	//		// 			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptStart, ptEnd), true, *ACTIVEMODEL);
	//		// 			eeh.AddToModel();
	//		rebar.EvaluateEndTypes(endTypesTmp);
	//		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
	//		rebars.push_back(rebar);
	return true;
}

RebarSetTag* MultiPlaneRebarAssembly::MakeRebars
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
	if (!isFlag)
	{
		totalAdjust = totalAdjust + sideCov + diameter * 0.5;//起点会便宜保护层，但不知道上面为什么不用
	}

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
			if (m_iMergeType == 0)
			{
				if (dir == 1)
				{
					DVec3d  vec1 = vecPoint[1] - vecPoint[0];
					DVec3d  vec2 = vecPoint[0] - vecPoint[1];
					if (m_wallfaceAssociateInfo.m_bUpIsStatr)
					{
						vec2.ScaleToLength(m_wallfaceAssociateInfo.m_dUpSlabThickness);
						vec1.ScaleToLength(m_wallfaceAssociateInfo.m_dbottomSlabThickness);
						vecPoint[0].Add(vec2);
						vecPoint[1].Add(vec1);
					}
					else
					{
						vec2.ScaleToLength(m_wallfaceAssociateInfo.m_dbottomSlabThickness);
						vec1.ScaleToLength(m_wallfaceAssociateInfo.m_dUpSlabThickness);
						vecPoint[0].Add(vec2);
						vecPoint[1].Add(vec1);
					}
				}
				endTypes.beg.SetptOrgin(vecPoint[0]);
				endTypes.end.SetptOrgin(vecPoint[1]);
			}

			makeRebarCurve(rebarCurves, vecPoint, iIndexFace, endTypes, bendRadius);
		}

		//		xPos += adjustedSpacing
		if (isFlag)
		{
			double totalTmp = vec.at(0).GetLength() - updownSideCover - diameter * 0.5 - startOffset;
			DVec3d offsetVec = vec.at(0).GetLineVec();

			for (int j = 0; j < (int)vecRebarLine.size(); j++)
			{
				if (i != numRebar - 2)
				{
					vecRebarLine[j].PerpendicularOffset(adjustedSpacing, offsetVec);
				}
				else
				{
					double leftDistance = totalTmp - totalAdjust;
					if (leftDistance > adjustedSpacing + updownSideCover + diameter * 0.5)
					{
						vecRebarLine[j].PerpendicularOffset(adjustedSpacing, offsetVec);
					}
					else
					{
						double lastMoveDistance = leftDistance - updownSideCover - diameter * 0.5;
						if (lastMoveDistance > 0)
						{
							vecRebarLine[j].PerpendicularOffset(lastMoveDistance, offsetVec);
						}
					}
				}

			}
			totalAdjust += adjustedSpacing;

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
				//加上中间缝大小
				if (j > 0)
				{
					double distance = vec[j].GetLineStartPoint().Distance(vec[j - 1].GetLineEndPoint());
					totalTmp += distance;
				}
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
			if (i == numRebar - 2)
			{
				/*double offersettemp = totalTmp - updownSideCover - diameter * 0.5 - startOffset - totalAdjust ;
				vecRebarLine[0].PerpendicularOffset(offersettemp, offsetVec);*/
				double leftDistance = totalTmp - totalAdjust;
				if (leftDistance > adjustedSpacing + updownSideCover + diameter * 0.5)
				{
					vecRebarLine[0].PerpendicularOffset(adjustedSpacing, offsetVec);
				}
				else
				{
					double lastMoveDistance = leftDistance - updownSideCover - diameter * 0.5;
					if (lastMoveDistance > 0)
					{
						vecRebarLine[0].PerpendicularOffset(lastMoveDistance, offsetVec);
					}
				}

			}
			else
			{
				vecRebarLine[0].PerpendicularOffset(adjustedSpacingTemp, offsetVec);
			}


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
		if (!FacePreviewButtonsDown)//预览状态下不生成钢筋
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

bool MultiPlaneRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	AnalyzeRebarDir();
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

	bool hasHorizFace = false;
	vector<int> vecHorizFaceIndex;

	for (size_t i = 0; i < m_vecFaceNormal.size(); i++)
	{
		if (abs(m_vecFaceNormal[i].DotProduct(DVec3d::From(0, 0, 1))) > 0.9)
		{
			hasHorizFace = true;
			//vecHorizFaceIndex.push_back(i);
			vecHorizFaceIndex.push_back(i);
			//break;
		}
	}

	vector<PIT::LineSegment> vecMergeSeg;  // 需要合并的线方向
	vector<PIT::LineSegment> vecSeg;	   // 不需要合并的线方向
	bool bFlag = true;    // true : 合并方向为横向钢筋  false ： 合并方向为竖向钢筋
	if (m_vecLineSeg.size() > 1)
	{
		if (hasHorizFace && vecHorizFaceIndex.size() != m_vecFaceNormal.size())
		{
			LineSeg horFacelineseg = m_vecLineSeg[vecHorizFaceIndex[0]];
			DVec3d vec = horFacelineseg.LineSeg1.GetLineVec();
			DVec3d vecAnother = horFacelineseg.LineSeg2.GetLineVec();
			for (size_t i = 0; i < m_vecLineSeg.size(); i++)
			{
				vector<int>::iterator result = find(vecHorizFaceIndex.begin(), vecHorizFaceIndex.end(), i);
				if (result == vecHorizFaceIndex.end())
				{
					DVec3d vec1 = m_vecLineSeg[i].LineSeg1.GetLineVec();
					DVec3d vec2 = m_vecLineSeg[i].LineSeg2.GetLineVec();
					if (abs(vec1.DotProduct(vecAnother)) > 0.9)
					{
						vecMergeSeg.push_back(m_vecLineSeg[i].LineSeg2);
						vecSeg.push_back(m_vecLineSeg[i].LineSeg1);
					}
					else
					{
						vecMergeSeg.push_back(m_vecLineSeg[i].LineSeg1);
						vecSeg.push_back(m_vecLineSeg[i].LineSeg2);
					}
				}
				else
				{
					vecMergeSeg.push_back(m_vecLineSeg[i].LineSeg1);
					vecSeg.push_back(m_vecLineSeg[i].LineSeg2);
				}
			}

			PIT::LineSegment me = vecMergeSeg[0];
			vecMergeSeg[0] = vecMergeSeg[1];
			vecMergeSeg[1] = me;


			DPoint3d start1 = vecMergeSeg[1].GetLineStartPoint();
			DPoint3d end1 = vecMergeSeg[1].GetLineEndPoint();
			if (COMPARE_VALUES_EPS(start1.y, end1.y, 10) == 0)
			{
				start1.y = vecMergeSeg[0].GetLineEndPoint().y;
				end1.y = vecMergeSeg[0].GetLineEndPoint().y;
				vecMergeSeg[1].SetLineStartPoint(start1);
				vecMergeSeg[1].SetLineEndPoint(end1);
			}


		}
		else
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
				//纵筋需要排序
				for (int k = 1; k < vecMergeSeg.size(); k++)
				{

					Dpoint3d ptstart = vecMergeSeg[k].GetLineStartPoint();
					Dpoint3d ptEnd = vecMergeSeg[k - 1].GetLineEndPoint();
					if (ptstart.Distance(ptEnd) > Waterstops_Width)
					{
						DVec3d vec = vecMergeSeg[k - 1].GetLineVec();
						vecMergeSeg[k - 1].SetLineEndPoint(vecMergeSeg[k - 1].GetLineStartPoint());
						vecMergeSeg[k - 1].SetLineStartPoint(ptEnd);

						vec.ScaleToLength(vecMergeSeg[k - 1].GetLength());
						Dpoint3d pt1 = vecSeg[k - 1].GetLineStartPoint();
						Dpoint3d pt2 = vecSeg[k - 1].GetLineEndPoint();
						pt1.Add(vec);
						pt2.Add(vec);
						vecSeg[k - 1].SetLineStartPoint(pt1);
						vecSeg[k - 1].SetLineEndPoint(pt2);
						if (k == vecMergeSeg.size() - 1)
						{
							DVec3d vec = vecMergeSeg[k].GetLineVec();
							vecMergeSeg[k].SetLineStartPoint(vecMergeSeg[k].GetLineEndPoint());
							vecMergeSeg[k].SetLineEndPoint(ptstart);
							vec.ScaleToLength(vecMergeSeg[k].GetLength());
							Dpoint3d pt1 = vecSeg[k].GetLineStartPoint();
							Dpoint3d pt2 = vecSeg[k].GetLineEndPoint();
							pt1.Add(vec);
							pt2.Add(vec);
							vecSeg[k].SetLineStartPoint(pt1);
							vecSeg[k].SetLineEndPoint(pt2);
						}
					}

				}
			}

		}
	}

	if (vecMergeSeg.size() == 0 || vecSeg.size() == 0)
	{
		return false;
	}


	//for (int k = 0; k < vecMergeSeg.size(); k++)
	//{
	//	EditElementHandle eehtest1;
	//	LineHandler::CreateLineElement(eehtest1, nullptr, vecMergeSeg[k].GetLineSeg(), true, *ACTIVEMODEL);
	//	eehtest1.AddToModel();
	//}

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
			vecEndType = { { 0,0,0 },{0,0,0} };
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

		////墙墙合并分析周围物体
		if (m_iMergeType == 0)
		{
			m_wallfaceAssociateInfo.clear();
			AnalyzeAssociateWallAndSlab(*m_Solid, i, const_cast<MSElementDescrP>(m_vecFace[0].GetElementDescrCP()));//由于针对两墙平行，暂时只拿第一个面			
		}

		int comDir = 0;
		if (!bFlag)
		{
			comDir = 1;
		}
		if (m_iMergeType == 0)
		{
			if (rebarDir == 1)
			{
				if (vecSeg[0].GetLineStartPoint().z > vecSeg[0].GetLineEndPoint().z)
				{
					m_wallfaceAssociateInfo.m_bUpIsStatr = true;
					vecEndType[0] = m_wallfaceAssociateInfo.m_topEndinfo;
					vecEndType[1] = m_wallfaceAssociateInfo.m_bottomEndinfo;
				}
				else
				{
					m_wallfaceAssociateInfo.m_bUpIsStatr = false;
					vecEndType[1] = m_wallfaceAssociateInfo.m_topEndinfo;
					vecEndType[0] = m_wallfaceAssociateInfo.m_bottomEndinfo;
				}
			}
			else
			{
				AnalyzeHorizeEndType(vecMergeSeg, i);
				vecEndType[0] = m_wallfaceAssociateInfo.m_HorStrType;
				vecEndType[1] = m_wallfaceAssociateInfo.m_HorEndType;
			}
		}

		if (rebarDir == comDir)
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
			if (m_iMergeType == 0)
			{
				for (size_t i = 0; i < vecSeg1.size(); i++)
				{
					DPoint3d pstart2 = vecSeg2[0].GetLineStartPoint();
					DPoint3d pstend2 = vecSeg2[0].GetLineEndPoint();


					DPoint3d pstart = vecSeg1[i].GetLineStartPoint();
					DPoint3d pstend = vecSeg1[i].GetLineEndPoint();
					if (pstart2.z > pstend2.z)
					{
						if (m_wallfaceAssociateInfo.m_wallTopFaceType == 1)
						{
							pstart.z += m_wallfaceAssociateInfo.m_dUpSlabThickness;
							pstend.z += m_wallfaceAssociateInfo.m_dUpSlabThickness;
						}

					}
					else
					{
						if (m_wallfaceAssociateInfo.m_wallBottomFaceType == 1)
						{
							pstart.z -= m_wallfaceAssociateInfo.m_dbottomSlabThickness;
							pstend.z -= m_wallfaceAssociateInfo.m_dbottomSlabThickness;
						}
					}

					vecSeg1[i].SetLineStartPoint(pstart);
					vecSeg1[i].SetLineEndPoint(pstend);
				}
				for (size_t i = 0; i < vecSeg2.size(); i++)
				{
					DPoint3d pstart = vecSeg2[i].GetLineStartPoint();
					DPoint3d pstend = vecSeg2[i].GetLineEndPoint();
					if (pstart.z < pstend.z)
					{
						if (m_wallfaceAssociateInfo.m_wallBottomFaceType == 1)
						{
							pstart.z -= m_wallfaceAssociateInfo.m_dbottomSlabThickness;
						}
						if (m_wallfaceAssociateInfo.m_wallTopFaceType == 1)
						{
							pstend.z += m_wallfaceAssociateInfo.m_dUpSlabThickness;
						}
					}
					else
					{
						if (m_wallfaceAssociateInfo.m_wallTopFaceType == 1)
						{
							pstart.z += m_wallfaceAssociateInfo.m_dUpSlabThickness;
						}
						if (m_wallfaceAssociateInfo.m_wallBottomFaceType == 1)
						{
							pstend.z -= m_wallfaceAssociateInfo.m_dbottomSlabThickness;
						}
					}
					vecSeg2[i].SetLineStartPoint(pstart);
					vecSeg2[i].SetLineEndPoint(pstend);
				}
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

	if (FacePreviewButtonsDown)//画预览线
	{
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
				m_allLines.push_back(eeh.GetElementRef());//存储所有画线
			}
		}
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

void MultiPlaneRebarAssembly::AnalyzeRebarDir()
{
	bool bHasHorizFace = false;
	bool bIsAllHorizFace = true;
	for (int i = 0; i < m_vecLineSeg.size(); i++)
	{
		DVec3d vec2 = m_vecLineSeg[i].LineSeg2.GetLineVec();
		if (abs(vec2.DotProduct(DVec3d::From(0, 0, 1))) < 0.9)
		{
			bHasHorizFace = true;

		}
		else
		{
			bIsAllHorizFace = false;
		}
	}



	for (int i = 1; i < m_vecLineSeg.size(); i++)
	{
		PIT::LineSegment  lineSeg;
		lineSeg = m_vecLineSeg.at(i - 1).LineSeg1;

		DPoint3d pt1[2];
		pt1[0] = lineSeg.GetLineStartPoint();
		pt1[1] = lineSeg.GetLineEndPoint();
		DPoint3d pt1Other[2];
		pt1Other[0] = m_vecLineSeg.at(i - 1).LineSeg2.GetLineStartPoint();
		pt1Other[1] = m_vecLineSeg.at(i - 1).LineSeg2.GetLineEndPoint();

		DPoint3d pt2[2];
		pt2[0] = m_vecLineSeg[i].LineSeg1.GetLineStartPoint();
		pt2[1] = m_vecLineSeg[i].LineSeg1.GetLineEndPoint();
		DPoint3d pt2Other[2];
		pt2Other[0] = m_vecLineSeg[i].LineSeg2.GetLineStartPoint();
		pt2Other[1] = m_vecLineSeg[i].LineSeg2.GetLineEndPoint();


		DVec3d vec = pt1[1] - pt1[0];
		vec.Normalize();
		DVec3d vecother = m_vecLineSeg.at(i - 1).LineSeg2.GetLineVec();

		DVec3d vec2 = pt2[1] - pt2[0];
		vec2.Normalize();
		if (bIsAllHorizFace)
		{
			m_iMergeType = 1;
			Dpoint3d ptStart = pt2[0], ptEnd = pt2[1];

			if (abs(vec.x - 0.00) < EPS)
			{
				ptStart.x = pt1[0].x;
				ptEnd.x = pt1[1].x;
			}
			if (abs(vec.y - 0.00) < EPS)
			{
				ptStart.y = pt1[0].y;
				ptEnd.y = pt1[1].y;
			}
			if (abs(vec.z - 0.00) < EPS)
			{
				ptStart.z = pt1[0].z;
				ptEnd.z = pt1[1].z;
			}
			double distance00 = ptStart.Distance(pt1[0]);
			double distance01 = ptStart.Distance(pt1[1]);
			double distance10 = ptEnd.Distance(pt1[0]);
			double distance11 = ptEnd.Distance(pt1[1]);

			if ((distance00 < distance01 && distance10 < distance11)
				|| (distance01 < distance00 &&distance11 < distance10))
			{
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
				m_vecLineSeg[i].LineSeg1.SetLineStartPoint(pt2[0]);
				m_vecLineSeg[i].LineSeg1.SetLineEndPoint(pt2[1]);
			}
			else
			{
				if (abs(vecother.x - 0.00) < EPS)
				{
					pt2Other[0].x = pt1Other[0].x;
					pt2Other[1].x = pt1Other[1].x;
				}

				if (abs(vecother.y - 0.00) < EPS)
				{
					pt2Other[0].y = pt1Other[0].y;
					pt2Other[1].y = pt1Other[1].y;
				}

				if (abs(vecother.z - 0.00) < EPS)
				{
					pt2Other[0].z = pt1Other[0].z;
					pt2Other[1].z = pt1Other[1].z;
				}
				m_vecLineSeg[i].LineSeg2.SetLineStartPoint(pt2Other[0]);
				m_vecLineSeg[i].LineSeg2.SetLineEndPoint(pt2Other[1]);
			}
		}
		else if (bHasHorizFace)
		{
			m_iMergeType = 2;
		}
		else
		{
			m_iMergeType = 0;
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
			m_vecLineSeg[i].LineSeg1.SetLineStartPoint(pt2[0]);
			m_vecLineSeg[i].LineSeg1.SetLineEndPoint(pt2[1]);
		}
	}
}

void MultiPlaneRebarAssembly::AnalyzeAssociateWallAndSlab(EditElementHandleCR eeh, int i, MSElementDescrP EdpFace)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	WallHelper::WallGeometryInfo wall_geometry_info;
	if (!WallHelper::analysis_geometry(eeh, wall_geometry_info))
	{
		mdlOutput_error(L"获得墙几何信息失败");
		return;
	}
	std::vector<WallHelper::Associated> associated;
	if (!WallHelper::GetAssociatedWallAndFloor(eeh, associated, wall_geometry_info))
	{
		mdlOutput_error(L"扫描相连构件失败");
		return;
	}
	auto top_slab_iter =
		std::find_if(
			associated.cbegin(), associated.cend(),
			[](const WallHelper::Associated &assoc)
	{
		return assoc.type == WallHelper::AssociatedType::TopFloor;
	});
	auto bottom_slab_iter =
		std::find_if(
			associated.cbegin(), associated.cend(),
			[](const WallHelper::Associated &assoc)
	{
		return assoc.type == WallHelper::AssociatedType::BottomFloor;
	});

	m_wallfaceAssociateInfo.m_associatedWall.clear();
	ElementId id = 0;
	for (int i = 0; i < associated.size(); i++)
	{
		if (associated[i].type == WallHelper::AssociatedType::Wall)
		{
			//EditElementHandle *tmpeeh = new EditElementHandle(associated[i].element, true);

			m_wallfaceAssociateInfo.m_associatedWall.push_back(associated[i].element);
		}
	}

	BrString strRebarSize(GetMainRebars().at(i).rebarSize);
	double	bendradius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);		//乘以了10
	double diameter = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL);
	double la0 = diameter * 15;
	double Lae = PlaneRebarAssembly::GetLae();
	double endLength = la0;
	if (Lae > 0)
	{
		Lae *= stod(GetMainRebars().at(i).rebarSize);
	}
	else
	{
		Lae = la0;
	}
	if (top_slab_iter != associated.cend())//有顶板
	{
		EditElementHandle top_slab(top_slab_iter->element, true);
		DVec3d top_slab_dir;
		if (!WallHelper::analysis_slab_position_with_wall(eeh, wall_geometry_info.normal, top_slab, top_slab_dir))
		{
			mdlOutput_error(L"分析墙的顶板方向失败");
			return;
		}

		DPoint3d ptmin, ptmax;
		mdlElmdscr_computeRange(&ptmin, &ptmax, top_slab.GetElementDescrP(), nullptr);

		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(top_slab, Eleeh, Holeehs);
		DPoint3d ptmiddle = DPoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);
		Dpoint3d ptUp = ptmiddle;
		Dpoint3d ptDown = ptmiddle;
		ptDown.z -= 5000 * UOR_PER_MilliMeter;
		ptUp.z += 5000 * UOR_PER_MilliMeter;
		Dpoint3d ptUp1 = ptUp;
		Dpoint3d ptDown1 = ptDown;
		ptUp1.x = ptmin.x + 100, ptUp1.y = ptmin.y + 100;
		ptDown1.x = ptmin.x + 100, ptDown1.y = ptmin.y + 100;
		vector<Dpoint3d> vecPT;
		GetIntersectPointsWithOldElmOwner(vecPT, &Eleeh, ptUp, ptDown, 0);
		GetIntersectPointsWithOldElmOwner(vecPT, &Eleeh, ptUp1, ptDown1, 0);

		m_wallfaceAssociateInfo.m_dUpSlabThickness = ptUp.Distance(ptDown) < ptUp1.Distance(ptDown1) ? ptUp.Distance(ptDown) : ptUp1.Distance(ptDown1);


		auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
		WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, top_slab, top_wall_inside_outside_faces);
		for (size_t i = 0; i < top_wall_inside_outside_faces.inside_faces.size(); i++)
		{
			MSElementDescrP insideface = top_wall_inside_outside_faces.inside_faces[i]->GetElementDescrP();
			MSElementDescrP insidefaceCopy;
			insideface->Duplicate(&insidefaceCopy);
			MSElementDescrP pResult1 = nullptr;
			MSElementDescrP pResult2 = nullptr;
			if (SUCCESS == mdlElmdscr_intersectShapes(&pResult1, &pResult2, insidefaceCopy, EdpFace, 0.1))
			{
				m_wallfaceAssociateInfo.m_wallTopFaceType = 0;
				m_wallfaceAssociateInfo.m_dTopOffset = 2 * diameter;
			}
		}
		for (size_t i = 0; i < top_wall_inside_outside_faces.outside_faces.size(); i++)
		{
			MSElementDescrP outsideface = top_wall_inside_outside_faces.outside_faces[i]->GetElementDescrP();
			MSElementDescrP outsidefaceCopy;
			outsideface->Duplicate(&outsidefaceCopy);
			MSElementDescrP pResult1 = nullptr;
			MSElementDescrP pResult2 = nullptr;
			if (SUCCESS == mdlElmdscr_intersectShapes(&pResult1, &pResult2, outsidefaceCopy, EdpFace, 0.1))
			{
				m_wallfaceAssociateInfo.m_wallTopFaceType = 1;
				//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
				endLength = Lae;
			}
		}
		auto top_angle = top_slab_dir.AngleTo(GetfaceNormal());

		// 转换到角度并设置到端部中
		double angel = top_angle / PI * 180;
		if (top_slab_dir == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
		{
			angel = 180;
		}
		m_wallfaceAssociateInfo.m_topEndinfo.endType = 4;
		m_wallfaceAssociateInfo.m_topEndinfo.rotateAngle = angel;
		m_wallfaceAssociateInfo.m_topEndinfo.endPtInfo.value1 = bendradius;
		m_wallfaceAssociateInfo.m_topEndinfo.endPtInfo.value3 = endLength;
	}
	if (bottom_slab_iter != associated.cend())//有底板
	{
		EditElementHandle bottom_slab(bottom_slab_iter->element, true);
		DVec3d bottom_slab_dir;
		if (!WallHelper::analysis_slab_position_with_wall(eeh, wall_geometry_info.normal, bottom_slab, bottom_slab_dir))
		{
			mdlOutput_error(L"分析墙的顶板方向失败");
			return;
		}



		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(bottom_slab, Eleeh, Holeehs);

		DPoint3d ptmin, ptmax;
		mdlElmdscr_computeRange(&ptmin, &ptmax, bottom_slab.GetElementDescrP(), nullptr);
		DPoint3d ptmiddle = DPoint3d::From((ptmin.x + ptmax.x) / 2, (ptmin.y + ptmax.y) / 2, (ptmin.z + ptmax.z) / 2);
		Dpoint3d ptUp = ptmiddle;
		Dpoint3d ptDown = ptmiddle;
		ptDown.z -= 5000 * UOR_PER_MilliMeter;
		ptUp.z += 5000 * UOR_PER_MilliMeter;
		Dpoint3d ptUp1 = ptUp;
		Dpoint3d ptDown1 = ptDown;
		ptUp1.x = ptmin.x + 100, ptUp1.y = ptmin.y + 100;
		ptDown1.x = ptmin.x + 100, ptDown1.y = ptmin.y + 100;
		vector<Dpoint3d> vecPT;
		GetIntersectPointsWithOldElmOwner(vecPT, &Eleeh, ptUp, ptDown, 0);
		GetIntersectPointsWithOldElmOwner(vecPT, &Eleeh, ptUp1, ptDown1, 0);
		m_wallfaceAssociateInfo.m_dbottomSlabThickness = ptUp.Distance(ptDown) < ptUp1.Distance(ptDown1) ? ptUp.Distance(ptDown) : ptUp1.Distance(ptDown1);

		auto top_wall_inside_outside_faces = WallHelper::WallInsideOutsideFaces();
		WallHelper::analysis_inside_outside_faces_by_slab(wall_geometry_info, bottom_slab, top_wall_inside_outside_faces);
		for (size_t i = 0; i < top_wall_inside_outside_faces.inside_faces.size(); i++)
		{
			MSElementDescrP insideface = top_wall_inside_outside_faces.inside_faces[i]->GetElementDescrP();
			MSElementDescrP insidefaceCopy;
			insideface->Duplicate(&insidefaceCopy);
			MSElementDescrP pResult1 = nullptr;
			MSElementDescrP pResult2 = nullptr;
			if (SUCCESS == mdlElmdscr_intersectShapes(&pResult1, &pResult2, insidefaceCopy, EdpFace, 0.1))
			{
				m_wallfaceAssociateInfo.m_wallBottomFaceType = 0;
				m_wallfaceAssociateInfo.m_dBottomOffset = 2 * diameter;
			}
		}
		for (size_t i = 0; i < top_wall_inside_outside_faces.outside_faces.size(); i++)
		{
			MSElementDescrP outsideface = top_wall_inside_outside_faces.outside_faces[i]->GetElementDescrP();
			MSElementDescrP outsidefaceCopy;
			outsideface->Duplicate(&outsidefaceCopy);
			MSElementDescrP pResult1 = nullptr;
			MSElementDescrP pResult2 = nullptr;
			if (SUCCESS == mdlElmdscr_intersectShapes(&pResult1, &pResult2, outsidefaceCopy, EdpFace, 0.1))
			{
				m_wallfaceAssociateInfo.m_wallBottomFaceType = 1;
				//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm - bendradius - diameter / 2;
				endLength = Lae;
			}
		}

		auto bottom_angle = bottom_slab_dir.AngleTo(GetfaceNormal());

		// 转换到角度并设置到端部中
		double angel = bottom_angle / PI * 180;
		if (bottom_slab_dir == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
		{
			angel = 180;
		}
		m_wallfaceAssociateInfo.m_bottomEndinfo.endType = 4;
		m_wallfaceAssociateInfo.m_bottomEndinfo.rotateAngle = angel;
		m_wallfaceAssociateInfo.m_bottomEndinfo.endPtInfo.value1 = bendradius;
		m_wallfaceAssociateInfo.m_bottomEndinfo.endPtInfo.value3 = endLength;

	}
	//删除相关处理
	for (WallHelper::Associated assoc : associated)
	{
		EditElementHandle eeh(assoc.element.GetElementId(), ACTIVEMODEL);
		if (eeh.IsValid())
		{
			eeh.DeleteFromModel();
		}
	}

}

void MultiPlaneRebarAssembly::AnalyzeHorizeEndType(vector<PIT::LineSegment> vecMergeSeg, int rebarnum)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	ElementId id = 0;
	DVec3d facenormal = GetfaceNormal();

	EditElementHandle Eleehold;
	std::vector<EditElementHandle*> vecholesold;
	EFT::GetSolidElementAndSolidHoles(*m_Solid, Eleehold, vecholesold);
	vector<DSegment3d> vecDownFontLineold;
	vector<DSegment3d> vecDownBackLineold;
	double heightOld;
	EFT::GetFrontBackLinePoint(Eleehold, vecDownFontLineold, vecDownBackLineold, &heightOld);

	if (vecDownFontLineold.size() == 0 && vecDownFontLineold.size() == 0)
	{
		return;
	}
	DSegment3d segFontOld = vecDownFontLineold[0];
	DSegment3d segBackOld = vecDownBackLineold[0];

	PIT::LineSegment lineSeg1(vecMergeSeg[0].GetLineStartPoint(), vecMergeSeg[vecMergeSeg.size() - 1].GetLineEndPoint());
	for (size_t i = 0; i < m_wallfaceAssociateInfo.m_associatedWall.size(); i++)
	{
		EditElementHandle eeh(m_wallfaceAssociateInfo.m_associatedWall[i].GetElementId(), m_wallfaceAssociateInfo.m_associatedWall[i].GetDgnModelP());
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> vecholes;
		EFT::GetSolidElementAndSolidHoles(eeh, Eleeh, vecholes);
		double height = 0;
		vector<DSegment3d> vecDownFontLine;
		vector<DSegment3d> vecDownBackLine;
		EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &height);
		if (vecDownFontLine.size() == 0 && vecDownBackLine.size() == 0)
		{
			continue;
		}
		Dpoint3d ptstatr, ptend;
		vecDownFontLine[0].GetEndPoints(ptstatr, ptend);
		DVec3d vecLine = ptend - ptstatr;
		vecLine.Normalize();
		DVec3d vecLineseg = lineSeg1.GetLineVec();

		if (abs(vecLineseg.DotProduct(vecLine)) > 0.9)
		{
			continue;
		}
		DSegment3d segmain = lineSeg1.GetLineSeg();
		DSegment3d segFont = vecDownFontLine[0];
		DSegment3d segBack = vecDownBackLine[0];
		DPoint3d fontstart, fontend;
		segFont.GetEndPoints(fontstart, fontend);
		DPoint3d backstart, backend;
		segBack.GetEndPoints(backstart, backend);
		DPoint3d ptStart = lineSeg1.GetLineStartPoint();
		DPoint3d ptEnd = lineSeg1.GetLineEndPoint();

		DPoint3d ptStartFont, ptEndFont;
		segFont.GetEndPoints(ptStartFont, ptEndFont);
		DPoint3d ptStartBack, ptEndBack;
		segBack.GetEndPoints(ptStartBack, ptEndBack);

		Dpoint3d ptFont, ptBack, ptFontold, ptBackold;
		//算交点时放在xoy水平面上
		DPoint3d ptStartFont1, ptEndFont1;
		segFont.GetEndPoints(ptStartFont1, ptEndFont1);
		DPoint3d ptStartBack1, ptEndBack1;
		segBack.GetEndPoints(ptStartBack1, ptEndBack1);
		ptStartFont1.z = 0;
		ptEndFont1.z = 0;
		ptStartBack1.z = 0;
		ptEndBack1.z = 0;
		DPoint3d ptStarMain1, ptEndMain1;
		segmain.GetEndPoints(ptStarMain1, ptEndMain1);
		ptStarMain1.z = 0;
		ptEndMain1.z = 0;
		segFont.SetStartPoint(ptStartFont1);
		segFont.SetEndPoint(ptEndFont1);
		segBack.SetStartPoint(ptStartBack1);
		segBack.SetEndPoint(ptEndBack1);
		segmain.SetStartPoint(ptStarMain1);
		segmain.SetEndPoint(ptEndMain1);
		//end
		mdlVec_intersect(&ptFont, &segFont, &segmain);//与主线的交点
		mdlVec_intersect(&ptBack, &segBack, &segmain);
		double DistanceAll = ptEnd.Distance(ptStart);
		double distnca = ptBack.DistanceXY(ptStart);
		if ((ptFont.DistanceXY(ptStart) > 10 && ptFont.DistanceXY(ptEnd) > 10)
			&& (ptBack.DistanceXY(ptStart) > 10 && ptBack.DistanceXY(ptEnd) > 10))
		{
			continue;
		}
		BrString strRebarSize(GetMainRebars().at(rebarnum).rebarSize);
		//double	diameter = RebarCode::GetBarDiameter(strSize1, ACTIVEMODEL);		//乘以了10
		double diameter = RebarCode::GetBarDiameter(strRebarSize, ACTIVEMODEL);
		//double	bendradius = RebarCode::GetPinRadius(strSize1, ACTIVEMODEL, false);		//乘以了10
		double	bendradius = RebarCode::GetPinRadius(strRebarSize, ACTIVEMODEL, false);
		double La0 = diameter * 15;//先为15d,如果判断外侧则修改
		double Lae = PlaneRebarAssembly::GetLae();
		double endLength = La0;
		if (Lae > 0)
		{
			Lae *= stod(GetMainRebars().at(rebarnum).rebarSize);
		}
		else
		{
			Lae = La0;
		}
		double DistanceThick = ptBack.Distance(ptFont);
		bool bStartIn = false;
		int bStartType = -1;
		int bEndInType = -1;
		if (ptFont.DistanceXY(ptStart) < 10)
		{
			double DistanceBack = ptBack.DistanceXY(ptEnd);
			mdlVec_intersect(&ptFontold, &segFontOld, &segFont);//当主线交点等于断点，则根据与原实体底线面判断
			mdlVec_intersect(&ptBackold, &segBackOld, &segFont);
			if (DistanceBack > DistanceAll)//在外面
			{
				bStartIn = false;
			}
			else //在里面
			{
				bStartIn = true;
			}
			if (bStartIn)
			{
				if (ptFontold.DistanceXY(ptStart) < 10)
				{
					if (ptFontold.DistanceXY(ptStartFont) > ptBackold.DistanceXY(ptStartFont))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptStart) < 10)
				{
					if (ptBackold.DistanceXY(ptStartFont) > ptFontold.DistanceXY(ptStartFont))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
			}
			else
			{
				if (ptFontold.DistanceXY(ptStart) < 10)
				{
					if ((ptFontold.DistanceXY(ptStartFont) < 10 || ptFontold.DistanceXY(ptEndFont) < 10)
						/*|| ptFontold.DistanceXY(ptStartFont) > ptBackold.DistanceXY(ptStartFont)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptStart) < 10)
				{
					if ((ptBackold.DistanceXY(ptStartFont) < 10 || ptBackold.DistanceXY(ptEndFont) < 10)
						/*||ptBackold.DistanceXY(ptStartFont) > ptFontold.DistanceXY(ptStartFont)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
			}

			DVec3d linvec = DVec3d::FromZero();
			if (ptFont.DistanceXY(fontstart) < ptFont.DistanceXY(fontend))
			{
				linvec = fontend - fontstart;
			}
			else
			{
				linvec = fontstart - fontend;
			}

			auto angle = linvec.AngleTo(GetfaceNormal());

			// 转换到角度并设置到端部中
			double angel = angle / PI * 180;
			if (linvec == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
			{
				angel = 180;
			}

			m_wallfaceAssociateInfo.m_HorStrType.endType = 4;
			m_wallfaceAssociateInfo.m_HorStrType.rotateAngle = angel;
			m_wallfaceAssociateInfo.m_HorStrType.endPtInfo.value1 = bendradius;
			m_wallfaceAssociateInfo.m_HorStrType.endPtInfo.value3 = endLength;
		}
		else if (ptFont.DistanceXY(ptEnd) < 10)
		{

			double DistanceFont = ptBack.DistanceXY(ptStart);
			mdlVec_intersect(&ptFontold, &segFontOld, &segFont);
			mdlVec_intersect(&ptBackold, &segBackOld, &segFont);
			if (DistanceFont > DistanceAll)//在外面
			{
				bStartIn = false;
			}
			else //在里面
			{
				bStartIn = true;
			}
			if (bStartIn)
			{
				if (ptFontold.DistanceXY(ptEnd) < 10)
				{
					if (ptFontold.DistanceXY(ptStartFont) > ptBackold.DistanceXY(ptStartFont))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptEnd) < 10)
				{
					if (ptBackold.DistanceXY(ptStartFont) > ptFontold.DistanceXY(ptStartFont))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
			}
			else
			{
				if (ptFontold.DistanceXY(ptEnd) < 10)
				{
					if ((ptFontold.DistanceXY(ptStartFont) < 10 || ptFontold.DistanceXY(ptEndFont) < 10)
						/*|| ptFontold.DistanceXY(ptStartFont) > ptBackold.DistanceXY(ptStartFont)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptEnd) < 10)
				{
					if ((ptBackold.DistanceXY(ptStartFont) < 10 || ptBackold.DistanceXY(ptEndFont) < 10)
						/*|| ptBackold.DistanceXY(ptStartFont) > ptFontold.DistanceXY(ptStartFont)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
			}

			DVec3d linvec = DVec3d::FromZero();
			if (ptFont.DistanceXY(fontstart) < ptFont.DistanceXY(fontend))
			{
				linvec = fontend - fontstart;
			}
			else
			{
				linvec = fontstart - fontend;
			}

			auto angle = linvec.AngleTo(GetfaceNormal());

			// 转换到角度并设置到端部中
			double angel = angle / PI * 180;
			if (linvec == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
			{
				angel = 180;
			}

			m_wallfaceAssociateInfo.m_HorEndType.endType = 4;
			m_wallfaceAssociateInfo.m_HorEndType.rotateAngle = angel;
			m_wallfaceAssociateInfo.m_HorEndType.endPtInfo.value1 = bendradius;
			m_wallfaceAssociateInfo.m_HorEndType.endPtInfo.value3 = endLength;
		}
		else if (ptBack.DistanceXY(ptEnd) < 10)
		{

			double DistanceFont = ptFont.DistanceXY(ptStart);
			mdlVec_intersect(&ptFontold, &segFontOld, &segBack);
			mdlVec_intersect(&ptBackold, &segBackOld, &segBack);
			if (DistanceFont > DistanceAll)//在外面
			{
				bStartIn = false;
			}
			else //在里面
			{
				bStartIn = true;
			}
			if (bStartIn)
			{
				if (ptFontold.DistanceXY(ptEnd) < 10)
				{
					if (ptFontold.DistanceXY(ptStartBack) > ptBackold.DistanceXY(ptStartBack))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptEnd) < 10)
				{
					if (ptBackold.DistanceXY(ptStartBack) > ptFontold.DistanceXY(ptStartBack))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
			}
			else
			{
				if (ptFontold.DistanceXY(ptEnd) < 10)
				{
					if ((ptFontold.DistanceXY(ptStartBack) < 10 || ptFontold.DistanceXY(ptEndBack) < 10)
						/*|| ptFontold.DistanceXY(ptStartBack) > ptBackold.DistanceXY(ptStartBack)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptEnd) < 10)
				{
					if ((ptBackold.DistanceXY(ptStartBack) < 10 || ptBackold.DistanceXY(ptEndBack) < 10)
						/*|| ptBackold.DistanceXY(ptStartBack) > ptFontold.DistanceXY(ptStartBack)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bEndInType = 1;
					}
				}
			}

			DVec3d linvec = DVec3d::FromZero();
			if (ptBack.DistanceXY(backstart) < ptBack.DistanceXY(backend))
			{
				linvec = backend - backstart;
			}
			else
			{
				linvec = backstart - backend;
			}

			auto angle = linvec.AngleTo(GetfaceNormal());

			// 转换到角度并设置到端部中
			double angel = angle / PI * 180;
			if (linvec == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
			{
				angel = 180;
			}

			m_wallfaceAssociateInfo.m_HorEndType.endType = 4;
			m_wallfaceAssociateInfo.m_HorEndType.rotateAngle = angel;
			m_wallfaceAssociateInfo.m_HorEndType.endPtInfo.value1 = bendradius;
			m_wallfaceAssociateInfo.m_HorEndType.endPtInfo.value3 = endLength;
		}
		else if (ptBack.DistanceXY(ptStart) < 10)
		{

			double DistanceFont = ptFont.DistanceXY(ptEnd);
			mdlVec_intersect(&ptFontold, &segFontOld, &segBack);
			mdlVec_intersect(&ptBackold, &segBackOld, &segBack);
			if (DistanceFont > DistanceAll)//在外面
			{
				bStartIn = false;
			}
			else //在里面
			{
				bStartIn = true;
			}
			if (bStartIn)
			{
				if (ptFontold.DistanceXY(ptStart) < 10)
				{
					if (ptFontold.DistanceXY(ptStartBack) > ptBackold.DistanceXY(ptStartBack))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptStart) < 10)
				{
					if (ptBackold.DistanceXY(ptStartBack) > ptFontold.DistanceXY(ptStartBack))
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
			}
			else
			{
				if (ptFontold.DistanceXY(ptStart) < 10)
				{
					if ((ptFontold.DistanceXY(ptStartBack) < 10 || ptFontold.DistanceXY(ptEndBack) < 10)
						/*|| ptFontold.DistanceXY(ptStartBack) > ptBackold.DistanceXY(ptStartBack)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
				else if (ptBackold.DistanceXY(ptStart) < 10)
				{
					if ((ptBackold.DistanceXY(ptStartBack) < 10 || ptBackold.DistanceXY(ptEndBack) < 10)
						/*|| ptBackold.DistanceXY(ptStartBack) > ptFontold.DistanceXY(ptStartBack)*/)
					{
						//LaE = g_globalpara.m_alength[(string)strRebarSize] * uor_per_mm;// -bendradius - diameter / 2;
						endLength = Lae;
						bStartType = 1;
					}
				}
			}


			DVec3d linvec = DVec3d::FromZero();
			if (ptBack.DistanceXY(backstart) < ptBack.DistanceXY(backend))
			{
				linvec = backend - backstart;
			}
			else
			{
				linvec = backstart - backend;
			}
			auto angle = linvec.AngleTo(GetfaceNormal());

			// 转换到角度并设置到端部中
			double angel = angle / PI * 180;
			if (linvec == DVec3d::FromZero())//top_slab_dir == DVec3d::FromZero()即墙与板为T型墙
			{
				angel = 180;
			}

			m_wallfaceAssociateInfo.m_HorStrType.endType = 4;
			m_wallfaceAssociateInfo.m_HorStrType.rotateAngle = angel;
			m_wallfaceAssociateInfo.m_HorStrType.endPtInfo.value1 = bendradius;
			m_wallfaceAssociateInfo.m_HorStrType.endPtInfo.value3 = endLength;
		}
		else
		{
			continue;
		}
		if (bStartType == 1)
		{
			m_wallfaceAssociateInfo.m_bStartType = 1;
		}
		else
		{
			m_wallfaceAssociateInfo.m_bStartType = 0;

			m_wallfaceAssociateInfo.m_dStartOffset = -2 * diameter;

		}
		if (bEndInType == 1)
		{
			m_wallfaceAssociateInfo.m_bEndInType = 1;

		}
		else
		{
			m_wallfaceAssociateInfo.m_bEndInType = 0;

			m_wallfaceAssociateInfo.m_dEndOffset = -2 * diameter;

		}
		EditElementHandle walleeh(m_wallfaceAssociateInfo.m_associatedWall[i], true);
		MSElementDescrP wallEdp = nullptr;
		walleeh.GetElementDescrP()->Duplicate(&wallEdp);
		m_wallfaceAssociateInfo.m_vecWallEdp.push_back(wallEdp);
	}

}

long MultiPlaneRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool MultiPlaneRebarAssembly::OnDoubleClick()
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

bool MultiPlaneRebarAssembly::Rebuild()
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
