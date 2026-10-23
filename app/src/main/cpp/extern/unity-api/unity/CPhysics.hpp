#pragma once

#include "common/include.hpp"

#include "../sdk_fwd.hpp"

namespace usdk
{
    class CPhysics
    {
    public:

        /*
        enum eQueryTriggerInteraction
        {
        	eUseGlobal = 0,
        	eIgnore = 1,
        	eCollide = 2
        };
        */

        static il2cpp::CClass* GetIl2cppClass();
        static mono::CType*    GetMonoType();

        static Vector3 GetGravity();
        
        static bool    Raycast(Vector3 vOrigin, Vector3 vDirection);
        static bool    Raycast(Vector3 vOrigin, Vector3 vDirection, SRaycastHit* pHit, float fMaxDistance, int iLayerMask = -1);

        static bool    Linecast(Vector3 vStart, Vector3 vEnd);
        static bool    Linecast(Vector3 vStart, Vector3 vEnd, SRaycastHit* pHit, int iLayerMask = -1);

        static int OverlapBoxNonAlloc(Vector3 vCenter, Vector3 vHalfExtents, mono::SArray<CCollider*>* pResults, Quaternion vOrientation, int iLayerMask, int iQueryTriggerInteraction = 0);
    };
} // usdk
