#include "_USTATION.h"
#include "XmlHelper.h"
#include "SetParam.h"
GlobalParameters g_globalpara;
extern RebarXmlInfo g_rebarXmlInfo;

void displayNodeAttributes(XmlNodeRef nodeRef)
{
	XmlNodeRef         attrNodeRef = NULL;
	XmlNamedNodeMapRef nodeMapRef = NULL;
	WChar          TestName[256];
	int            maxChars = 256;
	int status = mdlXMLDomElement_getAttribute(TestName, &maxChars, nodeRef, L"Name", XMLDATATYPE_WIDESTRING);

	WString wTestname(TestName);
	if (wTestname == L"Designcriteria")
	{
		maxChars = 256;
		WChar          TestName2[256];
		status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"Designcriteria", XMLDATATYPE_WIDESTRING);
		char tmpStr[512];
		BeStringUtilities::WCharToCurrentLocaleChar(tmpStr, TestName2, 256 * sizeof(wchar_t));
		g_globalpara.Setdesigncriteria(tmpStr);
		return;
	}
	else if (wTestname == L"DSeismicgrade")
	{
		maxChars = 256;
		WChar          TestName2[256];
		status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"DSeismicgrade", XMLDATATYPE_WIDESTRING);
		char tmpStr[512];
		BeStringUtilities::WCharToCurrentLocaleChar(tmpStr, TestName2, 256 * sizeof(wchar_t));
		g_globalpara.SetSplitDatas(tmpStr,g_globalpara.PopvecSeismicgrade());
		return;
	}
	else if (wTestname == L"Laplength")
	{
		XmlNodeRef         attrNodeRef = NULL;
		XmlNamedNodeMapRef nodeMapRef = NULL;
		mdlXMLDomElement_getAllAttributes(&nodeMapRef, nodeRef);
		int   status = mdlXMLDomAttrList_getFirstChild(&attrNodeRef, nodeMapRef);
		while (SUCCESS == status)
		{
			WString Tmp(L"Diameter_of_reinforcement");
			int            maxChars = 256;
			WChar          attrName[256], attrVal[256];
			mdlXMLDomAttr_getName(attrName, &maxChars, attrNodeRef);
			maxChars = 256;
			mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
			WString wName(attrName);
			WString wValue(attrVal);
			if (wName==L"Name")
			{
				status = mdlXMLDomAttrList_getNextChild(&attrNodeRef, nodeMapRef);
				continue;
			}
			wName = wName.substr(Tmp.length(), wName.length() - Tmp.length());
			g_globalpara.m_laplenth[StringOperator::Convert::WStringToString(wName.GetWCharCP())] = BeStringUtilities::Wtof(wValue.GetWCharCP());
			status = mdlXMLDomAttrList_getNextChild(&attrNodeRef, nodeMapRef);
		}
		if (NULL != nodeMapRef)
			mdlXMLDomAttrList_free(nodeMapRef);
		if (NULL != attrNodeRef)
			mdlXMLDomNode_free(attrNodeRef);
		return;
	}
	else if (wTestname == L"Straight_anchorage_length")
	{
		XmlNodeRef         attrNodeRef = NULL;
		XmlNamedNodeMapRef nodeMapRef = NULL;
		mdlXMLDomElement_getAllAttributes(&nodeMapRef, nodeRef);
		int   status = mdlXMLDomAttrList_getFirstChild(&attrNodeRef, nodeMapRef);
		while (SUCCESS == status)
		{
			WString Tmp(L"Diameter_of_reinforcement");
			int            maxChars = 256;
			WChar          attrName[256], attrVal[256];
			mdlXMLDomAttr_getName(attrName, &maxChars, attrNodeRef);
			maxChars = 256;
			mdlXMLDomAttr_getValue(attrVal, &maxChars, attrNodeRef);
			WString wName(attrName);
			WString wValue(attrVal);
			if (wName == L"Name")
			{
				status = mdlXMLDomAttrList_getNextChild(&attrNodeRef, nodeMapRef);
				continue;
			}
			wName = wName.substr(Tmp.length(), wName.length() - Tmp.length());
			g_globalpara.m_alength[StringOperator::Convert::WStringToString(wName.GetWCharCP())] = BeStringUtilities::Wtof(wValue.GetWCharCP());
			status = mdlXMLDomAttrList_getNextChild(&attrNodeRef, nodeMapRef);
		}
		if (NULL != nodeMapRef)
			mdlXMLDomAttrList_free(nodeMapRef);
		if (NULL != attrNodeRef)
			mdlXMLDomNode_free(attrNodeRef);
		return;
	}
	else if (wTestname == L"Strength_grade_of_concrete")
	{
		maxChars = 256;
		WChar          TestName2[256];
		status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"Strengthg_rade_of_concrete", XMLDATATYPE_WIDESTRING);
		char tmpStr[512];
		BeStringUtilities::WCharToCurrentLocaleChar(tmpStr, TestName2, 256 * sizeof(wchar_t));
		g_globalpara.SetSplitDatas(tmpStr, g_globalpara.PopvecConcreteGrade());
		return;
	}
	else if (wTestname == L"Rebar_model")
	{
		maxChars = 256;
		WChar          TestName2[256];
		status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"Rebar_model", XMLDATATYPE_WIDESTRING);
		char tmpStr[512];
		BeStringUtilities::WCharToCurrentLocaleChar(tmpStr, TestName2, 256 * sizeof(wchar_t));
		g_globalpara.SetSplitDatas(tmpStr, g_globalpara.PopvecRebarModel());
		return;
	}
	else if (wTestname == L"Thickness_of_protective_layer")
	{
	    maxChars = 256;
		WChar          TestName2[256];
		status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"Thickness_of_protective_layer", XMLDATATYPE_WIDESTRING);
		char tmpStr[512];
		BeStringUtilities::WCharToCurrentLocaleChar(tmpStr, TestName2, 256 * sizeof(wchar_t));
		g_globalpara.SetSplitDatas(tmpStr, g_globalpara.PopvecThicknesslayer());
		return;
	}
	else if (wTestname == L"Diameter_of_rebar")
	{
	maxChars = 256;
	WChar          TestName2[256];
	status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"Diameter_of_rebar", XMLDATATYPE_WIDESTRING);
	char tmpStr[512];
	BeStringUtilities::WCharToCurrentLocaleChar(tmpStr, TestName2, 256 * sizeof(wchar_t));
	g_globalpara.SetSplitDatas(tmpStr, g_globalpara.PopvecDiameter());
	}
	else if (wTestname == L"Simply_supported_compression_anchorage_length")
	{
	maxChars = 256;
	WChar          TestName2[256];
	status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"laS", XMLDATATYPE_WIDESTRING);
	char tmpStr[512];
	BeStringUtilities::WCharToCurrentLocaleChar(tmpStr, TestName2, 256 * sizeof(wchar_t));
	g_globalpara.SetlaS(tmpStr);
	return;

	}
	else if (wTestname == L"Anchorage_coefficient_of_seismictension")
	{
	maxChars = 256;
	WChar          TestName2[256];
	status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"laE", XMLDATATYPE_WIDESTRING);
	char tmpStr[512];
	BeStringUtilities::WCharToCurrentLocaleChar(tmpStr, TestName2, 256 * sizeof(wchar_t));
	g_globalpara.SetlaE(tmpStr);
	return;
	}
	else if (wTestname == L"Anchorage_length_of_dowelbar")
	{
	maxChars = 256;
	WChar          TestName2[256];
	status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"l", XMLDATATYPE_WIDESTRING);
	char tmpStr[512];
	BeStringUtilities::WCharToCurrentLocaleChar(tmpStr, TestName2, 256 * sizeof(wchar_t));
	g_globalpara.Setdowelbarl(tmpStr);
	return;
	}
	else if (wTestname == L"Tiebar_form")
	{
	maxChars = 256;
	WChar          TestName2[256];
	status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"Tiebar_form", XMLDATATYPE_WIDESTRING);
	char tmpStr[512];
	BeStringUtilities::WCharToCurrentLocaleChar(tmpStr, TestName2, 256 * sizeof(wchar_t));
	g_globalpara.SetSplitDatas(tmpStr, g_globalpara.PopvecTiebarform());
	return;
	}
	else if (wTestname == L"Minimum_length_of_end_bend")
	{
	maxChars = 256;
	WChar          TestName2[256];
	status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"Minimum_length_of_endbend", XMLDATATYPE_WIDESTRING);
	char tmpStr[512];
	BeStringUtilities::WCharToCurrentLocaleChar(tmpStr, TestName2, 256 * sizeof(wchar_t));
	g_globalpara.Setendbend(tmpStr);
	return;
	}
	else if (wTestname == L"Rebar_spacing")
	{
	maxChars = 256;
	WChar          TestName2[256];
	status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"Rebar_spacing", XMLDATATYPE_WIDESTRING);
	
	g_globalpara.Setrebarspacing(BeStringUtilities::Wtof(TestName2));
	return;
	}
	else if (wTestname == L"Rebar_style")
	{
		maxChars = 256;
		WChar          TestName2[256];
		status = mdlXMLDomElement_getAttribute(TestName2, &maxChars, nodeRef, L"Rebar_style", XMLDATATYPE_WIDESTRING);

		g_globalpara.Setrebarstyle(BeStringUtilities::Wtoi(TestName2));
	return;
	}

}
void ReadNode(XmlNodeRef nodeRef)
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
void readXML()
{
	memset(&g_globalpara, sizeof(GlobalParameters), 0);
	XmlDomRef  domRef = NULL;
	mdlXMLDom_create(&domRef);
	loadPathResultDlgParams();
	WString tmpName = g_rebarXmlInfo.xmlName;
	WString xmlName = L"C:/ProgramData/Bentley/ProStructures CONNECT Edition/Configuration/WorkSpaces/China/Standards/ProStructures/Rebar/Codes/";
	if (!tmpName.empty())
	{
		xmlName += tmpName;
	}
	else
	{
		xmlName += L"RebarCode_zhongguangheC40.xml";
	}
	
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

/**
 *  切割字符串
 *  @param  original    [in]    需要切分的字符串
 *  @param  separator    [in]    分隔符
 *  @param  col_limit   [in]    最多解析的栏目数,0解析全部域
 *  @param  result  [out]   输出列表
 */
void split_string(const string& original, char separator, vector<string>& result)
{
	result.clear();
	size_t current_pos = 0, previous_pos = 0;
	uint32_t col_count = 0;
	while ((current_pos = original.find(separator, previous_pos)) != string::npos)
	{
		string part(original.substr(previous_pos, current_pos - previous_pos));
		result.push_back(part);
		previous_pos = current_pos + 1;
		col_count++;
	}
	string part(original.substr(previous_pos));
	result.push_back(part);

}