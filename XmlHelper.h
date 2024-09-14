/*--------------------------------------------------------------------------------------+
|     $Source: /miscdev-root/miscdev/mdl/examples/imodelvisitor/XmlHelper.h,v $
|
|  $Copyright: (c) 2012 Bentley Systems, Incorporated. All rights reserved. $
+--------------------------------------------------------------------------------------*/
#pragma once

///////////////////////////////////////////////////////////////////////////////////////////
// This file should be included directly from a cpp file and not in a .h file that
// may get used for a PCH. The .cpp file that includes this must also be compiled
// outside of a multi-compile block for the #import statement to work properly.
//////////////////////////////////////////////////////////////////////////////////////////

#include <Bentley/WString.h>
#include <Mstn/XmlTools/mdlxmltools.fdf>
#include <Mstn\MdlApi\mdlxmltoolslink.fdf>
#include "CommonFile.h"
USING_NAMESPACE_BENTLEY
using namespace std;


void displayNodeAttributes(XmlNodeRef nodeRef);

void ReadNode(XmlNodeRef nodeRef);

void readXML();

void split_string(const string& original, char separator, vector<string>& result);

struct GlobalParameters
{
public:
	        BE_DATA_REFER(string, designcriteria)            //��Ʊ�׼
			BE_DATA_VALUE(vector<string>, vecSeismicgrade)   //�������
			BE_DATA_VALUE(vector<string>, vecConcreteGrade)  //�������ȼ�
			BE_DATA_VALUE(vector<string>, vecThicknesslayer) //��������
			BE_DATA_VALUE(vector<string>, vecRebarModel) //�ֽ��ͺ�
			BE_DATA_VALUE(vector<string>, vecDiameter)		  //�ֽ�ֱ��
			BE_DATA_REFER(string, laS)            //��֧��ѹê�̳���
			BE_DATA_REFER(string, laE)            //��������ê�̳���ϵ��
			BE_DATA_REFER(string, dowelbarl)            //����
			BE_DATA_VALUE(vector<std::string>, vecTiebarform)		  //�������ʽ
			BE_DATA_REFER(string, endbend)            //ĩ��������̳���
			BE_DATA_REFER(double, rebarspacing)       //�ֽ���
			BE_DATA_REFER(int,	  rebarstyle)         //���ɸֽ�����
public:
	map<std::string, double> m_laplenth;//��ӳ���L0
	map<std::string, double> m_alength;//ֱê�̳���La
	void SetSplitDatas(string tmpValue, vector<string>& vecDatas)
	{
		split_string(tmpValue, ',', vecDatas);
	}

};
extern GlobalParameters g_globalpara;



