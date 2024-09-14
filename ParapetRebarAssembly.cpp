#include "_ustation.h"
#include <SelectionRebar.h>
#include "resource.h"
#include "GalleryIntelligentRebar.h"
#include "ParapetRebarAssembly.h"
#include "ElementAttribute.h"
#include "ExtractFacesTool.h"
#include "PITMSCECommon.h"
#include "CommonFile.h"
#include "CParapetDlg.h"
using namespace PIT;
void ParapetRebarAssembly::InitRebarParam(double ulenth)
{
	if (ulenth <=0)
	{
		ulenth = 240;
	}
	double uor_per_mm = GetModelRef()->GetModelInfoCP()->GetUorPerMeter() / 1000.0;
	int iRebarLevelNum = GetRebarLevelNum();
	if (GetRebarLevelNum()!=4&& GetvecDirSize().size()!=4&&GetvecDir().size()!=4)
	{
		return;
	}
	double radius1, radius2, radius3, radius4;
	radius1 = RebarCode::GetBarDiameter(GetvecDirSize().at(0), ACTIVEMODEL) / 2;
	radius2 = RebarCode::GetBarDiameter(GetvecDirSize().at(1), ACTIVEMODEL) / 2;
	radius3 = RebarCode::GetBarDiameter(GetvecDirSize().at(2), ACTIVEMODEL) / 2;
	radius4 = RebarCode::GetBarDiameter(GetvecDirSize().at(3), ACTIVEMODEL) / 2;
	double dSideCover = GetSideCover()*uor_per_mm;
	double dpositiveCover = GetPositiveCover()*uor_per_mm;
	double dReverseCover = GetReverseCover()*uor_per_mm;
	double bendRadius;
	if (GetvecDir().at(0)==1&&GetvecDir().at(3)==1)//第一层和第四层为点筋
	{
		bendRadius = (PopSTwallData().width - dpositiveCover - dReverseCover - radius1 - radius4) / 2;
	}
	else if (GetvecDir().at(1) == 1 && GetvecDir().at(2) == 1)
	{
		bendRadius = (PopSTwallData().width - dpositiveCover - dReverseCover - 
			radius1*2 - radius4*2 - radius2 - radius3) / 2;
	}
	else
	{
		return;
	}
	PopSTwallData().height = PopSTwallData().height - bendRadius;
	for (int i = 0; i < iRebarLevelNum; ++i)
	{
		vector<PIT::EndType> vecEndType;
		if (GetvvecEndType().size()+1<i)		
		{
			PIT::EndType endType;
			memset(&endType, 0, sizeof(PIT::EndType));
			vecEndType = { { 0,0,0 },{0,0,0} ,endType };
			PopvvecEndType().push_back(vecEndType);
		}
		if (GetvecDir().at(i) == 1)
		{
			if (PopvvecEndType().at(i).size()>1)
			{
				PopvvecEndType().at(i).at(0).endType = 0;
				PopvvecEndType().at(i).at(0).offset = 0;
				PopvvecEndType().at(i).at(0).rotateAngle = 0;
				PopvvecEndType().at(i).at(0).endPtInfo.value1 = 0;
				PopvvecEndType().at(i).at(0).endPtInfo.value3 = 0;

				PopvvecEndType().at(i).at(1).endType = 6;
				PopvvecEndType().at(i).at(1).offset = -bendRadius / uor_per_mm;
				PopvvecEndType().at(i).at(1).rotateAngle = 90;
				PopvvecEndType().at(i).at(1).endPtInfo.value1 = bendRadius;
				PopvvecEndType().at(i).at(1).endPtInfo.value3 = ulenth * uor_per_mm;
			}
			
		}
		else
		{
			if (PopvvecEndType().at(i).size() > 1)
			{
				PopvvecEndType().at(i).at(0).endType = 0;
				PopvvecEndType().at(i).at(0).offset = 0;
				PopvvecEndType().at(i).at(0).rotateAngle = 0;
				PopvvecEndType().at(i).at(0).endPtInfo.value1 = 0;
				PopvvecEndType().at(i).at(0).endPtInfo.value3 = 0;

				PopvvecEndType().at(i).at(1).endType = 0;
				PopvvecEndType().at(i).at(1).offset = 0;
				PopvvecEndType().at(i).at(1).rotateAngle = 0;
				PopvvecEndType().at(i).at(1).endPtInfo.value1 = 0;
				PopvvecEndType().at(i).at(1).endPtInfo.value3 = 0;

			}
		}


	}
}
bool ParapetRebarAssembly::OnDoubleClick()
{
	EditElementHandle ehSel;
	if (!PIT::GetAssemblySelectElement(ehSel, this))
	{
		return false;
	}
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	pParapetDoubleRebarDlg = new CParapetDlg(CWnd::FromHandle(MSWIND));
	pParapetDoubleRebarDlg->SetSelectElement(ehSel);
	pParapetDoubleRebarDlg->Create(IDD_DIALOG_Parapet);
	pParapetDoubleRebarDlg->SetConcreteId(FetchConcrete());
	pParapetDoubleRebarDlg->ShowWindow(SW_SHOW);

// 	CParapetDlg dlg(CWnd::FromHandle(MSWIND));
// 	dlg.SetSelectElement(ehSel);
// 	//	dlg.SetSelectElement(ehSel);
// 	dlg.SetConcreteId(FetchConcrete());
// 	if (IDCANCEL == dlg.DoModal())
// 		return false;
	return true;
}

long ParapetRebarAssembly::GetStreamMap(BeStreamMap &map, int typeof /* = 0 */, int versionof /* = -1 */)
{
	switch (typeof)
	{
	case 0:
		return RebarExtendedElement::GetStreamMap(map, typeof, versionof);
	case 1:
		return RebarAssembly::GetStreamMap(map, typeof, versionof);
	case 2:
	{
		return 0;
	}
	default:
		break;
	}
	return -1;
}