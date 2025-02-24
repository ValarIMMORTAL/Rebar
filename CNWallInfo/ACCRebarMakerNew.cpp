#include "_ustation.h"
#include <SelectionRebar.h>
#include "ACCRebarMakerNew.h"
#include "PITACCRebarAssemblyNew.h"
#include "ElementAttribute.h"
#include "ScanIntersectTool.h"
#include "ExtractFacesTool.h"
#include "../XmlHelper.h"
#include "../XmlManager.h"
extern bool PreviewButtonDown; // 预览标志


ACCRebarMakerNew::ACCRebarMakerNew(ElementHandle currEh, const IntersectEle & ACCEh, ACCRebarAssemblyNew * rebarAssembly) 
	: m_eh(currEh), /*m_pACC(new CACCNew(currEh)),*/ m_ACCEh(ACCEh),m_RebarAssembly(rebarAssembly)
{
	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL: 
	{
		AnalyzingSTWallGeometricData(m_eh,m_ACCEh.Eh,m_CurSTwallData);
		break;
	}
	default:
		break;
	}
	
	if (m_ACCEh.EleType == "STWALL")
	{
		AnalyzingSTWallGeometricData(m_ACCEh.Eh,currEh, m_InSecSTwallData);
	}
	else if (m_ACCEh.EleType == "FLOOR")
	{
		AnalyzingSlabGeometricData(m_ACCEh.Eh, currEh,m_slabHeight);
	}
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	PlusHoles(rebarAssembly->GetSideCover()*uor_per_mm);

	GetCoverSideinFrontOrBack();

}

void ACCRebarMakerNew::AddRebarLevelAndType(RebarElementP rebarElement, BrString sizeKey, DPoint3d ptRebar, DPoint3d ptStr, DPoint3d ptEnd, double dDistance, bool bFlag)
{
	ElementId eleid = rebarElement->GetElementId();
	EditElementHandle tmprebar(eleid, ACTIVEMODEL);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	string Stype;

	DPoint3d ptProjection;
	mdlVec_projectPointToLine(&ptProjection, NULL, &ptRebar, &ptStr, &ptEnd);

	// 与中间线的距离
	double dLengthMid = ptProjection.Distance(ptRebar);
	
	if (bFlag)
	{
		mdlVec_projectPointToLine(&ptProjection, NULL, &ptRebar, &m_InSecSTwallData.ptPreStr, &m_InSecSTwallData.ptPreEnd);
	}
	else
	{
		mdlVec_projectPointToLine(&ptProjection, NULL, &ptRebar, &m_CurSTwallData.ptPreStr, &m_CurSTwallData.ptPreEnd);
	}

	// 与前层的距离
	double dLengthPre = ptProjection.Distance(ptRebar);

	if (bFlag)
	{
		mdlVec_projectPointToLine(&ptProjection, NULL, &ptRebar, &m_InSecSTwallData.ptBckStr, &m_InSecSTwallData.ptBckEnd);
	}
	else
	{
		mdlVec_projectPointToLine(&ptProjection, NULL, &ptRebar, &m_CurSTwallData.ptBckStr, &m_CurSTwallData.ptBckEnd);
	}

	// 与后层的距离
	double dLengthBck = ptProjection.Distance(ptRebar);

	int level = 0;
	// 前面的点筋  -- 前层距离小于1.5倍保护层距离属于前层
	if (COMPARE_VALUES_EPS(dLengthPre, dDistance * 1.5, EPS) < 0 && COMPARE_VALUES_EPS(dLengthPre, dDistance * 1.5, EPS) < 0)
	{
		Stype = "front";
		int maxLevel = 1;

		// 取前层最大层级
		for (ConcreteRebar st : m_vecRebarData)
		{
			if (st.datachange == 0 &&  maxLevel < st.rebarLevel)
			{
				maxLevel = st.rebarLevel;
			}
		}
		level = maxLevel;
	}
	// 后层距离小于1.5倍保护层距离属于后层
	else if (COMPARE_VALUES_EPS(dLengthBck, dDistance * 1.5, EPS) < 0) // 后面的点筋
	{
		Stype = "back";
		int maxLevel = 1;

		// 取后层最大层级
		for (ConcreteRebar st : m_vecRebarData)
		{
			if (st.datachange == 2 && maxLevel < st.rebarLevel)
			{
				maxLevel = st.rebarLevel;
			}
		}
		level = maxLevel;
	}
	else
	{
		Stype = "mid";
		int maxLevel = 1;

		// 取中间层最大层级
		for (ConcreteRebar st : m_vecRebarData)
		{
			if (st.datachange == 1 && maxLevel < st.rebarLevel)
			{
				maxLevel = st.rebarLevel;
			}
		}
		level = maxLevel;
	}

	int grade = 0;
	sizeKey = sizeKey.Right(1);
	if (sizeKey == "A")
	{
		grade = 0;
	}
	else if (sizeKey == "B")
	{
		grade = 1;
	}
	else if (sizeKey == "C")
	{
		grade = 2;
	}
	else if (sizeKey == "D")
	{
		grade = 3;
	}

	char tlevel[256];
	sprintf(tlevel, "%d", level);
	string slevel(tlevel);
	ElementRefP oldref = tmprebar.GetElementRef();
	SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
	tmprebar.ReplaceInModel(oldref);
}

bool ACCRebarMakerNew::ReCalculateWidthAndPoint()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	DPoint3d midPos = m_CurSTwallData.ptStart;
	midPos.Add(m_CurSTwallData.ptEnd);
	midPos.Scale(0.5);
	DPoint3d midPosProject;
	mdlVec_projectPointToLine(&midPosProject, NULL, &midPos, &m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptEnd);
	DPoint3d vecFront = midPos - midPosProject;
	vecFront.Normalize();
	//取当前构件的混凝土保护层
	WallRebarInfo curConcrete;
	if (GetCurElementHandle().IsValid())
	{
		ElementId concreteid = 0;
		GetElementXAttribute(m_eh.GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_eh.GetModelRef());
		if (SUCCESS != GetElementXAttribute(concreteid, sizeof(WallRebarInfo), curConcrete, WallRebarInfoXAttribute, ACTIVEMODEL))
		{
			curConcrete.concrete.sideCover = 50;
			curConcrete.concrete.postiveCover = 50;
			curConcrete.concrete.reverseCover = 50;
		}
	}
	//关联构件的混凝土保护层
	ACCConcrete ACConcrete;
	int retGetACC = GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ACCConcrete), ACConcrete, ConcreteCoverXAttribute, m_ACCEh.Eh.GetModelRef());
	if (retGetACC != SUCCESS)
	{
		ACConcrete = { 50,50,50 };
	}
	double disPosCover = 0;
	double disBkCover = 0;
	disPosCover = ACConcrete.postiveOrTopCover - curConcrete.concrete.sideCover;//前面保护层的差值
	disBkCover = ACConcrete.reverseOrBottomCover - curConcrete.concrete.sideCover;//后面保护层的差值
	if (disPosCover != 0 || disBkCover != 0)
	{
		double misDis = (disPosCover - disBkCover)*uor_per_mm / 2;
		if (!m_isFrCoverside)
		{
			misDis = misDis * -1;
		}
		vecFront.Scale(misDis);
		mdlVec_addPoint(&m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptStart, &vecFront);
		mdlVec_addPoint(&m_InSecSTwallData.ptEnd, &m_InSecSTwallData.ptEnd, &vecFront);
		m_InSecSTwallData.width = m_InSecSTwallData.width - disPosCover * uor_per_mm - disBkCover * uor_per_mm;
	}

	DPoint3d midPos2 = m_InSecSTwallData.ptStart;
	midPos2.Add(m_InSecSTwallData.ptEnd);
	midPos2.Scale(0.5);
	DPoint3d midPosProject2;
	mdlVec_projectPointToLine(&midPosProject2, NULL, &midPos2, &m_CurSTwallData.ptStart, &m_CurSTwallData.ptEnd);
	DPoint3d vecFront2 = midPos2 - midPosProject2;
	vecFront2.Normalize();
	double disPosCover2 = 0;
	double disBkCover2 = 0;
	disPosCover2 = curConcrete.concrete.postiveCover - curConcrete.concrete.sideCover;//前面保护层的差值
	disBkCover2 = curConcrete.concrete.reverseCover - curConcrete.concrete.sideCover;//后面保护层的差值
	if (disPosCover2 != 0 || disBkCover2 != 0)
	{
		double misDis = (disPosCover2 - disBkCover2)*uor_per_mm / 2;
		vecFront2.Scale(misDis);
		mdlVec_addPoint(&m_CurSTwallData.ptStart, &m_CurSTwallData.ptStart, &vecFront2);
		mdlVec_addPoint(&m_CurSTwallData.ptEnd, &m_CurSTwallData.ptEnd, &vecFront2);
		m_CurSTwallData.width = m_CurSTwallData.width - disPosCover2 * uor_per_mm - disBkCover2 * uor_per_mm;
	}
	return true;
}

bool ACCRebarMakerNew::ReCalculateWidthAndPointForL4()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	DPoint3d midPos = m_CurSTwallData.ptStart;
	midPos.Add(m_CurSTwallData.ptEnd);
	midPos.Scale(0.5);
	DPoint3d midPosProject;
	mdlVec_projectPointToLine(&midPosProject, NULL, &midPos, &m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptEnd);
	DPoint3d vecFront = midPos - midPosProject;
	vecFront.Normalize();
	//取当前构件的混凝土保护层
	WallRebarInfo curConcrete;
	if (GetCurElementHandle().IsValid())
	{
		ElementId concreteid = 0;
		GetElementXAttribute(m_eh.GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_eh.GetModelRef());
		if (SUCCESS != GetElementXAttribute(concreteid, sizeof(WallRebarInfo), curConcrete, WallRebarInfoXAttribute, ACTIVEMODEL))
		{
			curConcrete.concrete.sideCover = 50;
			curConcrete.concrete.postiveCover = 50;
			curConcrete.concrete.reverseCover = 50;
		}
	}
	//关联构件的混凝土保护层
	ACCConcrete ACConcrete;
	int retGetACC = GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ACCConcrete), ACConcrete, ConcreteCoverXAttribute, m_ACCEh.Eh.GetModelRef());
	if (retGetACC != SUCCESS)
	{
		ACConcrete = { 50,50,50 };
	}
	double disPosCover = 0;
	double disBkCover = 0;
	disPosCover = ACConcrete.postiveOrTopCover - curConcrete.concrete.sideCover;//前面保护层的差值
	disBkCover = ACConcrete.reverseOrBottomCover - curConcrete.concrete.sideCover;//后面保护层的差值
	if (disPosCover != 0 || disBkCover != 0)
	{
		double misDis = (disPosCover - disBkCover)*uor_per_mm / 2;
		if (!m_isFrCoverside)
		{
			misDis = misDis * -1;
		}
		vecFront.Scale(misDis);
		mdlVec_addPoint(&m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptStart, &vecFront);
		mdlVec_addPoint(&m_InSecSTwallData.ptEnd, &m_InSecSTwallData.ptEnd, &vecFront);
		m_InSecSTwallData.width = m_InSecSTwallData.width - disPosCover * uor_per_mm - disBkCover * uor_per_mm;
	}

	DPoint3d backStart;
	vector<DSegment3d> vecDownFontLine;
	vector<DSegment3d> vecDownBackLine;

	EditElementHandle testeeh(m_eh, false);
	//testeeh.AddToModel();

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	double tHeight;
	EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &tHeight);

	if (vecDownFontLine.empty() || vecDownBackLine.empty())
	{
		return false;
	}

	vecDownBackLine[0].GetStartPoint(backStart);
	



	DPoint3d midPos2 = backStart;
	/*midPos2.Add(m_InSecSTwallData.ptEnd);
	midPos2.Scale(0.5);*/
	DPoint3d midPosProject2;
	mdlVec_projectPointToLine(&midPosProject2, NULL, &midPos2, &m_CurSTwallData.ptStart, &m_CurSTwallData.ptEnd);
	DPoint3d vecFront2 = midPos2 - midPosProject2;
	vecFront2.Normalize();
	double disPosCover2 = 0;
	double disBkCover2 = 0;
	disPosCover2 = curConcrete.concrete.postiveCover - curConcrete.concrete.sideCover;//前面保护层的差值
	disBkCover2 = curConcrete.concrete.reverseCover - curConcrete.concrete.sideCover;//后面保护层的差值
	if (disPosCover2 != 0 || disBkCover2 != 0)
	{
		double misDis = (disPosCover2 - disBkCover2)*uor_per_mm / 2;
		vecFront2.Scale(misDis);
		mdlVec_addPoint(&m_CurSTwallData.ptStart, &m_CurSTwallData.ptStart, &vecFront2);
		mdlVec_addPoint(&m_CurSTwallData.ptEnd, &m_CurSTwallData.ptEnd, &vecFront2);
		m_CurSTwallData.width = m_CurSTwallData.width - disPosCover2 * uor_per_mm - disBkCover2 * uor_per_mm;
	}
	return true;
}


bool CutWallByACCData2(vector<EditElementHandle*> &vecCutWall,vector<DPoint3d>& vecCutPoints, DSegment3dR segLine,ElementHandleCR currEh,double curWallHeight, const vector<IntersectEle> &vecInEle, const vector<PIT::AssociatedComponent>& vecAC, DgnModelRefP modelRef)
{
	//0.先判断关联构件中有没有关联需切断待配筋墙的构件，若没有则无需截断。
	auto it = std::find_if(vecAC.begin(), vecAC.end(), [](const AssociatedComponent& tmphd) 
	{
		
		return (tmphd.isCut != 0&&tmphd.anchoringMethod==3); 
	});
	if (it == vecAC.end())
	{
		return false;
	}
	
	//1.先判断关联墙中有没有被锚入关系的墙,并全部存储。
	std::vector<IntersectEle> vecComp;
	for (size_t i = 0; i < vecInEle.size(); ++i)
	{
		std::string strACCName = vecInEle[i].EleName;
		auto it = std::find_if(vecAC.begin(), vecAC.end(), [&strACCName](const AssociatedComponent& tmphd) {return (std::string(tmphd.associatedComponentName) == strACCName && tmphd.isCut != 0); });
		if (it != vecAC.end())
		{
			vecComp.push_back(vecInEle[i]);
		}
	}

	//2.根据名称分组，
	std::vector<std::vector<IntersectEle> > vvInsecEle;
	for (size_t i = 0; i < vecComp.size(); ++i)
	{
		std::string strName = vecComp[i].EleName;
		strName = strName.substr(0, strName.length() - 3);
		std::vector<IntersectEle> vInsecEle = { vecComp[i] };
		for (size_t j = i + 1; j < vecComp.size() - 1; ++j)
		{
			std::string strNameNext = vecComp[j].EleName;
			strNameNext = strNameNext.substr(0, strNameNext.length() - 3);
			if (strName == strNameNext)
			{
				vInsecEle.push_back(vecComp[j]);
				std::vector<IntersectEle>::iterator it = vecComp.begin();
				std::advance(it, j);
				vecComp.erase(it);
			}
		}
		vvInsecEle.push_back(vInsecEle);
	}

	//3.将同一名称的墙底面进行合并
	vector<MSElementDescrP> vecAllDownFace;
	for (size_t i = 0; i < vvInsecEle.size(); ++i)
	{
		vector<MSElementDescrP> vecDownface;
		for (size_t j = 0; j < vvInsecEle[i].size(); ++j)
		{
			vector<MSElementDescrP> vec_line1;
			EditElementHandle downFace;
			vector<MSElementDescrP> vecDownFontLine1;
			vector<MSElementDescrP> vecDownBackLine1;
			EditElementHandle Eleeh1;
			std::vector<EditElementHandle*> Holeehs1;
			EFT::GetSolidElementAndSolidHoles(vvInsecEle[i][0].Eh, Eleeh1, Holeehs1);
			std::for_each(Holeehs1.begin(), Holeehs1.end(), [](EditElementHandle* &eh) {delete eh; eh = NULL; });
			ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh1, &downFace, vec_line1, vecDownFontLine1, vecDownBackLine1, NULL);
			std::for_each(vec_line1.begin(), vec_line1.end(), [](MSElementDescrP &ms) {mdlElmdscr_freeAll(&ms); ms = NULL; });
			vecDownface.push_back(downFace.ExtractElementDescr());
		}
		if (vecDownface.size() > 0)
		{
			MSElementDescrP msDownFace = vecDownface[0];
			if (vecDownface.size() == 2)
			{
				if (!CombinWallDescrPs(msDownFace, vecDownface))//合并失败时
				{
					for (MSElementDescrP tmpdescr : vecDownface)
					{
						if (tmpdescr != NULL)
						{
							mdlElmdscr_freeAll(&tmpdescr);
						}
					}
					continue;
				}
			}
			vecAllDownFace.push_back(msDownFace);
		}
	}

	//4.将底面转到水平，计算出最大范围并生成底面，再转回去拉升成体
	//
	vector<EditElementHandle*> vecAllSoild;
	vector<DSegment3d> vecDownFontLine;
	vector<DSegment3d> vecDownBackLine;
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(currEh, Eleeh, Holeehs);
	std::for_each(Holeehs.begin(), Holeehs.end(), [](EditElementHandle* &eh) {delete eh; eh = NULL; });
	ExtractFacesTool::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, NULL);
	if (vecDownFontLine.size() > 0)
	{
		segLine = DSegment3d::From(vecDownFontLine[0].point[0], vecDownFontLine[0].point[1]);
		CMatrix3D matrix = CMatrix3D::Ucs(DPoint3d::From(0,0,0), CVector3D::kXaxis, CVector3D::kYaxis);
		Transform trans;
		matrix.AssignTo(trans);
		for (size_t i = 0; i < vecAllDownFace.size(); ++i)
		{
			EditElementHandle eeh(vecAllDownFace[i], true, modelRef);
			TransformInfo tInfo(trans);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, tInfo);
			DPoint3d ptMin, ptMax;
			mdlElmdscr_computeRange(&ptMin, &ptMax, eeh.GetElementDescrP(), NULL);

			//以两个点绘制矩形底面
			DPoint3d ptShape[4];
			ptShape[0] = ptMin;
			ptShape[1] = ptMax;
			ptShape[1].y = ptMin.y;
			ptShape[2] = ptMax;
			ptShape[3] = ptMin;
			ptShape[3].y = ptMax.y;
			ptShape[0].z = ptShape[1].z = ptShape[2].z = ptShape[3].z = vecDownFontLine[0].point[0].z;
			EditElementHandle eehMaxShape;
			ShapeHandler::CreateShapeElement(eehMaxShape, NULL, ptShape, 4, true, *ACTIVEMODEL);
			//eehMaxShape.AddToModel();
			Transform inverseTrans;
			inverseTrans.InverseOf(trans);
			TransformInfo inverseInfo(inverseTrans);
			eehMaxShape.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehMaxShape, inverseInfo);
			EditElementHandle *eehSolid = new EditElementHandle;
			ISolidKernelEntityPtr ptarget;
			SolidUtil::Convert::ElementToBody(ptarget, eehMaxShape);
			SolidUtil::Modify::ThickenSheet(ptarget, curWallHeight,0.0);
			if (SUCCESS != SolidUtil::Convert::BodyToElement(*eehSolid, *ptarget, NULL, *ACTIVEMODEL))
			{
				return false;
			}
			vecAllSoild.push_back(eehSolid);
			//eehSolid->AddToModel();
			DPoint3d tmpMid = ptShape[0];
			tmpMid.Add(ptShape[2]);
			tmpMid.Scale(0.5);
			tmpMid.z = tmpMid.z + 10;
			vecCutPoints.push_back(tmpMid);
		}

		//5.将当前墙的前后线依次与关联墙进行交集运算
		vector<DPoint3d> vecIntersectFrontPt;
		vector<DPoint3d> vecIntersectBackPt;
		DPoint3d FrStr, FrEnd;
		FrStr = vecDownFontLine[0].point[0];
		FrEnd = vecDownFontLine[0].point[1];
		DPoint3d BkStr, BkEnd;
		BkStr = vecDownBackLine[0].point[0];
		BkEnd = vecDownBackLine[0].point[1];
		DPoint3d vecProject;
		mdlVec_projectPointToLine(&vecProject, NULL, &FrStr, &BkStr, &BkEnd);
		double width = vecProject.Distance(FrStr);
		vecProject = FrStr - vecProject;
		vecProject.Normalize();
		vecProject.Scale(10);
		mdlVec_addPoint(&FrStr, &FrStr, &vecProject);
		mdlVec_addPoint(&FrEnd, &FrEnd, &vecProject);
		vecProject.Scale(-1);
		mdlVec_addPoint(&BkStr, &BkStr, &vecProject);
		mdlVec_addPoint(&BkEnd, &BkEnd, &vecProject);
		GetIntersectPointsWithHoles(vecIntersectFrontPt, vecAllSoild, FrStr, FrEnd);
		GetIntersectPointsWithHoles(vecIntersectBackPt, vecAllSoild, BkStr, BkEnd);
		std::for_each(vecAllSoild.begin(), vecAllSoild.end(), [](EditElementHandle* &eh) {delete eh; eh = NULL; });

		//6.合并交集运算了之后的点，并将点全部投影到一条边上
		vecIntersectFrontPt.insert(vecIntersectFrontPt.end(), vecIntersectBackPt.begin(), vecIntersectBackPt.end());
		map<int, DSegment3d> mapSeg;	//截断的线
		for (size_t i = 0; i < vecIntersectFrontPt.size(); i++)
		{
			DPoint3d ptProjectFront;
			mdlVec_projectPointToLine(&ptProjectFront, NULL, &vecIntersectFrontPt[i], &vecDownFontLine[0].point[0], &vecDownFontLine[0].point[1]);
			DPoint3d ptProjectBack;
			mdlVec_projectPointToLine(&ptProjectBack, NULL, &vecIntersectFrontPt[i], &vecDownBackLine[0].point[0], &vecDownBackLine[0].point[1]);
			int dis = (int)ptProjectFront.Distance(vecDownFontLine[0].point[0]);
			mapSeg.insert(std::make_pair(dis, DSegment3d::From(ptProjectFront, ptProjectBack)));
		}
		for (int i = 0;i<vecCutPoints.size();i++)
		{
			DPoint3d tmpPt = vecCutPoints[i];
			DPoint3d ptProjectFront;
			mdlVec_projectPointToLine(&ptProjectFront, NULL, &tmpPt, &vecDownFontLine[0].point[0], &vecDownFontLine[0].point[1]);
			DPoint3d ptProjectBack;
			mdlVec_projectPointToLine(&ptProjectBack, NULL, &tmpPt, &vecDownBackLine[0].point[0], &vecDownBackLine[0].point[1]);

			tmpPt = ptProjectFront;
			tmpPt.Add(ptProjectBack);
			tmpPt.Scale(0.5);
			tmpPt.z = tmpPt.z + curWallHeight / 2;
			vecCutPoints[i] = tmpPt;
		}


		// 			EditElementHandle eehLine1, eehLine2;
		// 			LineHandler::CreateLineElement(eehLine1, NULL, DSegment3d::From(vecDownFontLine[0].point[0], vecDownFontLine[0].point[1]), true, *ACTIVEMODEL);
		// 			LineHandler::CreateLineElement(eehLine2, NULL, DSegment3d::From(vecDownBackLine[0].point[0], vecDownBackLine[0].point[1]), true, *ACTIVEMODEL);
		// 			eehLine1.AddToModel();
		// 			eehLine2.AddToModel();
		DPoint3d ptLeftDown = vecDownFontLine[0].point[0];
		DPoint3d ptLeftTop = vecDownBackLine[0].point[0];
		for (auto it = mapSeg.begin(); it != mapSeg.end(); ++it)
		{
			DPoint3d ptRightTop = it->second.point[1];
			DPoint3d ptRightDown = it->second.point[0];
			DPoint3d ptCut[4] = { ptLeftDown,ptRightDown,ptRightTop,ptLeftTop };
			EditElementHandle eehCutDownFace;
			ShapeHandler::CreateShapeElement(eehCutDownFace, NULL, ptCut, 4, modelRef->Is3d(), *modelRef);
			EditElementHandle *eehSolid = new EditElementHandle;
			ISolidKernelEntityPtr ptarget;
			SolidUtil::Convert::ElementToBody(ptarget, eehCutDownFace);
			//eehCutDownFace.AddToModel();
			if (ptarget == NULL)
			{
				continue;
			}
			SolidUtil::Modify::ThickenSheet(ptarget, curWallHeight, 0.0);
			if (SUCCESS != SolidUtil::Convert::BodyToElement(*eehSolid, *ptarget, NULL, *ACTIVEMODEL))
			{
				continue;
			}
			vecCutWall.push_back(eehSolid);
			//eehSolid->AddToModel();
			ptLeftTop = ptRightTop;
			ptLeftDown = ptRightDown;
		}

		//处理最后一组
		DPoint3d ptCut[4] = { ptLeftDown,vecDownFontLine[0].point[1],vecDownBackLine[0].point[1],ptLeftTop };
		EditElementHandle eehCutDownFace;
		ShapeHandler::CreateShapeElement(eehCutDownFace, NULL, ptCut, 4, modelRef->Is3d(), *modelRef);
		EditElementHandle *eehSolid = new EditElementHandle;
		ISolidKernelEntityPtr ptarget;
		SolidUtil::Convert::ElementToBody(ptarget, eehCutDownFace);
		//eehCutDownFace.AddToModel();
		if (ptarget == NULL)
		{
			return true;
		}
		SolidUtil::Modify::ThickenSheet(ptarget, curWallHeight, 0.0);
		if (SUCCESS == SolidUtil::Convert::BodyToElement(*eehSolid, *ptarget, NULL, *ACTIVEMODEL))
		{
			vecCutWall.push_back(eehSolid);
		}
		//eehSolid->AddToModel();
	}
	return true;
}

void CutRebarSetByHoles2(RebarSetTagArray& rebarsettagas,std::vector<EditElementHandle*>& holes)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	RebarEndType endType;
	endType.SetType(RebarEndType::kNone);
	RebarEndTypes   endTypes = { endType, endType };
	RebarModel *rmv = RMV;
	for (int i = 0;i<rebarsettagas.GetSize();i++)
	{
		RebarSetP rebset = rebarsettagas.At(i).PopRset();
		int nNum = (int)rebset->GetChildElementCount(); // 钢筋组中钢筋数量
		
		for (int j = 0; j < nNum; j++)
		{
			RebarElementP pRebar = rebset->GetChildElement(j);
			double diameter = pRebar->GetRebarShape(ACTIVEMODEL)->GetDiameter();
			RebarCurve curve;
			RebarShape * rebarshape = pRebar->GetRebarShape(ACTIVEMODEL);
			if (rebarshape == nullptr)
			{
				continue;
			}
			rebarshape->GetRebarCurve(curve);
			BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
			CMatrix3D tmp3d(pRebar->GetLocation());
			curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
			curve.DoMatrix(pRebar->GetLocation());
			bvector<DPoint3d> ips;
			curve.GetIps(ips);
			if (ips.size()>2)
			{
				DPoint3d ptstr1 = ips[0];
				DPoint3d ptend1 = ips[1];

				DPoint3d ptstr2 = ips[ips.size() - 2];
				DPoint3d ptend2 = ips[ips.size() - 1];
				CalculateIntersetPtWithHolesWithRebarCuve(ptstr1, ptend1, ptend1, holes);
				CalculateIntersetPtWithHolesWithRebarCuve(ptstr2, ptend2, ptstr2, holes);

				curve.PopVertices().At(0).SetIP(ptstr1);
				curve.PopVertices().At(ips.size() - 1).SetIP(ptend2);

				RebarShapeDataP shape = const_cast<RebarShapeDataP>(pRebar->GetShapeData(ACTIVEMODEL));
				if (shape == nullptr)
				{
					continue;
				}
				pRebar->Update(curve, diameter, endTypes, *shape, ACTIVEMODEL, false);
				if (rmv != nullptr)
				{
					rmv->SaveRebar(*pRebar, pRebar->GetModelRef(), true);
				}
			}
		}
	}

}

bool ACCRebarMakerNew::CreateACCRebar(ACCRebarAssemblyNew*  rebar, ElementHandleCR currEh, std::vector<PIT::ConcreteRebar> vecRebarData, DgnModelRefP modelRef)
{
	if (rebar == NULL)
		return NULL;
	bool ret = false;
	CACCNew acc(currEh);
	vector<ACCRebarMakerNew*> vecrebarMaker;
	switch (rebar->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(rebar);
		vector<PIT::AssociatedComponent>& vecAC = rebarAssembly->PopvecAC();
		vector<DPoint3d>& vecCutPts = rebarAssembly->PopvecCutPoints();
//		bool bMakeURebar = false;
		std::vector<IntersectEle> vecInEle;
		const std::vector<IntersectEle>& veCACCNewData = acc.GetACCData();
		const std::vector<IntersectEle>& vecIntersectSlab = acc.GetIntersectSlab();
		vecInEle.clear();
		vecInEle.shrink_to_fit();
//		vecInEle.resize(vecBothEndACCData.size() + vecIntersectSlab.size());
		vecInEle.insert(vecInEle.begin(), veCACCNewData.begin(), veCACCNewData.end());
		vecInEle.insert(vecInEle.end(), vecIntersectSlab.begin(), vecIntersectSlab.end());

		vector<EditElementHandle*> vecCutWall;
		DSegment3d segLine;
		bool bRet = CutWallByACCData2(vecCutWall, vecCutPts, segLine, currEh, rebarAssembly->GetSTwallData().height, vecInEle, vecAC, modelRef);
		rebarAssembly->SetCutSoild(vecCutWall, segLine, rebarAssembly->GetSTwallData().height);
		for (size_t i = 0; i < vecInEle.size(); ++i)
		{
			string strACCName = vecInEle[i].EleName;
			auto it = std::find_if(vecAC.begin(), vecAC.end(), [&strACCName](const AssociatedComponent& tmphd) {return string(tmphd.associatedComponentName) == strACCName; });
			if (it != vecAC.end())
			{
				if ((*it).associatedRelation == 1|| (*it).associatedRelation == 2)	//锚入
				{
					ACCRebarMakerNew* rebarMaker = nullptr;
					switch ((*it).anchoringMethod)
					{
					case 0:
 						rebarMaker = new ACCRebarMethod1_MakerNew(currEh, vecInEle[i], rebar);
						rebarMaker->SetvecRebarData(vecRebarData);
 						rebarMaker->MakeRebars(modelRef);
						break;
					case 1:
						rebarMaker = new ACCRebarMethod2_MakerNew(currEh, vecInEle[i], rebar);
						rebarMaker->SetvecRebarData(vecRebarData);
						rebarMaker->MakeRebars(modelRef);
						break;
					case 2:
						rebarMaker = new ACCRebarMethod3_MakerNew(currEh, vecInEle[i], rebar);
						rebarMaker->SetvecRebarData(vecRebarData);
						rebarMaker->MakeRebars(modelRef);
						//					bMakeURebar = true;
						break;
					case 3:
						rebarMaker = new ACCRebarMethod4_MakerNew(currEh, vecInEle[i], rebar);
						rebarMaker->SetvecRebarData(vecRebarData);
						rebarMaker->MakeRebars(modelRef);
						break;
					case 6:
						rebarMaker = new ACCRebarMethod7_MakerNew(currEh, vecInEle[i], rebar);
						rebarMaker->SetvecRebarData(vecRebarData);
						rebarMaker->MakeRebars(modelRef);
						break;
					case 8:
						rebarMaker = new ACCRebarMethod9_MakerNew(currEh, vecInEle[i], rebar);
						rebarMaker->SetvecRebarData(vecRebarData);
						rebarMaker->MakeRebars(modelRef);
						break;
					case 9:
						rebarMaker = new ACCRebarMethod10_MakerNew(currEh, vecInEle[i], rebar);
						rebarMaker->SetvecRebarData(vecRebarData);
						rebarMaker->MakeRebars(modelRef);
						break;
					default:
						break;
					}
					if (rebarMaker!=nullptr)
					{
						vecrebarMaker.push_back(rebarMaker);
					}
				}
			}
		}
		rebarAssembly->InitUcsMatrix();
		ret = rebarAssembly->MakeRebars(modelRef);
		//绘制关联处的钢筋
		if (vecrebarMaker.size()>0)
		{
			rebarAssembly->PopvecSetId().push_back(0);
			for (ACCRebarMakerNew* rebarMaker:vecrebarMaker)
			{
				RebarSetTag* pTag = rebarMaker->MakeRebar(rebarAssembly->PopvecSetId().back(), modelRef);
				if (pTag != NULL && (!PreviewButtonDown))
				{
					if (rebarAssembly->m_rsetTags.GetSize() > 0)
					{
						pTag->SetBarSetTag((int)rebarAssembly->m_rsetTags.GetSize() + 1);
						rebarAssembly->m_rsetTags.Add(pTag);
					}
				}
				double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
				rebarMaker->m_Holeehs.insert(rebarMaker->m_Holeehs.end(), rebarAssembly->m_useHoleehs.begin(), rebarAssembly->m_useHoleehs.end());
				CutRebarSetByHoles2(rebarAssembly->m_rsetTags, rebarMaker->m_Holeehs);
			}	

		}
		if (g_globalpara.Getrebarstyle() != 0)
		{
			rebarAssembly->AddRebarSets(rebarAssembly->m_rsetTags);
		}

 	}
	break;
	default:
		break;
 	}

	for (ACCRebarMakerNew* rebarMaker : vecrebarMaker)
	{
		if (rebarMaker != NULL)
		{
			delete rebarMaker;
			rebarMaker = NULL;
		}
	}
	vecrebarMaker.clear();

 	return ret;
}

RebarSetTag * ACCRebarMakerNew::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	return NULL;
}

bool ACCRebarMakerNew::AnalyzingSTWallGeometricData(const ElementHandle & eh,ElementHandle& cureeh, STWallGeometryInfo & stwallData)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	DPoint3d FrontStart, FrontEnd;
	DPoint3d BackStart, BackEnd;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	vector<DSegment3d> vecDownFontLine;
	vector<DSegment3d> vecDownBackLine;

	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &stwallData.height);

	if (vecDownFontLine.empty() || vecDownBackLine.empty())
	{
		return false;
	}

	DPoint3d pt1[2];
	vecDownFontLine[0].GetStartPoint(pt1[0]);
	vecDownFontLine[0].GetEndPoint(pt1[1]);

	DPoint3d pt2[2];
	vecDownBackLine[0].GetStartPoint(pt2[0]);
	vecDownBackLine[0].GetEndPoint(pt2[1]);

	EFT::GetRegionPts(FrontStart, FrontEnd, BackStart, BackEnd, Negs, pt1, pt2, model, stwallData.height);
	if (m_Holeehs.size() == 0)
	{
		std::vector<EditElementHandle*> Holeehs2;
		EFT::GetSolidElementAndSolidHoles(cureeh, Eleeh, Holeehs2);
		//m_Holeehs.insert(m_Holeehs.begin(), Holeehs.begin(), Holeehs.end());
		m_Holeehs.insert(m_Holeehs.end(), Holeehs2.begin(), Holeehs2.end());
	}
	
	
	stwallData.height = stwallData.height*uor_now / uor_ref;
	stwallData.width = FrontStart.Distance(BackStart)*uor_now / uor_ref;
	stwallData.length = FrontStart.Distance(FrontEnd)*uor_now / uor_ref;
// 	m_InSecSTwallData.ptStart = FrontStart;
// 	m_InSecSTwallData.ptEnd = FrontEnd;

	FrontStart = pt1[0];
	FrontEnd = pt1[1];
	BackStart = pt2[0];
	BackEnd = pt2[1];
	DPoint3d ptStartCenter, ptEndCenter;
	ptStartCenter.x = (FrontStart.x + BackStart.x) * 0.5;
	ptStartCenter.y = (FrontStart.y + BackStart.y) * 0.5;
	ptStartCenter.z = (FrontStart.z + BackStart.z) * 0.5;
	ptEndCenter.x = (FrontEnd.x + BackEnd.x) * 0.5;
	ptEndCenter.y = (FrontEnd.y + BackEnd.y) * 0.5;
	ptEndCenter.z = (FrontEnd.z + BackEnd.z) * 0.5;

	stwallData.ptStart = ptStartCenter;
	stwallData.ptEnd = ptEndCenter;
	stwallData.ptPreStr = FrontStart;
	stwallData.ptPreEnd = FrontEnd;
	stwallData.ptBckStr = BackStart;
	stwallData.ptBckEnd = BackEnd;

	return false;
}

bool ACCRebarMakerNew::AnalyzingSlabGeometricData(ElementHandleCR eh, ElementHandle& cureeh,double& slabHeight)
{
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle testeeh(eh, false);

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
		vecDownBackLine, &slabHeight);

	for (size_t i = 0; i < vecDownFaceLine.size(); ++i)
	{
		mdlElmdscr_freeAll(&vecDownFaceLine[i]);
		vecDownFaceLine[i] = NULL;
	}
	if (m_Holeehs.size() == 0)
	{
		std::vector<EditElementHandle*> Holeehs2;
		EFT::GetSolidElementAndSolidHoles(cureeh, Eleeh, Holeehs2);
		m_Holeehs.insert(m_Holeehs.begin(), Holeehs.begin(), Holeehs.end());
		m_Holeehs.insert(m_Holeehs.end(), Holeehs2.begin(), Holeehs2.end());
	}
	return true;

}

bool ACCRebarMakerNew::CaluateSTWallIntersectPart(vector<DPoint3d>& vecInSecPartPt)
{
	//不与X轴垂直时，左边为起点，右边为终点
	//与X轴垂直时，下边为起点，上边为终点
	//0x00  不与X轴垂直，左上角
	//0x01  不与X轴垂直，右上角
	//0x11  不与X轴垂直，右下角
	//0x10  不与X轴垂直，左上角

	//0x100   与X轴垂直，右下角
	//0x101   与X轴垂直，右上角
	//0x111   与X轴垂直，左上角
	//0x110   与X轴垂直，左下角
	m_vecInSecPartPt.clear();
	DSegment3d segCur = DSegment3d::From(m_CurSTwallData.ptStart, m_CurSTwallData.ptEnd);
	DSegment3d segInSec = DSegment3d::From(m_InSecSTwallData.ptStart, m_InSecSTwallData.ptEnd);
	DVec3d vecCur = m_CurSTwallData.ptEnd - m_CurSTwallData.ptStart;
	DVec3d vecInSec = m_InSecSTwallData.ptEnd - m_InSecSTwallData.ptStart;
	DPoint3d ptIntersect;
	mdlVec_intersect(&ptIntersect, &segCur, &segInSec);
	DPoint3d ptRect[4] = { ptIntersect,ptIntersect, ptIntersect, ptIntersect };	//底面矩形的四个顶点
	vecCur.Normalize();
	vecCur.Scale(m_InSecSTwallData.width / 2);
	vecInSec.Normalize();
	vecInSec.Scale(m_CurSTwallData.width / 2);
	DVec3d vecCurNeg(vecCur);
	vecCurNeg.Negate();
	DVec3d vecInSecNeg(vecInSec);
	vecInSecNeg.Negate();

	UInt32 relation = ACCSTWallRebarAssemblyNew::JudgeComponentRelation(m_CurSTwallData, m_InSecSTwallData);
	int iIndex = 0;
	if (relation == 0x000 || relation == 0x100)
	{
		ptRect[0].Add(vecCurNeg);
		ptRect[0].Add(vecInSecNeg);
		ptRect[1].Add(vecCur);
		ptRect[1].Add(vecInSecNeg);
		ptRect[2].Add(vecCur);
		ptRect[2].Add(vecInSec);
		ptRect[3].Add(vecCurNeg);
		ptRect[3].Add(vecInSec);
	}
	else if (relation == 0x001 || relation == 0x101)
	{
		ptRect[0].Add(vecCur);
		ptRect[0].Add(vecInSecNeg);
		ptRect[1].Add(vecCurNeg);
		ptRect[1].Add(vecInSecNeg);
		ptRect[2].Add(vecCurNeg);
		ptRect[2].Add(vecInSec);
		ptRect[3].Add(vecCur);
		ptRect[3].Add(vecInSec);
	}
	else if (relation == 0x010 || relation == 0x110)
	{
		ptRect[0].Add(vecCurNeg);
		ptRect[0].Add(vecInSec);
		ptRect[1].Add(vecCur);
		ptRect[1].Add(vecInSec);
		ptRect[2].Add(vecCur);
		ptRect[2].Add(vecInSecNeg);
		ptRect[3].Add(vecCurNeg);
		ptRect[3].Add(vecInSecNeg);
	}
	else
	{
		ptRect[0].Add(vecCur);
		ptRect[0].Add(vecInSecNeg);
		ptRect[1].Add(vecCurNeg);
		ptRect[1].Add(vecInSecNeg);
		ptRect[2].Add(vecCurNeg);
		ptRect[2].Add(vecInSec);
		ptRect[3].Add(vecCur);
		ptRect[3].Add(vecInSec);
	}

	vecInSecPartPt = { ptRect[0],ptRect[1],ptRect[2],ptRect[3] };
	return false;
}

bool ACCRebarMakerNew::MakeRebars(DgnModelRefP modelRef)
{
	return false;
}
void ACCRebarMakerNew::PlusHoles(double dSideCover)
{
	Transform matrix;
	for (int j = 0; j < m_Holeehs.size(); j++)
	{
		PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
		
	}
}
bool ACCRebarMakerNew::GetCoverSideinFrontOrBack()
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = m_ACCEh.Eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = m_ACCEh.Eh.GetModelRef();
	vector<DSegment3d> vecDownFontLine;
	vector<DSegment3d> vecDownBackLine;

	EditElementHandle testeeh(m_ACCEh.Eh, false);
	//testeeh.AddToModel();

	EditElementHandle Eleeh;
	double height;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &height);
    if (vecDownFontLine.empty() || vecDownBackLine.empty())
	{
		return false;
	}

	DPoint3d FrProject, BkProject;
	vecDownFontLine[0].GetStartPoint(FrProject);
	vecDownBackLine[0].GetStartPoint(BkProject);

	//当前构件中心线起点到关联构件终点线段的投影点
	mdlVec_projectPointToLine(&FrProject, NULL, &FrProject, &m_CurSTwallData.ptStart, &m_CurSTwallData.ptEnd);
	mdlVec_projectPointToLine(&BkProject, NULL, &BkProject, &m_CurSTwallData.ptStart, &m_CurSTwallData.ptEnd);
	m_isFrCoverside = true;
	if (FrProject.Distance(m_CurSTwallData.ptStart)> FrProject.Distance(m_CurSTwallData.ptEnd))//当前配筋墙的起点较远
	{
		if (m_CurSTwallData.ptStart.Distance(FrProject)< m_CurSTwallData.ptStart.Distance(BkProject))	//背部较远时
		{
			m_isFrCoverside = false;
		}
	}
	else
	{
		if (m_CurSTwallData.ptEnd.Distance(FrProject) < m_CurSTwallData.ptEnd.Distance(BkProject))	//背部较远时
		{
			m_isFrCoverside = false;
		}
	}
	return true;
}
bool ACCRebarMakerNew::makeRebarCurve
(
	DPoint3d ptstr,
	DPoint3d ptend,
	vector<PITRebarCurve>&     rebars,
	PITRebarEndTypes&		endTypes,
	double dSideCover
)
{
	DPoint3d pt1[2];
	pt1[0] = ptstr;
	pt1[1] = ptend;
	//确保起点终点是从小到大---begin
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
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetIntersectPointsWithHoles(tmppts, m_Holeehs, pt1[0], pt1[1], dSideCover, matrix);
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
		PITRebarEndTypes		tmpendTypes;

		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itr->second);
		vex->SetType(RebarVertex::kStart);
		if (pt1[0].Distance(itr->second) < 10)
		{
			tmpendTypes.beg = endTypes.beg;
		}
		tmpendTypes.beg.SetptOrgin(itr->second);
		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		if (pt1[1].Distance(itrplus->second) < 10)
		{
			tmpendTypes.end = endTypes.end;
		}

		tmpendTypes.end.SetptOrgin(itrplus->second);

		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itrplus->second);
		vex->SetType(RebarVertex::kEnd);

		rebar.EvaluateEndTypes(tmpendTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebars.push_back(rebar);
	}


	//rebar.DoMatrix(mat);
	return true;
}
bool ACCRebarMethod1_MakerNew::MakeRebars(DgnModelRefP modelRef)
{
	if (m_RebarAssembly == NULL)
		return NULL;
	bool ret = false;
	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);

		const vector<PIT::AssociatedComponent>& vecAC = rebarAssembly->PopvecAC();
		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		double dOffset;
		string strName = m_ACCEh.EleName;
		auto it = std::find_if(vecAC.begin(), vecAC.end(), [&strName](AssociatedComponent tmphd) {return string(tmphd.associatedComponentName) == strName; });
		if (it != vecAC.end())
			dOffset = (*it).endOffset;

		int level = 0, endTypeIndex = 0;
		double angle = -90;
		vector<vector<PIT::EndType> > &vvEndType = rebarAssembly->PopvvecEndType();
		UInt32 relation = ACCSTWallRebarAssemblyNew::JudgeComponentRelation(m_CurSTwallData, m_InSecSTwallData);
		endTypeIndex = relation & 0x01;
		if (relation == 0x111 || relation == 0x110 || relation == 0x001 || relation == 0x000)
		{
			level = (int)vvEndType.size() - 1;
			angle = 90;
		}
		//关联构件的混凝土保护层
		ACCConcrete ACConcrete;
		int retGetACC = GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ACCConcrete), ACConcrete, 
			ConcreteCoverXAttribute, m_ACCEh.Eh.GetModelRef());
		if (retGetACC != SUCCESS)
		{
			ACConcrete = { 50,50,50 };
		}
		double disCover = 0;
		double InSecCover = 0;
		if (m_isFrCoverside)//取关联构件的前保护层偏差
		{
			disCover = ACConcrete.postiveOrTopCover - g_wallRebarInfo.concrete.sideCover;
			InSecCover = ACConcrete.postiveOrTopCover;
		}
		else
		{
			disCover = ACConcrete.reverseOrBottomCover - g_wallRebarInfo.concrete.sideCover;
			InSecCover = ACConcrete.reverseOrBottomCover;
		}
		//是否预留长度
		//取当前墙配筋钢筋直径和关键构件墙钢筋直径
		vector<PIT::ConcreteRebar> vecRebar;
		BrString strRebarSize;
		bool isHaveAssociated = false;//关联构件钢筋锚入是否已经配置
		if (m_ACCEh.Eh.IsValid())	//取关联构件的钢筋尺寸，若关联构件未配筋，则无需绘制交集处纵向钢筋
		{
			ElementId concreteid = 0;
			GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
			GetElementXAttribute(concreteid, vecRebar, vecRebarDataXAttribute, ACTIVEMODEL);
			if (vecRebar.empty())//关联构件未配置钢筋
			{
				vector<PIT::AssociatedComponent>& vecRebarAC = rebarAssembly->GetvecAC();
				if (vecRebarAC.size())
				{
					auto it = std::find_if(vecRebarAC.begin(), vecRebarAC.end(), [&](AssociatedComponent ac) {return ac.endOffset != 0; });
					if (it != vecRebarAC.end())
					{
						double dim = (*it).endOffset * 0.5;
						strRebarSize.Format(L"%d", (int)dim);
						strRebarSize = strRebarSize + "A";
					}
				}
				else
				{
					strRebarSize = L"32A";	//若没有数据，默认为32mm钢筋
				}
			}
			else
			{
				strRebarSize = XmlManager::s_alltypes[vecRebar[0].rebarType];	//关联构件已配筋则取关联构件第一层钢筋直径
			}
		}
	
		BrString strCurRebarSize = XmlManager::s_alltypes[rebarAssembly->GetvecRebarType()[0]];//当前墙主筋
		
		double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
		double curDiameter = RebarCode::GetBarDiameter(strCurRebarSize, modelRef);

		//取当前墙配筋钢筋直径和关键构件墙钢筋直径


// 		EditElementHandle eeh;
// 		LineHandler::CreateLineElement(eeh, NULL, DSegment3d::From(m_CurSTwallData.ptStart, m_CurSTwallData.ptEnd), true, *ACTIVEMODEL);
// 		eeh.AddToModel();
		if (relation & 0x01)		//关联构件在当前构件的终点
		{
			//判断点是否在线上
			bool bNearStart = true;
			DPoint3d ptPro2;
			//当前构件中心线起点到关联构件终点线段的投影点
			mdlVec_projectPointToLine(&ptPro2, NULL, &m_CurSTwallData.ptEnd, &m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptEnd);
			if (!EFT::IsPointInLine(ptPro2, m_InSecSTwallData.ptStart, m_InSecSTwallData.ptEnd, modelRef, bNearStart))
			{
				//不在线上
				DPoint3d vec = m_CurSTwallData.ptStart - m_CurSTwallData.ptEnd;
				vec.Normalize();
				vec.Scale(m_InSecSTwallData.width);
				rebarAssembly->PopSTwallData().ptEnd.Add(vec);
				rebarAssembly->PopSTwallData().length -= m_InSecSTwallData.width;
				if (rebarAssembly->m_vecCutWallData.size() > 0)
				{
					rebarAssembly->m_vecCutWallData.back().ptEnd.Add(vec);
					rebarAssembly->m_vecCutWallData.back().length -= m_InSecSTwallData.width;
				}
			}
		}
		else
		{
			//判断点是否在线上
			DPoint3d ptPro1;
			//当前构件中心线起点到关联构件终点线段的投影点
			mdlVec_projectPointToLine(&ptPro1, NULL, &m_CurSTwallData.ptStart, &m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptEnd);
			bool bNearStart = true;
			if (!EFT::IsPointInLine(ptPro1, m_InSecSTwallData.ptStart, m_InSecSTwallData.ptEnd, modelRef, bNearStart))
			{
				//在线上
				DPoint3d vec = m_CurSTwallData.ptEnd - m_CurSTwallData.ptStart;
				vec.Normalize();
				vec.Scale(m_InSecSTwallData.width);
				rebarAssembly->PopSTwallData().ptStart.Add(vec);
				rebarAssembly->PopSTwallData().length -= m_InSecSTwallData.width;
				if (rebarAssembly->m_vecCutWallData.size() > 0)
				{
					rebarAssembly->m_vecCutWallData.front().ptStart.Add(vec);
					rebarAssembly->m_vecCutWallData.front().length -= m_InSecSTwallData.width;
				}
			}
		}
		int HJnum = 0;
		//终点偏移
		if (vvEndType.size())
		{
			vector<PIT::ConcreteRebar> vecRebarData;
			rebarAssembly->GetRebarData(vecRebarData);

			vvEndType[level][endTypeIndex].offset = dOffset;//当前构件端点偏移
			for (size_t i = 0; i < vecRebarData.size(); ++i)
			{
				if (vecRebarData[i].rebarDir == 0)	//所有横向钢筋都弯曲
				{
					vvEndType[i][endTypeIndex].rotateAngle = angle;//端部弯钩方向旋转
					vvEndType[i][endTypeIndex].endType = 4;//端部弯曲方式 90度弯曲 与上边变量一致
					vvEndType[i][endTypeIndex].offset -= m_InSecSTwallData.width / uor_per_mm - disCover;//端点延申
					double dim = RebarCode::GetBarDiameter(strCurRebarSize, modelRef);
					double dRadius = RebarCode::GetPinRadius(strCurRebarSize, modelRef, false);
					double disLenth;
					double LaLenth = g_globalpara.m_alength[(string)strCurRebarSize] * uor_per_mm;
					if (level != 0)//如果在最前面层
					{
						if (HJnum == 0)//第一次碰到横筋
						{
							//disLenth = m_InSecSTwallData.width - InSecCover * uor_per_mm - dRadius + PI / 2 * (dRadius - dim / 2);
							disLenth = LaLenth - (m_CurSTwallData.width - g_wallRebarInfo.concrete.postiveCover*uor_per_mm - dRadius - 50*uor_per_mm);
							HJnum = 1;
						}
						else//计算锚固的长度
						{
							disLenth = m_InSecSTwallData.width - InSecCover * uor_per_mm - dRadius + PI / 2 * (dRadius - dim / 2) - 2*dim;
						}		
					}
					else
					{
						if (HJnum > 0)
						{
							disLenth = LaLenth - (m_CurSTwallData.width - g_wallRebarInfo.concrete.reverseCover*uor_per_mm - dRadius - 50 * uor_per_mm);
						}
						else//第一次碰到横筋
						{
							disLenth = m_InSecSTwallData.width - InSecCover * uor_per_mm - dRadius + PI / 2 * (dRadius - dim / 2) - 2 * dim;
						}
						HJnum = 1;
					}
					if (LaLenth - disLenth<12*curDiameter)
					{
						vvEndType[i][endTypeIndex].endPtInfo.value3 = 12*curDiameter;
					}
					else
					{
						vvEndType[i][endTypeIndex].endPtInfo.value3 = LaLenth - disLenth;
					}
					


				}
				
			}
		}
	}
	break;
	case ACCRebarAssemblyNew::ComponentType::GWALL:
	{
		//		GWallRebarAssembly *rebarAssembly = dynamic_cast<GWallRebarAssembly*>(m_RebarAssembly);
	}
	break;
	default:
		break;
	}

	return ret;
}


RebarSetTag * ACCRebarMethod1_MakerNew::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	if (m_RebarAssembly == NULL)
		return NULL;

	RebarSetTag *tag = NULL;
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->StartUpdate(modelRef);
	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);
		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
			
		ReCalculateWidthAndPoint();
		CaluateSTWallIntersectPart(m_vecInSecPartPt);
		/*m_InSecSTwallData.ptStart = tmpstr;
		m_InSecSTwallData.ptEnd = tmpend;
		m_InSecSTwallData.width = tmpwidth;*/
// 		EditElementHandle eeh;
// 		LineStringHandler::CreateLineStringElement(eeh, NULL, &m_vecInSecPartPt[0],4, true, *ACTIVEMODEL);
// 		eeh.AddToModel();
		if (m_vecInSecPartPt.size() != 4)
		{
			return NULL;
		}
		DPoint3d ptCenter = (CPoint3D(m_vecInSecPartPt[0]) + CPoint3D(m_vecInSecPartPt[2])) / 2;
		bool bStart = false;
		DPoint3d cenptPro;
		//当前构件中心线起点到关联构件终点线段的投影点
		mdlVec_projectPointToLine(&cenptPro, NULL, &ptCenter, &m_CurSTwallData.ptStart, &m_CurSTwallData.ptEnd);
		if (!EFT::IsPointInLine(cenptPro,m_CurSTwallData.ptStart,m_CurSTwallData.ptEnd,ACTIVEMODEL, bStart))
		{
			return NULL;
		}

		vector<PIT::ConcreteRebar> vecRebar;
		bool isHaveAssociated = false;//关联构件钢筋锚入是否已经配置
		BrString strRebarSize;
		if (m_ACCEh.Eh.IsValid())	//取关联构件的钢筋尺寸，若关联构件未配筋，则无需绘制交集处纵向钢筋
		{
			ElementId concreteid = 0;
			GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
			GetElementXAttribute(concreteid, vecRebar, vecRebarDataXAttribute, ACTIVEMODEL);
			if (vecRebar.empty())//关联构件未配置钢筋
			{
				vector<PIT::AssociatedComponent>& vecRebarAC = rebarAssembly->GetvecAC();
				if (vecRebarAC.size())
				{
					auto it = std::find_if(vecRebarAC.begin(), vecRebarAC.end(), [&](AssociatedComponent ac) {return ac.endOffset != 0; });
					if (it != vecRebarAC.end())
					{
						double dim = (*it).endOffset * 0.5;
						strRebarSize.Format(L"%d", (int)dim);
						strRebarSize = strRebarSize + "A";
					}
				}
				else
				{
					strRebarSize = L"32A";	//若没有数据，默认为32mm钢筋
				}
			}
			else
			{
				strRebarSize = XmlManager::s_alltypes[vecRebar.at(0).rebarType];	//关联构件已配筋则取关联构件第一层钢筋直径
			}
		}
		
		BrString strCurRebarSize = XmlManager::s_alltypes[rebarAssembly->GetvecRebarType().at(0)];	//当前墙主筋
		double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
		double curDiameter = RebarCode::GetBarDiameter(strCurRebarSize, modelRef);

		double actDiameter = diameter > curDiameter ? diameter : curDiameter;
		BrString strActRebarSize = diameter > curDiameter ? strRebarSize : strCurRebarSize;

		double adjustedXLen, adjustedXSpacing;
		double adjustedYLen, adjustedYSpacing;

		double sideCover = rebarAssembly->GetSideCover()*uor_per_mm;

		int numRebar = 0;
		double spacing = 200*uor_per_mm;
		adjustedXLen = m_InSecSTwallData.width - sideCover * 2 - actDiameter * 2 - diameter;
		int numRebarX = (int)floor(adjustedXLen / spacing + 0.5) + 1;
		if (numRebarX==0)
		{
			return tag;
		}
		adjustedXSpacing = spacing;
		if (numRebarX > 1)
			adjustedXSpacing = adjustedXLen / (numRebarX - 1);

		adjustedYLen = m_CurSTwallData.width - sideCover * 2 - actDiameter * 2 - curDiameter;
		int numRebarY = (int)floor(adjustedYLen / spacing + 0.5) + 1;
		adjustedYSpacing = spacing;
		if (numRebarY > 1)
			adjustedYSpacing = adjustedYLen / (numRebarY - 1);
		numRebar = (numRebarX + numRebarY) * 2 - 4;

		vector<DSegment3d> vecRebarPt;
		DVec3d vec1 = m_vecInSecPartPt[1] - m_vecInSecPartPt[0];
		DVec3d vec2 = m_vecInSecPartPt[2] - m_vecInSecPartPt[1];
		vec1.Normalize();
		vec2.Normalize();
		double dOffset = actDiameter*1.5 + sideCover;
		DVec3d vecY = vec2;
		vecY.Scale(dOffset);
		DPoint3d ptOrign = m_vecInSecPartPt[0];
		ptOrign.Add(vecY);
		for (int i = 0; i < numRebarX; ++i)
		{
			PITRebarCurve rebarcurve;
			vec1.ScaleToLength(dOffset);
			DPoint3d ptStart = ptOrign;
			ptStart.Add(vec1);
			ptStart.z += sideCover;
			DPoint3d ptEnd = ptStart;
			ptEnd.z += m_CurSTwallData.height;
			ptEnd.z -= sideCover*2;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
			dOffset += adjustedXSpacing;
		}

		dOffset = adjustedYSpacing;
		for (int i = 0; i < numRebarY - 1; ++i)
		{
			DPoint3d ptStart = vecRebarPt.back().point[0];
			vec2.ScaleToLength(dOffset);
			ptStart.Add(vec2);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
		}

		DVec3d vec3 = m_vecInSecPartPt[3] - m_vecInSecPartPt[2];
		dOffset = adjustedXSpacing;
		vec3.Normalize();
		for (int i = 0; i < numRebarX - 1; ++i)
		{
			DPoint3d ptStart = vecRebarPt.back().point[0];
			vec3.ScaleToLength(dOffset);
			ptStart.Add(vec3);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
		}

		DVec3d vec4 = m_vecInSecPartPt[0] - m_vecInSecPartPt[3];
		dOffset = adjustedYSpacing;
		vec4.Normalize();
		for (int i = 0; i < numRebarY - 2; ++i)
		{
			DPoint3d ptStart = vecRebarPt.back().point[0];
			vec4.ScaleToLength(dOffset);
			ptStart.Add(vec4);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
		}

		if (vecRebarPt.size())
		{
			double bendRadius = RebarCode::GetPinRadius(strActRebarSize, modelRef, false);
			DVec3d vecA = vec1;
			DVec3d vecB = vec2;
			dOffset = bendRadius - sin(PI / 4) * (bendRadius - actDiameter * 0.5) + actDiameter + sideCover;
			vecA.ScaleToLength(dOffset);
			vecB.ScaleToLength(dOffset);
			DPoint3d ptO(m_vecInSecPartPt[0]);
			ptO.Add(vecA);
			ptO.Add(vecB);
			ptOrign = m_vecInSecPartPt[0];

			ptOrign.Add(vecA);
			ptOrign.Add(vecB);
			double dStartZ = vecRebarPt[0].point[0].z;
			vecRebarPt[0].point[0] = ptOrign;
			vecRebarPt[0].point[0].z = dStartZ;
			double dEndZ = vecRebarPt[0].point[1].z;
			vecRebarPt[0].point[1] = ptOrign;
			vecRebarPt[0].point[1].z = dEndZ;

// 			EditElementHandle eeh;
// 			LineHandler::CreateLineElement(eeh, NULL, DSegment3d::From(ptIntersect, ptO), true, *ACTIVEMODEL);
// 			eeh.AddToModel();
// 			EditElementHandle eeh1;
// 			LineHandler::CreateLineElement(eeh1, NULL, DSegment3d::From(ptIntersect, ptOrign), true, *ACTIVEMODEL);
// 			eeh1.AddToModel();
		}

		vector<PITRebarCurve> vecRebars;
		RebarEndType endTypeStart, endTypeEnd;
		endTypeStart.SetType(RebarEndType::kNone);
		endTypeEnd.SetType(RebarEndType::kNone);
		RebarEndTypes endTypes = { endTypeStart ,endTypeEnd };
		for (size_t i = 0; i < vecRebarPt.size();++i)
		{
			DPoint3d pt1[2];
			pt1[0] = vecRebarPt[i].point[0];
			pt1[1] = vecRebarPt[i].point[1];
			PITRebarEndTypes endTypes;
			endTypes.beg.SetType(PITRebarEndType::kNone);
			endTypes.end.SetType(PITRebarEndType::kNone);
			makeRebarCurve(vecRebarPt[i].point[0], vecRebarPt[i].point[1], vecRebars, endTypes, sideCover);
//			PITRebarCurve rebarcurve;
//			PITRebarEndTypes endTypes;
//			RebarVertexP vex;
//			vex = &rebarcurve.PopVertices().NewElement();
//			vex->SetIP(vecRebarPt[i].point[0]);
//			vex->SetType(RebarVertex::kStart);
//
//			vex = &rebarcurve.PopVertices().NewElement();
//			vex->SetIP(vecRebarPt[i].point[1]);
////			vex->SetType(RebarVertex::kEnd);
//
//			endTypes.beg.SetType(PITRebarEndType::kNone);
//			endTypes.end.SetType(PITRebarEndType::kNone);
//
//			rebarcurve.EvaluateEndTypes(endTypes);
//			vecRebars.push_back(rebarcurve);

// 			EditElementHandle eeh;
// 			LineHandler::CreateLineElement(eeh, NULL, vecRebarPt[i], true, *ACTIVEMODEL);
// 			eeh.AddToModel();
		}
		//确保起点终点是从小到大---begin
		

		//预览按钮按下，则画线
		if (PreviewButtonDown)
		{
			//绘制关联处钢筋，并将线段存入m_allLines
			for (auto it = vecRebarPt.begin(); it != vecRebarPt.end(); it++)
			{
				DSegment3d Dsegtemp(*it);
				DPoint3d strPoint = DPoint3d::From(Dsegtemp.point[0].x, Dsegtemp.point[0].y, Dsegtemp.point[0].z);
				DPoint3d endPoint = DPoint3d::From(Dsegtemp.point[1].x, Dsegtemp.point[1].y, Dsegtemp.point[1].z);
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_RebarAssembly->m_allLines.push_back(eeh.GetElementRef());
			}
		}


		RebarSymbology symb;
		string str(strActRebarSize);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);

		for (size_t i = 0; i < vecRebars.size(); ++i)
		{
			RebarElementP rebarElement = NULL;
			if(!PreviewButtonDown)//不是预览的情况下才生成钢筋
			{
				rebarElement = rebarSet->AssignRebarElement((int)i, (int)vecRebars.size(), symb, modelRef);
			}		
			PITRebarCurve rebarCurve = vecRebars[i];
			if (NULL != rebarElement)
			{
				RebarShapeData shape;
				shape.SetSizeKey((LPCTSTR)strActRebarSize);
				shape.SetIsStirrup(false);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				rebarElement->Update(rebarCurve, actDiameter, endTypes, shape, modelRef, false);
			}

			// 添加钢筋层号和正反面
			CPoint3D ptstr, ptend;
			rebarCurve.GetEndPoints(ptstr, ptend);
			ptstr.z = m_CurSTwallData.ptEnd.z;
			DPoint3d ptTmp = ptstr;
			double dDistance = sideCover + diameter;
			AddRebarLevelAndType(rebarElement, strCurRebarSize, ptTmp, m_CurSTwallData.ptStart, m_CurSTwallData.ptEnd, dDistance);
		}

		RebarSetData setdata;
		setdata.SetNumber((int)vecRebars.size());
		setdata.SetNominalSpacing(spacing);
		setdata.SetAverageSpacing(adjustedXSpacing);

		rebarSet->FinishUpdate(setdata, modelRef);
	}
	break;
	case ACCRebarAssemblyNew::ComponentType::GWALL:
	{
		//		GWallRebarAssembly *rebarAssembly = dynamic_cast<GWallRebarAssembly*>(m_RebarAssembly);
	}
	break;
	default:
		break;
	}

	tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);

	return tag;
}

bool ACCRebarMethod2_MakerNew::MakeRebars(DgnModelRefP modelRef)
{
	if (m_RebarAssembly == NULL)
		return NULL;
	bool ret = false;
	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);

// 		STWallGeometryInfo m_CurSTwallData;
// 		AnalyzingSTWallGeometricData(m_pACC->GetElementHandle(), m_CurSTwallData);
// 
// 		STWallGeometryInfo m_InSecSTwallData;
// 		AnalyzingSTWallGeometricData(m_ACCEh.Eh, m_InSecSTwallData);

		vector<PIT::AssociatedComponent>& vecAC = rebarAssembly->PopvecAC();

		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		double dOffset = 0.0;
		double L0 = 0.0,startL0 = 0.0,endL0 = 0.0;
		string strName = m_ACCEh.EleName;
		auto it = std::find_if(vecAC.begin(), vecAC.end(), [&strName](const AssociatedComponent &tmphd) {return string(tmphd.associatedComponentName) == strName; });
		if (it != vecAC.end())
		{
			dOffset = (*it).endOffset;
			startL0 = (*it).startL0;
			endL0 = (*it).endL0;
		}

		vector<PIT::ConcreteRebar> vecRebar;
		bool isHaveAssociated = false;//关联构件钢筋锚入是否已经配置
		BrString strRebarSize;
		if (m_ACCEh.Eh.IsValid())	//取关联构件的钢筋尺寸，若关联构件未配筋，则无需绘制交集处纵向钢筋
		{
			ElementId concreteid = 0;
			GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
			GetElementXAttribute(concreteid, vecRebar, vecRebarDataXAttribute, ACTIVEMODEL);
			if (vecRebar.empty())//关联构件未配置钢筋
			{
				vector<PIT::AssociatedComponent>& vecRebarAC = rebarAssembly->GetvecAC();
				if (vecRebarAC.size())
				{
					auto it = std::find_if(vecRebarAC.begin(), vecRebarAC.end(), [&](AssociatedComponent ac) {return ac.endOffset != 0; });
					if (it != vecRebarAC.end())
					{
						double dim = (*it).endOffset * 0.5;
						strRebarSize.Format(L"%d", (int)dim);
						strRebarSize = strRebarSize + "A";
					}
				}
				else
				{
					strRebarSize = L"32A";	//若没有数据，默认为32mm钢筋
				}
			}
			else
			{
				strRebarSize = XmlManager::s_alltypes[vecRebar.at(0).rebarType];	//关联构件已配筋则取关联构件第一层钢筋直径
			}
		}

		BrString strCurRebarSize = XmlManager::s_alltypes[rebarAssembly->GetvecRebarType().at(0)];	//当前墙主筋
		double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
		double curDiameter = RebarCode::GetBarDiameter(strCurRebarSize, modelRef);
		
		//取当前墙配筋钢筋直径和关键构件墙钢筋直径



		//取当前构件的混凝土保护层
		/*WallRebarInfo curConcrete;
		if (GetCurElementHandle().IsValid())
		{
			ElementId concreteid = 0;
			GetElementXAttribute(GetCurElementHandle().GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
			if (SUCCESS != GetElementXAttribute(concreteid, sizeof(WallRebarInfo), curConcrete, WallRebarInfoXAttribute, ACTIVEMODEL))
			{
				curConcrete.concrete.sideCover = 50;
				curConcrete.concrete.postiveCover = 50;
				curConcrete.concrete.reverseCover = 50;
			}
		}*/
		//关联构件的混凝土保护层
		ACCConcrete ACConcrete;
		int retGetACC = GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ACCConcrete), ACConcrete, ConcreteCoverXAttribute, m_ACCEh.Eh.GetModelRef());
		if (retGetACC != SUCCESS)
		{
			ACConcrete = { 50,50,50 };
		}
		double disCover = 0;
		double InSecCover = 0;
		if (m_isFrCoverside)//取关联构件的前保护层偏差
		{
			disCover = ACConcrete.postiveOrTopCover - g_wallRebarInfo.concrete.sideCover;
			InSecCover = ACConcrete.postiveOrTopCover;
		}
		else
		{
			disCover = ACConcrete.reverseOrBottomCover - g_wallRebarInfo.concrete.sideCover;
			InSecCover = ACConcrete.reverseOrBottomCover;
		}
		//是否预留长度
		bool bReservedlen = true;
		//if (COMPARE_VALUES_EPS(m_CurSTwallData.width - curConcrete.concrete.sideCover, m_InSecSTwallData.width - ACConcrete.sideCover, 1) >= 0)
		//{
		//	bReservedlen = true;
		//	if (COMPARE_VALUES_EPS(m_CurSTwallData.width - curConcrete.concrete.sideCover, m_InSecSTwallData.width - ACConcrete.sideCover, 1) == 0)
		//	{
		//		//判断关联构件是否已经配筋，若关联构件已经配筋则当前构件无需再预留长度
		//		ElementId concreteid = 0;
		//		GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
		//		vector<PIT::ConcreteRebar> vec11;
		//		GetElementXAttribute(concreteid, vec11, vecRebarDataXAttribute, ACTIVEMODEL);
		//		RebarAssembly *rebar = ACCRebarAssemblyNew::GetRebarAssembly(concreteid, "class PIT::ACCSTWallRebarAssemblyNew");
		//		if (rebar != nullptr)
		//		{
		//			bReservedlen = false;
		//		}
		//	}
		//}

		int level = 0, endTypeIndex = 0;
		double angle = -90;
		vector<vector<PIT::EndType> > &vvEndType = rebarAssembly->PopvvecEndType();
		UInt32 relation = ACCSTWallRebarAssemblyNew::JudgeComponentRelation(m_CurSTwallData, m_InSecSTwallData);
		endTypeIndex = relation & 0x01;
		if (it != vecAC.end())
		{
			(*it).acPositon = endTypeIndex;
		}
		L0 = relation & 0x01 ? endL0 : startL0;
		int reservedLenLevel = (int)vvEndType.size() - 1;
		if (relation == 0x111 || relation == 0x110 || relation == 0x001 || relation == 0x000)
		{
			level = (int)vvEndType.size() - 1;
			reservedLenLevel = 0;
			angle = 90;
		}

		/*if (relation == 0x000 || relation == 0x001 || relation == 0x111)
			angle = 90;*/

		int HJnum = 0;

		if (relation & 0x01)		//关联构件在当前构件的终点
		{
			//判断点是否在线上
			bool bNearStart = true;
			DPoint3d ptPro2;
			//当前构件中心线起点到关联构件终点线段的投影点
			mdlVec_projectPointToLine(&ptPro2, NULL, &m_CurSTwallData.ptEnd, &m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptEnd);
			if (!EFT::IsPointInLine(ptPro2, m_InSecSTwallData.ptStart, m_InSecSTwallData.ptEnd, modelRef, bNearStart))
			{
				isHaveAssociated = true;
				//不在线上
				DPoint3d vec = m_CurSTwallData.ptStart - m_CurSTwallData.ptEnd;
				vec.Normalize();
				vec.Scale(m_InSecSTwallData.width);
				rebarAssembly->PopSTwallData().ptEnd.Add(vec);
				rebarAssembly->PopSTwallData().length -= m_InSecSTwallData.width;
				if (rebarAssembly->m_vecCutWallData.size() > 0)
				{
					rebarAssembly->m_vecCutWallData.back().ptEnd.Add(vec);
					rebarAssembly->m_vecCutWallData.back().length -= m_InSecSTwallData.width;
				}
			}
		}
		else
		{
			//判断点是否在线上
			DPoint3d ptPro1;
			//当前构件中心线起点到关联构件终点线段的投影点
			mdlVec_projectPointToLine(&ptPro1, NULL, &m_CurSTwallData.ptStart, &m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptEnd);
			bool bNearStart = true;
			if (!EFT::IsPointInLine(ptPro1, m_InSecSTwallData.ptStart, m_InSecSTwallData.ptEnd, modelRef, bNearStart))
			{
				isHaveAssociated = true;
				//在线上
				DPoint3d vec = m_CurSTwallData.ptEnd - m_CurSTwallData.ptStart;
				vec.Normalize();
				vec.Scale(m_InSecSTwallData.width);
				rebarAssembly->PopSTwallData().ptStart.Add(vec);
				rebarAssembly->PopSTwallData().length -= m_InSecSTwallData.width;
				if (rebarAssembly->m_vecCutWallData.size() > 0)
				{
					rebarAssembly->m_vecCutWallData.front().ptStart.Add(vec);
					rebarAssembly->m_vecCutWallData.front().length -= m_InSecSTwallData.width;
				}
			}
		}
		//终点偏移
		if (vvEndType.size())
		{
			vector<PIT::ConcreteRebar> vecRebarData;
			rebarAssembly->GetRebarData(vecRebarData);

			vvEndType[level][endTypeIndex].offset = dOffset;//当前构件端点偏移
			for (size_t i = 0; i < vecRebarData.size(); ++i)
			{
				if (vecRebarData[i].rebarDir == 0)	//所有横向钢筋都弯曲
				{
					if (level!=0)//如果在最前面层
					{
						if (HJnum==0)//第一次碰到横筋
						{
							if ((curDiameter>diameter+10)||(COMPARE_VALUES_EPS(curDiameter,diameter,1)==0&&isHaveAssociated==false))//如果当前的直径大于等于
							{
								vvEndType[i][endTypeIndex].rotateAngle = angle;//端部弯钩方向旋转
								vvEndType[i][endTypeIndex].endType = 4;//端部弯曲方式 90度弯曲 与上边变量一致
								vvEndType[i][endTypeIndex].offset -= m_InSecSTwallData.width / uor_per_mm - disCover;
								//vvEndType[i][endTypeIndex].offset -= (m_InSecSTwallData.width / uor_per_mm -
								//	                                   ACConcrete.postiveOrTopCover + curConcrete.concrete.sideCover);//端点延申
								double dRadius = RebarCode::GetPinRadius(strCurRebarSize, modelRef, false);
								double L0Lenth = g_globalpara.m_laplenth[(string)strCurRebarSize] * uor_per_mm;
								vvEndType[i][endTypeIndex].endPtInfo.value3 = L0Lenth +
									m_CurSTwallData.width - g_wallRebarInfo.concrete.sideCover*uor_per_mm
									- dRadius + 50 * uor_per_mm;
							}
							else
							{
								HJnum = 1;
								continue;
							}
							HJnum = 1;
						}
						else//计算锚固的长度
						{
							vvEndType[i][endTypeIndex].rotateAngle = angle;//端部弯钩方向旋转
							vvEndType[i][endTypeIndex].endType = 4;//端部弯曲方式 90度弯曲 与上边变量一致
							vvEndType[i][endTypeIndex].offset -= m_InSecSTwallData.width / uor_per_mm - disCover;

							double dRadius = RebarCode::GetPinRadius(strCurRebarSize, modelRef, false);
							double disLenth = m_InSecSTwallData.width - InSecCover * uor_per_mm - 2 * diameter - dRadius + PI / 2 * (dRadius - curDiameter / 2);
							//double LaLenth = g_globalpara.m_alength[(int)curDiameter / uor_per_mm] * uor_per_mm;
							double LaLenth = g_globalpara.m_alength[(string)strCurRebarSize] * uor_per_mm;;
							
							if (LaLenth - disLenth < 12 * curDiameter)
							{
								vvEndType[i][endTypeIndex].endPtInfo.value3 = 12 * curDiameter;
							}
							else
							{
								vvEndType[i][endTypeIndex].endPtInfo.value3 = LaLenth - disLenth;
							}
							//vvEndType[i][endTypeIndex].endPtInfo.value3 = g_globalpara.m_laplenth[atoi(vecRebarData[i].rebarSize)] * uor_per_mm;

						}
					}
					else
					{
						if (HJnum > 0)//第二次碰到横筋
						{
							if ((curDiameter > diameter + 10) || (COMPARE_VALUES_EPS(curDiameter, diameter, 1) == 0 && isHaveAssociated == false))//如果当前的直径大于等于
							{
								vvEndType[i][endTypeIndex].rotateAngle = angle;//端部弯钩方向旋转
								vvEndType[i][endTypeIndex].endType = 4;//端部弯曲方式 90度弯曲 与上边变量一致
								vvEndType[i][endTypeIndex].offset -= m_InSecSTwallData.width / uor_per_mm - disCover;
								//vvEndType[i][endTypeIndex].offset -= (m_InSecSTwallData.width / uor_per_mm -
								//	                                   ACConcrete.postiveOrTopCover + curConcrete.concrete.sideCover);//端点延申
								//double dRadius = RebarCode::GetPinRadius(vecRebarData[i].rebarSize, modelRef, false);
								double dRadius = RebarCode::GetPinRadius(strCurRebarSize, modelRef, false);
								double L0Lenth = g_globalpara.m_laplenth[(string)strCurRebarSize] * uor_per_mm;
								vvEndType[i][endTypeIndex].endPtInfo.value3 = L0Lenth +
									m_CurSTwallData.width - g_wallRebarInfo.concrete.sideCover*uor_per_mm
									- dRadius + 50 * uor_per_mm;
							}
							else
							{
								continue;
							}
						}
						else//计算锚固的长度
						{
							vvEndType[i][endTypeIndex].rotateAngle = angle;//端部弯钩方向旋转
							vvEndType[i][endTypeIndex].endType = 4;//端部弯曲方式 90度弯曲 与上边变量一致
							vvEndType[i][endTypeIndex].offset -= m_InSecSTwallData.width / uor_per_mm - disCover;
							double dRadius = RebarCode::GetPinRadius(strCurRebarSize, modelRef, false);
							double disLenth = m_InSecSTwallData.width - InSecCover * uor_per_mm - 2 * diameter - dRadius + PI / 2 * (dRadius - curDiameter / 2);
							//double LaLenth = g_globalpara.m_alength[(int)curDiameter / uor_per_mm] * uor_per_mm;
							double LaLenth = g_globalpara.m_alength[(string)strCurRebarSize] * uor_per_mm;
							if (LaLenth - disLenth < 12 * curDiameter)
							{
								vvEndType[i][endTypeIndex].endPtInfo.value3 = 12 * curDiameter;
							}
							else
							{
								vvEndType[i][endTypeIndex].endPtInfo.value3 = LaLenth - disLenth;
							}
						}
						HJnum = 1;
					}


					
				}
// 				else
// 				{
// 					double dRadius = RebarCode::GetPinRadius(vecRebarData[i].rebarSize, modelRef, false);
// 					dRadius /= uor_per_mm;
// 					if (0 == endTypeIndex)
// 						rebarAssembly->PopvecStartOffset().at(i) += dRadius;
// 					else
// 						rebarAssembly->PopvecEndOffset().at(i) += dRadius;
// 				}
			}
			//if (!bReservedlen)
			//{
			//	vvEndType[reservedLenLevel][endTypeIndex].endType = 0;	//不预留长度的不需要弯曲
			//	vvEndType[reservedLenLevel][endTypeIndex].offset = -g_wallRebarInfo.concrete.sideCover;
			//}
			//else
			//{
			//	vvEndType[reservedLenLevel][endTypeIndex].endPtInfo.value3 += L0 * uor_per_mm;//预留长度
			//}
		}
	}
	break;
	case ACCRebarAssemblyNew::ComponentType::GWALL:
	{
		//		GWallRebarAssembly *rebarAssembly = dynamic_cast<GWallRebarAssembly*>(m_RebarAssembly);
	}
	break;
	default:
		break;
	}

	return ret;
}

RebarSetTag * ACCRebarMethod2_MakerNew::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	if (m_RebarAssembly == NULL)
		return NULL;

	RebarSetTag *tag = NULL;
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->StartUpdate(modelRef);
	if (NULL == rebarSet)
		return NULL;

	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);
		ReCalculateWidthAndPoint();
		CaluateSTWallIntersectPart(m_vecInSecPartPt);
		// 		EditElementHandle eeh;
		// 		LineStringHandler::CreateLineStringElement(eeh, NULL, &m_vecInSecPartPt[0],4, true, *ACTIVEMODEL);
		// 		eeh.AddToModel();
		if (m_vecInSecPartPt.size() != 4)
		{
			return NULL;
		}
		DPoint3d ptCenter = (CPoint3D(m_vecInSecPartPt[0]) + CPoint3D(m_vecInSecPartPt[2])) / 2;
		bool bStart = false;
		if (!EFT::IsPointInLine(ptCenter, m_CurSTwallData.ptStart, m_CurSTwallData.ptEnd, ACTIVEMODEL, bStart))
		{
			return NULL;
		}

		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		vector<PIT::ConcreteRebar> vecRebar;
		bool isHaveAssociated = false;//关联构件钢筋锚入是否已经配置
		BrString strRebarSize;
		if (m_ACCEh.Eh.IsValid())	//取关联构件的钢筋尺寸，若关联构件未配筋，则无需绘制交集处纵向钢筋
		{
			ElementId concreteid = 0;
			GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
			GetElementXAttribute(concreteid, vecRebar, vecRebarDataXAttribute, ACTIVEMODEL);
			if (vecRebar.empty())//关联构件未配置钢筋
			{
				vector<PIT::AssociatedComponent>& vecRebarAC = rebarAssembly->GetvecAC();
				if (vecRebarAC.size())
				{
					auto it = std::find_if(vecRebarAC.begin(), vecRebarAC.end(), [&](AssociatedComponent ac) {return ac.endOffset != 0; });
					if (it != vecRebarAC.end())
					{
						double dim = (*it).endOffset * 0.5;
						strRebarSize.Format(L"%d", (int)dim);
						strRebarSize = strRebarSize + "A";
					}
				}
				else
				{
					strRebarSize = L"32A";	//若没有数据，默认为32mm钢筋
				}
			}
			else
			{
				strRebarSize = XmlManager::s_alltypes[vecRebar.at(0).rebarType];	//关联构件已配筋则取关联构件第一层钢筋直径
			}
		}

		BrString strCurRebarSize = XmlManager::s_alltypes[rebarAssembly->GetvecRebarType().at(0)];	//当前墙主筋	
		double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
		double curDiameter = RebarCode::GetBarDiameter(strCurRebarSize, modelRef);

		double actDiameter = diameter > curDiameter ? diameter : curDiameter;
		BrString strActRebarSize = diameter > curDiameter ? strRebarSize : strCurRebarSize;

		double adjustedXLen, adjustedXSpacing;
		double adjustedYLen, adjustedYSpacing;

		double sideCover = rebarAssembly->GetSideCover()*uor_per_mm;

		int numRebar = 0;
		double spacing = 200 * uor_per_mm;
		adjustedXLen = m_InSecSTwallData.width - sideCover * 2 - curDiameter - diameter - actDiameter;
		int numRebarX = (int)floor(adjustedXLen / spacing + 0.5) + 1;
		adjustedXSpacing = spacing;
		if (numRebarX > 1)
			adjustedXSpacing = adjustedXLen / (numRebarX - 1);

		adjustedYLen = m_CurSTwallData.width - sideCover * 2 - curDiameter * 2 - actDiameter;
		int numRebarY = (int)floor(adjustedYLen / spacing + 0.5) + 1;
		adjustedYSpacing = spacing;
		if (numRebarY > 1)
			adjustedYSpacing = adjustedYLen / (numRebarY - 1);
		numRebar = (numRebarX + numRebarY) * 2 - 4;

		vector<DSegment3d> vecRebarPt;
		DVec3d vec1 = m_vecInSecPartPt[1] - m_vecInSecPartPt[0];
		DVec3d vec2 = m_vecInSecPartPt[2] - m_vecInSecPartPt[1];
		vec1.Normalize();
		vec2.Normalize();
		double dOffset = actDiameter * 0.5 + curDiameter + sideCover;
		DVec3d vecY = vec2;
		vecY.Scale(dOffset);
		DPoint3d ptOrign = m_vecInSecPartPt[0];
		ptOrign.Add(vecY);
		for (int i = 0; i < numRebarX; ++i)
		{
			PITRebarCurve rebarcurve;
			vec1.ScaleToLength(dOffset);
			DPoint3d ptStart = ptOrign;
			ptStart.Add(vec1);
			ptStart.z += sideCover;
			DPoint3d ptEnd = ptStart;
			ptEnd.z += m_CurSTwallData.height;
			ptEnd.z -= sideCover * 2;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
			dOffset += adjustedXSpacing;
		}

		dOffset = adjustedYSpacing;
		for (int i = 0; i < numRebarY - 1; ++i)
		{
			DPoint3d ptStart = vecRebarPt.back().point[0];
			vec2.ScaleToLength(dOffset);
			ptStart.Add(vec2);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
		}

		DVec3d vec3 = m_vecInSecPartPt[3] - m_vecInSecPartPt[2];
		dOffset = adjustedXSpacing;
		vec3.Normalize();
		for (int i = 0; i < numRebarX - 1; ++i)
		{
			DPoint3d ptStart = vecRebarPt.back().point[0];
			vec3.ScaleToLength(dOffset);
			ptStart.Add(vec3);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
		}

		DVec3d vec4 = m_vecInSecPartPt[0] - m_vecInSecPartPt[3];
		dOffset = adjustedYSpacing;
		vec4.Normalize();
		for (int i = 0; i < numRebarY - 2; ++i)
		{
			DPoint3d ptStart = vecRebarPt.back().point[0];
			vec4.ScaleToLength(dOffset);
			ptStart.Add(vec4);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
		}

		if (vecRebarPt.size())
		{
			double bendRadius = RebarCode::GetPinRadius(strCurRebarSize, modelRef, false);
			DVec3d vecA = vec1;
			DVec3d vecB = vec2;
			dOffset = bendRadius - sin(PI / 4) * (bendRadius - actDiameter * 0.5) + curDiameter + sideCover;
			vecA.ScaleToLength(dOffset);
			vecB.ScaleToLength(dOffset);
			DPoint3d ptO(m_vecInSecPartPt[0]);
			ptO.Add(vecA);
			ptO.Add(vecB);
			ptOrign = m_vecInSecPartPt[0];

			ptOrign.Add(vecA);
			ptOrign.Add(vecB);
			double dStartZ = vecRebarPt[0].point[0].z;
			vecRebarPt[0].point[0] = ptOrign;
			vecRebarPt[0].point[0].z = dStartZ;
			double dEndZ = vecRebarPt[0].point[1].z;
			vecRebarPt[0].point[1] = ptOrign;
			vecRebarPt[0].point[1].z = dEndZ;

			// 			EditElementHandle eeh;
			// 			LineHandler::CreateLineElement(eeh, NULL, DSegment3d::From(ptIntersect, ptO), true, *ACTIVEMODEL);
			// 			eeh.AddToModel();
			// 			EditElementHandle eeh1;
			// 			LineHandler::CreateLineElement(eeh1, NULL, DSegment3d::From(ptIntersect, ptOrign), true, *ACTIVEMODEL);
			// 			eeh1.AddToModel();
		}

		vector<PITRebarCurve> vecRebars;
		RebarEndType endTypeStart, endTypeEnd;
		endTypeStart.SetType(RebarEndType::kNone);
		endTypeEnd.SetType(RebarEndType::kNone);
		RebarEndTypes endTypes = { endTypeStart ,endTypeEnd };
		for (size_t i = 0; i < vecRebarPt.size(); ++i)
		{
			DPoint3d pt1[2];
			pt1[0] = vecRebarPt[i].point[0];
			pt1[1] = vecRebarPt[i].point[1];
			PITRebarEndTypes endTypes;
			endTypes.beg.SetType(PITRebarEndType::kNone);
			endTypes.end.SetType(PITRebarEndType::kNone);
			makeRebarCurve(vecRebarPt[i].point[0], vecRebarPt[i].point[1], vecRebars, endTypes, sideCover);
			//PITRebarCurve rebarcurve;
			//PITRebarEndTypes endTypes;
			//RebarVertexP vex;
			//vex = &rebarcurve.PopVertices().NewElement();
			//vex->SetIP(vecRebarPt[i].point[0]);
			//vex->SetType(RebarVertex::kStart);

			//vex = &rebarcurve.PopVertices().NewElement();
			//vex->SetIP(vecRebarPt[i].point[1]);
			////			vex->SetType(RebarVertex::kEnd);

			//endTypes.beg.SetType(PITRebarEndType::kNone);
			//endTypes.end.SetType(PITRebarEndType::kNone);

			//rebarcurve.EvaluateEndTypes(endTypes);
			//vecRebars.push_back(rebarcurve);
		}

		//预览按钮按下，则画线
		if (PreviewButtonDown)
		{
			//绘制关联处钢筋，并将线段存入m_allLines
			for (auto it = vecRebarPt.begin(); it != vecRebarPt.end(); it++)
			{
				DSegment3d Dsegtemp(*it);
				DPoint3d strPoint = DPoint3d::From(Dsegtemp.point[0].x, Dsegtemp.point[0].y, Dsegtemp.point[0].z);
				DPoint3d endPoint = DPoint3d::From(Dsegtemp.point[1].x, Dsegtemp.point[1].y, Dsegtemp.point[1].z);
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_RebarAssembly->m_allLines.push_back(eeh.GetElementRef());
			}
		}

		RebarSymbology symb;
		string str(strActRebarSize);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);

		for (size_t i = 0; i < vecRebars.size(); ++i)
		{
			RebarElementP rebarElement = NULL;
			if (!PreviewButtonDown)//预览标志
			{
				rebarElement = rebarSet->AssignRebarElement((int)i, (int)vecRebars.size(), symb, modelRef);
			}
			PITRebarCurve rebarCurve = vecRebars[i];
			if (NULL != rebarElement)
			{
				RebarShapeData shape;
				shape.SetSizeKey((LPCTSTR)strActRebarSize);
				shape.SetIsStirrup(false);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				rebarElement->Update(rebarCurve, actDiameter, endTypes, shape, modelRef, false);
			}

			// 添加钢筋层号和正反面
			CPoint3D ptstr, ptend;
			rebarCurve.GetEndPoints(ptstr, ptend);
			ptstr.z = m_CurSTwallData.ptEnd.z;
			DPoint3d ptTmp = ptstr;
			double dDistance = sideCover + diameter;
			AddRebarLevelAndType(rebarElement, strCurRebarSize, ptTmp, m_CurSTwallData.ptStart, m_CurSTwallData.ptEnd, dDistance);
		}

		RebarSetData setdata;
		setdata.SetNumber((int)vecRebars.size());
		setdata.SetNominalSpacing(spacing);
		setdata.SetAverageSpacing(adjustedXSpacing);

		rebarSet->FinishUpdate(setdata, modelRef);
	}
	break;
	case ACCRebarAssemblyNew::ComponentType::GWALL:
	{
		//		GWallRebarAssembly *rebarAssembly = dynamic_cast<GWallRebarAssembly*>(m_RebarAssembly);
	}
	break;
	default:
		break;
	}

	tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);

	return tag;
}

RebarSetP ACCRebarMethod3_MakerNew::MakeURebarCurve(ElementId rebarSetId, const vector<vector<DSegment3d> >& vvecStartEnd,double width, DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double tmpSidecover = m_RebarAssembly->GetSideCover();
	//关联构件的混凝土保护层
	ACCConcrete ACConcrete;
	int retGetACC = GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ACCConcrete), ACConcrete, ConcreteCoverXAttribute, m_ACCEh.Eh.GetModelRef());
	if (retGetACC != SUCCESS)
	{
		ACConcrete = { 50,50,50 };
	}
	if (m_isFrCoverside)
	{
		m_RebarAssembly->SetSideCover(ACConcrete.postiveOrTopCover);
	}
	else
	{
		m_RebarAssembly->SetSideCover(ACConcrete.reverseOrBottomCover);
	}

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->StartUpdate(modelRef);
	if (NULL == rebarSet)
	{
		m_RebarAssembly->SetSideCover(tmpSidecover);
		return NULL;
	}
	
	
	BrString rebarSize = XmlManager::s_alltypes[m_RebarAssembly->GetvecRebarType().at(0)];
	double diameter = RebarCode::GetBarDiameter(rebarSize, modelRef);
	double bendRadius = RebarCode::GetPinRadius(rebarSize, modelRef,false);
	
	double Ulenth = 50 * uor_per_mm + g_globalpara.m_alength[(string)rebarSize]*uor_per_mm;
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(m_RebarAssembly->GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };

	std::vector<PITRebarCurve> vecRebarCurve;
	const vector<DSegment3d> &vecPro = vvecStartEnd[0];
	const vector<DSegment3d> &vecReserve = vvecStartEnd[1];

	if (vecPro.size() != vecReserve.size())
	{
		m_RebarAssembly->SetSideCover(tmpSidecover);
		return NULL;
	}
	EditElementHandle solideeh(m_ACCEh.Eh, m_ACCEh.Eh.GetModelRef());
	EditElementHandle souceeeh(m_eh, m_eh.GetDgnModelP());
	vector<EditElementHandle*> vecSolids;
	vecSolids.push_back(&solideeh);
	vector<EditElementHandle*> tmps;
	tmps.push_back(&solideeh);
	tmps.push_back(&souceeeh);
	m_RebarExtendvec.Negate();
	ACCSTWallRebarAssemblyNew *rebarAssembly = nullptr;
	switch (m_RebarAssembly->GetcpType())
	{
	  case ACCRebarAssemblyNew::ComponentType::STWALL:
	  {
		rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);
	   }
	}
	if (rebarAssembly==nullptr)
	{
		m_RebarAssembly->SetSideCover(tmpSidecover);
		return rebarSet;
	}
	vector<vector<CPoint3D>>vct_vecUpt;//装多个U型钢筋的点
	for (size_t i = 0; i < vecPro.size(); ++i)
	{
		DPoint3d ptStart, ptEnd;
		if (m_Relation & 0x01)
		{
			ptStart = vecPro[i].point[1];
			ptEnd = vecReserve[i].point[1];
		}
		else
		{
			ptStart = vecPro[i].point[0];
			ptEnd = vecReserve[i].point[0];
		}
		{//因为起点和终点为截断了sidecover保护层长度了，将此长度加回来
			DPoint3d vec1 = m_RebarExtendvec;
			vec1.Normalize();
			vec1.Scale(tmpSidecover* uor_per_mm );
			ptStart.Add(vec1);
			ptEnd.Add(vec1);
		}
		DPoint3d vecZ = CVector3D::kZaxis;
		vecZ.Negate();
		vecZ.Scale(diameter);
		ptStart.Add(vecZ);
		ptEnd.Add(vecZ);

		vector<CPoint3D> vecUpt(4);	//U形钢筋的4个点
		//通过端点计算U形钢筋的4个点
		//第一个点
		vecUpt[0] = ptStart;
		DPoint3d vec1 = m_RebarExtendvec;
		vec1.Normalize();
		vec1.Negate();
		vec1.ScaleToLength(Ulenth);
		vecUpt[0].Add(vec1);

		//第二个点
		vecUpt[1] = ptStart;
		vecUpt[1].Add(m_RebarExtendvec);
		vec1.Normalize();
		vec1.Scale(diameter*0.5 + m_RebarAssembly->GetSideCover() * uor_per_mm);//因为弯曲所以点要反方向偏移一个钢筋的半径和保护层
		vecUpt[1].Add(vec1);

		//第三个点
		double dPositiveCover = m_RebarAssembly->GetPositiveCover();
		double dReverseCover = m_RebarAssembly->GetReverseCover();
		DPoint3d vec3 = ptEnd - ptStart;
		vec3.Normalize();
		vec3.ScaleToLength(width - dPositiveCover * uor_per_mm - dReverseCover * uor_per_mm - diameter);
		vecUpt[2] = vecUpt[1];
		vecUpt[2].Add(vec3);

		//第四个点
		vecUpt[3] = vecUpt[0];
		vecUpt[3].Add(vec3);

		if (ISPointInHoles(vecSolids,vecUpt[1])&&ISPointInHoles(vecSolids,vecUpt[2]))
		{
			CalculateIntersetPtWithHolesWithRebarCuve(vecUpt[0], vecUpt[1], vecUpt[0], rebarAssembly->m_useHoleehs);
			CalculateIntersetPtWithHolesWithRebarCuve(vecUpt[2], vecUpt[3], vecUpt[2], rebarAssembly->m_useHoleehs);
			
		}
		
		if (!ISPointInHoles(tmps, vecUpt[1]) && !ISPointInHoles(tmps, vecUpt[2]))//过滤掉不在实体范围内的U形钢筋
		{
			continue;
		}
		PITRebarCurve rebarcurve;
		rebarcurve.makeURebarCurve(vecUpt, bendRadius);
		vecRebarCurve.push_back(rebarcurve);
		vct_vecUpt.push_back(vecUpt);
	}

	int dURebarNumber = vecRebarCurve.size(); // U型筋数量
	m_RebarAssembly->SetSideCover(tmpSidecover);
	MakeIntersectionRebarCurve(vecRebarCurve, modelRef);

	//根据vct_vecUpt来画出预览的U型钢筋线
	if (PreviewButtonDown)
	{
		for (int x = 0; x < vct_vecUpt.size(); x++)
		{
			auto vctemp = vct_vecUpt[x];
			for (int y = 0; y < vctemp.size() - 1; y++)
			{
				DPoint3d strPoint = DPoint3d::From(vctemp[y].x, vctemp[y].y, vctemp[y].z);
				DPoint3d endPoint = DPoint3d::From(vctemp[y + 1].x, vctemp[y + 1].y, vctemp[y + 1].z);
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_RebarAssembly->m_allLines.push_back(eeh.GetElementRef());
			}
		}
	}


	RebarSymbology symb;
	string str(rebarSize);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);

	for (size_t i = 0; i < vecRebarCurve.size(); ++i)
	{
		RebarElementP rebarElement = NULL;
		if (!PreviewButtonDown)//预览标志，预览状态下不生成钢筋
		{
			rebarElement = rebarSet->AssignRebarElement((int)i, (int)vecRebarCurve.size(), symb, modelRef);
		}		
		RebarCurve rebarCurve = vecRebarCurve[i];
		if (NULL != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)rebarSize);
			shape.SetIsStirrup(false);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
		}

		if (i < dURebarNumber) // U型筋 正面第一层
		{
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			int grade = 0;
			BrString rebarSizeTmp = rebarSize.Right(1);
			if (rebarSizeTmp == "A")
			{
				grade = 0;
			}
			else if (rebarSizeTmp == "B")
			{
				grade = 1;
			}
			else if (rebarSizeTmp == "C")
			{
				grade = 2;
			}
			else if (rebarSizeTmp == "D")
			{
				grade = 3;
			}
		
			string slevel = "0";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, "front", ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		else // 点筋
		{
			// 添加钢筋层号和正反面
			CPoint3D ptstr, ptend;
			rebarCurve.GetEndPoints(ptstr, ptend);
			ptstr.z = m_CurSTwallData.ptEnd.z;
			DPoint3d ptTmp = ptstr;
			double actDiameter = RebarCode::GetBarDiameter(rebarSize, modelRef);
			double dDistance = m_RebarAssembly->GetSideCover() * uor_per_mm + actDiameter;
			AddRebarLevelAndType(rebarElement, rebarSize, ptTmp, m_CurSTwallData.ptStart, m_CurSTwallData.ptEnd, dDistance);
		}
	}

	RebarSetData setdata;
	setdata.SetNumber((int)vecRebarCurve.size());
	setdata.SetNominalSpacing(200);
	setdata.SetAverageSpacing(200);

	rebarSet->FinishUpdate(setdata, modelRef);
	
	return rebarSet;
}

bool ACCRebarMethod3_MakerNew::MakeIntersectionRebarCurve(std::vector<PITRebarCurve>& vecRebarCurve, DgnModelRefP modelRef)
{
	if (m_RebarAssembly == NULL)
		return false;

	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);
		ReCalculateWidthAndPoint();
		CaluateSTWallIntersectPart(m_vecInSecPartPt);
		if (m_vecInSecPartPt.size() != 4)
		{
			return NULL;
		}
		DPoint3d ptCenter = (CPoint3D(m_vecInSecPartPt[0]) + CPoint3D(m_vecInSecPartPt[2])) / 2;
		bool bStart = false;
		if (!EFT::IsPointInLine(ptCenter, m_CurSTwallData.ptStart, m_CurSTwallData.ptEnd, ACTIVEMODEL, bStart))
		{
			return NULL;
		}

		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		vector<PIT::ConcreteRebar> vecRebar;
		bool isHaveAssociated = false;//关联构件钢筋锚入是否已经配置
		BrString strRebarSize;
		if (m_ACCEh.Eh.IsValid())	//取关联构件的钢筋尺寸，若关联构件未配筋，则无需绘制交集处纵向钢筋
		{
			ElementId concreteid = 0;
			GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
			GetElementXAttribute(concreteid, vecRebar, vecRebarDataXAttribute, ACTIVEMODEL);
			if (vecRebar.empty())//关联构件未配置钢筋
			{
				vector<PIT::AssociatedComponent>& vecRebarAC = rebarAssembly->GetvecAC();
				if (vecRebarAC.size())
				{
					auto it = std::find_if(vecRebarAC.begin(), vecRebarAC.end(), [&](AssociatedComponent ac) {return ac.endOffset != 0; });
					if (it != vecRebarAC.end())
					{
						double dim = (*it).endOffset * 0.5;
						strRebarSize.Format(L"%d", (int)dim);
						strRebarSize = strRebarSize + "A";
					}
				}
				else
				{
					strRebarSize = L"32A";	//若没有数据，默认为32mm钢筋
				}
			}
			else
			{
				strRebarSize = XmlManager::s_alltypes[vecRebar.at(0).rebarType];	//关联构件已配筋则取关联构件第一层钢筋直径
			}
		}

		BrString strCurRebarSize = XmlManager::s_alltypes[rebarAssembly->GetvecRebarType().at(0)];	//当前墙主筋
		double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
		double curDiameter = RebarCode::GetBarDiameter(strCurRebarSize, modelRef);

		double actDiameter = diameter > curDiameter ? diameter : curDiameter;
		BrString strActRebarSize = diameter > curDiameter ? strRebarSize : strCurRebarSize;

		double adjustedXLen, adjustedXSpacing;
		double adjustedYLen, adjustedYSpacing;

		double sideCover = rebarAssembly->GetSideCover()*uor_per_mm;

		int numRebar = 0;
		double spacing = 200 * uor_per_mm;
		adjustedXLen = m_InSecSTwallData.width - sideCover * 2 - actDiameter * 2 - diameter;
		int numRebarX = (int)floor(adjustedXLen / spacing + 0.5) + 1;
		adjustedXSpacing = spacing;
		if (numRebarX > 1)
			adjustedXSpacing = adjustedXLen / (numRebarX - 1);

		adjustedYLen = m_CurSTwallData.width - sideCover * 2 - curDiameter * 2 - actDiameter;
		int numRebarY = (int)floor(adjustedYLen / spacing + 0.5) + 1;
		adjustedYSpacing = spacing;
		if (numRebarY > 1)
			adjustedYSpacing = adjustedYLen / (numRebarY - 1);
		numRebar = (numRebarX + numRebarY) * 2 - 4;

		vector<DSegment3d> vecRebarPt;
		DVec3d vec1 = m_vecInSecPartPt[1] - m_vecInSecPartPt[0];
		DVec3d vec2 = m_vecInSecPartPt[2] - m_vecInSecPartPt[1];
		DVec3d vec3 = m_vecInSecPartPt[3] - m_vecInSecPartPt[2];
		DVec3d vec4 = m_vecInSecPartPt[0] - m_vecInSecPartPt[3];
		vec1.Normalize();
		vec2.Normalize();
		vec3.Normalize();
		vec4.Normalize();
		double dOffset = actDiameter * 1.5 + sideCover;
		DVec3d vecY = vec2;
		vecY.Scale(dOffset);
		DPoint3d ptOrign = m_vecInSecPartPt[0];
		ptOrign.Add(vecY);
		for (int i = 0; i < numRebarX; ++i)
		{
			PITRebarCurve rebarcurve;
			vec1.ScaleToLength(dOffset);
			DPoint3d ptStart = ptOrign;
			ptStart.Add(vec1);
			ptStart.z += sideCover;
			DPoint3d ptEnd = ptStart;
			ptEnd.z += m_CurSTwallData.height;
			ptEnd.z -= sideCover * 2;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
			dOffset += adjustedXSpacing;
		}

		DPoint3d ptEndRebarStart1 = vecRebarPt.back().point[0];//偏移之前的当前行最后一根钢筋的起点
		if (vecRebarPt.size())
		{
			double bendRadius = RebarCode::GetPinRadius(strActRebarSize, modelRef, false);
			dOffset = bendRadius - sin(PI / 4) * (bendRadius - actDiameter * 0.5) + actDiameter + sideCover;
			DVec3d vecA = vec1;
			DVec3d vecB = vec2;
			vecA.ScaleToLength(dOffset);
			vecB.ScaleToLength(dOffset);
			DPoint3d ptO(m_vecInSecPartPt[0]);
			ptO.Add(vecA);
			ptO.Add(vecB);
			ptOrign = m_vecInSecPartPt[0];

			ptOrign.Add(vecA);
			ptOrign.Add(vecB);
			double dStartZ = vecRebarPt[0].point[0].z;
			vecRebarPt[0].point[0] = ptOrign;
			vecRebarPt[0].point[0].z = dStartZ;
			double dEndZ = vecRebarPt[0].point[1].z;
			vecRebarPt[0].point[1] = ptOrign;
			vecRebarPt[0].point[1].z = dEndZ;

			if ((int)curDiameter <= (int)diameter)
			{
				bendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);
				dOffset = bendRadius - sin(PI / 4) * (bendRadius - actDiameter * 0.5) + diameter + sideCover;

				vecA = vec2;
				vecB = vec3;
				vecA.ScaleToLength(dOffset);
				vecB.ScaleToLength(dOffset);
				ptO = m_vecInSecPartPt[1];
				ptO.Add(vecA);
				ptO.Add(vecB);
				ptOrign = m_vecInSecPartPt[1];

				ptOrign.Add(vecA);
				ptOrign.Add(vecB);
				dStartZ = vecRebarPt.back().point[0].z;
				vecRebarPt.back().point[0] = ptOrign;
				vecRebarPt.back().point[0].z = dStartZ;
				dEndZ = vecRebarPt.back().point[1].z;
				vecRebarPt.back().point[1] = ptOrign;
				vecRebarPt.back().point[1].z = dEndZ;
			}
		}

		dOffset = adjustedYSpacing;
		for (int i = 0; i < numRebarY - 1; ++i)
		{
			DPoint3d ptStart = ptEndRebarStart1;
			vec2.ScaleToLength(dOffset);
			ptStart.Add(vec2);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
			dOffset += adjustedYSpacing;
		}

		dOffset = adjustedXSpacing;
		for (int i = 0; i < numRebarX - 1; ++i)
		{
			DPoint3d ptStart = vecRebarPt.back().point[0];
			vec3.ScaleToLength(dOffset);
			ptStart.Add(vec3);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
		}

		DPoint3d ptEndRebarStart2 = vecRebarPt.back().point[0];//偏移之前的当前行最后一根钢筋的起点
		if (vecRebarPt.size())
		{
			double bendRadius = RebarCode::GetPinRadius(strActRebarSize, modelRef, false);
			dOffset = bendRadius - sin(PI / 4) * (bendRadius - actDiameter * 0.5) + actDiameter + sideCover;

			DVec3d vecA = vec4;
			DVec3d vecB = vec1;
			vecA.ScaleToLength(dOffset);
			vecB.ScaleToLength(dOffset);
			Dpoint3d ptO = m_vecInSecPartPt[3];
			ptO.Add(vecA);
			ptO.Add(vecB);
			ptOrign = m_vecInSecPartPt[3];

			ptOrign.Add(vecA);
			ptOrign.Add(vecB);
			double dStartZ = vecRebarPt.back().point[0].z;
			vecRebarPt.back().point[0] = ptOrign;
			vecRebarPt.back().point[0].z = dStartZ;
			double dEndZ = vecRebarPt.back().point[1].z;
			vecRebarPt.back().point[1] = ptOrign;
			vecRebarPt.back().point[1].z = dEndZ;
		}
		dOffset = adjustedYSpacing;
		for (int i = 0; i < numRebarY - 2; ++i)
		{
			DPoint3d ptStart = ptEndRebarStart2;
			vec4.ScaleToLength(dOffset);
			ptStart.Add(vec4);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
			dOffset += adjustedYSpacing;
		}

		RebarEndType endTypeStart, endTypeEnd;
		endTypeStart.SetType(RebarEndType::kNone);
		endTypeEnd.SetType(RebarEndType::kNone);
		RebarEndTypes endTypes = { endTypeStart ,endTypeEnd };
		for (size_t i = 0; i < vecRebarPt.size(); ++i)
		{
			PITRebarCurve rebarcurve;
			PITRebarEndTypes endTypes;
			RebarVertexP vex;
			vex = &rebarcurve.PopVertices().NewElement();
			vex->SetIP(vecRebarPt[i].point[0]);
			vex->SetType(RebarVertex::kStart);

			vex = &rebarcurve.PopVertices().NewElement();
			vex->SetIP(vecRebarPt[i].point[1]);

			endTypes.beg.SetType(PITRebarEndType::kNone);
			endTypes.end.SetType(PITRebarEndType::kNone);

			rebarcurve.EvaluateEndTypes(endTypes);
			vecRebarCurve.push_back(rebarcurve);

			if (PreviewButtonDown)//预览的状态下需要画线
			{
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, NULL, vecRebarPt[i], true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_RebarAssembly->m_allLines.push_back(eeh.GetElementRef());
			}

		}
	}
	break;
	case ACCRebarAssemblyNew::ComponentType::GWALL:
	{
		//		GWallRebarAssembly *rebarAssembly = dynamic_cast<GWallRebarAssembly*>(m_RebarAssembly);
	}
	break;
	default:
		break;
	}

	return false;

}

bool ACCRebarMethod3_MakerNew::MakeRebars(DgnModelRefP modelRef)
{
	if (m_RebarAssembly == NULL)
		return NULL;
	bool ret = false;
	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);

// 		STWallGeometryInfo m_CurSTwallData;
// 		AnalyzingSTWallGeometricData(m_pACC->GetElementHandle(), m_CurSTwallData);
// 
// 		STWallGeometryInfo m_InSecSTwallData;
// 		AnalyzingSTWallGeometricData(m_ACCEh.Eh, m_InSecSTwallData);

		const vector<PIT::AssociatedComponent>& vecAC = rebarAssembly->PopvecAC();
		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		double dOffset;
		string strName = m_ACCEh.EleName;
		auto it = std::find_if(vecAC.begin(), vecAC.end(), [&strName](AssociatedComponent tmphd) {return string(tmphd.associatedComponentName) == strName; });
		if (it != vecAC.end())
		{
			dOffset = (*it).endOffset;
			m_L0 = (*it).startL0;
		}

		int level = 0, endTypeIndex = 0, strTypeIndex = 1;
		double angle = 0;
		vector<vector<PIT::EndType> > &vvEndType = rebarAssembly->PopvvecEndType();
		m_Relation = ACCSTWallRebarAssemblyNew::JudgeComponentRelation(m_CurSTwallData, m_InSecSTwallData);
		endTypeIndex = m_Relation & 0x01;
		if (m_Relation == 0x111 || m_Relation == 0x110 || m_Relation == 0x001 || m_Relation == 0x000)
			level = (int)vvEndType.size() - 1;

		// 		if (relation == 0x000 || relation == 0x001 || relation == 0x111)
		// 			angle = 90;
		if (m_Relation & 0x01)		//关联构件在当前构件的终点
		{
			//判断点是否在线上
			bool bNearStart = true;
			m_RebarExtendvec = m_CurSTwallData.ptStart - m_CurSTwallData.ptEnd;
			m_RebarExtendvec.Normalize();
			m_RebarExtendvec.Scale(m_InSecSTwallData.width);
			DPoint3d ptPro2;
			//当前构件中心线起点到关联构件终点线段的投影点
			mdlVec_projectPointToLine(&ptPro2, NULL, &m_CurSTwallData.ptEnd, &m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptEnd);
			if (!EFT::IsPointInLine(ptPro2, m_InSecSTwallData.ptStart, m_InSecSTwallData.ptEnd, modelRef, bNearStart))
			{
				
				//不在线上
				rebarAssembly->PopSTwallData().ptEnd.Add(m_RebarExtendvec);
				rebarAssembly->PopSTwallData().length -= m_InSecSTwallData.width;
				if (rebarAssembly->m_vecCutWallData.size() > 0)
				{
					rebarAssembly->m_vecCutWallData.back().ptEnd.Add(m_RebarExtendvec);
					rebarAssembly->m_vecCutWallData.back().length -= m_InSecSTwallData.width;
				}
			}
		}
		else
		{
			//判断点是否在线上
			DPoint3d ptPro1;
			//当前构件中心线起点到关联构件终点线段的投影点
			mdlVec_projectPointToLine(&ptPro1, NULL, &m_CurSTwallData.ptStart, &m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptEnd);
			bool bNearStart = true;

			m_RebarExtendvec = m_CurSTwallData.ptEnd - m_CurSTwallData.ptStart;
			m_RebarExtendvec.Normalize();
			m_RebarExtendvec.Scale(m_InSecSTwallData.width);
			if (!EFT::IsPointInLine(ptPro1, m_InSecSTwallData.ptStart, m_InSecSTwallData.ptEnd, modelRef, bNearStart))
			{
				//在线上
				rebarAssembly->PopSTwallData().ptStart.Add(m_RebarExtendvec);
				rebarAssembly->PopSTwallData().length -= m_InSecSTwallData.width;
				if (rebarAssembly->m_vecCutWallData.size() > 0)
				{
					rebarAssembly->m_vecCutWallData.front().ptStart.Add(m_RebarExtendvec);
					rebarAssembly->m_vecCutWallData.front().length -= m_InSecSTwallData.width;
				}
			}
		}
		//终点偏移
		if (vvEndType.size())
		{
			vector<PIT::ConcreteRebar> vecRebarData;
			rebarAssembly->GetRebarData(vecRebarData);
			//取当前构件的混凝土保护层
			ACCConcrete ACconcrete;
			if (GetCurElementHandle().IsValid())
			{
				int retGetACC = GetElementXAttribute(GetCurElementHandle().GetElementId(), sizeof(ACCConcrete), ACconcrete, ConcreteCoverXAttribute, m_ACCEh.Eh.GetModelRef());
				if (retGetACC != SUCCESS)
				{
					ACconcrete = { 50,50,50 };
				}
			}

			vvEndType[level][endTypeIndex].offset = dOffset;//当前构件端点偏移
			for (size_t i = 0; i < vecRebarData.size(); ++i)
			{
				if (vecRebarData[i].rebarDir == 0)	//所有横向钢筋都弯曲
				{
					vvEndType[i][endTypeIndex].rotateAngle = angle;	//端部弯钩方向旋转
					vvEndType[i][endTypeIndex].endType = 0;			//都不进行弯曲
					//vvEndType[i][endTypeIndex].offset = /*m_CurSTwallData.width / uor_per_mm*/ - ACconcrete.sideCover;	//进行端点偏移缩回关联构件的宽度;
					//vvEndType[i][strTypeIndex].offset = /*m_CurSTwallData.width / uor_per_mm*/ -ACconcrete.sideCover;
					vvEndType[i][endTypeIndex].offset = 0;
				}
// 				else
// 				{
// 					double dRadius = RebarCode::GetPinRadius(vecRebarData[i].rebarSize, modelRef, false);
// 					dRadius /= uor_per_mm;
// 					if (0 == endTypeIndex)
// 						rebarAssembly->PopvecStartOffset().at(i) += dRadius;
// 					else
// 						rebarAssembly->PopvecEndOffset().at(i) += dRadius;
// 				}
			}
		}
	}
	break;
	case ACCRebarAssemblyNew::ComponentType::GWALL:
	{
		//		GWallRebarAssembly *rebarAssembly = dynamic_cast<GWallRebarAssembly*>(m_RebarAssembly);
	}
	break;
	default:
		break;
	}

	return ret;
}

RebarSetTag * ACCRebarMethod3_MakerNew::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return NULL;

	//绘制U形钢筋
	RebarSetTag* tag = NULL;

	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);
		vector<vector<DSegment3d> > vvecStartEnd;
		vvecStartEnd.push_back(rebarAssembly->m_vecRebarStartEnd[0]);
		vvecStartEnd.push_back(rebarAssembly->m_vecRebarStartEnd.back());

		RebarSetP rebarSet = MakeURebarCurve(rebarSetId, vvecStartEnd, rebarAssembly->GetSTwallData().width, modelRef);
		tag = new RebarSetTag();
		tag->SetRset(rebarSet);
		tag->SetIsStirrup(false);
	}
	break;
	default:
		break;
	}

	return tag;
}

bool ACCRebarMethod4_MakerNew::MakeRebars(DgnModelRefP modelRef)
{
	if (m_RebarAssembly == NULL)
		return NULL;
	bool ret = false;
	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);

// 		STWallGeometryInfo m_CurSTwallData;
// 		AnalyzingSTWallGeometricData(m_pACC->GetElementHandle(), m_CurSTwallData);
// 
// 		STWallGeometryInfo m_InSecSTwallData;
// 		AnalyzingSTWallGeometricData(m_ACCEh.Eh, m_InSecSTwallData);

		const vector<PIT::AssociatedComponent>& vecAC = rebarAssembly->PopvecAC();
		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		double dOffset;
		string strName = m_ACCEh.EleName;
		auto it = std::find_if(vecAC.begin(), vecAC.end(), [&strName](const AssociatedComponent &tmphd) {return string(tmphd.associatedComponentName) == strName; });
		if (it != vecAC.end())
			dOffset = (*it).endOffset;

		int level = 0, endTypeIndex = 0;
		double angle = -90;
		vector<vector<PIT::EndType> > &vvEndType = rebarAssembly->PopvvecEndType();
		UInt32 relation = ACCSTWallRebarAssemblyNew::JudgeComponentRelation(m_CurSTwallData, m_InSecSTwallData);
		endTypeIndex = relation & 0x01;

// 		if (relation & 0x01)		//关联构件在当前构件的终点
// 		{
// 			//判断点是否在线上
// 			bool bNearStart = true;
// 			DPoint3d ptPro2;
// 			//当前构件中心线起点到关联构件终点线段的投影点
// 			mdlVec_projectPointToLine(&ptPro2, NULL, &m_CurSTwallData.ptEnd, &m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptEnd);
// 			if (EFT::IsPointInLine(ptPro2, m_InSecSTwallData.ptStart, m_InSecSTwallData.ptEnd, modelRef, bNearStart))
// 			{
// 				//在线上
// 				DPoint3d vec = m_CurSTwallData.ptEnd - m_CurSTwallData.ptStart;
// 				vec.Normalize();
// 				vec.Scale(m_InSecSTwallData.width);
// 				rebarAssembly->PopSTwallData().ptEnd.Add(vec);
// 				rebarAssembly->PopSTwallData().length += m_InSecSTwallData.width;
// 			}
// 		}
// 		else
// 		{
// 			//判断点是否在线上
// 			DPoint3d ptPro1;
// 			//当前构件中心线起点到关联构件终点线段的投影点
// 			mdlVec_projectPointToLine(&ptPro1, NULL, &m_CurSTwallData.ptStart, &m_InSecSTwallData.ptStart, &m_InSecSTwallData.ptEnd);
// 			bool bNearStart = true;
// 			if (EFT::IsPointInLine(ptPro1, m_InSecSTwallData.ptStart, m_InSecSTwallData.ptEnd, modelRef, bNearStart))
// 			{
// 				//在线上
// 				DPoint3d vec = m_CurSTwallData.ptStart - m_CurSTwallData.ptEnd;
// 				vec.Normalize();
// 				vec.Scale(m_InSecSTwallData.width);
// 				rebarAssembly->PopSTwallData().ptStart.Add(vec);
// 				rebarAssembly->PopSTwallData().length += m_InSecSTwallData.width;
// 			}
// 		}
		//终点偏移
		if (vvEndType.size())
		{
			vector<PIT::ConcreteRebar> vecRebarData;
			rebarAssembly->GetRebarData(vecRebarData);
			//关联构件的混凝土保护层
			ACCConcrete ACConcrete;
			int retGetACC = GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ACCConcrete), ACConcrete, ConcreteCoverXAttribute, m_ACCEh.Eh.GetModelRef());
			if (retGetACC != SUCCESS)
			{
				ACConcrete = { 50,50,50 };
			}
			if (m_isFrCoverside)
			{
				dOffset += ACConcrete.postiveOrTopCover /*+ ACConcrete.sideCover*/ - g_wallRebarInfo.concrete.sideCover;
			}
			else
			{
				dOffset += ACConcrete.reverseOrBottomCover /*+ ACConcrete.sideCover*/ - g_wallRebarInfo.concrete.sideCover;
			}
			for (size_t i = 0; i < vecRebarData.size(); ++i)
			{
				if (vecRebarData[i].rebarDir == 0)	//所有横向钢筋都弯曲
				{

					vvEndType[i][endTypeIndex].offset = dOffset - m_InSecSTwallData.width/uor_per_mm;	//当前构件端点偏移
					if (i < vecRebarData.size()/2)
					{
						vvEndType[i][endTypeIndex].rotateAngle = angle;	//端部弯钩方向旋转
					}
					else
					{
						vvEndType[i][endTypeIndex].rotateAngle = -angle;	//端部弯钩方向反向
					}
					vvEndType[i][endTypeIndex].endType = 4;			//端部弯曲方式 90度弯曲 与上边变量一致

					BrString sizekey = XmlManager::s_alltypes[vecRebarData[i].rebarType];

					double dRadius = RebarCode::GetPinRadius(sizekey, modelRef, false);
					double dim = RebarCode::GetBarDiameter(sizekey, modelRef);
					//vvEndType[i][endTypeIndex].endPtInfo.value3 = m_CurSTwallData.width / uor_per_mm - curConcrete.concrete.sideCover - dRadius / uor_per_mm - dim / 2 / uor_per_mm;//预留长度
					//vvEndType[i][endTypeIndex].endPtInfo.value3 = g_globalpara.m_laplenth[atoi(vecRebarData[i].rebarSize)] * uor_per_mm;

					double LaLenth = g_globalpara.m_alength[(string)sizekey] * uor_per_mm;
					double disLenth = m_InSecSTwallData.width - dOffset - dRadius + PI/2*(dRadius - dim/2);
					if (LaLenth - disLenth < 12 * dim)
					{
						vvEndType[i][endTypeIndex].endPtInfo.value3 = 12 * dim;
					}
					else
					{
						vvEndType[i][endTypeIndex].endPtInfo.value3 = LaLenth - disLenth;
					}



				}
// 				else
// 				{
// 					double dRadius = RebarCode::GetPinRadius(vecRebarData[i].rebarSize, modelRef, false);
// 					dRadius /= uor_per_mm;
// 					if (0 == endTypeIndex)
// 						rebarAssembly->PopvecStartOffset().at(i) += dRadius;
// 					else
// 						rebarAssembly->PopvecEndOffset().at(i) += dRadius;
// 				}
 			}
		}
	}
	break;
	case ACCRebarAssemblyNew::ComponentType::GWALL:
	{
		//		GWallRebarAssembly *rebarAssembly = dynamic_cast<GWallRebarAssembly*>(m_RebarAssembly);
	}
	break;
	default:
		break;
	}

	return ret;
}

RebarSetTag * ACCRebarMethod4_MakerNew::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	if (m_RebarAssembly == NULL)
		return NULL;

	RebarSetTag *tag = NULL;
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->StartUpdate(modelRef);
	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);
		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		ReCalculateWidthAndPointForL4();
		CaluateSTWallIntersectPart(m_vecInSecPartPt);
		/*EditElementHandle eeh;
		LineStringHandler::CreateLineStringElement(eeh, NULL, &m_vecInSecPartPt[0], 4, true, *ACTIVEMODEL);
		eeh.AddToModel();*/
		if (m_vecInSecPartPt.size() != 4)
		{
			return NULL;
		}
		DPoint3d ptCenter = (CPoint3D(m_vecInSecPartPt[0]) + CPoint3D(m_vecInSecPartPt[2])) / 2;
		bool bStart = false;
		DPoint3d cenptPro;
		//当前构件中心线起点到关联构件终点线段的投影点
		/*mdlVec_projectPointToLine(&cenptPro, NULL, &ptCenter, &m_CurSTwallData.ptStart, &m_CurSTwallData.ptEnd);
		if (!EFT::IsPointInLine(cenptPro, m_CurSTwallData.ptStart, m_CurSTwallData.ptEnd, ACTIVEMODEL, bStart))
		{
			return NULL;
		}*/

		vector<PIT::ConcreteRebar> vecRebar;
		bool isHaveAssociated = false;//关联构件钢筋锚入是否已经配置
		BrString strRebarSize;
		if (m_ACCEh.Eh.IsValid())	//取关联构件的钢筋尺寸，若关联构件未配筋，则无需绘制交集处纵向钢筋
		{
			ElementId concreteid = 0;
			GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
			GetElementXAttribute(concreteid, vecRebar, vecRebarDataXAttribute, ACTIVEMODEL);
			if (vecRebar.empty())//关联构件未配置钢筋
			{
				vector<PIT::AssociatedComponent>& vecRebarAC = rebarAssembly->GetvecAC();
				if (vecRebarAC.size())
				{
					auto it = std::find_if(vecRebarAC.begin(), vecRebarAC.end(), [&](AssociatedComponent ac) {return ac.endOffset != 0; });
					if (it != vecRebarAC.end())
					{
						double dim = (*it).endOffset * 0.5;
						strRebarSize.Format(L"%d", (int)dim);
						strRebarSize = strRebarSize + "A";
					}
				}
				else
				{
					strRebarSize = L"32A";	//若没有数据，默认为32mm钢筋
				}
			}
			else
			{
				strRebarSize = XmlManager::s_alltypes[vecRebar.at(0).rebarType];	//关联构件已配筋则取关联构件第一层钢筋直径
			}
		}

		BrString strCurRebarSize = XmlManager::s_alltypes[rebarAssembly->GetvecRebarType().at(0)];	//当前墙主筋
		double diameter = RebarCode::GetBarDiameter(strRebarSize, modelRef);
		double curDiameter = RebarCode::GetBarDiameter(strCurRebarSize, modelRef);

		double actDiameter = diameter > curDiameter ? diameter : curDiameter;
		BrString strActRebarSize = diameter > curDiameter ? strRebarSize : strCurRebarSize;

		double adjustedXLen, adjustedXSpacing;
		double adjustedYLen, adjustedYSpacing;

		double sideCover = rebarAssembly->GetSideCover()*uor_per_mm;

		int numRebar = 0;
		double spacing = 200 * uor_per_mm;
		adjustedXLen = m_InSecSTwallData.width - sideCover * 2 - diameter * 2 - diameter;
		int numRebarX = (int)floor(adjustedXLen / spacing + 0.5) + 1;
		if (numRebarX == 0)
		{
			return tag;
		}
		adjustedXSpacing = spacing;
		if (numRebarX > 1)
			adjustedXSpacing = adjustedXLen / (numRebarX - 1);

		adjustedYLen = m_CurSTwallData.width - sideCover * 2 - curDiameter * 2 - actDiameter;
		int numRebarY = (int)floor(adjustedYLen / spacing + 0.5) + 1;
		adjustedYSpacing = spacing;
		if (numRebarY > 1)
			adjustedYSpacing = adjustedYLen / (numRebarY - 1);
		numRebar = (numRebarX + numRebarY) * 2 - 4;

		vector<DSegment3d> vecRebarPt;
		DVec3d vec1 = m_vecInSecPartPt[1] - m_vecInSecPartPt[0];
		DVec3d vec2 = m_vecInSecPartPt[2] - m_vecInSecPartPt[1];
		vec1.Normalize();
		vec2.Normalize();
		double dOffset = actDiameter/2 + curDiameter + sideCover;
		DVec3d vecY = vec2;
		vecY.Scale(dOffset);
		dOffset = actDiameter / 2 + diameter + sideCover;
		DPoint3d ptOrign = m_vecInSecPartPt[0];
		ptOrign.Add(vecY);
		for (int i = 0; i < numRebarX; ++i)
		{
			PITRebarCurve rebarcurve;
			vec1.ScaleToLength(dOffset);
			DPoint3d ptStart = ptOrign;
			ptStart.Add(vec1);
			ptStart.z += sideCover;
			DPoint3d ptEnd = ptStart;
			ptEnd.z += m_CurSTwallData.height;
			ptEnd.z -= sideCover * 2;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
			dOffset += adjustedXSpacing;
		}

		dOffset = adjustedYSpacing;
		for (int i = 0; i < numRebarY - 1; ++i)
		{
			DPoint3d ptStart = vecRebarPt.back().point[0];
			vec2.ScaleToLength(dOffset);
			ptStart.Add(vec2);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
		}

		DVec3d vec3 = m_vecInSecPartPt[3] - m_vecInSecPartPt[2];
		dOffset = adjustedXSpacing;
		vec3.Normalize();
		for (int i = 0; i < numRebarX - 1; ++i)
		{
			DPoint3d ptStart = vecRebarPt.back().point[0];
			vec3.ScaleToLength(dOffset);
			ptStart.Add(vec3);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
		}

		DVec3d vec4 = m_vecInSecPartPt[0] - m_vecInSecPartPt[3];
		dOffset = adjustedYSpacing;
		vec4.Normalize();
		for (int i = 0; i < numRebarY - 2; ++i)
		{
			DPoint3d ptStart = vecRebarPt.back().point[0];
			vec4.ScaleToLength(dOffset);
			ptStart.Add(vec4);
			DPoint3d ptEnd = ptStart;
			ptEnd.z = vecRebarPt.back().point[1].z;
			vecRebarPt.push_back(DSegment3d::From(ptStart, ptEnd));
		}

		//if (vecRebarPt.size())
		//{
		//	double bendRadius = RebarCode::GetPinRadius(strActRebarSize, modelRef, false);
		//	DVec3d vecA = vec1;
		//	DVec3d vecB = vec2;
		//	dOffset = bendRadius - sin(PI / 4) * (bendRadius - actDiameter * 0.5) + actDiameter + sideCover;
		//	vecA.ScaleToLength(dOffset);
		//	vecB.ScaleToLength(dOffset);
		//	DPoint3d ptO(m_vecInSecPartPt[0]);
		//	ptO.Add(vecA);
		//	ptO.Add(vecB);
		//	ptOrign = m_vecInSecPartPt[0];

		//	ptOrign.Add(vecA);
		//	ptOrign.Add(vecB);
		//	double dStartZ = vecRebarPt[0].point[0].z;
		//	vecRebarPt[0].point[0] = ptOrign;
		//	vecRebarPt[0].point[0].z = dStartZ;
		//	double dEndZ = vecRebarPt[0].point[1].z;
		//	vecRebarPt[0].point[1] = ptOrign;
		//	vecRebarPt[0].point[1].z = dEndZ;

		//	// 			EditElementHandle eeh;
		//	// 			LineHandler::CreateLineElement(eeh, NULL, DSegment3d::From(ptIntersect, ptO), true, *ACTIVEMODEL);
		//	// 			eeh.AddToModel();
		//	// 			EditElementHandle eeh1;
		//	// 			LineHandler::CreateLineElement(eeh1, NULL, DSegment3d::From(ptIntersect, ptOrign), true, *ACTIVEMODEL);
		//	// 			eeh1.AddToModel();
		//}

		vector<PITRebarCurve> vecRebars;
		RebarEndType endTypeStart, endTypeEnd;
		endTypeStart.SetType(RebarEndType::kNone);
		endTypeEnd.SetType(RebarEndType::kNone);
		RebarEndTypes endTypes = { endTypeStart ,endTypeEnd };
		for (size_t i = 0; i < vecRebarPt.size(); ++i)
		{
			DPoint3d pt1[2];
			pt1[0] = vecRebarPt[i].point[0];
			pt1[1] = vecRebarPt[i].point[1];
			PITRebarEndTypes endTypes;
			endTypes.beg.SetType(PITRebarEndType::kNone);
			endTypes.end.SetType(PITRebarEndType::kNone);
			makeRebarCurve(vecRebarPt[i].point[0], vecRebarPt[i].point[1], vecRebars, endTypes, sideCover);
			//			PITRebarCurve rebarcurve;
			//			PITRebarEndTypes endTypes;
			//			RebarVertexP vex;
			//			vex = &rebarcurve.PopVertices().NewElement();
			//			vex->SetIP(vecRebarPt[i].point[0]);
			//			vex->SetType(RebarVertex::kStart);
			//
			//			vex = &rebarcurve.PopVertices().NewElement();
			//			vex->SetIP(vecRebarPt[i].point[1]);
			////			vex->SetType(RebarVertex::kEnd);
			//
			//			endTypes.beg.SetType(PITRebarEndType::kNone);
			//			endTypes.end.SetType(PITRebarEndType::kNone);
			//
			//			rebarcurve.EvaluateEndTypes(endTypes);
			//			vecRebars.push_back(rebarcurve);

			// 			EditElementHandle eeh;
			// 			LineHandler::CreateLineElement(eeh, NULL, vecRebarPt[i], true, *ACTIVEMODEL);
			// 			eeh.AddToModel();
		}
		//确保起点终点是从小到大---begin


		//预览按钮按下，则画线
		if (PreviewButtonDown)
		{
			//绘制关联处钢筋，并将线段存入m_allLines
			for (auto it = vecRebarPt.begin(); it != vecRebarPt.end(); it++)
			{
				DSegment3d Dsegtemp(*it);
				DPoint3d strPoint = DPoint3d::From(Dsegtemp.point[0].x, Dsegtemp.point[0].y, Dsegtemp.point[0].z);
				DPoint3d endPoint = DPoint3d::From(Dsegtemp.point[1].x, Dsegtemp.point[1].y, Dsegtemp.point[1].z);
				EditElementHandle eeh;
				LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(strPoint, endPoint), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_RebarAssembly->m_allLines.push_back(eeh.GetElementRef());
			}
		}


		RebarSymbology symb;
		string str(strActRebarSize);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);

		for (size_t i = 0; i < vecRebars.size(); ++i)
		{
			RebarElementP rebarElement = NULL;
			if (!PreviewButtonDown)//不是预览的情况下才生成钢筋
			{
				rebarElement = rebarSet->AssignRebarElement((int)i, (int)vecRebars.size(), symb, modelRef);
			}
			PITRebarCurve rebarCurve = vecRebars[i];
			if (NULL != rebarElement)
			{
				RebarShapeData shape;
				shape.SetSizeKey((LPCTSTR)strActRebarSize);
				shape.SetIsStirrup(false);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				rebarElement->Update(rebarCurve, actDiameter, endTypes, shape, modelRef, false);
			}

			// 添加钢筋层号和正反面
			CPoint3D ptstr, ptend;
			rebarCurve.GetEndPoints(ptstr, ptend);
			ptstr.z = m_InSecSTwallData.ptEnd.z; 
			DPoint3d ptTmp = ptstr;
			double dDistance = sideCover + actDiameter;
			AddRebarLevelAndType(rebarElement, strCurRebarSize, ptTmp, m_InSecSTwallData.ptStart, m_InSecSTwallData.ptEnd, dDistance, true);
		}

		RebarSetData setdata;
		setdata.SetNumber((int)vecRebars.size());
		setdata.SetNominalSpacing(spacing);
		setdata.SetAverageSpacing(adjustedXSpacing);

		rebarSet->FinishUpdate(setdata, modelRef);
	}
	break;
	case ACCRebarAssemblyNew::ComponentType::GWALL:
	{
		//		GWallRebarAssembly *rebarAssembly = dynamic_cast<GWallRebarAssembly*>(m_RebarAssembly);
	}
	break;
	default:
		break;
	}

	tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);

	return tag;
}

bool ACCRebarMethod7_MakerNew::MakeRebars(DgnModelRefP modelRef)
{
	if (m_RebarAssembly == NULL)
		return NULL;
	bool ret = false;
	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);

		const vector<PIT::AssociatedComponent>& vecAC = rebarAssembly->PopvecAC();
		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		double dOffset,dL0;
		string strName = m_ACCEh.EleName;
		auto it = std::find_if(vecAC.begin(), vecAC.end(), [&strName](AssociatedComponent tmphd) {return string(tmphd.associatedComponentName) == strName; });
		if (it != vecAC.end())
		{
			dOffset = (*it).endOffset;
			dL0 = (*it).endL0;
		}

		vector<vector<PIT::EndType> > &vvEndType = rebarAssembly->PopvvecEndType();

		//终点偏移
		if (vvEndType.size())
		{
			vector<PIT::ConcreteRebar> vecRebarData;
			rebarAssembly->GetRebarData(vecRebarData);
			//取当前构件的混凝土保护层
			WallRebarInfo curConcrete;
			if (GetCurElementHandle().IsValid())
			{
				ElementId concreteid = 0;
				GetElementXAttribute(GetCurElementHandle().GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
				if (SUCCESS != GetElementXAttribute(concreteid, sizeof(WallRebarInfo), curConcrete, WallRebarInfoXAttribute, ACTIVEMODEL));
				{
					curConcrete.concrete.sideCover = 50;
					curConcrete.concrete.postiveCover = 50;
					curConcrete.concrete.reverseCover = 50;
				}
			}

			for (size_t i = 0; i < vecRebarData.size(); ++i)
			{
				if (vecRebarData[i].rebarDir == 1)	//所有纵向钢筋都锚入到板上
				{
					vvEndType[i][1].endType = 7;		//直锚
					vvEndType[i][1].endPtInfo.value1 = m_slabHeight + curConcrete.concrete.sideCover * uor_per_mm + dL0 * uor_per_mm + dOffset * uor_per_mm;
				}
			}
		}
	}
	break;
	case ACCRebarAssemblyNew::ComponentType::GWALL:
	{
		//		GWallRebarAssembly *rebarAssembly = dynamic_cast<GWallRebarAssembly*>(m_RebarAssembly);
	}
	break;
	default:
		break;
	}

	return ret;
}

RebarSetTag * ACCRebarMethod7_MakerNew::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	return nullptr;
}

bool ACCRebarMethod9_MakerNew::MakeRebars(DgnModelRefP modelRef)
{
	if (m_RebarAssembly == NULL)
		return NULL;
	bool ret = false;
	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);

		const vector<PIT::AssociatedComponent>& vecAC = rebarAssembly->PopvecAC();
		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		double dOffset, dL0, dLa;
		int isReverse = 0;
		string strName = m_ACCEh.EleName;
		auto it = std::find_if(vecAC.begin(), vecAC.end(), [&strName](AssociatedComponent tmphd) {return string(tmphd.associatedComponentName) == strName; });
		if (it != vecAC.end())
		{
			dOffset = (*it).endOffset;
			dL0 = (*it).endL0;
			dLa = (*it).La;
			isReverse = (*it).isReverse;
		}

		vector<vector<PIT::EndType> > &vvEndType = rebarAssembly->PopvvecEndType();

		//终点偏移
		if (vvEndType.size())
		{	
			vector<PIT::ConcreteRebar> vecRebarData;
			rebarAssembly->GetRebarData(vecRebarData);
			//取当前构件的混凝土保护层
			WallRebarInfo curConcrete;
			if (GetCurElementHandle().IsValid())
			{
				ElementId concreteid = 0;
				GetElementXAttribute(GetCurElementHandle().GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
				if (SUCCESS != GetElementXAttribute(concreteid, sizeof(WallRebarInfo), curConcrete, WallRebarInfoXAttribute, ACTIVEMODEL));
				{
					curConcrete.concrete.sideCover = 50;
					curConcrete.concrete.postiveCover = 50;
					curConcrete.concrete.reverseCover = 50;
				}
			}

			for (size_t i = 0; i < vecRebarData.size(); ++i)
			{
				if (vecRebarData[i].rebarDir == 1)	//所有纵向钢筋都锚入到板上
				{
					vvEndType[i][1].endType = 7;		//直锚
					if (!isReverse)
					{
						if (i < vecRebarData.size() / 2)	//前一半钢筋层
							vvEndType[i][1].endPtInfo.value1 = m_slabHeight + curConcrete.concrete.sideCover * uor_per_mm + dL0 * uor_per_mm + dOffset * uor_per_mm;
						else
							vvEndType[i][1].endPtInfo.value1 = curConcrete.concrete.sideCover * uor_per_mm + dLa * uor_per_mm;
					}
					else
					{
						if (i < vecRebarData.size() / 2)	//前一半钢筋层
							vvEndType[i][1].endPtInfo.value1 = curConcrete.concrete.sideCover * uor_per_mm + dLa * uor_per_mm;
						else
							vvEndType[i][1].endPtInfo.value1 = m_slabHeight + curConcrete.concrete.sideCover * uor_per_mm + dL0 * uor_per_mm + dOffset * uor_per_mm;
					}
				}
			}
		}
	}
	break;
	case ACCRebarAssemblyNew::ComponentType::GWALL:
	{
		//		GWallRebarAssembly *rebarAssembly = dynamic_cast<GWallRebarAssembly*>(m_RebarAssembly);
	}
	break;
	default:
		break;
	}

	return ret;
}

RebarSetTag * ACCRebarMethod9_MakerNew::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	return nullptr;
}

bool ACCRebarMethod10_MakerNew::MakeRebars(DgnModelRefP modelRef)
{
	if (m_RebarAssembly == NULL)
		return NULL;
	bool ret = false;
	switch (m_RebarAssembly->GetcpType())
	{
	case ACCRebarAssemblyNew::ComponentType::STWALL:
	{
		ACCSTWallRebarAssemblyNew *rebarAssembly = dynamic_cast<ACCSTWallRebarAssemblyNew*>(m_RebarAssembly);

		double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

		BrString strRebarSize;
		double dOffset, dL0, dLa;
		int isReverse = 0;
		string strName = m_ACCEh.EleName;
		const vector<PIT::AssociatedComponent>& vecAC = rebarAssembly->PopvecAC();
		auto it = std::find_if(vecAC.begin(), vecAC.end(), [&strName](AssociatedComponent tmphd) {return string(tmphd.associatedComponentName) == strName; });
		if (it != vecAC.end())
		{
			dOffset = (*it).endOffset;
			dL0 = (*it).endL0;
			dLa = (*it).La;
			isReverse = (*it).isReverse;
		}

		vector<vector<PIT::EndType> > &vvEndType = rebarAssembly->PopvvecEndType();

		//终点偏移
		if (vvEndType.size())
		{
			vector<PIT::ConcreteRebar> vecRebarData;
			rebarAssembly->GetRebarData(vecRebarData);
			//取当前构件的混凝土保护层
			WallRebarInfo InsecConcrete;
// 			if (GetCurElementHandle().IsValid())
// 			{
// 				ElementId concreteid = 0;
// 				GetElementXAttribute(GetCurElementHandle().GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
// 				if (SUCCESS != GetElementXAttribute(concreteid, sizeof(WallRebarInfo), curConcrete, WallRebarInfoXAttribute, ACTIVEMODEL));
// 				{
// 					curConcrete.concrete.sideCover = 50;
// 					curConcrete.concrete.postiveCover = 50;
// 					curConcrete.concrete.reverseCover = 50;
// 				}
// 			}

			vector<PIT::ConcreteRebar> vecRebar;
			if (m_ACCEh.Eh.IsValid())	//取关联构件的钢筋尺寸，若关联构件未配筋，则无需绘制交集处纵向钢筋
			{
				ElementId concreteid = 0;
				GetElementXAttribute(m_ACCEh.Eh.GetElementId(), sizeof(ElementId), concreteid, ConcreteIDXAttribute, m_ACCEh.Eh.GetModelRef());
				GetElementXAttribute(concreteid, vecRebar, vecRebarDataXAttribute, ACTIVEMODEL);
				if (vecRebar.empty())//关联构件未配置钢筋
				{
					strRebarSize = L"32A";	//若没有数据，默认为32mm钢筋
				}
				else
				{
					BrString sizekey = XmlManager::s_alltypes[vecRebar[0].rebarType];
					strRebarSize = sizekey;	//关联构件已配筋则取关联构件第一层钢筋直径
				}
				if (SUCCESS != GetElementXAttribute(concreteid, sizeof(WallRebarInfo), InsecConcrete, WallRebarInfoXAttribute, ACTIVEMODEL));
				{
					InsecConcrete.concrete.sideCover = 50;
					InsecConcrete.concrete.postiveCover = 50;
					InsecConcrete.concrete.reverseCover = 50;
				}
			}

			for (size_t i = 0; i < vecRebarData.size(); ++i)
			{
				if (vecRebarData[i].rebarDir == 1)	//所有纵向钢筋都锚入到板上
				{
					if (!isReverse)
					{
						if (i < vecRebarData.size() / 2)	//前一半钢筋层
						{
							vvEndType[i][1].endType = 7;		//直锚
							vvEndType[i][1].endPtInfo.value1 = InsecConcrete.concrete.postiveCover * uor_per_mm + dLa * uor_per_mm;
						}
						else
						{
							double dim = RebarCode::GetBarDiameter(strRebarSize, modelRef);
							double dCurSideCover = m_RebarAssembly->GetSideCover();
							vvEndType[i][1].offset = -(m_slabHeight / uor_per_mm + InsecConcrete.concrete.postiveCover - dim * 2 / uor_per_mm - dCurSideCover);
							vvEndType[i][1].endPtInfo.value1 = 0;
							vvEndType[i][1].endType = 4;			//端部弯曲方式 90度弯曲 与上边变量一致
							vvEndType[i][1].rotateAngle = 90;		//端部弯钩方向旋转
							vvEndType[i][1].endPtInfo.value3 = dLa;//预留长度
						}
					}
					else
					{
						if (i < vecRebarData.size() / 2)	//前一半钢筋层
						{
							double dim = RebarCode::GetBarDiameter(strRebarSize, modelRef);
							double dCurSideCover = m_RebarAssembly->GetSideCover();
							vvEndType[i][1].offset = -(m_slabHeight / uor_per_mm + InsecConcrete.concrete.postiveCover - dim * 2 / uor_per_mm - dCurSideCover);
							vvEndType[i][1].endPtInfo.value1 = 0;
							vvEndType[i][1].endType = 4;			//端部弯曲方式 90度弯曲 与上边变量一致
							vvEndType[i][1].rotateAngle = -90;		//端部弯钩方向旋转
							vvEndType[i][1].endPtInfo.value3 = dLa;//预留长度
						}
						else
						{
							vvEndType[i][1].endType = 7;		//直锚
							vvEndType[i][1].endPtInfo.value1 = InsecConcrete.concrete.postiveCover * uor_per_mm + dLa * uor_per_mm;
						}
					}
				}
			}
		}
	}
	break;
	case ACCRebarAssemblyNew::ComponentType::GWALL:
	{
	}
	break;
	default:
		break;
	}

	return ret;
}

RebarSetTag * ACCRebarMethod10_MakerNew::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	return nullptr;
}

RebarSetTag * LineStringRebarMakerNew::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	if (modelRef == NULL)
		return NULL;

	//绘制U形钢筋
	RebarSetTag* tag = nullptr;
	double diameter = RebarCode::GetBarDiameter(m_rebarSize, modelRef);
	RebarSetP rebarSet = MakeLineStringRebar(rebarSetId, m_callerId, m_segs, diameter, modelRef);
	tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);

	return tag;
}
bool LineStringRebarMakerNew::makeRebarCurve
(
	DPoint3d ptstr,
	DPoint3d ptend,
	vector<shared_ptr<PITRebarCurve>>&     rebars,
	PITRebarEndTypes&		endTypes,
	double dSideCover
)
{
	DPoint3d pt1[2];
	pt1[0] = ptstr;
	pt1[1] = ptend;
	//确保起点终点是从小到大---begin
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
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);
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
		PITRebarEndTypes		tmpendTypes;

		shared_ptr<PITRebarCurve> rebar(new PITRebarCurve);
		RebarVertexP vex;
		vex = &rebar->PopVertices().NewElement();
		vex->SetIP(itr->second);
		vex->SetType(RebarVertex::kStart);
		if (pt1[0].Distance(itr->second) < 10)
		{
			tmpendTypes.beg = endTypes.beg;
		}
		tmpendTypes.beg.SetptOrgin(itr->second);
		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		if (pt1[1].Distance(itrplus->second) < 10)
		{
			tmpendTypes.end = endTypes.end;
		}

		tmpendTypes.end.SetptOrgin(itrplus->second);

		vex = &rebar->PopVertices().NewElement();
		vex->SetIP(itrplus->second);
		vex->SetType(RebarVertex::kEnd);

		rebar->EvaluateEndTypes(tmpendTypes);
		//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebars.push_back(rebar);
	}


	//rebar.DoMatrix(mat);
	return true;
}
RebarSetP LineStringRebarMakerNew::MakeLineStringRebar(ElementId rebarSetId, ElementId callerId, const vector<LineSegment>& segs, double offset, DgnModelRefP modelRef)
{
	if (segs.empty())
	{
		return nullptr;
	}
	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->StartUpdate(modelRef);
	if (NULL == rebarSet)
		return NULL;
	double diameter = RebarCode::GetBarDiameter(m_rebarSize, modelRef);
	double bendRadius = RebarCode::GetPinRadius(m_rebarSize, modelRef, false);
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(callerId);
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };

	DPoint3d planeNormal;
	if (segs.size() == 1)
	{
		planeNormal = DPoint3d::From(0, 0, 1);
	}
	else
	{
		DPlane3d plane = DPlane3d::From3Points(segs.front().GetLineStartPoint(), segs.front().GetLineEndPoint(), segs.back().GetLineStartPoint());
		planeNormal = plane.normal;
	}

	vector<DPoint3d> rebarPts;
	double adjustedXLen = 0;
	double adjustedSpacing = 0;
	int numRebarTotal = 0;
	double xPos = 0;
	vector<shared_ptr<PITRebarCurve> >     rebarCurves;
	DPoint3d ptLast;
	for (size_t index = 0; index < segs.size(); ++index)
	{
		DVec3d vec;
		LineSegment seg = segs[index];
		vec.CrossProduct(planeNormal, seg.GetLineVec());
		seg.PerpendicularOffset(m_cover + diameter, vec);
		seg.Shorten(m_cover + diameter*0.5 + offset, true);
		seg.Shorten(m_cover + diameter*0.5 + offset, false);
		adjustedXLen = seg.GetLength();
		int numRebar = (int)floor(adjustedXLen / m_spacing + 0.5) + 1;
		numRebarTotal += numRebar;
		adjustedSpacing = m_spacing;
		if (numRebar > 1)
			adjustedSpacing = adjustedXLen / (numRebar - 1);

		PITRebarEndType start, end;
		start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
		end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
		PITRebarEndTypes   endTypes = { start, end };
		DPoint3d ptFirst;
		for (int i = 0; i < numRebar; i++)
		{
			if (0 != i)
			{
				seg.Shorten(adjustedSpacing, true);
			}
			DPoint3d ptStart = seg.GetLineStartPoint();
			if (0 == index && 0 == i)
			{
				ptFirst = ptStart;
			}
			if (ptStart.Distance(ptLast) < diameter)
			{
				numRebarTotal--;
				continue;
			}
			if (index == segs.size()-1 && i == numRebar - 1)
			{
				if (ptFirst.Distance(ptStart) < diameter)
				{
					numRebarTotal--;
					continue;
				}
			}
			DPoint3d ptEnd = planeNormal;
			ptEnd.ScaleToLength(m_height);
			ptEnd.Add(ptStart);
			makeRebarCurve(ptStart, ptEnd, rebarCurves, endTypes, 0);
			/*shared_ptr<PITRebarCurve> rebar(new PITRebarCurve);
			RebarVertexP vex;
			vex = &rebar->PopVertices().NewElement();
			vex->SetIP(ptStart);
			vex->SetType(RebarVertex::kStart);
			endTypes.beg.SetptOrgin(ptStart);
			endTypes.end.SetptOrgin(ptEnd);
			vex = &rebar->PopVertices().NewElement();
			vex->SetIP(ptEnd);
			vex->SetType(RebarVertex::kEnd);
			rebar->EvaluateEndTypes(endTypes);
			rebarCurves.push_back(rebar);*/
			if (i == numRebar -1)
			{
				ptLast = ptStart;
			}
		}
	}

	RebarSymbology symb;
	string str(m_rebarSize);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);

	for (size_t i = 0; i < rebarCurves.size(); ++i)
	{
		RebarElementP rebarElement = rebarSet->AssignRebarElement((int)i, (int)rebarCurves.size(), symb, modelRef);
		RebarCurve rebarCurve = *rebarCurves[i];
		if (NULL != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)m_rebarSize);
			shape.SetIsStirrup(false);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
		}
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebarTotal);
	setdata.SetNominalSpacing(m_spacing);
	setdata.SetAverageSpacing(adjustedSpacing);

	rebarSet->FinishUpdate(setdata, modelRef);

	return rebarSet;
}