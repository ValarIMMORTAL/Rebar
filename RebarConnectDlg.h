#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include "PITRebarCurve.h"
using namespace PIT;
// RebarConnectDlg 对话框

class RebarConnectDlg : public CDialogEx
{
	DECLARE_DYNAMIC(RebarConnectDlg)

public:
	RebarConnectDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~RebarConnectDlg();
	virtual BOOL OnInitDialog();
	struct RebarConnectInfo
	{
		int RebarLinkMethod;//连接方式
		bool missflg;//为true则错开
		double MissLen;//错开长度
		double LapLen;
		bool ChangeFlg;//为True则切换
		//bool tranLenthFlg;//选择延长的钢筋组

	}m_RebarConnectInfo;//界面信息
	int m_sizekey;//钢筋sizekey
	BrString m_SSizekey;
	//map<int, vector<ElementRefP>> m_mapselectrebars;
	std::vector<ElementRefP> m_selectrebars;//选择的钢筋集合
	//double m_dLineHigh;
	double m_maxlowZ;//下层钢筋最高点Z坐标
	double m_minHighZ;//上层钢筋最低点Z坐标

	map<ElementId, std::vector<ElementRefP>> map_vecLowRebarSet;//下层钢筋
	map<ElementId, std::vector<ElementRefP>> map_vecHighRebarSet;//上层钢筋
	vector<ElementRefP> m_vecAllLines;


	//************************************
	// 函数功能:   根据选择的钢筋集合计算出上下两层的竖直钢筋排序后放入map_vecLowRebarSet和map_vecHighRebarSet中
	// 返回类型:   void
	//作者:chenxuan   日期：2022/04/14
	//************************************
	void CalcRebarSet();
	//************************************
	// 函数功能:    删除上下两层钢筋中没有对应的钢筋的钢筋
	// 返回类型:   void
	// 参数: map<ElementId,std::vector<ElementRefP>> & map_vecLowRebarSet 下层钢筋
	// 参数: map<ElementId,std::vector<ElementRefP>> & map_vecHighRebarSet 上层钢筋
	//作者:chenxuan   日期：2022/04/14
	//************************************
	void DeleteAsymmetricalRebar(map<ElementId, std::vector<ElementRefP>>& map_vecLowRebarSet, map<ElementId, std::vector<ElementRefP>>& map_vecHighRebarSet);
	//************************************
	// 函数功能:    钢筋按rebarset分组
	// 返回类型:   void
	// 参数: map<ElementId
	// 参数: std::vector<ElementRefP>> & map_result 分组结果
	// 参数: std::vector<ElementRefP> selectRef 所有钢筋
	//作者:chenxuan   日期：2022/04/14
	//************************************
	void DevidRebarGroup(map<ElementId, std::vector<ElementRefP>>& map_result, std::vector<ElementRefP> selectRef);
	//************************************
	// 函数功能:    计算钢筋的起点和终点
	// 返回类型:   void
	// 参数: RebarElementP rep  钢筋
	// 参数: DPoint3d & PtStar 起点
	// 参数: DPoint3d & PtEnd 中点
	// 参数: double & diameter 直径
	// 参数: DgnModelRefP modelRef 
	//作者:chenxuan   日期：2022/04/14
	//************************************
	static void CalaRebarStartEnd(RebarElementP rep, DPoint3d& PtStar, DPoint3d& PtEnd, double& diameter, DgnModelRefP modelRef);
	//************************************
	// 函数功能:    按照选好的参数更新钢筋
	// 返回类型:   void
	//作者:chenxuan   日期：2022/04/14
	//************************************
	void UpdateVecRebar();
	//************************************
	// 函数功能:    获取钢筋的curve;该函数直接拷贝过来使用
	// 返回类型:   bool
	// 参数: DPoint3d ptstr
	// 参数: DPoint3d ptend
	// 参数: vector<PITRebarCurve> & rebars
	// 参数: PITRebarEndTypes & endTypes
	// 参数: double dSideCover
	//作者:chenxuan   日期：2022/04/14
	//************************************
	bool makeRebarCurve(DPoint3d ptstr, DPoint3d ptend, vector<PITRebarCurve>& rebars, PITRebarEndTypes&	endTypes, double dSideCover);
	//************************************
	// 函数功能:    对钢筋进行排序，按照X或者Y坐标
	// 返回类型:   bool
	// 参数: ElementRefP elmentA
	// 参数: ElementRefP elmentB
	//作者:chenxuan   日期：2022/04/14
	//************************************
	static bool sortRebarElementbyXorYpos(ElementRefP elmentA, ElementRefP elmentB);
	//************************************
	// 函数功能:    画预览线
	// 返回类型:   void
	//作者:chenxuan   日期：2022/04/14
	//************************************
	void DrawPreviewLine();
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_RebarConnect };
#endif

public:
	CComboBox m_ComboConnectMethod;
	CEdit  m_EidtMissLen;
	CButton m_Checkmiss;
	CButton m_ChangeButton;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedSelectCorssPos();
	afx_msg void OnEnChangeEditCrossLength();
	afx_msg void OnBnClickedCrossCheck();
	afx_msg void OnBnClickedCrossChange();
	afx_msg void OnBnClickedButtonRebarconnectpreview();

	DECLARE_MESSAGE_MAP()
	
};
