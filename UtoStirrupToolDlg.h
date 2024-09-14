#include "CommonFile.h"
#include "StirrupRebar.h"

// UtoStirrupToolDlg 对话框

class UtoStirrupToolRebar;

class UtoStirrupToolDlg : public CDialogEx
{
	DECLARE_DYNAMIC(UtoStirrupToolDlg)

public:
	UtoStirrupToolDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~UtoStirrupToolDlg();
	std::vector<ElementRefP> m_selectrebars;
	void SaveRebarPts(vector< vector<CPoint3D>>& Vctpts);
	void SaveRebarData(BrString& RebarSize, vector <ElementId>&vecElm);
	void ProcessRebarData();
	void PreviewLines();
	void clearLines();
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_UTOSTIRRUP };
#endif

private:
	std::vector<vector<CPoint3D>>	m_vctpts;//所有箍筋的点
	string mSelectedRebarType;
	vector<shared_ptr<UtoStirrupToolRebar> >	m_pStirrupRebars;
	char    m_rebarSize[512];		//钢筋尺寸
	char    m_SizeKey[512];		
	int		m_rebarType;			//钢筋型号
	vector<ElementId> m_vecElm_H;
	vector<ElementRefP> m_allLines;//预览画线


protected:
	virtual void PostNcDestroy();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_ComboSize;//箍筋尺寸
	CComboBox m_ComboType;//箍筋型号
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnCbnSelchangeCombo12();
	afx_msg void OnBnClickedButton2();
};




class UtoStirrupToolRebar : public PIT::StirrupRebar
{

public:
	vector<CPoint3D> m_vecRebarPt;
	BE_DATA_VALUE(PIT::StirrupRebarData, rebarData)
//	vector<shared_ptr<UtoStirrupToolRebar> >	m_pStirrupRebars;
public:
	explicit UtoStirrupToolRebar(const vector<CPoint3D> &vecRebarPts, PIT::StirrupRebarDataCR rebarData);
private:

};



