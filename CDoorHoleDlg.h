#pragma once
#include "CHoleRebarListCtrl.h"

// CDoorHoleDlg 对话框
class DoorHoleRebarAssembly;
class CDoorHoleDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDoorHoleDlg)

public:
	CDoorHoleDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDoorHoleDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_DOORHOLE };
#endif
	void InitUIData();

protected:
	virtual void PostNcDestroy();
public:

	std::vector<HoleRebarInfo::ReinForcingInfo> m_vecReinF;
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	std::map<std::string, IDandModelref> m_NEGholeidAndmodel;
	std::vector<RebarPoint> m_RebarPts;
	std::vector<DPoint3d> m_FrontPts;
	ElementHandle m_ehSel;
	Transform m_trans;
	ACCConcrete m_acconcrete;
	DoorHoleRebarAssembly*  m_HoleRebar;
	//HoleArcRFRebarAssembly*  m_ArcHoleRebar;
	static void RefreshDoorRebars(ElementId conid, EditElementHandleR eeh);
	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }
	ElementHandle GetSelectElement() { return m_ehSel; }
	void GetNowHoleNum();
	static bool GetArcCenterPoint(std::vector<RebarPoint>& RebarPts, DPoint3d& ptCenter, EditElementHandle* holedescr);
	void SetListDefaultData();
	void UpdateACList();
	virtual BOOL OnInitDialog();
	void ExTractHoleDatas();
	void InitReinForcingInfoData(HoleRebarInfo::ReinForcingInfo& refdata);
	void UpdateHoleDataView(string holename);
	void SetConcreteId(ElementId id) { m_ConcreteId = id; }
	bool isArcwall;
	ElementId m_ConcreteId;
public:
	void SetListRowData(const std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData) {
		m_vecReinF.clear();
		m_vecReinF.resize(vecListData.size());
		m_vecReinF = vecListData;
	}
	void GetListRowData(std::vector<HoleRebarInfo::ReinForcingInfo> &vecListData) {
		vecListData = m_vecReinF;
	};
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	string m_nowHolename;
	int m_nowHoleNum;
	CDoorHoleRebarCtrl m_list_doorhole;
	afx_msg void OnBnClickedOk();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnHdnItemclickListDoorhole(NMHDR *pNMHDR, LRESULT *pResult);
};
