#pragma once

#include <cstdint>
#include "Vector3.hpp"
#include "Vector2.hpp"

namespace usdk
{
    struct SRay 
    {
    public:
        Vector3 m_vOrigin;
        Vector3 m_vDirection;

        SRay() = default;
        ~SRay() = default;

        SRay(Vector3 vOrigin, Vector3 vDirection)
        {
            this->m_vOrigin = vOrigin;
            this->m_vDirection = vDirection;
        }
    };

    class CCollider;

    struct SRaycastHit
    {
    public:
        Vector3   m_vPoint;
        Vector3   m_vNormal;
        uint32_t  m_uFaceID;
        float     m_fDistance;
        Vector2   m_vUV;
        uint64_t  m_uColliderInstanceID;

        SRaycastHit() = default;
        ~SRaycastHit() = default;

        CCollider* GetCollider();
    };
}
