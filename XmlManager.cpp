#include "_ustation.h"
#include "XmlManager.h"

RebarSpace::BeXMLNode XmlManager::s_tmpnode;
vector<CString> XmlManager::s_alltypes;
vector<CString> XmlManager::s_allsize;
vector<CString> XmlManager::s_allgrades;
map<CString, BeXMLCell> XmlManager::s_mapalltypes;
void XmlManager::ReadXmlFile(string xmlpath)
{
	BrString tmpstring(xmlpath.c_str());
	StartXMLMemPool(); 
    s_tmpnode.ReadFile(tmpstring, false);
}
void XmlManager::GetAllRebarDatas()
{
	register INT i;
	BeXMLNodeList m_nodes = s_tmpnode.GetNodes().GetAt(0)->GetNodes();
	for (i = 0; i < m_nodes.GetSize(); i++)
	{
		BeXMLNode *node = m_nodes.GetAt(i);
		if (node != NULL)
		{
			if (node->GetName()== "RebarInfo")
			{
				CString name = node->GetString("Designation");
				CString tag = node->GetString("Size");
				CString val = node->GetString("Grade");
				s_alltypes.push_back(name);
				s_allsize.push_back(tag);
				s_mapalltypes[name].name = name;
				s_mapalltypes[name].tag = tag;
				s_mapalltypes[name].val = val;
			}
			else if (node->GetName() == "GradeInfo")
			{
				s_allgrades.push_back(node->GetString("Name"));
			}
		}
		
	}
	std::sort(s_allsize.begin(), s_allsize.end());
	s_allsize.erase(std::unique(s_allsize.begin(), s_allsize.end()), s_allsize.end());
}


CString XmlManager::GetSizeKeyBySizeAndGrade(string size,string grade)
{
	register INT i;
	BeXMLNodeList m_nodes = s_tmpnode.GetNodes().GetAt(0)->GetNodes();
	for (i = 0; i < m_nodes.GetSize(); i++)
	{
		BeXMLNode *node = m_nodes.GetAt(i);
		if (node != NULL)
		{
			if (node->GetName() == "RebarInfo")
			{
				CString name = node->GetString("Designation");
				CString tag = node->GetString("Size");
				CString val = node->GetString("Grade");
			
				if (string(CT2A(tag))==size&&string(CT2A(val))==grade)
				{
					return name;
				}

			}
		}

	}
	return L"";
}