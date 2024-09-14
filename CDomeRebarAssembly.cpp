#include "_ustation.h"
#include "CDomeRebarAssembly.h"
#include "ExtractFacesTool.h"
#include "ElementAttribute.h"
#include "PITMSCECommon.h"
#include "CDomeRebarMainDlg.h"
#include "resource.h"
#include "XmlHelper.h"
#include "SelectRebarTool.h"

using namespace PIT;

CDomeRebarAssembly::~CDomeRebarAssembly()
{
	m_stArcCutInfo.dRadius1 = 0.0;
	m_stArcCutInfo.dRadius2 = 0.0;
	m_stArcCutInfo.dAngleOrSpace = 0.0;
	m_stArcCutInfo.dStartOffset = 0.0;

	for (auto ms : m_vecArcLine)
	{
		mdlElmdscr_freeAll(&ms);
	}
	m_vecArcLine.clear();
	m_vecVertex.clear();

	m_arcCenter = DPoint3d::From(0, 0, 0);
}


void CDomeRebarAssembly::GetDomeFeatureParam(EditElementHandleR eh, DPoint3dCR maxDomePt)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	// EFT::GetDomeCutFace(testeeh, model);
	EditElementHandle			domeCutFace; // 񷶥����,��ת����
	GetDomeCutFace(domeCutFace);

	MSElementDescrP tmpDescr = domeCutFace.GetElementDescrP();
	DPoint3d facenormal;
	mdlElmdscr_extractNormal(&facenormal, NULL, tmpDescr, NULL); // ���ڲ���Ԫ�صķ�����
	if (facenormal.z < 0)
	{
		mdlElmdscr_reverseNormal(&tmpDescr, tmpDescr, domeCutFace.GetModelRef()); // ���桢ʵ�����Ԫ�صķ��� 
	}
	// Ϊ��� EditElementHandle ����һ���µ� MSElementDescr
	// ���е� MSElementDescr��������ڣ����ͷ�
	domeCutFace.SetElementDescr(tmpDescr, true, false, domeCutFace.GetModelRef());

	//�������ɢ
	ElementAgenda agenda;//��Ŵ�ɢ֮���Ԫ��
	DropGeometryPtr pDropGeometry = DropGeometry::Create();//����һ��DropGeometryʵ�������ü���ѡ��
	if (domeCutFace.GetElementType() == CMPLX_SHAPE_ELM) // ��ȡ�� ElementHandle ���õ�Ԫ�ص�Ԫ������
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Complex);
	}
	else if (domeCutFace.GetElementType() == SURFACE_ELM || domeCutFace.GetElementType() == CELL_HEADER_ELM)
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_Solids);
	}
	else
	{
		pDropGeometry->SetOptions(DropGeometry::Options::OPTION_LinearSegments);
	}
	if (SUCCESS != domeCutFace.GetDisplayHandler()->Drop(domeCutFace, agenda, *pDropGeometry)) // ����Ԫ�طŵ�һ�飨���򵥵ģ�ԭʼԪ���� 
	{
		agenda.Clear();
		mdlOutput_printf(MSG_STATUS, L"ͬ���ͼʱ��ɢǽ�ĵ���ʧ�ܣ�");
		return ;
	}
	vector<MSElementDescrP> vecCutLine;
	for (EditElementHandleR LineEeh : agenda)
	{
		//�鿴��Ԫ�صĳ���
		double LineLength;
		// ��ȡԪ�صĻ�����������
		mdlMeasure_linearProperties(&LineLength, NULL, NULL, NULL, NULL, NULL, NULL, NULL, LineEeh.GetElementDescrP(), 0);
		// ����1mm������Ԫ�ش���vecLines
		if (LineLength * 1000 / UOR_PER_METER > 1)
		{
			// ��ȡ��ȡ����� EditElementHandle ������ MSElementDescr ������Ȩ 
			vecCutLine.push_back(LineEeh.ExtractElementDescr());
		}
	}

	vector<MSElementDescrP> vecArcTmp;
	for (auto msdescrp : vecCutLine)
	{
		int lineType = mdlElement_getType(&msdescrp->el);
		if (msdescrp->el.ehdr.type == ARC_ELM)
		{
			vecArcTmp.push_back(msdescrp);
		}
		else
		{
			mdlElmdscr_freeAll(&msdescrp);
		}
	}

	if ((int)vecArcTmp.size() != 3)
	{
		return;
	}

	vector<MSElementDescrP>  vecTmp;
	for (int i = 0; i < (int)vecArcTmp.size(); i++)
	{
		DPoint3d minP, maxP;
		MSElementDescrP desCrp = vecArcTmp.at(i);
		mdlElmdscr_computeRange(&minP, &maxP, desCrp, NULL);
		if (COMPARE_VALUES_EPS(maxP.z, maxDomePt.z, uor_per_mm) == 0)
		{
			// �⻡
			m_vecArcLine.push_back(desCrp);
			continue;
		}
		vecTmp.push_back(desCrp);
	}

	DPoint3d minPFir, maxPFir;
	mdlElmdscr_computeRange(&minPFir, &maxPFir, vecTmp.at(0), NULL);

	DPoint3d minPSec, maxPSec;
	mdlElmdscr_computeRange(&minPSec, &maxPSec, vecTmp.at(1), NULL);

	if (COMPARE_VALUES_EPS(maxPFir.z, maxPSec.z, EPS) > 0)
	{
		// �ڻ�
		m_vecArcLine.push_back(vecTmp[0]);
		mdlElmdscr_freeAll(&vecTmp[1]);
	}
	else
	{	// �ڻ�
		m_vecArcLine.push_back(vecTmp[1]);
		mdlElmdscr_freeAll(&vecTmp[0]);
	}

	if ((int)m_vecArcLine.size() > 1)
	{
		double starR, sweepR;
		RotMatrix rotM;
		DPoint3d ptCenter;
		mdlArc_extract(NULL, &starR, &sweepR, &m_outerArcRadius, NULL, &rotM, &ptCenter, &m_vecArcLine.at(0)->el);
		mdlArc_extract(NULL, &starR, &sweepR, &m_innerArcRadius, NULL, &rotM, &ptCenter, &m_vecArcLine.at(1)->el);

		m_arcCenter = ptCenter;
	}

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());

	return;
}


// ȡ񷶥�ĺ����
void CDomeRebarAssembly::GetDomeCutFace(EditElementHandleR domeCutFace)
{

	DgnModelRefP model = ACTIVEMODEL;
	// MSElementDescr  *pDescrExtrusion = NULL;
	//����������
	ChainHeaderHandler::CreateChainHeaderElement(domeCutFace, NULL, true, true, *model);

	double missDis = 10000;
	double dRadius;
	DPoint3d fisrpt;
	DPoint3d ptVertex1;
	DPoint3d ptVertex2;
	DPoint3d ptVertex3;
	for (size_t i = 0; i < m_vecVertex.size(); ++i)
	{
		dRadius = m_vecVertex[i].dRadius;
		ptVertex2 = m_vecVertex[i].ptVertex;
		if (0 == i)			//��һ����
		{
			ptVertex1 = m_vecVertex[m_vecVertex.size() - 1].ptVertex;
		}
		if (i == m_vecVertex.size() - 1)	//���һ����
		{
			ptVertex3 = m_vecVertex[0].ptVertex;
		}
		else
		{
			ptVertex3 = m_vecVertex[i + 1].ptVertex;
		}
		if ((COMPARE_VALUES(m_vecVertex[i].dRadius, 0) == 0))			//��ǰ�㲻�ǻ�����һ����Ҳ���ǻ��Ļ�ֱ�ӻ���
		{
			EditElementHandle eehLine;
			if ((i == m_vecVertex.size() - 1) && COMPARE_VALUES(m_vecVertex[0].dRadius, 0) != 0)
			{
				double lenth = mdlVec_distance(&ptVertex1, &ptVertex2);	//�ж������Ƿ�Ϊ�غϵ㣬���Ϊ�غϵ㣬����Ҫ����
				if (COMPARE_VALUES(lenth, missDis) > 0)
				{
					if (SUCCESS == LineHandler::CreateLineElement(eehLine, NULL, DSegment3d::From(ptVertex1, ptVertex2), true, *model))
					{
						ChainHeaderHandler::AddComponentElement(domeCutFace, eehLine);
						//mdlElmdscr_initOrAddToChain(&pDescrExtrusion, eehLine.ExtractElementDescr());
					}
				}
				lenth = mdlVec_distance(&fisrpt, &ptVertex2);	//�ж������Ƿ�Ϊ�غϵ㣬���Ϊ�غϵ㣬����Ҫ����
				if (COMPARE_VALUES(lenth, missDis) > 0)
				{
					if (SUCCESS == LineHandler::CreateLineElement(eehLine, NULL, DSegment3d::From(ptVertex2, fisrpt), true, *model))
					{
						ChainHeaderHandler::AddComponentElement(domeCutFace, eehLine);
						//mdlElmdscr_initOrAddToChain(&pDescrExtrusion, eehLine.ExtractElementDescr());
					}
				}
			}
			else
			{
				if ((i != 0) || COMPARE_VALUES(m_vecVertex[m_vecVertex.size() - 1].dRadius, 0) == 0)			//��һ����
				{
					double lenth = mdlVec_distance(&ptVertex1, &ptVertex2);	//�ж������Ƿ�Ϊ�غϵ㣬���Ϊ�غϵ㣬����Ҫ����
					if (COMPARE_VALUES(lenth, missDis) > 0)
					{
						if (SUCCESS == LineHandler::CreateLineElement(eehLine, NULL, DSegment3d::From(ptVertex1, ptVertex2), true, *model))
						{
							ChainHeaderHandler::AddComponentElement(domeCutFace, eehLine);
							//mdlElmdscr_initOrAddToChain(&pDescrExtrusion, eehLine.ExtractElementDescr());
						}
					}
				}

				ptVertex1 = ptVertex2;
			}
		}
		else
		{
			if (COMPARE_VALUES(m_vecVertex[i].dRadius, 0))
			{
				EditElementHandle eehArc;
				DPoint3d ptArcBegin;
				DPoint3d ptArcEnd;
				//�ܻ���Բ���ͻ���Բ�������ܻ���Բ���ͻ�����
				if (SUCCESS == IntersectionPointToArc_Dome(eehArc, ptArcBegin, ptArcEnd, ptVertex2, ptVertex1, ptVertex3, dRadius, model))
				{
					//����Բ���������Բ���������յ㣬��ʱ��Ҫ����һ��������㣬��һ�������յ㻭��
					if (i != 0)
					{
						EditElementHandle eehLine1;
						double lenth = mdlVec_distance(&ptVertex1, &ptArcBegin);	//�ж������Ƿ�Ϊ�غϵ㣬���Ϊ�غϵ㣬����Ҫ����
						if (COMPARE_VALUES(lenth, missDis) > 0)
						{
							if (SUCCESS == LineHandler::CreateLineElement(eehLine1, NULL, DSegment3d::From(ptVertex1, ptArcBegin), true, *model))
							{
								ChainHeaderHandler::AddComponentElement(domeCutFace, eehLine1);
								//mdlElmdscr_initOrAddToChain(&pDescrExtrusion, eehLine1.ExtractElementDescr());
							}
						}
					}
					else
					{
						fisrpt = ptArcBegin;
					}
					ChainHeaderHandler::AddComponentElement(domeCutFace, eehArc);
					//mdlElmdscr_initOrAddToChain(&pDescrExtrusion, eehArc.ExtractElementDescr());
					if (i == m_vecVertex.size() - 1)
					{
						EditElementHandle eehLine2;
						if (COMPARE_VALUES(m_vecVertex[i].dRadius, 0) != 0)			//��һ����
						{
							double lenth = mdlVec_distance(&ptArcEnd, &m_vecVertex[0].ptVertex);	//�ж������Ƿ�Ϊ�غϵ㣬���Ϊ�غϵ㣬����Ҫ����
							if (COMPARE_VALUES(lenth, missDis) > 0)
							{
								if (SUCCESS == LineHandler::CreateLineElement(eehLine2, NULL, DSegment3d::From(ptArcEnd, m_vecVertex[0].ptVertex), true, *model))
								{
									ChainHeaderHandler::AddComponentElement(domeCutFace, eehLine2);
									//mdlElmdscr_initOrAddToChain(&pDescrExtrusion, eehLine2.ExtractElementDescr());
								}
							}
						}
						else
						{
							double lenth = mdlVec_distance(&ptArcEnd, &fisrpt);	//�ж������Ƿ�Ϊ�غϵ㣬���Ϊ�غϵ㣬����Ҫ����
							if (COMPARE_VALUES(lenth, missDis) > 0)
							{
								if (SUCCESS == LineHandler::CreateLineElement(eehLine2, NULL, DSegment3d::From(ptArcEnd, fisrpt), true, *model))
								{
									ChainHeaderHandler::AddComponentElement(domeCutFace, eehLine2);
									//mdlElmdscr_initOrAddToChain(&pDescrExtrusion, eehLine2.ExtractElementDescr());
								}
							}
						}
					}
					ptVertex1 = ptArcEnd;
				}
			}
		}
	}

	//TransformInfo transInfo(targetTrans);
	//domeCutFace.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(domeCutFace, transInfo);
	// m_domeCutFace.AddToModel();
}


// XY�����ֽ�
RebarSetTag* CDomeRebarAssembly::MakeRebars_XY(ElementId & rebarSetId, BrString sizeKey, vector<MSElementDescrP>& vecArcLine, DgnModelRefP modelRef, double levelSpacing, bool bFlag)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	vector<PIT::EndType> endType; // �˲���ʽ
	PIT::EndType endFir;
	PIT::EndType endSec;
	endFir.rotateAngle = 0.00;
	endFir.endType = 0;
	endFir.offset = 0.00;

	endSec.rotateAngle = 0.00;
	endSec.endType = 0;
	endSec.offset = 0.00;

	endType.push_back(endFir);
	endType.push_back(endSec);

	vector<CVector3D>  vecEndNormal; // �˲��乳����
	CVector3D vecFir = CVector3D::From(0, 0, 0);
	CVector3D vecSec = CVector3D::From(0, 0, 0);
	vecEndNormal.push_back(vecFir);
	vecEndNormal.push_back(vecSec);

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);

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

	// XY��ʽ��ֱ�����������ۣ������յ���������ĵ�
	makeXYRebarCurve(rebarCurvesNum, endTypes, modelRef, levelSpacing, diameter, bFlag);

	int numRebar = (int)rebarCurvesNum.size();

	vector<DSegment3d> vecStartEnd;
	for (int j = 0; j < (int)rebarCurvesNum.size(); ++j)
	{
		PITRebarCurve rebarCurve = rebarCurvesNum[j];
		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
		}
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(100.00);
	setdata.SetAverageSpacing(100.00);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}


// ����Բ�θֽ�
RebarSetTag* CDomeRebarAssembly::MakeRebars_Round(ElementId & rebarSetId, BrString sizeKey, vector<MSElementDescrP>& vecArcLine, DgnModelRefP modelRef, double levelSpacing)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	vector<PIT::EndType> endType; // �˲���ʽ
	PIT::EndType endFir;
	PIT::EndType endSec;
	endFir.rotateAngle = 0.00;
	endFir.endType = 0;
	endFir.offset = 0.00;

	endSec.rotateAngle = 0.00;
	endSec.endType = 0;
	endSec.offset = 0.00;

	endType.push_back(endFir);
	endType.push_back(endSec);

	vector<CVector3D>  vecEndNormal; // �˲��乳����
	CVector3D vecFir = CVector3D::From(0, 0, 0);
	CVector3D vecSec = CVector3D::From(0, 0, 0);
	vecEndNormal.push_back(vecFir);
	vecEndNormal.push_back(vecSec);

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);

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

	makeRoundRebarCurve(rebarCurvesNum, m_vecArcPoint, endTypes, modelRef, levelSpacing, diameter);

	int numRebar = (int)rebarCurvesNum.size();

	vector<DSegment3d> vecStartEnd;
	for (int j = 0; j < (int)rebarCurvesNum.size(); ++j)
	{
		PITRebarCurve rebarCurve = rebarCurvesNum[j];
		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
		}
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(100.00);
	setdata.SetAverageSpacing(100.00);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}


// ���ɻ��θֽ�
RebarSetTag* CDomeRebarAssembly::MakeRebars_Arc(ElementId & rebarSetId, BrString sizeKey, vector<MSElementDescrP>& vecArcLine, DgnModelRefP modelRef, double levelSpacing)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	vector<PIT::EndType> endType; // �˲���ʽ
	PIT::EndType endFir;
	PIT::EndType endSec;
	endFir.rotateAngle = 0.00;
	endFir.endType = 0;
	endFir.offset = 0.00;

	endSec.rotateAngle = 0.00;
	endSec.endType = 0;
	endSec.offset = 0.00;

	endType.push_back(endFir);
	endType.push_back(endSec);

	vector<CVector3D>  vecEndNormal; // �˲��乳����
	CVector3D vecFir = CVector3D::From(0, 0, 0);
	CVector3D vecSec = CVector3D::From(0, 0, 0);
	vecEndNormal.push_back(vecFir);
	vecEndNormal.push_back(vecSec);

	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kNone);
	endTypeEnd.SetType(RebarEndType::kNone);

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;

	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);

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

	MSElementDescrP msTmp = NULL;
	// 񷶥�����⻡
	mdlElmdscr_duplicate(&msTmp, vecArcLine.at(0));
	// end
	makeArcRebarCurve(rebarCurvesNum, msTmp, endTypes, levelSpacing, diameter, m_stArcCutInfo.dStartOffset);
	mdlElmdscr_freeAll(&msTmp);

	int numRebar = (int)rebarCurvesNum.size();

	vector<DSegment3d> vecStartEnd;
	for (int j = 0; j < (int)rebarCurvesNum.size(); ++j)
	{
		PITRebarCurve rebarCurve = rebarCurvesNum[j];
		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
		}
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(100.00);
	setdata.SetAverageSpacing(100.00);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}


// XY������ʽ���
bool CDomeRebarAssembly::MakeRebarXYIntersect(map<int, vector<PIT::DomeLevelDetailInfo>>::iterator itr, RebarSetTagArray& rsetTags, RebarSetTag* tag, DgnModelRefP modelRef, int& nTagIndex)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // ÿ��λ����

	vector <PIT::DomeLevelDetailInfo> vecLevelDetail = itr->second;

	double levelSpacing = 0.00; // ��ǰ��ļ��
	double totolSpacing = m_outerArcRadius - m_innerArcRadius; // �ڻ����⻡���
	double totalDiameter = 0.00;
	levelSpacing += m_Cover * uor_per_mm; // ���ϱ�����

	for (int i = 0; i < (int)vecLevelDetail.size(); i++)
	{
		m_vecSetId.push_back(0);

		// ת��sizeKey
		BrString sizeKey = vecLevelDetail.at(i).rebarSize;
		GetDiameterAddType(sizeKey, vecLevelDetail.at(i).rebarType);
		double diamter = RebarCode::GetBarDiameter(sizeKey, modelRef);
		// end

		// �ж���ǰ��ļ����û�д����ܼ��
		if (COMPARE_VALUES_EPS(levelSpacing + vecLevelDetail.at(i).dSpacing * uor_per_mm, totolSpacing, EPS) > 0)
		{
			// ���Ϻ����ĸֽ�ֱ��
			totalDiameter = 0.00;
			for (int j = i; j < (int)vecLevelDetail.size(); j++)
			{
				BrString sizeKey = vecLevelDetail.at(j).rebarSize;
				totalDiameter += RebarCode::GetBarDiameter(sizeKey, modelRef);
			}
			levelSpacing = totolSpacing - levelSpacing - m_Cover - totalDiameter;
		}
		else
		{
			levelSpacing += vecLevelDetail.at(i).dSpacing * uor_per_mm;
		}
		// ���ϵ�ǰ��İ뾶
		levelSpacing += diamter * 0.5;

		m_stArcCutInfo.dAngleOrSpace = vecLevelDetail.at(i).dAngleOrSpace;
		m_stArcCutInfo.dStartOffset = vecLevelDetail.at(i).dStartOffset;
		bool bFlag = vecLevelDetail.at(i).rebarShape == 0 ? true : false;
		tag = MakeRebars_XY(m_vecSetId[nTagIndex - 1], sizeKey, m_vecArcLine, modelRef, levelSpacing, bFlag);

		if (tag != NULL)
		{
			tag->SetBarSetTag(nTagIndex);
			rsetTags.Add(tag);
			nTagIndex++;
		}

		// ���ϵ�ǰ��İ뾶�����൱��������һ������
		levelSpacing += diamter * 0.5;
	}

	return true;
}


// ��������
bool CDomeRebarAssembly::MakeRebarHoopOrRadial(map<int, vector<PIT::DomeLevelDetailInfo>>::iterator itr, RebarSetTagArray& rsetTags, RebarSetTag* tag, DgnModelRefP modelRef, int& nTagIndex)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // ÿ��λ����

	vector <PIT::DomeLevelDetailInfo> vecLevelDetail = itr->second;

	double levelSpacing = 0.00; // ��ǰ����
	double totolSpacing = m_outerArcRadius - m_innerArcRadius; // �ڻ����⻡���
	double totalDiameter = 0.00;

	levelSpacing += m_Cover * uor_per_mm; // ���ϱ�����

	for (int i = 0; i < (int)vecLevelDetail.size(); i++)
	{
		m_vecSetId.push_back(0);

		// ת��sizeKey
		BrString sizeKey = vecLevelDetail.at(i).rebarSize;
		GetDiameterAddType(sizeKey, vecLevelDetail.at(i).rebarType);
		double diamter = RebarCode::GetBarDiameter(sizeKey, modelRef);
		// end

		// �ж���ǰ��ļ����û�д����ܼ��
		if (COMPARE_VALUES_EPS(levelSpacing + vecLevelDetail.at(i).dSpacing * uor_per_mm, totolSpacing, EPS) > 0)
		{
			// ���Ϻ����ĸֽ�ֱ��
			totalDiameter = 0.00;
			for (int j = i; j < (int)vecLevelDetail.size(); j++)
			{
				BrString sizeKey = vecLevelDetail.at(j).rebarSize;
				totalDiameter += RebarCode::GetBarDiameter(sizeKey, modelRef);
			}
			levelSpacing = totolSpacing - levelSpacing - m_Cover - totalDiameter;
			// end
		}
		else
		{
			levelSpacing += vecLevelDetail.at(i).dSpacing * uor_per_mm;
		}

		// ���ϵ�ǰ��İ뾶
		levelSpacing += diamter * 0.5;

		m_roundFlag = false;
		m_stArcCutInfo.dAngleOrSpace = vecLevelDetail.at(i).dAngleOrSpace;
		m_stArcCutInfo.dStartOffset = vecLevelDetail.at(i).dStartOffset;
		if (vecLevelDetail.at(i).rebarShape == 1)
		{
			// ���ɻ��θֽ�
			tag = MakeRebars_Arc(m_vecSetId[nTagIndex - 1], sizeKey, m_vecArcLine, modelRef, levelSpacing);
		}
		else
		{
			// ����һ�����õĻ����뵱ǰˮƽԲ�εĽ��㣬ȷ��Բ�θֽ��ˮƽ����ʱ�Ĵ�ֱ����
			double starR, sweepR;
			double radius;
			DPoint3d ArcDPs[2];
			RotMatrix rotM;
			DPoint3d centerpt;
			mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &m_vecArcLine.at(0)->el);

			DPoint3d arcStr = ArcDPs[0];
			DPoint3d arcEnd = ArcDPs[1];

			// ƫ�ƻ��ߣ������ĵ�ƫ��ָ������
			DPoint3d arcMid = DPoint3d::From(0, 0, 0);
			TranseArcSide(centerpt, arcStr, arcMid, arcEnd, (0 - levelSpacing));
			// end

			// ����Բ������������
			if (!IntersectRound(centerpt, arcStr, arcMid, arcEnd))
			{
				continue;
			}

			EditElementHandle eehArc;
			//��ptAΪԲ��,dRadiusΪ�뾶��ptIn1Ϊ������㣬ptIn2Ϊ�����յ㻭��
			ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(centerpt, arcStr, arcEnd), true, *ACTIVEMODEL);

			double dis2 = 0.00;

			mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, eehArc.GetElementDescrP(), &arcEnd, 0.1);

			dis2 /= 2;

			mdlElmdscr_pointAtDistance(&arcMid, nullptr, dis2, eehArc.GetElementDescrP(), 0.1);

			m_dZLevelStart = arcStr.z;
			m_dZLevelEnd = arcEnd.z;
			// end

			m_vecArcPoint.clear();
			m_vecArcPoint.push_back({ arcStr, arcMid, arcEnd, centerpt, 0.00 });
			m_roundFlag = true;

			// ����Բ�θֽ�
			tag = MakeRebars_Round(m_vecSetId[nTagIndex - 1], sizeKey, m_vecArcLine, modelRef, levelSpacing);
		}
		if (tag != NULL)
		{
			tag->SetBarSetTag(nTagIndex);
			rsetTags.Add(tag);
			nTagIndex++;
		}
		// ���ϵ�ǰ��İ뾶�����൱��������һ������
		levelSpacing += diamter * 0.5;
	}

	m_vecArcPoint.clear();
	return true;
}


bool CDomeRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // ÿ��λ����

	RebarSetTag* tag = NULL;

	if (m_mapDomeLevelDetailInfo.empty() || m_vecDomeLevelInfo.empty() || m_vecArcLine.empty())
	{
		return true;
	}

	m_vecSetId.clear();
	int nTagIndex = 1;
	for (int nIndex = 0; nIndex < m_vecDomeLevelInfo.size(); nIndex++)
	{
		PIT::DomeLevelInfo stDomeLevelInfo = m_vecDomeLevelInfo.at(nIndex);
		auto itr = m_mapDomeLevelDetailInfo.find(stDomeLevelInfo.nNumber);
		if (itr != m_mapDomeLevelDetailInfo.end())
		{
			m_stArcCutInfo.dRadius1 = stDomeLevelInfo.dRadius1;
			m_stArcCutInfo.dRadius2 = stDomeLevelInfo.dRadius2;
			
			if (stDomeLevelInfo.nLayoutType == 1) // ��������
			{
				MakeRebarHoopOrRadial(itr, rsetTags, tag, modelRef, nTagIndex);
			}
			else if (stDomeLevelInfo.nLayoutType == 0) // XY����
			{
				MakeRebarXYIntersect(itr, rsetTags, tag, modelRef, nTagIndex);
			}
		}
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

// ���������м�ƫ��ָ������
void CDomeRebarAssembly::TranseArcSide(DPoint3dR centerPt, DPoint3dR begPt, DPoint3dR midPt, DPoint3dR endPt, double reduceRadius)
{
	CVector3D vec = begPt - centerPt;
	vec.Normalize();
	if (COMPARE_VALUES_EPS(reduceRadius, 0.00, EPS) < 0)
	{
		vec.Negate();
	}
	vec.ScaleToLength(abs(reduceRadius));
	begPt.Add(vec);

	vec = midPt - centerPt;
	vec.Normalize();
	if (COMPARE_VALUES_EPS(reduceRadius, 0.00, EPS) < 0)
	{
		vec.Negate();
	}
	vec.ScaleToLength(abs(reduceRadius));
	midPt.Add(vec);

	vec = endPt - centerPt;
	vec.Normalize();
	if (COMPARE_VALUES_EPS(reduceRadius, 0.00, EPS) < 0)
	{
		vec.Negate();
	}
	vec.ScaleToLength(abs(reduceRadius));
	endPt.Add(vec);

	return;
}


DPoint3d CDomeRebarAssembly::GetIntersectRound(DPoint3d roundCenterPt, DPoint3dR centerPtr, DPoint3dR arcStr, DPoint3dR arcMid, DPoint3dR arcEnd, DgnModelRefP modelRef)
{
	DPoint3d intersecPt = DPoint3d::From(0, 0, 0);
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // ÿ��λ����
	// 񷶥�ڻ�Բ
	EditElementHandle eehRound;
	if (SUCCESS != EllipseHandler::CreateEllipseElement(eehRound, NULL, roundCenterPt, roundCenterPt.Distance(arcEnd) + 100 * uor_per_mm, roundCenterPt.Distance(arcEnd) + 100 * uor_per_mm, 0, true, *modelRef))
	{
		return intersecPt;
	}

	EditElementHandle eehArc;
	//��ptAΪԲ��,dRadiusΪ�뾶��ptIn1Ϊ������㣬ptIn2Ϊ�����յ㻭��
	ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(centerPtr, arcStr, arcEnd), true, *ACTIVEMODEL);

	vector<DPoint3d> intersectPts;
	if (EFT::IntersectForLineAndFace(intersectPts, eehArc, eehRound) != SUCCESS)
	{
		eehArc.AddToModel();
		eehRound.AddToModel();
		return intersecPt;
	}

	if (!intersectPts.empty())
	{
		intersecPt = intersectPts.at(0);
		return intersecPt;
	}

	return intersecPt;
}


// ����Բ������������
bool CDomeRebarAssembly::IntersectRound(DPoint3dR centerPtr, DPoint3dR arcStr, DPoint3dR arcMid, DPoint3dR arcEnd)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // ÿ��λ����
	// 񷶥�ڻ�Բ
	EditElementHandle eehRound;
	if (SUCCESS != ArcHandler::CreateArcElement(eehRound, NULL, m_circleCenter, (m_stArcCutInfo.dRadius1 + m_Cover) * uor_per_mm, (m_stArcCutInfo.dRadius1 + m_Cover) * uor_per_mm, 0, 0, 2 * PI, true, *ACTIVEMODEL))
	{
		return false;
	}
	 
	// ����������
	DPoint3d pt = m_circleCenter;
	pt.z -= m_domeHight;
	MSElementDescrP pSolidDescr;
	mdlSurface_project(&pSolidDescr, eehRound.GetElementDescrP(), &m_circleCenter, &pt, NULL);

	EditElementHandle eehArc;
	//��ptAΪԲ��,dRadiusΪ�뾶��ptIn1Ϊ������㣬ptIn2Ϊ�����յ㻭��
	ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(centerPtr, arcStr, arcEnd), true, *ACTIVEMODEL);

	// Բ��
	EditElementHandle eehSolid(pSolidDescr, true, true, ACTIVEMODEL);

	vector<DPoint3d> intersectPts;
	if (EFT::IntersectForLineAndFace(intersectPts, eehArc, eehSolid) != SUCCESS)
	{
		return false;
	}

	if (!intersectPts.empty())
	{
		arcStr = intersectPts.at(0);
	}

	// 񷶥�ڻ�Բ
	EditElementHandle eehRoundTmp;
	DPoint3d ptTmp = m_circleCenter;
	ptTmp.z = arcEnd.z;
	double  roudRadius = (m_stArcCutInfo.dRadius2 - m_Cover) * uor_per_mm;
	if (COMPARE_VALUES_EPS(roudRadius, ptTmp.Distance(arcEnd) - m_Cover * uor_per_mm, EPS) > 0)
	{
		roudRadius = ptTmp.Distance(arcEnd) - m_Cover * uor_per_mm;
	}

	if (SUCCESS != ArcHandler::CreateArcElement(eehRoundTmp, NULL, m_circleCenter, roudRadius, roudRadius, 0, 0, 2 * PI, true, *ACTIVEMODEL))
	{
		return false;
	}
	// ����������
	pt = m_circleCenter;
	pt.z -= m_domeHight;
	MSElementDescrP pSolidDescrTmp;
	mdlSurface_project(&pSolidDescrTmp, eehRoundTmp.GetElementDescrP(), &m_circleCenter, &pt, NULL);

	// Բ��
	EditElementHandle eehSolidTmp(pSolidDescrTmp, true, true, ACTIVEMODEL);

	intersectPts.clear();
	if (EFT::IntersectForLineAndFace(intersectPts, eehArc, eehSolidTmp) != SUCCESS)
	{
		return false;
	}
	if (!intersectPts.empty())
	{
		arcEnd = intersectPts.at(0);
	}

	return true;
}


void CDomeRebarAssembly::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}


// bFlag : �Ƿ���X�ֽ�
bool CDomeRebarAssembly::makeXYRebarCurve(vector<PIT::PITRebarCurve>& rebar, PIT::PITRebarEndTypes& endTypes, DgnModelRefP modelRef, double levelSpacing, double diameter, bool bFlag)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	DPoint3d circleCenter = m_circleCenter;
	circleCenter.z -= levelSpacing;

	DPoint3d ptStr = circleCenter;
	CVector3D vec = DPoint3d::From(-1, 0, 0);
	if (!bFlag)
	{
		vec = DPoint3d::From(0, 1, 0);
	}

	double spacing = m_stArcCutInfo.dAngleOrSpace * uor_per_mm;
	double adjustedXLen = 2700.0 * uor_per_mm - diameter * 0.5;
	int numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
	spacing = adjustedXLen / (numRebar - 1);

	// �ֽ��
	movePoint(vec, ptStr, 2700.0 * uor_per_mm);
	DPoint3d ptEnd = circleCenter;
	movePoint(vec, ptEnd, 2700.0 * uor_per_mm, false);
	// end

	// 񷶥�ڻ�Բ  2700 ͼֽ�����
	EditElementHandle eehRound;
	if (SUCCESS != EllipseHandler::CreateEllipseElement(eehRound, NULL, m_circleCenter, 2700.0 * uor_per_mm, 2700.0 * uor_per_mm, 0, true, *modelRef))
	{
		return false;
	}

	// ����Բ��
	EditElementHandle SolidElement;
	ISolidKernelEntityPtr RoundentityPtr;
	if (SolidUtil::Convert::ElementToBody(RoundentityPtr, eehRound) == SUCCESS)
	{
		if (SolidUtil::Modify::ThickenSheet(RoundentityPtr, 0.0, m_domeHight) == SUCCESS)
		{
			SolidUtil::Convert::BodyToElement(SolidElement, *RoundentityPtr, nullptr, *modelRef);
		}
	}

	bvector<DPoint3d> allpts;
	DPoint3d ptStrBack = ptStr;
	DPoint3d ptEndBack = ptEnd;
	for (int i = 0; i <  2 * numRebar - 1; i++)
	{
		double dSideCover = 0.0 * uor_per_mm; // ��Ԥ��������
		Transform matrix;
		mdlTMatrix_getIdentity(&matrix);
		vector<vector<DPoint3d>> vecPtRebars;
		vector<DPoint3d> tmpptsTmp;
		vecPtRebars.clear();
		tmpptsTmp.clear();

		// ������Բ������
		GetIntersectPointsWithOldElm(tmpptsTmp, &SolidElement, ptStr, ptEnd, dSideCover, matrix);

		if (tmpptsTmp.size() > 1)
		{
			// ���ڽ���Ϊ�������ϵ����
			GetIntersectPointsWithSlabRebar(vecPtRebars, tmpptsTmp, ptStr, ptEnd, &SolidElement, dSideCover);
		}

		if (tmpptsTmp.size() < 2 && vecPtRebars.size() == 0)
		{
			vector<DPoint3d> vecPt;
			vecPt.push_back(ptStr);
			vecPt.push_back(ptEnd);

			vecPtRebars.push_back(vecPt);
		}

		for (vector<vector<DPoint3d>>::iterator itr = vecPtRebars.begin(); itr != vecPtRebars.end(); itr++)
		{
			allpts.clear();
			
			DPoint3d ptTmp = itr->at(0);
			movePoint(ptTmp - circleCenter, ptTmp, 1605.0 * uor_per_mm);

			if (COMPARE_VALUES_EPS(levelSpacing, (m_outerArcRadius - m_innerArcRadius - 2 * m_Cover) * 0.5, EPS) > 0)
			{
				ptTmp.z += 150.0 * uor_per_mm;
			}
			else
			{
				ptTmp.z -= 150.0 * uor_per_mm;
			}
			allpts.push_back(ptTmp);
			allpts.push_back(itr->at(0));
			allpts.push_back(itr->at(1));

			ptTmp = itr->at(1);
			movePoint(ptTmp - circleCenter, ptTmp, 1605.0 * uor_per_mm);

			if (COMPARE_VALUES_EPS(levelSpacing, (m_outerArcRadius - m_innerArcRadius - 2 * m_Cover) * 0.5, EPS) > 0)
			{
				ptTmp.z += 150.0 * uor_per_mm;
			}
			else
			{
				ptTmp.z -= 150.0 * uor_per_mm;
			}
			allpts.push_back(ptTmp);

			RebarVertices  vers;
			GetRebarVerticesFromPoints(vers, allpts, diameter);
			PITRebarCurve rebarTmp;
			rebarTmp.SetVertices(vers);
			if (allpts.size() > 2)
			{
				rebarTmp.EvaluateEndTypesStirrup(endTypes);
			}
			else
			{
				rebarTmp.EvaluateEndTypes(endTypes);
			}
			rebar.push_back(rebarTmp);
		}

		// �������һ��
		if (i == numRebar - 1)
		{
			ptStr = ptStrBack;
			ptEnd = ptEndBack;
		}
		if (bFlag)
		{
			CVector3D vecMove = (i >= numRebar - 1) ? CVector3D::kYaxis : CVector3D::From(0, -1, 0);
			movePoint(vecMove, ptStr, spacing);
			movePoint(vecMove, ptEnd, spacing);

		}
		else
		{
			CVector3D vecMove = (i >= numRebar - 1) ? CVector3D::kXaxis : CVector3D::From(-1, 0, 0);
			movePoint(vecMove, ptStr, spacing);
			movePoint(vecMove, ptEnd, spacing);
		}
	}
	return true;
}


// vecArcPoint(��һ�㻡�θֽ������)
bool CDomeRebarAssembly::makeRoundRebarCurve(vector<PIT::PITRebarCurve>& rebar, vector<arcDefinePoint>& vecArcPoint, PIT::PITRebarEndTypes& endTypes, DgnModelRefP modelRef, double levelSpacing, double diameter)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	EditElementHandle eehArc;
	//��ptAΪԲ��,dRadiusΪ�뾶��ptIn1Ϊ������㣬ptIn2Ϊ�����յ㻭��
	ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(vecArcPoint.at(0).centerPtr, vecArcPoint.at(0).ptStr, vecArcPoint.at(0).ptEnd), true, *ACTIVEMODEL);

	double dis1 = 0.00;
	mdlElmdscr_distanceAtPoint(&dis1, nullptr, nullptr, eehArc.GetElementDescrP(), &vecArcPoint.at(0).ptEnd, 0.1); // ���ĳ���
	double dArcRadius = vecArcPoint.at(0).ptStr.Distance(vecArcPoint.at(0).centerPtr); // ���뾶
	double arcAngle = dis1 / dArcRadius; // ���ĽǶ�

	double dSpace = (m_stArcCutInfo.dAngleOrSpace * PI) / 180.0;
	int nRebarNum = (int)floor(arcAngle / dSpace + 0.5) + 1;
	dSpace = arcAngle / (nRebarNum - 1);
	
	DPoint3d	centerPoint = m_circleCenter;
	centerPoint.z = m_dZLevelStart; // Բ�ĵ�
	if (vecArcPoint.size() == 0)
	{
		return false;
	}
	for (int i = 0; i < nRebarNum; i++)
	{
		// Բ�θֽ�ͻ��θֽ���ཻ��
		DPoint3d arcStr = GetIntersectRound(centerPoint, vecArcPoint.at(0).centerPtr, vecArcPoint.at(0).ptStr, vecArcPoint.at(0).ptMid, vecArcPoint.at(0).ptEnd, modelRef);

		if (arcStr == DPoint3d::From(0, 0, 0))
		{
			continue;
		}

		double dZBack = arcStr.z;

		CVector3D vec;
		if (!m_roundFlag) // Բ�θֽ��ڵ�һ��ʱ
		{
			// ƫ�Ƹֽ�ߴ����
			vec = vecArcPoint.at(0).centerPtr - arcStr;
			vec.Normalize();
			movePoint(vec, arcStr, vecArcPoint.at(0).diameter * 0.5 + diameter * 0.5);
			centerPoint.z = arcStr.z;
		}

		vec = arcStr - centerPoint;
		vec.Normalize();
		movePoint(vec, arcStr, diameter * 0.5);

		// ����һ�λ�
		vec = arcStr - centerPoint;
		vec.Normalize();
		CVector3D vecNormal = vec.Perpendicular();
		DPoint3d arcEnd = centerPoint;
		DPoint3d arcMid;
		movePoint(vecNormal, arcEnd, centerPoint.Distance(arcStr));

		EditElementHandle eehArc;
		//��ptAΪԲ��,dRadiusΪ�뾶��ptIn1Ϊ������㣬ptIn2Ϊ�����յ㻭��
		ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(centerPoint, arcStr, arcEnd), true, *ACTIVEMODEL);

		double dis2 = 0.00;

		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, eehArc.GetElementDescrP(), &arcEnd, 0.1);

		dis2 /= 2;

		mdlElmdscr_pointAtDistance(&arcMid, nullptr, dis2, eehArc.GetElementDescrP(), 0.1);

		PITRebarCurve rebarCurve;
		if (CalculateRound(rebarCurve, arcStr, arcMid, arcEnd, 1))
		{
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(arcStr, arcMid, arcEnd), true, *ACTIVEMODEL);
		}

		// �������λ�
		for (int i = 0; i < 3; i++)
		{
			arcStr = arcEnd;
			arcEnd = centerPoint;

			vecNormal = vecNormal.Perpendicular();
			movePoint(vecNormal, arcEnd, centerPoint.Distance(arcStr));

			//��ptAΪԲ��,dRadiusΪ�뾶��ptIn1Ϊ������㣬ptIn2Ϊ�����յ㻭��
			ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(centerPoint, arcStr, arcEnd), true, *ACTIVEMODEL);

			dis2 = 0.00;

			mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, eehArc.GetElementDescrP(), &arcEnd, 0.1);

			dis2 /= 2;

			mdlElmdscr_pointAtDistance(&arcMid, nullptr, dis2, eehArc.GetElementDescrP(), 0.1);
			int nStep = 0;
			if (i == 2)
			{
				nStep = 2;
			}
			if (CalculateRound(rebarCurve, arcStr, arcMid, arcEnd, nStep))
			{
				EditElementHandle arceeh1;
				ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(arcStr, arcMid, arcEnd), true, *ACTIVEMODEL);
			}
		}
		rebar.push_back(rebarCurve);

		double dot = acos((dZBack - vecArcPoint.at(0).centerPtr.z) / dArcRadius); // �Ƕ�
		// ��ֱ���� == R * (cos(dot) - cost(dot + ���Ƕ�))
		centerPoint.z = dZBack - dArcRadius * (cos(dot) - cos(dot + dSpace));
	}

	return true;
}


bool CDomeRebarAssembly::makeArcRebarCurve(vector<PIT::PITRebarCurve>& rebar, MSElementDescrP arcMsDescrp, PITRebarEndTypes& endTypes, double transLength, double diameter, double startOffset)
{
	double dAngle = startOffset;
	// ƫ�ƽǶ�
	double dSpacing = m_stArcCutInfo.dAngleOrSpace;

	int nRebarNum = (int)floor((360.0 - startOffset) / dSpacing + 0.5);
	dSpacing = (360.0 - startOffset) / (nRebarNum - 1);

	for (int i = 0; i < nRebarNum - 1; i++) // ���һ������
	{
		Transform	tmpMatrix;
		mdlTMatrix_getIdentity(&tmpMatrix);
		if (COMPARE_VALUES_EPS(dAngle, 0.00, EPS) > 0)
		{
			mdlTMatrix_rotateByAngles(&tmpMatrix, &tmpMatrix, 0, 0, (dSpacing / 180.0) * PI);
		}
		mdlElmdscr_transform(&arcMsDescrp, &tmpMatrix);

		double starR, sweepR;
		double radius;
		DPoint3d ArcDPs[2];
		RotMatrix rotM;
		DPoint3d centerpt;
		mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &arcMsDescrp->el);

		DPoint3d arcStr = ArcDPs[0];
		DPoint3d arcEnd = ArcDPs[1];


		// ���������м�ƫ��ָ������
		DPoint3d arcMid = DPoint3d::From(0, 0, 0);
		TranseArcSide(centerpt, arcStr, arcMid, arcEnd, (0 - transLength));
		
		// ������Բ�������㣬ȷ��ˮƽ�满��ֱ����
		if (!IntersectRound(centerpt, arcStr, arcMid, arcEnd))
		{
			return false;
		}

		EditElementHandle eehArc;
		//��ptAΪԲ��,dRadiusΪ�뾶��ptIn1Ϊ������㣬ptIn2Ϊ�����յ㻭��
		ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(centerpt, arcStr, arcEnd), true, *ACTIVEMODEL);

		double dis2 = 0.00;

		mdlElmdscr_distanceAtPoint(&dis2, nullptr, nullptr, eehArc.GetElementDescrP(), &arcEnd, 0.1);

		dis2 /= 2;

		mdlElmdscr_pointAtDistance(&arcMid, nullptr, dis2, eehArc.GetElementDescrP(), 0.1);

		PITRebarCurve rebarTmp;
		if (CalculateArc(rebarTmp, arcStr, arcMid, arcEnd))
		{
			EditElementHandle arceeh1;
			ArcHandler::CreateArcElement(arceeh1, nullptr, DEllipse3d::FromPointsOnArc(arcStr, arcMid, arcEnd), true, *ACTIVEMODEL);
			//arceeh1.AddToModel();
			rebar.push_back(rebarTmp);
		}
		dAngle += dSpacing;
	}

	return true;
}


bool CDomeRebarAssembly::CalculateRound(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt, int nStep)
{
	bool ret = false;

	BeArcSeg arc(begPt, midPt, endPt);

	CPoint3D cen;
	arc.GetCenter(cen);

	if (arc.GetCenter(cen))
	{
		CPoint3D beg = begPt;
		CPoint3D med = midPt;
		CPoint3D end = endPt;

		CVector3D tan1 = arc.GetTangentVector(beg);
		CVector3D tan2 = arc.GetTangentVector(end);

		CPointVect pv1(beg, tan1);
		CPointVect pv2(end, tan2);

		CPoint3D ip;
		bool isIntersect = pv1.Intersect(ip, pv2);

		double radius = arc.GetRadius();

		RebarVertexP vex;
		if (nStep == 1) // Բ���׶λ�
		{
			vex = &(curve.PopVertices()).NewElement();
			vex->SetIP(beg);
			vex->SetType(RebarVertex::kStart);      // first IP
		}

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

			if (COMPARE_VALUES_EPS(d1, d2, EPS) < 0)
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

		if (nStep == 2) // Բ�����һ�λ�
		{
			vex = &curve.PopVertices().NewElement();
			vex->SetIP(end);
			vex->SetType(RebarVertex::kEnd);      // last IP
		}
		else
		{
			//vex = &curve.PopVertices().NewElement();
			//vex->SetIP(end);
			//vex->SetType(RebarVertex::kIP);      // last IP
		}

		ret = true;
	}

	return ret;
}


// �����ߵĸֽ�����
bool CDomeRebarAssembly::CalculateArc(PITRebarCurve& curve, CPoint3DCR begPt, CPoint3DCR midPt, CPoint3DCR endPt)
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


bool CDomeRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	SetSelectedModel(ehSel.GetModelRef());

	DgnModelRefP modelRef = ACTIVEMODEL;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_pDomeRebarMainDlg = new CDomeRebarMainDlg();
	m_pDomeRebarMainDlg->SetSelectElement(ehSel);
	m_pDomeRebarMainDlg->Create(IDD_DIALOG_DomeMainDlg);
	m_pDomeRebarMainDlg->ShowWindow(SW_SHOW);
	
	return true;
}


bool CDomeRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
	{
		return false;
	}

	ElementHandle ehWall(GetSelectedElement(), GetSelectedModel());
	if (!ehWall.IsValid())
	{
		return false;
	}

	DgnModelRefP modelRef = ehWall.GetModelRef();
	MakeRebars(modelRef);
	Save(modelRef);

	ElementId contid = FetchConcrete();
	//SetElementXAttribute(ehSel.GetElementId(), g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ehSel.GetModelRef());
	//eeh2.AddToModel();
	return true;
}


long CDomeRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

