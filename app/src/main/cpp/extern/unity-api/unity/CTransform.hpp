#pragma once

#include "common/include.hpp"
#include "CComponent.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CTransform : public CComponent 
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        Vector3     GetPosition();
        void        SetPosition(Vector3 vecOrigin);

        Vector3     GetLocalPosition();
        void        SetLocalPosition(Vector3 vecOrigin);

        EulerAngles GetEulerAngles();
        void        SetEulerAngles(EulerAngles vecAngles);

        EulerAngles GetLocalEulerAngles();
        void        SetLocalEulerAngles(EulerAngles vecAngles);

        Vector3     GetRight();
        Vector3     GetUp();
        Vector3     GetForward();

        Quaternion  GetRotation();
        void        SetRotation(Quaternion qRotation);

        Quaternion  GetLocalRotation();
        void        SetLocalRotation(Quaternion qRotation);

        Vector3     GetLocalScale();
        void        SetLocalScale(Vector3 vecScale);

        Vector3     GetLossyScale();

        CTransform* GetParent();
        void        SetParent(CTransform* pParent);
        void        SetParent(CTransform* pParent, bool bWorldPosStays);

        void        Rotate(Vector3 vecEulers);

        void        LookAt(Vector3 vOrigin);
    };
} // usdk
