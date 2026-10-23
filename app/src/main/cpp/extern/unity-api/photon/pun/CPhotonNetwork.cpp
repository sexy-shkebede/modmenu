#include "CPhotonNetwork.hpp"
#include "../../il2cpp/CClass.hpp"
#include "../../il2cpp/CObject.hpp"
#include "../../mono/CType.hpp"
#include "../../mono/SString.hpp"
#include "../../mono/SArray.hpp"
#include "../../sdk_core.hpp"

namespace photonsdk
{
    il2cpp::CClass* CPhotonNetwork::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("PhotonNetwork", "Photon.Pun", "PhotonUnityNetworking.dll");
    }

    bool CPhotonNetwork::GetIsConnected()
    {
        RESOLVE_SDK_METHOD(bool (*)(  ), GetIl2cppClass(), "get_IsConnected", 0);
        return func();
    }

    bool CPhotonNetwork::GetIsConnectedAndReady()
    {
        RESOLVE_SDK_METHOD(bool (*)(  ), GetIl2cppClass(), "get_IsConnectedAndReady", 0);
        return func();
    }

    CRoom* CPhotonNetwork::GetCurrentRoom()
    {
        RESOLVE_SDK_METHOD(CRoom* (*)(  ), GetIl2cppClass(), "get_CurrentRoom", 0);
        return func();
    }

    CPhotonPlayer* CPhotonNetwork::GetLocalPlayer()
    {
        RESOLVE_SDK_METHOD(CPhotonPlayer* (*)(  ), GetIl2cppClass(), "get_LocalPlayer", 0);
        return func();
    }

    double CPhotonNetwork::GetServerTime()
    {
        RESOLVE_SDK_METHOD(bool (*)(  ), GetIl2cppClass(), "get_ServerTime", 0);
        return func();
    }

    int CPhotonNetwork::GetServerTimestamp()
    {
        RESOLVE_SDK_METHOD(bool (*)(  ), GetIl2cppClass(), "get_ServerTimestamp", 0);
        return func();
    }

    bool CPhotonNetwork::GetIsMasterClient()
    {
        RESOLVE_SDK_METHOD(bool (*)(  ), GetIl2cppClass(), "get_IsMasterClient", 0);
        return func();
    }

    CPhotonPlayer* CPhotonNetwork::GetMasterClient()
    {
        RESOLVE_SDK_METHOD(CPhotonPlayer* (*)(  ), GetIl2cppClass(), "get_MasterClient", 0);
        return func();
    }

    bool CPhotonNetwork::SetMasterClient(CPhotonPlayer *pPlayer)
    {
        RESOLVE_SDK_METHOD(bool (*)( CPhotonPlayer* ), GetIl2cppClass(), "SetMasterClient", 1);
        return func(pPlayer);
    }

    bool CPhotonNetwork::JoinRandomRoom()
    {
        RESOLVE_SDK_METHOD(bool (*)(  ), GetIl2cppClass(), "JoinRandomRoom", 0);
        return func();
    }

    bool CPhotonNetwork::CloseConnection(CPhotonPlayer* pKickPlayer)
    {
        RESOLVE_SDK_METHOD(bool (*)( CPhotonPlayer* ), GetIl2cppClass(), "CloseConnection", 1);
        return func(pKickPlayer);
    }

    bool CPhotonNetwork::GetInRoom()
    {
        RESOLVE_SDK_METHOD(bool (*)(  ), GetIl2cppClass(), "get_InRoom", 0);
        return func();
    }

    mono::SString* CPhotonNetwork::GetNickName()
    {
        RESOLVE_SDK_METHOD(mono::SString* (*)(  ), GetIl2cppClass(), "get_NickName", 0);
        return func();
    }

    void CPhotonNetwork::SetNickName(const char* pszNickName)
    {
        RESOLVE_SDK_METHOD(mono::SString* (*)( mono::SString* ), GetIl2cppClass(), "set_NickName", 1);
        func(mono::SString::Create(pszNickName));
    }


    void CPhotonNetwork::Disconnect()
    {
        RESOLVE_SDK_METHOD(void (*)(  ), GetIl2cppClass(), "Disconnect", 0);
        func();
    }

    bool CPhotonNetwork::LeaveRoom(bool bBecomeInactive)
    {
        RESOLVE_SDK_METHOD(bool (*)( bool ), GetIl2cppClass(), "LeaveRoom", 1);
        return func(bBecomeInactive);
    }

    void CPhotonNetwork::Destroy(CPhotonView* pPhotonView)
    {
        RESOLVE_SDK_METHOD(void (*)( CPhotonView* ), GetIl2cppClass(), "Destroy", 1);
        func(pPhotonView);
    }

    void CPhotonNetwork::Destroy(usdk::CGameObject* pGameObject)
    {
        RESOLVE_SDK_METHOD(void (*)( usdk::CGameObject* ), GetIl2cppClass(), "Destroy", 1);
        func(pGameObject);
    }

    void CPhotonNetwork::DestroyPlayerObjects(CPhotonPlayer* pPlayer)
    {
        RESOLVE_SDK_METHOD(void (*)( CPhotonPlayer* ), GetIl2cppClass(), "DestroyPlayerObjects", 1);
        func(pPlayer);
    }

    void CPhotonNetwork::DestroyPlayerObjects(int iTargetPlayerId)
    {
        RESOLVE_SDK_METHOD(void (*)( int ), GetIl2cppClass(), "DestroyPlayerObjects", 1);
        func(iTargetPlayerId);
    }

    void CPhotonNetwork::RemoveRPCs(CPhotonPlayer* pPlayer)
    {
        RESOLVE_SDK_METHOD(void (*)( CPhotonPlayer* ), GetIl2cppClass(), "RemoveRPCs", 1);
        func(pPlayer);
    }

    void CPhotonNetwork::RemoveRPCs(CPhotonView* pTargetPhotonView)
    {
        RESOLVE_SDK_METHOD(void (*)( CPhotonView* ), GetIl2cppClass(), "RemoveRPCs", 1);
        func(pTargetPhotonView);
    }

    usdk::CGameObject* CPhotonNetwork::Instantiate(const char* pszPrefabName, Vector3 vOrigin, Quaternion qRotation, uint8_t iGroup, mono::SArray<il2cpp::CObject*>* pData)
    {
        RESOLVE_SDK_METHOD(usdk::CGameObject* (*)( mono::SString*, Vector3, Quaternion, uint8_t group, mono::SArray<il2cpp::CObject*>* ), GetIl2cppClass(), "Instantiate", 5);
        return func(mono::SString::Create(pszPrefabName), vOrigin, qRotation, iGroup, pData);
    }
    
    void CPhotonNetwork::OpRemoveCompleteCacheOfPlayer(int iActorNumber)
    {
        RESOLVE_SDK_METHOD(void (*)( int ), GetIl2cppClass(), "OpRemoveCompleteCacheOfPlayer", 1);
        func(iActorNumber);
    }
}
