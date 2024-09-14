// CRebarEndPointSetDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "resource.h"
#include "CRebarEndPointSetDlg.h"
#include "afxdialogex.h"
#include "RebarDetailElement.h"
#include "BentlyCommonfile.h"
#include "XmlHelper.h"
// CRebarEndPointSetDlg 对话框
extern GlobalParameters g_globalpara;	//全局参数

IMPLEMENT_DYNAMIC(CRebarEndPointSetDlg, CDialogEx)

CRebarEndPointSetDlg::CRebarEndPointSetDlg(int endType,CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_ENDTYPESET, pParent), m_endType(endType), m_bDrawPic(true)
{
	memset(&m_endPtInfo, 0, sizeof(PIT::EndType::RebarEndPointInfo));
}

CRebarEndPointSetDlg::~CRebarEndPointSetDlg()
{
}

void CRebarEndPointSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_EndType1, m_stEndType1);
	DDX_Control(pDX, IDC_STATIC_EndType2, m_stEndType2);
	DDX_Control(pDX, IDC_STATIC_EndType3, m_stEndType3);
	DDX_Control(pDX, IDC_STATIC_EndType4, m_stEndType4);
	DDX_Control(pDX, IDC_STATIC_EndType5, m_stEndType5);
	DDX_Control(pDX, IDC_STATIC_EndType6, m_stEndType6);
	DDX_Control(pDX, IDC_STATIC_EndTypePicture, m_stEndTypePic);
	DDX_Control(pDX, IDC_CHECK_OverrideTail, m_overrideTail);
}


BEGIN_MESSAGE_MAP(CRebarEndPointSetDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CRebarEndPointSetDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHECK_EndType1, &CRebarEndPointSetDlg::OnBnClickedCheckEndtype1)
	ON_BN_CLICKED(IDC_CHECK_EndType3, &CRebarEndPointSetDlg::OnBnClickedCheckEndtype3)
	ON_BN_CLICKED(IDC_CHECK_EndType4, &CRebarEndPointSetDlg::OnBnClickedCheckEndtype4)
	ON_BN_CLICKED(IDC_BUTTON_Load, &CRebarEndPointSetDlg::OnBnClickedButtonLoad)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_CHECK_OverrideTail, &CRebarEndPointSetDlg::OnBnClickedCheckOverridetail)
	ON_EN_KILLFOCUS(IDC_EDIT_EndType2, &CRebarEndPointSetDlg::OnEnChangeEditEndtype2)
	ON_EN_KILLFOCUS(IDC_EDIT_EndType3, &CRebarEndPointSetDlg::OnEnChangeEditEndtype3)
END_MESSAGE_MAP()


// CRebarEndPointSetDlg 消息处理程序


void CRebarEndPointSetDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strValue1, strValue2, strValue3, strValue4, strValue5, strValue6, strValue7, strValue8, strValue9;
	GetDlgItemText(IDC_EDIT_EndType1, strValue1);//弯曲直径
	GetDlgItemText(IDC_EDIT_EndType2, strValue3);//预留长度
	GetDlgItemText(IDC_EDIT_EndType3, strValue9);//尾部/肢
	GetDlgItemText(IDC_EDIT_EndType4, strValue2);//角度差
	GetDlgItemText(IDC_EDIT_EndType5, strValue5);
	GetDlgItemText(IDC_EDIT_EndType6, strValue6);
	GetDlgItemText(IDC_EDIT_EndType7, strValue7);
	GetDlgItemText(IDC_EDIT_EndType8, strValue8);
	GetDlgItemText(IDC_EDIT_EndType9, strValue9);
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	m_endPtInfo.value1 = atof(CT2A(strValue1)) * uor_per_mm;
	m_endPtInfo.value2 = atof(CT2A(strValue2)) * uor_per_mm;
	m_endPtInfo.value3 = atof(CT2A(strValue3)) * uor_per_mm;
	m_endPtInfo.value4 = atof(CT2A(strValue4)) * uor_per_mm;
	m_endPtInfo.value5 = atof(CT2A(strValue5)) * uor_per_mm;
	m_endPtInfo.value6 = atof(CT2A(strValue6)) * uor_per_mm;
	m_endPtInfo.value7 = atof(CT2A(strValue7)) * uor_per_mm;
	m_endPtInfo.value8 = atof(CT2A(strValue8)) * uor_per_mm;
	m_endPtInfo.value9 = atof(CT2A(strValue9)) * uor_per_mm;

	CDialogEx::OnOK();
}


BOOL CRebarEndPointSetDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_overrideTail.SetCheck(false);

	CString strValue1, strValue2, strValue3, strValue4, strValue5, strValue6;
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	switch (m_endType)
	{
	case 0:
	{
		m_stEndType1.ShowWindow(FALSE);
		m_stEndType2.ShowWindow(FALSE);
		m_stEndType3.ShowWindow(FALSE);
		m_stEndType4.ShowWindow(FALSE);
		m_stEndType5.ShowWindow(FALSE);
		m_stEndType6.ShowWindow(FALSE);
		m_overrideTail.ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType1)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType2)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType3)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType4)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType5)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType6)->ShowWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType)->EnableWindow(FALSE);
	}
	break;
	case 1:		//弯曲
	{
		m_stEndType1.SetWindowText(L"1.弯折直径(mm)");
		m_stEndType2.SetWindowText(L"3.长度(mm)");
		m_stEndType3.SetWindowText(L"9.尾部/肢(mm)");
		m_stEndType4.SetWindowText(L"2.角度差(mm)");
		m_stEndType3.ShowWindow(FALSE);
		//m_stEndType3.EnableWindow(FALSE);
		m_stEndType5.ShowWindow(FALSE);
		m_stEndType6.ShowWindow(FALSE);
		m_overrideTail.ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType3)->ShowWindow(FALSE);
		//GetDlgItem(IDC_EDIT_EndType3)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType5)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType6)->ShowWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType)->SetWindowText(L"弯曲");
		GetDlgItem(IDC_COMBO_EndType)->EnableWindow(FALSE);
		if (COMPARE_VALUES(m_endPtInfo.value1, 0) == 0)
		{
			m_endPtInfo.value1 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		strValue1.Format(L"%.2f", m_endPtInfo.value1 / uor_per_mm);//弯折直径
		strValue2.Format(L"%.2f", m_endPtInfo.value3 / uor_per_mm);//预留长度
		strValue3.Format(L"%.2f", m_endPtInfo.value9 / uor_per_mm);//尾部/肢
		strValue4.Format(L"%.2f", m_endPtInfo.value2 / uor_per_mm);//角度差
	}
	break;
	case 2:		//吊钩
	{
		m_stEndType1.SetWindowText(L"1.销钉直径(mm)");
		m_stEndType2.SetWindowText(L"3.长度(mm)");
		m_stEndType3.SetWindowText(L"9.尾部/肢(mm)");
		m_stEndType4.SetWindowText(L"4.弯折直径(mm)");
		m_stEndType5.SetWindowText(L"6.长度(mm)");
		m_stEndType3.ShowWindow(FALSE);
		//m_stEndType3.EnableWindow(FALSE);
		m_stEndType6.ShowWindow(FALSE);
		m_overrideTail.ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType3)->ShowWindow(FALSE);
		//GetDlgItem(IDC_EDIT_EndType3)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType6)->ShowWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType)->SetWindowText(L"吊钩");
		GetDlgItem(IDC_COMBO_EndType)->EnableWindow(FALSE);
		if (COMPARE_VALUES(m_endPtInfo.value1, 0) == 0)
		{
			m_endPtInfo.value1 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		if (COMPARE_VALUES(m_endPtInfo.value4, 0) == 0)
		{
			m_endPtInfo.value4 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		strValue1.Format(L"%.2f", m_endPtInfo.value1 / uor_per_mm);//销钉直径
		strValue2.Format(L"%.2f", m_endPtInfo.value3 / uor_per_mm);//长度
		strValue3.Format(L"%.2f", m_endPtInfo.value9 / uor_per_mm);//尾部/肢
		strValue4.Format(L"%.2f", m_endPtInfo.value4 / uor_per_mm);//弯折直径
		strValue5.Format(L"%.2f", m_endPtInfo.value6 / uor_per_mm);//预留长度
	}
	break;
	case 3:		//折线
	{
		m_stEndType1.SetWindowText(L"6.长度(mm)");
		m_stEndType2.SetWindowText(L"7.长度(mm)");
		m_stEndType3.ShowWindow(FALSE);
		m_stEndType4.ShowWindow(FALSE);
		m_stEndType5.SetWindowText(L"8.深度(mm)");
		m_stEndType3.ShowWindow(FALSE);
		m_stEndType4.ShowWindow(FALSE);
		m_stEndType6.ShowWindow(FALSE);
		m_overrideTail.ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType3)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType4)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType6)->ShowWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType)->SetWindowText(L"折线");
		GetDlgItem(IDC_COMBO_EndType)->EnableWindow(FALSE);
		strValue1.Format(L"%.2f", m_endPtInfo.value6);//长度
		strValue2.Format(L"%.2f", m_endPtInfo.value7);//长度
		strValue3.Format(L"%.2f", m_endPtInfo.value8);//深度
	}
	break;
	case 4:		//90度弯钩
	{
		m_stEndType1.SetWindowText(L"1.弯折直径(mm)");
		m_stEndType2.SetWindowText(L"3.长度(mm)");
		m_stEndType3.SetWindowText(L"9.尾部/肢(mm)");
		//m_stEndType3.ShowWindow(FALSE);
		m_stEndType3.EnableWindow(FALSE);
		m_stEndType4.ShowWindow(FALSE);
		m_stEndType5.ShowWindow(FALSE);
		m_stEndType6.ShowWindow(FALSE);
		//GetDlgItem(IDC_EDIT_EndType3)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType3)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType4)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType5)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType6)->ShowWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType)->SetWindowText(L"90度弯钩");
		GetDlgItem(IDC_COMBO_EndType)->EnableWindow(FALSE);
		if (COMPARE_VALUES(m_endPtInfo.value3, 0) == 0)
		{
			RebarEndType endType;
			endType.SetType(RebarEndType::kBend);
			m_endPtInfo.value3 = RebarCode::GetBendLength(m_strRebarSize, endType,ACTIVEMODEL);
		}
		if (COMPARE_VALUES(m_endPtInfo.value1, 0) == 0)
		{
			m_endPtInfo.value1 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		if (COMPARE_VALUES(m_endPtInfo.value9, 0) == 0)
		{
			m_rebarDia = RebarCode::GetBarDiameter(m_strRebarSize, ACTIVEMODEL);
			m_endPtInfo.value9 = m_endPtInfo.value3 + m_endPtInfo.value1 / 2 + m_rebarDia;
		}
		strValue1.Format(L"%.2f", m_endPtInfo.value1 / uor_per_mm);//弯折直径
		strValue2.Format(L"%.2f", m_endPtInfo.value3 / uor_per_mm);//长度
		strValue3.Format(L"%.2f", m_endPtInfo.value9 / uor_per_mm);//尾部/肢
	}
	break;
	case 5:		//135度弯钩
	{
		m_stEndType1.SetWindowText(L"1.弯折直径(mm)");
		m_stEndType2.SetWindowText(L"3.长度(mm)");
		m_stEndType3.SetWindowText(L"9.尾部/肢(mm)");
		//m_stEndType3.ShowWindow(FALSE);
		m_stEndType3.EnableWindow(FALSE);
		m_stEndType4.ShowWindow(FALSE);
		m_stEndType5.ShowWindow(FALSE);
		m_stEndType6.ShowWindow(FALSE);
		//GetDlgItem(IDC_EDIT_EndType3)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType3)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType4)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType5)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType6)->ShowWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType)->SetWindowText(L"135度弯钩");
		GetDlgItem(IDC_COMBO_EndType)->EnableWindow(FALSE);
		if (COMPARE_VALUES(m_endPtInfo.value3, 0) == 0)
		{
			RebarEndType endType;
			endType.SetType(RebarEndType::kCog);
			m_endPtInfo.value3 = RebarCode::GetBendLength(m_strRebarSize, endType, ACTIVEMODEL);
		}
		if (COMPARE_VALUES(m_endPtInfo.value1, 0) == 0)
		{
			m_endPtInfo.value1 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		if (COMPARE_VALUES(m_endPtInfo.value9, 0) == 0)
		{
			m_rebarDia = RebarCode::GetBarDiameter(m_strRebarSize, ACTIVEMODEL);
			m_endPtInfo.value9 = m_endPtInfo.value3 + m_endPtInfo.value1 / 2 + m_rebarDia;
		}
		strValue1.Format(L"%.2f", m_endPtInfo.value1 / uor_per_mm);//弯折直径
		strValue2.Format(L"%.2f", m_endPtInfo.value3 / uor_per_mm);//长度
		strValue3.Format(L"%.2f", m_endPtInfo.value9 / uor_per_mm);//尾部/肢
	}
	break;
	case 6:		//180度弯钩
	{
		m_stEndType1.SetWindowText(L"1.弯折直径(mm)");
		m_stEndType2.SetWindowText(L"3.长度(mm)");
		m_stEndType3.SetWindowText(L"9.尾部/肢(mm)");
	//	m_stEndType3.ShowWindow(FALSE);
		m_stEndType3.EnableWindow(FALSE);
		m_stEndType4.ShowWindow(FALSE);
		m_stEndType5.ShowWindow(FALSE);
		m_stEndType6.ShowWindow(FALSE);
		//GetDlgItem(IDC_EDIT_EndType3)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType3)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType4)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType5)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType6)->ShowWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType)->SetWindowText(L"180度弯钩");
		GetDlgItem(IDC_COMBO_EndType)->EnableWindow(FALSE);
		if (COMPARE_VALUES(m_endPtInfo.value3,0) == 0)
		{
			RebarEndType endType;
			endType.SetType(RebarEndType::kHook);
			m_endPtInfo.value3 = RebarCode::GetBendLength(m_strRebarSize, endType, ACTIVEMODEL);
		}
		if (COMPARE_VALUES(m_endPtInfo.value1, 0) == 0)
		{
			m_endPtInfo.value1 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		if (COMPARE_VALUES(m_endPtInfo.value9, 0) == 0)
		{
			m_rebarDia = RebarCode::GetBarDiameter(m_strRebarSize, ACTIVEMODEL);
			m_endPtInfo.value9 = m_endPtInfo.value3 + m_endPtInfo.value1 / 2 + m_rebarDia;
		}
		strValue1.Format(L"%.2f", m_endPtInfo.value1 / uor_per_mm);//弯折直径
		strValue2.Format(L"%.2f", m_endPtInfo.value3 / uor_per_mm);//长度
		strValue3.Format(L"%.2f", m_endPtInfo.value9 / uor_per_mm);//尾部/肢
	}
	break;
	case 7:		//直锚
	{
		m_stEndType1.SetWindowText(L"3.长度(mm)");
		m_stEndType2.ShowWindow(FALSE);
		m_stEndType3.ShowWindow(FALSE);
		m_stEndType4.ShowWindow(FALSE);
		m_stEndType5.ShowWindow(FALSE);
		m_stEndType6.ShowWindow(FALSE);
		m_overrideTail.ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType2)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType3)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType4)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType5)->ShowWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType6)->ShowWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType)->SetWindowText(L"直锚");
		GetDlgItem(IDC_COMBO_EndType)->EnableWindow(FALSE);
		if (COMPARE_VALUES(m_endPtInfo.value1, 0) == 0)
		{
			//int iRebarSize = atoi(m_strRebarSize);
			m_endPtInfo.value1 = g_globalpara.m_alength.at((string)m_strRebarSize) * uor_per_mm;
		}
		strValue1.Format(L"%.2f", m_endPtInfo.value1 / uor_per_mm);//长度
	}
	break;
	case 8:		//用户
	{
		m_stEndType1.SetWindowText(L"1.销钉直径(mm)");
		m_stEndType2.SetWindowText(L"2.角度");
		m_stEndType3.SetWindowText(L"3.长度(mm)");
		m_stEndType4.SetWindowText(L"4.销钉直径(mm)");
		m_stEndType5.SetWindowText(L"5.角度");
		m_stEndType6.SetWindowText(L"6.长度(mm)");
		m_overrideTail.ShowWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType)->SetWindowText(L"吊钩");
		GetDlgItem(IDC_COMBO_EndType)->EnableWindow(FALSE);
		if (COMPARE_VALUES(m_endPtInfo.value1, 0) == 0)
		{
			m_endPtInfo.value1 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		if (COMPARE_VALUES(m_endPtInfo.value4, 0) == 0)
		{
			m_endPtInfo.value4 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		strValue1.Format(L"%.2f", m_endPtInfo.value1 / uor_per_mm);//销钉直径
		strValue2.Format(L"%.2f", m_endPtInfo.value2 / uor_per_mm);//角度
		strValue3.Format(L"%.2f", m_endPtInfo.value3 / uor_per_mm);//长度
		strValue4.Format(L"%.2f", m_endPtInfo.value4 / uor_per_mm);//销钉直径
		strValue5.Format(L"%.2f", m_endPtInfo.value5 / uor_per_mm);//角度
		strValue6.Format(L"%.2f", m_endPtInfo.value6 / uor_per_mm);//长度
	}
	break;
	default:
		break;
	}



	SetDlgItemText(IDC_EDIT_EndType1, strValue1);
	SetDlgItemText(IDC_EDIT_EndType2, strValue2);
	SetDlgItemText(IDC_EDIT_EndType3, strValue3);
	SetDlgItemText(IDC_EDIT_EndType4, strValue4);
	SetDlgItemText(IDC_EDIT_EndType5, strValue5);
	SetDlgItemText(IDC_EDIT_EndType6, strValue6);

	if (!IsDlgButtonChecked(IDC_CHECK_EndType1))
	{
		GetDlgItem(IDC_COMBO_EndType2)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType3)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType4)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType5)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType6)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType7)->EnableWindow(FALSE);

		if (!IsDlgButtonChecked(IDC_CHECK_EndType3))
		{
			GetDlgItem(IDC_COMBO_EndType7)->EnableWindow(FALSE);
			GetDlgItem(IDC_COMBO_EndType8)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_EndType8)->EnableWindow(FALSE);
			if (!IsDlgButtonChecked(IDC_CHECK_EndType4))
			{
				GetDlgItem(IDC_EDIT_EndType9)->EnableWindow(FALSE);
			}
		}
	}
	else
	{
		GetDlgItem(IDC_CHECK_EndType3)->EnableWindow(FALSE);

		GetDlgItem(IDC_COMBO_EndType7)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType8)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType8)->EnableWindow(FALSE);
		if (!IsDlgButtonChecked(IDC_CHECK_EndType4))
		{
			GetDlgItem(IDC_EDIT_EndType9)->EnableWindow(FALSE);
		}
	}

//	UpdateData(FALSE);
	return TRUE;
}


void CRebarEndPointSetDlg::OnBnClickedCheckEndtype1()
{
	// TODO: 在此添加控件通知处理程序代码
	BOOL bState = IsDlgButtonChecked(IDC_CHECK_EndType1);

	GetDlgItem(IDC_COMBO_EndType2)->EnableWindow(bState);
	GetDlgItem(IDC_COMBO_EndType3)->EnableWindow(bState);
	GetDlgItem(IDC_COMBO_EndType4)->EnableWindow(bState);
	GetDlgItem(IDC_COMBO_EndType5)->EnableWindow(bState);
	GetDlgItem(IDC_COMBO_EndType6)->EnableWindow(bState);
	GetDlgItem(IDC_EDIT_EndType7)->EnableWindow(bState);

	if (bState && !IsDlgButtonChecked(IDC_CHECK_EndType3))
	{
		GetDlgItem(IDC_COMBO_EndType7)->EnableWindow(FALSE);
		GetDlgItem(IDC_COMBO_EndType8)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType8)->EnableWindow(FALSE);
		if (!IsDlgButtonChecked(IDC_CHECK_EndType4))
		{
			GetDlgItem(IDC_EDIT_EndType9)->EnableWindow(FALSE);
		}
	}
}


void CRebarEndPointSetDlg::OnBnClickedCheckEndtype3()
{
	// TODO: 在此添加控件通知处理程序代码
	BOOL bState = IsDlgButtonChecked(IDC_CHECK_EndType3);

	GetDlgItem(IDC_COMBO_EndType7)->EnableWindow(bState);
	GetDlgItem(IDC_COMBO_EndType8)->EnableWindow(bState);
	GetDlgItem(IDC_EDIT_EndType8)->EnableWindow(bState);
	if (bState && !IsDlgButtonChecked(IDC_CHECK_EndType4))
	{
		GetDlgItem(IDC_EDIT_EndType9)->EnableWindow(FALSE);
	}
}


void CRebarEndPointSetDlg::OnBnClickedCheckEndtype4()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CRebarEndPointSetDlg::OnBnClickedButtonLoad()
{
	// TODO: 在此添加控件通知处理程序代码
	//加载全局参数
	CString strValue1, strValue2, strValue3, strValue4, strValue5, strValue6;
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;

	switch (m_endType)
	{
	case 0:
		break;
	case 1:		//弯曲
	{
		m_stEndType1.SetWindowText(L"1.弯折直径(mm)");
		m_stEndType2.SetWindowText(L"3.长度(mm)");
		m_stEndType3.SetWindowText(L"9.尾部/肢(mm)");
		m_stEndType4.SetWindowText(L"2.角度差(mm)");
		if (COMPARE_VALUES(m_endPtInfo.value1, 0) == 0)
		{
			m_endPtInfo.value1 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		strValue1.Format(L"%.2f", m_endPtInfo.value1 / uor_per_mm);//弯折直径
		strValue2.Format(L"%.2f", m_endPtInfo.value3 / uor_per_mm);//预留长度
		strValue3.Format(L"%.2f", m_endPtInfo.value9 / uor_per_mm);//尾部/肢
		strValue4.Format(L"%.2f", m_endPtInfo.value2 / uor_per_mm);//角度差
	}
	break;
	case 2:		//吊钩
	{
		m_stEndType1.SetWindowText(L"1.销钉直径(mm)");
		m_stEndType2.SetWindowText(L"3.长度(mm)");
		m_stEndType3.SetWindowText(L"9.尾部/肢(mm)");
		m_stEndType4.SetWindowText(L"4.弯折直径(mm)");
		m_stEndType5.SetWindowText(L"6.长度(mm)");
		if (COMPARE_VALUES(m_endPtInfo.value1, 0) == 0)
		{
			m_endPtInfo.value1 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		if (COMPARE_VALUES(m_endPtInfo.value4, 0) == 0)
		{
			m_endPtInfo.value4 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		strValue1.Format(L"%.2f", m_endPtInfo.value1 / uor_per_mm);//销钉直径
		strValue2.Format(L"%.2f", m_endPtInfo.value3 / uor_per_mm);//长度
		strValue3.Format(L"%.2f", m_endPtInfo.value9 / uor_per_mm);//尾部/肢
		strValue4.Format(L"%.2f", m_endPtInfo.value4 / uor_per_mm);//弯折直径
		strValue5.Format(L"%.2f", m_endPtInfo.value6 / uor_per_mm);//预留长度
	}
	break;
	case 3:		//折线
	{
		m_stEndType1.SetWindowText(L"6.长度(mm)");
		m_stEndType2.SetWindowText(L"7.长度(mm)");
		m_stEndType5.SetWindowText(L"8.深度(mm)");
		strValue1.Format(L"%.2f", m_endPtInfo.value6);//长度
		strValue2.Format(L"%.2f", m_endPtInfo.value7);//长度
		strValue3.Format(L"%.2f", m_endPtInfo.value8);//深度
	}
	break;
	case 4:		//90度弯钩
	case 5:		//135度弯钩
	case 6:		//180度弯钩
	{
		m_stEndType1.SetWindowText(L"1.弯折直径(mm)");
		m_stEndType2.SetWindowText(L"3.长度(mm)");
		m_stEndType3.SetWindowText(L"9.尾部/肢(mm)");
		if (COMPARE_VALUES(m_endPtInfo.value3, 0) == 0)
		{
			RebarEndType endType;
			endType.SetType(RebarEndType::kHook);
			m_endPtInfo.value3 = RebarCode::GetBendLength(m_strRebarSize, endType, ACTIVEMODEL);
		}
		if (COMPARE_VALUES(m_endPtInfo.value1, 0) == 0)
		{
			m_endPtInfo.value1 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		strValue1.Format(L"%.2f", m_endPtInfo.value1 / uor_per_mm);//弯折直径
		strValue2.Format(L"%.2f", m_endPtInfo.value3 / uor_per_mm);//长度
		strValue3.Format(L"%.2f", m_endPtInfo.value9 / uor_per_mm);//尾部/肢
	}
	break;
	case 7:		//直锚
	{
		double defaultValue;
		//int iRebarSize = atoi(m_strRebarSize);
		defaultValue = g_globalpara.m_alength.at((string)m_strRebarSize);
		strValue1.Format(L"%.2f", defaultValue);//长度
	}
	break;
	case 8:		//用户
	{
		if (COMPARE_VALUES(m_endPtInfo.value1, 0) == 0)
		{
			m_endPtInfo.value1 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		if (COMPARE_VALUES(m_endPtInfo.value4, 0) == 0)
		{
			m_endPtInfo.value4 = RebarCode::GetPinRadius(m_strRebarSize, ACTIVEMODEL, false);
		}
		strValue1.Format(L"%.2f", m_endPtInfo.value1 / uor_per_mm);//销钉直径
		strValue2.Format(L"%.2f", m_endPtInfo.value2 / uor_per_mm);//角度
		strValue3.Format(L"%.2f", m_endPtInfo.value3 / uor_per_mm);//长度
		strValue4.Format(L"%.2f", m_endPtInfo.value4 / uor_per_mm);//销钉直径
		strValue5.Format(L"%.2f", m_endPtInfo.value5 / uor_per_mm);//角度
		strValue6.Format(L"%.2f", m_endPtInfo.value6 / uor_per_mm);//长度
	}
	break;
	default:
		break;
	}

	SetDlgItemText(IDC_EDIT_EndType1, strValue1);
	SetDlgItemText(IDC_EDIT_EndType2, strValue2);
	SetDlgItemText(IDC_EDIT_EndType3, strValue3);
	SetDlgItemText(IDC_EDIT_EndType4, strValue4);
	SetDlgItemText(IDC_EDIT_EndType5, strValue5);
	SetDlgItemText(IDC_EDIT_EndType6, strValue6);
	UpdateData(FALSE);
}


void CRebarEndPointSetDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	if (m_bDrawPic)
	{
		CBitmap bitmap;  // CBitmap对象，用于加载位图  
		HBITMAP hBmp;    // 保存CBitmap加载的位图的句柄   

		UINT bitmapId = 0;

		switch (m_endType)
		{
		case 0:
			bitmapId = IDB_BITMAP14;
			break;
		case 1:		//弯曲
			bitmapId = IDB_BITMAP15;
			break;
		case 2:		//吊钩
			bitmapId = IDB_BITMAP16;
			break;
		case 3:		//折线
			bitmapId = IDB_BITMAP17;
			break;
		case 4:		//90度弯钩
			bitmapId = IDB_BITMAP18;
			break;
		case 5:		//135度弯钩
			bitmapId = IDB_BITMAP19;
			break;
		case 6:		//180度弯钩
			bitmapId = IDB_BITMAP20;
		break;
		case 7:		//直锚
			bitmapId = IDB_BITMAP22;
			break;
		case 8:		//用户
			bitmapId = IDB_BITMAP21;
			break;
		default:
			break;
		}
		bitmap.LoadBitmap(bitmapId);
		// 将位图IDB_BITMAP1加载到bitmap   
		hBmp = (HBITMAP)bitmap.GetSafeHandle();  // 获取bitmap加载位图的句柄   
		m_stEndTypePic.SetBitmap(hBmp);
		m_bDrawPic = false;
	}
}


void CRebarEndPointSetDlg::OnBnClickedCheckOverridetail()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_overrideTail.GetCheck())
	{
		m_stEndType3.EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT_EndType3)->EnableWindow(TRUE);
		m_stEndType2.EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType2)->EnableWindow(FALSE);
	}
	else
	{
		m_stEndType3.EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_EndType3)->EnableWindow(FALSE);
		m_stEndType2.EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT_EndType2)->EnableWindow(TRUE);
	}
}


void CRebarEndPointSetDlg::OnEnChangeEditEndtype2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	CString value3;
	GetDlgItem(IDC_EDIT_EndType2)->GetWindowTextW(value3);
	double val3 = _ttof(value3) * uor_per_mm;
	double val9 = val3 + m_endPtInfo.value1 / 2 + m_rebarDia;
	CString value9;
	value9.Format(L"%.2f", val9 / uor_per_mm);//尾部/肢
	GetDlgItem(IDC_EDIT_EndType3)->SetWindowTextW(value9);
}


void CRebarEndPointSetDlg::OnEnChangeEditEndtype3()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	CString value9;
	GetDlgItem(IDC_EDIT_EndType3)->GetWindowTextW(value9);
	double val9 = _ttof(value9) * uor_per_mm;
	double val3 = val9 - m_endPtInfo.value1 / 2 - m_rebarDia;
	CString value3;
	value3.Format(L"%.2f", val3 / uor_per_mm);
	GetDlgItem(IDC_EDIT_EndType2)->SetWindowTextW(value3);
}
