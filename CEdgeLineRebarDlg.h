#pragma once
#include "PITRebarCurve.h"

struct ERebarDlgData
{
	BrString	sizeKey;
	double spacing;
};
// CEdgeLineRebarDlg 对话框
class CEdgeLineRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CEdgeLineRebarDlg)

public:
	CEdgeLineRebarDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CEdgeLineRebarDlg();
	void SetSelectElement(ElementHandleCR eh) { m_ehFal = eh; }//设置板实体
	void SetSelElement(ElementHandleCR eh) { m_ehSel = eh; }//设置墙
	DPoint3d ptLine[2];//选中线条2点
	DPoint3d  StrPtr;
	CVector3D Linevec;
	CVector3D Wallvec;//墙内侧方向
	//void SetLineSub(DPoint3d& eh) { ptLine = &eh; }//设置线
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_EdgeRebar };
#endif
private:
	ElementHandle m_ehSel;//选中的墙
	ElementHandle m_ehFal;//选中的板
	ERebarDlgData m_ErebarData;//延边加强筋数据
	//EditElementHandle* m_ehLine;//选中的线
	vector<ElementId>	m_vecLineId;//示意画线
	vector<RebarVertices> rebarVerticesNum;//钢筋生成数据
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	int rebarsize[15] = { 6 ,8,10,12,14,16,20,25,32,40,55 };
	CComboBox m_CombRebarSize;
	CComboBox m_CombRebarType;
	CComboBox m_CombRebarStyle;
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton1();
	void CalaRebarPoint();
	void ClearLine();
	bool makeRebarCurve(vector<RebarVertices>& m_rebarPts, DPoint3dCP points, size_t numVerts);
	bool GetFallThickness(double & slabHeight, ElementHandleCR eh);
	void GetWallThickness(double & thickness, ElementHandle ehSel);
	void GetWallThickness(double & thickness);
	void movePoint(DPoint3d vec, DPoint3d & movePt, double disLen, bool bFlag=true);
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeEdit13();
	afx_msg void OnBnClickedButton2();
};
