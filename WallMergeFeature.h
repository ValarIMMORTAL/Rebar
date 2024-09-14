#ifndef _WAllMERGRFEATURE_H_    
#define _WAllMERGRFEATURE_H_   
#pragma once
#include <RebarElements.h>
#include "CommonFile.h"
#include "PITRebarEndType.h"
#include "PITRebarCurve.h"
#include "PITArcSegment.h"
#include "PITRebarAssembly.h"

namespace PITI
{
	struct ELLWallGeometryInfo
	{
		double   dRadiusOut;	  // ��Բ�뾶
		double   dRadiusInn;	  // ��Բ�뾶
		double	 dHeight;		  // �߶�
		DPoint3d centerpt;		  // �������ĵ�
		DPoint3d ArcDPs[2];		  // �������յ�
		UInt16       type;         //Ԫ������

	};
	enum WallType
	{
		STWALL,
		GWALL,
		ARCWALL,
		ELLIPSEWall,
		Other
	};
}

 bool CSjointboard(ElementAgendaR selectedElement, EditElementHandle& combinewall);

 bool WallRebarDlg(ElementAgendaR selectedElement, EditElementHandle& combinewall, double high, bool mark);

 bool ArcWallMerge(ElementAgendaR selectedElement, EditElementHandle& combinewall);


#endif              //����3