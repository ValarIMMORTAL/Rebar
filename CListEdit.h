#pragma once  


// CListEdit  

class CListEdit : public CEdit
{
	DECLARE_DYNAMIC(CListEdit)

public:
	CListEdit();
	virtual ~CListEdit();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
protected:
	virtual void PreSubclassWindow();
};

class CLapOptionListEdit : public CEdit
{
	DECLARE_DYNAMIC(CLapOptionListEdit)

public:
	CLapOptionListEdit();
	virtual ~CLapOptionListEdit();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
protected:
	virtual void PreSubclassWindow();
};

class CEndTypeListEdit : public CEdit
{
	DECLARE_DYNAMIC(CEndTypeListEdit)

public:
	CEndTypeListEdit();
	virtual ~CEndTypeListEdit();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
protected:
	virtual void PreSubclassWindow();
};

class CACListEdit : public CEdit
{
	DECLARE_DYNAMIC(CACListEdit)

public:
	CACListEdit();
	virtual ~CACListEdit();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
protected:
	virtual void PreSubclassWindow();
};


class CTwinBarListEdit : public CEdit
{
	DECLARE_DYNAMIC(CTwinBarListEdit)

public:
	CTwinBarListEdit();
	virtual ~CTwinBarListEdit();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
protected:
	virtual void PreSubclassWindow();
};
