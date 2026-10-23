#include "CMeshFilter.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CMeshFilter::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("MeshFilter", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CMeshFilter::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.MeshFilter, UnityEngine.CoreModule.dll");
    }

    CMesh* CMeshFilter::GetSharedMesh()
    {
        RESOLVE_SDK_METHOD(CMesh* (*)( CMeshFilter* ), this->GetIl2cppClass(), "get_sharedMesh", 0);
        return func(this);
    }

    void CMeshFilter::SetSharedMesh(CMesh* pMesh)
    {
        RESOLVE_SDK_METHOD(void (*)( CMeshFilter*, CMesh* ), this->GetIl2cppClass(), "set_sharedMesh", 1);
        func(this, pMesh);
    }

    CMesh* CMeshFilter::GetMesh()
    {
        RESOLVE_SDK_METHOD(CMesh* (*)( CMeshFilter* ), this->GetIl2cppClass(), "get_mesh", 0);
        return func(this);
    }

    void CMeshFilter::SetMesh(CMesh* pMesh)
    {
        RESOLVE_SDK_METHOD(void (*)( CMeshFilter*, CMesh* ), this->GetIl2cppClass(), "set_mesh", 1);
        func(this, pMesh);
    }
} // usdk
