#include "settings.hpp"
#include "theme.hpp"
#include "ui.hpp"
#include "../mods/minecraft.hpp"

#include "imgui.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <android/log.h>

namespace
{
    settings::SSettings g_S;
    bool  g_bDirty       = false;
    float g_fScale       = 1.f;   // сглаженный размер, который читает хук
    float g_fTargetScale = 1.f;
    char  g_szPath[256]  = {};
    bool  g_bNoPath      = false;

    const char* Path()
    {
        if (g_szPath[0] || g_bNoPath) return g_bNoPath ? nullptr : g_szPath;

        // Имя пакета — из /proc/self/cmdline ("com.company.game" или "com.company.game:процесс")
        char szPkg[128] = {};
        if (FILE* f = fopen("/proc/self/cmdline", "rb"))
        {
            const size_t n = fread(szPkg, 1, sizeof(szPkg) - 1, f);
            szPkg[n] = 0;
            fclose(f);
        }
        if (char* pColon = strchr(szPkg, ':')) *pColon = 0;
        if (!szPkg[0] || strchr(szPkg, '/'))   // не пакет Android (например, тест на ПК)
        {
            g_bNoPath = true;
            return nullptr;
        }
        char szDir[192];
        snprintf(szDir, sizeof(szDir), "/data/data/%s/files", szPkg);
        mkdir(szDir, 0700);
        snprintf(g_szPath, sizeof(g_szPath), "%s/shkbd_menu.cfg", szDir);
        return g_szPath;
    }

    void Save()
    {
        const char* pszPath = Path();
        if (!pszPath) return;
        FILE* f = fopen(pszPath, "w");
        if (!f)
        {
            __android_log_print(ANDROID_LOG_WARN, "shkbd", "settings: can't write %s", pszPath);
            return;
        }
        const minecraft::SOptions mc = minecraft::GetOptions();
        fprintf(f, "seed=%d\nstyle=%d\ndark=%d\nsize=%.0f\nmotion=%d\n"
                   "mc_size=%.2f\nmc_snap=%d\nmc_solid=%d\nmc_hide=%d\nmc_bright=%.0f\n",
                g_S.iSeed, g_S.iStyle, (int)g_S.bDark, g_S.fSizePct, (int)g_S.bMotion,
                mc.fSize, (int)mc.bSnap, (int)mc.bSolid, (int)mc.bHideBehind, mc.fBrightness);
        fclose(f);
    }

    void ApplyTheme(bool bAnimate)
    {
        theme::Apply(theme::kSeeds[g_S.iSeed].rgb, (theme::EStyle)g_S.iStyle, g_S.bDark, bAnimate);
    }

    template <typename T> T Clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }
}

settings::SSettings& settings::Get() { return g_S; }

void settings::Load()
{
    minecraft::SOptions mc = minecraft::GetOptions();
    if (const char* pszPath = Path())
        if (FILE* f = fopen(pszPath, "r"))
        {
            char szLine[64];
            while (fgets(szLine, sizeof(szLine), f))
            {
                int i = 0; float v = 0.f;
                if      (sscanf(szLine, "seed=%d",     &i) == 1) g_S.iSeed    = i;
                else if (sscanf(szLine, "style=%d",    &i) == 1) g_S.iStyle   = i;
                else if (sscanf(szLine, "dark=%d",     &i) == 1) g_S.bDark    = i != 0;
                else if (sscanf(szLine, "size=%f",     &v) == 1) g_S.fSizePct = v;
                else if (sscanf(szLine, "motion=%d",   &i) == 1) g_S.bMotion  = i != 0;
                else if (sscanf(szLine, "mc_size=%f",   &v) == 1) mc.fSize       = v;
                else if (sscanf(szLine, "mc_snap=%d",   &i) == 1) mc.bSnap       = i != 0;
                else if (sscanf(szLine, "mc_solid=%d",  &i) == 1) mc.bSolid      = i != 0;
                else if (sscanf(szLine, "mc_hide=%d",   &i) == 1) mc.bHideBehind = i != 0;
                else if (sscanf(szLine, "mc_bright=%f", &v) == 1) mc.fBrightness = v;
            }
            fclose(f);
        }

    g_S.iSeed        = Clamp(g_S.iSeed, 0, theme::kSeedCount - 1);
    g_S.iStyle       = Clamp(g_S.iStyle, 0, theme::kStyleCount - 1);
    g_S.fSizePct     = Clamp(g_S.fSizePct, 80.f, 130.f);
    mc.fSize         = Clamp(mc.fSize, 0.1f, 1.f);
    mc.fBrightness   = Clamp(mc.fBrightness, 30.f, 100.f);
    minecraft::SetOptions(mc);
    g_fScale = g_fTargetScale = g_S.fSizePct / 100.f;
    ui::g_bMotion = g_S.bMotion;
    ApplyTheme(false);
}

void settings::Changed(bool bTheme)
{
    ui::g_bMotion = g_S.bMotion;
    if (bTheme) ApplyTheme(true);
    g_bDirty = true;
}

void settings::Tick(float dt)
{
    // Пока что-то тянут (слайдер), не сохраняем и не меняем размер панели: иначе панель
    // растёт прямо под лучом, и значение слайдера "уплывает".
    if (!ImGui::IsAnyItemActive())
    {
        g_fTargetScale = g_S.fSizePct / 100.f;
        if (g_bDirty) { Save(); g_bDirty = false; }
    }
    g_fScale = g_S.bMotion ? ui::Approach(g_fScale, g_fTargetScale, 12.f, dt) : g_fTargetScale;
}

float settings::PanelScale() { return g_fScale; }
