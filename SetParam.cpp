#include "_USTATION.h"
#include "SetParam.h"
#include "GalleryIntelligentRebarids.h"

RebarXmlInfo g_rebarXmlInfo;
extern WString g_taskID;

void savePathResultDlgParams(void)
{
	RscFileHandle  userPrefsH;

	if (SUCCESS == mdlDialog_userPrefFileOpen(&userPrefsH, RSC_READWRITE))
	{
		/* --- delete original section parameters --- */
		mdlResource_deleteByAlias(userPrefsH, RTYPE_XML, RSCID_XML, g_taskID.data());

		/* --- add new section parameters --- */
		mdlResource_addByAlias(userPrefsH, RTYPE_XML, RSCID_XML, &g_rebarXmlInfo, sizeof(RebarXmlInfo), g_taskID.data());

		/* --- delete original section string list --- */
		//status = mdlResource_delete(userPrefsH, RTYPE_STRINGLIST, XMLNAME_LIST_ID);

		/* --- add new section string list --- */
		//status = mdlStringList_addResource(userPrefsH, XMLNAME_LIST_ID, gSectionListP);

		/* --- Close user preference file ---- */
		mdlResource_closeFile(userPrefsH);
	}
}

void loadPathResultDlgParams(void)
{
	int           status = ERROR;
	RscFileHandle userPrefsH;

	memset(&g_rebarXmlInfo, 0, sizeof(RebarXmlInfo));

	/* --- load Section and Hatching Preferences --- */
	if (SUCCESS == mdlDialog_userPrefFileOpen(&userPrefsH, RSC_READWRITE))
	{
		RebarXmlInfo *xmlInfo = NULL;

		if (NULL != (xmlInfo = (RebarXmlInfo *)mdlResource_loadByAlias(userPrefsH, RTYPE_XML, RSCID_XML, g_taskID.data())))
		{
			ULong rscSize;

			mdlResource_query(&rscSize, xmlInfo, RSC_QRY_SIZE);

			if (rscSize == sizeof(RebarXmlInfo))
			{
				status = SUCCESS;
				g_rebarXmlInfo = *xmlInfo;
			}

			/* --- Free the memory used by the rsc --- */
			mdlResource_free(xmlInfo);
		}

		mdlResource_closeFile(userPrefsH);
	}
}
