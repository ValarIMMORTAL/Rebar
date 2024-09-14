#include "GalleryIntelligentRebarids.h"
#include "CWallRebarListCtrl.h"

// RainshedRebarDlg 对话框

class RainshedRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(RainshedRebarDlg)

public:
	RainshedRebarDlg(ElementHandleCR eh, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~RainshedRebarDlg();
protected:
	virtual void PostNcDestroy();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_RainshedRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	void SetSelectElement(ElementHandleCR eh) { ehSel = eh; }
	ElementHandle GetSelectElement() { return ehSel; }
	void SetConcreteId(ElementId id) { m_ConcreteId = id; }

private:
	ElementHandle ehSel;
	ElementHandle m_ehSel;
	ElementId m_ConcreteId;
private:
	void InitUIData();
	std::vector<PIT::ConcreteRebar> m_vecRebarData;//m_vecRebarData为接收全局的g_vecRebarData来进行运算

	PIT::Concrete m_Concrete;

	int			  m_RainshedType;

public:
	void SetListDefaultData();
	void UpdateRebarList();
public:
	virtual BOOL OnInitDialog();
	CMainRebarListCtrl m_listMainRebar;
	CEdit		m_EditPositive;
	CEdit		m_EditSide;
	CEdit		m_EditReverse;
	CComboBox	m_CombRainshedType;

	void SetListRowData(const std::vector<PIT::ConcreteRebar>& vecListData) {
		m_vecRebarData = vecListData;
	}
	afx_msg void OnEnChangeREdit1();
	afx_msg void OnBnClickedOk();
	afx_msg void OnLvnItemchangedRList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeREdit2();
	afx_msg void OnEnChangeREdit3();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedCancel();
};
