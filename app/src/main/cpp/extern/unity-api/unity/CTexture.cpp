#include "CTexture.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CTexture2D::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Texture2D", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CTexture2D::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Texture2D, UnityEngine.CoreModule.dll");
    }

    CTexture2D* CTexture2D::GetWhiteTexture() 
    {
        RESOLVE_SDK_METHOD(CTexture2D* (*)(  ), GetIl2cppClass(), "get_whiteTexture", 0);
        return func();
    }

    void CTexture2D::ctor(CTexture2D* pThiz, int32_t iWidth, int32_t iHeight, ETextureFormat eFormat, bool bMipChain)
    {
        RESOLVE_SDK_METHOD(void (*)( CTexture2D*, int32_t, int32_t, ETextureFormat, bool ), GetIl2cppClass(), ".ctor", 4);
        func(pThiz, iWidth, iHeight, eFormat, bMipChain);
    }

    mono::SArray<Color>* CTexture2D::GetPixels()
    {
        RESOLVE_SDK_METHOD(mono::SArray<Color>* (*)( CTexture2D* ), this->GetIl2cppClass(), "GetPixels", 0);
        return func(this);
    }

    void CTexture2D::SetPixels(mono::SArray<Color>* pixels)
    {
        RESOLVE_SDK_METHOD(void (*)( CTexture2D*, const mono::SArray<Color>* ), this->GetIl2cppClass(), "SetPixels", 1);
        func(this, pixels);
    }

    void CTexture2D::SetPixel(uint32_t x, uint32_t y, Color color)
    {
        RESOLVE_SDK_METHOD(void (*)( CTexture2D*, Color ), this->GetIl2cppClass(), "SetPixel", 3);
        func(this, color);
    }

    void CTexture2D::Apply() 
    {
        RESOLVE_SDK_METHOD(void (*)( CTexture2D* ), this->GetIl2cppClass(), "Apply", 0);
        func(this);
    }
} // usdk
