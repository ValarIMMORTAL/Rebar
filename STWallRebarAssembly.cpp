#include "_ustation.h"
#include "resource.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "STWallRebarAssembly.h"
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
#include <CElementTool.h>
#include <CPointTool.h>

#include "MakeRebarHelper.h"

extern bool PreviewButtonDown;//��Ҫ�������Ԥ����ť
extern map<int, vector<RebarPoint>> g_wallRebarPtsNoHole;

using namespace PIT;

void STWallRebarAssembly::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

// ��ȡ��Ĺؼ���
void STWallRebarAssembly::ExtractBoundaryPoints(MSElementDescrP faceDescr, vector<DPoint3d>& vec_ptBoundary) {
	// ��ȡ��߽�ؼ���
	PITCommonTool::CElementTool::ExtractCellPoints(faceDescr, vec_ptBoundary);

	// ȥ��
	vector<DPoint3d> vec_uniPtBoundary;
	if (!vec_ptBoundary.empty()) {
		vec_uniPtBoundary.push_back(vec_ptBoundary[0]); // ������һ����

		for (size_t i = 1; i < vec_ptBoundary.size(); ++i) {
			const DPoint3d& lastPoint = vec_uniPtBoundary.back();
			const DPoint3d& currentPoint = vec_ptBoundary[i];

			// �Ƚ��������Ƿ���ͬ
			if (!lastPoint.IsEqual(currentPoint)) {
				vec_uniPtBoundary.push_back(currentPoint); // ����ͬ������
			}
		}
	}

	// ����պ϶�����غϵ���ʼ��
	if (vec_uniPtBoundary.size() > 1 && vec_uniPtBoundary.front().IsEqual(vec_uniPtBoundary.back()))
		vec_uniPtBoundary.pop_back();

	// ��ȥ�غ�ĵ�Ż� vec_ptBoundary
	vec_ptBoundary = std::move(vec_uniPtBoundary);
}

bool STWallRebarAssembly::makaRebarCurve(const vector<DPoint3d>& linePts, double extendStrDis, double extendEndDis,
	double diameter, double strMoveDis, double endMoveDis, bool isInSide,
	const PIT::PITRebarEndTypes& endTypes, vector<PIT::PITRebarCurve>& rebars, std::vector<EditElementHandle*> upflooreehs,
	std::vector<EditElementHandle*> flooreehs, std::vector<EditElementHandle*> Walleehs, std::vector<EditElementHandle*>alleehs)
{
	//�����ṩ�ĵ���������
	EditElementHandle lineEeh;
	LineStringHandler::CreateLineStringElement(lineEeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
	//�����ߵ���β��
	DPoint3d strPt = { 0,0,0 }, endPt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&strPt, nullptr, &endPt, nullptr, lineEeh.GetElementDescrP(), ACTIVEMODEL);
	//mdlElmdscr_add(lineEeh.GetElementDescrP());
	if (GetIntersectPointsWithNegs(m_Negs, strPt, endPt))
	{
		return false;
	}
	m_vecRebarPtsLayer.push_back(strPt);
	m_vecRebarPtsLayer.push_back(endPt);

	//�����ԭʼǽ���ཻ��ĵ�
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	//double dSideCover = GetSideCover()*uor_per_mm;
	//vector<DPoint3d> tmppts;
	//Transform matrix;
	//GetPlacement().AssignTo(matrix);
	//GetIntersectPointsWithHoles(tmppts, m_useHoleehs, strPt, endPt, dSideCover, matrix);
	double dSideCover = 0/*GetSideCover() * uor_per_mm*/;
	vector<DPoint3d> tmppts;
	vector<DPoint3d> tmppts2;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	vector<vector<DPoint3d>> vecPtRebars;
	vector<DPoint3d> tmpptsTmp;
	vector<DPoint3d> ptsIntersectTmp;//�ཻ��
	vecPtRebars.clear();
	PITRebarEndTypes tmpendEndTypes = endTypes;
	PITRebarEndTypes tmpendEndTypes_2;
	int flag_str = 0;
	int flag_end = 1;
	int flag_mid = -1;
	DPoint3d end_Ptr;
	DPoint3d str_Ptr;
	tmpptsTmp.clear();
	EditElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	if (!ehWall.IsValid())
	{
		return false;
	}
	EFT::GetSolidElementAndSolidHoles(ehWall, Eleeh, Holeehs);//��ȡ����֮ǰ��ǽ��
	if (Eleeh.IsValid()) // ��ԭʵ���ཻ(�޿׶�)
	{
		//Eleeh.AddToModel();
		GetIntersectPointsWithOldElm(tmpptsTmp, &Eleeh, strPt, endPt, dSideCover, matrix);
		if (tmpptsTmp.size() > 1)
		{
			// ���ڽ���Ϊ�������ϵ����
			GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, strPt, endPt, &Eleeh, dSideCover);
		}
		//  �ų�����ֽ�����
		// ��ȡ is_inside �ľ���ֵ
		double is_transverse = std::abs(strPt.z - endPt.z);
		// �뿪��֮���ģ�Ͳ����ڽ��� ���� ��Ϊ����ֽ� �����
		GetIntersectPointsWithOldElm(ptsIntersectTmp, &ehWall, strPt, endPt, dSideCover, matrix);
		if (ptsIntersectTmp.size() < 1 && is_transverse >= 1000)
		{
			return false;
		}
	}


	if (tmpptsTmp.size() < 2 && vecPtRebars.size() == 0)
	{
		vector<DPoint3d> vecPt;
		vecPt.push_back(strPt);
		vecPt.push_back(endPt);

		vecPtRebars.push_back(vecPt);
	}
	//�����ܰ�Ϳ׶���ĵ�	
	for (size_t i = 0; i < vecPtRebars.size(); i++)
	{
		strPt = vecPtRebars.at(i).at(0);
		endPt = vecPtRebars.at(i).at(1);

		CVector3D  Vec(strPt, endPt);
		CVector3D  nowVec = Vec;
		double extendStrDistemp = extendStrDis, extendEndDistemp = extendEndDis;
		//���ݶ��װ����¼���ƫ�ƾ���
		//PITCommonTool::CPointTool::DrowOneLine(strPt, endPt, 2);
		Vec.Normalize();
		if (COMPARE_VALUES_EPS(abs(Vec.x), 1, 0.1) == 0 || COMPARE_VALUES_EPS(abs(Vec.y), 1, 0.1) == 0 || COMPARE_VALUES_EPS(abs(Vec.z), 1, 1e-6) == 0)
		{
			ReCalExtendDisByTopDownFloor(strPt, endPt, strMoveDis, endMoveDis, extendStrDis, extendEndDis, isInSide);
		}
		//������β�ƶ�����
		DVec3d strVec = strPt - endPt;
		strVec.Normalize();
		strVec.ScaleToLength(extendStrDis);
		DVec3d endVec = endPt - strPt;
		endVec.Normalize();
		endVec.ScaleToLength(extendEndDis);
		//�ƶ���β��
		strPt.Add(strVec);
		endPt.Add(endVec);
		//PITCommonTool::CPointTool::DrowOneLine(strPt, endPt, 1);
		EditElementHandle* eeh = &Eleeh;
		//���ƫ�Ƶĵ���������׶�����˵�����洦�����Ƕ���ཻ��
		nowVec.Normalize();
		if (COMPARE_VALUES_EPS(abs(nowVec.z), 1, EPS) == 0)
		{
			//�װ�ת����1.���ê��δ�ڵװ�����ê��ת��2.���ת���δê�뵽�κ�ʵ���У���ԭ����,���п׶��ضϲ���
			CVector3D vector = endTypes.beg.GetendNormal();
			CVector3D vecZ = CVector3D::From(0, 0, -1);
			CVector3D vecO = CVector3D::From(0, 0, 0);
			DPoint3d  endtemp_point3d = strPt;
			DPoint3d nonstrtemp_point3d = strPt;
			double lenth = endTypes.beg.GetbendLen() + endTypes.beg.GetbendRadius();
			FreeAll(Holeehs);
			Holeehs.clear();
			tmppts.clear();
			movePoint(vecZ, endtemp_point3d, lenth);
			DPoint3d zeropt = DPoint3d::From(0, 0, 0);
			DPoint3d tmpvecZ = DPoint3d::From(0, 0, 1);
			Transform tran;			//����ͶӰ����
			mdlTMatrix_computeFlattenTransform(&tran, &zeropt, &tmpvecZ);
			MSElementDescrP downface = nullptr;
			downface = ExtractFacesTool::GetCombineDownFace(ehWall);
			DPoint3d vecwall1 = DPoint3d::From(0, 0, 0);
			GetDownFaceVecAndThickness(downface, vecwall1);

			bool isOpen = false;
			for (int i = 0; i < Walleehs.size(); i++)
			{
				MSElementDescrP downface = nullptr;
				downface = ExtractFacesTool::GetCombineDownFace(*Walleehs[i]);
				//����ͶӰ��XOYƽ��
				mdlElmdscr_transform(&downface, &tran);
				DPoint3d vecwall = DPoint3d::From(0, 0, 0);
				GetDownFaceVecAndThickness(downface, vecwall);
				if (ISPointInElement(Walleehs[i], endtemp_point3d) && abs(vecwall1.DotProduct(vecwall) < 0.9))//��ֱʱ���ӳ�)
				{
					break;
				}
			}
			if (isOpen&&isInSide)//�ж�Z���Ϸ������·��Ƿ���ǽ�������ڲ�ֽ�
			{
				tmpendEndTypes.beg.SetendNormal(vecO);
				tmpendEndTypes.beg.SetType(PIT::PITRebarEndType::Type::kNone);
				strPt = endtemp_point3d;
			}
			else
			{
				endtemp_point3d = strPt;
				JudgeBarLinesLegitimate(nowVec, alleehs, tmpendEndTypes, lineEeh, eeh, vector, matrix, strMoveDis, lenth, strPt, str_Ptr, flag_str);
			}

			//����ת����1.���ê��δ�ڶ�������ê��ת��2.���ת���δê�뵽�κ�ʵ���У���ԭ����,���п׶��ضϲ���
			vector = endTypes.end.GetendNormal();
			endtemp_point3d = endPt;
			DPoint3d nonendtemp_point3d = endPt;
			lenth = endTypes.end.GetbendLen() + endTypes.end.GetbendRadius();
			FreeAll(Holeehs);
			Holeehs.clear();
			tmppts.clear();
			vecZ.Negate();
			isOpen = false;
			movePoint(vecZ, endtemp_point3d, lenth);
			for (int i = 0; i < Walleehs.size(); i++)
			{
				MSElementDescrP downface = nullptr;
				downface = ExtractFacesTool::GetCombineDownFace(*Walleehs[i]);
				//����ͶӰ��XOYƽ��
				mdlElmdscr_transform(&downface, &tran);
				DPoint3d vecwall = DPoint3d::From(0, 0, 0);
				GetDownFaceVecAndThickness(downface, vecwall);
				if (ISPointInElement(Walleehs[i], endtemp_point3d) && abs(vecwall1.DotProduct(vecwall) < 0.9))//��ֱʱ���ӳ�)
				{
					//isOpen = true;
					break;
				}
			}
			if (isOpen&&isInSide)//�ж�Z���Ϸ������·��Ƿ���ǽ�������ڲ�ֽ�
			{
				tmpendEndTypes.end.SetendNormal(vecO);
				tmpendEndTypes.end.SetType(PIT::PITRebarEndType::Type::kNone);
				endPt = endtemp_point3d;
			}
			else
			{
				endtemp_point3d = endPt;
				JudgeBarLinesLegitimate(nowVec, alleehs, tmpendEndTypes, lineEeh, eeh, vector, matrix, endMoveDis, lenth, endPt, end_Ptr, flag_end);
			}
		}
		else if (COMPARE_VALUES_EPS(abs(nowVec.x), 1, 0.1) == 0 || COMPARE_VALUES_EPS(abs(nowVec.y), 1, 0.1) == 0)
		{
			DPoint3d  strtemp_point3d = strPt, endtemp_point3d = endPt;//��ʼ��
			CVector3D str_vector = endTypes.beg.GetendNormal();//��ʼ�㷽��
			double str_extend_lenth = endTypes.beg.GetbendLen() + endTypes.beg.GetbendRadius();//��ʼ�����쳤��
			CVector3D end_vector = endTypes.end.GetendNormal();//ĩβ�㷽��
			double end_extend_lenth = endTypes.end.GetbendLen() + endTypes.end.GetbendRadius();//ĩβ�����쳤��
			CVector3D vectorZ = CVector3D::From(0, 0, 0);
			double lae = get_lae() * diameter / uor_per_mm;
			double der = diameter * 15;
			//�ж����̻����ӳ���ĵ��Ƿ���ǽ��
			strVec.Normalize();
			if (ISPointInHoles(Walleehs, strtemp_point3d) && str_vector.IsEqual(vectorZ))
			{
				tmpendEndTypes.beg = Hol;
				str_vector = tmpendEndTypes.beg.GetendNormal();
				str_extend_lenth = tmpendEndTypes.beg.GetbendLen() + tmpendEndTypes.beg.GetbendRadius();
				CVector3D vec = (m_walldata.vecToWall);
				if (isInSide)
				{
					tmpendEndTypes.beg.SetbendLen(der);
					DPoint3d temp = strtemp_point3d;
					movePoint(vec, temp, lae);
					if (ISPointInHoles(Walleehs, temp))
						tmpendEndTypes.beg.SetendNormal(vec);
					else
						tmpendEndTypes.beg.SetendNormal(vec.Negate());
				}

				//tmpendEndTypes.beg.SetendNormal(CVector3D::kZaxis.Perpendicular(nowVec));
			}
			else
			{
				nowVec.Negate();
				DPoint3d temp = strtemp_point3d;
				movePoint(nowVec, strtemp_point3d, lae);
				movePoint(nowVec, temp, der);
				//PITCommonTool::CPointTool::DrowOnePoint(strtemp_point3d, 1,11);
				if (ISPointInHoles(flooreehs, strtemp_point3d))
				{
					tmpendEndTypes.beg.SetendNormal(vectorZ);
					tmpendEndTypes.beg.SetType(PIT::PITRebarEndType::Type::kNone);
					strPt = strtemp_point3d;
				}
				else if (ISPointInHoles(flooreehs, temp))
				{
					tmpendEndTypes.beg.SetendNormal(vectorZ);
					tmpendEndTypes.beg.SetType(PIT::PITRebarEndType::Type::kNone);
					strPt = temp;
				}
				else
				{
					strtemp_point3d = strPt;
				}
			}

			if (ISPointInHoles(Walleehs, endtemp_point3d) && end_vector.IsEqual(vectorZ))
			{
				tmpendEndTypes.end = Hol;
				end_vector = tmpendEndTypes.end.GetendNormal();
				end_extend_lenth = tmpendEndTypes.end.GetbendLen() + tmpendEndTypes.end.GetbendRadius();
				CVector3D vec = (m_walldata.vecToWall);
				if (isInSide)
				{
					tmpendEndTypes.end.SetbendLen(der);
					DPoint3d temp = endtemp_point3d;
					movePoint(vec, temp, lae);
					if (ISPointInHoles(Walleehs, temp))
						tmpendEndTypes.end.SetendNormal(vec);
					else
						tmpendEndTypes.end.SetendNormal(vec.Negate());
				}
				//tmpendEndTypes.end.SetendNormal(CVector3D::kZaxis.Perpendicular(nowVec));
			}
			else
			{
				nowVec.Negate();
				DPoint3d temp = endtemp_point3d;
				movePoint(nowVec, endtemp_point3d, lae);
				movePoint(nowVec, temp, der);
				//PITCommonTool::CPointTool::DrowOnePoint(endtemp_point3d, 1, 12);
				if (ISPointInHoles(flooreehs, endtemp_point3d))
				{

					tmpendEndTypes.end.SetendNormal(vectorZ);
					tmpendEndTypes.end.SetType(PIT::PITRebarEndType::Type::kNone);
					endPt = endtemp_point3d;
				}
				else if (ISPointInHoles(flooreehs, temp))
				{
					tmpendEndTypes.end.SetendNormal(vectorZ);
					tmpendEndTypes.end.SetType(PIT::PITRebarEndType::Type::kNone);
					endPt = temp;
				}
				else
				{
					endtemp_point3d = endPt;
				}
			}

			//�ж��Ƿ�ê��
			EditElementHandle* eeh = &Eleeh;
			//�ж���ʼ�����ʼ��ê��λ���Ƿ�ê��
			JudgeBarLinesLegitimate(nowVec, alleehs, tmpendEndTypes, lineEeh, eeh, str_vector, matrix, strMoveDis, str_extend_lenth, strtemp_point3d, str_Ptr, flag_str);
			//β�˵��β�˵�λ���Ƿ�ê��
			JudgeBarLinesLegitimate(nowVec, alleehs, tmpendEndTypes, lineEeh, eeh, end_vector, matrix, endMoveDis, end_extend_lenth, endtemp_point3d, end_Ptr, flag_end);
		}

		DPoint3d strPttepm = strPt, endPttepm = endPt;

		GetIntersectPointsWithHoles(tmppts, m_useHoleehs, strPt, endPt, dSideCover, matrix);

		vector<DPoint3d> aro_tmppts;//��Χʵ��׶�����
		vector<DPoint3d> uniquePoints;
		GetIntersectPointsWithHoles(aro_tmppts, m_around_ele_holeEehs, strPt, endPt, dSideCover, matrix);


		for (size_t j = 0; j < aro_tmppts.size(); j++) {
			bool isUnique = true;

			for (size_t i = 0; i < uniquePoints.size(); i++) {
				// �����Ψһ��
				if (uniquePoints[i].x + 1 > aro_tmppts[j].x &&
					uniquePoints[i].x - 1 < aro_tmppts[j].x &&
					uniquePoints[i].y + 1 > aro_tmppts[j].y &&
					uniquePoints[i].y - 1 < aro_tmppts[j].y &&
					uniquePoints[i].z + 1 > aro_tmppts[j].z &&
					uniquePoints[i].z - 1 < aro_tmppts[j].z) {
					isUnique = false; // �ҵ������Ƶĵ�
					break; // ������ǰ�˳�ѭ��
				}
			}

			if (isUnique) {
				uniquePoints.push_back(aro_tmppts[j]); // ���Ψһ�ĵ�
			}
		}

		//�жϵװ�ֽ�ê�����Ƿ������׶���������˰�,��������������������Ҫ��ת��ǰ�˲�����ֻ������ֱ����
		if (tmppts.empty() && !aro_tmppts.empty()) {
			for (auto point : uniquePoints) {
				//PITCommonTool::CPointTool::DrowOnePoint(point, 1, 3); // ��
				tmppts.push_back(point);
			}
		}

		//���ݹ�ܿ׶���ĵ㽫�㰴������࣬����һ�飬�ȵ��ֽ��ߵ���β��
		map<int, DPoint3d> map_pts;
		bool isStr = false;
		for (DPoint3d pt : tmppts)
		{
			if (ExtractFacesTool::IsPointInLine(pt, strPt, endPt, ACTIVEMODEL, isStr))
			{
				int dis = (int)strPt.Distance(pt);
				if (map_pts.find(dis) != map_pts.end())
				{
					dis = dis + 1;
				}
				map_pts[dis] = pt;
			}
		}
		if (map_pts.find(0) != map_pts.end())
		{
			if (flag_str == 2)
			{
				map_pts[1] = str_Ptr;
			}
			else
				map_pts[1] = strPt;
		}
		else
		{
			if (flag_str == 2)
			{
				map_pts[0] = str_Ptr;
			}
			else
				map_pts[0] = strPt;
		}
		int dis = (int)strPt.Distance(endPt);
		if (map_pts.find(dis) == map_pts.end())
		{
			if (flag_end == 2)
			{
				map_pts[dis] = end_Ptr;
			}
			else
				map_pts[dis] = endPt;
		}
		else
		{
			dis = dis + 1;
			if (flag_end == 2)
			{
				map_pts[dis] = end_Ptr;
			}
			else
				map_pts[dis] = endPt;
		}
		for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
		{
			PITRebarEndTypes		tmpendTypes;
			CVector3D  nowVectmp;
			PITRebarCurve rebar;
			RebarVertexP vex;
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(itr->second);
			vex->SetType(RebarVertex::kStart);
			if (strPttepm.Distance(itr->second) < 10)
			{
				if (flag_str == 3)
				{
					tmpendTypes.beg = tmpendEndTypes_2.beg;
				}
				else
					tmpendTypes.beg = tmpendEndTypes.beg;
			}
			else
			{
				//nowVectmp = tmpendEndTypes.beg.GetendNormal();
				tmpendTypes.beg = Hol;
				//tmpendTypes.beg.SetendNormal(nowVectmp.Negate());
			}
			tmpendTypes.beg.SetptOrgin(itr->second);
			map<int, DPoint3d>::iterator itrplus = ++itr;
			if (itrplus == map_pts.end())
			{
				break;
			}
			if (endPttepm.Distance(itrplus->second) < 10)
			{
				if (flag_end == 3)
				{
					tmpendTypes.end = tmpendEndTypes_2.end;
				}
				else
					tmpendTypes.end = tmpendEndTypes.end;
			}
			else
			{
				//nowVectmp = tmpendEndTypes.end.GetendNormal();
				tmpendTypes.end = Hol;
				//tmpendTypes.end.SetendNormal(nowVectmp.Negate());
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

	return true;
}

RebarSetTag * STWallRebarAssembly::MakeRebars(ElementId & rebarSetId, BrStringCR sizeKey,
	const vector<BarLinesdata>& barLinesData,
	double strOffset, double endOffset, int level, int grade, DgnModelRefP modelRef,
	int rebarLineStyle, int rebarWeight)
{
	for (auto it : barLinesData)
	{
		//���ݱ����㡢�ӳ�ֵ���»��ƻ����ֽ��ߴ�
		//ReCalBarLineByCoverAndDis(it);
		MSElementDescrP barline = it.barline;
		MSElementDescrP path = it.path;
		//mdlElmdscr_add(barline);
		//mdlElmdscr_add(path);
	}

	if (barLinesData.size() == 0)
		return nullptr;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	double diameter = barLinesData.at(0).diameter / uor_per_mm;
	double spacing = barLinesData.at(0).spacing / uor_per_mm;
	bool isInSide = barLinesData.at(0).isInSide;
	double adjustedSpacing = spacing;

	EditElementHandle ehWall(GetSelectedElement(), GetSelectedModel());//����

	std::vector<EditElementHandle*> upflooreehs;
	std::vector<EditElementHandle*> flooreehs;
	std::vector<EditElementHandle*> Walleehs;
	std::vector<EditElementHandle*> alleehs;
	m_around_ele_holeEehs.clear();
	for (IDandModelref IdMode : m_walldata.floorID)//�м��
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//������ҪԪ����Χ�Ŀ׶�
			EditElementHandle aroEleeh;
			EFT::GetSolidElementAndSolidHoles(*eeh, aroEleeh, m_around_ele_holeEehs);

			if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
				eeh->GetElementDescrP();
				flooreehs.push_back(eeh);
				alleehs.push_back(eeh);
			}
		}
	}
	for (IDandModelref IdMode : m_walldata.downfloorID)//�װ�
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//������ҪԪ����Χ�Ŀ׶�
			EditElementHandle aroEleeh;
			EFT::GetSolidElementAndSolidHoles(*eeh, aroEleeh, m_around_ele_holeEehs);

			if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
				eeh->GetElementDescrP();
				//downflooreehs.push_back(eeh);
				//Walleehs.push_back(eeh);
				alleehs.push_back(eeh);
			}
		}
	}
	for (IDandModelref IdMode : m_walldata.upfloorID)//����
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//������ҪԪ����Χ�Ŀ׶�
			EditElementHandle aroEleeh;
			EFT::GetSolidElementAndSolidHoles(*eeh, aroEleeh, m_around_ele_holeEehs);

			if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
				eeh->GetElementDescrP();
				upflooreehs.push_back(eeh);
				//Walleehs.push_back(eeh);
				alleehs.push_back(eeh);
			}
		}
	}
	for (IDandModelref IdMode : m_walldata.wallID)//����ǽ(ֻ�ж���ê����Ҫ����ǽ)
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//������ҪԪ����Χ�Ŀ׶�
			EditElementHandle aroEleeh;
			EFT::GetSolidElementAndSolidHoles(*eeh, aroEleeh, m_around_ele_holeEehs);

			if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
				eeh->GetElementDescrP();
				upflooreehs.push_back(eeh);
				Walleehs.push_back(eeh);
				alleehs.push_back(eeh);
			}
		}
	}

	EditElementHandle *eeh = new EditElementHandle();//���廹��Ҫ��������
	eeh->Duplicate(ehWall);
	ISolidKernelEntityPtr entityPtr;
	if (ehWall.IsValid() && eeh->IsValid())
	{
		if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
		{
			SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
			eeh->GetElementDescrP();
			upflooreehs.push_back(eeh);
			//Walleehs.push_back(eeh);
			//downflooreehs.push_back(eeh);
			alleehs.push_back(eeh);
		}
	}

	vector<PIT::PITRebarCurve> rebarCurves;
	for (auto it : barLinesData)
	{
		//����˲���ʽ
		double strMoveDis = GetSideCover() * uor_per_mm + diameter / 2 * uor_per_mm + it.extstrdiameter;
		double endMoveDis = GetSideCover() * uor_per_mm + diameter / 2 * uor_per_mm + it.extenddiameter;
		PIT::PITRebarEndTypes pitRebarEndTypes;
		CalRebarEndTypes(it, sizeKey, pitRebarEndTypes, modelRef);
		double extendstrDis = it.extendstrDis;
		double extendendDis = it.extendendDis;
		if (pitRebarEndTypes.beg.GetType() != PITRebarEndType::Type::kNone)
		{
			extendstrDis -= diameter / 2 * uor_per_mm;
		}
		if (pitRebarEndTypes.end.GetType() != PITRebarEndType::Type::kNone)
		{
			extendendDis -= diameter / 2 * uor_per_mm;
		}

		//����ֽ���
		vector<vector<DPoint3d>> allLines;
		//mdlElmdscr_add(it.barline);
		//mdlElmdscr_add(it.path);
		ExtractFacesTool::GetVecToCurve(allLines, it.barline, it.path, spacing,
			it.strDis / uor_per_mm, it.endDis / uor_per_mm);
		if (allLines.size() == 0)
			continue;
		for (auto ptsIt : allLines)
		{
			makaRebarCurve(ptsIt, extendstrDis, extendendDis, diameter * uor_per_mm, strMoveDis, endMoveDis, isInSide, pitRebarEndTypes, rebarCurves, upflooreehs, flooreehs, Walleehs, alleehs);
		}
	}
	if (m_vecRebarPtsLayer.size() > 1)
	{
		for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
		{
			int n = m + 1;
			RebarPoint rbPt;
			rbPt.Layer = GetvecRebarLevel().at(m_nowlevel);
			rbPt.iIndex = m_nowlevel;
			rbPt.sec = 0;
			rbPt.vecDir = GetvecDir().at(m_nowlevel);
			rbPt.ptstr = m_vecRebarPtsLayer.at(m);
			rbPt.ptend = m_vecRebarPtsLayer.at(n);
			rbPt.DataExchange = GetvecDataExchange().at(m_nowlevel);
			g_vecRebarPtsNoHole.push_back(rbPt);
			m++;
		}
	}
	FreeAll(alleehs);
	upflooreehs.clear(); flooreehs.clear(); Walleehs.clear();
	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kBend);
	endTypeEnd.SetType(RebarEndType::kBend);

	int numRebar = rebarCurves.size();
	RebarSymbology symb;

	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);

	vector<vector<DPoint3d>> vecStartEnd;
	int index = 0;
	for (PITRebarCurve rebarCurve : rebarCurves)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (ISPointInHoles(m_useHoleehs, midPos))
		{
			if (ISPointInHoles(m_useHoleehs, ptstr) && ISPointInHoles(m_useHoleehs, ptend))
			{
				continue;
			}
		}
		if (ISPointInHoles(m_Negs, midPos))
		{
			if (ISPointInHoles(m_Negs, ptstr) && ISPointInHoles(m_Negs, ptend))
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
		/*EditElementHandle eeh;
		LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		eeh.AddToModel();
	*/
		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(index, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(false);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (isInSide)
			{
				Stype = "inner";
			}
			else
			{
				Stype = "out";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing, ACTIVEMODEL);

			ElementPropertiesSetterPtr propEle = ElementPropertiesSetter::Create();
			propEle->SetLinestyle(rebarLineStyle, NULL);
			propEle->SetWeight(rebarWeight);
			propEle->Apply(tmprebar);
			tmprebar.ReplaceInModel(oldref);

		}
		index++;
		vecStartEnd.push_back(linePts);
	}

	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing);
	CString spacingstring;
	spacingstring.Format(_T("%lf"), spacing);
	setdata.SetSpacingString(spacingstring);
	setdata.SetAverageSpacing(adjustedSpacing);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����
	rebarSet->SetSetData(setdata);
	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);

	return tag;
}

void STWallRebarAssembly::CalRebarEndTypes(const BarLinesdata & data, BrStringCR sizeKey, PIT::PITRebarEndTypes & pitRebarEndTypes, DgnModelRefP modelRef)
{
	//����ê�뷽����ֽ��ļн���˲�����
	auto GetEndType = [&](const CVector3D& vec1, const CVector3D& vec2, PIT::PITRebarEndType& endType)->int {
		double angle = vec1.AngleTo(vec2);
		endType.Setangle(angle);
		if (COMPARE_VALUES_EPS(angle, PI / 2, 0.1) == 0) //90��
			return PIT::PITRebarEndType::Type::kBend;
		if (COMPARE_VALUES_EPS(angle, 3 / 4 * PI, 0.1) == 0) //135��
			return PIT::PITRebarEndType::Type::kCog;
		if (COMPARE_VALUES_EPS(angle, 0, 0.1) == 0) //180����ê
			return PIT::PITRebarEndType::Type::kHook;
		if (COMPARE_VALUES_EPS(angle, PI, 0.1) == 0) //ֱê
			return PIT::PITRebarEndType::Type::kLap;
		else
			return PIT::PITRebarEndType::Type::kCustom;
	};

	CVector3D strVec = data.vecstr;
	CVector3D endVec = data.vecend;
	CVector3D HolVec = data.vecHoleehs;
	strVec.Normalize();
	endVec.Normalize();
	//��ȡê�����
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	//Ĭ��û�ж˲���ʽ
	PIT::PITRebarEndType strEndType, endEndType, HolEndType;
	strEndType.SetType(PIT::PITRebarEndType::Type::kNone);
	//����ֽ�����β
	DPoint3d lineStrPt = { 0,0,0 }, lineEndPt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&lineStrPt, nullptr, &lineEndPt, nullptr, data.barline, modelRef);
	//������β�˲���ʽ
	if (!strVec.IsEqual(Dpoint3d::FromZero()))
	{
		CVector3D lineVec = lineEndPt - lineStrPt;
		lineVec.Normalize();
		int endType = GetEndType(strVec, lineVec, strEndType);
		strEndType.SetType(PIT::PITRebarEndType::Type(endType));
		strEndType.SetbendLen(data.strMG - bendRadius);// - diameter / 2
		strEndType.SetbendRadius(bendRadius);
		strEndType.SetendNormal(strVec);
	}
	if (!endVec.IsEqual(Dpoint3d::FromZero()))
	{
		CVector3D lineVec = lineStrPt - lineEndPt;
		lineVec.Normalize();
		int endType = GetEndType(endVec, lineVec, endEndType);
		endEndType.SetType(PIT::PITRebarEndType::Type(endType));
		endEndType.SetbendLen(data.endMG - bendRadius);//  - diameter / 2
		endEndType.SetbendRadius(bendRadius);
		endEndType.SetendNormal(endVec);
	}
	if (!HolVec.IsEqual(Dpoint3d::FromZero()))
	{
		CVector3D lineVec = lineEndPt - lineStrPt;
		lineVec.Normalize();
		int endType = GetEndType(HolVec, lineVec, HolEndType);
		HolEndType.SetType(PIT::PITRebarEndType::Type(endType));
		HolEndType.SetbendLen(data.holMG - bendRadius - diameter / 2);
		HolEndType.SetbendRadius(bendRadius);
		HolEndType.SetendNormal(HolVec);
	}
	Hol = HolEndType;
	pitRebarEndTypes = { strEndType,endEndType };
}

/*
* @desc:		���ݶ��װ����¼�����������
* @param[in]	strPt �ֽ������
* @param[in]	endPt �ֽ����ص�
* @param[in]	strMoveLen �����ƶ����룬�����乳�Ǳ����㣬���乳�Ǳ�����+�ֽ�뾶
* @param[in]	endMoveLen �յ���ƶ����룬�����乳�Ǳ����㣬���乳�Ǳ�����+�ֽ�뾶
* @param[out]	extendStrDis �����������
* @param[out]	extendEndDis �յ���������
* @author	Hong ZhuoHui
* @Date:	2023/09/20
*/
void STWallRebarAssembly::ReCalExtendDisByTopDownFloor(const DPoint3d & strPt, const DPoint3d & endPt, double strMoveLen,
	double endMoveLen, double & extendStrDis, double & extendEndDis, bool isInSide)
{
	//��ȡǽ��
	//extendStrDis = 0; extendEndDis = 0;
	EditElementHandle wallEeh(GetSelectedElement(), GetSelectedModel());
	if (!wallEeh.IsValid())
		return;
	DPoint3d wallLowPt = { 0,0,0 }, wallHighPt = { 0,0,0 };
	mdlElmdscr_computeRange(&wallLowPt, &wallHighPt, wallEeh.GetElementDescrP(), nullptr);
	vector<DPoint3d> Pts;
	DVec3d strVec1 = strPt - endPt;
	strVec1.Normalize();
	strVec1.ScaleToLength(40 * UOR_PER_MilliMeter);
	DVec3d endVec1 = endPt - strPt;
	endVec1.Normalize();
	endVec1.ScaleToLength(40 * UOR_PER_MilliMeter);
	DPoint3d Pt1 = strPt, Pt2 = endPt;
	Pt1.Add(strVec1);
	Pt2.Add(endVec1);
	//PITCommonTool::CPointTool::DrowOneLine(strPt, endPt, 2);
	GetIntersectPointsWithOldElm(Pts, &wallEeh, Pt1, Pt2);
	//PITCommonTool::CPointTool::DrowOnePoint(Pts[0], 1, 6);
	//PITCommonTool::CPointTool::DrowOnePoint(Pts[1], 1, 1);
	//1.������С�ĵ�����ĵ�
	Dpoint3d minPt = { INT_MAX, INT_MAX, INT_MAX }, maxPt = { INT_MIN, INT_MIN, INT_MIN };
	auto calMinMaxPt = [&](IDandModelref floor) {
		EditElementHandle floorEeh(floor.ID, floor.tModel);
		if (!floorEeh.IsValid())
			return;
		DPoint3d lowPt = { 0,0,0 }, highPt = { 0,0,0 };
		mdlElmdscr_computeRange(&lowPt, &highPt, floorEeh.GetElementDescrP(), nullptr);
		if (COMPARE_VALUES_EPS(lowPt.x, minPt.x, 1e-6) == -1)
			minPt.x = lowPt.x;
		if (COMPARE_VALUES_EPS(lowPt.y, minPt.y, 1e-6) == -1)
			minPt.y = lowPt.y;
		if (COMPARE_VALUES_EPS(lowPt.z, minPt.z, 1e-6) == -1)
			minPt.z = lowPt.z;
		if (COMPARE_VALUES_EPS(highPt.x, maxPt.x, 1e-6) == 1)
			maxPt.x = highPt.x;
		if (COMPARE_VALUES_EPS(highPt.y, maxPt.y, 1e-6) == 1)
			maxPt.y = highPt.y;
		if (COMPARE_VALUES_EPS(highPt.z, maxPt.z, 1e-6) == 1)
			maxPt.z = highPt.z;
	};
	for (auto it : m_walldata.upfloorID)
	{
		calMinMaxPt(it);
	}
	for (auto it : m_walldata.downfloorID)
	{
		calMinMaxPt(it);
	}

	//2.����С������ͶӰ���ֽ�����
	DPoint3d minProPt = minPt, maxProPt = maxPt;
	mdlVec_projectPointToLine(&minProPt, nullptr, &minPt, &strPt, &endPt);
	mdlVec_projectPointToLine(&maxProPt, nullptr, &maxPt, &strPt, &endPt);

	//3.���ݵ�ͶӰ��ľ������ֽ����ӳ�����
	double strMinDis = strPt.Distance(minProPt);
	double endMinDis = endPt.Distance(minProPt);
	double strMaxDis = strPt.Distance(maxProPt);
	double endMaxDis = endPt.Distance(maxProPt);
	double strMoveDis = 0, endMoveDis = 0;
	if (COMPARE_VALUES_EPS(strMinDis, endMinDis, 1e-6) == -1)
	{
		strMoveDis = strMinDis;
		endMoveDis = endMaxDis;
	}
	else
	{
		strMoveDis = strMaxDis;
		endMoveDis = endMinDis;
	}

	//4.�ӳ��ֽ���
	DPoint3d newStrPt = strPt, newEndPt = endPt;
	//4.1������β�ƶ�����
	DVec3d strVec = strPt - endPt;
	strVec.Normalize();
	strVec.ScaleToLength(strMoveDis + strMoveLen); //movelen�������������ģ���Ϊ�ֽ�����б�ģ�ͶӰ���е����
	DVec3d endVec = endPt - strPt;
	endVec.Normalize();
	endVec.ScaleToLength(endMoveDis + endMoveLen);
	//4.2�ƶ���β��
	newStrPt.Add(strVec*1.5);
	newEndPt.Add(endVec*1.5);
	//PITCommonTool::CPointTool::DrowOneLine(newStrPt, newEndPt, 11);
	//5.�����а��󽻣��õ���ʼ��˵Ľ�����յ�˵Ľ��㼯��
	vector<DPoint3d> interStrPts, interEndPts, allPts; //����
	int  numAnchors = 0;//ê������жϱ�׼
	bool isStrRecorded = false; // ��־����Ƿ�������
	bool isEndRecorded = false; // ��־�յ��Ƿ�������
	auto calInterPts = [&](IDandModelref floor, bool islsay = true) {
		EditElementHandle floorEeh(floor.ID, floor.tModel);
		if (!floorEeh.IsValid())
			return;

		//5.1.1 ���˲���ǽ��Χ�ڵİ�		
		DPoint3d lowPt = { 0,0,0 }, highPt = { 0,0,0 };
		mdlElmdscr_computeRange(&lowPt, &highPt, floorEeh.GetElementDescrP(), nullptr);
		DRange3d  vecRange;
		vecRange.low = wallLowPt;
		vecRange.high = wallHighPt;
		strVec.Normalize();
		int strRecord = 2; // ������Ľ������
		int endRecord = 2; // ������Ľ������
		if (islsay)
		{
			if ((COMPARE_VALUES_EPS(lowPt.z, wallLowPt.z - 50 * UOR_PER_MilliMeter, 1e-6) == 1 && //����͵���ǽ֮��
				COMPARE_VALUES_EPS(highPt.z, wallHighPt.z + 50 * UOR_PER_MilliMeter, 1e-6) == -1) &&
				COMPARE_VALUES_EPS(wallHighPt.z, highPt.z, 1e-6) == 0) //����ߵ���ǽ֮��
			{
				if (vecRange.IsContainedXY(lowPt) && wallLowPt.DistanceXY(lowPt) > 10 && COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0) //����͵㲻��ǽ��Χ,����ߵ㲻��ǽ��Χ
				{
					return;
				}

				if (vecRange.IsContainedXY(highPt) && wallHighPt.DistanceXY(highPt) > 10 && COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0) //����͵㲻��ǽ��Χ,����ߵ㲻��ǽ��Χ
				{
					return;
				}
			}
			MSElementDescrP have_intersect = nullptr;//ʵ������ཻ
			if (!PITCommonTool::CSolidTool::SolidBoolWithFace(have_intersect, wallEeh.GetElementDescrP(), floorEeh.GetElementDescrP(), BOOLOPERATION::BOOLOPERATION_INTERSECT)
				&& COMPARE_VALUES_EPS(lowPt.z, wallHighPt.z - 50 * UOR_PER_MilliMeter, 1e-6) == -1
				&& COMPARE_VALUES_EPS(highPt.z, wallHighPt.z, 1e-6) == 1
				&& highPt.z - wallHighPt.z < 55 * UOR_PER_MilliMeter)//��������͵���ǽ֮�䵫�����ߵ㲻��ǽ֮���Ұ��ǽ�����
			{
				return;
			}
		}
		/*PITCommonTool::CPointTool::DrowOnePoint(wallLowPt, 1, 5);
		PITCommonTool::CPointTool::DrowOnePoint(wallHighPt, 1, 6);
		PITCommonTool::CPointTool::DrowOnePoint(lowPt, 1, 1);
		PITCommonTool::CPointTool::DrowOnePoint(highPt, 1, 2);*/


		if ((COMPARE_VALUES_EPS(lowPt.z, wallLowPt.z + 80 * UOR_PER_MilliMeter, 1e-6) == -1 ||
			COMPARE_VALUES_EPS(lowPt.z, wallHighPt.z - 80 * UOR_PER_MilliMeter, 1e-6) == 1) && //����͵㲻��ǽ֮��
			(COMPARE_VALUES_EPS(highPt.z, wallLowPt.z + 80 * UOR_PER_MilliMeter, 1e-6) == -1 ||
				COMPARE_VALUES_EPS(highPt.z, wallHighPt.z - 80 * UOR_PER_MilliMeter, 1e-6) == 1) &&//����ߵ㲻��ǽ֮��
			COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0)//�ֽ��߷���Z��Ϊ1
		{

			//5.1.1 �õ������İ�Χ��ͶӰ���ֽ����ϵĵ�
			DPoint3d floorMinProPt = lowPt, floorMaxProPt = highPt;
			mdlVec_projectPointToLine(&floorMinProPt, nullptr, &lowPt, &strPt, &endPt);
			mdlVec_projectPointToLine(&floorMaxProPt, nullptr, &highPt, &strPt, &endPt);
			//5.1.2 ������ߵķ���͸ֽ��߷����ϵ���õ������ڸֽ��߷����ϵ���ʼ���յ�
			DPoint3d floorStrPt = floorMinProPt, floorEndPt = floorMaxProPt;
			DVec3d vec1 = strPt - endPt; vec1.Normalize();
			DVec3d vec2 = floorMinProPt - floorMaxProPt; vec2.Normalize();
			if (vec1.DotProduct(vec2) < 0) //��ͬ����
			{
				floorStrPt = floorMaxProPt;
				floorEndPt = floorMinProPt;
			}
			//5.1.3 ������ߵ���ʼ��͸ֽ��ߵ��յ�ľ��� ���� ���ߵ��յ�͸ֽ��ߵ����ľ��� С�ڸֽ��ߵ�1/5 ���˰�
			double dis = strPt.Distance(endPt) / 3;
			if (COMPARE_VALUES_EPS(floorStrPt.Distance(endPt), dis, 1e-6) == -1 ||
				COMPARE_VALUES_EPS(floorEndPt.Distance(strPt), dis, 1e-6) == -1)
			{
				return;
			}
		}
		//5.2 ������彻��
		vector<DPoint3d> interPts;
		//PITCommonTool::CPointTool::DrowOneLine(newStrPt, newEndPt, 1);
		//GetIntersectPointsWithOldElm(interPts, &floorEeh, newStrPt, newEndPt);
		//5.1.2 ���˲���ǽ��Χ�ڵ�ǽ
		//����ָ��Ԫ����������Ԫ�صķ�Χ��
		DPoint3d range_data_low = { 0,0,0 }, range_data_high = { 0,0,0 };
		EditElementHandle wallEeh(GetSelectedElement(), GetSelectedModel());//��ȡѡ��model�Ĳ�����Χ
		DPoint3d wallLowPt = { 0,0,0 }, wallHighPt = { 0,0,0 };
		mdlElmdscr_computeRange(&wallLowPt, &wallHighPt, wallEeh.GetElementDescrP(), nullptr);
		for (int i = 0; i < m_walldata.cutWallfaces.size(); i++)//����Χ��ǽ�ĵ������ɸѡ
		{
			DPoint3d wallLow_Pt = { 0,0,0 }, wallHigh_Pt = { 0,0,0 };
			EditElementHandle wall_Eeh(m_walldata.wallID[i].ID, m_walldata.wallID[i].tModel);
			mdlElmdscr_computeRange(&wallLow_Pt, &wallHigh_Pt, wall_Eeh.GetElementDescrP(), nullptr);//��ȡѡ����Χǽ�Ĳ�����Χ

			if (COMPARE_VALUES(wallLow_Pt.z, wallLowPt.z) == 1 && COMPARE_VALUES(wallHigh_Pt.z, wallHighPt.z) == -1)//�����Χǽ��ѡ��ǽ��range��Χ��
			{
				range_data_low = wallLow_Pt;
				range_data_high = wallHigh_Pt;
			}
			//�ж��Ƿ���Ҫ�����жϣ������Χǽ��ѡ��ǽ��range��Χ�ڣ�����Χ�İ�ֻ��Ҫê��һ�Σ������Զ��ê��
			if ((numAnchors != 2 && (COMPARE_VALUES(strPt.z, range_data_low.z) == 1 && COMPARE_VALUES(endPt.z, range_data_low.z) == 1)
				&& (COMPARE_VALUES(strPt.z, range_data_high.z) == -1 && COMPARE_VALUES(endPt.z, range_data_high.z) == -1)) ||
				(numAnchors != 2 && (COMPARE_VALUES(strPt.x, range_data_low.x) == 1 && COMPARE_VALUES(endPt.x, range_data_low.x) == 1)
					&& (COMPARE_VALUES(strPt.x, range_data_high.x) == -1 && COMPARE_VALUES(endPt.x, range_data_high.x) == -1)))
			{
				numAnchors = 1;
			}
		}
		if (numAnchors == 1)//����ê��
		{
			GetIntersectPointsWithHole(interPts, &floorEeh, newStrPt, newEndPt);
			numAnchors = 2;
		}
		if (numAnchors == 0)//���Զ��ê��
		{
			GetIntersectPointsWithHole(interPts, &floorEeh, newStrPt, newEndPt);
			//PITCommonTool::CPointTool::DrowOnePoint(newStrPt, 1, 3);//��
			//PITCommonTool::CPointTool::DrowOnePoint(newEndPt, 1, 4);//��
		}
		//5.3 �������н��㣬���������һ����ֽ��߶˵㼫���ĵ㣬���������壨ǽ��֮��û�н���
		bool isValid = false;
		if (Pts.size() == 0)//�����ǽû�н���˵�������겿�֣�����Ҫ���й���
		{
			isValid = true;
		}
		else
		{
			DVec3d rebarVec = DVec3d::FromZero();
			if (interPts.size() >= 2)
				rebarVec.DifferenceOf(interPts[0], interPts[interPts.size() - 1]);
			rebarVec.Normalize();
			bool isHorizon = abs(rebarVec.DotProduct(DVec3d::UnitZ())) < 0.1;
			// ��ȡ¥����
			double thickness = GetFloorThickness(floorEeh) * UOR_PER_MilliMeter;
			double dSideCover = GetSideCover() * UOR_PER_MilliMeter;
			// �������ľ������򽻵�
			std::sort(interPts.begin(), interPts.end(), [&](const DPoint3d& a, const DPoint3d& b) {
				return COMPARE_VALUES_EPS(strPt.Distance(a), strPt.Distance(b), UOR_PER_MilliMeter) < 0;
			});

			// ���ˮƽ�ֽ����쵽���������߼�
			if (islsay && isInSide && interPts.size() >= 2 && isHorizon) {
				// �������ڽ���֮��ľ��룬���Ʋ�����¥����
				for (size_t i = 0; i < interPts.size() - 1; ++i) {
					double distBetweenPts = interPts[i].Distance(interPts[i + 1]);
					if (COMPARE_VALUES_EPS(distBetweenPts, thickness, UOR_PER_MilliMeter) > 0) {
						// ����������볬��¥���ȣ������ڶ�������λ�ã��ҽ�������
						DVec3d direction = interPts[i + 1] - interPts[i];
						direction.Normalize();
						direction.ScaleToLength(thickness);
						interPts[i + 1] = interPts[i];
						interPts[i + 1].Add(direction);
						break;
					}
				}
			}
			if (islsay && !isInSide && interPts.size() >= 2 && isHorizon)
			{
				// ��ȡ������йؼ���
				vector<DPoint3d> vec_ptBoundary;
				vector<EditElementHandle*> allFaces;
				PITCommonTool::CFaceTool::GetElementAllFaces(floorEeh, allFaces);
				for (auto facePtr : allFaces)
				{
					if (!facePtr || !facePtr->IsValid())
						continue;

					EditElementHandle& faceEeh = *facePtr;
					MSElementDescrP faceDescr = faceEeh.GetElementDescrP();
					if (!faceDescr)
						continue;

					vector<DPoint3d> faceBoundary;
					ExtractBoundaryPoints(faceDescr, faceBoundary);
					vec_ptBoundary.insert(vec_ptBoundary.end(), faceBoundary.begin(), faceBoundary.end());
				}
				
				for (DPoint3d boundaryPt : vec_ptBoundary)
				{
					// ���ؼ���ͶӰ���ֽ�����
					DPoint3d projectedPt;
					double outFractionP;
					mdlVec_projectPointToLine(&projectedPt, &outFractionP, &boundaryPt, &interPts.front(), &interPts.back());

					// ����ؼ�����ͶӰ����Z�����ϵľ���
					double distZ = abs(boundaryPt.z - projectedPt.z);

					// ���ͶӰ���Ƿ��ڸֽ��ߵ���ֹ�㷶Χ��
					bool isWithinRange = (COMPARE_VALUES(outFractionP, 0) >= 0
						&& COMPARE_VALUES(outFractionP, 1) <= 0);

					// �������С�ڱ������ȣ���ͶӰ������ֹ�㷶Χ�ڣ������ڶ�������λ�ã��ҽ������졣
					// ���ͶӰ���ڹؼ���ʵ�ʾ�����󣬿����ǿ׶�֮���Ӱ�죬ֻ����һ����Χ�ڵĹؼ���
					if (COMPARE_VALUES_EPS(distZ, dSideCover, 1 * UOR_PER_MilliMeter) < 0 && isWithinRange
						&& COMPARE_VALUES(boundaryPt.Distance(projectedPt), 3 * dSideCover) < 0)
					{
						DVec3d direction = interPts[1] - interPts[0];
						direction.Normalize();
						direction.ScaleToLength(thickness);
						interPts[1] = interPts[0];
						interPts[1].Add(direction);
						break;
					}
				}
			}
			double totalDis = strPt.Distance(endPt);
			for (auto it : interPts)
			{
				//PITCommonTool::CPointTool::DrowOnePoint(it, 1, 3);
				double strDis = strPt.Distance(it);
				double endDis = endPt.Distance(it);
				if (COMPARE_VALUES_EPS(strDis, 50 * UOR_PER_MilliMeter, 1e-6) <= 0 || //��ʼ����˵㼫��
					COMPARE_VALUES_EPS(endDis, 50 * UOR_PER_MilliMeter, 1e-6) <= 0 ||	//�յ���˵㼫��
					COMPARE_VALUES_EPS(totalDis, strDis + endDis, 1) == 0 ||//�����ڸֽ����м�
					vecRange.IsContainedXY(it))//����ǽXYƽ��ķ�Χ��
				{
					isValid = true;
					break;
				}
				// ������������齻���Ƿ�ӽ� interStrPts �� interEndPts����Ϊ�ж�ê����Ƿ���Կ�ʵ��
				// ��齻���Ƿ�ӽ����˵Ľ��㼯��
				for (auto strPtInters : interStrPts) {
					if (islsay && COMPARE_VALUES_EPS(strPtInters.Distance(it), 50 * UOR_PER_MilliMeter, 1e-6) <= 0) {
						isValid = true;
						break;
					}
				}

				if (isValid) {
					break;  // �����ȷ���߿������죬����ѭ��
				}

				// ��齻���Ƿ�ӽ��յ�˵Ľ��㼯��
				for (auto endPtInters : interEndPts) {
					if (islsay && COMPARE_VALUES_EPS(endPtInters.Distance(it), 50 * UOR_PER_MilliMeter, 1e-6) <= 0) {
						isValid = true;
						break;
					}
				}

				if (isValid) {
					break;  // �����ȷ���߿������죬����ѭ��
				}
			}
		}
		if (!isValid)
			return;
		endVec.Normalize();
		//5.4 ��¼��
		if (Pts.size() == 0 && COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0)
		{
			for (int i = 0; i <= (int)interPts.size() - 1; i++)
			{
				for (int j = 0; j < (int)interPts.size() - 1 - i; j++)
				{
					CVector3D vec = CVector3D::From(interPts[j + 1].x - interPts[j].x, interPts[j + 1].y - interPts[j].y, interPts[j + 1].z - interPts[j].z);
					vec.Normalize();
					if (!(COMPARE_VALUES_EPS(endVec.x, vec.x, 0.1) == 0 &&
						COMPARE_VALUES_EPS(endVec.y, vec.y, 0.1) == 0 &&
						COMPARE_VALUES_EPS(endVec.z, vec.z, 0.1) == 0))
					{
						DPoint3d Tmp = interPts[j];
						interPts[j] = interPts[j + 1];
						interPts[j + 1] = Tmp;
					}
				}
			}
			for (int i = 0; i <= (int)interPts.size() - 1; i++)
			{
				if (((i + 1) & 0x01))
					interStrPts.push_back(interPts[i]);
				else
					interEndPts.push_back(interPts[i]);
			}
		}
		else
		{
			// �����㲢���䵽���˻��յ��
			auto processIntersectionPoints = [&](DPoint3d point, vector<DPoint3d>& vec_interPts, bool& isRecorded, int& record) -> bool{
				if (!islsay)
				{
					if (!isRecorded)//ǽʵ����δ�������壬��������
						vec_interPts.push_back(point);
					return false;
				}
				if (vec_interPts.size() < 2 || record == 1)//���������壬���°����һ�����㣬�������죬���¼Ϊ��
				{
					vec_interPts.push_back(point);
					isRecorded = true;
					return false;
				}
				if (!isRecorded)//δ�������壬��������
				{
					if (record == 2)//ǽ��ǽ˳����Ҫ��������������㣬�����¼Ϊ��
						record--;
					vec_interPts.push_back(point);
					isRecorded = true;
					return false;
				}
				return true;
			};
			for (auto it : interPts)
			{
				double strDis = strPt.Distance(it);
				double endDis = endPt.Distance(it);
				allPts.push_back(it);
				if (COMPARE_VALUES_EPS(strDis, endDis, 1e-6) == -1)
				{
					if (!processIntersectionPoints(it, interStrPts, isStrRecorded, strRecord))
						continue;
					
					// ��������壬ѡ����İ�
					interPtsSort(interStrPts, endPt);
					double tempStrDis = strPt.Distance(interStrPts.back());
					if (COMPARE_VALUES_EPS(strDis, tempStrDis, 1e-6) == -1)//�µĽ����ڸ����İ�
					{
						if (strRecord == 2)//����������ʱ��գ���Ҫ�����°����������
							interStrPts.clear();
						interStrPts.push_back(it);
						strRecord--;
					}
				}
				else
				{
					if (!processIntersectionPoints(it, interEndPts, isEndRecorded, endRecord))
						continue;
					
					// ��������壬ѡ����İ�
					interPtsSort(interEndPts, strPt);
					double tempEndDis = endPt.Distance(interEndPts.back());
					if (COMPARE_VALUES_EPS(endDis, tempEndDis, 1e-6) == 1)//�µĽ����ڸ����İ�
					{
						if (endRecord == 2)//����������ʱ��գ���Ҫ�����°����������
							interEndPts.clear();
						interEndPts.push_back(it);
						endRecord--;
					}
				}

			}

		}

	};
	strVec.Normalize();
	for (auto it : m_walldata.upfloorID)
	{
		calInterPts(it);
	}
	for (auto it : m_walldata.downfloorID)
	{
		calInterPts(it);
	}
	if (!((COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) == 0) && isInSide))//���˵������ڲ�ֽ�
	{
		for (auto it : m_walldata.wallID)
		{
			calInterPts(it, false);
		}
	}
	if (!((COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0) && isInSide))//����ˮƽ���ڲ�ֽ�
	{
		for (auto it : m_walldata.floorID)
		{
			calInterPts(it, false);
		}
	}

	//ǽ����ܻ���Ҫ��������
	for (auto it : m_walldata.upfloorID)
	{
		calInterPts(it);
	}
	for (auto it : m_walldata.downfloorID)
	{
		calInterPts(it);
	}

	interPtsSort(interStrPts, endPt);
	interPtsSort(interEndPts, strPt);

	//7.��������ֵ
	if (interStrPts.size() > 0)
	{
		//PITCommonTool::CPointTool::DrowOnePoint(*interStrPts.begin(), 1, 3);
		extendStrDis = strPt.Distance(*interStrPts.begin());
		//���ݷ�������������
		strVec.Normalize();
		DVec3d extendStrVec = *interStrPts.begin() - strPt;
		extendStrVec.Normalize();
		if (extendStrVec.DotProduct(strVec) < 0) //������
			extendStrDis = extendStrDis * -1;
		//����������
		extendStrDis -= strMoveLen;
	}


	if (interEndPts.size() > 0)
	{
		//PITCommonTool::CPointTool::DrowOnePoint(*interEndPts.begin(), 1, 4);
		extendEndDis = endPt.Distance(*interEndPts.begin());
		//���ݷ�������������
		endVec.Normalize();
		DVec3d extendEndVec = *interEndPts.begin() - endPt;
		extendEndVec.Normalize();
		if (extendEndVec.DotProduct(endVec) < 0) //������
			extendEndDis = extendEndDis * -1;
		//����������
		extendEndDis -= endMoveLen;
	}


}

//��ȡê�̳���
double STWallRebarAssembly::get_lae() const
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


void STWallRebarAssembly::CalculateLeftRightBarLines(vector<BarLinesdata>& barlines, double fdiam, double allfdiam, DPoint3dR vecHoleehs, MSElementDescrP& path, MSElementDescrP& barline,
	int side, int index)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	WString strSize = GetvecDirSize().at(index);
	if (strSize.find(L"mm") != WString::npos)
	{
		strSize.ReplaceAll(L"mm", L"");
	}
	vector<MSElementDescrP>allpaths;
	double diameter1 = stoi(strSize.GetWCharCP())*uor_per_mm;
	double diameter = RebarCode::GetBarDiameter(strSize, ACTIVEMODEL);		//������10
	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;

	DPoint3d vecOutwall = m_walldata.vecToWall;//Զ��ǽ�ڵ�ƫ�Ʒ���
	vecOutwall.Negate();
	if (side != 0) vecOutwall.Negate();
	DPoint3d pt11, pt22;
	mdlElmdscr_extractEndPoints(&pt11, nullptr, &pt22, nullptr, barline, ACTIVEMODEL);
	double lengh = pt11.Distance(pt22) / 3;//�ƶ���1/3�ĵ�
	DPoint3d vecLine = pt22 - pt11;//��ʼ�ߴ�������
	vecLine.Normalize();
	vecLine.Scale(lengh);//�ӳ�
	//1������·���߳���
	//�����ж��ϲ���û�а壬�а�Ļ��Ƿ����Ҳ඼�ڰ�ķ�Χ�ڣ��������˵��Ϊ�ڲ��棨���Բ����ӳ�·���ߣ�
	//���ֻ��һ��˵��Ϊ����棬��Ҫ�ӳ�
	DPoint3d pt1, pt2;
	mdlElmdscr_extractEndPoints(&pt1, nullptr, &pt2, nullptr, path, ACTIVEMODEL);
	DPoint3d Tmept1 = pt1, Tmeppt2 = pt2;
	double D1 = 0, D2 = 0;
	ExtendLineByFloor(m_walldata.upfloorfaces, m_walldata.upfloorID, pt2, pt1, vecLine, m_walldata.upfloorth - diameter / uor_per_mm, D1, vecOutwall);
	ExtendLineByFloor(m_walldata.downfloorfaces, m_walldata.downfloorID, pt1, pt2, vecLine, m_walldata.downfloorth - diameter / uor_per_mm, D2, vecOutwall);


	if (Tmept1.Distance(pt1) > 1)//�°�
	{
		DPoint3d temp = Tmept1;
		pt1.z -= D2;
		temp.z = temp.z - diameter + D2;
		EditElementHandle tmpeeh;
		LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(pt1, temp), true, *ACTIVEMODEL);
		MSElementDescrP tmppath = tmpeeh.ExtractElementDescr();
		allpaths.push_back(tmppath);
	}
	//�м�
	EditElementHandle tmpeeh;
	LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(Tmept1, Tmeppt2), true, *ACTIVEMODEL);
	MSElementDescrP tmppath = tmpeeh.ExtractElementDescr();
	allpaths.push_back(tmppath);

	if (Tmeppt2.Distance(pt2) > 1)//�ϰ�
	{
		DPoint3d temp = Tmeppt2;
		pt2.z += D1;
		temp.z = temp.z + diameter - D1;
		EditElementHandle tmpeeh;
		LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(temp, pt2), true, *ACTIVEMODEL);
		MSElementDescrP tmppath = tmpeeh.ExtractElementDescr();
		allpaths.push_back(tmppath);
	}
	//tmpeeh.AddToModel();
	//2������ֽ��߳��Ⱥ�ê�̳��ȡ�ê�̷���
	//��ֽ�����ǽ�潻�㣬
	//(1)����н��㣬ֱ���ӳ��ֽ��ߵ����㴦
	DPoint3d facenormal;
	DPoint3d maxpt;
	mdlElmdscr_extractNormal(&facenormal, &maxpt, m_walldata.downFace, NULL);
	DPoint3d cpt = getCenterOfElmdescr(m_walldata.downFace);
	facenormal.Normalize();
	DPlane3d plane = DPlane3d::FromOriginAndNormal(cpt, DVec3d::From(facenormal.x, facenormal.y, facenormal.z));
	vector<DPoint3d> barpts;//ȡ�����иֽ��ߵ㼯��
	ExtractLineStringPoint(barline, barpts);

	//double L0 = 24 * uor_per_mm * 15;//15d
	//double Lae = 24 * uor_per_mm * 48 * 0.8;//48d
	double L0 = diameter1 * 15;//15d
	double Lae = diameter1 * get_lae() * 0.8 / uor_per_mm * 2;//LAE
	double mgDisHol = 0;
	//if (GetvecDataExchange().at(index) == 0)//�����
	//{
	//	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - 2 * diameter;
	//}
	//else if (GetvecDataExchange().at(index) == 2)//�ڲ���
	//{
	//	
	//}

	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - diameter;
	for (auto it : allpaths)
	{
		if (barpts.size() > 1)
		{//��ʼ���¼���ֽ�����ʼ��
			DPoint3d ptstrtmp = barpts[0];
			DPoint3d ptendtmp = barpts[barpts.size() - 1];
			double mgDisstr = 0;
			DPoint3d mgVecstr;
			mgDisstr = GetExtendptByWalls(ptstrtmp, barpts[1], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecstr);
			double mgDisend = 0;
			DPoint3d mgVecend;
			mgDisend = GetExtendptByWalls(ptendtmp, barpts[barpts.size() - 2], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecend);
			//linestringeeh.AddToModel();
		   //��2���ֽ�����ʼ�˼����������Ƿ�����ǽ�ڵĵ㣨�������Ȱ�����ǽ��ĳ��ȼ��㣩�����û��˵����ǽ���Ҷ�û������ǽ��ֱ�ӶϿ�����
		   //����У�����1/2�ĺ�ȳ��ȼ�����һ����Ƿ���ǽ�ڣ��ڵĻ���˵������Ϊ�ڲ�ǽ�棬���Ȱ�15d,����ΪԶ��ǽ�ڵķ��򣬸ֽ��߼���ֵ = ���汣���� + �Բ�ֽ�ֱ���� 
		   //���ֻ��һ���н���,˵��Ϊ�����ê�룬���Ȱ�0.8LAE,����Ϊ��ǽ�ڷ��򣬸ֽ��߼���ֵ =  ���汣���㡣
			//(3)�ֽ�����ֹ�˼��㣬�����裨2��
			BarLinesdata tmpbarline;
			tmpbarline.path = it;
			mdlElmdscr_duplicate(&tmpbarline.barline, barline);
			tmpbarline.spacing = GetvecDirSpacing().at(index)*uor_per_mm;
			tmpbarline.strDis = dSideCover + GetvecStartOffset().at(index)*uor_per_mm + diameter / 2;
			tmpbarline.endDis = dSideCover + GetvecEndOffset().at(index)*uor_per_mm + diameter / 2;
			tmpbarline.diameter = diameter;

			tmpbarline.extendstrDis = ptstrtmp.Distance(barpts[0]) - dSideCover;
			//����ê�볤���ж��ӳ�����
			if (tmpbarline.extendstrDis != 0 && abs(mgDisstr - L0) < 2)
			{
				tmpbarline.extendstrDis = tmpbarline.extendstrDis - allfdiam;
			}
			tmpbarline.extendendDis = ptendtmp.Distance(barpts[barpts.size() - 1]) - dSideCover;
			if (tmpbarline.extendendDis != 0 && abs(mgDisend - L0) < 2)
			{
				tmpbarline.extendendDis = tmpbarline.extendendDis - allfdiam;
			}
			tmpbarline.extstrdiameter = allfdiam - diameter / 2;
			tmpbarline.extenddiameter = allfdiam - diameter / 2;
			tmpbarline.isInSide = false;
			tmpbarline.vecstr = mgVecstr;
			tmpbarline.vecend = mgVecend;
			tmpbarline.vecHoleehs = vecHoleehs;
			tmpbarline.strMG = mgDisstr;
			tmpbarline.endMG = mgDisend;
			tmpbarline.holMG = mgDisHol;


			barlines.push_back(tmpbarline);

			////���Դ���
			//barpts[0] = ptstrtmp;
			//barpts[barpts.size() - 1] = ptendtmp;
			////���»��Ƹֽ���
			//EditElementHandle linestringeeh;
			//LineStringHandler::CreateLineStringElement(linestringeeh, nullptr, &barpts[0], barpts.size(), true, *ACTIVEMODEL);
			//MSElementDescrP nowlinedescr = linestringeeh.GetElementDescrP();

		}
	}

}

void STWallRebarAssembly::CalculateUpDownBarLines(vector<BarLinesdata>& barlines, double fdiam, double allfdiam, DPoint3dR vecHoleehs, MSElementDescrP& path, MSElementDescrP& barline,
	int side, int index)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	WString strSize = GetvecDirSize().at(index);
	if (strSize.find(L"mm") != WString::npos)
	{
		strSize.ReplaceAll(L"mm", L"");
	}
	double bendRadius = RebarCode::GetPinRadius(strSize, ACTIVEMODEL, false);
	double diameter = RebarCode::GetBarDiameter(strSize, ACTIVEMODEL);		//������10
	double diameter1 = stoi(strSize.GetWCharCP())*uor_per_mm;
	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double diameterStr = 0, diameterEnd = 0;
	DPoint3d vecOutwall = m_walldata.vecToWall;//Զ��ǽ�ڵ�ƫ�Ʒ���
	vecOutwall.Negate();
	if (side != 0) vecOutwall.Negate();
	DPoint3d pt11, pt22;
	mdlElmdscr_extractEndPoints(&pt11, nullptr, &pt22, nullptr, path, ACTIVEMODEL);
	double lengh = pt11.Distance(pt22) / 3;//�ƶ���1/3��
	DPoint3d vecLine = pt22 - pt11;//��ʼ�ߴ�������
	vecLine.Normalize();
	vecLine.Scale(lengh);//�ӳ�
	//1������·�ֽ��߳��ȡ��ֽ��ê�볤�Ⱥ�ê�뷽��
	//�����ж��ϲ���û�а壬�а�Ļ��Ƿ����Ҳ඼�ڰ�ķ�Χ�ڣ��������˵��Ϊ�ڲ��棨�ֽ����ӳ� = ���� - ������ - �Բ�ֽ�ֱ���ͣ��ֽ�ê�볤��Ϊ15d,ê�뷽��ΪԶ��ǽ����һ�ࣩ
	//���ֻ��һ��˵��Ϊ����棨�ֽ����ӳ� = ���� - ������,�ֽ�ê�볤��ΪLae,ê�뷽��Ϊ�а����һ�ࣩ
	DPoint3d pt1, pt2;
	mdlElmdscr_extractEndPoints(&pt1, nullptr, &pt2, nullptr, barline, ACTIVEMODEL);
	bool isInsidestr = false;
	bool isHavefloorstr = false;
	isHavefloorstr = CalculateBarLineDataByFloor(m_walldata.downfloorfaces, m_walldata.downfloorID, pt1, pt2, vecLine, m_walldata.downfloorth, vecOutwall, isInsidestr, diameterStr);
	//���Դ�����ʾ��ǰ���жϵ��λ��
	double mgDisstr = 0;//ê�̳���
	double extendstr = 0;//��������
	DPoint3d mgVecstr;//ê�̷���
	DPoint3d ptstr = pt1;
	DPoint3d ptend = pt2;
	if (isHavefloorstr)//����а�
	{
		if (isInsidestr)//���ڲ�
		{
			//PITCommonTool::CPointTool::DrowOneLine(pt1, pt2, 1);
			mgDisstr = 15 * diameter1;
			extendstr = extendstr + m_walldata.downfloorth*uor_per_mm - allfdiam - dSideCover;
			mgVecstr = vecOutwall;
			if (diameterStr == 0)
			{
				diameterStr = diameter;
				diameterStr = diameterStr * 2;
			}
		}
		else
		{
			//PITCommonTool::CPointTool::DrowOneLine(pt1, pt2, 2);
			mgDisstr = get_lae() * diameter1 / uor_per_mm;//lae
			extendstr = extendstr + m_walldata.downfloorth*uor_per_mm - dSideCover - diameterStr;
			mgVecstr = vecOutwall;
			mgVecstr.Negate();
		}
		Dpoint3d vectmp = ptstr - ptend;
		vectmp.Normalize();
		vectmp.Scale(extendstr);
		ptstr.Add(vectmp);

		//����ê�̷��򳤶���
		Dpoint3d mgpt = ptstr;
		mgVecstr.Scale(mgDisstr);
		mgpt.Add(mgVecstr);
		mgVecstr.Normalize();

		EditElementHandle theeh;
		LineHandler::CreateLineElement(theeh, nullptr, DSegment3d::From(ptstr, mgpt), true, *ACTIVEMODEL);
		//theeh.AddToModel();
	}
	else//����������Ϸ�û�а壬�ֽ���Ҫê�뵽ǽ���ڲ���ê�볤��Ϊǽ�� - ��ǰ������+�󱣻��㣩
	{
		mgDisstr = m_walldata.thickness - dPositiveCover - dReverseCover /*- bendRadius*/ - 1.5*diameter1;
		// ��������Ϸ�û�а壬������������Լ��ֽ�ƫ��
		// if (isInsidestr)//���ڲ�
		// {
		// 	extendstr = -1 * dPositiveCover - allfdiam;
		// }
		// else
		{
			extendstr = -1 * dPositiveCover;
		}

		mgVecstr = vecOutwall;
		//mgVecstr.Negate();
		//����ê�̷��򳤶���
		Dpoint3d mgpt = ptend;
		mgVecstr.Scale(mgDisstr);
		mgpt.Add(mgVecstr);
		mgVecstr.Normalize();

		EditElementHandle theeh;
		LineHandler::CreateLineElement(theeh, nullptr, DSegment3d::From(ptend, mgpt), true, *ACTIVEMODEL);
		//theeh.AddToModel();
	}
	bool isInsideend = false;
	bool isHavefloorend = false;
	isHavefloorend = CalculateBarLineDataByFloor(m_walldata.upfloorfaces, m_walldata.upfloorID, pt2, pt1, vecLine, m_walldata.upfloorth, vecOutwall, isInsideend, diameterEnd);
	double mgDisend = 0;//ê�̳���
	double mgDisHol = 0;//ê�̳���
	double extendend = 0;//��������
	DPoint3d mgVecend;//ê�̷���
	if (isHavefloorend)//����а�
	{
		if (isInsideend)//���ڲ�
		{
			mgDisend = 15 * diameter1;
			extendend = extendend + m_walldata.upfloorth*uor_per_mm - dSideCover - allfdiam;
			mgVecend = vecOutwall;
			if (diameterEnd == 0)
			{
				diameterEnd = diameter;
				diameterEnd = diameterEnd * 2;
			}
		}
		else
		{
			mgDisend = get_lae() * diameter1 / uor_per_mm;//lae
			extendend = extendend + m_walldata.upfloorth*uor_per_mm - dSideCover - diameterEnd;
			mgVecend = vecOutwall;
			mgVecend.Negate();
		}
		Dpoint3d vectmp = ptend - ptstr;
		vectmp.Normalize();
		vectmp.Scale(extendend);
		ptend.Add(vectmp);

		//����ê�̷��򳤶���
		Dpoint3d mgpt = ptend;
		mgVecend.Scale(mgDisstr);
		mgpt.Add(mgVecend);
		mgVecend.Normalize();

		EditElementHandle theeh;
		LineHandler::CreateLineElement(theeh, nullptr, DSegment3d::From(ptend, mgpt), true, *ACTIVEMODEL);
		//theeh.AddToModel();
	}
	else//����������Ϸ�û�а壬�ֽ���Ҫê�뵽ǽ���ڲ���ê�볤��Ϊǽ�� - ��ǰ������+�󱣻��㣩
	{
		mgDisend = m_walldata.thickness - dPositiveCover - dReverseCover/* - bendRadius*/ - 1.5*diameter1;
		extendend = 0;
		// ��������Ϸ�û�а壬������������Լ��ֽ�ƫ��
		// if (isInsidestr)//���ڲ�
		// {
		// 	extendend = -1 * dPositiveCover - allfdiam;
		// }
		// else
		{
			extendend = -1 * dPositiveCover;
		}
		mgVecend = vecOutwall;
		//mgVecend.Negate();
		//����ê�̷��򳤶���
		Dpoint3d mgpt = ptend;
		mgVecend.Scale(mgDisend);
		mgpt.Add(mgVecend);
		mgVecend.Normalize();

		EditElementHandle theeh;
		LineHandler::CreateLineElement(theeh, nullptr, DSegment3d::From(ptend, mgpt), true, *ACTIVEMODEL);
		//theeh.AddToModel();
	}
	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - 2 * diameter;
	//if (GetvecDataExchange().at(index) == 0)//�����
	//{
	//	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - 2 * diameter;
	//}
	//else if (GetvecDataExchange().at(index) == 2)//�ڲ���
	//{
	//	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - diameter;
	//}
	lae = get_lae() * diameter1 / uor_per_mm;//lae
	EditElementHandle teeh;
	LineHandler::CreateLineElement(teeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
	//teeh.AddToModel();
	DPoint3d facenormal;
	DPoint3d maxpt;
	mdlElmdscr_extractNormal(&facenormal, &maxpt, m_walldata.downFace, NULL);
	DPoint3d cpt = getCenterOfElmdescr(m_walldata.downFace);
	facenormal.Normalize();
	DPlane3d plane = DPlane3d::FromOriginAndNormal(cpt, DVec3d::From(facenormal.x, facenormal.y, facenormal.z));
	double L0 = 24 * uor_per_mm * 15;//15d
	double Lae = 24 * uor_per_mm * 48 *0.8;//48d
	//2������·���ߣ�����ȷ����û�п��ڵ�ǰǽ���������ǽ�壬����м��������ǽ�巽��Ҫ��·���ߴ�ֱ������ֱ�Ĺ��˵���
	//�ӳ�·���ߣ���·�����ƶ�������ǽ������λ�ô������·����������ǽ���߽��㣬������ͶӰ��·�����ϣ�����¼map<int,Dpoint3d>,��һ��ֵΪ��·���������룬�ڶ���ΪͶӰ�㣻
	//����ͶӰ�㣬�����µ�·���ߣ������·����ǰ���ΪͶӰ�㣬�Ҳ�Ϊԭʼ·���ߵ������յ㣬��Ҫ�ӳ����ӳ�����Ϊ�����汣����*2 + �ֽ�ֱ��
	//�����·�����յ�ΪͶӰ�㣬�Ҳ�Ϊԭʼ·���ߵ������յ㣬��Ҫ�ӳ����ӳ�����Ϊ�����汣����*2 + �ֽ�ֱ��
	GetCutPathLines(barlines, dSideCover, diameter1, path, m_walldata.cutWallfaces, m_walldata.downFace, m_walldata.height);//�����ڵ�ǰǽ����ǽ
	//3��������·�������ĸֽ��ߺ�·���ߣ�����������
	for (int i = 0; i < barlines.size(); i++)
	{
		barlines.at(i).spacing = GetvecDirSpacing().at(index)*uor_per_mm;
		barlines.at(i).strDis = dSideCover + GetvecStartOffset().at(i)*uor_per_mm + diameter * 1.5;
		barlines.at(i).endDis = dSideCover + GetvecEndOffset().at(i)*uor_per_mm + diameter * 1.5;
		barlines.at(i).diameter = diameter;
		barlines.at(i).extstrdiameter = diameterStr;
		barlines.at(i).extenddiameter = diameterEnd;
		barlines.at(i).extendstrDis = extendstr;
		barlines.at(i).extendendDis = extendend;
		barlines.at(i).vecstr = mgVecstr;
		barlines.at(i).vecend = mgVecend;
		barlines.at(i).vecHoleehs = vecHoleehs;
		barlines.at(i).strMG = mgDisstr;
		barlines.at(i).endMG = mgDisend;
		barlines.at(i).holMG = mgDisHol;
		barlines.at(i).isInSide = isInsidestr && isInsideend;
		vector<DPoint3d> barpts;//ȡ�����иֽ��ߵ㼯��
		ExtractLineStringPoint(barlines.at(i).path, barpts);
		if (barpts.size() > 1)
		{//��ʼ���¼���ֽ�����ʼ��
			DPoint3d ptstrtmp = barpts[0];
			DPoint3d ptendtmp = barpts[barpts.size() - 1];
			double mgDisstr = 0;
			DPoint3d mgVecstr;
			mgDisstr = GetExtendptByWalls(ptstrtmp, barpts[1], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecstr);
			double mgDisend = 0;
			DPoint3d mgVecend;
			mgDisend = GetExtendptByWalls(ptendtmp, barpts[barpts.size() - 2], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecend);
			//����ê�볤���ж��ӳ�����
			if (ptstrtmp.Distance(barpts[0]) > 10 && abs(mgDisstr - L0) > 100)//���ֽ���Ҫ�ӳ�
			{
				//������һ���ֽ��࣬��֤���һ���ֽ���ص�
				DPoint3d tmpptvec = barpts[0] - ptstrtmp;
				tmpptvec.Normalize();
				tmpptvec.Scale(barlines.at(i).spacing);
				ptstrtmp.Add(tmpptvec);
				barpts[0] = ptstrtmp;
			}
			if (ptendtmp.Distance(barpts[barpts.size() - 1]) > 10 && abs(mgDisend - L0) > 100)
			{
				//������һ���ֽ��࣬��֤���һ���ֽ���ص�
				DPoint3d tmpptvec = barpts[barpts.size() - 1] - ptendtmp;
				tmpptvec.Normalize();
				tmpptvec.Scale(barlines.at(i).spacing);
				ptendtmp.Add(tmpptvec);
				barpts[barpts.size() - 1] = ptendtmp;
			}
			//�ؽ�·����
			EditElementHandle linestringeeh;
			LineStringHandler::CreateLineStringElement(linestringeeh, nullptr, &barpts[0], barpts.size(), true, *ACTIVEMODEL);
			mdlElmdscr_freeAll(&barlines.at(i).path);
			barlines.at(i).path = linestringeeh.ExtractElementDescr();
			//mdlElmdscr_add(barlines.at(i).path);
		}
	}
}
void STWallRebarAssembly::CalculateBarLinesData(map<int, vector<BarLinesdata>> &barlines, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double dLevelSpace = 0;

	double dOffset = dPositiveCover;
	double dOffsetTb = dPositiveCover;

	DSegment3d temp = m_walldata.vecFontLine[0];
	DPoint3d ptr1, ptr2;
	m_walldata.vecFontLine[0].GetStartPoint(ptr1);
	m_walldata.vecFontLine[0].GetEndPoint(ptr2);
	ptr1.Add(ptr2);
	ptr1.Scale(0.5);
	//����ָ��Ԫ����������Ԫ�صķ�Χ��
	//�ж�vecFontLine�Ƿ�������� �����������ڲ��߻���
	auto CheckAndSwapFrontLine = [&](const vector<MSElementDescrP>& faces) -> bool {
		// ��������������������
		if (abs(ptr1.x - ptr2.x) > 10 * uor_per_mm && abs(ptr1.y - ptr2.y) > 10 * uor_per_mm)
			return false;

		DRange3d range;
		const double offset = 50 * uor_per_mm; // ��Χƫ����

		for (const auto& face : faces)
		{
			// ����Ԫ�ط�Χ
			mdlElmdscr_computeRange(&range.low, &range.high, face, nullptr);

			// ��С��Χ������ 50mm ƫ��
			range.low.x += offset;
			range.low.y += offset;
			range.high.x -= offset;
			range.high.y -= offset;

			// �ж� ptr1 �Ƿ��ڷ�Χ��
			if (range.IsContainedXY(ptr1))
			{
				// ����������߲���ת����
				m_walldata.vecFontLine[0] = m_walldata.vecBackLine[0];
				m_walldata.vecBackLine[0] = temp;
				m_walldata.vecToWall.Negate();
				return true; // �ѳɹ�����
			}
		}
		return false;
	};
	do
	{
		// �ȼ�� downfloorfaces
		if (CheckAndSwapFrontLine(m_walldata.downfloorfaces))
			break;

		// ��� downfloorfaces δ����ɹ����ټ�� upfloorfaces
		CheckAndSwapFrontLine(m_walldata.upfloorfaces);
	}
	while (false);

	//����ƫ�Ʒ���
	if (m_walldata.vecFontLine.size() == 0) return;
	DVec3d moveLine = m_walldata.vecdown;
	DPoint3d pt1[2];
	m_walldata.vecFontLine[0].GetStartPoint(pt1[0]);
	m_walldata.vecFontLine[0].GetEndPoint(pt1[1]);
	DPoint3d linevec = pt1[1] - pt1[0];
	linevec.Normalize();
	moveLine.CrossProduct(moveLine, linevec);
	DPoint3d pt2[2];
	m_walldata.vecBackLine[0].GetStartPoint(pt2[0]);

	//1�������桢�м䡢����ɸѡ����Ӧ�ĸֽ�������Ӧ�ĸֽ�ֱ��
	map<int, double> FDiameters;
	map<int, double> MDiameters;
	map<int, double> BDiameters;

	double allfdiam = 0;//ͳ�������ֽ�ֱ��
	double allmdiam = 0;//ͳ���м��ֽ�ֱ��
	double allbdiam = 0;//ͳ���ڲ���ֽ�ֱ��
	double alldiam = 0;//�Բ�ֽ�ֱ����
	DPoint3d vecHoleehs;//������ֿ׶���׶�ê��ķ���
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}
		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//������10
		if (GetvecDataExchange().at(i) == 0)//�����
		{
			FDiameters[GetvecRebarLevel().at(i)] = diameter;
			allfdiam = allfdiam + diameter;
		}
		else if (GetvecDataExchange().at(i) == 2)//�ڲ���
		{
			BDiameters[GetvecRebarLevel().at(i)] = diameter;
			allbdiam = allbdiam + diameter;
		}
		else
		{
			MDiameters[GetvecRebarLevel().at(i)] = diameter;
			allmdiam = allmdiam + diameter;
		}
	}

	//2�������ÿһ���ƫ��������������Ϊ��׼
	//�����棬���棬�м����������㣬
	//����ʱ��ƫ���� = ���汣���� + ��ǰ��ֽ�ֱ��/2 + ǰ��������ֽ�ֱ���ĺͣ�
	//�м�ʱ��ƫ���� = ǽ��/2 + ǰ��������ֽ�ֱ���ĺͣ�
	//����ʱ��ƫ���� = ǽ�� - ���汣���� - ����ǰ��ֽ�ֱ��/2 + ǰ��������ֽ�ֱ���ĺͣ���
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}

		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//������10
		double movedis = dPositiveCover + diameter / 2;
		double fdiam = 0;//ǰ�漸��ֽ�ֱ��

		if (GetvecDataExchange().at(i) == 0)//ǰ���
		{
			int tindex = GetvecRebarLevel().at(i);
			for (int k = 1; k < tindex; k++)//����֮ǰ�ĸֽ�ֱ��
			{
				movedis = FDiameters[k] + movedis;
				fdiam = fdiam + FDiameters[k];
			}
		}
		else if (GetvecDataExchange().at(i) == 2)//�����
		{
			movedis = m_walldata.thickness - dReverseCover - diameter / 2;
			int tindex = GetvecRebarLevel().at(i);
			for (int k = 1; k < tindex; k++)//����֮ǰ�ĸֽ�ֱ��
			{
				movedis = movedis - BDiameters[k];
				fdiam = fdiam + BDiameters[k];
			}
		}
		else
		{
			movedis = m_walldata.thickness / 2;
			int tindex = GetvecRebarLevel().at(i);
			for (int k = 1; k < tindex; k++)//����֮ǰ�ĸֽ�ֱ��
			{
				movedis = MDiameters[k] + movedis;
				fdiam = fdiam + MDiameters[k];
			}
		}
		if (GetvecDataExchange().at(i) == 0)//ǰ���,�Բ��Ϊ����
		{
			alldiam = allbdiam;
			vecHoleehs = pt2[0] - pt1[0];

		}
		else if (GetvecDataExchange().at(i) == 2)//����㣬�Բ��Ϊǰ��
		{
			alldiam = allfdiam;
			vecHoleehs = pt1[0] - pt2[0];
		}
		else
		{
			alldiam = 0;
		}

		vecHoleehs.Normalize();

		//����path�ߴ�������Ԫ�ء�ƫ��������ƫ�ƺ���ߴ�
		MSElementDescrP tmppath = GetLines(m_walldata.vecFontLine);
		GetMovePath(tmppath, movedis, m_walldata.downFace);
		//�����������
		DPoint3d startP, endP;
		mdlElmdscr_extractEndPoints(&startP, nullptr, &endP, nullptr, tmppath, ACTIVEMODEL);
		//PITCommonTool::CPointTool::DrowOneLine(DSegment3d::From(pt1, pt2), 4);//��
		endP = startP;
		endP.z = endP.z + m_walldata.height*uor_per_mm;
		EditElementHandle tlineeeh;
		LineHandler::CreateLineElement(tlineeeh, nullptr, DSegment3d::From(startP, endP), true, *ACTIVEMODEL);
		//PITCommonTool::CPointTool::DrowOneLine(DSegment3d::From(pt1, pt2), 3);//��

		//PITCommonTool::CPointTool::DrowOneLine(m_walldata.vecFontLine[0],1);//��
		//PITCommonTool::CPointTool::DrowOneLine(m_walldata.vecBackLine[0], 2);//��
		//3��������Χ���塢�װ��ǽ��������㵱ǰʵ����Χ�͸ֽ��������Լ��乳����
		//�ֽ��ͬ����ʽ��һ��

		if (GetvecDir().at(i) == 0)//ˮƽ����ֽ�
		{
			MSElementDescrP path = tlineeeh.GetElementDescrP();
			MSElementDescrP barline = tmppath;
			CalculateLeftRightBarLines(barlines[i], fdiam, alldiam, vecHoleehs, path, barline, GetvecDataExchange().at(i), i);
			mdlElmdscr_freeAll(&tmppath);
		}
		else//��ֱ����ֽ�
		{
			MSElementDescrP barline = tlineeeh.GetElementDescrP();
			MSElementDescrP  path = tmppath;
			CalculateUpDownBarLines(barlines[i], fdiam, alldiam, vecHoleehs, path, barline, GetvecDataExchange().at(i), i);
			mdlElmdscr_freeAll(&tmppath);
		}
		/*BarLinesdata tmpdata;
		mdlElmdscr_add(tmpdata.barline);
		mdlElmdscr_add(tmpdata.path);
		tmpdata.spacing = GetvecDirSpacing().at(i)*uor_per_mm;
		tmpdata.strDis = dSideCover + GetvecStartOffset().at(i)*uor_per_mm;
		tmpdata.endDis = dSideCover + GetvecEndOffset().at(i)*uor_per_mm;
		barlines[i].push_back(tmpdata);*/
	}

}
/*
* @desc:		���ݱ�������ƶ��������¼�������ֽ��ߴ�
* @param[out]	data ���������
* @return	MSElementDescrP �µĸֽ��ߴ�
* @author	Hong ZhuoHui
* @Date:	2023/09/13
*/
void STWallRebarAssembly::ReCalBarLineByCoverAndDis(BarLinesdata & data)
{
	//������β�ƶ�����
	double strMoveLen = 0 - data.strDis;
	double endMoveLen = 0 - data.endDis;
	//������β��
	DPoint3d strPt = { 0,0,0 }, endPt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&strPt, nullptr, &endPt, nullptr, data.barline, ACTIVEMODEL);
	//������β�ƶ�����
	DVec3d strVec = strPt - endPt;
	strVec.Normalize();
	strVec.ScaleToLength(strMoveLen);
	DVec3d endVec = endPt - strPt;
	endVec.Normalize();
	endVec.ScaleToLength(endMoveLen);
	//�ƶ���β��
	strPt.Add(strVec);
	endPt.Add(endVec);
	//���»���
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPt, endPt), true, *ACTIVEMODEL);
	data.barline = eeh.GetElementDescrP();
	//ƽ��·����
	mdlCurrTrans_begin();
	Transform tMatrix;
	mdlTMatrix_getIdentity(&tMatrix);
	mdlTMatrix_setTranslation(&tMatrix, &strVec);
	mdlElmdscr_transform(&data.path, &tMatrix);
	mdlCurrTrans_end();
}

/*
* @desc:		����ʵ�������׶���Z��ǽ��
* @param[in]	wallEeh ǽ
* @param[out]	holes �׶�
* @author	Hong ZhuoHui
* @Date:	2023/09/13
*/
void STWallRebarAssembly::CalHolesBySubtract(EditElementHandleCR wallEeh, std::vector<EditElementHandle*>& holes)
{
	if (m_walldata.downFace == nullptr)
		return;

	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	MSElementDescrP tmpDowmFace = nullptr;
	m_walldata.downFace->Duplicate(&tmpDowmFace);
	EditElementHandle eehdown;
	eehdown.SetElementDescr(tmpDowmFace, true, true, ACTIVEMODEL);
	ISolidKernelEntityPtr ptarget;
	SolidUtil::Convert::ElementToBody(ptarget, eehdown);
	SolidUtil::Modify::ThickenSheet(ptarget, m_walldata.height * uor_now, 0.0);
	if (SUCCESS == SolidUtil::Convert::BodyToElement(eehdown, *ptarget, NULL, *ACTIVEMODEL))
	{
		EditElementHandle eehhole;
		if (SolidBoolWithFace(eehhole, eehdown, wallEeh, BOOLOPERATION_SUBTRACT, false))
		{
			//ȡ���е���
			std::vector<EditElementHandle*> allfaces;
			//ExtractFacesTool::GetFaces(holeeeh, allfaces);
			GetHoleElementAllFaces(eehhole, allfaces);
			vector<ISolidKernelEntityPtr> allfoceentitys;
			for (int i = 0; i < allfaces.size(); i++)
			{
				ISolidKernelEntityPtr tmpptr;
				if (SolidUtil::Convert::ElementToBody(tmpptr, *allfaces.at(i)) == SUCCESS)
				{
					allfoceentitys.push_back(tmpptr);
				}
			}
			bvector<ISolidKernelEntityPtr> sewn;
			bvector<ISolidKernelEntityPtr> unsewn;
			SolidUtil::Modify::SewBodies(sewn, unsewn, &allfoceentitys.front(), allfoceentitys.size(), true);
			for (int i = 0; i < sewn.size(); i++)
			{
				ISolidKernelEntityPtr tmpptr = sewn.at(i);
				EditElementHandle* tmpeeh = new EditElementHandle();
				if (SolidUtil::Convert::BodyToElement(*tmpeeh, *tmpptr, nullptr, *ACTIVEMODEL) == SUCCESS)
				{
					char tmpc[256];
					itoa(i, tmpc, 10);
					string holename;
					holename = holename + "_Hole_" + tmpc;
					SetElemGraphItemTypeValue(*tmpeeh, holename, "NEG", L"PARADATA", L"PARADATA", ACTIVEMODEL);
					holes.push_back(tmpeeh);
				}
			}
		}
	}
}
void STWallRebarAssembly::CalculateTransform(vector<CVector3D> &vTransform, vector<CVector3D> &vTransformTb, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return;

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	vTransform.clear();
	vTransformTb.clear();
	double updownSideCover = 50 * uor_per_mm;
	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double dLevelSpace = 0;
	double diameterTie = 0.0;
	BrString strTieRebarSize(GetTieRebarInfo().rebarSize);
	if (strTieRebarSize != L""/*&& 0 != GetTieRebarInfo().tieRebarMethod*/)
	{
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


			// 			if (0 == i)
			// 			{
			// 				dOffset += diameter / 2.0;	//ƫ���ײ�ֽ�뾶
			// 				zTrans.y = dOffset;
			// 		    }
						//if (0 == i)
						//{
						//	dOffset += diameter / 2.0;	//ƫ���ײ�ֽ�뾶
						//	zTrans.y = dOffset;

						//	if (GetvecTwinRebarLevel().at(i).hasTwinbars)
						//	{
						//		if (diameterTb > diameter)//�����ĸֽ������ֱ����
						//		{
						//			dOffsetTb = dOffset;
						//			dOffsetTb -= (diameterTb / 2.0 - diameter / 2.0);
						//			zTransTb.y = dOffsetTb;
						//		}
						//		else
						//		{
						//			dOffsetTb = dOffset;
						//			dOffsetTb += (diameter / 2.0 - diameterTb / 2.0);
						//			zTransTb.y = dOffsetTb;
						//		}
						//	}
						//}
						//else
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


void STWallRebarAssembly::CalculateNowPlacementAndLenth(double &misDisstr, double &misDisend, double& lenth, CMatrix3D   mat, DgnModelRefP modelRef)
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
	if (pt1[0].Distance(ptstar) > pt1[0].Distance(ptend))//ȷ��pt1[0]Ϊ��ʼ��
	{
		DPoint3d tmpPt = pt1[0];
		pt1[0] = pt1[1];
		pt1[1] = tmpPt;
	}

	vector<DPoint3d> inpts;
	GetIntersectPointsWithHoles(inpts, m_Negs, ptstar, ptend);
	lenth = pt1[0].Distance(pt1[1]);
	//if (!GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	if (inpts.size() > 0)
	{
		/*EditElementHandle eeh1;
		LineHandler::CreateLineElement(eeh1, nullptr, DSegment3d::From(pt1[0], pt1[1]), true, *ACTIVEMODEL);
		eeh1.AddToModel();*/
		DPoint3d ptProject1;	//ͶӰ��
		mdlVec_projectPointToLine(&ptProject1, NULL, &pt1[0], &m_STwallData.ptStart, &m_STwallData.ptEnd);

		DPoint3d ptProject2;	//ͶӰ��
		mdlVec_projectPointToLine(&ptProject2, NULL, &pt1[1], &m_STwallData.ptStart, &m_STwallData.ptEnd);

		misDisstr = ptProject1.Distance(ptstar);

		misDisend = ptProject2.Distance(ptend);

	}



}


void STWallRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
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
				if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//������Ŷ�
				{
					continue;
				}
				bool isdoorNeg = false;//�ж��Ƿ�Ϊ�Ŷ�NEG
				isdoorNeg = IsDoorHoleNeg(m_Holeehs.at(j), m_doorsholes);
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
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}



}


bool STWallRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	CalculateUseHoles(modelRef);
	m_vecRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
	g_vecTieRebarPtsNoHole.clear();
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
	double dLevelSpace = 0;
	double dSideCover = GetSideCover()*uor_per_mm;
	//if (COMPARE_VALUES(dSideCover, m_STwallData.length) >= 0)	//������汣������ڵ���ǽ�ĳ���
	//{
	//	mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"���汣������ڵ���ǽ�ĳ���,�޷������ֽ��", MessageBoxIconType::Information);
	//	return false;
	//}

	if (m_walldata.downFace == nullptr) return false;
	map<int, vector<BarLinesdata>> barlines;
	CalculateBarLinesData(barlines, ACTIVEMODEL);
	map<int, bool> map_isInside;
	// �ҳ�ͬһ���У�Ϊ�ڲ�ĸֽ���
	for (const auto& it : barlines) 
	{
		// �����һ��isInSideΪtrue�����¼����
		for (const auto& bar : it.second)
		{
			if (!bar.isInSide)
				continue;
			map_isInside[GetvecDataExchange().at(it.first)] = true;
			break;
		}
	}
	int setCount = 0;
	m_nowlevel = -1;
	for (auto& it : barlines)
	{
		m_vecRebarPtsLayer.clear();
		m_nowlevel++;
		if (it.second.size() == 0)
			continue;
		setCount++;
		RebarSetTag* tag = NULL;
		PopvecSetId().push_back(0);
		// ͬһ���У������һ��ɹ��ж�Ϊ�ڲ࣬���ͬ��Ϊ�ڲ࣬�����ٲ��ú������ж�
		if (map_isInside.find(GetvecDataExchange().at(it.first)) != map_isInside.end()) {
			for (auto& bar : it.second) {
				bar.isInSide = true;
			}
		}
		// û��ȷ���������ʱ�����ⲿ���ж�
		if (!it.second[0].isInSide)
		{
			if (GetvecDataExchange().at(it.first) == 0)//�����
			{
				it.second[0].isInSide = false;
			}
			else if (GetvecDataExchange().at(it.first) == 2)//�ڲ���
			{
				it.second[0].isInSide = true;
			}
			//�ж��Ƿ�ǽȫ�������Χ
			DPoint3d ptr1, ptr2;
			m_walldata.vecFontLine[0].GetStartPoint(ptr1);
			m_walldata.vecFontLine[0].GetEndPoint(ptr2);
			ptr1.Add(ptr2);
			ptr1.Scale(0.5);//�е��ж�
			DRange3d range;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			for (auto data : m_walldata.downfloorfaces)//�ж�vecFontLine�Ƿ������ �����������ڲ�
			{
				if (abs(ptr1.x - ptr2.x) > 10 * uor_per_mm && abs(ptr1.y - ptr2.y) > 10 * uor_per_mm)
					break;
				mdlElmdscr_computeRange(&range.low, &range.high, data, NULL);
				//mdlElmdscr_add(it);
				range.low.x = range.low.x + 50 * uor_per_mm;
				range.low.y = range.low.y + 50 * uor_per_mm;
				range.high.x = range.high.x - 50 * uor_per_mm;
				range.high.y = range.high.y - 50 * uor_per_mm;
				if (range.IsContainedXY(ptr1))
				{
					it.second[0].isInSide = false;
					break;
				}
			}
		}
		
		tag = MakeRebars(PopvecSetId().back(), GetvecDirSize().at(it.first), it.second,
			GetvecStartOffset().at(it.first)*uor_per_mm, GetvecEndOffset().at(it.first)*uor_per_mm,
			GetvecRebarLevel().at(it.first), GetvecRebarType().at(it.first), ACTIVEMODEL, GetvecRebarLineStyle().at(it.first), GetvecRebarWeight().at(it.first));
		if (NULL != tag && (!PreviewButtonDown))
		{
			tag->SetBarSetTag(setCount);
			rsetTags.Add(tag);
		}
	}
	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

long STWallRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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


bool STWallRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pWallDoubleRebarDlg = new CWallRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pWallDoubleRebarDlg->SetSelectElement(ehSel);
	pWallDoubleRebarDlg->Create(IDD_DIALOG_WallRebar);
	pWallDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pWallDoubleRebarDlg->ShowWindow(SW_SHOW);

	return true;
}

bool STWallRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
		return false;

	DgnModelRefP modelRef = ehWall.GetModelRef();

	SetWallData(ehWall);
	MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();
	//SetElementXAttribute(ehSel.GetElementId(), g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ehSel.GetModelRef());
	//eeh2.AddToModel();
	return true;
}


bool STWallRebarAssembly::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;
	InitUcsMatrix();
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}

bool STWallRebarAssembly::AnalyzingWallGeometricData(ElementHandleCR eh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	auto type = JudgeElementType(eh);
	DPoint3d FrontStr, FrontEnd;
	DPoint3d BackStr, BackEnd;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();
	m_doorsholes.clear();
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	bool result = EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);//��ȡ����֮ǰ��ǽ��
	if (!result)
		return false;

	//1����ǽ��Ӳο�ģ�Ϳ�������ǰģ��
	EditElementHandle copyEleeh;
	copyEleeh.Duplicate(Eleeh);
	ElementCopyContext copier2(ACTIVEMODEL);
	copier2.SetSourceModelRef(Eleeh.GetModelRef());
	copier2.SetTransformToDestination(true);
	copier2.SetWriteElements(false);
	copier2.DoCopy(copyEleeh);
	m_walldata.ClearData();
	//2����ȡ�ϲ����ǽ�����������ͶӰ�����е���ĺϲ�
	m_walldata.downFace = ExtractFacesTool::GetCombineDownFace(copyEleeh);

	if (m_walldata.downFace != nullptr)
	{
		//mdlElmdscr_add(m_walldata.downFace);
	}
	//3����ȡǽ�����ǰ���ߴ��ͺ��
	m_walldata.vecFontLine.clear();
	m_walldata.vecBackLine.clear();
	ExtractFacesTool::GetFrontBackLinePoint(m_walldata.downFace, m_walldata.vecFontLine, m_walldata.vecBackLine, m_walldata.thickness);

	DPoint3d minP2, maxP2;
	//����ָ��Ԫ����������Ԫ�صķ�Χ��
	mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
	if (!Eleeh.IsValid())
	{
		//mdlDialog_dmsgsPrint(L"�Ƿ���ǽʵ��!");
		return false;
	}
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
	m_walldata.vecdown = DVec3d::From(facenormal.x, facenormal.y, facenormal.z);
	RotMatrix rMatrix;
	Transform trans;
	mdlRMatrix_getIdentity(&rMatrix);
	mdlRMatrix_fromVectorToVector(&rMatrix, &facenormal, &vecZ);//��ת��xoyƽ��
	mdlTMatrix_fromRMatrix(&trans, &rMatrix);
	mdlTMatrix_setOrigin(&trans, &minPos);
	copyEleeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(copyEleeh, TransformInfo(trans));
	downface.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(downface, TransformInfo(trans));

	GetDoorHoles(Holeehs, m_doorsholes);

	if (m_walldata.vecFontLine.empty() || m_walldata.vecBackLine.empty())
	{
		return false;
	}



	DPoint3d pt1[2];
	m_walldata.vecFontLine[0].GetStartPoint(pt1[0]);
	m_walldata.vecFontLine[0].GetEndPoint(pt1[1]);

	DPoint3d pt2[2];
	m_walldata.vecBackLine[0].GetStartPoint(pt2[0]);
	m_walldata.vecBackLine[0].GetEndPoint(pt2[1]);

	DPoint3d vecLine = pt1[1] - pt1[0];
	vecLine.Normalize();
	m_walldata.vecToWall = facenormal;
	m_walldata.vecToWall.CrossProduct(m_walldata.vecToWall, vecLine);


	if (m_walldata.vecBackLine.size() > 1 || m_walldata.vecFontLine.size() > 1)
	{
		GetMaxDownFacePts(m_walldata.vecFontLine, m_walldata.vecBackLine, pt1, pt2);
	}

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_STwallData.height);
	//����ָ��Ԫ����������Ԫ�صķ�Χ��
	mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
	m_walldata.height = (maxP2.z - minP2.z) / uor_now;
	m_STwallData.height = (maxP2.z - minP2.z)*uor_now / uor_ref;
	m_STwallData.width = FrontStr.Distance(BackStr)*uor_now / uor_ref;
	m_STwallData.length = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;
	m_STwallData.ptStart = FrontStr;
	m_STwallData.ptEnd = FrontEnd;
	m_width = m_STwallData.width;




	FrontStr.Add(FrontEnd);
	FrontStr.Scale(0.5);
	BackStr.Add(BackEnd);
	BackStr.Scale(0.5);
	m_LineNormal = BackStr - FrontStr;
	m_LineNormal.Normalize();

	m_Negs = Negs;
	if (Negs.size() > 0)//STWALL��б��
	{

		DPoint3d vecBack = pt2[0] - pt2[1];
		DPoint3d vecLeft = pt1[0] - pt2[0];
		DPoint3d vecRight = pt1[1] - pt2[1];

		vecBack.Normalize();
		vecLeft.Normalize();
		vecRight.Normalize();

		m_angle_left = vecBack.AngleTo(vecLeft);
		if (m_angle_left > PI / 2)
		{
			m_angle_left = PI - m_angle_left;
		}
		m_angle_right = vecBack.AngleTo(vecRight);
		if (m_angle_right > PI / 2)
		{
			m_angle_right = PI - m_angle_right;
		}

	}
	else
	{
		m_angle_left = PI / 2;
		m_angle_right = PI / 2;
	}
	m_Holeehs = Holeehs;

	string Ename, Etype;
	string wallname = GetwallName();
	if (GetEleNameAndType(testeeh, Ename, Etype))
	{
		wallname = Ename;
	}

	GetUpDownFloorFaces(m_walldata, testeeh);
	GetLeftRightWallFaces(m_walldata, testeeh, wallname);
	return true;
}

void STWallRebarAssembly::InitUcsMatrix()
{
	DPoint3d ptStart = m_STwallData.ptStart;
	DPoint3d ptEnd = m_STwallData.ptEnd;

	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
	yVec.Normalize();
	//	CVector3D  yVecNegate = yVec;
	//	yVecNegate.Negate();
	//	yVecNegate.Normalize();
	//	yVecNegate.ScaleToLength(m_STwallData.width);//������յ�����Ϊǽ�������ߣ����Դ˴�ֻ��Ҫƫ��1/2���
	//	ptStart.Add(yVecNegate);
	//	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(ptStart, ptEnd);
	xVecNew.Normalize();
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//����ΪX�ᣬˮƽ��ֱ����ΪY��
	SetPlacement(placement);
	CMatrix3D   mat = placement;
	mat = mat.Inverse();
	/*mat.m[2][0] = CVector3D::kZaxis.x;
	mat.m[2][1] = CVector3D::kZaxis.y;
	mat.m[2][2] = CVector3D::kZaxis.z;
	mat = mat.Inverse();
	placement = mat;
	SetPlacement(mat);

	mat = mat.Inverse();*/
	Transform trans;
	mat.AssignTo(trans);
	DPoint3d pt1 = ptStart;
	DPoint3d pt2 = ptEnd;
	EditElementHandle lineeeh;
	LineHandler::CreateLineElement(lineeeh, nullptr, DSegment3d::From(pt1, pt2), true, *ACTIVEMODEL);
	lineeeh.GetHandler().ApplyTransform(lineeeh, TransformInfo(trans));

	DPoint3d Vpt1[2];
	mdlLinear_extract(Vpt1, NULL, lineeeh.GetElementP(), ACTIVEMODEL);
	m_VecX = Vpt1[1] - Vpt1[0];
	m_VecX.Normalize();
	//lineeeh.AddToModel();

	pt1 = ptStart;
	pt2 = ptStart;
	pt2.z = pt2.z + 40000;
	EditElementHandle lineeeh2;
	LineHandler::CreateLineElement(lineeeh2, nullptr, DSegment3d::From(pt1, pt2), true, *ACTIVEMODEL);
	lineeeh2.GetHandler().ApplyTransform(lineeeh2, TransformInfo(trans));
	DPoint3d Vpt2[2];
	mdlLinear_extract(Vpt2, NULL, lineeeh2.GetElementP(), ACTIVEMODEL);
	m_VecZ = Vpt2[1] - Vpt2[0];
	m_VecZ.Normalize();
	//lineeeh2.AddToModel();

	DPoint3d tmppt = ptStart;
	yVec.Scale(m_STwallData.height);
	tmppt.Add(yVec);



	/*DPoint3d vecY = yVec;
	vecY.Normalize();
	vecY.Scale(m_STwallData.width / 5);
	ptStart.Add(vecY);
	ptEnd.Add(vecY);*/
	PopvecFrontPts().push_back(ptStart);
	PopvecFrontPts().push_back(ptEnd);
}

/*
* @desc:		������ΧԪ�غ͸ֽ��ߵ�λ�ü���ֽ��Ƿ�Ϸ�
* @param[in]	nowVec  �ж�����ֱ������ˮƽ����
* @param[in]	alleehs ��ΧԪ��
* @param[in]	tmpendEndTypes �˲���ʽ
* @param[in]	lineEeh �ֽ���
* @param[in]    Eleeh  ����֮ǰ��ʵ�壨ǽ���壩
* @param[in]	anchored_vec  ê��Ƕȣ�����
* @param[in]	matrix  ͶӰ����
* @param[in]	moveDis ���������
* @param[in]	lenth   ê�볤��
* @param[in]	ori_Point   ԭ�˵�λ��
* @param[in]	mov_Point   �޸Ķ˵�λ��
* @param[in]	type_flag   �Ƿ���Ҫ�˲���ʽ�����FLAGE=1��ʾ����Ҫ�˲���ʽ�����FLAGE=2��ʾβ�˵�λ���޸�Ϊmov_Pointλ��
* @param[in/out]	data ���������
* @author	ChenDong
* @Date:	2024/10/17
*/

void STWallRebarAssembly::JudgeBarLinesLegitimate(const CVector3D  nowVec, vector<EditElementHandle*>alleehs, PIT::PITRebarEndTypes& tmpendEndTypes,
	EditElementHandle &lineEeh, EditElementHandle *Eleeh, CVector3D anchored_vec
	, const Transform matrix, double moveDis, double lenth, DPoint3d &ori_Point, DPoint3d &mov_Point, int &type_Flag)
{
	bool is_str = false;//�Ƿ�Ϊ��ʼ��
	DPoint3d using_pt;
	DPoint3d mid_end_pt;//�ֽ�˲��е��ж�
	vector<DPoint3d> tmppts;//ʵ�彻��
	DPoint3d str_Pt = { 0,0,0 }, end_Pt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&str_Pt, nullptr, &end_Pt, nullptr, lineEeh.GetElementDescrP(), ACTIVEMODEL);
	double dSideCover = 0;

	using_pt = ori_Point;
	movePoint(anchored_vec, using_pt, lenth);

	//�ֽ��߶˵�͸ֽ�˵�λ���Ƿ�ê��
	if (!ISPointInHoles(alleehs, ori_Point) || !ISPointInHoles(alleehs, using_pt))
	{
		if (type_Flag == 0)//˵������ʼ��
			is_str = true;
		int endtype_flag = 0;//endtype_flag == 1 �˲��仡Ϊֱ�� ��Ϊ1Ϊ������ê
		//�˲���ʽ��ֱ��ê������бê��
		if (anchored_vec.x == floor(anchored_vec.x) && anchored_vec.y == floor(anchored_vec.y) && anchored_vec.z == floor(anchored_vec.z))
		{
			anchored_vec.Negate();
			endtype_flag = 1;//�˲��仡Ϊֱ��
		}
		else
		{
			anchored_vec.x = anchored_vec.x;
			anchored_vec.y = -anchored_vec.y;
			anchored_vec.z = anchored_vec.z;
		}
		using_pt = ori_Point;
		mid_end_pt = ori_Point;
		//����ȡ������һ��λ��
		STWallRebarAssembly::movePoint(anchored_vec, using_pt, lenth);
		STWallRebarAssembly::movePoint(anchored_vec, mid_end_pt, lenth / 2);
		//����ֽ���ĩ�˵�û��ê���κ�ʵ��    
		if (!ISPointInHoles(alleehs, ori_Point))
		{
			type_Flag = 2;
			//PITCommonTool::CPointTool::DrowOnePoint(ori_Point, 1, 3);//��
			mdlElmdscr_extractEndPoints(&str_Pt, nullptr, &end_Pt, nullptr, lineEeh.GetElementDescrP(), ACTIVEMODEL);
			//ִ�иֽ���ԭʵ������
			GetIntersectPointsWithOldElm(tmppts, Eleeh, str_Pt, end_Pt, dSideCover, matrix);
			//��ԭʵ���н���
			if (tmppts.size() > 1)
			{
				//����һ��������ľ���
				CVector3D vectortepm = end_Pt - using_pt;
				vectortepm.Normalize();
				vectortepm.ScaleToLength(moveDis);
				using_pt.Add(vectortepm);
				lenth = moveDis + tmpendEndTypes.end.GetbendRadius();

				if (is_str)//˵������ʼ��
					mov_Point = tmppts[tmppts.size() - 1];
				else
					mov_Point = tmppts[tmppts.size() - 2];

				if (COMPARE_VALUES_EPS(abs(nowVec.z), 1, EPS) == 0)//��ֱ������
				{
					if (is_str)
						mov_Point.z = mov_Point.z + lenth;
					else
						mov_Point.z = mov_Point.z - lenth;
				}

				else if (COMPARE_VALUES_EPS(abs(nowVec.x), 1, 0.1) == 0 || COMPARE_VALUES_EPS(abs(nowVec.y), 1, 0.1) == 0)//ˮƽ������
				{
					if (is_str)
						mov_Point.x = mov_Point.x + lenth;
					else
						mov_Point.x = mov_Point.x - lenth;
				}
				if (!ISPointInHoles(alleehs, mov_Point))//�ж��Ƿ���ʵ���ڣ���������
				{
					anchored_vec.Negate();
				}
			}
		}
		//�ֽ�ĩ�˵���Ȼû��ê���κ�ʵ�壬��ʵ���ڽض�
		else if (!ISPointInHoles(alleehs, using_pt) || !ISPointInHoles(alleehs, mid_end_pt))
		{
			if (endtype_flag == 1)//�˲��仡Ϊֱ��
			{
				type_Flag = 2;
				using_pt = ori_Point;
				DPoint3d end_pt = ori_Point;
				anchored_vec.Negate();//����ԭ

				mov_Point = ori_Point;
				auto temp = ori_Point;
				movePoint(anchored_vec, using_pt, lenth);
				GetIntersectPointsWithHoles(tmppts, alleehs, using_pt, temp, dSideCover, matrix);
				GetIntersectPointsWithHolesByInsert(tmppts, alleehs, using_pt, ori_Point, dSideCover, matrix);

				//�����ԭģ���н���
				if (tmppts.size() > 0)
				{
					CVector3D vectortepm = end_pt - using_pt;
					vectortepm.Normalize();
					vectortepm.ScaleToLength(moveDis);
					using_pt.Add(vectortepm);
					//��ȡ���쳤��
					auto temp_pt = ori_Point;
					temp_pt.z = 0;
					tmppts[0].z = 0;
					//�ֽ��ߵ㵽����ľ������һ��������������Լ�����
					lenth = mdlVec_distance(&tmppts[0], &temp_pt) - 2 * tmpendEndTypes.end.GetbendRadius();
					//lenth = moveDis + tmpendEndTypes.end.GetbendRadius();
					using_pt = ori_Point;
					movePoint(anchored_vec, using_pt, lenth);

					//PITCommonTool::CPointTool::DrowOnePoint(using_pt, 1, 1);//��
					if (!ISPointInHoles(alleehs, using_pt))
					{
						anchored_vec.Negate();//����ԭ
						//lenth -= tmpendEndTypes.end.GetbendRadius();
					}

					if (is_str)//˵������ʼ��
						tmpendEndTypes.beg.SetbendLen(lenth);
					else
						tmpendEndTypes.end.SetbendLen(lenth);
				}
			}
			else//�˲��仡��Ϊֱ���п��ܻ������ģ��
			{
				type_Flag = 3;
			}
		}

		if (is_str)//˵������ʼ��
			tmpendEndTypes.beg.SetendNormal(anchored_vec);
		else//˵����ĩ�˵�
			tmpendEndTypes.end.SetendNormal(anchored_vec);

	}
}