#pragma once

#include "../il2cpp/CClass.hpp"

namespace mono
{
    class CType
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static CType* FromName(const char* pszName);
    };
} // mono
