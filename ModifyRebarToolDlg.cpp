// ModifyRebarToolDlg.cpp: 实现文件
//

#include "_ustation.h"
#include "GalleryIntelligentRebar.h"
#include "ModifyRebarToolDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "SelectRebarTool.h"
#include "ConstantsDef.h"
#include "ExtractFacesTool.h"


// ModifyRebarToolDlg 对话框

IMPLEMENT_DYNAMIC(ModifyRebarToolDlg, CDialogEx)

ModifyRebarToolDlg::ModifyRebarToolDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_ModifyRebarTool, pParent)
{

}

ModifyRebarToolDlg::~ModifyRebarToolDlg()
{
}

void ModifyRebarToolDlg::SetSelectRebars(const vector<ElementRefP>& rebars)
{
	m_selectrebars = rebars;
	//EditElementHandle rebarEeh(m_selectRefRebar.at(0), ACTIVEMODEL);
	//Init(rebarEeh);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	EditElementHandle start(*m_selectrebars.begin(), (*m_selectrebars.begin())->GetDgnModelP());
	DPoint3d ptstr, ptend; //首尾钢筋的端点
	double diameter = 0;
	GetStartEndPointFromRebar(&start, ptstr, ptend, diameter);
	DVec3d vec = ptstr - ptend;
	DVec3d refVec = m_refRebarInfo.startPt - m_refRebarInfo.endPt;
	vec.Normalize();
	refVec.Normalize();
	string rebarType; //钢筋类型
	GetRebarLevelItemTypeValue(start, rebarType);
	if (rebarType == "back")
	{
		vec.Negate();
	}
	m_refRebarInfo.verVec.CrossProduct(vec, refVec);

	CalculateModifyRebars();
	PreviewRefLines();
}

void ModifyRebarToolDlg::SetSelectRefRebars(const ElementRefP& rebar)
{
	m_selectRefRebar = rebar;
	//根据参考钢筋获取参考钢筋信息
	EditElementHandle eeh(rebar->GetElementId(), rebar->GetDgnModelP());
	GetStartEndPointFromRebar(&eeh, m_refRebarInfo.startPt, m_refRebarInfo.endPt, m_refRebarInfo.diameter);
}

void ModifyRebarToolDlg::Init(EditElementHandle& rebarEeh)
{
	if (RebarElement::IsRebarElement(rebarEeh)) //是否为钢筋元素
	{
		RebarElementP rep = RebarElement::Fetch(rebarEeh);
		RebarSet* rebset = rep->GetRebarSet(ACTIVEMODEL);
		//(*elementToModify).GetModelRef()：从ElementAgenda的元素中提取DgnModelRef
		//DgnModelRef提供了对Bentley::DgnPlatform::DgnFile中的模型的访问
		//获取钢筋模板对象
		RebarShape * rebarshape = rep->GetRebarShape((rebarEeh).GetModelRef());
		if (rebarshape == nullptr)
			return;
		//获取钢筋模板中的线条形状
		RebarCurve curve;
		rebarshape->GetRebarCurve(curve);
		//获取钢筋的尺寸数据
		CString sizekey = rebarshape->GetShapeData().GetSizeKey();

		CString dia = sizekey.Left(sizekey.GetLength() - 1);
		CString grade = sizekey.Right(1);

		//设置等级
		if (grade == L"A")
			m_rebarGrade = 0;
		else if (grade == L"B")
			m_rebarGrade = 1;
		else if (grade == L"C")
			m_rebarGrade = 2;
		else
			m_rebarGrade = 3;
		m_rebarGradeCombo.SetCurSel(m_rebarGrade);//型号

		//设置直径
		if (dia.Find(L"mm") == -1)
		{
			dia += "mm";
			m_rebarDia = CT2A(dia);
		}
		int nIndex = m_rebarDiaCombo.FindStringExact(0, dia);
		m_rebarDiaCombo.SetCurSel(nIndex);//尺寸	

		//设置间隔
		m_spacing = 200;
		m_rebarsSpaceEdit.SetWindowTextW(std::to_wstring(m_spacing).c_str());
	}
}

void ModifyRebarToolDlg::CalculateModifyRebars()
{
	if (m_selectrebars.size() == 0)
	{
		return;
	}
	//选择一根钢筋
	if (m_selectrebars.size() == 1)
	{
		m_modifyRebars[0].push_back(m_selectrebars.at(0));
		m_updateRebars[m_selectrebars.at(0)->GetElementId()] = m_selectrebars.at(0);
	}

	DPoint3d startRebarPt, endRebarPt; //首尾钢筋的端点在参考钢筋线上的投影
	EditElementHandle start(*m_selectrebars.begin(), (*m_selectrebars.begin())->GetDgnModelP());
	DPoint3d ptstr, ptend; //首尾钢筋的端点
	double diameter = 0;
	GetStartEndPointFromRebar(&start, ptstr, ptend, diameter);
	mdlVec_projectPointToLine(&startRebarPt, nullptr, &ptstr, &m_refRebarInfo.startPt, &m_refRebarInfo.endPt);
	EditElementHandle end(*m_selectrebars.rbegin(), (*m_selectrebars.rbegin())->GetDgnModelP());
	GetStartEndPointFromRebar(&end, ptstr, ptend, diameter);
	mdlVec_projectPointToLine(&endRebarPt, nullptr, &ptstr, &m_refRebarInfo.startPt, &m_refRebarInfo.endPt);

	//将需要修改的钢筋按照距离分类
	auto Calculate = [&](const ElementRefP& rebarRef, const bool isInLine) {	
		EditElementHandle rebar(rebarRef, rebarRef->GetDgnModelP());
		DPoint3d ptstr, ptend; //钢筋端点
		double diameter = 0; //钢筋直径
		GetStartEndPointFromRebar(&rebar, ptstr, ptend, diameter);
		DPoint3d outPt; //钢筋起点在首尾钢筋投影连线上的投影点
		StatusInt ret = mdlVec_projectPointToLine(&outPt, nullptr, &ptstr, &startRebarPt, &endRebarPt);
		if (ret == SUCCESS)
		{
			if (isInLine) //如果需要判断是否在首尾之间
			{
				bool bFlag = false;
				if (!EFT::IsPointInLine(outPt, startRebarPt, endRebarPt, ACTIVEMODEL, bFlag)) //点是否在线上
				{
					return;
				}
			}
			double dis = outPt.Distance(startRebarPt); //投影点距离首根钢筋的距离
			vector<ElementRefP> rebars;
			auto findIt = m_modifyRebars.find(dis);
			if (findIt != m_modifyRebars.end())
			{
				rebars = findIt->second;
			}
			rebars.push_back(rebarRef);
			m_modifyRebars[dis] = rebars;
			m_updateRebars[rebarRef->GetElementId()] = rebarRef;
		}
	};

	//钢筋数量不为2
	if (m_selectrebars.size() != 2)
	{
		for (auto it : m_selectrebars)
		{		
			Calculate(it, false);
		}
		return;
	}

	//选择2根钢筋
	if (RebarElement::IsRebarElement(start))
	{
		//获取钢筋集合
		RebarElementP rep = RebarElement::Fetch(start);
		RebarSet* rebset = rep->GetRebarSet(ACTIVEMODEL);
		int nNum = (int)rebset->GetChildElementCount(); // 钢筋组中钢筋数量
		//遍历钢筋集合中的钢筋来得到需要修改的钢筋
		for (int j = 0; j < nNum; j++)
		{
			RebarElementP pRebar = rebset->GetChildElement(j);
			if (pRebar->GetElementRef() == nullptr)
			{
				continue;
			}
			Calculate(pRebar->GetElementRef(), true);
		}
	}
}

void ModifyRebarToolDlg::ClearRefLine()
{
	for (ElementRefP tmpeeh : m_refLines)
	{
		if (tmpeeh != nullptr)
		{
			EditElementHandle eeh(tmpeeh, tmpeeh->GetDgnModelP());
			eeh.DeleteFromModel();
		}
	}
	m_refLines.clear();
	m_refLines.clear();
	m_rebarVertices.clear();
}

void ModifyRebarToolDlg::GetRebarVerticies()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	map<ElementId, RebarVertices> rebarVertices; //选择钢筋的端点数据
	vector<RebarVertices> selectVertices;
	for (auto it : m_selectrebars)
	{
		EditElementHandle rebarEeh(it, ACTIVEMODEL);
		if (RebarElement::IsRebarElement(rebarEeh))
		{
			RebarElementP rep = RebarElement::Fetch(rebarEeh);
			DPoint3d ptstr, ptend;
			double diameter = 0;
			GetStartEndPointFromRebar(&rebarEeh, ptstr, ptend, diameter);
			double diaDis = stof(m_rebarDia) * uor_per_mm - diameter; //中心点需要往参考钢筋反向移动的距离，为了让钢筋贴着参考钢筋
			DVec3d vec = m_refRebarInfo.verVec; //移动方向
			vec.Normalize();
			vec.Scale(diaDis / 2);

			RebarShape * rebarshape = rep->GetRebarShape((rebarEeh).GetModelRef());
			if (rebarshape == nullptr)
				continue;		
			RebarCurve curve;//获取钢筋模板中的线条形状
			rebarshape->GetRebarCurve(curve);
			CMatrix3D tmp3d(rep->GetLocation());
			curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
			curve.DoMatrix(rep->GetLocation());
			RebarVertices  vers;
			RebarVertices oldVers = curve.GetVertices();
			for (size_t i = 0; i < oldVers.GetSize(); ++i)
			{
				RebarVertex *tmpVertex = &oldVers.At(i);
				if (tmpVertex->GetType() == RebarVertex::kStart || tmpVertex->GetType() == RebarVertex::kEnd)
				{
					DPoint3d tmpPt = tmpVertex->GetIP();
					tmpPt.Add(vec);
					tmpVertex->SetIP(tmpPt);
				}
				else
				{
					DPoint3d tmpPt = tmpVertex->GetIP();
					tmpPt.Add(vec);
					DPoint3d arcPt1 = tmpVertex->GetArcPt(0);
					arcPt1.Add(vec);
					DPoint3d arcPt2 = tmpVertex->GetArcPt(1);
					arcPt2.Add(vec);
					DPoint3d arcPt3 = tmpVertex->GetArcPt(2);
					arcPt3.Add(vec);
					DPoint3d centerPt = tmpVertex->GetCenter();
					centerPt.Add(vec);
					tmpVertex->SetIP(tmpPt);
					tmpVertex->SetArcPt(0, arcPt1);
					tmpVertex->SetArcPt(1, arcPt2);
					tmpVertex->SetArcPt(2, arcPt3);
					tmpVertex->SetCenter(centerPt);
				}
				
				vers.Add(tmpVertex);
			}

			selectVertices.push_back(vers);
			rebarVertices[it->GetElementId()] = vers;
		}
	}
	mdlSelect_freeAll();
	//选择2根钢筋
	if (m_selectrebars.size() == 2)
	{
		DPoint3d tmpStartPt = (*selectVertices.begin()).At(0).GetIP(); //首根钢筋的端点
		DPoint3d tmpEndPt = (*selectVertices.rbegin()).At(0).GetIP(); //最后钢筋的端点
		DPoint3d vecStartPt, vecEndPt; //首尾钢筋在参考钢筋线上的投影点
		mdlVec_projectPointToLine(&vecStartPt, nullptr, &tmpStartPt, &m_refRebarInfo.startPt, &m_refRebarInfo.endPt);
		mdlVec_projectPointToLine(&vecEndPt, nullptr, &tmpEndPt, &m_refRebarInfo.startPt, &m_refRebarInfo.endPt);
		DVec3d vec = vecEndPt - vecStartPt; //尾部钢筋到首部钢筋的方向，用来移动钢筋
		vec.Normalize();
		vec.Scale(m_spacing * uor_per_mm); //拉伸钢筋间距
		int index = 0; //移动次数
		for (auto it : m_modifyRebars)
		{
			double dis = it.first; //与首根钢筋的距离
			DVec3d negateVec = vec; //与首尾钢筋方向相反的方向
			negateVec.Normalize();
			negateVec.Negate();
			negateVec.Scale(dis); //拉伸反向向量
			for (auto rebarIt : it.second)
			{
				EditElementHandle rebarEeh(rebarIt, ACTIVEMODEL);
				if (RebarElement::IsRebarElement(rebarEeh))
				{
					RebarElementP rep = RebarElement::Fetch(rebarEeh);
					DPoint3d ptstr, ptend;
					double diameter = 0;
					GetStartEndPointFromRebar(&rebarEeh, ptstr, ptend, diameter);
					//先将钢筋移动到首根钢筋的位置，再往尾端移动间隔，得到新的位置
					ptstr.Add(negateVec);
					ptend.Add(negateVec);
					ptstr.Add(vec * index);
					ptend.Add(vec * index);
					DPoint3d outPt; //新的位置在参考钢筋线上的投影点
					//忽略超过参考钢筋范围钢筋
					StatusInt ret = mdlVec_projectPointToLine(&outPt, nullptr, &ptstr, &m_refRebarInfo.startPt, &m_refRebarInfo.endPt);
					if (SUCCESS != ret) 
					{
						SelectionSetManager::GetManager().AddElement(rebarEeh.GetElementRef(), ACTIVEMODEL);
						continue;
					}
					bool bFlag = false;
					if (!EFT::IsPointInLine(outPt, m_refRebarInfo.startPt, m_refRebarInfo.endPt, ACTIVEMODEL, bFlag)) //点是否在线上
					{
						SelectionSetManager::GetManager().AddElement(rebarEeh.GetElementRef(), ACTIVEMODEL);
						continue;
					}

					//将钢筋沿着参考钢筋垂直的方向移动直径差，以此让新的钢筋靠在参考钢筋上
					double diaDis = stof(m_rebarDia) * uor_per_mm - diameter;
					DVec3d verVec = m_refRebarInfo.verVec;
					verVec.Normalize();
					verVec.Scale(diaDis / 2);
					ptstr.Add(verVec);
					ptend.Add(verVec);

					RebarShape * rebarshape = rep->GetRebarShape((rebarEeh).GetModelRef());
					if (rebarshape == nullptr)
						continue;
					RebarCurve curve;//获取钢筋模板中的线条形状
					rebarshape->GetRebarCurve(curve);
					CMatrix3D tmp3d(rep->GetLocation());
					curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
					curve.DoMatrix(rep->GetLocation());
					RebarVertices  vers;
					RebarVertices oldVers = curve.GetVertices();
					for (size_t i = 0; i < oldVers.GetSize(); ++i)
					{
						RebarVertex* tmpVertex = &oldVers.At(i);
						if (tmpVertex->GetType() == RebarVertex::kStart || tmpVertex->GetType() == RebarVertex::kEnd)
						{
							DPoint3d tmpPt = tmpVertex->GetIP();
							tmpPt.Add(negateVec);
							tmpPt.Add(vec * index);
							tmpPt.Add(verVec);
							tmpVertex->SetIP(tmpPt);
						}
						else
						{
							DPoint3d tmpPt = tmpVertex->GetIP();
							tmpPt.Add(negateVec);
							tmpPt.Add(vec * index);
							tmpPt.Add(verVec);
							DPoint3d arcPt1 = tmpVertex->GetArcPt(0);
							arcPt1.Add(negateVec);
							arcPt1.Add(vec * index);
							arcPt1.Add(verVec);
							DPoint3d arcPt2 = tmpVertex->GetArcPt(1);
							arcPt2.Add(negateVec);
							arcPt2.Add(vec * index);
							arcPt2.Add(verVec);
							DPoint3d arcPt3 = tmpVertex->GetArcPt(2);
							arcPt3.Add(negateVec);
							arcPt3.Add(vec * index);
							arcPt3.Add(verVec);
							DPoint3d centerPt = tmpVertex->GetCenter();
							centerPt.Add(negateVec);
							centerPt.Add(vec * index);
							centerPt.Add(verVec);
							tmpVertex->SetIP(tmpPt);
							tmpVertex->SetArcPt(0, arcPt1);
							tmpVertex->SetArcPt(1, arcPt2);
							tmpVertex->SetArcPt(2, arcPt3);
							tmpVertex->SetCenter(centerPt);
						}
						
						vers.Add(tmpVertex);
					}
				
					m_rebarVertices[rebarIt->GetElementId()] = vers;
				}
			}
			index++;
		}
	}
	else
	{
		m_rebarVertices = rebarVertices;
	}

	{//删除钢筋处理
		mdlInput_sendSynchronizedKeyin(L"proconcrete delete rebar", 0, INPUTQ_EOQ, NULL);
		mdlInput_sendSynchronizedKeyin(L"choose element", 0, INPUTQ_EOQ, NULL);
	}

}

void ModifyRebarToolDlg::PreviewRefLines()
{
	ClearRefLine();
	GetRebarVerticies();
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	for (auto vers : m_rebarVertices)
	{
		for (int i = 0; i < vers.second.GetSize() - 1; i++)
		{
			//绘制直线
			RebarVertex   ver1 = vers.second.At(i);
			RebarVertex   ver2 = vers.second.At(i + 1);
			CPoint3D    pt1 = ver1.GetIP();
			CPoint3D    pt2 = ver2.GetIP();
			DPoint3d tpt1 = DPoint3d::From(pt1.x, pt1.y, pt1.z);
			DPoint3d tpt2 = DPoint3d::From(pt2.x, pt2.y, pt2.z);
			EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(tpt1, tpt2), true, *ACTIVEMODEL);
			eeh.AddToModel();

			//绘制圆
			EditElementHandle eeharc;
			double rebarDia = stof(m_rebarDia) * uor_per_mm;
			DVec3d vec = tpt1 - tpt2;
			ArcHandler::CreateArcElement(eeharc, nullptr, DEllipse3d::FromCenterNormalRadius(tpt1, vec, rebarDia / 2), true, *ACTIVEMODEL);
			eeharc.AddToModel();

			m_refLines.push_back(eeharc.GetElementRef());
			m_refLines.push_back(eeh.GetElementRef());
		}
	}
}

void ModifyRebarToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_Grade, m_rebarGradeCombo);
	DDX_Control(pDX, IDC_COMBO_Diameter, m_rebarDiaCombo);
	DDX_Control(pDX, IDC_EDIT_Space, m_rebarsSpaceEdit);
}


BOOL ModifyRebarToolDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端

	for each (auto var in g_listRebarType)
		m_rebarGradeCombo.AddString(var);

	for each (auto var in g_listRebarSize)
		m_rebarDiaCombo.AddString(var);

	EditElementHandle rebarEeh(m_selectRefRebar, ACTIVEMODEL);
	Init(rebarEeh);

	return TRUE;  // return TRUE unless you set the focus to a control
			  // 异常: OCX 属性页应返回 FALSE
}

BEGIN_MESSAGE_MAP(ModifyRebarToolDlg, CDialogEx)
	ON_CBN_SELCHANGE(IDC_COMBO_Grade, &ModifyRebarToolDlg::OnCbnSelchangeComboGrade)
	ON_CBN_SELCHANGE(IDC_COMBO_Diameter, &ModifyRebarToolDlg::OnCbnSelchangeComboDiameter)
	ON_BN_CLICKED(IDOK, &ModifyRebarToolDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &ModifyRebarToolDlg::OnBnClickedCancel)
	ON_EN_CHANGE(IDC_EDIT_Space, &ModifyRebarToolDlg::OnEnChangeEditSpace)
	ON_BN_CLICKED(IDC_BUTTON_Select, &ModifyRebarToolDlg::OnBnClickedButtonSelect)
END_MESSAGE_MAP()


// ModifyRebarToolDlg 消息处理程序


void ModifyRebarToolDlg::OnCbnSelchangeComboGrade()
{
	m_rebarGrade = m_rebarGradeCombo.GetCurSel();
	PreviewRefLines();
}


void ModifyRebarToolDlg::OnCbnSelchangeComboDiameter()
{
	int nIndex = m_rebarDiaCombo.GetCurSel();
	CString rebarDiaStr;
	m_rebarDiaCombo.GetLBText(nIndex, rebarDiaStr);
	if (rebarDiaStr.Find(L"mm") != -1)
		rebarDiaStr.Replace(L"mm", L"");
	m_rebarDia = CT2A(rebarDiaStr);
	PreviewRefLines();
}

void ModifyRebarToolDlg::OnBnClickedOk()
{
	if (m_rebarVertices.size() == 0)
	{
		GetRebarVerticies();
	}
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	for (auto it : m_updateRebars)
	{
		auto findIt = m_rebarVertices.find(it.first);
		if (findIt == m_rebarVertices.end())
		{
			continue;
		}
		ElementHandle eeh(it.first, it.second->GetDgnModelP());
		if (RebarElement::IsRebarElement(eeh)) //是否为钢筋元素
		{
			//从EditElementHandle获取元素
			RebarElementP rep = RebarElement::Fetch(eeh);
			//(*elementToModify).GetModelRef()：从ElementAgenda的元素中提取DgnModelRef
			//DgnModelRef提供了对Bentley::DgnPlatform::DgnFile中的模型的访问
			//获取钢筋模板对象
			RebarShape * rebarshape = rep->GetRebarShape((eeh).GetModelRef());
			if (rebarshape == nullptr)
				continue;
			//获取钢筋模板中的线条形状
			RebarCurve curve;
			rebarshape->GetRebarCurve(curve);
			curve.SetVertices(findIt->second);

			RebarEndType endType;
			endType.SetType(RebarEndType::kNone);
			RebarEndTypes   endTypes = { endType, endType };

			//设置点筋的颜色和图层
			BrString sizeKey = (rebarshape->GetShapeData().GetSizeKey());			
			RebarSymbology symb;
			string str(sizeKey);
			char ccolar[20] = { 0 };
			strcpy(ccolar, sizeKey);
			SetRebarColorBySize(ccolar, symb);
			symb.SetRebarLevel(TEXT_MAIN_REBAR);//画的是点筋则设置为主筋图层
			rep->SetRebarElementSymbology(symb);

			//设置新的rebarsize
			BrString rebarSize = BrString(m_rebarDia.c_str());
			GetDiameterAddType(rebarSize, m_rebarGrade);
			RebarShapeData shape = rebarshape->GetShapeData();	
			CString gradeStr;
			m_rebarGradeCombo.GetWindowTextW(gradeStr);
			shape.SetGrade(gradeStr);
			shape.SetSizeKey((LPCTSTR)rebarSize);
			double diameter = atof(m_rebarDia.c_str()) * uor_per_mm;
			rep->Update(curve, diameter, endTypes, shape, rep->GetModelRef(), false);

			//设置新的rebardata
			//TODO:没有生效
			EditElementHandle tmprebar(rep->GetElementId(), rep->GetModelRef());
			string level, rebarType, grade;
			GetRebarLevelItemTypeValue(tmprebar, level, rebarType, grade);
			SetRebarLevelItemTypeValue(tmprebar, level, m_rebarGrade, rebarType, it.second->GetDgnModelP());
			string reMark, markname;
			GetRebarCodeItemTypeValue(tmprebar, markname, reMark);
			if (markname!=""&&reMark!="")
			{
				SetRebarCodeItemTypeValue(tmprebar, reMark, markname, L"DgnLib", L"UserMark", ACTIVEMODEL);
			}
			tmprebar.ReplaceInModel(rep->GetElementRef());
			RebarModel *rmv = RMV;
			if (rmv != nullptr)
			{
				rmv->SaveRebar(*rep, rep->GetModelRef(), true);
			}
		}
	}
	ClearRefLine();
	CDialogEx::OnOK();
}

void ModifyRebarToolDlg::OnBnClickedCancel()
{
	ClearRefLine();
	CDialogEx::OnCancel();
}


void ModifyRebarToolDlg::OnEnChangeEditSpace()
{
	CString spaceStr;
	m_rebarsSpaceEdit.GetWindowTextW(spaceStr);
	m_spacing = _ttof(spaceStr);
	PreviewRefLines();
}


void ModifyRebarToolDlg::OnBnClickedButtonSelect()
{
	SelectRebarTool::InstallNewInstanceModifyDlg(CMDNAME_RebarSDKReadRebar, this);
}
