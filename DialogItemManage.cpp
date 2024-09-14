#include "_ustation.h"
#include "DialogItemManage.h"
#include "BentlyCommonfile.h"
#include "WallRebarDlg.h"

using namespace WallRebarItem;
	

WallRebarItem::WallMainRebarList *WallMainRebarList::pRebarList = NULL;
WallRebarItem::RebarLapOptionsList *RebarLapOptionsList::pLapOptionsList = NULL;
WallRebarItem::RebarEndTypeList *RebarEndTypeList::pEndTypeList = NULL;

int CListModel::addRow(ListRowP pRow)
{
	if (pRow == NULL || m_pListModel == NULL)
		return ERROR;
	return mdlListModel_addRow(m_pListModel, pRow);
}

int CListModel::insertRow(ListRowP pRow,int index)
{
	if (pRow == NULL || m_pListModel == NULL)
		return ERROR;
	return mdlListModel_insertRow(m_pListModel, pRow, index);
}

int CListModel::removeRow(ListRowP pRow, bool bDestroyRow)
{
	if (pRow == NULL || m_pListModel == NULL)
		return ERROR;
	return mdlListModel_removeRow(m_pListModel, pRow, bDestroyRow);
}

int CListModel::removeRow(int iRow, bool bDestroyRow)
{
	if (m_pListModel == NULL)
		return ERROR;
	return mdlListModel_removeRowAtIndex(m_pListModel, iRow, bDestroyRow);
}

int CListModel::removeRow(int iRow, int nRows, bool bDestroyRow)
{
	if (m_pListModel == NULL)
		return ERROR;
	return mdlListModel_removeRows(m_pListModel, iRow, nRows, bDestroyRow);
}

int CListModel::empty(bool bDestroyRow)
{
	if (m_pListModel == NULL)
		return ERROR;
	return	mdlListModel_empty(m_pListModel, bDestroyRow);
}

int CListModel::getId()
{
	if (m_pListModel == NULL)
		return ERROR;
	return mdlListModel_getId(m_pListModel);
}
int CListModel::getColumnCount()
{
	if (m_pListModel == NULL)
		return ERROR;
	return mdlListModel_getColumnCount(m_pListModel);
}
int CListModel::getRowCount()
{
	if (m_pListModel == NULL)
		return ERROR;
	return mdlListModel_getRowCount(m_pListModel);
}
int CListModel::getRowIndex(ListRowCP pRow)
{
	if (m_pListModel == NULL || pRow == NULL)
		return ERROR;
	return mdlListModel_getRowIndex(m_pListModel,pRow);
}

ListCellP CListModel::getCell(int iRow, int iCol)
{
	if (m_pListModel == NULL)
		return NULL;
	return mdlListModel_getCellAtIndexes(m_pListModel, iRow, iCol);
}
ListColumnP CListModel::getColumnAtIndex(int colIndex)
{
	if (m_pListModel == NULL)
		return NULL;
	return mdlListModel_getColumnAtIndex(m_pListModel, colIndex);
}
ListColumnP CListModel::getColumnById(int colId)
{
	if (m_pListModel == NULL)
		return NULL;
	return mdlListModel_getColumnById(m_pListModel, colId);
}
ListColumnP CListModel::getColumnByName(WCharCP pwName)
{
	if (m_pListModel == NULL || pwName == NULL)
		return NULL;
	return mdlListModel_getColumnByName(m_pListModel, pwName);
}
ListRowP CListModel::getFirstRow()
{
	if (m_pListModel == NULL)
		return NULL;
	return mdlListModel_getFirstRow(m_pListModel);
}
ListRowP CListModel::getLastRow()
{
	if (m_pListModel == NULL)
		return NULL;
	return mdlListModel_getLastRow(m_pListModel);
}
ListRowP CListModel::getNextRow(ListRowCP pRow)
{
	if (m_pListModel == NULL || pRow == NULL)
		return NULL;
	return mdlListModel_getNextRow(m_pListModel, pRow);
}
ListRowP CListModel::getPrevRow(ListRowCP pRow)
{
	if (m_pListModel == NULL || pRow == NULL)
		return NULL;
	return mdlListModel_getPrevRow(m_pListModel, pRow);
}
ListRowP CListModel::getRowAtIndex(int index)
{
	if (m_pListModel == NULL)
		return NULL;
	return mdlListModel_getRowAtIndex(m_pListModel, index);
}

void ListItem::UpdateListBox()
{
	m_pListBox->empty(true);
	bool ret = createListModel();
	if (ret)
	{
		MSDialogP pDlg = mdlDialog_find(m_rscDlgId, NULL);
		if (pDlg != NULL && m_pListBox != NULL)
		{
			ListModelP pListModel = m_pListBox->getListModel();
			DialogItem *pListBoxItem = pDlg->GetItemByTypeAndId(RTYPE_ListBox, m_rscListId);
			if (pListBoxItem)
			{
				mdlDialog_listBoxSetListModelP(pListBoxItem->rawItemP, pListModel, 0);
				mdlDialog_itemSynch(pDlg, pListBoxItem->itemIndex);
			}
		}
	}
}

void ListItem::SetCellDefaultText(DialogItemMessage *dimP)
{
	if (dimP == NULL)
		return;

	WCharCP       stringW;
	bool          found = false;
	int           row = -1, col = -1;

	DialogItem      *listBox = mdlDialog_itemGetByTypeAndId(dimP->db, RTYPE_ListBox, m_rscListId, 0);
	if (NULL == listBox)
		return;
	mdlDialog_listBoxGetNextSelection(&found, &row, &col, listBox->rawItemP);
	if (!found)
		return;
	ListModel   *listModel = mdlDialog_listBoxGetListModelP(listBox->rawItemP);
	if (NULL == listModel)
		return;
	ListRow *   rowP = mdlListModel_getRowAtIndex(listModel, row);
	if (NULL == rowP)
		return;
	ListCell *  cellP = mdlListRow_getCellAtIndex(rowP, col);
	if (SUCCESS == mdlListCell_getDisplayText(cellP, &stringW))
	{
		dimP->u.value.msValueDescrP->SetWChar(stringW);
	}
	dimP->u.value.hookHandled = true;
}

void WallMainRebarList::SetListRowData(const PIT::ConcreteRebar& rebarData,int iRowIndex)
{
	if (iRowIndex >= m_vecRebarData.size())
		return;
	m_vecRebarData[iRowIndex] = rebarData;
}

void WallMainRebarList::SetListRowData(const std::vector<PIT::ConcreteRebar>& vecRebarData)
{
	m_vecRebarData = vecRebarData;
}

void WallMainRebarList::SetDefaultListRowData()
{
	if (m_vecRebarData.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < GetListRow(); i++)
		{
			PIT::ConcreteRebar oneRebarData;
			int dir =  i & 0x01;
			if (0 == i)
				oneRebarData = { i,dir,"12",0,100,0,0,0 };
			else
			{
				double levelSpace;
				levelSpace = ((i + 1) & 0x01) * 100.0;
				oneRebarData = { i,dir,"12",0,100,0,0,levelSpace };
			}
			m_vecRebarData.push_back(oneRebarData);
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = GetListRow() - (int)m_vecRebarData.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				PIT::ConcreteRebar oneRebarData = { i,0,"12",0,100,0,0,100 };
				m_vecRebarData.push_back(oneRebarData);
			}
		}
		if (iOffset < 0)
		{
			iOffset *= -1;
			for (int i = 0; i < iOffset; i++)
			{
				m_vecRebarData.pop_back();
			}
		}
	}
}
/**
* brief 创建主要配筋编辑列表
* param[in]  nCols   列表列数
* return 返回下拉框配置控件指针
*/
bool WallMainRebarList::createListModel()
{
	if (GetListModelPtr() == NULL)
		return false;

	//list表参数定义
	ListModelP		pListModel = GetListModelPtr()->getListModel();
	ListRow*		pRow = NULL;
	int				rowIndex;//行
	int				colIndex;//列

	StringListP	strListRebarDir = mdlStringList_loadResource(NULL, STRINGLISTID_RebarDir);
	StringListP	strListRebarSize = mdlStringList_loadResource(NULL, STRINGLISTID_RebarSize);
	StringListP	strListRebarType = mdlStringList_loadResource(NULL, STRINGLISTID_RebarType);

	SetDefaultListRowData();
	int nCols = GetListModelPtr()->getColumnCount();
	for (rowIndex = 0; rowIndex < GetListRow(); rowIndex++)
	{
		pRow = mdlListRow_create(pListModel);

		//
		PIT::ConcreteRebar rebarInfo = m_vecRebarData[rowIndex];
		for (colIndex = 0; colIndex < nCols; colIndex++)
		{
			ListCell*	pCell = NULL;

			pCell = mdlListRow_getCellAtIndex(pRow, colIndex);

			switch (colIndex)//根据不同的列设置不同的值
			{
			case 0:		//层
			{
				if (0 == rowIndex)
				{
					mdlListCell_setStringValue(pCell, L"1LX", TRUE);
				}
				else
				{
					WPrintfString strValue(L"%dX", rowIndex);
					mdlListCell_setStringValue(pCell, strValue.GetWCharCP(), TRUE);
				}
				break;
			}

			case 1:		//方向
			{
				WCharCP strRebarDir;
				mdlStringList_getMember(&strRebarDir, NULL,strListRebarDir, rebarInfo.rebarDir);
				if (strRebarDir != NULL)
				{
					mdlListCell_setStringValue(pCell, strRebarDir, TRUE);
				}
				mdlListCell_setEditor(pCell, RTYPE_ComboBox, ListBoxComBoBoxID_RebarDir, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 2:		//直径
			{
				strcat(rebarInfo.rebarSize, "mm");
				WString strSize(rebarInfo.rebarSize);
				mdlListCell_setStringValue(pCell, strSize.GetWCharCP(), TRUE);
				mdlListCell_setEditor(pCell, RTYPE_ComboBox, ListBoxComBoBoxID_RebarSize, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 3:		//型号
			{
				WCharCP strRebarType;
				mdlStringList_getMember(&strRebarType, NULL, strListRebarType, rebarInfo.rebarType);
				if (strRebarType != NULL)
				{
					mdlListCell_setStringValue(pCell, strRebarType, TRUE);
				}
				mdlListCell_setEditor(pCell, RTYPE_ComboBox, ListBoxComBoBoxID_RebarType, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 4:		//钢筋间距
			{
				WPrintfString str(L"%.2f", rebarInfo.spacing);
				mdlListCell_setDoubleValue(pCell, rebarInfo.spacing);
				mdlListCell_setStringValue(pCell, str.GetWCharCP(), TRUE);
				mdlListCell_setEditor(pCell, RTYPE_Text, ListBoxTextID_RebarSpace, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 5:		//起点偏移
			{
				WPrintfString str(L"%.2f", rebarInfo.startOffset);
				mdlListCell_setDoubleValue(pCell, rebarInfo.startOffset);
				mdlListCell_setStringValue(pCell, str.GetWCharCP(), TRUE);
				mdlListCell_setEditor(pCell, RTYPE_Text, ListBoxTextID_RebarStartOffset, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 6:		//终点偏移
			{
				WPrintfString str(L"%.2f", rebarInfo.endOffset);
				mdlListCell_setDoubleValue(pCell, rebarInfo.endOffset);
				mdlListCell_setStringValue(pCell, str.GetWCharCP(), TRUE);
				mdlListCell_setEditor(pCell, RTYPE_Text, ListBoxTextID_RebarEndOffset, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 7:		//与前层间距
			{
				WPrintfString str(L"%.2f", rebarInfo.levelSpace);
				mdlListCell_setDoubleValue(pCell, rebarInfo.levelSpace);
				mdlListCell_setStringValue(pCell, str.GetWCharCP(), TRUE);
				mdlListCell_setEditor(pCell, RTYPE_Text, ListBoxTextID_RebarLevelSpace, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 8:		//数据交换
			{
				mdlListCell_setStringValue(pCell, L"空", TRUE);
				mdlListCell_setEditor(pCell, RTYPE_ComboBox, ListBoxComBoBoxID_RebarDataChange, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			default:
				break;
			}
		}
		GetListModelPtr()->addRow(pRow);
	}
	return true;
}


void WallMainRebarList::UpdateCellText(DialogItemMessage *dimP, const PIT::ConcreteRebar& rebarData)
{
	if (dimP == NULL)
	{
		return;
	}

	ListRowP rowP = NULL;
	//编辑框关闭刷新相应单元格数据
 	DialogItem   *listBox = mdlDialog_itemGetByTypeAndId(dimP->db, RTYPE_ListBox,GetListItemID(), 0);
	mdlDialog_listBoxNRowsChangedRedraw(listBox->rawItemP, TRUE);
	int rowIndex = -1;
	int colIndex = -1;
	bool found = false;

	if (SUCCESS == mdlDialog_listBoxGetNextSelection(&found, &rowIndex, &colIndex, listBox->rawItemP))
	{
		if (NULL != (rowP = mdlListModel_getRowAtIndex(GetListModelPtr()->getListModel(), rowIndex)))
		{
			switch (colIndex)//根据不同的列设置不同的值
			{
			case 0:
				m_vecRebarData[rowIndex].rebarLevel = rebarData.rebarLevel;
				break;
			case 1:
				m_vecRebarData[rowIndex].rebarDir = rebarData.rebarDir;
				break;
			case 2:
				strcpy(m_vecRebarData[rowIndex].rebarSize, rebarData.rebarSize);
				break;
			case 3:
				m_vecRebarData[rowIndex].rebarType = rebarData.rebarType;
				break;
			case 4:
				m_vecRebarData[rowIndex].spacing = rebarData.spacing;
				break;
			case 5:
				m_vecRebarData[rowIndex].startOffset = rebarData.startOffset;
				break;
			case 6:
				m_vecRebarData[rowIndex].endOffset = rebarData.endOffset;
				break;
			case 7:
				m_vecRebarData[rowIndex].levelSpace = rebarData.levelSpace;
				break;
			case 8:
			{
				if (0 != rebarData.datachange)
				{
					PIT::ConcreteRebar rebar = m_vecRebarData[rowIndex];
					m_vecRebarData[rowIndex] = m_vecRebarData[rebarData.datachange-1];
					m_vecRebarData[rebarData.datachange-1] = rebar;
				}
			}
				break;
			}
		}
		//更新
		UpdateListBox();
	}
}

WallMainRebarList* WallRebarItem::WallMainRebarList::getInstance()
{
	if (!pRebarList)
		pRebarList = new WallRebarItem::WallMainRebarList(9, g_wallRebarInfo.concrete.rebarLevelNum, DIALOGID_WallRebar, LISTBOXID_Rebar);
	return pRebarList;
}

void RebarEndTypeList::SetListRowData(const PIT::EndType& endTypeData, int iRowIndex)
{
	if (iRowIndex >= m_vecEndTypeData.size())
		return;
	m_vecEndTypeData[iRowIndex] = endTypeData;
}


void RebarEndTypeList::SetListRowData(const std::vector<PIT::EndType>& vecEndTypeData)
{
	m_vecEndTypeData = vecEndTypeData;
}

RebarEndTypeList * WallRebarItem::RebarEndTypeList::getInstance()
{
	if (!pEndTypeList)
		pEndTypeList = new RebarEndTypeList(5, g_wallRebarInfo.concrete.rebarLevelNum * 2, DIALOGID_WallRebar, LISTBOXID_EndType);
	return pEndTypeList;
}

void RebarEndTypeList::SetDefaultListRowData()
{
	if (m_vecEndTypeData.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < GetListRow(); i++)
		{
			PIT::EndType oneEndTypeData = { 0,0,0 };
			m_vecEndTypeData.push_back(oneEndTypeData);
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = GetListRow() - (int)m_vecEndTypeData.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				PIT::EndType oneEndTypeData = { 0,0,0 };
				m_vecEndTypeData.push_back(oneEndTypeData);
			}
		}
		if (iOffset < 0)
		{
			iOffset *= -1;
			for (int i = 0; i < iOffset; i++)
			{
				m_vecEndTypeData.pop_back();
			}
		}
	}
}

/**
* brief 创建端部样式编辑列表
* param[in]  nCols   列表列数
* return 返回下拉框配置控件指针
*/
bool RebarEndTypeList::createListModel()
{
	if (GetListModelPtr() == NULL)
		return false;

	//list表参数定义
	ListModelP		pListModel = GetListModelPtr()->getListModel();
	ListRow*		pRow = NULL;
	int				rowIndex;//行
	int				colIndex;//列

	StringListP	strListEndType = mdlStringList_loadResource(NULL, STRINGLISTID_EndType);

	SetDefaultListRowData();
	int nCols = GetListModelPtr()->getColumnCount();
	for (rowIndex = 0; rowIndex < GetListRow(); rowIndex++)
	{
		pRow = mdlListRow_create(pListModel);

		//
		PIT::EndType endTypeInfo = m_vecEndTypeData[rowIndex];
		for (colIndex = 0; colIndex < nCols; colIndex++)
		{
			ListCell*	pCell = NULL;

			pCell = mdlListRow_getCellAtIndex(pRow, colIndex);

			switch (colIndex)//根据不同的列设置不同的值
			{
			case 0:		//层
			{
				if (rowIndex == 0)
					mdlListCell_setStringValue(pCell, L"1LX始端", TRUE);
				else if(rowIndex == 1)
					mdlListCell_setStringValue(pCell, L"1LX终端", TRUE);
				else
				{
					if (rowIndex % 2 == 0)
					{
						WPrintfString strIndex(L"%dX始端",rowIndex/2);
						mdlListCell_setStringValue(pCell, strIndex, TRUE);
					}
					else
					{
						WPrintfString strIndex(L"%dX终端", rowIndex/2);
						mdlListCell_setStringValue(pCell, strIndex, TRUE);
					}
				}
				break;
			}

			case 1:		//类型
			{
				WCharCP strEndType;
				mdlStringList_getMember(&strEndType, NULL, strListEndType, endTypeInfo.endType);
				if (strEndType != NULL)
				{
					mdlListCell_setStringValue(pCell, strEndType, TRUE);
				}
				int iret = -1;
				iret = mdlListCell_setEditor(pCell, RTYPE_ComboBox, ListBoxComBoBoxID_EndType, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 2:		//端部属性
			{
				RgbColorDef s_RGBFront = { 130,130,130 };
				RgbColorDef s_RGBBack = { 111,111,111 };
				BSIColorDescr*	pForegroundColor;
				BSIColorDescr*	pBackgroundColor;
				pForegroundColor = mdlColorPal_getColorDescr(NULL, colIndex);
				pBackgroundColor = mdlColorPal_getColorDescr(NULL, colIndex);
				mdlColorDescr_setByRgb(pForegroundColor, &s_RGBFront, FALSE);
				mdlColorDescr_setByRgb(pBackgroundColor, &s_RGBBack, FALSE);
				mdlListCell_setBgColorDescr(pCell, pBackgroundColor);
				mdlListCell_setDisplayText(pCell, L"...");
				break;
			}
			case 3:		//偏移
			{
				WPrintfString str(L"%.2f", endTypeInfo.offset);
				mdlListCell_setDoubleValue(pCell, endTypeInfo.offset);
				mdlListCell_setStringValue(pCell, str.GetWCharCP(), TRUE);
				mdlListCell_setEditor(pCell, RTYPE_Text, ListBoxTextID_Offset, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 4:		//旋转角
			{
				WPrintfString str(L"%.2f", endTypeInfo.rotateAngle);
				mdlListCell_setDoubleValue(pCell, endTypeInfo.rotateAngle);
				mdlListCell_setStringValue(pCell, str.GetWCharCP(), TRUE);
				mdlListCell_setEditor(pCell, RTYPE_Text, ListBoxTextID_RotateAngle, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			default:
				break;
			}
		}
		GetListModelPtr()->addRow(pRow);
	}
	return true;
}


void RebarEndTypeList::UpdateCellText(DialogItemMessage *dimP, const PIT::EndType& endTypeData)
{
	if (dimP == NULL)
		return;

	ListRowP rowP = NULL;
	//编辑框关闭刷新相应单元格数据
	DialogItem   *listBox = mdlDialog_itemGetByTypeAndId(dimP->db, RTYPE_ListBox, GetListItemID(), 0);
	mdlDialog_listBoxNRowsChangedRedraw(listBox->rawItemP, TRUE);
	int rowIndex = -1;
	int colIndex = -1;
	bool found = false;

	if (SUCCESS == mdlDialog_listBoxGetNextSelection(&found, &rowIndex, &colIndex, listBox->rawItemP))
	{
		if (NULL != (rowP = mdlListModel_getRowAtIndex(GetListModelPtr()->getListModel(), rowIndex)))
		{
			switch (colIndex)//根据不同的列设置不同的值
			{
			case 0:
				break;
			case 1:
				m_vecEndTypeData[rowIndex].endType = endTypeData.endType;
				break;
			case 2:
				break;
			case 3:
				m_vecEndTypeData[rowIndex].offset = endTypeData.offset;
				break;
			case 4:
				m_vecEndTypeData[rowIndex].rotateAngle = endTypeData.rotateAngle;
				break;
			break;
			}
		}
		//更新
		UpdateListBox();
	}
}

void RebarLapOptionsList::SetListRowData(const PIT::LapOptions& endTypeData, int iRowIndex)
{
	if (iRowIndex >= m_vecLapOptionsData.size())
		return;
	m_vecLapOptionsData[iRowIndex] = endTypeData;
}


void RebarLapOptionsList::SetListRowData(const std::vector<PIT::LapOptions>& vecEndTypeData)
{
	m_vecLapOptionsData = vecEndTypeData;
}

RebarLapOptionsList * WallRebarItem::RebarLapOptionsList::getInstance()
{
	if (!pLapOptionsList)
		pLapOptionsList = new WallRebarItem::RebarLapOptionsList(7, g_wallRebarInfo.concrete.rebarLevelNum, DIALOGID_WallRebar, LISTBOXID_LapOptions);
	return pLapOptionsList;
}

void RebarLapOptionsList::SetDefaultListRowData()
{
	if (m_vecLapOptionsData.empty())//无数据时根据层数添加默认数据
	{
		for (int i = 0; i < GetListRow(); i++)
		{
			PIT::LapOptions oneEndTypeData = { i,0,0,12000,12000,0,0 };
			m_vecLapOptionsData.push_back(oneEndTypeData);
		}
	}
	else //往后删除数据或添加默认数据
	{
		int iOffset = GetListRow() - (int)m_vecLapOptionsData.size();
		if (iOffset > 0)
		{
			for (int i = 0; i < iOffset; i++)
			{
				PIT::LapOptions oneEndTypeData = { i,0,0,12000,12000,0,0 };
				m_vecLapOptionsData.push_back(oneEndTypeData);
			}
		}
		if (iOffset < 0)
		{
			iOffset *= -1;
			for (int i = 0; i < iOffset; i++)
			{
				m_vecLapOptionsData.pop_back();
			}
		}
	}
}

/**
* brief 创建搭接选项编辑列表
* param[in]  nCols   列表列数
* return 返回下拉框配置控件指针
*/
bool RebarLapOptionsList::createListModel()
{
	if (GetListModelPtr() == NULL)
		return false;

	//list表参数定义
	ListModelP		pListModel = GetListModelPtr()->getListModel();
	ListRow*		pRow = NULL;
	int				rowIndex;//行
	int				colIndex;//列

	StringListP	strListConnectMethod = mdlStringList_loadResource(NULL, STRINGLISTID_ConnectMethod);
	StringListP	strListIsStaggered = mdlStringList_loadResource(NULL, STRINGLISTID_IsStaggered);
	
	SetDefaultListRowData();
	int nCols = GetListModelPtr()->getColumnCount();
	for (rowIndex = 0; rowIndex < GetListRow(); rowIndex++)
	{
		pRow = mdlListRow_create(pListModel);

		//
		PIT::LapOptions lapOption = m_vecLapOptionsData[rowIndex];
		for (colIndex = 0; colIndex < nCols; colIndex++)
		{
			ListCell*	pCell = NULL;

			pCell = mdlListRow_getCellAtIndex(pRow, colIndex);

			switch (colIndex)//根据不同的列设置不同的值
			{
			case 0:		//层
			{
				if (0 == rowIndex)
				{
					mdlListCell_setStringValue(pCell, L"1LX", TRUE);
				}
				else
				{
					WPrintfString strValue(L"%dX", rowIndex);
					mdlListCell_setStringValue(pCell, strValue.GetWCharCP(), TRUE);
				}
				break;
			}
			case 1:		//连接方式
			{
				WCharCP strConnectMethod;
				mdlStringList_getMember(&strConnectMethod, NULL, strListConnectMethod, lapOption.connectMethod);
				if (strConnectMethod != NULL)
				{
					mdlListCell_setStringValue(pCell, strConnectMethod, TRUE);
				}
				int iret = -1;
				iret = mdlListCell_setEditor(pCell, RTYPE_ComboBox, ListBoxComBoBoxID_ConnectMethod, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 2:		//搭接长度
			{
				WPrintfString str(L"%.2f", lapOption.lapLength);
				mdlListCell_setDoubleValue(pCell, lapOption.lapLength);
				mdlListCell_setStringValue(pCell, str.GetWCharCP(), TRUE);
				mdlListCell_setEditor(pCell, RTYPE_Text, ListBoxTextID_LapLength, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 3:		//库存长度
			{
				WPrintfString str(L"%.2f", lapOption.stockLength);
				mdlListCell_setDoubleValue(pCell, lapOption.stockLength);
				mdlListCell_setStringValue(pCell, str.GetWCharCP(), TRUE);
				mdlListCell_setEditor(pCell, RTYPE_Text, ListBoxTextID_StockLength, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 4:		//加工长度
			{
				WPrintfString str(L"%.2f", lapOption.millLength);
				mdlListCell_setDoubleValue(pCell, lapOption.millLength);
				mdlListCell_setStringValue(pCell, str.GetWCharCP(), TRUE);
				mdlListCell_setEditor(pCell, RTYPE_Text, ListBoxTextID_MillLength, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 5:		//是否交错
			{
				WCharCP strIsStaggered;
				mdlStringList_getMember(&strIsStaggered, NULL, strListIsStaggered, lapOption.isStaggered);
				if (strIsStaggered != NULL)
				{
					mdlListCell_setStringValue(pCell, strIsStaggered, TRUE);
				}
				int iret = -1;
				iret = mdlListCell_setEditor(pCell, RTYPE_ComboBox, ListBoxComBoBoxID_IsStaggered, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			case 6:		//交错长度
			{
				WPrintfString str(L"%.2f", lapOption.staggeredLength);
				mdlListCell_setDoubleValue(pCell, lapOption.staggeredLength);
				mdlListCell_setStringValue(pCell, str.GetWCharCP(), TRUE);
				mdlListCell_setEditor(pCell, RTYPE_Text, ListBoxTextID_StaggeredLength, mdlSystem_getCurrMdlDesc(), false, true);
				break;
			}
			default:
				break;
			}
		}
		GetListModelPtr()->addRow(pRow);
	}
	return true;
}


void RebarLapOptionsList::UpdateCellText(DialogItemMessage *dimP, const PIT::LapOptions& lapOptionsData)
{
	if (dimP == NULL)
		return;

	ListRowP rowP = NULL;
	//编辑框关闭刷新相应单元格数据
	DialogItem   *listBox = mdlDialog_itemGetByTypeAndId(dimP->db, RTYPE_ListBox, GetListItemID(), 0);
	mdlDialog_listBoxNRowsChangedRedraw(listBox->rawItemP, TRUE);
	int rowIndex = -1;
	int colIndex = -1;
	bool found = false;

	if (SUCCESS == mdlDialog_listBoxGetNextSelection(&found, &rowIndex, &colIndex, listBox->rawItemP))
	{
		if (NULL != (rowP = mdlListModel_getRowAtIndex(GetListModelPtr()->getListModel(), rowIndex)))
		{
			switch (colIndex)//根据不同的列设置不同的值
			{
			case 0:
				break;
			case 1:
				m_vecLapOptionsData[rowIndex].connectMethod = lapOptionsData.connectMethod;
				break;
			case 2:
				m_vecLapOptionsData[rowIndex].lapLength = lapOptionsData.lapLength;
				break;
			case 3:
				m_vecLapOptionsData[rowIndex].stockLength = lapOptionsData.stockLength;
				break;
			case 4:
				m_vecLapOptionsData[rowIndex].millLength = lapOptionsData.millLength;
				break;
			case 5:
				m_vecLapOptionsData[rowIndex].isStaggered = lapOptionsData.isStaggered;
				break;
			case 6:
				m_vecLapOptionsData[rowIndex].staggeredLength = lapOptionsData.staggeredLength;
				break;
			default:
				break;
			}
		}
		//更新
		UpdateListBox();
	}
}