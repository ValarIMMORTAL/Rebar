
#include "_ustation.h"
#include "resource.h"
#include <SelectionRebar.h>
#include "GalleryIntelligentRebar.h"
#include "WallRebarAssembly.h"
#include "WallRebarDlg.h"
#include "ExtractFacesTool.h"
#include "CWallRebarDlg.h"
#include "TieRebar.h"
#include "ElementAttribute.h"
#include "PITMSCECommon.h"
#include "ExtractFacesTool.h"
#include "XmlHelper.h"
#include "WallMergeFeature.h"
// 比较浮点数，a>b返回1，a<b返回-1，a=b返回0
//#define COMPARE_VALUES_EPS(a,b,eps) (((a) - (b) > eps) ? 1 : (((a) - (b) < -eps) ? -1 : 0))
//YJC—板墙合并配筋
/**
* @返回类型  bool 创建成功返回true,失败返回false
* @函数说明  创建STWALLS联合元素的底面并设置联合元素的孔洞与高度
* @函数参数  selectedElement 所有墙/板名称集合
* @函数参数  combinewall  创建成功后得到的联合指针
**/
bool CSjointboard(ElementAgendaR selectedElement, EditElementHandle& combinewall)
{
	bool bFlag = true;
	//EditElementHandle combinewall;
	EditElementHandle newFace;
	vector<EditElementHandle*> holes;
	EditElementHandle Eleeh, Eleeh1;
	EditElementHandle eehDownFace, eehDownFace1;
	EFT::GetSolidElementAndSolidHoles(selectedElement[0], Eleeh, holes);//取孔洞
	EFT::GetSolidElementAndSolidHoles(selectedElement[1], Eleeh1, holes);//取孔洞

	double heightCom = 0.0, heightCom1 = 0.00;
	EFT::GetDownFace(Eleeh, eehDownFace, &heightCom);//取出底面  eehDownFace
	EFT::GetDownFace(Eleeh1, eehDownFace1, &heightCom1);//取出底面  eehDownFace1

	DPoint3d minP = { 0 }, maxP = { 0 };//底面元素描述符中元素的范围的最小和最大坐标
	DPoint3d minP1 = { 0 }, maxP1 = { 0 };//底面元素描述符中元素的范围的最小和最大坐标
	mdlElmdscr_computeRange(&minP, &maxP, eehDownFace.GetElementDescrP(), NULL); // 计算指定元素描述符中元素的范围 
	mdlElmdscr_computeRange(&minP1, &maxP1, eehDownFace1.GetElementDescrP(), NULL);

	if (DRange3d::From(minP, maxP).IntersectsWith(DRange3d::From(minP1, maxP1)))//计算两个面是否相交
	{
		bFlag = true;
	}
	else
	{
		return false;
	}

	//取并集 
	CurveVectorPtr profileUnion;
	CurveVectorPtr profile1 = ICurvePathQuery::ElementToCurveVector(eehDownFace);//把EditElementHandle转换CurveVectorPtr
	CurveVectorPtr profile = ICurvePathQuery::ElementToCurveVector(eehDownFace1);//把EditElementHandle转换CurveVectorPtr

	if (profile1 != nullptr && profile != nullptr)
	{
		profileUnion = CurveVector::AreaUnion(*profile1, *profile);//把两个面合成一个面
	}

	if (profileUnion != NULL)
	{
		DraftingElementSchema::ToElement(newFace, *profileUnion, nullptr, true, *ACTIVEMODEL);//把CurveVectorPtr转换成EditElementHandle
	}
	if (newFace.IsValid()) // 与原实体相交(无孔洞)   判断newFace是否有用
	{
		//将原平面往法向方向拉伸为一个实体
		EditElementHandle eehSolid;
		string elename, eletype;
		string elename2, eletype2;
		ISolidKernelEntityPtr ptarget;
		SolidUtil::Convert::ElementToBody(ptarget, newFace, true, true, true);
		if (SUCCESS == SolidUtil::Modify::ThickenSheet(ptarget, heightCom, 0))//拉伸一个实体
		{
			if (SUCCESS == SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL))
			{
				//eehSolid.AddToModel();
				GetEleNameAndType(selectedElement[0].GetElementId(), selectedElement[0].GetModelRef(), elename, eletype);//获取   elename, eletype
				GetEleNameAndType(selectedElement[1].GetElementId(), selectedElement[1].GetModelRef(), elename2, eletype2);//获取   elename, eletype
				elename = elename + "," + elename2;
				eehSolid.AddToModel();//画一个eehSolid实体

				//把打孔的参数传过去  
				T_ChildElementToControlFlagsMap _childElementToControlFlagsMap;
				T_ControlFlagsVector flagVec1;
				flagVec1.push_back(false); // visible  
				flagVec1.push_back(false); // temporary
				flagVec1.push_back(false); // profile
				_childElementToControlFlagsMap[eehSolid.GetElementRef()] = flagVec1;

				for (int i = 0; i < holes.size(); ++i)
				{

					EditElementHandle* holeeeh = holes.at(i);
					EditElementHandle tholeeeh;
					tholeeeh.Duplicate(*holeeeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(holeeeh->GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(tholeeeh);
					tholeeeh.AddToModel();
					T_ControlFlagsVector flagVec2;
					flagVec2.push_back(false); // visible  
					flagVec2.push_back(false); // temporary
					flagVec2.push_back(false); // profile
					_childElementToControlFlagsMap[tholeeeh.GetElementRef()] = flagVec2;
				}
				SmartFeatureNodePtr _unioNode;
				if (SUCCESS != FeatureCreate::CreateDifferenceFeature(_unioNode))
				{
					bFlag = false;
				}
				if ((SUCCESS != SmartFeatureElement::CreateAndWriteSmartFeatureElement(combinewall, eehSolid, eehSolid.GetModelRef(), *_unioNode, _childElementToControlFlagsMap, true)))
				{
					bFlag = false;
				}
				//打孔这里结束
				SetElemGraphPDMSItemTypeValue(combinewall, elename, eletype, L"PDMS_Attributes", L"PDMS_Attributes", ACTIVEMODEL);
				//隐藏实体
				ElementRefP oldref = combinewall.GetElementRef();
				mdlElmdscr_setVisible(combinewall.GetElementDescrP(), false);
				combinewall.ReplaceInModel(oldref);
				//隐藏实体
			}
		}
	}
	return  bFlag;
}



/**
* @返回类型  bool 创建成功返回true,失败返回false
* @函数说明  创建STWALLS联合元素的底面并设置联合元素的孔洞与高度
* @函数参数  selectedElement 所有墙/板名称集合
* @函数参数  combinewall  创建成功后得到的联合指针
* @函数参数  high 墙的高度
* @函数参数  mark  选择那块墙为底面
**/
bool WallRebarDlg(ElementAgendaR selectedElement, EditElementHandle& combinewall, double high, bool mark)
{
	bool bFlag = true;
	//EditElementHandle combinewall;
	EditElementHandle newFace;
	vector<EditElementHandle*> holes;
	EditElementHandle Eleeh, Eleeh1;
	EditElementHandle eehDownFace;
	EFT::GetSolidElementAndSolidHoles(selectedElement[0], Eleeh, holes);//取孔洞
	EFT::GetSolidElementAndSolidHoles(selectedElement[1], Eleeh1, holes);//取孔洞

	double heightCom = 0.0, heightCom1 = 0.00;
	CurveVectorPtr profile;
	if (mark)
	{
		EFT::GetDownFace(Eleeh, eehDownFace, &heightCom);//取出底面  eehDownFace
	}
	else
	{
		EFT::GetDownFace(Eleeh1, eehDownFace, &heightCom1);//取出底面  eehDownFace1
	}


	if (eehDownFace.IsValid()) // 与原实体相交(无孔洞)   判断newFace是否有用
	{
		//将原平面往法向方向拉伸为一个实体
		EditElementHandle eehSolid;
		string elename, eletype;
		string elename2, eletype2;
		ISolidKernelEntityPtr ptarget;
		SolidUtil::Convert::ElementToBody(ptarget, eehDownFace, true, true, true);
		if (SUCCESS == SolidUtil::Modify::ThickenSheet(ptarget, 0, high))//拉伸一个实体
		{
			if (SUCCESS == SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL))//ISolidKernelEntityPtr转EditElementHandle
				//eehSolid.AddToModel();
				GetEleNameAndType(selectedElement[0].GetElementId(), selectedElement[0].GetModelRef(), elename, eletype);//获取   elename, eletype
			    GetEleNameAndType(selectedElement[1].GetElementId(), selectedElement[1].GetModelRef(), elename2, eletype2);
				elename = elename + "," + elename2;
			{
				eehSolid.AddToModel();//画一个eehSolid实体
				//把打孔的参数传过去  
				T_ChildElementToControlFlagsMap _childElementToControlFlagsMap;
				T_ControlFlagsVector flagVec1;
				flagVec1.push_back(false); // visible  
				flagVec1.push_back(false); // temporary
				flagVec1.push_back(false); // profile
				_childElementToControlFlagsMap[eehSolid.GetElementRef()] = flagVec1;

				for (int i = 0; i < holes.size(); ++i)
				{

					EditElementHandle* holeeeh = holes.at(i);
					EditElementHandle tholeeeh;
					tholeeeh.Duplicate(*holeeeh);
					ElementCopyContext copier2(ACTIVEMODEL);
					copier2.SetSourceModelRef(holeeeh->GetModelRef());
					copier2.SetTransformToDestination(true);
					copier2.SetWriteElements(false);
					copier2.DoCopy(tholeeeh);
					tholeeeh.AddToModel();
					T_ControlFlagsVector flagVec2;
					flagVec2.push_back(false); // visible  
					flagVec2.push_back(false); // temporary
					flagVec2.push_back(false); // profile
					_childElementToControlFlagsMap[tholeeeh.GetElementRef()] = flagVec2;
				}
				SmartFeatureNodePtr _unioNode;
				if (SUCCESS != FeatureCreate::CreateDifferenceFeature(_unioNode))
				{
					bFlag = false;
				}
				if ((SUCCESS != SmartFeatureElement::CreateAndWriteSmartFeatureElement(combinewall, eehSolid, eehSolid.GetModelRef(), *_unioNode, _childElementToControlFlagsMap, true)))
				{
					bFlag = false;
				}
				//打孔这里结束
				SetElemGraphPDMSItemTypeValue(combinewall, elename, eletype, L"PDMS_Attributes", L"PDMS_Attributes", ACTIVEMODEL);
				//隐藏实体
				ElementRefP oldref = combinewall.GetElementRef();
				mdlElmdscr_setVisible(combinewall.GetElementDescrP(), false);
				combinewall.ReplaceInModel(oldref);
				//隐藏实体
			}
		}
	}
	return  bFlag;
}

/**
* @返回类型  bool 创建成功返回true,失败返回false
* @函数说明  创建GWALL弧形墙
* @函数参数  selectedElement 所有墙/板名称集合
* @函数参数  combinewall  创建成功后得到的联合指针
**/
bool ArcWallMerge(ElementAgendaR selectedElement, EditElementHandle& combinewall)
{
	ArcWallRebarAssembly SWX;
	PIT::ArcSegment arcFront[2], arcBack[2];
	SWX.AnalyzingWallGeometricDataARC(selectedElement[0], arcFront[0], arcBack[0]);//返回墙弧的 圆心 弧长  半径。。。
	SWX.AnalyzingWallGeometricDataARC(selectedElement[1], arcFront[1], arcBack[1]);//返回墙弧的 圆心 弧长  半径.。。
	double perimeter, perimeter1;
	perimeter = 3.141592653589*2.0*arcBack[0].dRadius;
	perimeter1 = arcBack[0].dLen + arcBack[1].dLen;
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	int judge = COMPARE_VALUES_EPS(arcFront[0].ptCenter.x, arcFront[1].ptCenter.x, uor_per_mm);
	int judge1 = COMPARE_VALUES_EPS(arcFront[0].ptCenter.y, arcFront[1].ptCenter.y, uor_per_mm);
	int judge2 = COMPARE_VALUES_EPS(arcFront[0].ptCenter.z, arcFront[1].ptCenter.z, uor_per_mm);

	if (!COMPARE_VALUES_EPS(perimeter, perimeter1, uor_per_mm) && judge == 0 && judge1 == 0 && judge2 == 0)
	{
		EditElementHandle eehDownFace;
		double heightCom;
		EFT::GetDownFace(selectedElement[0], eehDownFace, &heightCom);//取出墙的高度heightComs
		PITI::ELLWallGeometryInfo m_ELLWallData;
		m_ELLWallData.dRadiusOut = arcBack[0].dRadius;
		m_ELLWallData.dRadiusInn = arcFront[0].dRadius;
		m_ELLWallData.dHeight = heightCom;
		m_ELLWallData.centerpt = arcBack[0].ptCenter;
		m_ELLWallData.ArcDPs[0] = arcBack[0].ptStart;
		m_ELLWallData.ArcDPs[1] = arcBack[0].ptStart;
		m_ELLWallData.type = PITI::ELLIPSEWall;

		CSjointboard(selectedElement, combinewall);

		SetElementXAttribute(combinewall.GetElementId(), sizeof(m_ELLWallData), &m_ELLWallData, RoundrebarGroup, ACTIVEMODEL);//把数据保存进去
		return true;
	}
	else
	{
		return false;
	}
}