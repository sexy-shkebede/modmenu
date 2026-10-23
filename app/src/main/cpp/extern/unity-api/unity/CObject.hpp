#pragma once

#include "common/include.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CTransform;

    #pragma pack(1)
    class CObject 
    {
        char pad[sizeof(void*) * 2];
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        void* m_pCached;

        int GetInstanceID();
        static bool IsNullOrDestroyed(CObject* pObj);
        static CObject* FindObjectFromID(int iId);
        static mono::SArray<CObject*>* FindObjectsOfType(mono::CType* pType);
        static mono::SArray<CObject*>* FindObjectsByType(mono::CType* pType);
        static CObject* FindObjectOfType(mono::CType* pType);
        static CObject* Instantiate(CObject* pOrig, Vector3 vOrigin, Quaternion qRotation);
        static CObject* Instantiate(CObject* pOrig, Vector3 vOrigin, Quaternion qRotation, CTransform* pParent);
        void Destroy();
        void DontDestroyOnLoad();
        mono::SString* GetName();
        void SetName(mono::SString* u16Name);
        void SetName(const char* pszName);

    };
    #pragma pack()
} // usdk
