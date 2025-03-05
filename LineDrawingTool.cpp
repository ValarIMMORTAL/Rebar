#include "LineDrawingTool.h"

LineDrawingTool::LineDrawingTool(int toolId, int promptId, CFacesRebarEndTypeDlg* pDlg)
	: DgnPrimitiveTool(toolId, promptId), m_pDlg(pDlg)
{
}

void LineDrawingTool::_OnPostInstall()
{
	AccuSnap::GetInstance().EnableSnap(true); // 启用捕捉功能
	__super::_OnPostInstall();
}

void LineDrawingTool::_OnRestartTool()
{
	LineDrawingTool* pTool = new LineDrawingTool(GetToolId(), GetToolPrompt(), m_pDlg);
	pTool->InstallTool();
}

void LineDrawingTool::_OnDynamicFrame(DgnButtonEventCR ev)
{
	std::vector<DPoint3d> tmpPts;
	tmpPts.push_back(m_points.back());

	EditElementHandle eeh;

	tmpPts.push_back(*ev.GetPoint()); // 使用当前按钮位置作为终点

	if (!CreateElement(eeh, tmpPts))
		return;

	RedrawElems redrawElems;
	redrawElems.SetDynamicsViews(IViewManager::GetActiveViewSet(), ev.GetViewport());
	redrawElems.SetDrawMode(DRAW_MODE_TempDraw);
	redrawElems.SetDrawPurpose(DrawPurpose::Dynamics);

	redrawElems.DoRedraw(eeh);
}

void LineDrawingTool::SetupAndPromptForNextAction()
{
	if (m_points.size() != 2)
		return;

	DVec3d xVec = DVec3d::FromStartEndNormalize(m_points.front(), m_points.back());
	AccuDraw::GetInstance().SetContext(ACCUDRAW_SetXAxis, NULL, &xVec); // 按照最后一段方向调整 AccuDraw
}

bool LineDrawingTool::CreateElement(EditElementHandleR eeh, const std::vector<DPoint3d>& points)
{
	if (points.size() != 2)
		return false;

	// 使用 CurveVector/CurvePrimitive 创建线，兼容性更好
	if (SUCCESS != DraftingElementSchema::ToElement(eeh, *ICurvePrimitive::CreateLine(DSegment3d::From(points.front(), points.back())), NULL, ACTIVEMODEL->Is3d(), *ACTIVEMODEL))
		return false;

	ElementPropertyUtils::ApplyActiveSettings(eeh);
	return true;
}

bool LineDrawingTool::_OnDataButton(DgnButtonEventCR ev)
{
	if (m_points.empty())
		_BeginDynamics(); // 第一个点启动动态绘制

	m_points.push_back(*ev.GetPoint()); // 保存当前点
	SetupAndPromptForNextAction();

	if (m_points.size() == 2 && CreateElement(m_eehLine, m_points))
	{
		m_eehLine.AddToModel();
	}

	if (m_points.size() < 3)
		return false;

	if (m_eehLine.GetElementId() != 0)
	{
		m_eehLine.DeleteFromModel(); // 删除临时线
	}

	double length = CalculateLength(m_points[0], m_points[1]) / UOR_PER_MilliMeter; // 转换为毫米
	if (m_pDlg)
	{
		m_pDlg->SetOffsetValue(m_pDlg->GetCurrentRow(), length); // 更新偏移值
	}

	_ExitTool(); // 退出工具
	return true;
}

bool LineDrawingTool::_OnResetButton(DgnButtonEventCR ev)
{
	_OnRestartTool();
	return true;
}

double LineDrawingTool::CalculateLength(const DPoint3d& start, const DPoint3d& end) const
{
	return start.Distance(end); // 使用 Bentley 的 Distance 方法计算距离
}