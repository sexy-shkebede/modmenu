#include "CPhotonView.hpp"
#include "../../il2cpp/CClass.hpp"
#include "../../il2cpp/CObject.hpp"
#include "../../mono/CType.hpp"
#include "../../mono/SString.hpp"
#include "../../mono/SArray.hpp"
#include "../../sdk_core.hpp"

namespace photonsdk
{
    il2cpp::CClass* CPhotonView::GetIl2cppClass()
    {
        RESOLVE_SDK_IL2CPP_CLASS("PhotonView", "Photon.Pun", "PhotonUnityNetworking.dll");
    }

    bool CPhotonView::GetIsSceneView()
    {
        RESOLVE_SDK_METHOD(bool (*)( CPhotonView* ), this->GetIl2cppClass(), "get_IsSceneView", 0);
        return func(this);
    }

    bool CPhotonView::GetIsRoomView()
    {
        RESOLVE_SDK_METHOD(bool (*)( CPhotonView* ), this->GetIl2cppClass(), "get_IsRoomView", 0);
        return func(this);
    }

    bool CPhotonView::GetIsMine()
    {
        RESOLVE_SDK_METHOD(bool (*)( CPhotonView* ), this->GetIl2cppClass(), "get_IsMine", 0);
        return func(this);
    }

    CPhotonPlayer *CPhotonView::GetOwner()
    {
        RESOLVE_SDK_METHOD(CPhotonPlayer* (*)( CPhotonView* ), this->GetIl2cppClass(), "get_Owner", 0);
        return func(this);
    }

    int CPhotonView::GetOwnerActorNr()
    {
        RESOLVE_SDK_METHOD(int (*)( CPhotonView* ), this->GetIl2cppClass(), "get_OwnerActorNr", 0);
        return func(this);
    }

    int CPhotonView::GetControllerActorNr()
    {
        RESOLVE_SDK_METHOD(int (*)( CPhotonView* ), this->GetIl2cppClass(), "get_ControllerActorNr", 0);
        return func(this);
    }

    void CPhotonView::SetControllerActorNr(int value)
    {
        RESOLVE_SDK_METHOD(void (*)( CPhotonView*, int ), this->GetIl2cppClass(), "set_ControllerActorNr", 1);
        func(this, value);
    }

    int CPhotonView::GetViewID()
    {
        RESOLVE_SDK_METHOD(int (*)( CPhotonView* ), this->GetIl2cppClass(), "get_ViewID", 0);
        return func(this);
    }

    void CPhotonView::SetViewID(int id)
    {
        RESOLVE_SDK_METHOD(void (*)( CPhotonView*, int ), this->GetIl2cppClass(), "set_ViewID", 1);
        func(this, id);
    }

    void CPhotonView::RequestOwnership()
    {
        RESOLVE_SDK_METHOD(void (*)( CPhotonView* ), this->GetIl2cppClass(), "RequestOwnership", 0);
        func(this);
    }

    void CPhotonView::TransferOwnership(CPhotonPlayer *pPlayer)
    {
        RESOLVE_SDK_METHOD(void (*)( CPhotonView*, void * ), this->GetIl2cppClass(), "TransferOwnership", 1);
        func(this, pPlayer);
    }

    void CPhotonView::TransferOwnership(int iActorNr)
    {
        RESOLVE_SDK_METHOD(void (*)( CPhotonView*, int ), this->GetIl2cppClass(), "TransferOwnership", 1);
        func(this, iActorNr);
    }

    void CPhotonView::RPC(const char* pszMethodName, ERpcTarget eTarget, ...)
    {
        // ...
    }
}
