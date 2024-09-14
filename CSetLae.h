#pragma once
#include "ListCtrlEx.h"

struct StrLae
{
	CString g_StrLae_SeismicGrade;		//抗震等级
	CString g_StrLae_ConcreteGrade;		//混凝土等级
	CString g_StrLae_RebarGrade;		//钢筋等级
	CString g_StrLae_RebarDiameter;		//钢筋直径
	double g_db_LaeValue;				//锚固长度
};
//StrLae g_StrLae;

class CLaeDataListCtrl :public ListCtrlEx::CListCtrlEx
{
public:
	
};
// CSetLae 对话框

class CSetLae : public CDialogEx
{
	DECLARE_DYNAMIC(CSetLae)

public:
	CSetLae(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CSetLae();

	//初始化界面
	virtual BOOL OnInitDialog();
	//初始化ComboBox的值
	void InitCComboBox();
	//更新ComboBox的值
	void UpdateComboBox();
	//根据ComboBox的值设置Condition的值
	void SetConditionData();
	//根据全局变量的值设置Condition的值
	void Set_gConditionData();
	//填充表格数据
	void SetListData();
	//刷新表格数据
	void UpdateLaeList();
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
	enum { IDD = IDD_SetLae };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();

public:
	//s 抗震等级  d 钢筋直径
	//Condition分为4种情况，其取值分别为 0~3
	// || s=1/2 ,d<25 || s=1/2 ,d>25 ||
	// || s=3 ,  d<25 || s=3 ,  d>25 || 
	int m_Condition;
	int m_Sel_ConcreteGrade;//选择的混凝土等级的下标
	CString m_data;//表格中确定的数据

	map<int, vector<vector<CString>>> m_TableData;//存储每一行每一列各种情况的表格的数据

	CString m_Str_SeismicGrade;		//抗震等级
	CString m_Str_ConcreteGrade;	//混凝土等级
	CString m_Str_RebarGrade;		//钢筋等级
	CString m_Str_RebarDiameter;	//钢筋直径

	CComboBox m_SeismicGrade;		//抗震等级
	CComboBox m_ConcreteGrade;		//混凝土等级
	CComboBox m_RebarGrade;			//钢筋等级
	CComboBox m_RebarDiameter;		//钢筋直径

	CLaeDataListCtrl m_Lae_ListCtrl;//表格

	afx_msg void OnCbnSelchangeSeismicgrade();
	afx_msg void OnCbnSelchangeConcretegrade();
	afx_msg void OnCbnSelchangeRebargrade();
	afx_msg void OnCbnSelchangeRebardiameter();

};
