#pragma once

#include "CObject.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CMesh : public CObject
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        static CMesh* Create();
    };
} // usdk
