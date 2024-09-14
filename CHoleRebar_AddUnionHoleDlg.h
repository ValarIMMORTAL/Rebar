#pragma once
#include "CHoleRebarListCtrl.h"

// CHoleRebar_AddUnionHoleDlg 对话框
class CHoleRebar_ReinforcingDlg;
class CHoleRebar_StructualDlg;
class CHoleRebar_AddUnionHoleDlg : public CDialogEx
{
	
	DECLARE_DYNAMIC(CHoleRebar_AddUnionHoleDlg)

public:
	enum UNIONTYPE
	{
		ReinForcing,
		Structual
	};
	CHoleRebar_AddUnionHoleDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CHoleRebar_AddUnionHoleDlg();
	std::vector<HoleRebarInfo::ReinForcingInfo> m_vecReinF;
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	CHoleRebar_ReinforcingDlg* m_Reinforcingdlg;
	CHoleRebar_StructualDlg* m_Structualdlg;
	vector<DPoint3d> m_FrontPts;
	std::vector<RebarPoint> m_RebarPts;
	Transform m_trans;
	UNIONTYPE m_type;
public:
	virtual BOOL OnInitDialog();
	void SetListRowData(const std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData) {
		m_vecReinF = vecListData;
	}
	void GetListRowData(std::vector<HoleRebarInfo::ReinForcingInfo> &vecListData) {
		vecListData = m_vecReinF;
	};
	void SetHoleData(const std::map<std::string, IDandModelref>& vecListData) {
		m_holeidAndmodel = vecListData;
	}
	void GetHoleData(std::map<std::string, IDandModelref> &vecListData) {
		vecListData = m_holeidAndmodel;
	};
	void UpdateACList();
	void SetListDefaultData();
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum {
		IDD = IDD_DIALOG_ADDUNIONHOLE
	};
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CHoleRebarAddUnionCtrl m_list_addUnionHole;
	afx_msg void OnBnClickedOk();
	CEdit m_edit_unionname;
};
