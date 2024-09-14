#include "_ustation.h"
#include "StirrupRebar.h"
#include "CommonFile.h"
#include "BentlyCommonfile.h"


PIT::StirrupRebar::StirrupRebar(const vector<CPoint3D> &vecRebarPts, StirrupRebarDataCR rebarData)
	:m_vecRebarPts(vecRebarPts), m_rebarData(rebarData)
{
}

PIT::StirrupRebar::~StirrupRebar()
{
	// 	if (m_pOrgRebar != nullptr)
	// 	{
	// 		EditElementHandle eh(m_refLineId, m_pOrgRebar->GetModelRef());
	// 		eh.DeleteFromModel();
	// 	}

}

ElementId PIT::StirrupRebar::DrawRefLine(DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return 0;
	}

	if (m_vecRebarPts.size() == 4)
	{
		m_vecRebarPts.push_back(m_vecRebarPts[0]);
	}

	StatusInt ret = -1;
	EditElementHandle eeh;
	LineStringHandler::CreateLineStringElement(eeh, nullptr, &m_vecRebarPts[0], m_vecRebarPts.size(), true, *modelRef);
	ret = eeh.AddToModel();

	return eeh.GetElementId();
}

RebarElement * PIT::StirrupRebar::Create(RebarSetR rebarSet)
{
	DgnModelRefP modelRef = rebarSet.GetModelRef();
	if (m_vecRebarPts.size() < 5)
	{
		return nullptr;
	}
	BrString strRebarSize = m_rebarData.rebarSize;
	if (strRebarSize.Find(L"mm") != WString::npos)
		strRebarSize.Replace(L"mm", L"");

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dim = RebarCode::GetBarDiameter(strRebarSize, modelRef);
	double bendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, true);

	/**********֮ǰ�Ĺ��������***********/
	/*
	PITRebarEndType start, end;
	double startbendRadius, endbendRadius;
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;
	switch (m_rebarData.beg.endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		start.SetType(PITRebarEndType::kNone);
		break;
	case 7:	//ֱê
		start.SetType(PITRebarEndType::kLap);
		begStraightAnchorLen = m_rebarData.beg.endPtInfo.value1;	//ê�볤��
		break;
	case 4:	//90���乳
	{
		start.SetType(PITRebarEndType::kBend);
		startbendRadius = m_rebarData.beg.endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);
		}
		startbendLen = m_rebarData.beg.endPtInfo.value3;			//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = PITRebarCode::GetBendLength(strRebarSize, start, modelRef);
		}
	}
	break;
	case 5:	//135���乳
	{
		start.SetType(PITRebarEndType::kCog);
		startbendRadius = m_rebarData.beg.endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);
		}
		startbendLen = m_rebarData.beg.endPtInfo.value3;			//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen =	PITRebarCode::GetBendLength(strRebarSize, start, modelRef);
		}
	}
	break;
	case 6:	//180���乳
	{
		start.SetType(PITRebarEndType::kHook);
		startbendRadius = m_rebarData.beg.endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);	//������30
		}
		startbendLen = m_rebarData.beg.endPtInfo.value3;			//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = PITRebarCode::GetBendLength(strRebarSize, start, modelRef);	//������100
		}
	}
	break;
	case 8:	//�û�
		start.SetType(PITRebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (m_rebarData.end.endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		end.SetType(PITRebarEndType::kNone);
		break;
	case 7:	//ֱê
		end.SetType(PITRebarEndType::kLap);
		endStraightAnchorLen = m_rebarData.end.endPtInfo.value1;	//ê�볤��
		break;
	case 4:	//90���乳
	{
		end.SetType(PITRebarEndType::kBend);
		endbendRadius = m_rebarData.end.endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);
		}
		endbendLen = m_rebarData.end.endPtInfo.value3;				//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = PITRebarCode::GetBendLength(strRebarSize, end, modelRef);
		}
	}
	break;
	case 5:	//135���乳
	{
		end.SetType(PITRebarEndType::kCog);
		endbendRadius = m_rebarData.end.endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);
		}
		endbendLen = m_rebarData.end.endPtInfo.value3;				//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = PITRebarCode::GetBendLength(strRebarSize, end, modelRef);
		}
	}
	break;
	case 6:	//180���乳
	{
		end.SetType(PITRebarEndType::kHook);
		endbendRadius = m_rebarData.end.endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(strRebarSize, modelRef, false);	//������30
		}
		endbendLen = m_rebarData.end.endPtInfo.value3;				//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = PITRebarCode::GetBendLength(strRebarSize, end, modelRef);	//������100
		}
	}
	break;
	case 8:	//�û�
		end.SetType(PITRebarEndType::kCustom);
		break;
	default:
		break;
	}

	double bendLen = PITRebarCode::GetBendLength(strRebarSize, start, modelRef);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);
	start.SetstraightAnchorLen(begStraightAnchorLen);

	double dRotateAngle = m_rebarData.beg.rotateAngle;
	CVector3D vecBeg = m_vecRebarPts[m_vecRebarPts.size() - 2] - m_vecRebarPts.back();
	CVector3D rebarVec = m_vecRebarPts[1] - m_vecRebarPts.front();
	vecBeg.Normalize();
	vecBeg.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
	start.SetendNormal(vecBeg);


	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	dRotateAngle = m_rebarData.end.rotateAngle;
	CVector3D vecEnd = m_vecRebarPts[1] - m_vecRebarPts.front();
	vecEnd.Normalize();
	rebarVec = m_vecRebarPts[m_vecRebarPts.size() - 2] - m_vecRebarPts.back();
	vecEnd.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
	end.SetendNormal(vecEnd); 

	PITRebarEndTypes endTypes = { start,end };
	m_Curve.makeStirrupRebarCurve(m_vecRebarPts,bendRadius, endTypes);
	*/
	/***************************************/

	RebarEndType rebarEndType;
	switch (m_rebarData.beg.endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		rebarEndType.SetType((RebarEndType::Type)PITRebarEndType::kNone);
		break;
	case 7:	//ֱê
		rebarEndType.SetType((RebarEndType::Type)PITRebarEndType::kLap);
		break;
	case 4:	//90���乳
		rebarEndType.SetType((RebarEndType::Type)PITRebarEndType::kBend);
		break;
	case 5:	//135���乳
		rebarEndType.SetType((RebarEndType::Type)PITRebarEndType::kCog);
		break;
	case 6:	//180���乳
		rebarEndType.SetType((RebarEndType::Type)PITRebarEndType::kHook);
		break;
	case 8:	//�û�
		rebarEndType.SetType((RebarEndType::Type)PITRebarEndType::kCustom);
		break;
	default:
		break;
	}
	//double bendLen = RebarCode::GetBendLength(strRebarSize, rebarEndType, modelRef);
	double bendLen = 10 * dim;

	CVector3D   endNormal(0, 0.0, 1.0);//Ĭ�ϳ���
	{//����ʱ����ѭ������
		if (m_vecRebarPts.size()<2)
		{
			return nullptr;
		}
		DPoint3d vec1 = m_vecRebarPts[1] - m_vecRebarPts[0];
		vec1.Normalize();
		DPoint3d vec2 = m_vecRebarPts[2] - m_vecRebarPts[1];
		vec2.Normalize();
		vec1.CrossProduct(vec1, vec2);
		double dottmp = vec1.DotProduct(endNormal);
		if (dottmp<0)//����
		{
			reverse(m_vecRebarPts.begin(), m_vecRebarPts.end());
		}
	}
	
	RebarEndType endTypeStart, endTypeEnd;
	endTypeStart.SetType(RebarEndType::kCog);
	endTypeEnd.SetType(RebarEndType::kCog);
	RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
	//���ƿ������ڵ����߸ֽ�
	//PITRebarCurve rebarcurve;
	m_Curve.makeStirrupURebarWithNormal(m_vecRebarPts,bendRadius, bendLen, endTypes, endNormal);


	int index = (int)rebarSet.GetChildElementCount();
	int rebarNum = index + 1;
	RebarElementP rebarElement = rebarSet.AssignRebarElement(index, rebarNum, m_rebarData.rebarSymb, modelRef);
	if (nullptr != rebarElement)
	{
		RebarShapeData rebarData;
		rebarData.SetSizeKey((LPCTSTR)strRebarSize);
		rebarData.SetIsStirrup(false);
		rebarData.SetLength(m_Curve.GetLength() / uor_per_mm);
		RebarEndType endType;
		endType.SetType(RebarEndType::kNone);
		RebarEndTypes endTypes = { endType,endType };
		rebarElement->Update(m_Curve, dim, endTypes, rebarData, modelRef, false);

		ElementId eleid = rebarElement->GetElementId();
		EditElementHandle tmprebar(eleid, ACTIVEMODEL);
		string Stype = "StirrupRebar";
		Stype = Stype + m_rebarData.SelectedRebarType;
		ElementRefP oldref = tmprebar.GetElementRef();
		SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
		tmprebar.ReplaceInModel(oldref);
	}
	return rebarElement;
}

RebarElement * PIT::StirrupRebar::CreateStirrupRebar(RebarSetR rebarSet, const vector<CPoint3D>& vecRebarPts, PIT::StirrupRebarDataCR rebarData)
{
	return StirrupRebar(vecRebarPts, rebarData).Create(rebarSet);
}

PIT::StirrupRebarMaker::StirrupRebarMaker(const vector<ElementId>& rebar_Vs, const vector<ElementId>  &rebarId_Hs, PIT::StirrupRebarDataCR rebarData, bool bUp, DgnModelRefP modeRef)
	:m_bUp(bUp)
{
	UpdateStirrupRebars(rebar_Vs, rebarId_Hs, rebarData, bUp, modeRef);
}

PIT::StirrupRebarMaker::~StirrupRebarMaker()
{

}


bool PIT::StirrupRebarMaker::CalStirrupRebarPts(vector<CPoint3D>& pts, vector<ElementId> rebar_Vs, ElementId rebar_H, PIT::StirrupRebarDataCR rebarData, bool up, DgnModelRefP modeRef)
{
	if (modeRef == nullptr)
	{
		return false;
	}
	double uor_per_mm = modeRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	if (rebar_Vs.size() < 4)
	{
		return false;
	}
	//ѡ�е��ĸ���ֱ����ֽ�
	EditElementHandle eehRebar_V0(rebar_Vs[0], modeRef);
	EditElementHandle eehRebar_V1(rebar_Vs[1], modeRef);
	EditElementHandle eehRebar_V2(rebar_Vs[2], modeRef);
	EditElementHandle eehRebar_V3(rebar_Vs[3], modeRef);
	RebarElementP pRebar_V0 = RebarElement::Fetch(eehRebar_V0);
	RebarElementP pRebar_V1 = RebarElement::Fetch(eehRebar_V1);
	RebarElementP pRebar_V2 = RebarElement::Fetch(eehRebar_V2);
	RebarElementP pRebar_V3 = RebarElement::Fetch(eehRebar_V3);

	if (pRebar_V0 == nullptr || pRebar_V1 == nullptr || pRebar_V2 == nullptr || pRebar_V3 == nullptr)
	{
		return false;
	}
	//��ȡ�ĸ���ֵ����ֽ�������ͳߴ�����
	vector<CPoint3D> vecPts;
	BrString rebarSize_V0,rebarSize_V1,rebarSize_V2,rebarSize_V3;
	RebarCurve curve_V0, curve_V1, curve_V2, curve_V3;
	pRebar_V0->GetRebarCurve(curve_V0, rebarSize_V0, modeRef);
	pRebar_V1->GetRebarCurve(curve_V1, rebarSize_V1, modeRef);
	pRebar_V2->GetRebarCurve(curve_V2, rebarSize_V2, modeRef);
	pRebar_V3->GetRebarCurve(curve_V3, rebarSize_V3, modeRef);
	//��ȡ�ĸ��ֽ����߶�
	LineSegment maxLineSeg_V0,maxLineSeg_V1, maxLineSeg_V2,maxLineSeg_V3;
	if (!PITRebarCurve::GetMaxLenLine(curve_V0, maxLineSeg_V0) || !PITRebarCurve::GetMaxLenLine(curve_V1, maxLineSeg_V1) ||
		!PITRebarCurve::GetMaxLenLine(curve_V2, maxLineSeg_V2) || !PITRebarCurve::GetMaxLenLine(curve_V3, maxLineSeg_V3))
	{
		return false;
	}

	//�������ֽ�������ֽ��3ά���ĸ�����
	EditElementHandle eehRebar_H(rebar_H, modeRef);
	RebarElementP pRebar_H = RebarElement::Fetch(eehRebar_H);
	if (pRebar_H == nullptr)
	{
		return false;
	}
	RebarCurve curve_H;
	BrString rebarSize_H;
	pRebar_H->GetRebarCurve(curve_H, rebarSize_H, modeRef);
	LineSegment maxLineSeg_H;  //ˮƽ�ֽ����߶�
	if (!PITRebarCurve::GetMaxLenLine(curve_H, maxLineSeg_H))
	{
		return false;
	}

	BrString strRebarSize = rebarData.rebarSize;
	if (strRebarSize.Find(L"mm") != WString::npos)
		strRebarSize.Replace(L"mm", L"");
	double dimUReabr = RebarCode::GetBarDiameter(strRebarSize, modeRef);	//Ҫ���ƵĹ���ĸֽ�ֱ��
	double dimH = RebarCode::GetBarDiameter(rebarSize_H, modeRef);			//ˮƽ�ֽ��ֱ��
	//��ֱ����ֽ���ˮƽ����ֽ�Ľ���
	DPoint3d ptIntersect_V0 = maxLineSeg_V0.Intersect(maxLineSeg_H);
	DPoint3d ptIntersect_V1 = maxLineSeg_V1.Intersect(maxLineSeg_H);
	DPoint3d ptIntersect_V2 = maxLineSeg_V2.Intersect(maxLineSeg_H);
	DPoint3d ptIntersect_V3 = maxLineSeg_V3.Intersect(maxLineSeg_H);

	DVec3d rebarVec_V = maxLineSeg_V1.GetLineVec(); //��ֱ����ֽ���������������Ĵ�ֱ����
	//�������ڽ���֮�������
	DVec3d insecV0_1 = ptIntersect_V1 - ptIntersect_V0;
	DVec3d insecV1_2 = ptIntersect_V2 - ptIntersect_V1;
	DVec3d insecV2_3 = ptIntersect_V3 - ptIntersect_V2;
	DVec3d insecV3_0 = ptIntersect_V0 - ptIntersect_V3;
	//��ȡ�ĸ���ֱ�ֽ��ֱ��
	double dimV0 = RebarCode::GetBarDiameter(rebarSize_V0, modeRef);
	double dimV1 = RebarCode::GetBarDiameter(rebarSize_V1, modeRef);
	double dimV2 = RebarCode::GetBarDiameter(rebarSize_V2, modeRef);
	double dimV3 = RebarCode::GetBarDiameter(rebarSize_V3, modeRef);

	/************ˮƽƫ�ƣ��������ճ�һ��������ֱ�ֽ�����¹���ľ��η�Χ***********/
	//��Χ���ĸ���
	DPoint3d pt0 = ptIntersect_V0;
	DPoint3d pt1 = ptIntersect_V1;
	DPoint3d pt2 = ptIntersect_V2;
	DPoint3d pt3 = ptIntersect_V3;
	double scale0_1, scale0_3; //��ƽ������
	//�Ƿ���ˮƽ�ֽ�ƽ��
	if (insecV0_1.IsParallelTo(maxLineSeg_H.GetLineVec()))
	{
		scale0_1 = 1;
		scale0_3 = 0.5;
	}
	else
	{
		scale0_1 = 0.5;
		scale0_3 = 0.5;
	}

	//������������������������ƽ�ƽ���
	insecV0_1.Negate();
	insecV0_1.ScaleToLength(dimV0 * scale0_1 + dimUReabr * scale0_1);
	insecV3_0.ScaleToLength(dimV0 * scale0_3 + dimUReabr * scale0_3);

	pt0.Add(insecV0_1);
	pt0.Add(insecV3_0);

	insecV0_1.Negate();
	insecV0_1.ScaleToLength(dimV1 * scale0_1 + dimUReabr * scale0_1);
	insecV1_2.Negate();
	insecV1_2.ScaleToLength(dimV1 * scale0_3 + dimUReabr * scale0_3);
	pt1.Add(insecV0_1);
	pt1.Add(insecV1_2);

	insecV1_2.Negate();
	insecV1_2.ScaleToLength(dimV2 * scale0_3 + dimUReabr * scale0_3);
	insecV2_3.Negate();
	insecV2_3.ScaleToLength(dimV2 * scale0_1 + dimUReabr * scale0_1);
	pt2.Add(insecV1_2);
	pt2.Add(insecV2_3);

	insecV2_3.Negate();
	insecV2_3.ScaleToLength(dimV3 * scale0_1 + dimUReabr * scale0_1);
	insecV3_0.Negate();
	insecV3_0.ScaleToLength(dimV3 * scale0_3 + dimUReabr * scale0_3);
	pt3.Add(insecV2_3);
	pt3.Add(insecV3_0);
	
	//��ֱƫ��
	if (up)
	{
		rebarVec_V.Negate();
	}

	rebarVec_V.ScaleToLength(dimH * 0.5 + dimUReabr * 0.5);
	pt0.Add(rebarVec_V);
	pt1.Add(rebarVec_V);
	pt2.Add(rebarVec_V);
	pt3.Add(rebarVec_V);


	pts = { pt0,pt1,pt2,pt3,pt0};
	return true;
}

RebarSetTag * PIT::StirrupRebarMaker::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return nullptr;
	}

	//��Ӳ���
	RebarSetP rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (rebarSet == nullptr)
	{
		return nullptr;
	}
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(rebarSetId);
	rebarSet->StartUpdate(modelRef);
	int rebarNum = (int)m_pStirrupRebars.size();
	for (int i = 0; i < rebarNum; ++i)
	{
		m_pStirrupRebars[i]->Create(*rebarSet);
	}

	RebarSetTag *tag = new RebarSetTag;
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);
	return tag;
}

bool PIT::StirrupRebarMaker::UpdateStirrupRebar(vector<CPoint3D>& pts, const vector<ElementId>& rebar_Vs,ElementId rebar_H, PIT::StirrupRebarDataCR rebarData, bool up, int rebarIndex, DgnModelRefP modeRef)
{
	if (modeRef == nullptr)
	{
		return false;
	}

	pts.clear();
	if (!CalStirrupRebarPts(pts, rebar_Vs, rebar_H, rebarData, up, modeRef))
		return false;

	if (m_pStirrupRebars.empty() || (int)m_pStirrupRebars.size() <= rebarIndex)
	{
		m_pStirrupRebars.push_back(shared_ptr<PIT::StirrupRebar>(new PIT::StirrupRebar(pts, rebarData)));
	}
	else
	{
		m_pStirrupRebars[rebarIndex] = shared_ptr<PIT::StirrupRebar>(new PIT::StirrupRebar(pts, rebarData));
	}

	return true;
}

bool PIT::StirrupRebarMaker::UpdateStirrupRebars(const vector<ElementId>& rebar_Vs, const vector<ElementId>  &rebarId_Hs, PIT::StirrupRebarDataCR rebarData, bool up, DgnModelRefP modeRef)
{
	if (modeRef == nullptr)
	{
		return false;
	}

	m_pStirrupRebars.clear();
	vector<CPoint3D> arcVPts;
	for (int i = 0; i < (int)rebarId_Hs.size(); ++i)
	{
		vector<CPoint3D> pts;
		UpdateStirrupRebar(pts, rebar_Vs, rebarId_Hs[i], rebarData, up, i, modeRef);
		m_vecStirrupRebarPts.push_back(pts);
		if (i == 0)
		{
			arcVPts = pts;
		}
	}
	//���㹿��������������
	if (arcVPts.size() >= 5)
	{
		BrString strRebarSize = rebarData.rebarSize;
		if (strRebarSize.Find(L"mm") != WString::npos)
			strRebarSize.Replace(L"mm", L"");
		double bendRadius = RebarCode::GetPinRadius(strRebarSize, modeRef, true);

		for (int i = 0; i < arcVPts.size(); i++)
		{
			RebarHelper::RebarArcData arcdata;
			if (i == 0)
			{
				RebarHelper::IntersectionPointToArcDataRebar(arcdata, arcVPts[0], arcVPts[arcVPts.size() - 2], arcVPts[1], bendRadius);
				m_rebarArcDatas.push_back(arcdata);
			}
			else if (i == arcVPts.size() - 1)
			{
				continue;
			}
			else
			{
				DPoint3d ptVertex1 = arcVPts[i - 1];
				DPoint3d ptVertex2 = arcVPts[i];
				DPoint3d ptVertex3 = arcVPts[i + 1];
				RebarHelper::IntersectionPointToArcDataRebar(arcdata, ptVertex2, ptVertex1, ptVertex3, bendRadius);
				m_rebarArcDatas.push_back(arcdata);
			}
		}
	}

	return false;
}

bool PIT::StirrupRebarMaker::DrawRefLine(ElementId & id, int index, DgnModelRefP modelRef)
{
	if (index >= m_pStirrupRebars.size() || modelRef == nullptr)
	{
		return false;
	}

	id = m_pStirrupRebars[index]->DrawRefLine(modelRef);
	return true;
}

bool PIT::StirrupRebarMaker::DrawRefLine(vector<ElementId>& ids, DgnModelRefP modelRef)
{
	if (modelRef == nullptr)
	{
		return false;
	}

	ids.clear();
	for (auto ptr : m_pStirrupRebars)
	{
		if (ptr != nullptr)
		{
			ids.push_back(ptr->DrawRefLine(modelRef));
		}
	}
	return true;
}