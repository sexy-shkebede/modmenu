#include "CMaterial.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CMaterial::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Material", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CMaterial::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Material, UnityEngine.CoreModule.dll");
    }

    CMaterial* CMaterial::Create(const char* pszName, CShader* pShader)
    {
        RESOLVE_SDK_METHOD(void (*)( CMaterial*, CShader* ), GetIl2cppClass(), "CreateWithShader", 2);
        CMaterial* out = (CMaterial*)il2cpp::CObject::Allocate(CMaterial::GetIl2cppClass());
        func(out, pShader);
        out->SetName(pszName);
        return out;
    }

    CMaterial* CMaterial::Create(const char* pszName, CMaterial* pMaterial)
    {
        RESOLVE_SDK_METHOD(void (*)( CMaterial*, CMaterial* ), GetIl2cppClass(), "CreateWithMaterial", 2);
        CMaterial* out = (CMaterial*)il2cpp::CObject::Allocate(CMaterial::GetIl2cppClass());
        func(out, pMaterial);
        out->SetName(pszName);
        return out;
    }

    CShader* CMaterial::GetShader() 
    {
        RESOLVE_SDK_METHOD(CShader* (*)( const CMaterial* ), this->GetIl2cppClass(), "get_shader", 0);
        return func(this);
    }

    void CMaterial::SetShader(CShader* pShader) 
    {
        RESOLVE_SDK_METHOD(void (*)( CMaterial*, CShader* ), this->GetIl2cppClass(), "set_shader", 1);
        func(this, pShader);
    }

    CTexture* CMaterial::GetMainTexture()
    {
        RESOLVE_SDK_METHOD(CTexture* (*)( CMaterial* ), this->GetIl2cppClass(), "get_mainTexture", 0);
        return func(this);
    }

    void CMaterial::SetMainTexture(CTexture* pTexture)
    {
        RESOLVE_SDK_METHOD(void (*)( CMaterial*, CTexture* ), this->GetIl2cppClass(), "set_mainTexture", 1);
        func(this, pTexture);
    }

    Color CMaterial::GetColor(const char* pszProperty)
    {
        RESOLVE_SDK_METHOD(Color (*)( CMaterial*, mono::SString* ), this->GetIl2cppClass(), "GetColor", 1);
        return func(this, mono::SString::Create(pszProperty));
    }

    void CMaterial::SetColor(Color color, const char* pszProperty)
    {
        RESOLVE_SDK_METHOD(void (*)( CMaterial*, mono::SString*, Color ), this->GetIl2cppClass(), "SetColor", 2);
        func(this, mono::SString::Create(pszProperty), color);
    }

    float CMaterial::GetFloat(const char* pszProperty)
    {
        RESOLVE_SDK_METHOD(float (*)( const CMaterial*, mono::SString* ), this->GetIl2cppClass(), "GetFloat", 1);
        return func(this, mono::SString::Create(pszProperty));
    }

    void CMaterial::SetFloat(const char* pszProperty, float fValue)
    {
        RESOLVE_SDK_METHOD(void (*)( CMaterial*, mono::SString*, float ), this->GetIl2cppClass(), "SetFloat", 2);
        func(this, mono::SString::Create(pszProperty), fValue);
    }

    CTexture* CMaterial::GetTexture(const char* pszTexture)
    {
        RESOLVE_SDK_METHOD(CTexture* (*)( CMaterial*, mono::SString* ), this->GetIl2cppClass(), "GetTexture", 1);
        return func(this, mono::SString::Create(pszTexture));
    }

    void CMaterial::SetTexture(CTexture* pTexture, const char* pszTexture)
    {
        RESOLVE_SDK_METHOD(void (*)( CMaterial*, mono::SString*, CTexture* ), this->GetIl2cppClass(), "SetTexture", 2);
        func(this, mono::SString::Create(pszTexture), pTexture);
    }
} // usdk
