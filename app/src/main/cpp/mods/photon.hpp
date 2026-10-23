#pragma once

#include <string>
#include <vector>

// Photon PUN 2 через IL2CPP. Меню (рендер-поток) читает снимок состояния и ставит команды
// в очередь; всё общение с игрой идёт в главном потоке (photon::Tick из mods::Tick).
namespace photon
{
    struct SStats
    {
        bool        bChecked   = false;   // уже пробовали найти PUN в игре
        bool        bPun       = false;   // PUN 2 найден
        bool        bConnected = false;   // подключён и готов (мастер-сервер или комната)
        bool        bInRoom    = false;
        bool        bPrivate   = false;   // комната скрыта из поиска (IsVisible == false)
        bool        bMaster    = false;   // мы мастер-клиент
        int         iPing = -1, iOnline = -1, iRooms = -1, nPlayers = 0, nMaxPlayers = 0;
        std::string sRegion, sRoom, sNick, sUserId;
        std::string sMessage;             // результат последнего действия (для snackbar)
        int         iMessageId = 0;
    };

    SStats GetStats();
    // Предмет для спавна. bLocal — префаб самой игры: спавнится только у тебя (Object.Instantiate);
    // иначе сетевой (PhotonNetwork.Instantiate) — видят все в комнате.
    struct SItem { std::string sName; bool bLocal = false; };

    int    ItemsVersion();                          // меняется после каждого поиска предметов
    void   CopyItems(std::vector<SItem>& out);

    void LeaveRoom();
    void JoinRandomRoom();
    void SetNickName(const char* pszUtf8);
    void SetUserId(const char* pszUtf8);
    void ScanItems();                               // сетевые предметы и префабы игры в памяти
    // Луч спавна: включённый предмет спавнится по курку там, куда упёрся луч правой руки.
    // Работает только в приватной комнате. Пустая строка — выключить.
    void  SetArmedItem(const SItem* pItem);         // nullptr — выключить
    SItem GetArmedItem();                           // sName пустое — луч выключен

    void Tick(float fDt);                           // главный поток игры
}
