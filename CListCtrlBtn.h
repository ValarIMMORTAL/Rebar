#pragma once
#include <afxwin.h>
#define  WM_ENDPROP_CLICK  WM_USER + 100
#define  WM_CLEAR_CLICK  WM_USER + 101

class CListCtrlBtn :public CButton
{
	DECLARE_DYNAMIC(CListCtrlBtn)
public:
	CListCtrlBtn();
	CListCtrlBtn(int type, int nItem, int nSubItem, CRect rect, HWND hParent);
	~CListCtrlBtn();
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
	bool m_bSelected;
};