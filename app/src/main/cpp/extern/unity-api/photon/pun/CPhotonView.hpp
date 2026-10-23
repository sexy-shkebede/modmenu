#pragma once

#include "CGameObject.hpp"
#include "CPhotonPlayer.hpp"

#include "sdk_fwd.hpp"

namespace photonsdk
{
    enum class ERpcTarget : int
    {
        eAll = 0,
        eOthers = 1,
        eMasterClient = 2,
        eAllBuffered = 3,
        eOthersBuffered = 4,
        eAllViaServer = 5,
        eBufferedViaServer = 6
    };

    enum class ERpcArg : int
    {
        eInt = 0,
        eBool = 1,
        eFloat = 2,
        eString = 3,
        eVector3 = 4,
        ePlayer = 5,
        eEnd = 6
    };

    class CPhotonView
    {
    public:
        static il2cpp::CClass* GetIl2cppClass();

        bool            GetIsSceneView();
        bool            GetIsRoomView();
        bool            GetIsMine();
        CPhotonPlayer*  GetOwner();
        int             GetOwnerActorNr();
        int             GetControllerActorNr();
        void            SetControllerActorNr(int iValue);
        int             GetViewID();
        void            SetViewID(int iId);

        void            RequestOwnership();
        void            TransferOwnership(CPhotonPlayer *pPlayer);
        void            TransferOwnership(int iActorNr);
        void            RPC(const char* pszMethodName, ERpcTarget eTarget, ...);
    };
}
