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
		double   dRadiusOut;	  // 外圆半径
		double   dRadiusInn;	  // 内圆半径
		double	 dHeight;		  // 高度
		DPoint3d centerpt;		  // 弧的中心点
		DPoint3d ArcDPs[2];		  // 弧起点和终点
		UInt16       type;         //元素类型

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


#endif              //——3