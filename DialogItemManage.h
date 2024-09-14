#pragma once
#include "CommonFile.h"

class CListModel
{
public:
	CListModel(int Col)
	{ 
		m_pListModel = mdlListModel_create(Col);
	}
	~CListModel()
	{
		if (m_pListModel != NULL)
			mdlListModel_destroy(m_pListModel, true);
	}

public:
	int addRow(ListRowP pRow);
	int insertRow(ListRowP	pRow,int index);
	int removeRow(ListRowP pRow,bool bDestroyRow);
	int removeRow(int iRow, bool bDestroyRow);
	int removeRow(int iRow, int nRows, bool bDestroyRow);
	int empty(bool bDestroyRow);

	int getId();
	int getColumnCount();
	int getRowCount();
	int getRowIndex(ListRowCP pRow);

	ListCellP getCell(int iRow, int iCol);
	ListColumnP getColumnAtIndex(int colIndex);
	ListColumnP getColumnById(int colId);
	ListColumnP getColumnByName(WCharCP pwName);
	ListRowP getFirstRow();
	ListRowP getLastRow();
	ListRowP getNextRow(ListRowCP pRow);
	ListRowP getPrevRow(ListRowCP pRow);
	ListRowP getRowAtIndex(int index);

	ListModelP getListModel() { return m_pListModel; }
private:
	ListModelP m_pListModel;
};

// template<typename DataType, int dlgId, int rscId>
// class ItemList
// {
// public:
// 	explicit ItemList(int listCol, int listRow) :m_pListBox(new CListModel(listCol)), m_iRows(listRow) {};
// 	~ItemList() {};
// protected:
// 	virtual bool createListModel() { return true; }
// 	void SetDefaultListRowData() { return true; }
// public:
// 	void UpdateListBox() 
// 	{
// 		m_pListBox->empty(true);
// 		bool ret = createListModel();
// 		if (ret)
// 		{
// 			MSDialogP pDlg = mdlDialog_find(dlgId, NULL);
// 			if (pDlg != NULL && m_pListBox != NULL)
// 			{
// 				ListModelP pListModel = m_pListBox->getListModel();
// 				DialogItem *pListBoxItem = pDlg->GetItemByTypeAndId(RTYPE_ListBox, rscId);
// 				mdlDialog_listBoxSetListModelP(pListBoxItem->rawItemP, pListModel, 0);
// 				mdlDialog_itemSynch(pDlg, pListBoxItem->itemIndex);
// 			}
// 		}
// 	}
// 
// 	void SetCellDefaultText(DialogItemMessage *dimP)
// 	{
// 		if (dimP == NULL)
// 			return;
// 		WCharCP       stringW;
// 		bool          found = false;
// 		int           row = -1, col = -1;
// 		DialogItem      *listBox = mdlDialog_itemGetByTypeAndId(dimP->db, RTYPE_ListBox, rscId, 0);
// 		if (NULL == listBox)
// 			return;
// 		mdlDialog_listBoxGetNextSelection(&found, &row, &col, listBox->rawItemP);
// 		if (!found)
// 			return;
// 		ListModel   *listModel = mdlDialog_listBoxGetListModelP(listBox->rawItemP);
// 		if (NULL == listModel)
// 			return;
// 		ListRow *   rowP = mdlListModel_getRowAtIndex(listModel, row);
// 		if (NULL == rowP)
// 			return;
// 		ListCell *  cellP = mdlListRow_getCellAtIndex(rowP, col);
// 		if (SUCCESS == mdlListCell_getDisplayText(cellP, &stringW))
// 			dimP->u.value.msValueDescrP->SetWChar(stringW);
// 		dimP->u.value.hookHandled = true;
// 	}
// 
// 	void UpdateCellText(DialogItemMessage *dimP, const DataType& data);
// 
// 	void SetListRowData(const DataType& vecData, int iRowIndex);
// 
// 	void SetListRowData(const std::vector<DataType>& vecData);
// 
// 	void SetListRow(int iRows) { m_iRows = iRows; }
// 	DataType GetListRowData(int iRowIndex)
// 	{
// 		if (iRowIndex < m_vecRebarData.size())
// 			return m_vecRebarData[iRowIndex];
// 
// 		return DataType();
// 	}
// 	std::vector<DataType> GetListRowData() { return m_vecData; }
// private:
// 	int m_iRows;
// 	std::tr1::shared_ptr<CListModel> m_pListBox;
// 	std::vector<DataType> m_vecData;
// };

class ListItem
{
public:
	explicit ListItem(int listCol, int listRow, int dlgId, int rscId) :m_pListBox(new CListModel(listCol)), m_iRows(listRow),m_rscDlgId(dlgId),m_rscListId(rscId){};
	virtual ~ListItem() {};
protected:
	virtual bool createListModel() { return true; }
	void SetDefaultListRowData() {}
public:
	void UpdateListBox();

	void SetCellDefaultText(DialogItemMessage *dimP);

	void SetListRow(int iRows) { m_iRows = iRows; }

	int	GetListRow() { return m_iRows; }

	int	GetDialogID() { return m_rscDlgId; }

	int	GetListItemID() { return m_rscListId; }

	std::tr1::shared_ptr<CListModel> GetListModelPtr() { return m_pListBox; }

private:
	int m_rscDlgId;
	int m_rscListId;
	int m_iRows;
	std::tr1::shared_ptr<CListModel> m_pListBox;
};


namespace WallRebarItem
{
	class WallMainRebarList : public ListItem
	{
	public:
		explicit WallMainRebarList(int listCol, int listRow,int dlgId,int rscId) :ListItem(listCol,listRow,dlgId,rscId){};
		~WallMainRebarList() {};
	protected:
		bool createListModel();
		void SetDefaultListRowData();
	public:
		void UpdateCellText(DialogItemMessage *dimP, const PIT::ConcreteRebar& rebarData);

		void SetListRowData(const PIT::ConcreteRebar& vecRebarData,int iRowIndex);

		void SetListRowData(const std::vector<PIT::ConcreteRebar>& vecRebarData);

		PIT::ConcreteRebar GetListRowData(int iRowIndex)
		{ 
			if(iRowIndex < m_vecRebarData.size())
				return m_vecRebarData[iRowIndex]; 
			
			return PIT::ConcreteRebar();
		}
		std::vector<PIT::ConcreteRebar> GetListRowData() { return m_vecRebarData; }

	public:
		static WallMainRebarList* getInstance();
		static void delInstance()
		{
			if (pRebarList)
			{
				delete pRebarList;
				pRebarList = NULL;
			}
		}
	private:
		std::vector<PIT::ConcreteRebar> m_vecRebarData;

		//墙配筋钢筋搭接选项列表指针
		static WallMainRebarList *pRebarList;
	};

	class RebarLapOptionsList : public ListItem
	{
	public:
		explicit RebarLapOptionsList(int listCol, int listRow, int dlgId, int rscId) :ListItem(listCol, listRow, dlgId, rscId) 
		{
			m_vecLapOptionsData.reserve(listRow);
		};
		~RebarLapOptionsList() {};
	protected:
		bool createListModel();
		void SetDefaultListRowData();
	public:
		void UpdateCellText(DialogItemMessage *dimP, const PIT::LapOptions& rebarData);

		void SetListRowData(const PIT::LapOptions& vecRebarData, int iRowIndex);

		void SetListRowData(const std::vector<PIT::LapOptions>& vecRebarData);

		PIT::LapOptions GetListRowData(int iRowIndex)
		{
			if (iRowIndex < m_vecLapOptionsData.size())
				return m_vecLapOptionsData[iRowIndex];

			return PIT::LapOptions();
		}
		std::vector<PIT::LapOptions> GetListRowData() { return m_vecLapOptionsData; }
	public:
		static RebarLapOptionsList* getInstance();

		static void delInstance()
		{
			if (pLapOptionsList)
			{
				delete pLapOptionsList;
				pLapOptionsList = NULL;
			}

		}
	private:
		std::vector<PIT::LapOptions> m_vecLapOptionsData;

		//墙配筋钢筋搭接选项列表指针
		static RebarLapOptionsList *pLapOptionsList;
	};

	class RebarEndTypeList : public ListItem
	{
	public:
		explicit RebarEndTypeList(int listCol, int listRow, int dlgId, int rscId) :ListItem(listCol, listRow, dlgId, rscId) 
		{
			m_vecEndTypeData.reserve(listRow);
		};
		~RebarEndTypeList() {};
	protected:
		bool createListModel();
		void SetDefaultListRowData();
	public:
		void UpdateCellText(DialogItemMessage *dimP, const PIT::EndType& rebarData);

		void SetListRowData(const PIT::EndType& vecRebarData, int iRowIndex);

		void SetListRowData(const std::vector<PIT::EndType>& vecRebarData);

		PIT::EndType GetListRowData(int iRowIndex)
		{
			if (iRowIndex < m_vecEndTypeData.size())
				return m_vecEndTypeData[iRowIndex];

			return PIT::EndType();
		}
		std::vector<PIT::EndType> GetListRowData()
		{ 
			if (m_vecEndTypeData.empty())
			{
				return std::vector<PIT::EndType>();
			}
			return m_vecEndTypeData; 
		}

	public:
		static RebarEndTypeList* getInstance();

		static void delInstance()
		{
			if (pEndTypeList)
			{
				delete pEndTypeList;
				pEndTypeList = NULL;
			}
		}
	private:
		std::vector<PIT::EndType> m_vecEndTypeData;

		//墙配筋钢筋端部样式列表指针
		static RebarEndTypeList *pEndTypeList;
	};
};
