#pragma once

#include "CommonFile.h"
#include <vector>

namespace Gallery
{
    class SlabHelper
    {
    public:
        /// @brief �ж�һ��Ԫ���Ƿ��ǰ�
        /// @param element
        /// @return 
        static bool is_slab(const ElementHandle &element);
    };
} // namespace Gallery


