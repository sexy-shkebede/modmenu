#include "CRigidbody.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CRigidbody::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Rigidbody", "UnityEngine", "UnityEngine.PhysicsModule.dll");
    }

    mono::CType* CRigidbody::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Rigidbody, UnityEngine.PhysicsModule.dll");
    }

    Vector3 CRigidbody::GetVelocity()
    {
        RESOLVE_SDK_METHOD(Vector3 (*)( CRigidbody* ), this->GetIl2cppClass(), "get_velocity", 0);
        return func(this);
    }

    void CRigidbody::SetVelocity(Vector3 vecVelocity)
    {
        RESOLVE_SDK_METHOD(void (*)( CRigidbody*, Vector3 ), this->GetIl2cppClass(), "set_velocity", 1);
        func(this, vecVelocity);
    }

    Vector3 CRigidbody::GetLinearVelocity()
    {
        RESOLVE_SDK_METHOD(Vector3 (*)( CRigidbody* ), this->GetIl2cppClass(), "get_linearVelocity", 0);
        return func(this);
    }

    void CRigidbody::SetLinearVelocity(Vector3 vecVelocity)
    {
        RESOLVE_SDK_METHOD(void (*)( CRigidbody*, Vector3 ), this->GetIl2cppClass(), "set_linearVelocity", 1);
        func(this, vecVelocity);
    }

    Vector3 CRigidbody::GetAngularVelocity()
    {
        RESOLVE_SDK_METHOD(Vector3 (*)( CRigidbody* ), this->GetIl2cppClass(), "get_angularVelocity", 0);
        return func(this);
    }

    void CRigidbody::SetAngularVelocity(Vector3 vecVelocity)
    {
        RESOLVE_SDK_METHOD(void (*)( CRigidbody*, Vector3 ), this->GetIl2cppClass(), "set_angularVelocity", 1);
        func(this, vecVelocity);
    }

    float CRigidbody::GetMass()
    {
        RESOLVE_SDK_METHOD(float (*)( CRigidbody* ), this->GetIl2cppClass(), "get_mass", 0);
        return func(this);
    }

    void CRigidbody::SetMass(float fValue)
    {
        RESOLVE_SDK_METHOD(void (*)( CRigidbody*, float ), this->GetIl2cppClass(), "set_mass", 1);
        func(this, fValue);
    }

    bool CRigidbody::GetUseGravity()
    {
        RESOLVE_SDK_METHOD(bool (*)( CRigidbody* ), this->GetIl2cppClass(), "get_useGravity", 0);
        return func(this);
    }

    void CRigidbody::SetUseGravity(bool bState)
    {
        RESOLVE_SDK_METHOD(void (*)( CRigidbody*, bool ), this->GetIl2cppClass(), "set_useGravity", 1);
        func(this, bState);
    }

    bool CRigidbody::GetIsKinematic()
    {
        RESOLVE_SDK_METHOD(bool (*)( CRigidbody* ), this->GetIl2cppClass(), "get_isKinematic", 0);
        return func(this);
    }

    void CRigidbody::SetIsKinematic(bool bState)
    {
        RESOLVE_SDK_METHOD(void (*)( CRigidbody*, bool ), this->GetIl2cppClass(), "set_isKinematic", 1);
        func(this, bState);
    }

    void CRigidbody::AddForce(Vector3 vecForce, EForceMode eMode)
    {
        RESOLVE_SDK_METHOD(void (*)( CRigidbody*, Vector3, EForceMode ), this->GetIl2cppClass(), "AddForce", 2);
        func(this, vecForce, eMode);
    }
} // usdk
