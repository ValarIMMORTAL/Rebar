// CAnchorinModify.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CAnchorinModify.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ElementAttribute.h"
#include "ConstantsDef.h"
#include "SingleRebarAssembly.h"
#include "SelectRebarTool.h"


// CAnchorinModify 对话框

IMPLEMENT_DYNAMIC(CAnchorinModify, CDialogEx)

CAnchorinModify::CAnchorinModify(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_AnchorinModify, pParent)
{
}

CAnchorinModify::~CAnchorinModify()
{
}

void CAnchorinModify::Push_RebarData()
{
	for (int i = 0; i < m_selectrebars.size(); i++)
	{
		ElementHandle eeh(m_selectrebars[i], ACTIVEMODEL);
		if (RebarElement::IsRebarElement(eeh))
		{
			double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

			RebarCurve curve;
			RebarElementP rep = RebarElement::Fetch(eeh);
			RebarSet* rebset = rep->GetRebarSet(ACTIVEMODEL);
			RebarShape * rebarshape = rep->GetRebarShape(ACTIVEMODEL);
			if (rebarshape != NULL)
			{
				rebarshape->GetRebarCurve(curve);
				BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
				m_vecDir.push_back(Sizekey);
				m_Diameter = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);

				CMatrix3D tmp3d(rep->GetLocation());
				curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
				curve.DoMatrix(rep->GetLocation());
				RebarVertices  vers = curve.PopVertices();
				m_rebarPts.push_back(vers);
			}
		}
	}

}

void CAnchorinModify::UpdateRebarData(vector<RebarVertices>& rebarPts)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	CButton* button = (CButton*)GetDlgItem(IDC_CHECK1);
	if (button->GetCheck() == 1) // 尾部
	{
		if (m_stAnchorinModifyInfo.anchorinStyle == 0) // 弯锚
		{
			for (int i = 0; i < rebarPts.size(); i++)
			{
				RebarVertices vers = rebarPts.at(i);
				if (vers.GetSize() < 3)
				{
					continue;
				}
				int nIndex = 1;
				RebarVertexP ver_next = vers.GetAt(vers.GetSize() - nIndex); // 下一个点 尾部最后一个点

				nIndex++;
				RebarVertexP ver = vers.GetAt(vers.GetSize() - nIndex);		// 中间点
				RebarVertexP ver_pre = vers.GetAt(vers.GetSize() - nIndex - 1);	// 上一个点

				DPoint3d ptStr_Pre, ptEnd_Pre;
				DPoint3d ptStr_Nex, ptEnd_Nex;

				ptStr_Nex = ver_next->GetIP();
				ptEnd_Nex = ver->GetIP();

				ptStr_Pre = ver_pre->GetIP();
				ptEnd_Pre = ver->GetArcPt(0);
				while (COMPARE_VALUES_EPS(ver_pre->GetRadius(), 0.00, EPS) != 0)
				{
					if (COMPARE_VALUES_EPS(ver->GetIP().Distance(ver_pre->GetIP()), 50.0 * uor_per_mm, EPS) > 0)
					{
						break;
					}
					nIndex++;
					if (vers.GetSize() - nIndex - 1 < 0)
					{
						break;
					}
					ver = vers.GetAt(vers.GetSize() - nIndex);
					ver_pre = vers.GetAt(vers.GetSize() - nIndex - 1);

					ptStr_Pre = ver_pre->GetIP();
					ptEnd_Pre = ver->GetArcPt(0);
				}

				CVector3D vecPre = ptStr_Pre - ptEnd_Pre;
				CVector3D vecNex = ptStr_Nex - ptEnd_Nex;
				vecPre.Normalize();
				vecNex.Normalize();

				CVector3D normal = CVector3D::From(0, 0, 0);
				mdlVec_crossProduct(&normal, &vecNex, &vecPre);

				CVector3D realNomal = CVector3D::From(0, 0, 0);
				mdlVec_crossProduct(&realNomal, &vecNex, &normal);

				DSegment3d seg_pre = { ptStr_Pre, ptEnd_Pre };
				DSegment3d seg_nex = { ptStr_Nex, ptEnd_Nex };
				DPoint3d ptIntersec;
				// 计算两条无限延生线段的交点
				if (SUCCESS == mdlVec_intersect(&ptIntersec, &seg_pre, &seg_nex))
				{
					DPoint3d ptStr = ptIntersec;
					DPoint3d ptEnd = ptStr;

					movePoint(vecNex, ptEnd, m_stAnchorinModifyInfo.anchorinLength * uor_per_mm);

					CMatrix3D mat = CMatrix3D::Rotate(ptStr, m_stAnchorinModifyInfo.anchorinAngle * (PI / 180.0), realNomal);

					Transform trans;
					mat.AssignTo(trans);
					TransformInfo transinfo(trans);
					EditElementHandle eehLine;
					LineHandler::CreateLineElement(eehLine, nullptr, DSegment3d::From(ptStr, ptEnd), true, *ACTIVEMODEL);
					eehLine.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehLine, transinfo);

					DPoint3d pt1[2];
					mdlLinear_extract(pt1, NULL, eehLine.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改

					bvector<DPoint3d> allpts;
					allpts.clear();
					for (int j = 0; j < vers.GetSize() - nIndex; j++)
					{
						allpts.push_back(rebarPts.at(i).At(j).GetIP());
					}
					allpts.push_back(ptIntersec);
					allpts.push_back(pt1[1]);
					RebarVertices  versTmp;
					BrString sizeKey = m_vecDir.at(i);
					double diameter = RebarCode::GetBarDiameter(sizeKey, ACTIVEMODEL);
					double BendRadius = RebarCode::GetPinRadius(sizeKey, ACTIVEMODEL,false);
					GetRebarVerticesFromPoints(versTmp, allpts, BendRadius);

					ptStr = versTmp.GetAt(versTmp.GetSize() - 1)->GetIP();
					ptEnd = versTmp.GetAt(versTmp.GetSize() - 2)->GetArcPt(2);

					CVector3D vec = ptStr - ptEnd;
					ptStr = ptEnd;
					movePoint(vec, ptStr, m_stAnchorinModifyInfo.anchorinLength * uor_per_mm);
					versTmp.GetAt(versTmp.GetSize() - 1)->SetIP(ptStr);

					rebarPts.at(i) = versTmp;
				}

			}
		}
		else // 直锚
		{
			for (int i = 0; i < rebarPts.size(); i++)
			{
				RebarVertices vers = rebarPts.at(i);
				if (vers.GetSize() < 3)
				{
					continue;
				}
				int nIndex = 1;
				RebarVertexP ver_next = vers.GetAt(vers.GetSize() - nIndex); // 下一个点 尾部最后一个点

				nIndex++;
				RebarVertexP ver = vers.GetAt(vers.GetSize() - nIndex);		// 中间点
				RebarVertexP ver_pre = vers.GetAt(vers.GetSize() - nIndex - 1);	// 上一个点

				DPoint3d ptStr_Pre, ptEnd_Pre;
				DPoint3d ptStr_Nex, ptEnd_Nex;

				ptStr_Nex = ver_next->GetIP();
				ptEnd_Nex = ver->GetIP();

				ptStr_Pre = ver_pre->GetIP();
				ptEnd_Pre = ver->GetArcPt(0);
				while (COMPARE_VALUES_EPS(ver_pre->GetRadius(), 0.00, EPS) != 0)
				{
					if (COMPARE_VALUES_EPS(ver->GetIP().Distance(ver_pre->GetIP()), 50.0 * uor_per_mm, EPS) > 0)
					{
						break;
					}
					nIndex++;
					if (vers.GetSize() - nIndex - 1 < 0)
					{
						break;
					}
					ver = vers.GetAt(vers.GetSize() - nIndex);
					ver_pre = vers.GetAt(vers.GetSize() - nIndex - 1);

					ptStr_Pre = ver_pre->GetIP();
					ptEnd_Pre = ver->GetArcPt(0);
				}

				DPoint3d ptStr = ptEnd_Pre;

				movePoint(ptEnd_Pre - ptStr_Pre, ptStr, m_stAnchorinModifyInfo.anchorinLength * uor_per_mm);

				bvector<DPoint3d> allpts;
				allpts.clear();
				for (int j = 0; j < vers.GetSize() - nIndex; j++)
				{
					allpts.push_back(rebarPts.at(i).At(j).GetIP());
				}

				if (COMPARE_VALUES_EPS(m_stAnchorinModifyInfo.anchorinLength, 0.0, EPS) != 0)
				{
					allpts.push_back(ptStr);
				}
				else
				{
					allpts.push_back(ptEnd_Pre);
				}

				RebarVertices  versTmp;
				BrString sizeKey = m_vecDir.at(i);
				double diameter = RebarCode::GetBarDiameter(sizeKey, ACTIVEMODEL);
				double BendRadius = RebarCode::GetPinRadius(sizeKey, ACTIVEMODEL, false);
				GetRebarVerticesFromPoints(versTmp, allpts, BendRadius);
				rebarPts.at(i) = versTmp;
			}
		}
	}
	else // 头部
	{
		if (m_stAnchorinModifyInfo.anchorinStyle == 0) // 弯锚
		{
			for (int i = 0; i < rebarPts.size(); i++)
			{
				RebarVertices vers = rebarPts.at(i);
				if (vers.GetSize() < 3)
				{
					continue;
				}

				int nIndex = 0;
				RebarVertexP ver_pre = vers.GetAt(nIndex); // 第一个点

				nIndex++;
				RebarVertexP ver = vers.GetAt(nIndex);		// 中间点
				RebarVertexP ver_nex = vers.GetAt(nIndex + 1);	// 下一个点

				DPoint3d ptStr_Pre, ptEnd_Pre;
				DPoint3d ptStr_Nex, ptEnd_Nex;

				ptStr_Pre = ver_pre->GetIP(); // 第一个点
				ptEnd_Pre = ver->GetIP();     // 第二个点

				ptStr_Nex = ver_nex->GetIP();
				ptEnd_Nex = ver->GetArcPt(2);
				while (COMPARE_VALUES_EPS(ver_nex->GetRadius(), 0.00, EPS) != 0)
				{
					if (COMPARE_VALUES_EPS(ver->GetIP().Distance(ver_nex->GetIP()), 50.0 * uor_per_mm, EPS) > 0)
					{
						break;
					}
					nIndex++;
					if (nIndex > vers.GetSize() - 1)
					{
						break;
					}
					ver = vers.GetAt(nIndex);
					ver_nex = vers.GetAt(nIndex + 1);

					ptStr_Nex = ver_nex->GetIP();
					ptEnd_Nex = ver->GetArcPt(2);
				}

				CVector3D vecPre = ptStr_Pre - ptEnd_Pre;
				CVector3D vecNex = ptEnd_Nex - ptStr_Nex;
				vecPre.Normalize();
				vecNex.Normalize();

				CVector3D normal = CVector3D::From(0, 0, 0);
				mdlVec_crossProduct(&normal, &vecNex, &vecPre);

				CVector3D realNomal = CVector3D::From(0, 0, 0);
				mdlVec_crossProduct(&realNomal, &vecPre, &normal);
				realNomal.Normalize();

				DSegment3d seg_pre = { ptStr_Pre, ptEnd_Pre };
				DSegment3d seg_nex = { ptStr_Nex, ptEnd_Nex };
				DPoint3d ptIntersec;
				// 计算两条无限延生线段的交点
				if (SUCCESS == mdlVec_intersect(&ptIntersec, &seg_pre, &seg_nex))
				{
					DPoint3d ptStr = ptIntersec;
					DPoint3d ptEnd = ptStr;

					movePoint(vecPre, ptEnd, m_stAnchorinModifyInfo.anchorinLength * uor_per_mm);

					CMatrix3D mat = CMatrix3D::Rotate(ptStr, m_stAnchorinModifyInfo.anchorinAngle * (PI / 180.0), realNomal);

					Transform trans;
					mat.AssignTo(trans);
					TransformInfo transinfo(trans);
					EditElementHandle eehLine;
					LineHandler::CreateLineElement(eehLine, nullptr, DSegment3d::From(ptStr, ptEnd), true, *ACTIVEMODEL);
					eehLine.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eehLine, transinfo);

					DPoint3d pt1[2];
					mdlLinear_extract(pt1, NULL, eehLine.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改

					bvector<DPoint3d> allpts;
					allpts.clear();
					allpts.push_back(pt1[1]);
					allpts.push_back(ptIntersec);
					for (int j = nIndex + 1; j < vers.GetSize(); j++)
					{
						allpts.push_back(rebarPts.at(i).At(j).GetIP());
					}
					RebarVertices  versTmp;
					BrString sizeKey = m_vecDir.at(i);
					double diameter = RebarCode::GetBarDiameter(sizeKey, ACTIVEMODEL);
					double BendRadius = RebarCode::GetPinRadius(sizeKey, ACTIVEMODEL, false);
					GetRebarVerticesFromPoints(versTmp, allpts, BendRadius);

					ptStr = versTmp.GetAt(0)->GetIP();
					ptEnd = versTmp.GetAt(1)->GetArcPt(0);

					CVector3D vec = ptStr - ptEnd;
					ptStr = ptEnd;
					movePoint(vec, ptStr, m_stAnchorinModifyInfo.anchorinLength * uor_per_mm);
					versTmp.GetAt(0)->SetIP(ptStr);

					rebarPts.at(i) = versTmp;
				}
			}
		}
		else // 直锚
		{
			for (int i = 0; i < rebarPts.size(); i++)
			{
				RebarVertices vers = rebarPts.at(i);
				if (vers.GetSize() < 3)
				{
					continue;
				}

				int nIndex = 0;
				RebarVertexP ver_pre = vers.GetAt(nIndex); // 第一个点

				nIndex++;
				RebarVertexP ver = vers.GetAt(nIndex);		// 中间点
				RebarVertexP ver_nex = vers.GetAt(nIndex + 1);	// 下一个点

				DPoint3d ptStr_Pre, ptEnd_Pre;
				DPoint3d ptStr_Nex, ptEnd_Nex;

				ptStr_Pre = ver_pre->GetIP(); // 第一个点
				ptEnd_Pre = ver->GetIP();     // 第二个点

				ptStr_Nex = ver_nex->GetIP();
				ptEnd_Nex = ver->GetArcPt(2);
				while (COMPARE_VALUES_EPS(ver_nex->GetRadius(), 0.00, EPS) != 0)
				{
					if (COMPARE_VALUES_EPS(ver->GetIP().Distance(ver_nex->GetIP()), 50.0 * uor_per_mm, EPS) > 0)
					{
						break;
					}
					nIndex++;
					if (nIndex > vers.GetSize() - 1)
					{
						break;
					}
					ver = vers.GetAt(nIndex);
					ver_nex = vers.GetAt(nIndex + 1);

					ptStr_Nex = ver_nex->GetIP();
					ptEnd_Nex = ver->GetArcPt(2);
				}

				DPoint3d ptStr = ver->GetArcPt(2);

				movePoint(ptStr_Pre - ptEnd_Pre, ptStr, m_stAnchorinModifyInfo.anchorinLength * uor_per_mm);

				bvector<DPoint3d> allpts;
				allpts.clear();
				if (COMPARE_VALUES_EPS(m_stAnchorinModifyInfo.anchorinLength, 0.0, EPS) != 0)
				{
					allpts.push_back(ptStr);
				}
				else
				{
					allpts.push_back(ptEnd_Nex);
				}

				for (int j = nIndex + 1; j < vers.GetSize(); j++)
				{
					allpts.push_back(rebarPts.at(i).At(j).GetIP());
				}

				RebarVertices  versTmp;
				BrString sizeKey = m_vecDir.at(i);
				double diameter = RebarCode::GetBarDiameter(sizeKey, ACTIVEMODEL);
				double BendRadius = RebarCode::GetPinRadius(sizeKey, ACTIVEMODEL, false);
				GetRebarVerticesFromPoints(versTmp, allpts, BendRadius);
				rebarPts.at(i) = versTmp;
			}
		}
	}
}

void CAnchorinModify::SetAnchorinModeifyInfo(bool bFlag)
{
	if (m_selectrebars.size() > 0)
	{
		ElementHandle eeh(m_selectrebars[0], ACTIVEMODEL);
		if (RebarElement::IsRebarElement(eeh))
		{
			double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

			RebarCurve curve;
			RebarElementP rep = RebarElement::Fetch(eeh);
			RebarSet* rebset = rep->GetRebarSet(ACTIVEMODEL);
			RebarShape * rebarshape = rep->GetRebarShape(ACTIVEMODEL);
			if (rebarshape != NULL)
			{
				rebarshape->GetRebarCurve(curve);
				BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
				m_Diameter = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL);

				CMatrix3D tmp3d(rep->GetLocation());
				curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
				curve.DoMatrix(rep->GetLocation());
				RebarVertices  vers = curve.PopVertices();

				for (int i = 0; i < vers.GetSize() - 1; i++)
				{
					if (i == vers.GetSize() - 2 && bFlag)
					{
						RebarVertexP ver = vers.GetAt(i); //第一个点
						RebarVertexP ver_next = vers.GetAt(i + 1); // 中间点

						int nIndex = 1;
						while (i - nIndex >= 0)
						{
							RebarVertexP ver_pre = vers.GetAt(i - nIndex);

							if (COMPARE_VALUES_EPS(ver_pre->GetRadius(), 0.00, EPS) == 0)
							{
								if (COMPARE_VALUES_EPS(ver->GetIP().Distance(ver_pre->GetIP()), ver->GetIP().Distance(ver_next->GetIP()), EPS) < 0)
								{
									m_stAnchorinModifyInfo.anchorinStyle = 1; // 直锚
									m_stAnchorinModifyInfo.anchorinLength = ver->GetArcPt(2).Distance(ver_next->GetIP()) / uor_per_mm;
									m_stAnchorinModifyInfo.anchorinAngle = 0.0;
								}
								break;
							}
							if (COMPARE_VALUES_EPS(ver->GetIP().Distance(ver_pre->GetIP()), 50.0 * uor_per_mm, EPS) > 0)
							{
								if (COMPARE_VALUES_EPS(ver->GetIP().Distance(ver_pre->GetIP()), ver->GetIP().Distance(ver_next->GetIP()), EPS) < 0)
								{
									m_stAnchorinModifyInfo.anchorinStyle = 1; // 直锚
									m_stAnchorinModifyInfo.anchorinLength = ver->GetArcPt(2).Distance(ver_next->GetIP()) / uor_per_mm;
									m_stAnchorinModifyInfo.anchorinAngle = 0.0;
								}
								break;
							}
							nIndex++;
						}

						// 前后两段 弧线 -- 直线
						if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) != 0 && COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) == 0)
						{
							m_stAnchorinModifyInfo.anchorinStyle = 0; // 弯锚
							m_stAnchorinModifyInfo.anchorinLength = ver->GetArcPt(2).Distance(ver_next->GetIP()) / uor_per_mm;
						
							m_stAnchorinModifyInfo.anchorinAngle = 0.0;
						}
						// 前后两段 直线 -- 直线
						else if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) == 0 && COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) == 0)
						{
							m_stAnchorinModifyInfo.anchorinStyle = 1; // 直锚
							m_stAnchorinModifyInfo.anchorinLength = ver->GetIP().Distance(ver_next->GetIP()) / uor_per_mm;
							m_stAnchorinModifyInfo.anchorinAngle = 0.0;
						}
					}

					if (i == 0 && !bFlag)
					{
						RebarVertexP ver = vers.GetAt(i); // 第一个点
						RebarVertexP ver_next = vers.GetAt(i + 1); // 第二个点

						int nIndex = 2;
						while (i + nIndex < vers.GetSize())
						{
							RebarVertexP ver_pre = vers.GetAt(i + nIndex); // 第三个点

							if (COMPARE_VALUES_EPS(ver_pre->GetRadius(), 0.00, EPS) == 0) // 直的
							{
								if (COMPARE_VALUES_EPS(ver_next->GetIP().Distance(ver_pre->GetIP()), ver_next->GetIP().Distance(ver->GetIP()), EPS) < 0)
								{
									m_stAnchorinModifyInfo.anchorinStyle = 1; // 直锚
									m_stAnchorinModifyInfo.anchorinLength = ver->GetArcPt(2).Distance(ver_next->GetIP()) / uor_per_mm;
									m_stAnchorinModifyInfo.anchorinAngle = 0.0;
								}
								break;
							}
							if (COMPARE_VALUES_EPS(ver_next->GetIP().Distance(ver_pre->GetIP()), 50.0 * uor_per_mm, EPS) > 0)
							{
								if (COMPARE_VALUES_EPS(ver_next->GetIP().Distance(ver_pre->GetIP()), ver_next->GetIP().Distance(ver->GetIP()), EPS) < 0)
								{
									m_stAnchorinModifyInfo.anchorinStyle = 1; // 直锚
									m_stAnchorinModifyInfo.anchorinLength = ver->GetArcPt(2).Distance(ver_next->GetIP()) / uor_per_mm;
									m_stAnchorinModifyInfo.anchorinAngle = 0.0;
								}
								break;
							}
							nIndex++;
						}


						if (i + 2 < vers.GetSize())
						{
							RebarVertexP ver_pre = vers.GetAt(i + 2); // 第三个点

							if (COMPARE_VALUES_EPS(ver_next->GetIP().Distance(ver->GetIP()), ver_next->GetIP().Distance(ver_pre->GetIP()), EPS) > 0)
							{
								m_stAnchorinModifyInfo.anchorinStyle = 1; // 直锚
								m_stAnchorinModifyInfo.anchorinLength = ver->GetArcPt(2).Distance(ver_next->GetIP()) / uor_per_mm;
								m_stAnchorinModifyInfo.anchorinAngle = 0.0;
								break;
							}
						}

						// 前后两段 直线 -- 直线
						if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) == 0 && COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) == 0)
						{
							m_stAnchorinModifyInfo.anchorinStyle = 1; // 直锚
							m_stAnchorinModifyInfo.anchorinLength = ver->GetIP().Distance(ver_next->GetIP()) / uor_per_mm;
							m_stAnchorinModifyInfo.anchorinAngle = 0.0;
						}
						// 前后两段 直线 -- 弧线
						else if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) == 0 && COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) != 0)
						{
							m_stAnchorinModifyInfo.anchorinStyle = 0; // 弯锚
							m_stAnchorinModifyInfo.anchorinLength = ver->GetIP().Distance(ver_next->GetArcPt(0)) /uor_per_mm;
							m_stAnchorinModifyInfo.anchorinAngle = 0.0;
						}
					}
				}
			}
		}
	}
}

// CInsertRebarMain 消息处理程序
BOOL CAnchorinModify::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	m_allLines.clear();
	m_rebarPts.clear();
	m_vecDir.clear();
	m_rebarPtsBack.clear();

	SetAnchorinModeifyInfo(false);
	Push_RebarData();
	
	m_rebarPtsBack.clear();
	m_rebarPtsBack.insert(m_rebarPtsBack.end(), m_rebarPts.begin(), m_rebarPts.end());
	DarwRebarLine(m_rebarPtsBack);

	CString strTmp = CString();
	strTmp.Format(_T("%.2f"), m_stAnchorinModifyInfo.anchorinLength);
	m_EditLength.SetWindowText(strTmp);

	strTmp.Format(_T("%.2f"), m_stAnchorinModifyInfo.anchorinAngle);
	m_EditAngle.SetWindowText(strTmp);

	for (auto str : g_listAnchorinStyle)
	{
		m_CombStyle.AddString(str);
	}
	m_CombStyle.SetCurSel(m_stAnchorinModifyInfo.anchorinStyle);
	if (m_stAnchorinModifyInfo.anchorinStyle == 1)
	{
		m_CombStyle.EnableWindow(FALSE);
	}

	return true;
}

void CAnchorinModify::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT1, m_EditAngle);
	DDX_Control(pDX, IDC_EDIT2, m_EditLength);
	DDX_Control(pDX, IDC_COMBO1, m_CombStyle);
}


BEGIN_MESSAGE_MAP(CAnchorinModify, CDialogEx)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CAnchorinModify::OnCbnSelchangeComboRtype)
	ON_EN_KILLFOCUS(IDC_EDIT1, &CAnchorinModify::OnEnKillfocusEdit1)
	ON_EN_KILLFOCUS(IDC_EDIT2, &CAnchorinModify::OnEnKillfocusEdit2)
	ON_BN_CLICKED(IDC_CHECK1, &CAnchorinModify::OnBnClickedCheck1)
	ON_BN_CLICKED(IDOK, &CAnchorinModify::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CAnchorinModify::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON1, &CAnchorinModify::OnBnClickedButton1)
END_MESSAGE_MAP()


// CAnchorinModify 消息处理程序


void CAnchorinModify::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	CButton* button = (CButton*)GetDlgItem(IDC_CHECK1);
	if (button->GetCheck() == 1)
	{
		SetAnchorinModeifyInfo();
	}
	else
	{
		SetAnchorinModeifyInfo(false);
	}
	m_CombStyle.EnableWindow(TRUE);
	CString strTmp = CString();
	strTmp.Format(_T("%.2f"), m_stAnchorinModifyInfo.anchorinLength);
	m_EditLength.SetWindowText(strTmp);

	strTmp.Format(_T("%.2f"), m_stAnchorinModifyInfo.anchorinAngle);
	m_EditAngle.SetWindowText(strTmp);

	m_CombStyle.SetCurSel(m_stAnchorinModifyInfo.anchorinStyle);
	if (m_stAnchorinModifyInfo.anchorinStyle == 1)
	{
		m_CombStyle.EnableWindow(FALSE);
	}
	else
	{
		m_rebarPtsBack.clear();
		m_rebarPtsBack.insert(m_rebarPtsBack.end(), m_rebarPts.begin(), m_rebarPts.end());
		UpdateRebarData(m_rebarPtsBack);
		DarwRebarLine(m_rebarPtsBack);
	}
}

void CAnchorinModify::DarwRebarLine(const vector<RebarVertices>& m_rebarPts)
{
	ClearLines();

	DgnModelRefP modelRef = ACTIVEMODEL;
	EditElementHandle lineEeh;

	for (auto vers : m_rebarPts)
	{
		ChainHeaderHandler::CreateChainHeaderElement(lineEeh, NULL, false, true, *modelRef);
		for (int i = 0; i < (int)vers.GetSize() - 1; i++)
		{
			RebarVertexP ver = vers.GetAt(i);
			RebarVertexP ver_next = vers.GetAt(i + 1);

			// 前后两段 直线 -- 直线
			if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) == 0 && COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) == 0)
			{
				EditElementHandle eehTmp;
				if (SUCCESS != LineHandler::CreateLineElement(eehTmp, NULL, DSegment3d::From(ver->GetIP(), ver_next->GetIP()), true, *modelRef))
				{
					return;
				}
				ChainHeaderHandler::AddComponentElement(lineEeh, eehTmp);
			}
			// 前后两段 直线 -- 弧线
			else if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) == 0 && COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) != 0)
			{
				EditElementHandle eehTmp;
				if (SUCCESS != LineHandler::CreateLineElement(eehTmp, NULL, DSegment3d::From(ver->GetIP(), ver_next->GetArcPt()[0]), true, *modelRef))
				{
					return;
				}
				ChainHeaderHandler::AddComponentElement(lineEeh, eehTmp);
			}
			// 前后两段 弧线 -- 直线
			else if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) != 0 && COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) == 0)
			{
				EditElementHandle eehArc;
				//画圆弧
				if (SUCCESS != ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(ver->GetCenter(), ver->GetArcPt()[0], ver->GetArcPt()[2]), modelRef->Is3d(), *modelRef))
				{
					return;
				}
				ChainHeaderHandler::AddComponentElement(lineEeh, eehArc);

				EditElementHandle eehTmp;
				if (SUCCESS != LineHandler::CreateLineElement(eehTmp, NULL, DSegment3d::From(ver->GetArcPt(2), ver_next->GetIP()), true, *modelRef))
				{
					return;
				}
				ChainHeaderHandler::AddComponentElement(lineEeh, eehTmp);
			}
			// 前后两段 弧线 -- 弧线
			else if (COMPARE_VALUES_EPS(ver->GetRadius(), 0.00, EPS) != 0 && COMPARE_VALUES_EPS(ver_next->GetRadius(), 0.00, EPS) != 0)
			{
				EditElementHandle eehArc;
				//画圆弧
				if (SUCCESS != ArcHandler::CreateArcElement(eehArc, NULL, DEllipse3d::FromArcCenterStartEnd(ver->GetCenter(), ver->GetArcPt()[0], ver->GetArcPt()[2]), modelRef->Is3d(), *modelRef))
				{
					return;
				}
				ChainHeaderHandler::AddComponentElement(lineEeh, eehArc);

				EditElementHandle eehArcTmp;
				//画圆弧
				if (SUCCESS != ArcHandler::CreateArcElement(eehArcTmp, NULL, DEllipse3d::FromArcCenterStartEnd(ver_next->GetCenter(), ver_next->GetArcPt()[0], ver_next->GetArcPt()[2]), modelRef->Is3d(), *modelRef))
				{
					return;
				}
				ChainHeaderHandler::AddComponentElement(lineEeh, eehArc);
			}
		}

		ChainHeaderHandler::AddComponentComplete(lineEeh);
		lineEeh.AddToModel();

		m_allLines.push_back(lineEeh.GetElementRef());
	}

}

void CAnchorinModify::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

void CAnchorinModify::ClearLines()
{
	for (ElementRefP tmpeeh : m_allLines)
	{
		if (tmpeeh != nullptr)
		{
			EditElementHandle eeh(tmpeeh, tmpeeh->GetDgnModelP());
			eeh.DeleteFromModel();
		}
	}
}

void CAnchorinModify::OnCbnSelchangeComboRtype()
{
	m_stAnchorinModifyInfo.anchorinStyle = m_CombStyle.GetCurSel();

	m_rebarPtsBack.clear();
	m_rebarPtsBack.insert(m_rebarPtsBack.end(), m_rebarPts.begin(), m_rebarPts.end());
	UpdateRebarData(m_rebarPtsBack);
	DarwRebarLine(m_rebarPtsBack);
}

// 角度
void CAnchorinModify::OnEnKillfocusEdit1()
{
	CString strTmp = CString();
	m_EditAngle.GetWindowText(strTmp);
	m_stAnchorinModifyInfo.anchorinAngle = atof(CT2A(strTmp));

	m_rebarPtsBack.clear();
	m_rebarPtsBack.insert(m_rebarPtsBack.end(), m_rebarPts.begin(), m_rebarPts.end());
	UpdateRebarData(m_rebarPtsBack);
	DarwRebarLine(m_rebarPtsBack);

	return;
}

// 长度
void CAnchorinModify::OnEnKillfocusEdit2()
{

	CString strTmp = CString();
	m_EditLength.GetWindowText(strTmp);
	m_stAnchorinModifyInfo.anchorinLength = atof(CT2A(strTmp));

	m_rebarPtsBack.clear();
	m_rebarPtsBack.insert(m_rebarPtsBack.end(), m_rebarPts.begin(), m_rebarPts.end());
	UpdateRebarData(m_rebarPtsBack);
	DarwRebarLine(m_rebarPtsBack);

	return;
}



void CAnchorinModify::OnBnClickedOk()
{
	RebarSet * rebset = nullptr;
	EditElementHandle start(m_selectrebars[0], m_selectrebars[0]->GetDgnModelP());
	if (RebarElement::IsRebarElement(start))
	{
		RebarElementP rep = RebarElement::Fetch(start);
		rebset = rep->GetRebarSet(ACTIVEMODEL);
		if (rebset != nullptr&&m_rebarPts.size() > 0 && m_rebarPts.size() == m_vecDir.size())
		{
			ElementId conid;
			int rebar_cage_type;
			conid = rebset->GetConcreteOwner(ACTIVEMODEL, rebar_cage_type);
			/*RebarModel *rmv = RMV;
			BeConcreteData condata;
			int rebar_cage_type;
			if (rmv != nullptr)
			{
				rmv->GetConcreteData(*rep, rep->GetModelRef(), condata, rebar_cage_type);
			}

			ElementId conid = 0;
			conid = condata.GetRexId().GetElementId();*/
			RebarAssemblies reas;
			RebarAssembly::GetRebarAssemblies(conid, reas);

			// WaitForSingleObject(this->m_hWndTop, INFINITE);
			RebarAssembly* rebarasb = nullptr;
			for (int i = 0; i < reas.GetSize(); i++)
			{
				RebarAssembly* rebaras = reas.GetAt(i);
				if (rebaras->GetCallerId() == rebset->GetCallerId())
				{
					rebarasb = rebaras;
				}

			}
			if (rebarasb != nullptr)
			{
				DgnModelRefP        modelRef = ACTIVEMODEL;
				SingleRebarAssembly*  singleRebar = REA::Create<SingleRebarAssembly>(modelRef);

				ElementId tmpid = rebarasb->GetSelectedElement();
				if (tmpid == 0)
				{
					return;
				}
				DgnModelRefP modelp = rebarasb->GetSelectedModel();
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

				m_rebarPts.clear();
				m_rebarPts.insert(m_rebarPts.end(), m_rebarPtsBack.begin(), m_rebarPtsBack.end());
				singleRebar->SetSelectedRebar(start);
				singleRebar->SetSlabData(ehSel);
				singleRebar->SetvecDirSize(m_vecDir);
				singleRebar->SetrebarPts(m_rebarPts);
				singleRebar->Setspacing(rebset->GetSetData().GetNominalSpacing());
				singleRebar->SetConcreteOwner(conid);
				singleRebar->MakeRebars(modelRef);
				singleRebar->Save(modelRef); // must save after creating rebars
			}
			RebarModel *rmv = RMV;
			for (ElementRefP tref : m_selectrebars)
			{
				EditElementHandle tmpeeh(tref, tref->GetDgnModelP());
				RebarElementP rep = RebarElement::Fetch(tmpeeh);
				rmv->Delete(*rep,ACTIVEMODEL);
				//tmpeeh.DeleteFromModel();
			}
		}
	}

	ClearLines();
	m_allLines.clear();
	m_rebarPts.clear();
	m_vecDir.clear();
	m_rebarPtsBack.clear();
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}


void CAnchorinModify::OnBnClickedCancel()
{
	ClearLines();
	m_allLines.clear();
	m_rebarPts.clear();
	m_vecDir.clear();
	m_rebarPtsBack.clear();
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}


void CAnchorinModify::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
		// TODO: 在此添加控件通知处理程序代码
	CString sTmp = CString();
	double dAngle = m_stAnchorinModifyInfo.anchorinAngle;
	
	dAngle += 90.0;
	if (COMPARE_VALUES_EPS(dAngle, 360.0, EPS) >= 0)
	{
		dAngle -= 360.0;
	}
		
	sTmp.Format(_T("%.2f"), dAngle);
	m_EditAngle.SetWindowText(sTmp);
	m_stAnchorinModifyInfo.anchorinAngle = dAngle;

	m_rebarPtsBack.clear();
	m_rebarPtsBack.insert(m_rebarPtsBack.end(), m_rebarPts.begin(), m_rebarPts.end());
	UpdateRebarData(m_rebarPtsBack);
	DarwRebarLine(m_rebarPtsBack);
}
