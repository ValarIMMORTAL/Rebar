// CSetLa0.cpp: 实现文件
//

#include "_USTATION.h"
#include "resource.h"
#include "CommonFile.h"
#include "GalleryIntelligentRebar.h"
#include "CSetLa0.h"
#include "afxdialogex.h"

StrLa0 g_StrLa0 = { L"C20" ,L"<=25%" ,L"HPB300" ,L"<=25" ,L"一、二级" ,54};
//g_StrLa0.g_StrLa0_SeismicGrade = L"一、二级";
//g_StrLa0.g_StrLa0_OverlapArea = L"<=25%";
//g_StrLa0.g_StrLa0_RebarDiameter = L"<=25";
//g_StrLa0.g_StrLa0_ConcreteGrade = L"C20";
//g_StrLa0.g_StrLa0_RebarGrade = L"HPB300";
// CSetLa0 对话框

IMPLEMENT_DYNAMIC(CSetLa0, CDialogEx)

CSetLa0::CSetLa0(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SetLa0, pParent)
{
	m_Condition = 0;
	m_Str_SeismicGrade = L"一、二级";
	m_Str_OverlapArea = L"<=25%";
	m_Str_RebarDiameter = L"<=25";
	m_Str_ConcreteGrade = L"C20";
	m_Str_RebarGrade = L"HPB300";
	m_Sel_ConcreteGrade = 0;
	m_data = L"54";
}

CSetLa0::~CSetLa0()
{
}

void CSetLa0::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ConcreteGrade, m_ConcreteGrade);
	DDX_Control(pDX, IDC_OverlapArea, m_OverlapArea);
	DDX_Control(pDX, IDC_RebarGrade, m_RebarGrade);
	DDX_Control(pDX, IDC_RebarDiameter, m_RebarDiameter);
	DDX_Control(pDX, IDC_SeismicGrade, m_SeismicGrade);
	DDX_Control(pDX, IDC_LIST1, m_La0_ListCtrl);
}


BEGIN_MESSAGE_MAP(CSetLa0, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSetLa0::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_ConcreteGrade, &CSetLa0::OnCbnSelchangeConcretegrade)
	ON_CBN_SELCHANGE(IDC_OverlapArea, &CSetLa0::OnCbnSelchangeOverlaparea)
	ON_CBN_SELCHANGE(IDC_RebarGrade, &CSetLa0::OnCbnSelchangeRebargrade)
	ON_CBN_SELCHANGE(IDC_RebarDiameter, &CSetLa0::OnCbnSelchangeRebardiameter)
	ON_CBN_SELCHANGE(IDC_SeismicGrade, &CSetLa0::OnCbnSelchangeSeismicgrade)
END_MESSAGE_MAP()


// CSetLa0 消息处理程序


void CSetLa0::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}

//初始化ComboBox的值
void CSetLa0::InitCComboBox()
{
	//混凝土等级
	m_ConcreteGrade.AddString(L"C20");
	m_ConcreteGrade.SetCurSel(0);
	m_ConcreteGrade.AddString(L"C25");
	m_ConcreteGrade.AddString(L"C30");
	m_ConcreteGrade.AddString(L"C35");
	m_ConcreteGrade.AddString(L"C40");
	m_ConcreteGrade.AddString(L"C45");
	m_ConcreteGrade.AddString(L"C50");
	m_ConcreteGrade.AddString(L"C55");
	m_ConcreteGrade.AddString(L"C60");
	
	SetDlgItemText(IDC_ConcreteGrade, L"C20");

	//搭接面积
	m_OverlapArea.AddString(L"<=25%");
	m_OverlapArea.SetCurSel(0);
	m_OverlapArea.AddString(L"50%");
	
	SetDlgItemText(IDC_OverlapArea, L"<=25%");

	//钢筋等级
	m_RebarGrade.AddString(L"HPB300");
	m_RebarGrade.SetCurSel(0);
	m_RebarGrade.AddString(L"HRB335");
	m_RebarGrade.AddString(L"HRB400");
	m_RebarGrade.AddString(L"HRB500");
	
	SetDlgItemText(IDC_RebarGrade, L"HPB300");

	//钢筋直径
	m_RebarDiameter.AddString(L"<=25");
	m_RebarDiameter.SetCurSel(0);
	m_RebarDiameter.AddString(L">25");
	
	SetDlgItemText(IDC_RebarDiameter, L"<=25");

	//抗震等级
	m_SeismicGrade.AddString(L"一、二级");
	m_SeismicGrade.SetCurSel(0);
	m_SeismicGrade.AddString(L"三级");
	
	SetDlgItemText(IDC_SeismicGrade, L"一、二级");


}

void CSetLa0::UpdateComboBox()
{
	if (g_StrLa0.g_StrLa0_ConcreteGrade != L"")
	{
		SetDlgItemText(IDC_ConcreteGrade, g_StrLa0.g_StrLa0_ConcreteGrade);
		if (g_StrLa0.g_StrLa0_ConcreteGrade == L"C20")
			m_Sel_ConcreteGrade = 0;
		else if (g_StrLa0.g_StrLa0_ConcreteGrade == L"C25")
			m_Sel_ConcreteGrade = 1;
		else if (g_StrLa0.g_StrLa0_ConcreteGrade == L"C30")
			m_Sel_ConcreteGrade = 2;
		else if (g_StrLa0.g_StrLa0_ConcreteGrade == L"C35")
			m_Sel_ConcreteGrade = 3;
		else if (g_StrLa0.g_StrLa0_ConcreteGrade == L"C40")
			m_Sel_ConcreteGrade = 4;
		else if (g_StrLa0.g_StrLa0_ConcreteGrade == L"C45")
			m_Sel_ConcreteGrade = 5;
		else if (g_StrLa0.g_StrLa0_ConcreteGrade == L"C50")
			m_Sel_ConcreteGrade = 6;
		else if (g_StrLa0.g_StrLa0_ConcreteGrade == L"C55")
			m_Sel_ConcreteGrade = 7;
		else if (g_StrLa0.g_StrLa0_ConcreteGrade == L"C60")
			m_Sel_ConcreteGrade = 8;
	}

	if (g_StrLa0.g_StrLa0_OverlapArea != L"")
		SetDlgItemText(IDC_OverlapArea, g_StrLa0.g_StrLa0_OverlapArea);

	if (g_StrLa0.g_StrLa0_RebarGrade != L"")
		SetDlgItemText(IDC_RebarGrade, g_StrLa0.g_StrLa0_RebarGrade);

	if (g_StrLa0.g_StrLa0_RebarDiameter != L"")
		SetDlgItemText(IDC_RebarDiameter, g_StrLa0.g_StrLa0_RebarDiameter);

	if (g_StrLa0.g_StrLa0_SeismicGrade != L"")
		SetDlgItemText(IDC_SeismicGrade, g_StrLa0.g_StrLa0_SeismicGrade);

	if ((g_StrLa0.g_StrLa0_ConcreteGrade != L"") || (g_StrLa0.g_StrLa0_OverlapArea != L"") ||
		(g_StrLa0.g_StrLa0_RebarGrade != L"") || (g_StrLa0.g_StrLa0_RebarDiameter != L"") || (g_StrLa0.g_StrLa0_SeismicGrade != L""))
	{
		Set_gConditionData();
		SetListData();
		UpdateLa0List();
		Save_gConcreteAndRebar_Grade();
	}
}

//根据ComboBox的值设置Condition的值
void CSetLa0::SetConditionData()
{
	if ((m_Str_SeismicGrade == L"一、二级") && (m_Str_OverlapArea == L"<=25%") && (m_Str_RebarDiameter == L"<=25"))
	{
		m_Condition = 0;
	}
	else if((m_Str_SeismicGrade == L"一、二级") && (m_Str_OverlapArea == L"<=25%") && (m_Str_RebarDiameter == L">25"))
	{
		m_Condition = 1;
	}
	else if ((m_Str_SeismicGrade == L"一、二级") && (m_Str_OverlapArea == L"50%") && (m_Str_RebarDiameter == L"<=25"))
	{
		m_Condition = 2;
	}
	else if ((m_Str_SeismicGrade == L"一、二级") && (m_Str_OverlapArea == L"50%") && (m_Str_RebarDiameter == L">25"))
	{
		m_Condition = 3;
	}
	else if ((m_Str_SeismicGrade == L"三级") && (m_Str_OverlapArea == L"<=25%") && (m_Str_RebarDiameter == L"<=25"))
	{
		m_Condition = 4;
	}
	else if ((m_Str_SeismicGrade == L"三级") && (m_Str_OverlapArea == L"<=25%") && (m_Str_RebarDiameter == L">25"))
	{
		m_Condition = 5;
	}
	else if ((m_Str_SeismicGrade == L"三级") && (m_Str_OverlapArea == L"50%") && (m_Str_RebarDiameter == L"<=25"))
	{
		m_Condition = 6;
	}
	else if ((m_Str_SeismicGrade == L"三级") && (m_Str_OverlapArea == L"50%") && (m_Str_RebarDiameter == L">25"))
	{
		m_Condition = 7;
	}

	return;
}


//根据全局变量的值设置Condition的值
void CSetLa0::Set_gConditionData()
{
	if ((g_StrLa0.g_StrLa0_SeismicGrade == L"一、二级") && (g_StrLa0.g_StrLa0_OverlapArea == L"<=25%") && (g_StrLa0.g_StrLa0_RebarDiameter == L"<=25"))
	{
		m_Condition = 0;
	}
	else if ((g_StrLa0.g_StrLa0_SeismicGrade == L"一、二级") && (g_StrLa0.g_StrLa0_OverlapArea == L"<=25%") && (g_StrLa0.g_StrLa0_RebarDiameter == L">25"))
	{
		m_Condition = 1;
	}
	else if ((g_StrLa0.g_StrLa0_SeismicGrade == L"一、二级") && (g_StrLa0.g_StrLa0_OverlapArea == L"50%") && (g_StrLa0.g_StrLa0_RebarDiameter == L"<=25"))
	{
		m_Condition = 2;
	}
	else if ((g_StrLa0.g_StrLa0_SeismicGrade == L"一、二级") && (g_StrLa0.g_StrLa0_OverlapArea == L"50%") && (g_StrLa0.g_StrLa0_RebarDiameter == L">25"))
	{
		m_Condition = 3;
	}
	else if ((g_StrLa0.g_StrLa0_SeismicGrade == L"三级") && (g_StrLa0.g_StrLa0_OverlapArea == L"<=25%") && (g_StrLa0.g_StrLa0_RebarDiameter == L"<=25"))
	{
		m_Condition = 4;
	}
	else if ((g_StrLa0.g_StrLa0_SeismicGrade == L"三级") && (g_StrLa0.g_StrLa0_OverlapArea == L"<=25%") && (g_StrLa0.g_StrLa0_RebarDiameter == L">25"))
	{
		m_Condition = 5;
	}
	else if ((g_StrLa0.g_StrLa0_SeismicGrade == L"三级") && (g_StrLa0.g_StrLa0_OverlapArea == L"50%") && (g_StrLa0.g_StrLa0_RebarDiameter == L"<=25"))
	{
		m_Condition = 6;
	}
	else if ((g_StrLa0.g_StrLa0_SeismicGrade == L"三级") && (g_StrLa0.g_StrLa0_OverlapArea == L"50%") && (g_StrLa0.g_StrLa0_RebarDiameter == L">25"))
	{
		m_Condition = 7;
	}

	return;
}

//填充表格数据
void CSetLa0::SetListData()
{
	//存第一行，每一列的数据
	vector<vector<CString>> row1;

	vector<CString> column1;
	column1.push_back(L"54");
	column1.push_back(L"");
	column1.push_back(L"63");
	column1.push_back(L"");
	column1.push_back(L"49");
	column1.push_back(L"");
	column1.push_back(L"57");
	column1.push_back(L"");
	row1.push_back(column1);
	vector<CString> column2;
	column2.push_back(L"47");
	column2.push_back(L"");
	column2.push_back(L"55");
	column2.push_back(L"");
	column2.push_back(L"43");
	column2.push_back(L"");
	column2.push_back(L"50");
	column2.push_back(L"");
	row1.push_back(column2);
	vector<CString> column3;
	column3.push_back(L"42");
	column3.push_back(L"");
	column3.push_back(L"49");
	column3.push_back(L"");
	column3.push_back(L"38");
	column3.push_back(L"");
	column3.push_back(L"45");
	column3.push_back(L"");
	row1.push_back(column3);
	vector<CString> column4;
	column4.push_back(L"38");
	column4.push_back(L"");
	column4.push_back(L"45");
	column4.push_back(L"");
	column4.push_back(L"35");
	column4.push_back(L"");
	column4.push_back(L"41");
	column4.push_back(L"");
	row1.push_back(column4);
	vector<CString> column5;
	column5.push_back(L"35");
	column5.push_back(L"");
	column5.push_back(L"41");
	column5.push_back(L"");
	column5.push_back(L"31");
	column5.push_back(L"");
	column5.push_back(L"36");
	column5.push_back(L"");
	row1.push_back(column5);
	vector<CString> column6;
	column6.push_back(L"34");
	column6.push_back(L"");
	column6.push_back(L"39");
	column6.push_back(L"");
	column6.push_back(L"30");
	column6.push_back(L"");
	column6.push_back(L"35");
	column6.push_back(L"");
	row1.push_back(column6);
	vector<CString> column7;
	column7.push_back(L"31");
	column7.push_back(L"");
	column7.push_back(L"36");
	column7.push_back(L"");
	column7.push_back(L"29");
	column7.push_back(L"");
	column7.push_back(L"34");
	column7.push_back(L"");
	row1.push_back(column7);
	vector<CString> column8;
	column8.push_back(L"30");
	column8.push_back(L"");
	column8.push_back(L"35");
	column8.push_back(L"");
	column8.push_back(L"28");
	column8.push_back(L"");
	column8.push_back(L"32");
	column8.push_back(L"");
	row1.push_back(column8);
	vector<CString> column9;
	column9.push_back(L"29");
	column9.push_back(L"");
	column9.push_back(L"34");
	column9.push_back(L"");
	column9.push_back(L"26");
	column9.push_back(L"");
	column9.push_back(L"31");
	column9.push_back(L"");
	row1.push_back(column9);
	m_TableData[1] = row1;

	column1.clear();
	column2.clear();
	column3.clear();
	column4.clear();
	column5.clear();
	column6.clear();
	column7.clear();
	column8.clear();
	column9.clear();
	//存第二行，每一列的数据
	vector<vector<CString>> row2;
	
	column1.push_back(L"53");
	column1.push_back(L"");
	column1.push_back(L"62");
	column1.push_back(L"");
	column1.push_back(L"48");
	column1.push_back(L"");
	column1.push_back(L"56");
	column1.push_back(L"");
	row2.push_back(column1);
	column2.push_back(L"46");
	column2.push_back(L"");
	column2.push_back(L"53");
	column2.push_back(L"");
	column2.push_back(L"42");
	column2.push_back(L"");
	column2.push_back(L"49");
	column2.push_back(L"");
	row2.push_back(column2);
	column3.push_back(L"40");
	column3.push_back(L"");
	column3.push_back(L"46");
	column3.push_back(L"");
	column3.push_back(L"36");
	column3.push_back(L"");
	column3.push_back(L"42");
	column3.push_back(L"");
	row2.push_back(column3);
	column4.push_back(L"37");
	column4.push_back(L"");
	column4.push_back(L"43");
	column4.push_back(L"");
	column4.push_back(L"34");
	column4.push_back(L"");
	column4.push_back(L"39");
	column4.push_back(L"");
	row2.push_back(column4);
	column5.push_back(L"35");
	column5.push_back(L"");
	column5.push_back(L"41");
	column5.push_back(L"");
	column5.push_back(L"31");
	column5.push_back(L"");
	column5.push_back(L"36");
	column5.push_back(L"");
	row2.push_back(column5);
	column6.push_back(L"31");
	column6.push_back(L"");
	column6.push_back(L"36");
	column6.push_back(L"");
	column6.push_back(L"29");
	column6.push_back(L"");
	column6.push_back(L"34");
	column6.push_back(L"");
	row2.push_back(column6);
	column7.push_back(L"30");
	column7.push_back(L"");
	column7.push_back(L"35");
	column7.push_back(L"");
	column7.push_back(L"28");
	column7.push_back(L"");
	column7.push_back(L"32");
	column7.push_back(L"");
	row2.push_back(column7);
	column8.push_back(L"29");
	column8.push_back(L"");
	column8.push_back(L"34");
	column8.push_back(L"");
	column8.push_back(L"26");
	column8.push_back(L"");
	column8.push_back(L"31");
	column8.push_back(L"");
	row2.push_back(column8);
	column9.push_back(L"29");
	column9.push_back(L"");
	column9.push_back(L"34");
	column9.push_back(L"");
	column9.push_back(L"26");
	column9.push_back(L"");
	column9.push_back(L"31");
	column9.push_back(L"");
	row2.push_back(column9);
	m_TableData[2] = row2;

	column1.clear();
	column2.clear();
	column3.clear();
	column4.clear();
	column5.clear();
	column6.clear();
	column7.clear();
	column8.clear();
	column9.clear();
	//存第三行，每一列的数据
	vector<vector<CString>> row3;

	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	row3.push_back(column1);
	column2.push_back(L"55");
	column2.push_back(L"61");
	column2.push_back(L"64");
	column2.push_back(L"71");
	column2.push_back(L"50");
	column2.push_back(L"55");
	column2.push_back(L"59");
	column2.push_back(L"64");
	row3.push_back(column2);
	column3.push_back(L"48");
	column3.push_back(L"54");
	column3.push_back(L"56");
	column3.push_back(L"63");
	column3.push_back(L"44");
	column3.push_back(L"49");
	column3.push_back(L"52");
	column3.push_back(L"57");
	row3.push_back(column3);
	column4.push_back(L"44");
	column4.push_back(L"48");
	column4.push_back(L"52");
	column4.push_back(L"56");
	column4.push_back(L"41");
	column4.push_back(L"44");
	column4.push_back(L"48");
	column4.push_back(L"52");
	row3.push_back(column4);
	column5.push_back(L"40");
	column5.push_back(L"44");
	column5.push_back(L"46");
	column5.push_back(L"52");
	column5.push_back(L"36");
	column5.push_back(L"41");
	column5.push_back(L"42");
	column5.push_back(L"48");
	row3.push_back(column5);
	column6.push_back(L"38");
	column6.push_back(L"43");
	column6.push_back(L"45");
	column6.push_back(L"50");
	column6.push_back(L"35");
	column6.push_back(L"40");
	column6.push_back(L"41");
	column6.push_back(L"46");
	row3.push_back(column6);
	column7.push_back(L"37");
	column7.push_back(L"42");
	column7.push_back(L"43");
	column7.push_back(L"49");
	column7.push_back(L"34");
	column7.push_back(L"38");
	column7.push_back(L"39");
	column7.push_back(L"45");
	row3.push_back(column7);
	column8.push_back(L"36");
	column8.push_back(L"40");
	column8.push_back(L"42");
	column8.push_back(L"46");
	column8.push_back(L"32");
	column8.push_back(L"36");
	column8.push_back(L"38");
	column8.push_back(L"42");
	row3.push_back(column8);
	column9.push_back(L"35");
	column9.push_back(L"38");
	column9.push_back(L"41");
	column9.push_back(L"45");
	column9.push_back(L"31");
	column9.push_back(L"35");
	column9.push_back(L"36");
	column9.push_back(L"41");
	row3.push_back(column9);
	m_TableData[3] = row3;

	column1.clear();
	column2.clear();
	column3.clear();
	column4.clear();
	column5.clear();
	column6.clear();
	column7.clear();
	column8.clear();
	column9.clear();
	//存第四行，每一列的数据
	vector<vector<CString>> row4;

	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	column1.push_back(L"");
	row4.push_back(column1);
	column2.push_back(L"66");
	column2.push_back(L"73");
	column2.push_back(L"77");
	column2.push_back(L"85");
	column2.push_back(L"60");
	column2.push_back(L"67");
	column2.push_back(L"70");
	column2.push_back(L"78");
	row4.push_back(column2);
	column3.push_back(L"59");
	column3.push_back(L"65");
	column3.push_back(L"69");
	column3.push_back(L"76");
	column3.push_back(L"54");
	column3.push_back(L"59");
	column3.push_back(L"63");
	column3.push_back(L"69");
	row4.push_back(column3);
	column4.push_back(L"54");
	column4.push_back(L"59");
	column4.push_back(L"63");
	column4.push_back(L"69");
	column4.push_back(L"49");
	column4.push_back(L"54");
	column4.push_back(L"57");
	column4.push_back(L"63");
	row4.push_back(column4);
	column5.push_back(L"49");
	column5.push_back(L"55");
	column5.push_back(L"57");
	column5.push_back(L"64");
	column5.push_back(L"46");
	column5.push_back(L"50");
	column5.push_back(L"53");
	column5.push_back(L"59");
	row4.push_back(column5);
	column6.push_back(L"47");
	column6.push_back(L"52");
	column6.push_back(L"55");
	column6.push_back(L"60");
	column6.push_back(L"43");
	column6.push_back(L"47");
	column6.push_back(L"50");
	column6.push_back(L"55");
	row4.push_back(column6);
	column7.push_back(L"44");
	column7.push_back(L"48");
	column7.push_back(L"52");
	column7.push_back(L"56");
	column7.push_back(L"41");
	column7.push_back(L"44");
	column7.push_back(L"48");
	column7.push_back(L"52");
	row4.push_back(column7);
	column8.push_back(L"43");
	column8.push_back(L"47");
	column8.push_back(L"50");
	column8.push_back(L"55");
	column8.push_back(L"40");
	column8.push_back(L"43");
	column8.push_back(L"46");
	column8.push_back(L"50");
	row4.push_back(column8);
	column9.push_back(L"42");
	column9.push_back(L"46");
	column9.push_back(L"49");
	column9.push_back(L"53");
	column9.push_back(L"38");
	column9.push_back(L"42");
	column9.push_back(L"45");
	column9.push_back(L"49");
	row4.push_back(column9);
	m_TableData[4] = row4;

	return;

}

//刷新表格数据
void CSetLa0::UpdateLa0List()
{
	m_La0_ListCtrl.DeleteAllItems();
	//分成si行一行一行的生成表格
	//第一行
	m_La0_ListCtrl.InsertItem(0, L"");
	CString strValue = L"HPB300";
	m_La0_ListCtrl.SetItemText(0, 0, strValue);
	for (int i = 1; i < 10; ++i)
	{
		CString strValue = L"";
		strValue = m_TableData[1].at(i - 1).at(m_Condition);
		m_La0_ListCtrl.SetItemText(0, i, strValue);
	}

	//第二行
	m_La0_ListCtrl.InsertItem(1, L"");
	strValue = L"HRB335";
	m_La0_ListCtrl.SetItemText(1, 0, strValue);
	for (int i = 1; i < 10; ++i)
	{
		CString strValue = L"";
		strValue = m_TableData[2].at(i - 1).at(m_Condition);
		m_La0_ListCtrl.SetItemText(1, i, strValue);
	}

	//第三行
	m_La0_ListCtrl.InsertItem(2, L"");
	strValue = L"HRB400";
	m_La0_ListCtrl.SetItemText(2, 0, strValue);
	for (int i = 1; i < 10; ++i)
	{
		CString strValue = L"";
		strValue = m_TableData[3].at(i - 1).at(m_Condition);
		m_La0_ListCtrl.SetItemText(2, i, strValue);
	}

	//第四行
	m_La0_ListCtrl.InsertItem(3, L"");
	strValue = L"HRB500";
	m_La0_ListCtrl.SetItemText(3, 0, strValue);
	for (int i = 1; i < 10; ++i)
	{
		CString strValue = L"";
		strValue = m_TableData[4].at(i - 1).at(m_Condition);
		m_La0_ListCtrl.SetItemText(3, i, strValue);
	}

	return;
}

BOOL CSetLa0::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//1、初始化四个CComboBox按钮的值
	InitCComboBox();

	//2、设置列表属性并在列表控件中插入列
	LONG lStyle;
	lStyle = GetWindowLong(m_La0_ListCtrl.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_La0_ListCtrl.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_La0_ListCtrl.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_La0_ListCtrl.SetExtendedStyle(dwStyle);			// 设置扩展风格

	tagRECT stRect;
	m_La0_ListCtrl.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_La0_ListCtrl.InsertColumn(0, L"", (int)(width / 10.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_La0_ListCtrl.InsertColumn(1, L"C20", (int)(width / 10.0), ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_La0_ListCtrl.InsertColumn(2, L"C25", (int)(width / 10.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_La0_ListCtrl.InsertColumn(3, L"C30", (int)(width / 10.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_La0_ListCtrl.InsertColumn(4, L"C35", (int)(width / 10.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_La0_ListCtrl.InsertColumn(5, L"C40", (int)(width / 10.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_La0_ListCtrl.InsertColumn(6, L"C45", (int)(width / 10.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_La0_ListCtrl.InsertColumn(7, L"C50", (int)(width / 10.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_La0_ListCtrl.InsertColumn(8, L"C55", (int)(width / 10.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_La0_ListCtrl.InsertColumn(9, L"C60", (int)(width / 10.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	
	//3、根据CComboBox的值设置m_Condition
	SetConditionData();

	//4、填写表格的值存到 m_TableData
	SetListData();

	//5、根据m_Condition的值设置表格的数据
	UpdateLa0List();

	//6、读取XML文件参数
	readXML();

	//7、根据全局变量设置的值刷新到上一次设置的表格
	UpdateComboBox();



	return TRUE;
}

void CSetLa0::OnCbnSelchangeConcretegrade()
{
	// TODO: 在此添加控件通知处理程序代码

	//int nSel;
	//获取组合框控件的列表框中选中项的索引
	m_Sel_ConcreteGrade = m_ConcreteGrade.GetCurSel();
	//根据选中索引项获取该项字符串
	m_ConcreteGrade.GetLBText(m_Sel_ConcreteGrade, m_Str_ConcreteGrade);

	/*SetConditionData();
	UpdateLa0List();*/
	//SaveConcreteAndRebar_Grade();

	g_StrLa0.g_StrLa0_ConcreteGrade = m_Str_ConcreteGrade;
	Set_gConditionData();
	Save_gConcreteAndRebar_Grade();

	WriteXml();

	return;
}


void CSetLa0::OnCbnSelchangeOverlaparea()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSel;
	//获取组合框控件的列表框中选中项的索引
	nSel = m_OverlapArea.GetCurSel();
	//根据选中索引项获取该项字符串
	m_OverlapArea.GetLBText(nSel, m_Str_OverlapArea);

	//SetConditionData();
	g_StrLa0.g_StrLa0_OverlapArea = m_Str_OverlapArea;
	Set_gConditionData();
	UpdateLa0List();
	//SaveConcreteAndRebar_Grade();
	Save_gConcreteAndRebar_Grade();

	WriteXml();

	return;
}


void CSetLa0::OnCbnSelchangeRebargrade()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSel;
	//获取组合框控件的列表框中选中项的索引
	nSel = m_RebarGrade.GetCurSel();
	//根据选中索引项获取该项字符串
	m_RebarGrade.GetLBText(nSel, m_Str_RebarGrade);

	/*SetConditionData();
	UpdateLa0List();*/
	//SaveConcreteAndRebar_Grade();

	g_StrLa0.g_StrLa0_RebarGrade = m_Str_RebarGrade;
	Set_gConditionData();
	Save_gConcreteAndRebar_Grade();

	WriteXml();

	return;
}


void CSetLa0::OnCbnSelchangeRebardiameter()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSel;
	//获取组合框控件的列表框中选中项的索引
	nSel = m_RebarDiameter.GetCurSel();
	//根据选中索引项获取该项字符串
	m_RebarDiameter.GetLBText(nSel, m_Str_RebarDiameter);

	//SetConditionData();
	g_StrLa0.g_StrLa0_RebarDiameter = m_Str_RebarDiameter;
	Set_gConditionData();
	UpdateLa0List();
	//SaveConcreteAndRebar_Grade();
	Save_gConcreteAndRebar_Grade();

	WriteXml();

	return;
}


void CSetLa0::OnCbnSelchangeSeismicgrade()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSel;
	//获取组合框控件的列表框中选中项的索引
	nSel = m_SeismicGrade.GetCurSel();
	//根据选中索引项获取该项字符串
	m_SeismicGrade.GetLBText(nSel, m_Str_SeismicGrade);


	g_StrLa0.g_StrLa0_SeismicGrade = m_Str_SeismicGrade;
	//SetConditionData();
	Set_gConditionData();
	UpdateLa0List();
	//SaveConcreteAndRebar_Grade();
	Save_gConcreteAndRebar_Grade();

	WriteXml();

	return;
}

//保存表格里面的值
void CSetLa0::SaveConcreteAndRebar_Grade()
{
	if (m_Str_RebarGrade == L"HPB300")
	{
		m_data = m_TableData[1].at(m_Sel_ConcreteGrade).at(m_Condition);
	}
	else if (m_Str_RebarGrade == L"HRB335")
	{
		m_data = m_TableData[2].at(m_Sel_ConcreteGrade).at(m_Condition);
	}
	else if(m_Str_RebarGrade == L"HRB400")
	{
		m_data= m_TableData[3].at(m_Sel_ConcreteGrade).at(m_Condition);
	}
	else if(m_Str_RebarGrade == L"HRB500")
	{
		m_data = m_TableData[4].at(m_Sel_ConcreteGrade).at(m_Condition);
	}

	return;

}

void CSetLa0::Save_gConcreteAndRebar_Grade()
{
	if (g_StrLa0.g_StrLa0_RebarGrade == L"HPB300")
	{
		g_StrLa0.g_db_La0Value = _ttof(m_TableData[1].at(m_Sel_ConcreteGrade).at(m_Condition));
	}
	else if (g_StrLa0.g_StrLa0_RebarGrade == L"HRB335")
	{
		g_StrLa0.g_db_La0Value = _ttof(m_TableData[2].at(m_Sel_ConcreteGrade).at(m_Condition));
	}
	else if (g_StrLa0.g_StrLa0_RebarGrade == L"HRB400")
	{
		g_StrLa0.g_db_La0Value = _ttof(m_TableData[3].at(m_Sel_ConcreteGrade).at(m_Condition));
	}
	else if (g_StrLa0.g_StrLa0_RebarGrade == L"HRB500")
	{
		g_StrLa0.g_db_La0Value = _ttof(m_TableData[4].at(m_Sel_ConcreteGrade).at(m_Condition));
	}

	return;
}


//将参数值写入到XML文件中
void CSetLa0::WriteXml()
{
	
	CString StrLa0_ConcreteGrade = g_StrLa0.g_StrLa0_ConcreteGrade;		//混凝土等级
	CString StrLa0_OverlapArea = g_StrLa0.g_StrLa0_OverlapArea;			//搭接面积
	CString StrLa0_RebarGrade = g_StrLa0.g_StrLa0_RebarGrade;			//钢筋等级
	CString StrLa0_RebarDiameter = g_StrLa0.g_StrLa0_RebarDiameter;		//钢筋直径
	CString StrLa0_SeismicGrade = g_StrLa0.g_StrLa0_SeismicGrade;		//抗震等级
	double  db_La0Value = g_StrLa0.g_db_La0Value;						//搭接长度

	// 1 ---- 创建XML DOM
	XmlDomRef domRef = NULL;
	mdlXMLDom_create(&domRef);

	// 2 ---- 创建根元素
	XmlNodeRef rootNodeRef = NULL;
	mdlXMLDom_createNode(&rootNodeRef, domRef, MDLXMLTOOLS_NODE_ELEMENT, L"MyLa0Data", L"Bentley.ECOM.Namespace");
	mdlXMLDom_setRootElement(domRef, rootNodeRef);

	// 3 ---- 添加子元素及其属性值对
	XmlNodeRef nodeRef = NULL;
	//3-1 添加保护层数据
	mdlXMLDom_addElement(&nodeRef, domRef, rootNodeRef, XMLDATATYPE_WIDESTRING, L"La0Data", L"La0Data");
	mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"Name", L"Str_La0Data");
	if (NULL != nodeRef)
	{
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"StrLa0_ConcreteGrade", StrLa0_ConcreteGrade);
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"StrLa0_OverlapArea", StrLa0_OverlapArea);
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"StrLa0_RebarGrade", StrLa0_RebarGrade);
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"StrLa0_RebarDiameter", StrLa0_RebarDiameter);
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"StrLa0_SeismicGrade", StrLa0_SeismicGrade);

		mdlXMLDomNode_free(nodeRef);
		nodeRef = NULL;
	}
	mdlXMLDom_addElement(&nodeRef, domRef, rootNodeRef, XMLDATATYPE_WIDESTRING, L"La0Value", L"La0Value");
	mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"Name", L"Str_La0Value");
	if (NULL != nodeRef)
	{
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_DOUBLE, L"db_La0Value", &db_La0Value);
		mdlXMLDomNode_free(nodeRef);
		nodeRef = NULL;
	}

	// 4 ---- 保存成磁盘上的文件
	string filepath_Template = "C:\\Template_XmlFile";
	if (access(filepath_Template.c_str(), 0) == -1)
	{
		CString Filepath(filepath_Template.c_str());
		CreateDirectory(Filepath, NULL);
	}
	CString Standardpath = L"C:\\Template_XmlFile\\";//L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\";

	CString Filepath = Standardpath + L"La0Data" + L".xml";
	if (SUCCESS == mdlXMLDom_save(domRef, FILESPEC_LOCAL, Filepath, NULL, NULL, L"utf-8", false, true, true))
	{
		mdlXMLDomNode_free(rootNodeRef);
		mdlXMLDom_free(domRef);

	}

}


//解析xml数据
void CSetLa0::displayNodeAttributes(XmlNodeRef nodeRef)
{
	XmlNodeRef         attrNodeRef = NULL;
	XmlNamedNodeMapRef nodeMapRef = NULL;
	WChar          TestName[256];
	int            maxChars = 256;
	int status = mdlXMLDomElement_getAttribute(TestName, &maxChars, nodeRef, L"Name", XMLDATATYPE_WIDESTRING);
	//读取xml文件的数据，读取到特定数据后将其保存
	WString wTestname(TestName);
	if (wTestname == L"Str_La0Data")
	{
		XmlNodeRef         attrNodeRef = NULL;
		XmlNamedNodeMapRef nodeMapRef = NULL;
		mdlXMLDomElement_getAllAttributes(&nodeMapRef, nodeRef);
		int   status = mdlXMLDomAttrList_getFirstChild(&attrNodeRef, nodeMapRef);
		while (SUCCESS == status)
		{
			int            maxChars = 256;
			WChar          attrName[256], attrVal[256];
			mdlXMLDomAttr_getName(attrName, &maxChars, attrNodeRef);
			maxChars = 256;
			mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
			WString wName(attrName);
			if (wName == L"Name")
			{
				status = mdlXMLDomAttrList_getNextChild(&attrNodeRef, nodeMapRef);
				continue;
			}
			if (wName == L"StrLa0_ConcreteGrade")
			{
				g_StrLa0.g_StrLa0_ConcreteGrade = attrVal;
			}
			else if (wName == L"StrLa0_OverlapArea")
			{
				g_StrLa0.g_StrLa0_OverlapArea = attrVal;
			}
			else if (wName == L"StrLa0_RebarGrade")
			{
				g_StrLa0.g_StrLa0_RebarGrade = attrVal;
			}
			else if (wName == L"StrLa0_RebarDiameter")
			{
				g_StrLa0.g_StrLa0_RebarDiameter = attrVal;
			}
			else if (wName == L"StrLa0_SeismicGrade")
			{
				g_StrLa0.g_StrLa0_SeismicGrade = attrVal;
			}

			status = mdlXMLDomAttrList_getNextChild(&attrNodeRef, nodeMapRef);
		}
		if (NULL != nodeMapRef)
			mdlXMLDomAttrList_free(nodeMapRef);
		if (NULL != attrNodeRef)
			mdlXMLDomNode_free(attrNodeRef);

		return;
	}
	else if (wTestname == L"Str_La0Value")
	{
		maxChars = 256;
		double La0Value;
		status = mdlXMLDomElement_getAttribute(&La0Value, &maxChars, nodeRef, L"db_La0Value", XMLDATATYPE_DOUBLE);
		g_StrLa0.g_db_La0Value = La0Value;
		return;
	}


}


//解析xml数据
void CSetLa0::ReadNode(XmlNodeRef nodeRef)
{
	int            maxChars = 256;
	WChar          nodeName[256];
	mdlXMLDomNode_getName(nodeName, &maxChars, nodeRef);
	WString nodeNameObj(nodeName);
	if (!nodeNameObj.CompareTo(L"#comment"))
		return;
	if (!nodeNameObj.CompareTo(L"#text"))
	{
		maxChars = 256;
		mdlXMLDomNode_getValue(nodeName, &maxChars, nodeRef);
		return;
	}
	displayNodeAttributes(nodeRef);

	XmlNodeRef     childNodeRef = NULL;
	XmlNodeListRef nodeListRef = NULL;
	mdlXMLDomNode_getChildNodes(&nodeListRef, nodeRef);
	int   status = mdlXMLDomNodeList_getFirstChild(&childNodeRef, nodeListRef);
	while (SUCCESS == status)
	{
		ReadNode(childNodeRef);
		status = mdlXMLDomNodeList_getNextChild(&childNodeRef, nodeListRef);
	}
	if (NULL != nodeListRef)
		mdlXMLDomNodeList_free(nodeListRef);
	if (NULL != childNodeRef)
		mdlXMLDomNode_free(childNodeRef);

}


//解析xml数据
void CSetLa0::readXML()
{
	XmlDomRef  domRef = NULL;
	mdlXMLDom_create(&domRef);
	/*loadPathResultDlgParams();
	WString tmpName = g_rebarXmlInfo.xmlName;*/
	/*int nIndex = m_cmb_RebarTemplate.GetCurSel();
	CString cmbstr;
	m_cmb_RebarTemplate.GetLBText(nIndex, cmbstr);*/
	CString Standardpath = L"C:\\Template_XmlFile\\"; //L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\";
	CString Filepath = Standardpath + L"La0Data" + L".xml";
	//WString xmlName = L"C:/ProgramData/Bentley/ProStructures CONNECT Edition/Configuration/WorkSpaces/China/Standards/ProStructures/Rebar/Codes/myXML.xml";
	WString xmlName(Filepath);
	/*if (!tmpName.empty())
	{
		xmlName += tmpName;
	}
	else
	{
		xmlName += L"RebarCode_zhongguangheC40.xml";
	}*/

	if (SUCCESS != mdlXMLDom_load(domRef, FILESPEC_LOCAL, xmlName.data(), NULL, NULL))
	{
		//mdlDialog_dmsgsPrint(L"mdlXMLDom_load error");
		return;
	}

	XmlNodeRef rootRef = NULL;
	mdlXMLDom_getRootElement(&rootRef, domRef);
	ReadNode(rootRef);

	if (NULL != rootRef)
		mdlXMLDomNode_free(rootRef);
	if (NULL != domRef)
		mdlXMLDom_free(domRef);

}