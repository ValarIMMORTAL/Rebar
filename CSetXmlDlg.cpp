// CSetXmlDlg.cpp: 实现文件
//
#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CSetXmlDlg.h"
#include "afxdialogex.h"
#include "ElementAttribute.h"
#include "resource.h"
#include "SetParam.h"
#include "XmlHelper.h"

extern RebarXmlInfo g_rebarXmlInfo;

// CSetXmlDlg 对话框

IMPLEMENT_DYNAMIC(CSetXmlDlg, CDialogEx)

CSetXmlDlg::CSetXmlDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_SetXml, pParent)
{

}

CSetXmlDlg::~CSetXmlDlg()
{
}

BOOL CSetXmlDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	int i = 0;
	for (auto var : m_listFileName)
	{
		m_comboXmlName.InsertString(i, var);
		if (i == 0)
		{
			m_xmlFileName = WString(var);
		}
		++i;
	}
	m_comboXmlName.SetCurSel(0);
	
	loadPathResultDlgParams();
	m_xmlFileName = g_rebarXmlInfo.xmlName;
	int nIndex = m_comboXmlName.FindStringExact(0, m_xmlFileName.data());
	m_comboXmlName.SetCurSel(nIndex);

	return TRUE;
}

void CSetXmlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_XmlFileName, m_comboXmlName);
}


BEGIN_MESSAGE_MAP(CSetXmlDlg, CDialogEx)
	ON_CBN_SELCHANGE(IDC_COMBO_XmlFileName, &CSetXmlDlg::OnCbnSelchangeComboXmlfilename)
	ON_BN_CLICKED(IDOK, &CSetXmlDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CSetXmlDlg 消息处理程序


void CSetXmlDlg::OnCbnSelchangeComboXmlfilename()
{
	auto it = m_listFileName.begin();
	advance(it, m_comboXmlName.GetCurSel());
	m_xmlFileName = WString(*it);
}


void CSetXmlDlg::OnBnClickedOk()
{
	wmemcpy(g_rebarXmlInfo.xmlName, m_xmlFileName.GetWCharCP(), 512);
	savePathResultDlgParams();
	readXML();
	CDialogEx::OnOK();
}
