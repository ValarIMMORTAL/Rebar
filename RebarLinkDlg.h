#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"

// RebarLinkDlg 对话框

class RebarLinkDlg : public CDialogEx
{
	DECLARE_DYNAMIC(RebarLinkDlg)

public:
	RebarLinkDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~RebarLinkDlg();
	virtual BOOL OnInitDialog();
	struct RebarLinkInfo
	{
		int RebarLinkMethod;//连接方式
		bool missflg;//为true则错开
		double MissLen;//错开长度
		bool ChangeFlg;//为True则切换
		bool tranLenthFlg;//选择延长的钢筋组

	}m_RebarLinkInfo;
	map<int, vector<ElementRefP>> m_mapselectrebars;
	std::vector<ElementRefP> m_selectrebars;


	map<ElementId, vector<RebarVertices>> m_AllPts;
	vector<vector<BrString>> m_AllvecDir;
	int  m_sizekey;
	int  m_sizekey1;
	BrString m_SSizeKey;
	vector<ElementRefP> m_Editrebars;//需要修改的钢筋
	vector<ElementRefP> m_allLines;


private:
	CComboBox m_ComboLinkMethod;
	CEdit  m_EidtMissLen;
	CButton m_Checkmiss;
	CButton m_ChangeButton;
	void InitUIData();
	DECLARE_MESSAGE_MAP()

public:
	void SorSelcetRebar();
	void CalculateVertexAndDrawLines();
	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);
	CVector3D LinkDirAndLen(DPoint3d& pt1, DPoint3d& pt2, bool& Jugeflg, BrString& Sizekey);
	void ClearLines();
	void MakeRebar(EditElementHandleR start, vector<RebarVertices>& m_rebarPts, vector<BrString>& m_vecDir, ElementId conid);

protected:
	virtual void PostNcDestroy();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_REBARLINK };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnBnClickedCheck7();
	afx_msg void OnBnClickedButton2();
};
