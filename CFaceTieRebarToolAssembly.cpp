#include "_ustation.h"
#include "CFaceTieRebarToolAssembly.h"
#include "ExtractFacesTool.h"
#include "TieRebar.h"
#include "SelectRebarTool.h"
#include "XmlHelper.h"


CFaceTieRebarToolAssembly::CFaceTieRebarToolAssembly(ElementId id /*= 0*/, DgnModelRefP modelRef /*= NULL*/)
{
	InitReabrData();
}

CFaceTieRebarToolAssembly::~CFaceTieRebarToolAssembly()
{
	for (int i = 0; i < m_Negs.size(); i++)
	{

		if (m_Negs.at(i) != nullptr)
		{
			delete m_Negs.at(i);
			m_Negs.at(i) = nullptr;
		}
	}
	for (int j = 0; j < m_Holeehs.size(); j++)
	{

		if (m_Holeehs.at(j) != nullptr)
		{
			delete m_Holeehs.at(j);
			m_Holeehs.at(j) = nullptr;
		}
	}
}

void CFaceTieRebarToolAssembly::InitReabrData()
{
	m_MainFirLevSizekey = "";
	m_MainSecLevSizekey = "";
	m_AnthorFirLevSizekey = "";
	m_AnthorSecLevSizekey = "";

	m_MainLevVec = CVector3D::From(0, 0, 0);
	m_AnthorLevVec = CVector3D::From(0, 0, 0);

	m_vecMainFirLevRebar.clear();
	m_vecMainSecLevRebar.clear();
	m_vecAnthorFirLevRebar.clear();
	m_vecAnthorSecLevRebar.clear();
}

bool CFaceTieRebarToolAssembly::AnalyzingWallGeometricData(ElementHandleCR eh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	DPoint3d FrontStr, FrontEnd;
	DPoint3d BackStr, BackEnd;
	std::vector<EditElementHandle*> Negs;

	DgnModelRefP model = eh.GetModelRef();
	vector<DSegment3d> vecDownFontLine;
	vector<DSegment3d> vecDownBackLine;

	EditElementHandle testeeh(eh, false);
	//testeeh.AddToModel();

	double dHeight = 0.00;

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &dHeight);

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

	if (vecDownBackLine.size() > 1 || vecDownFontLine.size() > 1)
	{
		GetMaxDownFacePts(vecDownFontLine, vecDownBackLine, pt1, pt2);
	}

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, dHeight);

	m_ptStart = FrontStr;
	m_ptEnd = FrontEnd;

	m_Negs = Negs;
	m_Holeehs = Holeehs;

	return true;
}

void CFaceTieRebarToolAssembly::GetEleNameAndType(ElementHandleR eeh)
{
	WString Ename;
	WString Etype;
	DgnECManagerR ecMgr = DgnECManager::GetManager();
	FindInstancesScopePtr scope = FindInstancesScope::CreateScope(eeh, FindInstancesScopeOption(DgnECHostType::Element));
	ECQueryPtr            ecQuery = ECQuery::CreateQuery(ECQUERY_PROCESS_SearchAllClasses);
	ecQuery->SetSelectProperties(true);
	for (DgnECInstancePtr instance : ecMgr.FindInstances(*scope, *ecQuery))
	{
		DgnElementECInstanceP elemInst = instance->GetAsElementInstance();
		if (elemInst->GetClass().GetDisplayLabel() == L"PDMS_Attributes")//如果有PDMS_Attributes的EC属性，读取唯一NAME值
		{
			ECN::ECValue ecVal;
			elemInst->GetValue(ecVal, L"NAME");
			Ename = ecVal.ToString();
			elemInst->GetValue(ecVal, L"TYPE");
			Etype = ecVal.ToString();
			break;
		}
	}
	if (Ename.ContainsI(L"VB") || Etype.ContainsI(L"wall"))
	{
		m_modelType = 0;
	}
	else
	{
		m_modelType = 1;
	}
	return;
}

void CFaceTieRebarToolAssembly::CalculateSelectRebarInfo(vector<ElementRefP>& vecSelectrebars, DgnModelRefP modelRef)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	InitReabrData();

	for (vector<ElementRefP>::iterator itr = vecSelectrebars.begin(); itr != vecSelectrebars.end(); itr++)
	{
		EditElementHandle eehHandle(*itr, (*itr)->GetDgnModelP());
		if (RebarElement::IsRebarElement(eehHandle))
		{
			double diameter;
			DPoint3d ptstr, ptend;
			vector<DSegment3d> vecSeg;
			GetStartEndPointFromRebar(&eehHandle, ptstr, ptend, diameter);

			if (m_MainLevVec.IsZero() || abs(m_MainLevVec.DotProduct(ptend - ptstr) > 0.9)/*!m_MainLevVec.IsPerpendicularTo(ptend - ptstr)*/) // 主方向
			{
				// 第一层
				if (push_vecPoint(m_vecMainFirLevRebar, ptstr, ptend, m_MainLevVec, m_AnthorLevVec, modelRef))
				{
					if (m_MainFirLevSizekey == "")
					{
						RebarCurve curve;
						RebarElementP rep = RebarElement::Fetch(eehHandle);
						RebarShape * rebarshape = rep->GetRebarShape(eehHandle.GetModelRef());
						m_MainFirLevSizekey = rebarshape->GetShapeData().GetSizeKey();
					}
					continue;
				}

				// 第二层
				if (push_vecPoint(m_vecMainSecLevRebar, ptstr, ptend, m_MainLevVec, m_AnthorLevVec, modelRef))
				{
					if (m_MainSecLevSizekey == "")
					{
						RebarCurve curve;
						RebarElementP rep = RebarElement::Fetch(eehHandle);
						RebarShape * rebarshape = rep->GetRebarShape(eehHandle.GetModelRef());
						m_MainSecLevSizekey = rebarshape->GetShapeData().GetSizeKey();
					}
					continue;
				}
			}
			else // 另一方向
			{
				DPoint3d ptFirStr, ptFirEnd, ptStr;
				m_vecMainFirLevRebar[0].GetStartPoint(ptFirStr);
				m_vecMainFirLevRebar[0].GetEndPoint(ptFirEnd);
				CVector3D vec = ptFirEnd - ptFirStr;
				vec.Normalize();

				CVector3D vecNormol = CVector3D::kZaxis;

				CVector3D vecTmp = ptend - ptstr;
				vecTmp.Normalize();
				vecNormol = vec.CrossProduct(vecTmp);

				if (m_modelType == 1)
				{
					vecNormol = CVector3D::kZaxis;
				}

				// 计算一个点在由原点和法线 bvector 定义的平面上的投影
				mdlVec_projectPointToPlane(&ptStr, &ptstr, &ptFirStr, &vecNormol);
				double dMainDiameter = RebarCode::GetBarDiameter(m_MainFirLevSizekey, modelRef);
				RebarCurve curve;
				RebarElementP rep = RebarElement::Fetch(eehHandle);
				RebarShape * rebarshape = rep->GetRebarShape(eehHandle.GetModelRef());
				BrString rebarKey = rebarshape->GetShapeData().GetSizeKey();
				double diameter = RebarCode::GetBarDiameter(rebarKey, modelRef);

				if (COMPARE_VALUES_EPS(ptStr.Distance(ptstr), dMainDiameter * 0.5 + diameter * 0.5, 10) == 0)
				{
					// 第一层
					if (push_vecPoint(m_vecAnthorFirLevRebar, ptstr, ptend, m_AnthorLevVec, m_MainLevVec, modelRef))
					{
						if (m_AnthorFirLevSizekey == "")
						{
							m_AnthorFirLevSizekey = rebarKey;
						}
						continue;
					}
				}
				else
				{
					// 第二层
					if (push_vecPoint(m_vecAnthorSecLevRebar, ptstr, ptend, m_AnthorLevVec, m_MainLevVec, modelRef))
					{
						if (m_AnthorSecLevSizekey == "")
						{
							m_AnthorSecLevSizekey = rebarKey;
						}
						continue;
					}
				}
			}
		}
	}

	if (m_vecMainFirLevRebar.size() == 0 || m_vecMainSecLevRebar.size() == 0 ||
		m_vecAnthorFirLevRebar.size() == 0 || m_vecAnthorSecLevRebar.size() == 0)
	{
		return;
	}
	//计算主方向两层之间的距离
	DPoint3d mainFirProjectStrPt = { 0,0,0 }, mainFirProjectEndPt = { 0,0,0 }, mainSecProjectPt = { 0,0,0 };
	mdlVec_projectPointToPlane(&mainFirProjectStrPt, &(*m_vecMainFirLevRebar.begin()).point[0], &m_vecAnthorFirLevRebar.at(0).point[0], m_MainLevVec);
	mdlVec_projectPointToPlane(&mainFirProjectEndPt, &(*m_vecMainFirLevRebar.rbegin()).point[0], &m_vecAnthorFirLevRebar.at(0).point[0], m_MainLevVec);
	mdlVec_projectPointToPlane(&mainSecProjectPt, &m_vecMainSecLevRebar.at(0).point[0], &m_vecAnthorFirLevRebar.at(0).point[0], m_MainLevVec);
	DPoint3d mainProjectPt = { 0,0,0 };
	mdlVec_projectPointToLine(&mainProjectPt, nullptr, &mainSecProjectPt, &mainFirProjectStrPt, &mainFirProjectEndPt);
	double mainDis = mainSecProjectPt.Distance(mainProjectPt);

	//计算另一方向两层间的距离
	DPoint3d anotherFirProjectStrPt = { 0,0,0 }, anotherFirProjectEndPt = { 0,0,0 }, anoterSecProjectPt = { 0,0,0 };
	mdlVec_projectPointToPlane(&anotherFirProjectStrPt, &(*m_vecAnthorFirLevRebar.begin()).point[0], &m_vecMainFirLevRebar.at(0).point[0], m_AnthorLevVec);
	mdlVec_projectPointToPlane(&anotherFirProjectEndPt, &(*m_vecAnthorFirLevRebar.rbegin()).point[0], &m_vecMainFirLevRebar.at(0).point[0], m_AnthorLevVec);
	mdlVec_projectPointToPlane(&anoterSecProjectPt, &m_vecAnthorSecLevRebar.at(0).point[0], &m_vecMainFirLevRebar.at(0).point[0], m_AnthorLevVec);
	DPoint3d anotherProjectPt = { 0,0,0 };
	mdlVec_projectPointToLine(&anotherProjectPt, nullptr, &anoterSecProjectPt, &anotherFirProjectStrPt, &anotherFirProjectEndPt);
	double anotherDis = anoterSecProjectPt.Distance(anotherProjectPt);

	if (COMPARE_VALUES_EPS(mainDis, anotherDis, 1) == -1) //主方向在内侧
	{
		//交换方向
		CVector3D tmpVec = m_MainLevVec;
		m_MainLevVec = m_AnthorLevVec;
		m_AnthorLevVec = tmpVec;

		//交换第一层主方向和另一方向钢筋
		vector<DSegment3d> tmpSegs = m_vecMainFirLevRebar;
		m_vecMainFirLevRebar = m_vecAnthorFirLevRebar;
		m_vecAnthorFirLevRebar = tmpSegs;
		BrString tmpStr = m_MainFirLevSizekey;
		m_MainFirLevSizekey = m_AnthorFirLevSizekey;
		m_AnthorFirLevSizekey = tmpStr;

		//交换第二层主方向和另一方向钢筋
		tmpSegs = m_vecMainSecLevRebar;
		m_vecMainSecLevRebar = m_vecAnthorSecLevRebar;
		m_vecAnthorSecLevRebar = tmpSegs;
		tmpStr = m_MainSecLevSizekey;
		m_MainSecLevSizekey = m_AnthorSecLevSizekey;
		m_AnthorSecLevSizekey = tmpStr;
	}
}

void CFaceTieRebarToolAssembly::SortAllVecRebar()
{
	SortVecRebar(m_vecMainFirLevRebar, m_AnthorLevVec, m_MainLevVec);
	SortVecRebar(m_vecMainSecLevRebar, m_AnthorLevVec, m_MainLevVec);
	SortVecRebar(m_vecAnthorFirLevRebar, m_MainLevVec, m_AnthorLevVec);
	SortVecRebar(m_vecAnthorSecLevRebar, m_MainLevVec, m_AnthorLevVec);
}

void CFaceTieRebarToolAssembly::TraveAllRebar(RebarAssembly* pRebarAssembly, DgnModelRefP modelRef)
{
	RebarSets rebar_sets;
	pRebarAssembly->GetRebarSets(rebar_sets, ACTIVEMODEL); // 钢筋组
	if (m_isContinRebar) // 连续配筋
	{
		for (int i = 0; i < rebar_sets.GetSize(); i++) // 遍历所有钢筋
		{
			RebarSetP rebarSet = &rebar_sets.At(i);
			int nNum = (int)rebarSet->GetChildElementCount(); // 钢筋组中钢筋数量
			for (int j = 0; j < nNum; j++)
			{
				double diameter;
				DPoint3d ptStar, ptEnd;
				RebarElementP pRebar = rebarSet->GetChildElement(j);
				CalaRebarStartEnd(pRebar, ptStar, ptEnd, diameter, modelRef);
				CVector3D vec = ptEnd - ptStar;
				vec.Normalize();
				if (vec.IsEqual(m_MainLevVec, EPS))
				{
					if (m_vecMainFirLevRebar.size() >= 2)
					{
						bool bFlag = false;
						DPoint3d pt[2];
						m_vecMainFirLevRebar[0].GetStartPoint(pt[0]);
						m_vecMainFirLevRebar[1].GetStartPoint(pt[1]);
						if (EFT::IsPointInLine(ptStar, pt[0], pt[1], modelRef, bFlag))
						{
							m_vecMainFirLevRebar.push_back({ ptStar, ptEnd });
							continue;
						}
					}

					if (m_vecMainSecLevRebar.size() >= 2)
					{
						bool bFlag = false;
						DPoint3d pt[2];
						m_vecMainSecLevRebar[0].GetStartPoint(pt[0]);
						m_vecMainSecLevRebar[1].GetStartPoint(pt[1]);
						if (EFT::IsPointInLine(ptStar, pt[0], pt[1], modelRef, bFlag))
						{
							m_vecMainSecLevRebar.push_back({ ptStar, ptEnd });
							continue;
						}
					}
				}
				else if (vec.IsEqual(m_AnthorLevVec, EPS))
				{
					if (m_vecAnthorFirLevRebar.size() >= 2)
					{
						bool bFlag = false;
						DPoint3d pt[2];
						m_vecAnthorFirLevRebar[0].GetStartPoint(pt[0]);
						m_vecAnthorFirLevRebar[1].GetStartPoint(pt[1]);
						if (EFT::IsPointInLine(ptStar, pt[0], pt[1], modelRef, bFlag))
						{
							m_vecAnthorFirLevRebar.push_back({ ptStar, ptEnd });
							continue;
						}
					}

					if (m_vecAnthorSecLevRebar.size() >= 2)
					{
						bool bFlag = false;
						DPoint3d pt[2];
						m_vecAnthorSecLevRebar[0].GetStartPoint(pt[0]);
						m_vecAnthorSecLevRebar[1].GetStartPoint(pt[1]);
						if (EFT::IsPointInLine(ptStar, pt[0], pt[1], modelRef, bFlag))
						{
							m_vecAnthorSecLevRebar.push_back({ ptStar, ptEnd });
							continue;
						}
					}
				}
			}
		}
	}

	if (m_AnthorLevVec.IsZero()) //只选择了一个方向的钢筋,另一个方向的交集钢筋需要遍历
	{
		for (int i = 0; i < rebar_sets.GetSize(); i++) // 遍历所有钢筋
		{
			RebarSetP rebarSet = &rebar_sets.At(i);
			int nNum = (int)rebarSet->GetChildElementCount(); // 钢筋组中钢筋数量
			for (int j = 0; j < nNum; j++)
			{
				double diameter;
				DPoint3d ptStar, ptEnd;
				RebarElementP pRebar = rebarSet->GetChildElement(j);
				CalaRebarStartEnd(pRebar, ptStar, ptEnd, diameter, modelRef);
				CVector3D vec = ptEnd - ptStar;
				vec.Normalize();
				if (!vec.IsEqual(m_MainLevVec, EPS))
				{
					bool bFlag = true;
					for (int i = 0; i < (int)m_vecMainFirLevRebar.size(); i++)
					{
						DPoint3d pt[2];
						m_vecMainFirLevRebar[i].GetStartPoint(pt[0]);
						m_vecMainFirLevRebar[i].GetEndPoint(pt[1]);
						DPoint3d ptProStr, ptProEnd, ptIntersec;
						vec = ptEnd - ptStar;
						vec.Normalize();
						CVector3D vecCom = pt[1] - pt[0];
						vecCom.Normalize();
						CVector3D vecNormal = vecCom.CrossProduct(vec);
						mdlVec_projectPointToPlane(&ptProStr, &ptStar, &pt[0], &vecNormal);
						double diameterTmp = RebarCode::GetBarDiameter(m_MainFirLevSizekey, modelRef);
						if (COMPARE_VALUES_EPS(ptProStr.Distance(ptStar), diameter * 0.5 + diameterTmp * 0.5, 10) != 0)
						{
							continue;
						}
						bFlag = false;
						mdlVec_projectPointToPlane(&ptProEnd, &ptEnd, &pt[0], &vecNormal);
						DSegment3d segTmp = DSegment3d::From(ptProStr, ptProEnd);

						//EditElementHandle eeh;
						//LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptProStr, ptProEnd), true, *ACTIVEMODEL);
						//eeh.AddToModel();

						if (SUCCESS == mdlVec_intersect(&ptIntersec, &segTmp, &m_vecMainFirLevRebar[i]))
						{
							bool isStr = false;
							if (!EFT::IsPointInLine(ptIntersec, ptProStr, ptProEnd, modelRef, isStr))
							{
								continue;
							}
							if (m_AnthorLevVec.IsZero())
							{
								m_AnthorLevVec = ptEnd - ptStar;
								m_AnthorLevVec.Normalize();
							}
							m_vecAnthorFirLevRebar.push_back({ ptStar, ptEnd });
						}
					}

					if (!bFlag)
					{
						continue;
					}

					for (int i = 0; i < (int)m_vecMainSecLevRebar.size(); i++)
					{
						DPoint3d pt[2];
						m_vecMainSecLevRebar[i].GetStartPoint(pt[0]);
						m_vecMainSecLevRebar[i].GetEndPoint(pt[1]);
						DPoint3d ptProStr, ptProEnd, ptIntersec;
						vec = ptEnd - ptStar;
						vec.Normalize();
						CVector3D vecCom = pt[1] - pt[0];
						vecCom.Normalize();
						CVector3D vecNormal = vecCom.CrossProduct(vec);
						mdlVec_projectPointToPlane(&ptProStr, &ptStar, &pt[0], &vecNormal);
						double diameterTmp = RebarCode::GetBarDiameter(m_MainFirLevSizekey, modelRef);
						if (COMPARE_VALUES_EPS(ptProStr.Distance(ptStar), diameter * 0.5 + diameterTmp * 0.5, 10) != 0)
						{
							continue;
						}
						mdlVec_projectPointToPlane(&ptProEnd, &ptEnd, &pt[0], &vecNormal);
						DSegment3d segTmp = DSegment3d::From(ptProStr, ptProEnd);
						if (SUCCESS == mdlVec_intersect(&ptIntersec, &segTmp, &m_vecMainSecLevRebar[i]))
						{
							bool isStr = false;
							if (!EFT::IsPointInLine(ptIntersec, ptProStr, ptProEnd, modelRef, isStr))
							{
								continue;
							}
							if (m_AnthorLevVec.IsZero())
							{
								m_AnthorLevVec = ptEnd - ptStar;
								m_AnthorLevVec.Normalize();
							}
							m_vecAnthorSecLevRebar.push_back({ ptStar, ptEnd });
						}
					}
				}

			}
		}
	}
}

bool CFaceTieRebarToolAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素

	// 有四层钢筋，两两求交，确定拉筋的起始点
	FaceRebarDataArray faceDataArray;
	faceDataArray.posRebarData.HRebarData.rebarSize = m_MainFirLevSizekey;
	faceDataArray.posRebarData.HRebarData.rebarSpacing = m_posSpacing1;
	faceDataArray.posRebarData.VRebarData.rebarSize = m_MainSecLevSizekey;
	faceDataArray.posRebarData.VRebarData.rebarSpacing = m_revSpacing1;

	faceDataArray.revRebarData.HRebarData.rebarSize = m_AnthorFirLevSizekey;
	faceDataArray.revRebarData.HRebarData.rebarSpacing = m_posSpacing2;
	faceDataArray.revRebarData.VRebarData.rebarSize = m_AnthorSecLevSizekey;
	faceDataArray.revRebarData.VRebarData.rebarSpacing = m_revSpacing2;

	vector<vector<DSegment3d> > vecStartEnd;

	// 取钢筋起始点
	GetStartEndPoint(vecStartEnd);

	// 转换钢筋sizeKey
	GetDiameterAddType(m_tieRebarInfo.rebarSize, m_tieRebarInfo.rebarType);
	BrString strTieRebarSize = m_tieRebarInfo.rebarSize;
	if (strTieRebarSize.Find(L"mm") != string::npos)
	{
		strTieRebarSize = strTieRebarSize.Left(strTieRebarSize.GetLength() - 2);	//删掉mm
	}
	// end

	// 配置间隔方式，如间隔开一根或几根
	TieRebarMaker tieRebarMaker(faceDataArray, vecStartEnd, (TieFaceRebarStyle)m_tieRebarInfo.tieRebarStyle, strTieRebarSize);
	tieRebarMaker.m_CallerId = GetCallerId();
	tieRebarMaker.SetCustomStyle(m_tieRebarInfo.rowInterval, m_tieRebarInfo.colInterval);
	tieRebarMaker.SetTrans(m_trans);
	// end

	vector<EditElementHandle*> vecAllSolid;
	vecAllSolid.insert(vecAllSolid.begin(), m_Holeehs.begin(), m_Holeehs.end());
	vecAllSolid.insert(vecAllSolid.end(), m_Negs.begin(), m_Negs.end());
	tieRebarMaker.SetHoles(vecAllSolid);
	tieRebarMaker.SetHoleCover(GetCover()*uor_per_mm); // 孔洞
	m_vecSetId.resize(1);
	m_vecSetId.at(0) = 0;
	tieRebarMaker.SetModeType(m_modelType);
	tieRebarMaker.SetDownVec(m_ptStart, m_ptEnd); // 所属实体的底面方向
	RebarSetTag* tag = tieRebarMaker.MakeRebarforFace(PopvecSetId().at(GetvecSetId().size() - 1),m_uFaceId,m_angle1, m_angle2, m_tieRebarInfo.tieRebarMethod, modelRef);
	if (NULL != tag)
	{
		tag->SetBarSetTag(1);
		rsetTags.Add(tag);
	}
	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}

	return true;
}

void CFaceTieRebarToolAssembly::SortVecRebar(vector<DSegment3d>& vecSeg, const CVector3D& perpendicularSortVec, const CVector3D& parallelSortVec)
{
	//先在钢筋线垂直方向做排序
	SortVecRebar(vecSeg, perpendicularSortVec);

	//再将有截断关系的钢筋线再同方向上做排序
	DVec3d compVec = DVec3d::From(parallelSortVec.x, parallelSortVec.y, parallelSortVec.z);
	compVec.Normalize();
	for (size_t i = 0; i < vecSeg.size(); ++i)
	{
		size_t swapIndex = i;
		DPoint3d lastSegStrPt = vecSeg.at(i).point[0];

		for (size_t j = i + 1; j < vecSeg.size(); ++j)
		{
			DPoint3d curSegStrPt = vecSeg.at(j).point[0];
			DVec3d vec = curSegStrPt - lastSegStrPt;
			vec.Normalize();
			if (!vec.IsParallelTo(compVec))
			{
				break;
			}
			if (vec.IsPositiveParallelTo(compVec))
			{
				continue;
			}
			else
			{
				swapIndex = j;
				lastSegStrPt = curSegStrPt;
			}
		}
		if (swapIndex != i)
		{
			swap(vecSeg[i], vecSeg[swapIndex]);
		}
	}
}

void CFaceTieRebarToolAssembly::SortVecRebar(vector<DSegment3d>& vecSeg, const CVector3D& sortVec)
{
	for (int i = 0; i < (int)vecSeg.size() - 1; i++)
	{
		for (int j = 0; j < (int)vecSeg.size() - 1 - i; j++)
		{
			DPoint3d pt1, pt2;
			vecSeg[j].GetStartPoint(pt1);
			vecSeg[j + 1].GetStartPoint(pt2);
			if (COMPARE_VALUES_EPS(sortVec.z, 0.00, EPS) != 0)
			{
				CVector3D vec = CVector3D::From(0, 0, pt2.z - pt1.z);
				vec.Normalize();
				if (COMPARE_VALUES_EPS(sortVec.z, vec.z, EPS) != 0)
				{
					DSegment3d ptTmp = vecSeg[j];
					vecSeg[j] = vecSeg[j + 1];
					vecSeg[j + 1] = ptTmp;
				}
			}
			else
			{
				CVector3D vec = CVector3D::From(pt2.x - pt1.x, pt2.y - pt1.y, 0);
				vec.Normalize();
				if (!(COMPARE_VALUES_EPS(sortVec.x, vec.x, EPS) == 0 && COMPARE_VALUES_EPS(sortVec.y, vec.y, EPS) == 0))
				{
					DSegment3d ptTmp = vecSeg[j];
					vecSeg[j] = vecSeg[j + 1];
					vecSeg[j + 1] = ptTmp;
				}
			}

		}
	}
}

void CFaceTieRebarToolAssembly::SetEndAngle(double Angle1, double Angle2)
{
	m_angle1 = Angle1;
	m_angle2 = Angle2;
}

bool CFaceTieRebarToolAssembly::push_vecPoint(vector<DSegment3d>& vecLevRebar, DPoint3d& ptstr, DPoint3d& ptend, CVector3D& vec, CVector3D& vecAnthor, DgnModelRefP modelRef)
{
	bool bFlag = false;
	if (vecLevRebar.size() > 0)
	{
		DPoint3d ptFirStr, ptFirEnd, ptStr;
		vecLevRebar[0].GetStartPoint(ptFirStr);
		vecLevRebar[0].GetEndPoint(ptFirEnd);

		CVector3D vecNormol = CVector3D::kZaxis;
		if (m_modelType == 0)
		{
			vec.Normalize();
			vecAnthor.Normalize();
			if (!vec.IsZero() && !vecAnthor.IsZero())
			{
				vecNormol = vec.CrossProduct(vecAnthor);
			}
			else
			{
				if (vec.IsEqual(CVector3D::kZaxis, EPS))
				{
					CVector3D vecTmp = m_ptEnd - m_ptStart;
					vecTmp.Normalize();
					vecNormol = vec.CrossProduct(vecTmp);
				}
				else
				{
					vecNormol = vec.CrossProduct(CVector3D::kZaxis);
				}
			}
		}

		// 计算一个点在由原点和法线 bvector 定义的平面上的投影
		mdlVec_projectPointToPlane(&ptStr, &ptstr, &ptFirStr, &vecNormol);

		//if (!EFT::IsPointInLine(ptStr, ptFirStr, ptFirEnd, modelRef, bFlag))
		//{
		//	return false;
		//}
		if (COMPARE_VALUES_EPS(ptStr.Distance(ptstr), 0.00, 10) == 0)
		{
			vecLevRebar.push_back({ ptstr, ptend });
			bFlag = true;
		}
	}
	else
	{
		vec = ptend - ptstr;
		vec.Normalize();
		vecLevRebar.push_back({ ptstr, ptend });
		bFlag = true;
	}
	return bFlag;
}

void CFaceTieRebarToolAssembly::CalaRebarStartEnd(RebarElementP rep, DPoint3d& PtStar, DPoint3d& PtEnd, double& diameter, DgnModelRefP modelRef)
{
	RebarCurve curve;
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	RebarShape * rebarshape = rep->GetRebarShape(modelRef);
	if (rebarshape == nullptr)
	{
		return;
	}

	rebarshape->GetRebarCurve(curve);
	BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
	diameter = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);

	CMatrix3D tmp3d(rep->GetLocation());
	curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
	curve.DoMatrix(rep->GetLocation());
	RebarVertices  vers = curve.PopVertices();

	double maxLenth = 0;
	for (int i = 0; i < vers.GetSize() - 1; i++)
	{
		RebarVertex   ver1 = vers.At(i);
		RebarVertex   ver2 = vers.At(i + 1);
		CPoint3D const&     pt1 = ver1.GetIP();
		CPoint3D const&     pt2 = ver2.GetIP();
		DPoint3d tpt1 = DPoint3d::From(pt1.x, pt1.y, pt1.z);
		DPoint3d tpt2 = DPoint3d::From(pt2.x, pt2.y, pt2.z);
		if (i == 0)
		{
			maxLenth = tpt1.Distance(tpt2);
			PtStar = tpt1;
			PtEnd = tpt2;
		}
		else if (maxLenth < tpt1.Distance(tpt2))
		{
			maxLenth = tpt1.Distance(tpt2);
			PtStar = tpt1;
			PtEnd = tpt2;
		}

	}
}

void CFaceTieRebarToolAssembly::GetStartEndPoint(vector<vector<DSegment3d> >& vecStartEnd)
{
	if (m_vecMainFirLevRebar.size() == 0 || m_vecMainSecLevRebar.size() == 0 ||
		m_vecAnthorFirLevRebar.size() == 0 || m_vecAnthorSecLevRebar.size() == 0)
	{
		return;
	}

	CVector3D vec = m_ptEnd - m_ptStart;
	vec.Normalize();

	m_MainLevVec.Normalize();
	m_AnthorLevVec.Normalize();

	CVector3D vecCom; // 对比方向
	if (vec.IsEqual(m_MainLevVec, EPS))
	{
		vecCom = m_AnthorLevVec.CrossProduct(vec);
	}
	else
	{
		vecCom = m_MainLevVec.CrossProduct(vec);
	}
	if (m_modelType == 1)
	{
		vecCom = CVector3D::kZaxis;
	}
	vecCom.Normalize();

	DPoint3d ptMainFirStr;
	m_vecMainFirLevRebar[0].GetStartPoint(ptMainFirStr);

	DPoint3d ptMainSecStr;
	m_vecMainSecLevRebar[0].GetStartPoint(ptMainSecStr);
	vec = ptMainSecStr - ptMainFirStr;
	vec.Normalize();

	if ((vec.DotProduct(vecCom)) > 0.9 /*vec.IsEqual(vecCom, EPS)*/)
	{
		vecStartEnd.push_back(m_vecMainFirLevRebar);
		vecStartEnd.push_back(m_vecAnthorFirLevRebar);
		vecStartEnd.push_back(m_vecAnthorSecLevRebar);
		vecStartEnd.push_back(m_vecMainSecLevRebar);
	}
	else
	{
		vecStartEnd.push_back(m_vecMainSecLevRebar);
		vecStartEnd.push_back(m_vecAnthorSecLevRebar);
		vecStartEnd.push_back(m_vecAnthorFirLevRebar);
		vecStartEnd.push_back(m_vecMainFirLevRebar);
	}
}

long CFaceTieRebarToolAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

