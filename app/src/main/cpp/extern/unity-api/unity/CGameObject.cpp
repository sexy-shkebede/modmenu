#include "CGameObject.hpp"
#include <string.h>
#include "../il2cpp/CClass.hpp"
#include "../il2cpp/CObject.hpp"
#include "../mono/CType.hpp"
#include "../mono/SString.hpp"
#include "CMaterial.hpp"
#include "CMeshRenderer.hpp"
#include "../sdk_core.hpp"

namespace usdk
{
    il2cpp::CClass* CGameObject::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("GameObject", "UnityEngine", "UnityEngine.CoreModule.dll");
    }

    mono::CType* CGameObject::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("UnityEngine.GameObject, UnityEngine.CoreModule.dll");
    }

    CGameObject* CGameObject::Create(const char* pszName)
    {
        RESOLVE_SDK_METHOD(CGameObject* (*)( CGameObject*, mono::SString* ), GetIl2cppClass(), "Internal_CreateGameObject", 2);
        CGameObject* obj = (CGameObject*)il2cpp::CObject::Allocate(CGameObject::GetIl2cppClass());
        func(obj, mono::SString::Create(pszName));
        return obj;
    }

    CGameObject* CGameObject::CreatePrimitive(const char* pszName, EPrimitiveType eType, CMaterial* pMaterial)
    {
        RESOLVE_SDK_METHOD(CGameObject* (*)( int ), GetIl2cppClass(), "CreatePrimitive", 1);

        CGameObject* pObject = func((int)eType);
        pObject->SetName(pszName);
        
        if (pMaterial)
        {
            CMeshRenderer* pMeshRenderer = (decltype(pMeshRenderer))pObject->GetComponent(CMeshRenderer::GetMonoType());
            if (pMeshRenderer)
                pMeshRenderer->SetMaterial(pMaterial);
        }

        return pObject;
    }

    CTransform* CGameObject::GetTransform()
    {
        RESOLVE_SDK_METHOD(CTransform* (*)( CGameObject* ), this->GetIl2cppClass(), "get_transform", 0);
        return func(this);
    }

    mono::SString* CGameObject::GetTag()
    {
        RESOLVE_SDK_METHOD(mono::SString* (*)( CGameObject* ), this->GetIl2cppClass(), "get_tag", 0);
        return func(this);
    }

    bool CGameObject::CompareTag(const char* pszTag)
    {
        return !strcmp(pszTag, this->GetTag()->ToUTF8());
    }

    bool CGameObject::GetIsActive()
    {
        RESOLVE_SDK_METHOD(bool (*)( CGameObject* ), this->GetIl2cppClass(), "get_activeSelf", 0);
        return func(this);
    }

    void CGameObject::SetActive(bool bState)
    {
        RESOLVE_SDK_METHOD(void (*)( CGameObject*, bool ), this->GetIl2cppClass(), "SetActive", 1);
        func(this, bState);
    }

    int CGameObject::GetLayer()
    {
        RESOLVE_SDK_METHOD(int (*)( CGameObject* ), this->GetIl2cppClass(), "get_layer", 0);
        return func(this);
    }

    void CGameObject::SetLayer(int iLayer)
    {
        RESOLVE_SDK_METHOD(void (*)( CGameObject*, int ), this->GetIl2cppClass(), "set_layer", 1);
        func(this, iLayer);
    }

    CComponent* CGameObject::AddComponent(mono::CType* pType)
    {
        RESOLVE_SDK_METHOD(CComponent* (*)( CGameObject*, mono::CType* ), this->GetIl2cppClass(), "AddComponent", 1);
        return func(this, pType);
    }

    CComponent* CGameObject::GetComponent(mono::CType* pType)
    {
        RESOLVE_SDK_METHOD(CComponent* (*)( CGameObject*, mono::CType* ), this->GetIl2cppClass(), "GetComponent", 1);
        return func(this, pType);
    }

    mono::SArray<CComponent*>* CGameObject::GetComponents(mono::CType* pType)
    {
        RESOLVE_SDK_METHOD(mono::SArray<CComponent*>* (*)( CGameObject*, mono::CType* ), this->GetIl2cppClass(), "GetComponents", 1);
        return func(this, pType);
    }

    mono::SArray<CComponent*>* CGameObject::GetComponentsInChildren(mono::CType* pType, bool bIncludeInactive)
    {
        RESOLVE_SDK_METHOD(mono::SArray<CComponent*>* (*)( CGameObject*, mono::CType*, bool ), this->GetIl2cppClass(), "GetComponentsInChildren", 2);
        return func(this, pType, bIncludeInactive);
    }
} // usdk
