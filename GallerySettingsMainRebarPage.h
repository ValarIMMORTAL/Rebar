#pragma once

#include "resource.h"
#include <afxwin.h>
#include <afxdialogex.h>
#include <afxcmn.h>
#include <vector>
#include "CWallRebarListCtrl.h"
#include "GallerySettings.h"
#include "C_RebarTemplate.h"

namespace Gallery
{
	struct ConcreteRebar
	{
		int		rebarLevel;			//钢筋层
		int		rebarDir;			//方向			0：横向钢筋，1：竖向钢筋
		char    rebarSize[512];		//钢筋尺寸
		int		rebarType;			//钢筋型号
		double  spacing;			//钢筋间距
		int		datachange;			//数据交换
		int		rebarLineStyle;		//钢筋线型
		int		rebarWeight;		//钢筋线宽
	};

	// SettingsMainRebarPage 对话框
	class SettingsMainRebarPage : public CDialogEx
	{
		DECLARE_DYNAMIC(SettingsMainRebarPage)

	public:
		/// @details 界面默认使用项目中保存的设置
		SettingsMainRebarPage(CWnd *pParent = nullptr); // 标准构造函数
		virtual ~SettingsMainRebarPage();

// 对话框数据
#ifdef AFX_DESIGN_TIME
		enum
		{
			IDD = IDD_DIALOG_Gallery_Settings_MainRebar_Page
		};
#endif

	protected:
		virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV 支持
		virtual BOOL OnInitDialog() override;

	public: // Controls
		// 钢筋配置
		CMainRebarListCtrl m_main_rebar_list;
		// 正面保护层厚度
		CEdit m_front_edit;
		// 侧面保护层厚度
		CEdit m_side_edit;
		// 背面保护层厚度
		CEdit m_back_edit;
		// 钢筋层数
		CEdit m_layer_edit;
		// 规避孔洞
		CButton m_hole_check;
		// 忽略孔洞尺寸
		CEdit m_hole_size_edit;
		//界面上的保护层等数据
		PIT::DlgData  m_dlgData;
		//钢筋模板界面
		C_RebarTemplate		m_RebarTemplate;			

	private: // members
	public:	 // Methods
		/// @brief 读取当前界面上的配置数据
		GallerySettings read_settings();

		/// @brief  使用新的配置数据更新界面
		/// @param settings 新的配置数据
		void update_settings(const GallerySettings &settings);

		// 设置默认的钢筋每一层数据
		void SetDefaultData(int rebarLevelNum);

		vector<ConcreteRebar> m_vecRebarData; 

		DECLARE_MESSAGE_MAP()
		// Event handlers
	public:
		afx_msg void on_hole_check_box_clicked();
		afx_msg void on_layer_edit_killed_focus();
		// 配筋模板按钮
		afx_msg void OnBnClickedRebartemplate();
		afx_msg void OnBnClickedUpdatadlg();
		afx_msg void OnBnClickedButton2();
	};
	extern SettingsMainRebarPage * g_settingMaindlg;
}
