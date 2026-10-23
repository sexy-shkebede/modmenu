#include "CCollider.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CCollider::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Collider", "UnityEngine", "UnityEngine.PhysicsModule.dll");
    }

    mono::CType* CCollider::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Collider, UnityEngine.PhysicsModule.dll");
    }

    SBounds CCollider::GetBounds() 
    {
        RESOLVE_SDK_METHOD(SBounds (*)( CCollider* ), this->GetIl2cppClass(), "get_bounds", 0);
        return func(this);
    }
    
    bool CCollider::GetIsTrigger()
    {
        RESOLVE_SDK_METHOD(bool (*)( CCollider* ), this->GetIl2cppClass(), "get_isTrigger", 0);
        return func(this);
    }
    
    void CCollider::SetIsTrigger(bool bState)
    {
        RESOLVE_SDK_METHOD(void (*)( CCollider*, bool ), this->GetIl2cppClass(), "set_isTrigger", 1);
        return func(this, bState);
    }
} // usdk
