#include "_ustation.h"
#include "resource.h"
#include <SelectionRebar.h>
#include "RainshedREbar.h"
#include "WallRebarDlg.h"
#include "ExtractFacesTool.h"
#include "RainshedRebarDlg.h"
#include "SelectRebarTool.h"
#include "ElementAttribute.h"
#include "PITRebarEndType.h"
#include "PITMSCECommon.h"
#include "BentlyCommonfile.h"
#include "XmlHelper.h"

using namespace PIT;

int direction2;
RainshedRebarAssembly::RainshedRebarAssembly(ElementId id, DgnModelRefP modelRef) :     // 构造函数初始化一些值
	RebarAssembly(id, modelRef),
	m_PositiveCover(0),                          //正面保护层
	m_ReverseCover(0),                           //反面保护层
	m_SideCover(0),                               //反面保护层
	m_RebarLevelNum(4)                            //钢筋层数
{
	Init();
}
void RainshedRebarAssembly::Init()            //重新指定容器的长度为m_RebarLevelNum
{
	m_vecDir.resize(m_RebarLevelNum);            //方向,0表示x轴，1表示z轴
	m_vecDirSize.resize(m_RebarLevelNum);         //尺寸
	m_vecDirSpacing.resize(m_RebarLevelNum);       //间隔
	m_vecRebarType.resize(m_RebarLevelNum);        //钢筋类型
	m_vecStartOffset.resize(m_RebarLevelNum);       //起点偏移
	m_vecEndOffset.resize(m_RebarLevelNum);         //终点偏移
	m_vecLevelSpace.resize(m_RebarLevelNum);       //与前层间距
	m_vecSetId.resize(m_RebarLevelNum);            //SetId
	int twinRebarLevel = 0;

	m_vecSetId.resize(m_RebarLevelNum + twinRebarLevel);
	for (size_t i = 0; i < m_vecSetId.size(); i++)
	{
		m_vecSetId[i] = 0;
	}
}

bool RainshedRebarAssembly::IsSmartSmartFeature(EditElementHandle& eeh)
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
				eeh.AddToModel();
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
				return false;
			}

		}
		return true;
	}
}



void RainshedRebarAssembly::SetConcreteData(PIT::Concrete const& concreteData)    //设置三个保护层信息和层数
{
	m_PositiveCover = concreteData.postiveCover;
	m_ReverseCover = concreteData.reverseCover;
	m_SideCover = concreteData.sideCover;
	m_RebarLevelNum = concreteData.rebarLevelNum;
}

void RainshedRebarAssembly::SetRebarData(vector<PIT::ConcreteRebar> const& vecData)     //用vector数组存每层钢筋的信息
{
	if (vecData.empty())
	{
		return;
	}
	if (vecData.size() != m_RebarLevelNum)
	{
		return;
	}

	for (size_t i = 0; i < vecData.size(); i++)
	{
		if (i < m_vecDir.size())
		{
			m_vecDir[i] = vecData[i].rebarDir;                       //方向
			m_vecDirSize[i] = vecData[i].rebarSize;                   //钢筋尺寸
			m_vecRebarType[i] = vecData[i].rebarType;                 //钢筋型号
			m_vecDirSpacing[i] = vecData[i].spacing;                //钢筋间距
			m_vecStartOffset[i] = vecData[i].startOffset;           //起点偏移
			m_vecEndOffset[i] = vecData[i].endOffset;              //终点偏移
			m_vecLevelSpace[i] = vecData[i].levelSpace;           //钢筋层间隔
		}
		else                                                    //减少
		{
			m_vecDir.push_back(vecData[i].rebarDir);
			m_vecDirSize.push_back(vecData[i].rebarSize);
			m_vecRebarType.push_back(vecData[i].rebarType);
			m_vecDirSpacing.push_back(vecData[i].spacing);
			m_vecStartOffset.push_back(vecData[i].startOffset);
			m_vecEndOffset.push_back(vecData[i].endOffset);
			m_vecLevelSpace.push_back(vecData[i].levelSpace);
			m_vecSetId.push_back(0);
		}
	}

}

bool STRainshedRebarAssembly::makeRebarCurve
(
	vector<PIT::PITRebarCurve>&     rebars,
	double                  xPos,
	double                  yLen,
	double                  zhig,
	double					startOffset,
	double					endOffset,
	PIT::PITRebarEndTypes&	endTypes,
	CMatrix3D const&        mat,
	bool&					tag
)
{
	CPoint3D  startPt;
	CPoint3D  endPt;

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	if (4 == direction2) // 第五层钢筋
	{
		startPt = CPoint3D::From(0, yLen / 2.0, xPos);
		endPt = CPoint3D::From(0, -yLen / 2.0, xPos);
	}
	else
	{
		if (direction2 < 2) // 第3层钢筋
		{
			startPt = CPoint3D::From(xPos, -yLen / 2.0, 0.0);
			endPt = CPoint3D::From(xPos, yLen / 2.0, 0.0);
		}
		else
		{
			if (0 == GetvecDir().at(direction2))
			{
				startPt = CPoint3D::From(0.0, -yLen / 2.0, 0.0);
				endPt = CPoint3D::From(0.0, yLen / 2.0, 0.0);
			}
			else
			{
				startPt = CPoint3D::From(xPos, -yLen / 2.0, 0.0);
				endPt = CPoint3D::From(xPos, yLen / 2.0, 0.0);
			}
		}
	}
	// 矩阵变换
	Transform trans;
	mat.AssignTo(trans);
	TransformInfo transinfo(trans);
	EditElementHandle eeh;
	LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(startPt, endPt), true, *ACTIVEMODEL);
	eeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(eeh, transinfo);
	//eeh.AddToModel();

	DPoint3d pt1[2];
	mdlLinear_extract(pt1, NULL, eeh.GetElementP(), ACTIVEMODEL);//暂时使用当前激活MODEL，如有问题再修改
	if (direction2 > 1 && direction2 < 4)  // 第2层到第5层钢筋单独处理
	{	
		if (0 == GetvecDir().at(direction2))
		{
			DVec3d vec, vecTmp;
			mdlVec_subtractPoint(&vec, &m_RainshedData.ptStart_H, &m_RainshedData.ptEnd_H);
			vec.Normalize();

			mdlVec_subtractPoint(&vecTmp, &m_RainshedData.ptStart_L, &m_RainshedData.ptEnd_L);
			vecTmp.Normalize();

			double dot = mdlVec_dotProduct(&vec, &vecTmp);

			double moveLen = xPos / dot;

			movePoint(vec, pt1[0], moveLen, false);
			movePoint(vec, pt1[1], moveLen, false);
		}
		else
		{
			pt1[1] = pt1[0];

			CVector3D vec = m_RainshedData.ptEnd_H - m_RainshedData.ptStart_H;
			vec.Normalize();

			vec.ScaleToLength(yLen);
			pt1[1].Add(vec);
		}
	}

	m_vecRebarPtsLayer.push_back(pt1[0]);
	m_vecRebarPtsLayer.push_back(pt1[1]);

	double dSideCover = GetSideCover()*uor_per_mm;
	double dPositiveCover = GetPositiveCover() * uor_per_mm;
	double dReverseCover = GetReverseCover() * uor_per_mm;
	vector<DPoint3d> tmppts;
	vector<DPoint3d> tmpptsTmp;
	Transform matrix;
	GetPlacement().AssignTo(matrix);

	// 规避孔洞
	GetIntersectPointsWithHoles(tmppts, m_useHoleehs, pt1[0], pt1[1], dSideCover, matrix);

	map<int, DPoint3d> map_pts;
	for (DPoint3d pt : tmppts)
	{
		int dis = (int)pt1[0].Distance(pt);
		if (dis == 0 || dis == (int)pt1[0].Distance(pt1[1]))//防止过滤掉起点和终点
		{
			dis = dis + 1;
		}

		map_pts[dis] = pt;
	}
	map_pts[0] = pt1[0];
	map_pts[(int)pt1[0].Distance(pt1[1])] = pt1[1];

	RebarVertices  vers;
	bvector<DPoint3d> allpts;


	if (1 == GetvecDir().at(direction2))
	{

		DPoint3d temp ;
		CVector3D  tempVec(pt1[1], pt1[0]);
		CVector3D  arcVec = tempVec;
		arcVec.Normalize();
		arcVec.ScaleToLength( m_diameter1/2);
		(pt1[1]).Add(arcVec);
		temp = pt1[1];
		double dPositiveCover = GetPositiveCover()*uor_per_mm;//下
		double dReverseCover = GetReverseCover()*uor_per_mm;//上
		
		if (direction2 < 2)
		{
			CVector3D  tempVec(pt1[1], pt1[0]);
			CVector3D  arcVec = tempVec;
			arcVec.Normalize();
			arcVec.ScaleToLength(2 * m_diameter1);
			(pt1[1]).Add(arcVec);
			temp = pt1[1];

			if (0 == m_RainshedData.moreheight)//moreheight 不为0自由端凸出的
			{
				temp.z += (m_RainshedData.minheight - dReverseCover - dPositiveCover - m_diameter1 * 4);// 弯曲长度=减去钢筋直接和上面保护层
				allpts.push_back(pt1[0]);
				allpts.push_back(pt1[1]);
				allpts.push_back(temp);
			}
			else
			{
				// temp.z += (m_RainshedData.maxheight -dReverseCover - dPositiveCover - m_diameter1 * 2);
				temp.z += (m_STRainshedData.height - dReverseCover - dPositiveCover - m_diameter1);
				allpts.push_back(pt1[0]);
				allpts.push_back(pt1[1]);
				allpts.push_back(temp);
//				arcVec.ScaleToLength(m_diameter1 *2);
//				temp.Add(arcVec);
////				
//				allpts.push_back(temp);
//				temp.z -= (m_diameter1 * 2);
//				allpts.push_back(temp);
			}
			
		}
		else
		{
			allpts.push_back(pt1[0]);
			allpts.push_back(pt1[1]);
	
			if (0 == m_RainshedData.moreheight)//moreheight 不为0自由端凸出的
			{
				temp.z -= (m_RainshedData.minheight - dPositiveCover - dReverseCover - m_diameter1 * 3);
				allpts.push_back(temp);
			}
			else
			{
				temp.z -= (m_RainshedData.minheight - dPositiveCover - dReverseCover - m_diameter1 * 3);
				allpts.push_back(temp);
				//temp.z += (m_diameter1 *2);
				//allpts.push_back(temp);
				//arcVec.ScaleToLength(m_diameter1 * 2);
				//temp.Add(arcVec);
				//allpts.push_back(temp);
			}
		}
	}
	else
	{
		if (0 == m_RainshedData.moreheight)
		{
			allpts.push_back(pt1[0]);
			allpts.push_back(pt1[1]);
		}
		else
		{
			if (direction2 == 1)
			{
				DPoint3d ptTmp = pt1[0];
				CVector3D vecNormal = m_RainshedData.ptEnd_L - m_RainshedData.ptStart_L;
				vecNormal.Normalize();

				BrString sizeKey = GetvecDirSize().at(direction2 - 1);

				double diameter = RebarCode::GetBarDiameter(sizeKey, ACTIVEMODEL);

				// movePoint(vecNormal, ptTmp, (m_RainshedData.width_down - m_RainshedData.width) * 0.5);

				DPoint3d ptTTmp = ptTmp;
				CVector3D vec = CVector3D::From(0, 0, 1);
				// movePoint(vec, ptTTmp, m_RainshedData.moreheight - dPositiveCover - dReverseCover - diameter - m_diameter1 * 0.5);

				double dLength = m_STRainshedData.height - dPositiveCover - dReverseCover - diameter - m_diameter1;
				movePoint(vec, ptTTmp, dLength);

				allpts.push_back(ptTTmp);
				allpts.push_back(ptTmp);
				allpts.push_back(pt1[1]);

				ptTmp = pt1[1];

				// movePoint(vecNormal, ptTmp, (m_RainshedData.width_down - m_RainshedData.width) * 0.5, false);
				// movePoint(vec, ptTTmp, m_RainshedData.moreheight - dPositiveCover - dReverseCover - diameter - m_diameter1 * 0.5);
				movePoint(vec, ptTmp, dLength);
				allpts.push_back(ptTmp);
			}
			else if (direction2 == 4)
			{
				DPoint3d ptTmp = pt1[0];
				CVector3D vecNormal = m_RainshedData.ptStart_L - m_RainshedData.ptEnd_L;
				vecNormal.Normalize();

				WString strSizeTmp = GetvecDirSize().at(1);
				if (strSizeTmp.find(L"mm") != WString::npos)
				{
					strSizeTmp.ReplaceAll(L"mm", L"");
				}

				WString strSizeTTmp = GetvecDirSize().at(0);
				if (strSizeTTmp.find(L"mm") != WString::npos)
				{
					strSizeTTmp.ReplaceAll(L"mm", L"");
				}

				WString strSizeTTmp1 = GetvecDirSize().at(3);
				if (strSizeTTmp1.find(L"mm") != WString::npos)
				{
					strSizeTTmp1.ReplaceAll(L"mm", L"");
				}

				WString strSizeTTmp2 = GetvecDirSize().at(2);
				if (strSizeTTmp2.find(L"mm") != WString::npos)
				{
					strSizeTTmp2.ReplaceAll(L"mm", L"");
				}

				double dDiameter = RebarCode::GetBarDiameter(strSizeTmp, ACTIVEMODEL) + RebarCode::GetBarDiameter(strSizeTTmp, ACTIVEMODEL) + RebarCode::GetBarDiameter(strSizeTTmp1, ACTIVEMODEL)
					+ +RebarCode::GetBarDiameter(strSizeTTmp2, ACTIVEMODEL);


				movePoint(vecNormal, ptTmp, m_RainshedData.length_down - dSideCover * 2 - m_diameter1 - dDiameter);

				allpts.push_back(ptTmp);
				allpts.push_back(pt1[0]);

				ptTmp = pt1[1];

				movePoint(vecNormal, ptTmp, m_RainshedData.length_down - dSideCover * 2 - m_diameter1 - dDiameter);

				allpts.push_back(pt1[1]);
				allpts.push_back(ptTmp);
			}
			else
			{
				//DPoint3d temp;
				//temp = pt1[0];
				allpts.push_back(pt1[0]);
				allpts.push_back(pt1[1]);
			}
		}

	}

	if (allpts.size() > 1)
	{
		endTypes.beg.SetptOrgin(allpts.at(0));
		endTypes.end.SetptOrgin(allpts.at(allpts.size() - 1));
	}

	// 传入点数据，得到钢筋顶点
	GetRebarVerticesFromPoints(vers, allpts, m_diameter1);
	PIT::PITRebarCurve rebar;
	rebar.SetVertices(vers);
	if (direction2 == 1 || direction2 == 4)
	{
		// 箍筋弯钩处理
		rebar.EvaluateEndTypesStirrup(endTypes);
	}
	else
	{
		rebar.EvaluateEndTypes(endTypes);
	}

	rebars.push_back(rebar);

	//for (map<int, DPoint3d>::iterator itr = map_pts.begin(); itr != map_pts.end(); itr++)
	//{
	//	PITRebarCurve rebar;
	//	RebarVertexP vex;
	//	vex = &rebar.PopVertices().NewElement();
	//	vex->SetIP(itr->second);
	//	vex->SetType(RebarVertex::kStart);
	//	endTypes.beg.SetptOrgin(itr->second);

	//	rebar.SetVertices(vers);

	//	map<int, DPoint3d>::iterator itrplus = ++itr;
	//	if (itrplus == map_pts.end())
	//	{
	//		break;
	//	}

	//	endTypes.end.SetptOrgin(itrplus->second);

	//	vex = &rebar.PopVertices().NewElement();
	//	vex->SetIP(itrplus->second);
	//	vex->SetType(RebarVertex::kEnd);

	//	rebar.EvaluateEndTypes(endTypes);
	//	//		rebar.EvaluateEndTypes(endTypes, bendRadius, bendLen + bendRadius, &endNormal);
	//	rebars.push_back(rebar);
	//}


	//rebar.DoMatrix(mat);
	return true;
}

RebarSetTag* STRainshedRebarAssembly::MakeRebars
(
	ElementId&          rebarSetId,
	BrStringCR          sizeKey,
	double              xLen,
	double              width,
	double              spacing,
	double              startOffset,
	double              endOffset,

	vector<PIT::EndType> const& endType,	//存储起点端部与终点端部数据
	vector<CVector3D> const& vecEndNormal,
	CMatrix3D const&    mat,

	DgnModelRefP        modelRef
)
{
	bool const isStirrup = false;

	RebarSetP   rebarSet = RebarSet::Fetch(rebarSetId, modelRef);
	if (NULL == rebarSet)
		return NULL;

	rebarSet->SetRebarDisplayMode(RebarDisplayMode::kRebarCylinderMode);
	rebarSet->SetCallerId(GetCallerId());
	rebarSet->StartUpdate(modelRef);

	RebarEndType endTypeStart, endTypeEnd;
	switch (endType[0].endType)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeStart.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeStart.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeStart.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeStart.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeStart.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeStart.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	switch (endType[1].endType)
	{
	case 0:	//无
	case 1:	//弯曲
	case 2:	//吊钩
	case 3:	//折线
		endTypeEnd.SetType(RebarEndType::kNone);
		break;
	case 7:	//直锚
		endTypeEnd.SetType(RebarEndType::kLap);
		break;
	case 4:	//90度弯钩
		endTypeEnd.SetType(RebarEndType::kBend);
		break;
	case 5:	//135度弯钩
		endTypeEnd.SetType(RebarEndType::kCog);
		break;
	case 6:	//180度弯钩
		endTypeEnd.SetType(RebarEndType::kHook);
		break;
	case 8:	//用户
		endTypeEnd.SetType(RebarEndType::kCustom);
		break;
	default:
		break;
	}

	//	}//由于板配筋和墙配筋方向不同改保护层正反侧面
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double diameter = RebarCode::GetBarDiameter(sizeKey, modelRef);
	m_diameter1 = diameter;
	double bendRadius = RebarCode::GetPinRadius(sizeKey, modelRef, false) * 0.50;	//乘以了30

	m_bendRadius1 = bendRadius;
	double startbendLen = RebarCode::GetBendLength(sizeKey, endTypeStart, modelRef) * 0.5;	//乘以了100

	double endbendLen = RebarCode::GetBendLength(sizeKey, endTypeEnd, modelRef) * 0.5;	//乘以了100

	int numRebar = 0;
	double adjustedXLen, adjustedSpacing, highspacing;

	double allSideCov = GetSideCover()*uor_per_mm * 2; // 侧边保护层


	if (4 == direction2) // 第五层钢筋
	{
		WString strSizeFir = GetvecDirSize().at(1);
		if (strSizeFir.find(L"mm") != WString::npos)
		{
			strSizeFir.ReplaceAll(L"mm", L"");
		}
		// double tempLen;
		// tempLen = m_RainshedData.moreheight - m_RainshedData.minheight + 200;
		adjustedXLen = m_STRainshedData.height - GetPositiveCover() * 2 * uor_per_mm - m_zLength - diameter * 0.5 - RebarCode::GetBarDiameter(strSizeFir, modelRef);
	}
	else
	{
		if (1 == GetvecDir().at(direction2)) // 第2层钢筋
		{
			int iIndex = 1;
			if (direction2 == 3)
			{
				iIndex = 2;
			}
			WString strSizeTmp = GetvecDirSize().at(iIndex);
			if (strSizeTmp.find(L"mm") != WString::npos)
			{
				strSizeTmp.ReplaceAll(L"mm", L"");
			}
			double dTemp = RebarCode::GetBarDiameter(strSizeTmp, modelRef);

			adjustedXLen = xLen - 2.0 * GetSideCover() * uor_per_mm - diameter - startOffset - endOffset - 2 * dTemp;
		}
		else
		{
			adjustedXLen = xLen - 2.0 * GetSideCover() * uor_per_mm - 4 * diameter - startOffset - endOffset;
		}
	}
	numRebar = (int)floor(adjustedXLen / spacing + 0.5) + 1;
	adjustedSpacing = spacing;
	if (4 == direction2)
	{
		numRebar = 2;
	}
	if (numRebar > 1)
		adjustedSpacing = adjustedXLen / (numRebar - 1);
	highspacing = (m_RainshedData.maxheight - m_RainshedData.minheight) / (numRebar - 1);
	double xPos = startOffset;
	double zhig = 0;

	vector<PIT::PITRebarCurve>     rebarCurvesNum;
	int j = 0;
	double endTypeStartOffset = endType[0].offset * uor_per_mm;
	double endTypEendOffset = endType[1].offset * uor_per_mm;
	if (endType[0].endType != 0)	//端部弯曲时额外偏移钢筋半径
		endTypeStartOffset += diameter * 0.5;
	if (endType[1].endType != 0)	//端部弯曲时额外偏移钢筋半径
		endTypEendOffset += diameter * 0.5;

	double rebarLength = width - allSideCov;
	if (1 == direction2)
	{
		rebarLength -= diameter;

		if (GetRebarLevelNum() > 4)
		{
			WString strSizeFir = GetvecDirSize().at(4);
			if (strSizeFir.find(L"mm") != WString::npos)
			{
				strSizeFir.ReplaceAll(L"mm", L"");
			}
			rebarLength -= RebarCode::GetBarDiameter(strSizeFir, modelRef) * 2;
		}
	}

	if (2 == direction2)
	{
		rebarLength -= diameter;

		if (GetRebarLevelNum() > 4)
		{
			WString strSizeFir = GetvecDirSize().at(1);
			if (strSizeFir.find(L"mm") != WString::npos)
			{
				strSizeFir.ReplaceAll(L"mm", L"");
			}

			WString strSizeFour = GetvecDirSize().at(4);
			if (strSizeFour.find(L"mm") != WString::npos)
			{
				strSizeFour.ReplaceAll(L"mm", L"");
			}

			rebarLength -= RebarCode::GetBarDiameter(strSizeFir, modelRef) * 2 - RebarCode::GetBarDiameter(strSizeFour, modelRef) * 2;
		}
	}

	if (4 == direction2)
	{
		rebarLength -= diameter;
	}

	if (3 == direction2)
	{
		DVec3d vec, vecTmp;
		mdlVec_subtractPoint(&vec, &m_RainshedData.ptStart_H, &m_RainshedData.ptEnd_H);
		vec.Normalize();

		mdlVec_subtractPoint(&vecTmp, &m_RainshedData.ptStart_L, &m_RainshedData.ptEnd_L);
		vecTmp.Normalize();

		double dot = mdlVec_dotProduct(&vec, &vecTmp);

		rebarLength = rebarLength / dot;
	}

	PIT::PITRebarEndType start, end;
	start.SetType((PIT::PITRebarEndType::Type)endTypeStart.GetType());
	start.Setangle(endType[0].rotateAngle);

	start.SetbendLen(endbendLen);
	start.SetbendRadius(bendRadius);

	start.SetendNormal(vecEndNormal[0]);

	end.SetType((PIT::PITRebarEndType::Type)endTypeEnd.GetType());
	end.Setangle(endType[1].rotateAngle);

	end.SetbendLen(endbendLen);
	end.SetbendRadius(bendRadius);

	end.SetendNormal(vecEndNormal[1]);

	PITRebarEndTypes   endTypes = { start, end };

	for (int i = 0; i < numRebar; i++)//钢筋属性
	{
		bool tag;
		vector<PITRebarCurve>     rebarCurves;

		tag = true;
		makeRebarCurve(rebarCurves, xPos, rebarLength, zhig, endTypeStartOffset, endTypEendOffset, endTypes, mat, tag);

		xPos += adjustedSpacing;
		zhig += highspacing;
		if (!tag)
			continue;
		rebarCurvesNum.insert(rebarCurvesNum.end(), rebarCurves.begin(), rebarCurves.end());
	}//rebarset里面rebarelement初步建立完成
	//钢筋组
	numRebar = (int)rebarCurvesNum.size();

	RebarSymbology symb;

	string str(sizeKey);
	char ccolar[20] = { 0 };
	strcpy(ccolar, str.c_str());
	SetRebarColorBySize(ccolar, symb);
	symb.SetRebarLevel(TEXT_MAIN_REBAR);

	vector<vector<DPoint3d>> vecStartEnd;
	for (RebarCurve rebarCurve : rebarCurvesNum)
	{
		CPoint3D ptstr, ptend;
		rebarCurve.GetEndPoints(ptstr, ptend);
		DPoint3d midPos = ptstr;
		midPos.Add(ptend);
		midPos.Scale(0.5);
		if (g_wallRebarInfo.concrete.isHandleHole)
		{
			if (ISPointInHoles(m_Holeehs, midPos))
			{
				if (ISPointInHoles(m_Holeehs, ptstr) && ISPointInHoles(m_Holeehs, ptend))
				{
					continue;
				}
			}
		}

		vector<DPoint3d> linePts;
		RebarVertices vertices = rebarCurve.GetVertices();
		for (size_t i = 0; i < vertices.GetSize(); ++i)
		{
			RebarVertex *tmpVertex = &vertices.At(i);
			linePts.push_back(tmpVertex->GetIP());
		}
		//EditElementHandle eeh;
		//LineHandler::CreateLineElement(eeh, nullptr, DSegment3d::From(ptstr, ptend), true, *ACTIVEMODEL);
		//eeh.AddToModel();

		RebarElementP rebarElement = rebarSet->AssignRebarElement(j, numRebar, symb, modelRef);
		if (nullptr != rebarElement)
		{
			RebarShapeData shape;

			shape.SetSizeKey((LPCTSTR)sizeKey);
			shape.SetIsStirrup(isStirrup);
			shape.SetLength(rebarCurve.GetLength() / uor_per_mm);
			RebarEndTypes   endTypes = { endTypeStart, endTypeEnd };
			rebarElement->Update(rebarCurve, diameter, endTypes, shape, modelRef, false);

			ElementId eleid = rebarElement->GetElementId();
			EditElementHandle tmprebar(eleid, ACTIVEMODEL);
			string Stype = "front";
			ElementRefP oldref = tmprebar.GetElementRef();
			SetRebarLevelItemTypeValue(tmprebar, Stype, ACTIVEMODEL);
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
	ret = rebarSet->FinishUpdate(setdata, modelRef);	//返回的是钢筋条数

	RebarSetTag* tag = new RebarSetTag();
	tag->SetRset(rebarSet);
	tag->SetIsStirrup(isStirrup);

	return tag;
}

bool STRainshedRebarAssembly::MakeRebars(DgnModelRefP modelRef)      // 创建钢筋
{
	NewRebarAssembly(modelRef);
	RebarSetTagArray rsetTags;
	m_vecAllRebarStartEnd.clear();
	g_vecRebarPtsNoHole.clear();

	int iTwinbarSetIdIndex = 0;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	CMatrix3D const rot90 = CMatrix3D::Rotate(PI / 2.0, CVector3D::kZaxis);
	double dLevelSpace = 0;
	double dSideCover = GetSideCover()*uor_per_mm;//反面保护层
	//if ((COMPARE_VALUES(dSideCover, m_RainshedData.length) >= 0) || (COMPARE_VALUES(dSideCover, m_RainshedData.width) >= 0))	//如果侧面保护层大于等于墙的长度（板配筋改成高度）
	//{
	//	mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"侧面保护层大于等于  板的长度或宽度 ,无法创建钢筋层", MessageBoxIconType::Information);
	//	return false;
	//}
	vector<CVector3D> vTrans;
	//计算侧面整体偏移量
	if (GetRebarLevelNum() == 4)
	{
		CalculateTransformSPY1(vTrans, modelRef);
	}
	else if (GetRebarLevelNum() == 5)
	{
		CalculateTransformSPY2(vTrans, modelRef);
	}
	if (vTrans.size() != GetRebarLevelNum())
	{
		return false;
	}
	//高当作宽，墙面
	double dLength = m_RainshedData.width_down;
	double dWidth = m_RainshedData.length_down; // 以下底面为标准长度

#ifdef PDMSIMPORT
	dLength *= modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	dWidth *= modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
#endif

	int iRebarSetTag = 0;
	int iRebarLevelNum = GetRebarLevelNum();
	vector<PIT::EndType> vecEndType;
	for (int i = 0; i < iRebarLevelNum; i++)
	{
		if (GetvecSetId().size() < iRebarLevelNum)
		{
			PopvecSetId().push_back(0);
		}

		direction2 = i;
		RebarSetTag* tag = NULL;
		CMatrix3D   mat;
		vector<PIT::EndType> vecEndType;


		//RebarEndType endType;
		//endType.SetType(RebarEndType::kBend);//设置端部90°
		//搭接，此时根据搭接选项的数据进行钢筋截断，生成多组钢筋
		double dActualWidth = dWidth;
		double dActualLength = dLength;
		int iRebarSetNum = 1;
		double overLength = 0.0;
		if (GetvecDir().at(i) == 1)	//纵向钢筋
		{

			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向

			vecEndType = { { 0,0,0 },{0,0,0} };
			for (size_t k = 0; k < vecEndNormal.size(); ++k)
			{
				double dRotateAngle = vecEndType.at(k).rotateAngle;
				CVector3D rebarVec = m_RainshedData.ptEnd_L - m_RainshedData.ptStart_L;
				endNormal = CVector3D::From(0, 0, 1);
				if (i == iRebarLevelNum - 1 || i == iRebarLevelNum - 2)
				{
					endNormal.Negate();
				}
				CVector3D  yVecNegate = CVector3D::kZaxis.CrossProduct(rebarVec);     //返回两个向量的（标量）叉积。y
				endNormal.Rotate(-90 * PI / 180, yVecNegate);
				endNormal.Rotate(dRotateAngle * PI / 180, yVecNegate);	//以钢筋方向为轴旋转
				vecEndNormal[k] = endNormal;
			}

			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;


			tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dLength, dWidth, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i) * uor_per_mm,
				GetvecEndOffset().at(i) * uor_per_mm, vecEndType, vecEndNormal, mat, modelRef);
			vecEndType.clear();
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}

			vecEndType.clear();

		}
		else
		{
			vector<CVector3D> vecEndNormal(2);
			CVector3D	endNormal;	//端部弯钩方向

			vecEndType = { { 6,0,0 },{6,0,0} };
			for (size_t k = 0; k < vecEndNormal.size(); ++k)
			{
				double dRotateAngle = vecEndType.at(k).rotateAngle;
				CVector3D rebarVec = m_RainshedData.ptEnd_L - m_RainshedData.ptStart_L;
				endNormal = CVector3D::From(0, 0, 1);
				if (i == 1 || i == 4)
				{
					if (iRebarLevelNum == 4 && i == 1)
					{
						endNormal = CVector3D::From(0, 0, -1);
					}
					else if (iRebarLevelNum == 5 && i == 1)
					{
						endNormal = m_RainshedData.ptStart_L - m_RainshedData.ptStartBack_L;
						endNormal.Normalize();
						if (k == 1)
						{
							endNormal.Negate();
						}
					}
					else
					{
						endNormal =  m_RainshedData.ptStartBack_L - m_RainshedData.ptStart_L;
						endNormal.Normalize();
						if (k == 1)
						{
							endNormal.Negate();
						}

					}
				}
				endNormal.Rotate(dRotateAngle * PI / 180, rebarVec);	//以钢筋方向为轴旋转
				vecEndNormal[k] = endNormal;
			}

			mat = rot90;
			mat.SetTranslation(vTrans[i]);
			mat = GetPlacement() * mat;

			tag = MakeRebars(PopvecSetId().at(i), GetvecDirSize().at(i), dWidth, dLength, GetvecDirSpacing().at(i)*uor_per_mm, GetvecStartOffset().at(i) * uor_per_mm,
				GetvecEndOffset().at(i) * uor_per_mm, vecEndType, vecEndNormal, mat, modelRef);
			vecEndType.clear();
			if (NULL != tag)
			{
				tag->SetBarSetTag(i + 1);
				rsetTags.Add(tag);
			}

			vecEndType.clear();
		}

	}

	if (g_globalpara.Getrebarstyle() != 0)
	{
		return (SUCCESS == AddRebarSets(rsetTags));
	}
	return true;
}

void STRainshedRebarAssembly::CalculateTransformSPY1(vector<CVector3D> &vTransform, DgnModelRefP modelRef)         //计算转换
{
	if (modelRef == NULL && GetRebarLevelNum() < 4)
	{
		return;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;             //单位

	vTransform.clear();

	double dSideCover = GetSideCover() * uor_per_mm;//侧
	double dPositiveCover = GetPositiveCover() * uor_per_mm; //上
	double dReverseCover = GetReverseCover() * uor_per_mm; //下

	WString strSizeBend = GetvecDirSize().at(1);
	if (strSizeBend.find(L"mm") != WString::npos)
	{
		strSizeBend.ReplaceAll(L"mm", L"");
	}
	double bendRadius = RebarCode::GetPinRadius(strSizeBend, modelRef, false) * 0.50;	//乘以了30

	WString strSizeIndex = GetvecDirSize().at(0);
	if (strSizeIndex.find(L"mm") != WString::npos)
	{
		strSizeIndex.ReplaceAll(L"mm", L"");
	}

	double zLen = dReverseCover + (bendRadius * 2 - RebarCode::GetBarDiameter(strSizeIndex, modelRef));
	double dIndex = 0.00;
	for (int i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}
		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10

		zLen += diameter * 0.5;

		CVector3D zTrans = CVector3D::From(0, 0, 0);

		double levelSpacing = GetvecLevelSpace().at(i) * uor_per_mm;

		double dHeight = m_STRainshedData.height;
		if (GetRebarLevelNum() == 5)
		{
			dHeight = m_RainshedData.maxheight;
		}

		if (COMPARE_VALUES_EPS(zLen + levelSpacing, dHeight, EPS) > 0)
		{
			double diameterTol = 0.00;
			for (int j = i + 1; j < GetRebarLevelNum(); j++)
			{
				WString strSizeTmp = GetvecDirSize().at(j);
				if (strSizeTmp.find(L"mm") != WString::npos)
				{
					strSizeTmp.ReplaceAll(L"mm", L"");
				}
				diameterTol += RebarCode::GetBarDiameter(strSizeTmp, modelRef); // 剩余层的总直径
			}

			WString strSizeTmp = GetvecDirSize().at(2);
			if (strSizeTmp.find(L"mm") != WString::npos)
			{
				strSizeTmp.ReplaceAll(L"mm", L"");
			}
			double bendRadiusTmp = RebarCode::GetPinRadius(strSizeTmp, modelRef, false) * 0.50;	//乘以了30

			WString strSizeTTmp = GetvecDirSize().at(3);
			if (strSizeTTmp.find(L"mm") != WString::npos)
			{
				strSizeTTmp.ReplaceAll(L"mm", L"");
			}

			double dTmp = bendRadiusTmp * 2 - RebarCode::GetBarDiameter(strSizeTTmp, modelRef);

			zLen += dHeight - zLen - dPositiveCover - diameterTol - diameter * 0.5 - dTmp;
		}
		else
		{
			zLen += levelSpacing;
		}
		
		if (i == 0)
		{
			WString strSizeTmp = GetvecDirSize().at(1);
			if (strSizeTmp.find(L"mm") != WString::npos)
			{
				strSizeTmp.ReplaceAll(L"mm", L"");
			}

			zTrans.z = zLen;
			zTrans.x = diameter * 0.5 + dSideCover + RebarCode::GetBarDiameter(strSizeTmp, modelRef);
			zTrans.y = m_RainshedData.length_down * 0.5;
		}
		else if (i == 1)
		{
			zTrans.z = zLen;
			zTrans.y = diameter * 0.5 + dSideCover;
			zTrans.x = m_RainshedData.width_down * 0.5;
		}
		else if (i == 2)
		{
			zTrans.y = diameter * 0.5 + dSideCover;
			zTrans.x = m_RainshedData.width_down * 0.5;
			
			DVec3d vec, vecTmp;
			mdlVec_subtractPoint(&vec, &m_RainshedData.ptStart_H, &m_RainshedData.ptEnd_H);
			vec.Normalize();

			mdlVec_subtractPoint(&vecTmp, &m_RainshedData.ptStart_L, &m_RainshedData.ptEnd_L);
			vecTmp.Normalize();

			double dot = mdlVec_dotProduct(&vec, &vecTmp);
			zTrans.z = (zLen - 1.0 * uor_per_mm) * dot;

			dIndex = zLen;
		}
		else if (i == 3)
		{
			WString strSizeTmp = GetvecDirSize().at(2);
			if (strSizeTmp.find(L"mm") != WString::npos)
			{
				strSizeTmp.ReplaceAll(L"mm", L"");
			}

			zTrans.z = zLen;
			zTrans.x = diameter * 0.5 + dSideCover + RebarCode::GetBarDiameter(strSizeTmp, modelRef);

			DVec3d vec, vecTmp;
			mdlVec_subtractPoint(&vec, &m_RainshedData.ptStart_H, &m_RainshedData.ptEnd_H);
			vec.Normalize();

			mdlVec_subtractPoint(&vecTmp, &m_RainshedData.ptStart_L, &m_RainshedData.ptEnd_L);
			vecTmp.Normalize();

			double dot = mdlVec_dotProduct(&vec, &vecTmp);
			zTrans.y = (m_RainshedData.length_down * 0.5) / dot;
		}

		zLen += diameter * 0.5;
		vTransform.push_back(zTrans);
	}
}

void STRainshedRebarAssembly::CalculateTransformSPY2(vector<CVector3D> &vTransform, DgnModelRefP modelRef)         //计算转换
{
	if (modelRef == NULL && GetRebarLevelNum() < 5)
	{
		return;
	}

	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;             //单位

	vTransform.clear();

	double dSideCover = GetSideCover() * uor_per_mm;//侧
	double dPositiveCover = GetPositiveCover() * uor_per_mm; //上
	double dReverseCover = GetReverseCover() * uor_per_mm; //下

	WString strSizeBend = GetvecDirSize().at(1);
	if (strSizeBend.find(L"mm") != WString::npos)
	{
		strSizeBend.ReplaceAll(L"mm", L"");
	}
	double bendRadius = RebarCode::GetPinRadius(strSizeBend, modelRef, false) * 0.50;	//乘以了30

	WString strSizeIndex = GetvecDirSize().at(0);
	if (strSizeIndex.find(L"mm") != WString::npos)
	{
		strSizeIndex.ReplaceAll(L"mm", L"");
	}

	double zLen = dReverseCover;
	for (int i = 0; i < GetRebarLevelNum(); i++)
	{
		WString strSize = GetvecDirSize().at(i);
		if (strSize.find(L"mm") != WString::npos)
		{
			strSize.ReplaceAll(L"mm", L"");
		}
		double diameter = RebarCode::GetBarDiameter(strSize, modelRef);		//乘以了10

		zLen += diameter * 0.5;

		CVector3D zTrans = CVector3D::From(0, 0, 0);

		double levelSpacing = GetvecLevelSpace().at(i) * uor_per_mm;

		double dHeight = m_STRainshedData.height;
		if (GetRebarLevelNum() == 5)
		{
			dHeight = m_RainshedData.maxheight;
		}

		if (COMPARE_VALUES_EPS(zLen + levelSpacing, dHeight, EPS) > 0)
		{
			double diameterTol = 0.00;
			for (int j = i + 1; j < GetRebarLevelNum() - 1; j++)
			{
				WString strSizeTmp = GetvecDirSize().at(j);
				if (strSizeTmp.find(L"mm") != WString::npos)
				{
					strSizeTmp.ReplaceAll(L"mm", L"");
				}
				diameterTol += RebarCode::GetBarDiameter(strSizeTmp, modelRef); // 剩余层的总直径
			}

			WString strSizeTmp = GetvecDirSize().at(2);
			if (strSizeTmp.find(L"mm") != WString::npos)
			{
				strSizeTmp.ReplaceAll(L"mm", L"");
			}
			double bendRadiusTmp = RebarCode::GetPinRadius(strSizeTmp, modelRef, false) * 0.50;	//乘以了30

			WString strSizeTTmp = GetvecDirSize().at(3);
			if (strSizeTTmp.find(L"mm") != WString::npos)
			{
				strSizeTTmp.ReplaceAll(L"mm", L"");
			}

			double dTmp = bendRadiusTmp * 2 - RebarCode::GetBarDiameter(strSizeTTmp, modelRef);

			zLen += dHeight - zLen - dPositiveCover - diameterTol - diameter * 0.5 - dTmp;
		}
		else
		{
			zLen += levelSpacing;
		}

		if (i == 0)
		{
			WString strSizeTmp = GetvecDirSize().at(1);
			if (strSizeTmp.find(L"mm") != WString::npos)
			{
				strSizeTmp.ReplaceAll(L"mm", L"");
			}

			zTrans.z = zLen;
			zTrans.x = diameter * 0.5 + dSideCover + RebarCode::GetBarDiameter(strSizeTmp, modelRef);
			zTrans.y = m_RainshedData.length_down * 0.5;
		}
		else if (i == 1)
		{
			zTrans.z = zLen;
			zTrans.y = diameter * 0.5 + dSideCover;
			zTrans.x = m_RainshedData.width_down * 0.5;
		}
		else if (i == 2)
		{
			zTrans.y = diameter * 0.5 + dSideCover;
			zTrans.x = m_RainshedData.width_down * 0.5;

			DVec3d vec, vecTmp;
			mdlVec_subtractPoint(&vec, &m_RainshedData.ptStart_H, &m_RainshedData.ptEnd_H);
			vec.Normalize();

			mdlVec_subtractPoint(&vecTmp, &m_RainshedData.ptStart_L, &m_RainshedData.ptEnd_L);
			vecTmp.Normalize();

			double dot = mdlVec_dotProduct(&vec, &vecTmp);

			zTrans.z = zLen;
		}
		else if (i == 3)
		{
			WString strSizeTmp = GetvecDirSize().at(2);
			if (strSizeTmp.find(L"mm") != WString::npos)
			{
				strSizeTmp.ReplaceAll(L"mm", L"");
			}

			zTrans.z = zLen;
			zTrans.x = diameter * 0.5 + dSideCover + RebarCode::GetBarDiameter(strSizeTmp, modelRef);

			DVec3d vec, vecTmp;
			mdlVec_subtractPoint(&vec, &m_RainshedData.ptStart_H, &m_RainshedData.ptEnd_H);
			vec.Normalize();

			mdlVec_subtractPoint(&vecTmp, &m_RainshedData.ptStart_L, &m_RainshedData.ptEnd_L);
			vecTmp.Normalize();

			double dot = mdlVec_dotProduct(&vec, &vecTmp);
			zTrans.y = (m_RainshedData.length_down * 0.5) / dot;
		}
		else if (i == 4)
		{
			double dZLength = m_RainshedData.minheight;
			if (COMPARE_VALUES_EPS(zLen + levelSpacing, m_STRainshedData.height, EPS) > 0)
			{
				dZLength += m_STRainshedData.height - dZLength - dPositiveCover - diameter * 0.5;
			}
			else
			{
				dZLength += levelSpacing;
			}

			m_zLength = dZLength + dPositiveCover;
			zTrans.z = m_zLength;
			zTrans.x = m_RainshedData.width_down * 0.5;

			WString strSizeTmp = GetvecDirSize().at(1);
			if (strSizeTmp.find(L"mm") != WString::npos)
			{
				strSizeTmp.ReplaceAll(L"mm", L"");
			}

			WString strSizeTTmp = GetvecDirSize().at(0);
			if (strSizeTTmp.find(L"mm") != WString::npos)
			{
				strSizeTTmp.ReplaceAll(L"mm", L"");
			}

			WString strSizeTTmp1 = GetvecDirSize().at(3);
			if (strSizeTTmp1.find(L"mm") != WString::npos)
			{
				strSizeTTmp1.ReplaceAll(L"mm", L"");
			}

			WString strSizeTTmp2 = GetvecDirSize().at(2);
			if (strSizeTTmp2.find(L"mm") != WString::npos)
			{
				strSizeTTmp2.ReplaceAll(L"mm", L"");
			}

			double dDiameter = RebarCode::GetBarDiameter(strSizeTmp, modelRef) + RebarCode::GetBarDiameter(strSizeTTmp, modelRef) + RebarCode::GetBarDiameter(strSizeTTmp1, modelRef) +
				+RebarCode::GetBarDiameter(strSizeTTmp2, modelRef);

			zTrans.y = m_RainshedData.length_down - dSideCover - diameter * 0.5 - dDiameter;
		}

		zLen += diameter * 0.5;
		vTransform.push_back(zTrans);
	}
}

long STRainshedRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
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

bool STRainshedRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	pSTRainshedDoubleRebarDlg = new RainshedRebarDlg(ehSel,CWnd::FromHandle(MSWIND));
	pSTRainshedDoubleRebarDlg->Create(IDD_DIALOG_RainshedRebar);
	pSTRainshedDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pSTRainshedDoubleRebarDlg->ShowWindow(SW_SHOW);


// 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
// 	RainshedRebarDlg dlg(ehSel, CWnd::FromHandle(MSWIND));
// 
// 	//	dlg.SetSelectElement(ehSel);
// 	dlg.SetConcreteId(FetchConcrete());
// 	if (IDCANCEL == dlg.DoModal())
// 	{
// 		return false;
// 	}

	return true;
}

bool STRainshedRebarAssembly::Rebuild()
{
	if (!GetSelectedModel())
		return false;

	ElementHandle ehslab(GetSelectedElement(), GetSelectedModel());
	if (!ehslab.IsValid())
		return false;

	DgnModelRefP modelRef = ehslab.GetModelRef();

	SetWallData(ehslab);

	MakeRebars(modelRef);//调用创建钢筋
	Save(modelRef);

	ElementId contid = FetchConcrete();
	return true;
}

bool STRainshedRebarAssembly::SetWallData(ElementHandleCR eh)
{
#ifdef PDMSIMPORT
	WString strName;
	WString strType;
	DgnECManagerR ecMgr = DgnECManager::GetManager();
	FindInstancesScopePtr scope = FindInstancesScope::CreateScope(eh, FindInstancesScopeOption(DgnECHostType::Element));
	ECQueryPtr            ecQuery = ECQuery::CreateQuery(ECQUERY_PROCESS_SearchAllClasses);
	//ECQUERY_PROCESS_SearchAllExtrinsic will only search ECXAttr
	ecQuery->SetSelectProperties(true);

	for (DgnECInstancePtr instance : ecMgr.FindInstances(*scope, *ecQuery))
	{
		DgnElementECInstanceP elemInst = instance->GetAsElementInstance();
		for (ECPropertyP ecProp : elemInst->GetClass().GetProperties())
		{
			WString strLabel = ecProp->GetDisplayLabel().GetWCharCP();
			if (strLabel != L"NAME" && strLabel != L"TYPE")
			{
				continue;
			}

			WString valStr;
			ECValue ecVal;
			elemInst->GetValue(ecVal, ecProp->GetName().GetWCharCP());
			IDgnECTypeAdapterR typeAdapter = IDgnECTypeAdapter::GetForProperty(*ecProp);
			IDgnECTypeAdapterContextPtr typeContext = IDgnECTypeAdapterContext::Create(*ecProp, *elemInst, ecProp->GetName().GetWCharCP());
			typeAdapter.ConvertToString(valStr, ecVal, *typeContext);

			if (strLabel == L"NAME")
				strName = valStr;
			else
				strType = valStr;
		}
	}
	string name = StringOperator::Convert::WStringToString(strName.c_str());
	//读取数据
//	ComponentDataManage dataManage("C:\\Users\\Power-Itech-LX\\Desktop\\data\\1BFX15--VB-.bim"); // 构件数据管理
	ComponentDataManage dataManage("C:\\Users\\Power-Itech-LX\\Desktop\\1BFX-----CW\\1BFX-----B\\data\\1BFX2078VB-.bim"); // 构件数据管理
	dataManage.GetAllComponentData();

	vector<IComponent*> vecComponent = ComponentDataManage::GetAllComponent();
	for (size_t i = 0; i < vecComponent.size(); i++)
	{
		IComponent *Component = vecComponent[i];
		string strComponentName = Component->GetEleName();
		string strType = Component->GetEleType();
		if (strType == "STWALL")
		{
			STWallComponent* stWall = dynamic_cast<STWallComponent*>(Component);
			if (stWall == NULL)
			{
				continue;
			}
			string strComponentName = stWall->GetEleName();
			if (name == strComponentName)
			{
				pWall = stWall;
			}
		}
	}
	if (pWall == NULL || !eh.IsValid())
	{
		return false;
	}

	double uor_per_mm = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	m_STwallData.length = pWall->GetPtStart().Distance(pWall->GetPtEnd()) * uor_per_mm;
	m_STwallData.width = pWall->GetWidth() * uor_per_mm;
	m_STwallData.height = pWall->GetHeight() * uor_per_mm;

	DPoint3d ptStart = pWall->GetPtStart();
	ptStart.x *= uor_per_mm;
	ptStart.y *= uor_per_mm;
	ptStart.z *= uor_per_mm;
	DPoint3d ptEnd = pWall->GetPtEnd();
	ptEnd.x *= uor_per_mm;
	ptEnd.y *= uor_per_mm;
	ptEnd.z *= uor_per_mm;


	CVector3D  xVec(ptStart, ptEnd);

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);
	//由于该线为底面中心线，需将原点进行偏移1/2的厚度
	//以yVec的反反向进行偏移1/2的墙厚
	CVector3D  yVecNegate = yVec;
	yVecNegate.Negate();
	yVecNegate.Normalize();
	yVecNegate.ScaleToLength(m_STwallData.width * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	ptStart.Add(yVecNegate);
	ptEnd.Add(yVecNegate);

	CVector3D  xVecNew(ptStart, ptEnd);

	// 	CVector3D  xVecNegate = xVecNew;
	// 	xVecNegate.Normalize();
	// 	xVecNegate.ScaleToLength(m_STwallData.length * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	// 	ptStart.Add(xVecNegate);
	// 	CVector3D  zVecNegate = CVector3D::kZaxis;
	// 	zVecNegate.Normalize();
	// 	zVecNegate.ScaleToLength(m_STwallData.height * 0.5);//起点与终点连线为墙的中心线，所以此处只需要偏移1/2厚度
	// 	ptStart.Add(zVecNegate);
	BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
#else
	//	m_eeh1 = eh;
	bool bRet = AnalyzingSlabGeometricData(eh);
	if (!bRet)
		return false;

	DPoint3d ptStart = m_RainshedData.ptStart_L;            //原点

	m_xVec = m_RainshedData.ptStartBack_L - m_RainshedData.ptStart_L;
	m_xVec.Normalize();

	CVector3D  yVec = CVector3D::kZaxis.CrossProduct(m_xVec);     //返回两个向量的（标量）叉积。y

	BeMatrix   placement = CMatrix3D::Ucs(ptStart, m_xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴
//	BeMatrix   placement = CMatrix3D::Ucs(ptStart, CVector3D::kXaxis, CVector3D::kYaxis, false);		//方向为X轴，水平垂直方向为Y轴
	SetPlacement(placement);

	SetSelectedElement(eh.GetElementId());
	SetSelectedModel(eh.GetModelRef());
	return true;
#endif
}

bool STRainshedRebarAssembly::AnalyzingSlabGeometricData(ElementHandleCR eh)
{
	double uor_now = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double uor_ref = eh.GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	vector<MSElementDescrP> vecDownFaceLine;
	vector<MSElementDescrP> vecDownFontLine;
	vector<MSElementDescrP> vecDownBackLine;

	EditElementHandle testeeh(eh, false);
//	DPoint3d tempP;
	EditElementHandle eeh1(eh, eh.GetModelRef());
	EditElementHandle eehDownFace;

	vector<DSegment3d> vec_linefront_Down; // 雨棚上底面前面的线
	vector<DSegment3d> vec_lineback_Down; // 雨棚下底面背面的线
	vector<MSElementDescrP> vecHighFaceLine;
	vector<MSElementDescrP> vecLowFontLine;
	vector<DSegment3d> vecHightFaceLine; // 雨棚上顶面前面的线
	vector<DSegment3d> vecHightBackLine; // 雨棚下顶面背面的线
	STRainshedRebarAssembly::GetFrontBackLineAndHighFace(eh, NULL, vecHighFaceLine, vecLowFontLine, vecHightFaceLine, vecHightBackLine, vec_linefront_Down, vec_lineback_Down, &m_STRainshedData.height);

	DgnModelRefP model = eh.GetModelRef();

	vector<DPoint3d> allpoint;
	vector<DPoint3d> twopoint;
	DPoint3d tmpp[2];

	for (size_t i = 0; i < vecHighFaceLine.size(); i++)
	{
		mdlLinear_extract(tmpp, NULL, &vecHighFaceLine[i]->el, model);//暂时使用当前激活MODEL，如有问题再修改
		mdlElmdscr_freeAll(&vecHighFaceLine.at(i));

		allpoint.push_back(tmpp[0]);
		allpoint.push_back(tmpp[1]);
	}

	for (size_t i = 0; i < vecLowFontLine.size(); i++)
	{
		// mdlLinear_extract(tmpp, NULL, &vecLowFontLine[i]->el, model);//暂时使用当前激活MODEL，如有问题再修改
		mdlElmdscr_freeAll(&vecLowFontLine.at(i));
	}

	for (size_t i = 0; i < allpoint.size(); i++)
	{
		if (abs(allpoint[i].z - m_RainshedData.hightfacep.high.z) < 10)
		{
			if (twopoint.size() > 0)
			{
				if (!(allpoint[i].IsEqual(twopoint[0])))
					twopoint.push_back(allpoint[i]);
			}
			else
			{
				twopoint.push_back(allpoint[i]);
			}

		}
	}

	DPoint3d ptStart;
	DPoint3d ptEnd;
	if ((twopoint[0].x - twopoint[1].x) < 10)
	{
		if ((twopoint[0].y - twopoint[1].y) > 0)
		{
			ptStart = twopoint[1];
			ptEnd = twopoint[0];
		}
		else
		{
			ptStart = twopoint[0];
			ptEnd = twopoint[1];
		}

	}
	else
	{
		if ((twopoint[0].x - twopoint[1].x) > 0)
		{
			ptStart = twopoint[1];
			ptEnd = twopoint[0];
		}
		else
		{
			ptStart = twopoint[0];
			ptEnd = twopoint[1];
		}

	}

	CVector3D  xVec(ptStart, ptEnd);
	m_xVec = xVec;

	DPoint3d pt1[2];
	vecHightFaceLine[0].GetStartPoint(pt1[0]);
	vecHightFaceLine[0].GetEndPoint(pt1[1]);

	DPoint3d pt2[2];
	vecHightBackLine[0].GetStartPoint(pt2[0]);
	vecHightBackLine[0].GetEndPoint(pt2[1]);

	std::vector<EditElementHandle*> Negs;
	if (vecHightBackLine.size() > 1 || vecHightFaceLine.size() > 1)
	{
		GetMaxDownFacePts(vecHightFaceLine, vecHightBackLine, pt1, pt2);
	}

	DPoint3d FrontStr, FrontEnd;
	DPoint3d BackStr, BackEnd;
	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_STRainshedData.height);
	m_RainshedData.length = FrontStr.Distance(FrontEnd)*uor_now / uor_ref; // 上底面的长
	m_RainshedData.width = FrontStr.Distance(BackStr)*uor_now / uor_ref; // 上底面的宽

	m_RainshedData.ptStart_H = BackStr;
	m_RainshedData.ptEnd_H = BackEnd;

	vec_linefront_Down[0].GetStartPoint(pt1[0]);
	vec_linefront_Down[0].GetEndPoint(pt1[1]);

	vec_lineback_Down[0].GetStartPoint(pt2[0]);
	vec_lineback_Down[0].GetEndPoint(pt2[1]);

	if (vec_lineback_Down.size() > 1 || vec_linefront_Down.size() > 1)
	{
		GetMaxDownFacePts(vec_linefront_Down, vec_lineback_Down, pt1, pt2);
	}

	EFT::GetRegionPts(FrontStr, FrontEnd, BackStr, BackEnd, Negs, pt1, pt2, model, m_STRainshedData.height);
	if (GetRebarLevelNum() == 4)
	{
		m_RainshedData.length_down = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;
		m_RainshedData.width_down = FrontStr.Distance(BackStr)*uor_now / uor_ref;

		m_RainshedData.ptStart_L = BackStr;
		m_RainshedData.ptEnd_L = BackEnd;
		m_RainshedData.ptStartBack_L = FrontStr;
	}
	else
	{
		m_RainshedData.length_down = FrontStr.Distance(BackStr)*uor_now / uor_ref;
		m_RainshedData.width_down = FrontStr.Distance(FrontEnd)*uor_now / uor_ref;

		m_RainshedData.ptStart_L = FrontStr;
		m_RainshedData.ptEnd_L = BackStr;
		m_RainshedData.ptStartBack_L = FrontEnd;
	}

	//EditElementHandle eehTmp3;
	//LineHandler::CreateLineElement(eehTmp3, nullptr, DSegment3d::From(m_RainshedData.ptStart_L, m_RainshedData.ptEnd_L), true, *ACTIVEMODEL);
	//eehTmp3.AddToModel();

	return true;
}

void STRainshedRebarAssembly::movePoint(DPoint3d vec, DPoint3d& movePt, double disLen, bool bFlag)
{
	vec.Normalize();
	if (!bFlag)
	{
		vec.Negate();
	}
	vec.ScaleToLength(disLen);
	movePt.Add(vec);
}

void STRainshedRebarAssembly::GetRainshedFace(ElementHandleCR eeh, EditElementHandleR DownFace, EditElementHandleR DownLowFace, double* tHeight)
{
	ISolidKernelEntityPtr entityPtr;
	if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
	{
		DRange3d range;
		Dpoint3d lowPt;
		Dpoint3d highPt;
		BentleyStatus nStatus = SolidUtil::GetEntityRange(range, *entityPtr);
		if (SUCCESS == nStatus)
		{
			if (tHeight != NULL)
				*tHeight = range.ZLength();
			lowPt = range.low;
			highPt = range.high;
		}

		bvector<ISubEntityPtr> subEntities;
		size_t iSubEntityNum = SolidUtil::GetBodyFaces(&subEntities, *entityPtr);

		if (iSubEntityNum > 0)
		{
			size_t iSize = subEntities.size();
			DRange3d tp[2],tp2;
			ISubEntityPtr tempsubEntity[2];
			int i = 0;
			int j = 0;
			for (size_t iIndex = 0; iIndex < iSize; ++iIndex)
			{
				ISubEntityPtr subEntity = subEntities[iIndex];

				DRange3d tmprange;
				nStatus = SolidUtil::GetSubEntityRange(tmprange, *subEntity);
				if (SUCCESS == nStatus)
				{
					if (GetRebarLevelNum() == 4)
					{
						double dTmp = abs(tmprange.high.z - tmprange.low.z);
						double dTTmp = abs(range.high.z - range.low.z);
						// (z轴最小值最大值差 小于 盖板体rang的z轴差  且 最大z值在最上面) 或 z轴值在一个面上
						if ((COMPARE_VALUES_EPS(dTmp, dTTmp, 10) < 0 && abs(tmprange.high.z - range.high.z) < 10) || abs(tmprange.high.z - tmprange.low.z) < 10)
						{
							if (!(abs(tmprange.high.z - tmprange.low.z) < 10))//上面和底面
							{//存放上下斜面range和面
								if (i < 2)
								{
									tp[i] = tmprange;
									tempsubEntity[i] = subEntity;
								}
								i++;
							}
							else
							{
								if (abs(tmprange.high.z - range.low.z) < 10)//最底面
								{
									if (i < 2)
									{
										tp[i] = tmprange;
										tempsubEntity[i] = subEntity;
									}
									i++;
								}
							}

						}
					}

					if (GetRebarLevelNum() == 5)
					{
						if (!(abs(tmprange.high.z - range.high.z) < 10)) // rang最大z值不等于最高值
						{
							j++;
							if (!(abs(tmprange.high.z - tmprange.low.z) < 10))//上面和底面
							{//存放上下斜面range和面
								if (i < 2)
								{
									tp[i] = tmprange;
									tempsubEntity[i] = subEntity;
								}
								i++;
								tp2 = tmprange;
							}
							else
							{
								if (abs(tmprange.high.z - range.low.z) < 10)//最底面
								{
									if (i < 2)
									{
										tp[i] = tmprange;
										tempsubEntity[i] = subEntity;
									}
									i++;
								}
							}

						}
					}
				}
			}
			ISubEntityPtr tempsub;//tempsub存放顶面
			ISubEntityPtr lowsub;// 最底面
			if ((tp[0].high.z - tp[1].high.z)>0)// 如果第一个range高的的点大于第二个的，那么第一个range为上面斜面range
			{
				m_RainshedData.hightfacep = tp[0];
				m_RainshedData.lowfacep = tp[1];
				tempsub = tempsubEntity[0];
				lowsub = tempsubEntity[1];
			}
			else
			{
				m_RainshedData.hightfacep = tp[1];
				m_RainshedData.lowfacep = tp[0];
				tempsub = tempsubEntity[1];
				lowsub = tempsubEntity[0];
			}

			Dpoint3d temp = m_RainshedData.lowfacep.high;
			temp.x = m_RainshedData.lowfacep.low.x;
			m_RainshedData.width_down = temp.Distance(m_RainshedData.lowfacep.low);

			temp = m_RainshedData.lowfacep.high;
			temp.y = m_RainshedData.lowfacep.low.y;
			m_RainshedData.length = temp.Distance(m_RainshedData.lowfacep.low);

			m_RainshedData.maxheight = m_RainshedData.hightfacep.high.z - m_RainshedData.lowfacep.low.z;
			m_RainshedData.minheight = m_RainshedData.hightfacep.low.z - m_RainshedData.lowfacep.high.z;
			if(j == 2)
				m_RainshedData.moreheight = tp2.high.z - m_RainshedData.lowfacep.high.z;
			//获取底面
			CurveVectorPtr  curves;
			SolidUtil::Convert::SubEntityToCurveVector(curves, *tempsub);
			if (curves != NULL)
			{
				DraftingElementSchema::ToElement(DownFace, *curves, nullptr, eeh.GetModelRef()->Is3d(), *eeh.GetModelRef());
				if (DownFace.IsValid())
				{
					;
				}
			}
			else
			{
				IGeometryPtr geom;
				SolidUtil::Convert::SubEntityToGeometry(geom, *tempsub, *eeh.GetModelRef());
				ISolidPrimitivePtr tmpPtr = geom->GetAsISolidPrimitive();
				if (tmpPtr != NULL)
				{

					if (SUCCESS == DraftingElementSchema::ToElement(DownFace, *tmpPtr, nullptr, *eeh.GetModelRef()))
					{
						;
					}
				}
			}

			//获取底面
			CurveVectorPtr  curvesTmp;
			SolidUtil::Convert::SubEntityToCurveVector(curvesTmp, *lowsub);
			if (curvesTmp != NULL)
			{
				DraftingElementSchema::ToElement(DownLowFace, *curvesTmp, nullptr, eeh.GetModelRef()->Is3d(), *eeh.GetModelRef());
				if (DownLowFace.IsValid())
				{
					;
				}
			}
			else
			{
				IGeometryPtr geom;
				SolidUtil::Convert::SubEntityToGeometry(geom, *lowsub, *eeh.GetModelRef());
				ISolidPrimitivePtr tmpPtr = geom->GetAsISolidPrimitive();
				if (tmpPtr != NULL)
				{

					if (SUCCESS == DraftingElementSchema::ToElement(DownLowFace, *tmpPtr, nullptr, *eeh.GetModelRef()))
					{
						;
					}
				}
			}
		}

	}
}

void STRainshedRebarAssembly::SetRebarData(vector<PIT::ConcreteRebar> const& vecData)     //用vector数组存每层钢筋的信息
{
	if (vecData.empty())
	{
		return;
	}
	if (vecData.size() != GetRebarLevelNum())
	{
		return;
	}

	for (size_t i = 0; i < vecData.size(); i++)
	{

		if (i < GetvecDir().size())
		{
			GetvecDir()[i] = vecData[i].rebarDir;                       //方向
			GetvecDirSize()[i] = vecData[i].rebarSize;                   //钢筋尺寸
			GetvecRebarType()[i] = vecData[i].rebarType;                 //钢筋型号
			GetvecDirSpacing()[i] = vecData[i].spacing;                //钢筋间距
			GetvecStartOffset()[i] = vecData[i].startOffset;           //起点偏移
			GetvecEndOffset()[i] = vecData[i].endOffset;              //终点偏移
			GetvecLevelSpace()[i] = vecData[i].levelSpace;           //钢筋层间隔
		}
		else                                                    //减少
		{
			GetvecDir().push_back(vecData[i].rebarDir);
			GetvecDirSize().push_back(vecData[i].rebarSize);
			GetvecRebarType().push_back(vecData[i].rebarType);
			GetvecDirSpacing().push_back(vecData[i].spacing);
			GetvecStartOffset().push_back(vecData[i].startOffset);
			GetvecEndOffset().push_back(vecData[i].endOffset);
			GetvecLevelSpace().push_back(vecData[i].levelSpace);
			GetvecSetId().push_back(0);
		}
	}

}

bool STRainshedRebarAssembly::GetFrontBackLineAndHighFace(ElementHandleCR eeh, EditElementHandle* pHighFace, vector<MSElementDescrP>& vec_line, vector<MSElementDescrP>& vec_Lowline,
	vector<DSegment3d>& vec_linefront, vector<DSegment3d>& vec_lineback, vector<DSegment3d>& vec_linefront_Down, vector<DSegment3d>& vec_lineback_Down, double* tHeight)
{
	DPoint3d ptBegin, ptOver;
	vector<DPoint3d> vecPoints;

	vector<MSElementDescrP> vec_line1;
	vector<MSElementDescrP> vec_line2;

	vector<MSElementDescrP> vec_line3;
	vector<MSElementDescrP> vec_line4;

	EditElementHandle eehHighFace;
	EditElementHandle eehLowFace;
	GetRainshedFace(eeh, eehHighFace, eehLowFace, tHeight);
	if (!ExtractFacesTool::GetTwoLineFromDownFace(eehHighFace, vec_line, vec_line1, vec_line2, ptBegin, ptOver, vecPoints, eeh.GetModelRef()))
		return false;

	if (!ExtractFacesTool::GetTwoLineFromDownFace(eehLowFace, vec_Lowline, vec_line3, vec_line4, ptBegin, ptOver, vecPoints, eeh.GetModelRef()))
		return false;

	for (MSElementDescrP ms : vec_Lowline)
	{
		mdlElmdscr_freeAll(&ms);
	}

	if (vec_line1.empty() || vec_line2.empty())
	{
		return false;
	}

	//	EditElementHandle eh(vec_line1[0], true, true, ACTIVEMODEL);
	//	eh.AddToModel();

	for (MSElementDescrP ms : vec_line1)
	{
		DPoint3d pt1[2];
		mdlLinear_extract(pt1, NULL, &ms->el, eeh.GetModelRef());
		DVec3d vec = pt1[1] - pt1[0];
		vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
		vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
		vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
		DVec3d vecX = DVec3d::From(1, 0, 0);
		vec.Normalize();
		if (vec.IsPerpendicularTo(vecX))
		{
			if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) < 0)
			{
				vec_linefront.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_linefront.push_back({ pt1[1],pt1[0] });
			}
		}
		else
		{
			if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) < 0)
			{
				vec_linefront.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_linefront.push_back({ pt1[1],pt1[0] });
			}
		}
		mdlElmdscr_freeAll(&ms);
	}

	for (MSElementDescrP ms : vec_line2)
	{
		DPoint3d pt1[2];
		mdlLinear_extract(pt1, NULL, &ms->el, eeh.GetModelRef());
		DVec3d vec = pt1[1] - pt1[0];
		vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
		vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
		vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
		DVec3d vecX = DVec3d::From(1, 0, 0);
		vec.Normalize();
		if (vec.IsPerpendicularTo(vecX))
		{
			if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) < 0)
			{
				vec_lineback.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_lineback.push_back({ pt1[1],pt1[0] });
			}
		}
		else
		{
			if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) < 0)
			{
				vec_lineback.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_lineback.push_back({ pt1[1],pt1[0] });
			}
		}
		mdlElmdscr_freeAll(&ms);
	}

	if (vec_line3.empty() || vec_line4.empty())
	{
		return false;
	}

	//	EditElementHandle eh(vec_line1[0], true, true, ACTIVEMODEL);
	//	eh.AddToModel();

	for (MSElementDescrP ms : vec_line3)
	{
		DPoint3d pt1[2];
		mdlLinear_extract(pt1, NULL, &ms->el, eeh.GetModelRef());
		DVec3d vec = pt1[1] - pt1[0];
		vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
		vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
		vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
		DVec3d vecX = DVec3d::From(1, 0, 0);
		vec.Normalize();
		if (vec.IsPerpendicularTo(vecX))
		{
			if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) < 0)
			{
				vec_linefront_Down.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_linefront_Down.push_back({ pt1[1],pt1[0] });
			}
		}
		else
		{
			if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) < 0)
			{
				vec_linefront_Down.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_linefront_Down.push_back({ pt1[1],pt1[0] });
			}
		}
		mdlElmdscr_freeAll(&ms);
	}

	for (MSElementDescrP ms : vec_line4)
	{
		DPoint3d pt1[2];
		mdlLinear_extract(pt1, NULL, &ms->el, eeh.GetModelRef());
		DVec3d vec = pt1[1] - pt1[0];
		vec.x = COMPARE_VALUES_EPS(abs(vec.x), 0, 10) == 0 ? 0 : vec.x;
		vec.y = COMPARE_VALUES_EPS(abs(vec.y), 0, 10) == 0 ? 0 : vec.y;
		vec.z = COMPARE_VALUES_EPS(abs(vec.z), 0, 10) == 0 ? 0 : vec.z;
		DVec3d vecX = DVec3d::From(1, 0, 0);
		vec.Normalize();
		if (vec.IsPerpendicularTo(vecX))
		{
			if (COMPARE_VALUES_EPS(pt1[0].y, pt1[1].y, 10) < 0)
			{
				vec_lineback_Down.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_lineback_Down.push_back({ pt1[1],pt1[0] });
			}
		}
		else
		{
			if (COMPARE_VALUES_EPS(pt1[0].x, pt1[1].x, 10) < 0)
			{
				vec_lineback_Down.push_back({ pt1[0],pt1[1] });
			}
			else
			{
				vec_lineback_Down.push_back({ pt1[1],pt1[0] });
			}
		}
		mdlElmdscr_freeAll(&ms);
	}
	return true;
}