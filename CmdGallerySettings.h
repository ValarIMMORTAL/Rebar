#pragma once

#include <afxwin.h>
#include "resource.h"
#include "GallerySettingsDialog.h"

namespace Gallery
{
	/// �ȵ��������
	void cmd_gallery_settings(WCharCP);

	/// ֧���������
	void cmd_gallery_buttress(WCharCP);

	///�ر߼�ǿ��
	void cmd_Edge_settings(WCharCP);
	
	void TieRebarFaceTools(WCharCP unparesed);

	///¥�����
	void StairsRebarSetting(WCharCP unparsed);

	// �׶���ǿ��
	void HoleReinForcingRebarSetting(WCharCP unparsed);

	//��ˮ�����
	void cmd_gallery_Catchpit(WCharCP unparsed);

}