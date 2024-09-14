// GallerySettingsEndTypePage.cpp: 实现文件
//

#include "_USTATION.h"
#include "GallerySettingsEndTypePage.h"
#include "afxdialogex.h"


// GallerySettingsEndTypePage 对话框

IMPLEMENT_DYNAMIC(GallerySettingsEndTypePage, CDialogEx)

GallerySettingsEndTypePage::GallerySettingsEndTypePage(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_Gallery_Settings_EndType_Page, pParent)
{

}

GallerySettingsEndTypePage::~GallerySettingsEndTypePage()
{
}

void GallerySettingsEndTypePage::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(GallerySettingsEndTypePage, CDialogEx)
END_MESSAGE_MAP()


// GallerySettingsEndTypePage 消息处理程序
