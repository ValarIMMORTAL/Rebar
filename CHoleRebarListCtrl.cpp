#include "_ustation.h"
#include "CHoleRebarListCtrl.h"
#include "ConstantsDef.h"
#include "CHoleRebar_ReinforcingDlg.h"
#include "CHoleRebar_StructualDlg.h"
#include "CDoorHoleDlg.h"
#include "BentlyCommonfile.h"
#include "ScanIntersectTool.h"

bool g_closeDlg = false;

BEGIN_MESSAGE_MAP(CHoleRebarReinForcingCtrl, CListCtrlEx)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()



void CHoleRebarReinForcingCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	ListCtrlEx::CListCtrlEx::OnLButtonDown(nFlags, point);
	DeleteElements();
	CellIndex ix = Point2Cell(point);
	CString testring = GetItemText(ix.first, 0);
	std::string STDStr(CW2A(testring.GetString()));
	
	if (m_Dlg == nullptr)
	{
		return;
	}
	if (m_holeidAndmodel[STDStr].ID!=0&& m_holeidAndmodel[STDStr].tModel!=nullptr)
	{
		EditElementHandle eeh(m_holeidAndmodel[STDStr].ID, m_holeidAndmodel[STDStr].tModel);
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			if (m_Dlg != nullptr)
			{
				m_Dlg->m_nowHolename = STDStr;
				m_Dlg->GetNowHoleNum();
				m_Dlg->UpdateHoleDataView(STDStr);
			}
			SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
			ElementCopyContext copier2(ACTIVEMODEL);
			copier2.SetSourceModelRef(eeh.GetModelRef());
			copier2.SetTransformToDestination(true);
			copier2.SetWriteElements(false);
			copier2.DoCopy(eeh);
			eeh.AddToModel();
			m_affectedElement.FindByID(eeh.GetElementId(), ACTIVEMODEL, true);	
			SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), ACTIVEMODEL);
			mdlView_updateSingle(0);
		}
		
	}
	else if (STDStr!="")
	{
		if (m_Dlg != nullptr)
		{
			m_Dlg->m_nowHolename = STDStr;
			m_Dlg->GetNowHoleNum();
			m_Dlg->UpdateHoleDataView(STDStr);
		}
		for (HoleRebarInfo::ReinForcingInfo& rfInfo : m_Dlg->m_vecReinF)
		{
			string unionName(rfInfo.Uname);
			if (unionName.find(STDStr)!=string::npos)
			{
				
				if (m_holeidAndmodel[rfInfo.Hname].ID != 0 && m_holeidAndmodel[rfInfo.Hname].tModel != nullptr)
				{
					EditElementHandle eeh(m_holeidAndmodel[rfInfo.Hname].ID, m_holeidAndmodel[rfInfo.Hname].tModel);
					ISolidKernelEntityPtr entityPtr;
					if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
					{
						SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
						ElementCopyContext copier2(ACTIVEMODEL);
						copier2.SetSourceModelRef(eeh.GetModelRef());
						copier2.SetTransformToDestination(true);
						copier2.SetWriteElements(false);
						copier2.DoCopy(eeh);
						eeh.AddToModel();
						m_vecunionID.push_back(eeh.GetElementId());
						SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), ACTIVEMODEL);
						mdlView_updateSingle(0);
					}
				}
			}
		}
	}
}
void CHoleRebarReinForcingCtrl::DeleteElements()
{	
	mdlSelect_freeAll();
	m_affectedElement.DeleteFromModel();
	for (ElementId eehid: m_vecunionID)
	{
		EditElementHandle eeh(eehid, ACTIVEMODEL);
		eeh.DeleteFromModel();

	}
	m_vecunionID.clear();
}
void CHoleRebarReinForcingCtrl::GetAllRebarData(std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
	{
		string strholename = CT2A(it->second[0]);
		for (HoleRebarInfo::ReinForcingInfo& tmpinfo:vecListData)
		{
			string holename(tmpinfo.Hname);
			if (holename!=strholename)
			{
				continue;
			}
			tmpinfo.Hsize = atof(CT2A(it->second[1]));
			tmpinfo.MainRebarDis = atof(CT2A(it->second[2]));
			break;
		}
	}
}


BEGIN_MESSAGE_MAP(CHoleRebarStructualCtrl, CListCtrlEx)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void CHoleRebarStructualCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	ListCtrlEx::CListCtrlEx::OnLButtonDown(nFlags, point);
	DeleteElements();
	CellIndex ix = Point2Cell(point);
	CString testring = GetItemText(ix.first, 0);
	std::string STDStr(CW2A(testring.GetString()));

	if (m_Dlg == nullptr)
	{
		return;
	}
	if (m_holeidAndmodel[STDStr].ID != 0 && m_holeidAndmodel[STDStr].tModel != nullptr)
	{
		EditElementHandle eeh(m_holeidAndmodel[STDStr].ID, m_holeidAndmodel[STDStr].tModel);
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			if (m_Dlg != nullptr)
			{
				m_Dlg->m_nowHolename = STDStr;
				m_Dlg->GetNowHoleNum();
				m_Dlg->UpdateHoleDataView(STDStr);
			}
			SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
			ElementCopyContext copier2(ACTIVEMODEL);
			copier2.SetSourceModelRef(eeh.GetModelRef());
			copier2.SetTransformToDestination(true);
			copier2.SetWriteElements(false);
			copier2.DoCopy(eeh);
			eeh.AddToModel();
			m_affectedElement.FindByID(eeh.GetElementId(), ACTIVEMODEL, true);
			SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), ACTIVEMODEL);
			mdlView_updateSingle(0);
		}

	}
	else if (STDStr != "")
	{
		if (m_Dlg != nullptr)
		{
			m_Dlg->m_nowHolename = STDStr;
			m_Dlg->GetNowHoleNum();
			m_Dlg->UpdateHoleDataView(STDStr);
		}
		for (HoleRebarInfo::ReinForcingInfo& rfInfo : m_Dlg->m_vecReinF)
		{
			string unionName(rfInfo.Uname);
			if (unionName == STDStr)
			{

				if (m_holeidAndmodel[rfInfo.Hname].ID != 0 && m_holeidAndmodel[rfInfo.Hname].tModel != nullptr)
				{
					EditElementHandle eeh(m_holeidAndmodel[rfInfo.Hname].ID, m_holeidAndmodel[rfInfo.Hname].tModel);
					ISolidKernelEntityPtr entityPtr;
					if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
					{
						SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
						ElementCopyContext copier2(ACTIVEMODEL);
						copier2.SetSourceModelRef(eeh.GetModelRef());
						copier2.SetTransformToDestination(true);
						copier2.SetWriteElements(false);
						copier2.DoCopy(eeh);
						eeh.AddToModel();
						m_vecunionID.push_back(eeh.GetElementId());
						SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), ACTIVEMODEL);
						mdlView_updateSingle(0);
					}
				}
			}
		}
	}

}
void CHoleRebarStructualCtrl::DeleteElements()
{
	mdlSelect_freeAll();
	m_affectedElement.DeleteFromModel();
	for (ElementId eehid : m_vecunionID)
	{
		EditElementHandle eeh(eehid, ACTIVEMODEL);
		eeh.DeleteFromModel();

	}
	m_vecunionID.clear();
}
void CHoleRebarStructualCtrl::GetAllRebarData(std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
	{
		string strholename = CT2A(it->second[0]);
		for (HoleRebarInfo::ReinForcingInfo tmpinfo : vecListData)
		{
			string holename(tmpinfo.Hname);
			if (holename != strholename)
			{
				continue;
			}
			tmpinfo.Hsize = atof(CT2A(it->second[1]));
			break;
		}
	}
}


BEGIN_MESSAGE_MAP(CHoleRebarAddUnionCtrl, CListCtrlEx)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()



void CHoleRebarAddUnionCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	ListCtrlEx::CListCtrlEx::OnLButtonDown(nFlags, point);
	mdlSelect_freeAll();
	m_affectedElement.DeleteFromModel();
	CellIndex ix = Point2Cell(point);
	CString testring = GetItemText(ix.first, 0);
	std::string STDStr(CW2A(testring.GetString()));

	if (m_holeidAndmodel[STDStr].ID != 0 && m_holeidAndmodel[STDStr].tModel != nullptr)
	{
		EditElementHandle eeh(m_holeidAndmodel[STDStr].ID, m_holeidAndmodel[STDStr].tModel);
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
			ElementCopyContext copier2(ACTIVEMODEL);
			copier2.SetSourceModelRef(eeh.GetModelRef());
			copier2.SetTransformToDestination(true);
			copier2.SetWriteElements(false);
			copier2.DoCopy(eeh);
			eeh.AddToModel();
			m_affectedElement.FindByID(eeh.GetElementId(), ACTIVEMODEL, true);
			SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), ACTIVEMODEL);
			mdlView_updateSingle(0);
		}
	}

}
void CHoleRebarAddUnionCtrl::GetAllRebarData(std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	vector<HoleRebarInfo::ReinForcingInfo> tmpHole;
	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
	{
		string strholename = CT2A(it->second[0]);
		for (HoleRebarInfo::ReinForcingInfo& tmpinfo : vecListData)
		{
			string holename(tmpinfo.Hname);
			if (holename != strholename)
			{
				continue;
			}	
			tmpinfo.isUnionChild = atoi(CT2A(it->second[2]));
			if (tmpinfo.isUnionChild)
			{
				
				if (tmpinfo.Uname!="")
				{
					char tUname[512];
					sprintf(tUname, "%s", m_unionName.c_str());
					string tname(tUname);
					string nname(tmpinfo.Uname);//当前名称
					nname = nname + "," + tname;
					sprintf(tmpinfo.Uname, "%s", nname.c_str());
				}
				else
				{
					sprintf(tmpinfo.Uname, "%s", m_unionName.c_str());
				}
			}
			break;
		}
	}
}
void CHoleRebarAddUnionCtrl::GetAllRebarDataStructual(std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	vector<HoleRebarInfo::ReinForcingInfo> tmpHole;
	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
	{
		string strholename = CT2A(it->second[0]);
		for (HoleRebarInfo::ReinForcingInfo& tmpinfo : vecListData)
		{
			string holename(tmpinfo.Hname);
			if (holename != strholename)
			{
				continue;
			}
			tmpinfo.isUnionChild = atoi(CT2A(it->second[2]));
			if (tmpinfo.isUnionChild)
			{

				if (tmpinfo.Uname != "")
				{
					char tUname[512];
					sprintf(tUname, "%s", m_unionName.c_str());
					string tname(tUname);
					string nname(tmpinfo.Uname);//当前名称
					nname = nname + "," + tname;
					sprintf(tmpinfo.Uname, "%s", nname.c_str());
				}
				else
				{
					sprintf(tmpinfo.Uname, "%s", m_unionName.c_str());
				}
			}
			break;
		}
	}
}

BEGIN_MESSAGE_MAP(CDoorHoleRebarCtrl, CListCtrlEx)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()



void CDoorHoleRebarCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	ListCtrlEx::CListCtrlEx::OnLButtonDown(nFlags, point);
	DeleteElements();
	CellIndex ix = Point2Cell(point);
	CString testring = GetItemText(ix.first, 0);
	std::string STDStr(CW2A(testring.GetString()));

	if (m_Dlg == nullptr)
	{
		return;
	}
	if (m_holeidAndmodel[STDStr].ID != 0 && m_holeidAndmodel[STDStr].tModel != nullptr)
	{
		EditElementHandle eeh(m_holeidAndmodel[STDStr].ID, m_holeidAndmodel[STDStr].tModel);
		ISolidKernelEntityPtr entityPtr;
		if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
		{
			if (m_Dlg != nullptr)
			{
				m_Dlg->m_nowHolename = STDStr;
				m_Dlg->GetNowHoleNum();
				m_Dlg->UpdateHoleDataView(STDStr);
			}
			SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
			ElementCopyContext copier2(ACTIVEMODEL);
			copier2.SetSourceModelRef(eeh.GetModelRef());
			copier2.SetTransformToDestination(true);
			copier2.SetWriteElements(false);
			copier2.DoCopy(eeh);
			eeh.AddToModel();
			m_affectedElement.FindByID(eeh.GetElementId(), ACTIVEMODEL, true);
			SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), ACTIVEMODEL);
			mdlView_updateSingle(0);
		}

	}
	else if (STDStr != "")
	{
		if (m_Dlg != nullptr)
		{
			m_Dlg->m_nowHolename = STDStr;
			m_Dlg->GetNowHoleNum();
			m_Dlg->UpdateHoleDataView(STDStr);
		}
		for (HoleRebarInfo::ReinForcingInfo& rfInfo : m_Dlg->m_vecReinF)
		{
			string unionName(rfInfo.Uname);
			if (unionName == STDStr)
			{

				if (m_holeidAndmodel[rfInfo.Hname].ID != 0 && m_holeidAndmodel[rfInfo.Hname].tModel != nullptr)
				{
					EditElementHandle eeh(m_holeidAndmodel[rfInfo.Hname].ID, m_holeidAndmodel[rfInfo.Hname].tModel);
					ISolidKernelEntityPtr entityPtr;
					if (SolidUtil::Convert::ElementToBody(entityPtr, eeh) == SUCCESS)
					{
						SolidUtil::Convert::BodyToElement(eeh, *entityPtr, nullptr, *eeh.GetModelRef());
						ElementCopyContext copier2(ACTIVEMODEL);
						copier2.SetSourceModelRef(eeh.GetModelRef());
						copier2.SetTransformToDestination(true);
						copier2.SetWriteElements(false);
						copier2.DoCopy(eeh);
						eeh.AddToModel();
						m_vecunionID.push_back(eeh.GetElementId());
						SelectionSetManager::GetManager().AddElement(eeh.GetElementRef(), ACTIVEMODEL);
						mdlView_updateSingle(0);
					}
				}
			}
		}
	}
}
void CDoorHoleRebarCtrl::DeleteElements()
{
	mdlSelect_freeAll();
	m_affectedElement.DeleteFromModel();
	for (ElementId eehid : m_vecunionID)
	{
		EditElementHandle eeh(eehid, ACTIVEMODEL);
		eeh.DeleteFromModel();

	}
	m_vecunionID.clear();
}
void CDoorHoleRebarCtrl::GetAllRebarData(std::vector<HoleRebarInfo::ReinForcingInfo>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
	{
		string strholename = CT2A(it->second[0]);
		for (HoleRebarInfo::ReinForcingInfo& tmpinfo : vecListData)
		{
			string holename(tmpinfo.Hname);
			if (holename != strholename)
			{
				continue;
			}
			tmpinfo.Hsize = atof(CT2A(it->second[1]));
			tmpinfo.MainRebarDis = atof(CT2A(it->second[2]));
			tmpinfo.isGenerate = atoi(CT2A(it->second[3]));
			break;
		}
	}
}





void SelectHoleTools::InstallNewInstance(int toolId, CDialogEx* Linedlg)
{
	g_closeDlg = false;
	SelectHoleTools* tool = new SelectHoleTools(toolId);

	CHoleRebar_StructualDlg* dlg = dynamic_cast<CHoleRebar_StructualDlg*>(Linedlg);
	if (dlg == nullptr)
	{//加强钢筋的对话框指针
		CHoleRebar_ReinforcingDlg* dlg = dynamic_cast<CHoleRebar_ReinforcingDlg*>(Linedlg);
		if (dlg != nullptr)
		{
			tool->m_RHoledlg = dlg;
		}
	}
	else
	{//构造钢筋的对话框指针
		tool->m_SHoledlg = dlg;
	}
	tool->InstallTool();
}


bool SelectHoleTools::_OnDataButton(DgnButtonEventCR ev)
{
	if (g_closeDlg)//点击了确定后推出选择工具
	{
		_ExitTool();
		return true;
	}
	auto& ssm = SelectionSetManager::GetManager();
	ssm.EmptyAll();
	HitPathCP   path = _DoLocate(ev, true, ComponentMode::Innermost);
	if (path == NULL)
		return false;
	ElementHandle Holeeh(mdlDisplayPath_getElem((DisplayPathP)path, 0), mdlDisplayPath_getPathRoot((DisplayPathP)path));
	ssm.AddElement(Holeeh.GetElementRef(), ACTIVEMODEL);
	EditElementHandle eeh(Holeeh, false);

	if (m_SHoledlg)
	{
		if (g_closeDlg)//点击了确定后推出选择工具
		{
			_ExitTool();
			return true;
		}
		m_SHoledlg->GetSeclectElement(eeh);
	}
	else if (m_RHoledlg)
	{
		if (g_closeDlg)//点击了确定后推出选择工具
		{
			_ExitTool();
			return true;
		}
		m_RHoledlg->GetSeclectElement(eeh);
	}
	else
	{
	}

	return true;
}

bool SelectHoleTools::_OnModifyComplete(DgnButtonEventCR ev)
{
	ElementAgenda selectedElement;
	selectedElement = GetElementAgenda();

	return true;
}


bool SelectHoleTools::_OnPostLocate(HitPathCP path, WStringR cantAcceptReason)
{
	if (!__super::_OnPostLocate(path, cantAcceptReason))
		return false;
	return true;
}

bool SelectHoleTools::_WantAdditionalLocate(DgnButtonEventCP ev)
{
	if (NULL == ev)
		return true;

	return GetElementAgenda().GetCount() != 2;
}