// ShowHideRebarByDiaDlg.cpp: 实现文件
//
#include "_ustation.h"
#include "GalleryIntelligentRebar.h"
#include "ShowHideRebarByDiaDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"


// ShowHideRebarByDiaDlg 对话框

IMPLEMENT_DYNAMIC(ShowHideRebarByDiaDlg, CDialogEx)

ShowHideRebarByDiaDlg::ShowHideRebarByDiaDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_ShowHideRebarByDia, pParent)
{

}

ShowHideRebarByDiaDlg::~ShowHideRebarByDiaDlg()
{
}

void ShowHideRebarByDiaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_DIA, m_rebarDiaCombo);
	DDX_Control(pDX, IDC_COMBO_SHOWHIDE, m_showHideCombo);
}


BOOL ShowHideRebarByDiaDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端

	for each (auto var in g_listRebarSize)
		m_rebarDiaCombo.AddString(var);
	m_rebarDiaCombo.SetCurSel(0);
	OnCbnSelchangeComboDia();

	m_showHideCombo.InsertString(0, CString("显示"));
	m_showHideCombo.InsertString(1, CString("隐藏"));
	m_showHideCombo.SetCurSel(0);
	m_isShow = true;

	return TRUE;
}

BEGIN_MESSAGE_MAP(ShowHideRebarByDiaDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &ShowHideRebarByDiaDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBO_DIA, &ShowHideRebarByDiaDlg::OnCbnSelchangeComboDia)
	ON_CBN_SELCHANGE(IDC_COMBO_SHOWHIDE, &ShowHideRebarByDiaDlg::OnCbnSelchangeComboShowhide)
END_MESSAGE_MAP()


// ShowHideRebarByDiaDlg 消息处理程序


void ShowHideRebarByDiaDlg::OnBnClickedOk()
{
	for (auto it : m_selectRebars)
	{
		EditElementHandle eeh(it, ACTIVEMODEL);
		if (RebarElement::IsRebarElement(eeh)) //是否为钢筋元素
		{
			//从EditElementHandle获取元素
			RebarElementP rep = RebarElement::Fetch(eeh);
			//(*elementToModify).GetModelRef()：从ElementAgenda的元素中提取DgnModelRef
			//DgnModelRef提供了对Bentley::DgnPlatform::DgnFile中的模型的访问
			//获取钢筋模板对象
			RebarShape * rebarshape = rep->GetRebarShape((eeh).GetModelRef());
			if (rebarshape == nullptr)
				continue;
			//获取钢筋模板中的线条形状
			RebarCurve curve;
			rebarshape->GetRebarCurve(curve);
			//获取钢筋的尺寸数据
			BrString Sizekey = (rebarshape->GetShapeData().GetSizeKey());
			//从钢筋尺寸数据中获取直径
			double diameter = RebarCode::GetBarDiameter(Sizekey, ACTIVEMODEL); //直径
			if (diameter != m_rebarDia)
			{
				continue;
			}
			if (m_isShow)
			{
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), true);
			}
			else
			{
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
			}
			eeh.ReplaceInModel(it);
		}
	}
	CDialogEx::OnOK();
}


void ShowHideRebarByDiaDlg::OnCbnSelchangeComboDia()
{
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0; //根据当前dgn模型获取比例因子
	int nIndex = m_rebarDiaCombo.GetCurSel();
	CString rebarDiaStr;
	m_rebarDiaCombo.GetLBText(nIndex, rebarDiaStr);
	if (rebarDiaStr.Find(_T("mm")) != -1)
	{
		rebarDiaStr.Replace(_T("mm"), _T(""));
	}
	m_rebarDia = _ttof(rebarDiaStr) * uor_per_mm;
}


void ShowHideRebarByDiaDlg::OnCbnSelchangeComboShowhide()
{
	int index = m_showHideCombo.GetCurSel();
	if (index == 0)
	{
		m_isShow = true;
	}
	else
	{
		m_isShow = false;
	}
}
