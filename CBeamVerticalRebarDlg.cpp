// CBeamVerticalRebarDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "CBeamVerticalRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "ElementAttribute.h"


// CBeamVerticalRebarDlg 对话框

class CBeamVerticalRebarDlg;
IMPLEMENT_DYNAMIC(CBeamVerticalRebarDlg, CDialogEx)

CBeamVerticalRebarDlg::CBeamVerticalRebarDlg(ElementHandle ehSel, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_BeamVerticalRebar, pParent)
{
	m_ehSel = ehSel;
	m_nCurAreaLine = 0;
	m_bChange = true;

	m_arrPosition[0] = _T("左侧");
	m_arrPosition[1] = _T("右侧");
	m_arrPosition[2] = _T("连续");
}

CBeamVerticalRebarDlg::~CBeamVerticalRebarDlg()
{
}

void CBeamVerticalRebarDlg::SetRebarSizeAndType(CComboBox& CombRebarSizeCtl, CComboBox& CombRebarTypeCtl, char* pRebarSize, int nRebarType)
{
	for (auto var : g_listRebarSize)
	{
		CombRebarSizeCtl.AddString(var);
	}
	int nIndex = 0;
	CString sTemp = CString(pRebarSize);
	for (auto var : g_listRebarSize)
	{
		if (sTemp.Compare(var) == 0)
		{
			m_CombRebarSize = var;
			break;
		}
		nIndex++;
	}
	nIndex = 0;
	for (auto var : g_listRebarType)
	{
		CombRebarTypeCtl.AddString(var);
		if (nIndex == nRebarType)
		{
			m_CombRebarType = var;
			break;
		}
		nIndex++;
	}
}

void CBeamVerticalRebarDlg::SetDefaultData(vector<BeamRebarInfo::BeamAreaVertical>& vecBeamAreaData, vector<BeamRebarInfo::BeamRebarVertical>& vecBeamRebarData)
{
	if (vecBeamAreaData.size() == 0)
	{
		for (unsigned short i = 0; i < 3; i++)
		{
			BeamRebarInfo::BeamAreaVertical stBeamAreaData;
			char arrTemp[3] = {""};
			arrTemp[0] = 'T';
			arrTemp[1] = i + 49;
			strncpy(stBeamAreaData.label, arrTemp, sizeof(arrTemp));

			stBeamAreaData.dSpace = 0.00;
			stBeamAreaData.dStartOffset = 0.00;
			stBeamAreaData.nTotNum = 5;
			stBeamAreaData.dEndOffset = 0.00;
			stBeamAreaData.nPosition = 0;
			vecBeamAreaData.push_back(stBeamAreaData);
		}
	}

	if (vecBeamRebarData.size() == 0)
	{
		for (unsigned short i = 0; i < 3; i++)
		{
			BeamRebarInfo::BeamRebarVertical stBeamRebarVertical;

			strncpy(stBeamRebarVertical.label, "T1", sizeof("T1"));
			stBeamRebarVertical.dLeftOffset = 0.00;
			stBeamRebarVertical.dRightOffset = 0.00;
			stBeamRebarVertical.dLeftRotateAngle = 0.00;
			stBeamRebarVertical.dRightRotateAngle = 0.00;
			stBeamRebarVertical.nPosition = 0;
			stBeamRebarVertical.nTotNum = 0;
			stBeamRebarVertical.nLeftEndStyle = 0;
			stBeamRebarVertical.nRightEndStyle = 0;
			strncpy(stBeamRebarVertical.rebarSize, "12mm", sizeof("12mm"));
			stBeamRebarVertical.rebarType = 0;

			vecBeamRebarData.push_back(stBeamRebarVertical);
		}
	}
}

void CBeamVerticalRebarDlg::SetBeamVerticalData(ElementId condit)
{
	SetElementXAttribute(condit, m_vecBeamAreaData, vecBeamAreaXAttribute, ACTIVEMODEL);
	SetElementXAttribute(condit, m_vecBeamRebarData, vecBeamRebarXAttribute, ACTIVEMODEL);
}

BOOL CBeamVerticalRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ElementId testid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	GetElementXAttribute(testid, m_vecBeamAreaData, vecBeamAreaXAttribute, ACTIVEMODEL);
	GetElementXAttribute(testid, m_vecBeamRebarData, vecBeamRebarXAttribute, ACTIVEMODEL);

	SetDefaultData(m_vecBeamAreaData, m_vecBeamRebarData); // 设置默认参数

	CString sTemp = CString();

	m_EidtLabel = m_vecBeamAreaData[0].label;
	m_EidtSpace = m_vecBeamAreaData[0].dSpace;
	m_EidtStartOffset = m_vecBeamAreaData[0].dStartOffset;
	m_EidtTotNum = m_vecBeamAreaData[0].nTotNum;
	m_EidtEndOffset = m_vecBeamAreaData[0].dEndOffset;

	int nIndex = 0;
	for (auto it = g_listRebarPosition.begin(); it != g_listRebarPosition.end(); it++)
	{
		m_CombPositionCtl.AddString(*it);
		if (nIndex == m_vecBeamAreaData[0].nPosition)
		{
			m_CombPosition = *it;
		}
		nIndex++;
	}

	m_EidtLabel2 = m_vecBeamRebarData[0].label;

	m_EidtLeftOffset =  m_vecBeamRebarData[0].dLeftOffset;

	m_EidtRightOffset =  m_vecBeamRebarData[0].dRightOffset;

	m_EidtTotNum2 = m_vecBeamRebarData[0].nTotNum;

	m_EditLeftRotateAngle.Format(_T("%d°"), (int)m_vecBeamRebarData[0].dLeftRotateAngle);
	m_EditRightRotateAngle.Format(_T("%d°"), (int)m_vecBeamRebarData[0].dRightRotateAngle);

	m_EditLeftRotateAngleCtl.EnableWindow(FALSE);
	m_EditRightRotateAngleCtl.EnableWindow(FALSE);

	nIndex = 0;
	for (auto var : m_arrPosition)
	{
		m_CombPositionCtl2.AddString(var);
		if (nIndex == m_vecBeamRebarData[0].nPosition)
		{
			m_CombPosition2 = var;
		}
		nIndex++;
	}

	SetRebarSizeAndType(m_CombRebarSizeCtl, m_CombRebarTypeCtl, m_vecBeamRebarData[0].rebarSize, m_vecBeamRebarData[0].rebarType);

	nIndex = 0;
	for (auto var : g_listEndType)
	{
		m_CombLeftEndStyleCtl.AddString(var);
		if (nIndex == m_vecBeamRebarData[0].nLeftEndStyle)
		{
			m_CombLeftEndStyle = var;
		}
		nIndex++;
	}

	nIndex = 0;
	for (auto var : g_listEndType)
	{
		m_CombRightEndStyleCtl.AddString(var);
		if (nIndex == m_vecBeamRebarData[0].nRightEndStyle)
		{
			m_CombRightEndStyle = var;
		}
		nIndex++;
	}

	SetButtonFontSize();

	LONG lStyle;
	lStyle = GetWindowLong(m_ListBeamAreaInfo.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_ListBeamAreaInfo.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_ListBeamAreaInfo.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用于report 风格的listctrl ） 
	m_ListBeamAreaInfo.SetExtendedStyle(dwStyle);	// 设置扩展风格 
	//设置列表控件的报表显示方式
	// m_ListBeamAreaInfo.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_ListBeamAreaInfo.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_ListBeamAreaInfo.InsertColumn(0, _T("标签"), (int)(width / 6 * 0.75), ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_ListBeamAreaInfo.InsertColumn(1, _T("位置"), (int)(width / 6 * 0.75), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamAreaInfo.InsertColumn(2, _T("总数量"), (int)(width / 6 * 0.75), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamAreaInfo.InsertColumn(3, _T("间隙"), (int)(width / 6 * 0.75), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamAreaInfo.InsertColumn(4, _T("起点偏移"), (int)(width / 6 * 1.5), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_ListBeamAreaInfo.InsertColumn(5, _T("终点偏移"), (int)(width / 6 * 1.5), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
//	m_ListBeamAreaInfo.SetShowProgressPercent(TRUE);
//	m_ListBeamAreaInfo.SetSupportSort(TRUE);

	UpdateListBeamInfo();

	UpdateData(false);

	return TRUE;
}

void CBeamVerticalRebarDlg::UpdateListBeamInfo()
{
	m_ListBeamAreaInfo.DeleteAllItems();
	for (int i = 0; i < m_vecBeamAreaData.size(); i++)
	{
		m_ListBeamAreaInfo.InsertItem(i, _T("")); // 插入行
		for (int j = 0; j < 6; j++)
		{
			CString strValue;
			switch (j)
			{
			case 0:
				strValue = m_vecBeamAreaData[i].label;
				break;
			case 1:
			{
				auto it = g_listRebarPosition.begin();
				advance(it, m_vecBeamAreaData[i].nPosition);
				strValue = *it;
				break;
			}
			case 2:
				strValue.Format(_T("%d"), m_vecBeamAreaData[i].nTotNum);
				break;
			case 3:
				strValue.Format(_T("%.2f"), m_vecBeamAreaData[i].dSpace);
				break;
			case 4:
				strValue.Format(_T("%.2f"), m_vecBeamAreaData[i].dStartOffset);
				break;
			case 5:
				strValue.Format(_T("%.2f"), m_vecBeamAreaData[i].dEndOffset);
				break;
			default:
				break;
			}
			m_ListBeamAreaInfo.SetItemText(i, j, strValue);
		}
	}

//	m_ListBeamAreaInfo.SetShowProgressPercent(TRUE);
//	m_ListBeamAreaInfo.SetSupportSort(TRUE);
}


void CBeamVerticalRebarDlg::SetButtonFontSize()
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

void CBeamVerticalRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	// 区域数据
	DDX_Text(pDX, IDC_EDIT1,  m_EidtLabel);				// 标签
	DDX_Text(pDX, IDC_EDIT2,  m_EidtSpace);				// 间隙
	DDX_Text(pDX, IDC_EDIT7,  m_EidtStartOffset);		// 起点偏移
	DDX_Text(pDX, IDC_EDIT3,  m_EidtTotNum);			// 总数量
	DDX_Text(pDX, IDC_EDIT8,  m_EidtEndOffset);			// 终点偏移
	DDX_Text(pDX, IDC_COMBO1, m_CombPosition);			// 位置

	// 钢筋数据
	DDX_Text(pDX, IDC_EDIT4,  m_EidtLabel2);			// 标签
	DDX_Text(pDX, IDC_EDIT9,  m_EidtLeftOffset);		// 左端偏移
	DDX_Text(pDX, IDC_EDIT10, m_EidtRightOffset);		// 右端偏移
	DDX_Text(pDX, IDC_EDIT5,  m_EidtTotNum2);			// 总数量
	DDX_Text(pDX, IDC_EDIT11, m_EditLeftRotateAngle);	// 左旋转角
	DDX_Text(pDX, IDC_EDIT12, m_EditRightRotateAngle);	// 右旋转角
	DDX_Text(pDX, IDC_COMBO2, m_CombPosition2);			// 位置
	DDX_Text(pDX, IDC_COMBO3, m_CombRebarSize);			// 钢筋尺寸
	DDX_Text(pDX, IDC_COMBO4, m_CombRebarType);			// 钢筋类型
	DDX_Text(pDX, IDC_COMBO11, m_CombLeftEndStyle);		// 左端样式
	DDX_Text(pDX, IDC_COMBO6, m_CombRightEndStyle);		// 右端样式

	// 区域数据
	DDX_Control(pDX, IDC_EDIT1, m_EidtLabelCtl);				// 标签
	DDX_Control(pDX, IDC_EDIT2, m_EidtSpaceCtl);				// 间隙
	DDX_Control(pDX, IDC_EDIT7, m_EidtStartOffsetCtl);		// 起点偏移
	DDX_Control(pDX, IDC_EDIT3, m_EidtTotNumCtl);				// 总数量
	DDX_Control(pDX, IDC_EDIT8, m_EidtEndOffsetCtl);			// 终点偏移
	DDX_Control(pDX, IDC_COMBO1, m_CombPositionCtl);			// 位置

	// 钢筋数据
	DDX_Control(pDX, IDC_EDIT4, m_EidtLabelCtl2);				// 标签
	DDX_Control(pDX, IDC_EDIT9, m_EidtLeftOffsetCtl);			// 左端偏移
	DDX_Control(pDX, IDC_EDIT10, m_EidtRightOffsetCtl);			// 右端偏移
	DDX_Control(pDX, IDC_EDIT5, m_EidtTotNumCtl2);				// 总数量
	DDX_Control(pDX, IDC_EDIT11, m_EditLeftRotateAngleCtl);		// 左旋转角
	DDX_Control(pDX, IDC_EDIT12, m_EditRightRotateAngleCtl);	// 右旋转角
	DDX_Control(pDX, IDC_COMBO2, m_CombPositionCtl2);			// 位置
	DDX_Control(pDX, IDC_COMBO3, m_CombRebarSizeCtl);			// 钢筋尺寸
	DDX_Control(pDX, IDC_COMBO4, m_CombRebarTypeCtl);			// 钢筋类型
	DDX_Control(pDX, IDC_COMBO11, m_CombLeftEndStyleCtl);		// 左端样式
	DDX_Control(pDX, IDC_COMBO6, m_CombRightEndStyleCtl);		// 右端样式
	
	DDX_Control(pDX, IDC_BUTTON5, m_ButAreaAdd);
	DDX_Control(pDX, IDC_BUTTON6, m_ButAreaDel);
	DDX_Control(pDX, IDC_BUTTON7, m_ButRebarAdd);
	DDX_Control(pDX, IDC_BUTTON8, m_ButRebarDel);

	DDX_Control(pDX, IDC_LIST1, m_ListBeamAreaInfo);		// 箍筋数据
}


BEGIN_MESSAGE_MAP(CBeamVerticalRebarDlg, CDialogEx)
	ON_EN_CHANGE(IDC_EDIT1, &CBeamVerticalRebarDlg::OnEnChangeEdit1)
	ON_EN_CHANGE(IDC_EDIT2, &CBeamVerticalRebarDlg::OnEnChangeEdit2)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CBeamVerticalRebarDlg::OnCbnSelchangeCombo1)
	ON_EN_CHANGE(IDC_EDIT7, &CBeamVerticalRebarDlg::OnEnChangeEdit7)
	ON_EN_CHANGE(IDC_EDIT3, &CBeamVerticalRebarDlg::OnEnChangeEdit3)
	ON_EN_CHANGE(IDC_EDIT8, &CBeamVerticalRebarDlg::OnEnChangeEdit8)
	ON_EN_CHANGE(IDC_EDIT4, &CBeamVerticalRebarDlg::OnEnChangeEdit4)
	ON_EN_CHANGE(IDC_EDIT9, &CBeamVerticalRebarDlg::OnEnChangeEdit9)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CBeamVerticalRebarDlg::OnCbnSelchangeCombo2)
	ON_EN_CHANGE(IDC_EDIT10, &CBeamVerticalRebarDlg::OnEnChangeEdit10)
	ON_EN_CHANGE(IDC_EDIT5, &CBeamVerticalRebarDlg::OnEnChangeEdit5)
	ON_CBN_SELCHANGE(IDC_COMBO11, &CBeamVerticalRebarDlg::OnCbnSelchangeCombo11)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CBeamVerticalRebarDlg::OnCbnSelchangeCombo3)
	ON_CBN_SELCHANGE(IDC_COMBO4, &CBeamVerticalRebarDlg::OnCbnSelchangeCombo4)
	ON_EN_CHANGE(IDC_EDIT11, &CBeamVerticalRebarDlg::OnEnChangeEdit11)
	ON_CBN_SELCHANGE(IDC_COMBO6, &CBeamVerticalRebarDlg::OnCbnSelchangeCombo6)
	ON_EN_CHANGE(IDC_EDIT12, &CBeamVerticalRebarDlg::OnEnChangeEdit12)
	ON_BN_CLICKED(IDC_BUTTON2, &CBeamVerticalRebarDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON4, &CBeamVerticalRebarDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON6, &CBeamVerticalRebarDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON5, &CBeamVerticalRebarDlg::OnBnClickedButton5)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CBeamVerticalRebarDlg::OnLvnItemchangedList1)
	ON_BN_CLICKED(IDC_BUTTON1, &CBeamVerticalRebarDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CBeamVerticalRebarDlg 消息处理程序


void CBeamVerticalRebarDlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	strncpy(m_vecBeamAreaData[m_nCurAreaLine].label, CT2A(m_EidtLabel.GetBuffer()), m_EidtLabel.GetLength() + 1);
	UpdateListBeamInfo();
}


void CBeamVerticalRebarDlg::OnEnChangeEdit2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	m_vecBeamAreaData[m_nCurAreaLine].dSpace = m_EidtSpace;
	UpdateListBeamInfo();
}


void CBeamVerticalRebarDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	int nIndex = m_CombPositionCtl.GetCurSel();

	int i = 0;
	for (auto var : g_listRebarPosition)
	{
		if (i == nIndex)
		{
			m_CombPosition = var;
			break;
		}
		i++;
	}
	m_vecBeamAreaData[m_nCurAreaLine].nPosition = nIndex;
	
	UpdateListBeamInfo();
}


void CBeamVerticalRebarDlg::OnEnChangeEdit7()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	m_vecBeamAreaData[m_nCurAreaLine].dStartOffset = m_EidtStartOffset;
	UpdateListBeamInfo();
}


void CBeamVerticalRebarDlg::OnEnChangeEdit3()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	m_vecBeamAreaData[m_nCurAreaLine].nTotNum = m_EidtTotNum;
	UpdateListBeamInfo();
}


void CBeamVerticalRebarDlg::OnEnChangeEdit8()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	m_vecBeamAreaData[m_nCurAreaLine].dEndOffset = m_EidtEndOffset;
	UpdateListBeamInfo();
}


void CBeamVerticalRebarDlg::OnEnChangeEdit4()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	// sprintf(m_stVerticalRebar.label2, "%s", CT2A(strTemp));
	strncpy(m_vecBeamRebarData[m_nCurAreaLine].label, CT2A(m_EidtLabel2), sizeof(m_vecBeamRebarData[m_nCurAreaLine].label));
}


void CBeamVerticalRebarDlg::OnEnChangeEdit9()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	m_vecBeamRebarData[m_nCurAreaLine].dLeftOffset = m_EidtLeftOffset;
}


void CBeamVerticalRebarDlg::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
	int nIndex = m_CombPositionCtl2.GetCurSel();

	int i = 0;
	for (auto var : m_arrPosition)
	{
		if (i == nIndex)
		{
			m_CombPosition2 = var;
			break;
		}
		i++;
	}
	m_vecBeamRebarData[m_nCurAreaLine].nPosition = nIndex;
}


void CBeamVerticalRebarDlg::OnEnChangeEdit10()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	m_vecBeamRebarData[m_nCurAreaLine].dRightOffset = m_EidtRightOffset;
}


void CBeamVerticalRebarDlg::OnEnChangeEdit5()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	m_vecBeamRebarData[m_nCurAreaLine].nTotNum = m_EidtTotNum;
}


void CBeamVerticalRebarDlg::OnCbnSelchangeCombo11()
{
	// TODO: 在此添加控件通知处理程序代码
	int nIndex = m_CombLeftEndStyleCtl.GetCurSel();
	int i = 0;
	for (auto var : g_listEndType)
	{
		if (nIndex == i)
		{
			m_CombLeftEndStyle = var;
			break;
		}
		i++;
	}
	m_vecBeamRebarData[m_nCurAreaLine].nLeftEndStyle = nIndex;
}


void CBeamVerticalRebarDlg::OnCbnSelchangeCombo3()
{
	// TODO: 在此添加控件通知处理程序代码
	// UpdateData(true);
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
	strncpy(m_vecBeamRebarData[m_nCurAreaLine].rebarSize, CT2A(strTemp.GetBuffer()), strTemp.GetLength() + 1);
}


void CBeamVerticalRebarDlg::OnCbnSelchangeCombo4()
{
	// TODO: 在此添加控件通知处理程序代码
	//m_stVerticalRebar.rebarType = m_CombRebarType.GetCurSel();
}


void CBeamVerticalRebarDlg::OnEnChangeEdit11()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码

	UpdateData(true);
	m_EditLeftRotateAngle.Replace(_T("°"), _T(""));
	m_vecBeamRebarData[m_nCurAreaLine].dLeftRotateAngle = atoi(CT2A(m_EditLeftRotateAngle));
}


void CBeamVerticalRebarDlg::OnCbnSelchangeCombo6()
{
	// TODO: 在此添加控件通知处理程序代码
	int nIndex = m_CombRightEndStyleCtl.GetCurSel();
	int i = 0;
	for (auto var : g_listEndType)
	{
		if (nIndex == i)
		{
			m_CombRightEndStyle = var;
			break;
		}
		i++;
	}
	m_vecBeamRebarData[m_nCurAreaLine].nRightEndStyle = nIndex;
}


void CBeamVerticalRebarDlg::OnEnChangeEdit12()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(true);
	m_EditLeftRotateAngle.Replace(_T("°"), _T(""));
	m_vecBeamRebarData[m_nCurAreaLine].dRightRotateAngle = atoi(CT2A(m_EditLeftRotateAngle));
}


void CBeamVerticalRebarDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString s = CString();
	UpdateData(true);
	s = m_EditLeftRotateAngle;
	if (s.Compare(_T("0°")) == 0 || s.Compare(_T("0.00°")) == 0)
	{
		m_EditLeftRotateAngle.Format(_T("%d°"), 90);
		m_vecBeamRebarData[m_nCurAreaLine].dLeftRotateAngle = 90.00;
	}
	else if (s.Compare(_T("90°")) == 0 || s.Compare(_T("90.00°")) == 0)
	{
		m_EditLeftRotateAngle.Format(_T("%d°"), 180);
		m_vecBeamRebarData[m_nCurAreaLine].dLeftRotateAngle = 180.00;
	}
	else if (s.Compare(_T("180°")) == 0 || s.Compare(_T("180.00°")) == 0)
	{
		m_EditLeftRotateAngle.Format(_T("%d°"), 270);
		m_vecBeamRebarData[m_nCurAreaLine].dLeftRotateAngle = 270.00;
	}
	else
	{
		m_EditLeftRotateAngle.Format(_T("%d°"), 0);
		m_vecBeamRebarData[m_nCurAreaLine].dLeftRotateAngle = 0.00;
	}
	UpdateData(false);
}


void CBeamVerticalRebarDlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	CString s = CString();
	UpdateData(true);
	s = m_EditRightRotateAngle;
	if (s.Compare(_T("0°")) == 0 || s.Compare(_T("0.00°")) == 0)
	{
		m_EditRightRotateAngle.Format(_T("%d°"), 90);
		m_vecBeamRebarData[m_nCurAreaLine].dRightRotateAngle = 90.00;
	}
	else if (s.Compare(_T("90°")) == 0 || s.Compare(_T("90.00°")) == 0)
	{
		m_EditRightRotateAngle.Format(_T("%d°"), 180);
		m_vecBeamRebarData[m_nCurAreaLine].dRightRotateAngle = 180.00;
	}
	else if (s.Compare(_T("180°")) == 0 || s.Compare(_T("180.00°")) == 0)
	{
		m_EditRightRotateAngle.Format(_T("%d°"), 270);
		m_vecBeamRebarData[m_nCurAreaLine].dRightRotateAngle = 270.00;
	}
	else
	{
		m_EditRightRotateAngle.Format(_T("%d°"), 0);
		m_vecBeamRebarData[m_nCurAreaLine].dRightRotateAngle = 0.00;
	}
	UpdateData(false);
}

void CBeamVerticalRebarDlg::SetAreaDataEnable(BOOL bEnable)
{
	m_EidtLabelCtl.EnableWindow(bEnable);
	m_EidtSpaceCtl.EnableWindow(bEnable);
	m_EidtStartOffsetCtl.EnableWindow(bEnable);
	m_EidtTotNumCtl.EnableWindow(bEnable);
	m_EidtEndOffsetCtl.EnableWindow(bEnable);
	m_CombPositionCtl.EnableWindow(bEnable);
}


void CBeamVerticalRebarDlg::OnBnClickedButton6()
{
	// TODO: 在此添加控件通知处理程序代码
	// 减少
	int nCnt = 0;
	auto it = m_vecBeamAreaData.begin();
	auto it_rebar = m_vecBeamRebarData.begin();
	for (int i = 0; i < m_nCurAreaLine; i++)
	{
		it++;
		it_rebar++;
	}
	m_vecBeamAreaData.erase(it);
	m_vecBeamRebarData.erase(it_rebar);

	UpdateListBeamInfo();

	m_nCurAreaLine = (int)m_vecBeamAreaData.size() - 1;
	if (m_vecBeamAreaData.size() > 0)
	{
		UpdateAreaData(m_vecBeamAreaData.at(m_vecBeamAreaData.size() - 1));
		UpdateRebarData(m_vecBeamRebarData.at(m_vecBeamRebarData.size() - 1));

		UpdateData(false);
	}
	else
	{
		m_EidtLabel= _T("");
		m_EidtSpace = 0.00;
		m_EidtStartOffset = 0.00;
		m_EidtTotNum = 0;
		m_EidtEndOffset = 0.00;
		m_CombPosition = *g_listRebarPosition.begin();

	    UpdateData(false);

		SetAreaDataEnable(FALSE);
		m_ButAreaDel.EnableWindow(FALSE);
	}
}


void CBeamVerticalRebarDlg::OnBnClickedButton5()
{
	// TODO: 在此添加控件通知处理程序代码
	// 增加
	BeamRebarInfo::BeamAreaVertical stBeamAreaData;
	memset(stBeamAreaData.label, 0, sizeof(stBeamAreaData.label));
	stBeamAreaData.dSpace = 0.00;
	stBeamAreaData.dStartOffset = 0.00;
	stBeamAreaData.nTotNum = 0;
	stBeamAreaData.dEndOffset = 0.00;
	stBeamAreaData.nPosition = 0;
	m_vecBeamAreaData.push_back(stBeamAreaData);
	UpdateListBeamInfo();
	if (m_vecBeamAreaData.size() == 1)
	{
		SetAreaDataEnable(TRUE);
		m_ButAreaDel.EnableWindow(TRUE);
	}
	m_nCurAreaLine = (int)m_vecBeamAreaData.size() - 1;
	UpdateAreaData(stBeamAreaData);

	BeamRebarInfo::BeamRebarVertical stBeamRebarVertical;
	strncpy(stBeamRebarVertical.label, "T1", sizeof("T1"));
	stBeamRebarVertical.dLeftOffset = 0.00;
	stBeamRebarVertical.dRightOffset = 0.00;
	stBeamRebarVertical.dLeftRotateAngle = 0.00;
	stBeamRebarVertical.dRightRotateAngle = 0.00;
	stBeamRebarVertical.nPosition = 0;
	stBeamRebarVertical.nTotNum = 0;
	stBeamRebarVertical.nLeftEndStyle = 0;
	stBeamRebarVertical.nRightEndStyle = 0;
	strncpy(stBeamRebarVertical.rebarSize, "12mm", sizeof("12mm"));
	stBeamRebarVertical.rebarType = 0;
	UpdateRebarData(stBeamRebarVertical);

	UpdateData(false);
	m_vecBeamRebarData.push_back(stBeamRebarVertical);

}

void CBeamVerticalRebarDlg::UpdateRebarData(BeamRebarInfo::BeamRebarVertical& stBeamRebarVertical)
{
	m_EidtLabel2 = stBeamRebarVertical.label;

	m_CombPosition2 = m_arrPosition[stBeamRebarVertical.nPosition];

	m_EidtLeftOffset = stBeamRebarVertical.dLeftOffset;

	m_EidtRightOffset = stBeamRebarVertical.dRightOffset;

	m_EidtTotNum2 = stBeamRebarVertical.nTotNum;

	auto it = g_listEndType.begin();
	for (int i = 0; i < stBeamRebarVertical.nLeftEndStyle; i++)
	{
		it++;
	}
	m_CombLeftEndStyle = *it;

	it = g_listEndType.begin();
	for (int i = 0; i < stBeamRebarVertical.nRightEndStyle; i++)
	{
		it++;
	}
	m_CombRightEndStyle = *it;

	m_EditLeftRotateAngle.Format(_T("%d°"), (int)stBeamRebarVertical.dLeftRotateAngle);

	m_EditRightRotateAngle.Format(_T("%d°"), (int)stBeamRebarVertical.dRightRotateAngle);

	m_CombRebarSize = stBeamRebarVertical.rebarSize;
}

void CBeamVerticalRebarDlg::UpdateAreaData(BeamRebarInfo::BeamAreaVertical& stBeamAreaData)
{
	m_EidtLabel = stBeamAreaData.label;

	m_EidtSpace = stBeamAreaData.dSpace;

	m_EidtStartOffset = stBeamAreaData.dStartOffset;

	m_EidtTotNum = stBeamAreaData.nTotNum;

	m_EidtEndOffset = stBeamAreaData.dEndOffset;

	auto it = g_listRebarPosition.begin();
	for (int i = 0; i < stBeamAreaData.nPosition; i++)
	{
		it++;
	}

	m_CombPosition = *it;
}


void CBeamVerticalRebarDlg::OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	if (pNMListView->uChanged == LVIF_STATE)
	{
		if (pNMListView->uNewState)
		{
			m_nCurAreaLine = pNMListView->iItem; // 选中的是哪一行

			m_ListBeamAreaInfo.SetItemState(m_nCurAreaLine, LVIS_SELECTED, LVIS_SELECTED);

			//要进行的操作
			m_ListBeamAreaInfo.GetAllRebarData(m_vecBeamAreaData);
			UpdateAreaData(m_vecBeamAreaData.at(m_nCurAreaLine));
			UpdateRebarData(m_vecBeamRebarData.at(m_nCurAreaLine));

			UpdateData(false);
		}
	}

	*pResult = 0;
}


void CBeamVerticalRebarDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
}
