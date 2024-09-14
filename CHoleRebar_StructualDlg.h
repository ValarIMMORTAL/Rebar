#pragma once
#include "CHoleRebarListCtrl.h"

	// HoleRebar_Reinforcing 对话框
class  HoleSTRebarAssembly;
class  HoleArcSTRebarAssembly;;
class CHoleRebar_StructualDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CHoleRebar_StructualDlg)

public:
	CHoleRebar_StructualDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CHoleRebar_StructualDlg();
protected:
	virtual void PostNcDestroy();
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_HoleRebar_Structural };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持


public:

	map<std::string, ElementId > m_NewHoleElements;
	map<std::string, int > m_ListIndexAndName;//每一个孔洞名称及list中对应的index
	vector<ElementRefP> m_vctSelectHoles;//点击点选孔洞后：所有孔洞的实体
	void clearSelectHoles();
	void GetSeclectElement(EditElementHandleR HoleEeh);//从选择工具那里获取选中的孔洞
private:
	void InitUIData();


public:
	//当前选中的孔洞名称
	string m_nowHolename;
	//当前孔洞的序号
	int m_nowHoleNum;
	//是否是弧形墙上的孔洞
	bool isArcwall;
	//孔洞点选数据集合
	std::vector<HoleRebarInfo::ReinForcingInfo> m_vecReinF;
	//板件上所有孔洞ID和Model指针
	std::map<std::string, IDandModelref> m_holeidAndmodel;
	//所有主筋数据（孔洞截取前的数据）
	std::vector<RebarPoint> m_RebarPts;
	//所有并筋数据（孔洞截取前的数据）
	std::vector<RebarPoint> m_TwinRebarPts;
	//板件分段的点数据，主要针对多段墙时，如果STWALL只有两个点数据
	std::vector<DPoint3d>   m_FrontPts;
	//并筋参数
	std::vector<TwinBarSet::TwinBarLevelInfo>	m_vecTwinBarData;
	//当前元素句柄
	ElementHandle m_ehSel;
	//当前段将元素转到和X轴平行的矩阵
	Transform m_trans;
	//当前元素的保护层相关信息
	ACCConcrete m_acconcrete;
	//创建直线加强筋指针
	HoleSTRebarAssembly*  m_HoleRebar;
	//创建弧形加强筋指针
	HoleArcSTRebarAssembly*  m_ArcHoleRebar;
	//是否为板件
	bool isFloor;

	static void RefreshStructualRebars(ElementId conid, EditElementHandleR eeh);
	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }
	ElementHandle GetSelectElement() { return m_ehSel; }
	void GetNowHoleNum();
	void SetListDefaultData();
	void UpdateACList();
	void ExTractHoleDatas();
	void InitStructualInfoData(HoleRebarInfo::ReinForcingInfo& refdata);
	void UpdateHoleDataView(string holename);
	void SetConcreteId(ElementId id) { m_ConcreteId = id; }

	ElementId m_ConcreteId;
public:
	void SetListRowData(const std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData) {
		m_vecReinF = vecListData;
	}
	void GetListRowData(std::vector<HoleRebarInfo::ReinForcingInfo> &vecListData) {
		vecListData = m_vecReinF;
	};

public:
	virtual BOOL OnInitDialog();
	BrString m_rebarDia;
	int Typenum;
	std::vector<PIT::ConcreteRebar>	m_vecRebarData;
	CHoleRebarStructualCtrl m_list_holeStructual;
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	/*
	* @desc:	选择直径
	* @author	hzh
	* @Date:	2022/09/01
	*/
	int crosswise;
	int urebar;
	CButton m_check_Structualh3;
	CButton m_check_structualh4;
	CButton m_check_structualv1;
	CButton m_check_structualv2;
	CButton m_check_structualtwinv1;
	CButton m_check_structualtwinv2;
	CButton m_check_structualtwinh3;
	CButton m_check_structualtwinh4;
	afx_msg void OnBnClickedCheckStructualv1();
	afx_msg void OnBnClickedCheckStructualv2();
	afx_msg void OnBnClickedCheckStructualh3();
	afx_msg void OnBnClickedCheckStructualh4();
	afx_msg void OnBnClickedCheckStructualtwinv1();
	afx_msg void OnBnClickedCheckStructualtwinv2();
	afx_msg void OnBnClickedCheckStructualtwinh3();
	afx_msg void OnBnClickedCheckStructualtwinh4();
	afx_msg void OnBnClickedSetUnionholestButton();
	afx_msg void OnBnClickedButtonDissunionst();
	CButton m_check_seclectall;
	afx_msg void OnBnClickedCheckStrselectall();
	afx_msg void OnBnClickedButton1();
	CComboBox m_diameter;
	CComboBox m_type;
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnCbnSelchangeCombo2();
	CButton m_crosswise;
	CButton m_urebar;
	afx_msg void OnBnClickedCrosswiseRebar();
	afx_msg void OnBnClickedURebar();
};


