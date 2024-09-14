#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include "CFaceTieRebarToolAssembly.h"

// TieRebarFaceDlg 对话框

class TieRebarFaceDlg : public CDialogEx
{
	DECLARE_DYNAMIC(TieRebarFaceDlg)

public:
	TieRebarFaceDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~TieRebarFaceDlg();
	virtual BOOL OnInitDialog();
	void SetFaceId(ElementId faceid);
	void InitRebarSetsAndehSel();

	std::vector<ElementRefP> m_selectrebars;
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_TieRebarFace1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
private:
	ElementId m_uFaceId;
	ElementHandle					m_ehSel;
	FaceTieReBarInfo					m_tieRebarInfo;
	RebarAssembly*					m_pRebarAssembly;
	ElementId						m_contid;

	std::vector<PIT::ConcreteRebar>		m_vecRebarData;

	CFaceTieRebarToolAssembly*			m_pTieRebarAssembly;
public:
	CComboBox m_tieRebarStyle;
	CComboBox m_tieRebarType;
	CComboBox m_TieRebarDiameter;
//	CEdit m_ange1;
//	CEdit m_angel2;
	CEdit m_angle1;
	CEdit m_angle2;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnSelchangeCombo1();
};


struct CreateFaceTool : public DgnPrimitiveTool
{
private:
	bvector<DPoint3d>   m_points;
	EditElementHandle m_eeh;
	TieRebarFaceDlg* m_pTie;
public:

	CreateFaceTool(int toolID, TieRebarFaceDlg* pTie) : DgnPrimitiveTool(toolID, 0)
	{
		m_pTie = pTie;
	}

	virtual void _OnPostInstall() override;
	virtual void _OnRestartTool() override;
	virtual bool _OnDataButton(DgnButtonEventCR ev) override;
	virtual bool _OnResetButton(DgnButtonEventCR ev) override;
	virtual void _OnDynamicFrame(DgnButtonEventCR ev) override;
	//virtual void _OnUndoPreviousStep() override;
	//StatusInt       _OnElementModify(EditElementHandleR  el) override { return ERROR; }
	bool CreateElement(EditElementHandleR eeh, bvector<DPoint3d> const& points);
	//void CreateAcceptedSegmentsTransient();
public:

	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	static void InstallNewInstance(int toolId, TieRebarFaceDlg* pTie);

};