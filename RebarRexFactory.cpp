#include "_ustation.h"
#include "RebarRexFactory.h"
#include "GeneralFaceRebarAssembly.h"
#include "LDSlabRebarAssembly.h"
#include "WallRebarAssembly.h"
#include "HoleRebarAssembly.h"
#include "BaseRebarAssembly.h"
RebarExtendedElementP RebarRexFactory::Make(BrStringCR rex_name) const
{
    if(rex_name == Gallery::GeneralFaceRebarAssembly::GetVClassName())
    {
        return new Gallery::GeneralFaceRebarAssembly(0, nullptr);
    }
	if (rex_name == Gallery::LDSlabRebarAssembly::GetVClassName())
	{
		return new Gallery::LDSlabRebarAssembly(0, nullptr);
	}
	if (rex_name == STWallRebarAssembly::GetVClassName())
	{
		return new STWallRebarAssembly(0, nullptr);
	}
	if (rex_name == HoleRFRebarAssembly::GetVClassName())
	{
		return new HoleRFRebarAssembly(0, nullptr);
	}
	if (rex_name == AbanurusRebarAssembly::GetVClassName())
	{
		return new AbanurusRebarAssembly(0, nullptr);
	}
    return nullptr;
}
