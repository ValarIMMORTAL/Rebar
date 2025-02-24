#pragma once
#include "../../ListCtrlEx.h"
#include "../../CommonFile.h"

class CMainRebarListCtrlNew :public ListCtrlEx::CListCtrlEx
{
public:
	void GetAllRebarData(vector<PIT::ConcreteRebar>& vecListData);

	DECLARE_MESSAGE_MAP()
	// message handlers
	afx_msg void OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
};

class CRebarEndTypeListCtrlNew :public ListCtrlEx::CListCtrlEx
{
public:
	void GetAllRebarData(vector<PIT::EndType>& vecListData);
};

class CCNCutRebarList :public ListCtrlEx::CListCtrlEx
{
public:
	void GetAllRebarData(vector<CNCutRebarInfo>& vecListData);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
};


class CTwinBarListCtrlNew :public ListCtrlEx::CListCtrlEx
{
public:
	void GetAllRebarData(vector<TwinBarSet::TwinBarLevelInfo>& vecListData);
};

class CWallRebarAssociatedCtrlNew :public ListCtrlEx::CListCtrlEx
{ 
public:
	void GetAllRebarData(vector<PIT::AssociatedComponent>& vecListData);

public:
	
	ElementAgenda  affectedElements;//树形对话框中高亮选择的元素
	DECLARE_MESSAGE_MAP()
	// message handlers
	afx_msg void		OnLButtonDown(UINT nFlags, CPoint point);
};

