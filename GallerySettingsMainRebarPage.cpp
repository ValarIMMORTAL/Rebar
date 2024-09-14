// GallerySettingsMainRebarPage.cpp: 实现文件
//
#include "_USTATION.h"
#include "GallerySettingsMainRebarPage.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"
#include "CommonFile.h"
#include "ListCtrlEx.h"
#include <string.h>

#define WM_LIST_UPDATED (WM_USER + 1)

namespace Gallery
{
	//extern SettingsMainRebarPage * g_settingMaindlg;
	SettingsMainRebarPage * g_settingMaindlg = nullptr;
	// GallerySettingsMainRebarPage 对话框

	IMPLEMENT_DYNAMIC(SettingsMainRebarPage, CDialogEx)

	SettingsMainRebarPage::SettingsMainRebarPage(CWnd *pParent /*=nullptr*/)
		: CDialogEx(IDD_DIALOG_Gallery_Settings_MainRebar_Page, pParent)
	{
	}

	SettingsMainRebarPage::~SettingsMainRebarPage()
	{
		g_settingMaindlg = nullptr;
		free(g_settingMaindlg);
	}

	void SettingsMainRebarPage::DoDataExchange(CDataExchange *pDX)
	{
		CDialogEx::DoDataExchange(pDX);
		DDX_Control(pDX, IDC_LIST1, this->m_main_rebar_list);
		DDX_Control(pDX, IDC_EDIT1, this->m_front_edit);
		DDX_Control(pDX, IDC_EDIT2, this->m_side_edit);
		DDX_Control(pDX, IDC_EDIT3, this->m_back_edit);
		DDX_Control(pDX, IDC_EDIT4, this->m_layer_edit);
		DDX_Control(pDX, IDC_HOLE_CHECK, this->m_hole_check);
		DDX_Control(pDX, IDC_MHOLESIZE_EDIT, this->m_hole_size_edit);
	}

	/// 更新钢筋层到界面上
	void update_rebar_list(ListCtrlEx::CListCtrlEx &list, const GallerySettings::Layer *rebar_layers, int layer_count);

	BOOL SettingsMainRebarPage::OnInitDialog()
	{
		CDialogEx::OnInitDialog();
		g_settingMaindlg = this;
		// 初始化列表
		// 设置列表的style
		LONG style = GetWindowLong(this->m_main_rebar_list.m_hWnd, GWL_STYLE); // 获取当前窗口style
		style &= ~LVS_TYPEMASK;												   // 清除显示方式位
		style |= LVS_REPORT;												   // 设置style
		style |= LVS_SINGLESEL;												   // 单选模式
		SetWindowLong(this->m_main_rebar_list.m_hWnd, GWL_STYLE, style);	   // 设置style

		DWORD ex_style = this->m_main_rebar_list.GetExtendedStyle();
		ex_style |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ）
		ex_style |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ）
		this->m_main_rebar_list.SetExtendedStyle(ex_style); // 设置扩展风格

		RECT rect;
		this->m_main_rebar_list.GetClientRect(&rect);
		double width = rect.right - rect.left;
		// 在列表控件中插入列
		this->m_main_rebar_list.InsertColumn(0, _T("层"), (int)(width / 8.0 * 0.75), ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
		this->m_main_rebar_list.InsertColumn(1, _T("方向"), (int)(width / 8.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
		this->m_main_rebar_list.InsertColumn(2, _T("直径"), (int)(width / 8.0), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
		this->m_main_rebar_list.InsertColumn(3, _T("类型"), (int)(width / 8.0 *1.25), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
		this->m_main_rebar_list.InsertColumn(4, _T("间距"), (int)(width / 8.0* 1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
		//this->m_main_rebar_list.InsertColumn(5, _T("起点偏移"), (int)(width / 8.0 * 1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
		//this->m_main_rebar_list.InsertColumn(6, _T("终点偏移"), (int)(width / 8.0 * 1.25), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
		//this->m_main_rebar_list.InsertColumn(7, _T("与前层间距"), (int)(width / 8.0), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
		this->m_main_rebar_list.InsertColumn(5, _T("位置"), (int)(width / 8.0 * 1.25), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
		//this->m_main_rebar_list.InsertColumn(9, _T("颜色"), (int)(width / 8 * 0.95), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
		this->m_main_rebar_list.InsertColumn(6, _T("线形"), (int)(width / 8 * 0.8), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
		this->m_main_rebar_list.InsertColumn(7, _T("线宽"), (int)(width / 8 * 0.8), ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

		/// 使用默认参数
		this->update_settings(GallerySettings::from_default());

		return TRUE;
	}

	template <class T>
	const T &get_list_n(const std::list<T> &list, size_t n)
	{
		auto iter = list.begin();
		std::advance(iter, n);
		return *iter;
	}

	void update_rebar_list(ListCtrlEx::CListCtrlEx &list, const GallerySettings::Layer *layers, int layer_count)
	{
		// 清空list中之前的数据
		list.DeleteAllItems();

		for (int i = 0; i < layer_count; ++i)
		{
			const auto &layer = layers[i];

			// 插入一个新行到list中
			list.InsertItem(i, L"");

			CString str;
			// 层号
			str.Format(L"%dL", layer.layer_index);
			list.SetItemText(i, 0, str);

			// 方向
			ListCtrlEx::CStrList dir_list(g_listRebarDir);
			list.SetCellStringList(i, 1, dir_list);
			if (layer.direction == RebarDirection::Horizontal)
			{
				list.SetItemText(i, 1, get_list_n(g_listRebarDir, 0));
			}
			else
			{
				list.SetItemText(i, 1, get_list_n(g_listRebarDir, 1));
			}

			// 直径(尺寸)
			ListCtrlEx::CStrList size_list(g_listRebarSize);
			list.SetCellStringList(i, 2, size_list);
			str = layer.size_str;
			str += L"mm";
			list.SetItemText(i, 2, str);

			// 类型
			ListCtrlEx::CStrList type_list(g_listRebarType);
			list.SetCellStringList(i, 3, type_list);
			list.SetItemText(i, 3, get_list_n(g_listRebarType, layer.type));

			// 间距
			str.Format(L"%.2f", layer.spacing);
			list.SetItemText(i, 4, str);

			//// 起始偏移
			//str.Format(L"%.2f", layer.start_offset);
			//list.SetItemText(i, 5, str);

			//// 终点偏移
			//str.Format(L"%.2f", layer.end_offset);
			//list.SetItemText(i, 6, str);

			//// 层间距
			//str.Format(L"%.2f", layer.layer_space);
			//list.SetItemText(i, 7, str);

			// 位置
			ListCtrlEx::CStrList pos_list(g_listRebarPose);
			list.SetCellStringList(i, 5, pos_list);
			list.SetItemText(i, 5, get_list_n(g_listRebarPose, static_cast<int>(layer.position)));

			// 颜色
			/*std::list<CString> RebarColor;
			for (int i = -1; i < 256; ++i)
			{
				CString tmpCStr;
				tmpCStr.Format(_T("%d"), i);
				RebarColor.push_back(tmpCStr);
			}
			list.SetCellStringList(i, 9, RebarColor);
			CString strColor;
			strColor.Format(L"%d", layer.rebarColor);
			list.SetItemText(i, 9, strColor);*/

			// 线型
			std::list<CString> Rebar_LineStyle;
			for (int i = 0; i < 8; ++i)
			{
				CString tmpCStr;
				tmpCStr.Format(_T("%d"), i);
				Rebar_LineStyle.push_back(tmpCStr);
			}
			list.SetCellStringList(i, 6, Rebar_LineStyle);
			CString strLineStyle;
			strLineStyle.Format(L"%d", layer.rebarLineStyle);
			list.SetItemText(i, 6, strLineStyle);

			// 线宽
			std::list<CString> Rebar_Weight;
			for (int i = 0; i < 32; ++i)
			{
				CString tmpCStr;
				tmpCStr.Format(_T("%d"), i);
				Rebar_Weight.push_back(tmpCStr);
			}
			list.SetCellStringList(i, 7, Rebar_Weight);
			CString strWeight;
			strWeight.Format(L"%d", layer.rebarWeight);
			list.SetItemText(i, 7, strWeight);

		}
	}

	/// 读取钢筋数据到settings中
	void read_rebar_list(ListCtrlEx::CListCtrlEx &list, GallerySettings &settings)
	{
		// 获得数据
		std::map<int, vector<CString>> all_data;
		list.GetAllData(all_data);

		settings.layer_count = (int)all_data.size();

		for (const auto &item : all_data)
		{
			auto row = item.first;
			auto &cells = item.second;

			auto &layer = settings.layers[row];

			// 层
			{
				CString str(cells[0]);
				str.Delete(str.GetLength() - 1, 1); // 删除末尾的L
				layer.layer_index = atoi(CT2A(str));
			}

			// 方向
			{
				auto begin = g_listRebarDir.cbegin();
				auto end = g_listRebarDir.cend();
				CString str(cells[1]);
				auto result = std::find(begin, end, str);
				auto index = (int)std::distance(begin, result);
				layer.direction = static_cast<RebarDirection>(index);
			}

			// 直径
			{
				CString str(cells[2]);
				str = str.Left(str.GetLength() - 2); // 删除mm
				strcpy(layer.size_str, CT2A(str));
			}

			// 类型
			{
				auto begin = g_listRebarType.cbegin();
				auto end = g_listRebarType.cend();
				CString str(cells[3]);
				auto result = std::find(begin, end, str);
				auto index = (int)std::distance(begin, result);
				layer.type = index;
			}

			// 间距
			layer.spacing = atof(CT2A(cells[4]));

			//// 起始偏移
			//layer.start_offset = atof(CT2A(cells[5]));

			//// 终点偏移
			//layer.end_offset = atof(CT2A(cells[6]));

			//// 层与层间距
			//layer.layer_space = atof(CT2A(cells[7]));

			// 位置
			{
				auto begin = g_listRebarPose.cbegin();
				auto end = g_listRebarPose.cend();
				CString str(cells[5]);
				auto result = std::find(begin, end, str);
				auto index = (int)std::distance(begin, result);
				layer.position = static_cast<RebarPosition>(index);
			}

			//// 钢筋颜色 
			//{
			//	CString strCellValue = cells[9];
			//	layer.rebarColor = _ttoi(strCellValue);
			//}
			// 钢筋线形
			{
				CString strCellValue = cells[6];
				layer.rebarLineStyle = _ttoi(strCellValue);
			}
			// 钢筋线宽
			{
				CString strCellValue = cells[7];
				layer.rebarWeight = _ttoi(strCellValue);
			}
		}
	}

	BEGIN_MESSAGE_MAP(SettingsMainRebarPage, CDialogEx)
	ON_BN_CLICKED(IDC_HOLE_CHECK, &SettingsMainRebarPage::on_hole_check_box_clicked)
	ON_EN_KILLFOCUS(IDC_EDIT4, &SettingsMainRebarPage::on_layer_edit_killed_focus)
		ON_BN_CLICKED(IDC_RebarTemplate, &SettingsMainRebarPage::OnBnClickedRebartemplate)
		ON_BN_CLICKED(IDC_UpdataDlg, &SettingsMainRebarPage::OnBnClickedUpdatadlg)
		ON_BN_CLICKED(IDC_BUTTON2, &SettingsMainRebarPage::OnBnClickedButton2)
	END_MESSAGE_MAP()

	// GallerySettingsMainRebarPage 消息处理程序

	GallerySettings SettingsMainRebarPage::read_settings()
	{
		auto settings = GallerySettings::from_default();

		CString str;

		// 读取正面保护层厚度
		this->m_front_edit.GetWindowTextW(str);
		settings.front_cover = atof(CT2A(str));

		// 读取侧面保护层厚度
		this->m_side_edit.GetWindowTextW(str);
		settings.side_cover = atof(CT2A(str));

		// 读取背面保护层厚度
		this->m_back_edit.GetWindowTextW(str);
		settings.back_cover = atof(CT2A(str));

		// 读取孔洞大小设置
		this->m_hole_size_edit.GetWindowTextW(str);
		settings.hole_size = atof(CT2A(str));

		// 读取孔洞设置
		settings.handle_hole = (bool)this->m_hole_check.GetCheck();

		// 读取钢筋层设置
		read_rebar_list(this->m_main_rebar_list, settings);

		return settings;
	}

	void SettingsMainRebarPage::update_settings(const GallerySettings &settings)
	{
		// 初始化界面
		// 初始化钢筋保护层数据到界面上
		CString front_string, side_string, back_string;
		front_string.Format(_T("%.2f"), settings.front_cover);
		side_string.Format(_T("%.2f"), settings.side_cover);
		back_string.Format(_T("%.2f"), settings.back_cover);

		this->m_front_edit.SetWindowTextW(front_string);
		this->m_side_edit.SetWindowTextW(side_string);
		this->m_back_edit.SetWindowTextW(back_string);

		// 初始化孔洞设置到界面上
		CString hole_size_string;
		hole_size_string.Format(_T("%.2f"), settings.hole_size);

		this->m_hole_size_edit.SetWindowTextW(hole_size_string);

		// 初始化孔洞设置
		auto handle_hole = settings.handle_hole;
		this->m_hole_check.SetCheck(handle_hole);
		// 根据是否需要规避孔洞，禁用孔洞大小文本框
		this->m_hole_size_edit.SetReadOnly(!handle_hole);

		// 初始化钢筋层数设置
		CString layer_string;
		layer_string.Format(_T("%d"), settings.layer_count);
		this->m_layer_edit.SetWindowTextW(layer_string);

		// 在表中插入行
		update_rebar_list(this->m_main_rebar_list, settings.layers, settings.layer_count);
	}


	// 设置默认的钢筋层数据
	// rebarLevleNum : 钢筋层数
	void SettingsMainRebarPage::SetDefaultData(int rebarLevelNum)
	{
		if (rebarLevelNum - (int)m_vecRebarData.size() != 0)
		{
			m_vecRebarData.clear();//添加了墙后位置后，添加此代码
		}
		if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
		{
			if (rebarLevelNum > 0)
			{
				int midpos = rebarLevelNum / 2;
				for (int i = 0; i < rebarLevelNum; ++i)
				{
					ConcreteRebar oneRebarData;
					if (i < midpos)//前半部分
					{
						int dir = (i) & 0x01;
						oneRebarData = { i + 1,dir,"12",2,150 };
						
						oneRebarData.datachange = 0;
						oneRebarData.rebarLineStyle = 0;
						oneRebarData.rebarWeight = 0;
					}
					else//后半部分
					{
						int dir = (i + 1) & 0x01;
						oneRebarData = { i,dir,"12",2,150 };
						oneRebarData.rebarLevel = rebarLevelNum - i;
						oneRebarData.datachange = 2;
					}

					m_vecRebarData.push_back(oneRebarData);
				}
			}

		}
		else //往后删除数据或添加默认数据
		{
			int iOffset = rebarLevelNum - (int)m_vecRebarData.size();
			if (iOffset > 0)
			{
				for (int i = 0; i < iOffset; i++)
				{
					int dir = (i + 1) & 0x01;

					ConcreteRebar oneRebarData = { i,dir,"12",2,150};
					m_vecRebarData.push_back(oneRebarData);
				}
			}
			if (iOffset < 0)
			{
				iOffset *= -1;
				for (int i = 0; i < iOffset; i++)
				{
					m_vecRebarData.pop_back();
				}
			}
		}
		
	}

	// 切换孔洞规避
	void SettingsMainRebarPage::on_hole_check_box_clicked()
	{
		auto checked = (bool)this->m_hole_check.GetCheck();

		// 禁用孔洞大小文本框
		this->m_hole_size_edit.SetReadOnly(!checked);
	}

	void SettingsMainRebarPage::on_layer_edit_killed_focus()
	{
		// 临时的设置, 用于读取钢筋层设置
		//GallerySettings settings;
		//// read current rebar list
		//read_rebar_list(this->m_main_rebar_list, settings);

		//CString str;
		//this->m_layer_edit.GetWindowTextW(str);

		//auto old_layer_count = settings.layer_count;
		//auto new_layer_count = atoi(CT2A(str));

		//auto diff = new_layer_count - old_layer_count;

		//if (diff < 0)
		//{
		//	// 需要删除
		//	settings.remove_layer(-diff);
		//}
		//else
		//{
		//	// 需要增加
		//	settings.add_default_layer(diff);
		//}

		//update_rebar_list(this->m_main_rebar_list, settings.layers, settings.layer_count);

		CString strlevelnum;
		GetDlgItemText(IDC_EDIT4, strlevelnum);
		int rebarLevelNum = atoi(CT2A(strlevelnum));
		
		SetDefaultData(rebarLevelNum);
		m_main_rebar_list.DeleteAllItems();
		for (int i = 0; i < m_vecRebarData.size(); ++i)
		{
			CString strValue;
			strValue.Format(_T("%dL"), m_vecRebarData[i].rebarLevel);
			m_main_rebar_list.InsertItem(i, _T("")); // 插入行
			m_main_rebar_list.SetItemText(i, 0, strValue);
			int nSubCnt = m_main_rebar_list.GetColumnCount() - 1;
			for (int j = 1; j <= nSubCnt; ++j)
			{
				CString strValue;
				switch (j)
				{
				case 1:
				{
					ListCtrlEx::CStrList strlist = g_listRebarDir;
					if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2) //放射
					{
						strlist = g_listRebarRadiateDir;
					}
					m_main_rebar_list.SetCellStringList(i, j, strlist);
					auto it = strlist.begin();
					advance(it, m_vecRebarData[i].rebarDir);
					strValue = *it;
				}
				break;
				case 2:
				{
					ListCtrlEx::CStrList strlist = g_listRebarSize;
					m_main_rebar_list.SetCellStringList(i, j, strlist);
					strValue = m_vecRebarData[i].rebarSize;
					strValue += _T("mm");
				}
				break;
				case 3:
				{
					ListCtrlEx::CStrList strlist = g_listRebarType;
					m_main_rebar_list.SetCellStringList(i, j, strlist);
					auto it = strlist.begin();
					advance(it, m_vecRebarData[i].rebarType);
					strValue = *it;
				}
				break;
				case 4:
						strValue.Format(_T("%.2f"), m_vecRebarData[i].spacing);
					break;
				case 5:
				{
					ListCtrlEx::CStrList strlist = g_listRebarPose;
					m_main_rebar_list.SetCellStringList(i, j, strlist);
					auto it = strlist.begin();
					if (m_vecRebarData[i].datachange > 2)
					{
						m_vecRebarData[i].datachange = 0;
					}
					advance(it, m_vecRebarData[i].datachange);
					strValue = *it;
				}
				break;
				case 6:
				{
					std::list<CString> Rebar_LineStyle;
					for (int i = 0; i < 8; ++i)
					{
						CString tmpCStr;
						tmpCStr.Format(_T("%d"), i);
						Rebar_LineStyle.push_back(tmpCStr);
					}
					m_main_rebar_list.SetCellStringList(i, j, Rebar_LineStyle);
					auto it = Rebar_LineStyle.begin();
					advance(it, m_vecRebarData[i].rebarLineStyle);
					strValue = *it;
				}
				break;
				case 7:
				{
					std::list<CString> Rebar_Weight;
					for (int i = 0; i < 32; ++i)
					{
						CString tmpCStr;
						tmpCStr.Format(_T("%d"), i);
						Rebar_Weight.push_back(tmpCStr);
					}
					m_main_rebar_list.SetCellStringList(i, j, Rebar_Weight);
					auto it = Rebar_Weight.begin();
					advance(it, m_vecRebarData[i].rebarWeight);
					strValue = *it;
				}
				break;
				default:
					break;
				}
				m_main_rebar_list.SetItemText(i, j, strValue);
			}

		}


	}
}


void Gallery::SettingsMainRebarPage::OnBnClickedRebartemplate()
{
	// TODO: 在此添加控件通知处理程序代码
	// 1、获取当前界面保护层等数据
	CString strpostive;
	GetDlgItemText(IDC_EDIT1, strpostive);
	CString strside;
	GetDlgItemText(IDC_EDIT2, strside);
	CString strreverse;
	GetDlgItemText(IDC_EDIT3, strreverse);
	CString strholesize;
	GetDlgItemText(IDC_MHOLESIZE_EDIT, strholesize);
	CString strlevelnum;
	GetDlgItemText(IDC_EDIT4, strlevelnum);

	// 2、将其数据保存到参数模板类
	m_dlgData.postiveCover = _ttof(strpostive);
	m_dlgData.sideCover = _ttof(strside);
	m_dlgData.reverseCover = _ttof(strreverse);
	m_dlgData.missHoleSize = _ttof(strholesize);
	m_dlgData.rebarLevelNum = _ttoi(strlevelnum);
	m_RebarTemplate.Set_m_dlgData(m_dlgData);

	// 3、将表格数据保存到参数模板类
	vector<PIT::ConcreteRebar> vecListData;
	m_main_rebar_list.GetAllRebarData_wall(vecListData);
	m_RebarTemplate.Set_m_vecRebarData(vecListData);

	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_RebarTemplate.Set_isWall();
	m_RebarTemplate.Create(IDD_DIALOG_RebarTemplate);
	m_RebarTemplate.SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);//使窗口总是在最前面
	m_RebarTemplate.ShowWindow(SW_SHOW);

}


void Gallery::SettingsMainRebarPage::OnBnClickedUpdatadlg()
{
	// TODO: 在此添加控件通知处理程序代码
	PIT::DlgData main_dlg = m_RebarTemplate.m_Get_dlgData;
	CString str_postive;
	str_postive.Format(L"%.2f", main_dlg.postiveCover);
	//m_EditPositive.SetWindowTextW(str_postive);
	GetDlgItem(IDC_EDIT1)->SetWindowTextW(str_postive);
	g_wallRebarInfo.concrete.postiveCover = main_dlg.postiveCover;

	CString str_side;
	str_side.Format(L"%.2f", main_dlg.sideCover);
	//m_EditSide.SetWindowTextW(str_side);
	GetDlgItem(IDC_EDIT2)->SetWindowTextW(str_side);
	g_wallRebarInfo.concrete.sideCover = main_dlg.sideCover;

	CString str_reverse;
	str_reverse.Format(L"%.2f", main_dlg.reverseCover);
	//m_EditReverse.SetWindowTextW(str_reverse);
	GetDlgItem(IDC_EDIT3)->SetWindowTextW(str_reverse);
	g_wallRebarInfo.concrete.reverseCover = main_dlg.reverseCover;

	CString str_holesize;
	str_holesize.Format(L"%.2f", main_dlg.missHoleSize);
	//m_mholesize_edit.SetWindowTextW(str_holesize);
	GetDlgItem(IDC_MHOLESIZE_EDIT)->SetWindowTextW(str_holesize);
	g_wallRebarInfo.concrete.MissHoleSize = main_dlg.missHoleSize;

	CString str_levelnum;
	str_levelnum.Format(L"%d", main_dlg.rebarLevelNum);
	//m_EditLevel.SetWindowTextW(str_levelnum);
	GetDlgItem(IDC_EDIT4)->SetWindowTextW(str_levelnum);
	g_wallRebarInfo.concrete.rebarLevelNum = main_dlg.rebarLevelNum;



	// 2.设置表格数据
	m_main_rebar_list.DeleteAllItems();
	std::vector<PIT::ConcreteRebar> main_listdata = m_RebarTemplate.m_Get_vecRebarData;
	m_RebarTemplate.m_Get_vecRebarData.clear();
	for (int i = 0; i < main_listdata.size(); ++i)
	{
		CString strValue;
		strValue.Format(_T("%dL"), main_listdata[i].rebarLevel);
		m_main_rebar_list.InsertItem(i, _T("")); // 插入行
		m_main_rebar_list.SetItemText(i, 0, strValue);
		int nSubCnt = m_main_rebar_list.GetColumnCount() - 1;
		for (int j = 1; j <= nSubCnt; ++j)
		{
			CString strValue;
			switch (j)
			{
			case 1:
			{
				ListCtrlEx::CStrList strlist = g_listRebarDir;
				if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2) //放射
				{
					strlist = g_listRebarRadiateDir;
				}
				m_main_rebar_list.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, main_listdata[i].rebarDir);
				strValue = *it;
			}
			break;
			case 2:
			{
				ListCtrlEx::CStrList strlist = g_listRebarSize;
				m_main_rebar_list.SetCellStringList(i, j, strlist);
				strValue = main_listdata[i].rebarSize;
				strValue += _T("mm");
			}
			break;
			case 3:
			{
				ListCtrlEx::CStrList strlist = g_listRebarType;
				m_main_rebar_list.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, main_listdata[i].rebarType);
				strValue = *it;
			}
			break;
			case 4:
				if (g_wallRebarInfo.concrete.m_SlabRebarMethod == 2 && main_listdata[i].rebarDir == 1)
				{
					strValue.Format(_T("%.2f"), main_listdata[i].angle);
				}
				else
				{
					strValue.Format(_T("%.2f"), main_listdata[i].spacing);
				}
				break;
			/*case 5:
				strValue.Format(_T("%.2f"), main_listdata[i].startOffset);
				break;
			case 6:
				strValue.Format(_T("%.2f"), main_listdata[i].endOffset);
				break;
			case 7:
				strValue.Format(_T("%.2f"), main_listdata[i].levelSpace);
				break;*/
			case 5:
			{
				//之前为数据交换的功能
				/*ListCtrlEx::CStrList strlist;
				strlist.push_back(_T("无"));
				for (int i = 0; i < g_wallRebarInfo.concrete.rebarLevelNum; ++i)
				{
					CString strValue;
					if (0 == i)
						strValue = _T("1XL");
					else
						strValue.Format(_T("%dL"), i);
					strlist.push_back(strValue);
				}
				m_main_rebar_list.SetCellStringList(i, j, strlist);
				strValue = _T("无");*/
				ListCtrlEx::CStrList strlist = g_listRebarPose;
				m_main_rebar_list.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				if (main_listdata[i].datachange > 2)
				{
					main_listdata[i].datachange = 0;
				}
				advance(it, main_listdata[i].datachange);
				strValue = *it;
			}
			break;
			//case 9:
			//{
			//	std::list<CString> RebarColor;
			//	for (int i = -1; i < 256; ++i)
			//	{
			//		CString tmpCStr;
			//		tmpCStr.Format(_T("%d"), i);
			//		RebarColor.push_back(tmpCStr);
			//	}
			//	m_main_rebar_list.SetCellStringList(i, j, RebarColor);
			//	auto it = RebarColor.begin();
			//	advance(it, main_listdata[i].rebarColor);
			//	strValue = *it;
			//	//strValue = _T("-1");
			//}
			//	break;
			case 6:
			{
				std::list<CString> Rebar_LineStyle;
				for (int i = 0; i < 8; ++i)
				{
					CString tmpCStr;
					tmpCStr.Format(_T("%d"), i);
					Rebar_LineStyle.push_back(tmpCStr);
				}
				m_main_rebar_list.SetCellStringList(i, j, Rebar_LineStyle);
				auto it = Rebar_LineStyle.begin();
				advance(it, main_listdata[i].rebarLineStyle);
				strValue = *it;
				//strValue = _T("0");
			}
			break;
			case 7:
			{
				std::list<CString> Rebar_Weight;
				for (int i = 0; i < 32; ++i)
				{
					CString tmpCStr;
					tmpCStr.Format(_T("%d"), i);
					Rebar_Weight.push_back(tmpCStr);
				}
				m_main_rebar_list.SetCellStringList(i, j, Rebar_Weight);
				auto it = Rebar_Weight.begin();
				advance(it, main_listdata[i].rebarWeight);
				strValue = *it;
				//strValue = _T("0");
			}
			break;
			default:
				break;
			}
			m_main_rebar_list.SetItemText(i, j, strValue);
		}

	}
	//g_vecRebarData = main_listdata;
	main_listdata.clear();

	//CTabCtrl* pTabCtrl = dynamic_cast<CTabCtrl*>(GetParent());
	//if (pTabCtrl != nullptr)
	//{
	//	CWallRebarDlg* pDlg = dynamic_cast<CWallRebarDlg*>(pTabCtrl->GetParent());
	//	if (pDlg)
	//	{
	//		//			pDlg->m_PageLapOption.UpdateLapOptionList();
	//		pDlg->m_PageEndType.UpdateEndTypeList();
	//		pDlg->m_PageTwinBars.UpdateTwinBarsList();
	//	}
	//	else
	//	{
	//		CSlabRebarDlg* pDlg = dynamic_cast<CSlabRebarDlg*>(pTabCtrl->GetParent());
	//		if (pDlg)
	//		{
	//			//			pDlg->m_PageLapOption.UpdateLapOptionList();
	//			pDlg->m_PageEndType.UpdateEndTypeList();
	//			pDlg->m_PageTwinBars.UpdateTwinBarsList();
	//		}
	//	}
	//}
}


void Gallery::SettingsMainRebarPage::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
}
