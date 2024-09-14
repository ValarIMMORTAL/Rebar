#include "_USTATION.h"
#include "TabController.h"

TabController::TabController(CTabCtrl &tab)
    : m_tab(&tab),
      m_pages()
{
}

/// ����tab�е�page����tab�Ĵ�С
void fit_page_in_tab(const CTabCtrl &tab, CDialog &dialog)
{
	CRect rc;
	tab.GetClientRect(rc);
	// ����������ϲ���ǩҳ�ĸ߶�
	rc.top += 20;
	rc.bottom -= 0;
	rc.left += 0;
	rc.right -= 0;

	dialog.MoveWindow(&rc);
}

void TabController::create_page(CDialog &page, uint32_t template_id, LPCTSTR title)
{
    page.Create(template_id, this->m_tab);
    fit_page_in_tab(*this->m_tab, page);


    auto index = this->m_pages.size();
    this->m_tab->InsertItem((int)index, title);

    this->m_pages.push_back(&page);
    
    this->show_current();
}

void TabController::update_selection()
{
    this->show_current();
}

CDialog &TabController::current() const
{
    if (this->m_pages.empty())
    {
        throw std::out_of_range("TabController��û��Page, �޷���õ�ǰѡ���ҳ��");
    }

    return *this->m_pages[this->m_tab->GetCurSel()];
}

/// @brief ��ʾ��ǰ��tabҳ
void TabController::show_current()
{
    auto selected_index = this->m_tab->GetCurSel();
    for (size_t i = 0; i < this->m_pages.size(); ++i)
    {
        auto page = this->m_pages[i];

        // ֻ��ʾ��ǰѡ�е�ҳ��
        if (i == (size_t)selected_index)
        {
            page->ShowWindow(SW_SHOW);
        }
        else
        {
            page->ShowWindow(SW_HIDE);            
        }        
    }
}
