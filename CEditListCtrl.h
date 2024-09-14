#pragma once  
#include "CListEdit.h"  
#include "CListComboBox.h"  
#include "CListCtrlBtn.h"
#include "CommonFile.h"
#include "ListCtrlEx.h"
#define  IDC_CELL_EDIT      0xffe0  
#define  IDC_CELL_COMBOBOX  0xffe1  

// CEditListCtrl  

class CEditListCtrl : public ListCtrlEx::CListCtrlEx
{
	DECLARE_DYNAMIC(CEditListCtrl)

public:
	CEditListCtrl();
	virtual ~CEditListCtrl();

protected:
	DECLARE_MESSAGE_MAP()

private:
	CListEdit m_edit;
	CListComboBox m_comboBox;
	int m_nRow; //��  
	int m_nCol; //��  

public:

	// ���༭��ɺ�ͬ������  
	void DisposeEdit(void);

	//����ʧЧ��Ϣ  
	void SendInvalidateMsg();

	// ���õ�ǰ���ڵķ��  
	void SetStyle(void);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnCbnSelchangeCbCellComboBox();

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};


class CWallRebarEditListCtrl : public ListCtrlEx::CListCtrlEx
{
	DECLARE_DYNAMIC(CWallRebarEditListCtrl)

public:
	CWallRebarEditListCtrl() {};
	virtual ~CWallRebarEditListCtrl() {};

protected:
	DECLARE_MESSAGE_MAP()

private:
	CListEdit m_edit;
	CListComboBox m_comboBox;
	CFont	m_Font;

	int m_nRow; //��  
	int m_nCol; //��  

public:
	void GetAllRebarData(std::vector<PIT::ConcreteRebar> &vecListData);
	// ���༭��ɺ�ͬ������  
	void DisposeEdit(void);

	//����ʧЧ��Ϣ
	void SendInvalidateMsg();

	void SetListEditShowWindow(UINT state);
	void SetListComboBoxShowWindow(UINT state);
	// ���õ�ǰ���ڵķ��  
	void SetStyle(void);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnCbnSelchangeCbCellComboBox();

// 	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
// 	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
// 	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};

class CRebarLapOptionEditListCtrl : public ListCtrlEx::CListCtrlEx
{
	DECLARE_DYNAMIC(CRebarLapOptionEditListCtrl)

public:
	CRebarLapOptionEditListCtrl() {};
	virtual ~CRebarLapOptionEditListCtrl() {};

protected:
	DECLARE_MESSAGE_MAP()

private:
	CLapOptionListEdit m_edit;
	CLapOptionListComboBox m_comboBox;
	CFont	m_Font;

	int m_nRow; //��  
	int m_nCol; //��  

public:
	void GetAllRebarData(std::vector<PIT::LapOptions> &vecListData);
	// ���༭��ɺ�ͬ������  
	void DisposeEdit(void);

	//����ʧЧ��Ϣ
	void SendInvalidateMsg();

	void SetListEditShowWindow(UINT state);
	void SetListComboBoxShowWindow(UINT state);
	// ���õ�ǰ���ڵķ��  
	void SetStyle(void);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnCbnSelchangeCbCellComboBox();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};

class CRebarEndTypeEditListCtrl : public ListCtrlEx::CListCtrlEx
{
	DECLARE_DYNAMIC(CRebarEndTypeEditListCtrl)

public:
	CRebarEndTypeEditListCtrl() {};
	virtual ~CRebarEndTypeEditListCtrl() {
		for (size_t i = 0; i < m_vecBtnEndProp.size(); ++i)//�ڴ��ͷ�����
		{
			m_vecBtnEndProp[i]->DestroyWindow();
			delete m_vecBtnEndProp[i];
			m_vecBtnEndProp[i] = NULL;

		}
		for (size_t i = 0; i < m_vecBtnClear.size(); ++i)//�ڴ��ͷ�����
		{
			m_vecBtnClear[i]->DestroyWindow();
			delete m_vecBtnClear[i];
			m_vecBtnClear[i] = NULL;
		}
	};

protected:
	DECLARE_MESSAGE_MAP()

private:
	CEndTypeListEdit m_edit;
	CEndTypeListComboBox m_comboBox;
	std::vector<CListCtrlBtn*> m_vecBtnEndProp;
	std::vector<CListCtrlBtn*> m_vecBtnClear;
	CFont	m_Font;

	int m_nRow; //��  
	int m_nCol; //��  


public:
	int m_nCreatBtnCol;

public:
	void AddEndPropButton(int nItem, int nSubItem,const CString& btnText, HWND hMain);
	void AddClearButton(int nItem, int nSubItem, const CString& btnText, HWND hMain);
public:
	void GetAllRebarData(std::vector<PIT::EndType> &vecListData);
	// ���༭��ɺ�ͬ������  
	void DisposeEdit(void);

	//����ʧЧ��Ϣ
	void SendInvalidateMsg();

	void SetListEditShowWindow(UINT state);
	void SetListComboBoxShowWindow(UINT state);
	// ���õ�ǰ���ڵķ��  
	void SetStyle(void);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnCbnSelchangeCbCellComboBox();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};

class CACListCtrl : public ListCtrlEx::CListCtrlEx
{
	DECLARE_DYNAMIC(CACListCtrl)

public:
	CACListCtrl() {};
	virtual ~CACListCtrl() {};

protected:
	DECLARE_MESSAGE_MAP()

private:
	CACListComboBox m_comboBox;
	CFont			m_Font;

	int m_nRow; //��  
	int m_nCol; //��  

public:
	void GetAllRebarData(std::vector<PIT::AssociatedComponent> &vecListData);
	// ���༭��ɺ�ͬ������  
	void DisposeEdit(void);

	//����ʧЧ��Ϣ
	void SendInvalidateMsg();

//	void SetListEditShowWindow(UINT state);
	void SetListComboBoxShowWindow(UINT state);
	// ���õ�ǰ���ڵķ��  
	void SetStyle(void);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnCbnSelchangeCbCellComboBox();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
