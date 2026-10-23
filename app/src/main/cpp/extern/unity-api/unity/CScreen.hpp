#pragma once

#include "common/Resolution.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CScreen
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        static SResolution GetResolution();
        static void SetResolution(int32_t w, int32_t h);
        static void SetResolution(SResolution res);
    };
} // usdk
