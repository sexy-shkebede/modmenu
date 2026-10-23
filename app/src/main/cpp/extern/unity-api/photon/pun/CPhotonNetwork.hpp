#pragma once

#include "CPhotonView.hpp"
#include "CRoom.hpp"

#include "sdk_fwd.hpp"

namespace photonsdk
{
    class CPhotonNetwork 
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();
        
        static bool                 GetIsConnected();
        static bool                 GetIsConnectedAndReady();
        static CRoom*               GetCurrentRoom();
        static CPhotonPlayer*       GetLocalPlayer();
        static double               GetServerTime();
        static int                  GetServerTimestamp();
        static bool                 GetIsMasterClient();
        static CPhotonPlayer*       GetMasterClient();
        static bool                 SetMasterClient(CPhotonPlayer *pPlayer);
        static bool                 JoinRandomRoom();
        static bool                 CloseConnection(CPhotonPlayer* pKickPlayer);
        static bool                 GetInRoom();
        static mono::SString*       GetNickName();
        static void                 SetNickName(const char* pszNickName);

        static void                 Disconnect();
        static bool                 LeaveRoom(bool bBecomeInactive = true);
        static void                 Destroy(CPhotonView* pPhotonView);
        static void                 Destroy(usdk::CGameObject* pGameObject);
        static void                 DestroyPlayerObjects(CPhotonPlayer* pPlayer);
        static void                 DestroyPlayerObjects(int iTargetPlayerId);
        static void                 RemoveRPCs(CPhotonPlayer* pPlayer);
        static void                 RemoveRPCs(CPhotonView* pTargetPhotonView);
        static usdk::CGameObject*   Instantiate(const char* pszPrefabName, Vector3 vOrigin, Quaternion qRotation, uint8_t iGroup, mono::SArray<il2cpp::CObject*>* pData = nullptr);
        static void                 OpRemoveCompleteCacheOfPlayer(int iActorNumber);
    };
}
