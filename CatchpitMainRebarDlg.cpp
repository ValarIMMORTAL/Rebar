﻿// CatchpitMainRebarDlg.cpp: 实现文件
//


#include "CatchpitMainRebarDlg.h"
#include "afxdialogex.h"
#include "_USTATION.h"
#include "resource.h"
#include "CommonFile.h"

#include "ConstantsDef.h"
#include "ElementAttribute.h"
#include "PITBimMSCEConvert.h"
#include "PITRebarCurve.h"

// CatchpitMainRebarDlg 对话框
CatchpitMainRebarDlg * g_CatchpitMainDlg = nullptr;

IMPLEMENT_DYNAMIC(CatchpitMainRebarDlg, CDialogEx)

CatchpitMainRebarDlg::CatchpitMainRebarDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_Catchpit_MainRebar, pParent)
{
	m_Concrete.postiveCover = 50;
	m_Concrete.sideCover = 50;
	m_Concrete.rebarLevelNum = 2;
	m_Concrete.isHandleHole = 0;
	m_Concrete.MissHoleSize = g_wallRebarInfo.concrete.MissHoleSize;
}

CatchpitMainRebarDlg::~CatchpitMainRebarDlg()
{
	if (nullptr != g_CatchpitMainDlg)
	{
		g_CatchpitMainDlg = nullptr;
		free(g_CatchpitMainDlg);
	}
}

BOOL CatchpitMainRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	g_CatchpitMainDlg = this;
	// TODO:  在此添加额外的初始化
	InitUIData();


	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void CatchpitMainRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_EditPositive);
	DDX_Control(pDX, IDC_EDIT2, m_EditSide);
	DDX_Control(pDX, IDC_HOLE_CHECK, m_hole_check);
	DDX_Control(pDX, IDC_MHOLESIZE_EDIT, m_mholesize_edit);
	DDX_Control(pDX, IDC_EDIT4, m_EditLevel);
	DDX_Control(pDX, IDC_LIST1, m_listMainRebar);
}

void CatchpitMainRebarDlg::OnBnClickedUpdatadlg()
{
	// 1.设置保护层等数据
	PIT::DlgData main_dlg = m_RebarTemplate.m_Get_dlgData;
	CString str_postive;
	str_postive.Format(L"%.2f", main_dlg.postiveCover);
	m_EditPositive.SetWindowTextW(str_postive);
	//g_wallRebarInfo.concrete.postiveCover = main_dlg.postiveCover;

	CString str_side;
	str_side.Format(L"%.2f", main_dlg.sideCover);
	m_EditSide.SetWindowTextW(str_side);
	//g_wallRebarInfo.concrete.sideCover = main_dlg.sideCover;

	/*CString str_reverse;
	str_reverse.Format(L"%.2f", main_dlg.reverseCover);
	m_EditReverse.SetWindowTextW(str_reverse);
	g_wallRebarInfo.concrete.reverseCover = main_dlg.reverseCover;*/

	CString str_holesize;
	str_holesize.Format(L"%.2f", main_dlg.missHoleSize);
	m_mholesize_edit.SetWindowTextW(str_holesize);
	//g_wallRebarInfo.concrete.MissHoleSize = main_dlg.missHoleSize;

	CString str_levelnum;
	str_levelnum.Format(L"%d", main_dlg.rebarLevelNum);
	m_EditLevel.SetWindowTextW(str_levelnum);
	m_Concrete.rebarLevelNum = main_dlg.rebarLevelNum;
	//g_wallRebarInfo.concrete.rebarLevelNum = main_dlg.rebarLevelNum;



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
				ListCtrlEx::CStrList strlist = g_listRebarPose;
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
			//case 9:
			//{
			//	std::list<CString> Rebar_LineStyle;
			//	for (int i = 0; i < 8; ++i)
			//	{
			//		CString tmpCStr;
			//		tmpCStr.Format(_T("%d"), i);
			//		Rebar_LineStyle.push_back(tmpCStr);
			//	}
			//	m_listMainRebar.SetCellStringList(i, j, Rebar_LineStyle);
			//	auto it = Rebar_LineStyle.begin();
			//	advance(it, main_listdata[i].rebarLineStyle);
			//	strValue = *it;
			//	//strValue = _T("0");
			//}
			//break;
			//case 10:
			//{
			//	std::list<CString> Rebar_Weight;
			//	for (int i = 0; i < 32; ++i)
			//	{
			//		CString tmpCStr;
			//		tmpCStr.Format(_T("%d"), i);
			//		Rebar_Weight.push_back(tmpCStr);
			//	}
			//	m_listMainRebar.SetCellStringList(i, j, Rebar_Weight);
			//	auto it = Rebar_Weight.begin();
			//	advance(it, main_listdata[i].rebarWeight);
			//	strValue = *it;
			//	//strValue = _T("0");
			//}
			//break;
			default:
				break;
			}
			m_listMainRebar.SetItemText(i, j, strValue);
		}

	}
	g_vecRebarData = main_listdata;
	main_listdata.clear();
}

void CatchpitMainRebarDlg::Save_templateName()
{
	m_templateName = m_RebarTemplate.Get_templateName();
}

void CatchpitMainRebarDlg::SetListDefaultData()
{
	if (m_Concrete.rebarLevelNum - (int)m_vecRebarData.size() != 0)
	{
		m_vecRebarData.clear();//添加了墙后位置后，添加此代码
	}
	if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
	{
		if (m_Concrete.rebarLevelNum > 0)
		{
			int midpos = m_Concrete.rebarLevelNum / 2;
			PIT::ConcreteRebar oneRebarData;
			for (int i = 0; i < m_Concrete.rebarLevelNum; ++i)
			{
				if (m_Concrete.rebarLevelNum == 2)
				{
					int dir = (i) & 0x01;
					oneRebarData = { i + 1,dir,"12",2,150,0,0,0.0 };
					oneRebarData.datachange = 0;

					m_vecRebarData.push_back(oneRebarData);
					continue;
				}

				if (i < midpos)//前半部分
				{
					int dir = (i) & 0x01;
					double levelSpace;
					levelSpace = 0;
					oneRebarData = { i + 1,dir,"12",2,150,0,0,levelSpace };
					oneRebarData.datachange = 0;
				}
				else//后半部分
				{
					int dir = (i + 1) & 0x01;
					double levelSpace;
					levelSpace = 0;
					oneRebarData = { i + 1,dir,"12",2,150,0,0,levelSpace };
					oneRebarData.datachange = 0;
					//oneRebarData.rebarLevel = m_Concrete.rebarLevelNum - i;
					oneRebarData.datachange = 2;
				}
				m_vecRebarData.push_back(oneRebarData);
			}
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = m_Concrete.rebarLevelNum - (int)m_vecRebarData.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				int dir = (i + 1) & 0x01;
				double levelSpace;
				levelSpace = dir * 2000.0;
				PIT::ConcreteRebar oneRebarData = { i,dir,"12",2,150,0,0,levelSpace };
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

void CatchpitMainRebarDlg::UpdateRebarList()
{
	m_listMainRebar.DeleteAllItems();
	SetListDefaultData();
	for (int i = 0; i < m_Concrete.rebarLevelNum; ++i)
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
			case 1:
			{
				ListCtrlEx::CStrList strlist = g_listRebarDir;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecRebarData[i].rebarDir);
				strValue = *it;
			}
			break;
			case 2:
			{
				ListCtrlEx::CStrList strlist = g_listRebarSize;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				strValue = m_vecRebarData[i].rebarSize;
				strValue += _T("mm");
			}
			break;
			case 3:
			{
				ListCtrlEx::CStrList strlist = g_listRebarType;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecRebarData[i].rebarType);
				strValue = *it;
			}
			break;
			case 4:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].spacing);
				break;
			case 5:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].startOffset);
				break;
			case 6:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].endOffset);
				break;
			case 7:
				strValue.Format(_T("%.2f"), m_vecRebarData[i].levelSpace);
				break;
			case 8:
			{
				ListCtrlEx::CStrList strlist = g_listRebarPose;
				m_listMainRebar.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				if (m_vecRebarData[i].datachange > 2)
				{
					m_vecRebarData[i].datachange = 0;
				}
				advance(it, m_vecRebarData[i].datachange);
				strValue = *it;
			}
			break;
			default:
				break;
			}
			m_listMainRebar.SetItemText(i, j, strValue);
		}

	}
}

PIT::Concrete CatchpitMainRebarDlg::GetConcreteData()
{
	CString strPostiveCover;
	m_EditPositive.GetWindowText(strPostiveCover);
	m_Concrete.postiveCover = atof(CT2A(strPostiveCover));

	CString strSideCover;
	m_EditSide.GetWindowText(strSideCover);
	m_Concrete.sideCover = atof(CT2A(strSideCover));
	return m_Concrete;
}

void CatchpitMainRebarDlg::SetSelectfaces(bvector<ISubEntityPtr>& selectfaces)
{
	m_selectfaces = selectfaces;
}

void CatchpitMainRebarDlg::InitUIData()
{
	CString strLevel, strProCover, strSideCover, strMissHoleSize;
	strLevel.Format(_T("%d"), m_Concrete.rebarLevelNum);
	strProCover.Format(_T("%.2f"), m_Concrete.postiveCover);
	strSideCover.Format(_T("%.2f"), m_Concrete.sideCover);
	strMissHoleSize.Format(_T("%.2f"), m_Concrete.MissHoleSize);
	if (m_Concrete.isHandleHole)
	{
		m_hole_check.SetCheck(true);
		m_mholesize_edit.SetReadOnly(FALSE);
	}
	else
	{
		m_hole_check.SetCheck(false);
		m_mholesize_edit.SetReadOnly(TRUE);
	}
	/*m_EditPositive.SetWindowText(strProCover);
	m_EditSide.SetWindowText(strSideCover);
	m_EditLevel.SetWindowText(strLevel);
	m_mholesize_edit.SetWindowText(strMissHoleSize);*/



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
	m_listMainRebar.InsertColumn(0, _T("层"), (int)(width / 8.0*0.75), ListCtrlEx::EditBox, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_listMainRebar.InsertColumn(1, _T("方向"), (int)(width / 8.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(2, _T("直径"), (int)(width / 8.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(3, _T("类型"), (int)(width / 8.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(4, _T("间距"), (int)(width / 8.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(5, _T("起点偏移"), (int)(width / 8.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(6, _T("终点偏移"), (int)(width / 8.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listMainRebar.InsertColumn(7, _T("与前层间距"), (int)(width / 8.0 * 1.2), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	//m_listMainRebar.InsertColumn(8, _T("位置"), (int)(width / 9.0*0.75), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_EditPositive.SetWindowText(strProCover);
	m_EditSide.SetWindowText(strSideCover);
	m_EditLevel.SetWindowText(strLevel);
	m_mholesize_edit.SetWindowText(strMissHoleSize);

	UpdateRebarList();
}


BEGIN_MESSAGE_MAP(CatchpitMainRebarDlg, CDialogEx)
	ON_EN_CHANGE(IDC_EDIT1, &CatchpitMainRebarDlg::OnEnChangeEdit1)
	ON_EN_CHANGE(IDC_EDIT2, &CatchpitMainRebarDlg::OnEnChangeEdit2)
	ON_BN_CLICKED(IDC_HOLE_CHECK, &CatchpitMainRebarDlg::OnBnClickedHoleCheck)
	ON_EN_CHANGE(IDC_MHOLESIZE_EDIT, &CatchpitMainRebarDlg::OnEnChangeMholesizeEdit)
	ON_EN_CHANGE(IDC_EDIT4, &CatchpitMainRebarDlg::OnEnChangeEdit4)
	ON_BN_CLICKED(IDC_RebarTemplate, &CatchpitMainRebarDlg::OnBnClickedRebartemplate)
END_MESSAGE_MAP()


// CatchpitMainRebarDlg 消息处理程序


void CatchpitMainRebarDlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。
	CString strPostiveCover;
	m_EditPositive.GetWindowText(strPostiveCover);
	m_Concrete.postiveCover = atof(CT2A(strPostiveCover));
	// TODO:  在此添加控件通知处理程序代码
}


void CatchpitMainRebarDlg::OnEnChangeEdit2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。
	CString strSideCover;
	m_EditSide.GetWindowText(strSideCover);
	m_Concrete.sideCover = atof(CT2A(strSideCover));
	// TODO:  在此添加控件通知处理程序代码
}


void CatchpitMainRebarDlg::OnBnClickedHoleCheck()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_hole_check.GetCheck())
	{
		m_mholesize_edit.SetReadOnly(FALSE);
		m_Concrete.isHandleHole = 1;
	}
	else
	{
		m_mholesize_edit.SetReadOnly(TRUE);
		m_Concrete.isHandleHole = 0;
	}
}


void CatchpitMainRebarDlg::OnEnChangeMholesizeEdit()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。
	CString strMissHoleSize;
	m_mholesize_edit.GetWindowText(strMissHoleSize);
	m_Concrete.MissHoleSize = atof(CT2A(strMissHoleSize));
	//g_wallRebarInfo.concrete.MissHoleSize = atof(CT2A(strMissHoleSize));
	// TODO:  在此添加控件通知处理程序代码
}


void CatchpitMainRebarDlg::OnEnChangeEdit4()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。
	CString strLevelNum;
	m_EditLevel.GetWindowText(strLevelNum);
	m_Concrete.rebarLevelNum = atoi(CT2A(strLevelNum));
	UpdateRebarList();
	// TODO:  在此添加控件通知处理程序代码
}


void CatchpitMainRebarDlg::OnBnClickedRebartemplate()
{
	// TODO: 在此添加控件通知处理程序代码
	// 1、获取当前界面保护层等数据
	CString strpostive;
	GetDlgItemText(IDC_EDIT1, strpostive);
	CString strside;
	GetDlgItemText(IDC_EDIT2, strside);
	CString strholesize;
	GetDlgItemText(IDC_MHOLESIZE_EDIT, strholesize);
	CString strlevelnum;
	GetDlgItemText(IDC_EDIT4, strlevelnum);

	// 2、将其数据保存到参数模板类
	m_dlgData.postiveCover = _ttof(strpostive);//正面
	m_dlgData.sideCover = _ttof(strside);//侧面
	m_dlgData.missHoleSize = _ttof(strholesize);//忽略尺寸
	m_dlgData.rebarLevelNum = _ttoi(strlevelnum);//钢筋层数
	m_RebarTemplate.Set_m_dlgData(m_dlgData);

	// 3、将表格数据保存到参数模板类
	std::vector<PIT::ConcreteRebar> DlgvecRebarData;
	this->m_listMainRebar.GetAllRebarData(DlgvecRebarData);
	//this->GetAllRebarData(DlgvecRebarData);
	m_RebarTemplate.Set_m_vecRebarData(DlgvecRebarData);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_RebarTemplate.Set_isFace();
	m_RebarTemplate.Create(IDD_DIALOG_RebarTemplate);
	//m_RebarTemplate.set_dlg(this);
	m_RebarTemplate.SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);//使窗口总是在最前面
	//m_RebarTemplate.DoModal();
	m_RebarTemplate.ShowWindow(SW_SHOW);
}
