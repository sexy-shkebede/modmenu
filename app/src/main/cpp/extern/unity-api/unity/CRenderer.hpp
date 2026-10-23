#pragma once

#include "CComponent.hpp"
#include "CMaterial.hpp"
#include "common/Bounds.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CMaterial;

    class CRenderer : public CComponent 
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();
        
        SBounds GetBounds();
        bool GetRendererEnabled();
        bool GetForceRenderingOff();
        void SetLocalBounds(SBounds value);
        void SetRendererEnabled(bool bState);
        void SetForceRenderingOff(bool bState);

        CMaterial* GetMaterial();
        CMaterial* GetSharedMaterial();
        mono::SArray<CMaterial*>* GetMaterials();
        void SetMaterial(CMaterial* pMaterial);
        void SetSharedMaterial(CMaterial* pMaterial);
        void SetMaterials(mono::SArray<CMaterial*>* Materials);
    };
} // usdk
