#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	CAssociatedComponentNew
*	Project:		三维配筋出图项目
*	Author:			LiuXiang
*	Date:			2021/04/12
	Version:		V1.0
*	Description:	CAssociatedComponentNew
*	History:
*	1. Date:		2021/04/12
*	Author:			LiuXiang
*	Modification:	create file
*
**************************************************************/

#include "ScanIntersectTool.h"

class CAssociatedComponentNew
{
public:
	CAssociatedComponentNew(ElementHandle eh):m_NowElm(eh)
	{
		GetAllIntersectDatas();
		GetBothEndIntersectDatas();
	}

	~CAssociatedComponentNew() {}

public:
	const vector<IntersectEle>& GetACCData() { return m_InSDatas; }
	const vector<IntersectEle>& GetBothEndACCData() { return m_BothEndInSDatas; }
	const vector<IntersectEle>& GetIntersectSlab() { return m_InSSlabDatas; }

	ElementHandle GetElementHandle() { return m_NowElm; }

	//获取左端关联构件数据的
protected:
	void GetAllIntersectDatas();
	void GetBothEndIntersectDatas();

private:
	bool GetComponentCenterLine(ElementHandleCR eeh, vector<DPoint3d> &vecVertex, double *fuzzyThickness);
private:
	ElementHandle			m_NowElm;				//当前选中的墙
	vector<IntersectEle>	m_InSDatas;				//关联构件的数据，用于展示
	vector<IntersectEle>	m_BothEndInSDatas;		//两端关联构件的数据，用于配筋
	vector<IntersectEle>	m_InSSlabDatas;			//关联板的数据，用于配筋
};

typedef CAssociatedComponentNew  CACCNew;
