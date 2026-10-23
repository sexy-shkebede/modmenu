#include "CMesh.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CMesh::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Mesh", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CMesh::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Mesh, UnityEngine.CoreModule.dll");
    }

    CMesh* CMesh::Create()
    {
        RESOLVE_SDK_METHOD(void (*)( CMesh* ), CMesh::GetIl2cppClass(), "Internal_Create", 1);
        CMesh* ret = (CMesh*)il2cpp::CObject::Allocate(GetIl2cppClass());
        func(ret);
        return ret;
    }
} // usdk
