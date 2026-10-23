#include "CNetworkObject.hpp"
#include "../../il2cpp/CClass.hpp"
#include "../../il2cpp/CMethod.hpp"
#include "../../mono/CType.hpp"

namespace photonsdk
{
    il2cpp::CClass* CNetworkObject::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("NetworkObject", "Fusion", "Fusion.Runtime.dll");
    }

    mono::CType* CNetworkObject::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("Fusion.NetworkObject, Fusion.Runtime");
    }

    bool CNetworkObject::GetIsValid()
    {
        RESOLVE_SDK_METHOD(bool (*)(void*), GetIl2cppClass(), "get_IsValid", 0);
        return func(this);
    }

    bool CNetworkObject::GetHasInputAuthority()
    {
        RESOLVE_SDK_METHOD(bool (*)(void*), GetIl2cppClass(), "get_HasInputAuthority", 0);
        return func(this);
    }

    bool CNetworkObject::GetHasStateAuthority()
    {
        RESOLVE_SDK_METHOD(bool (*)(void*), GetIl2cppClass(), "get_HasStateAuthority", 0);
        return func(this);
    }

    SPlayerRef CNetworkObject::GetInputAuthority()
    {
        RESOLVE_SDK_METHOD(SPlayerRef (*)(void*), GetIl2cppClass(), "get_InputAuthority", 0);
        return func(this);
    }

    SPlayerRef CNetworkObject::GetStateAuthority()
    {
        RESOLVE_SDK_METHOD(SPlayerRef (*)(void*), GetIl2cppClass(), "get_StateAuthority", 0);
        return func(this);
    }

    void CNetworkObject::ReleaseStateAuthority()
    {
        RESOLVE_SDK_METHOD(void (*)(void*), GetIl2cppClass(), "ReleaseStateAuthority", 0);
        func(this);
    }

    void CNetworkObject::RemoveInputAuthority()
    {
        RESOLVE_SDK_METHOD(void (*)(void*), GetIl2cppClass(), "RemoveInputAuthority", 0);
        func(this);
    }

    void CNetworkObject::MakeUnowned()
    {
        RESOLVE_SDK_METHOD(void (*)(void*), GetIl2cppClass(), "MakeUnowned", 0);
        func(this);
    }
}
