#pragma once
#include "CHoleRebarListCtrl.h"
#include "CHoleRebar_AddUnionHoleDlg.h"
// HoleRebar_Reinforcing 对话框
class HoleRFRebarAssembly;
class HoleArcRFRebarAssembly;


class CHoleRebar_ReinforcingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CHoleRebar_ReinforcingDlg)

public:
	CHoleRebar_ReinforcingDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CHoleRebar_ReinforcingDlg();
protected:
	virtual void PostNcDestroy();
	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_HoleRebar_Reinforcing };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	map<std::string, ElementId > m_NewHoleElements;
	map<std::string, int > m_ListIndexAndName;//每一个孔洞名称及list中对应的index
	vector<ElementRefP> m_vctSelectHoles;//点击点选孔洞后：所有孔洞的实体
	void clearSelectHoles();
	void GetSeclectElement(EditElementHandleR HoleEeh);//从选择工具那里获取选中的孔洞
private:
	void InitUIData();


public:
	//孔洞点选数据集合
	std::vector<HoleRebarInfo::ReinForcingInfo> m_vecReinF;
	//板件上所有孔洞ID和Model指针
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	//所有主筋数据（孔洞截取前的数据）
	std::vector<RebarPoint> m_RebarPts;
	//板件分段的点数据，主要针对多段墙时，如果STWALL只有两个点数据
	std::vector<DPoint3d> m_FrontPts;
	//当前元素句柄
	ElementHandle m_ehSel;
	//当前段将元素转到和X轴平行的矩阵
	Transform m_trans;
	//当前元素的保护层相关信息
	ACCConcrete m_acconcrete;
	//创建直线构造筋指针
	HoleRFRebarAssembly*  m_HoleRebar;
	//创建弧形构造筋指针
	HoleArcRFRebarAssembly*  m_ArcHoleRebar;

	static void RefreshReinforcingRebars(ElementId conid, EditElementHandleR eeh);
	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }
	ElementHandle GetSelectElement() { return m_ehSel; }
	void GetNowHoleNum();
	static bool GetArcCenterPoint(std::vector<RebarPoint>& RebarPts, DPoint3d& ptCenter, EditElementHandle* holedescr);
	static void GetUseHoles(EditElementHandleR m_ehSel, vector<DPoint3d>& m_FrontPts, vector<EditElementHandle*>& Holeehs,
		vector<EditElementHandle*>& useHoleehs, DgnModelRefP modelRef, bool isfoor = false);
	static void GetARCUseHoles(EditElementHandleR m_ehSel, vector<EditElementHandle*>& Holeehs, vector<EditElementHandle*>& useHoleehs, DgnModelRefP modelRef);
	void SetListDefaultData();
	void UpdateACList();
	void ExTractHoleDatas();
	void InitReinForcingInfoData(HoleRebarInfo::ReinForcingInfo& refdata);
	void UpdateHoleDataView(string holename);
	void SetConcreteId(ElementId id) { m_ConcreteId = id; }
	//标识是否为弧形墙
	bool isArcwall;
	//标识是否为板件
	bool isFloor;
	//混凝土ID
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

public:
	BrString m_rebarDia;
	int Typenum;
	std::vector<PIT::ConcreteRebar>	m_vecRebarData;
	virtual BOOL OnInitDialog();
	CHoleRebarReinForcingCtrl m_list_holeReinforcing;
	afx_msg void OnHdnItemclickListAc(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnBnClickedOk();
	CButton m_Check_reinv1;
	CButton m_Check_revinv2;
	CButton m_Check_revinh3;
	CButton m_Check_revinh4;
	CEdit m_Edit_revinnumv1;
	CEdit m_Edit_revinnumv2;
	CEdit m_Edit_revinnumh3;
	CEdit m_Edit_revinnumh4;
	CEdit m_Edit_reinspacingv1;
	CEdit m_Edit_reinspacingv2;
	CEdit m_Edit_reinspacingh3;
	CEdit m_Edit_reinspacingh4;
	string m_nowHolename;
	int m_nowHoleNum;
	afx_msg void OnBnClickedCheckReinv1();
	afx_msg void OnBnClickedCheckReinv2();
	afx_msg void OnBnClickedCheckReinh3();
	afx_msg void OnBnClickedCheckReinh4();
	afx_msg void OnEnChangeEditReinnumv1();
	afx_msg void OnEnChangeEditReinnumv2();
	afx_msg void OnEnChangeEditReinnumh3();
	afx_msg void OnEnChangeEditReinnumh4();
	afx_msg void OnEnChangeEditReinspacingv1();
	afx_msg void OnEnChangeEditReinspacingv2();
	afx_msg void OnEnChangeEditReinspacingh3();
	afx_msg void OnEnChangeEditReinspacingh4();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedSetUnionholeButton();
	afx_msg void OnBnClickedButtonDissunion();
	CButton m_check_selectall;
	//	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedCheckReinseclectall();
	afx_msg void OnBnClickedButton1();
	CComboBox m_diameter;
	CComboBox m_type;
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnCbnSelchangeCombo2();
};
