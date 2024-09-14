#include "_ustation.h"
#include "SlabHelper.h"
#include "ScanIntersectTool.h"

namespace Gallery
{
    bool SlabHelper::is_slab(const ElementHandle &element)
    {
        std::string _name, type;
        if (!GetEleNameAndType(const_cast<ElementHandleR>(element), _name, type))
        {
            return false;
        }
        auto result_pos = type.find("FLOOR");
        return result_pos != std::string::npos;
    }
}
