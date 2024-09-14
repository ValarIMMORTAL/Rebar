/*--------------------------------------------------------------------------------------+
|
|     $Source: CInsertRebarAssembly.cpp $
|
|  $Copyright: (c) 2021 PowerItech Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/

#include "_ustation.h"
#include "CommonFile.h"
#include <SelectionRebar.h>
#include "CInsertRebarAssemblyColumn.h"
#include "ExtractFacesTool.h"
#include "CInsertRebarMainDlg.h"
#include "ElementAttribute.h"
#include "resource.h"
#include "XmlHelper.h"
#include "PITRebarCurve.h"
CInsertRebarAssemblyColumn::CInsertRebarAssemblyColumn(ElementId id, DgnModelRefP modelRef) : RebarAssembly(id, modelRef)
{
	Init();
}

void CInsertRebarAssemblyColumn::Init()
{
	m_shape = 0;
	m_length = 0.00;
	m_width = 0.00;
	m_heigth = 0.00;
	m_columeCover = 0.00;
	m_longNum = 0;
	m_shortNum = 0;
	m_embedLength = 0.00;
	m_expandLength = 0.00;
	m_endType = 0;
	m_cornerType = 0;
	m_rotateAngle = 0.00;

	m_vecSetId.resize(2);

	for (unsigned int i = 0; i < m_vecSetId.size(); i++)
	{
		m_vecSetId[i] = 0;
	}
}

bool CInsertRebarAssemblyColumn::IsSmartSmartFeature(EditElementHandle& eeh)
{
	SmartFeatureNodePtr pFeatureNode;
	if (SmartFeatureElement::IsSmartFeature(eeh))
	{
		return true;
	}
	else
	{
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			if (SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef()) == SUCCESS)
			{
				eeh.AddToModel();
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}

		}
		return true;
	}
}

void CInsertRebarAssemblyColumn::CalculateTransform(vector<CVector3D>& vTransform, DgnModelRefP modelRef, bool isFlag)
{
	if (modelRef == NULL)
	{
		return;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // ÿ��������

	double dHigth = (m_embedLength + m_expandLength) * uor_per_mm;
	double dColCover = GetcolumeCover() * uor_per_mm;
	double dWidth = Getwidth() * uor_per_mm;
	double dLength = Getlength() * uor_per_mm;

	double dSpacing = isFlag ? dWidth : dLength;

	double verticalLen = m_verticalLen * uor_per_mm;
	double hoopRebarLen = m_hoopRebarLen * uor_per_mm;

	WString strSize = GetverticalSize();
	if (strSize.find(L"mm") != WString::npos)
	{
		strSize.ReplaceAll(L"mm", L"");
	}
	double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//ֱ��������10

	int nNum = 0;
	if (m_longNum > 0)
	{
		nNum++;
	}
	if (m_shortNum > 0)
	{
		nNum++;
	}

	for (int i = 0; i < nNum; i++)
	{
		CVector3D	Trans(0.0, 0.0, 0.0);
		if (i == 0)
		{
			Trans.x = dColCover + diameter * 0.5 + verticalLen + hoopRebarLen;
			Trans.y = -(dColCover + diameter * 0.5 + verticalLen + hoopRebarLen);
			Trans.z = 0.00;
		}
		else
		{
			Trans.x = dColCover + diameter * 0.5 + verticalLen + hoopRebarLen;
			Trans.y = -(dSpacing - dColCover - diameter * 0.5 - verticalLen - hoopRebarLen);
			Trans.z = 0.00;
		}
		vTransform.push_back(Trans);
	}
	
}

bool CInsertRebarAssemblyColumn::MakeRebarCurve
(
	vector<RebarCurve>&     rebars,
	double                  xPos,
	double                  yLen,
	double                  bendRadius,
	double                  bendLen,
	RebarEndTypes const&    endTypes,
	CVector3D const&        endNormal,
	CMatrix3D const&        mat
)
{
	CPoint3D  startPt;
	CPoint3D  endPt;

	startPt = CPoint3D::From(xPos, 0.0, yLen / 2.0);
	endPt = CPoint3D::From(xPos, 0.0, -yLen / 2.0);

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	// eeh.AddToModel();

	DPoint3d pt1[2];
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//��ʱʹ�õ�ǰ����MODEL�������������޸�

	map<int, DPoint3d> map_pts;
	map_pts[0] = pt1[0];
	map_pts[(int)pt1[0].Distance(pt1[1])] = pt1[1];

	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{
		RebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itr->second);
		vex->SetType(RebarVertex::kStart);

		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}

		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(itrplus->second);
		vex->SetType(RebarVertex::kEnd);

		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
		rebars.push_back(rebar);
	}

	return true;
}

bool CInsertRebarAssemblyColumn::MakeRebars(DgnModelRefP modelRef)
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags; // �ֽ�װ����

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;		//������λ mm

	double CoverNum = GetcolumeCover() * uor_per_mm; // ��������
	if (COMPARE_VALUES(CoverNum, Getlength() * uor_per_mm) >= 0 || COMPARE_VALUES(CoverNum, Getwidth() * uor_per_mm) >= 0)
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"����������ڵ������ĳ����,�޷������ֽ��", MessageBoxIconType::Information);
		return false;
	}

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis); // CMatrix3D 4 * 4 �ľ���
	vector<CVector3D> vTransform;
	bool isFlag = (COMPARE_VALUES(Getlength(), Getwidth()) > 0); // �����ǳ���
	CalculateTransform(vTransform, modelRef, isFlag);
	if (vTransform.size() < 2)
	{
		return false;
	}

	RebarSetTag* tag = NULL;
	if (COMPARE_VALUES(Getlength(), Getwidth()) >= 0) // �����ǳ���
	{
		tag = MakeRebars(GetlongNum(), GetshortNum(), true, vTransform, modelRef);
	}
	else // �����ǳ���
	{
		tag = MakeRebars(GetshortNum(), GetlongNum(), false, vTransform, modelRef);
	}

	if (tag != NULL)
	{
		tag->SetBarSetTag(1);
		rsetTags.Add(tag);
	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags)); // ��Ӹֽ���
	}
	return true;
}

RebarSetTag*  CInsertRebarAssemblyColumn::MakeRebars
(
	int						nSideRebarNum,	// ����ֽ�����
	int						nFrontRebarNum, // ����ֽ�����
	bool					isFlag,			// true: ����Ϊ����, false: ����Ϊ����
	vector<CVector3D>& vTransform, 
	DgnModelRefP modelRef
)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;		//������λ mm

	double dPos = 0.0;
	double dColCover = GetcolumeCover() * uor_per_mm;
	double dWidth = Getwidth() * uor_per_mm;
	double dLength = Getlength() * uor_per_mm;

	double dLongLength = (COMPARE_VALUES(m_length, m_width) > 0) ? (m_length * uor_per_mm) : (m_width * uor_per_mm); // ���泤��
	double dShortLength = (COMPARE_VALUES(m_length, m_width) > 0) ? (m_width * uor_per_mm) : (m_length * uor_per_mm); // ���泤��

	WString strPreSize = m_verticalSize;
	if (strPreSize.find(L"mm") != WString::npos)
	{
		strPreSize.ReplaceAll(L"mm", L"");
	}

	double bendRadius = RebarCode::GetPinRadius(strPreSize, modelRef, false);	//������30
	double bVerticalDiameter = RebarCode::GetBarDiameter(strPreSize, modelRef); // �ݽ�ֱ��

	double dHeigth = (m_embedLength + m_expandLength) * uor_per_mm;  // ���ó��� + ��չ���� == �ݽ��

	double zPos = (m_expandLength - m_embedLength) * uor_per_mm * 0.5; // ��Ҫƽ�Ƶ�z����

	double verticalLen = m_verticalLen * uor_per_mm; // �ݽ���
	double hoopRebarLen = m_hoopRebarLen * uor_per_mm; // ������

	double dLongSpacing = dLongLength - GetcolumeCover() * uor_per_mm * 2 - bVerticalDiameter - (verticalLen + hoopRebarLen) * 2;
	dLongSpacing /= m_longNum - 1; // ������

	double dShortSpacing = dShortLength - GetcolumeCover() * uor_per_mm * 2 - bVerticalDiameter - (verticalLen + hoopRebarLen) * 2;
	dShortSpacing /= m_shortNum - 1; // ������

	double dYSpacing = isFlag ? dLongSpacing : dShortSpacing;
	double dXSpacing = isFlag ? dShortSpacing : dLongSpacing;

	double dTrans = isFlag ? dLength : dWidth;

	bool const isStirrup = false;

	RebarEndType endTypeStart, endTypeEnd;	//	β����ʽ
	endTypeStart.SetType(RebarEndType::kNone);
	switch (m_endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeEnd.SetType(RebarEndType::kLap);
		break;
	case 4:	//90���乳
		endTypeEnd.SetType(RebarEndType::kBend);
		break;
	case 5:	//135���乳
		endTypeEnd.SetType(RebarEndType::kCog);
		break;
	case 6:	//180���乳
		endTypeEnd.SetType(RebarEndType::kHook);
		break;
	case 8:	//�û�
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	double bendLen = RebarCode::GetBendLength(strPreSize, endTypeStart, modelRef);	//������100

	RebarSetP   rebarSet = RebarSet::Fetch(m_vecSetId[0], modelRef);
	if (NULL == rebarSet)
	{
		return NULL;
	}

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	double dNextPos = dPos;
	int numRebar = nFrontRebarNum * 2 + nSideRebarNum * 2 - 4; // �������
	CVector3D	TransformTmp(vTransform[0]);
	vector<RebarCurve>     rebarCurvesNum;
	for (int i = 0; i < numRebar; i++)
	{
		CMatrix3D   mat;
		vector<RebarCurve>      rebarCurve;
		RebarEndTypes   endTypes = { endTypeStart, endTypeEnd }; // �˲���ʽ

		double dRotateAngle = GetrotateAngle();
		CVector3D vec =  m_ptStart - m_ptEnd;
		CVector3D	endNormal;	//�˲��乳����
		vec.Normalize();
		endNormal = vec.Perpendicular();

		if (i == nSideRebarNum || i == 2 * nSideRebarNum)
		{
			dPos = 0;
			dNextPos = 0;
		}

		if (i >= 2 * nSideRebarNum  && i % 2 == 0)
		{
			TransformTmp.y -= dXSpacing;
			dNextPos = dTrans - dColCover * 2 - bVerticalDiameter - (verticalLen + hoopRebarLen) * 2;
		}

		if (i < nSideRebarNum)  // 3
		{
			CVector3D	TransformIndex(vTransform[0]);
			TransformIndex.z = zPos;
			mat.SetTranslation(TransformIndex);
			dNextPos += dYSpacing;
			endNormal.Rotate((270.0 + dRotateAngle) * PI / 180, CVector3D::kZaxis); // ��Z����ת

			// �乳����
			if (m_cornerType == CornerType::Corner_Tilt) // �ǲ���б
			{
				if (i == 0)
				{
					endNormal.Rotate(45.0 * PI / 180, CVector3D::kZaxis);
				}
				else if (i == nSideRebarNum - 1)
				{
					endNormal.Rotate(-45.0 * PI / 180, CVector3D::kZaxis);
				}
			}
		}
		else if (i < 2 * nSideRebarNum) // 6
		{
			CVector3D	TransformIndex(vTransform[1]);
			TransformIndex.z = zPos;
			mat.SetTranslation(TransformIndex);
			dNextPos += dYSpacing;
			endNormal.Rotate((90.0 + dRotateAngle) * PI / 180, CVector3D::kZaxis); // ��Z����ת

			// �乳����
			if (m_cornerType == CornerType::Corner_Tilt) // �ǲ���б
			{
				if (i == nSideRebarNum)
				{
					endNormal.Rotate(-45.0 * PI / 180, CVector3D::kZaxis);
				}
				else if (i == nSideRebarNum *  2 - 1)
				{
					endNormal.Rotate(45.0 * PI / 180, CVector3D::kZaxis);
				}
			}
		}
		else
		{
			TransformTmp.z = zPos;
			mat.SetTranslation(TransformTmp);
			endNormal.Rotate((0.0 + dRotateAngle) * PI / 180, CVector3D::kZaxis); // ��Z����ת
		}

		if ((numRebar - i <= (nFrontRebarNum * 2 - 4)) && (i >= 2 * nSideRebarNum) && (i % 2 == 1))
		{
			endNormal.Negate();
		}
		
		mat = GetPlacement() * mat;

		if (i >= 2 * nSideRebarNum  && i % 2 == 0)
		{
			dPos = 0.0;
		}

		MakeRebarCurve(rebarCurve, dPos, dHeigth, bendRadius, bendLen, endTypes, endNormal, mat); // ��ֽ�����

		dPos = dNextPos;

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurve.begin(), rebarCurve.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	string str(m_verticalSize);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_INSERTION_REBAR);

	int j = 0;
	vector<pair<CPoint3D, CPoint3D> > vecStartEnd;
	for (RebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		vecStartEnd.push_back(make_pair(ptstr, ptend));
		//EditElementHandle eeh;
		//LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		//eeh.AddToModel();

		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef); // ���嵽ĳһ���ֽ�
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			BrString	sizeKey = m_verticalSize;
			sizeKey.Replace(L"mm", L"");
			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, bVerticalDiameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string Stype = "InsertRebar";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, (isFlag ? dXSpacing : dYSpacing) / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);
		}
		j++;
	}

	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing((isFlag ? dXSpacing : dYSpacing) / uor_per_mm);
	setdata.SetAverageSpacing((isFlag ? dXSpacing : dYSpacing) / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);
	
	return tag;
}


void CInsertRebarAssemblyColumn::SetInsertRebarData(InsertRebarInfo& stInserRebarInfo)
{
	Setshape(stInserRebarInfo.colInfo.shape);					// 0: ����   1: Բ��
	SetcolumeCover(stInserRebarInfo.colInfo.columeCover);		// ��������
	SetlongNum(stInserRebarInfo.rebarInfo.longNum);				// ��������
	SetshortNum(stInserRebarInfo.rebarInfo.shortNum);			// ��������
	SetembedLength(stInserRebarInfo.rebarInfo.embedLength);		// ���ó���
	SetexpandLength(stInserRebarInfo.rebarInfo.expandLength);	// ��չ����
	SetendType(stInserRebarInfo.rebarInfo.endType);				// �˲���ʽ����
	SetcornerType(stInserRebarInfo.rebarInfo.cornerType);		// �乳����
	SetrotateAngle(stInserRebarInfo.rebarInfo.rotateAngle);		// ��ת��
	SetverticalSize(stInserRebarInfo.rebarInfo.rebarSize);		// �ݽ�ߴ�
	SetverticalType(stInserRebarInfo.rebarInfo.rebarType);		// �ݽ�����
	// SetvecSetId(stInserRebarInfo.rebarInfo.vecSetId);

	CString strTemp = CString(stInserRebarInfo.colInfo.rebarVerticalSize);
	strTemp.Replace(L"mm", L"");
	SetverticalLen(atof(CT2A(strTemp)));

	CString strtemp = CString(stInserRebarInfo.colInfo.rebarHoopSize);
	strtemp.Replace(L"mm", L"");
	SethoopRebarLen(atof(CT2A(strTemp)));
}

// �������ĳ��ȺͿ��
void CInsertRebarAssemblyColumn::CalcColumnLengthAndWidth(ElementHandleCR eh)
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

	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	EFT::GetFrontBackLinePoint(Eleeh, vecDownFontLine, vecDownBackLine, &m_heigth);

	if (vecDownFontLine.empty() || vecDownBackLine.empty())
	{
		return ;
	}

	DPoint3d pt1[2];
	vecDownFontLine[0].GetStartPoint(pt1[0]);
	vecDownFontLine[0].GetEndPoint(pt1[1]);

	DPoint3d pt2[2];
	vecDownBackLine[0].GetStartPoint(pt2[0]);
	vecDownBackLine[0].GetEndPoint(pt2[1]);

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_heigth);

	m_heigth = m_heigth * uor_now / uor_ref;
	m_width = FrontStr.Distance(BackStr) * uor_now / uor_ref;
	m_length = FrontStr.Distance(FrontEnd) * uor_now / uor_ref;
	m_ptStart = FrontStr;
	m_ptEnd = FrontEnd;

	DPoint3d ptStart = m_ptStart;
	DPoint3d ptEnd = m_ptEnd;

	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
	//	CVector3D  yVecNegate = yVec;
	//	yVecNegate.Negate();
	//	yVecNegate.Normalize();
	//	yVecNegate.ScaleToLength(m_STwallData.width);//������յ�����Ϊǽ�������ߣ����Դ˴�ֻ��Ҫƫ��1/2���
	//	ptStart.Add(yVecNegate);
	//	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(ptStart, ptEnd);
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//����ΪX�ᣬˮƽ��ֱ����ΪY��
	SetPlacement(placement);

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return;
}

long CInsertRebarAssemblyColumn::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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


bool CInsertRebarAssemblyColumn::Rebuild()
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
bool CInsertRebarAssemblyColumn::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	SetSelectedModel(ehSel.GetModelRef());
	GetConcreteXAttribute(GetConcreteOwner(), ACTIVEMODEL);

	DgnModelRefP modelRef = ACTIVEMODEL;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_pInsertRebarMainDlg = new CInsertRebarMainDlg;
	m_pInsertRebarMainDlg->SetFirstItem(0);
	m_pInsertRebarMainDlg->SetSelectElement(ehSel);
	m_pInsertRebarMainDlg->SetBashElement(g_InsertElm);
	m_pInsertRebarMainDlg->SetConcreteId(FetchConcrete());
	m_pInsertRebarMainDlg->Create(IDD_DIALOG_InsertRebarMain);
	m_pInsertRebarMainDlg->ShowWindow(SW_SHOW);
	m_pInsertRebarMainDlg->m_PageColInserRebar.SetColInsertRebar(this);
	return true;
}