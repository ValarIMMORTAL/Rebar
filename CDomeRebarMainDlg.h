#pragma once
#include "CommonFile.h"
#include "CWallRebarListCtrl.h"
//#include "ElementAlgorithm.h"
#include "CDomeRebarDlg.h"
#include "CDomeRebarAssembly.h"

// CDomeRebarMainDlg 对话框

class CDomeRebarMainDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDomeRebarMainDlg)

public:
	CDomeRebarMainDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDomeRebarMainDlg();

	virtual BOOL OnInitDialog();

	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }

protected:
	virtual void PostNcDestroy();

private:

	CDomeRebarDlg					m_pageDomeRebarDlg;

	DPoint3d						m_circleCenter;

	ElementHandle					m_ehSel;
	double							m_dScaleLength;
	vector<st_VertexVec>			m_vecVertex;

	CTabCtrl						m_tab;

	int								m_CurSelTab;
	CDialog*						pDialog[1];			 //用来保存对话框对象指针

	Transform						m_targetTrans;

	PIT::DomeCoverInfo							 m_stDomeCoverInfo;
	vector<PIT::DomeLevelInfo>					 m_vecDomeLevelInfo;		// 穹顶配筋范围信息
	map<int, vector<PIT::DomeLevelDetailInfo>>   m_mapDomeLevelDetailInfo;  // 穹顶每层钢筋具体信息map

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_DomeMainDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCancel();
};
