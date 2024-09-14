
// ListComboBox.cpp : implementation file  
//  

#include "_ustation.h"
#include "CListComboBox.h"
#include "CEditListCtrl.h"  


// CListComboBox  

IMPLEMENT_DYNAMIC(CListComboBox, CComboBox)

CListComboBox::CListComboBox()
{

}

CListComboBox::~CListComboBox()
{
}


BEGIN_MESSAGE_MAP(CListComboBox, CComboBox)
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, &CListComboBox::OnCbnDropdown)
END_MESSAGE_MAP()



// CListComboBox message handlers  

void CListComboBox::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);
	//ShowWindow(SW_HIDE);  

	CWallRebarEditListCtrl *temp = (CWallRebarEditListCtrl*)GetParent();
	temp->DisposeEdit(); //���ø����ڵ�DisposeEdit()����  
}

void CListComboBox::OnCbnDropdown()
{
	CClientDC dc(this);
	int nTotalHeight = 0;
	//��ȡ������Ϣ  
	dc.SelectObject(GetFont());
	//��ȡ��ǰitem�ĸ���  
	int nCount = GetCount();
	if (nCount <= 0) return;
	//��ȡ����ĸ߶�  
	CString strLable = _T("");
	GetLBText(GetCurSel(), strLable);
	int nHeight = dc.GetTextExtent(strLable).cy;

	//��Ͽ�߶�  
	CRect rect;
	GetWindowRect(rect);
	int height = rect.Height();

	if (nCount > 30)
		nTotalHeight = 30 * nHeight + height;
	else
		nTotalHeight = nCount * nHeight + height;

	//���������ĸ߶�  
	CRect rc;
	GetClientRect(&rc);
	SetWindowPos(NULL, 0, 0, rc.Width(), rc.Height() + nTotalHeight, SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);
}


IMPLEMENT_DYNAMIC(CLapOptionListComboBox, CComboBox)

CLapOptionListComboBox::CLapOptionListComboBox()
{

}

CLapOptionListComboBox::~CLapOptionListComboBox()
{
}


BEGIN_MESSAGE_MAP(CLapOptionListComboBox, CComboBox)
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, &CLapOptionListComboBox::OnCbnDropdown)
END_MESSAGE_MAP()

void CLapOptionListComboBox::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);

	CRebarLapOptionEditListCtrl *temp = (CRebarLapOptionEditListCtrl*)GetParent();
	temp->DisposeEdit(); //���ø����ڵ�DisposeEdit()����  
}

void CLapOptionListComboBox::OnCbnDropdown()
{
	CListComboBox::OnCbnDropdown();
}


IMPLEMENT_DYNAMIC(CEndTypeListComboBox, CComboBox)

CEndTypeListComboBox::CEndTypeListComboBox()
{

}

CEndTypeListComboBox::~CEndTypeListComboBox()
{
}

BEGIN_MESSAGE_MAP(CEndTypeListComboBox, CComboBox)
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, &CEndTypeListComboBox::OnCbnDropdown)
END_MESSAGE_MAP()

void CEndTypeListComboBox::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);

	CRebarEndTypeEditListCtrl *temp = (CRebarEndTypeEditListCtrl*)GetParent();
	temp->DisposeEdit(); //���ø����ڵ�DisposeEdit()����  
}

void CEndTypeListComboBox::OnCbnDropdown()
{
	CListComboBox::OnCbnDropdown();
}


IMPLEMENT_DYNAMIC(CACListComboBox, CComboBox)

CACListComboBox::CACListComboBox()
{
}

CACListComboBox::~CACListComboBox()
{
}

BEGIN_MESSAGE_MAP(CACListComboBox, CComboBox)
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, &CACListComboBox::OnCbnDropdown)
END_MESSAGE_MAP()

void CACListComboBox::OnKillFocus(CWnd* pNewWnd)
{
	CComboBox::OnKillFocus(pNewWnd);

	CACListCtrl *temp = (CACListCtrl*)GetParent();
	temp->DisposeEdit(); //���ø����ڵ�DisposeEdit()����  
}

void CACListComboBox::OnCbnDropdown()
{
	CListComboBox::OnCbnDropdown();
}