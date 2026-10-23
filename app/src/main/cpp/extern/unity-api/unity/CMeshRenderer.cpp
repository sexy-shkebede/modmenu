#include "CMeshRenderer.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CMeshRenderer::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("MeshRenderer", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CMeshRenderer::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.MeshRenderer, UnityEngine.CoreModule.dll");
    }

    void CMeshRenderer::DontStripMeshRenderer()
    {
        RESOLVE_SDK_METHOD(void (*)( CMeshRenderer* ), this->GetIl2cppClass(), "DontStripMeshRenderer", 0);
        func(this);
    }
} // usdk
