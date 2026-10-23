#include "CRenderer.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CRenderer::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Renderer", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CRenderer::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Renderer, UnityEngine.CoreModule.dll");
    }

    SBounds CRenderer::GetBounds()
    {
        RESOLVE_SDK_METHOD(SBounds (*)( CRenderer* ), this->GetIl2cppClass(), "get_bounds", 0);
        return func(this);
    }

    void CRenderer::SetLocalBounds(SBounds value)
    {
        RESOLVE_SDK_METHOD(void (*)( CRenderer*, SBounds ), this->GetIl2cppClass(), "set_localBounds", 1);
        func(this, value);
    }
    
    CMaterial* CRenderer::GetSharedMaterial()
    {
        RESOLVE_SDK_METHOD(CMaterial* (*)( CRenderer* ), this->GetIl2cppClass(), "get_sharedMaterial", 0);
        return func(this);
    }

    void CRenderer::SetSharedMaterial(CMaterial* pMaterial)
    {
        RESOLVE_SDK_METHOD(void (*)( CRenderer*, CMaterial* ), this->GetIl2cppClass(), "set_sharedMaterial", 1);
        func(this, pMaterial);
    }

    CMaterial* CRenderer::GetMaterial()
    {
        RESOLVE_SDK_METHOD(CMaterial* (*)( CRenderer* ), this->GetIl2cppClass(), "get_material", 0);
        return func(this);
    }

    void CRenderer::SetMaterial(CMaterial* pMaterial)
    {
        RESOLVE_SDK_METHOD(void (*)( CRenderer*, CMaterial* ), this->GetIl2cppClass(), "set_material", 1);
        func(this, pMaterial);
    }

    mono::SArray<CMaterial*>* CRenderer::GetMaterials()
    {
        RESOLVE_SDK_METHOD(mono::SArray<CMaterial*>* (*)( CRenderer* ), this->GetIl2cppClass(), "get_materials", 0);
        return func(this);
    }

    void CRenderer::SetMaterials(mono::SArray<CMaterial*>* Materials)
    {
        RESOLVE_SDK_METHOD(void (*)( CRenderer*, mono::SArray<CMaterial*>* ), this->GetIl2cppClass(), "set_materials", 1);
        func(this, Materials);
    }

    bool CRenderer::GetRendererEnabled()
    {
        RESOLVE_SDK_METHOD(bool (*)( CRenderer* ), this->GetIl2cppClass(), "get_enabled", 0);
        return func(this);
    }

    void CRenderer::SetRendererEnabled(bool bState)
    {
        RESOLVE_SDK_METHOD(void (*)( CRenderer*, bool ), this->GetIl2cppClass(), "set_enabled", 1);
        func(this, bState);
    }

    bool CRenderer::GetForceRenderingOff()
    {
        RESOLVE_SDK_METHOD(bool (*)( CRenderer* ), this->GetIl2cppClass(), "get_forceRenderingOff", 0);
        return func(this);
    }

    void CRenderer::SetForceRenderingOff(bool bState)
    {
        RESOLVE_SDK_METHOD(void (*)( CRenderer*, bool ), this->GetIl2cppClass(), "set_forceRenderingOff", 1);
        func(this, bState);
    }
} // usdk
