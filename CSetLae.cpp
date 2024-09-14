// CSetLae.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CSetLae.h"
#include "afxdialogex.h"
#include "resource.h"
#include "CommonFile.h"

StrLae g_StrLae = { _T("一、二级") ,L"C25" ,L"HPB300" ,_T("<=25") ,39};
//g_StrLae.g_StrLae_ConcreteGrade = L"C25";
//g_StrLae.g_StrLae_RebarDiameter = _T("<=25");
//g_StrLae.g_StrLae_RebarGrade = L"HPB300";
//g_StrLae.g_StrLae_SeismicGrade = _T("一、二级");
// CSetLae 对话框

IMPLEMENT_DYNAMIC(CSetLae, CDialogEx)

CSetLae::CSetLae(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SetLae, pParent)
{
	m_Condition = 0;
	m_Str_SeismicGrade = _T("一、二级");
	m_Str_RebarDiameter = _T("<=25");
	m_Str_RebarGrade = L"HPB300";
	m_Str_ConcreteGrade = L"C25";
	m_Sel_ConcreteGrade = 0;
	m_data = L"39";

}

CSetLae::~CSetLae()
{
}

void CSetLae::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SeismicGrade, m_SeismicGrade);
	DDX_Control(pDX, IDC_ConcreteGrade, m_ConcreteGrade);
	DDX_Control(pDX, IDC_RebarGrade, m_RebarGrade);
	DDX_Control(pDX, IDC_RebarDiameter, m_RebarDiameter);
	DDX_Control(pDX, IDC_LIST2, m_Lae_ListCtrl);
}


BEGIN_MESSAGE_MAP(CSetLae, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSetLae::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_SeismicGrade, &CSetLae::OnCbnSelchangeSeismicgrade)
	ON_CBN_SELCHANGE(IDC_ConcreteGrade, &CSetLae::OnCbnSelchangeConcretegrade)
	ON_CBN_SELCHANGE(IDC_RebarGrade, &CSetLae::OnCbnSelchangeRebargrade)
	ON_CBN_SELCHANGE(IDC_RebarDiameter, &CSetLae::OnCbnSelchangeRebardiameter)
END_MESSAGE_MAP()


// CSetLae 消息处理程序


void CSetLae::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}

//初始化ComboBox的值
void CSetLae::InitCComboBox()
{
	//混凝土等级
	m_ConcreteGrade.AddString(_T("C25"));
	m_ConcreteGrade.SetCurSel(0);
	m_ConcreteGrade.AddString(_T("C30"));
	m_ConcreteGrade.AddString(_T("C35"));
	m_ConcreteGrade.AddString(_T("C40"));
	m_ConcreteGrade.AddString(_T("C45"));
	m_ConcreteGrade.AddString(_T("C50"));
	m_ConcreteGrade.AddString(_T("C55"));
	m_ConcreteGrade.AddString(_T(">= C60"));
	
	SetDlgItemText(IDC_ConcreteGrade, _T("C25"));


	//钢筋等级
	m_RebarGrade.AddString(_T("HPB300"));
	m_RebarGrade.SetCurSel(0);
	m_RebarGrade.AddString(_T("HRB400"));
	m_RebarGrade.AddString(_T("HRB500"));
	
	SetDlgItemText(IDC_RebarGrade, _T("HPB300"));

	//钢筋直径
	m_RebarDiameter.AddString(_T("<=25"));
	m_RebarDiameter.SetCurSel(0);
	m_RebarDiameter.AddString(_T(">25"));
	
	SetDlgItemText(IDC_RebarDiameter, _T("<=25"));

	//抗震等级
	m_SeismicGrade.AddString(_T("一、二级"));
	m_SeismicGrade.SetCurSel(0);
	m_SeismicGrade.AddString(_T("三级"));
	
	SetDlgItemText(IDC_SeismicGrade, _T("一、二级"));
}

void CSetLae::UpdateComboBox()
{
	if (g_StrLae.g_StrLae_ConcreteGrade != L"")
	{
		SetDlgItemText(IDC_ConcreteGrade, g_StrLae.g_StrLae_ConcreteGrade);
		if (g_StrLae.g_StrLae_ConcreteGrade == L"C25")
			m_Sel_ConcreteGrade = 0;
		else if (g_StrLae.g_StrLae_ConcreteGrade == L"C30")
			m_Sel_ConcreteGrade = 1;
		else if (g_StrLae.g_StrLae_ConcreteGrade == L"C35")
			m_Sel_ConcreteGrade = 2;
		else if (g_StrLae.g_StrLae_ConcreteGrade == L"C40")
			m_Sel_ConcreteGrade = 3;
		else if (g_StrLae.g_StrLae_ConcreteGrade == L"C45")
			m_Sel_ConcreteGrade = 4;
		else if (g_StrLae.g_StrLae_ConcreteGrade == L"C50")
			m_Sel_ConcreteGrade = 5;
		else if (g_StrLae.g_StrLae_ConcreteGrade == L"C55")
			m_Sel_ConcreteGrade = 6;
		else if (g_StrLae.g_StrLae_ConcreteGrade == L">= C60")
			m_Sel_ConcreteGrade = 7;
	}

	if (g_StrLae.g_StrLae_RebarGrade != L"")
		SetDlgItemText(IDC_RebarGrade, g_StrLae.g_StrLae_RebarGrade);

	if (g_StrLae.g_StrLae_RebarDiameter != L"")
		SetDlgItemText(IDC_RebarDiameter, g_StrLae.g_StrLae_RebarDiameter);

	if (g_StrLae.g_StrLae_SeismicGrade != L"")
		SetDlgItemText(IDC_SeismicGrade, g_StrLae.g_StrLae_SeismicGrade);


	if ((g_StrLae.g_StrLae_ConcreteGrade != L"") || (g_StrLae.g_StrLae_RebarGrade != L"") ||
		(g_StrLae.g_StrLae_RebarDiameter != L"") || (g_StrLae.g_StrLae_SeismicGrade != L""))
	{
		Set_gConditionData();
		SetListData();
		UpdateLaeList();
		Save_gConcreteAndRebar_Grade();
	}
}

//根据ComboBox的值设置Condition的值
void CSetLae::SetConditionData()
{
	if ((m_Str_SeismicGrade == _T("一、二级")) && (m_Str_RebarDiameter == _T("<=25")))
	{
		m_Condition = 0;
	}
	else if ((m_Str_SeismicGrade == _T("一、二级"))  && (m_Str_RebarDiameter == _T(">25")))
	{
		m_Condition = 1;
	}
	else if ((m_Str_SeismicGrade == _T("三级"))  && (m_Str_RebarDiameter == _T("<=25")))
	{
		m_Condition = 2;
	}
	else if ((m_Str_SeismicGrade == _T("三级"))  && (m_Str_RebarDiameter == _T(">25")))
	{
		m_Condition = 3;
	}

	return;
}

void CSetLae::Set_gConditionData()
{
	if ((g_StrLae.g_StrLae_SeismicGrade == _T("一、二级")) && (g_StrLae.g_StrLae_RebarDiameter == _T("<=25")))
	{
		m_Condition = 0;
	}
	else if ((g_StrLae.g_StrLae_SeismicGrade == _T("一、二级")) && (g_StrLae.g_StrLae_RebarDiameter == _T(">25")))
	{
		m_Condition = 1;
	}
	else if ((g_StrLae.g_StrLae_SeismicGrade == _T("三级")) && (g_StrLae.g_StrLae_RebarDiameter == _T("<=25")))
	{
		m_Condition = 2;
	}
	else if ((g_StrLae.g_StrLae_SeismicGrade == _T("三级")) && (g_StrLae.g_StrLae_RebarDiameter == _T(">25")))
	{
		m_Condition = 3;
	}

	return;
}

//填充表格数据
void CSetLae::SetListData()
{
	//存第一行，每一列的数据
	vector<vector<CString>> row1;

	vector<CString> column1;
	column1.push_back(_T("39"));
	column1.push_back(_T(""));
	column1.push_back(_T("36"));
	column1.push_back(_T(""));
	row1.push_back(column1);
	vector<CString> column2;
	column2.push_back(_T("35"));
	column2.push_back(_T(""));
	column2.push_back(_T("32"));
	column2.push_back(_T(""));
	row1.push_back(column2);
	vector<CString> column3;
	column3.push_back(_T("32"));
	column3.push_back(_T(""));
	column3.push_back(_T("29"));
	column3.push_back(_T(""));
	row1.push_back(column3);
	vector<CString> column4;
	column4.push_back(_T("29"));
	column4.push_back(_T(""));
	column4.push_back(_T("26"));
	column4.push_back(_T(""));
	row1.push_back(column4);
	vector<CString> column5;
	column5.push_back(_T("28"));
	column5.push_back(_T(""));
	column5.push_back(_T("25"));
	column5.push_back(_T(""));
	row1.push_back(column5);
	vector<CString> column6;
	column6.push_back(_T("26"));
	column6.push_back(_T(""));
	column6.push_back(_T("24"));
	column6.push_back(_T(""));
	row1.push_back(column6);
	vector<CString> column7;
	column7.push_back(_T("25"));
	column7.push_back(_T(""));
	column7.push_back(_T("23"));
	column7.push_back(_T(""));
	row1.push_back(column7);
	vector<CString> column8;
	column8.push_back(_T("24"));
	column8.push_back(_T(""));
	column8.push_back(_T("22"));
	column8.push_back(_T(""));
	row1.push_back(column8);
	m_TableData[1] = row1;

	column1.clear();
	column2.clear();
	column3.clear();
	column4.clear();
	column5.clear();
	column6.clear();
	column7.clear();
	column8.clear();
	//存第二行，每一列的数据
	vector<vector<CString>> row2;

	column1.push_back(_T("46"));
	column1.push_back(_T("51"));
	column1.push_back(_T("42"));
	column1.push_back(_T("46"));
	row2.push_back(column1);
	column2.push_back(_T("40"));
	column2.push_back(_T("45"));
	column2.push_back(_T("37"));
	column2.push_back(_T("41"));
	row2.push_back(column2);
	column3.push_back(_T("37"));
	column3.push_back(_T("40"));
	column3.push_back(_T("34"));
	column3.push_back(_T("37"));
	row2.push_back(column3);
	column4.push_back(_T("33"));
	column4.push_back(_T("37"));
	column4.push_back(_T("30"));
	column4.push_back(_T("34"));
	row2.push_back(column4);
	column5.push_back(_T("32"));
	column5.push_back(_T("36"));
	column5.push_back(_T("29"));
	column5.push_back(_T("33"));
	row2.push_back(column5);
	column6.push_back(_T("31"));
	column6.push_back(_T("35"));
	column6.push_back(_T("28"));
	column6.push_back(_T("32"));
	row2.push_back(column6);
	column7.push_back(_T("30"));
	column7.push_back(_T("33"));
	column7.push_back(_T("27"));
	column7.push_back(_T("30"));
	row2.push_back(column7);
	column8.push_back(_T("29"));
	column8.push_back(_T("32"));
	column8.push_back(_T("26"));
	column8.push_back(_T("29"));
	row2.push_back(column8);
	m_TableData[2] = row2;

	column1.clear();
	column2.clear();
	column3.clear();
	column4.clear();
	column5.clear();
	column6.clear();
	column7.clear();
	column8.clear();
	//存第三行，每一列的数据
	vector<vector<CString>> row3;

	column1.push_back(_T("55"));
	column1.push_back(_T("61"));
	column1.push_back(_T("50"));
	column1.push_back(_T("56"));
	row3.push_back(column1);
	column2.push_back(_T("49"));
	column2.push_back(_T("54"));
	column2.push_back(_T("45"));
	column2.push_back(_T("49"));
	row3.push_back(column2);
	column3.push_back(_T("45"));
	column3.push_back(_T("49"));
	column3.push_back(_T("41"));
	column3.push_back(_T("45"));
	row3.push_back(column3);
	column4.push_back(_T("41"));
	column4.push_back(_T("46"));
	column4.push_back(_T("38"));
	column4.push_back(_T("42"));
	row3.push_back(column4);
	column5.push_back(_T("39"));
	column5.push_back(_T("43"));
	column5.push_back(_T("36"));
	column5.push_back(_T("39"));
	row3.push_back(column5);
	column6.push_back(_T("37"));
	column6.push_back(_T("40"));
	column6.push_back(_T("34"));
	column6.push_back(_T("37"));
	row3.push_back(column6);
	column7.push_back(_T("36"));
	column7.push_back(_T("39"));
	column7.push_back(_T("33"));
	column7.push_back(_T("36"));
	row3.push_back(column7);
	column8.push_back(_T("35"));
	column8.push_back(_T("38"));
	column8.push_back(_T("32"));
	column8.push_back(_T("35"));
	row3.push_back(column8);
	m_TableData[3] = row3;

	return;

}

//刷新表格数据
void CSetLae::UpdateLaeList()
{
	m_Lae_ListCtrl.DeleteAllItems();
	//分成三行一行一行的生成表格
	//第一行
	m_Lae_ListCtrl.InsertItem(0, _T(""));
	CString strValue = _T("HPB300");
	m_Lae_ListCtrl.SetItemText(0, 0, strValue);
	for (int i = 1; i < 9; ++i)
	{
		CString strValue = _T("");
		strValue = m_TableData[1].at(i - 1).at(m_Condition);
		m_Lae_ListCtrl.SetItemText(0, i, strValue);
	}

	//第二行
	m_Lae_ListCtrl.InsertItem(1, _T(""));
	strValue = _T("HRB400");
	m_Lae_ListCtrl.SetItemText(1, 0, strValue);
	for (int i = 1; i < 9; ++i)
	{
		CString strValue = _T("");
		strValue = m_TableData[2].at(i - 1).at(m_Condition);
		m_Lae_ListCtrl.SetItemText(1, i, strValue);
	}

	//第三行
	m_Lae_ListCtrl.InsertItem(2, _T(""));
	strValue = _T("HRB500");
	m_Lae_ListCtrl.SetItemText(2, 0, strValue);
	for (int i = 1; i < 9; ++i)
	{
		CString strValue = _T("");
		strValue = m_TableData[3].at(i - 1).at(m_Condition);
		m_Lae_ListCtrl.SetItemText(2, i, strValue);
	}

	return;

}


BOOL CSetLae::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//1、初始化四个CComboBox按钮的值
	InitCComboBox();

	//2、设置列表属性并在列表控件中插入列
	LONG lStyle;
	lStyle = GetWindowLong(m_Lae_ListCtrl.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_Lae_ListCtrl.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_Lae_ListCtrl.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_Lae_ListCtrl.SetExtendedStyle(dwStyle);			// 设置扩展风格

	tagRECT stRect;
	m_Lae_ListCtrl.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_Lae_ListCtrl.InsertColumn(0, _T(""), (int)(width / 9.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_Lae_ListCtrl.InsertColumn(1, _T("C25"), (int)(width / 9.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_Lae_ListCtrl.InsertColumn(2, _T("C30"), (int)(width / 9.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_Lae_ListCtrl.InsertColumn(3, _T("C35"), (int)(width / 9.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_Lae_ListCtrl.InsertColumn(4, _T("C40"), (int)(width / 9.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_Lae_ListCtrl.InsertColumn(5, _T("C45"), (int)(width / 9.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_Lae_ListCtrl.InsertColumn(6, _T("C50"), (int)(width / 9.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_Lae_ListCtrl.InsertColumn(7, _T("C55"), (int)(width / 9.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);
	m_Lae_ListCtrl.InsertColumn(8, _T(">= C60"), (int)(width / 9.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByDigit);

	//3、根据CComboBox的值设置m_Condition
	SetConditionData();

	//4、填写表格的值存到 m_TableData
	SetListData();

	//5、根据m_Condition的值设置表格的数据
	UpdateLaeList();

	//6、 读取XML数据
	readXML();
	
	//7、根据全局变量设置的值刷新到上一次设置的表格
	UpdateComboBox();

	return TRUE;
}


void CSetLae::OnCbnSelchangeSeismicgrade()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSel;
	//获取组合框控件的列表框中选中项的索引
	nSel = m_SeismicGrade.GetCurSel();
	//根据选中索引项获取该项字符串
	m_SeismicGrade.GetLBText(nSel, m_Str_SeismicGrade);

	g_StrLae.g_StrLae_SeismicGrade = m_Str_SeismicGrade;

	//SetConditionData();
	Set_gConditionData();
	UpdateLaeList();
	//SaveConcreteAndRebar_Grade();
	Save_gConcreteAndRebar_Grade();

	WriteXml();

	return;

}


void CSetLae::OnCbnSelchangeConcretegrade()
{
	// TODO: 在此添加控件通知处理程序代码
	//获取组合框控件的列表框中选中项的索引
	m_Sel_ConcreteGrade = m_ConcreteGrade.GetCurSel();
	//根据选中索引项获取该项字符串
	m_ConcreteGrade.GetLBText(m_Sel_ConcreteGrade, m_Str_ConcreteGrade);

	/*SetConditionData();
	UpdateLaeList();*/
	g_StrLae.g_StrLae_ConcreteGrade = m_Str_ConcreteGrade;
	Set_gConditionData();
	//SaveConcreteAndRebar_Grade();
	Save_gConcreteAndRebar_Grade();

	WriteXml();

	return;

}


void CSetLae::OnCbnSelchangeRebargrade()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSel;
	//获取组合框控件的列表框中选中项的索引
	nSel = m_RebarGrade.GetCurSel();
	//根据选中索引项获取该项字符串
	m_RebarGrade.GetLBText(nSel, m_Str_RebarGrade);

	/*SetConditionData();
	UpdateLaeList();*/
	g_StrLae.g_StrLae_RebarGrade = m_Str_RebarGrade;
	Set_gConditionData();
	//SaveConcreteAndRebar_Grade();
	Save_gConcreteAndRebar_Grade();

	WriteXml();

	return;

}


void CSetLae::OnCbnSelchangeRebardiameter()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSel;
	//获取组合框控件的列表框中选中项的索引
	nSel = m_RebarDiameter.GetCurSel();
	//根据选中索引项获取该项字符串
	m_RebarDiameter.GetLBText(nSel, m_Str_RebarDiameter);

	g_StrLae.g_StrLae_RebarDiameter = m_Str_RebarDiameter;
	//SetConditionData();
	Set_gConditionData();
	UpdateLaeList();
	//SaveConcreteAndRebar_Grade();
	Save_gConcreteAndRebar_Grade();

	WriteXml();

	return;

}

//保存表格里面的值
void CSetLae::SaveConcreteAndRebar_Grade()
{
	if (m_Str_RebarGrade == L"HPB300")
	{
		m_data = m_TableData[1].at(m_Sel_ConcreteGrade).at(m_Condition);
	}
	else if (m_Str_RebarGrade == L"HRB400")
	{
		m_data = m_TableData[2].at(m_Sel_ConcreteGrade).at(m_Condition);
	}
	else if (m_Str_RebarGrade == L"HRB500")
	{
		m_data = m_TableData[3].at(m_Sel_ConcreteGrade).at(m_Condition);
	}

	return;
}

void CSetLae::Save_gConcreteAndRebar_Grade()
{
	if (g_StrLae.g_StrLae_RebarGrade == L"HPB300")
	{
		g_StrLae.g_db_LaeValue = _ttof(m_TableData[1].at(m_Sel_ConcreteGrade).at(m_Condition));
	}
	else if (g_StrLae.g_StrLae_RebarGrade == L"HRB400")
	{
		g_StrLae.g_db_LaeValue = _ttof(m_TableData[2].at(m_Sel_ConcreteGrade).at(m_Condition));
	}
	else if (g_StrLae.g_StrLae_RebarGrade == L"HRB500")
	{
		g_StrLae.g_db_LaeValue = _ttof(m_TableData[3].at(m_Sel_ConcreteGrade).at(m_Condition));
	}

	return;

}


//将参数值写入到XML文件中
void CSetLae::WriteXml()
{
	CString StrLae_SeismicGrade = g_StrLae.g_StrLae_SeismicGrade;		//抗震等级
	CString StrLae_ConcreteGrade = g_StrLae.g_StrLae_ConcreteGrade;		//混凝土等级
	CString StrLae_RebarGrade = g_StrLae.g_StrLae_RebarGrade;			//钢筋等级
	CString StrLae_RebarDiameter = g_StrLae.g_StrLae_RebarDiameter;		//钢筋直径
	double db_LaeValue = g_StrLae.g_db_LaeValue;						//锚固长度

	// 1 ---- 创建XML DOM
	XmlDomRef domRef = NULL;
	mdlXMLDom_create(&domRef);

	// 2 ---- 创建根元素
	XmlNodeRef rootNodeRef = NULL;
	mdlXMLDom_createNode(&rootNodeRef, domRef, MDLXMLTOOLS_NODE_ELEMENT, L"MyLaeData", L"Bentley.ECOM.Namespace");
	mdlXMLDom_setRootElement(domRef, rootNodeRef);

	// 3 ---- 添加子元素及其属性值对
	XmlNodeRef nodeRef = NULL;
	//3-1 添加保护层数据
	mdlXMLDom_addElement(&nodeRef, domRef, rootNodeRef, XMLDATATYPE_WIDESTRING, L"LaeData", L"LaeData");
	mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"Name", L"Str_LaeData");
	if (NULL != nodeRef)
	{
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"StrLae_SeismicGrade", StrLae_SeismicGrade);
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"StrLae_ConcreteGrade", StrLae_ConcreteGrade);
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"StrLae_RebarGrade", StrLae_RebarGrade);
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"StrLae_RebarDiameter", StrLae_RebarDiameter);

		mdlXMLDomNode_free(nodeRef);
		nodeRef = NULL;
	}
	mdlXMLDom_addElement(&nodeRef, domRef, rootNodeRef, XMLDATATYPE_WIDESTRING, L"LaeValue", L"LaeValue");
	mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"Name", L"Str_LaeValue");
	if (NULL != nodeRef)
	{
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_DOUBLE, L"db_LaeValue", &db_LaeValue);
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
	CString Standardpath = L"C:\\Template_XmlFile\\";
	//CString Standardpath = L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\";

	CString Filepath = Standardpath + L"LaeData" + L".xml";
	if (SUCCESS == mdlXMLDom_save(domRef, FILESPEC_LOCAL, Filepath, NULL, NULL, L"utf-8", false, true, true))
	{
		mdlXMLDomNode_free(rootNodeRef);
		mdlXMLDom_free(domRef);

	}

}


//解析xml数据
void CSetLae::displayNodeAttributes(XmlNodeRef nodeRef)
{
	XmlNodeRef         attrNodeRef = NULL;
	XmlNamedNodeMapRef nodeMapRef = NULL;
	WChar          TestName[256];
	int            maxChars = 256;
	int status = mdlXMLDomElement_getAttribute(TestName, &maxChars, nodeRef, L"Name", XMLDATATYPE_WIDESTRING);
	//读取xml文件的数据，读取到特定数据后将其保存
	WString wTestname(TestName);
	if (wTestname == L"Str_LaeData")
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
			if (wName == L"StrLae_SeismicGrade")
			{
				g_StrLae.g_StrLae_SeismicGrade = attrVal;
			}
			else if (wName == L"StrLae_ConcreteGrade")
			{
				g_StrLae.g_StrLae_ConcreteGrade = attrVal;
			}
			else if (wName == L"StrLae_RebarGrade")
			{
				g_StrLae.g_StrLae_RebarGrade = attrVal;
			}
			else if (wName == L"StrLae_RebarDiameter")
			{
				g_StrLae.g_StrLae_RebarDiameter = attrVal;
			}

			status = mdlXMLDomAttrList_getNextChild(&attrNodeRef, nodeMapRef);
		}
		if (NULL != nodeMapRef)
			mdlXMLDomAttrList_free(nodeMapRef);
		if (NULL != attrNodeRef)
			mdlXMLDomNode_free(attrNodeRef);

		return;
	}
	else if (wTestname == L"Str_LaeValue")
	{
		maxChars = 256;
		double LaeValue;
		status = mdlXMLDomElement_getAttribute(&LaeValue, &maxChars, nodeRef, L"db_LaeValue", XMLDATATYPE_DOUBLE);
		g_StrLae.g_db_LaeValue = LaeValue;
		return;
	}
	

}


//解析xml数据
void CSetLae::ReadNode(XmlNodeRef nodeRef)
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
void CSetLae::readXML()
{
	XmlDomRef  domRef = NULL;
	mdlXMLDom_create(&domRef);
	/*loadPathResultDlgParams();
	WString tmpName = g_rebarXmlInfo.xmlName;*/
	/*int nIndex = m_cmb_RebarTemplate.GetCurSel();
	CString cmbstr;
	m_cmb_RebarTemplate.GetLBText(nIndex, cmbstr);*/
	CString Standardpath = L"C:\\Template_XmlFile\\";//L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\";
	CString Filepath = Standardpath + L"LaeData" + L".xml";
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
