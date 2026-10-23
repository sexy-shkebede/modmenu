#pragma once
#include <cstdint>
#include "../sdk_il2cpp_offsets.hpp"

#include "CAssembly.hpp"
#include "CClass.hpp"

namespace il2cpp
{
    class CImage
    {
    public:
        inline const char* GetName() { return *(const char**)((uintptr_t)this + SDK_IL2CPP_OFFSET_IMAGE_NAME); }
        inline const char* GetNameNoExt() { return *(const char**)((uintptr_t)this + SDK_IL2CPP_OFFSET_IMAGE_NAME_NO_EXT); }
        inline CAssembly* GetAssembly() { return *(CAssembly**)((uintptr_t)this + SDK_IL2CPP_OFFSET_IMAGE_ASSEMBLY); }
        inline uint32_t GetTypeCount() { return *(uint32_t*)((uintptr_t)this + SDK_IL2CPP_OFFSET_IMAGE_TYPE_COUNT); }

        CClass* GetClass(uint32_t nIndex);
    };
} // il2cpp
