#pragma once
//#include "CSlabRebarDlg.h"
#include "XmlHelper.h"
#include <cstdio>
//#include "resource.h"
//#include "CWallMainRebarDlg.h"


// C_RebarTemplate 对话框

class C_RebarTemplate : public CDialogEx
{
	DECLARE_DYNAMIC(C_RebarTemplate)

public:
	C_RebarTemplate(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~C_RebarTemplate();

	//初始化界面
	virtual BOOL OnInitDialog();
	//设置表格里面的内容，将主界面的数据保存到本类
	void Set_m_vecRebarData(vector<PIT::ConcreteRebar> vecRebarData);
	//设置保护层等数据，将主界面的数据保存到本类
	void Set_m_dlgData(PIT::DlgData dlgData);

	//将配筋参数生成一个XML文件
	void Write_xmlData(PIT::DlgData dlgData, std::vector<PIT::ConcreteRebar> vecRebarData);

	//解析xml数据
	void displayNodeAttributes(XmlNodeRef nodeRef);
	//解析xml数据
	void ReadNode(XmlNodeRef nodeRef);
	//解析xml数据
	void readXML();
	//搜索该路径下的xml文件，用于将其加入到combobox列表中
	void Search_XmlFile(string pathName, vector<string>& vecFiles, string format);
	//设置该界面是由墙配筋打开
	void Set_isWall() { isWall = true; }
	//设置该界面是由板配筋打开
	void Set_isFloor() { isFloor = true; }
	//设置该界面是由板配筋打开
	void Set_isFace() { isFace = true; }
	//CDialog	*m_mainDlg;
	////CWallMainRebarDlg * parent_dlg = (CWallMainRebarDlg *)GetParent();

	//void set_dlg(CDialog * parent_dlg);
	// 获取上一次设置的参数模板名称
	CString Get_templateName() { return m_LastTemplateName; }
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_RebarTemplate };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	//从配筋界面获取到的数据
	std::vector<PIT::ConcreteRebar>			m_vecRebarData;
	PIT::DlgData							m_dlgData;

	CString  m_Str_TemplateName;//编辑框的值

	CString  m_LastTemplateName;//上一次选择的配筋模板名称

	CString  m_cmbStr_RebarTemplate;//combox的值

	vector<CString> m_vec_cmblist;//combobox的列表值

	vector<string> m_vec_xmlfiles;//获取保存过的模板文件名

	bool isWall;//是否是墙

	bool isFloor;//是否是板 默认都是false

	bool isFace;//是否是面

	int m_rebarNum;//记录读取到的钢筋层数

public:
	// 根据xml文件读取到的数据
	std::vector<PIT::ConcreteRebar>			m_Get_vecRebarData;
	PIT::DlgData							m_Get_dlgData;
	
	// 保存模板按钮
	afx_msg void OnBnClickedSavetemplate();
	//afx_msg void OnBnClickedButton3();1
	CListCtrl m_listTemplateName;
	CEdit m_edit_TemplateName;
	CComboBox m_cmb_RebarTemplate;
	// 删除模板按钮
	afx_msg void OnBnClickedDeletetemplate();
	// 确认按钮
	afx_msg void OnBnClickedOk();
	// combobox 切换数据
	afx_msg void OnCbnSelchangetemplate();
	afx_msg void OnBnClickedCancel();
};
