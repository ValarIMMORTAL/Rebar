// CHiddenBeamSetDlg.cpp: 实现文件
//

#include "_USTATION.h"
#include "GalleryIntelligentRebar.h"
#include "CHiddenBeamSetDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ConstantsDef.h"
#include <RebarCatalog.h>
#include "RebarElements.h"
#include "ExtractFacesTool.h"
#include "ElementAttribute.h"

// CHiddenBeamSetDlg 对话框

IMPLEMENT_DYNAMIC(CHiddenBeamSetDlg, CDialogEx)

CHiddenBeamSetDlg::CHiddenBeamSetDlg(ElementHandleCR eh, bool bEmbededCloumn, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_HiddenBeam, pParent), m_ehSel(eh), m_bEmbededCloumn(bEmbededCloumn)
{
	vector<DSegment3d> frontSegs;
	vector<DSegment3d> backSegs;
	double dLen = 0;
	double dHeight = 0;
	EFT::GetFrontBackLinePoint(eh, frontSegs, backSegs, &dHeight);
	if (!frontSegs.empty())
	{
		dLen = frontSegs.front().Length();
	}
	else if (!backSegs.empty())
	{
		dLen = frontSegs.back().Length();
	}
	else
	{
		dLen = 4400;
	}

	if (m_bEmbededCloumn)
	{
		m_BeamLen = dHeight;
	}
	else
	{
		m_BeamLen = dLen;
	}
}

CHiddenBeamSetDlg::~CHiddenBeamSetDlg()
{

}

void CHiddenBeamSetDlg::InitCheck()
{
	m_Check.SetCheck(0);
}

void CHiddenBeamSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_CombDownFace);
	DDX_Control(pDX, IDC_CHECK1, m_Check);
}


BEGIN_MESSAGE_MAP(CHiddenBeamSetDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CHiddenBeamSetDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CHiddenBeamSetDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON1, &CHiddenBeamSetDlg::OnBnClickedButton1)
	ON_EN_CHANGE(IDC_EDIT1, &CHiddenBeamSetDlg::OnEnChangeEdit1)
	ON_BN_CLICKED(IDC_CHECK1, &CHiddenBeamSetDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_BUTTON3, &CHiddenBeamSetDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON2, &CHiddenBeamSetDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_CHECK5, &CHiddenBeamSetDlg::OnBnClickedCheck5)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON12, &CHiddenBeamSetDlg::OnBnClickedButton12)
END_MESSAGE_MAP()


// CHiddenBeamSetDlg 消息处理程序


bool CHiddenBeamSetDlg::SetDownFace(ElementRefP pElmDownFace)
{
	if (pElmDownFace == nullptr)
	{
		return false;
	}

	m_pElmDownFace = pElmDownFace;
	m_vecNormal = GetFaceNormal();
	return true;
}

DVec3d CHiddenBeamSetDlg::GetFaceNormal()
{
	if (m_pElmDownFace == nullptr)
	{
		return DVec3d();
	}

	EditElementHandle eeh(m_pElmDownFace, ACTIVEMODEL);
	DVec3d vecNormal;
	mdlElmdscr_extractNormal(&vecNormal, nullptr, eeh.GetElementDescrCP(), nullptr);
	return vecNormal;
}

bool CHiddenBeamSetDlg::DrawBeam(bool negate)
{
	if (m_pElmDownFace != nullptr)
	{
		EditElementHandle eeh(m_pElmDownFace, ACTIVEMODEL);

		DVec3d vecNormal;
		DPoint3d ptInFace;
		if (SUCCESS != mdlElmdscr_extractNormal(&vecNormal, &ptInFace, eeh.GetElementDescrCP(), nullptr))
		{
			return false;
		}
		
		vecNormal.ScaleToLength(m_BeamLen);
		if (negate)
		{
			vecNormal.Negate();
		}

		EditElementHandle eehBeam;
		SolidHandler::CreateProjectionElement(eehBeam, NULL, eeh, ptInFace, vecNormal, NULL, true, *ACTIVEMODEL);	
		if (m_elmRef == nullptr)
		{
			eehBeam.AddToModel();
			m_elmRef = eehBeam.GetElementRef();
		}
		else
		{
			EditElementHandle eehOld(m_elmRef, ACTIVEMODEL);
			if (eehOld.GetElementDescrP() == nullptr)
			{
				eehBeam.AddToModel();
				m_elmRef = eehBeam.GetElementRef();
			}
			else
			{
				eehOld.ReplaceElementDescr(eehBeam.ExtractElementDescr());
				eehOld.ReplaceInModel(m_elmRef);
			}

		}
	}
	return true;
}

BOOL CHiddenBeamSetDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowPos(&this->wndTopMost, 0, 0, 0, 0, SWP_NOSIZE);//让对话框界面显示在最前端
	m_CombDownFace.ResetContent();
	if (m_bEmbededCloumn)
	{
		SetWindowText(L"暗柱配筋");
		GetDlgItem(IDC_BUTTON1)->SetWindowText(L"绘制暗柱底面");
		for each (auto var in g_listDownFace)
			m_CombDownFace.AddString(var);

		m_CombDownFace.SetCurSel(0);
	}
	else
	{
		m_CombDownFace.AddString(g_listDownFace.front());
		m_CombDownFace.SetCurSel(0);
	}

	double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	double showLen = m_BeamLen / uor_per_mm;
	CString strLen;
	strLen.Format(L"%.2f", showLen);
	GetDlgItem(IDC_EDIT1)->SetWindowText(strLen);
	// TODO:  在此添加额外的初始化

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CHiddenBeamSetDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	//删除面
	if (m_pElmDownFace != nullptr)
	{
		EditElementHandle eh(m_pElmDownFace, ACTIVEMODEL);
		eh.DeleteFromModel();
		m_pElmDownFace = nullptr;
	}
	if (m_bEmbededCloumn)
	{
		mdlInput_sendSynchronizedKeyin(L"proconcrete add rebar column", 0, INPUTQ_EOQ, NULL);
	}
	else
	{
		mdlInput_sendSynchronizedKeyin(L"proconcrete add rebar beam", 0, INPUTQ_EOQ, NULL);
	}
}


void CHiddenBeamSetDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_pElmDownFace != nullptr || m_elmRef != nullptr)
	{
		if (IDCANCEL == MessageBox(TEXT("关闭将删除之前绘制的所有实体与钢筋，是否继续关闭！"), TEXT("提示"), MB_OKCANCEL))
		{
			return;
		}

		//删除面
		if (m_pElmDownFace != nullptr)
		{
			EditElementHandle eh(m_pElmDownFace, ACTIVEMODEL);
			eh.DeleteFromModel();
			m_pElmDownFace = nullptr;
		}

		//删除体和钢筋
		if (m_elmRef != nullptr)
		{
			EditElementHandle eeh(m_elmRef, ACTIVEMODEL);
			eeh.DeleteFromModel();
			m_elmRef = nullptr;
		}
	}
	CDialogEx::OnCancel();
}

//绘制梁底面
void CHiddenBeamSetDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	int type = 0;
	if (m_bEmbededCloumn)
	{
		m_CombDownFace.GetCurSel();
	}
	switch (type)
	{
	case 0:
		mdlInput_sendSynchronizedKeyin(L"place block icon", 0, INPUTQ_EOQ, NULL);
		break;
	case 1:
		mdlInput_sendSynchronizedKeyin(L"place circle icon", 0, INPUTQ_EOQ, NULL);
		break;
	default:
		break;
	}
	bDownFaceDrawn = true;
}


void CHiddenBeamSetDlg::OnEnChangeEdit1()
{
	// TODO:  在此添加控件通知处理程序代码
	CString strLen;
	GetDlgItem(IDC_EDIT1)->GetWindowTextW(strLen);
	if (strLen != "")
	{
		double uor_per_mm = ACTIVEMODEL->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
		m_BeamLen = atof(CT2A(strLen)) * uor_per_mm;
	}
	DrawBeam(m_Check.GetCheck());

}

//反向
void CHiddenBeamSetDlg::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	DrawBeam(m_Check.GetCheck());
}

//选择自定义底面
void CHiddenBeamSetDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
}

//预览
void CHiddenBeamSetDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	//获取最后一次绘制的元素
	if (!bDownFaceDrawn)
	{
		MessageBox(TEXT("请先绘制底面！"), TEXT("提示"), MB_OK);
		return;
	}

	auto pos = mdlModelRef_getEof(ACTIVEMODEL);
	ElementRefP elRef = mdlModelRef_getElementRef(ACTIVEMODEL, pos - 1);
	EditElementHandle eehFace(elRef, ACTIVEMODEL);
	if (eehFace.GetElementType() != SHAPE_ELM && eehFace.GetElementType() != ELLIPSE_ELM)
	{
		if (m_bEmbededCloumn)
		{
			MessageBox(TEXT("请先绘制正确的柱底面！"), TEXT("提示"), MB_OK);
		}
		else
		{
			MessageBox(TEXT("请先绘制正确的梁底面！"), TEXT("提示"), MB_OK);

		}
		return;
	}

	m_pElmDownFace = elRef;
	DrawBeam(m_Check.GetCheck());
}


void CHiddenBeamSetDlg::OnBnClickedCheck5()
{
	// TODO: 在此添加控件通知处理程序代码

	vector<ElementId> vecElmId;
	int id = m_bEmbededCloumn ? HideCloumnElmID : HideBeamElmID;
	GetElementXAttribute(m_ehSel.GetElementId(), vecElmId, id, m_ehSel.GetModelRef());
	if (m_elmRef != nullptr)
	{
		if (std::find(vecElmId.begin(), vecElmId.end(), m_elmRef->GetElementId()) == vecElmId.end())
		{
			vecElmId.push_back(m_elmRef->GetElementId());
			SetElementXAttribute(m_ehSel.GetElementId(), vecElmId, id, m_ehSel.GetModelRef());
		}
	}

	if (((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck())
	{
		for (size_t i = 0; i < vecElmId.size(); ++i)
		{
			EditElementHandle eeh(vecElmId[i], ACTIVEMODEL);
			if (eeh.IsValid())
			{
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), false);
				eeh.ReplaceInModel(oldRef);
			}
		}
	}
	else	
	{ 
		for (size_t i = 0; i < vecElmId.size(); ++i)
		{
			EditElementHandle eeh(vecElmId[i], ACTIVEMODEL);
			if (eeh.IsValid())
			{
				ElementRefP oldRef = eeh.GetElementRef();
				mdlElmdscr_setVisible(eeh.GetElementDescrP(), true);
				eeh.ReplaceInModel(oldRef);
			}
		}
	}
}


void CHiddenBeamSetDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_pElmDownFace != nullptr || m_elmRef != nullptr)
	{
		if (IDCANCEL == MessageBox(TEXT("关闭将删除之前绘制的所有实体与钢筋，是否继续关闭！"), TEXT("提示"), MB_OKCANCEL))
		{
			return;
		}

		//删除面
		if (m_pElmDownFace != nullptr)
		{
			EditElementHandle eh(m_pElmDownFace, ACTIVEMODEL);
			eh.DeleteFromModel();
			m_pElmDownFace = nullptr;
		}

		//删除体和钢筋
		if (m_elmRef != nullptr)
		{
			EditElementHandle eeh(m_elmRef, ACTIVEMODEL);
			eeh.DeleteFromModel();
			m_elmRef = nullptr;
		}
	}

	CDialogEx::OnClose();
}

void CHiddenBeamSetDlg::OnBnClickedButton12()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_elmRef != nullptr)
	{
		vector<ElementId> vecElmId;
		int id = m_bEmbededCloumn ? HideCloumnElmID : HideBeamElmID;
		GetElementXAttribute(m_ehSel.GetElementId(), vecElmId, id, m_ehSel.GetModelRef());

		if (((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == 1) // 暗梁、暗柱只有在隐藏的时候才保存上一次的配筋体
		{
			if (std::find(vecElmId.begin(), vecElmId.end(), m_elmRef->GetElementId()) == vecElmId.end())
			{
				vecElmId.push_back(m_elmRef->GetElementId());
				SetElementXAttribute(m_ehSel.GetElementId(), vecElmId, id, m_ehSel.GetModelRef());
			}
		}
		else
		{
			//删除面
			if (m_pElmDownFace != nullptr)
			{
				EditElementHandle eh(m_pElmDownFace, ACTIVEMODEL);
				eh.DeleteFromModel();
				m_pElmDownFace = nullptr;
			}

			//删除体和钢筋 
			if (m_elmRef != nullptr)
			{
				EditElementHandle eeh(m_elmRef, ACTIVEMODEL);
				eeh.DeleteFromModel();
				m_elmRef = nullptr;
			}
		}
		m_elmRef = nullptr;
	}

	CDialogEx::OnOK();
}
