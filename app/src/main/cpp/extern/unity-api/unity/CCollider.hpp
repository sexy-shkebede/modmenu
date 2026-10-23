#pragma once

#include "CComponent.hpp"
#include "common/Bounds.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CCollider : public CComponent
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        SBounds GetBounds();
        bool GetIsTrigger();
        void SetIsTrigger(bool bState);
    };
} // usdk
