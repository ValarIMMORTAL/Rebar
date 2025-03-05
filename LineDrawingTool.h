#ifndef LINEDRAWINGTOOL_H
#define LINEDRAWINGTOOL_H

#include "CFacesRebarEndTypeDlg.h"
#include <Mstn\MdlApi\MdlApi.h>
#include <DgnPlatform\DgnPlatformApi.h>
#include <DgnView\DgnElementSetTool.h>

USING_NAMESPACE_BENTLEY_DGNPLATFORM
USING_NAMESPACE_BENTLEY_MSTNPLATFORM
USING_NAMESPACE_BENTLEY_MSTNPLATFORM_ELEMENT

class LineDrawingTool : public DgnPrimitiveTool
{
public:
	LineDrawingTool(int toolId, int promptId, CFacesRebarEndTypeDlg* pDlg);
	virtual ~LineDrawingTool() override = default;

	// 工具安装后的初始化
	virtual void _OnPostInstall() override;

	// 工具重启
	virtual void _OnRestartTool() override;

	// 动态绘制帧（跟随鼠标）
	virtual void _OnDynamicFrame(DgnButtonEventCR ev) override;

	// 数据按钮点击（确定点）
	virtual bool _OnDataButton(DgnButtonEventCR ev) override;

	// 重置按钮点击
	virtual bool _OnResetButton(DgnButtonEventCR ev) override;

private:
	std::vector<DPoint3d> m_points;         // 存储绘制点的列表
	EditElementHandle m_eehLine;            // 临时线元素句柄
	CFacesRebarEndTypeDlg* m_pDlg;          // 指向对话框的指针，用于回调

	// 设置并提示下一个动作
	void SetupAndPromptForNextAction();

	// 通过点创建线元素
	bool CreateElement(EditElementHandleR eeh, const std::vector<DPoint3d>& points);

	// 计算两点间距离（单位：毫米）
	double CalculateLength(const DPoint3d& start, const DPoint3d& end) const;
};

#endif // LINEDRAWINGTOOL_H