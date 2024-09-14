// CBreakEllipseWallDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CBreakEllipseWallDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "WallRebarAssembly.h"
#include "ExtractFacesTool.h"


// CBreakEllipseWallDlg 对话框

IMPLEMENT_DYNAMIC(CBreakEllipseWallDlg, CDialogEx)

CBreakEllipseWallDlg::CBreakEllipseWallDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_BreakEllipseWall, pParent)
{

}

CBreakEllipseWallDlg::~CBreakEllipseWallDlg()
{
}

BOOL CBreakEllipseWallDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_editBreakNum.SetWindowText(L"4");
	m_breakNum = 4;

	LONG lStyle;
	lStyle = GetWindowLong(m_listBreakCtrl.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_listBreakCtrl.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_listBreakCtrl.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_listBreakCtrl.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_listBreakCtrl.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_listBreakCtrl.InsertColumn(0, _T("序号"), (int)(width / 3), ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_listBreakCtrl.InsertColumn(1, _T("开始角度"), (int)(width / 3), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listBreakCtrl.InsertColumn(2, _T("结束角度"), (int)(width / 3), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	
	ElementId contid = 0;
	GetElementXAttribute(m_selEh.GetElementId(), sizeof(ElementId), contid, ConcreteIDXAttribute, m_selEh.GetModelRef());
	GetElementXAttribute(contid, m_vecListData, vecBreakDataXAttribute, ACTIVEMODEL);
	if (m_vecListData.size() > 0)
	{
		m_breakNum = m_vecListData.size();
		m_editBreakNum.SetWindowText(to_wstring(m_breakNum).data());
		InitListData();
	}
	else
	{
		UpdateListData();
	}	
	return true;
}

bool CBreakEllipseWallDlg::SetSelectElement(ElementHandleCR eh)
{
	m_selEh = eh;
	WallRebarAssembly::WallType wallType = WallRebarAssembly::JudgeWallType(m_selEh);
	if (wallType == WallRebarAssembly::ELLIPSEWall)
	{
		m_beginAngle = 0;
		m_endAngle = 360;
		return true;
	}
	if (wallType == WallRebarAssembly::ARCWALL)
	{
		EditElementHandle testeeh(eh, false);
		EditElementHandle Eleeh;
		std::vector<EditElementHandle*> Holeehs;
		EFT::GetSolidElementAndSolidHoles(testeeh, Eleeh, Holeehs);
		vector<MSElementDescrP> vecDownFaceLine;
		vector<MSElementDescrP> vecDownFontLine;
		vector<MSElementDescrP> vecDownBackLine;
		double height = 0;
		ExtractFacesTool::GetFrontBackLineAndDownFace(Eleeh, NULL, vecDownFaceLine, vecDownFontLine,
			vecDownBackLine, &height);

		double frontRadius, backRadius;
		frontRadius = backRadius = 0;
		int j = 0;
		for (int i = 0; i < vecDownFaceLine.size(); i++)
		{
			if (vecDownFaceLine[i] != nullptr)
			{
				if (vecDownFaceLine[i]->el.ehdr.type == ARC_ELM)
				{
					double starR, sweepR;
					double radius;
					DPoint3d ArcDPs[2];
					RotMatrix rotM;
					DPoint3d centerpt;
					mdlArc_extract(ArcDPs, &starR, &sweepR, &radius, NULL, &rotM, &centerpt, &vecDownFaceLine[i]->el);
					if (j == 0)
					{
						frontRadius = backRadius = radius;
						j++;
					}
					else
					{
						if (frontRadius > radius)
						{
							frontRadius = radius;
						}
						if (backRadius < radius)
						{
							backRadius = radius;
						}
					}
				}
			}
		}
		double starangleF, endangleF;
		double starangleB, endangleB;
		DPoint3d centerpt;
		CalcuteStrEndAngleAndRadius(starangleF, endangleF, frontRadius, centerpt, vecDownFaceLine);
		CalcuteStrEndAngleAndRadius(starangleB, endangleB, backRadius, centerpt, vecDownFaceLine);
		m_beginAngle = COMPARE_VALUES_EPS(starangleF, starangleB, 1e-6) == -1 ? starangleF : starangleB;
		m_endAngle = COMPARE_VALUES_EPS(endangleF, endangleB, 1e-6) == 1 ? endangleF : endangleB;
		return true;
	}
	return false;
}

void CBreakEllipseWallDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_BreakInfo, m_listBreakCtrl);
	DDX_Control(pDX, IDC_EDIT_BreakNum, m_editBreakNum);
}


void CBreakEllipseWallDlg::UpdateListData()
{
	double everageAngle = fabs(m_endAngle - m_beginAngle) / m_breakNum;
	m_listBreakCtrl.DeleteAllItems();
	for (int i = 0; i < m_breakNum; ++i)
	{		
		m_listBreakCtrl.InsertItem(i, _T("")); // 插入行
		m_listBreakCtrl.SetItemText(i, 0, std::to_wstring(i + 1).data());
		int colNum = m_listBreakCtrl.GetColumnCount();
		for (int j = 1; j < colNum; ++j)
		{
			CString strValue;
			switch (j)
			{
			case 1: 
			{
				double beginAngle = m_beginAngle + everageAngle * i;
				strValue.Format(_T("%.2f"), beginAngle);
				break;
			}		
			case 2:
			{
				double endAngle = m_beginAngle + everageAngle * (i + 1);
				strValue.Format(_T("%.2f"), endAngle);
				break;
			}
			default:
				break;
			}
			m_listBreakCtrl.SetItemText(i, j, strValue);
		}
	}
}

void CBreakEllipseWallDlg::InitListData()
{
	int i = 0;
	for (auto it : m_vecListData)
	{
		m_listBreakCtrl.InsertItem(i, _T("")); // 插入行
		m_listBreakCtrl.SetItemText(i, 0, std::to_wstring(i + 1).data());
		int colNum = m_listBreakCtrl.GetColumnCount();
		for (int j = 1; j < colNum; ++j)
		{
			CString strValue;
			switch (j)
			{
			case 1:
			{
				strValue.Format(_T("%.2f"), it.beginAngle);
				break;
			}
			case 2:
			{
				strValue.Format(_T("%.2f"), it.endAngle);
				break;
			}
			default:
				break;
			}
			m_listBreakCtrl.SetItemText(i, j, strValue);
		}
		++i;
	}
}

BEGIN_MESSAGE_MAP(CBreakEllipseWallDlg, CDialogEx)
	ON_EN_KILLFOCUS(IDC_EDIT_BreakNum, &CBreakEllipseWallDlg::OnEnKillfocusEditBreaknum)
	//ON_LBN_SETFOCUS(IDC_LIST_BreakInfo, &CBreakEllipseWallDlg::OnEnKillfocusEditBreaknum)
	ON_BN_CLICKED(IDOK, &CBreakEllipseWallDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CBreakEllipseWallDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CBreakEllipseWallDlg 消息处理程序


void CBreakEllipseWallDlg::OnEnKillfocusEditBreaknum()
{
	CString numStr;
	m_editBreakNum.GetWindowTextW(numStr);
	m_breakNum = _ttoi(numStr);
	UpdateListData();
}


void CBreakEllipseWallDlg::OnBnClickedOk()
{
	m_listBreakCtrl.GetAllRebarData(m_vecListData);
	CDialogEx::OnOK();
}


void CBreakEllipseWallDlg::OnBnClickedCancel()
{
	m_vecListData.clear();
	PIT::BreakAngleData data;
	data.beginAngle = m_beginAngle;
	data.endAngle = m_endAngle;
	m_vecListData.push_back(data);
	CDialogEx::OnCancel();
}
