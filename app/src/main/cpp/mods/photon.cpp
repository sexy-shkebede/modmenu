#include "photon.hpp"
#include "il2cpp_util.hpp"

#include <android/log.h>

namespace
{
    struct SPun
    {
        il2::SMethod InRoom, CurrentRoom;   // static Photon.Pun.PhotonNetwork
        il2::SMethod RoomIsVisible;         // Photon.Realtime.Room
    };
    SPun P;
    int  g_iPunState = 0;   // 0 — не искали, 1 — PUN найден, -1 — нет, -2 — есть, но комнату не узнать

    bool InitPun()
    {
        if (g_iPunState != 0) return g_iPunState > 0;
        g_iPunState = -1;

        void* pNet = il2::FindClass("Photon.Pun", "PhotonNetwork");
        if (!pNet)
        {
            __android_log_print(ANDROID_LOG_INFO, "shkbd", "photon: Photon.Pun.PhotonNetwork not found (game isn't on PUN 2?)");
            return false;
        }
        P.InRoom        = il2::Find(pNet, "get_InRoom", 0);
        P.CurrentRoom   = il2::Find(pNet, "get_CurrentRoom", 0);
        P.RoomIsVisible = il2::Find(il2::FindClass("Photon.Realtime", "Room"), "get_IsVisible", 0);
        g_iPunState = (P.InRoom && P.CurrentRoom && P.RoomIsVisible) ? 1 : -2;
        __android_log_print(ANDROID_LOG_INFO, "shkbd", "photon: PUN %s", g_iPunState > 0 ? "found" : "found, but room methods are missing");
        return g_iPunState > 0;
    }
}

photon::ERoom photon::GetRoom()
{
    // PUN есть, но тип комнаты не узнать — считаем публичной, как и раньше в меню
    if (!InitPun()) return g_iPunState == -2 ? ERoom::Public : ERoom::NoPun;
    if (!P.InRoom.Call<bool>()) return ERoom::NotInRoom;
    void* pRoom = P.CurrentRoom.Call<void*>();
    if (!pRoom) return ERoom::NotInRoom;
    return P.RoomIsVisible.Call<bool>(pRoom) ? ERoom::Public : ERoom::Private;
}
