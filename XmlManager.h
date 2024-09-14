#pragma once
#include "Rebar/BeXMLNode.h"
using namespace std;
using namespace RebarSpace;
class XmlManager
{
public:
	static  RebarSpace::BeXMLNode s_tmpnode;
	static vector<CString> s_alltypes;
	static vector<CString> s_allsize;
	static vector<CString> s_allgrades;
	static map<CString, BeXMLCell> s_mapalltypes;//BeXMLCell中name标识钢筋类型，tag为钢筋尺寸，val为钢筋等级
	static void ReadXmlFile(string xmlpath);
	static void GetAllRebarDatas();
	static CString GetSizeKeyBySizeAndGrade(string size, string grade);

};

