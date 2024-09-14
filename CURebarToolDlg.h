#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include <RebarDetailElement.h>
#include "URebar.h"

#define		UREBAR_MINLEN		10
#define		UREBAR_MAXLEN		10000

struct URebarDlgData
{
	PIT::URebarData		rebarData;			//U形钢筋数据
	int					interval{0};		//布置多根U形钢筋时布置间隔
	bool				bUp	{false};		//U形钢筋在上方还是下方
	bool				bNegtive{false};	//U形钢筋是否反向
	bool				bLegLenNeg{false};	//U形钢筋腿长是否互换
	bool				bInner = false;	//是否往内箍
};

// CURebarToolDlg 对话框

class CURebarToolDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CURebarToolDlg)

public:
	CURebarToolDlg(const vector<ElementId> &vecElm, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CURebarToolDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_URebarTool };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	void DrawRefLine();
	void UpdateRebarData();
	std::vector<ElementRefP> mSelsectedRebar;
	string mSelectedRebarType;
private:
	URebarDlgData			m_dlgData;
	std::vector<ElementId>	m_vecElm;			//U形筋将放置在该数组元素的上方或下方
	std::vector<ElementId>	m_refLineIds;		//U形筋参考线
	ElementId				m_elm_V1;			//纵向钢筋1
	ElementId				m_elm_V2;			//纵向钢筋2
	bool					m_bMonitor;			//
	std::vector<EditElementHandle*> m_Holeehs;	//孔洞
	bool					m_isEnd = false;	//L型钢筋依靠位置是否在钢筋尾端
	bool					m_isLRebar = false;	//是否生成L型钢筋
public:
	void	SetElementId_V1(ElementId elm_V1) { m_elm_V1 = elm_V1; }
	void	SetElementId_V2(ElementId elm_V2) { m_elm_V2 = elm_V2; }
	void	SetDefaultRebarSize(BrStringCR strSize) { m_dlgData.rebarData.rebarSize = strSize; }
	void	SetDefaultRebarType(BrStringCR strType) { m_dlgData.rebarData.rebarType = strType; }
	void	CalcWallHoles();
	void	SetIsEnd(bool isEnd) { m_isEnd = isEnd; }
	void	SetIsLRebar(bool isLRebar) { m_isLRebar = isLRebar; }
public:

	int rebarsize[15] = {6 ,8,10,12,14,16,20,25,32,40,55}; 
	CComboBox m_CombRebarSize;
	CComboBox m_CombRebarType;
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton3();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedCheck5();
	afx_msg void OnEnChangeEdit13();
	afx_msg void OnBnClickedButton2();
	CButton m_CheckNeg;
	CButton m_CheckUp;
	CButton m_CheckLength;
	CButton m_CheckInner;
	afx_msg void OnEnChangeEdit15();
	afx_msg void OnEnChangeEdit16();
	afx_msg void OnBnClickedCheck6();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnCbnKillfocusCombo1();
	afx_msg void OnBnClickedCheckInner();
};
