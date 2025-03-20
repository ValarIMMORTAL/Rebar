#include "_ustation.h"
#include "resource.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "STGWallRebarAssembly.h"
#include "WallRebarDlg.h"
#include "ExtractFacesTool.h"
#include "CWallRebarDlg.h"
#include "TieRebar.h"
#include "ElementAttribute.h"
#include "PITMSCECommon.h"
#include "ExtractFacesTool.h"
#include "XmlHelper.h"
#include "CSolidTool.h"
#include "CFaceTool.h"
#include <unordered_set>
// #include "SelectRebarTool.h"
#include <RebarHelper.h>
// #include "HoleRebarAssembly.h"
#include <CPointTool.h>

#include "MakeRebarHelper.h"

extern bool PreviewButtonDown;//��Ҫ�������Ԥ����ť
extern map<int, vector<RebarPoint>> g_wallRebarPtsNoHole;

using namespace PIT;

bool STGWallRebarAssembly::AnalyzingWallGeometricData(ElementHandleCR eh)
{
	DgnModelRefP model = eh.GetModelRef();
	double uor_per_mm = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	m_frontLinePts.clear();
	m_backLinePts.clear();
	m_doorsholes.clear();
	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFrontLine;
	vector<MSElementDescrP> vecDownBackLine;
	EditElementHandle testeeh(eh, false);
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	GetDoorHoles(Holeehs, m_doorsholes);
	if (!GetFrontBackLines(Eleeh, vecDownFrontLine, vecDownBackLine))
	{
		return false;
	}

	auto GetPt = [](const MSElementDescrP& lineDecrP, LinePt& pt) {
		mdlElmdscr_extractEndPoints(&pt.startPt, nullptr, &pt.endPt, nullptr, lineDecrP, ACTIVEMODEL);
		if (COMPARE_VALUES_EPS(pt.startPt.x, pt.endPt.x, 1) == 1)
		{
			DPoint3d tmppt = pt.startPt;
			pt.startPt = pt.endPt;
			pt.endPt = tmppt;
		}
		else if (COMPARE_VALUES_EPS(pt.startPt.y, pt.endPt.y, 1) == 1)
		{
			DPoint3d tmppt = pt.startPt;
			pt.startPt = pt.endPt;
			pt.endPt = tmppt;
		}
	};
	for (auto it : vecDownFrontLine)
	{
		//mdlElmdscr_add(it);
		LinePt pt;
		GetPt(it, pt);
		m_frontLinePts.push_back(pt);
	}
	for (auto it : vecDownBackLine)
	{
		//mdlElmdscr_add(it);
		LinePt pt;
		GetPt(it, pt);
		m_backLinePts.push_back(pt);
	}

	bool isreverse = false;
	if (m_backLinePts.size() > m_frontLinePts.size())
	{
		vector<LinePt> tmpPts = m_backLinePts;
		m_backLinePts = m_frontLinePts;
		m_frontLinePts = tmpPts;
		isreverse = true;
	}
	map<int, vector<DPoint3d>> linePts;
	int index = 0;
	for (auto it : m_backLinePts)
	{
		for (auto frontIt : m_frontLinePts)
		{
			DPoint3d startPt;
			StatusInt startRet = mdlVec_projectPointToLine(&startPt, nullptr, &frontIt.startPt, &it.startPt, &it.endPt);
			bool flag = false;
			startRet = ExtractFacesTool::IsPointInLine(startPt, it.startPt, it.endPt, ACTIVEMODEL, flag);
			DPoint3d endPt;
			StatusInt endRet = mdlVec_projectPointToLine(&endPt, nullptr, &frontIt.endPt, &it.startPt, &it.endPt);
			endRet = ExtractFacesTool::IsPointInLine(endPt, it.startPt, it.endPt, ACTIVEMODEL, flag);
			//��ʼ�㶼����ͶӰ����������ʼ����յ���һ������ͶӰ����back����ʼ����յ���Ϊ�ֶε���ʼ����յ�
			DPoint3d frontStrPt = frontIt.startPt, frontEndPt = frontIt.endPt;
			if (!startRet && !endRet)
			{
				continue;
			}
			if (!endRet)
			{
				endPt = it.endPt;
				mdlVec_projectPointToLine(&frontEndPt, nullptr, &endPt, &frontIt.startPt, &frontIt.endPt);
			}
			if (!startRet)
			{
				startPt = it.startPt;
				mdlVec_projectPointToLine(&frontStrPt, nullptr, &startPt, &frontIt.startPt, &frontIt.endPt);
			}
			//ȥ��̫�̵��߶�
			double dis = startPt.Distance(endPt);
			if (COMPARE_VALUES_EPS(dis, 10, 0) == -1)
			{
				continue;
			}

			vector<DPoint3d> pts;
			if (isreverse)
			{
				pts.push_back(startPt);
				pts.push_back(endPt);
				pts.push_back(frontStrPt);
				pts.push_back(frontEndPt);
			}
			else
			{
				pts.push_back(frontStrPt);
				pts.push_back(frontEndPt);
				pts.push_back(startPt);
				pts.push_back(endPt);
			}
			linePts[index] = pts;
			index++;
		}
	}

	SetLinePts(linePts);
	m_width = 0;
	for (auto it : linePts)
	{
		double width = it.second.at(0).Distance(it.second.at(2));
		if (COMPARE_VALUES_EPS(m_width, width, 1) == -1)
		{
			m_width = width;
		}
	}
	CalcWallsInRange();
	m_Holeehs = Holeehs;
	return true;
}

void STGWallRebarAssembly::CalculateUseHoles(DgnModelRefP modelRef)
{
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double misssize = uor_per_mm * g_wallRebarInfo.concrete.MissHoleSize;
	m_useHoleehs.clear();
	double dSideCover = GetSideCover()*uor_per_mm;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	Transform trans;
	GetPlacement().AssignTo(trans);
	trans.InverseOf(trans);

	if (g_wallRebarInfo.concrete.isHandleHole)//������Ҫ����Ŀ׶�
	{
		for (int j = 0; j < m_Holeehs.size(); j++)
		{
			EditElementHandle eeh;
			eeh.Duplicate(*m_Holeehs.at(j));
			bool isdoorNeg = false;//�ж��Ƿ�Ϊ�Ŷ�NEG
			isdoorNeg = IsDoorHoleNeg(m_Holeehs.at(j), m_doorsholes);
			ISolidKernelEntityPtr entityPtr;
			if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
			{
				SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
			}
			if (isdoorNeg)
			{
				PlusSideCover(eeh, dSideCover, matrix, isdoorNeg, m_width);
			}
			//DPoint3d ptcenter = getCenterOfElmdescr(eeh.GetElementDescrP());
			//vector<DPoint3d> interpts;
			//DPoint3d tmpStr, tmpEnd;
			//tmpStr = m_STwallData.ptStart;
			//tmpEnd = m_STwallData.ptEnd;
			//tmpStr.z = tmpEnd.z = ptcenter.z;
			//GetIntersectPointsWithHole(interpts, &eeh, tmpStr, tmpEnd);
			//if (interpts.size() > 0)
			//{
			TransformInfo transinfo(trans);
			eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
			DPoint3d minP;
			DPoint3d maxP;
			//����ָ��Ԫ����������Ԫ�صķ�Χ��
			mdlElmdscr_computeRange(&minP, &maxP, eeh.GetElementDescrP(), NULL);
			DRange3d range;
			range.low = minP;
			range.high = maxP;
			bool isNeed = false;
			if (range.XLength() > misssize || range.ZLength() > misssize)
			{
				isNeed = true;
			}

			if (isNeed)
			{
				if (m_doorsholes[m_Holeehs.at(j)] != nullptr)//������Ŷ�
				{
					continue;
				}
				ElementCopyContext copier(ACTIVEMODEL);
				copier.SetSourceModelRef(m_Holeehs.at(j)->GetModelRef());
				copier.SetTransformToDestination(true);
				copier.SetWriteElements(false);
				copier.DoCopy(*m_Holeehs.at(j));
				if (isdoorNeg)
				{
					PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix, isdoorNeg, m_width);
				}
				else
				{
					PlusSideCover(*m_Holeehs.at(j), dSideCover, matrix);
				}

				m_useHoleehs.push_back(m_Holeehs.at(j));
			}
			//}

		}
	}
	if (m_useHoleehs.size() > 1)
	{
		UnionIntersectHoles(m_useHoleehs, m_Holeehs);
	}

}

bool STGWallRebarAssembly::SetWallData(ElementHandleCR eh)
{
	bool bRet = AnalyzingWallGeometricData(eh);
	if (!bRet)
		return false;
	//InitUcsMatrix();
	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
}

bool STGWallRebarAssembly::MakeRebars(DgnModelRefP modelRef)
{
	map<int, vector<DPoint3d>> linePts = GetLinePts();
	if (linePts.empty())
	{
		return false;
	}
	if (m_frontLinePts.empty() && m_backLinePts.empty())
		return false;

	InitLevelHoriInfos();

	EditElementHandle testeeh(GetSelectedElement(), GetSelectedModel());
	EditElementHandle Eleeh;
	std::vector<EditElementHandle*> Holeehs;
	EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
	SetSelectedElement(Eleeh.GetElementId());
	if (g_globalpara.Getrebarstyle() != 0)
	{
		NewRebarAssembly(modelRef);
	}
	SetSelectedElement(testeeh.GetElementId());
	for (int j = 0; j < Holeehs.size(); j++)
	{
		if (Holeehs.at(j) != nullptr)
		{
			delete Holeehs.at(j);
			Holeehs.at(j) = nullptr;
		}
	}
	RebarSetTagArray rsetTags;
	m_useHoleehs.clear();
	CalculateUseHoles(modelRef);

	double uor_ref = GetSelectedModel()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	//for (auto it : m_useHoleehs)
	//{
	//	it->AddToModel();
	//}

	int numtag = 0;

	m_vecAllRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();
	g_vecTieRebarPtsNoHole.clear();

	double thickness = 0;
	GetMaxThickness(modelRef, thickness);

	//�жϵ�ǰǽ������ǽ�Ĺ�ϵ
	auto UpdateWallPos = [](DVec3d& refVec, const DPoint3d& tmpStrPt, const DPoint3d& tmpEndPt)->WallPos {
		WallPos wallPos = Horizontal;
		if (COMPARE_VALUES_EPS(tmpStrPt.Distance(tmpEndPt), 10, 1) == -1)
		{
			return wallPos;
		}
		DVec3d tmpVec = tmpStrPt - tmpEndPt;
		refVec.Normalize();
		tmpVec.Normalize();
		if (tmpVec.DotProduct(refVec) > 0)
		{
			wallPos = IN_WALL;
		}
		else
		{
			wallPos = OUT_WALL;
		}

		return wallPos;
	};

	for (int k = 0; k < linePts.size(); k++)
	{

		if (linePts[k].size() != 4)
		{
			continue;
		}
		double  sidecover = GetSideCover();//ȡ�ò��汣����
		m_sidecover = GetSideCover();

		GetUcsAndStartEndData(k, thickness, modelRef, true);

		if (thickness >= MaxWallThickness * uor_per_mm)
		{
			//SetSideCover(0);//�Ƚ����汣��������Ϊ0			
		}
		CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kYaxis);
		double dLevelSpace = 0;
		double dSideCover = m_sidecover * uor_per_mm;
		if (COMPARE_VALUES(dSideCover, m_STwallData.width) >= 0)	//������汣������ڵ���ǽ�ĳ���
		{
			mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"���汣������ڵ���ǽ�ĳ���,�޷������ֽ��", MessageBoxIconType::Information);
			return false;
		}

		vector<CVector3D> vTrans;
		vector<CVector3D> vTransTb;
		//�����������ƫ����
		CalculateTransform(vTrans, vTransTb, modelRef);
		if (vTrans.size() != GetRebarLevelNum())
		{
			return false;
		}

		double dLength = m_STwallData.length;
		double dWidth = m_STwallData.height;

		int iRebarSetTag = 0;
		int iRebarLevelNum = GetRebarLevelNum();
		int iTwinbarSetIdIndex = 0;
		vector<PIT::EndType> vecEndType;
		for (int i = 0; i < iRebarLevelNum; ++i)
		{
			RebarSetTag* tag = NULL;
			CMatrix3D   mat, matTb;

			vector<PIT::EndType> vecEndType;
			if (GetvvecEndType().empty())		//û�����ö˲���ʽ������Ĭ��ֵ
			{
				EndType endType;
				memset(&endType, 0, sizeof(endType));
				vecEndType = { endType,endType };
			}
			else
			{
				vecEndType = GetvvecEndType().at(i);
			}

			CVector3D tmpVector(m_LineNormal);
			tmpVector.Scale(vTrans[i].y);
			CMatrix3D   tmpmat;
			tmpmat.SetTranslation(tmpVector);
			double  Misdisstr, Misdisend = 0;
			double tLenth;
			CalculateNowPlacementAndLenth(Misdisstr, Misdisend, tLenth, tmpmat, modelRef);

			vector<CVector3D> tmpEndNormal(2);
			WallPos leftWall = Horizontal, rightWall = Horizontal;
			double leftDis = 0, rightDis = 0; //����ǽ�뵱ǰǽ��ƫ�����
			if (GetvecDataExchange().at(i) == 0) //front
			{
				if (k != 0) //����ǽ
				{
					DPoint3d tmpStrPt = linePts.at(k - 1).at(1);
					DPoint3d tmpEndPt = linePts.at(k).at(0);
					leftDis = tmpStrPt.Distance(tmpEndPt);
					DVec3d refVec = linePts.at(k).at(2) - linePts.at(k).at(0);
					leftWall = UpdateWallPos(refVec, tmpStrPt, tmpEndPt);
					DVec3d tmpVec = tmpStrPt - tmpEndPt;
					tmpVec.Normalize();
					if (leftWall == IN_WALL)
					{
						tmpEndNormal.at(0) = tmpVec;
					}
				}
				if (k != linePts.size() - 1) //����ǽ
				{
					DPoint3d tmpStrPt = linePts.at(k + 1).at(0);
					DPoint3d tmpEndPt = linePts.at(k).at(1);
					rightDis = tmpStrPt.Distance(tmpEndPt);
					DVec3d refVec = linePts.at(k).at(3) - linePts.at(k).at(1);
					rightWall = UpdateWallPos(refVec, tmpStrPt, tmpEndPt);
					DVec3d tmpVec = tmpStrPt - tmpEndPt;
					tmpVec.Normalize();
					if (rightWall == IN_WALL)
					{
						tmpEndNormal.at(1) = tmpVec;
					}
				}
			}
			else //back
			{
				if (k != 0) //����ǽ
				{
					DPoint3d tmpStrPt = linePts.at(k - 1).at(3);
					DPoint3d tmpEndPt = linePts.at(k).at(2);
					leftDis = tmpStrPt.Distance(tmpEndPt);
					DVec3d refVec = linePts.at(k).at(0) - linePts.at(k).at(2);
					leftWall = UpdateWallPos(refVec, tmpStrPt, tmpEndPt);
					DVec3d tmpVec = tmpStrPt - tmpEndPt;
					tmpVec.Normalize();
					if (leftWall == IN_WALL)
					{
						tmpEndNormal.at(0) = tmpVec;
					}
				}
				if (k != linePts.size() - 1) //����ǽ
				{
					DPoint3d tmpStrPt = linePts.at(k + 1).at(2);
					DPoint3d tmpEndPt = linePts.at(k).at(3);
					rightDis = tmpStrPt.Distance(tmpEndPt);
					DVec3d refVec = linePts.at(k).at(1) - linePts.at(k).at(3);
					rightWall = UpdateWallPos(refVec, tmpStrPt, tmpEndPt);
					DVec3d tmpVec = tmpStrPt - tmpEndPt;
					tmpVec.Normalize();
					if (rightWall == IN_WALL)
					{
						tmpEndNormal.at(1) = tmpVec;
					}
				}
			}

			if (vTrans.size() != GetRebarLevelNum())
			{
				return false;
			}
			double diameter = RebarCode::GetBarDiameter(GetvecDirSize().at(i), modelRef) / 2;
			m_vecRebarPtsLayer.clear();
			m_vecTwinRebarPtsLayer.clear();
			m_vecTieRebarPtsLayer.clear();
			if (GetvecDir().at(i) == 1)	//����ֽ�
			{
				bool drawlast = true;
				//if (i <= 1 && thickness >= MaxWallThickness * uor_per_mm&&k != linePts.size() - 1)//������600�������ǵ�һ�λ������Ҳ������һ��ǽ�������һ��������
				//{
				//	drawlast = false;
				//}

				double misDisH_left, misDisH_right;

				if (COMPARE_VALUES_EPS(m_angle_left, PI / 2, 0.001))
				{
					misDisH_left = (1 / sin(m_angle_left) - 1)*diameter + Misdisstr;
				}
				else
				{
					misDisH_left = 0;
				}
				if (COMPARE_VALUES_EPS(m_angle_right, PI / 2, 0.001))
				{
					misDisH_right = (1 / sin(m_angle_right) - 1)*diameter + Misdisend;
				}
				else
				{
					misDisH_right = 0;
				}

				vector<CVector3D> vecEndNormal(2);
				CVector3D	endNormal;	//�˲��乳����
				if (GetvvecEndType().size() > 0)
				{
					for (size_t k = 0; k < vecEndNormal.size(); ++k)
					{
						double dRotateAngle = vecEndType.at(k).rotateAngle;
						endNormal = m_STwallData.ptEnd - m_STwallData.ptStart;
						endNormal.Normalize();
						if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
						{
							endNormal.Negate();
						}
						CVector3D rebarVec = CVector3D::kZaxis;
						/*					endNormal = rebarVec.CrossProduct(vec);*/
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
						vecEndNormal[k] = endNormal;
					}
				}
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;
				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
				//���Ʋ���--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
				{
					SetIsPushTieRebar(false);
					//�Ȼ��Ʒǲ����
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dLength, dWidth,
						GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat,
						GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i),
						GetvecDataExchange().at(i), false, modelRef, drawlast, false);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);
					}

					SetIsPushTieRebar(true);
					//���Ʋ����
					tag = MakeRebars(PopvecSetId().at(iTwinbarSetIdIndex + iRebarLevelNum), GetvecDirSize().at(i),
						dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm,
						GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal,
						matTb, GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i),
						GetvecDataExchange().at(i), true, modelRef, drawlast, false);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1 + numtag);
						rsetTags.Add(tag);
					}
					iTwinbarSetIdIndex++;
				}
				else //��ǰ��δ���ò���
				{
					SetIsPushTieRebar(true);
					TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dLength, dWidth,
						GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm + misDisH_left,
						GetvecEndOffset().at(i)*uor_per_mm + misDisH_right, vecEndType, vecEndNormal, mat,
						twinRebar, GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i),
						false, modelRef, drawlast, false);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);
					}
				}
				vecEndType.clear();
			}
			else
			{
				double misDisV_left, misDisV_right;
				if (COMPARE_VALUES_EPS(m_angle_left, PI / 2, 0.001))
				{
					misDisV_left = diameter / tan(m_angle_left);
				}
				else
				{
					misDisV_left = 0;
				}
				if (COMPARE_VALUES_EPS(m_angle_right, PI / 2, 0.001))
				{
					misDisV_right = diameter / tan(m_angle_right);
				}
				else
				{
					misDisV_right = 0;
				}
				double leftSideCov = m_sidecover * uor_per_mm / sin(m_angle_left);
				double rightSideCov = m_sidecover * uor_per_mm / sin(m_angle_right);
				double allSideCov = leftSideCov + rightSideCov;

				tLenth = tLenth - (misDisV_left + misDisV_right);
				vTrans[i].x = (tLenth - allSideCov) / 2 + Misdisstr + leftSideCov;
				vector<CVector3D> vecEndNormal(2);
				CVector3D	endNormal;	//�˲��乳����
				if (GetvvecEndType().size() > 0)
				{
					for (size_t k = 0; k < vecEndNormal.size(); ++k)
					{
						double dRotateAngle = vecEndType.at(k).rotateAngle;
						CVector3D rebarVec = m_STwallData.ptEnd - m_STwallData.ptStart;
						endNormal = CVector3D::From(0, 0, -1);
						if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
						{
							endNormal.Negate();
						}
						endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//�Ըֽ��Ϊ����ת
						vecEndNormal[k] = endNormal;
					}
				}
				if (leftWall != Horizontal)
				{
					vecEndNormal.at(0) = tmpEndNormal.at(0);
				}
				if (rightWall != Horizontal)
				{
					vecEndNormal.at(1) = tmpEndNormal.at(1);
				}
				//if (leftWall == IN_WALL)
				//{
				//	vecEndType[0].endType = 4;
				//}
				//if (rightWall == IN_WALL)
				//{
				//	vecEndType[1].endType = 4;
				//}
				mat = rot90;
				mat.SetTranslation(vTrans[i]);
				mat = GetPlacement() * mat;
				matTb = rot90;
				matTb.SetTranslation(vTransTb[i]);
				matTb = GetPlacement() * matTb;
				//������Ϊ�����,ż����Ϊ��ͨ��

				//���Ʋ���--begin
				if (GetvecTwinRebarLevel().size() == GetRebarLevelNum() && GetvecTwinRebarLevel().at(i).hasTwinbars)	//���Ʋ���
				{
					SetIsPushTieRebar(true);
					//�Ȼ��Ʒǲ����
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dWidth, tLenth,
						GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat,
						GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i),
						GetvecDataExchange().at(i), false, modelRef, true, true, leftWall, rightWall,
						leftDis, rightDis);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);

						CalcLevelHoriInfos(i, tag, rightWall, m_levelHoriTags);
					}

					SetIsPushTieRebar(false);
					//���Ʋ����
					tag = MakeRebars(PopvecSetId().at(iTwinbarSetIdIndex + iRebarLevelNum), GetvecDirSize().at(i),
						dWidth, tLenth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, matTb,
						GetvecTwinRebarLevel().at(i), GetvecRebarLevel().at(i), GetvecRebarType().at(i),
						GetvecDataExchange().at(i), true, modelRef, true, true, leftWall, rightWall,
						leftDis, rightDis);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(iTwinbarSetIdIndex + iRebarLevelNum + 1 + numtag);
						rsetTags.Add(tag);
						CalcLevelHoriInfos(i, tag, rightWall, m_twinLevelHoriTags);
					}
					iTwinbarSetIdIndex++;

				}
				else //��ǰ��δ���ò���
				{
					SetIsPushTieRebar(true);
					TwinBarSet::TwinBarLevelInfo twinRebar = { "",0,"",0,0 };
					tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dWidth, tLenth,
						GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i)*uor_per_mm,
						GetvecEndOffset().at(i)*uor_per_mm, vecEndType, vecEndNormal, mat, twinRebar,
						GetvecRebarLevel().at(i), GetvecRebarType().at(i), GetvecDataExchange().at(i),
						false, modelRef, true, true, leftWall, rightWall, leftDis, rightDis);
					if (NULL != tag && (!PreviewButtonDown))
					{
						tag->SetBarSetTag(i + 1 + numtag);
						rsetTags.Add(tag);
						CalcLevelHoriInfos(i, tag, rightWall, m_levelHoriTags);
					}
				}
				//end
				vecEndType.clear();
				if (rightWall == Horizontal)
				{
					m_levelIsHori[i] = true;
				}
				else
				{
					m_levelIsHori[i] = false;
				}
			}
			if (m_vecRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = k;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecRebarPtsLayer.at(m);
					rbPt.ptend = m_vecRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecRebarPtsNoHole.push_back(rbPt);
					vector<int> walls = m_rangeIdxWalls[k];
					for (auto wallId : walls)
					{
						g_wallRebarPtsNoHole[wallId].push_back(rbPt);
					}
					m++;
				}
			}
			if (m_vecTwinRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecTwinRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = k;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTwinRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTwinRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTwinRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
			if (m_vecTieRebarPtsLayer.size() > 1)
			{
				for (int m = 0; m < m_vecTieRebarPtsLayer.size() - 1; m++)
				{
					int n = m + 1;
					RebarPoint rbPt;
					rbPt.Layer = GetvecRebarLevel().at(i);
					rbPt.iIndex = i;
					rbPt.sec = 0;
					rbPt.vecDir = GetvecDir().at(i);
					rbPt.ptstr = m_vecTieRebarPtsLayer.at(m);
					rbPt.ptend = m_vecTieRebarPtsLayer.at(n);
					rbPt.DataExchange = GetvecDataExchange().at(i);
					g_vecTieRebarPtsNoHole.push_back(rbPt);
					m++;
				}
			}
			SetSideCover(sidecover);
		}



		numtag = (int)rsetTags.GetSize();
	}

	UpdateHoriRebars(m_levelHoriTags, TEXT_MAIN_REBAR);
	UpdateHoriRebars(m_twinLevelHoriTags, TEXT_TWIN_REBAR);

	if (PreviewButtonDown)//Ԥ����ť���£���������
	{
		m_allLines.clear();
		for (auto it = m_vecRebarStartEnd.begin(); it != m_vecRebarStartEnd.end(); it++)
		{
			vector<vector<DPoint3d>> faceLinePts = *it;
			for (auto it : faceLinePts)
			{
				vector<DPoint3d> linePts = it;
				EditElementHandle eeh;
				LineStringHandler::CreateLineStringElement(eeh, nullptr, &linePts[0], linePts.size(), true, *ACTIVEMODEL);
				eeh.AddToModel();
				m_allLines.push_back(eeh.GetElementRef());
			}
		}
		return true;
	}

	bool ret = true;
	if (g_globalpara.Getrebarstyle() != 0)
	{
		ret = AddRebarSets(rsetTags);
	}
	return ret;
}

RebarSetTag* STGWallRebarAssembly::MakeRebars(ElementId& rebarSetId, BrStringCR sizeKey,
	double xLen, double height, double spacing, double startOffset, double endOffset,
	vector<PIT::EndType> const& endType, /*�洢���˲����յ�˲����� */
	vector<CVector3D> const& vecEndNormal, CMatrix3D const& mat,
	TwinBarSet::TwinBarLevelInfo const& twinBarInfo, int level, int grade,
	int DataExchange, bool bTwinbarLevel, DgnModelRefP modelRef,
	bool drawlast /*= true */,
	bool  isHoriRebar /*= true*/,
	WallPos leftWall/* = Horizontal*/,
	WallPos rightWall/* = Horizontal*/,
	double leftDis /*= 0*/,
	double rightDis /*= 0*/)
{
	rebarSetId = 0;
	//	m_IsTwinrebar = bTwinbarLevel;
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;

	if (endType.size() != vecEndNormal.size() || endType.size() == 0)
	{
		return NULL;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double startbendRadius, endbendRadius;
	double startbendLen, startbendLenTb, endbendLen, endbendLenTb;
	double begStraightAnchorLen, endStraightAnchorLen;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	double LaLenth = g_globalpara.m_alength[(string)sizeKey] * uor_per_mm;
	double L0Lenth = g_globalpara.m_laplenth[(string)sizeKey] * uor_per_mm;
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
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = 12 * diameter;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef) * uor_per_mm;	//������100
		}
		startbendLenTb = startbendLen;
	}
	break;
	case 5:	//135���乳
	{
		endTypeStart.SetType(RebarEndType::kCog);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		}
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}
	break;
	case 6:	//180���乳
	{
		endTypeStart.SetType(RebarEndType::kHook);
		startbendRadius = endType[0].endPtInfo.value1;
		if (COMPARE_VALUES(startbendRadius, 0) == 0)
		{
			startbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		startbendLen = endType[0].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(startbendLen, 0) == 0)
		{
			startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef);	//������100
		}
		startbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
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
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = 12 * diameter;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef) * uor_per_mm;	//������100
		}
		endbendLenTb = endbendLen;
	}
	break;
	case 5:	//135���乳
	{
		endTypeEnd.SetType(RebarEndType::kCog);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = rightDis;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		}
		endbendLenTb = endbendLen;
	}
	break;
	case 6:	//180���乳
	{
		endTypeEnd.SetType(RebarEndType::kHook);
		endbendRadius = endType[1].endPtInfo.value1;
		if (COMPARE_VALUES(endbendRadius, 0) == 0)
		{
			endbendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false);	//������30
		}
		endbendLen = endType[1].endPtInfo.value3;//Ԥ������
		if (COMPARE_VALUES(endbendLen, 0) == 0)
		{
			endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef);	//������100
		}
		endbendLenTb = RebarCode::GetBendLength(twinBarInfo.rebarSize, endTypeEnd, modelRef);	//������100
	}

	break;
	case 8:	//�û�
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	double diameterTb = RebarCode::GetBarDiameter(twinBarInfo.rebarSize, modelRef);
	double bendRadiusTb = RebarCode::GetPinRadius(twinBarInfo.rebarSize, modelRef, false);	//������30
	double adjustedXLen, adjustedSpacing;

	double leftSideCov = GetSideCover()*uor_per_mm / sin(m_angle_left);
	double rightSideCov = GetSideCover() *uor_per_mm / sin(m_angle_right);
	double allSideCov = leftSideCov + rightSideCov;
	int numRebar = 0;
	if (twinBarInfo.hasTwinbars)	//����
		adjustedXLen = xLen - allSideCov - diameter - diameterTb - startOffset - endOffset;
	else
		adjustedXLen = xLen - allSideCov - diameter - startOffset - endOffset;
	if (bTwinbarLevel)				//�����ֽ�����
	{
		numRebar = (int)floor(adjustedXLen / (spacing * (twinBarInfo.interval + 1)) + 0.5) + 1;
		int numRebar1 = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar1 > 1)
		{
			adjustedSpacing = adjustedXLen / (numRebar1 - 1);	//�ǲ����ƽ�����
			adjustedSpacing *= (twinBarInfo.interval + 1);		//�����ʵ�ʼ������Ըֽ���
		}
	}
	else
	{
		numRebar = (int)floor(adjustedXLen / spacing + 0.85) + 1;
		adjustedSpacing = spacing;
		if (numRebar > 1)
			adjustedSpacing = adjustedXLen / (numRebar - 1);
	}

	double xPos = startOffset;
	if (bTwinbarLevel)				//�������ƫ��һ�ξ���
	{
		xPos += diameter * 0.5;
		if (diameterTb > diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radiusTb - radius)*(radiusTb - radius), 0.5) - radius;
			xPos += dOffset;
		}
		else if (diameterTb < diameter)
		{
			double radius = diameter * 0.5;
			double radiusTb = diameterTb * 0.5;
			double dOffset = pow((radius + radiusTb)*(radius + radiusTb) - (radius - radiusTb)*(radius - radiusTb), 0.5) - radius;
			xPos += dOffset;
		}
		else
		{
			xPos += diameter * 0.5;
		}
	}
	vector<PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0)	//�˲�����ʱ����ƫ�Ƹֽ�뾶
		endTypEendOffset += diameter * 0.5;

	PITRebarEndType start, end;
	start.SetType((PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);
	start.SetstraightAnchorLen(begStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		start.SetbendLen(startbendLenTb);
		start.SetbendRadius(startbendRadius);
	}
	else
	{
		start.SetbendLen(startbendLen);
		start.SetbendRadius(startbendRadius);
	}
	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);
	end.SetstraightAnchorLen(endStraightAnchorLen);
	if (bTwinbarLevel)				//�����
	{
		end.SetbendLen(endbendLenTb);
		end.SetbendRadius(bendRadiusTb);
	}
	else
	{
		end.SetbendLen(endbendLen);
		end.SetbendRadius(endbendRadius);
	}
	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };
	RebarEndType tmpType;
	tmpType.SetType(RebarEndType::kBend);
	//double bendLen = RebarCode::GetBendLength(sizeKey, tmpType, modelRef);

	for (int i = 0; i < numRebar; i++)
	{
		if (!drawlast&&i == numRebar - 1)
		{
			continue;
		}
		vector<PITRebarCurve>     rebarCurves;
		//		RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
		makeRebarCurve(rebarCurves, xPos, height - allSideCov, endTypeStartOffset, endTypEendOffset,
			endTypes, mat, GWallRebarAssembly::GetIsTwinrebar(), leftWall, rightWall, leftDis, rightDis,
			L0Lenth, diameter);

		xPos += adjustedSpacing;

		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}

	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;
	if (twinBarInfo.hasTwinbars && bTwinbarLevel)
	{
		SetRebarColorBySize(twinBarInfo.rebarSize, symb);
		symb.SetRebarLevel(TEXT_TWIN_REBAR);
	}
	else
	{
		string str(sizeKey);
		char ccolar[20] = { 0 };
		strcpy(ccolar, str.c_str());
		SetRebarColorBySize(ccolar, symb);
		symb.SetRebarLevel(TEXT_MAIN_REBAR);
	}
	vector<vector<DPoint3d>> vecStartEnd;
	for (PITRebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (ISPointInHoles(m_Holeehs, midPos))
		{
			if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
			{
				continue;
			}
		}

		vector<DPoint3d> linePts;
		RebarVertices vertices = rebarCurve.GetVertices();
		for (size_t i = 0; i < (size_t)vertices.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &vertices.At(i);
			linePts.push_back(tmpVertex->GetIP());
		}
		/*	EditElementHandle eeh;
			LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
			eeh.AddToModel();*/

		RebarElementP rebarElement = NULL;;
		if (!PreviewButtonDown)
		{//������Ԥ��״̬�������ɸֽ�
			rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		}
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;
			if (bTwinbarLevel)
			{
				shape.SetSizeKey(CString(twinBarInfo.rebarSize));
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameterTb, endTypes, shape, modelRef, false);
			}
			else
			{
				shape.SetSizeKey((LPCTSTR)sizeKey);
				shape.SetIsStirrup(isStirrup);
				shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
				RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
				rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);
			}
			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);

			char tlevel[256];
			sprintf(tlevel, "%d", level);
			string slevel(tlevel);
			string Stype;
			if (DataExchange == 0)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinfront";
				else
					Stype = "front";
			}
			else if (DataExchange == 1)
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinmidden";
				else
					Stype = "midden";
			}
			else
			{
				if (twinBarInfo.hasTwinbars && bTwinbarLevel)
					Stype = "Twinback";
				else
					Stype = "back";
			}
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, slevel, grade, Stype, ACTIVEMODEL);
			SetRebarHideData(tmprebar, spacing / uor_per_mm, ACTIVEMODEL);
			tmprebar.ReplaceInModel(oldref);

		}
		j++;
		vecStartEnd.push_back(linePts);
	}
	m_vecRebarStartEnd.push_back(vecStartEnd);
	RebarSetData setdata;
	setdata.SetNumber(numRebar);
	setdata.SetNominalSpacing(spacing / uor_per_mm);
	setdata.SetAverageSpacing(adjustedSpacing / uor_per_mm);

	int ret = -1;
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//���ص��Ǹֽ�����

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

bool STGWallRebarAssembly::makeRebarCurve(vector<PIT::PITRebarCurve>& rebars, double xPos, double yLen,
	double startOffset, double endOffset, PIT::PITRebarEndTypes& endTypes, CMatrix3D const& mat,
	bool isTwin /*= false */, WallPos leftWalls/* = Horizontal*/, WallPos rightWall/* = Horizontal*/,
	double leftDis /*= 0*/, double rightDis /*= 0*/, double bendLen/* = 0*/, double  rebarDia/* = 0*/)
{
	CPoint3D  startPt;
	CPoint3D  endPt;

	//������Ϊ��ֵ
// 	if (startPt < 0)
// 		startPt = 0;
// 	if (endOffset < 0)
// 		endOffset = 0;

	startPt = CPoint3D::From(xPos, 0.0, -yLen / 2.0 + startOffset);
	endPt = CPoint3D::From(xPos, 0.0, yLen / 2.0 - endOffset);

	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	//eeh.AddToModel();

	DPoint3d pt1[2];
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//��ʱʹ�õ�ǰ����MODEL�������������޸�
	DPoint3d lineStrPt = pt1[0], lineEndPt = pt1[1];

	//ȷ������յ��Ǵ�С����---begin
	DVec3d vec = pt1[1] - pt1[0];
	DVec3d vecX = DVec3d::From(1, 0, 0);
	vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
	vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
	vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
	vec.Normalize();
	if (vec.IsPerpendicularTo(vecX))
	{
		if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
		if (leftWalls == OUT_WALL)
		{
			pt1[0].y -= bendLen + leftDis;
		}
		if (rightWall == OUT_WALL)
		{
			pt1[1].y += (bendLen + rightDis);
		}
	}
	else
	{
		if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) > 0)
		{
			DPoint3d ptTmp = pt1[0];
			pt1[0] = pt1[1];
			pt1[1] = ptTmp;
		}
		if (leftWalls == OUT_WALL)
		{
			pt1[0].x -= bendLen + leftDis;
		}
		if (rightWall == OUT_WALL)
		{
			pt1[1].x += bendLen + rightDis;
		}
	}
	//---end

	if (GetIntersectPointsWithNegs(m_Negs, pt1[0], pt1[1]))
	{
		return false;
	}
	m_vecRebarPtsLayer.push_back(pt1[0]);
	m_vecRebarPtsLayer.push_back(pt1[1]);
	if (isTwin)
	{
		m_vecTwinRebarPtsLayer.push_back(pt1[0]);
		m_vecTwinRebarPtsLayer.push_back(pt1[1]);
	}

	//if (m_isPushTieRebar)
	//{
	//	m_vecTieRebarPtsLayer.push_back(pt1[0]);
	//	m_vecTieRebarPtsLayer.push_back(pt1[1]);
	//}
	map<int, DPoint3d> map_pts = CalcRebarPts(pt1[0], pt1[1]);

	for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	{
		PITRebarCurve rebar;
		RebarVertexP vex;
		vex = &rebar.PopVertices().NewElement();
		DPoint3d startPt = itr->second;
		vex->SetIP(startPt);
		vex->SetType(RebarVertex::kStart);
		endTypes.beg.SetptOrgin(startPt);

		map<int, DPoint3d>::iterator itrplus = ++itr;
		if (itrplus == map_pts.end())
		{
			break;
		}
		DPoint3d endPt = itrplus->second;
		endTypes.end.SetptOrgin(endPt);

		vex = &rebar.PopVertices().NewElement();
		vex->SetIP(endPt);
		vex->SetType(RebarVertex::kEnd);

		PIT::PITRebarEndTypes tmpEndTypes = endTypes;
		if (!(startPt == lineStrPt) && leftWalls == IN_WALL)
		{
			tmpEndTypes.beg.SetbendLen(0);
			tmpEndTypes.beg.SetType(PITRebarEndType::kNone);
			tmpEndTypes.beg.SetbendRadius(0);
		}
		if (!(endPt == lineEndPt) && rightWall == IN_WALL)
		{
			tmpEndTypes.end.SetbendLen(0);
			tmpEndTypes.end.SetType(PITRebarEndType::kNone);
			tmpEndTypes.end.SetbendRadius(0);
		}
		rebar.EvaluateEndTypes(tmpEndTypes);
		rebars.push_back(rebar);
	}
	//rebar.DoMatrix(mat);
	return true;
}

void STGWallRebarAssembly::InitLevelHoriInfos()
{
	int levelNum = GetRebarLevelNum();
	m_levelIsHori.clear();
	for (int i = 0; i < levelNum; ++i)
	{
		m_levelIsHori[i] = false;
	}
	m_levelHoriTags.clear();
	m_twinLevelHoriTags.clear();
}

void STGWallRebarAssembly::CalcLevelHoriInfos(int level, RebarSetTag* tag, WallPos rightWall,
	map<int, vector<vector<RebarSetTag*>>>& levelHoriTags)
{
	vector<vector<RebarSetTag*>> levelTags = levelHoriTags[level];;
	if (!m_levelIsHori[level])
	{
		vector<RebarSetTag*> tags;
		tags.push_back(tag);
		levelTags.push_back(tags);
	}
	else
	{
		size_t num = levelTags.size() - 1;
		levelTags[num].push_back(tag);
	}
	levelHoriTags[level] = levelTags;
}

void STGWallRebarAssembly::UpdateHoriRebars(const map<int, vector<vector<RebarSetTag*>>>& levelHoriTags,
	const CString& levelName)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	for (auto it : levelHoriTags)
	{
		vector<vector<RebarSetTag*>> levelTags = it.second;
		for (auto itr : levelTags)
		{
			if (itr.size() < 2)
			{
				continue;
			}
			map<int, vector<ElementRefP>> zRebars;
			for (auto tagIt : itr)
			{
				RebarSetP rebarSet = tagIt->GetRset();
				int nNum = (int)rebarSet->GetChildElementCount(); // �ֽ����иֽ�����
				if (nNum == 0)
				{
					continue;
				}
				for (int j = 0; j < nNum; j++)
				{
					RebarElementP pRebar = rebarSet->GetChildElement(j);
					ElementId rebarElementId = pRebar->GetRebarElementId();

					EditElementHandle rebarEle(rebarElementId, ACTIVEMODEL);
					DPoint3d center = getCenterOfElmdescr(rebarEle.GetElementDescrP());
					int posz = (int)(center.z / (uor_per_mm * 10));
					zRebars[posz].push_back(rebarEle.GetElementRef());
				}
			}
			UpdateRebars(zRebars, levelName);
		}

	}

}

void STGWallRebarAssembly::UpdateRebars(const map<int, vector<ElementRefP>>& zRebars, const CString& levelName)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	auto GetBendPt = [&](EditElementHandleP rebarEeh, DPoint3d& bendStrPt, DPoint3d& bendEndPt, DPoint3d& bendPt) {
		RebarElementP rep = RebarElement::Fetch(*rebarEeh);
		RebarShape * rebarshape = rep->GetRebarShape(rebarEeh->GetModelRef());
		RebarCurve strcurve;//��ȡ�ֽ�ģ���е�������״
		rebarshape->GetRebarCurve(strcurve);
		CMatrix3D tmp3d(rep->GetLocation());
		strcurve.MatchRebarCurve(tmp3d, strcurve, uor_per_mm);
		strcurve.DoMatrix(rep->GetLocation());
		RebarVertices  vers;
		RebarVertices oldVers = strcurve.GetVertices();
		for (size_t i = 0; i < (size_t)oldVers.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &oldVers.At(i);
			if (tmpVertex->GetType() == RebarVertex::kStart)
			{
				bendStrPt = tmpVertex->GetIP();
			}
			if (tmpVertex->GetType() == RebarVertex::kEnd)
			{
				bendEndPt = tmpVertex->GetIP();
			}
			if (tmpVertex->GetType() == RebarVertex::kIP)
			{
				bendPt = tmpVertex->GetIP();
			}
		}
	};

	//if (levelName == TEXT_MAIN_REBAR)
	//{
	//	m_vecRebarStartEnd.clear();
	//}

	for (auto it : zRebars)
	{
		vector<ElementRefP> rebars = it.second;
		size_t num = rebars.size();
		EditElementHandle strRebarEeh(rebars[0], rebars[0]->GetDgnModelP());
		EditElementHandle endRebarEeh(rebars[num - 1], rebars[num - 1]->GetDgnModelP());
		if (RebarElement::IsRebarElement(strRebarEeh) && RebarElement::IsRebarElement(endRebarEeh))
		{
			//��β�ֽ��Ƿ����乳�������乳ʱ��λ��
			bool strIsBend = false, endIsBend = false;
			DPoint3d bendStrPt = { 0,0,0 }, bendEndPt = { 0,0,0 }; //��β��������µ���ʼ����յ�
			RebarElementP strRep = RebarElement::Fetch(strRebarEeh);
			DPoint3d firstBendStrPt = { 0,0,0 }, firstBendEndPt = { 0,0,0 }, firstBendPt = { 0,0,0 }; //�׸��ֽ����ʼ�㣬��ֹ���������
			GetBendPt(&strRebarEeh, firstBendStrPt, firstBendEndPt, firstBendPt);
			DPoint3d strPt = firstBendStrPt; //�߽�����
			RebarShape * strShape = strRep->GetRebarShape(strRebarEeh.GetModelRef());
			if ((!firstBendPt.IsEqual(DPoint3d::FromZero())) &&
				(strShape->GetEndType(0).GetType() != RebarEndType::kNone
					|| strShape->GetEndType(1).GetType() != RebarEndType::kNone))
			{
				strIsBend = true;
				strPt = firstBendPt;
				bendStrPt = firstBendStrPt;
			}
			RebarElementP endRep = RebarElement::Fetch(endRebarEeh);
			DPoint3d endBendStrPt = { 0,0,0 }, endBendEndPt = { 0,0,0 }, endBendPt = { 0,0,0 }; //ĩβ���ֽ����ʼ�㣬��ֹ���������
			GetBendPt(&endRebarEeh, endBendStrPt, endBendEndPt, endBendPt);
			DPoint3d endPt = endBendEndPt; //�߽���յ�
			RebarShape * endShape = endRep->GetRebarShape(endRebarEeh.GetModelRef());
			if ((!endBendPt.IsEqual(DPoint3d::FromZero())) &&
				(endShape->GetEndType(0).GetType() != RebarEndType::kNone ||
					endShape->GetEndType(1).GetType() != RebarEndType::kNone))
			{
				endIsBend = true;
				endPt = endBendPt;
				bendEndPt = endBendEndPt;
			}

			//�߽�Ķ˵�
			DVec3d vec = strPt - endPt;
			if (vec.IsPerpendicularTo(DVec3d::UnitX())) //��x����ֱ
			{
				if (COMPARE_VALUES_EPS(strPt.y, endPt.y, 10) == 1)
				{
					DPoint3d tmpPt = strPt;
					strPt = endPt;
					endPt = tmpPt;

					tmpPt = bendStrPt;
					bendStrPt = bendEndPt;
					bendEndPt = tmpPt;
				}
			}
			else
			{
				if (COMPARE_VALUES_EPS(strPt.x, endPt.x, 10) == 1)
				{
					DPoint3d tmpPt = strPt;
					strPt = endPt;
					endPt = tmpPt;

					tmpPt = bendStrPt;
					bendStrPt = bendEndPt;
					bendEndPt = tmpPt;
				}
			}

			//��ܿ׶������		
			map<int, DPoint3d> map_pts = CalcRebarPts(strPt, endPt); //��ܺ�ĵ�

			//vector<vector<DPoint3d>> vecStartEnd;
			int index = 0;
			auto endIt = map_pts.end(); --endIt;
			for (auto itr = map_pts.begin(); itr != map_pts.end(); itr++)
			{
				if (index >= rebars.size())
				{
					break;
				}
				//��Ҫ�޸ĵĸֽ��Ϊ�ϲ���ĸֽ�����<=�ϲ�ǰ�ģ����޸ĵķ�ʽ�����øֽ����Բ���
				EditElementHandle rebarEeh(rebars[index], ACTIVEMODEL);
				RebarElementP rep = RebarElement::Fetch(rebarEeh);
				RebarShape * rebarshape = rep->GetRebarShape(rebarEeh.GetModelRef());
				if (rebarshape == nullptr)
				{
					return;
				}
				BrString sizeKey(rebarshape->GetShapeData().GetSizeKey());
				double diameter = RebarCode::GetBarDiameter(sizeKey, ACTIVEMODEL);

				RebarEndType endType1, endType2;
				endType1.SetType(RebarEndType::kNone);
				endType2.SetType(RebarEndType::kNone);
				bvector<DPoint3d> allpts;
				if (strIsBend && itr == map_pts.begin())
				{
					allpts.push_back(bendStrPt);
					endType1.SetType(RebarEndType::kBend);
				}
				allpts.push_back(itr->second);
				auto nextItr = ++itr;
				allpts.push_back(nextItr->second);
				if (endIsBend && itr == endIt)
				{
					allpts.push_back(bendEndPt);
					endType2.SetType(RebarEndType::kBend);
				}
				RebarVertices  vers;
				// GetRebarVerticesFromPoints(vers, allpts, diameter);
				RebarHelper::GetRebarVerticesFromPoints(vers, allpts, diameter);

				//��ȡ�ֽ�ģ���е�������״
				RebarCurve curve;
				rebarshape->GetRebarCurve(curve);
				curve.SetVertices(vers);

				RebarEndTypes endTypes = { endType1, endType2 };

				//���õ�����ɫ��ͼ��
				RebarSymbology symb;
				string str(sizeKey);
				char ccolar[20] = { 0 };
				strcpy(ccolar, str.c_str());
				SetRebarColorBySize(ccolar, symb);
				symb.SetRebarLevel(levelName);//�����ǵ��������Ϊ����ͼ��
				rep->SetRebarElementSymbology(symb);

				RebarShapeData shape = rebarshape->GetShapeData();
				rep->Update(curve, diameter, endTypes, shape, rep->GetModelRef(), false);
				RebarModel *rmv = RMV;
				if (rmv != nullptr)
				{
					rmv->SaveRebar(*rep, rep->GetModelRef(), true);
				}

				//if (levelName == TEXT_MAIN_REBAR)
				//{
				//	
				//	vector<DPoint3d> linePts;
				//	RebarVertices vertices = curve.GetVertices();
				//	for (size_t i = 0; i < vertices.GetSize(); ++i)
				//	{
				//		RebarVertex *tmpVertex = &vertices.At(i);
				//		linePts.push_back(tmpVertex->GetIP());
				//	}
				//	vecStartEnd.push_back(linePts);
				//}
				index++;
			}
			//ɾ������ֽ�
			for (size_t i = index; i < rebars.size(); ++i)
			{
				ElementHandle rebarEeh(rebars[i], rebars[i]->GetDgnModelP());
				RebarElementP rep = RebarElement::Fetch(rebarEeh);
				if (rep == nullptr)
				{
					return;
				}
				RebarModel *rmv = RMV;
				if (rmv != nullptr)
				{
					rmv->Delete(*rep, ACTIVEMODEL);
				}
			}
		}
	}
}

map<int, DPoint3d> STGWallRebarAssembly::CalcRebarPts(DPoint3d& strPt, DPoint3d& endPt)
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double dSideCover = m_sidecover * uor_per_mm;
	vector<DPoint3d> tmppts;
	Transform matrix;
	GetPlacement().AssignTo(matrix);
	if (g_wallRebarInfo.concrete.isHandleHole)
	{
		GetIntersectPointsWithHoles(tmppts, m_useHoleehs, strPt, endPt, dSideCover, matrix);
	}

	map<int, DPoint3d> map_pts;
	bool isStr = false;
	for (DPoint3d pt : tmppts)
	{
		if (ExtractFacesTool::IsPointInLine(pt, strPt, endPt, ACTIVEMODEL, isStr))
		{
			int dis = (int)strPt.Distance(pt);
			if (map_pts.find(dis) != map_pts.end())
			{
				dis = dis + 1;
			}
			map_pts[dis] = pt;
		}
	}
	if (map_pts.find(0) != map_pts.end())
	{
		map_pts[1] = strPt;
	}
	else
	{
		map_pts[0] = strPt;
	}
	int dis = (int)strPt.Distance(endPt);
	if (map_pts.find(dis) == map_pts.end())
	{
		map_pts[dis] = endPt;
	}
	else
	{
		dis = dis + 1;
		map_pts[dis] = endPt;
	}
	return map_pts;
}


bool __IsSmartSmartFeature__(EditElementHandle& eeh)
{
	SmartFeatureNodePtr pFeatureNode;
	if (SmartFeatureElement::IsSmartFeature(eeh))
	{
		return true;
	}
	else
	{
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			if (SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef()) == SUCCESS)
			{
				//eeh.AddToModel();
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}

		}
		return true;
	}
}

void STGWallRebarAssembly::CalcWallsInRange()
{
	g_wallRebarPtsNoHole.clear();
	ElementAgenda selectedElement;
	SelectionSetManager::GetManager().BuildAgenda(selectedElement);
	if (selectedElement.GetCount() < 2)
	{
		return;
	}
	vector<EditElementHandle*> allWalls;
	map<ElementId, EditElementHandle*> tmpselcets;
	for (EditElementHandleR eeh : selectedElement)
	{
		if (eeh.IsValid())
		{
			ElementId tmpid = eeh.GetElementId();
			if (tmpid != 0)
			{
				tmpselcets[tmpid] = &eeh;
			}
		}
	}
	for (map<ElementId, EditElementHandle*>::iterator itr = tmpselcets.begin(); itr != tmpselcets.end(); itr++)
	{
		if (itr->second == nullptr)
		{
			continue;
		}
		// if (HoleRFRebarAssembly::IsSmartSmartFeature(*itr->second))
		if (__IsSmartSmartFeature__(*itr->second))
		{
			allWalls.push_back(itr->second);
		}
	}

	map<int, vector<DPoint3d>> linePts = GetLinePts();
	if (allWalls.size() == 0 || linePts.size() == 0)
	{
		return;
	}
	map<int, MSElementDescrP> wallDownfaces; //ǽ����
	for (auto it : allWalls)
	{
		if (!it->IsValid())
		{
			continue;
		}
		int id = (int)(it->GetElementId());
		EditElementHandle downFace;
		double height = 0;
		ExtractFacesTool::GetDownFace(*it, downFace, &height);
		wallDownfaces[id] = downFace.ExtractElementDescr();
	}
	for (auto it : linePts)
	{
		vector<DPoint3d> pts = it.second;
		if (pts.size() != 4)
		{
			continue;
		}
		//��С��Χ
		DVec3d vec1 = pts[0] - pts[1];
		DVec3d vec2 = pts[2] - pts[3];
		vec1.Normalize(); vec1.Scale(10);
		pts[1].Add(vec1);
		vec1.Negate();
		pts[0].Add(vec1);
		vec2.Normalize(); vec2.Scale(10);
		pts[3].Add(vec2);
		vec2.Negate();
		pts[2].Add(vec2);
		DPoint3d shapePts[4] = { pts[0], pts[1], pts[2], pts[3] };
		EditElementHandle shapeEeh;
		ShapeHandler::CreateShapeElement(shapeEeh, nullptr, shapePts, 4, true, *ACTIVEMODEL);
		MSElementDescrP shapeDescr = shapeEeh.GetElementDescrP();
		for (auto wallIt : wallDownfaces)
		{
			DPoint3d interPt;
			int num = mdlIntersect_allBetweenElms(&interPt, nullptr, 1, wallIt.second, shapeDescr, nullptr, 1);
			if (num > 0)
			{
				m_rangeIdxWalls[it.first].push_back(wallIt.first);
			}
		}
		mdlElmdscr_freeAll(&shapeDescr);
	}

}


bool STGWallRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pSTGWallDoubleRebarDlg = new CWallRebarDlg(ehSel, CWnd::FromHandle(MSWIND));
	pSTGWallDoubleRebarDlg->SetSelectElement(ehSel);
	pSTGWallDoubleRebarDlg->Create(IDD_DIALOG_WallRebar);
	pSTGWallDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pSTGWallDoubleRebarDlg->ShowWindow(SW_SHOW);
	return true;
}

bool STGWallRebarAssembly::GetFrontBackLines(EditElementHandleCR eeh, vector<MSElementDescrP>& frontLines, vector<MSElementDescrP>& backLines)
{
	vector<MSElementDescrP> vecDownFaceLine;
	double height;
	ExtractFacesTool::GetFrontBackLineAndDownFace(eeh, NULL, vecDownFaceLine, frontLines, backLines, &height);
	SetGWallHeight(height);
	if (vecDownFaceLine.empty() || frontLines.empty() || backLines.empty())
		return false;

	auto CalculateLines = [](vector<MSElementDescrP>& downLines) {
		int index = 0;
		for (auto it = downLines.begin() + 1; it != downLines.end();)
		{
			auto lastIt = it; lastIt--;
			DPoint3d lastStartPt, lastEndPt;
			mdlElmdscr_extractEndPoints(&lastStartPt, nullptr, &lastEndPt, nullptr, *lastIt, ACTIVEMODEL);
			DVec3d lastVec = lastEndPt - lastStartPt;
			lastVec.Normalize();
			DPoint3d startPt, endPt;
			mdlElmdscr_extractEndPoints(&startPt, nullptr, &endPt, nullptr, *it, ACTIVEMODEL);
			DVec3d curVec = endPt - startPt;
			curVec.Normalize();
			if (fabs(fabs(lastVec.DotProduct(curVec)) - 1) < 1e-6) //ƽ��
			{
				DPoint3d intersectPt;
				if (mdlIntersect_allBetweenElms(&intersectPt, nullptr, 1, *lastIt, *it, nullptr, 1) > 0)
				{
					downLines.erase(lastIt);
					DSegment3d seg = DSegment3d::From(lastStartPt, endPt);
					EditElementHandle eeh;
					LineHandler::CreateLineElement(eeh, nullptr, seg, true, *ACTIVEMODEL);
					downLines[index] = eeh.GetElementDescrP();
					continue;
				}
			}
			index++;
			it++;
		}
	};

	//CalculateLines(frontLines);
	//CalculateLines(backLines);

	auto FilterLines = [](vector<MSElementDescrP>& lines) {
		DPoint3d firstStrPt, firstEndPt;
		mdlElmdscr_extractEndPoints(&firstStrPt, nullptr, &firstEndPt, nullptr, *lines.begin(), ACTIVEMODEL);
		DVec3d firstVec = firstEndPt - firstStrPt;
		firstVec.Normalize();
		for (auto it = lines.begin(); it != lines.end();)
		{
			DPoint3d strPt, endPt;
			mdlElmdscr_extractEndPoints(&strPt, nullptr, &endPt, nullptr, *it, ACTIVEMODEL);
			DVec3d vec = endPt - strPt;
			vec.Normalize();
			if (fabs(fabs(vec.DotProduct(firstVec)) - 1) < 1e-6) //ƽ��
			{
				it++;
			}
			else
			{
				lines.erase(it);
			}
		}
	};

	FilterLines(frontLines);
	FilterLines(backLines);
	return true;
}

long STGWallRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}
