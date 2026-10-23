#include "menu.hpp"
#include "ui.hpp"
#include "pages.hpp"
#include "keyboard.hpp"
#include "settings.hpp"
#include "theme.hpp"
#include "../config.hpp"
#include "../mods/photon.hpp"

#include "imgui.h"
#include "imgui_internal.h"

#include <stdio.h>
#include <string.h>
#include <string>

using namespace ui;

// Раскладка по канону M3 list-detail: слева навигационный список, справа панель страницы.
// Пока страница закрыта, список занимает всю ширину; при выборе сжимается влево (emphasized, 500 мс),
// а панель выезжает, прицепленная к его краю.
namespace
{
    // ---- Раскладка в dp: панель 720×540 dp, поля 24 dp (medium window class) ----
    constexpr float kMargin  = dp(24);
    constexpr float kAppBarH = dp(64);            // small top app bar
    constexpr float kTop     = kAppBarH + dp(8);
    constexpr float kItemH   = dp(56);            // строка списка в одну линию
    constexpr float kItemGap = dp(2);             // зазор между сегментами группы
    constexpr float kListW   = dp(216);           // ширина списка рядом с открытой страницей
    constexpr float kSpacer  = dp(24);            // зазор между панелями list-detail
    constexpr float kPaneBar = dp(64);            // шапка панели страницы
    constexpr float kFooterH = dp(32);            // подсказка внизу списка

    constexpr int kMaxPages = 8;
    static_assert(pages::kCount <= kMaxPages, "too many pages: raise kMaxPages in menu.cpp");

    struct SState
    {
        int   nActive = -1;    // открытая страница; -1 — список во всю ширину
        int   nShown  = 0;     // что показывает панель (нужно и пока она уезжает)
        int   nPrev   = -1;    // уходящая страница при переключении (fade through)
        float fSplit  = 0.f;   // линейный прогресс перехода список → list-detail
        float fPage   = 1.f;   // линейный прогресс смены содержимого
        float fClose  = 0.f;
        float fHot    = 0.f;
        float aHover[kMaxPages] = {}, aPress[kMaxPages] = {}, aSel[kMaxPages] = {};
    };
    SState S;

    void DrawAppBar(ImDrawList* dl, float W)
    {
        const float cy = dp(36);
        Text(dl, type::TitleLarge, ImVec2(kMargin, cy - type::TitleLarge * 0.5f), Col(role::OnSurface), "shkbd internal");

        // FPS — assist chip: 32 dp, форма small, контур outline variant; точка primary / tertiary / error
        const float fps = ImGui::GetIO().Framerate;
        char szFps[24];
        snprintf(szFps, sizeof(szFps), "%d fps", (int)(fps + 0.5f));
        const ImVec2   ts  = TextSize(type::LabelLarge, szFps);
        const float    cw  = dp(8 + 18 + 8) + ts.x + dp(16);
        const ImVec2   c1(W - kMargin, cy + dp(16)), c0(c1.x - cw, cy - dp(16));
        const unsigned dot = fps >= 68.f ? role::Primary : (fps >= 50.f ? role::Tertiary : role::Error);
        dl->AddRect(ImVec2(c0.x + dp(0.5f), c0.y + dp(0.5f)), ImVec2(c1.x - dp(0.5f), c1.y - dp(0.5f)), Col(role::OutlineVariant), shape::Small, dp(1));
        dl->AddCircleFilled(ImVec2(c0.x + dp(8 + 9), cy), dp(4), Col(dot), 16);
        Text(dl, type::LabelLarge, ImVec2(c0.x + dp(8 + 18 + 8), cy - type::LabelLarge * 0.5f), Col(role::OnSurface), szFps);
    }

    // Место пункта: основная группа — сверху, вторичная (Settings, Info) прижата к низу списка
    struct SSlot { float y; bool bFirst, bLast; };
    SSlot NavSlot(int i, float H)
    {
        int nIndex = 0, nCount = 0;
        for (int k = 0; k < pages::kCount; k++)
            if (pages::kPages[k].iGroup == pages::kPages[i].iGroup) { nIndex += k < i; nCount++; }
        const float fGroupH = (float)nCount * kItemH + (float)(nCount - 1) * kItemGap;
        const float y0      = pages::kPages[i].iGroup == 0 ? kTop : H - kMargin - kFooterH - dp(16) - fGroupH;
        return { y0 + (float)nIndex * (kItemH + kItemGap), nIndex == 0, nIndex == nCount - 1 };
    }

    void DrawNavList(ImDrawList* dl, float W, float H, float e, float dt)
    {
        const float w = Lerp(W - kMargin * 2.f, kListW, e);
        for (int i = 0; i < pages::kCount; i++)
        {
            const pages::SPage& page = pages::kPages[i];
            const SSlot  slot = NavSlot(i, H);
            const ImVec2 p0(kMargin, slot.y), p1(p0.x + w, p0.y + kItemH);

            ImGui::PushID(i);
            ImGui::SetCursorScreenPos(p0);
            const bool bPressed = ImGui::InvisibleButton("##nav", ImVec2(w, kItemH), ImGuiButtonFlags_PressedOnClick);
            const bool bHovered = ImGui::IsItemHovered();
            const bool bHeld    = ImGui::IsItemActive();
            ImGui::PopID();

            if (bPressed)
            {
                if (S.nActive == i)
                    S.nActive = -1;   // повторное нажатие — закрыть страницу
                else
                {
                    S.nPrev   = (S.nActive >= 0 && S.fSplit > 0.5f) ? S.nShown : -1;
                    S.nActive = i;
                    S.nShown  = i;
                    S.fPage   = 0.f;
                }
            }

            S.aHover[i] = Approach(S.aHover[i], bHovered ? 1.f : 0.f, 20.f, dt);
            S.aPress[i] = Approach(S.aPress[i], bHeld    ? 1.f : 0.f, 30.f, dt);
            S.aSel[i]   = MoveTowards(S.aSel[i], S.nActive == i ? 1.f : 0.f, dt / dur::Medium2);
            const float sel = Emphasized(S.aSel[i]);

            // Сегмент группы: внешние углы large, внутренние extra small. Выбранный превращается
            // в "пилюлю" цвета secondary container — активный индикатор M3.
            const float    rTop    = Lerp(slot.bFirst ? shape::Large : shape::ExtraSmall, kItemH * 0.5f, sel);
            const float    rBottom = Lerp(slot.bLast  ? shape::Large : shape::ExtraSmall, kItemH * 0.5f, sel);
            const unsigned base    = Mix(role::SurfaceContainerHigh, role::SecondaryContainer, sel);
            const unsigned content = Mix(role::OnSurface, role::OnSecondaryContainer, sel);
            const unsigned fill    = Mix(base, content, ImMax(state::Hover * S.aHover[i], state::Pressed * S.aPress[i]));
            RoundRect(dl, p0, p1, rTop, rTop, rBottom, rBottom, Col(fill));

            const float cy = (p0.y + p1.y) * 0.5f;
            Icon(dl, page.eIcon, ImVec2(p0.x + dp(16 + 12), cy), dp(24), Col(Mix(role::OnSurfaceVariant, role::OnSecondaryContainer, sel)));
            Text(dl, type::BodyLarge, ImVec2(p0.x + dp(56), cy - type::BodyLarge * 0.5f), Col(content), page.pszName);

            // chevron_right: строка ведёт на страницу; в режиме list-detail не нужен
            if (e < 0.999f)
                Icon(dl, EIcon::ChevronRight, ImVec2(p1.x - dp(16 + 12), cy), dp(24), Col(role::OnSurfaceVariant, 1.f - e));
        }
    }

    void DrawPageContent(int iPage, float fAlpha, float fOffsetY, ImVec2 p0, ImVec2 p1)
    {
        if (fAlpha <= 0.002f) return;
        const pages::SPage& page = pages::kPages[iPage];
        ImDrawList*         dl   = ImGui::GetWindowDrawList();
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * fAlpha);

        // Заголовок (title large) на одной вертикали с текстом строк
        Text(dl, type::TitleLarge, ImVec2(p0.x + dp(32), p0.y + kPaneBar * 0.5f - type::TitleLarge * 0.5f + fOffsetY), Col(role::OnSurface), page.pszName);

        // Содержимое — дочернее окно: обычная раскладка ImGui, лишнее обрезается,
        // длинную страницу листает правый стик.
        ImGui::SetCursorScreenPos(ImVec2(p0.x + dp(16), p0.y + kPaneBar + fOffsetY));
        ImGui::PushID(iPage);
        const ImVec2 childPos = ImGui::GetCursorScreenPos();
        const float  childH   = p1.y - p0.y - kPaneBar - dp(16);
        float        fScroll  = 0.f, fScrollMax = 0.f;
        if (ImGui::BeginChild("##page", ImVec2(p1.x - p0.x - dp(32), childH), ImGuiChildFlags_None,
                              ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar))
        {
            page.pfnDraw();
            fScroll    = ImGui::GetScrollY();
            fScrollMax = ImGui::GetScrollMaxY();
        }
        ImGui::EndChild();
        ImGui::PopID();

        // Тонкий индикатор прокрутки, если страница длиннее панели (листает правый стик)
        if (fScrollMax > 1.f)
        {
            const float trackT = childPos.y + dp(4), trackH = childH - dp(8);
            const float thumbH = ImMax(dp(32), trackH * childH / (childH + fScrollMax));
            const float ty     = trackT + (trackH - thumbH) * (fScroll / fScrollMax);
            const float x      = p1.x - dp(8);
            RoundRect(dl, ImVec2(x - dp(2), ty), ImVec2(x + dp(2), ty + thumbH), dp(2), dp(2), dp(2), dp(2), Col(role::Outline, 0.6f));
        }
        ImGui::PopStyleVar();
    }

    void DrawPane(ImDrawList* dl, float W, float H, float e)
    {
        if (e <= 0.002f) return;

        const float  pw = W - kMargin * 2.f - kListW - kSpacer;
        const float  x0 = kMargin + Lerp(W - kMargin * 2.f, kListW, e) + kSpacer;
        const ImVec2 p0(x0, kTop), p1(x0 + pw, H - kMargin);

        ImGui::PushClipRect(ImVec2(0.f, 0.f), ImVec2(W - kMargin + 1.f, H), true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * ImMin(1.f, e * 1.6f));
        RoundRect(dl, p0, p1, shape::Large, shape::Large, shape::Large, shape::Large, Col(role::SurfaceContainerLow));

        // Смена страницы — M3 fade through: старая гаснет за первые 30 %, новая проявляется
        // (emphasized decelerate) с небольшим подъёмом.
        if (S.nPrev >= 0 && S.fPage < 0.3f)
            DrawPageContent(S.nPrev, 1.f - S.fPage / 0.3f, 0.f, p0, p1);
        else
        {
            const float a = EmphasizedDecelerate(S.nPrev >= 0 ? (S.fPage - 0.3f) / 0.7f : S.fPage);
            DrawPageContent(S.nShown, a, dp(8) * (1.f - a), p0, p1);
        }

        // Кнопка-иконка «закрыть»: слой состояния 40 dp, иконка 24 dp, зона нажатия 48 dp
        const ImVec2 cc(p1.x - dp(12 + 20), p0.y + kPaneBar * 0.5f);
        ImGui::SetCursorScreenPos(ImVec2(cc.x - dp(24), cc.y - dp(24)));
        if (ImGui::InvisibleButton("##close", ImVec2(dp(48), dp(48)), ImGuiButtonFlags_PressedOnClick))
            S.nActive = -1;
        const float fTarget = ImGui::IsItemActive() ? 1.25f : (ImGui::IsItemHovered() ? 1.f : 0.f);   // 1.25 × 8 % = 10 %
        S.fClose = Approach(S.fClose, fTarget, 20.f, ImGui::GetIO().DeltaTime);
        if (S.fClose > 0.002f)
            dl->AddCircleFilled(cc, dp(20), Col(Mix(role::SurfaceContainerLow, role::OnSurfaceVariant, state::Hover * S.fClose)), 40);
        Icon(dl, EIcon::Close, cc, dp(24), Col(role::OnSurfaceVariant));

        ImGui::PopStyleVar();
        ImGui::PopClipRect();
    }

    void DrawFooter(ImDrawList* dl, float H)
    {
        // Подсказка внизу списка: клавиша в контурном чипе + подпись
        const char*  pszKey = cfg::ToggleButtonName();
        const ImVec2 ks     = TextSize(type::LabelLarge, pszKey);
        const float  kw     = ImMax(dp(32), ks.x + dp(24)), kh = kFooterH;
        const ImVec2 k0(kMargin, H - kMargin - kh), k1(k0.x + kw, k0.y + kh);
        dl->AddRect(ImVec2(k0.x + dp(0.5f), k0.y + dp(0.5f)), ImVec2(k1.x - dp(0.5f), k1.y - dp(0.5f)), Col(role::OutlineVariant), shape::Small, dp(1));
        Text(dl, type::LabelLarge, ImVec2(k0.x + (kw - ks.x) * 0.5f, k0.y + (kh - type::LabelLarge) * 0.5f), Col(role::OnSurface), pszKey);
        Text(dl, type::BodyMedium, ImVec2(k1.x + dp(12), k0.y + (kh - type::BodyMedium) * 0.5f), Col(role::OnSurfaceVariant), "Open / close menu");
    }

    void DrawCursor()
    {
        ImGuiIO& io = ImGui::GetIO();
        S.fHot = Approach(S.fHot, ImGui::IsAnyItemHovered() ? 1.f : 0.f, 20.f, io.DeltaTime);
        if (!ImGui::IsMousePosValid(&io.MousePos)) return;

        // Кольцо primary с тёмной обводкой (видно и на светлых элементах) и точка в центре;
        // над кликабельным кольцо чуть больше, при нажатии сжимается.
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        const float r  = io.MouseDown[0] ? dp(7) : dp(10) + dp(2) * S.fHot;
        fg->AddCircle(io.MousePos, r + dp(1.5f), Col(0x000000, 0.35f), 40, dp(1));
        fg->AddCircle(io.MousePos, r, Col(role::Primary), 40, dp(2));
        fg->AddCircleFilled(io.MousePos, dp(3), Col(role::OnSurface), 16);
    }
}

void menu::Setup()
{
    LoadFont();
    settings::Load();   // тема, анимации, размер панели, скорость полёта

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding    = ImVec2(0.f, 0.f);
    style.WindowBorderSize = 0.f;
    style.WindowRounding   = 0.f;
    style.ChildBorderSize  = 0.f;
    style.ItemSpacing      = ImVec2(0.f, dp(2));   // зазор между сегментами списка
}

void menu::Draw(float fVisibility)
{
    ImGuiIO& io = ImGui::GetIO();
    // Текстура выше панели (снизу полоса спрайтов луча) — раскладка по размеру панели
    const float W = (float)cfg::kPanelResX, H = (float)cfg::kPanelResY, dt = AnimDt();
    WarmupGlyphs();
    theme::Tick(dt);
    settings::Tick(io.DeltaTime);

    // M3 motion: список ↔ list-detail — emphasized, 500 мс; смена содержимого — 300 мс
    S.fSplit = MoveTowards(S.fSplit, S.nActive >= 0 ? 1.f : 0.f, dt / dur::Long2);
    S.fPage  = MoveTowards(S.fPage, 1.f, dt / dur::Medium2);
    if (S.fPage >= 1.f) S.nPrev = -1;
    const float e = Emphasized(S.fSplit);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImClamp(fVisibility, 0.f, 1.f));
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
    ImGui::SetNextWindowSize(ImVec2(W, H));
    ImGui::Begin("##shkbd", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    RoundRect(dl, ImVec2(0.f, 0.f), ImVec2(W, H), shape::ExtraLarge, shape::ExtraLarge, shape::ExtraLarge, shape::ExtraLarge, Col(role::Surface));
    DrawAppBar(dl, W);
    DrawNavList(dl, W, H, e, dt);
    DrawPane(dl, W, H, e);
    DrawFooter(dl, H);

    ImGui::End();
    keyboard::Draw(W, H);   // экранная клавиатура — отдельным окном поверх меню
    DrawCursor();
    ImGui::PopStyleVar();
}

void menu::DrawSprites()
{
    ImDrawList* fg = ImGui::GetForegroundDrawList();

    // Луч: поперечный профиль — затухающие края и светлая сердцевина; по длине картинка растягивается
    {
        const float x = (float)cfg::kBeamSprite[0], y = (float)cfg::kBeamSprite[1];
        const float w = (float)cfg::kBeamSprite[2], h = (float)cfg::kBeamSprite[3], cx = x + w * 0.5f;
        const ImU32 edge = Col(role::Primary, 0.f), glow = Col(role::Primary, 0.85f);
        fg->AddRectFilledMultiColor(ImVec2(x, y),  ImVec2(cx, y + h),    edge, glow, glow, edge);
        fg->AddRectFilledMultiColor(ImVec2(cx, y), ImVec2(x + w, y + h), glow, edge, edge, glow);
        fg->AddRectFilled(ImVec2(cx - 3.f, y), ImVec2(cx + 3.f, y + h), Col(role::OnPrimaryContainer));
    }

    // Прицел: кольцо primary с тёмной обводкой (видно на светлых стенах) и точка в центре
    {
        const ImVec2 c((float)cfg::kReticleSprite[0] + (float)cfg::kReticleSprite[2] * 0.5f,
                       (float)cfg::kReticleSprite[1] + (float)cfg::kReticleSprite[3] * 0.5f);
        fg->AddCircle(c, 44.f, Col(0x000000, 0.35f), 64, 16.f);
        fg->AddCircle(c, 44.f, Col(role::Primary), 64, 8.f);
        fg->AddCircleFilled(c, 12.f, Col(0x000000, 0.35f), 24);
        fg->AddCircleFilled(c, 8.f, Col(role::OnSurface), 24);
    }

    // Шар платформы: ImGui-круги в цветах темы, будто рука застряла в полупрозрачной сфере
    {
        const ImVec2 c((float)cfg::kBallSprite[0] + (float)cfg::kBallSprite[2] * 0.5f,
                       (float)cfg::kBallSprite[1] + (float)cfg::kBallSprite[3] * 0.5f);
        const float  R = 58.f;
        fg->AddCircleFilled(c, R, Col(role::PrimaryContainer, 0.45f), 64);
        for (int i = 1; i <= 6; i++)   // объём: светлеющие круги, смещённые к блику
        {
            const float t = (float)i / 6.f;
            fg->AddCircleFilled(ImVec2(c.x - 14.f * t, c.y - 16.f * t), R * (1.f - 0.75f * t), Col(role::Primary, 0.10f), 48);
        }
        fg->AddCircle(c, R - 2.f, Col(role::Primary, 0.9f), 64, 4.f);
        fg->AddCircleFilled(ImVec2(c.x - 22.f, c.y - 24.f), 9.f, Col(0xFFFFFF, 0.55f), 24);
    }

    // Подпись: что заспавнится — плашка inverse surface, как tooltip в M3
    {
        const std::string sItem = photon::GetArmedItem().sName;
        const char*       psz   = strrchr(sItem.c_str(), '/') ? strrchr(sItem.c_str(), '/') + 1 : sItem.c_str();
        const float  x = (float)cfg::kLabelSprite[0], y = (float)cfg::kLabelSprite[1];
        const float  w = (float)cfg::kLabelSprite[2], h = (float)cfg::kLabelSprite[3];
        const ImVec2 ts = TextSize(type::TitleLarge, psz);
        const float  pw = ImMin(w, ts.x + 64.f), ph = 80.f;
        const ImVec2 p0(x + (w - pw) * 0.5f, y + (h - ph) * 0.5f), p1(p0.x + pw, p0.y + ph);
        RoundRect(fg, p0, p1, ph * 0.5f, ph * 0.5f, ph * 0.5f, ph * 0.5f, Col(role::InverseSurface));
        fg->PushClipRect(ImVec2(p0.x + 24.f, p0.y), ImVec2(p1.x - 24.f, p1.y), true);
        Text(fg, type::TitleLarge, ImVec2(ts.x > pw - 64.f ? p0.x + 32.f : p0.x + (pw - ts.x) * 0.5f, p0.y + (ph - type::TitleLarge) * 0.5f),
             Col(role::InverseOnSurface), psz);
        fg->PopClipRect();
    }
}
