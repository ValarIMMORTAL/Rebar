#include "_USTATION.h"
#include "CColInsertRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "CInsertRebarAssemblyColumn.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "ElementAttribute.h"

// CInsertRebarDlg 对话框
IMPLEMENT_DYNAMIC(CColInsertRebarDlg, CDialogEx)

CColInsertRebarDlg::CColInsertRebarDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_InsertRebarCol, pParent), m_ConcreteId(0)
{
	m_pColInsertRebarAssembly = NULL;
}

void CColInsertRebarDlg::SetDefaultInfo(InsertRebarInfo& stInsertRebarInfo)
{
	if (COMPARE_VALUES(stInsertRebarInfo.colInfo.width, 0.00) == 0 ||
		COMPARE_VALUES(stInsertRebarInfo.colInfo.length, 0.00) == 0 ||
		COMPARE_VALUES(stInsertRebarInfo.colInfo.heigth, 0.00) == 0) 
	{
		// 计算柱的长度和宽度
		CInsertRebarAssemblyColumn	objInsertRebarAssembly(m_ehSel.GetElementId(), m_ehSel.GetModelRef());
		objInsertRebarAssembly.CalcColumnLengthAndWidth(m_ehSel);
		stInsertRebarInfo.colInfo.length = objInsertRebarAssembly.Getlength();
		stInsertRebarInfo.colInfo.width = objInsertRebarAssembly.Getwidth();
		stInsertRebarInfo.colInfo.heigth = objInsertRebarAssembly.Getheigth();

		stInsertRebarInfo.colInfo.shape = 0;
		stInsertRebarInfo.colInfo.columeCover = 0.00;
		char arrDefault[] = { "12mm" };
		strncpy_s(stInsertRebarInfo.colInfo.rebarVerticalSize, arrDefault, sizeof(arrDefault));
		stInsertRebarInfo.colInfo.rebarVerticalType = 0;
		strncpy_s(stInsertRebarInfo.colInfo.rebarHoopSize, arrDefault, sizeof(arrDefault));
		stInsertRebarInfo.colInfo.rebarHoopType = 0;
		stInsertRebarInfo.rebarInfo.longNum = 3;
		stInsertRebarInfo.rebarInfo.shortNum = 2;
		strncpy_s(stInsertRebarInfo.rebarInfo.rebarSize, arrDefault, sizeof(arrDefault));
		stInsertRebarInfo.rebarInfo.rebarType = 0;
		stInsertRebarInfo.rebarInfo.embedLength = 0.00;
		stInsertRebarInfo.rebarInfo.expandLength = 0.00;
		stInsertRebarInfo.rebarInfo.endType = 0;
		stInsertRebarInfo.rebarInfo.cornerType = 0;
		stInsertRebarInfo.rebarInfo.rotateAngle = 0.00;
	}
}

void CColInsertRebarDlg::SetRebarSizeAndType(CComboBox& CombRebarSize, CComboBox& CombRebarType)
{
	for (auto var : g_listRebarSize)
	{
		CombRebarSize.AddString(var);
	}
	int nIndex = 0;
	CString sTemp = CString(g_InsertRebarInfo.colInfo.rebarVerticalSize);
	for (auto var : g_listRebarSize)
	{
		if (sTemp.Compare(var) == 0)
		{
			CombRebarSize.SetCurSel(nIndex);
			break;
		}
		nIndex++;
	}
	for (auto var : g_listRebarType)
	{
		CombRebarType.AddString(var);
	}
	CombRebarType.SetCurSel(g_InsertRebarInfo.colInfo.rebarVerticalType);
}

// CInsertRebarDlg 消息处理程序
BOOL CColInsertRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
	GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);

	g_InsertRebarInfo.colInfo.length = 0.00;
	g_InsertRebarInfo.colInfo.width = 0.00;
	g_InsertRebarInfo.colInfo.heigth = 0.00;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(g_InsertRebarInfo), g_InsertRebarInfo, InsertColInfoAttribute, m_ehSel.GetModelRef());

	SetDefaultInfo(g_InsertRebarInfo);

	// 长度
	CString sTemp = CString();
	sTemp.Format(_T("%.3f"), g_InsertRebarInfo.colInfo.length);
	m_EditElementLength.SetWindowText(sTemp);
	m_EditElementLength.EnableWindow(FALSE);

	// 宽度
	sTemp.Format(_T("%.3f"), g_InsertRebarInfo.colInfo.width);
	m_EditElementWidth.SetWindowText(sTemp);
	m_EditElementWidth.EnableWindow(FALSE);

	// 形状
	m_CombElementShape.AddString(_T("方形"));
	m_CombElementShape.AddString(_T("圆形"));
	m_CombElementShape.SetCurSel(g_InsertRebarInfo.colInfo.shape);

	// 柱保护层
	sTemp.Format(_T("%.3f"), g_InsertRebarInfo.colInfo.columeCover);
	m_EditProtectiveLayer.SetWindowText(sTemp);

	// 纵筋尺寸
	SetRebarSizeAndType(m_CombVerticalSize, m_CombVerticalType);

	// 箍筋尺寸
	SetRebarSizeAndType(m_CombHoopRebarSize, m_CombHoopRebarType);

	// 长面数量
	sTemp.Format(_T("%d"), g_InsertRebarInfo.rebarInfo.longNum);
	m_EditLongSurfaceNum.SetWindowText(sTemp);

	// 短面数量
	sTemp.Format(_T("%d"), g_InsertRebarInfo.rebarInfo.shortNum);
	m_EditShortSurfaceNum.SetWindowText(sTemp);

	// 钢筋名称
	SetRebarSizeAndType(m_CombRebarSize, m_CombRebarType);

	// 拓展
	sTemp.Format(_T("%.3f"), g_InsertRebarInfo.rebarInfo.embedLength);
	m_EditEmbed.SetWindowText(sTemp);

	// 埋置
	sTemp.Format(_T("%.3f"), g_InsertRebarInfo.rebarInfo.expandLength);
	m_EditExpand.SetWindowText(sTemp);

	//端部样式类型
	// CString s = g_vecEndType[0];
	for (auto var : g_listEndType)
	{
		m_CombEndStyle.AddString(var);
	}
	m_CombEndStyle.SetCurSel(g_InsertRebarInfo.rebarInfo.endType);

	// 弯钩方向
	m_CombHookDirection.AddString(_T("角部倾斜"));
	m_CombHookDirection.AddString(_T("角部垂直"));
	m_CombHookDirection.SetCurSel(g_InsertRebarInfo.rebarInfo.cornerType);

	// 旋转角
	sTemp.Format(_T("%d°"), g_InsertRebarInfo.rebarInfo.rotateAngle);
	m_EditRotationAngle.SetWindowText(sTemp);
	m_EditRotationAngle.EnableWindow(FALSE);

	// m_EditElementLength.SetWindowText();

	return TRUE;  // return TRUE unless you set the focus to a control
			  // 异常: OCX 属性页应返回 FALSE
}

CColInsertRebarDlg::~CColInsertRebarDlg()
{
}

void CColInsertRebarDlg::SetDiagParam(InsertRebarInfo& stInserRebarInfo)
{
	m_CombElementShape.AddString(_T("方形"));
	m_CombElementShape.AddString(_T("圆形"));

	for (auto var : g_listRebarSize)
	{
		m_CombVerticalSize.AddString(var);
	}
	for (auto var : g_listRebarType)
	{
		m_CombVerticalType.AddString(var);
	}
	for (auto var : g_listRebarSize)
	{
		m_CombHoopRebarSize.AddString(var);
	}
	for (auto var : g_listRebarType)
	{
		m_CombHoopRebarType.AddString(var);
	}
	for (auto var : g_listRebarSize)
	{
		m_CombRebarSize.AddString(var);
	}
	for (auto var : g_listRebarType)
	{
		m_CombRebarType.AddString(var);
	}
	for (auto var : g_listEndType)
	{
		m_CombEndStyle.AddString(var);
	}
	m_CombHookDirection.AddString(_T("角部倾斜"));
	m_CombHookDirection.AddString(_T("角部垂直"));

	m_InsertRebarInfo = stInserRebarInfo;

	m_CombElementShape.SetCurSel(stInserRebarInfo.colInfo.shape);

	CString sTemp = CString();
	sTemp.Format(_T("%.3f"), stInserRebarInfo.colInfo.length);
	m_EditElementLength.SetWindowText(sTemp);
	m_EditElementLength.EnableWindow(FALSE);

	sTemp.Format(_T("%.3f"), stInserRebarInfo.colInfo.width);
	m_EditElementWidth.SetWindowText(sTemp);
	m_EditElementWidth.EnableWindow(FALSE);

	sTemp.Format(_T("%.3f"), stInserRebarInfo.colInfo.columeCover);
	m_EditProtectiveLayer.SetWindowText(sTemp);

	sTemp = stInserRebarInfo.colInfo.rebarVerticalSize;
	int nIndex = 0;
	for (auto var : g_listRebarSize)
	{
		if (sTemp.Compare(var) == 0)
		{
			m_CombVerticalSize.SetCurSel(nIndex);
			break;
		}
		nIndex++;
	}
	m_CombVerticalType.SetCurSel(stInserRebarInfo.colInfo.rebarVerticalType);

	nIndex = 0;
	sTemp = stInserRebarInfo.colInfo.rebarHoopSize;
	for (auto var : g_listRebarSize)
	{
		if (sTemp.Compare(var) == 0)
		{
			m_CombHoopRebarSize.SetCurSel(nIndex);
			break;
		}
		nIndex++;
	}
	m_CombHoopRebarType.SetCurSel(stInserRebarInfo.colInfo.rebarHoopType);

	nIndex = 0;
	sTemp = stInserRebarInfo.rebarInfo.rebarSize;
	for (auto var : g_listRebarSize)
	{
		if (sTemp.Compare(var) == 0)
		{
			m_CombRebarSize.SetCurSel(nIndex);
			break;
		}
		nIndex++;
	}

	sTemp.Format(_T("%d"), stInserRebarInfo.rebarInfo.longNum);
	m_EditLongSurfaceNum.SetWindowText(sTemp);

	sTemp.Format(_T("%d"), stInserRebarInfo.rebarInfo.shortNum);
	m_EditShortSurfaceNum.SetWindowText(sTemp);

	nIndex = 0;
	sTemp = stInserRebarInfo.rebarInfo.rebarSize;
	for (auto var : g_listRebarSize)
	{
		if (sTemp.Compare(var) == 0)
		{
			m_CombRebarSize.SetCurSel(nIndex);
			break;
		}
		nIndex++;
	}
	m_CombRebarType.SetCurSel(stInserRebarInfo.rebarInfo.rebarType);

	sTemp.Format(_T("%.3f"), stInserRebarInfo.rebarInfo.embedLength);
	m_EditEmbed.SetWindowText(sTemp);

	sTemp.Format(_T("%.3f"), stInserRebarInfo.rebarInfo.expandLength);
	m_EditExpand.SetWindowText(sTemp);

	m_CombEndStyle.SetCurSel(stInserRebarInfo.rebarInfo.endType);
	m_CombHookDirection.SetCurSel(stInserRebarInfo.rebarInfo.cornerType);

	sTemp.Format(_T("%.f°"), stInserRebarInfo.rebarInfo.rotateAngle);
	m_EditRotationAngle.SetWindowText(sTemp);

}

void CColInsertRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO1, m_CombElementShape); // 形状
	DDX_Control(pDX, IDC_EDIT2, m_EditElementLength); // 长度
	DDX_Control(pDX, IDC_EDIT3, m_EditElementWidth);  // 宽度
	DDX_Control(pDX, IDC_EDIT5, m_EditProtectiveLayer); // 柱保护层
	DDX_Control(pDX, IDC_COMBO2, m_CombVerticalSize); // 纵筋尺寸
	DDX_Control(pDX, IDC_COMBO3, m_CombVerticalType); // 纵筋型号
	DDX_Control(pDX, IDC_COMBO4, m_CombHoopRebarSize); // 箍筋尺寸
	DDX_Control(pDX, IDC_COMBO5, m_CombHoopRebarType); // 箍筋类型
	DDX_Control(pDX, IDC_EDIT6, m_EditLongSurfaceNum); // 长面数量
	DDX_Control(pDX, IDC_EDIT7, m_EditShortSurfaceNum); // 短面数量
	DDX_Control(pDX, IDC_COMBO7, m_CombRebarSize); // 钢筋名称
	DDX_Control(pDX, IDC_COMBO8, m_CombRebarType); // 钢筋类型
	DDX_Control(pDX, IDC_EDIT8, m_EditEmbed); // 埋置
	DDX_Control(pDX, IDC_EDIT9, m_EditExpand); // 拓展
	DDX_Control(pDX, IDC_COMBO9, m_CombEndStyle); // 端部样式类型
	DDX_Control(pDX, IDC_COMBO10, m_CombHookDirection); // 弯钩方向
	DDX_Control(pDX, IDC_EDIT10, m_EditRotationAngle); // 旋转角
}

// CInsertRebarDlg 消息处理程序

BEGIN_MESSAGE_MAP(CColInsertRebarDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CColInsertRebarDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CColInsertRebarDlg::OnBnClickedCancel)
	ON_EN_CHANGE(IDC_EDIT8, &CColInsertRebarDlg::OnEnChangeEdit8)
	ON_EN_CHANGE(IDC_EDIT7, &CColInsertRebarDlg::OnEnChangeEdit7)
	ON_BN_CLICKED(IDC_BUTTON1, &CColInsertRebarDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CColInsertRebarDlg::OnBnClickedButton2)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CColInsertRebarDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_CHECK1, &CColInsertRebarDlg::OnBnClickedCheck1)
	ON_CBN_SELCHANGE(IDC_COMBO10, &CColInsertRebarDlg::OnCbnSelchangeCombo10)
	ON_EN_CHANGE(IDC_EDIT5, &CColInsertRebarDlg::OnEnChangeEdit5)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CColInsertRebarDlg::OnCbnSelchangeCombo2)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CColInsertRebarDlg::OnCbnSelchangeCombo3)
	ON_CBN_SELCHANGE(IDC_COMBO4, &CColInsertRebarDlg::OnCbnSelchangeCombo4)
	ON_CBN_SELCHANGE(IDC_COMBO5, &CColInsertRebarDlg::OnCbnSelchangeCombo5)
	ON_EN_CHANGE(IDC_EDIT6, &CColInsertRebarDlg::OnEnChangeEdit6)
	ON_CBN_SELCHANGE(IDC_COMBO7, &CColInsertRebarDlg::OnCbnSelchangeCombo7)
	ON_CBN_SELCHANGE(IDC_COMBO8, &CColInsertRebarDlg::OnCbnSelchangeCombo8)
	ON_EN_CHANGE(IDC_EDIT9, &CColInsertRebarDlg::OnEnChangeEdit9)
	ON_CBN_SELCHANGE(IDC_COMBO9, &CColInsertRebarDlg::OnCbnSelchangeCombo9)
	ON_EN_CHANGE(IDC_EDIT10, &CColInsertRebarDlg::OnEnChangeEdit10)
	ON_EN_CHANGE(IDC_EDIT3, &CColInsertRebarDlg::OnEnChangeEdit3)
	ON_EN_CHANGE(IDC_EDIT2, &CColInsertRebarDlg::OnEnChangeEdit2)
END_MESSAGE_MAP()

void CColInsertRebarDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}


void CColInsertRebarDlg::OnEnChangeEdit2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}


void CColInsertRebarDlg::OnEnChangeEdit8()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString strTemp = CString();
	m_EditEmbed.GetWindowText(strTemp);

	g_InsertRebarInfo.rebarInfo.embedLength = atof(CT2A(strTemp));

}


void CColInsertRebarDlg::OnEnChangeEdit7()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString	strTemp = CString();

	m_EditShortSurfaceNum.GetWindowText(strTemp);
	g_InsertRebarInfo.rebarInfo.shortNum = atoi(CT2A(strTemp));
}


void CColInsertRebarDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CColInsertRebarDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString s = CString();
	m_EditRotationAngle.GetWindowText(s);
	if (s.Compare(_T("0°")) == 0)
	{
		m_EditRotationAngle.SetWindowText(_T("90°"));
		g_InsertRebarInfo.rebarInfo.rotateAngle = 90.00;
	}
	else if (s.Compare(_T("90°")) == 0)
	{
		m_EditRotationAngle.SetWindowText(_T("180°"));
		g_InsertRebarInfo.rebarInfo.rotateAngle = 180.00;
	}
	else if (s.Compare(_T("180°")) == 0)
	{
		m_EditRotationAngle.SetWindowText(_T("270°"));
		g_InsertRebarInfo.rebarInfo.rotateAngle = 270.00;
	}
	else
	{
		m_EditRotationAngle.SetWindowText(_T("0°"));
		g_InsertRebarInfo.rebarInfo.rotateAngle = 0.00;
	}
}


void CColInsertRebarDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	g_InsertRebarInfo.colInfo.shape = m_CombElementShape.GetCurSel();
}


void CColInsertRebarDlg::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CColInsertRebarDlg::OnCbnSelchangeCombo10()
{
	// TODO: 在此添加控件通知处理程序代码
	g_InsertRebarInfo.rebarInfo.cornerType = m_CombHookDirection.GetCurSel();
}


void CColInsertRebarDlg::OnEnChangeEdit5()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString	strTemp = CString();

	m_EditProtectiveLayer.GetWindowText(strTemp);
	g_InsertRebarInfo.colInfo.columeCover = atof(CT2A(strTemp));
}


void CColInsertRebarDlg::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strTemp = CString();
	unsigned short nIndex = m_CombVerticalSize.GetCurSel();
	unsigned short i = 0;
	for (auto it : g_listRebarSize)
	{
		if (i == nIndex)
		{
			strTemp = it;
		}
		i++;
	}
	strncpy(g_InsertRebarInfo.colInfo.rebarVerticalSize, CT2A(strTemp.GetBuffer()), strTemp.GetLength() + 1);
}


void CColInsertRebarDlg::OnCbnSelchangeCombo3()
{
	// TODO: 在此添加控件通知处理程序代码
	g_InsertRebarInfo.colInfo.rebarVerticalType = m_CombVerticalType.GetCurSel();
}


void CColInsertRebarDlg::OnCbnSelchangeCombo4()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strTemp = CString();
	unsigned short nIndex = m_CombHoopRebarSize.GetCurSel();
	unsigned short i = 0;
	for (auto it : g_listRebarSize)
	{
		if (i == nIndex)
		{
			strTemp = it;
		}
		i++;
	}
	strncpy(g_InsertRebarInfo.colInfo.rebarHoopSize, CT2A(strTemp.GetBuffer()), strTemp.GetLength() + 1);
}


void CColInsertRebarDlg::OnCbnSelchangeCombo5()
{
	// TODO: 在此添加控件通知处理程序代码
	g_InsertRebarInfo.colInfo.rebarHoopType = m_CombHoopRebarType.GetCurSel();
}


void CColInsertRebarDlg::OnEnChangeEdit6()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码

	CString	strTemp = CString();

	m_EditLongSurfaceNum.GetWindowText(strTemp);
	g_InsertRebarInfo.rebarInfo.longNum = atoi(CT2A(strTemp));
}


void CColInsertRebarDlg::OnCbnSelchangeCombo7()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strTemp = CString();
	unsigned short nIndex = m_CombRebarSize.GetCurSel();
	unsigned short i = 0;
	for (auto it : g_listRebarSize)
	{
		if (i == nIndex)
		{
			strTemp = it;
		}
		i++;
	}
	strncpy(g_InsertRebarInfo.rebarInfo.rebarSize, CT2A(strTemp.GetBuffer()), strTemp.GetLength() + 1);
}


void CColInsertRebarDlg::OnCbnSelchangeCombo8()
{
	// TODO: 在此添加控件通知处理程序代码
	g_InsertRebarInfo.rebarInfo.rebarType = m_CombRebarType.GetCurSel();
}


void CColInsertRebarDlg::OnEnChangeEdit9()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString	strTemp = CString();

	m_EditExpand.GetWindowText(strTemp);
	g_InsertRebarInfo.rebarInfo.expandLength = atof(CT2A(strTemp));
}


void CColInsertRebarDlg::OnCbnSelchangeCombo9()
{
	// TODO: 在此添加控件通知处理程序代码
	g_InsertRebarInfo.rebarInfo.endType = m_CombEndStyle.GetCurSel();
}


void CColInsertRebarDlg::OnEnChangeEdit10()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}

void CColInsertRebarDlg::CoverOnBnClickedOk()
{
	OnBnClickedOk();
}

void CColInsertRebarDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = m_ehSel.GetModelRef();
	EditElementHandle eeh(m_ehSel, m_ehSel.GetModelRef());

	CInsertRebarAssemblyColumn::IsSmartSmartFeature(eeh);
	if (m_pColInsertRebarAssembly == NULL)
	{
		m_pColInsertRebarAssembly = REA::Create<CInsertRebarAssemblyColumn>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	}

	m_pColInsertRebarAssembly->CalcColumnLengthAndWidth(eeh);
	m_pColInsertRebarAssembly->SetInsertRebarData(g_InsertRebarInfo);
	m_pColInsertRebarAssembly->MakeRebars(modelRef);
	m_pColInsertRebarAssembly->Save(modelRef);

	SetElementXAttribute(m_ehSel.GetElementId(), sizeof(g_InsertRebarInfo), &g_InsertRebarInfo, InsertColInfoAttribute, m_ehSel.GetModelRef());

	return;
}


void CColInsertRebarDlg::OnEnChangeEdit3()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}
