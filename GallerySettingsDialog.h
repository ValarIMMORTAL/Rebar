#pragma once

#include "resource.h"
#include <afxwin.h>
#include <afxdialogex.h>
#include <afxcmn.h>
#include <vector>
#include <DgnPlatform/DgnPlatform.h>
#include "TabController.h"
#include "GallerySettingsMainRebarPage.h"
#include "GallerySettingsEndTypePage.h"

namespace Gallery
{
	// GallerySettingsDialog 对话框

	class SettingsDialog : public CDialogEx
	{
	public:
		DECLARE_DYNAMIC(SettingsDialog)

	public:
		// 使用选中的墙和板初始化配筋设置面板
		SettingsDialog(CWnd* pParent = nullptr);   // 标准构造函数
		virtual ~SettingsDialog();

	// 对话框数据
	#ifdef AFX_DESIGN_TIME
		enum { IDD = IDD_DIALOG_Gallery_Settings };
	#endif

	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
		virtual BOOL OnInitDialog() override;

	public: // controls
		CTabCtrl m_control_tab;
		SettingsMainRebarPage m_main_rebar_page;
		// SettingsEndTypePage m_end_type_page;

		TabController m_tab_controller;
		int m_ClickedFlag;//点击了点选按钮并且选中墙后设置为1，使得墙配筋界面不会关闭
						  //没有点击点选按钮就选中墙，会设置为0，使得按下确定按钮界面就会关闭

	private: // event handlers
		void on_ok_button_click();
		void on_tab_select_changed(NMHDR *pNMHDR, LRESULT *pResult);

		DECLARE_MESSAGE_MAP()
	public:
		afx_msg void on_select_model_button_clicked();

	public:
		std::function<void(const GallerySettings &, const ElementAgenda &agenda)> on_ok;
	
	private:
		CFont m_font; //提示字体
	public:
		afx_msg void OnBnClickedCancel();
	};
}

