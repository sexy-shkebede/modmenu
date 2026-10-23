#pragma once

#include "CRenderer.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CMeshRenderer : public CRenderer 
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        void DontStripMeshRenderer();
    };
} // usdk
