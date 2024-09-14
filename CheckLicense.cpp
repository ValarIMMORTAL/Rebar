// 功能：授权接口文件
// 文件名：CheckLicense.cpp
// 日期：2019/11/20
#include "_USTATION.h"
#include "CheckLicense.h"

#include <algorithm>

#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>

#include <mstn/mdlapi/mssystem.fdf>
#include <mstn/mdlapi/msinput.fdf>

#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "Shell32.lib")

std::string CKto_utf8(const wchar_t* buffer, int len)
{
	int nChars = ::WideCharToMultiByte(
		CP_UTF8,
		0,
		buffer,
		len,
		NULL,
		0,
		NULL,
		NULL);
	if (nChars == 0)return"";

	std::string newbuffer;
	newbuffer.resize(nChars);
	::WideCharToMultiByte(
		CP_UTF8,
		0,
		buffer,
		len,
		const_cast<char*>(newbuffer.c_str()),
		nChars,
		NULL,
		NULL);

	return newbuffer;
}

std::string CKto_utf8(const std::wstring& str)
{
	return CKto_utf8(str.c_str(), (int)str.size());
}
std::string CKWStringToString(const std::wstring &wstr)
{
	std::string str;
	int nLen = (int)wstr.length();
	str.resize(nLen, ' ');
	int nResult = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wstr.c_str(), nLen, (LPSTR)str.c_str(), nLen, NULL, NULL);
	if (nResult == 0)
	{
		return "";
	}
	return str;
}
std::wstring CKStringToWString(const std::string& str)
{
	int num = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
	wchar_t *wide = new wchar_t[num];
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wide, num);
	std::wstring w_str(wide);
	delete[] wide;
	return w_str;
}
std::string CKGetProgramFilesPath(const wstring &licensePathName)
{
	std::wstring wsValue = L"";
	wchar_t buffer[MAX_PATH];
	::ZeroMemory(buffer, MAX_PATH*sizeof(wchar_t));
	if (::SHGetSpecialFolderPathW(NULL, buffer, CSIDL_COMMON_APPDATA, NULL))
	{
		wsValue = buffer;
	}

	//不为空  
	wchar_t szTmp[MAX_PATH] = { 0 };
	wcscpy_s(szTmp, wsValue.c_str());
	::PathAppendW(szTmp, licensePathName.c_str()); // L"PDMSToBentley");

	return CKto_utf8(szTmp);
}

// 检查许可证接口
typedef int (*InitializeRegContextFunc)(const char* path);
typedef int (*CheckRpFailShowRDFunc)(const char* name, const char* ver);

/***************************************检查许可函数***************************************/
bool CheckLicense(const string &taskName, const wstring &licensePathName)
{
	//string maName = taskName + ".ma";
	//string unloadKeyin = string("mdl unload ") + taskName;
	//std::transform(maName.begin(), maName.end(), maName.begin(), ::tolower);

	char str[256];
	//wstring wappPath = mdlSystem_getApplicationPath (NULL);
	//string appPath = CKWStringToString(wappPath);
	//
	////const char* appPath = mdlSystem_getApplicationPath (NULL);
	//std::transform(appPath.begin(), appPath.end(), appPath.begin(), ::tolower);
	//string::size_type position = appPath.find(maName.c_str());

	//if (string::npos == position)
	//{
	//	mdlInput_sendKeyin(CKStringToWString(unloadKeyin).c_str(), 0, INPUTQ_EOQ, NULL);
	//	return false;
	//}
	string path = CKGetProgramFilesPath(licensePathName);
	string appPath = path + "\\PS\\";
	sprintf(str,"%sRDTool.dll", appPath.c_str());
	string sspath(str);
	WString tmpPath(sspath.c_str());
	//HINSTANCE hDll = LoadLibrary (str);
	HINSTANCE hDll =LoadLibraryEx(tmpPath.GetWCharCP(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	//int errId = GetLastError();

	if (NULL == hDll)
	{
		return false;
	}
	else
    {
		bool rt = false;
        InitializeRegContextFunc initRegContext = (InitializeRegContextFunc)GetProcAddress(hDll, "InitializeRegContext");
        if (initRegContext)
		{
			int re = initRegContext(path.c_str());
			if (re == 0)
			{
				CheckRpFailShowRDFunc checkRpFailShowRD = (CheckRpFailShowRDFunc)GetProcAddress(hDll, "CheckRpFailShowRD");
				if (checkRpFailShowRD && (checkRpFailShowRD(taskName.c_str(), "V1.0.0.0") == 0))
				{
					rt = true;
				}
			}
        }
        FreeLibrary(hDll);

		return rt;
    }
}
