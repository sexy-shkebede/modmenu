#pragma once

#include <cstdint>

namespace il2cpp
{
    class CMethod;

    struct virtual_invoke_data_t
    {
        CMethod* pMethod;
        uint64_t ptr;

        inline void hook(const uint64_t hk, uint64_t* origin)
        {
            if(origin != NULL)
            {
                if(*origin == 0)
                {
                    *origin = ptr;
                }
            }
            ptr = hk;
        }
    };
} // il2cpp
