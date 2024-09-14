/*--------------------------------------------------------------------------------------+
|
|     $Source: MstnExamples/Annotations/ReportsExample/ReportsExamplePlaceReportTool.cpp $
|
|  $Copyright: (c) 2015 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/
#include "_ustation.h"
#include "ReportRebarList.h"
#include <RebarDetailElement.h>
#include "CommonFile.h"
#include "BentlyCommonfile.h"
#include <Bentley/BeStringUtilities.h>
/*=================================================================================**//**
* @bsiclass                                                               Bentley Systems
+===============+===============+===============+===============+===============+======*/
// class    ReportsExamplePlaceReportTool : DgnPrimitiveTool
// {
// 
// private:
// 
// 	ReportsExamplePlaceReportTool*					m_pRebarAssembly;
// /*---------------------------------------------------------------------------------**//**
// * @bsimethod                                                              Bentley Systems
// +---------------+---------------+---------------+---------------+---------------+------*/
//     static void PopulateTextTableCell(TextTableR table, UInt32 rowIndex, UInt32 columnIndex, WCharCP heading, bool mergeCells=false, UInt32 numColumns=0);
// 
// /*---------------------------------------------------------------------------------**//**
// * @bsimethod                                                              Bentley Systems
// +---------------+---------------+---------------+---------------+---------------+------*/
//     static void PopulateTextTableColumns(TextTableR table);
// 
// /*---------------------------------------------------------------------------------**//**
// * @bsimethod                                                              Bentley Systems
// +---------------+---------------+---------------+---------------+---------------+------*/
//     static void CreateTextTableReport(EditElementHandleR tableEeh, DPoint3d origin);
// 
// protected:
// 
// /*---------------------------------------------------------------------------------**//**
// * @bsimethod                                                              Bentley Systems
// +---------------+---------------+---------------+---------------+---------------+------*/
// ReportsExamplePlaceReportTool (int toolName, int toolPrompt) : DgnPrimitiveTool (toolName, toolPrompt) {}
// 
// /*---------------------------------------------------------------------------------**//**
// * @bsimethod                                                              Bentley Systems
// +---------------+---------------+---------------+---------------+---------------+------*/
// virtual void _OnPostInstall () override;
// 
// /*---------------------------------------------------------------------------------**//**
// * @bsimethod                                                              Bentley Systems
// +---------------+---------------+---------------+---------------+---------------+------*/
// virtual void _OnRestartTool () override {InstallNewInstance (GetToolId (), GetToolPrompt ());}
// 
// /*---------------------------------------------------------------------------------**//**
// * @bsimethod                                                              Bentley Systems
// +---------------+---------------+---------------+---------------+---------------+------*/
// virtual bool _OnDataButton (DgnButtonEventCR ev) override;
// 
// /*---------------------------------------------------------------------------------**//**
// * @bsimethod                                                              Bentley Systems
// +---------------+---------------+---------------+---------------+---------------+------*/
// virtual bool _OnResetButton (DgnButtonEventCR ev) override {_ExitTool (); return true;}
// 
// /*---------------------------------------------------------------------------------**//**
// * @bsimethod                                                              Bentley Systems
// +---------------+---------------+---------------+---------------+---------------+------*/
// virtual void _OnDynamicFrame (DgnButtonEventCR ev) override;
// 
// public:
// 
// /*---------------------------------------------------------------------------------**//**
// * @bsimethod                                                              Bentley Systems
// +---------------+---------------+---------------+---------------+---------------+------*/
//     static void InstallNewInstance (int toolId, int toolPrompt);
// 
// }; // ReportsExamplePlaceReportTool

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
void ReportsExamplePlaceReportTool::PopulateTextTableCell(TextTableR table, UInt32 rowIndex, UInt32 columnIndex, WCharCP heading, bool mergeCells, UInt32 numColumns)
{
	TableCellIndex index(rowIndex, columnIndex);

	if (mergeCells)
	{
		table.MergeCells(index, 1, numColumns);
	}

	TextTableCellP cell = table.GetCell(index);
	TextBlockPtr textBlock = cell->CreateEmptyTextBlock();
	if (!heading)
	{}
	else
	{
		textBlock->AppendText(heading);
	}

	cell->SetTextBlock(*textBlock);
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
void ReportsExamplePlaceReportTool::PopulateTextTableColumns(TextTableR table)
{
	//Heading 1s
	PopulateTextTableCell(table, 0, 0, s_TitleFirst.c_str(), true, 2);
	PopulateTextTableCell(table, 0, 2, s_TitleSecond.c_str(), true, 7);
	PopulateTextTableCell(table, 1, 0, s_SecondLine.c_str(), true, 9);
	PopulateTextTableCell(table, 2, 0, s_NULL.c_str(), true, 2);
	PopulateTextTableCell(table, 2, 2, s_ThirdLineFirst.c_str(), true, 3);
	PopulateTextTableCell(table, 2, 6, s_ThirdLineSecond.c_str(), true, 3);
	PopulateTextTableCell(table, 3, 0, s_Diameter.c_str(), true, 2);
	PopulateTextTableCell(table, 3, 2, s_AllLen.c_str());
	PopulateTextTableCell(table, 3, 3, s_PerWei.c_str());
	PopulateTextTableCell(table, 3, 4, s_AllWei.c_str());
	//隔一列
	PopulateTextTableCell(table, 3, 6, s_AllLen.c_str());
	PopulateTextTableCell(table, 3, 7, s_PerWei.c_str());
	PopulateTextTableCell(table, 3, 8, s_AllWei.c_str());

// 	PopulateTextTableCell(table, 0, 0, ReportsExample::s_itemTypeVendor.c_str(), true, 2);
// 	PopulateTextTableCell(table, 0, 2, ReportsExample::s_itemTypeItem.c_str(), true, 9);
// 
// 	//Heading 2
// 	PopulateTextTableCell(table, 1, 0, ReportsExample::s_columnRebarOder.c_str());
// 	PopulateTextTableCell(table, 1, 1, ReportsExample::s_columnRebarDiam.c_str());
// 	PopulateTextTableCell(table, 1, 2, ReportsExample::s_columnGroupNum.c_str());
// 	PopulateTextTableCell(table, 1, 3, ReportsExample::s_columnRebarNum.c_str());
// 	PopulateTextTableCell(table, 1, 4, ReportsExample::s_columnRebarAllNum.c_str());
// 	PopulateTextTableCell(table, 1, 5, ReportsExample::s_columnSinMaxLen.c_str());
// 	PopulateTextTableCell(table, 1, 6, ReportsExample::s_columnRebarShapeNum.c_str());
// 	PopulateTextTableCell(table, 1, 7, ReportsExample::s_columnHookType.c_str());
// 	PopulateTextTableCell(table, 1, 8, ReportsExample::s_columnRebarSize.c_str());
// 	PopulateTextTableCell(table, 1, 9, ReportsExample::s_columnRebarShape.c_str());
// 	PopulateTextTableCell(table, 1, 10, ReportsExample::s_columnVersion.c_str());
// 
// 	//Heading 3
// 	PopulateTextTableCell(table, 2, 0, ReportsExample::s_columnNull.c_str());
// 	PopulateTextTableCell(table, 2, 1, ReportsExample::s_columnMM.c_str());
// 	PopulateTextTableCell(table, 2, 2, ReportsExample::s_columnNull.c_str());
// 	PopulateTextTableCell(table, 2, 3, ReportsExample::s_columnNull.c_str());
// 	PopulateTextTableCell(table, 2, 4, ReportsExample::s_columnNull.c_str());
// 	PopulateTextTableCell(table, 2, 5, ReportsExample::s_columnMM.c_str());
// 	PopulateTextTableCell(table, 2, 6, ReportsExample::s_columnNull.c_str());
// 	PopulateTextTableCell(table, 2, 7, ReportsExample::s_columnNull.c_str());
// 	PopulateTextTableCell(table, 2, 8, ReportsExample::s_columnMMa.c_str());
// 	PopulateTextTableCell(table, 2, 9, ReportsExample::s_columnNull.c_str());
// 	PopulateTextTableCell(table, 2, 10, ReportsExample::s_columnNull.c_str());

}


//PopulateTextTableColumns(*tTable)


/**
* @返回类型  void
* @函数说明  创建新的model
* @函数参数  string struname
* @函数参数  DgnModelRefP & indexchartmodel
* @函数参数  bool isActive
**/
void ReportsExamplePlaceReportTool::Create_Model(WString wstrName, DgnModelRefP & indexchartmodel, bool isActive)
{
//	struname = ReplaceString(struname.c_str(), "[/*?:<>|\"\\\\]", "@");
//	WString wstrName(struname.c_str());
	//StringToWstring(wstrName, this->GetModelName());
	//model 已存在
	//Gets active dgn file
	DgnFileP dgnFileP = ISessionMgr::GetActiveDgnFile();
	if (nullptr == dgnFileP)
		return;

	ModelId modelId = dgnFileP->FindModelIdByName(wstrName.c_str());
	if (INVALID_MODELID != modelId)
	{
		//Gets the model
		DgnModelPtr dgnModelPtr = dgnFileP->LoadModelById(modelId);

		//Delets the model
		if (dgnFileP->DeleteModel(*dgnModelPtr) == SUCCESS)
		{
			//Save the changes to file
			dgnFileP->ProcessChanges(DgnSaveReason::UserInitiated, 0);
		}
	}
	DgnModelType  modeltype = DgnModelType::Drawing;
	bool is3d = false;
	if (MDLERR_MODELNAMEEXISTS == mdlDgnFileObj_createModel(&indexchartmodel, mdlModelRef_getDgnFile(ACTIVEMODEL), MASTERFILE, wstrName.c_str(), NULL, &modeltype, &is3d))
	{
		if (SUCCESS != mdlModelRef_createWorkingByName(&indexchartmodel, mdlModelRef_getDgnFile(ACTIVEMODEL), wstrName.c_str(), FALSE, FALSE))
			return;
	}

	DgnModelRefP active = ACTIVEMODEL;
	mdlModelRef_activateAndDisplay(indexchartmodel);
	mdlModelRef_activateAndDisplay(active);
	if (isActive)
	{
		mdlModelRef_activateAndDisplay(indexchartmodel);
		fitView(tcb->lstvw);
	}
	ModelInfoPtr modelInfo = indexchartmodel->GetDgnModelP()->GetModelInfo().MakeCopy();
	if (SUCCESS == modelInfo->SetAnnotationScaleFactor(50))
	{
		if (DGNMODEL_STATUS_Success == indexchartmodel->GetDgnModelP()->SetModelInfo(*modelInfo))
		{

		}    // changes are now applied to the model.
	}
}

/*---------------------------------------------------------------------------------**//**
* Create a TextTable and populate its rows with data coming from the report result.
* Create a TextTable element based on this TextTable.
* This method is used both for placing the TextTable and during dynamics.
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
void ReportsExamplePlaceReportTool::CreateTextTableReport(EditElementHandleR tableEeh, DPoint3d origin, DgnModelRefP modelRef)
{

	//Active DGN file and model.
	DgnFileP  activeDgnFile = ISessionMgr::GetActiveDgnFile();
// 	DgnModelP activeDgnModel = ISessionMgr::GetActiveDgnModelP();
// 	This is used for setting margins.
// 	double uors = activeDgnModel->GetModelInfo().MakeCopy()->GetUorPerStorage();
//Get report definition.
// 	ReportDefinitionNodePtr reportNode = ReportDefinitionNode::FindByPath(ReportsExample::s_reportPath.c_str(), *activeDgnFile);
// 
// 	if (!reportNode.IsValid())
// 		return;
// 
// 	//Get Report results and then rows count.
// 	//This rows count will set for rows count of the TextTable.
// 	ReportResults results(*reportNode, activeDgnModel);
// 	UInt32 rowsCount = 0;
// 	ReportResults::const_iterator end = results.end();
// 	for (ReportResults::const_iterator iter = results.begin(); iter != end; ++iter)
// 	{
// 		++rowsCount;
// 	}
// 
// 	if (0 == rowsCount)
// 	{
// 		mdlOutput_messageCenter(OutputMessagePriority::TempRight, L"Report does not contain any rows.", NULL, OutputMessageAlert::None);
// 		return;
// 	}
// 
// 	//Get text style from settings (default).
// 	DgnTextStylePtr textStyle = DgnTextStyle::GetSettings(*activeDgnFile);
// 
// 	//We want to set height in TextTable::Create() from the text style.
// 	double height;
// 	textStyle->GetProperty(TextStyle_Height, height);
// 
// 	//Create text table.
// 	//The 2 rows are for headings
// 	//Heading 1: Information like what item types these columns are from.
// 	//Heading 2: Columns names.
//	TextTablePtr tTable = TextTable::Create(rowsCount + 4, reportNode->GetColumnCount(), textStyle->GetID(), height, *activeDgnModel);

	DgnTextStylePtr textStyle = DgnTextStyle::GetSettings(*activeDgnFile);
 	double height;
 	textStyle->GetProperty(TextStyle_Height, height);
	UInt32 rowsCount = (UInt32)(ma.size() + 19);
	UInt32 ColumnCount = 9;//按照表格写固定的行列，若直径个数增加可以增加行数

	TextTablePtr tTable = TextTable::Create(rowsCount, ColumnCount, textStyle->GetID(), height, *modelRef->GetDgnModelP());//28行9列，画在原点


	//Set origin to the point where the mouse pointer is. This is true both for placing the texttable and during dynamics.
	tTable->SetOrigin(origin);

	//Set some default left & right margins for the texttable.
	TableCellMarginValues margins;
	DgnModelP activeDgnModel = modelRef->GetDgnModelP();
	double uors = activeDgnModel->GetModelInfo().MakeCopy()->GetUorPerStorage();
	margins.m_left = margins.m_right = 10.0 * uors;
	tTable->SetDefaultMargins(margins);

	//Set default cell alignment to centermiddle.
	tTable->SetDefaultCellAlignment(TableCellAlignment::CenterMiddle);

	//Populate Columns information.
	PopulateTextTableColumns(*tTable);
	UInt32 columnIndex = 0;
	UInt32 rowIndex = 4;
	CString strTemp = L"";
	char StrText[100];
	WChar strText[100];

	if (1)//由钢筋等级决定填充哪列
	{
		for (map<int, double>::iterator it = ma.begin(); it != ma.end(); it++)
		{
			strTemp.Format(_T("%d"), it->first);
			strcpy(StrText, CT2A(strTemp));
			BeStringUtilities::CurrentLocaleCharToWChar(strText, StrText, 100);
			PopulateTextTableCell(*tTable, rowIndex, columnIndex, strText, true, 2);

			strTemp.Format(_T("%.1f"), it->second);
			strcpy(StrText, CT2A(strTemp));
			BeStringUtilities::CurrentLocaleCharToWChar(strText, StrText, 100);
			PopulateTextTableCell(*tTable, rowIndex, columnIndex + 2, strText);
			rowIndex++;
		}
		rowIndex = 4;
		columnIndex = 3;
		for (map<double, double>::iterator itt = mb.begin(); itt != mb.end(); itt++)
		{
			strTemp.Format(_T("%.3f"), itt->first);
			strcpy(StrText, CT2A(strTemp));
			BeStringUtilities::CurrentLocaleCharToWChar(strText, StrText, 100);
			PopulateTextTableCell(*tTable, rowIndex, columnIndex, strText);

			strTemp.Format(_T("%.2f"), itt->second);
			strcpy(StrText, CT2A(strTemp));
			BeStringUtilities::CurrentLocaleCharToWChar(strText, StrText, 100);
			PopulateTextTableCell(*tTable, rowIndex, columnIndex + 1, strText);
			rowIndex++;
		}
	}
	else
	{
		rowIndex = 4;
		columnIndex = 6;//中间隔了一行5
		for (map<int, double>::iterator it = ma.begin(); it != ma.end(); it++)
		{
			strTemp.Format(_T("%.1f"), it->second);
			strcpy(StrText, CT2A(strTemp));
			BeStringUtilities::CurrentLocaleCharToWChar(strText, StrText, 100);
			PopulateTextTableCell(*tTable, rowIndex, columnIndex, strText);
			rowIndex++;
		}
		rowIndex = 4;
		for (map<double, double>::iterator itt = mb.begin(); itt != mb.end(); itt++)
		{
			strTemp.Format(_T("%.3f"), itt->first);
			strcpy(StrText, CT2A(strTemp));
			BeStringUtilities::CurrentLocaleCharToWChar(strText, StrText, 100);
			PopulateTextTableCell(*tTable, rowIndex, columnIndex + 1, strText);

			strTemp.Format(_T("%.2f"), itt->second);
			strcpy(StrText, CT2A(strTemp));
			BeStringUtilities::CurrentLocaleCharToWChar(strText, StrText, 100);
			PopulateTextTableCell(*tTable, rowIndex, columnIndex + 2, strText);
			rowIndex++;
		}
	}


	//中间隔5行
	PopulateTextTableCell(*tTable, rowIndex, 0, s_NULL.c_str(), true, 9);
	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_NULL.c_str(), true, 9);
	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_NULL.c_str(), true, 9);
	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_NULL.c_str(), true, 9);
	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_NULL.c_str(), true, 9);

	columnIndex = 0;
	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_AvgDiameter.c_str(),true,2);
	BeStringUtilities::CurrentLocaleCharToWChar(strText, WeightListInfo.AvgDamieter, 100);
	PopulateTextTableCell(*tTable, rowIndex, 2, strText, true, 7);

	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_BndLen.c_str(), true, 2);
	BeStringUtilities::CurrentLocaleCharToWChar(strText, WeightListInfo.BndRebarLength, 100);
	PopulateTextTableCell(*tTable, rowIndex, 2, strText, true, 7);

	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_StrLen.c_str(), true, 2);
	BeStringUtilities::CurrentLocaleCharToWChar(strText, WeightListInfo.StrRebarLength, 100);
	PopulateTextTableCell(*tTable, rowIndex, 2, strText, true, 7);

	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_AllLen.c_str(), true, 2);
	BeStringUtilities::CurrentLocaleCharToWChar(strText, WeightListInfo.AllLen, 100);
	PopulateTextTableCell(*tTable, rowIndex, 2, strText, true, 7);

	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_BndWei.c_str(), true, 2);
	BeStringUtilities::CurrentLocaleCharToWChar(strText, WeightListInfo.BndRebarWeight, 100);
	PopulateTextTableCell(*tTable, rowIndex, 2, strText, true, 7);

	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_StrWei.c_str(), true, 2);
	BeStringUtilities::CurrentLocaleCharToWChar(strText, WeightListInfo.StrRebarWeight, 100);
	PopulateTextTableCell(*tTable, rowIndex, 2, strText, true, 7);

	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_AllWei.c_str(), true, 2);
	BeStringUtilities::CurrentLocaleCharToWChar(strText, WeightListInfo.AllWei, 100);
	PopulateTextTableCell(*tTable, rowIndex, 2, strText, true, 7);

	//中间隔一行
	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_NULL.c_str(), true, 9);

	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_One.c_str(), true, 3);
	BeStringUtilities::CurrentLocaleCharToWChar(strText, WeightListInfo.AllWei, 100);
	PopulateTextTableCell(*tTable, rowIndex, 3, strText, true, 6);

	PopulateTextTableCell(*tTable, ++rowIndex, 0, s_All.c_str(), true, 3);
	BeStringUtilities::CurrentLocaleCharToWChar(strText, WeightListInfo.AllWei, 100);
	PopulateTextTableCell(*tTable, rowIndex, 3, strText, true, 6);

	TextTableHandler::CreateTextTableElement(tableEeh, *tTable, ACTIVEMODEL);
}

/*---------------------------------------------------------------------------------**//**
* This tool requires only a single point to place the texttable as the texttable is
* already created from report therefore we start dynamics here.
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
// void ReportsExamplePlaceReportTool::_OnPostInstall ()
//     {
//     mdlOutput_rscPrintf (MSG_PROMPT, 0, STRINGLISTID_ReportsExampleTextMessages, PROMPT_PlaceReport);
//     _BeginDynamics ();
//     __super::_OnPostInstall ();
//     }

/*---------------------------------------------------------------------------------**//**
* Place the texttable if the user send a data point.
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
bool ReportsExamplePlaceReportTool::_OnDataButton(DgnButtonEventCR ev)
{
	EditElementHandle tableEeh;
//	CreateTextTableReport(tableEeh, *ev.GetPoint());
	if (tableEeh.IsValid())
		tableEeh.AddToModel();

	return true;
}

/*---------------------------------------------------------------------------------**//**
* In dynamics create and show the texttable but do not add it to model.
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
void ReportsExamplePlaceReportTool::_OnDynamicFrame(DgnButtonEventCR ev)
{
	EditElementHandle tableEeh;
//	CreateTextTableReport(tableEeh, *ev.GetPoint());
	if (!tableEeh.IsValid())
		_ExitTool();

	RedrawElems redrawElems;

	redrawElems.SetDynamicsViews(IViewManager::GetActiveViewSet(), ev.GetViewport());
	redrawElems.SetDrawMode(DRAW_MODE_TempDraw);
	redrawElems.SetDrawPurpose(DrawPurpose::Dynamics);

	redrawElems.DoRedraw(tableEeh);
}

/*---------------------------------------------------------------------------------**//**
* Method to create and install a new instance of the tool. If InstallTool returns ERROR,
* the new tool instance will be freed/invalid. Never call delete on RefCounted classes.
*
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
void ReportsExamplePlaceReportTool::InstallNewInstance(int toolId, int toolPrompt)
{
	ReportsExamplePlaceReportTool* tool = new ReportsExamplePlaceReportTool(toolId, toolPrompt);

	tool->InstallTool();
}

// Public void reportsExample_placeReport (WCharCP unparsed)
//     {
//     if(!ReportsExample::ReportDefinitionExists())
//         {
//         mdlOutput_messageCenter (OutputMessagePriority::TempRight, L"Table cannot be created. Looks like Report does not exist.", NULL, OutputMessageAlert::None);
//         return;
//         }
//     else
//         {
//         ReportsExamplePlaceReportTool::InstallNewInstance (CMDNAME_ReportsExamplePlaceReport, PROMPT_PlaceReport);
//         }
//     }


void ReportsExamplePlaceReportTool::CalaRebarStartEnd(RebarElementP rep, DPoint3d& PtStar, DPoint3d& PtEnd, double& diameter, double& StrmaxLenth, double& sLenth, DgnModelRefP modelRef)
{
	RebarCurve curve;
	double uor_per_mm = modelRef->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	RebarShape * rebarshape = rep->GetRebarShape(modelRef);
	rebarshape->GetRebarCurve(curve);
	BrString Sizekey(rebarshape->GetShapeData().GetSizeKey());
	diameter = RebarCode::GetBarDiameter(Sizekey, modelRef);

	CMatrix3D tmp3d(rep->GetLocation());
	curve.MatchRebarCurve(tmp3d, curve, uor_per_mm);
	curve.DoMatrix(rep->GetLocation());
	RebarVertices  vers = curve.PopVertices();
	sLenth = curve.GetLength();

	for (int i = 0; i < vers.GetSize() - 1; i++)
	{
		RebarVertex   ver1 = vers.At(i);
		RebarVertex   ver2 = vers.At(i + 1);
		CPoint3D pt1 = ver1.GetIP();
		CPoint3D pt2 = ver2.GetIP();
// 			if (COMPARE_VALUES_EPS(ver1.GetRadius(), 0.00, EPS) == 0 && COMPARE_VALUES_EPS(ver2.GetRadius(), 0.00, EPS) == 0)//直线钢筋
// 			{
// 				StrmaxLenth += pt1.Distance(pt2);
// 			}
// 			else if (COMPARE_VALUES_EPS(ver1.GetRadius(), 0.00, EPS) == 0 && COMPARE_VALUES_EPS(ver2.GetRadius(), 0.00, EPS) != 0)//先直后弯
// 			{
// 				StrmaxLenth += pt1.Distance(Arc2);
// 			}
// 			else if (COMPARE_VALUES_EPS(ver1.GetRadius(), 0.00, EPS) != 0 && COMPARE_VALUES_EPS(ver2.GetRadius(), 0.00, EPS) == 0)//先弯后直
// 			{
// 				StrmaxLenth += Arc1.Distance(pt2);
// 			}
// 			else//弧形钢筋
// 			{}
		if (COMPARE_VALUES_EPS(ver1.GetRadius(), 0.00, EPS) == 0)
		{
			pt1 = ver1.GetIP();
		}
		else
		{
			pt1 = ver1.GetArcPt(2);
		}
		if (COMPARE_VALUES_EPS(ver2.GetRadius(), 0.00, EPS) == 0)
		{
			pt2 = ver2.GetIP();
			StrmaxLenth += pt1.Distance(pt2);
		}
		else
		{
			pt2 = ver2.GetArcPt(0);
			StrmaxLenth += pt1.Distance(pt2);;
		}


	}
}


void ReportsExamplePlaceReportTool::GetRebarWeightInfo(RebarAssemblies& reas,DgnModelRefP modelRef)
{

	double RebarLength8 = 0.00;
	double RebarLength10 = 0.00;
	double RebarLength12 = 0.00;
	double RebarLength14 = 0.00;
	double RebarLength16 = 0.00;
	double RebarLength20 = 0.00;
	double RebarLength25 = 0.00;
	double RebarLength32 = 0.00;
	double RebarLength40 = 0.00;

	int Num[9] = { 0 };//每种不同直径的钢筋根数
	double mm[9] = { 0.00 };//每种不同直径的钢筋的最大直线长度
	double perWeight[9] = { 0.395,0.617,0.888,1.208,1.578,2.466,3.853,6.313,9.864 };//标准单位重量

	for (int i = 0; i < reas.GetSize(); i++)
	{
		RebarAssembly* rebaras = reas.GetAt(i);
		RebarSets rebar_sets;
		rebaras->GetRebarSets(rebar_sets, ACTIVEMODEL);

		int RebarNum = 0;
		for (int j = 0; j < rebar_sets.GetSize(); j++) // 遍历一个RebarAssembly的所有钢筋组
		{
			RebarSetP rebarSet = &rebar_sets.At(j);
			DPoint3d ptStar, ptEnd;
			double RebarLength = 0.00;//整根钢筋长度
			double MaxLength = 0.00;//直线最长钢筋长度
			double diameter = 0.00;
			double  RebarSetLength = 0.00;

			int RebarSetNum = (int)rebarSet->GetChildElementCount(); // 每组钢筋根数
			if (RebarSetNum > 1)
			{
				int RebarSetNum = 0;
				RebarElementP pRebar = rebarSet->GetChildElement(1);//取一根钢筋的信息，求出直径、长度
				CalaRebarStartEnd(pRebar, ptStar, ptEnd, diameter, MaxLength,RebarLength, modelRef);

				switch ((int)diameter/10)
				{
				case 8:
					RebarSetNum = (int)rebarSet->GetChildElementCount();
					RebarSetLength = RebarSetNum * RebarLength;//单根钢筋长度 * 该组钢筋数量->该组的钢筋总长
					RebarLength8 += RebarSetLength;//所选择的所有钢筋中直径为8的总长度
					Num[0] += RebarSetNum;//所选择的所有钢筋中直径为8的钢筋根数
					MaxLength *= RebarSetNum;
					mm[0] += MaxLength;
					break;
				case 10:
					RebarSetNum = (int)rebarSet->GetChildElementCount();
					RebarSetLength = RebarSetNum * RebarLength;
					RebarLength10 += RebarSetLength;
					Num[1] += RebarSetNum;
					MaxLength *= RebarSetNum;
					mm[1] += MaxLength;
					break;
				case 12:
					RebarSetNum = (int)rebarSet->GetChildElementCount();
					RebarSetLength = RebarSetNum * RebarLength;
					RebarLength12 += RebarSetLength;
					Num[2] += RebarSetNum;
					MaxLength *= RebarSetNum;
					mm[2] += MaxLength;
					break;
				case 14:
					RebarSetNum = (int)rebarSet->GetChildElementCount();
					RebarSetLength = RebarSetNum * RebarLength;
					RebarLength14 += RebarSetLength;
					Num[3] += RebarSetNum;
					MaxLength *= RebarSetNum;
					mm[3] += MaxLength;
					break;
				case 16:
					RebarSetNum = (int)rebarSet->GetChildElementCount();
					RebarSetLength = RebarSetNum * RebarLength;
					RebarLength16 += RebarSetLength;
					Num[4] += RebarSetNum;
					MaxLength *= RebarSetNum;
					mm[4] += MaxLength;
					break;
				case 20:
					RebarSetNum = (int)rebarSet->GetChildElementCount();
					RebarSetLength = RebarSetNum * RebarLength;
					RebarLength20 += RebarSetLength;
					Num[5] += RebarSetNum;
					MaxLength *= RebarSetNum;
					mm[5] += MaxLength;
					break;
				case 25:
					RebarSetNum = (int)rebarSet->GetChildElementCount();
					RebarSetLength = RebarSetNum * RebarLength;
					RebarLength25 += RebarSetLength;
					Num[6] += RebarSetNum;
					MaxLength *= RebarSetNum;
					mm[6] += MaxLength;
					break;
				case 32:
					RebarSetNum = (int)rebarSet->GetChildElementCount();
					RebarSetLength = RebarSetNum * RebarLength;
					RebarLength32 += RebarSetLength;
					Num[7] += RebarSetNum;
					MaxLength *= RebarSetNum;
					mm[7] += MaxLength;
					break;
				case 40:
					RebarSetNum = (int)rebarSet->GetChildElementCount();
					RebarSetLength = RebarSetNum * RebarLength;
					RebarLength40 += RebarSetLength;
					Num[8] += RebarSetNum;
					MaxLength *= RebarSetNum;
					mm[8] += MaxLength;
					break;
				default:
					break;
				}
			}
		}
	}
//直径-----总长度
	ma[8]= RebarLength8 / 1000;
	ma[10] = RebarLength10 / 1000;
	ma[12] = RebarLength12/1000;
	ma[14] = RebarLength14 / 1000;
	ma[16] = RebarLength16 / 1000;
	ma[20] = RebarLength20 / 1000;
	ma[25] = RebarLength25 / 1000;
	ma[32] = RebarLength32 / 1000;
	ma[40] = RebarLength40 / 1000;
//单位重量-----总重量
	mb[0.395] = (RebarLength8 / 1000) * 0.395;
	mb[0.617] = (RebarLength10 / 1000)* 0.617;
	mb[0.888] = (RebarLength12 / 1000)* 0.888;
	mb[1.208] = (RebarLength14 / 1000)* 1.208;
	mb[1.578] = (RebarLength16 / 1000)* 1.578;
	mb[2.466] = (RebarLength20 / 1000)* 2.466;
	mb[3.853] = (RebarLength25 / 1000)* 3.853;
	mb[6.313] = (RebarLength32 / 1000)* 6.313;
	mb[9.864] = (RebarLength40 / 1000)* 9.864;

	//求总长度
	double AllLength = 0.0;
	for (map<int, double>::iterator it = ma.begin(); it != ma.end(); it++)
		AllLength += it->second;

	//求总重量
	double AllWeight = 0.00;
	for (map<double, double>::iterator itt = mb.begin(); itt != mb.end(); itt++)
		AllWeight += itt->second;

	//求平均直径 （目前只有9种不同直径的钢筋）
	int allnum = 0;
	for (int a = 0; a < sizeof(Num) / sizeof(Num[0]); a++)
		allnum += Num[a];//总根数
	double AvgDamieter = (8*Num[0] + 10* Num[1] + 12 * Num[2] + 14 * Num[3] + 16 * Num[4]+ 20 * Num[5] + 25 * Num[6] + 32 * Num[7] + 40 * Num[8])/ allnum;

	//求直线钢筋总长度
	double StrRebarLen= 0.00;
	for (int k = 0;k < sizeof(mm)/sizeof(mm[0]); k++)
		StrRebarLen += (mm[k]/1000);

	//求直线钢筋重量 = 每种直径的直线钢筋长度 * 对应的单位重量
	double StrRebarWeight = 0.00;
	if (sizeof(Num)/sizeof(Num[0]) == sizeof(mm)/sizeof(mm[0]))
	{
		for (int b = 0; b < sizeof(mm) / sizeof(mm[0]); b++)
			StrRebarWeight += (mm[b] /1000) * perWeight[b];
	}

	//求弯曲钢筋总长度
	double BndRebarLen = 0.00;
	BndRebarLen = AllLength - StrRebarLen;

	//求弯曲钢筋总重量
	double BndRebarWeight = 0.00;
	BndRebarWeight = AllWeight - StrRebarWeight;

	CString strTemp = L"";
	strTemp.Format(_T("%.0f"), AllLength );
	strcpy(WeightListInfo.AllLen, CT2A(strTemp));

	strTemp.Format(_T("%.0f"), AllWeight);
	strcpy(WeightListInfo.AllWei, CT2A(strTemp));

	strTemp.Format(_T("%.1f"), AvgDamieter);
	strcpy(WeightListInfo.AvgDamieter, CT2A(strTemp));

	strTemp.Format(_T("%.f"), BndRebarLen);
	strcpy(WeightListInfo.BndRebarLength, CT2A(strTemp));

	strTemp.Format(_T("%.f"), StrRebarLen);
	strcpy(WeightListInfo.StrRebarLength, CT2A(strTemp));

	strTemp.Format(_T("%.f"), StrRebarWeight);
	strcpy(WeightListInfo.StrRebarWeight, CT2A(strTemp));

	strTemp.Format(_T("%.0f"), BndRebarWeight);
	strcpy(WeightListInfo.BndRebarWeight, CT2A(strTemp));

}