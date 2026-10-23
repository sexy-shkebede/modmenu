#include "CShader.hpp"
#include "../il2cpp/CClass.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CShader::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Shader", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CShader::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Shader, UnityEngine.CoreModule.dll");
    }

    CShader* CShader::Find(const char* pszName)
    {
        RESOLVE_SDK_METHOD(CShader* (*)( mono::SString* ), GetIl2cppClass(), "Find", 1);
        return func(mono::SString::Create(pszName));
    }

    int CShader::PropertyToID(const char* pszProperty)
    {
        RESOLVE_SDK_METHOD(int (*)( mono::SString* ), GetIl2cppClass(), "PropertyToID", 1);
        return func(mono::SString::Create(pszProperty));
    }
} // usdk
