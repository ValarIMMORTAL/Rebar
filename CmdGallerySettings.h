#pragma once

#include <afxwin.h>
#include "resource.h"
#include "GallerySettingsDialog.h"

namespace Gallery
{
	/// 廊道配筋设置
	void cmd_gallery_settings(WCharCP);

	/// 支墩配筋命令
	void cmd_gallery_buttress(WCharCP);

	///沿边加强筋
	void cmd_Edge_settings(WCharCP);
	
	void TieRebarFaceTools(WCharCP unparesed);

	///楼梯配筋
	void StairsRebarSetting(WCharCP unparsed);

	// 孔洞加强筋
	void HoleReinForcingRebarSetting(WCharCP unparsed);

	//集水坑配筋
	void cmd_gallery_Catchpit(WCharCP unparsed);

	//内圆外方配筋
	void cmd_CircleAndSquare_Settings(WCharCP unparsed);

}