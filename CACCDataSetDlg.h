#pragma once
#include "GalleryIntelligentRebarids.h"
// CACCDataSetDlg 对话框

class CACCDataSetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CACCDataSetDlg)

public:
//	CACCDataSetDlg(CWnd* pParent = nullptr);   // 标准构造函数
	CACCDataSetDlg(ElementHandleCR eeh, const ACCConcrete &concrete,double offset,CWnd* pParent = nullptr);
	virtual ~CACCDataSetDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ACC_INFO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	ElementHandle m_eh;
	ACCConcrete m_concrete;
	double	m_offset;
	int		m_anchoringMethod;

	double	m_L0;
	double	m_La;
	int		m_IsReverse;
	int		m_IsCut;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();

public:
	void SetAnchoringMethod(int method) { m_anchoringMethod = method; }
	ACCConcrete GetACConcreteData() { return m_concrete; }
	double GetOffset() { return m_offset; }
	void SetL0(double L0) { m_L0 = L0; }
	double GetL0() { return m_L0; }
	void SetLa(double La) { m_La = La; }
	double GetLa() { return m_La; }
	void SetIsReverse(int isReverse) { m_IsReverse = isReverse; }
	int GetIsReverse() { return m_IsReverse; }
	void SetIsCut(int isCut) { m_IsCut = isCut; }
	int GetIsCut() { return m_IsCut; }
	CButton m_ctrlCheck1;
	CButton m_ctrlCheck2;
};
