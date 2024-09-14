// CFacesRebarDlg.cpp: 实现文件
//
#pragma once
#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CFacesRebarDlg.h"

#include <CFaceTool.h>

#include "afxdialogex.h"
#include "resource.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "CommonFile.h"
#include "ElementAttribute.h"
#include "FacesRebarAssembly.h"

#include "ExtractFacesTool.h"
#include "ConstantsDef.h"
#include "ScanIntersectTool.h"
#include "SelectFaceTool.h"


// CFacesRebarDlg 对话框

bool FacePreviewButtonsDown = false;//面配筋界面的预览按钮

IMPLEMENT_DYNAMIC(CFacesRebarDlg, CDialogEx)

CFacesRebarDlg::CFacesRebarDlg(ElementHandleCR ehOld, ElementId ehnew, const bvector<ISubEntityPtr>& faces, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_FacesRebar, pParent), _ehOld(ehOld), _ehNew(ehnew), m_selectfaces(faces), m_ConcreteId(0)
{
	m_FaceRebarPtr = NULL;
	m_vecRebarData.clear();
	m_vecRebarData.shrink_to_fit();
	m_vecEndTypeData.clear();
	m_vecEndTypeData.shrink_to_fit();

	m_isHide = false;

	m_WallSetInfo.rebarType = 2;
}

CFacesRebarDlg::CFacesRebarDlg(const vector<ElementHandle>& ehOld, std::vector<vector<EditElementHandle*> >& faces, 
	const map<ElementId, EleFace>& eleFaces, CWnd * pParent)
	: CDialogEx(IDD_DIALOG_FacesRebar, pParent), _ehOlds(ehOld), m_ConcreteId(0), m_eleFaces(eleFaces)
{
	m_FaceRebarPtr = NULL;
	m_vecRebarData.clear();
	m_vecRebarData.shrink_to_fit();
	m_vecEndTypeData.clear();
	m_vecEndTypeData.shrink_to_fit();
	if (_ehOlds.size() > 0)
	{
		_ehOld = _ehOlds[0];
	}
	//区分选择模型中的上下面
	m_vvecDownFace.clear();
	m_vvecUpFace.clear();
	vector<vector<EditElementHandle*> > vvecUpDownFace;
	map<ElementId, vector<EditElementHandle*>> eleUpDownFaces;
	map<ElementId, size_t> eleDownIdx;
	vector<size_t> vecDownFaceIndex;
	for (auto it : eleFaces)
	{
		DPoint3d ptInFace;
		DPoint3d ptNormal;
		DPoint3d ptX = DPoint3d::From(1, 0, 0);
		DPoint3d ptY = DPoint3d::From(0, 1, 0);
		DPoint3d ptZ = DPoint3d::From(0, 0, 1);
		
		vector<EditElementHandle*> vecUpDownFace;
		size_t downFaceIndex = 0;
		int minZ = 429496000;
		vector<EditElementHandle*> faceEehs = it.second.faces;
		for (size_t j = 0; j < faceEehs.size();++j)
		{
			//mdlElmdscr_addByModelRef(faces[i][j]->GetElementDescrP(), ACTIVEMODEL);
			mdlElmdscr_extractNormal(&ptNormal, &ptInFace, faceEehs[j]->GetElementDescrCP(), &ptZ);
			if (ptNormal.IsParallelTo(ptX) || ptNormal.IsParallelTo(ptY))
			{

// 				delete faces[i][j];
// 				faces[i][j] = nullptr;
				continue;
			}

			double areap = 0;
			mdlMeasure_areaProperties(nullptr, &areap, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, faceEehs[j]->GetElementDescrP(), 0);
			double minarea = MINFLOORAREA;
			if (areap < minarea)
			{
				continue;
			}

			if (ptNormal.IsParallelTo(ptZ))
			{
				vecUpDownFace.push_back(faceEehs[j]);

				if ((int)ptInFace.z < minZ)
				{
					minZ = (int)ptInFace.z;
					downFaceIndex = vecUpDownFace.size() - 1;
				}
			}
		}

		vvecUpDownFace.push_back(vecUpDownFace);
		vecDownFaceIndex.push_back(downFaceIndex);
		eleUpDownFaces[it.first] = vecUpDownFace;
		eleDownIdx[it.first] = downFaceIndex;
	}

	for (size_t i = 0; i < vvecUpDownFace.size(); ++i)
	{
		vector<EditElementHandle*> vecDownFace;
		vector<EditElementHandle*> vecUpFace;
		for (size_t j = 0; j < vvecUpDownFace[i].size(); ++j)
		{
			if (j == vecDownFaceIndex[i])
			{
				vecDownFace.push_back(vvecUpDownFace[i][j]);
			}
			else
			{
				vecUpFace.push_back(vvecUpDownFace[i][j]);
			}

		}
		m_vvecDownFace.push_back(vecDownFace);
		m_vvecUpFace.push_back(vecUpFace);
	}

	for (auto it : eleUpDownFaces)
	{
		vector<EditElementHandle*> vecDownFace;
		vector<EditElementHandle*> vecUpFace;
		size_t idx = eleDownIdx.find(it.first)->second;
		for (size_t i = 0; i < it.second.size(); ++i)
		{
			if (i == idx)
			{
				vecDownFace.push_back(it.second[i]);
			}
			else
			{
				vecUpFace.push_back(it.second[i]);
			}
		}
		m_eleDownFaces[it.first]= vecDownFace;
		m_eleUpFaces[it.first] = vecUpFace;
	}
}

CFacesRebarDlg::~CFacesRebarDlg()
{

// 	for (size_t i = 0; i < m_vvecUpFace.size(); ++i)
// 	{
// 		for (size_t j = 0; j < m_vvecUpFace[i].size(); ++j)
// 		{
// 			delete m_vvecUpFace[i][j];
// 			m_vvecUpFace[i][j] = nullptr;
// 		}
// 	}
// 
// 	for (size_t i = 0; i < m_vvecDownFace.size(); ++i)
// 	{
// 		for (size_t j = 0; j < m_vvecDownFace[i].size(); ++j)
// 		{
// 			delete m_vvecDownFace[i][j];
// 			m_vvecDownFace[i][j] = nullptr;
// 		}
// 	}
}

void CFacesRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_FacesRebar, m_tab);
	DDX_Control(pDX, IDC_COMBO2, m_ComboSize);
	DDX_Control(pDX, IDC_COMBO3, m_ComboType);
	DDX_Control(pDX, IDC_EDIT1, m_EditSpace);
	DDX_Control(pDX, IDC_STATIC_WALLNAME, m_static_wallname);
}


BEGIN_MESSAGE_MAP(CFacesRebarDlg, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_FacesRebar, &CFacesRebarDlg::OnTcnSelchangeTabFacerebar)
	ON_BN_CLICKED(IDOK, &CFacesRebarDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CFacesRebarDlg::OnBnClickedCancel)
	ON_EN_CHANGE(IDC_EDIT1, &CFacesRebarDlg::OnEnChangeEdit1)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CFacesRebarDlg::OnCbnSelchangeCombo3)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CFacesRebarDlg::OnCbnSelchangeCombo2)
	ON_STN_CLICKED(IDC_STATIC_WALLNAME, &CFacesRebarDlg::OnStnClickedStaticWallname)
	ON_BN_CLICKED(IDC_BUTTON1, &CFacesRebarDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CWallRebar 消息处理程序


BOOL CFacesRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	if (m_FaceDlgType == 1)
	{
		this->SetWindowText(_T("多板联合配筋"));
	}

	if (!_ehOld.IsValid())
	{
		_ehOld = _ehOlds[0];
	}
	ElementId contid = 0;
	GetElementXAttribute(_ehOld.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, _ehOld.GetModelRef());

	GetElementXAttribute(contid, sizeof(PIT::Concrete), m_Concrete, ConcreteXAttribute, ACTIVEMODEL);
	GetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	m_vecEndTypeData.clear();
	//GetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);

	m_PageMainRebar.SetHide(m_isHide);
	m_PageMainRebar.SetDlgType(m_FaceDlgType);
	m_PageMainRebar.SetListRowData(m_vecRebarData);
	g_vecRebarData = m_vecRebarData;
	m_PageMainRebar.SetSelectfaces(m_selectfaces);

	m_PageMainRebar.SavePrt(this);

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
	m_PageMainRebar.Create(IDD_DIALOG_FacesRebar_MainRebar, &m_tab);
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

	ACCConcrete wallACConcrete;
	int ret = GetElementXAttribute(_ehOld.GetElementId(), sizeof(ACCConcrete), wallACConcrete, ConcreteCoverXAttribute, _ehOld.GetModelRef());
	if (ret == SUCCESS)	//关联构件配筋时存储过数据,优先使用关联构件设置的保护层
	{
		m_Concrete.postiveCover = wallACConcrete.postiveOrTopCover;
		m_Concrete.reverseCover = wallACConcrete.reverseOrBottomCover;
		m_Concrete.sideCover = wallACConcrete.sideCover;
	}
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CFacesRebarDlg::OnTcnSelchangeTabFacerebar(NMHDR *pNMHDR, LRESULT *pResult)
{
	//// TODO: 在此添加控件通知处理程序代码
	////把当前的页面隐藏起来
	//pDialog[m_CurSelTab]->ShowWindow(SW_HIDE);
	////得到新的页面索引
	//m_CurSelTab = m_tab.GetCurSel();

	//switch (m_CurSelTab)
	//{
	//case 0:
	//{
	//	m_PageEndType.m_ListEndType.GetAllRebarData(m_vecEndTypeData);
	//	m_PageEndType.SetListRowData(m_vecEndTypeData);
	//	m_PageMainRebar.UpdateRebarList();
	//}
	//break;
	//case 1:
	//{
	//	m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	//	m_PageMainRebar.SetListRowData(m_vecRebarData);
	//	m_Concrete = m_PageMainRebar.GetConcreteData();
	//	m_PageEndType.SetRearLevelNum(m_Concrete.rebarLevelNum);
	//	m_PageEndType.m_vecRebarData = m_vecRebarData;
	//	m_PageEndType.UpdateEndTypeList();
	//}
	//break;
	//default:
	//	break;
	//}
	////把新的页面显示出来
	//pDialog[m_CurSelTab]->ShowWindow(SW_SHOW);
	//*pResult = 0;
}

void CFacesRebarDlg::multiSlabOrWallRebar(EditElementHandleR eeh, DgnModelRefP modelRef)
{
	vector<vector<EditElementHandle*> > faces = m_Concrete.isSlabUpFaceUnionRebar ? m_vvecUpFace : m_vvecDownFace;

	//多个面进行合并
	//1.如果是顶面合并，找出z值最小的面，如果是底面，找出z值最大的面
	int minZ = 429496000;
	int maxZ = -429496000;
	DPoint3d ptInCompFace;
	DPoint3d ptCompFaceNormal;
	EditElementHandle *CompFace = nullptr;
	for (size_t i = 0; i < faces.size(); ++i)
	{
		for (size_t j = 0; j < faces[i].size(); ++j)
		{
			//mdlElmdscr_add(faces[i][j]->GetElementDescrP());
			DPoint3d ptInFace;
			DPoint3d ptNormal;
			DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
			mdlElmdscr_extractNormal(&ptNormal, &ptInFace, faces[i][j]->GetElementDescrCP(), &ptDefault);
			if (!ptNormal.IsParallelTo(ptDefault))
			{
				continue;
			}

			if (m_Concrete.isSlabUpFaceUnionRebar)
			{
				if ((int)ptInFace.z < minZ)
				{
					minZ = (int)ptInFace.z;
					CompFace = faces[i][j];
					ptInCompFace = ptInFace;
					ptCompFaceNormal = ptNormal;
				}
			}
			else
			{
				if ((int)ptInFace.z > maxZ)
				{
					maxZ = (int)ptInFace.z;
					CompFace = faces[i][j];
					ptInCompFace = ptInFace;
					ptCompFaceNormal = ptNormal;
				}
			}
		}

	}
	//2.将其他面都往该面偏移,并进行合并
	CurveVectorPtr profileUnion = ICurvePathQuery::ElementToCurveVector(*CompFace);

	// 需要配置的新面，把多个面合并成一个整体的面
	EditElementHandle newFace;
	// end
	for (size_t i = 0; i < faces.size(); ++i)
	{
		for (size_t j = 0; j < faces[i].size(); ++j)
		{
			if (faces[i][j] != CompFace)
			{
				DPoint3d ptInFace;
				DVec3d ptNormal;
				DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
				mdlElmdscr_extractNormal(&ptNormal, &ptInFace, faces[i][j]->GetElementDescrCP(), &ptDefault);

				if (!ptDefault.IsParallelTo(ptNormal))
				{
					continue;
				}

				DPoint3d ptInCmpFacePro;
				mdlVec_projectPointToPlane(&ptInCmpFacePro, &ptInCompFace, &ptInFace, &ptNormal);
				DPoint3d ptTrans = ptInCompFace - ptInCmpFacePro;
				Transform trans;
				mdlTMatrix_getIdentity(&trans);
				mdlTMatrix_setTranslation(&trans, &ptTrans);
				faces[i][j]->GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(*faces[i][j], (TransformInfo)trans);

				//取并集
				CurveVectorPtr profile = ICurvePathQuery::ElementToCurveVector(*faces[i][j]);
				if (profileUnion != nullptr && profile != nullptr)
				{
					profileUnion = CurveVector::AreaUnion(*profileUnion, *profile);
				}
			}
		}
	}
	if (profileUnion != NULL)
	{
		DraftingElementSchema::ToElement(newFace, *profileUnion, nullptr, true, *ACTIVEMODEL);
	}
	newFace.AddToModel();
	FacesRebarAssembly*  faceRebar = REA::Create<PlaneRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
	faceRebar->m_face = newFace;
	if (m_vvecUpFace.empty() && m_vvecDownFace.empty())
	{
		faceRebar->m_Solid = &eeh;
	}
	else
	{
		faceRebar->m_vecElm = _ehOlds;
		faceRebar->m_slabUpFace = m_Concrete.isSlabUpFaceUnionRebar;
	}

	if (FacePreviewButtonsDown)
	{
		faceRebar->m_allLines.clear();//先清空所有画线
	}
	mdlElmdscr_extractNormal(&faceRebar->PopfaceNormal(), NULL, newFace.GetElementDescrCP(), NULL);
	faceRebar->PopfaceNormal().Negate();
	faceRebar->AnalyzingFaceGeometricData(newFace);
	faceRebar->SetConcrete(m_Concrete);
	faceRebar->SetMainRebars(m_vecRebarData);
	faceRebar->SetRebarEndTypes(m_vecEndTypeData);
	faceRebar->MakeRebars(modelRef);
	faceRebar->Save(modelRef); // must save after creating rebars
	newFace.DeleteFromModel();
	ElementId contid = faceRebar->FetchConcrete();

	if (FacePreviewButtonsDown)//预览标志
	{
		m_FaceRebarPtr = faceRebar;
	}

	EditElementHandle eeh2(contid, ACTIVEMODEL);
	ElementRefP oldRef = eeh2.GetElementRef();
	mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
	eeh2.ReplaceInModel(oldRef);
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	auto it = m_vecRebarData.begin();
	for (; it != m_vecRebarData.end(); it++)
	{
		BrString strRebarSize = it->rebarSize;
		strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
		strcpy(it->rebarSize, CT2A(strRebarSize));
	}
	/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
	SetConcreteXAttribute(contid, ACTIVEMODEL);
	SetElementXAttribute(contid, sizeof(PIT::WallRebarInfo), &m_Concrete, WallRebarInfoXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
	SetElementXAttribute(_ehOld.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, _ehOld.GetModelRef());
	SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
}

void CFacesRebarDlg::multiDiffThickSlabOrWallRebar(EditElementHandleR eeh, DgnModelRefP modelRef)
{
	map<ElementId, vector<EditElementHandle*>> faces = m_Concrete.isSlabUpFaceUnionRebar ? m_eleUpFaces : m_eleDownFaces;
	
	//面信息
	struct FaceInfo{
		int z = 0;								//面的z值
		EditElementHandle *faceEeh = nullptr;	//面
		DPoint3d ptInFace = {0,0,0};			//面上原点
		DPoint3d faceNormal = {0,0,0};			//面的法向量
		ElementId eleId = 0;					//面对应的板的元素id
	};
	map<int, vector<FaceInfo>> zFaces; //z与面集合的映射

	//1.按z给所有面分类
	for (auto it : faces)
	{
		for (size_t j = 0; j < it.second.size(); ++j)
		{
			FaceInfo face;

			DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
			mdlElmdscr_extractNormal(&face.faceNormal, &face.ptInFace, it.second[j]->GetElementDescrCP(), &ptDefault);
			if (!face.faceNormal.IsParallelTo(ptDefault))
			{
				continue;
			}
			face.z = face.ptInFace.z;
			face.faceEeh = it.second[j];
			
			vector<FaceInfo> faceInfos;
			auto findIt = zFaces.find(face.z);
			if (findIt != zFaces.end())
			{
				faceInfos = findIt->second;
			}
			face.eleId = it.first;
			faceInfos.push_back(face);		
			zFaces[face.z] = faceInfos;
		}

	}

	//2.合并同z值的面
	map<int, CurveVectorPtr> zUnionFaces; //z与合并面曲线映射
	for (auto it : zFaces)
	{
		vector<FaceInfo> curFaces = it.second;
		if (curFaces.size() == 0)
		{
			continue;
		}
		FaceInfo firstFace = curFaces.at(0);
		CurveVectorPtr profileUnion = ICurvePathQuery::ElementToCurveVector(*firstFace.faceEeh);
		//把多个面合并成一个整体的面
		for (size_t i = 1; i < curFaces.size(); ++i)
		{
			FaceInfo faceInfo = curFaces.at(i);
			DPoint3d ptDefault = DPoint3d::From(0, 0, 1);

			if (!ptDefault.IsParallelTo(faceInfo.faceNormal))
			{
				continue;
			}

			//取并集
			CurveVectorPtr profile = ICurvePathQuery::ElementToCurveVector(*faceInfo.faceEeh);
			if (profileUnion != nullptr && profile != nullptr)
			{
				profileUnion = CurveVector::AreaUnion(*profileUnion, *profile);
			}
		}
		if (profileUnion != nullptr)
		{
			zUnionFaces[it.first] = profileUnion;
		}
	}

	//3.减掉其它包含在本面内的不同z的面，最后得到的面用来配筋
	for (auto compIt : zUnionFaces)
	{
		EditElementHandle compEeh;
		DraftingElementSchema::ToElement(compEeh, *compIt.second, nullptr, true, *ACTIVEMODEL);
		DPoint3d ptInCompFace;
		DVec3d ptCompNormal;
		DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
		mdlElmdscr_extractNormal(&ptCompNormal, &ptInCompFace, compEeh.GetElementDescrCP(), &ptDefault);
		CurveVectorPtr profileDiffer = compIt.second;
		for (auto it : zUnionFaces)
		{
			if (it != compIt)
			{
				EditElementHandle itEeh;
				DraftingElementSchema::ToElement(itEeh, *it.second, nullptr, true, *ACTIVEMODEL);
				DPoint3d ptInFace;
				DVec3d ptNormal;
				mdlElmdscr_extractNormal(&ptNormal, &ptInFace, itEeh.GetElementDescrCP(), &ptDefault);

				//把其他面投影到原始面上
				DPoint3d ptInCmpFacePro;
				mdlVec_projectPointToPlane(&ptInCmpFacePro, &ptInFace, &ptInCompFace, &ptCompNormal);
				DPoint3d ptTrans = ptInCmpFacePro - ptInFace;
				Transform trans;
				mdlTMatrix_getIdentity(&trans);
				mdlTMatrix_setTranslation(&trans, &ptTrans);
				itEeh.GetHandler(MISSING_HANDLER_PERMISSION_Transform).ApplyTransform(itEeh, (TransformInfo)trans);
				//mdlElmdscr_addByModelRef(eeh.GetElementDescrP(), ACTIVEMODEL);

				CurveVectorPtr profile = ICurvePathQuery::ElementToCurveVector(itEeh);
				DRange3d compRange, itRange;
				profileDiffer->GetRange(compRange);
				profile->GetRange(itRange);
				if (!itRange.IsContained(compRange))
				{
					continue;
				}
				//求差
				CurveVectorPtr res = CurveVector::AreaDifference(*profileDiffer, *profile);
				if (res != nullptr)
				{
					profileDiffer = res;
				}
			}
		}

		//计算组成合并面的面所对应的板元素，以在后面计算各自的孔洞
		vector<FaceInfo> faceInfos = zFaces[compIt.first];
		vector<ElementHandle> ehs;
		for (auto faceIt : faceInfos)
		{
			ElementId eleId = faceIt.eleId;
			if (m_eleFaces.find(eleId) != m_eleFaces.end())
			{
				ehs.push_back(m_eleFaces[eleId].eeh);
			}
		}

		FacesRebarAssembly* faceRebar = REA::Create<PlaneRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		if (m_vvecUpFace.empty() && m_vvecDownFace.empty())
		{
			faceRebar->m_Solid = &eeh;
		}
		else
		{
			faceRebar->m_vecElm = ehs;
			faceRebar->m_slabUpFace = m_Concrete.isSlabUpFaceUnionRebar;
		}

		EditElementHandle newFace;
		if (profileDiffer != NULL)
		{
			DraftingElementSchema::ToElement(newFace, *profileDiffer, nullptr, true, *ACTIVEMODEL);
		}
		newFace.AddToModel();
		faceRebar->m_face = newFace;

		if (FacePreviewButtonsDown)
		{
			faceRebar->m_allLines.clear();//先清空所有画线
		}
		//mdlElmdscr_addByModelRef(newFace.GetElementDescrP(), ACTIVEMODEL);
		mdlElmdscr_extractNormal(&faceRebar->PopfaceNormal(), NULL, newFace.GetElementDescrCP(), NULL);
		faceRebar->PopfaceNormal().Negate();
		faceRebar->AnalyzingFaceGeometricData(newFace);
		faceRebar->SetConcrete(m_Concrete);
		faceRebar->SetMainRebars(m_vecRebarData);
		faceRebar->SetRebarEndTypes(m_vecEndTypeData);
		faceRebar->MakeRebars(modelRef);
		faceRebar->Save(modelRef); // must save after creating rebars
		newFace.DeleteFromModel();
		ElementId contid = faceRebar->FetchConcrete();

		if (FacePreviewButtonsDown)//预览标志
		{
			m_FaceRebarPtr = faceRebar;
		}

		EditElementHandle eeh2(contid, ACTIVEMODEL);
		ElementRefP oldRef = eeh2.GetElementRef();
		mdlElmdscr_setVisible(eeh2.GetElementDescrP(), false);
		eeh2.ReplaceInModel(oldRef);
		/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
		auto it = m_vecRebarData.begin();
		for (; it != m_vecRebarData.end(); it++)
		{
			BrString strRebarSize = it->rebarSize;
			strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 1);	//删掉型号
			strcpy(it->rebarSize, CT2A(strRebarSize));
		}
		/***********************************给sizekey去除型号再保存到模型中 ******************************************************/
		SetConcreteXAttribute(contid, ACTIVEMODEL);
		SetElementXAttribute(contid, sizeof(PIT::WallRebarInfo), &m_Concrete, WallRebarInfoXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
		SetElementXAttribute(_ehOld.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, _ehOld.GetModelRef());
		SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
	}
	
}

void CFacesRebarDlg::perpendicularFaceReabr(vector<double>& vecDis, ElementId& contid, EditElementHandleR eeh, DgnModelRefP modelRef)
{
	PlaneRebarAssembly*  faceRebar = NULL;
	vector<ElementRefP> vctAllLines;
	for (int i = 0; i < (int)m_selectfaces.size(); ++i)
	{
		EditElementHandle eehFace;
		if (!PIT::ConvertToElement::SubEntityToElement(eehFace, m_selectfaces[i], modelRef))
			continue;
		faceRebar = REA::Create<PlaneRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());
		MSElementDescrP newFace = nullptr;
		ExtractFacesTool::GetFaceByHoleSubtractFace(eehFace.GetElementDescrP(), newFace);
		if (m_Concrete.isHandleHole == 0 && newFace != nullptr)
		{
			eehFace.ReplaceElementDescr(newFace);
		}
		faceRebar->m_face = eehFace;
		faceRebar->m_Solid = &eeh;
		// 			bool bAnchor = 0 == i ? false : true;
		// 			faceRebar->SetAnchor(bAnchor);
		if (0 == (i & 1)) 
		{
			if (0 != i)
			{
				EditElementHandle eehFace2;
				if (PIT::ConvertToElement::SubEntityToElement(eehFace2, m_selectfaces[i - 1], modelRef))
				{
					MSElementDescrP tmpFace = nullptr;
					ExtractFacesTool::GetFaceByHoleSubtractFace(eehFace2.GetElementDescrP(), tmpFace);
					if (m_Concrete.isHandleHole == 0 && tmpFace != nullptr)
					{
						eehFace2.ReplaceElementDescr(tmpFace);
					}
					faceRebar->SetCrossPlanePre(eehFace2);
				}
			}

			if (i < m_selectfaces.size() - 1)
			{
				EditElementHandle eehFace2;
				if (PIT::ConvertToElement::SubEntityToElement(eehFace2, m_selectfaces[i + 1], modelRef))
				{
					MSElementDescrP tmpFace = nullptr;
					ExtractFacesTool::GetFaceByHoleSubtractFace(eehFace2.GetElementDescrP(), tmpFace);
					if (m_Concrete.isHandleHole == 0 && tmpFace != nullptr)
					{
						eehFace2.ReplaceElementDescr(tmpFace);
					}
					faceRebar->SetCrossPlaneNext(eehFace2);
				}
			}
		}

		if (FacePreviewButtonsDown)
		{
			faceRebar->m_allLines.clear();//先清空所有画线
		}
		CVector3D vecTmp = m_PageMainRebar.GetvecFaceNormal(i);
		DVec3d dvec = DVec3d::From(vecTmp.x, vecTmp.y, vecTmp.z);
		faceRebar->Setd(vecDis[i / 3]);
		faceRebar->SetfaceNormal(dvec);
		faceRebar->AnalyzingFaceGeometricData(eehFace);
		faceRebar->SetConcrete(m_Concrete);
		faceRebar->SetMainRebars(m_vecRebarData);
		faceRebar->SetRebarEndTypes(m_vecEndTypeData);
		faceRebar->MakeRebars(modelRef);
		faceRebar->Save(modelRef); // must save after creating rebars
		std::copy(faceRebar->m_allLines.begin(), faceRebar->m_allLines.end(), std::back_inserter(vctAllLines));//存储多个面的预览画线
	}
	contid = faceRebar->FetchConcrete();
	if (FacePreviewButtonsDown)
	{
		m_FaceRebarPtr = faceRebar;
		m_FaceRebarPtr->m_allLines = vctAllLines;
	}
}

// 单面或多面配筋
void CFacesRebarDlg::normalFaceRebar(ElementId& contid, EditElementHandleR eeh, DgnModelRefP modelRef)
{
	FacesRebarAssembly*  faceRebar = NULL;
	int iIndex = 0;
	MSElementDescrP edpResult = nullptr;
	std::vector<MSElementDescrP> VecFaceEdp;
	//合并面处理
	if (this->m_PageMainRebar.GetIsMergeFace() && m_selectfaces.size() > 1)
	{
		//是否勾选了合并面功能并且选择的面超过两个
		for (ISubEntityPtr face : m_selectfaces)
		{
			EditElementHandle eehFace;
			if (!PIT::ConvertToElement::SubEntityToElement(eehFace, face, modelRef))
			{				
				continue;
			}
			if (!eehFace.IsValid())
			{
				continue;
			}
			eehFace.GetElementDescrP();
			VecFaceEdp.push_back(eehFace.ExtractElementDescr());
		}
	}
	vector<ElementRefP> vctAllLines;
	if (VecFaceEdp.size() > 0 && ExtractFacesTool::CombineSlabFaces(edpResult, VecFaceEdp))
	{
		//次分支是合并面功能，之前遇到过一块板升有两个面，高度只相差一点点，所以用合并面功能将两个面合并
		EditElementHandle eehFace(edpResult,true,false, modelRef);
		DVec3d ptNormal;
		DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
		mdlElmdscr_extractNormal(&ptNormal, nullptr, eehFace.GetElementDescrCP(), &ptDefault);
		MSElementDescrP newFace = nullptr;

		if (/*ptDefault.IsParallelTo(ptNormal)*/abs(ptNormal.DotProduct(ptDefault)) > 0.1)//改为只要不是竖直面就用板的求法
		{
			ExtractFacesTool::GetOutLineFace(eehFace.GetElementDescrP(), newFace);
		}
		else
		{
			ExtractFacesTool::GetFaceByHoleSubtractFace(eehFace.GetElementDescrP(), newFace);
		}
		if (newFace == nullptr)
		{
			return;
		}
		if (m_Concrete.isHandleHole == 0 && newFace != nullptr)
		{
			//mdlElmdscr_add(newFace);
			eehFace.ReplaceElementDescr(newFace);
		}

		EditElementHandle newFaceEeh(newFace, true, false, ACTIVEMODEL);
		FacesRebarAssembly::FaceType faceType = FacesRebarAssembly::JudgeFaceType(newFaceEeh, modelRef);
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
			if (ptDefault.IsParallelTo(ptNormal))
			{
				faceRebar->m_UseXOYDir = true;
			}
		}

		bool bIsSumps = this->m_PageMainRebar.GetIsSumps();

		if (FacePreviewButtonsDown)
		{
			faceRebar->m_allLines.clear();
		}
		CVector3D vecTmp = m_PageMainRebar.GetvecFaceNormal(iIndex);
		DVec3d dvec = DVec3d::From(vecTmp.x, vecTmp.y, vecTmp.z); 
		faceRebar->m_bisSump = bIsSumps;
		faceRebar->m_face = eehFace;
		faceRebar->m_Solid = &eeh;
		faceRebar->SetfaceType(faceType);
		faceRebar->SetfaceNormal(dvec);
		faceRebar->AnalyzingFaceGeometricData(newFaceEeh);
		faceRebar->AnalyzingFloorData(eeh);
		faceRebar->SetConcrete(m_Concrete);
		faceRebar->SetMainRebars(m_vecRebarData);
		faceRebar->SetSelectedAnchorFace(m_faces);
		faceRebar->SetRebarEndTypes(m_vecEndTypeData);
		faceRebar->MakeRebars(modelRef);
		faceRebar->Save(modelRef); // must save after creating rebars
		contid = faceRebar->FetchConcrete();
		iIndex++;
		std::copy(faceRebar->m_allLines.begin(), faceRebar->m_allLines.end(), std::back_inserter(vctAllLines));//将多个面的画线依次存起来，最后赋值给m_FaceRebarPtr->m_AllLines
		if (newFace != nullptr)
		{
			mdlElmdscr_freeAll(&newFace);
			newFace = nullptr;
		}
	}
	else {
		MSDialog *oBar = mdlDialog_completionBarOpen(TXT_PDMSIMPORTING2);
		mdlDialog_completionBarUpdateEx(oBar, (WChar*)TXT_PDMSIMPORTING2, 0);
		for (ISubEntityPtr face : m_selectfaces)
		{
			mdlDialog_completionBarUpdateEx(oBar, (WChar*)TXT_PDMSIMPORTING2, (iIndex+1.0)/ m_selectfaces.size()*100);
			EditElementHandle eehFace;
			if (!PIT::ConvertToElement::SubEntityToElement(eehFace, face, modelRef))
			{
				iIndex++;
				continue;
			}

			if (!eehFace.IsValid())
			{
				continue;
			}
			if (m_selectfaces.size() > 1)
			{
				//多面互相锚固
				if (this->m_PageMainRebar.GetIsAnchorFace())
				{
					m_faces.clear();
					if (iIndex == 0 && m_selectfaces.size() > 1)
					{
						m_faces.push_back(m_selectfaces[1]);
					}
					else if (iIndex > 0 && iIndex < m_selectfaces.size() - 1)
					{
						m_faces.push_back(m_selectfaces[iIndex - 1]);
						m_faces.push_back(m_selectfaces[iIndex + 1]);
					}
					else if (iIndex != 0 && iIndex == m_selectfaces.size() - 1)
					{
						m_faces.push_back(m_selectfaces[iIndex - 1]);
					}
				}
			}
			
			

			
			DVec3d ptNormal;
			DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
			mdlElmdscr_extractNormal(&ptNormal, nullptr, eehFace.GetElementDescrCP(), &ptDefault);
			MSElementDescrP newFace = nullptr;

			if (/*ptDefault.IsParallelTo(ptNormal)*/abs(ptNormal.DotProduct(ptDefault)) > 0.1)//改为只要不是竖直面就用板的求法
			{
				ExtractFacesTool::GetOutLineFace(eehFace.GetElementDescrP(), newFace);
			}
			else
			{
				ExtractFacesTool::GetFaceByHoleSubtractFace(eehFace.GetElementDescrP(), newFace);
			}
			if (newFace == nullptr)
			{
				return;
			}
			/*if (m_Concrete.isHandleHole == 0 && newFace != nullptr)
			{*/
				//mdlElmdscr_add(newFace);
				eehFace.ReplaceElementDescr(newFace);//不管是否孔洞规避都补全面
			//}

			EditElementHandle newFaceEeh(newFace, true, false, ACTIVEMODEL);
			FacesRebarAssembly::FaceType faceType = FacesRebarAssembly::JudgeFaceType(newFaceEeh, modelRef);
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
				if (ptDefault.IsParallelTo(ptNormal))
				{
					faceRebar->m_UseXOYDir = true;
				}
			}
			bool bIsSumps = this->m_PageMainRebar.GetIsSumps();
			if (FacePreviewButtonsDown)
			{
				faceRebar->m_allLines.clear();
			}
			CVector3D vecTmp = m_PageMainRebar.GetvecFaceNormal(iIndex);
			DVec3d dvec = DVec3d::From(vecTmp.x, vecTmp.y, vecTmp.z);
			faceRebar->m_bisSump = bIsSumps;
			faceRebar->m_face = eehFace;
			faceRebar->m_Solid = &eeh;
			faceRebar->SetfaceType(faceType);
			faceRebar->SetfaceNormal(dvec);
			faceRebar->AnalyzingFaceGeometricData(newFaceEeh);
			faceRebar->AnalyzingFloorData(eeh);
			faceRebar->SetCurentFace(&newFaceEeh);
			faceRebar->SetConcrete(m_Concrete);
			faceRebar->SetMainRebars(m_vecRebarData);
			faceRebar->SetSelectedAnchorFace(m_faces);
			faceRebar->SetRebarEndTypes(m_vecEndTypeData);
			faceRebar->MakeRebars(modelRef);
			faceRebar->Save(modelRef); // must save after creating rebars
			contid = faceRebar->FetchConcrete();
			iIndex++;
			std::copy(faceRebar->m_allLines.begin(), faceRebar->m_allLines.end(), std::back_inserter(vctAllLines));//将多个面的画线依次存起来，最后赋值给m_FaceRebarPtr->m_AllLines
			if (newFace != nullptr)
			{
				mdlElmdscr_freeAll(&newFace);
				newFace = nullptr;
			}
			if(faceRebar->m_facePlace == 0) //内侧面
			{
				SetElementXAttribute(contid, m_vecRebarData, RebarInsideFace, ACTIVEMODEL);
			}
			else
			{
				SetElementXAttribute(contid, m_vecRebarData, RebarOutsideFace, ACTIVEMODEL);
			}
		}
		mdlDialog_completionBarUpdateEx(oBar, (WChar*)TXT_PDMSIMPORTING2, 100);
		mdlDialog_completionBarClose(oBar);
	}
	if (FacePreviewButtonsDown)
	{
		m_FaceRebarPtr = faceRebar;
		m_FaceRebarPtr->m_allLines = vctAllLines;
	}
	if (faceRebar!=nullptr)
	{
		SetElementXAttribute(contid, faceRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
	}
	
}

// 多面配筋 且 合并钢筋
void CFacesRebarDlg::multiFaceInlineRebar(ElementId& contid, EditElementHandleR eeh, DgnModelRefP modelRef)
{
	FacesRebarAssembly*  faceRebar = REA::Create<MultiPlaneRebarAssembly>(ACTIVEMODEL, eeh.GetElementId(), eeh.GetModelRef());

	int iIndex = 0;
	for (ISubEntityPtr face : m_selectfaces)
	{
		EditElementHandle eehFace;
		if (!PIT::ConvertToElement::SubEntityToElement(eehFace, face, modelRef))
		{
			iIndex++;
			continue;
		}

		MSElementDescrP faceDescr = nullptr;
		eehFace.GetElementDescrP()->Duplicate(&faceDescr);
		DVec3d ptNormal;
		DPoint3d ptDefault = DPoint3d::From(0, 0, 1);
		mdlElmdscr_extractNormal(&ptNormal, nullptr, faceDescr, &ptDefault);

		MSElementDescrP newFace = nullptr;
		if (abs(ptDefault.DotProduct(ptNormal)) > 0.9/*ptDefault.IsParallelTo(ptNormal)*/)
		{
			ExtractFacesTool::GetOutLineFace(faceDescr, newFace);
		}
		else
		{
			ExtractFacesTool::GetFaceByHoleSubtractFace(faceDescr, newFace);
		}
		if (newFace == nullptr)
		{
			continue;
		}
		if (m_Concrete.isHandleHole == 0 && newFace != nullptr)
		{
			//mdlElmdscr_add(newFace);
			eehFace.ReplaceElementDescr(newFace);
		}
		
		CVector3D vecTmp = m_PageMainRebar.GetvecFaceNormal(iIndex);
		DVec3d dvec = DVec3d::From(vecTmp.x, vecTmp.y, vecTmp.z);
		faceRebar->SetfaceNormal(dvec);
		faceRebar->m_Solid = &eeh;
		MultiPlaneRebarAssembly *multiFace = dynamic_cast<MultiPlaneRebarAssembly*>(faceRebar);
		if (multiFace != nullptr)
		{		
			EditElementHandle newEehFace;
			newEehFace.Duplicate(eehFace);
			multiFace->AddFace(newEehFace);
		}
		EditElementHandle newFaceEeh(newFace, true, false, ACTIVEMODEL);
		faceRebar->AnalyzingFaceGeometricData(newFaceEeh);
		iIndex++;
	}

	if (FacePreviewButtonsDown)
	{
		faceRebar->m_allLines.clear();//先清空所有画线
	}
	faceRebar->SetisReverse(m_PageMainRebar.GetIsReverse());
	faceRebar->SetConcrete(m_Concrete);
	faceRebar->SetMainRebars(m_vecRebarData);
	faceRebar->SetRebarEndTypes(m_vecEndTypeData);
	faceRebar->MakeRebars(modelRef);
	faceRebar->Save(modelRef); // must save after creating rebars
	contid = faceRebar->FetchConcrete();
	if (FacePreviewButtonsDown)
	{
		m_FaceRebarPtr = faceRebar;
	}
	SetElementXAttribute(contid, faceRebar->PopvecFrontPts(), FrontPtsXAttribute, ACTIVEMODEL);
}

void CFacesRebarDlg::OnBnClickedOk()
{
	CDialogEx::OnOK();
	// TODO: 在此添加控件通知处理程序代码
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
		else if(XmlManager::s_alltypes.size() > 0)
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
	/*std::vector<PIT::ConcreteRebar>			test_vecRebarData1;
	std::vector<PIT::ConcreteRebar>			test_vecRebarData2;
	GetElementXAttribute(testid, test_vecRebarData1, RebarInsideFace, ACTIVEMODEL);
	GetElementXAttribute(testid, test_vecRebarData2, RebarOutsideFace, ACTIVEMODEL);*/


	// 多面联合配筋 || 多板联合配筋
	if (!m_vvecUpFace.empty() || !m_vvecDownFace.empty())
	{
		//multiSlabOrWallRebar(eeh, modelRef);

		std::vector<PIT::ConcreteRebar> frontRebarDatas;
		std::vector<PIT::ConcreteRebar> backRebarDatas;
		for (auto it : m_vecRebarData)
		{
			if (it.datachange == 0)
			{
				frontRebarDatas.push_back(it);
			}
			if (it.datachange == 2)
			{
				backRebarDatas.push_back(it);
			}
		}
		if (frontRebarDatas.size() > 0)
		{
			m_Concrete.isSlabUpFaceUnionRebar = 1;
			m_Concrete.rebarLevelNum = frontRebarDatas.size();
			m_vecRebarData = frontRebarDatas;
			multiDiffThickSlabOrWallRebar(eeh, modelRef);
		}
		if (backRebarDatas.size() > 0)
		{
			m_Concrete.isSlabUpFaceUnionRebar = 0;
			m_Concrete.rebarLevelNum = backRebarDatas.size();
			m_vecRebarData = backRebarDatas;
			multiDiffThickSlabOrWallRebar(eeh, modelRef);
		}	
		if (m_FaceRebarPtr)
		{
			m_FaceRebarPtr->ClearLines();
		}
		return;
	}

	if (m_selectfaces.size() > 2)
	{
		EditElementHandle eehFace1;
		DPoint3d ptNormal1;
		DPoint3d ptInFace1;
		bool ret1 = PIT::ConvertToElement::SubEntityToElement(eehFace1, m_selectfaces[0], modelRef);
		mdlElmdscr_extractNormal(&ptNormal1, &ptInFace1, eehFace1.GetElementDescrCP(), NULL);
		for (int i = 1; i < (int)m_selectfaces.size() - 1; i += 2)
		{
			EditElementHandle eehFace2, eehFace3;
			bool ret2 = PIT::ConvertToElement::SubEntityToElement(eehFace2, m_selectfaces[i], modelRef);
			bool ret3 = PIT::ConvertToElement::SubEntityToElement(eehFace3, m_selectfaces[i + 1], modelRef);
			if (ret2 && ret3)
			{
				DPoint3d ptNormal2, ptNormal3;
				DPoint3d ptInFace2, ptInFace3;
				mdlElmdscr_extractNormal(&ptNormal2, &ptInFace2, eehFace2.GetElementDescrCP(), NULL);
				mdlElmdscr_extractNormal(&ptNormal3, &ptInFace3, eehFace3.GetElementDescrCP(), NULL);
				if (ptNormal1.IsPerpendicularTo(ptNormal2) && ptNormal1.IsParallelTo(ptNormal3))
				{
					//计算第三个面与第一个面的距离
					DPoint3d ptPro;
					mdlVec_projectPointToPlane(&ptPro, &ptInFace1, &ptInFace3, &ptNormal3);
					double dis = ptInFace1.Distance(ptPro);
					vecDis.push_back(dis);
				}

				ptNormal1 = ptNormal3;
				ptInFace1 = ptInFace3;
			}
		}
	}

	ElementId contid;
	//if (vecDis.size()) // 互相垂直面配筋（3个面及以上) 
	//{
	//	perpendicularFaceReabr(vecDis, contid, eeh, modelRef);
	//}
	//else
	{
		if (this->m_PageMainRebar.GetIsMergeRebar() && m_selectfaces.size() > 1)
		{
			// 多面配筋 且 合并钢筋
			multiFaceInlineRebar(contid, eeh, modelRef);
		}
		else
		{
			// 单面或多面配筋
			normalFaceRebar(contid, eeh, modelRef);
		}

	}
	
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
 	//SetElementXAttribute(contid, sizeof(PIT::WallRebarInfo), &m_Concrete, WallRebarInfoXAttribute, ACTIVEMODEL);
 	//SetElementXAttribute(contid, m_vecRebarData, vecRebarDataXAttribute, ACTIVEMODEL);
 	//SetElementXAttribute(contid, m_vecEndTypeData, vecEndTypeDataXAttribute, ACTIVEMODEL);
 	SetElementXAttribute(_ehOld.GetElementId(), sizeof(ElementId), &contid, ConcreteIDXAttribute, _ehOld.GetModelRef());
 	SetElementXAttribute(contid, g_vecRebarPtsNoHole, vecRebarPointsXAttribute, ACTIVEMODEL);
	ElementId unionId = -1;
	GetElementXAttribute(_ehOld.GetElementId(), sizeof(ElementId), unionId, UnionWallIDXAttribute, _ehOld.GetModelRef());
	if (unionId != -1)
	{
		SetElementXAttribute(unionId, sizeof(ElementId), &contid, ConcreteIDXAttribute, ACTIVEMODEL);
	}
	
	m_PageMainRebar.DeleteFaceLine();

	if (m_FaceRebarPtr)
	{
		m_FaceRebarPtr->ClearLines();
	}
	SelectFaceTool* tool = new SelectFaceTool(1, 1, _ehOld, _ehNew);
	tool->InstallTool();
}

void CFacesRebarDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	if (m_FaceRebarPtr)
	{
		m_FaceRebarPtr->ClearLines();
	}
	m_PageMainRebar.DeleteFaceLine();
}




void CFacesRebarDlg::PreviewRebarLines()
{
	FacePreviewButtonsDown = true;
	DgnModelRefP        modelRef = ACTIVEMODEL;
	DgnModelRefP        tmpModel = _ehOld.GetModelRef();
	EditElementHandle eeh(_ehOld, _ehOld.GetModelRef());

	m_Concrete = m_PageMainRebar.GetConcreteData();
	m_PageMainRebar.m_listMainRebar.GetAllRebarData(m_vecRebarData);
	m_PageMainRebar.SetListRowData(m_vecRebarData);


	/**********************给sizekey附加型号******************************/
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
		else
		{
			strRebarSize = XmlManager::s_alltypes[it->rebarType];
		}
		strcpy(it->rebarSize, CT2A(strRebarSize));
		GetDiameterAddType(it->rebarSize, it->rebarType);
	}
	/******************************给sizekey附加型号*****************************/

	ElementId testid = 0;
	GetElementXAttribute(eeh.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, eeh.GetModelRef());
	vector<double> vecDis;

	// 多面联合配筋 || 多板联合配筋
	if (!m_vvecUpFace.empty() || !m_vvecDownFace.empty())
	{
		//multiSlabOrWallRebar(eeh, modelRef);
		std::vector<PIT::ConcreteRebar> frontRebarDatas;
		std::vector<PIT::ConcreteRebar> backRebarDatas;
		for (auto it : m_vecRebarData)
		{
			if (it.datachange == 0)
			{
				frontRebarDatas.push_back(it);
			}
			if (it.datachange == 2)
			{
				backRebarDatas.push_back(it);
			}
		}
		if (frontRebarDatas.size() > 0)
		{
			m_Concrete.isSlabUpFaceUnionRebar = 1;
			m_Concrete.rebarLevelNum = frontRebarDatas.size();
			m_vecRebarData = frontRebarDatas;
			multiDiffThickSlabOrWallRebar(eeh, modelRef);
		}
		if (backRebarDatas.size() > 0)
		{
			m_Concrete.isSlabUpFaceUnionRebar = 0;
			m_Concrete.rebarLevelNum = backRebarDatas.size();
			m_vecRebarData = backRebarDatas;
			multiDiffThickSlabOrWallRebar(eeh, modelRef);
		}
		FacePreviewButtonsDown = false;
		return;
	}

	if (m_selectfaces.size() > 2)
	{
		EditElementHandle eehFace1;
		DPoint3d ptNormal1;
		DPoint3d ptInFace1;
		bool ret1 = PIT::ConvertToElement::SubEntityToElement(eehFace1, m_selectfaces[0], modelRef);
		mdlElmdscr_extractNormal(&ptNormal1, &ptInFace1, eehFace1.GetElementDescrCP(), NULL);
		for (int i = 1; i < (int)m_selectfaces.size() - 1; i += 2)
		{
			EditElementHandle eehFace2, eehFace3;
			bool ret2 = PIT::ConvertToElement::SubEntityToElement(eehFace2, m_selectfaces[i], modelRef);
			bool ret3 = PIT::ConvertToElement::SubEntityToElement(eehFace3, m_selectfaces[i + 1], modelRef);
			if (ret2 && ret3)
			{
				DPoint3d ptNormal2, ptNormal3;
				DPoint3d ptInFace2, ptInFace3;
				mdlElmdscr_extractNormal(&ptNormal2, &ptInFace2, eehFace2.GetElementDescrCP(), NULL);
				mdlElmdscr_extractNormal(&ptNormal3, &ptInFace3, eehFace3.GetElementDescrCP(), NULL);
				if (ptNormal1.IsPerpendicularTo(ptNormal2) && ptNormal1.IsParallelTo(ptNormal3))
				{
					//计算第三个面与第一个面的距离
					DPoint3d ptPro;
					mdlVec_projectPointToPlane(&ptPro, &ptInFace1, &ptInFace3, &ptNormal3);
					double dis = ptInFace1.Distance(ptPro);
					vecDis.push_back(dis);
				}

				ptNormal1 = ptNormal3;
				ptInFace1 = ptInFace3;
			}
		}
	}

	ElementId contid;
	if (vecDis.size()) // 互相垂直面配筋（3个面及以上）
	{
		perpendicularFaceReabr(vecDis, contid, eeh, modelRef);
	}
	else
	{
		if (this->m_PageMainRebar.GetIsMergeRebar() && m_selectfaces.size() > 1) // 多个面
		{
			multiFaceInlineRebar(contid, eeh, modelRef);
		}
		else
		{
			normalFaceRebar(contid, eeh, modelRef); // 一个面
		}

	}

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

	m_PageMainRebar.DeleteFaceLine();
	FacePreviewButtonsDown = false;
}


void CFacesRebarDlg::OnEnChangeEdit1() // 间距
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


void CFacesRebarDlg::OnCbnSelchangeCombo3() // 类型
{
	// TODO: 在此添加控件通知处理程序代码
	auto it = g_listRebarType.begin();
	advance(it, m_ComboType.GetCurSel());
	m_WallSetInfo.rebarType = m_ComboType.GetCurSel();
	m_PageMainRebar.ChangeRebarTypedata(m_WallSetInfo.rebarType);
	m_PageMainRebar.UpdateRebarList();
}


void CFacesRebarDlg::OnCbnSelchangeCombo2() // 直径
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


void CFacesRebarDlg::OnStnClickedStaticWallname()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CFacesRebarDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	SelectFaceTool2* tool = new SelectFaceTool2(1, 1, _ehOld,_ehNew,this);
	tool->InstallTool();
}
