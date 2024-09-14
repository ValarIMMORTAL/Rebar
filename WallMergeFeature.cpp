
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
// �Ƚϸ�������a>b����1��a<b����-1��a=b����0
//#define COMPARE_VALUES_EPS(a,b,eps) (((a) - (b) > eps) ? 1 : (((a) - (b) < -eps) ? -1 : 0))
//YJC����ǽ�ϲ����
/**
* @��������  bool �����ɹ�����true,ʧ�ܷ���false
* @����˵��  ����STWALLS����Ԫ�صĵ��沢��������Ԫ�صĿ׶���߶�
* @��������  selectedElement ����ǽ/�����Ƽ���
* @��������  combinewall  �����ɹ���õ�������ָ��
**/
bool CSjointboard(ElementAgendaR selectedElement, EditElementHandle& combinewall)
{
	bool bFlag = true;
	//EditElementHandle combinewall;
	EditElementHandle newFace;
	vector<EditElementHandle*> holes;
	EditElementHandle Eleeh, Eleeh1;
	EditElementHandle eehDownFace, eehDownFace1;
	EFT::GetSolidElementAndSolidHoles(selectedElement[0], Eleeh, holes);//ȡ�׶�
	EFT::GetSolidElementAndSolidHoles(selectedElement[1], Eleeh1, holes);//ȡ�׶�

	double heightCom = 0.0, heightCom1 = 0.00;
	EFT::GetDownFace(Eleeh, eehDownFace, &heightCom);//ȡ������  eehDownFace
	EFT::GetDownFace(Eleeh1, eehDownFace1, &heightCom1);//ȡ������  eehDownFace1

	DPoint3d minP = { 0 }, maxP = { 0 };//����Ԫ����������Ԫ�صķ�Χ����С���������
	DPoint3d minP1 = { 0 }, maxP1 = { 0 };//����Ԫ����������Ԫ�صķ�Χ����С���������
	mdlElmdscr_computeRange(&minP, &maxP, eehDownFace.GetElementDescrP(), NULL); // ����ָ��Ԫ����������Ԫ�صķ�Χ 
	mdlElmdscr_computeRange(&minP1, &maxP1, eehDownFace1.GetElementDescrP(), NULL);

	if (DRange3d::From(minP, maxP).IntersectsWith(DRange3d::From(minP1, maxP1)))//�����������Ƿ��ཻ
	{
		bFlag = true;
	}
	else
	{
		return false;
	}

	//ȡ���� 
	CurveVectorPtr profileUnion;
	CurveVectorPtr profile1 = ICurvePathQuery::ElementToCurveVector(eehDownFace);//��EditElementHandleת��CurveVectorPtr
	CurveVectorPtr profile = ICurvePathQuery::ElementToCurveVector(eehDownFace1);//��EditElementHandleת��CurveVectorPtr

	if (profile1 != nullptr && profile != nullptr)
	{
		profileUnion = CurveVector::AreaUnion(*profile1, *profile);//��������ϳ�һ����
	}

	if (profileUnion != NULL)
	{
		DraftingElementSchema::ToElement(newFace, *profileUnion, nullptr, true, *ACTIVEMODEL);//��CurveVectorPtrת����EditElementHandle
	}
	if (newFace.IsValid()) // ��ԭʵ���ཻ(�޿׶�)   �ж�newFace�Ƿ�����
	{
		//��ԭƽ��������������Ϊһ��ʵ��
		EditElementHandle eehSolid;
		string elename, eletype;
		string elename2, eletype2;
		ISolidKernelEntityPtr ptarget;
		SolidUtil::Convert::ElementToBody(ptarget, newFace, true, true, true);
		if (SUCCESS == SolidUtil::Modify::ThickenSheet(ptarget, heightCom, 0))//����һ��ʵ��
		{
			if (SUCCESS == SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL))
			{
				//eehSolid.AddToModel();
				GetEleNameAndType(selectedElement[0].GetElementId(), selectedElement[0].GetModelRef(), elename, eletype);//��ȡ   elename, eletype
				GetEleNameAndType(selectedElement[1].GetElementId(), selectedElement[1].GetModelRef(), elename2, eletype2);//��ȡ   elename, eletype
				elename = elename + "," + elename2;
				eehSolid.AddToModel();//��һ��eehSolidʵ��

				//�Ѵ�׵Ĳ�������ȥ  
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
				//����������
				SetElemGraphPDMSItemTypeValue(combinewall, elename, eletype, L"PDMS_Attributes", L"PDMS_Attributes", ACTIVEMODEL);
				//����ʵ��
				ElementRefP oldref = combinewall.GetElementRef();
				mdlElmdscr_setVisible(combinewall.GetElementDescrP(), false);
				combinewall.ReplaceInModel(oldref);
				//����ʵ��
			}
		}
	}
	return  bFlag;
}



/**
* @��������  bool �����ɹ�����true,ʧ�ܷ���false
* @����˵��  ����STWALLS����Ԫ�صĵ��沢��������Ԫ�صĿ׶���߶�
* @��������  selectedElement ����ǽ/�����Ƽ���
* @��������  combinewall  �����ɹ���õ�������ָ��
* @��������  high ǽ�ĸ߶�
* @��������  mark  ѡ���ǿ�ǽΪ����
**/
bool WallRebarDlg(ElementAgendaR selectedElement, EditElementHandle& combinewall, double high, bool mark)
{
	bool bFlag = true;
	//EditElementHandle combinewall;
	EditElementHandle newFace;
	vector<EditElementHandle*> holes;
	EditElementHandle Eleeh, Eleeh1;
	EditElementHandle eehDownFace;
	EFT::GetSolidElementAndSolidHoles(selectedElement[0], Eleeh, holes);//ȡ�׶�
	EFT::GetSolidElementAndSolidHoles(selectedElement[1], Eleeh1, holes);//ȡ�׶�

	double heightCom = 0.0, heightCom1 = 0.00;
	CurveVectorPtr profile;
	if (mark)
	{
		EFT::GetDownFace(Eleeh, eehDownFace, &heightCom);//ȡ������  eehDownFace
	}
	else
	{
		EFT::GetDownFace(Eleeh1, eehDownFace, &heightCom1);//ȡ������  eehDownFace1
	}


	if (eehDownFace.IsValid()) // ��ԭʵ���ཻ(�޿׶�)   �ж�newFace�Ƿ�����
	{
		//��ԭƽ��������������Ϊһ��ʵ��
		EditElementHandle eehSolid;
		string elename, eletype;
		string elename2, eletype2;
		ISolidKernelEntityPtr ptarget;
		SolidUtil::Convert::ElementToBody(ptarget, eehDownFace, true, true, true);
		if (SUCCESS == SolidUtil::Modify::ThickenSheet(ptarget, 0, high))//����һ��ʵ��
		{
			if (SUCCESS == SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL))//ISolidKernelEntityPtrתEditElementHandle
				//eehSolid.AddToModel();
				GetEleNameAndType(selectedElement[0].GetElementId(), selectedElement[0].GetModelRef(), elename, eletype);//��ȡ   elename, eletype
			    GetEleNameAndType(selectedElement[1].GetElementId(), selectedElement[1].GetModelRef(), elename2, eletype2);
				elename = elename + "," + elename2;
			{
				eehSolid.AddToModel();//��һ��eehSolidʵ��
				//�Ѵ�׵Ĳ�������ȥ  
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
				//����������
				SetElemGraphPDMSItemTypeValue(combinewall, elename, eletype, L"PDMS_Attributes", L"PDMS_Attributes", ACTIVEMODEL);
				//����ʵ��
				ElementRefP oldref = combinewall.GetElementRef();
				mdlElmdscr_setVisible(combinewall.GetElementDescrP(), false);
				combinewall.ReplaceInModel(oldref);
				//����ʵ��
			}
		}
	}
	return  bFlag;
}

/**
* @��������  bool �����ɹ�����true,ʧ�ܷ���false
* @����˵��  ����GWALL����ǽ
* @��������  selectedElement ����ǽ/�����Ƽ���
* @��������  combinewall  �����ɹ���õ�������ָ��
**/
bool ArcWallMerge(ElementAgendaR selectedElement, EditElementHandle& combinewall)
{
	ArcWallRebarAssembly SWX;
	PIT::ArcSegment arcFront[2], arcBack[2];
	SWX.AnalyzingWallGeometricDataARC(selectedElement[0], arcFront[0], arcBack[0]);//����ǽ���� Բ�� ����  �뾶������
	SWX.AnalyzingWallGeometricDataARC(selectedElement[1], arcFront[1], arcBack[1]);//����ǽ���� Բ�� ����  �뾶.����
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
		EFT::GetDownFace(selectedElement[0], eehDownFace, &heightCom);//ȡ��ǽ�ĸ߶�heightComs
		PITI::ELLWallGeometryInfo m_ELLWallData;
		m_ELLWallData.dRadiusOut = arcBack[0].dRadius;
		m_ELLWallData.dRadiusInn = arcFront[0].dRadius;
		m_ELLWallData.dHeight = heightCom;
		m_ELLWallData.centerpt = arcBack[0].ptCenter;
		m_ELLWallData.ArcDPs[0] = arcBack[0].ptStart;
		m_ELLWallData.ArcDPs[1] = arcBack[0].ptStart;
		m_ELLWallData.type = PITI::ELLIPSEWall;

		CSjointboard(selectedElement, combinewall);

		SetElementXAttribute(combinewall.GetElementId(), sizeof(m_ELLWallData), &m_ELLWallData, RoundrebarGroup, ACTIVEMODEL);//�����ݱ����ȥ
		return true;
	}
	else
	{
		return false;
	}
}