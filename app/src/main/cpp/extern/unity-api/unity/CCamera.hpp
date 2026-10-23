#pragma once

#include "common/include.hpp"
#include "CComponent.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    enum ECameraClearFlags : uint8_t
    {
        eNone,
        eSkybox,
        eColor,
        eSolidColor,
        eDepth,
        eNothing,
    };

    class CCamera : public CComponent
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        int GetCullingMask();
        void SetCullingMask(int iValue);
        float GetFOV();
        void SetFOV(float fValue);
        static CCamera* GetMain();
        static CCamera* GetCurrent();
        SMatrix4x4 GetViewMatrix();
        SMatrix4x4 GetProjectionMatrix();
        Vector3 WorldToScreen(Vector3 vOrigin);
        Vector3 WorldToScreenClamped(Vector3 vOrigin);
        SRay ScreenPointToRay(Vector3 vOrigin);
        ECameraClearFlags GetFlags();
        void SetFlags(ECameraClearFlags iFlags);
        void SetBackgroundColor(Color color);
    };
} // usdk
