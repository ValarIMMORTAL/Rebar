// CBeamHoopRebarDlg.cpp: 实现文件
//
#include "_USTATION.h"
#include "CBeamHoopRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "ElementAttribute.h"


// CBeamHoopRebarDlg 对话框

IMPLEMENT_DYNAMIC(CBeamHoopRebarDlg, CDialogEx)

CBeamHoopRebarDlg::CBeamHoopRebarDlg(ElementHandle ehSel, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_BeamHoopRebar, pParent)
{
	m_ehSel = ehSel;
}


CBeamHoopRebarDlg::~CBeamHoopRebarDlg()
{
}

void CBeamHoopRebarDlg::SetRebarSizeAndType(CComboBox& CombRebarSizeCtl, CComboBox& CombRebarTypeCtl, char* pRebarSize, int nRebarType)
{
	for (auto var : g_listRebarSize)
	{
		CombRebarSizeCtl.AddString(var);
	}
	m_CombRebarSize = pRebarSize;

	for (auto var : g_listRebarType)
	{
		CombRebarTypeCtl.AddString(var);
	}
	ListCtrlEx::CStrList strlist = g_listRebarType;
	auto it = strlist.begin();
	advance(it, nRebarType);
	m_CombRebarType = *it;
}

void CBeamHoopRebarDlg::SetDefaultData(BeamRebarInfo::BeamCommHoop& m_stBeamCommHoop, BeamRebarInfo::BeamRebarHoop& m_stBeamRebarHoop)
{
	m_stBeamCommHoop.dSpacing = 0.00; // 间距
	m_stBeamCommHoop.rebarType = 0; // 钢筋类型
	m_stBeamCommHoop.dStartPos = 0.00; // 始端
	m_stBeamCommHoop.dEnd_N_Deep = 0.00; // 通过N*深度
	m_stBeamCommHoop.nPostion = 0; // 位置
	m_stBeamCommHoop.dEndPos = 0.00; // 终端
	m_stBeamCommHoop.dStart_N_Deep = 0.00; // 通过N*深度
	m_stBeamRebarHoop.dOffset = 0.00;
	m_stBeamRebarHoop.dStartRotate = 0.00;
	m_stBeamRebarHoop.dEndRotate = 0.00;
	m_stBeamRebarHoop.nStartEndType = 0; // 起点端部类型
	m_stBeamRebarHoop.nFnishEndType = 0; // 起点端部类型

	memset(m_stBeamCommHoop.label, 0, sizeof(m_stBeamCommHoop.label));	// 标签
	memset(m_stBeamRebarHoop.label, 0, sizeof(m_stBeamRebarHoop.label));	// 标签
	strncpy(m_stBeamCommHoop.rebarSize, "12mm", sizeof("12mm"));		// 钢筋尺寸
}

void CBeamHoopRebarDlg::SetBeamHoopData(ElementId condit)
{
	SetElementXAttribute(condit, m_vecBeamCommHoop, vecBeamCommHoopXAttribute, ACTIVEMODEL);
	SetElementXAttribute(condit, m_vecBeamRebarHoop, vecBeamRebarHoopXAttribute, ACTIVEMODEL);
}

BOOL CBeamHoopRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ElementId testid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	GetElementXAttribute(testid, m_vecBeamCommHoop, vecBeamCommHoopXAttribute, ACTIVEMODEL);
	GetElementXAttribute(testid, m_vecBeamRebarHoop, vecBeamRebarHoopXAttribute, ACTIVEMODEL);

	SetDefaultData();

	CString sTemp = CString();
	sTemp.Format(_T("%s"), m_vecBeamCommHoop[0].label);
	m_EditLabel = sTemp;

	m_EditSpacing = m_vecBeamCommHoop[0].dSpacing;

	m_EditStartPos = m_vecBeamCommHoop[0].dStartPos;

	m_EditStart_N_Deep = m_vecBeamCommHoop[0].dStart_N_Deep;

	m_EditEnd_N_Deep = m_vecBeamCommHoop[0].dEnd_N_Deep;

	m_EditEndPos = m_vecBeamCommHoop[0].dEndPos;
		
	SetRebarSizeAndType(m_CombRebarSizeCtl, m_CombRebarTypeCtl, m_vecBeamCommHoop[0].rebarSize, m_vecBeamCommHoop[0].rebarType);

	int nIndex = 0;
	for (auto var : g_listRebarPosition)
	{
		if (nIndex == m_vecBeamCommHoop[0].nPostion)
		{
			m_CombPostion = var;
		}
		nIndex++;
		m_CombPostionCtl.AddString(var);
	}

	m_EditLabel2 = m_vecBeamRebarHoop[0].label;

	m_EditOffset = m_vecBeamRebarHoop[0].dOffset;

	m_EditStartRotate.Format(_T("%.f°"), m_vecBeamRebarHoop[0].dStartRotate);

	m_EditEndRotate.Format(_T("%.f°"), m_vecBeamRebarHoop[0].dEndRotate);

	m_EditStartRotateCtl.EnableWindow(FALSE);
	m_EditEndRotateCtl.EnableWindow(FALSE);

	nIndex = 0;
	for (auto var : g_listEndType)
	{
		if (nIndex == m_vecBeamRebarHoop[0].nStartEndType)
		{
			m_CombStartEndType = var;
		}
		nIndex++;
		m_CombStartEndTypeCtl.AddString(var);
	}

	nIndex = 0;
	for (auto var : g_listEndType)
	{
		if (nIndex == m_vecBeamRebarHoop[0].nFnishEndType)
		{
			m_CombFnishEndType = var;
		}
		nIndex++;
		m_CombFnishEndTypeCtl.AddString(var);
	}

	LONG lStyle;
	lStyle = GetWindowLong(m_ListBeamCommHoop.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_ListBeamCommHoop.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_ListBeamCommHoop.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_ListBeamCommHoop.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);


	//在列表控件中插入列
	m_ListBeamCommHoop.InsertColumn(0, _T("标签"), 100, ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_ListBeamCommHoop.InsertColumn(1, _T("钢筋名称"), 100, ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamCommHoop.InsertColumn(2, _T("间距"), 100, ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamCommHoop.InsertColumn(3, _T("终端"), 100, ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamCommHoop.InsertColumn(4, _T("始端"), 100, ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamCommHoop.InsertColumn(5, _T("钢筋尺寸"), 0, ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamCommHoop.InsertColumn(6, _T("通过N*深度"), 0, ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamCommHoop.InsertColumn(7, _T("通过N*深度"), 0, ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamCommHoop.InsertColumn(8, _T("位置"), 0, ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamCommHoop.SetShowProgressPercent(TRUE);
	m_ListBeamCommHoop.SetSupportSort(TRUE);

	UpdateCommList();

	SetButtonFontSize();
	UpdateData(FALSE);

	m_nCurCommLine = 0;

	return TRUE;
}

void CBeamHoopRebarDlg::SetDefaultData()
{
	if (m_vecBeamCommHoop.size() == 0)
	{
		for (size_t i = 0; i < 1; i++)
		{
			BeamRebarInfo::BeamCommHoop stBeamCommHoop;
			memset(stBeamCommHoop.label, 0, sizeof(stBeamCommHoop.label));
			stBeamCommHoop.dSpacing = 0.00;
			memset(stBeamCommHoop.rebarSize, 0, sizeof(stBeamCommHoop.rebarSize));
			strncpy(stBeamCommHoop.rebarSize, "12mm", sizeof("12mm") - 1);
			stBeamCommHoop.rebarType = 0;
			stBeamCommHoop.dStartPos = 0.00;
			stBeamCommHoop.dEnd_N_Deep = 0.00;
			stBeamCommHoop.nPostion = 0;
			stBeamCommHoop.dEndPos = 0.00;
			stBeamCommHoop.dStart_N_Deep = 0.00;
			m_vecBeamCommHoop.push_back(stBeamCommHoop);
		}
	}

	if (m_vecBeamRebarHoop.size() == 0)
	{
		for (size_t i = 0; i < 1; i++)
		{
			BeamRebarInfo::BeamRebarHoop stBeamRebarHoop;
			memset(stBeamRebarHoop.label, 0, sizeof(stBeamRebarHoop.label));
			stBeamRebarHoop.dOffset = 0.00;
			stBeamRebarHoop.dStartRotate = 0.00;
			stBeamRebarHoop.dEndRotate = 0.00;
			stBeamRebarHoop.nStartEndType = 0;
			stBeamRebarHoop.nFnishEndType = 0;
			m_vecBeamRebarHoop.push_back(stBeamRebarHoop);
		}
	}
}

void CBeamHoopRebarDlg::UpdateCommList()
{
	m_ListBeamCommHoop.DeleteAllItems();
	for (int i = 0; i < m_vecBeamCommHoop.size(); i++)
	{
		m_ListBeamCommHoop.InsertItem(i, _T("")); // 插入行
		for (int j = 0; j < 5; j++)
		{
			CString strValue;
			switch (j)
			{
			case 0:
				strValue = m_vecBeamCommHoop[i].label;
				break;
			case 1:
			{
				strValue = m_vecBeamCommHoop[i].rebarSize;
				break;
			}
			case 2:
				strValue.Format(_T("%.2f"), m_vecBeamCommHoop[i].dSpacing);
				break;
			case 3:
				strValue.Format(_T("%.2f"), m_vecBeamCommHoop[i].dEndPos);
				break;
			case 4:
				strValue.Format(_T("%.2f"), m_vecBeamCommHoop[i].dStartPos);
				break;
			default:
				break;
			}
			m_ListBeamCommHoop.SetItemText(i, j, strValue);
		}
	}
}

void CBeamHoopRebarDlg::SetButtonFontSize()
{
	//设置字体大小和字形
	CFont * f;
	f = new CFont;
	f->CreateFont(36, // nHeight 
		0, // nWidth 
		0, // nEscapement 
		0, // nOrientation 
		FW_BOLD, // nWeight 
		TRUE, // bItalic 
		FALSE, // bUnderline 
		0, // cStrikeOut 
		ANSI_CHARSET, // nCharSet 
		OUT_DEFAULT_PRECIS, // nOutPrecision 
		CLIP_DEFAULT_PRECIS, // nClipPrecision 
		DEFAULT_QUALITY, // nQuality 
		DEFAULT_PITCH | FF_SWISS, // nPitchAndFamily 
		_T("+")); // lpszFac 
	GetDlgItem(IDC_BUTTON5)->SetFont(f);
	GetDlgItem(IDC_BUTTON7)->SetFont(f);
	//  :: SetTextColor(HDC hDC,RGB(255,255,0)); //设置字体颜色

	f->CreateFont(36, // nHeight 
		40, // nWidth 
		0, // nEscapement 
		0, // nOrientation 
		FW_BOLD, // nWeight 
		TRUE, // bItalic 
		FALSE, // bUnderline 
		0, // cStrikeOut 
		ANSI_CHARSET, // nCharSet 
		OUT_DEFAULT_PRECIS, // nOutPrecision 
		CLIP_DEFAULT_PRECIS, // nClipPrecision 
		DEFAULT_QUALITY, // nQuality 
		DEFAULT_PITCH | FF_SWISS, // nPitchAndFamily 
		_T("-")); // lpszFac 
	GetDlgItem(IDC_BUTTON6)->SetFont(f);
	GetDlgItem(IDC_BUTTON8)->SetFont(f);
}

void CBeamHoopRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	// 通用
	DDX_Control(pDX, IDC_EDIT1,  m_EditLabelCtl); // 标签
	DDX_Control(pDX, IDC_EDIT2,  m_EditSpacingCtl); // 间距
	DDX_Control(pDX, IDC_EDIT3,  m_EditStartPosCtl); // 始端
	DDX_Control(pDX, IDC_EDIT6,  m_EditStart_N_DeepCtl); // 通过N*深度
	DDX_Control(pDX, IDC_EDIT4,  m_EditEndPosCtl); // 终端
	DDX_Control(pDX, IDC_EDIT7,  m_EditEnd_N_DeepCtl); // 通过N*深度
	DDX_Control(pDX, IDC_COMBO1, m_CombRebarSizeCtl); // 钢筋名称
	DDX_Control(pDX, IDC_COMBO2, m_CombRebarTypeCtl); // 钢筋类型
	DDX_Control(pDX, IDC_COMBO3, m_CombPostionCtl); // 位置
	DDX_Control(pDX, IDC_EDIT5,  m_EditDescript); // 说明

	// 箍筋数据
	DDX_Control(pDX, IDC_EDIT8,  m_EditLabelCtl2); // 标签
	DDX_Control(pDX, IDC_EDIT9,  m_EditOffsetCtl); // 偏移
	DDX_Control(pDX, IDC_EDIT11, m_EditStartRotateCtl); // 起点旋转角
	DDX_Control(pDX, IDC_EDIT12, m_EditEndRotateCtl); // 终点旋转角
	DDX_Control(pDX, IDC_COMBO4, m_CombStartEndTypeCtl); // 起点端部类型
	DDX_Control(pDX, IDC_COMBO5, m_CombFnishEndTypeCtl); // 终点端部类型

	DDX_Control(pDX, IDC_BUTTON5, m_BtnCommAdd);
	DDX_Control(pDX, IDC_BUTTON6, m_BtnCommDel);
	DDX_Control(pDX, IDC_BUTTON7, m_BtnRebarAdd);
	DDX_Control(pDX, IDC_BUTTON8, m_BtnRebarDel);

	DDX_Control(pDX, IDC_LIST1, m_ListBeamCommHoop);
	DDX_Control(pDX, IDC_LIST2, m_ListBeamRebarHoop);

	DDX_Text(pDX, IDC_EDIT1, m_EditLabel);
	DDX_Text(pDX, IDC_EDIT2, m_EditSpacing);
	DDX_Text(pDX, IDC_EDIT3, m_EditStartPos);
	DDX_Text(pDX, IDC_EDIT6, m_EditStart_N_Deep);
	DDX_Text(pDX, IDC_EDIT4, m_EditEndPos);
	DDX_Text(pDX, IDC_EDIT7, m_EditEnd_N_Deep);
	DDX_Text(pDX, IDC_COMBO1, m_CombRebarSize);
	DDX_Text(pDX, IDC_COMBO2, m_CombRebarType);
	DDX_Text(pDX, IDC_COMBO3, m_CombPostion);
	DDX_Text(pDX, IDC_EDIT8, m_EditLabel2);
	DDX_Text(pDX, IDC_EDIT9, m_EditOffset);
	DDX_Text(pDX, IDC_EDIT11, m_EditStartRotate);
	DDX_Text(pDX, IDC_EDIT12, m_EditEndRotate);
	DDX_Text(pDX, IDC_COMBO4, m_CombStartEndType);
	DDX_Text(pDX, IDC_COMBO5, m_CombFnishEndType);
}


BEGIN_MESSAGE_MAP(CBeamHoopRebarDlg, CDialogEx)
	ON_EN_CHANGE(IDC_EDIT1, &CBeamHoopRebarDlg::OnEnChangeEdit1)
	ON_EN_CHANGE(IDC_EDIT2, &CBeamHoopRebarDlg::OnEnChangeEdit2)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CBeamHoopRebarDlg::OnCbnSelchangeCombo1)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CBeamHoopRebarDlg::OnCbnSelchangeCombo2)
	ON_EN_CHANGE(IDC_EDIT3, &CBeamHoopRebarDlg::OnEnChangeEdit3)
	ON_EN_CHANGE(IDC_EDIT6, &CBeamHoopRebarDlg::OnEnChangeEdit6)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CBeamHoopRebarDlg::OnCbnSelchangeCombo3)
	ON_EN_CHANGE(IDC_EDIT4, &CBeamHoopRebarDlg::OnEnChangeEdit4)
	ON_EN_CHANGE(IDC_EDIT7, &CBeamHoopRebarDlg::OnEnChangeEdit7)
	ON_EN_CHANGE(IDC_EDIT8, &CBeamHoopRebarDlg::OnEnChangeEdit8)
	ON_EN_CHANGE(IDC_EDIT9, &CBeamHoopRebarDlg::OnEnChangeEdit9)
	ON_CBN_SELCHANGE(IDC_COMBO4, &CBeamHoopRebarDlg::OnCbnSelchangeCombo4)
	ON_CBN_SELCHANGE(IDC_COMBO5, &CBeamHoopRebarDlg::OnCbnSelchangeCombo5)
	ON_EN_CHANGE(IDC_EDIT11, &CBeamHoopRebarDlg::OnEnChangeEdit11)
	ON_EN_CHANGE(IDC_EDIT12, &CBeamHoopRebarDlg::OnEnChangeEdit12)
	ON_BN_CLICKED(IDC_BUTTON2, &CBeamHoopRebarDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON4, &CBeamHoopRebarDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CBeamHoopRebarDlg::OnBnClickedButton5)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CBeamHoopRebarDlg::OnLvnItemchangedList1)
	ON_BN_CLICKED(IDC_BUTTON6, &CBeamHoopRebarDlg::OnBnClickedButton6)
END_MESSAGE_MAP()


// CBeamHoopRebarDlg 消息处理程序


void CBeamHoopRebarDlg::OnEnChangeEdit1()
{

	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	strncpy(m_vecBeamCommHoop[m_nCurCommLine].label, CT2A(m_EditLabel.GetBuffer()), m_EditLabel.GetLength() + 1);
	UpdateCommList();
}


void CBeamHoopRebarDlg::OnEnChangeEdit2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_vecBeamCommHoop[m_nCurCommLine].dSpacing = m_EditSpacing;
	UpdateCommList();
}


void CBeamHoopRebarDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strTemp = CString();
	unsigned short nIndex = m_CombRebarSizeCtl.GetCurSel();
	unsigned short i = 0;
	for (auto it : g_listRebarSize)
	{
		if (i == nIndex)
		{
			strTemp = it;
			break;
		}
		i++;
	}
	strncpy(m_vecBeamCommHoop[m_nCurCommLine].rebarSize, CT2A(strTemp.GetBuffer()), strTemp.GetLength() + 1);
	UpdateCommList();
}


void CBeamHoopRebarDlg::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
	unsigned int nIndex = m_CombRebarTypeCtl.GetCurSel();
	unsigned int i = 0;
	for (auto var : g_listRebarType)
	{
		if (i == nIndex)
		{
			m_CombRebarType = var;
			break;
		}
		i++;
	}
	m_vecBeamCommHoop[m_nCurCommLine].rebarType = nIndex;
}


void CBeamHoopRebarDlg::OnEnChangeEdit3()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_vecBeamCommHoop[m_nCurCommLine].dStartPos = m_EditStartPos;
	UpdateCommList();
}


void CBeamHoopRebarDlg::OnEnChangeEdit6()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_vecBeamCommHoop[m_nCurCommLine].dStart_N_Deep = m_EditStart_N_Deep;
}


void CBeamHoopRebarDlg::OnCbnSelchangeCombo3()
{
	// TODO: 在此添加控件通知处理程序代码
	unsigned int i = 0;
	unsigned int nIndex = m_CombPostionCtl.GetCurSel();
	for (auto var : g_listRebarPosition)
	{
		if (i == nIndex)
		{
			m_CombPostion = var;
			break;
		}
		i++;
	}
	m_vecBeamCommHoop[m_nCurCommLine].nPostion = nIndex;
}


void CBeamHoopRebarDlg::OnEnChangeEdit4()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_vecBeamCommHoop[m_nCurCommLine].dEndPos = m_EditEndPos;
	UpdateCommList();
}


void CBeamHoopRebarDlg::OnEnChangeEdit7()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_vecBeamCommHoop[m_nCurCommLine].dEnd_N_Deep = m_EditEnd_N_Deep;
}


void CBeamHoopRebarDlg::OnEnChangeEdit8()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	strncpy(m_vecBeamRebarHoop[m_nCurCommLine].label, CT2A(m_EditLabel2), m_EditLabel2.GetLength() + 1);
}


void CBeamHoopRebarDlg::OnEnChangeEdit9()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_vecBeamRebarHoop[m_nCurCommLine].dOffset = m_EditOffset;
}


void CBeamHoopRebarDlg::OnCbnSelchangeCombo4()
{
	// TODO: 在此添加控件通知处理程序代码
	int nIndex = m_CombStartEndTypeCtl.GetCurSel();
	int i = 0;
	for (auto var : g_listEndType)
	{
		if (nIndex == i)
		{
			m_CombStartEndType = var;
			break;
		}
		i++;
	}
	m_vecBeamRebarHoop[m_nCurCommLine].nStartEndType = nIndex;
}


void CBeamHoopRebarDlg::OnCbnSelchangeCombo5()
{
	// TODO: 在此添加控件通知处理程序代码
	int nIndex = m_CombFnishEndTypeCtl.GetCurSel();
	int i = 0;
	for (auto var : g_listEndType)
	{
		if (nIndex == i)
		{
			m_CombFnishEndType = var;
			break;
		}
		i++;
	}
	m_vecBeamRebarHoop[m_nCurCommLine].nFnishEndType = nIndex;
}


void CBeamHoopRebarDlg::OnEnChangeEdit11()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_vecBeamRebarHoop[m_nCurCommLine].dStartRotate = atoi(CT2A(m_EditStartRotate));
}


void CBeamHoopRebarDlg::OnEnChangeEdit12()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_EditEndRotate.Replace(_T("°"), _T(""));
	m_vecBeamRebarHoop[m_nCurCommLine].dEndRotate = atoi(CT2A(m_EditEndRotate));
}


void CBeamHoopRebarDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString s = CString();
	UpdateData(TRUE);
	s = m_EditStartRotate;
	if (s.Compare(_T("0°")) == 0)
	{
		m_EditStartRotate.Format(_T("180°"));
		m_vecBeamRebarHoop[m_nCurCommLine].dStartRotate = 180.00;
	}
	else if (s.Compare(_T("180°")) == 0)
	{
		m_EditStartRotate.Format(_T("0°"));
		m_vecBeamRebarHoop[m_nCurCommLine].dStartRotate = 0.00;
	}
	UpdateData(FALSE);
}


void CBeamHoopRebarDlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	CString s = CString();
	UpdateData(TRUE);
	s = m_EditEndRotate;
	if (s.Compare(_T("0°")) == 0)
	{
		m_EditEndRotate.Format(_T("180°"));
		m_vecBeamRebarHoop[m_nCurCommLine].dEndRotate = 180.00;
	}
	else if (s.Compare(_T("180°")) == 0)
	{
		m_EditEndRotate.Format(_T("0°"));
		m_vecBeamRebarHoop[m_nCurCommLine].dEndRotate = 0.00;
	}
	UpdateData(FALSE);
}

void CBeamHoopRebarDlg::SetCommDataEnable(bool bFlag)
{
	m_EditLabelCtl.EnableWindow(bFlag);
	m_EditSpacingCtl.EnableWindow(bFlag);
	m_CombRebarSizeCtl.EnableWindow(bFlag);
	m_CombRebarTypeCtl.EnableWindow(bFlag);
	m_EditDescript.EnableWindow(bFlag);

	m_EditEndPosCtl.EnableWindow(bFlag);
	m_EditEnd_N_DeepCtl.EnableWindow(bFlag);

	m_CombPostionCtl.EnableWindow(bFlag);

	m_EditStartPosCtl.EnableWindow(bFlag);
	m_EditStart_N_DeepCtl.EnableWindow(bFlag);
}

void CBeamHoopRebarDlg::OnBnClickedButton5()
{
	// add
	// TODO: 在此添加控件通知处理程序代码
	BeamRebarInfo::BeamCommHoop stBeamCommHoop;
	memset(stBeamCommHoop.label, 0, sizeof(stBeamCommHoop.label));
	stBeamCommHoop.dSpacing = 0.00;
	memset(stBeamCommHoop.rebarSize, 0, sizeof(stBeamCommHoop.rebarSize));
	strncpy(stBeamCommHoop.rebarSize, "12mm", sizeof("12mm"));
	stBeamCommHoop.rebarType = 0;
	stBeamCommHoop.dStartPos = 0.00;
	stBeamCommHoop.dEnd_N_Deep = 0.00;
	stBeamCommHoop.nPostion = 0;
	stBeamCommHoop.dEndPos = 0.00;
	stBeamCommHoop.dStart_N_Deep = 0.00;
	m_vecBeamCommHoop.push_back(stBeamCommHoop);
	UpdateCommList();

	if (m_vecBeamCommHoop.size() == 1)
	{
		SetCommDataEnable(TRUE);
		m_BtnCommDel.EnableWindow(TRUE);
	}
	m_nCurCommLine = (int)m_vecBeamCommHoop.size() - 1;
	UpdateCommData(stBeamCommHoop);

	BeamRebarInfo::BeamRebarHoop stBeamRebarHoop;
	memset(stBeamRebarHoop.label, 0, sizeof(stBeamRebarHoop.label));
	stBeamRebarHoop.dOffset = 0.00;
	stBeamRebarHoop.dStartRotate = 0.00;
	stBeamRebarHoop.dEndRotate = 0.00;
	stBeamRebarHoop.nStartEndType = 0;
	stBeamRebarHoop.nFnishEndType = 0;
	m_vecBeamRebarHoop.push_back(stBeamRebarHoop);
	UpdateRebarData(stBeamRebarHoop);

	UpdateData(FALSE);
}

void CBeamHoopRebarDlg::UpdateRebarData(const BeamRebarInfo::BeamRebarHoop& stBeamRebarHoop)
{
	m_EditLabel2 == stBeamRebarHoop.label;
	m_EditOffset = stBeamRebarHoop.dOffset;

	ListCtrlEx::CStrList strlist = g_listEndType;
	list<CString>::iterator it = strlist.begin();
	advance(it, stBeamRebarHoop.nStartEndType);
	m_CombStartEndType = *it;
	m_CombStartEndTypeCtl.SetCurSel(stBeamRebarHoop.nStartEndType);

	it = strlist.begin();
	advance(it, stBeamRebarHoop.nFnishEndType);
	m_CombFnishEndType = *it;
	m_CombFnishEndTypeCtl.SetCurSel(stBeamRebarHoop.nFnishEndType);

	m_EditStartRotate.Format(_T("%.f°"), stBeamRebarHoop.dStartRotate);
	m_EditEndRotate.Format(_T("%.f°"), stBeamRebarHoop.dEndRotate);
}

void CBeamHoopRebarDlg::UpdateCommData(const BeamRebarInfo::BeamCommHoop& stBeamCommHoop)
{
	m_EditLabel = stBeamCommHoop.label;
	m_EditSpacing = stBeamCommHoop.dSpacing;

	CString sTemp = CString(stBeamCommHoop.rebarSize);
	auto find = std::find(g_listRebarSize.begin(), g_listRebarSize.end(), sTemp);
	int nIndex = (int)std::distance(g_listRebarSize.begin(), find);
	m_CombRebarSizeCtl.SetCurSel(nIndex);
	m_CombRebarSize = sTemp;

	m_CombRebarTypeCtl.SetCurSel(stBeamCommHoop.rebarType);
	ListCtrlEx::CStrList strlist = g_listRebarType;
	auto it = strlist.begin();
	advance(it, stBeamCommHoop.rebarType);
	m_CombRebarType = *it;

	m_CombPostionCtl.SetCurSel(stBeamCommHoop.nPostion);
	ListCtrlEx::CStrList strlist_Pos = g_listRebarPosition;
	auto it_er = strlist_Pos.begin();
	advance(it_er, stBeamCommHoop.nPostion);
	m_CombPostion = *it_er;

	m_EditStartPos = stBeamCommHoop.dStartPos;
	m_EditEndPos = stBeamCommHoop.dEndPos;
	m_EditStart_N_Deep = stBeamCommHoop.dStart_N_Deep;
	m_EditEnd_N_Deep = stBeamCommHoop.dEnd_N_Deep;
}


void CBeamHoopRebarDlg::OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	if (pNMListView->uChanged == LVIF_STATE)
	{
		if (pNMListView->uNewState)
		{
			m_nCurCommLine = pNMListView->iItem; // 选中的是哪一行

			m_ListBeamCommHoop.SetItemState(m_nCurCommLine, LVIS_SELECTED, LVIS_SELECTED);

			//要进行的操作
			// m_ListBeamCommHoop.GetAllRebarData(m_vecBeamCommHoop);

			UpdateCommData(m_vecBeamCommHoop.at(m_nCurCommLine));
			UpdateRebarData(m_vecBeamRebarHoop.at(m_nCurCommLine));
			UpdateData(FALSE);
		}
	}

	*pResult = 0;
}


void CBeamHoopRebarDlg::OnBnClickedButton6()
{
	// delete
	// TODO: 在此添加控件通知处理程序代码
	auto it = m_vecBeamCommHoop.begin();
	auto it_rebar = m_vecBeamRebarHoop.begin();
	for (int i = 0; i < m_nCurCommLine; i++)
	{
		it++;
		it_rebar++;
	}
	m_vecBeamCommHoop.erase(it);
	m_vecBeamRebarHoop.erase(it_rebar);

	UpdateCommList();

	m_nCurCommLine = (int)m_vecBeamCommHoop.size() - 1;
	if (m_vecBeamCommHoop.size() > 0)
	{
		UpdateCommData(m_vecBeamCommHoop.at(m_vecBeamCommHoop.size() - 1));
		UpdateRebarData(m_vecBeamRebarHoop.at(m_vecBeamRebarHoop.size() - 1));

		UpdateData(FALSE);
	}
	else
	{
		m_EditLabel = _T("");
		m_EditSpacing = 0.00;
		m_CombRebarSize = _T("");
		m_CombRebarType = _T("");
		m_EditStartPos = 0.00;
		m_EditEnd_N_Deep = 0.00;
		m_CombPostion = _T("");
		m_EditEndPos = 0.00;
		m_EditStart_N_Deep = 0.00;
		UpdateData(false);

		SetCommDataEnable(FALSE);
		m_BtnCommDel.EnableWindow(FALSE);
	}

}
