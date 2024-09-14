#pragma once

#include <afxwin.h>
#include "resource.h"
#include "GallerySettingsDialog.h"

namespace Gallery
{
	/// ÀÈµÀÅä½îÉèÖÃ
	void cmd_gallery_settings(WCharCP);

	/// Ö§¶ÕÅä½îÃüÁî
	void cmd_gallery_buttress(WCharCP);

	///ÑØ±ß¼ÓÇ¿½î
	void cmd_Edge_settings(WCharCP);
	
	void TieRebarFaceTools(WCharCP unparesed);

	///Â¥ÌİÅä½î
	void StairsRebarSetting(WCharCP unparsed);

	// ¿×¶´¼ÓÇ¿½î
	void HoleReinForcingRebarSetting(WCharCP unparsed);

	//¼¯Ë®¿ÓÅä½î
	void cmd_gallery_Catchpit(WCharCP unparsed);

}