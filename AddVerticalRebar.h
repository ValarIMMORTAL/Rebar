#pragma once
#include "CommonFile.h"
#include "GalleryIntelligentRebarids.h"
#include <RebarDetailElement.h>
class AddVerticalRebar
{
public:
	vector<RebarVertices> m_rebarPts;
	vector<BrString> m_vecDir;

	map<int, vector<RebarPoint>> m_mapselectrebars;
	std::vector<ElementRefP> m_selectrebars;
	std::vector<ElementRefP> m_Verticalrebars;
	vector<ElementRefP> m_allLines;
	AddVerticalRebar();   // ��׼���캯��
	virtual ~AddVerticalRebar();
	void SorSelcetRebar();

	void DrawVerticalReabr();
	
};

