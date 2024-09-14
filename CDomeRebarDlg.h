#pragma once
#include "CommonFile.h"
#include "CWallRebarListCtrl.h"
//#include "ElementAlgorithm.h"
#include "CDomeDetailDlg.h"

// CDomeRebarDlg 对话框

class CDomeRebarDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDomeRebarDlg)

public:
	CDomeRebarDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDomeRebarDlg();

	virtual BOOL OnInitDialog();

	void DeleteRound();

	void addCutRadius()
	{
		if (m_editRoundHandle.IsValid())
		{
			m_editRoundHandle.AddToModel();
		}
	}

	void SetSelectElement(ElementHandleCR eh) { m_ehSel = eh; }

	ElementHandle GetSelectElement() { return m_ehSel; }

	void GetvecVertex(vector<st_VertexVec>& vecVertex)
	{
		vecVertex = m_vecVertex;
	}

	void GetCover(double& dCover)
	{
		dCover = m_Cover;
	}

	void SetCover(double& dCover)
	{
		m_Cover = dCover;
	}

	void SetNumber(int nNumber)
	{
		m_Number = nNumber;
	}

	void GettargetTrans(TransformR targetTrans)
	{
		targetTrans = m_targetTrans;
	}

	void GetScaleLength(double& scaleLen)
	{
		scaleLen = m_dScaleLength;
	}

	void GetCircleCenter(DPoint3d& circleCenter)
	{
		circleCenter = m_circleCenter;
	}

	void SetvecDomeLevelInfo(vector<PIT::DomeLevelInfo>& vecDomeLevelInfo)
	{
		m_vecDomeLevelInfo = vecDomeLevelInfo;
	}

	void SetmapDomeLevelDetailInfo(map<int, vector<PIT::DomeLevelDetailInfo>>& mapDomeLevelDetailInfo)
	{
		m_mapDomeLevelDetailInfo = mapDomeLevelDetailInfo;
	}

	void GetvecDomeLevelInfo(vector<PIT::DomeLevelInfo>& vecDomeLevelInfo)
	{
		this->m_listCtl.GetAllRebarData(m_vecDomeLevelInfo);
		vecDomeLevelInfo = m_vecDomeLevelInfo;
	}

	void GetmapDomeLevelDetailInfo(map<int, vector<PIT::DomeLevelDetailInfo>>& mapDomeLevelDetailInfo)
	{
		if (m_pDomeDetailDlg != NULL)
		{
			vector<PIT::DomeLevelDetailInfo> vecDomeLevelDetailInfo;
			m_pDomeDetailDlg->GetDomeLevelDetailInfo(vecDomeLevelDetailInfo);
			if (vecDomeLevelDetailInfo.size() > 0)
			{
				auto itr = m_mapDomeLevelDetailInfo.find(vecDomeLevelDetailInfo.at(0).nNumber);
				if (itr == m_mapDomeLevelDetailInfo.end())
				{
					m_mapDomeLevelDetailInfo.insert(make_pair(vecDomeLevelDetailInfo.at(0).nNumber, vecDomeLevelDetailInfo));
				}
				else
				{
					itr->second = vecDomeLevelDetailInfo;
				}
			}
		}
		mapDomeLevelDetailInfo = m_mapDomeLevelDetailInfo;
	}

	void UpdateRebarList();

public:

	int								m_Number;
	CEdit							m_EditNumber;
	CRebarDomeListCtrl				m_listCtl;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_DomeRebarDlg };
#endif

private:
	CEdit									m_EditCover;				// 保护层
	double									m_Cover;

	ElementHandle							m_ehSel;
	EditElementHandle						m_editRoundHandle;	// 画顶面示意圆

	vector<st_VertexVec>					m_vecVertex;   // 启动几何点数据
	Transform								m_targetTrans; // 穹顶几何位置信息

	DPoint3d								m_circleCenter; // 穹顶顶面中心点
	double									m_dScaleLength; // 穹顶顶面中心和底面中心的距离

	CDomeDetailDlg*							m_pDomeDetailDlg;

	vector<PIT::DomeLevelInfo>						 m_vecDomeLevelInfo;		// 穹顶配筋范围信息
	map<int, vector<PIT::DomeLevelDetailInfo>>		 m_mapDomeLevelDetailInfo;  // 穹顶每层钢筋具体信息 map

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLvnItemchangedList2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeEdit1();
	afx_msg LRESULT OnButtonDown(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEnKillfocusEdit2();
};
