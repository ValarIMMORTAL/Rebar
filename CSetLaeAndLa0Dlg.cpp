// CSetLaeAndLa0Dlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CSetLaeAndLa0Dlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "CommonFile.h"
#include "CSetLae.h"

// CSetLaeAndLa0Dlg 对话框

extern StrLa0 g_StrLa0;
extern StrLae g_StrLae;

int g_global_stander = 0; // 定义并初始化全局变量

IMPLEMENT_DYNAMIC(CSetLaeAndLa0Dlg, CDialogEx)

CSetLaeAndLa0Dlg::CSetLaeAndLa0Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_SetLaeAndLa0, pParent)
{
	m_CurSelTab = 0;
}

CSetLaeAndLa0Dlg::~CSetLaeAndLa0Dlg()
{
}

void CSetLaeAndLa0Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_La, m_tab);
	DDX_Control(pDX, IDC_COMBO1, m_Stander_ComboBox);
}


BEGIN_MESSAGE_MAP(CSetLaeAndLa0Dlg, CDialogEx)
	// 	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_WallRebar, &CSetLaeAndLa0Dlg::OnTcnSelchangeTabWallrebar)
	// 	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_La, &CSetLaeAndLa0Dlg::OnTcnSelchangeTabLa)
	ON_BN_CLICKED(IDOK, &CSetLaeAndLa0Dlg::OnBnClickedOk)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_La, &CSetLaeAndLa0Dlg::OnTcnSelchangeTabLa)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CSetLaeAndLa0Dlg::OnStandard_Selection)
END_MESSAGE_MAP()

//CSetLaeAndLa0Dlg 结构标准选择消息处理程序
void CSetLaeAndLa0Dlg::OnStandard_Selection()
{
	// 设置默认选择
	//m_Stander_ComboBox.SetCurSel(0); // 设定默认选择

	// 获取当前选择的索引
	int number = m_Stander_ComboBox.GetCurSel();
	CString text;

	// 确保选择有效后再获取文本
	if (number != CB_ERR) {
		m_Stander_ComboBox.GetLBText(number, text); // 用有效的索引获取文本

		// 根据文本设置全局变量
		if (text == "混凝土结构设计标准") {
			g_global_stander = 0;
		}
		else {
			g_global_stander = 1;
		}
	}
	else {
		// 处理无效选择的情况
		AfxMessageBox(_T("无效选择，请重试！"));
		return; // 提前返回，避免继续执行
	}

	// 初始化对话框
	pDialog[0]->OnInitDialog();
	pDialog[1]->OnInitDialog();
}


// CSetLaeAndLa0Dlg 消息处理程序
void CSetLaeAndLa0Dlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	USES_CONVERSION;
	//根据选择的界面下标选择保存是搭接还是锚固的表格的数据，0是搭接，1是锚固
	/*if (m_CurSelTab == 0)
	{*/
	// 将前面保存的数据删除
	if (g_globalpara.m_laplenth.find("A") != g_globalpara.m_laplenth.end())
	{
		auto it = g_globalpara.m_laplenth.find("A");
		g_globalpara.m_laplenth.erase(it);
	}
	if (g_globalpara.m_laplenth.find("B") != g_globalpara.m_laplenth.end())
	{
		auto it = g_globalpara.m_laplenth.find("B");
		g_globalpara.m_laplenth.erase(it);
	}
	if (g_globalpara.m_laplenth.find("C") != g_globalpara.m_laplenth.end())
	{
		auto it = g_globalpara.m_laplenth.find("C");
		g_globalpara.m_laplenth.erase(it);
	}
	if (g_globalpara.m_laplenth.find("D") != g_globalpara.m_laplenth.end())
	{
		auto it = g_globalpara.m_laplenth.find("D");
		g_globalpara.m_laplenth.erase(it);
	}

	//std::string La0_RebarGrade(W2A(m_La0Dlg.m_Str_RebarGrade));
	std::string La0_RebarGrade(W2A(g_StrLa0.g_StrLa0_RebarGrade));
	if (La0_RebarGrade == "HPB300")
		La0_RebarGrade = "A";
	else if (La0_RebarGrade == "HRB335")
		La0_RebarGrade = "B";
	else if (La0_RebarGrade == "HRB400")
		La0_RebarGrade = "C";
	else if (La0_RebarGrade == "HRB500")
		La0_RebarGrade = "D";
	//_ttof将CString转为double
	g_globalpara.m_laplenth[La0_RebarGrade] = g_StrLa0.g_db_La0Value;
	//m_La0Dlg.WriteXml();
/*}
else
{*/
	if (g_globalpara.m_alength.find("A") != g_globalpara.m_alength.end())
	{
		auto it = g_globalpara.m_alength.find("A");
		g_globalpara.m_alength.erase(it);
	}
	if (g_globalpara.m_alength.find("C") != g_globalpara.m_alength.end())
	{
		auto it = g_globalpara.m_alength.find("C");
		g_globalpara.m_alength.erase(it);
	}
	if (g_globalpara.m_alength.find("D") != g_globalpara.m_alength.end())
	{
		auto it = g_globalpara.m_alength.find("D");
		g_globalpara.m_alength.erase(it);
	}

	//std::string Lae_RebarGrade(W2A(m_LaeDlg.m_Str_RebarGrade));
	std::string Lae_RebarGrade(W2A(g_StrLae.g_StrLae_RebarGrade));
	if (Lae_RebarGrade == "HPB300")
		Lae_RebarGrade = "A";
	/*else if (Lae_RebarGrade == "HRB335")
		Lae_RebarGrade = "B";*/
	else if (Lae_RebarGrade == "HRB400")
		Lae_RebarGrade = "C";
	else if (Lae_RebarGrade == "HRB500")
		Lae_RebarGrade = "D";
	//_ttof将CString转为double
	g_globalpara.m_alength[Lae_RebarGrade] = g_StrLae.g_db_LaeValue;
	//m_LaeDlg.WriteXml();
//}

	CString str;
	str.Format(L"存取的搭接长度L0 = %f", g_StrLa0.g_db_La0Value);
	mdlDialog_dmsgsPrint(str);
	CString str1;
	str1.Format(L"存取的锚固长度La = %f", g_StrLae.g_db_LaeValue);
	mdlDialog_dmsgsPrint(str1);
	mdlDialog_dmsgsPrintA("\n");
}


void CSetLaeAndLa0Dlg::OnTcnSelchangeTabLa(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	//把当前的页面隐藏起来
	pDialog[m_CurSelTab]->ShowWindow(SW_HIDE);
	//得到新的页面索引和选择的相应钢筋标准的索引
	m_CurSelTab = m_tab.GetCurSel();

	switch (m_CurSelTab)
	{
	case 0:
	{
		//1、根据CComboBox的值设置m_Condition
		//m_La0Dlg.SetConditionData();
		//m_La0Dlg.Set_gConditionData();
		//
		////2、填写表格的值存到 m_TableData
		//m_La0Dlg.SetListData();
		//
		////3、刷新表格
		//m_La0Dlg.UpdateLa0List();
		//
		////4.保存表格里的值
		////m_La0Dlg.SaveConcreteAndRebar_Grade();
		//m_La0Dlg.Save_gConcreteAndRebar_Grade();
		pDialog[0]->OnInitDialog();

	}
	break;
	case 1:
	{
		//1、根据CComboBox的值设置m_Condition
		//m_LaeDlg.SetConditionData();
		//m_LaeDlg.Set_gConditionData();
		//
		////2、填写表格的值存到 m_TableData
		//m_LaeDlg.SetListData();
		//
		////3、刷新表格
		//m_LaeDlg.UpdateLaeList();
		//
		////4、保存表格里的值
		////m_LaeDlg.SaveConcreteAndRebar_Grade();
		//m_LaeDlg.Save_gConcreteAndRebar_Grade();
		pDialog[1]->OnInitDialog();

	}
	break;
	default:
		break;
	}

	//把新的页面显示出来
	pDialog[m_CurSelTab]->ShowWindow(SW_SHOW);

	*pResult = 0;
}


BOOL CSetLaeAndLa0Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	//1、为Tab Control增加两个页面以及创建钢筋标准选择
	if (g_global_stander == 1)
		SetDlgItemText(IDC_COMBO1, _T("核电厂混凝土结构技术标准"));
	else
		SetDlgItemText(IDC_COMBO1, _T("混凝土结构技术标准"));

	m_tab.InsertItem(0, _T("搭接"));
	m_tab.InsertItem(1, _T("锚固"));
	g_global_stander = 0;
	m_La0Dlg.Create(IDD_SetLa0, &m_tab);//搭接
	m_LaeDlg.Create(IDD_SetLae, &m_tab);//锚固

	m_Stander_ComboBox.AddString(_T("混凝土结构设计标准"));
	m_Stander_ComboBox.AddString(_T("核电厂混凝土结构技术标准"));
	//2、设定在Tab内显示的范围
	CRect rc;
	m_tab.GetClientRect(rc);
	rc.top += 20;
	rc.bottom -= 0;
	rc.left += 0;
	rc.right -= 0;
	m_La0Dlg.MoveWindow(&rc);
	m_LaeDlg.MoveWindow(&rc);

	//3、把对话框对象指针保存起来
	pDialog[0] = &m_La0Dlg;
	pDialog[1] = &m_LaeDlg;

	//4、显示初始页面
	pDialog[0]->ShowWindow(SW_SHOW);
	pDialog[1]->ShowWindow(SW_HIDE);

	//5、保存当前选择
	m_CurSelTab = 0;
	return TRUE;
}