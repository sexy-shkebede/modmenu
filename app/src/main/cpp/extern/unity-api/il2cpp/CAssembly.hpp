#pragma once
#include <cstdint>
#include "../sdk_il2cpp_offsets.hpp"

namespace il2cpp
{
    class CImage;

    class CAssembly
    {
    public:
        inline CImage* GetImage() { return *(CImage**)((uintptr_t)this + SDK_IL2CPP_OFFSET_ASSEMBLY_IMAGE); }
        
        static CAssembly* FromName(const char* pszName);
    };
} // il2cpp
