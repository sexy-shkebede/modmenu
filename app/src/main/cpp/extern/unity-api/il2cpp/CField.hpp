#pragma once
#include <cstdint>
#include "../sdk_il2cpp_offsets.hpp"

namespace il2cpp
{
    class CClass;
    class CType;

    class CField
    {
    public:
        inline const char* GetName() { return *(const char**)((uintptr_t)this + SDK_IL2CPP_OFFSET_FIELD_NAME); }
        inline CType* GetType() { return *(CType**)((uintptr_t)this + SDK_IL2CPP_OFFSET_FIELD_TYPE); }
        inline CClass* GetParentClass() { return *(CClass**)((uintptr_t)this + SDK_IL2CPP_OFFSET_FIELD_PARENT); }
        inline uint32_t GetOffset() { return *(uint32_t*)((uintptr_t)this + SDK_IL2CPP_OFFSET_FIELD_OFFSET); }

        template <typename T>
        inline T GetValue(void* pInstance) 
        { 
            return *(T*)((uintptr_t)pInstance + GetOffset()); 
        }

        template <typename T>
        inline void SetValue(void* pInstance, T value) 
        { 
            *(T*)((uintptr_t)pInstance + GetOffset()) = value; 
        }
    };
} // il2cpp
