#pragma once
#include "CommonFile.h"

// CStarisRebarDlog 对话框
class CStairsRebarAssembly;
class CStarisRebarDlog : public CDialogEx
{
	DECLARE_DYNAMIC(CStarisRebarDlog)

public:
	CStarisRebarDlog(ElementHandleCR eh, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CStarisRebarDlog();

	virtual BOOL OnInitDialog();

	std::vector<PIT::ConcreteRebar> m_vecRebarData;

	void SetRebarAssembly(CStairsRebarAssembly* pStairsRebar)
	{
		m_pStairsRebar = pStairsRebar;
	}
protected:
	virtual void PostNcDestroy();
private:
	StairRebarInfo m_StairsRebarInfo;
	ElementHandle ehSel;

	CStairsRebarAssembly*			m_pStairsRebar;
	//	CStairsRebarAssembly*			m_pStairsRebarAssembly;
	//	void InitUIData();


	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_Stairs };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:

	CEdit		m_EditCover;
	CComboBox m_ComboStyle;
	CComboBox m_ComboSize;
	CComboBox m_ComboType;//型号


	afx_msg void OnEnChangeEdit15();//保护层
	afx_msg void OnCbnSelchangeCombo12();//楼梯样式
	afx_msg void OnCbnSelchangeCombo2();//钢筋尺寸
	afx_msg void OnCbnSelchangeCombo13();//钢筋型号
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
