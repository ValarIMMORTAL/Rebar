#pragma once
#include "ListCtrlEx.h"
#include "CommonFile.h"

class CMainRebarListCtrl :public ListCtrlEx::CListCtrlEx
{
public:
	void GetAllRebarData(vector<PIT::ConcreteRebar>& vecListData);
	void GetUniteRebarData(vector<PIT::ConcreteRebar>& vecListData);
	void GetAllRebarData_wall(vector<PIT::ConcreteRebar>& vecListData);
	void SetisWall() { m_isWall = true; }
	bool m_isWall = false;
	DECLARE_MESSAGE_MAP()
	// message handlers
	afx_msg void		OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	
};

// class CInsertRebarListCtrl :public ListCtrlEx::CListCtrlEx
// {
// public:
// 	void GetAllRebarData(vector<InsertRebarInfo::WallInfo>& vecWallInsertData);
// };

//�ֽ��
// class CustomRebarlListCtrl :public ListCtrlEx::CListCtrlEx
// {
// public:
// 	 void GetAllRebarData(vector<CustomRebarl>& vecWCustomRebarl);
// };

class CRebarEndTypeListCtrl :public ListCtrlEx::CListCtrlEx
{
public:
	void GetAllRebarData(vector<PIT::EndType>& vecListData);
	void SetRebarEndPointInfo(int irow, const PIT::EndType::RebarEndPointInfo& endPointInfo) {
		m_rebarEndPointInfos[irow] = endPointInfo; }

private:
	map<int, PIT::EndType::RebarEndPointInfo> m_rebarEndPointInfos;
};

// class CRebarDomeListCtrl :public ListCtrlEx::CListCtrlEx
// {
// public:
// 	void GetAllRebarData(vector<PIT::DomeLevelInfo>& vecListData);

// 	void GetAllRebarData(vector<PIT::DomeLevelDetailInfo>& vecListData);
// };

class CTwinBarListCtrl :public ListCtrlEx::CListCtrlEx
{
public:
	void GetAllRebarData(vector<TwinBarSet::TwinBarLevelInfo>& vecListData);
};
class CWallRebarAssociatedCtrl :public ListCtrlEx::CListCtrlEx
{
public:
	void GetAllRebarData(vector<PIT::AssociatedComponent>& vecListData);

public:
	
	ElementAgenda  affectedElements;//���ζԻ����и���ѡ���Ԫ��
	DECLARE_MESSAGE_MAP()
	// message handlers
	afx_msg void		OnLButtonDown(UINT nFlags, CPoint point);
};

// class CBeamListCtrl : public ListCtrlEx::CListCtrlEx
// {
// public:

// 	void GetAllRebarData(vector<BeamRebarInfo::BeamAreaVertical>& vecBeamAreaData);

// 	void GetAllRebarData(vector<BeamRebarInfo::BeamCommHoop>& vecBeamCommHoop);
// };


 class CBaseRebarListCtrl :public ListCtrlEx::CListCtrlEx
 {
 public:
 	void GetAllRebarData(vector<PIT::ConcreteRebar>& vecListData);

 };

class CBreakEllipseWallListCtrl :public ListCtrlEx::CListCtrlEx
{
public:
	void GetAllRebarData(vector<PIT::BreakAngleData>& vecListData);

	DECLARE_MESSAGE_MAP()
	// message handlers
	afx_msg void		OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
};
