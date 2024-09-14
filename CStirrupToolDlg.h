#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include <RebarDetailElement.h>
#include "StirrupRebar.h"

struct StirrupRebarDlgData
{
	int							interval{ 0 };		//布置多根箍筋时布置间隔
	bool						bUp{ false };		//箍筋在上方还是下方
	PIT::StirrupRebarData		rebarData;			//箍筋数据
};

// CStirrupToolDlg 对话框

class CStirrupToolDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CStirrupToolDlg)

public:
	CStirrupToolDlg(const vector<ElementId> &vecElm, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CStirrupToolDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_StittupRebarTool };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	void DrawRefLine();
	bool UpdateRebarData();
	vector<ElementRefP>	m_selectrebars;
	std:: string mSelectedRebarType;
private:
	StirrupRebarDlgData		m_dlgData;
	std::vector<ElementId>	m_vecElm_H;			//箍筋将放置在该数组元素的上方或下方
	std::vector<ElementId>	m_refLineIds;		//箍筋参考线
	std::vector<ElementId>	m_vecElm_V;			//纵向钢筋
	bool					m_bMonitor;			//
public:
	void	SetElementId_Vs(const std::vector<ElementId>& elm_Vs) { m_vecElm_V = elm_Vs; }
	void	SetDefaultRebarSize(BrStringCR strSize) { m_dlgData.rebarData.rebarSize = strSize; }
	void	SetDefaultRebarType(BrStringCR strType) { m_dlgData.rebarData.rebarType = strType; }

	void	InitRebarEndPointData();
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnEnChangeEdit13();
	afx_msg void OnBnClickedCheck5();
	CComboBox m_CombRebarSize;
	CComboBox m_CombRebarType;
	CButton m_CheckUp;
	CComboBox m_CombBegType;
	CComboBox m_CombEndType;
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton9();
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedButton7();
	afx_msg void OnBnClickedButton4();

private:
	/*
	* @desc:	更新点筋，移动点筋到箍筋弯曲中心位置
	* @param[in]	rebarAcrDatas 箍筋弯曲数据
	* @param[in]	modelRef 模型
	* @author	hzh
	* @Date:	2022/09/08
	*/
	void UpdateVRebars(const vector<RebarArcData>& rebarAcrDatas, DgnModelRefP modelRef);
};
