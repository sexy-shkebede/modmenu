#include "CAssetBundle.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CAssetBundle::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("AssetBundle", "UnityEngine", "UnityEngine.AssetBundleModule.dll");
    }

    mono::CType* CAssetBundle::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.AssetBundle, UnityEngine.AssetBundleModule.dll");
    }

    CAssetBundle* CAssetBundleCreateRequest::GetAssetBundle() 
    {
        RESOLVE_SDK_METHOD(CAssetBundle* (*)( CAssetBundleCreateRequest* ), il2cpp::CClass::FromName(OBFUSCATE_STR("AssetBundleCreateRequest"), OBFUSCATE_STR("UnityEngine"), OBFUSCATE_STR("UnityEngine.AssetBundleModule.dll")), "get_assetBundle", 0);
        return func(this);
    }

    CObject* CAssetBundleRequest::GetAsset()
    {
        RESOLVE_SDK_METHOD(CObject* (*)( CAssetBundleRequest* ), il2cpp::CClass::FromName(OBFUSCATE_STR("AssetBundleRequest"), OBFUSCATE_STR("UnityEngine"), OBFUSCATE_STR("UnityEngine.AssetBundleModule.dll")), "get_asset", 0);
        return func(this);
    }

    mono::SArray<CObject*>* CAssetBundleRequest::GetAssets()
    {
        RESOLVE_SDK_METHOD(mono::SArray<CObject*>* (*)( CAssetBundleRequest* ), il2cpp::CClass::FromName(OBFUSCATE_STR("AssetBundleRequest"), OBFUSCATE_STR("UnityEngine"), OBFUSCATE_STR("UnityEngine.AssetBundleModule.dll")), "get_assets", 0);
        return func(this);
    }

    CAssetBundle* CAssetBundle::LoadFromFile(const char* pszPath)
    {
        RESOLVE_SDK_METHOD(CAssetBundle* (*)( mono::SString* ), GetIl2cppClass(), "LoadFromFile", 1);
        return func(mono::SString::Create(pszPath));
    }

    CObject* CAssetBundle::LoadAsset(const char* pszName, mono::CType* pType)
    {
        RESOLVE_SDK_METHOD(CObject* (*)( CAssetBundle*, mono::SString*, mono::CType* ), this->GetIl2cppClass(), "LoadAsset", 2);
        return func(this, mono::SString::Create(pszName), pType);
    }

    void CAssetBundle::Unload(bool bUnloadObjects)
    {
        RESOLVE_SDK_METHOD(void (*)( CAssetBundle*, bool ), this->GetIl2cppClass(), "Unload", 1);
        func(this, bUnloadObjects);
    }
} // usdk
