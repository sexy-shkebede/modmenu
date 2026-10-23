#pragma once

#include "CObject.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CTransform;
    class CGameObject;

    class CComponent : public CObject 
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static il2cpp::CClass* GetIl2cppBehaviourClass();
        static mono::CType*    GetMonoType();

        CTransform* GetTransform();
        CGameObject* GetGameObject();
        mono::SString* GetTag();
        CComponent* GetComponentInChildren(mono::CType* pType, bool bInactive = false);
        CComponent* GetComponentInParent(mono::CType* pType, bool bInactive = false);
        bool GetEnabled();
        void SetEnabled(bool bState);
    };
} // usdk
