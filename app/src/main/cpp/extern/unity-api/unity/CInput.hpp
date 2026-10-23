#pragma once

#include "common/Touch.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CInput
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        static STouch GetTouch(const uint8_t& index);
    };
} // usdk
