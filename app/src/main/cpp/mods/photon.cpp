#include "photon.hpp"
#include "il2cpp_util.hpp"
#include "unity_bridge.hpp"
#include "../config.hpp"
#include "../openxr_hook/openxr_hook.hpp"

#include <algorithm>
#include <ctype.h>
#include <mutex>
#include <stdint.h>
#include <android/log.h>

#include "unity/common/Quaternion.hpp"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "shkbd", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "shkbd", __VA_ARGS__)

namespace
{
    // ---------------------------------------------------------------- методы PUN
    struct SPun
    {
        // static Photon.Pun.PhotonNetwork
        il2::SMethod IsConnectedAndReady, InRoom, GetPing, CloudRegion, CountOfPlayers, CountOfRooms,
                     CurrentRoom, IsMasterClient, GetNickName, SetNickName, LocalPlayer, GetAuthValues,
                     SetAuthValues, LeaveRoom, JoinRandomRoom, Instantiate, GetPrefabPool;
        // Photon.Realtime.Room / Player / AuthenticationValues
        il2::SMethod RoomName, RoomPlayerCount, RoomMaxPlayers, RoomIsVisible, PlayerUserId, AuthCtor, AuthSetUserId;
        // UnityEngine
        il2::SMethod ResourcesLoadAll, ObjectGetName;
        void* pAuthClass      = nullptr;
        void* pPhotonViewType = nullptr;   // typeof(PhotonView)
        void* pStringClass    = nullptr;
    };
    SPun P;
    int  g_iPunState = 0;   // 0 — не искали, 1 — PUN найден, -1 — нет

    bool InitPun()
    {
        if (g_iPunState != 0) return g_iPunState > 0;
        g_iPunState = -1;

        void* pNet = il2::FindClass("Photon.Pun", "PhotonNetwork");
        if (!pNet)
        {
            LOGW("photon: Photon.Pun.PhotonNetwork not found (game isn't on PUN 2?)");
            return false;
        }
        P.IsConnectedAndReady = il2::Find(pNet, "get_IsConnectedAndReady", 0);
        P.InRoom              = il2::Find(pNet, "get_InRoom", 0);
        P.GetPing             = il2::Find(pNet, "GetPing", 0);
        P.CloudRegion         = il2::Find(pNet, "get_CloudRegion", 0);
        P.CountOfPlayers      = il2::Find(pNet, "get_CountOfPlayers", 0);
        P.CountOfRooms        = il2::Find(pNet, "get_CountOfRooms", 0);
        P.CurrentRoom         = il2::Find(pNet, "get_CurrentRoom", 0);
        P.IsMasterClient      = il2::Find(pNet, "get_IsMasterClient", 0);
        P.GetNickName         = il2::Find(pNet, "get_NickName", 0);
        P.SetNickName         = il2::Find(pNet, "set_NickName", 1);
        P.LocalPlayer         = il2::Find(pNet, "get_LocalPlayer", 0);
        P.GetAuthValues       = il2::Find(pNet, "get_AuthValues", 0);
        P.SetAuthValues       = il2::Find(pNet, "set_AuthValues", 1);
        P.LeaveRoom           = il2::Find(pNet, "LeaveRoom", 1);
        P.JoinRandomRoom      = il2::Find(pNet, "JoinRandomRoom", 0);
        P.Instantiate         = il2::Find(pNet, "Instantiate", 5);
        P.GetPrefabPool       = il2::Find(pNet, "get_PrefabPool", 0);

        void* pRoom   = il2::FindClass("Photon.Realtime", "Room");
        void* pPlayer = il2::FindClass("Photon.Realtime", "Player");
        P.pAuthClass  = il2::FindClass("Photon.Realtime", "AuthenticationValues");
        P.RoomName        = il2::Find(pRoom, "get_Name", 0);
        P.RoomPlayerCount = il2::Find(pRoom, "get_PlayerCount", 0);
        P.RoomMaxPlayers  = il2::Find(pRoom, "get_MaxPlayers", 0);
        P.RoomIsVisible   = il2::Find(pRoom, "get_IsVisible", 0);
        P.PlayerUserId    = il2::Find(pPlayer, "get_UserId", 0);
        P.AuthCtor        = il2::Find(P.pAuthClass, ".ctor", 0);
        P.AuthSetUserId   = il2::Find(P.pAuthClass, "set_UserId", 1);

        P.ResourcesLoadAll = il2::Find(il2::FindClass("UnityEngine", "Resources"), "LoadAll", 2);
        P.ObjectGetName    = il2::Find(il2::FindClass("UnityEngine", "Object"), "get_name", 0);
        P.pPhotonViewType  = il2::TypeObject(il2::FindClass("Photon.Pun", "PhotonView"));
        P.pStringClass     = il2::FindClass("System", "String");

        g_iPunState = (P.IsConnectedAndReady && P.InRoom) ? 1 : -1;
        LOGI("photon: PUN %s", g_iPunState > 0 ? "found" : "found, but core methods are missing");
        return g_iPunState > 0;
    }

    // ---------------------------------------------------------------- обмен с меню
    struct SCmd { int iType; std::string sArg; };
    enum ECmd { kLeave, kJoinRandom, kNick, kUserId, kScan };

    std::mutex               g_Lock;
    photon::SStats           g_Stats;
    std::vector<photon::SItem>    g_Items;
    std::vector<ubridge::SPrefab> g_LocalPrefabs;   // префабы игры с GC-handle (только главный поток)
    int                      g_iItemsVersion = 0;
    std::vector<SCmd>        g_Cmds;
    float                    g_fSinceRefresh = 1.f;
    photon::SItem            g_Armed;               // предмет луча спавна (пишет меню)
    bool                     g_bGunOn      = false;
    bool                     g_bGunTrigger = false;

    void Push(int iType, const char* psz = nullptr)
    {
        std::lock_guard<std::mutex> lock(g_Lock);
        g_Cmds.push_back({ iType, psz ? psz : "" });
    }

    void Message(const std::string& s)
    {
        std::lock_guard<std::mutex> lock(g_Lock);
        g_Stats.sMessage = s;
        g_Stats.iMessageId++;
    }

    // ---------------------------------------------------------------- состояние
    void* CurrentRoom() { return P.CurrentRoom && P.InRoom.Call<bool>() ? P.CurrentRoom.Call<void*>() : nullptr; }

    bool RoomIsPrivate(void* pRoom)
    {
        // Приватная комната — скрытая из поиска (так делают комнаты "по коду")
        return pRoom && P.RoomIsVisible && !P.RoomIsVisible.Call<bool>(pRoom);
    }

    void Refresh()
    {
        photon::SStats s;
        s.bChecked   = true;
        s.bPun       = true;
        s.bConnected = P.IsConnectedAndReady.Call<bool>();
        s.bInRoom    = P.InRoom.Call<bool>();
        if (P.GetPing && s.bConnected) s.iPing   = P.GetPing.Call<int>();
        if (P.CountOfPlayers)          s.iOnline = P.CountOfPlayers.Call<int>();
        if (P.CountOfRooms)            s.iRooms  = P.CountOfRooms.Call<int>();
        if (P.CloudRegion)
        {
            s.sRegion = il2::ToUtf8(P.CloudRegion.Call<void*>());
            s.sRegion = s.sRegion.substr(0, s.sRegion.find('/'));   // "eu/*" → "eu"
        }
        if (void* pRoom = CurrentRoom())
        {
            if (P.RoomName)        s.sRoom       = il2::ToUtf8(P.RoomName.Call<void*>(pRoom));
            if (P.RoomPlayerCount) s.nPlayers    = P.RoomPlayerCount.Call<uint8_t>(pRoom);
            if (P.RoomMaxPlayers)  s.nMaxPlayers = P.RoomMaxPlayers.Call<uint8_t>(pRoom);
            s.bPrivate = RoomIsPrivate(pRoom);
            if (P.IsMasterClient) s.bMaster = P.IsMasterClient.Call<bool>();
        }
        if (P.GetNickName) s.sNick = il2::ToUtf8(P.GetNickName.Call<void*>());
        if (P.LocalPlayer && P.PlayerUserId)
            if (void* pPlayer = P.LocalPlayer.Call<void*>())
                s.sUserId = il2::ToUtf8(P.PlayerUserId.Call<void*>(pPlayer));

        std::lock_guard<std::mutex> lock(g_Lock);
        s.sMessage   = g_Stats.sMessage;
        s.iMessageId = g_Stats.iMessageId;
        g_Stats      = s;
    }

    // ---------------------------------------------------------------- предметы
    // Точные имена префабов из кэша DefaultPool: там всё, что уже спавнилось в этой сессии
    // (включая пути вроде "Items/Sword", которые по одному имени не найти).
    std::vector<std::string> PoolCacheKeys()
    {
        std::vector<std::string> out;
        void* pPool = P.GetPrefabPool ? P.GetPrefabPool.Call<void*>() : nullptr;
        void* pDict = nullptr;
        if (!pPool || !il2::GetField(pPool, "ResourceCache", &pDict) || !pDict || !P.pStringClass) return out;

        void*              pDictClass = il2::ObjectClass(pDict);
        const il2::SMethod count      = il2::Find(pDictClass, "get_Count", 0, false);
        const il2::SMethod keys       = il2::Find(pDictClass, "get_Keys", 0, false);
        if (!count || !keys) return out;
        const int n     = count.Call<int>(pDict);
        void*     pKeys = keys.Call<void*>(pDict);
        if (n <= 0 || !pKeys) return out;
        const il2::SMethod copyTo = il2::Find(il2::ObjectClass(pKeys), "CopyTo", 2, false);
        void*              pArr   = il2::NewArray(P.pStringClass, (size_t)n);
        if (!copyTo || !pArr) return out;
        copyTo.Call<void>(pKeys, pArr, 0);
        for (size_t i = 0; i < il2::ArrayLength(pArr); i++)
            out.push_back(il2::ToUtf8(il2::ArrayData(pArr)[i]));
        return out;
    }

    std::string Lower(std::string s) { for (char& c : s) c = (char)tolower((unsigned char)c); return s; }

    void ScanItems()
    {
        // 1) Все префабы с PhotonView в Resources — PhotonNetwork.Instantiate берёт их оттуда
        std::vector<std::string> names;
        if (P.ResourcesLoadAll && P.pPhotonViewType && P.ObjectGetName)
            if (void* pArr = P.ResourcesLoadAll.Call<void*>(il2::NewString(""), P.pPhotonViewType))
                for (size_t i = 0; i < il2::ArrayLength(pArr); i++)
                    if (void* pObj = il2::ArrayData(pArr)[i])
                        names.push_back(il2::ToUtf8(P.ObjectGetName.Call<void*>(pObj)));

        // 2) Кэш пула: заменяем "голые" имена точными путями и добавляем недостающие
        std::vector<std::string> exact;
        try { exact = PoolCacheKeys(); } catch (...) { LOGW("photon: prefab pool cache unreadable"); }
        for (const std::string& e : exact)
        {
            const std::string leaf = e.substr(e.rfind('/') == std::string::npos ? 0 : e.rfind('/') + 1);
            auto it = std::find(names.begin(), names.end(), leaf);
            if (it != names.end()) *it = e;
            else                   names.push_back(e);
        }

        const auto ByName = [](const std::string& a, const std::string& b) { return Lower(a) < Lower(b); };
        std::sort(names.begin(), names.end(), ByName);
        names.erase(std::unique(names.begin(), names.end()), names.end());
        names.erase(std::remove(names.begin(), names.end(), std::string()), names.end());

        // 3) Префабы самой игры в памяти (без PhotonView и без камеры) — спавнятся локально
        for (const ubridge::SPrefab& pf : g_LocalPrefabs) il2::FreeGCHandle(pf.hObject);
        g_LocalPrefabs.clear();
        try { ubridge::FindLoadedPrefabs(g_LocalPrefabs, P.pPhotonViewType, 1500); }
        catch (...) { LOGW("photon: loaded prefab scan failed"); }
        std::sort(g_LocalPrefabs.begin(), g_LocalPrefabs.end(),
                  [&](const ubridge::SPrefab& a, const ubridge::SPrefab& b) { return ByName(a.sName, b.sName); });

        // Повторы имён и то, что уже есть в сетевом списке, убираем (handle освобождаем)
        const auto InNetwork = [&](const std::string& n)
        {
            for (const std::string& s : names)
                if (s == n || (s.size() > n.size() && s[s.size() - n.size() - 1] == '/' && s.compare(s.size() - n.size(), n.size(), n) == 0))
                    return true;
            return false;
        };
        std::vector<photon::SItem>    items;
        std::vector<ubridge::SPrefab> local;
        for (const std::string& s : names) items.push_back({ s, false });
        for (const ubridge::SPrefab& pf : g_LocalPrefabs)
        {
            if ((!local.empty() && local.back().sName == pf.sName) || InNetwork(pf.sName))
            {
                il2::FreeGCHandle(pf.hObject);
                continue;
            }
            items.push_back({ pf.sName, true });
            local.push_back(pf);
        }
        g_LocalPrefabs.swap(local);

        const size_t nNetwork = names.size(), nLocal = g_LocalPrefabs.size();
        {
            std::lock_guard<std::mutex> lock(g_Lock);
            g_Items.swap(items);
            g_iItemsVersion++;
        }
        Message("Found " + std::to_string(nNetwork) + " network items and " + std::to_string(nLocal) + " game prefabs");
    }

    void SpawnAt(const std::string& sName, const Vector3& at)
    {
        if (!P.Instantiate) { Message("PhotonNetwork.Instantiate is missing in this game"); return; }
        void* pObj = P.Instantiate.Call<void*>(il2::NewString(sName.c_str()), at, Quaternion::Identity(), (uint8_t)0, (void*)nullptr);
        Message(pObj ? "Spawned " + sName : sName + " couldn't be spawned");
    }

    void SpawnLocal(const std::string& sName, const Vector3& at)
    {
        for (const ubridge::SPrefab& pf : g_LocalPrefabs)
            if (pf.sName == sName)
            {
                Message(ubridge::InstantiateLocal(pf.hObject, at) ? "Spawned " + sName + " (only you see it)"
                                                                  : sName + " is no longer loaded, find items again");
                return;
            }
        Message(sName + " is no longer loaded, find items again");
    }

    void GunOff()
    {
        if (!g_bGunOn) return;
        g_bGunOn      = false;
        g_bGunTrigger = false;
        if (g_pOpenXRHook)
        {
            g_pOpenXRHook->SetSpawnBeam(false, -1.f);
            g_pOpenXRHook->m_Input.m_bMaskRightTrigger.store(false, std::memory_order_relaxed);
        }
    }

    Vector3 Mad(const Vector3& a, const Vector3& b, float s) { return Vector3(a.x + b.x * s, a.y + b.y * s, a.z + b.z * s); }

    // Луч из правой руки: raycast в мире игры, расстояние — хуку (он рисует луч и прицел),
    // по нажатию курка — спавн в точке попадания.
    void TickSpawnGun()
    {
        photon::SItem armed;
        {
            std::lock_guard<std::mutex> lock(g_Lock);
            armed = g_Armed;
        }
        COpenXRHook* pHook = g_pOpenXRHook;
        if (armed.sName.empty() || !pHook) { GunOff(); return; }

        // Только в приватной комнате: вышли из неё — луч выключается сам
        if (!RoomIsPrivate(CurrentRoom()))
        {
            {
                std::lock_guard<std::mutex> lock(g_Lock);
                g_Armed = photon::SItem();
            }
            Message("Spawn beam off: you're not in a private room");
            GunOff();
            return;
        }

        XrVector3f dirHead, posHead;
        Vector3    hand, dir;
        if (!pHook->GetRightAimInHead(dirHead, &posHead) ||
            !ubridge::HeadToWorld(Vector3(posHead.x, posHead.y, -posHead.z), Vector3(dirHead.x, dirHead.y, -dirHead.z), hand, dir))
        {
            pHook->SetSpawnBeam(false, -1.f);
            return;
        }

        // Луч стартует в 5 см перед рукой — коллайдеры своих рук не мешают
        const float   s      = ubridge::WorldScale();
        const Vector3 origin = Mad(hand, dir, 0.05f * s);
        ubridge::SHit hit {};
        const bool    bHit   = ubridge::Raycast(origin, dir, cfg::kSpawnBeamMaxM * s, hit);
        pHook->SetSpawnBeam(true, bHit ? hit.fDistance / s : -1.f);
        g_bGunOn = true;

        // Курок забираем у игры, пока луч включён и не указывает на меню
        const bool  bOnMenu  = pHook->IsPointerOnMenu();
        pHook->m_Input.m_bMaskRightTrigger.store(!bOnMenu, std::memory_order_relaxed);
        const float fTrigger = pHook->m_Input.Right.TriggerValue();
        const bool  bDown    = g_bGunTrigger ? fTrigger > cfg::kTriggerRelease : fTrigger >= cfg::kTriggerPress;
        if (bDown && !g_bGunTrigger && !bOnMenu)
        {
            // Чуть от поверхности — не застрянет в стене. Сетевой — через Photon, префаб игры — локально.
            if (bHit) (armed.bLocal ? SpawnLocal : SpawnAt)(armed.sName, Mad(hit.point, hit.normal, 0.15f * s));
            else      Message("Point the beam at a surface");
        }
        g_bGunTrigger = bDown;
    }

    void Execute(const SCmd& c)
    {
        switch (c.iType)
        {
        case kLeave:
            if (!P.InRoom.Call<bool>() || !P.LeaveRoom) { Message("Not in a room"); break; }
            Message(P.LeaveRoom.Call<bool>(true) ? "Leaving the room" : "Couldn't leave the room");
            break;
        case kJoinRandom:
            if (!P.IsConnectedAndReady.Call<bool>() || !P.JoinRandomRoom) { Message("Not connected to Photon"); break; }
            if (P.InRoom.Call<bool>()) { Message("Leave the room first"); break; }
            Message(P.JoinRandomRoom.Call<bool>() ? "Joining a random room" : "Couldn't join a random room");
            break;
        case kNick:
            if (c.sArg.empty() || !P.SetNickName) { Message("Nickname can't be empty"); break; }
            P.SetNickName.Call<void>(il2::NewString(c.sArg.c_str()));
            Message("Nickname set to " + c.sArg);
            break;
        case kUserId:
        {
            if (c.sArg.empty() || !P.GetAuthValues || !P.SetAuthValues || !P.AuthSetUserId) { Message("Can't set user ID in this game"); break; }
            void* pAuth = P.GetAuthValues.Call<void*>();
            if (!pAuth && P.AuthCtor && (pAuth = il2::NewObject(P.pAuthClass)) != nullptr)
            {
                P.AuthCtor.Call<void>(pAuth);
                P.SetAuthValues.Call<void>(pAuth);
            }
            if (!pAuth) { Message("Can't set user ID in this game"); break; }
            P.AuthSetUserId.Call<void>(pAuth, il2::NewString(c.sArg.c_str()));
            Message("User ID will be used on the next connection");
            break;
        }
        case kScan:  ScanItems();     break;
        }
    }
}

// ---------------------------------------------------------------- API для меню
photon::SStats photon::GetStats()                     { std::lock_guard<std::mutex> l(g_Lock); return g_Stats; }
int            photon::ItemsVersion()                 { std::lock_guard<std::mutex> l(g_Lock); return g_iItemsVersion; }
void           photon::CopyItems(std::vector<SItem>& out) { std::lock_guard<std::mutex> l(g_Lock); out = g_Items; }
void photon::LeaveRoom()               { Push(kLeave); }
void photon::JoinRandomRoom()          { Push(kJoinRandom); }
void photon::SetNickName(const char* s){ Push(kNick, s); }
void photon::SetUserId(const char* s)  { Push(kUserId, s); }
void photon::ScanItems()               { Push(kScan); }
void photon::SetArmedItem(const SItem* pItem)
{
    std::lock_guard<std::mutex> lock(g_Lock);
    g_Armed = pItem ? *pItem : SItem();
    g_Stats.sMessage = g_Armed.sName.empty() ? "Spawn beam off"
                     : "Beam on: pull the right trigger to spawn " + g_Armed.sName + (g_Armed.bLocal ? " (only you will see it)" : "");
    g_Stats.iMessageId++;
}
photon::SItem photon::GetArmedItem()   { std::lock_guard<std::mutex> l(g_Lock); return g_Armed; }

void photon::Tick(float fDt)
{
    std::vector<SCmd> cmds;
    bool              bArmed = false;
    {
        std::lock_guard<std::mutex> lock(g_Lock);
        cmds.swap(g_Cmds);
        bArmed = !g_Armed.sName.empty();
    }
    // Меню закрыто, команд нет и луч выключен — игру не трогаем
    const bool bMenu = g_pOpenXRHook && g_pOpenXRHook->IsMenuOpen();
    if (cmds.empty() && !bMenu && !bArmed) { GunOff(); return; }

    if (!InitPun())
    {
        {
            std::lock_guard<std::mutex> lock(g_Lock);
            g_Stats.bChecked = true;
            g_Stats.bPun     = false;
        }
        GunOff();
        return;
    }
    for (const SCmd& c : cmds) Execute(c);
    TickSpawnGun();

    // Статистика — дважды в секунду, пока меню открыто, и сразу после действий
    g_fSinceRefresh += fDt;
    if (!cmds.empty() || (bMenu && g_fSinceRefresh >= 0.5f))
    {
        g_fSinceRefresh = 0.f;
        Refresh();
    }
}
