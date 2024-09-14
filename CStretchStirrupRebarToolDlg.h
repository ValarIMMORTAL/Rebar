#pragma once
#include "CommonFile.h"
#include "StirrupRebar.h"

// CStretchStirrupRebarToolDlg 对话框

class CStretchStirrupRebarToolDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CStretchStirrupRebarToolDlg)

public:
	CStretchStirrupRebarToolDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CStretchStirrupRebarToolDlg();

	std::vector<ElementRefP> m_selectrebars;				//选中的箍筋
	/*
	* @desc:	保存选中的箍筋的点
	* @param	Vctpts 选中的箍筋的点集合	
	* @author	hzh
	* @Date:	2022/05/27
	*/
	void SaveRebarPts(vector< vector<CPoint3D>>& Vctpts);
	/*
	* @desc:	保存箍筋数据
	* @param	RebarSize 箍筋rebarsize数据
	* @param	vecElm 选中箍筋元素id集合	
	* @author	hzh
	* @Date:	2022/05/27
	*/
	void SaveRebarData(BrString& RebarSize, vector <ElementId>&vecElm);

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_STRETCHSTIRRUPREBAR };
#endif

private:
	std::vector<vector<CPoint3D>> m_vctpts;						//所有箍筋的点
	vector<ElementId> m_vecElm_H;								//选中的钢筋集合
	char    m_SizeKey[512];
	int		m_rebarType;										//钢筋型号
	char    m_rebarSize[512];									//钢筋尺寸
	std::string mSelectedRebarType;
	vector<vector<ElementRefP>> m_allLines;						//预览线集合
	double m_stretchLen;										//拉伸长度
	vector<shared_ptr<PIT::StirrupRebar>>	m_pStirrupRebars;	//箍筋
	bool m_dirIsPositive = true;								//拉伸方向是否为正向

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	/*
	* @desc:	画预览线
	* @author	hzh
	* @Date:	2022/05/27
	*/
	void PreviewLines();

	/*
	* @desc:	清空预览线
	* @author	hzh
	* @Date:	2022/05/27
	*/
	void clearLines();

	/*
	* @desc:	给箍筋设置rebardata参数，构造箍筋对象
	* @author	hzh
	* @Date:	2022/05/27
	*/
	void ProcessRebarData();

	/*
	* @desc:	设置选中的线段的两个端点
	* @param	startPoint 线段的一个端点
	* @param	endPoint 线段的另一个端点	
	* @remark	该函数会对选中的箍筋的点做平移计算，即会计算出拉伸后的m_vctpts
	* @author	hzh
	* @Date:	2022/05/27
	*/
	void SetEditLinePoint(DPoint3dCR startPoint, DPoint3dCR endPoint);

public:
	CComboBox m_ComboSize;		//箍筋尺寸
	CComboBox m_ComboType;		//箍筋型号
	CComboBox m_ComboDirection;	//拉伸方向

public:
	/*
	* @desc:	改变拉伸长度
	* @author	hzh
	* @Date:	2022/05/27
	*/
	afx_msg void OnEnChangeStretchLen();

	/*
	* @desc:	预览
	* @author	hzh
	* @Date:	2022/05/27
	*/
	afx_msg void OnBnClickedPreview();

	/*
	* @desc:	取消
	* @author	hzh
	* @Date:	2022/05/27
	*/
	afx_msg void OnBnClickedCancel();

	/*
	* @desc:	确定
	* @author	hzh
	* @Date:	2022/05/27
	*/
	afx_msg void OnBnClickedOk();

	/*
	* @desc:	选择参考线
	* @author	hzh
	* @Date:	2022/05/27
	*/
	afx_msg void OnBnClickedChooseEditLine();

	/*
	* @desc:	切换拉伸方向
	* @author	hzh
	* @Date:	2022/05/27
	*/
	afx_msg void OnCbnSelchangeDirection();

	/*
	* @desc:	修改箍筋直径
	* @author	hzh
	* @Date:	2022/09/01
	*/
	afx_msg void OnCbnSelchangeComboRabardia();
};
