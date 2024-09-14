#include "_USTATION.h"
#include "AssociatedComponent.h"

bool CAssociatedComponent::GetComponentCenterLine(ElementHandleCR eeh, vector<DPoint3d> &vecVertex,double *fuzzyThickness)
{
	vector<DPoint3d>().swap(vecVertex);
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle eehNow;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(eeh, eehNow, Holeehs);
	for (int j = 0; j < Holeehs.size(); j++)
	{
		delete Holeehs.at(j);
		Holeehs.at(j) = nullptr;
	}
	Holeehs.clear();
	ExtractFacesTool::GetFrontBackLineAndDownFace(eehNow, NULL, vecDownFaceLine, vecDownFontLine, vecDownBackLine, NULL);
	if (vecDownFaceLine.empty() || vecDownFontLine.empty() || vecDownBackLine.empty())
		return false;

	vector<DPoint3d> vecFontLinePt;
	for (size_t i = 0; i < vecDownFontLine.size(); ++i)
	{
		DPoint3d ptStart, ptEnd;
		mdlElmdscr_extractEndPoints(&ptStart, NULL, &ptEnd, NULL, vecDownFontLine[i], eeh.GetModelRef());
		if (i == 0)
			vecFontLinePt.push_back(ptStart);
		vecFontLinePt.push_back(ptEnd);
	}

	vector<DPoint3d> vecBackLinePt;
	for (size_t i = 0; i < vecDownBackLine.size(); ++i)
	{
		DPoint3d ptStart, ptEnd;
		mdlElmdscr_extractEndPoints(&ptStart, NULL, &ptEnd, NULL, vecDownBackLine[i], eeh.GetModelRef());
		if (i == 0)
			vecBackLinePt.push_back(ptStart);
		vecBackLinePt.push_back(ptEnd);
	}

	for (size_t i = 0; i < vecDownFaceLine.size(); i++)
	{
		mdlElmdscr_freeAll(&vecDownFaceLine[i]);
	}

	EditElementHandle eehLineStr;
	if (vecFontLinePt.size() > vecBackLinePt.size())
	{
		reverse(vecBackLinePt.begin(), vecBackLinePt.end());	//����
		LineStringHandler::CreateLineStringElement(eehLineStr, NULL, &vecFontLinePt[0], vecFontLinePt.size(), eeh.GetModelRef()->Is3d(), *eeh.GetModelRef());
		//������ĵ�����ͶӰ��Ԫ����ȡ���ĵ�
		if (eehLineStr.IsValid())
		{
			double dthicknessStart, dthicknessEnd;
			for (size_t i = 0; i < vecBackLinePt.size(); ++i)
			{
				DPoint3d ptCenter;
				DPoint3d ptPro;
				mdlProject_perpendicular(&ptPro, NULL, NULL, eehLineStr.GetElementDescrP(), eeh.GetModelRef(), &vecBackLinePt[i], NULL, 1e-6);
				if (0 == i)	//��㲻����
				{
					ptCenter.x = (vecBackLinePt[0].x + vecFontLinePt[0].x) * 0.5;
					ptCenter.y = (vecBackLinePt[0].y + vecFontLinePt[0].y) * 0.5;
					ptCenter.z = (vecBackLinePt[0].z + vecFontLinePt[0].z) * 0.5;
					dthicknessStart = ptPro.Distance(vecBackLinePt[i]);
				}
				else if (i == vecBackLinePt.size() -1)//�յ�Ҳ������
				{
					ptCenter.x = (vecBackLinePt[i].x + vecFontLinePt[vecFontLinePt.size() - 1].x) * 0.5;
					ptCenter.y = (vecBackLinePt[i].y + vecFontLinePt[vecFontLinePt.size() - 1].y) * 0.5;
					ptCenter.z = (vecBackLinePt[i].z + vecFontLinePt[vecFontLinePt.size() - 1].z) * 0.5;
					dthicknessEnd = ptPro.Distance(vecBackLinePt[i]);
				}
				else
				{
					//�����ͶӰ
					ptCenter.x = (vecBackLinePt[i].x + ptPro.x) * 0.5;
					ptCenter.y = (vecBackLinePt[i].y + ptPro.y) * 0.5;
					ptCenter.z = (vecBackLinePt[i].z + ptPro.z) * 0.5;
				}
				vecVertex.push_back(ptCenter);
			}
			if (fuzzyThickness != NULL)	//����ģ�����
				*fuzzyThickness = dthicknessStart > dthicknessEnd ? dthicknessStart : dthicknessEnd;
		}
	}
	else
	{
		reverse(vecFontLinePt.begin(), vecFontLinePt.end());	//����
		LineStringHandler::CreateLineStringElement(eehLineStr, NULL, &vecBackLinePt[0], vecBackLinePt.size(), eeh.GetModelRef()->Is3d(), *eeh.GetModelRef());
		//��ǰ��ĵ�����ͶӰ��Ԫ����ȡ���ĵ�
		if (eehLineStr.IsValid())
		{
			double dthicknessStart, dthicknessEnd;
			for (size_t i = 0; i < vecFontLinePt.size(); ++i)
			{
				DPoint3d ptCenter;
				DPoint3d ptPro;
				mdlProject_perpendicular(&ptPro, NULL, NULL, eehLineStr.GetElementDescrP(), eeh.GetModelRef(), &vecFontLinePt[i], NULL, 1e-6);
				if (0 == i)	//��㲻����
				{
					ptCenter.x = (vecFontLinePt[0].x + vecBackLinePt[0].x) * 0.5;
					ptCenter.y = (vecFontLinePt[0].y + vecBackLinePt[0].y) * 0.5;
					ptCenter.z = (vecFontLinePt[0].z + vecBackLinePt[0].z) * 0.5;
					dthicknessStart = ptPro.Distance(vecFontLinePt[i]);
				}
				else if (i == vecFontLinePt.size() - 1)//�յ�Ҳ������
				{
					ptCenter.x = (vecFontLinePt[i].x + vecBackLinePt[vecBackLinePt.size() - 1].x) * 0.5;
					ptCenter.y = (vecFontLinePt[i].y + vecBackLinePt[vecBackLinePt.size() - 1].y) * 0.5;
					ptCenter.z = (vecFontLinePt[i].z + vecBackLinePt[vecBackLinePt.size() - 1].z) * 0.5;
					dthicknessEnd = ptPro.Distance(vecFontLinePt[i]);
				}
				else
				{
					//�����ͶӰ
					ptCenter.x = (vecFontLinePt[i].x + ptPro.x) * 0.5;
					ptCenter.y = (vecFontLinePt[i].y + ptPro.y) * 0.5;
					ptCenter.z = (vecFontLinePt[i].z + ptPro.z) * 0.5;
				}
				vecVertex.push_back(ptCenter);
			}
			if (fuzzyThickness != NULL)	//����ģ�����
				*fuzzyThickness = dthicknessStart > dthicknessEnd ? dthicknessStart : dthicknessEnd;
		}
	}
	return true;
}

void CAssociatedComponent::GetAllIntersectDatas()
{
	vector<IntersectEle>().swap(m_InSDatas);
	EleIntersectDatas Same_Eles;
	GetSameIntersectDatas(m_NowElm, Same_Eles);
	for (IntersectEle tmphd : Same_Eles.InSDatas)
	{
		string name = tmphd.EleName;
		size_t pos = name.find("-");
		if (pos == -1)
		{
			pos = name.find("/");
		}
		if (pos > 4)
		{
			name = name.substr(pos - 4);
		}
		if (name.find("EB") == string::npos && name.find("NB") == string::npos && name.find("DB") == string::npos)
			m_InSDatas.push_back(tmphd);
//		if (tmphd.EleName.find("DB") != string::npos)
//			m_InSSlabDatas.push_back(tmphd);
	}

	EleIntersectDatas Up_Eles;
	EleIntersectDatas Down_Eles;
	GetUpAndDownIntersectDatas(m_NowElm, Up_Eles, Down_Eles);
	for (IntersectEle tmphd : Up_Eles.InSDatas)
	{
		string name = tmphd.EleName;
		size_t pos = name.find("-");
		if (pos == -1)
		{
			pos = name.find("/");
		}
		if (pos > 4)
		{
			name = name.substr(pos - 4);
		}
		if (name.find("EB") == string::npos && name.find("NB") == string::npos)
			m_InSDatas.push_back(tmphd);
		if (name.find("DB") != string::npos)
			m_InSSlabDatas.push_back(tmphd);
	}

	for (IntersectEle tmphd : Down_Eles.InSDatas)
	{
		string name = tmphd.EleName;
		size_t pos = name.find("-");
		if (pos == -1)
		{
			pos = name.find("/");
		}
		if (pos > 4)
		{
			name = name.substr(pos - 4);
		}
		if (name.find("EB") == string::npos && name.find("NB") == string::npos)
			m_InSDatas.push_back(tmphd);
// 		if (tmphd.EleName.find("DB") != string::npos)
// 			m_InSSlabDatas.push_back(tmphd);
	}
}

void CAssociatedComponent::GetBothEndIntersectDatas()
{
	if (!m_InSDatas.size())
		GetAllIntersectDatas();

	vector<IntersectEle>().swap(m_BothEndInSDatas);
	//���˵��������������Ĺ���
	//�ֱ�������������ĵ��������ߵĽ��㵽����ľ��룬�Ըþ������ж��Ƿ�Ϊ���˵Ĺ�������
	DgnModelRefP model = m_NowElm.GetModelRef();
	vector<DPoint3d> vecNowVertex;
	double dFuzzyThickness;
	if (!GetComponentCenterLine(m_NowElm, vecNowVertex, &dFuzzyThickness))
		return;

	m_BothEndInSDatas.clear();
	for (IntersectEle tmphd : m_InSDatas)
	{
		vector<DPoint3d> vecVertex;
		double dFuzzyThickness1;
		//�ж��������ߵĽ���
		if (GetComponentCenterLine(tmphd.Eh, vecVertex, &dFuzzyThickness1))
		{
			//�ֱ��������߶����յ��߶�֮��Ľ���
			DPoint3d ptPro1, ptPro2, ptPro3, ptPro4;

			//��ǰ������������㵽������������߶ε�ͶӰ��
			mdlVec_projectPointToLine(&ptPro1, NULL, &vecNowVertex[0], &vecVertex[0], &vecVertex[1]);
			double dis1 = ptPro1.Distance(vecNowVertex[0]);		//��ǰ������㵽ͶӰ��ľ���
			//��ǰ������������㵽���������յ��߶ε�ͶӰ��
			mdlVec_projectPointToLine(&ptPro2, NULL, &vecNowVertex[0], &vecVertex.back(), &vecVertex[vecVertex.size() - 2]);
			double dis2 = ptPro2.Distance(vecNowVertex[0]);		//��ǰ������㵽ͶӰ��ľ���
			//��ǰ�����������յ㵽������������߶ε�ͶӰ��
			mdlVec_projectPointToLine(&ptPro3, NULL, &vecNowVertex.back(), &vecVertex[0], &vecVertex[1]);
			double dis3 = ptPro3.Distance(vecNowVertex.back());	//��ǰ�����յ㵽ͶӰ��ľ���
			//��ǰ�����������յ㵽���������յ��߶ε�ͶӰ��
			mdlVec_projectPointToLine(&ptPro4, NULL, &vecNowVertex.back(), &vecVertex.back(), &vecVertex[vecVertex.size() - 2]);
			double dis4 = ptPro4.Distance(vecNowVertex.back());	//��ǰ�����յ㵽ͶӰ��ľ���

			//ģ���жϾ����Ƿ�С�ں��,����һ������С�ں�ȼ���˲�����
			if (dis1 < dFuzzyThickness || dis2 < dFuzzyThickness || dis3 < dFuzzyThickness || dis3 < dFuzzyThickness)
				m_BothEndInSDatas.push_back(tmphd);
		}
	}
}
