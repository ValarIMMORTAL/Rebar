#pragma once
/***********************************************************
*	Copyright (c) power-itech 2021. All rights reserved.
*	Module Name:	CAssociatedComponentNew
*	Project:		��ά����ͼ��Ŀ
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

	//��ȡ��˹����������ݵ�
protected:
	void GetAllIntersectDatas();
	void GetBothEndIntersectDatas();

private:
	bool GetComponentCenterLine(ElementHandleCR eeh, vector<DPoint3d> &vecVertex, double *fuzzyThickness);
private:
	ElementHandle			m_NowElm;				//��ǰѡ�е�ǽ
	vector<IntersectEle>	m_InSDatas;				//�������������ݣ�����չʾ
	vector<IntersectEle>	m_BothEndInSDatas;		//���˹������������ݣ��������
	vector<IntersectEle>	m_InSSlabDatas;			//����������ݣ��������
};

typedef CAssociatedComponentNew  CACCNew;
