#include "pages.hpp"
#include "settings.hpp"
#include "theme.hpp"
#include "../config.hpp"
#include "../minecraft/blocks.hpp"
#include "../minecraft/draw.hpp"
#include "../mods/minecraft.hpp"

#include "imgui.h"

#include <math.h>
#include <stdio.h>
#include <string>
#include <vector>

// Страницы рисуются компонентами из ui.hpp. Строки одной группы: First … Middle … Last.
namespace pages
{
    // ---------------------------------------------------------------- Minecraft
    namespace
    {
        int   s_iShownMessage = 0;
        float s_fSnackbar     = 0.f;

        // Блоки по категориям — в порядке каталога
        const std::vector<int>& CategoryBlocks(int iCat)
        {
            static std::vector<int> s_aCats[(int)mc::ECategory::Count];
            static bool             s_bReady = false;
            if (!s_bReady)
            {
                for (int i = 0; i < mc::BlockCount(); i++) s_aCats[(int)mc::Block(i).eCat].push_back(i);
                s_bReady = true;
            }
            return s_aCats[iCat];
        }

        void BlockIcon(ImDrawList* dl, int i, ImVec2 c, float fSize, void* pUser)
        {
            const int iType = pUser ? (*static_cast<const std::vector<int>*>(pUser))[i] : i;
            mc::DrawIcon(dl, mc::ContextAtlas(), iType, c, fSize);
        }

        const char* BlockName(int i, void* pUser)
        {
            return mc::Block((*static_cast<const std::vector<int>*>(pUser))[i]).pszName;
        }
    }

    void Minecraft()
    {
        const minecraft::SStatus st  = minecraft::GetStatus();
        minecraft::SOptions      opt = minecraft::GetOptions();
        const bool               bHand = st.iSelected >= 0;
        const char*              pszModeBtn = cfg::ModeButtonName();

        // ---- В руке: блок, режим, отмена
        char szHint[160];
        if (!bHand)
            snprintf(szHint, sizeof(szHint), "Pick a block below, point your right hand and pull the trigger.");
        else if (st.eMode == minecraft::EMode::Place)
            snprintf(szHint, sizeof(szHint), "Pull the right trigger to place it.%s%s%s", pszModeBtn ? " " : "", pszModeBtn ? pszModeBtn : "", pszModeBtn ? " switches to Break." : "");
        else
            snprintf(szHint, sizeof(szHint), "Pull the right trigger to break your blocks.%s%s%s", pszModeBtn ? " " : "", pszModeBtn ? pszModeBtn : "", pszModeBtn ? " switches to Place." : "");

        ui::Section("In hand");
        if (ui::IconRow(bHand ? mc::Block(st.iSelected).pszName : "Empty hand", szHint, BlockIcon, st.iSelected, nullptr, true, bHand, ui::ERow::First))
            minecraft::Select(-1);
        static const char* const kModes[] = { "Place", "Break" };
        int iMode = (int)st.eMode;
        if (ui::SegmentedRow("Mode", kModes, 2, &iMode, ui::ERow::Middle))
            minecraft::SetMode((minecraft::EMode)iMode);
        switch (ui::ActionsRow("Undo", "Clear all", st.bCanUndo, st.nBlocks > 0, ui::ERow::Last))
        {
            case 1: minecraft::Undo();     break;
            case 2: minecraft::ClearAll(); break;
            default: break;
        }

        char szCount[200];
        const char* pszSolid = !opt.bSolid           ? ""
                             : !st.bSolidSupported   ? " Solid blocks don't work here: the game strips the Unity methods they need."
                             : st.bPublicRoom        ? " Public room: blocks aren't solid here."
                             :                         "";
        snprintf(szCount, sizeof(szCount), "%d of %d blocks. Only you see them.%s", st.nBlocks, cfg::kMaxBlocks, pszSolid);
        ui::SupportText(szCount);

        // ---- Блоки: сетки по категориям, повторное нажатие убирает блок из руки
        for (int iCat = 0; iCat < (int)mc::ECategory::Count; iCat++)
        {
            const std::vector<int>& blocks = CategoryBlocks(iCat);
            if (blocks.empty()) continue;
            int iSel = -1;
            for (int i = 0; i < (int)blocks.size(); i++) if (blocks[i] == st.iSelected) iSel = i;

            ui::Section(mc::CategoryName((mc::ECategory)iCat));
            ImGui::PushID(iCat);
            const int iPressed = ui::TileGrid("##blocks", (int)blocks.size(), iSel, BlockIcon, BlockName,
                                              const_cast<std::vector<int>*>(&blocks), true);
            ImGui::PopID();
            if (iPressed >= 0) minecraft::Select(blocks[iPressed] == st.iSelected ? -1 : blocks[iPressed]);
        }

        // ---- Настройки блоков
        ui::Section("Options");
        bool bChanged = false;
        if (ui::SliderRow("Block size", &opt.fSize, 0.1f, 1.f, "%.2f m", ui::ERow::First))
        {
            opt.fSize = roundf(opt.fSize * 20.f) / 20.f;   // шаг 5 см
            bChanged  = true;
        }
        bChanged |= ui::SwitchRow("Snap to grid", "New blocks line up on a grid of their size", &opt.bSnap, ui::ERow::Middle);
        bChanged |= ui::SwitchRow("Solid blocks", "Stand on blocks and climb them. Off in public rooms", &opt.bSolid, ui::ERow::Middle, st.bSolidSupported);
        bChanged |= ui::SwitchRow("Hide behind walls", "Blocks fade out when the game's walls cover them", &opt.bHideBehind, ui::ERow::Middle);
        bChanged |= ui::SliderRow("Brightness", &opt.fBrightness, 30.f, 100.f, "%.0f%%", ui::ERow::Last);
        if (bChanged)
        {
            minecraft::SetOptions(opt);
            settings::Changed(false);
        }
        ui::Gap(56);   // место под snackbar

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

    // ---------------------------------------------------------------- Settings
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

        if (bTheme || bOther) settings::Changed(bTheme);
    }

    // ---------------------------------------------------------------- Info
    void Info()
    {
        ui::Section("Made by");
        ui::PersonRow("Claude", "Lead coder",    ui::ERow::First);
        ui::PersonRow("shkbd",  "Prompt master", ui::ERow::Last);

        ui::Section("Based on");
        ui::PersonRow("swesws", "Menu base",      ui::ERow::First);
        ui::PersonRow("pubert", "XrInput system", ui::ERow::Last);

        ui::Section("Block textures");
        ui::PersonRow("XSSheep",     "Pixel Perfection, CC BY-SA 4.0",        ui::ERow::First);
        ui::PersonRow("Nova Wostra", "Pixel Perfection Legacy, CC BY-SA 4.0", ui::ERow::Middle);
        ui::PersonRow("Mineclonia",  "Texture set the blocks come from",      ui::ERow::Last);

        ui::Section("Libraries");
        ui::PersonRow("Dear ImGui", "UI library by Omar Cornut", ui::ERow::First);
        ui::PersonRow("Dobby",      "Hooking library by jmpews", ui::ERow::Middle);
        ui::PersonRow("Unity-API",  "IL2CPP and Unity wrappers", ui::ERow::Last);
    }
}
