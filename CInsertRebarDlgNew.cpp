// CInsertRebarDlgNew.cpp: 实现文件

#include "_USTATION.h"
#include "CInsertRebarDlgNew.h"
#include "afxdialogex.h"
#include "resource.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "SelectRebarTool.h"
#include "ElementAttribute.h"
#include "PITRebarCurve.h"
#include "XmlHelper.h"
#include "ExtractFacesTool.h"
#include "ScanIntersectTool.h"
#include "SingleRebarAssembly.h"

extern GlobalParameters g_globalpara;
// CInsertRebarDlgNew 对话框

IMPLEMENT_DYNAMIC(CInsertRebarDlgNew, CDialogEx)

CInsertRebarDlgNew::CInsertRebarDlgNew(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_Insert, pParent)
{
	m_stWallInfo.embedLength = 400.0;
	m_stWallInfo.expandLength = 800.0;
	m_stWallInfo.rotateAngle = 0.00;
	m_stWallInfo.endType = 0;
	m_stWallInfo.wallTopType = 0;
	m_stWallInfo.staggeredStyle = 0;
	m_pInsertWallAssemblyNew = NULL;
	m_stWallInfo.postiveCover = 0.0;
	m_stWallInfo.HookDirection = 0;

	m_stWallInfo.NormalSpace = 0.0;
	m_stWallInfo.AverageSpace = 0.0;
	m_stWallInfo.isBack = false;
	m_stWallInfo.connectStyle = InsertRebarInfo::StaggerdJoint;
}


CInsertRebarDlgNew::~CInsertRebarDlgNew()
{

}


void CInsertRebarDlgNew::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

// CInsertRebarMain 消息处理程序
BOOL CInsertRebarDlgNew::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端

	if ((int)m_selectRebars.size() > 0)
	{
		// 过滤钢筋点，把钢筋终点朝下，同时取钢筋所在墙的ConcreteId、RebarAssembly和ElementHandle
		FilterReberPoint();
		if (!m_ehSel.IsValid())
		{
			return FALSE;
		}
		if (strcmp(m_RebarPts.at(0).sizeKey, "") != 0)
		{
			double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
			WString sizeKey = WString(m_RebarPts.at(0).sizeKey);
			//sizeKey = sizeKey.substr(0, 2);
			string Skey(StringOperator::Convert::WStringToString(sizeKey.GetWCharCP()));
			double curDiameter = RebarCode::GetBarDiameter(sizeKey, ACTIVEMODEL);
			m_stWallInfo.embedLength = g_globalpara.m_alength[Skey]; //g_globalpara.m_laplenth[atoi(m_RebarPts.at(0).sizeKey)];//搭接长度
		}
	}

	if ((int)m_selectRebars.size() == 0) // 双击进入的时候
	{
		GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), m_conid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
		GetElementXAttribute(m_conid, sizeof(InsertRebarInfo::WallInfo), m_stWallInfo, stInsertRebarInfoXAttribute, ACTIVEMODEL);
		GetElementXAttribute(m_conid, m_RebarPts, vecInsertPointAttrubute, ACTIVEMODEL);
		GetElementXAttribute(m_conid, sizeof(PIT::WallRebarInfo), m_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		
	}
	else
	{
		GetElementXAttribute(m_conid, sizeof(PIT::WallRebarInfo), m_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
	}
	for (auto var : g_listInsertType)
	{
		m_CombEndType.AddString(var);
	}
	m_CombEndType.SetCurSel(m_stWallInfo.endType);

	for (auto var : g_listRotateAngle)
	{
		m_CombEndNormal.AddString(var);
	}
	m_CombEndNormal.SetCurSel(m_stWallInfo.HookDirection);
	CString strTmp = CString();
	strTmp.Format(_T("%.2f"), m_stWallInfo.expandLength);
	m_EditExtended.SetWindowText(strTmp);

	strTmp.Format(_T("%.2f"), m_stWallInfo.embedLength);
	m_EditEmbed.SetWindowText(strTmp);

	strTmp.Format(_T("%.2f"), m_stWallInfo.rotateAngle);
	m_EditDirection.SetWindowText(strTmp);

	strTmp.Format(_T("%.f°"), m_stWallInfo.rotateAngle);
	auto find = std::find(g_listRotateAngle.begin(), g_listRotateAngle.end(), strTmp);
	int nIndex = (int)std::distance(g_listRotateAngle.begin(), find);
	m_CombEndNormal.SetCurSel(nIndex);

	if (m_stWallInfo.wallTopType == 1) // 上下墙类型
	{
		m_InsertType1.SetCheck(1); // 上墙变窄
	}
	else if (m_stWallInfo.wallTopType == 2)
	{
		m_InsertType2.SetCheck(1); // 上墙变宽
	}

	if (m_stWallInfo.isStaggered)
	{
		CButton* button = (CButton*)GetDlgItem(IDC_CHECK3);
		button->SetCheck(1);
	}
	else
	{
		CButton* button = (CButton*)GetDlgItem(IDC_CHECK3);
		button->SetCheck(0);
	}

	m_CombConnectStyle.InsertString(InsertRebarInfo::ConnectStyle::StaggerdJoint, _T("错开搭接"));
	m_CombConnectStyle.InsertString(InsertRebarInfo::ConnectStyle::MechanicalJoint, _T("机械连接"));
	m_CombConnectStyle.SetCurSel(InsertRebarInfo::ConnectStyle::StaggerdJoint);

	CVector3D vec;
	CInsertRebarAssemblySTWallNew::CalaWallNormalVec(m_ehSel, vec);
	CInsertRebarAssemblySTWallNew::SortVecRebar(m_RebarPts, vec);
	DrawRebarLine(ACTIVEMODEL);

	m_EditDirection.EnableWindow(FALSE);

	return true;
}

void CInsertRebarDlgNew::SetSlabParam(EditElementHandle& basis)
{
	double CurdiameterTie = 0.0;
	ElementId sourceid = basis.GetElementId();
	DgnModelRefP souremodel = basis.GetModelRef();
	ElementId uionID = ChangeSourceeeh2(basis);
	DgnModelRefP nowmodel = ACTIVEMODEL;
	if (uionID == 0)
	{
		uionID = basis.GetElementId();
		nowmodel = basis.GetModelRef();
	}
	EditElementHandle nowDB(uionID, nowmodel);
	ElementId baseid = 0;
	PIT::WallRebarInfo							m_slabRebarInfo; // 板相关信息
	double uor_per_mm = nowDB.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素
	GetElementXAttribute(nowDB.GetElementId(), sizeof(ElementId), baseid, ConcreteIDXAttribute, nowDB.GetModelRef());
	if (baseid==0)
	{
		GetElementXAttribute(sourceid, sizeof(ElementId), baseid, ConcreteIDXAttribute, souremodel);
		EditElementHandle Tmpeeh(sourceid, souremodel);
		nowDB.Duplicate(Tmpeeh);

	}
	if (baseid > 0)
	{
		std::vector<PIT::ConcreteRebar>	vecRebarDataTmp;
		GetElementXAttribute(baseid, vecRebarDataTmp, vecRebarDataXAttribute, ACTIVEMODEL);
		GetElementXAttribute(baseid, sizeof(PIT::WallRebarInfo), m_slabRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		GetElementXAttribute(baseid, sizeof(TieReBarInfo), m_tieRebarInfo, tieRebarInfoXAttribute, ACTIVEMODEL);
		GetDiameterAddType(m_tieRebarInfo.rebarSize, m_tieRebarInfo.rebarType);
		
		BrString strTieRebarSize(m_tieRebarInfo.rebarSize);
		if (strTieRebarSize != L"")
		{
			CurdiameterTie = RebarCode::GetBarDiameter(strTieRebarSize, ACTIVEMODEL) / UOR_PER_MilliMeter;	//拉筋直径
		}
		if (m_slabRebarInfo.concrete.postiveCover + CurdiameterTie - 0.00 > 0)
		{
			m_stWallInfo.postiveCover = m_slabRebarInfo.concrete.postiveCover;
		}
		
		double rebarDistance = 0.0;
		for (int j = 0; j < vecRebarDataTmp.size(); j++)
		{
			if (vecRebarDataTmp.at(j).datachange == 0) // 正面
			{
				BrString sizeKey = vecRebarDataTmp[j].rebarSize;
				GetDiameterAddType(sizeKey, vecRebarDataTmp[j].rebarType);
				double slabDiameter = RebarCode::GetBarDiameter(sizeKey, nowDB.GetModelRef()); // 钢筋直径
				rebarDistance += slabDiameter / uor_per_mm;        // 每层钢筋直径
				rebarDistance += vecRebarDataTmp.at(j).levelSpace; // 层间距
			}
		}
		m_stWallInfo.slabThickness = CInsertRebarAssemblySTWallNew::CalcSlabThickness(nowDB); // 板厚
		m_stWallInfo.slabDistance = m_stWallInfo.slabThickness - m_stWallInfo.postiveCover - CurdiameterTie - rebarDistance;
		CString strTmp = CString();
	}
	m_stWallInfo.slabThickness = CInsertRebarAssemblySTWallNew::CalcSlabThickness(nowDB); // 板厚
	if (m_stWallInfo.embedLength > m_stWallInfo.slabThickness)
	{
		m_stWallInfo.embedLength = m_stWallInfo.slabThickness;
	}
	// 过滤掉没有在墙底面附近的钢筋 （孔洞 -- 门洞规避）
	ISolidKernelEntityPtr entityPtr;
	if (SolidUtil::Convert::ElementToBody(entityPtr, nowDB) == SUCCESS) // 从可以表示单个线、片或实体的元素创建实体
	{
		DRange3d range;
		BentleyStatus nStatus = SolidUtil::GetEntityRange(range, *entityPtr); // 获取给定主体的轴对齐边界框
		if (SUCCESS == nStatus)
		{
			std::vector<InsertRebarPoint> RebarPts;
			RebarPts.insert(RebarPts.begin(), m_RebarPts.begin(), m_RebarPts.end());
			m_RebarPts.clear();
			for (InsertRebarPoint pt : RebarPts)
			{
				double distanceMin = pt.ptend.z - range.high.z;
				if (COMPARE_VALUES_EPS(distanceMin, (m_wallRebarInfo.concrete.reverseCover+ CurdiameterTie) * uor_per_mm, uor_per_mm) <= 0)
				{
					m_RebarPts.push_back(pt);
				}
			}
			for (auto it : m_selectRebars)
			{
				DPoint3d ptstr, ptend;
				double diameter = 0;
				GetStartEndPointFromRebar(&it, ptstr, ptend, diameter);
				double distanceMin = min(ptend.z - range.high.z, ptstr.z - range.high.z);
				if (COMPARE_VALUES_EPS(distanceMin, (m_wallRebarInfo.concrete.reverseCover + CurdiameterTie) * uor_per_mm, uor_per_mm) <= 0)
				{
					m_filterRebars.push_back(it);
				}
			}
		}
	}
	DrawRebarLine(ACTIVEMODEL);
}

void CInsertRebarDlgNew::CalaRebarPoint(std::vector<InsertRebarPoint>& RebarPts)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0; // 每单位像素
	double CurdiameterTie = 0.0;
	BrString strTieRebarSize(m_tieRebarInfo.rebarSize);
	if (strTieRebarSize != L"")
	{
		CurdiameterTie = RebarCode::GetBarDiameter(strTieRebarSize, ACTIVEMODEL) / UOR_PER_MilliMeter;	//拉筋直径
	}
	RebarPts.clear();
	int iIndex = 0;
	for (InsertRebarPoint pt : m_RebarPts)
	{
		double dDownPosition = pt.ptend.z - (m_wallRebarInfo.concrete.sideCover) * uor_per_mm; // 墙底部位置

		InsertRebarPoint stPoint;
		strcpy(stPoint.sizeKey, pt.sizeKey);
		strcpy(stPoint.SelectedRebarType, pt.SelectedRebarType);
		strcpy(stPoint.level, pt.level);
		strcpy(stPoint.grade, pt.grade);
		stPoint.ptstr = pt.ptend;
		stPoint.ptstr.z = dDownPosition;
		stPoint.Rsid = pt.Rsid;
		CButton* button = (CButton*)GetDlgItem(IDC_CHECK3);
		if (button->GetCheck() == 1 && (iIndex & 0x1)) // 交错配筋
		{
			stPoint.ptstr.z += m_stWallInfo.expandLength * uor_per_mm * 0.5;
		}
		else
		{
			stPoint.ptstr.z += m_stWallInfo.expandLength * uor_per_mm;
		}
		if (m_stWallInfo.endType == 1) // 90度弯钩
		{
			stPoint.isMid = true;
			BrString sizeKey = pt.sizeKey;
			sizeKey.Replace(L"mm", L"");
			double trueDiameter = atoi(sizeKey);
			double diameter = RebarCode::GetBarDiameter(sizeKey, ACTIVEMODEL); // 钢筋直径
			double bendRadius = RebarCode::GetPinRadius(sizeKey, ACTIVEMODEL, false);
			stPoint.ptmid = pt.ptend;
			stPoint.ptmid.z = dDownPosition;
			if (m_stWallInfo.slabDistance<1)//如果没有选板
			{
				m_stWallInfo.slabDistance = m_stWallInfo.embedLength + bendRadius + diameter;
			}
			stPoint.ptmid.z -= (m_stWallInfo.slabDistance * uor_per_mm - diameter * 0.5);

			CVector3D	endNormal;	//端部弯钩方向
			endNormal = CVector3D::kXaxis;
			endNormal.Normalize();
			endNormal.Rotate(m_stWallInfo.rotateAngle * PI / 180, CVector3D::kZaxis);	//以钢筋方向为轴旋转
			string Skey(sizeKey);
			if ((m_stWallInfo.slabDistance + bendRadius/uor_per_mm + diameter / (2*uor_per_mm)) < g_globalpara.m_alength[Skey])//90度弯钩要保证延申长度为12D，距离计算还有弯钩中的一个D
			{
				double endLenth = g_globalpara.m_alength[Skey] - m_stWallInfo.slabDistance;
				endNormal.ScaleToLength(endLenth* uor_per_mm);
			}
			else
			{
				double endLenth = bendRadius / uor_per_mm + 12* trueDiameter;
				endNormal.ScaleToLength(endLenth * uor_per_mm);
			}
			stPoint.ptend = stPoint.ptmid;
			stPoint.ptend.Add(endNormal);
		}
		else
		{
			BrString sizeKey = pt.sizeKey;
			string Skey(sizeKey);
			stPoint.ptend = pt.ptend;
			stPoint.ptend.z = dDownPosition;
			if (m_stWallInfo.embedLength< g_globalpara.m_alength[Skey])
			{
				m_stWallInfo.embedLength = g_globalpara.m_alength[Skey];
			}
			stPoint.ptend.z -= m_stWallInfo.embedLength * uor_per_mm;
		}
		RebarPts.push_back(stPoint);

		iIndex++;
	}
}

void CInsertRebarDlgNew::ClearLine()
{
	for (auto id : m_vecLineId)
	{
		EditElementHandle eeh(id, ACTIVEMODEL);
		eeh.DeleteFromModel();
	}
	m_vecLineId.clear();
}

void CInsertRebarDlgNew::DrawRebarLine(DgnModelRefP modelRef)
{
	std::vector<InsertRebarPoint> RebarPts;
	CalaRebarPoint(RebarPts);

	ClearLine();

	for (auto pt : RebarPts)
	{
		if (pt.isMid)
		{
			DPoint3d ptArr[3];
			ptArr[0] = pt.ptstr;
			ptArr[1] = pt.ptmid;
			ptArr[2] = pt.ptend;
			EditElementHandle eehLine;
			LineStringHandler::CreateLineStringElement(eehLine, nullptr, ptArr, 3, true, *modelRef);
			eehLine.AddToModel();
			m_vecLineId.push_back(eehLine.GetElementId());
		}
		else
		{
			EditElementHandle eehLine;
			LineHandler::CreateLineElement(eehLine, nullptr, DSegment3d::From(pt.ptstr, pt.ptend), true, *modelRef);
			eehLine.AddToModel();
			m_vecLineId.push_back(eehLine.GetElementId());
		}
	}
}

// 过滤钢筋点，把钢筋终点朝下，同时取钢筋所在墙的ConcreteId、RebarAssembly和ElementHandle
void CInsertRebarDlgNew::FilterReberPoint()
{
	if ((int)m_selectRebars.size() == 0)
	{
		return;
	}
	ElementHandle eehRebar = m_selectRebars[0];
	EditElementHandle Editeeh(eehRebar, ACTIVEMODEL);
	string rebarType;
	GetRebarLevelItemTypeValue(Editeeh, rebarType);
	if (rebarType == "back")
	{
		m_stWallInfo.isBack = true;
	}
	RebarElementP repTmp = RebarElement::Fetch(eehRebar);
	RebarModel *rmv = RMV;
	BeConcreteData condata;
	int rebar_cage_type;
	if (rmv != nullptr)
	{
		rmv->GetConcreteData(*repTmp, repTmp->GetModelRef(), condata, rebar_cage_type);
	}

	RebarSetP rebset = nullptr;
	rebset = repTmp->GetRebarSet(repTmp->GetModelRef());
	RebarShape * rebarshapeTmp = repTmp->GetRebarShape(m_selectRebars[0].GetModelRef());
	if (rebarshapeTmp==nullptr)
	{
		return;
	}
	BrString Sizekey(rebarshapeTmp->GetShapeData().GetSizeKey());
	m_stWallInfo.mainDiameter = RebarCode::GetBarDiameter(Sizekey, m_selectRebars[0].GetModelRef());
	int space = GetRebarHideData(m_selectRebars[0], m_selectRebars[0].GetModelRef());
	if (rebset != nullptr)
	{
		RebarSetP rootrebset = nullptr;
		rootrebset = rebset->GetParentRebarSet(repTmp->GetModelRef());
		rootrebset = rootrebset;
		RebarSets lapped_rebar_sets;
		rebset->GenerateLappedRebarSets(lapped_rebar_sets, repTmp->GetModelRef());
		if (space != 0)
		{
			m_stWallInfo.NormalSpace = space;
			m_stWallInfo.AverageSpace = space;
		}
		else
		{
			m_stWallInfo.NormalSpace = rebset->GetSetData().GetNominalSpacing();
			m_stWallInfo.AverageSpace = rebset->GetSetData().GetAverageSpacing();
		}
	}

	m_conid = condata.GetRexId().GetElementId();

	RebarAssemblies area;
	REA::GetRebarAssemblies(m_conid, area);

	RebarAssembly* rebarasb = nullptr;
	for (int i = 0; i < area.GetSize(); i++)
	{
		RebarAssembly* rebaras = area.GetAt(i);
		if (rebaras->GetCallerId() == rebset->GetCallerId())
		{
			rebarasb = rebaras;
		}
	}


	EditElementHandle ehSel;
	PIT::GetAssemblySelectElement(ehSel, rebarasb);
	m_ehSel = ehSel;
	for (int i = 0; i < m_selectRebars.GetCount(); i++)
	{
		if (RebarElement::IsRebarElement(m_selectRebars[i]))
		{
			RebarElementP rep = RebarElement::Fetch(m_selectRebars[i]);
			RebarSetP rebset = rep->GetRebarSet(ACTIVEMODEL);
			double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
			RebarShape * rebarshape = rep->GetRebarShape(m_selectRebars[i].GetModelRef());
			if (rebarshape == nullptr)
			{
				return ;
			}
			BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());

			InsertRebarPoint stInsertPoint;
			double diameter;
			DPoint3d ptstr, ptend;
			GetStartEndPointFromRebar(&m_selectRebars[i], ptstr, ptend, diameter);
			if (COMPARE_VALUES_EPS(ptstr.z, ptend.z, EPS) > 0)
			{
				stInsertPoint.ptstr = ptstr;
				stInsertPoint.ptend = ptend;
			}
			else
			{
				stInsertPoint.ptstr = ptend;
				stInsertPoint.ptend = ptstr;
			}

			strcpy(stInsertPoint.sizeKey, CT2A(Sizekey));
			string Level = "1";
			string Grade = "A";
			string Stype;
			GetRebarLevelItemTypeValue(m_selectRebars[i], Level, Stype, Grade);//获取选中钢筋的属性，写入插筋中			
			strcpy(stInsertPoint.SelectedRebarType, Stype.c_str());
			strcpy(stInsertPoint.level, Level.c_str());
			strcpy(stInsertPoint.grade, Grade.c_str());
			stInsertPoint.mid = m_selectRebars[i].GetElementId();
			stInsertPoint.Rsid = rebset->GetElementId();
			m_RebarPts.push_back(stInsertPoint);
		}
	}
}


void CInsertRebarDlgNew::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT18, m_EditExtended);					// 拓展长度
	DDX_Control(pDX, IDC_EDIT3, m_EditEmbed);						// 埋置长度
	DDX_Control(pDX, IDC_COMBO1, m_CombEndType);					// 端部样式
	DDX_Control(pDX, IDC_COMBO2, m_CombEndNormal);					// 弯钩方向

	DDX_Control(pDX, IDC_CHECK3, m_staggered_check);				// 交错配筋
	DDX_Control(pDX, IDC_EDIT_JC1, m_EditDirection);
	DDX_Control(pDX, IDC_COMBO_CONNECTSTYLE, m_CombConnectStyle);	//连接方式
	DDX_Control(pDX, IDC_INSERT_TZCDEDIT, m_EditExtended);
}


BEGIN_MESSAGE_MAP(CInsertRebarDlgNew, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CInsertRebarDlgNew::OnBnClickedButton1)
	ON_EN_KILLFOCUS(IDC_EDIT3, &CInsertRebarDlgNew::OnEnKillfocusEdit3)
	ON_EN_KILLFOCUS(IDC_EDIT3, &CInsertRebarDlgNew::OnEnKillfocusEdit18)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CInsertRebarDlgNew::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDOK, &CInsertRebarDlgNew::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CInsertRebarDlgNew::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CInsertRebarDlgNew::OnCbnSelchangeCombo2)
	ON_BN_CLICKED(IDC_CHECK3, &CInsertRebarDlgNew::OnBnClickedCheck3)
	ON_EN_KILLFOCUS(IDC_EDIT_JC1, &CInsertRebarDlgNew::OnEnKillfocusEditJc1)
	ON_EN_CHANGE(IDC_EDIT_JC1, &CInsertRebarDlgNew::OnEnChangeEditJc1)
	ON_CBN_SELCHANGE(IDC_COMBO_CONNECTSTYLE, &CInsertRebarDlgNew::OnCbnSelchangeComboConnectstyle)
	ON_EN_KILLFOCUS(IDC_INSERT_TZCDEDIT, &CInsertRebarDlgNew::OnEnKillfocusInsertTzcdedit)
END_MESSAGE_MAP()


// CInsertRebarDlgNew 消息处理程序
void CInsertRebarDlgNew::OnBnClickedButton1()
{

	DgnInsertRebarToolNew *pTool = new DgnInsertRebarToolNew(CMDNAME_InsertRebarTool, this);
	pTool->InstallTool();

	//SelectionSetManager::GetManager().EmptyAll();
	//// TODO: 在此添加控件通知处理程序代码
	//mdlInput_sendSynchronizedKeyin(L"element slection", 0, INPUTQ_EOQ, NULL);
}

// 埋置长度
void CInsertRebarDlgNew::OnEnKillfocusEdit3()
{
	CString strTemp = CString();
	m_EditEmbed.GetWindowText(strTemp);
	m_stWallInfo.embedLength = atof(CT2A(strTemp));

	DrawRebarLine(ACTIVEMODEL);
}

// 拓展长度
void CInsertRebarDlgNew::OnEnKillfocusEdit18()
{
	CString strTemp = CString();
	m_EditExtended.GetWindowText(strTemp);
	m_stWallInfo.expandLength = atof(CT2A(strTemp));

	DrawRebarLine(ACTIVEMODEL);
}


// 确定按钮
void CInsertRebarDlgNew::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	m_stWallInfo.endType = m_CombEndType.GetCurSel();

	DrawRebarLine(ACTIVEMODEL);
}

void CInsertRebarDlgNew::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
	int nType = m_CombEndNormal.GetCurSel();
	if (nType == 0)
	{
		m_stWallInfo.rotateAngle = 0.0;
		m_EditDirection.EnableWindow(FALSE);
	}
	else if (nType == 1)
	{
		m_stWallInfo.rotateAngle = 180.0;
		m_EditDirection.EnableWindow(FALSE);
	}
	else if (nType == 2)
	{
		m_EditDirection.EnableWindow(TRUE);
	}
	else
	{

	}
	DrawRebarLine(ACTIVEMODEL);
}



void CInsertRebarDlgNew::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());
	if (!eeh.IsValid())
	{
		return;
	}
	ClearLine();

	CInsertRebarAssemblySTWallNew::IsSmartSmartFeature(eeh);

	std::vector<InsertRebarPoint> RebarPts;
	CalaRebarPoint(RebarPts);

	DgnModelRefP modelRef = ACTIVEMODEL;
	if (m_pInsertWallAssemblyNew == NULL)
	{
		m_pInsertWallAssemblyNew = REA::Create<CInsertRebarAssemblySTWallNew>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	}

	CButton* button = (CButton*)GetDlgItem(IDC_CHECK3);
	if (button->GetCheck() == 0)
	{
		m_stWallInfo.isStaggered = false;
	}
	else
	{
		m_stWallInfo.isStaggered = true;
	}
	if (m_staggered_check.GetCheck()==true)
	{
		m_pInsertWallAssemblyNew->SetNormalSpace((int)((m_stWallInfo.NormalSpace+0.5)/10)*20);
		m_pInsertWallAssemblyNew->SetAverageSpace((int)((m_stWallInfo.AverageSpace + 0.5) / 10) * 20);
	}
	else
	{
		m_pInsertWallAssemblyNew->SetNormalSpace((int)((m_stWallInfo.NormalSpace + 0.5) / 10) * 10);
		m_pInsertWallAssemblyNew->SetAverageSpace((int)((m_stWallInfo.AverageSpace + 0.5) / 10) * 10);
	}
	
	m_pInsertWallAssemblyNew->SetisStaggered(m_stWallInfo.isStaggered);
	m_pInsertWallAssemblyNew->SetConcreteData(m_wallRebarInfo.concrete);
	m_pInsertWallAssemblyNew->AnalyzingWallGeometricData(eeh);
	m_pInsertWallAssemblyNew->SetstWallInfo(m_stWallInfo);
	m_pInsertWallAssemblyNew->SetvecRebarPts(RebarPts);
	m_pInsertWallAssemblyNew->MakeRebars(modelRef);
	m_pInsertWallAssemblyNew->Save(modelRef); // must save after creating rebars
	ElementId contid = m_pInsertWallAssemblyNew->FetchConcrete();
	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);

	SetElementXAttribute(contid, sizeof(InsertRebarInfo::WallInfo), &m_stWallInfo, stInsertRebarInfoXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_RebarPts, vecInsertPointAttrubute, ACTIVEMODEL);
	// SetElementXAttribute(contid, sizeof(PIT::WallRebarInfo), &m_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);

	SetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, eeh.GetModelRef());

	//修改点筋
	if (InsertRebarInfo::ConnectStyle::MechanicalJoint == m_stWallInfo.connectStyle)
	{
		UpdateSelectRebars(RebarPts, modelRef);
	}

	SelectionSetManager::GetManager().EmptyAll();
	DestroyWindow();
}


void CInsertRebarDlgNew::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	ClearLine();
	CDialogEx::OnCancel();
	DestroyWindow();
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
void DgnInsertRebarToolNew::SetupForLocate()
{
	UInt32      msgId;
	msgId = PROMPT_ACCEPTREJECT;
	bool doLocate = true;

	switch (GetElementAgenda().GetCount())
	{
	case 0:
		msgId = PROMPT_SELECT_BASIS;
		break;

	case 1:
		msgId = PROMPT_SELECT_EXPAND;
		break;

	default:
		msgId = PROMPT_ACCEPTREJECT;
		doLocate = false;
		break;
	}

	__super::_SetLocateCursor(doLocate);

	mdlAccuSnap_enableSnap(false);
	mdlAccuDraw_setEnabledState(false);

	mdlOutput_rscPrintf(MSG_PROMPT, NULL, STRINGLISTID_RebarSDKExampleTextMessages, msgId); // STRINGLISTID_ShaftToolTextMessages
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
bool DgnInsertRebarToolNew::_WantAdditionalLocate(DgnButtonEventCP ev)
{
	if (NULL == ev)
		return true; // This is a multi-locate tool...

	return GetElementAgenda().GetCount() != 1;
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
void DgnInsertRebarToolNew::_SetupAndPromptForNextAction()
{
	SetupForLocate();
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
bool DgnInsertRebarToolNew::_SetupForModify(DgnButtonEventCR ev, bool isDynamics)
{
	if (GetElementAgenda().GetCount() != 1)
	{
		return false;
	}

	return true;
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
EditElementHandleP DgnInsertRebarToolNew::_BuildLocateAgenda(HitPathCP path, DgnButtonEventCP ev)
{
	EditElementHandle eeh(path->GetHeadElem(), path->GetRoot());
	return GetElementAgenda().Insert(eeh);
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
bool DgnInsertRebarToolNew::_OnDataButton(DgnButtonEventCR ev)
{
	HitPathCP   path = _DoLocate(ev, true, ComponentMode::Innermost);
	if (path == NULL)
	{
		return false;
	}

	//获取元素句柄
	ElementHandle theElementHandle(mdlDisplayPath_getElem((DisplayPathP)path, 0), mdlDisplayPath_getPathRoot((DisplayPathP)path));
	EditElementHandle eehEdit(theElementHandle, false);
	m_pInsertRebarDlgNew->SetSlabParam(eehEdit);

	//终止动态，退出工具
	_EndDynamics();
	_ExitTool();
	return true;
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
bool DgnInsertRebarToolNew::_OnPostLocate(HitPathCP path, WStringR cantAcceptReason)
{
	if (!DgnElementSetTool::_OnPostLocate(path, cantAcceptReason))
		return false;

	ElementHandle eh(path->GetHeadElem(), path->GetRoot());

	return !(RebarElement::IsRebarElement(eh) || IsRebarDetailSet(eh));
	// return ShaftBodyAssembly::IsShaftBody(eh) || ShaftCapAssembly::IsShaftCap(eh);
}


/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
StatusInt DgnInsertRebarToolNew::_ProcessAgenda(DgnButtonEventCR  ev)
{
	EditElementHandleP start = GetElementAgenda().GetFirstP();
	EditElementHandleP last = start + GetElementAgenda().GetCount();

	return SUCCESS;
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
bool DgnInsertRebarToolNew::_OnInstall()
{
	_SetElemSource(SOURCE_Pick);
	return true;
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
void DgnInsertRebarToolNew::_OnRestartTool()
{
	InstallNewInstance(GetToolId(), m_pInsertRebarDlgNew);
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
void DgnInsertRebarToolNew::InstallNewInstance(int toolId, CInsertRebarDlgNew* pInsertRebarDlgNew)
{
	DgnInsertRebarToolNew* tool = new DgnInsertRebarToolNew(toolId, pInsertRebarDlgNew);
	mdlOutput_messageCenter(OutputMessagePriority::TempRight, L"Please pick two elements, the first is the Basis, the sceond is Wall Or Column", NULL, OutputMessageAlert::None);
	tool->InstallTool();
}

/*---------------------------------------------------------------------------------**//**
* @param[in]        unparsed        In this case unparsed is ignored.
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
Public void ShaftTool_placeRebar(WCharCP unparsed, CInsertRebarDlgNew* pInsertRebarDlgNew)
{
	DgnInsertRebarToolNew::InstallNewInstance(CMDNAME_InsertRebarTool, pInsertRebarDlgNew);
}


void CInsertRebarDlgNew::OnBnClickedCheck3()
{
	// TODO: 在此添加控件通知处理程序代码
	DrawRebarLine(ACTIVEMODEL);
}


void CInsertRebarDlgNew::OnEnKillfocusEditJc1()
{
	CString str = CString();
	m_EditDirection.GetWindowText(str);
	m_stWallInfo.rotateAngle = atof(CT2A(str));
	DrawRebarLine(ACTIVEMODEL);
}


void CInsertRebarDlgNew::OnEnChangeEditJc1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}


void CInsertRebarDlgNew::OnCbnSelchangeComboConnectstyle()
{
	m_stWallInfo.connectStyle = InsertRebarInfo::ConnectStyle(m_CombConnectStyle.GetCurSel());

}

void CInsertRebarDlgNew::UpdateSelectRebars(const vector<InsertRebarPoint>& rebarPts, DgnModelRefP modelRef)
{
#ifdef OldFile
	for (size_t i = 0; i < m_filterRebars.size(); ++i)
	{
		if (RebarElement::IsRebarElement(m_filterRebars[i]))
		{
			RebarElementP rep = RebarElement::Fetch(m_filterRebars[i]);
			double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
			RebarShape * rebarshape = rep->GetRebarShape(m_filterRebars[i].GetModelRef());
			if (rebarshape == nullptr)
			{
				return;
			}
			BrString sizeKey(rebarshape->GetShapeData().GetSizeKey());
			double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);

			//点筋的端点
			bvector<DPoint3d> allpts;
			DPoint3d ptStart = m_RebarPts.at(i).ptstr;
			DPoint3d ptEnd = rebarPts.at(i).ptstr;
			allpts.push_back(ptStart);
			allpts.push_back(ptEnd);
			RebarVertices  vers;
			GetRebarVerticesFromPoints(vers, allpts, diameter);

			//获取钢筋模板中的线条形状
			RebarCurve curve;
			rebarshape->GetRebarCurve(curve);
			curve.SetVertices(vers);

			RebarEndType endType;
			endType.SetType(RebarEndType::kNone);
			RebarEndTypes   endTypes = { endType, endType };

			//设置点筋的颜色和图层
			RebarSymbology symb;
			string str(sizeKey);
			char ccolar[20] = { 0 };
			strcpy(ccolar, str.c_str());
			SetRebarColorBySize(ccolar, symb);
			symb.SetRebarLevel(TEXT_MAIN_REBAR);//画的是点筋则设置为主筋图层
			rep->SetRebarElementSymbology(symb);

			RebarShapeData shape = rebarshape->GetShapeData();
			shape.SetSizeKey(CString(sizeKey.Get()));
			shape.SetIsStirrup(false);
			shape.SetLength(curve.GetLength() / uor_per_mm);
			shape.SetNominalLength(curve.GetLength() / uor_per_mm);
			shape.SetWeight(0);
			rep->Update(curve, diameter, endTypes, shape, rep->GetModelRef(), false);
			RebarModel *rmv = RMV;
			if (rmv != nullptr)
			{
				rmv->SaveRebar(*rep, rep->GetModelRef(), true);
				rmv->LoadRex(*rep, ACTIVEMODEL);
				rmv->SaveCatalog(ACTIVEMODEL);
			}
		}
	}
#else
	if (m_selectRebars.size()>0&&m_stWallInfo.slabThickness<10)
	{
		for (auto it : m_selectRebars)
		{
			m_filterRebars.push_back(it);
		}
	}
	//按钢筋rebarset对钢筋进行分组，然后分组更新钢筋
	map<ElementId, vector<IDandModelref>> rs_elemets;
	for (int i = 0; i < m_filterRebars.size(); i++)
	{
		EditElementHandle elementToModify(m_filterRebars[i], m_filterRebars[i].GetDgnModelP());
		if (RebarElement::IsRebarElement(elementToModify))
		{
			RebarElementP rep = RebarElement::Fetch(m_filterRebars[i]);
			RebarSet* rebset = rep->GetRebarSet(ACTIVEMODEL);
			if (rebset!=nullptr)
			{
				IDandModelref tmp;
				tmp.ID = elementToModify.GetElementId();
				tmp.tModel = elementToModify.GetModelRef();
				rs_elemets[rebset->GetElementId()].push_back(tmp);
			}
		}
	}
	map<ElementId, vector<IDandModelref>>::iterator itr = rs_elemets.begin();
	for (;itr!=rs_elemets.end();itr++)
	{
		vector<RebarVertices> mrebarPts;
		vector<BrString> vecDir;
		for (int i = 0; i < itr->second.size(); i++)
		{
			EditElementHandle elementToModify(itr->second[i].ID, itr->second[i].tModel);
			if (RebarElement::IsRebarElement(elementToModify))
			{
				RebarElementP rep = RebarElement::Fetch(elementToModify);
				double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
				RebarShape * rebarshape = rep->GetRebarShape(itr->second[i].tModel);
				if (rebarshape == nullptr)
				{
					return;
				}
				BrString sizeKey(rebarshape->GetShapeData().GetSizeKey());
				double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
				RebarCurve curve;

				DPoint3d ptStart;
				DPoint3d ptEnd;
				for (int j = 0; j < m_RebarPts.size(); j++)
				{
					if (m_RebarPts.at(j).mid == elementToModify.GetElementId())
					{
						ptStart = m_RebarPts.at(j).ptstr;
						ptEnd = rebarPts.at(j).ptstr;
						break;
					}
				}
				rebarshape->GetRebarCurve(curve);
				RebarVertices  vers;
				CMatrix3D tmp3d(rep->GetLocation());
				curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
				curve.DoMatrix(rep->GetLocation());
				vers = curve.GetVertices();
				if (vers.GetSize() > 1)
				{
					if (vers.GetAt(0)->GetIP().Distance(ptEnd) < vers.GetAt(vers.GetSize() - 1)->GetIP().Distance(ptEnd))
					{
						DPoint3d tmppt = vers.GetAt(0)->GetIP();
						tmppt.z = ptEnd.z;
						vers.GetAt(0)->SetIP(tmppt);
						vers.GetAt(0)->SetType(RebarVertex::PointType::kStart);
					}
					else
					{
						DPoint3d tmppt = vers.GetAt(vers.GetSize() - 1)->GetIP();
						tmppt.z = ptEnd.z;
						vers.GetAt(vers.GetSize() - 1)->SetIP(tmppt);
						vers.GetAt(vers.GetSize() - 1)->SetType(RebarVertex::PointType::kStart);
					}
				}
				//GetRebarVerticesFromPoints(vers, allpts, diameter);
				mrebarPts.push_back(vers);
				vecDir.push_back(sizeKey);
			}
		}
		RebarSet * rebset = nullptr;
		if (itr->second.size() < 1)
		{
			continue;
		}
		EditElementHandle start(itr->second[0].ID, itr->second[0].tModel);
		/*获取钢筋的所有属性值str*/
		string Level = "1";
		string Grade = "A";
		string type = "";
		string markname = "";
		string ReplacedMark = "";
		GetRebarCodeItemTypeValue(start, markname, ReplacedMark);
		GetRebarLevelItemTypeValue(start, Level, type, Grade);//获取选中钢筋的属性，写入U形筋中		
		int Spacing = GetRebarHideData(start, ACTIVEMODEL);
		/*获取钢筋的所有属性值end*/
		mdlSelect_freeAll();
		if (RebarElement::IsRebarElement(start))
		{
			RebarElementP rep = RebarElement::Fetch(start);
			rebset = rep->GetRebarSet(ACTIVEMODEL);
			if (rebset != nullptr&&itr->second.size() > 0 && itr->second.size() == vecDir.size())
			{

				RebarModel *rmv = RMV;
				BeConcreteData condata;
				int rebar_cage_type;
				if (rmv != nullptr)
				{
					rmv->GetConcreteData(*rep, rep->GetModelRef(), condata, rebar_cage_type);
				}

				ElementId conid = 0;
				conid = condata.GetRexId().GetElementId();
				if (conid == 0)
				{
					conid = rebset->GetConcreteOwner(ACTIVEMODEL, rebar_cage_type);
					if (conid == 0)
					{
						return;
					}
				}
				//int rebar_cage_type;
				//conid = rebset->GetConcreteOwner(ACTIVEMODEL, rebar_cage_type);
				RebarAssemblies reas;
				RebarAssembly::GetRebarAssemblies(conid, reas);

				RebarAssembly* rebarasb = nullptr;
				for (int i = 0; i < reas.GetSize(); i++)
				{
					RebarAssembly* rebaras = reas.GetAt(i);
					if (rebaras->GetCallerId() == rebset->GetCallerId())
					{
						rebarasb = rebaras;
					}

				}
				if (rebarasb == nullptr&&reas.GetSize() > 0)
				{
					rebarasb = reas.GetAt(0);
				}
				if (rebarasb != nullptr)
				{
					DgnModelRefP        modelRef = ACTIVEMODEL;
					SingleRebarAssembly*  slabRebar = REA::Create<SingleRebarAssembly>(modelRef);

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

					slabRebar->SetSelectedRebar(start);
					slabRebar->SetSlabData(ehSel);
					slabRebar->SetvecDirSize(vecDir);
					slabRebar->SetrebarPts(mrebarPts);
					slabRebar->SetEcDatas(type, Level, Grade, markname, ReplacedMark);
					slabRebar->Setspacing(Spacing);
					slabRebar->MakeRebars(modelRef);
					slabRebar->Save(modelRef); // must save after creating rebars			
					ElementId contid = slabRebar->FetchConcrete();
					EditElementHandle eeh2(contid, ACTIVEMODEL);
					ElementRefP oldRef = eeh2.GetElementRef();
					mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
					eeh2.ReplaceInModel(oldRef);
				}

				{//删除钢筋处理
					for (IDandModelref tref : itr->second)
					{
						EditElementHandle eeh(tref.ID, tref.tModel);
						SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), ACTIVEMODEL);
					}
				}

			}
		}
	}
	CDialogEx::OnOK();
	mdlInput_sendSynchronizedKeyin(L"proconcrete delete rebar", 0, INPUTQ_EOQ, NULL);
	mdlInput_sendSynchronizedKeyin(L"choose element", 0, INPUTQ_EOQ, NULL);
#endif

}


void CInsertRebarDlgNew::OnEnKillfocusInsertTzcdedit()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strTemp = CString();
	m_EditExtended.GetWindowText(strTemp);
	m_stWallInfo.expandLength = atof(CT2A(strTemp));

	DrawRebarLine(ACTIVEMODEL);
}
