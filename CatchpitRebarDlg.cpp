// CatchpitRebarDlg.cpp: 实现文件
//

#include "CatchpitRebarDlg.h"
#include "afxdialogex.h"
#include "_USTATION.h"
#include "resource.h"
#include "CommonFile.h"
#include "ConstantsDef.h"
#include "ElementAttribute.h"
#include "PITBimMSCEConvert.h"
#include "PITRebarCurve.h"
#include "ScanIntersectTool.h"
#include "FacesRebarAssembly.h"
#include "CModelTool.h"
#include "CSolidTool.h"
#include "CElementTool.h"

// CatchpitRebarDlg 对话框

IMPLEMENT_DYNAMIC(CatchpitRebarDlg, CDialogEx)

CatchpitRebarDlg::CatchpitRebarDlg(ElementHandleCR eh, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CatchpitRebar, pParent)
{
	_ehOld = eh;
	m_ConcreteId = 0;
}

CatchpitRebarDlg::~CatchpitRebarDlg()
{
}

void CatchpitRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_FacesRebar, m_tab);
	DDX_Control(pDX, IDC_STATIC_WALLNAME, m_static_wallname);
	DDX_Control(pDX, IDC_COMBO2, m_ComboSize);
	DDX_Control(pDX, IDC_COMBO3, m_ComboType);
	DDX_Control(pDX, IDC_EDIT1, m_EditSpace);
}

BOOL CatchpitRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	/*if (!_ehOld.IsValid())
	{
		_ehOld = _ehOlds[0];
	}*/
	ElementId contid = 0;
	GetElementXAttribute(_ehOld.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, _ehOld.GetModelRef());

	
	GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	//m_vecEndTypeData.clear();
	//GetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);

	/*m_PageMainRebar.SetHide(m_isHide);
	m_PageMainRebar.SetDlgType(m_FaceDlgType);*/
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	g_vecRebarData = m_vecRebarData;
	m_PageMainRebar.SetSelectfaces(m_selectfaces);
	if (0xFFFF != GetElementXAttribute(contid, sizeof(PIT::Concrete), m_Concrete, ConcreteXAttribute, ACTIVEMODEL))
	{
		if(m_Concrete.rebarLevelNum == 2 && m_Concrete.postiveCover > 1 && m_Concrete.sideCover > 1)
			m_PageMainRebar.SetConcreteData(m_Concrete);
	}
	//m_PageMainRebar.SavePrt(this);

	string elename, eletype;
	GetEleNameAndType(_ehOld.GetElementId(), _ehOld.GetModelRef(), elename, eletype);
	wall_name = elename;
	CString wallname(elename.c_str());
	wallname = L"墙名:" + wallname;
	m_static_wallname.SetWindowTextW(wallname);

	for each (auto var in g_listRebarSize)
		m_ComboSize.AddString(var);
	for each (auto var in g_listRebarType)
		m_ComboType.AddString(var);

	CString strRebarSize(m_WallSetInfo.rebarSize);
	if (strRebarSize.Find(L"mm") == -1)
		strRebarSize += "mm";
	int nIndex = m_ComboSize.FindStringExact(0, strRebarSize);
	m_ComboSize.SetCurSel(nIndex);//尺寸
	m_ComboType.SetCurSel(m_WallSetInfo.rebarType);//型号

	// TODO:  在此添加额外的初始化
	//为Tab Control增加两个页面
	m_tab.InsertItem(0, _T("主要配筋"));
	//创建两个对话框
	m_PageMainRebar.Create(IDD_DIALOG_Catchpit_MainRebar, &m_tab);
	m_PageMainRebar.SetSelectElement(_ehOld);

	//设定在Tab内显示的范围
	CRect rc;
	m_tab.GetClientRect(rc);
	rc.top += 20;
	rc.bottom -= 0;
	rc.left += 0;
	rc.right -= 0;
	m_PageMainRebar.MoveWindow(&rc);

	//把对话框对象指针保存起来
	pDialog[0] = &m_PageMainRebar;

	//显示初始页面
	pDialog[0]->ShowWindow(SW_SHOW);

	//保存当前选择
	m_CurSelTab = 0;

	g_ConcreteId = m_ConcreteId;

	//ACCConcrete wallACConcrete;
	//int ret = GetElementXAttribute(_ehOld.GetElementId(), sizeof(ACCConcrete), wallACConcrete, ConcreteCoverXAttribute, _ehOld.GetModelRef());
	//if (ret == SUCCESS)	//关联构件配筋时存储过数据,优先使用关联构件设置的保护层
	//{
	//	m_Concrete.postiveCover = wallACConcrete.postiveOrTopCover;
	//	m_Concrete.reverseCover = wallACConcrete.reverseOrBottomCover;
	//	m_Concrete.sideCover = wallACConcrete.sideCover;
	//}
	AnalyseEehFaces();
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


// 分析集水坑的面与面之间的关系，将配筋面与其所有的锚固面存储到map中一一对应
// @Add by tanjie, 2024.1.9
void CatchpitRebarDlg::AnalyseEehFaces()
{
	std::vector<EditElementHandle*> allFaces;//未填充的所有的面
	ExtractFacesTool::GetFaces(_ehOld, allFaces);

	if (allFaces.size() != 14)//标准的集水坑或双集水坑
	{
		if(allFaces.size() == 11)//标准的单集水坑
			m_CatchpitType = 0;
		else
			m_CatchpitType = 2;
		/*for (auto it : allFaces)
	{
		mdlElmdscr_add(it->GetElementDescrP());
	}*/
		DRange3d eehRange;//构件范围
		mdlElmdscr_computeRange(&eehRange.low, &eehRange.high, _ehOld.GetElementDescrCP(), nullptr);

		if (1/*m_CatchpitType == 2*/)//如果是双集水坑
		{
			DRange3d eeh_Range = eehRange;
			eeh_Range.high.z += 10;
			eeh_Range.low.x -= 20;
			eeh_Range.low.y -= 20;
			eeh_Range.high.x += 20;
			eeh_Range.high.y += 20;
			m_AllFloors =
				scan_elements_in_range(
					eeh_Range,
					[&](const ElementHandle &eh) -> bool {
				if (eh.GetElementId() == _ehOld.GetElementId())
				{
					// 过滤掉自己
					return false;
				}
				// 只需要板
				return is_Floor(eh);
			});

			m_ScanedFloors.clear();
			for (auto floorEh : m_AllFloors)//将扫描到的板的实体保存
			{
				EditElementHandle flooreeh(floorEh.GetElementId(), floorEh.GetModelRef());
				if (flooreeh.IsValid())
				{
					EditElementHandle * eeh = new EditElementHandle();
					eeh->Duplicate(flooreeh);
					ISolidKernelEntityPtr entityPtr;
					if (SolidUtil::Convert::ElementToBody(entityPtr, *eeh) == SUCCESS)
					{
						SolidUtil::Convert::BodyToElement(*eeh, *entityPtr, nullptr, *ACTIVEMODEL);
						eeh->GetElementDescrP();
						m_ScanedFloors.emplace_back(eeh);
					}
				}
			}

			for (auto it = m_AllFloors.begin(); it != m_AllFloors.end(); ++it)
			{
				DRange3d tmpRange;//构件范围
				mdlElmdscr_computeRange(&tmpRange.low, &tmpRange.high, it->GetElementDescrCP(), nullptr);
				double height = 0.0;
				/*求板的厚度，通过获取所有边，起始点向量得出Z方向的线的长度*/
				vector<MSElementDescrP> edges;
				edges.clear();
				EditElementHandle eeh_Floor(m_AllFloors.front(), ACTIVEMODEL);
				PITCommonTool::CElementTool::GetALLEdges(eeh_Floor, edges);
				for (auto tmpedge : edges)
				{
					EditElementHandle tmpeeh(tmpedge, false, false, ACTIVEMODEL);
					DPoint3d pt[2];
					mdlLinear_extract(pt, NULL, tmpeeh.GetElementP(), tmpeeh.GetModelRef());
					CVector3D vec_line(pt[1], pt[0]);
					vec_line.Normalize();
					double tmpHeight = -1;
					if (COMPARE_VALUES_EPS(abs(vec_line.DotProduct(CVector3D::kZaxis)), 1, 0.1) == 0)
					{
						tmpHeight = mdlVec_distance(&pt[0], &pt[1]);
						if (height < tmpHeight)
						{
							height = tmpHeight;
						}
					}
				}
				m_mapFloorAndHeight[&*it] = height;
			}
		}

		vector<EditElementHandle*> insideFaces;//内侧的五个面
		vector<EditElementHandle*> outsideFaces;//外侧的五个面
		vector<EditElementHandle*> upFace;//最上面的一个面

		for (auto it : allFaces)
		{
			DRange3d tmpRange;
			mdlElmdscr_computeRange(&tmpRange.low, &tmpRange.high, it->GetElementDescrCP(), nullptr);

			if (tmpRange.ZLength() < 35 * UOR_PER_MilliMeter && fabs(tmpRange.high.z - eehRange.high.z) < 35 * UOR_PER_MilliMeter)
			{
				upFace.emplace_back(it);
				continue;
			}
			/*if (((tmpRange.ZLength() + 300) < eehRange.ZLength() && tmpRange.ZLength() > 300) || ((tmpRange.low.z - 200) > eehRange.low.z && (tmpRange.high.z + 200) < eehRange.high.z))
			{
				insideFaces.emplace_back(it);
			}
			else
			{
				outsideFaces.emplace_back(it);
			}*/
			if (COMPARE_VALUES_EPS(tmpRange.low.z, eehRange.low.z, 300) == 0)
			{
				outsideFaces.emplace_back(it);
			}
			else
			{
				insideFaces.emplace_back(it);
			}
		}
		/*for (auto it : insideFaces)
		{
			mdlElmdscr_add(it->GetElementDescrP());
		}*/
		//map< EditElementHandle*, vector<EditElementHandle*>> map_PlaneWith_VerticalPlanes;
		vector<EditElementHandle*> vec_Vertical_Plane;
		vec_Vertical_Plane.clear();
		for (auto it : insideFaces)
		{
			DPoint3d ptTmp;
			CVector3D vec;
			mdlElmdscr_extractNormal(&vec, &ptTmp, it->GetElementDescrCP(), NULL);
			vec.Normalize();
			for (auto otherit : insideFaces)
			{
				DPoint3d otherptTmp;
				CVector3D othervec;
				mdlElmdscr_extractNormal(&othervec, &otherptTmp, otherit->GetElementDescrCP(), NULL);
				othervec.Normalize();
				int number = mdlIntersect_allBetweenElms(nullptr, nullptr, 0, it->GetElementDescrP(), otherit->GetElementDescrP(), nullptr, 1);
				if (fabs(vec.DotProduct(othervec)) < 0.1 && number > 0)
				{
					vec_Vertical_Plane.emplace_back(otherit);
				}
			}
			DRange3d tmpRange;
			mdlElmdscr_computeRange(&tmpRange.low, &tmpRange.high, it->GetElementDescrCP(), nullptr);
			if (tmpRange.ZLength() > 300)
				vec_Vertical_Plane.emplace_back(upFace.front());
			m_map_PlaneWith_VerticalPlanes[it] = vec_Vertical_Plane;
			vec_Vertical_Plane.clear();
		}
		for (auto it : outsideFaces)
		{
			DPoint3d ptTmp;
			CVector3D vec;
			mdlElmdscr_extractNormal(&vec, &ptTmp, it->GetElementDescrCP(), NULL);
			vec.Normalize();
			DRange3d tmpRange1;
			mdlElmdscr_computeRange(&tmpRange1.low, &tmpRange1.high, it->GetElementDescrCP(), nullptr);
			for (auto otherit : outsideFaces)
			{
				DRange3d otherRange;
				mdlElmdscr_computeRange(&otherRange.low, &otherRange.high, otherit->GetElementDescrCP(), nullptr);
				DPoint3d otherptTmp;
				CVector3D othervec;
				mdlElmdscr_extractNormal(&othervec, &otherptTmp, otherit->GetElementDescrCP(), NULL);
				othervec.Normalize();
				int number = mdlIntersect_allBetweenElms(nullptr, nullptr, 0, it->GetElementDescrP(), otherit->GetElementDescrP(), nullptr, 1);
				if (fabs(vec.DotProduct(othervec)) < 0.1 && number > 0)
				{
					vec_Vertical_Plane.emplace_back(otherit);
				}
			}
			DRange3d tmpRange;
			mdlElmdscr_computeRange(&tmpRange.low, &tmpRange.high, it->GetElementDescrCP(), nullptr);
			if (tmpRange.ZLength() > 300)
				vec_Vertical_Plane.emplace_back(upFace.front());
			m_map_PlaneWith_VerticalPlanes[it] = vec_Vertical_Plane;
			vec_Vertical_Plane.clear();
		}
		//TEST
		//for (auto it : m_map_PlaneWith_VerticalPlanes)
		//{
		//	//SolidBoolOperation()
		//	DRange3d testRange;
		//	mdlElmdscr_computeRange(&testRange.low, &testRange.high, it.first->GetElementDescrCP(), nullptr);
		//	if (COMPARE_VALUES_EPS(-2297000,testRange.low.x,200) == 0 && COMPARE_VALUES_EPS(-2297000, testRange.high.x, 200) == 0)
		//	{
		//		mdlElmdscr_add(it.first->GetElementDescrP());
		//		for (auto secondit : it.second)
		//		{
		//			mdlElmdscr_add(secondit->GetElementDescrP());
		//		}
		//	}
		//}
		//TEST
		//auto it = m_map_PlaneWith_VerticalPlanes.begin();
		////it->first
		//mdlElmdscr_add(it->first->GetElementDescrP());
		//for (auto i : it->second)
		//{
		//	mdlElmdscr_add(i->GetElementDescrP());
		//}
	}
	else//BGAC 2BGC1003DB特殊的集水坑
	{
		m_CatchpitType = 1;
		/*for (auto it : allFaces)
		{
			mdlElmdscr_add(it->GetElementDescrP());
		}*/
		DRange3d eehRange;//构件范围
		mdlElmdscr_computeRange(&eehRange.low, &eehRange.high, _ehOld.GetElementDescrCP(), nullptr);

		/*for (auto it : allFaces)
		{
			DRange3d tmpRange;
			mdlElmdscr_computeRange(&tmpRange.low, &tmpRange.high, it->GetElementDescrCP(), nullptr);

			DPoint3d ptTmp;
			CVector3D vec;
			CVector3D zDir = CVector3D::From(0, 0, 1);
			mdlElmdscr_extractNormal(&vec, &ptTmp, it->GetElementDescrCP(), NULL);

			if (abs(vec.DotProduct(zDir)) > 0.9 && tmpRange.XLength() < 650 * UOR_PER_MilliMeter)
			{
				if (vec.DotProduct(zDir) < 0)
					vec.Negate();
				EditElementHandle eehSolid;
				ISolidKernelEntityPtr ptarget;
				SolidUtil::Convert::ElementToBody(ptarget, *it, true, true, true);
				if (SUCCESS == SolidUtil::Modify::ThickenSheet(ptarget, 601.0 * UOR_PER_MilliMeter, 0 * UOR_PER_MilliMeter))
				{
					if (SUCCESS == SolidUtil::Convert::BodyToElement(eehSolid, *ptarget, NULL, *ACTIVEMODEL))
					{
						MSElementDescrP mainDP = eehSolid.GetElementDescrP();
						vector<MSElementDescrP> boolDP;
						boolDP.emplace_back(const_cast<MSElementDescrP>(_ehOld.GetElementDescrCP()));
						PITCommonTool::CSolidTool::SolidBoolOperation(mainDP, boolDP,
							BOOLOPERATION::BOOLOPERATION_UNITE, ACTIVEMODEL);
						EditElementHandle eehcombimSolid(mainDP, true, false, ACTIVEMODEL);
						eehcombimSolid.AddToModel();
					}
				}
				else
					break;
			}


		}*/
		vector<EditElementHandle*> insideFaces;//内侧的五个面
		vector<EditElementHandle*> outsideFaces;//外侧的五个面
		vector<EditElementHandle*> upFace;//最上面的一个面
		vector<EditElementHandle*> secondUpFace;//第二高的顶面

		for (auto it : allFaces)
		{
			DRange3d tmpRange;
			mdlElmdscr_computeRange(&tmpRange.low, &tmpRange.high, it->GetElementDescrCP(), nullptr);

			DPoint3d ptTmp;
			CVector3D vec;
			mdlElmdscr_extractNormal(&vec, &ptTmp, it->GetElementDescrCP(), NULL);
			CVector3D xdir = CVector3D::From(1, 0, 0);
			if (abs(xdir.DotProduct(vec)) > 0.9)
			{
				if (tmpRange.YLength() < 650 * UOR_PER_MilliMeter || tmpRange.ZLength() < 650 * UOR_PER_MilliMeter)
					continue;
			}
			
			if (tmpRange.ZLength() < 20 * UOR_PER_MilliMeter && fabs(tmpRange.high.z - eehRange.high.z) < 20 * UOR_PER_MilliMeter)
			{
				upFace.emplace_back(it);
				continue;
			}
			if (tmpRange.ZLength() < 20 * UOR_PER_MilliMeter && tmpRange.XLength() < 650 * UOR_PER_MilliMeter)
			{
				secondUpFace.emplace_back(it);
				continue;
			}
			if (COMPARE_VALUES_EPS(tmpRange.low.z,eehRange.low.z,300) == 0)
			{
				outsideFaces.emplace_back(it);
			}
			else
			{
				insideFaces.emplace_back(it);
			}
		}
		/*for (auto it : insideFaces)
		{
			mdlElmdscr_add(it->GetElementDescrP());
		}*/
		//map< EditElementHandle*, vector<EditElementHandle*>> map_PlaneWith_VerticalPlanes;
		vector<EditElementHandle*> vec_Vertical_Plane;
		vec_Vertical_Plane.clear();
		for (auto it : insideFaces)
		{
			DPoint3d ptTmp;
			CVector3D vec;
			mdlElmdscr_extractNormal(&vec, &ptTmp, it->GetElementDescrCP(), NULL);
			vec.Normalize();
			for (auto otherit : insideFaces)
			{
				DPoint3d otherptTmp;
				CVector3D othervec;
				mdlElmdscr_extractNormal(&othervec, &otherptTmp, otherit->GetElementDescrCP(), NULL);
				othervec.Normalize();
				if (fabs(vec.DotProduct(othervec)) < 0.1)
				{
					vec_Vertical_Plane.emplace_back(otherit);
				}
			}
			DRange3d tmpRange;
			mdlElmdscr_computeRange(&tmpRange.low, &tmpRange.high, it->GetElementDescrCP(), nullptr);
			if (tmpRange.ZLength() > 1210 * UOR_PER_MilliMeter)
				vec_Vertical_Plane.emplace_back(upFace.front());
			else if(tmpRange.ZLength() > 300 && tmpRange.ZLength() < 1210 * UOR_PER_MilliMeter)
			{
				vec_Vertical_Plane.emplace_back(secondUpFace.front());
			}
			m_map_PlaneWith_VerticalPlanes[it] = vec_Vertical_Plane;
			vec_Vertical_Plane.clear();
		}
		for (auto it : outsideFaces)
		{
			DPoint3d ptTmp;
			CVector3D vec;
			mdlElmdscr_extractNormal(&vec, &ptTmp, it->GetElementDescrCP(), NULL);
			vec.Normalize();
			for (auto otherit : outsideFaces)
			{
				DPoint3d otherptTmp;
				CVector3D othervec;
				mdlElmdscr_extractNormal(&othervec, &otherptTmp, otherit->GetElementDescrCP(), NULL);
				othervec.Normalize();
				if (fabs(vec.DotProduct(othervec)) < 0.1)
				{
					vec_Vertical_Plane.emplace_back(otherit);
				}
			}
			DRange3d tmpRange;
			mdlElmdscr_computeRange(&tmpRange.low, &tmpRange.high, it->GetElementDescrCP(), nullptr);
			if (tmpRange.ZLength() > 1820 * UOR_PER_MilliMeter)
				vec_Vertical_Plane.emplace_back(upFace.front());
			else if(tmpRange.ZLength() > 300 && tmpRange.ZLength() < 1820 * UOR_PER_MilliMeter)
				vec_Vertical_Plane.emplace_back(secondUpFace.front());
			m_map_PlaneWith_VerticalPlanes[it] = vec_Vertical_Plane;
			vec_Vertical_Plane.clear();
		}

		//auto it = m_map_PlaneWith_VerticalPlanes.begin();
		////it->first
		//mdlElmdscr_add(it->first->GetElementDescrP());
		//for (auto i : it->second)
		//{
		//	mdlElmdscr_add(i->GetElementDescrP());
		//}
	}
	

}

void CatchpitRebarDlg::MakeFacesRebar(ElementId & contid, EditElementHandleR eeh, DgnModelRefP modelRef)
{
	FacesRebarAssembly*  faceRebar = NULL;
	int iIndex = 0;
	MSElementDescrP edpResult = nullptr;
	std::vector<MSElementDescrP> VecFaceEdp;
	vector<ElementRefP> vctAllLines;

	MSDialog *oBar = mdlDialog_completionBarOpen(TXT_PDMSIMPORTING2);
	mdlDialog_completionBarUpdateEx(oBar, (WChar*)TXT_PDMSIMPORTING2, 0);
	for (auto itPlane = m_map_PlaneWith_VerticalPlanes.begin(); itPlane != m_map_PlaneWith_VerticalPlanes.end(); ++itPlane)
	{
		
		mdlDialog_completionBarUpdateEx(oBar, (WChar*)TXT_PDMSIMPORTING2, (iIndex + 1.0) / m_map_PlaneWith_VerticalPlanes.size() * 100);
		
		MSElementDescrP tmpFace = nullptr;
		mdlElmdscr_duplicate(&tmpFace, itPlane->first->GetElementDescrP());

		EditElementHandle eehFace(tmpFace,true,false,ACTIVEMODEL);
		if (!eehFace.IsValid())
		{
			continue;
		}

		CVector3D ptNormal;
		DPoint3d otherptTmp;
		mdlElmdscr_extractNormal(&ptNormal, &otherptTmp, eehFace.GetElementDescrCP(), NULL);

		//DVec3d ptNormal;
		DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
		//mdlElmdscr_extractNormal(&ptNormal, nullptr, eehFace.GetElementDescrCP(), &ptDefault);
		//MSElementDescrP newFace = nullptr;

		//if (/*ptDefault.IsParallelTo(ptNormal)*/abs(ptNormal.DotProduct(ptDefault)) > 0.1)//改为只要不是竖直面就用板的求法
		//{
		//	ExtractFacesTool::GetOutLineFace(eehFace.GetElementDescrP(), newFace);
		//}
		//else
		//{
		//	ExtractFacesTool::GetFaceByHoleSubtractFace(eehFace.GetElementDescrP(), newFace);
		//}
		//if (newFace == nullptr)
		//{
		//	return;
		//}
		/*if (m_Concrete.isHandleHole == 0 && newFace != nullptr)
		{*/
		//mdlElmdscr_add(newFace);
		//eehFace.ReplaceElementDescr(newFace);//不管是否孔洞规避都补全面
	//}

		//EditElementHandle newFaceEeh(newFace, true, false, ACTIVEMODEL);
		//FacesRebarAssembly::FaceType faceType = FacesRebarAssembly::JudgeFaceType(eehFace, modelRef);
		FacesRebarAssembly::FaceType faceType = FacesRebarAssembly::Plane;
		switch (faceType)
		{
		case FacesRebarAssembly::other:
		case FacesRebarAssembly::Plane:
			faceRebar = REA::Create<PlaneRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());

			break;
		case FacesRebarAssembly::CamberedSurface:
			faceRebar = REA::Create<CamberedSurfaceRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
			break;
		default:
			return;
		}
		{
			if(fabs(ptDefault.DotProduct(ptNormal)) > 0.9)
			//if (ptDefault.IsParallelTo(ptNormal))
			{
				faceRebar->m_UseXOYDir = true;
			}
		}
		/*bool bIsSumps = this->m_PageMainRebar.GetIsSumps();
		if (FacePreviewButtonsDown)
		{
			faceRebar->m_allLines.clear();
		}*/
		CVector3D vecTmp = ptNormal;// m_PageMainRebar.GetvecFaceNormal(iIndex);
		vecTmp.Negate();
		DVec3d dvec = DVec3d::From(vecTmp.x, vecTmp.y, vecTmp.z);
		if (1/*m_CatchpitType == 2*/)
		{
			faceRebar->m_AllFloors = m_AllFloors;
			//faceRebar->m_UpFloor_Height = m_UpFloor_Height;
			faceRebar->m_ScanedFloors = m_ScanedFloors;
			faceRebar->m_mapFloorAndHeight = m_mapFloorAndHeight;
		}
		faceRebar->m_CatchpitType = m_CatchpitType;
		faceRebar->m_isCatchpit = true;
		faceRebar->m_bisSump = true;
		faceRebar->m_face = eehFace;
		faceRebar->m_Solid = &eeh;
		faceRebar->SetfaceType(faceType);
		faceRebar->SetfaceNormal(dvec);
		faceRebar->AnalyzingFaceGeometricData(eehFace);
		faceRebar->AnalyzingFloorData(eeh);
		faceRebar->SetConcrete(m_Concrete);
		faceRebar->SetMainRebars(m_vecRebarData);
		faceRebar->SetCurentFace(itPlane->first);
		faceRebar->SetVerticalPlanes(itPlane->second);
		faceRebar->SetRebarEndTypes(m_vecEndTypeData);
		faceRebar->MakeRebars(modelRef);
		faceRebar->Save(modelRef); // must save after creating rebars
		contid = faceRebar->FetchConcrete();
		iIndex++;
		//std::copy(faceRebar->m_allLines.begin(), faceRebar->m_allLines.end(), std::back_inserter(vctAllLines));//将多个面的画线依次存起来，最后赋值给m_FaceRebarPtr->m_AllLines
		//if (newFace != nullptr)
		//{
		//	mdlElmdscr_freeAll(&newFace);
		//	newFace = nullptr;
		//}
	}
	mdlDialog_completionBarUpdateEx(oBar, (WChar*)TXT_PDMSIMPORTING2, 100);
	mdlDialog_completionBarClose(oBar);
	
	if (faceRebar != nullptr)
	{
		SetElementXAttribute(contid, faceRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
	}

	
}

std::vector<ElementHandle> CatchpitRebarDlg::scan_elements_in_range(const DRange3d & range, std::function<bool(const ElementHandle&)> filter)
{
	std::vector<ElementHandle> ehs;

	std::map<ElementId, IComponent *> components;
	std::map<ElementId, MSElementDescrP> descriptions;

	/// 扫描在包围盒范围内的构件
	if (!PITCommonTool::CModelTool::AnalysisModelGetElements(range, components, descriptions))
	{
		// 返回空的即可
		return ehs;
	}

	const auto PLUS_ID = 1000000;

	ReachableModelRefCollection modelRefCol = ISessionMgr::GetActiveDgnModelRefP()->GetReachableModelRefs();
	for (const auto &kv : descriptions)
	{
		if (kv.second == NULL) continue;
		auto id = kv.first;
		// 这里返回的id可能加了一个PlusID以防止重复
		if (id >= PLUS_ID)
		{
			id -= PLUS_ID;
		}
		//const auto component = kv.second;
		// const auto desc = kv.second;

		// 扫描所有的model_ref，找到该元素所在的model_ref

		for (DgnModelRefP modelRef : modelRefCol)
		{
			EditElementHandle tmpeeh;
			if (tmpeeh.FindByID(id, modelRef) == SUCCESS)
			{
				auto eh = ElementHandle(id, modelRef);
				if (filter(eh))
				{
					// 满足条件，加入到输出 
					ehs.push_back(std::move(eh));
				}
			}

		}
	}

	// 排序+去重
	std::sort(
		ehs.begin(), ehs.end(),
		[](const ElementHandle &eh_a, const ElementHandle &eh_b) -> bool
	{
		return eh_a.GetElementId() < eh_b.GetElementId();
	});

	auto new_end = std::unique(
		ehs.begin(), ehs.end(),
		[](const ElementHandle &eh_a, const ElementHandle &eh_b) -> bool
	{
		return eh_a.GetElementId() == eh_b.GetElementId();
	});

	return std::vector<ElementHandle>(ehs.begin(), new_end);
}

bool CatchpitRebarDlg::is_Floor(const ElementHandle & element)
{
	std::string _name, type;
	if (!GetEleNameAndType(const_cast<ElementHandleR>(element), _name, type))
	{
		return false;
	}
	auto result_pos = type.find("FLOOR");
	return result_pos != std::string::npos;
}


BEGIN_MESSAGE_MAP(CatchpitRebarDlg, CDialogEx)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CatchpitRebarDlg::OnCbnSelchangeCombo2)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CatchpitRebarDlg::OnCbnSelchangeCombo3)
	ON_EN_CHANGE(IDC_EDIT1, &CatchpitRebarDlg::OnEnChangeEdit1)
	ON_BN_CLICKED(IDCANCEL, &CatchpitRebarDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &CatchpitRebarDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CatchpitRebarDlg 消息处理程序


void CatchpitRebarDlg::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
	auto it = g_listRebarSize.begin();
	advance(it, m_ComboSize.GetCurSel());
	strcpy(m_WallSetInfo.rebarSize, CT2A(*it));
	BrString str = *it;
	if (str != L"")
	{
		if (str.Find(L"mm") != -1)
		{
			str.Replace(L"mm", L"");
		}
	}
	strcpy(m_WallSetInfo.rebarSize, CT2A(str));
	m_PageMainRebar.ChangeRebarSizedata(m_WallSetInfo.rebarSize);
	m_PageMainRebar.UpdateRebarList();
}


void CatchpitRebarDlg::OnCbnSelchangeCombo3()
{
	// TODO: 在此添加控件通知处理程序代码
	auto it = g_listRebarType.begin();
	advance(it, m_ComboType.GetCurSel());
	m_WallSetInfo.rebarType = m_ComboType.GetCurSel();
	m_PageMainRebar.ChangeRebarTypedata(m_WallSetInfo.rebarType);
	m_PageMainRebar.UpdateRebarList();
}


void CatchpitRebarDlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString	strTemp = CString();
	m_EditSpace.GetWindowText(strTemp);
	m_WallSetInfo.spacing = atof(CT2A(strTemp));

	m_PageMainRebar.ChangeRebarSpacedata(m_WallSetInfo.spacing);
	m_PageMainRebar.UpdateRebarList();
}


void CatchpitRebarDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}


void CatchpitRebarDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = _ehOld.GetModelRef();
	EditElementHandle eeh(_ehOld, _ehOld.GetModelRef());

	m_Concrete = m_PageMainRebar.GetConcreteData();
	m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);


	/***********************************给sizekey附加型号******************************************************/
	auto it = m_vecRebarData.begin();
	for (; it != m_vecRebarData.end(); it++)
	{
		BrString strRebarSize = it->rebarSize;
		if (strRebarSize != L"")
		{
			if (strRebarSize.Find(L"mm") != -1)
			{
				strRebarSize.Replace(L"mm", L"");
			}
		}
		else if (XmlManager::s_alltypes.size() > 0)
		{
			strRebarSize = XmlManager::s_alltypes[it->rebarType];
		}
		strcpy(it->rebarSize, CT2A(strRebarSize));
		GetDiameterAddType(it->rebarSize, it->rebarType);
	}
	/***********************************给sizekey附加型号******************************************************/

	ElementId testid = 0;
	GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
	vector<double> vecDis;

	ElementId contid;

	// 单面或多面配筋
	MakeFacesRebar(contid, eeh, modelRef);

	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);

	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	auto it2 = m_vecRebarData.begin();
	for (; it2 != m_vecRebarData.end(); it2++)
	{
		BrString strRebarSize = it2->rebarSize;
		strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
		strcpy(it2->rebarSize, CT2A(strRebarSize));
	}
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/

	SetConcreteXAttribute(contid, ACTIVEMODEL);
	SetElementXAttribute(contid, sizeof(PIT::Concrete), &m_Concrete, ConcreteXAttribute, ACTIVEMODEL);
	//SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);+
	//SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(_ehOld.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, _ehOld.GetModelRef());
	SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
	ElementId unionId = -1;
	GetElementXAttribute(_ehOld.GetElementId(), sizeof(ElementId), unionId, UnionWallIDXAttribute, _ehOld.GetModelRef());
	if (unionId != -1)
	{
		SetElementXAttribute(unionId, sizeof(ElementId), &contid, ConcreteIDXAttribute, ACTIVEMODEL);
	}
}
