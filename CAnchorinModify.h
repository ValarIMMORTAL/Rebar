#pragma once
#include "CommonFile.h"
//#include "ElementAlgorithm.h"


// CAnchorinModify 对话框

class CAnchorinModify : public CDialogEx
{
	DECLARE_DYNAMIC(CAnchorinModify)

public:
	CAnchorinModify(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CAnchorinModify();

	struct AnchorinModifyInfo
	{
		int	   anchorinStyle;   // 锚固样式  0 -- 弯锚  1 -- 直锚  2 -- 直锚带端板
		double anchorinAngle;	// 锚固角度
		double anchorinLength;	// 锚固长度
		
		AnchorinModifyInfo()
		{
			anchorinStyle = 0;
			anchorinAngle = 0.0;
			anchorinLength = 0.0;
		}
	};

	std::vector<ElementRefP> m_selectrebars;
	BOOL OnInitDialog();

	void SetAnchorinModeifyInfo(bool bFlag = true);

	void movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag = true);

	void Push_RebarData();

	void UpdateRebarData(vector<RebarVertices>& rebarPts);

	void ClearLines();

	void DarwRebarLine(const vector<RebarVertices>& m_rebarPts);

private:
	CEdit						m_EditLength;			// 锚固长度
	CEdit						m_EditAngle;			// 锚固方向
	CComboBox					m_CombStyle;			// 锚固新式 0 -- 弯锚 1 -- 直锚  2 -- 直锚带端板

	AnchorinModifyInfo			m_stAnchorinModifyInfo;
	double					    m_Diameter;

	vector<BrString>			m_vecDir;				// 钢筋尺寸
	vector<RebarVertices>		m_rebarPts;				// 钢筋顶点
	vector<RebarVertices>		m_rebarPtsBack;			// 钢筋顶点 -- 备份 示意线时使用

	vector<ElementRefP>			m_allLines;				// 示意线

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_AnchorinModify };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedOk();
	afx_msg void OnCbnSelchangeComboRtype();
	afx_msg void OnEnKillfocusEdit1();
	afx_msg void OnEnKillfocusEdit2();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButton1();
};
