// CDomeRebarDlg.cpp: 实现文件
//
#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CDomeRebarDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ElementAttribute.h"
#include "CDomeRebarAssembly.h"
#include "ConstantsDef.h"

// CDomeRebarDlg 对话框

IMPLEMENT_DYNAMIC(CDomeRebarDlg, CDialogEx)

CDomeRebarDlg::CDomeRebarDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_DomeRebarDlg, pParent)
{
	m_Cover = 30.0;
	m_Number = 2;

	m_pDomeDetailDlg = NULL;
}

CDomeRebarDlg::~CDomeRebarDlg()
{
	if (m_pDomeDetailDlg != NULL)
	{
		delete m_pDomeDetailDlg;
		m_pDomeDetailDlg = NULL;
	}
}

void CDomeRebarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT1, m_EditCover);
	DDX_Control(pDX, IDC_EDIT2, m_EditNumber);
	DDX_Control(pDX, IDC_LIST2, m_listCtl);
}


BOOL CDomeRebarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	ElementId testid = 0;
	GetElementXAttribute(m_ehSel.GetElementId(), sizeof(ElementId), testid, ConcreteIDXAttribute, m_ehSel.GetModelRef());
	if (testid > 0)
	{
		//GetElementXAttribute(testid, sizeof(PIT::DomeInfo), m_stDomeInfo, DomeInfoAttribute, ACTIVEMODEL);
		//GetElementXAttribute(testid, m_vecDomeLevel, vecDomeLevelAttribute, ACTIVEMODEL);

		//GetElementXAttribute(testid, sizeof(PIT::DomeInfo::RoundDomeInfo), m_stRoundDomeInfo, stDomeRoundInfoAttribute, ACTIVEMODEL);
	}

	LONG lStyle;
	lStyle = GetWindowLong(m_listCtl.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_listCtl.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_listCtl.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_listCtl.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_listCtl.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_listCtl.InsertColumn(0, _T("序号"), (int)(width / 5) * 0.5, ListCtrlEx::Normal, LVCFMT_LEFT, ListCtrlEx::SortByDigit);
	m_listCtl.InsertColumn(1, _T("R1"), (int)(width / 5), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listCtl.InsertColumn(2, _T("R2"), (int)(width / 5), ListCtrlEx::EditBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listCtl.InsertColumn(3, _T("钢筋布置方式"), (int)(width / 5) * 1.5, ListCtrlEx::ComboBox, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_listCtl.InsertColumn(4, _T("钢筋信息"), (int)(width / 5), ListCtrlEx::Button, LVCFMT_CENTER, ListCtrlEx::SortByString);

	// m_listCtl.SetShowProgressPercent(TRUE);
	// m_listCtl.SetSupportSort(TRUE);

	UpdateRebarList();

	UInt32 xAttId = 1111;
	GetElementXAttribute(m_ehSel.GetElementId(), m_vecVertex, xAttId, m_ehSel.GetModelRef());
	UInt32 xAttId2 = 1112;
	GetElementXAttribute(m_ehSel.GetElementId(), (int)sizeof(Transform), m_targetTrans, xAttId2, m_ehSel.GetModelRef());

	int nMax = 0;
	double dMaxZ = 0.00;
	double dMinZ = 0.00;
	for (int i = 0; i < (int)m_vecVertex.size(); i++)
	{
		mdlTMatrix_transformPoint(&m_vecVertex.at(i).ptVertex, &m_targetTrans); // 位置变换

		if (COMPARE_VALUES_EPS(m_vecVertex.at(i).ptVertex.z, dMaxZ, EPS) > 0 || i == 0)
		{
			nMax = i;
			dMaxZ = m_vecVertex.at(i).ptVertex.z;
		}

		if (COMPARE_VALUES_EPS(m_vecVertex.at(i).ptVertex.z, dMinZ, EPS) < 0 || i == 0)
		{
			dMinZ = m_vecVertex.at(i).ptVertex.z;
		}
	}

	mdlInput_sendSynchronizedKeyin(L"rotate activeview top 1", 0, INPUTQ_HEAD, NULL);

	CString strTmp;
	strTmp.Format(_T("%.2f"), m_Cover);
	m_EditCover.SetWindowText(strTmp);

	strTmp.Format(_T("%d"), m_Number);
	m_EditNumber.SetWindowText(strTmp);
	if (m_vecVertex.size() == 0)
	{
		mdlDialog_openMessageBox(DIALOGID_MsgBoxOK, L"该模型没有包含穹顶配筋所需数据，请使用模型组装导出的模型", MessageBoxIconType::Warning);
		return true;
	}

	m_circleCenter = m_vecVertex.at(nMax).ptVertex;
	m_dScaleLength = dMaxZ - dMinZ;

	return true;
}

void CDomeRebarDlg::UpdateRebarList()
{
	m_listCtl.DeleteAllItems();
	if (m_vecDomeLevelInfo.empty())
	{
		m_Number = 2;
		m_mapDomeLevelDetailInfo.clear();
		for (int i = 0; i < 2; i++)
		{
			PIT::DomeLevelInfo stDomeLevelInfo = { i, 0, 5000, 0 };
			if (i == 0)
			{
				stDomeLevelInfo = { i, 0, 5000, 0 };
			}
			else if (i == 1)
			{
				stDomeLevelInfo = { i, 5000, 8000, 1 };
			}
			m_vecDomeLevelInfo.push_back(stDomeLevelInfo);

			vector<PIT::DomeLevelDetailInfo> vecDetailInfo;
			vecDetailInfo.clear();
			for (int j = 0; j < 4; j++)
			{
				PIT::DomeLevelDetailInfo stDomeDetailInfo = { j, i, j & 0x1, "12mm", 0, 200, 0.0, 0.0};
				if (stDomeLevelInfo.nLayoutType == 1)
				{
					if (j & 0x1)
					{
						stDomeDetailInfo.dAngleOrSpace = 0.5;
					}
					else
					{
						stDomeDetailInfo.dAngleOrSpace = 3.0;
					}
				}
				else if (stDomeLevelInfo.nLayoutType == 0)
				{
					stDomeDetailInfo.dAngleOrSpace = 200.0;
				}
				if (j == 2)
				{
					stDomeDetailInfo.dSpacing = 200000.0;
				}
				vecDetailInfo.push_back(stDomeDetailInfo);
			}
			m_mapDomeLevelDetailInfo.insert(make_pair(i, vecDetailInfo));
		}
	}
	if (m_Number > m_vecDomeLevelInfo.size())
	{
		for (int i = m_vecDomeLevelInfo.size(); i < m_Number; i++)
		{
			PIT::DomeLevelInfo stDomeLevelInfo = { i, 10000.0, 15000.0, 1 };
			m_vecDomeLevelInfo.push_back(stDomeLevelInfo);

			vector<PIT::DomeLevelDetailInfo> vecDetailInfo;
			for (int j = 0; j < 4; j++)
			{
				PIT::DomeLevelDetailInfo stDomeDetailInfo = { j, i, j & 0x1, "12mm", 0, 200.0, 0.0, 0.0};
				if (stDomeLevelInfo.nLayoutType == 1)
				{
					if (j & 0x1)
					{
						stDomeDetailInfo.dAngleOrSpace = 0.5;
					}
					else
					{
						stDomeDetailInfo.dAngleOrSpace = 3.0;
					}
				}
				else if (stDomeLevelInfo.nLayoutType == 0)
				{
					stDomeDetailInfo.dAngleOrSpace = 200.0;
				}

				if (j == 2)
				{
					stDomeDetailInfo.dSpacing = 200000.0;
				}
				vecDetailInfo.push_back(stDomeDetailInfo);
			}
			m_mapDomeLevelDetailInfo.insert(make_pair(i, vecDetailInfo));
		}
	}
	else if (m_Number < m_vecDomeLevelInfo.size())
	{
		int nSize = (int)m_vecDomeLevelInfo.size();
		for (int i = m_Number; i < nSize; i++)
		{
			auto itrFind = m_mapDomeLevelDetailInfo.find(m_vecDomeLevelInfo.at(m_vecDomeLevelInfo.size() - 1).nNumber);
			if (itrFind != m_mapDomeLevelDetailInfo.end())
			{
				m_mapDomeLevelDetailInfo.erase(itrFind);
			}

			m_vecDomeLevelInfo.pop_back();
		}
	}

	for (int i = 0; i < (int)m_vecDomeLevelInfo.size(); i++)
	{
		int nSubCnt = m_listCtl.GetColumnCount() - 1;

		m_listCtl.InsertItem(i, _T("")); // 插入行
		for (int j = 0; j <= nSubCnt; ++j)
		{
			CString strValue;
			switch (j)
			{
			case 0:
			{
				strValue.Format(_T("%d"), m_vecDomeLevelInfo[i].nNumber + 1);
				break;
			}
			case 1:
			{
				strValue.Format(_T("%.2f"), m_vecDomeLevelInfo[i].dRadius1);
				break;
			}
			case 2:
			{
				strValue.Format(_T("%.2f"), m_vecDomeLevelInfo[i].dRadius2);
				break;
			}
			case 3:
			{
				ListCtrlEx::CStrList strlist = g_listDomeRebarLayout;
				m_listCtl.SetCellStringList(i, j, strlist);
				auto it = strlist.begin();
				advance(it, m_vecDomeLevelInfo[i].nLayoutType);
				strValue = *it;
				break;
			}
			case 4:
			{
				strValue = _T("Click");
				break;
			}
			default: break;
			}

			m_listCtl.SetItemText(i, j, strValue);
		}
	}

	//	m_listMainRebar.SetShowProgressPercent(TRUE);
	//	m_listMainRebar.SetSupportSort(TRUE);
}

BEGIN_MESSAGE_MAP(CDomeRebarDlg, CDialogEx)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST2, &CDomeRebarDlg::OnLvnItemchangedList2)
	ON_MESSAGE(WM_ListCtrlEx_BUTTON_LBUTTONDOWN, &CDomeRebarDlg::OnButtonDown)
	ON_EN_CHANGE(IDC_EDIT1, &CDomeRebarDlg::OnEnChangeEdit1)
	ON_EN_KILLFOCUS(IDC_EDIT2, &CDomeRebarDlg::OnEnKillfocusEdit2)
END_MESSAGE_MAP()


// CDomeRebarDlg 消息处理程序
void CDomeRebarDlg::OnLvnItemchangedList2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}

void CDomeRebarDlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CString strTmp = CString();
	m_EditCover.GetWindowText(strTmp);

	m_Cover = atof(CT2A(strTmp));
}

void CDomeRebarDlg::DeleteRound()
{
	m_editRoundHandle.DeleteFromModel();
}

LRESULT CDomeRebarDlg::OnButtonDown(WPARAM wParam, LPARAM lParam)
{
	ListCtrlEx::ButtonCellMsg* msg = (ListCtrlEx::ButtonCellMsg*)lParam;

	if (m_pDomeDetailDlg != NULL)
	{
		vector<PIT::DomeLevelDetailInfo> vecDomeLevelDetailInfo;
		m_pDomeDetailDlg->GetDomeLevelDetailInfo(vecDomeLevelDetailInfo);
		if (vecDomeLevelDetailInfo.size() > 0)
		{
			auto itr = m_mapDomeLevelDetailInfo.find(vecDomeLevelDetailInfo.at(0).nNumber);
			if (itr == m_mapDomeLevelDetailInfo.end())
			{
				m_mapDomeLevelDetailInfo.insert(make_pair(vecDomeLevelDetailInfo.at(0).nNumber, vecDomeLevelDetailInfo));
			}
			else
			{
				itr->second = vecDomeLevelDetailInfo;
			}
		}
	}

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_pDomeDetailDlg = new CDomeDetailDlg();
	m_listCtl.GetAllRebarData(m_vecDomeLevelInfo);
	m_pDomeDetailDlg->SetLayoutType(m_vecDomeLevelInfo.at(msg->m_nRow).nLayoutType);
	auto itrTmp = m_mapDomeLevelDetailInfo.find(msg->m_nRow);
	if (itrTmp != m_mapDomeLevelDetailInfo.end())
	{
		m_pDomeDetailDlg->SetDomeLevelDetailInfo(itrTmp->second);
	}

	m_pDomeDetailDlg->Create(IDD_DIALOG_DomeDetail);
	m_pDomeDetailDlg->ShowWindow(SW_SHOW);

	return 0;
}

void CDomeRebarDlg::OnEnKillfocusEdit2()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strLevelNum;
	m_EditNumber.GetWindowText(strLevelNum);
	m_Number = atoi(CT2A(strLevelNum));

	UpdateRebarList();
}

