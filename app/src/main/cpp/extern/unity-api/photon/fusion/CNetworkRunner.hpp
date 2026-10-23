#pragma once
#include "../../sdk_core.hpp"
#include "../../unity/CComponent.hpp"
#include "SPlayerRef.hpp"
#include "CNetworkObject.hpp"
#include "../../mono/SNullable.hpp"

#include "sdk_fwd.hpp"

namespace photonsdk
{
    class CNetworkRunner : public usdk::CComponent
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType* GetMonoType();

        void Despawn(CNetworkObject* pNetworkObject);
        void Disconnect(SPlayerRef player, uint8_t token);
        CNetworkObject* Spawn(CNetworkObject* pPrefab, mono::SNullable<Vector3> position, mono::SNullable<Quaternion> rotation, mono::SNullable<SPlayerRef> inputAuthority, void* onBeforeSpawned, int32_t flags);
        
        SPlayerRef GetLocalPlayer();
        CNetworkObject* GetPlayerObject(SPlayerRef player);
        
        bool IsServer();
        bool IsClient();
        bool IsSharedModeMasterClient();
        
        int GetGameMode();

        static CNetworkRunner* GetRunnerForGameObject(usdk::CGameObject* pGameObject);
    };
}
