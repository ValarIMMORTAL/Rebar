// 功能：授权接口文件
// 文件名：CheckLicense.h
// 日期：2019/11/20

#ifndef __CHECK_LICENSE_H__
#define __CHECK_LICENSE_H__

#include <string>
#include <xstring>

using std::string;
using std::wstring;

// 功能：检查工具是否授权
// 参数 IN taskName 工具名
// 参数 IN licensePathName 授权文件目录
// 返回 true:已授权；false:未授权
extern bool CheckLicense(const string &taskName, const wstring &licensePathName); // (placedoortool, placedoortool)

#endif // __CHECK_LICENSE_H__