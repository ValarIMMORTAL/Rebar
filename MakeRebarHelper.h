#pragma once
#include "CommonFile.h"
#include "PITRebarEndType.h"

class MakeRebarHelper
{
public:
	/*
	* @desc:	����endtype�Ͷ˲������ȡ���˲���ʽ
	* @param[in]	endType ���ö˲���ʽ
	* @param[in]	vecEndNormal �˲�����
	* @param[out]	pitRebarEndTypes ���˲���ʽ
	* @author	Hong ZhuoHui
	* @Date:	2023/09/14
	*/
	static void GetRebarEndTypesByEndTypeAndVec(const vector<PIT::EndType>& endType, const vector<CVector3D>& vecEndNormal,
		const BrString& sizeKey, PIT::PITRebarEndTypes& pitRebarEndTypes);

};

