#pragma once
#include "CommonFile.h"
#include "PITRebarEndType.h"

class MakeRebarHelper
{
public:
	/*
	* @desc:	根据endtype和端部方向获取配筋端部样式
	* @param[in]	endType 设置端部样式
	* @param[in]	vecEndNormal 端部方向
	* @param[out]	pitRebarEndTypes 配筋端部样式
	* @author	Hong ZhuoHui
	* @Date:	2023/09/14
	*/
	static void GetRebarEndTypesByEndTypeAndVec(const vector<PIT::EndType>& endType, const vector<CVector3D>& vecEndNormal,
		const BrString& sizeKey, PIT::PITRebarEndTypes& pitRebarEndTypes);

};

