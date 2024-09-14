#pragma once

#include "CommonFile.h"
#include <vector>

namespace Gallery
{
    class SlabHelper
    {
    public:
        /// @brief 判断一个元素是否是板
        /// @param element
        /// @return 
        static bool is_slab(const ElementHandle &element);
    };
} // namespace Gallery


