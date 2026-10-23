#pragma once

#include "renderer/xr_platform_includes.hpp"
#include <math.h>

namespace xrmath
{

inline XrVector3f RotateVectorByQuaternion(XrQuaternionf q, XrVector3f v)
{
    const float x2 = q.x * 2, y2 = q.y * 2, z2 = q.z * 2;
    const float xx = q.x * x2, yy = q.y * y2, zz = q.z * z2;
    const float xy = q.x * y2, xz = q.x * z2, yz = q.y * z2;
    const float wx = q.w * x2, wy = q.w * y2, wz = q.w * z2;

    return
    {
        v.x * (1 - (yy + zz)) + v.y * (xy - wz)       + v.z * (xz + wy),
        v.x * (xy + wz)       + v.y * (1 - (xx + zz)) + v.z * (yz - wx),
        v.x * (xz - wy)       + v.y * (yz + wx)       + v.z * (1 - (xx + yy))
    };
}

inline XrQuaternionf QuaternionMultiply(XrQuaternionf a, XrQuaternionf b)
{
    return
    {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    };
}

inline XrQuaternionf QuaternionNormalize(XrQuaternionf q)
{
    const float fLenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
    if (fLenSq < 1e-12f) return { 0.f, 0.f, 0.f, 1.f };
    const float fInv = 1.f / sqrtf(fLenSq);
    return { q.x * fInv, q.y * fInv, q.z * fInv, q.w * fInv };
}

inline XrPosef PoseMultiply(XrPosef a, XrPosef b)
{
    a.orientation = QuaternionNormalize(a.orientation);
    b.orientation = QuaternionNormalize(b.orientation);
    const XrVector3f vRotated = RotateVectorByQuaternion(a.orientation, b.position);

    XrPosef out;
    out.orientation = QuaternionNormalize(QuaternionMultiply(a.orientation, b.orientation));
    out.position    = { vRotated.x + a.position.x, vRotated.y + a.position.y, vRotated.z + a.position.z };
    return out;
}

// Пересечение луча с quad-слоем. u — слева направо, v — сверху вниз, оба 0..1.
inline bool RaycastToQuad(XrVector3f vRayOrigin, XrVector3f vRayDir, XrPosef quadPose, float fQuadW, float fQuadH, float& fOutU, float& fOutV)
{
    const XrVector3f    vRel       = { vRayOrigin.x - quadPose.position.x, vRayOrigin.y - quadPose.position.y, vRayOrigin.z - quadPose.position.z };
    const XrQuaternionf qInv       = { -quadPose.orientation.x, -quadPose.orientation.y, -quadPose.orientation.z, quadPose.orientation.w };
    const XrVector3f    vLocalOrig = RotateVectorByQuaternion(qInv, vRel);
    const XrVector3f    vLocalDir  = RotateVectorByQuaternion(qInv, vRayDir);

    if (fabsf(vLocalDir.z) < 1e-6f) return false;
    const float t = -vLocalOrig.z / vLocalDir.z;
    if (t < 0) return false;

    fOutU = (vLocalOrig.x + vLocalDir.x * t) / fQuadW + 0.5f;
    fOutV = 1.f - ((vLocalOrig.y + vLocalDir.y * t) / fQuadH + 0.5f);
    return fOutU >= 0 && fOutU <= 1 && fOutV >= 0 && fOutV <= 1;
}

} // xrmath
