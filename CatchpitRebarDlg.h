#pragma once
#include "CatchpitMainRebarDlg.h"
#include "GalleryIntelligentRebar.h"

// CatchpitRebarDlg 对话框

class CatchpitRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CatchpitRebarDlg)

public:
	CatchpitRebarDlg(ElementHandleCR eh, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CatchpitRebarDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CatchpitRebar };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	PIT::Concrete							m_Concrete;
	std::vector<PIT::ConcreteRebar>			m_vecRebarData;
	std::vector<PIT::EndType>				m_vecEndTypeData;
	WallSetInfo m_WallSetInfo;

	int m_CurSelTab;
	CatchpitMainRebarDlg					m_PageMainRebar;	//主要配筋界面
	CDialog*								pDialog[1];			//用来保存对话框对象指针
public:
	virtual BOOL OnInitDialog();

	// 分析集水坑的面与面之间的关系，将配筋面与其所有的锚固面存储到map中一一对应
	// @Add by tanjie, 2024.1.9
	void AnalyseEehFaces();

	void MakeFacesRebar(ElementId& contid, EditElementHandleR eeh, DgnModelRefP modelRef);

	/// @brief 扫描range内的所有元素
		/// @param range 
		/// @param filter 过滤函数
		/// @return 
	std::vector<ElementHandle> scan_elements_in_range(const DRange3d &range, std::function<bool(const ElementHandle &)> filter);

	//判断扫描到的板
	bool is_Floor(const ElementHandle &element);

private:
	ElementHandle _ehOld;//选中的集水坑
	ElementId _ehNew;
	bvector<ISubEntityPtr> m_selectfaces;
	//vector<ElementHandle> _ehOlds;
	string wall_name;
	ElementId m_ConcreteId;

	int m_CatchpitType = -1;// 0是标准集水坑，1是特殊集水坑，2是双集水坑

	map< EditElementHandle*, vector<EditElementHandle*>> m_map_PlaneWith_VerticalPlanes;//配筋面与其对应的锚固面

	vector<ElementHandle> m_AllFloors;//保留集水坑上的板

	vector <EditElementHandle*> m_ScanedFloors;//板的实体

	double m_UpFloor_Height = 0.0;//集水坑上方板的高度

	map<ElementHandle*, double> m_mapFloorAndHeight;//集水坑上面的板对应的高度

public:
	CTabCtrl m_tab;
	CStatic m_static_wallname;
	// 尺寸
	CComboBox m_ComboSize;
	// 型号
	CComboBox m_ComboType;
	// 间距
	CEdit m_EditSpace;
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
};
