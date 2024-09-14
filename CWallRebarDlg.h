#pragma once
#include "CWallMainRebarDlg.h"
//#include "CWallRebarLapOptionDlg.h"
#include "CWallRebarEndTypeDlg.h"
//#include "CWallRebarAssociatedComponentDlg.h"
#include "CTwinBarSetDlg.h"
#include "CTieRebarSetDlg.h"
#include "PITACCRebarAssembly.h"
//#include "Mstn\MdlApi\mfcDialog\CBModalDialog.h"
//#include "Mstn\MdlApi\mfcDialog\CBModelessDialog.h"
// CWallRebar 对话框

class CWallRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CWallRebarDlg)

public:
	CWallRebarDlg(ElementHandleCR eh,CWnd* pParent = nullptr);   // 标准构造函数
//	CWallRebarDlg(UINT Id, CWnd* pParent = nullptr);
	virtual ~CWallRebarDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_WallRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void PostNcDestroy();
	DECLARE_MESSAGE_MAP()
public:
	CTabCtrl m_tab;

//	virtual StatusInt   Create();

 public:
	 static void RefreshWallRebars(ElementId testid, EditElementHandleR eeh);
	 void SetSelectElement(ElementHandleCR eh) { ehSel = eh; }
	 ElementHandle GetSelectElement() { return ehSel; }
	 void PreviewRebarLines();
	 void SetIsCMD(bool isCmd) { m_isCmdWall = isCmd; }
// 	void SetRebarInfo(const vector<PIT::ConcreteRebar>& vecWallRebar);
// 	void SetLapOptionInfo(const vector<PIT::LapOptions>& vecLapOption);
// 	void SetEndTypeInfo(const vector<PIT::EndType>& vecEndType);
//	 void SetACInfo(const vector<PIT::AssociatedComponent>& vecACData);
// private:
// 	WallRebarInfo wallRebarInfo;
	 void SetConcreteId(ElementId id) { m_ConcreteId = id; }
	 void GetThickness(double &thickness);
private:
	WallRebarAssembly * m_WallRebarLinesPtr;//预览按钮使用
	PIT::ACCRebarAssembly*  m_WallACCRebarLinesPtr;//ACC预览按钮使用

	ElementId m_ConcreteId;
public:
	ElementHandle ehSel;
	int m_CurSelTab;
	bool m_isCombineWall = false;//是否是合并配筋
	CWallMainRebarDlg					m_PageMainRebar;			//主要配筋
	CDialog*							pDialog[1];					//用来保存对话框对象指针
	virtual BOOL OnInitDialog();
	afx_msg void OnTcnSelchangeTabWallrebar(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedOk();
	void on_Clicke_maker(); // 按确认按钮筛选出墙之后进行配筋操作
	
	string wall_name;

	CFont m_font;	//字体
	bool m_isCmdWall = false; //是否是CMD命令打开配筋
	int m_ClickedFlag;//点击了点选按钮并且选中墙后设置为1，使得墙配筋界面不会关闭
					  //没有点击点选按钮就选中墙，会设置为0，使得按下确定按钮界面就会关闭
public:
	CComboBox m_ComboSize;//尺寸
	CComboBox m_ComboType;//型号
	CEdit	  m_EditSpace;//间距

	WallSetInfo m_WallSetInfo;
// 	char    m_rebarSize[512];		//钢筋尺寸
// 	int		m_rebarType;			//钢筋型号
// 	double  m_spacing;			//钢筋间距

	std::vector<PIT::ConcreteRebar>					m_vecRebarData;
	std::vector<PIT::LapOptions>					m_vecLapOptionData;
	std::vector<PIT::EndType>						m_vecEndTypeData;
	std::vector<PIT::AssociatedComponent>			m_vecACData;
	std::vector<TwinBarSet::TwinBarLevelInfo>	m_vecTwinBarData;
	TwinBarSet::TwinBarInfo						m_twinBarInfo;
	TieReBarInfo								m_tieRebarInfo;
	afx_msg void OnBnClickedCancel();
	CStatic m_static_wallname;
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnStnClickedStaticWallname();
	afx_msg void OnBnClickedSelectModel();
	CStatic m_thickness;
	CStatic m_wallthickness;
};
