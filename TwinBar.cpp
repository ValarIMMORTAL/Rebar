#include "_ustation.h"
#include "TwinBar.h"
#include "CommonFile.h"

PIT::TwinRebar::TwinRebar(RebarElementCP pOrgRebar, RebarDataCR rebarData, DVec3dCR vec)
	:m_pOrgRebar(pOrgRebar),m_rebarData(rebarData),m_vec(vec)
{
}

PIT::TwinRebar::~TwinRebar()
{
// 	if (m_pOrgRebar != nullptr)
// 	{
// 		EditElementHandle eh(m_refLineId, m_pOrgRebar->GetModelRef());
// 		eh.DeleteFromModel();
// 	}

}

ElementId PIT::TwinRebar::DrawRefLine()
{
	if (m_pOrgRebar == nullptr)
	{
		return 0;
	}

	DgnModelRefP modelRef = m_pOrgRebar->GetModelRef();

	if (m_rebarData.rebarSize.Find(L"mm") != WString::npos)
		m_rebarData.rebarSize.Replace(L"mm", L"");

	RebarCurve rebarCurve;
	BrString strRebarSize;
	m_pOrgRebar->GetRebarCurve(rebarCurve, strRebarSize, modelRef);

	if (strRebarSize.Find(L"mm") != WString::npos)
		strRebarSize.Replace(L"mm", L"");

	double dim = RebarCode::GetBarDiameter(m_rebarData.rebarSize, modelRef);
	double dimOrg = RebarCode::GetBarDiameter(strRebarSize, modelRef);

	DPoint3d pt = m_vec;
	pt.ScaleToLength(dim*0.5 + dimOrg*0.5);
	Transform trans;
	mdlTMatrix_getIdentity(&trans);
	mdlTMatrix_setTranslation(&trans, &pt);
;
	CurveVectorPtr curve = CurveVector::Create(CurveVector::BOUNDARY_TYPE_None);
	m_pOrgRebar->GetCurveVector(*curve, modelRef);
	EditElementHandle eeh;
	if (SUCCESS == DraftingElementSchema::ToElement(eeh, *curve, NULL, true, *modelRef))
	{
		eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, (TransformInfo)trans);
		eeh.AddToModel();

		return eeh.GetElementId();
	}
	return 0;
}

RebarElement * PIT::TwinRebar::Create(RebarSetR rebarSet)
{
	if (m_pOrgRebar == nullptr)
	{
		return nullptr;
	}

	DgnModelRefP modelRef = m_pOrgRebar->GetModelRef();

	BrString twinRebarSize = m_rebarData.rebarSize;
	if (twinRebarSize.Find(L"mm") != WString::npos)
		twinRebarSize.Replace(L"mm", L"");

	RebarCurve rebarCurve;
	BrString strRebarSize;
	m_pOrgRebar->GetRebarCurve(rebarCurve, strRebarSize, modelRef);

	if (strRebarSize.Find(L"mm") != WString::npos)
		strRebarSize.Replace(L"mm", L"");

	double dim = RebarCode::GetBarDiameter(twinRebarSize, modelRef);
	double dimOrg = RebarCode::GetBarDiameter(strRebarSize, modelRef);

	DPoint3d pt = m_vec;
	pt.ScaleToLength(dim*0.5 + dimOrg * 0.5);
	Transform trans;
	mdlTMatrix_getIdentity(&trans);
	mdlTMatrix_setTranslation(&trans, &pt);
	rebarCurve.DoTransform(trans);

	int index = (int)rebarSet.GetChildElementCount();
	int rebarNum = index + 1;
	RebarElementP rebarElement = rebarSet.AssignRebarElement(index, rebarNum, m_rebarData.rebarSymb, modelRef);

	int gradenum = 2;
	//SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
	if (m_rebarData.SelectedRebarGrade == "A")
		gradenum = 0;
	else if (m_rebarData.SelectedRebarGrade == "B")
		gradenum = 1;
	else if (m_rebarData.SelectedRebarGrade == "C")
		gradenum = 2;
	else
		gradenum = 3;
	if (nullptr != rebarElement)
	{
		RebarShapeData twinRebarData = *m_pOrgRebar->GetShapeData(modelRef);
		twinRebarData.SetSizeKey((LPCTSTR)twinRebarSize);
		RebarEndType endType;
		endType.SetType(RebarEndType::kNone);
		RebarEndTypes endTypes = { endType,endType };
		rebarElement->Update(rebarCurve, dim, endTypes, twinRebarData, modelRef, false);

		ElementId eleid = rebarElement->GetElementId();
		EditElementHandle tmprebar(eleid, ACTIVEMODEL);
		string Stype = "TwinRebar";
		ElementRefP oldref = tmprebar.GetElementRef();
		SetRebarLevelItemTypeValue(tmprebar, m_rebarData.SelectedRebarLevel, gradenum, Stype, modelRef);
		SetRebarHideData(tmprebar, m_rebarData.SelectedSpacing, ACTIVEMODEL);
		tmprebar.ReplaceInModel(oldref);
	}
	return rebarElement;
}

RebarElement * PIT::TwinRebar::CreateTwinRebar(RebarSetR rebarSet, RebarElementCP rebar, PIT::RebarDataCR rebarData, DVec3dCR vec)
{
	return rebar == nullptr ? nullptr : TwinRebar(rebar, rebarData, vec).Create(rebarSet);
}

PIT::TwinBarMaker::TwinBarMaker(ElementId OrgRebarId, PIT::RebarDataCR twinRebarData, DVec3dCR vec,DgnModelRefP modeRef)
	:m_bNegate(false),m_interval(1)
{
	EditElementHandle eehRebar(OrgRebarId, modeRef);
	RebarElementP pRebar = RebarElement::Fetch(eehRebar);
	if (pRebar != nullptr)
	{
		m_vecTwinRebar.push_back(std::shared_ptr<TwinRebar>(new TwinRebar(pRebar, twinRebarData, vec)));
	}
}

PIT::TwinBarMaker::TwinBarMaker(const std::vector<std::pair<ElementId, PIT::RebarData> > &twinRebars, DVec3dCR vec, DgnModelRefP modeRef)
	:m_bNegate(false), m_interval(1)
{
	for (size_t i = 0; i < twinRebars.size(); ++i)
	{
		EditElementHandle eehRebar(twinRebars[i].first, modeRef);
		RebarElementP pRebar = RebarElement::Fetch(eehRebar);
		if (pRebar != nullptr)
		{
			m_vecTwinRebar.push_back(std::shared_ptr<TwinRebar>(new TwinRebar(pRebar, twinRebars[i].second, vec)));
		}
	}
}

PIT::TwinBarMaker::~TwinBarMaker()
{
}

RebarSetTag * PIT::TwinBarMaker::MakeRebar(ElementId rebarSetId, DgnModelRefP modelRef)
{
	//Ìí¼Ó²¢½î
	RebarSetP rebarSet = RebarSet::Fetch(rebarSetId,modelRef);
	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	if (rebarSet == nullptr)
	{
		return nullptr;
	}

	int rebarNum = (int)m_vecTwinRebar.size();
	for (int i = 0; i < rebarNum; i += m_interval)
	{
		m_vecTwinRebar[i]->Create(*rebarSet);
	}

	RebarSetTag *tag = new RebarSetTag;
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(false);
	return tag;
}

// void PIT::TwinBarMaker::AddRebar(RebarSetR rebarSet, TwinRebarP rebar, DgnModelRefP modelRef)
// {
// }
