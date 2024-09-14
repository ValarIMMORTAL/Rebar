#include "_USTATION.h"
#include "CWallRebarListCtrl.h"

class BaseRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(BaseRebarDlg)

public:
	BaseRebarDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~BaseRebarDlg();

	CEdit m_EditPositive;
	CEdit m_EditSide;
	CEdit m_EditReverse;
	CEdit m_StirrupNum_edit;//箍筋数量
	CEdit m_StirrupStart_edit;//箍筋
	CEdit m_StirrupEnd_edit;//箍筋
	CComboBox m_ComboSize;//箍筋尺寸
	CComboBox m_ComboType;//箍筋型号
//	CButton m_hole_check;
// 是否规避孔洞设置
//	CEdit m_mholesize_edit;
// 忽略孔洞大小设置
//	CEdit m_Len_edit;
//延申长度

	ElementHandle m_ehSel;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_BaseRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();
	void UpdateRebarList();
	void SetListDefaultData();
	void GetConcreteData(PIT::Concrete& concreteData);
	DECLARE_MESSAGE_MAP()
protected:
	virtual void PostNcDestroy();
private:
	void InitUIData();
	std::vector<PIT::ConcreteRebar> m_vecRebarData;//m_vecRebarData为接收全局的g_vecRebarData来进行运算
	std::vector<PIT::LapOptions>					m_vecLapOptionData;
	std::vector<PIT::EndType>						m_vecEndTypeData;
	std::vector<PIT::AssociatedComponent>			m_vecACData;
	std::vector<TwinBarSet::TwinBarLevelInfo>	m_vecTwinBarData;
	TwinBarSet::TwinBarInfo						m_twinBarInfo;
	TieReBarInfo								m_tieRebarInfo;
//	CBaseRebarListCtrl m_listBaseRebar;
	StirrupData m_StirrupData;//箍筋与拉筋数据
	Abanurus_PTRebarData m_PTRebarData;//点筋数据
public:
	afx_msg void OnEnChangeSEdit1();
	afx_msg void OnEnChangeSEdit2();
	afx_msg void OnEnChangeSEdit3();
//	afx_msg void OnBnClickedSHoleCheck();
//	afx_msg void OnEnChangeSMholesizeEdit();
	afx_msg void OnBnClickedOk();
	afx_msg void OnCbnSelchangeCombo12();
	afx_msg void OnEnChangeSEdit4();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnEnChangeSEdit5();
	afx_msg void OnEnChangeSEdit6();
//	afx_msg void OnEnChangeSEdit7();
	afx_msg void OnBnClickedCancel();
	// 横向点筋数量
	CEdit m_edit_HPtNum;
	// 纵向点筋数量
	CEdit m_edit_VPtNum;
	// 点筋直径
	CComboBox m_Cob_PtDiameter;
	// 点筋等级
	CComboBox m_Cob_PtGrade;
	// 拉筋直径
//	CComboBox m_Cob_TieDiameter;
	// 拉筋等级
//	CComboBox m_Cob_TieGrade;
	// 拉筋数量
//	CEdit m_edit_TieNum;
	afx_msg void OnEnChangeEditHnum();
	afx_msg void OnEnChangeEditVnum();
//	afx_msg void OnEnChangeEditTienum();
	afx_msg void OnCbnSelchangeComboDiameter();
	afx_msg void OnCbnSelchangeComboGrade();
//	afx_msg void OnCbnSelchangeComboTiediameter();
//	afx_msg void OnCbnSelchangeComboTiegrade();
};
