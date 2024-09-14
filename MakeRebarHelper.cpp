#include "MakeRebarHelper.h"
#include "RebarHelper.h"
#include "PITRebarCurve.h"

/*
* @desc:	����endtype�Ͷ˲������ȡ���˲���ʽ
* @param[in]	endType ���ö˲���ʽ
* @param[in]	vecEndNormal �˲�����
* @param[out]	pitRebarEndTypes ���˲���ʽ
* @author	Hong ZhuoHui
* @Date:	2023/09/14
*/
void MakeRebarHelper::GetRebarEndTypesByEndTypeAndVec(const vector<PIT::EndType>& endType, const vector<CVector3D>& vecEndNormal,
	const BrString& sizeKey, PIT::PITRebarEndTypes & pitRebarEndTypes)
{
	RebarEndType endTypeStart, endTypeEnd;
	if (endType.size() != vecEndNormal.size() || endType.size() == 0)
	{
		return;
	}

	double startbendRadius, endbendRadius;
	double startbendLen, endbendLen;
	double begStraightAnchorLen, endStraightAnchorLen;
	switch (endType[0].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeStart.SetType(RebarEndType::kLap);
		begStraightAnchorLen = endType[0].endPtInfo.value1;	//ê�볤��
		break;
	case 4:	//90���乳
	{
		endTypeStart.SetType(RebarEndType::kBend);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, ACTIVEMODEL, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, ACTIVEMODEL);	//������100
		}
	}
	break;
	case 5:	//135���乳
	{
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, ACTIVEMODEL, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, ACTIVEMODEL);	//������100
		}
	}
	break;
	case 6:	//180���乳
	{
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, ACTIVEMODEL, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, ACTIVEMODEL);	//������100
		}
	}
	break;
	case 8:	//�û�
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (endType[1].endType)
	{
	case 0:	//��
	case 1:	//����
	case 2:	//����
	case 3:	//����
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//ֱê
		endTypeEnd.SetType(RebarEndType::kLap);
		endStraightAnchorLen = endType[1].endPtInfo.value1;	//ê�볤��
		break;
	case 4:	//90���乳
	{
		endTypeEnd.SetType(RebarEndType::kBend);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, ACTIVEMODEL, false);	//������30
		}
		endbendLen = endType[1].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, ACTIVEMODEL);	//������100
		}
	}
	break;
	case 5:	//135���乳
	{
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, ACTIVEMODEL, false);	//������30
		}
		endbendLen = endType[1].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, ACTIVEMODEL);	//������100
		}
	}
	break;
	case 6:	//180���乳
	{
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, ACTIVEMODEL, false);	//������30
		}
		endbendLen = endType[1].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, ACTIVEMODEL);	//������100
		}
	}

	break;
	case 8:	//�û�
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	PIT::PITRebarEndType start, end;
	start.SetType((PIT::PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	start.SetbendLen(startbendLen);
	start.SetbendRadius(startbendRadius);	
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	end.SetbendLen(endbendLen);
	end.SetbendRadius(endbendRadius);
	end.SetendNormal(vecEndNormal[1]);

	pitRebarEndTypes = { start, end };
}
