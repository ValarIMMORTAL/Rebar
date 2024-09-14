#pragma once
#include "CommonFile.h"


// ShowHideRebarByDiaDlg 对话框

class ShowHideRebarByDiaDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ShowHideRebarByDiaDlg)

public:
	ShowHideRebarByDiaDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~ShowHideRebarByDiaDlg();

	/*
	* @desc:	设置需要显示或隐藏的钢筋集合
	* @param[in]	selectRebars 需要显示或隐藏的钢筋集合	
	* @author	hzh
	* @Date:	2022/09/01
	*/
	void SetSelectedRebars(const std::vector<ElementRefP> selectRebars) { m_selectRebars = selectRebars; }

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ShowHideRebarByDia };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

private:
	std::vector<ElementRefP> m_selectRebars;	//选中的钢筋
	CComboBox m_rebarDiaCombo;					//钢筋直径
	CComboBox m_showHideCombo;					//显示隐藏
	bool m_isShow;								//是否显示
	double m_rebarDia;							//选择的钢筋直径
public:
	/*
	* @desc:	确定
	* @author	hzh
	* @Date:	2022/09/01
	*/
	afx_msg void OnBnClickedOk();

	/*
	* @desc:	选择直径
	* @author	hzh
	* @Date:	2022/09/01
	*/
	afx_msg void OnCbnSelchangeComboDia();

	/*
	* @desc:	选择显示还是隐藏
	* @author	hzh
	* @Date:	2022/09/01
	*/
	afx_msg void OnCbnSelchangeComboShowhide();
};
