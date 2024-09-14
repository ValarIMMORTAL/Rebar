#pragma once

#include <afxcmn.h>
#include <vector>

/// @brief 用于自动控制Tab页面的切换，本身不持有tab/page的资源
class TabController {
public:
    TabController(CTabCtrl &tab);

    TabController(const TabController &) = delete;
    TabController(TabController &&) = delete;
    ~TabController() = default;

    /// @brief 添加一个dialog到tab页面中
    void create_page(CDialog &page, uint32_t template_id, LPCTSTR title);

    /// @brief 选择页面
    /// @exception out_of_range: index超出范围会抛出
    void update_selection();

    /// @brief 获得当前选中的页面
    /// @exception out_of_range: 没有page时会抛出
    CDialog& current() const;

private:
    void show_current();

private:
    CTabCtrl *m_tab;
    std::vector<CDialog*> m_pages;
};

