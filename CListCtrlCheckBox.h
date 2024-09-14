#pragma once

#define  WM_CHECKBOX_CLICK  WM_USER + 500


class CListCtrlCheckBox : public CCheckListBox
{
	DECLARE_DYNAMIC(CListCtrlCheckBox)
public:
	CListCtrlCheckBox();
	CListCtrlCheckBox(int type, int nItem, int nSubItem, CRect rect, HWND hParent);
	~CListCtrlCheckBox();
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnBnClicked();
	int m_inItem;
	int m_inSubItem;
	CRect m_rect;
	HWND m_hParent;
	int  m_type;//按钮类型 用于处理消息 区分按钮信息
	BOOL bEnable;
};

