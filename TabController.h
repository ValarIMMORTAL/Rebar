#pragma once

#include <afxcmn.h>
#include <vector>

/// @brief �����Զ�����Tabҳ����л�����������tab/page����Դ
class TabController {
public:
    TabController(CTabCtrl &tab);

    TabController(const TabController &) = delete;
    TabController(TabController &&) = delete;
    ~TabController() = default;

    /// @brief ���һ��dialog��tabҳ����
    void create_page(CDialog &page, uint32_t template_id, LPCTSTR title);

    /// @brief ѡ��ҳ��
    /// @exception out_of_range: index������Χ���׳�
    void update_selection();

    /// @brief ��õ�ǰѡ�е�ҳ��
    /// @exception out_of_range: û��pageʱ���׳�
    CDialog& current() const;

private:
    void show_current();

private:
    CTabCtrl *m_tab;
    std::vector<CDialog*> m_pages;
};

