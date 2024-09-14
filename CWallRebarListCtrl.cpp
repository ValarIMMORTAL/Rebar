#include "_ustation.h"
#include "CWallRebarListCtrl.h"
#include "ConstantsDef.h"
#include "CWallMainRebarDlg.h"
#include "CoverslabMainRebarDlg.h"
#include "CSlabMainRebarDlg.h"
#include "CFacesMainRebarDlg.h"
#include "BentlyCommonfile.h"
#include "ScanIntersectTool.h"
#include "resource.h"
#include "CBreakEllipseWallDlg.h"

BEGIN_MESSAGE_MAP(CMainRebarListCtrl, CListCtrlEx)
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, &CMainRebarListCtrl::OnLvnEndlabeledit)
END_MESSAGE_MAP()

void CMainRebarListCtrl::GetAllRebarData(vector<PIT::ConcreteRebar>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	vecListData.clear();
	if (m_isWall)
	{
		for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
		{
			PIT::ConcreteRebar oneRebarInfo;
			memset(&oneRebarInfo, 0, sizeof(PIT::ConcreteRebar));
			for (int j = 0; j < it->second.size(); ++j)
			{
				switch (j)
				{
				case 0:
				{
					CString strCellValue = it->second[j];
					strCellValue.Delete(strCellValue.GetLength() - 1, strCellValue.GetLength());
					//oneRebarInfo.rebarLevel = (int)distance(mapAllData.begin(),it);
					oneRebarInfo.rebarLevel = (int)atoi(CT2A(strCellValue));
					break;
				}
				case 1:
				{
					CString strCellValue = it->second[j];
					auto find = std::find(g_listRebarDir.begin(), g_listRebarDir.end(), strCellValue);
					int nIndex = (int)std::distance(g_listRebarDir.begin(), find);
					if (find == g_listRebarDir.end())
					{
						find = std::find(g_listSlabRebarDir.begin(), g_listSlabRebarDir.end(), strCellValue);
						nIndex = (int)std::distance(g_listSlabRebarDir.begin(), find);
						if (find == g_listSlabRebarDir.end())
						{
							find = std::find(g_listRebarRadiateDir.begin(), g_listRebarRadiateDir.end(), strCellValue);
							nIndex = (int)std::distance(g_listRebarRadiateDir.begin(), find);
						}
					}
					oneRebarInfo.rebarDir = nIndex;
				}
				break;
				case 2:
				{
					CString strRebarSize = it->second[j];
					strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);	//删掉mm
					strcpy(oneRebarInfo.rebarSize, CT2A(strRebarSize));
					break;
				}
				case 3:
				{
					CString strCellValue = it->second[j];
					auto find = std::find(g_listRebarType.begin(), g_listRebarType.end(), strCellValue);
					int nIndex = (int)std::distance(g_listRebarType.begin(), find);
					oneRebarInfo.rebarType = nIndex;
				}
				break;
				case 4:
					oneRebarInfo.spacing = atof(CT2A(it->second[j]));
					break;
				case 5:
				{
					CString strCellValue = it->second[j];
					auto find = std::find(g_listRebarPose.begin(), g_listRebarPose.end(), strCellValue);
					int nIndex = (int)std::distance(g_listRebarPose.begin(), find);
					if (nIndex >= g_listRebarPose.size())
					{
						find = std::find(g_listfloorRebarPose.begin(), g_listfloorRebarPose.end(), strCellValue);
						nIndex = (int)std::distance(g_listfloorRebarPose.begin(), find);
					}
					oneRebarInfo.datachange = nIndex;
					break;
				}
				/*case 9:
				{
					CString strCellValue = it->second[j];
					oneRebarInfo.rebarColor = _ttoi(strCellValue);
					break;
				}*/
				case 6:
				{
					CString strCellValue = it->second[j];
					oneRebarInfo.rebarLineStyle = _ttoi(strCellValue);
					break;
				}
				case 7:
				{
					CString strCellValue = it->second[j];
					oneRebarInfo.rebarWeight = _ttoi(strCellValue);
					break;
				}

				}
			}
			vecListData.push_back(oneRebarInfo);
		}
	}
	else
	{
		for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
		{
			PIT::ConcreteRebar oneRebarInfo;
			memset(&oneRebarInfo, 0, sizeof(PIT::ConcreteRebar));
			for (int j = 0; j < it->second.size(); ++j)
			{
				switch (j)
				{
				case 0:
				{
					CString strCellValue = it->second[j];
					strCellValue.Delete(strCellValue.GetLength() - 1, strCellValue.GetLength());
					//oneRebarInfo.rebarLevel = (int)distance(mapAllData.begin(),it);
					oneRebarInfo.rebarLevel = (int)atoi(CT2A(strCellValue));
					break;
				}
				case 1:
				{
					CString strCellValue = it->second[j];
					auto find = std::find(g_listRebarDir.begin(), g_listRebarDir.end(), strCellValue);
					int nIndex = (int)std::distance(g_listRebarDir.begin(), find);
					if (find == g_listRebarDir.end())
					{
						find = std::find(g_listSlabRebarDir.begin(), g_listSlabRebarDir.end(), strCellValue);
						nIndex = (int)std::distance(g_listSlabRebarDir.begin(), find);
						if (find == g_listSlabRebarDir.end())
						{
							find = std::find(g_listRebarRadiateDir.begin(), g_listRebarRadiateDir.end(), strCellValue);
							nIndex = (int)std::distance(g_listRebarRadiateDir.begin(), find);
						}
					}
					oneRebarInfo.rebarDir = nIndex;
				}
				break;
				case 2:
				{
					CString strRebarSize = it->second[j];
					strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);	//删掉mm
					strcpy(oneRebarInfo.rebarSize, CT2A(strRebarSize));
					break;
				}
				case 3:
				{
					CString strCellValue = it->second[j];
					auto find = std::find(g_listRebarType.begin(), g_listRebarType.end(), strCellValue);
					int nIndex = (int)std::distance(g_listRebarType.begin(), find);
					oneRebarInfo.rebarType = nIndex;
				}
				break;
				case 4:
					oneRebarInfo.spacing = atof(CT2A(it->second[j]));
					break;
				case 5:
					oneRebarInfo.startOffset = atof(CT2A(it->second[j]));
					break;
				case 6:
					oneRebarInfo.endOffset = atof(CT2A(it->second[j]));
					break;
				case 7:
					oneRebarInfo.levelSpace = atof(CT2A(it->second[j]));
					break;
				case 8:
				{
					CString strCellValue = it->second[j];
					auto find = std::find(g_listRebarPose.begin(), g_listRebarPose.end(), strCellValue);
					int nIndex = (int)std::distance(g_listRebarPose.begin(), find);
					if (nIndex >= g_listRebarPose.size())
					{
						find = std::find(g_listfloorRebarPose.begin(), g_listfloorRebarPose.end(), strCellValue);
						nIndex = (int)std::distance(g_listfloorRebarPose.begin(), find);
					}
					oneRebarInfo.datachange = nIndex;
					break;
				}
				/*case 9:
				{
					CString strCellValue = it->second[j];
					oneRebarInfo.rebarColor = _ttoi(strCellValue);
					break;
				}*/
				case 9:
				{
					CString strCellValue = it->second[j];
					oneRebarInfo.rebarLineStyle = _ttoi(strCellValue);
					break;
				}
				case 10:
				{
					CString strCellValue = it->second[j];
					oneRebarInfo.rebarWeight = _ttoi(strCellValue);
					break;
				}

				}
			}
			vecListData.push_back(oneRebarInfo);
		}
	}
	
}

void CMainRebarListCtrl::GetUniteRebarData(vector<PIT::ConcreteRebar>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	vecListData.clear();
	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
	{
		PIT::ConcreteRebar oneRebarInfo;
		memset(&oneRebarInfo, 0, sizeof(PIT::ConcreteRebar));
		for (int j = 0; j < it->second.size(); ++j)
		{
			switch (j)
			{
			case 0:
			{
				CString strCellValue = it->second[j];
				strCellValue.Delete(strCellValue.GetLength() - 1, strCellValue.GetLength());
				//oneRebarInfo.rebarLevel = (int)distance(mapAllData.begin(),it);
				oneRebarInfo.rebarLevel = (int)atoi(CT2A(strCellValue));
				break;
			}
			case 1:
			{
				CString strRebarSize = it->second[j];
				strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);	//删掉mm
				strcpy(oneRebarInfo.rebarSize, CT2A(strRebarSize));
				break;
			}
			case 2:
			{
				CString strCellValue = it->second[j];
				auto find = std::find(g_listRebarType.begin(), g_listRebarType.end(), strCellValue);
				int nIndex = (int)std::distance(g_listRebarType.begin(), find);
				oneRebarInfo.rebarType = nIndex;
			}
			break;
			case 3:
				oneRebarInfo.spacing = atof(CT2A(it->second[j]));
				break;
			case 4:
				oneRebarInfo.startOffset = atof(CT2A(it->second[j]));
				break;
			case 5:
				oneRebarInfo.endOffset = atof(CT2A(it->second[j]));
				break;
			case 6:
				oneRebarInfo.levelSpace = atof(CT2A(it->second[j]));
				break;
			case 7:
			{
				CString strCellValue = it->second[j];
				auto find = std::find(g_listRebarPose.begin(), g_listRebarPose.end(), strCellValue);
				int nIndex = (int)std::distance(g_listRebarPose.begin(), find);
				if (nIndex >= g_listRebarPose.size())
				{
					find = std::find(g_listfloorRebarPose.begin(), g_listfloorRebarPose.end(), strCellValue);
					nIndex = (int)std::distance(g_listfloorRebarPose.begin(), find);
					if (nIndex >= g_listfloorRebarPose.size())
					{
						int tmep = 0;
						find = std::find(g_listRebartype.begin(), g_listRebartype.end(), strCellValue);
						tmep = (int)std::distance(g_listRebartype.begin(), find);
						oneRebarInfo.rebarDir = tmep;
					}
				}
				oneRebarInfo.datachange = nIndex;
				break;
			}
			/*case 9:
			{
				CString strCellValue = it->second[j];
				oneRebarInfo.rebarColor = _ttoi(strCellValue);
				break;
			}*/
			case 8:
			{
				CString strCellValue = it->second[j];
				oneRebarInfo.rebarLineStyle = _ttoi(strCellValue);
				break;
			}
			case 9:
			{
				CString strCellValue = it->second[j];
				oneRebarInfo.rebarWeight = _ttoi(strCellValue);
				break;
			}

			}
		}
		vecListData.push_back(oneRebarInfo);
	}
}

void CMainRebarListCtrl::GetAllRebarData_wall(vector<PIT::ConcreteRebar>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	vecListData.clear();
	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
	{
		PIT::ConcreteRebar oneRebarInfo;
		memset(&oneRebarInfo, 0, sizeof(PIT::ConcreteRebar));
		for (int j = 0; j < it->second.size(); ++j)
		{
			switch (j)
			{
			case 0:
			{
				CString strCellValue = it->second[j];
				strCellValue.Delete(strCellValue.GetLength() - 1, strCellValue.GetLength());
				//oneRebarInfo.rebarLevel = (int)distance(mapAllData.begin(),it);
				oneRebarInfo.rebarLevel = (int)atoi(CT2A(strCellValue));
				break;
			}
			case 1:
			{
				CString strCellValue = it->second[j];
				auto find = std::find(g_listRebarDir.begin(), g_listRebarDir.end(), strCellValue);
				int nIndex = (int)std::distance(g_listRebarDir.begin(), find);
				if (find == g_listRebarDir.end())
				{
					find = std::find(g_listSlabRebarDir.begin(), g_listSlabRebarDir.end(), strCellValue);
					nIndex = (int)std::distance(g_listSlabRebarDir.begin(), find);
					if (find == g_listSlabRebarDir.end())
					{
						find = std::find(g_listRebarRadiateDir.begin(), g_listRebarRadiateDir.end(), strCellValue);
						nIndex = (int)std::distance(g_listRebarRadiateDir.begin(), find);
					}
				}
				oneRebarInfo.rebarDir = nIndex;
			}
			break;
			case 2:
			{
				CString strRebarSize = it->second[j];
				strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);	//删掉mm
				strcpy(oneRebarInfo.rebarSize, CT2A(strRebarSize));
				break;
			}
			case 3:
			{
				CString strCellValue = it->second[j];
				auto find = std::find(g_listRebarType.begin(), g_listRebarType.end(), strCellValue);
				int nIndex = (int)std::distance(g_listRebarType.begin(), find);
				oneRebarInfo.rebarType = nIndex;
			}
			break;
			case 4:
				oneRebarInfo.spacing = atof(CT2A(it->second[j]));
				break;
			/*case 5:
				oneRebarInfo.startOffset = atof(CT2A(it->second[j]));
				break;
			case 6:
				oneRebarInfo.endOffset = atof(CT2A(it->second[j]));
				break;
			case 7:
				oneRebarInfo.levelSpace = atof(CT2A(it->second[j]));
				break;*/
			case 5:
			{
				CString strCellValue = it->second[j];
				auto find = std::find(g_listRebarPose.begin(), g_listRebarPose.end(), strCellValue);
				int nIndex = (int)std::distance(g_listRebarPose.begin(), find);
				oneRebarInfo.datachange = nIndex;
				break;
			}
			/*case 9:
			{
				CString strCellValue = it->second[j];
				oneRebarInfo.rebarColor = _ttoi(strCellValue);
				break;
			}*/
			case 6:
			{
				CString strCellValue = it->second[j];
				oneRebarInfo.rebarLineStyle = _ttoi(strCellValue);
				break;
			}
			case 7:
			{
				CString strCellValue = it->second[j];
				oneRebarInfo.rebarWeight = _ttoi(strCellValue);
				break;
			}

			}
		}
		vecListData.push_back(oneRebarInfo);
	}
}

void CMainRebarListCtrl::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	// Update the item text with the new text
	if (pDispInfo->item.iSubItem == 4)	//如果点击的是第5列
	{
		//判断输入的值是否小于100
		if (g_wallRebarInfo.concrete.m_SlabRebarMethod != 2 && atoi(CT2A(pDispInfo->item.pszText)) < 100)
		{
			GetParent()->MessageBox(L"输入值不能小于100", L"提示", MB_OK);
			*pResult = 0;
			return;
		}
	}
	if (m_isWall)
	{
		if (pDispInfo->item.iSubItem == 5)
		{
			vector<PIT::ConcreteRebar> vecRebarData;
			GetAllRebarData(vecRebarData);

			CString strTmp = pDispInfo->item.pszText;
			if (strTmp == "外侧面")
			{
				vecRebarData.at(pDispInfo->item.iItem).datachange = 0;
			}
			else if (strTmp == "中间")
			{
				vecRebarData.at(pDispInfo->item.iItem).datachange = 1;
			}
			else
			{
				vecRebarData.at(pDispInfo->item.iItem).datachange = 2;
			}

			CWallMainRebarDlg *dlg = dynamic_cast<CWallMainRebarDlg*>(GetParent());
			if (dlg == nullptr)
			{
				CSlabMainRebarDlg* dlgSlab = dynamic_cast<CSlabMainRebarDlg*>(GetParent());
				if (dlgSlab == nullptr)
				{
					// CFacesMainRebarDlg* dlgFace = dynamic_cast<CFacesMainRebarDlg*>(GetParent());
					// if (dlgFace != nullptr)
					// {
					// 	dlgFace->SetListRowData(vecRebarData);
					// 	dlgFace->UpdateRebarList();
					// }
				}
				else
				{
					dlgSlab->SetListRowData(vecRebarData);
					dlgSlab->UpdateRebarList();
				}
			}
			else
			{
				dlg->SetListRowData(vecRebarData);
				dlg->UpdateRebarList();
			}
		}
	}
	else if (!m_isWall)
	{
		if (pDispInfo->item.iSubItem == 8)	//如果点击的是第9列
		{
			vector<PIT::ConcreteRebar> vecRebarData;
			GetAllRebarData(vecRebarData);

			CString strTmp = pDispInfo->item.pszText;
			if (strTmp == "板底")
			{
				vecRebarData.at(pDispInfo->item.iItem).datachange = 0;
			}
			else if (strTmp == "中间")
			{
				vecRebarData.at(pDispInfo->item.iItem).datachange = 1;
			}
			else
			{
				vecRebarData.at(pDispInfo->item.iItem).datachange = 2;
			}

			CWallMainRebarDlg *dlg = dynamic_cast<CWallMainRebarDlg*>(GetParent());
			if (dlg == nullptr)
			{
				CSlabMainRebarDlg* dlgSlab = dynamic_cast<CSlabMainRebarDlg*>(GetParent());
				if (dlgSlab == nullptr)
				{
					// CFacesMainRebarDlg* dlgFace = dynamic_cast<CFacesMainRebarDlg*>(GetParent());
					// if (dlgFace != nullptr)
					// {
					// 	dlgFace->SetListRowData(vecRebarData);
					// 	dlgFace->UpdateRebarList();
					// }
				}
				else
				{
					dlgSlab->SetListRowData(vecRebarData);
					dlgSlab->UpdateRebarList();
				}
			}
			else
			{
				dlg->SetListRowData(vecRebarData);
				dlg->UpdateRebarList();
			}
		}
	}
	//else if (pDispInfo->item.iSubItem == 8)	//如果点击的是第9列
	//{
	//	vector<PIT::ConcreteRebar> vecRebarData;
	//	GetAllRebarData(vecRebarData);

	//	CString strTmp = pDispInfo->item.pszText;
	//	if (strTmp == "正面")
	//	{
	//		vecRebarData.at(pDispInfo->item.iItem).datachange = 0;
	//	}
	//	else if (strTmp == "中间")
	//	{
	//		vecRebarData.at(pDispInfo->item.iItem).datachange = 1;
	//	}
	//	else
	//	{
	//		vecRebarData.at(pDispInfo->item.iItem).datachange = 2;
	//	}

	//	CWallMainRebarDlg *dlg = dynamic_cast<CWallMainRebarDlg*>(GetParent());
	//	if (dlg == nullptr)
	//	{
	//		CSlabMainRebarDlg* dlgSlab = dynamic_cast<CSlabMainRebarDlg*>(GetParent());
	//		if (dlgSlab == nullptr)
	//		{
	//			// CFacesMainRebarDlg* dlgFace = dynamic_cast<CFacesMainRebarDlg*>(GetParent());
	//			// if (dlgFace != nullptr)
	//			// {
	//			// 	dlgFace->SetListRowData(vecRebarData);
	//			// 	dlgFace->UpdateRebarList();
	//			// }
	//		}
	//		else
	//		{
	//			dlgSlab->SetListRowData(vecRebarData);
	//			dlgSlab->UpdateRebarList();
	//		}
	//	}
	//	else
	//	{
	//		dlg->SetListRowData(vecRebarData);
	//		dlg->UpdateRebarList();
	//	}
	//}
	SetItemText(pDispInfo->item.iItem, pDispInfo->item.iSubItem, pDispInfo->item.pszText);
	GetParent()->SendMessage(WM_VALIDATE, GetDlgCtrlID(), (LPARAM)pDispInfo);

	vector<PIT::ConcreteRebar> vecRebarData;
	CWallMainRebarDlg *dlg = dynamic_cast<CWallMainRebarDlg*>(GetParent());
	if (dlg == nullptr)
	{
		CoverslabMainRebarDlg *dlgcover = dynamic_cast<CoverslabMainRebarDlg*>(GetParent());
		if (dlgcover!=nullptr)
		{
			dlgcover->m_listMainRebar.GetAllRebarData(vecRebarData);
			dlgcover->SetListRowData(vecRebarData);
		}
	}
	else
	{
		dlg->m_listMainRebar.GetAllRebarData(vecRebarData);
		dlg->SetListRowData(vecRebarData);
	}
	
	*pResult = 0;
}

// void CustomRebarlListCtrl::GetAllRebarData(vector<CustomRebarl>& vecWCustomRebarl)
// {
// 	map<int, vector<CString> > mapAllData;
// 	GetAllData(mapAllData);
// 	string strText;
// 	vecWCustomRebarl.clear();

// 	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
// 	{
// 		CustomRebarl vecRebarl;
// 		for (int j = 0; j < it->second.size(); ++j)
// 		{
// 			switch (j)
// 			{
// 			case 0:
// 				vecRebarl.number = atoi(CT2A(it->second[j])); //钢筋序号
// 				break;
// 			case 1:
// 				vecRebarl.Rebarlength = atof(CT2A(it->second[j]));//钢筋长度
// 				break;
// 			case 2:	
// 				if(it->second[j] ==L"外表皮长度")
// 				vecRebarl.lengthtype = 0; //长度类型
// 				else
// 				vecRebarl.lengthtype = 1; //长度类型
// 				break;
// 			default:
// 				break;
// 			}
// 		}
// 		vecWCustomRebarl.push_back(vecRebarl);
// 	}



// }


// void CInsertRebarListCtrl::GetAllRebarData(vector<InsertRebarInfo::WallInfo>& vecListData)
// {
// 	map<int, vector<CString> > mapAllData;
// 	GetAllData(mapAllData);
// 	double slabThickness = 0.00;
// 	double rebarDistance = 0.00;
// 	double postiveCover = 0.00;
// 	if (vecListData.size() > 0)
// 	{
// 		slabThickness = vecListData[0].slabThickness;
// 		rebarDistance = vecListData[0].slabDistance;
// 		postiveCover = vecListData[0].postiveCover;

// 	}
// 	vecListData.clear();
// 	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
// 	{
// 		InsertRebarInfo::WallInfo oneWallInfo;
// 		for (int j = 0; j < it->second.size(); ++j)
// 		{
// 			switch (j)
// 			{
// 			case 9:
// 				oneWallInfo.embedLength = atof(CT2A(it->second[j]));
// 				break;
// 			case 10:
// 				oneWallInfo.expandLength = atof(CT2A(it->second[j]));
// 				break;
// 			default:
// 				break;
// 			}
// 		}
// 		oneWallInfo.slabThickness = slabThickness;
// 		oneWallInfo.slabDistance = rebarDistance;
// 		oneWallInfo.postiveCover = postiveCover;
// 		vecListData.push_back(oneWallInfo);
// 	}
// }

void CRebarEndTypeListCtrl::GetAllRebarData(vector<PIT::EndType>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	vector<PIT::EndType> vecListDataTmp(vecListData.size());
	std::copy(vecListData.begin(), vecListData.end(), vecListDataTmp.begin());
	vecListData.clear();
	int irow = 0;
	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
	{
		PIT::EndType oneEndType;
		memset(&oneEndType, 0, sizeof(PIT::EndType));
		int nRow = (int)std::distance(mapAllData.begin(), it);
		if (vecListDataTmp.size() >= mapAllData.size())
		{
			oneEndType.endPtInfo = vecListDataTmp[nRow].endPtInfo;
		}
		for (int j = 0; j < it->second.size(); ++j)
		{
			switch (j)
			{
			case 0:
				break;
			case 1:
			{
				CString strCellValue = it->second[j];
				auto find = std::find(g_listEndType.begin(), g_listEndType.end(), strCellValue);
				int nIndex = (int)std::distance(g_listEndType.begin(), find);
				oneEndType.endType = nIndex;
			}
			break;
			case 2:
				oneEndType.endPtInfo = m_rebarEndPointInfos[irow];
				break;
			case 3:
				oneEndType.offset = atof(CT2A(it->second[j]));
				break;
			case 4:
			{
//				CString strCellValue = it->second[j];
// 				auto find = std::find(g_listRotateAngle.begin(), g_listRotateAngle.end(), strCellValue);
// 				int nIndex = (int)std::distance(g_listRotateAngle.begin(), find);
// 				if (find != g_listRotateAngle.end())
// 				{
// 					oneEndType.rotateAngle = 90.0 * nIndex;
// 				}
// 				else
// 				{
					oneEndType.rotateAngle = atof(CT2A(it->second[j]));
//				}
			}
			break;
			case 5:
				break;
			}
		}
		++irow;
		vecListData.push_back(oneEndType);
	}
}

// void CRebarDomeListCtrl::GetAllRebarData(vector<PIT::DomeLevelInfo>& vecListData)
// {
// 	map<int, vector<CString> > mapAllData;
// 	GetAllData(mapAllData);
// 	vecListData.clear();
// 	int iIndex = 0;
// 	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
// 	{
// 		PIT::DomeLevelInfo stDomeLevelInfo;
// 		memset(&stDomeLevelInfo, 0, sizeof(PIT::DomeLevelInfo));
// 		for (int j = 0; j < it->second.size(); ++j)
// 		{
// 			switch (j)
// 			{
// 			case 0:
// 			{
// 				stDomeLevelInfo.nNumber = iIndex;
// 				break;
// 			}
// 			case 1:
// 			{
// 				CString strCellValue = it->second[j];
// 				stDomeLevelInfo.dRadius1 = atof(CT2A(strCellValue));
// 				break;
// 			}
// 			case 2:
// 			{
// 				CString strCellValue = it->second[j];
// 				stDomeLevelInfo.dRadius2 = atof(CT2A(strCellValue));
// 				break;
// 			}
// 			case 3:
// 			{
// 				CString strCellValue = it->second[j];
// 				auto find = std::find(g_listDomeRebarLayout.begin(), g_listDomeRebarLayout.end(), strCellValue);
// 				int nFind = (int)std::distance(g_listDomeRebarLayout.begin(), find);
// 				stDomeLevelInfo.nLayoutType = nFind;
// 			}
// 			default:
// 				break;
// 			}
// 		}
// 		vecListData.push_back(stDomeLevelInfo);
// 		iIndex++;
// 	}
// 	return;
// }

// void CRebarDomeListCtrl::GetAllRebarData(vector<PIT::DomeLevelDetailInfo>& vecListData)
// {
// 	map<int, vector<CString> > mapAllData;
// 	int iIndex = 0;
// 	int nNumber = 0;
// 	if (vecListData.size() > 0)
// 	{
// 		nNumber = vecListData.at(0).nNumber;
// 	}
// 	vecListData.clear();
// 	GetAllData(mapAllData);
// 	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
// 	{
// 		PIT::DomeLevelDetailInfo stDomeLevelInfo;
// 		memset(&stDomeLevelInfo, 0, sizeof(PIT::DomeLevelDetailInfo));
// 		for (int j = 0; j < it->second.size(); ++j)
// 		{
// 			switch (j)
// 			{
// 			case 0:
// 			{
// 				stDomeLevelInfo.nLevel = iIndex;
// 				break;
// 			}
// 			case 1:
// 			{
// 				CString strCellValue = it->second[j];
// 				auto find = std::find(g_listDomeRebarDir.begin(), g_listDomeRebarDir.end(), strCellValue);
// 				if (find == g_listDomeRebarDir.end())
// 				{
// 					auto find_nex = std::find(g_listDomeRebarDirXY.begin(), g_listDomeRebarDirXY.end(), strCellValue);
// 					int nFind = (int)std::distance(g_listDomeRebarDirXY.begin(), find_nex);
// 					stDomeLevelInfo.rebarShape = nFind;
// 				}
// 				else
// 				{
// 					int nFind = (int)std::distance(g_listDomeRebarDir.begin(), find);
// 					stDomeLevelInfo.rebarShape = nFind;
// 				}
// 				break;

// 			}
// 			case 2:
// 			{
// 				CString strRebarSize = it->second[j];
// 				strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);	//删掉mm
// 				strcpy(stDomeLevelInfo.rebarSize, CT2A(strRebarSize));
// 				break;
// 			}
// 			case 3:
// 			{
// 				CString strCellValue = it->second[j];
// 				auto find = std::find(g_listRebarType.begin(), g_listRebarType.end(), strCellValue);
// 				int nFind = (int)std::distance(g_listRebarType.begin(), find);
// 				stDomeLevelInfo.rebarType = nFind;
// 				break;
// 			}
// 			case 4:
// 			{	
// 				CString strCellValue = it->second[j];
// 				stDomeLevelInfo.dAngleOrSpace = atof(CT2A(strCellValue));
// 				break;
// 			}
// 			case 5:
// 			{
// 				CString strCellValue = it->second[j];
// 				stDomeLevelInfo.dSpacing = atof(CT2A(strCellValue));
// 				break;
// 			}
// 			case 6:
// 			{
// 				CString strCellValue = it->second[j];
// 				stDomeLevelInfo.dStartOffset = atof(CT2A(strCellValue));
// 				break;
// 			}
// 			default:
// 				break;
// 			}
// 		}
// 		stDomeLevelInfo.nNumber = nNumber;
// 		vecListData.push_back(stDomeLevelInfo);
// 		iIndex++;
// 	}
// 	return;
// }


void CTwinBarListCtrl::GetAllRebarData(vector<TwinBarSet::TwinBarLevelInfo>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	vecListData.clear();
	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
	{
		TwinBarSet::TwinBarLevelInfo oneRebarInfo;
		memset(&oneRebarInfo, 0, sizeof(TwinBarSet::TwinBarLevelInfo));
		for (int j = 0; j < it->second.size(); ++j)
		{
			switch (j)
			{
			case 2:
			{
				CString strCellValue = it->second[j];
				strcpy(oneRebarInfo.levelName, CT2A(strCellValue));
				break;
			}
			case 3:
			{
				CString strCellValue = it->second[j];
				oneRebarInfo.hasTwinbars = atoi(CT2A(strCellValue));
			}
			break;
			case 4:
			{
				CString strRebarSize = it->second[j];
				strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);	//删掉mm
				strcpy(oneRebarInfo.rebarSize, CT2A(strRebarSize));

			}
			break;
			case 5:
			{
				CString strCellValue = it->second[j];
				auto find = std::find(g_listRebarType.begin(), g_listRebarType.end(), strCellValue);
				int nIndex = (int)std::distance(g_listRebarType.begin(), find);
				oneRebarInfo.rebarType = nIndex;
			}
			break;
			case 6:
			{
				CString strCellValue = it->second[j];
				oneRebarInfo.interval = atoi(CT2A(strCellValue));
			}
			break;
			default:
				break;
			}
		}
		vecListData.push_back(oneRebarInfo);
	}
}


BEGIN_MESSAGE_MAP(CWallRebarAssociatedCtrl, CListCtrlEx)
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, &CWallRebarAssociatedCtrl::OnLvnEndlabeledit)
END_MESSAGE_MAP()

map<string, IDandModelref> m_mapidAndmodel;//在导入了三维模型后，出图之前，存放所有的板件ID和model

void CWallRebarAssociatedCtrl::GetAllRebarData(vector<PIT::AssociatedComponent>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	if (vecListData.size() != mapAllData.size())
	{
		vecListData.clear();
	}

	if (vecListData.empty())
	{
		for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
		{
			PIT::AssociatedComponent oneRebarInfo;
			memset(&oneRebarInfo, 0, sizeof(PIT::AssociatedComponent));
			for (int j = 0; j < it->second.size(); ++j)
			{
				CString strCellValue = it->second[j];
				switch (j)
				{
				case 0:
				{
					strcpy(oneRebarInfo.CurrentWallName, CT2A(strCellValue));
					break;
				}
				case 1:
				{
					strcpy(oneRebarInfo.associatedComponentName, CT2A(strCellValue));
					break;
				}
				case 2:
				{
					auto find = std::find(g_listACRelation.begin(), g_listACRelation.end(), strCellValue);
					int nIndex = (int)std::distance(g_listACRelation.begin(), find);
					oneRebarInfo.mutualRelation = nIndex;
					break;
				}
				case 3:
				{
					auto find = std::find(g_listACRebarRelation.begin(), g_listACRebarRelation.end(), strCellValue);
					int nIndex = (int)std::distance(g_listACRebarRelation.begin(), find);
					oneRebarInfo.associatedRelation = nIndex;
					break;
				}
				case 4:
				{
					auto find = std::find(g_listACMethod.begin(), g_listACMethod.end(), strCellValue);
					int nIndex = (int)std::distance(g_listACMethod.begin(), find);
					oneRebarInfo.anchoringMethod = nIndex;
					break;
				}
				default:
					break;
				}
			}
			vecListData.push_back(oneRebarInfo);
		}
	}
	else
	{
		int i = 0;
		for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it,++i)
		{
//			AssociatedComponent oneRebarInfo;
			for (int j = 0; j < it->second.size(); ++j)
			{
				CString strCellValue = it->second[j];
				switch (j)
				{
				case 0:
				{
					strcpy(vecListData[i].CurrentWallName, CT2A(strCellValue));
					break;
				}
				case 1:
				{
					strcpy(vecListData[i].associatedComponentName, CT2A(strCellValue));
					break;
				}
				case 2:
				{
					auto find = std::find(g_listACRelation.begin(), g_listACRelation.end(), strCellValue);
					int nIndex = (int)std::distance(g_listACRelation.begin(), find);
					vecListData[i].mutualRelation = nIndex;
					break;
				}
				case 3:
				{
					auto find = std::find(g_listACRebarRelation.begin(), g_listACRebarRelation.end(), strCellValue);
					int nIndex = (int)std::distance(g_listACRebarRelation.begin(), find);
					vecListData[i].associatedRelation = nIndex;
					break;
				}
				case 4:
				{
					auto find = std::find(g_listACMethod.begin(), g_listACMethod.end(), strCellValue);
					int nIndex = (int)std::distance(g_listACMethod.begin(), find);
					vecListData[i].anchoringMethod = nIndex;
					break;
				}
				default:
					break;
				}
			}
//			vecListData.push_back(oneRebarInfo);
		}
	}
	
}

// void CBeamListCtrl::GetAllRebarData(vector<BeamRebarInfo::BeamCommHoop>& vecBeamCommHoop)
// {
// 	map<int, vector<CString>> mapAllData;
// 	GetAllData(mapAllData);
// 	vector<int> vecRebarType;
// 	for (unsigned int i = 0; i < vecBeamCommHoop.size(); i++)
// 	{
// 		vecRebarType.push_back(vecBeamCommHoop[i].rebarType);
// 	}
// 	vecBeamCommHoop.clear();

// 	int nIndex = 0;
// 	for (map<int, vector<CString>>::iterator it = mapAllData.begin(); it != mapAllData.end(); it++)
// 	{
// 		BeamRebarInfo::BeamCommHoop stBeamCommHoop;
// 		for (unsigned int i = 0; i < it->second.size(); i++)
// 		{
// 			switch (i)
// 			{
// 			case 0:
// 				strncpy(stBeamCommHoop.label, CT2A(it->second.at(i)), sizeof(stBeamCommHoop.label));
// 				break;
// 			case 1:
// 				strncpy(stBeamCommHoop.rebarSize, CT2A(it->second.at(i)), sizeof(stBeamCommHoop.rebarSize));
// 				break;
// 			case 2:
// 				stBeamCommHoop.dSpacing = atoi(CT2A(it->second.at(i)));
// 				break;
// 			case 3:
// 				stBeamCommHoop.dEndPos = atoi(CT2A(it->second.at(i)));
// 				break;
// 			case 4:
// 				stBeamCommHoop.dStartPos = atoi(CT2A(it->second.at(i)));
// 				break;
// 			case 5:
// 			{
// 				//CString strCellValue = it->second.at(i);
// 				//auto find = std::find(g_listRebarType.begin(), g_listRebarType.end(), strCellValue);
// 				//int nIndex = (int)std::distance(g_listRebarType.begin(), find);
// 				//stBeamCommHoop.rebarType = nIndex;
// 				stBeamCommHoop.rebarType = vecRebarType[nIndex];
// 				break;
// 			}
// 			case 6:
// 			{
// 				stBeamCommHoop.dStart_N_Deep = atof(CT2A(it->second.at(i)));
// 				break;
// 			}
// 			case 7:
// 			{
// 				stBeamCommHoop.dEnd_N_Deep = atof(CT2A(it->second.at(i)));
// 				break;
// 			}
// 			case 8:
// 			{
// 				stBeamCommHoop.nPostion = atoi(CT2A(it->second.at(i)));
// 				break;
// 			}
// 			default:
// 				break;
// 			}
// 		}
// 		nIndex++;
// 		vecBeamCommHoop.push_back(stBeamCommHoop);
// 	}
// }

// void CBeamListCtrl::GetAllRebarData(vector<BeamRebarInfo::BeamAreaVertical>& vecBeamAreaData)
// {
// 	map<int, vector<CString> > mapAllData;
// 	GetAllData(mapAllData);
// 	vecBeamAreaData.clear();

// 	for (map<int, vector<CString>>::iterator it = mapAllData.begin(); it != mapAllData.end(); it++)
// 	{
// 		BeamRebarInfo::BeamAreaVertical stBeamAreaData;
// 		for (unsigned int i = 0; i < it->second.size(); i++)
// 		{
// 			switch (i)
// 			{
// 			case 0:
// 				strncpy(stBeamAreaData.label, CT2A(it->second.at(i)), sizeof(stBeamAreaData.label));
// 				break;
// 			case 1:
// 			{
// 				auto find = std::find(g_listRebarPosition.begin(), g_listRebarPosition.end(), it->second.at(i));
// 				int nIndex = (int)std::distance(g_listRebarPosition.begin(), find);
// 				stBeamAreaData.nPosition = nIndex;
// 				break;
// 			}
// 			case 2:
// 				stBeamAreaData.nTotNum = atoi(CT2A(it->second.at(i)));
// 				break;
// 			case 3:
// 				stBeamAreaData.dSpace = atof(CT2A(it->second.at(i)));
// 				break;
// 			case 4:
// 				stBeamAreaData.dStartOffset = atof(CT2A(it->second.at(i)));
// 				break;
// 			case 5:
// 				stBeamAreaData.dEndOffset = atof(CT2A(it->second.at(i)));
// 				break;
// 			default:
// 				break;
// 			}
// 		}
// 		vecBeamAreaData.push_back(stBeamAreaData);
// 	}
// }

void CWallRebarAssociatedCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	affectedElements.ClearHilite();
	affectedElements.Clear();
	ListCtrlEx::CListCtrlEx::OnLButtonDown(nFlags, point);
	CellIndex ix = Point2Cell(point);
	CString testring = GetItemText(ix.first, 1);
	std::string STDStr(CW2A(testring.GetString()));

	EditElementHandle eeh(g_mapidAndmodel[STDStr].ID, g_mapidAndmodel[STDStr].tModel);
	affectedElements.Insert(eeh);
	affectedElements.Hilite();
}



 void CBaseRebarListCtrl::GetAllRebarData(vector<PIT::ConcreteRebar>& vecListData)
 {
 	map<int, vector<CString> > mapAllData;
 	GetAllData(mapAllData);
 	vecListData.clear();
 	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
 	{
 		PIT::ConcreteRebar oneRebarInfo;
 		memset(&oneRebarInfo, 0, sizeof(PIT::ConcreteRebar));
 		for (int j = 0; j < it->second.size(); ++j)
 		{
 			switch (j)
 			{
 			case 0:
 				oneRebarInfo.rebarLevel = (int)distance(mapAllData.begin(), it);
 				break;
 			case 1:
 			{
 				CString strCellValue = it->second[j];
 				auto find = std::find(g_listRebarDir.begin(), g_listRebarDir.end(), strCellValue);
 				int nIndex = (int)std::distance(g_listRebarDir.begin(), find);
 				if (find == g_listRebarDir.end())
 				{
 					find = std::find(g_listSlabRebarDir.begin(), g_listSlabRebarDir.end(), strCellValue);
 					nIndex = (int)std::distance(g_listSlabRebarDir.begin(), find);
 				}
 				oneRebarInfo.rebarDir = nIndex;
 			}
 			break;
 			case 2:
 			{
 				CString strRebarSize = it->second[j];
 				strRebarSize = strRebarSize.Left(strRebarSize.GetLength() - 2);	//删掉mm
 				strcpy(oneRebarInfo.rebarSize, CT2A(strRebarSize));
 				break;
 			}
 			case 3:
 			{
 				CString strCellValue = it->second[j];
 				auto find = std::find(g_listRebarType.begin(), g_listRebarType.end(), strCellValue);
 				int nIndex = (int)std::distance(g_listRebarType.begin(), find);
 				oneRebarInfo.rebarType = nIndex;
 			}
 			break;
 			case 4:
 				oneRebarInfo.spacing = atof(CT2A(it->second[j]));
 				break;
 			case 5:
 				oneRebarInfo.startOffset = atof(CT2A(it->second[j]));
 				break;
 			case 6:
 				oneRebarInfo.endOffset = atof(CT2A(it->second[j]));
 				break;
 			default:
 				break;
 				// 			case 5:
 				// 				oneRebarInfo.startOffset = atof(CT2A(it->second[j]));
 				// 				break;
 				// 			case 6:
 				// 				oneRebarInfo.endOffset = atof(CT2A(it->second[j]));
 				// 				break;
 				// 			case 7:
 				// 				oneRebarInfo.levelSpace = atof(CT2A(it->second[j]));
 				// 				break;
 				// 			case 8:
 				// 				oneRebarInfo.datachange = 0;
 				// 				break;
 			}
 		}
 		vecListData.push_back(oneRebarInfo);
 	}
 }

BEGIN_MESSAGE_MAP(CBreakEllipseWallListCtrl, CListCtrlEx)
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, &CBreakEllipseWallListCtrl::OnLvnEndlabeledit)
END_MESSAGE_MAP()
void CBreakEllipseWallListCtrl::GetAllRebarData(vector<PIT::BreakAngleData>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	vecListData.clear();
	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
	{
		PIT::BreakAngleData oneAngleRange;
		memset(&oneAngleRange, 0, sizeof(PIT::BreakAngleData));
		for (int j = 0; j < it->second.size(); ++j)
		{
			switch (j)
			{
			case 1:
			{
				CString strBeginAngle = it->second[j];
				oneAngleRange.beginAngle = _ttof(strBeginAngle);
				break;
			}
			case 2:
			{
				CString strEndAngle = it->second[j];
				oneAngleRange.endAngle = _ttof(strEndAngle);
				break;
			}		
			default:
				break;
			}
		}
		vecListData.push_back(oneAngleRange);
	}
}

void CBreakEllipseWallListCtrl::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	// Update the item text with the new text
	if (pDispInfo->item.iSubItem == 0)	//如果点击的是第1列
	{
		//判断输入的值是否小于100
		if (g_wallRebarInfo.concrete.m_SlabRebarMethod != 2 && atoi(CT2A(pDispInfo->item.pszText)) > 100)
		{
			GetParent()->MessageBox(L"输入值不能小于100", L"提示", MB_OK);
			*pResult = 0;
			return;
		}
	}
	else if (pDispInfo->item.iSubItem == 1)	//如果点击的是第2列
	{
		double strAngle = atof(CT2A(pDispInfo->item.pszText));
		//判断输入的值是否小于100
		if (strAngle > 360 || strAngle < 0)
		{
			GetParent()->MessageBox(L"输入值在0°-360°之间", L"提示", MB_OK);
			*pResult = 0;
			return;
		}

		vector<PIT::BreakAngleData> vecListData;
		GetAllRebarData(vecListData);
		double endAngle = vecListData.at(pDispInfo->item.iItem).endAngle;
		if (fabs(endAngle - strAngle) > 180)
		{
			GetParent()->MessageBox(L"环墙断开后的角度应小于180°", L"提示", MB_OK);
			*pResult = 0;
			return;
		}
		vecListData.at(pDispInfo->item.iItem).beginAngle = strAngle;

		CBreakEllipseWallDlg *dlg = dynamic_cast<CBreakEllipseWallDlg*>(GetParent());
		if (dlg != nullptr)
		{
			dlg->SetListData(vecListData);		
		}
	}
	else if (pDispInfo->item.iSubItem == 2)
	{
		double endAngle = atof(CT2A(pDispInfo->item.pszText));
		//判断输入的值是否小于100
		if (endAngle > 360 || endAngle < 0)
		{
			GetParent()->MessageBox(L"输入值在0°-360°之间", L"提示", MB_OK);
			*pResult = 0;
			return;
		}

		vector<PIT::BreakAngleData> vecListData;
		GetAllRebarData(vecListData);
		double strAngle = vecListData.at(pDispInfo->item.iItem).beginAngle;
		if (fabs(endAngle - strAngle) > 180)
		{
			GetParent()->MessageBox(L"环墙断开后的角度应小于180°", L"提示", MB_OK);
			*pResult = 0;
			return;
		}
		vecListData.at(pDispInfo->item.iItem).endAngle = endAngle;

		CBreakEllipseWallDlg *dlg = dynamic_cast<CBreakEllipseWallDlg*>(GetParent());
		if (dlg != nullptr)
		{
			dlg->SetListData(vecListData);
		}
	}
	SetItemText(pDispInfo->item.iItem, pDispInfo->item.iSubItem, pDispInfo->item.pszText);
	GetParent()->SendMessage(WM_VALIDATE, GetDlgCtrlID(), (LPARAM)pDispInfo);
	*pResult = 0;
}
