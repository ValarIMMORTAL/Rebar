#pragma once
#include "ListCtrlEx.h"

// CircleAndSquare 对话框

class CircleAndSquare : public CDialogEx
{
	DECLARE_DYNAMIC(CircleAndSquare)

public:
	CircleAndSquare(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CircleAndSquare();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CircleAndSquare };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	// 四边横筋点筋直径
	CComboBox m_HDiameter;
	// 四边横筋点筋钢筋等级
	CComboBox m_HGrade;
	// 圆形钢筋点筋直径
	CComboBox m_CDiameter;
	// 圆形钢筋点筋等级
	CComboBox m_CGrade;
	// 侧面保护层
	CEdit m_Edit_Side;
	// 正反面保护层
	CEdit m_Edit_Positive;
	// 四边横筋间距	// 四边横筋间距
	CEdit m_Edit_HSpacing;
	// 四边点筋间距
	CEdit m_Edit_PSpacing;
	// 圆形筋间距
	CEdit m_Edit_CSpacing;
	// 圆形点筋角度间距
	CEdit m_Edit_CAngle;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
