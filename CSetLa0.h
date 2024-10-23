#pragma once
#include "ListCtrlEx.h"

struct StrLa0
{
	CString g_StrLa0_ConcreteGrade;		//混凝土等级
	CString g_StrLa0_OverlapArea;		//搭接面积
	CString g_StrLa0_RebarGrade;		//钢筋等级
	CString g_StrLa0_RebarDiameter;		//钢筋直径
	CString g_StrLa0_SeismicGrade;		//抗震等级
	double  g_db_La0Value;				//搭接长度
};
//StrLa0 g_StrLa0;

class CLa0DataListCtrl :public ListCtrlEx::CListCtrlEx
{
public:

};

// CSetLa0 对话框

class CSetLa0 : public CDialogEx
{
	DECLARE_DYNAMIC(CSetLa0)

public:
	CSetLa0(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSetLa0();

	//初始化界面
	virtual BOOL OnInitDialog();
	//刷新表格数据
	virtual BOOL UpdateLaeListALL();
	//初始化ComboBox的值
	void InitCComboBox();
	//更新ComboBox的值
	void UpdateComboBox();
	//根据ComboBox的值设置Condition的值
	void SetConditionData();
	//根据全局变量的值设置Condition的值
	void Set_gConditionData();
	void SetConditionData_2();
	//填充表格数据
	void SetListData();
	void SetListData_2();
	//刷新表格数据
	void UpdateLa0List();
	void UpdateLa0List_2();
	//保存表格里面的值
	void SaveConcreteAndRebar_Grade();
	//根据全局变量里面的钢筋等级保存表格里面的值
	void Save_gConcreteAndRebar_Grade();

	//将参数值写入到XML文件中
	void WriteXml();
	//解析xml数据
	void displayNodeAttributes(XmlNodeRef nodeRef);
	//解析xml数据
	void ReadNode(XmlNodeRef nodeRef);
	//解析xml数据
	void readXML();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SetLa0 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();

public:
	//s 抗震等级 a 搭接面积 d 钢筋直径
	//Condition分为八种情况，其取值分别为 0~7
	// || s=1/2 ,a<25,d<25 || s=1/2 ,a<25,d>25 || s=1/2 ,a=50,d<25 || s=1/2 ,a=50,d>25 ||
	// || s=3 ,a<25,d<25   || s=3 ,a<25,d>25   || s=3 ,a=50,d<25   || s=3 ,a=50,d>25   ||
	int m_Condition;
	int m_Sel_ConcreteGrade;//选择的混凝土等级的下标
	bool m_bInitialized = false; // 标志位，用于检查是否已初始化
	CString m_data;//表格中确定的数据
	
	map<int, vector<vector<CString>>> m_TableData;

	CString m_Str_ConcreteGrade;	//混凝土等级
	CString m_Str_OverlapArea;		//搭接面积
	CString m_Str_RebarGrade;		//钢筋等级
	CString m_Str_RebarDiameter;	//钢筋直径
	CString m_Str_SeismicGrade;		//抗震等级

	CComboBox m_ConcreteGrade;	 //混凝土等级
	CComboBox m_OverlapArea;	 //搭接面积
	CComboBox m_RebarGrade;		 //钢筋等级
	CComboBox m_RebarDiameter;	 //钢筋直径
	CComboBox m_SeismicGrade;	 //抗震等级

	CLa0DataListCtrl m_La0_ListCtrl;
	afx_msg void OnCbnSelchangeConcretegrade();
	afx_msg void OnCbnSelchangeOverlaparea();
	afx_msg void OnCbnSelchangeRebargrade();
	afx_msg void OnCbnSelchangeRebardiameter();
	afx_msg void OnCbnSelchangeSeismicgrade();
};

