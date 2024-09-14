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
	        BE_DATA_REFER(string, designcriteria)            //设计标准
			BE_DATA_VALUE(vector<string>, vecSeismicgrade)   //环境类别
			BE_DATA_VALUE(vector<string>, vecConcreteGrade)  //混凝土等级
			BE_DATA_VALUE(vector<string>, vecThicknesslayer) //保护层厚度
			BE_DATA_VALUE(vector<string>, vecRebarModel) //钢筋型号
			BE_DATA_VALUE(vector<string>, vecDiameter)		  //钢筋直径
			BE_DATA_REFER(string, laS)            //简支受压锚固长度
			BE_DATA_REFER(string, laE)            //抗震受拉锚固长度系数
			BE_DATA_REFER(string, dowelbarl)            //插筋长度
			BE_DATA_VALUE(vector<std::string>, vecTiebarform)		  //拉结筋形式
			BE_DATA_REFER(string, endbend)            //末段弯折最短长度
			BE_DATA_REFER(double, rebarspacing)       //钢筋间距
			BE_DATA_REFER(int,	  rebarstyle)         //生成钢筋类型
public:
	map<std::string, double> m_laplenth;//搭接长度L0
	map<std::string, double> m_alength;//直锚固长度La
	void SetSplitDatas(string tmpValue, vector<string>& vecDatas)
	{
		split_string(tmpValue, ',', vecDatas);
	}

};
extern GlobalParameters g_globalpara;



