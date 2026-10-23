#pragma once
#include "../../sdk_core.hpp"
#include "../../unity/CComponent.hpp"
#include "SPlayerRef.hpp"

#include "sdk_fwd.hpp"

namespace photonsdk
{
    class CNetworkObject : public usdk::CComponent
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType* GetMonoType();

        bool GetIsValid();
        bool GetHasInputAuthority();
        bool GetHasStateAuthority();
        SPlayerRef GetInputAuthority();
        SPlayerRef GetStateAuthority();

        void ReleaseStateAuthority();
        void RemoveInputAuthority();
        void MakeUnowned();
    };
}
