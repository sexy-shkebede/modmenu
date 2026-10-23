#pragma once

#include "CObject.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CShader : public CObject 
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        static CShader* Find(const char* pszName);
        static int PropertyToID(const char* pszProperty);
    };
} // usdk
