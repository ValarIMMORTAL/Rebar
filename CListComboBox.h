#pragma once  


// CListComboBox  

class CListComboBox : public CComboBox
{
	DECLARE_DYNAMIC(CListComboBox)

public:
	CListComboBox();
	virtual ~CListComboBox();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnCbnDropdown();
};

class CLapOptionListComboBox : public CListComboBox
{
	DECLARE_DYNAMIC(CLapOptionListComboBox)

public:
	CLapOptionListComboBox();
	virtual ~CLapOptionListComboBox();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnCbnDropdown();
};

class CEndTypeListComboBox : public CListComboBox
{
	DECLARE_DYNAMIC(CEndTypeListComboBox)

public:
	CEndTypeListComboBox();
	virtual ~CEndTypeListComboBox();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnCbnDropdown();
};

class CACListComboBox : public CListComboBox
{
	DECLARE_DYNAMIC(CACListComboBox)

public:
	CACListComboBox();
	virtual ~CACListComboBox();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnCbnDropdown();
};