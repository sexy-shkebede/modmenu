#include "CTransform.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CTransform::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Transform", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CTransform::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Transform, UnityEngine.CoreModule.dll");
    }

    Vector3 CTransform::GetPosition() 
    {
        RESOLVE_SDK_METHOD(Vector3 (*)( CTransform* ), this->GetIl2cppClass(), "get_position", 0);
        return func(this);
    }

    void CTransform::SetPosition(Vector3 vecOrigin)
    {
        RESOLVE_SDK_METHOD(void (*)( CTransform*, Vector3 ), this->GetIl2cppClass(), "set_position", 1);
        func(this, vecOrigin);
    }

    Vector3 CTransform::GetLocalPosition() 
    {
        RESOLVE_SDK_METHOD(Vector3 (*)( CTransform* ), this->GetIl2cppClass(), "get_localPosition", 0);
        return func(this);
    }

    void CTransform::SetLocalPosition(Vector3 vecOrigin)
    {
        RESOLVE_SDK_METHOD(void (*)( CTransform*, Vector3 ), this->GetIl2cppClass(), "set_localPosition", 1);
        func(this, vecOrigin);
    }

    EulerAngles CTransform::GetEulerAngles()
    {
        RESOLVE_SDK_METHOD(EulerAngles (*)( CTransform* ), this->GetIl2cppClass(), "get_eulerAngles", 0);
        return func(this);
    }

    void CTransform::SetEulerAngles(EulerAngles vecAngles)
    {
        RESOLVE_SDK_METHOD(void (*)( CTransform*, EulerAngles ), this->GetIl2cppClass(), "set_eulerAngles", 1);
        func(this, vecAngles);
    }

    EulerAngles CTransform::GetLocalEulerAngles()
    {
        RESOLVE_SDK_METHOD(EulerAngles (*)( CTransform* ), this->GetIl2cppClass(), "get_localEulerAngles", 0);
        return func(this);
    }

    void CTransform::SetLocalEulerAngles(EulerAngles vecAngles)
    {
        RESOLVE_SDK_METHOD(void (*)( CTransform*, EulerAngles ), this->GetIl2cppClass(), "set_localEulerAngles", 1);
        func(this, vecAngles);
    }

    Vector3 CTransform::GetRight() 
    {
        RESOLVE_SDK_METHOD(Vector3 (*)( CTransform* ), this->GetIl2cppClass(), "get_right", 0);
        return func(this);
    }

    Vector3 CTransform::GetUp() 
    {
        RESOLVE_SDK_METHOD(Vector3 (*)( CTransform* ), this->GetIl2cppClass(), "get_up", 0);
        return func(this);
    }

    Vector3 CTransform::GetForward() 
    {
        RESOLVE_SDK_METHOD(Vector3 (*)( CTransform* ), this->GetIl2cppClass(), "get_forward", 0);
        return func(this);
    }

    Quaternion CTransform::GetRotation()
    {
        RESOLVE_SDK_METHOD(Quaternion (*)( CTransform* ), this->GetIl2cppClass(), "get_rotation", 0);
        return func(this);
    }

    void CTransform::SetRotation(Quaternion qRotation)
    {
        RESOLVE_SDK_METHOD(void (*)( CTransform*, Quaternion ), this->GetIl2cppClass(), "set_rotation", 1);
        func(this, qRotation);
    }

    Quaternion CTransform::GetLocalRotation()
    {
        RESOLVE_SDK_METHOD(Quaternion (*)( const CTransform* ), this->GetIl2cppClass(), "get_localRotation", 0);
        return func(this);
    }

    void CTransform::SetLocalRotation(Quaternion qRotation)
    {
        RESOLVE_SDK_METHOD(void (*)( CTransform*, Quaternion ), this->GetIl2cppClass(), "set_localRotation", 1);
        func(this, qRotation);
    }

    Vector3 CTransform::GetLocalScale() 
    {
        RESOLVE_SDK_METHOD(Vector3 (*)( CTransform* ), this->GetIl2cppClass(), "get_localScale", 0);
        return func(this);
    }

    void CTransform::SetLocalScale(Vector3 vecScale) 
    {
        RESOLVE_SDK_METHOD(void (*)( CTransform*, Vector3 ), this->GetIl2cppClass(), "set_localScale", 1);
        func(this, vecScale);
    }

    Vector3 CTransform::GetLossyScale() 
    {
        RESOLVE_SDK_METHOD(Vector3 (*)( CTransform* ), this->GetIl2cppClass(), "get_lossyScale", 0);
        return func(this);
    }

    CTransform* CTransform::GetParent() 
    {
        RESOLVE_SDK_METHOD(CTransform* (*)( CTransform* ), this->GetIl2cppClass(), "GetParent", 0);
        return func(this);
    }

    void CTransform::SetParent(CTransform* pParent) 
    {
        RESOLVE_SDK_METHOD(void (*)( CTransform*, CTransform* ), this->GetIl2cppClass(), "SetParent", 1);
        func(this, pParent);
    }

    void CTransform::SetParent(CTransform* pParent, bool bWorldPosStays) 
    {
        RESOLVE_SDK_METHOD(void (*)( CTransform*, CTransform*, bool ), this->GetIl2cppClass(), "SetParent", 2);
        func(this, pParent, bWorldPosStays);
    }

    void CTransform::Rotate(Vector3 vecEulers)
    {
        RESOLVE_SDK_METHOD(void (*)( CTransform*, Vector3 ), this->GetIl2cppClass(), "Rotate", 1);
        func(this, vecEulers);
    }

    void CTransform::LookAt(Vector3 vOrigin)
    {
        #define r2d() (180.f / 3.14159265358979323846f)

        Vector3 dir = vOrigin - this->GetPosition();

        float yaw = atan2f(dir.x, dir.z) * r2d();
        
        float distanceXZ = sqrtf(dir.x * dir.x + dir.z * dir.z);
        float pitch = -atan2f(dir.y, distanceXZ) * r2d();

        this->SetEulerAngles(EulerAngles(pitch, yaw, 0.f));

        #undef r2d
    }
} // usdk
