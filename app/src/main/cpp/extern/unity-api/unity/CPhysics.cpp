#include "CPhysics.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CPhysics::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Physics", "UnityEngine", "UnityEngine.PhysicsModule.dll");
    }

    mono::CType* CPhysics::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Physics, UnityEngine.PhysicsModule.dll");
    }

    Vector3 CPhysics::GetGravity()
    {
        RESOLVE_SDK_METHOD(Vector3 (*)(  ), CPhysics::GetIl2cppClass(), "get_gravity", 0);
        return func();
    }

    bool CPhysics::Raycast(Vector3 vOrigin, Vector3 vDirection)
    {
        RESOLVE_SDK_METHOD(bool (*)( Vector3, Vector3 ), GetIl2cppClass(), "Raycast", 2);
        return func(vOrigin, vDirection);
    }

    bool CPhysics::Raycast(Vector3 vOrigin, Vector3 vDirection, SRaycastHit* pHit, float fMaxDistance, int iLayerMask)
    {
        RESOLVE_SDK_METHOD(bool (*)( Vector3, Vector3, SRaycastHit*, float, int, int ), GetIl2cppClass(), "Raycast", 6);
        return func(vOrigin, vDirection, pHit, fMaxDistance, iLayerMask, 0);
    }

    bool CPhysics::Linecast(Vector3 vStart, Vector3 vEnd)
    {
        RESOLVE_SDK_METHOD(bool (*)( Vector3, Vector3 ), GetIl2cppClass(), "Linecast", 2);
        return func(vStart, vEnd);
    }

    bool CPhysics::Linecast(Vector3 vStart, Vector3 vEnd, SRaycastHit* pHit, int iLayerMask)
    {
        RESOLVE_SDK_METHOD(bool (*)( Vector3, Vector3, SRaycastHit*, int, int ), GetIl2cppClass(), "Linecast", 5);
        return func(vStart, vEnd, pHit, iLayerMask, 0);
    }

    int CPhysics::OverlapBoxNonAlloc(Vector3 vCenter, Vector3 vHalfExtents, mono::SArray<CCollider*>* pResults, Quaternion vOrientation, int iLayerMask, int iQueryTriggerInteraction)
    {
        RESOLVE_SDK_METHOD(int (*)( Vector3, Vector3, mono::SArray<CCollider*>*, Quaternion, int, int ), GetIl2cppClass(), "OverlapBoxNonAlloc", 6);
        return func(vCenter, vHalfExtents, pResults, vOrientation, iLayerMask, iQueryTriggerInteraction);
    }
} // usdk
