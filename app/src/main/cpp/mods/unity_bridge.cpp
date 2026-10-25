#include "unity_bridge.hpp"
#include "il2cpp_util.hpp"

#include <stdint.h>
#include <type_traits>
#include <android/log.h>

// Структуры передаются в IL2CPP-методы по значению — должны быть "простыми" (как в C#).
static_assert(std::is_trivially_copyable<Vector3>::value,    "Vector3 must be trivially copyable");
static_assert(std::is_trivially_copyable<Quaternion>::value, "Quaternion must be trivially copyable");

namespace
{
    struct SUnity
    {
        il2::SMethod CameraGetMain;           // static Camera Camera.main
        il2::SMethod ComponentGetTransform;   // Transform Component.transform
        il2::SMethod TransformGetParent;
        il2::SMethod TransformGetPosition;
        il2::SMethod TransformGetRotation;
        il2::SMethod TransformGetLossyScale;
        il2::SMethod ObjectIsAlive;           // static bool Object.op_Implicit(Object): жив ли объект
        il2::SMethod Raycast;                 // Physics.Raycast(Vector3, Vector3, out RaycastHit, ...)
        int          nRaycastArgs = 0;
    };
    SUnity U;
    int    g_iState = 0;

    bool Init()
    {
        if (g_iState != 0) return g_iState > 0;
        g_iState = -1;
        if (!il2::Init()) return false;

        void* pObject    = il2::FindClass("UnityEngine", "Object");
        void* pComponent = il2::FindClass("UnityEngine", "Component");
        void* pTransform = il2::FindClass("UnityEngine", "Transform");
        void* pCamera    = il2::FindClass("UnityEngine", "Camera");

        U.CameraGetMain          = il2::Find(pCamera,    "get_main", 0);
        U.ComponentGetTransform  = il2::Find(pComponent, "get_transform", 0);
        U.TransformGetParent     = il2::Find(pTransform, "get_parent", 0);
        U.TransformGetPosition   = il2::Find(pTransform, "get_position", 0);
        U.TransformGetRotation   = il2::Find(pTransform, "get_rotation", 0);
        U.TransformGetLossyScale = il2::Find(pTransform, "get_lossyScale", 0, false);
        U.ObjectIsAlive          = il2::Find(pObject,    "op_Implicit", 1);

        // Перегрузки Raycast с out RaycastHit: берём самую полную, какая уцелела после стриппинга
        static const char* const kRay[] = { "UnityEngine.Vector3", "UnityEngine.Vector3", "UnityEngine.RaycastHit&",
                                            "System.Single", "System.Int32", "UnityEngine.QueryTriggerInteraction" };
        void* pPhysics = il2::FindClass("UnityEngine", "Physics");
        for (int n = 6; n >= 3 && !U.Raycast; n--)
        {
            U.Raycast      = il2::FindByParams(pPhysics, "Raycast", kRay, n);
            U.nRaycastArgs = n;
        }
        if (!U.Raycast) __android_log_print(ANDROID_LOG_WARN, "shkbd", "unity: Physics.Raycast with RaycastHit not found");

        const bool bOk = U.CameraGetMain && U.ComponentGetTransform && U.TransformGetPosition && U.TransformGetRotation;
        g_iState = bOk ? 1 : -1;
        __android_log_print(ANDROID_LOG_INFO, "shkbd", "unity: bridge %s", bOk ? "ready" : "unavailable (required methods missing)");
        return bOk;
    }

    bool Alive(void* pObj)
    {
        if (!pObj) return false;
        return U.ObjectIsAlive ? U.ObjectIsAlive.Call<bool>(pObj) : true;
    }

    Vector3 Rotate(const Quaternion& q, const Vector3& v)
    {
        const float x2 = q.x * 2.f, y2 = q.y * 2.f, z2 = q.z * 2.f;
        const float xx = q.x * x2,  yy = q.y * y2,  zz = q.z * z2;
        const float xy = q.x * y2,  xz = q.x * z2,  yz = q.y * z2;
        const float wx = q.w * x2,  wy = q.w * y2,  wz = q.w * z2;
        return Vector3(v.x * (1.f - (yy + zz)) + v.y * (xy - wz)         + v.z * (xz + wy),
                       v.x * (xy + wz)         + v.y * (1.f - (xx + zz)) + v.z * (yz - wx),
                       v.x * (xz - wy)         + v.y * (yz + wx)         + v.z * (1.f - (xx + yy)));
    }

    float CameraScale(void* pCamTransform)
    {
        if (!U.TransformGetLossyScale) return 1.f;
        const float s = U.TransformGetLossyScale.Call<Vector3>(pCamTransform).y;
        return s > 1e-3f ? s : 1.f;
    }

    // Главная камера игры: объект и её Transform
    bool MainCamera(void*& pCamera, void*& pTransform)
    {
        if (!Init()) return false;
        pCamera = U.CameraGetMain.Call<void*>();
        if (!Alive(pCamera)) return false;
        pTransform = U.ComponentGetTransform.Call<void*>(pCamera);
        return Alive(pTransform);
    }
}

bool ubridge::HeadToWorld(Vector3 posHead, Vector3 dirHead, Vector3& outPos, Vector3& outDir)
{
    void* pCamera = nullptr; void* pCamTransform = nullptr;
    if (!MainCamera(pCamera, pCamTransform)) return false;

    const Quaternion q   = U.TransformGetRotation.Call<Quaternion>(pCamTransform);
    const Vector3    cam = U.TransformGetPosition.Call<Vector3>(pCamTransform);
    const float      s   = CameraScale(pCamTransform);   // трекинг в метрах → единицы мира игры
    const Vector3    off = Rotate(q, Vector3(posHead.x * s, posHead.y * s, posHead.z * s));
    outPos = Vector3(cam.x + off.x, cam.y + off.y, cam.z + off.z);
    outDir = Rotate(q, dirHead);
    return true;
}

bool ubridge::GetRig(Vector3& outPos, Quaternion& outRot, float& outScale, Vector3& outCamPos)
{
    void* pCamera = nullptr; void* pCamTransform = nullptr;
    if (!MainCamera(pCamera, pCamTransform)) return false;

    outCamPos = U.TransformGetPosition.Call<Vector3>(pCamTransform);
    outScale  = CameraScale(pCamTransform);
    void* pParent = U.TransformGetParent ? U.TransformGetParent.Call<void*>(pCamTransform) : nullptr;
    if (Alive(pParent))
    {
        outPos = U.TransformGetPosition.Call<Vector3>(pParent);
        outRot = U.TransformGetRotation.Call<Quaternion>(pParent);
    }
    else
    {
        outPos = Vector3(0.f, 0.f, 0.f);
        outRot = Quaternion(0.f, 0.f, 0.f, 1.f);
    }
    return true;
}

float ubridge::WorldScale()
{
    void* pCamera = nullptr; void* pCamTransform = nullptr;
    return MainCamera(pCamera, pCamTransform) ? CameraScale(pCamTransform) : 1.f;
}

bool ubridge::Raycast(Vector3 origin, Vector3 dir, float fMaxDistance, SHit& out)
{
    if (!Init() || !U.Raycast) return false;

    // UnityEngine.RaycastHit (раскладка стабильна с Unity 2018)
    struct SRaycastHit { Vector3 point; Vector3 normal; uint32_t uFaceId; float fDistance; float uv[2]; int32_t iCollider; };
    static_assert(sizeof(SRaycastHit) == 44, "RaycastHit layout");
    SRaycastHit hit {};
    constexpr int kDefaultLayers  = -5;   // Physics.DefaultRaycastLayers: всё, кроме Ignore Raycast
    constexpr int kIgnoreTriggers = 1;    // QueryTriggerInteraction.Ignore

    bool bHit = false;
    switch (U.nRaycastArgs)
    {
        case 6:  bHit = U.Raycast.Call<bool>(origin, dir, &hit, fMaxDistance, kDefaultLayers, kIgnoreTriggers); break;
        case 5:  bHit = U.Raycast.Call<bool>(origin, dir, &hit, fMaxDistance, kDefaultLayers); break;
        case 4:  bHit = U.Raycast.Call<bool>(origin, dir, &hit, fMaxDistance); break;
        default: bHit = U.Raycast.Call<bool>(origin, dir, &hit); break;
    }
    if (!bHit || hit.fDistance > fMaxDistance) return false;
    out = { hit.point, hit.normal, hit.fDistance };
    return true;
}
