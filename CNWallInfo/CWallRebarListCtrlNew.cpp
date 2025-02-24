#include "_ustation.h"
#include "CWallRebarListCtrlNew.h"
#include "../../ConstantsDef.h"
#include "CWallMainRebarDlgNew.h"
#include "BentlyCommonfile.h"
#include "ScanIntersectTool.h"
#include "CCNCutRebarDlg.h"
#include "../../resource.h"

BEGIN_MESSAGE_MAP(CMainRebarListCtrlNew, CListCtrlEx)
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, &CMainRebarListCtrlNew::OnLvnEndlabeledit)
END_MESSAGE_MAP()

void CMainRebarListCtrlNew::GetAllRebarData(vector<PIT::ConcreteRebar>& vecListData)
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
				}
				oneRebarInfo.rebarDir = nIndex;
			}
			break;
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
				oneRebarInfo.datachange = nIndex;
				break;
			}
				
			}
		}
		vecListData.push_back(oneRebarInfo);
	}
}

void CMainRebarListCtrlNew::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
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
	else if (pDispInfo->item.iSubItem == 8)	//如果点击的是第9列
	{
		vector<PIT::ConcreteRebar> vecRebarData;
		GetAllRebarData(vecRebarData);

		CString strTmp = pDispInfo->item.pszText;
		if (strTmp == "正面")
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

		CWallMainRebarDlgNew *dlg = dynamic_cast<CWallMainRebarDlgNew*>(GetParent());
		dlg->SetListRowData(vecRebarData);
		dlg->UpdateRebarList();
	}
	SetItemText(pDispInfo->item.iItem, pDispInfo->item.iSubItem, pDispInfo->item.pszText);
	GetParent()->SendMessage(WM_VALIDATE, GetDlgCtrlID(), (LPARAM)pDispInfo);

	vector<PIT::ConcreteRebar> vecRebarData;
	CWallMainRebarDlgNew *dlg = dynamic_cast<CWallMainRebarDlgNew*>(GetParent());

	dlg->m_listMainRebar.GetAllRebarData(vecRebarData);
	dlg->SetListRowData(vecRebarData);

	
	*pResult = 0;
}

void CRebarEndTypeListCtrlNew::GetAllRebarData(vector<PIT::EndType>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	vector<PIT::EndType> vecListDataTmp(vecListData.size());
	std::copy(vecListData.begin(), vecListData.end(), vecListDataTmp.begin());
	vecListData.clear();
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
		vecListData.push_back(oneEndType);
	}
}



BEGIN_MESSAGE_MAP(CCNCutRebarList, CListCtrlEx)
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, &CCNCutRebarList::OnLvnEndlabeledit)
END_MESSAGE_MAP()

void CCNCutRebarList::GetAllRebarData(vector<CNCutRebarInfo>& vecListData)
{
	map<int, vector<CString> > mapAllData;
	GetAllData(mapAllData);
	vecListData.clear();
	for (auto it = mapAllData.begin(); it != mapAllData.end(); ++it)
	{
		CNCutRebarInfo stCutRebarInfo;
		memset(&stCutRebarInfo, 0, sizeof(CNCutRebarInfo));
		for (int j = 0; j < it->second.size(); ++j)
		{
			switch (j)
			{
				case 0:
				{
					CString strCellValue = it->second[j];
					stCutRebarInfo.nIndex = atoi(CT2A(strCellValue));
				}
				break;
				case 1:
				{
					CString strCellValue = it->second[j];
					stCutRebarInfo.dLength = atof(CT2A(strCellValue));
				}
				break;
				default:
				{
					break;
				}
			}
		}
		vecListData.push_back(stCutRebarInfo);
	}
}


afx_msg void CCNCutRebarList::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	// Update the item text with the new text

	if (pDispInfo->item.iSubItem == 1)	//如果点击的是第2列
	{
		SetItemText(pDispInfo->item.iItem, pDispInfo->item.iSubItem, pDispInfo->item.pszText);
		GetParent()->SendMessage(WM_VALIDATE, GetDlgCtrlID(), (LPARAM)pDispInfo);

		vector<CNCutRebarInfo> vecCutInfo;
		CCNCutRebarDlg *dlg = dynamic_cast<CCNCutRebarDlg*>(GetParent());

		dlg->m_ListCutRebar.GetAllRebarData(vecCutInfo);
		if (vecCutInfo.size() > 0)
		{
			vecCutInfo.pop_back();
		}
		dlg->SetvecCutInfo(vecCutInfo);
		dlg->SetListDefaultData();
		dlg->UpdateCutList();
	}

	*pResult = 0;
}


void CTwinBarListCtrlNew::GetAllRebarData(vector<TwinBarSet::TwinBarLevelInfo>& vecListData)
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


BEGIN_MESSAGE_MAP(CWallRebarAssociatedCtrlNew, CListCtrlEx)
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, &CWallRebarAssociatedCtrlNew::OnLvnEndlabeledit)
END_MESSAGE_MAP()

void CWallRebarAssociatedCtrlNew::GetAllRebarData(vector<PIT::AssociatedComponent>& vecListData)
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

void CWallRebarAssociatedCtrlNew::OnLButtonDown(UINT nFlags, CPoint point)
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



