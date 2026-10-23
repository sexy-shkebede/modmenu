#pragma once

#include "CObject.hpp"
#include <cstdint>

namespace il2cpp
{
    //typedef struct Il2CppArrayBounds Il2CppArrayBounds;
    //typedef uintptr_t il2cpp_array_size_t;
    struct CArray
    {
    public:
        CObject   m_Obj;
        void*     m_pBounds;        // Il2CppArrayBounds*
        uintptr_t m_uMaxLength;     // il2cpp_array_size_t
        void*     m_Items[0];
    };
} // il2cpp
