#pragma once

#include "CComponent.hpp"
#include "CMesh.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CMeshFilter : public CComponent
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        CMesh*                 GetSharedMesh();
        void                   SetSharedMesh(CMesh* pMesh);
        CMesh*                 GetMesh();
        void                   SetMesh(CMesh* pMesh);
    };
} // usdk
