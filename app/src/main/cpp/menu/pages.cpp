#include "pages.hpp"
#include "keyboard.hpp"
#include "settings.hpp"
#include "theme.hpp"
#include "../config.hpp"
#include "../mods/mods.hpp"
#include "../mods/photon.hpp"

#include "imgui.h"

#include <ctype.h>
#include <stdio.h>
#include <string>
#include <vector>

// Страницы рисуются компонентами из ui.hpp. Строки одной группы: First … Middle … Last.
namespace pages
{
    void Movement()
    {
        char szFly[80], szNoclip[80];
        snprintf(szFly,    sizeof(szFly),    "Hold %s to fly where your right hand points", cfg::FlyButtonName());
        snprintf(szNoclip, sizeof(szNoclip), "Hold %s to pass through walls", cfg::FlyButtonName());
        ui::SwitchRow("Fly",       szFly,    &mods::Fly.bEnabled,       ui::ERow::First);
        ui::SwitchRow("Noclip",    szNoclip, &mods::Noclip.bEnabled,    ui::ERow::Middle);
        ui::SwitchRow("Platforms", "Pull a trigger to grab the air with that hand", &mods::Platforms.bEnabled, ui::ERow::Last);
        if (mods::Platforms.bEnabled && !photon::GetArmedItem().sName.empty())
            ui::SupportText("The right trigger belongs to the spawn beam right now, so platforms work on the left hand until you turn the beam off.");
    }

    void Misc()        { ui::EmptyState(ui::EIcon::GridView, "Nothing here yet", "Add features in menu/pages.cpp"); }
    void Overpowered() { ui::EmptyState(ui::EIcon::Bolt,     "Nothing here yet", "Add features in menu/pages.cpp"); }


    void Settings()
    {
        settings::SSettings& s = settings::Get();

        // Кружки-превью пересчитываются, только когда меняется стиль или тёмный/светлый режим
        static unsigned s_aPreview[theme::kSeedCount][3];
        static int      s_iKey = -1;
        const int       iKey   = s.iStyle * 2 + (s.bDark ? 1 : 0);
        if (iKey != s_iKey)
        {
            for (int i = 0; i < theme::kSeedCount; i++)
                theme::Preview(theme::kSeeds[i].rgb, (theme::EStyle)s.iStyle, s.bDark, s_aPreview[i]);
            s_iKey = iKey;
        }

        ui::Section("Theme");
        bool bTheme = false;
        bTheme |= ui::ColorGridRow("Color", theme::kSeeds[s.iSeed].pszName, s_aPreview, theme::kSeedCount, &s.iSeed, ui::ERow::First);
        bTheme |= ui::SegmentedRow("Style", theme::kStyleNames, theme::kStyleCount, &s.iStyle, ui::ERow::Middle);
        bTheme |= ui::SwitchRow("Dark theme", nullptr, &s.bDark, ui::ERow::Last);

        ui::Section("Menu");
        bool bOther = false;
        bOther |= ui::SliderRow("Size", &s.fSizePct, 80.f, 130.f, "%.0f%%", ui::ERow::First);
        bOther |= ui::SwitchRow("Animations", nullptr, &s.bMotion, ui::ERow::Last);

        ui::Section("Fly");
        bOther |= ui::SliderRow("Speed", &mods::Fly.fSpeed, 2.f, 30.f, "%.0f m/s");

        if (bTheme || bOther) settings::Changed(bTheme);
    }

    void Info()
    {
        ui::Section("Made by");
        ui::PersonRow("Claude", "Lead coder",    ui::ERow::First);
        ui::PersonRow("shkbd",  "Prompt master", ui::ERow::Last);

        ui::Section("Based on");
        ui::PersonRow("swesws", "Menu base",      ui::ERow::First);
        ui::PersonRow("pubert", "XrInput system", ui::ERow::Last);

        ui::Section("Libraries");
        ui::PersonRow("Dear ImGui", "UI library by Omar Cornut", ui::ERow::First);
        ui::PersonRow("Dobby",      "Hooking library by jmpews", ui::ERow::Middle);
        ui::PersonRow("Unity-API",  "IL2CPP and Unity wrappers", ui::ERow::Last);
    }

    // ---------------------------------------------------------------- Photon
    namespace
    {
        char                     s_szSearch[33]  = "";
        std::vector<photon::SItem> s_Items;
        int                      s_iItemsVersion = -1;
        int                      s_iShownMessage = 0;
        float                    s_fSnackbar     = 0.f;

        bool Matches(const std::string& s, const char* q)
        {
            if (!*q) return true;
            const size_t n = strlen(q);
            for (size_t i = 0; i + n <= s.size(); i++)
            {
                size_t k = 0;
                while (k < n && tolower((unsigned char)s[i + k]) == tolower((unsigned char)q[k])) k++;
                if (k == n) return true;
            }
            return false;
        }

        ui::ERow RowPos(size_t i, size_t n) { return n == 1 ? ui::ERow::Single : i == 0 ? ui::ERow::First : i + 1 == n ? ui::ERow::Last : ui::ERow::Middle; }

        // Группа предметов одного вида с поиском; тумблер включает предмет для луча спавна
        void ItemGroup(const char* pszTitle, const char* pszNote, bool bLocal, const photon::SItem& armed, bool bCanSpawn)
        {
            constexpr int    kMaxShown = 50;
            std::vector<int> shown;
            int              nTotal = 0, nMatches = 0;
            for (int i = 0; i < (int)s_Items.size(); i++)
            {
                if (s_Items[i].bLocal != bLocal) continue;
                nTotal++;
                if (Matches(s_Items[i].sName, s_szSearch) && ++nMatches <= kMaxShown)
                    shown.push_back(i);
            }

            ui::Section(pszTitle);
            if (!nTotal)
            {
                ui::SupportText(bLocal ? "No game prefabs are loaded right now." : "No network items found.");
                return;
            }
            ui::SupportText(pszNote);
            for (size_t k = 0; k < shown.size(); k++)
            {
                const photon::SItem& item = s_Items[shown[k]];
                bool bOn = item.sName == armed.sName && item.bLocal == armed.bLocal;   // включён только один
                ImGui::PushID(shown[k]);
                if (ui::SwitchRow(item.sName.c_str(), nullptr, &bOn, RowPos(k, shown.size()), bCanSpawn))
                    photon::SetArmedItem(bOn ? &item : nullptr);
                ImGui::PopID();
            }
            char szMore[96];
            if (nMatches > kMaxShown)
            {
                snprintf(szMore, sizeof(szMore), "Showing %d of %d. Use Search to narrow it down.", kMaxShown, nMatches);
                ui::SupportText(szMore);
            }
            else if (nMatches == 0)
                ui::SupportText("Nothing matches this search.");
        }
    }

    void Photon()
    {
        const photon::SStats st = photon::GetStats();
        if (!st.bChecked) { ui::EmptyState(ui::EIcon::Cloud, "Looking for Photon", "Checking the game for PUN 2"); return; }
        if (!st.bPun)     { ui::EmptyState(ui::EIcon::Cloud, "Photon PUN not found", "This game doesn't use PUN 2, or its build strips it"); return; }

        // ---- Сервер: статистика и комната
        char szPing[16] = "-", szPlayers[24] = "-", szOnline[16] = "-", szRooms[16] = "-";
        if (st.iPing >= 0)   snprintf(szPing, sizeof(szPing), "%d ms", st.iPing);
        if (st.iOnline >= 0) snprintf(szOnline, sizeof(szOnline), "%d", st.iOnline);
        if (st.iRooms >= 0)  snprintf(szRooms, sizeof(szRooms), "%d", st.iRooms);
        if (st.bInRoom)
        {
            if (st.nMaxPlayers > 0) snprintf(szPlayers, sizeof(szPlayers), "%d / %d", st.nPlayers, st.nMaxPlayers);
            else                    snprintf(szPlayers, sizeof(szPlayers), "%d", st.nPlayers);
        }
        const ui::SStat stats[] =
        {
            { "Status",        st.bInRoom ? "In room" : (st.bConnected ? "Connected" : "Offline") },
            { "Region",        st.sRegion.empty() ? "-" : st.sRegion.c_str() },
            { st.bInRoom ? (st.bPrivate ? "Room, private" : "Room, public") : "Room", st.bInRoom ? st.sRoom.c_str() : "-" },
            { "Players",       szPlayers },
            { "Ping",          szPing },
            { "Master client", st.bInRoom ? (st.bMaster ? "You" : "Someone else") : "-" },
            { "Online now",    szOnline },
            { "Rooms",         szRooms },
        };
        ui::Section("Server");
        ui::StatGrid(stats, IM_ARRAYSIZE(stats));
        ui::Gap(6);
        switch (ui::ActionsRow("Leave room", "Join random room", st.bInRoom, st.bConnected && !st.bInRoom))
        {
            case 1: photon::LeaveRoom();      break;
            case 2: photon::JoinRandomRoom(); break;
            default: break;
        }

        // ---- Ник и ID
        ui::Section("Identity");
        if (ui::ValueRow("Nickname", st.sNick.empty() ? "Not set" : st.sNick.c_str(), ui::ERow::First))
            keyboard::Open("Nickname", st.sNick.c_str(), 24, [](const char* s) { photon::SetNickName(s); });
        if (ui::ValueRow("User ID", st.sUserId.empty() ? "Not set" : st.sUserId.c_str(), ui::ERow::Last))
            keyboard::Open("User ID", st.sUserId.c_str(), 64, [](const char* s) { photon::SetUserId(s); });
        ui::SupportText("A new user ID is used on the next connection. Games with their own login (PlayFab and similar) assign it themselves.");

        // ---- Предметы: сетевые (видят все в комнате) и префабы самой игры (только у тебя)
        ui::Section("Items");
        if (photon::ItemsVersion() != s_iItemsVersion)
        {
            s_iItemsVersion = photon::ItemsVersion();
            photon::CopyItems(s_Items);
        }
        switch (ui::ActionsRow(s_Items.empty() ? "Find items" : "Find again", s_szSearch[0] ? "Change search" : "Search", true, !s_Items.empty()))
        {
            case 1: photon::ScanItems(); break;
            case 2: keyboard::Open("Search", s_szSearch, 32, [](const char* s) { snprintf(s_szSearch, sizeof(s_szSearch), "%s", s); }); break;
            default: break;
        }

        const bool          bCanSpawn = st.bInRoom && st.bPrivate;
        const photon::SItem armed     = photon::GetArmedItem();
        if (s_Items.empty())
            ui::SupportText("Finds network items and the game's own prefabs. The first search can take a few seconds.");
        else if (!bCanSpawn)
            ui::SupportText("Spawning works only in private rooms.");
        else if (armed.sName.empty())
            ui::SupportText("Turn an item on and a beam comes out of your right hand. Pull the trigger to spawn the item where the beam hits.");
        else
            ui::SupportText("Beam is on. Pull the right trigger to spawn where it hits, or turn the item off.");

        if (!s_Items.empty())
        {
            ItemGroup("Network items", "Spawned through Photon: everyone in the room sees them.", false, armed, bCanSpawn);
            ItemGroup("Game prefabs", "Prefabs the game has loaded. They spawn only on your side: other players don't see them.", true, armed, bCanSpawn);
        }

        // ---- Результат последнего действия — snackbar на 4 секунды
        if (st.iMessageId != s_iShownMessage)
        {
            s_iShownMessage = st.iMessageId;
            s_fSnackbar     = 4.f;
        }
        s_fSnackbar = s_fSnackbar > 0.f ? s_fSnackbar - ImGui::GetIO().DeltaTime : 0.f;
        const float fIn = (4.f - s_fSnackbar) * 5.f, fOut = s_fSnackbar * 4.f;
        ui::Snackbar(st.sMessage.c_str(), fIn < fOut ? (fIn < 1.f ? fIn : 1.f) : (fOut < 1.f ? fOut : 1.f));
    }
}
