#pragma once

#include "CObject.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CTransform;
    class CComponent;

    enum EPrimitiveType : int
    {
        eSphere   = 0,
        eCapsule  = 1,
        eCylinder = 2,
        eCube     = 3,
        ePlane    = 4,
        eQuad     = 5
    };

    class CGameObject : public CObject
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        static CGameObject*    Create(const char* pszName);
        static CGameObject*    CreatePrimitive(const char* pszName, EPrimitiveType eType, class CMaterial* pMaterial = nullptr);
        CTransform*            GetTransform();
        mono::SString*         GetTag();
        bool                   CompareTag(const char* pszTag);
        bool                   GetIsActive();
        void                   SetActive(bool bState);
        int                    GetLayer();
        void                   SetLayer(int iLayer);

        CComponent*            AddComponent(mono::CType* pType);
        CComponent*            GetComponent(mono::CType* pType);
        mono::SArray<CComponent*>* GetComponents(mono::CType* pType);
        mono::SArray<CComponent*>* GetComponentsInChildren(mono::CType* pType, bool bIncludeInactive = true);
    };
} // usdk
