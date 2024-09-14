// CHoleRebar_AddUnionHoleDlg.cpp: 实现文件
//
#include "_USTATION.h"
#include "resource.h"
#include "CHoleRebar_ReinforcingDlg.h"
#include "CHoleRebar_AddUnionHoleDlg.h"
#include "CHoleRebar_StructualDlg.h"
#include "afxdialogex.h"
#include "ConstantsDef.h"
#include "GalleryIntelligentRebarids.h"
#include "ScanIntersectTool.h"
#include "ElementAttribute.h"
#include "HoleRebarAssembly.h"
#include "XmlHelper.h"
#include "PITMSCECommon.h"
// CHoleRebar_AddUnionHoleDlg 对话框

IMPLEMENT_DYNAMIC(CHoleRebar_AddUnionHoleDlg, CDialogEx)

CHoleRebar_AddUnionHoleDlg::CHoleRebar_AddUnionHoleDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_ADDUNIONHOLE, pParent)
{

}

CHoleRebar_AddUnionHoleDlg::~CHoleRebar_AddUnionHoleDlg()
{
}
// CWallRebarAssociatedComponentDlg 消息处理程序


BOOL CHoleRebar_AddUnionHoleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// TODO:  在此添加额外的初始化
	LONG lStyle;
	lStyle = GetWindowLong(m_list_addUnionHole.m_hWnd, GWL_STYLE);//获取当前窗口style
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;	//设置style
	lStyle |= LVS_SINGLESEL;//单选模式
	SetWindowLong(m_list_addUnionHole.m_hWnd, GWL_STYLE, lStyle);//设置style

	DWORD dwStyle = m_list_addUnionHole.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;					// 选中某行使整行高亮（仅仅适用与report 风格的listctrl ） 
	dwStyle |= LVS_EX_GRIDLINES;						// 网格线（仅仅适用与report 风格的listctrl ） 
	m_list_addUnionHole.SetExtendedStyle(dwStyle);			// 设置扩展风格 
	//设置列表控件的报表显示方式
	//m_listMainRebar.ModifyStyle(LVS_ICON | LVS_SMALLICON | LVS_LIST, LVS_REPORT);

	tagRECT stRect;
	m_list_addUnionHole.GetClientRect(&stRect);
	double width = stRect.right - stRect.left;
	//在列表控件中插入列
	m_list_addUnionHole.InsertColumn(0, _T("孔洞名称"), (int)(width / 3.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_list_addUnionHole.InsertColumn(1, _T("孔洞大小"), (int)(width / 3.0), ListCtrlEx::Normal, LVCFMT_CENTER, ListCtrlEx::SortByString);
	m_list_addUnionHole.InsertColumn(2, _T("是否添加到联合孔洞"), (int)(width / 3.0), ListCtrlEx::CheckBox, LVCFMT_CENTER, ListCtrlEx::SortByString);

	//	vector<PIT::AssociatedComponent> vecACData;
	
	UpdateACList();
	SetListDefaultData();
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}
void CHoleRebar_AddUnionHoleDlg::SetListDefaultData()
{
	
	m_list_addUnionHole.m_holeidAndmodel.insert(m_holeidAndmodel.begin(), m_holeidAndmodel.end());

}
void CHoleRebar_AddUnionHoleDlg::UpdateACList()
{
	int nItemNum = 0;
	if (m_type==ReinForcing)
	{
		for (int j = 0; j < m_vecReinF.size(); ++j)
		{
			if ( m_vecReinF[j].isUnionHole)//如果已经是联合洞口的子孔洞或联合孔洞，跳过显示
			{
				continue;
			}
			m_list_addUnionHole.InsertItem(nItemNum, _T("")); // 插入行
			m_list_addUnionHole.SetItemText(nItemNum, 0, CString(m_vecReinF[j].Hname));
			for (int k = 1; k < 3; ++k)
			{
				CString strValue;
				switch (k)
				{
				case 1:
					strValue.Format(_T("%f"), m_vecReinF[j].Hsize);
					break;
				case 2:
					//strValue.Format(_T("%f"), m_vecReinF[j].MainRebarDis);
					m_list_addUnionHole.SetCellChecked(j, k, FALSE);
					break;
				default:
					break;
				}
				m_list_addUnionHole.SetItemText(nItemNum, k, strValue);
			}
			nItemNum++;
		}
	}
	else
	{
		for (int j = 0; j < m_vecReinF.size(); ++j)
		{
			if ( m_vecReinF[j].isUnionHole)//如果已经是联合洞口的子孔洞或联合孔洞，跳过显示
			{
				continue;
			}
			m_list_addUnionHole.InsertItem(nItemNum, _T("")); // 插入行
			m_list_addUnionHole.SetItemText(nItemNum, 0, CString(m_vecReinF[j].Hname));
			for (int k = 1; k < 3; ++k)
			{
				CString strValue;
				switch (k)
				{
				case 1:
					strValue.Format(_T("%f"), m_vecReinF[j].Hsize);
					break;
				case 2:
					//strValue.Format(_T("%f"), m_vecReinF[j].MainRebarDis);
					m_list_addUnionHole.SetCellChecked(j, k, FALSE);
					break;
				default:
					break;
				}
				m_list_addUnionHole.SetItemText(nItemNum, k, strValue);
			}
			nItemNum++;
		}
	}
	
	//m_list_addUnionHole.SetShowProgressPercent(TRUE);
	//m_list_addUnionHole.SetSupportSort(TRUE);
}
void CHoleRebar_AddUnionHoleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_AddUnionHole, m_list_addUnionHole);
	DDX_Control(pDX, IDC_EDIT_UNIONNAME, m_edit_unionname);
}


BEGIN_MESSAGE_MAP(CHoleRebar_AddUnionHoleDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CHoleRebar_AddUnionHoleDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CHoleRebar_AddUnionHoleDlg 消息处理程序


void CHoleRebar_AddUnionHoleDlg::OnBnClickedOk()
{
	
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	mdlSelect_freeAll();
	m_list_addUnionHole.m_affectedElement.DeleteFromModel();
	CString strName;
	m_edit_unionname.GetWindowTextW(strName);
	if (strName!=L"")
	{
		m_list_addUnionHole.m_unionName = CT2A(strName);
		m_list_addUnionHole.GetAllRebarData(m_vecReinF);
		vector<string> vecUnionchildname;
		for (HoleRebarInfo::ReinForcingInfo tmpinfo : m_vecReinF)
		{
			if (tmpinfo.isUnionChild)
			{
				string tuname(tmpinfo.Uname);
				if (tuname.find(m_list_addUnionHole.m_unionName)!=string::npos)
				{
					string hname(tmpinfo.Hname);
					vecUnionchildname.push_back(hname);
				}
			}
		}
		int findtran = 0;
		for (string tname : vecUnionchildname)
		{
			if (findtran)
			{
				break;
			}
			if (m_holeidAndmodel[tname].ID != 0 && m_holeidAndmodel[tname].tModel != nullptr)
			{
				EditElementHandle eeh(m_holeidAndmodel[tname].ID, m_holeidAndmodel[tname].tModel);
				ElementCopyContext copier2(ACTIVEMODEL);
				copier2.SetSourceModelRef(eeh.GetModelRef());
				copier2.SetTransformToDestination(true);
				copier2.SetWriteElements(false);
				copier2.DoCopy(eeh);
				DPoint3d ptele = getCenterOfElmdescr(eeh.GetElementDescrP());
				if (m_FrontPts.size()>0)//非弧形墙时，此size大于0
				{
					for (int i = 0; i < m_FrontPts.size() - 1; i++)
					{

						vector<DPoint3d> interpts;
						DPoint3d tmpStr, tmpEnd;
						tmpStr = m_FrontPts[i];
						tmpEnd = m_FrontPts[i + 1];
						tmpStr.z = tmpEnd.z = ptele.z;
						GetIntersectPointsWithHole(interpts, &eeh, tmpStr, tmpEnd);
						if (interpts.size() > 0)
						{
							DPoint3d ptStart = m_FrontPts[i];
							DPoint3d ptEnd = m_FrontPts[i + 1];

							CVector3D  xVec(ptStart, ptEnd);

							CVector3D  yVec = CVector3D::kZaxis.CrossProduct(xVec);

							CVector3D  xVecNew(ptStart, ptEnd);
							BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVecNew, yVec, false);		//方向为X轴，水平垂直方向为Y轴

							placement.AssignTo(m_trans);
							m_trans.InverseOf(m_trans);
							findtran = 1;
							break;
						}
					}
				}
				else//弧形墙
				{
					DPoint3d ptcenter;
					if (!CHoleRebar_ReinforcingDlg::GetArcCenterPoint(m_RebarPts, ptcenter, &eeh))
					{
						continue;
					}
					ptcenter.z = ptele.z;
					CVector3D yVec = ptcenter - ptele;
					yVec.Normalize();

					CVector3D  xVec = yVec.CrossProduct(CVector3D::kZaxis);

					DPoint3d ptStart = ptcenter;
					BeMatrix   placement = CMatrix3D::Ucs(ptStart, xVec, yVec, false);		//方向为X轴，水平垂直方向为Y轴

					placement.AssignTo(m_trans);
					findtran = 1;
					break;
				}
				

			}
		}


		if (m_type == ReinForcing)
		{
			
			HoleRebarInfo::ReinForcingInfo tmpInfo;

			tmpInfo.v1 = 0;
			tmpInfo.v2 = 0;
			tmpInfo.h3 = 0;
			tmpInfo.h4 = 0;
			tmpInfo.numv1 = 1;
			tmpInfo.numv2 = 1;
			tmpInfo.numh3 = 1;
			tmpInfo.numh4 = 1;
			tmpInfo.spacingv1 = 150;//g_globalpara.Getrebarspacing();
			tmpInfo.spacingv2 = 150;//g_globalpara.Getrebarspacing();
			tmpInfo.spacingh3 = 150;//g_globalpara.Getrebarspacing();
			tmpInfo.spacingh4 = 150;//g_globalpara.Getrebarspacing();


			sprintf(tmpInfo.Hname, "%s", m_list_addUnionHole.m_unionName.c_str());
			tmpInfo.isUnionHole = true;
			tmpInfo.isUnionChild = false;
			tmpInfo.Hsize = 0;
			tmpInfo.MainRebarDis = 10;
			HoleRFRebarAssembly::GetUnionHoleeehAndSize(nullptr, tmpInfo.Hsize,
				m_vecReinF, m_trans, m_list_addUnionHole.m_unionName, m_holeidAndmodel);
			if (tmpInfo.Hsize == 0)
			{
				return;
			}
			else if (tmpInfo.Hsize < 300)
			{
				tmpInfo.v1 = 1;
				tmpInfo.h3 = 1;

			}
			else
			{
				tmpInfo.v1 = 1;
				tmpInfo.v2 = 1;
				tmpInfo.h3 = 1;
				tmpInfo.h4 = 1;
			}

			m_vecReinF.push_back(tmpInfo);
			m_Reinforcingdlg->SetListRowData(m_vecReinF);
			m_Reinforcingdlg->UpdateACList();
		}
		else
		{
		
			HoleRebarInfo::ReinForcingInfo tmpInfo;

			tmpInfo.v1 = 1;
			tmpInfo.v2 = 1;
			tmpInfo.h3 = 1;
			tmpInfo.h4 = 1;
			tmpInfo.have_twinv1 = 0;
			tmpInfo.have_twinv2 = 0;
			tmpInfo.have_twinh3 = 0;
			tmpInfo.have_twinh4 = 0;
			


			sprintf(tmpInfo.Hname, "%s", m_list_addUnionHole.m_unionName.c_str());
			tmpInfo.isUnionHole = true;
			tmpInfo.isUnionChild = false;
			tmpInfo.Hsize = 0;
			HoleRFRebarAssembly::GetUnionHoleeehAndSize(nullptr, tmpInfo.Hsize,
				m_vecReinF, m_trans, m_list_addUnionHole.m_unionName, m_holeidAndmodel);
			

			m_vecReinF.push_back(tmpInfo);
			m_Structualdlg->SetListRowData(m_vecReinF);
			m_Structualdlg->UpdateACList();
		}
		
	}
	
	



}
