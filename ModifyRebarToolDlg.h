#pragma once
#include "CommonFile.h"


// ModifyRebarToolDlg 对话框

class ModifyRebarToolDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ModifyRebarToolDlg)

public:
	ModifyRebarToolDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~ModifyRebarToolDlg();

	/*
	* @desc:	设置选择钢筋（首尾钢筋或需要修改的钢筋）
	* @param[in]	rebars 	选择钢筋
	* @author	hzh
	* @Date:	2022/09/23
	*/
	void SetSelectRebars(const vector<ElementRefP>& rebars);

	/*
	* @desc:	设置参考钢筋
	* @param[in]	rebar 参考钢筋	
	* @author	hzh
	* @Date:	2022/09/23
	*/
	void SetSelectRefRebars(const ElementRefP& rebar);
	

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_ModifyRebarTool };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

private:
	struct doubleKeyComp {
	public:
		bool operator()(const double& a, const double& b)const {
			return COMPARE_VALUES_EPS(a, b, 1) == -1;
		}
	};
	ElementRefP m_selectRefRebar;										//参考钢筋
	vector<ElementRefP> m_selectrebars;									//选择钢筋
	map<double, vector<ElementRefP>, doubleKeyComp> m_modifyRebars;		//钢筋距离首根钢筋的长度与钢筋集合的映射
	map<ElementId, ElementRefP> m_updateRebars;							//需要修改的钢筋，为了修改时使用
	double m_spacing;													//钢筋间隔
	int m_rebarGrade;													//钢筋等级
	string m_rebarDia;													//钢筋直径
	map<ElementId, RebarVertices> m_rebarVertices;						//钢筋的端点信息
	vector<ElementRefP> m_refLines;										//参考线
	//参考钢筋信息
	struct RefRebarInfo
	{
		DPoint3d startPt;	//开始点
		DPoint3d endPt;		//结束点
		double diameter;	//直径
		DVec3d verVec;		//垂直方向
	};
	RefRebarInfo m_refRebarInfo;	//参考钢筋信息

	CComboBox m_rebarGradeCombo;	//钢筋等级
	CComboBox m_rebarDiaCombo;		//钢筋直径
	CEdit m_rebarsSpaceEdit;		//钢筋间隔


private:
	/*
	* @desc:	初始化
	* @param[in]	eeh 用以初始化对话框信息的钢筋	
	* @author	hzh	
	* @Date:	2022/09/23
	*/
	void Init(EditElementHandle& eeh);

	/*
	* @desc:	计算需要修改的钢筋
	* @remark	如果选择两个钢筋，则修改两个钢筋中间的所有钢筋，否则修改选择的钢筋
	* @author	hzh
	* @Date:	2022/09/23
	*/
	void CalculateModifyRebars();

	/*
	* @desc:	清除预览线	
	* @author	hzh
	* @Date:	2022/09/23
	*/
	void ClearRefLine();

	/*
	* @desc:	获取修改后的钢筋端点信息
	* @author	hzh
	* @Date:	2022/09/23
	*/
	void GetRebarVerticies();

	/*
	* @desc:	预览
	* @author	hzh
	* @Date:	2022/09/23
	*/
	void PreviewRefLines();
	
public:
	/*
	* @desc:	修改钢筋等级
	* @author	hzh
	* @Date:	2022/09/23
	*/
	afx_msg void OnCbnSelchangeComboGrade();

	/*
	* @desc:	修改钢筋直径
	* @author	hzh
	* @Date:	2022/09/23
	*/
	afx_msg void OnCbnSelchangeComboDiameter();

	/*
	* @desc:	确定
	* @author	hzh
	* @Date:	2022/09/23
	*/
	afx_msg void OnBnClickedOk();

	/*
	* @desc:	取消
	* @author	hzh
	* @Date:	2022/09/23
	*/
	afx_msg void OnBnClickedCancel();

	/*
	* @desc:	修改间隔
	* @author	hzh
	* @Date:	2022/09/23
	*/
	afx_msg void OnEnChangeEditSpace();

	/*
	* @desc:	选择钢筋
	* @author	hzh
	* @Date:	2022/09/23
	*/
	afx_msg void OnBnClickedButtonSelect();
};
