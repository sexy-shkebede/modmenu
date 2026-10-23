#pragma once

#include "CComponent.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    enum EForceMode : int
    {
        eForce = 0,
	    eAcceleration = 5,
	    eImpulse = 1,
	    eVelocityChange = 2
    };

    class CRigidbody : public CComponent
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        Vector3 GetVelocity();
        void    SetVelocity(Vector3 vecVelocity);

        Vector3 GetLinearVelocity();
        void    SetLinearVelocity(Vector3 vecVelocity);

        Vector3 GetAngularVelocity();
        void    SetAngularVelocity(Vector3 vecVelocity);

        float   GetMass();
        void    SetMass(float fValue);

        bool    GetUseGravity();
        void    SetUseGravity(bool bState);

        bool    GetIsKinematic();
        void    SetIsKinematic(bool bState);

        void    AddForce(Vector3 vecForce, EForceMode eMode);
    };
} // usdk
