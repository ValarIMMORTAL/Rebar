#pragma once
#include "rebarelements.h"
#include "GalleryIntelligentRebarids.h"
#include <vector>
#include <map>
#include "PITMSCECommon.h"
class  ReportsExamplePlaceReportTool : public DgnPrimitiveTool
{
private:

//	std::vector <RebarListInfo>	 m_vctRebarListInfo;//û�õ�
	std::map<int, double> ma;//ֱ��---�ܳ���
	std::map<double, double> mb;//��λ����----������
	Rebarinfo WeightListInfo;
	EditElementHandle  m_eeh;

	WString s_TitleFirst = L"���Ǹۺ˵糧3��4�Ż���";
	WString s_TitleSecond = L"BRX-��Ӧ�ѳ����ڲ��ṹ��ߴ�+6.500m �� +17.500mǽBRE2655VB,2656VB,3155VB,3156VB,3173VB�ֽ��BS4RA112112DWJG42MD_BB";
	WString s_SecondLine = L"�ֽ�������\n����������1";
	WString s_ThirdLineFirst = L"��HPB4004E��������Ӧ��400MPA��";
	WString s_ThirdLineSecond = L"��HPB300��������Ӧ��300MPA��";
	WString s_NULL = L"";//����������ʽ

	WString s_Diameter = L"ֱ��";
	WString s_AllLen = L"�ܳ���\n(m)";
	WString s_PerWei = L"��λ����\n(kg/m)";
	WString s_AllWei = L"������\n(kg)";

	WString s_AvgDiameter = L"ƽ��ֱ��(mm)";
	WString s_BndLen = L"�����ֽ��(m)";
	WString s_StrLen = L"ֱ�߸ֽ��(m)";
	WString s_BndWei = L"�����ֽ�����(kg)";
	WString s_StrWei = L"ֱ�߸ֽ�����(kg)";

	WString s_One = L"һ�鹹��������(kg)";
	WString s_All = L"���й���������(kg)";

// 	WString ReportsExample::s_vendorItemLibrary = L"VendorCatalogItem";
// 	WString ReportsExample::s_itemTypeVendor = L"Vendor";
// 	WString ReportsExample::s_propertyVendorName = L"VendorName";                    //String
// 	WString ReportsExample::s_propertyAddress = L"MainAddress";                   //String
// 	WString ReportsExample::s_itemTypeItem = L"VendorItem";
// 	WString ReportsExample::s_propertyModel = L"ProductModel";                  //String
// 	WString ReportsExample::s_propertyOutOfStock = L"OutofStock";                    //Boolean
// 	WString ReportsExample::s_propertyPrice = L"Price";                         //Double
// 	//Second Item Type Library and Item Type.
// 	WString ReportsExample::s_itemDetailsLibrary = L"VendorItemDetails";
// 	WString ReportsExample::s_itemTypeInfo = L"Information";
// 	WString ReportsExample::s_propertySize = L"Size";                          //Double
// 	WString ReportsExample::s_propertyDesc = L"Description";                   //String
// 	//Report category and definition.
// 	WString ReportsExample::s_reportCategoryName = L"VendorCatalogReports";
// 	WString ReportsExample::s_reportName = L"VendorCatalogReport";
// 	WString ReportsExample::s_columnRebarOder = L"�ֽ���";
// 	WString ReportsExample::s_columnRebarDiam = L"ֱ��";
// 	WString ReportsExample::s_columnGroupNum = L"����";
// 	WString ReportsExample::s_columnRebarNum = L"ÿ�����";
// 	WString ReportsExample::s_columnRebarAllNum = L"�ܸ���";
// 	WString ReportsExample::s_columnSinMaxLen = L"�����ֽ���󳤶�";
// 	WString ReportsExample::s_columnRebarShapeNum = L"��״����";
// 	WString ReportsExample::s_columnHookType = L"�乳����"; //1������  2������
// 	WString ReportsExample::s_columnRebarSize = L"�ߴ�";
// 	WString ReportsExample::s_columnRebarShape = L"��״";
// 	WString ReportsExample::s_columnVersion = L"�汾";
// 	WString ReportsExample::s_columnNull = L"";
// 	WString ReportsExample::s_columnMM = L"mm";
// 	WString ReportsExample::s_columnMMa = L"mm - a";
// 	WString ReportsExample::s_reportPath = L"VendorCatalogReports\\VendorCatalogReport";

protected:

	ReportsExamplePlaceReportTool(int toolName, int toolPrompt) : DgnPrimitiveTool(toolName, toolPrompt) {}

	//virtual void _OnPostInstall() override;


	virtual void _OnRestartTool() override { InstallNewInstance(GetToolId(), GetToolPrompt()); }


	virtual bool _OnDataButton(DgnButtonEventCR ev) override;

	virtual bool _OnResetButton(DgnButtonEventCR ev) override { _ExitTool(); return true; }

	virtual void _OnDynamicFrame(DgnButtonEventCR ev) override;

public:

	ReportsExamplePlaceReportTool(ElementId id = 0, DgnModelRefP modelRef = NULL) : DgnPrimitiveTool(0, 0) {}
	virtual ~ReportsExamplePlaceReportTool() {}

	void PopulateTextTableCell(TextTableR table, UInt32 rowIndex, UInt32 columnIndex, WCharCP heading, bool mergeCells = false, UInt32 numColumns = 0);

	void PopulateTextTableColumns(TextTableR table);


	static void InstallNewInstance(int toolId, int toolPrompt);

	void CreateTextTableReport(EditElementHandleR tableEeh, DPoint3d origin, DgnModelRefP modelRef);

	void CalaRebarStartEnd(RebarElementP rep, DPoint3d& PtStar, DPoint3d& PtEnd, double& diameter, double& StrmaxLenth, double& sLenth, DgnModelRefP modelRef);

	void GetRebarWeightInfo(RebarAssemblies& reas, DgnModelRefP modelRef);

	void Create_Model(WString wstrName, DgnModelRefP & indexchartmodel, bool isActive);
}; 

class   MyElementSet : public IElementSet
{
private:
	EditElementHandleCP m_first, m_curr, m_last;
	bool GetCurrent(ElementHandle& val)
	{
		if ((NULL == m_curr) || (m_curr > m_last))
			return false;
		val = *m_curr;
		return  true;
	}
public:
	MyElementSet(EditElementHandleCP first, EditElementHandleCP last)
	{
		m_curr = m_first = first;
		m_last = last;
	}
	virtual bool GetFirst(ElementHandle& val) override
	{
		m_curr = m_first;
		return GetCurrent(val);
	}
	virtual bool GetNext(ElementHandle& val) override
	{
		m_curr++;
		return GetCurrent(val);
	}
	virtual size_t GetCount() override
	{
		return (NULL == m_first) ? 0 : (m_last - m_first) + 1;
	}
};

