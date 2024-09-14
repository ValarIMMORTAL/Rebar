#pragma once
//#include "ListCtrlEx.h"
#include "CWallRebarListCtrl.h"
#include "C_RebarTemplate.h"
#include "GalleryIntelligentRebar.h"
// CatchpitMainRebarDlg 对话框

class CatchpitMainRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CatchpitMainRebarDlg)

public:
	CatchpitMainRebarDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CatchpitMainRebarDlg();

	virtual BOOL OnInitDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_Catchpit_MainRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	// 正面保护层
	CEdit m_EditPositive;
	// 侧面保护层
	CEdit m_EditSide;
	// 规避孔洞
	CButton m_hole_check;
	// 忽略尺寸
	CEdit m_mholesize_edit;
	// 钢筋层数
	CEdit m_EditLevel;
	// 配筋参数列表
	CMainRebarListCtrl m_listMainRebar;

	bvector<ISubEntityPtr> m_selectfaces;

	PIT::DlgData  m_dlgData;			//界面上的保护层等数据
	C_RebarTemplate	 m_RebarTemplate;	//钢筋模板界面
	CString m_templateName;				//上一次使用的模板名称
	void OnBnClickedUpdatadlg();		//刷新界面，参数模板按确认按钮之后调用
	void Save_templateName();			//保存上一次的模板名称

	void SetListDefaultData();

	void UpdateRebarList();

	PIT::Concrete GetConcreteData();

	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }

	void SetConcreteData(PIT::Concrete concrete) { m_Concrete = concrete; }

	void SetListRowData(const std::vector<PIT::ConcreteRebar>& vecListData) {
		m_vecRebarData = vecListData;
	}

	void GetListRowData(std::vector<PIT::ConcreteRebar> &vecListData) {
		vecListData = m_vecRebarData;
	};

	void SetSelectfaces(bvector<ISubEntityPtr>& selectfaces);

	void ChangeRebarSizedata(char * rebarSize)
	{
		auto it = m_vecRebarData.begin();
		for (it; it != m_vecRebarData.end(); it++)
			strcpy(it->rebarSize, rebarSize);
	}
	void ChangeRebarTypedata(int rebarType)
	{
		auto it = m_vecRebarData.begin();
		for (it; it != m_vecRebarData.end(); it++)
			it->rebarType = rebarType;
	}
	void ChangeRebarSpacedata(double rebarSpace)
	{
		auto it = m_vecRebarData.begin();
		for (it; it != m_vecRebarData.end(); it++)
			it->spacing = rebarSpace;
	}

private:
	void InitUIData();
	PIT::Concrete	m_Concrete;
	std::vector<PIT::ConcreteRebar> m_vecRebarData;

	ElementHandle m_ehSel;
public:
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnBnClickedHoleCheck();
	afx_msg void OnEnChangeMholesizeEdit();
	afx_msg void OnEnChangeEdit4();
	afx_msg void OnBnClickedRebartemplate();
};
