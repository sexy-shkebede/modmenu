#include "CType.hpp"
#include "SString.hpp"
#include "../sdk_core.hpp"

namespace mono
{
    il2cpp::CClass* CType::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Type", "System", "mscorlib.dll");
    }

    CType* CType::FromName(const char* pszName)
    {
        static CType*(*fn)(SString*,bool,bool) = (decltype(fn))(CType::GetIl2cppClass()->GetMethodFromName(OBFUSCATE_STR("GetType"), 3)->GetMethodPointer());
        return fn(SString::Create(pszName), false, false);
    }
} // mono
