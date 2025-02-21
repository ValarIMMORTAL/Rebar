#pragma once

#include "CFacesMainRebarDlgEx.h"
#include "CommonFile.h"
#include "FacesRebarAssembly.h"
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
	CDialog*							pDialog[1];					//用来保存对话框对象指针

	CTabCtrl m_tab;
	FacesRebarAssembly * m_FaceRebarPtr;

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

struct          SelectFaceTool3 : LocateSubEntityTool
{
	ElementHandle	_eh;
	ElementId		_ehNew;
	int				_selectNum;
	CFacesRebarDlgEx *m_ptr;
public:


	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	SelectFaceTool3(int cmdName, int prompt, ElementHandleCR ehOld, ElementId ehNew, CFacesRebarDlgEx *ptr, int selectNum = 1) :_eh(ehOld), _ehNew(ehNew), _selectNum(selectNum), m_ptr(ptr)
	{
		SetCmdName(cmdName, prompt);
	}
protected:
	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual bool _CollectCurves() override { return false; } // Tool does not support wire bodies...wire bodies won't be collected.
	virtual bool _CollectSurfaces() override { return false; } // Tool does not support sheet bodies...sheet bodies won't be collected.

	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual BentleyStatus _OnProcessSolidPrimitive(ISolidPrimitivePtr& geomPtr, DisplayPathCR path) override { return ERROR; } // Promote capped surface to solid body...
	virtual BentleyStatus _OnProcessPolyface(PolyfaceHeaderPtr& geomPtr, DisplayPathCR path) override { return SUCCESS; } // Don't convert a closed mesh to a BRep (and don't collect), can be expensive for large meshes...

	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual ISubEntity::SubEntityType _GetSubEntityTypeMask() override { return ISubEntity::SubEntityType_Face; }
	virtual bool _RequireSubEntitySupport() override { return true; } // Require solid w/at least 1 face...
	virtual bool _AcceptIdentifiesSubEntity() { return false; } // Solid accept point may also accept first face (except hollow which can apply to entire body)...
	virtual bool _AllowMissToAccept(DgnButtonEventCR ev) { return __super::_AllowMissToAccept(ev); } // Don't require face for hollow...

	/*---------------------------------------------------------------------------------**//**
	* Return true if this element should be accepted for the modify operation.
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual bool _IsElementValidForOperation(ElementHandleCR eh, HitPathCP path, WStringR cantAcceptReason) override
	{
		// Base class implementation returns true if geometry cache isn't empty, which in this case means the cache contains at least 1 BRep solid.
		// To be valid for modification element should be fully represented by a single solid; reject if there are multiple solid bodies or missing geometry.
		// NOTE: Simple count test is sufficient (w/o also checking TryGetAsBRep) as override of _Collect and _OnProcess methods have tool only collecting BRep solids.
		return (__super::_IsElementValidForOperation(eh, path, cantAcceptReason) && 1 == GetElementGraphicsCacheCount(eh) && !IsGeometryMissing(eh));
	}


	/*---------------------------------------------------------------------------------**//**
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual StatusInt _OnElementModify(EditElementHandleR eeh) override
	{
		m_ptr->m_faces = GetAcceptedSubEntities();
		mdlSelect_freeAll();

		return ERROR;
	}

	/*---------------------------------------------------------------------------------**//**
	* Install a new instance of the tool. Will be called in response to external events
	* such as undo or by the base class from _OnReinitialize when the tool needs to be
	* reset to it's initial state.
	* @bsimethod                                                              Bentley Systems
	+---------------+---------------+---------------+---------------+---------------+------*/
	virtual void _OnRestartTool() override
	{
		/*bvector<ISubEntityPtr> faces = GetAcceptedSubEntities();
		if (faces.size() == 0)
		{
			EditElementHandle eehDel(_ehNew, ACTIVEMODEL);
			eehDel.DeleteFromModel();
			mdlInput_sendSynchronizedKeyin(L"displayset clear", 0, INPUTQ_EOQ, NULL);
			_ExitTool();
			return;
		}*/
		SelectFaceTool3* tool = new SelectFaceTool3(1, 1, _eh, _ehNew, m_ptr);
		tool->InstallTool();
	}

	virtual bool _OnResetButton(DgnButtonEventCR  ev) override
	{
		/*EditElementHandle eehDel(_ehNew, ACTIVEMODEL);
		eehDel.DeleteFromModel();
		mdlInput_sendSynchronizedKeyin(L"displayset clear", 0, INPUTQ_EOQ, NULL);
		_ExitTool();*/
		SelectFaceTool3* tool = new SelectFaceTool3(1, 1, _eh, _ehNew, m_ptr);
		tool->InstallTool();
		return true;
	}

	bool    IsRebarDetailSet(ElementHandleCR eh) { return NULL != RebarDetailSet::Fetch(eh); }

	bool	_IsModifyOriginal() override { return false; }//使能选择参考文件中的元素
// 	virtual void    _OnPostInstall() override
// 	{
// 		ElementCopyContextP pElmcopy = _GetCopyContext();
// 		pElmcopy->AddHandler()
// 	}

	virtual bool _OnPostLocate(HitPathCP path, WStringR cantAcceptReason) override
	{
		if (!DgnElementSetTool::_OnPostLocate(path, cantAcceptReason))
			return false;

		ElementHandle eh(path->GetHeadElem(), path->GetRoot());

		return !(RebarElement::IsRebarElement(eh) || IsRebarDetailSet(eh));
	}
};