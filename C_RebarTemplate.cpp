// C_RebarTemplate.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "C_RebarTemplate.h"
#include "afxdialogex.h"
#include "resource.h"
#include "CommonFile.h"
#include "CWallMainRebarDlg.h"
#include "GallerySettingsMainRebarPage.h"
#include "CFacesMainRebarDlg.h"
#include "CFacesMainRebarDlgEx.h"
#include "CatchpitMainRebarDlg.h"
#include <Windows.h>

using namespace Gallery;

extern CWallMainRebarDlg* g_wallMainDlg;
extern CFacesMainRebarDlg* g_faceMainDlg;
extern CFacesMainRebarDlgEx* g_faceMainDlgEx;
extern CatchpitMainRebarDlg * g_CatchpitMainDlg;
//extern SettingsMainRebarPage* g_settingMaindlg;

// C_RebarTemplate 对话框

IMPLEMENT_DYNAMIC(C_RebarTemplate, CDialogEx)

C_RebarTemplate::C_RebarTemplate(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_RebarTemplate, pParent)
{
	isWall = false;
	isFloor = false;
	isFace = false;
	m_rebarNum = 0;
}

C_RebarTemplate::~C_RebarTemplate()
{
	m_Get_dlgData = { 0,0,0,0,0 };
	m_Get_vecRebarData.clear();

}

BOOL C_RebarTemplate::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// 如果在malapps中文件夹不存在就创建存放配筋模板的文件夹
	string filepath_Template = "C:\\Template_XmlFile";
	if (access(filepath_Template.c_str(), 0) == -1)
	{
		CString Filepath(filepath_Template.c_str());
		CreateDirectory(Filepath, NULL);
	}
	string filepath_Wall = "C:\\Template_XmlFile\\Wall_template";//"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Wall_template";
	string filepath_Floor = "C:\\Template_XmlFile\\Floor_template";//"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Floor_template";
	string filepath_Face = "C:\\Template_XmlFile\\Face_template";//"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Face_template";
	if (access(filepath_Wall.c_str(), 0) == -1)//返回值为-1表示不存在，需要创建
	{
		CString Filepath_Wall(filepath_Wall.c_str());
		CreateDirectory(Filepath_Wall, NULL);
	}
	if (access(filepath_Floor.c_str(), 0) == -1)//返回值为-1表示不存在，需要创建
	{
		CString Filepath_Floor(filepath_Floor.c_str());
		CreateDirectory(Filepath_Floor, NULL);
	}
	if (access(filepath_Face.c_str(), 0) == -1)//返回值为-1表示不存在，需要创建
	{
		CString Filepath_Face(filepath_Face.c_str());
		CreateDirectory(Filepath_Face, NULL);
	}

	// 默认是上一次保存的名字
	if (nullptr != g_wallMainDlg)
	{
		if (g_wallMainDlg->m_templateName != L"")
		{
			m_Str_TemplateName = g_wallMainDlg->m_templateName;
			SetDlgItemText(IDC_EDIT_TemplateName, m_Str_TemplateName);
		}
			
	}
	else if (nullptr != g_faceMainDlg)
	{
		if (g_faceMainDlg->m_templateName != L"")
		{
			m_Str_TemplateName = g_faceMainDlg->m_templateName;
			SetDlgItemText(IDC_EDIT_TemplateName, m_Str_TemplateName);
		}
	}
	else if (nullptr != g_CatchpitMainDlg)
	{
		if (g_CatchpitMainDlg->m_templateName != L"")
		{
			m_Str_TemplateName = g_CatchpitMainDlg->m_templateName;
			SetDlgItemText(IDC_EDIT_TemplateName, m_Str_TemplateName);
		}
	}

	string filepath = "";
	if (isWall)
	{
		filepath = filepath_Wall;//"C:\\ProgramData\\CGNGalleryRebar\\PS";
	}
	else if (isFloor)
	{
		filepath = filepath_Floor;// "C:\\ProgramData\\CGNGalleryRebar";
	}
	else if (isFace)
	{
		filepath = filepath_Face;// "C:\\ProgramData\\CGNGalleryRebar\\Dgnlib";
	}
	 
	Search_XmlFile(filepath, m_vec_xmlfiles, ".xml");
	for (auto xmlfile : m_vec_xmlfiles)
	{
		auto filename = xmlfile.substr(0, xmlfile.size()-4);
		CString xmlfileName(filename.c_str());
		if (xmlfileName == L"config")
			continue;
		if (xmlfileName == L"La0Data")
			continue;
		if (xmlfileName == L"LaeData")
			continue;
		m_vec_cmblist.push_back(xmlfileName);
	}
	m_vec_xmlfiles.clear();
	sort(m_vec_cmblist.begin(), m_vec_cmblist.end());
	vector<CString>::iterator iter = unique(m_vec_cmblist.begin(), m_vec_cmblist.end());
	m_vec_cmblist.erase(iter, m_vec_cmblist.end());
	for (auto xmlfileName : m_vec_cmblist)
	{
		m_cmb_RebarTemplate.AddString(xmlfileName);
	}
	m_cmb_RebarTemplate.SetCurSel(0);

	return TRUE;
}

void C_RebarTemplate::Set_m_vecRebarData(vector<PIT::ConcreteRebar> vecRebarData)
{
	m_vecRebarData = vecRebarData;
}

void C_RebarTemplate::Set_m_dlgData(PIT::DlgData dlgData)
{
	m_dlgData = dlgData;
}

void C_RebarTemplate::Write_xmlData(PIT::DlgData dlgData, std::vector<PIT::ConcreteRebar> vecRebarData)
{
	double  postiveCover = dlgData.postiveCover;		//顶部保护层
	double  sideCover = dlgData.sideCover;				//侧面保护层
	double  reverseCover = dlgData.reverseCover;		//底部保护层
	double  missHoleSize = dlgData.missHoleSize;		//忽略尺寸
	int		rebarLevelNum = dlgData.rebarLevelNum;		//钢筋层数

	// 1 ---- 创建XML DOM
	XmlDomRef domRef = NULL;
	mdlXMLDom_create(&domRef);

	// 2 ---- 创建根元素
	XmlNodeRef rootNodeRef = NULL;
	mdlXMLDom_createNode(&rootNodeRef, domRef, MDLXMLTOOLS_NODE_ELEMENT, L"RebarTemplate", L"Bentley.ECOM.Namespace");
	mdlXMLDom_setRootElement(domRef, rootNodeRef);

	// 3 ---- 添加子元素及其属性值对
	XmlNodeRef nodeRef = NULL;
	//3-1 添加保护层数据
	mdlXMLDom_addElement(&nodeRef, domRef, rootNodeRef, XMLDATATYPE_WIDESTRING, L"DlgData", L"ProtectiveLayer");
	mdlXMLDomElement_removeAttribute(nodeRef, L"xmlns");
	mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"Name", L"ProtectiveLayer");
	if (NULL != nodeRef)
	{
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_DOUBLE, L"postiveCover", &postiveCover);
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_DOUBLE, L"sideCover", &sideCover);
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_DOUBLE, L"reverseCover", &reverseCover);
		mdlXMLDomNode_free(nodeRef);
		nodeRef = NULL;
	}
	mdlXMLDom_addElement(&nodeRef, domRef, rootNodeRef, XMLDATATYPE_WIDESTRING, L"DlgData", L"MissHoleSize");
	mdlXMLDomElement_removeAttribute(nodeRef, L"xmlns");
	mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"Name", L"MissHoleSize");
	if (NULL != nodeRef)
	{
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_DOUBLE, L"missHoleSize", &missHoleSize);
	}
	mdlXMLDom_addElement(&nodeRef, domRef, rootNodeRef, XMLDATATYPE_WIDESTRING, L"DlgData", L"RebarLevelNum");
	mdlXMLDomElement_removeAttribute(nodeRef, L"xmlns");
	mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"Name", L"RebarLevelNum");
	if (NULL != nodeRef)
	{
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_INT32, L"rebarLevelNum", &rebarLevelNum);
	}

	for (int i = 0; i < rebarLevelNum; ++i)
	{
		string rebarSize;
		rebarSize = m_vecRebarData[i].rebarSize;
		CString tmpstr;
		tmpstr.Format(_T("%d"), i);
		CString rebarLevel = _T("RebarLevel") + tmpstr;
		mdlXMLDom_addElement(&nodeRef, domRef, rootNodeRef, XMLDATATYPE_WIDESTRING, L"DlgData", rebarLevel);
		mdlXMLDomElement_removeAttribute(nodeRef, L"xmlns");
		mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_WIDESTRING, L"Name", rebarLevel);
		if (NULL != nodeRef)
		{
			mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_INT32, L"rebarLevel", &m_vecRebarData[i].rebarLevel);
			mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_INT32, L"rebarDir", &m_vecRebarData[i].rebarDir);
			mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_STRING, L"rebarSize", &rebarSize);
			mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_INT32, L"rebarType", &m_vecRebarData[i].rebarType);
			mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_DOUBLE, L"spacing", &m_vecRebarData[i].spacing); 
			mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_DOUBLE, L"startOffset", &m_vecRebarData[i].startOffset);
			mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_DOUBLE, L"endOffset", &m_vecRebarData[i].endOffset);
			mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_DOUBLE, L"levelSpace", &m_vecRebarData[i].levelSpace);
			mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_INT32, L"datachange", &m_vecRebarData[i].datachange);
			mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_INT32, L"rebarLineStyle", &m_vecRebarData[i].rebarLineStyle);
			mdlXMLDomElement_addAttribute(nodeRef, XMLDATATYPE_INT32, L"rebarWeight", &m_vecRebarData[i].rebarWeight);

			mdlXMLDomNode_free(nodeRef);
			nodeRef = NULL;
		}
	}


	// 4 ---- 保存成磁盘上的文件
	CString filepath_Wall = L"C:\\Template_XmlFile\\Wall_template\\"; //L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Wall_template\\";
	CString filepath_Floor = L"C:\\Template_XmlFile\\Floor_template\\";//L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Floor_template\\";
	CString filepath_Face = L"C:\\Template_XmlFile\\Face_template\\";//L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Face_template\\";

	CString Standardpath;
	if (isWall)
	{
		Standardpath = filepath_Wall;// L"C:\\ProgramData\\CGNGalleryRebar\\PS\\";
	}
	if (isFloor)
	{
		Standardpath = filepath_Floor;// L"C:\\ProgramData\\CGNGalleryRebar\\";
	}
	if (isFace)
	{
		Standardpath = filepath_Face;// L"C:\\ProgramData\\CGNGalleryRebar\\Dgnlib\\";
	}
	CString Filepath = Standardpath + m_Str_TemplateName + L".xml";
	if (SUCCESS == mdlXMLDom_save(domRef, FILESPEC_LOCAL, Filepath, NULL, NULL, L"utf-8", false, true, true))
	{
		m_cmb_RebarTemplate.AddString(m_Str_TemplateName);
		SetDlgItemText(IDC_template, m_Str_TemplateName);

		mdlXMLDomNode_free(rootNodeRef);
		mdlXMLDom_free(domRef);

	}


}


void C_RebarTemplate::displayNodeAttributes(XmlNodeRef nodeRef)
{
	XmlNodeRef         attrNodeRef = NULL;
	XmlNamedNodeMapRef nodeMapRef = NULL;
	WChar          TestName[256];
	int            maxChars = 256;
	int status = mdlXMLDomElement_getAttribute(TestName, &maxChars, nodeRef, L"Name", XMLDATATYPE_WIDESTRING);
	//读取xml文件的数据，读取到特定数据后将其保存
	WString wTestname(TestName);
	if (wTestname == L"ProtectiveLayer")
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
			double dValue = _ttof(attrVal);
			if (wName == L"Name")
			{
				status = mdlXMLDomAttrList_getNextChild(&attrNodeRef, nodeMapRef);
				continue;
			}
			if (wName == L"postiveCover")
			{
				m_Get_dlgData.postiveCover = dValue;
			}
			else if (wName == L"sideCover")
			{
				m_Get_dlgData.sideCover = dValue;
			}
			else if (wName == L"reverseCover")
			{
				m_Get_dlgData.reverseCover = dValue;
			}

			status = mdlXMLDomAttrList_getNextChild(&attrNodeRef, nodeMapRef);
		}
		if (NULL != nodeMapRef)
			mdlXMLDomAttrList_free(nodeMapRef);
		if (NULL != attrNodeRef)
			mdlXMLDomNode_free(attrNodeRef);

		return;
	}
	else if (wTestname == L"MissHoleSize")
	{
		maxChars = 256;
		double missHoleSize;
		status = mdlXMLDomElement_getAttribute(&missHoleSize, &maxChars, nodeRef, L"missHoleSize", XMLDATATYPE_DOUBLE);
		m_Get_dlgData.missHoleSize = missHoleSize;
		return;
	}
	else if (wTestname == L"RebarLevelNum")
	{
		maxChars = 256;
		int rebarLevelNum;
		status = mdlXMLDomElement_getAttribute(&rebarLevelNum, &maxChars, nodeRef, L"rebarLevelNum", XMLDATATYPE_INT32);
		m_Get_dlgData.rebarLevelNum = rebarLevelNum;
		m_rebarNum = rebarLevelNum;
		return;
	}
	if (m_rebarNum > 0)
	{
		for (int i = 0; i < m_rebarNum; ++i)
		{
			CString tmpstr;
			tmpstr.Format(_T("%d"), i);
			CString cname = L"RebarLevel" + tmpstr;
			WString wname(cname);
			if (wTestname == wname)
			{
				PIT::ConcreteRebar oneRebar;
				XmlNodeRef         attrNodeRef = NULL;
				XmlNamedNodeMapRef nodeMapRef = NULL;
				mdlXMLDomElement_getAllAttributes(&nodeMapRef, nodeRef);
				int   status = mdlXMLDomAttrList_getFirstChild(&attrNodeRef, nodeMapRef);
				while (SUCCESS == status)
				{
					int            maxChars = 256;
					WChar          attrName[256];
					mdlXMLDomAttr_getName(attrName, &maxChars, attrNodeRef);

					WString wName(attrName);

					if (wName == L"Name")
					{
						status = mdlXMLDomAttrList_getNextChild(&attrNodeRef, nodeMapRef);
						continue;
					}
					if (wName == L"rebarLevel")
					{
						int            maxChars = 256;
						WChar          attrVal[256];
						mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
						int dValue = _ttoi(attrVal);
						oneRebar.rebarLevel = dValue;
					}
					else if (wName == L"rebarDir")
					{
						int            maxChars = 256;
						WChar          attrVal[256];
						mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
						int dValue = _ttoi(attrVal);
						oneRebar.rebarDir = dValue;
					}
					else if (wName == L"rebarSize")
					{
						int            maxChars = 256;
						WChar          attrVal[256];
						mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
						char rebarsize[512] = { 0 };
						wcstombs(rebarsize, attrVal, 512);
						memcpy(oneRebar.rebarSize, rebarsize, 512);

					}
					else if (wName == L"rebarType")
					{
						int            maxChars = 256;
						WChar          attrVal[256];
						mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
						int dValue = _ttoi(attrVal);
						oneRebar.rebarType = dValue;
					}
					else if (wName == L"spacing")
					{
						int            maxChars = 256;
						WChar          attrVal[256];
						mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
						double dValue = _ttof(attrVal);
						oneRebar.spacing = dValue;
					}
					else if (wName == L"startOffset")
					{
						int            maxChars = 256;
						WChar          attrVal[256];
						mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
						double dValue = _ttof(attrVal);
						oneRebar.startOffset = dValue;
					}
					else if (wName == L"endOffset")
					{
						int            maxChars = 256;
						WChar          attrVal[256];
						mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
						double dValue = _ttof(attrVal);
						oneRebar.endOffset = dValue;
					}
					else if (wName == L"levelSpace")
					{
						int            maxChars = 256;
						WChar          attrVal[256];
						mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
						double dValue = _ttof(attrVal);
						oneRebar.levelSpace = dValue;
					}
					else if (wName == L"datachange")
					{
						int            maxChars = 256;
						WChar          attrVal[256];
						mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
						int dValue = _ttoi(attrVal);
						oneRebar.datachange = dValue;
					}
					else if (wName == L"rebarLineStyle")
					{
						int            maxChars = 256;
						WChar          attrVal[256];
						mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
						int dValue = _ttoi(attrVal);
						oneRebar.rebarLineStyle = dValue;
					}
					else if (wName == L"rebarWeight")
					{
					int            maxChars = 256;
					WChar          attrVal[256];
					mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
					int dValue = _ttoi(attrVal);
					oneRebar.rebarWeight = dValue;
					}


					status = mdlXMLDomAttrList_getNextChild(&attrNodeRef, nodeMapRef);
				}
				m_Get_vecRebarData.push_back(oneRebar);
				if (NULL != nodeMapRef)
					mdlXMLDomAttrList_free(nodeMapRef);
				if (NULL != attrNodeRef)
					mdlXMLDomNode_free(attrNodeRef);
			}
		}
	}
	
}

void C_RebarTemplate::ReadNode(XmlNodeRef nodeRef)
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

void C_RebarTemplate::readXML()
{
	//memset(&g_globalpara, sizeof(GlobalParameters), 0);
	XmlDomRef  domRef = NULL;
	mdlXMLDom_create(&domRef);
	/*loadPathResultDlgParams();
	WString tmpName = g_rebarXmlInfo.xmlName;*/
	/*int nIndex = m_cmb_RebarTemplate.GetCurSel();
	CString cmbstr;
	m_cmb_RebarTemplate.GetLBText(nIndex, cmbstr);*/
	CString cmbstr;
	GetDlgItemText(IDC_template, cmbstr);
	if (cmbstr.GetLength() == 0)
	{
		mdlDialog_dmsgsPrint(L"请选择有效的配筋模板名称");
		return;
	}
	CString filepath_Wall = L"C:\\Template_XmlFile\\Wall_template\\";//L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Wall_template\\";
	CString filepath_Floor = L"C:\\Template_XmlFile\\Floor_template\\";//L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Floor_template\\";
	CString filepath_Face = L"C:\\Template_XmlFile\\Face_template\\";//L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Face_template\\";

	CString Standardpath;
	if (isWall)
	{
		Standardpath = filepath_Wall;// L"C:\\ProgramData\\CGNGalleryRebar\\PS\\";
	}
	else if (isFloor)
	{
		Standardpath = filepath_Floor;// L"C:\\ProgramData\\CGNGalleryRebar\\";
	}
	else if (isFace)
	{
		Standardpath = filepath_Face;// L"C:\\ProgramData\\CGNGalleryRebar\\Dgnlib\\";
	}
	CString Filepath = Standardpath + cmbstr + L".xml";
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
		mdlDialog_dmsgsPrint(L"mdlXMLDom_load error");
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

void C_RebarTemplate::Search_XmlFile(string pathName, vector<string>& vecFiles, string format)
{
	//读取文件夹下所有文件的名字（指定后缀）
	intptr_t hFile = 0;//文件句柄，过会儿用来查找
	struct _finddata_t fileinfo;//文件信息
	string p;

	if ((hFile = _findfirst(p.assign(pathName).append("//*" + format).c_str(), &fileinfo)) != -1)
		//如果查找到第一个文件
	{
		do
		{
			if ((fileinfo.attrib &  _A_SUBDIR))//如果是文件夹
			{
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					Search_XmlFile(p.assign(pathName).append("\\").append(fileinfo.name), vecFiles, format);
				;
			}
			else//如果是文件
			{
				vecFiles.push_back(fileinfo.name);
			}
		} while (_findnext(hFile, &fileinfo) == 0);    //能寻找到其他文件

		_findclose(hFile);    //结束查找，关闭句柄
	}
}


void C_RebarTemplate::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_TemplateName, m_edit_TemplateName);
	DDX_Control(pDX, IDC_template, m_cmb_RebarTemplate);
}


BEGIN_MESSAGE_MAP(C_RebarTemplate, CDialogEx)
	ON_BN_CLICKED(IDC_SaveTemplate, &C_RebarTemplate::OnBnClickedSavetemplate)
	//ON_BN_CLICKED(IDC_BUTTON3, &C_RebarTemplate::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_DeleteTemplate, &C_RebarTemplate::OnBnClickedDeletetemplate)
	ON_BN_CLICKED(IDOK, &C_RebarTemplate::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_template, &C_RebarTemplate::OnCbnSelchangetemplate)
	ON_BN_CLICKED(IDCANCEL, &C_RebarTemplate::OnBnClickedCancel)
END_MESSAGE_MAP()


// C_RebarTemplate 消息处理程序


void C_RebarTemplate::OnBnClickedSavetemplate()
{
	// TODO: 在此添加控件通知处理程序代码
	// 得到编辑框的模板名称并将其数据写入到XML文件
	GetDlgItemText(IDC_EDIT_TemplateName, m_Str_TemplateName);
	if (m_Str_TemplateName.GetLength() == 0)
	{
		mdlDialog_dmsgsPrint(L"添加模板失败，请添加模板名称");
		return;
	}
	m_vec_cmblist.push_back(m_Str_TemplateName);
	this->Write_xmlData(m_dlgData, m_vecRebarData);
	
}


//void C_RebarTemplate::OnBnClickedButton3()
//{
//	// TODO: 在此添加控件通知处理程序代码
//	this->readXML();
//}




void C_RebarTemplate::OnBnClickedDeletetemplate()
{
	// TODO: 在此添加控件通知处理程序代码
	/*int nIndex = m_cmb_RebarTemplate.GetCurSel();
	CString cmbText;
	m_cmb_RebarTemplate.GetLBText(nIndex, cmbText);*/

	CString cmbText;
	GetDlgItemText(IDC_template, cmbText);
	// 根据combobox的值，设定相应的文件路径
	CString filepath_Wall = L"C:\\Template_XmlFile\\Wall_template\\";//L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Wall_template\\";
	CString filepath_Floor = L"C:\\Template_XmlFile\\Floor_template\\"; //L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Floor_template\\";
	CString filepath_Face = L"C:\\Template_XmlFile\\Face_template\\"; //L"C:\\Program Files\\Bentley\\ProStructures CONNECT Edition\\ProStructures\\Mdlapps\\Face_template\\";

	CString Standardpath;
	if (isWall)
	{
		Standardpath = filepath_Wall;// L"C:\\ProgramData\\CGNGalleryRebar\\PS\\";
	}
	else if (isFloor)
	{
		Standardpath = filepath_Floor;// L"C:\\ProgramData\\CGNGalleryRebar\\";
	}
	else if (isFace)
	{
		Standardpath = filepath_Face;// L"C:\\ProgramData\\CGNGalleryRebar\\Dgnlib\\";
	}
	CString Filepath = Standardpath + cmbText + L".xml";
	/*char * FilePath_Name = Filepath.GetBuffer(Filepath.GetLength()+1);
	Filepath.ReleaseBuffer();*/
	USES_CONVERSION;
	char * pathName = W2A(Filepath);
	// remove()，删除该路径的xml文件
	if (remove(pathName) == 0)
	{
		int nIndex = m_cmb_RebarTemplate.GetCurSel();
		m_cmb_RebarTemplate.DeleteString(nIndex);
		/*CString str_delete;
		m_cmb_RebarTemplate.GetLBText(nIndex, str_delete);*/
		auto it = std::find(m_vec_cmblist.begin(), m_vec_cmblist.end(), cmbText);
		m_vec_cmblist.erase(it);
		// 删除之后combobox 显示其他一个名字，如果为空，则设置为空
		SetDlgItemText(IDC_template, m_vec_cmblist.front());
		if (m_vec_cmblist.size() == 0)
		{
			SetDlgItemText(IDC_template,L"");
		}
	}

}


void C_RebarTemplate::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();

	GetDlgItemText(IDC_template, m_LastTemplateName);
	CString cmbstr;
	GetDlgItemText(IDC_template, cmbstr);
	if (cmbstr.GetLength() == 0)
	{
		mdlDialog_dmsgsPrint(L"请选择有效的配筋模板名称");
		return;
	}

	// 读取xml文件相应的数据，便于主界面得到xml里面配筋模板的数据
	this->readXML();
	m_vec_cmblist.clear();
	// 板配筋时，g_wallMainDlg 为主要配筋界面的指针
	if (nullptr != g_wallMainDlg)
	{
		// 调用刷新函数，将读取到的xml文件数据刷新到主界面
		g_wallMainDlg->OnBnClickedUpdatadlg();
		g_wallMainDlg->Save_templateName();
		if (nullptr != g_wallMainDlg)
		{
			g_wallMainDlg = nullptr;
			free(g_wallMainDlg);	
		}
	}
	// 墙配筋时，g_settingMaindlg 为主要配筋界面的指针
	//else if (nullptr != g_settingMaindlg)//旧版墙配筋，
	//{
	//	// 调用刷新函数，将读取到的xml文件数据刷新到主界面
	//	g_settingMaindlg->OnBnClickedUpdatadlg();
	//}
	// 面配筋时，g_faceMainDlg 为面配筋界面的指针
	else if (nullptr != g_faceMainDlg)
	{
		g_faceMainDlg->OnBnClickedUpdatadlg();
		g_faceMainDlg->Save_templateName();
		if (nullptr != g_faceMainDlg)
		{
			g_faceMainDlg = nullptr;
			delete g_faceMainDlg;
		}
	}
	else if (nullptr != g_faceMainDlgEx)
	{
		g_faceMainDlgEx->OnBnClickedUpdatadlg();
		g_faceMainDlgEx->Save_templateName();
		if (nullptr != g_faceMainDlgEx)
		{
			g_faceMainDlgEx = nullptr;
			delete g_faceMainDlgEx;
		}
	}
	else if (nullptr != g_CatchpitMainDlg)
	{
		g_CatchpitMainDlg->OnBnClickedUpdatadlg();
		g_CatchpitMainDlg->Save_templateName();
		if (nullptr != g_CatchpitMainDlg)
		{
			g_CatchpitMainDlg = nullptr;
			free(g_CatchpitMainDlg);
		}
	}
	this->DestroyWindow();

	
}


void C_RebarTemplate::OnCbnSelchangetemplate()
{
	// TODO: 在此添加控件通知处理程序代码
	int nIndex = m_cmb_RebarTemplate.GetCurSel();
	m_cmb_RebarTemplate.GetLBText(nIndex, m_cmbStr_RebarTemplate);
	m_cmb_RebarTemplate.SetCurSel(nIndex);
}


void C_RebarTemplate::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	m_vec_cmblist.clear();
	this->DestroyWindow();
}
