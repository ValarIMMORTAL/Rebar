// BaseRebarDlg.cpp: 实现文件
//
#include "Public.h"
#include "_USTATION.h"
#include "BaseRebarDlg.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"
#include "PITPSPublic.h"
#include "RebarElements.h"
#include "ElementAttribute.h"
#include "BaseRebarAssembly.h"
#include "ExtractFacesTool.h"
#include "resource.h"
#include "PITACCRebarAssembly.h"


// BaseRebarDlg 对话框

IMPLEMENT_DYNAMIC(BaseRebarDlg, CDialogEx)

BaseRebarDlg::BaseRebarDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_BaseRebar, pParent)
{
	m_vecRebarData.clear();
	m_vecRebarData.shrink_to_fit();
	m_vecLapOptionData.clear();
	m_vecLapOptionData.shrink_to_fit();
	m_vecEndTypeData.clear();
	m_vecEndTypeData.shrink_to_fit();
	m_vecACData.clear();
	m_vecACData.shrink_to_fit();
	m_vecTwinBarData.clear();
	m_vecTwinBarData.shrink_to_fit();
	memset(&m_twinBarInfo, 0, sizeof(TwinBarSet::TwinBarInfo));
	memset(&m_tieRebarInfo, 0, sizeof(TieReBarInfo));

	


}

BaseRebarDlg::~BaseRebarDlg()
{
}

void BaseRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//  DDX_Control(pDX, IDC_S_LIST1, m_listBaseRebar);
	DDX_Control(pDX, IDC_S_EDIT1, m_EditPositive);
	DDX_Control(pDX, IDC_S_EDIT2, m_EditSide);
	DDX_Control(pDX, IDC_S_EDIT3, m_EditReverse);
	DDX_Control(pDX, IDC_S_EDIT4, m_StirrupNum_edit);
	DDX_Control(pDX, IDC_S_EDIT5, m_StirrupStart_edit);
	DDX_Control(pDX, IDC_S_EDIT6, m_StirrupEnd_edit);
	//  DDX_Control(pDX, IDC_S_EDIT7, m_Len_edit);
	DDX_Control(pDX, IDC_COMBO1, m_ComboSize);
	DDX_Control(pDX, IDC_COMBO12, m_ComboType);
	//  DDX_Control(pDX, IDC_S_HOLE_CHECK, m_hole_check);
	//  DDX_Control(pDX, IDC_S_MHOLESIZE_EDIT, m_mholesize_edit);
	DDX_Control(pDX, IDC_EDIT_HNum, m_edit_HPtNum);
	DDX_Control(pDX, IDC_EDIT_VNum, m_edit_VPtNum);
	DDX_Control(pDX, IDC_COMBO_Diameter, m_Cob_PtDiameter);
	DDX_Control(pDX, IDC_COMBO_Grade, m_Cob_PtGrade);
	//  DDX_Control(pDX, IDC_COMBO_TieDiameter, m_Cob_TieDiameter);
	//  DDX_Control(pDX, IDC_COMBO_TieGrade, m_Cob_TieGrade);
	//  DDX_Control(pDX, IDC_EDIT_TieNum, m_edit_TieNum);
}


BEGIN_MESSAGE_MAP(BaseRebarDlg, CDialogEx)
	ON_EN_CHANGE(IDC_S_EDIT1, &BaseRebarDlg::OnEnChangeSEdit1)
	ON_EN_CHANGE(IDC_S_EDIT2, &BaseRebarDlg::OnEnChangeSEdit2)
	ON_EN_CHANGE(IDC_S_EDIT3, &BaseRebarDlg::OnEnChangeSEdit3)
//	ON_BN_CLICKED(IDC_S_HOLE_CHECK, &BaseRebarDlg::OnBnClickedSHoleCheck)
//	ON_EN_CHANGE(IDC_S_MHOLESIZE_EDIT, &BaseRebarDlg::OnEnChangeSMholesizeEdit)
	ON_BN_CLICKED(IDOK, &BaseRebarDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBO12, &BaseRebarDlg::OnCbnSelchangeCombo12)
	ON_EN_CHANGE(IDC_S_EDIT4, &BaseRebarDlg::OnEnChangeSEdit4)
	ON_CBN_SELCHANGE(IDC_COMBO1, &BaseRebarDlg::OnCbnSelchangeCombo1)
	ON_EN_CHANGE(IDC_S_EDIT5, &BaseRebarDlg::OnEnChangeSEdit5)
	ON_EN_CHANGE(IDC_S_EDIT6, &BaseRebarDlg::OnEnChangeSEdit6)
//	ON_EN_CHANGE(IDC_S_EDIT7, &BaseRebarDlg::OnEnChangeSEdit7)
	ON_EN_CHANGE(IDC_EDIT_HNum, &BaseRebarDlg::OnEnChangeEditHnum)
	ON_EN_CHANGE(IDC_EDIT_VNum, &BaseRebarDlg::OnEnChangeEditVnum)
//	ON_EN_CHANGE(IDC_EDIT_TieNum, &BaseRebarDlg::OnEnChangeEditTienum)
	ON_CBN_SELCHANGE(IDC_COMBO_Diameter, &BaseRebarDlg::OnCbnSelchangeComboDiameter)
	ON_CBN_SELCHANGE(IDC_COMBO_Grade, &BaseRebarDlg::OnCbnSelchangeComboGrade)
//	ON_CBN_SELCHANGE(IDC_COMBO_TieDiameter, &BaseRebarDlg::OnCbnSelchangeComboTiediameter)
//	ON_CBN_SELCHANGE(IDC_COMBO_TieGrade, &BaseRebarDlg::OnCbnSelchangeComboTiegrade)
END_MESSAGE_MAP()


void BaseRebarDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

BOOL BaseRebarDlg::OnInitDialog()                 //初始化
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端

	for each (auto var in g_listRebarType)
	{
		m_ComboType.AddString(var);
		m_Cob_PtGrade.AddString(var);
		//m_Cob_TieGrade.AddString(var);
	}
		

	for each (auto var in g_listRebarSize)
	{
		m_ComboSize.AddString(var);
		m_Cob_PtDiameter.AddString(var);
		//m_Cob_TieDiameter.AddString(var);
	}
		
	m_PTRebarData.ptHNum = 2;
	m_PTRebarData.ptVNum = 2;
	sprintf(m_PTRebarData.ptrebarSize, "%s", "12");
	m_PTRebarData.ptrebarType = 0;
	sprintf(m_PTRebarData.stirrupRebarsize, "%s", "12");
	m_PTRebarData.stirrupRebarType = 0;
	/*m_PTRebarData.tieNum = 1;
	sprintf(m_PTRebarData.tierebarSize, "%s", "12");
	m_PTRebarData.tierebarType = 0;*/

	m_StirrupData.rebarEndOffset = 50;
	m_StirrupData.rebarStartOffset = 50;
	m_StirrupData.rebarNum = 1;
	sprintf(m_StirrupData.rebarSize, "%s", "12");
	m_StirrupData.rebarType = 0;
	m_StirrupData.tranLenth = 200;


	CDialogEx::OnInitDialog();
	ElementId contid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, m_ehSel.GetModelRef());

	if (contid != 0)
	{
		GetElementXAttribute(contid, sizeof(WallRebarInfo), g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
		GetElementXAttribute(contid, sizeof(StirrupData), m_StirrupData, BaseRebarStirrupXAttribute, ACTIVEMODEL);
		GetElementXAttribute(contid, sizeof(Abanurus_PTRebarData), m_PTRebarData, BaseAbanurus_PTRebarDataXAttribute, ACTIVEMODEL);
		GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		strcpy(m_PTRebarData.stirrupRebarsize, m_StirrupData.rebarSize);
	}
	InitUIData();
	
	
	return TRUE;  
}


void BaseRebarDlg::InitUIData()
{
	CString strLevel,strPositive, strSide, strReverse, strMissHoleSize;
	g_wallRebarInfo.concrete.rebarLevelNum = 2;
	strLevel.Format(_T("%d"), g_wallRebarInfo.concrete.rebarLevelNum);
	strPositive.Format(_T("%.2f"), g_wallRebarInfo.concrete.postiveCover);
	strReverse.Format(_T("%.2f"), g_wallRebarInfo.concrete.reverseCover);
	strSide.Format(_T("%.2f"), g_wallRebarInfo.concrete.sideCover);
	strMissHoleSize.Format(_T("%.2f"), g_wallRebarInfo.concrete.MissHoleSize);
	if (g_wallRebarInfo.concrete.isHandleHole)
	{
		//m_hole_check.SetCheck(true);
		//m_mholesize_edit.SetReadOnly(FALSE);
	}
	else
	{
		//m_hole_check.SetCheck(false);
		//m_mholesize_edit.SetReadOnly(TRUE);
	}

	m_EditPositive.SetWindowText(strPositive);                          //顶部保护层
	m_EditReverse.SetWindowText(strReverse);                             //底部保护层
	m_EditSide.SetWindowText(strSide);                                 //侧面保护层
	//m_mholesize_edit.SetWindowText(strMissHoleSize);


	CString strLen = L"";
	strLen.Format(_T("%.2f"), m_StirrupData.tranLenth);
	//m_Len_edit.SetWindowText(strLen);

	//箍筋数据
	CString strStirrupNum, strStirrupStart,strStirrupEnd;
	strStirrupNum.Format(_T("%d"), m_StirrupData.rebarNum);
	m_StirrupNum_edit.SetWindowText(strStirrupNum);
	strStirrupStart.Format(_T("%d"), m_StirrupData.rebarStartOffset);
	m_StirrupStart_edit.SetWindowText(strStirrupStart);
	strStirrupEnd.Format(_T("%d"), m_StirrupData.rebarEndOffset);
	m_StirrupEnd_edit.SetWindowText(strStirrupEnd);

	CString strStirrupSize(m_StirrupData.rebarSize);
	if (strStirrupSize.Find(L"A") != -1 || strStirrupSize.Find(L"B") != -1 || strStirrupSize.Find(L"C") != -1 || strStirrupSize.Find(L"D") != -1)
		strStirrupSize.Delete(strStirrupSize.GetLength() - 1);
	if (strStirrupSize.Find(L"mm") == -1)
		strStirrupSize += "mm";
	int nIndex = m_ComboSize.FindStringExact(0, strStirrupSize);
	m_ComboSize.SetCurSel(nIndex);//尺寸
	m_ComboType.SetCurSel(m_StirrupData.rebarType);//型号

	//点筋数据
	CString strHptnum, strVptnum, strTienum;
	strHptnum.Format(_T("%d"), m_PTRebarData.ptHNum);
	m_edit_HPtNum.SetWindowText(strHptnum);
	strVptnum.Format(_T("%d"), m_PTRebarData.ptVNum);
	m_edit_VPtNum.SetWindowTextW(strVptnum);
	CString strPtSize(m_PTRebarData.ptrebarSize);
	if (strPtSize.Find(L"A") != -1 || strPtSize.Find(L"B") != -1 || strPtSize.Find(L"C") != -1 || strPtSize.Find(L"D") != -1)
		strPtSize.Delete(strPtSize.GetLength() - 1);
	if (strPtSize.Find(L"mm") == -1)
		strPtSize += "mm";
	int ptIndex = m_Cob_PtDiameter.FindStringExact(0, strPtSize);
	m_Cob_PtDiameter.SetCurSel(ptIndex);//尺寸
	m_Cob_PtGrade.SetCurSel(m_PTRebarData.ptrebarType);//型号

	//拉筋数据
	//strTienum.Format(_T("%d"), m_PTRebarData.tieNum);
	//m_edit_TieNum.SetWindowTextW(strTienum);
	//CString strTieSize(m_PTRebarData.tierebarSize);
	//if (strTieSize.Find(L"A") != -1 || strTieSize.Find(L"B") != -1 || strTieSize.Find(L"C") != -1 || strTieSize.Find(L"D") != -1)
	//	strTieSize.Delete(strTieSize.GetLength() - 1);
	//if (strTieSize.Find(L"mm") == -1)
	//	strTieSize += "mm";
	//int tieIndex = m_Cob_TieDiameter.FindStringExact(0, strTieSize);
	//m_Cob_TieDiameter.SetCurSel(tieIndex);//尺寸
	//m_Cob_TieGrade.SetCurSel(m_PTRebarData.tierebarType);//型号

	//LONG lStyle;
	//lStyle = GetWindowLong(m_listBaseRebar.m_hWnd, GWL_STYLE);//获取当前窗口style
	//lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	//lStyle |= LVS_REPORT;	//设置style
	//lStyle |= LVS_SINGLESEL;//单选模式
	//SetWindowLong(m_listBaseRebar.m_hWnd, GWL_STYLE, lStyle);//设置style

	//DWORD dwStyle = m_listBaseRebar.GetExtendedStyle();
	//dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	//dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	//m_listBaseRebar.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	////设置列表控件的报表显示方式
	////m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	//tagRECT stRect;
	//m_listBaseRebar.GetClientRect(&stRect);
	//double width = stRect.right - stRect.left;
	////在列表控件中插入列
	//m_listBaseRebar.InsertColumn(0, _T("层"), (int)(width / 8 * 0.75), ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	//m_listBaseRebar.InsertColumn(1, _T("方向"), (int)(width / 8 * 0.75), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	//m_listBaseRebar.InsertColumn(2, _T("直径"), (int)(width / 8 * 0.75), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	//m_listBaseRebar.InsertColumn(3, _T("类型"), (int)(width / 8), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	//m_listBaseRebar.InsertColumn(4, _T("间距"), (int)(width / 8 * 0.75), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	//m_listBaseRebar.InsertColumn(5, _T("起点偏移"), (int)(width / 8 * 0.75), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	//m_listBaseRebar.InsertColumn(6, _T("终点偏移"), (int)(width / 8 * 0.75), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	//m_listBaseRebar.InsertColumn(7, _T("图层"), (int)(width / 8*2.5), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

	//UpdateRebarList();
}



void BaseRebarDlg::SetListDefaultData()                   //添加钢筋的默认层的信息
{
	if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < 2; ++i)
		{
			PIT::ConcreteRebar oneRebarData;

			if (0 == i)
				oneRebarData = { i,0,"12",0,150,0,0 };
			else if (1 == i)
				oneRebarData = { i,1,"12",0,150,0,0 };
			else
			{
			}
			m_vecRebarData.push_back(oneRebarData);
		}
	}
	for (int i = 0; i < 2 * 2; i++)
	{
		PIT::EndType::RebarEndPointInfo endPtInfo;
		memset(&endPtInfo, 0, sizeof(endPtInfo));
		PIT::EndType oneEndTypeData = { 0,0,0 ,endPtInfo };
		m_vecEndTypeData.push_back(oneEndTypeData);
	}
	for (int i = 0; i < m_vecRebarData.size(); i++)
	{
		CString strTwinBar = L"";
		TwinBarSet::TwinBarLevelInfo oneTwinBarData;
		strcpy(oneTwinBarData.levelName, CT2A(strTwinBar.GetBuffer()));
		oneTwinBarData.hasTwinbars = 0;
		strcpy(oneTwinBarData.rebarSize, m_vecRebarData[i].rebarSize);
		oneTwinBarData.rebarType = m_vecRebarData[i].rebarType;
		oneTwinBarData.interval = 0;
		m_vecTwinBarData.push_back(oneTwinBarData);
	}
	memset(&m_tieRebarInfo, 0, sizeof(TieReBarInfo));
	memset(&m_twinBarInfo, 0, sizeof(TwinBarSet::TwinBarInfo));
	g_vecRebarData = m_vecRebarData;
}


void BaseRebarDlg::UpdateRebarList()                      //更新
{
	//m_listBaseRebar.DeleteAllItems();
	//SetListDefaultData();
	//for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
	//{
	//	if (0 == i)
	//	{
	//		m_listBaseRebar.InsertItem(i, _T("")); // 插入行
	//		m_listBaseRebar.SetItemText(i, 0, _T("1LX"));
	//	}
	//	else
	//	{
	//		CString strValue;
	//		strValue.Format(_T("%dL"), i);
	//		m_listBaseRebar.InsertItem(i, _T("")); // 插入行
	//		m_listBaseRebar.SetItemText(i, 0, strValue);

	//	}
	//	int nSubCnt = m_listBaseRebar.GetColumnCount() - 1;                    //返回此表中从 1 开始的列号。
	//	for (int j = 1; j <= nSubCnt; ++j)
	//	{
	//		CString strValue;
	//		switch (j)
	//		{
	//		case 1:
	//		{
	//			ListCtrlEx::CStrList strlist = g_listSlabRebarDir;
	//			m_listBaseRebar.SetCellStringList(i, j, strlist);
	//			auto it = strlist.begin();
	//			advance(it, m_vecRebarData[i].rebarDir);
	//			strValue = *it;
	//		}
	//		break;
	//		case 2:
	//		{
	//			ListCtrlEx::CStrList strlist = g_listRebarSize;
	//			m_listBaseRebar.SetCellStringList(i, j, strlist);
	//			strValue = m_vecRebarData[i].rebarSize;
	//			if (strValue.Find(L"A") != -1 || strValue.Find(L"B") != -1 || strValue.Find(L"C") != -1 || strValue.Find(L"D") != -1)
	//				strValue.Delete(strValue.GetLength() - 1);
	//			if (strValue.Find(L"mm") == -1)
	//				strValue += _T("mm");
	//		}
	//		break;
	//		case 3:
	//		{
	//			ListCtrlEx::CStrList strlist = g_listRebarType;
	//			m_listBaseRebar.SetCellStringList(i, j, strlist);
	//			auto it = strlist.begin();
	//			advance(it, m_vecRebarData[i].rebarType);
	//			strValue = *it;
	//		}
	//		break;
	//		case 4:
	//			strValue.Format(_T("%.2f"), m_vecRebarData[i].spacing);
	//			break;
	//		case 5:
	//			strValue.Format(_T("%.2f"), m_vecRebarData[i].startOffset);
	//			break;
	//		case 6:
	//			strValue.Format(_T("%.2f"), m_vecRebarData[i].endOffset);
	//			break;
	//		case 7:
	//		{
	//			list<CString> levelNames;
	//			for (LevelHandle lh : ACTIVEMODEL->GetLevelCacheR())//获取DGN文件图层名字
	//			{
	//				WString    lvlName1;
	//				lh.GetDisplayName(lvlName1);
	//				if (lvlName1.find(L"CW_") == 0)
	//				{
	//					CString lvlNames1 = lvlName1.c_str();
	//					levelNames.push_back(lvlNames1);
	//				}
	//				
	//			}
	//			int listNum1 = levelNames.size();

	//			FileLevelCacheR fileLvlCac = ISessionMgr::GetManager().GetActiveDgnFile()->GetLevelCacheR();

	//			T_LevelCachePtrVector*   t_lvlCacPtrVec = fileLvlCac.GetLibraries();
	//			for (FileLevelCachePtr* fileLvlCacPtr = t_lvlCacPtrVec->begin(); fileLvlCacPtr != t_lvlCacPtrVec->end(); fileLvlCacPtr++)//获取DGNlib文件图层名字
	//			{
	//				for (LevelHandle lvlHan = fileLvlCacPtr->GetCR()->begin(); lvlHan != fileLvlCacPtr->GetCR()->end(); ++lvlHan)
	//				{
	//					WString lvlName(lvlHan.GetName());
	//					if (lvlName.find(L"CW_") == 0)
	//					{
	//						CString lvlNames = lvlName.c_str();
	//						levelNames.push_back(lvlNames);
	//					}
	//					
	//				}
	//			}
	//			int listNum2 = levelNames.size();
	//			m_listBaseRebar.SetCellStringList(i, j, levelNames);
	//			/*auto it = levelNames.begin();
	//			strValue = *it;*/
	//			strValue = TEXT_MAIN_REBAR;
	//			break;
	//		}
	//		default:
	//			break;
	//		}
	//		m_listBaseRebar.SetItemText(i, j, strValue);
	//	}

	//}
	//g_vecRebarData = m_vecRebarData;
}


void BaseRebarDlg::GetConcreteData(PIT::Concrete& concreteData)
{
	concreteData = g_wallRebarInfo.concrete;
}

void BaseRebarDlg::OnBnClickedCancel()
{

	CDialogEx::OnCancel();
	DestroyWindow();
}

void BaseRebarDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();

	GetConcreteData(g_wallRebarInfo.concrete);
	//m_listBaseRebar.GetAllRebarData(m_vecRebarData);

	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = m_ehSel.GetModelRef();

	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());
	AbanurusRebarAssembly*  BaseRebar = NULL;

	AbanurusRebarAssembly::IsSmartSmartFeature(eeh);

	ElementId contid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, eeh.GetModelRef());
	/***********************************给sizekey附加型号******************************************************/
	auto it = m_vecRebarData.begin();
	for (; it != m_vecRebarData.end(); it++)
	{
		BrString strRebarSize = it->rebarSize;
		if (strRebarSize != L"")
		{
			if (strRebarSize.Find(L"mm") != -1)
			{
				strRebarSize.Replace(L"mm", L"");
			}
		}
		else
		{
			strRebarSize = XmlManager::s_alltypes[it->rebarType];
		}
		strcpy(it->rebarSize, CT2A(strRebarSize));
		GetDiameterAddType(it->rebarSize, it->rebarType);
		it->datachange = 2;
	}

	BrString strStirrupSize = m_StirrupData.rebarSize;
	if (strStirrupSize.Find(L"A") != -1 || strStirrupSize.Find(L"B") != -1 || strStirrupSize.Find(L"C") != -1 || strStirrupSize.Find(L"D") != -1)
		strStirrupSize.Delete(strStirrupSize.GetLength() - 1);
	if (strStirrupSize != L"")
	{
		if (strStirrupSize.Find(L"mm") != -1)
		{
			strStirrupSize.Replace(L"mm", L"");
		}
	}
	strcpy(m_StirrupData.rebarSize, CT2A(strStirrupSize));
	GetDiameterAddType(m_StirrupData.rebarSize, m_StirrupData.rebarType);
	strcpy(m_PTRebarData.stirrupRebarsize, m_StirrupData.rebarSize);
	//设置点筋数据给sizekey附加型号
	BrString strPtSize = m_PTRebarData.ptrebarSize;
	if (strPtSize.Find(L"A") != -1 || strPtSize.Find(L"B") != -1 || strPtSize.Find(L"C") != -1 || strPtSize.Find(L"D") != -1)
		strPtSize.Delete(strPtSize.GetLength() - 1);
	if (strPtSize != L"")
	{
		if (strPtSize.Find(L"mm") != -1)
		{
			strPtSize.Replace(L"mm", L"");
		}
	}
	strcpy(m_PTRebarData.ptrebarSize, CT2A(strPtSize));
	GetDiameterAddType(m_PTRebarData.ptrebarSize, m_PTRebarData.ptrebarType);
	/***********************************给sizekey附加型号******************************************************/

// 	BaseType baseType = AbanurusRebarAssembly::JudgeBaseType(m_ehSel);

	RebarAssembly* rebaras = PIT::ACCRebarAssembly::GetRebarAssembly(contid, "class AbanurusRebarAssembly");
	BaseRebar = dynamic_cast<AbanurusRebarAssembly*>(rebaras);
	if (BaseRebar == nullptr)
	{
		BaseRebar = REA::Create<AbanurusRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	}
	BaseRebar->PopvecFrontPts().clear();
	BaseRebar->SetSlabData(eeh);
	BaseRebar->SetConcreteData(g_wallRebarInfo.concrete, m_StirrupData, m_StirrupData.tranLenth, m_PTRebarData);
	BaseRebar->SetRebarData(m_vecRebarData);
	BaseRebar->SetvecLapOptions(m_vecLapOptionData);
	BaseRebar->SetRebarEndTypes(m_vecEndTypeData);
	BaseRebar->SetvecTwinRebarLevel(m_vecTwinBarData);
	//		wallRebar->InitRebarSetId();
	BaseRebar->SetTieRebarInfo(m_tieRebarInfo);
	//BaseRebar->InitRebarParam(m_StirrupData.tranLenth);
	BaseRebar->MakeRebars(modelRef);
	BaseRebar->Save(modelRef); // must save after creating rebars
	contid = BaseRebar->FetchConcrete();
	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);

	SetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	SetElementXAttribute(contid, sizeof(WallRebarInfo), &g_wallRebarInfo, WallRebarInfoXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, sizeof(StirrupData), &m_StirrupData, BaseRebarStirrupXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, sizeof(Abanurus_PTRebarData), &m_PTRebarData, BaseAbanurus_PTRebarDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);

	DestroyWindow();
}




void BaseRebarDlg::OnEnChangeSEdit1()//正面保护层
{
	CString strPostiveCover;
	m_EditPositive.GetWindowText(strPostiveCover);
	g_wallRebarInfo.concrete.postiveCover = atof(CT2A(strPostiveCover));
}


void BaseRebarDlg::OnEnChangeSEdit2()//侧面保护层
{
	CString strSideCover;
	m_EditSide.GetWindowText(strSideCover);
	g_wallRebarInfo.concrete.sideCover = atof(CT2A(strSideCover));
}


void BaseRebarDlg::OnEnChangeSEdit3()//底面保护层
{
	CString strReverseCover;
	m_EditReverse.GetWindowText(strReverseCover);
	g_wallRebarInfo.concrete.reverseCover = atof(CT2A(strReverseCover));
}


//void BaseRebarDlg::OnBnClickedSHoleCheck()
//{
//	if (m_hole_check.GetCheck())
//	{
//		m_mholesize_edit.SetReadOnly(FALSE);
//		g_wallRebarInfo.concrete.isHandleHole = 1;
//
//	}
//	else
//	{
//		m_mholesize_edit.SetReadOnly(TRUE);
//		g_wallRebarInfo.concrete.isHandleHole = 0;
//	}
//}


//void BaseRebarDlg::OnEnChangeSMholesizeEdit()//忽略尺寸
//{
//	CString strMissHoleSize;
//	m_mholesize_edit.GetWindowText(strMissHoleSize);
//	g_wallRebarInfo.concrete.MissHoleSize = atof(CT2A(strMissHoleSize));
//}




void BaseRebarDlg::OnCbnSelchangeCombo12()//箍筋类型
{
	auto it = g_listRebarType.begin();
	advance(it, m_ComboType.GetCurSel());
	m_StirrupData.rebarType = m_ComboType.GetCurSel();
	m_PTRebarData.stirrupRebarType = m_ComboType.GetCurSel();
}


void BaseRebarDlg::OnEnChangeSEdit4()//箍筋数量
{
	CString strStirrupNum;
	m_StirrupNum_edit.GetWindowText(strStirrupNum);
	m_StirrupData.rebarNum = atoi(CT2A(strStirrupNum));
}


void BaseRebarDlg::OnCbnSelchangeCombo1()//箍筋直径
{
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboSize.GetCurSel());
	strcpy(m_StirrupData.rebarSize, CT2A(*it));
	strcpy(m_PTRebarData.stirrupRebarsize, CT2A(*it));
}


void BaseRebarDlg::OnEnChangeSEdit5()//箍筋起点偏移
{
	CString strStartOffset;
	m_StirrupStart_edit.GetWindowText(strStartOffset);
	m_StirrupData.rebarStartOffset = atof(CT2A(strStartOffset));
}


void BaseRebarDlg::OnEnChangeSEdit6()
{
	CString strEndOffset;
	m_StirrupEnd_edit.GetWindowText(strEndOffset);
	m_StirrupData.rebarEndOffset = atof(CT2A(strEndOffset));
}


//void BaseRebarDlg::OnEnChangeSEdit7()//延申长度
//{
//	CString strLen;
//	m_Len_edit.GetWindowText(strLen);
//	m_StirrupData.tranLenth = atof(CT2A(strLen));
//}


void BaseRebarDlg::OnEnChangeEditHnum()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString strptHnum;
	m_edit_HPtNum.GetWindowText(strptHnum);
	int Hnum = atoi(CT2A(strptHnum));
	if(Hnum >= 2)
		m_PTRebarData.ptHNum = atoi(CT2A(strptHnum));
	else
	{
		mdlDialog_dmsgsPrint(L"横向数量至少为2");
		Hnum = 2;
		CString strHnum;
		strHnum.Format(_T("%d"), Hnum);
		m_edit_HPtNum.SetWindowTextW(strHnum);
		m_PTRebarData.ptHNum = Hnum;
	}

}


void BaseRebarDlg::OnEnChangeEditVnum()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString strptVnum;
	m_edit_VPtNum.GetWindowText(strptVnum);
	int Vnum = atoi(CT2A(strptVnum));
	if (Vnum >= 2)
		m_PTRebarData.ptVNum = atoi(CT2A(strptVnum));
	else
	{
		mdlDialog_dmsgsPrint(L"纵向数量至少为2");
		CString strVnum;
		Vnum = 2;
		strVnum.Format(_T("%d"), Vnum);
		m_edit_VPtNum.SetWindowTextW(strVnum);
		m_PTRebarData.ptVNum = Vnum;

	}
}


//void BaseRebarDlg::OnEnChangeEditTienum()
//{
//	// TODO:  如果该控件是 RICHEDIT 控件，它将不
//	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
//	// 函数并调用 CRichEditCtrl().SetEventMask()，
//	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。
//
//	// TODO:  在此添加控件通知处理程序代码
//	/*CString strTienum;
//	m_edit_TieNum.GetWindowText(strTienum);
//	m_PTRebarData.tieNum = atoi(CT2A(strTienum));*/
//}


void BaseRebarDlg::OnCbnSelchangeComboDiameter()
{
	// TODO: 在此添加控件通知处理程序代码
	auto it = g_listRebarSize.begin();
	advance(it, m_Cob_PtDiameter.GetCurSel());
	strcpy(m_PTRebarData.ptrebarSize, CT2A(*it));
}


void BaseRebarDlg::OnCbnSelchangeComboGrade()
{
	// TODO: 在此添加控件通知处理程序代码
	auto it = g_listRebarType.begin();
	advance(it, m_Cob_PtGrade.GetCurSel());
	m_PTRebarData.ptrebarType = m_Cob_PtGrade.GetCurSel();
}


//void BaseRebarDlg::OnCbnSelchangeComboTiediameter()
//{
//	// TODO: 在此添加控件通知处理程序代码
//	/*auto it = g_listRebarSize.begin();
//	advance(it, m_Cob_TieDiameter.GetCurSel());
//	strcpy(m_PTRebarData.tierebarSize, CT2A(*it));*/
//}


//void BaseRebarDlg::OnCbnSelchangeComboTiegrade()
//{
//	// TODO: 在此添加控件通知处理程序代码
//	/*auto it = g_listRebarType.begin();
//	advance(it, m_Cob_TieGrade.GetCurSel());
//	m_PTRebarData.tierebarType = m_Cob_TieGrade.GetCurSel();*/
//}
