#include "CCamera.hpp"
#include "../sdk_core.hpp"
#include "../il2cpp/CClass.hpp"
#include "../mono/CType.hpp"
#include "CTransform.hpp"

namespace usdk
{
    il2cpp::CClass* CCamera::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Camera", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CCamera::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Camera, UnityEngine.CoreModule.dll");
    }

    int CCamera::GetCullingMask() 
    {
        RESOLVE_SDK_METHOD(int (*)( CCamera* ), this->GetIl2cppClass(), "get_cullingMask", 0);
        return func(this);
    }

    void CCamera::SetCullingMask(int iValue)
    {
        RESOLVE_SDK_METHOD(void (*)( CCamera*, int ), this->GetIl2cppClass(), "set_cullingMask", 1);
        func(this, iValue);
    }

    float CCamera::GetFOV() 
    {
        RESOLVE_SDK_METHOD(float (*)( CCamera* ), this->GetIl2cppClass(), "get_fieldOfView", 0);
        return func(this);
    }

    void CCamera::SetFOV(float fValue)
    {
        RESOLVE_SDK_METHOD(void (*)( CCamera*, float ), this->GetIl2cppClass(), "set_fieldOfView", 1);
        func(this, fValue);
    }

    CCamera* CCamera::GetMain() 
    {
        RESOLVE_SDK_METHOD(CCamera* (*)(  ), GetIl2cppClass(), "get_main", 0);
        return func();
    }

    CCamera* CCamera::GetCurrent() 
    {
        RESOLVE_SDK_METHOD(CCamera* (*)(  ), GetIl2cppClass(), "get_current", 0);
        return func();
    }

    SMatrix4x4 CCamera::GetViewMatrix()
    {
        SMatrix4x4 ret{};
        RESOLVE_SDK_METHOD(void (*)( CCamera*, SMatrix4x4& ), this->GetIl2cppClass(), "get_worldToCameraMatrix_Injected", 1);
        func(this, ret);
        return ret;
    }

    SMatrix4x4 CCamera::GetProjectionMatrix()
    {
        SMatrix4x4 ret{};
        RESOLVE_SDK_METHOD(void (*)( CCamera*, SMatrix4x4& ), this->GetIl2cppClass(), "get_projectionMatrix_Injected", 1);
        func(this, ret);
        return ret;
    }

    Vector3 CCamera::WorldToScreen(Vector3 vOrigin)
    {
        RESOLVE_SDK_METHOD(Vector3 (*)( CCamera*, Vector3 ), this->GetIl2cppClass(), "WorldToScreenPoint", 1);
        return func(this, vOrigin);
    }

    Vector3 CCamera::WorldToScreenClamped(Vector3 vOrigin)
    {
        Vector3 ret{};
        CCamera* cam = CCamera::GetMain();
        if (cam != NULL) {
            const Vector3 vecCamera = cam->GetTransform()->GetPosition();
            const Vector3 vecForward = cam->GetTransform()->GetForward();
            const Vector3 vecDelta = vOrigin - vecCamera;
            const float fDot = vecForward.Dot(vecDelta);
            const Vector3 vecClamped = fDot <= 0 ? vecCamera + (vecDelta - (vecForward * fDot * 1.01f)) : vOrigin;
            ret = cam->WorldToScreen(vecClamped);
        }
        return ret;
    }

    SRay CCamera::ScreenPointToRay(Vector3 vOrigin)
    {
        RESOLVE_SDK_METHOD(SRay (*)( CCamera*, Vector3 ), this->GetIl2cppClass(), "ScreenPointToRay", 1);
        if (!func) return SRay();
        return func(this, vOrigin);
    }

    ECameraClearFlags CCamera::GetFlags()
    {
        RESOLVE_SDK_METHOD(ECameraClearFlags (*)( CCamera* ), this->GetIl2cppClass(), "get_clearFlags", 0);
        return func(this);
    }

    void CCamera::SetFlags(ECameraClearFlags iFlags)
    {
        RESOLVE_SDK_METHOD(void (*)( CCamera*, ECameraClearFlags ), this->GetIl2cppClass(), "set_clearFlags", 1);
        func(this, iFlags);
    }

    void CCamera::SetBackgroundColor(Color color)
    {
        RESOLVE_SDK_METHOD(void (*)( CCamera*, Color ), this->GetIl2cppClass(), "set_backgroundColor", 1);
        func(this, color);
    }
} // usdk
