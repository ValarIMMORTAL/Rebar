// CWallMainRebar.cpp: 实现文件
//

#include "_USTATION.h"
#include "CUniteMainRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include "CWallRebarDlg.h"
#include "CommonFile.h"
#include "AssociatedComponent.h"
#include "ElementAttribute.h"
#include "PITRebarCurve.h"

#include "CSlabRebarDlg.h"
#include "ACCRebarMaker.h"
#include "CSlabRebarDlg.h"
#include "MySlabRebarAssembly.h"
#include "CBreakEllipseWallDlg.h"
#include "WallHelper.h"
#define WM_UPDATELIST  WM_USER+1   // do something  
#define WM_LEVELCHANGE  WM_USER+2   // do something  

// 主要配筋界面指针的全局变量，在OnInitDialog时初始化
CUniteMainRebarDlg * g_wallMainDlg = nullptr;

// CWallMainRebar 对话框

IMPLEMENT_DYNAMIC(CUniteMainRebarDlg, CDialogEx)

CUniteMainRebarDlg::CUniteMainRebarDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_Combine_Setting_MainRebar, pParent), m_assodlg(NULL)
{
	pm_MainPageRebar = nullptr;

	mArcLine.radius = 0.0;
	mArcLine.ArcLen = 0.0;
	mArcLine.ptStart = DPoint3d::FromZero();
	mArcLine.ptEnd = DPoint3d::FromZero();
	mArcLine.centerpt = DPoint3d::FromZero();

}

CUniteMainRebarDlg::~CUniteMainRebarDlg()
{
	if (m_assodlg != nullptr)
	{
		delete m_assodlg;
		m_assodlg = nullptr;
	}
	g_wallMainDlg = nullptr;
	free(g_wallMainDlg);
}


void CUniteMainRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_listMainRebar);
	DDX_Control(pDX, IDC_EDIT1, m_EditPositive);
	DDX_Control(pDX, IDC_EDIT2, m_EditSide);
	DDX_Control(pDX, IDC_EDIT3, m_EditReverse);
	DDX_Control(pDX, IDC_EDIT4, m_EditLevel);
	DDX_Control(pDX, IDC_HOLE_CHECK, m_hole_check);
	DDX_Control(pDX, IDC_MHOLESIZE_EDIT, m_mholesize_edit);
	DDX_Control(pDX, IDC_COMBO1, m_ComRebarMethod);
}


BEGIN_MESSAGE_MAP(CUniteMainRebarDlg, CDialogEx)
	ON_EN_KILLFOCUS(IDC_EDIT1, &CUniteMainRebarDlg::OnEnKillfocusEdit1)
	ON_EN_KILLFOCUS(IDC_EDIT2, &CUniteMainRebarDlg::OnEnKillfocusEdit2)
	ON_EN_KILLFOCUS(IDC_EDIT3, &CUniteMainRebarDlg::OnEnKillfocusEdit3)
	ON_EN_KILLFOCUS(IDC_EDIT4, &CUniteMainRebarDlg::OnEnKillfocusEdit4)
	ON_MESSAGE(WM_UPDATELIST, &CUniteMainRebarDlg::OnComboBoxDataChange)
	ON_BN_CLICKED(IDC_BUTTON1, &CUniteMainRebarDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_HOLE_CHECK, &CUniteMainRebarDlg::OnBnClickedHoleCheck)
	ON_EN_KILLFOCUS(IDC_MHOLESIZE_EDIT, &CUniteMainRebarDlg::OnEnKillfocusMholesizeEdit)
	ON_BN_CLICKED(IDC_BUTTON2, &CUniteMainRebarDlg::OnBnClickedButton2)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CUniteMainRebarDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON_BreakEllipseWall, &CUniteMainRebarDlg::OnBnClickedButtonBreakellipsewall)
	//ON_BN_CLICKED(IDC_SaveData, &CUniteMainRebarDlg::OnBnClickedSavedata)
	ON_BN_CLICKED(IDC_RebarTemplate, &CUniteMainRebarDlg::OnBnClickedRebartemplate)
	ON_BN_CLICKED(IDC_UpdataDlg, &CUniteMainRebarDlg::OnBnClickedUpdatadlg)
END_MESSAGE_MAP()


// CWallMainRebar 消息处理程序

void CUniteMainRebarDlg::InitUIData()
{
	CString strLevel, strPositive, strSide, strReverse, strMissHoleSize;
	g_wallRebarInfo.concrete.rebarLevelNum = 2;
	strLevel.Format(_T("%d"), g_wallRebarInfo.concrete.rebarLevelNum);
	strPositive.Format(_T("%.2f"), g_wallRebarInfo.concrete.postiveCover);
	strReverse.Format(_T("%.2f"), g_wallRebarInfo.concrete.reverseCover);
	strSide.Format(_T("%.2f"), g_wallRebarInfo.concrete.sideCover);
	strMissHoleSize.Format(_T("%.2f"), g_wallRebarInfo.concrete.MissHoleSize);

	for each (auto var in g_listSlabRebarMethod)
		m_ComRebarMethod.AddString(var);

	if (g_wallRebarInfo.concrete.isHandleHole)
	{
		m_hole_check.SetCheck(true);
		m_mholesize_edit.SetReadOnly(FALSE);
	}
	else
	{
		m_hole_check.SetCheck(false);
		m_mholesize_edit.SetReadOnly(TRUE);
	}
	m_EditLevel.SetWindowText(strLevel);
	m_EditPositive.SetWindowText(strPositive);
	m_EditReverse.SetWindowText(strReverse);
	m_EditSide.SetWindowText(strSide);
	m_mholesize_edit.SetWindowText(strMissHoleSize);

	m_ComRebarMethod.SetCurSel(g_wallRebarInfo.concrete.m_SlabRebarMethod);//配筋方式


	LONG lStyle;
	lStyle = GetWindowLong(m_listMainRebar.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_listMainRebar.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_listMainRebar.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_listMainRebar.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_listMainRebar.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_listMainRebar.InsertColumn(0, _T("层"), (int)(width / 11.0*0.75), ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	//m_listMainRebar.InsertColumn(1, _T("方向"), (int)(0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(1, _T("直径"), (int)(width / 11.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(2, _T("类型"), (int)(width / 11.0*1.25), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	/*if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2)
	{
		m_listMainRebar.InsertColumn(3, _T("环向间距/径向角度"), (int)(width / 11.0 * 2), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	}
	else*/
	m_listMainRebar.InsertColumn(3, _T("间距"), (int)(width / 11.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(4, _T("起点偏移"), (int)(width / 11.0*1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(5, _T("终点偏移"), (int)(width / 11.0*1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(6, _T("与前层间距"), (int)(width / 11.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(7, _T("位置"), (int)(width / 11.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	//m_listMainRebar.InsertColumn(9, _T("颜色"), (int)(width / 11*0.95), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(8, _T("线形"), (int)(width / 11 ), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(9, _T("线宽"), (int)(width / 11 ), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	//	m_listMainRebar.SetShowProgressPercent(TRUE);
	//	m_listMainRebar.SetSupportSort(TRUE);

	//	SetListRowData(g_vecRebarData);

	UpdateRebarList();
}


//void CUniteMainRebarDlg::SetListDefaultData()
//{
//	if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
//	{
//		for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
//		{
//			PIT::ConcreteRebar oneRebarData;
//			if (0 == i)
//				oneRebarData = { i,0,"12",0,200,0,0,0 };
//			else if (1 == i)
//				oneRebarData = { i,1,"12",0,200,0,0,0 };
//			else
//			{
//				int dir = (i + 1) & 0x01;
//				double levelSpace;
//				levelSpace = ((i + 1) & 0x01) * 2000.0;
//				oneRebarData = { i,dir,"12",0,200,0,0,levelSpace };
//			}
//			m_vecRebarData.push_back(oneRebarData);
//		}
//	}
//	else //往后删除数据或添加默认数据
//	{
//		int iOffset = g_wallRebarInfo.concrete.rebarLevelNum - (int)m_vecRebarData.size();
//		if (iOffset > 0)
//		{
//			for (int i = 0; i < iOffset; i++)
//			{
//				int dir = (i + 1) & 0x01;
//				double levelSpace;
//				levelSpace = dir * 2000.0;
//				PIT::ConcreteRebar oneRebarData = { i,dir,"12",0,200,0,0,levelSpace };
//				m_vecRebarData.push_back(oneRebarData);
//			}
//		}
//		if (iOffset < 0)
//		{
//			iOffset *= -1;
//			for (int i = 0; i < iOffset; i++)
//			{
//				m_vecRebarData.pop_back();
//			}
//		}
//	}
//	g_vecRebarData = m_vecRebarData;
//}
void CUniteMainRebarDlg::SetListDefaultData()
{
	if (g_wallRebarInfo.concrete.rebarLevelNum - (int)m_vecRebarData.size() != 0)
	{
		m_vecRebarData.clear();//添加了墙后位置后，添加此代码
	}
	if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
	{
		if (g_wallRebarInfo.concrete.rebarLevelNum > 0)
		{
			//int midpos = g_wallRebarInfo.concrete.rebarLevelNum / 2;
			for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
			{
				PIT::ConcreteRebar oneRebarData;
				//if (i < midpos)//前半部分
				{
					int dir = (i) & 0x01;
					double levelSpace;
					levelSpace = 0;
					oneRebarData = { i + 1,dir,"12",2,150,0,0,levelSpace };
					oneRebarData.datachange = i;
					if (oneRebarData.rebarDir == 0)
					{
						strcpy(oneRebarData.rebarSize, "16");
					}
					else
					{
						strcpy(oneRebarData.rebarSize, "20");
					}
				}
				//else//后半部分
				//{
				//	int dir = (i + 1) & 0x01;
				//	double levelSpace;
				//	levelSpace = 0;
				//	oneRebarData = { i,dir,"12",2,150,0,0,levelSpace };
				//	oneRebarData.datachange = 0;
				//	oneRebarData.rebarLevel = g_wallRebarInfo.concrete.rebarLevelNum - i;
				//	oneRebarData.datachange = 2;
				//}

				m_vecRebarData.push_back(oneRebarData);
			}
		}

	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = g_wallRebarInfo.concrete.rebarLevelNum - (int)m_vecRebarData.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				int dir = (i + 1) & 0x01;
				double levelSpace;
				levelSpace = dir * 2000.0;
				PIT::ConcreteRebar oneRebarData = { i,dir,"12",2,200,0,0,levelSpace };
				if (oneRebarData.rebarDir == 0)
				{
					strcpy(oneRebarData.rebarSize, "16");
				}
				else
				{
					strcpy(oneRebarData.rebarSize, "20");
				}
				m_vecRebarData.push_back(oneRebarData);
			}
		}
		if (iOffset < 0)
		{
			iOffset *= -1;
			for (int i = 0; i < iOffset; i++)
			{
				m_vecRebarData.pop_back();
			}
		}
	}
	//PIT::SetLevelidByRebarData(m_vecRebarData);
	g_vecRebarData = m_vecRebarData;
}


void CUniteMainRebarDlg::getWallSetInfo(WallSetInfo& wallsetinfo)
{
	strcpy(m_WallsetInfo.rebarSize, wallsetinfo.rebarSize);
	m_WallsetInfo.rebarType = wallsetinfo.rebarType;
	m_WallsetInfo.spacing = wallsetinfo.spacing;
}

void CUniteMainRebarDlg::Set_m_DlgData()
{
	CString strpostive;
	GetDlgItemText(IDC_EDIT1, strpostive);
	CString strside;
	GetDlgItemText(IDC_EDIT2, strside);
	CString strreverse;
	GetDlgItemText(IDC_EDIT3, strreverse);
	CString strholesize;
	GetDlgItemText(IDC_MHOLESIZE_EDIT, strholesize);
	CString strlevelnum;
	GetDlgItemText(IDC_EDIT4, strlevelnum);

	m_dlgData.postiveCover = _ttof(strpostive);
	m_dlgData.sideCover = _ttof(strside);
	m_dlgData.reverseCover = _ttof(strreverse);
	m_dlgData.missHoleSize = _ttof(strholesize);
	m_dlgData.rebarLevelNum = _ttoi(strlevelnum);

}

void CUniteMainRebarDlg::GetAllRebarData(std::vector<PIT::ConcreteRebar> &vecListData)
{
	m_listMainRebar.GetUniteRebarData(vecListData);
	for (auto& it : vecListData)
	{
		if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2 && it.rebarDir == 1) //放射径向
		{
			it.angle = it.spacing;
			it.spacing = mArcLine.radius / 10 * PI / 180 * it.angle;
		}
	}
	std::vector<PIT::ConcreteRebar> tmep = vecListData;
	int allsize = vecListData.size();
	for(int i= allsize-1;i>=0;i--)
	{
		PIT::ConcreteRebar temp = vecListData[i];
		vecListData[i].datachange = 0;
		temp.datachange = 2;
		vecListData.push_back(temp);
	}
}

void CUniteMainRebarDlg::GetAllWallRebarData(std::vector<PIT::ConcreteRebar> &vecListData)
{
	m_listMainRebar.GetAllRebarData(vecListData);
	DVec3d arcToCenterVec = mArcLine.ptStart - mArcLine.centerpt;
	arcToCenterVec.Normalize();
	for (auto& it : vecListData)
	{
		if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2 && it.rebarDir == 1) //放射径向
		{
			it.angle = it.spacing;
		}
		else
		{
			it.angle = 0;
		}
	}
}

bool  CUniteMainRebarDlg::ParsingLineDir(EditElementHandleR ehLine)
{
	DPoint3d ptStr, ptEnd;
	if (SUCCESS == mdlElmdscr_extractEndPoints(&ptStr, nullptr, &ptEnd, nullptr, ehLine.GetElementDescrP(), ehLine.GetModelRef()))
	{
		m_seg = DSegment3d::From(ptStr, ptEnd);
		return  true;
	}
	else
		return false;

}


bool  CUniteMainRebarDlg::ParsingArcLineDir(EditElementHandleR ehLine)
{
	MSElementDescrP ArcMse = ehLine.GetElementDescrP();
	if (ArcMse == nullptr)
		return false;

	double starR, sweepR = 0.00;
	double radius = 0.00;
	DPoint3d ArcDPs[2] = { 0 };
	RotMatrix rotM;
	DPoint3d centerpt = { 0 };
	mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &ArcMse->el);
	double ArcLen;
	mdlElmdscr_distanceAtPoint(&ArcLen, nullptr, nullptr, ArcMse, &ArcDPs[1], 0.1);
	double dis2 = ArcLen / 2;
	DPoint3d midPt = { 0,0,0 };
	mdlElmdscr_pointAtDistance(&midPt, nullptr, dis2, ArcMse, 0.1);

	//将弧线的Z轴坐标移动到与板件底面Z坐标一致
	ArcDPs[0].z = m_height;
	mArcLine.ptStart = ArcDPs[0];
	ArcDPs[1].z = m_height;
	mArcLine.ptEnd = ArcDPs[1];
	centerpt.z = m_height;
	mArcLine.centerpt = centerpt;
	mArcLine.radius = radius;
	midPt.z = m_height;
	mArcLine.ptMid = midPt;
	mArcLine.isCircle = false;

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (ArcDPs[0].Distance(ArcDPs[1]) < uor_per_mm)
	{
		MSElement newDescr;
		mdlArc_create(&newDescr, NULL, &centerpt, radius, radius, nullptr, 0, 2 * PI - 0.05);
		DPoint3d newArcDPs[2];
		RotMatrix rotM;
		mdlArc_extract(newArcDPs, nullptr, nullptr, &radius, NULL, &rotM, &centerpt, &newDescr);
		MSElementDescrP ptmp = MSElementDescr::Allocate(newDescr);
		//mdlElmdscr_add(ptmp);
		mArcLine.ptStart = newArcDPs[0];
		mArcLine.ptEnd = newArcDPs[1];
		//mdlElmdscr_distanceAtPoint(&mArcLine.ArcLen, nullptr, nullptr, ptmp, &newArcDPs[1], 0.1);
		mArcLine.isCircle = true;
	}
	else
	{
		mArcLine.ArcLen = ArcLen;
	}
	return true;
}




void CUniteMainRebarDlg::UpdateRebarList()
{
	m_listMainRebar.DeleteAllItems();
	SetListDefaultData();
	for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
	{
		CString strValue;
		strValue.Format(_T("%dL"), m_vecRebarData[i].rebarLevel);
		m_listMainRebar.InsertItem(i, _T("")); // 插入行
		m_listMainRebar.SetItemText(i, 0, strValue);
		int nSubCnt = m_listMainRebar.GetColumnCount() - 1;
		for (int j = 1; j <= nSubCnt; ++j)
		{
			CString strValue;
			switch (j)
			{
			//case 1:
			//{
			//	//ListCtrlEx::CStrList strlist = g_listRebarDir;
			//	//if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2) //放射
			//	//{
			//	//	strlist = g_listRebarRadiateDir;
			//	//}
			//	//m_listMainRebar.SetCellStringList(i, j, strlist);
			//	//auto it = strlist.begin();
			//	//advance(it, m_vecRebarData[i].rebarDir);
			//	//strValue = "";
			//}
			//break;
			case 1:
			{
				ListCtrlEx::CStrList strlist = g_listRebarSize;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				strValue = m_vecRebarData[i].rebarSize;
				strValue += _T("mm");
			}
			break;
			case 2:
			{
				ListCtrlEx::CStrList strlist = g_listRebarType;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecRebarData[i].rebarType);
				strValue = *it;
			}
			break;
			case 3:
				/*if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2 && m_vecRebarData[i].rebarDir == 1)
				{
					strValue.Format(_T("%.2f"), m_vecRebarData[i].angle);
				}
				else*/
				{
					strValue.Format(_T("%.2f"), m_vecRebarData[i].spacing);
				}
				break;
			case 4:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].startOffset);
				break;
			case 5:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].endOffset);
				break;
			case 6:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].levelSpace);
				break;
			case 7:
			{
				//之前为数据交换的功能
				ListCtrlEx::CStrList strlist = g_listRebartype;
				//djp,修改板配筋外侧面为顶底面str
				/*CString str;
				pm_MainPageRebar->GetWindowTextW(str);
				if (str.Find(L"板") != -1)
				{
					strlist = g_listfloorRebarPose;
				}*/
				//djp,修改板配筋外侧面为顶底面end
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				if (m_vecRebarData[i].datachange > 1)
				{
					m_vecRebarData[i].datachange = 0;
				}
				advance(it, i);
				strValue = *it;
			}
			break;
			//case 9:
			//{
			//	std::list<CString> RebarColor;
			//	for (int i = -1; i < 256; ++i)
			//	{
			//		CString tmpCStr;
			//		tmpCStr.Format(_T("%d"), i);
			//		RebarColor.push_back(tmpCStr);
			//	}
			//	m_listMainRebar.SetCellStringList(i, j, RebarColor);
			//	auto it = RebarColor.begin();
			//	advance(it, m_vecRebarData[i].rebarColor);
			//	strValue = *it;
			//	//strValue = _T("-1");
			//}
			//	break;
			case 8:
			{
				std::list<CString> Rebar_LineStyle;
				for (int i = 0; i < 8; ++i)
				{
					CString tmpCStr;
					tmpCStr.Format(_T("%d"), i);
					Rebar_LineStyle.push_back(tmpCStr);
				}
				m_listMainRebar.SetCellStringList(i, j, Rebar_LineStyle);
				auto it = Rebar_LineStyle.begin();
				advance(it, m_vecRebarData[i].rebarLineStyle);
				strValue = *it;
				//strValue = _T("0");
			}
			break;
			case 9:
			{
				std::list<CString> Rebar_Weight;
				for (int i = 0; i < 32; ++i)
				{
					CString tmpCStr;
					tmpCStr.Format(_T("%d"), i);
					Rebar_Weight.push_back(tmpCStr);
				}
				m_listMainRebar.SetCellStringList(i, j, Rebar_Weight);
				auto it = Rebar_Weight.begin();
				advance(it, m_vecRebarData[i].rebarWeight);
				strValue = *it;
				//strValue = _T("0");
			}
			break;
			default:
				break;
			}
			m_listMainRebar.SetItemText(i, j, strValue);
		}

	}
	//	m_listMainRebar.SetShowProgressPercent(TRUE);
	//	m_listMainRebar.SetSupportSort(TRUE);
}

void CUniteMainRebarDlg::GetConcreteData(PIT::Concrete& concreteData)
{
	// 	CString strLevelNum,strPostiveCover,strSideCover,strReverseCover;
	// 	m_EditLevel.GetWindowText(strLevelNum);
	// 	concreteData.rebarLevelNum = atoi(CT2A(strLevelNum));
	// 	concreteData.isTwinbars = m_ComboIsTwinBars.GetCurSel();
	// 	m_EditPositive.GetWindowText(strPostiveCover);
	// 	g_wallRebarInfo.concrete.postiveCover = atof(CT2A(strPostiveCover));
	// 	m_EditSide.GetWindowText(strSideCover);
	// 	g_wallRebarInfo.concrete.sideCover = atof(CT2A(strSideCover));
	// 	m_EditReverse.GetWindowText(strReverseCover);
	// 	g_wallRebarInfo.concrete.reverseCover = atof(CT2A(strReverseCover));
	concreteData = g_wallRebarInfo.concrete;
}

BOOL CUniteMainRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	g_wallMainDlg = this;
	/*if (g_SelectedElm.GetElementRef() == nullptr)
		GetDlgItem(IDC_BUTTON2)->ShowWindow(SW_HIDE);
	else
		GetDlgItem(IDC_BUTTON2)->ShowWindow(SW_SHOW);*/
	// TODO:  在此添加额外的初始化
	InitUIData();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CUniteMainRebarDlg::OnEnKillfocusEdit1()
{
	// TODO: 在此添加控件通知处理程序代码

	CString strPostiveCover;
	m_EditPositive.GetWindowText(strPostiveCover);
	g_wallRebarInfo.concrete.postiveCover = atof(CT2A(strPostiveCover));
}


void CUniteMainRebarDlg::OnEnKillfocusEdit2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strSideCover;
	m_EditSide.GetWindowText(strSideCover);
	g_wallRebarInfo.concrete.sideCover = atof(CT2A(strSideCover));
}


void CUniteMainRebarDlg::OnEnKillfocusEdit3()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strReverseCover;
	m_EditReverse.GetWindowText(strReverseCover);
	g_wallRebarInfo.concrete.reverseCover = atof(CT2A(strReverseCover));
}


void CUniteMainRebarDlg::OnEnKillfocusEdit4()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strLevelNum;
	m_EditLevel.GetWindowText(strLevelNum);
	g_wallRebarInfo.concrete.rebarLevelNum = atoi(CT2A(strLevelNum));
	UpdateRebarList();
}


LRESULT CUniteMainRebarDlg::OnComboBoxDataChange(WPARAM wParam, LPARAM lParam)
{
	SetListRowData(g_vecRebarData);
	UpdateRebarList();
	return 0;
}

//关联构件
void CUniteMainRebarDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_assodlg == NULL)
		ScanAllElements(g_mapidAndmodel);

	if (m_ehSel.IsValid())
	{
		ElementId testid = 0;
		GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
		if (testid != 0)
		{
			m_listMainRebar.GetAllRebarData(g_vecRebarData);
			SetElementXAttribute(testid, g_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		}
	}

	vector<PIT::AssociatedComponent> vecAC;
	if (m_assodlg != NULL)
	{
		m_assodlg->DestroyWindow();
		m_assodlg->GetListRowData(vecAC);
		delete m_assodlg;
		m_assodlg = NULL;
		//		vecAC = g_vecACData;
	}

	m_assodlg = new CWallRebarAssociatedComponentDlg(m_ehSel, this);
	if (vecAC.size())
	{
		m_assodlg->SetListRowData(vecAC);
	}
	m_assodlg->Create(IDD_DIALOG_WallRebar_AssociatedComponent, this);
	m_assodlg->ShowWindow(SW_NORMAL);
	m_assodlg->UpdateWindow();
}


void CUniteMainRebarDlg::OnBnClickedHoleCheck()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_hole_check.GetCheck())
	{
		m_mholesize_edit.SetReadOnly(FALSE);
		g_wallRebarInfo.concrete.isHandleHole = 1;

	}
	else
	{
		m_mholesize_edit.SetReadOnly(TRUE);
		g_wallRebarInfo.concrete.isHandleHole = 0;
	}
}


void CUniteMainRebarDlg::OnEnKillfocusMholesizeEdit()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strMissHoleSize;
	m_mholesize_edit.GetWindowText(strMissHoleSize);
	g_wallRebarInfo.concrete.MissHoleSize = atof(CT2A(strMissHoleSize));
}



void CUniteMainRebarDlg::OnBnClickedButton2()
{
	CWallRebarDlg* dlg = dynamic_cast<CWallRebarDlg*>(pm_MainPageRebar);

	if (dlg == nullptr)
	{//板
		CSlabRebarDlg* dlg = dynamic_cast<CSlabRebarDlg*>(pm_MainPageRebar);
		if (dlg != nullptr)
		{
			dlg->PreviewRebarLines();
		}
	}
	else
	{//墙
		dlg->PreviewRebarLines();
	}
}


void CUniteMainRebarDlg::OnCbnSelchangeCombo1()//配筋方式（板才有）
{
	g_wallRebarInfo.concrete.m_SlabRebarMethod = m_ComRebarMethod.GetCurSel();
	m_listMainRebar.DeleteColumn(4);
	tagRECT stRect;
	m_listMainRebar.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2) //放射
	{
		m_listMainRebar.InsertColumn(4, _T("环向间距/径向角度"), (int)(width / 9.0 * 2), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	}
	else
	{
		m_listMainRebar.InsertColumn(4, _T("间距"), (int)(width / 9.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	}
	UpdateRebarList();
}



void CUniteMainRebarDlg::OnBnClickedButtonBreakellipsewall()
{
	CBreakEllipseWallDlg dlg(this);
	bool res = dlg.SetSelectElement(m_ehSel);
	if (!res)
	{
		return;
	}
	dlg.DoModal();
	m_vecListData = dlg.GetBreakAngleData();
}


//void CUniteMainRebarDlg::OnBnClickedSavedata()
//{ 
//	// TODO: 在此添加控件通知处理程序代码
//	this->Set_m_DlgData();
//}


void CUniteMainRebarDlg::OnBnClickedRebartemplate()
{
	// TODO: 在此添加控件通知处理程序代码
	// 1、获取当前界面保护层等数据
	CString strpostive;
	GetDlgItemText(IDC_EDIT1, strpostive);
	CString strside;
	GetDlgItemText(IDC_EDIT2, strside);
	CString strreverse;
	GetDlgItemText(IDC_EDIT3, strreverse);
	CString strholesize;
	GetDlgItemText(IDC_MHOLESIZE_EDIT, strholesize);
	CString strlevelnum;
	GetDlgItemText(IDC_EDIT4, strlevelnum);

	// 2、将其数据保存到参数模板类
	m_dlgData.postiveCover = _ttof(strpostive);
	m_dlgData.sideCover = _ttof(strside);
	m_dlgData.reverseCover = _ttof(strreverse);
	m_dlgData.missHoleSize = _ttof(strholesize);
	m_dlgData.rebarLevelNum = _ttoi(strlevelnum);
	m_RebarTemplate.Set_m_dlgData(m_dlgData);

	// 3、将表格数据保存到参数模板类
	std::vector<PIT::ConcreteRebar> DlgvecRebarData;
	this->GetAllRebarData(DlgvecRebarData);
	m_RebarTemplate.Set_m_vecRebarData(DlgvecRebarData);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_RebarTemplate.Set_isFloor();
	m_RebarTemplate.Create(IDD_DIALOG_RebarTemplate);
	//m_RebarTemplate.set_dlg(this);
	m_RebarTemplate.SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);//使窗口总是在最前面
	//m_RebarTemplate.DoModal();
	m_RebarTemplate.ShowWindow(SW_SHOW);


}


void CUniteMainRebarDlg::OnBnClickedUpdatadlg()
{
	// TODO: 在此添加控件通知处理程序代码
	// 1.设置保护层等数据
	PIT::DlgData main_dlg = m_RebarTemplate.m_Get_dlgData;
	CString str_postive;
	str_postive.Format(L"%.2f", main_dlg.postiveCover);
	m_EditPositive.SetWindowTextW(str_postive);
	g_wallRebarInfo.concrete.postiveCover = main_dlg.postiveCover;

	CString str_side;
	str_side.Format(L"%.2f", main_dlg.sideCover);
	m_EditSide.SetWindowTextW(str_side);
	g_wallRebarInfo.concrete.sideCover = main_dlg.sideCover;

	CString str_reverse;
	str_reverse.Format(L"%.2f", main_dlg.reverseCover);
	m_EditReverse.SetWindowTextW(str_reverse);
	g_wallRebarInfo.concrete.reverseCover = main_dlg.reverseCover;

	CString str_holesize;
	str_holesize.Format(L"%.2f", main_dlg.missHoleSize);
	m_mholesize_edit.SetWindowTextW(str_holesize);
	g_wallRebarInfo.concrete.MissHoleSize = main_dlg.missHoleSize;

	CString str_levelnum;
	str_levelnum.Format(L"%d", main_dlg.rebarLevelNum);
	m_EditLevel.SetWindowTextW(str_levelnum);
	g_wallRebarInfo.concrete.rebarLevelNum = main_dlg.rebarLevelNum;



	// 2.设置表格数据
	m_listMainRebar.DeleteAllItems();
	std::vector<PIT::ConcreteRebar> main_listdata = m_RebarTemplate.m_Get_vecRebarData;
	m_RebarTemplate.m_Get_vecRebarData.clear();
	for (int i = 0; i < main_listdata.size(); ++i)
	{
		CString strValue;
		strValue.Format(_T("%dL"), main_listdata[i].rebarLevel);
		m_listMainRebar.InsertItem(i, _T("")); // 插入行
		m_listMainRebar.SetItemText(i, 0, strValue);
		int nSubCnt = m_listMainRebar.GetColumnCount() - 1;
		for (int j = 1; j <= nSubCnt; ++j)
		{
			CString strValue;
			switch (j)
			{
			case 1:
			{
				ListCtrlEx::CStrList strlist = g_listRebarDir;
				if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2) //放射
				{
					strlist = g_listRebarRadiateDir;
				}
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, main_listdata[i].rebarDir);
				strValue = *it;
			}
			break;
			case 2:
			{
				ListCtrlEx::CStrList strlist = g_listRebarSize;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				strValue = main_listdata[i].rebarSize;
				strValue += _T("mm");
			}
			break;
			case 3:
			{
				ListCtrlEx::CStrList strlist = g_listRebarType;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, main_listdata[i].rebarType);
				strValue = *it;
			}
			break;
			case 4:
				if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2 && main_listdata[i].rebarDir == 1)
				{
					strValue.Format(_T("%.2f"), main_listdata[i].angle);
				}
				else
				{
					strValue.Format(_T("%.2f"), main_listdata[i].spacing);
				}
				break;
			case 5:
				strValue.Format(_T("%.2f"), main_listdata[i].startOffset);
				break;
			case 6:
				strValue.Format(_T("%.2f"), main_listdata[i].endOffset);
				break;
			case 7:
				strValue.Format(_T("%.2f"), main_listdata[i].levelSpace);
				break;
			case 8:
			{
				//之前为数据交换的功能
				/*ListCtrlEx::CStrList strlist;
				strlist.push_back(_T("无"));
				for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
				{
					CString strValue;
					if (0 == i)
						strValue = _T("1XL");
					else
						strValue.Format(_T("%dL"), i);
					strlist.push_back(strValue);
				}
				m_listMainRebar.SetCellStringList(i, j, strlist);
				strValue = _T("无");*/
				ListCtrlEx::CStrList strlist = g_listfloorRebarPose;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				if (main_listdata[i].datachange > 2)
				{
					main_listdata[i].datachange = 0;
				}
				advance(it, main_listdata[i].datachange);
				strValue = *it;
			}
			break;
			//case 9:
			//{
			//	std::list<CString> RebarColor;
			//	for (int i = -1; i < 256; ++i)
			//	{
			//		CString tmpCStr;
			//		tmpCStr.Format(_T("%d"), i);
			//		RebarColor.push_back(tmpCStr);
			//	}
			//	m_listMainRebar.SetCellStringList(i, j, RebarColor);
			//	auto it = RebarColor.begin();
			//	advance(it, main_listdata[i].rebarColor);
			//	strValue = *it;
			//	//strValue = _T("-1");
			//}
			//	break;
			case 9:
			{
				std::list<CString> Rebar_LineStyle;
				for (int i = 0; i < 8; ++i)
				{
					CString tmpCStr;
					tmpCStr.Format(_T("%d"), i);
					Rebar_LineStyle.push_back(tmpCStr);
				}
				m_listMainRebar.SetCellStringList(i, j, Rebar_LineStyle);
				auto it = Rebar_LineStyle.begin();
				advance(it, main_listdata[i].rebarLineStyle);
				strValue = *it;
				//strValue = _T("0");
			}
			break;
			case 10:
			{
				std::list<CString> Rebar_Weight;
				for (int i = 0; i < 32; ++i)
				{
					CString tmpCStr;
					tmpCStr.Format(_T("%d"), i);
					Rebar_Weight.push_back(tmpCStr);
				}
				m_listMainRebar.SetCellStringList(i, j, Rebar_Weight);
				auto it = Rebar_Weight.begin();
				advance(it, main_listdata[i].rebarWeight);
				strValue = *it;
				//strValue = _T("0");
			}
			break;
			default:
				break;
			}
			m_listMainRebar.SetItemText(i, j, strValue);
		}

	}
	g_vecRebarData = main_listdata;
	main_listdata.clear();

}
