#pragma once
#include "CWallRebarListCtrl.h"

// CBreakEllipseWallDlg 对话框

class CBreakEllipseWallDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBreakEllipseWallDlg)

public:
	CBreakEllipseWallDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CBreakEllipseWallDlg();

	virtual BOOL OnInitDialog();

	/*
	* @desc:	设置选择的墙
	* @param[in]	eh 墙		
	* @author	Hong ZhuoHui
	* @Date:	2023/05/20
	*/
	bool SetSelectElement(ElementHandleCR eh);

	vector<PIT::BreakAngleData> GetBreakAngleData() const { return m_vecListData; }

	void SetListData(const vector<PIT::BreakAngleData>& listData) { m_vecListData = listData; }

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_BreakEllipseWall };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	/*
	* @desc:	更新list数据
	* @author	Hong ZhuoHui
	* @Date:	2023/05/20
	*/
	void UpdateListData();

	/*
	* @desc:	初始化list数据
	* @author	Hong ZhuoHui
	* @Date:	2023/05/20
	*/
	void InitListData();

private:
	CBreakEllipseWallListCtrl m_listBreakCtrl;	//断开数据list
	CEdit m_editBreakNum;						//断开数量
	size_t m_breakNum = 0;							//断开数量
	ElementHandle m_selEh;						//选择墙体
	double m_beginAngle = 0;					//总的开始角度
	double m_endAngle = 360;					//总的结束角度
	vector<PIT::BreakAngleData> m_vecListData;	//断开后的角度数据
public:
	/*
	* @desc:	断开数量编辑框取消聚焦的相应函数	
	* @author	Hong ZhuoHui
	* @Date:	2023/05/20
	*/
	afx_msg void OnEnKillfocusEditBreaknum();

	/*
	* @desc:	确定
	* @author	Hong ZhuoHui
	* @Date:	2023/05/20
	*/
	afx_msg void OnBnClickedOk();

	/*
	* @desc:	取消	
	* @author	Hong ZhuoHui
	* @Date:	2023/05/20
	*/
	afx_msg void OnBnClickedCancel();
};
