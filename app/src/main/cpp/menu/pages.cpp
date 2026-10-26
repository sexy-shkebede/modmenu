#include "pages.hpp"
#include "settings.hpp"
#include "theme.hpp"
#include "../fly/brainview.hpp"
#include "../mods/fly.hpp"

#include "imgui.h"

#include <math.h>
#include <stdio.h>
#include <string>

// Страницы рисуются компонентами из ui.hpp. Строки одной группы: First … Middle … Last.
namespace pages
{
    // ---------------------------------------------------------------- Fly
    namespace
    {
        int   s_iShownMessage = 0;
        float s_fSnackbar     = 0.f;

        // 138639 → "138,639"
        std::string Thousands(unsigned long long v)
        {
            std::string s = std::to_string(v);
            for (int i = (int)s.size() - 3; i > 0; i -= 3) s.insert((size_t)i, ",");
            return s;
        }

        // Пометки на картинке мозга: входы, на которые сейчас идёт сигнал, и выходы за порогом
        void BrainMarks(ImDrawList* dl, ImVec2 p0, ImVec2 p1, const flymod::SStatus& st)
        {
            struct SMark { const char* pszGroup; const char* pszLabel; bool bOn; bool bOut; };
            const float* in = st.aIn;
            const float* o  = st.aOut;
            const SMark  aMarks[] = {
                { "obj_L",   "LC10",       in[fly::kObjL]  > 3.f,  false }, { "obj_R",  "LC10",      in[fly::kObjR]  > 3.f, false },
                { "loom_L",  "LPLC2/LC4",  in[fly::kLoomL] > 3.f,  false }, { "loom_R", "LPLC2/LC4", in[fly::kLoomR] > 3.f, false },
                { "puff",    "bristles",   in[fly::kPuff]  > 3.f,  false }, { "sugar",  "sugar GRN", in[fly::kSugar] > 3.f, false },
                { "bitter",  "bitter GRN", in[fly::kBitter] > 3.f, false }, { "hear_A", "JO (ear)",  in[fly::kHearA] > 3.f, false },
                { "odor_L",  "ORN (smell)", in[fly::kOdorL] > 3.f, false }, { "odor_R", "ORN (smell)", in[fly::kOdorR] > 3.f, false },
                { "steer_L", "DNa",        o[fly::kSteerL] > 15.f, true  }, { "steer_R", "DNa",      o[fly::kSteerR] > 15.f, true },
                { "gf",      "DNp01",      o[fly::kGF]     > 25.f, true  }, { "mn9",    "MN9",       o[fly::kMN9]    > 8.f,  true },
                { "groom",   "DNg84/35",   o[fly::kGroom]  > 35.f, true  },
            };
            const float fs = ui::type::BodySmall * 0.85f;
            for (const SMark& m : aMarks)
            {
                ImVec2 uv;
                if (!m.bOn || !fly::BrainViewGroup(m.pszGroup, uv)) continue;
                const ImVec2 c(p0.x + (p1.x - p0.x) * uv.x, p0.y + (p1.y - p0.y) * uv.y);
                const unsigned rgb = m.bOut ? 0xFFFFFF : 0x7FD8FF;   // экран мозга всегда тёмный
                dl->AddCircle(c, ui::dp(9), ui::Col(rgb, 0.9f), 24, ui::dp(1.5f));
                ui::Text(dl, fs, ImVec2(c.x + ui::dp(11), c.y - fs * 0.5f), ui::Col(rgb, 0.9f), m.pszLabel);
            }
            ui::Text(dl, fs, ImVec2(p0.x + ui::dp(8), p1.y - fs - ui::dp(4)), ui::Col(0xFFFFFF, 0.55f), "L");
            ui::Text(dl, fs, ImVec2(p1.x - ui::dp(16), p1.y - fs - ui::dp(4)), ui::Col(0xFFFFFF, 0.55f), "R");
        }

        void Meter(const char* pszName, float fHz, float fFull, float fMark, ui::ERow pos)
        {
            char sz[24];
            snprintf(sz, sizeof(sz), "%.0f Hz", fHz);
            ui::MeterRow(pszName, sz, fHz / fFull, fMark > 0.f ? fMark / fFull : -1.f, fMark > 0.f && fHz > fMark, pos);
        }
    }

    void Fly()
    {
        flymod::StartBrain();
        const fly::SSimInfo   info = fly::SimInfo();
        const flymod::SStatus st   = flymod::GetStatus();
        flymod::SOptions      opt  = flymod::GetOptions();
        char sz[256];

        // ---- Мозг: загрузка, ошибка или статистика
        ui::Section("Brain");
        if (info.eLoad == fly::ELoad::Failed)
        {
            const std::string sHint = flymod::BrainHint();
            snprintf(sz, sizeof(sz), "%s", info.sError.c_str());
            ui::MeterRow("fly_brain.bin not loaded", "", 0.f, -1.f, false, ui::ERow::Single);
            ui::SupportText(sz);
            snprintf(sz, sizeof(sz), "Build it on a PC: python tools/fly/make_brain.py --download. Then: %s and restart the game.", sHint.c_str());
            ui::SupportText(sz);
            return;
        }
        if (info.eLoad != fly::ELoad::Ready)
        {
            snprintf(sz, sizeof(sz), "%.0f%%", info.fProgress * 100.f);
            ui::MeterRow("Unpacking the connectome", sz, info.fProgress, -1.f, false, ui::ERow::Single);
            ui::SupportText("FlyWire v783: 138,639 neurons and 15 million connections. Takes a few seconds.");
            return;
        }

        const std::string sN = Thousands(info.nNeurons), sM = Thousands(info.nConnections), sA = Thousands(info.nActive);
        char szLived[32], szSpeed[32];
        snprintf(szLived, sizeof(szLived), "%.1f s", info.dSimMs / 1000.0);
        snprintf(szSpeed, sizeof(szSpeed), st.bFly && info.fSpeed > 0.f ? "%.1f× real time" : "—", info.fSpeed);
        const ui::SStat aStats[] = {
            { "Neurons", sN.c_str() }, { "Connections", sM.c_str() },
            { "Brain time", szLived }, { "Could run at", szSpeed },
            { "Spiked in 100 ms", st.bFly ? sA.c_str() : "—" }, { "Fly", st.bFly ? st.pszState : "Not spawned" },
        };
        ui::StatGrid(aStats, 6);
        ui::Gap(2);

        // Картинка мозга с пометками
        fly::BrainViewUpdate(ImGui::GetIO().DeltaTime);
        if (ImTextureData* pTex = fly::BrainView())
        {
            ImVec2 p0, p1;
            if (ui::ImageRow(pTex->GetTexRef(), (float)fly::kViewW / (float)fly::kViewH, 0x0D0E12,
                             "All neurons seen from behind. Bright: spiked in the last ~0.2 s. Rings: senses we drive (blue) and commands above threshold (white).",
                             p0, p1, ui::ERow::Single))
                BrainMarks(ImGui::GetWindowDrawList(), p0, p1, st);
        }
        if (st.bFly && info.fSpeed > 0.f && info.fSpeed < 1.f)
            ui::SupportText("The headset can't keep up with real time: the fly lives in slow motion.");

        // ---- Луч правой руки
        ui::Section("Right hand");
        static const char* const kTools[] = { "Off", "Fly", "Sugar", "Lure" };
        int iTool = (int)st.eTool;
        if (ui::SegmentedRow("Beam", kTools, 4, &iTool, ui::ERow::First)) flymod::SetTool((flymod::ETool)iTool);
        if (ui::ActionsRow("Clear sugar", nullptr, st.nDrops > 0, false, ui::ERow::Last) == 1) flymod::ClearSugar();
        static const char* const kHints[] = {
            "The fly lives on its own: walks on floors, walls and ceilings, flies, lands, grooms off dust, searches for food when hungry. Swing at it and looming neurons LPLC2 / LC4 fire the giant fiber; touch it gently and head bristles start grooming.",
            "Point at any surface, even a wall or a ceiling, and pull the trigger. Pull again to move the fly.",
            "Pull the trigger to put sugar drops. The fly sees them with LC10, steers with DNa01 / DNa02 and drinks with its proboscis (MN9).",
            "Hold the trigger and slowly drag the dot near the fly: LC10 neurons follow small moving things.",
        };
        ui::SupportText(kHints[iTool < 0 || iTool > 3 ? 0 : iTool]);

        // ---- Стимулы прямо в сенсорные нейроны (как кнопки экспериментатора в fly-connectome)
        ui::Section("Stimulate neurons");
        const bool bFly = st.bFly && st.eState != flymod::EState::Swatted;
        switch (ui::ActionsRow("Air puff", "Sugar taste", bFly, bFly, ui::ERow::First))
        {
            case 1: flymod::Stimulate(flymod::EStim::Puff);  break;
            case 2: flymod::Stimulate(flymod::EStim::Sugar); break;
            default: break;
        }
        switch (ui::ActionsRow("Loom left", "Loom right", bFly, bFly, ui::ERow::Middle))
        {
            case 1: flymod::Stimulate(flymod::EStim::LoomL); break;
            case 2: flymod::Stimulate(flymod::EStim::LoomR); break;
            default: break;
        }
        switch (ui::ActionsRow("Bitter taste", "Loud clap", bFly, bFly, ui::ERow::Last))
        {
            case 1: flymod::Stimulate(flymod::EStim::Bitter); break;
            case 2: flymod::Stimulate(flymod::EStim::Sound);  break;
            default: break;
        }
        ui::SupportText("Puff: antenna bristles BM_Ant / BM_Fr. Sugar and bitter: taste neurons of the proboscis. Loom: LPLC2 + LC4 of one eye. Clap: Johnston's organ, the fly's ear.");

        // ---- Выходы: средняя частота группы нейронов, риска — порог поведения
        ui::Section("Commands from the brain");
        const float* o = st.aOut;
        Meter("DNa01 / DNa02 left · steering",  o[fly::kSteerL], 60.f,  0.f,  ui::ERow::First);
        Meter("DNa01 / DNa02 right · steering", o[fly::kSteerR], 60.f,  0.f,  ui::ERow::Middle);
        Meter("DNp01 giant fiber · takeoff",    o[fly::kGF],     100.f, 45.f, ui::ERow::Middle);
        Meter("MN9 · proboscis",                o[fly::kMN9],    40.f,  8.f,  ui::ERow::Middle);
        Meter("DNg84 / DNg35 · grooming",       o[fly::kGroom],  150.f, 35.f, ui::ERow::Last);

        // ---- Муха
        ui::Section("Fly");
        snprintf(sz, sizeof(sz), "%.0f%%", st.fHunger * 100.f);
        char szDust[16], szEaten[16], szFlights[16];
        snprintf(szDust, sizeof(szDust), "%.0f%%", st.fDust * 100.f);
        snprintf(szEaten, sizeof(szEaten), "%d", st.nEaten);
        snprintf(szFlights, sizeof(szFlights), "%d", st.nFlights);
        const ui::SStat aFly[] = { { "Hunger", st.bFly ? sz : "—" }, { "Dust on the head", st.bFly ? szDust : "—" },
                                   { "Drops eaten", szEaten }, { "Flights", szFlights },
                                   { "Now", st.bFly ? st.pszState : "Not spawned" }, { "Why", st.bFly ? st.pszCause : "—" } };
        ui::StatGrid(aFly, 6);
        ui::Gap(2);
        switch (ui::ActionsRow("Remove fly", "Reset brain", st.bFly, true, ui::ERow::Single))
        {
            case 1: flymod::RemoveFly();  break;
            case 2: flymod::ResetBrain(); break;
            default: break;
        }

        // ---- Настройки
        ui::Section("Options");
        bool bChanged = false;
        if (ui::SliderRow("Fly size", &opt.fSize, 1.f, 30.f, "%.0f×", ui::ERow::First)) { opt.fSize = roundf(opt.fSize); bChanged = true; }
        bChanged |= ui::SwitchRow("Labels", "What the fly does and which neurons decided it", &opt.bLabels, ui::ERow::Last);
        if (bChanged)
        {
            flymod::SetOptions(opt);
            settings::Changed(false);
        }
        ui::SupportText("1× is a real fruit fly, 2.5 mm. Bigger is easier to see; its world scales with it, so it walks and flies just as much faster.");
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

        ui::Section("Fly brain");
        ui::PersonRow("Shiu et al. 2024", "Whole-brain spiking model, Nature",       ui::ERow::First);
        ui::PersonRow("FlyWire",          "Connectome v783, Dorkenwald et al. 2024", ui::ERow::Middle);
        ui::PersonRow("fly-connectome",   "Senses, readouts and behaviour, XYZ-WEB3", ui::ERow::Middle);
        ui::PersonRow("NeuroMechFly",     "Leg rhythm and grooming the body follows", ui::ERow::Last);

        ui::Section("Libraries");
        ui::PersonRow("Dear ImGui", "UI library by Omar Cornut", ui::ERow::First);
        ui::PersonRow("Dobby",      "Hooking library by jmpews", ui::ERow::Middle);
        ui::PersonRow("Unity-API",  "IL2CPP and Unity wrappers", ui::ERow::Last);
    }
}
