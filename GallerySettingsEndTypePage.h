#pragma once

#include "resource.h"
#include <afxwin.h>
#include <afxdialogex.h>
#include <afxcmn.h>


// GallerySettingsEndTypePage 对话框

class GallerySettingsEndTypePage : public CDialogEx
{
	DECLARE_DYNAMIC(GallerySettingsEndTypePage)

public:
	GallerySettingsEndTypePage(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~GallerySettingsEndTypePage();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_Gallery_Settings_EndType_Page };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
