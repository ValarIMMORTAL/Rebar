#pragma once

#include <afxwin.h>
#include "resource.h"
#include "GallerySettingsDialog.h"
#include "CmdGallerySettings.h"
#include "BaseRebarDlg.h"
#include "CEdgeLineRebarDlg.h"
#include "ScanIntersectTool.h"
#include "TieRebarFaceDlg.h"
#include "CStarisRebarDlog.h"
#include "CHoleRebar_ReinforcingDlg.h"
#include "WallRebarAssembly.h"
#include "CatchpitRebarDlg.h"

namespace Gallery
{
	/// �ȵ��������
	void cmd_gallery_settings(WCharCP)
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		auto dialog = new SettingsDialog(CWnd::FromHandle(MSWIND));
		dialog->Create(IDD_DIALOG_Gallery_Settings);
		dialog->ShowWindow(SW_SHOW);
	}

	// ֧���������
	void cmd_gallery_buttress(WCharCP)
	{
		BaseRebarDlg * p_BaseRebarPtr = NULL;
		ElementAgenda selectedElement;
		SelectionSetManager::GetManager().BuildAgenda(selectedElement);
		if (selectedElement.GetCount() < 1)
			return;

		DgnModelRefP        modelRef = ACTIVEMODEL;
		g_SelectedElm = selectedElement[0];
		EditElementHandle eeh(g_SelectedElm, g_SelectedElm.GetModelRef());
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		p_BaseRebarPtr = new BaseRebarDlg();
		p_BaseRebarPtr->m_ehSel = eeh;
		p_BaseRebarPtr->Create(IDD_DIALOG_BaseRebar);
		p_BaseRebarPtr->ShowWindow(SW_SHOW);
	}

	//�ر߼�ǿ��
	void cmd_Edge_settings(WCharCP)
	{
		CEdgeLineRebarDlg * p_EdgeRebarPtr = NULL;
		ElementAgenda selectedElement;
		if (!GetSelectAgenda(selectedElement, L"��ѡ���"))
		{
			return;
		}
		if (selectedElement.GetCount() >1)
			return;

		DgnModelRefP        modelRef = ACTIVEMODEL;
		g_SelectedElm = selectedElement[0];
		string Ename, Ename1; string Etype;
		GetEleNameAndType(selectedElement[0].GetElementId(), selectedElement[0].GetModelRef(), Ename, Etype);
		if (Etype != "FLOOR")
			return;
		EditElementHandle eeh(g_SelectedElm, g_SelectedElm.GetModelRef());
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		p_EdgeRebarPtr = new CEdgeLineRebarDlg();
		p_EdgeRebarPtr->SetSelectElement(eeh);
		p_EdgeRebarPtr->Create(IDD_DIALOG_EdgeRebar);
		p_EdgeRebarPtr->ShowWindow(SW_SHOW);
	}

	TieRebarFaceDlg* pTieRebarFaceDlg = NULL;
	void TieRebarFaceTools(WCharCP unparesed)
	{
		ElementAgenda selectedElement;
		SelectionSetManager::GetManager().BuildAgenda(selectedElement);
		if (selectedElement.GetCount() > 0)
		{
			AFX_MANAGE_STATE(AfxGetStaticModuleState());
			pTieRebarFaceDlg = new TieRebarFaceDlg(CWnd::FromHandle(MSWIND));
			pTieRebarFaceDlg->m_selectrebars.clear();
			EditElementHandleP elementToModify = selectedElement.GetFirstP();
			for (; elementToModify <= selectedElement.GetLast(); elementToModify++)
			{
				if (RebarElement::IsRebarElement(*elementToModify))
				{
					pTieRebarFaceDlg->m_selectrebars.push_back(elementToModify->GetElementRef());
				}
			}
			if (pTieRebarFaceDlg->m_selectrebars.size() == 0)
			{
				return;
			}
			mdlSelect_freeAll();
			pTieRebarFaceDlg->Create(IDD_DIALOG_TieRebarFace1);
			pTieRebarFaceDlg->ShowWindow(SW_SHOW);
		}
	}
	//¥�����
	void StairsRebarSetting(WCharCP unparsed)
	{
		CStarisRebarDlog* pStairsRebar = NULL;
		ElementAgenda selectedElement;
		if (!GetSelectAgenda(selectedElement, L"��ѡ��¥��"))
		{
			return;
		}

		if (selectedElement.GetCount() < 1)
		{
			//���ܶ�ѡ��ֻ�ܴ���һ��ǽ
			return;
		}

		DgnModelRefP        modelRef = ACTIVEMODEL;
		g_SelectedElm = selectedElement[0];
		EditElementHandle eeh(selectedElement[0], selectedElement[0].GetModelRef());

		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		pStairsRebar = new CStarisRebarDlog(g_SelectedElm, CWnd::FromHandle(MSWIND));
		pStairsRebar->Create(IDD_DIALOG_Stairs);
		pStairsRebar->ShowWindow(SW_SHOW);
	}

	//�ϲ����(һС���������ǽ����)
	void CombineRebarSetting(WCharCP unparsed)
	{
		ElementAgenda selectedElement;
		SelectionSetManager::GetManager().BuildAgenda(selectedElement);
		if (selectedElement.GetCount() > 0)
		{
			AFX_MANAGE_STATE(AfxGetStaticModuleState());
		}
	}



	// �׶���ǿ��
	CHoleRebar_ReinforcingDlg* pHoleReinForcingDlg = NULL;
	void HoleReinForcingRebarSetting(WCharCP unparsed)
	{
		ElementAgenda selectedElement;
		SelectionSetManager::GetManager().BuildAgenda(selectedElement);
		if (selectedElement.GetCount() > 0)
		{
			//if (selectedElement.GetCount() > 1)
			//{
			//	//���ܶ�ѡ��ֻ�ܴ�������ǽ
			//	return;
			//}
			g_SelectedElm = selectedElement[0];
			EditElementHandle eeh(g_SelectedElm, g_SelectedElm.GetModelRef());
			if (RebarElement::IsRebarElement(eeh))
			{
				return;
			}
			ChangeSourceeeh(eeh);
			g_SelectedElm = eeh;

			WallRebarAssembly::ElementType eleType = WallRebarAssembly::JudgeElementType(eeh);
			bool isarcwall = false;
			bool isfloor = false;
			pHoleReinForcingDlg = new CHoleRebar_ReinforcingDlg;
			if (WallRebarAssembly::JudgeElementType(eeh) != WallRebarAssembly::FLOOR)
			{
				WallRebarAssembly::WallType wallType = WallRebarAssembly::JudgeWallType(eeh);
				if (wallType == WallRebarAssembly::ARCWALL || wallType == WallRebarAssembly::ELLIPSEWall)
				{
					isarcwall = true;
				}
			}
			else
			{
				isfloor = true;
			}
			AFX_MANAGE_STATE(AfxGetStaticModuleState());
			pHoleReinForcingDlg->isArcwall = isarcwall;
			pHoleReinForcingDlg->isFloor = isfloor;
			pHoleReinForcingDlg->SetSelectElement(g_SelectedElm);
			pHoleReinForcingDlg->Create(IDD_DIALOG_HoleRebar_Reinforcing);
			//		dlg.m_PageAssociatedComponent.SetListRowData(vecACData);
			pHoleReinForcingDlg->ShowWindow(SW_SHOW);
		}
		else
		{
			//����ѡ��һ��ǽ
			mdlDialog_dmsgsPrint(L"��ѡ��һ��ǽ");
		}
	}


	//��ˮ�����
	CatchpitRebarDlg * pCatchpitDlg = NULL;
	void cmd_gallery_Catchpit(WCharCP unparsed)
	{
		ElementAgenda selectedElement;
		SelectionSetManager::GetManager().BuildAgenda(selectedElement);
		if (selectedElement.GetCount() > 0)
		{
			g_SelectedElm = selectedElement[0];
			EditElementHandle eeh(selectedElement[0], selectedElement[0].GetModelRef());

			AFX_MANAGE_STATE(AfxGetStaticModuleState());
			pCatchpitDlg = new CatchpitRebarDlg(g_SelectedElm, CWnd::FromHandle(MSWIND));
			pCatchpitDlg->Create(IDD_DIALOG_CatchpitRebar);
			pCatchpitDlg->ShowWindow(SW_SHOW);

		}
		else
		{
			//����ѡ��һ��ǽ
			mdlDialog_dmsgsPrint(L"��ѡ��һ����ˮ��");
		}
	}

	//��Բ�ⷽ���
	void cmd_CircleAndSquare_Settings(WCharCP unparsed)
	{
		
	}

}