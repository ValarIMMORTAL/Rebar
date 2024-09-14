#pragma once

//钢筋xml文件信息
struct RebarXmlInfo
{
	WChar xmlName[MAX_LEVEL_NAME_LENGTH];
};
/**
* brief 保存数值参数
* return 无返回值
*/
void savePathResultDlgParams(void);

/**
* @返回类型  void
* @函数说明  加载参数值
* @函数参数  void
**/
void loadPathResultDlgParams(void);

