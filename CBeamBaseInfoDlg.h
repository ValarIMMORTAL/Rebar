#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"

// CBeamBaseInfoDlg 对话框

class CBeamBaseInfoDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBeamBaseInfoDlg)

public:
	CBeamBaseInfoDlg(ElementHandle ehSel, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CBeamBaseInfoDlg();

	BOOL OnInitDialog();

	void SetDefaultData(BeamRebarInfo::BeamBaseData& stBeamBaseData);

	void SetBeamBaseData(ElementId contid);

	BeamRebarInfo::BeamBaseData					m_stBeamBaseData;

private:
	CEdit							m_EditWidth;	  // 宽度
	CEdit							m_EditDepth;	  // 深度
	CEdit							m_EditLeftSide;	  // 左侧支撑
	CEdit							m_EditRightSide;  // 右侧支撑
	CEdit							m_EditLeftXLen;   // 左侧X距离
	CEdit							m_EditRightXLen;  // 右侧X距离
	CEdit							m_EditNetSpan;	  // 净跨度
	CEdit							m_EditAxisToAxis; // 轴线到轴线

	ElementHandle						m_ehSel;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_BeamInfo };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnEnChangeEdit3();
	afx_msg void OnEnChangeEdit4();
	afx_msg void OnEnChangeEdit5();
	afx_msg void OnEnChangeEdit6();
	afx_msg void OnEnChangeEdit7();
	afx_msg void OnEnChangeEdit8();
};
