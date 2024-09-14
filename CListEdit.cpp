
// ListEdit.cpp : implementation file  
//  

#include "_ustation.h"
#include "CListEdit.h"
#include "CEditListCtrl.h"  


// CListEdit  

IMPLEMENT_DYNAMIC(CListEdit, CEdit)

CListEdit::CListEdit()
{

}

CListEdit::~CListEdit()
{
}


BEGIN_MESSAGE_MAP(CListEdit, CEdit)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()



// CListEdit message handlers  



void CListEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);

	//ShowWindow(SW_HIDE);  
	CWallRebarEditListCtrl* temp = (CWallRebarEditListCtrl*)GetParent();
	temp->DisposeEdit();

}

void CListEdit::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class  

	CEdit::PreSubclassWindow();
}


IMPLEMENT_DYNAMIC(CLapOptionListEdit, CEdit)

CLapOptionListEdit::CLapOptionListEdit()
{

}

CLapOptionListEdit::~CLapOptionListEdit()
{
}


BEGIN_MESSAGE_MAP(CLapOptionListEdit, CEdit)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()



// CListEdit message handlers  



void CLapOptionListEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);

	//ShowWindow(SW_HIDE);  
	CRebarLapOptionEditListCtrl* temp = (CRebarLapOptionEditListCtrl*)GetParent();
	temp->DisposeEdit();

}

void CLapOptionListEdit::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class  

	CEdit::PreSubclassWindow();
}


IMPLEMENT_DYNAMIC(CEndTypeListEdit, CEdit)

CEndTypeListEdit::CEndTypeListEdit()
{
}

CEndTypeListEdit::~CEndTypeListEdit()
{
}

BEGIN_MESSAGE_MAP(CEndTypeListEdit, CEdit)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

// CListEdit message handlers  
void CEndTypeListEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);

	//ShowWindow(SW_HIDE);  
	CRebarEndTypeEditListCtrl* temp = (CRebarEndTypeEditListCtrl*)GetParent();
	temp->DisposeEdit();

}

void CEndTypeListEdit::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class  

	CEdit::PreSubclassWindow();
}


IMPLEMENT_DYNAMIC(CACListEdit, CEdit)

CACListEdit::CACListEdit()
{
}

CACListEdit::~CACListEdit()
{
}

BEGIN_MESSAGE_MAP(CACListEdit, CEdit)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

// CListEdit message handlers  
void CACListEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);

	//ShowWindow(SW_HIDE);  
	CACListCtrl* temp = (CACListCtrl*)GetParent();
	temp->DisposeEdit();

}

void CACListEdit::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class  

	CEdit::PreSubclassWindow();
}


IMPLEMENT_DYNAMIC(CTwinBarListEdit, CEdit)

CTwinBarListEdit::CTwinBarListEdit()
{
}

CTwinBarListEdit::~CTwinBarListEdit()
{
}
BEGIN_MESSAGE_MAP(CTwinBarListEdit, CEdit)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

// CListEdit message handlers  
void CTwinBarListEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);

	//ShowWindow(SW_HIDE);  
// 	CTwinBarEditListCtrl* temp = (CTwinBarEditListCtrl*)GetParent();
// 	temp->DisposeEdit();

}

void CTwinBarListEdit::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class  

	CEdit::PreSubclassWindow();
}