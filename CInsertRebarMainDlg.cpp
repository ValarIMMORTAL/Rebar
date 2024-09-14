// CInsertRebarMain.cpp: 实现文件

#include "_USTATION.h"
#include "CInsertRebarMainDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include <RebarDetailElement.h>
#include "RebarElements.h"
#include "ConstantsDef.h"
#include "CommonFile.h"

// CInsertRebarMain 对话框

CInsertRebarMainDlg* pInsertRebarMainDlg = NULL;

IMPLEMENT_DYNAMIC(CInsertRebarMainDlg, CDialogEx)

CInsertRebarMainDlg::CInsertRebarMainDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_InsertRebarMain, pParent)
{
	m_CurSelTab = 0;
	m_ConcreteId = 0;
	m_FirstItem = 0;
	m_arrItem[0] = _T("柱插筋");
	// m_arrItem[1] = _T("墙插筋");
}

CInsertRebarMainDlg::~CInsertRebarMainDlg()
{
}

void CInsertRebarMainDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

// CInsertRebarMain 消息处理程序
BOOL CInsertRebarMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//创建两个对话框
	m_PageColInserRebar.SetSelectElement(m_ehSel);
	m_PageWallInsertRebar.SetSelectElement(m_ehSel);
	m_PageColInserRebar.SetBashElement(m_basis);
	m_PageWallInsertRebar.SetBashElement(m_basis);
	m_PageColInserRebar.SetConcreteId(m_ConcreteId);
	m_PageWallInsertRebar.SetConcreteId(m_ConcreteId);
	m_PageColInserRebar.Create(IDD_DIALOG_InsertRebarCol, &m_tab);
	m_PageWallInsertRebar.Create(IDD_DIALOG_InsertRebarWall, &m_tab);

	//设定在Tab内显示的范围
	CRect rc;
	m_tab.GetClientRect(rc);
	rc.top += 20;
	rc.bottom -= 0;
	rc.left += 0;
	rc.right -= 0;
	m_PageColInserRebar.MoveWindow(&rc);
	m_PageWallInsertRebar.MoveWindow(&rc);
	bIsFirst = false;

	if (m_FirstItem == 0)
	{
		//为Tab Control增加两个页面
		// m_tab.InsertItem(0, m_arrItem[0]);
		m_tab.InsertItem(0, m_arrItem[1]);

		//把对话框对象指针保存起来
		// p_Dialog[0] = &m_PageColInserRebar;
		p_Dialog[0] = &m_PageWallInsertRebar;
	}
	else if (m_FirstItem == 1)
	{
		//为Tab Control增加两个页面
		//m_tab.InsertItem(0, m_arrItem[1]);
		//// m_tab.InsertItem(1, m_arrItem[0]);

		////把对话框对象指针保存起来
		//p_Dialog[0] = &m_PageWallInsertRebar;
		//p_Dialog[1] = &m_PageColInserRebar;
	}

	// 显示初始页面
	p_Dialog[0]->ShowWindow(SW_SHOW);
	// p_Dialog[1]->ShowWindow(SW_HIDE);
	m_CurSelTab = 0;

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CInsertRebarMainDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_TAB_InserRebar, m_tab);
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInsertRebarMainDlg, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_InserRebar, &CInsertRebarMainDlg::OnTcnSelchangeTab2)
	ON_BN_CLICKED(IDOK, &CInsertRebarMainDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CInsertRebarMainDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CInsertRebarMain 消息处理程序


void CInsertRebarMainDlg::OnTcnSelchangeTab2(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 在此添加控件通知处理程序代码

	//把当前的页面隐藏起来
	p_Dialog[m_CurSelTab]->ShowWindow(SW_HIDE);
	//得到新的页面索引
	m_CurSelTab = m_tab.GetCurSel();
	//把新的页面显示出来
	p_Dialog[m_CurSelTab]->ShowWindow(SW_SHOW);

	//if (m_PageWallInsertRebar.GetRebarPtsSize() == 0 && m_CurSelTab == 1 && m_FirstItem == 0)
	//{
	//	MessageBox(TEXT("请先配置主筋数据"), TEXT("墙插筋"), MB_OK);
	//}

	*pResult = 0;
}


void CInsertRebarMainDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	m_PageWallInsertRebar.CoverOnBnClickedOk();
	DestroyWindow();
	//if ((m_FirstItem == 0 && m_CurSelTab == 0) || (m_FirstItem == 1 && m_CurSelTab == 1))
	//{
	//	m_PageColInserRebar.CoverOnBnClickedOk();
	//}
	//else if ((m_FirstItem == 0 && m_CurSelTab == 1) || (m_FirstItem == 1 && m_CurSelTab == 0))
	//{
	//	m_PageWallInsertRebar.CoverOnBnClickedOk();
	//}
}


void CInsertRebarMainDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	DestroyWindow();
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
void DgnInsertRebarTool::SetupForLocate()
{
	UInt32      msgId;
	msgId = PROMPT_ACCEPTREJECT;
	bool doLocate = true;

	switch (GetElementAgenda().GetCount())
	{
	case 0:
		msgId = PROMPT_SELECT_BASIS;
		break;

	case 1:
		msgId = PROMPT_SELECT_EXPAND;
		break;

	default:
		msgId = PROMPT_ACCEPTREJECT;
		doLocate = false;
		break;
	}

	__super::_SetLocateCursor(doLocate);

	mdlAccuSnap_enableSnap(false);
	mdlAccuDraw_setEnabledState(false);

	mdlOutput_rscPrintf(MSG_PROMPT, NULL, STRINGLISTID_RebarSDKExampleTextMessages, msgId); // STRINGLISTID_ShaftToolTextMessages
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
bool DgnInsertRebarTool::_WantAdditionalLocate(DgnButtonEventCP ev)
{
	if (NULL == ev)
		return true; // This is a multi-locate tool...

	return GetElementAgenda().GetCount() != 2;
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
void DgnInsertRebarTool::_SetupAndPromptForNextAction()
{
	SetupForLocate();
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+------*/
bool DgnInsertRebarTool::_SetupForModify(DgnButtonEventCR ev, bool isDynamics)
{
	if (GetElementAgenda().GetCount() != 2)
	{
		return false;
	}

	return true;
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
EditElementHandleP DgnInsertRebarTool::_BuildLocateAgenda(HitPathCP path, DgnButtonEventCP ev)
{
	EditElementHandle eeh(path->GetHeadElem(), path->GetRoot());
	return GetElementAgenda().Insert(eeh);
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
bool DgnInsertRebarTool::_OnDataButton(DgnButtonEventCR ev)
{
	HitPathCP path = _DoLocate(ev, true, ComponentMode::Innermost);

	//If user has selected at least one element and _DoLocate() returns NULL, this means the user has finished selecting elements.
	if (NULL == path && 0 != GetElementAgenda().size())
	{
		_SetupAndPromptForNextAction();
	}

	return __super::_OnDataButton(ev);;
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
bool DgnInsertRebarTool::_OnPostLocate(HitPathCP path, WStringR cantAcceptReason)
{
	if (!DgnElementSetTool::_OnPostLocate(path, cantAcceptReason))
		return false;

	ElementHandle eh(path->GetHeadElem(), path->GetRoot());

	return !(RebarElement::IsRebarElement(eh) || IsRebarDetailSet(eh));
	// return ShaftBodyAssembly::IsShaftBody(eh) || ShaftCapAssembly::IsShaftCap(eh);
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
void DgnInsertRebarTool::CreateRebarElement(EditElementHandleR eeh, bool isDynamics)
{
	if (!isDynamics)
	{
		g_InsertElm = eeh;
		return;
	}
	g_ColumnElm = eeh;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (pInsertRebarMainDlg != NULL)
	{
		delete pInsertRebarMainDlg;
		pInsertRebarMainDlg = NULL;
	}
	pInsertRebarMainDlg = new CInsertRebarMainDlg();
	pInsertRebarMainDlg->SetSelectElement(g_ColumnElm);
	pInsertRebarMainDlg->SetBashElement(g_InsertElm);
	pInsertRebarMainDlg->Create(IDD_DIALOG_InsertRebarMain);
	//		dlg.m_PageAssociatedComponent.SetListRowData(vecACData);
	pInsertRebarMainDlg->ShowWindow(SW_SHOW);

	// WString outMessage;
	// WString::Sprintf(outMessage, L"Element ID: %I64d", eeh.GetElementId());
	// mdlOutput_messageCenter(OutputMessagePriority::TempRight, L"Wrong element: ", outMessage.c_str(), OutputMessageAlert::None);
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
StatusInt DgnInsertRebarTool::_ProcessAgenda(DgnButtonEventCR  ev)
{
	EditElementHandleP start = GetElementAgenda().GetFirstP();
	EditElementHandleP last = start + GetElementAgenda().GetCount();

	//Apply the required modification to each element in ElementAgenda.
	int nIndex = 0;
	for (EditElementHandleP elementToModify = start; elementToModify < last; elementToModify++)
	{
		ElementRefP oldRef = elementToModify->GetElementRef();
		bool isSecondElement = false;
		if (nIndex == 1)
		{
			isSecondElement = true;
		}
		nIndex++;
		CreateRebarElement(*elementToModify, isSecondElement);
	}

	return SUCCESS;
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
bool DgnInsertRebarTool::_OnInstall()
{
	_SetElemSource(SOURCE_Pick);
	return true;
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
void DgnInsertRebarTool::_OnRestartTool()
{
	InstallNewInstance(GetToolId());
}

/*---------------------------------------------------------------------------------**//**
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
void DgnInsertRebarTool::InstallNewInstance(int toolId)
{
	DgnInsertRebarTool* tool = new DgnInsertRebarTool(toolId);
	mdlOutput_messageCenter(OutputMessagePriority::TempRight, L"Please pick two elements, the first is the Basis, the sceond is Wall Or Column", NULL, OutputMessageAlert::None);
	tool->InstallTool();
}

/*---------------------------------------------------------------------------------**//**
* @param[in]        unparsed        In this case unparsed is ignored.
* @bsimethod                                                              Bentley Systems
+---------------+---------------+---------------+---------------+---------------+--*/
Public void ShaftTool_placeRebar(WCharCP unparsed)
{
	DgnInsertRebarTool::InstallNewInstance(CMDNAME_InsertRebarTool);
}
