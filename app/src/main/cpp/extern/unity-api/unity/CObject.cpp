#include "CObject.hpp"
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "../mono/SArray.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CObject::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("Object", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CObject::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.Object, UnityEngine.CoreModule.dll");
    }

    int CObject::GetInstanceID() 
    {
        RESOLVE_SDK_METHOD(int (*)( CObject* ), this->GetIl2cppClass(), "GetInstanceID", 0);
        return func(this);
    }

    bool CObject::IsNullOrDestroyed(CObject* pObj)
    {
        if (!pObj) return true;
        if (pObj->m_pCached == nullptr) return true;
        return false;
    }

    CObject* CObject::FindObjectFromID(int iId)
    {
        RESOLVE_SDK_METHOD(CObject* (*)( int32_t ), GetIl2cppClass(), "FindObjectFromInstanceID", 1);
        return func(iId);
    }

    mono::SArray<CObject*>* CObject::FindObjectsOfType(mono::CType* pType)
    {
        RESOLVE_SDK_METHOD(mono::SArray<CObject*>* (*)( mono::CType* ), GetIl2cppClass(), "FindObjectsOfType", 1);
        return func(pType);
    }

    mono::SArray<CObject*>* CObject::FindObjectsByType(mono::CType* pType)
    {
        RESOLVE_SDK_METHOD(mono::SArray<CObject*>* (*)( mono::CType*, uint8_t, uint8_t ), GetIl2cppClass(), "FindObjectsByType", 3);
        return func(pType, 0, 0);
    }

    CObject* CObject::FindObjectOfType(mono::CType* pType)
    {
        mono::SArray<CObject*>* pArr = CObject::FindObjectsOfType(pType);
        if (pArr != NULL)
        {
            if (pArr->IsValid())
            {
                return pArr->Get(0);
            }
        }
        return NULL;
    }

    CObject* CObject::Instantiate(CObject* pOrig, Vector3 vOrigin, Quaternion qRotation)
    {
        RESOLVE_SDK_METHOD(CObject* (*)( CObject*, Vector3, Quaternion ), GetIl2cppClass(), "Instantiate", 3);
        return func(pOrig, vOrigin, qRotation);
    }

    CObject* CObject::Instantiate(CObject* pOrig, Vector3 vOrigin, Quaternion qRotation, CTransform* pParent)
    {
        RESOLVE_SDK_METHOD(CObject* (*)( CObject*, Vector3, Quaternion, CTransform* ), GetIl2cppClass(), "Instantiate", 4);
        return func(pOrig, vOrigin, qRotation, pParent);
    }

    void CObject::Destroy() 
    {
        RESOLVE_SDK_METHOD(void (*)( CObject* ), this->GetIl2cppClass(), "Destroy", 1);
        func(this);
    }

    void CObject::DontDestroyOnLoad() 
    {
        RESOLVE_SDK_METHOD(void (*)( CObject* ), this->GetIl2cppClass(), "DontDestroyOnLoad", 1);
        func(this);
    }

    mono::SString* CObject::GetName()
    {
        RESOLVE_SDK_METHOD(mono::SString* (*)( CObject* ), this->GetIl2cppClass(), "get_name", 0);
        return func(this);
    }

    void CObject::SetName(mono::SString* u16Name)
    {
        RESOLVE_SDK_METHOD(void (*)( CObject*, mono::SString* ), this->GetIl2cppClass(), "set_name", 1);
        func(this, u16Name);
    }

    void CObject::SetName(const char* pszName)
    {
        this->SetName(mono::SString::Create(pszName));
    }
} // usdk
