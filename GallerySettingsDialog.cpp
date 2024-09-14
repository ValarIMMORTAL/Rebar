// GallerySettingsDialog.cpp: 实现文件
//
#include "_USTATION.h"
#include "GallerySettingsDialog.h"
#include "afxdialogex.h"
#include <Mstn/MdlApi/MdlApi.h>
#include <functional>
#include "PickElementTool.h"
#include "ScanIntersectTool.h"

using namespace std;
using namespace Bentley;
using namespace Bentley::DgnPlatform;

namespace Gallery
{
	namespace _local {
		/// @brief 判断一个元素是不是或板
		/// @details 这个是看PDMS参数中的Type中有没有WALL字样 
		bool is_wall_or_slab(EditElementHandleCR element)
		{
			std::string _name, type;
			if(!GetEleNameAndType(const_cast<EditElementHandleR>(element), _name, type))
			{
				return false;
			}
			auto result_pos_wall = type.find("WALL");
			auto result_pos_slab = type.find("FLOOR");
			if(result_pos_wall != std::string::npos || result_pos_slab != std::string::npos)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		/// @brief 从选中的元素中读取设置
		/// @param settings 
		/// @return 成功返回true
		bool read_settings_from_selected_elements(const ElementAgenda &agenda, GallerySettings &settings)
		{
			for(auto &element : agenda)
			{
				if(GallerySettings::from_element(element, settings))
				{
					return true;
				}
			}

			return false;
		}
	}

	// GallerySettingsDialog 对话框

	IMPLEMENT_DYNAMIC(SettingsDialog, CDialogEx)

	SettingsDialog::SettingsDialog(CWnd *pParent /*=nullptr*/)
		: CDialogEx(IDD_DIALOG_Gallery_Settings, pParent),
		  m_control_tab(),
		  m_main_rebar_page(),
		//   m_end_type_page(),
		  m_tab_controller(this->m_control_tab)
	{
		m_ClickedFlag = 0;
	}

    SettingsDialog::~SettingsDialog()
	{
	}

	void SettingsDialog::DoDataExchange(CDataExchange* pDX)
	{
		CDialogEx::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_TAB1, this->m_control_tab);
	}


	BEGIN_MESSAGE_MAP(SettingsDialog, CDialogEx)
		ON_BN_CLICKED(IDOK, &SettingsDialog::on_ok_button_click)
		ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &SettingsDialog::on_tab_select_changed)
		ON_BN_CLICKED(IDC_SELECT_MODEL, &SettingsDialog::on_select_model_button_clicked)
		ON_BN_CLICKED(IDCANCEL, &SettingsDialog::OnBnClickedCancel)
	END_MESSAGE_MAP()

	BOOL SettingsDialog::OnInitDialog()
	{
		CDialogEx::OnInitDialog();

		// 界面初始化
		// 添加MainRebar Page到tab页中
		this->m_tab_controller.create_page(this->m_main_rebar_page, IDD_DIALOG_Gallery_Settings_MainRebar_Page, L"主要配筋");

		//设置提示字体
		CWnd* pWnd = GetDlgItem(IDC_STATIC_Prompt);
		m_font.CreatePointFont(120, _T("宋体"), NULL);
		pWnd->SetFont(&m_font);

		// 如果已经有选择的元素
		ElementAgenda agenda;
		SelectionSetManager::GetManager().BuildAgenda(agenda);

		GallerySettings settings;
		if(!_local::read_settings_from_selected_elements(agenda, settings))
		{
			// 读取失败
			// 使用默认值
			settings = GallerySettings::from_default();
		}

		/// 初始化设置页面
		this->m_main_rebar_page.update_settings(settings);

		// 添加EndType到tab页中
		// 不需要端面样式
		// this->m_tab_controller.create_page(this->m_end_type_page, IDD_DIALOG_Gallery_Settings_EndType_Page, L"端面样式");

		return TRUE;
	}

	// 点击确定
	// 功能：写入当前参数到模型中去
	void SettingsDialog::on_ok_button_click(){
		auto settings = this->m_main_rebar_page.read_settings();

		// 写入配置到选中的模型中
		ElementAgenda agenda;
		if (!GetSelectAgenda(agenda, L"请选择廊道中的墙和板"))
		{
			return;
		}

		for(auto &selected_element: agenda)
		{
			settings.save_to_element(selected_element);
		}

		if(this->on_ok)
		{
			this->on_ok(settings,agenda);
		}
		
	}


	void SettingsDialog::on_tab_select_changed(NMHDR *pNMHDR, LRESULT *pResult)
	{
		this->m_tab_controller.update_selection();
		*pResult = 0;
	}


	void SettingsDialog::on_select_model_button_clicked()
	{
		m_ClickedFlag = 0;
		auto select_wall_tool = new PickElementTool(
			// on_filter
			_local::is_wall_or_slab,
			// on_complete
			[this](const ElementAgenda &agenda)
			{
				CString str;
				/*for(auto &entry: agenda)
				{
					str.Format(L"id = %d", entry.GetElementId());
					mdlDialog_dmsgsPrint(str);
				}
				mdlDialog_dmsgsPrintA("\n");*/

				GallerySettings settings;
				//if (!_local::read_settings_from_selected_elements(agenda, settings))
				//{
				//	// 读取失败
				//	// 使用默认值
				//	settings = GallerySettings::from_default();
				//}
				//if(_local::read_settings_from_selected_elements(agenda, settings))
				{
					// 读取当前界面数据
					// 更新到界面上
					settings = this->m_main_rebar_page.read_settings();
					this->m_main_rebar_page.update_settings(settings);
				}

				// 设置当前窗口焦点
				this->SetFocus();
				this->on_ok_button_click();
			},
			// on_cancel
			[this](void)
			{
				this->SetFocus();
			});

		select_wall_tool->InstallTool();
		m_ClickedFlag = 1;
	}

}



void Gallery::SettingsDialog::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	if (nullptr != this)
	{
		delete this;
	}

}

