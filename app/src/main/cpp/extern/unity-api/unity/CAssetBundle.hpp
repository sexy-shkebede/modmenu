#pragma once

#include "CObject.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CAssetBundle;

    class CAssetBundleCreateRequest 
    {
    public:
        CAssetBundle *GetAssetBundle();
    };

    class CAssetBundleRequest 
    {
    public:
        CObject* GetAsset();
        mono::SArray<CObject*>* GetAssets();
    };

    class CAssetBundle : public CObject
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        static CAssetBundle *LoadFromFile(const char* pszPath);
        CObject *LoadAsset(const char* pszName, mono::CType* pType);
        void Unload(bool bUnloadObjects);
    };
} // usdk
