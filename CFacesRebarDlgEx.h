#pragma once

#include "CFacesMainRebarDlgEx.h"
#include "CFacesRebarEndTypeDlg.h"
#include "CommonFile.h"
#include "FacesRebarAssemblyEx.h"
#include "PlaneRebarAssemblyEx.h"
#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"


// CFacesRebarDlgEx 对话框

class CFacesRebarDlgEx : public CDialogEx
{
	DECLARE_DYNAMIC(CFacesRebarDlgEx)

	//元素面信息
	struct EleFace
	{
		ElementId eehId;					//元素id
		vector<EditElementHandle*> faces;	//元素的所有面
		ElementHandle eeh;					//元素
	};

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_FacesRebarEx };
#endif

public:
	CFacesRebarDlgEx(ElementHandleCR ehOld, ElementId ehNew, const bvector<ISubEntityPtr>& faces, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CFacesRebarDlgEx();

	PIT::Concrete							m_Concrete;
	std::vector<PIT::ConcreteRebar>			m_vecRebarData;
	std::vector<PIT::EndType>				m_vecEndTypeData;

	CComboBox	m_ComboSize;//尺寸
	CComboBox	m_ComboType;//型号
	CEdit		m_EditSpace;//间距
	CStatic		m_static_wallname;
	WallSetInfo m_WallSetInfo;
	bvector<ISubEntityPtr> m_faces;

	int m_CurSelTab;
	CFacesMainRebarDlgEx				m_PageMainRebar;			//主要配筋
	CFacesRebarEndTypeDlg				m_PageEndType;				//端部样式
	CDialog*							pDialog[2];					//用来保存对话框对象指针

	CTabCtrl m_tab;
	FacesRebarAssemblyEx* m_FaceRebarPtr;

	void SetSelectElement(ElementHandleCR eh) { _ehOld = eh; }
	ElementHandle GetSelectElement() { return _ehOld; }
	void SetConcreteId(ElementId id) { m_ConcreteId = id; }

	void SetIsHide(bool isHide){ m_isHide = isHide; }

	void SetDlgType(int FaceDlgType){ m_FaceDlgType = FaceDlgType; }

	void multiSlabOrWallRebar(EditElementHandleR eeh, DgnModelRefP modelRef);

	/*
	* @desc:	多板合并配筋（不等厚处理）
	* @param[in]	eeh 板元素
	* @param[in]	modelRef 模型
	* @author	hzh
	* @Date:	2022/11/24
	*/
	void multiDiffThickSlabOrWallRebar(EditElementHandleR eeh, DgnModelRefP modelRef);

	void perpendicularFaceReabr(vector<double>& vecDis, ElementId& contid, EditElementHandleR eeh, DgnModelRefP modelRef);
	//面配经主要函数入口
	void normalFaceRebar(ElementId& contid, EditElementHandleR eeh, DgnModelRefP modelRef);

	void multiFaceInlineRebar(ElementId& contid, EditElementHandleR eeh, DgnModelRefP modelRef);

	void PreviewRebarLines();

	virtual BOOL OnInitDialog();
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnTcnSelchangeTabFacerebarex(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnStnClickedStaticWallname();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	DECLARE_MESSAGE_MAP()

private:
	ElementHandle _ehOld;
	ElementId _ehNew;
	bvector<ISubEntityPtr> m_selectfaces;
	vector<ElementHandle> _ehOlds;
	vector<vector<EditElementHandle*> > m_vvecUpFace;
	vector<vector<EditElementHandle*> > m_vvecDownFace;
	ElementId m_ConcreteId;
	map<ElementId, EleFace> m_eleFaces;							//元素id与面信息的映射
	map<ElementId, vector<EditElementHandle*>> m_eleUpFaces;	//元素id与对应的上面集合
	map<ElementId, vector<EditElementHandle*>> m_eleDownFaces;	//元素id与对应的下面集合

	bool m_isHide;
	int  m_FaceDlgType; // 0: 正常面配筋 1:多板联合配筋
	string wall_name;

};