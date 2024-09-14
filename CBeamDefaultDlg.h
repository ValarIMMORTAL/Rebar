#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"

// CBeamDefaultDlg 对话框

class CBeamDefaultDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBeamDefaultDlg)

public:
	CBeamDefaultDlg(ElementHandle ehSel, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CBeamDefaultDlg();

	BOOL OnInitDialog();

	void SetBeamDefaultData(ElementId condit);

	void SetDefaultData(BeamRebarInfo::BeamDefaultData& stDefaultInfo);

	CString					arrRebarCal[2];

	BeamRebarInfo::BeamDefaultData			m_stBeamDefaultData;
private:
	CEdit					m_EditTop;		 // 上方
	CEdit					m_EditLeft;		 // 左表面
	CEdit					m_EditRight;	 // 右表面
	CEdit					m_EditUnder;     // 下表面
	CEdit					m_EditMargin;	 // 边距
	CEdit					m_EditFloor;	 // 层	
	CComboBox				m_CombRebarCal;  // 钢筋计算

	ElementHandle						m_ehSel;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_BeamDefault };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEdit6();
	afx_msg void OnEnChangeEdit3();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnEnChangeEdit5();
	afx_msg void OnEnChangeEdit4();
};
