#include "unity_bridge.hpp"
#include "il2cpp_util.hpp"

#include <stdint.h>
#include <type_traits>
#include <android/log.h>

#include "unity/common/Quaternion.hpp"

// Структуры передаются в IL2CPP-методы по значению — должны быть "простыми" (как в C#).
static_assert(std::is_trivially_copyable<Vector3>::value,    "Vector3 must be trivially copyable");
static_assert(std::is_trivially_copyable<Quaternion>::value, "Quaternion must be trivially copyable");

namespace
{
    struct SUnity
    {
        il2::SMethod CameraGetMain;           // static Camera Camera.main
        il2::SMethod ComponentGetTransform;   // Transform Component.transform
        il2::SMethod GetComponentInParent2;   // Component Component.GetComponentInParent(Type, bool)
        il2::SMethod GetComponentInParent1;   // Component Component.GetComponentInParent(Type)
        il2::SMethod TransformGetParent;
        il2::SMethod TransformGetPosition;
        il2::SMethod TransformSetPosition;
        il2::SMethod TransformGetRotation;
        il2::SMethod ObjectIsAlive;           // static bool Object.op_Implicit(Object): жив ли объект
        il2::SMethod RigidbodySetVelocity;    // linearVelocity (Unity 6) или velocity
        il2::SMethod TransformGetLossyScale;
        il2::SMethod RigidbodySetDetectCollisions;
        il2::SMethod ColliderSetEnabled;
        void*        pCharacterControllerType = nullptr;
        // поиск префабов
        il2::SMethod FindObjectsOfTypeAll;          // static Object[] Resources.FindObjectsOfTypeAll(Type)
        il2::SMethod GameObjectGetScene;            // Scene GameObject.scene (int handle; 0 — вне сцен)
        il2::SMethod GameObjectGetTransform;
        il2::SMethod GameObjectGetComponentInChildren;   // (Type, bool includeInactive)
        il2::SMethod ObjectGetName, ObjectGetHideFlags;
        il2::SMethod ObjectInstantiate;             // static Object Object.Instantiate(Object, Vector3, Quaternion)
        void*        pGameObjectType = nullptr;
        void*        pCameraType     = nullptr;
        il2::SMethod Raycast;                 // Physics.Raycast(Vector3, Vector3, out RaycastHit, ...)
        int          nRaycastArgs = 0;
        void*        pRigidbodyType = nullptr;// typeof(Rigidbody)
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
        void* pRigidbody = il2::FindClass("UnityEngine", "Rigidbody");

        U.CameraGetMain         = il2::Find(pCamera,    "get_main", 0);
        U.ComponentGetTransform = il2::Find(pComponent, "get_transform", 0);
        U.GetComponentInParent2 = il2::Find(pComponent, "GetComponentInParent", 2, false);
        U.GetComponentInParent1 = il2::Find(pComponent, "GetComponentInParent", 1, false);
        U.TransformGetParent    = il2::Find(pTransform, "get_parent", 0);
        U.TransformGetPosition  = il2::Find(pTransform, "get_position", 0);
        U.TransformSetPosition  = il2::Find(pTransform, "set_position", 1);
        U.TransformGetRotation  = il2::Find(pTransform, "get_rotation", 0);
        U.ObjectIsAlive         = il2::Find(pObject,    "op_Implicit", 1);
        U.RigidbodySetVelocity  = il2::Find(pRigidbody, "set_linearVelocity", 1, false);
        if (!U.RigidbodySetVelocity)
            U.RigidbodySetVelocity = il2::Find(pRigidbody, "set_velocity", 1);
        U.pRigidbodyType = il2::TypeObject(pRigidbody);
        U.TransformGetLossyScale       = il2::Find(pTransform, "get_lossyScale", 0, false);
        U.RigidbodySetDetectCollisions = il2::Find(pRigidbody, "set_detectCollisions", 1, false);
        U.ColliderSetEnabled           = il2::Find(il2::FindClass("UnityEngine", "Collider"), "set_enabled", 1, false);
        U.pCharacterControllerType     = il2::TypeObject(il2::FindClass("UnityEngine", "CharacterController"));

        void* pGameObject = il2::FindClass("UnityEngine", "GameObject");
        U.pGameObjectType        = il2::TypeObject(pGameObject);
        U.pCameraType            = il2::TypeObject(pCamera);
        U.FindObjectsOfTypeAll   = il2::Find(il2::FindClass("UnityEngine", "Resources"), "FindObjectsOfTypeAll", 1, false);
        U.GameObjectGetScene     = il2::Find(pGameObject, "get_scene", 0, false);
        U.GameObjectGetTransform = il2::Find(pGameObject, "get_transform", 0, false);
        U.ObjectGetName          = il2::Find(pObject, "get_name", 0, false);
        U.ObjectGetHideFlags     = il2::Find(pObject, "get_hideFlags", 0, false);
        static const char* const kChildren[] = { "System.Type", "System.Boolean" };
        static const char* const kInstantiate[] = { "UnityEngine.Object", "UnityEngine.Vector3", "UnityEngine.Quaternion" };
        U.GameObjectGetComponentInChildren = il2::FindByParams(pGameObject, "GetComponentInChildren", kChildren, 2);
        U.ObjectInstantiate                = il2::FindByParams(pObject, "Instantiate", kInstantiate, 3);

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

        const bool bOk = U.CameraGetMain && U.ComponentGetTransform && U.TransformGetPosition &&
                         U.TransformSetPosition && U.TransformGetRotation;
        g_iState = bOk ? 1 : -1;
        __android_log_print(ANDROID_LOG_INFO, "shkbd", "unity: bridge %s", bOk ? "ready" : "unavailable (required methods missing)");
        return bOk;
    }

    bool Alive(void* pObj)
    {
        if (!pObj) return false;
        return U.ObjectIsAlive ? U.ObjectIsAlive.Call<bool>(pObj) : true;
    }

    void* RootOf(void* pTransform)
    {
        if (!U.TransformGetParent) return pTransform;
        for (int i = 0; i < 64; i++)
        {
            void* pParent = U.TransformGetParent.Call<void*>(pTransform);
            if (!pParent) break;
            pTransform = pParent;
        }
        return pTransform;
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

namespace
{
    // Компонент нужного типа на камере или выше по иерархии (игрок) — или nullptr
    void* FindInParent(void* pCamera, void* pType)
    {
        if (!pType) return nullptr;
        void* p = nullptr;
        if (U.GetComponentInParent2)      p = U.GetComponentInParent2.Call<void*>(pCamera, pType, false);
        else if (U.GetComponentInParent1) p = U.GetComponentInParent1.Call<void*>(pCamera, pType);
        return Alive(p) ? p : nullptr;
    }
}

bool ubridge::MovePlayerBy(Vector3 delta)
{
    void* pCamera = nullptr; void* pCamTransform = nullptr;
    if (!MainCamera(pCamera, pCamTransform)) return false;

    // Физический игрок (Rigidbody где-то над камерой) или просто корень иерархии камеры
    void* pRigidbody = FindInParent(pCamera, U.pRigidbodyType);
    void* pTarget    = pRigidbody ? U.ComponentGetTransform.Call<void*>(pRigidbody) : RootOf(pCamTransform);
    if (!Alive(pTarget)) return false;

    const Vector3 pos = U.TransformGetPosition.Call<Vector3>(pTarget);
    U.TransformSetPosition.Call<void>(pTarget, Vector3(pos.x + delta.x, pos.y + delta.y, pos.z + delta.z));

    // Гасим скорость, иначе гравитация и инерция тянут вниз и "сносят" движение
    if (pRigidbody && U.RigidbodySetVelocity)
        U.RigidbodySetVelocity.Call<void>(pRigidbody, Vector3(0.f, 0.f, 0.f));
    return true;
}

bool ubridge::MoveLocalPlayer(Vector3 dirCamera, float fDistance)
{
    void* pCamera = nullptr; void* pCamTransform = nullptr;
    if (!MainCamera(pCamera, pCamTransform)) return false;
    const Vector3 dir = Rotate(U.TransformGetRotation.Call<Quaternion>(pCamTransform), dirCamera);
    return MovePlayerBy(Vector3(dir.x * fDistance, dir.y * fDistance, dir.z * fDistance));
}

bool ubridge::SetPlayerCollisions(bool bEnabled)
{
    void* pCamera = nullptr; void* pCamTransform = nullptr;
    if (!MainCamera(pCamera, pCamTransform)) return false;
    bool bAny = false;
    if (U.RigidbodySetDetectCollisions)
        if (void* pRigidbody = FindInParent(pCamera, U.pRigidbodyType))
        {
            U.RigidbodySetDetectCollisions.Call<void>(pRigidbody, bEnabled);
            bAny = true;
        }
    if (U.ColliderSetEnabled)
        if (void* pController = FindInParent(pCamera, U.pCharacterControllerType))
        {
            U.ColliderSetEnabled.Call<void>(pController, bEnabled);
            bAny = true;
        }
    return bAny;
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

bool ubridge::FindLoadedPrefabs(std::vector<SPrefab>& out, void* pSkipType, size_t nMax)
{
    if (!Init() || !U.FindObjectsOfTypeAll || !U.pGameObjectType || !U.GameObjectGetTransform || !U.ObjectGetName)
    {
        __android_log_print(ANDROID_LOG_WARN, "shkbd", "unity: can't list loaded prefabs (methods stripped)");
        return false;
    }
    // Все загруженные GameObject: и в сценах, и ассеты-префабы, и их дочерние объекты
    void*        pArr = U.FindObjectsOfTypeAll.Call<void*>(U.pGameObjectType);
    const size_t n    = il2::ArrayLength(pArr);
    for (size_t i = 0; i < n && out.size() < nMax; i++)
    {
        void* pGo = il2::ArrayData(pArr)[i];
        if (!pGo) continue;
        // Префаб: вне сцен (handle 0), корень своей иерархии, не скрытый служебный объект
        if (U.GameObjectGetScene && U.GameObjectGetScene.Call<int32_t>(pGo) != 0) continue;
        if (U.ObjectGetHideFlags && (U.ObjectGetHideFlags.Call<int32_t>(pGo) & 1)) continue;   // HideInHierarchy
        void* pTransform = U.GameObjectGetTransform.Call<void*>(pGo);
        if (!pTransform || (U.TransformGetParent && U.TransformGetParent.Call<void*>(pTransform))) continue;
        // Без камеры внутри и без pSkipType
        if (U.GameObjectGetComponentInChildren)
        {
            if (U.pCameraType && U.GameObjectGetComponentInChildren.Call<void*>(pGo, U.pCameraType, true)) continue;
            if (pSkipType     && U.GameObjectGetComponentInChildren.Call<void*>(pGo, pSkipType, true))     continue;
        }
        std::string sName = il2::ToUtf8(U.ObjectGetName.Call<void*>(pGo));
        if (sName.empty()) continue;
        const uint32_t h = il2::NewGCHandle(pGo);
        if (!h) continue;   // без handle не держим — никаких голых указателей между кадрами
        out.push_back({ std::move(sName), h });
    }
    return true;
}

bool ubridge::InstantiateLocal(uint32_t hPrefab, Vector3 at)
{
    if (!Init() || !U.ObjectInstantiate) return false;
    void* pPrefab = il2::GCHandleTarget(hPrefab);
    if (!Alive(pPrefab)) return false;   // игра выгрузила префаб
    return U.ObjectInstantiate.Call<void*>(pPrefab, at, Quaternion::Identity()) != nullptr;
}
