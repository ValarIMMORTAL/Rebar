#include "_ustation.h"
#include "EdgeRebarAssembly.h"
#include "PITRebarCurve.h"
#include "ExtractFacesTool.h"

EdgeRebarAssembly::EdgeRebarAssembly(ElementId id, DgnModelRefP modelRef) : RebarAssembly(id, modelRef)
{

}

bool EdgeRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	bool const isStirrup = false;
	RebarSetP   rebarSet = RebarSet::Fetch(m_SetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	int j = 0;
	RebarEndType endTypeStart, endTypeEnd;
	//double diameter = RebarCode::GetBarDiameter(m_sizeKey, modelRef);	
 	char tmpchar[256];
 	strcpy(tmpchar, CT2A(m_sizeKey));
 	std::string stRebarsize(tmpchar);
 	int diameter = (int)atoi(stRebarsize.c_str());//直径
	RebarSymbology symb;
	{
		string str(m_sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}

	vector<PIT::PITRebarCurve>     rebarCurvesNum;


	makeRebarCurve(rebarCurvesNum, m_rebarPts);


	int numRebar = rebarCurvesNum.size();

	vector<vector<DPoint3d>> vecStartEnd;
	for (PIT::PITRebarCurve rebarCurve : rebarCurvesNum)//根据钢筋数据分别生成钢筋
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);

		vector<DPoint3d> linePts;
		linePts.push_back(ptstr);
		linePts.push_back(ptend);
		//vecStartEnd.push_back(DSegment3d::From(ptstr, ptend));
		/*	EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			eeh.AddToModel();
*/
		RebarElementP rebarElement = NULL;;
		//		if (!PreviewButtonDown)
		{//若不是预览状态下则生成钢筋
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			{
				shape.SetSizeKey((LPCTSTR)m_sizeKey);
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameter*uor_per_mm, endTypes, shape, modelRef, false);
			}
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string Stype = "Edge";

			/*int num = Gettypenum();
			if (m_RebarGrade != "")
			{
				if (m_RebarGrade == "A")
					num = 0;
				else if (m_RebarGrade == "B")
					num = 1;
				else if (m_RebarGrade == "C")
					num = 2;
				else
					num = 3;
			}*/
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, "ddd", 2, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, m_spacing/uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
		j++;
		vecStartEnd.push_back(linePts);
	}

	//	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(m_spacing / uor_per_mm);
	setdata.SetAverageSpacing(m_spacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	if (NULL != tag)
	{
		tag->SetBarSetTag(0);
		rsetTags.Add(tag);
		AddRebarSets(rsetTags);
		return true;
	}


	return false;
}

bool EdgeRebarAssembly::SetSlabData(ElementHandleCR slab)
{
	ElementHandle eeh(slab);
	if (!eeh.IsValid())
	{
		ElementId conid = GetConcreteOwner();
		eeh = ElementHandle(conid, ACTIVEMODEL);
	}
	if (eeh.GetElementId() <= 0)
		return false;
	SetSelectedElement(eeh.GetElementId());
	SetSelectedModel(eeh.GetModelRef());
	return true;
}
bool EdgeRebarAssembly::makeRebarCurve(vector<PIT::PITRebarCurve>&  rebars, vector<RebarVertices>& pts)
{

	for (auto it = pts.begin(); it != pts.end(); it++)
	{
		PIT::PITRebarCurve rebar;
		for (int i = 0; i < it->GetSize(); i++)
		{
			RebarVertexP vex;
			vex = &rebar.PopVertices().NewElement();
			vex->SetIP(it->At(i).GetIP());
			vex->SetArcPt(0, it->At(i).GetArcPt(0));
			vex->SetArcPt(1, it->At(i).GetArcPt(1));
			vex->SetArcPt(2, it->At(i).GetArcPt(2));
			vex->SetCenter(it->At(i).GetCenter());
			vex->SetRadius(it->At(i).GetRadius());
			vex->SetTanPt(0, it->At(i).GetTanPt(0));
			vex->SetTanPt(1, it->At(i).GetTanPt(1));
			vex->SetType(it->At(i).GetType());
		}

		rebars.push_back(rebar);
	}

	return true;
}

long EdgeRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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