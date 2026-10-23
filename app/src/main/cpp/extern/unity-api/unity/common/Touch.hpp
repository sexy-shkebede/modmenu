#pragma once

#include <cstdint>
#include "Vector2.hpp"
#include "Vector3.hpp"

namespace usdk
{
    enum ETouchPhase : uint8_t 
    {
        eBegan = 0,
        eMoved = 1,
        eStationary = 2,
        eEnded = 3,
        eCanceled = 4,
    };

    enum ETouchType : uint8_t 
    {
        eDirect = 0,
        eIndirect = 1,
        eStylus = 2,
    };

#pragma pack(1)
    struct STouch 
    {
    public:
        int32_t m_iFingerID;
        Vector2 m_vPosition;
        Vector2 m_vRawPosition;
        Vector2 m_vPositionDelta;
        float m_fTimeDelta;
        int32_t m_iTapCount;
        ETouchPhase m_phase;
        char pad[0x3];
        ETouchType m_type;
        char pad2[0x3];
        float m_fPressure;
        float m_fMaximumPossiblePressure;
        float m_fRadius;
        float m_fRadiusVariance;
        float m_fAltitudeAngle;
        float m_fAzimuthAngle;

        STouch() = default;
        ~STouch() = default;
    };
#pragma pack()
}
