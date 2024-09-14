﻿#pragma once
#include "CSlabMainRebarDlg.h"
//#include "CWallRebarLapOptionDlg.h"
#include "CSlabRebarEndType.h"
//#include "CWallRebarAssociatedComponentDlg.h"
#include "CSlabTwinBarSetDlg.h"
#include "CSlabTieRebarSetDlg.h"
#include "CWallRebarEndTypeDlg.h"
#include "CTwinBarSetDlg.h"
#include "CTieRebarSetDlg.h"
#include "CWallMainRebarDlg.h"
#include "CUniteMainRebarDlg.h"
#include "GallerySettings.h"

// CUniteRebarDlg 对话框
class MySlabRebarAssembly;
class CUniteRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CUniteRebarDlg)

public:
	CUniteRebarDlg(ElementHandleCR eh, CWnd* pParent = nullptr);
	virtual ~CUniteRebarDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_Combine_Setting};
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CTabCtrl m_tab;
public:
	static void RefreshSlabRebars(ElementId conid, EditElementHandleR eeh);
	void SetSelectElement(ElementAgendaR eh) { m_selectedElement = eh; }
	void SetehSel(ElementHandleCR eh) { ehSel = eh; }
	void ArcRebarMethod(EditElementHandleR eh);
	ElementHandle GetSelectElement() { return ehSel; }
	void SetConcreteId(ElementId id) { m_ConcreteId = id; }
	void PreviewRebarLines();
	void SetIsCombineSlab(bool isCombine) { m_isCombineSlab = isCombine; }
	WallSetInfo m_WallSetInfo;

	void SetArcLineInfo(ArcRebar&  arcLine)
	{
		m_PageMainRebar.mArcLine.centerpt = arcLine.centerpt;

		m_PageMainRebar.mArcLine.ptStart = arcLine.ptStart;
		m_PageMainRebar.mArcLine.ptEnd = arcLine.ptEnd;
		m_PageMainRebar.mArcLine.radius = arcLine.radius;
		m_PageMainRebar.m_height = arcLine.centerpt.z; //当用户画弧线时，没有画在贴合底面的位置：直接将获取到的圆心坐标Z值赋给弧线的z值
		m_PageMainRebar.mArcLine.ArcLen = arcLine.ArcLen;
	}

private:
	ElementHandle ehSel;
	ElementHandle ehhagain;
	ElementId m_ConcreteId;
	ElementAgenda m_selectedElement;   //选中的一段廊道的构建
	MySlabRebarAssembly*  m_slabRebar; //板预览按钮使用
	WallRebarAssembly * m_WallRebarLinesPtr;//墙预览按钮使用

	//EditElementHandle ehes;
	std::vector<PIT::ConcreteRebar>					m_vecRebarData;
	std::vector<PIT::LapOptions>						m_vecLapOptionData;
	std::vector<PIT::EndType>						m_vecEndTypeData;
	std::vector<PIT::AssociatedComponent>			m_vecACData;
	std::vector<TwinBarSet::TwinBarLevelInfo>	m_vecTwinBarData;
	TwinBarSet::TwinBarInfo						m_twinBarInfo;
	TieReBarInfo								m_tieRebarInfo;

	//合并配筋用
	ElementHandle m_eeh1;
	ElementHandle m_eeh2;
protected:
	virtual void PostNcDestroy();
public:
	CComboBox m_ComboSize;//尺寸
	CComboBox m_ComboType;//型号
	CEdit	  m_EditSpace;//间距

	WallSetInfo m_SlabSetInfo;
	int m_ClickedFlag;//点击了点选按钮并且选中墙后设置为1，使得墙配筋界面不会关闭
						  //没有点击点选按钮就选中墙，会设置为0，使得按下确定按钮界面就会关闭

	int repeate = 1;
	int m_CurSelTab;
	CUniteMainRebarDlg					    m_PageMainRebar;			//主要配筋
	CDialog*							pDialog[1];					//用来保存对话框对象指针
	virtual BOOL OnInitDialog();

	afx_msg void OnBnClickedOk();
	Gallery::GallerySettings read_settings();
	bool isslab(EditElementHandleCR element);
	bool iswall(EditElementHandleCR element);
	void on_Clicke_makerSlab();
	void on_Clicke_makerWall();
	afx_msg void OnBnClickedCancel();
	//	afx_msg void OnTcnSelchangeDialogSlabrebar(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTcnSelchangeTabSlabrebar(NMHDR *pNMHDR, LRESULT *pResult);
	CStatic m_static_panelname;
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnEnChangeEdit1();
	CButton m_repeat;
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedSelectModel();

private:
	bool m_isCombineSlab = false; //是否合并板
	CFont m_font; //提示字体
public:
	afx_msg void OnBnClickedSelectModel2();
};
