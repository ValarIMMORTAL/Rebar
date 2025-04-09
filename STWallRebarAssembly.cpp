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

extern bool PreviewButtonDown;//主要配筋界面的预览按钮
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

// 提取面的关键点
void STWallRebarAssembly::ExtractBoundaryPoints(MSElementDescrP faceDescr, vector<DPoint3d>& vec_ptBoundary) {
	// 提取面边界关键点
	PITCommonTool::CElementTool::ExtractCellPoints(faceDescr, vec_ptBoundary);

	// 去重
	vector<DPoint3d> vec_uniPtBoundary;
	if (!vec_ptBoundary.empty()) {
		vec_uniPtBoundary.push_back(vec_ptBoundary[0]); // 保留第一个点

		for (size_t i = 1; i < vec_ptBoundary.size(); ++i) {
			const DPoint3d& lastPoint = vec_uniPtBoundary.back();
			const DPoint3d& currentPoint = vec_ptBoundary[i];

			// 比较两个点是否相同
			if (!lastPoint.IsEqual(currentPoint)) {
				vec_uniPtBoundary.push_back(currentPoint); // 不相同，保留
			}
		}
	}

	// 处理闭合多边形重合的起始端
	if (vec_uniPtBoundary.size() > 1 && vec_uniPtBoundary.front().IsEqual(vec_uniPtBoundary.back()))
		vec_uniPtBoundary.pop_back();

	// 将去重后的点放回 vec_ptBoundary
	vec_ptBoundary = std::move(vec_uniPtBoundary);
}

bool STWallRebarAssembly::makaRebarCurve(const vector<DPoint3d>& linePts, double extendStrDis, double extendEndDis,
	double diameter, double strMoveDis, double endMoveDis, bool isInSide,
	const PIT::PITRebarEndTypes& endTypes, vector<PIT::PITRebarCurve>& rebars, std::vector<EditElementHandle*> upflooreehs,
	std::vector<EditElementHandle*> flooreehs, std::vector<EditElementHandle*> Walleehs, std::vector<EditElementHandle*>alleehs)
{
	//根据提供的点连成线条
	EditElementHandle lineEeh;
	LineStringHandler::CreateLineStringElement(lineEeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
	//计算线的首尾点
	DPoint3d strPt = { 0,0,0 }, endPt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&strPt, nullptr, &endPt, nullptr, lineEeh.GetElementDescrP(), ACTIVEMODEL);
	//mdlElmdscr_add(lineEeh.GetElementDescrP());
	if (GetIntersectPointsWithNegs(m_Negs, strPt, endPt))
	{
		return false;
	}
	m_vecRebarPtsLayer.push_back(strPt);
	m_vecRebarPtsLayer.push_back(endPt);

	//计算和原始墙体相交后的点
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
	vector<DPoint3d> ptsIntersectTmp;//相交点
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
	EFT::GetSolidElementAndSolidHoles(ehWall, Eleeh, Holeehs);//获取开孔之前的墙体
	if (Eleeh.IsValid()) // 与原实体相交(无孔洞)
	{
		//Eleeh.AddToModel();
		GetIntersectPointsWithOldElm(tmpptsTmp, &Eleeh, strPt, endPt, dSideCover, matrix);
		if (tmpptsTmp.size() > 1)
		{
			// 存在交点为两个以上的情况
			GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, strPt, endPt, &Eleeh, dSideCover);
		}
		//  排除横向钢筋的情况
		// 获取 is_inside 的绝对值
		double is_transverse = std::abs(strPt.z - endPt.z);
		// 与开孔之后的模型不存在交点 并且 不为横向钢筋 的情况
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
	//计算规避板和孔洞后的点	
	for (size_t i = 0; i < vecPtRebars.size(); i++)
	{
		strPt = vecPtRebars.at(i).at(0);
		endPt = vecPtRebars.at(i).at(1);

		CVector3D  Vec(strPt, endPt);
		CVector3D  nowVec = Vec;
		double extendStrDistemp = extendStrDis, extendEndDistemp = extendEndDis;
		//根据顶底板重新计算偏移距离
		//PITCommonTool::CPointTool::DrowOneLine(strPt, endPt, 2);
		Vec.Normalize();
		if (COMPARE_VALUES_EPS(abs(Vec.x), 1, 0.1) == 0 || COMPARE_VALUES_EPS(abs(Vec.y), 1, 0.1) == 0 || COMPARE_VALUES_EPS(abs(Vec.z), 1, 1e-6) == 0)
		{
			ReCalExtendDisByTopDownFloor(strPt, endPt, strMoveDis, endMoveDis, extendStrDis, extendEndDis, isInSide);
		}
		//计算首尾移动向量
		DVec3d strVec = strPt - endPt;
		strVec.Normalize();
		strVec.ScaleToLength(extendStrDis);
		DVec3d endVec = endPt - strPt;
		endVec.Normalize();
		endVec.ScaleToLength(extendEndDis);
		//移动首尾点
		strPt.Add(strVec);
		endPt.Add(endVec);
		//PITCommonTool::CPointTool::DrowOneLine(strPt, endPt, 1);
		EditElementHandle* eeh = &Eleeh;
		//如果偏移的点是在自身孔洞里面说明上面处理跟镶嵌板相交了
		nowVec.Normalize();
		if (COMPARE_VALUES_EPS(abs(nowVec.z), 1, EPS) == 0)
		{
			//底板转向处理1.如果锚入未在底板内则锚入转向。2.如果转向后并未锚入到任何实体中，则还原反向,进行孔洞截断操作
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
			Transform tran;			//构造投影矩阵
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
				//将面投影到XOY平面
				mdlElmdscr_transform(&downface, &tran);
				DPoint3d vecwall = DPoint3d::From(0, 0, 0);
				GetDownFaceVecAndThickness(downface, vecwall);
				if (ISPointInElement(Walleehs[i], endtemp_point3d) && abs(vecwall1.DotProduct(vecwall) < 0.9))//垂直时才延长)
				{
					break;
				}
			}
			if (isOpen&&isInSide)//判断Z轴上方或者下方是否有墙并且是内侧钢筋
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

			//顶板转向处理1.如果锚入未在顶板内则锚入转向。2.如果转向后并未锚入到任何实体中，则还原反向,进行孔洞截断操作
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
				//将面投影到XOY平面
				mdlElmdscr_transform(&downface, &tran);
				DPoint3d vecwall = DPoint3d::From(0, 0, 0);
				GetDownFaceVecAndThickness(downface, vecwall);
				if (ISPointInElement(Walleehs[i], endtemp_point3d) && abs(vecwall1.DotProduct(vecwall) < 0.9))//垂直时才延长)
				{
					//isOpen = true;
					break;
				}
			}
			if (isOpen&&isInSide)//判断Z轴上方或者下方是否有墙并且是内侧钢筋
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
			DPoint3d  strtemp_point3d = strPt, endtemp_point3d = endPt;//起始点
			CVector3D str_vector = endTypes.beg.GetendNormal();//起始点方向
			double str_extend_lenth = endTypes.beg.GetbendLen() + endTypes.beg.GetbendRadius();//起始点延伸长度
			CVector3D end_vector = endTypes.end.GetendNormal();//末尾点方向
			double end_extend_lenth = endTypes.end.GetbendLen() + endTypes.end.GetbendRadius();//末尾点延伸长度
			CVector3D vectorZ = CVector3D::From(0, 0, 0);
			double lae = get_lae() * diameter / uor_per_mm;
			double der = diameter * 15;
			//判断缩短或者延长后的点是否在墙内
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

			//判断是否锚空
			EditElementHandle* eeh = &Eleeh;
			//判断起始点和起始点锚入位置是否锚出
			JudgeBarLinesLegitimate(nowVec, alleehs, tmpendEndTypes, lineEeh, eeh, str_vector, matrix, strMoveDis, str_extend_lenth, strtemp_point3d, str_Ptr, flag_str);
			//尾端点和尾端点位置是否锚出
			JudgeBarLinesLegitimate(nowVec, alleehs, tmpendEndTypes, lineEeh, eeh, end_vector, matrix, endMoveDis, end_extend_lenth, endtemp_point3d, end_Ptr, flag_end);
		}

		DPoint3d strPttepm = strPt, endPttepm = endPt;

		GetIntersectPointsWithHoles(tmppts, m_useHoleehs, strPt, endPt, dSideCover, matrix);

		vector<DPoint3d> aro_tmppts;//周围实体孔洞交点
		vector<DPoint3d> uniquePoints;
		GetIntersectPointsWithHoles(aro_tmppts, m_around_ele_holeEehs, strPt, endPt, dSideCover, matrix);


		for (size_t j = 0; j < aro_tmppts.size(); j++) {
			bool isUnique = true;

			for (size_t i = 0; i < uniquePoints.size(); i++) {
				// 检查点的唯一性
				if (uniquePoints[i].x + 1 > aro_tmppts[j].x &&
					uniquePoints[i].x - 1 < aro_tmppts[j].x &&
					uniquePoints[i].y + 1 > aro_tmppts[j].y &&
					uniquePoints[i].y - 1 < aro_tmppts[j].y &&
					uniquePoints[i].z + 1 > aro_tmppts[j].z &&
					uniquePoints[i].z - 1 < aro_tmppts[j].z) {
					isUnique = false; // 找到了相似的点
					break; // 可以提前退出循环
				}
			}

			if (isUnique) {
				uniquePoints.push_back(aro_tmppts[j]); // 添加唯一的点
			}
		}

		//判断底板钢筋锚入板后是否碰到孔洞或者伸出了板,如果出现了这种情况则需要反转当前端部方向，只处理竖直方向
		if (tmppts.empty() && !aro_tmppts.empty()) {
			for (auto point : uniquePoints) {
				//PITCommonTool::CPointTool::DrowOnePoint(point, 1, 3); // 绿
				tmppts.push_back(point);
			}
		}

		//根据规避孔洞后的点将点按距离分类，两两一组，等到钢筋线的首尾点
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
		//根据保护层、延长值重新绘制基础钢筋线串
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

	EditElementHandle ehWall(GetSelectedElement(), GetSelectedModel());//自身

	std::vector<EditElementHandle*> upflooreehs;
	std::vector<EditElementHandle*> flooreehs;
	std::vector<EditElementHandle*> Walleehs;
	std::vector<EditElementHandle*> alleehs;
	m_around_ele_holeEehs.clear();
	for (IDandModelref IdMode : m_walldata.floorID)//中间板
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//计算需要元素周围的孔洞
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
	for (IDandModelref IdMode : m_walldata.downfloorID)//底板
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//计算需要元素周围的孔洞
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
	for (IDandModelref IdMode : m_walldata.upfloorID)//顶板
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//计算需要元素周围的孔洞
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
	for (IDandModelref IdMode : m_walldata.wallID)//加上墙(只有顶板锚入需要考虑墙)
	{
		EditElementHandle fllorEh(IdMode.ID, IdMode.tModel);
		if (fllorEh.IsValid())
		{
			EditElementHandle* eeh = new EditElementHandle();
			eeh->Duplicate(fllorEh);
			ISolidKernelEntityPtr entityPtr;
			//计算需要元素周围的孔洞
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

	EditElementHandle *eeh = new EditElementHandle();//顶板还需要加入自身
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
		//计算端部样式
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

		//计算钢筋线
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
		{//若不是预览状态下则生成钢筋
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
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数
	rebarSet->SetSetData(setdata);
	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);

	return tag;
}

void STWallRebarAssembly::CalRebarEndTypes(const BarLinesdata & data, BrStringCR sizeKey, PIT::PITRebarEndTypes & pitRebarEndTypes, DgnModelRefP modelRef)
{
	//根据锚入方向与钢筋方向的夹角算端部类型
	auto GetEndType = [&](const CVector3D& vec1, const CVector3D& vec2, PIT::PITRebarEndType& endType)->int {
		double angle = vec1.AngleTo(vec2);
		endType.Setangle(angle);
		if (COMPARE_VALUES_EPS(angle, PI / 2, 0.1) == 0) //90度
			return PIT::PITRebarEndType::Type::kBend;
		if (COMPARE_VALUES_EPS(angle, 3 / 4 * PI, 0.1) == 0) //135度
			return PIT::PITRebarEndType::Type::kCog;
		if (COMPARE_VALUES_EPS(angle, 0, 0.1) == 0) //180度弯锚
			return PIT::PITRebarEndType::Type::kHook;
		if (COMPARE_VALUES_EPS(angle, PI, 0.1) == 0) //直锚
			return PIT::PITRebarEndType::Type::kLap;
		else
			return PIT::PITRebarEndType::Type::kCustom;
	};

	CVector3D strVec = data.vecstr;
	CVector3D endVec = data.vecend;
	CVector3D HolVec = data.vecHoleehs;
	strVec.Normalize();
	endVec.Normalize();
	//获取锚入参数
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	//默认没有端部样式
	PIT::PITRebarEndType strEndType, endEndType, HolEndType;
	strEndType.SetType(PIT::PITRebarEndType::Type::kNone);
	//计算钢筋线首尾
	DPoint3d lineStrPt = { 0,0,0 }, lineEndPt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&lineStrPt, nullptr, &lineEndPt, nullptr, data.barline, modelRef);
	//计算首尾端部样式
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
* @desc:		根据顶底板重新计算伸缩距离
* @param[in]	strPt 钢筋线起点
* @param[in]	endPt 钢筋线重点
* @param[in]	strMoveLen 起点端移动距离，不带弯钩是保护层，带弯钩是保护层+钢筋半径
* @param[in]	endMoveLen 终点端移动距离，不带弯钩是保护层，带弯钩是保护层+钢筋半径
* @param[out]	extendStrDis 起点伸缩距离
* @param[out]	extendEndDis 终点伸缩距离
* @author	Hong ZhuoHui
* @Date:	2023/09/20
*/
void STWallRebarAssembly::ReCalExtendDisByTopDownFloor(const DPoint3d & strPt, const DPoint3d & endPt, double strMoveLen,
	double endMoveLen, double & extendStrDis, double & extendEndDis, bool isInSide)
{
	//获取墙体
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
	//1.计算最小的点和最大的点
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

	//2.把最小和最大点投影到钢筋线上
	DPoint3d minProPt = minPt, maxProPt = maxPt;
	mdlVec_projectPointToLine(&minProPt, nullptr, &minPt, &strPt, &endPt);
	mdlVec_projectPointToLine(&maxProPt, nullptr, &maxPt, &strPt, &endPt);

	//3.根据到投影点的距离计算钢筋线延长距离
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

	//4.延长钢筋线
	DPoint3d newStrPt = strPt, newEndPt = endPt;
	//4.1计算首尾移动向量
	DVec3d strVec = strPt - endPt;
	strVec.Normalize();
	strVec.ScaleToLength(strMoveDis + strMoveLen); //movelen是用来修正误差的，因为钢筋线是斜的，投影点有点误差
	DVec3d endVec = endPt - strPt;
	endVec.Normalize();
	endVec.ScaleToLength(endMoveDis + endMoveLen);
	//4.2移动首尾点
	newStrPt.Add(strVec*1.5);
	newEndPt.Add(endVec*1.5);
	//PITCommonTool::CPointTool::DrowOneLine(newStrPt, newEndPt, 11);
	//5.和所有板求交，得到起始点端的交点和终点端的交点集合
	vector<DPoint3d> interStrPts, interEndPts, allPts; //交点
	int  numAnchors = 0;//锚入次数判断标准
	bool isStrRecorded = false; // 标志起点是否遇到板
	bool isEndRecorded = false; // 标志终点是否遇到板
	auto calInterPts = [&](IDandModelref floor, bool islsay = true) {
		EditElementHandle floorEeh(floor.ID, floor.tModel);
		if (!floorEeh.IsValid())
			return;

		//5.1.1 过滤不在墙范围内的板		
		DPoint3d lowPt = { 0,0,0 }, highPt = { 0,0,0 };
		mdlElmdscr_computeRange(&lowPt, &highPt, floorEeh.GetElementDescrP(), nullptr);
		DRange3d  vecRange;
		vecRange.low = wallLowPt;
		vecRange.high = wallHighPt;
		strVec.Normalize();
		int strRecord = 2; // 遇到板的交点计数
		int endRecord = 2; // 遇到板的交点计数
		if (islsay)
		{
			if ((COMPARE_VALUES_EPS(lowPt.z, wallLowPt.z - 50 * UOR_PER_MilliMeter, 1e-6) == 1 && //板最低点在墙之间
				COMPARE_VALUES_EPS(highPt.z, wallHighPt.z + 50 * UOR_PER_MilliMeter, 1e-6) == -1) &&
				COMPARE_VALUES_EPS(wallHighPt.z, highPt.z, 1e-6) == 0) //板最高点在墙之间
			{
				if (vecRange.IsContainedXY(lowPt) && wallLowPt.DistanceXY(lowPt) > 10 && COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0) //板最低点不在墙范围,板最高点不在墙范围
				{
					return;
				}

				if (vecRange.IsContainedXY(highPt) && wallHighPt.DistanceXY(highPt) > 10 && COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0) //板最低点不在墙范围,板最高点不在墙范围
				{
					return;
				}
			}
			MSElementDescrP have_intersect = nullptr;//实体和面相交
			if (!PITCommonTool::CSolidTool::SolidBoolWithFace(have_intersect, wallEeh.GetElementDescrP(), floorEeh.GetElementDescrP(), BOOLOPERATION::BOOLOPERATION_INTERSECT)
				&& COMPARE_VALUES_EPS(lowPt.z, wallHighPt.z - 50 * UOR_PER_MilliMeter, 1e-6) == -1
				&& COMPARE_VALUES_EPS(highPt.z, wallHighPt.z, 1e-6) == 1
				&& highPt.z - wallHighPt.z < 55 * UOR_PER_MilliMeter)//如果板的最低点在墙之间但板的最高点不在墙之间且板和墙不相接
			{
				return;
			}
		}
		/*PITCommonTool::CPointTool::DrowOnePoint(wallLowPt, 1, 5);
		PITCommonTool::CPointTool::DrowOnePoint(wallHighPt, 1, 6);
		PITCommonTool::CPointTool::DrowOnePoint(lowPt, 1, 1);
		PITCommonTool::CPointTool::DrowOnePoint(highPt, 1, 2);*/


		if ((COMPARE_VALUES_EPS(lowPt.z, wallLowPt.z + 80 * UOR_PER_MilliMeter, 1e-6) == -1 ||
			COMPARE_VALUES_EPS(lowPt.z, wallHighPt.z - 80 * UOR_PER_MilliMeter, 1e-6) == 1) && //板最低点不在墙之间
			(COMPARE_VALUES_EPS(highPt.z, wallLowPt.z + 80 * UOR_PER_MilliMeter, 1e-6) == -1 ||
				COMPARE_VALUES_EPS(highPt.z, wallHighPt.z - 80 * UOR_PER_MilliMeter, 1e-6) == 1) &&//板最高点不在墙之间
			COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0)//钢筋线方向Z不为1
		{

			//5.1.1 得到这个板的包围盒投影到钢筋线上的点
			DPoint3d floorMinProPt = lowPt, floorMaxProPt = highPt;
			mdlVec_projectPointToLine(&floorMinProPt, nullptr, &lowPt, &strPt, &endPt);
			mdlVec_projectPointToLine(&floorMaxProPt, nullptr, &highPt, &strPt, &endPt);
			//5.1.2 计算板线的方向和钢筋线方向关系，得到板线在钢筋线方向上的起始和终点
			DPoint3d floorStrPt = floorMinProPt, floorEndPt = floorMaxProPt;
			DVec3d vec1 = strPt - endPt; vec1.Normalize();
			DVec3d vec2 = floorMinProPt - floorMaxProPt; vec2.Normalize();
			if (vec1.DotProduct(vec2) < 0) //不同方向
			{
				floorStrPt = floorMaxProPt;
				floorEndPt = floorMinProPt;
			}
			//5.1.3 如果板线的起始点和钢筋线的终点的距离 或者 板线的终点和钢筋线的起点的距离 小于钢筋线的1/5 过滤板
			double dis = strPt.Distance(endPt) / 3;
			if (COMPARE_VALUES_EPS(floorStrPt.Distance(endPt), dis, 1e-6) == -1 ||
				COMPARE_VALUES_EPS(floorEndPt.Distance(strPt), dis, 1e-6) == -1)
			{
				return;
			}
		}
		//5.2 计算与板交点
		vector<DPoint3d> interPts;
		//PITCommonTool::CPointTool::DrowOneLine(newStrPt, newEndPt, 1);
		//GetIntersectPointsWithOldElm(interPts, &floorEeh, newStrPt, newEndPt);
		//5.1.2 过滤不在墙范围内的墙
		//计算指定元素描述符中元素的范围。
		DPoint3d range_data_low = { 0,0,0 }, range_data_high = { 0,0,0 };
		EditElementHandle wallEeh(GetSelectedElement(), GetSelectedModel());//获取选择model的参数范围
		DPoint3d wallLowPt = { 0,0,0 }, wallHighPt = { 0,0,0 };
		mdlElmdscr_computeRange(&wallLowPt, &wallHighPt, wallEeh.GetElementDescrP(), nullptr);
		for (int i = 0; i < m_walldata.cutWallfaces.size(); i++)//对周围的墙的底面进行筛选
		{
			DPoint3d wallLow_Pt = { 0,0,0 }, wallHigh_Pt = { 0,0,0 };
			EditElementHandle wall_Eeh(m_walldata.wallID[i].ID, m_walldata.wallID[i].tModel);
			mdlElmdscr_computeRange(&wallLow_Pt, &wallHigh_Pt, wall_Eeh.GetElementDescrP(), nullptr);//获取选择周围墙的参数范围

			if (COMPARE_VALUES(wallLow_Pt.z, wallLowPt.z) == 1 && COMPARE_VALUES(wallHigh_Pt.z, wallHighPt.z) == -1)//如果周围墙在选择墙的range范围内
			{
				range_data_low = wallLow_Pt;
				range_data_high = wallHigh_Pt;
			}
			//判断是否需要单次判断，如果周围墙在选择墙的range范围内，对周围的板只需要锚入一次，不可以多次锚入
			if ((numAnchors != 2 && (COMPARE_VALUES(strPt.z, range_data_low.z) == 1 && COMPARE_VALUES(endPt.z, range_data_low.z) == 1)
				&& (COMPARE_VALUES(strPt.z, range_data_high.z) == -1 && COMPARE_VALUES(endPt.z, range_data_high.z) == -1)) ||
				(numAnchors != 2 && (COMPARE_VALUES(strPt.x, range_data_low.x) == 1 && COMPARE_VALUES(endPt.x, range_data_low.x) == 1)
					&& (COMPARE_VALUES(strPt.x, range_data_high.x) == -1 && COMPARE_VALUES(endPt.x, range_data_high.x) == -1)))
			{
				numAnchors = 1;
			}
		}
		if (numAnchors == 1)//单次锚入
		{
			GetIntersectPointsWithHole(interPts, &floorEeh, newStrPt, newEndPt);
			numAnchors = 2;
		}
		if (numAnchors == 0)//可以多次锚入
		{
			GetIntersectPointsWithHole(interPts, &floorEeh, newStrPt, newEndPt);
			//PITCommonTool::CPointTool::DrowOnePoint(newStrPt, 1, 3);//红
			//PITCommonTool::CPointTool::DrowOnePoint(newEndPt, 1, 4);//红
		}
		//5.3 遍历所有交点，如果不存在一个与钢筋线端点极近的点，则过滤这个板（墙板之间没有交）
		bool isValid = false;
		if (Pts.size() == 0)//如果跟墙没有交点说明是延申部分，不需要进行过滤
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
			// 获取楼板厚度
			double thickness = GetFloorThickness(floorEeh) * UOR_PER_MilliMeter;
			double dSideCover = GetSideCover() * UOR_PER_MilliMeter;
			// 按与起点的距离排序交点
			std::sort(interPts.begin(), interPts.end(), [&](const DPoint3d& a, const DPoint3d& b) {
				return COMPARE_VALUES_EPS(strPt.Distance(a), strPt.Distance(b), UOR_PER_MilliMeter) < 0;
			});

			// 添加水平钢筋延伸到板厚度限制逻辑
			if (islsay && isInSide && interPts.size() >= 2 && isHorizon) {
				// 计算相邻交点之间的距离，限制不超过楼板厚度
				for (size_t i = 0; i < interPts.size() - 1; ++i) {
					double distBetweenPts = interPts[i].Distance(interPts[i + 1]);
					if (COMPARE_VALUES_EPS(distBetweenPts, thickness, UOR_PER_MilliMeter) > 0) {
						// 如果交点间距离超过楼板厚度，调整第二个交点位置，且结束延伸
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
				// 提取板的所有关键点
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
					// 将关键点投影到钢筋线上
					DPoint3d projectedPt;
					double outFractionP;
					mdlVec_projectPointToLine(&projectedPt, &outFractionP, &boundaryPt, &interPts.front(), &interPts.back());

					// 计算关键点与投影点在Z方向上的距离
					double distZ = abs(boundaryPt.z - projectedPt.z);

					// 检查投影点是否在钢筋线的起止点范围内
					bool isWithinRange = (COMPARE_VALUES(outFractionP, 0) >= 0
						&& COMPARE_VALUES(outFractionP, 1) <= 0);

					// 如果距离小于保护层厚度，且投影点在起止点范围内，调整第二个交点位置，且结束延伸。
					// 如果投影点于关键点实际距离过大，可能是孔洞之类的影响，只考虑一定范围内的关键点
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
				if (COMPARE_VALUES_EPS(strDis, 50 * UOR_PER_MilliMeter, 1e-6) <= 0 || //起始点与端点极近
					COMPARE_VALUES_EPS(endDis, 50 * UOR_PER_MilliMeter, 1e-6) <= 0 ||	//终点与端点极近
					COMPARE_VALUES_EPS(totalDis, strDis + endDis, 1) == 0 ||//交点在钢筋线中间
					vecRange.IsContainedXY(it))//点在墙XY平面的范围内
				{
					isValid = true;
					break;
				}
				// 新增条件：检查交点是否接近 interStrPts 或 interEndPts。此为判断锚入点是否可以跨实体
				// 检查交点是否接近起点端的交点集合
				for (auto strPtInters : interStrPts) {
					if (islsay && COMPARE_VALUES_EPS(strPtInters.Distance(it), 50 * UOR_PER_MilliMeter, 1e-6) <= 0) {
						isValid = true;
						break;
					}
				}

				if (isValid) {
					break;  // 如果已确定线可以延伸，跳出循环
				}

				// 检查交点是否接近终点端的交点集合
				for (auto endPtInters : interEndPts) {
					if (islsay && COMPARE_VALUES_EPS(endPtInters.Distance(it), 50 * UOR_PER_MilliMeter, 1e-6) <= 0) {
						isValid = true;
						break;
					}
				}

				if (isValid) {
					break;  // 如果已确定线可以延伸，跳出循环
				}
			}
		}
		if (!isValid)
			return;
		endVec.Normalize();
		//5.4 记录点
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
			// 处理交点并分配到起点端或终点端
			auto processIntersectionPoints = [&](DPoint3d point, vector<DPoint3d>& vec_interPts, bool& isRecorded, int& record) -> bool{
				if (!islsay)
				{
					if (!isRecorded)//墙实体且未遇到过板，正常延伸
						vec_interPts.push_back(point);
					return false;
				}
				if (vec_interPts.size() < 2 || record == 1)//初次遇到板，或新板的另一个交点，正常延伸，需记录为板
				{
					vec_interPts.push_back(point);
					isRecorded = true;
					return false;
				}
				if (!isRecorded)//未遇到过板，正常延伸
				{
					if (record == 2)//墙板墙顺序，需要保留板的两个交点，且需记录为板
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
					
					// 多次遇到板，选择近的板
					interPtsSort(interStrPts, endPt);
					double tempStrDis = strPt.Distance(interStrPts.back());
					if (COMPARE_VALUES_EPS(strDis, tempStrDis, 1e-6) == -1)//新的交点在更近的板
					{
						if (strRecord == 2)//仅在遇到板时清空，需要保留新板的两个交点
							interStrPts.clear();
						interStrPts.push_back(it);
						strRecord--;
					}
				}
				else
				{
					if (!processIntersectionPoints(it, interEndPts, isEndRecorded, endRecord))
						continue;
					
					// 多次遇到板，选择近的板
					interPtsSort(interEndPts, strPt);
					double tempEndDis = endPt.Distance(interEndPts.back());
					if (COMPARE_VALUES_EPS(endDis, tempEndDis, 1e-6) == 1)//新的交点在更近的板
					{
						if (endRecord == 2)//仅在遇到板时清空，需要保留新板的两个交点
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
	if (!((COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) == 0) && isInSide))//过滤掉竖向内侧钢筋
	{
		for (auto it : m_walldata.wallID)
		{
			calInterPts(it, false);
		}
	}
	if (!((COMPARE_VALUES_EPS(abs(strVec.z), 1, 1e-6) != 0) && isInSide))//过滤水平向内侧钢筋
	{
		for (auto it : m_walldata.floorID)
		{
			calInterPts(it, false);
		}
	}

	//墙后可能还需要延伸至板
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

	//7.计算伸缩值
	if (interStrPts.size() > 0)
	{
		//PITCommonTool::CPointTool::DrowOnePoint(*interStrPts.begin(), 1, 3);
		extendStrDis = strPt.Distance(*interStrPts.begin());
		//根据法向计算距离正负
		strVec.Normalize();
		DVec3d extendStrVec = *interStrPts.begin() - strPt;
		extendStrVec.Normalize();
		if (extendStrVec.DotProduct(strVec) < 0) //反方向
			extendStrDis = extendStrDis * -1;
		//减掉保护层
		extendStrDis -= strMoveLen;
	}


	if (interEndPts.size() > 0)
	{
		//PITCommonTool::CPointTool::DrowOnePoint(*interEndPts.begin(), 1, 4);
		extendEndDis = endPt.Distance(*interEndPts.begin());
		//根据法向计算距离正负
		endVec.Normalize();
		DVec3d extendEndVec = *interEndPts.begin() - endPt;
		extendEndVec.Normalize();
		if (extendEndVec.DotProduct(endVec) < 0) //反方向
			extendEndDis = extendEndDis * -1;
		//减掉保护层
		extendEndDis -= endMoveLen;
	}


}

//获取锚固长度
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
	double diameter = RebarCode::GetBarDiameter(strSize, ACTIVEMODEL);		//乘以了10
	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;

	DPoint3d vecOutwall = m_walldata.vecToWall;//远离墙内的偏移方向
	vecOutwall.Negate();
	if (side != 0) vecOutwall.Negate();
	DPoint3d pt11, pt22;
	mdlElmdscr_extractEndPoints(&pt11, nullptr, &pt22, nullptr, barline, ACTIVEMODEL);
	double lengh = pt11.Distance(pt22) / 3;//移动到1/3的点
	DPoint3d vecLine = pt22 - pt11;//起始线串反方向
	vecLine.Normalize();
	vecLine.Scale(lengh);//延长
	//1、计算路径线长度
	//首先判断上侧有没有板，有板的话是否左右侧都在板的范围内，如果都在说明为内侧面（可以不用延长路径线）
	//如果只有一侧说明为外侧面，需要延长
	DPoint3d pt1, pt2;
	mdlElmdscr_extractEndPoints(&pt1, nullptr, &pt2, nullptr, path, ACTIVEMODEL);
	DPoint3d Tmept1 = pt1, Tmeppt2 = pt2;
	double D1 = 0, D2 = 0;
	ExtendLineByFloor(m_walldata.upfloorfaces, m_walldata.upfloorID, pt2, pt1, vecLine, m_walldata.upfloorth - diameter / uor_per_mm, D1, vecOutwall);
	ExtendLineByFloor(m_walldata.downfloorfaces, m_walldata.downfloorID, pt1, pt2, vecLine, m_walldata.downfloorth - diameter / uor_per_mm, D2, vecOutwall);


	if (Tmept1.Distance(pt1) > 1)//下半
	{
		DPoint3d temp = Tmept1;
		pt1.z -= D2;
		temp.z = temp.z - diameter + D2;
		EditElementHandle tmpeeh;
		LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(pt1, temp), true, *ACTIVEMODEL);
		MSElementDescrP tmppath = tmpeeh.ExtractElementDescr();
		allpaths.push_back(tmppath);
	}
	//中间
	EditElementHandle tmpeeh;
	LineHandler::CreateLineElement(tmpeeh, nullptr, DSegment3d::From(Tmept1, Tmeppt2), true, *ACTIVEMODEL);
	MSElementDescrP tmppath = tmpeeh.ExtractElementDescr();
	allpaths.push_back(tmppath);

	if (Tmeppt2.Distance(pt2) > 1)//上半
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
	//2、计算钢筋线长度和锚固长度、锚固方向
	//求钢筋线与墙面交点，
	//(1)如果有交点，直接延长钢筋线到交点处
	DPoint3d facenormal;
	DPoint3d maxpt;
	mdlElmdscr_extractNormal(&facenormal, &maxpt, m_walldata.downFace, NULL);
	DPoint3d cpt = getCenterOfElmdescr(m_walldata.downFace);
	facenormal.Normalize();
	DPlane3d plane = DPlane3d::FromOriginAndNormal(cpt, DVec3d::From(facenormal.x, facenormal.y, facenormal.z));
	vector<DPoint3d> barpts;//取到所有钢筋线点集合
	ExtractLineStringPoint(barline, barpts);

	//double L0 = 24 * uor_per_mm * 15;//15d
	//double Lae = 24 * uor_per_mm * 48 * 0.8;//48d
	double L0 = diameter1 * 15;//15d
	double Lae = diameter1 * get_lae() * 0.8 / uor_per_mm * 2;//LAE
	double mgDisHol = 0;
	//if (GetvecDataExchange().at(index) == 0)//外侧面
	//{
	//	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - 2 * diameter;
	//}
	//else if (GetvecDataExchange().at(index) == 2)//内侧面
	//{
	//	
	//}

	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - diameter;
	for (auto it : allpaths)
	{
		if (barpts.size() > 1)
		{//开始重新计算钢筋线起始点
			DPoint3d ptstrtmp = barpts[0];
			DPoint3d ptendtmp = barpts[barpts.size() - 1];
			double mgDisstr = 0;
			DPoint3d mgVecstr;
			mgDisstr = GetExtendptByWalls(ptstrtmp, barpts[1], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecstr);
			double mgDisend = 0;
			DPoint3d mgVecend;
			mgDisend = GetExtendptByWalls(ptendtmp, barpts[barpts.size() - 2], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecend);
			//linestringeeh.AddToModel();
		   //（2）钢筋线起始端计算线两边是否有在墙内的点（长度首先按超过墙厚的长度计算），如果没有说明此墙左右都没有其他墙，直接断开处理
		   //如果有：再用1/2的厚度长度计算另一侧点是否在墙内，在的话，说明此线为内侧墙面，长度按15d,方向为远离墙内的方向，钢筋线剪短值 = 侧面保护层 + 对侧钢筋直径和 
		   //如果只有一侧有交点,说明为外侧面锚入，长度按0.8LAE,方向为朝墙内方向，钢筋线剪短值 =  侧面保护层。
			//(3)钢筋线终止端计算，按步骤（2）
			BarLinesdata tmpbarline;
			tmpbarline.path = it;
			mdlElmdscr_duplicate(&tmpbarline.barline, barline);
			tmpbarline.spacing = GetvecDirSpacing().at(index)*uor_per_mm;
			tmpbarline.strDis = dSideCover + GetvecStartOffset().at(index)*uor_per_mm + diameter / 2;
			tmpbarline.endDis = dSideCover + GetvecEndOffset().at(index)*uor_per_mm + diameter / 2;
			tmpbarline.diameter = diameter;

			tmpbarline.extendstrDis = ptstrtmp.Distance(barpts[0]) - dSideCover;
			//根据锚入长度判断延长长度
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

			////测试代码
			//barpts[0] = ptstrtmp;
			//barpts[barpts.size() - 1] = ptendtmp;
			////重新绘制钢筋线
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
	double diameter = RebarCode::GetBarDiameter(strSize, ACTIVEMODEL);		//乘以了10
	double diameter1 = stoi(strSize.GetWCharCP())*uor_per_mm;
	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double diameterStr = 0, diameterEnd = 0;
	DPoint3d vecOutwall = m_walldata.vecToWall;//远离墙内的偏移方向
	vecOutwall.Negate();
	if (side != 0) vecOutwall.Negate();
	DPoint3d pt11, pt22;
	mdlElmdscr_extractEndPoints(&pt11, nullptr, &pt22, nullptr, path, ACTIVEMODEL);
	double lengh = pt11.Distance(pt22) / 3;//移动到1/3点
	DPoint3d vecLine = pt22 - pt11;//起始线串反方向
	vecLine.Normalize();
	vecLine.Scale(lengh);//延长
	//1、计算路钢筋线长度、钢筋的锚入长度和锚入方向
	//首先判断上侧有没有板，有板的话是否左右侧都在板的范围内，如果都在说明为内侧面（钢筋线延长 = 板厚度 - 保护层 - 对侧钢筋直径和，钢筋锚入长度为15d,锚入方向为远离墙的那一侧）
	//如果只有一侧说明为外侧面（钢筋线延长 = 板厚度 - 保护层,钢筋锚入长度为Lae,锚入方向为有板的那一侧）
	DPoint3d pt1, pt2;
	mdlElmdscr_extractEndPoints(&pt1, nullptr, &pt2, nullptr, barline, ACTIVEMODEL);
	bool isInsidestr = false;
	bool isHavefloorstr = false;
	isHavefloorstr = CalculateBarLineDataByFloor(m_walldata.downfloorfaces, m_walldata.downfloorID, pt1, pt2, vecLine, m_walldata.downfloorth, vecOutwall, isInsidestr, diameterStr);
	//测试代码显示当前的判断点的位置
	double mgDisstr = 0;//锚固长度
	double extendstr = 0;//延升长度
	DPoint3d mgVecstr;//锚固方向
	DPoint3d ptstr = pt1;
	DPoint3d ptend = pt2;
	if (isHavefloorstr)//如果有板
	{
		if (isInsidestr)//在内侧
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

		//绘制锚固方向长度线
		Dpoint3d mgpt = ptstr;
		mgVecstr.Scale(mgDisstr);
		mgpt.Add(mgVecstr);
		mgVecstr.Normalize();

		EditElementHandle theeh;
		LineHandler::CreateLineElement(theeh, nullptr, DSegment3d::From(ptstr, mgpt), true, *ACTIVEMODEL);
		//theeh.AddToModel();
	}
	else//如果竖向方向上方没有板，钢筋需要锚入到墙体内部，锚入长度为墙厚 - （前保护层+后保护层）
	{
		mgDisstr = m_walldata.thickness - dPositiveCover - dReverseCover /*- bendRadius*/ - 1.5*diameter1;
		// 如果竖向上方没有板，则不区分内外侧以及钢筋偏移
		// if (isInsidestr)//在内侧
		// {
		// 	extendstr = -1 * dPositiveCover - allfdiam;
		// }
		// else
		{
			extendstr = -1 * dPositiveCover;
		}

		mgVecstr = vecOutwall;
		//mgVecstr.Negate();
		//绘制锚固方向长度线
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
	double mgDisend = 0;//锚固长度
	double mgDisHol = 0;//锚固长度
	double extendend = 0;//延升长度
	DPoint3d mgVecend;//锚固方向
	if (isHavefloorend)//如果有板
	{
		if (isInsideend)//在内侧
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

		//绘制锚固方向长度线
		Dpoint3d mgpt = ptend;
		mgVecend.Scale(mgDisstr);
		mgpt.Add(mgVecend);
		mgVecend.Normalize();

		EditElementHandle theeh;
		LineHandler::CreateLineElement(theeh, nullptr, DSegment3d::From(ptend, mgpt), true, *ACTIVEMODEL);
		//theeh.AddToModel();
	}
	else//如果竖向方向上方没有板，钢筋需要锚入到墙体内部，锚入长度为墙厚 - （前保护层+后保护层）
	{
		mgDisend = m_walldata.thickness - dPositiveCover - dReverseCover/* - bendRadius*/ - 1.5*diameter1;
		extendend = 0;
		// 如果竖向上方没有板，则不区分内外侧以及钢筋偏移
		// if (isInsidestr)//在内侧
		// {
		// 	extendend = -1 * dPositiveCover - allfdiam;
		// }
		// else
		{
			extendend = -1 * dPositiveCover;
		}
		mgVecend = vecOutwall;
		//mgVecend.Negate();
		//绘制锚固方向长度线
		Dpoint3d mgpt = ptend;
		mgVecend.Scale(mgDisend);
		mgpt.Add(mgVecend);
		mgVecend.Normalize();

		EditElementHandle theeh;
		LineHandler::CreateLineElement(theeh, nullptr, DSegment3d::From(ptend, mgpt), true, *ACTIVEMODEL);
		//theeh.AddToModel();
	}
	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - 2 * diameter;
	//if (GetvecDataExchange().at(index) == 0)//外侧面
	//{
	//	mgDisHol = m_walldata.thickness - dPositiveCover - dReverseCover - 2 * diameter;
	//}
	//else if (GetvecDataExchange().at(index) == 2)//内侧面
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
	//2、计算路径线，首先确认有没有靠在当前墙上面的其他墙体，如果有计算出其他墙体方向，要与路径线垂直，不垂直的过滤掉；
	//延长路径线，将路径线移动到其他墙体中心位置处，求出路径线与其他墙体线交点，将交点投影到路径线上，并记录map<int,Dpoint3d>,第一个值为离路径线起点距离，第二个为投影点；
	//根据投影点，计算新的路径线，如果新路径线前起点为投影点，且不为原始路径线的起点或终点，需要延长，延长长度为：侧面保护层*2 + 钢筋直径
	//如果新路径线终点为投影点，且不为原始路径线的起点或终点，需要延长，延长长度为：侧面保护层*2 + 钢筋直径
	GetCutPathLines(barlines, dSideCover, diameter1, path, m_walldata.cutWallfaces, m_walldata.downFace, m_walldata.height);//处理靠在当前墙体上墙
	//3、根据新路径构建的钢筋线和路径线，设置面数据
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
		vector<DPoint3d> barpts;//取到所有钢筋线点集合
		ExtractLineStringPoint(barlines.at(i).path, barpts);
		if (barpts.size() > 1)
		{//开始重新计算钢筋线起始点
			DPoint3d ptstrtmp = barpts[0];
			DPoint3d ptendtmp = barpts[barpts.size() - 1];
			double mgDisstr = 0;
			DPoint3d mgVecstr;
			mgDisstr = GetExtendptByWalls(ptstrtmp, barpts[1], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecstr);
			double mgDisend = 0;
			DPoint3d mgVecend;
			mgDisend = GetExtendptByWalls(ptendtmp, barpts[barpts.size() - 2], m_walldata.thickness, m_walldata.downFace, m_walldata.cutWallfaces, plane, cpt, Lae, L0, mgVecend);
			//根据锚入长度判断延长长度
			if (ptstrtmp.Distance(barpts[0]) > 10 && abs(mgDisstr - L0) > 100)//外侧钢筋需要延长
			{
				//往回缩一个钢筋间距，保证最后一根钢筋不会重叠
				DPoint3d tmpptvec = barpts[0] - ptstrtmp;
				tmpptvec.Normalize();
				tmpptvec.Scale(barlines.at(i).spacing);
				ptstrtmp.Add(tmpptvec);
				barpts[0] = ptstrtmp;
			}
			if (ptendtmp.Distance(barpts[barpts.size() - 1]) > 10 && abs(mgDisend - L0) > 100)
			{
				//往回缩一个钢筋间距，保证最后一根钢筋不会重叠
				DPoint3d tmpptvec = barpts[barpts.size() - 1] - ptendtmp;
				tmpptvec.Normalize();
				tmpptvec.Scale(barlines.at(i).spacing);
				ptendtmp.Add(tmpptvec);
				barpts[barpts.size() - 1] = ptendtmp;
			}
			//重建路径线
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
	//计算指定元素描述符中元素的范围。
	//判断vecFontLine是否是外侧线 如果不是则跟内侧线互换
	auto CheckAndSwapFrontLine = [&](const vector<MSElementDescrP>& faces) -> bool {
		// 如果两点距离过大，跳过检查
		if (abs(ptr1.x - ptr2.x) > 10 * uor_per_mm && abs(ptr1.y - ptr2.y) > 10 * uor_per_mm)
			return false;

		DRange3d range;
		const double offset = 50 * uor_per_mm; // 范围偏移量

		for (const auto& face : faces)
		{
			// 计算元素范围
			mdlElmdscr_computeRange(&range.low, &range.high, face, nullptr);

			// 缩小范围，留出 50mm 偏移
			range.low.x += offset;
			range.low.y += offset;
			range.high.x -= offset;
			range.high.y -= offset;

			// 判断 ptr1 是否在范围内
			if (range.IsContainedXY(ptr1))
			{
				// 交换内外侧线并反转方向
				m_walldata.vecFontLine[0] = m_walldata.vecBackLine[0];
				m_walldata.vecBackLine[0] = temp;
				m_walldata.vecToWall.Negate();
				return true; // 已成功处理
			}
		}
		return false;
	};
	do
	{
		// 先检查 downfloorfaces
		if (CheckAndSwapFrontLine(m_walldata.downfloorfaces))
			break;

		// 如果 downfloorfaces 未处理成功，再检查 upfloorfaces
		CheckAndSwapFrontLine(m_walldata.upfloorfaces);
	}
	while (false);

	//计算偏移方向
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

	//1、按正面、中间、背面筛选出对应的钢筋层号与相应的钢筋直径
	map<int, double> FDiameters;
	map<int, double> MDiameters;
	map<int, double> BDiameters;

	double allfdiam = 0;//统计外侧面钢筋直径
	double allmdiam = 0;//统计中间层钢筋直径
	double allbdiam = 0;//统计内侧面钢筋直径
	double alldiam = 0;//对侧钢筋直径和
	DPoint3d vecHoleehs;//如果出现孔洞则孔洞锚入的方向
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}
		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10
		if (GetvecDataExchange().at(i) == 0)//外侧面
		{
			FDiameters[GetvecRebarLevel().at(i)] = diameter;
			allfdiam = allfdiam + diameter;
		}
		else if (GetvecDataExchange().at(i) == 2)//内侧面
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

	//2、计算出每一层的偏移量，以正面线为基准
	//按正面，背面，中间三种来计算，
	//正面时：偏移量 = 正面保护层 + 当前层钢筋直径/2 + 前面其他层钢筋直径的和；
	//中间时：偏移量 = 墙厚/2 + 前面其他层钢筋直径的和；
	//背面时：偏移量 = 墙厚 - 反面保护层 - （当前层钢筋直径/2 + 前面其他层钢筋直径的和）；
	for (size_t i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}

		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10
		double movedis = dPositiveCover + diameter / 2;
		double fdiam = 0;//前面几层钢筋直径

		if (GetvecDataExchange().at(i) == 0)//前面层
		{
			int tindex = GetvecRebarLevel().at(i);
			for (int k = 1; k < tindex; k++)//计算之前的钢筋直径
			{
				movedis = FDiameters[k] + movedis;
				fdiam = fdiam + FDiameters[k];
			}
		}
		else if (GetvecDataExchange().at(i) == 2)//后面层
		{
			movedis = m_walldata.thickness - dReverseCover - diameter / 2;
			int tindex = GetvecRebarLevel().at(i);
			for (int k = 1; k < tindex; k++)//计算之前的钢筋直径
			{
				movedis = movedis - BDiameters[k];
				fdiam = fdiam + BDiameters[k];
			}
		}
		else
		{
			movedis = m_walldata.thickness / 2;
			int tindex = GetvecRebarLevel().at(i);
			for (int k = 1; k < tindex; k++)//计算之前的钢筋直径
			{
				movedis = MDiameters[k] + movedis;
				fdiam = fdiam + MDiameters[k];
			}
		}
		if (GetvecDataExchange().at(i) == 0)//前面层,对侧层为后面
		{
			alldiam = allbdiam;
			vecHoleehs = pt2[0] - pt1[0];

		}
		else if (GetvecDataExchange().at(i) == 2)//后面层，对侧层为前面
		{
			alldiam = allfdiam;
			vecHoleehs = pt1[0] - pt2[0];
		}
		else
		{
			alldiam = 0;
		}

		vecHoleehs.Normalize();

		//根据path线串、底面元素、偏移量计算偏移后的线串
		MSElementDescrP tmppath = GetLines(m_walldata.vecFontLine);
		GetMovePath(tmppath, movedis, m_walldata.downFace);
		//计算竖向的线
		DPoint3d startP, endP;
		mdlElmdscr_extractEndPoints(&startP, nullptr, &endP, nullptr, tmppath, ACTIVEMODEL);
		//PITCommonTool::CPointTool::DrowOneLine(DSegment3d::From(pt1, pt2), 4);//蓝
		endP = startP;
		endP.z = endP.z + m_walldata.height*uor_per_mm;
		EditElementHandle tlineeeh;
		LineHandler::CreateLineElement(tlineeeh, nullptr, DSegment3d::From(startP, endP), true, *ACTIVEMODEL);
		//PITCommonTool::CPointTool::DrowOneLine(DSegment3d::From(pt1, pt2), 3);//绿

		//PITCommonTool::CPointTool::DrowOneLine(m_walldata.vecFontLine[0],1);//红
		//PITCommonTool::CPointTool::DrowOneLine(m_walldata.vecBackLine[0], 2);//黄
		//3、根据周围顶板、底板和墙面情况计算当前实际配筋范围和钢筋线延升以及弯钩方向
		//钢筋方向不同处理方式不一样

		if (GetvecDir().at(i) == 0)//水平方向钢筋
		{
			MSElementDescrP path = tlineeeh.GetElementDescrP();
			MSElementDescrP barline = tmppath;
			CalculateLeftRightBarLines(barlines[i], fdiam, alldiam, vecHoleehs, path, barline, GetvecDataExchange().at(i), i);
			mdlElmdscr_freeAll(&tmppath);
		}
		else//垂直方向钢筋
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
* @desc:		根据保护层和移动距离重新计算基础钢筋线串
* @param[out]	data 配筋线数据
* @return	MSElementDescrP 新的钢筋线串
* @author	Hong ZhuoHui
* @Date:	2023/09/13
*/
void STWallRebarAssembly::ReCalBarLineByCoverAndDis(BarLinesdata & data)
{
	//计算首尾移动距离
	double strMoveLen = 0 - data.strDis;
	double endMoveLen = 0 - data.endDis;
	//计算首尾点
	DPoint3d strPt = { 0,0,0 }, endPt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&strPt, nullptr, &endPt, nullptr, data.barline, ACTIVEMODEL);
	//计算首尾移动向量
	DVec3d strVec = strPt - endPt;
	strVec.Normalize();
	strVec.ScaleToLength(strMoveLen);
	DVec3d endVec = endPt - strPt;
	endVec.Normalize();
	endVec.ScaleToLength(endMoveLen);
	//移动首尾点
	strPt.Add(strVec);
	endPt.Add(endVec);
	//重新画线
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPt, endPt), true, *ACTIVEMODEL);
	data.barline = eeh.GetElementDescrP();
	//平移路径线
	mdlCurrTrans_begin();
	Transform tMatrix;
	mdlTMatrix_getIdentity(&tMatrix);
	mdlTMatrix_setTranslation(&tMatrix, &strVec);
	mdlElmdscr_transform(&data.path, &tMatrix);
	mdlCurrTrans_end();
}

/*
* @desc:		根据实体求差算孔洞（Z型墙）
* @param[in]	wallEeh 墙
* @param[out]	holes 孔洞
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
			//取所有的面
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
		diameterTie = RebarCode::GetBarDiameter(strTieRebarSize, modelRef);	//拉筋直径
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

		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10
		double diameterTb = 0.0;
		if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
		{
			diameterTb = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(i).rebarSize, modelRef);		//乘以了10
		}

		if (diameter > BE_TOLERANCE)
		{
			CVector3D	zTrans(0.0, 0.0, 0.0);
			CVector3D	zTransTb;
			if (GetvecDir().at(i) == 0) //水平
			{
				zTrans.z = updownSideCover - diameter * 0.5;
				zTrans.x = m_STwallData.length * 0.5;
				zTransTb = zTrans;
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum())
				{
					if (GetvecTwinRebarLevel().at(i).hasTwinbars)//当前钢筋为并筋层
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
			// 				dOffset += diameter / 2.0;	//偏移首层钢筋半径
			// 				zTrans.y = dOffset;
			// 		    }
						//if (0 == i)
						//{
						//	dOffset += diameter / 2.0;	//偏移首层钢筋半径
						//	zTrans.y = dOffset;

						//	if (GetvecTwinRebarLevel().at(i).hasTwinbars)
						//	{
						//		if (diameterTb > diameter)//并筋层的钢筋比主筋直径大
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

				double diameterPre = RebarCode::GetBarDiameter(strSizePre, modelRef);		//乘以了10
				if (0 == i)
				{
					dOffset += diameter / 2.0;	//偏移首层钢筋半径
					dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm;
				}
				else
				{
					dLevelSpace = GetvecLevelSpace().at(i) * uor_per_mm + diameter * 0.5 + diameterPre * 0.5;//层间距加上当前钢筋直径
				}

				dOffset += dLevelSpace;
				dOffsetTb = dOffset;
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
				{
					if (diameterTb > diameter)//并筋层的钢筋比主筋直径大
						dOffsetTb += (diameterTb / 2.0 - diameter / 2.0);
					else
						dOffsetTb -= (diameter / 2.0 - diameterTb / 2.0);
				}
				if (COMPARE_VALUES(m_STwallData.width - dOffset, dReverseCover + diameterTie) < 0)		//当前钢筋层已嵌入到了反面保护层中时，实际布置的钢筋层间距就不再使用设置的与上层间距，而是使用保护层进行限制
				{
					zTrans.y = m_STwallData.width - dReverseCover - diameter / 2.0 - diameterTie;
					zTransTb.y = zTrans.y;
					if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)
					{
						if (diameterTb > diameter)//并筋层的钢筋比主筋直径大
							zTransTb.y += (diameterTb / 2.0 - diameter / 2.0);
						else
							zTransTb.y -= (diameter / 2.0 - diameterTb / 2.0);
					}
					//判断：如果上一层的zTrans.y与当前层的zTrans.y相同，则上一层减去当前层的钢筋直径。（防止钢筋碰撞）
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
							diameterPre = RebarCode::GetBarDiameter(strSize1, modelRef);		//乘以了10
							if (COMPARE_VALUES(vTransform[j].y + diameterPre * 0.5, compare - diameter * 0.5) > 0)	//嵌入了下一根钢筋终
							{
								if (j == vTransform.size() - 1)//为当前超出的第一元素
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
									double diameterTbPre = RebarCode::GetBarDiameter(GetvecTwinRebarLevel().at(j).rebarSize, modelRef);		//乘以了10

									if (diameterTbPre > diameterPre)//并筋层的钢筋比主筋直径大
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
					// 					if (GetvecTwinRebarLevel().at(i).hasTwinbars && diameterTb > diameter)	//并筋层的钢筋比主筋直径大
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
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改
	if (pt1[0].Distance(ptstar) > pt1[0].Distance(ptend))//确保pt1[0]为起始点
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
		DPoint3d ptProject1;	//投影点
		mdlVec_projectPointToLine(&ptProject1, NULL, &pt1[0], &m_STwallData.ptStart, &m_STwallData.ptEnd);

		DPoint3d ptProject2;	//投影点
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

	if (g_wallRebarInfo.concrete.isHandleHole)//计算需要处理的孔洞
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
				if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//如果是门洞
				{
					continue;
				}
				bool isdoorNeg = false;//判断是否为门洞NEG
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
	//if (COMPARE_VALUES(dSideCover, m_STwallData.length) >= 0)	//如果侧面保护层大于等于墙的长度
	//{
	//	mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"侧面保护层大于等于墙的长度,无法创建钢筋层", MessageBoxIconType::Information);
	//	return false;
	//}

	if (m_walldata.downFace == nullptr) return false;
	map<int, vector<BarLinesdata>> barlines;
	CalculateBarLinesData(barlines, ACTIVEMODEL);
	map<int, bool> map_isInside;
	// 找出同一面中，为内侧的钢筋组
	for (const auto& it : barlines) 
	{
		// 如果有一个isInSide为true，则记录下来
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
		// 同一面中，如果有一组成功判定为内侧，则均同步为内侧，并不再采用后续的判定
		if (map_isInside.find(GetvecDataExchange().at(it.first)) != map_isInside.end()) {
			for (auto& bar : it.second) {
				bar.isInSide = true;
			}
		}
		// 没有确定好内外侧时进行这部分判定
		if (!it.second[0].isInSide)
		{
			if (GetvecDataExchange().at(it.first) == 0)//外侧面
			{
				it.second[0].isInSide = false;
			}
			else if (GetvecDataExchange().at(it.first) == 2)//内侧面
			{
				it.second[0].isInSide = true;
			}
			//判断是否墙全部被板包围
			DPoint3d ptr1, ptr2;
			m_walldata.vecFontLine[0].GetStartPoint(ptr1);
			m_walldata.vecFontLine[0].GetEndPoint(ptr2);
			ptr1.Add(ptr2);
			ptr1.Scale(0.5);//中点判断
			DRange3d range;
			//计算指定元素描述符中元素的范围。
			for (auto data : m_walldata.downfloorfaces)//判断vecFontLine是否是外侧 如果不是则跟内侧
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
	bool result = EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);//获取开孔之前的墙体
	if (!result)
		return false;

	//1、将墙体从参考模型拷贝到当前模型
	EditElementHandle copyEleeh;
	copyEleeh.Duplicate(Eleeh);
	ElementCopyContext copier2(ACTIVEMODEL);
	copier2.SetSourceModelRef(Eleeh.GetModelRef());
	copier2.SetTransformToDestination(true);
	copier2.SetWriteElements(false);
	copier2.DoCopy(copyEleeh);
	m_walldata.ClearData();
	//2、获取合并后的墙底面包含顶面投影和所有底面的合并
	m_walldata.downFace = ExtractFacesTool::GetCombineDownFace(copyEleeh);

	if (m_walldata.downFace != nullptr)
	{
		//mdlElmdscr_add(m_walldata.downFace);
	}
	//3、获取墙底面的前后线串和厚度
	m_walldata.vecFontLine.clear();
	m_walldata.vecBackLine.clear();
	ExtractFacesTool::GetFrontBackLinePoint(m_walldata.downFace, m_walldata.vecFontLine, m_walldata.vecBackLine, m_walldata.thickness);

	DPoint3d minP2, maxP2;
	//计算指定元素描述符中元素的范围。
	mdlElmdscr_computeRange(&minP2, &maxP2, copyEleeh.GetElementDescrP(), NULL);
	if (!Eleeh.IsValid())
	{
		//mdlDialog_dmsgsPrint(L"非法的墙实体!");
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
	mdlRMatrix_fromVectorToVector(&rMatrix, &facenormal, &vecZ);//旋转到xoy平面
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
	//计算指定元素描述符中元素的范围。
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
	if (Negs.size() > 0)//STWALL有斜边
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
	//	yVecNegate.ScaleToLength(m_STwallData.width);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	//	ptStart.Add(yVecNegate);
	//	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(ptStart, ptEnd);
	xVecNew.Normalize();
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
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
* @desc:		根据周围元素和钢筋线的位置计算钢筋是否合法
* @param[in]	nowVec  判断是竖直方向还是水平方向
* @param[in]	alleehs 周围元素
* @param[in]	tmpendEndTypes 端部样式
* @param[in]	lineEeh 钢筋线
* @param[in]    Eleeh  开孔之前的实体（墙、板）
* @param[in]	anchored_vec  锚入角度，方向
* @param[in]	matrix  投影矩阵
* @param[in]	moveDis 保护层距离
* @param[in]	lenth   锚入长度
* @param[in]	ori_Point   原端点位置
* @param[in]	mov_Point   修改端点位置
* @param[in]	type_flag   是否不需要端部样式，如果FLAGE=1表示不需要端部样式，如果FLAGE=2表示尾端点位置修改为mov_Point位置
* @param[in/out]	data 配筋线数据
* @author	ChenDong
* @Date:	2024/10/17
*/

void STWallRebarAssembly::JudgeBarLinesLegitimate(const CVector3D  nowVec, vector<EditElementHandle*>alleehs, PIT::PITRebarEndTypes& tmpendEndTypes,
	EditElementHandle &lineEeh, EditElementHandle *Eleeh, CVector3D anchored_vec
	, const Transform matrix, double moveDis, double lenth, DPoint3d &ori_Point, DPoint3d &mov_Point, int &type_Flag)
{
	bool is_str = false;//是否为起始点
	DPoint3d using_pt;
	DPoint3d mid_end_pt;//钢筋端部中点判断
	vector<DPoint3d> tmppts;//实体交点
	DPoint3d str_Pt = { 0,0,0 }, end_Pt = { 0,0,0 };
	mdlElmdscr_extractEndPoints(&str_Pt, nullptr, &end_Pt, nullptr, lineEeh.GetElementDescrP(), ACTIVEMODEL);
	double dSideCover = 0;

	using_pt = ori_Point;
	movePoint(anchored_vec, using_pt, lenth);

	//钢筋线端点和钢筋端点位置是否锚出
	if (!ISPointInHoles(alleehs, ori_Point) || !ISPointInHoles(alleehs, using_pt))
	{
		if (type_Flag == 0)//说明是起始点
			is_str = true;
		int endtype_flag = 0;//endtype_flag == 1 端部弯弧为直角 不为1为其他弯锚
		//端部样式是直角锚还是倾斜锚入
		if (anchored_vec.x == floor(anchored_vec.x) && anchored_vec.y == floor(anchored_vec.y) && anchored_vec.z == floor(anchored_vec.z))
		{
			anchored_vec.Negate();
			endtype_flag = 1;//端部弯弧为直角
		}
		else
		{
			anchored_vec.x = anchored_vec.x;
			anchored_vec.y = -anchored_vec.y;
			anchored_vec.z = anchored_vec.z;
		}
		using_pt = ori_Point;
		mid_end_pt = ori_Point;
		//方向取反再求一次位置
		STWallRebarAssembly::movePoint(anchored_vec, using_pt, lenth);
		STWallRebarAssembly::movePoint(anchored_vec, mid_end_pt, lenth / 2);
		//如果钢筋线末端点没有锚入任何实体    
		if (!ISPointInHoles(alleehs, ori_Point))
		{
			type_Flag = 2;
			//PITCommonTool::CPointTool::DrowOnePoint(ori_Point, 1, 3);//绿
			mdlElmdscr_extractEndPoints(&str_Pt, nullptr, &end_Pt, nullptr, lineEeh.GetElementDescrP(), ACTIVEMODEL);
			//执行钢筋与原实体作交
			GetIntersectPointsWithOldElm(tmppts, Eleeh, str_Pt, end_Pt, dSideCover, matrix);
			//与原实体有交点
			if (tmppts.size() > 1)
			{
				//回缩一个保护层的距离
				CVector3D vectortepm = end_Pt - using_pt;
				vectortepm.Normalize();
				vectortepm.ScaleToLength(moveDis);
				using_pt.Add(vectortepm);
				lenth = moveDis + tmpendEndTypes.end.GetbendRadius();

				if (is_str)//说明是起始点
					mov_Point = tmppts[tmppts.size() - 1];
				else
					mov_Point = tmppts[tmppts.size() - 2];

				if (COMPARE_VALUES_EPS(abs(nowVec.z), 1, EPS) == 0)//竖直方向上
				{
					if (is_str)
						mov_Point.z = mov_Point.z + lenth;
					else
						mov_Point.z = mov_Point.z - lenth;
				}

				else if (COMPARE_VALUES_EPS(abs(nowVec.x), 1, 0.1) == 0 || COMPARE_VALUES_EPS(abs(nowVec.y), 1, 0.1) == 0)//水平方向上
				{
					if (is_str)
						mov_Point.x = mov_Point.x + lenth;
					else
						mov_Point.x = mov_Point.x - lenth;
				}
				if (!ISPointInHoles(alleehs, mov_Point))//判断是否在实体内，不在则反向
				{
					anchored_vec.Negate();
				}
			}
		}
		//钢筋末端点仍然没有锚入任何实体，在实体内截断
		else if (!ISPointInHoles(alleehs, using_pt) || !ISPointInHoles(alleehs, mid_end_pt))
		{
			if (endtype_flag == 1)//端部弯弧为直角
			{
				type_Flag = 2;
				using_pt = ori_Point;
				DPoint3d end_pt = ori_Point;
				anchored_vec.Negate();//方向还原

				mov_Point = ori_Point;
				auto temp = ori_Point;
				movePoint(anchored_vec, using_pt, lenth);
				GetIntersectPointsWithHoles(tmppts, alleehs, using_pt, temp, dSideCover, matrix);
				GetIntersectPointsWithHolesByInsert(tmppts, alleehs, using_pt, ori_Point, dSideCover, matrix);

				//如果与原模型有交点
				if (tmppts.size() > 0)
				{
					CVector3D vectortepm = end_pt - using_pt;
					vectortepm.Normalize();
					vectortepm.ScaleToLength(moveDis);
					using_pt.Add(vectortepm);
					//获取延伸长度
					auto temp_pt = ori_Point;
					temp_pt.z = 0;
					tmppts[0].z = 0;
					//钢筋线点到交点的距离包含一个保护层距离所以减二倍
					lenth = mdlVec_distance(&tmppts[0], &temp_pt) - 2 * tmpendEndTypes.end.GetbendRadius();
					//lenth = moveDis + tmpendEndTypes.end.GetbendRadius();
					using_pt = ori_Point;
					movePoint(anchored_vec, using_pt, lenth);

					//PITCommonTool::CPointTool::DrowOnePoint(using_pt, 1, 1);//绿
					if (!ISPointInHoles(alleehs, using_pt))
					{
						anchored_vec.Negate();//方向还原
						//lenth -= tmpendEndTypes.end.GetbendRadius();
					}

					if (is_str)//说明是起始点
						tmpendEndTypes.beg.SetbendLen(lenth);
					else
						tmpendEndTypes.end.SetbendLen(lenth);
				}
			}
			else//端部弯弧不为直角有可能会延伸出模型
			{
				type_Flag = 3;
			}
		}

		if (is_str)//说明是起始点
			tmpendEndTypes.beg.SetendNormal(anchored_vec);
		else//说明是末端点
			tmpendEndTypes.end.SetendNormal(anchored_vec);

	}
}