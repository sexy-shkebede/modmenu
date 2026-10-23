#pragma once

#include "CMethod.hpp"
#include "../sdk_il2cpp_offsets.hpp"
namespace il2cpp
{
    class CImage;
    class CMethod;
    class CField;
    struct virtual_invoke_data_t;

    class CClass
    {
    public:
        inline CImage* GetImage() { return *(CImage**)((uintptr_t)this + SDK_IL2CPP_OFFSET_CLASS_IMAGE); }
        inline const char* GetName() { return *(const char**)((uintptr_t)this + SDK_IL2CPP_OFFSET_CLASS_NAME); }
        inline const char* GetNamespace() { return *(const char**)((uintptr_t)this + SDK_IL2CPP_OFFSET_CLASS_NAMESPACE); }
        inline CClass* GetParent() { return *(CClass**)((uintptr_t)this + SDK_IL2CPP_OFFSET_CLASS_PARENT); }

        static CClass* FromName(const char* pszClassName, const char* pszNamespace, const char* pszAssembly);
        CMethod*       GetMethods(void** iter);
        CMethod*       GetMethodFromName(const char* pszName, uint32_t nArgCount, uint8_t nSkip = 0);
        CMethod*       GetMethodContainsName(const char* pszName, uint32_t nArgCount, uint8_t nSkip = 0);
        CField*        GetFields(void** iter);
        CField*        GetFieldFromName(const char* pszName);
    };
} // il2cpp
