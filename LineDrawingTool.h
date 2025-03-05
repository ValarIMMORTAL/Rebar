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

	// ���߰�װ��ĳ�ʼ��
	virtual void _OnPostInstall() override;

	// ��������
	virtual void _OnRestartTool() override;

	// ��̬����֡��������꣩
	virtual void _OnDynamicFrame(DgnButtonEventCR ev) override;

	// ���ݰ�ť�����ȷ���㣩
	virtual bool _OnDataButton(DgnButtonEventCR ev) override;

	// ���ð�ť���
	virtual bool _OnResetButton(DgnButtonEventCR ev) override;

private:
	std::vector<DPoint3d> m_points;         // �洢���Ƶ���б�
	EditElementHandle m_eehLine;            // ��ʱ��Ԫ�ؾ��
	CFacesRebarEndTypeDlg* m_pDlg;          // ָ��Ի����ָ�룬���ڻص�

	// ���ò���ʾ��һ������
	void SetupAndPromptForNextAction();

	// ͨ���㴴����Ԫ��
	bool CreateElement(EditElementHandleR eeh, const std::vector<DPoint3d>& points);

	// �����������루��λ�����ף�
	double CalculateLength(const DPoint3d& start, const DPoint3d& end) const;
};

#endif // LINEDRAWINGTOOL_H