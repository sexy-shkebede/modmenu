#include "CComponent.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CComponent::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Component", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    il2cpp::CClass* CComponent::GetIl2cppBehaviourClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Behaviour", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CComponent::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Component, UnityEngine.CoreModule.dll");
    }

    CTransform* CComponent::GetTransform() 
    {
        RESOLVE_SDK_METHOD(CTransform* (*)( CComponent* ), this->GetIl2cppClass(), "get_transform", 0);
        return func(this);
    }

    CGameObject* CComponent::GetGameObject() 
    {
        RESOLVE_SDK_METHOD(CGameObject* (*)( CComponent* ), this->GetIl2cppClass(), "get_gameObject", 0);
        return func(this);
    }

    mono::SString* CComponent::GetTag() 
    {
        RESOLVE_SDK_METHOD(mono::SString* (*)( CComponent* ), this->GetIl2cppClass(), "get_tag", 0);
        return func(this);
    }

    CComponent* CComponent::GetComponentInChildren(mono::CType* pType, bool bInactive)
    {
        RESOLVE_SDK_METHOD(CComponent* (*)( CComponent*, mono::CType*, bool ), this->GetIl2cppClass(), "GetComponentInChildren", 2);
        return func(this, pType, bInactive);
    }

    CComponent* CComponent::GetComponentInParent(mono::CType* pType, bool bInactive)
    {
        RESOLVE_SDK_METHOD(CComponent* (*)( CComponent*, mono::CType*, bool ), this->GetIl2cppClass(), "GetComponentInParent", 2);
        return func(this, pType, bInactive);
    }

    bool CComponent::GetEnabled()
    {
        RESOLVE_SDK_METHOD(bool (*)( CComponent* ), this->GetIl2cppBehaviourClass(), "get_enabled", 0);
        return func(this);
    }

    void CComponent::SetEnabled(bool bState)
    {
        RESOLVE_SDK_METHOD(bool (*)( CComponent*, bool ), this->GetIl2cppBehaviourClass(), "set_enabled", 1);
        func(this, bState);
    }
} // usdk
