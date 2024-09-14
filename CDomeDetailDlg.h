#pragma once
#include "CommonFile.h"
#include "CWallRebarListCtrl.h"
//#include "ElementAlgorithm.h"

// CDomeDetailDlg 对话框

class CDomeDetailDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDomeDetailDlg)

public:
	CDomeDetailDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDomeDetailDlg();

	virtual BOOL OnInitDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_DomeDetail };
#endif

public:
	CRebarDomeListCtrl	m_listCtl;

	void SetDomeLevelDetailInfo(vector<PIT::DomeLevelDetailInfo>& vecDomeLevelDetailInfo)
	{
		if (m_LayoutType == 1)
		{
			for (int i = 0; i < vecDomeLevelDetailInfo.size(); i++)
			{
				if (vecDomeLevelDetailInfo.at(i).rebarShape == 1 && COMPARE_VALUES_EPS(vecDomeLevelDetailInfo.at(i).dAngleOrSpace, 50, EPS) > 0)
				{
					vecDomeLevelDetailInfo.at(i).dAngleOrSpace = 0.5;
				}

				if (vecDomeLevelDetailInfo.at(i).rebarShape == 0 && COMPARE_VALUES_EPS(vecDomeLevelDetailInfo.at(i).dAngleOrSpace, 50, EPS) > 0)
				{
					vecDomeLevelDetailInfo.at(i).dAngleOrSpace = 3.0;
				}
			}
		}

		m_vecDomeLevelDetailInfo = vecDomeLevelDetailInfo;
	}

	void GetDomeLevelDetailInfo(vector<PIT::DomeLevelDetailInfo>& vecDomeLevelDetailInfo)
	{
		m_listCtl.GetAllRebarData(m_vecDomeLevelDetailInfo);
		vecDomeLevelDetailInfo = m_vecDomeLevelDetailInfo;
	}

	void SetLayoutType(int& LayoutType) // 设置钢筋布置方式 0 : XY正交  1 : 环径正交		
	{
		m_LayoutType = LayoutType;
	}

	void UpdateRebarList();

	void SetListDefaultData();

private:
	CEdit									 m_EditLevelNum; // 钢筋层数

	vector<PIT::DomeLevelDetailInfo>		 m_vecDomeLevelDetailInfo;  // 穹顶每层钢筋具体信息

	int										 m_LevelNum;
	int										 m_LayoutType;     // 钢筋布置方式 0 : XY正交  1 : 环径正交										

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnKillfocusEdit1();
};
