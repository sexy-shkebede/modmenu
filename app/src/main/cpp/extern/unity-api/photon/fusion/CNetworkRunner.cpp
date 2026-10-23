#include "CNetworkRunner.hpp"
#include "../../il2cpp/CClass.hpp"
#include "../../il2cpp/CMethod.hpp"
#include "../../mono/CType.hpp"
#include "../../il2cpp/CType.hpp"
#include <string.h>

namespace photonsdk
{
    il2cpp::CClass* CNetworkRunner::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("NetworkRunner", "Fusion", "Fusion.Runtime.dll");
    }

    mono::CType* CNetworkRunner::GetMonoType()
    {
        RESOLVE_SDK_MONO_TYPE("Fusion.NetworkRunner, Fusion.Runtime");
    }

    void CNetworkRunner::Despawn(CNetworkObject* pNetworkObject)
    {
        RESOLVE_SDK_METHOD(void (*)(void*, CNetworkObject*), GetIl2cppClass(), "Despawn", 1);
        func(this, pNetworkObject);
    }

    void CNetworkRunner::Disconnect(SPlayerRef player, uint8_t token)
    {
        RESOLVE_SDK_METHOD(void (*)(void*, SPlayerRef, uint8_t), GetIl2cppClass(), "Disconnect", 2);
        func(this, player, token);
    }

    CNetworkObject* CNetworkRunner::Spawn(CNetworkObject* pPrefab, mono::SNullable<Vector3> position, mono::SNullable<Quaternion> rotation, mono::SNullable<SPlayerRef> inputAuthority, void* onBeforeSpawned, int32_t flags)
    {
        static void* pMethodPtr = nullptr;
        if (!pMethodPtr)
        {
            void* pIter{};
            while (il2cpp::CMethod* pMethod = GetIl2cppClass()->GetMethods(&pIter))
            {
                if (pMethod && !strcmp(pMethod->GetName(), oxorany("Spawn")) && pMethod->GetArgCount() == 6)
                {
                    il2cpp::CType** pArgs = pMethod->GetArgs();
                    il2cpp::CClass* pArgClass = pArgs[0]->GetClassOrElementClass();
                    if (pArgClass && !strcmp(pArgClass->GetName(), oxorany("NetworkObject")))
                    {
                        pMethodPtr = pMethod->GetMethodPointer();
                        break;
                    }
                }
            }
        }
        
        if (pMethodPtr)
        {
            auto func = (CNetworkObject* (*)(void*, CNetworkObject*, mono::SNullable<Vector3>, mono::SNullable<Quaternion>, mono::SNullable<SPlayerRef>, void*, int32_t))pMethodPtr;
            return func(this, pPrefab, position, rotation, inputAuthority, onBeforeSpawned, flags);
        }
        return nullptr;
    }

    SPlayerRef CNetworkRunner::GetLocalPlayer()
    {
        RESOLVE_SDK_METHOD(SPlayerRef (*)(void*), GetIl2cppClass(), "get_LocalPlayer", 0);
        return func(this);
    }

    CNetworkObject* CNetworkRunner::GetPlayerObject(SPlayerRef player)
    {
        RESOLVE_SDK_METHOD(CNetworkObject* (*)(void*, SPlayerRef), GetIl2cppClass(), "GetPlayerObject", 1);
        return func(this, player);
    }

    bool CNetworkRunner::IsServer()
    {
        RESOLVE_SDK_METHOD(bool (*)(void*), GetIl2cppClass(), "get_IsServer", 0);
        return func(this);
    }

    bool CNetworkRunner::IsClient()
    {
        RESOLVE_SDK_METHOD(bool (*)(void*), GetIl2cppClass(), "get_IsClient", 0);
        return func(this);
    }

    bool CNetworkRunner::IsSharedModeMasterClient()
    {
        RESOLVE_SDK_METHOD(bool (*)(void*), GetIl2cppClass(), "get_IsSharedModeMasterClient", 0);
        return func(this);
    }

    int CNetworkRunner::GetGameMode()
    {
        RESOLVE_SDK_METHOD(int (*)(void*), GetIl2cppClass(), "get_GameMode", 0);
        return func(this);
    }

    CNetworkRunner* CNetworkRunner::GetRunnerForGameObject(usdk::CGameObject* pGameObject)
    {
        RESOLVE_SDK_METHOD(CNetworkRunner* (*)(void*, usdk::CGameObject*), GetIl2cppClass(), "GetRunnerForGameObject", 1);
        return func(nullptr, pGameObject);
    }
}
