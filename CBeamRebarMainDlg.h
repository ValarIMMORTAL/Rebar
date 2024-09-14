#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include "CBeamVerticalRebarDlg.h"
#include "CBeamHoopRebarDlg.h"
#include "CBeamDefaultDlg.h"
#include "CBeamBaseInfoDlg.h"

// CBeamRebarMainDlg 对话框

class CBeamRebarAssembly;
class CBeamRebarMainDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBeamRebarMainDlg)

public:
	CBeamRebarMainDlg(ElementHandle ehSel, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CBeamRebarMainDlg();
protected:
	virtual void PostNcDestroy();
public:
	virtual BOOL OnInitDialog();

	void SetRebarAssembly(CBeamRebarAssembly* pBeamRebarAssembly)
	{
		m_pBeamRebarAssembly = pBeamRebarAssembly;
	}

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum {IDD = IDD_DIALOG_BeamRebarMainDlg};
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

private:
	CTabCtrl				m_tab;
	CDialog*				p_Dialog[4];				// 用来保存对话框对象指
	CBeamHoopRebarDlg	    m_PageHoopRebarDlg;
	CBeamVerticalRebarDlg   m_PageVerticalRebarDlg;
	CBeamDefaultDlg			m_PageDefaultDlg;
	CBeamBaseInfoDlg		m_PageBaseDlg;
	ElementHandle			m_ehSel;
	int						m_CurSelTab;

	CBeamRebarAssembly*		m_pBeamRebarAssembly;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
